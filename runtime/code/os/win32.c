#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <shellscalingapi.h>
#include <shlobj.h>
#include <pathcch.h>

#define CINTERFACE
#define COBJMACROS
#define CONST_VTABLE
#include <MMDeviceAPI.h>
#include <AudioClient.h>

// @todo: maybe i should switch to xaudio2 but its probably not any better with the com crap they
// shove on top of every new winapi these days
//

const CLSID CLSID_MMDeviceEnumerator = {
    0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e}
};

const IID IID_IMMDeviceEnumerator = {
    0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}
};

const IID IID_IAudioClient = {
    0x1cb9ad4c, 0xdbfa, 0x4c32, {0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2}
};

const IID IID_IAudioRenderClient = {
    0xf294acfc, 0x3146, 0x4483, {0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2}
};

#define REFTIMES_PER_SEC      (10000000)
#define REFTIMES_PER_MILLISEC (10000)

#include <stdio.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "pathcch.lib")
#pragma comment(lib, "synchronization.lib")

#define XI_MAX_TITLE_COUNT 1024

typedef struct xiWin32WindowData {
    // this is required to be the same as the renderer-side structures, not to be modified
    // :renderer_core
    //
    HINSTANCE hInstance;
    HWND hwnd;
} xiWin32WindowData;

typedef struct xiWin32DisplayInfo {
    HMONITOR handle;

    xi_b32 is_primary;

    // this is to get the actual device name
    //
    LPWSTR name; // \\.\DISPLAY[n]

    RECT bounds;
    xi_u32 dpi; // raw dpi value

    xiDisplay xi;
} xiWin32DisplayInfo;

// context containing any required win32 information
//
typedef struct xiWin32Context {
    xiContext xi;

    volatile xi_b32 running; // for whether the _application_ is even running

    xiArena arena;

    xi_b32 quitting;

    xi_u32 display_count;
    xiWin32DisplayInfo displays[XI_MAX_DISPLAYS];

    // timing info
    //
    xi_b32 did_update;

    xi_u64 counter_start;
    xi_u64 counter_freq;

    xi_s64 accum_ns; // signed just in-case, due to clamp on delta time we don't need the range
    xi_u64 fixed_ns;
    xi_u64 clamp_ns;

    struct {
        HWND handle;

        LONG windowed_style; // style before entering fullscreen
        WINDOWPLACEMENT placement;

        xi_b32 user_resizing;

        xi_u32 dpi;

        xi_u32    last_state;
        xi_buffer title;
    } window;

    struct {
        xi_b32 enabled;

        IAudioClient *client;
        IAudioRenderClient *renderer;
        xi_u32 frame_count;
    } audio;

    struct {
        xi_b32 valid;
        HMODULE lib;

        xiRendererInit *init;
    } renderer;

    struct {
        xi_u64 last_time;

        HMODULE dll;
        LPWSTR  dll_src_path; // where the game dll is compiled to
        LPWSTR  dll_dst_path; // where the game dll is copied to prevent file locking

        xiGameCode *code;
    } game;
} xiWin32Context;

// :note win32 memory allocation functions
//
xi_uptr xi_os_allocation_granularity_get() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    xi_uptr result = info.dwAllocationGranularity;
    return result;
}

xi_uptr xi_os_page_size_get() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    xi_uptr result = info.dwPageSize;
    return result;
}

void *xi_os_virtual_memory_reserve(xi_uptr size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
    return result;
}

xi_b32 xi_os_virtual_memory_commit(void *base, xi_uptr size) {
    xi_b32 result = VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE) != 0;
    return result;
}

void xi_os_virtual_memory_decommit(void *base, xi_uptr size) {
    VirtualFree(base, size, MEM_DECOMMIT);
}

void xi_os_virtual_memory_release(void *base, xi_uptr size) {
    (void) size; // :note not needed by VirtualFree when releasing memory

    VirtualFree(base, 0, MEM_RELEASE);
}

// :note win32 threading
//
XI_INTERNAL DWORD win32_thread_proc(LPVOID arg) {
    xiThreadQueue *queue = (xiThreadQueue *) arg;
    xi_thread_run(queue);

    return 0;
}

void xi_os_thread_start(xiThreadQueue *arg) {
    DWORD id;
    HANDLE handle = CreateThread(0, 0, win32_thread_proc, (void *) arg, 0, &id);
    CloseHandle(handle);
}

void xi_os_futex_wait(xiFutex *futex, xiFutex value) {
    for (;;) {
        WaitOnAddress(futex, (PVOID) &value, sizeof(value), INFINITE);
        if (*futex != value) { break; }
    }
}

void xi_os_futex_signal(xiFutex *futex) {
    WakeByAddressSingle((PVOID) futex);
}

void xi_os_futex_broadcast(xiFutex *futex) {
    WakeByAddressAll((PVOID) futex);
}

//
// :note utility functions
//

XI_INTERNAL DWORD win32_wstr_count(LPCWSTR wstr) {
    DWORD result = 0;
    while (wstr[result] != L'\0') {
        result += 1;
    }

    return result;
}

XI_INTERNAL BOOL win32_wstr_equal(LPWSTR a, LPWSTR b) {
    BOOL result = TRUE;

    while (*a++ && *b++) {
        if (a[0] != b[0]) {
            result = false;
            break;
        }
    }

    return result;
}

XI_INTERNAL LPWSTR win32_utf8_to_utf16(xi_string str) {
    LPWSTR result = 0;

    DWORD count = MultiByteToWideChar(CP_UTF8, 0, (LPCCH) str.data, (int) str.count, 0, 0);
    if (count) {
        xiArena *temp = xi_temp_get();

        result = xi_arena_push_array(temp, WCHAR, count + 1);

        MultiByteToWideChar(CP_UTF8, 0, (LPCCH) str.data, (int) str.count, result, count);
        result[count] = 0;
    }

    return result;
}

XI_INTERNAL xi_string win32_utf16_to_utf8(LPWSTR wstr) {
    xi_string result = { 0 };

    DWORD count = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, 0, 0, 0, 0);
    if (count) {
        xiArena *temp = xi_temp_get();

        result.count = count - 1;
        result.data  = xi_arena_push_array(temp, xi_u8, count);

        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, (LPSTR) result.data, count, 0, 0);
    }

    return result;
}

//
// :note window and display management
//
XI_INTERNAL LONG win32_window_style_get_for_state(HWND window, xi_u32 state) {
    LONG result = GetWindowLongW(window, GWL_STYLE);

    switch (state) {
        case XI_WINDOW_STATE_WINDOWED_RESIZABLE: {
            result |= WS_OVERLAPPEDWINDOW;
        }
        break;
        case XI_WINDOW_STATE_WINDOWED: {
            result |= WS_OVERLAPPEDWINDOW;
            result &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }
        break;
        case XI_WINDOW_STATE_WINDOWED_BORDERLESS:
        case XI_WINDOW_STATE_FULLSCREEN: {
            result &= ~WS_OVERLAPPEDWINDOW;
        }
        break;

        // do nothing
        //
        default: {} break;
    }

    return result;
}

XI_INTERNAL LRESULT CALLBACK win32_main_window_handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    xiWin32Context *context = (xiWin32Context *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (message) {
        case WM_CLOSE: {
            if (context) { context->running = false; }
        }
        break;
        case WM_ENTERSIZEMOVE: {
            if (context) {
                context->window.user_resizing = true;
            }
        }
        break;
        case WM_EXITSIZEMOVE: {
            if (context) {
                context->window.user_resizing = false;
            }
        }
        break;
        case WM_DPICHANGED: {
            if (context) {
                LPRECT rect = (LPRECT) lParam;
                LONG style  = GetWindowLongW(hwnd, GWL_STYLE);
                WORD dpi    = LOWORD(wParam);

                int x = rect->left;
                int y = rect->top;

                AdjustWindowRectExForDpi(rect, style, FALSE, 0, dpi);

                int width  = (rect->right - rect->left);
                int height = (rect->bottom - rect->top);

                UINT flags = SWP_NOOWNERZORDER | SWP_NOZORDER;

                SetWindowPos(hwnd, HWND_TOP, x, y, width, height, flags);

                context->window.dpi = dpi;
            }
        }
        break;
        case WM_GETDPISCALEDSIZE: {
            if (context) {
                // we have to interact with this message instead of just using the standard LPRECT
                // provided to WM_DPICHANGED because the client resolutions it provides are incorrect
                //
                LONG dpi   = (LONG) wParam;
                PSIZE size = (PSIZE) lParam;

                RECT client_rect;
                GetClientRect(hwnd, &client_rect);

                xi_f32 scale = (xi_f32) dpi / context->window.dpi;

                // may produce off by 1px errors when scaling odd resolutions
                //
                size->cx = (LONG) (scale * (client_rect.right - client_rect.left));
                size->cy = (LONG) (scale * (client_rect.bottom - client_rect.top));

                result = TRUE;
            }
        }
        break;
        default: {
            result = DefWindowProcW(hwnd, message, wParam, lParam);
        }
        break;
    }

    return result;
}

