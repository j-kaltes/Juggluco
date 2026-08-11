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
#include "authorization_vector_feedback.h"

#include <stddef.h>
#include <string.h>

static uint32_t feedback_add_carry_u32(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a + (uint64_t)b) >> 32);
}

static uint32_t feedback_multiply_high_u32(uint32_t a, uint32_t b) {
    return (uint32_t)(((uint64_t)a * (uint64_t)b) >> 32);
}

static uint32_t feedback_multiply_low_u32(uint32_t a, uint32_t b) {
    return (uint32_t)((uint64_t)a * (uint64_t)b);
}

static const uint32_t k_vector_feedback_f4191510[8] = {
    0x481c91b3u, 0x768a8d8bu, 0x9d416b15u, 0x1fd2f8a3u,
    0xeffe87f9u, 0x19558651u, 0xfadaaf8du, 0x2b584059u
};
static const uint32_t k_vector_feedback_f4191530[8] = {
    0x70e55fcfu, 0x4e554995u, 0x34d2e04bu, 0x1df3c13fu,
    0xe5952df5u, 0x3d0455f5u, 0xbf894194u, 0x5971ed19u
};

static const l3_authorization_vector_feedback_pair32 k_vector_feedback_f41a2830[16] = {
    {0x9d575f07u, 0xcdbd0032u}, {0x26f5b1c2u, 0x330ed8dfu},
    {0xb094047du, 0xa860b18bu}, {0x3a325738u, 0x1db28a38u},
    {0xc3d0a9f3u, 0x830462e4u}, {0x4d6efcaeu, 0xf8563b91u},
    {0xd70d4f69u, 0x6da8143du}, {0x60aba224u, 0xd2f9eceau},
    {0xea49f4dfu, 0x484bc596u}, {0x73e8479au, 0xbd9d9e43u},
    {0xfd869a55u, 0x22ef76efu}, {0x8724ed10u, 0x98414f9cu},
    {0x76de141cu, 0x08759d80u}, {0x007c66d7u, 0x7dc7762du},
    {0x8a1ab992u, 0xe3194ed9u}, {0x13b90c4du, 0x586b2786u}
};

static l3_authorization_vector_feedback_pair32 vector_feedback_fold_step(
    l3_authorization_vector_feedback_pair32 v,
    const l3_authorization_vector_feedback_pair32 table[16]) {
    const unsigned idx = v.lo & 0x0fu;
    const uint32_t shifted_lo = (v.lo >> 4) | (v.hi << 28);
    const uint32_t shifted_hi = v.hi >> 4;
    l3_authorization_vector_feedback_pair32 out;
    out.lo = shifted_lo + table[idx].lo;
    out.hi = shifted_hi + table[idx].hi + feedback_add_carry_u32(shifted_lo, table[idx].lo);
    return out;
}

static l3_authorization_vector_feedback_pair32 vector_feedback_fold_rounds(
    l3_authorization_vector_feedback_pair32 v,
    const l3_authorization_vector_feedback_pair32 table[16],
    unsigned rounds) {
    for (unsigned i = 0; i < rounds; ++i) v = vector_feedback_fold_step(v, table);
    return v;
}

static l3_authorization_vector_feedback_pair32 vector_feedback_seed2830_one(uint32_t transformed,
                                                     l3_authorization_vector_feedback_pair32 *seed_out,
                                                     l3_authorization_vector_feedback_pair32 *folded_out) {
    const uint32_t lo0 = feedback_multiply_low_u32(transformed, 0xa37f3821u);
    l3_authorization_vector_feedback_pair32 seed = {
        lo0 + 0x13f6bf1au,
        (uint32_t)(transformed * 0xb8fe49d0u + feedback_multiply_high_u32(transformed, 0xa37f3821u) +
                   0x1b5bb304u + feedback_add_carry_u32(lo0, 0x13f6bf1au))
    };
    const l3_authorization_vector_feedback_pair32 folded =
        vector_feedback_fold_rounds(seed, k_vector_feedback_f41a2830, 8u);
    const uint32_t lo1 = feedback_multiply_low_u32(transformed, 0x1b149ddbu);
    l3_authorization_vector_feedback_pair32 out = {
        lo1 + 0x8ee4dbbcu,
        (uint32_t)(transformed * 0x7dc007f5u + feedback_multiply_high_u32(transformed, 0x1b149ddbu) +
                   folded.lo * 0xbc581985u + 0x6b2f7370u +
                   feedback_add_carry_u32(lo1, 0x8ee4dbbcu))
    };
    if (seed_out) *seed_out = seed;
    if (folded_out) *folded_out = folded;
    return out;
}

