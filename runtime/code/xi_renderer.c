#define RenderCommandPush(renderer, type) \
    RenderCommandPushSize(renderer, RENDER_COMMAND_##type, sizeof(type))

FileScope void *RenderCommandPushSize(RendererContext *renderer, U32 type, U64 size) {
    void *result = 0;

    Buffer *command_buffer = &renderer->command_buffer;

    S64 total = size + sizeof(U32);
    if ((command_buffer->used + total) <= command_buffer->limit) {
        U32 *header = cast(U32 *) (command_buffer->data + command_buffer->used);

        header[0] = type;
        result    = cast(void *) (header + 1);

        command_buffer->used += total;
    }

    return result;
}

FileScope B32 RendererTextureEqual(RendererTexture a, RendererTexture b) {
    B32 result = a.value == b.value;
    return result;
}

FileScope B32 DrawCallShouldRestart(RendererContext *renderer, RendererTexture texture) {
    B32 result = false;

    Assert(renderer->draw_call != 0); // we check this externally, just incase it gets called elsewhere

    RenderCommandDraw *draw = renderer->draw_call;

    B32 is_current_sprite = RendererTextureIsSprite(renderer, draw->texture);
    B32 is_new_sprite     = RendererTextureIsSprite(renderer, texture);

    result = !((is_current_sprite && is_new_sprite) || RendererTextureEqual(draw->texture, texture));
    return result;
}

FileScope RenderCommandDraw *RendererDrawCallIssue(RendererContext *renderer, RendererTexture texture) {
    RenderCommandDraw *result = renderer->draw_call;

    if (!result || DrawCallShouldRestart(renderer, texture)) {
        result = RenderCommandPush(renderer, RenderCommandDraw);
        if (result) {
            result->texture       = texture;
            result->ubo_offset    = renderer->camera.ubo_offset;
            result->vertex_offset = renderer->vertices.count;
            result->vertex_count  = 0;

            result->index_offset = renderer->indices.count;
            result->index_count  = 0;

            renderer->draw_call = result;
        }
    }

    return result;
}

B32 RendererTextureIsSprite(RendererContext *renderer, RendererTexture texture) {
    B32 result = (texture.width  <= renderer->sprite_array.dimension) &&
                    (texture.height <= renderer->sprite_array.dimension);

    return result;
}

// data will contain a pointer to write data to transfer to
//
RendererTransferTask *RendererTransferQueueEnqueueSize(RendererTransferQueue *queue, void **data, U64 size) {
    RendererTransferTask *result = 0;

    Assert(size <= queue->limit);

    if (queue->task_count < queue->task_limit) {
        U32 mask  = (queue->task_limit - 1);
        U32 index = (queue->first_task + queue->task_count) & mask;

        result = &queue->tasks[index];

        // acquire the memory
        //
        if (queue->write_offset == queue->read_offset) {
            // rowo[-----------------------]
            //
            // in this case there is nothing on the queue so the entire buffer is available to us
            //
            if (queue->task_count == 0) {
                // the transfer ring is empty so we can just enqueue from the start
                //
                queue->write_offset = 0;
                queue->read_offset  = 0;
            }
        }
        else if (queue->write_offset < queue->read_offset) {
            // [xxxx]wo[------]ro[xxxxx]
            //
            // this case there is a chunk of unused memory between the two offset pointers so
            // we check if it is big enough to hold our size and use that
            //
            U64 size_middle = queue->read_offset - queue->write_offset;
            if (size_middle < size) {
                result = 0;
            }
        }
        else /*if (queue->read_offset < queue->write_offset)*/ {
            // [----]ro[xxxxxx]wo[-----]
            //
            // in this case there are two chunks of unused memory at the beginning and end of
            // the ring so we check if the chunk at the end is big enough, if it isn't, we move
            // the write offset forward and use the one at the beginning if it is big enough.
            //
            // its a bit wasteful if the one at the end isn't big enough, however, this only happens
            // if the one at the beginning is big enough, if neither can fit the size we need the
            // ring isn't modified and the size isn't enqueued
            //
            U64 size_end = queue->limit - queue->write_offset;
            if (size_end < size) {
                U64 size_begin = queue->read_offset;
                if (size_begin < size) {
                    result = 0;
                }
                else {
                    // we have to go from the beginning in this case
                    //
                    queue->write_offset = 0;
                }
            }
        }

        if (result) {
            result->state  = RENDERER_TRANSFER_TASK_STATE_PENDING;
            result->offset = queue->write_offset;
            result->size   = size;

            queue->write_offset += size;
            queue->task_count   += 1;

            // @todo: this should just be stored on the resulting RendererTransferTask structure
            //
            *data = cast(void *) (queue->base + result->offset);
        }
    }

    return result;
}

void CameraTransformSet(RendererContext *renderer, Vec3F x, Vec3F y, Vec3F z, Vec3F p, U32 flags, F32 nearp, F32 farp, F32 fov) {
    CameraTransform *tx = &renderer->camera;

    tx->x = x;
    tx->y = y;
    tx->z = z;

    tx->p = p;

    Assert(renderer->setup.window_dim.w > 0);
    Assert(renderer->setup.window_dim.h > 0);

    Mat4x4FInv projection;

    F32 aspect_ratio = cast(F32) renderer->setup.window_dim.w / cast(F32) renderer->setup.window_dim.h;
    if (flags & CAMERA_TRANSFORM_FLAG_ORTHOGRAPHIC) {
        projection = M4x4F_OrthographicProjection(aspect_ratio, nearp, farp);
    }
    else {
        projection = M4x4F_PerspectiveProjection(fov, aspect_ratio, nearp, farp);
    }

    Mat4x4FInv camera = M4x4F_CameraViewProjection(x, y, z, p);

    tx->transform.fwd = M4x4F_Mul(projection.fwd, camera.fwd);
    tx->transform.inv = M4x4F_Mul(camera.inv, projection.inv);

    // push onto the uniforms arena for binding later
    //
    tx->ubo_offset = renderer->uniforms.used;

    ShaderGlobals *globals = cast(ShaderGlobals *) (renderer->uniforms.data + tx->ubo_offset);
    renderer->uniforms.used += renderer->ubo_globals_size;

    // @important: do not read from this memory
    //
    globals->transform       = tx->transform.fwd;
    globals->camera_position = V4F_FromV3F(tx->p, 1.0);

    globals->time       = 0; // @todo: get time
    globals->dt         = 0; // @todo: get dt
    globals->window_dim = renderer->setup.window_dim;

    renderer->draw_call = 0;
}

void CameraTransformSetFromAxes(RendererContext *renderer, Vec3F x, Vec3F y, Vec3F z, Vec3F p, U32 flags) {
    CameraTransformSet(renderer, x, y, z, p, flags, 0.001f, 10000.0f, 65.0f);
}

void CameraTransformGet(CameraTransform *camera, F32 aspect, Vec3F x, Vec3F y, Vec3F z, Vec3F p, U32 flags, F32 nearp, F32 farp, F32 fov) {
    camera->x = x;
    camera->y = y;
    camera->z = z;
    camera->p = p;

    Mat4x4FInv projection;

    if (flags & CAMERA_TRANSFORM_FLAG_ORTHOGRAPHIC) {
        projection = M4x4F_OrthographicProjection(aspect, nearp, farp);
    }
    else {
        projection = M4x4F_PerspectiveProjection(fov, aspect, nearp, farp);
    }

    Mat4x4FInv tx = M4x4F_CameraViewProjection(x, y, z, p);

    camera->transform.fwd = M4x4F_Mul(projection.fwd, tx.fwd);
    camera->transform.inv = M4x4F_Mul(tx.inv, projection.inv);
}

void CameraTransformGetFromAxes(CameraTransform *camera, F32 aspect, Vec3F x, Vec3F y, Vec3F z, Vec3F p, U32 flags) {
    CameraTransformGet(camera, aspect, x, y, z, p, flags, 0.001f, 10000.0f, 70.0f);
}

Vec3F V2FUnproject(CameraTransform *camera, Vec2F clip) {
    Vec3F result = V2FUnprojectAt(camera, clip, camera->p.z);
    return result;
}

Vec3F V2FUnprojectAt(CameraTransform *camera, Vec2F clip, F32 z) {
    Vec3F result;

    Vec4F z_dist = V4F_FromV3F(V3F_Sub(camera->p, V3F_Scale(camera->z, z)), 1.0f);
    Vec4F persp  = M4x4F_MulV4F(camera->transform.fwd, z_dist);

    clip = V2F_Scale(clip, persp.w);

    Vec4F world = M4x4F_MulV4F(camera->transform.inv, V4F(clip.x, clip.y, persp.z, persp.w));

    result = world.xyz;
    return result;
}

Rect3F CameraBoundsGet(CameraTransform *camera) {
    Rect3F result;
    result.min = V2FUnproject(camera, V2F(-1, -1));
    result.max = V2FUnproject(camera, V2F( 1,  1));

    return result;
}

Rect3F CameraBoundsGetAt(CameraTransform *camera, F32 z) {
    Rect3F result;
    result.min = V2FUnprojectAt(camera, V2F(-1, -1), z);
    result.max = V2FUnprojectAt(camera, V2F( 1,  1), z);

    return result;
}

void RendererLayerPush(RendererContext *renderer) {
    renderer->layer += renderer->layer_offset;
}
