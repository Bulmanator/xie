FileScope U32 V4F_ColourStore(Vec4F colour) {
    U32 result;

    colour.rgb = V3F_Scale(colour.rgb, colour.a);
    colour     = V4F_SRGB1FromLinear1(colour);

    result = V4F_ColourPackABGR(colour);
    return result;
}
FileScope void QuadVerticesDraw(RendererContext *renderer, RendererTexture texture,
        Vertex3 vt0, Vertex3 vt1, Vertex3 vt2, Vertex3 vt3)
{
    RenderCommandDraw *cmd = RendererDrawCallIssue(renderer, texture);

    // @todo: the whole drawing routine could be a bit more efficient if we wrote directly into the vertices,
    // this should turn into some sort of 'vertices_push' call which returns a pointer to 4 vertices for
    // us to write into and calculates the offsets/indicies for us
    //
    Assert(cmd);

    if (cmd) {
        Vertex3 *vertices = renderer->vertices.base + cmd->vertex_offset + cmd->vertex_count;
        U16     *indices  = renderer->indices.base  + cmd->index_offset  + cmd->index_count;

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

        Assert((cmd->vertex_count + 6) <= U16_MAX);

        // set the indices for this quad
        //
        U16 offset = cast(U16) cmd->vertex_count;

        indices[0] = 0 + offset;
        indices[1] = 1 + offset;
        indices[2] = 2 + offset;

        indices[3] = 1 + offset;
        indices[4] = 3 + offset;
        indices[5] = 2 + offset;

        renderer->vertices.count += 4;
        renderer->indices.count  += 6;

        cmd->vertex_count += 4;
        cmd->index_count  += 6;
    }
}

void QuadDraw(RendererContext *renderer, Vec4F colour, Vec2F center, Vec2F dimension, F32 turns) {
    U32 ucolour    = V4F_ColourStore(colour);
    Vec2F half_dim = V2F_Scale(dimension, 0.5f);

    Vertex3 vt[4];

    Vec2F rot = V2F_UnitArm(turns);

    vt[0].p.xy = V2F_Add(center, V2F_Rotate(rot, V2F(-half_dim.x, half_dim.y)));
    vt[0].uv   = V3F(0, 0, 1);
    vt[0].c    = ucolour;

    vt[1].p.xy = V2F_Add(center, V2F_Rotate(rot, V2F_Neg(half_dim)));
    vt[1].uv   = V3F(0, 1, 1);
    vt[1].c    = ucolour;

    vt[2].p.xy = V2F_Add(center, V2F_Rotate(rot, half_dim));
    vt[2].uv   = V3F(1, 1, 1);
    vt[2].c    = ucolour;

    vt[3].p.xy = V2F_Add(center, V2F_Rotate(rot, V2F(half_dim.x, -half_dim.y)));
    vt[3].uv   = V3F(1, 0, 1);
    vt[3].c    = ucolour;

    // just white
    //
    RendererTexture texture = renderer->assets->assets[1].data.texture;
    QuadVerticesDraw(renderer, texture, vt[0], vt[1], vt[2], vt[3]);
}

void LineDraw(RendererContext *renderer, Vec4F start_c, Vec2F start_p, Vec4F end_c, Vec2F end_p, F32 thickness) {
    // calculate direction and normal of line
    //
    Vec2F dir  = V2F_Normalise(V2F_Sub(end_p, start_p));
    Vec2F norm = V2F_Perp(dir);

    U32 ustart  = V4F_ColourStore(start_c);
    U32 uend    = V4F_ColourStore(end_c);

    F32 half_width = 0.5f * thickness;

    Vertex3 vt[4];

    vt[0].p.xy = V2F_Sub(start_p, V2F_Scale(norm, half_width));
    vt[0].uv   = V3F(0, 0, 1);
    vt[0].c    = ustart;

    vt[1].p.xy = V2F_Sub(end_p, V2F_Scale(norm, half_width));
    vt[1].uv   = V3F(1, 1, 1);
    vt[1].c    = uend;

    vt[2].p.xy = V2F_Add(start_p, V2F_Scale(norm, half_width));
    vt[2].uv   = V3F(1, 0, 1);
    vt[2].c    = ustart;

    vt[3].p.xy = V2F_Add(end_p, V2F_Scale(norm, half_width));
    vt[3].uv   = V3F(0, 1, 1);
    vt[3].c    = uend;

    RendererTexture texture = renderer->assets->assets[1].data.texture;
    QuadVerticesDraw(renderer, texture, vt[0], vt[1], vt[2], vt[3]);
}

