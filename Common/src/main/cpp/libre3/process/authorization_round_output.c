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
#include "authorization_round_internal.h"

#include <string.h>

/* ---- v315: compose V303/V310/V313 vector materializers with v314 terminal exports ---- */

int l3_authorization_reduce_and_export_round_outputs(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar1_unused,
    const l3_authorization_word_pair param5_high_pairs26[26],
    const int32_t param5_local5b0[13],
    const int32_t param5_round_words13[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const l3_authorization_word_pair param6_high_pairs26[26],
    const int32_t param6_local548[13],
    const int32_t param6_round_mixed13[13],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const l3_authorization_word_pair param7_high_pairs26[26],
    const int32_t param7_local5e4[13],
    const int32_t param7_mixed_round13[13],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_reduced_outputs_trace *trace) {
    if (!seed_words13 || !param5_high_pairs26 || !param5_local5b0 ||
        !param5_round_words13 || !param5_local4c0_halves2 || !param5_local4d0_halves2 ||
        !param5_auvar98_halves2 || !param6_high_pairs26 || !param6_local548 ||
        !param6_round_mixed13 || !param6_high_seed_halves2 || !param6_low_seed_halves2 ||
        !param6_middle_low_seed_halves2 || !param7_high_pairs26 || !param7_local5e4 ||
        !param7_mixed_round13 || !param7_high_seed_halves2 || !param7_low_seed_halves2 ||
        !param7_middle_low_seed_halves2 || !out_param5_words13 || !out_param6_words13 ||
        !out_param7_words13) {
        return -1;
    }

    l3_authorization_reduced_outputs_trace tmp;
    l3_authorization_reduced_outputs_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_stage1_stage_convolve_and_reduce_vectors13(
        param5_high_pairs26, param5_local5b0, param5_round_words13,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        dst->param5_local478_halves13, &dst->param5_scalar0,
        &dst->param5_low_conv_trace, &dst->param5_vector_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_stage2_stage_convolve_and_reduce_vectors13(
        param6_high_pairs26, param6_local548, param6_round_mixed13,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        dst->param6_local478_halves13, &dst->param6_scalar0,
        &dst->param6_low_conv_trace, &dst->param6_vector_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_stage3_stage_convolve_and_reduce_vectors13(
        param7_high_pairs26, param7_local5e4, param7_mixed_round13,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        dst->param7_local478_halves13, &dst->param7_scalar0,
        &dst->param7_low_conv_trace, &dst->param7_vector_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_apply_feedback_outputs(
        seed_words13, native_param2_unused, scalar1_unused,
        dst->param5_local478_halves13, dst->param5_scalar0,
        dst->param6_local478_halves13, dst->param6_scalar0,
        dst->param7_local478_halves13, dst->param7_scalar0,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->terminal_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}


/* ---- v316: bind high accumulator/convolution images into V315 terminal path ---- */

int l3_authorization_build_and_export_round_outputs(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar1_unused,
    const int32_t param5_local514[13],
    const int32_t param5_round2630[13],
    const int32_t param5_local5b0[13],
    const int32_t param5_round27b0[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const int32_t param6_high_local548[13],
    const int32_t param6_high_round2b30[13],
    const int32_t param6_low_local548[13],
    const int32_t param6_low_round2b30[13],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const int32_t param7_high_local5e4[13],
    const int32_t param7_high_round2c30[13],
    const int32_t param7_low_local5e4[13],
    const int32_t param7_low_round2c30[13],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_convolved_outputs_trace *trace) {
    if (!seed_words13 || !param5_local514 || !param5_round2630 ||
        !param5_local5b0 || !param5_round27b0 || !param5_local4c0_halves2 ||
        !param5_local4d0_halves2 || !param5_auvar98_halves2 ||
        !param6_high_local548 || !param6_high_round2b30 || !param6_low_local548 ||
        !param6_low_round2b30 || !param6_high_seed_halves2 || !param6_low_seed_halves2 ||
        !param6_middle_low_seed_halves2 || !param7_high_local5e4 ||
        !param7_high_round2c30 || !param7_low_local5e4 || !param7_low_round2c30 ||
        !param7_high_seed_halves2 || !param7_low_seed_halves2 ||
        !param7_middle_low_seed_halves2 || !out_param5_words13 ||
        !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    l3_authorization_convolved_outputs_trace tmp;
    l3_authorization_convolved_outputs_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_core_post_secondary_mix_accum_convolution26(
        param5_local514, param5_round2630,
        dst->param5_high_pairs26, &dst->param5_high_conv_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_stage2_stage_accumulate_and_convolve26(
        param6_high_local548, param6_high_round2b30,
        dst->param6_high_pairs26, &dst->param6_high_conv_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_stage3_stage_accumulate_and_convolve26(
        param7_high_local5e4, param7_high_round2c30,
        dst->param7_high_pairs26, &dst->param7_high_conv_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_reduce_and_export_round_outputs(
        seed_words13, native_param2_unused, scalar1_unused,
        dst->param5_high_pairs26, param5_local5b0, param5_round27b0,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        dst->param6_high_pairs26, param6_low_local548, param6_low_round2b30,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        dst->param7_high_pairs26, param7_low_local5e4, param7_low_round2c30,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->terminal_bound_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}

/* ---- v317: bind V316 to live FUN_f3fbc5c0 round-context windows ---- */

static void mixer_core_extract_round_window13(const int32_t *param1_words,
                                             uint32_t round_index,
                                             uint32_t byte_offset,
                                             int32_t out_words13[13]) {
    const uint32_t word_base = (round_index * 0x34u + byte_offset) / 4u;
    for (unsigned i = 0; i < 13u; ++i) out_words13[i] = param1_words[word_base + i];
}

int l3_authorization_export_round_outputs_from_context(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar1_unused,
    const int32_t *param1_words,
    uint32_t round_index,
    const int32_t param5_local514[13],
    const int32_t param5_local5b0[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const int32_t param6_high_local548[13],
    const int32_t param6_low_local548[13],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const int32_t param7_high_local5e4[13],
    const int32_t param7_low_local5e4[13],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_context_outputs_trace *trace) {
    if (!seed_words13 || !param1_words || !param5_local514 || !param5_local5b0 ||
        !param5_local4c0_halves2 || !param5_local4d0_halves2 || !param5_auvar98_halves2 ||
        !param6_high_local548 || !param6_low_local548 || !param6_high_seed_halves2 ||
        !param6_low_seed_halves2 || !param6_middle_low_seed_halves2 ||
        !param7_high_local5e4 || !param7_low_local5e4 || !param7_high_seed_halves2 ||
        !param7_low_seed_halves2 || !param7_middle_low_seed_halves2 ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    l3_authorization_context_outputs_trace tmp;
    l3_authorization_context_outputs_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));
    dst->round_index = round_index;

    mixer_core_extract_round_window13(param1_words, round_index, 0x2ccu, dst->round_2cc_words13);
    mixer_core_extract_round_window13(param1_words, round_index, 0xec8u, dst->round_ec8_words13);

    int rc = l3_authorization_build_and_export_round_outputs(
        seed_words13, native_param2_unused, scalar1_unused,
        param5_local514, dst->round_2cc_words13,
        param5_local5b0, dst->round_ec8_words13,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        param6_high_local548, dst->round_2cc_words13,
        param6_low_local548, dst->round_2cc_words13,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        param7_high_local5e4, dst->round_ec8_words13,
        param7_low_local5e4, dst->round_ec8_words13,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->bound_high_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}

/* ---- v318: bind V317 to live FUN_f3fbc5c0 seed/scalar context slots ---- */

static void mixer_core_extract_context_seed13(const int32_t *param1_words,
                                             int32_t out_seed13[13]) {
    const uint32_t word_base = 0x90u / 4u;
    for (unsigned i = 0; i < 13u; ++i) out_seed13[i] = param1_words[word_base + i];
}

int l3_authorization_export_round_outputs(
    const int32_t *param1_words,
    uint32_t round_index,
    const int32_t param5_local514[13],
    const int32_t param5_local5b0[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const int32_t param6_high_local548[13],
    const int32_t param6_low_local548[13],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const int32_t param7_high_local5e4[13],
    const int32_t param7_low_local5e4[13],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_live_outputs_trace *trace) {
    if (!param1_words || !param5_local514 || !param5_local5b0 ||
        !param5_local4c0_halves2 || !param5_local4d0_halves2 || !param5_auvar98_halves2 ||
        !param6_high_local548 || !param6_low_local548 || !param6_high_seed_halves2 ||
        !param6_low_seed_halves2 || !param6_middle_low_seed_halves2 ||
        !param7_high_local5e4 || !param7_low_local5e4 || !param7_high_seed_halves2 ||
        !param7_low_seed_halves2 || !param7_middle_low_seed_halves2 ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    l3_authorization_live_outputs_trace tmp;
    l3_authorization_live_outputs_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));
    dst->round_index = round_index;

    mixer_core_extract_context_seed13(param1_words, dst->seed_words13);
    dst->native_param2_unused = (uint32_t)param1_words[0x268u / 4u];
    dst->scalar1_unused = (uint32_t)param1_words[0x26cu / 4u];

    int rc = l3_authorization_export_round_outputs_from_context(
        dst->seed_words13, dst->native_param2_unused, dst->scalar1_unused,
        param1_words, round_index,
        param5_local514, param5_local5b0,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        param6_high_local548, param6_low_local548,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        param7_high_local5e4, param7_low_local5e4,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->live_round_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}
