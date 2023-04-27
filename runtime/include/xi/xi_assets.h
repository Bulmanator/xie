#if !defined(XI_ASSETS_H_)
#define XI_ASSETS_H_

typedef struct xiRendererTransferTask xiRendererTransferTask;
typedef struct xiRendererTransferQueue xiRendererTransferQueue;

enum xiAssetState {
    XI_ASSET_STATE_UNLOADED = 0,
    XI_ASSET_STATE_PENDING,
    XI_ASSET_STATE_LOADED
};

typedef struct xiImageHandle {
   xi_u32 value;
} xiImageHandle;

typedef struct xiSoundHandle {
   xi_u32 value;
} xiSoundHandle;

typedef struct xiAssetFile {
    xi_b32 modified; // whether we need to rewrite the asset info table and string table

    xiFileHandle handle;
    xiaHeader    header;

    // these are out here so we can mark them as atomic variables to be
    // updated, as the xiaHeader has strict packing requirements we don't want
    // to mess with that in there. these are copied into the header when writing out the
    // file
    //
    volatile xi_uptr next_write_offset;
    volatile xi_u32  asset_count;

    xi_u32 base_asset;
    volatile xi_buffer strings; // the string table, we need this incase we need to rewrite the file after
                                // a contained asset is imported again due to change, or we append to the file
                                // as a new asset is found
                                //
                                // it is marked volatile for the import pass as new asset names are appended
                                // to it in a multi-threaded fashion
} xiAssetFile;

typedef struct xiAsset {
    xi_u32 state;

    xi_u32 type;
    xi_u32 index;
    xi_u32 file;

    union {
        xiRendererTexture texture;
        xi_uptr sample_base;
    } data;
} xiAsset;

typedef struct xiAssetHash {
    xi_u32 index;
    xi_u32 hash;

    xi_string value;
} xiAssetHash;

typedef struct xiAssetLoadInfo {
    xiAssetManager *assets;
    xiAssetFile *file;
    xiAsset *asset;

    xi_uptr offset;
    xi_uptr size;
    void *data;

    xiRendererTransferTask *transfer_task;
} xiAssetLoadInfo;

typedef struct xiAssetManager {
    xiArena *arena;
    xiThreadPool *thread_pool;
    xiRendererTransferQueue *transfer_queue;

    xi_u32 file_count;
    xiAssetFile *files;

    volatile xi_u32 asset_count;

    xi_u32 max_assets;
    xiAsset *assets;
    xiaAssetInfo *xias;

    // these have to be persistent as we don't wait for loading from file to be complete on each frame
    // as that would kinda defeat the point of them being threaded, thus we can't create these from our
    // temp arena as the temp arena may be cleared after a frame before these load structures have
    // finished processing, there are 'max_asset' count of them in the array
    //
    // @todo: this should just be a circular buffer
    //
    xi_u32 next_load;
    volatile xi_u32 total_loads;
    xiAssetLoadInfo *asset_loads;

    // hash table for lookup
    //
    xi_u32 lookup_table_count;
    xiAssetHash *lookup_table;

    // texture handles are acquired from this
    //
    // @incomplete: this needs a big overhaul, we should be requesting handles directly from the renderer
    // this way _it_ can control how resources are allocated and evicted (as this varies wildly depending
    // on graphics backend) and then we request the data attached to said handle, if the data is some
    // invalid value we request to reload the asset
    //
    // :renderer_handles
    //
    xi_u16 next_sprite;
    xi_u16 next_texture;

    xi_u32 max_sprite_handles;
    xi_u32 max_texture_handles;

    // this value is controlled by the texture array dimension set for the renderer, it can be controlled
    // there. any value set here directly will be overwritten
    //
    xi_u32 sprite_dimension;
    xi_f32 animation_dt; // in seconds

    xi_u32 sample_rate;
    xi_buffer sample_buffer;

    struct {
        xi_b32 enabled;
        xi_string search_dir;
        xi_string sprite_prefix;

        xi_buffer strings;
    } importer;
} xiAssetManager;

