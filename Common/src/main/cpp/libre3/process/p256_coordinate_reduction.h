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
#ifndef L3_GENERATED_COORDINATE_REDUCTION_H
#define L3_GENERATED_COORDINATE_REDUCTION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTH_PAIR_REDUCE_LEFT_WORDS 10u
#define L3_AUTH_PAIR_REDUCE_RIGHT_WORDS 13u
#define L3_AUTH_PAIR_REDUCE_CONV_WORDS 26u
#define L3_AUTH_PAIR_REDUCE_PARAM3_WORDS 13u
#define L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS 13u
#define L3_AUTH_PAIR_REDUCE_REDUCTION_STACK_PAIRS 62u

/* v208: literal seed/prefix front-end of FUN_f3fc2304.
 *
 * Native FUN_f3fc2304 first builds two 64-bit pair streams before the large
 * convolution/reduction body:
 *   - param_1[0..9] through the DAT_f41a0278 ladder, with cumulative prefixes
 *     stored at auStack_210[0x36..0x49].
 *   - param_2[0..12] through the DAT_f41a02f8 ladder, with cumulative prefixes
 *     stored at auStack_210[0x1c..0x35].
 *
 * The raw arrays receive the per-lane generated pairs.  The prefix arrays
 * receive native little-endian 64-bit cumulative sums as {lo, hi} words.
 */
int l3_p256_coordinate_reduce_seed_prefixes(
    const int32_t left10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS],
    const int32_t right13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS],
    uint32_t left_raw10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS][2],
    uint32_t left_prefix10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS][2],
    uint32_t right_raw13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS][2],
    uint32_t right_prefix13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS][2]);


/* v209: native convolution/front-reduction stage of FUN_f3fc2304.
 *
 * This consumes the raw/prefix pair streams produced by
 * l3_p256_coordinate_reduce_seed_prefixes() and emits the 26 native little-endian
 * 64-bit local_270 pairs.  The formula is the straight-line decompiler
 * block after the two seed-prefix loops, including the native default value
 * for lanes with no overlap.
 */
int l3_p256_coordinate_reduce_convolution_pairs26(
    const uint32_t left_raw10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS][2],
    const uint32_t left_prefix10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS][2],
    const uint32_t right_raw13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS][2],
    const uint32_t right_prefix13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS][2],
    uint32_t out_pairs26[L3_AUTH_PAIR_REDUCE_CONV_WORDS][2]);



/* v210: literal FUN_f3fc2304 param_3 seed-pair stage.
 *
 * After the 26-pair local_270 convolution/front-reduction stage, native
 * FUN_f3fc2304 builds a 13-pair stream from param_3 using DAT_f4190270 /
 * DAT_f4190290 lane tokenization and the DAT_f41a0378 nibble ladder.  This
 * helper emits the native auStack_210[0x4a..0x63] pair image as {lo, hi}.
 */
int l3_p256_coordinate_reduce_param3_pairs13(
    const int32_t param3[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS],
    uint32_t out_pairs13[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS][2]);


/* v211: scalar DAT_f41a0478/DAT_f41a04f8 ladder core inside the
 * post-param_3 FUN_f3fc2304 reduction loop. This is the straight-line scalar
 * transform between the updated local_270 pair and the subsequent vector-lane
 * feedback term; it deliberately does not model the surrounding NEON stores. */
int l3_p256_coordinate_reduce_param3_scalar_ladder_core(
    const uint32_t local_pair[2],
    uint32_t mid_pair[2],
    uint32_t low_ladder_out[2]);



/* v213: scalar param_5/param_6 seed core in FUN_f3fc2304.
 *
 * This ports the straight-line block after local_270[0] is available and
 * before the param_3/vector-lane accumulation loop.  It combines param_5,
 * param_6, and one local_270 pair; runs the DAT_f41a03f8 ladder; and emits
 * the native base pair, factor pair, and lane-add pair consumed by the next
 * reduction stage.
 */
int l3_p256_coordinate_reduce_param56_reduction_seed_core(
    uint32_t param5,
    uint32_t param6,
    const uint32_t local_pair[2],
    uint32_t base_pair[2],
    uint32_t factor_pair[2],
    uint32_t add_pair[2]);


/* v214: scalar lane-store/update core in the FUN_f3fc2304 13-lane
 * reduction loop. This ports the native scalar pair updates around the
 * NEON/vector lane stores: current local_270 slot, aux238 slot, and
 * auStack_210 output-state slot. It deliberately leaves the later vector
 * feedback state that produces the next rolling local_270[0] pair outside
 * this helper. */
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
    uint32_t out_state_pair[2]);



/* v215: scalar feedback update for the next rolling local_270[0] pair in
 * the FUN_f3fc2304 13-lane reduction loop. This ports the native block after
 * the DAT_f41a0478/DAT_f41a04f8 scalar ladders and after the lane-vector
 * accumulator has produced auVar32._0_8_. */
int l3_p256_coordinate_reduce_next_rolling_pair_core(
    const uint32_t mid_pair[2],
    const uint32_t low_ladder_out[2],
    const uint32_t lane_acc_pair[2],
    uint32_t out_next_pair[2]);



