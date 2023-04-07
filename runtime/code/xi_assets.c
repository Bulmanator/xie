// @temp: this should be part of the public facing api!
//
#if XI_COMPILER_CL
    #define XI_ATOMIC_COMPARE_EXCHANGE_U32(target, value, compare) (_InterlockedCompareExchange((volatile long *) target, value, compare) == (long) compare)
    #define XI_ATOMIC_INCREMENT_U32(target) _InterlockedIncrement((volatile long *) target)
#else
    #error "incomplete implementation"
#endif

// forward declaring stuff that might be needed
//
XI_INTERNAL void xi_asset_manager_import_to_xia(xiAssetManager *assets);

// these are some large 32-bit prime numbers which are xor'd with the string hash value allowing us to
// produce a different hash values for the same 'name' of different asset types
//
// these haven't been chosen in any particular way
//
XI_GLOBAL const xi_u32 tbl_xia_type_hash_mix[XIA_ASSET_TYPE_COUNT - 1] = {
    0xcb03f267,
};

// it is important that "name" is in asset manager owned memory as it is stored on the asset hash directly
//
XI_INTERNAL void xi_asset_manager_asset_hash_insert(xiAssetManager *assets,
        xi_u32 type, xi_u32 index, xi_string name)
{
    xi_u32 mask = assets->lookup_table_count - 1;
    xi_u32 mix  = tbl_xia_type_hash_mix[type];
    xi_u32 hash = xi_str_djb2_hash(name) ^ mix;

    xi_u32 i = 0;

    xiAssetHash *slot = 0;
    do {
        if (i > assets->lookup_table_count) {
            slot = 0;
            break;
        }

        // quadratic probing
        //
        xi_u32 table_index = (hash + (i * i)) & mask;
        slot = &assets->lookup_table[table_index];

        i += 1;
    } while (!XI_ATOMIC_COMPARE_EXCHANGE_U32(&slot->index, index, 0));

    // we initialise our lookup table to be 2 * round_pow2(max_assets), which will have a maximum load
    // factor of 50%, so this lookup will realisitically never fail
    //
    XI_ASSERT(slot != 0);

    slot->hash  = hash;
    slot->index = index; // overall asset index
    slot->value = name;  // its safe to store this string here because we have to hold if for the files
}

