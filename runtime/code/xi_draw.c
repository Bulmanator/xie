
XI_INTERNAL void xi_quad_draw_vertices(xiRenderer *renderer,
        xi_vert3 vt0, xi_vert3 vt1, xi_vert3 vt2, xi_vert3 vt3)
{
    xiRenderCommandDraw *cmd = xi_renderer_draw_call_issue(renderer);

    XI_ASSERT(cmd);

    if (cmd) {
        xi_vert3 *vertices = renderer->vertices.base + cmd->vertex_offset + cmd->vertex_count;
        xi_u16   *indices  = renderer->indices.base  + cmd->index_offset  + cmd->index_count;

        // set vertices for this quad
        //
        vertices[0] = vt0;
        vertices[1] = vt1;
        vertices[2] = vt2;
        vertices[3] = vt3;

        XI_ASSERT((cmd->vertex_count + 6) <= XI_U16_MAX);

        // set the indices for this quad
        //
        xi_u16 offset = (xi_u16) cmd->vertex_count;

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

void xi_quad_draw_xy(xiRenderer *renderer, xi_v4 colour,
        xi_v2 center, xi_v2 dimension, xi_f32 angle)
{
    // @todo: rotate the quad
    //
    (void) angle;

    // convert colour to pre-multiplied alpha and then pack into xi_u32
    //
    // @todo: i need to implement math functions
    //
    colour.r *= colour.a;
    colour.g *= colour.a;
    colour.b *= colour.a;

    xi_u32 ucolour =
        ((xi_u32) (colour.a * 255) << 24) |
        ((xi_u32) (colour.b * 255) << 16) |
        ((xi_u32) (colour.g * 255) <<  8) |
        ((xi_u32) (colour.r * 255) <<  0);

    xi_v2 half_dim;
    half_dim.x = (0.5f * dimension.x);
    half_dim.y = (0.5f * dimension.y);

    xi_vert3 vt[4];

    vt[0].p.x  = center.x - half_dim.x;
    vt[0].p.y  = center.y + half_dim.y;
    vt[0].p.z  = 0; // @todo: add z componenet
    vt[0].uv.x = 0;
    vt[0].uv.y = 0;
    vt[0].uv.z = 1;
    vt[0].c    = ucolour;

    vt[1].p.x  = center.x - half_dim.x;
    vt[1].p.y  = center.y - half_dim.y;
    vt[1].p.z  = 0; // @todo: add z componenet
    vt[1].uv.x = 0;
    vt[1].uv.y = 1;
    vt[1].uv.z = 1;
    vt[1].c    = ucolour;

    vt[2].p.x  = center.x + half_dim.x;
    vt[2].p.y  = center.y + half_dim.y;
    vt[2].p.z  = 0; // @todo: add z componenet
    vt[2].uv.x = 1;
    vt[2].uv.y = 1;
    vt[2].uv.z = 1;
    vt[2].c    = ucolour;

    vt[3].p.x  = center.x + half_dim.x;
    vt[3].p.y  = center.y - half_dim.y;
    vt[3].p.z  = 0; // @todo: add z componenet
    vt[3].uv.x = 1;
    vt[3].uv.y = 0;
    vt[3].uv.z = 1;
    vt[3].c    = ucolour;

    xi_quad_draw_vertices(renderer, vt[0], vt[1], vt[2], vt[3]);
}

void xi_sprite_draw_xy_scaled(xiRenderer *renderer, xiImageHandle image,
        xi_v2 center, xi_f32 scale, xi_f32 angle)
{
    // @todo: rotate the quad
    //
    (void) angle;

    xiRendererTexture texture = xi_image_data_get(renderer->assets, image);

    xi_u32 ucolour = 0xFFFFFFFF;

    xiaImageInfo *image_info = xi_image_info_get(renderer->assets, image);

    xi_v2 half_dim;
    if (image_info->width > image_info->height) {
        half_dim.w = 0.5f * scale;
        half_dim.h = 0.5f * scale * ((xi_f32) image_info->height / (xi_f32) image_info->width);
    }
    else {
        half_dim.w = 0.5f * scale * ((xi_f32) image_info->width / (xi_f32) image_info->height);
        half_dim.h = 0.5f * scale;
    }

    xi_vert3 vt[4];

    vt[0].p.x  = center.x - half_dim.x;
    vt[0].p.y  = center.y + half_dim.y;
    vt[0].p.z  = 0; // @todo: add z componenet
    vt[0].uv.x = 0;
    vt[0].uv.y = 0;
    vt[0].uv.z = texture.index;
    vt[0].c    = ucolour;

    vt[1].p.x  = center.x - half_dim.x;
    vt[1].p.y  = center.y - half_dim.y;
    vt[1].p.z  = 0; // @todo: add z componenet
    vt[1].uv.x = 0;
    vt[1].uv.y = (image_info->height / (xi_f32) renderer->sprite_array.dimension);
    vt[1].uv.z = texture.index;
    vt[1].c    = ucolour;

    vt[2].p.x  = center.x + half_dim.x;
    vt[2].p.y  = center.y + half_dim.y;
    vt[2].p.z  = 0; // @todo: add z componenet
    vt[2].uv.x = (image_info->width  / (xi_f32) renderer->sprite_array.dimension);
    vt[2].uv.y = 0;
    vt[2].uv.z = texture.index;
    vt[2].c    = ucolour;

    vt[3].p.x  = center.x + half_dim.x;
    vt[3].p.y  = center.y - half_dim.y;
    vt[3].p.z  = 0; // @todo: add z componenet
    vt[3].uv.x = (image_info->width / (xi_f32) renderer->sprite_array.dimension);
    vt[3].uv.y = (image_info->height / (xi_f32) renderer->sprite_array.dimension);
    vt[3].uv.z = texture.index;
    vt[3].c    = ucolour;

    xi_quad_draw_vertices(renderer, vt[0], vt[1], vt[2], vt[3]);

}
