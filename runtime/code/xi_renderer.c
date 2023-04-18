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
