#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <gl/gl.h>

#include <xi/xi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "opengl32.lib")

#define WGL_LOAD_FUNCTION(x) x = (xiFunction_##x *) wglGetProcAddress(#x)

// from WGL_ARB_pixel_format
//
#define WGL_NUMBER_PIXEL_FORMATS_ARB    0x2000
#define WGL_DRAW_TO_WINDOW_ARB          0x2001
#define WGL_DRAW_TO_BITMAP_ARB          0x2002
#define WGL_ACCELERATION_ARB            0x2003
#define WGL_NEED_PALETTE_ARB            0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB     0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB      0x2006
#define WGL_SWAP_METHOD_ARB             0x2007
#define WGL_NUMBER_OVERLAYS_ARB         0x2008
#define WGL_NUMBER_UNDERLAYS_ARB        0x2009
#define WGL_TRANSPARENT_ARB             0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB   0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB  0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB             0x200C
#define WGL_SHARE_STENCIL_ARB           0x200D
#define WGL_SHARE_ACCUM_ARB             0x200E
#define WGL_SUPPORT_GDI_ARB             0x200F
#define WGL_SUPPORT_OPENGL_ARB          0x2010
#define WGL_DOUBLE_BUFFER_ARB           0x2011
#define WGL_STEREO_ARB                  0x2012
#define WGL_PIXEL_TYPE_ARB              0x2013
#define WGL_COLOR_BITS_ARB              0x2014
#define WGL_RED_BITS_ARB                0x2015
#define WGL_RED_SHIFT_ARB               0x2016
#define WGL_GREEN_BITS_ARB              0x2017
#define WGL_GREEN_SHIFT_ARB             0x2018
#define WGL_BLUE_BITS_ARB               0x2019
#define WGL_BLUE_SHIFT_ARB              0x201A
#define WGL_ALPHA_BITS_ARB              0x201B
#define WGL_ALPHA_SHIFT_ARB             0x201C
#define WGL_ACCUM_BITS_ARB              0x201D
#define WGL_ACCUM_RED_BITS_ARB          0x201E
#define WGL_ACCUM_GREEN_BITS_ARB        0x201F
#define WGL_ACCUM_BLUE_BITS_ARB         0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB        0x2021
#define WGL_DEPTH_BITS_ARB              0x2022
#define WGL_STENCIL_BITS_ARB            0x2023
#define WGL_AUX_BUFFERS_ARB             0x2024

#define WGL_NO_ACCELERATION_ARB      0x2025
#define WGL_GENERIC_ACCELERATION_ARB 0x2026
#define WGL_FULL_ACCELERATION_ARB    0x2027

#define WGL_SWAP_EXCHANGE_ARB        0x2028
#define WGL_SWAP_COPY_ARB            0x2029
#define WGL_SWAP_UNDEFINED_ARB       0x202A

#define WGL_TYPE_RGBA_ARB            0x202B
#define WGL_TYPE_COLORINDEX_ARB      0x202C

typedef BOOL xiFunction_wglGetPixelFormatAttribivARB(HDC, int, int, UINT, const int *, int *);
typedef BOOL xiFunction_wglGetPixelFormatAttribfvARB(HDC, int, int, UINT, const int *, FLOAT *);
typedef BOOL xiFunction_wglChoosePixelFormatARB(HDC, const int *, const FLOAT *, UINT, int *, UINT *);

// from EXT_framebuffer_sRGB
//
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x20A9

// from EXT_extensions_string
//
typedef const char *xiFunction_wglGetExtensionsStringEXT(void);

// from WGL_ARB_create_context
//      WGL_ARB_create_context_profile
//
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB   0x2093
#define WGL_CONTEXT_FLAGS_ARB         0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB  0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB              0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

#define ERROR_INVALID_VERSION_ARB 0x2095
#define ERROR_INVALID_PROFILE_ARB 0x2096

typedef HGLRC xiFunction_wglCreateContextAttribsARB(HDC, HGLRC, const int *);

// global function pointers for wgl extension calls
//
static xiFunction_wglGetPixelFormatAttribivARB *wglGetPixelFormatAttribivARB;
static xiFunction_wglGetPixelFormatAttribfvARB *wglGetPixelFormatAttribfvARB;
static xiFunction_wglChoosePixelFormatARB      *wglChoosePixelFormatARB;

static xiFunction_wglGetExtensionsStringEXT *wglGetExtensionsStringEXT;

static xiFunction_wglCreateContextAttribsARB *wglCreateContextAttribsARB;

