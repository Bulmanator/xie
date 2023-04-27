// @todo: this should be part of the public facing api!
//
#if XI_COMPILER_CL
    #define XI_ATOMIC_COMPARE_EXCHANGE_U32(target, value, compare) (_InterlockedCompareExchange((volatile long *) target, value, compare) == (long) compare)
    #define XI_ATOMIC_EXCHANGE_U32(target, value) _InterlockedExchange((volatile long *) target, value)
    #define XI_ATOMIC_INCREMENT_U32(target) _InterlockedIncrement((volatile long *) target)
    #define XI_ATOMIC_DECREMENT_U32(target) _InterlockedDecrement((volatile long *) target)
#else
    #error "incomplete implementation"
#endif

// forward declaring stuff that might be needed
//
XI_INTERNAL void xi_asset_manager_import_to_xia(xiAssetManager *assets);
XI_INTERNAL xi_uptr xi_image_mip_mapped_size(xi_u32 w, xi_u32 h);

// these are some large 32-bit prime numbers which are xor'd with the string hash value allowing us to
// produce a different hash values for the same 'name' of different asset types
//
// these haven't been chosen in any particular way
//
// @todo: should 'animation' have its own mix here, even though it is just a set of image assets. we
// may want a static 'image' called one thing, but an animated variant of that to be called the same
// thus we would have to have different asset types for animation and image
//
XI_GLOBAL const xi_u32 tbl_xia_type_hash_mix[XIA_ASSET_TYPE_COUNT - 1] = {
    0xcb03f267,
    0xe14e6b0b
};

