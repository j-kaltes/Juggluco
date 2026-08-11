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
#include "authorization_vector_reduction.h"

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

static void nibble_ladder64_run7(const uint32_t tab[16][2], uint32_t *lo, uint32_t *hi) {
    uint32_t xlo = *lo;
    uint32_t xhi = *hi;
    for (unsigned i = 0; i < 7u; ++i) {
        unsigned nib = xlo & 0x0fu;
        uint32_t shifted = (xlo >> 4) | (xhi << 28);
        uint32_t tlo = tab[nib][0];
        uint32_t nlo = shifted + tlo;
        uint32_t nhi = (xhi >> 4) + tab[nib][1] + add_carry_u32(shifted, tlo);
        xlo = nlo;
        xhi = nhi;
    }
    *lo = xlo;
    *hi = xhi;
}

static const uint32_t k_f4190410[8] = {
    0xb2f90e93u, 0x6219996bu, 0x94712335u, 0x67e2db83u,
    0xe658ea99u, 0x21f187f1u, 0xcfbdbaadu, 0x03317ef9u
};

static const uint32_t k_f4190430[8] = {
    0xa380e5e6u, 0xca83656cu, 0xf14cb7e2u, 0xf2b4bd56u,
    0xdd71a5ccu, 0x9a31cdccu, 0x57e9fdcbu, 0xb5743770u
};

static const uint32_t k_f41a0770[16][2] = {
    {0xcf617fecu, 0x02e1d4d0u}, {0xfb0d15f1u, 0x12fc0cc2u},
    {0xe73374a9u, 0x2a35f354u}, {0xd359d361u, 0x316fd9e6u},
    {0xbf803219u, 0x48a9c078u}, {0xeb2bc81eu, 0x58c3f86au},
    {0xd75226d6u, 0x6ffddefcu}, {0xc378858eu, 0x7737c58eu},
    {0xef241b93u, 0x8751fd80u}, {0xdb4a7a4bu, 0x9e8be412u},
    {0xc770d903u, 0xa5c5caa4u}, {0xf31c6f08u, 0xb5e00296u},
    {0xdf42cdc0u, 0xcd19e928u}, {0xcb692c78u, 0xd453cfbau},
    {0xf714c27du, 0xe46e07acu}, {0xe33b2135u, 0xfba7ee3eu}
};

static const uint32_t k_f41a07f0[16][2] = {
    {0x0cf6c26cu, 0x5ae5e2fcu}, {0x5c8a5580u, 0x853a02dcu},
    {0xac1de894u, 0xbf8e22bcu}, {0x8ef2749fu, 0xe6d8d52eu},
    {0xde8607b3u, 0x112cf50eu}, {0xc15a93beu, 0x4877a780u},
    {0x10ee26d2u, 0x72cbc761u}, {0xf3c2b2ddu, 0xaa1679d2u},
    {0x435645f1u, 0xd46a99b3u}, {0x262ad1fcu, 0x0bb54c25u},
    {0x75be6510u, 0x36096c05u}, {0xc551f824u, 0x605d8be5u},
    {0xa826842fu, 0x97a83e57u}, {0xf7ba1743u, 0xc1fc5e37u},
    {0xda8ea34eu, 0xf94710a9u}, {0x2a223662u, 0x239b308au}
};

static void vector_reduce_seed_pair_from_token(uint32_t token, uint32_t out_pair[2]) {
    uint32_t lo0 = multiply_low_u32(token, 0x54d868e9u);
    uint32_t lo = lo0 + 0x35440bfbu;
    uint32_t hi = multiply_low_u32(token, 0xeb858586u) + multiply_high_u32(token, 0x54d868e9u) +
                  0x43949ba4u + (uint32_t)(0xcabbf404u < lo0);
    uint32_t ladder = nibble_ladder64_run8_low(k_f41a0770, lo, hi);

    uint32_t lo1 = multiply_low_u32(token, 0x7578ea6fu);
    out_pair[0] = lo1 + 0x2e8dcd5bu;
    out_pair[1] = multiply_low_u32(token, 0x34c2671du) + multiply_high_u32(token, 0x7578ea6fu) +
                  multiply_low_u32(ladder, 0x1e3ade69u) + 0xd86322f5u +
                  (uint32_t)(0xd17232a4u < lo1);
}

