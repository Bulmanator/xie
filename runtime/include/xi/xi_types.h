#if !defined(XI_TYPES_H_)
#define XI_TYPES_H_

#if defined(__cplusplus)
extern "C" {
#endif

//
// --------------------------------------------------------------------------------
// :Platform_Macros
// --------------------------------------------------------------------------------
//

#define COMPILER_CLANG 0
#define COMPILER_CL    0
#define COMPILER_GCC   0

#if defined(__clang__)
    #undef  COMPILER_CLANG
    #define COMPILER_CLANG 1
#elif defined(_MSC_VER)
    #undef  COMPILER_CL
    #define COMPILER_CL 1
#elif defined(__GNUC__)
    #undef  COMPILER_GCC
    #define COMPILER_GCC 1
#else
    #error "unsupported compiler."
#endif

#define ARCH_AMD64   0
#define ARCH_AARCH64 0

#if defined(__amd64__) || defined(_M_AMD64)
    #undef  ARCH_AMD64
    #define ARCH_AMD64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #undef  ARCH_AARCH64
    #define ARCH_AARCH64 1
#else
    #error "unsupported architecture."
#endif

#define OS_WIN32 0
#define OS_LINUX 0

#if defined(_WIN32)
    #undef  OS_WIN32
    #define OS_WIN32 1

    #pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#elif defined(__linux__)
    #undef  OS_LINUX
    #define OS_LINUX 1
#endif

#if COMPILER_CL
    // architecture specific includes are dealt with internally
    //
    #include <intrin.h>
#elif (COMPILER_CLANG || COMPILER_GCC)
    #if ARCH_AMD64
        #include <x86intrin.h>
    #elif ARCH_AARCH64
        #include <arm_neon.h>
    #endif
#endif

//
// --------------------------------------------------------------------------------
// :Core_Types
// --------------------------------------------------------------------------------
//

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t  S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef int8_t  B8;
typedef int16_t B16;
typedef int32_t B32;
typedef int64_t B64;

typedef float  F32;
typedef double F64;

typedef struct Str8 Str8;
struct Str8 {
    S64 count; // in bytes
    U8  *data;
};

typedef struct Buffer Buffer;
struct Buffer {
    union {
        struct {
            S64 used;
            U8  *data;
        };

        Str8 str;
    };

    S64 limit;
};

typedef union Vec2U Vec2U;
union Vec2U {
    struct {
        U32 x, y;
    };

    struct {
        U32 w, h;
    };

    U32 e[2];
} ;


typedef union Vec2S Vec2S;
union Vec2S {
    struct {
        S32 x, y;
    };

    struct {
        S32 w, h;
    };

    S32 e[2];
};

typedef union Vec2F Vec2F;
union Vec2F {
    struct {
        F32 x, y;
    };

    struct {
        F32 u, v;
    };

    struct {
        F32 w, h;
    };

    F32 e[2];
};

typedef union Vec3F Vec3F;
union Vec3F {
    struct {
        F32 x, y, z;
    };

    struct {
        F32 r, g, b;
    };

    struct {
        F32 w, h, d;
    };

    struct {
        Vec2F xy;
        F32   _z;
    };

    F32 e[3];
};

typedef union Vec4F Vec4F;
union Vec4F {
    struct {
        F32 x, y, z;
        F32 w;
    };

    struct {
        F32 r, g, b;
        F32 a;
    };

    struct {
        Vec3F xyz;
        F32   _w;
    };

    struct {
        Vec3F rgb;
        F32   _a;
    };

    struct {
        Vec2F xy;
        Vec2F zw;
    };

    F32 e[4];
};

typedef struct Rect2F Rect2F;
struct Rect2F {
    Vec2F min;
    Vec2F max;
};

typedef struct Rect3F Rect3F;
struct Rect3F {
    Vec3F min;
    Vec3F max;
};

// :note all matrices are assumed to be in row-major order
//

typedef union Mat4x4F Mat4x4F;
union Mat4x4F {
    F32   m[4][4];
    F32   e[16];
    Vec4F r[4];
};

typedef struct Mat4x4FInv Mat4x4FInv;
struct Mat4x4FInv {
    Mat4x4F fwd;
    Mat4x4F inv;
};

typedef struct Vertex3 Vertex3;
struct Vertex3 {
    Vec3F p;
    Vec3F uv; // @todo: does this need to be full precision?
    U32   c;
};

//
// --------------------------------------------------------------------------------
// :Utility_Macros
// --------------------------------------------------------------------------------
//

#define C_LINKAGE

#if defined(__cplusplus)
    #undef  C_LINKAGE
    #define C_LINKAGE "C"
#endif

#if COMPILER_CL
    #if defined(XI_RUNTIME_BUILD)
        #define Func extern C_LINKAGE __declspec(dllexport)
    #else
        #define Func extern C_LINKAGE __declspec(dllimport)
    #endif
#elif (COMPILER_CLANG || COMPILER_GCC)
    #define Func extern C_LINKAGE
#endif

#if COMPILER_CL
    #define ExportFunc extern C_LINKAGE __declspec(dllexport)
#else
    #define ExportFunc extern C_LINKAGE
#endif

#if COMPILER_CL
    #define ThreadVar __declspec(thread)
#elif (COMPILER_CLANG || COMPILER_GCC)
    #define ThreadVar __thread
#endif

#define Inline inline

#define FileScope    static
#define GlobalVar    static
#define LocalPersist static

#define InternalGlue(a, b)   a##b
#define InternalStringify(x) #x

#define Glue(a, b)   InternalGlue(a, b)
#define Stringify(x) InternalStringify(x)

#define FourCC(a, b, c, d) (((U32) (d) << 24) | ((U32) (c) << 16) | ((U32) (b) << 8) | ((U32) (a) << 0))

#define cast(x) (x)

#define ArraySize(x)   (sizeof(x) / sizeof((x)[0]))
#define OffsetTo(T, m) ((U64) &(((T *) 0)->m))

#if defined(__cplusplus)
    #define AlignOf(T) alignof(T)
#else
    #define AlignOf(T) _Alignof(T)
#endif

#define AlignUp(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))
#define AlignDown(x, a) ((x) & ~((a) - 1))

#define AlignUp4(x)  (((x) + 3)  & ~3)
#define AlignUp8(x)  (((x) + 7)  & ~7)
#define AlignUp16(x) (((x) + 15) & ~15)

#define KB(x) (((U64) (x)) << 10ULL)
#define MB(x) (((U64) (x)) << 20ULL)
#define GB(x) (((U64) (x)) << 30ULL)
#define TB(x) (((U64) (x)) << 40ULL)

#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))

#define Clamp(min, a, max) (Min(Max(a, min), max))
#define Clamp01(a) Clamp(0, a, 1)

#define StaticAssert(exp) static_assert(exp, #exp)

#if !defined(XI_NO_ASSERT)
    #include <assert.h>
    #define Assert(exp) assert(exp)
#else
    #define Assert(exp) (void) (exp)
#endif

//
// --------------------------------------------------------------------------------
// :Type_Limits
// --------------------------------------------------------------------------------
//

#define U8_MAX  ((U8)  0xFF)
#define U16_MAX ((U16) 0xFFFF)
#define U32_MAX ((U32) 0xFFFFFFFF)
#define U64_MAX ((U64) 0xFFFFFFFFFFFFFFFF)

#define S8_MIN  ((S8)  0x80)
#define S16_MIN ((S16) 0x8000)
#define S32_MIN ((S32) 0x80000000)
#define S64_MIN ((S64) 0x8000000000000000)

#define S8_MAX  ((S8)  0x7F)
#define S16_MAX ((S16) 0x7FFF)
#define S32_MAX ((S32) 0x7FFFFFFF)
#define S64_MAX ((S64) 0x7FFFFFFFFFFFFFFF)

#define F32_MIN ((F32) 1.17549435082228750796873653722224568e-038F)
#define F64_MIN ((F64) 2.22507385850720138309023271733240406e-308L)

#define F32_MAX ((F32) 3.40282346638528859811704183484516925e+038F)
#define F64_MAX ((F64) 1.79769313486231570814527423731704357e+308L)

#if defined(__cplusplus)
}
#endif

#endif  // XI_TYPES_H_
