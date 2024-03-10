#if !defined(XI_MATHS_H_)
#define XI_MATHS_H_

#if !defined(EPSILON_F32)
    #define EPSILON_F32 0.00001f
#endif

#define F32_PI  (3.141592653589793238462643383279502884197169f)
#define F32_TAU (2.0f * F32_PI)

//
// --------------------------------------------------------------------------------
// :Random_Number_Generator
// --------------------------------------------------------------------------------
//

typedef struct RandomState RandomState;
struct RandomState {
    U64 state;
};

Inline RandomState RandomSeed(U64 seed);

Inline U32 U32_RandomNext(RandomState *rng);
Inline U64 U64_RandomNext(RandomState *rng);

Inline F32 F32_RandomUnilateral(RandomState *rng);
Inline F64 F64_RandomUnilateral(RandomState *rng);

Inline F32 F32_RandomBilateral(RandomState *rng);
Inline F64 F64_RandomBilateral(RandomState *rng);

Inline U32 U32_RandomRange(RandomState *rng, U32 min, U32 max);
Inline S32 S32_RandomRange(RandomState *rng, S32 min, S32 max);
Inline U64 U64_RandomRange(RandomState *rng, U64 min, U64 max);
Inline S64 S64_RandomRange(RandomState *rng, S64 min, S64 max);
Inline F32 F32_RandomRange(RandomState *rng, F32 min, F32 max);
Inline F64 F64_RandomRange(RandomState *rng, F64 min, F64 max);

Inline U32 U32_RandomChoice(RandomState *rng, U32 count);
Inline U64 U64_RandomChoice(RandomState *rng, U64 count);

//
// --------------------------------------------------------------------------------
// :Basic
// --------------------------------------------------------------------------------
//

Inline F32 F32_Sqrt(F32 a);
Inline F32 F32_InvSqrt(F32 a);
Inline F32 F32_InvSqrtApprox(F32 a);

Inline F32 F32_Sin(F32 turns);
Inline F32 F32_Cos(F32 turns);
Inline F32 F32_Tan(F32 turns);

Inline F32 F32_Lerp(F32 a, F32 b, F32 t);
Inline F64 F64_Lerp(F64 a, F64 b, F64 t);

Inline Vec2F V2F_Lerp(Vec2F a, Vec2F b, F32 t);
Inline Vec3F V3F_Lerp(Vec3F a, Vec3F b, F32 t);
Inline Vec4F V4F_Lerp(Vec4F a, Vec4F b, F32 t);

Inline U32 U32_Pow2Next(U32 x);
Inline U32 U32_Pow2Prev(U32 x);
Inline U32 U32_Pow2Nearest(U32 x);

//
// --------------------------------------------------------------------------------
// :Construction
// --------------------------------------------------------------------------------
//

Inline Vec2U V2U(U32 x, U32 y);
Inline Vec2S V2S(S32 x, S32 y);

Inline Vec2F V2F(F32 x, F32 y);
Inline Vec3F V3F(F32 x, F32 y, F32 z);
Inline Vec4F V4F(F32 x, F32 y, F32 z, F32 w);

Inline Vec2F V2F_UnitArm(F32 turn);

Inline Mat4x4F M4x4F(F32 diagonal);

Inline Mat4x4F M4x4F_Rows(Vec3F x, Vec3F y, Vec3F z);
Inline Mat4x4F M4x4F_Columns(Vec3F x, Vec3F y, Vec3F z);

Inline Mat4x4F M4x4F_RotationX(F32 turns);
Inline Mat4x4F M4x4F_RotationY(F32 turns);
Inline Mat4x4F M4x4F_RotationZ(F32 turns);

Func Mat4x4FInv M4x4F_PerspectiveProjection(F32 fov, F32 aspect, F32 nearp, F32 farp);
Func Mat4x4FInv M4x4F_OrthographicProjection(F32 aspect, F32 nearp, F32 farp);
Func Mat4x4FInv M4x4F_CameraViewProjection(Vec3F x, Vec3F y, Vec3F z, Vec3F position);

//
// --------------------------------------------------------------------------------
// :Conversion
// --------------------------------------------------------------------------------
//

Inline Vec2F V2F_FromV2U(Vec2U a);
Inline Vec2F V2F_FromV2S(Vec2S a);

Inline Vec2S V2S_FromV2U(Vec2U a);
Inline Vec2S V2S_FromV2F(Vec2F a);

Inline Vec2U V2U_FromV2S(Vec2S a);
Inline Vec2U V2U_FromV2F(Vec2F a);