int l3_authorization_vector_reduce_seed_pairs13(
    const int32_t param1_13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS],
    uint32_t out_seed_pairs13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS][2]) {
    if (!param1_13 || !out_seed_pairs13) {
        return -1;
    }

    for (unsigned i = 0; i < L3_AUTH_VECTOR_REDUCE_SEED_WORDS; ++i) {
        unsigned lane = i & 7u;
        uint32_t token = ((uint32_t)param1_13[i] * k_f4190410[lane] +
                          k_f4190430[lane]) * 0xf02fdc37u + 0x7287110cu;
        vector_reduce_seed_pair_from_token(token, out_seed_pairs13[i]);
    }

    return 0;
}

int l3_authorization_vector_reduce_lane_scalar_core(
    uint32_t param3,
    uint32_t param4,
    const uint32_t rolling_pair[2],
    l3_authorization_vector_reduce_lane_scalars *out) {
    if (!rolling_pair || !out) {
        return -1;
    }

    uint32_t p3_mul395 = multiply_low_u32(param3, 0x395a8260u);
    uint32_t p3_mul8b4 = multiply_low_u32(param3, 0x8b4a12e5u);
    uint32_t base_lo = p3_mul395 + 0xcdcd9fc4u;
    uint32_t base_hi = p3_mul8b4 + 0xfa09ec12u;

    uint32_t cur_lo = rolling_pair[0];
    uint32_t cur_hi = rolling_pair[1];
    uint64_t prod_cur_base_hi = (uint64_t)cur_lo * (uint64_t)base_hi;
    uint32_t prod_cur_base_hi_lo = (uint32_t)prod_cur_base_hi;
    uint32_t lane_lo = base_lo + prod_cur_base_hi_lo;

    uint32_t base_hi_affine =
        param4 * 0x8b4a12e5u +
        param3 * 0x6e8af016u +
        multiply_high_u32(param3, 0x8b4a12e5u) +
        0x11f6f83eu +
        (uint32_t)(0x05f613edu < p3_mul8b4);

    uint32_t lane_hi =
        param4 * 0x395a8260u +
        param3 * 0x8ec7e360u +
        multiply_high_u32(param3, 0x395a8260u) +
        0x5e78992du +
        (uint32_t)(0x3232603bu < p3_mul395) +
        base_hi_affine * cur_lo +
        base_hi * cur_hi +
        (uint32_t)(prod_cur_base_hi >> 32) +
        add_carry_u32(base_lo, prod_cur_base_hi_lo);

    uint32_t lane_mul_cfc = multiply_low_u32(lane_lo, 0xcfc10609u);
    uint32_t ladder_lo = lane_mul_cfc + 0xb5fa40beu;
    uint32_t ladder_hi =
        lane_hi * 0xcfc10609u +
        lane_lo * 0x0a02b571u +
        multiply_high_u32(lane_lo, 0xcfc10609u) +
        0x1505ce6cu +
        (uint32_t)(0x4a05bf41u < lane_mul_cfc);

    nibble_ladder64_run7(k_f41a07f0, &ladder_lo, &ladder_hi);

    uint64_t prod_ladder_b = (uint64_t)ladder_lo * 0xb0000000ull;
    uint32_t prod_ladder_b_lo = (uint32_t)prod_ladder_b;
    uint32_t prod_lane_107 = multiply_low_u32(lane_lo, 0x1074257du);
    uint32_t mid_sum = prod_lane_107 + prod_ladder_b_lo;
    uint32_t second_lo = mid_sum + 0x087a9e1au;
    uint32_t mul34_low = multiply_low_u32(second_lo, 0x34b174bbu);

    uint32_t second_hi =
        lane_hi * 0x1074257du +
        lane_lo * 0xaadb97d5u +
        multiply_high_u32(lane_lo, 0x1074257du) +
        ladder_lo * 0xc5e6edf2u +
        (uint32_t)(prod_ladder_b >> 32) +
        ladder_hi * 0xb0000000u +
        add_carry_u32(prod_lane_107, prod_ladder_b_lo) +
        0x0550feaau +
        (uint32_t)(0xf78561e5u < mid_sum);

    uint32_t scale = mul34_low + 0xec8fedc6u;
    uint32_t mul6e_low = multiply_low_u32(second_lo, 0x6e21b4e2u);
    uint32_t vector_bias = mul6e_low + 0x9839fba4u;

    out->base_pair[0] = base_lo;
    out->base_pair[1] = base_hi;
    out->lane_pair[0] = lane_lo;
    out->lane_pair[1] = lane_hi;
    out->ladder7_pair[0] = ladder_lo;
    out->ladder7_pair[1] = ladder_hi;
    out->second_pair[0] = second_lo;
    out->second_pair[1] = second_hi;
    out->scale = scale;
    out->vector_bias = vector_bias;
    out->mul34_low = mul34_low;
    out->mul6e_low = mul6e_low;
    return 0;
}

