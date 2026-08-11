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
#ifndef L3_AUTHORIZATION_MATERIAL_SECONDARY_MIX_H
#define L3_AUTHORIZATION_MATERIAL_SECONDARY_MIX_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTH_ROUND_SECONDARY_WORDS13 13u
#define L3_AUTH_ROUND_SECONDARY_PAIRS13 13u
#define L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26 26u

typedef struct l3_authorization_secondary_pair32 {
    uint32_t lo;
    uint32_t hi;
} l3_authorization_secondary_pair32;

typedef struct l3_authorization_secondary_param3_accum15_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_param3_accum15_trace;

typedef struct l3_authorization_secondary_param3_accum1670_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_param3_accum1670_trace;


typedef struct l3_authorization_secondary_param3_convolution26_trace {
    l3_authorization_secondary_pair32 convolution26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range15_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1670_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 pre_reduce26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 output26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t first_last_count26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][3];
} l3_authorization_secondary_param3_convolution26_trace;

/* v265: first FUN_f3fcc2d0 generated pair accumulator over param_3.
 * This ports the native block that starts at the FUN_f3fcc2d0 prologue,
 * folds through DAT_f41a15f0, writes the individual 13 pair stream
 * beginning at auStack_1c0+2, and maintains the cumulative carry chain
 * later exposed through the local_e0/local_d8/... window.
 */
int l3_authorization_secondary_param3_accum15_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_accum15_trace *out);

/* Sibling head seed used by the following local_220/auStack_1c0+0x1c path.
 * It ports the scalar head driven by DAT_f41a1670 before the second 13-lane
 * loop begins.  A later checkpoint will extend this into the full sibling
 * loop; keeping this exact seed available makes the next boundary explicit.
 */


/* v266: sibling FUN_f3fcc2d0 generated pair accumulator over param_3.
 * This extends the v265 scalar head through the native DAT_f41a1670
 * 13-lane loop, writing the local_220 pair stream and the cumulative
 * auStack_1c0+0x1c handoff image used by the following convolution.
 */
int l3_authorization_secondary_param3_accum1670_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_accum1670_trace *out);


/* v267: first FUN_f3fcc2d0 convolution/reduction network.
 * Consumes the v265 DAT_f41a15f0 pair stream and the v266 DAT_f41a1670
 * pair stream, computes the native 26-lane cross-convolution plus range
 * correction, and emits the local_328 image passed to FUN_f3fdc45c.
 */
int l3_authorization_secondary_param3_convolution26(
    const l3_authorization_secondary_pair32 raw15_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix15_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1670_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1670_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_convolution26_trace *out);


/* v268: bounded FUN_f3fdc45c helper family used by downstream FUN_f3fcc2d0.
 * FUN_f3fdc45c has the same high-level structure as FUN_f3fdb57c, but uses
 * its own seed tables and lane/export affine constants.  It consumes a 13-word
 * seed vector plus a mutable 26-pair state image such as V267 local_328.
 */
typedef struct l3_authorization_secondary_vector_reduce_lane_scalars {
    uint32_t base_pair[2];
    uint32_t lane_pair[2];
    uint32_t ladder1770_pair[2];
    uint32_t second_pair[2];
    uint32_t scale;
    uint32_t vector_bias;
    uint32_t mul8cec_low;
    uint32_t mul19f4_low;
} l3_authorization_secondary_vector_reduce_lane_scalars;

typedef struct l3_authorization_secondary_vector_reduce_vector_feedback_result {
    uint32_t scale_pair[2];
    uint32_t vector_bias_pair[2];
    uint32_t product_pairs12[12][2];
} l3_authorization_secondary_vector_reduce_vector_feedback_result;

typedef struct l3_authorization_secondary_vector_reduce_lane_tail_result {
    uint32_t current_affine_pair[2];
    uint32_t ladder17f0_pair[2];
    uint32_t post17f0_pair[2];
    uint32_t ladder1870_mix;
    uint32_t tail_product_pair[2];
    uint32_t final_lane1_pair[2];
    uint32_t final_lane12_pair[2];
} l3_authorization_secondary_vector_reduce_lane_tail_result;

typedef struct l3_authorization_secondary_vector_reduce_run13_trace {
    uint32_t seed_pairs13[L3_AUTH_ROUND_SECONDARY_WORDS13][2];
    l3_authorization_secondary_vector_reduce_lane_scalars lane_scalars[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_vector_reduce_lane_tail_result lane_tails[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_vector_reduce_run13_trace;

typedef struct l3_authorization_secondary_vector_reduce_export_trace {
    uint32_t export_inputs13[L3_AUTH_ROUND_SECONDARY_WORDS13][2];
    uint32_t pre_export_word13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t ladder7_pair13[L3_AUTH_ROUND_SECONDARY_WORDS13][2];
    uint32_t low_after15_13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t rolling_pair_after13[L3_AUTH_ROUND_SECONDARY_WORDS13][2];
} l3_authorization_secondary_vector_reduce_export_trace;

typedef struct l3_authorization_secondary_vector_reduce_full_export_trace {
    l3_authorization_secondary_vector_reduce_run13_trace run13;
    l3_authorization_secondary_vector_reduce_export_trace export_trace;
} l3_authorization_secondary_vector_reduce_full_export_trace;

int l3_authorization_secondary_vector_reduce_seed_pairs13(
    const int32_t param1_13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_seed_pairs13[L3_AUTH_ROUND_SECONDARY_WORDS13][2]);

int l3_authorization_secondary_vector_reduce_lane_scalar_core(
    uint32_t param3,
    uint32_t param4,
    const uint32_t rolling_pair[2],
    l3_authorization_secondary_vector_reduce_lane_scalars *out);

int l3_authorization_secondary_vector_reduce_vector_feedback_stores(
    const uint32_t seed_pairs13[L3_AUTH_ROUND_SECONDARY_WORDS13][2],
    const l3_authorization_secondary_vector_reduce_lane_scalars *lane_scalars,
    unsigned lane_index,
    uint32_t state_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2],
    l3_authorization_secondary_vector_reduce_vector_feedback_result *out);

int l3_authorization_secondary_vector_reduce_lane_scalar_tail(
    const uint32_t seed_tail_pair[2],
    const l3_authorization_secondary_vector_reduce_lane_scalars *lane_scalars,
    unsigned lane_index,
    uint32_t state_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2],
    l3_authorization_secondary_vector_reduce_lane_tail_result *out);

int l3_authorization_secondary_vector_reduce_run13_update(
    const int32_t param1_13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t param3,
    uint32_t param4,
    uint32_t state_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2],
    l3_authorization_secondary_vector_reduce_run13_trace *out);

