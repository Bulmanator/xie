// forward declaring stuff that might be needed
//
FileScope void AssetManagerImportToXia(AssetManager *assets);
FileScope U64  MipMappedImageSize(U32 w, U32 h);

// these are some large 32-bit prime numbers which are xor'd with the string hash value allowing us to
// produce a different hash values for the same 'name' of different asset types
//
// these haven't been chosen in any particular way
//
// @todo: should 'animation' have its own mix here, even though it is just a set of image assets. we
// may want a static 'image' called one thing, but an animated variant of that to be called the same
// thus we would have to have different asset types for animation and image
//
GlobalVar const U32 tbl_xia_type_hash_mix[XIA_ASSET_TYPE_COUNT - 1] = {
    0xcb03f267,
    0xe14e6b0b
};

// it is important that "name" is in asset manager owned memory as it is stored on the asset hash directly
//
FileScope void AssetManagerHashInsert(AssetManager *assets, Xia_AssetType type, U32 index, Str8 name) {
    U32 mask = assets->lookup_table_count - 1;
    U32 mix  = tbl_xia_type_hash_mix[type];
    U32 hash = Str8_Fnv1aHashU32(name) ^ mix;

    U32 it = 0;
    AssetHash *slot = 0;
    do {
        if (it > assets->lookup_table_count) {
            slot = 0;
            break;
        }

        // Quadratic probing
        //
        U32 table_index = (hash + (it * it)) & mask;
        slot = &assets->lookup_table[table_index];

        it += 1;
    } while (!U32AtomicCompareExchange(&slot->index, index, 0));

    // we initialise our lookup table to be 2 * round_pow2(max_assets), which will have a maximum load
    // factor of 50%, so this lookup will realisitically never fail
    //
    // That said 50% is pretty low so should probably not waste that much space
    //
    Assert(slot != 0);

    slot->hash  = hash;
    slot->index = index; // overall asset index
    slot->value = name;  // its safe to store this string here because we have to hold it for the files
}

