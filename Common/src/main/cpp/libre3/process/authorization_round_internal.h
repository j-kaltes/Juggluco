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
#ifndef L3_AUTHORIZATION_ROUND_INTERNAL_H
#define L3_AUTHORIZATION_ROUND_INTERNAL_H

#include <stdint.h>

#include "authorization_round_mixer.h"
#include "authorization_round_primary.h"
#include "authorization_vector_feedback.h"
#include "authorization_round_secondary.h"



typedef struct {
    uint64_t input_local478_halves13[13][2];
    l3_authorization_vector_feedback_pair32 state_before26[26];
    l3_authorization_vector_feedback_pair32 state_after26[26];
    uint32_t native_param2_unused;
    uint32_t vector_feedback_words13[13];
    uint32_t param5_words13[13];
    l3_authorization_vector_feedback_run13_update_export_trace vector_feedback_trace;
} l3_authorization_feedback_output_a_trace;

/* v307: bind the v303 local_478/local_3b8 vector stream into the completed
 * FUN_f3fdd33c seed/update/export helper, then apply the immediate native
 * FUN_f3fbc5c0 param_5 affine export materialization.
 *
 * native_param2_unused corresponds to the second FUN_f3fdd33c argument at this
 * call-site.  FUN_f3fdd33c does not consume it; it is accepted here so the
 * adapter-facing signature mirrors the native call boundary exactly.
 */
int l3_authorization_apply_feedback_output_a(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar0,
    uint32_t scalar1,
    const uint64_t local478_halves13[13][2],
    uint32_t out_param5_words13[13],
    l3_authorization_feedback_output_a_trace *trace);

int l3_authorization_materialize_round_output_a(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param5_words13[13]);


typedef struct {
    uint64_t input_local478_halves13[13][2];
    l3_authorization_vector_feedback_pair32 state_before26[26];
    l3_authorization_vector_feedback_pair32 state_after26[26];
    uint32_t native_param2_unused;
    uint32_t vector_feedback_words13[13];
    uint32_t param6_words13[13];
    l3_authorization_vector_feedback_run13_update_export_trace vector_feedback_trace;
} l3_authorization_feedback_output_b_trace;

/* Bind a local_478-style 13-vector stream through the same FUN_f3fdd33c
 * helper, then apply the second native export materialization into param_6. */
int l3_authorization_apply_feedback_output_b(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar0,
    uint32_t scalar1,
    const uint64_t local478_halves13[13][2],
    uint32_t out_param6_words13[13],
    l3_authorization_feedback_output_b_trace *trace);

int l3_authorization_materialize_round_output_b(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param6_words13[13]);


typedef struct {
    uint64_t input_local478_halves13[13][2];
    l3_authorization_vector_feedback_pair32 state_before26[26];
    l3_authorization_vector_feedback_pair32 state_after26[26];
    uint32_t native_param2_unused;
    uint32_t vector_feedback_words13[13];
    uint32_t param7_words13[13];
    l3_authorization_vector_feedback_run13_update_export_trace vector_feedback_trace;
} l3_authorization_feedback_output_c_trace;

/* v313: bind a local_478-style 13-vector stream through FUN_f3fdd33c,
 * then apply the third native export materialization into caller param_7. */
int l3_authorization_apply_feedback_output_c(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar0,
    uint32_t scalar1,
    const uint64_t local478_halves13[13][2],
    uint32_t out_param7_words13[13],
    l3_authorization_feedback_output_c_trace *trace);

int l3_authorization_materialize_round_output_c(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param7_words13[13]);


typedef struct {
    l3_authorization_feedback_output_a_trace param5_trace;
    l3_authorization_feedback_output_b_trace param6_trace;
    l3_authorization_feedback_output_c_trace param7_trace;
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
    uint32_t scalar0_param5;
    uint32_t scalar0_param6;
    uint32_t scalar0_param7;
    uint32_t native_param2_unused;
    uint32_t scalar1_unused;
} l3_authorization_feedback_outputs_trace;

/* v314: adapter-facing terminal wrapper for the three completed
 * FUN_f3fdd33c call-sites in the FUN_f3fbc5c0 loop.  The caller supplies the
 * three already materialized local_478-style vector streams and the three
 * native scalar0 arguments prepared by the vector materializers.  The helper
 * runs the completed FUN_f3fdd33c seed/update/export path for each stream and
 * applies the immediate native param_5/param_6/param_7 affine exports.
 */
