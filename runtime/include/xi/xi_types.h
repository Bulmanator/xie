#if !defined(XI_TYPES_H_)
#define XI_TYPES_H_

#if defined(__cplusplus)
extern "C" {
#endif

//
// :note compiler detection
//

#define XI_COMPILER_CLANG 0
#define XI_COMPILER_CL    0
#define XI_COMPILER_GCC   0

#if defined(__clang__)
    #undef  XI_COMPILER_CLANG
    #define XI_COMPILER_CLANG 1
#elif defined(_MSC_VER)
    #undef  XI_COMPILER_CL
    #define XI_COMPILER_CL 1
#elif defined(__GNUC__)
    #undef  XI_COMPILER_GCC
    #define XI_COMPILER_GCC 1
#else
    #error "unsupported compiler."
#endif

//
// :note architecture detection
//

#define XI_ARCH_AMD64   0
#define XI_ARCH_AARCH64 0

#if defined(__amd64__) || defined(_M_AMD64)
    #undef  XI_ARCH_AMD64
    #define XI_ARCH_AMD64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #undef  XI_ARCH_AARCH64
    #define XI_ARCH_AARCH64 1
#else
    #error "unsupported architecture."
#endif

//
// :note core types
//

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef int8_t  b8;
typedef int16_t b16;
typedef int32_t b32;
typedef int64_t b64;

typedef uintptr_t uptr;
typedef intptr_t  sptr;

typedef float  f32;
typedef double f64;

typedef struct string {
    uptr count; // in bytes
    u8  *data;
} string;

typedef struct buffer {
    union {
        struct {
            uptr used;
            u8  *data;
        };

        string str;
    };

    uptr limit;
} buffer;

//
// :note math types
//

typedef union v2u {
    struct {
        u32 x, y;
    };

    struct {
        u32 w, h;
    };

    u32 e[2];
} v2u;


typedef union v2s {
    struct {
        s32 x, y;
    };

    struct {
        s32 w, h;
    };

    s32 e[2];
} v2s;

typedef union v2 {
    struct {
        f32 x, y;
    };

    struct {
        f32 u, v;
    };

    struct {
        f32 w, h;
    };

    f32 e[2];
} v2;

typedef union v3 {
    struct {
        f32 x, y, z;
    };

    struct {
        f32 r, g, b;
    };

    struct {
        f32 w, h, d;
    };

    struct {
        v2  xy;
        f32 _z;
    };

    f32 e[3];
} v3;

typedef union v4 {
    struct {
        f32 x, y, z;
        f32 w;
    };

    struct {
        f32 r, g, b;
        f32 a;
    };

    struct {
        v3  xyz;
        f32 _w;
    };

    f32 e[4];
} v4;

typedef struct rect2 {
    v2 min;
    v2 max;
} rect2;

typedef struct rect3 {
    v3 min;
    v3 max;
} rect3;

// :note all matrices are assumed to be in row-major order
//

typedef union m2x2 {
    f32 m[2][2];
    f32 e[4];
    v2  r[2];
} m2x2;

typedef union m4x4 {
    f32 m[4][4];
    f32 e[16];
    v4  r[4];
} m4x4;

typedef struct m4x4_inv {
    m4x4 fwd;
    m4x4 inv;
} m4x4_inv;

//
// :note utility macros
//
#if XI_COMPILER_CL
    #if defined(XI_RUNTIME_BUILD)
        #define XI_API __declspec(dllexport)
    #else
        #define XI_API __declspec(dllimport)
    #endif
#elif (XI_COMPILER_CLANG || XI_COMPILER_GCC)
    #define XI_API
#endif

#if XI_COMPILER_CL
    #define XI_EXPORT __declspec(dllexport)
#elif (XI_COMPILER_CLANG || XI_COMPILER_GCC)
    #define XI_EXPORT
#endif

#if XI_COMPILER_CL
    #define thread_var __declspec(thread)
#elif (XI_COMPILER_CLANG || XI_COMPILER_GCC)
    #define thread_var __thread
#endif

#define XI_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define XI_OFFSET_OF(t, m) ((uptr) &(((t *) 0)->m))

#define XI_ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define XI_ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

#define XI_ALIGN4(x) (((x) + 3) & ~3)
#define XI_ALIGN8(x) (((x) + 7) & ~7)
#define XI_ALIGN16(x) (((x) + 15) & ~15)

#define XI_KB(x) (((uptr) (x)) << 10ULL)
#define XI_MB(x) (((uptr) (x)) << 20ULL)
#define XI_GB(x) (((uptr) (x)) << 30ULL)
#define XI_TB(x) (((uptr) (x)) << 40ULL)

#define XI_MIN(a, b) ((a) < (b) ? (a) : (b))
#define XI_MAX(a, b) ((a) > (b) ? (a) : (b))

#if !defined(XI_NO_ASSERT)
    #include <assert.h>
    #define XI_ASSERT(exp) assert(exp)
#else
    #define XI_ASSERT(exp) (void) exp
#endif

//
// :note type limits
//

#define XI_U8_MAX  ((u8)  0xFF)
#define XI_U16_MAX ((u16) 0xFFFF)
#define XI_U32_MAX ((u32) 0xFFFFFFFF)
#define XI_U64_MAX ((u64) 0xFFFFFFFFFFFFFFFF)

#define XI_S8_MAX  ((s8)  0x7F)
#define XI_S16_MAX ((s16) 0x7FFF)
#define XI_S32_MAX ((s32) 0x7FFFFFFF)
#define XI_S64_MAX ((s64) 0x7FFFFFFFFFFFFFFF)

#define XI_UPTR_MAX ((uptr) UINTPTR_MAX)
#define XI_SPTR_MAX ((sptr) INTPTR_MAX)

// @todo: remove float.h dep
//
#include <float.h>

#define XI_F32_MAX ((f32) FLT_MAX)
#define XI_F64_MAX ((f64) DBL_MAX)

#define XI_S8_MIN  ((s8)  0x80)
#define XI_S16_MIN ((s16) 0x8000)
#define XI_S32_MIN ((s32) 0x80000000)
#define XI_S64_MIN ((s64) 0x8000000000000000)

#define XI_SPTR_MIN ((sptr) INTPTR_MIN)

#define XI_F32_MIN ((f32) FLT_MIN)
#define XI_F64_MIN ((f64) DBL_MIN)

#if defined(__cplusplus)
}
#endif

#endif  // XI_TYPES_H_
