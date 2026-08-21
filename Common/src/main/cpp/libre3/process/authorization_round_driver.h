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
    L3_AUTH_ROUND_DRIVER_NATIVE_ERROR = -5
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

/* Working values retained by the compact round driver.  Trace-only copies of
 * live registers, feedback inputs and before/after states are deliberately
 * omitted from the production path. */
typedef struct {
    uint32_t round_index;
    uint32_t fifth_words13[13];
    int32_t sixth_seed_words13[13];
    uint32_t sixth_words13[13];
    uint32_t primary_state_a_words13[13];
    uint32_t primary_state_b_words13[13];
    uint32_t primary_state_c_words13[13];
    int32_t secondary_state_a_words13[13];
    int32_t secondary_state_b_words13[13];
    uint32_t secondary_mix_a_words13[13];
    uint32_t secondary_mix_b_words13[13];
    uint32_t secondary_mix_c_words13[13];
    uint32_t secondary_mix_d_words13[13];
    uint32_t secondary_mix_e_words13[13];
    uint32_t secondary_carry_seed_words26[26];
    l3_authorization_round_handoff tail;
} l3_authorization_round_observation;

/* Native loop state plus the two caller-state vectors carried across rounds. */
typedef struct {
    l3_authorization_round_handoff round;
    uint32_t has_secondary_caller_words;
    int32_t secondary_caller_state_a_words13[13];
    int32_t secondary_caller_state_b_words13[13];
} l3_authorization_round_state;

typedef struct {
    l3_authorization_round_observation round;
} l3_authorization_round_step_result;

/* Compact statically reconstructed FUN_f3fbc5c0 core result.  Intermediate
 * preamble material is private to the driver; callers only need the final
 * state and the number of completed rounds. */
typedef struct {
    uint32_t rounds_run;
    l3_authorization_round_state final_state;
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
