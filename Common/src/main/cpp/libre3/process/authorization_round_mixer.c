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
#include "authorization_round_mixer.h"

#include "p256_scalar_multiply.h"

#include <stddef.h>
#include <string.h>

static uint32_t load_le32(const uint8_t *p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void store_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint32_t mla32(int32_t x, uint32_t mul, uint32_t add) {
    return (uint32_t)((uint32_t)x * mul + add);
}

static uint64_t pair_words_to_u64(const int32_t words[2]) {
    return (uint64_t)(uint32_t)words[0] |
           (uint64_t)(uint32_t)words[1] << 32;
}

static uint32_t add_carry_u32(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a + (uint64_t)b) >> 32);
}

static uint32_t multiply_high_u32(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a * (uint64_t)b) >> 32);
}

static uint32_t multiply_low_u32(uint32_t a, uint32_t b) {
    return (uint32_t)((uint64_t)a * (uint64_t)b);
}

int l3_authorization_core_pair_reduce_affine_setup(
    const int32_t left13[L3_AUTH_ROUND_CORE_FC2304_WORDS],
    const int32_t right13[L3_AUTH_ROUND_CORE_FC2304_WORDS],
    const uint8_t *parent_ctx,
    uint8_t aux48[L3_AUTH_ROUND_CORE_AUX_BYTES],
    int32_t vec_a13[L3_AUTH_ROUND_CORE_VEC_WORDS],
    int32_t vec_b13[L3_AUTH_ROUND_CORE_VEC_WORDS]) {
    if (!left13 || !right13 || !parent_ctx || !aux48 || !vec_a13 || !vec_b13) return -1;

    static const uint32_t aux_mul_a[4] = {0x15d10b37u, 0x6631029fu, 0xd731557bu, 0x7bacb18fu};
    static const uint32_t aux_add_a[4] = {0xc15925dau, 0x7bca13b6u, 0x7955486eu, 0x68a919b3u};
    static const uint32_t aux_mul_b[4] = {0x86a44cffu, 0x8e949285u, 0xc9b94179u, 0x428bfda7u};
    static const uint32_t aux_add_b[4] = {0x1a31337bu, 0x75452132u, 0xe950deaeu, 0xa190c717u};

    static const uint32_t va_mul_a[4] = {0xaa063d97u, 0xf6eb9115u, 0x228d6731u, 0x101d70cdu};
    static const uint32_t va_add_a[4] = {0x921ff676u, 0xc041b05fu, 0x0ff21b06u, 0x7a90d542u};
    static const uint32_t va_mul_b[4] = {0x02191137u, 0xd5368bbfu, 0x8e74789bu, 0xa021ecfbu};
    static const uint32_t va_add_b[4] = {0x04559c88u, 0x37d4aa0du, 0x4d5c254du, 0x2b74a6d7u};

    for (unsigned i = 0; i < 4u; ++i) {
        store_le32(aux48 + i * 4u, mla32(left13[i], aux_mul_a[i], aux_add_a[i]));
        store_le32(aux48 + 0x10u + i * 4u, mla32(left13[4u + i], aux_mul_b[i], aux_add_b[i]));
        store_le32(aux48 + 0x20u + i * 4u, mla32(left13[8u + i], aux_mul_a[i], aux_add_a[i]));

        vec_a13[i] = (int32_t)mla32(right13[i], va_mul_a[i], va_add_a[i]);
        vec_a13[4u + i] = (int32_t)mla32(right13[4u + i], va_mul_b[i], va_add_b[i]);
        vec_a13[8u + i] = (int32_t)mla32(right13[8u + i], va_mul_a[i], va_add_a[i]);
    }
    /* Native scalar lanes after the three vector stores. */
    vec_a13[12] = (int32_t)mla32(right13[12], 0x02191137u, 0x04559c88u);

    /* Native initial param_7 affine from parent context offsets 0x3c..0x64. */
    static const uint32_t vb_mul[10] = {
        0xf30d5757u, 0x698fbc57u, 0xcd44dda5u, 0xf14a103fu, 0xe4805e25u,
        0xdabef6edu, 0xe0c6025bu, 0xa9aa99e1u, 0xf30d5757u, 0x698fbc57u
    };
    static const uint32_t vb_add[10] = {
        0x8b31c45bu, 0xe3ff83d3u, 0x1c22ceedu, 0x4a2df598u, 0xb9a15628u,
        0x07f1232bu, 0x6e1f9b02u, 0x25c9c90bu, 0x8b31c45bu, 0xe3ff83d3u
    };
    for (unsigned i = 0; i < 10u; ++i) {
        uint32_t w = load_le32(parent_ctx + 0x3cu + i * 4u);
        vec_b13[i] = (int32_t)(w * vb_mul[i] + vb_add[i]);
    }
    vec_b13[10] = (int32_t)0x6e35d846u;
    vec_b13[11] = (int32_t)0xb9486634u;
    vec_b13[12] = (int32_t)0xd0768182u;
    return 0;
}



int l3_authorization_core_pair_reduce_pair_affine_native(
    const int32_t first_left10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS],
    const int32_t second_left10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS],
    const int32_t shared_right13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS],
    const int32_t shared_param3[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS],
    uint32_t param5,
    uint32_t param6,
    const uint8_t *full_parent_ctx,
    int32_t out_first_words13[L3_AUTH_ROUND_CORE_FC2304_WORDS],
    int32_t out_second_words13[L3_AUTH_ROUND_CORE_FC2304_WORDS],
    uint8_t aux48[L3_AUTH_ROUND_CORE_AUX_BYTES],
    int32_t vec_a13[L3_AUTH_ROUND_CORE_VEC_WORDS],
    int32_t vec_b13[L3_AUTH_ROUND_CORE_VEC_WORDS]) {
    if (!first_left10 || !second_left10 || !shared_right13 || !shared_param3 ||
        !full_parent_ctx || !out_first_words13 || !out_second_words13 ||
        !aux48 || !vec_a13 || !vec_b13) {
        return -1;
    }

    uint32_t first_u[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS];
    uint32_t second_u[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS];
    if (l3_p256_coordinate_reduce_full_words13_native(
            first_left10, shared_right13, shared_param3,
            param5, param6, first_u) != 0 ||
        l3_p256_coordinate_reduce_full_words13_native(
            second_left10, shared_right13, shared_param3,
            param5, param6, second_u) != 0) {
        return -1;
    }

    for (unsigned i = 0; i < L3_AUTH_ROUND_CORE_FC2304_WORDS; ++i) {
        out_first_words13[i] = (int32_t)first_u[i];
        out_second_words13[i] = (int32_t)second_u[i];
    }
    return l3_authorization_core_pair_reduce_affine_setup(
        out_first_words13, out_second_words13, full_parent_ctx + 0x268u,
        aux48, vec_a13, vec_b13);
}

int l3_authorization_prepare_round_pipeline(
    const uint8_t *full_parent_ctx,
    const uint8_t left66[0x42],
    const uint8_t right66[0x42],
    const uint8_t scalar35[35],
    int32_t out_first_words13[L3_AUTH_ROUND_CORE_FC2304_WORDS],
    int32_t out_second_words13[L3_AUTH_ROUND_CORE_FC2304_WORDS],
    uint32_t out_param5_words13[L3_AUTH_ROUND_CORE_VEC_WORDS],
    uint32_t out_param6_words13[L3_AUTH_ROUND_CORE_VEC_WORDS],
    uint32_t out_param7_words13[L3_AUTH_ROUND_CORE_VEC_WORDS],
    l3_authorization_vector_seed *out_initial_param5_seed) {
    if (!full_parent_ctx || !left66 || !right66 || !scalar35 ||
        !out_first_words13 || !out_second_words13 || !out_param5_words13 ||
        !out_param6_words13 || !out_param7_words13 ||
        !out_initial_param5_seed) {
        return -1;
    }

    int32_t first_lanes10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS];
    int32_t second_lanes10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS];
    int32_t shared_right13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS];
    int32_t shared_param3[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS];
    if (l3_authorization_core_initial_lanes10(
            full_parent_ctx, left66, right66, scalar35,
            first_lanes10, second_lanes10) != 0) {
        return -1;
    }

    for (unsigned i = 0; i < L3_AUTH_PAIR_REDUCE_RIGHT_WORDS; ++i) {
        shared_right13[i] =
            (int32_t)load_le32(full_parent_ctx + 0x270u + 4u * i);
        shared_param3[i] =
            (int32_t)load_le32(full_parent_ctx + 0x90u + 4u * i);
    }

    uint8_t aux48[L3_AUTH_ROUND_CORE_AUX_BYTES];
    int32_t vec_a13[L3_AUTH_ROUND_CORE_VEC_WORDS];
    int32_t vec_b13[L3_AUTH_ROUND_CORE_VEC_WORDS];
    const int rc = l3_authorization_core_pair_reduce_pair_affine_native(
        first_lanes10, second_lanes10, shared_right13, shared_param3,
        load_le32(full_parent_ctx + 0x268u),
        load_le32(full_parent_ctx + 0x26cu),
        full_parent_ctx, out_first_words13, out_second_words13,
        aux48, vec_a13, vec_b13);
    if (rc != 0) return rc;

    for (unsigned i = 0; i < 12u; ++i) {
        out_param5_words13[i] = load_le32(aux48 + 4u * i);
    }
    out_param5_words13[12] =
        mla32(out_first_words13[12], 0x86a44cffu, 0x1a31337bu);
    for (unsigned i = 0; i < 13u; ++i) {
        out_param6_words13[i] = (uint32_t)vec_a13[i];
        out_param7_words13[i] = (uint32_t)vec_b13[i];
    }

    /* At the first loop entry local_4c0/local_4d0 still contain the raw
     * second FUN_f3fc2304 vector groups, while auVar98 contains the affine
     * param_6 middle vector. */
    out_initial_param5_seed->high_halves2[0] =
        pair_words_to_u64(out_second_words13 + 8u);
    out_initial_param5_seed->high_halves2[1] =
        pair_words_to_u64(out_second_words13 + 10u);
    out_initial_param5_seed->low_halves2[0] =
        pair_words_to_u64(out_second_words13 + 4u);
    out_initial_param5_seed->low_halves2[1] =
        pair_words_to_u64(out_second_words13 + 6u);
    out_initial_param5_seed->middle_low_halves2[0] =
        (uint64_t)out_param6_words13[4] |
        (uint64_t)out_param6_words13[5] << 32;
    out_initial_param5_seed->middle_low_halves2[1] =
        (uint64_t)out_param6_words13[6] |
        (uint64_t)out_param6_words13[7] << 32;
    return 0;
}

/* v299 constants: FUN_f3fbc5c0 post-FUN_f3fcc2d0 accumulator seam. */
static const l3_authorization_word_pair k_mixer_core_f41a2630[16] = {
    {0xe61fe94cu, 0xdf71d7c8u}, {0xe6733161u, 0x6821f956u},
    {0xe6c67976u, 0xf0d21ae4u}, {0xe1e5403cu, 0x8e802392u},
    {0xe2388851u, 0x17304520u}, {0xe28bd066u, 0xafe066aeu},
    {0xe2df187bu, 0x3890883cu}, {0xe3326090u, 0xc140a9cau},
    {0xe385a8a5u, 0x59f0cb58u}, {0xe3d8f0bau, 0xe2a0ece6u},
    {0xe42c38cfu, 0x7b510e74u}, {0xe47f80e4u, 0x04013002u},
    {0xe4d2c8f9u, 0x9cb15190u}, {0xe526110eu, 0x2561731eu},
    {0xe5795923u, 0xbe1194acu}, {0xe5cca138u, 0x46c1b63au}
};

static const l3_authorization_word_pair k_mixer_core_f41a26b0[16] = {
    {0x84170cf4u, 0xf5eac9edu}, {0xd9ef3d80u, 0x6e3b7b61u},
    {0x2fc76e0cu, 0xd68c2cd6u}, {0x859f9e98u, 0x4edcde4au},
    {0xdb77cf24u, 0xb72d8fbeu}, {0x314fffb0u, 0x2f7e4133u},
    {0x8728303cu, 0x97cef2a7u}, {0xdd0060c8u, 0x001fa41bu},
    {0x32d89154u, 0x78705590u}, {0x88b0c1e0u, 0xe0c10704u},
    {0xde88f26cu, 0x5911b878u}, {0x346122f8u, 0xc16269edu},
    {0x2cb64ac5u, 0x34a8041cu}, {0x828e7b51u, 0xacf8b590u},
    {0xd866abddu, 0x15496704u}, {0x2e3edc69u, 0x8d9a1879u}
};

