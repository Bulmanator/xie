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

xi_u32 xi_str_djb2_hash(xi_string str) {
    xi_u32 result = 5381;

    for (xi_uptr it = 0; it < str.count; ++it) {
        result = ((result << 5) + result) + str.data[it]; // hash * 33 + str.data[it]
    }

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


