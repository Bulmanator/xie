#if !defined(XI_UTILITY_H_)
#define XI_UTILITY_H_

#if defined(__cplusplus)
extern "C" {
#endif

Func void MemoryZero(void *base, U64 size);
Func void MemoryCopy(void *dst, void *src, U64 size);
Func void MemorySet(void *base, U8 value, U64 size);

Func U8 U8CharToLowercase(U8 c);
Func U8 U8CharToUppercase(U8 c);

Func U32 Str8Djb2HashU32(Str8 str);
Func U32 Str8Fnv1aHashU32(Str8 str);

// returns true if the entire string was parsed successfully into 'number', otherwise false
//
Func B32 Str8ParseU32(Str8 str, U32 *number);

// returns the actual number of bytes copied, equal to count if sufficient space is available in the buffer,
// otherwise will be the size of the remainding space before copy
//
Func S64 BufferAppend(Buffer *buffer, void *base, S64 count);

// return true if they successfully put them into the buffer, if false nothing is copied
//
Func B32  BufferPutU8(Buffer *buffer, U8  value);
Func B32 BufferPutU16(Buffer *buffer, U16 value);
Func B32 BufferPutU32(Buffer *buffer, U32 value);
Func B32 BufferPutU64(Buffer *buffer, U64 value);

// logging interface
//
typedef struct Logger Logger;
struct Logger {
    FileHandle file;
    Buffer output;
};

// create a new logger with a buffer of the given size from an existing file handle or open a new file
// at a given path to log to
//
Func Logger LoggerCreate(M_Arena *arena, FileHandle file, S64 limit);
Func Logger LoggerCreateFromPath(M_Arena *arena, Str8 path, S64 limit);

Func void LoggerFlush(Logger *logger);

// print a log message into the loggers buffer.
//
// each line logged will automatically append informational styling and a new line, log lines will be
// output in the format:
//          [ident] :: format_string\n
//          format_string\n (if ident is empty or null)
//
// @todo: how to deal with buffer end? currently truncates
//
Func void LogPrintArgs(Logger *logger, Str8 ident, const char *format, va_list args);
Func void LogPrint(Logger *logger, Str8 ident, const char *format, ...);

#if defined(XI_NO_LOG)
    #define Log(...)
#else
    #define Log(logger, ident, format, ...) LogPrint(logger, ident, format, ##__VA_ARGS__)
#endif

#if defined(__cplusplus)
}
#endif

#endif  // XI_UTILITY_H_