int l3_authorization_secondary_vector_reduce_export_words13(
    const uint32_t state_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_vector_reduce_export_trace *out);

int l3_authorization_secondary_vector_reduce_run13_update_export(
    const int32_t param1_13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t param3,
    uint32_t param4,
    uint32_t state_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_vector_reduce_full_export_trace *out);


/* v269: bind the FUN_f3fcc2d0 caller-side arguments into FUN_f3fdc45c,
 * then materialize the immediate post-call vector-affine 13-word stream
 * (local_35c/local_34c/local_33c/local_32c in the decompiler).
 */
typedef struct l3_authorization_secondary_post_secondary_vector_reduce_affine_trace {
    uint32_t input_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t affine_group0_4[4];
    uint32_t affine_group1_4[4];
    uint32_t affine_group2_4[4];
    uint32_t scalar_tail_word12;
    uint32_t output_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_post_secondary_vector_reduce_affine_trace;

typedef struct l3_authorization_secondary_bound_secondary_vector_reduce_affine_trace {
    l3_authorization_secondary_param3_convolution26_trace convolution;
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    l3_authorization_secondary_post_secondary_vector_reduce_affine_trace affine;
    uint32_t state_before_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_bound_secondary_vector_reduce_affine_trace;

int l3_authorization_secondary_post_secondary_vector_reduce_affine_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_secondary_vector_reduce_affine_trace *out);

int l3_authorization_secondary_param3_bound_secondary_vector_reduce_affine_words13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_secondary_vector_reduce_affine_trace *out);


/* v270: second FUN_f3fcc2d0 generated-pair accumulator family.
 * After the V269 f3fdc45c affine materialization, native FUN_f3fcc2d0
 * builds two more 13-lane generated-pair streams: one from caller param_5
 * through DAT_f41a1970 and one from the V269 affine words through
 * DAT_f41a19f0.  These mirror the earlier param_3 accumulator shape and
 * feed the following convolution/reduction network.
 */
typedef struct l3_authorization_secondary_param5_accum1970_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_param5_accum1970_trace;

typedef struct l3_authorization_secondary_affine_accum19f0_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_affine_accum19f0_trace;

typedef struct l3_authorization_secondary_param5_affine_accum_trace {
    l3_authorization_secondary_param5_accum1970_trace accum1970;
    l3_authorization_secondary_affine_accum19f0_trace accum19f0;
} l3_authorization_secondary_param5_affine_accum_trace;

int l3_authorization_secondary_param5_accum1970_pairs13(
    const int32_t param5_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param5_accum1970_trace *out);

int l3_authorization_secondary_affine_accum19f0_pairs13(
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_affine_accum19f0_trace *out);

int l3_authorization_secondary_param5_affine_accum_pairs13(
    const int32_t param5_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw1970_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1970_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw19f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix19f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param5_affine_accum_trace *out);

/* v271: second FUN_f3fcc2d0 convolution/reduction network.
 * Consumes the V270 DAT_f41a1970 stream and DAT_f41a19f0 stream,
 * computes the native 26-lane cross-convolution plus range correction,
 * and emits the second local_328 image passed to the next FUN_f3fdc45c call.
 */
typedef struct l3_authorization_secondary_param5_convolution26_trace {
    l3_authorization_secondary_pair32 convolution26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1970_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range19f0_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 pre_reduce26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 output26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t first_last_count26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][3];
} l3_authorization_secondary_param5_convolution26_trace;

int l3_authorization_secondary_param5_convolution26(
    const l3_authorization_secondary_pair32 raw1970_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1970_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw19f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix19f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param5_convolution26_trace *out);

int l3_authorization_secondary_param5_affine_accum_convolution26(
    const int32_t param5_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param5_convolution26_trace *out);
int l3_authorization_secondary_param3_accum_convolution26(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_convolution26_trace *out);


/* v272: bind the second FUN_f3fcc2d0 local_328 image into the following
 * FUN_f3fdc45c call, then materialize the mixed param_1/param_4/native-output
 * 13-word stream and its DAT_f41a1a70/DAT_f4190d30/DAT_f4190d50 export
 * into local_390.  This is the native block immediately after the V271
 * convolution and second FUN_f3fdc45c call-site.
 */
