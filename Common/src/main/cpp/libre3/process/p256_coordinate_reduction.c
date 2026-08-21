#include "p256_coordinate_reduction.h"

#include <stddef.h>
#include <string.h>

static uint32_t carry4_u32(uint32_t a, uint32_t b) {
    return (uint32_t)((uint32_t)(a + b) < a);
}

static uint32_t hi32_u64(uint64_t x) {
    return (uint32_t)(x >> 32);
}

static void pair_add(uint32_t acc[2], const uint32_t v[2]) {
    uint32_t lo = acc[0] + v[0];
    uint32_t c = carry4_u32(acc[0], v[0]);
    acc[0] = lo;
    acc[1] = acc[1] + v[1] + c;
}

static void nibble_ladder_step(const uint32_t table[16][2], uint32_t *lo, uint32_t *hi) {
    uint32_t old_lo = *lo;
    uint32_t old_hi = *hi;
    unsigned nib = old_lo & 0x0fu;
    uint32_t shifted = (old_lo >> 4) | (old_hi << 28);
    uint32_t tlo = table[nib][0];
    uint32_t nlo = shifted + tlo;
    uint32_t nhi = (old_hi >> 4) + table[nib][1] + carry4_u32(shifted, tlo);
    *lo = nlo;
    *hi = nhi;
}

static uint32_t nibble_ladder_run8_low(const uint32_t table[16][2], uint32_t lo, uint32_t hi) {
    for (unsigned i = 0; i < 8u; ++i) {
        nibble_ladder_step(table, &lo, &hi);
    }
    return lo;
}

static const uint32_t k_dat_f41a0278_pairs[16][2] = {
    {0x6358122eu, 0x5bac9230u}, {0x1d6c437bu, 0x2dd55410u},
    {0xfe7900b1u, 0xf6d2129au}, {0xb88d31feu, 0xc8fad47au},
    {0x9999ef34u, 0x91f79305u}, {0x53ae2081u, 0x642054e5u},
    {0x34baddb7u, 0x3d1d1370u}, {0x15c79aedu, 0x0619d1fbu},
    {0xcfdbcc3au, 0xd84293dau}, {0xb0e88970u, 0xa13f5265u},
    {0x6afcbabdu, 0x73681445u}, {0x4c0977f3u, 0x4c64d2d0u},
    {0x061da940u, 0x1e8d94b0u}, {0xe72a6676u, 0xe78a533au},
    {0xc83723acu, 0xb08711c5u}, {0x824b54f9u, 0x82afd3a5u}
};


static const uint32_t k_dat_f4190270[8] = {
    0x753132ddu, 0x36f44785u, 0x5b1efddbu, 0xd01108edu,
    0xa19d2fb7u, 0xb3ca39dfu, 0xa8c507e3u, 0xa5666957u
};

static const uint32_t k_dat_f4190290[8] = {
    0xeb384724u, 0x8987787eu, 0x228259e8u, 0xfdd99ab4u,
    0x2dbec61eu, 0xd44f1e1eu, 0x38cdbf4fu, 0x583e01bau
};

static const uint32_t k_dat_f41a0378_pairs[16][2] = {
    {0xe98e111au, 0x7d3c9501u}, {0xa74a9a9au, 0xe6f61782u},
    {0xa1f0da53u, 0x512ceae5u}, {0x5fad63d3u, 0xcae66d66u},
    {0x5a53a38cu, 0x351d40c9u}, {0x54f9e345u, 0xaf54142cu},
    {0x12b66cc5u, 0x190d96adu}, {0x0d5cac7eu, 0x83446a10u},
    {0xcb1935feu, 0xfcfdec90u}, {0xc5bf75b7u, 0x6734bff3u},
    {0x837bff37u, 0xd0ee4274u}, {0x7e223ef0u, 0x4b2515d7u},
    {0x78c87ea9u, 0xb55be93au}, {0x36850829u, 0x2f156bbbu},
    {0x312b47e2u, 0x994c3f1eu}, {0xeee7d162u, 0x0305c19eu}
};


static const uint32_t k_dat_f41a03f8_pairs[16][2] = {
    {0xdc944c5au, 0xe50d6812u}, {0xf498ed55u, 0x9c242250u},
    {0x0c9d8e50u, 0x433adc8fu}, {0xa4897f66u, 0xf9d8602cu},
    {0xbc8e2061u, 0xa0ef1a6au}, {0xd492c15cu, 0x5805d4a8u},
    {0xec976257u, 0x0f1c8ee6u}, {0x049c0352u, 0xb6334925u},
    {0x1ca0a44du, 0x6d4a0363u}, {0xb48c9563u, 0x13e78700u},
    {0xcc91365eu, 0xcafe413eu}, {0xe495d759u, 0x7214fb7cu},
    {0xfc9a7854u, 0x292bb5bau}, {0x149f194fu, 0xd0426ff9u},
    {0xac8b0a65u, 0x86dff396u}, {0xc48fab60u, 0x3df6add4u}
};

static const uint32_t k_dat_f41a0478_pairs[16][2] = {
    {0xd35925b6u, 0x7da2cc55u},
    {0x8828324bu, 0x6186f1b1u},
    {0xcdfc511bu, 0x50aa4fceu},
    {0x82cb5db0u, 0x448e752au},
    {0x379a6a45u, 0x38729a86u},
    {0xec6976dau, 0x2c56bfe1u},
    {0xa138836fu, 0x103ae53du},
    {0xe70ca23fu, 0x0f5e435au},
    {0x9bdbaed4u, 0xf34268b6u},
    {0x50aabb69u, 0xe7268e12u},
    {0x0579c7feu, 0xdb0ab36eu},
    {0xba48d493u, 0xceeed8c9u},
    {0x6f17e128u, 0xb2d2fe25u},
    {0xb4ebfff8u, 0xa1f65c42u},
    {0x69bb0c8du, 0x95da819eu},
    {0x1e8a1922u, 0x89bea6fau}
};