XI_INTERNAL void xi_asset_manager_init(xiArena *arena, xiAssetManager *assets, xiContext *xi) {
    assets->arena = arena;
    assets->thread_pool = &xi->thread_pool;

    xiArena *temp  = xi_temp_get();
    xi_string path = xi_str_format(temp, "%.*s/data", xi_str_unpack(xi->system.executable_path));

    if (!xi_os_directory_create(path)) { return; }

    // @hardcode: renderer not setup properly yet!
    //
    assets->sprite_dimension = 256;

    xiDirectoryList data_dir, xia_files;
    xi_directory_list_get(temp, &data_dir, path, false);
    xi_directory_list_filter_for_extension(temp, &xia_files, &data_dir, xi_str_wrap_const(".xia"));

    xi_u32 file_access = assets->importer.enabled ? XI_FILE_ACCESS_FLAG_READWRITE : XI_FILE_ACCESS_FLAG_READ;

    // the first two assets are reserved for error checking and a white texture used in place when
    // textureless quads are drawn, the 0 index will be a magenta/black checkerboard texture,
    // the 1 index will be said white texture
    //
    assets->asset_count = 2;

    xi_u32 total_assets = assets->asset_count;

    if (xia_files.count != 0) {
        assets->file_count = 0;
        assets->files = xi_arena_push_array(arena, xiAssetFile, xia_files.count);

        // iterate over all of the files we have found, open them and check their header is valid
        // we then sum up the available assets and load their information
        //
        for (xi_u32 it = 0; it < xia_files.count; ++it) {
            xiDirectoryEntry *entry = &xia_files.entries[it];
            xiAssetFile *file = &assets->files[assets->file_count];

            // @todo: logging...
            //

            if (!xi_os_file_open(&file->handle, entry->path, file_access)) { continue; }
            if (!xi_os_file_read(&file->handle, &file->header, 0, sizeof(xiaHeader))) { continue; }

            if (file->header.signature != XIA_HEADER_SIGNATURE) { continue; }
            if (file->header.version   != XIA_HEADER_VERSION)   { continue; }

            file->base_asset = total_assets;
            total_assets += file->header.asset_count;

            // we know the asset information comes directly after the asset data so we can append
            // new assets at the info offset and then write the new asset info after the fact
            //
            // we also copy across the current asset count for updating
            //
            file->next_write_offset = file->header.info_offset;
            file->asset_count       = file->header.asset_count;

            // valid file found, increment
            //
            assets->file_count += 1;
        }

        if (assets->importer.enabled) {
            if (assets->max_assets == 0) {
                // like above we choose some max number of assets for the user to import if
                // they don't specify an amount for themselves
                //
                assets->max_assets = 1024;
            }
        }
    }

    xi_b32 new_file = false;
    if (assets->file_count == 0) {
        // in this case we either don't have any packed files to load from disk, or all of the packed
        // files we did find were invalid. if the importer is enabled, make a new file, otherwise continue
        // without files
        //
        if (assets->importer.enabled) {
            assets->file_count = 1;
            assets->files = xi_arena_push_type(arena, xiAssetFile);

            xiAssetFile *file = &assets->files[0];

            // so we can just choose the name of the asset file
            //
            xi_string imported = xi_str_format(temp, "%.*s/imported.xia", xi_str_unpack(path));
            if (xi_os_file_open(&file->handle, imported, file_access)) {
                // completely empty file header, but valid to read
                //
                file->header.signature      = XIA_HEADER_SIGNATURE;
                file->header.version        = XIA_HEADER_VERSION;
                file->header.asset_count    = 0;
                file->header.str_table_size = 0;
                file->header.info_offset    = sizeof(xiaHeader);

                file->base_asset        = assets->asset_count; // should == 2
                file->next_write_offset = file->header.info_offset;
                file->asset_count       = 0;

                new_file = true;

                if (assets->max_assets == 0) {
                    // some number of assets to import into if the user doesn't specify their own amount
                    //
                    assets->max_assets = 1024;
                }

                file->strings.used  = 0;
                file->strings.limit = assets->max_assets * (XI_U8_MAX  + 1);
                file->strings.data  = xi_arena_push_size(arena, file->strings.limit);
            }

        }

        if (!new_file) {
            assets->file_count = 0;
            assets->files      = 0;
        }
    }


    if (assets->max_assets < total_assets) {
        // we make sure there are at least as many assets as we found, the
        // user can override this count to allow assets to be imported
        //
        assets->max_assets = total_assets;
    }

    assets->xias   = xi_arena_push_array(arena, xiaAssetInfo, assets->max_assets);
    assets->assets = xi_arena_push_array(arena, xiAsset, assets->max_assets);

    assets->lookup_table_count = xi_pow2_nearest_u32(assets->max_assets) << 1;
    assets->lookup_table = xi_arena_push_array(arena, xiAssetHash, assets->lookup_table_count);

    // if there are no files available this loop won't do anything
    //
    // this loads the asset information
    //
    if (!new_file) {
        xi_u32 running_total = 0;
        for (xi_u32 it = 0; it < assets->file_count; ++it) {
            xiAssetFile *file = &assets->files[it];

            xi_u32 count = file->header.asset_count;

            // read the asset information
            //
            xi_uptr offset = file->header.info_offset;
            xi_uptr size   = count * sizeof(xiaAssetInfo);

            xiaAssetInfo *xias = &assets->xias[assets->asset_count];
            if (!xi_os_file_read(&file->handle, xias, offset, size)) { continue; }

            // read the string table, comes directly after the asset info array
            //
            offset += size;

            size = file->header.str_table_size;
            if (assets->importer.enabled && (it + 1) == assets->file_count) {
                // we always import to the final file, so if the importer is enabled give it the max
                // amount of space for strings so we can add more to it later when importing new assets
                //
                size = assets->max_assets * (XI_U8_MAX + 1);
            }

            xi_u8 *strings = xi_arena_push_size(arena, size);
            if (!xi_os_file_read(&file->handle, strings, offset, file->header.str_table_size)) { continue; }

            file->strings.limit = size;
            file->strings.used  = file->header.str_table_size;
            file->strings.data  = strings;

            for (xi_u32 ai = 0; ai < count; ++ai) {
                xiAsset *asset = &assets->assets[running_total + ai];

                asset->state = XI_ASSET_STATE_UNLOADED;
                asset->index = ai;
                asset->type  = xias[ai].type;
                asset->file  = it;

                // :asset_lookup insert into hash table
                //
                xi_string name;
                name.count =  strings[0];
                name.data  = &strings[1];

                xi_asset_manager_asset_hash_insert(assets, asset->type, running_total + ai, name);

                strings += name.count + 1;
            }

            assets->asset_count += count;
        }
    }

    if (assets->importer.enabled) {
        // we make a copy of the user set strings to make sure we own the memory of them
        //
        assets->importer.search_dir    = xi_str_copy(arena, assets->importer.search_dir);
        assets->importer.sprite_prefix = xi_str_copy(arena, assets->importer.sprite_prefix);

        xi_asset_manager_import_to_xia(assets);
    }
}

