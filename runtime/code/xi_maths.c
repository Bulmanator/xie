xi_u32 xi_pow2_next_u32(xi_u32 x) {
    xi_u32 result = x;

    // @todo: how do we want this to work with inputs that are already powers of 2,
    // currently it just returns the value itself, but maybe we want to shift it up by
    // 1 in this case
    //

    result--;
    result |= (result >>  1);
    result |= (result >>  2);
    result |= (result >>  4);
    result |= (result >>  8);
    result |= (result >> 16);
    result++;

    return result;
}

xi_u32 xi_pow2_prev_u32(xi_u32 x) {
    xi_u32 result = xi_pow2_next_u32(x) >> 1;
    return result;
}

xi_u32 xi_pow2_nearest_u32(xi_u32 x) {
    xi_u32 next = xi_pow2_next_u32(x);
    xi_u32 prev = next >> 1;

    xi_u32 result = (next - x) > (x - prev) ? prev : next;
    return result;
}