XI_INTERNAL BOOL CALLBACK win32_displays_enumerate(HMONITOR hMonitor,
        HDC hdcMonitor, LPRECT lpRect, LPARAM lParam)
{
    BOOL result = FALSE;

    (void) lpRect;
    (void) hdcMonitor;

    xiWin32Context *context = (xiWin32Context *) lParam;

    // @todo: i need to store the rect bounds of each monitors instead of just the information we
    // pass back to the game becasue they will be used to center windows and place windows
    // on different displays depending on init parameters
    //

    if (context->display_count < XI_MAX_DISPLAYS) {
        result = TRUE;

        xiWin32DisplayInfo *info = &context->displays[context->display_count];

        MONITORINFOEXW monitor_info = { 0 };
        monitor_info.cbSize = sizeof(MONITORINFOEXW);

        if (GetMonitorInfoW(hMonitor, (LPMONITORINFO) &monitor_info)) {
            UINT xdpi, ydpi;
            if (GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi) == S_OK) {
                info->xi.scale = (xi_f32) xdpi / 96.0f;
                info->bounds   = monitor_info.rcMonitor;
                info->dpi      = xdpi;

                DEVMODEW mode = { 0 };
                mode.dmSize = sizeof(DEVMODEW);
                if (EnumDisplaySettingsW(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &mode)) {
                    info->handle = hMonitor;

                    xi_uptr count = win32_wstr_count(monitor_info.szDevice) + 1;
                    info->name = xi_arena_push_copy_array(&context->arena, monitor_info.szDevice, WCHAR, count);

                    info->xi.width  = mode.dmPelsWidth;
                    info->xi.height = mode.dmPelsHeight;

                    info->xi.refresh_rate = (xi_f32) mode.dmDisplayFrequency;

                    info->is_primary = (monitor_info.dwFlags & MONITORINFOF_PRIMARY) != 0;

                    context->display_count += 1;
                }
            }
        }
    }

    return result;
}

XI_INTERNAL void win32_display_info_get(xiWin32Context *context) {
    EnumDisplayMonitors(0, 0, win32_displays_enumerate, (LPARAM) &context->xi);

    xiArena *temp = xi_temp_get();

    xi_u32 path_count = 0;
    xi_u32 mode_count = 0;

    DISPLAYCONFIG_PATH_INFO *paths = 0;
    DISPLAYCONFIG_MODE_INFO *modes = 0;

    xi_u32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;

    LONG result;

    do {
        result = GetDisplayConfigBufferSizes(flags, &path_count, &mode_count);
        if (result == ERROR_SUCCESS) {
            paths = xi_arena_push_array(temp, DISPLAYCONFIG_PATH_INFO, path_count);
            modes = xi_arena_push_array(temp, DISPLAYCONFIG_MODE_INFO, mode_count);

            result = QueryDisplayConfig(flags, &path_count, paths, &mode_count, modes, 0);
        }
    }
    while (result == ERROR_INSUFFICIENT_BUFFER);

    if (result == ERROR_SUCCESS) {
        for (xi_u32 it = 0; it < path_count; ++it) {
            DISPLAYCONFIG_PATH_INFO *path = &paths[it];

            DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name = { 0 };
            source_name.header.adapterId = path->sourceInfo.adapterId;
            source_name.header.id        = path->sourceInfo.id;
            source_name.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
            source_name.header.size      = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);

            result = DisplayConfigGetDeviceInfo(&source_name.header);
            if (result == ERROR_SUCCESS) {
                LPWSTR gdi_name = source_name.viewGdiDeviceName;

                DISPLAYCONFIG_TARGET_DEVICE_NAME target_name = { 0 };
                target_name.header.adapterId = path->targetInfo.adapterId;
                target_name.header.id        = path->targetInfo.id;
                target_name.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
                target_name.header.size      = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);

                result = DisplayConfigGetDeviceInfo(&target_name.header);
                if (result == ERROR_SUCCESS) {
                    LPWSTR monitor_name = target_name.monitorFriendlyDeviceName;

                    for (xi_u32 d = 0; d < context->display_count; ++d) {
                        xiWin32DisplayInfo *display = &context->displays[d];
                        if (win32_wstr_equal(display->name, gdi_name)) {
                            xi_string name = win32_utf16_to_utf8(monitor_name);
                            display->xi.name = xi_str_copy(&context->arena, name);
                        }
                    }
                }
            }
        }
    }

    xiContext *xi = &context->xi;

    xi->system.display_count = context->display_count;

    for (xi_u32 it = 0; it < context->display_count; ++it) {
        xi->system.displays[it] = context->displays[it].xi;
    }
}

//
// :note file system and path management
//
enum xiWin32PathType {
    WIN32_PATH_TYPE_EXECUTABLE = 0,
    WIN32_PATH_TYPE_WORKING,
    WIN32_PATH_TYPE_USER,
    WIN32_PATH_TYPE_TEMP
};

// :note allocated paths are stored within the temporary arena, if a path is required to be
// permanent it must be copied elsewhere, this is not recommended however as certain paths can be
// changed through external means leading to stale path referencing
//
// a path will be in 'win32' form, that is null-terminated, encoded in utf-16 with backslashes as
// the path segment separator 'win32_path_convert_to_api' should be called on all 'win32' strings
// that need to be used by the user facing runtime api. this function will convert from utf-16 to
// utf-8 and will change all path segment separators to forward slashes, it will not, however,
// remove the volume letter designator from win32 paths, as without this there is no way to know if
// certain paths reside on other drives than the working drive
//
XI_INTERNAL LPWSTR win32_system_path_get(xi_u32 type) {
    LPWSTR result = 0;

    xiArena *temp = xi_temp_get();

    switch (type) {
        case WIN32_PATH_TYPE_EXECUTABLE: {
            DWORD buffer_count = MAX_PATH;

            // get the executable filename, windows doesn't just tell us how long the path
            // is so we have to do this loop until we create a buffer that is big enough, in
            // most cases MAX_PATH will suffice
            //
            DWORD count;
            do {
                result = xi_arena_push_array(temp, WCHAR, buffer_count);
                count  = GetModuleFileNameW(0, result, buffer_count);

                buffer_count *= 2;
            } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

            // find the last backslash path separator
            //
            while (count--) {
                if (result[count] == L'\\') {
                    break;
                }
            }

            // null terminate the string
            //
            result[count] = L'\0';

            // @todo: pop the unused buffer amount but it requires a change to the above loop
            //
        }
        break;
        case WIN32_PATH_TYPE_WORKING: {
            result = xi_arena_push_array(temp, WCHAR, MAX_PATH + 1);

            // the documentation tells you not to use this in multi-threaded applications due to the
            // path being stored in a global variable, while this is technically a multi-threaded
            // application it currently only has one "main" thread so it'll be fine unless windows
            // is reading/writing with its own threads in the background
            //
            // +1 for null-terminating character
            //
            DWORD count = GetCurrentDirectoryW(MAX_PATH + 1, result);
            if (count > MAX_PATH) {
                xi_arena_pop_last(temp);

                result = xi_arena_push_array(temp, WCHAR, count);
                GetCurrentDirectoryW(count, result);
            }
            else {
                xi_arena_pop_array(temp, WCHAR, MAX_PATH - count);
            }
        }
        break;
        case WIN32_PATH_TYPE_USER: {
            PWSTR path;
            if (SHGetKnownFolderPath(&FOLDERID_RoamingAppData, 0, 0, &path) == S_OK) {
                DWORD len = win32_wstr_count(path) + 1;
                result    = xi_arena_push_copy_array(temp, path, WCHAR, len);
            }

            CoTaskMemFree(path);
        }
        break;
        case WIN32_PATH_TYPE_TEMP: {
            // according to the documentation the temp path cannot exceed MAX_PATH excluding null
            // terminating byte, so MAX_PATH + 1 in total
            //
            // the documentation also stated GetTempPath2W should be called instead of GetTempPathW, even
            // though for non-SYSTEM level processes they are functionally identical, but it wouldn't
            // locate the function in kerne32.dll, requires a relatively new build of windows 10 or
            // windows 11 so it can't be relied on
            //
            result = xi_arena_push_array(temp, WCHAR, MAX_PATH + 1);
            DWORD count = GetTempPathW(MAX_PATH + 1, result);
            if (count) {
                result[count - 1] = L'\0'; // remove the final backslash as we don't need it
                xi_arena_pop_array(temp, WCHAR, MAX_PATH - count + 1);
            }
        }
        break;
        default: {} break;
    }

    return result;
}

XI_INTERNAL void win32_path_convert_from_api_buffer(WCHAR *out, xi_uptr cch_limit, xi_string path) {
    LPWSTR wpath = win32_utf8_to_utf16(path);
    if (wpath) {
        WCHAR *start = wpath;
        while (start[0]) {
            if (start[0] == L'/') { start[0] = L'\\'; }
            start += 1;
        }

        xi_b32 requires_volume = (path.count >= 1) && (path.data[0] == '/');
        xi_b32 is_absolute     = (path.count >= 3) &&
                                 ((path.data[0] >= 'A' && path.data[0]  <= 'Z') ||
                                 (path.data[0] >= 'a'  && path.data[0]  >= 'z')) &&
                                 (path.data[1] == ':') && (path.data[2] == '/');

        if (is_absolute) {
            // this checks if there is a drive letter on the path, if there is we take the path
            // and canonicalize it to simplify the path removing and relative reference
            //
            PathCchCanonicalizeEx(out, cch_limit, wpath, PATHCCH_ALLOW_LONG_PATHS);
        }
        else if (requires_volume) {
            // if it doesn't contain a drive letter but has a leading forward slash the path
            // is absolute but requires us to append the correct volume letter from the current working
            // directory
            //
            WCHAR volume[MAX_PATH + 1];
            GetVolumePathNameW(wpath, volume, MAX_PATH + 1);

            PathCchCombineEx(out, cch_limit, volume, wpath, PATHCCH_ALLOW_LONG_PATHS);
        }
        else {
            // assume some sort of relative path so combine with the working directory,
            // PathCchCombine will resolve any relative '.' and '..' inputs for us
            //
            LPWSTR working = win32_system_path_get(WIN32_PATH_TYPE_WORKING);
            PathCchCombineEx(out, cch_limit, working, wpath, PATHCCH_ALLOW_LONG_PATHS);
        }
    }
}

// this function assumes you are supplying a 'win32' absolute path
//
XI_INTERNAL xi_string win32_path_convert_to_api(xiArena *arena, WCHAR *wpath) {
    xi_string path = win32_utf16_to_utf8(wpath);

    for (xi_uptr it = 0; it < path.count; ++it) {
        if (path.data[it] == '\\') { path.data[it] = '/'; }
    }

    xi_string result = xi_str_copy(arena, path);
    return result;
}

