#if !defined(XI_ASSETS_H_)
#define XI_ASSETS_H_

enum xiAssetState {
    XI_ASSET_STATE_UNLOADED = 0,
    XI_ASSET_STATE_LOADING,
    XI_ASSET_STATE_LOADED
};

typedef struct xiImageHandle {
   xi_u32 value;
} xiImageHandle;

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
} xiAsset;

typedef struct xiAssetHash {
    xi_u32 index;
    xi_u32 hash;

    xi_string value;
} xiAssetHash;

typedef struct xiAssetManager {
    xiArena *arena;
    xiThreadPool *thread_pool;

    xi_u32 file_count;
    xiAssetFile *files;

    XI_ATOMIC_VAR xi_u32 asset_count;

    xi_u32 max_assets;
    xiAsset *assets;
    xiaAssetInfo *xias;

    // hash table for lookup
    //
    xi_u32 lookup_table_count;
    xiAssetHash *lookup_table;

    // this value is controlled by the texture array dimension set for the renderer, it can be controlled
    // there. any value set here directly will be overwritten
    //
    xi_u32 sprite_dimension;
    xi_f32 animation_dt; // in seconds

    struct {
        xi_b32 enabled;
        xi_string search_dir;
        xi_string sprite_prefix;

        xi_buffer strings;
    } importer;
} xiAssetManager;

#endif  // XI_ASSETS_H_
