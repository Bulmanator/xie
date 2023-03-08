#if !defined(XI_OPENGL_H_)
#define XI_OPENGL_H_

#include <xi/xi.h>

#if XI_OS_WIN32
    #define WIN32_LEAN_AND_MEAN 1
    #include <windows.h>

    #include <gl/gl.h>
#endif

// any defines from glcorearb.h we need
//
#define GL_MAJOR_VERSION    0x821B
#define GL_MINOR_VERSION    0x821C
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#define GL_SRGB8_ALPHA8     0x8C43

// any function typedefs for extension functions to load dynamically
//
typedef void xiOpenGL_glGenVertexArrays(GLsizei, GLuint *);

#define GL_FUNCTION_POINTER(name) xiOpenGL_gl##name *name

typedef struct xiOpenGLContext {
    xiRenderer renderer;

    struct {
        b32 srgb;
        b32 multisample;

        GLint major_version;
        GLint minor_version;
    } info;

    xiArena arena;

    GLenum texture_format;

    GLuint vao;
    GLuint quad_buffer;

    GL_FUNCTION_POINTER(GenVertexArrays);
} xiOpenGLContext;

#undef GL_FUNCTION_POINTER

// os specific calls
//
static xiOpenGLContext *xi_os_opengl_context_create(void *platform);

#endif  // XI_OPENGL_H_
