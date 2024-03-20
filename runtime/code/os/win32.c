//
// --------------------------------------------------------------------------------
// :Basic_Includes
// --------------------------------------------------------------------------------
//

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include <shellscalingapi.h>
#include <shlobj.h>
#include <pathcch.h>

#include <stdio.h>

#define MAX_TITLE_COUNT 1024

//
// --------------------------------------------------------------------------------
// :Audio_Defines
// --------------------------------------------------------------------------------
//

#define CINTERFACE
#define COBJMACROS
#define CONST_VTABLE
#include <MMDeviceAPI.h>
#include <AudioClient.h>

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

//
// --------------------------------------------------------------------------------
// :Library_Links
// --------------------------------------------------------------------------------
//

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "pathcch.lib")
#pragma comment(lib, "synchronization.lib")

//
// --------------------------------------------------------------------------------
    // :Win32_Structures
// --------------------------------------------------------------------------------
//

typedef struct Win32_WindowData Win32_WindowData;
struct Win32_WindowData {
    // this is required to be the same as the renderer-side structures, not to be modified
    // :renderer_core
    //
    HINSTANCE hInstance;
    HWND      hwnd;
};

typedef struct Win32_DisplayInfo Win32_DisplayInfo;
struct Win32_DisplayInfo {
    Win32_DisplayInfo *next;

    HMONITOR handle;

    // this is to get the actual device name
    //
    LPWSTR gdi_name; // \\.\DISPLAY[n]
    Str8   name;     // 'friendly name' in UTF-8

    F32 refresh_rate;

    DWORD flags;
    RECT  bounds;

    U32 dpi; // raw dpi value
    U32 index;
};

// context containing any required win32 information
//
typedef struct Win32_Context Win32_Context;
struct Win32_Context {
    Xi_Engine xi;

    M_Arena *arena;

    volatile B32 running; // for whether the _application_ is even running

    B32 quitting;

    // We have a list of Win32 display info as we don't know how many monitors
    // are going to be in use up front, this gathers the relevant information and then
    // flattens to an array of DisplayInfo when presented to the user
    //
    struct {
        U32 count;

        Win32_DisplayInfo *first;
        Win32_DisplayInfo *last;
    } displays;

    // timing info
    //
    B32 did_update;

    U64 counter_start;
    U64 counter_freq;

    S64 accum_ns; // signed just in-case, due to clamp on delta time we don't need the range
    U64 fixed_ns;
    U64 clamp_ns;

    struct {
        HWND handle;

        LONG windowed_style; // style before entering fullscreen
        WINDOWPLACEMENT placement;

        B32 user_resizing;
        B32 maximised;

        U32 dpi;

        U32    last_state;
        Buffer title;
    } window;

    struct {
        B32 enabled;

        IAudioClient *client;
        IAudioRenderClient *renderer;
        U32 frame_count;
    } audio;

    struct {
        B32 valid;
        HMODULE lib;

        RendererInitProc *Init;
    } renderer;

    struct {
        U64 last_time;

        HMODULE dll;
        LPWSTR  dll_src_path; // where the game dll is compiled to
        LPWSTR  dll_dst_path; // where the game dll is copied to prevent file locking

        Xi_GameCode *code;
    } game;
};

//
// --------------------------------------------------------------------------------
// :Win32_Virtual_Memory
// --------------------------------------------------------------------------------
//

U64 OS_AllocationGranularity() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    U64 result = info.dwAllocationGranularity;
    return result;
}

U64 OS_PageSize() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    U64 result = info.dwPageSize;
    return result;
}

void *OS_MemoryReserve(U64 size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
    return result;
}

B32 OS_MemoryCommit(void *base, U64 size) {
    B32 result = VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE) != 0;
    return result;
}

void OS_MemoryDecommit(void *base, U64 size) {
    VirtualFree(base, size, MEM_DECOMMIT);
}

void OS_MemoryRelease(void *base, U64 size) {
    (void) size; // :note not needed by VirtualFree when releasing memory

    VirtualFree(base, 0, MEM_RELEASE);
}

//
// --------------------------------------------------------------------------------
// :Win32_Threading
// --------------------------------------------------------------------------------
//

FileScope DWORD Win32_ThreadProc(LPVOID arg) {
    ThreadQueue *queue = cast(ThreadQueue *) arg;
    ThreadRun(queue);

    return 0;
}

void OS_ThreadStart(ThreadQueue *arg) {
    DWORD id;
    HANDLE handle = CreateThread(0, 0, Win32_ThreadProc, cast(void *) arg, 0, &id);
    CloseHandle(handle);
}

void OS_FutexWait(Futex *futex, Futex value) {
    for (;;) {
        WaitOnAddress(futex, cast(PVOID) &value, sizeof(value), INFINITE);
        if (*futex != value) { break; }
    }
}

void OS_FutexSignal(Futex *futex) {
    WakeByAddressSingle((PVOID) futex);
}

void OS_FutexBroadcast(Futex *futex) {
    WakeByAddressAll((PVOID) futex);
}

//
// --------------------------------------------------------------------------------
// :Win32_Utilities
// --------------------------------------------------------------------------------
//

FileScope DWORD Win32_Str16Count(LPCWSTR wstr) {
    DWORD result = 0;
    while (wstr[result] != L'\0') {
        result += 1;
    }

    return result;
}

FileScope BOOL Win32_Str16Equal(LPWSTR a, LPWSTR b) {
    BOOL result;

    S64 it;
    for (it = 0; a[it] && b[it]; it += 1) {
        if (a[it] != b[it]) { break; }
    }

    result = (a[it] == 0) && (b[it] == 0);
    return result;
}

FileScope LPWSTR Win32_Str8ToStr16(Str8 str) {
    LPWSTR result = 0;

    DWORD count = MultiByteToWideChar(CP_UTF8, 0, (LPCCH) str.data, (int) str.count, 0, 0);
    if (count) {
        M_Arena *temp = M_TempGet();

        result = M_ArenaPush(temp, WCHAR, count + 1);

        MultiByteToWideChar(CP_UTF8, 0, (LPCCH) str.data, (int) str.count, result, count);
        result[count] = 0;
    }

    return result;
}

FileScope Str8 Win32_Str16ToStr8(LPWSTR wstr) {
    Str8 result = { 0 };

    DWORD count = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, 0, 0, 0, 0);
    if (count) {
        M_Arena *temp = M_TempGet();

        result.count = count - 1;
        result.data  = M_ArenaPush(temp, U8, count);

        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, (LPSTR) result.data, count, 0, 0);
    }

    return result;
}

//
// --------------------------------------------------------------------------------
// :Win32_Window_Management
// --------------------------------------------------------------------------------
//

FileScope LONG Win32_WindowStyleForState(HWND window, WindowState state) {
    LONG result = GetWindowLongW(window, GWL_STYLE);

    switch (state) {
        case WINDOW_STATE_WINDOWED_RESIZABLE: {
            result |= WS_OVERLAPPEDWINDOW;
        }
        break;
        case WINDOW_STATE_WINDOWED: {
            result |= WS_OVERLAPPEDWINDOW;
            result &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }
        break;
        case WINDOW_STATE_WINDOWED_BORDERLESS:
        case WINDOW_STATE_FULLSCREEN: {
            result &= ~WS_OVERLAPPEDWINDOW;
        }
        break;

        // do nothing
        //
        default: {} break;
    }

    return result;
}