FileScope void AssetManagerInit(M_Arena *arena, AssetManager *assets, Xi_Engine *xi) {
    assets->arena          = arena;
    assets->thread_pool    = &xi->thread_pool;
    assets->transfer_queue = &xi->renderer.transfer_queue;

    M_Arena *temp = M_TempGet();
    Str8     path = Str8_Format(temp, "%.*s/data", Str8_Arg(xi->system.executable_path));

    if (!OS_DirectoryCreate(path)) { return; }

    // this is guaranteed to be valid as the renderer is initialised before the asset manager is
    // initialised, if the user doesn't specify a dimension for the sprite array the renderer
    // will select a default
    //
    assets->sprite_dimension = xi->renderer.sprite_array.dimension;

    DirectoryList data_dir  = DirectoryListGet(temp, path, false);
    DirectoryList xia_files = DirectoryListFilterForSuffix(temp, &data_dir, Str8_Literal(".xia"));

    U32 file_access = assets->importer.enabled ? FILE_ACCESS_FLAG_READWRITE : FILE_ACCESS_FLAG_READ;

    // the first two assets are reserved for error checking and a white texture used in place when
    // textureless quads are drawn, the 0 index will be a magenta/black checkerboard texture,
    // the 1 index will be said white texture
    //
    assets->num_assets = 2;

    U32 total_assets = assets->num_assets;

    if (xia_files.num_entries != 0) {
        AssetFile *files  = M_ArenaPush(temp, AssetFile, xia_files.num_entries);
        assets->num_files = 0;

        // iterate over all of the files we have found, open them and check their header is valid
        // we then sum up the available assets and load their information
        //
        for (U32 it = 0; it < xia_files.num_entries; ++it) {
            DirectoryEntry *entry = &xia_files.entries[it];
            AssetFile      *file  = &files[assets->num_files];

            B32 valid = false;

            file->handle = OS_FileOpen(entry->path, file_access);
            if (OS_FileRead(&file->handle, &file->header, 0, sizeof(Xia_Header))) {
                U32 signature = file->header.signature;
                U32 version   = file->header.version;

                if (signature == XIA_HEADER_SIGNATURE && version == XIA_HEADER_VERSION) {
                    file->base_asset  = total_assets;
                    total_assets     += file->header.num_assets;

                    // we know the asset information comes directly after the asset data so we can append
                    // new assets at the info offset and then write the new asset info after the fact
                    //
                    // we also copy across the current asset count for updating
                    //
                    file->next_write_offset = file->header.info_offset;
                    file->num_assets        = file->header.num_assets;

                    // valid file found, increment
                    //
                    assets->num_files += 1;

                    valid = true;
                }
            }

            // @todo: logging.. if any of this falis!
            //

            if (!valid) { OS_FileClose(&file->handle); }
        }

        assets->files = M_ArenaPushCopy(arena, files, AssetFile, assets->num_files);

        if (assets->importer.enabled) {
            if (assets->max_assets == 0) {
                // like above we choose some max number of assets for the user to import if
                // they don't specify an amount for themselves
                //
                assets->max_assets = 1024;
            }
        }
    }

    B32 new_file = false;
    if (assets->num_files == 0) {
        // in this case we either don't have any packed files to load from disk, or all of the packed
        // files we did find were invalid. if the importer is enabled, make a new file, otherwise continue
        // without files
        //
        if (assets->importer.enabled) {
            assets->num_files = 1;
            assets->files     = M_ArenaPush(arena, AssetFile, assets->num_files);

            AssetFile *file = &assets->files[0];

            // so we can just choose the name of the asset file
            //
            Str8 imported = Str8_Format(temp, "%.*s/imported.xia", Str8_Arg(path));
            file->handle  = OS_FileOpen(imported, file_access);

            // completely empty file header, but valid to read
            //
            file->header.signature      = XIA_HEADER_SIGNATURE;
            file->header.version        = XIA_HEADER_VERSION;
            file->header.num_assets     = 0;
            file->header.str_table_size = 0;
            file->header.info_offset    = sizeof(Xia_Header);

            file->base_asset        = assets->num_assets; // should == 2
            file->next_write_offset = file->header.info_offset;
            file->num_assets        = 0;

            new_file = true;

            if (assets->max_assets == 0) {
                // some number of assets to import into if the user doesn't specify their own amount
                //
                assets->max_assets = 1024;
            }

            file->strings.used  = 0;
            file->strings.limit = assets->max_assets * (U8_MAX + 1);
            file->strings.data  = M_ArenaPush(arena, U8, file->strings.limit);
        }

        if (!new_file) {
            assets->num_files = 0;
            assets->files     = 0;
        }
    }


    if (assets->max_assets < total_assets) {
        // we make sure there are at least as many assets as we found, the
        // user can override this count to allow assets to be imported
        //
        assets->max_assets = total_assets;
    }

    assets->xias   = M_ArenaPush(arena, Xia_AssetInfo, assets->max_assets);
    assets->assets = M_ArenaPush(arena, Asset,       assets->max_assets);

    assets->lookup_table_count = U32_Pow2Nearest(assets->max_assets) << 1;
    assets->lookup_table       = M_ArenaPush(arena, AssetHash, assets->lookup_table_count);

    assets->max_texture_handles = xi->renderer.texture_limit;
    assets->max_sprite_handles  = xi->renderer.sprite_array.limit;
    assets->next_sprite         = 2;
    assets->next_texture        = 0;

    assets->total_loads = 0;
    assets->next_load   = 0;
    assets->asset_loads = M_ArenaPush(arena, AssetLoadInfo, assets->max_assets);

    // load the asset information
    //
    if (!new_file) {
        U32 running_total = assets->num_assets; // some assets are reserved up front
        for (U32 it = 0; it < assets->num_files; ++it) {
            AssetFile *file = &assets->files[it];

            // read the asset information
            //
            U32 count  = file->header.num_assets;
            U64 offset = file->header.info_offset;
            U64 size   = count * sizeof(Xia_AssetInfo);

            Xia_AssetInfo *xias = &assets->xias[assets->num_assets];
            if (!OS_FileRead(&file->handle, xias, offset, size)) { continue; }

            // read the string table, comes directly after the asset info array
            //
            offset += size;

            size = file->header.str_table_size;
            if (assets->importer.enabled && (it + 1) == assets->num_files) {
                // we always import to the final file, so if the importer is enabled give it the max
                // amount of space for strings so we can add more to it later when importing new assets
                //
                size = assets->max_assets * (U8_MAX + 1);
            }

            U8 *strings = M_ArenaPush(arena, U8, size);
            if (!OS_FileRead(&file->handle, strings, offset, file->header.str_table_size)) {
                M_ArenaPopLast(arena);
                continue;
            }

            file->strings.limit = size;
            file->strings.used  = file->header.str_table_size;
            file->strings.data  = strings;

            for (U32 ai = 0; ai < count; ++ai) {
                Asset         *asset = &assets->assets[running_total + ai];
                Xia_AssetInfo *xia   = &xias[ai];

                asset->state = ASSET_STATE_UNLOADED;
                asset->index = ai;
                asset->type  = xia->type;
                asset->file  = it;

                if (xia->name_offset != U32_MAX) {
                    // :asset_lookup insert into hash table
                    //
                    Str8 name;
                    name.count =  strings[xia->name_offset];
                    name.data  = &strings[xia->name_offset + 1];

                    AssetManagerHashInsert(assets, asset->type, running_total + ai, name);
                }
            }

            assets->num_assets += count;
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
        U32 w = 256;
        U32 h = 256;

        Asset *error = &assets->assets[0];

        error->state = ASSET_STATE_LOADED;
        error->type  = XIA_ASSET_TYPE_IMAGE;
        error->index = 0;
        error->file  = cast(U32) -1; // invalid, as its not part of a file. setting this to -1 will catch bugs

        error->data.texture.index  = 0;
        error->data.texture.width  = cast(U16) w;
        error->data.texture.height = cast(U16) h;

        Xia_AssetInfo *error_xia = &assets->xias[0];

        error_xia->offset      = 0;
        error_xia->size        = 0;
        error_xia->name_offset = 0;
        error_xia->type        = XIA_ASSET_TYPE_IMAGE;

        error_xia->image.width       = w;
        error_xia->image.height      = h;
        error_xia->image.frame_count = 0;
        error_xia->image.mip_levels  = 0;

        error_xia->write_time = 0;

        void *data = 0;
        U64   size = MipMappedImageSize(w, h);

        RendererTransferTask *task = RendererTransferQueueEnqueueSize(assets->transfer_queue, &data, size);

        Assert(data != 0);

        U32 colour = 0xFF000000;

        U32 *base = data;
        while (w > 1 || h > 1) {
            U32 pps = 1;
            if (w > 4 || h > 4) { pps = w / 4; }

            for (U32 y = 0; y < h; ++y) {
                if ((y % pps) == 0) { colour ^= 0x00FF00FF; } // force rows to start with different colours

                for (U32 x = 0; x < w; ++x) {
                    if ((x % pps) == 0) { colour ^= 0x00FF00FF; }

                    U32 px = (y * w) + x;
                    base[px] = colour;
                }
            }

            base += (w * h); // already a U32 so no need for 4x

            w >>= 1;
            h >>= 1;
        }

        task->texture    = error->data.texture;
        task->mip_levels = 8;
        task->state      = RENDERER_TRANSFER_TASK_STATE_LOADED;
    }

    // upload the solid white texture
    //
    {
        // :default_texture_bandwidth
        //
        U32 w = 256;
        U32 h = 256;

        Asset *white = &assets->assets[1];

        white->state = ASSET_STATE_LOADED;
        white->type  = XIA_ASSET_TYPE_IMAGE;
        white->index = 1;
        white->file  = cast(U32) -1; // invalid, as its not part of a file. setting this to -1 will catch bugs

        white->data.texture.index  = 1;
        white->data.texture.width  = cast(U16) w;
        white->data.texture.height = cast(U16) h;

        Xia_AssetInfo *white_xia = &assets->xias[1];

        white_xia->offset      = 0;
        white_xia->size        = 0;
        white_xia->name_offset = 0;
        white_xia->type        = XIA_ASSET_TYPE_IMAGE;

        white_xia->image.width       = w;
        white_xia->image.height      = h;
        white_xia->image.frame_count = 0;
        white_xia->image.mip_levels  = 0;

        white_xia->write_time = 0;

        void *data = 0;
        U64   size = MipMappedImageSize(w, h);

        RendererTransferTask *task = RendererTransferQueueEnqueueSize(assets->transfer_queue, &data, size);
        Assert(data != 0);

        MemorySet(data, 0xFF, size);

        task->texture    = white->data.texture;
        task->mip_levels = 8;
        task->state      = RENDERER_TRANSFER_TASK_STATE_LOADED;
    }

    if (assets->sample_buffer.limit == 0) {
        // to be on the safe side
        //
        // @incomplete: THIS WILL NOT WORK PROPERLY, IT SHOULD BE CHANGED ANYWAY BECAUSE THE WHOLE
        // SYSTEM IT BAAAAAAAAAAAAAAAAAD!!!!!!!!!
        //
        assets->sample_buffer.limit = MB(128);
    }

    assets->sample_buffer.used = 0;
    assets->sample_buffer.data = M_ArenaPush(arena, U8, assets->sample_buffer.limit);

    assets->sample_rate = xi->audio_player.sample_rate;
    Assert(assets->sample_rate > 0);

    xi->renderer.assets = assets;

    if (assets->importer.enabled) {
        // we make a copy of the user set strings to make sure we own the memory of them
        //
        assets->importer.search_dir    = Str8_PushCopy(arena, assets->importer.search_dir);
        assets->importer.sprite_prefix = Str8_PushCopy(arena, assets->importer.sprite_prefix);

        AssetManagerImportToXia(assets);
    }
}

//
// :note loading assets
//
FileScope THREAD_TASK_PROC(AssetDataLoad) {
    (void) pool;

    AssetLoadInfo *load = cast(AssetLoadInfo *) arg;

    AssetManager *assets = load->assets;
    AssetFile    *file   = load->file;
    Asset        *asset  = load->asset;

    if (OS_FileRead(&file->handle, load->data, load->offset, load->size)) {
        if (load->transfer_task) {
            // mark the task as loaded so it can be uploaded to the gpu later on
            //
            U32AtomicExchange(&load->transfer_task->state, RENDERER_TRANSFER_TASK_STATE_LOADED);
        }

        U32AtomicExchange(&asset->state, ASSET_STATE_LOADED);
    }
    else {
        U32AtomicExchange(&asset->state, ASSET_STATE_UNLOADED);
        if (load->transfer_task) {
            // mark the transfer task as unused to signify we shouldn't copy from this as it failed to
            // load
            //
            U32AtomicExchange(&load->transfer_task->state, RENDERER_TRANSFER_TASK_STATE_UNUSED);
        }
    }

    U32AtomicDecrement(&assets->total_loads);
}

FileScope RendererTexture RendererTextureAcquire(AssetManager *assets, U32 width, U32 height) {
    RendererTexture result;

    if (width > assets->sprite_dimension || height > assets->sprite_dimension) {
        // can't be a sprite, otherwise our asset packer would've resized it for us
        //
        result.index = assets->next_texture++;

        // :renderer_handles
        //
        Assert(assets->next_texture <= assets->max_texture_handles);
    }
    else {
        result.index  = assets->next_sprite++;

        // :renderer_handles
        //
        Assert(assets->next_sprite <= assets->max_sprite_handles);
    }

    // @todo: error checking if width or height > U16_MAX, maybe even request from the renderer the
    // max supported texture size and limit on that, gpus don't currently support textures this large anyway
    //
    result.width  = cast(U16) width;
    result.height = cast(U16) height;

    return result;
}

// ---- Asset Loading Calls
//

void ImageLoad(AssetManager *assets, ImageHandle image) {
    if (assets->total_loads < assets->max_assets) {
        if (image.value < assets->num_assets) {
            Asset *asset = &assets->assets[image.value];
            if (asset->state != ASSET_STATE_LOADED) {
                Xia_AssetInfo *xia = &assets->xias[image.value];

                // it is a bug if an image handle is passed to this function and the asset type is not
                // an image so we should assert this to catch bugs
                //
                Assert(xia->type == XIA_ASSET_TYPE_IMAGE);

                asset->data.texture = RendererTextureAcquire(assets, xia->image.width, xia->image.height);

                AssetLoadInfo *asset_load  = &assets->asset_loads[assets->next_load++];
                assets->next_load         %= assets->max_assets;

                RendererTransferQueue *transfer_queue = assets->transfer_queue;

                asset->state = ASSET_STATE_PENDING;

                asset_load->assets = assets;
                asset_load->file   = &assets->files[asset->file];
                asset_load->asset  = asset;
                asset_load->offset = xia->offset;
                asset_load->size   = xia->size;

                RendererTransferTask *task = RendererTransferQueueEnqueueSize(transfer_queue, &asset_load->data, asset_load->size);
                asset_load->transfer_task  = task;

                if (task != 0) {
                    task->texture        = asset->data.texture;
                    task->mip_levels     = xia->image.mip_levels;
                    assets->total_loads += 1;

                    ThreadPoolEnqueue(assets->thread_pool, AssetDataLoad, cast(void *) asset_load);
                }
            }
        }
    }
}

void SoundLoad(AssetManager *assets, SoundHandle sound) {
    if (assets->total_loads < assets->max_assets) {
        if (sound.value < assets->num_assets) {
            Asset *asset = &assets->assets[sound.value];
            if (asset->state != ASSET_STATE_LOADED) {
                Xia_AssetInfo *xia = &assets->xias[sound.value];

                // it is a bug if a sound handle is passed to this function and the asset type is not
                // a sound so we should assert this to catch bugs
                //
                Assert(xia->type == XIA_ASSET_TYPE_SOUND);

                U64 remaining = (assets->sample_buffer.limit - assets->sample_buffer.used);
                if (remaining >= xia->size) {
                    // @todo: this is BAD! better sound loading
                    //
                    asset->data.sample_base     = assets->sample_buffer.used;
                    assets->sample_buffer.used += xia->size;

                    AssetLoadInfo *asset_load  = &assets->asset_loads[assets->next_load++];
                    assets->next_load         %= assets->max_assets;

                    asset->state = ASSET_STATE_PENDING;

                    asset_load->assets = assets;
                    asset_load->file   = &assets->files[asset->file];
                    asset_load->asset  = asset;
                    asset_load->data   = &assets->sample_buffer.data[asset->data.sample_base];
                    asset_load->offset = xia->offset;
                    asset_load->size   = xia->size;

                    assets->total_loads += 1;

                    ThreadPoolEnqueue(assets->thread_pool, AssetDataLoad, cast(void *) asset_load);
                }
            }
        }
    }
}

FileScope U32 AssetIndexGetByName(AssetManager *assets, Str8 name, Xia_AssetType type) {
    U32 result;

    // @todo: pass the xia as a parameter here instead of just type and or in the
    // frame count to the mix for images?
    //
    // this will allow a still image and an animation to have the same name without requiring
    // a whole new type and if the frame count is zero or does nothing
    //

    U32 mask = assets->lookup_table_count - 1;
    U32 mix  = tbl_xia_type_hash_mix[type];
    U32 hash = Str8_Fnv1aHashU32(name) ^ mix;

    U32 i     = 0;
    B32 found = false;

    do {
        if (i > assets->lookup_table_count) {
            // we have tried every slot and didn't find anything so bail
            //
            result = 0;
            break;
        }

        // quadratic probing
        //
        U32 table_index = (hash + (i * i)) & mask;
        AssetHash *slot = &assets->lookup_table[table_index];

        // :asset_slot
        //
        result = slot->index;
        found  = (slot->hash == hash) && Str8_Equal(slot->value, name, 0);

        i += 1;
    }
    while (!found && result);

    return result;
}

// ---- Asset acquisition
//

ImageHandle ImageGetByName(AssetManager *assets, Str8 name) {
    ImageHandle result;

    result.value = AssetIndexGetByName(assets, name, XIA_ASSET_TYPE_IMAGE);
    return result;
}

SoundHandle SoundGetByName(AssetManager *assets, Str8 name) {
    SoundHandle result;

    result.value = AssetIndexGetByName(assets, name, XIA_ASSET_TYPE_SOUND);
    return result;
}

SpriteAnimation SpriteAnimationGetByName(AssetManager *assets, Str8 name, SpriteAnimationFlags flags) {
    ImageHandle image      = ImageGetByName(assets, name);
    SpriteAnimation result = SpriteAnimationFromImage(assets, image, flags);
    return result;
}

SpriteAnimation SpriteAnimationFromImage(AssetManager *assets, ImageHandle image, SpriteAnimationFlags flags) {
    SpriteAnimation result = { 0 };

    result.base_frame = image;
    result.flags      = flags;
    result.frame_dt   = assets->animation_dt;

    // @todo: don't mandate frame_count, static images can technically work as 'single frame animations'
    // so nothing will break if we just do that, it simplifies this a lot
    //
    Xia_ImageInfo *info = ImageInfoGet(assets, image);
    if (info) {
        Assert(info->frame_count > 0);

        result.frame_count = info->frame_count;
        if (flags & SPRITE_ANIMATION_FLAG_REVERSE) {
            result.current_frame = info->frame_count - 1; // -1 as zero based!
        }
    }

    for (U32 it = 0; it < result.frame_count; ++it) {
        ImageHandle handle = { image.value + it };
        ImageLoad(assets, handle); // we prefetch the frames
    }

    return result;
}

// ---- Asset Info
//

Xia_ImageInfo *ImageInfoGet(AssetManager *assets, ImageHandle image) {
    Xia_ImageInfo *result = &assets->xias[image.value].image;
    return result;
}

Xia_SoundInfo *SoundInfoGet(AssetManager *assets, SoundHandle sound) {
    Xia_SoundInfo *result = &assets->xias[sound.value].sound;
    return result;
}

Xia_ImageInfo *SpriteAnimationInfoGet(AssetManager *assets, SpriteAnimation *animation) {
    Xia_ImageInfo *result = ImageInfoGet(assets, animation->base_frame);
    return result;
}

// ---- Asset Data
//

RendererTexture ImageDataGet(AssetManager *assets, ImageHandle image) {
    RendererTexture result = assets->assets[0].data.texture; // always valid

    Asset *asset = &assets->assets[image.value];
    Assert(asset->type == XIA_ASSET_TYPE_IMAGE); // is a bug to pass image handle that isn't an image

    if (asset->state == ASSET_STATE_UNLOADED) {
        // happens here to a) make sure the texture handle is valid, and b) we can't rely on
        // get_by_name to load the asset due to certain paths such as animations etc.
        //
        ImageLoad(assets, image);
    }

    result = asset->data.texture;
    return result;
}

S16 *SoundDataGet(AssetManager *assets, SoundHandle sound) {
    S16 *result = 0;

    Asset *asset = &assets->assets[sound.value];
    Assert(asset->type == XIA_ASSET_TYPE_SOUND);

    if (asset->state == ASSET_STATE_UNLOADED) {
        SoundLoad(assets, sound);
    }

    result = cast(S16 *) &assets->sample_buffer.data[asset->data.sample_base];
    return result;
}

ImageHandle SpriteAnimationFrameGet(SpriteAnimation *animation) {
    ImageHandle result;

    result.value = animation->base_frame.value + animation->current_frame;
    return result;
}

// ---- Other animation functions
//

B32 SpriteAnimationUpdate(SpriteAnimation *animation, F32 dt) {
    B32 result = false;

    B32 reverse    = (animation->flags & SPRITE_ANIMATION_FLAG_REVERSE);
    U32 last_frame = reverse ? 0 : animation->frame_count - 1;

    if (animation->flags & SPRITE_ANIMATION_FLAG_ONE_SHOT) {
        // we ony return 'true' for one-shot animations once, this is handled below when the animation
        // actually transitions to this last frame
        //
        if (animation->current_frame == last_frame) { return result; }
    }
    else if (animation->flags & SPRITE_ANIMATION_FLAG_PAUSED) {
        return result;
    }

    animation->frame_accum += dt;

    if (animation->frame_accum >= animation->frame_dt) {
        animation->frame_accum -= animation->frame_dt;

        // check if we are at the end of the animation
        //
        if (animation->current_frame == last_frame) {
            if (animation->flags & SPRITE_ANIMATION_FLAG_PING_PONG) {
                animation->flags ^= SPRITE_ANIMATION_FLAG_REVERSE;
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

F32 SpriteAnimationPlaybackPercentageGet(SpriteAnimation *animation) {
    F32 result;

    U32 frames = animation->frame_count - 1; // zero based to make it align with 1.0 at 100%

    if (animation->flags & SPRITE_ANIMATION_FLAG_REVERSE) {
        result = (frames - animation->current_frame) / cast(F32) frames;
    }
    else {
        result = animation->current_frame / cast(F32) frames;
    }

    return result;
}

void SpriteAnimationReset(SpriteAnimation *animation) {
    U32 first_frame = (animation->flags & SPRITE_ANIMATION_FLAG_REVERSE) ? animation->frame_count - 1 : 0;
    animation->current_frame = first_frame;
}

void SpriteAnimationPause(SpriteAnimation *animation) {
    animation->flags |= SPRITE_ANIMATION_FLAG_PAUSED;
}

void SpriteAnimationResume(SpriteAnimation *animation) {
    animation->flags &= ~SPRITE_ANIMATION_FLAG_PAUSED;
}

//
// --------------------------------------------------------------------------------
// :Importer
// --------------------------------------------------------------------------------
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
FileScope U32 AssetHandleReserveCount(AssetManager *assets, U32 count) {
    U32 result;

    U32 next;
    do {
        result = assets->num_assets;
        next   = result + count;

        if (next > assets->max_assets) {
            result = 0;
            break;
        }
    } while (!U32AtomicCompareExchange(&assets->num_assets, next, result));

    return result;
}

// reserves the size specified in the asset pack file, will return the offset into the file in which it
// is valid to write that data to. assets are always appended to the last valid file opened
//
FileScope U64 AssetManagerSizeReserve(AssetManager *assets, U64 size) {
    U64 result;

    AssetFile *file = &assets->files[assets->num_files - 1];

    result = U64AtomicAdd(&file->next_write_offset, size);
    return result;
}

FileScope Str8 AssetNameAppend(AssetFile *file, Xia_AssetInfo *info, Str8 name) {
    Str8 result = { 0 };

    Assert(name.count <= U8_MAX);

    // we don't need to check if there is enough space left in the strings buffer as we allocated
    // enough space to store 'max_assets' worth of max name length data
    //
    U64 offset = U64AtomicAdd((volatile U64 *) &file->strings.used, name.count + 1);
    U8  *data  = &file->strings.data[offset];

    // @todo: maybe a secondary pass to insert the string names single-threaded would be a good
    // idea, we could do a validation to see if packed assets were actually written to the file
    // etc. as well!
    //
    data[0] = cast(U8) name.count;
    MemoryCopy(&data[1], name.data, name.count);

    info->name_offset = cast(U32) offset;

    result = Str8_WrapCount(&data[1], name.count);
    return result;
}

U64 MipMappedImageSize(U32 w, U32 h) {
    U64 result = (w * h);

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

typedef struct AssetImportInfo AssetImportInfo;
struct AssetImportInfo {
    U32 handle;      // if this is != 0, we have pre-allocated a handle for the asset
    S32 frame_index; // valid if >= 0, signifies its an animation frame

    AssetManager   *assets;
    DirectoryEntry *entry;
};

FileScope THREAD_TASK_PROC(ImageAssetImport) {
    (void) pool;

    AssetImportInfo *import = cast(AssetImportInfo *) arg;
    AssetManager    *assets = import->assets;

    M_Arena *temp = M_TempGet();

    DirectoryEntry *entry = import->entry;

    Str8 file_data = FileReadAll(temp, entry->path);
    if (file_data.count) {
        int w, h, c;
        stbi_uc *image_data = stbi_load_from_memory(file_data.data, (int) file_data.count, &w, &h, &c, 4);

        if (image_data) {
            U32 handle = import->handle;
            if (handle == 0) {
                // reserve a single asset
                //
                handle = AssetHandleReserveCount(assets, 1);
            }

            if (handle != 0) {
                AssetFile *file = &assets->files[assets->num_files - 1];

                U8 *pixels      = image_data;
                U8  level_count = 0;
                U64 total_size  = 4 * w * h;

                Str8 basename    = Str8_RemoveAfterLast(entry->basename, '.');
                B32  is_animated = (import->frame_index >= 0);
                Str8 prefix      = Str8_Prefix(basename, assets->importer.sprite_prefix.count);
                B32  is_sprite   = Str8_Equal(prefix, assets->importer.sprite_prefix, 0);

                if (is_sprite) {
                    // we advance past the prefix so the actual lookup name doesn't
                    // require it
                    //
                    basename = Str8_Advance(basename, prefix.count);
                }
                else if (is_animated && (import->frame_index == 0)) {
                    // we only do this for the first animation frame as subsequent animation frames cannot
                    // be looked up directly so we don't need to insert them into the asset lookup table
                    //
                    basename = Str8_RemoveAfterLast(basename, '_');
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

                U8 *base = image_data;
                for (S32 y = 0; y < h; ++y) {
                    for (S32 x = 0; x < w; ++x) {
                        // Get the colour components
                        //
                        Vec4F linear;
                        linear.rgb = V3F(base[0], base[1], base[2]);
                        linear.a   = (c == 3) ? 255.0f : base[3];

                        // Convert to light linear space and pre-multiply the alpha
                        //
                        linear     = V4F_Linear1FromSRGB255(linear);
                        linear.xyz = V3F_Scale(linear.xyz, linear.a);

                        // Convert back to SRGB and write back to image data
                        //
                        Vec4F srgb = V4F_SRGB255FromLinear1(linear);

                        base[0] = cast(U8) srgb.r;
                        base[1] = cast(U8) srgb.g;
                        base[2] = cast(U8) srgb.b;

                        base += 4;
                    }
                }

                // now we have our image as 'pre-multiplied srgb' we can downsize if as
                // needed to fit in the texture array if it is a sprite
                //
                if (is_sprite || is_animated) {
                    if (w > cast(S32) assets->sprite_dimension ||
                        h > cast(S32) assets->sprite_dimension)
                    {
                        F32 ratio = assets->sprite_dimension / cast(F32) Max(w, h);

                        int new_w = (int) (ratio * w); // trunc or round?
                        int new_h = (int) (ratio * h); // trunc or round?

                        total_size = MipMappedImageSize(new_w, new_h);
                        pixels     = M_ArenaPush(temp, U8, total_size);

                        Assert(new_w <= cast(S32) assets->sprite_dimension);
                        Assert(new_h <= cast(S32) assets->sprite_dimension);

                        stbir_resize_uint8_srgb(image_data, w, h, 0,
                                pixels, new_w, new_h, 0, 4, 3, STBIR_FLAG_ALPHA_PREMULTIPLIED);

                        w = new_w;
                        h = new_h;
                    }
                    else {
                        // we don't need to resize the base image but we need to generate
                        // mipmaps
                        //
                        total_size = MipMappedImageSize(w, h);
                        pixels     = M_ArenaPush(temp, U8, total_size);

                        // copy the base image data into the new buffer
                        //
                        MemoryCopy(pixels, image_data, 4 * w * h);
                    }

                    // generate mip maps
                    //
                    // @todo: we can just do this via the graphics api with a blit command rather
                    // than wasting the space in the asset file :mip_map_gen
                    //
                    U8 *cur   = pixels;
                    U32 cur_w = w;
                    U32 cur_h = h;

                    U8 *next   = cur + (4 * cur_w * cur_h);
                    U32 next_w = cur_w >> 1;
                    U32 next_h = cur_h >> 1;

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

                    Assert(next == (pixels + total_size));
                }
                else {
                    // @copypaste: from above just for testing, mip-maps for non-sprite images,
                    // just do this in gpu api with blit operations!!!
                    //
                    // if requested! :mip_map_gen
                    //
                    total_size = 4 * w * h; // MipMappedImageSize(w, h);
                    pixels     = M_ArenaPush(temp, U8, total_size);

                    // copy the base image data into the new buffer
                    //
                    MemoryCopy(pixels, image_data, 4 * w * h);

#if 0
                    // generate mip maps
                    //
                    U8 *cur   = pixels;
                    U32 cur_w = w;
                    U32 cur_h = h;

                    U8 *next   = cur + (4 * cur_w * cur_h);
                    U32 next_w = cur_w >> 1;
                    U32 next_h = cur_h >> 1;

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

                    Assert(next == (pixels + total_size));
#endif
                }

                Asset         *asset = &assets->assets[handle];
                Xia_AssetInfo *xia   = &assets->xias[handle];

                asset->state = ASSET_STATE_UNLOADED;
                asset->type  = XIA_ASSET_TYPE_IMAGE;
                asset->index = handle - file->base_asset;
                asset->file  = assets->num_files - 1;

                // 4GiB per asset
                //
                Assert(total_size <= U32_MAX);

                xia->type        = asset->type;
                xia->size        = cast(U32) total_size;
                xia->offset      = AssetManagerSizeReserve(assets, xia->size);
                xia->write_time  = entry->time;

                // @todo: 'name_offset' being a U32 artifically limits the file size to just
                // under 4GiB maybe this should just be a U64 instead
                //
                xia->name_offset = U32_MAX;

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
                    Str8 name = AssetNameAppend(file, xia, basename);
                    AssetManagerHashInsert(assets, asset->type, handle, name);
                }

                // @todo: this assumes we successfully wrote to the file
                //
                // i don't know what the best option for write failure here is, it currently
                // means there will be a hole in the file, but the asset lookup will remain
                // vaild etc.
                //
                OS_FileWrite(&file->handle, pixels, xia->offset, xia->size);

                file->modified = true;
                U32AtomicIncrement(&file->num_assets);
            }

            free(image_data);
        }
    }
}

//
// wav importing
//
#pragma pack(push, 1)
typedef struct Wav_Header Wav_Header;
struct Wav_Header {
    U32 riff_code;
    U32 size;
    U32 wave_code;
};

#define WAV_PCM_FORMAT (0x0001)

#define RIFF_CODE_RIFF FourCC('R', 'I', 'F', 'F')
#define RIFF_CODE_WAVE FourCC('W', 'A', 'V', 'E')
#define RIFF_CODE_FMT  FourCC('f', 'm', 't', ' ')
#define RIFF_CODE_DATA FourCC('d', 'a', 't', 'a')

typedef struct Wav_Chunk Wav_Chunk;
struct Wav_Chunk {
    U32 code;
    U32 size;
};

typedef struct Wav_FmtChunk Wav_FmtChunk;
struct Wav_FmtChunk {
    U16 format;
    U16 channels;
    U32 sample_rate;
    U32 data_rate;
    U16 block_align;
    U16 bit_depth;
    U16 ext_size;
    U16 valid_bits;
    U32 channel_mask;
    U8  sub_format[16];
};

#pragma pack(pop)

FileScope THREAD_TASK_PROC(SoundAssetImport) {
    (void) pool;

    AssetImportInfo *import = cast(AssetImportInfo *) arg;
    AssetManager    *assets = import->assets;

    M_Arena *temp = M_TempGet();

    DirectoryEntry *entry = import->entry;

    Str8 file_data = FileReadAll(temp, entry->path);
    if (file_data.count) {
        Wav_Header *header = cast(Wav_Header *) file_data.data;
        if (header->riff_code == RIFF_CODE_RIFF &&
            header->wave_code == RIFF_CODE_WAVE)
        {
            U8 *data  = file_data.data;
            U32 total = header->size;
            U32 used  = sizeof(Wav_Header);

            Wav_FmtChunk *fmt = 0;
            U32 samples_size = 0;
            U8  *samples     = 0;

            while (used < total) {
                Wav_Chunk *chunk = cast(Wav_Chunk *) (data + used);
                switch (chunk->code) {
                    case RIFF_CODE_FMT: {
                        fmt = cast(Wav_FmtChunk *) (chunk + 1);
                    }
                    break;
                    case RIFF_CODE_DATA: {
                        samples_size = chunk->size;
                        samples      = cast(U8 *) (chunk + 1);
                    }
                    break;
                }

                used += sizeof(Wav_Chunk);
                used += chunk->size;
            }

            if (fmt && samples && samples_size > 0) {
                B32 matches = (fmt->format == WAV_PCM_FORMAT) && (fmt->channels == 2) &&
                    (fmt->sample_rate == assets->sample_rate) && (fmt->bit_depth == 16);

                // @todo: We should chunk sounds if they are deemed to be 'too large'
                //

                if (matches) {
                    U32 handle = AssetHandleReserveCount(assets, 1);
                    if (handle != 0) {
                        AssetFile     *file  = &assets->files[assets->num_files - 1];
                        Asset         *asset = &assets->assets[handle];
                        Xia_AssetInfo *xia   = &assets->xias[handle];

                        asset->state = ASSET_STATE_UNLOADED;
                        asset->type  = XIA_ASSET_TYPE_SOUND;
                        asset->index = handle - file->base_asset;
                        asset->file  = assets->num_files - 1;

                        // 4GiB per asset, we couldn't even load a sound this big anyway
                        //
                        Assert(samples_size <= U32_MAX);

                        xia->type       = asset->type;
                        xia->size       = cast(U32) samples_size;
                        xia->offset     = AssetManagerSizeReserve(assets, xia->size);
                        xia->write_time = entry->time;

                        // @todo: 'name_offset' being a U32 artifically limits the file size to just
                        // under 4GiB maybe this should just be a U64 instead
                        //
                        xia->name_offset = U32_MAX;

                        xia->sound.num_channels = fmt->channels;
                        xia->sound.num_samples  = samples_size / (fmt->bit_depth >> 3);

                        Str8 basename = Str8_RemoveAfterLast(entry->basename, '.');
                        Str8 name     = AssetNameAppend(file, xia, basename);

                        AssetManagerHashInsert(assets, asset->type, handle, name);

                        OS_FileWrite(&file->handle, samples, xia->offset, xia->size);

                        file->modified = true;
                        U32AtomicIncrement(&file->num_assets);
                    }
                }
            }
        }
    }
}

void AssetManagerImportToXia(AssetManager *assets) {
    if (assets->num_files == 0) {
        // something went wrong during init causing no files to be allocated, just disable the importer
        // and bail
        //
        assets->importer.enabled = false;
        return;
    }

    M_Arena *temp = M_TempGet();

    DirectoryList list = DirectoryListGet(temp, assets->importer.search_dir, false);

    U32 next_import = 0;
    AssetImportInfo *imports = M_ArenaPush(temp, AssetImportInfo, assets->max_assets);

    for (U32 it = 0; it < list.num_entries; ++it) {
        DirectoryEntry *entry = &list.entries[it];

        if (entry->type == DIRECTORY_ENTRY_TYPE_DIRECTORY) {
            // assume its an animation because it has its own sub-directory
            //
            // @test: see if its okay to clobber the input list by supplying the input and output as the
            // same list, could be useful when you're only filtering once and don't care about the overall list
            //
            // @todo: this should be its own threaded task which then in turn enqueues its own image import
            // task for each frame of the animation
            //
            DirectoryList all    = DirectoryListGet(temp, entry->path, false);
            DirectoryList frames = DirectoryListFilterForSuffix(temp, &all, Str8_Literal(".png"));

            if (frames.num_entries > 0) {
                B32 needs_packing;
                {
                    // check if the animation has already been packed into the asset file, if it has
                    // then we don't need to import it again...
                    //
                    // @todo: this doesn't check if any of the frames have been modified (newer write time)
                    // but it should check incase there have been file modifications
                    //
                    DirectoryEntry *first = &frames.entries[0];

                    Str8        name  = Str8_RemoveAfterLast(first->basename, '_');
                    ImageHandle image = ImageGetByName(assets, name);
                    needs_packing     = (image.value == 0);
                }

                if (needs_packing) {
                    // assume there are count many frames
                    //
                    U32 *map = M_ArenaPush(temp, U32, frames.num_entries);

                    U32 frame_count = 0;
                    for (U32 f = 0; f < frames.num_entries; ++f) {
                        DirectoryEntry *frame = &frames.entries[f];

                        Str8 number     = Str8_RemoveBeforeLast(Str8_RemoveAfterLast(frame->basename, '.'), '_');
                        S64 frame_index = Str8_ParseInteger(number);

                        Assert(frame_index >= 0 && frame_index < frames.num_entries);

                        map[frame_index]  = f;
                        frame_count      += 1;
                    }

                    if (frame_count == frames.num_entries) {
                        // we have a valid animation and all frames are present
                        //
                        U32 base_handle = AssetHandleReserveCount(assets, frame_count);
                        if (base_handle != 0) {
                            // @hack: this entire animation import needs to be pulled out into its
                            // own routine ideally, so we can leverage a different thread to do this
                            // stuff for us, really all this loop should be is pushing 'asset_import' onto
                            // the thread queue
                            //
                            Xia_AssetInfo *base     = &assets->xias[base_handle];
                            base->image.frame_count = cast(U8) frame_count;

                            for (U32 f = 0; f < frame_count; ++f) {
                                Assert(next_import < assets->max_assets);

                                AssetImportInfo *import = &imports[next_import];
                                next_import += 1;

                                import->frame_index = cast(S32) f;

                                import->assets = assets;
                                import->entry  = &frames.entries[map[f]];

                                import->handle = base_handle + f;

                                ThreadPoolEnqueue(assets->thread_pool, ImageAssetImport, cast(void *) import);
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
        else if (Str8_EndsWith(entry->basename, Str8_Literal(".png"))) {
            // we have an image asset!
            //
            // @incomplete: this will break if there are more assets to import than max assets were
            // allocated! come up with some sort of ring buffer design to use the imports
            //
            Assert(next_import < assets->max_assets);

            // check if it is a sprite
            //
            // @duplicate: from above in the actual image import code, maybe this can happen
            // there?
            //
            Str8 basename = Str8_RemoveAfterLast(entry->basename, '.');
            Str8 prefix   = Str8_Prefix(basename, assets->importer.sprite_prefix.count);

            if (Str8_Equal(prefix, assets->importer.sprite_prefix, 0)) {
                basename = Str8_Advance(basename, prefix.count);
            }

            // @todo: this needs to import assets that have changed!
            //

            ImageHandle image = ImageGetByName(assets, basename);
            if (image.value == 0) {
                AssetImportInfo *import = &imports[next_import];
                next_import += 1;

                import->assets      =  assets;
                import->entry       =  entry;
                import->handle      =  0; // allocated automatically
                import->frame_index = -1;

                ThreadPoolEnqueue(assets->thread_pool, ImageAssetImport, cast(void *) import);
            }
        }
        else if (Str8_EndsWith(entry->basename, Str8_Literal(".wav"))) {
            // this assumes that the audio player has been configured to the sample-rate that
            // the games assest will be using
            //
            Str8        name  = Str8_RemoveAfterLast(entry->basename, '.');
            SoundHandle sound = SoundGetByName(assets, name);

            if (sound.value == 0) {
                AssetImportInfo *import = &imports[next_import];
                next_import += 1;

                import->assets = assets;
                import->entry  = entry;
                import->handle = 0; // allocated automatically

                ThreadPoolEnqueue(assets->thread_pool, SoundAssetImport, cast(void *) import);
            }
        }
    }

    ThreadPoolAwaitComplete(assets->thread_pool);

    for (U32 it = 0; it < assets->num_files; ++it) {
        AssetFile *file = &assets->files[it];
        if (file->modified) {
            Xia_Header *header = &file->header;

            // update the asset count, info offset and size of the string table
            //
            header->num_assets     = file->num_assets;
            header->info_offset    = file->next_write_offset;
            header->str_table_size = file->strings.used;

            // write out header
            //
            OS_FileWrite(&file->handle, header, 0, sizeof(Xia_Header));

            // write out asset info
            //
            U64 info_size = header->num_assets * sizeof(Xia_AssetInfo);
            OS_FileWrite(&file->handle, &assets->xias[file->base_asset], header->info_offset, info_size);

            // write out string table
            //
            U64 str_table_offset = header->info_offset + info_size;
            OS_FileWrite(&file->handle, file->strings.data, str_table_offset, header->str_table_size);

            file->modified = false;
        }
    }
}
