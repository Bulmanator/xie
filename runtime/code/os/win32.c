#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <shellscalingapi.h>
#include <shlobj.h>

#include <stdio.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

#define XI_MAX_GAME_LOAD_TRIES 10

// custom messages to create and destroy our threaded window
//
#define XI_CREATE_WINDOW  (WM_USER + 0x1234)
#define XI_DESTROY_WINDOW (WM_USER + 0x1235)

#define XI_MAX_TITLE_COUNT 1024

// information passed to XI_CREATE_WINDOW
//
typedef struct xiWin32WindowInfo {
    LPCWSTR   lpClassName;
    LPCWSTR   lpWindowName;
    DWORD     dwStyle;
    int       X;
    int       Y;
    int       nWidth;
    int       nHeight;
    HINSTANCE hInstance;
} xiWin32WindowInfo;

typedef struct xiWin32DisplayInfo {
    HMONITOR handle;

    b32 is_primary;

    // this is to get the actual device name
    //
    string gdi_name; // \\.\DISPLAY[n]

    RECT bounds;
    u32 dpi; // raw dpi value

    xiDisplay xi;
} xiWin32DisplayInfo;

// context containing any required win32 information
//
typedef struct xiWin32Context {
    xiContext xi;

    b32 running;

    xiArena arena;

    DWORD main_thread;
    HWND  create_window;

    u32 display_count;
    xiWin32DisplayInfo displays[XI_MAX_DISPLAYS];

    struct {
        HWND handle;

        LONG windowed_style; // style before entering fullscreen
        WINDOWPLACEMENT placement;

        b32 user_resizing;

        u32 dpi;

        u32    last_state;
        buffer title;
    } window;

    struct {
        b32 dynamic;
        u64 last_time;

        HMODULE dll;
        LPWSTR dll_src_path; // where the game dll is compiled to
        LPWSTR dll_dst_path; // where the game dll is copied to prevent file locking

        LPCSTR init_name;
        LPCSTR simulate_name;
        LPCSTR render_name;

        xiGameInit   *init;
        xiGameRender *render;

        xiGameCode *code;
    } game;
} xiWin32Context;

// :note win32 memory allocation functions
//
uptr xi_os_allocation_granularity_get() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    uptr result = info.dwAllocationGranularity;
    return result;
}

uptr xi_os_page_size_get() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    uptr result = info.dwPageSize;
    return result;
}

void *xi_os_virtual_memory_reserve(uptr size) {
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
    return result;
}

b32 xi_os_virtual_memory_commit(void *base, uptr size) {
    b32 result = VirtualAlloc(base, size, MEM_COMMIT, PAGE_READWRITE) != 0;
    return result;
}

void xi_os_virtual_memory_decommit(void *base, uptr size) {
    VirtualFree(base, size, MEM_DECOMMIT);
}

void xi_os_virtual_memory_release(void *base, uptr size) {
    (void) size; // :note not needed by VirtualFree when releasing memory

    VirtualFree(base, 0, MEM_RELEASE);
}

// :note win32 run functions
//

//
// :note utility functions
//

static DWORD win32_wstr_length_get(LPCWSTR wstr) {
    DWORD result = 0;
    while (wstr[result] != L'\0') {
        result += 1;
    }

    return result;
}

static string win32_wcstr_wrap(LPWSTR str) {
    string result;
    result.count = (win32_wstr_length_get(str) << 1);
    result.data  = (u8 *) str;

    return result;
}

static string win32_utf8_to_utf16(xiArena *arena, string str) {
    string result = { 0 };

    DWORD count = MultiByteToWideChar(CP_UTF8, 0, (LPCCH) str.data, (int) str.count, 0, 0);
    if (count) {
        LPWSTR wstr = xi_arena_push_array(arena, WCHAR, count + 1);

        MultiByteToWideChar(CP_UTF8, 0, (LPCCH) str.data, (int) str.count, wstr, count);
        wstr[count] = 0;

        result.count = (count << 1);
        result.data  = (u8 *) wstr;
    }

    return result;
}

