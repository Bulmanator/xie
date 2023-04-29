// this is where all api functions marked as 'inline' are implemented, this file is directly included at the
// end of xi.h and thus is not compiled into the .dll/.so allowing the compiler freedom to easily inline
// these small scale functions
//

//
// :note implementation of xi_string.h inline functions
//

xi_string xi_str_wrap_count(xi_u8 *data, xi_uptr count) {
    xi_string result;
    result.count = count;
    result.data  = data;

    return result;
}

xi_string xi_str_wrap_range(xi_u8 *start, xi_u8 *end) {
    xi_string result;

    XI_ASSERT(start <= end);

    result.count = (xi_uptr) (end - start);
    result.data  = start;

    return result;
}

//
// :note implementation of xi_maths.h inline functions
//
#include <math.h>

// xorshift rng
//
void xi_rng_seed(xiRandomState *rng, xi_u64 seed) {
    rng->state = seed;
}

xi_u32 xi_rng_u32(xiRandomState *rng) {
    xi_u32 result = (xi_u32) xi_rng_u64(rng);
    return result;
}

xi_u64 xi_rng_u64(xiRandomState *rng) {
    xi_u64 result = rng->state;

    result ^= (result << 13);
    result ^= (result >>  7);
    result ^= (result << 17);

    rng->state = result;
    return result;
}

xi_f32 xi_rng_unilateral_f32(xiRandomState *rng) {
    xi_f32 result = (xi_f32) xi_rng_unilateral_f64(rng);
    return result;
}

xi_f32 xi_rng_bilateral_f32(xiRandomState *rng) {
    xi_f32 result = -1.0f + (2.0f * xi_rng_unilateral_f32(rng));
    return result;
}

xi_f64 xi_rng_unilateral_f64(xiRandomState *rng) {
    xi_f64 result = xi_rng_u64(rng) / (xi_f64) XI_U64_MAX;
    return result;
}

xi_f64 xi_rng_bilateral_f64(xiRandomState *rng) {
    xi_f64 result = -1.0f + (2.0f * xi_rng_unilateral_f64(rng));
    return result;
}

xi_u32 xi_rng_range_u32(xiRandomState *rng, xi_u32 min, xi_u32 max) {
    xi_u32 result = min + (xi_u32) ((xi_rng_unilateral_f32(rng) * (max - min)) + 0.5f);
    return result;
}

xi_f32 xi_rng_range_f32(xiRandomState *rng, xi_f32 min, xi_f32 max) {
    xi_f32 result = min + (xi_rng_unilateral_f32(rng) * (max - min));
    return result;
}

xi_u64 xi_rng_range_u64(xiRandomState *rng, xi_u64 min, xi_u64 max) {
    xi_u64 result = min + (xi_u64) ((xi_rng_unilateral_f64(rng) * (max - min)) + 0.5);
    return result;
}

xi_f64 xi_rng_range_f64(xiRandomState *rng, xi_f64 min, xi_f64 max) {
    xi_f64 result = min + (xi_rng_unilateral_f64(rng) * (max - min));
    return result;
}

xi_u32 xi_rng_choice_u32(xiRandomState *rng, xi_u32 choice_count) {
    xi_u32 result = xi_rng_u32(rng) % choice_count;
    return result;
}

// xi_sqrt and xi_rsqrt_approx are implemented using simd intrinsics below
// :simd_maths
//
xi_f32 xi_rsqrt(xi_f32 a) {
    xi_f32 result = 1.0f / xi_sqrt(a);
    return result;
}

// trig. functions
//
// @todo: we should replace these with our own functions so we can change from radians to turns
//
xi_f32 xi_sin(xi_f32 a) {
    xi_f32 result = sinf(a);
    return result;
}

xi_f32 xi_cos(xi_f32 a) {
    xi_f32 result = cosf(a);
    return result;
}

xi_f32 xi_tan(xi_f32 a) {
    xi_f32 result = tanf(a);
    return result;
}

// lerp
//
xi_f32 xi_lerp_f32(xi_f32 a, xi_f32 b, xi_f32 t) {
    xi_f32 result = (a * (1.0f - t)) + (b * t);
    return result;
}

xi_f64 xi_lerp_f64(xi_f64 a, xi_f64 b, xi_f64 t) {
    xi_f64 result = (a * (1.0 - t)) + (b * t);
    return result;
}

