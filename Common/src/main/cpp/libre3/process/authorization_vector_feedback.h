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
#ifndef L3_AUTHORIZATION_MATERIAL_VECTOR_FEEDBACK_H
#define L3_AUTHORIZATION_MATERIAL_VECTOR_FEEDBACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTH_VECTOR_FEEDBACK_WORDS13 13u
#define L3_AUTH_VECTOR_FEEDBACK_PAIRS13 13u
#define L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26 26u

typedef struct {
    uint32_t lo;
    uint32_t hi;
} l3_authorization_vector_feedback_pair32;

typedef struct {
    uint32_t transformed13[13];
    l3_authorization_vector_feedback_pair32 seed_after_first_affine13[13];
    l3_authorization_vector_feedback_pair32 folded2830_13[13];
    l3_authorization_vector_feedback_pair32 output_pairs13[13];
} l3_authorization_vector_feedback_seed2830_trace;


typedef struct {
    uint32_t base_pair[2];
    uint32_t lane_pair[2];
    uint32_t ladder28b0_pair[2];
    uint32_t second_pair[2];
    uint32_t scale;
    uint32_t vector_bias;
    uint32_t mul_bce_low;
    uint32_t mul_380_low;
} l3_authorization_vector_feedback_lane_scalars;

typedef struct {
    uint32_t scale_pair[2];
    uint32_t vector_bias_pair[2];
    uint32_t product_pairs12[12][2];
} l3_authorization_vector_feedback_vector_feedback_result;

typedef struct {
    uint32_t current_affine_pair[2];
    uint32_t ladder2930_pair[2];
    uint32_t post2930_pair[2];
    uint32_t ladder29b0_mix;
    uint32_t tail_product_pair[2];
    uint32_t final_lane1_pair[2];
    uint32_t final_lane12_pair[2];
} l3_authorization_vector_feedback_lane_tail_result;



typedef struct {
    l3_authorization_vector_feedback_pair32 input_pairs13[13];
    uint32_t affine_lo13[13];
    l3_authorization_vector_feedback_pair32 fold7_pairs13[13];
    uint32_t fold_final_mix13[13];
    uint32_t rolling_after13[13][2];
    uint32_t output_words13[13];
} l3_authorization_vector_feedback_export_trace;

typedef struct {
    l3_authorization_vector_feedback_seed2830_trace seed;
    l3_authorization_vector_feedback_pair32 seed_pairs13[13];
    l3_authorization_vector_feedback_lane_scalars lane_scalars[13];
    l3_authorization_vector_feedback_lane_tail_result lane_tails[13];
    l3_authorization_vector_feedback_pair32 state_before_lane13[13][26];
    l3_authorization_vector_feedback_pair32 state_after_lane13[13][26];
    l3_authorization_vector_feedback_pair32 final_state26[26];
} l3_authorization_vector_feedback_run13_trace;

typedef struct {
    l3_authorization_vector_feedback_run13_trace update;
    l3_authorization_vector_feedback_export_trace export_trace;
    uint32_t output_words13[13];
} l3_authorization_vector_feedback_run13_update_export_trace;


/* v304: first native block inside FUN_f3fdd33c.
 *
 * The native helper begins by expanding the 13-word seed vector at param_1
 * into a 13-pair stack image (local_c8..local_68).  Each lane is first
 * transformed through the DAT_f4191510 / DAT_f4191530 lane tables, then
 * folded through DAT_f41a2830, and finally reduced into the pair consumed
 * by the later vector-feedback loop.
 */
int l3_authorization_vector_feedback_seed2830_pairs13(
    const int32_t seed_words13[L3_AUTH_VECTOR_FEEDBACK_WORDS13],
    l3_authorization_vector_feedback_pair32 out_pairs13[L3_AUTH_VECTOR_FEEDBACK_PAIRS13],
    l3_authorization_vector_feedback_seed2830_trace *trace);



/* v305: native FUN_f3fdd33c vector-feedback update loop.
 * This consumes the v304 13-pair seed image together with the mutable
 * 26-pair param_5 state.  Each lane computes the scalar core, applies
 * the 12-pair vector feedback, then applies the scalar tail update to
 * lane+12 and the rolling lane+1 pair.
 */
int l3_authorization_vector_feedback_lane_scalar_core(
    uint32_t param3,
    uint32_t param4,
    const l3_authorization_vector_feedback_pair32 *rolling_pair,
    l3_authorization_vector_feedback_lane_scalars *out);

int l3_authorization_vector_feedback_vector_feedback_stores(
    const l3_authorization_vector_feedback_pair32 seed_pairs13[L3_AUTH_VECTOR_FEEDBACK_PAIRS13],
    const l3_authorization_vector_feedback_lane_scalars *lane_scalars,
    unsigned lane_index,
    l3_authorization_vector_feedback_pair32 state_pairs26[L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26],
    l3_authorization_vector_feedback_vector_feedback_result *out);

int l3_authorization_vector_feedback_lane_scalar_tail(
    const l3_authorization_vector_feedback_pair32 *seed_tail_pair,
    const l3_authorization_vector_feedback_lane_scalars *lane_scalars,
    unsigned lane_index,
    l3_authorization_vector_feedback_pair32 state_pairs26[L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26],
    l3_authorization_vector_feedback_lane_tail_result *out);

int l3_authorization_vector_feedback_run13_update(
    const int32_t seed_words13[L3_AUTH_VECTOR_FEEDBACK_WORDS13],
    uint32_t param3,
    uint32_t param4,
    l3_authorization_vector_feedback_pair32 state_pairs26[L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26],
    l3_authorization_vector_feedback_run13_trace *out);


/* v306: terminal native FUN_f3fdd33c export loop.
 * The native tail consumes the high half of the mutable param_5 state
 * (param_5 + 0x1a, represented here as state_pairs26[13..25]),
 * folds through DAT_f41a2a30, and writes the 13-word param_6 export
 * through DAT_f4191550 / DAT_f4191570.
 */
int l3_authorization_vector_feedback_export_words13(
    const l3_authorization_vector_feedback_pair32 state_pairs26[L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26],
    uint32_t out_words13[L3_AUTH_VECTOR_FEEDBACK_WORDS13],
    l3_authorization_vector_feedback_export_trace *trace);

int l3_authorization_vector_feedback_run13_update_export(
    const int32_t seed_words13[L3_AUTH_VECTOR_FEEDBACK_WORDS13],
    uint32_t param3,
    uint32_t param4,
    l3_authorization_vector_feedback_pair32 state_pairs26[L3_AUTH_VECTOR_FEEDBACK_STATE_PAIRS26],
    uint32_t out_words13[L3_AUTH_VECTOR_FEEDBACK_WORDS13],
    l3_authorization_vector_feedback_run13_update_export_trace *trace);

#ifdef __cplusplus
}
#endif

#endif