int l3_authorization_vector_feedback_seed2830_pairs13(
    const int32_t seed_words13[L3_AUTH_VECTOR_FEEDBACK_WORDS13],
    l3_authorization_vector_feedback_pair32 out_pairs13[L3_AUTH_VECTOR_FEEDBACK_PAIRS13],
    l3_authorization_vector_feedback_seed2830_trace *trace) {
    if (!seed_words13 || !out_pairs13) return -1;

    l3_authorization_vector_feedback_seed2830_trace tmp;
    l3_authorization_vector_feedback_seed2830_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    for (unsigned lane = 0; lane < 13u; ++lane) {
        const unsigned idx = lane & 7u;
        const uint32_t pre = (uint32_t)seed_words13[lane] * k_vector_feedback_f4191510[idx] +
                             k_vector_feedback_f4191530[idx];
        const uint32_t transformed = pre * 0x3e6d7d45u + 0x02d890ad6u;
        dst->transformed13[lane] = transformed;
        out_pairs13[lane] = vector_feedback_seed2830_one(
            transformed,
            &dst->seed_after_first_affine13[lane],
            &dst->folded2830_13[lane]);
        dst->output_pairs13[lane] = out_pairs13[lane];
    }
    return 0;
}

static const l3_authorization_vector_feedback_pair32 k_vector_feedback_f41a28b0[16] = {
    {0x8f92939cu, 0x2271d655u}, {0xcdcf08e9u, 0x7523e291u},
    {0x0c0b7e36u, 0xc7d5eeceu}, {0x4a47f383u, 0x1a87fb0au},
    {0x888468d0u, 0x6d3a0746u}, {0xc6c0de1du, 0xbfec1382u},
    {0x04fd536au, 0x029e1fbfu}, {0x4339c8b7u, 0x55502bfbu},
    {0x81763e04u, 0xa8023837u}, {0xdbeb5e82u, 0xff9380afu},
    {0x1a27d3cfu, 0x42458cecu}, {0x5864491cu, 0x94f79928u},
    {0x96a0be69u, 0xe7a9a564u}, {0xd4dd33b6u, 0x3a5bb1a0u},
    {0x1319a903u, 0x8d0dbdddu}, {0x51561e50u, 0xdfbfca19u}
};

static const l3_authorization_vector_feedback_pair32 k_vector_feedback_f41a2930[16] = {
    {0xfaf93fa3u, 0xe8da407bu}, {0x506d6b51u, 0x135c7b1fu},
    {0xa5e196ffu, 0x4ddeb5c2u}, {0xfb55c2adu, 0x7860f065u},
    {0x50c9ee5bu, 0xa2e32b09u}, {0xa63e1a09u, 0xdd6565acu},
    {0xfbb245b7u, 0x07e7a04fu}, {0x51267165u, 0x3269daf3u},
    {0xa69a9d13u, 0x6cec1596u}, {0xfc0ec8c1u, 0x976e5039u},
    {0x5182f46fu, 0xc1f08addu}, {0x4fb4653eu, 0xf44f1b4bu},
    {0xa52890ecu, 0x2ed155eeu}, {0xfa9cbc9au, 0x59539091u},
    {0x5010e848u, 0x83d5cb35u}, {0xa58513f6u, 0xbe5805d8u}
};