void win32_directory_list_builder_get_recursive(xiArena *arena,
        xiDirectoryListBuilder *builder, LPWSTR path, xi_uptr path_limit, xi_b32 recursive)
{
    xiArena *temp = xi_temp_get();
    xi_string base = win32_path_convert_to_api(temp, path);

    PathCchAppendEx(path, path_limit, L"*", PATHCCH_ALLOW_LONG_PATHS);

    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW(path, &data);

    if (find != INVALID_HANDLE_VALUE) {
        do {
            // ignore hidden files and relative directory entries
            //
            // all files/directories that start with '.' will be ignored as these are the
            // default semantics for hidden files on unix-like platforms, windows doesn't always
            // explicitly mark these as hidden though
            //
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0)  { continue; }
            if (data.cFileName[0] == L'.') { continue; } // :cf

            FILETIME time = data.ftLastWriteTime;
            xi_b32 is_dir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            xi_string name = win32_utf16_to_utf8(data.cFileName);
            xi_string full_path = xi_str_format(arena, "%.*s/%.*s", xi_str_unpack(base), xi_str_unpack(name));

            // we know there is going to be at least one forward slash because of the format xi_string above
            //
            xi_uptr last_slash = 0;
            xi_str_find_last(full_path, &last_slash, '/');

            xi_string basename = xi_str_advance(full_path, last_slash + 1);

            xi_uptr last_dot = 0;
            if (xi_str_find_last(basename, &last_dot, '.')) {
                // dot may not be found in the case of directory names and files without extensions
                //
                basename = xi_str_prefix(basename, last_dot);
            }

            xiDirectoryEntry entry;
            entry.type     = is_dir ? XI_DIRECTORY_ENTRY_TYPE_DIRECTORY : XI_DIRECTORY_ENTRY_TYPE_FILE;
            entry.path     = full_path;
            entry.basename = basename;
            entry.size     = ((xi_u64) data.nFileSizeHigh  << 32) | ((xi_u64) data.nFileSizeLow);
            entry.time     = ((xi_u64) time.dwHighDateTime << 32) | ((xi_u64) time.dwLowDateTime);

            xi_directory_list_builder_append(builder, &entry);

            if (recursive && is_dir) {
                PathCchRemoveFileSpec(path, path_limit); // remove \\* for search
                PathCchAppendEx(path, path_limit, data.cFileName, PATHCCH_ALLOW_LONG_PATHS);

                win32_directory_list_builder_get_recursive(arena, builder, path, path_limit, recursive);

                PathCchRemoveFileSpec(path, path_limit);
            }
        }
        while (FindNextFileW(find, &data) != FALSE);

        FindClose(find);
    }
}

void xi_os_directory_list_build(xiArena *arena,
        xiDirectoryListBuilder *builder, xi_string path, xi_b32 recursive)
{
    WCHAR wpath[PATHCCH_MAX_CCH];
    win32_path_convert_from_api_buffer(wpath, PATHCCH_MAX_CCH, path);

    win32_directory_list_builder_get_recursive(arena, builder, wpath, PATHCCH_MAX_CCH, recursive);
}

xi_b32 xi_os_directory_entry_get_by_path(xiArena *arena, xiDirectoryEntry *entry, xi_string path) {
    xi_b32 result = false;

    WCHAR wpath[PATHCCH_MAX_CCH];
    win32_path_convert_from_api_buffer(wpath, PATHCCH_MAX_CCH, path);

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExW(wpath, GetFileExInfoStandard, &data)) {
        FILETIME time = data.ftLastWriteTime;
        xi_b32 is_dir = ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

        xi_string full_path = win32_path_convert_to_api(arena, wpath);
        xi_string basename = full_path;

        xi_uptr last_slash = 0;
        if (xi_str_find_last(full_path, &last_slash, '/')) {
            basename = xi_str_advance(basename, last_slash + 1);
        }

        xi_uptr last_dot = 0;
        if (xi_str_find_last(basename, &last_dot, '.')) {
            basename = xi_str_prefix(basename, last_dot);
        }

        entry->type     = is_dir ? XI_DIRECTORY_ENTRY_TYPE_DIRECTORY : XI_DIRECTORY_ENTRY_TYPE_FILE;
        entry->path     = full_path;
        entry->basename = basename;
        entry->size     = ((xi_u64) data.nFileSizeHigh  << 32) | ((xi_u64) data.nFileSizeLow);
        entry->time     = ((xi_u64) time.dwHighDateTime << 32) | ((xi_u64) time.dwLowDateTime);

        result = true;
    }

    return result;
}

xi_b32 xi_os_directory_create(xi_string path) {
    xi_b32 result = true;

    WCHAR wpath[PATHCCH_MAX_CCH];
    win32_path_convert_from_api_buffer(wpath, PATHCCH_MAX_CCH, path);

    xi_uptr it = 0;
    while (wpath[it] != 0) {
        if (wpath[it] == L'\\') {
            wpath[it] = L'\0'; // temporarily null terminate at this segment

            if (!CreateDirectoryW(wpath, 0)) {
                DWORD error = GetLastError();
                if (error != ERROR_ALREADY_EXISTS) {
                    result = false;
                    break;
                }
            }

            wpath[it] = L'\\'; // revert back and continue on
        }

        it += 1;
    }

    if (result) {
        // will return false if the path already exists, we get the file attributes
        // of the path to check if it is a directory, if not, return false
        //
        if (!CreateDirectoryW(wpath, 0)) {
            DWORD error = GetLastError();
            if (error == ERROR_ALREADY_EXISTS) {
                DWORD attrs = GetFileAttributesW(wpath);
                result = ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
            }
        }
    }

    return result;
}

void xi_os_directory_delete(xi_string path) {
    WCHAR wpath[PATHCCH_MAX_CCH];
    win32_path_convert_from_api_buffer(wpath, PATHCCH_MAX_CCH, path);

    RemoveDirectoryW(wpath);
}

xi_b32 xi_os_file_open(xiFileHandle *handle, xi_string path, xi_u32 access) {
    xi_b32 result = true;

    handle->status = XI_FILE_HANDLE_STATUS_VALID;
    handle->os     = 0;

    WCHAR wpath[PATHCCH_MAX_CCH];
    win32_path_convert_from_api_buffer(wpath, PATHCCH_MAX_CCH, path);

    DWORD access_flags = 0;
    DWORD creation     = 0;

    if (access & XI_FILE_ACCESS_FLAG_READ) {
        access_flags |= GENERIC_READ;
        creation      = OPEN_EXISTING;
    }

    if (access & XI_FILE_ACCESS_FLAG_WRITE) {
        access_flags |= GENERIC_WRITE;
        creation      = OPEN_ALWAYS;
    }

    HANDLE hFile = CreateFileW(wpath, access_flags, FILE_SHARE_READ, 0, creation, FILE_ATTRIBUTE_NORMAL, 0);

    *(HANDLE *) &handle->os = hFile;

    if (hFile == INVALID_HANDLE_VALUE) {
        handle->status = XI_FILE_HANDLE_STATUS_FAILED_OPEN;
        result = false;
    }

    return result;
}

void xi_os_file_close(xiFileHandle *handle) {
    HANDLE hFile = *(HANDLE *) &handle->os;

    if (hFile != INVALID_HANDLE_VALUE) { CloseHandle(hFile); }
    handle->status = XI_FILE_HANDLE_STATUS_CLOSED;
}

void xi_os_file_delete(xi_string path) {
    WCHAR wpath[PATHCCH_MAX_CCH];
    win32_path_convert_from_api_buffer(wpath, PATHCCH_MAX_CCH, path);

    DeleteFileW(wpath);
}

xi_b32 xi_os_file_read(xiFileHandle *handle, void *dst, xi_uptr offset, xi_uptr size) {
    xi_b32 result = false;

    if (handle->status == XI_FILE_HANDLE_STATUS_VALID) {
        result = true;

        HANDLE hFile = *(HANDLE *) &handle->os;

        XI_ASSERT(hFile != INVALID_HANDLE_VALUE);
        XI_ASSERT(offset != XI_FILE_OFFSET_APPEND);

        OVERLAPPED overlapped = { 0 };
        overlapped.Offset     = (DWORD) offset;
        overlapped.OffsetHigh = (DWORD) (offset >> 32);

        xi_u8   *data   = (xi_u8 *) dst;
        xi_uptr to_read = size;

        do {
            DWORD nread = 0;
            if (ReadFile(hFile, data, (DWORD) to_read, &nread, &overlapped)) {
                to_read -= nread;

                data   += nread;
                offset += nread;

                overlapped.Offset     = (DWORD) offset;
                overlapped.OffsetHigh = (DWORD) (offset >> 32);
            }
            else {
                result = false;
                handle->status = XI_FILE_HANDLE_STATUS_FAILED_READ;
            }
        } while ((handle->status == XI_FILE_HANDLE_STATUS_VALID) && (to_read > 0));
    }

    return result;
}

xi_b32 xi_os_file_write(xiFileHandle *handle, void *src, xi_uptr offset, xi_uptr size) {
    xi_b32 result = false;

    if (handle->status == XI_FILE_HANDLE_STATUS_VALID) {
        result = true;

        HANDLE hFile = *(HANDLE *) &handle->os;

        XI_ASSERT(hFile != INVALID_HANDLE_VALUE);

        OVERLAPPED *overlapped = 0;
        OVERLAPPED overlapped_offset = { 0 };

        if (offset == XI_FILE_OFFSET_APPEND) {
            // we are appending so move to the end of the file and don't specify an overlapped structure,
            // this is technically invalid to call on the stdout/stderr handles but it seems to work
            // correctly regardless so we don't have to do any detection to prevent against it
            //
            SetFilePointer(hFile, 0, 0, FILE_END);
        }
        else {
            overlapped_offset.Offset     = (DWORD) offset;
            overlapped_offset.OffsetHigh = (DWORD) (offset >> 32);

            overlapped = &overlapped_offset;
        }

        xi_u8   *data    = (xi_u8 *) src;
        xi_uptr to_write = size;

        do {
            DWORD nwritten = 0;
            if (WriteFile(hFile, data, (DWORD) to_write, &nwritten, overlapped)) {
                to_write -= nwritten;

                data   += nwritten;
                offset += nwritten;

                if (overlapped) {
                    overlapped->Offset     = (DWORD) offset;
                    overlapped->OffsetHigh = (DWORD) (offset >> 32);
                }
            }
            else {
                handle->status = XI_FILE_HANDLE_STATUS_FAILED_WRITE;
                result = false;
            }
        } while ((handle->status == XI_FILE_HANDLE_STATUS_VALID) && (to_write > 0));
    }

    return result;
}

