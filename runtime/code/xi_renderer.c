#define xi_render_command_push(renderer, type) \
    xi_render_command_push_size(renderer, XI_RENDER_COMMAND_##type, sizeof(type))

static void *xi_render_command_push_size(xiRenderer *renderer, u32 type, uptr size) {
    void *result = 0;

    buffer *commands = &renderer->commands;

    uptr total = size + sizeof(u32);
    if ((commands->used + total) <= commands->limit) {
        u32 *header = (u32 *) (commands->data + commands->used);

        header[0] = type;
        result    = (void *) (header + 1);

        commands->used += total;
    }

    return result;
}

static xiRenderCommandDraw *xi_renderer_draw_call_issue(xiRenderer *renderer) {
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

static void xi_quad_draw_vertices(xiRenderer *renderer, vert3 vt0, vert3 vt1, vert3 vt2, vert3 vt3) {
    xiRenderCommandDraw *cmd = xi_renderer_draw_call_issue(renderer);

    XI_ASSERT(cmd);

    if (cmd) {
        vert3 *vertices = renderer->vertices.base + cmd->vertex_offset + cmd->vertex_count;
        u16   *indices  = renderer->indices.base  + cmd->index_offset  + cmd->index_count;

        // set vertices for this quad
        //
        vertices[0] = vt0;
        vertices[1] = vt1;
        vertices[2] = vt2;
        vertices[3] = vt3;

        XI_ASSERT((cmd->vertex_count + 6) <= XI_U16_MAX);

        // set the indices for this quad
        //
        u16 offset = (u16) cmd->vertex_count;

        indices[0] = 0 + offset;
        indices[1] = 1 + offset;
        indices[2] = 2 + offset;

        indices[3] = 1 + offset;
        indices[4] = 2 + offset;
        indices[5] = 3 + offset;

        renderer->vertices.count += 4;
        renderer->indices.count  += 6;

        cmd->vertex_count += 4;
        cmd->index_count  += 6;
    }
}

void xi_quad_draw_xy(xiRenderer *renderer, v4 colour,
        v2 center, v2 dimension, f32 angle)
{
    // @todo: rotate the quad
    //
    (void) angle;

    // convert colour to pre-multiplied alpha and then pack into u32
    //
    // @todo: i need to implement math functions
    //
    colour.r *= colour.a;
    colour.g *= colour.a;
    colour.b *= colour.a;

    u32 ucolour =
        ((u32) (colour.a * 255) << 24) |
        ((u32) (colour.r * 255) << 16) |
        ((u32) (colour.g * 255) <<  8) |
        ((u32) (colour.b * 255) <<  0);

    v2 half_dim;
    half_dim.x = (0.5f * dimension.x);
    half_dim.y = (0.5f * dimension.y);

    vert3 vt[4];

    vt[0].p.x  = center.x - half_dim.x;
    vt[0].p.y  = center.y + half_dim.y;
    vt[0].p.z  = 0; // @todo: add z componenet
    vt[0].uv.x = 0;
    vt[0].uv.y = 0;
    vt[0].c    = ucolour;

    vt[1].p.x  = center.x - half_dim.x;
    vt[1].p.y  = center.y - half_dim.y;
    vt[1].p.z  = 0; // @todo: add z componenet
    vt[1].uv.x = 0;
    vt[1].uv.y = 1;
    vt[1].c    = ucolour;

    vt[2].p.x  = center.x + half_dim.x;
    vt[2].p.y  = center.y + half_dim.y;
    vt[2].p.z  = 0; // @todo: add z componenet
    vt[2].uv.x = 1;
    vt[2].uv.y = 1;
    vt[2].c    = ucolour;

    vt[3].p.x  = center.x + half_dim.x;
    vt[3].p.y  = center.y - half_dim.y;
    vt[3].p.z  = 0; // @todo: add z componenet
    vt[3].uv.x = 1;
    vt[3].uv.y = 0;
    vt[3].c    = ucolour;

    xi_quad_draw_vertices(renderer, vt[0], vt[1], vt[2], vt[3]);
}
