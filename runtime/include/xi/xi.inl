// this is where all api functions marked as 'inline' are implemented, this file is directly included at the
// end of xi.h and thus is not compiled into the .dll/.so allowing the compiler freedom to easily inline
// these small scale functions
//

//
// :note implementation of xi_string.h inline functions
//

Str8 Str8_WrapCount(U8 *data, S64 count) {
    Str8 result;
    result.count = count;
    result.data  = data;

    return result;
}

Str8 Str8_WrapRange(U8 *start, U8 *end) {
    Str8 result;
    result.count = cast(S64) (end - start);
    result.data  = start;

    return result;
}

//
// --------------------------------------------------------------------------------
// :Random_Number_Generator
// --------------------------------------------------------------------------------
//

#include <math.h>

RandomState RandomSeed(U64 seed) {
    RandomState result;
    result.state = seed;

    return result;
}

U32 U32_RandomNext(RandomState *rng) {
    U32 result = cast(U32) U64_RandomNext(rng);
    return result;
}

U64 U64_RandomNext(RandomState *rng) {
    U64 result = rng->state;

    result ^= (result << 13);
    result ^= (result >>  7);
    result ^= (result << 17);

    rng->state = result;

    return result;
}

F32 F32_RandomUnilateral(RandomState *rng) {
    F32 result = cast(F32) F64_RandomUnilateral(rng);
    return result;
}

F64 F64_RandomUnilateral(RandomState *rng) {
    F64 result = U64_RandomNext(rng) / cast(F64) U64_MAX;
    return result;
}

F32 F32_RandomBilateral(RandomState *rng) {
    F32 result = -1.0f + (2.0f * F32_RandomUnilateral(rng));
    return result;
}

F64 F64_RandomBilateral(RandomState *rng) {
    F64 result = -1.0 + (2.0 * F64_RandomUnilateral(rng));
    return result;
}

U32 U32_RandomRange(RandomState *rng, U32 min, U32 max) {
    U32 result = min + cast(U32) ((F32_RandomUnilateral(rng) * (max - min)) + 0.5f);
    return result;
}

S32 S32_RandomRange(RandomState *rng, S32 min, S32 max) {
    S32 result = min + cast(S32) ((F32_RandomUnilateral(rng) * (max - min)) + 0.5f);
    return result;
}

U64 U64_RandomRange(RandomState *rng, U64 min, U64 max) {
    U64 result = min + cast(U64) ((F64_RandomUnilateral(rng) * (max - min)) + 0.5);
    return result;
}

S64 S64_RandomRange(RandomState *rng, S64 min, S64 max) {
    S64 result = min + cast(S64) ((F64_RandomUnilateral(rng) * (max - min)) + 0.5);
    return result;
}

F32 F32_RandomRange(RandomState *rng, F32 min, F32 max) {
    F32 result = min + (F32_RandomUnilateral(rng) * (max - min));
    return result;
}

F64 F64_RandomRange(RandomState *rng, F64 min, F64 max) {
    F64 result = min + (F64_RandomUnilateral(rng) * (max - min));
    return result;
}

U32 U32_RandomChoice(RandomState *rng, U32 count) {
    Assert(count != 0);

    U32 result = U32_RandomNext(rng) % count;
    return result;
}

U64 U64_RandomChoice(RandomState *rng, U64 count) {
    Assert(count != 0);

    U64 result = U64_RandomNext(rng) % count;
    return result;
}

//
// --------------------------------------------------------------------------------
// :Basic
// --------------------------------------------------------------------------------
//

// F32Sqrt and F32InvSqrtApprox are implemented using simd intrinsics below :simd_maths
//
F32 F32_InvSqrt(F32 a) {
    F32 result = 1.0f / F32_Sqrt(a);
    return result;
}

F32 F32_Sin(F32 turns) {
    F32 result = sinf(F32_TAU * turns);
    return result;
}

F32 F32_Cos(F32 turns) {
    F32 result = cosf(F32_TAU * turns);
    return result;
}

F32 F32_Tan(F32 turns) {
    F32 result = tanf(F32_TAU * turns);
    return result;
}

F32 F32_Lerp(F32 a, F32 b, F32 t) {
    F32 result = a + ((b - a) * t);
    return result;
}

