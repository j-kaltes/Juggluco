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
#ifndef L3_AUTHORIZATION_MATERIAL_VECTOR_REDUCE_H
#define L3_AUTHORIZATION_MATERIAL_VECTOR_REDUCE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTH_VECTOR_REDUCE_SEED_WORDS 13u
#define L3_AUTH_VECTOR_REDUCE_STATE_PAIRS 26u

typedef struct l3_authorization_vector_reduce_lane_scalars {
    uint32_t base_pair[2];
    uint32_t lane_pair[2];
    uint32_t ladder7_pair[2];
    uint32_t second_pair[2];
    uint32_t scale;
    uint32_t vector_bias;
    uint32_t mul34_low;
    uint32_t mul6e_low;
} l3_authorization_vector_reduce_lane_scalars;

typedef struct l3_authorization_vector_reduce_vector_feedback_result {
    uint32_t scale_pair[2];
    uint32_t vector_bias_pair[2];
    uint32_t product_pairs12[12][2];
} l3_authorization_vector_reduce_vector_feedback_result;




typedef struct l3_authorization_vector_reduce_lane_tail_result {
    uint32_t current_affine_pair[2];
    uint32_t ladder0870_pair[2];
    uint32_t post0870_pair[2];
    uint32_t ladder08f0_mix;
    uint32_t tail_product_pair[2];
    uint32_t final_lane1_pair[2];
    uint32_t final_lane12_pair[2];
} l3_authorization_vector_reduce_lane_tail_result;


typedef struct l3_authorization_vector_reduce_run13_trace {
    uint32_t seed_pairs13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS][2];
    l3_authorization_vector_reduce_lane_scalars lane_scalars[L3_AUTH_VECTOR_REDUCE_SEED_WORDS];
    uint32_t vector_store_states13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS]
                                         [L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2];
    l3_authorization_vector_reduce_lane_tail_result lane_tails[L3_AUTH_VECTOR_REDUCE_SEED_WORDS];
    uint32_t lane_done_states13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS]
                                      [L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2];
} l3_authorization_vector_reduce_run13_trace;


typedef struct l3_authorization_vector_reduce_export_trace {
    uint32_t export_inputs13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS][2];
    uint32_t pre_export_word13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS];
    uint32_t ladder7_pair13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS][2];
    uint32_t low_after15_13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS];
    uint32_t rolling_pair_after13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS][2];
} l3_authorization_vector_reduce_export_trace;

typedef struct l3_authorization_vector_reduce_full_export_trace {
    l3_authorization_vector_reduce_run13_trace run13;
    l3_authorization_vector_reduce_export_trace export_trace;
} l3_authorization_vector_reduce_full_export_trace;

/* v224: first bounded FUN_f3fdb57c helper.
 *
 * FUN_f3fc3c70 calls FUN_f3fdb57c immediately after building the first
 * local_578 26-pair convolution image.  FUN_f3fdb57c begins by deriving a
 * 13-pair local_c8 coefficient/seed schedule from its param_1 pointer before
 * the NEON/vector feedback loop mutates param_5 and writes param_6.
 *
 * This helper ports that initial seed schedule only.  Pairs are returned in
 * native little-endian {lo32, hi32} order.
 */
int l3_authorization_vector_reduce_seed_pairs13(
    const int32_t param1_13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS],
    uint32_t out_seed_pairs13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS][2]);

/* v225: scalar front half of one FUN_f3fdb57c update lane.
 *
 * This is the native scalar block following the v224 local_c8 seed schedule:
 * it binds param_3/param_4 and the current rolling param_5 pair, runs the
 * DAT_f41a07f0 seven-nibble ladder, and emits the multiplier/bias words used
 * by the subsequent q-register vector feedback stores.  It intentionally does
 * not model those vector stores or the later DAT_f41a0870/DAT_f41a08f0 tail.
 */
int l3_authorization_vector_reduce_lane_scalar_core(
    uint32_t param3,
    uint32_t param4,
    const uint32_t rolling_pair[2],
    l3_authorization_vector_reduce_lane_scalars *out);

