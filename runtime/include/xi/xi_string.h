#if !defined(XI_STRING_H_)
#define XI_STRING_H_

// string utility functions, strings are expected to be encoded in utf-8.
// _all_ counts are expected to be in number of bytes, not characters
//

#define xi_str_wrap_const(x) xi_str_wrap_count((u8 *) (x), sizeof(x) - sizeof(*(x)))

// this will unpack a 'string' into the format for %*.s in a format string
//
#define xi_str_unpack(x) (int) (x).count, (x).data

extern XI_API string xi_str_wrap_count(u8 *data, uptr count);
extern XI_API string xi_str_wrap_range(u8 *start, u8 *end);
extern XI_API string xi_str_wrap_cstr(u8 *data); // null-terminated

extern XI_API b32 xi_str_is_valid(string str);

extern XI_API string xi_str_copy(xiArena *arena, string str);

enum xiStringCompareFlags {
    XI_STRING_COMPARE_IGNORE_CASE = (1 << 0),
    XI_STRING_COMPARE_INEXACT_RHS = (1 << 1)
};

extern XI_API b32 xi_str_equal_flags(string a, string b, u32 flags);
extern XI_API b32 xi_str_equal(string a, string b);

// format strings
//
extern XI_API string xi_str_format_args(xiArena *arena, const char *format, va_list args);
extern XI_API string xi_str_format_buffer_args(buffer *b, const char *format, va_list args);

extern XI_API string xi_str_format(xiArena *arena, const char *format, ...);
extern XI_API string xi_str_format_buffer(buffer *b, const char *format, ...);

extern XI_API string xi_str_prefix(string str, uptr count);
extern XI_API string xi_str_suffix(string str, uptr count);

extern XI_API string xi_str_advance(string str, uptr count);
extern XI_API string xi_str_remove(string str, uptr count);

extern XI_API string xi_str_slice(string str, uptr start, uptr end);

// up to the first occurrence of 'codepoint'
//
// 'offset' will be filled out with the byte offset into the string where codepoint was found and will
// return true, if the codepoint is not found the function will return false and the contents of 'offset'
// will remain unmodified
//
extern XI_API b32 xi_str_find_first(string str, uptr *offset, u32 codepoint);
extern XI_API b32 xi_str_find_last(string str,  uptr *offset, u32 codepoint);

#endif  // XI_STRING_H_