//
// :note game code management
//

XI_INTERNAL void win32_game_code_init(xiWin32Context *context) {
    xiGameCode *game = context->game.code;

    xiArena *temp = xi_temp_get();

    if (game->dynamic) {
        xiDirectoryList full_list = { 0 };
        xi_directory_list_get(temp, &full_list, context->xi.system.executable_path, false);

        xiDirectoryList dll_list = { 0 };
        xi_directory_list_filter_for_extension(temp, &dll_list, &full_list, xi_str_wrap_const(".dll"));

        // @todo: there was some weird behaviour happening here when searching for a valid .dll
        // using LoadLibrary would work fine, the reference count for the lib would be one but calling
        // FreeLibrary would never decrement the libs reference count thus never actually closing it.
        //
        // FreeLibrary would return TRUE and GetLastError() would be zero, so the system _thinks_ it
        // is removing the library, but then actually doesn't. i have no idea what caused this
        // this causes the main .dll file to be locked preventing the compiler from writing to it breaking
        // the whole dynamic code reloading system.
        //
        // as this is a possibility, maybe what we should do is copy to a random location for each search
        // .dll and load it there to see if it is valid. if it is we select the source path as our .dll to
        // reload and keep the one we copied to loaded.
        // then on realoding we close the lib, but always copy the new .dll to a different random path,
        // this way it doesn't matter if the lib failed to close or not because we are at a different path
        //
        // :weird_dll
        //

        xi_b32 found = false;
        for (xi_u32 it = 0; !found && (it < dll_list.count); ++it) {
            xiDirectoryEntry *entry = &dll_list.entries[it];

            WCHAR wpath[PATHCCH_MAX_CCH];
            win32_path_convert_from_api_buffer(wpath, PATHCCH_MAX_CCH, entry->path);

            HMODULE lib = LoadLibraryW(wpath);
            if (lib) {
                xiGameInit     *init     = (xiGameInit *)     GetProcAddress(lib, "__xi_game_init");
                xiGameSimulate *simulate = (xiGameSimulate *) GetProcAddress(lib, "__xi_game_simulate");
                xiGameRender   *render   = (xiGameRender *)   GetProcAddress(lib, "__xi_game_render");

                if (init != 0 && simulate != 0 && render != 0) {
                    // we found a valid .dll that exports the functions specified so we can
                    // close the loaded library and save the path to it
                    //
                    // @todo: maybe we should select the one with the latest time if there are
                    // multiple .dlls available?
                    //
                    DWORD count = win32_wstr_count(wpath) + 1;
                    context->game.dll_src_path =
                        xi_arena_push_copy_array(&context->arena, wpath, WCHAR, count);

                    found = true;
                }

                FreeLibrary(lib);
            }
        }

        if (found) {
            // get temporary path to copy dll file to
            //
            LPWSTR temp_path = win32_system_path_get(WIN32_PATH_TYPE_TEMP);

            WCHAR dst_path[MAX_PATH + 1];
            if (GetTempFileNameW(temp_path, L"xie", 0x1234, dst_path)) {
                DWORD count = win32_wstr_count(dst_path) + 1;
                context->game.dll_dst_path = xi_arena_push_copy_array(&context->arena, dst_path, WCHAR, count);
            }

            if (CopyFileW(context->game.dll_src_path, context->game.dll_dst_path, FALSE)) {
                context->game.dll = LoadLibraryW(context->game.dll_dst_path);

                if (context->game.dll) {
                    game->init     =     (xiGameInit *) GetProcAddress(context->game.dll, "__xi_game_init");
                    game->simulate = (xiGameSimulate *) GetProcAddress(context->game.dll, "__xi_game_simulate");
                    game->render   =   (xiGameRender *) GetProcAddress(context->game.dll, "__xi_game_render");

                    if (game_code_is_valid(game)) {
                        WIN32_FILE_ATTRIBUTE_DATA attributes;
                        if (GetFileAttributesExW(context->game.dll_src_path,
                                    GetFileExInfoStandard, &attributes))
                        {
                            context->game.last_time =
                                ((xi_u64) attributes.ftLastWriteTime.dwLowDateTime  << 32) |
                                ((xi_u64) attributes.ftLastWriteTime.dwHighDateTime << 0);

                        }
                    }
                }
            }
        }
        else {
            // @todo: logging...
            //
        }

        if (!game->init)     { game->init     = __xi_game_init;     }
        if (!game->simulate) { game->simulate = __xi_game_simulate; }
        if (!game->render)   { game->render   = __xi_game_render;   }
    }
}

XI_INTERNAL void win32_game_code_reload(xiWin32Context *context) {
    // @todo: we should really be reading the .dll file to see if it has debug information. this is
    // because the .pdb file location is hardcoded into the debug data directory. when debugging on
    // some debuggers (like microsoft visual studio) it will lock the .pdb file and prevent the compiler
    // from writing to it causing a whole host of issues
    //
    // there are several ways around this, loading the .dll patching the .pdb path to something unique
    // is the "proper" way, you can also specify the /pdb linker flag with msvc.
    //
    // :weird_dll this has the same issue as above, but kinda in reverse. if the system doesn't
    // respect our request to close our temporary dll it will fail to copy, thus we shoul really be
    // using something with a unique number
    //
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    if (GetFileAttributesExW(context->game.dll_src_path, GetFileExInfoStandard, &attributes)) {
        xi_u64 time = ((xi_u64) attributes.ftLastWriteTime.dwLowDateTime  << 32) |
                      ((xi_u64) attributes.ftLastWriteTime.dwHighDateTime << 0);

        if (time != context->game.last_time) {
            xiGameCode *game = context->game.code;

            if (context->game.dll) {
                FreeLibrary(context->game.dll);

                context->game.dll = 0;
                game->init        = 0;
                game->simulate    = 0;
                game->render      = 0;
            }

            if (CopyFileW(context->game.dll_src_path, context->game.dll_dst_path, FALSE)) {
                HMODULE dll = LoadLibraryW(context->game.dll_dst_path);

                if (dll) {
                    context->game.dll = dll;

                    game->init     =     (xiGameInit *) GetProcAddress(dll, "__xi_game_init");
                    game->simulate = (xiGameSimulate *) GetProcAddress(dll, "__xi_game_simulate");
                    game->render   =   (xiGameRender *) GetProcAddress(dll, "__xi_game_render");

                    if (game_code_is_valid(game)) {
                        context->game.last_time = time;

                        // call init again but signal to the developer that the code was reloaded
                        // rather than initially called
                        //
                        game->init(&context->xi, XI_GAME_RELOADED);
                    }
                }
            }

            if (!game->init)     { game->init     = __xi_game_init;     }
            if (!game->simulate) { game->simulate = __xi_game_simulate; }
            if (!game->render)   { game->render   = __xi_game_render;   }
        }
    }
}

//
// :note sound functions
//
XI_INTERNAL void win32_audio_initialise(xiWin32Context *context) {
    xiAudioPlayer *player = &context->xi.audio_player;
    xi_audio_player_init(&context->arena, player);

    // initialise wasapi
    //
    REFERENCE_TIME duration =
        (REFERENCE_TIME) ((player->frame_count / (xi_f32) player->sample_rate) * REFTIMES_PER_SEC);

    IAudioClient *audio_client = 0;
    IAudioRenderClient *audio_renderer = 0;
    xi_u32 frame_count = 0;

    IMMDeviceEnumerator *enumerator = 0;

    HRESULT hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0,
            CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void **) &enumerator);

    if (SUCCEEDED(hr)) {
        IMMDevice *device = 0;

        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device);
        if (SUCCEEDED(hr)) {
            hr = IMMDevice_Activate(device, &IID_IAudioClient, CLSCTX_ALL, 0, (void **) &audio_client);
            if (SUCCEEDED(hr)) {
                WAVEFORMATEX wav_format = { 0 };
                wav_format.wFormatTag      = WAVE_FORMAT_PCM;
                wav_format.nChannels       = 2;
                wav_format.nSamplesPerSec  = player->sample_rate;
                wav_format.wBitsPerSample  = 16;
                wav_format.nBlockAlign     = (wav_format.wBitsPerSample * wav_format.nChannels) / 8;
                wav_format.nAvgBytesPerSec = wav_format.nBlockAlign * wav_format.nSamplesPerSec;
                wav_format.cbSize          = 0;
                hr = IAudioClient_Initialize(audio_client, AUDCLNT_SHAREMODE_SHARED,
                        AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM, duration, 0, &wav_format, 0);

                if (SUCCEEDED(hr)) {
                    hr = IAudioClient_GetBufferSize(audio_client, &frame_count);

                    if (SUCCEEDED(hr)) {
                        hr = IAudioClient_GetService(audio_client,
                                &IID_IAudioRenderClient, (void **) &audio_renderer);

                        if (SUCCEEDED(hr)) {
                            hr = IAudioClient_Start(audio_client);
                            context->audio.enabled = SUCCEEDED(hr);
                        }
                    }
                }
            }
        }
    }

    if (context->audio.enabled) {
        context->audio.client      = audio_client;
        context->audio.renderer    = audio_renderer;
        context->audio.frame_count = frame_count;
    }
}