/* v226: q-register vector-feedback stores for one FUN_f3fdb57c lane.
 *
 * This consumes the v224 local_c8 seed schedule and the v225 lane scalar
 * values.  It reconstructs the full 64-bit scale and vector-bias pairs that
 * the native code keeps split across ARM core registers, multiplies the first
 * twelve local_c8 64-bit seed pairs by the scale pair, then applies the native
 * q5 + product update to the overlapping param_5 window at lane..lane+11.
 * The scalar DAT_f41a0870 / DAT_f41a08f0 tail that updates lane+1 and
 * lane+12 is intentionally left outside this boundary.
 */
int l3_authorization_vector_reduce_vector_feedback_stores(
    const uint32_t seed_pairs13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS][2],
    const l3_authorization_vector_reduce_lane_scalars *lane_scalars,
    unsigned lane_index,
    uint32_t state_pairs26[L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2],
    l3_authorization_vector_reduce_vector_feedback_result *out);


/* v227: scalar tail for one FUN_f3fdb57c lane after vector-feedback stores.
 *
 * Native code next consumes the just-updated current param_5 lane, runs the
 * DAT_f41a0870 and DAT_f41a08f0 ladder/tail block, adds the local_c8[12]
 * scale/bias contribution into state[lane+12], and overwrites the high/low
 * words of state[lane+1].  This helper intentionally models one lane only;
 * the 13-lane driver and final param_6 export remain outside this boundary.
 */
int l3_authorization_vector_reduce_lane_scalar_tail(
    const uint32_t seed_tail_pair[2],
    const l3_authorization_vector_reduce_lane_scalars *lane_scalars,
    unsigned lane_index,
    uint32_t state_pairs26[L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2],
    l3_authorization_vector_reduce_lane_tail_result *out);



/* v228: full 13-lane FUN_f3fdb57c update loop before final param_6 export.
 *
 * This stitches v224-v227 in native order: derive local_c8 seed pairs once,
 * then run thirteen overlapping param_5 lanes.  Each lane uses the current
 * state[lane] pair as the rolling input, applies the vector-feedback stores to
 * state[lane..lane+11], applies the scalar tail to state[lane+12] and
 * state[lane+1], then advances by one 64-bit pair.  The trailing param_6
 * export loop remains a separate unresolved boundary.
 */
int l3_authorization_vector_reduce_run13_update(
    const int32_t param1_13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS],
    uint32_t param3,
    uint32_t param4,
    uint32_t state_pairs26[L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2],
    l3_authorization_vector_reduce_run13_trace *out);


/* v229: final FUN_f3fdb57c param_6 export loop.
 *
 * Native FUN_f3fdb57c finishes by reading the high half of the mutated
 * param_5 image, starting at pair 13, and writing thirteen 32-bit param_6
 * words.  This helper ports that terminal loop, including the rolling
 * 64-bit affine state, the DAT_f41a0970 nibble ladder, and the final
 * DAT_f4190450/DAT_f4190470 lane affine table.
 */
int l3_authorization_vector_reduce_export_words13(
    const uint32_t state_pairs26[L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2],
    uint32_t out_words13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS],
    l3_authorization_vector_reduce_export_trace *out);

/* v229: complete bounded FUN_f3fdb57c body.
 *
 * This composes the v228 13-lane update with the v229 param_6 export.
 * It still represents FUN_f3fdb57c as a standalone native boundary; callers
 * such as FUN_f3fc3c70 must provide the surrounding convolution image.
 */
int l3_authorization_vector_reduce_run13_update_export(
    const int32_t param1_13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS],
    uint32_t param3,
    uint32_t param4,
    uint32_t state_pairs26[L3_AUTH_VECTOR_REDUCE_STATE_PAIRS][2],
    uint32_t out_words13[L3_AUTH_VECTOR_REDUCE_SEED_WORDS],
    l3_authorization_vector_reduce_full_export_trace *out);

#ifdef __cplusplus
}
#endif

#endif
