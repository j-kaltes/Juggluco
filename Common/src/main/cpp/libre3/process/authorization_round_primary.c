/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Tue Aug 11 16:33:40 CEST 2026                                                */
#include "authorization_round_primary.h"

#include <string.h>

#include <stddef.h>

static uint32_t add_carry_u32(uint32_t a, uint32_t b) {
    return (uint32_t)((uint32_t)(a + b) < a);
}

static uint32_t multiply_low_u32(uint32_t a, uint32_t b) {
    return (uint32_t)((uint64_t)a * (uint64_t)b);
}

static uint32_t multiply_high_u32(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a * (uint64_t)b) >> 32);
}

static void pair64_add(uint32_t acc[2], const uint32_t v[2]) {
    uint32_t lo = acc[0] + v[0];
    uint32_t c = add_carry_u32(acc[0], v[0]);
    acc[0] = lo;
    acc[1] = acc[1] + v[1] + c;
}

static uint32_t ladder32_run7_then_next(const uint32_t tab[16], uint32_t *x_inout) {
    uint32_t x = *x_inout;
    for (unsigned i = 0; i < 7u; ++i) {
        x = tab[x & 0x0fu] + (x >> 4);
    }
    *x_inout = x;
    return tab[x & 0x0fu] + (x >> 4);
}

static uint32_t nibble_ladder64_run8_low(const uint32_t tab[16][2], uint32_t lo, uint32_t hi) {
    for (unsigned i = 0; i < 8u; ++i) {
        unsigned nib = lo & 0x0fu;
        uint32_t shifted = (lo >> 4) | (hi << 28);
        uint32_t tlo = tab[nib][0];
        uint32_t nlo = shifted + tlo;
        uint32_t nhi = (hi >> 4) + tab[nib][1] + add_carry_u32(shifted, tlo);
        lo = nlo;
        hi = nhi;
    }
    return lo;
}

static const uint32_t k_f4190310[8] = {
    0x167b401bu, 0xf16ffbbfu, 0x5ad44907u, 0xe9b8ff5fu,
    0xd7835b37u, 0x97582f03u, 0x520b17f5u, 0xfd2e9523u
};
static const uint32_t k_f4190330[8] = {
    0x13356eeeu, 0xb04268ecu, 0xef8a6744u, 0xc0b05c7du,
    0x453902bfu, 0xc59db16bu, 0x9ba8ce93u, 0x6936914cu
};
static const uint32_t k_f4190350[8] = {
    0xc54045e3u, 0x7b25bbb7u, 0xe85ee96du, 0x0338bb63u,
    0xaaad4e23u, 0xf297a187u, 0x31a4b15bu, 0x9f8a9fefu
};
static const uint32_t k_f4190370[8] = {
    0xd0d0c898u, 0x9af51046u, 0x4e541cfcu, 0xcd7d2862u,
    0x8daa2ccdu, 0xb77f1cbbu, 0xb8c04957u, 0x8a64cf9bu
};
static const uint32_t k_f4190390[8] = {
    0xad4eaf3du, 0x4a0d4eb9u, 0x66f367b1u, 0xa7771819u,
    0xec4d8f01u, 0xb734e395u, 0x3d8cf133u, 0x878d1e75u
};
static const uint32_t k_f41903b0[8] = {
    0x88ed76d7u, 0xf503e5c9u, 0x9d3a8e31u, 0x6484ba40u,
    0x7f228d0eu, 0x4010edc2u, 0x136845dau, 0x08a05069u
};
static const uint32_t k_f41903d0[8] = {
    0x6ad14551u, 0xf068e8c5u, 0x8ef5023fu, 0x6ff170d1u,
    0xa8e16e91u, 0x24168655u, 0x5313a0e9u, 0x11bdee5du
};
static const uint32_t k_f41903f0[8] = {
    0xbb26da20u, 0x359a545au, 0x53463034u, 0xc0447236u,
    0x828ed41bu, 0x8d0ede21u, 0xcc112a09u, 0x374ec4e9u
};

static const uint32_t k_f41a062c[16] = {
    0xfed56b77u, 0x08a3c8e6u, 0x152cbe7cu, 0x2efb1bebu,
    0x3b841181u, 0x45526ef0u, 0x5f20cc5fu, 0x6ba9c1f5u,
    0x75781f64u, 0x820114fau, 0x9bcf7269u, 0xa85867ffu,
    0xb226c56eu, 0xceafbb04u, 0xd87e1873u, 0xe24c75e2u
};
static const uint32_t k_f41a0670[16][2] = {
    {0x62cbda87u,0xbe7e21d5u}, {0xe08def85u,0xc02f20f2u},
    {0xf5b8f91au,0xd04b8fccu}, {0x737b0e18u,0xe1fc8eeau},
    {0x88a617adu,0xf218fdc4u}, {0x06682cabu,0x03c9fce2u},
    {0x842a41a9u,0x157afbffu}, {0x99554b3eu,0x25976ad9u},
    {0x1717603cu,0x374869f7u}, {0x2c4269d1u,0x4764d8d1u},
    {0xaa047ecfu,0x5915d7eeu}, {0xbf2f8864u,0x693246c8u},
    {0x3cf19d62u,0x7ae345e6u}, {0x521ca6f7u,0x8affb4c0u},
    {0xcfdebbf5u,0x9cb0b3ddu}, {0x4da0d0f3u,0xae61b2fbu}
};
static const uint32_t k_f41a06f0[16][2] = {
    {0xa95d10b0u,0x1f61979au}, {0xefa811aeu,0x407cd424u},
    {0x7380ef7bu,0x7a038bd7u}, {0xf759cd48u,0xa38a4389u},
    {0x7b32ab15u,0xdd10fb3cu}, {0xff0b88e2u,0x0697b2eeu},
    {0x82e466afu,0x301e6aa1u}, {0x06bd447cu,0x69a52254u},
    {0x8a962249u,0x932bda06u}, {0x0e6f0016u,0xccb291b9u},
    {0x9247dde3u,0xf639496bu}, {0x1620bbb0u,0x2fc0011eu},
    {0x99f9997du,0x5946b8d0u}, {0x1dd2774au,0x82cd7083u},
    {0xa1ab5517u,0xbc542835u}, {0x258432e4u,0xe5dadfe8u}
};

static const uint32_t k_f4190490[8] = {
    0xd181b191u, 0x4ff89bbfu, 0x8078c5dfu, 0xcf6a5b17u,
    0x01a6d9adu, 0x0be10bebu, 0xac96540du, 0x238db1f5u
};
static const uint32_t k_f41904b0[8] = {
    0x54f18141u, 0xead2ff98u, 0x271b1f53u, 0x7d826817u,
    0xb8479d81u, 0xf4374a2bu, 0x24efcd11u, 0xe7469e8bu
};
static const uint32_t k_f41904d0[8] = {
    0xe30d2c33u, 0x7fa69fb9u, 0x00cf7235u, 0x076cf313u,
    0x8dc6c4bbu, 0x90ee4c15u, 0x90ebbf87u, 0x4f83d1bdu
};
static const uint32_t k_f41904f0[8] = {
    0x9184067bu, 0x80d256c2u, 0x264fad37u, 0x905a2d38u,
    0x63a4c127u, 0xab581ff3u, 0x29e7cbcbu, 0xa590609cu
};
static const uint32_t k_f4190510[8] = {
    0x3efd5dd5u, 0xfe0a5a67u, 0x4c00f593u, 0x3ddbebb5u,
    0x59fee8ddu, 0xd80aa073u, 0xd32f6cd9u, 0x1eb5909bu
};
static const uint32_t k_f4190530[8] = {
    0x830c1758u, 0x171a0ea1u, 0x2949111au, 0xdfa2ca17u,
    0x35d71404u, 0x69c2f086u, 0xb314c79cu, 0xb2c0ec3bu
};
static const uint32_t k_f4190550[8] = {
    0x4800b905u, 0xaa09ddabu, 0x78684e03u, 0x88dc216fu,
    0xf3f356c1u, 0x6166dc11u, 0x1b66b8d1u, 0x3d575337u
};
static const uint32_t k_f4190570[8] = {
    0x6e36165au, 0x0e44febau, 0xa0d759fau, 0x6ee04a30u,
    0x7e6aab21u, 0x45e3709au, 0xdecbb754u, 0xc3c3d13bu
};

static const uint32_t k_f41a09f0[16] = {
    0x6fde9c0eu, 0x36ac84a8u, 0x0ae784efu, 0xd1b56d89u,
    0xa8835623u, 0x7cbe566au, 0x438c3f04u, 0x1a5a279eu,
    0xe1281038u, 0xb563107fu, 0x8c30f919u, 0x52fee1b3u,
    0x2739e1fau, 0xfe07ca94u, 0xc4d5b32eu, 0x9910b375u
};
static const uint32_t k_f41a0a30[16][2] = {
    {0x0b8d5df5u,0x1137c746u}, {0x89998bd7u,0xece9cb03u},
    {0x07a5b9b9u,0xb89bcec1u}, {0x85b1e79bu,0x844dd27eu},
    {0x03be157du,0x5fffd63cu}, {0x81ca435fu,0x2bb1d9f9u},
    {0xffd67141u,0xf763ddb6u}, {0x7de29f23u,0xc315e174u},
    {0xfbeecd05u,0x9ec7e531u}, {0x79fafae7u,0x6a79e8efu},
    {0xf80728c9u,0x362becacu}, {0x9550788cu,0x06bdb492u},
    {0x135ca66eu,0xd26fb850u}, {0x9168d450u,0xae21bc0du},
    {0x0f750232u,0x79d3bfcbu}, {0x8d813014u,0x4585c388u}
};
static const uint32_t k_f41a0ab0[16][2] = {
    {0x4dfdaca2u,0x6443f93du}, {0x977e263bu,0x3af67fb5u},
    {0x5e531cfdu,0x05c0faadu}, {0xa7d39696u,0xdc738125u},
    {0x6ea88d58u,0xa73dfc1du}, {0xb82906f1u,0x7df08295u},
    {0x7efdfdb3u,0x48bafd8du}, {0xc87e774cu,0x1f6d8405u},
    {0x8f536e0eu,0xea37fefdu}, {0x562864d0u,0xb50279f5u},
    {0x9fa8de69u,0x8bb5006du}, {0x667dd52bu,0x567f7b65u},
    {0xaffe4ec4u,0x2d3201ddu}, {0x76d34586u,0xf7fc7cd5u},
    {0xc053bf1fu,0xceaf034du}, {0x8728b5e1u,0x99797e45u}
};

static void param6_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = multiply_low_u32(token, 0xfd97b913u);
    uint32_t lo = lo0 + 0x76efc4b0u;
    uint32_t hi = multiply_low_u32(token, 0x477fc4a7u) + multiply_high_u32(token, 0xfd97b913u) +
                  0xa6f4af7du + (uint32_t)(0x89103b4fu < lo0);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0670, lo, hi);

    uint32_t lo1 = multiply_low_u32(token, 0x42795131u);
    out_pair[0] = lo1 + 0x6b15a228u;
    out_pair[1] = multiply_low_u32(token, 0x1e7f054au) + multiply_high_u32(token, 0x42795131u) +
                  multiply_low_u32(ladder, 0xa7e7c6d5u) + 0xa74b9d3eu +
                  (uint32_t)(0x94ea5dd7u < lo1);
}

static void local5e0_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = multiply_low_u32(token, 0xc40302a7u);
    uint32_t lo = lo0 + 0xa9673191u;
    uint32_t hi = multiply_low_u32(token, 0x0a4c30b6u) + multiply_high_u32(token, 0xc40302a7u) +
                  0xf9124313u + (uint32_t)(0x5698ce6eu < lo0);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a06f0, lo, hi);

    uint32_t lo1 = multiply_low_u32(token, 0x161c832bu);
    out_pair[0] = lo1 + 0x80607861u;
    out_pair[1] = multiply_low_u32(token, 0xf02bc3fcu) + multiply_high_u32(token, 0x161c832bu) +
                  multiply_low_u32(ladder, 0xd898e023u) + 0xc5ca8832u +
                  (uint32_t)(0x7f9f879eu < lo1);
}

int l3_authorization_primary_param6_seed_prefixes13(
    const int32_t param6_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    int32_t out_local5e0_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param6_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_param6_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]) {
    if (!param6_13 || !out_local5e0_13 || !out_param6_raw13 || !out_param6_prefix13 ||
        !out_local5e0_raw13 || !out_local5e0_prefix13) {
        return -1;
    }

    uint32_t rolling = 0x59900bebu;
    for (unsigned i = 0; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        uint32_t p = (uint32_t)param6_13[i];
        rolling = multiply_low_u32(rolling, 0x1f53c4a3u) +
                  multiply_low_u32(p * k_f4190310[lane] + k_f4190330[lane], 0xf96f5276u) +
                  0x008fe56cu;
        uint32_t t = rolling * 0xa9914925u + 0x5463fc77u;
        uint32_t next = ladder32_run7_then_next(k_f41a062c, &t);
        out_local5e0_13[i] = (int32_t)(multiply_low_u32(rolling * 0x8dad9c39u +
                                                  t * 0xb0000000u + 0x3c2abb71u,
                                                  k_f4190350[lane]) +
                                       k_f4190370[lane]);
        rolling = t * 0x1ae7786fu + next * 0x51887910u + 0xa4058c61u;
    }

    uint32_t pair[2];
    uint32_t prefix[2];

    uint32_t token = (uint32_t)param6_13[0] * 0xf4a8fe5fu + 0x46826f33u;
    param6_pair_from_token(token, pair);
    out_param6_raw13[0][0] = pair[0];
    out_param6_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_param6_prefix13[0][0] = prefix[0];
    out_param6_prefix13[0][1] = prefix[1];

    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = ((uint32_t)param6_13[i] * k_f4190390[lane] + k_f41903b0[lane]) *
                0xadb4bdcbu + 0x874c77b6u;
        param6_pair_from_token(token, pair);
        out_param6_raw13[i][0] = pair[0];
        out_param6_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_param6_prefix13[i][0] = prefix[0];
        out_param6_prefix13[i][1] = prefix[1];
    }

    token = (uint32_t)out_local5e0_13[0] * 0x6c2817b1u + 0x292f1244u;
    local5e0_pair_from_token(token, pair);
    out_local5e0_raw13[0][0] = pair[0];
    out_local5e0_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_local5e0_prefix13[0][0] = prefix[0];
    out_local5e0_prefix13[0][1] = prefix[1];

    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = ((uint32_t)out_local5e0_13[i] * k_f41903d0[lane] + k_f41903f0[lane]) *
                0x3faa9461u + 0x101bec24u;
        local5e0_pair_from_token(token, pair);
        out_local5e0_raw13[i][0] = pair[0];
        out_local5e0_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_local5e0_prefix13[i][0] = prefix[0];
        out_local5e0_prefix13[i][1] = prefix[1];
    }

    return 0;
}

static void pair64_subtract(uint32_t acc[2], const uint32_t v[2]) {
    uint32_t borrow = (acc[0] < v[0]);
    acc[0] -= v[0];
    acc[1] -= v[1] + borrow;
}

static void pair_range_sum(const uint32_t prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
                                unsigned first,
                                unsigned last,
                                uint32_t out_pair[2]) {
    out_pair[0] = prefix13[last][0];
    out_pair[1] = prefix13[last][1];
    if (first != 0u) {
        pair64_subtract(out_pair, prefix13[first - 1u]);
    }
}

int l3_authorization_primary_param6_local5e0_convolution26(
    const uint32_t param6_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t param6_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]) {
    if (!param6_raw13 || !param6_prefix13 || !local5e0_raw13 || !local5e0_prefix13 ||
        !out_pairs26) {
        return -1;
    }

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t uVar14 = 0x6c3f599fu;
        uint32_t iVar16 = 0x3d8c991au;

        if (first <= last) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned k = first; k <= last; ++k) {
                const uint32_t *p6 = param6_raw13[k];
                const uint32_t *l5 = local5e0_raw13[idx - k];
                uint64_t prod = (uint64_t)p6[0] * (uint64_t)l5[0];
                uint32_t prod_lo = (uint32_t)prod;
                uint32_t prod_hi = (uint32_t)(prod >> 32);
                uint32_t old = conv_lo;
                conv_lo += prod_lo;
                conv_hi += l5[1] * p6[0] + l5[0] * p6[1] + prod_hi + (uint32_t)(conv_lo < old);
            }

            uint32_t conv_scaled_lo = multiply_low_u32(conv_lo, 0x84b92317u);
            uint32_t p6_sum[2];
            pair_range_sum(param6_prefix13, first, last, p6_sum);

            uint32_t l5_sum[2];
            pair_range_sum(local5e0_prefix13, idx - last, idx - first, l5_sum);

            uint32_t len = (last - first) + 1u;
            uint64_t len_prod = (uint64_t)len * 0xee8d2566u;
            uint32_t len_lo = (uint32_t)len_prod;
            uint32_t len_hi = (uint32_t)(len_prod >> 32);
            uint32_t term0 = len_lo + 0x6c3f599fu;
            uint32_t term1 = term0 + conv_scaled_lo;
            uint32_t p6_scaled_lo = multiply_low_u32(p6_sum[0], 0xd8f05169u);
            uint32_t term2 = term1 + p6_scaled_lo;
            uint32_t l5_scaled_lo = multiply_low_u32(l5_sum[0], 0x769e0d9au);
            uVar14 = term2 + l5_scaled_lo;

            iVar16 = len * 0x98d8bed3u + len_hi + 0x3d8c991au +
                     add_carry_u32(len_lo, 0x6c3f599fu) +
                     conv_hi * 0x84b92317u +
                     conv_lo * 0xf2e517b1u + multiply_high_u32(conv_lo, 0x84b92317u) +
                     add_carry_u32(term0, conv_scaled_lo) +
                     p6_sum[1] * 0xd8f05169u +
                     p6_sum[0] * 0xea695490u + multiply_high_u32(p6_sum[0], 0xd8f05169u) +
                     add_carry_u32(term1, p6_scaled_lo) +
                     l5_sum[1] * 0x769e0d9au +
                     l5_sum[0] * 0xa17859dfu + multiply_high_u32(l5_sum[0], 0x769e0d9au) +
                     add_carry_u32(term2, l5_scaled_lo);
        }

        uint32_t out_lo_mul = multiply_low_u32(uVar14, 0xcc38a31bu);
        out_pairs26[idx][0] = out_lo_mul + 0x66d8dc5bu;
        out_pairs26[idx][1] = iVar16 * 0xcc38a31bu + uVar14 * 0xe4f2c162u +
                              multiply_high_u32(uVar14, 0xcc38a31bu) + 0x9bb9903au +
                              add_carry_u32(out_lo_mul, 0x66d8dc5bu);
    }

    return 0;
}


int l3_authorization_primary_first_vector_reduce_update_export(
    const int32_t param6_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_first_vector_reduce_trace *out) {
    if (!param6_13 || !vector_reduce_param1_13 || !out_state26 || !out_words13) {
        return -1;
    }

    int32_t local5e0_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t param6_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t param6_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];

    int rc = l3_authorization_primary_param6_seed_prefixes13(
        param6_13, local5e0_13, param6_raw13, param6_prefix13,
        local5e0_raw13, local5e0_prefix13);
    if (rc != 0) {
        return rc;
    }

    rc = l3_authorization_primary_param6_local5e0_convolution26(
        param6_raw13, param6_prefix13, local5e0_raw13, local5e0_prefix13,
        out_state26);
    if (rc != 0) {
        return rc;
    }

    if (out) {
        memcpy(out->local5e0_13, local5e0_13, sizeof(local5e0_13));
        memcpy(out->param6_raw13, param6_raw13, sizeof(param6_raw13));
        memcpy(out->param6_prefix13, param6_prefix13, sizeof(param6_prefix13));
        memcpy(out->local5e0_raw13, local5e0_raw13, sizeof(local5e0_raw13));
        memcpy(out->local5e0_prefix13, local5e0_prefix13, sizeof(local5e0_prefix13));
        memcpy(out->pre_vector_reduce_state26, out_state26, sizeof(out->pre_vector_reduce_state26));
    }

    return l3_authorization_vector_reduce_run13_update_export(
        vector_reduce_param1_13, vector_reduce_param3, vector_reduce_param4,
        out_state26, out_words13, out ? &out->f3fdb57c : NULL);
}

static void post_first_affine_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = multiply_low_u32(token, 0x895bb759u);
    uint32_t lo = lo0 + 0x9aa021f6u;
    uint32_t hi = multiply_low_u32(token, 0xb89cf533u) + multiply_high_u32(token, 0x895bb759u) +
                  0xa4d5ea9fu + (uint32_t)(0x655fde09u < lo0);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0ab0, lo, hi);

    uint32_t lo1 = multiply_low_u32(token, 0x771ad095u);
    out_pair[0] = lo1 + 0x484507d5u;
    out_pair[1] = multiply_low_u32(token, 0x8bcff545u) + multiply_high_u32(token, 0x771ad095u) +
                  multiply_low_u32(ladder, 0xde768863u) + 0xdffd9606u +
                  (uint32_t)(0xb7baf82au < lo1);
}

static void post_first_local614_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = multiply_low_u32(token, 0xc3652dfbu);
    uint32_t lo = lo0 + 0xcae1a358u;
    uint32_t hi = multiply_low_u32(token, 0x98c15f87u) + multiply_high_u32(token, 0xc3652dfbu) +
                  0xb72e7030u + (uint32_t)(0x351e5ca7u < lo0);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0a30, lo, hi);

    uint32_t lo1 = multiply_low_u32(token, 0x427d9297u);
    out_pair[0] = lo1 + 0x17762cabu;
    out_pair[1] = multiply_low_u32(token, 0x6f66f2ffu) + multiply_high_u32(token, 0x427d9297u) +
                  multiply_low_u32(ladder, 0xa0b628ebu) + 0x80e656d0u +
                  (uint32_t)(0xe889d354u < lo1);
}

static void post_first_make_affine_words(
    const uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]) {
    static const uint32_t k0[4] = {0xf22e00a9u, 0x7632184fu, 0x68317277u, 0x9e5d9279u};
    static const uint32_t k1[4] = {0x724512d7u, 0x63779373u, 0xae0516c3u, 0xae98af1bu};
    static const uint32_t add0[4] = {
        0xb7091d89u, 0xec5876d6u, 0x0085073bu, 0xe293cd5au
    };
    static const uint32_t add1[4] = {
        0x0325af50u, 0x369e5fecu, 0x3ddde17au, 0x202a9b15u
    };

    for (unsigned i = 0; i < 4u; ++i) {
        out_affine13[i] = multiply_low_u32(first_words13[i], k0[i]) + add0[i];
        out_affine13[i + 4u] = multiply_low_u32(first_words13[i + 4u], k1[i]) + add1[i];
        out_affine13[i + 8u] = multiply_low_u32(first_words13[i + 8u], k0[i]) + add0[i];
    }
    out_affine13[12] = first_words13[12] * 0x724512d7u + 0x0325af50u;
}

static void post_first_make_local614(
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    int32_t out_local614_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]) {
    uint32_t rolling = 0x3bb7c8bfu;
    for (unsigned i = 0; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        uint32_t p = (uint32_t)param1_13[i];
        rolling = rolling * 0xa89c1431u +
                  multiply_low_u32(p * k_f4190490[lane] + k_f41904b0[lane], 0xd47e47aeu) +
                  0xe1ea644fu;
        uint32_t t = rolling * 0x6b68dff7u + 0x62e592f7u;
        uint32_t next = ladder32_run7_then_next(k_f41a09f0, &t);
        out_local614_13[i] = (int32_t)(multiply_low_u32(rolling * 0xe00f77bdu +
                                                  t * 0x50000000u + 0xd78ff690u,
                                                  k_f41904d0[lane]) +
                                       k_f41904f0[lane]);
        rolling = t * 0xdfee5f77u + next * 0x011a0890u + 0x8d87ffccu;
    }
}

int l3_authorization_primary_post_first_seed_prefixes13(
    const uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    int32_t out_local614_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_affine_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local614_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local614_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]) {
    if (!first_words13 || !param1_13 || !out_affine_words13 || !out_local614_13 ||
        !out_affine_raw13 || !out_affine_prefix13 || !out_local614_raw13 ||
        !out_local614_prefix13) {
        return -1;
    }

    post_first_make_affine_words(first_words13, out_affine_words13);
    post_first_make_local614(param1_13, out_local614_13);

    uint32_t pair[2];
    uint32_t prefix[2];

    uint32_t token = out_affine_words13[0] * 0x22b4d00fu + 0x8bfa6d59u;
    post_first_affine_pair_from_token(token, pair);
    out_affine_raw13[0][0] = pair[0];
    out_affine_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_affine_prefix13[0][0] = prefix[0];
    out_affine_prefix13[0][1] = prefix[1];

    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = ((out_affine_words13[i] * k_f4190550[lane] + k_f4190570[lane]) *
                 0x2ac52103u) + 0x5134904bu;
        post_first_affine_pair_from_token(token, pair);
        out_affine_raw13[i][0] = pair[0];
        out_affine_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_affine_prefix13[i][0] = prefix[0];
        out_affine_prefix13[i][1] = prefix[1];
    }

    token = (uint32_t)out_local614_13[0] * 0x7567e04fu + 0x687d4ef2u;
    post_first_local614_pair_from_token(token, pair);
    out_local614_raw13[0][0] = pair[0];
    out_local614_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_local614_prefix13[0][0] = prefix[0];
    out_local614_prefix13[0][1] = prefix[1];

    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = (((uint32_t)out_local614_13[i] * k_f4190510[lane] + k_f4190530[lane]) *
                 0xcfcb8393u) + 0x4dd1df6au;
        post_first_local614_pair_from_token(token, pair);
        out_local614_raw13[i][0] = pair[0];
        out_local614_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_local614_prefix13[i][0] = prefix[0];
        out_local614_prefix13[i][1] = prefix[1];
    }

    return 0;
}