F64 F64_Lerp(F64 a, F64 b, F64 t) {
    F64 result = a + ((b - a) * t);
    return result;
}

Vec2F V2F_Lerp(Vec2F a, Vec2F b, F32 t) {
    Vec2F result;
    result.x = F32_Lerp(a.x, b.x, t);
    result.y = F32_Lerp(a.y, b.y, t);

    return result;
}

Vec3F V3F_Lerp(Vec3F a, Vec3F b, F32 t) {
    Vec3F result;
    result.x = F32_Lerp(a.x, b.x, t);
    result.y = F32_Lerp(a.y, b.y, t);
    result.z = F32_Lerp(a.z, b.z, t);

    return result;
}

Vec4F V4F_Lerp(Vec4F a, Vec4F b, F32 t) {
    Vec4F result;
    result.x = F32_Lerp(a.x, b.x, t);
    result.y = F32_Lerp(a.y, b.y, t);
    result.z = F32_Lerp(a.z, b.z, t);
    result.w = F32_Lerp(a.w, b.w, t);

    return result;
}

U32 U32_Pow2Next(U32 x) {
    U32 result = x;

    // @todo: how do we want this to work with inputs that are already powers of 2,
    // currently it just returns the value itself, but maybe we want to shift it up by
    // 1 in this case
    //

    result--;
    result |= (result >>  1);
    result |= (result >>  2);
    result |= (result >>  4);
    result |= (result >>  8);
    result |= (result >> 16);
    result++;

    return result;
}

U32 U32_Pow2Prev(U32 x) {
    U32 result = U32_Pow2Next(x) >> 1;
    return result;
}

U32 U32_Pow2Nearest(U32 x) {
    U32 next = U32_Pow2Next(x);
    U32 prev = next >> 1;

    U32 result = (next - x) > (x - prev) ? prev : next;
    return result;
}

//
// --------------------------------------------------------------------------------
// :Construction
// --------------------------------------------------------------------------------
//

Vec2U V2U(U32 x, U32 y) {
    Vec2U result;
    result.x = x;
    result.y = y;

    return result;
}

Vec2S V2S(S32 x, S32 y) {
    Vec2S result;
    result.x = x;
    result.y = y;

    return result;
}

Vec2F V2F(F32 x, F32 y) {
    Vec2F result;
    result.x = x;
    result.y = y;

    return result;
}