static const uint32_t k_dat_f41a04f8_pairs[16][2] = {
    {0x156342ceu, 0x953217e3u},
    {0x2ed94d2bu, 0xcd714497u},
    {0xa9449a71u, 0xf1b1e8eeu},
    {0x23afe7b7u, 0x25f28d46u},
    {0x3d25f214u, 0x5e31b9fau},
    {0xb7913f5au, 0x82725e51u},
    {0xd10749b7u, 0xbab18b05u},
    {0x4b7296fdu, 0xeef22f5du},
    {0x64e8a15au, 0x17315c11u},
    {0xdf53eea0u, 0x4b720068u},
    {0x59bf3be6u, 0x7fb2a4c0u},
    {0x73354643u, 0xa7f1d174u},
    {0xeda09389u, 0xdc3275cbu},
    {0x07169de6u, 0x0471a280u},
    {0x8181eb2cu, 0x38b246d7u},
    {0x9af7f589u, 0x60f1738bu}
};


static const uint32_t k_dat_f41a0578_pairs[16][2] = {
    {0x1ce62b82u, 0xa976b3adu},
    {0xba415e79u, 0x51c2d846u},
    {0x2cdf218du, 0x05e8215bu},
    {0xca3a5484u, 0xbe3445f4u},
    {0x6795877bu, 0x66806a8eu},
    {0xda334a8fu, 0x1aa5b3a2u},
    {0x778e7d86u, 0xc2f1d83cu},
    {0x14e9b07du, 0x7b3dfcd6u},
    {0x87877391u, 0x2f6345eau},
    {0x24e2a688u, 0xd7af6a84u},
    {0xc23dd97fu, 0x8ffb8f1du},
    {0x5f990c76u, 0x3847b3b7u},
    {0xd236cf8au, 0xec6cfccbu},
    {0x6f920281u, 0x94b92165u},
    {0x0ced3578u, 0x4d0545ffu},
    {0x7f8af88cu, 0xf12a8f13u}
};

static const uint32_t k_dat_f41902b0[8] = {
    0x25228cc1u, 0x5d629f07u, 0x9d48299bu, 0x97672d3fu,
    0xd002321du, 0xcc1a9131u, 0xc6af939bu, 0x162a929du
};

static const uint32_t k_dat_f41902d0[8] = {
    0x5232f832u, 0xa28de997u, 0xcd626b24u, 0x08db5a75u,
    0xefef2043u, 0x9677caf8u, 0xc1b48a0au, 0x5efe1a29u
};

static const uint32_t k_dat_f41a02f8_pairs[16][2] = {
    {0x882f3655u, 0xe031ee8cu}, {0xfefa8873u, 0xffa988f5u},
    {0x600806feu, 0x0da26874u}, {0xd6d3591cu, 0x1d1a02ddu},
    {0x4d9eab3au, 0x2c919d47u}, {0xaeac29c5u, 0x3a8a7cc5u},
    {0x25777be3u, 0x4a02172fu}, {0x9c42ce01u, 0x5979b198u},
    {0xfd504c8cu, 0x67729116u}, {0x741b9eaau, 0x76ea2b80u},
    {0xeae6f0c8u, 0x8661c5e9u}, {0x4bf46f53u, 0x945aa568u},
    {0xc2bfc171u, 0xa3d23fd1u}, {0x398b138fu, 0xb349da3bu},
    {0x9a98921au, 0xc142b9b9u}, {0x1163e438u, 0xd0ba5423u}
};

static const uint32_t k_unk_f41901f0[8] = {
    0x01444e11u, 0xb36b9457u, 0xe0cda1d9u, 0x96213bf3u,
    0x71f9b01du, 0x6ba8bf53u, 0x0d7bc155u, 0x303285b7u
};

static const uint32_t k_unk_f4190210[8] = {
    0xe5f74bf5u, 0x7dc78934u, 0x835b0efcu, 0xaa3ea153u,
    0x9900c0e6u, 0x00d86cd7u, 0x3375064eu, 0x0681e1b9u
};

static const uint32_t k_unk_f4190230[8] = {
    0xbdc40e1du, 0x3165ea3bu, 0xaca49d5fu, 0x7304c2f5u,
    0x574b76cbu, 0xebcdafffu, 0x35d77d15u, 0x1900dc33u
};

static const uint32_t k_unk_f4190250[8] = {
    0xdbf00442u, 0xcdb30935u, 0xd1e0b3b8u, 0xf7e8d18bu,
    0xade44c67u, 0x34a6dcb1u, 0x1b94a26cu, 0x95ffd0feu
};

static void pair_reduce_left_pair_from_token(uint32_t token, uint32_t out[2]) {
    uint64_t p0 = (uint64_t)token * 0x9009a865u;
    uint32_t p0lo = (uint32_t)p0;
    uint32_t lo = p0lo + 0xd5d2de7du;
    uint32_t hi = token * 0x31cf3440u + hi32_u64(p0) + 0x10c1681fu + (uint32_t)(0x2a2d2182u < p0lo);
    uint32_t ladder_low8 = nibble_ladder_run8_low(k_dat_f41a0278_pairs, lo, hi);

    uint64_t p1 = (uint64_t)token * 0xa203b439u;
    uint32_t p1lo = (uint32_t)p1;
    out[0] = p1lo + 0x2927e4f7u;
    out[1] = token * 0x04afab55u + hi32_u64(p1) + ladder_low8 * 0x038f82bbu +
             0xaf5113a4u + (uint32_t)(0xd6d81b08u < p1lo);
}

static void pair_reduce_right_pair_from_token(uint32_t token, uint32_t out[2]) {
    uint64_t p0 = (uint64_t)token * 0x8bdcf779u;
    uint32_t p0lo = (uint32_t)p0;
    uint32_t lo = p0lo + 0xbdf39e23u;
    uint32_t hi = token * 0x2d66319bu + hi32_u64(p0) + 0x82b71182u + (uint32_t)(0x420c61dcu < p0lo);
    uint32_t ladder_low8 = nibble_ladder_run8_low(k_dat_f41a02f8_pairs, lo, hi);

    uint64_t p1 = (uint64_t)token * 0xd33c5147u;
    uint32_t p1lo = (uint32_t)p1;
    out[0] = p1lo + 0x03ac50ef3u;
    out[1] = token * 0xeb5cffe8u + hi32_u64(p1) + ladder_low8 * 0x22046141u +
             0x6f252c9au + (uint32_t)(0xc53af10cu < p1lo);
}