// it is important that "name" is in asset manager owned memory as it is stored on the asset hash directly
//
XI_INTERNAL void xi_asset_manager_asset_hash_insert(xiAssetManager *assets,
        xi_u32 type, xi_u32 index, xi_string name)
{
    xi_u32 mask = assets->lookup_table_count - 1;
    xi_u32 mix  = tbl_xia_type_hash_mix[type];
    xi_u32 hash = xi_str_fnv1a_hash_u32(name) ^ mix;

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
    assets->transfer_queue = &xi->renderer.transfer_queue;

    xiArena *temp  = xi_temp_get();
    xi_string path = xi_str_format(temp, "%.*s/data", xi_str_unpack(xi->system.executable_path));

    if (!xi_os_directory_create(path)) { return; }

    // this is guaranteed to be valid as the renderer is initialised before the asset manager is
    // initialised, if the user doesn't specify a dimension for the sprite array the renderer
    // will select a default
    //
    assets->sprite_dimension = xi->renderer.sprite_array.dimension;

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
        xiAssetFile *files = xi_arena_push_array(temp, xiAssetFile, xia_files.count);
        assets->file_count = 0;

        // iterate over all of the files we have found, open them and check their header is valid
        // we then sum up the available assets and load their information
        //
        for (xi_u32 it = 0; it < xia_files.count; ++it) {
            xiDirectoryEntry *entry = &xia_files.entries[it];
            xiAssetFile *file = &files[assets->file_count];

            xi_b32 valid = false;

            if (xi_os_file_open(&file->handle, entry->path, file_access)) {
                if (xi_os_file_read(&file->handle, &file->header, 0, sizeof(xiaHeader))) {
                    if (file->header.signature == XIA_HEADER_SIGNATURE &&
                        file->header.version   == XIA_HEADER_VERSION)
                    {
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

                        valid = true;
                    }
                }
            }

            // @todo: logging.. if any of this falis!
            //

            if (!valid) { xi_os_file_close(&file->handle); }
        }

        assets->files = xi_arena_push_copy_array(arena, files, xiAssetFile, assets->file_count);

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

    assets->max_texture_handles = xi->renderer.texture_limit;
    assets->max_sprite_handles  = xi->renderer.sprite_array.limit;
    assets->next_sprite         = 2;
    assets->next_texture        = 0;

    assets->total_loads = 0;
    assets->next_load   = 0;
    assets->asset_loads = xi_arena_push_array(arena, xiAssetLoadInfo, assets->max_assets);

    // load the asset information
    //
    if (!new_file) {
        xi_u32 running_total = assets->asset_count; // some assets are reserved up front
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
                xiaAssetInfo *xia = &xias[ai];

                asset->state = XI_ASSET_STATE_UNLOADED;
                asset->index = ai;
                asset->type  = xia->type;
                asset->file  = it;


                if (xia->name_offset != XI_U32_MAX) {
                    // :asset_lookup insert into hash table
                    //
                    xi_string name;
                    name.count =  strings[xia->name_offset];
                    name.data  = &strings[xia->name_offset + 1];

                    xi_asset_manager_asset_hash_insert(assets, asset->type, running_total + ai, name);
                }
            }

            assets->asset_count += count;
        }
    }

    // upload the error texture
    //
    {
        // @incomplete: we really need samplers per texture so we don't have to upload this much
        // data for it. not really wasting anything though as its stored in the texture array which
        // already has pre-allocated space, but does cost in bandwidth at init time
        //
        // :default_texture_bandwidth
        //
        xi_u32 w = 256;
        xi_u32 h = 256;

        xiAsset *error = &assets->assets[0];

        error->state = XI_ASSET_STATE_LOADED;
        error->type  = XIA_ASSET_TYPE_IMAGE;
        error->index = 0;
        error->file  = (xi_u32) -1; // invalid, as its not part of a file. setting this to -1 will catch bugs

        error->data.texture.index  = 0;
        error->data.texture.width  = (xi_u16) w;
        error->data.texture.height = (xi_u16) h;

        xiaAssetInfo *error_xia = &assets->xias[0];

        error_xia->offset       = 0;
        error_xia->size         = 0;
        error_xia->name_offset  = 0;
        error_xia->type         = XIA_ASSET_TYPE_IMAGE;

        error_xia->image.width       = w;
        error_xia->image.height      = h;
        error_xia->image.frame_count = 0;
        error_xia->image.mip_levels  = 0;

        error_xia->write_time = 0;

        XI_ASSERT(w == h);

        void *data   = 0;
        xi_uptr size = xi_image_mip_mapped_size(w, h);

        xiRendererTransferTask *task;
        task = xi_renderer_transfer_queue_enqueue_size(assets->transfer_queue, &data, size);

        XI_ASSERT(data != 0);

        xi_u32 colour = 0xFF000000;

        xi_u32 *base = data;
        while (w > 1 || h > 1) {
            xi_u32 pps = 1;
            if (w > 4 || h > 4) { pps = w / 4; }

            for (xi_u32 y = 0; y < h; ++y) {
                if ((y % pps) == 0) { colour ^= 0x00FF00FF; } // force rows to start with different colours

                for (xi_u32 x = 0; x < w; ++x) {
                    if ((x % pps) == 0) { colour ^= 0x00FF00FF; }

                    xi_u32 px = (y * w) + x;
                    base[px] = colour;
                }
            }

            base += (w * h); // already a xi_u32 so no need for 4x

            w >>= 1;
            h >>= 1;
        }

        task->texture = error->data.texture;
        task->state   = XI_RENDERER_TRANSFER_TASK_STATE_LOADED;
    }

    // upload the solid white texture
    //
    {
        // :default_texture_bandwidth
        //
        xi_u32 w = 256;
        xi_u32 h = 256;

        xiAsset *white = &assets->assets[1];

        white->state = XI_ASSET_STATE_LOADED;
        white->type  = XIA_ASSET_TYPE_IMAGE;
        white->index = 1;
        white->file  = (xi_u32) -1; // invalid, as its not part of a file. setting this to -1 will catch bugs

        white->data.texture.index  = 1;
        white->data.texture.width  = (xi_u16) w;
        white->data.texture.height = (xi_u16) h;

        xiaAssetInfo *white_xia = &assets->xias[1];

        white_xia->offset      = 0;
        white_xia->size        = 0;
        white_xia->name_offset = 0;
        white_xia->type        = XIA_ASSET_TYPE_IMAGE;

        white_xia->image.width       = w;
        white_xia->image.height      = h;
        white_xia->image.frame_count = 0;
        white_xia->image.mip_levels  = 0;

        white_xia->write_time = 0;

        void *data   = 0;
        xi_uptr size = xi_image_mip_mapped_size(w, h);

        xiRendererTransferTask *task;
        task = xi_renderer_transfer_queue_enqueue_size(assets->transfer_queue, &data, size);

        xi_memory_set(data, 0xFF, size);

        task->texture = white->data.texture;
        task->state   = XI_RENDERER_TRANSFER_TASK_STATE_LOADED;
    }

    if (assets->sample_buffer.limit == 0) {
        // to be on the safe side
        //
        assets->sample_buffer.limit = XI_MB(128);
    }

    assets->sample_buffer.used = 0;
    assets->sample_buffer.data = xi_arena_push_size(arena, assets->sample_buffer.limit);

    assets->sample_rate = xi->audio_player.sample_rate;
    XI_ASSERT(assets->sample_rate > 0);

    xi->renderer.assets = assets;

    if (assets->importer.enabled) {
        // we make a copy of the user set strings to make sure we own the memory of them
        //
        assets->importer.search_dir    = xi_str_copy(arena, assets->importer.search_dir);
        assets->importer.sprite_prefix = xi_str_copy(arena, assets->importer.sprite_prefix);

        xi_asset_manager_import_to_xia(assets);
    }
}