int l3_authorization_primary_post_first_convolution26(
    const uint32_t affine_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t affine_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t local614_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t local614_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]) {
    if (!affine_raw13 || !affine_prefix13 || !local614_raw13 || !local614_prefix13 ||
        !out_pairs26) {
        return -1;
    }

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t uVar14 = 0xb10dd3aeu;
        uint32_t iVar16 = 0x4cee8915u;

        if (first <= last) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned k = first; k <= last; ++k) {
                const uint32_t *a = affine_raw13[k];
                const uint32_t *b = local614_raw13[idx - k];
                uint64_t prod = (uint64_t)a[0] * (uint64_t)b[0];
                uint32_t old = conv_lo;
                conv_lo += (uint32_t)prod;
                conv_hi += b[1] * a[0] + b[0] * a[1] + (uint32_t)(prod >> 32) +
                           (uint32_t)(conv_lo < old);
            }

            uint32_t conv_scaled_lo = multiply_low_u32(conv_lo, 0x40219053u);
            uint32_t b_sum[2];
            pair_range_sum(local614_prefix13, first, last, b_sum);

            uint32_t a_sum[2];
            pair_range_sum(affine_prefix13, idx - last, idx - first, a_sum);

            uint32_t len = (last - first) + 1u;
            uint64_t len_prod = (uint64_t)len * 0xf26e041cu;
            uint32_t len_lo = (uint32_t)len_prod;
            uint32_t len_hi = (uint32_t)(len_prod >> 32);
            uint32_t term0 = len_lo + 0xb10dd3aeu;
            uint32_t term1 = term0 + conv_scaled_lo;
            uint32_t b_scaled_lo = multiply_low_u32(b_sum[0], 0xe1ac54cau);
            uint32_t term2 = term1 + b_scaled_lo;
            uint32_t a_scaled_lo = multiply_low_u32(a_sum[0], 0x998622c2u);
            uVar14 = term2 + a_scaled_lo;

            iVar16 = len * 0x1133f447u + len_hi + 0x4cee8915u +
                     add_carry_u32(len_lo, 0xb10dd3aeu) +
                     conv_hi * 0x40219053u +
                     conv_lo * 0x2fbdd6afu + multiply_high_u32(conv_lo, 0x40219053u) +
                     add_carry_u32(term0, conv_scaled_lo) +
                     b_sum[1] * 0xe1ac54cau +
                     b_sum[0] * 0xba8ac7e6u + multiply_high_u32(b_sum[0], 0xe1ac54cau) +
                     add_carry_u32(term1, b_scaled_lo) +
                     a_sum[1] * 0x998622c2u +
                     a_sum[0] * 0x5edd8a59u + multiply_high_u32(a_sum[0], 0x998622c2u) +
                     add_carry_u32(term2, a_scaled_lo);
        }

        uint32_t out_lo_mul = multiply_low_u32(uVar14, 0x772616afu);
        out_pairs26[idx][0] = out_lo_mul + 0xefe6e52eu;
        out_pairs26[idx][1] = iVar16 * 0x772616afu + uVar14 * 0x852bfe57u +
                              multiply_high_u32(uVar14, 0x772616afu) + 0x54e20373u +
                              add_carry_u32(out_lo_mul, 0xefe6e52eu);
    }

    return 0;
}

int l3_authorization_primary_post_first_pre_second_preimage(
    const uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_first_seed_trace *out) {
    if (!first_words13 || !param1_13 || !out_pairs26) {
        return -1;
    }

    l3_authorization_primary_post_first_seed_trace tmp;
    l3_authorization_primary_post_first_seed_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_first_seed_prefixes13(
        first_words13, param1_13,
        dst->affine_words13, dst->local614_13,
        dst->affine_raw13, dst->affine_prefix13,
        dst->local614_raw13, dst->local614_prefix13);
    if (rc != 0) {
        return rc;
    }

    return l3_authorization_primary_post_first_convolution26(
        dst->affine_raw13, dst->affine_prefix13,
        dst->local614_raw13, dst->local614_prefix13,
        out_pairs26);
}

int l3_authorization_primary_second_vector_reduce_update_export(
    const uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_second_vector_reduce_trace *out) {
    if (!first_words13 || !param1_13 || !vector_reduce_param1_13 || !out_state26 || !out_words13) {
        return -1;
    }

    l3_authorization_primary_post_first_seed_trace local_seed;
    l3_authorization_primary_post_first_seed_trace *seed_dst = out ? &out->seed : &local_seed;

    int rc = l3_authorization_primary_post_first_pre_second_preimage(
        first_words13, param1_13, out_state26, seed_dst);
    if (rc != 0) {
        return rc;
    }

    if (out) {
        memcpy(out->pre_vector_reduce_state26, out_state26, sizeof(out->pre_vector_reduce_state26));
    }

    return l3_authorization_vector_reduce_run13_update_export(
        vector_reduce_param1_13, vector_reduce_param3, vector_reduce_param4,
        out_state26, out_words13, out ? &out->f3fdb57c : NULL);
}

int l3_authorization_primary_live_q654_from_second_words13(
    const uint32_t second_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_q6_words4[4],
    uint32_t out_q5_words4[4],
    uint32_t out_q4_words4[4]) {
    static const uint32_t k_q10_q9_mul[4] = {
        0x0c56e031u, 0x15f25fc7u, 0x0052e0c5u, 0x1a177f3bu
    };
    static const uint32_t k_q8_mul[4] = {
        0x52d94cf1u, 0x3d0956e5u, 0xb0badff3u, 0xec2e7181u
    };
    static const uint32_t k_q11_add[4] = {
        0x2bd3b4b2u, 0x81cd8396u, 0xde71c5e6u, 0xe3286562u
    };
    static const uint32_t k_q12_add[4] = {
        0xcbac0b76u, 0xcf02a1cdu, 0x58b1c9eeu, 0xb772b368u
    };
    if (!second_words13 || !out_q6_words4 || !out_q5_words4 ||
        !out_q4_words4) {
        return -1;
    }
    for (unsigned i = 0; i < 4u; ++i) {
        out_q6_words4[i] =
            second_words13[i] * k_q10_q9_mul[i] + k_q12_add[i];
        out_q5_words4[i] =
            second_words13[i + 4u] * k_q8_mul[i] + k_q11_add[i];
        out_q4_words4[i] =
            second_words13[i + 8u] * k_q10_q9_mul[i] + k_q12_add[i];
    }
    return 0;
}


/* v233: post-second-FUN_f3fdb57c setup.  This isolates the next native
 * FUN_f3fc3c70 seam: the second call's exported 13-word stream is first
 * transformed by three 4-lane affine vector blocks plus a scalar tail; in
 * parallel, param_3 is expanded through the f41a0b30/f41a0bb0 ladders.  The
 * two resulting streams feed the next 26-pair preimage before the third
 * FUN_f3fdb57c call.
 */
static const uint32_t k_f4190590[8] = {
    0xffbda07du, 0x7fa066efu, 0x8d53a605u, 0xaaae40d3u,
    0xd5a86a75u, 0xe10b7351u, 0x80c6ac91u, 0x20d76b0du
};
static const uint32_t k_f41905b0[8] = {
    0x6bd8e542u, 0xbe4a19ebu, 0xee21714eu, 0xdc8cd1d0u,
    0x2d050a42u, 0x7c7fd535u, 0x6baa82d3u, 0x50469dbeu
};
static const uint32_t k_f41905d0[8] = {
    0x7b20f13bu, 0xdc50e179u, 0x12646073u, 0xd215acf5u,
    0x09912683u, 0xc50b3a47u, 0xbf8adf07u, 0xe545042bu
};
static const uint32_t k_f41905f0[8] = {
    0xb199d2a2u, 0x6da2bbd1u, 0x46a44fb6u, 0x600df764u,
    0x30c725a2u, 0x0578fb77u, 0xbfbe58a9u, 0xf2ba6dc6u
};

static const uint32_t k_f41a0b30[16][2] = {
    {0x07cf6b56u,0x90bedda5u}, {0x441f8793u,0x2c216846u},
    {0x806fa3d0u,0xb783f2e7u}, {0x7b147ec8u,0x462e44d7u},
    {0xb7649b05u,0xd190cf78u}, {0xf3b4b742u,0x6cf35a19u},
    {0x3004d37fu,0xf855e4bbu}, {0x6c54efbcu,0x83b86f5cu},
    {0xa8a50bf9u,0x1f1af9fdu}, {0xa349e6f1u,0xadc54bedu},
    {0xdf9a032eu,0x3927d68eu}, {0x1bea1f6bu,0xc48a6130u},
    {0x583a3ba8u,0x5fecebd1u}, {0x948a57e5u,0xeb4f7672u},
    {0x8f2f32ddu,0x79f9c862u}, {0xcb7f4f1au,0x055c5303u}
};
static const uint32_t k_f41a0bb0[16][2] = {
    {0x66da6e34u,0xcf232d58u}, {0xf684c57du,0xb0fcd6ddu},
    {0x862f1cc6u,0xa2d68063u}, {0x169d3ed6u,0x9e2c5d4au},
    {0xa647961fu,0x800606cfu}, {0x36b5b82fu,0x7b5be3b6u},
    {0xc6600f78u,0x6d358d3bu}, {0x56ce3188u,0x588b6a22u},
    {0xe67888d1u,0x4a6513a7u}, {0x7622e01au,0x3c3ebd2du},
    {0x0691022au,0x27949a14u}, {0x963b5973u,0x196e4399u},
    {0x26a97b83u,0x04c42080u}, {0xb653d2ccu,0xf69dca05u},
    {0x46c1f4dcu,0xe1f3a6ecu}, {0xd66c4c25u,0xd3cd5071u}
};

static void post_second_make_affine_words(
    const uint32_t second_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]) {
    static const uint32_t k_a[4] = {0x0c56e031u, 0x15f25fc7u, 0x0052e0c5u, 0x1a177f3bu};
    static const uint32_t k_b[4] = {0x52d94cf1u, 0x3d0956e5u, 0xb0badff3u, 0xec2e7181u};
    static const uint32_t add_a[4] = {0xcbac0b76u, 0xcf02a1cdu, 0x58b1c9eeu, 0xb772b368u};
    static const uint32_t add_b[4] = {0x2bd3b4b2u, 0x81cd8396u, 0xde71c5e6u, 0xe3286562u};
    for (unsigned i = 0; i < 4u; ++i) {
        out_affine13[i] = second_words13[i] * k_a[i] + add_a[i];
        out_affine13[i + 4u] = second_words13[i + 4u] * k_b[i] + add_b[i];
        out_affine13[i + 8u] = second_words13[i + 8u] * k_a[i] + add_a[i];
    }
    out_affine13[12] = second_words13[12] * 0x52d94cf1u + 0x2bd3b4b2u;
}

static void post_second_param3a_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0xea345d83u;
    uint32_t lo = lo0 + 0x03f5a7e7u;
    uint32_t hi = token * 0x41a0cea4u + multiply_high_u32(token, 0xea345d83u) +
                  0xc4fc37c8u + add_carry_u32(lo0, 0x03f5a7e7u);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0b30, lo, hi);
    uint32_t lo1 = token * 0xecc8234du;
    out_pair[0] = lo1 + 0xf31044c5u;
    out_pair[1] = token * 0xd74e7b4fu + multiply_high_u32(token, 0xecc8234du) +
                  ladder * 0xecaa0d11u + 0x71094967u +
                  add_carry_u32(lo1, 0xf31044c5u);
}

static void post_second_param3b_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0xe710656bu;
    uint32_t lo = lo0 + 0xf936fd9bu;
    uint32_t hi = token * 0xfa1333a5u + multiply_high_u32(token, 0xe710656bu) +
                  0x1f6b9cd8u + add_carry_u32(lo0, 0xf936fd9bu);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0bb0, lo, hi);
    uint32_t lo1 = token * 0x8f0bcb4bu;
    out_pair[0] = lo1 + 0x0235c205u;
    out_pair[1] = token * 0xcca33ceeu + multiply_high_u32(token, 0x8f0bcb4bu) +
                  ladder * 0x99ee365fu + 0xfc9827d5u +
                  add_carry_u32(lo1, 0x0235c205u);
}

int l3_authorization_primary_post_second_seed_prefixes13(
    const uint32_t second_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_affine_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_param3_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_param3_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]) {
    if (!second_words13 || !param3_13 || !out_affine_words13 || !out_affine_raw13 ||
        !out_affine_prefix13 || !out_param3_raw13 || !out_param3_prefix13) {
        return -1;
    }

    post_second_make_affine_words(second_words13, out_affine_words13);

    uint32_t pair[2];
    uint32_t prefix[2];

    uint32_t token = (uint32_t)param3_13[0] * 0x8ffa4d0fu + 0x6a97a2acu;
    post_second_param3b_pair_from_token(token, pair);
    out_param3_raw13[0][0] = pair[0];
    out_param3_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_param3_prefix13[0][0] = prefix[0];
    out_param3_prefix13[0][1] = prefix[1];

    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = ((uint32_t)param3_13[i] * k_f41905d0[lane] +
                 k_f41905f0[lane]) * 0x36d5563du + 0xff640612u;
        post_second_param3b_pair_from_token(token, pair);
        out_param3_raw13[i][0] = pair[0];
        out_param3_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_param3_prefix13[i][0] = prefix[0];
        out_param3_prefix13[i][1] = prefix[1];
    }

    /*
     * The f41a0b30 stream is the first 64-bit convolution operand.  The
     * vector-affine second-call output remains live in callee-saved q4-q6
     * across the third FUN_f3fdb57c call and belongs to the following stage;
     * it is not folded into these pairs.
     */
    for (unsigned i = 0; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        uint32_t t = (i == 0u)
            ? ((uint32_t)param3_13[0] * 0x2d1ab04bu + 0x93f82e35u)
            : (((uint32_t)param3_13[i] * k_f4190590[lane] +
                k_f41905b0[lane]) * 0x60a3f667u + 0x0a6084a7u);
        uint32_t a_pair[2];
        post_second_param3a_pair_from_token(t, a_pair);
        out_affine_raw13[i][0] = a_pair[0];
        out_affine_raw13[i][1] = a_pair[1];
        if (i == 0u) {
            prefix[0] = a_pair[0];
            prefix[1] = a_pair[1];
        } else {
            pair64_add(prefix, a_pair);
        }
        out_affine_prefix13[i][0] = prefix[0];
        out_affine_prefix13[i][1] = prefix[1];
    }

    return 0;
}

int l3_authorization_primary_post_second_convolution26(
    const uint32_t affine_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t affine_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t param3_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t param3_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]) {
    if (!affine_raw13 || !affine_prefix13 || !param3_raw13 || !param3_prefix13 || !out_pairs26) {
        return -1;
    }

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x7c45962eu;
        uint32_t v = 0x055cecc8u;
        if (first <= last) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned k = first; k <= last; ++k) {
                const uint32_t *a = affine_raw13[k];
                const uint32_t *b = param3_raw13[idx - k];
                uint64_t prod = (uint64_t)a[0] * (uint64_t)b[0];
                uint32_t old = conv_lo;
                conv_lo += (uint32_t)prod;
                conv_hi += b[1] * a[0] + b[0] * a[1] + (uint32_t)(prod >> 32) +
                           (uint32_t)(conv_lo < old);
            }

            uint32_t conv_scaled_lo = conv_lo * 0x9da692b5u;
            uint32_t a_sum[2];
            pair_range_sum(affine_prefix13, first, last, a_sum);
            uint32_t b_sum[2];
            pair_range_sum(param3_prefix13, idx - last, idx - first, b_sum);

            uint32_t len = (last - first) + 1u;
            uint64_t len_prod = (uint64_t)len * 0x4a2fa05cu;
            uint32_t len_lo = (uint32_t)len_prod;
            uint32_t len_hi = (uint32_t)(len_prod >> 32);
            uint32_t term0 = len_lo + 0x7c45962eu;
            uint32_t term1 = term0 + conv_scaled_lo;
            uint32_t a_scaled_lo = a_sum[0] * 0x08110a41u;
            uint32_t term2 = term1 + a_scaled_lo;
            uint32_t b_scaled_lo = b_sum[0] * 0x2718de0cu;
            u = term2 + b_scaled_lo;

            v = len * 0x56449a16u + len_hi + 0x055cecc8u +
                add_carry_u32(len_lo, 0x7c45962eu) +
                conv_hi * 0x9da692b5u + conv_lo * 0x79837df8u +
                multiply_high_u32(conv_lo, 0x9da692b5u) + add_carry_u32(term0, conv_scaled_lo) +
                a_sum[1] * 0x08110a41u + a_sum[0] * 0x72b90095u +
                multiply_high_u32(a_sum[0], 0x08110a41u) + add_carry_u32(term1, a_scaled_lo) +
                b_sum[1] * 0x2718de0cu + b_sum[0] * 0x93ac7d48u +
                multiply_high_u32(b_sum[0], 0x2718de0cu) + add_carry_u32(term2, b_scaled_lo);
        }

        uint32_t out_lo_mul = u * 0xa2205ed5u;
        out_pairs26[idx][0] = out_lo_mul + 0xa194b4dau;
        out_pairs26[idx][1] = v * 0xa2205ed5u + u * 0xb7ca60e3u +
                              multiply_high_u32(u, 0xa2205ed5u) + 0xf8eb431du +
                              add_carry_u32(out_lo_mul, 0xa194b4dau);
    }
    return 0;
}

int l3_authorization_primary_post_second_pre_third_preimage(
    const uint32_t second_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_second_seed_trace *out) {
    if (!second_words13 || !param3_13 || !out_pairs26) {
        return -1;
    }
    l3_authorization_primary_post_second_seed_trace tmp;
    l3_authorization_primary_post_second_seed_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_second_seed_prefixes13(
        second_words13, param3_13, dst->affine_words13, dst->affine_raw13,
        dst->affine_prefix13, dst->param3_raw13, dst->param3_prefix13);
    if (rc != 0) {
        return rc;
    }
    return l3_authorization_primary_post_second_convolution26(
        dst->affine_raw13, dst->affine_prefix13,
        dst->param3_raw13, dst->param3_prefix13, out_pairs26);
}


int l3_authorization_primary_third_vector_reduce_update_export(
    const uint32_t second_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_third_vector_reduce_trace *out) {
    if (!second_words13 || !param3_13 || !vector_reduce_param1_13 ||
        !out_state26 || !out_words13) {
        return -1;
    }

    l3_authorization_primary_post_second_seed_trace tmp_seed;
    l3_authorization_primary_post_second_seed_trace *seed = out ? &out->seed : &tmp_seed;
    int rc = l3_authorization_primary_post_second_pre_third_preimage(
        second_words13, param3_13, out_state26, seed);
    if (rc != 0) {
        return rc;
    }

    if (out) {
        memcpy(out->pre_vector_reduce_state26, out_state26, sizeof(out->pre_vector_reduce_state26));
    }

    return l3_authorization_vector_reduce_run13_update_export(
        vector_reduce_param1_13, vector_reduce_param3, vector_reduce_param4,
        out_state26, out_words13, out ? &out->f3fdb57c : NULL);
}

/* v235: post-third-FUN_f3fdb57c setup.  Native FUN_f3fc3c70 takes the
 * third call's local_4a4..local_474 words, forms a 13-word affine stream,
 * expands it through two parallel f41a0c30/f41a0cb0 pair ladders, and
 * convolves those streams into the 26-pair image before the fourth
 * FUN_f3fdb57c call.
 */
static const uint32_t k_f4190610[8] = {
    0xe1b7dc9fu, 0x324fa895u, 0xf8c00cf3u, 0x9a70fc33u,
    0xb2fb859fu, 0xfbd316f5u, 0x2b01ff19u, 0x2d0c9ba9u
};
static const uint32_t k_f4190630[8] = {
    0xda9bc744u, 0xe744d298u, 0xe09c6688u, 0x97cf8a51u,
    0xbc3a2187u, 0xb297e4f9u, 0xa7ab787du, 0xd02d49c7u
};
static const uint32_t k_f4190650[8] = {
    0x4e58c859u, 0x73a0dc13u, 0xd4df1aa5u, 0x2c05a565u,
    0xf7726759u, 0x3e10e0b3u, 0xef7a39afu, 0x0148819fu
};
static const uint32_t k_f4190670[8] = {
    0x7ddd0c2fu, 0x44895b7bu, 0x098a670bu, 0x76a8a18au,
    0xb1044404u, 0x773d1c22u, 0xe6ff24beu, 0x1c3a5dc4u
};

static const uint32_t k_f41a0c30[16][2] = {
    {0xd572c2e7u,0x258f66cau}, {0x3d6d66f8u,0x3d368e08u},
    {0xa5680b09u,0x44ddb545u}, {0x0d62af1au,0x5c84dc83u},
    {0xf579e826u,0x6b5b3278u}, {0x5d748c37u,0x730259b6u},
    {0xc56f3048u,0x8aa980f3u}, {0x2d69d459u,0x9250a831u},
    {0x9564786au,0xa9f7cf6eu}, {0x7d7bb176u,0xb8ce2564u},
    {0xe5765587u,0xc0754ca1u}, {0x4d70f998u,0xd81c73dfu},
    {0xb56b9da9u,0xefc39b1cu}, {0x1d6641bau,0xf76ac25au},
    {0x057d7ac6u,0x06411850u}, {0x6d781ed7u,0x1de83f8du}
};
static const uint32_t k_f41a0cb0[16][2] = {
    {0xb1d3a97cu,0xa1e36343u}, {0xe910b5ccu,0x709a18a3u},
    {0x204dc21cu,0x4f50ce04u}, {0x578ace6cu,0x1e078364u},
    {0x8ec7dabcu,0xecbe38c4u}, {0xc604e70cu,0xbb74ee24u},
    {0x89712e5du,0x8ec04d81u}, {0xc0ae3aadu,0x5d7702e1u},
    {0xf7eb46fdu,0x2c2db841u}, {0x2f28534du,0xfae46da2u},
    {0x66655f9du,0xc99b2302u}, {0x9da26bedu,0x9851d862u},
    {0xd4df783du,0x67088dc2u}, {0x0c1c848du,0x35bf4323u},
    {0x435990ddu,0x0475f883u}, {0x7a969d2du,0xd32cade3u}
};

static void post_third_make_affine_words(
    const uint32_t third_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]) {
    static const uint32_t k_a[4] = {0x72b17f75u, 0xd0800de7u, 0xda01e0a1u, 0x3325334bu};
    static const uint32_t k_b[4] = {0x590ec40fu, 0xcf117931u, 0x8b8e5f1du, 0xa8e3ffb3u};
    static const uint32_t add_a[4] = {
        0x5adefdcau, 0xba6bdda1u, 0xf8af36dau, 0x43145596u
    };
    static const uint32_t add_b[4] = {
        0xff280527u, 0xfacdf5c2u, 0x3513b3eeu, 0x22eb7604u
    };
    for (unsigned i = 0; i < 4u; ++i) {
        out_affine13[i] = third_words13[i] * k_a[i] + add_a[i];
        out_affine13[i + 4u] = third_words13[i + 4u] * k_b[i] + add_b[i];
        out_affine13[i + 8u] = third_words13[i + 8u] * k_a[i] + add_a[i];
    }
    out_affine13[12] = third_words13[12] * 0x590ec40fu + 0xff280527u;
}

static void post_third_c30_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0xbe2fd295u;
    uint32_t lo = lo0 + 0x7937de1bu;
    uint32_t hi = token * 0xeeb19525u + multiply_high_u32(token, 0xbe2fd295u) +
                  0x88f4d528u + add_carry_u32(lo0, 0x7937de1bu);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0c30, lo, hi);
    uint32_t lo1 = token * 0x30f445e9u;
    out_pair[0] = lo1 + 0xf60b4fd4u;
    out_pair[1] = token * 0x7e0f5616u + multiply_high_u32(token, 0x30f445e9u) +
                  ladder * 0xd888bafbu + 0x2d552151u +
                  add_carry_u32(lo1, 0xf60b4fd4u);
}