xi_v2 xi_lerp_v2(xi_v2 a, xi_v2 b, xi_f32 t) {
    xi_v2 result;
    result.x = xi_lerp_f32(a.x, b.x, t);
    result.y = xi_lerp_f32(a.y, b.y, t);

    return result;
}

xi_v3 xi_lerp_v3(xi_v3 a, xi_v3 b, xi_f32 t) {
    xi_v3 result;
    result.x = xi_lerp_f32(a.x, b.x, t);
    result.y = xi_lerp_f32(a.y, b.y, t);
    result.z = xi_lerp_f32(a.z, b.z, t);

    return result;
}

xi_v4 xi_lerp_v4(xi_v4 a, xi_v4 b, xi_f32 t) {
    xi_v4 result;
    result.x = xi_lerp_f32(a.x, b.x, t);
    result.y = xi_lerp_f32(a.y, b.y, t);
    result.z = xi_lerp_f32(a.z, b.z, t);
    result.w = xi_lerp_f32(a.w, b.w, t);

    return result;
}

// create functions
//
xi_v2u xi_v2u_create(xi_u32 x, xi_u32 y) {
    xi_v2u result;
    result.x = x;
    result.y = y;

    return result;
}

xi_v2s xi_v2s_create(xi_s32 x, xi_s32 y) {
    xi_v2s result;
    result.x = x;
    result.y = y;

    return result;
}

xi_v2 xi_v2_create(xi_f32 x, xi_f32 y) {
    xi_v2 result;
    result.x = x;
    result.y = y;

    return result;
}