static string win32_utf16_to_utf8(xiArena *arena, string str) {
    string result = { 0 };

    // @todo: we should not require the input string to be null-terminated here,
    // we can basically do the same as the function above but in reverse
    //

    DWORD count = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH) str.data, -1, 0, 0, 0, 0);
    if (count) {
        result.count = count - 1;
        result.data  = xi_arena_push_array(arena, u8, count);

        WideCharToMultiByte(CP_UTF8, 0, (LPCWCH) str.data, -1, (LPSTR) result.data, count, 0, 0);
    }

    return result;
}

//
// :note window and display management
//
static LONG win32_window_style_get_for_state(LONG old_state, u32 state) {
    LONG result = old_state;

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
        case XI_WINDOW_STATE_BORDERLESS:
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

static LRESULT CALLBACK win32_window_creation_handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    switch (message) {
        case XI_CREATE_WINDOW: {
            xiWin32WindowInfo *info = (xiWin32WindowInfo *) wParam;

            result = (LRESULT) CreateWindowExW(0, info->lpClassName, info->lpWindowName,
                    info->dwStyle, info->X, info->Y, info->nWidth, info->nHeight, 0, 0, info->hInstance, 0);
        }
        break;

        case XI_DESTROY_WINDOW: {
            DestroyWindow((HWND) wParam);
        }
        break;

        default: {
            result = DefWindowProcW(hwnd, message, wParam, lParam);
        } break;
    }

    return result;
}

static LRESULT CALLBACK win32_main_window_handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    xiWin32Context *context = (xiWin32Context *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (message) {
        case WM_CLOSE: {
            PostThreadMessageW(context->main_thread, message, (WPARAM) hwnd, lParam);
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
                OutputDebugStringW(L"DPI CHANGED!\n");

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

                f32 scale = (f32) dpi / context->window.dpi;

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

static BOOL CALLBACK win32_displays_enumerate(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lpRect, LPARAM lParam) {
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
                info->xi.scale = (f32) xdpi / 96.0f;
                info->bounds   = monitor_info.rcMonitor;
                info->dpi      = xdpi;

                DEVMODEW mode = { 0 };
                mode.dmSize = sizeof(DEVMODEW);
                if (EnumDisplaySettingsW(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &mode)) {
                    OutputDebugStringW(monitor_info.szDevice);
                    OutputDebugStringW(L"\n");

                    string name = win32_wcstr_wrap(monitor_info.szDevice);

                    info->gdi_name.count = name.count;
                    info->gdi_name.data  = xi_arena_push_copy(&context->arena, name.data, name.count);

                    info->handle = hMonitor;

                    info->xi.width  = mode.dmPelsWidth;
                    info->xi.height = mode.dmPelsHeight;

                    info->xi.refresh_rate = (f32) mode.dmDisplayFrequency;

                    info->is_primary = (monitor_info.dwFlags & MONITORINFOF_PRIMARY) != 0;

                    context->display_count += 1;
                }
            }
        }
    }

    return result;
}

static void win32_display_info_get(xiWin32Context *context) {
    EnumDisplayMonitors(0, 0, win32_displays_enumerate, (LPARAM) &context->xi);

    xiArena *temp = xi_temp_get();

    u32 path_count = 0;
    u32 mode_count = 0;

    DISPLAYCONFIG_PATH_INFO *paths = 0;
    DISPLAYCONFIG_MODE_INFO *modes = 0;

    u32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;

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
        for (u32 it = 0; it < path_count; ++it) {
            DISPLAYCONFIG_PATH_INFO *path = &paths[it];

            DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name = { 0 };
            source_name.header.adapterId = path->sourceInfo.adapterId;
            source_name.header.id        = path->sourceInfo.id;
            source_name.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
            source_name.header.size      = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);

            result = DisplayConfigGetDeviceInfo(&source_name.header);
            if (result == ERROR_SUCCESS) {
                string gdi_name = win32_wcstr_wrap(source_name.viewGdiDeviceName);

                DISPLAYCONFIG_TARGET_DEVICE_NAME target_name = { 0 };
                target_name.header.adapterId = path->targetInfo.adapterId;
                target_name.header.id        = path->targetInfo.id;
                target_name.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
                target_name.header.size      = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);

                result = DisplayConfigGetDeviceInfo(&target_name.header);
                if (result == ERROR_SUCCESS) {
                    string name = win32_wcstr_wrap(target_name.monitorFriendlyDeviceName);

                    for (u32 d = 0; d < context->display_count; ++d) {
                        xiWin32DisplayInfo *display = &context->displays[d];
                        if (xi_str_equal(display->gdi_name, gdi_name)) {
                            display->xi.name = win32_utf16_to_utf8(&context->arena, name);
                        }
                    }
                }
            }
        }
    }

    xiContext *xi = &context->xi;

    xi->system.display_count = context->display_count;

    for (u32 it = 0; it < context->display_count; ++it) {
        xi->system.displays[it] = context->displays[it].xi;
    }
}

