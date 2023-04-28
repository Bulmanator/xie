#if !defined(XI_MATHS_H_)
#define XI_MATHS_H_

#if !defined(XI_EPSILON_F32)
    #define XI_EPSILON_F32 0.00001f
#endif

typedef struct xiRandomState {
    xi_u64 state;
} xiRandomState;

// rng xorshift
//
inline void xi_rng_seed(xiRandomState *rng, xi_u64 seed);

inline xi_u32 xi_rng_u32(xiRandomState *rng);
inline xi_u64 xi_rng_u64(xiRandomState *rng);

inline xi_f32 xi_rng_unilateral_f32(xiRandomState *rng); //  0 to 1
inline xi_f32 xi_rng_bilateral_f32(xiRandomState *rng);  // -1 to 1

inline xi_f64 xi_rng_unilateral_f64(xiRandomState *rng); //  0 to 1
inline xi_f64 xi_rng_bilateral_f64(xiRandomState *rng);  // -1 to 1

inline xi_u32 xi_rng_range_u32(xiRandomState *rng, xi_u32 min, xi_u32 max);
inline xi_f32 xi_rng_range_f32(xiRandomState *rng, xi_f32 min, xi_f32 max);

inline xi_u64 xi_rng_range_u64(xiRandomState *rng, xi_u64 min, xi_u64 max);
inline xi_f64 xi_rng_range_f64(xiRandomState *rng, xi_f64 min, xi_f64 max);

inline xi_u32 xi_rng_choice_u32(xiRandomState *rng, xi_u32 choice_count);

// sqrt
//
inline xi_f32 xi_sqrt(xi_f32 a);
inline xi_f32 xi_rsqrt(xi_f32 a);
inline xi_f32 xi_rsqrt_approx(xi_f32 a);

// trig. functions
//
inline xi_f32 xi_sin(xi_f32 a);
inline xi_f32 xi_cos(xi_f32 a);
inline xi_f32 xi_tan(xi_f32 a);

// lerp
//
inline xi_f32 xi_lerp_f32(xi_f32 a, xi_f32 b, xi_f32 t);
inline xi_f64 xi_lerp_f64(xi_f64 a, xi_f64 b, xi_f64 t);

inline xi_v2 xi_lerp_v2(xi_v2 a, xi_v2 b, xi_f32 t);
inline xi_v3 xi_lerp_v3(xi_v3 a, xi_v3 b, xi_f32 t);
inline xi_v4 xi_lerp_v4(xi_v4 a, xi_v4 b, xi_f32 t);

// create functions
//
inline xi_v2u xi_v2u_create(xi_u32 x, xi_u32 y);
inline xi_v2s xi_v2s_create(xi_s32 x, xi_s32 y);

inline xi_v2 xi_v2_create(xi_f32 x, xi_f32 y);
inline xi_v3 xi_v3_create(xi_f32 x, xi_f32 y, xi_f32 z);
inline xi_v4 xi_v4_create(xi_f32 x, xi_f32 y, xi_f32 z, xi_f32 w);

// conversion functions
//
// :note conversion functions that are not here can be retrieved directly from the struct via the union
// setup, i.e. v4 -> v3 conversion can be done via v4.xyz, v3 -> v2 conversion can be done via v3.xy
//
// float to integer vector conversion will truncate
//
inline xi_v2u xi_v2u_from_v2s(xi_v2s xy);
inline xi_v2u xi_v2u_from_v2(xi_v2 xy);

inline xi_v2s xi_v2s_from_v2u(xi_v2u xy);
inline xi_v2s xi_v2s_from_v2(xi_v2 xy);

inline xi_v2 xi_v2_from_v2u(xi_v2u xy);
inline xi_v2 xi_v2_from_v2s(xi_v2s xy);

inline xi_v3 xi_v3_from_v2(xi_v2  xy, xi_f32 z);

inline xi_v4 xi_v4_from_v2(xi_v2 xy, xi_f32 z, xi_f32 w);
inline xi_v4 xi_v4_from_v3(xi_v3 xyz, xi_f32 w);

// creates a 2x2 rotation matrix from the angle provided
//
inline xi_m2x2 xi_m2x2_from_radians(xi_f32 angle);

// creates a 4x4 matrix using the supplied axes as the first three rows/columns
//
inline xi_m4x4 xi_m4x4_from_rows_v3(xi_v3 x_axis, xi_v3 y_axis, xi_v3 z_axis);
inline xi_m4x4 xi_m4x4_from_columns_v3(xi_v3 x_axis, xi_v3 y_axis, xi_v3 z_axis);

// creates a 4x4 matrix camera trasform from the xyz axes and places it at the position specified
//
extern XI_API xi_m4x4_inv xi_m4x4_from_camera_transform(xi_v3 x_axis, xi_v3 y_axis, xi_v3 z_axis, xi_v3 position);

// dot
//
inline xi_f32 xi_v2_dot(xi_v2 a, xi_v2 b);
inline xi_f32 xi_v3_dot(xi_v3 a, xi_v3 b);
inline xi_f32 xi_v4_dot(xi_v4 a, xi_v4 b);

// length
//
inline xi_f32 xi_v2_length(xi_v2 a);
inline xi_f32 xi_v3_length(xi_v3 a);
inline xi_f32 xi_v4_length(xi_v4 a);

// normalize or zero vector if len == 0
//
inline xi_v2 xi_v2_noz(xi_v2 a);
inline xi_v3 xi_v3_noz(xi_v3 a);
inline xi_v4 xi_v4_noz(xi_v4 a);

