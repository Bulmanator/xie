#include "xi_opengl.h"

extern XI_EXPORT XI_RENDERER_INIT(xi_opengl_init) {
    xiOpenGLContext *gl = xi_os_opengl_context_create(platform);
    if (gl) {
        // we were able to create an os specific opengl context, so continue with the os non-specific
        // initialisation
        //
        if (gl->info.srgb) {
            gl->texture_format = GL_SRGB8_ALPHA8;
            glEnable(GL_FRAMEBUFFER_SRGB);
        }
        else {
            gl->texture_format = GL_RGBA8;
        }
    }

    xiRenderer *result = (xiRenderer *) gl;
    return result;
}

#if XI_OS_WIN32
    #include "os/wgl.c"
#endif
