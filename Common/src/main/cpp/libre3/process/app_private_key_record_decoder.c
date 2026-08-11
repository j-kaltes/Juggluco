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
#include "app_private_key_record_decoder.h"

#include <string.h>

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void write_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

static int validate_private_key_tables(const l3_private_key_record_decoder_tables *t) {
    if (!t || !t->state_transition_table || !t->frame_normalization_program || !t->payload_decode_program || !t->payload_initial_state || !t->mask_frame_program) return L3_PRIVATE_KEY_ERR_ARGUMENT;
    if (t->state_transition_table_len < 0x20000u || t->frame_normalization_program_len < 0x84u ||
        t->payload_decode_program_len < 0x1300u || t->payload_initial_state_len < 0x142u || t->mask_frame_program_len < 0x200u) return L3_PRIVATE_KEY_ERR_TABLE_SIZE;
    return L3_PRIVATE_KEY_OK;
}

int l3_private_key_parse_record_header(const uint8_t *key, size_t key_len, l3_private_key_record_header *out) {
    if (!key || !out || key_len < 0xa5u) return L3_PRIVATE_KEY_ERR_ARGUMENT;
    memset(out, 0, sizeof(*out));
    memcpy(out->magic, key, 4);
    out->mode = key[4];
    out->key_index = read_be32(key + 0x05);
    out->bit_len = read_be32(key + 0x09);
    out->selected_index = read_be32(key + 0x0d);
    out->byte_len = read_be32(key + 0x5d);
    out->block_count = (out->byte_len + 0x10u) >> 4;
    return L3_PRIVATE_KEY_OK;
}

/* Generic copy of DAT_f4295564/table state-machine transform. */
static int table_state_transform(const uint8_t *dat429, const uint8_t *table,
                           uint32_t param1, uint32_t param2,
                           const uint8_t *p3, const uint8_t *p4,
                           uint8_t *out, size_t out_len) {
    uint32_t uvar4 = ((param2 << 14) & 0xffffffffu) >> 18;
    uint32_t rem = param2 >> 18;
    size_t need = (size_t)uvar4 + (size_t)rem;
    uint8_t state = 0;
    if (!dat429 || !table || !p3 || !p4 || !out) return L3_PRIVATE_KEY_ERR_ARGUMENT;
    if (out_len < need) return L3_PRIVATE_KEY_ERR_SHORT_OUT;
    for (uint32_t i = 0; i < uvar4; ++i) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ (uint32_t)p3[i]) | ((uint32_t)p4[i] << 8)) ^
                       ((uint32_t)table[i + (param1 & 0x3fffffu)] << 11);
        state = dat429[idx];
        out[i] = (uint8_t)(state & 7u);
    }
    for (uint32_t i = 0; i < rem; ++i) {
        uint32_t idx = ((uint32_t)(state & 0xf8u) | ((uint32_t)p4[uvar4 + i] << 8)) ^
                       ((uint32_t)table[i + (param1 & 0x3fffffu) + uvar4] << 11);
        state = dat429[idx];
        out[uvar4 + i] = (uint8_t)(state & 7u);
    }
    return L3_PRIVATE_KEY_OK;
}

/* Generic copy of FUN_f3efd128-style transform. */
static int table_state_transform_with_skip(const uint8_t *dat429, const uint8_t *table,
                          uint32_t param1, uint32_t param2,
                          const uint8_t *p3, const uint8_t *p4,
                          uint8_t *out, size_t out_len) {
    uint32_t uvar7 = ((param2 << 14) & 0xffffffffu) >> 18;
    uint32_t rem = param2 >> 18;
    uint32_t uvar8 = (param1 >> 22) | ((param2 & 0xfu) << 10);
    uint32_t uvar4 = param1 & 0x3fffffu;
    size_t need = (size_t)uvar7 + (size_t)rem;
    uint8_t state = 0;
    if (!dat429 || !table || !p3 || !p4 || !out) return L3_PRIVATE_KEY_ERR_ARGUMENT;
    if (out_len < need) return L3_PRIVATE_KEY_ERR_SHORT_OUT;
    for (uint32_t i = 0; i < uvar8; ++i) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ (uint32_t)p3[i]) | ((uint32_t)p4[i] << 8)) ^
                       ((uint32_t)table[i + uvar4] << 11);
        state = dat429[idx];
    }
    for (uint32_t i = 0; i < uvar7; ++i) {
        uint32_t idx = (((uint32_t)(state & 0xf8u) ^ (uint32_t)p3[uvar8 + i]) |
                       ((uint32_t)p4[uvar8 + i] << 8)) ^ ((uint32_t)table[i + uvar4 + uvar8] << 11);
        state = dat429[idx];
        out[i] = (uint8_t)(state & 7u);
    }
    for (uint32_t i = 0; i < rem; ++i) {
        uint32_t idx = (uint32_t)(state & 0xf8u) | ((uint32_t)table[i + uvar4 + uvar7 + uvar8] << 11);
        state = dat429[idx];
        out[uvar7 + i] = (uint8_t)(state & 7u);
    }
    return L3_PRIVATE_KEY_OK;
}

