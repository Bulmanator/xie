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

xi_uptr xi_buffer_append(xi_buffer *out, void *base, xi_uptr count) {
    xi_uptr result = XI_MIN(count, out->limit - out->used);
    xi_memory_copy(out->data + out->used, base, result);

    out->used += result;

    return result;
}