//
// :note loading assets
//
XI_INTERNAL XI_THREAD_TASK_PROC(xi_asset_data_load) {
    (void) pool;

    xiAssetLoadInfo *load = (xiAssetLoadInfo *) arg;

    xiAssetManager *assets = load->assets;
    xiAsset *asset    = load->asset;
    xiAssetFile *file = load->file;

    if (xi_os_file_read(&file->handle, load->data, load->offset, load->size)) {
        if (load->transfer_task) {
            // mark the task as loaded so it can be uploaded to the gpu later on
            //
            XI_ATOMIC_EXCHANGE_U32(&load->transfer_task->state, XI_RENDERER_TRANSFER_TASK_STATE_LOADED);
        }

        XI_ATOMIC_EXCHANGE_U32(&asset->state, XI_ASSET_STATE_LOADED);
    }
    else {
        XI_ATOMIC_EXCHANGE_U32(&asset->state, XI_ASSET_STATE_UNLOADED);
        if (load->transfer_task) {
            // mark the transfer task as unused to signify we shouldn't copy from this as it failed to
            // load
            //
            XI_ATOMIC_EXCHANGE_U32(&load->transfer_task->state, XI_RENDERER_TRANSFER_TASK_STATE_UNUSED);
        }
    }

    XI_ATOMIC_DECREMENT_U32(&assets->total_loads);
}

XI_INTERNAL xiRendererTexture xi_renderer_texture_acquire(xiAssetManager *assets, xi_u32 width, xi_u32 height) {
    xiRendererTexture result;

    if (width > assets->sprite_dimension || height > assets->sprite_dimension) {
        // can't be a sprite, otherwise our asset packer would've resized it for us
        //
        result.index = assets->next_texture++;

        // :renderer_handles
        //
        XI_ASSERT(assets->next_texture <= assets->max_texture_handles);
    }
    else {
        result.index  = assets->next_sprite++;

        // :renderer_handles
        //
        XI_ASSERT(assets->next_sprite <= assets->max_sprite_handles);
    }

    // @todo: error checking if width or height > U16_MAX, maybe even request from the renderer the
    // max supported texture size and limit on that
    //
    result.width  = (xi_u16) width;
    result.height = (xi_u16) height;

    return result;
}

void xi_image_load(xiAssetManager *assets, xiImageHandle image) {
    if (assets->total_loads < assets->max_assets) {
        if (image.value < assets->asset_count) {
            xiAsset *asset = &assets->assets[image.value];
            if (asset->state != XI_ASSET_STATE_LOADED) {
                xiaAssetInfo *xia = &assets->xias[image.value];

                // it is a bug if an image handle is passed to this function and the asset type is not
                // an image so we should assert this to catch bugs
                //
                XI_ASSERT(xia->type == XIA_ASSET_TYPE_IMAGE);

                asset->data.texture = xi_renderer_texture_acquire(assets, xia->image.width, xia->image.height);

                xiAssetLoadInfo *asset_load = &assets->asset_loads[assets->next_load++];
                assets->next_load %= assets->max_assets;

                xiRendererTransferQueue *transfer_queue = assets->transfer_queue;

                asset->state = XI_ASSET_STATE_PENDING;

                asset_load->assets = assets;
                asset_load->file   = &assets->files[asset->file];
                asset_load->asset  = asset;
                asset_load->offset = xia->offset;
                asset_load->size   = xia->size;

                xiRendererTransferTask *transfer_task;
                transfer_task = xi_renderer_transfer_queue_enqueue_size(transfer_queue,
                            &asset_load->data, asset_load->size);

                asset_load->transfer_task = transfer_task;
                if (transfer_task != 0) {
                    transfer_task->texture = asset->data.texture;
                    assets->total_loads += 1;
                    xi_thread_pool_enqueue(assets->thread_pool, xi_asset_data_load, (void *) asset_load);
                }
            }
        }
    }
}

XI_INTERNAL xi_u32 xi_asset_index_get_by_name(xiAssetManager *assets, xi_string name, xi_u32 type) {
    xi_u32 result;

    xi_u32 mask = assets->lookup_table_count - 1;
    xi_u32 mix  = tbl_xia_type_hash_mix[type];
    xi_u32 hash = xi_str_fnv1a_hash_u32(name) ^ mix;

    xi_u32 i = 0;

    xi_b32 found = false;

    do {
        if (i > assets->lookup_table_count) {
            // we have tried every slot and didn't find anything so bail
            //
            result = 0;
            break;
        }

        // quadratic probing
        //
        xi_u32 table_index = (hash + (i * i)) & mask;
        xiAssetHash *slot  = &assets->lookup_table[table_index];

        // :asset_slot
        //
        result = slot->index;
        found  = (slot->hash == hash) && xi_str_equal(slot->value, name);

        i += 1;
    }
    while (!found && result);

    return result;
}

