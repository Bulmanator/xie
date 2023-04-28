#include <SDL2/SDL_video.h>

#define SDL2_LOAD_GL_FUNCTION(x) gl->x = (xiOpenGL_gl##x *) SDL->GL_GetProcAddress(XI_STRINGIFY(gl##x)); XI_ASSERT(gl->x != 0)

typedef SDL_GLContext xiSDL2_SDL_GL_CreateContext(SDL_Window *window);

typedef void *xiSDL2_SDL_GL_GetProcAddress(const char *);

typedef int  xiSDL2_SDL_GL_SetSwapInterval(int);
typedef int  xiSDL2_SDL_GL_SetAttribute(SDL_GLattr, int);
typedef void xiSDL2_SDL_GL_SwapWindow(SDL_Window *);
typedef void xiSDL2_SDL_GL_DeleteContext(SDL_GLContext);

#define SDL2_FUNCTION_POINTER(name) xiSDL2_SDL_##name *name

// :renderer_core passed as the platform pointer to init
//
typedef struct xiSDL2WindowData {
    SDL_Window *window;

    SDL2_FUNCTION_POINTER(GL_CreateContext);
    SDL2_FUNCTION_POINTER(GL_GetProcAddress);
    SDL2_FUNCTION_POINTER(GL_SetSwapInterval);
    SDL2_FUNCTION_POINTER(GL_SetAttribute);
    SDL2_FUNCTION_POINTER(GL_SwapWindow);
    SDL2_FUNCTION_POINTER(GL_DeleteContext);
} xiSDL2WindowData;

typedef struct xiSDL2OpenGLContext {
    xiOpenGLContext gl;

    xi_b32 valid;

    SDL_Window *window;
    SDL_GLContext context;

    SDL2_FUNCTION_POINTER(GL_SetSwapInterval);
    SDL2_FUNCTION_POINTER(GL_SwapWindow);
    SDL2_FUNCTION_POINTER(GL_DeleteContext);
} xiSDL2OpenGLContext;

#undef SDL2_FUNCTION_POINTER

XI_INTERNAL XI_RENDERER_SUBMIT(sdl2_opengl_submit);

static xiOpenGLContext *gl_os_context_create(xiRenderer *renderer, void *platform) {
    xiArena arena = { 0 };
    xi_arena_init_virtual(&arena, XI_GB(2));

    xiSDL2OpenGLContext *sdlgl = xi_arena_push_type(&arena, xiSDL2OpenGLContext);
    xiOpenGLContext *gl = &sdlgl->gl;

    gl->arena = arena;

    xiSDL2WindowData *window = (xiSDL2WindowData *) platform;
    xiSDL2WindowData *SDL = window;

    SDL->GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL->GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
    SDL->GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
    SDL->GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    // @todo: test this on nvidia, iirc setting framebuffer bit sizes explicitly
    // has issues on those drivers
    //
    SDL->GL_SetAttribute(SDL_GL_RED_SIZE,      8);
    SDL->GL_SetAttribute(SDL_GL_GREEN_SIZE,    8);
    SDL->GL_SetAttribute(SDL_GL_BLUE_SIZE,     8);
    SDL->GL_SetAttribute(SDL_GL_ALPHA_SIZE,    8);
    SDL->GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);
    SDL->GL_SetAttribute(SDL_GL_STENCIL_SIZE,  8);

    SDL->GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL->GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL->GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    sdlgl->context = SDL->GL_CreateContext(window->window);
    if (sdlgl->context != 0) {
        gl->info.srgb = true; // @todo: actually check
        gl->info.multisample = false;

        // load opengl extension functions
        //
        SDL2_LOAD_GL_FUNCTION(GenVertexArrays);
        SDL2_LOAD_GL_FUNCTION(GenBuffers);
        SDL2_LOAD_GL_FUNCTION(BindBuffer);
        SDL2_LOAD_GL_FUNCTION(BufferStorage);
        SDL2_LOAD_GL_FUNCTION(MapBufferRange);
        SDL2_LOAD_GL_FUNCTION(GenProgramPipelines);
        SDL2_LOAD_GL_FUNCTION(CreateShader);
        SDL2_LOAD_GL_FUNCTION(ShaderSource);
        SDL2_LOAD_GL_FUNCTION(CompileShader);
        SDL2_LOAD_GL_FUNCTION(GetShaderiv);
        SDL2_LOAD_GL_FUNCTION(CreateProgram);
        SDL2_LOAD_GL_FUNCTION(ProgramParameteri);
        SDL2_LOAD_GL_FUNCTION(AttachShader);
        SDL2_LOAD_GL_FUNCTION(LinkProgram);
        SDL2_LOAD_GL_FUNCTION(DetachShader);
        SDL2_LOAD_GL_FUNCTION(GetProgramiv);
        SDL2_LOAD_GL_FUNCTION(GetProgramInfoLog);
        SDL2_LOAD_GL_FUNCTION(DeleteProgram);
        SDL2_LOAD_GL_FUNCTION(GetShaderInfoLog);
        SDL2_LOAD_GL_FUNCTION(DeleteShader);
        SDL2_LOAD_GL_FUNCTION(BindVertexArray);
        SDL2_LOAD_GL_FUNCTION(VertexAttribPointer);
        SDL2_LOAD_GL_FUNCTION(EnableVertexAttribArray);
        SDL2_LOAD_GL_FUNCTION(UseProgramStages);
        SDL2_LOAD_GL_FUNCTION(BindProgramPipeline);
        SDL2_LOAD_GL_FUNCTION(FlushMappedBufferRange);
        SDL2_LOAD_GL_FUNCTION(DrawElementsBaseVertex);
        SDL2_LOAD_GL_FUNCTION(TexStorage3D);
        SDL2_LOAD_GL_FUNCTION(TexSubImage3D);
        SDL2_LOAD_GL_FUNCTION(TexImage3D);
        SDL2_LOAD_GL_FUNCTION(DebugMessageCallback);
        SDL2_LOAD_GL_FUNCTION(BufferSubData);
        SDL2_LOAD_GL_FUNCTION(BufferData);
        SDL2_LOAD_GL_FUNCTION(ActiveTexture);
        SDL2_LOAD_GL_FUNCTION(BindBufferRange);

        renderer->init   = xi_opengl_init;
        renderer->submit = sdl2_opengl_submit;

        sdlgl->window = SDL->window;
        sdlgl->GL_SetSwapInterval = SDL->GL_SetSwapInterval;
        sdlgl->GL_SwapWindow      = SDL->GL_SwapWindow;
        sdlgl->GL_DeleteContext   = SDL->GL_DeleteContext;

        sdlgl->valid = true;
    }

    if (renderer->setup.vsync) {
        SDL->GL_SetSwapInterval(1);
    }

    if (!sdlgl->valid) {
        xi_arena_deinit(&arena);
        sdlgl = 0;
        gl    = 0;
    }

    return gl;
}

static void gl_os_context_delete(xiOpenGLContext *gl) {
    xiSDL2OpenGLContext *SDL = (xiSDL2OpenGLContext *) gl;

    SDL->GL_DeleteContext(SDL->context);

    xiArena arena = gl->arena;
    xi_arena_deinit(&arena);
}

XI_RENDERER_SUBMIT(sdl2_opengl_submit) {
    xiSDL2OpenGLContext *SDL = (xiSDL2OpenGLContext *) renderer->backend;

    SDL->GL_SetSwapInterval(renderer->setup.vsync ? 1 : 0);

    xi_opengl_submit(renderer);

    SDL->GL_SwapWindow(SDL->window);
}