enum xiAnimationFlags {
    XI_ANIMATION_FLAG_REVERSE   = (1 << 0), // play in reverse
    XI_ANIMATION_FLAG_PING_PONG = (1 << 1), // play forward then backward etc.
    XI_ANIMATION_FLAG_ONE_SHOT  = (1 << 2), // play once
    XI_ANIMATION_FLAG_PAUSED    = (1 << 3), // don't update frame even when update is called
};

// :note by default animations will loop forever and will play forward, the flags above can be combined
// to alter this behaviour.
//

typedef struct xiAnimation {
    xiImageHandle base_frame;

    xi_u32 flags;

    xi_f32 frame_accum;
    xi_f32 frame_dt;

    xi_u32 current_frame;
    xi_u32 frame_count;
} xiAnimation;

// image handling functions
//
extern XI_API xiImageHandle xi_image_get_by_name_str(xiAssetManager *assets, xi_string name);
extern XI_API xiImageHandle xi_image_get_by_name(xiAssetManager *assets, const char *name);

extern XI_API xiaImageInfo *xi_image_info_get(xiAssetManager *assets, xiImageHandle image);
extern XI_API xiRendererTexture xi_image_data_get(xiAssetManager *assets, xiImageHandle image);

// animation handling functions
//
extern XI_API xiAnimation xi_animation_get_by_name_str_flags(xiAssetManager *assets,
        xi_string name, xi_u32 flags);

extern XI_API xiAnimation xi_animation_get_by_name_flags(xiAssetManager *assets,
        const char *name, xi_u32 flags);

extern XI_API xiAnimation xi_animation_create_from_image_flags(xiAssetManager *assets,
        xiImageHandle image, xi_u32 flags);

extern XI_API xiAnimation xi_animation_get_by_name_str(xiAssetManager *assets, xi_string name);
extern XI_API xiAnimation xi_animation_get_by_name(xiAssetManager *assets, const char *name);
extern XI_API xiAnimation xi_animation_create_from_image(xiAssetManager *assets, xiImageHandle image);

// this will return true if the animation reached the end, for looping animations this will happen every
// time it loops. for ping-pong animations this will happen once for each full playback of the animation
// in either direction, thus for a full "ping-pong" it will return 'true' twice otherwise for one-shot
// animations this will happen a single time
//
// if the animation is paused, with the XI_ANIMATION_FLAG_PAUSED flag, this function will never return true
//
extern XI_API xi_b32 xi_animation_update(xiAnimation *animation, xi_f32 dt);

// get, as a percentage, the amount of the animation that has been played, for looping animations this
// resets every loop, for ping-pong animations tracks only 'ping' or 'pong' percentage
//
// can be used to execute events at certain times throughout the animation
//
extern XI_API xi_f32 xi_animation_playback_percentage_get(xiAnimation *animation);

// reset an animation, this can be used to restart one-shot animations that have finished or simply restart
// animations in the middle of their playback
//
extern XI_API void xi_animation_reset(xiAnimation *animation);

extern XI_API void xi_animation_pause(xiAnimation *animation);
extern XI_API void xi_animation_unpause(xiAnimation *animation);

// returns the image handle associated with the current frame of the animation
//
extern XI_API xiImageHandle xi_animation_current_frame_get(xiAnimation *animation);

// sound handling functions
//
extern XI_API xiSoundHandle xi_sound_get_by_name_str(xiAssetManager *assets, xi_string name);
extern XI_API xiSoundHandle xi_sound_get_by_name(xiAssetManager *assets, const char *name);

extern XI_API xiaSoundInfo *xi_sound_info_get(xiAssetManager *assets, xiSoundHandle sound);
extern XI_API xi_s16 *xi_sound_data_get(xiAssetManager *assets, xiSoundHandle sound);

#endif  // XI_ASSETS_H_
