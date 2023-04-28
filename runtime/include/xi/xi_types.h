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
// :note operating system detection
//
#define XI_OS_WIN32 0
#define XI_OS_LINUX 0

#if defined(_WIN32)
    #undef  XI_OS_WIN32
    #define XI_OS_WIN32 1

    #pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#elif defined(__linux__)
    #undef  XI_OS_LINUX
    #define XI_OS_LINUX 1
#endif

//
// :note intrinsic includes
//
#if XI_COMPILER_CL
    // architecture specific includes are dealt with internally
    //
    #include <intrin.h>
#elif (XI_COMPILER_CLANG || XI_COMPILER_GCC)
    #if XI_ARCH_AMD64
        #include <x86intrin.h>
    #elif XI_ARCH_AARCH64
        #include <arm_neon.h>
    #endif
#endif

//
// :note core types
//

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

typedef uint8_t  xi_u8;
typedef uint16_t xi_u16;
typedef uint32_t xi_u32;
typedef uint64_t xi_u64;

typedef int8_t  xi_s8;
typedef int16_t xi_s16;
typedef int32_t xi_s32;
typedef int64_t xi_s64;

typedef int8_t  xi_b8;
typedef int16_t xi_b16;
typedef int32_t xi_b32;
typedef int64_t xi_b64;

typedef uintptr_t xi_uptr;
typedef intptr_t  xi_sptr;

typedef float  xi_f32;
typedef double xi_f64;

typedef struct xi_string {
    xi_uptr count; // in bytes
    xi_u8  *data;
} xi_string;

typedef struct xi_buffer {
    union {
        struct {
            xi_uptr used;
            xi_u8  *data;
        };

        xi_string str;
    };

    xi_uptr limit;
} xi_buffer;

//
// :note math types
//

typedef union xi_v2u {
    struct {
        xi_u32 x, y;
    };

    struct {
        xi_u32 w, h;
    };

    xi_u32 e[2];
} xi_v2u;


typedef union xi_v2s {
    struct {
        xi_s32 x, y;
    };

    struct {
        xi_s32 w, h;
    };

    xi_s32 e[2];
} xi_v2s;

typedef union xi_v2 {
    struct {
        xi_f32 x, y;
    };

    struct {
        xi_f32 u, v;
    };

    struct {
        xi_f32 w, h;
    };

    xi_f32 e[2];
} xi_v2;

typedef union xi_v3 {
    struct {
        xi_f32 x, y, z;
    };

    struct {
        xi_f32 r, g, b;
    };

    struct {
        xi_f32 w, h, d;
    };

    struct {
        xi_v2  xy;
        xi_f32 _z;
    };

    xi_f32 e[3];
} xi_v3;

typedef union xi_v4 {
    struct {
        xi_f32 x, y, z;
        xi_f32 w;
    };

    struct {
        xi_f32 r, g, b;
        xi_f32 a;
    };

    struct {
        xi_v3  xyz;
        xi_f32 _w;
    };

    struct {
        xi_v3  rgb;
        xi_f32 _a;
    };

    xi_f32 e[4];
} xi_v4;

typedef struct xi_rect2 {
    xi_v2 min;
    xi_v2 max;
} xi_rect2;

typedef struct xi_rect3 {
    xi_v3 min;
    xi_v3 max;
} xi_rect3;

// :note all matrices are assumed to be in row-major order
//

typedef union xi_m2x2 {
    xi_f32 m[2][2];
    xi_f32 e[4];
    xi_v2  r[2];
} xi_m2x2;

typedef union xi_m4x4 {
    xi_f32 m[4][4];
    xi_f32 e[16];
    xi_v4  r[4];
} xi_m4x4;

typedef struct xi_m4x4_inv {
    xi_m4x4 fwd;
    xi_m4x4 inv;
} xi_m4x4_inv;

// :note vertex type
//
typedef struct xi_vert3 {
    xi_v3 p;
    xi_v3 uv; // @todo: does this need to be full precision?
    xi_u32 c;
} xi_vert3; // 24 bytes

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
    #define XI_THREAD_VAR __declspec(thread)
#elif (XI_COMPILER_CLANG || XI_COMPILER_GCC)
    #define XI_THREAD_VAR __thread
#endif

#define XI_INTERNAL static
#define XI_GLOBAL   static

#define XI_STRINGIFY(x) #x

#define XI_FOURCC(a, b, c, d) \
    (((xi_u32) (d) << 24) | ((xi_u32) (c) << 16) | ((xi_u32) (b) << 8) | ((xi_u32) (a) << 0))

#define XI_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define XI_OFFSET_OF(t, m) ((xi_uptr) &(((t *) 0)->m))

#define XI_ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define XI_ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

#define XI_ALIGN4(x) (((x) + 3) & ~3)
#define XI_ALIGN8(x) (((x) + 7) & ~7)
#define XI_ALIGN16(x) (((x) + 15) & ~15)

#define XI_KB(x) (((xi_uptr) (x)) << 10ULL)
#define XI_MB(x) (((xi_uptr) (x)) << 20ULL)
#define XI_GB(x) (((xi_uptr) (x)) << 30ULL)
#define XI_TB(x) (((xi_uptr) (x)) << 40ULL)

#define XI_MIN(a, b) ((a) < (b) ? (a) : (b))
#define XI_MAX(a, b) ((a) > (b) ? (a) : (b))

#define XI_CLAMP(a, min, max) (XI_MIN(XI_MAX(a, min), max))

#if !defined(XI_NO_ASSERT)
    #include <assert.h>
    #define XI_ASSERT(exp) assert(exp)
    #define XI_STATIC_ASSERT(exp) static_assert(exp, #exp)
#else
    #define XI_ASSERT(exp) (void) (exp)
    #define XI_STATIC_ASSERT(exp)
#endif

//
// :note type limits
//

#define XI_U8_MAX  ((xi_u8)  0xFF)
#define XI_U16_MAX ((xi_u16) 0xFFFF)
#define XI_U32_MAX ((xi_u32) 0xFFFFFFFF)
#define XI_U64_MAX ((xi_u64) 0xFFFFFFFFFFFFFFFF)

#define XI_S8_MAX  ((xi_s8)  0x7F)
#define XI_S16_MAX ((xi_s16) 0x7FFF)
#define XI_S32_MAX ((xi_s32) 0x7FFFFFFF)
#define XI_S64_MAX ((xi_s64) 0x7FFFFFFFFFFFFFFF)

#define XI_UPTR_MAX ((xi_uptr) UINTPTR_MAX)
#define XI_SPTR_MAX ((xi_sptr) INTPTR_MAX)

// @todo: remove float.h dep
//
#include <float.h>

#define XI_F32_MAX ((xi_f32) FLT_MAX)
#define XI_F64_MAX ((xi_f64) DBL_MAX)

#define XI_S8_MIN  ((xi_s8)  0x80)
#define XI_S16_MIN ((xi_s16) 0x8000)
#define XI_S32_MIN ((xi_s32) 0x80000000)
#define XI_S64_MIN ((xi_s64) 0x8000000000000000)

#define XI_SPTR_MIN ((xi_sptr) INTPTR_MIN)

#define XI_F32_MIN ((xi_f32) FLT_MIN)
#define XI_F64_MIN ((xi_f64) DBL_MIN)

#if defined(__cplusplus)
}
#endif

#endif  // XI_TYPES_H_
