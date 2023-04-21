void xi_memory_zero(void *base, xi_uptr size) {
    xi_u8 *base8 = (xi_u8 *) base;

    while (size--) {
        base8[0] = 0;
        base8++;
    }
}

void xi_memory_set(void *base, xi_u8 value, xi_uptr size) {
    xi_u8 *base8 = (xi_u8 *) base;

    while (size--) {
        base8[0] = value;
        base8++;
    }
}

void xi_memory_copy(void *dst, void *src, xi_uptr size) {
    xi_u8 *dst8 = (xi_u8 *) dst;
    xi_u8 *src8 = (xi_u8 *) src;

    while (size--) {
        dst8[0] = src8[0];

        dst8++;
        src8++;
    }
}

xi_u8 xi_char_to_lowercase(xi_u8 c) {
    xi_u8 result = (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
    return result;
}

xi_u8 xi_char_to_uppercase(xi_u8 c) {
    xi_u8 result = (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c;
    return result;
}

xi_u32 xi_str_djb2_hash_u32(xi_string str) {
    xi_u32 result = 5381;

    for (xi_uptr it = 0; it < str.count; ++it) {
        result = ((result << 5) + result) + str.data[it]; // hash * 33 + str.data[it]
    }

    return result;
}

xi_u32 xi_str_fnv1a_hash_u32(xi_string str) {
    xi_u32 result = 0x811c9dc5;
    for (xi_uptr it = 0; it < str.count; ++it) {
        result  = (result ^ (xi_u32) str.data[it]);
        result *= 0x1000193;
    }

    return result;
}

xi_b32 xi_str_parse_u32(xi_string str, xi_u32 *number) {
    xi_b32 result = true;

    xi_u32 total = 0;
    for (xi_uptr it = 0; it < str.count; ++it) {
        if (str.data[it] >= '0' && str.data[it] <= '9') {
            total *= 10;
            total += (str.data[it] - '0');
        }
        else {
            result = false;
            total  = 0;

            break;
        }
    }

    *number = total;

    return result;
}

xi_uptr xi_buffer_append(xi_buffer *buffer, void *base, xi_uptr count) {
    xi_uptr result = XI_MIN(count, buffer->limit - buffer->used);

    if (result > 0) {
        xi_memory_copy(buffer->data + buffer->used, base, result);
        buffer->used += result;
    }

    return result;
}

xi_b32 xi_buffer_put_u8(xi_buffer *buffer, xi_u8 value) {
    xi_b32 result = (buffer->limit - buffer->used) >= 1;

    if (result) { buffer->data[buffer->used++] = value; }
    return result;
}

xi_b32 xi_buffer_put_u16(xi_buffer *buffer, xi_u16 value) {
    xi_b32 result = (buffer->limit - buffer->used) >= 2;
    if (result) {
        xi_u8 *ptr = (xi_u8 *) &value;

        buffer->data[buffer->used++] = ptr[0];
        buffer->data[buffer->used++] = ptr[1];
    }

    return result;
}

xi_b32 xi_buffer_put_u32(xi_buffer *buffer, xi_u32 value) {
    xi_b32 result = (buffer->limit - buffer->used) >= 4;

    if (result) { xi_buffer_append(buffer, &value, 4); }
    return result;
}

xi_b32 xi_buffer_put_u64(xi_buffer *buffer, xi_u64 value) {
    xi_b32 result = (buffer->limit - buffer->used) >= 8;

    if (result) { xi_buffer_append(buffer, &value, 8); }
    return result;
}

void xi_logger_create(xiArena *arena, xiLogger *logger, xiFileHandle file, xi_u32 level, xi_uptr size) {
    if (file.status == XI_FILE_HANDLE_STATUS_VALID) {
        logger->file  = file;
        logger->level = level;

        logger->output.used  = 0;
        logger->output.limit = size;
        logger->output.data  = xi_arena_push_size(arena, logger->output.limit);
    }
}

xi_b32 xi_logger_create_from_path(xiArena *arena, xiLogger *logger, xi_string path, xi_u32 level, xi_uptr size) {
    xi_b32 result = false;
    if (xi_os_file_open(&logger->file, path, XI_FILE_ACCESS_FLAG_WRITE)) {
        logger->level = level;

        logger->output.used  = 0;
        logger->output.limit = size;
        logger->output.data  = xi_arena_push_size(arena, logger->output.limit);

        result = true;
    }

    return result;
}

void xi_log_print_args(xiLogger *logger, xi_u32 level, const char *format, va_list args) {
    if (logger->level <= level) {
        switch (level) {
            case XI_LOG_LEVEL_DEBUG:   { xi_str_format_buffer(&logger->output, "[DEBUG]   :: "); } break;
            case XI_LOG_LEVEL_INFO:    { xi_str_format_buffer(&logger->output, "[INFO]    :: "); } break;
            case XI_LOG_LEVEL_WARNING: { xi_str_format_buffer(&logger->output, "[WARNING] :: "); } break;
            case XI_LOG_LEVEL_ERROR:   { xi_str_format_buffer(&logger->output, "[ERROR]   :: "); } break;
            case XI_LOG_LEVEL_FATAL:   { xi_str_format_buffer(&logger->output, "[FATAL]   :: "); } break;
        }

        xi_str_format_buffer_args(&logger->output, format, args);

        xi_buffer_put_u8(&logger->output, '\n');

        XI_ASSERT(level != XI_LOG_LEVEL_FATAL);
    }
}

void xi_log_print(xiLogger *logger, xi_u32 level, const char *format, ...) {
    if (logger->level <= level) {
        switch (level) {
            case XI_LOG_LEVEL_DEBUG:   { xi_str_format_buffer(&logger->output, "[DEBUG]   :: "); } break;
            case XI_LOG_LEVEL_INFO:    { xi_str_format_buffer(&logger->output, "[INFO]    :: "); } break;
            case XI_LOG_LEVEL_WARNING: { xi_str_format_buffer(&logger->output, "[WARNING] :: "); } break;
            case XI_LOG_LEVEL_ERROR:   { xi_str_format_buffer(&logger->output, "[ERROR]   :: "); } break;
            case XI_LOG_LEVEL_FATAL:   { xi_str_format_buffer(&logger->output, "[FATAL]   :: "); } break;
        }

        va_list args;
        va_start(args, format);

        xi_str_format_buffer_args(&logger->output, format, args);

        va_end(args);

        xi_buffer_put_u8(&logger->output, '\n');

        XI_ASSERT(level != XI_LOG_LEVEL_FATAL);
    }
}


void xi_logger_flush(xiLogger *logger) {
    if ((logger->file.status == XI_FILE_HANDLE_STATUS_VALID) && (logger->output.used > 0)) {
        xi_os_file_write(&logger->file, logger->output.data, XI_FILE_OFFSET_APPEND, logger->output.used);
        logger->output.used = 0;
    }
}