xiImageHandle xi_image_get_by_name_str(xiAssetManager *assets, xi_string name) {
    xiImageHandle result;

    result.value = xi_asset_index_get_by_name(assets, name, XIA_ASSET_TYPE_IMAGE);
    return result;
}

xiImageHandle xi_image_get_by_name(xiAssetManager *assets, const char *name) {
    xiImageHandle result = xi_image_get_by_name_str(assets, xi_str_wrap_cstr(name));
    return result;
}

xiaImageInfo *xi_image_info_get(xiAssetManager *assets, xiImageHandle image) {
    xiaImageInfo *result = &assets->xias[image.value].image;
    return result;
}

xiRendererTexture xi_image_data_get(xiAssetManager *assets, xiImageHandle image) {
    xiRendererTexture result = assets->assets[0].data.texture; // always valid

    xiAsset *asset = &assets->assets[image.value];
    XI_ASSERT(asset->type == XIA_ASSET_TYPE_IMAGE); // is a bug to pass image handle that isn't an image

    if (asset->state == XI_ASSET_STATE_UNLOADED) {
        // happens here to a) make sure the texture handle is valid, and b) we can't rely on
        // get_by_name to load the asset due to certain paths such as animations etc.
        //
        xi_image_load(assets, image);
    }

    result = asset->data.texture;

    return result;
}

xiAnimation xi_animation_get_by_name_str_flags(xiAssetManager *assets, xi_string name, xi_u32 flags) {
    xiImageHandle image = xi_image_get_by_name_str(assets, name);
    xiAnimation result  = xi_animation_create_from_image_flags(assets, image, flags);
    return result;
}

xiAnimation xi_animation_get_by_name_flags(xiAssetManager *assets, const char *name, xi_u32 flags) {
    xiAnimation result = xi_animation_get_by_name_str_flags(assets, xi_str_wrap_cstr(name), flags);
    return result;
}

xiAnimation xi_animation_create_from_image_flags(xiAssetManager *assets, xiImageHandle image, xi_u32 flags) {
    xiAnimation result = { 0 };

    result.base_frame = image;
    result.flags      = flags;
    result.frame_dt   = assets->animation_dt;

    xiaImageInfo *info = xi_image_info_get(assets, image);
    if (info) {
        XI_ASSERT(info->frame_count > 0);

        result.frame_count = info->frame_count;
        if (flags & XI_ANIMATION_FLAG_REVERSE) {
            result.current_frame = info->frame_count - 1; // -1 as zero based!
        }
    }

    for (xi_u32 it = 0; it < result.frame_count; ++it) {
        xiImageHandle handle = { image.value + it };
        xi_image_load(assets, handle); // we prefetch the frames
    }

    return result;
}

xiAnimation xi_animation_get_by_name_str(xiAssetManager *assets, xi_string name) {
    xiAnimation result = xi_animation_get_by_name_str_flags(assets, name, 0);
    return result;
}

xiAnimation xi_animation_get_by_name(xiAssetManager *assets, const char *name) {
    xiAnimation result = xi_animation_get_by_name_str(assets, xi_str_wrap_cstr(name));
    return result;
}

xiAnimation xi_animation_create_from_image(xiAssetManager *assets, xiImageHandle image) {
    xiAnimation result = xi_animation_create_from_image_flags(assets, image, 0);
    return result;
}

xi_b32 xi_animation_update(xiAnimation *animation, xi_f32 dt) {
    xi_b32 result = false;

    xi_b32 reverse     = (animation->flags & XI_ANIMATION_FLAG_REVERSE);
    xi_u32 last_frame  = reverse ? 0 : animation->frame_count - 1;

    if (animation->flags & XI_ANIMATION_FLAG_ONE_SHOT) {
        // we ony return 'true' for one-shot animations once, this is handled below when the animation
        // actually transitions to this last frame
        //
        if (animation->current_frame == last_frame) { return result; }
    }
    else if (animation->flags & XI_ANIMATION_FLAG_PAUSED) {
        return result;
    }

    animation->frame_accum += dt;
    if (animation->frame_accum >= animation->frame_dt) {
        animation->frame_accum -= animation->frame_dt;

        // check if we are at the end of the animation
        //
        if (animation->current_frame == last_frame) {
            if (animation->flags & XI_ANIMATION_FLAG_PING_PONG) {
                animation->flags ^= XI_ANIMATION_FLAG_REVERSE;
                reverse = !reverse;
            }
            else {
                // loop back to the beginning, one-shot is handled above
                //
                animation->current_frame = reverse ? animation->frame_count - 1 : 0;
            }
        }

        // update the animation frame
        //
        if (reverse) { animation->current_frame -= 1; }
                else { animation->current_frame += 1; }

        result = (animation->current_frame == last_frame);
    }

    return result;
}

