#if !defined(XI_STRING_H_)
#define XI_STRING_H_

// string utility functions, strings are expected to be encoded in utf-8.
// _all_ counts are expected to be in number of bytes, not characters
//

#define xi_str_wrap_const(x) xi_str_wrap_count((xi_u8 *) (x), sizeof(x) - sizeof(*(x)))

// this will unpack a 'string' into the format for %*.s in a format string
//
#define xi_str_unpack(x) (int) (x).count, (x).data

extern XI_API xi_string xi_str_wrap_count(xi_u8 *data, xi_uptr count);
extern XI_API xi_string xi_str_wrap_range(xi_u8 *start, xi_u8 *end);
extern XI_API xi_string xi_str_wrap_cstr(xi_u8 *data); // null-terminated

// has a count and a data pointer
//
extern XI_API xi_b32 xi_str_is_valid(xi_string str);

extern XI_API xi_string xi_str_copy(xiArena *arena, xi_string str);

enum xiStringCompareFlags {
    XI_STRING_COMPARE_IGNORE_CASE = (1 << 0),
    XI_STRING_COMPARE_INEXACT_RHS = (1 << 1)
};

extern XI_API xi_b32 xi_str_equal_flags(xi_string a, xi_string b, xi_u32 flags);
extern XI_API xi_b32 xi_str_equal(xi_string a, xi_string b);

// format strings
//
extern XI_API xi_string xi_str_format_args(xiArena *arena, const char *format, va_list args);
extern XI_API xi_string xi_str_format_buffer_args(xi_buffer *b, const char *format, va_list args);

extern XI_API xi_string xi_str_format(xiArena *arena, const char *format, ...);
extern XI_API xi_string xi_str_format_buffer(xi_buffer *b, const char *format, ...);

// substring functions
//
extern XI_API xi_string xi_str_prefix(xi_string str, xi_uptr count);
extern XI_API xi_string xi_str_suffix(xi_string str, xi_uptr count);

extern XI_API xi_string xi_str_advance(xi_string str, xi_uptr count);
extern XI_API xi_string xi_str_remove(xi_string str, xi_uptr count);

extern XI_API xi_string xi_str_slice(xi_string str, xi_uptr start, xi_uptr end);

// up to the first occurrence of 'codepoint'
//
// 'offset' will be filled out with the byte offset into the xi_string where codepoint was found and will
// return true, if the codepoint is not found the function will return false and the contents of 'offset'
// will remain unmodified
//
extern XI_API xi_b32 xi_str_find_first(xi_string str, xi_uptr *offset, xi_u32 codepoint);
extern XI_API xi_b32 xi_str_find_last(xi_string str,  xi_uptr *offset, xi_u32 codepoint);

#endif  // XI_STRING_H_
