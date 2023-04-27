XI_INTERNAL xi_uptr xi_cstr_count(const char *data) {
    xi_uptr result = 0;
    if (data) {
        while (data[result] != 0) {
            result += 1;
        }
    }

    return result;
}

xi_string xi_str_wrap_cstr(const char *data) {
    xi_string result;
    result.count = xi_cstr_count(data);
    result.data  = (xi_u8 *) data;

    return result;
}

xi_b32 xi_str_is_valid(xi_string str) {
    xi_b32 result = (str.count > 0) && (str.data != 0);
    return result;
}

xi_string xi_str_copy(xiArena *arena, xi_string str) {
    xi_string result;
    result.count = str.count;
    result.data  = xi_arena_push_copy(arena, str.data, result.count);

    return result;
}

xi_b32 xi_str_equal_flags(xi_string a, xi_string b, xi_u32 flags) {
    xi_b32 result = (a.data == b.data) && (a.count == b.count); // exactly the same don't do anything
    if (!result) {
        xi_b32 ignore_case = (flags & XI_STRING_COMPARE_IGNORE_CASE);
        xi_b32 inexact_rhs = (flags & XI_STRING_COMPARE_INEXACT_RHS);

        xi_uptr count = a.count;
        if (inexact_rhs) {
            count = XI_MIN(a.count, b.count);
            result = true;
        }
        else {
            result = (a.count == b.count);
        }

        if (result) {
            for (xi_uptr it = 0; it < count; ++it) {
                xi_u8 cpa = a.data[it];
                xi_u8 cpb = b.data[it];

                if (ignore_case) {
                    cpa = xi_char_to_lowercase(cpa);
                    cpb = xi_char_to_lowercase(cpb);
                }

                if (cpa != cpb) {
                    result = false;
                    break;
                }
            }
        }
    }

    return result;
}

xi_b32 xi_str_equal(xi_string a, xi_string b) {
    xi_b32 result = (a.data == b.data) && (a.count == b.count); // exactly the same don't do anything
    if (!result) {
        result = (a.count == b.count); // make sure length matches
        if (result) {
            for (xi_uptr it = 0; it < a.count; ++it) {
                if (a.data[it] != b.data[it]) { // compare data
                    result = false;
                    break;
                }
            }
        }
    }

    return result;
}

#include <stdio.h> // for vsnprintf

// @todo: we will probably want to move away from vsnprintf at some point as it would allow us
// to print our own types like vectors, matrices and have direct support for counted xi_strings
//
// :crt_dep
//

XI_INTERNAL xi_uptr xi_format_print(xi_string out, const char *format, va_list args) {
    xi_uptr result = vsnprintf((char *) out.data, out.count, format, args);
    return result;
}

xi_string xi_str_format_args(xiArena *arena, const char *format, va_list args) {
    xi_string result;

    va_list copy;
    va_copy(copy, args);

    xi_string test;
    test.count = 1024;
    test.data  = xi_arena_push_size(arena, test.count);

    xi_uptr count = xi_format_print(test, format, args);
    if (count > test.count) {
        xi_arena_pop_size(arena, test.count);

        result.count = count;
        result.data  = xi_arena_push_size(arena, count);

        xi_format_print(result, format, copy);
    }
    else {
        result.data  = test.data;
        result.count = count;

        xi_arena_pop_size(arena, test.count - count);
    }

    return result;
}

xi_string xi_str_format_buffer_args(xi_buffer *b, const char *format, va_list args) {
    xi_string result;

    result.data  = &b->data[b->used];
    result.count = (b->limit - b->used);

    xi_uptr count = xi_format_print(result, format, args);
    result.count  = XI_MIN(count, result.count); // truncate if there isn't enough space

    b->used += result.count;
    XI_ASSERT(b->used <= b->limit);

    return result;
}

xi_string xi_str_format(xiArena *arena, const char *format, ...) {
    xi_string result;

    va_list args;
    va_start(args, format);

    result = xi_str_format_args(arena, format, args);

    va_end(args);

    return result;
}

xi_string xi_str_format_buffer(xi_buffer *b, const char *format, ...) {
    xi_string result;

    va_list args;
    va_start(args, format);

    result = xi_str_format_buffer_args(b, format, args);

    va_end(args);

    return result;
}

xi_string xi_str_prefix(xi_string str, xi_uptr count) {
    xi_string result;

    result.count = XI_MIN(count, str.count);
    result.data  = str.data;

    return result;
}

xi_string xi_str_suffix(xi_string str, xi_uptr count) {
    xi_string result;

    result.count = XI_MIN(count, str.count);
    result.data  = str.data + (str.count - result.count);

    return result;
}

xi_string xi_str_advance(xi_string str, xi_uptr count) {
    xi_string result;

    result.count = str.count - XI_MIN(count, str.count);
    result.data  = str.data  + XI_MIN(count, str.count);

    return result;
}

xi_string xi_str_remove(xi_string str, xi_uptr count) {
    xi_string result;

    result.count = str.count - XI_MIN(count, str.count);
    result.data  = str.data;

    return result;
}

xi_string xi_str_slice(xi_string str, xi_uptr start, xi_uptr end) {
    xi_string result;

    XI_ASSERT(start <= end);
    XI_ASSERT((end - start) <= str.count);

    result.count = XI_MIN(str.count, end - start);
    result.data  = str.data + XI_MIN(str.count, start);

    return result;
}

xi_b32 xi_str_find_first(xi_string str, xi_uptr *offset, xi_u32 codepoint) {
    xi_b32 result = false;

    // @incomplete: decode utf8 correctly, this only supports ascii
    //
    // :utf8
    //
    xi_u8 cp = (xi_u8) codepoint;

    xi_uptr index = 0;
    while (str.count--) {
        if (str.data[index] == cp) {
            *offset = index;
            result  = true;
            break;
        }

        index += 1;
    }

    return result;
}

xi_b32 xi_str_find_last(xi_string str, xi_uptr *offset, xi_u32 codepoint) {
    xi_b32 result = false;

    // :utf8
    //
    xi_u8 cp = (xi_u8) codepoint;

    xi_uptr index = str.count;
    while (index--) {
        if (str.data[index] == cp) {
            *offset = index;
            result  = true;
            break;
        }
    }

    return result;
}

xi_b32 xi_str_starts_with(xi_string str, xi_string prefix) {
    xi_string start = xi_str_prefix(str, prefix.count);

    xi_b32 result = xi_str_equal(start, prefix);
    return result;
}

xi_b32 xi_str_ends_with(xi_string str, xi_string suffix) {
    xi_string end = xi_str_suffix(str, suffix.count);

    xi_b32 result = xi_str_equal(end, suffix);
    return result;
}