int l3_private_key_normalize_masked_frames(const l3_private_key_record_decoder_tables *t,
                                        const uint8_t *masked_frames, size_t masked_frames_len,
                                        uint8_t *out_normalized_frames, size_t out_len) {
    int rc = validate_private_key_tables(t);
    if (rc) return rc;
    if (!masked_frames || !out_normalized_frames) return L3_PRIVATE_KEY_ERR_ARGUMENT;
    if (out_len < masked_frames_len) return L3_PRIVATE_KEY_ERR_SHORT_OUT;
    size_t off = 0;
    while (off < masked_frames_len) {
        size_t block = masked_frames_len - off;
        if (block > 0x42u) block = 0x42u;
        rc = table_state_transform(t->state_transition_table, t->frame_normalization_program, 0, 0x420u,
                             masked_frames + off, masked_frames + off, out_normalized_frames + off, block);
        if (rc) return rc;
        off += block;
    }
    return L3_PRIVATE_KEY_OK;
}

static void build_private_key_decode_frame(uint8_t frame[0x142], const uint8_t seed_0x102[0x102],
                              uint32_t u2, uint32_t u5, uint32_t u6, uint32_t u7) {
    memset(frame, 0, 0x142);
    write_le32(frame + 0x00, u2); write_le32(frame + 0x04, u5); write_le32(frame + 0x08, u6); write_le32(frame + 0x0c, u7);
    write_le32(frame + 0x10, u2); write_le32(frame + 0x14, u5); write_le32(frame + 0x18, u6); write_le32(frame + 0x1c, u7);
    write_le32(frame + 0x20, u2); write_le32(frame + 0x24, u5); write_le32(frame + 0x28, u6);
    frame[0x2c] = (uint8_t)(u7 & 0xffu); frame[0x2d] = (uint8_t)((u7 >> 8) & 0xffu); frame[0x2e] = (uint8_t)((u7 >> 16) & 0xffu);
    frame[0x2f] = (uint8_t)(u2 & 0xffu); frame[0x30] = (uint8_t)((u2 >> 8) & 0xffu); frame[0x31] = (uint8_t)((u2 >> 16) & 0xffu); frame[0x32] = (uint8_t)((u2 >> 24) & 0xffu);
    write_le32(frame + 0x33, u5); write_le32(frame + 0x37, u6); write_le32(frame + 0x3b, u7);
    frame[0x3f] = 6;
    memcpy(frame + 0x40, seed_0x102, 0x102);
}

int l3_private_key_build_decoded_payload(const l3_private_key_record_decoder_tables *t,
                                     const uint8_t *normalized_frames, size_t normalized_frames_len,
                                     uint32_t bit_len,
                                     uint8_t *out_payload, size_t out_len) {
    int rc = validate_private_key_tables(t);
    if (rc) return rc;
    if (!normalized_frames || !out_payload) return L3_PRIVATE_KEY_ERR_ARGUMENT;
    if (out_len < L3_PRIVATE_KEY_PAYLOAD_LEN) return L3_PRIVATE_KEY_ERR_SHORT_OUT;

    uint8_t ctx[0x142];
    uint8_t frame[0x142];
    uint8_t tmp[0x142];
    memset(ctx, 0, sizeof(ctx));
    memcpy(ctx, t->payload_initial_state, t->payload_initial_state_len < sizeof(ctx) ? t->payload_initial_state_len : sizeof(ctx));

    uint32_t uvar3 = ((bit_len + 7u) >> 3) + 0xfu;
    uint32_t block_count = uvar3 >> 4;
    size_t ptr = 0;
    for (uint32_t b = 0; b < block_count; ++b) {
        if (ptr + 0x42u > normalized_frames_len) return L3_PRIVATE_KEY_ERR_ARGUMENT;
        uint8_t local_frame[0x142];
        build_private_key_decode_frame(local_frame, ctx, 0, 0, 0, 0);
        rc = table_state_transform(t->state_transition_table, t->payload_decode_program, 0x1116u, 0x1420u,
                             local_frame, local_frame, ctx, sizeof(ctx));
        if (rc) return rc;
        rc = table_state_transform_with_skip(t->state_transition_table, t->payload_decode_program, 0x2b0u, 0x4000420u,
                            normalized_frames + ptr, normalized_frames + ptr, frame, sizeof(frame));
        if (rc) return rc;
        rc = table_state_transform(t->state_transition_table, t->payload_decode_program, 0x466u, 0x1420u,
                             frame, ctx, tmp, sizeof(tmp));
        if (rc) return rc;
        memcpy(ctx, tmp, sizeof(ctx));
        ptr += 0x42u;
    }

    if ((uvar3 & 0xfu) != 0xfu) {
        uint32_t count = (uvar3 & 0xfu) ^ 0xfu;
        for (uint32_t i = 0; i < count; ++i) {
            rc = table_state_transform_with_skip(t->state_transition_table, t->payload_decode_program, 0x10005c8u, 0x1013e0u,
                                ctx, ctx, tmp, sizeof(tmp));
            if (rc) return rc;
            memcpy(ctx, tmp, sizeof(ctx));
        }
    }

    return table_state_transform(t->state_transition_table, t->payload_decode_program, 0x174u, 0x10a0u,
                           ctx, ctx, out_payload, L3_PRIVATE_KEY_PAYLOAD_LEN);
}