xi_v3 xi_v3_create(xi_f32 x, xi_f32 y, xi_f32 z) {
    xi_v3 result;
    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

xi_v4 xi_v4_create(xi_f32 x, xi_f32 y, xi_f32 z, xi_f32 w) {
    xi_v4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

// conversion functions
//
xi_v2u xi_v2u_from_v2s(xi_v2s xy) {
    xi_v2u result;
    result.x = (xi_u32) xy.x;
    result.y = (xi_u32) xy.y;

    return result;
}

xi_v2u xi_v2u_from_v2(xi_v2 xy) {
    xi_v2u result;
    result.x = (xi_u32) xy.x;
    result.y = (xi_u32) xy.y;

    return result;
}

xi_v2s xi_v2s_from_v2u(xi_v2u xy) {
    xi_v2s result;
    result.x = (xi_s32) xy.x;
    result.y = (xi_s32) xy.y;

    return result;
}

xi_v2s xi_v2s_from_v2(xi_v2 xy) {
    xi_v2s result;
    result.x = (xi_s32) xy.x;
    result.y = (xi_s32) xy.y;

    return result;
}

xi_v2 xi_v2_from_v2u(xi_v2u xy) {
    xi_v2 result;
    result.x = (xi_f32) xy.x;
    result.y = (xi_f32) xy.y;

    return result;
}

xi_v2 xi_v2_from_v2s(xi_v2s xy) {
    xi_v2 result;
    result.x = (xi_f32) xy.x;
    result.y = (xi_f32) xy.y;

    return result;
}

xi_v3 xi_v3_from_v2(xi_v2 xy, xi_f32 z) {
    xi_v3 result;
    result.x = xy.x;
    result.y = xy.y;
    result.z = z;

    return result;
}

xi_v4 xi_v4_from_v2(xi_v2 xy, xi_f32 z, xi_f32 w) {
    xi_v4 result;
    result.x = xy.x;
    result.y = xy.y;
    result.z = z;
    result.w = w;

    return result;
}

xi_v4 xi_v4_from_v3(xi_v3 xyz, xi_f32 w) {
    xi_v4 result;
    result.x = xyz.x;
    result.y = xyz.y;
    result.z = xyz.z;
    result.w = w;

    return result;
}

xi_m2x2 xi_m2x2_from_radians(xi_f32 angle) {
    xi_m2x2 result;

    xi_f32 s = xi_sin(angle);
    xi_f32 c = xi_cos(angle);

    result.m[0][0] =  c;
    result.m[0][1] = -s;
    result.m[1][0] =  s;
    result.m[1][1] =  c;

    return result;
}

xi_m4x4 xi_m4x4_from_rows_v3(xi_v3 x_axis, xi_v3 y_axis, xi_v3 z_axis) {
    xi_m4x4 result = {
        x_axis.x, x_axis.y, x_axis.z, 0,
        y_axis.x, y_axis.y, y_axis.z, 0,
        z_axis.x, z_axis.y, z_axis.z, 0,
        0,        0,        0,        1
    };

    return result;
}

xi_m4x4 xi_m4x4_from_columns_v3(xi_v3 x_axis, xi_v3 y_axis, xi_v3 z_axis) {
    xi_m4x4 result = {
        x_axis.x, y_axis.x, z_axis.x, 0,
        x_axis.y, y_axis.y, z_axis.y, 0,
        x_axis.z, y_axis.z, z_axis.z, 0,
        0,        0,        0,        1
    };

    return result;
}

// dot
//
xi_f32 xi_v2_dot(xi_v2 a, xi_v2 b) {
    xi_f32 result = (a.x * b.x) + (a.y * b.y);
    return result;
}

xi_f32 xi_v3_dot(xi_v3 a, xi_v3 b) {
    xi_f32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    return result;
}

xi_f32 xi_v4_dot(xi_v4 a, xi_v4 b) {
    xi_f32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    return result;
}

// length
//
xi_f32 xi_v2_length(xi_v2 a) {
    xi_f32 result = xi_sqrt(xi_v2_dot(a, a));
    return result;
}

xi_f32 xi_v3_length(xi_v3 a) {
    xi_f32 result = xi_sqrt(xi_v3_dot(a, a));
    return result;
}

xi_f32 xi_v4_length(xi_v4 a) {
    xi_f32 result = xi_sqrt(xi_v4_dot(a, a));
    return result;
}

// normalize or zero vector if len = 0
//
xi_v2 xi_v2_noz(xi_v2 a) {
    xi_v2 result = { 0 };

    xi_f32 len_sq = xi_v2_dot(a, a);
    if (len_sq > (XI_EPSILON_F32 * XI_EPSILON_F32)) {
        result = xi_v2_mul_f32(a, 1.0f / xi_sqrt(len_sq));
    }

    return result;
}

xi_v3 xi_v3_noz(xi_v3 a) {
    xi_v3 result = { 0 };

    xi_f32 len_sq = xi_v3_dot(a, a);
    if (len_sq > (XI_EPSILON_F32 * XI_EPSILON_F32)) {
        result = xi_v3_mul_f32(a, 1.0f / xi_sqrt(len_sq));
    }

    return result;
}

xi_v4 xi_v4_noz(xi_v4 a) {
    xi_v4 result = { 0 };

    xi_f32 len_sq = xi_v4_dot(a, a);
    if (len_sq > (XI_EPSILON_F32 * XI_EPSILON_F32)) {
        result = xi_v4_mul_f32(a, 1.0f / xi_sqrt(len_sq));
    }

    return result;
}

// min and max for vectors
//
xi_v2 xi_v2_min(xi_v2 a, xi_v2 b) {
    xi_v2 result;
    result.x = XI_MIN(a.x, b.x);
    result.y = XI_MIN(a.y, b.y);

    return result;
}

xi_v3 xi_v3_min(xi_v3 a, xi_v3 b) {
    xi_v3 result;
    result.x = XI_MIN(a.x, b.x);
    result.y = XI_MIN(a.y, b.y);
    result.z = XI_MIN(a.z, b.z);

    return result;
}

xi_v4 xi_v4_min(xi_v4 a, xi_v4 b) {
    xi_v4 result;
    result.x = XI_MIN(a.x, b.x);
    result.y = XI_MIN(a.y, b.y);
    result.z = XI_MIN(a.z, b.z);
    result.w = XI_MIN(a.w, b.w);

    return result;
}

xi_v2 xi_v2_max(xi_v2 a, xi_v2 b) {
    xi_v2 result;
    result.x = XI_MAX(a.x, b.x);
    result.y = XI_MAX(a.y, b.y);

    return result;
}

xi_v3 xi_v3_max(xi_v3 a, xi_v3 b) {
    xi_v3 result;
    result.x = XI_MAX(a.x, b.x);
    result.y = XI_MAX(a.y, b.y);
    result.z = XI_MAX(a.z, b.z);

    return result;
}

xi_v4 xi_v4_max(xi_v4 a, xi_v4 b) {
    xi_v4 result;
    result.x = XI_MAX(a.x, b.x);
    result.y = XI_MAX(a.y, b.y);
    result.z = XI_MAX(a.z, b.z);
    result.w = XI_MAX(a.w, b.w);

    return result;
}

// misc vector functions
//
xi_v2 xi_v2_perp(xi_v2 a) {
    xi_v2 result = xi_v2_create(-a.y, a.x);
    return result;
}

xi_v3 xi_v3_cross(xi_v3 a, xi_v3 b) {
    xi_v3 result;
    result.x = (a.y * b.z) - (a.z * b.y);
    result.y = (a.z * b.x) - (a.z * b.x);
    result.z = (a.x * b.y) - (a.y * b.x);

    return result;
}

// operators
//
xi_v2u xi_v2u_add(xi_v2u a, xi_v2u b) {
    xi_v2u result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);

    return result;
}

xi_v2s xi_v2s_add(xi_v2s a, xi_v2s b) {
    xi_v2s result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);

    return result;
}

