#if !defined(XI_UTILITY_H_)
#define XI_UTILITY_H_

#if defined(__cplusplus)
extern "C" {
#endif

extern XI_API void xi_memory_zero(void *base, uptr size);
extern XI_API void xi_memory_set(void *base, u8 value, uptr size);
extern XI_API void xi_memory_copy(void *dst, void *src, uptr size);

extern XI_API u8 xi_char_to_lowercase(u8 c);
extern XI_API u8 xi_char_to_uppercase(u8 c);

// returns the actual number of bytes copied, equal to count if sufficient space is available in the buffer,
// otherwise will be the size of the remainding space before copy
//
extern XI_API uptr xi_buffer_append(buffer *out, void *base, uptr count);
extern XI_API uptr xi_buffer_append_bytes(buffer *out, u8 byte, uptr count);

#if defined(__cplusplus)
}
#endif

#endif  // XI_UTILITY_H_