static const uint32_t k_mixer_core_f4191410[8] = {
    0xdb594885u, 0x2451fbb3u, 0xaf4965b9u, 0x77452029u,
    0x68fe9e81u, 0x6c421d0du, 0x8140266bu, 0x2029fdddu
};
static const uint32_t k_mixer_core_f4191430[8] = {
    0x72fb9d1eu, 0xb130fa8fu, 0xe3d485ceu, 0x0095f1e6u,
    0x54975b3eu, 0x1cabda7eu, 0x2d279520u, 0xd4462ae1u
};
static const uint32_t k_mixer_core_f4191450[8] = {
    0x189c3483u, 0x3a67935bu, 0xba708d83u, 0xc9dc20bbu,
    0xc7c90f1fu, 0x3b9ea1ddu, 0x8991829fu, 0x57b605b3u
};
static const uint32_t k_mixer_core_f4191470[8] = {
    0x54a6f12du, 0xea45169cu, 0x56dc4d5cu, 0x1e70dc7fu,
    0x3a717d9eu, 0xa7b97c8cu, 0xb973290au, 0xf229790bu
};

static l3_authorization_word_pair mixer_core_fold_step(l3_authorization_word_pair v,
                                                  const l3_authorization_word_pair table[16]) {
    const unsigned idx = v.lo & 0x0fu;
    const uint32_t shifted_lo = (v.lo >> 4) | (v.hi << 28);
    const uint32_t shifted_hi = v.hi >> 4;
    l3_authorization_word_pair out;
    out.lo = shifted_lo + table[idx].lo;
    out.hi = shifted_hi + table[idx].hi + add_carry_u32(shifted_lo, table[idx].lo);
    return out;
}

static l3_authorization_word_pair mixer_core_fold_rounds(l3_authorization_word_pair v,
                                                    const l3_authorization_word_pair table[16],
                                                    unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) v = mixer_core_fold_step(v, table);
    return v;
}

static l3_authorization_word_pair mixer_core_make_2630_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0x43fb4a2bu);
    l3_authorization_word_pair seed = {
        lo0 + 0xc27df65cu,
        (uint32_t)(x * 0x44387bc7u + multiply_high_u32(x, 0x43fb4a2bu) +
                   0x48f5f11du + add_carry_u32(lo0, 0xc27df65cu))
    };
    const uint32_t folded_low = mixer_core_fold_rounds(seed, k_mixer_core_f41a2630, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0xb9a810f3u);
    l3_authorization_word_pair out = {
        lo1 + 0x6b36f33bu,
        (uint32_t)(x * 0xf6611835u + multiply_high_u32(x, 0xb9a810f3u) +
                   0x49db7f20u + add_carry_u32(lo1, 0x6b36f33bu) +
                   folded_low * 0xb65a27a7u)
    };
    return out;
}

static l3_authorization_word_pair mixer_core_make_26b0_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0x9156d639u);
    l3_authorization_word_pair seed = {
        lo0 + 0xb7a8c6dau,
        (uint32_t)(x * 0xe8e893dfu + multiply_high_u32(x, 0x9156d639u) +
                   0xe46ac0cau + add_carry_u32(lo0, 0xb7a8c6dau))
    };
    const uint32_t folded_low = mixer_core_fold_rounds(seed, k_mixer_core_f41a26b0, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0x90233bcdu);
    l3_authorization_word_pair out = {
        lo1 + 0x370f35d4u,
        (uint32_t)(x * 0x02f8bf79u + multiply_high_u32(x, 0x90233bcdu) +
                   0xbac8c1f7u + add_carry_u32(lo1, 0x370f35d4u) +
                   folded_low * 0x00200dcbu)
    };
    return out;
}

static l3_authorization_word_pair mixer_core_pair_add(l3_authorization_word_pair a,
                                                 l3_authorization_word_pair b) {
    l3_authorization_word_pair out;
    out.lo = a.lo + b.lo;
    out.hi = a.hi + b.hi + add_carry_u32(a.lo, b.lo);
    return out;
}

int l3_authorization_core_local514_accum2630_pairs13(
    const int32_t local514[13],
    l3_authorization_word_pair raw2630[13],
    l3_authorization_word_pair prefix2630[13]) {
    if (!local514 || !raw2630 || !prefix2630) return -1;
    uint32_t x = (uint32_t)local514[0] * 0x9286076bu + 0xec96638bu;
    raw2630[0] = mixer_core_make_2630_pair(x);
    prefix2630[0] = raw2630[0];
    for (unsigned lane = 1; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        uint32_t pre = (uint32_t)local514[lane] * k_mixer_core_f4191410[idx] +
                       k_mixer_core_f4191430[idx];
        x = pre * 0x8c340b2fu + 0xae8c4109u;
        raw2630[lane] = mixer_core_make_2630_pair(x);
        prefix2630[lane] = mixer_core_pair_add(prefix2630[lane - 1u], raw2630[lane]);
    }
    return 0;
}

int l3_authorization_core_roundctx_accum26b0_pairs13(
    const int32_t round_words13[13],
    l3_authorization_word_pair raw26b0[13],
    l3_authorization_word_pair prefix26b0[13]) {
    if (!round_words13 || !raw26b0 || !prefix26b0) return -1;
    uint32_t x = (uint32_t)round_words13[0] * 0x1ee6afcdu + 0x4b669849u;
    raw26b0[0] = mixer_core_make_26b0_pair(x);
    prefix26b0[0] = raw26b0[0];
    for (unsigned lane = 1; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        uint32_t pre = (uint32_t)round_words13[lane] * k_mixer_core_f4191450[idx] +
                       k_mixer_core_f4191470[idx];
        x = pre * 0xfd1e796fu + 0xf2bfc0c6u;
        raw26b0[lane] = mixer_core_make_26b0_pair(x);
        prefix26b0[lane] = mixer_core_pair_add(prefix26b0[lane - 1u], raw26b0[lane]);
    }
    return 0;
}

int l3_authorization_core_post_secondary_mix_accum2630_26b0_pairs13(
    const int32_t local514[13],
    const int32_t round_words13[13],
    l3_authorization_word_pair raw2630[13],
    l3_authorization_word_pair prefix2630[13],
    l3_authorization_word_pair raw26b0[13],
    l3_authorization_word_pair prefix26b0[13],
    l3_authorization_core_post_secondary_mix_accum_trace *trace) {
    if (!local514 || !round_words13 || !raw2630 || !prefix2630 || !raw26b0 || !prefix26b0) return -1;
    int rc = l3_authorization_core_local514_accum2630_pairs13(local514, raw2630, prefix2630);
    if (rc) return rc;
    rc = l3_authorization_core_roundctx_accum26b0_pairs13(round_words13, raw26b0, prefix26b0);
    if (rc) return rc;
    if (trace) {
        memcpy(trace->local514_raw2630, raw2630, sizeof(trace->local514_raw2630));
        memcpy(trace->local514_prefix2630, prefix2630, sizeof(trace->local514_prefix2630));
        memcpy(trace->round_raw26b0, raw26b0, sizeof(trace->round_raw26b0));
        memcpy(trace->round_prefix26b0, prefix26b0, sizeof(trace->round_prefix26b0));
    }
    return 0;
}


/* ---- v300: FUN_f3fbc5c0 post-FUN_f3fcc2d0 convolution/reduction ---- */

static void mixer_pair_range_sum(
    const l3_authorization_word_pair prefix13[L3_AUTH_ROUND_CORE_PAIRS13],
    unsigned first,
    unsigned last,
    l3_authorization_word_pair *out) {
    out->lo = prefix13[last].lo;
    out->hi = prefix13[last].hi;
    if (first != 0u) {
        const uint32_t borrow = (out->lo < prefix13[first - 1u].lo);
        out->lo -= prefix13[first - 1u].lo;
        out->hi -= prefix13[first - 1u].hi + borrow;
    }
}