XI_INTERNAL void win32_audio_mix(xiWin32Context *context) {
    if (context->audio.enabled) {
        xiContext *xi = &context->xi;

        // wow non-insane audio api, good job microsoft!
        //

        xi_u32 queued_frames = 0;
        HRESULT hr = IAudioClient_GetCurrentPadding(context->audio.client, &queued_frames);
        if (SUCCEEDED(hr)) {
            XI_ASSERT(queued_frames <= context->audio.frame_count);

            xi_u32 frame_count = context->audio.frame_count - queued_frames;
            xi_u8 *samples = 0;

            if (frame_count > 0) {
                hr = IAudioRenderClient_GetBuffer(context->audio.renderer, frame_count, &samples);
                if (SUCCEEDED(hr)) {
                    xi_f32 dt = (xi_f32) xi->time.delta.s;
                    xi_audio_player_update(&xi->audio_player, &xi->assets, (xi_s16 *) samples, frame_count, dt);

                    IAudioRenderClient_ReleaseBuffer(context->audio.renderer, frame_count, 0);
                }
            }
        }
    }
}

// :note input handling functions
//
XI_GLOBAL xi_u8 tbl_win32_virtual_key_to_input_key[] = {
    // @todo: switch to raw input for usb hid scancodes instead of using this virtual keycode
    // lookup table
    //
    [VK_RETURN]     = XI_KEYBOARD_KEY_RETURN,    [VK_ESCAPE]    = XI_KEYBOARD_KEY_ESCAPE,
    [VK_BACK]       = XI_KEYBOARD_KEY_BACKSPACE, [VK_TAB]       = XI_KEYBOARD_KEY_TAB,
    [VK_HOME]       = XI_KEYBOARD_KEY_HOME,      [VK_PRIOR]     = XI_KEYBOARD_KEY_PAGEUP,
    [VK_DELETE]     = XI_KEYBOARD_KEY_DELETE,    [VK_END]       = XI_KEYBOARD_KEY_END,
    [VK_NEXT]       = XI_KEYBOARD_KEY_PAGEDOWN,  [VK_RIGHT]     = XI_KEYBOARD_KEY_RIGHT,
    [VK_LEFT]       = XI_KEYBOARD_KEY_LEFT,      [VK_DOWN]      = XI_KEYBOARD_KEY_DOWN,
    [VK_UP]         = XI_KEYBOARD_KEY_UP,        [VK_F1]        = XI_KEYBOARD_KEY_F1,
    [VK_F2]         = XI_KEYBOARD_KEY_F2,        [VK_F3]        = XI_KEYBOARD_KEY_F3,
    [VK_F4]         = XI_KEYBOARD_KEY_F4,        [VK_F5]        = XI_KEYBOARD_KEY_F5,
    [VK_F6]         = XI_KEYBOARD_KEY_F6,        [VK_F7]        = XI_KEYBOARD_KEY_F7,
    [VK_F8]         = XI_KEYBOARD_KEY_F8,        [VK_F9]        = XI_KEYBOARD_KEY_F9,
    [VK_F10]        = XI_KEYBOARD_KEY_F10,       [VK_F11]       = XI_KEYBOARD_KEY_F11,
    [VK_F12]        = XI_KEYBOARD_KEY_F12,       [VK_INSERT]    = XI_KEYBOARD_KEY_INSERT,
    [VK_SPACE]      = XI_KEYBOARD_KEY_SPACE,     [VK_OEM_3]     = XI_KEYBOARD_KEY_QUOTE,
    [VK_OEM_COMMA]  = XI_KEYBOARD_KEY_COMMA,     [VK_OEM_MINUS] = XI_KEYBOARD_KEY_MINUS,
    [VK_OEM_PERIOD] = XI_KEYBOARD_KEY_PERIOD,    [VK_OEM_2]     = XI_KEYBOARD_KEY_SLASH,

    // these are just direct mappings
    //
    ['0'] = XI_KEYBOARD_KEY_0, ['1'] = XI_KEYBOARD_KEY_1, ['2'] = XI_KEYBOARD_KEY_2,
    ['3'] = XI_KEYBOARD_KEY_3, ['4'] = XI_KEYBOARD_KEY_4, ['5'] = XI_KEYBOARD_KEY_5,
    ['6'] = XI_KEYBOARD_KEY_6, ['7'] = XI_KEYBOARD_KEY_7, ['8'] = XI_KEYBOARD_KEY_8,
    ['9'] = XI_KEYBOARD_KEY_9,

    [VK_OEM_1] = XI_KEYBOARD_KEY_SEMICOLON, [VK_OEM_PLUS] = XI_KEYBOARD_KEY_EQUALS,

    [VK_LSHIFT] = XI_KEYBOARD_KEY_LSHIFT, [VK_LCONTROL] = XI_KEYBOARD_KEY_LCTRL,
    [VK_LMENU]  = XI_KEYBOARD_KEY_LALT,

    [VK_RSHIFT] = XI_KEYBOARD_KEY_RSHIFT, [VK_RCONTROL] = XI_KEYBOARD_KEY_RCTRL,
    [VK_RMENU]  = XI_KEYBOARD_KEY_RALT,

    [VK_OEM_4] = XI_KEYBOARD_KEY_LBRACKET, [VK_OEM_7] = XI_KEYBOARD_KEY_BACKSLASH,
    [VK_OEM_6] = XI_KEYBOARD_KEY_RBRACKET, [VK_OEM_8] = XI_KEYBOARD_KEY_GRAVE,

    // remap uppercase to lowercase
    //
    ['A'] = XI_KEYBOARD_KEY_A, ['B'] = XI_KEYBOARD_KEY_B, ['C'] = XI_KEYBOARD_KEY_C,
    ['D'] = XI_KEYBOARD_KEY_D, ['E'] = XI_KEYBOARD_KEY_E, ['F'] = XI_KEYBOARD_KEY_F,
    ['G'] = XI_KEYBOARD_KEY_G, ['H'] = XI_KEYBOARD_KEY_H, ['I'] = XI_KEYBOARD_KEY_I,
    ['J'] = XI_KEYBOARD_KEY_J, ['K'] = XI_KEYBOARD_KEY_K, ['L'] = XI_KEYBOARD_KEY_L,
    ['M'] = XI_KEYBOARD_KEY_M, ['N'] = XI_KEYBOARD_KEY_N, ['O'] = XI_KEYBOARD_KEY_O,
    ['P'] = XI_KEYBOARD_KEY_P, ['Q'] = XI_KEYBOARD_KEY_Q, ['R'] = XI_KEYBOARD_KEY_R,
    ['S'] = XI_KEYBOARD_KEY_S, ['T'] = XI_KEYBOARD_KEY_T, ['U'] = XI_KEYBOARD_KEY_U,
    ['V'] = XI_KEYBOARD_KEY_V, ['W'] = XI_KEYBOARD_KEY_W, ['X'] = XI_KEYBOARD_KEY_X,
    ['Y'] = XI_KEYBOARD_KEY_Y, ['Z'] = XI_KEYBOARD_KEY_Z,
};

//
// :note update pass
//