int l3_p256_coordinate_reduce_seed_prefixes(
    const int32_t left10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS],
    const int32_t right13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS],
    uint32_t left_raw10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS][2],
    uint32_t left_prefix10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS][2],
    uint32_t right_raw13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS][2],
    uint32_t right_prefix13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS][2]) {
    if (!left10 || !right13 || !left_raw10 || !left_prefix10 || !right_raw13 || !right_prefix13) {
        return -1;
    }

    uint32_t acc[2];
    uint32_t token = (uint32_t)left10[0] * 0x7c3435b9u + 0xfaf99c1au;
    pair_reduce_left_pair_from_token(token, left_raw10[0]);
    acc[0] = left_raw10[0][0];
    acc[1] = left_raw10[0][1];
    left_prefix10[0][0] = acc[0];
    left_prefix10[0][1] = acc[1];
    for (unsigned i = 1u; i < L3_AUTH_PAIR_REDUCE_LEFT_WORDS; ++i) {
        unsigned lane = i & 7u;
        uint32_t inner = (uint32_t)left10[i] * k_unk_f41901f0[lane] + k_unk_f4190210[lane];
        token = inner * 0x03076529u + 0xa2b3c8ddu;
        pair_reduce_left_pair_from_token(token, left_raw10[i]);
        pair_add(acc, left_raw10[i]);
        left_prefix10[i][0] = acc[0];
        left_prefix10[i][1] = acc[1];
    }

    token = (uint32_t)right13[0] * 0xf5887e63u + 0x892c4847u;
    pair_reduce_right_pair_from_token(token, right_raw13[0]);
    acc[0] = right_raw13[0][0];
    acc[1] = right_raw13[0][1];
    right_prefix13[0][0] = acc[0];
    right_prefix13[0][1] = acc[1];
    for (unsigned i = 1u; i < L3_AUTH_PAIR_REDUCE_RIGHT_WORDS; ++i) {
        unsigned lane = i & 7u;
        uint32_t inner = (uint32_t)right13[i] * k_unk_f4190230[lane] + k_unk_f4190250[lane];
        token = inner * 0x771b167fu + 0xabc67f89u;
        pair_reduce_right_pair_from_token(token, right_raw13[i]);
        pair_add(acc, right_raw13[i]);
        right_prefix13[i][0] = acc[0];
        right_prefix13[i][1] = acc[1];
    }
    return 0;
}

static void pair_sub(uint32_t out[2], const uint32_t a[2], const uint32_t b[2]) {
    uint32_t borrow = (uint32_t)(a[0] < b[0]);
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1] - borrow;
}

static void pair_range_prefix(uint32_t out[2], const uint32_t prefix[][2], unsigned lo, unsigned hi) {
    if (lo == 0u) {
        out[0] = prefix[hi][0];
        out[1] = prefix[hi][1];
    } else {
        pair_sub(out, prefix[hi], prefix[lo - 1u]);
    }
}

int l3_p256_coordinate_reduce_convolution_pairs26(
    const uint32_t left_raw10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS][2],
    const uint32_t left_prefix10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS][2],
    const uint32_t right_raw13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS][2],
    const uint32_t right_prefix13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_PAIR_REDUCE_CONV_WORDS][2]) {
    if (!left_raw10 || !left_prefix10 || !right_raw13 || !right_prefix13 || !out_pairs26) {
        return -1;
    }

    for (unsigned k = 0u; k < L3_AUTH_PAIR_REDUCE_CONV_WORDS; ++k) {
        uint32_t native_lo = 0x176126b4u;
        uint32_t native_hi = 0xe55feaa6u; /* -0x1aa0155a */

        unsigned lo = (k <= 12u) ? 0u : (k - 12u);
        unsigned hi = (k < 9u) ? k : 9u;
        if (lo <= hi) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned j = lo; j <= hi; ++j) {
                unsigned r = k - j;
                uint64_t prod = (uint64_t)right_raw13[r][0] * (uint64_t)left_raw10[j][0];
                uint32_t plo = (uint32_t)prod;
                uint32_t carry = carry4_u32(conv_lo, plo);
                conv_lo += plo;
                conv_hi += right_raw13[r][1] * left_raw10[j][0] +
                           right_raw13[r][0] * left_raw10[j][1] +
                           hi32_u64(prod) + carry;
            }

            uint32_t left_sum[2];
            uint32_t right_sum[2];
            pair_range_prefix(left_sum, left_prefix10, lo, hi);
            pair_range_prefix(right_sum, right_prefix13, k - hi, k - lo);

            uint32_t count = (hi - lo) + 1u;
            uint64_t p_count = (uint64_t)count * 0x0b157820u;
            uint32_t p_count_lo = (uint32_t)p_count;
            uint32_t term_count_lo = p_count_lo + 0x176126b4u;

            uint64_t p_conv = (uint64_t)conv_lo * 0x40fdd0c3u;
            uint32_t p_conv_lo = (uint32_t)p_conv;
            uint32_t term_conv_lo = term_count_lo + p_conv_lo;

            uint64_t p_left = (uint64_t)left_sum[0] * 0x5f986de8u;
            uint32_t p_left_lo = (uint32_t)p_left;
            uint32_t term_left_lo = term_conv_lo + p_left_lo;

            uint64_t p_right = (uint64_t)right_sum[0] * 0x98bcd2fcu;
            uint32_t p_right_lo = (uint32_t)p_right;
            native_lo = term_left_lo + p_right_lo;
            native_hi = count * 0x90d61d66u + hi32_u64(p_count) + 0xe55feaa6u +
                        (uint32_t)(0xe89ed94bu < p_count_lo) +
                        conv_hi * 0x40fdd0c3u + conv_lo * 0xc5f9834au + hi32_u64(p_conv) +
                        carry4_u32(term_count_lo, p_conv_lo) +
                        left_sum[1] * 0x5f986de8u + left_sum[0] * 0x8376615cu + hi32_u64(p_left) +
                        carry4_u32(term_conv_lo, p_left_lo) +
                        right_sum[1] * 0x98bcd2fcu + right_sum[0] * 0x945d068eu + hi32_u64(p_right) +
                        carry4_u32(term_left_lo, p_right_lo);
        }

        uint64_t p_final = (uint64_t)native_lo * 0x02c37ed9u;
        uint32_t p_final_lo = (uint32_t)p_final;
        out_pairs26[k][0] = p_final_lo + 0x16cd3ae9u;
        out_pairs26[k][1] = native_hi * 0x02c37ed9u + native_lo * 0xc0ca4c7eu +
                            hi32_u64(p_final) + 0x49c6b6b5u +
                            (uint32_t)(0xe932c516u < p_final_lo);
    }
    return 0;
}