//
// :note file system and path management
//
// all paths returned from these functions are encoded in utf-16 for use with other winapi functions,
// when they are passed back to user facing code they are converted to utf-8 and all backslash separators
// are converted to forward slashes
//
enum xiWin32PathType {
    WIN32_PATH_TYPE_EXECUTABLE = 0,
    WIN32_PATH_TYPE_WORKING,
    WIN32_PATH_TYPE_USER,
    WIN32_PATH_TYPE_TEMP
};

static string win32_system_path_get(xiArena *arena, u32 type, b32 convert_backslashes) {
    string result = { 0 };

    xiArena *temp = xi_temp_get();

    switch (type) {
        case WIN32_PATH_TYPE_EXECUTABLE: {
            DWORD buffer_count = MAX_PATH;

            // get the executable filename, windows doesn't just tell us how long the path
            // is so we have to do this loop until we create a buffer that is big enough, in
            // most cases MAX_PATH will suffice
            //
            DWORD  count;
            WCHAR *path;
            do {
                path  = xi_arena_push_array(temp, WCHAR, buffer_count);
                count = GetModuleFileNameW(0, path, buffer_count);

                buffer_count *= 2;
            } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

            // find the last backslash path separator
            //
            while (count--) {
                if (path[count] == L'\\') {
                    break;
                }
            }

            // null terminate the string
            //
            path[count] = L'\0';

            result.count = count << 1; // in bytes
            result.data  = (u8 *) xi_arena_push_array_copy(arena, path, WCHAR, count + 1); // keep null
        }
        break;
        case WIN32_PATH_TYPE_WORKING: {
            WCHAR *path = xi_arena_push_array(arena, WCHAR, MAX_PATH);

            // the documentation tells you not to use this in multi-threaded applications due to the
            // path being stored in a global variable, while this is technically a multi-threaded application
            // it currently only has one "main" thread so it'll be fine unless windows is reading/writing
            // with its own threads in the background
            //
            DWORD count = GetCurrentDirectoryW(MAX_PATH, path);
            if (count > MAX_PATH) {
                xi_arena_pop_array(arena, WCHAR, MAX_PATH);

                path  = xi_arena_push_array(arena, WCHAR, count);
                count = GetCurrentDirectoryW(count, path);
            }
            else {
                xi_arena_pop_array(arena, WCHAR, MAX_PATH - count - 1);
            }

            result.count = count << 1;
            result.data  = (u8 *) path;
        }
        break;
        case WIN32_PATH_TYPE_USER: {
            PWSTR path;
            if (SHGetKnownFolderPath(&FOLDERID_RoamingAppData, 0, 0, &path) == S_OK) {
                DWORD len = win32_wstr_length_get(path);

                result.count = len << 1;
                result.data  = (u8 *) xi_arena_push_array_copy(arena, path, WCHAR, len + 1);
            }

            CoTaskMemFree(path);
        }
        break;
        case WIN32_PATH_TYPE_TEMP: {
            // according to the documentation the temp path cannot exceed MAX_PATH + 1 excluding null
            // terminating byte
            //
            // the documentation also stated GetTempPath2W should be called instead of GetTempPathW, even
            // though for non-SYSTEM level processes they are functionally identical, but it wouldn't
            // locate the function in kerne32.dll so it must be a recent addition?
            //
            WCHAR path[MAX_PATH + 1];
            DWORD count = GetTempPathW(MAX_PATH + 1, path);
            if (count) {
                path[count - 1] = L'\0'; // null-terminate at the end, this removes the final backslash as well

                result.count = (count - 1) << 1;
                result.data  = (u8 *) xi_arena_push_array_copy(arena, path, WCHAR, count);
            }
        }
        break;
        default: {} break;
    }

    if (result.data && convert_backslashes) {
        WCHAR *path = (WCHAR *) result.data;

        DWORD it = 0;
        while (path[it] != 0) {
            if (path[it] == L'\\') {
                path[it] = L'/';
            }

            it += 1;
        }
    }

    return result;
}