static void post_third_cb0_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0x6c49500du;
    uint32_t lo = lo0 + 0x5750f8eau;
    uint32_t hi = token * 0x9da5f811u + multiply_high_u32(token, 0x6c49500du) +
                  0x182b8f9cu + add_carry_u32(lo0, 0x5750f8eau);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0cb0, lo, hi);
    uint32_t lo1 = token * 0x53d0aae5u;
    out_pair[0] = lo1 + 0x8f4aa8d2u;
    out_pair[1] = token * 0x024ddc28u + multiply_high_u32(token, 0x53d0aae5u) +
                  ladder * 0x13d8c7c7u + 0x39afd5feu +
                  add_carry_u32(lo1, 0x8f4aa8d2u);
}

int l3_authorization_primary_post_third_seed_prefixes13(
    const uint32_t third_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_c30_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_c30_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_cb0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_cb0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]) {
    if (!third_words13 || !out_affine_words13 || !out_c30_raw13 || !out_c30_prefix13 ||
        !out_cb0_raw13 || !out_cb0_prefix13) {
        return -1;
    }

    post_third_make_affine_words(third_words13, out_affine_words13);

    uint32_t pair[2];
    uint32_t prefix[2] = {0u, 0u};

    uint32_t token = out_affine_words13[0] * 0xe576428bu + 0x3e515990u;
    post_third_c30_pair_from_token(token, pair);
    out_c30_raw13[0][0] = pair[0];
    out_c30_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_c30_prefix13[0][0] = prefix[0];
    out_c30_prefix13[0][1] = prefix[1];
    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = (out_affine_words13[i] * k_f4190610[lane] +
                 k_f4190630[lane]) * 0x6544e695u + 0x4a8f46fcu;
        post_third_c30_pair_from_token(token, pair);
        out_c30_raw13[i][0] = pair[0];
        out_c30_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_c30_prefix13[i][0] = prefix[0];
        out_c30_prefix13[i][1] = prefix[1];
    }

    token = out_affine_words13[0] * 0x137aa4d9u + 0x534ba73fu;
    post_third_cb0_pair_from_token(token, pair);
    out_cb0_raw13[0][0] = pair[0];
    out_cb0_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_cb0_prefix13[0][0] = prefix[0];
    out_cb0_prefix13[0][1] = prefix[1];
    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = (out_affine_words13[i] * k_f4190650[lane] +
                 k_f4190670[lane]) * 0x26f53081u + 0xd1a4b390u;
        post_third_cb0_pair_from_token(token, pair);
        out_cb0_raw13[i][0] = pair[0];
        out_cb0_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_cb0_prefix13[i][0] = prefix[0];
        out_cb0_prefix13[i][1] = prefix[1];
    }

    return 0;
}

int l3_authorization_primary_post_third_convolution26(
    const uint32_t c30_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t c30_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t cb0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t cb0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]) {
    if (!c30_raw13 || !c30_prefix13 || !cb0_raw13 || !cb0_prefix13 || !out_pairs26) {
        return -1;
    }

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x552f2b90u;
        uint32_t v = 0x132f876fu;
        if (first <= last) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned k = first; k <= last; ++k) {
                const uint32_t *a = c30_raw13[k];
                const uint32_t *b = cb0_raw13[idx - k];
                uint64_t p = (uint64_t)b[0] * (uint64_t)a[0];
                uint32_t plo = (uint32_t)p;
                uint32_t old = conv_lo;
                conv_lo += plo;
                conv_hi += b[1] * a[0] + a[1] * b[0] + (uint32_t)(p >> 32) +
                           add_carry_u32(old, plo);
            }

            uint32_t conv_scaled_lo = conv_lo * 0x53ba33f3u;
            uint32_t a_sum[2];
            pair_range_sum(c30_prefix13, first, last, a_sum);
            uint32_t b_sum[2];
            pair_range_sum(cb0_prefix13, idx - last, idx - first, b_sum);

            uint32_t len = (last - first) + 1u;
            uint64_t len_prod = (uint64_t)len * 0x3dbf420eu;
            uint32_t len_lo = (uint32_t)len_prod;
            uint32_t len_hi = (uint32_t)(len_prod >> 32);
            uint32_t term0 = len_lo + 0x552f2b90u;
            uint32_t term1 = term0 + conv_scaled_lo;
            uint32_t a_scaled_lo = a_sum[0] * 0xe68d0e86u;
            uint32_t term2 = term1 + a_scaled_lo;
            uint32_t b_scaled_lo = b_sum[0] * 0x4538fff7u;
            u = term2 + b_scaled_lo;

            v = len * 0x1a2d551au + len_hi + 0x132f876fu +
                add_carry_u32(len_lo, 0x552f2b90u) +
                conv_hi * 0x53ba33f3u + conv_lo * 0x323da6aeu +
                multiply_high_u32(conv_lo, 0x53ba33f3u) + add_carry_u32(term0, conv_scaled_lo) +
                a_sum[1] * 0xe68d0e86u + a_sum[0] * 0x9dcda696u +
                multiply_high_u32(a_sum[0], 0xe68d0e86u) + add_carry_u32(term1, a_scaled_lo) +
                b_sum[1] * 0x4538fff7u + b_sum[0] * 0xad3bdd99u +
                multiply_high_u32(b_sum[0], 0x4538fff7u) + add_carry_u32(term2, b_scaled_lo);
        }

        uint32_t out_lo_mul = u * 0x660d7237u;
        out_pairs26[idx][0] = out_lo_mul + 0x91871130u;
        out_pairs26[idx][1] = v * 0x660d7237u + u * 0x6c69b0d1u +
                              multiply_high_u32(u, 0x660d7237u) + 0x948f83e9u +
                              add_carry_u32(out_lo_mul, 0x91871130u);
    }
    return 0;
}

int l3_authorization_primary_post_third_pre_fourth_preimage(
    const uint32_t third_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_third_seed_trace *out) {
    if (!third_words13 || !out_pairs26) {
        return -1;
    }
    l3_authorization_primary_post_third_seed_trace tmp;
    l3_authorization_primary_post_third_seed_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_third_seed_prefixes13(
        third_words13, dst->affine_words13, dst->c30_raw13, dst->c30_prefix13,
        dst->cb0_raw13, dst->cb0_prefix13);
    if (rc != 0) {
        return rc;
    }
    return l3_authorization_primary_post_third_convolution26(
        dst->c30_raw13, dst->c30_prefix13, dst->cb0_raw13, dst->cb0_prefix13,
        out_pairs26);
}

int l3_authorization_primary_fourth_vector_reduce_update_export(
    const uint32_t third_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_fourth_vector_reduce_trace *out) {
    if (!third_words13 || !vector_reduce_param1_13 || !out_state26 || !out_words13) {
        return -1;
    }

    l3_authorization_primary_post_third_seed_trace local_seed;
    l3_authorization_primary_post_third_seed_trace *seed_dst = out ? &out->seed : &local_seed;

    int rc = l3_authorization_primary_post_third_pre_fourth_preimage(
        third_words13, out_state26, seed_dst);
    if (rc != 0) {
        return rc;
    }

    if (out) {
        memcpy(out->pre_vector_reduce_state26, out_state26, sizeof(out->pre_vector_reduce_state26));
    }

    return l3_authorization_vector_reduce_run13_update_export(
        vector_reduce_param1_13, vector_reduce_param3, vector_reduce_param4,
        out_state26, out_words13, out ? &out->f3fdb57c : NULL);
}


/* v237: post-fourth front-end. */
static const uint32_t k_f4190690[8] = {
    0xdeb3d175u, 0x2e2db31bu, 0x92dcb9bbu, 0x62b12ed3u,
    0xa200cf81u, 0x95ff1377u, 0xf9da4f61u, 0xd4e56de9u
};
static const uint32_t k_f41906b0[8] = {
    0xc93c96d0u, 0x30203de3u, 0x3dee466au, 0xc7c728beu,
    0x599c8c10u, 0xe8a58ba2u, 0x1acaa3e0u, 0xf58c8d82u
};
static const uint32_t k_f41a0d30[16][2] = {
    {0x39225a42u,0x6aab6911u}, {0x7291b115u,0x5856dfcau},
    {0x8701fdfbu,0x4394928du}, {0x9b724ae1u,0x3ed24550u},
    {0xd4e1a1b4u,0x2c7dbc09u}, {0xe951ee9au,0x17bb6eccu},
    {0xfdc23b80u,0x02f9218fu}, {0x37319253u,0xf0a49849u},
    {0x4ba1df39u,0xebe24b0cu}, {0x60122c1fu,0xd71ffdcfu},
    {0x998182f2u,0xc4cb7488u}, {0xadf1cfd8u,0xb009274bu},
    {0xc2621cbeu,0xab46da0eu}, {0xfbd17391u,0x98f250c7u},
    {0x1041c077u,0x8430038bu}, {0x24b20d5du,0x7f6db64eu}
};

static void post_fourth_make_affine_words(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]) {
    static const uint32_t k_a[4] = {0x35ed61ffu, 0x1b375c4fu, 0xbcf0a2b5u, 0x7b7fa675u};
    static const uint32_t k_b[4] = {0x0126070fu, 0x77db2d5fu, 0x0f7320b3u, 0x779cfe2fu};
    static const uint32_t add_a[4] = {
        0x383cb4adu, 0x8264dec0u, 0xa6fd8c05u, 0x817461aeu
    };
    static const uint32_t add_b[4] = {
        0x57a1f5e9u, 0x0f5e3179u, 0x902b0e1fu, 0x8416ea04u
    };
    for (unsigned i = 0; i < 4u; ++i) {
        out_affine13[i] = fourth_words13[i] * k_a[i] + add_a[i];
        out_affine13[i + 4u] = fourth_words13[i + 4u] * k_b[i] + add_b[i];
        out_affine13[i + 8u] = fourth_words13[i + 8u] * k_a[i] + add_a[i];
    }
    out_affine13[12] = fourth_words13[12] * 0x0126070fu + 0x57a1f5e9u;
}

static void post_fourth_param1_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0x0f94ee3du;
    uint32_t lo = lo0 + 0xd9a16ce7u;
    uint32_t hi = token * 0x60e35950u + multiply_high_u32(token, 0x0f94ee3du) +
                  0x5891d4aeu + add_carry_u32(lo0, 0xd9a16ce7u);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0d30, lo, hi);
    uint32_t lo1 = token * 0xc8104ab3u;
    out_pair[0] = lo1 + 0x012ce2a9u;
    out_pair[1] = token * 0x0b45c13bu + multiply_high_u32(token, 0xc8104ab3u) +
                  ladder * 0x684ae451u + 0x5b426d5eu +
                  add_carry_u32(lo1, 0x012ce2a9u);
}

int l3_authorization_primary_post_fourth_affine_param1_frontend13(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param1_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_fourth_frontend_trace *out) {
    if (!fourth_words13 || !param1_13 || !out_affine_words13 || !out_param1_raw13) {
        return -1;
    }

    post_fourth_make_affine_words(fourth_words13, out_affine_words13);

    for (unsigned i = 0; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        uint32_t token = ((uint32_t)param1_13[i] * k_f4190690[lane] +
                          k_f41906b0[lane]) * 0x57c18e61u + 0x0f227a15u;
        post_fourth_param1_pair_from_token(token, out_param1_raw13[i]);
    }

    if (out) {
        memcpy(out->affine_words13, out_affine_words13, sizeof(out->affine_words13));
        memcpy(out->param1_raw13, out_param1_raw13, sizeof(out->param1_raw13));
    }
    return 0;
}

/* v238: post-fourth local_718 self-convolution. */
static const uint32_t k_f41906d0[8] = {
    0x2d92344fu, 0xf2292ce7u, 0xd5e3bc0fu, 0x3cc01281u,
    0xe30a0141u, 0x26bd2ba9u, 0x65e89c8fu, 0x183a9de9u
};
static const uint32_t k_f41906f0[8] = {
    0x976be07bu, 0xd2959dc6u, 0xd26f0324u, 0xc3333b13u,
    0x7faee541u, 0xedcc80b6u, 0x98b8d649u, 0xb97eae34u
};
static const uint32_t k_f41a0db0[16][2] = {
    {0xca098ce9u,0x14c45600u}, {0xa2f6359eu,0x0a4a54e9u},
    {0x7be2de53u,0xffd053d2u}, {0x54cf8708u,0xe55652bbu},
    {0x2dbc2fbdu,0xdadc51a4u}, {0x77de4d23u,0xc80261ffu},
    {0x50caf5d8u,0xbd8860e8u}, {0x29b79e8du,0xa30e5fd1u},
    {0x02a44742u,0x98945ebau}, {0xdb90eff7u,0x8e1a5da2u},
    {0xb47d98acu,0x73a05c8bu}, {0x8d6a4161u,0x69265b74u},
    {0x6656ea16u,0x5eac5a5du}, {0x3f4392cbu,0x44325946u},
    {0x18303b80u,0x39b8582fu}, {0xf11ce435u,0x2f3e5717u}
};

static void ladder0db0_mid_tail(uint32_t lo, uint32_t hi,
                                     uint32_t *mid_lo, uint32_t *mid_hi,
                                     uint32_t *tail_mix_low) {
    for (unsigned i = 0; i < 7u; ++i) {
        unsigned nib = lo & 0x0fu;
        uint32_t shifted = (lo >> 4) | (hi << 28);
        uint32_t tlo = k_f41a0db0[nib][0];
        uint32_t nlo = shifted + tlo;
        uint32_t nhi = (hi >> 4) + k_f41a0db0[nib][1] + add_carry_u32(shifted, tlo);
        lo = nlo;
        hi = nhi;
    }
    *mid_lo = lo;
    *mid_hi = hi;

    for (unsigned i = 0; i < 7u; ++i) {
        unsigned nib = lo & 0x0fu;
        uint32_t shifted = (lo >> 4) | (hi << 28);
        uint32_t tlo = k_f41a0db0[nib][0];
        uint32_t nlo = shifted + tlo;
        uint32_t nhi = (hi >> 4) + k_f41a0db0[nib][1] + add_carry_u32(shifted, tlo);
        lo = nlo;
        hi = nhi;
    }

    unsigned nib = lo & 0x0fu;
    uint32_t shifted = (lo >> 4) | (hi << 28);
    uint32_t tlo = k_f41a0db0[nib][0];
    uint32_t lo15 = shifted + tlo;
    uint32_t hi15 = (hi >> 4) + k_f41a0db0[nib][1] + add_carry_u32(shifted, tlo);
    uint32_t lo16_shifted = (lo15 >> 4) | (hi15 << 28);
    uint32_t lo16 = lo16_shifted + k_f41a0db0[lo15 & 0x0fu][0];
    *tail_mix_low = lo16;
}

int l3_authorization_primary_post_fourth_self_convolution26(
    const uint32_t param1_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_rolling_tail[2]) {
    if (!param1_raw13 || !out_local718_26) {
        return -1;
    }

    uint32_t roll_hi = 0x0da3f683u;
    uint32_t roll_lo = 0x04d9ad4eu;

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++idx) {
        unsigned s = (idx < 13u) ? 0u : idx - 12u;
        unsigned e = (idx < 13u) ? idx : 12u;
        uint32_t acc_lo = 0x7b60dcc7u;
        uint32_t acc_hi = 0xd73f2fc2u;

        while (s < e) {
            uint32_t b_lo = param1_raw13[e][0];
            uint32_t b_hi = param1_raw13[e][1];
            uint32_t a_lo = param1_raw13[s][0];
            uint32_t a_hi = param1_raw13[s][1];
            uint32_t b_scaled_lo = b_lo * 0x5603a245u;
            uint32_t b_pair_lo = b_scaled_lo + 0x4e93e5d1u;
            uint64_t prod = (uint64_t)a_lo * (uint64_t)b_pair_lo;
            uint32_t prod_lo = (uint32_t)prod;
            uint32_t b_aff_lo = b_lo * 0x4e93e5d1u;
            uint32_t c0 = add_carry_u32(acc_lo, b_aff_lo);
            uint32_t t0 = acc_lo + b_aff_lo;
            uint32_t c1 = add_carry_u32(t0, prod_lo);
            uint32_t t1 = t0 + prod_lo;
            uint32_t c2 = add_carry_u32(t1, 0xc861fcadu);
            (void)c2;
            acc_lo = t1 + 0xc861fcadu;
            acc_hi = acc_hi +
                     b_hi * 0x4e93e5d1u +
                     b_lo * 0xf3afadc5u + multiply_high_u32(b_lo, 0x4e93e5d1u) + c0 +
                     (b_hi * 0x5603a245u +
                      b_lo * 0x97e8cd77u + multiply_high_u32(b_lo, 0x5603a245u) +
                      0xf3afadc5u + add_carry_u32(b_scaled_lo, 0x4e93e5d1u)) * a_lo +
                     b_pair_lo * a_hi + (uint32_t)(prod >> 32) + c1 +
                     0x0227daf0u + add_carry_u32(t1, 0xc861fcadu);
            ++s;
            --e;
        }

        uint32_t u14, u23, i18, i16, bias, cflag;
        if (s == e) {
            uint32_t t_lo = roll_lo * 0x6c70ab9fu;
            uint32_t r_lo = acc_lo * 0x0b58b32au;
            uint32_t sum_lo = r_lo + t_lo;
            uint64_t prod0 = (uint64_t)sum_lo * 0xbccf3bd1ull;
            u14 = (uint32_t)prod0;
            uint32_t mid_lo = param1_raw13[s][0];
            uint32_t mid_hi = param1_raw13[s][1];
            uint32_t mid_scaled = mid_lo * 0xf4733379u;
            uint32_t mid_pair_lo = mid_scaled + 0x11afd16au;
            uint64_t prod1 = (uint64_t)mid_lo * (uint64_t)mid_pair_lo;
            u23 = (uint32_t)prod1;
            i18 = (acc_hi * 0x0b58b32au +
                   acc_lo * 0x9a9a2e6bu + multiply_high_u32(acc_lo, 0x0b58b32au) +
                   roll_hi * 0x6c70ab9fu +
                   roll_lo * 0x109299dau + multiply_high_u32(roll_lo, 0x6c70ab9fu) +
                   add_carry_u32(r_lo, t_lo)) * 0xbccf3bd1u +
                  sum_lo * 0x0490c9e6u + (uint32_t)(prod0 >> 32);
            i16 = (mid_hi * 0xf4733379u +
                   mid_lo * 0x8a8ff0e6u + multiply_high_u32(mid_lo, 0xf4733379u) +
                   0x2b48db3du + add_carry_u32(mid_scaled, 0x11afd16au)) * mid_lo +
                  mid_pair_lo * mid_hi + (uint32_t)(prod1 >> 32);
            cflag = add_carry_u32(u14, 0xfa9edbaau);
            u14 += 0xfa9edbaau;
            bias = 0x6ce2fbd4u;
        } else {
            uint32_t t_lo = roll_lo * 0x091ac1cfu;
            u14 = t_lo + 0xf6183b29u;
            u23 = acc_lo * 0x6baaf34au;
            i18 = roll_hi * 0x091ac1cfu +
                  roll_lo * 0x09cf674eu + multiply_high_u32(roll_lo, 0x091ac1cfu);
            i16 = acc_hi * 0x6baaf34au +
                  acc_lo * 0x2b72d309u + multiply_high_u32(acc_lo, 0x6baaf34au);
            cflag = add_carry_u32(t_lo, 0xf6183b29u);
            bias = 0xfd84d08bu;
        }

        uint32_t next_lo = u14 + u23;
        uint32_t lo_mul = next_lo * 0x852f768fu;
        uint32_t ladder_lo = lo_mul + 0x6461aaa5u;
        uint32_t ladder_hi = (i18 + bias + cflag + i16 + add_carry_u32(u14, u23)) * 0x852f768fu +
                             next_lo * 0x595b3642u + multiply_high_u32(next_lo, 0x852f768fu) +
                             0x55ae8ba6u + add_carry_u32(lo_mul, 0x6461aaa5u);
        uint32_t mid_lo, mid_hi, tail_mix;
        ladder0db0_mid_tail(ladder_lo, ladder_hi, &mid_lo, &mid_hi, &tail_mix);

        out_local718_26[idx] = (next_lo * 0xf9511dfbu + mid_lo * 0xb0000000u + 0x2d383a07u) *
                               k_f41906d0[idx & 7u] + k_f41906f0[idx & 7u];

        uint32_t mid_mul = mid_lo * 0xe0603761u;
        roll_lo = mid_mul + 0x9d04ba5au;
        roll_hi = mid_hi * 0xe0603761u +
                  mid_lo * 0x42c54970u + multiply_high_u32(mid_lo, 0xe0603761u) +
                  tail_mix * 0xf9fc89f0u + 0xc43b2b69u + add_carry_u32(mid_mul, 0x9d04ba5au);
    }

    if (out_rolling_tail) {
        out_rolling_tail[0] = roll_lo;
        out_rolling_tail[1] = roll_hi;
    }
    return 0;
}

int l3_authorization_primary_post_fourth_frontend_self_convolution26(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param1_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_fourth_selfconv_trace *out) {
    if (!fourth_words13 || !param1_13 || !out_affine_words13 || !out_param1_raw13 || !out_local718_26) {
        return -1;
    }
    int rc = l3_authorization_primary_post_fourth_affine_param1_frontend13(
        fourth_words13, param1_13, out_affine_words13, out_param1_raw13,
        out ? &out->frontend : NULL);
    if (rc != 0) {
        return rc;
    }
    uint32_t tail[2];
    rc = l3_authorization_primary_post_fourth_self_convolution26(
        out_param1_raw13, out_local718_26, out ? out->rolling_tail : tail);
    if (rc != 0) {
        return rc;
    }
    if (out) {
        memcpy(out->local718_26, out_local718_26, sizeof(out->local718_26));
    }
    return 0;
}

/* v239: post-fourth f41a0e30/f41a0eb0 staging and convolution preimage. */
static const uint32_t k_f4190710[8] = {
    0xbce88117u, 0x3a5d6a03u, 0x1ee95507u, 0x4921e3d1u,
    0x269398b3u, 0xdc6dce21u, 0x3101b88bu, 0x57265cd7u
};
static const uint32_t k_f4190730[8] = {
    0xfa4e41f1u, 0x21168cafu, 0xb8ba2038u, 0xea4138b0u,
    0x9089cd81u, 0xd808db20u, 0x89ab32c9u, 0x90b769dcu
};
static const uint32_t k_f4190750[8] = {
    0x18875bc3u, 0xffe1d63bu, 0x2ff9d4a1u, 0xc374ac4bu,
    0x917288a9u, 0xfa784c2du, 0x72622f31u, 0x445b23d3u
};
static const uint32_t k_f4190770[8] = {
    0x07848f34u, 0x72f484d0u, 0x32cb3e3au, 0xdad31680u,
    0xba21e574u, 0xfbbbba45u, 0x8a9d4bc3u, 0x0615c836u
};
static const uint32_t k_f41a0e30[16][2] = {
    {0x34e0d97du,0x739e964bu}, {0x26cffcc9u,0x81d1363cu},
    {0xa191d578u,0x970004ceu}, {0x1c53ae27u,0xac2ed361u},
    {0x971586d6u,0xb15da1f3u}, {0x8904aa22u,0xcf9041e4u},
    {0x03c682d1u,0xd4bf1077u}, {0x7e885b80u,0xe9eddf09u},
    {0x70777eccu,0xf8207efau}, {0xeb39577bu,0x0d4f4d8cu},
    {0x65fb302au,0x127e1c1fu}, {0x57ea5376u,0x20b0bc10u},
    {0xd2ac2c25u,0x35df8aa2u}, {0x4d6e04d4u,0x4b0e5935u},
    {0x3f5d2820u,0x5940f926u}, {0xba1f00cfu,0x6e6fc7b8u}
};
static const uint32_t k_f41a0eb0[16][2] = {
    {0xb472dc84u,0x1e624d63u}, {0x0f71ef43u,0xcdf69f19u},
    {0x6a710202u,0x7d8af0ceu}, {0xc57014c1u,0x2d1f4283u},
    {0x907418dbu,0xd99c8b1cu}, {0xeb732b9au,0x8930dcd1u},
    {0x46723e59u,0x38c52e87u}, {0xa1715118u,0xe859803cu},
    {0xfc7063d7u,0x97edd1f1u}, {0xc77467f1u,0x446b1a8au},
    {0x22737ab0u,0xf3ff6c40u}, {0x7d728d6fu,0xa393bdf5u},
    {0xd871a02eu,0x53280faau}, {0x3370b2edu,0x02bc6160u},
    {0x8e6fc5acu,0xb250b315u}, {0x5973c9c6u,0x6ecdfbaeu}
};

