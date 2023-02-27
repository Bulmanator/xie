#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <shellscalingapi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shcore.lib")

#include <xi/xi.h>

#define XI_MAX_GAME_LOAD_TRIES 10

#define XI_CREATE_WINDOW  (WM_USER + 0x1234)
#define XI_DESTROY_WINDOW (WM_USER + 0x1235)

typedef struct xiWin32WindowInfo {
    LPCWSTR lpClassName;
    LPCWSTR lpWindowName;
    DWORD   dwStyle;
    int     X;
    int     Y;
    int     nWidth;
    int     nHeight;
    HINSTANCE hInstance;
} xiWin32WindowInfo;

typedef struct xiWin32Context {
    xiContext xi;

    DWORD main_thread;
    HWND  create_window;

    HWND window;
} xiWin32Context;

static LRESULT CALLBACK xi_window_creation_handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
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

static LRESULT CALLBACK xi_main_window_handler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    xiWin32Context *context = (xiWin32Context *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (message) {
        case WM_CLOSE: {
            PostThreadMessageW(context->main_thread, message, (WPARAM) hwnd, lParam);
        }
        break;
        default: {
            result = DefWindowProcW(hwnd, message, wParam, lParam);
        }
        break;
    }

    return result;
}

static BOOL CALLBACK xi_displays_enumerate(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lpRect, LPARAM lParam) {
    BOOL result = FALSE;

    (void) lpRect;
    (void) hdcMonitor;

    xiContext *xi = (xiContext *) lParam;

    if (xi->system.display_count < XI_MAX_DISPLAYS) {
        result = TRUE;

        xiDisplay *display = &xi->system.displays[xi->system.display_count];

        MONITORINFOEXW monitor_info = { 0 };
        monitor_info.cbSize = sizeof(MONITORINFOEXW);

        if (GetMonitorInfoW(hMonitor, (LPMONITORINFO) &monitor_info)) {
            UINT xdpi, ydpi;
            if (GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi) == S_OK) {
                display->scale = (f32) xdpi / 96.0f;

                DEVMODEW mode = { 0 };
                mode.dmSize = sizeof(DEVMODEW);
                if (EnumDisplaySettingsW(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &mode)) {
                    display->width        = mode.dmPelsWidth;
                    display->height       = mode.dmPelsHeight;
                    display->refresh_rate = (f32) mode.dmDisplayFrequency;

                    xi->system.display_count += 1;
                }
            }
        }
    }

    return result;
}

static DWORD WINAPI xi_main_thread(LPVOID param) {
    DWORD result = 0;

    xiWin32Context *context = (xiWin32Context *) param;

    // @todo: eventually we will load a configuration file here which will tell us some information
    // about the game, where the loaded  code is and its functions names, set the icon etc.
    //

    WNDCLASSW wnd_class = { 0 };
    wnd_class.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wnd_class.lpfnWndProc   = xi_main_window_handler;
    wnd_class.hInstance     = GetModuleHandleW(0);
    wnd_class.hIcon         = LoadIconA(0, IDI_APPLICATION);
    wnd_class.hCursor       = LoadCursorA(0, IDC_ARROW);
    wnd_class.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wnd_class.lpszClassName = L"xi_game_window_class";

    if (RegisterClassW(&wnd_class)) {
        EnumDisplayMonitors(0, 0, xi_displays_enumerate, (LPARAM) &context->xi);

        SYSTEM_INFO system_info;
        GetSystemInfo(&system_info);

        context->xi.system.processor_count = system_info.dwNumberOfProcessors;

        // @todo: load game dll and call the user-side init function, this is where
        // the user is expected to fill out xi.window information to be used below
        // when actually creating the window
        //

        xiWin32WindowInfo window_info = { 0 };
        window_info.lpClassName  = wnd_class.lpszClassName;
        window_info.lpWindowName = L"xie";
        window_info.dwStyle      = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        window_info.X            = CW_USEDEFAULT;
        window_info.Y            = CW_USEDEFAULT;
        window_info.nWidth       = CW_USEDEFAULT;
        window_info.nHeight      = CW_USEDEFAULT;
        window_info.hInstance    = wnd_class.hInstance;

        context->window = (HWND) SendMessageW(context->create_window, XI_CREATE_WINDOW, (WPARAM) &window_info, 0);
        if (context->window) {
            SetWindowLongPtrW(context->window, GWLP_USERDATA, (LONG_PTR) context);

            b32 running = true;
            while (running) {
                MSG msg;
                while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
                    switch (msg.message) {
                        case WM_QUIT: { running = false; } break;
                        case WM_CLOSE: {
                            SendMessageW(context->create_window, XI_DESTROY_WINDOW, msg.wParam, 0);
                            PostQuitMessage(0);
                        }
                        break;
                    }
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

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    (void) hPrevInstance;
    (void) lpCmdLine;
    (void) nCmdShow;

    WNDCLASSW create_class = { 0 };
    create_class.lpfnWndProc   = xi_window_creation_handler;
    create_class.hInstance     = hInstance;
    create_class.hIcon         = LoadIconA(0, IDI_APPLICATION);
    create_class.hCursor       = LoadCursorA(0, IDC_ARROW);
    create_class.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    create_class.lpszClassName = L"xi_create_window_class";

    if (RegisterClassW(&create_class)) {
        HWND create_window = CreateWindowExW(0, create_class.lpszClassName, L"xi_create_window", 0,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

        if (create_window) {
            xiWin32Context context = { 0 };

            context.create_window = create_window;

            CreateThread(0, 0, xi_main_thread, &context, 0, &context.main_thread);

            for (;;) {
                MSG msg;
                GetMessageW(&msg, 0, 0, 0);

                // @todo: we need to filter out messages that we want to process here
                //

                DispatchMessage(&msg);
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