Vec3F V3F(F32 x, F32 y, F32 z) {
    Vec3F result;
    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

Vec4F V4F(F32 x, F32 y, F32 z, F32 w) {
    Vec4F result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

Vec2F V2F_UnitArm(F32 turn) {
    Vec2F result;
    result.x = F32_Cos(turn);
    result.y = F32_Sin(turn);

    return result;
}

Mat4x4F M4x4F(F32 diagonal) {
    Mat4x4F result = {
        diagonal, 0, 0, 0,
        0, diagonal, 0, 0,
        0, 0, diagonal, 0,
        0, 0, 0, diagonal
    };

    return result;
}

Mat4x4F M4x4F_Rows(Vec3F x, Vec3F y, Vec3F z) {
    Mat4x4F result = {
        x.x, x.y, x.z, 0,
        y.x, y.y, y.z, 0,
        z.x, z.y, z.z, 0,
        0,   0,   0,   1
    };

    return result;
}

Mat4x4F M4x4F_Columns(Vec3F x, Vec3F y, Vec3F z) {
    Mat4x4F result = {
        x.x, y.x, z.x, 0,
        x.y, y.y, z.y, 0,
        x.z, y.z, z.z, 0,
        0,   0,   0,   1
    };

    return result;
}

Mat4x4F M4x4F_RotationX(F32 turns) {
    F32 c = F32_Cos(turns);
    F32 s = F32_Sin(turns);

    Mat4x4F result = {
        1, 0,  0, 0,
        0, c, -s, 0,
        0, s,  c, 0,
        0, 0,  0, 1
    };

    return result;
}

Mat4x4F M4x4F_RotationY(F32 turns) {
    F32 c = F32_Cos(turns);
    F32 s = F32_Sin(turns);

    Mat4x4F result = {
         c, 0, s, 0,
         0, 1, 0, 0,
        -s, 0, c, 0,
         0, 0, 0, 1
    };

    return result;
}

Mat4x4F M4x4F_RotationZ(F32 turns) {
    F32 c = F32_Cos(turns);
    F32 s = F32_Sin(turns);

    Mat4x4F result = {
        c, -s, 0, 0,
        s,  c, 0, 0,
        0,  0, 1, 0,
        0,  0, 0, 1
    };

    return result;
}

//
// --------------------------------------------------------------------------------
// :Conversion
// --------------------------------------------------------------------------------
//

Vec2F V2F_FromV2U(Vec2U a) {
    Vec2F result;
    result.x = cast(F32) a.x;
    result.y = cast(F32) a.y;

    return result;
}

Vec2F V2F_FromV2S(Vec2S a) {
    Vec2F result;
    result.x = cast(F32) a.x;
    result.y = cast(F32) a.y;

    return result;
}

Vec2S V2S_FromV2U(Vec2U a) {
    Vec2S result;
    result.x = cast(S32) a.x;
    result.y = cast(S32) a.y;

    return result;
}

Vec2S V2S_FromV2F(Vec2F a) {
    Vec2S result;
    result.x = cast(S32) a.x;
    result.y = cast(S32) a.y;

    return result;
}

Vec2U V2U_FromV2S(Vec2S a) {
    Vec2U result;
    result.x = cast(U32) a.x;
    result.y = cast(U32) a.y;

    return result;
}

Vec2U V2U_FromV2F(Vec2F a) {
    Vec2U result;
    result.x = cast(U32) a.x;
    result.y = cast(U32) a.y;

    return result;
}

Vec3F V3F_FromV2F(Vec2F xy,  F32 z) {
    Vec3F result;
    result.xy = xy;
    result.z  = z;

    return result;
}

Vec4F V4F_FromV2F(Vec2F xy,  F32 z, F32 w) {
    Vec4F result;
    result.xy = xy;
    result.z  = z;
    result.w  = w;

    return result;
}

Vec4F V4F_FromV3F(Vec3F xyz, F32 w) {
    Vec4F result;
    result.xyz = xyz;
    result.w   = w;

    return result;
}

Vec4F M4x4F_RowExtract(Mat4x4F a, U32 row) {
    Assert(row < 4);

    Vec4F result = a.r[row];
    return result;
}

Vec4F M4x4F_ColumnExtract(Mat4x4F a, U32 column) {
    Assert(column < 4);

    Vec4F result = V4F(a.m[0][column], a.m[1][column], a.m[2][column], a.m[3][column]);
    return result;
}

U32 V4F_ColourPackABGR(Vec4F colour) {
    U32 result =
        (((U32) (255.0f * colour.a) & 0xFF) << 24) |
        (((U32) (255.0f * colour.b) & 0xFF) << 16) |
        (((U32) (255.0f * colour.g) & 0xFF) <<  8) |
        (((U32) (255.0f * colour.r) & 0xFF) <<  0);

    return result;
}

Vec4F U32_ColourUnpackARGB(U32 colour) {
    Vec4F result;
    result.a = ((colour >> 24) & 0xFF) / 255.0f;
    result.r = ((colour >> 16) & 0xFF) / 255.0f;
    result.g = ((colour >>  8) & 0xFF) / 255.0f;
    result.b = ((colour >>  0) & 0xFF) / 255.0f;

    return result;
}

Vec4F U32_ColourUnpackABGR(U32 colour) {
    Vec4F result;
    result.a = ((colour >> 24) & 0xFF) / 255.0f;
    result.b = ((colour >> 16) & 0xFF) / 255.0f;
    result.g = ((colour >>  8) & 0xFF) / 255.0f;
    result.r = ((colour >>  0) & 0xFF) / 255.0f;

    return result;
}

//
// --------------------------------------------------------------------------------
// :Vector_Basic
// --------------------------------------------------------------------------------
//

F32 V2F_Dot(Vec2F a, Vec2F b) {
    F32 result = (a.x * b.x) + (a.y * b.y);
    return result;
}

F32 V3F_Dot(Vec3F a, Vec3F b) {
    F32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    return result;
}

F32 V4F_Dot(Vec4F a, Vec4F b) {
    F32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    return result;
}

F32 V2F_LengthSq(Vec2F a) {
    F32 result = V2F_Dot(a, a);
    return result;
}

F32 V3F_LengthSq(Vec3F a) {
    F32 result = V3F_Dot(a, a);
    return result;
}

F32 V4F_LengthSq(Vec4F a) {
    F32 result = V4F_Dot(a, a);
    return result;
}

F32 V2F_Length(Vec2F a) {
    F32 result = F32_Sqrt(V2F_Dot(a, a));
    return result;
}

F32 V3F_Length(Vec3F a) {
    F32 result = F32_Sqrt(V3F_Dot(a, a));
    return result;
}

F32 V4F_Length(Vec4F a) {
    F32 result = F32_Sqrt(V4F_Dot(a, a));
    return result;
}

Vec2F V2F_Normalise(Vec2F a) {
    Vec2F result = V2F(0, 0);

    F32 len = V2F_Length(a);
    if (len != 0.0) {
        result = V2F_Scale(a, 1.0f / len);
    }

    return result;
}

Vec3F V3F_Normalise(Vec3F a) {
    Vec3F result = V3F(0, 0, 0);

    F32 len = V3F_Length(a);
    if (len != 0.0) {
        result = V3F_Scale(a, 1.0f / len);
    }

    return result;
}

Vec4F V4F_Normalise(Vec4F a) {
    Vec4F result = V4F(0, 0, 0, 0);

    F32 len = V4F_Length(a);
    if (len != 0.0) {
        result = V4F_Scale(a, 1.0f / len);
    }

    return result;
}

Vec2F V2F_Min(Vec2F a, Vec2F b) {
    Vec2F result;
    result.x = Min(a.x, b.x);
    result.y = Min(a.y, b.y);

    return result;
}

Vec3F V3F_Min(Vec3F a, Vec3F b) {
    Vec3F result;
    result.x = Min(a.x, b.x);
    result.y = Min(a.y, b.y);
    result.z = Min(a.z, b.z);

    return result;
}

Vec4F V4F_Min(Vec4F a, Vec4F b) {
    Vec4F result;
    result.x = Min(a.x, b.x);
    result.y = Min(a.y, b.y);
    result.z = Min(a.z, b.z);
    result.w = Min(a.w, b.w);

    return result;
}

Vec2F V2F_Max(Vec2F a, Vec2F b) {
    Vec2F result;
    result.x = Max(a.x, b.x);
    result.y = Max(a.y, b.y);

    return result;
}

Vec3F V3F_Max(Vec3F a, Vec3F b) {
    Vec3F result;
    result.x = Max(a.x, b.x);
    result.y = Max(a.y, b.y);
    result.z = Max(a.z, b.z);

    return result;
}

Vec4F V4F_Max(Vec4F a, Vec4F b) {
    Vec4F result;
    result.x = Max(a.x, b.x);
    result.y = Max(a.y, b.y);
    result.z = Max(a.z, b.z);
    result.w = Max(a.w, b.w);

    return result;
}

Vec2F V2F_Perp(Vec2F a) {
    Vec2F result;
    result.x = -a.y;
    result.y =  a.x;

    return result;
}

Vec2F V2F_Rotate(Vec2F arm, Vec2F v) {
    Vec2F result;
    result.x = (arm.x * v.x) - (arm.y * v.y);
    result.y = (arm.y * v.x) + (arm.x * v.y);

    return result;
}

F32 V2F_Cross(Vec2F a, Vec2F b) {
    F32 result = (a.x * b.y) - (a.y * b.x);
    return result;
}

Vec3F V3F_Cross(Vec3F a, Vec3F b) {
    Vec3F result;
    result.x = (a.y * b.z) - (a.z * b.y);
    result.y = (a.z * b.x) - (a.x * b.z);
    result.z = (a.x * b.y) - (a.y * b.x);

    return result;
}

//
// --------------------------------------------------------------------------------
// :Operators
// --------------------------------------------------------------------------------
//

Vec2U V2U_Add(Vec2U a, Vec2U b) {
    Vec2U result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);

    return result;
}

Vec2S V2S_Add(Vec2S a, Vec2S b) {
    Vec2S result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);

    return result;
}

