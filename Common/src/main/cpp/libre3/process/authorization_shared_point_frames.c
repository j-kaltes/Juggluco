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
#include "authorization_shared_point_frames.h"

#include <stdlib.h>
#include <string.h>

static int validate_frame_tables(const l3_authorization_frame_tables *t) {
    if (!t || !t->state_transition_table) return L3_AUTH_ERR_ARGUMENT;
    if (t->state_transition_table_len < 0x20000u) return L3_AUTH_ERR_TABLE_SIZE;
    return L3_AUTH_OK;
}

static int validate_digest_frame_program(const l3_authorization_frame_tables *t) {
    int rc = validate_frame_tables(t);
    if (rc) return rc;
    if (!t->digest_frame_program) return L3_AUTH_ERR_ARGUMENT;
    if (t->digest_frame_program_len < 0x186u) return L3_AUTH_ERR_TABLE_SIZE;
    return L3_AUTH_OK;
}

static int validate_range_extract_program(const l3_authorization_frame_tables *t) {
    int rc = validate_frame_tables(t);
    if (rc) return rc;
    if (!t->range_extract_program) return L3_AUTH_ERR_ARGUMENT;
    if (t->range_extract_program_len < 0x14au) return L3_AUTH_ERR_TABLE_SIZE;
    return L3_AUTH_OK;
}

static int table_state_transform(const uint8_t *dat429, const uint8_t *table,
                           uint32_t param1, uint32_t param2,
                           const uint8_t *p3, const uint8_t *p4,
                           uint8_t *out, size_t out_len) {
    uint32_t whole = ((param2 << 14) & 0xffffffffu) >> 18;
    uint32_t rem = param2 >> 18;
    size_t need = (size_t)whole + (size_t)rem;
    uint8_t state = 0;
    if (!dat429 || !table || !p3 || !p4 || !out) return L3_AUTH_ERR_ARGUMENT;
    if (out_len < need) return L3_AUTH_ERR_ARGUMENT;
    for (uint32_t i = 0; i < whole; ++i) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ (uint32_t)p3[i]) |
                        ((uint32_t)p4[i] << 8)) ^
                       ((uint32_t)table[i + (param1 & 0x3fffffu)] << 11);
        state = dat429[idx];
        out[i] = (uint8_t)(state & 7u);
    }
    for (uint32_t i = 0; i < rem; ++i) {
        uint32_t idx = ((uint32_t)(state & 0xf8u) |
                        ((uint32_t)p4[whole + i] << 8)) ^
                       ((uint32_t)table[i + (param1 & 0x3fffffu) + whole] << 11);
        state = dat429[idx];
        out[whole + i] = (uint8_t)(state & 7u);
    }
    return L3_AUTH_OK;
}

void l3_authorization_frame_stream_clear(l3_authorization_frame_stream *obj) {
    if (!obj) return;
    free(obj->frames);
    memset(obj, 0, sizeof(*obj));
}

int l3_authorization_frame_stream_copy(l3_authorization_frame_stream *obj,
                           uint32_t type_word,
                           const uint8_t *frames,
                           uint32_t frame_count,
                           uint32_t byte_len) {
    if (!obj) return L3_AUTH_ERR_ARGUMENT;
    memset(obj, 0, sizeof(*obj));
    obj->type_word = type_word;
    obj->byte_len = byte_len;
    obj->frame_count = frame_count;
    if (frame_count) {
        size_t n = (size_t)frame_count * L3_AUTH_FRAME_LEN;
        if (!frames) return L3_AUTH_ERR_ARGUMENT;
        obj->frames = (uint8_t *)malloc(n);
        if (!obj->frames) return L3_AUTH_ERR_ALLOC;
        memcpy(obj->frames, frames, n);
    }
    return L3_AUTH_OK;
}

