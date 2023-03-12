void xi_memory_zero(void *base, uptr size) {
    u8 *base8 = (u8 *) base;

    while (size--) {
        base8[0] = 0;
        base8++;
    }
}

void xi_memory_set(void *base, u8 value, uptr size) {
    u8 *base8 = (u8 *) base;

    while (size--) {
        base8[0] = value;
        base8++;
    }
}

void xi_memory_copy(void *dst, void *src, uptr size) {
    u8 *dst8 = (u8 *) dst;
    u8 *src8 = (u8 *) src;

    while (size--) {
        dst8[0] = src8[0];

        dst8++;
        src8++;
    }
}

u8 xi_char_to_lowercase(u8 c) {
    u8 result = (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
    return result;
}

u8 xi_char_to_uppercase(u8 c) {
    u8 result = (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c;
    return result;
}

uptr xi_buffer_append(buffer *out, void *base, uptr count) {
    uptr result = XI_MIN(count, out->limit - out->used);
    xi_memory_copy(out->data + out->used, base, result);

    out->used += result;

    return result;
}