xi_v2 xi_v2_add(xi_v2 a, xi_v2 b) {
    xi_v2 result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);

    return result;
}

xi_v3 xi_v3_add(xi_v3 a, xi_v3 b) {
    xi_v3 result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);
    result.z = (a.z + b.z);

    return result;
}

xi_v4 xi_v4_add(xi_v4 a, xi_v4 b) {
    xi_v4 result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);
    result.z = (a.z + b.z);
    result.w = (a.w + b.w);

    return result;
}

xi_v2s xi_v2s_neg(xi_v2s a) {
    xi_v2s result;
    result.x = -a.x;
    result.y = -a.y;

    return result;
}

xi_v2  xi_v2_neg(xi_v2 a) {
    xi_v2 result;
    result.x = -a.x;
    result.y = -a.y;

    return result;
}

xi_v3  xi_v3_neg(xi_v3 a) {
    xi_v3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return result;
}

xi_v4  xi_v4_neg(xi_v4 a) {
    xi_v4 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;

    return result;
}

xi_v2u xi_v2u_sub(xi_v2u a, xi_v2u b) {
    xi_v2u result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);

    return result;
}

xi_v2s xi_v2s_sub(xi_v2s a, xi_v2s b) {
    xi_v2s result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);

    return result;
}

xi_v2 xi_v2_sub(xi_v2 a, xi_v2 b) {
    xi_v2 result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);

    return result;
}

xi_v3 xi_v3_sub(xi_v3 a, xi_v3 b) {
    xi_v3 result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);
    result.z = (a.z - b.z);

    return result;
}

xi_v4 xi_v4_sub(xi_v4 a, xi_v4 b) {
    xi_v4 result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);
    result.z = (a.z - b.z);
    result.w = (a.w - b.w);

    return result;
}

xi_v2 xi_v2_mul(xi_v2 a, xi_v2 b) {
    xi_v2 result;
    result.x = (a.x * b.x);
    result.y = (a.y * b.y);

    return result;
}

xi_v3 xi_v3_mul(xi_v3 a, xi_v3 b) {
    xi_v3 result;
    result.x = (a.x * b.x);
    result.y = (a.y * b.y);
    result.z = (a.z * b.z);

    return result;
}

xi_v4 xi_v4_mul(xi_v4 a, xi_v4 b) {
    xi_v4 result;
    result.x = (a.x * b.x);
    result.y = (a.y * b.y);
    result.z = (a.z * b.z);
    result.w = (a.w * b.w);

    return result;
}

xi_m2x2 xi_m2x2_mul(xi_m2x2 a, xi_m2x2 b) {
    xi_m2x2 result;
    result.m[0][0] = (a.m[0][0] * b.m[0][0]) + (a.m[0][1] * b.m[1][0]);
    result.m[0][1] = (a.m[0][0] * b.m[0][1]) + (a.m[0][1] * b.m[1][1]);
    result.m[1][0] = (a.m[1][0] * b.m[0][0]) + (a.m[1][1] * b.m[1][0]);
    result.m[1][1] = (a.m[1][0] * b.m[0][1]) + (a.m[0][1] * b.m[1][1]);

    return result;
}

xi_m4x4 xi_m4x4_mul(xi_m4x4 a, xi_m4x4 b) {
    // @todo: simd
    //
    xi_m4x4 result;

    for (xi_u32 r = 0; r < 4; ++r) {
        for (xi_u32 c = 0; c < 4; ++c) {
            result.m[r][c] =
                (a.m[r][0] * b.m[0][c]) + (a.m[r][1] * b.m[1][c]) +
                (a.m[r][2] * b.m[2][c]) + (a.m[r][3] * b.m[3][c]);
        }
    }

    return result;
}

