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

XI_GLOBAL const xi_f32 tbl_srgb_u8_to_linear_f32[] = {
    // 'exact' representation table for lookup as there are only 256 possible values
    //
    0x0.000000p+0,  0x1.3e4568p-12, 0x1.3e4568p-11, 0x1.dd681ep-11, 0x1.3e4568p-10, 0x1.8dd6c2p-10,
    0x1.dd681ep-10, 0x1.167cbcp-9,  0x1.3e4568p-9,  0x1.660e16p-9,  0x1.8dd6c2p-9,  0x1.b6a31ap-9,
    0x1.e1e31ap-9,  0x1.07c38cp-8,  0x1.1fcc2cp-8,  0x1.390ffap-8,  0x1.53936ep-8,  0x1.6f5adep-8,
    0x1.8c6a92p-8,  0x1.aac6c2p-8,  0x1.ca7382p-8,  0x1.eb74e0p-8,  0x1.06e76ap-7,  0x1.18c2a4p-7,
    0x1.2b4e06p-7,  0x1.3e8b7cp-7,  0x1.527cd6p-7,  0x1.6723eep-7,  0x1.7c8292p-7,  0x1.929a86p-7,
    0x1.a96d8ep-7,  0x1.c0fd62p-7,  0x1.d94bbep-7,  0x1.f25a44p-7,  0x1.061550p-6,  0x1.135f3ep-6,
    0x1.210bb6p-6,  0x1.2f1b8ap-6,  0x1.3d8f84p-6,  0x1.4c6866p-6,  0x1.5ba6fap-6,  0x1.6b4c02p-6,
    0x1.7b5840p-6,  0x1.8bcc72p-6,  0x1.9ca956p-6,  0x1.adefaap-6,  0x1.bfa020p-6,  0x1.d1bb72p-6,
    0x1.e44262p-6,  0x1.f7358cp-6,  0x1.054ad8p-5,  0x1.0f31bep-5,  0x1.194fcep-5,  0x1.23a564p-5,
    0x1.2e32cap-5,  0x1.38f862p-5,  0x1.43f67ep-5,  0x1.4f2d6ap-5,  0x1.5a9d7cp-5,  0x1.664708p-5,
    0x1.722a5cp-5,  0x1.7e47ccp-5,  0x1.8a9fa8p-5,  0x1.97323ep-5,  0x1.a3ffdep-5,  0x1.b108d4p-5,
    0x1.be4d6ep-5,  0x1.cbcdfcp-5,  0x1.d98ac8p-5,  0x1.e7841ep-5,  0x1.f5ba52p-5,  0x1.0216cep-4,
    0x1.096f2ap-4,  0x1.10e660p-4,  0x1.187c94p-4,  0x1.2031ecp-4,  0x1.28068ap-4,  0x1.2ffa94p-4,
    0x1.380e2cp-4,  0x1.404176p-4,  0x1.489496p-4,  0x1.5107aep-4,  0x1.599ae0p-4,  0x1.624e50p-4,
    0x1.6b2224p-4,  0x1.741676p-4,  0x1.7d2b6ap-4,  0x1.866124p-4,  0x1.8fb7c4p-4,  0x1.992f6cp-4,
    0x1.a2c83ep-4,  0x1.ac8258p-4,  0x1.b65ddep-4,  0x1.c05aeep-4,  0x1.ca79aap-4,  0x1.d4ba32p-4,
    0x1.df1caap-4,  0x1.e9a126p-4,  0x1.f447d4p-4,  0x1.ff10c8p-4,  0x1.04fe12p-3,  0x1.0a8504p-3,
    0x1.101d4ap-3,  0x1.15c6f2p-3,  0x1.1b820ep-3,  0x1.214eaap-3,  0x1.272cd8p-3,  0x1.2d1ca6p-3,
    0x1.331e22p-3,  0x1.39315cp-3,  0x1.3f5664p-3,  0x1.458d46p-3,  0x1.4bd616p-3,  0x1.5230dap-3,
    0x1.589da8p-3,  0x1.5f1c8ap-3,  0x1.65ad90p-3,  0x1.6c50cap-3,  0x1.730644p-3,  0x1.79ce0cp-3,
    0x1.80a832p-3,  0x1.8794c4p-3,  0x1.8e93d0p-3,  0x1.95a562p-3,  0x1.9cc98cp-3,  0x1.a40056p-3,
    0x1.ab49d2p-3,  0x1.b2a60ep-3,  0x1.ba1516p-3,  0x1.c196f8p-3,  0x1.c92bc0p-3,  0x1.d0d37ep-3,
    0x1.d88e40p-3,  0x1.e05c18p-3,  0x1.e83d08p-3,  0x1.f03120p-3,  0x1.f83870p-3,  0x1.002984p-2,
    0x1.044078p-2,  0x1.08611ap-2,  0x1.0c8b74p-2,  0x1.10bf8ap-2,  0x1.14fd64p-2,  0x1.194506p-2,
    0x1.1d967ap-2,  0x1.21f1c2p-2,  0x1.2656e8p-2,  0x1.2ac5f0p-2,  0x1.2f3ee2p-2,  0x1.33c1c4p-2,
    0x1.384e9cp-2,  0x1.3ce56ep-2,  0x1.418644p-2,  0x1.463124p-2,  0x1.4ae610p-2,  0x1.4fa512p-2,
    0x1.546e30p-2,  0x1.59416ep-2,  0x1.5e1ed2p-2,  0x1.630666p-2,  0x1.67f830p-2,  0x1.6cf430p-2,
    0x1.71fa6ep-2,  0x1.770af2p-2,  0x1.7c25c2p-2,  0x1.814ae2p-2,  0x1.867a5ap-2,  0x1.8bb42ep-2,
    0x1.90f866p-2,  0x1.964706p-2,  0x1.9ba016p-2,  0x1.a1039ap-2,  0x1.a67198p-2,  0x1.abea16p-2,
    0x1.b16d1ap-2,  0x1.b6faa8p-2,  0x1.bc92cap-2,  0x1.c23582p-2,  0x1.c7e2d6p-2,  0x1.cd9acep-2,
    0x1.d35d6cp-2,  0x1.d92abap-2,  0x1.df02bap-2,  0x1.e4e574p-2,  0x1.ead2ecp-2,  0x1.f0cb28p-2,
    0x1.f6ce2ep-2,  0x1.fcdc04p-2,  0x1.017a5ap-1,  0x1.048c1cp-1,  0x1.07a34ep-1,  0x1.0abff2p-1,
    0x1.0de210p-1,  0x1.1109a2p-1,  0x1.1436aep-1,  0x1.17693ap-1,  0x1.1aa144p-1,  0x1.1dded2p-1,
    0x1.2121e4p-1,  0x1.246a80p-1,  0x1.27b8a6p-1,  0x1.2b0c5ap-1,  0x1.2e659ep-1,  0x1.31c476p-1,
    0x1.3528e2p-1,  0x1.3892e6p-1,  0x1.3c0286p-1,  0x1.3f77c2p-1,  0x1.42f2a0p-1,  0x1.46731ep-1,
    0x1.49f944p-1,  0x1.4d8510p-1,  0x1.511686p-1,  0x1.54adaap-1,  0x1.584a7ep-1,  0x1.5bed02p-1,
    0x1.5f953cp-1,  0x1.634332p-1,  0x1.66f6dcp-1,  0x1.6ab044p-1,  0x1.6e6f6ap-1,  0x1.723450p-1,
    0x1.75fefcp-1,  0x1.79cf6ep-1,  0x1.7da5a8p-1,  0x1.8181acp-1,  0x1.856380p-1,  0x1.894b24p-1,
    0x1.8d389ap-1,  0x1.912be6p-1,  0x1.952508p-1,  0x1.992406p-1,  0x1.9d28e0p-1,  0x1.a1339ap-1,
    0x1.a54434p-1,  0x1.a95ab2p-1,  0x1.ad7718p-1,  0x1.b19966p-1,  0x1.b5c19ep-1,  0x1.b9efc4p-1,
    0x1.be23dcp-1,  0x1.c25de4p-1,  0x1.c69ddep-1,  0x1.cae3d8p-1,  0x1.cf2fc6p-1,  0x1.d381b6p-1,
    0x1.d7d99ap-1,  0x1.dc3788p-1,  0x1.e09b70p-1,  0x1.e50568p-1,  0x1.e9755cp-1,  0x1.edeb66p-1,
    0x1.f26772p-1,  0x1.f6e996p-1,  0x1.fb71c0p-1,  0x1.000000p+0
};