// :note importer code is below!
//
#define STBI_ONLY_PNG 1
#define STB_IMAGE_STATIC 1
#define STB_IMAGE_IMPLEMENTATION 1

#define STB_IMAGE_RESIZE_STATIC 1
#define STB_IMAGE_RESIZE_IMPLEMENTATION 1

#include <stb_image.h>
#include <stb_image_resize.h>

// reserves 'count' number of handles if there is enough space left, otherwise returns zero.
//
// returns the handle index of the first valid asset, if > 1 sequential handles can be used up to, but not
// including the number reserved
//
XI_INTERNAL xi_u32 xi_asset_handles_reserve(xiAssetManager *assets, xi_u32 count) {
    xi_u32 result;

    xi_u32 next;
    do {
        result = assets->asset_count;
        next   = result + count;

        if (next > assets->max_assets) {
            result = 0;
            break;
        }
    } while (!XI_ATOMIC_COMPARE_EXCHANGE_U32(&assets->asset_count, next, result));

    return result;
}

// reserves the size specified in the asset pack file, will return the offset into the file in which it
// is valid to write that data to. assets are always appended to the last valid file opened
//
XI_INTERNAL xi_uptr xi_asset_manager_size_reserve(xiAssetManager *assets, xi_uptr size) {
    xi_uptr result;

    xiAssetFile *file = &assets->files[assets->file_count - 1];

    result = XI_ATOMIC_ADD_U64(&file->next_write_offset, size);
    return result;
}

XI_INTERNAL xi_string xi_asset_name_append(xiAssetFile *file, xi_string name) {
    xi_string result = { 0 };

    XI_ASSERT(name.count <= XI_U8_MAX);

    // we don't need to check if there is enough space left in the strings buffer as we allocated
    // enough space to store 'max_assets' worth of max name length data
    //
    xi_uptr offset = XI_ATOMIC_ADD_U64(&file->strings.used, name.count + 1);
    xi_u8  *data   = &file->strings.data[offset];

    // this is probably real bad for false sharing because the string names are short and packed
    // closely one after another, maybe there is a better way to do this.
    //
    // @todo: maybe a secondary pass to insert the string names single-threaded would be a good
    // idea, we could do a validation to see if packed assets were actually written to the file
    // etc. as well!
    //
    data[0] = (xi_u8) name.count;
    xi_memory_copy(&data[1], name.data, name.count);

    result = xi_str_wrap_count(&data[1], name.count);
    return result;
}

XI_INTERNAL xi_uptr xi_image_mip_mapped_size(xi_u32 w, xi_u32 h) {
    xi_uptr result = (w * h);

    while (w > 1 || h > 1) {
        w >>= 1;
        h >>= 1;

        if (w == 0) { w = 1; }
        if (h == 0) { h = 1; }

        result += (w * h);
    }

    result <<= 2; // 4 bytes per pixel
    return result;
}

typedef struct xiAssetImportInfo {
    xiAssetManager *assets;
    xiDirectoryEntry *entry;
} xiAssetImportInfo;