int l3_authorization_frames_concatenate(const l3_authorization_frame_stream *lhs,
                          const l3_authorization_frame_stream *rhs,
                          l3_authorization_frame_stream *out) {
    if (!lhs || !rhs || !out) return L3_AUTH_ERR_ARGUMENT;
    if (lhs->type_word != 0 || rhs->type_word != 0) return L3_AUTH_ERR_STATE;
    if ((lhs->byte_len & 0x0fu) != 0 || (rhs->byte_len & 0x0fu) != 0) return L3_AUTH_ERR_STATE;
    uint32_t out_frames = lhs->frame_count + rhs->frame_count;
    uint32_t out_len = lhs->byte_len + rhs->byte_len;
    l3_authorization_frame_stream_clear(out);
    out->type_word = 0;
    out->byte_len = out_len;
    out->frame_count = out_frames;
    if (out_frames) {
        size_t lhs_n = (size_t)lhs->frame_count * L3_AUTH_FRAME_LEN;
        size_t rhs_n = (size_t)rhs->frame_count * L3_AUTH_FRAME_LEN;
        out->frames = (uint8_t *)malloc(lhs_n + rhs_n);
        if (!out->frames) return L3_AUTH_ERR_ALLOC;
        if (lhs_n) memcpy(out->frames, lhs->frames, lhs_n);
        if (rhs_n) memcpy(out->frames + lhs_n, rhs->frames, rhs_n);
    }
    return L3_AUTH_OK;
}

/* Port of native FUN_f3f05ba4 using the DAT_f40e8423-relative subtable. */
static void digest_frame_second_half_transform(const l3_authorization_frame_tables *t,
                            const uint8_t in1[0x82], const uint8_t in2[0x82],
                            uint8_t out[0x82]) {
    const uint8_t *tab = t->digest_frame_program;
    uint32_t state = 0;
    for (int i = 0; i != 0x40; ++i) {
        uint32_t idx = (((state & 0xf8u) ^ (uint32_t)in1[i]) |
                        ((uint32_t)in2[i] << 8)) ^
                       ((uint32_t)tab[0x0c4u + (uint32_t)i] << 11);
        state = t->state_transition_table[idx];
    }
    for (int i = 0; i != -0x42; --i) {
        int src = 0x40 - i;
        uint32_t idx = (((state & 0xf8u) ^ (uint32_t)in1[src]) |
                        ((uint32_t)in2[src] << 8)) ^
                       ((uint32_t)tab[0x104u + (uint32_t)(-i)] << 11);
        state = t->state_transition_table[idx];
        out[-i] = (uint8_t)(state & 7u);
    }
    for (int i = 0; i != 0x40; ++i) {
        uint32_t idx = (state & 0xf8u) | ((uint32_t)tab[0x146u + (uint32_t)i] << 11);
        state = t->state_transition_table[idx];
        out[0x42 + i] = (uint8_t)(state & 7u);
    }
}

int l3_authorization_encode_digest_result_frames(const l3_authorization_frame_tables *tables,
                                          const uint8_t final82[0x82],
                                          uint8_t out_two_frames[2 * L3_AUTH_FRAME_LEN]) {
    int rc = validate_digest_frame_program(tables);
    if (rc) return rc;
    if (!final82 || !out_two_frames) return L3_AUTH_ERR_ARGUMENT;
    uint8_t state[0x82];
    uint8_t tmp[0x82];
    rc = table_state_transform(tables->state_transition_table, tables->digest_frame_program,
                         0u, 0x820u, final82, final82, state, sizeof(state));
    if (rc) return rc;
    rc = table_state_transform(tables->state_transition_table, tables->digest_frame_program,
                         0x82u, 0x420u, state, state, out_two_frames, L3_AUTH_FRAME_LEN);
    if (rc) return rc;
    digest_frame_second_half_transform(tables, state, state, state);
    rc = table_state_transform(tables->state_transition_table, tables->digest_frame_program,
                         0x82u, 0x420u, state, state,
                         out_two_frames + L3_AUTH_FRAME_LEN, L3_AUTH_FRAME_LEN);
    if (rc) return rc;
    memcpy(tmp, out_two_frames, L3_AUTH_FRAME_LEN);
    memcpy(out_two_frames, out_two_frames + L3_AUTH_FRAME_LEN, L3_AUTH_FRAME_LEN);
    memcpy(out_two_frames + L3_AUTH_FRAME_LEN, tmp, L3_AUTH_FRAME_LEN);
    return L3_AUTH_OK;
}