static const l3_authorization_vector_feedback_pair32 k_vector_feedback_f41a29b0[16] = {
    {0xee0c6d5au, 0xe9c6d3e5u}, {0x536b3fe6u, 0x1b8b60a6u},
    {0xb8ca1272u, 0x4d4fed66u}, {0x1e28e4feu, 0x7f147a27u},
    {0x0bf1e297u, 0xa4ca77b3u}, {0x7150b523u, 0xd68f0473u},
    {0xd6af87afu, 0x08539133u}, {0xc4788548u, 0x3e098ebfu},
    {0x29d757d4u, 0x6fce1b80u}, {0x8f362a60u, 0x9192a840u},
    {0x7cff27f9u, 0xc748a5ccu}, {0xe25dfa85u, 0xf90d328cu},
    {0x47bccd11u, 0x2ad1bf4du}, {0x3585caaau, 0x5087bcd9u},
    {0x9ae49d36u, 0x824c4999u}, {0x00436fc2u, 0xb410d65au}
};

static void vector_feedback_ladder_run7(const l3_authorization_vector_feedback_pair32 table[16], uint32_t *lo, uint32_t *hi) {
    l3_authorization_vector_feedback_pair32 v = {*lo, *hi};
    v = vector_feedback_fold_rounds(v, table, 7u);
    *lo = v.lo;
    *hi = v.hi;
}

static uint32_t vector_feedback_ladder_mix(const l3_authorization_vector_feedback_pair32 table[16], uint32_t lo, uint32_t hi) {
    vector_feedback_ladder_run7(table, &lo, &hi);
    const unsigned nib = lo & 0x0fu;
    const uint32_t step8_low = ((lo >> 4) | (hi << 28)) + table[nib].lo;
    return (step8_low >> 4) + table[step8_low & 0x0fu].lo;
}

static void vector_feedback_add_pair64(l3_authorization_vector_feedback_pair32 *acc, const uint32_t addend[2]) {
    const uint32_t old = acc->lo;
    acc->lo = old + addend[0];
    acc->hi = acc->hi + addend[1] + feedback_add_carry_u32(old, addend[0]);
}

static void vector_feedback_mul_pair64(const l3_authorization_vector_feedback_pair32 *a, const uint32_t b[2], uint32_t out[2]) {
    const uint64_t p = (uint64_t)a->lo * (uint64_t)b[0];
    out[0] = (uint32_t)p;
    out[1] = (uint32_t)(a->hi * b[0] + a->lo * b[1] + (uint32_t)(p >> 32));
}