XI_INTERNAL XI_THREAD_TASK_PROC(xi_image_asset_import) {
    (void) pool;

    xiAssetImportInfo *import = (xiAssetImportInfo *) arg;
    xiAssetManager *assets    = import->assets;

    xiArena *temp = xi_temp_get();

    xiDirectoryEntry *entry = import->entry;

    xi_string file_data = xi_file_read_all(temp, entry->path);
    if (xi_str_is_valid(file_data)) {
        int w, h, c;
        stbi_uc *image_data = stbi_load_from_memory(file_data.data,
                (int) file_data.count, &w, &h, &c, 4);

        if (image_data) {
            xi_u32 handle = xi_asset_handles_reserve(assets, 1);
            if (handle != 0) {
                xiAssetFile *file = &assets->files[assets->file_count - 1];

                xi_u8 *pixels      = image_data;
                xi_u8 level_count  = 0;
                xi_uptr total_size = 4 * w * h;

                // this is just a raw load and write, no modification done yet, do things
                // incrementally!
                //
                // also isn't multi-threaded, this will give me a better idea of what parts
                // can be part of the thread task and what parts have to be done here
                //
                xi_string basename = entry->basename;

                xi_string prefix = xi_str_prefix(basename, assets->importer.sprite_prefix.count);
                xi_b32 is_sprite = xi_str_equal(prefix, assets->importer.sprite_prefix);
                if (is_sprite) {
                    // we advance past the prefix so the actual lookup name doesn't
                    // require it
                    //
                    basename = xi_str_advance(basename, prefix.count);
                }

                // :note if an image asset is designated as as "sprite" it will be
                // downsized to fit within the texture array dimension specified by the
                // renderer, it will also have mip-maps generated and stored within
                // the file for it.
                //
                // animations are assumed to have each frame be a "sprite"
                // the base asset will encode the number of frames, subsequent frames will
                // not be included in the asset lookup array
                //

                xi_u8 *base = image_data;
                for (xi_s32 y = 0; y < h; ++y) {
                    for (xi_s32 x = 0; x < w; ++x) {
                        xi_f32 na = (c == 3) ? 1.0f : (base[3] / 255.0f);
                        xi_f32 lr = xi_srgb_u8_to_linear_f32(base[0]);
                        xi_f32 lg = xi_srgb_u8_to_linear_f32(base[1]);
                        xi_f32 lb = xi_srgb_u8_to_linear_f32(base[2]);

                        // pre-multiply the alpha and pack back into srgb
                        //
                        base[0] = xi_linear_f32_to_srgb_u8(na * lr);
                        base[1] = xi_linear_f32_to_srgb_u8(na * lg);
                        base[2] = xi_linear_f32_to_srgb_u8(na * lb);

                        base += 4;
                    }
                }

                // now we have our image as 'pre-multiplied srgb' we can downsize if as
                // needed to fit in the texture array if it is a sprite
                //
                if (is_sprite) {
                    if (w > (xi_s32) assets->sprite_dimension ||
                        h > (xi_s32) assets->sprite_dimension)
                    {
                        xi_f32 ratio = assets->sprite_dimension / (xi_f32) XI_MAX(w, h);

                        int new_w = (int) (ratio * w); // trunc or round?
                        int new_h = (int) (ratio * h); // trunc or round?

                        total_size = xi_image_mip_mapped_size(new_w, new_h);
                        pixels     = xi_arena_push_size(temp, total_size);

                        XI_ASSERT(new_w <= (xi_s32) assets->sprite_dimension);
                        XI_ASSERT(new_h <= (xi_s32) assets->sprite_dimension);

                        stbir_resize_uint8_srgb(image_data, w, h, 0,
                                pixels, new_w, new_h, 0, 4, 3, STBIR_FLAG_ALPHA_PREMULTIPLIED);

                        w = new_w;
                        h = new_h;
                    }
                    else {
                        // we don't need to resize the base image but we need to generate
                        // mipmaps
                        //
                        total_size = xi_image_mip_mapped_size(w, h);
                        pixels     = xi_arena_push_size(temp, total_size);
                    }

                    // generate mip maps
                    //
                    xi_u8 *cur   = pixels;
                    xi_u32 cur_w = w;
                    xi_u32 cur_h = h;

                    xi_u8 *next   = cur + (4 * cur_w * cur_h);
                    xi_u32 next_w = cur_w >> 1;
                    xi_u32 next_h = cur_h >> 1;

                    while (cur_w > 1 || cur_h > 1) {
                        if (next_w == 0) { next_w = 1; }
                        if (next_h == 0) { next_h = 1; }

                        stbir_resize_uint8_srgb(cur, cur_w, cur_h, 0,
                                next, next_w, next_h, 0, 4, 3, STBIR_FLAG_ALPHA_PREMULTIPLIED);

                        cur   = next;
                        cur_w = next_w;
                        cur_h = next_h;

                        next   = cur + (4 * cur_w * cur_h);
                        next_w = cur_w >> 1;
                        next_h = cur_h >> 1;

                        level_count += 1;
                    }

                    XI_ASSERT(next == (pixels + total_size));
                }

                xiAsset *asset    = &assets->assets[handle];
                xiaAssetInfo *xia = &assets->xias[handle];

                asset->state = XI_ASSET_STATE_UNLOADED;
                asset->type  = XIA_ASSET_TYPE_IMAGE;
                asset->index = handle - file->base_asset;
                asset->file  = assets->file_count - 1;

                // 4GiB per asset
                //
                XI_ASSERT(total_size <= XI_U32_MAX);

                xia->type   = asset->type;
                xia->size   = (xi_u32) total_size;
                xia->offset = xi_asset_manager_size_reserve(assets, xia->size);

                xia->write_time = entry->time;

                xia->image.width      = w;
                xia->image.height     = h;
                xia->image.flags      = 0;
                xia->image.mip_levels = level_count;

                xi_string name = xi_asset_name_append(file, basename);
                xi_asset_manager_asset_hash_insert(assets, asset->type, handle, name);

                // @todo: this assumes we successfully wrote to the file
                //
                // i don't know what the best option for write failure here is, it currently
                // means there will be a hole in the file, but the asset lookup will remain
                // vaild etc.
                //
                xi_os_file_write(&file->handle, pixels, xia->offset, xia->size);

                file->modified = true;
                XI_ATOMIC_INCREMENT_U32(&file->asset_count);
            }

            free(image_data);
        }
    }
}