xi_f32 xi_animation_playback_percentage_get(xiAnimation *animation) {
    xi_f32 result;

    xi_u32 frames = animation->frame_count - 1; // zero based to make it align with 1.0 at 100%

    if (animation->flags & XI_ANIMATION_FLAG_REVERSE) {
        result =  (frames - animation->current_frame) / (xi_f32) frames;
    }
    else {
        result = animation->current_frame / (xi_f32) frames;
    }

    return result;
}

void xi_animation_reset(xiAnimation *animation) {
    xi_u32 first_frame = (animation->flags & XI_ANIMATION_FLAG_REVERSE) ? animation->frame_count - 1 : 0;
    animation->current_frame = first_frame;
}

void xi_animation_pause(xiAnimation *animation) {
    animation->flags |= XI_ANIMATION_FLAG_PAUSED;
}

void xi_animation_unpause(xiAnimation *animation) {
    animation->flags &= ~XI_ANIMATION_FLAG_PAUSED;
}

xiImageHandle xi_animation_current_frame_get(xiAnimation *animation) {
    xiImageHandle result;

    result.value = animation->base_frame.value + animation->current_frame;
    return result;
}

// sound handling functions
//
void xi_sound_load(xiAssetManager *assets, xiSoundHandle sound) {
    if (assets->total_loads < assets->max_assets) {
        if (sound.value < assets->asset_count) {
            xiAsset *asset = &assets->assets[sound.value];
            if (asset->state != XI_ASSET_STATE_LOADED) {
                xiaAssetInfo *xia = &assets->xias[sound.value];

                // it is a bug if a sound handle is passed to this function and the asset type is not
                // a sound so we should assert this to catch bugs
                //
                XI_ASSERT(xia->type == XIA_ASSET_TYPE_SOUND);

                xi_uptr remaining = (assets->sample_buffer.limit - assets->sample_buffer.used);
                if (remaining >= xia->size) {
                    asset->data.sample_base = assets->sample_buffer.used;
                    assets->sample_buffer.used += xia->size;

                    xiAssetLoadInfo *asset_load = &assets->asset_loads[assets->next_load++];
                    assets->next_load %= assets->max_assets;

                    asset->state = XI_ASSET_STATE_PENDING;

                    asset_load->assets = assets;
                    asset_load->file   = &assets->files[asset->file];
                    asset_load->asset  = asset;
                    asset_load->data   = &assets->sample_buffer.data[asset->data.sample_base];
                    asset_load->offset = xia->offset;
                    asset_load->size   = xia->size;

                    assets->total_loads += 1;

                    xi_thread_pool_enqueue(assets->thread_pool, xi_asset_data_load, (void *) asset_load);
                }
            }
        }
    }
}

xiSoundHandle xi_sound_get_by_name_str(xiAssetManager *assets, xi_string name) {
    xiSoundHandle result;

    result.value = xi_asset_index_get_by_name(assets, name, XIA_ASSET_TYPE_SOUND);
    return result;
}

xiSoundHandle xi_sound_get_by_name(xiAssetManager *assets, const char *name) {
    xiSoundHandle result = xi_sound_get_by_name_str(assets, xi_str_wrap_cstr(name));
    return result;
}

xiaSoundInfo *xi_sound_info_get(xiAssetManager *assets, xiSoundHandle sound) {
    xiaSoundInfo *result = &assets->xias[sound.value].sound;
    return result;
}

xi_s16 *xi_sound_data_get(xiAssetManager *assets, xiSoundHandle sound) {
    xi_s16 *result = 0;

    xiAsset *asset = &assets->assets[sound.value];
    XI_ASSERT(asset->type == XIA_ASSET_TYPE_SOUND);

    if (asset->state == XI_ASSET_STATE_UNLOADED) {
        xi_sound_load(assets, sound);
    }

    result = (xi_s16 *) &assets->sample_buffer.data[asset->data.sample_base];
    return result;
}

//
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

XI_INTERNAL xi_string xi_asset_name_append(xiAssetFile *file, xiaAssetInfo *info, xi_string name) {
    xi_string result = { 0 };

    XI_ASSERT(name.count <= XI_U8_MAX);

    // we don't need to check if there is enough space left in the strings buffer as we allocated
    // enough space to store 'max_assets' worth of max name length data
    //
    xi_uptr offset = XI_ATOMIC_ADD_U64(&file->strings.used, name.count + 1);
    xi_u8  *data   = &file->strings.data[offset];

    // @todo: maybe a secondary pass to insert the string names single-threaded would be a good
    // idea, we could do a validation to see if packed assets were actually written to the file
    // etc. as well!
    //
    data[0] = (xi_u8) name.count;
    xi_memory_copy(&data[1], name.data, name.count);

    info->name_offset = (xi_u32) offset;

    result = xi_str_wrap_count(&data[1], name.count);
    return result;
}