XI_STATIC_ASSERT(sizeof(tbl_srgb_u8_to_linear_f32) == 256 * sizeof(xi_f32));

xi_f32 xi_srgb_u8_to_linear_f32(xi_u8 component) {
    // just a lookup table
    //
    xi_f32 result = tbl_srgb_u8_to_linear_f32[component];
    return result;
}

// https://gist.github.com/rygorous/2203834
//
XI_GLOBAL const xi_u32 tbl_linear_f32_to_srgb_u8[104] = {
    0x0073000d, 0x007a000d, 0x0080000d, 0x0087000d, 0x008d000d, 0x0094000d, 0x009a000d, 0x00a1000d,
    0x00a7001a, 0x00b4001a, 0x00c1001a, 0x00ce001a, 0x00da001a, 0x00e7001a, 0x00f4001a, 0x0101001a,
    0x010e0033, 0x01280033, 0x01410033, 0x015b0033, 0x01750033, 0x018f0033, 0x01a80033, 0x01c20033,
    0x01dc0067, 0x020f0067, 0x02430067, 0x02760067, 0x02aa0067, 0x02dd0067, 0x03110067, 0x03440067,
    0x037800ce, 0x03df00ce, 0x044600ce, 0x04ad00ce, 0x051400ce, 0x057b00c5, 0x05dd00bc, 0x063b00b5,
    0x06970158, 0x07420142, 0x07e30130, 0x087b0120, 0x090b0112, 0x09940106, 0x0a1700fc, 0x0a9500f2,
    0x0b0f01cb, 0x0bf401ae, 0x0ccb0195, 0x0d950180, 0x0e56016e, 0x0f0d015e, 0x0fbc0150, 0x10630143,
    0x11070264, 0x1238023e, 0x1357021d, 0x14660201, 0x156601e9, 0x165a01d3, 0x174401c0, 0x182401af,
    0x18fe0331, 0x1a9602fe, 0x1c1502d2, 0x1d7e02ad, 0x1ed4028d, 0x201a0270, 0x21520256, 0x227d0240,
    0x239f0443, 0x25c003fe, 0x27bf03c4, 0x29a10392, 0x2b6a0367, 0x2d1d0341, 0x2ebe031f, 0x304d0300,
    0x31d105b0, 0x34a80555, 0x37520507, 0x39d504c5, 0x3c37048b, 0x3e7c0458, 0x40a8042a, 0x42bd0401,
    0x44c20798, 0x488e071e, 0x4c1c06b6, 0x4f76065d, 0x52a50610, 0x55ac05cc, 0x5892058f, 0x5b590559,
    0x5e0c0a23, 0x631c0980, 0x67db08f6, 0x6c55087f, 0x70940818, 0x74a007bd, 0x787d076c, 0x7c330723,
};

xi_u8 xi_linear_f32_to_srgb_u8(xi_f32 component) {
    xi_u8 result;

    union f32_u32 {
        xi_f32 f;
        xi_u32 u;
    };

    union f32_u32 close_one = { .u = 0x3f7fffff };
    union f32_u32 min_value = { .u = 114 << 23  };

    if (!(component > min_value.f)) { component = min_value.f; }
    if (component > close_one.f)    { component = close_one.f; }

    union f32_u32 f;
    f.f = component;

    xi_u32 lookup = tbl_linear_f32_to_srgb_u8[(f.u - min_value.u) >> 20];
    xi_u32 bias   = (lookup >> 16) << 9;
    xi_u32 scale  = lookup & 0xffff;

    xi_u32 t = (f.u >> 12) & 0xff;

    result = (xi_u8) ((bias + (scale * t)) >> 16);
    return result;
}