int l3_private_key_build_payload_from_masked_frames(const l3_private_key_record_decoder_tables *tables,
                                      const uint8_t *masked_frames, size_t masked_frames_len,
                                      uint32_t bit_len,
                                      uint8_t *out_payload, size_t out_len) {
    uint8_t pv[L3_PRIVATE_KEY_NORMALIZED_FRAMES_LEN];
    if (masked_frames_len > sizeof(pv)) return L3_PRIVATE_KEY_ERR_ARGUMENT;
    int rc = l3_private_key_normalize_masked_frames(tables, masked_frames, masked_frames_len, pv, sizeof(pv));
    if (rc) return rc;
    return l3_private_key_build_decoded_payload(tables, pv, masked_frames_len, bit_len, out_payload, out_len);
}




/* FUN_f3f02b18 / DAT_f40e8251 transform. */
static int private_key_mask_table_transform(const l3_private_key_record_decoder_tables *t,
                           uint32_t param1, uint32_t param2,
                           const uint8_t *p3, const uint8_t *p4,
                           uint8_t *out, size_t out_len) {
    return table_state_transform(t->state_transition_table, t->mask_frame_program, param1, param2, p3, p4, out, out_len);
}

static void build_private_key_mask_bits_frame(const uint8_t prev16[16], const uint8_t mask16[16], uint8_t frame66[66]) {
    uint8_t x[16];
    uint8_t seq[16];
    for (unsigned i = 0; i < 16; ++i) x[i] = (uint8_t)(prev16[i] ^ mask16[i]);

    /*
     * This matches FUN_f3f03eb8 stack layout:
     *   local_50 = CONCAT13(x[12], x[13], x[14], x[15])
     *   local_4c = CONCAT13(x[ 8], x[ 9], x[10], x[11])
     *   local_48 = CONCAT13(x[ 4], x[ 5], x[ 6], x[ 7])
     *   local_44 = CONCAT13(x[ 0], x[ 1], x[ 2], x[ 3])
     * then bytes are read from &local_50 on little-endian ARM.
     */
    seq[0] = x[15]; seq[1] = x[14]; seq[2] = x[13]; seq[3] = x[12];
    seq[4] = x[11]; seq[5] = x[10]; seq[6] = x[9];  seq[7] = x[8];
    seq[8] = x[7];  seq[9] = x[6];  seq[10]= x[5];  seq[11]= x[4];
    seq[12]= x[3];  seq[13]= x[2];  seq[14]= x[1];  seq[15]= x[0];

    memset(frame66, 0, 66);
    frame66[0] = 0x00;
    frame66[1] = 0x05; /* local_92 = 0x500, little-endian */
    for (unsigned j = 0, u = 0; j < 64; ++j, u += 2) {
        frame66[2 + j] = (uint8_t)((seq[j >> 2] >> (u & 6u)) & 3u);
    }
}