XI_INTERNAL void win32_xi_context_update(xiWin32Context *context) {
    xiContext *xi = &context->xi;

    // timing informations
    //
    LARGE_INTEGER time;
    if (QueryPerformanceCounter(&time)) {

        xi_u64 counter_end = time.QuadPart;
        xi_u64 delta_ns = (1000000000 * (counter_end - context->counter_start)) / context->counter_freq;

        // don't let delta time exceed the maximum specified
        //
        delta_ns = XI_MIN(delta_ns, context->clamp_ns);

        context->accum_ns += delta_ns;

        xi->time.ticks = counter_end;
        context->counter_start = counter_end;

        // delta time is fixed, set it here just in-case the user changes it for whatever reason
        // it'll recover the next update. however, if they're doing that already it will probably
        // produce wonky results and there isn't really much we can do about it anyway
        //
        xi->time.delta.ns = context->fixed_ns;
        xi->time.delta.us = (context->fixed_ns / 1000);
        xi->time.delta.ms = (context->fixed_ns / 1000000);
        xi->time.delta.s  = (context->fixed_ns / 1000000000.0);

        // @todo: should this just be a straight summation here, or should we increment these
        // each time we call simulate with the fixed hz
        //
        xi->time.total.ns += (delta_ns);
        xi->time.total.us  = (xi_u64) ((xi->time.total.ns / 1000.0) + 0.5);
        xi->time.total.ms  = (xi_u64) ((xi->time.total.ns / 1000000.0) + 0.5);
        xi->time.total.s   = (xi->time.total.ns / 1000000000.0);
    }

    win32_audio_mix(context);

    // process windows messages that our application received
    //
    // @todo: input reset call?
    //
    xiInputMouse *mouse = &xi->mouse;
    xiInputKeyboard *kb = &xi->keyboard;

    // @todo: this might require some more thought with the multiple update situation, there could be
    // places where if across a frame boundary where an update did not occur due to the timing being
    // faster than the fixed step we could have a button event that has both 'pressed' and 'released' on the
    // same frame. i guess this is possible anyway even with perfect timing if the polling rate is high enough?
    //
    // we could have a single update pass that is always called but shouldn't be used for physics
    // or whatever needs fixed timesteps, maybe we could use that for input handling and this issue goes away
    //
    // otherwise maybe it really is just best to have an event driven system that can be handled in the
    // simulate function, as it won't do anything if multiple updates happened (assuming all events were
    // consumed on the first iteration) and it can also easily queue up new events when no update happened
    //
    // :step_input
    //
    //
    if (context->did_update) {
        // reset mouse params for this frame
        //
        mouse->connected    = true;
        mouse->active       = false;
        mouse->delta.screen = xi_v2s_create(0, 0);
        mouse->delta.clip   =  xi_v2_create(0, 0);
        mouse->delta.wheel  =  xi_v2_create(0, 0);

        mouse->left.pressed   = mouse->left.released   = false;
        mouse->middle.pressed = mouse->middle.released = false;
        mouse->right.pressed  = mouse->right.released  = false;
        mouse->x0.pressed     = mouse->x0.released     = false;
        mouse->x1.pressed     = mouse->x1.released     = false;

        // reset keyboard params for this frame
        //
        kb->connected = true;
        kb->active    = false;

        for (xi_u32 it = 0; it < XI_KEYBOARD_KEY_COUNT; ++it) {
            kb->keys[it].pressed  = false;
            kb->keys[it].released = false;
        }
    }

    context->did_update = false;

    MSG msg;
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
        switch (msg.message) {
            // keyboard input
            //
            case WM_KEYUP:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN: {
                xi_b32 down = (msg.lParam & (1 << 31)) == 0;

                // we have to do some pre-processing to split up the left and right variants of
                // control keys
                //
                switch (msg.wParam) {
                    case VK_MENU: {
                        xiInputButton *left  = &kb->keys[XI_KEYBOARD_KEY_LALT];
                        xiInputButton *right = &kb->keys[XI_KEYBOARD_KEY_RALT];

                        xi_input_button_handle(left,  GetKeyState(VK_LMENU) & 0x8000);
                        xi_input_button_handle(right, GetKeyState(VK_RMENU) & 0x8000);

                        kb->alt = (left->down || right->down);
                    }
                    break;
                    case VK_CONTROL: {
                        // @todo: windows posts a ctrl+alt message for altgr for legacy reasons
                        // as its platform-specific we don't really want to say the ctrl key
                        // is 'pressed' when right alt is pressed (altgr on some keyboards) so it
                        // should be filtered, but i need to come up with a sane way of figuring out
                        // if it was posted by windows rather than pressed explicitly by the user
                        //
                        xiInputButton *left  = &kb->keys[XI_KEYBOARD_KEY_LCTRL];
                        xiInputButton *right = &kb->keys[XI_KEYBOARD_KEY_RCTRL];

                        xi_input_button_handle(left,  GetKeyState(VK_LCONTROL) & 0x8000);
                        xi_input_button_handle(right, GetKeyState(VK_RCONTROL) & 0x8000);

                        kb->ctrl = (left->down || right->down);
                    }
                    break;
                    case VK_SHIFT: {
                        xiInputButton *left  = &kb->keys[XI_KEYBOARD_KEY_LSHIFT];
                        xiInputButton *right = &kb->keys[XI_KEYBOARD_KEY_RSHIFT];

                        xi_input_button_handle(left,  GetKeyState(VK_LSHIFT) & 0x8000);
                        xi_input_button_handle(right, GetKeyState(VK_RSHIFT) & 0x8000);

                        kb->shift = (left->down || right->down);
                    }
                    break;
                    default: {
                        xi_u32 key_index   = tbl_win32_virtual_key_to_input_key[msg.wParam];
                        xiInputButton *key = &kb->keys[key_index];

                        xi_input_button_handle(key, down);
                    }
                    break;
                }

                kb->active = true;
            }
            break;


            // left mouse button
            //
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP: {
                xi_b32 down = (msg.message == WM_LBUTTONDOWN);
                xi_input_button_handle(&mouse->left, down);

                mouse->active = true;
            }
            break;

            // middle mouse button
            //
            case WM_MBUTTONUP:
            case WM_MBUTTONDOWN: {
                xi_b32 down = (msg.message == WM_MBUTTONDOWN);
                xi_input_button_handle(&mouse->middle, down);

                mouse->active = true;
            }
            break;

            // right mouse button
            //
            case WM_RBUTTONUP:
            case WM_RBUTTONDOWN: {
                xi_b32 down = (msg.message == WM_RBUTTONDOWN);
                xi_input_button_handle(&mouse->right, down);

                mouse->active = true;
            }
            break;

            // extra mouse buttons, windows doesn't differentiate x0 and x1 at the event level
            //
            case WM_XBUTTONUP:
            case WM_XBUTTONDOWN: {
                xi_b32 down = (msg.message == WM_XBUTTONDOWN);

                if (msg.wParam & MK_XBUTTON1) { xi_input_button_handle(&mouse->x0, down); }
                if (msg.wParam & MK_XBUTTON2) { xi_input_button_handle(&mouse->x1, down); }

                mouse->active = true;
            }
            break;

            case WM_MOUSEMOVE: {
                POINTS pt = MAKEPOINTS(msg.lParam);

                xi_v2s old_screen = mouse->position.screen;
                xi_v2  old_clip   = mouse->position.clip;

                // update the screen space position
                //
                mouse->position.screen = xi_v2s_create(pt.x, pt.y);

                // update the clip space position
                //
                mouse->position.clip.x = -1.0f + (2.0f * (pt.x / (xi_f32) xi->window.width));
                mouse->position.clip.y =  1.0f - (2.0f * (pt.y / (xi_f32) xi->window.height));

                // calculate the deltas, a summation in case we get more than one mouse move
                // event within a single frame
                //
                xi_v2s delta_screen = xi_v2s_sub(mouse->position.screen, old_screen);
                xi_v2  delta_clip   =  xi_v2_sub(mouse->position.clip,   old_clip);

                mouse->delta.screen = xi_v2s_add(mouse->delta.screen, delta_screen);
                mouse->delta.clip   =  xi_v2_add(mouse->delta.clip,   delta_clip);

                mouse->active = true;
            }
            break;

            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL: {
                xi_b32 vertical = (msg.message == WM_MOUSEWHEEL);
                xi_f32 delta    = GET_WHEEL_DELTA_WPARAM(msg.wParam) / (xi_f32) WHEEL_DELTA;

                if (vertical) {
                    mouse->position.wheel.y += delta;
                    mouse->delta.wheel.y    += delta;
                }
                else {
                    mouse->position.wheel.x += delta;
                    mouse->delta.wheel.x    += delta;
                }

                mouse->active = true;
            }
            break;
        }
    }

    // if context->running is false that means we have recieved a WM_CLOSE message in our window proc
    // which means the user has pressed the close button etc. so we signal to the game code that
    // we are quitting, which can be used to do processing after the fact, such as saving game state etc.
    //
    // we have a duplicated 'quitting' variable to prevent the user from overriding this decision as the
    // window is now closing, the user can still set xi->quit to true to manucally close the engine
    // from their game code
    //
    context->quitting = !context->running;
    xi->quit = context->quitting;

    // check for any changes to the xiContext structure which may have been issued during
    // user game code execution
    //
    if (!context->window.user_resizing) {
        // :note we do not allow the application to manually set the window size if the user
        // is resizing the window as it goes against the intent of the user, causes odd
        // visual glitches and prevents the window from being truly resizable if the application
        // is going to overwrite the resized dimension anyway
        //
        // if the application programmer wants to be able to specify strict window sizes it is
        // recommended they do not create a resizable window and can then specify the size
        // by setting xi->window.width and xi->window.height respectivley.
        //
        // this also extends to the user moving the window
        //
        HWND hwnd = context->window.handle;

        if (xi->window.state != XI_WINDOW_STATE_FULLSCREEN) {
            // :note change the window resoluation if the application has modified
            // it directly, this is ignored when the window is in fullscreen mode
            //
            RECT client_rect;
            GetClientRect(hwnd, &client_rect);

            xi_u32 width  = (client_rect.right  - client_rect.left);
            xi_u32 height = (client_rect.bottom - client_rect.top);

            xi_f32 current_scale = (xi_f32) context->window.dpi / USER_DEFAULT_SCREEN_DPI;

            xi_u32 desired_width  = (xi_u32) (current_scale * xi->window.width);
            xi_u32 desired_height = (xi_u32) (current_scale * xi->window.height);

            if (width != desired_width || height != desired_height) {
                LONG style       = GetWindowLongW(hwnd, GWL_STYLE);
                HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

                UINT xdpi, ydpi;
                if (GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi) == S_OK) {
                    client_rect.left   = 0;
                    client_rect.top    = 0;
                    client_rect.right  = xi->window.width;
                    client_rect.bottom = xi->window.height;

                    if (AdjustWindowRectExForDpi(&client_rect, style, FALSE, 0, xdpi)) {
                        UINT flags = SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOZORDER;

                        width  = client_rect.right - client_rect.left;
                        height = client_rect.bottom - client_rect.top;

                        SetWindowPos(hwnd, HWND_TOP, 0, 0, width, height, flags);
                    }
                }
            }
        }

        if (xi->window.state != context->window.last_state) {
            // :note change the window style if the application has requested
            // it to be modified
            //
            LONG new_style = win32_window_style_get_for_state(hwnd, xi->window.state);

            xi_s32 x = 0, y = 0;
            xi_s32 w = 0, h = 0;
            UINT flags = SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER;

            if (context->window.last_state == XI_WINDOW_STATE_FULLSCREEN) {
                // we are leaving fullscreen so restore the window position to where it was before
                // entering fullscreen
                //
                SetWindowPlacement(hwnd, &context->window.placement);

                // we have to restore the style to back before we went into fullscreen otherwise
                // the client area will include the old decorations area making it slightly too big
                //
                SetWindowLongW(hwnd, GWL_STYLE, context->window.windowed_style);
                SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, flags);
            }

            if (xi->window.state == XI_WINDOW_STATE_FULLSCREEN) {
                // we are entering fullscreen so remember where our window was in windowed mode
                // in case we transition back to windowed mode
                //
                // we know that if the above is true this can't happen because of the outer if statement
                // checking if xi->window.state != context->window.last_state
                //
                if (GetWindowPlacement(hwnd, &context->window.placement)) {
                    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

                    MONITORINFO info = { 0 };
                    info.cbSize = sizeof(MONITORINFO);

                    if (GetMonitorInfoA(monitor, &info)) {
                        x = info.rcMonitor.left;
                        y = info.rcMonitor.top;

                        w = (info.rcMonitor.right - info.rcMonitor.left);
                        h = (info.rcMonitor.bottom - info.rcMonitor.top);

                        flags &= ~(SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

                        context->window.windowed_style = GetWindowLongW(hwnd, GWL_STYLE);
                    }
                }
            }
            else {
                // we are back in windowed mode, however, due to client-side decorations we _may_
                // need to re-adjust the window size, for example going from windowed -> fullscreen ->
                // windowed borderless would yield the wrong window size using only the placement saved
                // before entering fullscreen as it has a window size that takes the previously
                // visible decorations into account
                //
                RECT client_rect;
                GetClientRect(hwnd, &client_rect);

                HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

                UINT xdpi, ydpi;
                if (GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi) == S_OK) {
                    if (AdjustWindowRectExForDpi(&client_rect, new_style, FALSE, 0, xdpi)) {
                        w = (client_rect.right - client_rect.left);
                        h = (client_rect.bottom - client_rect.top);

                        flags &= ~(SWP_NOSIZE | SWP_NOZORDER);
                    }
                }
            }

            SetWindowLongW(hwnd, GWL_STYLE, new_style);
            SetWindowPos(hwnd, HWND_TOP, x, y, w, h, flags);

            context->window.last_state = xi->window.state;
        }
    }

    if (!xi_str_equal(xi->window.title, context->window.title.str)) {
        // :note update window title if it has changed
        //
        context->window.title.used = XI_MIN(xi->window.title.count, context->window.title.limit);
        xi_memory_copy(context->window.title.data, xi->window.title.data, context->window.title.used);

        LPWSTR new_title = win32_utf8_to_utf16(xi->window.title);
        SetWindowTextW(context->window.handle, new_title);
    }

    // update window state
    //
    RECT client_area;
    GetClientRect(context->window.handle, &client_area);

    xi_u32 width  = (client_area.right - client_area.left);
    xi_u32 height = (client_area.bottom - client_area.top);

    if (width != 0 && height != 0) {
        xi->window.width  = width;
        xi->window.height = height;

        xi->renderer.setup.window_dim.w = xi->window.width;
        xi->renderer.setup.window_dim.h = xi->window.height;
    }

    xi->window.title = context->window.title.str;

    // update the index of the display that the window is currently on
    //
    HMONITOR monitor = MonitorFromWindow(context->window.handle, MONITOR_DEFAULTTONEAREST);
    for (xi_u32 it = 0; it < context->display_count; ++it) {
        if (context->displays[it].handle == monitor) {
            xi->window.display = it;
            break;
        }
    }

    xiGameCode *game = context->game.code;
    if (game->dynamic) {
        // reload the code if it needs to be
        //
        win32_game_code_reload(context);
    }
}

