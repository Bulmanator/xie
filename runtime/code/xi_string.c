FileScope S64 NullTerminatedLength(const char *data) {
    S64 result = 0;
    if (data) {
        while (data[result] != 0) {
            result += 1;
        }
    }

    return result;
}

Str8 Str8_WrapNullTerminated(const char *data) {
    Str8 result;
    result.count = NullTerminatedLength(data);
    result.data  = cast(U8 *) data;

    return result;
}

Str8 Str8_PushCopy(M_Arena *arena, Str8 str) {
    Str8 result;
    result.count = str.count;
    result.data  = M_ArenaPush(arena, U8, result.count + 1, M_ARENA_NO_ZERO);

    MemoryCopy(result.data, str.data, result.count);

    result.data[result.count] = 0;

    return result;
}

B32 Str8_Equal(Str8 a, Str8 b, StringCompareFlags flags) {
    B32 result = (a.data == b.data) && (a.count == b.count); // exactly the same don't do anything
    if (!result) {
        B32 ignore_case = (flags & STRING_COMPARE_IGNORE_CASE);
        B32 inexact_rhs = (flags & STRING_COMPARE_INEXACT_RHS);

        S64 count = a.count;
        if (inexact_rhs) {
            count  = Min(a.count, b.count);
            result = true;
        }
        else {
            result = (a.count == b.count);
        }

        if (result) {
            for (S64 it = 0; it < count; ++it) {
                U8 cpa = a.data[it];
                U8 cpb = b.data[it];

                if (ignore_case) {
                    cpa = U8CharToLowercase(cpa);
                    cpb = U8CharToLowercase(cpb);
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

#include <stdio.h> // for vsnprintf

// @todo: we will probably want to move away from vsnprintf at some point as it would allow us
// to print our own types like vectors, matrices and have direct support for counted xi_strings
//
// :crt_dep
//

FileScope S64 Str8_FormatProcess(Str8 out, const char *format, va_list args) {
    S64 result = vsnprintf((char *) out.data, out.count, format, args);
    return result;
}

Str8 Str8_FormatArgs(M_Arena *arena, const char *format, va_list args) {
    Str8 result;

    va_list copy;
    va_copy(copy, args);

    Str8 test;
    test.count = 1024;
    test.data  = M_ArenaPush(arena, U8, test.count);

    S64 count = Str8_FormatProcess(test, format, args);
    if (count > test.count) {
        M_ArenaPopLast(arena);

        result.count = count;
        result.data  = M_ArenaPush(arena, U8, count);

        Str8_FormatProcess(result, format, copy);
    }
    else {
        result.data  = test.data;
        result.count = count;

        M_ArenaPopSize(arena, test.count - count);
    }

    return result;
}

Str8 Str8_FormatArgsToBuffer(Buffer *buffer, const char *format, va_list args) {
    Str8 result;

    result.data  = &buffer->data[buffer->used];
    result.count = (buffer->limit - buffer->used);

    S64 count    = Str8_FormatProcess(result, format, args);
    result.count = Min(count, result.count); // truncate if there isn't enough space

    buffer->used += result.count;

    Assert(buffer->used <= buffer->limit);

    return result;
}

Str8 Str8_Format(M_Arena *arena, const char *format, ...) {
    Str8 result;

    va_list args;
    va_start(args, format);

    result = Str8_FormatArgs(arena, format, args);

    va_end(args);

    return result;
}

Str8 Str8_FormatToBuffer(Buffer *buffer, const char *format, ...) {
    Str8 result;

    va_list args;
    va_start(args, format);

    result = Str8_FormatArgsToBuffer(buffer, format, args);

    va_end(args);

    return result;
}

Str8 Str8_Prefix(Str8 str, S64 count) {
    Str8 result;

    result.count = Min(count, str.count);
    result.data  = str.data;

    return result;
}

Str8 Str8_Suffix(Str8 str, S64 count) {
    Str8 result;

    result.count = Min(count, str.count);
    result.data  = str.data + (str.count - result.count);

    return result;
}

Str8 Str8_Advance(Str8 str, S64 count) {
    Str8 result;

    result.count = str.count - Min(count, str.count);
    result.data  = str.data  + Min(count, str.count);

    return result;
}

Str8 Str8_Remove(Str8 str, S64 count) {
    Str8 result;

    result.count = str.count - Min(count, str.count);
    result.data  = str.data;

    return result;
}

Str8 Str8_Slice(Str8 str, S64 start, S64 end) {
    Str8 result;

    Assert(start <= end);
    Assert((end - start) <= str.count);

    result.count = Min(str.count, end - start);
    result.data  = str.data + Min(str.count, start);

    return result;
}

B32 Str8_FindFirst(Str8 str, S64 *offset, U32 codepoint) {
    B32 result = false;

    S64 count = 0;
    while (str.count != 0) {
        Codepoint c = UTF8_Decode(str);

        if (c.value == codepoint) {
            *offset = count;
            result  = true;
            break;
        }

        count += c.count;
        str    = Str8_Advance(str, c.count);
    }

    return result;
}

B32 Str8_FindLast(Str8 str, S64 *offset, U32 codepoint) {
    B32 result = false;

    // We unfortunatley have to decode the entire string because variable length
    //

    S64 count = 0;
    while (str.count != 0) {
        Codepoint c = UTF8_Decode(str);

        if (c.value == codepoint) {
            // This will set the count to the last occurrence of codepoint
            //
            *offset = count;
            result  = true;
        }

        count += c.count;
        str    = Str8_Advance(str, c.count);
    }

    return result;
}

B32 Str8_StartsWith(Str8 str, Str8 prefix) {
    Str8 start = Str8_Prefix(str, prefix.count);
    B32 result = Str8_Equal(start, prefix, 0);
    return result;
}

B32 Str8_EndsWith(Str8 str, Str8 suffix) {
    Str8 end    = Str8_Suffix(str, suffix.count);
    B32  result = Str8_Equal(end, suffix, 0);
    return result;
}

Str8 Str8_RemoveAfterFirst(Str8 str, U32 codepoint) {
    Str8 result = str;

    S64 offset;
    if (Str8_FindFirst(str, &offset, codepoint)) {
        result.count = offset;
    }

    return result;
}

Str8 Str8_RemoveAfterLast(Str8 str, U32 codepoint) {
    Str8 result = str;

    S64 offset;
    if (Str8_FindLast(str, &offset, codepoint)) {
        result.count = offset;
    }

    return result;
}

Str8 Str8_RemoveBeforeFirst(Str8 str, U32 codepoint) {
    Str8 result = str;

    S64 offset;
    if (Str8_FindFirst(str, &offset, codepoint)) {
        // To know how many characters to advance to skip the occurrence of 'codepoint'
        // we need to encode the codepoint to UTF-8
        //
        // :Skip_Before
        //
        U8 encoding[4];
        U32 count = UTF8_Encode(encoding, codepoint);
        result    = Str8_Advance(str, offset + count);
    }

    return result;
}

Str8 Str8_RemoveBeforeLast(Str8 str, U32 codepoint) {
    Str8 result = str;

    S64 offset;
    if (Str8_FindLast(str, &offset, codepoint)) {
        // :Skip_Before
        //
        U8 encoding[4];
        U32 count = UTF8_Encode(encoding, codepoint);
        result    = Str8_Advance(str, offset + count);
    }

    return result;
}


Codepoint UTF8_Decode(Str8 str) {
    Codepoint result;

    LocalPersist const U8 lengths[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
    };

    LocalPersist const U32 masks[] = { 0x00, 0x7F, 0x1F, 0x0F, 0x07 };

    if (str.count > 0) {
        U8 *data = str.data;

        U32 len   = lengths[data[0] >> 3];
        U32 avail = cast(U32) Min(len, str.count);

        U32 codepoint = data[0] & masks[len];

        for (U32 it = 1; it < avail; ++it) {
            codepoint <<= 6;
            codepoint  |= (data[it] & 0x3F);
        }

        result.value = codepoint;
        result.count = avail; // don't go past the end of the string, may produce invalid codepoints
    }
    else {
        result.value = '?';
        result.count =  1 ;
    }

    return result;
}

U32 UTF8_Encode(U8 *output, U32 codepoint) {
    U32 result;

    if (codepoint <= 0x7F) {
        output[0] = cast(U8) codepoint;
        result    = 1;
    }
    else if (codepoint <= 0x7FF) {
        output[0] = ((codepoint >> 6) & 0x1F) | 0xC0;
        output[1] = ((codepoint >> 0) & 0x3F) | 0x80;
        result    = 2;
    }
    else if (codepoint <= 0xFFFF) {
        output[0] = ((codepoint >> 12) & 0x0F) | 0xE0;
        output[1] = ((codepoint >>  6) & 0x3F) | 0x80;
        output[2] = ((codepoint >>  0) & 0x3F) | 0x80;
        result    = 3;
    }
    else if (codepoint <= 0x10FFFF) {
        output[0] = ((codepoint >> 18) & 0x07) | 0xF0;
        output[1] = ((codepoint >> 12) & 0x3F) | 0x80;
        output[2] = ((codepoint >>  6) & 0x3F) | 0x80;
        output[3] = ((codepoint >>  0) & 0x3F) | 0x80;
        result    = 4;
    }
    else {
        output[0] = '?';
        result    =  1 ;
    }

    return result;
}
