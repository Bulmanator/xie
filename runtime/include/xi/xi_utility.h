#if !defined(XI_UTILITY_H_)
#define XI_UTILITY_H_

#if defined(__cplusplus)
extern "C" {
#endif

extern XI_API void xi_memory_zero(void *base, xi_uptr size);
extern XI_API void xi_memory_set(void *base, xi_u8 value, xi_uptr size);
extern XI_API void xi_memory_copy(void *dst, void *src, xi_uptr size);

extern XI_API xi_u8 xi_char_to_lowercase(xi_u8 c);
extern XI_API xi_u8 xi_char_to_uppercase(xi_u8 c);

extern XI_API xi_u32 xi_djb2_str_hash(xi_string str);

// returns the actual number of bytes copied, equal to count if sufficient space is available in the buffer,
// otherwise will be the size of the remainding space before copy
//
extern XI_API xi_uptr xi_buffer_append(xi_buffer *buffer, void *base, xi_uptr count);

// return true if they successfully put them into the buffer, if false nothing is copied
//
extern XI_API xi_b32 xi_buffer_put_u8(xi_buffer *buffer,  xi_u8  value);
extern XI_API xi_b32 xi_buffer_put_u16(xi_buffer *buffer, xi_u16 value);
extern XI_API xi_b32 xi_buffer_put_u32(xi_buffer *buffer, xi_u32 value);
extern XI_API xi_b32 xi_buffer_put_u64(xi_buffer *buffer, xi_u64 value);

#if defined(__cplusplus)
}
#endif

#endif  // XI_UTILITY_H_