static void pair_reduce_param3_pair_from_token(uint32_t token, uint32_t out[2]) {
    uint64_t p0 = (uint64_t)token * 0xf2d1874fu;
    uint32_t p0lo = (uint32_t)p0;
    uint32_t lo = p0lo + 0xc5458340u;
    uint32_t hi = token * 0xe11a8990u + hi32_u64(p0) + 0xf42d4588u +
                  (uint32_t)(0x3aba7cbfu < p0lo);
    uint32_t ladder_low8 = nibble_ladder_run8_low(k_dat_f41a0378_pairs, lo, hi);

    uint64_t p1 = (uint64_t)token * 0x55338a3bu;
    uint32_t p1lo = (uint32_t)p1;
    out[0] = p1lo + 0x483987adu;
    out[1] = token * 0x5a1fc497u + hi32_u64(p1) +
             ladder_low8 * 0x53b2acabu + 0x4300a1fdu +
             (uint32_t)(0xb7c67852u < p1lo);
}

int l3_p256_coordinate_reduce_param3_pairs13(
    const int32_t param3[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS],
    uint32_t out_pairs13[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS][2]) {
    if (!param3 || !out_pairs13) {
        return -1;
    }
    for (unsigned i = 0u; i < L3_AUTH_PAIR_REDUCE_PARAM3_WORDS; ++i) {
        unsigned lane = i & 7u;
        uint32_t token = ((uint32_t)param3[i] * k_dat_f4190270[lane] + k_dat_f4190290[lane]) *
                         0x154ecc93u + 0xfa8351b8u;
        pair_reduce_param3_pair_from_token(token, out_pairs13[i]);
    }
    return 0;
}


static void nibble_ladder_run_n(const uint32_t table[16][2], uint32_t *lo, uint32_t *hi, unsigned n) {
    for (unsigned i = 0; i < n; ++i) {
        nibble_ladder_step(table, lo, hi);
    }
}

int l3_p256_coordinate_reduce_param3_scalar_ladder_core(
    const uint32_t local_pair[2],
    uint32_t mid_pair[2],
    uint32_t low_ladder_out[2]) {
    if (!local_pair || !mid_pair || !low_ladder_out) {
        return -1;
    }

    /* Native block after the first param_3/vector-lane accumulation in
     * FUN_f3fc2304. The decompiler names the input pair uVar22/iVar14.
     */
    uint64_t p0 = (uint64_t)local_pair[0] * 0xeb4891f7u;
    uint32_t p0lo = (uint32_t)p0;
    uint32_t lo = p0lo + 0xa54387ecu;
    uint32_t hi = local_pair[1] * 0xeb4891f7u + local_pair[0] * 0xafb01dccu +
                  hi32_u64(p0) + 0x6c74a7cfu + (uint32_t)(0x5abc7813u < p0lo);

    nibble_ladder_run_n(k_dat_f41a0478_pairs, &lo, &hi, 7u);

    uint64_t p1 = (uint64_t)lo * 0xe6f33fc7u;
    uint32_t p1lo = (uint32_t)p1;
    uint32_t mid_lo = p1lo + 0x01de12f3u;
    uint32_t mid_hi = hi * 0xe6f33fc7u + lo * 0xddf6bf95u + hi32_u64(p1) +
                      0x630ad558u + (uint32_t)(0xfe21ed0cu < p1lo);
    mid_pair[0] = mid_lo;
    mid_pair[1] = mid_hi;

    uint64_t p2 = (uint64_t)mid_lo * 0xd3dd6c83u;
    uint32_t p2lo = (uint32_t)p2;
    lo = p2lo + 0x3d9c7d1du;
    hi = mid_hi * 0xd3dd6c83u + mid_lo * 0xb7b19ae7u +
         hi32_u64(p2) + 0x300be744u + (uint32_t)(0xc26382e2u < p2lo);

    nibble_ladder_run_n(k_dat_f41a04f8_pairs, &lo, &hi, 7u);

    uint32_t shifted = (lo >> 4) | (hi << 28);
    unsigned nib = lo & 0x0fu;
    uint32_t final_lo = shifted + k_dat_f41a04f8_pairs[nib][0];
    /* Native only consumes this low table word in the following scalar term. */
    low_ladder_out[0] = final_lo;
    low_ladder_out[1] = k_dat_f41a04f8_pairs[final_lo & 0x0fu][0];
    return 0;
}