void QuadOutlineDraw(RendererContext *renderer, Vec4F colour, Vec2F center, Vec2F dimension, F32 turns, F32 thickness) {
    Vec2F half_dim = V2F_Scale(dimension, 0.5f);
    Vec2F rot      = V2F_UnitArm(turns);

    Vec2F pt[4];

    pt[0] = V2F_Add(center, V2F_Rotate(rot, V2F_Neg(half_dim)));
    pt[1] = V2F_Add(center, V2F_Rotate(rot, V2F(-half_dim.x, half_dim.y)));
    pt[2] = V2F_Add(center, V2F_Rotate(rot, half_dim));
    pt[3] = V2F_Add(center, V2F_Rotate(rot, V2F(half_dim.x, -half_dim.y)));

    F32 half_width = 0.5f * thickness;

    for (U32 it = 0; it < 4; ++it) {
        Vec2F next = pt[(it + 1) & 3];

        Vec2F dir = V2F_Normalise(V2F_Sub(next, pt[it]));
        Vec2F end = V2F_Add(next, V2F_Scale(dir, half_width));

        LineDraw(renderer, colour, pt[it], colour, end, thickness);
    }
}

FileScope inline Vec2F SpriteUVGet(Vec2F dim, F32 sprite_dimension) {
    Vec2F result = V2F(1, 1);

    if (dim.w <= sprite_dimension && dim.h <= sprite_dimension) {
        result = V2F_Scale(dim, 1.0f / cast(F32) sprite_dimension);
    }

    return result;
}

void SpriteDrawScaled(RendererContext *renderer, ImageHandle image, Vec2F center, F32 scale, F32 turns) {
    ColouredSpriteDraw(renderer, image, V4F(1, 1, 1, 1), center, V2F(scale, scale), turns);
}

void SpriteDraw(RendererContext *renderer, ImageHandle image, Vec2F center, Vec2F dimension, F32 turns) {
    ColouredSpriteDraw(renderer, image, V4F(1, 1, 1, 1), center, dimension, turns);
}

void ColouredSpriteDrawScaled(RendererContext *renderer, ImageHandle image, Vec4F colour, Vec2F center, F32 scale, F32 turns) {
    ColouredSpriteDraw(renderer, image, colour, center, V2F(scale, scale), turns);
}

void ColouredSpriteDraw(RendererContext *renderer, ImageHandle image, Vec4F colour, Vec2F center, Vec2F dimension, F32 turns) {
    RendererTexture texture = ImageDataGet(renderer->assets, image);

    Vec2F rot   = V2F_UnitArm(turns);
    U32 ucolour = V4F_ColourStore(colour);

    Xia_ImageInfo *image_info = ImageInfoGet(renderer->assets, image);
    Vec2F          image_dim  = V2F((F32) image_info->width, (F32) image_info->height);

    Vec2F half_dim;
    if (image_info->width > image_info->height) {
        half_dim.w = 0.5f * dimension.x;
        half_dim.h = 0.5f * dimension.y * (image_dim.h / image_dim.w);
    }
    else {
        half_dim.w = 0.5f * dimension.x * (image_dim.w / image_dim.h);
        half_dim.h = 0.5f * dimension.y;
    }

    Vec3F uv;
    uv.xy = SpriteUVGet(image_dim, cast(F32) renderer->sprite_array.dimension);
    uv.z  = RendererTextureIsSprite(renderer, texture) ? texture.index : 0.0f;

    Vertex3 vt[4];

    vt[0].p.xy = V2F_Add(center, V2F_Rotate(rot, V2F(-half_dim.x, half_dim.y)));
    vt[0].uv   = V3F(0, 0, uv.z);
    vt[0].c    = ucolour;

    vt[1].p.xy = V2F_Add(center, V2F_Rotate(rot, V2F_Neg(half_dim)));
    vt[1].uv   = V3F(0, uv.y, uv.z);
    vt[1].c    = ucolour;

    vt[2].p.xy = V2F_Add(center, V2F_Rotate(rot, half_dim));
    vt[2].uv   = V3F(uv.x, 0, uv.z);
    vt[2].c    = ucolour;

    vt[3].p.xy = V2F_Add(center, V2F_Rotate(rot, V2F(half_dim.x, -half_dim.y)));
    vt[3].uv   = uv;
    vt[3].c    = ucolour;

    QuadVerticesDraw(renderer, texture, vt[0], vt[1], vt[2], vt[3]);
}