static void pair64_add(uint32_t acc[2], const uint32_t addend[2]) {
    uint32_t old = acc[0];
    acc[0] = old + addend[0];
    acc[1] = acc[1] + addend[1] + add_carry_u32(old, addend[0]);
}

static void pair64_multiply(const uint32_t a[2], const uint32_t b[2], uint32_t out[2]) {
    uint64_t p = (uint64_t)a[0] * (uint64_t)b[0];
    out[0] = (uint32_t)p;
    out[1] = (uint32_t)(
        a[1] * b[0] +
        a[0] * b[1] +
        (uint32_t)(p >> 32));
}

int l3_authorization_vector_reduce_vector_feedback_stores(
    const uint32_t seed_pairs13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS][2],
    const l3_authorization_vector_reduce_lane_scalars *lane_scalars,
    unsigned lane_index,
    uint32_t state_pairs26[L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2],
    l3_authorization_vector_reduce_vector_feedback_result *out) {
    if (!seed_pairs13 || !lane_scalars || !state_pairs26) {
        return -1;
    }
    if (lane_index >= L3_AUTH_VECTOR_REDUCE_SEED_WORDS ||
        lane_index + 11u >= L3_AUTH_VECTOR_REDUCE_STATE_PAIRS) {
        return -2;
    }

    uint32_t scale_pair[2];
    scale_pair[0] = lane_scalars->scale;
    scale_pair[1] =
        lane_scalars->second_pair[1] * 0x34b174bbu +
        lane_scalars->second_pair[0] * 0xe04d7355u +
        multiply_high_u32(lane_scalars->second_pair[0], 0x34b174bbu) +
        0xa20a0591u +
        (uint32_t)(0x13701239u < lane_scalars->mul34_low);

    uint32_t vector_bias_pair[2];
    vector_bias_pair[0] = lane_scalars->vector_bias;
    vector_bias_pair[1] =
        lane_scalars->second_pair[1] * 0x6e21b4e2u +
        lane_scalars->second_pair[0] * 0xe2ceb689u +
        multiply_high_u32(lane_scalars->second_pair[0], 0x6e21b4e2u) +
        0x4712507fu +
        (uint32_t)(0x67c6045bu < lane_scalars->mul6e_low);

    if (out) {
        out->scale_pair[0] = scale_pair[0];
        out->scale_pair[1] = scale_pair[1];
        out->vector_bias_pair[0] = vector_bias_pair[0];
        out->vector_bias_pair[1] = vector_bias_pair[1];
    }

    for (unsigned j = 0; j < 12u; ++j) {
        uint32_t prod[2];
        pair64_multiply(seed_pairs13[j], scale_pair, prod);
        if (out) {
            out->product_pairs12[j][0] = prod[0];
            out->product_pairs12[j][1] = prod[1];
        }
        pair64_add(state_pairs26[lane_index + j], vector_bias_pair);
        pair64_add(state_pairs26[lane_index + j], prod);
    }

    return 0;
}

