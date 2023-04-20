#define xi_render_command_push(renderer, type) \
    xi_render_command_push_size(renderer, XI_RENDER_COMMAND_##type, sizeof(type))

XI_INTERNAL void *xi_render_command_push_size(xiRenderer *renderer, xi_u32 type, xi_uptr size) {
    void *result = 0;

    xi_buffer *command_buffer = &renderer->command_buffer;

    xi_uptr total = size + sizeof(xi_u32);
    if ((command_buffer->used + total) <= command_buffer->limit) {
        xi_u32 *header = (xi_u32 *) (command_buffer->data + command_buffer->used);

        header[0] = type;
        result    = (void *) (header + 1);

        command_buffer->used += total;
    }

    return result;
}

XI_INTERNAL xiRenderCommandDraw *xi_renderer_draw_call_issue(xiRenderer *renderer) {
    xiRenderCommandDraw *result = renderer->draw_call;
    if (!result) {
        result = xi_render_command_push(renderer, xiRenderCommandDraw);
        if (result) {
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

xi_b32 xi_renderer_texture_is_sprite(xiRenderer *renderer, xiRendererTexture texture) {
    xi_b32 result = (texture.width  <= renderer->sprite_array.dimension) &&
                    (texture.height <= renderer->sprite_array.dimension);

    return result;
}

xiRendererTransferTask *xi_renderer_transfer_queue_enqueue_size(xiRendererTransferQueue *transfer_queue,
        void **data, xi_uptr size) // data will contain a pointer to write data to transfer to
{
    xiRendererTransferTask *result = 0;

    XI_ASSERT(size <= transfer_queue->limit);

    if (transfer_queue->task_count < transfer_queue->max_tasks) {
        xi_u32 mask  = (transfer_queue->max_tasks - 1);
        xi_u32 index = (transfer_queue->first_task + transfer_queue->task_count) & mask;

        result = &transfer_queue->tasks[index];

        // acquire the memory
        //
        if (transfer_queue->write_offset == transfer_queue->read_offset) {
            // rowo[-----------------------]
            //
            // in this case there is nothing on the queue so the entire buffer is available to us
            //
            if (transfer_queue->task_count == 0) {
                // the transfer ring is empty so we can just enqueue from the start
                //
                transfer_queue->write_offset = 0;
                transfer_queue->read_offset  = 0;
            }
        }
        else if (transfer_queue->write_offset < transfer_queue->read_offset) {
            // [xxxx]wo[------]ro[xxxxx]
            //
            // this case there is a chunk of unused memory between the two offset pointers so
            // we check if it is big enough to hold our size and use that
            //
            xi_uptr size_middle = transfer_queue->read_offset - transfer_queue->write_offset;
            if (size_middle < size) {
                result = 0;
            }

        }
        else /*if (transfer_queue->read_offset < transfer_queue->write_offset)*/ {
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
            xi_uptr size_end = transfer_queue->limit - transfer_queue->write_offset;
            if (size_end < size) {
                xi_uptr size_begin = transfer_queue->read_offset;
                if (size_begin < size) {
                    result = 0;
                }
                else {
                    // we have to go from the beginning in this case
                    //
                    transfer_queue->write_offset = 0;
                }
            }
        }

        if (result) {
            result->state  = XI_RENDERER_TRANSFER_TASK_STATE_PENDING;
            result->offset = transfer_queue->write_offset;
            result->size   = size;

            transfer_queue->write_offset += size;
            transfer_queue->task_count += 1;

            *data = (void *) (transfer_queue->base + result->offset);
        }
    }

    return result;
}

void xi_camera_transform_set(xiRenderer *renderer,
        xi_v3 x_axis, xi_v3 y_axis, xi_v3 z_axis, xi_v3 position,
        xi_u32 flags, xi_f32 near_plane, xi_f32 far_plane, xi_f32 focal_length)
{
    xiCameraTransform *tx = &renderer->camera;

    tx->x_axis   = x_axis;
    tx->y_axis   = y_axis;
    tx->z_axis   = z_axis;

    tx->position = position;

    XI_ASSERT(renderer->setup.window_dim.w > 0);
    XI_ASSERT(renderer->setup.window_dim.h > 0);

    xi_m4x4_inv projection;

    xi_f32 aspect_ratio = (xi_f32) renderer->setup.window_dim.w / (xi_f32) renderer->setup.window_dim.h;
    if (flags & XI_CAMERA_TRANSFORM_FLAG_ORTHOGRAPHIC) {
        projection = xi_m4x4_orthographic_projection(aspect_ratio, near_plane, far_plane);
    }
    else {
        projection = xi_m4x4_perspective_projection(focal_length, aspect_ratio, near_plane, far_plane);
    }

    xi_m4x4_inv camera = xi_m4x4_from_camera_transform(x_axis, y_axis, z_axis, position);

    tx->transform.fwd = xi_m4x4_mul(projection.fwd, camera.fwd);
    tx->transform.inv = xi_m4x4_mul(camera.inv, projection.inv);

    // push onto the uniforms arena for binding later
    //
    tx->ubo_offset = xi_arena_offset_get(&renderer->uniforms);

    xiShaderGlobals *globals = xi_arena_push_type(&renderer->uniforms, xiShaderGlobals);

    // @important: do not read from this memory
    //
    globals->transform       = tx->transform.fwd;
    globals->camera_position = xi_v4_from_v3(tx->position, 1.0);

    globals->time    = 0; // @todo: get time
    globals->dt      = 0; // @todo: get dt
    globals->unused0 = 0;
    globals->unused1 = 0;

    renderer->draw_call = 0;
}

void xi_camera_transform_set_axes(xiRenderer *renderer,
        xi_v3 x_axis, xi_v3 y_axis, xi_v3 z_axis, xi_v3 position, xi_u32 flags)
{
    xi_f32 focal_length = 1.5696855771174903493f; // ~65 degree fov
    xi_camera_transform_set(renderer, x_axis, y_axis, z_axis, position, flags, 0.001f, 10000.0f, focal_length);
}