typedef struct l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_trace {
    uint32_t input_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t mixed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t loop_x13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t ladder7_13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t ladder8_rolling13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t rolling_before13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t output_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_trace;

typedef struct l3_authorization_secondary_bound_second_secondary_vector_reduce_mix390_trace {
    l3_authorization_secondary_param5_convolution26_trace convolution;
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_trace mix390;
    uint32_t state_before_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_bound_second_secondary_vector_reduce_mix390_trace;

int l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_trace *out);

int l3_authorization_secondary_derive_state_stage1(
    const int32_t param5_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_second_secondary_vector_reduce_mix390_trace *out);


/* v273: third FUN_f3fcc2d0 generated-pair accumulator family.
 * After the V272 local_390 materialization, native FUN_f3fcc2d0 builds
 * another pair of 13-lane streams: param_3 through DAT_f41a1ab0 and the
 * earlier V269 post-FUN_f3fdc45c affine stream (local_35c) through
 * DAT_f41a1b30.  The V272 local_390 stream is preserved for the following
 * mixed param_2 block; it is not the DAT_f41a1b30 input at this boundary.
 */
typedef struct l3_authorization_secondary_param3_accum1ab0_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_param3_accum1ab0_trace;

typedef struct l3_authorization_secondary_affine_accum1b30_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_affine_accum1b30_trace;

typedef struct l3_authorization_secondary_param3_affine_accum1ab0_1b30_trace {
    l3_authorization_secondary_param3_accum1ab0_trace accum1ab0;
    l3_authorization_secondary_affine_accum1b30_trace accum1b30;
} l3_authorization_secondary_param3_affine_accum1ab0_1b30_trace;

int l3_authorization_secondary_param3_accum1ab0_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_accum1ab0_trace *out);

int l3_authorization_secondary_affine_accum1b30_pairs13(
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_affine_accum1b30_trace *out);

int l3_authorization_secondary_param3_affine_accum1ab0_1b30_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw1ab0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1ab0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw1b30_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1b30_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_affine_accum1ab0_1b30_trace *out);


/* v274: third FUN_f3fcc2d0 convolution/reduction network.
 * Consumes the V273 DAT_f41a1ab0 stream and DAT_f41a1b30 stream,
 * computes the native 26-lane cross-convolution plus range correction,
 * and emits the next local_328 image passed to another FUN_f3fdc45c call.
 */
typedef struct l3_authorization_secondary_param3_affine_convolution26_trace {
    l3_authorization_secondary_pair32 convolution26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1ab0_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1b30_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 pre_reduce26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 output26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t first_last_count26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][3];
} l3_authorization_secondary_param3_affine_convolution26_trace;

int l3_authorization_secondary_param3_affine_convolution26(
    const l3_authorization_secondary_pair32 raw1ab0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1ab0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1b30_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1b30_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_affine_convolution26_trace *out);

int l3_authorization_secondary_param3_affine_accum_convolution26(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_affine_convolution26_trace *out);


/* v275: bind the third FUN_f3fcc2d0 local_328 image into FUN_f3fdc45c,
 * then materialize the post-call param_2/param_4 mixed local_3c4 stream.
 * This is the native block immediately after the V274 convolution and before
 * the following DAT_f41a1cf0/DAT_f41a1d70 accumulator family.
 */
typedef struct l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_trace {
    uint32_t input_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t mixed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t loop_x13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t ladder7_13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t ladder8_rolling13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t rolling_before13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t output_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_trace;

typedef struct l3_authorization_secondary_bound_third_secondary_vector_reduce_mix3c4_trace {
    l3_authorization_secondary_param3_affine_convolution26_trace convolution;
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_trace mix3c4;
    uint32_t state_before_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_bound_third_secondary_vector_reduce_mix3c4_trace;

int l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_trace *out);

int l3_authorization_secondary_derive_state_stage2(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t affine_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_third_secondary_vector_reduce_mix3c4_trace *out);

/* Native bridge omitted by the older staged reconstruction: caller param_6
 * and the third FUN_f3fdc45c export form the fourth call's 26-pair state. */
int l3_authorization_secondary_param6_third_export_convolution26(
    const int32_t caller_param6_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t third_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32
        out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26]);