int l3_p256_coordinate_reduce_param56_reduction_seed_core(
    uint32_t param5,
    uint32_t param6,
    const uint32_t local_pair[2],
    uint32_t base_pair[2],
    uint32_t factor_pair[2],
    uint32_t add_pair[2]) {
    if (!local_pair || !base_pair || !factor_pair || !add_pair) {
        return -1;
    }

    /* Native FUN_f3fc2304 block immediately after local_270[0] has been
     * produced and before the param_3/vector-lane accumulation loop. */
    uint64_t p5a = (uint64_t)param5 * 0x54d576cfu;
    uint32_t p5a_lo = (uint32_t)p5a;
    uint64_t p5b = (uint64_t)param5 * 0x815394c5u;
    uint32_t p5b_lo = (uint32_t)p5b;

    uint32_t seed_lo = p5a_lo + 0xd75eed53u;
    uint32_t seed_hi = param6 * 0x54d576cfu + param5 * 0x369a90fdu +
                       hi32_u64(p5a) + 0xe954e1a4u +
                       (uint32_t)(0x28a112acu < p5a_lo);
    uint32_t seed2_lo = p5b_lo + 0x485e2ad2u;
    uint32_t seed2_hi = param6 * 0x815394c5u + param5 * 0xa77c4781u +
                        hi32_u64(p5b) + 0xe022c65bu +
                        (uint32_t)(0xb7a1d52du < p5b_lo);

    uint64_t prod_local = (uint64_t)seed2_lo * local_pair[0];
    uint32_t prod_local_lo = (uint32_t)prod_local;
    uint32_t base_lo = seed_lo + prod_local_lo;
    uint32_t base_hi = seed_hi + local_pair[1] * seed2_lo +
                       local_pair[0] * seed2_hi + hi32_u64(prod_local) +
                       carry4_u32(seed_lo, prod_local_lo);
    base_pair[0] = base_lo;
    base_pair[1] = base_hi;

    uint64_t p0 = (uint64_t)base_lo * 0xa3eaf46du;
    uint32_t p0lo = (uint32_t)p0;
    uint32_t lo = p0lo + 0xdb0b19bau;
    uint32_t hi = base_hi * 0xa3eaf46du + base_lo * 0xfbe838ecu +
                  hi32_u64(p0) + 0x190bb141u +
                  (uint32_t)(0x24f4e645u < p0lo);

    uint32_t pre7_hi = 0u;
    uint32_t pre7_shifted = 0u;
    uint32_t pre7_tlo = 0u;
    unsigned pre7_nib = 0u;
    for (unsigned i = 0u; i < 7u; ++i) {
        uint32_t old_lo = lo;
        uint32_t old_hi = hi;
        unsigned nib = old_lo & 0x0fu;
        uint32_t shifted = (old_lo >> 4) | (old_hi << 28);
        uint32_t tlo = k_dat_f41a03f8_pairs[nib][0];
        if (i == 6u) {
            pre7_hi = old_hi;
            pre7_shifted = shifted;
            pre7_tlo = tlo;
            pre7_nib = nib;
        }
        lo = shifted + tlo;
        hi = (old_hi >> 4) + k_dat_f41a03f8_pairs[nib][1] + carry4_u32(shifted, tlo);
    }
    uint32_t ladder_low7 = lo;
    uint32_t ladder_high7_for_term = (pre7_hi >> 4) + k_dat_f41a03f8_pairs[pre7_nib][1] +
                                     carry4_u32(pre7_shifted, pre7_tlo);

    uint64_t p_ladder = (uint64_t)ladder_low7 * 0xd0000000u;
    uint32_t p_ladder_lo = (uint32_t)p_ladder;
    uint64_t p_base2 = (uint64_t)base_lo * 0xff2213c7u;
    uint32_t p_base2_lo = (uint32_t)p_base2;
    uint32_t sum_lo = p_base2_lo + p_ladder_lo;
    uint32_t affine_lo = sum_lo + 0xf4a6c23cu;

    uint32_t factor_pre_hi = base_hi * 0xff2213c7u + base_lo * 0xb444693du +
                             hi32_u64(p_base2) + ladder_low7 * 0xc60dcff7u +
                             hi32_u64(p_ladder) + ladder_high7_for_term * 0xd0000000u +
                             carry4_u32(p_base2_lo, p_ladder_lo) + 0xaa700f07u +
                             (uint32_t)(0x0b593dc3u < sum_lo);

    uint64_t p_factor = (uint64_t)affine_lo * 0x8876e91fu;
    uint32_t p_factor_lo = (uint32_t)p_factor;
    factor_pair[0] = p_factor_lo + 0x5e7a4627u;
    factor_pair[1] = factor_pre_hi * 0x8876e91fu + affine_lo * 0xf58241acu +
                     hi32_u64(p_factor) + 0x4e734185u +
                     (uint32_t)(0xa185b9d8u < p_factor_lo);

    uint64_t p_add = (uint64_t)affine_lo * 0x23ef02ecu;
    uint32_t p_add_lo = (uint32_t)p_add;
    add_pair[0] = p_add_lo + 0x2310e38cu;
    add_pair[1] = factor_pre_hi * 0x23ef02ecu + affine_lo * 0x878d92f7u +
                  hi32_u64(p_add) + 0x06022adau +
                  (uint32_t)(0xdcef1c73u < p_add_lo);
    return 0;
}


int l3_p256_coordinate_reduce_reduction_lane_store_core(
    const uint32_t rolling_pair[2],
    const uint32_t aux238_pair[2],
    const uint32_t state_pair[2],
    const uint32_t coeff_local_pair[2],
    const uint32_t coeff_aux_pair[2],
    const uint32_t coeff_state_pair[2],
    const uint32_t factor_pair[2],
    const uint32_t add_pair[2],
    uint32_t out_local_pair[2],
    uint32_t out_aux238_pair[2],
    uint32_t out_state_pair[2]) {
    if (!rolling_pair || !aux238_pair || !state_pair || !coeff_local_pair ||
        !coeff_aux_pair || !coeff_state_pair || !factor_pair || !add_pair ||
        !out_local_pair || !out_aux238_pair || !out_state_pair) {
        return -1;
    }

    const uint32_t factor_lo = factor_pair[0];
    const uint32_t factor_hi = factor_pair[1];
    const uint32_t add_lo = add_pair[0];
    const uint32_t add_hi = add_pair[1];

    /* Native lines equivalent to:
     *   local_270[i]   = add + rolling_local0 + coeff_local * factor
     *   auStack_238[i] = add + aux238[i]       + coeff_aux   * factor
     *   auStack_210[i] = add + state[i]        + coeff_state * factor
     * with full 64-bit low/high carry propagation.
     */
    uint64_t p_local = (uint64_t)coeff_local_pair[0] * factor_lo;
    uint32_t p_local_lo = (uint32_t)p_local;
    uint32_t sum_local0 = add_lo + rolling_pair[0];
    out_local_pair[0] = sum_local0 + p_local_lo;
    out_local_pair[1] = add_hi + rolling_pair[1] + carry4_u32(add_lo, rolling_pair[0]) +
                        factor_hi * coeff_local_pair[0] +
                        factor_lo * coeff_local_pair[1] +
                        hi32_u64(p_local) +
                        carry4_u32(sum_local0, p_local_lo);

    uint64_t p_aux = (uint64_t)coeff_aux_pair[0] * factor_lo;
    uint32_t p_aux_lo = (uint32_t)p_aux;
    uint32_t sum_aux0 = add_lo + aux238_pair[0];
    out_aux238_pair[0] = sum_aux0 + p_aux_lo;
    out_aux238_pair[1] = add_hi + aux238_pair[1] + carry4_u32(add_lo, aux238_pair[0]) +
                         factor_hi * coeff_aux_pair[0] +
                         factor_lo * coeff_aux_pair[1] +
                         hi32_u64(p_aux) +
                         carry4_u32(sum_aux0, p_aux_lo);

    uint64_t p_state = (uint64_t)coeff_state_pair[0] * factor_lo;
    uint32_t p_state_lo = (uint32_t)p_state;
    uint32_t sum_state0 = p_state_lo + add_lo;
    out_state_pair[0] = sum_state0 + state_pair[0];
    out_state_pair[1] = factor_hi * coeff_state_pair[0] +
                        factor_lo * coeff_state_pair[1] +
                        hi32_u64(p_state) + add_hi +
                        carry4_u32(p_state_lo, add_lo) +
                        state_pair[1] +
                        carry4_u32(sum_state0, state_pair[0]);
    return 0;
}