Vec2F V2F_Add(Vec2F a, Vec2F b) {
    Vec2F result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);

    return result;
}

Vec3F V3F_Add(Vec3F a, Vec3F b) {
    Vec3F result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);
    result.z = (a.z + b.z);

    return result;
}

Vec4F V4F_Add(Vec4F a, Vec4F b) {
    Vec4F result;
    result.x = (a.x + b.x);
    result.y = (a.y + b.y);
    result.z = (a.z + b.z);
    result.w = (a.w + b.w);

    return result;
}

Vec2S V2S_Neg(Vec2S a) {
    Vec2S result;
    result.x = -a.x;
    result.y = -a.y;

    return result;
}

Vec2F V2F_Neg(Vec2F a) {
    Vec2F result;
    result.x = -a.x;
    result.y = -a.y;

    return result;
}

Vec3F V3F_Neg(Vec3F a) {
    Vec3F result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;

    return result;
}

Vec4F V4F_Neg(Vec4F a) {
    Vec4F result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;

    return result;
}

Vec2U V2U_Sub(Vec2U a, Vec2U b) {
    Vec2U result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);

    return result;
}

Vec2S V2S_Sub(Vec2S a, Vec2S b) {
    Vec2S result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);

    return result;
}

Vec2F V2F_Sub(Vec2F a, Vec2F b) {
    Vec2F result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);

    return result;
}