static void post_fourth_e30_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0x29e4d07fu;
    uint32_t lo = lo0 + 0x0ecb55abu;
    uint32_t hi = token * 0x05582afeu + multiply_high_u32(token, 0x29e4d07fu) +
                  0xe9d53fa0u + add_carry_u32(lo0, 0x0ecb55abu);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0e30, lo, hi);
    uint32_t lo1 = token * 0xc9837a39u;
    out_pair[0] = lo1 + 0x238ff7bfu;
    out_pair[1] = token * 0x96956989u + multiply_high_u32(token, 0xc9837a39u) +
                  ladder * 0x7f5126b9u + 0xd6b6641cu +
                  add_carry_u32(lo1, 0x238ff7bfu);
}

static void post_fourth_eb0_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0xbb3cd999u;
    uint32_t lo = lo0 + 0xd2f0ae99u;
    uint32_t hi = token * 0xaddeec05u + multiply_high_u32(token, 0xbb3cd999u) +
                  0xc6885a0bu + add_carry_u32(lo0, 0xd2f0ae99u);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0eb0, lo, hi);
    uint32_t lo1 = token * 0x9c595be9u;
    out_pair[0] = lo1 + 0x223cf40eu;
    out_pair[1] = token * 0xcdcff8f6u + multiply_high_u32(token, 0x9c595be9u) +
                  ladder * 0x9c48d92fu + 0xf4fe2325u +
                  add_carry_u32(lo1, 0x223cf40eu);
}

int l3_authorization_primary_post_fourth_staging_pair_streams13(
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t seed13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_e30_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_e30_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_eb0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_eb0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]) {
    if (!affine_words13 || !seed13 || !out_e30_raw13 || !out_e30_prefix13 ||
        !out_eb0_raw13 || !out_eb0_prefix13) {
        return -1;
    }

    uint32_t pair[2];
    uint32_t prefix[2];

    uint32_t token = (uint32_t)seed13[0] * 0x97518895u + 0xc910ca73u;
    post_fourth_e30_pair_from_token(token, pair);
    out_e30_raw13[0][0] = pair[0];
    out_e30_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_e30_prefix13[0][0] = prefix[0];
    out_e30_prefix13[0][1] = prefix[1];
    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = ((uint32_t)seed13[i] * k_f4190710[lane] +
                 k_f4190730[lane]) * 0xd2875733u + 0xdaf9c070u;
        post_fourth_e30_pair_from_token(token, pair);
        out_e30_raw13[i][0] = pair[0];
        out_e30_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_e30_prefix13[i][0] = prefix[0];
        out_e30_prefix13[i][1] = prefix[1];
    }

    token = affine_words13[0] * 0x6ea6956bu + 0x042dab13u;
    post_fourth_eb0_pair_from_token(token, pair);
    out_eb0_raw13[0][0] = pair[0];
    out_eb0_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_eb0_prefix13[0][0] = prefix[0];
    out_eb0_prefix13[0][1] = prefix[1];
    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = (affine_words13[i] * k_f4190750[lane] +
                 k_f4190770[lane]) * 0x4358cd39u + 0x5b1d247fu;
        post_fourth_eb0_pair_from_token(token, pair);
        out_eb0_raw13[i][0] = pair[0];
        out_eb0_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_eb0_prefix13[i][0] = prefix[0];
        out_eb0_prefix13[i][1] = prefix[1];
    }
    return 0;
}

int l3_authorization_primary_post_fourth_staging_convolution26(
    const uint32_t e30_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t e30_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t eb0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t eb0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]) {
    if (!e30_raw13 || !e30_prefix13 || !eb0_raw13 || !eb0_prefix13 || !out_pairs26) {
        return -1;
    }

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x370636d0u;
        uint32_t v = 0xd59277f4u;
        if (first <= last) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned k = first; k <= last; ++k) {
                const uint32_t *a = e30_raw13[k];
                const uint32_t *b = eb0_raw13[idx - k];
                uint64_t prod = (uint64_t)a[0] * (uint64_t)b[0];
                uint32_t old = conv_lo;
                conv_lo += (uint32_t)prod;
                conv_hi += b[1] * a[0] + b[0] * a[1] + (uint32_t)(prod >> 32) +
                           (uint32_t)(conv_lo < old);
            }

            uint32_t conv_scaled_lo = conv_lo * 0x92c55be5u;
            uint32_t e30_sum[2];
            pair_range_sum(e30_prefix13, first, last, e30_sum);
            uint32_t eb0_sum[2];
            pair_range_sum(eb0_prefix13, idx - last, idx - first, eb0_sum);

            uint32_t len = (last - first) + 1u;
            uint64_t len_prod = (uint64_t)len * 0xeaf87407u;
            uint32_t len_lo = (uint32_t)len_prod;
            uint32_t len_hi = (uint32_t)(len_prod >> 32);
            uint32_t term0 = len_lo + 0x370636d0u;
            uint32_t term1 = term0 + conv_scaled_lo;
            uint32_t e30_scaled_lo = e30_sum[0] * 0x72d59ae1u;
            uint32_t term2 = term1 + e30_scaled_lo;
            uint32_t eb0_scaled_lo = eb0_sum[0] * 0x530eeaa3u;
            u = term2 + eb0_scaled_lo;

            v = len * 0x4d4d4307u + len_hi + 0xd59277f4u +
                add_carry_u32(len_lo, 0x370636d0u) +
                conv_hi * 0x92c55be5u + conv_lo * 0x0f70cc7fu +
                multiply_high_u32(conv_lo, 0x92c55be5u) + add_carry_u32(term0, conv_scaled_lo) +
                e30_sum[1] * 0x72d59ae1u + e30_sum[0] * 0x9f338f9fu +
                multiply_high_u32(e30_sum[0], 0x72d59ae1u) + add_carry_u32(term1, e30_scaled_lo) +
                eb0_sum[1] * 0x530eeaa3u + eb0_sum[0] * 0xf66c7befu +
                multiply_high_u32(eb0_sum[0], 0x530eeaa3u) + add_carry_u32(term2, eb0_scaled_lo);
        }

        uint32_t out_lo_mul = u * 0x46707901u;
        out_pairs26[idx][0] = out_lo_mul + 0xfb36874eu;
        out_pairs26[idx][1] = v * 0x46707901u + u * 0x105faa31u +
                              multiply_high_u32(u, 0x46707901u) + 0x62a6df39u +
                              add_carry_u32(out_lo_mul, 0xfb36874eu);
    }
    return 0;
}

int l3_authorization_primary_post_fourth_pre_fifth_preimage(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t post_fourth_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t seed13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param1_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_fourth_staging_trace *out) {
    if (!fourth_words13 || !post_fourth_param1_13 || !seed13 || !out_affine_words13 ||
        !out_param1_raw13 || !out_local718_26 || !out_pairs26) {
        return -1;
    }

    l3_authorization_primary_post_fourth_staging_trace tmp;
    l3_authorization_primary_post_fourth_staging_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_fourth_frontend_self_convolution26(
        fourth_words13, post_fourth_param1_13, out_affine_words13,
        out_param1_raw13, out_local718_26, &dst->selfconv);
    if (rc != 0) {
        return rc;
    }
    rc = l3_authorization_primary_post_fourth_staging_pair_streams13(
        out_affine_words13, seed13, dst->e30_raw13, dst->e30_prefix13,
        dst->eb0_raw13, dst->eb0_prefix13);
    if (rc != 0) {
        return rc;
    }
    rc = l3_authorization_primary_post_fourth_staging_convolution26(
        dst->e30_raw13, dst->e30_prefix13, dst->eb0_raw13,
        dst->eb0_prefix13, out_pairs26);
    if (rc != 0) {
        return rc;
    }
    if (out) {
        memcpy(out->pre_fifth_pairs26, out_pairs26, sizeof(out->pre_fifth_pairs26));
    }
    return 0;
}

/* v240: post-fourth local_718/DAT_f41a0f30 pair-stream staging. */
static const uint32_t k_f4190790[8] = {
    0x05c3b6d3u, 0x1fd7075bu, 0xf8e98e13u, 0xb35fee9du,
    0x7c5cb2ddu, 0x3b158855u, 0xafe00f93u, 0x2991db15u
};
static const uint32_t k_f41907b0[8] = {
    0x71ce8a6fu, 0xe837826eu, 0x6841ce24u, 0xc99d1d29u,
    0xa141e6b3u, 0x3bcb9562u, 0x686faee5u, 0xc0223b8cu
};
static const uint32_t k_f41907d0[8] = {
    0x8601e831u, 0xa3b25e77u, 0x4d8ff3ebu, 0xdcc0411fu,
    0x96d80e7du, 0xba94abc7u, 0xad59804du, 0x310b38d9u
};
static const uint32_t k_f41907f0[8] = {
    0xf6e07653u, 0x2a1fb7ccu, 0x54a808d4u, 0x3948d652u,
    0x7a95b797u, 0x01143354u, 0x092b6e62u, 0x566ff9d7u
};
static const uint32_t k_f41a05f8[L3_AUTH_ROUND_PRIMARY_SEED_WORDS] = {
    0x46ce98e0u, 0xf074d292u, 0xb4f18f02u, 0xf00cfda8u,
    0xe0c893cfu, 0xafd2aefau, 0x053e4868u, 0x5deb8e8bu,
    0x9e584607u, 0xf074d292u, 0xb4f18f02u, 0xf00cfda8u,
    0xe0c893cfu
};
static const uint32_t k_f41a0f30[16][2] = {
    {0xca01fdeau,0x05a2f059u}, {0x348cddd2u,0xfc9599a4u},
    {0x9f17bdbau,0xe38842eeu}, {0x09a29da2u,0xda7aec39u},
    {0xcb7f7f0bu,0xc24300dcu}, {0x360a5ef3u,0xb935aa27u},
    {0xa0953edbu,0xa0285371u}, {0x0b201ec3u,0x971afcbcu},
    {0x75aafeabu,0x8e0da606u}, {0xe035de93u,0x75004f50u},
    {0x4ac0be7bu,0x6bf2f89bu}, {0xb54b9e63u,0x52e5a1e5u},
    {0x1fd67e4bu,0x49d84b30u}, {0x8a615e33u,0x30caf47au},
    {0xf4ec3e1bu,0x27bd9dc4u}, {0x5f771e03u,0x1eb0470fu}
};
static const uint32_t k_f41a0fb0[16][2] = {
    {0x3c2aa599u,0x12853f3au}, {0x9c315fe3u,0x2857e9a0u},
    {0x51698df8u,0x39c7073au}, {0xb1704842u,0x4f99b1a0u},
    {0x1177028cu,0x556c5c07u}, {0x717dbcd6u,0x6b3f066du},
    {0xd1847720u,0x7111b0d3u}, {0x318b316au,0x86e45b3au},
    {0xe6c35f7fu,0x985378d3u}, {0x46ca19c9u,0xae26233au},
    {0xa6d0d413u,0xb3f8cda0u}, {0x06d78e5du,0xc9cb7807u},
    {0x66de48a7u,0xdf9e226du}, {0x1c1676bcu,0xe10d4007u},
    {0x7c1d3106u,0xf6dfea6du}, {0xdc23eb50u,0x0cb294d3u}
};

static void post_fourth_local718_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0x90ab3f03u;
    uint32_t lo = lo0 + 0x7e1dbf3fu;
    uint32_t hi = token * 0x8d68000du + multiply_high_u32(token, 0x90ab3f03u) +
                  0x0fb10f23u + add_carry_u32(lo0, 0x7e1dbf3fu);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0f30, lo, hi);
    uint32_t lo1 = token * 0xc8250415u;
    out_pair[0] = lo1 + 0x54aa3f41u;
    out_pair[1] = token * 0x18264730u + multiply_high_u32(token, 0xc8250415u) +
                  ladder * 0x203ae6f9u + 0x6ab014fbu +
                  add_carry_u32(lo1, 0x54aa3f41u);
}

static void post_fourth_fixed_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0x7ed57c85u;
    uint32_t lo = lo0 + 0x0391298bu;
    uint32_t hi = token * 0x17145189u + multiply_high_u32(token, 0x7ed57c85u) +
                  0x62b28a7au + add_carry_u32(lo0, 0x0391298bu);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0fb0, lo, hi);
    uint32_t lo1 = token * 0x3f7b6b91u;
    out_pair[0] = lo1 + 0x62a14739u;
    out_pair[1] = token * 0xec38d551u + multiply_high_u32(token, 0x3f7b6b91u) +
                  ladder * 0xbbbdc963u + 0x0c7634f9u +
                  add_carry_u32(lo1, 0x62a14739u);
}


int l3_authorization_primary_post_fourth_local718_staging_pair_streams(
    const uint32_t local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_local718_raw26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_local718_prefix26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_fixed_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_fixed_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_fourth_local718_staging_trace *out) {
    if (!local718_26 || !out_local718_raw26 || !out_local718_prefix26 ||
        !out_fixed_raw13 || !out_fixed_prefix13) {
        return -1;
    }

    uint32_t pair[2];
    uint32_t prefix[2];

    uint32_t token = local718_26[0] * 0xddd4aea1u + 0x4fb36323u;
    post_fourth_local718_pair_from_token(token, pair);
    out_local718_raw26[0][0] = pair[0];
    out_local718_raw26[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_local718_prefix26[0][0] = prefix[0];
    out_local718_prefix26[0][1] = prefix[1];

    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++i) {
        unsigned lane = i & 7u;
        token = (local718_26[i] * k_f4190790[lane] +
                 k_f41907b0[lane]) * 0xef40c43bu + 0xb65c7f8eu;
        post_fourth_local718_pair_from_token(token, pair);
        out_local718_raw26[i][0] = pair[0];
        out_local718_raw26[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_local718_prefix26[i][0] = prefix[0];
        out_local718_prefix26[i][1] = prefix[1];
    }

    out_fixed_raw13[0][0] = 0xcf9b9658u;
    out_fixed_raw13[0][1] = 0xe345765eu;
    prefix[0] = 0xcf9b9658u;
    prefix[1] = 0xe345765eu;
    out_fixed_prefix13[0][0] = prefix[0];
    out_fixed_prefix13[0][1] = prefix[1];

    for (unsigned i = 1; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = (k_f41a05f8[i] * k_f41907d0[lane] +
                 k_f41907f0[lane]) * 0xbf2ecd7bu + 0x0cd3052eu;
        post_fourth_fixed_pair_from_token(token, pair);
        out_fixed_raw13[i][0] = pair[0];
        out_fixed_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_fixed_prefix13[i][0] = prefix[0];
        out_fixed_prefix13[i][1] = prefix[1];
    }

    if (out) {
        memcpy(out->local718_raw26, out_local718_raw26, sizeof(out->local718_raw26));
        memcpy(out->local718_prefix26, out_local718_prefix26, sizeof(out->local718_prefix26));
        memcpy(out->fixed_raw13, out_fixed_raw13, sizeof(out->fixed_raw13));
        memcpy(out->fixed_prefix13, out_fixed_prefix13, sizeof(out->fixed_prefix13));
    }
    return 0;
}

/* v241: convolution over the v240 local_718/fixed pair streams. */
static void pair_range_sum26(
    const uint32_t prefix26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    unsigned first,
    unsigned last,
    uint32_t out_pair[2]) {
    out_pair[0] = prefix26[last][0];
    out_pair[1] = prefix26[last][1];
    if (first != 0u) {
        uint32_t borrow = (out_pair[0] < prefix26[first - 1u][0]);
        out_pair[0] -= prefix26[first - 1u][0];
        out_pair[1] -= prefix26[first - 1u][1] + borrow;
    }
}

int l3_authorization_primary_post_fourth_local718_fixed_convolution26(
    const uint32_t local718_raw26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    const uint32_t local718_prefix26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    const uint32_t fixed_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t fixed_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]) {
    if (!local718_raw26 || !local718_prefix26 || !fixed_raw13 ||
        !fixed_prefix13 || !out_pairs26) {
        return -1;
    }

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++idx) {
        unsigned first = idx - 12u;
        if (idx < 12u || first == 0u) {
            first = 0u;
        }

        uint32_t u = 0xe5cae725u;
        uint32_t v = 0xa8bda7b7u; /* -0x57425849 */
        if (first <= idx) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned k = first; k <= idx; ++k) {
                const uint32_t *a = local718_raw26[k];
                const uint32_t *b = fixed_raw13[idx - k];
                uint64_t prod = (uint64_t)a[0] * (uint64_t)b[0];
                uint32_t old = conv_lo;
                conv_lo += (uint32_t)prod;
                conv_hi += b[1] * a[0] + b[0] * a[1] +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv_lo < old);
            }

            uint32_t local_sum[2];
            pair_range_sum26(local718_prefix26, first, idx, local_sum);
            unsigned fixed_last = idx - first;
            const uint32_t *fixed_sum = fixed_prefix13[fixed_last];
            uint32_t len = fixed_last + 1u;

            uint64_t len_prod = (uint64_t)len * 0x2c30eba3u;
            uint32_t len_lo = (uint32_t)len_prod;
            uint32_t len_hi = (uint32_t)(len_prod >> 32);
            uint64_t fixed_prod = (uint64_t)fixed_sum[0] * 0xeda85b69u;
            uint32_t fixed_scaled_lo = (uint32_t)fixed_prod;
            uint32_t conv_scaled_lo = conv_lo * 0xd1120433u;
            uint32_t local_scaled_lo = local_sum[0] * 0x42061f91u;

            uint32_t t0 = len_lo + 0xe5cae725u;
            uint32_t t1 = t0 + conv_scaled_lo;
            uint32_t t2 = t1 + local_scaled_lo;
            u = t2 + fixed_scaled_lo;

            v = len * 0x0376e491u + len_hi + 0xa8bda7b7u +
                add_carry_u32(len_lo, 0xe5cae725u) +
                conv_hi * 0xd1120433u + conv_lo * 0x7116cb3cu +
                multiply_high_u32(conv_lo, 0xd1120433u) + add_carry_u32(t0, conv_scaled_lo) +
                local_sum[1] * 0x42061f91u + local_sum[0] * 0xca82db89u +
                multiply_high_u32(local_sum[0], 0x42061f91u) + add_carry_u32(t1, local_scaled_lo) +
                fixed_sum[1] * 0xeda85b69u + fixed_sum[0] * 0x3308742fu +
                (uint32_t)(fixed_prod >> 32) + add_carry_u32(t2, fixed_scaled_lo);
        }

        uint32_t out_lo = u * 0x29a1dba5u;
        out_pairs26[idx][0] = out_lo + 0xfc85a16au;
        out_pairs26[idx][1] = v * 0x29a1dba5u + u * 0x67c11270u +
                              multiply_high_u32(u, 0x29a1dba5u) + 0x03e6f09d5u +
                              add_carry_u32(out_lo, 0xfc85a16au);
    }
    return 0;
}

int l3_authorization_primary_post_fourth_local718_staging_convolution26(
    const uint32_t local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_fourth_local718_convolution_trace *out) {
    if (!local718_26 || !out_pairs26) {
        return -1;
    }
    l3_authorization_primary_post_fourth_local718_convolution_trace tmp;
    l3_authorization_primary_post_fourth_local718_convolution_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_fourth_local718_staging_pair_streams(
        local718_26, dst->streams.local718_raw26, dst->streams.local718_prefix26,
        dst->streams.fixed_raw13, dst->streams.fixed_prefix13, &dst->streams);
    if (rc != 0) {
        return rc;
    }
    rc = l3_authorization_primary_post_fourth_local718_fixed_convolution26(
        dst->streams.local718_raw26, dst->streams.local718_prefix26,
        dst->streams.fixed_raw13, dst->streams.fixed_prefix13, out_pairs26);
    if (rc != 0) {
        return rc;
    }
    if (out) {
        memcpy(out->conv_pairs26, out_pairs26, sizeof(out->conv_pairs26));
    }
    return 0;
}

/* v242: vector-affine fold after the post-fourth local_3a0 convolution. */
static uint64_t pack_pair(const uint32_t pair[2]) {
    return ((uint64_t)pair[1] << 32) | (uint64_t)pair[0];
}

static void unpack_pair(uint64_t lane, uint32_t pair[2]) {
    pair[0] = (uint32_t)lane;
    pair[1] = (uint32_t)(lane >> 32);
}

int l3_authorization_primary_post_fourth_vector_affine_state26(
    const uint32_t pre_fifth_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    const uint32_t local3a0_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_fourth_vector_affine_trace *out) {
    if (!pre_fifth_pairs26 || !local3a0_pairs26 || !out_state26) {
        return -1;
    }

    const uint64_t pre_mul_lane = 0xcc37487174205329ULL;
    const uint64_t local_mul_lane = 0xee2f5d61d4749dcdULL;
    const uint64_t add_lane = 0x242d46348cd49cabULL;
    for (unsigned i = 0; i < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++i) {
        uint64_t pre_lane = pack_pair(pre_fifth_pairs26[i]);
        uint64_t local_lane = pack_pair(local3a0_pairs26[i]);

        /* f3fc88c0..f3fc8eb8 implements two ordinary low-64-bit
         * multiplications in scalar ARM instructions, two lanes at a time.
         * The apparent high-half masking in the decompiler is only the
         * UMULL/MLA expansion of these full 64-bit products.
         */
        uint64_t folded =
            pre_lane * pre_mul_lane + local_lane * local_mul_lane + add_lane;
        unpack_pair(folded, out_state26[i]);
    }

    if (out) {
        memcpy(out->pre_fifth_pairs26, pre_fifth_pairs26, sizeof(out->pre_fifth_pairs26));
        memcpy(out->local3a0_pairs26, local3a0_pairs26, sizeof(out->local3a0_pairs26));
        memcpy(out->state26, out_state26, sizeof(out->state26));
    }
    return 0;
}

int l3_authorization_primary_post_fourth_pre_fifth_state26(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t post_fourth_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t seed13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_fourth_vector_affine_trace *out) {
    if (!fourth_words13 || !post_fourth_param1_13 || !seed13 || !out_state26) {
        return -1;
    }

    uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t param1_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t local718_26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t pre_fifth_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    l3_authorization_primary_post_fourth_staging_trace staging;

    int rc = l3_authorization_primary_post_fourth_pre_fifth_preimage(
        fourth_words13, post_fourth_param1_13, seed13,
        affine_words13, param1_raw13, local718_26, pre_fifth_pairs26, &staging);
    if (rc != 0) {
        return rc;
    }

    uint32_t local3a0_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    rc = l3_authorization_primary_post_fourth_local718_staging_convolution26(
        local718_26, local3a0_pairs26, NULL);
    if (rc != 0) {
        return rc;
    }

    return l3_authorization_primary_post_fourth_vector_affine_state26(
        pre_fifth_pairs26, local3a0_pairs26, out_state26, out);
}

/* v243: fifth FUN_f3fdb57c call-site after the post-fourth vector-affine
 * state image. */
int l3_authorization_primary_fifth_vector_reduce_update_export(
    const uint32_t fourth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t post_fourth_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t seed13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_fifth_vector_reduce_trace *out) {
    if (!fourth_words13 || !post_fourth_param1_13 || !seed13 ||
        !vector_reduce_param1_13 || !out_state26 || !out_words13) {
        return -1;
    }

    l3_authorization_primary_post_fourth_vector_affine_trace local_pre;
    l3_authorization_primary_post_fourth_vector_affine_trace *pre_dst =
        out ? &out->pre_state : &local_pre;

    int rc = l3_authorization_primary_post_fourth_pre_fifth_state26(
        fourth_words13, post_fourth_param1_13, seed13, out_state26, pre_dst);
    if (rc != 0) {
        return rc;
    }

    if (out) {
        memcpy(out->pre_vector_reduce_state26, out_state26, sizeof(out->pre_vector_reduce_state26));
    }

    return l3_authorization_vector_reduce_run13_update_export(
        vector_reduce_param1_13, vector_reduce_param3, vector_reduce_param4,
        out_state26, out_words13, out ? &out->f3fdb57c : NULL);
}


/* v244: post-fifth FUN_f3fdb57c affine stream. */
static void post_fifth_make_affine_words(
    const uint32_t fifth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]) {
    static const uint32_t k_a[4] = {
        0x6e71c307u, 0x23968a7du, 0xffe0d5edu, 0x88dc6f55u
    };
    static const uint32_t k_b[4] = {
        0xf05ba2ffu, 0x19438af1u, 0xbc8c5f71u, 0xec53102fu
    };
    static const uint32_t k_add_a[4] = {
        0x655d1ad0u, 0x175eb40du, 0x19567470u, 0x3369becau
    };
    static const uint32_t k_add_b[4] = {
        0x0288dcccu, 0x4b7bb55fu, 0xba5aa6bau, 0x539126b2u
    };
    for (unsigned i = 0; i < 4u; ++i) {
        out_affine13[i] = fifth_words13[i] * k_a[i] + k_add_a[i];
        out_affine13[i + 4u] =
            fifth_words13[i + 4u] * k_b[i] + k_add_b[i];
        out_affine13[i + 8u] =
            fifth_words13[i + 8u] * k_a[i] + k_add_a[i];
    }
    out_affine13[12] = fifth_words13[12] * 0xf05ba2ffu + 0x0288dcccu;
}