XI_INTERNAL DWORD WINAPI win32_game_thread(LPVOID param) {
    DWORD result = 0;

    xiWin32Context *context = (xiWin32Context *) param;

    xiContext *xi = &context->xi;

    xi->version.major = XI_VERSION_MAJOR;
    xi->version.minor = XI_VERSION_MINOR;
    xi->version.patch = XI_VERSION_PATCH;

    {
        // setup the stdout file handle for use with loggers
        //
        // this may change between XI_ENGINE_CONFIGURE and XI_GAME_INIT onwards in the event
        // the application is compiled with subsystem windows (thus not having a console) and
        // the user requesting a console to be opened and having output re-directed through
        // a new handle
        //
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);

        xiFileHandle *out = &xi->system.out;
        xiFileHandle *err = &xi->system.err;

        *(HANDLE *) &out->os = hOut;
        *(HANDLE *) &err->os = hErr;

        out->status = (hOut == INVALID_HANDLE_VALUE) ?
            XI_FILE_HANDLE_STATUS_FAILED_OPEN : XI_FILE_HANDLE_STATUS_VALID;

        err->status = (hErr == INVALID_HANDLE_VALUE) ?
            XI_FILE_HANDLE_STATUS_FAILED_OPEN : XI_FILE_HANDLE_STATUS_VALID;
    }

    // setup title buffer
    //
    context->window.title.used  = 0;
    context->window.title.limit = XI_MAX_TITLE_COUNT;
    context->window.title.data  = xi_arena_push_array(&context->arena, xi_u8, XI_MAX_TITLE_COUNT);

    LPWSTR exe_path     = win32_system_path_get(WIN32_PATH_TYPE_EXECUTABLE);
    LPWSTR temp_path    = win32_system_path_get(WIN32_PATH_TYPE_TEMP);
    LPWSTR user_path    = win32_system_path_get(WIN32_PATH_TYPE_USER);
    LPWSTR working_path = win32_system_path_get(WIN32_PATH_TYPE_WORKING);

    xi->system.executable_path = win32_path_convert_to_api(&context->arena, exe_path);
    xi->system.temp_path       = win32_path_convert_to_api(&context->arena, temp_path);
    xi->system.user_path       = win32_path_convert_to_api(&context->arena, user_path);
    xi->system.working_path    = win32_path_convert_to_api(&context->arena, working_path);

    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);

    xi->system.processor_count = system_info.dwNumberOfProcessors;

    win32_game_code_init(context);

    xiGameCode *game = context->game.code;

    // call pre-init for engine system configurations
    //
    // @todo: this could, and probably should, just be replaced with a configuration file in
    // the future. or could just have both a pre-init call and a configuration file
    //
    game->init(xi, XI_ENGINE_CONFIGURE);

    // open console if requested
    //
    if (xi->system.console_open) {
        xi->system.console_open = AttachConsole(ATTACH_PARENT_PROCESS);
        if (!xi->system.console_open) {
            xi->system.console_open = AllocConsole();
        }

        if (xi->system.console_open) {
            // this only seems to work for the main executable and not the loaded game
            // code .dll, i guess the .dll inherits the stdout/stderr handles when it is loaded
            // and they don't get updated after the fact
            //
            // this is backed up by the fact that once the game code is dynamically reloaded it
            // starts working...
            //
            // :hacky_console
            //
            xiFileHandle *out = &xi->system.out;
            xiFileHandle *err = &xi->system.err;

            HANDLE hOutput = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

            *(HANDLE *) &out->os = hOutput;
            *(HANDLE *) &err->os = hOutput;

            if (hOutput != INVALID_HANDLE_VALUE) {
                // update the handles to reflect the redirected ones
                //
                SetStdHandle(STD_OUTPUT_HANDLE, hOutput);
                SetStdHandle(STD_ERROR_HANDLE,  hOutput);

                out->status = err->status = XI_FILE_HANDLE_STATUS_VALID;

                // we are just going to reload the .dll in place if a console was opened, its
                // not a massive deal and only happens once, as the console is mainly used for debugging
                // this probably won't even happen in a final game anyway
                //
                // :hacky_console
                //
                // @todo: should i even bother with this, it is only needed for printf to work but as we
                // have our own logging system in place and that writes directly to the file handle we
                // specify the updated stdout/stderr handles are reflected without the need to reload
                // the .dll
                //
                // context->game.last_time = 0;
                // @todo: reenable at some point win32_game_code_reload(context);
            }
            else {
                out->status = err->status = XI_FILE_HANDLE_STATUS_FAILED_OPEN;
            }
        }
    }

    //
    // configure window
    //
    HWND hwnd = context->window.handle;

    // show the window now it has been configured to be visible
    //
    ShowWindowAsync(hwnd, SW_SHOW);

    xi_u32 display_index = XI_MIN(xi->window.display, xi->system.display_count);
    xiWin32DisplayInfo *display = &context->displays[display_index];

    xi_f32 scale = display->xi.scale;

    UINT swp_flags = (SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);

    if (xi->window.width == 0 || xi->window.height == 0) {
        if (xi->window.display == 0) {
            // the user didn't move the window from the default display and also didn't specify a
            // size that they wanted, as the window was created with the correct defaults we don't
            // need to do anything so add these flags in to prevent SetWindowPos from altering
            // the window position/size
            //
            swp_flags |= (SWP_NOSIZE | SWP_NOMOVE);
        }

        xi->window.width  = 1280;
        xi->window.height = 720;
    }


    RECT rect;
    rect.left   = 0;
    rect.top    = 0;
    rect.right  = (LONG) (scale * xi->window.width);
    rect.bottom = (LONG) (scale * xi->window.height);

    // set the window size scaled correctly
    //
    xi->window.width  = rect.right;
    xi->window.height = rect.bottom;

    int X, Y;
    int nWidth, nHeight;
    DWORD dwStyle;

    // we are setting up the window initially as if it were a non-fullscreen window, this allows us
    // to grab the initial window placement before entering fullscreen so we don't get odd results
    // if the user decides to leave fullscreen after the fact
    //
    // :fullscreen_window
    //
    xi_u32 state = xi->window.state;
    if (state == XI_WINDOW_STATE_FULLSCREEN) { state = XI_WINDOW_STATE_WINDOWED; }

    dwStyle = win32_window_style_get_for_state(hwnd, state);

    if (AdjustWindowRectExForDpi(&rect, dwStyle, FALSE, 0, display->dpi)) {
        nWidth  = (rect.right - rect.left);
        nHeight = (rect.bottom - rect.top);
    }
    else {
        nWidth  = xi->window.width;
        nHeight = xi->window.height;
    }

    // center the window on the selected display
    //
    X = display->bounds.left + ((xi_s32) (display->xi.width  - nWidth)  >> 1);
    Y = display->bounds.top  + ((xi_s32) (display->xi.height - nHeight) >> 1);

    // as the window was previously created with default parameters we can just select the correct
    // dimensions specified by the user and then set the window position/size to that, this
    // also handles the fact that windows will ignore borderless styles when initially creating a window
    //
    SetWindowLongW(hwnd, GWL_STYLE, dwStyle);
    SetWindowPos(hwnd, HWND_TOP, X, Y, nWidth, nHeight, swp_flags);

    if (xi_str_is_valid(xi->window.title)) {
        xi->window.title.count = XI_MIN(xi->window.title.count, context->window.title.limit);

        LPWSTR title = win32_utf8_to_utf16(xi->window.title);
        SetWindowTextW(hwnd, title);

        // copy it into our buffer as we need to own any pointer in case they are in static storage and get
        // reloaded
        //
        xi_memory_copy(context->window.title.data, xi->window.title.data, xi->window.title.count);
        context->window.title.used = xi->window.title.count;

        xi->window.title = context->window.title.str;
    }

    context->window.dpi = display->dpi;
    context->window.last_state = xi->window.state;

    // show the window now it has been configured and the renderer has been initialised
    //
    ShowWindowAsync(hwnd, SW_SHOW);

    // :fullscreen_window
    //
    if (xi->window.state == XI_WINDOW_STATE_FULLSCREEN) {
        if (GetWindowPlacement(hwnd, &context->window.placement)) {
            int x = display->bounds.left;
            int y = display->bounds.top;

            int w = (display->bounds.right - display->bounds.left);
            int h = (display->bounds.bottom - display->bounds.top);

            // we need to update the style as we initially configured as if it were a normal window
            // but fullscreen windows don't have a titlebar/border
            //
            // we need to save the windowed style as it may need to adjust the client rect when leaving
            // fullscreen mode
            //
            context->window.windowed_style = GetWindowLongW(hwnd, GWL_STYLE);
            LONG style = win32_window_style_get_for_state(hwnd, xi->window.state);

            UINT flags = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED;
            SetWindowLongW(hwnd, GWL_STYLE, style);
            SetWindowPos(hwnd, HWND_TOP, x, y, w, h, flags);

            // update user-side window to reflect the new size
            //
            xi->window.width  = w;
            xi->window.height = h;
        }
    }

    // load and initialise the renderer
    //
    xiRenderer *renderer = &xi->renderer;
    {
        renderer->setup.window_dim.w = xi->window.width;
        renderer->setup.window_dim.h = xi->window.height;

        HMODULE lib = LoadLibraryA("xi_opengld.dll");
        if (lib) {
            context->renderer.lib  = lib;
            context->renderer.init = (xiRendererInit *) GetProcAddress(lib, "xi_opengl_init");

            if (context->renderer.init) {
                xiWin32WindowData data = { 0 }; // :renderer_core
                data.hInstance = GetModuleHandleW(0);
                data.hwnd      = hwnd;

                context->renderer.valid = context->renderer.init(renderer, &data);
            }
        }
    }

    if (context->renderer.valid) {
        // audio may fail to initialise, in that case we just continue without audio
        //
        win32_audio_initialise(context);

        // setup thread pool
        //
        xi_thread_pool_init(&context->arena, &xi->thread_pool, system_info.dwNumberOfProcessors);
        xi_asset_manager_init(&context->arena, &xi->assets, xi);

        // setup timing information
        //
        {
            if (xi->time.delta.fixed_hz == 0) {
                xi->time.delta.fixed_hz = 100;
            }

            xi_u64 fixed_ns   = (1000000000 / xi->time.delta.fixed_hz);
            context->fixed_ns = fixed_ns;

            if (xi->time.delta.clamp_s <= 0.0f) {
                xi->time.delta.clamp_s = 0.2f;
                context->clamp_ns = 200000000;
            }
            else {
                context->clamp_ns = (xi_u64) (1000000000 * xi->time.delta.clamp_s);
            }

            LARGE_INTEGER counter;
            if (QueryPerformanceFrequency(&counter)) {
                context->counter_freq = counter.QuadPart;
            }
            else {
                // not really much we can do here if the above call fails, so just default to
                // some non-zero value, this will cause very inaccurate timings, 10mhz
                //
                context->counter_freq = 10000000;
            }

            if (QueryPerformanceCounter(&counter)) {
                context->counter_start = counter.QuadPart;
                xi->time.ticks = context->counter_start;
            }
        }

        // once all of the engine systems have been setup, send the game code a post init
        // call which allows them to call any init they require using the engine systems
        //
        game->init(xi, XI_GAME_INIT);

        while (true) {
            win32_xi_context_update(context);

            // :temp_usage temporary memory is reset after the xiContext is updated this means
            // temporary allocations can be used inside the update function without worring about
            // it being released and/or reused
            //
            xi_temp_reset();

            while (context->accum_ns >= (xi_s64) context->fixed_ns) {
                // do while loop to force at least one update per iteration
                //
                game->simulate(xi);
                context->accum_ns -= context->fixed_ns;

                context->did_update = true;

                // @hack: this is to prevent processing audio events multiple times if this
                // loop does multiple iterations, we should probably have a 'fixed_simulate' function
                // along with a 'simulate' function that the game code exports. the fixed one is called
                // like in this loop with a fixed timestep (potentially) multiple times a frame whereas
                // the normal simulate function is only called once per frame
                //
                // :multiple_updates
                //
                xi->audio_player.event_count = 0;

                // we only need to process one simulate if we are quitting
                //
                if (context->quitting || xi->quit) { break; }
            }

            if (context->accum_ns < 0) { context->accum_ns = 0; }

            game->render(xi, renderer);

            // :note we have to do this in-case the user has set the thread queue to have a single
            // thread if this is the case the only thread that can execute work is the main thread,
            // however, as the main thread is busy running the game anything that is pushed on to the
            // work queue will never be executed as no thread are looking at it.
            //
            // happens after the game code 'simulate' and 'render' functions have been called
            // so we know we aren't going to get anymore tasks from user-side
            //
            // so we check if the thread count is 1 and then wait on it here. this will be very slow
            // as everything is now single-threaded, however for debugging will be very useful and
            // keeps the game working when no worker threads are enabled
            //
            // :single_worker
            //
            if (xi->thread_pool.thread_count == 1) { xi_thread_pool_await_complete(&xi->thread_pool); }

            // we check if the renderer is valid and do not run in the case it is not
            // this allows us to assume the submit function is always there
            //
            renderer->submit(renderer);

            if (context->quitting || xi->quit) {
                // leave after executing an entire frame, this allows the game code to do one last
                // iteration with the 'quit' bool set to true allowing it to do any cleanup/other
                // work that it needs to do when quitting...
                //
                break;
            }
        }

        // just in case there are any tasks left on the pool which are part of the game's
        // shutdown code
        //
        xi_thread_pool_await_complete(&xi->thread_pool);
    }

    // tell our main thread that is processing our messages that we have now quit and it should
    // exit its message loop, it will wait for us to actually finish processing, after this
    // line is is assumed that our window is no longer valid
    //
    PostMessageW(hwnd, WM_QUIT, (WPARAM) result, 0);
    return result;
}