typedef struct l3_authorization_secondary_bound_param6_fourth_trace {
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    l3_authorization_secondary_post_third_secondary_vector_reduce_mix3c4_trace mix3c4;
    uint32_t state_before_f3fdc45c26[
        L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[
        L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_bound_param6_fourth_trace;



/* v276: local_390 generated-pair accumulator family.
 * The native block after the V275 local_3c4 materialization immediately
 * builds two pair streams from the V272 local_390 words: local_390 through
 * DAT_f41a1cf0 and local_390 through DAT_f41a1d70.  Keeping this API named
 * local390 is intentional: local_3c4 is carried forward but is not consumed
 * by these two accumulators.
 */
typedef struct l3_authorization_secondary_local390_accum1cf0_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local390_accum1cf0_trace;

typedef struct l3_authorization_secondary_local390_accum1d70_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local390_accum1d70_trace;

typedef struct l3_authorization_secondary_local390_accum1cf0_1d70_trace {
    l3_authorization_secondary_local390_accum1cf0_trace accum1cf0;
    l3_authorization_secondary_local390_accum1d70_trace accum1d70;
} l3_authorization_secondary_local390_accum1cf0_1d70_trace;

int l3_authorization_secondary_local390_accum1cf0_pairs13(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local390_accum1cf0_trace *out);

int l3_authorization_secondary_local390_accum1d70_pairs13(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local390_accum1d70_trace *out);

int l3_authorization_secondary_local390_accum1cf0_1d70_pairs13(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw1cf0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1cf0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw1d70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1d70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local390_accum1cf0_1d70_trace *out);


/* v277: local_390 DAT_f41a1cf0/DAT_f41a1d70 convolution/reduction network.
 * Consumes the V276 accumulator streams, computes the native 26-lane
 * cross-convolution plus range corrections, and emits the next local_328
 * image for the subsequent FUN_f3fdc45c(...) call.
 */
typedef struct l3_authorization_secondary_local390_convolution26_trace {
    l3_authorization_secondary_pair32 convolution26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1cf0_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1d70_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 pre_reduce26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 output26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t first_last_count26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][3];
} l3_authorization_secondary_local390_convolution26_trace;

int l3_authorization_secondary_local390_convolution26(
    const l3_authorization_secondary_pair32 raw1cf0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1cf0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1d70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1d70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local390_convolution26_trace *out);

int l3_authorization_secondary_local390_accum_convolution26(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local390_convolution26_trace *out);


/* v278: bind the V277 local_328 image into FUN_f3fdc45c, then materialize
 * the post-call local_3f8/local_3e8/local_3d8/local_3c8 affine stream.
 * This native block is immediately after the V277 convolution and before
 * the following DAT_f41a1df0/DAT_f41a1e70 accumulator family.
 */
typedef struct l3_authorization_secondary_post_fourth_secondary_vector_reduce_mix3f8_trace {
    uint32_t input_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t affine_group0_4[4];
    uint32_t affine_group1_4[4];
    uint32_t affine_group2_4[4];
    uint32_t scalar_tail_word12;
    uint32_t output_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_post_fourth_secondary_vector_reduce_mix3f8_trace;

typedef struct l3_authorization_secondary_bound_fourth_secondary_vector_reduce_mix3f8_trace {
    l3_authorization_secondary_local390_convolution26_trace convolution;
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    l3_authorization_secondary_post_fourth_secondary_vector_reduce_mix3f8_trace mix3f8;
    uint32_t state_before_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_bound_fourth_secondary_vector_reduce_mix3f8_trace;

int l3_authorization_secondary_post_fourth_secondary_vector_reduce_mix3f8_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_fourth_secondary_vector_reduce_mix3f8_trace *out);

int l3_authorization_secondary_derive_state_stage3(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_fourth_secondary_vector_reduce_mix3f8_trace *out);


/* v279: generated-pair accumulator family after the fourth FUN_f3fdc45c
 * materialization.  The DAT_f41a1df0 stream is seeded from local_3f8[0]
 * and then consumes local_3f8[1..12].  The sibling DAT_f41a1e70 stream is
 * seeded from the caller scalar at param_4+0x26c and then consumes
 * local_390[1..12].
 */
typedef struct l3_authorization_secondary_local3f8_accum1df0_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local3f8_accum1df0_trace;

typedef struct l3_authorization_secondary_local390_accum1e70_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local390_accum1e70_trace;

typedef struct l3_authorization_secondary_local3f8_local390_accum1df0_1e70_trace {
    l3_authorization_secondary_local3f8_accum1df0_trace accum1df0;
    l3_authorization_secondary_local390_accum1e70_trace accum1e70;
} l3_authorization_secondary_local3f8_local390_accum1df0_1e70_trace;

int l3_authorization_secondary_local3f8_accum1df0_pairs13(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3f8_accum1df0_trace *out);

int l3_authorization_secondary_local390_accum1e70_pairs13(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar1,
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local390_accum1e70_trace *out);

int l3_authorization_secondary_local3f8_local390_accum1df0_1e70_pairs13(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar1,
    l3_authorization_secondary_pair32 raw1df0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1df0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw1e70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1e70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3f8_local390_accum1df0_1e70_trace *out);



/* v280: local_3f8/local_390 DAT_f41a1df0/DAT_f41a1e70 convolution/reduction network.
 * Consumes the V279 accumulator streams, computes the native 26-lane
 * cross-convolution plus range corrections, and emits the next local_328
 * image for another FUN_f3fdc45c(...) call.
 */
typedef struct l3_authorization_secondary_local3f8_local390_convolution26_trace {
    l3_authorization_secondary_pair32 convolution26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1df0_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1e70_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 pre_reduce26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 output26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t first_last_count26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][3];
} l3_authorization_secondary_local3f8_local390_convolution26_trace;

int l3_authorization_secondary_local3f8_local390_convolution26(
    const l3_authorization_secondary_pair32 raw1df0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1df0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1e70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1e70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local3f8_local390_convolution26_trace *out);

int l3_authorization_secondary_local3f8_local390_accum_convolution26(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar1,
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local3f8_local390_convolution26_trace *out);



/* v281: bind the V280 local_328 image into FUN_f3fdc45c, then materialize
 * the post-call local_42c/local_41c/local_418/local_414/local_410/local_400/local_3fc stream.
 * This is the native block immediately after the V280 convolution and before
 * the following DAT_f41a1ef0 accumulator family.
 */
typedef struct l3_authorization_secondary_post_fifth_secondary_vector_reduce_mix42c_trace {
    uint32_t input_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t affine_group0_4[4];
    uint32_t affine_middle3[3];
    uint32_t affine_group7_10[4];
    uint32_t affine_tail11_12[2];
    uint32_t output_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_post_fifth_secondary_vector_reduce_mix42c_trace;

typedef struct l3_authorization_secondary_bound_fifth_secondary_vector_reduce_mix42c_trace {
    l3_authorization_secondary_local3f8_local390_convolution26_trace convolution;
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    l3_authorization_secondary_post_fifth_secondary_vector_reduce_mix42c_trace mix42c;
    uint32_t state_before_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_bound_fifth_secondary_vector_reduce_mix42c_trace;

int l3_authorization_secondary_post_fifth_secondary_vector_reduce_mix42c_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_fifth_secondary_vector_reduce_mix42c_trace *out);

int l3_authorization_secondary_derive_state_stage4(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t local390_word0_for_accum,
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_fifth_secondary_vector_reduce_mix42c_trace *out);


/* v282: generated-pair accumulator family after the fifth FUN_f3fdc45c
 * materialization.  The DAT_f41a1ef0 stream consumes caller param_1.  The
 * sibling DAT_f41a1f70 stream consumes the carried local_3f8 words from V278.
 * The V281 local_42c/mix42c export is carried forward but is not consumed by
 * this accumulator family.
 */
typedef struct l3_authorization_secondary_param1_accum1ef0_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_param1_accum1ef0_trace;

typedef struct l3_authorization_secondary_local3f8_accum1f70_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local3f8_accum1f70_trace;

typedef struct l3_authorization_secondary_param1_local3f8_accum1ef0_1f70_trace {
    l3_authorization_secondary_param1_accum1ef0_trace accum1ef0;
    l3_authorization_secondary_local3f8_accum1f70_trace accum1f70;
} l3_authorization_secondary_param1_local3f8_accum1ef0_1f70_trace;

int l3_authorization_secondary_param1_accum1ef0_pairs13(
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param1_accum1ef0_trace *out);

int l3_authorization_secondary_local3f8_accum1f70_pairs13(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3f8_accum1f70_trace *out);

int l3_authorization_secondary_param1_local3f8_accum1ef0_1f70_pairs13(
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw1ef0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1ef0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw1f70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1f70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param1_local3f8_accum1ef0_1f70_trace *out);


/* v283: param_1/local_3f8 DAT_f41a1ef0/DAT_f41a1f70 convolution/reduction network.
 * Consumes the V282 accumulator streams, computes the native 26-lane
 * cross-convolution plus range corrections, and emits the next local_328
 * image for another FUN_f3fdc45c(...) call.
 */
typedef struct l3_authorization_secondary_param1_local3f8_convolution26_trace {
    l3_authorization_secondary_pair32 convolution26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1ef0_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1f70_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 pre_reduce26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 output26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t first_last_count26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][3];
} l3_authorization_secondary_param1_local3f8_convolution26_trace;

int l3_authorization_secondary_param1_local3f8_convolution26(
    const l3_authorization_secondary_pair32 raw1ef0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1ef0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1f70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1f70_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param1_local3f8_convolution26_trace *out);

int l3_authorization_secondary_param1_local3f8_accum_convolution26(
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param1_local3f8_convolution26_trace *out);


/* v284: bind the V283 local_328 image into the next FUN_f3fdc45c(...)
 * call and expose its 13-word export as an explicit boundary.  The following
 * native materialization mixes this export with V281 local_42c, carried local
 * streams, and param_4 fields; keeping this call-site standalone gives that
 * larger vector block a stable input image.
 */
typedef struct l3_authorization_secondary_bound_sixth_secondary_vector_reduce_export_trace {
    l3_authorization_secondary_param1_local3f8_convolution26_trace convolution;
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    uint32_t state_before_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_bound_sixth_secondary_vector_reduce_export_trace;

int l3_authorization_secondary_derive_feedback_state(
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_sixth_secondary_vector_reduce_export_trace *out);

/* v285: source block after the sixth FUN_f3fdc45c export.  The native code
 * carries the V284 export into three 4-lane vector products while the main
 * pair network consumes local_3c4 twice through DAT_f41a2070 and DAT_f41a1ff0.
 * This boundary exposes both generated-pair streams and the side-vector images
 * needed by the later post-call materialization.
 */
typedef struct l3_authorization_secondary_local3c4_accum2070_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local3c4_accum2070_trace;

typedef struct l3_authorization_secondary_local3c4_accum1ff0_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local3c4_accum1ff0_trace;

typedef struct l3_authorization_secondary_sixth_local3c4_accum_sources_trace {
    uint32_t sixth_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_local3c4_accum2070_trace accum2070;
    l3_authorization_secondary_local3c4_accum1ff0_trace accum1ff0;
    uint32_t side_vec60_0_3[4];
    uint32_t side_vec99_4_7[4];
    uint32_t side_vec81_8_11[4];
    uint32_t side_word12;
} l3_authorization_secondary_sixth_local3c4_accum_sources_trace;

int l3_authorization_secondary_local3c4_accum2070_pairs13(
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3c4_accum2070_trace *out);

int l3_authorization_secondary_local3c4_accum1ff0_pairs13(
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3c4_accum1ff0_trace *out);

int l3_authorization_secondary_sixth_export_local3c4_accum2070_1ff0_sources(
    const uint32_t sixth_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw2070_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix2070_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw1ff0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix1ff0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_sixth_local3c4_accum_sources_trace *out);


/* v286: local_3c4 DAT_f41a2070/DAT_f41a1ff0 convolution/reduction network.
 * Consumes the V285 accumulator streams, computes the native 26-lane
 * cross-convolution plus range corrections, and emits the next local_328
 * image for another FUN_f3fdc45c(...) call.
 */
typedef struct l3_authorization_secondary_local3c4_convolution26_trace {
    l3_authorization_secondary_pair32 convolution26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range2070_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range1ff0_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 pre_reduce26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 output26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t first_last_count26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][3];
} l3_authorization_secondary_local3c4_convolution26_trace;

int l3_authorization_secondary_local3c4_convolution26(
    const l3_authorization_secondary_pair32 raw2070_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix2070_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw1ff0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix1ff0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local3c4_convolution26_trace *out);


/* v287: bind the V286 local_3c4 convolution image into the next native
 * FUN_f3fdc45c(...) call, then reuse the repeated local_390 materialization
 * shape that appears after the second call-site.  This exposes the post-V286
 * f3fdc45c export and the refreshed local_390 stream as one boundary.
 */
typedef struct l3_authorization_secondary_bound_seventh_secondary_vector_reduce_mix390_trace {
    l3_authorization_secondary_local3c4_convolution26_trace convolution;
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    l3_authorization_secondary_post_second_secondary_vector_reduce_mix390_trace mix390;
    uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t caller_mix_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t caller_scalar0;
    uint32_t caller_scalar1;
    uint32_t state_before_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_bound_seventh_secondary_vector_reduce_mix390_trace;






/* v321: rolling export loops immediately after the seventh FUN_f3fdc45c call.
 * The native block first exports a 13-word param_7/local_5b0 image through
 * DAT_f41a20f0 with DAT_f41910b0/DAT_f41910d0, then materializes the sibling
 * local_460 stream through DAT_f41a2130 with DAT_f41910f0/DAT_f4191110.
 * These helpers expose the two native rolling export ladders with their
 * preimage words made explicit; the vector preimage materialization that feeds
 * them remains a separate caller binding seam.
 */
typedef struct l3_authorization_secondary_param7_export20f0_trace {
    uint32_t pre_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t loop_x13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t ladder7_13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t ladder8_rolling13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t rolling_before13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t output_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_param7_export20f0_trace;

typedef struct l3_authorization_secondary_local460_export2130_trace {
    uint32_t pre_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t loop_x13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t ladder7_13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t ladder8_rolling13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t rolling_before13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t output_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local460_export2130_trace;

typedef struct l3_authorization_secondary_param7_local460_exports_trace {
    l3_authorization_secondary_param7_export20f0_trace param7_export;
    l3_authorization_secondary_local460_export2130_trace local460_export;
    uint32_t param7_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_param7_local460_exports_trace;

int l3_authorization_secondary_param7_export20f0_words13(
    const uint32_t pre_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param7_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_param7_export20f0_trace *out);

int l3_authorization_secondary_local460_export2130_words13(
    const uint32_t pre_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_local460_export2130_trace *out);



/* v325: native vector preimage materialization immediately after the seventh
 * FUN_f3fdc45c(...) call.  This resolves the V321 caller seam by computing
 * the 13 preimage words that feed the DAT_f41a20f0 param_7/local_5b0 export
 * ladder and, after that output exists, the 13 preimage words that feed the
 * DAT_f41a2130 local_460 ladder.
 */
typedef struct l3_authorization_secondary_vector_preimage_param7_local460_trace {
    uint32_t sixth_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t caller_scalar0;
    uint32_t caller_scalar1;
    l3_authorization_secondary_sixth_local3c4_accum_sources_trace source_trace;
    l3_authorization_secondary_local3c4_convolution26_trace convolution;
    uint32_t state_before_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t param7_pre_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t param7_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_param7_export20f0_trace param7_export;
    uint32_t local460_pre_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_local460_export2130_trace local460_export;
} l3_authorization_secondary_vector_preimage_param7_local460_trace;

int l3_authorization_secondary_vector_preimage_param7_local460_words13(
    const uint32_t sixth_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t *param4_ctx_words,
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_param7_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_vector_preimage_param7_local460_trace *out);

/* v288: generated-pair accumulator family after the refreshed local_390/export
 * phase.  The native block consumes the carried local_3c4 stream through
 * DAT_f41a2170 and the freshly materialized local_460 stream through
 * DAT_f41a21f0 before the next convolution/reduction network.
 */
typedef struct l3_authorization_secondary_local3c4_accum2170_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local3c4_accum2170_trace;

typedef struct l3_authorization_secondary_local460_accum21f0_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local460_accum21f0_trace;

typedef struct l3_authorization_secondary_local3c4_local460_accum2170_21f0_trace {
    l3_authorization_secondary_local3c4_accum2170_trace accum2170;
    l3_authorization_secondary_local460_accum21f0_trace accum21f0;
} l3_authorization_secondary_local3c4_local460_accum2170_21f0_trace;

int l3_authorization_secondary_local3c4_accum2170_pairs13(
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3c4_accum2170_trace *out);

int l3_authorization_secondary_local460_accum21f0_pairs13(
    const uint32_t local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local460_accum21f0_trace *out);

int l3_authorization_secondary_local3c4_local460_accum2170_21f0_pairs13(
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw2170_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix2170_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw21f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix21f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local3c4_local460_accum2170_21f0_trace *out);

/* v289: convolution/reduction over the V288 DAT_f41a2170/DAT_f41a21f0
 * streams plus the immediate native local_4c8 word materialization.  Unlike
 * the earlier local_328-producing convolutions, this native seam feeds a
 * 26-word side stream before the following accumulator family.
 */
typedef struct l3_authorization_secondary_local3c4_local460_convolution26_trace {
    l3_authorization_secondary_pair32 convolution26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range2170_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range21f0_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 pre_reduce26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t first_last_count26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][3];
} l3_authorization_secondary_local3c4_local460_convolution26_trace;

int l3_authorization_secondary_local3c4_local460_convolution26_materialize4c8(
    const l3_authorization_secondary_pair32 raw2170_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix2170_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw21f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix21f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    uint32_t out_local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local3c4_local460_convolution26_trace *out);

int l3_authorization_secondary_local3c4_local460_accum_convolution26_materialize4c8(
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local3c4_local460_convolution26_trace *out);


/* v290: generated-pair accumulators over native param_2 and carried local_42c.
 * The native block transforms param_2 through DAT_f41911f0/DAT_f4191210
 * before DAT_f41a22f0, and transforms local_42c through
 * DAT_f4191230/DAT_f4191250 before DAT_f41a2370.
 */
typedef struct l3_authorization_secondary_param2_accum22f0_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_param2_accum22f0_trace;

typedef struct l3_authorization_secondary_local42c_accum2370_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local42c_accum2370_trace;

typedef struct l3_authorization_secondary_param2_local42c_accum22f0_2370_trace {
    l3_authorization_secondary_param2_accum22f0_trace accum22f0;
    l3_authorization_secondary_local42c_accum2370_trace accum2370;
} l3_authorization_secondary_param2_local42c_accum22f0_2370_trace;

int l3_authorization_secondary_param2_accum22f0_pairs13(
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param2_accum22f0_trace *out);

int l3_authorization_secondary_local42c_accum2370_pairs13(
    const uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local42c_accum2370_trace *out);

int l3_authorization_secondary_param2_local42c_accum22f0_2370_pairs13(
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw22f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix22f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw2370_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix2370_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param2_local42c_accum22f0_2370_trace *out);

/* v291: convolution/range-correction over the V290 DAT_f41a22f0/DAT_f41a2370
 * streams plus the immediate native local_530 word materialization.
 */
typedef struct l3_authorization_secondary_param2_local42c_convolution26_trace {
    l3_authorization_secondary_pair32 convolution26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range22f0_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range2370_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 pre_reduce26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t first_last_count26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][3];
} l3_authorization_secondary_param2_local42c_convolution26_trace;

int l3_authorization_secondary_param2_local42c_convolution26_materialize530(
    const l3_authorization_secondary_pair32 raw22f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix22f0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw2370_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix2370_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    uint32_t out_local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param2_local42c_convolution26_trace *out);

int l3_authorization_secondary_param2_local42c_accum_convolution26_materialize530(
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param2_local42c_convolution26_trace *out);


/* v292: mixed local_598 materialization after the V289/V291 side-stream
 * pair.  Native FUN_f3fcc2d0 combines the 26-word local_4c8 stream,
 * param_4+0x194 words, and the 26-word local_530 stream.  Stack reuse also
 * leaves words 2..12 seeded from the preceding accumulator image; words 0..1
 * and 13..25 are overwritten by native constants before the three affine
 * additions and DAT_f41a2470 rolling fold.
 */
typedef struct l3_authorization_secondary_local4c8_param4_local530_fold598_trace {
    uint32_t native_seed26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t after_local4c8_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t after_param4_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t pre_fold26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t fold7_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t fold8_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t local598_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
} l3_authorization_secondary_local4c8_param4_local530_fold598_trace;

int l3_authorization_secondary_local4c8_param4_local530_fold598_words26(
    const uint32_t local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t param4_194_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t carry_seed_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    uint32_t out_local598_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local4c8_param4_local530_fold598_trace *out);


/* v293: transform the 26-word local_598 stream through DAT_f4191350 /
 * DAT_f4191370 and the 64-bit DAT_f41a24b0 fold network, producing the
 * next 26-pair local_328 image consumed by the following FUN_f3fdc45c call.
 */
typedef struct l3_authorization_secondary_local598_transform24b0_trace {
    uint32_t transformed_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t final_fold_lows26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 output_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
} l3_authorization_secondary_local598_transform24b0_trace;

typedef struct l3_authorization_secondary_local598_bound_transform24b0_trace {
    l3_authorization_secondary_local4c8_param4_local530_fold598_trace fold598;
    uint32_t local598_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_local598_transform24b0_trace transform24b0;
} l3_authorization_secondary_local598_bound_transform24b0_trace;

int l3_authorization_secondary_local598_transform24b0_pairs26(
    const uint32_t local598_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_local598_transform24b0_trace *out);



/* v294: bind the V293 local_328 image into the following FUN_f3fdc45c(...)
 * call and materialize the immediate native param_8 export.  The call-site
 * passes literal param_2=0x1a to FUN_f3fdc45c; the helper model preserves the
 * consumed seed/scalar/state inputs and records caller_scalar1 even though the
 * current f3fdc45c helper family does not consume it.
 */
typedef struct l3_authorization_secondary_post_transform24b0_param8_trace {
    uint32_t input_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t param8_group0_4[4];
    uint32_t param8_group1_4[4];
    uint32_t param8_group2_4[4];
    uint32_t param8_tail_word12;
    uint32_t output_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_post_transform24b0_param8_trace;

typedef struct l3_authorization_secondary_bound_secondary_vector_reduce_param8_trace {
    uint32_t local598_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t caller_scalar0;
    uint32_t caller_scalar1;
    l3_authorization_secondary_local598_transform24b0_trace transform24b0;
    uint32_t state_before_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_post_transform24b0_param8_trace param8_export;
} l3_authorization_secondary_bound_secondary_vector_reduce_param8_trace;

typedef struct l3_authorization_secondary_bound_fold598_secondary_vector_reduce_param8_trace {
    l3_authorization_secondary_local4c8_param4_local530_fold598_trace fold598;
    uint32_t local598_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_bound_secondary_vector_reduce_param8_trace bound_param8;
} l3_authorization_secondary_bound_fold598_secondary_vector_reduce_param8_trace;

int l3_authorization_secondary_post_transform24b0_param8_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_transform24b0_param8_trace *out);

int l3_authorization_secondary_local598_bound_secondary_vector_reduce_param8_words13(
    const uint32_t local598_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_secondary_vector_reduce_param8_trace *out);

int l3_authorization_secondary_build_terminal_output_a(
    const uint32_t local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t param4_194_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t carry_seed_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_fold598_secondary_vector_reduce_param8_trace *out);


/* v295: post-param8 generated-pair accumulators over transformed param_3
 * and refreshed local_390.  These port the DAT_f41a2530 / DAT_f41a25b0
 * pair streams that follow the V294 param_8 export.
 */
typedef struct l3_authorization_secondary_param3_accum2530_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_param3_accum2530_trace;

typedef struct l3_authorization_secondary_local390_accum25b0_trace {
    uint32_t transformed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_pair32 generated_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    l3_authorization_secondary_pair32 cumulative_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13];
    uint32_t final_fold_lows13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local390_accum25b0_trace;

typedef struct l3_authorization_secondary_param3_local390_accum2530_25b0_trace {
    l3_authorization_secondary_param3_accum2530_trace accum2530;
    l3_authorization_secondary_local390_accum25b0_trace accum25b0;
} l3_authorization_secondary_param3_local390_accum2530_25b0_trace;

int l3_authorization_secondary_param3_accum2530_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_accum2530_trace *out);

int l3_authorization_secondary_local390_accum25b0_pairs13(
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_cumulative13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_local390_accum25b0_trace *out);

int l3_authorization_secondary_param3_local390_accum2530_25b0_pairs13(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 raw2530_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix2530_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 raw25b0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 prefix25b0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_param3_local390_accum2530_25b0_trace *out);


/* v296: post-param8 convolution/reduction network.
 * Consumes the V295 DAT_f41a2530 and DAT_f41a25b0 generated-pair
 * streams and emits the next local_328 image for the following
 * FUN_f3fdc45c(...) call.
 */
typedef struct l3_authorization_secondary_param3_local390_convolution26_trace {
    l3_authorization_secondary_pair32 convolution26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range2530_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 range25b0_26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 pre_reduce26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    l3_authorization_secondary_pair32 output26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26];
    uint32_t first_last_count26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][3];
} l3_authorization_secondary_param3_local390_convolution26_trace;

int l3_authorization_secondary_param3_local390_convolution26(
    const l3_authorization_secondary_pair32 raw2530_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix2530_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 raw25b0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    const l3_authorization_secondary_pair32 prefix25b0_13[L3_AUTH_ROUND_SECONDARY_PAIRS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_local390_convolution26_trace *out);

int l3_authorization_secondary_param3_local390_accum_convolution26(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_pair32 out_pairs26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    l3_authorization_secondary_param3_local390_convolution26_trace *out);

/* v297: bind the V296 local_328 image into the following FUN_f3fdc45c(...)
 * call and materialize the immediate native param_9 export.  This is the
 * terminal export seam at the end of the currently ported FUN_f3fcc2d0 tail.
 */
typedef struct l3_authorization_secondary_post_param3_local390_param9_trace {
    uint32_t input_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t param9_group0_4[4];
    uint32_t param9_group1_4[4];
    uint32_t param9_group2_4[4];
    uint32_t param9_tail_word12;
    uint32_t output_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_post_param3_local390_param9_trace;

typedef struct l3_authorization_secondary_bound_secondary_vector_reduce_param9_trace {
    l3_authorization_secondary_param3_local390_convolution26_trace convolution;
    uint32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t caller_scalar0;
    uint32_t caller_scalar1;
    uint32_t state_before_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    uint32_t state_after_f3fdc45c26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26][2];
    l3_authorization_secondary_vector_reduce_full_export_trace f3fdc45c;
    uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    l3_authorization_secondary_post_param3_local390_param9_trace param9_export;
} l3_authorization_secondary_bound_secondary_vector_reduce_param9_trace;

int l3_authorization_secondary_post_param3_local390_param9_words13(
    const uint32_t secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param9_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_post_param3_local390_param9_trace *out);

int l3_authorization_secondary_build_terminal_output_b(
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_param9_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_bound_secondary_vector_reduce_param9_trace *out);



/* v298: compose the finished FUN_f3fcc2d0 tail exports.  Native FUN_f3fcc2d0
 * ends by exporting param_8 and param_9 through two nearby FUN_f3fdc45c call
 * sites.  This adapter-facing wrapper keeps the two terminal exports together
 * while preserving the split V294 and V297 traces for audit/debug replay.
 */
typedef struct l3_authorization_secondary_terminal_param8_param9_trace {
    l3_authorization_secondary_bound_fold598_secondary_vector_reduce_param8_trace param8_tail;
    l3_authorization_secondary_bound_secondary_vector_reduce_param9_trace param9_tail;
    uint32_t param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t param9_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_terminal_param8_param9_trace;

int l3_authorization_secondary_build_terminal_outputs(
    const uint32_t local4c8_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t param4_194_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t local530_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const uint32_t carry_seed_words26[L3_AUTH_ROUND_SECONDARY_CONV_PAIRS26],
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    uint32_t out_param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param9_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_terminal_param8_param9_trace *out);


/* v328: compose the late upstream FUN_f3fcc2d0 local_42c and sixth
 * FUN_f3fdc45c export seams into the V325 vector preimage materializer.
 * This replaces two explicit caller-supplied preimage inputs with the native
 * local_3f8/local_390/param_1-derived production path; local_3c4 remains the
 * next open upstream seam. */
typedef struct l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_trace {
    l3_authorization_secondary_bound_fifth_secondary_vector_reduce_mix42c_trace local42c_trace;
    l3_authorization_secondary_bound_sixth_secondary_vector_reduce_export_trace sixth_trace;
    l3_authorization_secondary_vector_preimage_param7_local460_trace preimage_trace;
    uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t sixth_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local5b0_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
    uint32_t local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13];
} l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_trace;

int l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_param7_local460_words13(
    const uint32_t local3f8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const uint32_t local390_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t *param4_ctx_words,
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_seed_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t caller_scalar0,
    uint32_t caller_scalar1,
    const uint32_t local3c4_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local42c_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_sixth_secondary_vector_reduce_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local5b0_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_local460_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_trace *out);

/* Complete caller-visible FUN_f3fcc2d0 calculation.  caller_param5 and
 * caller_param6 are the two preinitialized stack arrays (FUN_f3fbc5c0
 * local_4ac/local_4e0); they are read but not modified.  The three outputs
 * correspond to native param_7, param_8, and param_9.
 */
int l3_authorization_secondary_run(
    const int32_t param1_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t param2_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t param3_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const void *parent_context,
    const int32_t caller_param5_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    const int32_t caller_param6_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param7_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param8_words13[L3_AUTH_ROUND_SECONDARY_WORDS13],
    uint32_t out_param9_words13[L3_AUTH_ROUND_SECONDARY_WORDS13]);

#ifdef __cplusplus
}
#endif

#endif
