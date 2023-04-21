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

extern XI_API xi_u32 xi_djb2_str_hash_u32(xi_string str);
extern XI_API xi_u32 xi_fnv1a_str_hash_u32(xi_string str);

// returns true if the entire string was parsed successfully into 'number', otherwise false
//
extern XI_API xi_b32 xi_str_parse_u32(xi_string str, xi_u32 *number);

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

// logging interface
//
enum xiLogLevel {
    XI_LOG_LEVEL_DEBUG = 0,
    XI_LOG_LEVEL_INFO,
    XI_LOG_LEVEL_WARNING,
    XI_LOG_LEVEL_ERROR,
    XI_LOG_LEVEL_FATAL // will fire an assert if assertions are enabled
};

typedef struct xiLogger {
    xiFileHandle file;

    xi_u32 level;
    xi_buffer output;
} xiLogger;

// create a new logger with a buffer of the given size from an existing file handle or open a new file
// at a given path to log to
//
// it will only log messages which are greater than or equal to its 'level'
//
extern XI_API void xi_logger_create(xiArena *arena, xiLogger *logger,
        xiFileHandle file, xi_u32 level, xi_uptr size);

extern XI_API xi_b32 xi_logger_create_from_path(xiArena *arena, xiLogger *logger,
        xi_string path, xi_u32 level, xi_uptr size);

// print a log message into the loggers buffer.
//
// each line logged will automatically append informational styling and a new line, log lines will be
// output in the format:
//          [LEVEL] :: format_string\n
//
// @todo: how to deal with buffer end? currently truncates
//
extern XI_API void xi_log_print_args(xiLogger *logger, xi_u32 level, const char *format, va_list args);
extern XI_API void xi_log_print(xiLogger *logger, xi_u32 level, const char *format, ...);

// :note this can be used to completely disable logging in a build if desired, if the user calls
// xi_log_print or xi_log_print_args directly their logging will not be affacted by this
//
#if defined(XI_NO_LOG)
    #define xi_log(...)
#else
    #define xi_log(logger, level, format, ...) xi_log_print(logger, level, format, ##__VA_ARGS__)
#endif

// write the current contents of the logger out to its file handle, this can be called at any point
//
extern XI_API void xi_logger_flush(xiLogger *logger);

#if defined(__cplusplus)
}
#endif

#endif  // XI_UTILITY_H_
