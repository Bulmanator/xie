#if !defined(XI_ASSETS_H_)
#define XI_ASSETS_H_

// @todo: this whole asset system kinda needs overhauling
//

typedef struct RendererTransferTask  RendererTransferTask;
typedef struct RendererTransferQueue RendererTransferQueue;

typedef U32 AssetState;
enum AssetState {
    ASSET_STATE_UNLOADED = 0,
    ASSET_STATE_PENDING,
    ASSET_STATE_LOADED
};

typedef struct ImageHandle ImageHandle;
struct ImageHandle {
   U32 value;
};

typedef struct SoundHandle SoundHandle;
struct SoundHandle {
   U32 value;
};

typedef struct AssetFile AssetFile;
struct AssetFile {
    B32 modified; // whether we need to rewrite the asset info table and string table

    Xia_Header header;

    FileHandle handle;

    // these are out here so we can mark them as atomic variables to be
    // updated, as the xiaHeader has strict packing requirements we don't want
    // to mess with that in there. these are copied into the header when writing out the
    // file
    //
    volatile U64 next_write_offset;

             U32 base_asset;
    volatile U32 num_assets;

    volatile Buffer strings; // the string table, we need this incase we need to rewrite the file after
                             // a contained asset is imported again due to change, or we append to the file
                             // as a new asset is found
                             //
                             // it is marked volatile for the import pass as new asset names are appended
                             // to it in a multi-threaded fashion
};

typedef struct Asset Asset;
struct Asset {
    volatile AssetState state;

    U32 type;
    U32 index;
    U32 file;

    union {
        RendererTexture texture;
        U64 sample_base;
    } data;
};

typedef struct AssetHash AssetHash;
struct AssetHash {
    U32 index;
    U32 hash;

    Str8 value;
};

typedef struct AssetLoadInfo AssetLoadInfo;
struct AssetLoadInfo {
    AssetManager *assets;

    AssetFile *file;
    Asset     *asset;

    U64 offset;
    U64 size;

    void *data;

    RendererTransferTask *transfer_task;
};

struct AssetManager {
    M_Arena    *arena;
    ThreadPool *thread_pool;

    RendererTransferQueue *transfer_queue;

    U32 num_files;
    AssetFile *files;

    volatile U32 num_assets;

    U32           max_assets;
    Asset         *assets;
    Xia_AssetInfo *xias;

    // these have to be persistent as we don't wait for loading from file to be complete on each frame
    // as that would kinda defeat the point of them being threaded, thus we can't create these from our
    // temp arena as the temp arena may be cleared after a frame before these load structures have
    // finished processing, there are 'max_asset' count of them in the array
    //
    // @todo: this should just be a circular buffer
    //
    U32 next_load;
    volatile U32 total_loads;
    AssetLoadInfo *asset_loads;

    // hash table for lookup
    //
    U32 lookup_table_count;
    AssetHash *lookup_table;

    // texture handles are acquired from this
    //
    // @incomplete: this needs a big overhaul, we should be requesting handles directly from the renderer
    // this way _it_ can control how resources are allocated and evicted (as this varies wildly depending
    // on graphics backend) and then we request the data attached to said handle, if the data is some
    // invalid value we request to reload the asset
    //
    // :renderer_handles
    //
    U16 next_sprite;
    U16 next_texture;

    U32 max_sprite_handles;
    U32 max_texture_handles;

    // this value is controlled by the texture array dimension set for the renderer, it can be controlled
    // there. any value set here directly will be overwritten
    //
    U32 sprite_dimension;
    F32 animation_dt; // in seconds

    U32 sample_rate;
    Buffer sample_buffer;

    struct {
        B32  enabled;
        Str8 search_dir;
        Str8 sprite_prefix;

        Buffer strings;
    } importer;
};

typedef U32 SpriteAnimationFlags;
enum AnimationFlags {
    SPRITE_ANIMATION_FLAG_REVERSE   = (1 << 0), // play in reverse
    SPRITE_ANIMATION_FLAG_PING_PONG = (1 << 1), // play forward then backward etc.
    SPRITE_ANIMATION_FLAG_ONE_SHOT  = (1 << 2), // play once
    SPRITE_ANIMATION_FLAG_PAUSED    = (1 << 3), // don't update frame even when update is called
};

// :note by default animations will loop forever and will play forward, the flags above can be combined
// to alter this behaviour.
//
// @todo: this shouldn't even be here
//

typedef struct SpriteAnimation SpriteAnimation;
struct SpriteAnimation {
    ImageHandle base_frame;

    SpriteAnimationFlags flags;

    F32 frame_accum;
    F32 frame_dt;

    U32 current_frame;
    U32 frame_count;
};

// Asset acquisition functions
//
Func ImageHandle ImageGetByName(AssetManager *assets, Str8 name);
Func SoundHandle SoundGetByName(AssetManager *assets, Str8 name);

Func SpriteAnimation SpriteAnimationGetByName(AssetManager *assets, Str8 name, SpriteAnimationFlags flags);
Func SpriteAnimation SpriteAnimationFromImage(AssetManager *assets, ImageHandle image, SpriteAnimationFlags flags);

// Asset information
//
Func Xia_ImageInfo *ImageInfoGet(AssetManager *assets, ImageHandle image);
Func Xia_SoundInfo *SoundInfoGet(AssetManager *assets, SoundHandle sound);

Func Xia_ImageInfo *SpriteAnimationInfoGet(AssetManager *assets, SpriteAnimation *animation);

// Asset data
//
Func RendererTexture ImageDataGet(AssetManager *assets, ImageHandle image);
Func S16 *           SoundDataGet(AssetManager *assets, SoundHandle sound);

Func ImageHandle SpriteAnimationFrameGet(SpriteAnimation *animation);

// Other misc animation functions
//
// ... again these shouldn't be here as they're not direct assets
//
// This api is kinda icky the engine shouldn't split update/render and animations should just be
// updated and rendererd directly, there shouldn't be the notion of 'pause' or whatever the
// system should just call 'SpriteAnimationDraw' or whatever if they want to update the animation
//
Func B32  SpriteAnimationUpdate(SpriteAnimation *animation, F32 dt);
Func F32  SpriteAnimationPlaybackPercentageGet(SpriteAnimation *animation);
Func void SpriteAnimationReset(SpriteAnimation *animation);
Func void SpriteAnimationPause(SpriteAnimation *animation);
Func void SpriteAnimationResume(SpriteAnimation *animation);

#endif  // XI_ASSETS_H_
