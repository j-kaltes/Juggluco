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

/* ---- v319: bind FUN_f3fc3c70 local_514/local_548/local_57c outputs into V318 ---- */

static void mixer_core_copy_u32_to_i32_13(const uint32_t in_words13[13], int32_t out_words13[13]) {
    for (unsigned i = 0; i < 13u; ++i) out_words13[i] = (int32_t)in_words13[i];
}

int l3_authorization_round_bind_primary_outputs(
    const int32_t *param1_words,
    uint32_t round_index,
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[13],
    const uint32_t param4_param5_words13[13],
    const uint32_t param4_staging_words13[13],
    const uint32_t affine_words13[13],
    const uint32_t param4_mix_words26[26],
    const uint32_t local3a0_words26[26],
    const int32_t seventh_vector_reduce_param1_13[13],
    uint32_t seventh_vector_reduce_param3,
    uint32_t seventh_vector_reduce_param4,
    const int32_t eighth_local5e0_words13[13],
    const int32_t eighth_param3_words13[13],
    const int32_t eighth_vector_reduce_param1_13[13],
    uint32_t eighth_vector_reduce_param3,
    uint32_t eighth_vector_reduce_param4,
    const int32_t secondary_mix_local5b0_words13[13],
    const int32_t secondary_mix_local5e4_words13[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_round_primary_trace *trace) {
    if (!param1_words || !live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !sixth_words13 || !param4_param5_words13 || !param4_staging_words13 ||
        !affine_words13 || !param4_mix_words26 || !local3a0_words26 ||
        !seventh_vector_reduce_param1_13 || !eighth_local5e0_words13 ||
        !eighth_param3_words13 || !eighth_vector_reduce_param1_13 ||
        !secondary_mix_local5b0_words13 || !secondary_mix_local5e4_words13 ||
        !param5_local4c0_halves2 || !param5_local4d0_halves2 || !param5_auvar98_halves2 ||
        !param6_high_seed_halves2 || !param6_low_seed_halves2 || !param6_middle_low_seed_halves2 ||
        !param7_high_seed_halves2 || !param7_low_seed_halves2 || !param7_middle_low_seed_halves2 ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    static l3_authorization_round_primary_trace tmp;
    l3_authorization_round_primary_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_primary_post_sixth_terminal_words13(
        live_q6_words4, live_q5_words4, live_q4_words4,
        sixth_words13, param4_param5_words13, param4_staging_words13,
        affine_words13, param4_mix_words26, local3a0_words26,
        seventh_vector_reduce_param1_13, seventh_vector_reduce_param3, seventh_vector_reduce_param4,
        eighth_local5e0_words13, eighth_param3_words13, eighth_vector_reduce_param1_13,
        eighth_vector_reduce_param3, eighth_vector_reduce_param4,
        dst->local514_words13, dst->local548_words13, dst->f3fc3c70_eighth_words13,
        dst->local57c_words13, &dst->f3fc3c70_trace);
    if (rc != 0) return rc;

    int32_t local514_i32[13];
    int32_t local548_i32[13];
    mixer_core_copy_u32_to_i32_13(dst->local514_words13, local514_i32);
    mixer_core_copy_u32_to_i32_13(dst->local548_words13, local548_i32);

    rc = l3_authorization_export_round_outputs(
        param1_words, round_index,
        local514_i32, secondary_mix_local5b0_words13,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        local548_i32, local548_i32,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        secondary_mix_local5e4_words13, secondary_mix_local5e4_words13,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->live_ctx_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}

/* ---- v320: bind FUN_f3fcc2d0 terminal local_5e4/local_618 outputs into V319 ---- */

static void mixer_core_extract_fcc2d0_seed13(const int32_t *param1_words,
                                                 int32_t out_seed13[13]) {
    const uint32_t word_base = 0x90u / 4u;
    for (unsigned i = 0; i < 13u; ++i) out_seed13[i] = param1_words[word_base + i];
}

static uint32_t mixer_core_extract_ctx_u32(const int32_t *param1_words,
                                               uint32_t byte_offset) {
    return (uint32_t)param1_words[byte_offset / 4u];
}

int l3_authorization_round_bind_secondary_outputs(
    const int32_t *param1_words,
    uint32_t round_index,
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[13],
    const uint32_t param4_param5_words13[13],
    const uint32_t param4_staging_words13[13],
    const uint32_t affine_words13[13],
    const uint32_t param4_mix_words26[26],
    const uint32_t local3a0_words26[26],
    const int32_t seventh_vector_reduce_param1_13[13],
    uint32_t seventh_vector_reduce_param3,
    uint32_t seventh_vector_reduce_param4,
    const int32_t eighth_local5e0_words13[13],
    const int32_t eighth_param3_words13[13],
    const int32_t eighth_vector_reduce_param1_13[13],
    uint32_t eighth_vector_reduce_param3,
    uint32_t eighth_vector_reduce_param4,
    const int32_t secondary_mix_local5b0_words13[13],
    const uint32_t secondary_mix_local4c8_words26[26],
    const int32_t secondary_mix_param4_194_words26[26],
    const uint32_t secondary_mix_local530_words26[26],
    const uint32_t secondary_mix_carry_seed_words26[26],
    const int32_t secondary_mix_param3_words13[13],
    const uint32_t secondary_mix_local390_words13[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_round_secondary_trace *trace) {
    if (!param1_words || !live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !sixth_words13 || !param4_param5_words13 || !param4_staging_words13 ||
        !affine_words13 || !param4_mix_words26 || !local3a0_words26 ||
        !seventh_vector_reduce_param1_13 || !eighth_local5e0_words13 ||
        !eighth_param3_words13 || !eighth_vector_reduce_param1_13 ||
        !secondary_mix_local5b0_words13 || !secondary_mix_local4c8_words26 ||
        !secondary_mix_param4_194_words26 || !secondary_mix_local530_words26 ||
        !secondary_mix_carry_seed_words26 || !secondary_mix_param3_words13 ||
        !secondary_mix_local390_words13 || !param5_local4c0_halves2 ||
        !param5_local4d0_halves2 || !param5_auvar98_halves2 ||
        !param6_high_seed_halves2 || !param6_low_seed_halves2 ||
        !param6_middle_low_seed_halves2 || !param7_high_seed_halves2 ||
        !param7_low_seed_halves2 || !param7_middle_low_seed_halves2 ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    static l3_authorization_round_secondary_trace tmp;
    l3_authorization_round_secondary_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    int32_t fcc_seed13[13];
    mixer_core_extract_fcc2d0_seed13(param1_words, fcc_seed13);
    const uint32_t fcc_scalar0 = mixer_core_extract_ctx_u32(param1_words, 0x268u);
    const uint32_t fcc_scalar1 = mixer_core_extract_ctx_u32(param1_words, 0x26cu);

    int rc = l3_authorization_secondary_build_terminal_outputs(
        secondary_mix_local4c8_words26,
        secondary_mix_param4_194_words26,
        secondary_mix_local530_words26,
        secondary_mix_carry_seed_words26,
        secondary_mix_param3_words13,
        secondary_mix_local390_words13,
        fcc_seed13,
        fcc_scalar0,
        fcc_scalar1,
        dst->secondary_mix_local5e4_words13,
        dst->secondary_mix_local618_words13,
        &dst->secondary_mix_terminal_trace);
    if (rc != 0) return rc;

    int32_t local5e4_i32[13];
    mixer_core_copy_u32_to_i32_13(dst->secondary_mix_local5e4_words13, local5e4_i32);

    rc = l3_authorization_round_bind_primary_outputs(
        param1_words, round_index,
        live_q6_words4, live_q5_words4, live_q4_words4,
        sixth_words13, param4_param5_words13, param4_staging_words13,
        affine_words13, param4_mix_words26, local3a0_words26,
        seventh_vector_reduce_param1_13, seventh_vector_reduce_param3, seventh_vector_reduce_param4,
        eighth_local5e0_words13, eighth_param3_words13, eighth_vector_reduce_param1_13,
        eighth_vector_reduce_param3, eighth_vector_reduce_param4,
        secondary_mix_local5b0_words13, local5e4_i32,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->bound_fc3c70_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}

/* ---- v322: bind FUN_f3fcc2d0 param_7/local_5b0 export into V320 ---- */


/* ---- v323: consume computed FUN_f3fcc2d0 local_460 into local_4c8 before V320 ---- */


/* ---- v324: compute FUN_f3fcc2d0 local_530 before V323 live terminal path ---- */


/* ---- v325: compute FUN_f3fcc2d0 V321 vector preimages before live terminal path ---- */

int l3_authorization_round_build_secondary_preimage(
    const int32_t *param1_words,
    uint32_t round_index,
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[13],
    const uint32_t param4_param5_words13[13],
    const uint32_t param4_staging_words13[13],
    const uint32_t affine_words13[13],
    const uint32_t param4_mix_words26[26],
    const uint32_t local3a0_words26[26],
    const int32_t seventh_vector_reduce_param1_13[13],
    uint32_t seventh_vector_reduce_param3,
    uint32_t seventh_vector_reduce_param4,
    const int32_t eighth_local5e0_words13[13],
    const int32_t eighth_param3_words13[13],
    const int32_t eighth_vector_reduce_param1_13[13],
    uint32_t eighth_vector_reduce_param3,
    uint32_t eighth_vector_reduce_param4,
    const uint32_t secondary_mix_sixth_secondary_vector_reduce_words13[13],
    const uint32_t secondary_mix_local3c4_words13[13],
    const int32_t secondary_mix_param2_words13[13],
    const uint32_t secondary_mix_local42c_words13[13],
    const int32_t secondary_mix_param4_194_words26[26],
    const uint32_t secondary_mix_carry_seed_words26[26],
    const int32_t secondary_mix_param3_words13[13],
    const uint32_t secondary_mix_local390_words13[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_round_secondary_preimage_trace *trace) {
    if (!param1_words || !live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !sixth_words13 || !param4_param5_words13 || !param4_staging_words13 ||
        !affine_words13 || !param4_mix_words26 || !local3a0_words26 ||
        !seventh_vector_reduce_param1_13 || !eighth_local5e0_words13 ||
        !eighth_param3_words13 || !eighth_vector_reduce_param1_13 ||
        !secondary_mix_sixth_secondary_vector_reduce_words13 || !secondary_mix_local3c4_words13 ||
        !secondary_mix_param2_words13 || !secondary_mix_local42c_words13 ||
        !secondary_mix_param4_194_words26 || !secondary_mix_carry_seed_words26 ||
        !secondary_mix_param3_words13 || !secondary_mix_local390_words13 ||
        !param5_local4c0_halves2 || !param5_local4d0_halves2 || !param5_auvar98_halves2 ||
        !param6_high_seed_halves2 || !param6_low_seed_halves2 || !param6_middle_low_seed_halves2 ||
        !param7_high_seed_halves2 || !param7_low_seed_halves2 || !param7_middle_low_seed_halves2 ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    static l3_authorization_round_secondary_preimage_trace tmp;
    l3_authorization_round_secondary_preimage_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    int32_t fcc_seed13[13];
    mixer_core_extract_fcc2d0_seed13(param1_words, fcc_seed13);
    const uint32_t fcc_scalar0 = mixer_core_extract_ctx_u32(param1_words, 0x268u);
    const uint32_t fcc_scalar1 = mixer_core_extract_ctx_u32(param1_words, 0x26cu);

    int rc = l3_authorization_secondary_vector_preimage_param7_local460_words13(
        secondary_mix_sixth_secondary_vector_reduce_words13,
        secondary_mix_local3c4_words13,
        secondary_mix_local42c_words13,
        param1_words,
        fcc_seed13,
        fcc_scalar0,
        fcc_scalar1,
        dst->secondary_mix_local5b0_words13,
        dst->secondary_mix_local460_words13,
        &dst->secondary_mix_preimage_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_secondary_local3c4_local460_accum_convolution26_materialize4c8(
        secondary_mix_local3c4_words13,
        dst->secondary_mix_local460_words13,
        dst->secondary_mix_local4c8_words26,
        &dst->secondary_mix_local4c8_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_secondary_param2_local42c_accum_convolution26_materialize530(
        secondary_mix_param2_words13,
        secondary_mix_local42c_words13,
        dst->secondary_mix_local530_words26,
        &dst->secondary_mix_local530_trace);
    if (rc != 0) return rc;

    int32_t local5b0_i32[13];
    mixer_core_copy_u32_to_i32_13(dst->secondary_mix_local5b0_words13, local5b0_i32);

    rc = l3_authorization_round_bind_secondary_outputs(
        param1_words, round_index,
        live_q6_words4, live_q5_words4, live_q4_words4,
        sixth_words13, param4_param5_words13, param4_staging_words13,
        affine_words13, param4_mix_words26, local3a0_words26,
        seventh_vector_reduce_param1_13, seventh_vector_reduce_param3, seventh_vector_reduce_param4,
        eighth_local5e0_words13, eighth_param3_words13, eighth_vector_reduce_param1_13,
        eighth_vector_reduce_param3, eighth_vector_reduce_param4,
        local5b0_i32,
        dst->secondary_mix_local4c8_words26,
        secondary_mix_param4_194_words26,
        dst->secondary_mix_local530_words26,
        secondary_mix_carry_seed_words26,
        secondary_mix_param3_words13,
        secondary_mix_local390_words13,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->bound_secondary_mix_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}

/* ---- v326: consume carried FUN_f3fcc2d0 local_618 through post-param_7 accumulator ---- */

int l3_authorization_round_fold_secondary_tail(
    const int32_t *param1_words,
    uint32_t round_index,
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[13],
    const uint32_t param4_param5_words13[13],
    const uint32_t param4_staging_words13[13],
    const uint32_t affine_words13[13],
    const uint32_t param4_mix_words26[26],
    const uint32_t local3a0_words26[26],
    const int32_t seventh_vector_reduce_param1_13[13],
    uint32_t seventh_vector_reduce_param3,
    uint32_t seventh_vector_reduce_param4,
    const int32_t eighth_local5e0_words13[13],
    const int32_t eighth_param3_words13[13],
    const int32_t eighth_vector_reduce_param1_13[13],
    uint32_t eighth_vector_reduce_param3,
    uint32_t eighth_vector_reduce_param4,
    const uint32_t secondary_mix_sixth_secondary_vector_reduce_words13[13],
    const uint32_t secondary_mix_local3c4_words13[13],
    const int32_t secondary_mix_param2_words13[13],
    const uint32_t secondary_mix_local42c_words13[13],
    const int32_t secondary_mix_param4_194_words26[26],
    const uint32_t secondary_mix_carry_seed_words26[26],
    const int32_t secondary_mix_param3_words13[13],
    const uint32_t secondary_mix_local390_words13[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_round_tail_convolution_trace *trace) {
    if (!param1_words || !live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !sixth_words13 || !param4_param5_words13 || !param4_staging_words13 ||
        !affine_words13 || !param4_mix_words26 || !local3a0_words26 ||
        !seventh_vector_reduce_param1_13 || !eighth_local5e0_words13 ||
        !eighth_param3_words13 || !eighth_vector_reduce_param1_13 ||
        !secondary_mix_sixth_secondary_vector_reduce_words13 || !secondary_mix_local3c4_words13 ||
        !secondary_mix_param2_words13 || !secondary_mix_local42c_words13 ||
        !secondary_mix_param4_194_words26 || !secondary_mix_carry_seed_words26 ||
        !secondary_mix_param3_words13 || !secondary_mix_local390_words13 ||
        !param5_local4c0_halves2 || !param5_local4d0_halves2 || !param5_auvar98_halves2 ||
        !param6_high_seed_halves2 || !param6_low_seed_halves2 || !param6_middle_low_seed_halves2 ||
        !param7_high_seed_halves2 || !param7_low_seed_halves2 || !param7_middle_low_seed_halves2 ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    static l3_authorization_round_tail_convolution_trace tmp;
    l3_authorization_round_tail_convolution_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_round_build_secondary_preimage(
        param1_words, round_index,
        live_q6_words4, live_q5_words4, live_q4_words4,
        sixth_words13, param4_param5_words13, param4_staging_words13,
        affine_words13, param4_mix_words26, local3a0_words26,
        seventh_vector_reduce_param1_13, seventh_vector_reduce_param3, seventh_vector_reduce_param4,
        eighth_local5e0_words13, eighth_param3_words13, eighth_vector_reduce_param1_13,
        eighth_vector_reduce_param3, eighth_vector_reduce_param4,
        secondary_mix_sixth_secondary_vector_reduce_words13,
        secondary_mix_local3c4_words13,
        secondary_mix_param2_words13,
        secondary_mix_local42c_words13,
        secondary_mix_param4_194_words26,
        secondary_mix_carry_seed_words26,
        secondary_mix_param3_words13,
        secondary_mix_local390_words13,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->bound_preimage_trace);
    if (rc != 0) return rc;

    memcpy(dst->secondary_mix_local618_words13,
           dst->bound_preimage_trace.bound_secondary_mix_trace.secondary_mix_local618_words13,
           sizeof(dst->secondary_mix_local618_words13));
    memcpy(dst->round_ec8_words13,
           dst->bound_preimage_trace.bound_secondary_mix_trace.bound_fc3c70_trace.live_ctx_trace.live_round_trace.round_ec8_words13,
           sizeof(dst->round_ec8_words13));

    int32_t local618_i32[13];
    mixer_core_copy_u32_to_i32_13(dst->secondary_mix_local618_words13, local618_i32);
    rc = l3_authorization_stage4_stage_accumulate_and_convolve26(
        local618_i32,
        dst->round_ec8_words13,
        dst->mixer_core_post_local618_pairs26,
        &dst->post_local618_convolution_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}

/* ---- v327: bind post-local_618 materialized local_478 into following FUN_f3fdd33c param_7 ---- */

int l3_authorization_round_apply_tail_feedback(
    const int32_t *param1_words,
    uint32_t round_index,
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[13],
    const uint32_t param4_param5_words13[13],
    const uint32_t param4_staging_words13[13],
    const uint32_t affine_words13[13],
    const uint32_t param4_mix_words26[26],
    const uint32_t local3a0_words26[26],
    const int32_t seventh_vector_reduce_param1_13[13],
    uint32_t seventh_vector_reduce_param3,
    uint32_t seventh_vector_reduce_param4,
    const int32_t eighth_local5e0_words13[13],
    const int32_t eighth_param3_words13[13],
    const int32_t eighth_vector_reduce_param1_13[13],
    uint32_t eighth_vector_reduce_param3,
    uint32_t eighth_vector_reduce_param4,
    const uint32_t secondary_mix_sixth_secondary_vector_reduce_words13[13],
    const uint32_t secondary_mix_local3c4_words13[13],
    const int32_t secondary_mix_param2_words13[13],
    const uint32_t secondary_mix_local42c_words13[13],
    const int32_t secondary_mix_param4_194_words26[26],
    const uint32_t secondary_mix_carry_seed_words26[26],
    const int32_t secondary_mix_param3_words13[13],
    const uint32_t secondary_mix_local390_words13[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_round_tail_feedback_trace *trace) {
    if (!param1_words || !live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !sixth_words13 || !param4_param5_words13 || !param4_staging_words13 ||
        !affine_words13 || !param4_mix_words26 || !local3a0_words26 ||
        !seventh_vector_reduce_param1_13 || !eighth_local5e0_words13 ||
        !eighth_param3_words13 || !eighth_vector_reduce_param1_13 ||
        !secondary_mix_sixth_secondary_vector_reduce_words13 || !secondary_mix_local3c4_words13 ||
        !secondary_mix_param2_words13 || !secondary_mix_local42c_words13 ||
        !secondary_mix_param4_194_words26 || !secondary_mix_carry_seed_words26 ||
        !secondary_mix_param3_words13 || !secondary_mix_local390_words13 ||
        !param5_local4c0_halves2 || !param5_local4d0_halves2 || !param5_auvar98_halves2 ||
        !param6_high_seed_halves2 || !param6_low_seed_halves2 || !param6_middle_low_seed_halves2 ||
        !param7_high_seed_halves2 || !param7_low_seed_halves2 || !param7_middle_low_seed_halves2 ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    static l3_authorization_round_tail_feedback_trace tmp;
    l3_authorization_round_tail_feedback_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_round_fold_secondary_tail(
        param1_words, round_index,
        live_q6_words4, live_q5_words4, live_q4_words4,
        sixth_words13, param4_param5_words13, param4_staging_words13,
        affine_words13, param4_mix_words26, local3a0_words26,
        seventh_vector_reduce_param1_13, seventh_vector_reduce_param3, seventh_vector_reduce_param4,
        eighth_local5e0_words13, eighth_param3_words13, eighth_vector_reduce_param1_13,
        eighth_vector_reduce_param3, eighth_vector_reduce_param4,
        secondary_mix_sixth_secondary_vector_reduce_words13,
        secondary_mix_local3c4_words13,
        secondary_mix_param2_words13,
        secondary_mix_local42c_words13,
        secondary_mix_param4_194_words26,
        secondary_mix_carry_seed_words26,
        secondary_mix_param3_words13,
        secondary_mix_local390_words13,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->bound_local618_trace);
    if (rc != 0) return rc;

    memcpy(dst->pre_param7_words13, out_param7_words13,
           sizeof(dst->pre_param7_words13));

    const l3_authorization_word_pair *param7_high_pairs26 =
        dst->bound_local618_trace.bound_preimage_trace.bound_secondary_mix_trace.bound_fc3c70_trace.live_ctx_trace.live_round_trace.bound_high_trace.param7_high_pairs26;
    memcpy(dst->param7_high_pairs26, param7_high_pairs26, sizeof(dst->param7_high_pairs26));

    rc = l3_authorization_stage3_stage_vector_reduction13(
        dst->bound_local618_trace.mixer_core_post_local618_pairs26,
        dst->param7_high_pairs26,
        param7_high_seed_halves2,
        param7_low_seed_halves2,
        param7_middle_low_seed_halves2,
        dst->post_local618_local478_halves13,
        &dst->post_local618_scalar0,
        &dst->post_local618_vector_trace);
    if (rc != 0) return rc;

    int32_t vector_feedback_seed13[13];
    mixer_core_extract_fcc2d0_seed13(param1_words, vector_feedback_seed13);
    const uint32_t native_param2_unused = mixer_core_extract_ctx_u32(param1_words, 0x268u);
    const uint32_t scalar1_unused = mixer_core_extract_ctx_u32(param1_words, 0x26cu);

    rc = l3_authorization_apply_feedback_output_c(
        vector_feedback_seed13,
        native_param2_unused,
        dst->post_local618_scalar0,
        scalar1_unused,
        dst->post_local618_local478_halves13,
        out_param7_words13,
        &dst->post_local618_param7_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}

/* ---- v329: bind V328 local_3f8/sixth/local_42c preimage producer into V327 ---- */

int l3_authorization_round_build_tail_preimage(
    const int32_t *param1_words,
    uint32_t round_index,
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[13],
    const uint32_t param4_param5_words13[13],
    const uint32_t param4_staging_words13[13],
    const uint32_t affine_words13[13],
    const uint32_t param4_mix_words26[26],
    const uint32_t local3a0_words26[26],
    const int32_t seventh_vector_reduce_param1_13[13],
    uint32_t seventh_vector_reduce_param3,
    uint32_t seventh_vector_reduce_param4,
    const int32_t eighth_local5e0_words13[13],
    const int32_t eighth_param3_words13[13],
    const int32_t eighth_vector_reduce_param1_13[13],
    uint32_t eighth_vector_reduce_param3,
    uint32_t eighth_vector_reduce_param4,
    const uint32_t secondary_mix_local3c4_words13[13],
    const int32_t secondary_mix_param2_words13[13],
    const int32_t secondary_mix_param4_194_words26[26],
    const uint32_t secondary_mix_carry_seed_words26[26],
    const int32_t secondary_mix_param3_words13[13],
    const uint32_t secondary_mix_local390_words13[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_round_tail_preimage_trace *trace) {
    if (!param1_words || !live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !sixth_words13 || !param4_param5_words13 || !param4_staging_words13 ||
        !affine_words13 || !param4_mix_words26 || !local3a0_words26 ||
        !seventh_vector_reduce_param1_13 || !eighth_local5e0_words13 ||
        !eighth_param3_words13 || !eighth_vector_reduce_param1_13 ||
        !secondary_mix_local3c4_words13 || !secondary_mix_param2_words13 ||
        !secondary_mix_param4_194_words26 || !secondary_mix_carry_seed_words26 ||
        !secondary_mix_param3_words13 || !secondary_mix_local390_words13 ||
        !param5_local4c0_halves2 || !param5_local4d0_halves2 || !param5_auvar98_halves2 ||
        !param6_high_seed_halves2 || !param6_low_seed_halves2 || !param6_middle_low_seed_halves2 ||
        !param7_high_seed_halves2 || !param7_low_seed_halves2 || !param7_middle_low_seed_halves2 ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    static l3_authorization_round_tail_preimage_trace tmp;
    l3_authorization_round_tail_preimage_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_primary_post_sixth_terminal_words13(
        live_q6_words4, live_q5_words4, live_q4_words4,
        sixth_words13, param4_param5_words13, param4_staging_words13,
        affine_words13, param4_mix_words26, local3a0_words26,
        seventh_vector_reduce_param1_13, seventh_vector_reduce_param3, seventh_vector_reduce_param4,
        eighth_local5e0_words13, eighth_param3_words13, eighth_vector_reduce_param1_13,
        eighth_vector_reduce_param3, eighth_vector_reduce_param4,
        dst->f3fc3c70_local514_words13,
        dst->f3fc3c70_local548_words13,
        dst->f3fc3c70_eighth_words13,
        dst->f3fc3c70_local57c_words13,
        &dst->f3fc3c70_param1_trace);
    if (rc != 0) return rc;

    mixer_core_extract_fcc2d0_seed13(param1_words, dst->secondary_mix_seed_words13);
    dst->secondary_mix_scalar0 = mixer_core_extract_ctx_u32(param1_words, 0x268u);
    dst->secondary_mix_scalar1 = mixer_core_extract_ctx_u32(param1_words, 0x26cu);

    rc = l3_authorization_secondary_derive_state_stage3(
        secondary_mix_local390_words13,
        dst->secondary_mix_seed_words13,
        dst->secondary_mix_scalar0,
        dst->secondary_mix_scalar1,
        dst->secondary_mix_local3f8_words13,
        &dst->secondary_mix_local3f8_trace);
    if (rc != 0) return rc;

    int32_t secondary_mix_param1_local514_i32[13];
    for (unsigned i = 0; i < 13u; ++i) {
        secondary_mix_param1_local514_i32[i] = (int32_t)dst->f3fc3c70_local514_words13[i];
    }

    rc = l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_param7_local460_words13(
        dst->secondary_mix_local3f8_words13,
        secondary_mix_local390_words13,
        param1_words,
        secondary_mix_param1_local514_i32,
        dst->secondary_mix_seed_words13,
        dst->secondary_mix_scalar0,
        dst->secondary_mix_scalar1,
        secondary_mix_local3c4_words13,
        dst->secondary_mix_local42c_words13,
        dst->secondary_mix_sixth_secondary_vector_reduce_words13,
        dst->secondary_mix_local5b0_words13,
        dst->secondary_mix_local460_words13,
        &dst->secondary_mix_v328_preimage_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_round_apply_tail_feedback(
        param1_words, round_index,
        live_q6_words4, live_q5_words4, live_q4_words4,
        sixth_words13, param4_param5_words13, param4_staging_words13,
        affine_words13, param4_mix_words26, local3a0_words26,
        seventh_vector_reduce_param1_13, seventh_vector_reduce_param3, seventh_vector_reduce_param4,
        eighth_local5e0_words13, eighth_param3_words13, eighth_vector_reduce_param1_13,
        eighth_vector_reduce_param3, eighth_vector_reduce_param4,
        dst->secondary_mix_sixth_secondary_vector_reduce_words13,
        secondary_mix_local3c4_words13,
        secondary_mix_param2_words13,
        dst->secondary_mix_local42c_words13,
        secondary_mix_param4_194_words26,
        secondary_mix_carry_seed_words26,
        secondary_mix_param3_words13,
        secondary_mix_local390_words13,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->bound_v327_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}


/* ---- v330: derive FUN_f3fcc2d0 local_3c4 before the V329 binding ---- */

int l3_authorization_round_resolve_tail_state(
    const int32_t *param1_words,
    uint32_t round_index,
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[13],
    const uint32_t param4_param5_words13[13],
    const uint32_t param4_staging_words13[13],
    const uint32_t affine_words13[13],
    const uint32_t param4_mix_words26[26],
    const uint32_t local3a0_words26[26],
    const int32_t seventh_vector_reduce_param1_13[13],
    uint32_t seventh_vector_reduce_param3,
    uint32_t seventh_vector_reduce_param4,
    const int32_t eighth_local5e0_words13[13],
    const int32_t eighth_param3_words13[13],
    const int32_t eighth_vector_reduce_param1_13[13],
    uint32_t eighth_vector_reduce_param3,
    uint32_t eighth_vector_reduce_param4,
    const int32_t secondary_mix_param2_words13[13],
    const int32_t secondary_mix_param4_194_words26[26],
    const uint32_t secondary_mix_carry_seed_words26[26],
    const int32_t secondary_mix_param3_words13[13],
    const uint32_t secondary_mix_local390_words13[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_round_tail_state_trace *trace) {
    if (!param1_words || !live_q6_words4 || !live_q5_words4 || !live_q4_words4 ||
        !sixth_words13 || !param4_param5_words13 || !param4_staging_words13 ||
        !affine_words13 || !param4_mix_words26 || !local3a0_words26 ||
        !seventh_vector_reduce_param1_13 || !eighth_local5e0_words13 ||
        !eighth_param3_words13 || !eighth_vector_reduce_param1_13 ||
        !secondary_mix_param2_words13 || !secondary_mix_param4_194_words26 ||
        !secondary_mix_carry_seed_words26 || !secondary_mix_param3_words13 ||
        !secondary_mix_local390_words13 || !param5_local4c0_halves2 ||
        !param5_local4d0_halves2 || !param5_auvar98_halves2 ||
        !param6_high_seed_halves2 || !param6_low_seed_halves2 ||
        !param6_middle_low_seed_halves2 || !param7_high_seed_halves2 ||
        !param7_low_seed_halves2 || !param7_middle_low_seed_halves2 ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    static l3_authorization_round_tail_state_trace tmp;
    l3_authorization_round_tail_state_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    int32_t fcc_seed13[13];
    mixer_core_extract_fcc2d0_seed13(param1_words, fcc_seed13);
    const uint32_t fcc_scalar0 = mixer_core_extract_ctx_u32(param1_words, 0x268u);
    const uint32_t fcc_scalar1 = mixer_core_extract_ctx_u32(param1_words, 0x26cu);

    int rc = l3_authorization_secondary_param3_bound_secondary_vector_reduce_affine_words13(
        secondary_mix_param3_words13,
        fcc_seed13,
        fcc_scalar0,
        fcc_scalar1,
        dst->secondary_mix_affine_words13,
        NULL);
    if (rc != 0) return rc;

    for (unsigned i = 0; i < 13u; ++i) {
        dst->secondary_mix_param4_c4_words13[i] = param1_words[(0xc4u / 4u) + i];
    }

    rc = l3_authorization_secondary_derive_state_stage2(
        secondary_mix_param3_words13,
        dst->secondary_mix_affine_words13,
        fcc_seed13,
        fcc_scalar0,
        fcc_scalar1,
        secondary_mix_local390_words13,
        secondary_mix_param2_words13,
        dst->secondary_mix_param4_c4_words13,
        dst->secondary_mix_local3c4_words13,
        &dst->secondary_mix_local3c4_trace);
    if (rc != 0) return rc;

    rc = l3_authorization_round_build_tail_preimage(
        param1_words, round_index,
        live_q6_words4, live_q5_words4, live_q4_words4,
        sixth_words13, param4_param5_words13, param4_staging_words13,
        affine_words13, param4_mix_words26, local3a0_words26,
        seventh_vector_reduce_param1_13, seventh_vector_reduce_param3, seventh_vector_reduce_param4,
        eighth_local5e0_words13, eighth_param3_words13, eighth_vector_reduce_param1_13,
        eighth_vector_reduce_param3, eighth_vector_reduce_param4,
        dst->secondary_mix_local3c4_words13,
        secondary_mix_param2_words13,
        secondary_mix_param4_194_words26,
        secondary_mix_carry_seed_words26,
        secondary_mix_param3_words13,
        secondary_mix_local390_words13,
        param5_local4c0_halves2, param5_local4d0_halves2, param5_auvar98_halves2,
        param6_high_seed_halves2, param6_low_seed_halves2, param6_middle_low_seed_halves2,
        param7_high_seed_halves2, param7_low_seed_halves2, param7_middle_low_seed_halves2,
        out_param5_words13, out_param6_words13, out_param7_words13,
        &dst->bound_v329_trace);
    if (rc != 0) return rc;

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}

/* ---- v331: native loop-tail next-round context handoff after V330 ---- */

static void extract_round_window13(const int32_t *param1_words,
                                                  uint32_t round_index,
                                                  uint32_t byte_base,
                                                  int32_t out_words13[13]) {
    const uint32_t word_base = (byte_base + round_index * 0x34u) / 4u;
    for (unsigned i = 0; i < 13u; ++i) out_words13[i] = param1_words[word_base + i];
}

int l3_authorization_round_finish(
    const int32_t *param1_words,
    uint32_t round_index,
    const uint32_t live_q6_words4[4],
    const uint32_t live_q5_words4[4],
    const uint32_t live_q4_words4[4],
    const uint32_t sixth_words13[13],
    const uint32_t param4_param5_words13[13],
    const uint32_t param4_staging_words13[13],
    const uint32_t affine_words13[13],
    const uint32_t param4_mix_words26[26],
    const uint32_t local3a0_words26[26],
    const int32_t seventh_vector_reduce_param1_13[13],
    uint32_t seventh_vector_reduce_param3,
    uint32_t seventh_vector_reduce_param4,
    const int32_t eighth_local5e0_words13[13],
    const int32_t eighth_param3_words13[13],
    const int32_t eighth_vector_reduce_param1_13[13],
    uint32_t eighth_vector_reduce_param3,
    uint32_t eighth_vector_reduce_param4,
    const int32_t secondary_mix_param2_words13[13],
    const int32_t secondary_mix_param4_194_words26[26],
    const uint32_t secondary_mix_carry_seed_words26[26],
    const int32_t secondary_mix_param3_words13[13],
    const uint32_t secondary_mix_local390_words13[13],
    const uint64_t param5_local4c0_halves2[2],
    const uint64_t param5_local4d0_halves2[2],
    const uint64_t param5_auvar98_halves2[2],
    const uint64_t param6_high_seed_halves2[2],
    const uint64_t param6_low_seed_halves2[2],
    const uint64_t param6_middle_low_seed_halves2[2],
    const uint64_t param7_high_seed_halves2[2],
    const uint64_t param7_low_seed_halves2[2],
    const uint64_t param7_middle_low_seed_halves2[2],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_round_finish_trace *trace) {
    if (!param1_words || !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    static l3_authorization_round_finish_trace tmp;
    l3_authorization_round_finish_trace *dst = trace ? trace : &tmp;
    memset(dst, 0, sizeof(*dst));

    int rc = l3_authorization_round_resolve_tail_state(
        param1_words,
        round_index,
        live_q6_words4,
        live_q5_words4,
        live_q4_words4,
        sixth_words13,
        param4_param5_words13,
        param4_staging_words13,
        affine_words13,
        param4_mix_words26,
        local3a0_words26,
        seventh_vector_reduce_param1_13,
        seventh_vector_reduce_param3,
        seventh_vector_reduce_param4,
        eighth_local5e0_words13,
        eighth_param3_words13,
        eighth_vector_reduce_param1_13,
        eighth_vector_reduce_param3,
        eighth_vector_reduce_param4,
        secondary_mix_param2_words13,
        secondary_mix_param4_194_words26,
        secondary_mix_carry_seed_words26,
        secondary_mix_param3_words13,
        secondary_mix_local390_words13,
        param5_local4c0_halves2,
        param5_local4d0_halves2,
        param5_auvar98_halves2,
        param6_high_seed_halves2,
        param6_low_seed_halves2,
        param6_middle_low_seed_halves2,
        param7_high_seed_halves2,
        param7_low_seed_halves2,
        param7_middle_low_seed_halves2,
        out_param5_words13,
        out_param6_words13,
        out_param7_words13,
        &dst->bound_v330_trace);
    if (rc != 0) return rc;

    dst->round_index = round_index;
    dst->has_next_round = (round_index != 0u) ? 1u : 0u;
    dst->next_round_index = dst->has_next_round ? (round_index - 1u) : 0u;

    extract_round_window13(param1_words, round_index, 0x2ccu,
                                         dst->current_round_2cc_words13);
    extract_round_window13(param1_words, round_index, 0xec8u,
                                         dst->current_round_ec8_words13);
    if (dst->has_next_round) {
        extract_round_window13(param1_words, dst->next_round_index, 0x2ccu,
                                             dst->next_round_2cc_words13);
        extract_round_window13(param1_words, dst->next_round_index, 0xec8u,
                                             dst->next_round_ec8_words13);
    }

    memcpy(dst->param5_words13, out_param5_words13, sizeof(dst->param5_words13));
    memcpy(dst->param6_words13, out_param6_words13, sizeof(dst->param6_words13));
    memcpy(dst->param7_words13, out_param7_words13, sizeof(dst->param7_words13));
    return 0;
}

/* ---- v332: V331 loop-tail to next FUN_f3fc3c70 live-q boundary ---- */


/* ---- v333: derive next FUN_f3fc3c70 param_4/eighth inputs from context ---- */





/* ---- v334: derive next FUN_f3fc3c70 sixth export and affine companion ---- */


/* ---- v335: derive next fifth/sixth seed windows from V331 loop context ---- */


/* ---- v336: derive next local_3a0 image from native +0x194 context slice ---- */


/* ---- v337: bind V336 next FUN_f3fc3c70 terminal into following FUN_f3fcc2d0 preimage ---- */



/* ---- v338: feed V337 next FUN_f3fcc2d0 preimage into the next terminal round ---- */


/* ---- v339: derive V338 second-round post-preimage context/carry images ---- */



/* ---- v340: derive following FUN_f3fcc2d0 param_2/param_3 from next FUN_f3fc3c70 ---- */


/* ---- v341: derive following FUN_f3fcc2d0 local_390 through V272 mix390 ---- */


/* ---- v342: drive the derived second round through the V331 loop-tail wrapper ---- */


/* ---- v343: feed V342 second-tail handoff into third FUN_f3fc3c70 boundary ---- */


/* ---- v344: bind V343 third FUN_f3fc3c70 terminal into following FUN_f3fcc2d0 preimage ---- */



/* ---- v345: feed V344 following FUN_f3fcc2d0 preimage into the third terminal path ---- */



/* ---- v346: derive V345 third post-preimage context/carry images ---- */


/* ---- v347: drive the completed third round through the V331 loop-tail wrapper ---- */


/* ---- v348: feed V347 third-tail handoff into fourth FUN_f3fc3c70 boundary ---- */


/* ---- v349: bind V348 fourth FUN_f3fc3c70 terminal into following FUN_f3fcc2d0 preimage ---- */



/* ---- v350: feed V349 following FUN_f3fcc2d0 preimage into the fourth terminal path ---- */



/* ---- v351: derive V350 fourth post-preimage context/carry images ---- */



/* ---- v352: drive the completed fourth round through the V331 loop-tail wrapper ---- */


/* ---- v353: feed V352 fourth-tail handoff into fifth FUN_f3fc3c70 boundary ---- */


/* ---- v354: bind V353 fifth FUN_f3fc3c70 terminal into following FUN_f3fcc2d0 preimage ---- */


/* ---- v355: feed V354 following FUN_f3fcc2d0 preimage into the fifth terminal path ---- */


/* ---- v356: derive V355 fifth post-preimage context/carry images ---- */


/* ---- v357: re-enter completed V356 fifth terminal through V331 loop tail ---- */


/* ---- v358: feed V357 fifth-tail handoff into sixth FUN_f3fc3c70 boundary ---- */


/* ---- v359: bind V358 sixth FUN_f3fc3c70 terminal into following FUN_f3fcc2d0 preimage ---- */


/* ---- v360: feed V359 following FUN_f3fcc2d0 preimage into the sixth terminal path ---- */



/* ---- v361: derive V360 sixth post-preimage context/carry images ---- */


/* ---- v362: re-enter V361 sixth terminal through V331 loop-tail wrapper ---- */


/* ---- v363: feed V362 sixth-tail handoff into seventh FUN_f3fc3c70 boundary ---- */



/* ---- v364: bind V363 seventh FUN_f3fc3c70 terminal into following FUN_f3fcc2d0 preimage ---- */


/* ---- v365: feed V364 seventh FUN_f3fcc2d0 preimage into seventh terminal ---- */


/* ---- v366: derive V365 seventh post-preimage ctx/carry images ---- */



/* ---- v367: re-enter V366 seventh terminal through V331 loop-tail wrapper ---- */


/* ---- v368: feed V367 seventh-tail handoff into eighth FUN_f3fc3c70 boundary ---- */


/* ---- v369: bind V368 eighth FUN_f3fc3c70 terminal into following FUN_f3fcc2d0 preimage ---- */


/* ---- v370: feed V369 eighth FUN_f3fcc2d0 preimage into eighth terminal ---- */


/* ---- v371: derive V370 eighth post-preimage ctx/carry images ---- */



/* ---- v372: re-enter V371 eighth terminal through V331 loop-tail wrapper ---- */

/* ---- v373: feed V372 eighth-tail handoff into ninth FUN_f3fc3c70 boundary ---- */




/* ---- v377: bind V373 ninth FUN_f3fc3c70 terminal into following FUN_f3fcc2d0 preimage ---- */



/* ---- v378: V377 ninth preimage into terminal path ---- */


/* ---- v379: derive V378 ninth post-preimage ctx/carry images ---- */


/* ---- v380: re-enter V379 ninth terminal through V331 loop-tail wrapper ---- */


/* ---- v381: feed V380 ninth-tail handoff into tenth FUN_f3fc3c70 boundary ---- */


/* ---- v382: bind V381 tenth FUN_f3fc3c70 output into following FUN_f3fcc2d0 preimage ---- */


/* ---- v383: V382 tenth preimage into terminal path ---- */



/* ---- v384: derive V383 tenth terminal context/carry internally ---- */


/* ---- v385: re-enter V384 tenth terminal through V331 loop-tail wrapper ---- */



/* ---- v386: feed V385 tenth-tail handoff into eleventh FUN_f3fc3c70 boundary ---- */


/* ---- v387: bind V386 eleventh FUN_f3fc3c70 output into following FUN_f3fcc2d0 preimage ---- */



/* ---- v388: V387 eleventh preimage into terminal path ---- */



/* ---- v389: derive V388 eleventh terminal context/carry internally ---- */



/* ---- v390: re-enter V389 eleventh terminal through V331 loop-tail wrapper ---- */


/* ---- v391: feed V390 eleventh-tail handoff into twelfth FUN_f3fc3c70 boundary ---- */


/* ---- v409: bind V391 twelfth FUN_f3fc3c70 output into following FUN_f3fcc2d0 preimage ---- */



/* ---- v410: V409 twelfth preimage into terminal path ---- */


/* ---- v411: derive V410 twelfth terminal context/carry internally ---- */


/* ---- v412: re-enter V411 twelfth terminal through V331 loop-tail wrapper ---- */



/* ---- v413: V412 twelfth tail to thirteenth FUN_f3fc3c70 boundary ---- */


/* ---- v414: V413 thirteenth FUN_f3fc3c70 to following thirteenth FUN_f3fcc2d0 preimage ---- */


/* ---- v415: V414 thirteenth preimage into terminal path with internally-derived ctx/carry ---- */



/* ---- v416: re-enter V415 thirteenth terminal through V331 loop-tail wrapper ---- */


/* ---- v417: V416 thirteenth tail to fourteenth FUN_f3fc3c70 boundary ---- */


/* ---- v418: V417 fourteenth FUN_f3fc3c70 to following fourteenth FUN_f3fcc2d0 preimage ---- */



/* ---- v419: V418 fourteenth preimage into terminal path with internally-derived ctx/carry ---- */



/* ---- v420: re-enter V419 fourteenth terminal through V331 loop-tail wrapper ---- */



/* ---- v421: V420 fourteenth tail to fifteenth FUN_f3fc3c70 boundary ---- */



/* ---- v422: V421 fifteenth FUN_f3fc3c70 to following fifteenth FUN_f3fcc2d0 preimage ---- */


/* ---- v423: V422 fifteenth preimage into terminal path with internally-derived ctx/carry ---- */




/* ---- v424: V423 fifteenth terminal re-enters V331 loop-tail handoff ---- */



/* ---- v425: V424 fifteenth tail to sixteenth FUN_f3fc3c70 boundary ---- */


/* ---- v426: V425 sixteenth FUN_f3fc3c70 to following sixteenth FUN_f3fcc2d0 preimage ---- */




/* ---- v427: V426 sixteenth preimage into terminal path with internally-derived ctx/carry ---- */



/* ---- v428: V427 sixteenth terminal re-enters V331 loop-tail handoff ---- */





/* ---- v429: V428 sixteenth tail to seventeenth FUN_f3fc3c70 boundary ---- */



/* ---- v430: V429 seventeenth FUN_f3fc3c70 to following seventeenth FUN_f3fcc2d0 preimage ---- */






/* ---- v431: V430 seventeenth preimage into terminal path with internally-derived ctx/carry ---- */



/* ---- v432: V431 seventeenth terminal re-enters V331 loop-tail handoff ---- */




/* ---- v433: V432 seventeenth tail to eighteenth FUN_f3fc3c70 boundary ---- */




/* ---- v434: V433 eighteenth FUN_f3fc3c70 to following eighteenth FUN_f3fcc2d0 preimage ---- */


/* ---- v435: V434 eighteenth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */




/* ---- v436: V435 eighteenth terminal re-enters V331 loop-tail handoff ---- */


/* ---- v437: V436 eighteenth tail to nineteenth FUN_f3fc3c70 boundary ---- */


/* ---- v438: V437 nineteenth FUN_f3fc3c70 to following nineteenth FUN_f3fcc2d0 preimage ---- */





/* ---- v439: V438 nineteenth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */



/* ---- v440: V439 nineteenth terminal re-enters V331 loop-tail handoff ---- */


/* ---- v441: V440 nineteenth tail to twentieth FUN_f3fc3c70 boundary ---- */




/* ---- v442: V441 twentieth FUN_f3fc3c70 to following twentieth FUN_f3fcc2d0 preimage ---- */


/* ---- v443: V442 twentieth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */



/* ---- v444: V443 twentieth terminal re-enters V331 loop-tail handoff ---- */


/* ---- v445: V444 twentieth tail to twenty-first FUN_f3fc3c70 boundary ---- */


/* ---- v446: V445 twentyfirst FUN_f3fc3c70 to following twentyfirst FUN_f3fcc2d0 preimage ---- */





/* ---- v447: V446 twenty-first FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */



/* ---- v448: V447 twenty-first terminal re-enters V331 loop-tail handoff ---- */


/* ---- v449: V448 twenty-first tail to twenty-second FUN_f3fc3c70 boundary ---- */




/* ---- v450: V445 twentysecond FUN_f3fc3c70 to following twentysecond FUN_f3fcc2d0 preimage ---- */






/* ---- v451: V446 twenty-first FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */





/* ---- v452: V451 twenty-second terminal re-enters V331 loop-tail handoff ---- */



/* ---- v453: V444 twentysecond tail to twenty-first FUN_f3fc3c70 boundary ---- */


/* ---- v454: V453 twenty-third FUN_f3fc3c70 to following twenty-third FUN_f3fcc2d0 preimage ---- */





/* ---- v455: V454 twenty-third FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v456: V455 twenty-third terminal re-enters V331 loop-tail handoff ---- */










/* ---- v457: V456 twentythird tail to twenty-first FUN_f3fc3c70 boundary ---- */



/* ---- v458: V457 twenty-fourth FUN_f3fc3c70 to following twenty-fourth FUN_f3fcc2d0 preimage ---- */


/* ---- v459: V458 twenty-fourth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v461: V460 twenty-fourth tail to twenty-fifth FUN_f3fc3c70 boundary ---- */






/* ---- v462: V461 twenty-fifth FUN_f3fc3c70 to following twenty-fifth FUN_f3fcc2d0 preimage ---- */





/* ---- v463: V462 twenty-fifth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */




/* ---- v464: V463 twenty-fifth terminal to V331 tail/next-round handoff ---- */


/* ---- v474: V464 twenty-fifth tail to twenty-sixth FUN_f3fc3c70 boundary ---- */


/* ---- v475: V474 twenty-sixth FUN_f3fc3c70 to following twenty-sixth FUN_f3fcc2d0 preimage ---- */



/* ---- v476: V475 twenty-sixth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */



/* ---- v477: V476 twenty-sixth terminal to V331 tail/next-round handoff ---- */



/* ---- v478: V477 twenty-sixth tail to twenty-seventh FUN_f3fc3c70 boundary ---- */


/* ---- v479: V478 twenty-seventh FUN_f3fc3c70 to following twenty-seventh FUN_f3fcc2d0 preimage ---- */


/* ---- v480: V479 twenty-seventh FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v481: V480 twenty-seventh terminal to V331 tail/next-round handoff ---- */


/* ---- v482: V481 twenty-seventh tail to twenty-eighth FUN_f3fc3c70 boundary ---- */


/* ---- v483: V482 twenty-eighth FUN_f3fc3c70 to following twenty-eighth FUN_f3fcc2d0 preimage ---- */


/* ---- v484: V483 twenty-eighth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v485: V484 twenty-eighth terminal to V331 tail/next-round handoff ---- */


/* ---- v486: V485 twenty-eighth tail to twenty-ninth FUN_f3fc3c70 boundary ---- */


/* ---- v487: V486 twenty-ninth FUN_f3fc3c70 to following twenty-ninth FUN_f3fcc2d0 preimage ---- */


/* ---- v488: V487 twenty-ninth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v489: V488 twenty-ninth terminal to V331 tail/next-round handoff ---- */


/* ---- v490: V489 twenty-ninth tail to thirtieth FUN_f3fc3c70 boundary ---- */


/* ---- v491: V490 thirtieth FUN_f3fc3c70 to following thirtieth FUN_f3fcc2d0 preimage ---- */


/* ---- v492: V491 thirtieth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v493: V492 thirtieth terminal to V331 tail/next-round handoff ---- */


/* ---- v494: V493 thirtieth tail to thirtyfirst FUN_f3fc3c70 boundary ---- */


/* ---- v495: V494 thirty-first FUN_f3fc3c70 to following thirty-first FUN_f3fcc2d0 preimage ---- */


/* ---- v496: V495 thirty-first FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v497: V496 thirtyfirst terminal to V331 tail/next-round handoff ---- */


/* ---- v498: V497 thirtyfirst tail to thirtysecond FUN_f3fc3c70 boundary ---- */



/* ---- v499: V498 thirty-second FUN_f3fc3c70 to following thirty-second FUN_f3fcc2d0 preimage ---- */



/* ---- v500: V499 thirty-second FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v501: V500 thirtysecond terminal to V331 tail/next-round handoff ---- */


/* ---- v502: V501 thirtysecond tail to thirtythird FUN_f3fc3c70 boundary ---- */


/* ---- v503: V502 thirty-third FUN_f3fc3c70 to following thirty-third FUN_f3fcc2d0 preimage ---- */



/* ---- v504: V503 thirty-third FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v505: V504 thirtythird terminal to V331 tail/next-round handoff ---- */


/* ---- v506: V505 thirtythird tail to thirtyfourth FUN_f3fc3c70 boundary ---- */



/* ---- v507: V506 thirty-fourth FUN_f3fc3c70 to following thirty-fourth FUN_f3fcc2d0 preimage ---- */



/* ---- v508: V507 thirty-fourth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v509: V508 thirtyfourth terminal to V331 tail/next-round handoff ---- */


/* ---- v510: V509 thirtyfourth tail to thirtyfifth FUN_f3fc3c70 boundary ---- */



/* ---- v511: V510 thirty-fifth FUN_f3fc3c70 to following thirty-fifth FUN_f3fcc2d0 preimage ---- */



/* ---- v512: V511 thirty-fifth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v513: V512 thirtyfifth terminal to V331 tail/next-round handoff ---- */


/* ---- v514: V513 thirtyfifth tail to thirtysixth FUN_f3fc3c70 boundary ---- */



/* ---- v515: V514 thirty-sixth FUN_f3fc3c70 to following thirty-sixth FUN_f3fcc2d0 preimage ---- */



/* ---- v516: V515 thirty-sixth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v517: V516 thirtysixth terminal to V331 tail/next-round handoff ---- */


/* ---- v518: V517 thirtysixth tail to thirtyseventh FUN_f3fc3c70 boundary ---- */


/* ---- v519: V518 thirty-seventh FUN_f3fc3c70 to following thirty-seventh FUN_f3fcc2d0 preimage ---- */



/* ---- v520: V519 thirty-seventh FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v521: V520 thirtyseventh terminal to V331 tail/next-round handoff ---- */


/* ---- v522: V521 thirtyseventh tail to thirtyeighth FUN_f3fc3c70 boundary ---- */


/* ---- v523: V522 thirty-eighth FUN_f3fc3c70 to following thirty-eighth FUN_f3fcc2d0 preimage ---- */



/* ---- v524: V523 thirty-eighth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v525: V524 thirtyeighth terminal to V331 tail/next-round handoff ---- */


/* ---- v526: V525 thirtyeighth tail to thirtyninth FUN_f3fc3c70 boundary ---- */


/* ---- v527: V526 thirty-ninth FUN_f3fc3c70 to following thirty-ninth FUN_f3fcc2d0 preimage ---- */



/* ---- v528: V527 thirty-ninth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v529: V528 thirtyninth terminal to V331 tail/next-round handoff ---- */


/* ---- v530: V529 thirtyninth tail to fortieth FUN_f3fc3c70 boundary ---- */


/* ---- v531: V530 fortieth FUN_f3fc3c70 to following fortieth FUN_f3fcc2d0 preimage ---- */



/* ---- v532: V531 fortieth FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v533: V532 fortieth terminal to V331 tail/next-round handoff ---- */


/* ---- v534: V533 fortieth tail to fortyfirst FUN_f3fc3c70 boundary ---- */


/* ---- v535: V534 fortyfirst FUN_f3fc3c70 to following fortyfirst FUN_f3fcc2d0 preimage ---- */



/* ---- v536: V535 fortyfirst FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v537: V536 fortyfirst terminal to V331 tail/next-round handoff ---- */


/* ---- v538: V537 fortyfirst tail to fortysecond FUN_f3fc3c70 boundary ---- */

/* ---- v539: V538 fortysecond FUN_f3fc3c70 to following fortysecond FUN_f3fcc2d0 preimage ---- */



/* ---- v540: V539 fortysecond FUN_f3fcc2d0 preimage to terminal with derived ctx/carry ---- */


/* ---- v541: V540 forty-second terminal to V331 tail/next-round handoff ---- */