int xie_run(xiGameCode *code) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSW wndclass = { 0 };
    wndclass.style         = CS_OWNDC;
    wndclass.lpfnWndProc   = win32_main_window_handler;
    wndclass.hInstance     = GetModuleHandleW(0);
    wndclass.hIcon         = LoadIconA(0, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursorA(0, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wndclass.lpszClassName = L"xi_game_window_class";

    if (RegisterClassW(&wndclass)) {
        xiArena arena = { 0 };
        xi_arena_init_virtual(&arena, XI_GB(4));

        xiWin32Context *context = xi_arena_push_type(&arena, xiWin32Context);

        context->arena     = arena;
        context->game.code = code;

        // acquire information about the displays connected
        //
        win32_display_info_get(context);
        if (context->display_count != 0) {
            // center on the first monitor, which will be the default if the user chooses not
            // to configure any of the window properties
            //
            xiWin32DisplayInfo *display = &context->displays[0];

            int X, Y;
            int nWidth = 1280, nHeight = 720;

            RECT client;
            client.left   = 0;
            client.top    = 0;
            client.right  = (LONG) (display->xi.scale * nWidth);
            client.bottom = (LONG) (display->xi.scale * nHeight);

            if (AdjustWindowRectExForDpi(&client, WS_OVERLAPPEDWINDOW, FALSE, 0, display->dpi)) {
                nWidth  = (client.right - client.left);
                nHeight = (client.bottom - client.top);
            }

            X = display->bounds.left + ((xi_s32) (display->xi.width  - nWidth)  >> 1);
            Y = display->bounds.top  + ((xi_s32) (display->xi.height - nHeight) >> 1);

            HWND hwnd = CreateWindowExW(0, wndclass.lpszClassName, L"xie", WS_OVERLAPPEDWINDOW,
                    X, Y, nWidth, nHeight, 0, 0, wndclass.hInstance,  0);

            if (hwnd != 0) {
                context->window.handle = hwnd;
                context->running = true;

                SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) context);

                DWORD game_thread;
                HANDLE hThread = CreateThread(0, 0, win32_game_thread, context, 0, &game_thread);
                if (hThread != 0) {
                    while (true) {
                        MSG msg;
                        if (!GetMessageW(&msg, 0, 0, 0)) {
                            // we got a WM_QUIT message so we should quit
                            //
                            XI_ASSERT(msg.message == WM_QUIT);
                            break;
                        }

                        // @todo: we might want to capture text messages in the future so will have to
                        // call TranslateMessage(&msg); and handle the WM_CHAR message
                        //

                        switch (msg.message) {
                            case WM_SYSKEYUP:
                            case WM_KEYUP:

                            case WM_SYSKEYDOWN:
                            case WM_KEYDOWN:

                            case WM_LBUTTONUP:
                            case WM_MBUTTONUP:
                            case WM_RBUTTONUP:
                            case WM_XBUTTONUP:

                            case WM_LBUTTONDOWN:
                            case WM_MBUTTONDOWN:
                            case WM_RBUTTONDOWN:
                            case WM_XBUTTONDOWN:

                            case WM_MOUSEMOVE:
                            case WM_MOUSEWHEEL:
                            case WM_MOUSEHWHEEL:
                            {
                                PostThreadMessageW(game_thread, msg.message, msg.wParam, msg.lParam);
                            }
                            break;

                            default: { DispatchMessage(&msg); }
                        }
                    }

                    // wait for our main thread to actually exit
                    //
                    WaitForSingleObject(hThread, INFINITE);

                    DestroyWindow(hwnd);
                }
                else {
                    // @todo: logging...
                    //
                }
            }
            else {
                // @todo: logging...
                //
            }
        }
        else {
            // @todo: logging...
            //
        }
    }
    else {
        // @todo: logging...
        //
    }

    return 0;
}
