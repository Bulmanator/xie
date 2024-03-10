#if !defined(XI_DRAW_H_)
#define XI_DRAW_H_

// this is here to contain all of the draw functions, we want the draw functions to take handles
// retrieved from the asset system, not the render system. however, the asset system requires the
// renderer types to be defined thus creating a cyclic dependency that the c/c++ compilers cannot work
// out because they don't have out of order compilation
//
//
Func void QuadDraw(RendererContext *renderer, Vec4F colour, Vec2F center, Vec2F dim, F32 turns);
Func void LineDraw(RendererContext *renderer, Vec4F start_c, Vec2F start_p, Vec4F end_c, Vec2F end_p, F32 thickness);
Func void QuadOutlineDraw(RendererContext *renderer, Vec4F colour, Vec2F center, Vec2F dim, F32 turns, F32 thickness);

Func void SpriteDrawScaled(RendererContext *renderer, ImageHandle image, Vec2F center, F32 scale, F32 turns);
Func void SpriteDraw(RendererContext *renderer, ImageHandle image, Vec2F center, Vec2F dim, F32 turns);

Func void ColouredSpriteDrawScaled(RendererContext *renderer, ImageHandle image, Vec4F colour, Vec2F center, F32 scale, F32 turns);
Func void ColouredSpriteDraw(RendererContext *renderer, ImageHandle image, Vec4F colour, Vec2F center, Vec2F dim, F32 turns);

#endif  // XI_DRAW_H_