int l3_authorization_frames_digest(const l3_authorization_frame_tables *tables,
                        const l3_authorization_frame_stream *input,
                        const uint8_t *prefix, size_t prefix_len,
                        const uint8_t *suffix, size_t suffix_len,
                        const l3_authorization_digest_callbacks *digest,
                        l3_authorization_frame_stream *out) {
    int rc = validate_digest_frame_program(tables);
    if (rc) return rc;
    if (!input || !digest || !digest->init || !digest->update ||
        !digest->update_frame || !digest->final82 || !out) return L3_AUTH_ERR_ARGUMENT;
    if (input->type_word != 0) return L3_AUTH_ERR_STATE;
    if (input->frame_count != ((input->byte_len + 0x0fu) >> 4)) return L3_AUTH_ERR_STATE;
    if ((prefix_len && !prefix) || (suffix_len && !suffix)) return L3_AUTH_ERR_ARGUMENT;

    if (digest->init(digest->user) != 0) return L3_AUTH_ERR_CALLBACK;
    if (prefix_len && digest->update(digest->user, prefix, prefix_len) != 0) return L3_AUTH_ERR_CALLBACK;
    for (uint32_t i = 0; i < input->frame_count; ++i) {
        uint32_t remaining = input->byte_len - i * 0x10u;
        uint32_t n = remaining > 0x10u ? 0x10u : remaining;
        if (digest->update_frame(digest->user, input->frames + (size_t)i * L3_AUTH_FRAME_LEN, n) != 0) {
            return L3_AUTH_ERR_CALLBACK;
        }
    }
    if (suffix_len && digest->update(digest->user, suffix, suffix_len) != 0) return L3_AUTH_ERR_CALLBACK;
    uint8_t final82[0x82];
    if (digest->final82(digest->user, final82) != 0) return L3_AUTH_ERR_CALLBACK;

    uint8_t frames[2 * L3_AUTH_FRAME_LEN];
    rc = l3_authorization_encode_digest_result_frames(tables, final82, frames);
    if (rc) return rc;
    l3_authorization_frame_stream_clear(out);
    return l3_authorization_frame_stream_copy(out, 0, frames, 2, 0x20u);
}

/* Port of native FUN_f3f06178; input/output are the 0x82-byte shift state. */
static void range_extract_shift_transform(const l3_authorization_frame_tables *t,
                            const uint8_t in1[0x82], const uint8_t in2[0x82],
                            uint8_t out[0x82]) {
    const uint8_t *tab = t->range_extract_program;
    uint32_t idx0 = (((uint32_t)in2[0] << 8) | in1[0]) ^ 0x1b800u;
    uint32_t b0 = t->state_transition_table[idx0];
    uint32_t idx1 = ((((b0 & 0xf8u) ^ in1[1]) | ((uint32_t)in2[1] << 8)) ^ 0x2800u);
    uint32_t b1 = t->state_transition_table[idx1];
    uint32_t idx2 = ((((b1 & 0xf8u) ^ in1[2]) | ((uint32_t)in2[2] << 8)) ^ 0x1e800u);
    uint32_t b2 = t->state_transition_table[idx2];
    uint32_t idx3 = ((((b2 & 0xf8u) ^ in1[3]) | ((uint32_t)in2[3] << 8)) ^ 0x9000u);
    uint32_t state = t->state_transition_table[idx3];
    for (int i = 0; i != -0x7e; --i) {
        int src = 4 - i;
        uint32_t idx = (((state & 0xf8u) ^ (uint32_t)in1[src]) |
                        ((uint32_t)in2[src] << 8)) ^
                       ((uint32_t)tab[0x004u + (uint32_t)(-i)] << 11);
        state = t->state_transition_table[idx];
        out[-i] = (uint8_t)(state & 7u);
    }
    uint32_t b = t->state_transition_table[(state & 0xf8u) | 0x9000u];
    out[0x7e] = (uint8_t)(b & 7u);
    b = t->state_transition_table[(b & 0xf8u) | 0x14800u];
    out[0x7f] = (uint8_t)(b & 7u);
    b = t->state_transition_table[(b & 0xf8u) | 0xa800u];
    out[0x80] = (uint8_t)(b & 7u);
    out[0x81] = (uint8_t)(t->state_transition_table[(b & 0xf8u) | 0xa800u] & 7u);
}