int l3_private_key_build_mask_frames(const l3_private_key_record_decoder_tables *t,
                                      const uint8_t *key, size_t key_len,
                                      const uint8_t manager_mask_b4_16[16],
                                      uint8_t *out_mask_frames,
                                      size_t out_len) {
    int rc = validate_private_key_tables(t);
    if (rc) return rc;
    if (!key || !manager_mask_b4_16 || !out_mask_frames) return L3_PRIVATE_KEY_ERR_ARGUMENT;

    l3_private_key_record_header hdr;
    rc = l3_private_key_parse_record_header(key, key_len, &hdr);
    if (rc) return rc;
    size_t need = (size_t)hdr.block_count * 0x42u;
    if (out_len < need || key_len < 0x61u + (size_t)hdr.block_count * 0x10u) return L3_PRIVATE_KEY_ERR_SHORT_OUT;

    uint8_t frame66[66];
    const uint8_t *prev = key + 0x31u;       /* abStack_44 from FUN_f3f03878: param_8 = key+0x31 */
    const uint8_t *cursor = key + 0x61u;
    for (uint32_t b = 0; b < hdr.block_count; ++b) {
        build_private_key_mask_bits_frame(prev, manager_mask_b4_16, frame66);
        rc = private_key_mask_table_transform(t, 0x42u, 0x420u, frame66, frame66, out_mask_frames + b * 0x42u, 0x42u);
        if (rc) return rc;
        prev = cursor;
        cursor += 0x10u;
    }
    return L3_PRIVATE_KEY_OK;
}

int l3_private_key_combine_object_outputs_with_mask_frames(const l3_private_key_record_decoder_tables *t,
                                             const uint8_t *key, size_t key_len,
                                             const uint8_t manager_mask_b4_16[16],
                                             const uint8_t *object_outputs,
                                             size_t object_outputs_len,
                                             uint8_t *out_masked_frames,
                                             size_t out_len) {
    int rc = validate_private_key_tables(t);
    if (rc) return rc;
    if (!object_outputs || !out_masked_frames) return L3_PRIVATE_KEY_ERR_ARGUMENT;
    l3_private_key_record_header hdr;
    rc = l3_private_key_parse_record_header(key, key_len, &hdr);
    if (rc) return rc;
    size_t need = (size_t)hdr.block_count * 0x42u;
    if (object_outputs_len < need || out_len < need) return L3_PRIVATE_KEY_ERR_SHORT_OUT;

    uint8_t mask_frames[L3_PRIVATE_KEY_MASKED_FRAMES_LEN];
    if (need > sizeof(mask_frames)) return L3_PRIVATE_KEY_ERR_SHORT_OUT;
    rc = l3_private_key_build_mask_frames(t, key, key_len, manager_mask_b4_16, mask_frames, sizeof(mask_frames));
    if (rc) return rc;

    for (uint32_t b = 0; b < hdr.block_count; ++b) {
        rc = private_key_mask_table_transform(t, 0x84u, 0x420u,
                             object_outputs + b * 0x42u,
                             mask_frames + b * 0x42u,
                             out_masked_frames + b * 0x42u,
                             0x42u);
        if (rc) return rc;
    }
    return L3_PRIVATE_KEY_OK;
}



int l3_private_key_decode_blocks_with_object_method(
    const l3_private_key_record_decoder_tables *tables,
    l3_private_key_private_key_object_method_cb cb,
    void *cb_user,
    const uint8_t *private_key_record,
    size_t private_key_record_len,
    const uint8_t *manager_a8_selected_0x424,
    size_t manager_a8_selected_len,
    const uint8_t manager_mask_b4_16[16],
    uint8_t *out_masked_frames,
    size_t out_len) {
    (void)manager_a8_selected_0x424;
    (void)manager_a8_selected_len;
    if (!private_key_record || !manager_mask_b4_16 || !out_masked_frames || !tables || !cb) {
        return L3_PRIVATE_KEY_ERR_ARGUMENT;
    }
    int rc = validate_private_key_tables(tables);
    if (rc) return rc;

    l3_private_key_record_header hdr;
    rc = l3_private_key_parse_record_header(private_key_record, private_key_record_len, &hdr);
    if (rc) return rc;
    size_t need = (size_t)hdr.block_count * 0x42u;
    if (need > L3_PRIVATE_KEY_OBJECT_OUTPUTS_LEN || out_len < need ||
        private_key_record_len < 0x61u + (size_t)hdr.block_count * 0x10u) {
        return L3_PRIVATE_KEY_ERR_SHORT_OUT;
    }

    uint8_t object_outputs[L3_PRIVATE_KEY_OBJECT_OUTPUTS_LEN];
    memset(object_outputs, 0, sizeof(object_outputs));
    for (uint32_t b = 0; b < hdr.block_count; ++b) {
        rc = cb(cb_user,
                private_key_record + 0x61u + (size_t)b * 0x10u,
                object_outputs + (size_t)b * 0x42u);
        if (rc) return rc;
    }

    return l3_private_key_combine_object_outputs_with_mask_frames(tables,
                                                   private_key_record,
                                                   private_key_record_len,
                                                   manager_mask_b4_16,
                                                   object_outputs, need,
                                                   out_masked_frames, out_len);
}


