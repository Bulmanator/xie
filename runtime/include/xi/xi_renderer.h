#if !defined(XI_RENDERER_H_)
#define XI_RENDERER_H_

typedef struct AssetManager AssetManager;
typedef struct RendererContext RendererContext;

#define RENDERER_INIT(name) B32 name(RendererContext *renderer, void *platform)
#define RENDERER_SUBMIT(name) void name(RendererContext *renderer)

typedef RENDERER_INIT(RendererInitProc);
typedef RENDERER_SUBMIT(RendererSubmitProc);

#if !defined(RENDERER_BACKEND)
    typedef void RendererBackend;
#endif

typedef U32 RendererTransferTaskState;
enum RendererTransferTaskState {
    RENDERER_TRANSFER_TASK_STATE_UNUSED = 0,
    RENDERER_TRANSFER_TASK_STATE_PENDING,
    RENDERER_TRANSFER_TASK_STATE_LOADED
};

typedef union RendererTexture RendererTexture;
union RendererTexture {
    struct {
        U32 index;
        U16 width;
        U16 height;
    };

    U64 value;
};

typedef struct RendererTransferTask RendererTransferTask;
struct RendererTransferTask {
    volatile RendererTransferTaskState state;

    RendererTexture texture;

    U64 offset;
    U64 size;
};

typedef struct RendererTransferQueue RendererTransferQueue;
struct RendererTransferQueue {
    U8 *base;
    U64 limit;
    U64 write_offset;
    U64 read_offset;

    U32 first_task;
    U32 task_count;

    U32 task_limit;
    RendererTransferTask *tasks;
};

typedef U32 RenderCommandType;
enum RenderCommandType {
    RENDER_COMMAND_INVALID = 0,
    RENDER_COMMAND_RenderCommandDraw
};

typedef struct RenderCommandDraw RenderCommandDraw;
struct RenderCommandDraw {
    RendererTexture texture;

    U64 ubo_offset; // offset into ubo for globals

    U32 vertex_offset;
    U32 vertex_count;

    U32 index_offset;
    U32 index_count;
};

typedef U32 CameraTransformFlags;
enum CameraTransformFlags {
    CAMERA_TRANSFORM_FLAG_ORTHOGRAPHIC = (1 << 0), // default is perspecitve
};

typedef struct CameraTransform CameraTransform;
struct CameraTransform {
    Vec3F x;
    Vec3F y;
    Vec3F z;

    Vec3F p;

    U64 ubo_offset;
    Mat4x4FInv transform;
};

// @todo: this might need to be aligned internally
//
typedef struct ShaderGlobals ShaderGlobals;
struct ShaderGlobals {
    Mat4x4F transform;
    Vec4F   camera_position;
    F32     time;
    F32     dt;
    Vec2U   window_dim;
};

// @todo: this whole rendering system need overhauling
//

typedef struct RendererContext RendererContext;
struct RendererContext {
    RendererBackend    *backend;
    RendererInitProc   *Init;
    RendererSubmitProc *Submit;

    AssetManager *assets;

    struct {
        B32   vsync;
        Vec2U window_dim;
    } setup;

    struct {
        U32 dimension; // the square dimension of each texture slot
        U32 limit;     // how many slots in the array
    } sprite_array;

    // number of non-sprite textures to allow
    // :name this is confusing, should be named something better
    //
    U32 texture_limit;

    struct {
        Vertex3 *base;
        U32      count;
        U32      limit;
    } vertices;

    struct {
        U16 *base;
        U32  count;
        U32  limit;
    } indices;

    S64 ubo_globals_size; // sizeof(ShaderGlobals) but aligned accordingly
    Buffer uniforms;

    CameraTransform camera;

    RendererTransferQueue transfer_queue;

    F32 layer_offset; // each pushed layer is offset by this amount, default is 1.0f
    F32 layer; // current z layer, can be reset by assigning to zero, reset automatically each frame

    Buffer command_buffer;
    RenderCommandDraw *draw_call;
};

// misc utils
//
Func B32 RendererTextureIsSprite(RendererContext *renderer, RendererTexture texture);

// transfer queue
//
Func RendererTransferTask *RendererTransferQueueEnqueueSize(RendererTransferQueue *queue, void **data, U64 size);

// camera transform
//
Func void CameraTransformSet(RendererContext *renderer, Vec3F x, Vec3F y, Vec3F z, Vec3F p, U32 flags, F32 nearp, F32 farp, F32 fov);
Func void CameraTransformSetFromAxes(RendererContext *renderer, Vec3F x, Vec3F y, Vec3F z, Vec3F p, U32 flags);

Func void CameraTransformGet(CameraTransform *camera, F32 aspect, Vec3F x, Vec3F y, Vec3F z, Vec3F p, U32 flags, F32 nearp, F32 farp, F32 fov);
Func void CameraTransformGetFromAxes(CameraTransform *camera, F32 aspect, Vec3F x, Vec3F y, Vec3F z, Vec3F p, U32 flags);

Func Vec3F V2FUnproject(CameraTransform *camera, Vec2F clip);
Func Vec3F V2FUnprojectAt(CameraTransform *camera, Vec2F clip, F32 z);

Func Rect3F CameraBoundsGet(CameraTransform *camera);
Func Rect3F CameraBoundsGetAt(CameraTransform *camera, F32 z);

// pushes a new layer, this increments the z offset by the amount specified by 'layer_offset'
// all sprites/quads will then be drawn at this z offset until another layer is pushed
//
// @todo: remove this whole system, just use MSAA with alpha to coverage, for what we're doing its
// probably performant enough these days and has a much simpler interface
//
Func void RendererLayerPush(RendererContext *renderer);

#endif  // XI_RENDERER_H_