Inline Vec3F V3F_FromV2F(Vec2F xy,  F32 z);
Inline Vec4F V4F_FromV2F(Vec2F xy,  F32 z, F32 w);
Inline Vec4F V4F_FromV3F(Vec3F xyz, F32 w);

Inline Vec4F M4x4F_RowExtract(Mat4x4F a, U32 row);
Inline Vec4F M4x4F_ColumnExtract(Mat4x4F a, U32 column);

Inline U32 V4F_ColourPackABGR(Vec4F colour);

Inline Vec4F U32_ColourUnpackARGB(U32 colour);
Inline Vec4F U32_ColourUnpackABGR(U32 colour);

Func Vec4F V4F_Linear1FromSRGB255(Vec4F colour);
Func Vec4F V4F_SRGB255FromLinear1(Vec4F colour);

Func Vec4F V4F_Linear1FromSRGB1(Vec4F colour);
Func Vec4F V4F_SRGB1FromLinear1(Vec4F colour);

//
// --------------------------------------------------------------------------------
// :Vector_Basic
// --------------------------------------------------------------------------------
//

Inline F32 V2F_Dot(Vec2F a, Vec2F b);
Inline F32 V3F_Dot(Vec3F a, Vec3F b);
Inline F32 V4F_Dot(Vec4F a, Vec4F b);

Inline F32 V2F_LengthSq(Vec2F a);
Inline F32 V3F_LengthSq(Vec3F a);
Inline F32 V4F_LengthSq(Vec4F a);

Inline F32 V2F_Length(Vec2F a);
Inline F32 V3F_Length(Vec3F a);
Inline F32 V4F_Length(Vec4F a);

Inline Vec2F V2F_Normalise(Vec2F a);
Inline Vec3F V3F_Normalise(Vec3F a);
Inline Vec4F V4F_Normalise(Vec4F a);

Inline Vec2F V2F_Min(Vec2F a, Vec2F b);
Inline Vec3F V3F_Min(Vec3F a, Vec3F b);
Inline Vec4F V4F_Min(Vec4F a, Vec4F b);

Inline Vec2F V2F_Max(Vec2F a, Vec2F b);
Inline Vec3F V3F_Max(Vec3F a, Vec3F b);
Inline Vec4F V4F_Max(Vec4F a, Vec4F b);

Inline Vec2F V2F_Perp(Vec2F a);
Inline Vec2F V2F_Rotate(Vec2F arm, Vec2F v);
Inline F32   V2F_Cross(Vec2F a, Vec2F b);
Inline Vec3F V3F_Cross(Vec3F a, Vec3F b);

//
// --------------------------------------------------------------------------------
// :Operators
// --------------------------------------------------------------------------------
//

Inline Vec2U V2U_Add(Vec2U a, Vec2U b);
Inline Vec2S V2S_Add(Vec2S a, Vec2S b);
Inline Vec2F V2F_Add(Vec2F a, Vec2F b);
Inline Vec3F V3F_Add(Vec3F a, Vec3F b);
Inline Vec4F V4F_Add(Vec4F a, Vec4F b);

Inline Vec2S V2S_Neg(Vec2S a);
Inline Vec2F V2F_Neg(Vec2F a);
Inline Vec3F V3F_Neg(Vec3F a);
Inline Vec4F V4F_Neg(Vec4F a);

Inline Vec2U V2U_Sub(Vec2U a, Vec2U b);
Inline Vec2S V2S_Sub(Vec2S a, Vec2S b);
Inline Vec2F V2F_Sub(Vec2F a, Vec2F b);
Inline Vec3F V3F_Sub(Vec3F a, Vec3F b);
Inline Vec4F V4F_Sub(Vec4F a, Vec4F b);

Inline Vec2F V2F_Hadamard(Vec2F a, Vec2F b);
Inline Vec3F V3F_Hadamard(Vec3F a, Vec3F b);
Inline Vec4F V4F_Hadamard(Vec4F a, Vec4F b);

Inline Mat4x4F M4x4F_Mul(Mat4x4F a, Mat4x4F b);

Inline Vec3F M4x4F_MulV3F(Mat4x4F a, Vec3F b);
Inline Vec4F M4x4F_MulV4F(Mat4x4F a, Vec4F b);

Inline Vec2F V2F_Scale(Vec2F a, F32 b);
Inline Vec3F V3F_Scale(Vec3F a, F32 b);
Inline Vec4F V4F_Scale(Vec4F a, F32 b);

Inline Vec2F V2F_Div(Vec2F a, Vec2F b);
Inline Vec3F V3F_Div(Vec3F a, Vec3F b);
Inline Vec4F V4F_Div(Vec4F a, Vec4F b);

#endif  // XI_MATHS_H_