int l3_authorization_primary_post_fifth_affine_words13(
    const uint32_t fifth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_fifth_affine_trace *out) {
    if (!fifth_words13 || !out_affine_words13) {
        return -1;
    }
    post_fifth_make_affine_words(fifth_words13, out_affine_words13);
    if (out) {
        memcpy(out->fifth_words13, fifth_words13, sizeof(out->fifth_words13));
        memcpy(out->affine_words13, out_affine_words13, sizeof(out->affine_words13));
    }
    return 0;
}

/* v245: post-fifth DAT_f41a1030/DAT_f41a10b0 pair-stream staging. */
static const uint32_t k_f4190810[8] = {
    0x777437ddu, 0xeff1c18fu, 0x6dfbcc9fu, 0x22127ccdu,
    0xfba3498fu, 0x254081d5u, 0xc5834b45u, 0xe42dbc65u
};
static const uint32_t k_f4190830[8] = {
    0x119c32ddu, 0x440a6f1du, 0x61927a19u, 0x0d22a844u,
    0xa871353fu, 0xd7a3836bu, 0xb4e4b06cu, 0xf33a0a04u
};
static const uint32_t k_f4190850[8] = {
    0xbdbd898fu, 0x0d50a7d5u, 0x6bacf685u, 0xed15aadfu,
    0xa29bffd5u, 0x5493a4b7u, 0xaac1df87u, 0xb68ed5e7u
};
static const uint32_t k_f4190870[8] = {
    0x357a2f15u, 0x123319d5u, 0x578ee969u, 0x7949d4b2u,
    0x8c5153ebu, 0x055e258fu, 0xa907aaeau, 0x64c413f2u
};
static const uint32_t k_f41a1030[16][2] = {
    {0x2c09d0f5u,0x4666dc18u}, {0xc02b2781u,0xf86cdbd0u},
    {0x544c7e0du,0xaa72db89u}, {0xe86dd499u,0x5c78db41u},
    {0x7c8f2b25u,0x0e7edafau}, {0x10b081b1u,0xb084dab3u},
    {0xa4d1d83du,0x628ada6bu}, {0x38f32ec9u,0x1490da24u},
    {0xcd148555u,0xc696d9dcu}, {0x6135dbe1u,0x789cd995u},
    {0xb341c9aeu,0x2a42ddc4u}, {0x4763203au,0xdc48dd7du},
    {0xdb8476c6u,0x8e4edd35u}, {0x6fa5cd52u,0x3054dceeu},
    {0x03c723deu,0xe25adca7u}, {0x97e87a6au,0x9460dc5fu}
};
static const uint32_t k_f41a10b0[16][2] = {
    {0x290db913u,0x165cdc43u}, {0x30287463u,0x253312d9u},
    {0x638570ecu,0x328c769du}, {0x6aa02c3cu,0x4162ad33u},
    {0x9dfd28c5u,0x5ebc10f7u}, {0xa517e415u,0x6d92478du},
    {0xd874e09eu,0x7aebab51u}, {0x0bd1dd27u,0x88450f16u},
    {0x12ec9877u,0x971b45acu}, {0x46499500u,0xa474a970u},
    {0x4d645050u,0xb34ae006u}, {0x80c14cd9u,0xc0a443cau},
    {0x87dc0829u,0xdf7a7a60u}, {0xbb3904b2u,0xecd3de24u},
    {0xee96013bu,0xfa2d41e8u}, {0xf5b0bc8bu,0x0903787eu}
};

static void post_fifth_f1030_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0x92813615u;
    uint32_t lo = lo0 + 0x9a8ab70bu;
    uint32_t hi = token * 0x034250f3u + multiply_high_u32(token, 0x92813615u) +
                  0x6096566au + add_carry_u32(lo0, 0x9a8ab70bu);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a1030, lo, hi);
    uint32_t lo1 = token * 0xd85671b9u;
    out_pair[0] = lo1 + 0x27eb9420u;
    out_pair[1] = token * 0x51eeae36u + multiply_high_u32(token, 0xd85671b9u) +
                  ladder * 0xf32485ebu + 0x8ee7d1d5u +
                  add_carry_u32(lo1, 0x27eb9420u);
}

static void post_fifth_f10b0_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0xe3ae092bu;
    uint32_t lo = lo0 + 0x9b74680fu;
    uint32_t hi = token * 0x93fa9377u + multiply_high_u32(token, 0xe3ae092bu) +
                  0xd15e6c3eu + add_carry_u32(lo0, 0x9b74680fu);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a10b0, lo, hi);
    uint32_t lo1 = token * 0xce95b50du;
    out_pair[0] = lo1 + 0x26838630u;
    out_pair[1] = token * 0xd087f9e7u + multiply_high_u32(token, 0xce95b50du) +
                  ladder * 0x7cc7d159u + 0xce3af3d6u +
                  add_carry_u32(lo1, 0x26838630u);
}

int l3_authorization_primary_post_fifth_staging_pair_streams13(
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1030_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1030_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f10b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f10b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_fifth_staging_trace *out) {
    if (!affine_words13 || !out_f1030_raw13 || !out_f1030_prefix13 ||
        !out_f10b0_raw13 || !out_f10b0_prefix13) {
        return -1;
    }

    uint32_t pair[2];
    uint32_t prefix[2];

    uint32_t token = affine_words13[0] * 0x25039f0du + 0xd5d68161u;
    post_fifth_f1030_pair_from_token(token, pair);
    out_f1030_raw13[0][0] = pair[0];
    out_f1030_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_f1030_prefix13[0][0] = prefix[0];
    out_f1030_prefix13[0][1] = prefix[1];

    for (unsigned i = 1u; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = (affine_words13[i] * k_f4190810[lane] +
                 k_f4190830[lane]) * 0x859aa8f1u + 0x56779754u;
        post_fifth_f1030_pair_from_token(token, pair);
        out_f1030_raw13[i][0] = pair[0];
        out_f1030_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_f1030_prefix13[i][0] = prefix[0];
        out_f1030_prefix13[i][1] = prefix[1];
    }

    token = affine_words13[0] * 0x98888de5u + 0x27fc6150u;
    post_fifth_f10b0_pair_from_token(token, pair);
    out_f10b0_raw13[0][0] = pair[0];
    out_f10b0_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_f10b0_prefix13[0][0] = prefix[0];
    out_f10b0_prefix13[0][1] = prefix[1];

    for (unsigned i = 1u; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = (affine_words13[i] * k_f4190850[lane] +
                 k_f4190870[lane]) * 0x0e402f4bu + 0x234bbb29u;
        post_fifth_f10b0_pair_from_token(token, pair);
        out_f10b0_raw13[i][0] = pair[0];
        out_f10b0_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_f10b0_prefix13[i][0] = prefix[0];
        out_f10b0_prefix13[i][1] = prefix[1];
    }

    if (out) {
        memcpy(out->affine_words13, affine_words13, sizeof(out->affine_words13));
        memcpy(out->f1030_raw13, out_f1030_raw13, sizeof(out->f1030_raw13));
        memcpy(out->f1030_prefix13, out_f1030_prefix13, sizeof(out->f1030_prefix13));
        memcpy(out->f10b0_raw13, out_f10b0_raw13, sizeof(out->f10b0_raw13));
        memcpy(out->f10b0_prefix13, out_f10b0_prefix13, sizeof(out->f10b0_prefix13));
    }
    return 0;
}

int l3_authorization_primary_post_fifth_affine_staging_pair_streams13(
    const uint32_t fifth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1030_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1030_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f10b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f10b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_fifth_staging_trace *out) {
    if (!fifth_words13 || !out_affine_words13 || !out_f1030_raw13 ||
        !out_f1030_prefix13 || !out_f10b0_raw13 || !out_f10b0_prefix13) {
        return -1;
    }
    int rc = l3_authorization_primary_post_fifth_affine_words13(
        fifth_words13, out_affine_words13, NULL);
    if (rc != 0) {
        return rc;
    }
    return l3_authorization_primary_post_fifth_staging_pair_streams13(
        out_affine_words13, out_f1030_raw13, out_f1030_prefix13,
        out_f10b0_raw13, out_f10b0_prefix13, out);
}


/* v246: post-fifth 26-pair convolution over staged DAT_f41a1030/DAT_f41a10b0
 * streams.  The constants and range-correction shape correspond to the native
 * loop that writes local_578 immediately before the next FUN_f3fdb57c call.
 */
int l3_authorization_primary_post_fifth_staging_convolution26(
    const uint32_t f1030_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f1030_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f10b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f10b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]) {
    if (!f1030_raw13 || !f1030_prefix13 || !f10b0_raw13 ||
        !f10b0_prefix13 || !out_pairs26) {
        return -1;
    }

    for (unsigned idx = 0u; idx < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0xa779b326u;
        uint32_t v = 0xad85077eu; /* -0x527af882 */
        if (first <= last) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned k = first; k <= last; ++k) {
                const uint32_t *a = f10b0_raw13[k];
                const uint32_t *b = f1030_raw13[idx - k];
                uint64_t prod = (uint64_t)a[0] * (uint64_t)b[0];
                uint32_t old = conv_lo;
                conv_lo += (uint32_t)prod;
                conv_hi += b[1] * a[0] + b[0] * a[1] +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv_lo < old);
            }

            uint32_t f1030_sum[2];
            pair_range_sum(f1030_prefix13, first, last, f1030_sum);
            uint32_t f10b0_sum[2];
            pair_range_sum(f10b0_prefix13, idx - last, idx - first, f10b0_sum);

            uint32_t len = (last - first) + 1u;
            uint64_t len_prod = (uint64_t)len * 0x33e95b1eu;
            uint32_t len_lo = (uint32_t)len_prod;
            uint32_t len_hi = (uint32_t)(len_prod >> 32);
            uint32_t conv_scaled_lo = conv_lo * 0xe6057d05u;
            uint32_t f1030_scaled_lo = f1030_sum[0] * 0xd2e500c3u;
            uint32_t f10b0_scaled_lo = f10b0_sum[0] * 0x3a9a22b2u;

            uint32_t t0 = len_lo + 0xa779b326u;
            uint32_t t1 = t0 + conv_scaled_lo;
            uint32_t t2 = t1 + f1030_scaled_lo;
            u = t2 + f10b0_scaled_lo;

            v = len * 0x71dee661u + len_hi + 0xad85077eu +
                add_carry_u32(len_lo, 0xa779b326u) +
                conv_hi * 0xe6057d05u + conv_lo * 0x3e2d6d87u +
                multiply_high_u32(conv_lo, 0xe6057d05u) + add_carry_u32(t0, conv_scaled_lo) +
                f1030_sum[1] * 0xd2e500c3u + f1030_sum[0] * 0x4883881du +
                multiply_high_u32(f1030_sum[0], 0xd2e500c3u) + add_carry_u32(t1, f1030_scaled_lo) +
                f10b0_sum[1] * 0x3a9a22b2u + f10b0_sum[0] * 0x2ab63205u +
                multiply_high_u32(f10b0_sum[0], 0x3a9a22b2u) + add_carry_u32(t2, f10b0_scaled_lo);
        }

        uint32_t out_lo = u * 0xf602c693u;
        out_pairs26[idx][0] = out_lo + 0x40a24a4eu;
        out_pairs26[idx][1] = v * 0xf602c693u + u * 0xa7052c91u +
                              multiply_high_u32(u, 0xf602c693u) + 0x56a05b32u +
                              add_carry_u32(out_lo, 0x40a24a4eu);
    }
    return 0;
}

int l3_authorization_primary_post_fifth_affine_staging_convolution26(
    const uint32_t fifth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_fifth_convolution_trace *out) {
    if (!fifth_words13 || !out_affine_words13 || !out_pairs26) {
        return -1;
    }

    l3_authorization_primary_post_fifth_convolution_trace tmp;
    l3_authorization_primary_post_fifth_convolution_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_fifth_affine_staging_pair_streams13(
        fifth_words13, out_affine_words13, dst->staging.f1030_raw13,
        dst->staging.f1030_prefix13, dst->staging.f10b0_raw13,
        dst->staging.f10b0_prefix13, &dst->staging);
    if (rc != 0) {
        return rc;
    }
    rc = l3_authorization_primary_post_fifth_staging_convolution26(
        dst->staging.f1030_raw13, dst->staging.f1030_prefix13,
        dst->staging.f10b0_raw13, dst->staging.f10b0_prefix13, out_pairs26);
    if (rc != 0) {
        return rc;
    }
    if (out) {
        memcpy(out->conv_pairs26, out_pairs26, sizeof(out->conv_pairs26));
    }
    return 0;
}

/* v247: sixth FUN_f3fdb57c call-site after the post-fifth staged-stream
 * convolution. */
int l3_authorization_primary_sixth_vector_reduce_update_export(
    const uint32_t fifth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_sixth_vector_reduce_trace *out) {
    if (!fifth_words13 || !vector_reduce_param1_13 || !out_affine_words13 ||
        !out_state26 || !out_words13) {
        return -1;
    }

    l3_authorization_primary_post_fifth_convolution_trace local_pre;
    l3_authorization_primary_post_fifth_convolution_trace *pre_dst = out ? &out->pre_state : &local_pre;

    int rc = l3_authorization_primary_post_fifth_affine_staging_convolution26(
        fifth_words13, out_affine_words13, out_state26, pre_dst);
    if (rc != 0) {
        return rc;
    }

    if (out) {
        memcpy(out->pre_vector_reduce_state26, out_state26, sizeof(out->pre_vector_reduce_state26));
    }

    return l3_authorization_vector_reduce_run13_update_export(
        vector_reduce_param1_13, vector_reduce_param3, vector_reduce_param4,
        out_state26, out_words13, out ? &out->f3fdb57c : NULL);
}


/* v261: q-register affine source feeding the v248 DAT_f41a1130 param_5 ladder. */
static const uint32_t k_f3fc9d60[4] = {
    0x90894052u, 0x73f59e0eu, 0xcfd77ebau, 0x2d40dfc2u
};
static const uint32_t k_f3fc9d70[4] = {
    0xe4f37827u, 0x1397788fu, 0x745a5d6fu, 0x4a4e9825u
};
static const uint32_t k_f3fc9d80[4] = {
    0x18192756u, 0x34b8c316u, 0xba63fcfau, 0x4bb87fb2u
};
static const uint32_t k_f3fc9d90[4] = {
    0x80458909u, 0xababd89fu, 0x5530147bu, 0x089c318du
};
static const uint32_t k_f3fc9da0[4] = {
    0x7a5f3805u, 0xd5b50ca9u, 0x260a8c59u, 0xaed70aa7u
};
static const uint32_t k_f3fc9db0[4] = {
    0xbd397c07u, 0xbb1f12ddu, 0x5fc9668fu, 0x61df5ec7u
};
static const uint32_t k_f3fc9dc0[4] = {
    0xea1c4e6fu, 0xf25a1c4au, 0xaf7983bfu, 0xcc7f3e18u
};
static const uint32_t k_f3fc9dd0[4] = {
    0x9115388cu, 0x0866abe4u, 0x01690f10u, 0xd06bad2fu
};

static uint32_t affine3(uint32_t a, uint32_t ka,
                             uint32_t b, uint32_t kb,
                             uint32_t c, uint32_t kc,
                             uint32_t add) {
    return a * ka + b * kb + c * kc + add;
}

int l3_authorization_primary_post_sixth_param5_source_words13(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param4,
    uint32_t out_source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_param5_source_trace *out) {
    if (!live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !sixth_words13 || !param4_words13 || !out_source_words13) {
        return -1;
    }

    for (unsigned i = 0; i < 4u; ++i) {
        out_source_words13[i] = affine3(
            live_q6_words4[i], k_f3fc9d60[i],
            sixth_words13[i], k_f3fc9d70[i],
            param4_words13[i], k_f3fc9d90[i],
            k_f3fc9dc0[i]);
        out_source_words13[4u + i] = affine3(
            live_q5_words4[i], k_f3fc9d80[i],
            sixth_words13[4u + i], k_f3fc9da0[i],
            param4_words13[4u + i], k_f3fc9db0[i],
            k_f3fc9dd0[i]);
        out_source_words13[8u + i] = affine3(
            live_q4_words4[i], k_f3fc9d60[i],
            sixth_words13[8u + i], k_f3fc9d70[i],
            param4_words13[8u + i], k_f3fc9d90[i],
            k_f3fc9dc0[i]);
    }
    out_source_words13[12] =
        sixth_words13[12] * 0x7a5f3805u +
        vector_reduce_param4 * 0x18192756u +
        param4_words13[12] * 0xbd397c07u +
        0x9115388cu;

    if (out) {
        memcpy(out->live_q6_words4, live_q6_words4, sizeof(out->live_q6_words4));
        memcpy(out->live_q5_words4, live_q5_words4, sizeof(out->live_q5_words4));
        memcpy(out->live_q4_words4, live_q4_words4, sizeof(out->live_q4_words4));
        memcpy(out->sixth_words13, sixth_words13, sizeof(out->sixth_words13));
        memcpy(out->param4_words13, param4_words13, sizeof(out->param4_words13));
        out->vector_reduce_param4 = vector_reduce_param4;
        memcpy(out->source_words13, out_source_words13, sizeof(out->source_words13));
    }
    return 0;
}

/* v248: post-sixth DAT_f41a1130 ladder stream feeding param_5. */
static const uint32_t k_f4190890[8] = {
    0x55de5855u, 0x9793fab3u, 0xd1e4d1f1u, 0xf14bcc01u,
    0x51afa9a9u, 0xb2fefc8du, 0x9a7d34bbu, 0xccf3e0bdu
};
static const uint32_t k_f41908b0[8] = {
    0xe1c9be27u, 0x9749b6e6u, 0xc32c9fabu, 0xbd3d4503u,
    0xadc318a3u, 0x1ff60dafu, 0x0e57c503u, 0xacfbfe80u
};
static const uint32_t k_f41a1130[16] = {
    0x5c69d367u, 0x0de6d45au, 0xb21e7ae8u, 0x639b7bdbu,
    0x15187cceu, 0xc6957dc1u, 0x78127eb4u, 0x2c4a2542u,
    0xddc72635u, 0x8f442728u, 0x30c1281bu, 0xe23e290eu,
    0x93bb2a01u, 0x47f2d08fu, 0xf96fd182u, 0xaaecd275u
};

int l3_authorization_primary_post_sixth_param5_ladder_words13(
    const uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_param5_ladder_trace *out) {
    if (!source_words13 || !out_param5_words13) {
        return -1;
    }

    uint32_t rolling = 0xd2665524u; /* -0x2d99aadc */
    uint32_t ladder = 0u;
    for (unsigned lane = 0; lane < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++lane) {
        uint32_t mixed = rolling * 0xd867357bu + source_words13[lane]; /* -0x2798ca85 */
        ladder = mixed * 0x0e823315u + 0xd4114aefu;
        uint32_t next = ladder32_run7_then_next(k_f41a1130, &ladder);
        out_param5_words13[lane] =
            (mixed * 0x760d5245u + ladder * 0xf0000000u + 0xd79e066eu) *
            k_f4190890[lane & 7u] + k_f41908b0[lane & 7u];
        if (lane != L3_AUTH_ROUND_PRIMARY_SEED_WORDS - 1u) {
            rolling = ladder * 0xcf0a93a7u + next * 0x0f56c590u + 0xbf4f438bu;
        }
    }

    if (out) {
        memcpy(out->source_words13, source_words13, sizeof(out->source_words13));
        memcpy(out->param5_words13, out_param5_words13, sizeof(out->param5_words13));
        out->final_ladder_word = ladder;
        out->final_rolling_word = rolling;
    }
    return 0;
}


int l3_authorization_primary_post_sixth_param5_source_ladder_words13(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param4,
    uint32_t out_source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_param5_source_ladder_trace *out) {
    if (!out_source_words13 || !out_param5_words13) {
        return -1;
    }
    l3_authorization_primary_post_sixth_param5_source_ladder_trace tmp;
    l3_authorization_primary_post_sixth_param5_source_ladder_trace *dst = out ? out : &tmp;

    int rc = l3_authorization_primary_post_sixth_param5_source_words13(
        live_q6_words4, live_q5_words4, live_q4_words4,
        sixth_words13, param4_words13, vector_reduce_param4,
        out_source_words13, &dst->source);
    if (rc != 0) {
        return rc;
    }
    return l3_authorization_primary_post_sixth_param5_ladder_words13(
        out_source_words13, out_param5_words13, &dst->ladder);
}

/* v249: post-sixth DAT_f41a1170 / DAT_f41a11b0 staging. */
static const uint32_t k_f41908d0[8] = {
    0xa7c699cbu, 0x7da49d67u, 0x51290527u, 0x9431ebdfu,
    0xbcfc69e9u, 0x60ad93b3u, 0xffb98155u, 0x3bba964du
};
static const uint32_t k_f41908f0[8] = {
    0x48c2257eu, 0x663058d2u, 0x38c5af05u, 0x70a8c89cu,
    0x8750561bu, 0x0aa0ae5fu, 0xd34cc8c4u, 0x86500e79u
};
static const uint32_t k_f4190910[8] = {
    0x628a224bu, 0xf238faa9u, 0x1e8d4c19u, 0xe92400dbu,
    0xe615b2a9u, 0xd2330613u, 0x2fb7a423u, 0xdecf8403u
};
static const uint32_t k_f4190930[8] = {
    0x1ba7aebfu, 0xbda7647fu, 0x3b9aa063u, 0x3b086a50u,
    0x3ba0bfedu, 0x76d65e21u, 0x361ecd68u, 0x20af8690u
};
static const uint32_t k_f41a1170[16] = {
    0xada0f4eeu, 0xf53c8eecu, 0x4cd828eau, 0x9473c2e8u,
    0xed50a353u, 0x34ec3d51u, 0x8c87d74fu, 0xd564b7bau,
    0x2d0051b8u, 0x749bebb6u, 0xcd78cc21u, 0x1514661fu,
    0x6cb0001du, 0xb58ce088u, 0x0d287a86u, 0x54c41484u
};
static const uint32_t k_f41a11b0[16][2] = {
    {0xd43d320fu,0x6b0a3788u}, {0xb4b1d6bfu,0x363142acu},
    {0xda4e4612u,0x011f836du}, {0xffeab565u,0xdc0dc42eu},
    {0xe05f5a15u,0xa734cf52u}, {0x05fbc968u,0x72231014u},
    {0x2b9838bbu,0x4d1150d5u}, {0x0c0cdd6bu,0x18385bf9u},
    {0x31a94cbeu,0xe3269cbau}, {0x5745bc11u,0xbe14dd7bu},
    {0x7ce22b64u,0x89031e3cu}, {0x5d56d014u,0x542a2960u},
    {0x82f33f67u,0x2f186a21u}, {0xa88faebau,0xfa06aae2u},
    {0x8904536au,0xc52db606u}, {0xaea0c2bdu,0x901bf6c7u}
};

static void post_sixth_f11b0_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0xb714cad3u;
    uint32_t lo = lo0 + 0x262c62ecu;
    uint32_t hi = token * 0x2e3d3f04u + multiply_high_u32(token, 0xb714cad3u) +
                  0x07f583a5u + add_carry_u32(lo0, 0x262c62ecu);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a11b0, lo, hi);

    uint32_t lo1 = token * 0xc92bc3f7u;
    out_pair[0] = lo1 + 0x227c913fu;
    out_pair[1] = token * 0x30d6c7cfu + multiply_high_u32(token, 0xc92bc3f7u) +
                  ladder * 0x90845c33u + 0x455a57e6u +
                  add_carry_u32(lo1, 0x227c913fu);
}

int l3_authorization_primary_post_sixth_f1170_ladder_words13(
    const uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_staging_trace *out) {
    if (!source_words13 || !out_f1170_words13) {
        return -1;
    }

    uint32_t rolling = 0xf235ef0bu; /* -0x0dca10f5 */
    uint32_t ladder = 0u;
    for (unsigned lane = 0; lane < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++lane) {
        uint32_t mixed = rolling * 0x6a28b523u + source_words13[lane];
        ladder = mixed * 0x022a1469u + 0xa706682du;
        uint32_t next = ladder32_run7_then_next(k_f41a1170, &ladder);
        out_f1170_words13[lane] =
            (mixed * 0xa0560919u + ladder * 0xf0000000u + 0xb2f13580u) *
            k_f41908d0[lane & 7u] + k_f41908f0[lane & 7u];
        if (lane != L3_AUTH_ROUND_PRIMARY_SEED_WORDS - 1u) {
            rolling = ladder * 0xb391d0d3u + next * 0xc6e2f2d0u + 0x03b512f3u;
        }
    }

    if (out) {
        memcpy(out->source_words13, source_words13, sizeof(out->source_words13));
        memcpy(out->f1170_words13, out_f1170_words13, sizeof(out->f1170_words13));
        out->final_ladder_word = ladder;
        out->final_rolling_word = rolling;
    }
    return 0;
}

