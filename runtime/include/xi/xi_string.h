#if !defined(XI_STRING_H_)
#define XI_STRING_H_

//
// :Note String utilities expect UTF-8 encoded strings, counts are expected in bytes not characters
//

#define Str8_Arg(x) (int) (x).count, (x).data

//
// --------------------------------------------------------------------------------
// :String_Wrappers
// --------------------------------------------------------------------------------
//

Func    Str8 Str8_WrapNullTerminated(const char *data);
Inline  Str8 Str8_WrapCount(U8 *data,  S64 count);
Inline  Str8 Str8_WrapRange(U8 *start, U8  *end);
#define Str8_Literal(x) Str8_WrapCount((U8 *) (x), sizeof(x) - sizeof(*(x)))

Func Str8 Str8_PushCopy(M_Arena *arena, Str8 str); // this will null-terminated the string

//
// --------------------------------------------------------------------------------
// :String_Compare
// --------------------------------------------------------------------------------
//

typedef U32 StringCompareFlags;
enum StringCompareFlags {
    STRING_COMPARE_IGNORE_CASE = (1 << 0),
    STRING_COMPARE_INEXACT_RHS = (1 << 1)
};

Func B32 Str8_Equal(Str8 a, Str8 b, StringCompareFlags flags);

//
// --------------------------------------------------------------------------------
// :String_Format
// --------------------------------------------------------------------------------
//

Func Str8 Str8_FormatArgs(M_Arena *arena, const char *format, va_list args);
Func Str8 Str8_FormatArgsToBuffer(Buffer *buffer, const char *format, va_list args);

Func Str8 Str8_Format(M_Arena *arena, const char *format, ...);
Func Str8 Str8_FormatToBuffer(Buffer *buffer, const char *format, ...);

//
// --------------------------------------------------------------------------------
// :String_Manipulation
// --------------------------------------------------------------------------------
//

Func Str8 Str8_Prefix(Str8 str, S64 count);
Func Str8 Str8_Suffix(Str8 str, S64 count);

Func Str8 Str8_Advance(Str8 str, S64 count);
Func Str8 Str8_Remove(Str8 str, S64 count);

Func Str8 Str8_Slice(Str8 str, S64 start, S64 end);

// up to the first occurrence of 'codepoint'
//
// 'offset' will be filled out with the byte offset into the string where codepoint was found and will
// return true, if the codepoint is not found the function will return false and the contents of 'offset'
// will remain unmodified
//
Func B32 Str8_FindFirst(Str8 str, S64 *offset, U32 codepoint);
Func B32 Str8_FindLast(Str8 str,  S64 *offset, U32 codepoint);

Func B32 Str8_StartsWith(Str8 str, Str8 prefix);
Func B32 Str8_EndsWith(Str8 str, Str8 suffix);

// These all remove the instance of 'codepoint' that was found, if no instance is found the
// returned string is the same as 'str'
//
Func Str8 Str8_RemoveAfterFirst(Str8 str, U32 codepoint);
Func Str8 Str8_RemoveAfterLast (Str8 str, U32 codepoint);

Func Str8 Str8_RemoveBeforeFirst(Str8 str, U32 codepoint);
Func Str8 Str8_RemoveBeforeLast (Str8 str, U32 codepoint);

//
// --------------------------------------------------------------------------------
// :Unicode_Coding
// --------------------------------------------------------------------------------
//

typedef struct Codepoint Codepoint;
struct Codepoint {
    U32 value; // the value of the codepoint
    U32 count; // number of bytes that were consumed
};

Func Codepoint UTF8_Decode(Str8 str);

// Expects 'output' to be 4 bytes long if needed by codepoint, puts the encoded value into output and
// returns the number of bytes used to encode the value
//
Func U32 UTF8_Encode(U8 *output, U32 codepoint);

#endif  // XI_STRING_H_