void xi_asset_manager_import_to_xia(xiAssetManager *assets) {
    if (assets->file_count == 0) {
        // something went wrong during init causing no files to be allocated, just disable the importer
        // and bail
        //
        assets->importer.enabled = false;
        return;
    }

    xiArena *temp = xi_temp_get();

    xiDirectoryList list;
    xi_directory_list_get(temp, &list, assets->importer.search_dir, false); // we search sub-dirs separately

    xiAssetImportInfo *imports = xi_arena_push_array(temp, xiAssetImportInfo, list.count);

    for (xi_u32 it = 0; it < list.count; ++it) {
        xiDirectoryEntry *entry = &list.entries[it];

        if (entry->type == XI_DIRECTORY_ENTRY_TYPE_DIRECTORY) {
            // assume its an animation because it has its own sub-directory
            //
            // @todo: import animations!
            //
        }
        else {
            xi_uptr dot;
            if (!xi_str_find_last(entry->path, &dot, '.')) { continue; }

            xi_string ext = xi_str_suffix(entry->path, entry->path.count - dot);
            if (xi_str_equal(ext, xi_str_wrap_const(".png"))) {
                // we have an image asset!
                //
                xiAssetImportInfo *import = &imports[it];

                import->assets = assets;
                import->entry  = entry;

                xi_thread_pool_enqueue(assets->thread_pool, xi_image_asset_import, (void *) import);
            }
            else {
                int x = 0;
                x += 1;

                // unsupported asset type!
            }
        }
    }

    xi_thread_pool_await_complete(assets->thread_pool);

    for (xi_u32 it = 0; it < assets->file_count; ++it) {
        xiAssetFile *file = &assets->files[it];
        if (file->modified) {
            xiaHeader *header = &file->header;

            // update the asset count, info offset and size of the string table
            //
            header->asset_count    = file->asset_count;
            header->info_offset    = file->next_write_offset;
            header->str_table_size = file->strings.used;

            // write out header
            //
            xi_os_file_write(&file->handle, header, 0, sizeof(xiaHeader));

            // write out asset info
            //
            xi_uptr info_size = header->asset_count * sizeof(xiaAssetInfo);
            xi_os_file_write(&file->handle, &assets->xias[file->base_asset], header->info_offset, info_size);

            // write out string table
            //
            xi_uptr str_table_offset = header->info_offset + info_size;
            xi_os_file_write(&file->handle, file->strings.data, str_table_offset, header->str_table_size);

            file->modified = false;
        }
    }
}
