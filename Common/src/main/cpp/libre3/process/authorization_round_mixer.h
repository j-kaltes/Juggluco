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
#ifndef L3_AUTHORIZATION_MATERIAL_MIXER_H
#define L3_AUTHORIZATION_MATERIAL_MIXER_H

#include <stdint.h>

#include "p256_coordinate_reduction.h"

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTH_ROUND_CORE_FC2304_WORDS 13u
#define L3_AUTH_ROUND_CORE_AUX_WORDS 12u
#define L3_AUTH_ROUND_CORE_AUX_BYTES 48u
#define L3_AUTH_ROUND_CORE_VEC_WORDS 13u
#define L3_AUTH_ROUND_CORE_PAIRS13 13u
#define L3_AUTH_ROUND_CORE_CONV_PAIRS26 26u

typedef struct {
    uint64_t high_halves2[2];
    uint64_t low_halves2[2];
    uint64_t middle_low_halves2[2];
} l3_authorization_vector_seed;

/* v207: exact affine output setup after the two FUN_f3fc2304 calls in
 * FUN_f3fbc5c0.  The left13/right13 inputs correspond to the native local_4ac
 * and local_4e0 13-word blocks.  parent_ctx must expose at least bytes
 * 0x3c..0x64 for the initial param_7 context affine.
 *
 * Outputs match the first straight-line native writes:
 *   param_5[0..0x2f]  -> aux48 words 0..11
 *   param_6[0..0x33]  -> vec_a13
 *   param_7[0..0x33]  -> vec_b13 initial affine seed
 */
int l3_authorization_core_pair_reduce_affine_setup(
    const int32_t left13[L3_AUTH_ROUND_CORE_FC2304_WORDS],
    const int32_t right13[L3_AUTH_ROUND_CORE_FC2304_WORDS],
    const uint8_t *parent_ctx,
    uint8_t aux48[L3_AUTH_ROUND_CORE_AUX_BYTES],
    int32_t vec_a13[L3_AUTH_ROUND_CORE_VEC_WORDS],
    int32_t vec_b13[L3_AUTH_ROUND_CORE_VEC_WORDS]);

/* v221: native-order pair of FUN_f3fc2304 calls followed by the v207
 * FUN_f3fbc5c0 affine setup.
 *
 * FUN_f3fbc5c0 invokes FUN_f3fc2304 twice without resetting q13/q14 in
 * scalar state.  The first call's final q13/q14 carry is therefore fed into
 * the second call.  The two 13-word outputs are then passed through the
 * existing affine setup boundary.
 */

/* V544: ARM-listing-faithful pair of FUN_f3fc2304 calls followed by the
 * affine setup.  The full parent context is supplied here; the native
 * param_7 seed begins at parent + 0x2a4, so this wrapper passes parent+0x268
 * to the older affine helper.  No vector-register state crosses either
 * FUN_f3fc2304 call. */
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
    int32_t vec_b13[L3_AUTH_ROUND_CORE_VEC_WORDS]);

/* V545: complete static FUN_f3fbc5c0 preamble, from the two generated 0x42
 * frames and the 35-byte scalar through point multiplication, both
 * FUN_f3fc2110 conversions, both native-order FUN_f3fc2304 calls, and the
 * initial param_5/param_6/param_7 affine writes.  All shared inputs are read
 * from their native full-parent offsets; there is no hidden register state.
 * The returned initial_param5_seed is the three-vector state consumed by the
 * first round's param_5 materializer. */
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
    l3_authorization_vector_seed *out_initial_param5_seed);



typedef struct {
    uint32_t lo;
    uint32_t hi;
} l3_authorization_word_pair;

typedef struct {
    l3_authorization_word_pair local514_raw2630[13];
    l3_authorization_word_pair local514_prefix2630[13];
    l3_authorization_word_pair round_raw26b0[13];
    l3_authorization_word_pair round_prefix26b0[13];
} l3_authorization_core_post_secondary_mix_accum_trace;

/* v299: first native loop seam after FUN_f3fc3c70/FUN_f3fcc2d0 in
 * FUN_f3fbc5c0.  The local514 stream is folded through DAT_f41a2630.
 * Lane 0 is scalar-headed; lanes 1..12 use UNK_f4191410/1430 first.
 */
int l3_authorization_core_local514_accum2630_pairs13(
    const int32_t local514[13],
    l3_authorization_word_pair raw2630[13],
    l3_authorization_word_pair prefix2630[13]);

/* v299 sibling stream: round-context words at native
 * param_1 + round*0x34 + 0x2cc are folded through DAT_f41a26b0.
 * Lane 0 is scalar-headed; lanes 1..12 use UNK_f4191450/1470 first.
 */
