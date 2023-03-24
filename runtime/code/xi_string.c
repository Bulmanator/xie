string xi_str_wrap_count(u8 *data, uptr count) {
    string result;
    result.count = count;
    result.data  = data;

    return result;
}

string xi_str_wrap_range(u8 *start, u8 *end) {
    string result;

    XI_ASSERT(start <= end);

    result.count = (uptr) (end - start);
    result.data  = start;

    return result;
}

static uptr xi_cstr_count(u8 *data) {
    uptr result = 0;
    while (data[result] != 0) {
        result += 1;
    }

    return result;
}

string xi_str_wrap_cstr(u8 *data) {
    string result;
    result.count = xi_cstr_count(data);
    result.data  = data;

    return result;
}

b32 xi_str_is_valid(string str) {
    b32 result = (str.count > 0) && (str.data != 0);
    return result;
}

string xi_str_copy(xiArena *arena, string str) {
    string result;
    result.count = str.count;
    result.data  = xi_arena_push_copy(arena, str.data, result.count);

    return result;
}

b32 xi_str_equal_flags(string a, string b, u32 flags) {
    b32 result = (a.data == b.data) && (a.count == b.count); // exactly the same don't do anything
    if (!result) {
        b32 ignore_case = (flags & XI_STRING_COMPARE_IGNORE_CASE);
        b32 inexact_rhs = (flags & XI_STRING_COMPARE_INEXACT_RHS);

        uptr count = a.count;
        if (inexact_rhs) {
            count = XI_MIN(a.count, b.count);
        }
        else {
            result = (a.count == b.count);
        }

        if (result) {
            for (uptr it = 0; it < count; ++it) {
                // @todo: correctly decode utf-8
                //
                u8 cpa = a.data[it];
                u8 cpb = b.data[it];

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

b32 xi_str_equal(string a, string b) {
    b32 result = (a.data == b.data) && (a.count == b.count); // exactly the same don't do anything
    if (!result) {
        result = (a.count == b.count); // make sure length matches
        if (result) {
            for (uptr it = 0; it < a.count; ++it) {
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
// to print our own types like vectors, matrices and have direct support for counted strings
//

static uptr xi_format_print(string out, const char *format, va_list args) {
    uptr result = vsnprintf((char *) out.data, out.count, format, args);
    return result;
}

string xi_str_format_args(xiArena *arena, const char *format, va_list args) {
    string result;

    va_list copy;
    va_copy(copy, args);

    string test;
    test.count = 1024;
    test.data  = xi_arena_push_size(arena, test.count);

    uptr count = xi_format_print(test, format, args);
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

string xi_str_format_buffer_args(buffer *b, const char *format, va_list args) {
    string result;

    result.data  = &b->data[b->used];
    result.count = (b->limit - b->used);

    xi_format_print(result, format, args);

    return result;
}

string xi_str_format(xiArena *arena, const char *format, ...) {
    string result;

    va_list args;
    va_start(args, format);

    result = xi_str_format_args(arena, format, args);

    va_end(args);

    return result;
}

string xi_str_format_buffer(buffer *b, const char *format, ...) {
    string result;

    va_list args;
    va_start(args, format);

    result = xi_str_format_buffer_args(b, format, args);

    va_end(args);

    return result;
}

string xi_str_prefix(string str, uptr count) {
    string result;

    XI_ASSERT(str.count >= count);

    result.count = XI_MIN(count, str.count);
    result.data  = str.data;

    return result;
}

string xi_str_suffix(string str, uptr count) {
    string result;

    XI_ASSERT(str.count >= count);

    result.count = XI_MIN(count, str.count);
    result.data  = str.data + (str.count - result.count);

    return result;
}

string xi_str_advance(string str, uptr count) {
    string result;

    XI_ASSERT(str.count >= count);

    result.count = str.count - XI_MIN(count, str.count);
    result.data  = str.data  + XI_MIN(count, str.count);

    return result;
}

string xi_str_remove(string str, uptr count) {
    string result;

    XI_ASSERT(str.count >= count);

    result.count = str.count - XI_MIN(count, str.count);
    result.data  = str.data;

    return result;
}

string xi_str_slice(string str, uptr start, uptr end) {
    string result;

    XI_ASSERT(start <= end);
    XI_ASSERT((end - start) <= str.count);

    result.count = XI_MIN(str.count, end - start);
    result.data  = str.data + XI_MIN(str.count, start);

    return result;
}

b32 xi_str_find_first(string str, uptr *offset, u32 codepoint) {
    b32 result = false;

    // @incomplete: decode utf8 correctly, this only supports ascii
    //
    // :utf8
    //
    u8 cp = (u8) codepoint;

    uptr index = 0;
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

b32 xi_str_find_last(string str, uptr *offset, u32 codepoint) {
    b32 result = false;

    // :utf8
    //
    u8 cp = (u8) codepoint;

    uptr index = str.count;
    while (index--) {
        if (str.data[index] == cp) {
            *offset = index;
            result  = true;
            break;
        }
    }

    return result;
}