int l3_authorization_apply_feedback_outputs(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar1_unused,
    const uint64_t param5_local478_halves13[13][2],
    uint32_t param5_scalar0,
    const uint64_t param6_local478_halves13[13][2],
    uint32_t param6_scalar0,
    const uint64_t param7_local478_halves13[13][2],
    uint32_t param7_scalar0,
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13],
    l3_authorization_feedback_outputs_trace *trace);


typedef struct {
    l3_authorization_stage1_stage_convolution_trace param5_low_conv_trace;
    l3_authorization_stage1_stage_vector_reduction_trace param5_vector_trace;
    l3_authorization_stage2_stage_convolution_trace param6_low_conv_trace;
    l3_authorization_stage2_stage_vector_reduction_trace param6_vector_trace;
    l3_authorization_stage3_stage_convolution_trace param7_low_conv_trace;
    l3_authorization_stage3_stage_vector_reduction_trace param7_vector_trace;
    l3_authorization_feedback_outputs_trace terminal_trace;
    uint64_t param5_local478_halves13[13][2];
    uint64_t param6_local478_halves13[13][2];
    uint64_t param7_local478_halves13[13][2];
    uint32_t param5_scalar0;
    uint32_t param6_scalar0;
    uint32_t param7_scalar0;
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_reduced_outputs_trace;

/* v315: compose the three upstream local_478-style vector materializers
 * (V303/V310/V313) with the v314 terminal FUN_f3fdd33c export wrapper.
 * This is an adapter-facing seam for the caller loop: each param_N export gets
 * its own carried high 26-pair image, low accumulator inputs, seed vectors, and
 * computed scalar0 argument; all three then share the same FUN_f3fdd33c seed
 * vector and native scalar1/param2 call-site arguments.
 */
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
    l3_authorization_reduced_outputs_trace *trace);


typedef struct {
    l3_authorization_core_post_secondary_mix_convolution26_trace param5_high_conv_trace;
    l3_authorization_stage2_stage_convolution_trace param6_high_conv_trace;
    l3_authorization_stage3_stage_convolution_trace param7_high_conv_trace;
    l3_authorization_word_pair param5_high_pairs26[26];
    l3_authorization_word_pair param6_high_pairs26[26];
    l3_authorization_word_pair param7_high_pairs26[26];
    l3_authorization_reduced_outputs_trace terminal_bound_trace;
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_convolved_outputs_trace;

/* v316: bind the earlier accumulator/convolution families that produce the
 * three carried high 26-pair images into the v315 vector-materializer and
 * terminal-export wrapper.  This advances the caller-level integration from
 * externally supplied high images to native-order inputs for the V299/V300,
 * V308/V309, and V311/V312 convolution families.
 */
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
    l3_authorization_convolved_outputs_trace *trace);


typedef struct {
    uint32_t round_index;
    int32_t round_2cc_words13[13];
    int32_t round_ec8_words13[13];
    l3_authorization_convolved_outputs_trace bound_high_trace;
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_context_outputs_trace;

/* v317: bind the v316 integrated terminal path to the live FUN_f3fbc5c0
 * round-context layout.  The native loop uses a 0x34-byte stride per round.
 * For a given round_index this helper extracts two contiguous 13-word windows
 * from the param_1-style context image:
 *
 *   round_2cc[i] = *(int32_t *)(param_1 + round_index*0x34 + 0x2cc + i*4)
 *   round_ec8[i] = *(int32_t *)(param_1 + round_index*0x34 + 0xec8 + i*4)
 *
 * Those windows feed the already-ported V316 high/low terminal integration in
 * the native reuse pattern: +0x2cc for DAT_f41a26b0/DAT_f41a2b30, and
 * +0xec8 for DAT_f41a27b0/DAT_f41a2c30.
 */
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
    l3_authorization_context_outputs_trace *trace);