xi_uptr xi_image_mip_mapped_size(xi_u32 w, xi_u32 h) {
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
    xi_u32 handle;      // if this is != 0, we have pre-allocated a handle for the asset
    xi_s32 frame_index; // valid if >= 0, signifies its an animation frame

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
            xi_u32 handle = import->handle;
            if (handle == 0) {
                // reserve a single asset
                //
                handle = xi_asset_handles_reserve(assets, 1);
            }

            if (handle != 0) {
                xiAssetFile *file = &assets->files[assets->file_count - 1];

                xi_u8 *pixels      = image_data;
                xi_u8 level_count  = 0;
                xi_uptr total_size = 4 * w * h;

                xi_string basename = entry->basename;

                xi_b32 is_animated = (import->frame_index >= 0);
                xi_string prefix   = xi_str_prefix(basename, assets->importer.sprite_prefix.count);
                xi_b32 is_sprite   = xi_str_equal(prefix, assets->importer.sprite_prefix);

                if (is_sprite) {
                    // we advance past the prefix so the actual lookup name doesn't
                    // require it
                    //
                    basename = xi_str_advance(basename, prefix.count);
                }
                else if (is_animated && (import->frame_index == 0)) {
                    // we only do this for the first animation frame as subsequent animation frames cannot
                    // be looked up directly so we don't need to insert them into the asset lookup table
                    //
                    xi_uptr offset = 0;
                    if (xi_str_find_last(basename, &offset, '_')) {
                        // animation frames have _XXXX at the end to mark what their frame number is
                        // so we slice off the end
                        //
                        basename = xi_str_remove(basename, basename.count - offset);
                    }
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
                        xi_f32 lr = xi_srgb_unorm_to_linear_norm(base[0]);
                        xi_f32 lg = xi_srgb_unorm_to_linear_norm(base[1]);
                        xi_f32 lb = xi_srgb_unorm_to_linear_norm(base[2]);

                        // pre-multiply the alpha and pack back into srgb
                        //
                        base[0] = xi_linear_norm_to_srgb_unorm(na * lr);
                        base[1] = xi_linear_norm_to_srgb_unorm(na * lg);
                        base[2] = xi_linear_norm_to_srgb_unorm(na * lb);

                        base += 4;
                    }
                }

                // now we have our image as 'pre-multiplied srgb' we can downsize if as
                // needed to fit in the texture array if it is a sprite
                //
                if (is_sprite || is_animated) {
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

                        // copy the base image data into the new buffer
                        //
                        xi_memory_copy(pixels, image_data, 4 * w * h);
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

                xia->type        = asset->type;
                xia->size        = (xi_u32) total_size;
                xia->offset      = xi_asset_manager_size_reserve(assets, xia->size);
                xia->write_time  = entry->time;

                // @todo: 'name_offset' being a xi_u32 artifically limits the file size to just
                // under 4GiB maybe this shuold just be a xi_u64 instead
                //
                xia->name_offset = XI_U32_MAX;

                xia->image.width      = w;
                xia->image.height     = h;
                xia->image.flags      = 0;
                xia->image.mip_levels = level_count;

                // xia->image.frame_count is set elsewhere so it doesn't need to be touched here
                //

                // this puts the name in the asset lookup table if it is either not part of
                // an animation or if it is the first frame of an animation
                //
                if (import->frame_index <= 0) {
                    xi_string name = xi_asset_name_append(file, xia, basename);
                    xi_asset_manager_asset_hash_insert(assets, asset->type, handle, name);
                }

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

//
// wav importing
//
#pragma pack(push, 1)
typedef struct xiWavHeader {
    xi_u32 riff_code;
    xi_u32 size;
    xi_u32 wave_code;
} xiWavHeader;

#define XI_PCM_FORMAT (0x0001)

#define XI_RIFF_CODE_RIFF XI_FOURCC('R', 'I', 'F', 'F')
#define XI_RIFF_CODE_WAVE XI_FOURCC('W', 'A', 'V', 'E')
#define XI_RIFF_CODE_FMT  XI_FOURCC('f', 'm', 't', ' ')
#define XI_RIFF_CODE_DATA XI_FOURCC('d', 'a', 't', 'a')

typedef struct xiWavChunk {
    xi_u32 code;
    xi_u32 size;
} xiWavChunk;

typedef struct xiWavFmtChunk {
    xi_u16 format;
    xi_u16 channels;
    xi_u32 sample_rate;
    xi_u32 data_rate;
    xi_u16 block_align;
    xi_u16 bit_depth;
    xi_u16 ext_size;
    xi_u16 valid_bits;
    xi_u32 channel_mask;
    xi_u8  sub_format[16];
} xiWavFmtChunk;

#pragma pack(pop)

XI_INTERNAL XI_THREAD_TASK_PROC(xi_sound_asset_import) {
    (void) pool;

    xiAssetImportInfo *import = (xiAssetImportInfo *) arg;
    xiAssetManager *assets    = import->assets;

    xiArena *temp = xi_temp_get();

    xiDirectoryEntry *entry = import->entry;

    xi_string file_data = xi_file_read_all(temp, entry->path);
    if (xi_str_is_valid(file_data)) {
        xiWavHeader *header = (xiWavHeader *) file_data.data;
        if (header->riff_code == XI_RIFF_CODE_RIFF &&
            header->wave_code == XI_RIFF_CODE_WAVE)
        {
            xi_u8 *data  = file_data.data;
            xi_u32 total = header->size;
            xi_u32 used  = sizeof(xiWavHeader);

            xiWavFmtChunk *fmt = 0;

            xi_u32 samples_size = 0;
            xi_u8 *samples = 0;

            while (used < total) {
                xiWavChunk *chunk = (xiWavChunk *) (data + used);
                switch (chunk->code) {
                    case XI_RIFF_CODE_FMT: {
                        fmt = (xiWavFmtChunk *) (chunk + 1);
                    }
                    break;
                    case XI_RIFF_CODE_DATA: {
                        samples_size = chunk->size;
                        samples = (xi_u8 *) (chunk + 1);
                    }
                    break;
                }

                used += sizeof(xiWavChunk);
                used += chunk->size;
            }

            if (fmt && samples && samples_size > 0) {
                xi_b32 matches = (fmt->format == XI_PCM_FORMAT) && (fmt->channels == 2) &&
                    (fmt->sample_rate == assets->sample_rate) && (fmt->bit_depth == 16);

                if (matches) {
                    xi_u32 handle = xi_asset_handles_reserve(assets, 1);
                    if (handle != 0) {
                        xiAssetFile *file = &assets->files[assets->file_count - 1];

                        xiAsset *asset    = &assets->assets[handle];
                        xiaAssetInfo *xia = &assets->xias[handle];

                        asset->state = XI_ASSET_STATE_UNLOADED;
                        asset->type  = XIA_ASSET_TYPE_SOUND;
                        asset->index = handle - file->base_asset;
                        asset->file  = assets->file_count - 1;

                        // 4GiB per asset, we couldn't even load a sound this big anyway
                        //
                        XI_ASSERT(samples_size <= XI_U32_MAX);

                        xia->type        = asset->type;
                        xia->size        = (xi_u32) samples_size;
                        xia->offset      = xi_asset_manager_size_reserve(assets, xia->size);
                        xia->write_time  = entry->time;

                        // @todo: 'name_offset' being a xi_u32 artifically limits the file size to just
                        // under 4GiB maybe this shuold just be a xi_u64 instead
                        //
                        xia->name_offset = XI_U32_MAX;

                        xia->sound.channel_count = fmt->channels;
                        xia->sound.sample_count  = samples_size / (fmt->bit_depth >> 3);

                        xi_string name = xi_asset_name_append(file, xia, entry->basename);
                        xi_asset_manager_asset_hash_insert(assets, asset->type, handle, name);

                        xi_os_file_write(&file->handle, samples, xia->offset, xia->size);

                        file->modified = true;
                        XI_ATOMIC_INCREMENT_U32(&file->asset_count);
                    }
                }
            }
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

    xi_u32 next_import = 0;
    xiAssetImportInfo *imports = xi_arena_push_array(temp, xiAssetImportInfo, assets->max_assets);

    for (xi_u32 it = 0; it < list.count; ++it) {
        xiDirectoryEntry *entry = &list.entries[it];

        if (entry->type == XI_DIRECTORY_ENTRY_TYPE_DIRECTORY) {
            // assume its an animation because it has its own sub-directory
            //
            // @test: see if its okay to clobber the input list by supplying the input and output as the
            // same list, could be useful when you're only filtering once and don't care about the overall list
            //
            // @todo: this should be its own threaded task which then in turn enqueues its own image import
            // task for each frame of the animation
            //
            xiDirectoryList all, frames;
            xi_directory_list_get(temp, &all, entry->path, false);
            xi_directory_list_filter_for_extension(temp, &frames, &all, xi_str_wrap_const(".png"));

            if (frames.count > 0) {
                xi_b32 needs_packing = true;
                {
                    // check if the animation has already been packed into the asset file, if it has
                    // then we don't need to import it again...
                    //
                    // @todo: this doesn't check if any of the frames have been modified (newer write time)
                    // but it should check incase there have been file modifications
                    //
                    xiDirectoryEntry *first = &frames.entries[0];
                    xi_uptr offset = 0;
                    if (xi_str_find_last(first->basename, &offset, '_')) {
                        xi_string name = xi_str_remove(first->basename, first->basename.count - offset);

                        xiImageHandle image = xi_image_get_by_name_str(assets, name);
                        needs_packing = (image.value == 0);
                    }
                    else {
                        // incorrect naming format
                        //
                        // @robustness: the first entry may fail, however, if this is the case the
                        // animation won't be packed anyway because the number of found frames won't
                        // match the number of entries in the frames directory list
                        //
                        needs_packing = false;
                    }
                }

                if (needs_packing) {
                    // assume there are count many frames
                    //
                    xi_u32 *map = xi_arena_push_array(temp, xi_u32, frames.count);

                    xi_u32 frame_count = 0;
                    for (xi_u32 f = 0; f < frames.count; ++f) {
                        xiDirectoryEntry *frame = &frames.entries[f];

                        xi_uptr offset;
                        if (xi_str_find_last(frame->basename, &offset, '_')) {

                            xi_string number = xi_str_advance(frame->basename, offset + 1);
                            xi_u32 frame_index;
                            if (xi_str_parse_u32(number, &frame_index)) {
                                XI_ASSERT(frame_index < frames.count);

                                // this makes sure the frames are in order
                                //
                                map[frame_index] = f;
                                frame_count += 1;
                            }
                            else {
                                // @todo: logging...
                                //
                            }
                        }
                    }

                    if (frame_count == frames.count) {
                        // we have a valid animation and all frames are present
                        //
                        xi_u32 base_handle = xi_asset_handles_reserve(assets, frame_count);
                        if (base_handle != 0) {
                            // @hack: this entire animation import needs to be pulled out into its
                            // own routine ideally, so we can leverage a different thread to do this
                            // stuff for us, really all this loop should be is pushing 'asset_import' onto
                            // the thread queue
                            //
                            xiaAssetInfo *base = &assets->xias[base_handle];
                            base->image.frame_count = (xi_u8) frame_count;

                            for (xi_u32 f = 0; f < frame_count; ++f) {
                                XI_ASSERT(next_import < assets->max_assets);

                                xiAssetImportInfo *import = &imports[next_import];
                                next_import += 1;

                                import->frame_index = (xi_s32) f;

                                import->assets = assets;
                                import->entry  = &frames.entries[map[f]];

                                import->handle = base_handle + f;

                                xi_thread_pool_enqueue(assets->thread_pool,
                                        xi_image_asset_import, (void *) import);
                            }
                        }
                    }
                    else {
                        // @todo: logging...
                        //
                    }
                }
            }
            else {
                // @todo: logging....
                //
            }
        }
        else if (xi_str_ends_with(entry->path, xi_str_wrap_const(".png"))) {
            // we have an image asset!
            //
            // @incomplete: this will break if there are more assets to import than max assets were
            // allocated! come up with some sort of ring buffer design to use the imports
            //
            XI_ASSERT(next_import < assets->max_assets);

            // check if it is a sprite
            //
            // @duplicate: from above in the actual image import code, maybe this can happen
            // there?
            //
            xi_string basename = entry->basename;
            xi_string prefix   = xi_str_prefix(basename, assets->importer.sprite_prefix.count);
            if (xi_str_equal(prefix, assets->importer.sprite_prefix)) {
                basename = xi_str_advance(basename, prefix.count);
            }

            // @todo: this needs to import assets that have changed!
            //

            xiImageHandle image = xi_image_get_by_name_str(assets, basename);
            if (image.value == 0) {
                xiAssetImportInfo *import = &imports[next_import];
                next_import += 1;

                import->assets      =  assets;
                import->entry       =  entry;
                import->handle      =  0; // allocated automatically
                import->frame_index = -1;

                xi_thread_pool_enqueue(assets->thread_pool, xi_image_asset_import, (void *) import);
            }
        }
        else if (xi_str_ends_with(entry->path, xi_str_wrap_const(".wav"))) {
            // this assumes that the audio player has been configured to the sample-rate that
            // the games assest will be using
            //
            xiSoundHandle sound = xi_sound_get_by_name_str(assets, entry->basename);
            if (sound.value == 0) {
                xiAssetImportInfo *import = &imports[next_import];
                next_import += 1;

                import->assets = assets;
                import->entry  = entry;
                import->handle = 0; // allocated automatically

                xi_thread_pool_enqueue(assets->thread_pool, xi_sound_asset_import, (void *) import);
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
