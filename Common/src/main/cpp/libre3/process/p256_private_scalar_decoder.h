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
#ifndef L3_PRIVATE_SCALAR_TAIL_DECODER_H
#define L3_PRIVATE_SCALAR_TAIL_DECODER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS 10u
#define L3_AUTH_SCALAR_TAIL_TAIL_LEN 0x23u
#define L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS 20u

/* Exact final lane-affine + 10x28-bit writer at the end of FUN_f3fbab70.
 * Input is the native auStack_1d8[0x42..0x4b] word vector immediately before
 * the final DAT_f41901b0/DAT_f41901d0 affine map.  This helper does not port
 * the upstream generation of those ten words. */

/* Exact final pre-affine 10-iteration loop of FUN_f3fbab70.
 * Input is auStack_1d8[6..25], the 10 native 64-bit accumulator pairs
 * immediately before the DAT_f41a01f8 nibble-ladder loop that writes
 * auStack_1d8[0x42..0x4b]. */
void l3_p256_private_scalar_lastloop_pairs20_to_words10(
    const uint32_t pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS]);

void l3_p256_private_scalar_lastloop_pairs20_to_tail35(
    const uint32_t pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint8_t out35[L3_AUTH_SCALAR_TAIL_TAIL_LEN]);

void l3_p256_private_scalar_final_words10_to_tail35(
    const uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint8_t out35[L3_AUTH_SCALAR_TAIL_TAIL_LEN]);

void l3_p256_private_scalar_final_words10_to_lanes10(
    const uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t lanes10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS]);

/* Exact recurring 8-step seed-pair generators used by FUN_f3fbab70 before the
 * remaining accumulator-pair generation.  These correspond to the DAT_f419fef8
 * and DAT_f419ff78 nibble-ladder blocks and return the native little-endian
 * 64-bit pair as two uint32_t words. */
void l3_p256_private_scalar_seed_stream_a_pair(uint32_t x, uint32_t out_pair2[2]);
void l3_p256_private_scalar_seed_stream_b_pair(uint32_t x, uint32_t out_pair2[2]);

/* Exact DAT_f419fff8 seed-pair generator and the ten-word schedule over
 * DAT_f4190130/DAT_f4190150 used by FUN_f3fbab70 before the larger
 * accumulator convolution. */
/* Exact native seed and cumulative-prefix schedules used at the start of
 * FUN_f3fbab70.  These model the two 10-entry 64-bit prefix vectors built
 * from DAT_f419fef8 and DAT_f419ff78 before the convolution body.  Pass NULL
 * for either output vector if only seeds or only prefixes are needed. */
void l3_p256_private_scalar_build_prefix_stream_a(
    const uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t out_seed_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t out_prefix_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS]);

void l3_p256_private_scalar_build_prefix_stream_b(
    const uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t out_seed_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t out_prefix_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS]);


/* Exact convolution body that combines the native fef8/ff78 seed and prefix
 * schedules into the 23 64-bit local_228 pairs consumed by the later
 * FUN_f3fbab70 reduction stage.  This ports the decompiled loop that runs
 * for uVar8=0..0x16 immediately after the seed-prefix schedule setup. */
#define L3_AUTH_SCALAR_TAIL_CONV_PAIRS 23u
void l3_p256_private_scalar_convolution_pairs23(
    const uint32_t fef8_seed_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    const uint32_t ff78_seed_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    const uint32_t fef8_prefix_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    const uint32_t ff78_prefix_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t out_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u]);


/* Exact first scalar core of the FUN_f3fbab70 post-convolution reduction.
 * This is the native DAT_f41a0078 nibble-ladder block that combines the
 * param_1+0x1c70 base pair with one local_228 convolution pair and produces
 * the two 64-bit scalar pairs consumed by the later vector accumulation stage
 * in the 13-iteration reduction loop. */
void l3_p256_private_scalar_reduction_seed_core(
    const uint32_t base_pair2[2],
    const uint32_t conv_pair2[2],
    uint32_t out_mul_pair2[2],
    uint32_t out_add_pair2[2]);


/* Exact scalar feedback half of the FUN_f3fbab70 post-convolution reduction
 * loop.  This covers the native multiply-accumulate update of one local_228
 * pair followed by the DAT_f41a00f8 and DAT_f41a0178 nibble-ladder chain that
 * seeds the next local_228 feedback pair.  The surrounding NEON vector-lane
 * accumulation is still ported separately. */
void l3_p256_private_scalar_reduction_feedback_core(
    const uint32_t current_pair2[2],
    const uint32_t add_pair2[2],
    const uint32_t mul_pair2[2],
    const uint32_t coeff_pair2[2],
    const uint32_t feedback_pair2[2],
    uint32_t out_updated_pair2[2],
    uint32_t out_feedback_pair2[2]);


/* v200: exact scalar rolling shell around the v198/v199 reduction pieces.
 * This iterates the native base-pair transform, fixed low coefficient pair,
 * and caller-supplied vector feedback carrier pairs over the first 13
 * reduction slots.  The caller-supplied feedback carriers correspond to the
 * still-separate NEON/vector-lane accumulation outputs used by the native
 * FUN_f3fbab70 loop. */