typedef struct {
    uint32_t round_index;
    int32_t seed_words13[13];
    uint32_t native_param2_unused;
    uint32_t scalar1_unused;
    l3_authorization_context_outputs_trace live_round_trace;
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_live_outputs_trace;

/* v318: bind V317 to the live FUN_f3fbc5c0 param_1 seed/scalar layout.
 * In addition to the round windows extracted by V317, this helper reads the
 * shared FUN_f3fdd33c seed vector and scalar call-site arguments directly from
 * the param_1-style context image:
 *
 *   seed_words13[i]       = *(int32_t *)(param_1 + 0x90 + i*4)
 *   native_param2_unused  = *(uint32_t *)(param_1 + 0x268)
 *   scalar1_unused        = *(uint32_t *)(param_1 + 0x26c)
 */
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
    l3_authorization_live_outputs_trace *trace);


/* v319: bind the live FUN_f3fc3c70 output seam into the V318 live-context
 * terminal wrapper.  Native FUN_f3fbc5c0 first calls FUN_f3fc3c70, producing
 * local_514/local_548/local_57c.  This helper computes those three 13-word
 * outputs through the already-ported V264 terminal path, then feeds local_514
 * and local_548 into the V318 param_5/param_6/param_7 terminal chain.  The
 * local_57c output is preserved in the trace for the following FUN_f3fcc2d0
 * binding seam.
 */
typedef struct {
    l3_authorization_primary_post_sixth_terminal_trace f3fc3c70_trace;
    uint32_t local514_words13[13];
    uint32_t local548_words13[13];
    uint32_t local57c_words13[13];
    uint32_t f3fc3c70_eighth_words13[13];
    l3_authorization_live_outputs_trace live_ctx_trace;
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_round_primary_trace;

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
    l3_authorization_round_primary_trace *trace);



/* v320: bind the completed FUN_f3fcc2d0 terminal param_8/param_9 export
 * into the V319 live FUN_f3fbc5c0 terminal path.  Native FUN_f3fcc2d0
 * writes param_8 to caller local_5e4 and param_9 to caller local_618; this
 * helper computes both through the V298 terminal wrapper, feeds local_5e4
 * into the existing V319 path, and preserves local_618 in the trace for the
 * next post-param_7 loop seam.  The earlier FUN_f3fcc2d0 param_7/local_5b0
 * export remains an explicit input until that native materializer is bound.
 */
typedef struct {
    l3_authorization_secondary_terminal_param8_param9_trace secondary_mix_terminal_trace;
    uint32_t secondary_mix_local5e4_words13[13];
    uint32_t secondary_mix_local618_words13[13];
    l3_authorization_round_primary_trace bound_fc3c70_trace;
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_round_secondary_trace;

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
    l3_authorization_round_secondary_trace *trace);



/* v325: compute the post-seventh FUN_f3fdc45c vector preimages that feed the
 * V321 param_7/local_5b0 and local_460 export ladders, then continue through
 * the V324 local_530 and V320 live terminal path.  This removes the explicit
 * secondary_mix_param7_pre/local460_pre caller seam from the newest wrapper.
 */
typedef struct {
    l3_authorization_secondary_vector_preimage_param7_local460_trace secondary_mix_preimage_trace;
    uint32_t secondary_mix_local5b0_words13[13];
    uint32_t secondary_mix_local460_words13[13];
    uint32_t secondary_mix_local4c8_words26[26];
    l3_authorization_secondary_local3c4_local460_convolution26_trace secondary_mix_local4c8_trace;
    uint32_t secondary_mix_local530_words26[26];
    l3_authorization_secondary_param2_local42c_convolution26_trace secondary_mix_local530_trace;
    l3_authorization_round_secondary_trace bound_secondary_mix_trace;
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_round_secondary_preimage_trace;

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
    l3_authorization_round_secondary_preimage_trace *trace);


/* v326: consume the V320 carried FUN_f3fcc2d0 local_618 export through the
 * next native post-param_7 DAT_f41a2db0/DAT_f41a2e30 accumulator-convolution
 * seam while preserving the V325 visible param_5/param_6/param_7 boundary. */
typedef struct {
    l3_authorization_round_secondary_preimage_trace bound_preimage_trace;
    int32_t round_ec8_words13[13];
    uint32_t secondary_mix_local618_words13[13];
    l3_authorization_stage4_stage_convolution_trace post_local618_convolution_trace;
    l3_authorization_word_pair mixer_core_post_local618_pairs26[26];
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_round_tail_convolution_trace;

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
    l3_authorization_round_tail_convolution_trace *trace);