int l3_p256_coordinate_reduce_next_rolling_pair_core(
    const uint32_t mid_pair[2],
    const uint32_t low_ladder_out[2],
    const uint32_t lane_acc_pair[2],
    uint32_t out_next_pair[2]) {
    if (!mid_pair || !low_ladder_out || !lane_acc_pair || !out_next_pair) {
        return -1;
    }

    /* Native FUN_f3fc2304 block immediately after the DAT_f41a04f8 scalar
     * ladder and the auVar32 vector-lane accumulator.  The decompiler names
     * are: uVar26/iVar25 for mid_pair, uVar9/iVar14 for low_ladder_out, and
     * auVar32._0_8_ for lane_acc_pair.
     */
    uint64_t p_mid = (uint64_t)mid_pair[0] * 0xddcf6b51u;
    uint32_t p_mid_lo = (uint32_t)p_mid;

    uint64_t p_roll = (uint64_t)p_mid_lo * 0xd7867db1u;
    uint32_t p_roll_lo = (uint32_t)p_roll;
    uint32_t sum0 = p_roll_lo + lane_acc_pair[0];
    out_next_pair[0] = sum0 + 0x9fe6ca94u;

    uint32_t low_feedback = (low_ladder_out[0] >> 4) + low_ladder_out[1];
    uint32_t pre = mid_pair[1] * 0xddcf6b51u +
                   mid_pair[0] * 0x26f0b700u +
                   hi32_u64(p_mid) +
                   low_feedback * 0xa8717650u;

    out_next_pair[1] = pre * 0xd7867db1u +
                       p_mid_lo * 0x4fe5b143u +
                       hi32_u64(p_roll) +
                       lane_acc_pair[1] +
                       carry4_u32(p_roll_lo, lane_acc_pair[0]) +
                       0xb1d1416fu +
                       (uint32_t)(0x6019356bu < sum0);
    return 0;
}


static int pair_reduce_output_words13_impl(
    const uint32_t state_pairs13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS][2],
    uint32_t out_words13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS],
    uint32_t out_saved_ladder_trace[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS][2],
    uint32_t out_feedback_trace[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS],
    uint32_t out_state_trace[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS][2]) {
    if (!state_pairs13 || !out_words13) {
        return -1;
    }

    uint32_t lo_state = 0x4dcc2007u;
    uint32_t hi_state = 0xc3223cd3u; /* -0x3cddc32d */

    for (unsigned lane_index = 0u; lane_index < L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS; ++lane_index) {
        const uint32_t pair_lo = state_pairs13[lane_index][0];
        const uint32_t pair_hi = state_pairs13[lane_index][1];

        uint64_t p_pair = (uint64_t)pair_lo * 0xb0a1864bu;
        uint32_t p_pair_lo = (uint32_t)p_pair;
        uint64_t p_state = (uint64_t)lo_state * 0xfa55455fu;
        uint32_t p_state_lo = (uint32_t)p_state;
        uint32_t sum0 = p_state_lo + p_pair_lo;
        uint32_t base_lo = sum0 + 0x7c40ee69u;

        uint64_t p_base = (uint64_t)base_lo * 0x967a8423u;
        uint32_t p_base_lo = (uint32_t)p_base;
        uint32_t ladder_lo = p_base_lo + 0xb2592175u;
        uint32_t ladder_hi = (hi_state * 0xfa55455fu + lo_state * 0xd0bf6bd5u + hi32_u64(p_state) +
                              pair_hi * 0xb0a1864bu + pair_lo * 0xd3143f23u + hi32_u64(p_pair) +
                              carry4_u32(p_state_lo, p_pair_lo) + 0x695d3948u +
                              (uint32_t)(0x83bf1196u < sum0)) * 0x967a8423u +
                             base_lo * 0x1e284982u + hi32_u64(p_base) + 0xd6b5468au +
                             (uint32_t)(0x4da6de8au < p_base_lo);

        /* Native keeps the state after the 7th DAT_f41a0578 step for the
         * output affine and for the rolling state update, then continues eight
         * complete steps plus one final low-half shifted add for feedback.
         */
        nibble_ladder_run_n(k_dat_f41a0578_pairs, &ladder_lo, &ladder_hi, 7u);
        uint32_t saved_ladder_lo = ladder_lo;
        uint32_t saved_ladder_hi = ladder_hi;
        if (out_saved_ladder_trace) {
            out_saved_ladder_trace[lane_index][0] = saved_ladder_lo;
            out_saved_ladder_trace[lane_index][1] = saved_ladder_hi;
        }

        uint64_t p_roll = (uint64_t)saved_ladder_lo * 0x020f9d55u;
        uint32_t p_roll_lo = (uint32_t)p_roll;

        nibble_ladder_run_n(k_dat_f41a0578_pairs, &ladder_lo, &ladder_hi, 8u);
        uint32_t shifted = (ladder_lo >> 4) | (ladder_hi << 28);
        unsigned nib = ladder_lo & 0x0fu;
        uint32_t feedback_lo = shifted + k_dat_f41a0578_pairs[nib][0];
        if (out_feedback_trace) {
            out_feedback_trace[lane_index] = feedback_lo;
        }

        unsigned out_lane = lane_index & 7u;
        out_words13[lane_index] = (base_lo * 0xa00df5c5u +
                                   saved_ladder_lo * 0x90000000u +
                                   0x0a1156c6u) * k_dat_f41902b0[out_lane] +
                                  k_dat_f41902d0[out_lane];

        lo_state = p_roll_lo + 0x22cb9a2fu;
        hi_state = saved_ladder_hi * 0x020f9d55u +
                   saved_ladder_lo * 0x139f0e23u + hi32_u64(p_roll) +
                   feedback_lo * 0xdf062ab0u +
                   0x0041cdf8u + (uint32_t)(0xdd3465d0u < p_roll_lo);
        if (out_state_trace) {
            out_state_trace[lane_index][0] = lo_state;
            out_state_trace[lane_index][1] = hi_state;
        }
    }
    return 0;
}