static const uint32_t k_f41a0870[16][2] = {
    {0x6bdfa0fau, 0x57b65c0eu}, {0x81b04d57u, 0xc187df92u},
    {0x37d51c9du, 0x32c87948u}, {0x4da5c8fau, 0xac99fcccu},
    {0x03ca9840u, 0x1dda9682u}, {0x199b449du, 0x87ac1a06u},
    {0xcfc013e3u, 0xf8ecb3bbu}, {0xe590c040u, 0x62be373fu},
    {0x9bb58f86u, 0xd3fed0f5u}, {0x51da5eccu, 0x453f6aabu},
    {0x67ab0b29u, 0xbf10ee2fu}, {0x1dcfda6fu, 0x205187e5u},
    {0x33a086ccu, 0x9a230b69u}, {0xe9c55612u, 0x0b63a51eu},
    {0xff96026fu, 0x753528a2u}, {0xb5bad1b5u, 0xe675c258u}
};

static const uint32_t k_f41a08f0[16][2] = {
    {0xac28c655u, 0xeabd27beu}, {0xde1bbf8cu, 0xf5b02b5eu},
    {0xb742530cu, 0x00ba44feu}, {0xe9354c43u, 0x1bad489eu},
    {0xc25bdfc3u, 0x26b7623eu}, {0xf44ed8fau, 0x31aa65deu},
    {0xcd756c7au, 0x4cb47f7eu}, {0xa69bfffau, 0x57be991eu},
    {0xd88ef931u, 0x62b19cbeu}, {0xb1b58cb1u, 0x7dbbb65eu},
    {0xe3a885e8u, 0x88aeb9feu}, {0xbccf1968u, 0x93b8d39eu},
    {0xeec2129fu, 0xaeabd73eu}, {0xc7e8a61fu, 0xb9b5f0deu},
    {0xa10f399fu, 0xc4c00a7eu}, {0xd30232d6u, 0xdfb30e1eu}
};

static void ladder64_run7_pair(const uint32_t tab[16][2], uint32_t *lo, uint32_t *hi) {
    uint32_t xlo = *lo;
    uint32_t xhi = *hi;
    for (unsigned i = 0; i < 7u; ++i) {
        unsigned nib = xlo & 0x0fu;
        uint32_t shifted = (xlo >> 4) | (xhi << 28);
        uint32_t tlo = tab[nib][0];
        uint32_t nlo = shifted + tlo;
        uint32_t nhi = (xhi >> 4) + tab[nib][1] + add_carry_u32(shifted, tlo);
        xlo = nlo;
        xhi = nhi;
    }
    *lo = xlo;
    *hi = xhi;
}

static uint32_t vector_reduction_ladder_mix(uint32_t lo, uint32_t hi) {
    uint32_t xlo = lo;
    uint32_t xhi = hi;
    for (unsigned i = 0; i < 7u; ++i) {
        unsigned nib = xlo & 0x0fu;
        uint32_t shifted = (xlo >> 4) | (xhi << 28);
        uint32_t tlo = k_f41a08f0[nib][0];
        uint32_t nlo = shifted + tlo;
        uint32_t nhi = (xhi >> 4) + k_f41a08f0[nib][1] + add_carry_u32(shifted, tlo);
        xlo = nlo;
        xhi = nhi;
    }
    unsigned nib = xlo & 0x0fu;
    uint32_t step8_low = ((xlo >> 4) | (xhi << 28)) + k_f41a08f0[nib][0];
    return (step8_low >> 4) + k_f41a08f0[step8_low & 0x0fu][0];
}