xi_v2 xi_v2_mul_f32(xi_v2 a, xi_f32 b) {
    xi_v2 result;
    result.x = (a.x * b);
    result.y = (a.y * b);

    return result;
}

xi_v3 xi_v3_mul_f32(xi_v3 a, xi_f32 b) {
    xi_v3 result;
    result.x = (a.x * b);
    result.y = (a.y * b);
    result.z = (a.z * b);

    return result;
}

xi_v4 xi_v4_mul_f32(xi_v4 a, xi_f32 b) {
    xi_v4 result;
    result.x = (a.x * b);
    result.y = (a.y * b);
    result.z = (a.z * b);
    result.w = (a.w * b);

    return result;
}

xi_v2 xi_m2x2_mul_v2(xi_m2x2 a, xi_v2 b) {
    xi_v2 result;
    result.x = (a.m[0][0] * b.x) + (a.m[0][1] * b.y);
    result.y = (a.m[1][0] * b.x) + (a.m[1][1] * b.y);

    return result;
}

xi_v3 xi_m4x4_mul_v3(xi_m4x4 a, xi_v3 b) {
    xi_v3 result = xi_m4x4_mul_v4(a, xi_v4_from_v3(b, 1.0f)).xyz;
    return result;
}

xi_v4 xi_m4x4_mul_v4(xi_m4x4 a, xi_v4 b) {
    // @todo: simd?
    //
    xi_v4 result;
    result.x = xi_v4_dot(a.r[0], b);
    result.y = xi_v4_dot(a.r[1], b);
    result.z = xi_v4_dot(a.r[2], b);
    result.w = xi_v4_dot(a.r[3], b);

    return result;
}

xi_v2 xi_v2_div(xi_v2 a, xi_v2 b) {
    xi_v2 result;
    result.x = (a.x / b.x);
    result.y = (a.y / b.y);

    return result;
}

xi_v3 xi_v3_div(xi_v3 a, xi_v3 b) {
    xi_v3 result;
    result.x = (a.x / b.x);
    result.y = (a.y / b.y);
    result.z = (a.z / b.z);

    return result;
}

xi_v4 xi_v4_div(xi_v4 a, xi_v4 b) {
    xi_v4 result;
    result.x = (a.x / b.x);
    result.y = (a.y / b.y);
    result.z = (a.z / b.z);
    result.w = (a.w / b.w);

    return result;
}

xi_v2 xi_v2_div_f32(xi_v2 a, xi_f32 b) {
    xi_v2 result;
    result.x = (a.x / b);
    result.y = (a.y / b);

    return result;
}

xi_v3 xi_v3_div_f32(xi_v3 a, xi_f32 b) {
    xi_v3 result;
    result.x = (a.x / b);
    result.y = (a.y / b);
    result.z = (a.z / b);

    return result;
}

xi_v4 xi_v4_div_f32(xi_v4 a, xi_f32 b) {
    xi_v4 result;
    result.x = (a.x / b);
    result.y = (a.y / b);
    result.z = (a.z / b);
    result.w = (a.w / b);

    return result;
}

xi_v4 xi_m4x4_row_get(xi_m4x4 m, xi_u32 row) {
    XI_ASSERT(row < 4);

    xi_v4 result = m.r[row];
    return result;
}

xi_v4 xi_m4x4_column_get(xi_m4x4 m, xi_u32 column) {
    XI_ASSERT(column < 4);

    xi_v4 result = xi_v4_create(m.m[0][column], m.m[1][column], m.m[2][column], m.m[3][column]);
    return result;
}

xi_m4x4 xi_m4x4_translate_v3(xi_m4x4 m, xi_v3 t) {
    xi_m4x4 result = m;
    result.m[0][3] += t.x;
    result.m[1][3] += t.y;
    result.m[2][3] += t.z;

    return result;
}

xi_m4x4 xi_m4x4_x_axis_rotation(xi_f32 radians) {
    xi_f32 s = xi_sin(radians);
    xi_f32 c = xi_cos(radians);

    xi_m4x4 result = {
        1, 0,  0, 0,
        0, c, -s, 0,
        0, s,  c, 0,
        0, 0,  0, 0
    };

    return result;
}

