#include "authorization_shared_point_core.h"

#include "authorization_finalizer.h"
#include "authorization_frame_transform.h"
#include "authorization_round_driver.h"
#include "authorization_shared_point_tail.h"
#include "authorization_parent_contexts.inc"
#include "authorization_tables.inc"

#include <stdint.h>
#include <string.h>

static const l3_authorization_frame_transform_tables
    l3_authorization_shared_point_transform_tables = {
        k_dat40c6a52,
        k_dat40c6a52_logical_len,
        L3_AUTH_TABLE_PACK6_PREFIX5375_RAW_TAIL
    };

const l3_authorization_frame_tables
    l3_authorization_shared_point_frame_tables = {
        k_dat40e8423,
        k_dat40e8423_logical_len,
        L3_AUTH_TABLE_PACK6,
        k_dat40e85ab,
        k_dat40e85ab_logical_len,
        L3_AUTH_TABLE_PACK6
    };

typedef union {
    struct {
        uint8_t left66[L3_AUTHORIZATION_SHARED_POINT_FRAME_LEN];
        uint8_t right66[L3_AUTHORIZATION_SHARED_POINT_FRAME_LEN];
    } round;
    struct {
        l3_authorization_final_compact_result result;
        uint32_t param2[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
        uint32_t param3[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
        uint32_t parent[L3_AUTH_FINAL_STATE_FIRST_LOOP_PAIRS];
    } finalizer;
} l3_authorization_shared_point_phase;

enum {
    L3_AUTHORIZATION_SHARED_POINT_TRANSFORM_RAW_LEN = 0x4c,
    L3_AUTHORIZATION_SHARED_POINT_TRANSFORM_STATE_LEN = 0x10a
};

_Static_assert(
    L3_AUTHORIZATION_SHARED_POINT_TRANSFORM_RAW_LEN +
            L3_AUTHORIZATION_SHARED_POINT_TRANSFORM_STATE_LEN <=
        sizeof(l3_authorization_round_pipeline_result),
    "post-finalizer transform must fit the reusable round workspace");

size_t l3_authorization_shared_point_workspace_size(void) {
    const size_t finalizer_size =
        l3_authorization_finalizer_compact_workspace_size();
    return finalizer_size > sizeof(l3_authorization_round_pipeline_result)
               ? finalizer_size
               : sizeof(l3_authorization_round_pipeline_result);
}

size_t l3_authorization_shared_point_workspace_alignment(void) {
    const size_t finalizer_alignment =
        l3_authorization_finalizer_compact_workspace_alignment();
    const size_t round_alignment =
        _Alignof(l3_authorization_round_pipeline_result);
    return finalizer_alignment > round_alignment
               ? finalizer_alignment
               : round_alignment;
}

int l3_authorization_shared_point_frames_into(
    const uint8_t native_scalar35[35],
    const uint8_t peer_public64[64],
    void *workspace,
    size_t workspace_size,
    uint8_t out_frames[L3_AUTHORIZATION_SHARED_POINT_OUTPUT_LEN]) {
    const size_t required_workspace =
        l3_authorization_shared_point_workspace_size();
    const size_t required_alignment =
        l3_authorization_shared_point_workspace_alignment();
    if (!native_scalar35 || !peer_public64 || !workspace || !out_frames) {
        return L3_AUTHORIZATION_SHARED_POINT_ERR_ARGUMENT;
    }
    if (workspace_size < required_workspace ||
        (uintptr_t)workspace % required_alignment != 0u) {
        return L3_AUTHORIZATION_SHARED_POINT_ERR_WORKSPACE;
    }

    l3_authorization_shared_point_phase phase;
    memset(&phase.round, 0, sizeof(phase.round));
    for (unsigned i = 0u; i < 32u; ++i) {
        phase.round.left66[i] = peer_public64[31u - i];
        phase.round.right66[i] = peer_public64[63u - i];
    }

    l3_authorization_round_pipeline_result *round =
        (l3_authorization_round_pipeline_result *)workspace;
    int rc = l3_authorization_run_round_pipeline(
        (const int32_t *)(const void *)k_p6_parent_context_db,
        sizeof(k_p6_parent_context_db) / sizeof(int32_t),
        phase.round.left66, phase.round.right66, native_scalar35,
        59u, round);
    if (rc != 0 || round->rounds_run != 59u ||
        round->final_state.round.has_next_round != 0u) {
        return L3_AUTHORIZATION_SHARED_POINT_ERR_ENGINE;
    }

    memcpy(phase.finalizer.param2,
           round->final_state.round.param5_words13,
           sizeof(phase.finalizer.param2));
    memcpy(phase.finalizer.param3,
           round->final_state.round.param7_words13,
           sizeof(phase.finalizer.param3));
    memcpy(phase.finalizer.parent, k_p6_parent_context_db,
           sizeof(phase.finalizer.parent));

    rc = l3_authorization_finalize_round_result_compact_with_workspace(
        phase.finalizer.param2,
        phase.finalizer.param3,
        phase.finalizer.parent,
        L3_P6_PARENT_PAIR_LO,
        L3_P6_PARENT_PAIR_HI,
        512u, 512u, 512u,
        workspace, workspace_size,
        &phase.finalizer.result);
    if (rc != 0 || !phase.finalizer.result.terminal_taken) {
        return L3_AUTHORIZATION_SHARED_POINT_ERR_ENGINE;
    }

    /* The finalizer no longer needs workspace here.  Reuse its first bytes for
       the two non-overlapping transform buffers instead of retaining another
       342-byte phase on the constrained caller's stack. */
    uint8_t *const raw76 = (uint8_t *)workspace;
    uint8_t *const state10a =
        raw76 + L3_AUTHORIZATION_SHARED_POINT_TRANSFORM_RAW_LEN;
    rc = l3_authorization_transform_shared_point_tail(
        phase.finalizer.result.raw40, raw76);
    if (rc == 0) {
        rc = l3_authorization_normalize_frame_state(
            &l3_authorization_shared_point_transform_tables,
            raw76,
            state10a);
    }
    uint32_t frame_count = 0u;
    if (rc == 0) {
        rc = l3_authorization_expand_frame_state(
            &l3_authorization_shared_point_transform_tables,
            state10a,
            0x20u,
            out_frames,
            L3_AUTHORIZATION_SHARED_POINT_OUTPUT_LEN,
            &frame_count);
    }
    if (rc != 0 || frame_count != L3_AUTHORIZATION_SHARED_POINT_FRAME_COUNT) {
        return L3_AUTHORIZATION_SHARED_POINT_ERR_ENGINE;
    }
    return L3_AUTHORIZATION_SHARED_POINT_OK;
}