int l3_authorization_primary_post_sixth_f11b0_pair_streams13(
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]) {
    if (!affine_words13 || !out_f11b0_raw13 || !out_f11b0_prefix13) {
        return -1;
    }

    uint32_t prefix[2] = {0u, 0u};
    for (unsigned lane = 0; lane < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++lane) {
        unsigned tab_lane = lane & 7u;
        uint32_t token = (affine_words13[lane] * k_f4190910[tab_lane] +
                          k_f4190930[tab_lane]) * 0xb820ae5du + 0x48cee620u;
        uint32_t pair[2];
        post_sixth_f11b0_pair_from_token(token, pair);
        out_f11b0_raw13[lane][0] = pair[0];
        out_f11b0_raw13[lane][1] = pair[1];
        pair64_add(prefix, pair);
        out_f11b0_prefix13[lane][0] = prefix[0];
        out_f11b0_prefix13[lane][1] = prefix[1];
    }
    return 0;
}

int l3_authorization_primary_post_sixth_staging_pair_streams13(
    const uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_sixth_staging_trace *out) {
    if (!source_words13 || !affine_words13 || !out_f1170_words13 ||
        !out_f11b0_raw13 || !out_f11b0_prefix13) {
        return -1;
    }

    l3_authorization_primary_post_sixth_staging_trace tmp;
    l3_authorization_primary_post_sixth_staging_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_sixth_f1170_ladder_words13(
        source_words13, out_f1170_words13, dst);
    if (rc != 0) {
        return rc;
    }
    rc = l3_authorization_primary_post_sixth_f11b0_pair_streams13(
        affine_words13, out_f11b0_raw13, out_f11b0_prefix13);
    if (rc != 0) {
        return rc;
    }

    if (out) {
        memcpy(out->affine_words13, affine_words13, sizeof(out->affine_words13));
        memcpy(out->f11b0_raw13, out_f11b0_raw13, sizeof(out->f11b0_raw13));
        memcpy(out->f11b0_prefix13, out_f11b0_prefix13, sizeof(out->f11b0_prefix13));
    }
    return 0;
}



/* v262: q-register affine source feeding the v249 DAT_f41a1170 staging
 * stream.  This is the native block at f3fc9e60..f3fc9f80: it reuses the
 * live q6/q5/q4 vectors, the v248 param_5 words, and the next param_4 object
 * slice to materialize local_130..local_100 before the f1170 ladder.
 */
static const uint32_t k_f3fc9de0[4] = {
    0x02c2ed5du, 0x75fa1193u, 0xe77444a1u, 0xc316a775u
};
static const uint32_t k_f3fc9df0[4] = {
    0x9fcfa0dfu, 0xa0180c6bu, 0x78208f43u, 0x03e7f80fu
};
static const uint32_t k_f3fc9e00[4] = {
    0x0d5e5b07u, 0x48fefe67u, 0x5094b241u, 0x5ef8e6cdu
};
static const uint32_t k_f3fc9e10[4] = {
    0x65582c4du, 0xc11268abu, 0xb72c75a1u, 0x0a0ca191u
};
static const uint32_t k_f3fc9e20[4] = {
    0xf7415f79u, 0x53032c73u, 0xad46fe0fu, 0xfd12ab4du
};
static const uint32_t k_f3fc9e30[4] = {
    0x87d2d5deu, 0x40eb4d58u, 0x19a24189u, 0xcde64ef9u
};
static const uint32_t k_f3fc9e40[4] = {
    0x44ace5a9u, 0x5a6fce15u, 0x20e76823u, 0x5a553f65u
};
static const uint32_t k_f3fc9e50[4] = {
    0x2f1fc586u, 0x7252b68fu, 0xb157ce18u, 0xc71a283cu
};

int l3_authorization_primary_post_sixth_staging_source_words13(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_next_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param4,
    uint32_t out_source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_staging_source_trace *out) {
    if (!live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !param5_words13 || !param4_next_words13 || !out_source_words13) {
        return -1;
    }

    for (unsigned i = 0; i < 4u; ++i) {
        out_source_words13[i] = affine3(
            live_q6_words4[i], k_f3fc9de0[i],
            param4_next_words13[i], k_f3fc9df0[i],
            param5_words13[i], k_f3fc9e10[i],
            k_f3fc9e30[i]);
        out_source_words13[4u + i] = affine3(
            live_q5_words4[i], k_f3fc9e00[i],
            param4_next_words13[4u + i], k_f3fc9e20[i],
            param5_words13[4u + i], k_f3fc9e40[i],
            k_f3fc9e50[i]);
        out_source_words13[8u + i] = affine3(
            live_q4_words4[i], k_f3fc9de0[i],
            param4_next_words13[8u + i], k_f3fc9df0[i],
            param5_words13[8u + i], k_f3fc9e10[i],
            k_f3fc9e30[i]);
    }
    out_source_words13[12] =
        param5_words13[12] * 0x44ace5a9u +
        vector_reduce_param4 * 0x0d5e5b07u +
        param4_next_words13[12] * 0xf7415f79u +
        0x2f1fc586u;

    if (out) {
        memcpy(out->live_q6_words4, live_q6_words4, sizeof(out->live_q6_words4));
        memcpy(out->live_q5_words4, live_q5_words4, sizeof(out->live_q5_words4));
        memcpy(out->live_q4_words4, live_q4_words4, sizeof(out->live_q4_words4));
        memcpy(out->param5_words13, param5_words13, sizeof(out->param5_words13));
        memcpy(out->param4_next_words13, param4_next_words13, sizeof(out->param4_next_words13));
        out->vector_reduce_param4 = vector_reduce_param4;
        memcpy(out->source_words13, out_source_words13, sizeof(out->source_words13));
    }
    return 0;
}

int l3_authorization_primary_post_sixth_staging_source_pair_streams13(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_next_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param4,
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_sixth_staging_source_pair_streams_trace *out) {
    if (!affine_words13 || !out_source_words13 || !out_f1170_words13 ||
        !out_f11b0_raw13 || !out_f11b0_prefix13) {
        return -1;
    }

    l3_authorization_primary_post_sixth_staging_source_pair_streams_trace tmp;
    l3_authorization_primary_post_sixth_staging_source_pair_streams_trace *dst = out ? out : &tmp;

    int rc = l3_authorization_primary_post_sixth_staging_source_words13(
        live_q6_words4, live_q5_words4, live_q4_words4,
        param5_words13, param4_next_words13, vector_reduce_param4,
        out_source_words13, &dst->source);
    if (rc != 0) {
        return rc;
    }
    return l3_authorization_primary_post_sixth_staging_pair_streams13(
        out_source_words13, affine_words13, out_f1170_words13,
        out_f11b0_raw13, out_f11b0_prefix13, &dst->staging);
}

/* v250: post-sixth DAT_f41a1230 companion stream and pair convolution.
 * This is the bounded convolution image immediately after the v249
 * DAT_f41a1170/DAT_f41a11b0 staging.  It stops at the native 26-pair
 * accumulator/range-correction image; the following DAT_f41a12b0 scalar
 * word transform remains a separate seam.
 */
static const uint32_t k_f4190950[8] = {
    0x1045d955u, 0x09e18a01u, 0x93e857c1u, 0x75183979u,
    0x5adf80cfu, 0xe34d677du, 0x96c141cbu, 0xece42283u
};
static const uint32_t k_f4190970[8] = {
    0xba165dc5u, 0x86a711c9u, 0xf83ef8d6u, 0x803910dfu,
    0x5ac27ec6u, 0xe91a4138u, 0x2ea9a72fu, 0xa2a324b0u
};
static const uint32_t k_f41a1230[16][2] = {
    {0xae0aa4f2u,0x9f52298bu}, {0x3bb415fdu,0x8849cffeu},
    {0xc95d8708u,0x71417670u}, {0x5706f813u,0x6a391ce3u},
    {0xe4b0691eu,0x5330c355u}, {0x7259da29u,0x4c2869c8u},
    {0x00034b34u,0x3520103bu}, {0x8dacbc3fu,0x2e17b6adu},
    {0x40bf1c9bu,0x1794f5f7u}, {0xce688da6u,0x008c9c69u},
    {0x5c11feb1u,0xf98442dcu}, {0xe9bb6fbcu,0xe27be94eu},
    {0x7764e0c7u,0xdb738fc1u}, {0x050e51d2u,0xc46b3634u},
    {0x92b7c2ddu,0xbd62dca6u}, {0x206133e8u,0xa65a8319u}
};

static void post_sixth_f1230_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = token * 0x5a51d6cfu;
    uint32_t lo = lo0 + 0x126910e6u;
    uint32_t hi = token * 0x213903fau + multiply_high_u32(token, 0x5a51d6cfu) +
                  0xc1928010u + add_carry_u32(lo0, 0x126910e6u);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a1230, lo, hi);

    uint32_t lo1 = token * 0x3e28fb9du;
    out_pair[0] = lo1 + 0x30e0e845u;
    out_pair[1] = token * 0xc31f9a4du + multiply_high_u32(token, 0x3e28fb9du) +
                  ladder * 0x23381e2du + 0x44546ca4u +
                  add_carry_u32(lo1, 0x30e0e845u);
}

int l3_authorization_primary_post_sixth_f1230_pair_streams13(
    const uint32_t f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1230_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1230_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]) {
    if (!f1170_words13 || !out_f1230_raw13 || !out_f1230_prefix13) {
        return -1;
    }

    uint32_t prefix[2] = {0u, 0u};
    for (unsigned lane = 0; lane < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++lane) {
        unsigned tab_lane = lane & 7u;
        uint32_t token = (f1170_words13[lane] * k_f4190950[tab_lane] +
                          k_f4190970[tab_lane]) * 0xe15c9823u + 0x15c0e91fu;
        uint32_t pair[2];
        post_sixth_f1230_pair_from_token(token, pair);
        out_f1230_raw13[lane][0] = pair[0];
        out_f1230_raw13[lane][1] = pair[1];
        pair64_add(prefix, pair);
        out_f1230_prefix13[lane][0] = prefix[0];
        out_f1230_prefix13[lane][1] = prefix[1];
    }
    return 0;
}

static void post_sixth_f12b0_scalar_from_pair(
    const uint32_t pair[2], unsigned lane, uint32_t *out_word,
    uint32_t out_mid_pair[2], uint32_t out_next_pair[2], uint32_t *out_tail_low);

int l3_authorization_primary_post_sixth_staging_convolution26(
    const uint32_t f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f1230_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t f1230_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]) {
    if (!f11b0_raw13 || !f11b0_prefix13 || !f1230_raw13 || !f1230_prefix13 ||
        !out_pairs26) {
        return -1;
    }

    uint32_t rolling_base[2] = {0xd397bc91u, 0x3a104980u};
    for (unsigned idx = 0; idx < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = rolling_base[0];
        uint32_t v = rolling_base[1];
        if (first <= last) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned k = first; k <= last; ++k) {
                const uint32_t *a = f1230_raw13[k];
                const uint32_t *b = f11b0_raw13[idx - k];
                uint64_t prod = (uint64_t)a[0] * (uint64_t)b[0];
                uint32_t old = conv_lo;
                conv_lo += (uint32_t)prod;
                conv_hi += b[1] * a[0] + b[0] * a[1] +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv_lo < old);
            }

            uint32_t f11b0_sum[2];
            pair_range_sum(f11b0_prefix13, first, last, f11b0_sum);
            uint32_t f1230_sum[2];
            pair_range_sum(f1230_prefix13, idx - last, idx - first, f1230_sum);

            uint32_t len = (last - first) + 1u;
            uint64_t len_prod = (uint64_t)len * 0xe29cdb98u;
            uint32_t len_lo = (uint32_t)len_prod;
            uint32_t len_hi = (uint32_t)(len_prod >> 32);
            uint32_t conv_scaled_lo = conv_lo * 0xa2bfef29u;
            uint32_t f11b0_scaled_lo = f11b0_sum[0] * 0xab592a48u;
            uint32_t f1230_scaled_lo = f1230_sum[0] * 0x50378e13u;

            uint32_t t0 = len_lo + rolling_base[0];
            uint32_t t1 = t0 + conv_scaled_lo;
            uint32_t t2 = t1 + f11b0_scaled_lo;
            u = t2 + f1230_scaled_lo;

            v = len * 0x2fc32b70u + len_hi + rolling_base[1] +
                add_carry_u32(len_lo, rolling_base[0]) +
                conv_hi * 0xa2bfef29u + conv_lo * 0x70e1283bu +
                multiply_high_u32(conv_lo, 0xa2bfef29u) + add_carry_u32(t0, conv_scaled_lo) +
                f11b0_sum[1] * 0xab592a48u + f11b0_sum[0] * 0xf01ddbb4u +
                multiply_high_u32(f11b0_sum[0], 0xab592a48u) + add_carry_u32(t1, f11b0_scaled_lo) +
                f1230_sum[1] * 0x50378e13u + f1230_sum[0] * 0xf1797223u +
                multiply_high_u32(f1230_sum[0], 0x50378e13u) + add_carry_u32(t2, f1230_scaled_lo);
        }

        out_pairs26[idx][0] = u;
        out_pairs26[idx][1] = v;

        uint32_t discarded_word;
        uint32_t discarded_mid[2];
        uint32_t next_pair[2];
        uint32_t discarded_tail;
        post_sixth_f12b0_scalar_from_pair(
            out_pairs26[idx], idx, &discarded_word, discarded_mid, next_pair,
            &discarded_tail);
        rolling_base[0] = next_pair[0];
        rolling_base[1] = next_pair[1];
    }
    return 0;
}

int l3_authorization_primary_post_sixth_companion_convolution26(
    const uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1230_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1230_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_sixth_companion_trace *out) {
    if (!source_words13 || !affine_words13 || !out_f1170_words13 ||
        !out_f11b0_raw13 || !out_f11b0_prefix13 || !out_f1230_raw13 ||
        !out_f1230_prefix13 || !out_pairs26) {
        return -1;
    }

    l3_authorization_primary_post_sixth_companion_trace tmp;
    l3_authorization_primary_post_sixth_companion_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_sixth_staging_pair_streams13(
        source_words13, affine_words13, out_f1170_words13,
        out_f11b0_raw13, out_f11b0_prefix13, &dst->staging);
    if (rc != 0) {
        return rc;
    }
    rc = l3_authorization_primary_post_sixth_f1230_pair_streams13(
        out_f1170_words13, out_f1230_raw13, out_f1230_prefix13);
    if (rc != 0) {
        return rc;
    }
    rc = l3_authorization_primary_post_sixth_staging_convolution26(
        out_f11b0_raw13, out_f11b0_prefix13, out_f1230_raw13,
        out_f1230_prefix13, out_pairs26);
    if (rc != 0) {
        return rc;
    }

    if (out) {
        memcpy(out->f1230_raw13, out_f1230_raw13, sizeof(out->f1230_raw13));
        memcpy(out->f1230_prefix13, out_f1230_prefix13, sizeof(out->f1230_prefix13));
        memcpy(out->conv_pairs26, out_pairs26, sizeof(out->conv_pairs26));
    }
    return 0;
}

/* v251: DAT_f41a12b0 scalar output transform over the v250 pair image. */
static const uint32_t k_f4190990[8] = {
    0xa9085df5u, 0x67ea8585u, 0xc0e90223u, 0x2209ddf1u,
    0x479f9041u, 0x2752c727u, 0x1734ea57u, 0x1d47ca89u
};
static const uint32_t k_f41909b0[8] = {
    0x3c5f76bdu, 0x59257f08u, 0x260b2310u, 0x09601032u,
    0xf42d9829u, 0xa8f4f48cu, 0x5169a82eu, 0xe5992feeu
};
static const uint32_t k_f41a12b0[16][2] = {
    {0x64d39feau,0x17be53d6u}, {0x92d0f4beu,0x8e7462e0u},
    {0xc0ce4992u,0xf52a71eau}, {0xf53a8ef3u,0x6999ea6du},
    {0x2337e3c7u,0xd04ff978u}, {0x5135389bu,0x47060882u},
    {0x7f328d6fu,0xbdbc178cu}, {0xb39ed2d0u,0x222b900fu},
    {0xe19c27a4u,0x98e19f19u}, {0x0f997c78u,0x0f97ae24u},
    {0x4405c1d9u,0x740726a7u}, {0x720316adu,0xeabd35b1u},
    {0xa0006b81u,0x517344bbu}, {0xd46cb0e2u,0xc5e2bd3eu},
    {0x026a05b6u,0x3c98cc49u}, {0x30675a8au,0xa34edb53u}
};

static void ladder64_step(const uint32_t tab[16][2], uint32_t *lo, uint32_t *hi) {
    uint32_t xlo = *lo;
    uint32_t xhi = *hi;
    unsigned nib = xlo & 0x0fu;
    uint32_t shifted = (xlo >> 4) | (xhi << 28);
    uint32_t tlo = tab[nib][0];
    uint32_t nlo = shifted + tlo;
    uint32_t nhi = (xhi >> 4) + tab[nib][1] + add_carry_u32(shifted, tlo);
    *lo = nlo;
    *hi = nhi;
}

static void post_sixth_f12b0_scalar_from_pair(
    const uint32_t pair[2], unsigned lane, uint32_t *out_word,
    uint32_t out_mid_pair[2], uint32_t out_next_pair[2], uint32_t *out_tail_low) {
    uint32_t in_lo = pair[0];
    uint32_t in_hi = pair[1];

    uint32_t lo0 = in_lo * 0x45e7fd31u;
    uint32_t lo = lo0 + 0xa33ccbb5u;
    uint32_t hi = in_hi * 0x45e7fd31u + in_lo * 0x8f209ccdu +
                  multiply_high_u32(in_lo, 0x45e7fd31u) + 0xdc9b756cu +
                  add_carry_u32(lo0, 0xa33ccbb5u);

    for (unsigned i = 0; i < 7u; ++i) {
        ladder64_step(k_f41a12b0, &lo, &hi);
    }
    uint32_t mid_lo = lo;
    uint32_t mid_hi = hi;

    for (unsigned i = 0; i < 7u; ++i) {
        ladder64_step(k_f41a12b0, &lo, &hi);
    }

    /* Native Ghidra lowers the tail as one normal low-word step followed by
     * a final low-table lookup used only by the rolling hi update.  Keep this
     * shape explicit instead of hiding it inside a generic 64-bit ladder.
     */
    unsigned nib = lo & 0x0fu;
    uint32_t shifted = (lo >> 4) | (hi << 28);
    uint32_t step_lo = shifted + k_f41a12b0[nib][0];
    uint32_t step_hi = (hi >> 4) + k_f41a12b0[nib][1] +
                       add_carry_u32(shifted, k_f41a12b0[nib][0]);
    (void)step_hi;
    uint32_t tail_low = (step_lo >> 4) | (step_hi << 28);
    tail_low += k_f41a12b0[step_lo & 0x0fu][0];

    uint32_t out = (in_lo * 0x4d9eaff3u + mid_lo * 0xd0000000u + 0x82b7eeda) *
                   k_f4190990[lane & 7u] + k_f41909b0[lane & 7u];

    uint32_t next_lo0 = mid_lo * 0x4a403bd1u;
    uint32_t next_lo = next_lo0 + 0x792afa3bu;
    uint32_t next_hi = mid_hi * 0x4a403bd1u + mid_lo * 0xda86667eu +
                       multiply_high_u32(mid_lo, 0x4a403bd1u) +
                       tail_low * 0x5bfc42f0u +
                       0xa1a47ae4u + add_carry_u32(next_lo0, 0x792afa3bu);

    if (out_word) *out_word = out;
    if (out_mid_pair) {
        out_mid_pair[0] = mid_lo;
        out_mid_pair[1] = mid_hi;
    }
    if (out_next_pair) {
        out_next_pair[0] = next_lo;
        out_next_pair[1] = next_hi;
    }
    if (out_tail_low) *out_tail_low = tail_low;
}

int l3_authorization_primary_post_sixth_f12b0_output_words26(
    const uint32_t pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_sixth_f12b0_output_trace *out) {
    if (!pairs26 || !out_words26) {
        return -1;
    }

    uint32_t mid[2] = {0u, 0u};
    uint32_t next[2] = {0u, 0u};
    uint32_t tail = 0u;
    for (unsigned lane = 0; lane < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++lane) {
        post_sixth_f12b0_scalar_from_pair(
            pairs26[lane], lane, &out_words26[lane], mid, next, &tail);
        if (out) {
            out->mid_ladder_pairs26[lane][0] = mid[0];
            out->mid_ladder_pairs26[lane][1] = mid[1];
            out->rolling_pairs26[lane][0] = next[0];
            out->rolling_pairs26[lane][1] = next[1];
            out->tail_ladder_low26[lane] = tail;
        }
    }

    if (out) {
        memcpy(out->input_pairs26, pairs26, sizeof(out->input_pairs26));
        memcpy(out->output_words26, out_words26, sizeof(out->output_words26));
        out->final_rolling_pair[0] = next[0];
        out->final_rolling_pair[1] = next[1];
        out->final_state_ladder_low = tail;
    }
    return 0;
}

int l3_authorization_primary_post_sixth_companion_output_words26(
    const uint32_t source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1230_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_f1230_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_sixth_output_trace *out) {
    if (!source_words13 || !affine_words13 || !out_f1170_words13 ||
        !out_f11b0_raw13 || !out_f11b0_prefix13 || !out_f1230_raw13 ||
        !out_f1230_prefix13 || !out_pairs26 || !out_words26) {
        return -1;
    }

    l3_authorization_primary_post_sixth_output_trace tmp;
    l3_authorization_primary_post_sixth_output_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_sixth_companion_convolution26(
        source_words13, affine_words13, out_f1170_words13, out_f11b0_raw13,
        out_f11b0_prefix13, out_f1230_raw13, out_f1230_prefix13, out_pairs26,
        &dst->companion);
    if (rc != 0) {
        return rc;
    }
    rc = l3_authorization_primary_post_sixth_f12b0_output_words26(
        out_pairs26, out_words26, &dst->f12b0);
    return rc;
}

static const uint32_t k_first_local5ac_mul_a[4] = {
    0xf22e00a9u, 0x7632184fu, 0x68317277u, 0x9e5d9279u
};
static const uint32_t k_first_local5ac_add_a[4] = {
    0xb7091d89u, 0xec5876d6u, 0x0085073bu, 0xe293cd5au
};
static const uint32_t k_first_local5ac_mul_b[4] = {
    0x724512d7u, 0x63779373u, 0xae0516c3u, 0xae98af1bu
};
static const uint32_t k_first_local5ac_add_b[4] = {
    0x0325af50u, 0x369e5fecu, 0x3ddde17au, 0x202a9b15u
};
static const uint32_t k_f41909d0_local3a0[8] = {
    0x9782169fu, 0x99892471u, 0x7e4fc0f9u, 0x794be4fdu,
    0x768f7293u, 0xae5f4b83u, 0xff695dc3u, 0xb81318d5u
};
static const uint32_t k_f41909f0_local3a0[8] = {
    0xafbc9224u, 0x5f30a944u, 0xd37ebf04u, 0x0670ec86u,
    0x9d540ca9u, 0x30f954e4u, 0xa54cd732u, 0xa17cbb17u
};
static const uint32_t k_f41a1330_local3a0[16][2] = {
    {0x9d67bfb1u,0xf313dca9u}, {0x5e3311b8u,0xee6579ccu},
    {0x848bea1eu,0xdeb55e41u}, {0xaae4c284u,0xcf0542b6u},
    {0xd13d9aeau,0xbf55272bu}, {0xf7967350u,0xafa50ba0u},
    {0x1def4bb6u,0x9ff4f016u}, {0x4448241cu,0x8044d48bu},
    {0x6aa0fc82u,0x7094b900u}, {0x90f9d4e8u,0x60e49d75u},
    {0xb752ad4eu,0x513481eau}, {0xddab85b4u,0x4184665fu},
    {0x04045e1au,0x31d44ad5u}, {0x2a5d3680u,0x22242f4au},
    {0x50b60ee6u,0x127413bfu}, {0x770ee74cu,0x02c3f834u}
};
static const uint32_t k_f4190a10_local3a0[8] = {
    0x71995e11u, 0x7191bdcdu, 0xbdbc96f5u, 0xde748e83u,
    0x6dec7395u, 0x68348557u, 0x325a9f79u, 0x682047d1u
};
static const uint32_t k_f4190a30_local3a0[8] = {
    0x27b7e510u, 0xeb9a04c3u, 0x0bc6f7cbu, 0x9a0c144au,
    0x8190a324u, 0x35fec3f9u, 0x538d87dcu, 0x0203e606u
};
static const uint32_t k_f41a13b0_local3a0[16][2] = {
    {0xd0c2d363u,0x75fb9da5u}, {0x769c2f53u,0x095273f6u},
    {0x1c758b43u,0x9ca94a47u}, {0xc24ee733u,0x20002097u},
    {0x68284323u,0xb356f6e8u}, {0x0e019f13u,0x46adcd39u},
    {0xb3dafb03u,0xda04a389u}, {0xfc1e97f4u,0x67ee14cfu},
    {0xa1f7f3e4u,0xfb44eb20u}, {0x47d14fd4u,0x8e9bc171u},
    {0xedaaabc4u,0x11f297c1u}, {0x938407b4u,0xa5496e12u},
    {0x395d63a4u,0x38a04463u}, {0xdf36bf94u,0xcbf71ab3u},
    {0x85101b84u,0x5f4df104u}, {0x2ae97774u,0xe2a4c755u}
};

