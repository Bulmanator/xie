void MemoryZero(void *base, U64 size) {
    U8 *base8 = cast(U8 *) base;

    while (size--) {
        *base8++ = 0;
    }
}

void MemoryCopy(void *dst, void *src, U64 size) {
    U8 *dst8 = cast(U8 *) dst;
    U8 *src8 = cast(U8 *) src;

    while (size--) {
        *dst8++ = *src8++;
    }
}

void MemorySet(void *base, U8 value, U64 size) {
    U8 *base8 = cast(U8 *) base;

    while (size--) {
        *base8++ = value;
    }
}

U8 U8CharToLowercase(U8 c) {
    U8 result = (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
    return result;
}

U8 U8CharToUppercase(U8 c) {
    U8 result = (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c;
    return result;
}

U32 Str8_Djb2HashU32(Str8 str) {
    U32 result = 5381;

    for (S64 it = 0; it < str.count; ++it) {
        result = ((result << 5) + result) + str.data[it]; // hash * 33 + str.data[it]
    }

    return result;
}

U32 Str8_Fnv1aHashU32(Str8 str) {
    U32 result = 0x811c9dc5;
    for (S64 it = 0; it < str.count; ++it) {
        result  = (result ^ cast(U32) str.data[it]);
        result *= 0x1000193;
    }

    return result;
}

S64 Str8_ParseInteger(Str8 str) {
    S64 result = 0;
    S64 sign   = 1;

    for (S64 it = 0; it < str.count; ++it) {
        if (it == 0 && str.data[it] == '-') {
            sign = -1;
        }
        else if (str.data[it] >= '0' && str.data[it] <= '9') {
            result *= 10;
            result += (str.data[it] - '0');
        }
        else if (it != 0 || str.data[it] != '+') {
            break;
        }
    }

    result *= sign;

    return result;
}

S64 BufferAppend(Buffer *buffer, void *base, S64 count) {
    S64 result = Min(count, buffer->limit - buffer->used);

    if (result > 0) {
        MemoryCopy(buffer->data + buffer->used, base, result);
        buffer->used += result;
    }

    return result;
}

B32 BufferPutU8(Buffer *buffer, U8 value) {
    B32 result = (buffer->limit - buffer->used) >= 1;

    if (result) { buffer->data[buffer->used++] = value; }
    return result;
}

B32 BufferPutU16(Buffer *buffer, U16 value) {
    B32 result = (buffer->limit - buffer->used) >= 2;
    if (result) {
        U8 *ptr = cast(U8 *) &value;

        buffer->data[buffer->used++] = ptr[0];
        buffer->data[buffer->used++] = ptr[1];
    }

    return result;
}

B32 BufferPutU32(Buffer *buffer, U32 value) {
    B32 result = (buffer->limit - buffer->used) >= 4;

    if (result) { BufferAppend(buffer, &value, 4); }
    return result;
}

B32 BufferPutU64(Buffer *buffer, U64 value) {
    B32 result = (buffer->limit - buffer->used) >= 8;

    if (result) { BufferAppend(buffer, &value, 8); }
    return result;
}

Logger LoggerCreate(M_Arena *arena, FileHandle file, S64 size) {
    Logger result = { 0 };

    if (file.status == FILE_HANDLE_STATUS_VALID) {
        result.file = file;

        result.output.used  = 0;
        result.output.limit = size;
        result.output.data  = M_ArenaPush(arena, U8, result.output.limit);
    }

    return result;
}

Logger LoggerCreateFromPath(M_Arena *arena, Str8 path, S64 size) {
    Logger result = { 0 };

    result.file         = OS_FileOpen(path, FILE_ACCESS_FLAG_WRITE);
    result.output.used  = 0;
    result.output.limit = size;
    result.output.data  = M_ArenaPush(arena, U8, result.output.limit);

    return result;
}

void LoggerFlush(Logger *logger) {
    if ((logger->file.status == FILE_HANDLE_STATUS_VALID) && (logger->output.used > 0)) {
        OS_FileWrite(&logger->file, logger->output.data, FILE_OFFSET_APPEND, logger->output.used);
        logger->output.used = 0;
    }
}

void LogPrintArgs(Logger *logger, Str8 ident, const char *format, va_list args) {
    if (ident.count) {
        Str8_FormatToBuffer(&logger->output, "[%.*s] :: ", Str8_Arg(ident));
    }

    Str8_FormatArgsToBuffer(&logger->output, format, args);
}

void LogPrint(Logger *logger, Str8 ident, const char *format, ...) {
    if (ident.count) {
        Str8_FormatToBuffer(&logger->output, "[%.*s] :: ", Str8_Arg(ident));
    }

    va_list args;
    va_start(args, format);

    Str8_FormatArgsToBuffer(&logger->output, format, args);

    va_end(args);
}

FileScope B32 Xi_GameCodeIsValid(Xi_GameCode *game) {
    B32 result = (game->Init != 0) && (game->Simulate != 0) && (game->Render != 0);
    return result;
}

FileScope void InputButtonHandle(InputButton *button, B32 down) {
    if (button->down != down) {
        button->pressed  = button->pressed  || (!button->down &&  down);
        button->released = button->released || ( button->down && !down);
        button->down     = down;
        button->repeat   = 0;
    }
    else {
        button->repeat += 1;
    }
}