/* v327: consume the V326 post-local_618 26-pair image through the native
 * local_478-style vector materializer and bind it into the following
 * FUN_f3fdd33c param_7 export.  This keeps the already completed param_5 and
 * param_6 outputs from the V326 path, and replaces caller param_7 with the
 * native post-local_618 terminal export. */
typedef struct {
    l3_authorization_round_tail_convolution_trace bound_local618_trace;
    l3_authorization_word_pair param7_high_pairs26[26];
    uint64_t post_local618_local478_halves13[13][2];
    uint32_t post_local618_scalar0;
    l3_authorization_stage3_stage_vector_reduction_trace post_local618_vector_trace;
    l3_authorization_feedback_output_c_trace post_local618_param7_trace;
    uint32_t pre_param7_words13[13];
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_round_tail_feedback_trace;

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
    l3_authorization_round_tail_feedback_trace *trace);

/* v329: bind the V328 FUN_f3fcc2d0 local_3f8/sixth/local_42c preimage
 * producer into the latest V327 live FUN_f3fbc5c0 terminal wrapper.  The
 * native FUN_f3fc3c70 local_514 output is used as FUN_f3fcc2d0 param_1;
 * local_3f8 is derived from local_390 through the fourth FUN_f3fdc45c seam;
 * local_3c4 remains the explicit upstream preimage input. */
typedef struct {
    l3_authorization_primary_post_sixth_terminal_trace f3fc3c70_param1_trace;
    uint32_t f3fc3c70_local514_words13[13];
    uint32_t f3fc3c70_local548_words13[13];
    uint32_t f3fc3c70_eighth_words13[13];
    uint32_t f3fc3c70_local57c_words13[13];
    l3_authorization_secondary_bound_fourth_secondary_vector_reduce_mix3f8_trace secondary_mix_local3f8_trace;
    l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_trace secondary_mix_v328_preimage_trace;
    l3_authorization_round_tail_feedback_trace bound_v327_trace;
    int32_t secondary_mix_seed_words13[13];
    uint32_t secondary_mix_scalar0;
    uint32_t secondary_mix_scalar1;
    uint32_t secondary_mix_local3f8_words13[13];
    uint32_t secondary_mix_local42c_words13[13];
    uint32_t secondary_mix_sixth_secondary_vector_reduce_words13[13];
    uint32_t secondary_mix_local5b0_words13[13];
    uint32_t secondary_mix_local460_words13[13];
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_round_tail_preimage_trace;

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
    l3_authorization_round_tail_preimage_trace *trace);

/* v330: derive FUN_f3fcc2d0 local_3c4 from the native third FUN_f3fdc45c
 * path before the V329 preimage/live terminal binding.  This removes the
 * explicit local_3c4 seam from the adapter-facing wrapper; the param_4 + 0xc4
 * caller-mix vector is read directly from the full FUN_f3fbc5c0 context. */
typedef struct {
    l3_authorization_secondary_bound_third_secondary_vector_reduce_mix3c4_trace secondary_mix_local3c4_trace;
    l3_authorization_round_tail_preimage_trace bound_v329_trace;
    uint32_t secondary_mix_affine_words13[13];
    int32_t secondary_mix_param4_c4_words13[13];
    uint32_t secondary_mix_local3c4_words13[13];
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_round_tail_state_trace;

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
    l3_authorization_round_tail_state_trace *trace);



/* v331: bind the V330 full-round terminal output to the native loop-tail
 * state handoff.  After the post-local_618 FUN_f3fdd33c param_7 export,
 * native FUN_f3fbc5c0 decrements the 0x34-byte round cursor; this wrapper
 * exposes both the current and next round-context windows (+0x2cc/+0xec8)
 * alongside the V330 param_5/param_6/param_7 outputs. */
typedef struct {
    l3_authorization_round_tail_state_trace bound_v330_trace;
    uint32_t round_index;
    uint32_t next_round_index;
    uint32_t has_next_round;
    int32_t current_round_2cc_words13[13];
    int32_t current_round_ec8_words13[13];
    int32_t next_round_2cc_words13[13];
    int32_t next_round_ec8_words13[13];
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
} l3_authorization_round_finish_trace;

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
    l3_authorization_round_finish_trace *trace);

#endif /* L3_AUTHORIZATION_ROUND_INTERNAL_H */