int l3_authorization_vector_feedback_lane_scalar_core(
    uint32_t param3,
    uint32_t param4,
    const l3_authorization_vector_feedback_pair32 *rolling_pair,
    l3_authorization_vector_feedback_lane_scalars *out) {
    if (!rolling_pair || !out) return -1;

    const uint32_t p3_mul996 = feedback_multiply_low_u32(param3, 0x9960078fu);
    const uint32_t p3_mule58 = feedback_multiply_low_u32(param3, 0xe588ad3fu);
    const uint32_t base_lo = p3_mul996 + 0xb0d7ebe2u;
    const uint32_t base_hi = p3_mule58 + 0x959d2bd6u;

    const uint32_t cur_lo = rolling_pair->lo;
    const uint32_t cur_hi = rolling_pair->hi;
    const uint64_t p_cur_base_hi = (uint64_t)cur_lo * (uint64_t)base_hi;
    const uint32_t p_cur_base_hi_lo = (uint32_t)p_cur_base_hi;
    const uint32_t lane_lo = base_lo + p_cur_base_hi_lo;

    const uint32_t base_hi_affine =
        param4 * 0xe588ad3fu +
        param3 * 0xe3a64543u +
        feedback_multiply_high_u32(param3, 0xe588ad3fu) +
        0xbd249cd1u +
        (uint32_t)(0x6a62d429u < p3_mule58);

    const uint32_t lane_hi =
        param4 * 0x9960078fu +
        param3 * 0x7cb07569u +
        feedback_multiply_high_u32(param3, 0x9960078fu) +
        0xd41251cfu +
        (uint32_t)(0x4f28141du < p3_mul996) +
        base_hi_affine * cur_lo +
        base_hi * cur_hi +
        (uint32_t)(p_cur_base_hi >> 32) +
        feedback_add_carry_u32(base_lo, p_cur_base_hi_lo);

    const uint32_t lane_mul_c40 = feedback_multiply_low_u32(lane_lo, 0xc40e41c3u);
    uint32_t ladder_lo = lane_mul_c40 + 0xc7139db5u;
    uint32_t ladder_hi =
        lane_hi * 0xc40e41c3u +
        lane_lo * 0xd94e02c1u +
        feedback_multiply_high_u32(lane_lo, 0xc40e41c3u) +
        0x4986767du +
        (uint32_t)(0x038ec624au < lane_mul_c40);
    vector_feedback_ladder_run7(k_vector_feedback_f41a28b0, &ladder_lo, &ladder_hi);

    const uint64_t p_ladder_900 = (uint64_t)ladder_lo * 0x90000000ull;
    const uint32_t p_ladder_900_lo = (uint32_t)p_ladder_900;
    const uint32_t lane_mul_9ee = feedback_multiply_low_u32(lane_lo, 0x9ee80505u);
    const uint32_t mid_sum = lane_mul_9ee + p_ladder_900_lo;
    const uint32_t second_lo = mid_sum + 0x7f771555u;
    const uint32_t mul_bce_low = feedback_multiply_low_u32(second_lo, 0xbce4572fu);
    const uint32_t second_hi =
        lane_hi * 0x9ee80505u +
        lane_lo * 0x7599a150u +
        feedback_multiply_high_u32(lane_lo, 0x9ee80505u) +
        ladder_lo * 0xd0842d66u +
        (uint32_t)(p_ladder_900 >> 32) +
        ladder_hi * 0x90000000u +
        feedback_add_carry_u32(lane_mul_9ee, p_ladder_900_lo) +
        0xf2cad220u +
        (uint32_t)(0x8088eaaau < mid_sum);

    const uint32_t scale = mul_bce_low + 0x4490efc1u;
    const uint32_t mul_380_low = feedback_multiply_low_u32(second_lo, 0x3804b2a2u);
    const uint32_t vector_bias = mul_380_low + 0x9ed2cf7eu;

    out->base_pair[0] = base_lo;
    out->base_pair[1] = base_hi;
    out->lane_pair[0] = lane_lo;
    out->lane_pair[1] = lane_hi;
    out->ladder28b0_pair[0] = ladder_lo;
    out->ladder28b0_pair[1] = ladder_hi;
    out->second_pair[0] = second_lo;
    out->second_pair[1] = second_hi;
    out->scale = scale;
    out->vector_bias = vector_bias;
    out->mul_bce_low = mul_bce_low;
    out->mul_380_low = mul_380_low;
    return 0;
}

static void vector_feedback_scale_pair(const l3_authorization_vector_feedback_lane_scalars *s, uint32_t out_pair[2]) {
    out_pair[0] = s->scale;
    out_pair[1] =
        s->second_pair[1] * 0xbce4572fu +
        s->second_pair[0] * 0x1b400ea2u +
        feedback_multiply_high_u32(s->second_pair[0], 0xbce4572fu) +
        0xef546a3cu +
        (uint32_t)(0xbb6f103eu < s->mul_bce_low);
}

static void vector_feedback_vector_bias_pair(const l3_authorization_vector_feedback_lane_scalars *s, uint32_t out_pair[2]) {
    out_pair[0] = s->vector_bias;
    out_pair[1] =
        s->second_pair[1] * 0x3804b2a2u +
        s->second_pair[0] * 0x645f6155u +
        feedback_multiply_high_u32(s->second_pair[0], 0x3804b2a2u) +
        0x9a7b9809u +
        (uint32_t)(0x612d3081u < s->mul_380_low);
}

