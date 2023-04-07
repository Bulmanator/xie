#if !defined(XI_MATHS_H_)
#define XI_MATHS_H_

extern XI_API xi_u32 xi_pow2_next_u32(xi_u32 x);
extern XI_API xi_u32 xi_pow2_prev_u32(xi_u32 x);

extern XI_API xi_u32 xi_pow2_nearest_u32(xi_u32 x);

// converts a single colour component from srgb byte unorm to normalised light-linear float
//
extern XI_API xi_f32 xi_srgb_u8_to_linear_f32(xi_u8 component);

// converts a single colour component from normalised light-linear float to srgb byte unorm
//
extern XI_API xi_u8 xi_linear_f32_to_srgb_u8(xi_f32 component);

#endif  // XI_MATHS_H_
