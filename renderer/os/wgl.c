#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "opengl32.lib")

#define WGL_LOAD_FUNCTION(prefix, x) (prefix)->x = (xiOpenGL_##prefix##x *) wglGetProcAddress(XI_STRINGIFY(prefix##x))

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

typedef BOOL xiOpenGL_wglGetPixelFormatAttribivARB(HDC, int, int, UINT, const int *, int *);
typedef BOOL xiOpenGL_wglGetPixelFormatAttribfvARB(HDC, int, int, UINT, const int *, FLOAT *);
typedef BOOL xiOpenGL_wglChoosePixelFormatARB(HDC, const int *, const FLOAT *, UINT, int *, UINT *);

// from EXT_framebuffer_sRGB
//
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x20A9

// from EXT_extensions_string
//
typedef const char *xiOpenGL_wglGetExtensionsStringEXT(void);

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

typedef HGLRC xiOpenGL_wglCreateContextAttribsARB(HDC, HGLRC, const int *);

// from EXT_swap_control
//
typedef BOOL xiOpenGL_wglSwapIntervalEXT(int);
typedef int  xiOpenGL_wglGetSwapIntervalEXT(void);

#define WGL_FUNCTION_POINTER(name) xiOpenGL_wgl##name *name

// main wgl context, this houses the win32 specific stuff
//
typedef struct xiWin32OpenGLContext {
    xiOpenGLContext gl;

    b32 valid;

    HWND  hwnd;
    HDC   hdc;
    HGLRC hglrc;

    // extension support
    //
    b32 WGL_EXT_framebuffer_sRGB;
    b32 WGL_ARB_pixel_format;
    b32 WGL_ARB_create_context;
    b32 WGL_ARB_multisample;
    b32 WGL_EXT_swap_control;

    WGL_FUNCTION_POINTER(GetExtensionsStringEXT);

    WGL_FUNCTION_POINTER(ChoosePixelFormatARB);
    WGL_FUNCTION_POINTER(CreateContextAttribsARB);

    WGL_FUNCTION_POINTER(SwapIntervalEXT);
    WGL_FUNCTION_POINTER(GetSwapIntervalEXT);
} xiWin32OpenGLContext;

#undef WGL_FUNCTION_POINTER

static XI_RENDERER_SUBMIT(wgl_renderer_submit);