FileScope LRESULT CALLBACK Win32_MainWindowHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    Win32_Context *context = cast(Win32_Context *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    if (!context) {
        result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    else {
        switch (uMsg) {
            case WM_CLOSE: {
                context->running = false;
            }
            break;
            case WM_ENTERSIZEMOVE:
            case WM_EXITSIZEMOVE: {
                context->window.user_resizing = (uMsg == WM_ENTERSIZEMOVE);
            }
            break;
            case WM_SIZE: {
                context->window.maximised = (wParam == SIZE_MAXIMIZED);
            }
            break;
            case WM_DPICHANGED: {
                if (IsWindowVisible(hwnd)) {
                    // If the window is not visible we are stil going through the init process and we
                    // don't need this to mess with the window size if we are placing the window on
                    // a user selected monitor
                    //
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
                // we have to interact with this message instead of just using the standard LPRECT
                // provided to WM_DPICHANGED because the client resolutions it provides are incorrect
                //
                LONG dpi   = (LONG) wParam;
                PSIZE size = (PSIZE) lParam;

                RECT client_rect;
                GetClientRect(hwnd, &client_rect);

                F32 scale = cast(F32) dpi / context->window.dpi;

                // may produce off by 1px errors when scaling odd resolutions
                //
                size->cx = (LONG) (scale * (client_rect.right - client_rect.left));
                size->cy = (LONG) (scale * (client_rect.bottom - client_rect.top));

                result = TRUE;
            }
            break;
            default: {
                result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
            }
            break;
        }
    }

    return result;
}

FileScope BOOL CALLBACK Win32_DisplayEnumerate(HMONITOR hMonitor,  HDC hdcMonitor, LPRECT lpRect, LPARAM lParam) {
    BOOL result = TRUE;

    (void) lpRect;
    (void) hdcMonitor;

    Win32_Context *context = cast(Win32_Context *) lParam;

    MONITORINFOEXW monitor_info = { 0 };
    monitor_info.cbSize = sizeof(MONITORINFOEXW);

    if (GetMonitorInfoW(hMonitor, cast(LPMONITORINFO) &monitor_info)) {
        UINT xdpi, ydpi;
        if (GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi) == S_OK) {
            DEVMODEW mode = { 0 };
            mode.dmSize = sizeof(DEVMODEW);

            if (EnumDisplaySettingsW(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &mode)) {
                Win32_DisplayInfo *info = M_ArenaPush(context->arena, Win32_DisplayInfo);

                info->next   = 0;

                info->bounds = monitor_info.rcMonitor;
                info->dpi    = xdpi;

                info->handle = hMonitor;

                S64 name_count = Win32_Str16Count(monitor_info.szDevice) + 1;
                info->gdi_name = M_ArenaPushCopy(context->arena, monitor_info.szDevice, WCHAR, name_count);

                info->refresh_rate = cast(F32) mode.dmDisplayFrequency;
                info->flags        = monitor_info.dwFlags;
                info->index        = context->displays.count;

                if (context->displays.first) {
                    context->displays.last->next = info;
                    context->displays.last       = info;
                }
                else {
                    context->displays.first = context->displays.last = info;
                }

                context->displays.count += 1;
#if 0
                info->xi.width  = mode.dmPelsWidth;
                info->xi.height = mode.dmPelsHeight;

                info->xi.scale        = cast(F32) xdpi / 96.0f;
                info->xi.refresh_rate = cast(F32) mode.dmDisplayFrequency;
                info->xi.primary      = (monitor_info.dwFlags & MONITORINFOF_PRIMARY) != 0;

                if (context->first_display) {
                    context->last_display->xi.next = cast(DisplayInfo *) info;
                    context->last_display          = info;
                }
                else {
                    context->first_display = info;
                    context->last_display  = info;

                    info->xi.next = 0;
                }
#endif
            }
        }
    }

    return result;
}

FileScope void Win32_DisplayInfoGet(Win32_Context *context) {
    EnumDisplayMonitors(0, 0, Win32_DisplayEnumerate, cast(LPARAM) context);

    Xi_Engine *xi = &context->xi;

    xi->system.num_displays = 0;
    xi->system.displays     = M_ArenaPush(context->arena, DisplayInfo, context->displays.count);

    M_Arena *temp = M_TempGet();

    U32 path_count = 0;
    U32 mode_count = 0;

    DISPLAYCONFIG_PATH_INFO *paths = 0;
    DISPLAYCONFIG_MODE_INFO *modes = 0;

    U32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;

    LONG result;

    do {
        result = GetDisplayConfigBufferSizes(flags, &path_count, &mode_count);
        if (result == ERROR_SUCCESS) {
            paths = M_ArenaPush(temp, DISPLAYCONFIG_PATH_INFO, path_count);
            modes = M_ArenaPush(temp, DISPLAYCONFIG_MODE_INFO, mode_count);

            result = QueryDisplayConfig(flags, &path_count, paths, &mode_count, modes, 0);
        }
    }
    while (result == ERROR_INSUFFICIENT_BUFFER);

    if (result == ERROR_SUCCESS) {
        for (U32 it = 0; it < path_count; ++it) {
            DISPLAYCONFIG_PATH_INFO *path = &paths[it];

            DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name = { 0 };
            source_name.header.adapterId = path->sourceInfo.adapterId;
            source_name.header.id        = path->sourceInfo.id;
            source_name.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
            source_name.header.size      = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);

            result = DisplayConfigGetDeviceInfo(&source_name.header);
            if (result == ERROR_SUCCESS) {
                DISPLAYCONFIG_TARGET_DEVICE_NAME target_name = { 0 };
                target_name.header.adapterId = path->targetInfo.adapterId;
                target_name.header.id        = path->targetInfo.id;
                target_name.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
                target_name.header.size      = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);

                result = DisplayConfigGetDeviceInfo(&target_name.header);
                if (result == ERROR_SUCCESS) {
                    LPWSTR gdi_name     = source_name.viewGdiDeviceName;
                    LPWSTR monitor_name = target_name.monitorFriendlyDeviceName;

                    for (Win32_DisplayInfo *dpy = context->displays.first;
                            dpy != 0;
                            dpy = dpy->next)
                    {
                        if (Win32_Str16Equal(dpy->gdi_name, gdi_name)) {
                            Str8 name = Win32_Str16ToStr8(monitor_name);
                            dpy->name = Str8_PushCopy(context->arena, name);

                            break;
                        }
                    }
                }
            }
        }
    }

    for (Win32_DisplayInfo *dpy = context->displays.first;
            dpy != 0;
            dpy = dpy->next)
    {
        DisplayInfo *info = &xi->system.displays[xi->system.num_displays];

        info->width  = cast(U32) (dpy->bounds.right  - dpy->bounds.left);
        info->height = cast(U32) (dpy->bounds.bottom - dpy->bounds.top);
        info->name   = dpy->name;

        info->scale        = dpy->dpi / 96.0f;
        info->refresh_rate = dpy->refresh_rate;
        info->primary      = (dpy->flags & MONITORINFOF_PRIMARY) != 0;

        xi->system.num_displays += 1;
    }

    Assert(xi->system.num_displays == context->displays.count);
}

//
// --------------------------------------------------------------------------------
// :Win32_File_System
// --------------------------------------------------------------------------------
//