/* v216: first native vector feedback producer in the FUN_f3fc2304
 * 13-lane reduction loop.  This ports the low-16-bit lane-product/mask
 * vector updates that produce the lane accumulator consumed by the v215
 * next-rolling-pair core.  Vectors are represented as two native 64-bit
 * lanes, matching the decompiler's q-register _0_8_/_8_8_ view. */



/* v217: decompiler-shaped vector feedback producer in the FUN_f3fc2304
 * 13-lane reduction loop.  This API is retained for the historical V217-V221
 * regression suite.  The ARM listing shows that q13/q14 are rebuilt inside
 * every iteration rather than carried into the function or between lanes;
 * new code should use the native-order V544 entry points below. */



/* v218: native-order full reduction lane step for FUN_f3fc2304.
 *
 * This stitches the already ported v213..v217 boundaries in the same order as
 * the native loop body: param5/param6 seed, scalar lane stores, DAT_f41a0478 /
 * DAT_f41a04f8 ladder, vector feedback stages, q13/q14 carry, and the final
 * overwrite of the current auStack_268 64-bit lane with the next rolling
 * local_270[0] pair.  Vector windows are represented as two native 64-bit
 * lanes. */


/* v219: historical decompiler-shaped 13-lane reduction driver.
 *
 * This helper installs the 26-pair convolution image and the 13 param_3
 * coefficient pairs into the same overlapping pair indices used by the native
 * stack frame, then runs the v218 lane step for all 13 lanes.  Its explicit
 * q13/q14 inputs are retained for regression compatibility; they are not
 * native function inputs.  The emitted state_pairs13 are the
 * exact pair slots consumed by the v212 output-compression loop: auStack_210
 * pair slots 1..13, i.e. native stack pair indices 13..25.
 */

/* V544: ARM-listing-faithful 13-lane reduction driver.
 *
 * Native FUN_f3fc2304 constructs q13/q14 inside each lane from the current
 * add/factor pair and the 13 param_3 coefficient pairs.  It also consumes the
 * coefficient image as one scalar pair, three two-lane vectors, one scalar
 * pair, two two-lane vectors, and one final scalar pair:
 *
 *   49 | 50..51 | 52..53 | 54..55 | 56 | 57..58 | 59..60 | 61
 *
 * This entry point follows that layout directly and has no hidden register
 * inputs or outputs.  out_state_pairs13 is the native stack[13..25] image
 * consumed by the final output-compression loop.
 */

/* Exact native stack-layout variant.  FUN_f3fc2304 retains the 13 right
 * prefixes and 10 left prefixes in stack pairs 26..48 while reducing. */
int l3_p256_coordinate_reduce_reduction_run13_native_seeded(
    uint32_t param5,
    uint32_t param6,
    const uint32_t conv_pairs26[L3_AUTH_PAIR_REDUCE_CONV_WORDS][2],
    const uint32_t retained_prefix_pairs23[23][2],
    const uint32_t param3_pairs13[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS][2],
    uint32_t out_state_pairs13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS][2],
    uint32_t out_next_rolling_pair[2],
    uint32_t out_stack_pairs[L3_AUTH_PAIR_REDUCE_REDUCTION_STACK_PAIRS][2]);

/* Differential-oracle variant of the native-order reduction.  Each trace
 * record is the complete 62-pair stack image after one native lane. */

/* v212: final FUN_f3fc2304 output compression loop.
 *
 * After the post-param_3 vector/reduction loop has updated the native
 * auStack_210 pair state, FUN_f3fc2304 skips pair slot 0 and compresses
 * pair slots 1..13 through the DAT_f41a0578 nibble ladder and the
 * DAT_f41902b0/DAT_f41902d0 lane affine table to produce param_7[0..12].
 * This helper ports that final scalar output loop exactly for a supplied
 * 13-pair image of those consumed slots.
 */
int l3_p256_coordinate_reduce_output_words13_from_state_pairs13(
    const uint32_t state_pairs13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS][2],
    uint32_t out_words13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS]);


/* v220: historical callable FUN_f3fc2304 words13 pipeline.
 *
 * Wires the v208 seed-prefix front-end, v209 convolution image, v210
 * param_3 coefficient image, v219 13-lane reduction driver, and v212
 * output-compression loop into one boundary.  The explicit q13/q14 inputs
 * reflect the older decompiler model and remain for regression compatibility.
 */

/* V544: complete callable FUN_f3fc2304 pipeline reconstructed from the ARM
 * instruction stream.  All vector temporaries are derived internally. */
int l3_p256_coordinate_reduce_full_words13_native(
    const int32_t left10[L3_AUTH_PAIR_REDUCE_LEFT_WORDS],
    const int32_t right13[L3_AUTH_PAIR_REDUCE_RIGHT_WORDS],
    const int32_t param3[L3_AUTH_PAIR_REDUCE_PARAM3_WORDS],
    uint32_t param5,
    uint32_t param6,
    uint32_t out_words13[L3_AUTH_PAIR_REDUCE_OUTPUT_WORDS]);

/* v221: historical FUN_f3fc2304 pipeline with modeled q13/q14 export.
 *
 * Retained for the V221 regression suite.  The ARM listing shows the next
 * invocation overwrites these temporaries before reading them.
 */

#ifdef __cplusplus
}
#endif

#endif