int l3_authorization_vector_feedback_vector_feedback_stores(
    const l3_authorization_vector_feedback_pair32 seed_pairs13[L3_AUTH_VECTOR_FEEDBACK_PAIRS13],
    const l3_authorization_vector_feedback_lane_scalars *lane_scalars,
    unsigned lane_index,
    l3_authorization_vector_feedback_pair32 state_pairs26[L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26],
    l3_authorization_vector_feedback_vector_feedback_result *out) {
    if (!seed_pairs13 || !lane_scalars || !state_pairs26) return -1;
    if (lane_index >= L3_AUTH_VECTOR_FEEDBACK_PAIRS13 || lane_index + 11u >= L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26) return -2;

    uint32_t scale_pair[2];
    uint32_t bias_pair[2];
    vector_feedback_scale_pair(lane_scalars, scale_pair);
    vector_feedback_vector_bias_pair(lane_scalars, bias_pair);

    if (out) {
        out->scale_pair[0] = scale_pair[0];
        out->scale_pair[1] = scale_pair[1];
        out->vector_bias_pair[0] = bias_pair[0];
        out->vector_bias_pair[1] = bias_pair[1];
    }

    for (unsigned j = 0; j < 12u; ++j) {
        uint32_t prod[2];
        vector_feedback_mul_pair64(&seed_pairs13[j], scale_pair, prod);
        if (out) {
            out->product_pairs12[j][0] = prod[0];
            out->product_pairs12[j][1] = prod[1];
        }
        vector_feedback_add_pair64(&state_pairs26[lane_index + j], bias_pair);
        vector_feedback_add_pair64(&state_pairs26[lane_index + j], prod);
    }
    return 0;
}

int l3_authorization_vector_feedback_lane_scalar_tail(
    const l3_authorization_vector_feedback_pair32 *seed_tail_pair,
    const l3_authorization_vector_feedback_lane_scalars *lane_scalars,
    unsigned lane_index,
    l3_authorization_vector_feedback_pair32 state_pairs26[L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26],
    l3_authorization_vector_feedback_lane_tail_result *out) {
    if (!seed_tail_pair || !lane_scalars || !state_pairs26) return -1;
    if (lane_index + 12u >= L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26) return -2;

    uint32_t scale_pair[2];
    uint32_t bias_pair[2];
    vector_feedback_scale_pair(lane_scalars, scale_pair);
    vector_feedback_vector_bias_pair(lane_scalars, bias_pair);

    uint32_t tail_product[2];
    vector_feedback_mul_pair64(seed_tail_pair, scale_pair, tail_product);
    vector_feedback_add_pair64(&state_pairs26[lane_index + 12u], bias_pair);
    vector_feedback_add_pair64(&state_pairs26[lane_index + 12u], tail_product);

    const uint32_t cur_lo = state_pairs26[lane_index].lo;
    const uint32_t cur_hi = state_pairs26[lane_index].hi;
    const uint32_t next_lo_old = state_pairs26[lane_index + 1u].lo;
    const uint32_t next_hi_old = state_pairs26[lane_index + 1u].hi;

    const uint64_t p_cur = (uint64_t)cur_lo * 0x22f66155ull;
    const uint32_t p_cur_lo = (uint32_t)p_cur;
    const uint32_t aff_lo = p_cur_lo + 0x04eca57fu;
    const uint32_t aff_hi =
        cur_hi * 0x22f66155u +
        cur_lo * 0x6dbb2270u +
        (uint32_t)(p_cur >> 32) +
        0xb1b71aa2u +
        (uint32_t)(0xfb135a80u < p_cur_lo);

    uint32_t lad2930_lo = aff_lo;
    uint32_t lad2930_hi = aff_hi;
    vector_feedback_ladder_run7(k_vector_feedback_f41a2930, &lad2930_lo, &lad2930_hi);

    const uint64_t p_lad = (uint64_t)lad2930_lo * 0x3c4bc5e9ull;
    const uint32_t p_lad_lo = (uint32_t)p_lad;
    const uint32_t post2930_lo = p_lad_lo + 0x3be52d8cu;
    const uint32_t post2930_hi =
        lad2930_hi * 0x3c4bc5e9u +
        lad2930_lo * 0xabea7334u +
        (uint32_t)(p_lad >> 32) +
        0x80b656dau +
        (uint32_t)(0xc41ad273u < p_lad_lo);

    const uint32_t p_post_335 = feedback_multiply_low_u32(post2930_lo, 0x3350441bu);
    const uint32_t lad29_in_lo = p_post_335 + 0xb376b04bu;
    const uint32_t lad29_in_hi =
        post2930_hi * 0x3350441bu +
        post2930_lo * 0x10df637fu +
        feedback_multiply_high_u32(post2930_lo, 0x3350441bu) +
        0x88c7f5f1u +
        (uint32_t)(0x4c894fb4u < p_post_335);
    const uint32_t lad29_mix = vector_feedback_ladder_mix(k_vector_feedback_f41a29b0, lad29_in_lo, lad29_in_hi);

    const uint64_t p_post = (uint64_t)post2930_lo * 0x01ed22fbull;
    const uint32_t p_post_lo = (uint32_t)p_post;
    const uint64_t p_corr = (uint64_t)p_post_lo * 0x57aecccfull;
    const uint32_t corr_lo = (uint32_t)p_corr;
    const uint32_t sum_next_lo = corr_lo + next_lo_old;
    const uint32_t final_next_lo = sum_next_lo + 0x47538825u;

    const uint32_t corr_inner =
        post2930_hi * 0x01ed22fbu +
        post2930_lo * 0x9db6e7b0u +
        (uint32_t)(p_post >> 32) +
        lad29_mix * 0x290355f0u;
    const uint32_t final_next_hi =
        corr_inner * 0x57aecccful +
        p_post_lo * 0x0b2429dfu +
        (uint32_t)(p_corr >> 32) +
        next_hi_old +
        feedback_add_carry_u32(corr_lo, next_lo_old) +
        0xef65f558u +
        (uint32_t)(0xb8ac77dau < sum_next_lo);

    state_pairs26[lane_index + 1u].lo = final_next_lo;
    state_pairs26[lane_index + 1u].hi = final_next_hi;

    if (out) {
        out->current_affine_pair[0] = aff_lo;
        out->current_affine_pair[1] = aff_hi;
        out->ladder2930_pair[0] = lad2930_lo;
        out->ladder2930_pair[1] = lad2930_hi;
        out->post2930_pair[0] = post2930_lo;
        out->post2930_pair[1] = post2930_hi;
        out->ladder29b0_mix = lad29_mix;
        out->tail_product_pair[0] = tail_product[0];
        out->tail_product_pair[1] = tail_product[1];
        out->final_lane1_pair[0] = final_next_lo;
        out->final_lane1_pair[1] = final_next_hi;
        out->final_lane12_pair[0] = state_pairs26[lane_index + 12u].lo;
        out->final_lane12_pair[1] = state_pairs26[lane_index + 12u].hi;
    }
    return 0;
}