Vec3F V3F_Sub(Vec3F a, Vec3F b) {
    Vec3F result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);
    result.z = (a.z - b.z);

    return result;
}

Vec4F V4F_Sub(Vec4F a, Vec4F b) {
    Vec4F result;
    result.x = (a.x - b.x);
    result.y = (a.y - b.y);
    result.z = (a.z - b.z);

    return result;
}

Vec2F V2F_Hadamard(Vec2F a, Vec2F b) {
    Vec2F result;
    result.x = (a.x * b.x);
    result.y = (a.y * b.y);

    return result;
}

Vec3F V3F_Hadamard(Vec3F a, Vec3F b) {
    Vec3F result;
    result.x = (a.x * b.x);
    result.y = (a.y * b.y);
    result.z = (a.z * b.z);

    return result;
}

Vec4F V4F_Hadamard(Vec4F a, Vec4F b) {
    Vec4F result;
    result.x = (a.x * b.x);
    result.y = (a.y * b.y);
    result.z = (a.z * b.z);
    result.w = (a.w * b.w);

    return result;
}

Mat4x4F M4x4F_Mul(Mat4x4F a, Mat4x4F b) {
    Mat4x4F result;

    // @todo: simd
    //

    for (U32 r = 0; r < 4; ++r) {
        for (U32 c = 0; c < 4; ++c) {
            result.m[r][c] =
                (a.m[r][0] * b.m[0][c]) + (a.m[r][1] * b.m[1][c]) +
                (a.m[r][2] * b.m[2][c]) + (a.m[r][3] * b.m[3][c]);
        }
    }

    return result;
}

Vec3F M4x4F_MulV3F(Mat4x4F a, Vec3F b) {
    Vec3F result = M4x4F_MulV4F(a, V4F_FromV3F(b, 1.0f)).xyz;
    return result;
}

Vec4F M4x4F_MulV4F(Mat4x4F a, Vec4F b) {
    Vec4F result;
    result.x = V4F_Dot(a.r[0], b);
    result.y = V4F_Dot(a.r[1], b);
    result.z = V4F_Dot(a.r[2], b);
    result.w = V4F_Dot(a.r[3], b);

    return result;
}

Vec2F V2F_Scale(Vec2F a, F32 b) {
    Vec2F result;
    result.x = (a.x * b);
    result.y = (a.y * b);

    return result;
}

Vec3F V3F_Scale(Vec3F a, F32 b) {
    Vec3F result;
    result.x = (a.x * b);
    result.y = (a.y * b);
    result.z = (a.z * b);

    return result;
}

Vec4F V4F_Scale(Vec4F a, F32 b) {
    Vec4F result;
    result.x = (a.x * b);
    result.y = (a.y * b);
    result.z = (a.z * b);
    result.w = (a.w * b);

    return result;
}

