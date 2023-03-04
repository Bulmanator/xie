string xi_str_wrap_count(u8 *data, uptr count) {
    string result;
    result.count = count;
    result.data  = data;

    return result;
}

string xi_str_wrap_range(u8 *start, u8 *end) {
    string result;
    result.count = (uptr) (end - start);
    result.data  = start;

    return result;
}

static uptr xi_str_count(u8 *data) {
    uptr result = 0;
    while (data[result] != 0) {
        result += 1;
    }

    return result;
}

string xi_str_wrap_cstr(u8 *data) {
    string result;
    result.count = xi_str_count(data);
    result.data  = data;

    return result;
}

b32 xi_str_is_valid(string str) {
    b32 result = (str.count > 0) && (str.data != 0);
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

extern XI_API string xi_str_prefix(string str, uptr count) {
    string result;
    result.count = XI_MIN(count, str.count);
    result.data  = str.data;

    return result;
}

extern XI_API string xi_str_suffix(string str, uptr count) {
    string result;
    result.count = XI_MIN(count, str.count);
    result.data  = str.data + (str.count - result.count);

    return result;
}

extern XI_API string xi_str_advance(string str, uptr count) {
    string result;
    result.count = str.count - XI_MIN(count, str.count);
    result.data  = str.data  + XI_MIN(count, str.count);

    return result;
}
extern XI_API string xi_str_remove(string str, uptr count) {
    string result;
    result.count = str.count - XI_MIN(count, str.count);
    result.data  = str.data;

    return result;
}

extern XI_API string xi_str_slice(string str, uptr start, uptr end) {
    string result;
    result.count = XI_MIN(str.count, end - start);
    result.data  = str.data + XI_MIN(str.count, start);

    return result;
}


