#if !defined(XI_RENDERER_H_)
#define XI_RENDERER_H_

// @todo: add configuration parameters
//

typedef struct xiRenderer xiRenderer;

#define XI_RENDERER_INIT(name) xiRenderer *name(void *platform)
#define XI_RENDERER_SUBMIT(name) void name(xiRenderer *renderer)

typedef XI_RENDERER_INIT(xiRendererInit);
typedef XI_RENDERER_SUBMIT(xiRendererSubmit);

enum xiRenderCommandType {
    XI_RENDER_COMMAND_INVALID = 0,
    XI_RENDER_COMMAND_xiRenderCommandDraw
};

typedef struct xiRenderCommandDraw {
    u32 vertex_offset;
    u32 vertex_count;

    u32 index_offset;
    u32 index_count;
} xiRenderCommandDraw;

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

    xiRenderCommandDraw *draw_call;
} xiRenderer;

// 2d textureless quads
//
extern XI_API void xi_quad_draw_xy(xiRenderer *renderer, v4 colour,
        v2 center, v2 dimension, f32 angle);

#endif  // XI_RENDERER_H_
