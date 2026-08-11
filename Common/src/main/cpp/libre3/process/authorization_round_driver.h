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
#ifndef L3_AUTHORIZATION_MATERIAL_ROUNDS_H
#define L3_AUTHORIZATION_MATERIAL_ROUNDS_H

#include <stddef.h>
#include <stdint.h>

#include "authorization_round_mixer.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L3_AUTH_ROUND_DRIVER_OK = 0,
    L3_AUTH_ROUND_DRIVER_DONE = 1,
    L3_AUTH_ROUND_DRIVER_MORE = 1,
    L3_AUTH_ROUND_DRIVER_INVALID = -1,
    L3_AUTH_ROUND_DRIVER_BOUNDS = -2,
    L3_AUTH_ROUND_DRIVER_STATE_MISMATCH = -3,
    L3_AUTH_ROUND_DRIVER_NO_MEMORY = -4,
    L3_AUTH_ROUND_DRIVER_NATIVE_ERROR = -5,
    /* Rounded-up end of the highest fixed preamble read (+0x1d96).  The
     * round-dependent +0x2cc/+0xec8 windows are checked separately. */
    L3_AUTH_ROUND_DRIVER_MIN_CONTEXT_WORDS = 0x1d98u / 4u
};

/* Historical V542 oracle ABI.  The V331-era reconstruction supplied all nine
 * seed vectors explicitly.  The ARM listing shows that only the param_5
 * three-vector seed enters a round: param_5 derives param_6's seed, param_6
 * derives param_7's seed, and param_7 derives the next-round param_5 seed.
 * New code should use the carry-aware native API below. */
typedef struct {
    uint64_t param5_local4c0_halves2[2];
    uint64_t param5_local4d0_halves2[2];
    uint64_t param5_auvar98_halves2[2];
    uint64_t param6_high_seed_halves2[2];
    uint64_t param6_low_seed_halves2[2];
    uint64_t param6_middle_low_seed_halves2[2];
    uint64_t param7_high_seed_halves2[2];
    uint64_t param7_low_seed_halves2[2];
    uint64_t param7_middle_low_seed_halves2[2];
} l3_authorization_round_seed;

/* Compact authoritative loop-tail state.  This replaces the multi-megabyte
 * versioned trace nesting for all rounds after the reconstructed V541 seam. */
typedef struct {
    uint32_t has_next_round;
    uint32_t next_round_index;
    uint32_t param5_words13[13];
    uint32_t param6_words13[13];
    uint32_t param7_words13[13];
    int32_t next_round_2cc_words13[13];
    int32_t next_round_ec8_words13[13];
} l3_authorization_round_handoff;

/* Compact call-boundary observation for the three vector-feedback transformations
 * in one native round.  arguments3 follows the actual ARM register order:
 * r1 (per-call vector scalar), r2 (shared context scalar), r3 (shared
 * context scalar). */
typedef struct {
    uint32_t arguments3[3];
    int32_t seed_words13[13];
    int32_t vector_input_a13[13];
    int32_t vector_input_b13[13];
    uint32_t vector_high_pairs26[26][2];
    uint32_t vector_low_pairs26[26][2];
    uint32_t state_before26[26][2];
    uint32_t state_after26[26][2];
    uint32_t output_words13[13];
} l3_authorization_feedback_observation;

/* Lean observability record for one completed round.  It contains the
 * boundaries useful for differential tests without recursively embedding
 * every prior round. */
typedef struct {
    uint32_t round_index;
    uint32_t live_q6_words4[4];
    uint32_t live_q5_words4[4];
    uint32_t live_q4_words4[4];
    uint32_t fifth_words13[13];
    int32_t sixth_seed_words13[13];
    uint32_t sixth_words13[13];
    uint32_t primary_state_a_words13[13];
    uint32_t primary_state_b_words13[13];
    uint32_t primary_intermediate_words13[13];
    uint32_t primary_state_c_words13[13];
    int32_t secondary_state_a_words13[13];
    int32_t secondary_state_b_words13[13];
    uint32_t secondary_mix_a_words13[13];
    uint32_t secondary_mix_b_words13[13];
    uint32_t secondary_mix_c_words13[13];
    uint32_t secondary_mix_d_words13[13];
    uint32_t secondary_mix_e_words13[13];
    uint32_t secondary_carry_seed_words26[26];
    l3_authorization_feedback_observation vector_feedback_calls3[3];
    l3_authorization_round_handoff tail;
} l3_authorization_round_observation;

/* Native loop state: the ordinary round handoff plus the three-vector seed
 * consumed by the pending round's param_5 materializer. */
typedef struct {
    l3_authorization_round_handoff round;
    l3_authorization_vector_seed param5_seed;
    uint32_t has_secondary_caller_words;
    int32_t secondary_caller_state_a_words13[13];
    int32_t secondary_caller_state_b_words13[13];
} l3_authorization_round_state;

typedef struct {
    l3_authorization_round_observation round;
    l3_authorization_vector_seed next_param5_seed;
} l3_authorization_round_step_result;

/* Complete statically reconstructed FUN_f3fbc5c0 core result.  The caller
 * supplies the parent context, both generated 0x42-byte frames, and the
 * 35-byte scalar; this record exposes the preamble boundary and the final
 * compact round state without requiring callers to reproduce handoffs. */
typedef struct {
    int32_t preamble_first_words13[13];
    int32_t preamble_second_words13[13];
    uint32_t initial_param5_words13[13];
    uint32_t initial_param6_words13[13];
    uint32_t initial_param7_words13[13];
    l3_authorization_vector_seed initial_param5_seed;
    uint32_t rounds_run;
    l3_authorization_round_state final_state;
    l3_authorization_round_step_result last_round;
} l3_authorization_round_pipeline_result;

/* Initialize a pending native round directly.  Unlike
 * the historical replay handoff initializer, pending_round_index is not decremented;
 * passing 58 enters the first FUN_f3fbc5c0 iteration. */
int l3_authorization_round_state_init(
    const int32_t *param1_words,
    size_t param1_word_count,
    uint32_t pending_round_index,
    const uint32_t param5_words13[13],
    const uint32_t param6_words13[13],
    const uint32_t param7_words13[13],
    const l3_authorization_vector_seed *param5_seed,
    l3_authorization_round_state *out);


int l3_authorization_run_rounds(
    const int32_t *param1_words,
    size_t param1_word_count,
    l3_authorization_round_state *inout_state,
    uint32_t max_rounds,
    uint32_t *rounds_run,
    l3_authorization_round_step_result *last_result);

/* Run the static preamble and then at most max_rounds native rounds, starting
 * with round 58.  Passing 59 completes the reconstructed round core.  This
 * is intentionally an internal-core API: producing the parent context,
 * frames, and scalar from public process1(6) inputs is a separate upstream
 * constructor boundary. */
int l3_authorization_run_round_pipeline(
    const int32_t *parent_context_words,
    size_t parent_context_word_count,
    const uint8_t left66[0x42],
    const uint8_t right66[0x42],
    const uint8_t scalar35[35],
    uint32_t max_rounds,
    l3_authorization_round_pipeline_result *out);

#ifdef __cplusplus
}
#endif

#endif