int l3_authorization_core_roundctx_accum26b0_pairs13(
    const int32_t round_words13[13],
    l3_authorization_word_pair raw26b0[13],
    l3_authorization_word_pair prefix26b0[13]);

int l3_authorization_core_post_secondary_mix_accum2630_26b0_pairs13(
    const int32_t local514[13],
    const int32_t round_words13[13],
    l3_authorization_word_pair raw2630[13],
    l3_authorization_word_pair prefix2630[13],
    l3_authorization_word_pair raw26b0[13],
    l3_authorization_word_pair prefix26b0[13],
    l3_authorization_core_post_secondary_mix_accum_trace *trace);


typedef struct {
    uint32_t first_last_count26[26][3];
    l3_authorization_word_pair convolution26[26];
    l3_authorization_word_pair range2630_26[26];
    l3_authorization_word_pair range26b0_26[26];
    l3_authorization_word_pair pre_reduce26[26];
    l3_authorization_word_pair output26[26];
} l3_authorization_core_post_secondary_mix_convolution26_trace;

/* v300: convolution/range-correction network consuming the v299
 * DAT_f41a2630 and DAT_f41a26b0 generated-pair streams.  The output
 * corresponds to the native local_370[0x34..0x67] / local_328 image.
 */
int l3_authorization_core_post_secondary_mix_convolution26(
    const l3_authorization_word_pair raw2630_13[13],
    const l3_authorization_word_pair prefix2630_13[13],
    const l3_authorization_word_pair raw26b0_13[13],
    const l3_authorization_word_pair prefix26b0_13[13],
    l3_authorization_word_pair out_pairs26[26],
    l3_authorization_core_post_secondary_mix_convolution26_trace *trace);

int l3_authorization_core_post_secondary_mix_accum_convolution26(
    const int32_t local514[13],
    const int32_t round_words13[13],
    l3_authorization_word_pair out_pairs26[26],
    l3_authorization_core_post_secondary_mix_convolution26_trace *trace);


typedef struct {
    l3_authorization_word_pair local5b0_raw2730[13];
    l3_authorization_word_pair local5b0_prefix2730[13];
    l3_authorization_word_pair round_raw27b0[13];
    l3_authorization_word_pair round_prefix27b0[13];
} l3_authorization_stage1_stage_accumulation_trace;

/* v301: next native loop seam after the v300 convolution image.  The
 * FUN_f3fcc2d0 local_5b0 export is folded through DAT_f41a2730.
 * Lane 0 is scalar-headed; lanes 1..12 use UNK_f4191490/14b0 first.
 */
int l3_authorization_core_local5b0_accum2730_pairs13(
    const int32_t local5b0[13],
    l3_authorization_word_pair raw2730[13],
    l3_authorization_word_pair prefix2730[13]);

/* v301 sibling stream: the native round-context window beginning at
 * param_1 + round*0x34 + 0xec8 is folded through DAT_f41a27b0.
 * Lane 0 is scalar-headed; lanes 1..12 use UNK_f41914d0/14f0 first.
 */
int l3_authorization_core_roundctx_accum27b0_pairs13(
    const int32_t round_words13[13],
    l3_authorization_word_pair raw27b0[13],
    l3_authorization_word_pair prefix27b0[13]);

int l3_authorization_core_post_accum2730_27b0_pairs13(
    const int32_t local5b0[13],
    const int32_t round_words13[13],
    l3_authorization_word_pair raw2730[13],
    l3_authorization_word_pair prefix2730[13],
    l3_authorization_word_pair raw27b0[13],
    l3_authorization_word_pair prefix27b0[13],
    l3_authorization_stage1_stage_accumulation_trace *trace);


typedef struct {
    uint32_t first_last_count26[26][3];
    l3_authorization_word_pair convolution26[26];
    l3_authorization_word_pair range2730_26[26];
    l3_authorization_word_pair range27b0_26[26];
    l3_authorization_word_pair pre_reduce26[26];
    l3_authorization_word_pair output26[26];
} l3_authorization_stage1_stage_convolution_trace;

/* v302: convolution/range-correction network consuming the v301
 * DAT_f41a2730 and DAT_f41a27b0 generated-pair streams.  The output
 * corresponds to the native local_370[0..0x33] image immediately before
 * the local_478/local_468/... vector materialization.
 */
int l3_authorization_stage1_stage_convolution26(
    const l3_authorization_word_pair raw2730_13[13],
    const l3_authorization_word_pair prefix2730_13[13],
    const l3_authorization_word_pair raw27b0_13[13],
    const l3_authorization_word_pair prefix27b0_13[13],
    l3_authorization_word_pair out_pairs26[26],
    l3_authorization_stage1_stage_convolution_trace *trace);

