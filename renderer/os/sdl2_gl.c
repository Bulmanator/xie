#include <SDL2/SDL_video.h>
#include <SDL2/SDL_syswm.h>

#define SDL2_LOAD_GL_FUNCTION(x) gl->x = (GL_gl##x *) SDL->GL_GetProcAddress(Stringify(gl##x)); Assert(gl->x != 0)

typedef SDL_GLContext SDL2_SDL_GL_CreateContext(SDL_Window *window);

typedef void *SDL2_SDL_GL_GetProcAddress(const char *);

typedef int  SDL2_SDL_GL_SetSwapInterval(int);
typedef int  SDL2_SDL_GL_SetAttribute(SDL_GLattr, int);
typedef void SDL2_SDL_GL_SwapWindow(SDL_Window *);
typedef void SDL2_SDL_GL_DeleteContext(SDL_GLContext);

typedef SDL_bool SDL2_SDL_GetWindowWMInfo(SDL_Window *, SDL_SysWMinfo *);

#define SDL2_FUNCTION_POINTER(name) SDL2_SDL_##name *name

// :renderer_core passed as the platform pointer to init
//
typedef struct SDL2_WindowData SDL2_WindowData;
struct SDL2_WindowData {
    SDL_Window *window;

    SDL2_FUNCTION_POINTER(GL_CreateContext);
    SDL2_FUNCTION_POINTER(GL_GetProcAddress);
    SDL2_FUNCTION_POINTER(GL_SetSwapInterval);
    SDL2_FUNCTION_POINTER(GL_SetAttribute);
    SDL2_FUNCTION_POINTER(GL_SwapWindow);
    SDL2_FUNCTION_POINTER(GL_DeleteContext);
    SDL2_FUNCTION_POINTER(GetWindowWMInfo);
};

// :naming very close to the actual SDL naming could be confusing
//
typedef struct SDL2GL_Context SDL2GL_Context;
struct SDL2GL_Context {
    GL_Context gl;

    B32 valid;

    SDL_Window *window;
    SDL_GLContext context;

    SDL2_FUNCTION_POINTER(GL_SetSwapInterval);
    SDL2_FUNCTION_POINTER(GL_SwapWindow);
    SDL2_FUNCTION_POINTER(GL_DeleteContext);
};

#undef SDL2_FUNCTION_POINTER

FileScope RENDERER_SUBMIT(SDL2GL_Submit);

FileScope GL_Context *GL_ContextCreate(RendererContext *renderer, void *platform) {
    GL_Context *result = 0;

    M_Arena *arena = M_ArenaAlloc(GB(2), 0);

    SDL2GL_Context *context = M_ArenaPush(arena, SDL2GL_Context);
    GL_Context     *gl      = &context->gl;

    result = gl;

    gl->arena = arena;

    SDL2_WindowData *SDL = cast(SDL2_WindowData *) platform;

    SDL->GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL->GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
    SDL->GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
    SDL->GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,         SDL_GL_CONTEXT_DEBUG_FLAG);

    // @todo: test this on nvidia, iirc setting framebuffer bit sizes explicitly
    // has issues on those drivers
    //
    SDL->GL_SetAttribute(SDL_GL_RED_SIZE,      8);
    SDL->GL_SetAttribute(SDL_GL_GREEN_SIZE,    8);
    SDL->GL_SetAttribute(SDL_GL_BLUE_SIZE,     8);
    SDL->GL_SetAttribute(SDL_GL_ALPHA_SIZE,    8);
    SDL->GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);
    SDL->GL_SetAttribute(SDL_GL_STENCIL_SIZE,  8);

    SDL->GL_SetAttribute(SDL_GL_DOUBLEBUFFER,             1);
    SDL->GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL->GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,       1);

    context->context = SDL->GL_CreateContext(SDL->window);
    if (context->context != 0) {
        gl->info.srgb        = true;  // @todo: actually check
        gl->info.multisample = false; // this shouldn't be forced off

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

        renderer->Init   = GL_Init;
        renderer->Submit = SDL2GL_Submit;

        context->window             = SDL->window;
        context->GL_SetSwapInterval = SDL->GL_SetSwapInterval;
        context->GL_SwapWindow      = SDL->GL_SwapWindow;
        context->GL_DeleteContext   = SDL->GL_DeleteContext;

        context->valid = true;
    }

    if (renderer->setup.vsync) { SDL->GL_SetSwapInterval(1); }

    if (!context->valid) {
        M_ArenaRelease(arena);

        context = 0;
        gl      = 0;
    }

    return result;
}

FileScope void GL_ContextDelete(GL_Context *gl) {
    SDL2GL_Context *SDL = cast(SDL2GL_Context *) gl;

    SDL->GL_DeleteContext(SDL->context);
    M_ArenaRelease(gl->arena);
}

RENDERER_SUBMIT(SDL2GL_Submit) {
    SDL2GL_Context *SDL = cast(SDL2GL_Context *) renderer->backend;

    SDL->GL_SetSwapInterval(renderer->setup.vsync ? 1 : 0);

    GL_Submit(renderer);

    SDL->GL_SwapWindow(SDL->window);
}