int l3_p256_coordinate_reduce_output_words13_from_state_pairs13(
    const uint32_t state_pairs13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS][2],
    uint32_t out_words13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS]) {
    return pair_reduce_output_words13_impl(
        state_pairs13, out_words13, NULL, NULL, NULL);
}









static uint64_t pair_reduce_words_to_u64(const uint32_t pair[2]) {
    return ((uint64_t)pair[1] << 32) | (uint64_t)pair[0];
}


static void pair_reduce_u64_to_words(uint64_t x, uint32_t pair[2]) {
    pair[0] = (uint32_t)x;
    pair[1] = (uint32_t)(x >> 32);
}


static uint64_t pair_reduce_pair_to_u64(const uint32_t pair[2]) {
    return ((uint64_t)pair[1] << 32) | (uint64_t)pair[0];
}

static int pair_reduce_reduction_run13_native_impl(
    uint32_t param5,
    uint32_t param6,
    const uint32_t conv_pairs26[L3_AUTH_PAIR_REDUCE_CONV_WORDS][2],
    const uint32_t retained_prefix_pairs23[23][2],
    const uint32_t param3_pairs13[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS][2],
    uint32_t out_state_pairs13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS][2],
    uint32_t out_next_rolling_pair[2],
    uint32_t out_stack_pairs[L3_AUTH_PAIR_REDUCE_REDUCTION_STACK_PAIRS][2],
    uint32_t (*out_trace)[L3_AUTH_PAIR_REDUCE_REDUCTION_STACK_PAIRS][2]) {
    if (!conv_pairs26 || !param3_pairs13 || !out_state_pairs13 ||
        !out_next_rolling_pair || !out_stack_pairs) {
        return -1;
    }

    uint32_t stack[L3_AUTH_PAIR_REDUCE_REDUCTION_STACK_PAIRS][2];
    memset(stack, 0, sizeof stack);
    for (unsigned i = 0; i < L3_AUTH_PAIR_REDUCE_CONV_WORDS; ++i) {
        stack[i][0] = conv_pairs26[i][0];
        stack[i][1] = conv_pairs26[i][1];
    }
    if (retained_prefix_pairs23) {
        for (unsigned i = 0; i < 23u; ++i) {
            stack[26u + i][0] = retained_prefix_pairs23[i][0];
            stack[26u + i][1] = retained_prefix_pairs23[i][1];
        }
    }
    for (unsigned i = 0; i < L3_AUTH_PAIR_REDUCE_PARAM3_WORDS; ++i) {
        stack[49u + i][0] = param3_pairs13[i][0];
        stack[49u + i][1] = param3_pairs13[i][1];
    }

    uint32_t rolling[2] = {stack[0][0], stack[0][1]};
    for (unsigned lane = 0; lane < L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS; ++lane) {
        uint64_t stack268_window[2] = {
            pair_reduce_words_to_u64(stack[1u + lane]),
            pair_reduce_words_to_u64(stack[2u + lane])
        };
        uint64_t stack258_window[2] = {
            pair_reduce_words_to_u64(stack[3u + lane]),
            pair_reduce_words_to_u64(stack[4u + lane])
        };
        uint64_t stack248_window[2] = {
            pair_reduce_words_to_u64(stack[5u + lane]),
            pair_reduce_words_to_u64(stack[6u + lane])
        };
        uint64_t stack230_window[2] = {
            pair_reduce_words_to_u64(stack[8u + lane]),
            pair_reduce_words_to_u64(stack[9u + lane])
        };
        uint64_t stack220_window[2] = {
            pair_reduce_words_to_u64(stack[10u + lane]),
            pair_reduce_words_to_u64(stack[11u + lane])
        };

        uint32_t base[2], factor[2], add[2];
        uint32_t out_local[2], out_aux[2], out_state[2];
        uint32_t mid[2], low_ladder[2], lane_acc[2], next_rolling[2];
        if (l3_p256_coordinate_reduce_param56_reduction_seed_core(
                param5, param6, rolling, base, factor, add) != 0) {
            return -1;
        }
        if (l3_p256_coordinate_reduce_reduction_lane_store_core(
                rolling, stack[7u + lane], stack[12u + lane],
                stack[49u], stack[56u], stack[61u], factor, add,
                out_local, out_aux, out_state) != 0) {
            return -1;
        }

        /* The listing loads q12/q11/q10 from coefficient pairs 50..55.
         * Each adjacent pair is one 64-bit NEON lane.  q13 is the add pair
         * duplicated; q14 is the two q12 coefficient lanes multiplied by
         * the current factor.  Expressing the vector adds directly avoids
         * the decompiler's spurious cross-iteration register inputs. */
        const uint64_t add64 = pair_reduce_pair_to_u64(add);
        const uint64_t factor64 = pair_reduce_pair_to_u64(factor);
        uint64_t out268[2], out258[2], out248[2];
        out268[0] = stack268_window[0] + add64 +
                    pair_reduce_pair_to_u64(stack[50u]) * factor64;
        out268[1] = stack268_window[1] + add64 +
                    pair_reduce_pair_to_u64(stack[51u]) * factor64;
        out258[0] = stack258_window[0] + add64 +
                    pair_reduce_pair_to_u64(stack[52u]) * factor64;
        out258[1] = stack258_window[1] + add64 +
                    pair_reduce_pair_to_u64(stack[53u]) * factor64;
        out248[0] = stack248_window[0] + add64 +
                    pair_reduce_pair_to_u64(stack[54u]) * factor64;
        out248[1] = stack248_window[1] + add64 +
                    pair_reduce_pair_to_u64(stack[55u]) * factor64;
        pair_reduce_u64_to_words(out268[0], lane_acc);

        if (l3_p256_coordinate_reduce_param3_scalar_ladder_core(
                out_local, mid, low_ladder) != 0 ||
            l3_p256_coordinate_reduce_next_rolling_pair_core(
                mid, low_ladder, lane_acc, next_rolling) != 0) {
            return -1;
        }

        uint64_t out230[2], out220[2];
        out230[0] = stack230_window[0] + add64 +
                    pair_reduce_pair_to_u64(stack[57u]) * factor64;
        out230[1] = stack230_window[1] + add64 +
                    pair_reduce_pair_to_u64(stack[58u]) * factor64;
        out220[0] = stack220_window[0] + add64 +
                    pair_reduce_pair_to_u64(stack[59u]) * factor64;
        out220[1] = stack220_window[1] + add64 +
                    pair_reduce_pair_to_u64(stack[60u]) * factor64;

        stack[lane][0] = out_local[0];
        stack[lane][1] = out_local[1];
        stack[7u + lane][0] = out_aux[0];
        stack[7u + lane][1] = out_aux[1];
        stack[12u + lane][0] = out_state[0];
        stack[12u + lane][1] = out_state[1];

        /* Native overwrites the low q14/auStack_268 lane with the next
         * rolling scalar pair after preserving the high lane. */
        stack[1u + lane][0] = next_rolling[0];
        stack[1u + lane][1] = next_rolling[1];
        pair_reduce_u64_to_words(out268[1], stack[2u + lane]);
        pair_reduce_u64_to_words(out258[0], stack[3u + lane]);
        pair_reduce_u64_to_words(out258[1], stack[4u + lane]);
        pair_reduce_u64_to_words(out248[0], stack[5u + lane]);
        pair_reduce_u64_to_words(out248[1], stack[6u + lane]);
        pair_reduce_u64_to_words(out230[0], stack[8u + lane]);
        pair_reduce_u64_to_words(out230[1], stack[9u + lane]);
        pair_reduce_u64_to_words(out220[0], stack[10u + lane]);
        pair_reduce_u64_to_words(out220[1], stack[11u + lane]);

        rolling[0] = next_rolling[0];
        rolling[1] = next_rolling[1];
        if (out_trace) {
            memcpy(out_trace[lane], stack, sizeof stack);
        }
    }

    for (unsigned i = 0; i < L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS; ++i) {
        out_state_pairs13[i][0] = stack[13u + i][0];
        out_state_pairs13[i][1] = stack[13u + i][1];
    }
    out_next_rolling_pair[0] = rolling[0];
    out_next_rolling_pair[1] = rolling[1];
    memcpy(out_stack_pairs, stack, sizeof stack);
    return 0;
}