int l3_authorization_stage1_stage_accumulate_and_convolve26(
    const int32_t local5b0[13],
    const int32_t round_words13[13],
    l3_authorization_word_pair out_pairs26[26],
    l3_authorization_stage1_stage_convolution_trace *trace);


typedef struct {
    uint64_t input_low_halves13[13][2];
    uint64_t input_high_halves13[13][2];
    uint64_t side_from_high13[13][2];
    uint64_t side_from_low13[13][2];
    uint64_t bias13[13][2];
    uint64_t output_halves13[13][2];
    uint32_t vector_feedback_scalar_arg;
} l3_authorization_stage1_stage_vector_reduction_trace;

/* v303: vector materialization after the v302 low local_370[0..0x33]
 * convolution image.  The native block combines that low 26-pair image with
 * the carried high 26-pair image in local_370[0x34..0x67] and three
 * high-half seed vectors (local_4c0, local_4d0, auVar98) to form the
 * local_478/local_468/.../local_3b8 13-vector stream.  The output is stored
 * as 13 vectors, each represented by two native 64-bit halves.
 */
int l3_authorization_stage1_stage_vector_reduction13(
    const l3_authorization_word_pair low_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const l3_authorization_word_pair high_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26],
    const uint64_t local4c0_halves2[2],
    const uint64_t local4d0_halves2[2],
    const uint64_t auvar98_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage1_stage_vector_reduction_trace *trace);

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
    l3_authorization_stage1_stage_vector_reduction_trace *vec_trace);


typedef struct {
    l3_authorization_word_pair local548_raw2ab0[13];
    l3_authorization_word_pair local548_prefix2ab0[13];
    l3_authorization_word_pair round_raw2b30[13];
    l3_authorization_word_pair round_prefix2b30[13];
} l3_authorization_stage2_stage_accumulation_trace;

/* v308: post first-FUN_f3fdd33c/param_5 export accumulator seam inside
 * FUN_f3fbc5c0.  The local_548 stream is folded through DAT_f41a2ab0.
 * The sibling DAT_f41a2b30 stream is a mixed round-context input: lane 0 is
 * the native current-round word at +0x2cc, while lanes 1..12 are the twelve
 * words from the moving +0xe98 window. */
int l3_authorization_core_local548_accum2ab0_pairs13(
    const int32_t local548[13],
    l3_authorization_word_pair raw2ab0[13],
    l3_authorization_word_pair prefix2ab0[13]);

int l3_authorization_core_roundctx_accum2b30_pairs13(
    const int32_t round_mixed13[13],
    l3_authorization_word_pair raw2b30[13],
    l3_authorization_word_pair prefix2b30[13]);

int l3_authorization_core_post_accum2ab0_2b30_pairs13(
    const int32_t local548[13],
    const int32_t round_mixed13[13],
    l3_authorization_word_pair raw2ab0[13],
    l3_authorization_word_pair prefix2ab0[13],
    l3_authorization_word_pair raw2b30[13],
    l3_authorization_word_pair prefix2b30[13],
    l3_authorization_stage2_stage_accumulation_trace *trace);


typedef struct {
    uint32_t first_last_count26[26][3];
    l3_authorization_word_pair convolution26[26];
    l3_authorization_word_pair range2ab0_26[26];
    l3_authorization_word_pair range2b30_26[26];
    l3_authorization_word_pair pre_reduce26[26];
    l3_authorization_word_pair output26[26];
} l3_authorization_stage2_stage_convolution_trace;

/* v309: convolution/range-correction network consuming the v308
 * DAT_f41a2ab0 and DAT_f41a2b30 generated-pair streams.  The output is the
 * next native 26-pair local_370 high-half image after the first
 * FUN_f3fdd33c/param_5 export path. */
int l3_authorization_stage2_stage_convolution26(
    const l3_authorization_word_pair raw2ab0_13[13],
    const l3_authorization_word_pair prefix2ab0_13[13],
    const l3_authorization_word_pair raw2b30_13[13],
    const l3_authorization_word_pair prefix2b30_13[13],
    l3_authorization_word_pair out_pairs26[26],
    l3_authorization_stage2_stage_convolution_trace *trace);

int l3_authorization_stage2_stage_accumulate_and_convolve26(
    const int32_t local548[13],
    const int32_t round_mixed13[13],
    l3_authorization_word_pair out_pairs26[26],
    l3_authorization_stage2_stage_convolution_trace *trace);