xi_m4x4 xi_m4x4_y_axis_rotation(xi_f32 radians) {
    xi_f32 s = xi_sin(radians);
    xi_f32 c = xi_cos(radians);

    xi_m4x4 result = {
         c, 0, s, 0,
         0, 1, 0, 0,
        -s, 0, c, 0,
         0, 0, 0, 0
    };

    return result;
}

xi_m4x4 xi_m4x4_z_axis_rotation(xi_f32 radians) {
    xi_f32 s = xi_sin(radians);
    xi_f32 c = xi_cos(radians);

    xi_m4x4 result = {
        c, -s, 0, 0,
        s,  c, 0, 0,
        0,  0, 1, 0,
        0,  0, 0, 0
    };

    return result;
}

// colour packing
//
xi_u32 xi_v4_colour_abgr_pack_norm(xi_v4 v) {
    xi_u32 result =
        (((xi_u32) (255.0f * v.a) & 0xFF) << 24) |
        (((xi_u32) (255.0f * v.b) & 0xFF) << 16) |
        (((xi_u32) (255.0f * v.g) & 0xFF) <<  8) |
        (((xi_u32) (255.0f * v.r) & 0xFF) <<  0);

    return result;
}

xi_u32 xi_v4_colour_abgr_pack_unorm(xi_v4 v) {
    xi_u32 result =
        (((xi_u32) v.a & 0xFF) << 24) |
        (((xi_u32) v.b & 0xFF) << 16) |
        (((xi_u32) v.g & 0xFF) <<  8) |
        (((xi_u32) v.r & 0xFF) <<  0);

    return result;
}

xi_v4 xi_u32_colour_unpack_argb_norm(xi_u32 c) {
    xi_v4 result;
    result.a = ((c >> 24) & 0xFF) / 255.0f;
    result.r = ((c >> 16) & 0xFF) / 255.0f;
    result.g = ((c >>  8) & 0xFF) / 255.0f;
    result.b = ((c >>  0) & 0xFF) / 255.0f;

    return result;
}

xi_v4 xi_u32_colour_unpack_abgr_norm(xi_u32 c) {
    xi_v4 result;
    result.a = ((c >> 24) & 0xFF) / 255.0f;
    result.b = ((c >> 16) & 0xFF) / 255.0f;
    result.g = ((c >>  8) & 0xFF) / 255.0f;
    result.r = ((c >>  0) & 0xFF) / 255.0f;

    return result;
}

// :simd_maths
//
#if XI_ARCH_AMD64

xi_f32 xi_sqrt(xi_f32 a) {
    xi_f32 result = _mm_cvtss_f32(_mm_sqrt_ps(_mm_set1_ps(a)));
    return result;
}

xi_f32 xi_rsqrt_approx(xi_f32 a) {
    xi_f32 result = _mm_cvtss_f32(_mm_rsqrt_ps(_mm_set1_ps(a)));
    return result;
}

#elif XI_ARCH_AARCH64
    #error "incomplete implementation"
#endif

// for linux we bootstrap load the xie_run function directly from the .so file
// rather than linking with it becasue linking with shared objects on linux comes
// with a whole host of problems
//
#if XI_OS_LINUX

#include <dlfcn.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>

#include <stdio.h>

int xie_run(xiGameCode *code) {
    int result = 1;

    int (*__xie_bootstrap_run)(xiGameCode *);
    void *lib;

    // attempt to load a global install, if fails fallback to load the local
    // install in the same directory as the executable.
    //
    lib = dlopen("libxid.so", RTLD_NOW | RTLD_GLOBAL);
    if (!lib) {
        char path[PATH_MAX + 1];
        ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);
        if (len > 0) {
            while ((len > 0) && (path[len] != '/')) {
                len -= 1;
            }

            path[len + 1] = 0; // now basedir with terminating slash
            strcat(path, "libxid.so");

            lib = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
        }
    }

    if (lib != 0) {
        __xie_bootstrap_run = dlsym(lib, "__xie_bootstrap_run");
        if (__xie_bootstrap_run != 0) {
            result = __xie_bootstrap_run(code);
        }
        else {
            printf("[internal error] :: failed to load '__xie_bootstrap_run' (%s)\n", dlerror());
        }
    }
    else {
        printf("[internal error] :: failed to load libxid.so (%s)\n", dlerror());
    }

    return result;
}

#endif