int l3_authorization_vector_feedback_run13_update(
    const int32_t seed_words13[L3_AUTH_VECTOR_FEEDBACK_WORDS13],
    uint32_t param3,
    uint32_t param4,
    l3_authorization_vector_feedback_pair32 state_pairs26[L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26],
    l3_authorization_vector_feedback_run13_trace *out) {
    if (!seed_words13 || !state_pairs26) return -1;

    l3_authorization_vector_feedback_pair32 seed_pairs13[L3_AUTH_VECTOR_FEEDBACK_PAIRS13];
    l3_authorization_vector_feedback_seed2830_trace seed_trace;
    int rc = l3_authorization_vector_feedback_seed2830_pairs13(seed_words13, seed_pairs13, &seed_trace);
    if (rc != 0) return rc;

    if (out) {
        memset(out, 0, sizeof(*out));
        out->seed = seed_trace;
        memcpy(out->seed_pairs13, seed_pairs13, sizeof(seed_pairs13));
    }

    for (unsigned lane = 0; lane < L3_AUTH_VECTOR_FEEDBACK_PAIRS13; ++lane) {
        if (out) {
            memcpy(out->state_before_lane13[lane], state_pairs26,
                   sizeof(out->state_before_lane13[lane]));
        }
        l3_authorization_vector_feedback_lane_scalars scalars;
        rc = l3_authorization_vector_feedback_lane_scalar_core(param3, param4, &state_pairs26[lane], &scalars);
        if (rc != 0) return rc;
        if (out) out->lane_scalars[lane] = scalars;
        rc = l3_authorization_vector_feedback_vector_feedback_stores(seed_pairs13, &scalars, lane, state_pairs26, NULL);
        if (rc != 0) return rc;
        rc = l3_authorization_vector_feedback_lane_scalar_tail(&seed_pairs13[12], &scalars, lane, state_pairs26,
                                             out ? &out->lane_tails[lane] : NULL);
        if (rc != 0) return rc;
        if (out) {
            memcpy(out->state_after_lane13[lane], state_pairs26,
                   sizeof(out->state_after_lane13[lane]));
        }
    }

    if (out) memcpy(out->final_state26, state_pairs26, sizeof(out->final_state26));
    return 0;
}