int l3_authorization_vector_reduce_lane_scalar_tail(
    const uint32_t seed_tail_pair[2],
    const l3_authorization_vector_reduce_lane_scalars *lane_scalars,
    unsigned lane_index,
    uint32_t state_pairs26[L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2],
    l3_authorization_vector_reduce_lane_tail_result *out) {
    if (!seed_tail_pair || !lane_scalars || !state_pairs26) {
        return -1;
    }
    if (lane_index + 12u >= L3_AUTH_VECTOR_REDUCE_STATE_PAIRS) {
        return -2;
    }

    uint32_t scale_pair[2];
    scale_pair[0] = lane_scalars->scale;
    scale_pair[1] =
        lane_scalars->second_pair[1] * 0x34b174bbu +
        lane_scalars->second_pair[0] * 0xe04d7355u +
        multiply_high_u32(lane_scalars->second_pair[0], 0x34b174bbu) +
        0xa20a0591u +
        (uint32_t)(0x13701239u < lane_scalars->mul34_low);

    uint32_t vector_bias_pair[2];
    vector_bias_pair[0] = lane_scalars->vector_bias;
    vector_bias_pair[1] =
        lane_scalars->second_pair[1] * 0x6e21b4e2u +
        lane_scalars->second_pair[0] * 0xe2ceb689u +
        multiply_high_u32(lane_scalars->second_pair[0], 0x6e21b4e2u) +
        0x4712507fu +
        (uint32_t)(0x67c6045bu < lane_scalars->mul6e_low);

    uint32_t tail_product[2];
    pair64_multiply(seed_tail_pair, scale_pair, tail_product);
    pair64_add(state_pairs26[lane_index + 12u], vector_bias_pair);
    pair64_add(state_pairs26[lane_index + 12u], tail_product);

    uint32_t cur_lo = state_pairs26[lane_index][0];
    uint32_t cur_hi = state_pairs26[lane_index][1];
    uint32_t next_lo_old = state_pairs26[lane_index + 1u][0];
    uint32_t next_hi_old = state_pairs26[lane_index + 1u][1];

    uint64_t p_cur = (uint64_t)cur_lo * 0x01f36b95ull;
    uint32_t p_cur_lo = (uint32_t)p_cur;
    uint32_t aff_lo = p_cur_lo + 0x726dafd7u;
    uint32_t aff_hi =
        cur_hi * 0x01f36b95u +
        cur_lo * 0xbb798481u +
        (uint32_t)(p_cur >> 32) +
        0x85f39a9eu +
        (uint32_t)(0x8d925028u < p_cur_lo);

    uint32_t lad87_lo = aff_lo;
    uint32_t lad87_hi = aff_hi;
    ladder64_run7_pair(k_f41a0870, &lad87_lo, &lad87_hi);

    uint64_t p87 = (uint64_t)lad87_lo * 0x980d777bull;
    uint32_t p87_lo = (uint32_t)p87;
    uint32_t post87_lo = p87_lo + 0x23b17266u;
    uint32_t post87_hi =
        lad87_hi * 0x980d777bu +
        lad87_lo * 0xce9f293eu +
        (uint32_t)(p87 >> 32) +
        0xc964863fu +
        (uint32_t)(0xdc4e8d99u < p87_lo);

    uint64_t p_post_f305 = (uint64_t)post87_lo * 0xf305d2d3ull;
    uint32_t p_post_f305_lo = (uint32_t)p_post_f305;
    uint32_t lad8_in_lo = p_post_f305_lo + 0xc6f9a9ceu;
    uint32_t lad8_in_hi =
        post87_hi * 0xf305d2d3u +
        post87_lo * 0xc9e3f163u +
        (uint32_t)(p_post_f305 >> 32) +
        0x841188fdu +
        (uint32_t)(0x39065631u < p_post_f305_lo);
    uint32_t lad8_mix = vector_reduction_ladder_mix(lad8_in_lo, lad8_in_hi);

    uint64_t p_post_e11 = (uint64_t)post87_lo * 0xe11c32d1ull;
    uint32_t p_post_e11_lo = (uint32_t)p_post_e11;
    uint64_t p_corr = (uint64_t)p_post_e11_lo * 0x91498d77ull;
    uint32_t corr_lo = (uint32_t)p_corr;
    uint32_t sum_next_lo = corr_lo + next_lo_old;
    uint32_t final_next_lo = sum_next_lo + 0x3b598f95u;
    uint32_t corr_inner =
        post87_hi * 0xe11c32d1u +
        post87_lo * 0x6279a855u +
        (uint32_t)(p_post_e11 >> 32) +
        lad8_mix * 0x3608ab50u;
    uint32_t final_next_hi =
        corr_inner * 0x91498d77u +
        p_post_e11_lo * 0x74ed5552u +
        (uint32_t)(p_corr >> 32) +
        next_hi_old +
        add_carry_u32(corr_lo, next_lo_old) +
        0x30192689u +
        (uint32_t)(0xc4a6706au < sum_next_lo);

    state_pairs26[lane_index + 1u][0] = final_next_lo;
    state_pairs26[lane_index + 1u][1] = final_next_hi;

    if (out) {
        out->current_affine_pair[0] = aff_lo;
        out->current_affine_pair[1] = aff_hi;
        out->ladder0870_pair[0] = lad87_lo;
        out->ladder0870_pair[1] = lad87_hi;
        out->post0870_pair[0] = post87_lo;
        out->post0870_pair[1] = post87_hi;
        out->ladder08f0_mix = lad8_mix;
        out->tail_product_pair[0] = tail_product[0];
        out->tail_product_pair[1] = tail_product[1];
        out->final_lane1_pair[0] = final_next_lo;
        out->final_lane1_pair[1] = final_next_hi;
        out->final_lane12_pair[0] = state_pairs26[lane_index + 12u][0];
        out->final_lane12_pair[1] = state_pairs26[lane_index + 12u][1];
    }

    return 0;
}