//
// :note game code management
//
static b32 win32_game_code_is_valid(xiWin32Context *context) {
    b32 result = (context->game.init != 0) && (context->game.render != 0); // && (context->game.simulate != 0)
    return result;
}

static void win32_game_code_init(xiWin32Context *context) {
    xiArena *temp = xi_temp_get();

    xiGameCode *code = context->game.code;

    if (code->type == XI_GAME_CODE_TYPE_STATIC) {
        context->game.init     = code->functions.init;
        // context->game.simulate = code->functions.simulate;
        context->game.render   = code->functions.render;

        context->game.dynamic  = false;
    }
    else {
        context->game.dynamic = true;

        string exe_path = win32_system_path_get(temp, WIN32_PATH_TYPE_EXECUTABLE, false);
        string dll_name = win32_utf8_to_utf16(temp, code->names.lib);

        buffer src_path;
        src_path.used  = 0;
        src_path.limit = exe_path.count + dll_name.count + 12;
        src_path.data  = xi_arena_push_size(&context->arena, src_path.limit);

        xi_buffer_append(&src_path, exe_path.data, exe_path.count);
        xi_buffer_append(&src_path, L"\\", sizeof(WCHAR));
        xi_buffer_append(&src_path, dll_name.data, dll_name.count);
        xi_buffer_append(&src_path, L".dll", 5 * sizeof(WCHAR)); // include null byte

        context->game.dll_src_path = (LPWSTR) src_path.data;

        // get temporary path to copy dll file to
        //
        string temp_path = win32_system_path_get(temp, WIN32_PATH_TYPE_TEMP, false);

        WCHAR dst_path[MAX_PATH];
        if (GetTempFileNameW((LPCWSTR) temp_path.data, L"xie", 0x1234, dst_path)) {
            u32 count = win32_wstr_length_get(dst_path) + 1;

            // @incomplete: swap to copy array

            context->game.dll_dst_path = xi_arena_push_array(&context->arena, WCHAR, count);
            xi_memory_copy(context->game.dll_dst_path, dst_path, count << 1);
        }

        if (CopyFileW(context->game.dll_src_path, context->game.dll_dst_path, FALSE)) {
            context->game.dll = LoadLibraryW(context->game.dll_dst_path);
            if (context->game.dll) {
                LPSTR init     = xi_arena_push_copy(&context->arena, code->names.init.data,     code->names.init.count     + 1);
                LPSTR simulate = xi_arena_push_copy(&context->arena, code->names.simulate.data, code->names.simulate.count + 1);
                LPSTR render   = xi_arena_push_copy(&context->arena, code->names.render.data,   code->names.render.count   + 1);

                context->game.init_name     = init;
                context->game.simulate_name = simulate;
                context->game.render_name   = render;

                context->game.init   = (xiGameInit *)   GetProcAddress(context->game.dll, init);
                context->game.render = (xiGameRender *) GetProcAddress(context->game.dll, render);

                if (win32_game_code_is_valid(context)) {
                    WIN32_FILE_ATTRIBUTE_DATA attributes;
                    if (GetFileAttributesExW(context->game.dll_src_path, GetFileExInfoStandard, &attributes)) {
                        context->game.last_time =
                            ((u64) attributes.ftLastWriteTime.dwLowDateTime  << 32) |
                            ((u64) attributes.ftLastWriteTime.dwHighDateTime << 0);

                    }
                }
            }
        }
    }
}

