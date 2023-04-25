#if !defined(XI_DRAW_H_)
#define XI_DRAW_H_

// this is here to contain all of the draw functions, we want the draw functions to take handles
// retrieved from the asset system, not the render system. however, the asset system requires the
// renderer types to be defined thus creating a cyclic dependency that the c/c++ compilers cannot work
// out because they don't have out of order compilation
//

extern XI_API void xi_quad_draw_xy(xiRenderer *renderer, xi_v4 colour,
        xi_v2 center, xi_v2 dimension, xi_f32 angle);


extern XI_API void xi_line_draw_xy(xiRenderer *renderer, xi_v4 start_colour, xi_v2 start_position,
        xi_v4 end_colour, xi_v2 end_position, xi_f32 thickness);

extern XI_API void xi_quad_outline_draw_xy(xiRenderer *renderer, xi_v4 colour,
        xi_v2 center, xi_v2 dimension, xi_f32 angle, xi_f32 thickness);

// textured quads, sprites
//
extern XI_API void xi_sprite_draw_xy_scaled(xiRenderer *renderer, xiImageHandle image,
        xi_v2 center, xi_f32 scale, xi_f32 angle);

extern XI_API void xi_sprite_draw_xy(xiRenderer *renderer, xiImageHandle image,
        xi_v2 center, xi_v2 dimension, xi_f32 angle);

// coloured variants of the sprite drawing above, this will modulate the colour into the texture
//
extern XI_API void xi_coloured_sprite_draw_xy_scaled(xiRenderer *renderer, xiImageHandle image, xi_v4 colour,
        xi_v2 center, xi_f32 scale, xi_f32 angle);

extern XI_API void xi_coloured_sprite_draw_xy(xiRenderer *renderer, xiImageHandle image, xi_v4 colour,
        xi_v2 center, xi_v2 dimension, xi_f32 angle);

#endif  // XI_DRAW_H_