static void make_first_local5ac_words13(
    const uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]) {
    for (unsigned i = 0; i < 4u; ++i) {
        out_words13[i] =
            first_words13[i] * k_first_local5ac_mul_a[i] +
            k_first_local5ac_add_a[i];
        out_words13[i + 4u] =
            first_words13[i + 4u] * k_first_local5ac_mul_b[i] +
            k_first_local5ac_add_b[i];
        out_words13[i + 8u] =
            first_words13[i + 8u] * k_first_local5ac_mul_a[i] +
            k_first_local5ac_add_a[i];
    }
    out_words13[12] =
        first_words13[12] * k_first_local5ac_mul_b[0] +
        k_first_local5ac_add_b[0];
}

static void make_local5ac_pairs13(
    const uint32_t local5ac_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2]) {
    for (unsigned i = 0; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        const unsigned lane = i & 7u;
        const uint32_t token =
            (local5ac_words13[i] * k_f41909d0_local3a0[lane] +
             k_f41909f0_local3a0[lane]) * 0xa1236035u + 0xd378ab96u;

        const uint32_t lo0 = token * 0x733c827du;
        uint32_t lo = lo0 + 0xe244c2beu;
        uint32_t hi =
            token * 0x32ba087au + multiply_high_u32(token, 0x733c827du) +
            0x99143799u + add_carry_u32(lo0, 0xe244c2beu);
        for (unsigned step = 0; step < 7u; ++step) {
            ladder64_step(k_f41a1330_local3a0, &lo, &hi);
        }
        const uint32_t shifted = (lo >> 4) | (hi << 28);
        const uint32_t folded =
            shifted + k_f41a1330_local3a0[lo & 0x0fu][0];

        const uint32_t pair_lo0 = token * 0x3802d907u;
        out_pairs13[i][0] = pair_lo0 + 0x64e1a82fu;
        out_pairs13[i][1] =
            token * 0x53a0d8d2u + multiply_high_u32(token, 0x3802d907u) +
            folded * 0x8fd9c32du + 0x6182b424u +
            add_carry_u32(pair_lo0, 0x64e1a82fu);
    }
}

static void make_local3a0_words26(
    const uint32_t pairs13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS]) {
    uint32_t rolling_lo = 0x2fe8debbu;
    uint32_t rolling_hi = 0xe5706ac0u;

    for (unsigned index = 0; index < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++index) {
        unsigned first = index > 12u ? index - 12u : 0u;
        unsigned last = index < 12u ? index : 12u;
        uint32_t acc_lo = 0x26bf90bdu;
        uint32_t acc_hi = 0x7c6bf597u;

        while (first < last) {
            const uint32_t a_lo = pairs13[last][0];
            const uint32_t a_hi = pairs13[last][1];
            const uint32_t b_lo = pairs13[first][0];
            const uint32_t b_hi = pairs13[first][1];
            ++first;
            --last;

            const uint32_t q_lo = a_lo * 0x08b2fcdbu;
            const uint32_t transformed_lo = q_lo + 0xb4ae0999u;
            const uint64_t product = (uint64_t)b_lo * transformed_lo;
            const uint32_t product_lo = (uint32_t)product;
            const uint32_t product_hi = (uint32_t)(product >> 32);
            const uint32_t a_scaled_lo = a_lo * 0xb4ae0999u;
            const uint32_t carry_acc_a = add_carry_u32(acc_lo, a_scaled_lo);
            const uint32_t sum_a = acc_lo + a_scaled_lo;
            const uint32_t carry_acc_product =
                add_carry_u32(sum_a, product_lo);
            const uint32_t sum_product = sum_a + product_lo;
            const uint32_t carry_bias =
                add_carry_u32(sum_product, 0x071ebaa3u);
            acc_lo = sum_product + 0x071ebaa3u;

            const uint32_t transformed_hi =
                a_hi * 0x08b2fcdbu + a_lo * 0x83d3b12au +
                multiply_high_u32(a_lo, 0x08b2fcdbu) + 0x86d487cdu +
                add_carry_u32(q_lo, 0xb4ae0999u);
            acc_hi =
                acc_hi + a_hi * 0xb4ae0999u + a_lo * 0x86d487cdu +
                multiply_high_u32(a_lo, 0xb4ae0999u) + carry_acc_a +
                transformed_hi * b_lo + transformed_lo * b_hi +
                product_hi + carry_acc_product + 0x8e9b71f3u +
                carry_bias;
        }

        uint32_t combined_part_lo;
        uint32_t combined_part_hi;
        uint32_t add_bias_lo;
        uint32_t add_bias_hi;
        uint32_t add_bias_carry;
        if (first == last) {
            const uint32_t rolling_mul_lo = rolling_lo * 0x1e92ec81u;
            const uint32_t acc_mul_lo = acc_lo * 0x18afaaa2u;
            const uint32_t sum_lo = acc_mul_lo + rolling_mul_lo;
            const uint32_t sum_carry =
                add_carry_u32(acc_mul_lo, rolling_mul_lo);
            const uint64_t sum_product =
                (uint64_t)sum_lo * 0x10e2316fu;
            add_bias_lo = (uint32_t)sum_product;

            const uint32_t diag_lo = pairs13[first][0];
            const uint32_t diag_hi = pairs13[first][1];
            const uint32_t diag_mul_lo = diag_lo * 0x7013c385u;
            const uint32_t diag_factor = diag_mul_lo + 0xa52b210eu;
            const uint64_t diag_product =
                (uint64_t)diag_lo * diag_factor;
            combined_part_lo = (uint32_t)diag_product;

            combined_part_hi =
                (acc_hi * 0x18afaaa2u + acc_lo * 0x650b76f7u +
                 multiply_high_u32(acc_lo, 0x18afaaa2u) +
                 rolling_hi * 0x1e92ec81u + rolling_lo * 0x68619c60u +
                 multiply_high_u32(rolling_lo, 0x1e92ec81u) + sum_carry) *
                    0x10e2316fu +
                sum_lo * 0xaf64690eu + (uint32_t)(sum_product >> 32);

            const uint32_t diag_factor_hi =
                diag_hi * 0x7013c385u + diag_lo * 0xdfd68942u +
                multiply_high_u32(diag_lo, 0x7013c385u) + 0x239754cbu +
                add_carry_u32(diag_mul_lo, 0xa52b210eu);
            add_bias_hi =
                diag_factor_hi * diag_lo + diag_factor * diag_hi +
                (uint32_t)(diag_product >> 32);
            add_bias_carry = add_carry_u32(add_bias_lo, 0x799538dfu);
            add_bias_lo += 0x799538dfu;
            add_bias_hi += 0xe8c7e78eu;
        } else {
            add_bias_lo = rolling_lo * 0x3adb3cefu;
            combined_part_lo = acc_lo * 0x16d7fe3eu;
            combined_part_hi =
                rolling_hi * 0x3adb3cefu + rolling_lo * 0x80733812u +
                multiply_high_u32(rolling_lo, 0x3adb3cefu);
            add_bias_hi =
                acc_hi * 0x16d7fe3eu + acc_lo * 0x454c6d78u +
                multiply_high_u32(acc_lo, 0x16d7fe3eu);
            add_bias_carry = add_carry_u32(add_bias_lo, 0xfdd34222u);
            add_bias_lo += 0xfdd34222u;
            add_bias_hi += 0x59f04977u;
        }

        const uint32_t combined_lo = add_bias_lo + combined_part_lo;
        const uint32_t combined_hi =
            combined_part_hi + add_bias_hi + add_bias_carry +
            add_carry_u32(add_bias_lo, combined_part_lo);
        const uint32_t ladder_lo0 = combined_lo * 0x8d63aa85u;
        uint32_t ladder_lo = ladder_lo0 + 0x4ad57f46u;
        uint32_t ladder_hi =
            combined_hi * 0x8d63aa85u + combined_lo * 0x30fb8938u +
            multiply_high_u32(combined_lo, 0x8d63aa85u) + 0xea5ba59au +
            add_carry_u32(ladder_lo0, 0x4ad57f46u);

        for (unsigned step = 0; step < 7u; ++step) {
            ladder64_step(k_f41a13b0_local3a0,
                               &ladder_lo, &ladder_hi);
        }
        const uint32_t saved_lo = ladder_lo;
        const uint32_t saved_hi = ladder_hi;
        for (unsigned step = 0; step < 7u; ++step) {
            ladder64_step(k_f41a13b0_local3a0,
                               &ladder_lo, &ladder_hi);
        }
        ladder64_step(k_f41a13b0_local3a0,
                           &ladder_lo, &ladder_hi);
        const uint32_t tail_shifted =
            (ladder_lo >> 4) | (ladder_hi << 28);
        const uint32_t tail_low =
            tail_shifted +
            k_f41a13b0_local3a0[ladder_lo & 0x0fu][0];

        const unsigned lane = index & 7u;
        out_words26[index] =
            (combined_lo * 0x8815a389u + saved_lo * 0xb0000000u +
             0x1e4dfe06u) * k_f4190a10_local3a0[lane] +
            k_f4190a30_local3a0[lane];

        const uint32_t rolling_lo0 = saved_lo * 0x183d7083u;
        rolling_lo = rolling_lo0 + 0xc8cb6696u;
        rolling_hi =
            saved_hi * 0x183d7083u + saved_lo * 0xf11f1d13u +
            multiply_high_u32(saved_lo, 0x183d7083u) +
            tail_low * 0x7c28f7d0u + 0x8f191b4bu +
            add_carry_u32(rolling_lo0, 0xc8cb6696u);
    }
}

int l3_authorization_primary_post_sixth_local3a0_words26(
    const uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_local5ac_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_pairs13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS]) {
    if (!first_words13 || !out_local5ac_words13 || !out_pairs13 ||
        !out_local3a0_words26) {
        return -1;
    }
    make_first_local5ac_words13(first_words13, out_local5ac_words13);
    make_local5ac_pairs13(out_local5ac_words13, out_pairs13);
    make_local3a0_words26(out_pairs13, out_local3a0_words26);
    return 0;
}

/* v252: local_130 mixer and DAT_f41a1430 scalar output after v251. */
static const uint32_t k_f4190a50[8] = {
    0x95a96407u, 0x77dc87d7u, 0x4edbff31u, 0x37fe09e3u,
    0xc5df58f3u, 0x86114495u, 0x12679e05u, 0xb4f64f5bu
};
static const uint32_t k_f4190a70[8] = {
    0xc048531du, 0xfab158ddu, 0xc9bd0ac1u, 0xaa55037bu,
    0x71c1da2fu, 0x27107e4du, 0x95013a63u, 0xbed52d43u
};
static const uint32_t k_f4190a90[8] = {
    0x5dcc2361u, 0x8faf5c35u, 0x3f89f90du, 0xb5393ffbu,
    0x7bc87d6du, 0xc7036477u, 0x675a41b9u, 0x682b3da1u
};
static const uint32_t k_f4190ab0[8] = {
    0x7d7bee37u, 0x7e99909fu, 0xb637cc37u, 0xbc636599u,
    0xc2a0b2a7u, 0x24d294c1u, 0xf356a0d1u, 0x330c1a09u
};
static const uint32_t k_f4190ad0[8] = {
    0x93968ce4u, 0xa90da209u, 0xc2821b49u, 0x78de6369u,
    0x0f9d8c72u, 0x0015d6b8u, 0x774cade1u, 0xc4dca574u
};
static const uint32_t k_f41a1430[16] = {
    0x6a8bfbcfu, 0x35c0e322u, 0x0ed0a0feu, 0xda058851u,
    0xa315462du, 0x7e4a2d80u, 0x4759eb5cu, 0x1069a938u,
    0xeb9e908bu, 0xb4ae4e67u, 0x8fe335bau, 0x58f2f396u,
    0x2427dae9u, 0xfd3798c5u, 0xc64756a1u, 0x917c3df4u
};

static void local130_apply_native_overrides(uint32_t w[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
                                                 const uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS]) {
    static const uint32_t k_block_a[4] = {
        0xd6b7507au, 0x6ee235a1u, 0xcf766a5eu, 0xe25923c5u
    };
    static const uint32_t k_block_b[4] = {
        0x28c9d280u, 0x1966cfb2u, 0x56c838c8u, 0x9cb4ecd2u
    };

    /*
     * Native materializes local_130[1..25] as alternating four-word
     * constant blocks before applying the three mixer streams.  Earlier
     * reconstruction retained caller data in slots 2..12, although those
     * words have already been overwritten at f3fcb244..f3fcb278.
     */
    for (unsigned base = 1u; base <= 21u; base += 8u) {
        memcpy(w + base, k_block_b, sizeof(k_block_b));
        memcpy(w + base + 4u, k_block_a, sizeof(k_block_a));
    }
    w[25] = k_block_b[0];

    /* local_130 = CONCAT44(word1, word0). */
    w[0] = f12b0_words26[0] * 0x95a96407u + 0xe25923c5u;
    w[1] = f12b0_words26[1] * 0x77dc87d7u + 0x28c9d280u;
}

int l3_authorization_primary_post_sixth_local130_mix_words26(
    const uint32_t initial_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t param4_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_sixth_local130_mix_trace *out) {
    if (!initial_words26 || !f12b0_words26 || !param4_words26 ||
        !local3a0_words26 || !out_mixed_words26) {
        return -1;
    }

    memcpy(out_mixed_words26, initial_words26, L3_AUTH_ROUND_PRIMARY_CONV_PAIRS * sizeof(uint32_t));
    local130_apply_native_overrides(out_mixed_words26, f12b0_words26);

    for (unsigned i = 2u; i < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++i) {
        out_mixed_words26[i] += f12b0_words26[i] * k_f4190a50[i & 7u];
    }
    for (unsigned i = 0u; i < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++i) {
        out_mixed_words26[i] += param4_words26[i] * k_f4190a70[i & 7u];
    }
    for (unsigned i = 0u; i < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++i) {
        out_mixed_words26[i] += local3a0_words26[i] * k_f4190a90[i & 7u];
    }
    /* The ARM body contains the same local_3a0 accumulation loop twice
     * (f3fcb308 and f3fcb330); both additions are part of the result. */
    for (unsigned i = 0u; i < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++i) {
        out_mixed_words26[i] += local3a0_words26[i] * k_f4190a90[i & 7u];
    }

    if (out) {
        memcpy(out->initial_words26, initial_words26, sizeof(out->initial_words26));
        memcpy(out->f12b0_words26, f12b0_words26, sizeof(out->f12b0_words26));
        memcpy(out->param4_words26, param4_words26, sizeof(out->param4_words26));
        memcpy(out->local3a0_words26, local3a0_words26, sizeof(out->local3a0_words26));
        memcpy(out->mixed_words26, out_mixed_words26, sizeof(out->mixed_words26));
    }
    return 0;
}

static uint32_t f1430_ladder7(uint32_t x) {
    for (unsigned i = 0; i < 7u; ++i) {
        x = k_f41a1430[x & 0x0fu] + (x >> 4);
    }
    return x;
}

int l3_authorization_primary_post_sixth_f1430_output_words26(
    const uint32_t mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_sixth_f1430_output_trace *out) {
    if (!mixed_words26 || !out_words26) {
        return -1;
    }

    uint32_t rolling = 0xbfef0d84u; /* -0x4010f27c */
    for (unsigned i = 0u; i < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++i) {
        uint32_t before = rolling;
        uint32_t mix = before * 0x24914bbdu + mixed_words26[i];
        uint32_t x = mix * 0x98113babu + 0xa5a8d0e3u; /* -0x67eec455 */
        uint32_t ladder = f1430_ladder7(x);
        uint32_t table = k_f41a1430[ladder & 0x0fu];
        uint32_t word = (mix * 0x4ac25fd3u + ladder * 0x70000000u + 0x0dd7617bu) *
                        k_f4190ab0[i & 7u] + k_f4190ad0[i & 7u];
        rolling = ladder * 0x4a95c9bfu + (table + (ladder >> 4)) * 0x56a36410u + 0xa749ad4au;
        out_words26[i] = word;
        if (out) {
            out->input_words26[i] = mixed_words26[i];
            out->ladder_words26[i] = ladder;
            out->rolling_before26[i] = before;
            out->rolling_after26[i] = rolling;
            out->output_words26[i] = word;
        }
    }
    if (out) {
        out->final_rolling = rolling;
    }
    return 0;
}

int l3_authorization_primary_post_sixth_local130_f1430_words26(
    const uint32_t initial_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t param4_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    l3_authorization_primary_post_sixth_local130_f1430_trace *out) {
    if (!initial_words26 || !f12b0_words26 || !param4_words26 || !local3a0_words26 ||
        !out_mixed_words26 || !out_words26) {
        return -1;
    }
    l3_authorization_primary_post_sixth_local130_f1430_trace tmp;
    l3_authorization_primary_post_sixth_local130_f1430_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_sixth_local130_mix_words26(
        initial_words26, f12b0_words26, param4_words26, local3a0_words26,
        out_mixed_words26, &dst->mix);
    if (rc != 0) return rc;
    return l3_authorization_primary_post_sixth_f1430_output_words26(
        out_mixed_words26, out_words26, &dst->f1430);
}
/* v253: DAT_f41a1470 pair-image transform over the v252 local_718 words. */
static const uint32_t k_f4190af0[8] = {
    0x375acff5u, 0xb20e6a7du, 0xfd9b95f5u, 0x511e709bu,
    0x769102a5u, 0xb87a2d63u, 0x38014a33u, 0xd2d5fb4bu
};
static const uint32_t k_f4190b10[8] = {
    0x383058d4u, 0x681bb2a3u, 0x2cc0f12bu, 0x5de76a75u,
    0x5323218eu, 0x447929e0u, 0xe2eedd35u, 0xe2f8560cu
};
static const uint32_t k_f41a1470[16][2] = {
    {0x259628d2u,0x267c90e5u}, {0x213c354du,0x990fd3b4u},
    {0x1ce241c8u,0x0ba31683u}, {0x2674f320u,0x7c5f1d22u},
    {0x221aff9bu,0xeef25ff1u}, {0x1dc10c16u,0x5185a2c0u},
    {0x2753bd6eu,0xc241a95fu}, {0x22f9c9e9u,0x34d4ec2eu},
    {0x1e9fd664u,0xa7682efdu}, {0x283287bcu,0x1824359cu},
    {0x23d89437u,0x8ab7786bu}, {0x1f7ea0b2u,0xfd4abb3au},
    {0x2911520au,0x6e06c1d9u}, {0x24b75e85u,0xd09a04a8u},
    {0x205d6b00u,0x432d4777u}, {0x29f01c58u,0xb3e94e16u}
};

int l3_authorization_primary_post_sixth_f1470_state26(
    const uint32_t local718_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_sixth_f1470_state_trace *out) {
    if (!local718_words26 || !out_state26) {
        return -1;
    }

    for (unsigned i = 0u; i < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++i) {
        uint32_t lane = i & 7u;
        uint32_t affine = (local718_words26[i] * k_f4190af0[lane] +
                           k_f4190b10[lane]) * 0x1044e25fu + 0x2e6d7d4du;

        uint32_t lo0 = multiply_low_u32(affine, 0x279e0621u);
        uint32_t lo = lo0 + 0xcaf68f34u;
        uint32_t hi = multiply_low_u32(affine, 0xd6ae53b2u) +
                      multiply_high_u32(affine, 0x279e0621u) +
                      0x280991aeu + (uint32_t)(0x350970cbu < lo0);
        uint32_t ladder = nibble_ladder64_run8_low(k_f41a1470, lo, hi);

        uint32_t pair_lo_mul = multiply_low_u32(affine, 0xc0bdf697u);
        out_state26[i][0] = pair_lo_mul + 0xe2b8bb83u;
        out_state26[i][1] = multiply_low_u32(affine, 0xf4b7d53bu) +
                            multiply_high_u32(affine, 0xc0bdf697u) +
                            ladder * 0xfe550a49u +
                            0xf3dd910du + (uint32_t)(0x1d47447cu < pair_lo_mul);

        if (out) {
            out->input_words26[i] = local718_words26[i];
            out->affine_words26[i] = affine;
            out->ladder_lows26[i] = ladder;
            out->state26[i][0] = out_state26[i][0];
            out->state26[i][1] = out_state26[i][1];
        }
    }
    return 0;
}

int l3_authorization_primary_post_sixth_f1430_f1470_state26(
    const uint32_t initial_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t param4_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_local718_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    l3_authorization_primary_post_sixth_f1430_f1470_trace *out) {
    if (!initial_words26 || !f12b0_words26 || !param4_words26 || !local3a0_words26 ||
        !out_mixed_words26 || !out_local718_words26 || !out_state26) {
        return -1;
    }
    l3_authorization_primary_post_sixth_f1430_f1470_trace tmp;
    l3_authorization_primary_post_sixth_f1430_f1470_trace *dst = out ? out : &tmp;
    int rc = l3_authorization_primary_post_sixth_local130_f1430_words26(
        initial_words26, f12b0_words26, param4_words26, local3a0_words26,
        out_mixed_words26, out_local718_words26, &dst->f1430);
    if (rc != 0) return rc;
    return l3_authorization_primary_post_sixth_f1470_state26(
        out_local718_words26, out_state26, &dst->f1470);
}



/* v254: seventh FUN_f3fdb57c call-site after the post-sixth f1430/f1470
 * state-image builder. */
int l3_authorization_primary_seventh_vector_reduce_update_export(
    const uint32_t initial_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t param4_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_local718_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_seventh_vector_reduce_trace *out) {
    if (!initial_words26 || !f12b0_words26 || !param4_words26 || !local3a0_words26 ||
        !vector_reduce_param1_13 || !out_mixed_words26 || !out_local718_words26 ||
        !out_state26 || !out_words13) {
        return -1;
    }

    l3_authorization_primary_post_sixth_f1430_f1470_trace local_pre;
    l3_authorization_primary_post_sixth_f1430_f1470_trace *pre_dst = out ? &out->pre_state : &local_pre;

    int rc = l3_authorization_primary_post_sixth_f1430_f1470_state26(
        initial_words26, f12b0_words26, param4_words26, local3a0_words26,
        out_mixed_words26, out_local718_words26, out_state26, pre_dst);
    if (rc != 0) {
        return rc;
    }

    if (out) {
        memcpy(out->pre_vector_reduce_state26, out_state26, sizeof(out->pre_vector_reduce_state26));
    }

    return l3_authorization_vector_reduce_run13_update_export(
        vector_reduce_param1_13, vector_reduce_param3, vector_reduce_param4,
        out_state26, out_words13, out ? &out->f3fdb57c : NULL);
}


/* v263: native-order post-sixth source-to-seventh composition. */
int l3_authorization_primary_post_sixth_composed_seventh_update_export(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_staging_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_mix_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_staging_source_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_local718_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_composed_seventh_trace *out) {
    if (!live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !sixth_words13 || !param4_param5_words13 || !param4_staging_words13 ||
        !affine_words13 || !param4_mix_words26 || !local3a0_words26 ||
        !vector_reduce_param1_13 || !out_param5_words13 ||
        !out_staging_source_words13 || !out_f12b0_words26 ||
        !out_mixed_words26 || !out_local718_words26 || !out_state26 ||
        !out_words13) {
        return -1;
    }

    l3_authorization_primary_post_sixth_composed_seventh_trace tmp;
    l3_authorization_primary_post_sixth_composed_seventh_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    /* local_75c is no longer the original F3FDB param_3 here.  Native
     * replaces it after the second call with the invariant exported tail
     * word transformed at f3fc5a24/f3fc5cb0, and reuses that saved value in
     * both scalar lane-12 source formulas below.
     */
    const uint32_t post_sixth_tail_scalar =
        sixth_words13[12] * 0x52d94cf1u + 0x2bd3b4b2u;

    uint32_t param5_source13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    int rc = l3_authorization_primary_post_sixth_param5_source_ladder_words13(
        live_q6_words4, live_q5_words4, live_q4_words4, sixth_words13,
        param4_param5_words13, post_sixth_tail_scalar, param5_source13,
        out_param5_words13, &dst->param5);
    if (rc != 0) {
        return rc;
    }

    uint32_t f1170_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t f11b0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t f11b0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    rc = l3_authorization_primary_post_sixth_staging_source_pair_streams13(
        live_q6_words4, live_q5_words4, live_q4_words4, out_param5_words13,
        param4_staging_words13, post_sixth_tail_scalar, affine_words13,
        out_staging_source_words13, f1170_words13, f11b0_raw13,
        f11b0_prefix13, &dst->staging);
    if (rc != 0) {
        return rc;
    }

    uint32_t f1230_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t f1230_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t companion_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    rc = l3_authorization_primary_post_sixth_companion_output_words26(
        out_staging_source_words13, affine_words13, f1170_words13,
        f11b0_raw13, f11b0_prefix13, f1230_raw13, f1230_prefix13,
        companion_pairs26, out_f12b0_words26, &dst->companion_output);
    if (rc != 0) {
        return rc;
    }

    memset(dst->local130_initial26, 0, sizeof(dst->local130_initial26));
    memcpy(dst->local130_initial26, out_staging_source_words13,
           L3_AUTH_ROUND_PRIMARY_SEED_WORDS * sizeof(uint32_t));

    return l3_authorization_primary_seventh_vector_reduce_update_export(
        dst->local130_initial26, out_f12b0_words26, param4_mix_words26,
        local3a0_words26, vector_reduce_param1_13, vector_reduce_param3,
        vector_reduce_param4, out_mixed_words26, out_local718_words26,
        out_state26, out_words13, &dst->seventh);
}