static void win32_game_code_reload(xiWin32Context *context) {
    // @todo: we should really be reading the .dll file to see if it has debug information. this is
    // because the .pdb file location is hardcoded into the debug data directory. when debugging on
    // some debuggers (like microsoft visual studio) it will lock the .pdb file and prevent the compiler
    // from writing to it causing a whole host of issues
    //
    // there are several ways around this, loading the .dll patching the .pdb path to something unique
    // is the "proper" way, you can also specify the /pdb linker flag with msvc.
    //
    // this doesn't affect because remedybg doesn't lock the .pdb file
    //
    WIN32_FILE_ATTRIBUTE_DATA attributes;
    if (GetFileAttributesExW(context->game.dll_src_path, GetFileExInfoStandard, &attributes)) {
        u64 time = ((u64) attributes.ftLastWriteTime.dwLowDateTime  << 32) |
                   ((u64) attributes.ftLastWriteTime.dwHighDateTime << 0);

        if (time != context->game.last_time) {
            if (context->game.dll) {
                FreeLibrary(context->game.dll);

                context->game.dll    = 0;
                context->game.init   = 0;
                context->game.render = 0;
            }

            if (CopyFileW(context->game.dll_src_path, context->game.dll_dst_path, FALSE)) {
                context->game.dll = LoadLibraryW(context->game.dll_dst_path);
                if (context->game.dll) {
                    // @hardcode: should be able to change this to whatever, just need to push zstr
                    //
                    context->game.init   = (xiGameInit *)   GetProcAddress(context->game.dll, "game_init");
                    context->game.render = (xiGameRender *) GetProcAddress(context->game.dll, "game_render");

                    if (win32_game_code_is_valid(context)) {
                        context->game.last_time = time;

                        // call init again but signal to the developer that the code was reloaded
                        // rather than initially called
                        //
                        context->xi.flags |= XI_CONTEXT_FLAG_RELOADED;
                        context->game.init(&context->xi);

                        // remove the flag again so subsequent calls to simulate or render don't
                        // look like a reload
                        //
                        context->xi.flags &= ~XI_CONTEXT_FLAG_RELOADED;
                    }
                }
            }
        }
    }
}

//
// :note update pass
//

static void win32_xi_context_update(xiWin32Context *context) {
    xiContext *xi = &context->xi;

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
            // ignore resizes while in fullscreen as they don't make sense
            //
            RECT client_rect;
            GetClientRect(hwnd, &client_rect);

            u32 width  = (client_rect.right  - client_rect.left);
            u32 height = (client_rect.bottom - client_rect.top);

            f32 current_scale = (f32) context->window.dpi / USER_DEFAULT_SCREEN_DPI;

            u32 desired_width  = (u32) (current_scale * xi->window.width);
            u32 desired_height = (u32) (current_scale * xi->window.height);

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
                        width  = client_rect.right - client_rect.left;
                        height = client_rect.bottom - client_rect.top;

                        SetWindowPos(hwnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
                    }
                }
            }
        }

        if (xi->window.state != context->window.last_state) {
            LONG old_style = GetWindowLongW(hwnd, GWL_STYLE);
            LONG new_style = win32_window_style_get_for_state(old_style, xi->window.state);

            s32 x = 0, y = 0;
            s32 w = 0, h = 0;
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
        context->window.title.used = XI_MIN(xi->window.title.count, context->window.title.limit);
        xi_memory_copy(context->window.title.data, xi->window.title.data, context->window.title.used);

        xiArena *temp = xi_temp_get();
        string new_title = win32_utf8_to_utf16(temp, xi->window.title);

        SetWindowTextW(context->window.handle, (LPWSTR) new_title.data);
    }

    // process windows messages that our application received
    //
    MSG msg;
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
        switch (msg.message) {
            case WM_QUIT: { context->running = false; } break;
            case WM_CLOSE: {
                SendMessageW(context->create_window, XI_DESTROY_WINDOW, msg.wParam, 0);
                PostQuitMessage(0);
            }
            break;

            // @todo: process other messages
            //
        }
    }

    // update window state
    //
    RECT client_area;
    GetClientRect(context->window.handle, &client_area);

    u32 width  = (client_area.right - client_area.left);
    u32 height = (client_area.bottom - client_area.top);

    if (width != 0 && height != 0) {
        xi->window.width  = width;
        xi->window.height = height;
    }

    xi->window.title = context->window.title.str;

    // update the index of the display that the window is currently on
    //
    HMONITOR monitor = MonitorFromWindow(context->window.handle, MONITOR_DEFAULTTONEAREST);
    for (u32 it = 0; it < context->display_count; ++it) {
        if (context->displays[it].handle == monitor) {
            xi->window.display = it;
            break;
        }
    }

    // @todo: update other state that the game context needs and that didn't come from
    // messages
    //

    if (context->game.dynamic) {
        // reload the code if it needs to be
        //
        win32_game_code_reload(context);
    }
}

