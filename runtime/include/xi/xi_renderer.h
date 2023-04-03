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
    xi_u32 vertex_offset;
    xi_u32 vertex_count;

    xi_u32 index_offset;
    xi_u32 index_count;
} xiRenderCommandDraw;

typedef struct xiRenderer {
    xiRendererInit   *init;
    xiRendererSubmit *submit;

    struct {
        xi_b32 vsync;
        xi_v2u window_dim;
    } setup;

    struct {
        xi_vert3 *base;
        xi_u32    count;
        xi_u32    limit;
    } vertices;

    struct {
        xi_u16 *base;
        xi_u32  count;
        xi_u32  limit;
    } indices;

    xiArena uniforms;
    xi_buffer commands;

    xiRenderCommandDraw *draw_call;
} xiRenderer;

// 2d textureless quads
//
extern XI_API void xi_quad_draw_xy(xiRenderer *renderer, xi_v4 colour,
        xi_v2 center, xi_v2 dimension, xi_f32 angle);

#endif  // XI_RENDERER_H_