int l3_authorization_vector_reduce_run13_update(
    const int32_t param1_13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS],
    uint32_t param3,
    uint32_t param4,
    uint32_t state_pairs26[L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2],
    l3_authorization_vector_reduce_run13_trace *out) {
    if (!param1_13 || !state_pairs26) {
        return -1;
    }

    uint32_t seed_pairs13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS][2];
    int rc = l3_authorization_vector_reduce_seed_pairs13(param1_13, seed_pairs13);
    if (rc != 0) {
        return rc;
    }
    if (out) {
        for (unsigned i = 0; i < L3_AUTH_VECTOR_REDUCE_SEED_WORDS; ++i) {
            out->seed_pairs13[i][0] = seed_pairs13[i][0];
            out->seed_pairs13[i][1] = seed_pairs13[i][1];
        }
    }

    for (unsigned lane = 0; lane < L3_AUTH_VECTOR_REDUCE_SEED_WORDS; ++lane) {
        l3_authorization_vector_reduce_lane_scalars scalars;
        rc = l3_authorization_vector_reduce_lane_scalar_core(param3, param4, state_pairs26[lane], &scalars);
        if (rc != 0) {
            return rc;
        }
        if (out) {
            out->lane_scalars[lane] = scalars;
        }
        rc = l3_authorization_vector_reduce_vector_feedback_stores(seed_pairs13, &scalars, lane, state_pairs26, NULL);
        if (rc != 0) {
            return rc;
        }
        if (out) {
            for (unsigned i = 0; i < L3_AUTH_VECTOR_REDUCE_STATE_PAIRS; ++i) {
                out->vector_store_states13[lane][i][0] = state_pairs26[i][0];
                out->vector_store_states13[lane][i][1] = state_pairs26[i][1];
            }
        }
        rc = l3_authorization_vector_reduce_lane_scalar_tail(seed_pairs13[12], &scalars, lane, state_pairs26,
                                             out ? &out->lane_tails[lane] : NULL);
        if (rc != 0) {
            return rc;
        }
        if (out) {
            for (unsigned i = 0; i < L3_AUTH_VECTOR_REDUCE_STATE_PAIRS; ++i) {
                out->lane_done_states13[lane][i][0] = state_pairs26[i][0];
                out->lane_done_states13[lane][i][1] = state_pairs26[i][1];
            }
        }
    }

    return 0;
}