static const l3_authorization_vector_feedback_pair32 k_vector_feedback_f41a2a30[16] = {
    {0x01500ba3u, 0x0e378ebcu}, {0x44974dc3u, 0xf0d34786u},
    {0x87de8fe3u, 0xe36f0050u}, {0xcb25d203u, 0xd60ab91au},
    {0x0e6d1423u, 0xc8a671e5u}, {0x51b45643u, 0xbb422aafu},
    {0x94fb9863u, 0xaddde379u}, {0xd842da83u, 0x90799c43u},
    {0xe715faa4u, 0x8959c869u}, {0x2a5d3cc4u, 0x7bf58134u},
    {0x6da47ee4u, 0x6e9139feu}, {0xb0ebc104u, 0x512cf2c8u},
    {0xf4330324u, 0x43c8ab92u}, {0x377a4544u, 0x3664645du},
    {0x7ac18764u, 0x29001d27u}, {0xbe08c984u, 0x1b9bd5f1u}
};

static const uint32_t k_vector_feedback_f4191550[8] = {
    0xedaf4f0fu, 0x791b4987u, 0x3216c3d5u, 0x1af0477du,
    0x0d7f0071u, 0xad3add39u, 0xf17b619du, 0x794d3ae5u
};

static const uint32_t k_vector_feedback_f4191570[8] = {
    0x05d63483u, 0x5d8922dbu, 0xae82fb57u, 0xc78a2693u,
    0xf349de85u, 0xa42d39e5u, 0xe9282459u, 0x2b8377ffu
};

static l3_authorization_vector_feedback_pair32 vector_feedback_fold_step_2a30(l3_authorization_vector_feedback_pair32 v) {
    return vector_feedback_fold_step(v, k_vector_feedback_f41a2a30);
}