int l3_authorization_core_post_secondary_mix_convolution26(
    const l3_authorization_word_pair raw2630_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair prefix2630_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair raw26b0_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair prefix26b0_13[L3_AUTH_ROUND_CORE_PAIRS13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    l3_authorization_core_post_secondary_mix_convolution26_trace *out) {
    if (!raw2630_13 || !prefix2630_13 || !raw26b0_13 || !prefix26b0_13 || !out_pairs26) return -1;

    l3_authorization_core_post_secondary_mix_convolution26_trace tmp;
    l3_authorization_core_post_secondary_mix_convolution26_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_CORE_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x22b1c58fu;
        uint32_t v = 0x67b719ecu;
        l3_authorization_word_pair conv = {0u, 0u};
        l3_authorization_word_pair sum2630 = {0u, 0u};
        l3_authorization_word_pair sum26b0 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_word_pair *a2630 = &raw2630_13[k];
                const l3_authorization_word_pair *b26b0 = &raw26b0_13[idx - k];
                const uint64_t prod = (uint64_t)a2630->lo * (uint64_t)b26b0->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b26b0->hi * a2630->lo + b26b0->lo * a2630->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            mixer_pair_range_sum(prefix2630_13, first, last, &sum2630);
            mixer_pair_range_sum(prefix26b0_13, idx - last, idx - first, &sum26b0);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0xfa082deau;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0x948a9d0bu;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum2630_scaled = (uint64_t)sum2630.lo * 0x7a8830f6u;
            const uint32_t sum2630_scaled_lo = (uint32_t)sum2630_scaled;
            const uint64_t sum26b0_scaled = (uint64_t)sum26b0.lo * 0x9eb52f65u;
            const uint32_t sum26b0_scaled_lo = (uint32_t)sum26b0_scaled;

            const uint32_t t0 = count_lo + 0x22b1c58fu;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum2630_scaled_lo;
            u = t2 + sum26b0_scaled_lo;

            v = count * 0x621eddd9u + count_hi + 0x67b719ecu +
                add_carry_u32(count_lo, 0x22b1c58fu) +
                conv.hi * 0x948a9d0bu + conv.lo * 0x0958e6b0u +
                (uint32_t)(conv_scaled >> 32) + add_carry_u32(t0, conv_scaled_lo) +
                sum2630.hi * 0x7a8830f6u + sum2630.lo * 0x3b1bddb3u +
                (uint32_t)(sum2630_scaled >> 32) + add_carry_u32(t1, sum2630_scaled_lo) +
                sum26b0.hi * 0x9eb52f65u + sum26b0.lo * 0x0557f754u +
                (uint32_t)(sum26b0_scaled >> 32) + add_carry_u32(t2, sum26b0_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x659458afu;
        out_pairs26[idx].lo = out_lo_mul + 0x8b0149c1u;
        out_pairs26[idx].hi = v * 0x659458afu + u * 0xe3cdc7ffu +
                              multiply_high_u32(u, 0x659458afu) + 0x9520fc85u +
                              add_carry_u32(out_lo_mul, 0x8b0149c1u);
        dst->convolution26[idx] = conv;
        dst->range2630_26[idx] = sum2630;
        dst->range26b0_26[idx] = sum26b0;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_core_post_secondary_mix_accum_convolution26(
    const int32_t local514[13],
    const int32_t round_words13[13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    l3_authorization_core_post_secondary_mix_convolution26_trace *out) {
    if (!local514 || !round_words13 || !out_pairs26) return -1;

    l3_authorization_word_pair raw2630[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair prefix2630[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair raw26b0[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair prefix26b0[L3_AUTH_ROUND_CORE_PAIRS13];

    int rc = l3_authorization_core_post_secondary_mix_accum2630_26b0_pairs13(
        local514, round_words13, raw2630, prefix2630, raw26b0, prefix26b0, NULL);
    if (rc != 0) return rc;
    return l3_authorization_core_post_secondary_mix_convolution26(
        raw2630, prefix2630, raw26b0, prefix26b0, out_pairs26, out);
}

/* ---- v301: FUN_f3fbc5c0 post-V300 local_5b0/round-context accumulator seam ---- */
static const l3_authorization_word_pair k_mixer_core_f41a2730[16] = {
    {0x9a418441u, 0x7ef29ddbu}, {0x288206fbu, 0xa8adbc61u},
    {0xf2c1a14eu, 0xd838a4a3u}, {0xbd013ba1u, 0x07c38ce6u},
    {0x4b41be5bu, 0x317eab6cu}, {0x158158aeu, 0x610993afu},
    {0xa3c1db68u, 0x9ac4b234u}, {0x6e0175bbu, 0xca4f9a77u},
    {0xfc41f875u, 0xf40ab8fcu}, {0xc68192c8u, 0x2395a13fu},
    {0x90c12d1bu, 0x53208982u}, {0x1f01afd5u, 0x8cdba808u},
    {0xe9414a28u, 0xbc66904au}, {0x7781cce2u, 0xe621aed0u},
    {0x41c16735u, 0x15ac9713u}, {0xd001e9efu, 0x4f67b598u}
};

static const l3_authorization_word_pair k_mixer_core_f41a27b0[16] = {
    {0xa41dca61u, 0x8269945cu}, {0xef88858du, 0xba791baau},
    {0x3af340b9u, 0xe288a2f9u}, {0xcfb24926u, 0x199fb562u},
    {0x1b1d0452u, 0x41af3cb1u}, {0x6687bf7eu, 0x79bec3ffu},
    {0xb1f27aaau, 0xa1ce4b4du}, {0xfd5d35d6u, 0xd9ddd29bu},
    {0x48c7f102u, 0x01ed59eau}, {0x9432ac2eu, 0x39fce138u},
    {0xdf9d675au, 0x620c6886u}, {0x2b082286u, 0x9a1befd5u},
    {0x7672ddb2u, 0xc22b7723u}, {0xc1dd98deu, 0xfa3afe71u},
    {0x0d48540au, 0x224a85c0u}, {0x58b30f36u, 0x5a5a0d0eu}
};

static const uint32_t k_mixer_core_f4191490[8] = {
    0xf06ade37u, 0xa92a3d05u, 0x345fa6d5u, 0x21fb2a23u,
    0x28c7edc5u, 0xffd6305bu, 0x1b2541d9u, 0xb87a1291u
};
static const uint32_t k_mixer_core_f41914b0[8] = {
    0xa4dd6635u, 0x7d18dd13u, 0x1ab86a3du, 0xd8b0be6bu,
    0x3867e356u, 0xf7632d42u, 0xf6b0472bu, 0x86c39d08u
};
static const uint32_t k_mixer_core_f41914d0[8] = {
    0xc5929ecfu, 0x007fc0fbu, 0x68790793u, 0xf29d8c07u,
    0x9c67ecb7u, 0x722cdcffu, 0x587b57fdu, 0x15c00f3fu
};
static const uint32_t k_mixer_core_f41914f0[8] = {
    0x74cd3630u, 0x815563e7u, 0xb36528ccu, 0x15ae5243u,
    0xc493269du, 0xb0fbb6a8u, 0x8b129b53u, 0xd1e08b0du
};

static l3_authorization_word_pair mixer_core_make_2730_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0xc16485fdu);
    l3_authorization_word_pair seed = {
        lo0 + 0x2c6e224bu,
        (uint32_t)(x * 0x7ad4d490u + multiply_high_u32(x, 0xc16485fdu) +
                   0x49006ba8u + add_carry_u32(lo0, 0x2c6e224bu))
    };
    const uint32_t folded_low = mixer_core_fold_rounds(seed, k_mixer_core_f41a2730, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0xa2699f1bu);
    l3_authorization_word_pair out = {
        lo1 + 0xc9b66549u,
        (uint32_t)(x * 0x7f04b253u + multiply_high_u32(x, 0xa2699f1bu) +
                   0xfd1f1761u + add_carry_u32(lo1, 0xc9b66549u) +
                   folded_low * 0x425ec709u)
    };
    return out;
}

static l3_authorization_word_pair mixer_core_make_27b0_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0x7ebeb9d5u);
    l3_authorization_word_pair seed = {
        lo0 + 0x83a9a888u,
        (uint32_t)(x * 0xba5ecacfu + multiply_high_u32(x, 0x7ebeb9d5u) +
                   0xd284686eu + add_carry_u32(lo0, 0x83a9a888u))
    };
    const uint32_t folded_low = mixer_core_fold_rounds(seed, k_mixer_core_f41a27b0, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0x8938b6d3u);
    l3_authorization_word_pair out = {
        lo1 + 0x6488cca5u,
        (uint32_t)(x * 0x3acdf727u + multiply_high_u32(x, 0x8938b6d3u) +
                   0x84fc4503u + add_carry_u32(lo1, 0x6488cca5u) +
                   folded_low * 0x3a65e5f9u)
    };
    return out;
}

int l3_authorization_core_local5b0_accum2730_pairs13(
    const int32_t local5b0[13],
    l3_authorization_word_pair raw2730[13],
    l3_authorization_word_pair prefix2730[13]) {
    if (!local5b0 || !raw2730 || !prefix2730) return -1;
    uint32_t x = (uint32_t)local5b0[0] * 0x07d7c7b1u + 0xfdf949a7u;
    raw2730[0] = mixer_core_make_2730_pair(x);
    prefix2730[0] = raw2730[0];
    for (unsigned lane = 1; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        const uint32_t pre = (uint32_t)local5b0[lane] * k_mixer_core_f4191490[idx] +
                             k_mixer_core_f41914b0[idx];
        x = pre * 0x97325557u + 0x3a71f4a4u;
        raw2730[lane] = mixer_core_make_2730_pair(x);
        prefix2730[lane] = mixer_core_pair_add(prefix2730[lane - 1u], raw2730[lane]);
    }
    return 0;
}

int l3_authorization_core_roundctx_accum27b0_pairs13(
    const int32_t round_words13[13],
    l3_authorization_word_pair raw27b0[13],
    l3_authorization_word_pair prefix27b0[13]) {
    if (!round_words13 || !raw27b0 || !prefix27b0) return -1;
    uint32_t x = (uint32_t)round_words13[0] * 0x5fbe037fu + 0x53092a75u;
    raw27b0[0] = mixer_core_make_27b0_pair(x);
    prefix27b0[0] = raw27b0[0];
    for (unsigned lane = 1; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        const uint32_t pre = (uint32_t)round_words13[lane] * k_mixer_core_f41914d0[idx] +
                             k_mixer_core_f41914f0[idx];
        x = pre * 0xe91afc51u + 0x2be3c545u;
        raw27b0[lane] = mixer_core_make_27b0_pair(x);
        prefix27b0[lane] = mixer_core_pair_add(prefix27b0[lane - 1u], raw27b0[lane]);
    }
    return 0;
}

int l3_authorization_core_post_accum2730_27b0_pairs13(
    const int32_t local5b0[13],
    const int32_t round_words13[13],
    l3_authorization_word_pair raw2730[13],
    l3_authorization_word_pair prefix2730[13],
    l3_authorization_word_pair raw27b0[13],
    l3_authorization_word_pair prefix27b0[13],
    l3_authorization_stage1_stage_accumulation_trace *trace) {
    if (!local5b0 || !round_words13 || !raw2730 || !prefix2730 || !raw27b0 || !prefix27b0) return -1;
    int rc = l3_authorization_core_local5b0_accum2730_pairs13(local5b0, raw2730, prefix2730);
    if (rc) return rc;
    rc = l3_authorization_core_roundctx_accum27b0_pairs13(round_words13, raw27b0, prefix27b0);
    if (rc) return rc;
    if (trace) {
        memcpy(trace->local5b0_raw2730, raw2730, sizeof(trace->local5b0_raw2730));
        memcpy(trace->local5b0_prefix2730, prefix2730, sizeof(trace->local5b0_prefix2730));
        memcpy(trace->round_raw27b0, raw27b0, sizeof(trace->round_raw27b0));
        memcpy(trace->round_prefix27b0, prefix27b0, sizeof(trace->round_prefix27b0));
    }
    return 0;
}

/* ---- v302: FUN_f3fbc5c0 post-V300 local_5b0/round-context convolution/reduction ---- */

int l3_authorization_stage1_stage_convolution26(
    const l3_authorization_word_pair raw2730_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair prefix2730_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair raw27b0_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair prefix27b0_13[L3_AUTH_ROUND_CORE_PAIRS13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    l3_authorization_stage1_stage_convolution_trace *out) {
    if (!raw2730_13 || !prefix2730_13 || !raw27b0_13 || !prefix27b0_13 || !out_pairs26) return -1;

    l3_authorization_stage1_stage_convolution_trace tmp;
    l3_authorization_stage1_stage_convolution_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_CORE_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0xc5530f04u;
        uint32_t v = 0x207925c4u;
        l3_authorization_word_pair conv = {0u, 0u};
        l3_authorization_word_pair sum2730 = {0u, 0u};
        l3_authorization_word_pair sum27b0 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_word_pair *a2730 = &raw2730_13[k];
                const l3_authorization_word_pair *b27b0 = &raw27b0_13[idx - k];
                const uint64_t prod = (uint64_t)a2730->lo * (uint64_t)b27b0->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b27b0->hi * a2730->lo + b27b0->lo * a2730->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            mixer_pair_range_sum(prefix2730_13, first, last, &sum2730);
            mixer_pair_range_sum(prefix27b0_13, idx - last, idx - first, &sum27b0);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0x7f92c384u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0xedebb5b9u;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum2730_scaled = (uint64_t)sum2730.lo * 0x551beb5eu;
            const uint32_t sum2730_scaled_lo = (uint32_t)sum2730_scaled;
            const uint64_t sum27b0_scaled = (uint64_t)sum27b0.lo * 0xd555226eu;
            const uint32_t sum27b0_scaled_lo = (uint32_t)sum27b0_scaled;

            const uint32_t t0 = count_lo + 0xc5530f04u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum2730_scaled_lo;
            u = t2 + sum27b0_scaled_lo;

            v = count * 0xb1bfd44bu + count_hi + 0x207925c4u +
                add_carry_u32(count_lo, 0xc5530f04u) +
                conv.hi * 0xedebb5b9u + conv.lo * 0x718720e7u +
                (uint32_t)(conv_scaled >> 32) + add_carry_u32(t0, conv_scaled_lo) +
                sum2730.hi * 0x551beb5eu + sum2730.lo * 0x9a84b987u +
                (uint32_t)(sum2730_scaled >> 32) + add_carry_u32(t1, sum2730_scaled_lo) +
                sum27b0.hi * 0xd555226eu + sum27b0.lo * 0x645be48bu +
                (uint32_t)(sum27b0_scaled >> 32) + add_carry_u32(t2, sum27b0_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x80ecdaa3u;
        out_pairs26[idx].lo = out_lo_mul + 0xbc122b89u;
        out_pairs26[idx].hi = v * 0x80ecdaa3u + u * 0xb9bd8b79u +
                              multiply_high_u32(u, 0x80ecdaa3u) + 0x4cc80b25u +
                              add_carry_u32(out_lo_mul, 0xbc122b89u);
        dst->convolution26[idx] = conv;
        dst->range2730_26[idx] = sum2730;
        dst->range27b0_26[idx] = sum27b0;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_stage1_stage_accumulate_and_convolve26(
    const int32_t local5b0[13],
    const int32_t round_words13[13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    l3_authorization_stage1_stage_convolution_trace *out) {
    if (!local5b0 || !round_words13 || !out_pairs26) return -1;

    l3_authorization_word_pair raw2730[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair prefix2730[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair raw27b0[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair prefix27b0[L3_AUTH_ROUND_CORE_PAIRS13];

    int rc = l3_authorization_core_post_accum2730_27b0_pairs13(
        local5b0, round_words13, raw2730, prefix2730, raw27b0, prefix27b0, NULL);
    if (rc != 0) return rc;
    return l3_authorization_stage1_stage_convolution26(
        raw2730, prefix2730, raw27b0, prefix27b0, out_pairs26, out);
}


/* ---- v303: FUN_f3fbc5c0 post-V302 local_478/local_468/... vector materialization ---- */

static uint64_t mixer_core_vec478_bias_half(void) {
    return UINT64_C(0x025dfeff412df1b0);
}

int l3_authorization_stage1_stage_vector_reduction13(
    const l3_authorization_word_pair low_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const l3_authorization_word_pair high_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const uint64_t local4c0_halves2[2],
    const uint64_t local4d0_halves2[2],
    const uint64_t auvar98_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage1_stage_vector_reduction_trace *out) {
    if (!low_pairs26 || !high_pairs26 || !local4c0_halves2 || !local4d0_halves2 ||
        !auvar98_halves2 || !out_halves13) return -1;

    l3_authorization_stage1_stage_vector_reduction_trace tmp;
    l3_authorization_stage1_stage_vector_reduction_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    const uint32_t mul_low = 0x7e5908b1u;
    const uint64_t mul_low64 = UINT64_C(0x7bb0e96d7e5908b1);
    const uint64_t mul_high64 = UINT64_C(0x07eb30e45bebcb4d);
    const uint64_t bias = mixer_core_vec478_bias_half();

    for (unsigned lane = 0; lane < 13u; ++lane) {
        const unsigned p0 = lane * 2u;
        const unsigned p1 = p0 + 1u;
        const uint64_t high0 =
            (uint64_t)high_pairs26[p0].lo |
            (uint64_t)high_pairs26[p0].hi << 32;
        const uint64_t high1 =
            (uint64_t)high_pairs26[p1].lo |
            (uint64_t)high_pairs26[p1].hi << 32;
        const uint64_t low0 =
            (uint64_t)low_pairs26[p0].lo |
            (uint64_t)low_pairs26[p0].hi << 32;
        const uint64_t low1 =
            (uint64_t)low_pairs26[p1].lo |
            (uint64_t)low_pairs26[p1].hi << 32;
        const uint64_t side_high0 = high0 * mul_high64;
        const uint64_t side_high1 = high1 * mul_high64;
        const uint64_t side_low0 = low0 * mul_low64;
        const uint64_t side_low1 = low1 * mul_low64;

        out_halves13[lane][0] = side_high0 + side_low0 + bias;
        out_halves13[lane][1] = side_high1 + side_low1 + bias;

        dst->input_low_halves13[lane][0] = ((uint64_t)low_pairs26[p0].hi << 32) | low_pairs26[p0].lo;
        dst->input_low_halves13[lane][1] = ((uint64_t)low_pairs26[p1].hi << 32) | low_pairs26[p1].lo;
        dst->input_high_halves13[lane][0] = ((uint64_t)high_pairs26[p0].hi << 32) | high_pairs26[p0].lo;
        dst->input_high_halves13[lane][1] = ((uint64_t)high_pairs26[p1].hi << 32) | high_pairs26[p1].lo;
        dst->side_from_high13[lane][0] = side_high0;
        dst->side_from_high13[lane][1] = side_high1;
        dst->side_from_low13[lane][0] = side_low0;
        dst->side_from_low13[lane][1] = side_low1;
        dst->bias13[lane][0] = bias;
        dst->bias13[lane][1] = bias;
        dst->output_halves13[lane][0] = out_halves13[lane][0];
        dst->output_halves13[lane][1] = out_halves13[lane][1];
    }

    const uint64_t scalar_mul = (uint64_t)low_pairs26[24].lo * (uint64_t)mul_low;
    const uint32_t scalar_arg =
        low_pairs26[24].hi * mul_low +
        low_pairs26[24].lo * 0x7bb0e96du +
        (uint32_t)(scalar_mul >> 32);
    if (out_vector_feedback_scalar_arg) *out_vector_feedback_scalar_arg = scalar_arg;
    dst->vector_feedback_scalar_arg = scalar_arg;
    (void)local4c0_halves2;
    (void)local4d0_halves2;
    (void)auvar98_halves2;
    return 0;
}

int l3_authorization_stage1_stage_convolve_and_reduce_vectors13(
    const l3_authorization_word_pair high_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const int32_t local5b0[13],
    const int32_t round_words13[13],
    const uint64_t local4c0_halves2[2],
    const uint64_t local4d0_halves2[2],
    const uint64_t auvar98_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage1_stage_convolution_trace *conv_trace,
    l3_authorization_stage1_stage_vector_reduction_trace *vec_trace) {
    if (!high_pairs26 || !local5b0 || !round_words13 || !local4c0_halves2 ||
        !local4d0_halves2 || !auvar98_halves2 || !out_halves13) return -1;

    l3_authorization_word_pair low_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26];
    int rc = l3_authorization_stage1_stage_accumulate_and_convolve26(local5b0, round_words13,
                                                           low_pairs26, conv_trace);
    if (rc != 0) return rc;
    return l3_authorization_stage1_stage_vector_reduction13(
        low_pairs26, high_pairs26, local4c0_halves2, local4d0_halves2, auvar98_halves2,
        out_halves13, out_vector_feedback_scalar_arg, vec_trace);
}

/* ---- v308: FUN_f3fbc5c0 post-V307 local_548/round-context accumulator seam ---- */
static const l3_authorization_word_pair k_mixer_core_f41a2ab0[16] = {
    {0x71bcb514u, 0x82182325u}, {0xcba38daau, 0xbaee1561u},
    {0x258a6640u, 0xe3c4079eu}, {0x2c8e89c3u, 0x16ba25e6u},
    {0x86756259u, 0x4f901822u}, {0xe05c3aefu, 0x78660a5eu},
    {0x3a431385u, 0xa13bfc9bu}, {0x41473708u, 0xd4321ae3u},
    {0x9b2e0f9eu, 0x0d080d1fu}, {0xf514e834u, 0x35ddff5bu},
    {0xfc190bb7u, 0x68d41da3u}, {0x55ffe44du, 0x91aa0fe0u},
    {0xafe6bce3u, 0xca80021cu}, {0xb6eae066u, 0xfd762064u},
    {0x10d1b8fcu, 0x264c12a1u}, {0x6ab89192u, 0x5f2204ddu}
};

static const l3_authorization_word_pair k_mixer_core_f41a2b30[16] = {
    {0x6b24d1e9u, 0x5655d261u}, {0x3b0b1026u, 0x8b7186a3u},
    {0x3e76eda0u, 0xb367c6dfu}, {0x0e5d2bddu, 0xe8837b21u},
    {0xde436a1au, 0x1d9f2f62u}, {0xe1af4794u, 0x45956f9eu},
    {0xb19585d1u, 0x7ab123e0u}, {0x817bc40eu, 0xafccd822u},
    {0x84e7a188u, 0xd7c3185eu}, {0x54cddfc5u, 0x0cdecca0u},
    {0x24b41e02u, 0x31fa80e2u}, {0x281ffb7cu, 0x69f0c11eu},
    {0xf80639b9u, 0x9f0c755fu}, {0xc7ec77f6u, 0xc42829a1u},
    {0x97d2b633u, 0xf943dde3u}, {0x9b3e93adu, 0x213a1e1fu}
};

static const uint32_t k_mixer_core_f4191590[8] = {
    0xefed6a17u, 0x5aa7b5a3u, 0x020f09c1u, 0xa8d2a6efu,
    0xdaa0abf7u, 0xea496e9fu, 0x67c70833u, 0x765eeed3u
};
static const uint32_t k_mixer_core_f41915b0[8] = {
    0x5c9ae0ebu, 0x6105aeadu, 0x998a7d2au, 0x6accfb1au,
    0x700ad0d2u, 0x12335731u, 0xc30f6383u, 0x5621c462u
};
static const uint32_t k_mixer_core_f41915d0[8] = {
    0xce1155bfu, 0x291c1577u, 0x1a21a2bfu, 0xe5458e57u,
    0x8064b08bu, 0x7f312021u, 0x0923aa0bu, 0xd754eeafu
};
static const uint32_t k_mixer_core_f41915f0[8] = {
    0x5b6fe5e4u, 0x2fdb7affu, 0xda62b8bfu, 0x00019d9eu,
    0x519f4829u, 0x6c3307afu, 0x6abc5405u, 0x36c3111au
};

static l3_authorization_word_pair mixer_core_make_2ab0_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0x0beb3bcfu);
    l3_authorization_word_pair seed = {
        lo0 + 0xc4e6ccc3u,
        (uint32_t)(x * 0x1e440053u + multiply_high_u32(x, 0x0beb3bcfu) +
                   0x06b5b484u + add_carry_u32(lo0, 0xc4e6ccc3u))
    };
    const uint32_t folded_low = mixer_core_fold_rounds(seed, k_mixer_core_f41a2ab0, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0x929c7ac7u);
    l3_authorization_word_pair out = {
        lo1 + 0x7459312cu,
        (uint32_t)(x * 0xbdb64878u + multiply_high_u32(x, 0x929c7ac7u) +
                   0x11363d9au + add_carry_u32(lo1, 0x7459312cu) +
                   folded_low * 0x8ddfc877u)
    };
    return out;
}

static l3_authorization_word_pair mixer_core_make_2b30_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0x9371a44du);
    l3_authorization_word_pair seed = {
        lo0 + 0x7cb7aa30u,
        (uint32_t)(x * 0x7fc4919eu + multiply_high_u32(x, 0x9371a44du) +
                   0xb68a990cu + add_carry_u32(lo0, 0x7cb7aa30u))
    };
    const uint32_t folded_low = mixer_core_fold_rounds(seed, k_mixer_core_f41a2b30, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0x410aaaa5u);
    l3_authorization_word_pair out = {
        lo1 + 0xf71dd5e4u,
        (uint32_t)(x * 0xfae543cdu + multiply_high_u32(x, 0x410aaaa5u) +
                   0xae70d906u + add_carry_u32(lo1, 0xf71dd5e4u) +
                   folded_low * 0x9c65d447u)
    };
    return out;
}

int l3_authorization_core_local548_accum2ab0_pairs13(
    const int32_t local548[13],
    l3_authorization_word_pair raw2ab0[13],
    l3_authorization_word_pair prefix2ab0[13]) {
    if (!local548 || !raw2ab0 || !prefix2ab0) return -1;
    uint32_t x = (uint32_t)local548[0] * 0xe79938a5u + 0xbf169a79u;
    raw2ab0[0] = mixer_core_make_2ab0_pair(x);
    prefix2ab0[0] = raw2ab0[0];
    for (unsigned lane = 1; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        const uint32_t pre = (uint32_t)local548[lane] * k_mixer_core_f4191590[idx] +
                             k_mixer_core_f41915b0[idx];
        x = pre * 0xcc6e34a3u + 0x69cfa8d8u;
        raw2ab0[lane] = mixer_core_make_2ab0_pair(x);
        prefix2ab0[lane] = mixer_core_pair_add(prefix2ab0[lane - 1u], raw2ab0[lane]);
    }
    return 0;
}

int l3_authorization_core_roundctx_accum2b30_pairs13(
    const int32_t round_mixed13[13],
    l3_authorization_word_pair raw2b30[13],
    l3_authorization_word_pair prefix2b30[13]) {
    if (!round_mixed13 || !raw2b30 || !prefix2b30) return -1;
    uint32_t x = (uint32_t)round_mixed13[0] * 0xdd0eb1d5u + 0xd5343aa6u;
    raw2b30[0] = mixer_core_make_2b30_pair(x);
    prefix2b30[0] = raw2b30[0];
    for (unsigned lane = 1; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        const uint32_t pre = (uint32_t)round_mixed13[lane] * k_mixer_core_f41915d0[idx] +
                             k_mixer_core_f41915f0[idx];
        x = pre * 0x24e4e56bu + 0xb5ba305au;
        raw2b30[lane] = mixer_core_make_2b30_pair(x);
        prefix2b30[lane] = mixer_core_pair_add(prefix2b30[lane - 1u], raw2b30[lane]);
    }
    return 0;
}

int l3_authorization_core_post_accum2ab0_2b30_pairs13(
    const int32_t local548[13],
    const int32_t round_mixed13[13],
    l3_authorization_word_pair raw2ab0[13],
    l3_authorization_word_pair prefix2ab0[13],
    l3_authorization_word_pair raw2b30[13],
    l3_authorization_word_pair prefix2b30[13],
    l3_authorization_stage2_stage_accumulation_trace *trace) {
    if (!local548 || !round_mixed13 || !raw2ab0 || !prefix2ab0 || !raw2b30 || !prefix2b30) return -1;
    int rc = l3_authorization_core_local548_accum2ab0_pairs13(local548, raw2ab0, prefix2ab0);
    if (rc != 0) return rc;
    rc = l3_authorization_core_roundctx_accum2b30_pairs13(round_mixed13, raw2b30, prefix2b30);
    if (rc != 0) return rc;
    if (trace) {
        memcpy(trace->local548_raw2ab0, raw2ab0, sizeof(trace->local548_raw2ab0));
        memcpy(trace->local548_prefix2ab0, prefix2ab0, sizeof(trace->local548_prefix2ab0));
        memcpy(trace->round_raw2b30, raw2b30, sizeof(trace->round_raw2b30));
        memcpy(trace->round_prefix2b30, prefix2b30, sizeof(trace->round_prefix2b30));
    }
    return 0;
}

/* ---- v309: FUN_f3fbc5c0 post-V307 local_548/mixed round-context convolution/reduction ---- */

int l3_authorization_stage2_stage_convolution26(
    const l3_authorization_word_pair raw2ab0_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair prefix2ab0_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair raw2b30_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair prefix2b30_13[L3_AUTH_ROUND_CORE_PAIRS13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    l3_authorization_stage2_stage_convolution_trace *out) {
    if (!raw2ab0_13 || !prefix2ab0_13 || !raw2b30_13 || !prefix2b30_13 || !out_pairs26) return -1;

    l3_authorization_stage2_stage_convolution_trace tmp;
    l3_authorization_stage2_stage_convolution_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_CORE_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x1ca7579au;
        uint32_t v = 0xebecb8fbu;
        l3_authorization_word_pair conv = {0u, 0u};
        l3_authorization_word_pair sum2ab0 = {0u, 0u};
        l3_authorization_word_pair sum2b30 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_word_pair *a2ab0 = &raw2ab0_13[k];
                const l3_authorization_word_pair *b2b30 = &raw2b30_13[idx - k];
                const uint64_t prod = (uint64_t)a2ab0->lo * (uint64_t)b2b30->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b2b30->hi * a2ab0->lo + b2b30->lo * a2ab0->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            mixer_pair_range_sum(prefix2ab0_13, first, last, &sum2ab0);
            mixer_pair_range_sum(prefix2b30_13, idx - last, idx - first, &sum2b30);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0x8aab861cu;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0xb09dce43u;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum2ab0_scaled = (uint64_t)sum2ab0.lo * 0x4b9b747bu;
            const uint32_t sum2ab0_scaled_lo = (uint32_t)sum2ab0_scaled;
            const uint64_t sum2b30_scaled = (uint64_t)sum2b30.lo * 0x57d915bcu;
            const uint32_t sum2b30_scaled_lo = (uint32_t)sum2b30_scaled;

            const uint32_t t0 = count_lo + 0x1ca7579au;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum2ab0_scaled_lo;
            u = t2 + sum2b30_scaled_lo;

            v = count * 0x65b6deffu + count_hi + 0xebecb8fbu +
                add_carry_u32(count_lo, 0x1ca7579au) +
                conv.hi * 0xb09dce43u + conv.lo * 0x2d4ab508u +
                (uint32_t)(conv_scaled >> 32) + add_carry_u32(t0, conv_scaled_lo) +
                sum2ab0.hi * 0x4b9b747bu + sum2ab0.lo * 0xd63a673bu +
                (uint32_t)(sum2ab0_scaled >> 32) + add_carry_u32(t1, sum2ab0_scaled_lo) +
                sum2b30.hi * 0x57d915bcu + sum2b30.lo * 0x7a1ae893u +
                (uint32_t)(sum2b30_scaled >> 32) + add_carry_u32(t2, sum2b30_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x2268bd0bu;
        out_pairs26[idx].lo = out_lo_mul + 0x649eb9a8u;
        out_pairs26[idx].hi = v * 0x2268bd0bu + u * 0x5fad32d9u +
                              multiply_high_u32(u, 0x2268bd0bu) + 0x21566693u +
                              add_carry_u32(out_lo_mul, 0x649eb9a8u);
        dst->convolution26[idx] = conv;
        dst->range2ab0_26[idx] = sum2ab0;
        dst->range2b30_26[idx] = sum2b30;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_stage2_stage_accumulate_and_convolve26(
    const int32_t local548[13],
    const int32_t round_mixed13[13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    l3_authorization_stage2_stage_convolution_trace *out) {
    if (!local548 || !round_mixed13 || !out_pairs26) return -1;

    l3_authorization_word_pair raw2ab0[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair prefix2ab0[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair raw2b30[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair prefix2b30[L3_AUTH_ROUND_CORE_PAIRS13];

    int rc = l3_authorization_core_post_accum2ab0_2b30_pairs13(
        local548, round_mixed13, raw2ab0, prefix2ab0, raw2b30, prefix2b30, NULL);
    if (rc != 0) return rc;
    return l3_authorization_stage2_stage_convolution26(
        raw2ab0, prefix2ab0, raw2b30, prefix2b30, out_pairs26, out);
}


/* ---- v310: FUN_f3fbc5c0 vector materialization after V309 convolution image ---- */

static uint64_t third_stage_vector_bias_half(void) {
    return UINT64_C(0x0561afdae7338f74b);
}

int l3_authorization_stage2_stage_vector_reduction13(
    const l3_authorization_word_pair low_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const l3_authorization_word_pair high_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const uint64_t high_seed_halves2[2],
    const uint64_t low_seed_halves2[2],
    const uint64_t middle_low_seed_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage2_stage_vector_reduction_trace *out) {
    if (!low_pairs26 || !high_pairs26 || !high_seed_halves2 || !low_seed_halves2 ||
        !middle_low_seed_halves2 || !out_halves13) return -1;

    l3_authorization_stage2_stage_vector_reduction_trace tmp;
    l3_authorization_stage2_stage_vector_reduction_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    const uint32_t mul_low = 0x33a8ae13u;
    const uint64_t mul_low64 = UINT64_C(0x6559320433a8ae13);
    const uint64_t mul_high64 = UINT64_C(0x42051227d794c51b);
    const uint64_t bias = third_stage_vector_bias_half();

    for (unsigned lane = 0; lane < 13u; ++lane) {
        const unsigned p0 = lane * 2u;
        const unsigned p1 = p0 + 1u;

        const uint64_t high0 =
            (uint64_t)high_pairs26[p0].lo |
            (uint64_t)high_pairs26[p0].hi << 32;
        const uint64_t high1 =
            (uint64_t)high_pairs26[p1].lo |
            (uint64_t)high_pairs26[p1].hi << 32;
        const uint64_t low0 =
            (uint64_t)low_pairs26[p0].lo |
            (uint64_t)low_pairs26[p0].hi << 32;
        const uint64_t low1 =
            (uint64_t)low_pairs26[p1].lo |
            (uint64_t)low_pairs26[p1].hi << 32;
        const uint64_t side_high0 = high0 * mul_high64;
        const uint64_t side_high1 = high1 * mul_high64;
        const uint64_t side_low0 = low0 * mul_low64;
        const uint64_t side_low1 = low1 * mul_low64;

        out_halves13[lane][0] = side_high0 + side_low0 + bias;
        out_halves13[lane][1] = side_high1 + side_low1 + bias;

        dst->input_low_halves13[lane][0] = ((uint64_t)low_pairs26[p0].hi << 32) | low_pairs26[p0].lo;
        dst->input_low_halves13[lane][1] = ((uint64_t)low_pairs26[p1].hi << 32) | low_pairs26[p1].lo;
        dst->input_high_halves13[lane][0] = ((uint64_t)high_pairs26[p0].hi << 32) | high_pairs26[p0].lo;
        dst->input_high_halves13[lane][1] = ((uint64_t)high_pairs26[p1].hi << 32) | high_pairs26[p1].lo;
        dst->side_from_high13[lane][0] = side_high0;
        dst->side_from_high13[lane][1] = side_high1;
        dst->side_from_low13[lane][0] = side_low0;
        dst->side_from_low13[lane][1] = side_low1;
        dst->bias13[lane][0] = bias;
        dst->bias13[lane][1] = bias;
        dst->output_halves13[lane][0] = out_halves13[lane][0];
        dst->output_halves13[lane][1] = out_halves13[lane][1];
    }

    const uint64_t scalar_mul = (uint64_t)low_pairs26[24].lo * (uint64_t)mul_low;
    const uint32_t scalar_arg =
        low_pairs26[24].hi * mul_low +
        low_pairs26[24].lo * 0x65593204u +
        (uint32_t)(scalar_mul >> 32);
    if (out_vector_feedback_scalar_arg) *out_vector_feedback_scalar_arg = scalar_arg;
    dst->vector_feedback_scalar_arg = scalar_arg;
    (void)high_seed_halves2;
    (void)low_seed_halves2;
    (void)middle_low_seed_halves2;
    return 0;
}

int l3_authorization_stage2_stage_convolve_and_reduce_vectors13(
    const l3_authorization_word_pair high_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const int32_t local548[13],
    const int32_t round_mixed13[13],
    const uint64_t high_seed_halves2[2],
    const uint64_t low_seed_halves2[2],
    const uint64_t middle_low_seed_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage2_stage_convolution_trace *conv_trace,
    l3_authorization_stage2_stage_vector_reduction_trace *vec_trace) {
    if (!high_pairs26 || !local548 || !round_mixed13 || !high_seed_halves2 ||
        !low_seed_halves2 || !middle_low_seed_halves2 || !out_halves13) return -1;

    l3_authorization_word_pair low_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26];
    int rc = l3_authorization_stage2_stage_accumulate_and_convolve26(local548, round_mixed13,
                                                           low_pairs26, conv_trace);
    if (rc != 0) return rc;
    return l3_authorization_stage2_stage_vector_reduction13(
        low_pairs26, high_pairs26, high_seed_halves2, low_seed_halves2, middle_low_seed_halves2,
        out_halves13, out_vector_feedback_scalar_arg, vec_trace);
}

/* ---- v311: FUN_f3fbc5c0 post-V310 local_5e4/mixed-round accumulator seam ---- */
static const l3_authorization_word_pair k_mixer_core_f41a2bb0[16] = {
    {0xfb48abbcu, 0xa6147ed4u}, {0x63a2f424u, 0x5a251744u},
    {0xcbfd3c8cu, 0x0e35afb3u}, {0x345784f4u, 0xb2464823u},
    {0x7025a087u, 0x65fe5e40u}, {0xd87fe8efu, 0x1a0ef6afu},
    {0x40da3157u, 0xce1f8f1fu}, {0xa93479bfu, 0x7230278eu},
    {0x118ec227u, 0x2640bffeu}, {0x4d5cddbau, 0xd9f8d61bu},
    {0xb5b72622u, 0x8e096e8au}, {0x1e116e8au, 0x321a06fau},
    {0x866bb6f2u, 0xe62a9f69u}, {0xeec5ff5au, 0x9a3b37d8u},
    {0x2a941aedu, 0x4df34df6u}, {0x92ee6355u, 0xf203e665u}
};

static const l3_authorization_word_pair k_mixer_core_f41a2c30[16] = {
    {0x325dcf6du, 0x765d81c3u}, {0xd8b4c4dfu, 0x674ab9d2u},
    {0x1e71226au, 0x569238e3u}, {0x642d7ff5u, 0x45d9b7f3u},
    {0x0a847567u, 0x36c6f003u}, {0x5040d2f2u, 0x260e6f13u},
    {0xf697c864u, 0x16fba722u}, {0x3c5425efu, 0x06432633u},
    {0xe2ab1b61u, 0xf7305e42u}, {0x286778ecu, 0xe677dd53u},
    {0xcebe6e5eu, 0xd7651562u}, {0x147acbe9u, 0xc6ac9473u},
    {0x5a372974u, 0xb5f41383u}, {0x008e1ee6u, 0xa6e14b93u},
    {0x464a7c71u, 0x9628caa3u}, {0xeca171e3u, 0x871602b2u}
};

static const uint32_t k_mixer_core_f4191610[8] = {
    0x6a5331a7u, 0xc13ff6dbu, 0x503c9b51u, 0x619f0d91u,
    0x499b134du, 0xf2fb21afu, 0xe00af5ddu, 0xf9ed3a7bu
};
static const uint32_t k_mixer_core_f4191630[8] = {
    0x27d860f8u, 0x03952698u, 0x1044f313u, 0x03ce1af8u,
    0xbd7a8bd1u, 0x7dd0e11eu, 0x3e49de9fu, 0x853fcb1eu
};
static const uint32_t k_mixer_core_f4191650[8] = {
    0x0be4e7b1u, 0xe5454785u, 0xbd3664edu, 0xaaec4e79u,
    0x2f9bf5c9u, 0x4958f181u, 0x82e79383u, 0xbc5d1f41u
};
static const uint32_t k_mixer_core_f4191670[8] = {
    0x5a65e05eu, 0x6bf31527u, 0x7ed117c2u, 0x87eaf0cbu,
    0xe5e48371u, 0x312293e6u, 0xc37f3fbbu, 0x04eac701u
};

static l3_authorization_word_pair mixer_core_make_2bb0_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0xc3d374c1u);
    l3_authorization_word_pair seed = {
        lo0 + 0x24a02001u,
        (uint32_t)(x * 0xed7f1276u + multiply_high_u32(x, 0xc3d374c1u) +
                   0x0cac9ed4u + add_carry_u32(lo0, 0x24a02001u))
    };
    const uint32_t folded_low = mixer_core_fold_rounds(seed, k_mixer_core_f41a2bb0, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0x0aa7bf13u);
    l3_authorization_word_pair out = {
        lo1 + 0x465f2c76u,
        (uint32_t)(x * 0x16fcb37eu + multiply_high_u32(x, 0x0aa7bf13u) +
                   0x3bcc0eb5u + add_carry_u32(lo1, 0x465f2c76u) +
                   folded_low * 0x11147b2du)
    };
    return out;
}

static l3_authorization_word_pair mixer_core_make_2c30_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0x25d42083u);
    l3_authorization_word_pair seed = {
        lo0 + 0xd864a4f7u,
        (uint32_t)(x * 0xc805bd46u + multiply_high_u32(x, 0x25d42083u) +
                   0x400fa5deu + add_carry_u32(lo0, 0xd864a4f7u))
    };
    const uint32_t folded_low = mixer_core_fold_rounds(seed, k_mixer_core_f41a2c30, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0x9ab8030fu);
    l3_authorization_word_pair out = {
        lo1 + 0xfc787256u,
        (uint32_t)(x * 0x4901d17bu + multiply_high_u32(x, 0x9ab8030fu) +
                   0x9e3f79c6u + add_carry_u32(lo1, 0xfc787256u) +
                   folded_low * 0x9667ca7bu)
    };
    return out;
}

int l3_authorization_core_local5e4_accum2bb0_pairs13(
    const int32_t local5e4[13],
    l3_authorization_word_pair raw2bb0[13],
    l3_authorization_word_pair prefix2bb0[13]) {
    if (!local5e4 || !raw2bb0 || !prefix2bb0) return -1;
    uint32_t x = (uint32_t)local5e4[0] * 0x594ed8cdu + 0xf0f25119u;
    raw2bb0[0] = mixer_core_make_2bb0_pair(x);
    prefix2bb0[0] = raw2bb0[0];
    for (unsigned lane = 1; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        const uint32_t pre = (uint32_t)local5e4[lane] * k_mixer_core_f4191610[idx] +
                             k_mixer_core_f4191630[idx];
        x = pre * 0x33e1286bu + 0xd8630971u;
        raw2bb0[lane] = mixer_core_make_2bb0_pair(x);
        prefix2bb0[lane] = mixer_core_pair_add(prefix2bb0[lane - 1u], raw2bb0[lane]);
    }
    return 0;
}

int l3_authorization_core_roundmix_accum2c30_pairs13(
    const int32_t mixed_round13[13],
    l3_authorization_word_pair raw2c30[13],
    l3_authorization_word_pair prefix2c30[13]) {
    if (!mixed_round13 || !raw2c30 || !prefix2c30) return -1;
    uint32_t x = (uint32_t)mixed_round13[0] * 0x9e012bd1u + 0x4d0416eeu;
    raw2c30[0] = mixer_core_make_2c30_pair(x);
    prefix2c30[0] = raw2c30[0];
    for (unsigned lane = 1; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        const uint32_t pre = (uint32_t)mixed_round13[lane] * k_mixer_core_f4191650[idx] +
                             k_mixer_core_f4191670[idx];
        x = pre * 0x8695ae21u + 0x9cac46d0u;
        raw2c30[lane] = mixer_core_make_2c30_pair(x);
        prefix2c30[lane] = mixer_core_pair_add(prefix2c30[lane - 1u], raw2c30[lane]);
    }
    return 0;
}

int l3_authorization_core_post_accum2bb0_2c30_pairs13(
    const int32_t local5e4[13],
    const int32_t mixed_round13[13],
    l3_authorization_word_pair raw2bb0[13],
    l3_authorization_word_pair prefix2bb0[13],
    l3_authorization_word_pair raw2c30[13],
    l3_authorization_word_pair prefix2c30[13],
    l3_authorization_stage3_stage_accumulation_trace *trace) {
    if (!local5e4 || !mixed_round13 || !raw2bb0 || !prefix2bb0 || !raw2c30 || !prefix2c30) return -1;
    int rc = l3_authorization_core_local5e4_accum2bb0_pairs13(local5e4, raw2bb0, prefix2bb0);
    if (rc) return rc;
    rc = l3_authorization_core_roundmix_accum2c30_pairs13(mixed_round13, raw2c30, prefix2c30);
    if (rc) return rc;
    if (trace) {
        memcpy(trace->local5e4_raw2bb0, raw2bb0, sizeof(trace->local5e4_raw2bb0));
        memcpy(trace->local5e4_prefix2bb0, prefix2bb0, sizeof(trace->local5e4_prefix2bb0));
        memcpy(trace->roundmix_raw2c30, raw2c30, sizeof(trace->roundmix_raw2c30));
        memcpy(trace->roundmix_prefix2c30, prefix2c30, sizeof(trace->roundmix_prefix2c30));
    }
    return 0;
}

/* ---- v312: FUN_f3fbc5c0 post-V310 local_5e4/mixed round-context convolution/reduction ---- */

int l3_authorization_stage3_stage_convolution26(
    const l3_authorization_word_pair raw2bb0_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair prefix2bb0_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair raw2c30_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair prefix2c30_13[L3_AUTH_ROUND_CORE_PAIRS13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    l3_authorization_stage3_stage_convolution_trace *out) {
    if (!raw2bb0_13 || !prefix2bb0_13 || !raw2c30_13 || !prefix2c30_13 || !out_pairs26) return -1;

    l3_authorization_stage3_stage_convolution_trace tmp;
    l3_authorization_stage3_stage_convolution_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_CORE_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x9676696fu;
        uint32_t v = 0xf5eaf2cfu;
        l3_authorization_word_pair conv = {0u, 0u};
        l3_authorization_word_pair sum2bb0 = {0u, 0u};
        l3_authorization_word_pair sum2c30 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_word_pair *a2bb0 = &raw2bb0_13[k];
                const l3_authorization_word_pair *b2c30 = &raw2c30_13[idx - k];
                const uint64_t prod = (uint64_t)a2bb0->lo * (uint64_t)b2c30->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b2c30->hi * a2bb0->lo + b2c30->lo * a2bb0->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            mixer_pair_range_sum(prefix2bb0_13, first, last, &sum2bb0);
            mixer_pair_range_sum(prefix2c30_13, idx - last, idx - first, &sum2c30);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0x25996798u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0xbd10df31u;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum2bb0_scaled = (uint64_t)sum2bb0.lo * 0x47195eaeu;
            const uint32_t sum2bb0_scaled_lo = (uint32_t)sum2bb0_scaled;
            const uint64_t sum2c30_scaled = (uint64_t)sum2c30.lo * 0x6e0326d4u;
            const uint32_t sum2c30_scaled_lo = (uint32_t)sum2c30_scaled;

            const uint32_t t0 = count_lo + 0x9676696fu;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum2bb0_scaled_lo;
            u = t2 + sum2c30_scaled_lo;

            v = count * 0xdd529d46u + count_hi + 0xf5eaf2cfu +
                add_carry_u32(count_lo, 0x9676696fu) +
                conv.hi * 0xbd10df31u + conv.lo * 0x707ae871u +
                (uint32_t)(conv_scaled >> 32) + add_carry_u32(t0, conv_scaled_lo) +
                sum2bb0.hi * 0x47195eaeu + sum2bb0.lo * 0xcf6cb87du +
                (uint32_t)(sum2bb0_scaled >> 32) + add_carry_u32(t1, sum2bb0_scaled_lo) +
                sum2c30.hi * 0x6e0326d4u + sum2c30.lo * 0xaf682438u +
                (uint32_t)(sum2c30_scaled >> 32) + add_carry_u32(t2, sum2c30_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x2013df9bu;
        out_pairs26[idx].lo = out_lo_mul + 0x2d3645e1u;
        out_pairs26[idx].hi = v * 0x2013df9bu + u * 0xa0e14102u +
                              multiply_high_u32(u, 0x2013df9bu) + 0xd848c030u +
                              add_carry_u32(out_lo_mul, 0x2d3645e1u);
        dst->convolution26[idx] = conv;
        dst->range2bb0_26[idx] = sum2bb0;
        dst->range2c30_26[idx] = sum2c30;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_stage3_stage_accumulate_and_convolve26(
    const int32_t local5e4[13],
    const int32_t mixed_round13[13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    l3_authorization_stage3_stage_convolution_trace *out) {
    if (!local5e4 || !mixed_round13 || !out_pairs26) return -1;

    l3_authorization_word_pair raw2bb0[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair prefix2bb0[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair raw2c30[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair prefix2c30[L3_AUTH_ROUND_CORE_PAIRS13];

    int rc = l3_authorization_core_post_accum2bb0_2c30_pairs13(
        local5e4, mixed_round13, raw2bb0, prefix2bb0, raw2c30, prefix2c30, NULL);
    if (rc != 0) return rc;
    return l3_authorization_stage3_stage_convolution26(
        raw2bb0, prefix2bb0, raw2c30, prefix2c30, out_pairs26, out);
}


/* ---- v313: FUN_f3fbc5c0 vector materialization after V312 convolution image ---- */

static uint64_t fourth_stage_vector_bias_half(void) {
    return UINT64_C(0x33669327560da81a);
}

int l3_authorization_stage3_stage_vector_reduction13(
    const l3_authorization_word_pair low_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const l3_authorization_word_pair high_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const uint64_t high_seed_halves2[2],
    const uint64_t low_seed_halves2[2],
    const uint64_t middle_low_seed_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage3_stage_vector_reduction_trace *out) {
    if (!low_pairs26 || !high_pairs26 || !high_seed_halves2 || !low_seed_halves2 ||
        !middle_low_seed_halves2 || !out_halves13) return -1;

    l3_authorization_stage3_stage_vector_reduction_trace tmp;
    l3_authorization_stage3_stage_vector_reduction_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    const uint32_t mul_low = 0x52acb075u;
    const uint64_t mul_low64 = UINT64_C(0x90b3c2f252acb075);
    const uint64_t mul_high64 = UINT64_C(0x1a757ec319f63d8b);
    const uint64_t bias = fourth_stage_vector_bias_half();

    for (unsigned lane = 0; lane < 13u; ++lane) {
        const unsigned p0 = lane * 2u;
        const unsigned p1 = p0 + 1u;

        const uint64_t high0 =
            (uint64_t)high_pairs26[p0].lo |
            (uint64_t)high_pairs26[p0].hi << 32;
        const uint64_t high1 =
            (uint64_t)high_pairs26[p1].lo |
            (uint64_t)high_pairs26[p1].hi << 32;
        const uint64_t low0 =
            (uint64_t)low_pairs26[p0].lo |
            (uint64_t)low_pairs26[p0].hi << 32;
        const uint64_t low1 =
            (uint64_t)low_pairs26[p1].lo |
            (uint64_t)low_pairs26[p1].hi << 32;
        const uint64_t side_high0 = high0 * mul_high64;
        const uint64_t side_high1 = high1 * mul_high64;
        const uint64_t side_low0 = low0 * mul_low64;
        const uint64_t side_low1 = low1 * mul_low64;

        out_halves13[lane][0] = side_high0 + side_low0 + bias;
        out_halves13[lane][1] = side_high1 + side_low1 + bias;

        dst->input_low_halves13[lane][0] = ((uint64_t)low_pairs26[p0].hi << 32) | low_pairs26[p0].lo;
        dst->input_low_halves13[lane][1] = ((uint64_t)low_pairs26[p1].hi << 32) | low_pairs26[p1].lo;
        dst->input_high_halves13[lane][0] = ((uint64_t)high_pairs26[p0].hi << 32) | high_pairs26[p0].lo;
        dst->input_high_halves13[lane][1] = ((uint64_t)high_pairs26[p1].hi << 32) | high_pairs26[p1].lo;
        dst->side_from_high13[lane][0] = side_high0;
        dst->side_from_high13[lane][1] = side_high1;
        dst->side_from_low13[lane][0] = side_low0;
        dst->side_from_low13[lane][1] = side_low1;
        dst->bias13[lane][0] = bias;
        dst->bias13[lane][1] = bias;
        dst->output_halves13[lane][0] = out_halves13[lane][0];
        dst->output_halves13[lane][1] = out_halves13[lane][1];
    }

    const uint64_t scalar_mul = (uint64_t)low_pairs26[24].lo * (uint64_t)mul_low;
    const uint32_t scalar_arg =
        low_pairs26[24].hi * mul_low +
        low_pairs26[24].lo * 0x90b3c2f2u +
        (uint32_t)(scalar_mul >> 32);
    if (out_vector_feedback_scalar_arg) *out_vector_feedback_scalar_arg = scalar_arg;
    dst->vector_feedback_scalar_arg = scalar_arg;
    (void)high_seed_halves2;
    (void)low_seed_halves2;
    (void)middle_low_seed_halves2;
    return 0;
}

int l3_authorization_stage3_stage_convolve_and_reduce_vectors13(
    const l3_authorization_word_pair high_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const int32_t local5e4[13],
    const int32_t mixed_round13[13],
    const uint64_t high_seed_halves2[2],
    const uint64_t low_seed_halves2[2],
    const uint64_t middle_low_seed_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage3_stage_convolution_trace *conv_trace,
    l3_authorization_stage3_stage_vector_reduction_trace *vec_trace) {
    if (!high_pairs26 || !local5e4 || !mixed_round13 || !high_seed_halves2 ||
        !low_seed_halves2 || !middle_low_seed_halves2 || !out_halves13) return -1;

    l3_authorization_word_pair low_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26];
    int rc = l3_authorization_stage3_stage_accumulate_and_convolve26(local5e4, mixed_round13,
                                                           low_pairs26, conv_trace);
    if (rc != 0) return rc;
    return l3_authorization_stage3_stage_vector_reduction13(
        low_pairs26, high_pairs26, high_seed_halves2, low_seed_halves2, middle_low_seed_halves2,
        out_halves13, out_vector_feedback_scalar_arg, vec_trace);
}

/* ---- native param_7 high stream: local_57c/current +0x2cc round window ---- */
static const l3_authorization_word_pair k_mixer_core_f41a2cb0[16] = {
    {0x6e06861au, 0xaddfa8d4u}, {0xca39d9cdu, 0xb70da740u},
    {0x266d2d80u, 0xc03ba5adu}, {0x82a08133u, 0xd969a419u},
    {0x1b9e99b7u, 0xefb7bbc0u}, {0x77d1ed6au, 0xf8e5ba2cu},
    {0xd405411du, 0x0213b898u}, {0x303894d0u, 0x1b41b705u},
    {0x8c6be883u, 0x246fb571u}, {0xe89f3c36u, 0x3d9db3ddu},
    {0x44d28fe9u, 0x46cbb24au}, {0xa105e39cu, 0x5ff9b0b6u},
    {0xfd39374fu, 0x6927af22u}, {0x596c8b02u, 0x7255ad8fu},
    {0xb59fdeb5u, 0x8b83abfbu}, {0x11d33268u, 0x94b1aa68u}
};

static const l3_authorization_word_pair k_mixer_core_f41a2d30[16] = {
    {0xad51d8a7u, 0xcf0e00b6u}, {0x078ad85fu, 0x1c602ee0u},
    {0xb81cfd4eu, 0x6b3faa61u}, {0x1255fd06u, 0xb891d88bu},
    {0x6c8efcbeu, 0x05e406b4u}, {0x1d2121adu, 0x54c38236u},
    {0x775a2165u, 0xa215b05fu}, {0x27ec4654u, 0xf0f52be1u},
    {0x8225460cu, 0x4e475a0au}, {0x32b76afbu, 0x9d26d58cu},
    {0x8cf06ab3u, 0xea7903b5u}, {0x3d828fa2u, 0x39587f37u},
    {0x97bb8f5au, 0x86aaad60u}, {0xf1f48f12u, 0xd3fcdb89u},
    {0xa286b401u, 0x22dc570bu}, {0xfcbfb3b9u, 0x702e8534u}
};

static const uint32_t k_mixer_core_f4191690[8] = {
    0x6d24bfadu, 0x01f5ca93u, 0x40a0e457u, 0x610944f5u,
    0xbf2a2293u, 0x6e27c74bu, 0xee2d611fu, 0x20b7ead7u
};
static const uint32_t k_mixer_core_f41916b0[8] = {
    0x4919c750u, 0xccce8817u, 0x2ca1d121u, 0xa96ab9ccu,
    0xd839edf5u, 0xa15a1c6au, 0xe846c67cu, 0xf0ee9851u
};
static const uint32_t k_mixer_core_f41916d0[8] = {
    0x7c64924bu, 0xd125c163u, 0xacddc34bu, 0x0dc43fc3u,
    0xcf10f987u, 0x424b8d15u, 0xed960107u, 0x4ef597fbu
};
static const uint32_t k_mixer_core_f41916f0[8] = {
    0x2a983164u, 0x12d880bbu, 0x8ee4397bu, 0xe440f566u,
    0x87a830edu, 0x7f186d2bu, 0x94826479u, 0x29e34512u
};

static l3_authorization_word_pair mixer_core_make_2cb0_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0x328ce74fu);
    l3_authorization_word_pair seed = {
        lo0 + 0xf055731fu,
        (uint32_t)(x * 0x850ff4ceu +
                   multiply_high_u32(x, 0x328ce74fu) +
                   0xf0b0c911u +
                   add_carry_u32(lo0, 0xf055731fu))
    };
    const uint32_t folded_low =
        mixer_core_fold_rounds(seed, k_mixer_core_f41a2cb0, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0xe97ff137u);
    l3_authorization_word_pair out = {
        lo1 + 0x796d5d68u,
        (uint32_t)(x * 0x9f5ef305u +
                   multiply_high_u32(x, 0xe97ff137u) +
                   0x6948a07du +
                   add_carry_u32(lo1, 0x796d5d68u) +
                   folded_low * 0xe7e1a267u)
    };
    return out;
}

static l3_authorization_word_pair mixer_core_make_2d30_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0xaa67b40du);
    l3_authorization_word_pair seed = {
        lo0 + 0xf7b97885u,
        (uint32_t)(x * 0x5f983888u +
                   multiply_high_u32(x, 0xaa67b40du) +
                   0x9a177b36u +
                   add_carry_u32(lo0, 0xf7b97885u))
    };
    const uint32_t folded_low =
        mixer_core_fold_rounds(seed, k_mixer_core_f41a2d30, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0x55da8ddbu);
    l3_authorization_word_pair out = {
        lo1 + 0x1a346d1cu,
        (uint32_t)(x * 0x482c70d2u +
                   multiply_high_u32(x, 0x55da8ddbu) +
                   0x62cb47ffu +
                   add_carry_u32(lo1, 0x1a346d1cu) +
                   folded_low * 0xe698b879u)
    };
    return out;
}

static int mixer_core_param7_high_accum_pairs13(
    const int32_t local57c[13],
    const int32_t round_2cc_words13[13],
    l3_authorization_word_pair raw2cb0[13],
    l3_authorization_word_pair prefix2cb0[13],
    l3_authorization_word_pair raw2d30[13],
    l3_authorization_word_pair prefix2d30[13]) {
    if (!local57c || !round_2cc_words13 || !raw2cb0 || !prefix2cb0 ||
        !raw2d30 || !prefix2d30) {
        return -1;
    }

    uint32_t x = (uint32_t)local57c[0] * 0x3e0994dfu + 0x9a34ce72u;
    raw2cb0[0] = mixer_core_make_2cb0_pair(x);
    prefix2cb0[0] = raw2cb0[0];
    x = (uint32_t)round_2cc_words13[0] * 0x1b784041u + 0xa2816f4eu;
    raw2d30[0] = mixer_core_make_2d30_pair(x);
    prefix2d30[0] = raw2d30[0];

    for (unsigned lane = 1u; lane < 13u; ++lane) {
        const unsigned table_index = lane & 7u;
        uint32_t pre =
            (uint32_t)local57c[lane] * k_mixer_core_f4191690[table_index] +
            k_mixer_core_f41916b0[table_index];
        x = pre * 0x7564083bu + 0x87c95f02u;
        raw2cb0[lane] = mixer_core_make_2cb0_pair(x);
        prefix2cb0[lane] =
            mixer_core_pair_add(prefix2cb0[lane - 1u], raw2cb0[lane]);

        pre = (uint32_t)round_2cc_words13[lane] *
                  k_mixer_core_f41916d0[table_index] +
              k_mixer_core_f41916f0[table_index];
        x = pre * 0x8400c023u + 0x1aa7aea2u;
        raw2d30[lane] = mixer_core_make_2d30_pair(x);
        prefix2d30[lane] =
            mixer_core_pair_add(prefix2d30[lane - 1u], raw2d30[lane]);
    }
    return 0;
}

int l3_authorization_core_param7_high_accum_convolution26(
    const int32_t local57c[13],
    const int32_t round_2cc_words13[13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26]) {
    if (!local57c || !round_2cc_words13 || !out_pairs26) return -1;

    l3_authorization_word_pair raw2cb0[13], prefix2cb0[13];
    l3_authorization_word_pair raw2d30[13], prefix2d30[13];
    int rc = mixer_core_param7_high_accum_pairs13(
        local57c, round_2cc_words13,
        raw2cb0, prefix2cb0, raw2d30, prefix2d30);
    if (rc != 0) return rc;

    for (unsigned idx = 0u;
         idx < L3_AUTH_ROUND_CORE_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0xa4e76bfau;
        uint32_t v = 0x1f296660u;
        l3_authorization_word_pair conv = {0u, 0u};
        l3_authorization_word_pair sum2cb0 = {0u, 0u};
        l3_authorization_word_pair sum2d30 = {0u, 0u};

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_word_pair *a = &raw2cb0[k];
                const l3_authorization_word_pair *b = &raw2d30[idx - k];
                const uint64_t product = (uint64_t)a->lo * b->lo;
                const uint32_t old_low = conv.lo;
                conv.lo += (uint32_t)product;
                conv.hi += b->hi * a->lo + b->lo * a->hi +
                           (uint32_t)(product >> 32) +
                           (uint32_t)(conv.lo < old_low);
            }

            mixer_pair_range_sum(
                prefix2cb0, first, last, &sum2cb0);
            mixer_pair_range_sum(
                prefix2d30, idx - last, idx - first, &sum2d30);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_product =
                (uint64_t)count * 0xbb6670a9u;
            const uint32_t count_low = (uint32_t)count_product;
            const uint64_t conv_scaled =
                (uint64_t)conv.lo * 0xa55485d1u;
            const uint32_t conv_scaled_low = (uint32_t)conv_scaled;
            const uint64_t sum2cb0_scaled =
                (uint64_t)sum2cb0.lo * 0xe199e13bu;
            const uint32_t sum2cb0_scaled_low =
                (uint32_t)sum2cb0_scaled;
            const uint64_t sum2d30_scaled =
                (uint64_t)sum2d30.lo * 0x3e44b55bu;
            const uint32_t sum2d30_scaled_low =
                (uint32_t)sum2d30_scaled;

            const uint32_t t0 = count_low + 0xa4e76bfau;
            const uint32_t t1 = t0 + conv_scaled_low;
            const uint32_t t2 = t1 + sum2cb0_scaled_low;
            u = t2 + sum2d30_scaled_low;
            v = count * 0xd41ee751u +
                (uint32_t)(count_product >> 32) + 0x1f296660u +
                add_carry_u32(count_low, 0xa4e76bfau) +
                conv.hi * 0xa55485d1u + conv.lo * 0x6dc0784bu +
                (uint32_t)(conv_scaled >> 32) +
                add_carry_u32(t0, conv_scaled_low) +
                sum2cb0.hi * 0xe199e13bu +
                sum2cb0.lo * 0xc1d6f439u +
                (uint32_t)(sum2cb0_scaled >> 32) +
                add_carry_u32(t1, sum2cb0_scaled_low) +
                sum2d30.hi * 0x3e44b55bu +
                sum2d30.lo * 0xce7008d6u +
                (uint32_t)(sum2d30_scaled >> 32) +
                add_carry_u32(t2, sum2d30_scaled_low);
        }

        const uint32_t output_low_product = u * 0x96f9f22fu;
        out_pairs26[idx].lo = output_low_product + 0x136492b1u;
        out_pairs26[idx].hi =
            v * 0x96f9f22fu + u * 0x104d9b15u +
            multiply_high_u32(u, 0x96f9f22fu) + 0xa1d9b969u +
            add_carry_u32(
                output_low_product, 0x136492b1u);
    }
    return 0;
}

/* ---- v326: FUN_f3fbc5c0 post-param_7 local_618/round-context accumulator seam ---- */
static const l3_authorization_word_pair k_mixer_core_f41a2db0[16] = {
    {0x048f5995u, 0xe71de697u}, {0xc1d259b7u, 0x58c88ed3u},
    {0x6bcb2b1cu, 0xcdd813cfu}, {0x15c3fc81u, 0x32e798cbu},
    {0xd306fca3u, 0xa4924107u}, {0x7cffce08u, 0x19a1c603u},
    {0x26f89f6du, 0x8eb14affu}, {0xd0f170d2u, 0xf3c0cffau},
    {0x8e3470f4u, 0x656b7837u}, {0x382d4259u, 0xda7afd33u},
    {0xe22613beu, 0x4f8a822eu}, {0x9f6913e0u, 0xb1352a6bu},
    {0x4961e545u, 0x2644af67u}, {0xf35ab6aau, 0x9b543462u},
    {0xb09db6ccu, 0x0cfedc9fu}, {0x5a968831u, 0x720e619bu}
};

static const l3_authorization_word_pair k_mixer_core_f41a2e30[16] = {
    {0x463bf94eu, 0x2b76c609u}, {0xb23d2b89u, 0x352e9e46u},
    {0xb9a958abu, 0x48aff5f7u}, {0x25aa8ae6u, 0x5267ce35u},
    {0x2d16b808u, 0x65e925e6u}, {0x9917ea43u, 0x7fa0fe23u},
    {0xa0841765u, 0x832255d4u}, {0x0c8549a0u, 0x9cda2e12u},
    {0x78867bdbu, 0xa692064fu}, {0x7ff2a8fdu, 0xba135e00u},
    {0xebf3db38u, 0xc3cb363du}, {0xf360085au, 0xd74c8deeu},
    {0x5f613a95u, 0xe104662cu}, {0x66cd67b7u, 0xf485bdddu},
    {0xd2ce99f2u, 0x0e3d961au}, {0xda3ac714u, 0x11beedcbu}
};

static const uint32_t k_mixer_core_f4191710[8] = {
    0x6d5f0979u, 0x5e1a76e7u, 0xc0385cb7u, 0xb82cf19bu,
    0xfc8266e5u, 0xca36ca35u, 0x93a43c27u, 0xdd2154a7u
};
static const uint32_t k_mixer_core_f4191730[8] = {
    0x3d2be180u, 0xbc2aaa64u, 0xfdab5746u, 0x26a1c5d1u,
    0x51cb5388u, 0x529bda1cu, 0xe5580c4eu, 0xc3bc651bu
};
static const uint32_t k_mixer_core_f4191750[8] = {
    0xa4ece9fbu, 0xc6087397u, 0x7c4ff60fu, 0xb6c13793u,
    0xb5f21603u, 0xe9da73ebu, 0x791030c1u, 0xb0e2d32bu
};
static const uint32_t k_mixer_core_f4191770[8] = {
    0xeb294f80u, 0xcbadba83u, 0x0b63a24cu, 0x63dd740fu,
    0xb99bb171u, 0xd46af158u, 0x6937425fu, 0xd1149ea1u
};

static l3_authorization_word_pair mixer_core_make_2db0_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0x6b91e853u);
    l3_authorization_word_pair seed = {
        lo0 + 0xb7ac9a3bu,
        (uint32_t)(x * 0x55e62117u + multiply_high_u32(x, 0x6b91e853u) +
                   0x78482dabu + add_carry_u32(lo0, 0xb7ac9a3bu))
    };
    const uint32_t folded_low = mixer_core_fold_rounds(seed, k_mixer_core_f41a2db0, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0xdbdccc47u);
    l3_authorization_word_pair out = {
        lo1 + 0xc47e12b1u,
        (uint32_t)(x * 0xe08b306eu + multiply_high_u32(x, 0xdbdccc47u) +
                   0xefc3c1cau + add_carry_u32(lo1, 0xc47e12b1u) +
                   folded_low * 0xcafa4243u)
    };
    return out;
}

static l3_authorization_word_pair mixer_core_make_2e30_pair(uint32_t x) {
    const uint32_t lo0 = multiply_low_u32(x, 0xb49ee07du);
    l3_authorization_word_pair seed = {
        lo0 + 0xe044db07u,
        (uint32_t)(x * 0x24516004u + multiply_high_u32(x, 0xb49ee07du) +
                   0xd2638d16u + add_carry_u32(lo0, 0xe044db07u))
    };
    const uint32_t folded_low = mixer_core_fold_rounds(seed, k_mixer_core_f41a2e30, 8u).lo;
    const uint32_t lo1 = multiply_low_u32(x, 0x040b3dafu);
    l3_authorization_word_pair out = {
        lo1 + 0x24466032u,
        (uint32_t)(x * 0x4afad824u + multiply_high_u32(x, 0x040b3dafu) +
                   0x6e1ae599u + add_carry_u32(lo1, 0x24466032u) +
                   folded_low * 0x2d7ec565u)
    };
    return out;
}

int l3_authorization_core_local618_accum2db0_pairs13(
    const int32_t local618[13],
    l3_authorization_word_pair raw2db0[13],
    l3_authorization_word_pair prefix2db0[13]) {
    if (!local618 || !raw2db0 || !prefix2db0) return -1;
    uint32_t x = (uint32_t)local618[0] * 0xe949d4c5u + 0x7f721d47u;
    raw2db0[0] = mixer_core_make_2db0_pair(x);
    prefix2db0[0] = raw2db0[0];
    for (unsigned lane = 1u; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        const uint32_t pre = (uint32_t)local618[lane] * k_mixer_core_f4191710[idx] +
                             k_mixer_core_f4191730[idx];
        x = pre * 0x3fc65eadu + 0x22fdb9c7u;
        raw2db0[lane] = mixer_core_make_2db0_pair(x);
        prefix2db0[lane] = mixer_core_pair_add(prefix2db0[lane - 1u], raw2db0[lane]);
    }
    return 0;
}

int l3_authorization_core_roundctx_accum2e30_pairs13(
    const int32_t round_ec8_words13[13],
    l3_authorization_word_pair raw2e30[13],
    l3_authorization_word_pair prefix2e30[13]) {
    if (!round_ec8_words13 || !raw2e30 || !prefix2e30) return -1;
    uint32_t x = (uint32_t)round_ec8_words13[0] * 0xc02a842fu + 0x4669959bu;
    raw2e30[0] = mixer_core_make_2e30_pair(x);
    prefix2e30[0] = raw2e30[0];
    for (unsigned lane = 1u; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        const uint32_t pre = (uint32_t)round_ec8_words13[lane] * k_mixer_core_f4191750[idx] +
                             k_mixer_core_f4191770[idx];
        x = pre * 0x6b794c5du + 0x814db41bu;
        raw2e30[lane] = mixer_core_make_2e30_pair(x);
        prefix2e30[lane] = mixer_core_pair_add(prefix2e30[lane - 1u], raw2e30[lane]);
    }
    return 0;
}

int l3_authorization_core_post_accum2db0_2e30_pairs13(
    const int32_t local618[13],
    const int32_t round_ec8_words13[13],
    l3_authorization_word_pair raw2db0[13],
    l3_authorization_word_pair prefix2db0[13],
    l3_authorization_word_pair raw2e30[13],
    l3_authorization_word_pair prefix2e30[13],
    l3_authorization_stage4_stage_accumulation_trace *trace) {
    if (!local618 || !round_ec8_words13 || !raw2db0 || !prefix2db0 || !raw2e30 || !prefix2e30) return -1;
    int rc = l3_authorization_core_local618_accum2db0_pairs13(local618, raw2db0, prefix2db0);
    if (rc) return rc;
    rc = l3_authorization_core_roundctx_accum2e30_pairs13(round_ec8_words13, raw2e30, prefix2e30);
    if (rc) return rc;
    if (trace) {
        memcpy(trace->local618_raw2db0, raw2db0, sizeof(trace->local618_raw2db0));
        memcpy(trace->local618_prefix2db0, prefix2db0, sizeof(trace->local618_prefix2db0));
        memcpy(trace->roundctx_raw2e30, raw2e30, sizeof(trace->roundctx_raw2e30));
        memcpy(trace->roundctx_prefix2e30, prefix2e30, sizeof(trace->roundctx_prefix2e30));
    }
    return 0;
}

int l3_authorization_stage4_stage_convolution26(
    const l3_authorization_word_pair raw2db0_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair prefix2db0_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair raw2e30_13[L3_AUTH_ROUND_CORE_PAIRS13],
    const l3_authorization_word_pair prefix2e30_13[L3_AUTH_ROUND_CORE_PAIRS13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    l3_authorization_stage4_stage_convolution_trace *out) {
    if (!raw2db0_13 || !prefix2db0_13 || !raw2e30_13 || !prefix2e30_13 || !out_pairs26) return -1;

    l3_authorization_stage4_stage_convolution_trace tmp;
    l3_authorization_stage4_stage_convolution_trace *dst = out ? out : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned idx = 0; idx < L3_AUTH_ROUND_CORE_CONV_PAIRS26; ++idx) {
        unsigned first = idx - 12u;
        unsigned last;
        if (idx < 12u || first == 0u) {
            first = 0u;
            last = idx;
        } else {
            last = 12u;
        }

        uint32_t u = 0x2d81fda0u;
        uint32_t v = 0xbe5b5308u;
        l3_authorization_word_pair conv = {0u, 0u};
        l3_authorization_word_pair sum2db0 = {0u, 0u};
        l3_authorization_word_pair sum2e30 = {0u, 0u};

        dst->first_last_count26[idx][0] = first;
        dst->first_last_count26[idx][1] = last;
        dst->first_last_count26[idx][2] = (first <= last) ? ((last - first) + 1u) : 0u;

        if (first <= last) {
            for (unsigned k = first; k <= last; ++k) {
                const l3_authorization_word_pair *a2db0 = &raw2db0_13[k];
                const l3_authorization_word_pair *b2e30 = &raw2e30_13[idx - k];
                const uint64_t prod = (uint64_t)a2db0->lo * (uint64_t)b2e30->lo;
                const uint32_t old = conv.lo;
                conv.lo += (uint32_t)prod;
                conv.hi += b2e30->hi * a2db0->lo + b2e30->lo * a2db0->hi +
                           (uint32_t)(prod >> 32) + (uint32_t)(conv.lo < old);
            }

            mixer_pair_range_sum(prefix2db0_13, first, last, &sum2db0);
            mixer_pair_range_sum(prefix2e30_13, idx - last, idx - first, &sum2e30);

            const uint32_t count = (last - first) + 1u;
            const uint64_t count_prod = (uint64_t)count * 0xcf2f05a9u;
            const uint32_t count_lo = (uint32_t)count_prod;
            const uint32_t count_hi = (uint32_t)(count_prod >> 32);
            const uint64_t conv_scaled = (uint64_t)conv.lo * 0x316b3801u;
            const uint32_t conv_scaled_lo = (uint32_t)conv_scaled;
            const uint64_t sum2db0_scaled = (uint64_t)sum2db0.lo * 0x6f5ee343u;
            const uint32_t sum2db0_scaled_lo = (uint32_t)sum2db0_scaled;
            const uint64_t sum2e30_scaled = (uint64_t)sum2e30.lo * 0x3c74eea3u;
            const uint32_t sum2e30_scaled_lo = (uint32_t)sum2e30_scaled;

            const uint32_t t0 = count_lo + 0x2d81fda0u;
            const uint32_t t1 = t0 + conv_scaled_lo;
            const uint32_t t2 = t1 + sum2db0_scaled_lo;
            u = t2 + sum2e30_scaled_lo;

            v = count * 0x8f9be348u + count_hi + 0xbe5b5308u +
                add_carry_u32(count_lo, 0x2d81fda0u) +
                conv.hi * 0x316b3801u + conv.lo * 0x6f71acd0u +
                (uint32_t)(conv_scaled >> 32) + add_carry_u32(t0, conv_scaled_lo) +
                sum2db0.hi * 0x6f5ee343u + sum2db0.lo * 0x3aa8f59du +
                (uint32_t)(sum2db0_scaled >> 32) + add_carry_u32(t1, sum2db0_scaled_lo) +
                sum2e30.hi * 0x3c74eea3u + sum2e30.lo * 0x8100919fu +
                (uint32_t)(sum2e30_scaled >> 32) + add_carry_u32(t2, sum2e30_scaled_lo);
        }

        const uint32_t out_lo_mul = u * 0x721c638bu;
        out_pairs26[idx].lo = out_lo_mul + 0x2022e078u;
        out_pairs26[idx].hi = v * 0x721c638bu + u * 0x8a45d901u +
                              multiply_high_u32(u, 0x721c638bu) + 0xad6aadf2u +
                              add_carry_u32(out_lo_mul, 0x2022e078u);
        dst->convolution26[idx] = conv;
        dst->range2db0_26[idx] = sum2db0;
        dst->range2e30_26[idx] = sum2e30;
        dst->pre_reduce26[idx].lo = u;
        dst->pre_reduce26[idx].hi = v;
        dst->output26[idx] = out_pairs26[idx];
    }
    return 0;
}

int l3_authorization_stage4_stage_accumulate_and_convolve26(
    const int32_t local618[13],
    const int32_t round_ec8_words13[13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    l3_authorization_stage4_stage_convolution_trace *out) {
    if (!local618 || !round_ec8_words13 || !out_pairs26) return -1;

    l3_authorization_word_pair raw2db0[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair prefix2db0[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair raw2e30[L3_AUTH_ROUND_CORE_PAIRS13];
    l3_authorization_word_pair prefix2e30[L3_AUTH_ROUND_CORE_PAIRS13];

    int rc = l3_authorization_core_post_accum2db0_2e30_pairs13(
        local618, round_ec8_words13, raw2db0, prefix2db0, raw2e30, prefix2e30, NULL);
    if (rc != 0) return rc;
    return l3_authorization_stage4_stage_convolution26(
        raw2db0, prefix2db0, raw2e30, prefix2e30, out_pairs26, out);
}