static const uint32_t k_f41a0970[16][2] = {
    {0x206d3c7cu, 0x6e73e44eu}, {0xf83b31dbu, 0x5ee53483u},
    {0xd009273au, 0x4f5684b9u}, {0xa7d71c99u, 0x3fc7d4efu},
    {0x7fa511f8u, 0x20392525u}, {0x57730757u, 0x10aa755bu},
    {0x2f40fcb6u, 0x011bc591u}, {0x070ef215u, 0xf18d15c7u},
    {0xdedce774u, 0xe1fe65fcu}, {0x39cb86e4u, 0xdb5ab2d5u},
    {0x11997c43u, 0xcbcc030bu}, {0xe96771a2u, 0xbc3d5340u},
    {0xc1356701u, 0xacaea376u}, {0x99035c60u, 0x9d1ff3acu},
    {0x70d151bfu, 0x8d9143e2u}, {0x489f471eu, 0x7e029418u}
};

static const uint32_t k_f4190450[8] = {
    0x743f9ec5u, 0x2dfeeb4du, 0xb440dcadu, 0xa5db3307u,
    0xf27f6667u, 0xc9304ecbu, 0xe19bbbbbu, 0x687e2f45u
};

static const uint32_t k_f4190470[8] = {
    0x04e1aec5u, 0xad8d35ecu, 0x810680d9u, 0x34f3cefeu,
    0x2a13aa41u, 0x339c8516u, 0x4a98d63eu, 0xfc03eab2u
};

static void vector_reduce_ladder0970_step(uint32_t *lo, uint32_t *hi) {
    uint32_t xlo = *lo;
    uint32_t xhi = *hi;
    unsigned nib = xlo & 0x0fu;
    uint32_t shifted = (xlo >> 4) | (xhi << 28);
    uint32_t tlo = k_f41a0970[nib][0];
    uint32_t nlo = tlo + shifted;
    uint32_t nhi = k_f41a0970[nib][1] + (xhi >> 4) + add_carry_u32(tlo, shifted);
    *lo = nlo;
    *hi = nhi;
}