/* v310: second post-FUN_f3fdd33c vector materialization after the V309
 * convolution image.  This is the same native rolling 13-lane shape as V303,
 * but with the second constant family and scalar argument used by the next
 * FUN_f3fdd33c call before the immediate param_6 export. */
typedef struct {
    uint64_t input_low_halves13[13][2];
    uint64_t input_high_halves13[13][2];
    uint64_t side_from_high13[13][2];
    uint64_t side_from_low13[13][2];
    uint64_t bias13[13][2];
    uint64_t output_halves13[13][2];
    uint32_t vector_feedback_scalar_arg;
} l3_authorization_stage2_stage_vector_reduction_trace;

int l3_authorization_stage2_stage_vector_reduction13(
    const l3_authorization_word_pair low_pairs26[26],
    const l3_authorization_word_pair high_pairs26[26],
    const uint64_t high_seed_halves2[2],
    const uint64_t low_seed_halves2[2],
    const uint64_t middle_low_seed_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage2_stage_vector_reduction_trace *trace);

int l3_authorization_stage2_stage_convolve_and_reduce_vectors13(
    const l3_authorization_word_pair high_pairs26[26],
    const int32_t local548[13],
    const int32_t round_mixed13[13],
    const uint64_t high_seed_halves2[2],
    const uint64_t low_seed_halves2[2],
    const uint64_t middle_low_seed_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage2_stage_convolution_trace *conv_trace,
    l3_authorization_stage2_stage_vector_reduction_trace *vec_trace);



typedef struct {
    l3_authorization_word_pair local5e4_raw2bb0[13];
    l3_authorization_word_pair local5e4_prefix2bb0[13];
    l3_authorization_word_pair roundmix_raw2c30[13];
    l3_authorization_word_pair roundmix_prefix2c30[13];
} l3_authorization_stage3_stage_accumulation_trace;

/* v311: generated-pair accumulator seam after the second FUN_f3fdd33c
 * binding / immediate param_6 export.  The local_5e4 stream is folded
 * through DAT_f41a2bb0.  Lane 0 is scalar-headed; lanes 1..12 use
 * UNK_f4191610/1630 first. */
int l3_authorization_core_local5e4_accum2bb0_pairs13(
    const int32_t local5e4[13],
    l3_authorization_word_pair raw2bb0[13],
    l3_authorization_word_pair prefix2bb0[13]);

/* v311 sibling stream: DAT_f41a2c30 uses a mixed round-context input.
 * mixed_round13[0] is the carried +0xec8 head word; mixed_round13[1..12]
 * are the moving +0x1a94 window words. */
int l3_authorization_core_roundmix_accum2c30_pairs13(
    const int32_t mixed_round13[13],
    l3_authorization_word_pair raw2c30[13],
    l3_authorization_word_pair prefix2c30[13]);

int l3_authorization_core_post_accum2bb0_2c30_pairs13(
    const int32_t local5e4[13],
    const int32_t mixed_round13[13],
    l3_authorization_word_pair raw2bb0[13],
    l3_authorization_word_pair prefix2bb0[13],
    l3_authorization_word_pair raw2c30[13],
    l3_authorization_word_pair prefix2c30[13],
    l3_authorization_stage3_stage_accumulation_trace *trace);


/* v312: convolution/range-correction network consuming the v311
 * DAT_f41a2bb0 and DAT_f41a2c30 generated-pair streams. */
typedef struct {
    uint32_t first_last_count26[26][3];
    l3_authorization_word_pair convolution26[26];
    l3_authorization_word_pair range2bb0_26[26];
    l3_authorization_word_pair range2c30_26[26];
    l3_authorization_word_pair pre_reduce26[26];
    l3_authorization_word_pair output26[26];
} l3_authorization_stage3_stage_convolution_trace;

int l3_authorization_stage3_stage_convolution26(
    const l3_authorization_word_pair raw2bb0_13[13],
    const l3_authorization_word_pair prefix2bb0_13[13],
    const l3_authorization_word_pair raw2c30_13[13],
    const l3_authorization_word_pair prefix2c30_13[13],
    l3_authorization_word_pair out_pairs26[26],
    l3_authorization_stage3_stage_convolution_trace *trace);

int l3_authorization_stage3_stage_accumulate_and_convolve26(
    const int32_t local5e4[13],
    const int32_t mixed_round13[13],
    l3_authorization_word_pair out_pairs26[26],
    l3_authorization_stage3_stage_convolution_trace *trace);


/* v313: third post-FUN_f3fdd33c vector materialization after the V312
 * convolution image.  Same rolling 13-lane local_478-style shape as V303/V310,
 * but with the third constant family before the immediate param_7 export. */