typedef U32 Win32_PathType;
enum {
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
// the path segment separator 'Win32_PathConvertToAPI' should be called on all 'win32' strings
// that need to be used by the user facing runtime api. this function will convert from utf-16 to
// utf-8 and will change all path segment separators to forward slashes, it will not, however,
// remove the volume letter designator from win32 paths, as without this there is no way to know if
// certain paths reside on other drives than the working drive
//
FileScope LPWSTR Win32_SystemPathGet(Win32_PathType type) {
    LPWSTR result = 0;

    M_Arena *temp = M_TempGet();

    switch (type) {
        case WIN32_PATH_TYPE_EXECUTABLE: {
            DWORD buffer_count = MAX_PATH;

            // get the executable filename, windows doesn't just tell us how long the path
            // is so we have to do this loop until we create a buffer that is big enough, in
            // most cases MAX_PATH will suffice
            //
            DWORD count;
            do {
                result = M_ArenaPush(temp, WCHAR, buffer_count);
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
            // the documentation tells you not to use this in multi-threaded applications due to the
            // path being stored in a global variable, while this is technically a multi-threaded
            // application it currently only has one "main" thread so it'll be fine unless windows
            // is reading/writing with its own threads in the background
            //
            DWORD count = GetCurrentDirectoryW(0, 0);
            if (count > 0) {
                result = M_ArenaPush(temp, WCHAR, count);
                GetCurrentDirectoryW(count, result);
            }
        }
        break;
        case WIN32_PATH_TYPE_USER: {
            PWSTR path;
            if (SHGetKnownFolderPath(&FOLDERID_RoamingAppData, 0, 0, &path) == S_OK) {
                DWORD len = Win32_Str16Count(path) + 1;
                result    = M_ArenaPushCopy(temp, path, WCHAR, len);
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
            // we could try to manually load it and use it if its available, but as the functionality
            // is the same is there really any point?
            //
            result = M_ArenaPush(temp, WCHAR, MAX_PATH + 1);
            DWORD count = GetTempPathW(MAX_PATH + 1, result);
            if (count) {
                result[count - 1] = L'\0'; // remove the final backslash as we don't need it
                M_ArenaPop(temp, WCHAR, MAX_PATH - count + 1);
            }
        }
        break;
        default: {} break;
    }

    return result;
}

FileScope void Win32_PathConvertFromAPI(WCHAR *out, U64 cch_limit, Str8 path) {
    LPWSTR wpath = Win32_Str8ToStr16(path);
    if (wpath) {
        // Swap file separator
        //
        WCHAR *start = wpath;
        while (start[0]) {
            if (start[0] == L'/') { start[0] = L'\\'; }
            start += 1;
        }

        B32 requires_volume = (path.count >= 1) && (path.data[0] == '/');
        B32 is_absolute     = (path.count >= 3) &&
                              ((path.data[0] >= 'A'  &&  path.data[0] <= 'Z') ||
                               (path.data[0] >= 'a'  &&  path.data[0] >= 'z')) &&
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
            LPWSTR working = Win32_SystemPathGet(WIN32_PATH_TYPE_WORKING);
            PathCchCombineEx(out, cch_limit, working, wpath, PATHCCH_ALLOW_LONG_PATHS);
        }
    }
}

// this function assumes you are supplying a 'win32' absolute path
//
FileScope Str8 Win32_PathConvertToAPI(M_Arena *arena, WCHAR *wpath) {
    Str8 path = Win32_Str16ToStr8(wpath);

    for (S64 it = 0; it < path.count; ++it) {
        if (path.data[it] == '\\') { path.data[it] = '/'; }
    }

    Str8 result = Str8_PushCopy(arena, path);
    return result;
}

void Win32_DirectoryListBuilderRecursiveGet(M_Arena *arena, DirectoryListBuilder *builder, LPWSTR path, U64 path_limit, B32 recursive) {
    M_Arena *temp = M_TempGet();
    Str8     base = Win32_PathConvertToAPI(temp, path);

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
            B32 is_dir     = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            Str8 name      = Win32_Str16ToStr8(data.cFileName);
            Str8 full_path = Str8_Format(arena, "%.*s/%.*s", Str8_Arg(base), Str8_Arg(name));

            // we know there is going to be at least one forward slash because of the format xi_string above
            //
            Str8 basename = Str8_RemoveBeforeLast(full_path, '/');

            DirectoryEntry entry;
            entry.type     = is_dir ? DIRECTORY_ENTRY_TYPE_DIRECTORY : DIRECTORY_ENTRY_TYPE_FILE;
            entry.path     = full_path;
            entry.basename = basename;
            entry.size     = (cast(U64) data.nFileSizeHigh  << 32) | (data.nFileSizeLow);
            entry.time     = (cast(U64) time.dwHighDateTime << 32) | (time.dwLowDateTime);

            DirectoryListBuilderAppend(builder, entry);

            if (recursive && is_dir) {
                PathCchRemoveFileSpec(path, path_limit); // remove \\* for search
                PathCchAppendEx(path, path_limit, data.cFileName, PATHCCH_ALLOW_LONG_PATHS);

                Win32_DirectoryListBuilderRecursiveGet(arena, builder, path, path_limit, recursive);

                PathCchRemoveFileSpec(path, path_limit);
            }
        }
        while (FindNextFileW(find, &data) != FALSE);

        FindClose(find);
    }
}

void OS_DirectoryListBuild(M_Arena *arena, DirectoryListBuilder *builder, Str8 path, B32 recursive) {
    WCHAR wpath[PATHCCH_MAX_CCH];
    Win32_PathConvertFromAPI(wpath, PATHCCH_MAX_CCH, path);

    Win32_DirectoryListBuilderRecursiveGet(arena, builder, wpath, PATHCCH_MAX_CCH, recursive);
}

B32 OS_DirectoryEntryGetByPath(M_Arena *arena, DirectoryEntry *entry, Str8 path) {
    B32 result = false;

    WCHAR wpath[PATHCCH_MAX_CCH];
    Win32_PathConvertFromAPI(wpath, PATHCCH_MAX_CCH, path);

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExW(wpath, GetFileExInfoStandard, &data)) {
        FILETIME time = data.ftLastWriteTime;
        B32 is_dir    = ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

        Str8 full_path = Win32_PathConvertToAPI(arena, wpath);
        Str8 basename  = Str8_RemoveBeforeLast(full_path, '/');


        entry->type     = is_dir ? DIRECTORY_ENTRY_TYPE_DIRECTORY : DIRECTORY_ENTRY_TYPE_FILE;
        entry->path     = full_path;
        entry->basename = basename;
        entry->size     = (cast(U64) data.nFileSizeHigh  << 32) | (data.nFileSizeLow);
        entry->time     = (cast(U64) time.dwHighDateTime << 32) | (time.dwLowDateTime);

        result = true;
    }

    return result;
}