int l3_authorization_vector_feedback_export_words13(
    const l3_authorization_vector_feedback_pair32 state_pairs26[L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26],
    uint32_t out_words13[L3_AUTH_VECTOR_FEEDBACK_WORDS13],
    l3_authorization_vector_feedback_export_trace *trace) {
    if (!state_pairs26 || !out_words13) return -1;

    l3_authorization_vector_feedback_export_trace tmp;
    l3_authorization_vector_feedback_export_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    uint32_t roll_lo = 0xeb251319u;
    uint32_t roll_hi = 0x356e3affu;

    for (unsigned lane = 0; lane < L3_AUTH_VECTOR_FEEDBACK_WORDS13; ++lane) {
        const l3_authorization_vector_feedback_pair32 in = state_pairs26[13u + lane];
        dst->input_pairs13[lane] = in;

        const uint64_t p_state = (uint64_t)in.lo * 0x90ea36c5ull;
        const uint32_t p_state_lo = (uint32_t)p_state;
        const uint64_t p_roll = (uint64_t)roll_lo * 0x0c211b3bull;
        const uint32_t p_roll_lo = (uint32_t)p_roll;
        const uint32_t sum = p_roll_lo + p_state_lo;
        const uint32_t affine_lo = sum + 0x5d72f4d2u;
        const uint64_t p_affine = (uint64_t)affine_lo * 0x896b2c31ull;
        const uint32_t p_affine_lo = (uint32_t)p_affine;
        const uint32_t fold_in_lo = p_affine_lo + 0x498b1ee7u;
        const uint32_t affine_hi =
            (uint32_t)(roll_hi * 0x0c211b3bu +
                       roll_lo * 0x415fa1a5u +
                       (uint32_t)(p_roll >> 32) +
                       in.hi * 0x90ea36c5u +
                       in.lo * 0x2285f8a3u +
                       (uint32_t)(p_state >> 32) +
                       feedback_add_carry_u32(p_roll_lo, p_state_lo) +
                       0x76e2ca77u +
                       (uint32_t)(0xa28d0b2du < sum)) * 0x896b2c31u +
            affine_lo * 0xa6cc6847u +
            (uint32_t)(p_affine >> 32) +
            0x1b9ae4b3u +
            (uint32_t)(0xb674e118u < p_affine_lo);

        l3_authorization_vector_feedback_pair32 v = {fold_in_lo, affine_hi};
        for (unsigned i = 0; i < 7u; ++i) v = vector_feedback_fold_step_2a30(v);
        const l3_authorization_vector_feedback_pair32 fold7 = v;

        const uint64_t p_fold7 = (uint64_t)fold7.lo * 0x49b51f63ull;
        const uint32_t p_fold7_lo = (uint32_t)p_fold7;

        v = vector_feedback_fold_step_2a30(v);
        v = vector_feedback_fold_step_2a30(v);
        v = vector_feedback_fold_step_2a30(v);
        v = vector_feedback_fold_step_2a30(v);
        v = vector_feedback_fold_step_2a30(v);
        v = vector_feedback_fold_step_2a30(v);
        v = vector_feedback_fold_step_2a30(v);
        const uint32_t final_pre_lo = ((v.lo >> 4) | (v.hi << 28)) +
                                      k_vector_feedback_f41a2a30[v.lo & 0x0fu].lo;
        const uint32_t final_mix = (final_pre_lo >> 4) + k_vector_feedback_f41a2a30[final_pre_lo & 0x0fu].lo;

        const uint32_t export_pre = affine_lo * 0x4322465bu + fold7.lo * 0x50000000u + 0xaaa54306u;
        const uint32_t out = export_pre * k_vector_feedback_f4191550[lane & 7u] +
                             k_vector_feedback_f4191570[lane & 7u];
        out_words13[lane] = out;

        roll_lo = p_fold7_lo + 0x6fbe8aa4u;
        roll_hi =
            fold7.hi * 0x49b51f63u +
            fold7.lo * 0xf804c734u +
            (uint32_t)(p_fold7 >> 32) +
            final_mix * 0x64ae09d0u +
            0xaf80a2e3u +
            (uint32_t)(0x9041755bu < p_fold7_lo);

        dst->affine_lo13[lane] = affine_lo;
        dst->fold7_pairs13[lane] = fold7;
        dst->fold_final_mix13[lane] = final_mix;
        dst->rolling_after13[lane][0] = roll_lo;
        dst->rolling_after13[lane][1] = roll_hi;
        dst->output_words13[lane] = out;
    }
    return 0;
}

int l3_authorization_vector_feedback_run13_update_export(
    const int32_t seed_words13[L3_AUTH_VECTOR_FEEDBACK_WORDS13],
    uint32_t param3,
    uint32_t param4,
    l3_authorization_vector_feedback_pair32 state_pairs26[L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26],
    uint32_t out_words13[L3_AUTH_VECTOR_FEEDBACK_WORDS13],
    l3_authorization_vector_feedback_run13_update_export_trace *trace) {
    if (!seed_words13 || !state_pairs26 || !out_words13) return -1;
    if (trace) memset(trace, 0, sizeof(*trace));
    int rc = l3_authorization_vector_feedback_run13_update(seed_words13, param3, param4, state_pairs26,
                                         trace ? &trace->update : NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_vector_feedback_export_words13(state_pairs26, out_words13,
                                       trace ? &trace->export_trace : NULL);
    if (rc != 0) return rc;
    if (trace) memcpy(trace->output_words13, out_words13, sizeof(trace->output_words13));
    return 0;
}