int l3_authorization_vector_reduce_export_words13(
    const uint32_t state_pairs26[L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2],
    uint32_t out_words13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS],
    l3_authorization_vector_reduce_export_trace *out) {
    if (!state_pairs26 || !out_words13) {
        return -1;
    }

    uint32_t rolling_lo = 0xf28a9150u;
    uint32_t rolling_hi = 0x9b46bf3du; /* native iVar16 = -0x64b940c3 */

    for (unsigned lane = 0; lane < L3_AUTH_VECTOR_REDUCE_SEED_WORDS; ++lane) {
        const uint32_t cur_lo = state_pairs26[13u + lane][0];
        const uint32_t cur_hi = state_pairs26[13u + lane][1];

        uint64_t p_cur = (uint64_t)cur_lo * 0xf9d2115dull;
        uint32_t p_cur_lo = (uint32_t)p_cur;
        uint32_t p_roll_lo = multiply_low_u32(rolling_lo, 0xf60dfaefu);
        uint32_t sum0 = p_roll_lo + p_cur_lo;
        uint32_t base = sum0 + 0x1296dd2du;
        uint32_t p_base_1b = multiply_low_u32(base, 0x1ba5f1bfu);
        uint32_t ladder_lo = p_base_1b + 0x3d1fc816u;

        uint32_t ladder_hi =
            rolling_hi * 0xf60dfaefu +
            rolling_lo * 0x9295bd03u +
            multiply_high_u32(rolling_lo, 0xf60dfaefu) +
            cur_hi * 0xf9d2115du +
            cur_lo * 0x17ba4585u +
            (uint32_t)(p_cur >> 32) +
            add_carry_u32(p_roll_lo, p_cur_lo) +
            0x093bd807u +
            (uint32_t)(0xed6922d2u < sum0);
        ladder_hi =
            ladder_hi * 0x1ba5f1bfu +
            base * 0xc5be821eu +
            multiply_high_u32(base, 0x1ba5f1bfu) +
            0xafc290dcu +
            (uint32_t)(0xc2e037e9u < p_base_1b);

        for (unsigned i = 0; i < 7u; ++i) {
            vector_reduce_ladder0970_step(&ladder_lo, &ladder_hi);
        }

        uint32_t ladder7_lo = ladder_lo;
        uint32_t ladder7_hi = ladder_hi;
        uint32_t roll_mul_lo = multiply_low_u32(ladder7_lo, 0xa08b71b1u);
        uint32_t pre_export = base * 0x556ea36bu + ladder7_lo * 0xb0000000u + 0xff4f0ec3u;
        out_words13[lane] = pre_export * k_f4190450[lane & 7u] + k_f4190470[lane & 7u];

        for (unsigned i = 0; i < 7u; ++i) {
            vector_reduce_ladder0970_step(&ladder_lo, &ladder_hi);
        }

        unsigned nib = ladder_lo & 0x0fu;
        uint32_t shifted = (ladder_lo >> 4) | (ladder_hi << 28);
        uint32_t low_after15 = k_f41a0970[nib][0] + shifted;
        uint32_t final_low_lookup = k_f41a0970[low_after15 & 0x0fu][0];

        rolling_lo = roll_mul_lo + 0xd0ac33c7u;
        rolling_hi =
            ladder7_hi * 0xa08b71b1u +
            ladder7_lo * 0xa2556cbau +
            multiply_high_u32(ladder7_lo, 0xa08b71b1u) +
            (final_low_lookup + (low_after15 >> 4)) * 0xf748e4f0u +
            0x0cad2234u +
            (uint32_t)(0x2f53cc38u < roll_mul_lo);

        if (out) {
            out->export_inputs13[lane][0] = cur_lo;
            out->export_inputs13[lane][1] = cur_hi;
            out->pre_export_word13[lane] = pre_export;
            out->ladder7_pair13[lane][0] = ladder7_lo;
            out->ladder7_pair13[lane][1] = ladder7_hi;
            out->low_after15_13[lane] = low_after15;
            out->rolling_pair_after13[lane][0] = rolling_lo;
            out->rolling_pair_after13[lane][1] = rolling_hi;
        }
    }

    return 0;
}

int l3_authorization_vector_reduce_run13_update_export(
    const int32_t param1_13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS],
    uint32_t param3,
    uint32_t param4,
    uint32_t state_pairs26[L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2],
    uint32_t out_words13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS],
    l3_authorization_vector_reduce_full_export_trace *out) {
    if (!param1_13 || !state_pairs26 || !out_words13) {
        return -1;
    }
    int rc = l3_authorization_vector_reduce_run13_update(param1_13, param3, param4, state_pairs26,
                                          out ? &out->run13 : NULL);
    if (rc != 0) {
        return rc;
    }
    return l3_authorization_vector_reduce_export_words13(state_pairs26, out_words13,
                                          out ? &out->export_trace : NULL);
}