B32 OS_DirectoryCreate(Str8 path) {
    B32 result = true;

    WCHAR wpath[PATHCCH_MAX_CCH];
    Win32_PathConvertFromAPI(wpath, PATHCCH_MAX_CCH, path);

    U64 it = 0;
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

void OS_DirectoryDelete(Str8 path) {
    WCHAR wpath[PATHCCH_MAX_CCH];
    Win32_PathConvertFromAPI(wpath, PATHCCH_MAX_CCH, path);

    RemoveDirectoryW(wpath);
}

FileHandle OS_FileOpen(Str8 path, FileAccessFlags access) {
    FileHandle result = { 0 };

    WCHAR wpath[PATHCCH_MAX_CCH];
    Win32_PathConvertFromAPI(wpath, PATHCCH_MAX_CCH, path);

    DWORD access_flags = 0;
    DWORD creation     = 0;

    if (access & FILE_ACCESS_FLAG_READ) {
        access_flags |= GENERIC_READ;
        creation      = OPEN_EXISTING;
    }

    if (access & FILE_ACCESS_FLAG_WRITE) {
        access_flags |= GENERIC_WRITE;
        creation      = OPEN_ALWAYS;
    }

    HANDLE hFile = CreateFileW(wpath, access_flags, FILE_SHARE_READ, 0, creation, FILE_ATTRIBUTE_NORMAL, 0);

    result.v = cast(U64) hFile;

    if (hFile == INVALID_HANDLE_VALUE) {
        result.status = FILE_HANDLE_STATUS_FAILED_OPEN;
    }

    return result;
}

void OS_FileClose(FileHandle *handle) {
    HANDLE hFile = cast(HANDLE) handle->v;

    if (hFile != INVALID_HANDLE_VALUE) { CloseHandle(hFile); }
    handle->status = FILE_HANDLE_STATUS_CLOSED;
}

void OS_FileDelete(Str8 path) {
    WCHAR wpath[PATHCCH_MAX_CCH];
    Win32_PathConvertFromAPI(wpath, PATHCCH_MAX_CCH, path);

    DeleteFileW(wpath);
}

B32 OS_FileRead(FileHandle *handle, void *dst, U64 offset, U64 size) {
    B32 result = false;

    if (handle->status == FILE_HANDLE_STATUS_VALID) {
        result = true;

        HANDLE hFile = cast(HANDLE) handle->v;

        Assert(hFile  != INVALID_HANDLE_VALUE);
        Assert(offset != FILE_OFFSET_APPEND);

        OVERLAPPED overlapped = { 0 };
        overlapped.Offset     = cast(DWORD) (offset >>  0);
        overlapped.OffsetHigh = cast(DWORD) (offset >> 32);

        U8 *data      = cast(U8 *) dst;
        U64 remaining = size;

        do {
            DWORD nread   = 0;
            DWORD to_read = (remaining > U32_MAX) ? U32_MAX : (DWORD) remaining;

            if (ReadFile(hFile, data, to_read, &nread, &overlapped)) {
                Assert(nread <= remaining);

                remaining -= nread;

                data   += nread;
                offset += nread;

                overlapped.Offset     = cast(DWORD) (offset >>  0);
                overlapped.OffsetHigh = cast(DWORD) (offset >> 32);
            }
            else {
                result = false;
                handle->status = FILE_HANDLE_STATUS_FAILED_READ;

                break;
            }
        } while (remaining > 0);
    }

    return result;
}

B32 OS_FileWrite(FileHandle *handle, void *src, U64 offset, U64 size) {
    B32 result = false;

    if (handle->status == FILE_HANDLE_STATUS_VALID) {
        result = true;

        HANDLE hFile = cast(HANDLE) handle->v;

        Assert(hFile != INVALID_HANDLE_VALUE);

        OVERLAPPED *overlapped = 0;
        OVERLAPPED overlapped_offset = { 0 };

        if (offset == FILE_OFFSET_APPEND) {
            // we are appending so move to the end of the file and don't specify an overlapped structure,
            // this is technically invalid to call on the stdout/stderr handles but it seems to work
            // correctly regardless so we don't have to do any detection to prevent against it
            //
            SetFilePointer(hFile, 0, 0, FILE_END);
        }
        else {
            overlapped_offset.Offset     = cast(DWORD) (offset >>  0);
            overlapped_offset.OffsetHigh = cast(DWORD) (offset >> 32);

            overlapped = &overlapped_offset;
        }

        U8 *data      = cast(U8 *) src;
        U64 remaining = size;

        do {
            DWORD nwritten = 0;
            DWORD to_write = (remaining > U32_MAX) ? U32_MAX : (DWORD) remaining;

            if (WriteFile(hFile, data, to_write, &nwritten, overlapped)) {
                Assert(nwritten <= remaining);

                remaining-= nwritten;

                data   += nwritten;
                offset += nwritten;

                if (overlapped) {
                    overlapped->Offset     = cast(DWORD) (offset >>  0);
                    overlapped->OffsetHigh = cast(DWORD) (offset >> 32);
                }
            }
            else {
                handle->status = FILE_HANDLE_STATUS_FAILED_WRITE;
                result = false;

                break;
            }
        } while (remaining > 0);
    }

    return result;
}

//
// --------------------------------------------------------------------------------
// :Win32_Game_Code
// --------------------------------------------------------------------------------
//

FileScope void Win32_GameCodeInit(Win32_Context *context) {
    Xi_GameCode *game = context->game.code;

    Assert(game != 0);

    M_Arena *temp = M_TempGet();

    if (game->reloadable) {
        DirectoryList full_list = DirectoryListGet(temp, context->xi.system.executable_path, false);
        DirectoryList dll_list  = DirectoryListFilterForSuffix(temp, &full_list, Str8_Literal(".dll"));

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

        B32 found = false;
        for (U32 it = 0; !found && it < dll_list.num_entries; ++it) {
            DirectoryEntry *entry = &dll_list.entries[it];

            WCHAR wpath[PATHCCH_MAX_CCH];
            Win32_PathConvertFromAPI(wpath, PATHCCH_MAX_CCH, entry->path);

            HMODULE lib = LoadLibraryW(wpath);
            if (lib) {
                Xi_GameInitProc     *Init     = cast(Xi_GameInitProc     *)     GetProcAddress(lib, "__xi_game_init");
                Xi_GameSimulateProc *Simulate = cast(Xi_GameSimulateProc *) GetProcAddress(lib, "__xi_game_simulate");
                Xi_GameRenderProc   *Render   = cast(Xi_GameRenderProc   *)   GetProcAddress(lib, "__xi_game_render");

                if (Init != 0 && Simulate != 0 && Render != 0) {
                    // we found a valid .dll that exports the functions specified so we can
                    // close the loaded library and save the path to it
                    //
                    // @todo: maybe we should select the one with the latest time if there are
                    // multiple .dlls available?
                    //
                    DWORD count = Win32_Str16Count(wpath) + 1;
                    context->game.dll_src_path = M_ArenaPushCopy(context->arena, wpath, WCHAR, count);

                    found = true;
                }

                FreeLibrary(lib);
            }
        }

        if (found) {
            // get temporary path to copy dll file to
            //
            LPWSTR temp_path = Win32_SystemPathGet(WIN32_PATH_TYPE_TEMP);

            WCHAR dst_path[MAX_PATH + 1];
            if (GetTempFileNameW(temp_path, L"xie", 0x1234, dst_path)) {
                DWORD count = Win32_Str16Count(dst_path) + 1;
                context->game.dll_dst_path = M_ArenaPushCopy(context->arena, dst_path, WCHAR, count);
            }

            if (CopyFileW(context->game.dll_src_path, context->game.dll_dst_path, FALSE)) {
                context->game.dll = LoadLibraryW(context->game.dll_dst_path);

                if (context->game.dll) {
                    game->Init     = cast(Xi_GameInitProc     *) GetProcAddress(context->game.dll, "__xi_game_init");
                    game->Simulate = cast(Xi_GameSimulateProc *) GetProcAddress(context->game.dll, "__xi_game_simulate");
                    game->Render   = cast(Xi_GameRenderProc   *) GetProcAddress(context->game.dll, "__xi_game_render");

                    if (Xi_GameCodeIsValid(game)) {
                        WIN32_FILE_ATTRIBUTE_DATA attributes;
                        if (GetFileAttributesExW(context->game.dll_src_path, GetFileExInfoStandard, &attributes)) {
                            FILETIME ft = attributes.ftLastWriteTime;
                            context->game.last_time = (cast(U64) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
                        }
                    }
                }
            }
        }
        else {
            // @todo: logging...
            //
        }

        if (!game->Init)     { game->Init     = __xi_game_init;     }
        if (!game->Simulate) { game->Simulate = __xi_game_simulate; }
        if (!game->Render)   { game->Render   = __xi_game_render;   }
    }
}

FileScope void Win32_GameCodeReload(Win32_Context *context) {
    // @todo: we should really be reading the .dll file to see if it has debug information. this is
    // because the .pdb file location is hardcoded into the debug data directory. when debugging on
    // some debuggers (like microsoft visual studio) it will lock the .pdb file and prevent the compiler
    // from writing to it causing a whole host of issues
    //
    // there are several ways around this, loading the .dll patching the .pdb path to something unique
    // is the "proper" way, you can also specify the /pdb linker flag with msvc.
    //
    // :weird_dll this has the same issue as above, but kinda in reverse. if the system doesn't
    // respect our request to close our temporary dll it will fail to copy, thus we should really be
    // using something with a unique number
    //
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    if (GetFileAttributesExW(context->game.dll_src_path, GetFileExInfoStandard, &attributes)) {
        FILETIME ft = attributes.ftLastWriteTime;
        U64 time    = (cast(U64) ft.dwHighDateTime << 32) | ft.dwLowDateTime;

        if (time != context->game.last_time) {
            Xi_GameCode *game = context->game.code;

            if (context->game.dll) {
                FreeLibrary(context->game.dll);

                context->game.dll = 0;
                game->Init        = 0;
                game->Simulate    = 0;
                game->Render      = 0;
            }

            if (CopyFileW(context->game.dll_src_path, context->game.dll_dst_path, FALSE)) {
                HMODULE dll = LoadLibraryW(context->game.dll_dst_path);

                if (dll) {
                    context->game.dll = dll;

                    game->Init     = cast(Xi_GameInitProc     *) GetProcAddress(dll, "__xi_game_init");
                    game->Simulate = cast(Xi_GameSimulateProc *) GetProcAddress(dll, "__xi_game_simulate");
                    game->Render   = cast(Xi_GameRenderProc   *) GetProcAddress(dll, "__xi_game_render");

                    if (Xi_GameCodeIsValid(game)) {
                        context->game.last_time = time;

                        // call init again but signal to the developer that the code was reloaded
                        // rather than initially called
                        //
                        game->Init(&context->xi, XI_GAME_RELOADED);
                    }
                }
            }

            if (!game->Init)     { game->Init     = __xi_game_init;     }
            if (!game->Simulate) { game->Simulate = __xi_game_simulate; }
            if (!game->Render)   { game->Render   = __xi_game_render;   }
        }
    }
}

//
// --------------------------------------------------------------------------------
// :Win32_Audio
// --------------------------------------------------------------------------------
//

FileScope void Win32_AudioInit(Win32_Context *context) {
    AudioPlayer *player = &context->xi.audio_player;
    AudioPlayerInit(context->arena, player);

    // initialise wasapi
    //
    REFERENCE_TIME duration = cast(REFERENCE_TIME) ((player->frame_count / cast(F32) player->sample_rate) * REFTIMES_PER_SEC);

    IAudioClient        *client     = 0;
    IAudioRenderClient  *renderer   = 0;
    IMMDeviceEnumerator *enumerator = 0;

    U32 frame_count = 0;

    HRESULT hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, &IID_IMMDeviceEnumerator, cast(void **) &enumerator);
    if (SUCCEEDED(hr)) {
        IMMDevice *device = 0;

        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device);
        if (SUCCEEDED(hr)) {
            hr = IMMDevice_Activate(device, &IID_IAudioClient, CLSCTX_ALL, 0, cast(void **) &client);
            if (SUCCEEDED(hr)) {
                WAVEFORMATEX wav_format = { 0 };
                wav_format.wFormatTag      = WAVE_FORMAT_PCM;
                wav_format.nChannels       = 2;
                wav_format.nSamplesPerSec  = player->sample_rate;
                wav_format.wBitsPerSample  = 16;
                wav_format.nBlockAlign     = (wav_format.wBitsPerSample * wav_format.nChannels) / 8;
                wav_format.nAvgBytesPerSec = wav_format.nBlockAlign * wav_format.nSamplesPerSec;
                wav_format.cbSize          = 0;

                hr = IAudioClient_Initialize(client, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM, duration, 0, &wav_format, 0);
                if (SUCCEEDED(hr)) {
                    hr = IAudioClient_GetBufferSize(client, &frame_count);

                    if (SUCCEEDED(hr)) {
                        hr = IAudioClient_GetService(client, &IID_IAudioRenderClient, cast(void **) &renderer);
                        if (SUCCEEDED(hr)) {
                            hr = IAudioClient_Start(client);
                            context->audio.enabled = SUCCEEDED(hr);
                        }
                    }
                }
            }
        }
    }

    if (context->audio.enabled) {
        context->audio.client      = client;
        context->audio.renderer    = renderer;
        context->audio.frame_count = frame_count;
    }
}