xiOpenGLContext *gl_os_context_create(void *platform) {
    // @todo: configure renderer
    // @todo: error checking!
    //
    xiArena arena = { 0 };
    xi_arena_init_virtual(&arena, XI_MB(64));

    xiWin32OpenGLContext *wgl = xi_arena_push_type(&arena, xiWin32OpenGLContext);
    xiOpenGLContext *gl = &wgl->gl;

    gl->arena = arena;

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

        HWND hwnd = CreateWindowA(wnd_class.lpszClassName, "xie_wgl", WS_OVERLAPPEDWINDOW,
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

        WGL_LOAD_FUNCTION(wgl, GetExtensionsStringEXT);

        if (wgl->GetExtensionsStringEXT) {
            const char *ext_str = wgl->GetExtensionsStringEXT();
            string extensions   = xi_str_wrap_cstr((u8 *) ext_str);
            string extension    = xi_str_find_from_left(extensions, ' ');

#define WGL_CHECK_EXTENSION(name) if (xi_str_equal(extension, (string) xi_str_wrap_const(#name))) { wgl->name = true; }

            while (xi_str_is_valid(extension)) {
                WGL_CHECK_EXTENSION(WGL_EXT_framebuffer_sRGB)
                else WGL_CHECK_EXTENSION(WGL_ARB_multisample)
                else WGL_CHECK_EXTENSION(WGL_ARB_pixel_format)
                else WGL_CHECK_EXTENSION(WGL_ARB_create_context)
                else WGL_CHECK_EXTENSION(WGL_EXT_swap_control)

                extensions = xi_str_advance(extensions, extension.count + 1);
                extension  = xi_str_find_from_left(extensions, ' ');
            }

#undef WGL_CHECK_EXTENSION
        }

        if (wgl->WGL_ARB_pixel_format)   { WGL_LOAD_FUNCTION(wgl, ChoosePixelFormatARB);    }
        if (wgl->WGL_ARB_create_context) { WGL_LOAD_FUNCTION(wgl, CreateContextAttribsARB); }
        if (wgl->WGL_EXT_swap_control) {
            WGL_LOAD_FUNCTION(wgl, SwapIntervalEXT);
            WGL_LOAD_FUNCTION(wgl, GetSwapIntervalEXT);
        }

        // clean up old context and dummy window
        //
        wglMakeCurrent(0, 0);
        wglDeleteContext(hglrc);

        ReleaseDC(hwnd, hdc);

        DestroyWindow(hwnd);
        UnregisterClassA(wnd_class.lpszClassName, wnd_class.hInstance);
    }

    wgl->hwnd = *(HWND *) platform;
    wgl->hdc  = GetDC(wgl->hwnd);

    if (wgl->WGL_ARB_pixel_format) {
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

        if (wgl->WGL_EXT_framebuffer_sRGB) {
            attribs[num_attribs++] = WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT;
            attribs[num_attribs++] = GL_TRUE;
        }

        XI_ASSERT(num_attribs <= 32);

        // end off the list with GL_NONE
        //
        attribs[num_attribs] = GL_NONE;

        UINT num_formats;
        int pixel_format;

        wgl->ChoosePixelFormatARB(wgl->hdc, attribs, 0, 1, &pixel_format, &num_formats);

        PIXELFORMATDESCRIPTOR pfd;
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);

        DescribePixelFormat(wgl->hdc, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        SetPixelFormat(wgl->hdc, pixel_format, &pfd);
    }

    if (wgl->WGL_ARB_create_context) {
        // create context
        //
        // @todo: debug bit needs to be conditional
        //
        int flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB;

        const int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 4,
            WGL_CONTEXT_FLAGS_ARB,         flags,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            GL_NONE
        };

        wgl->hglrc = wgl->CreateContextAttribsARB(wgl->hdc, 0, attribs);

        if (wgl->hglrc) {
            if (wglMakeCurrent(wgl->hdc, wgl->hglrc)) {
                gl->info.srgb        = wgl->WGL_EXT_framebuffer_sRGB;
                gl->info.multisample = wgl->WGL_ARB_multisample && false; // this will check if it was enabled

                // @todo: load opengl extension functions here
                //
                WGL_LOAD_FUNCTION(gl, GenVertexArrays);
                WGL_LOAD_FUNCTION(gl, GenBuffers);
                WGL_LOAD_FUNCTION(gl, BindBuffer);
                WGL_LOAD_FUNCTION(gl, BufferStorage);
                WGL_LOAD_FUNCTION(gl, MapBufferRange);
                WGL_LOAD_FUNCTION(gl, GenProgramPipelines);
                WGL_LOAD_FUNCTION(gl, CreateShader);
                WGL_LOAD_FUNCTION(gl, ShaderSource);
                WGL_LOAD_FUNCTION(gl, CompileShader);
                WGL_LOAD_FUNCTION(gl, GetShaderiv);
                WGL_LOAD_FUNCTION(gl, CreateProgram);
                WGL_LOAD_FUNCTION(gl, ProgramParameteri);
                WGL_LOAD_FUNCTION(gl, AttachShader);
                WGL_LOAD_FUNCTION(gl, LinkProgram);
                WGL_LOAD_FUNCTION(gl, DetachShader);
                WGL_LOAD_FUNCTION(gl, GetProgramiv);
                WGL_LOAD_FUNCTION(gl, GetProgramInfoLog);
                WGL_LOAD_FUNCTION(gl, DeleteProgram);
                WGL_LOAD_FUNCTION(gl, GetShaderInfoLog);
                WGL_LOAD_FUNCTION(gl, DeleteShader);

                gl->renderer.init   = xi_opengl_init;
                gl->renderer.submit = wgl_renderer_submit;

                wgl->valid = true;
            }
        }
    }

    if (wgl->WGL_EXT_swap_control) { wgl->SwapIntervalEXT(1); }

    if (!wgl->valid) {
        xi_arena_deinit(&arena);
        wgl = 0;
        gl  = 0;
    }

    return gl;
}

void gl_os_context_delete(xiOpenGLContext *gl) {
    xiWin32OpenGLContext *wgl = (xiWin32OpenGLContext *) gl;

    wglMakeCurrent(0, 0);
    wglDeleteContext(wgl->hglrc);

    ReleaseDC(wgl->hwnd, wgl->hdc);

    xiArena arena = gl->arena;
    xi_arena_deinit(&arena);
}

XI_RENDERER_SUBMIT(wgl_renderer_submit) {
    xiWin32OpenGLContext *wgl = (xiWin32OpenGLContext *) renderer;

    XI_ASSERT((wgl != 0) && wgl->valid);

    if (wgl->WGL_EXT_swap_control) {
        wgl->SwapIntervalEXT(renderer->setup.vsync ? 1 : 0);
    }

    //xi_opengl_renderer_submit((xiOpenGLContext *) renderer);

    SwapBuffers(wgl->hdc);
}
