XI_INTERNAL void xi_quad_draw_vertices(xiRenderer *renderer, xiRendererTexture texture,
        xi_vert3 vt0, xi_vert3 vt1, xi_vert3 vt2, xi_vert3 vt3)
{
    xiRenderCommandDraw *cmd = xi_renderer_draw_call_issue(renderer, texture);

    // @todo: the whole drawing routine could be a bit more efficient if we wrote directly into the vertices,
    // this should turn into some sort of 'vertices_push' call which returns a pointer to 4 vertices for
    // us to write into and calculates the offsets/indicies for us
    //
    XI_ASSERT(cmd);

    if (cmd) {
        xi_vert3 *vertices = renderer->vertices.base + cmd->vertex_offset + cmd->vertex_count;
        xi_u16   *indices  = renderer->indices.base  + cmd->index_offset  + cmd->index_count;

        // set the layer of the vertices, all other draw commands go through this path so
        // less error prone to set it here
        //
        vt0.p.z = renderer->layer;
        vt1.p.z = renderer->layer;
        vt2.p.z = renderer->layer;
        vt3.p.z = renderer->layer;

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
    colour.xyz = xi_v3_mul_f32(colour.xyz, colour.a); // premultiply alpha
    xi_u32 ucolour = xi_v4_colour_abgr_pack_norm(colour); // pack to u32

    xi_v2 half_dim;
    half_dim.x = (0.5f * dimension.x);
    half_dim.y = (0.5f * dimension.y);

    xi_vert3 vt[4];

    xi_f32 texture_index = 1;
    xi_m2x2 rot = xi_m2x2_from_radians(angle);

    vt[0].p.xy = xi_v2_add(center, xi_m2x2_mul_v2(rot, xi_v2_create(-half_dim.x, half_dim.y)));
    vt[0].uv   = xi_v3_create(0, 0, texture_index);
    vt[0].c    = ucolour;

    vt[1].p.xy = xi_v2_add(center, xi_m2x2_mul_v2(rot, xi_v2_neg(half_dim)));
    vt[1].uv   = xi_v3_create(0, 1, texture_index);
    vt[1].c    = ucolour;

    vt[2].p.xy = xi_v2_add(center, xi_m2x2_mul_v2(rot, half_dim));
    vt[2].uv   = xi_v3_create(1, 1, texture_index);
    vt[2].c    = ucolour;

    vt[3].p.xy = xi_v2_add(center, xi_m2x2_mul_v2(rot, xi_v2_create(half_dim.x, -half_dim.y)));
    vt[3].uv   = xi_v3_create(1, 0, texture_index);
    vt[3].c    = ucolour;

    // just white
    //
    // @todo: this should probably be able to be looked up using "_white" naming or something
    //
    xiRendererTexture texture = renderer->assets->assets[1].data.texture;

    xi_quad_draw_vertices(renderer, texture, vt[0], vt[1], vt[2], vt[3]);
}

void xi_line_draw_xy(xiRenderer *renderer, xi_v4 start_colour, xi_v2 start_position,
        xi_v4 end_colour, xi_v2 end_position, xi_f32 thickness)
{
    // calculate direction and normal of line
    //
    xi_v2 dir  = xi_v2_noz(xi_v2_sub(end_position, start_position));
    xi_v2 norm = xi_v2_perp(dir);

    // pre-multiply and pack
    //
    start_colour.rgb = xi_v3_mul_f32(start_colour.rgb, start_colour.a);
    end_colour.rgb   = xi_v3_mul_f32(end_colour.rgb,   end_colour.a);

    xi_u32 ustart = xi_v4_colour_abgr_pack_norm(start_colour);
    xi_u32 uend   = xi_v4_colour_abgr_pack_norm(end_colour);

    xi_vert3 vt[4];

    xi_f32 half_width = 0.5f * thickness;

    vt[0].p.xy = xi_v2_sub(start_position, xi_v2_mul_f32(norm, half_width));
    vt[0].uv   = xi_v3_create(0, 0, 1);
    vt[0].c    = ustart;

    vt[1].p.xy = xi_v2_add(start_position, xi_v2_mul_f32(norm, half_width));
    vt[1].uv   = xi_v3_create(1, 0, 1);
    vt[1].c    = ustart;

    vt[2].p.xy = xi_v2_sub(end_position, xi_v2_mul_f32(norm, half_width));
    vt[2].uv   = xi_v3_create(1, 1, 1);
    vt[2].c    = uend;

    vt[3].p.xy = xi_v2_add(end_position, xi_v2_mul_f32(norm, half_width));
    vt[3].uv   = xi_v3_create(0, 1, 1);
    vt[3].c    = uend;

    xiRendererTexture texture = renderer->assets->assets[1].data.texture;
    xi_quad_draw_vertices(renderer, texture, vt[0], vt[1], vt[2], vt[3]);
}

void xi_quad_outline_draw_xy(xiRenderer *renderer, xi_v4 colour,
        xi_v2 center, xi_v2 dimension, xi_f32 angle, xi_f32 thickness)
{
    xi_v2 half_dim = xi_v2_mul_f32(dimension, 0.5f);
    xi_m2x2 rot = xi_m2x2_from_radians(angle);

    xi_v2 pt[4];

    pt[0] = xi_v2_add(center, xi_m2x2_mul_v2(rot, xi_v2_neg(half_dim)));
    pt[1] = xi_v2_add(center, xi_m2x2_mul_v2(rot, xi_v2_create(-half_dim.x, half_dim.y)));
    pt[2] = xi_v2_add(center, xi_m2x2_mul_v2(rot, half_dim));
    pt[3] = xi_v2_add(center, xi_m2x2_mul_v2(rot, xi_v2_create(half_dim.x, -half_dim.y)));

    xi_f32 half_width = 0.5f * thickness;

    for (xi_u32 it = 0; it < 4; ++it) {
        xi_v2 next = pt[(it + 1) & 3];

        xi_v2 dir = xi_v2_noz(xi_v2_sub(next, pt[it]));
        xi_v2 end = xi_v2_add(next, xi_v2_mul_f32(dir, half_width));

        xi_line_draw_xy(renderer, colour, pt[it], colour, end, thickness);
    }
}

XI_INTERNAL inline xi_v2 xi_uv_for_sprite(xi_v2 dim, xi_u32 sprite_dimension) {
    xi_v2 result = xi_v2_create(1, 1);
    if (dim.w <= sprite_dimension && dim.h <= sprite_dimension) {
        result = xi_v2_mul_f32(dim, 1.0f / (xi_f32) sprite_dimension);
    }

    return result;
}

void xi_sprite_draw_xy_scaled(xiRenderer *renderer, xiImageHandle image,
        xi_v2 center, xi_f32 scale, xi_f32 angle)
{
    xi_coloured_sprite_draw_xy(renderer, image, xi_v4_create(1, 1, 1, 1),
            center, xi_v2_create(scale, scale), angle);
}

void xi_sprite_draw_xy(xiRenderer *renderer, xiImageHandle image,
        xi_v2 center, xi_v2 dimension, xi_f32 angle)
{
    xi_coloured_sprite_draw_xy(renderer, image, xi_v4_create(1, 1, 1, 1), center, dimension, angle);
}

void xi_coloured_sprite_draw_xy_scaled(xiRenderer *renderer, xiImageHandle image, xi_v4 colour,
        xi_v2 center, xi_f32 scale, xi_f32 angle)
{
    xi_coloured_sprite_draw_xy(renderer, image, colour, center, xi_v2_create(scale, scale), angle);
}

void xi_coloured_sprite_draw_xy(xiRenderer *renderer, xiImageHandle image, xi_v4 colour,
        xi_v2 center, xi_v2 dimension, xi_f32 angle)
{
    xiRendererTexture texture = xi_image_data_get(renderer->assets, image);

    xi_m2x2 rot = xi_m2x2_from_radians(angle);

    // store colour as pre-multiplied alpha and pack to u32
    //
    colour.rgb = xi_v3_mul_f32(colour.rgb, colour.a);
    xi_u32 ucolour = xi_v4_colour_abgr_pack_norm(colour);

    xiaImageInfo *image_info = xi_image_info_get(renderer->assets, image);
    xi_v2 image_dim = xi_v2_create((xi_f32) image_info->width, (xi_f32) image_info->height);

    xi_v2 half_dim;
    if (image_info->width > image_info->height) {
        half_dim.w = 0.5f * dimension.x;
        half_dim.h = 0.5f * dimension.y * (image_dim.h / image_dim.w);
    }
    else {
        half_dim.w = 0.5f * dimension.x * (image_dim.w / image_dim.h);
        half_dim.h = 0.5f * dimension.y;
    }

    xi_v3 uv;
    uv.xy = xi_uv_for_sprite(image_dim, renderer->sprite_array.dimension);
    uv.z  = xi_renderer_texture_is_sprite(renderer, texture) ? texture.index : 0.0f;

    xi_vert3 vt[4];

    vt[0].p.xy = xi_v2_add(center, xi_m2x2_mul_v2(rot, xi_v2_create(-half_dim.x, half_dim.y)));
    vt[0].uv   = xi_v3_create(0, 0, uv.z);
    vt[0].c    = ucolour;

    vt[1].p.xy = xi_v2_add(center, xi_m2x2_mul_v2(rot, xi_v2_neg(half_dim)));
    vt[1].uv   = xi_v3_create(0, uv.y, uv.z);
    vt[1].c    = ucolour;

    vt[2].p.xy = xi_v2_add(center, xi_m2x2_mul_v2(rot, half_dim));
    vt[2].uv   = xi_v3_create(uv.x, 0, uv.z);
    vt[2].c    = ucolour;

    vt[3].p.xy = xi_v2_add(center, xi_m2x2_mul_v2(rot, xi_v2_create(half_dim.x, -half_dim.y)));
    vt[3].uv   = uv;
    vt[3].c    = ucolour;

    xi_quad_draw_vertices(renderer, texture, vt[0], vt[1], vt[2], vt[3]);
}