typedef struct {
    uint64_t input_low_halves13[13][2];
    uint64_t input_high_halves13[13][2];
    uint64_t side_from_high13[13][2];
    uint64_t side_from_low13[13][2];
    uint64_t bias13[13][2];
    uint64_t output_halves13[13][2];
    uint32_t vector_feedback_scalar_arg;
} l3_authorization_stage3_stage_vector_reduction_trace;

int l3_authorization_stage3_stage_vector_reduction13(
    const l3_authorization_word_pair low_pairs26[26],
    const l3_authorization_word_pair high_pairs26[26],
    const uint64_t high_seed_halves2[2],
    const uint64_t low_seed_halves2[2],
    const uint64_t middle_low_seed_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage3_stage_vector_reduction_trace *trace);

int l3_authorization_stage3_stage_convolve_and_reduce_vectors13(
    const l3_authorization_word_pair high_pairs26[26],
    const int32_t local5e4[13],
    const int32_t mixed_round13[13],
    const uint64_t high_seed_halves2[2],
    const uint64_t low_seed_halves2[2],
    const uint64_t middle_low_seed_halves2[2],
    uint64_t out_halves13[13][2],
    uint32_t *out_vector_feedback_scalar_arg,
    l3_authorization_stage3_stage_convolution_trace *conv_trace,
    l3_authorization_stage3_stage_vector_reduction_trace *vec_trace);

/* Native high convolution image for the param_7 FUN_f3fdd33c call. */
int l3_authorization_core_param7_high_accum_convolution26(
    const int32_t local57c[13],
    const int32_t round_2cc_words13[13],
    l3_authorization_word_pair out_pairs26[L3_AUTH_ROUND_CORE_CONV_PAIRS26]);



/* v326: post-param_7 carried local_618 accumulator seam after the completed
 * FUN_f3fcc2d0 terminal path.  Native FUN_f3fbc5c0 folds the carried
 * local_618 export through DAT_f41a2db0 and a live +0xec8 round-context
 * sibling through DAT_f41a2e30 before the next local_478 materialization. */
typedef struct {
    l3_authorization_word_pair local618_raw2db0[13];
    l3_authorization_word_pair local618_prefix2db0[13];
    l3_authorization_word_pair roundctx_raw2e30[13];
    l3_authorization_word_pair roundctx_prefix2e30[13];
} l3_authorization_stage4_stage_accumulation_trace;

int l3_authorization_core_local618_accum2db0_pairs13(
    const int32_t local618[13],
    l3_authorization_word_pair raw2db0[13],
    l3_authorization_word_pair prefix2db0[13]);

int l3_authorization_core_roundctx_accum2e30_pairs13(
    const int32_t round_ec8_words13[13],
    l3_authorization_word_pair raw2e30[13],
    l3_authorization_word_pair prefix2e30[13]);

int l3_authorization_core_post_accum2db0_2e30_pairs13(
    const int32_t local618[13],
    const int32_t round_ec8_words13[13],
    l3_authorization_word_pair raw2db0[13],
    l3_authorization_word_pair prefix2db0[13],
    l3_authorization_word_pair raw2e30[13],
    l3_authorization_word_pair prefix2e30[13],
    l3_authorization_stage4_stage_accumulation_trace *trace);

/* v326: convolution/range-correction network consuming the v326
 * DAT_f41a2db0 and DAT_f41a2e30 generated-pair streams. */
typedef struct {
    uint32_t first_last_count26[26][3];
    l3_authorization_word_pair convolution26[26];
    l3_authorization_word_pair range2db0_26[26];
    l3_authorization_word_pair range2e30_26[26];
    l3_authorization_word_pair pre_reduce26[26];
    l3_authorization_word_pair output26[26];
} l3_authorization_stage4_stage_convolution_trace;

int l3_authorization_stage4_stage_convolution26(
    const l3_authorization_word_pair raw2db0_13[13],
    const l3_authorization_word_pair prefix2db0_13[13],
    const l3_authorization_word_pair raw2e30_13[13],
    const l3_authorization_word_pair prefix2e30_13[13],
    l3_authorization_word_pair out_pairs26[26],
    l3_authorization_stage4_stage_convolution_trace *trace);

int l3_authorization_stage4_stage_accumulate_and_convolve26(
    const int32_t local618[13],
    const int32_t round_ec8_words13[13],
    l3_authorization_word_pair out_pairs26[26],
    l3_authorization_stage4_stage_convolution_trace *trace);

#ifdef __cplusplus
}
#endif

#endif
