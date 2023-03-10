#if !defined(XI_RENDERER_H_)
#define XI_RENDERER_H_

// @todo: add configuration parameters
//

typedef struct xiRenderer xiRenderer;

#define XI_RENDERER_INIT(name) xiRenderer *name(void *platform)
#define XI_RENDERER_SUBMIT(name) void name(xiRenderer *renderer)

typedef XI_RENDERER_INIT(xiRendererInit);
typedef XI_RENDERER_SUBMIT(xiRendererSubmit);

typedef struct xiRenderer {
    xiRendererInit   *init;
    xiRendererSubmit *submit;

    struct {
        b32 vsync;
        v2u window_dim;
    } setup;

    struct {
        vert3 *base;
        u32    count;
        u32    limit;
    } vertices;

    struct {
        u16 *base;
        u32  count;
        u32  limit;
    } indices;

    xiArena uniforms;
    buffer  commands;
} xiRenderer;

#endif  // XI_RENDERER_H_