FileScope void Win32_AudioMix(Win32_Context *context) {
    if (context->audio.enabled) {
        Xi_Engine *xi = &context->xi;

        // wow non-insane audio api, good job microsoft! ... lets ignore init
        //

        U32 queued_frames = 0;
        HRESULT hr = IAudioClient_GetCurrentPadding(context->audio.client, &queued_frames);
        if (SUCCEEDED(hr)) {
            Assert(queued_frames <= context->audio.frame_count);

            U32 frame_count = context->audio.frame_count - queued_frames;
            U8 *samples     = 0;

            if (frame_count > 0) {
                hr = IAudioRenderClient_GetBuffer(context->audio.renderer, frame_count, &samples);
                if (SUCCEEDED(hr)) {
                    F32 dt = cast(F32) xi->time.delta.s;
                    AudioPlayerUpdate(&xi->audio_player, &xi->assets, cast(S16 *) samples, frame_count, dt);

                    IAudioRenderClient_ReleaseBuffer(context->audio.renderer, frame_count, 0);
                }
            }
        }
    }
}

//
// --------------------------------------------------------------------------------
// :Win32_Input
// --------------------------------------------------------------------------------
//

GlobalVar U8 tbl_win32_virtual_key_to_input_key[] = {
    // @todo: switch to raw input for usb hid scancodes instead of using this virtual keycode
    // lookup table
    //
    [VK_RETURN]     = KEYBOARD_KEY_RETURN,    [VK_ESCAPE]    = KEYBOARD_KEY_ESCAPE,
    [VK_BACK]       = KEYBOARD_KEY_BACKSPACE, [VK_TAB]       = KEYBOARD_KEY_TAB,
    [VK_HOME]       = KEYBOARD_KEY_HOME,      [VK_PRIOR]     = KEYBOARD_KEY_PAGEUP,
    [VK_DELETE]     = KEYBOARD_KEY_DELETE,    [VK_END]       = KEYBOARD_KEY_END,
    [VK_NEXT]       = KEYBOARD_KEY_PAGEDOWN,  [VK_RIGHT]     = KEYBOARD_KEY_RIGHT,
    [VK_LEFT]       = KEYBOARD_KEY_LEFT,      [VK_DOWN]      = KEYBOARD_KEY_DOWN,
    [VK_UP]         = KEYBOARD_KEY_UP,        [VK_F1]        = KEYBOARD_KEY_F1,
    [VK_F2]         = KEYBOARD_KEY_F2,        [VK_F3]        = KEYBOARD_KEY_F3,
    [VK_F4]         = KEYBOARD_KEY_F4,        [VK_F5]        = KEYBOARD_KEY_F5,
    [VK_F6]         = KEYBOARD_KEY_F6,        [VK_F7]        = KEYBOARD_KEY_F7,
    [VK_F8]         = KEYBOARD_KEY_F8,        [VK_F9]        = KEYBOARD_KEY_F9,
    [VK_F10]        = KEYBOARD_KEY_F10,       [VK_F11]       = KEYBOARD_KEY_F11,
    [VK_F12]        = KEYBOARD_KEY_F12,       [VK_INSERT]    = KEYBOARD_KEY_INSERT,
    [VK_SPACE]      = KEYBOARD_KEY_SPACE,     [VK_OEM_3]     = KEYBOARD_KEY_QUOTE,
    [VK_OEM_COMMA]  = KEYBOARD_KEY_COMMA,     [VK_OEM_MINUS] = KEYBOARD_KEY_MINUS,
    [VK_OEM_PERIOD] = KEYBOARD_KEY_PERIOD,    [VK_OEM_2]     = KEYBOARD_KEY_SLASH,

    // these are just direct mappings
    //
    ['0'] = KEYBOARD_KEY_0, ['1'] = KEYBOARD_KEY_1, ['2'] = KEYBOARD_KEY_2,
    ['3'] = KEYBOARD_KEY_3, ['4'] = KEYBOARD_KEY_4, ['5'] = KEYBOARD_KEY_5,
    ['6'] = KEYBOARD_KEY_6, ['7'] = KEYBOARD_KEY_7, ['8'] = KEYBOARD_KEY_8,
    ['9'] = KEYBOARD_KEY_9,

    [VK_OEM_1] = KEYBOARD_KEY_SEMICOLON, [VK_OEM_PLUS] = KEYBOARD_KEY_EQUALS,

    [VK_LSHIFT] = KEYBOARD_KEY_LSHIFT, [VK_LCONTROL] = KEYBOARD_KEY_LCTRL,
    [VK_LMENU]  = KEYBOARD_KEY_LALT,

    [VK_RSHIFT] = KEYBOARD_KEY_RSHIFT, [VK_RCONTROL] = KEYBOARD_KEY_RCTRL,
    [VK_RMENU]  = KEYBOARD_KEY_RALT,

    [VK_OEM_4] = KEYBOARD_KEY_LBRACKET, [VK_OEM_7] = KEYBOARD_KEY_BACKSLASH,
    [VK_OEM_6] = KEYBOARD_KEY_RBRACKET, [VK_OEM_8] = KEYBOARD_KEY_GRAVE,

    // remap uppercase to lowercase
    //
    ['A'] = KEYBOARD_KEY_A, ['B'] = KEYBOARD_KEY_B, ['C'] = KEYBOARD_KEY_C,
    ['D'] = KEYBOARD_KEY_D, ['E'] = KEYBOARD_KEY_E, ['F'] = KEYBOARD_KEY_F,
    ['G'] = KEYBOARD_KEY_G, ['H'] = KEYBOARD_KEY_H, ['I'] = KEYBOARD_KEY_I,
    ['J'] = KEYBOARD_KEY_J, ['K'] = KEYBOARD_KEY_K, ['L'] = KEYBOARD_KEY_L,
    ['M'] = KEYBOARD_KEY_M, ['N'] = KEYBOARD_KEY_N, ['O'] = KEYBOARD_KEY_O,
    ['P'] = KEYBOARD_KEY_P, ['Q'] = KEYBOARD_KEY_Q, ['R'] = KEYBOARD_KEY_R,
    ['S'] = KEYBOARD_KEY_S, ['T'] = KEYBOARD_KEY_T, ['U'] = KEYBOARD_KEY_U,
    ['V'] = KEYBOARD_KEY_V, ['W'] = KEYBOARD_KEY_W, ['X'] = KEYBOARD_KEY_X,
    ['Y'] = KEYBOARD_KEY_Y, ['Z'] = KEYBOARD_KEY_Z,
};

