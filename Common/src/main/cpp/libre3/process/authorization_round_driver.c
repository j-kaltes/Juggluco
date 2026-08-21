#include "authorization_round_driver.h"

#include <stdint.h>
#include <string.h>

#include "authorization_round_feedback.h"
#include "authorization_round_mixer.h"
#include "authorization_round_primary.h"
#include "authorization_round_secondary.h"
#include "authorization_vector_feedback.h"

typedef int (*l3_authorization_round_post_vector_feedback_fn)(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_words13[13]);

static int run_vector_feedback_export(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar1,
    const uint64_t local478_halves13[13][2],
    l3_authorization_round_post_vector_feedback_fn post_export,
    uint32_t out_words13[13]) {
    if (!seed_words13 || !local478_halves13 || !post_export ||
        !out_words13) {
        return -1;
    }

    l3_authorization_vector_feedback_pair32 state26[26];
    for (unsigned lane = 0; lane < 13u; ++lane) {
        state26[2u * lane].lo = (uint32_t)local478_halves13[lane][0];
        state26[2u * lane].hi =
            (uint32_t)(local478_halves13[lane][0] >> 32);
        state26[2u * lane + 1u].lo =
            (uint32_t)local478_halves13[lane][1];
        state26[2u * lane + 1u].hi =
            (uint32_t)(local478_halves13[lane][1] >> 32);
    }
    uint32_t feedback_words13[13];
    int rc = l3_authorization_vector_feedback_run13_update_export(
        seed_words13, native_param2_unused, scalar1, state26,
        feedback_words13, NULL);
    if (rc != 0) return rc;
    return post_export(feedback_words13, out_words13);
}