extern XI_EXPORT XI_RENDERER_INIT(win32_opengl_init) {
    // @todo: configure renderer
    // @todo: error checking!
    //
    xiRenderer *result = 0;

    // we first have to make a dummy window to create an old opengl context and load
    // the new wgl choose pixel format and create context calls, after which we can
    // create a modern opengl context for our actual window
    //
    {
        WNDCLASSA wnd_class = { 0 };
        wnd_class.lpfnWndProc   = DefWindowProcA;
        wnd_class.hInstance     = GetModuleHandle(0);
        wnd_class.lpszClassName = "xie_dummy_opengl";

        RegisterClassA(&wnd_class);

        HWND hwnd = CreateWindowA(wnd_class.lpszClassName, "opengl", WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, wnd_class.hInstance, 0);

        HDC hdc = GetDC(hwnd);

        PIXELFORMATDESCRIPTOR pfd = { 0 };
        pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion     = 1;
        pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType   = PFD_TYPE_RGBA;
        pfd.cColorBits   = 24;
        pfd.cAlphaBits   = 8;
        pfd.cDepthBits   = 24;
        pfd.cStencilBits = 8;
        pfd.iLayerType   = PFD_MAIN_PLANE;

        int pixel_format = ChoosePixelFormat(hdc, &pfd);

        SetPixelFormat(hdc, pixel_format, &pfd);

        HGLRC hglrc = wglCreateContext(hdc);
        wglMakeCurrent(hdc, hglrc);

        WGL_LOAD_FUNCTION(wglGetPixelFormatAttribivARB);
        WGL_LOAD_FUNCTION(wglGetPixelFormatAttribfvARB);
        WGL_LOAD_FUNCTION(wglChoosePixelFormatARB);
        WGL_LOAD_FUNCTION(wglGetExtensionsStringEXT);
        WGL_LOAD_FUNCTION(wglCreateContextAttribsARB);

        // clean up old context and dummy window
        //
        wglMakeCurrent(0, 0);
        wglDeleteContext(hglrc);

        ReleaseDC(hwnd, hdc);

        DestroyWindow(hwnd);
        UnregisterClassA(wnd_class.lpszClassName, wnd_class.hInstance);
    }

    // @todo: go through the extensions string and check support for extensions we want
    //

    HWND window = *(HWND *) platform;
    HDC  hdc    = GetDC(window);

    {
        UINT num_attribs = 0;
        int attribs[32];

        // these are all of the basic format attribs that are by default supported and we
        // want
        //
        attribs[num_attribs++] = WGL_DRAW_TO_WINDOW_ARB;
        attribs[num_attribs++] = GL_TRUE;

        attribs[num_attribs++] = WGL_SUPPORT_OPENGL_ARB;
        attribs[num_attribs++] = GL_TRUE;

        attribs[num_attribs++] = WGL_DOUBLE_BUFFER_ARB;
        attribs[num_attribs++] = GL_TRUE;

        attribs[num_attribs++] = WGL_COLOR_BITS_ARB;
        attribs[num_attribs++] = 24;
        attribs[num_attribs++] = WGL_ALPHA_BITS_ARB;
        attribs[num_attribs++] = 8;
        attribs[num_attribs++] = WGL_DEPTH_BITS_ARB;
        attribs[num_attribs++] = 24;
        attribs[num_attribs++] = WGL_STENCIL_BITS_ARB;
        attribs[num_attribs++] = 8;

        attribs[num_attribs++] = WGL_PIXEL_TYPE_ARB;
        attribs[num_attribs++] = WGL_TYPE_RGBA_ARB;

        attribs[num_attribs++] = WGL_ACCELERATION_ARB;
        attribs[num_attribs++] = WGL_FULL_ACCELERATION_ARB;


        // @todo: check if sRGB is supported and enable here
        //

        XI_ASSERT(num_attribs <= 32);

        // end off the list with GL_NONE to signify the end
        //
        attribs[num_attribs] = GL_NONE;

        UINT num_formats;
        int pixel_format;

        wglChoosePixelFormatARB(hdc, attribs, 0, 1, &pixel_format, &num_formats);

        PIXELFORMATDESCRIPTOR pfd;
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);

        DescribePixelFormat(hdc, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        SetPixelFormat(hdc, pixel_format, &pfd);
    }

    HGLRC hglrc;
    {
        // create context
        //
        // @todo: debug bit needs to be conditional
        //
        int flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB;

        const int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_FLAGS_ARB,         flags,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            GL_NONE
        };

        hglrc = wglCreateContextAttribsARB(hdc, 0, attribs);
    }

    wglMakeCurrent(hdc, hglrc);

    return result;
}