/* v255: post-seventh FUN_f3fdb57c export affine into native param_6. */
static void post_seventh_make_param6_words(
    const uint32_t seventh_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]) {
    static const uint32_t k_a[4] = {
        0xa6d418ffu, 0x62231c0bu, 0xbc5aed61u, 0x2bb0eca5u
    };
    static const uint32_t add_a[4] = {
        0xfdcdebcau, 0x059eaa77u, 0x6fa3f9fbu, 0x8a108446u
    };
    static const uint32_t k_b[4] = {
        0x36052a6du, 0x61a7f459u, 0x558d7c0du, 0x4c02e453u
    };
    static const uint32_t add_b[4] = {
        0x108ba927u, 0xb344eb0du, 0xa76e1a93u, 0xc8f221deu
    };
    for (unsigned i = 0; i < 4u; ++i) {
        out_param6_words13[i] = seventh_words13[i] * k_a[i] + add_a[i];
        out_param6_words13[i + 4u] =
            seventh_words13[i + 4u] * k_b[i] + add_b[i];
        out_param6_words13[i + 8u] =
            seventh_words13[i + 8u] * k_a[i] + add_a[i];
    }
    out_param6_words13[12] = seventh_words13[12] * 0x36052a6du + 0x108ba927u;
}

int l3_authorization_primary_post_seventh_param6_words13(
    const uint32_t seventh_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_seventh_param6_trace *out) {
    if (!seventh_words13 || !out_param6_words13) {
        return -1;
    }
    post_seventh_make_param6_words(seventh_words13, out_param6_words13);
    if (out) {
        memcpy(out->seventh_words13, seventh_words13, sizeof(out->seventh_words13));
        memcpy(out->param6_words13, out_param6_words13, sizeof(out->param6_words13));
    }
    return 0;
}

/* v256: post-seventh staging pair streams.  This is the section after the
 * v255 param_6 writeback and before the next convolution/FUN_f3fdb57c call.
 */
static const uint32_t k_f4190b30[8] = {
    0xd8a8d42fu, 0x7214debbu, 0x326e3c41u, 0x3539e8afu,
    0x79fe8aefu, 0xf905992bu, 0x1522dc97u, 0x76bb3d23u
};
static const uint32_t k_f4190b50[8] = {
    0x4ada15a1u, 0x44753267u, 0x249e5d8du, 0x3e135e8bu,
    0x14a07426u, 0x2de03320u, 0x520cc338u, 0x61937858u
};
static const uint32_t k_f4190b70[8] = {
    0x2fb5740bu, 0x7f628969u, 0x9bba73c3u, 0xc8d16f25u,
    0x6ff590d3u, 0xafe2f657u, 0x728aef17u, 0xf64d1ffbu
};
static const uint32_t k_f4190b90[8] = {
    0xda116d9bu, 0x5e83105au, 0x3e7cbf6fu, 0xaa489d3du,
    0x1c94109bu, 0x75bb2aa0u, 0x33cc13b2u, 0xb7ffb47fu
};
static const uint32_t k_f41a14f0[16][2] = {
    {0x87f478bcu,0xf196e228u}, {0x2fbeaa99u,0x6b00fcc0u},
    {0xd788dc76u,0xd46b1757u}, {0x7f530e53u,0x4dd531efu},
    {0x271d4030u,0xb73f4c87u}, {0xcee7720du,0x20a9671eu},
    {0x76b1a3eau,0x9a1381b6u}, {0x1e7bd5c7u,0x037d9c4eu},
    {0xc64607a4u,0x7ce7b6e5u}, {0x6e103981u,0xe651d17du},
    {0x15da6b5eu,0x5fbbec15u}, {0xbda49d3bu,0xc92606acu},
    {0xe8cbb149u,0x3bee77c9u}, {0x9095e326u,0xa5589261u},
    {0x38601503u,0x1ec2acf9u}, {0xe02a46e0u,0x882cc790u}
};
static const uint32_t k_f41a1570[16][2] = {
    {0x31cdb016u,0x6e4a652eu}, {0xdc5c9184u,0x1cf2da56u},
    {0x4e9c15fdu,0xc2c38951u}, {0xf92af76bu,0x716bfe79u},
    {0xa3b9d8d9u,0x201473a2u}, {0x4e48ba47u,0xdebce8cbu},
    {0xf8d79bb5u,0x8d655df3u}, {0xa3667d23u,0x3c0dd31cu},
    {0x15a6019cu,0xe1de8217u}, {0xc034e30au,0x9086f73fu},
    {0x6ac3c478u,0x4f2f6c68u}, {0x1552a5e6u,0xfdd7e191u},
    {0xbfe18754u,0xac8056b9u}, {0x32210bcdu,0x525105b4u},
    {0xdcafed3bu,0x00f97adcu}, {0x873ecea9u,0xbfa1f005u}
};

static void post_seventh_local5e0_pair(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = multiply_low_u32(token, 0x2b34f99bu);
    uint32_t lo = lo0 + 0x3f92000du;
    uint32_t hi = multiply_low_u32(token, 0x06a1f70ccu) +
                  multiply_high_u32(token, 0x2b34f99bu) +
                  0xc91844d7u + (uint32_t)(0xc06dfff2u < lo0);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a14f0, lo, hi);

    uint32_t lo1 = multiply_low_u32(token, 0xe140f80bu);
    out_pair[0] = lo1 + 0x2f4b34e1u;
    out_pair[1] = multiply_low_u32(token, 0x93240f42u) +
                  multiply_high_u32(token, 0xe140f80bu) +
                  ladder * 0xee2f25afu +
                  0x72ddbbffu + (uint32_t)(0xd0b4cb1eu < lo1);
}

static void post_seventh_param3_pair(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = multiply_low_u32(token, 0x3244e655u);
    uint32_t lo = lo0 + 0x4d103538u;
    uint32_t hi = multiply_low_u32(token, 0x91479103u) +
                  multiply_high_u32(token, 0x3244e655u) +
                  0x29f1b628u + (uint32_t)(0xb2efcac7u < lo0);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a1570, lo, hi);

    uint32_t lo1 = multiply_low_u32(token, 0x7c6fe227u);
    out_pair[0] = lo1 + 0xdc38b21au;
    out_pair[1] = multiply_low_u32(token, 0x840fc1d4u) +
                  multiply_high_u32(token, 0x7c6fe227u) +
                  ladder * 0xbb967575u +
                  0x899b606cu + (uint32_t)(0x23c74de5u < lo1);
}

int l3_authorization_primary_post_seventh_staging_pair_streams13(
    const int32_t local5e0_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_param3_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_param3_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    l3_authorization_primary_post_seventh_staging_trace *out) {
    if (!local5e0_words13 || !param3_words13 || !out_local5e0_raw13 ||
        !out_local5e0_prefix13 || !out_param3_raw13 || !out_param3_prefix13) {
        return -1;
    }

    uint32_t pair[2];
    uint32_t prefix[2];

    uint32_t token = (uint32_t)local5e0_words13[0] * 0x34b89d55u + 0x6b752cf1u;
    post_seventh_local5e0_pair(token, pair);
    out_local5e0_raw13[0][0] = pair[0];
    out_local5e0_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_local5e0_prefix13[0][0] = prefix[0];
    out_local5e0_prefix13[0][1] = prefix[1];
    for (unsigned i = 1u; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = ((uint32_t)local5e0_words13[i] * k_f4190b30[lane] +
                 k_f4190b50[lane]) * 0xa97991bbu + 0xb5ce2f56u;
        post_seventh_local5e0_pair(token, pair);
        out_local5e0_raw13[i][0] = pair[0];
        out_local5e0_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_local5e0_prefix13[i][0] = prefix[0];
        out_local5e0_prefix13[i][1] = prefix[1];
    }

    token = (uint32_t)param3_words13[0] * 0xc51396f3u + 0x082f5b3cu;
    post_seventh_param3_pair(token, pair);
    out_param3_raw13[0][0] = pair[0];
    out_param3_raw13[0][1] = pair[1];
    prefix[0] = pair[0];
    prefix[1] = pair[1];
    out_param3_prefix13[0][0] = prefix[0];
    out_param3_prefix13[0][1] = prefix[1];
    for (unsigned i = 1u; i < L3_AUTH_ROUND_PRIMARY_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        token = ((uint32_t)param3_words13[i] * k_f4190b70[lane] +
                 k_f4190b90[lane]) * 0x737a11b9u + 0x6c71db39u;
        post_seventh_param3_pair(token, pair);
        out_param3_raw13[i][0] = pair[0];
        out_param3_raw13[i][1] = pair[1];
        pair64_add(prefix, pair);
        out_param3_prefix13[i][0] = prefix[0];
        out_param3_prefix13[i][1] = prefix[1];
    }

    if (out) {
        memcpy(out->local5e0_words13, local5e0_words13, sizeof(out->local5e0_words13));
        memcpy(out->param3_words13, param3_words13, sizeof(out->param3_words13));
        memcpy(out->local5e0_raw13, out_local5e0_raw13, sizeof(out->local5e0_raw13));
        memcpy(out->local5e0_prefix13, out_local5e0_prefix13, sizeof(out->local5e0_prefix13));
        memcpy(out->param3_raw13, out_param3_raw13, sizeof(out->param3_raw13));
        memcpy(out->param3_prefix13, out_param3_prefix13, sizeof(out->param3_prefix13));
    }
    return 0;
}

/* v257: post-seventh convolution/reduction body.  This is the native
 * local_578 image immediately after the v256 DAT_f41a14f0/DAT_f41a1570
 * staging streams and immediately before the next FUN_f3fdb57c call.
 */
int l3_authorization_primary_post_seventh_convolution26(
    const uint32_t local5e0_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t local5e0_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t param3_raw13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    const uint32_t param3_prefix13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2]) {
    if (!local5e0_raw13 || !local5e0_prefix13 || !param3_raw13 ||
        !param3_prefix13 || !out_pairs26) {
        return -1;
    }

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_PRIMARY_CONV_PAIRS; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x9b6f1f2bu;
        uint32_t v = 0xa07459ddu;
        if (first <= last) {
            uint32_t conv_lo = 0u;
            uint32_t conv_hi = 0u;
            for (unsigned k = first; k <= last; ++k) {
                const uint32_t *l5 = local5e0_raw13[k];
                const uint32_t *p3 = param3_raw13[idx - k];
                uint64_t prod = (uint64_t)l5[0] * (uint64_t)p3[0];
                uint32_t old = conv_lo;
                conv_lo += (uint32_t)prod;
                conv_hi += p3[1] * l5[0] + p3[0] * l5[1] +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv_lo < old);
            }

            uint32_t conv_scaled_lo = conv_lo * 0xde5524cfu;
            uint32_t l5_sum[2];
            pair_range_sum(local5e0_prefix13, first, last, l5_sum);
            uint32_t p3_sum[2];
            pair_range_sum(param3_prefix13, idx - last, idx - first, p3_sum);

            uint32_t len = (last - first) + 1u;
            uint64_t len_prod = (uint64_t)len * 0x8e83b600u;
            uint32_t len_lo = (uint32_t)len_prod;
            uint32_t len_hi = (uint32_t)(len_prod >> 32);
            uint32_t term0 = len_lo + 0x9b6f1f2bu;
            uint32_t term1 = term0 + conv_scaled_lo;
            uint32_t l5_scaled_lo = l5_sum[0] * 0xe534b6b8u;
            uint32_t term2 = term1 + l5_scaled_lo;
            uint32_t p3_scaled_lo = p3_sum[0] * 0x3294bcc0u;
            u = term2 + p3_scaled_lo;

            v = len * 0xe6b63a2du + len_hi + 0xa07459ddu +
                add_carry_u32(len_lo, 0x9b6f1f2bu) +
                conv_hi * 0xde5524cfu + conv_lo * 0x11e4517au +
                multiply_high_u32(conv_lo, 0xde5524cfu) + add_carry_u32(term0, conv_scaled_lo) +
                l5_sum[1] * 0xe534b6b8u + l5_sum[0] * 0x3065bc46u +
                multiply_high_u32(l5_sum[0], 0xe534b6b8u) + add_carry_u32(term1, l5_scaled_lo) +
                p3_sum[1] * 0x3294bcc0u + p3_sum[0] * 0xc350320fu +
                multiply_high_u32(p3_sum[0], 0x3294bcc0u) + add_carry_u32(term2, p3_scaled_lo);
        }

        uint32_t out_lo_mul = u * 0xe14f537du;
        out_pairs26[idx][0] = out_lo_mul + 0x20bd6421u;
        out_pairs26[idx][1] = v * 0xe14f537du + u * 0x4c39fe23u +
                              multiply_high_u32(u, 0xe14f537du) + 0xe98f96eau +
                              add_carry_u32(out_lo_mul, 0x20bd6421u);
    }
    return 0;
}



/* v258: compose the post-seventh convolution image with the final/eighth
 * FUN_f3fdb57c update+export call-site. */
int l3_authorization_primary_eighth_vector_reduce_update_export(
    const int32_t local5e0_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_pre_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2],
    uint32_t out_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_eighth_vector_reduce_trace *out) {
    if (!local5e0_words13 || !param3_words13 || !vector_reduce_param1_13 ||
        !out_pre_state26 || !out_state26 || !out_words13) {
        return -1;
    }

    uint32_t lraw[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t lpref[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t praw[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t ppref[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    l3_authorization_primary_post_seventh_staging_trace local_seed;
    l3_authorization_primary_post_seventh_staging_trace *seed_dst = out ? &out->seed : &local_seed;

    int rc = l3_authorization_primary_post_seventh_staging_pair_streams13(
        local5e0_words13, param3_words13, lraw, lpref, praw, ppref, seed_dst);
    if (rc != 0) return rc;

    rc = l3_authorization_primary_post_seventh_convolution26(lraw, lpref, praw, ppref, out_pre_state26);
    if (rc != 0) return rc;

    if (out) {
        memcpy(out->pre_vector_reduce_state26, out_pre_state26, sizeof(out->pre_vector_reduce_state26));
    }
    memcpy(out_state26, out_pre_state26, L3_AUTH_ROUND_PRIMARY_CONV_PAIRS * 2u * sizeof(uint32_t));

    return l3_authorization_vector_reduce_run13_update_export(
        vector_reduce_param1_13, vector_reduce_param3, vector_reduce_param4,
        out_state26, out_words13, out ? &out->f3fdb57c : NULL);
}

/* v259: final post-eighth export affine into native param_7. */
static void post_eighth_make_param7_words(
    const uint32_t eighth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]) {
    static const uint32_t k_a[4] = {
        0x5b1fe849u, 0x25947e5fu, 0x5bdb441bu, 0x58299c73u
    };
    static const uint32_t add_a[4] = {
        0xd2f19c50u, 0x57e22f32u, 0x20a9b9b5u, 0xa66640cfu
    };
    static const uint32_t k_b[4] = {
        0xc47bc9f5u, 0xd1c47071u, 0x24537675u, 0xd4ea6933u
    };
    static const uint32_t add_b[4] = {
        0x84bcf927u, 0xc4779807u, 0x398d8cbdu, 0xc10670d2u
    };
    for (unsigned i = 0; i < 4u; ++i) {
        out_param7_words13[i] = eighth_words13[i] * k_a[i] + add_a[i];
        out_param7_words13[i + 4u] =
            eighth_words13[i + 4u] * k_b[i] + add_b[i];
        out_param7_words13[i + 8u] =
            eighth_words13[i + 8u] * k_a[i] + add_a[i];
    }
    out_param7_words13[12] = eighth_words13[12] * 0xc47bc9f5u + 0x84bcf927u;
}

int l3_authorization_primary_post_eighth_param7_words13(
    const uint32_t eighth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_eighth_param7_trace *out) {
    if (!eighth_words13 || !out_param7_words13) {
        return -1;
    }
    post_eighth_make_param7_words(eighth_words13, out_param7_words13);
    if (out) {
        memcpy(out->eighth_words13, eighth_words13, sizeof(out->eighth_words13));
        memcpy(out->param7_words13, out_param7_words13, sizeof(out->param7_words13));
    }
    return 0;
}


/* v260: composed terminal param_6/param_7 tail wrapper. */
int l3_authorization_primary_terminal_param6_param7_words13(
    const uint32_t seventh_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t local5e0_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t param3_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t vector_reduce_param3,
    uint32_t vector_reduce_param4,
    uint32_t out_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_eighth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_terminal_param6_param7_trace *out) {
    if (!seventh_words13 || !local5e0_words13 || !param3_words13 ||
        !vector_reduce_param1_13 || !out_param6_words13 || !out_eighth_words13 ||
        !out_param7_words13) {
        return -1;
    }

    l3_authorization_primary_terminal_param6_param7_trace tmp;
    l3_authorization_primary_terminal_param6_param7_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_primary_post_seventh_param6_words13(
        seventh_words13, out_param6_words13, &dst->param6);
    if (rc != 0) return rc;

    uint32_t pre_state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    uint32_t state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    rc = l3_authorization_primary_eighth_vector_reduce_update_export(
        local5e0_words13, param3_words13, vector_reduce_param1_13,
        vector_reduce_param3, vector_reduce_param4,
        pre_state26, state26, out_eighth_words13, &dst->eighth);
    if (rc != 0) return rc;

    return l3_authorization_primary_post_eighth_param7_words13(
        out_eighth_words13, out_param7_words13, &dst->param7);
}


/* v264: compose the post-sixth source-to-seventh segment with the terminal
 * param_6/eighth/param_7 export wrapper. */
int l3_authorization_primary_post_sixth_terminal_words13(
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_staging_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t param4_mix_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS],
    const int32_t seventh_vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t seventh_vector_reduce_param3,
    uint32_t seventh_vector_reduce_param4,
    const int32_t eighth_local5e0_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t eighth_param3_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const int32_t eighth_vector_reduce_param1_13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t eighth_vector_reduce_param3,
    uint32_t eighth_vector_reduce_param4,
    uint32_t out_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_eighth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    l3_authorization_primary_post_sixth_terminal_trace *out) {
    if (!live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !sixth_words13 || !param4_param5_words13 || !param4_staging_words13 ||
        !affine_words13 || !param4_mix_words26 || !local3a0_words26 ||
        !seventh_vector_reduce_param1_13 || !eighth_local5e0_words13 ||
        !eighth_param3_words13 || !eighth_vector_reduce_param1_13 ||
        !out_param5_words13 || !out_param6_words13 || !out_eighth_words13 ||
        !out_param7_words13) {
        return -1;
    }

    l3_authorization_primary_post_sixth_terminal_trace tmp;
    l3_authorization_primary_post_sixth_terminal_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t staging_source13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t f12b0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t mixed_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t local718_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    uint32_t state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    uint32_t seventh_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];

    int rc = l3_authorization_primary_post_sixth_composed_seventh_update_export(
        live_q6_words4, live_q5_words4, live_q4_words4, sixth_words13,
        param4_param5_words13, param4_staging_words13, affine_words13,
        param4_mix_words26, local3a0_words26, seventh_vector_reduce_param1_13,
        seventh_vector_reduce_param3, seventh_vector_reduce_param4,
        out_param5_words13, staging_source13, f12b0_words26, mixed_words26,
        local718_words26, state26, seventh_words13, &dst->post_sixth);
    if (rc != 0) {
        return rc;
    }

    return l3_authorization_primary_terminal_param6_param7_words13(
        seventh_words13, eighth_local5e0_words13, eighth_param3_words13,
        eighth_vector_reduce_param1_13, eighth_vector_reduce_param3,
        eighth_vector_reduce_param4, out_param6_words13, out_eighth_words13,
        out_param7_words13, &dst->terminal);
}

int l3_authorization_primary_run(
    const uint32_t input_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t input_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const uint32_t input_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    const void *parent_context,
    uint32_t out_param5_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param6_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS],
    uint32_t out_param7_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS]) {
    if (!input_param5_words13 || !input_param6_words13 ||
        !input_param7_words13 || !parent_context || !out_param5_words13 ||
        !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    const uint8_t *context = (const uint8_t *)parent_context;
    const int32_t *context_param1 =
        (const int32_t *)(const void *)(context + 0x090u);
    uint32_t scalar0;
    uint32_t scalar1;
    memcpy(&scalar0, context + 0x268u, sizeof(scalar0));
    memcpy(&scalar1, context + 0x26cu, sizeof(scalar1));

    uint32_t state26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS][2];
    uint32_t first_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t second_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t current_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t next_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t affine_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];

    int rc = l3_authorization_primary_first_vector_reduce_update_export(
        (const int32_t *)input_param6_words13, context_param1,
        scalar0, scalar1, state26, first_words13, NULL);
    if (rc != 0) return rc;

    rc = l3_authorization_primary_second_vector_reduce_update_export(
        first_words13, (const int32_t *)input_param5_words13,
        context_param1, scalar0, scalar1, state26, second_words13, NULL);
    if (rc != 0) return rc;

    uint32_t live_q6_words4[4];
    uint32_t live_q5_words4[4];
    uint32_t live_q4_words4[4];
    rc = l3_authorization_primary_live_q654_from_second_words13(
        second_words13, live_q6_words4, live_q5_words4, live_q4_words4);
    if (rc != 0) return rc;

    rc = l3_authorization_primary_third_vector_reduce_update_export(
        second_words13, (const int32_t *)input_param7_words13,
        context_param1, scalar0, scalar1, state26, current_words13, NULL);
    if (rc != 0) return rc;

    rc = l3_authorization_primary_fourth_vector_reduce_update_export(
        current_words13, context_param1, scalar0, scalar1,
        state26, next_words13, NULL);
    if (rc != 0) return rc;

    rc = l3_authorization_primary_fifth_vector_reduce_update_export(
        next_words13, (const int32_t *)input_param5_words13,
        (const int32_t *)(const void *)(context + 0x05cu),
        context_param1, scalar0, scalar1, state26, current_words13, NULL);
    if (rc != 0) return rc;

    rc = l3_authorization_primary_sixth_vector_reduce_update_export(
        current_words13, context_param1, scalar0, scalar1,
        affine_words13, state26, next_words13, NULL);
    if (rc != 0) return rc;

    uint32_t local5ac_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t local5ac_pairs13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t local3a0_words26[L3_AUTH_ROUND_PRIMARY_CONV_PAIRS];
    rc = l3_authorization_primary_post_sixth_local3a0_words26(
        first_words13, local5ac_words13, local5ac_pairs13,
        local3a0_words26);
    if (rc != 0) return rc;

    int32_t eighth_local5e0_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t seed_raw1[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t seed_prefix1[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t seed_raw2[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    uint32_t seed_prefix2[L3_AUTH_ROUND_PRIMARY_SEED_WORDS][2];
    rc = l3_authorization_primary_param6_seed_prefixes13(
        (const int32_t *)input_param6_words13, eighth_local5e0_words13,
        seed_raw1, seed_prefix1, seed_raw2, seed_prefix2);
    if (rc != 0) return rc;

    uint32_t result5[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t result6[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t result7[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    uint32_t eighth_words13[L3_AUTH_ROUND_PRIMARY_SEED_WORDS];
    rc = l3_authorization_primary_post_sixth_terminal_words13(
        live_q6_words4, live_q5_words4, live_q4_words4, next_words13,
        (const uint32_t *)(const void *)(context + 0x0f8u),
        (const uint32_t *)(const void *)(context + 0x12cu),
        affine_words13,
        (const uint32_t *)(const void *)(context + 0x1fcu),
        local3a0_words26, context_param1, scalar0, scalar1,
        eighth_local5e0_words13, (const int32_t *)input_param7_words13,
        context_param1, scalar0, scalar1, result5, result6, eighth_words13,
        result7, NULL);
    if (rc != 0) return rc;

    memcpy(out_param5_words13, result5, sizeof(result5));
    memcpy(out_param6_words13, result6, sizeof(result6));
    memcpy(out_param7_words13, result7, sizeof(result7));
    return 0;
}