int l3_authorization_frames_extract_range(const l3_authorization_frame_tables *tables,
                         const l3_authorization_frame_stream *input,
                         uint32_t offset,
                         uint32_t length,
                         l3_authorization_frame_stream *out) {
    int rc = validate_range_extract_program(tables);
    if (rc) return rc;
    if (!input || !out) return L3_AUTH_ERR_ARGUMENT;
    if (input->type_word != 0) return L3_AUTH_ERR_STATE;
    if (offset > input->byte_len || length > input->byte_len - offset) return L3_AUTH_ERR_ARGUMENT;
    uint32_t frames = (length + 0x0fu) >> 4;
    uint8_t *buf = NULL;
    if (frames) {
        buf = (uint8_t *)malloc((size_t)frames * L3_AUTH_FRAME_LEN);
        if (!buf) return L3_AUTH_ERR_ALLOC;
    }
    uint32_t input_frames = (input->byte_len + 0x0fu) >> 4;
    for (uint32_t b = 0; b < frames; ++b) {
        uint32_t src_frame = b + (offset >> 4);
        uint32_t next_frame = src_frame + 1u < input_frames ? src_frame + 1u : src_frame;
        /* Native FUN_f3f06328 lays this input out as four zeroed NEON
         * vectors followed immediately by a copy of the current 0x42-byte
         * frame.  FUN_f3f06080 consumes the complete 0x82-byte span. */
        uint8_t zero_and_current[0x82];
        uint8_t state[0x82];
        memset(zero_and_current, 0, 0x40u);
        memcpy(zero_and_current + 0x40u,
               input->frames + (size_t)src_frame * L3_AUTH_FRAME_LEN,
               L3_AUTH_FRAME_LEN);
        memset(state, 0, sizeof(state));
        rc = table_state_transform(tables->state_transition_table, tables->range_extract_program,
                             0x86u, 0x1000420u,
                             input->frames + (size_t)next_frame * L3_AUTH_FRAME_LEN,
                             zero_and_current, state, sizeof(state));
        if (rc) { free(buf); return rc; }
        uint32_t mod = offset & 0x0fu;
        for (uint32_t i = mod; i < 0x10u; ++i) {
            uint8_t tmp[0x82];
            memset(tmp, 0, sizeof(tmp));
            range_extract_shift_transform(tables, state, state, tmp);
            memcpy(state, tmp, sizeof(state));
        }
        rc = table_state_transform(tables->state_transition_table, tables->range_extract_program,
                             0x108u, 0x420u, state, state,
                             buf + (size_t)b * L3_AUTH_FRAME_LEN, L3_AUTH_FRAME_LEN);
        if (rc) { free(buf); return rc; }
    }
    l3_authorization_frame_stream_clear(out);
    out->type_word = 0;
    out->byte_len = length;
    out->frame_count = frames;
    out->frames = buf;
    return L3_AUTH_OK;
}