/* Native terminal path, evaluating each convolution/materializer once. */
static int call_native_terminal_dynamic(
    const int32_t seed_words13[13],
    uint32_t native_param2_unused,
    uint32_t scalar1_unused,
    const int32_t round_2cc_words13[13],
    const int32_t round_ec8_words13[13],
    const uint32_t local514_words13[13],
    const uint32_t local5b0_words13[13],
    const uint32_t local548_words13[13],
    const uint32_t local57c_words13[13],
    const uint32_t local5e4_words13[13],
    const uint32_t local618_words13[13],
    uint32_t out_param5_words13[13],
    uint32_t out_param6_words13[13],
    uint32_t out_param7_words13[13]) {
    if (!seed_words13 || !round_2cc_words13 || !round_ec8_words13 ||
        !local514_words13 || !local5b0_words13 || !local548_words13 ||
        !local57c_words13 || !local5e4_words13 || !local618_words13 ||
        !out_param5_words13 || !out_param6_words13 || !out_param7_words13) {
        return -1;
    }

    int32_t local514[13], local5b0[13], local548[13], local57c[13];
    int32_t local5e4[13], local618[13];
    memcpy(local514, local514_words13, sizeof(local514));
    memcpy(local5b0, local5b0_words13, sizeof(local5b0));
    memcpy(local548, local548_words13, sizeof(local548));
    memcpy(local57c, local57c_words13, sizeof(local57c));
    memcpy(local5e4, local5e4_words13, sizeof(local5e4));
    memcpy(local618, local618_words13, sizeof(local618));

    l3_authorization_word_pair high_pairs26[26];
    l3_authorization_word_pair low_pairs26[26];
    l3_authorization_word_pair param6_low_pairs26[26];
    uint64_t local478_halves13[13][2];
    int rc = l3_authorization_core_post_secondary_mix_accum_convolution26(
        local514, round_2cc_words13, high_pairs26, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_stage1_stage_accumulate_and_convolve26(
        local5b0, round_ec8_words13, low_pairs26, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_stage1_stage_vector_reduction13(
        low_pairs26, high_pairs26, local478_halves13);
    if (rc != 0) return rc;
    rc = run_vector_feedback_export(
        seed_words13, native_param2_unused, scalar1_unused,
        local478_halves13, l3_authorization_materialize_round_output_a,
        out_param5_words13);
    if (rc != 0) return rc;

    rc = l3_authorization_stage2_stage_accumulate_and_convolve26(
        local548, round_2cc_words13, high_pairs26, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_stage3_stage_accumulate_and_convolve26(
        local5e4, round_ec8_words13, param6_low_pairs26, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_stage2_stage_vector_reduction13(
        param6_low_pairs26, high_pairs26, local478_halves13);
    if (rc != 0) return rc;
    rc = run_vector_feedback_export(
        seed_words13, native_param2_unused, scalar1_unused,
        local478_halves13, l3_authorization_materialize_round_output_b,
        out_param6_words13);
    if (rc != 0) return rc;

    rc = l3_authorization_core_param7_high_accum_convolution26(
        local57c, round_2cc_words13, high_pairs26);
    if (rc != 0) return rc;
    rc = l3_authorization_stage4_stage_accumulate_and_convolve26(
        local618, round_ec8_words13, low_pairs26, NULL);
    if (rc != 0) return rc;
    rc = l3_authorization_stage3_stage_vector_reduction13(
        low_pairs26, high_pairs26, local478_halves13);
    if (rc != 0) return rc;
    rc = run_vector_feedback_export(
        seed_words13, native_param2_unused, scalar1_unused,
        local478_halves13, l3_authorization_materialize_round_output_c,
        out_param7_words13);
    if (rc != 0) return rc;

    return 0;
}

static void copy_i32_words(const int32_t *ctx, size_t byte_offset,
                           size_t count, int32_t *out) {
    memcpy(out, (const uint8_t *)ctx + byte_offset, count * sizeof(*out));
}

static void copy_u32_words(const int32_t *ctx, size_t byte_offset,
                           size_t count, uint32_t *out) {
    memcpy(out, (const uint8_t *)ctx + byte_offset, count * sizeof(*out));
}

static uint32_t load_u32(const int32_t *ctx, size_t byte_offset) {
    uint32_t value;
    memcpy(&value, (const uint8_t *)ctx + byte_offset, sizeof(value));
    return value;
}

static int context_covers_round(size_t word_count, uint32_t round_index) {
    if (word_count > SIZE_MAX / sizeof(int32_t)) return 0;
    const size_t context_bytes = word_count * sizeof(int32_t);
    /* The +0xec8 round window is above both the +0x2cc window and every
     * fixed preamble read (whose highest end is +0x2a4), so this one exact
     * bound covers the complete production read closure. */
    const uint64_t last_window_end64 = UINT64_C(0xec8) +
        (uint64_t)round_index * UINT64_C(0x34) +
        UINT64_C(13) * sizeof(int32_t);
    if (last_window_end64 > (uint64_t)SIZE_MAX) return 0;
    return (size_t)last_window_end64 <= context_bytes;
}

static int validate_handoff_windows(const int32_t *ctx,
                                    const l3_authorization_round_handoff *tail) {
    int32_t expected_2cc[13];
    int32_t expected_ec8[13];
    const size_t cursor = (size_t)tail->next_round_index * 0x34u;
    copy_i32_words(ctx, 0x2ccu + cursor, 13u, expected_2cc);
    copy_i32_words(ctx, 0xec8u + cursor, 13u, expected_ec8);
    return memcmp(expected_2cc, tail->next_round_2cc_words13, sizeof(expected_2cc)) == 0 &&
           memcmp(expected_ec8, tail->next_round_ec8_words13, sizeof(expected_ec8)) == 0;
}


static int round_step_with_workspace(
    const int32_t *param1_words,
    size_t param1_word_count,
    const l3_authorization_round_handoff *previous_tail,
    const int32_t native_secondary_caller_state_a_words13[13],
    const int32_t native_secondary_caller_state_b_words13[13],
    l3_authorization_round_observation *out) {
    if (!param1_words || !previous_tail || !out ||
        ((native_secondary_caller_state_a_words13 == NULL) !=
         (native_secondary_caller_state_b_words13 == NULL))) {
        return L3_AUTH_ROUND_DRIVER_INVALID;
    }
    memset(out, 0, sizeof(*out));
    if (!previous_tail->has_next_round) return L3_AUTH_ROUND_DRIVER_DONE;
    if (!context_covers_round(param1_word_count, previous_tail->next_round_index)) {
        return L3_AUTH_ROUND_DRIVER_BOUNDS;
    }
    if (!validate_handoff_windows(param1_words, previous_tail)) {
        return L3_AUTH_ROUND_DRIVER_STATE_MISMATCH;
    }

    out->round_index = previous_tail->next_round_index;
    for (unsigned i = 0; i < 13u; ++i) {
        out->fifth_words13[i] = (uint32_t)previous_tail->next_round_2cc_words13[i];
        out->sixth_seed_words13[i] = previous_tail->next_round_ec8_words13[i];
    }

    const uint32_t scalar0 = load_u32(param1_words, 0x268u);
    const uint32_t scalar1 = load_u32(param1_words, 0x26cu);
    uint32_t affine_words13[13];
    uint32_t sixth_state26[26][2];
    int rc = l3_authorization_primary_sixth_vector_reduce_update_export(
        out->fifth_words13,
        out->sixth_seed_words13,
        scalar0,
        scalar1,
        affine_words13,
        sixth_state26,
        out->sixth_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;

    uint32_t param4_param5_words13[13];
    uint32_t param4_staging_words13[13];
    uint32_t param4_mix_words26[26];
    uint32_t local3a0_words26[26];
    int32_t vector_reduce_param1_words13[13];
    copy_u32_words(param1_words, 0x0f8u, 13u, param4_param5_words13);
    copy_u32_words(param1_words, 0x12cu, 13u, param4_staging_words13);
    copy_u32_words(param1_words, 0x1fcu, 26u, param4_mix_words26);
    copy_u32_words(param1_words, 0x194u, 26u, local3a0_words26);
    copy_i32_words(param1_words, 0x090u, 13u, vector_reduce_param1_words13);

    rc = l3_authorization_primary_run(
        previous_tail->param5_words13,
        previous_tail->param6_words13,
        previous_tail->param7_words13,
        param1_words,
        out->primary_state_a_words13,
        out->primary_state_b_words13,
        out->primary_state_c_words13);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;

    int32_t secondary_input_words13[13];
    int32_t secondary_context_words13[13];
    int32_t caller_mix_words13[13];
    int32_t caller_seed_words13[13];
    int32_t param4_c4_words13[13];
    uint32_t secondary_affine_words13[13];
    uint32_t local5b0_words13[13];
    uint32_t sixth_secondary_vector_reduce_words13[13];
    memcpy(secondary_input_words13, out->primary_state_a_words13,
           sizeof(secondary_input_words13));
    memcpy(out->secondary_state_a_words13, out->primary_state_b_words13,
           sizeof(out->secondary_state_a_words13));
    memcpy(out->secondary_state_b_words13, out->primary_state_c_words13,
           sizeof(out->secondary_state_b_words13));
    memcpy(secondary_context_words13, param4_param5_words13,
           sizeof(secondary_context_words13));
    memcpy(caller_mix_words13, param4_staging_words13,
           sizeof(caller_mix_words13));
    copy_i32_words(param1_words, 0x090u, 13u, caller_seed_words13);
    copy_i32_words(param1_words, 0x0c4u, 13u, param4_c4_words13);

    rc = l3_authorization_secondary_param3_bound_secondary_vector_reduce_affine_words13(
        out->secondary_state_b_words13,
        caller_seed_words13,
        scalar0,
        scalar1,
        secondary_affine_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    rc = l3_authorization_secondary_derive_state_stage1(
        secondary_context_words13,
        secondary_affine_words13,
        caller_seed_words13,
        scalar0,
        scalar1,
        secondary_input_words13,
        caller_mix_words13,
        out->secondary_mix_a_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    rc = l3_authorization_secondary_derive_state_stage2(
        out->secondary_state_b_words13,
        secondary_affine_words13,
        caller_seed_words13,
        scalar0,
        scalar1,
        out->secondary_mix_a_words13,
        out->secondary_state_a_words13,
        param4_c4_words13,
        out->secondary_mix_b_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    rc = l3_authorization_secondary_derive_state_stage3(
        out->secondary_mix_a_words13,
        caller_seed_words13,
        scalar0,
        scalar1,
        out->secondary_mix_c_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    rc = l3_authorization_secondary_local3f8_bound_sixth_local42c_preimage_param7_local460_words13(
        out->secondary_mix_c_words13,
        out->secondary_mix_a_words13,
        param1_words,
        secondary_input_words13,
        caller_seed_words13,
        scalar0,
        scalar1,
        out->secondary_mix_b_words13,
        out->secondary_mix_d_words13,
        sixth_secondary_vector_reduce_words13,
        local5b0_words13,
        out->secondary_mix_e_words13,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;

    l3_authorization_secondary_pair32 raw22f0[13], prefix22f0[13];
    l3_authorization_secondary_pair32 raw2370[13], prefix2370[13];
    rc = l3_authorization_secondary_param2_local42c_accum22f0_2370_pairs13(
        out->secondary_state_a_words13,
        out->secondary_mix_d_words13,
        raw22f0,
        prefix22f0,
        raw2370,
        prefix2370,
        NULL);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    memset(out->secondary_carry_seed_words26, 0,
           sizeof(out->secondary_carry_seed_words26));
    memcpy(out->secondary_carry_seed_words26 + 2u, prefix22f0,
           24u * sizeof(uint32_t));

    int32_t secondary_context_mix_words26[26];
    copy_i32_words(param1_words, 0x194u, 26u, secondary_context_mix_words26);
    {
        uint32_t local4c8_words26[26], local530_words26[26];
        uint32_t local5e4_words13[13], local618_words13[13];
        rc = l3_authorization_secondary_local3c4_local460_accum_convolution26_materialize4c8(
            out->secondary_mix_b_words13, out->secondary_mix_e_words13,
            local4c8_words26, NULL);
        if (rc == 0) {
            rc = l3_authorization_secondary_param2_local42c_accum_convolution26_materialize530(
                out->secondary_state_a_words13, out->secondary_mix_d_words13,
                local530_words26, NULL);
        }
        if (rc == 0 && native_secondary_caller_state_a_words13) {
            rc = l3_authorization_secondary_run(
                (const int32_t *)out->primary_state_a_words13,
                (const int32_t *)out->primary_state_b_words13,
                (const int32_t *)out->primary_state_c_words13,
                param1_words,
                native_secondary_caller_state_a_words13,
                native_secondary_caller_state_b_words13,
                local5b0_words13, local5e4_words13, local618_words13);
        } else if (rc == 0) {
            rc = l3_authorization_secondary_build_terminal_outputs(
                local4c8_words26, secondary_context_mix_words26,
                local530_words26, out->secondary_carry_seed_words26,
                out->secondary_state_b_words13, out->secondary_mix_a_words13,
                vector_reduce_param1_words13, scalar0, scalar1,
                local5e4_words13, local618_words13, NULL);
        }
        if (rc == 0) {
            rc = call_native_terminal_dynamic(
                vector_reduce_param1_words13, scalar0, scalar1,
                previous_tail->next_round_2cc_words13,
                previous_tail->next_round_ec8_words13,
                out->primary_state_a_words13, local5b0_words13,
                out->primary_state_b_words13,
                out->primary_state_c_words13, local5e4_words13,
                local618_words13,
                out->tail.param5_words13, out->tail.param6_words13,
                out->tail.param7_words13);
        }
        if (rc == 0 && out->round_index != 0u) {
            out->tail.has_next_round = 1u;
            out->tail.next_round_index = out->round_index - 1u;
            const size_t next_cursor = (size_t)out->tail.next_round_index * 0x34u;
            copy_i32_words(param1_words, 0x2ccu + next_cursor, 13u,
                           out->tail.next_round_2cc_words13);
            copy_i32_words(param1_words, 0xec8u + next_cursor, 13u,
                           out->tail.next_round_ec8_words13);
        }
    }
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;
    return L3_AUTH_ROUND_DRIVER_OK;
}



int l3_authorization_round_state_init(
    const int32_t *param1_words,
    size_t param1_word_count,
    uint32_t pending_round_index,
    const uint32_t param5_words13[13],
    const uint32_t param6_words13[13],
    const uint32_t param7_words13[13],
    l3_authorization_round_state *out) {
    if (!param1_words || !param5_words13 || !param6_words13 ||
        !param7_words13 || !out) {
        return L3_AUTH_ROUND_DRIVER_INVALID;
    }
    memset(out, 0, sizeof(*out));
    if (!context_covers_round(param1_word_count, pending_round_index)) {
        return L3_AUTH_ROUND_DRIVER_BOUNDS;
    }
    out->round.has_next_round = 1u;
    out->round.next_round_index = pending_round_index;
    memcpy(out->round.param5_words13, param5_words13,
           sizeof(out->round.param5_words13));
    memcpy(out->round.param6_words13, param6_words13,
           sizeof(out->round.param6_words13));
    memcpy(out->round.param7_words13, param7_words13,
           sizeof(out->round.param7_words13));
    const size_t cursor = (size_t)pending_round_index * 0x34u;
    copy_i32_words(param1_words, 0x2ccu + cursor, 13u,
                   out->round.next_round_2cc_words13);
    copy_i32_words(param1_words, 0xec8u + cursor, 13u,
                   out->round.next_round_ec8_words13);
    return L3_AUTH_ROUND_DRIVER_OK;
}


int l3_authorization_run_rounds(
    const int32_t *param1_words,
    size_t param1_word_count,
    l3_authorization_round_state *inout_state,
    uint32_t max_rounds,
    uint32_t *rounds_run,
    l3_authorization_round_step_result *last_result) {
    if (!param1_words || !inout_state) return L3_AUTH_ROUND_DRIVER_INVALID;
    if (rounds_run) *rounds_run = 0u;
    if (last_result) memset(last_result, 0, sizeof(*last_result));
    if (!inout_state->round.has_next_round) return L3_AUTH_ROUND_DRIVER_OK;
    if (max_rounds == 0u) return L3_AUTH_ROUND_DRIVER_MORE;

    l3_authorization_round_state cursor = *inout_state;
    l3_authorization_round_step_result current;
    uint32_t count = 0u;
    int rc = L3_AUTH_ROUND_DRIVER_OK;
    while (cursor.round.has_next_round && count < max_rounds) {
        memset(&current, 0, sizeof(current));
        rc = round_step_with_workspace(
            param1_words, param1_word_count, &cursor.round,
            cursor.has_secondary_caller_words
                ? cursor.secondary_caller_state_a_words13 : NULL,
            cursor.has_secondary_caller_words
                ? cursor.secondary_caller_state_b_words13 : NULL,
            &current.round);
        if (rc != L3_AUTH_ROUND_DRIVER_OK) break;
        cursor.round = current.round.tail;
        *inout_state = cursor;
        if (last_result) *last_result = current;
        ++count;
    }
    if (rounds_run) *rounds_run = count;
    if (rc < 0) return rc;
    return cursor.round.has_next_round ? L3_AUTH_ROUND_DRIVER_MORE
                                      : L3_AUTH_ROUND_DRIVER_OK;
}

int l3_authorization_run_round_pipeline(
    const int32_t *parent_context_words,
    size_t parent_context_word_count,
    const uint8_t left66[0x42],
    const uint8_t right66[0x42],
    const uint8_t scalar35[35],
    uint32_t max_rounds,
    l3_authorization_round_pipeline_result *out) {
    if (!parent_context_words || !left66 || !right66 || !scalar35 || !out) {
        return L3_AUTH_ROUND_DRIVER_INVALID;
    }
    memset(out, 0, sizeof(*out));
    if (!context_covers_round(parent_context_word_count, 58u)) {
        return L3_AUTH_ROUND_DRIVER_BOUNDS;
    }

    int32_t preamble_first_words13[13];
    int32_t preamble_second_words13[13];
    uint32_t initial_param5_words13[13];
    uint32_t initial_param6_words13[13];
    uint32_t initial_param7_words13[13];
    int rc = l3_authorization_prepare_round_pipeline(
        (const uint8_t *)parent_context_words, left66, right66, scalar35,
        preamble_first_words13, preamble_second_words13,
        initial_param5_words13, initial_param6_words13,
        initial_param7_words13);
    if (rc != 0) return L3_AUTH_ROUND_DRIVER_NATIVE_ERROR;

    l3_authorization_round_state cursor;
    rc = l3_authorization_round_state_init(
        parent_context_words, parent_context_word_count, 58u,
        initial_param5_words13, initial_param6_words13,
        initial_param7_words13, &cursor);
    if (rc != L3_AUTH_ROUND_DRIVER_OK) return rc;
    cursor.has_secondary_caller_words = 1u;
    memcpy(cursor.secondary_caller_state_a_words13, preamble_first_words13,
           sizeof(cursor.secondary_caller_state_a_words13));
    memcpy(cursor.secondary_caller_state_b_words13, preamble_second_words13,
           sizeof(cursor.secondary_caller_state_b_words13));

    rc = l3_authorization_run_rounds(
        parent_context_words, parent_context_word_count, &cursor, max_rounds,
        &out->rounds_run, NULL);
    out->final_state = cursor;
    return rc;
}