int l3_p256_coordinate_reduce_reduction_run13_native_seeded(
    uint32_t param5,
    uint32_t param6,
    const uint32_t conv_pairs26[L3_AUTH_PAIR_REDUCE_CONV_WORDS][2],
    const uint32_t retained_prefix_pairs23[23][2],
    const uint32_t param3_pairs13[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS][2],
    uint32_t out_state_pairs13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS][2],
    uint32_t out_next_rolling_pair[2],
    uint32_t out_stack_pairs[L3_AUTH_PAIR_REDUCE_REDUCTION_STACK_PAIRS][2]) {
    if (!retained_prefix_pairs23) return -1;
    return pair_reduce_reduction_run13_native_impl(
        param5, param6, conv_pairs26, retained_prefix_pairs23, param3_pairs13,
        out_state_pairs13, out_next_rolling_pair, out_stack_pairs, NULL);
}



int l3_p256_coordinate_reduce_full_words13_native(
    const int32_t left10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS],
    const int32_t right13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS],
    const int32_t param3[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS],
    uint32_t param5,
    uint32_t param6,
    uint32_t out_words13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS]) {
    if (!left10 || !right13 || !param3 || !out_words13) {
        return -1;
    }

    uint32_t conv[L3_AUTH_PAIR_REDUCE_CONV_WORDS][2];
    uint32_t left_raw[L3_AUTH_PAIR_REDUCE_LEFT_WORDS][2];
    uint32_t left_prefix[L3_AUTH_PAIR_REDUCE_LEFT_WORDS][2];
    uint32_t right_raw[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS][2];
    uint32_t right_prefix[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS][2];
    uint32_t retained_prefix[23][2];
    uint32_t p3_pairs[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS][2];
    uint32_t state[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS][2];
    uint32_t next_rolling[2];
    uint32_t stack[L3_AUTH_PAIR_REDUCE_REDUCTION_STACK_PAIRS][2];
    if (l3_p256_coordinate_reduce_seed_prefixes(
            left10, right13,
            left_raw, left_prefix, right_raw, right_prefix) != 0 ||
        l3_p256_coordinate_reduce_convolution_pairs26(
            left_raw, left_prefix, right_raw, right_prefix, conv) != 0 ||
        l3_p256_coordinate_reduce_param3_pairs13(param3, p3_pairs) != 0 ||
        (memcpy(retained_prefix, right_prefix, sizeof right_prefix),
         memcpy(retained_prefix + 13, left_prefix, sizeof left_prefix),
         l3_p256_coordinate_reduce_reduction_run13_native_seeded(
             param5, param6, conv, retained_prefix, p3_pairs,
             state, next_rolling, stack)) != 0) {
        return -1;
    }
    return l3_p256_coordinate_reduce_output_words13_from_state_pairs13(state, out_words13);
}