#define L3_AUTH_SCALAR_TAIL_REDUCTION_ITERS 13u


/* v201: native-shaped feedback-carrier derivation for the FUN_f3fbab70
 * post-convolution reduction.  This removes the v200 caller-supplied carrier
 * placeholder by deriving the 13 carrier pairs from the rolling local_228
 * state and all ten DAT_f419fff8 coefficient pairs. */



/* v202: native-layout handoff from the post-convolution reduction state to the
 * already ported final FUN_f3fbab70 last-loop input.  In the native stack
 * layout, auStack_1d8[6..25] aliases local_228 pair slots 13..22.  This
 * helper runs the v197/v201 reduction state machine and exposes those ten
 * pairs directly, retiring the digest-fabricated accumulator-pair handoff for
 * callers that can provide the 10 upstream words and base pair. */

/* v203: single native-layout reduction slot machine.  This exposes the whole
 * local_228[0..22] image after the 13 post-convolution reduction iterations
 * and centralizes the carrier/update logic previously duplicated by the v201
 * carrier helper and the v202 slots13 handoff. */
void l3_p256_private_scalar_reduction_slots23_state(
    const uint32_t base_pair2[2],
    const uint32_t initial_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u],
    const uint32_t coeff_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t out_final_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u]);

void l3_p256_private_scalar_reduction_trace13_state(
    const uint32_t base_pair2[2],
    const uint32_t initial_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u],
    const uint32_t coeff_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS],
    uint32_t out_feedback_carrier_pairs26[L3_AUTH_SCALAR_TAIL_REDUCTION_ITERS * 2u],
    uint32_t out_updated_pairs26[L3_AUTH_SCALAR_TAIL_REDUCTION_ITERS * 2u],
    uint32_t out_next_pairs26[L3_AUTH_SCALAR_TAIL_REDUCTION_ITERS * 2u],
    uint32_t out_final_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u]);




/* v205: native-shaped FUN_f3fbab70 stream split.  The native function does not
 * use one 10-word vector for all upstream schedules: fef8 words come from the
 * param_2-derived object frame, ff78 words come from parent_ctx+0x1c80, and
 * fff8 coefficient words come from parent_ctx+0x1ac8.  These helpers expose
 * that three-stream boundary before the shared convolution/reduction/tail path. */
void l3_p256_private_scalar_build_convolution_from_streams(
    const uint32_t fef8_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    const uint32_t ff78_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t out_pairs46[L3_AUTH_SCALAR_TAIL_CONV_PAIRS * 2u]);

void l3_p256_private_scalar_streams10_to_reduction_pairs20_boundary(
    const uint32_t base_pair2[2],
    const uint32_t fef8_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    const uint32_t ff78_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    const uint32_t coeff_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t out_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS]);

void l3_p256_private_scalar_streams10_to_tail35_reduction_boundary(
    const uint32_t base_pair2[2],
    const uint32_t fef8_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    const uint32_t ff78_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    const uint32_t coeff_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint8_t out35[L3_AUTH_SCALAR_TAIL_TAIL_LEN]);

void l3_p256_private_scalar_coefficient_stream_pair(uint32_t x, uint32_t out_pair2[2]);
void l3_p256_private_scalar_build_coefficient_schedule(
    const uint32_t words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t out_pairs20[L3_AUTH_SCALAR_TAIL_LASTLOOP_INPUT_WORDS]);


/* v206: exact native input stream extractors for FUN_f3fbab70.
 * param2_frame must contain param_2[0], param_2[1], and the first two words
 * of param_2[2] (0x28 bytes total).  This reconstructs the 10-word
 * param_2-derived stream used by the DAT_f419fef8 schedule: the two NEON
 * vmla.i32-generated 4-word blocks plus local_230/local_22c.
 *
 * parent_ctx extractors mirror the native offsets used by FUN_f3fbab70:
 *   base_pair2    = param_1 + 0x1c70
 *   ff78_words10  = param_1 + 0x1c80
 *   coeff_words10 = param_1 + 0x1ac8
 */
#define L3_AUTH_SCALAR_TAIL_PARAM2_FRAME_LEN 0x28u
#define L3_AUTH_SCALAR_TAIL_PARENT_CTX_MIN_LEN 0x1ca8u

int l3_p256_private_scalar_decode_frame_words(
    const uint8_t param2_frame[L3_AUTH_SCALAR_TAIL_PARAM2_FRAME_LEN],
    uint32_t fef8_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS]);

int l3_p256_private_scalar_extract_parent_streams(
    const uint8_t *parent_ctx,
    uint32_t base_pair2[2],
    uint32_t ff78_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS],
    uint32_t coeff_words10[L3_AUTH_SCALAR_TAIL_FINAL_INPUT_WORDS]);

int l3_p256_decode_app_private_scalar35(
    const uint8_t *parent_ctx,
    const uint8_t param2_frame[L3_AUTH_SCALAR_TAIL_PARAM2_FRAME_LEN],
    uint8_t out35[L3_AUTH_SCALAR_TAIL_TAIL_LEN]);

#ifdef __cplusplus
}
#endif

#endif
