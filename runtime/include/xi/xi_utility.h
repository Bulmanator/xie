#if !defined(XI_UTILITY_H_)
#define XI_UTILITY_H_

#if defined(__cplusplus)
extern "C" {
#endif

extern XI_API void xi_memory_zero(void *base, uptr size);
extern XI_API void xi_memory_set(void *base, u8 value, uptr size);
extern XI_API void xi_memory_copy(void *dst, void *src, uptr size);

#if defined(__cplusplus)
}
#endif

#endif  // XI_UTILITY_H_