// min and max for vectors
//
inline xi_v2 xi_v2_min(xi_v2 a, xi_v2 b);
inline xi_v3 xi_v3_min(xi_v3 a, xi_v3 b);
inline xi_v4 xi_v4_min(xi_v4 a, xi_v4 b);

inline xi_v2 xi_v2_max(xi_v2 a, xi_v2 b);
inline xi_v3 xi_v3_max(xi_v3 a, xi_v3 b);
inline xi_v4 xi_v4_max(xi_v4 a, xi_v4 b);

// misc vector functions
//
inline xi_v2 xi_v2_perp(xi_v2 a);

inline xi_v3 xi_v3_cross(xi_v3 a, xi_v3 b);

// operators
//
inline xi_v2u xi_v2u_add(xi_v2u a, xi_v2u b);
inline xi_v2s xi_v2s_add(xi_v2s a, xi_v2s b);

inline xi_v2 xi_v2_add(xi_v2 a, xi_v2 b);
inline xi_v3 xi_v3_add(xi_v3 a, xi_v3 b);
inline xi_v4 xi_v4_add(xi_v4 a, xi_v4 b);

inline xi_v2s xi_v2s_neg(xi_v2s a);

inline xi_v2  xi_v2_neg(xi_v2 a);
inline xi_v3  xi_v3_neg(xi_v3 a);
inline xi_v4  xi_v4_neg(xi_v4 a);

inline xi_v2u xi_v2u_sub(xi_v2u a, xi_v2u b);
inline xi_v2s xi_v2s_sub(xi_v2s a, xi_v2s b);

inline xi_v2 xi_v2_sub(xi_v2 a, xi_v2 b);
inline xi_v3 xi_v3_sub(xi_v3 a, xi_v3 b);
inline xi_v4 xi_v4_sub(xi_v4 a, xi_v4 b);

inline xi_v2 xi_v2_mul(xi_v2 a, xi_v2 b);
inline xi_v3 xi_v3_mul(xi_v3 a, xi_v3 b);
inline xi_v4 xi_v4_mul(xi_v4 a, xi_v4 b);

inline xi_m2x2 xi_m2x2_mul(xi_m2x2 a, xi_m2x2 b);
inline xi_m4x4 xi_m4x4_mul(xi_m4x4 a, xi_m4x4 b);

inline xi_v2 xi_v2_mul_f32(xi_v2 a, xi_f32 b);
inline xi_v3 xi_v3_mul_f32(xi_v3 a, xi_f32 b);
inline xi_v4 xi_v4_mul_f32(xi_v4 a, xi_f32 b);

inline xi_v2 xi_m2x2_mul_v2(xi_m2x2 a, xi_v2 b);
inline xi_v3 xi_m4x4_mul_v3(xi_m4x4 a, xi_v3 b);
inline xi_v4 xi_m4x4_mul_v4(xi_m4x4 a, xi_v4 b);

inline xi_v2 xi_v2_div(xi_v2 a, xi_v2 b);
inline xi_v3 xi_v3_div(xi_v3 a, xi_v3 b);
inline xi_v4 xi_v4_div(xi_v4 a, xi_v4 b);

inline xi_v2 xi_v2_div_f32(xi_v2 a, xi_f32 b);
inline xi_v3 xi_v3_div_f32(xi_v3 a, xi_f32 b);
inline xi_v4 xi_v4_div_f32(xi_v4 a, xi_f32 b);

// retrieve a single row/column from a matrix as a vector
//
inline xi_v4 xi_m4x4_row_get(xi_m4x4 m, xi_u32 row);
inline xi_v4 xi_m4x4_column_get(xi_m4x4 m, xi_u32 column);

// projection matrices
//
extern XI_API xi_m4x4_inv xi_m4x4_orthographic_projection(xi_f32 aspect_ratio,
        xi_f32 near_plane, xi_f32 far_plane);

extern XI_API xi_m4x4_inv xi_m4x4_perspective_projection(xi_f32 focal_length,
        xi_f32 aspect_ratio, xi_f32 near_plane, xi_f32 far_plane);

// transforms for m4x4
//
inline xi_m4x4 xi_m4x4_translate_v3(xi_m4x4 m, xi_v3 t);

inline xi_m4x4 xi_m4x4_x_axis_rotation(xi_f32 radians);
inline xi_m4x4 xi_m4x4_y_axis_rotation(xi_f32 radians);
inline xi_m4x4 xi_m4x4_z_axis_rotation(xi_f32 radians);

// pow2 functions
//
extern XI_API xi_u32 xi_pow2_next_u32(xi_u32 x);
extern XI_API xi_u32 xi_pow2_prev_u32(xi_u32 x);

extern XI_API xi_u32 xi_pow2_nearest_u32(xi_u32 x);

// packs v4 f32 colour into abgr order 8-bit per component u32, norm variant assumes [0-1] normalised
// range, unorm variant assumes unormalised [0-255] range
//
inline xi_u32 xi_v4_colour_abgr_pack_norm(xi_v4 c);
inline xi_u32 xi_v4_colour_abgr_pack_unorm(xi_v4 c);

// unpacks a packed 32-bit integer to normalised floating-point v4
//
inline xi_v4 xi_u32_colour_unpack_argb_norm(xi_u32 c);
inline xi_v4 xi_u32_colour_unpack_abgr_norm(xi_u32 c);

// converts a single colour component from srgb byte unorm to normalised light-linear float
//
extern XI_API xi_f32 xi_srgb_unorm_to_linear_norm(xi_u8 component);

// converts a single colour component from normalised light-linear float to srgb byte unorm
//
extern XI_API xi_u8 xi_linear_norm_to_srgb_unorm(xi_f32 component);

#endif  // XI_MATHS_H_