static DWORD WINAPI win32_main_thread(LPVOID param) {
    DWORD result = 0;

    xiWin32Context *context = (xiWin32Context *) param;

    WNDCLASSW wnd_class = { 0 };
    wnd_class.style         = CS_OWNDC;
    wnd_class.lpfnWndProc   = win32_main_window_handler;
    wnd_class.hInstance     = GetModuleHandleW(0);
    wnd_class.hIcon         = LoadIconA(0, IDI_APPLICATION);
    wnd_class.hCursor       = LoadCursorA(0, IDC_ARROW);
    wnd_class.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wnd_class.lpszClassName = L"xi_game_window_class";

    if (RegisterClassW(&wnd_class)) {
        win32_display_info_get(context);

        xiArena   *temp = xi_temp_get();
        xiContext *xi   = &context->xi;

        // setup title buffer
        //
        context->window.title.used  = 0;
        context->window.title.limit = XI_MAX_TITLE_COUNT;
        context->window.title.data  = xi_arena_push_array(&context->arena, u8, XI_MAX_TITLE_COUNT);

        SYSTEM_INFO system_info;
        GetSystemInfo(&system_info);

        string exe_path     = win32_system_path_get(temp, WIN32_PATH_TYPE_EXECUTABLE, true /* convert backslashes */);
        string temp_path    = win32_system_path_get(temp, WIN32_PATH_TYPE_TEMP, true);
        string user_path    = win32_system_path_get(temp, WIN32_PATH_TYPE_USER, true);
        string working_path = win32_system_path_get(temp, WIN32_PATH_TYPE_WORKING, true);

        xi->system.executable_path = win32_utf16_to_utf8(&context->arena, exe_path);
        xi->system.temp_path       = win32_utf16_to_utf8(&context->arena, temp_path);
        xi->system.user_path       = win32_utf16_to_utf8(&context->arena, user_path);
        xi->system.working_path    = win32_utf16_to_utf8(&context->arena, working_path);

        xi->system.processor_count = system_info.dwNumberOfProcessors;

        win32_game_code_init(context);
        if (win32_game_code_is_valid(context)) {
            context->game.init(xi);
        }

        u32 display_index = XI_MIN(xi->window.display, xi->system.display_count);
        xiWin32DisplayInfo *display = &context->displays[display_index];

        UINT dpi  = display->dpi;
        f32 scale = display->xi.scale;

        XI_ASSERT(scale >= 1.0f);

        xiWin32WindowInfo window_info = { 0 };
        window_info.lpClassName = wnd_class.lpszClassName;
        window_info.dwStyle     = win32_window_style_get_for_state(0, xi->window.state);
        window_info.hInstance   = wnd_class.hInstance;

        if (xi->window.width == 0 || xi->window.height == 0) {
            // just default to 1280x720 if the user doesn't specify a window size
            // could use CW_USEDEFAULT but its a pain because you can't properly
            // specify X,Y with it
            //
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

        if (AdjustWindowRectExForDpi(&rect, window_info.dwStyle, FALSE, 0, dpi)) {
            window_info.nWidth  = (rect.right - rect.left);
            window_info.nHeight = (rect.bottom - rect.top);
        }
        else {
            window_info.nWidth  = xi->window.width;
            window_info.nHeight = xi->window.height;
        }

        window_info.X = display->bounds.left + ((s32) (display->xi.width  - window_info.nWidth)  >> 1);
        window_info.Y = display->bounds.top  + ((s32) (display->xi.height - window_info.nHeight) >> 1);

        if (xi_str_is_valid(xi->window.title)) {
            xi->window.title.count = XI_MIN(xi->window.title.count, context->window.title.limit);
            string title = win32_utf8_to_utf16(temp, xi->window.title);

            window_info.lpWindowName = (LPWSTR) title.data;
        }
        else {
            window_info.lpWindowName = L"xie";
        }

        HWND hwnd = (HWND) SendMessageW(context->create_window, XI_CREATE_WINDOW, (WPARAM) &window_info, 0);
        if (hwnd != 0) {
            context->window.handle = hwnd;
            context->window.dpi    = dpi;

            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) context);

            // we have to reset the style in the case of borderless windows because windows
            // doesn't respect the dwStyle we provide to window creation and will add WS_CAPTION back
            // in giving our borderless window a titlebar that we didn't want
            //
            {
                LONG style = GetWindowLong(hwnd, GWL_STYLE);
                style      = win32_window_style_get_for_state(style, xi->window.state);

                UINT flags = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED;

                SetWindowLongW(hwnd, GWL_STYLE, style);
                SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, flags);

                context->window.last_state = xi->window.state;
            }

            // we copy the title into our own buffer, that is capped at 1024 bytes
            // in the event the incoming title is stored in .bss storage we can't store a pointer
            // to it directly due to dynamic code reloading
            //
            {
                string title = xi->window.title;
                if (!xi_str_is_valid(title)) {
                    title = (string) xi_str_wrap_const("xie");
                }

                XI_ASSERT(title.count <= XI_MAX_TITLE_COUNT);

                xi_memory_copy(context->window.title.data, title.data, title.count);

                context->window.title.used = title.count;
                xi->window.title = context->window.title.str;
            }

            // we setup the window as normal and then after creation set it to fullscreen
            // this way we can get an initial window placement to return the window back into windowed
            // mode with
            //
            if (xi->window.state == XI_WINDOW_STATE_FULLSCREEN) {
                if (GetWindowPlacement(hwnd, &context->window.placement)) {
                    int x = display->bounds.left;
                    int y = display->bounds.top;

                    int w = (display->bounds.right - display->bounds.left);
                    int h = (display->bounds.bottom - display->bounds.top);

                    SetWindowPos(hwnd, HWND_TOP, x, y, w, h, SWP_NOZORDER | SWP_NOOWNERZORDER);
                }
            }

            ShowWindow(hwnd, SW_SHOW);

            // load and initialise renderer
            //
            // @todo: make this more substantial, it is very hacky at the moment while i test stuff
            //
            HMODULE renderer_lib = LoadLibraryA("xi_opengld.dll");
            xiRendererInit *init = (xiRendererInit *) GetProcAddress(renderer_lib, "xi_opengl_init");

            xiRenderer *renderer = 0;
            if (init) {
                renderer = init(&hwnd);
            }

            context->running = true;
            while (context->running) {
                win32_xi_context_update(context);
                xi_temp_reset();

                if (renderer) {
                    if (context->game.render) {
                        context->game.render(xi, renderer);
                    }

                    renderer->setup.window_dim.w = xi->window.width;
                    renderer->setup.window_dim.h = xi->window.height;

                    renderer->submit(renderer);
                }
            }
        }
        else {
            // @todo: logging...
            //
            result = 1;
        }
    }
    else {
        // @todo: logging...
        //
        result = 1;
    }

    ExitProcess(result);
}

