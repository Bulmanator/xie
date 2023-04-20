#if !defined(XI_RENDERER_H_)
#define XI_RENDERER_H_

typedef struct xiAssetManager xiAssetManager;
typedef struct xiRenderer xiRenderer;

#define XI_RENDERER_INIT(name) xi_b32 name(xiRenderer *renderer, void *platform)
#define XI_RENDERER_SUBMIT(name) void name(xiRenderer *renderer)

typedef XI_RENDERER_INIT(xiRendererInit);
typedef XI_RENDERER_SUBMIT(xiRendererSubmit);

#if !defined(XI_RENDERER_BACKEND)
    typedef void xiRendererBackend;
#endif

enum xiRendererTransferTaskState {
    XI_RENDERER_TRANSFER_TASK_STATE_UNUSED = 0,
    XI_RENDERER_TRANSFER_TASK_STATE_PENDING,
    XI_RENDERER_TRANSFER_TASK_STATE_LOADED
};

typedef union xiRendererTexture {
    struct {
        xi_u16 index;
        xi_u16 generation;
        xi_u16 width;
        xi_u16 height;
    };

    xi_u64 value;
} xiRendererTexture;

typedef struct xiRendererTransferTask {
    volatile xi_u32 state;

    xiRendererTexture texture;

    xi_uptr offset;
    xi_uptr size;
} xiRendererTransferTask;

typedef struct xiRendererTransferQueue {
    xi_u8  *base;
    xi_uptr limit;
    xi_uptr write_offset;
    xi_uptr read_offset;

    xi_u32 first_task;
    xi_u32 task_count;

    xi_u32 max_tasks;
    xiRendererTransferTask *tasks;
} xiRendererTransferQueue;

enum xiRenderCommandType {
    XI_RENDER_COMMAND_INVALID = 0,
    XI_RENDER_COMMAND_xiRenderCommandDraw
};

typedef struct xiRenderCommandDraw {
    xi_uptr ubo_offset; // offset into ubo for globals

    xi_u32 vertex_offset;
    xi_u32 vertex_count;

    xi_u32 index_offset;
    xi_u32 index_count;
} xiRenderCommandDraw;

enum xiCameraTransformFlags {
    XI_CAMERA_TRANSFORM_FLAG_ORTHOGRAPHIC = (1 << 0), // default is perspecitve
};

typedef struct xiCameraTransform {
    xi_v3 x_axis;
    xi_v3 y_axis;
    xi_v3 z_axis;

    xi_v3 position;

    xi_uptr ubo_offset;
    xi_m4x4_inv transform;
} xiCameraTransform;

// @todo: this might need to be aligned internally
//
typedef struct xiShaderGlobals {
    xi_m4x4 transform;
    xi_v4   camera_position;
    xi_f32  time;
    xi_f32  dt;
    xi_f32  unused0;
    xi_f32  unused1;
} xiShaderGlobals;

typedef struct xiRenderer {
    xiRendererBackend *backend;
    xiRendererInit    *init;
    xiRendererSubmit  *submit;

    xiAssetManager *assets;

    struct {
        xi_b32 vsync;
        xi_v2u window_dim;
    } setup;

    struct {
        xi_u32 dimension; // the square dimension of each texture slot
        xi_u32 limit;     // how many slots in the array
    } sprite_array;

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
    xiCameraTransform camera;

    xiRendererTransferQueue transfer_queue;

    xi_buffer command_buffer;
    xiRenderCommandDraw *draw_call;
} xiRenderer;

// misc utils
//
extern XI_API xi_b32 xi_renderer_texture_is_sprite(xiRenderer *renderer, xiRendererTexture texture);

// transfer queue
//
extern XI_API xiRendererTransferTask *xi_renderer_transfer_queue_enqueue_size(xiRendererTransferQueue *queue,
        void **data, xi_uptr size);

// camera transform
//
extern XI_API void xi_camera_transform_set(xiRenderer *renderer,
        xi_v3 x_axis, xi_v3 y_axis, xi_v3 z_axis, xi_v3 position,
        xi_u32 flags, xi_f32 near_plane, xi_f32 far_plane, xi_f32 focal_length);

// like above but will use a default near plane of 0.001f, a default far plane of 10000.0f and a focal
// length matching roughly 65 degree fov
//
extern XI_API void xi_camera_transform_set_axes(xiRenderer *renderer,
        xi_v3 x_axis, xi_v3 y_axis, xi_v3 z_axis, xi_v3 position, xi_u32 flags);

#endif  // XI_RENDERER_H_