//
// Update pass
//

FileScope void Win32_ContextUpdate(Win32_Context *context) {
    Xi_Engine *xi = &context->xi;

    // timing informations
    //
    LARGE_INTEGER time;
    if (QueryPerformanceCounter(&time)) {
        U64 counter_end = time.QuadPart;
        U64 delta_ns    = (1000000000 * (counter_end - context->counter_start)) / context->counter_freq;

        // don't let delta time exceed the maximum specified
        //
        delta_ns = Min(delta_ns, context->clamp_ns);

        context->accum_ns += delta_ns;

        xi->time.ticks         = counter_end;
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
        xi->time.total.us  = cast(U64) ((xi->time.total.ns / 1000.0) + 0.5);
        xi->time.total.ms  = cast(U64) ((xi->time.total.ns / 1000000.0) + 0.5);
        xi->time.total.s   = (xi->time.total.ns / 1000000000.0);
    }

    Win32_AudioMix(context);

    // process windows messages that our application received
    //
    // @todo: input reset call?
    //
    InputMouse *mouse = &xi->mouse;
    InputKeyboard *kb = &xi->keyboard;

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
    if (context->did_update) {
        // reset mouse params for this frame
        //
        mouse->connected    = true;
        mouse->active       = false;
        mouse->delta.screen = V2S(0, 0);
        mouse->delta.clip   = V2F(0, 0);
        mouse->delta.wheel  = V2F(0, 0);

        mouse->left.pressed   = mouse->left.released   = false;
        mouse->middle.pressed = mouse->middle.released = false;
        mouse->right.pressed  = mouse->right.released  = false;
        mouse->x0.pressed     = mouse->x0.released     = false;
        mouse->x1.pressed     = mouse->x1.released     = false;

        // reset keyboard params for this frame
        //
        kb->connected = true;
        kb->active    = false;

        for (U32 it = 0; it < KEYBOARD_KEY_COUNT; ++it) {
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
                B32 down = (msg.lParam & (1 << 31)) == 0;

                // we have to do some pre-processing to split up the left and right variants of
                // control keys
                //
                switch (msg.wParam) {
                    case VK_MENU: {
                        InputButton *left  = &kb->keys[KEYBOARD_KEY_LALT];
                        InputButton *right = &kb->keys[KEYBOARD_KEY_RALT];

                        InputButtonHandle(left,  GetKeyState(VK_LMENU) & 0x8000);
                        InputButtonHandle(right, GetKeyState(VK_RMENU) & 0x8000);

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
                        InputButton *left  = &kb->keys[KEYBOARD_KEY_LCTRL];
                        InputButton *right = &kb->keys[KEYBOARD_KEY_RCTRL];

                        InputButtonHandle(left,  GetKeyState(VK_LCONTROL) & 0x8000);
                        InputButtonHandle(right, GetKeyState(VK_RCONTROL) & 0x8000);

                        kb->ctrl = (left->down || right->down);
                    }
                    break;
                    case VK_SHIFT: {
                        InputButton *left  = &kb->keys[KEYBOARD_KEY_LSHIFT];
                        InputButton *right = &kb->keys[KEYBOARD_KEY_RSHIFT];

                        InputButtonHandle(left,  GetKeyState(VK_LSHIFT) & 0x8000);
                        InputButtonHandle(right, GetKeyState(VK_RSHIFT) & 0x8000);

                        kb->shift = (left->down || right->down);
                    }
                    break;
                    default: {
                        U32 key_index    = tbl_win32_virtual_key_to_input_key[msg.wParam];
                        InputButton *key = &kb->keys[key_index];

                        InputButtonHandle(key, down);
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
                B32 down = (msg.message == WM_LBUTTONDOWN);
                InputButtonHandle(&mouse->left, down);

                mouse->active = true;
            }
            break;

            // middle mouse button
            //
            case WM_MBUTTONUP:
            case WM_MBUTTONDOWN: {
                B32 down = (msg.message == WM_MBUTTONDOWN);
                InputButtonHandle(&mouse->left, down);

                mouse->active = true;
            }
            break;

            // right mouse button
            //
            case WM_RBUTTONUP:
            case WM_RBUTTONDOWN: {
                B32 down = (msg.message == WM_RBUTTONDOWN);
                InputButtonHandle(&mouse->left, down);

                mouse->active = true;
            }
            break;

            // extra mouse buttons, windows doesn't differentiate x0 and x1 at the event level
            //
            case WM_XBUTTONUP:
            case WM_XBUTTONDOWN: {
                B32 down = (msg.message == WM_XBUTTONDOWN);

                if (msg.wParam & MK_XBUTTON1) { InputButtonHandle(&mouse->x0, down); }
                if (msg.wParam & MK_XBUTTON2) { InputButtonHandle(&mouse->x1, down); }

                mouse->active = true;
            }
            break;

            case WM_MOUSEMOVE: {
                POINTS pt = MAKEPOINTS(msg.lParam);

                Vec2S old_screen = mouse->position.screen;
                Vec2F old_clip   = mouse->position.clip;

                // update the screen space position
                //
                mouse->position.screen = V2S(pt.x, pt.y);

                // update the clip space position
                //
                mouse->position.clip.x = -1.0f + (2.0f * (pt.x / cast(F32) xi->window.width));
                mouse->position.clip.y =  1.0f - (2.0f * (pt.y / cast(F32) xi->window.height));

                // calculate the deltas, a summation in case we get more than one mouse move
                // event within a single frame
                //
                Vec2S delta_screen = V2S_Sub(mouse->position.screen, old_screen);
                Vec2F delta_clip   = V2F_Sub(mouse->position.clip,   old_clip);

                mouse->delta.screen = V2S_Add(mouse->delta.screen, delta_screen);
                mouse->delta.clip   = V2F_Add(mouse->delta.clip,   delta_clip);

                mouse->active = true;
            }
            break;

            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL: {
                B32 vertical = (msg.message == WM_MOUSEWHEEL);
                F32 delta    = GET_WHEEL_DELTA_WPARAM(msg.wParam) / cast(F32) WHEEL_DELTA;

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
    if (!context->window.user_resizing && !context->window.maximised) {
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

        if (xi->window.state != WINDOW_STATE_FULLSCREEN) {
            // :note change the window resoluation if the application has modified
            // it directly, this is ignored when the window is in fullscreen mode
            //
            RECT client_rect;
            GetClientRect(hwnd, &client_rect);

            U32 width  = (client_rect.right  - client_rect.left);
            U32 height = (client_rect.bottom - client_rect.top);

            F32 current_scale = cast(F32) context->window.dpi / USER_DEFAULT_SCREEN_DPI;

            U32 desired_width  = cast(U32) (current_scale * xi->window.width);
            U32 desired_height = cast(U32) (current_scale * xi->window.height);

            if (width != desired_width || height != desired_height) {
                LONG     style   = GetWindowLongW(hwnd, GWL_STYLE);
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
            LONG new_style = Win32_WindowStyleForState(hwnd, xi->window.state);

            S32 x = 0, y = 0;
            S32 w = 0, h = 0;
            UINT flags = SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER;

            if (context->window.last_state == WINDOW_STATE_FULLSCREEN) {
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

            if (xi->window.state == WINDOW_STATE_FULLSCREEN) {
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

    if (!Str8_Equal(xi->window.title, context->window.title.str, 0)) {
        // :note update window title if it has changed
        //
        context->window.title.used = Min(xi->window.title.count, context->window.title.limit);
        MemoryCopy(context->window.title.data, xi->window.title.data, context->window.title.used);

        LPWSTR new_title = Win32_Str8ToStr16(xi->window.title);
        SetWindowTextW(context->window.handle, new_title);
    }

    // update window state
    //
    RECT client_area;
    GetClientRect(context->window.handle, &client_area);

    U32 width  = (client_area.right - client_area.left);
    U32 height = (client_area.bottom - client_area.top);

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
    for (Win32_DisplayInfo *dpy = context->displays.first;
            dpy != 0;
            dpy = dpy->next)
    {
        if (dpy->handle == monitor) {
            xi->window.display = dpy->index;
            break;
        }
    }

    Xi_GameCode *game = context->game.code;
    if (game->reloadable) {
        // reload the code if it needs to be
        //
        Win32_GameCodeReload(context);
    }
}

FileScope DWORD WINAPI Win32_GameThread(LPVOID param) {
    DWORD result = 0;

    Win32_Context *context = cast(Win32_Context *) param;

    Xi_Engine *xi = &context->xi;

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

        FileHandle *out = &xi->system.out;
        FileHandle *err = &xi->system.err;

        out->v = cast(U64) hOut;
        err->v = cast(U64) hErr;

        out->status = (hOut == INVALID_HANDLE_VALUE) ? FILE_HANDLE_STATUS_FAILED_OPEN : FILE_HANDLE_STATUS_VALID;
        err->status = (hErr == INVALID_HANDLE_VALUE) ? FILE_HANDLE_STATUS_FAILED_OPEN : FILE_HANDLE_STATUS_VALID;
    }

    // setup title buffer
    //
    context->window.title.used  = 0;
    context->window.title.limit = MAX_TITLE_COUNT;
    context->window.title.data  = M_ArenaPush(context->arena, U8, MAX_TITLE_COUNT);

    LPWSTR exe_path     = Win32_SystemPathGet(WIN32_PATH_TYPE_EXECUTABLE);
    LPWSTR temp_path    = Win32_SystemPathGet(WIN32_PATH_TYPE_TEMP);
    LPWSTR user_path    = Win32_SystemPathGet(WIN32_PATH_TYPE_USER);
    LPWSTR working_path = Win32_SystemPathGet(WIN32_PATH_TYPE_WORKING);

    xi->system.executable_path = Win32_PathConvertToAPI(context->arena, exe_path);
    xi->system.temp_path       = Win32_PathConvertToAPI(context->arena, temp_path);
    xi->system.user_path       = Win32_PathConvertToAPI(context->arena, user_path);
    xi->system.working_path    = Win32_PathConvertToAPI(context->arena, working_path);

    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);

    xi->system.num_cpus = system_info.dwNumberOfProcessors;

    Win32_GameCodeInit(context);

    Xi_GameCode *game = context->game.code;

    // call pre-init for engine system configurations
    //
    // @todo: this could, and probably should, just be replaced with a configuration file in
    // the future. or could just have both a pre-init call and a configuration file
    //
    game->Init(xi, XI_ENGINE_CONFIGURE);

    // open console if requested
    //
    if (xi->system.use_console) {
        xi->system.use_console = AttachConsole(ATTACH_PARENT_PROCESS);
        if (!xi->system.use_console) {
            xi->system.use_console = AllocConsole();
        }

        if (xi->system.use_console) {
            // this only seems to work for the main executable and not the loaded game
            // code .dll, i guess the .dll inherits the stdout/stderr handles when it is loaded
            // and they don't get updated after the fact
            //
            // this is backed up by the fact that once the game code is dynamically reloaded it
            // starts working...
            //
            // :hacky_console
            //
            FileHandle *out = &xi->system.out;
            FileHandle *err = &xi->system.err;

            HANDLE hOut = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

            out->v = cast(U64) hOut;
            err->v = cast(U64) hOut;

            if (hOut != INVALID_HANDLE_VALUE) {
                // update the handles to reflect the redirected ones
                //
                SetStdHandle(STD_OUTPUT_HANDLE, hOut);
                SetStdHandle(STD_ERROR_HANDLE,  hOut);

                out->status = err->status = FILE_HANDLE_STATUS_VALID;

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
                //
            }
            else {
                out->status = err->status = FILE_HANDLE_STATUS_FAILED_OPEN;
            }
        }
    }

    //
    // configure window
    //
    HWND hwnd = context->window.handle;

    U32 dpy_index = Min(xi->window.display, context->displays.count);

    Win32_DisplayInfo *display = context->displays.first;
    for (U32 it = 0; it < dpy_index; it += 1) {
        display = display->next;
    }

    Assert(display != 0);

    F32 scale = display->dpi / 96.0f;

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
    rect.right  = cast(LONG) (scale * xi->window.width);
    rect.bottom = cast(LONG) (scale * xi->window.height);

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
    WindowState state = xi->window.state;
    if (state == WINDOW_STATE_FULLSCREEN) { state = WINDOW_STATE_WINDOWED; }

    dwStyle = Win32_WindowStyleForState(hwnd, state);

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
    {
        U32 dpy_width  = (display->bounds.right  - display->bounds.left);
        U32 dpy_height = (display->bounds.bottom - display->bounds.top);

        X = display->bounds.left + (cast(S32) (dpy_width  - nWidth)  >> 1);
        Y = display->bounds.top  + (cast(S32) (dpy_height - nHeight) >> 1);
    }

    // as the window was previously created with default parameters we can just select the correct
    // dimensions specified by the user and then set the window position/size to that, this
    // also handles the fact that windows will ignore borderless styles when initially creating a window
    //
    SetWindowLongW(hwnd, GWL_STYLE, dwStyle);
    SetWindowPos(hwnd, HWND_TOP, X, Y, nWidth, nHeight, swp_flags);

    if (xi->window.title.count) {
        xi->window.title.count = Min(xi->window.title.count, context->window.title.limit);

        LPWSTR title = Win32_Str8ToStr16(xi->window.title);
        SetWindowTextW(hwnd, title);

        // copy it into our buffer as we need to own any pointer in case they are in static storage and get
        // reloaded
        //
        MemoryCopy(context->window.title.data, xi->window.title.data, xi->window.title.count);
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
    if (xi->window.state == WINDOW_STATE_FULLSCREEN) {
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
            LONG style = Win32_WindowStyleForState(hwnd, xi->window.state);

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
    // THIS IS SO BAD!!! PLEASEEEEEEEEEEEE CHANGE IT
    //
    RendererContext *renderer = &xi->renderer;
#if 0
    {
        renderer->setup.window_dim.w = xi->window.width;
        renderer->setup.window_dim.h = xi->window.height;

        HMODULE lib = LoadLibraryA("xi_opengld.dll");
        if (lib) {
            context->renderer.lib  = lib;
            context->renderer.Init = cast(RendererInitProc *) GetProcAddress(lib, "GL_Init");

            if (context->renderer.Init) {
                Win32_WindowData data = { 0 }; // :renderer_core
                data.hInstance = GetModuleHandleW(0);
                data.hwnd      = hwnd;

                context->renderer.valid = context->renderer.Init(renderer, &data);
            }
        }
    }
#else
    {
        renderer->setup.window_dim.w = xi->window.width;
        renderer->setup.window_dim.h = xi->window.height;

        HMODULE lib = LoadLibraryA("xi_vulkan.dll");
        if (lib) {
            context->renderer.lib  = lib;
            context->renderer.Init = cast(RendererInitProc *) GetProcAddress(lib, "VK_Init");

            if (context->renderer.Init) {
                Win32_WindowData data = { 0 };
                data.hInstance = GetModuleHandleW(0);
                data.hwnd      = hwnd;

                renderer->setup.debug = true;

                context->renderer.valid = context->renderer.Init(renderer, &data);
            }
        }
    }
#endif

    if (context->renderer.valid) {
        // audio may fail to initialise, in that case we just continue without audio
        //
        Win32_AudioInit(context);

        // setup thread pool
        //
        ThreadPoolInit(context->arena, &xi->thread_pool, system_info.dwNumberOfProcessors);
        AssetManagerInit(context->arena, &xi->assets, xi);

        // setup timing information
        //
        {
            if (xi->time.delta.fixed_hz == 0) {
                xi->time.delta.fixed_hz = 100;
            }

            U64 fixed_ns      = (1000000000 / xi->time.delta.fixed_hz);
            context->fixed_ns = fixed_ns;

            if (xi->time.delta.clamp_s <= 0.0f) {
                xi->time.delta.clamp_s = 0.2f;
                context->clamp_ns = 200000000;
            }
            else {
                context->clamp_ns = cast(U64) (1000000000 * xi->time.delta.clamp_s);
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
        game->Init(xi, XI_GAME_INIT);

        while (true) {
            Win32_ContextUpdate(context);

            // :temp_usage temporary memory is reset after the xiContext is updated this means
            // temporary allocations can be used inside the update function without worring about
            // it being released and/or reused
            //
            M_TempReset();

            while (context->accum_ns >= cast(S64) context->fixed_ns) {
                // do while loop to force at least one update per iteration
                //
                game->Simulate(xi);
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

            game->Render(xi, renderer);

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
            if (xi->thread_pool.num_threads == 1) { ThreadPoolAwaitComplete(&xi->thread_pool); }

            // we check if the renderer is valid and do not run in the case it is not
            // this allows us to assume the submit function is always there
            //
            renderer->Submit(renderer);

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
        ThreadPoolAwaitComplete(&xi->thread_pool);
    }

    // tell our main thread that is processing our messages that we have now quit and it should
    // exit its message loop, it will wait for us to actually finish processing, after this
    // line is is assumed that our window is no longer valid
    //
    PostMessageW(hwnd, WM_QUIT, (WPARAM) result, 0);
    return result;
}

int Xi_EngineRun(Xi_GameCode *code) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSW wndclass = { 0 };
    wndclass.style         = CS_OWNDC;
    wndclass.lpfnWndProc   = Win32_MainWindowHandler;
    wndclass.hInstance     = GetModuleHandleW(0);
    wndclass.hIcon         = LoadIconA(0, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursorA(0, IDC_ARROW);
    wndclass.hbrBackground = cast(HBRUSH) GetStockObject(BLACK_BRUSH);
    wndclass.lpszClassName = L"xi_game_window_class";

    // @todo: This is going to have a large overhaul, it is way over complicated! The
    // 'Simulate' and 'Render' functions will be rolled into a single function then
    // we can just call the loaded game 'tick' function from WM_SIZE/WM_PAINT do update
    // dynamically as we are resizing. This prevents the need for the extra thread etc.
    //

    if (RegisterClassW(&wndclass)) {
        M_Arena *arena = M_ArenaAlloc(GB(8), 0);
        Win32_Context *context = M_ArenaPush(arena, Win32_Context);

        context->arena     = arena;
        context->game.code = code;

        // acquire information about the displays connected
        //
        Win32_DisplayInfoGet(context);

        if (context->displays.count != 0) {
            // center on the first monitor, which will be the default if the user chooses not
            // to configure any of the window properties
            //
            Win32_DisplayInfo *display = context->displays.first;

            F32 scale = display->dpi / 96.0f;

            U32 dpy_width  = (display->bounds.right  - display->bounds.left);
            U32 dpy_height = (display->bounds.bottom - display->bounds.top);

            int X, Y;
            int nWidth = 1280, nHeight = 720;

            DWORD dwStyle = WS_OVERLAPPEDWINDOW;

            RECT client;
            client.left   = 0;
            client.top    = 0;
            client.right  = cast(LONG) (scale * nWidth);
            client.bottom = cast(LONG) (scale * nHeight);

            if (AdjustWindowRectExForDpi(&client, dwStyle, FALSE, 0, display->dpi)) {
                nWidth  = (client.right - client.left);
                nHeight = (client.bottom - client.top);
            }

            X = display->bounds.left + (cast(S32) (dpy_width  - nWidth)  >> 1);
            Y = display->bounds.top  + (cast(S32) (dpy_height - nHeight) >> 1);

            HWND hwnd = CreateWindowExW(0, wndclass.lpszClassName, L"xie", dwStyle, X, Y, nWidth, nHeight, 0, 0, wndclass.hInstance, 0);

            if (hwnd != 0) {
                context->window.handle = hwnd;
                context->running       = true;

                SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) context);

                DWORD game_thread;
                HANDLE hThread = CreateThread(0, 0, Win32_GameThread, context, 0, &game_thread);
                if (hThread != 0) {
                    while (true) {
                        MSG msg;
                        if (!GetMessageW(&msg, 0, 0, 0)) {
                            // we got a WM_QUIT message so we should quit
                            //
                            Assert(msg.message == WM_QUIT);
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
