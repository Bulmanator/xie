#if !defined(XI_DRAW_H_)
#define XI_DRAW_H_

// this is here to contain all of the draw functions, we want the draw functions to take handles
// retrieved from the asset system, not the render system. however, the asset system requires the
// renderer types to be defined thus creating a cyclic dependency that the c/c++ compilers cannot work
// out because they don't have out of order compilation
//

extern XI_API void xi_quad_draw_xy(xiRenderer *renderer, xi_v4 colour,
        xi_v2 center, xi_v2 dimension, xi_f32 angle);

extern XI_API void xi_sprite_draw_xy_scaled(xiRenderer *renderer, xiImageHandle image,
        xi_v2 center, xi_f32 scale, xi_f32 angle);

extern XI_API void xi_sprite_draw_xy(xiRenderer *renderer, xiImageHandle image,
        xi_v2 center, xi_v2 dimension, xi_f32 angle);

#endif  // XI_DRAW_H_
