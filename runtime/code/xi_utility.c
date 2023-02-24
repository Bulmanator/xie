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