int xie_run(xiGameCode *code) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSW create_class = { 0 };
    create_class.lpfnWndProc   = win32_window_creation_handler;
    create_class.hInstance     = GetModuleHandleW(0);
    create_class.hIcon         = LoadIconA(0, IDI_APPLICATION);
    create_class.hCursor       = LoadCursorA(0, IDC_ARROW);
    create_class.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    create_class.lpszClassName = L"xi_create_window_class";

    if (RegisterClassW(&create_class)) {
        HWND create_window = CreateWindowExW(0, create_class.lpszClassName, L"xi_create_window", 0,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, create_class.hInstance, 0);

        if (create_window) {
            xiArena arena = { 0 };
            xi_arena_init_virtual(&arena, XI_MB(128));

            xiWin32Context *context = xi_arena_push_type(&arena, xiWin32Context);

            context->arena         = arena;
            context->game.code     = code;
            context->create_window = create_window;

            CreateThread(0, 0, win32_main_thread, context, 0, &context->main_thread);

            for (;;) {
                MSG msg;
                GetMessageW(&msg, 0, 0, 0);
                TranslateMessage(&msg);

                // @todo: we need to filter out messages that we want to process here
                //
                if (msg.message == WM_KEYUP) {
                    PostThreadMessage(context->main_thread, msg.message, msg.wParam, msg.lParam);
                }
                else {
                    DispatchMessage(&msg);
                }
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