Vec2F V2F_Div(Vec2F a, Vec2F b) {
    Vec2F result;
    result.x = (a.x / b.x);
    result.y = (a.y / b.y);

    return result;
}

Vec3F V3F_Div(Vec3F a, Vec3F b) {
    Vec3F result;
    result.x = (a.x / b.x);
    result.y = (a.y / b.y);
    result.z = (a.z / b.z);

    return result;
}

Vec4F V4F_Div(Vec4F a, Vec4F b) {
    Vec4F result;
    result.x = (a.x / b.x);
    result.y = (a.y / b.y);
    result.z = (a.z / b.z);
    result.w = (a.w / b.w);

    return result;
}

// :simd_maths
//
#if ARCH_AMD64

F32 F32_Sqrt(F32 a) {
    F32 result = _mm_cvtss_f32(_mm_sqrt_ps(_mm_set1_ps(a)));
    return result;
}

F32 F32_InvSqrtApprox(F32 a) {
    F32 result = _mm_cvtss_f32(_mm_rsqrt_ps(_mm_set1_ps(a)));
    return result;
}

#elif ARCH_AARCH64
    #error "incomplete implementation"
#endif

// for linux we bootstrap load the EngineRun function directly from the .so file
// rather than linking with it becasue linking with shared objects on linux comes
// with a whole host of problems
//
#if XI_OS_LINUX

#include <dlfcn.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>

#include <stdio.h>

StaticAssert(!"ERROR NOT COMPLETE!");

int EngineRun(GameCode *code) {
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

// :Compiler_Intrinsics
//
#if COMPILER_CL

U32 U32AtomicIncrement(volatile U32 *dst) {
    U32 result = _InterlockedIncrement((volatile long *) dst);
    return result;
}

U32 U32AtomicDecrement(volatile U32 *dst) {
    U32 result = _InterlockedDecrement((volatile long *) dst);
    return result;
}

U32 U32AtomicExchange(volatile U32 *dst, U32 value) {
    U32 result = _InterlockedExchange((volatile long *) dst, value);
    return result;
}

B32 U32AtomicCompareExchange(volatile U32 *dst, U32 value, U32 compare) {
    B32 result = _InterlockedCompareExchange((volatile long *) dst, value, compare) == (long) compare;
    return result;
}

U64 U64AtomicAdd(volatile U64 *dst, U64 amount) {
    U64 result = _InterlockedExchangeAdd64((volatile __int64 *) dst, (__int64) amount);
    return result;
}

U64 U64AtomicExchange(volatile U64 *dst, U64 value) {
    U64 result = _InterlockedExchange64((volatile __int64 *) dst, (__int64) value);
    return result;
}

B32 U64AtomicCompareExchange(volatile U64 *dst, U64 value, U64 compare) {
    B32 result = _InterlockedCompareExchange64((__int64 volatile *) dst, value, compare) == (__int64) compare;
    return result;
}

#elif (COMPILER_CLANG || COMPILER_GCC)

U32 U32AtomicIncrement(volatile U32 *dst) {
    U32 result = __atomic_add_fetch(dst, 1, __ATOMIC_SEQ_CST);
    return result;
}

U32 U32AtomicDecrement(volatile U32 *dst) {
    U32 result = __atomic_sub_fetch(dst, 1, __ATOMIC_SEQ_CST);
    return result;
}

U32 U32AtomicExchange(volatile U32 *dst, U32 value) {
    U32 result = __atomic_exchange_n(dst, value, __ATOMIC_SEQ_CST);
    return result;
}

B32 U32AtomicCompareExchange(volatile U32 *dst, U32 value, U32 compare) {
    B32 result = __atomic_compare_exchange_n(dst, &compare, value, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return result;
}

U64 U64AtomicAdd(volatile U64 *dst, U64 amount) {
    U64 result = __atomic_fetch_add(dst, amount, __ATOMIC_SEQ_CST);
    return result;
}

U64 U64AtomicExchange(volatile U64 *dst, U64 value) {
    U64 result = __atomic_exchange_n(dst, value, __ATOMIC_SEQ_CST);
    return result;
}

B32 U64AtomicCompareExchange(volatile U64 *dst, U64 value, U64 compare) {
    B32 result = __atomic_compare_exchange_n(dst, &compare, value, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return result;
}

#endif
