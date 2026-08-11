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
#include "authorization_digest.h"

#include <string.h>
#ifdef L3_AUTHORIZATION_DIGEST_DEBUG_TRACE
#include <stdio.h>
#endif

enum {
    ST_TOTAL_LO = 0x000,
    ST_TOTAL_HI = 0x004,
    ST_BLOCKS = 0x008,
    ST_BLOCK_INDEX = 0x110,
    ST_LANES = 0x114,
    ST_RAW_DIRTY = 0x1a4,
    ST_RAW_PENDING = 0x1a5,
    ST_RAW_PENDING_LEN = 0x1e8,
    ST_RAW_WORDS = 0x1ec
};

static uint32_t rd32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t rd32be(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void wr32le(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static int validate_digest_tables(const l3_authorization_digest_tables *t) {
    if (!t || !t->state_transition_table || !t->digest_update_program ||
        !t->digest_initialization_program) {
        return L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT;
    }
    if (t->state_transition_table_len < 0x20000u ||
        t->digest_update_program_len < 0x1d60u ||
        t->digest_initialization_program_len < 0x120u) {
        return L3_AUTHORIZATION_DIGEST_ERR_TABLE_SIZE;
    }
    return L3_AUTHORIZATION_DIGEST_OK;
}

static int digest_table_transform(const l3_authorization_digest_tables *t,
                   uint32_t p1, uint32_t p2,
                   const uint8_t *p3, const uint8_t *p4, uint8_t *p5) {
    uint32_t whole = (p2 << 14) >> 18;
    uint32_t rem = p2 >> 18;
    uint32_t base = p1 & 0x3fffffu;
    uint32_t state = 0;
    size_t count = (size_t)whole + rem;
    if (!p3 || !p4 || !p5 || (size_t)base + count > t->digest_update_program_len) {
        return L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT;
    }
    for (uint32_t i = 0; i < whole; ++i) {
        uint32_t index = (((state & 0xf8u) ^ p3[i]) |
                          ((uint32_t)p4[i] << 8)) ^
                         ((uint32_t)t->digest_update_program[base + i] << 11);
        if (index >= t->state_transition_table_len) return L3_AUTHORIZATION_DIGEST_ERR_TABLE_SIZE;
        state = t->state_transition_table[index];
        p5[i] = (uint8_t)(state & 7u);
    }
    for (uint32_t i = 0; i < rem; ++i) {
        uint32_t index = ((state & 0xf8u) |
                          ((uint32_t)p4[whole + i] << 8)) ^
                         ((uint32_t)t->digest_update_program[base + whole + i] << 11);
        if (index >= t->state_transition_table_len) return L3_AUTHORIZATION_DIGEST_ERR_TABLE_SIZE;
        state = t->state_transition_table[index];
        p5[whole + i] = (uint8_t)(state & 7u);
    }
    return L3_AUTHORIZATION_DIGEST_OK;
}

static int digest_table_transform_with_skip(const l3_authorization_digest_tables *t,
                   uint32_t p1, uint32_t p2,
                   const uint8_t *p3, const uint8_t *p4, uint8_t *p5) {
    uint32_t out_count = (p2 << 14) >> 18;
    uint32_t skip = (p1 >> 22) | ((p2 & 0xfu) << 10);
    uint32_t rem = p2 >> 18;
    uint32_t base = p1 & 0x3fffffu;
    uint32_t state = 0;
    size_t table_count = (size_t)skip + out_count + rem;
    if (!p3 || !p4 || !p5 ||
        (size_t)base + table_count > t->digest_update_program_len) {
        return L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT;
    }
    for (uint32_t i = 0; i < skip; ++i) {
        uint32_t index = (((state & 0xf8u) ^ p3[i]) |
                          ((uint32_t)p4[i] << 8)) ^
                         ((uint32_t)t->digest_update_program[base + i] << 11);
        if (index >= t->state_transition_table_len) return L3_AUTHORIZATION_DIGEST_ERR_TABLE_SIZE;
        state = t->state_transition_table[index];
    }
    for (uint32_t i = 0; i < out_count; ++i) {
        uint32_t pos = skip + i;
        uint32_t index = (((state & 0xf8u) ^ p3[pos]) |
                          ((uint32_t)p4[pos] << 8)) ^
                         ((uint32_t)t->digest_update_program[base + pos] << 11);
        if (index >= t->state_transition_table_len) return L3_AUTHORIZATION_DIGEST_ERR_TABLE_SIZE;
        state = t->state_transition_table[index];
        p5[i] = (uint8_t)(state & 7u);
    }
    for (uint32_t i = 0; i < rem; ++i) {
        uint32_t pos = skip + out_count + i;
        uint32_t index = (state & 0xf8u) |
                         ((uint32_t)t->digest_update_program[base + pos] << 11);
        if (index >= t->state_transition_table_len) return L3_AUTHORIZATION_DIGEST_ERR_TABLE_SIZE;
        state = t->state_transition_table[index];
        p5[out_count + i] = (uint8_t)(state & 7u);
    }
    return L3_AUTHORIZATION_DIGEST_OK;
}

static uint32_t rotr32(uint32_t value, unsigned shift) {
    return (value >> shift) | (value << (32u - shift));
}

static void sha256_compress(uint32_t state[8], const uint8_t block[64]) {
    static const uint32_t k[64] = {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
        0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
        0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
        0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
        0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
        0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
        0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
        0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
        0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    uint32_t w[64];
    for (unsigned i = 0; i < 16u; ++i) w[i] = rd32be(block + i * 4u);
    for (unsigned i = 16u; i < 64u; ++i) {
        uint32_t s0 = rotr32(w[i - 15u], 7) ^ rotr32(w[i - 15u], 18) ^
                      (w[i - 15u] >> 3);
        uint32_t s1 = rotr32(w[i - 2u], 17) ^ rotr32(w[i - 2u], 19) ^
                      (w[i - 2u] >> 10);
        w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
    }
    uint32_t a=state[0],b=state[1],c=state[2],d=state[3];
    uint32_t e=state[4],f=state[5],g=state[6],h=state[7];
    for (unsigned i = 0; i < 64u; ++i) {
        uint32_t s1 = rotr32(e,6) ^ rotr32(e,11) ^ rotr32(e,25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + s1 + ch + k[i] + w[i];
        uint32_t s0 = rotr32(a,2) ^ rotr32(a,13) ^ rotr32(a,22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = s0 + maj;
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
}

static void sha256_shadow_begin(uint8_t state[0x210], uint32_t mode_bytes) {
    static const uint32_t sha256_iv[8] = {
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    static const uint32_t sha224_iv[8] = {
        0xc1059ed8u,0x367cd507u,0x3070dd17u,0xf70e5939u,
        0xffc00b31u,0x68581511u,0x64f98fa7u,0xbefa4fa4u
    };
    const uint32_t *iv = mode_bytes == 0x1cu ? sha224_iv : sha256_iv;
    for (unsigned i = 0; i < 8u; ++i) wr32le(state + ST_RAW_WORDS + 4u*i, iv[i]);
    state[ST_RAW_DIRTY] = 1u;
}

static void sha256_shadow_update(uint8_t state[0x210], const uint8_t *data,
                              size_t len, uint32_t mode_bytes) {
    if (!state[ST_RAW_DIRTY]) sha256_shadow_begin(state, mode_bytes);
    uint32_t pending = rd32le(state + ST_RAW_PENDING_LEN);
    uint32_t words[8];
    for (unsigned i = 0; i < 8u; ++i) words[i] = rd32le(state + ST_RAW_WORDS + 4u*i);
    while (len) {
        size_t take = 64u - pending;
        if (take > len) take = len;
        memcpy(state + ST_RAW_PENDING + pending, data, take);
        pending += (uint32_t)take;
        data += take;
        len -= take;
        if (pending == 64u) {
            sha256_compress(words, state + ST_RAW_PENDING);
            uint32_t old = rd32le(state + ST_TOTAL_LO);
            wr32le(state + ST_TOTAL_LO, old + 64u);
            if (old + 64u < old) {
                wr32le(state + ST_TOTAL_HI, rd32le(state + ST_TOTAL_HI) + 1u);
            }
            pending = 0;
        }
    }
    wr32le(state + ST_RAW_PENDING_LEN, pending);
    for (unsigned i = 0; i < 8u; ++i) wr32le(state + ST_RAW_WORDS + 4u*i, words[i]);
}

static void encode_word18(uint32_t value, uint16_t tag, uint8_t out[18]) {
    out[0] = (uint8_t)tag;
    out[1] = (uint8_t)(tag >> 8);
    for (unsigned i = 0; i < 16u; ++i) {
        out[2u+i] = (uint8_t)((value >> (2u*i)) & 3u);
    }
}

static int digest_compress_generated_state(l3_authorization_digest_context *ctx);
static int digest_absorb_encoded_block(l3_authorization_digest_context *ctx,
                       const uint8_t input[66], uint32_t byte_count);

static int digest_encode_pending_bytes(l3_authorization_digest_context *ctx,
                             const uint8_t *bytes, uint32_t len) {
    while (len) {
        uint32_t take = len > 16u ? 16u : len;
        uint8_t reversed[16] = {0};
        uint8_t canonical[66] = {0};
        uint8_t encoded[66];
        canonical[1] = 1u;
        for (uint32_t i = 0; i < take; ++i) reversed[15u-i] = bytes[i];
        for (unsigned i = 0; i < 64u; ++i) {
            canonical[2u+i] = (uint8_t)((reversed[i >> 2] >> ((2u*i) & 6u)) & 3u);
        }
        int rc = digest_table_transform(&ctx->tables, 0xdf7u, 0x420u,
                        canonical, canonical, encoded);
        if (rc) return rc;
        rc = digest_absorb_encoded_block(ctx, encoded, take);
        if (rc) return rc;
        bytes += take;
        len -= take;
    }
    return L3_AUTHORIZATION_DIGEST_OK;
}

static int digest_flush_pending_bytes(l3_authorization_digest_context *ctx) {
    uint8_t *state = ctx->state;
    if (!state[ST_RAW_DIRTY]) return L3_AUTHORIZATION_DIGEST_OK;
    static const uint16_t tags[8] = {
        0x0707u,0x0406u,0x0501u,0x0205u,
        0x0503u,0x0705u,0x0007u,0x0605u
    };
    static const uint16_t bases[8] = {
        0x0e4bu,0x05b0u,0x105eu,0x028bu,
        0x0c1fu,0x0f26u,0x02f8u,0x0c85u
    };
    static const uint16_t lane_offsets[8] = {
        0x114u,0x126u,0x138u,0x14au,
        0x15cu,0x16eu,0x180u,0x192u
    };
    state[ST_RAW_DIRTY] = 0u;
    for (unsigned i = 0; i < 8u; ++i) {
        uint8_t frame[18];
        encode_word18(rd32le(state + ST_RAW_WORDS + 4u*i), tags[i], frame);
        int rc = digest_table_transform(&ctx->tables, bases[i], 0x120u,
                        frame, frame, state + lane_offsets[i]);
        if (rc) return rc;
    }
    uint32_t pending = rd32le(state + ST_RAW_PENDING_LEN);
    int rc = digest_encode_pending_bytes(ctx, state + ST_RAW_PENDING, pending);
    if (rc) return rc;
    wr32le(state + ST_RAW_PENDING_LEN, 0u);
    return L3_AUTHORIZATION_DIGEST_OK;
}

static int digest_absorb_encoded_block(l3_authorization_digest_context *ctx,
                       const uint8_t input[66], uint32_t byte_count) {
    uint8_t *state = ctx->state;
    if (byte_count == 0u) return L3_AUTHORIZATION_DIGEST_OK;
    uint32_t mod = rd32le(state + ST_TOTAL_LO) & 0x0fu;
    uint32_t first = 16u - mod;
    uint32_t block_index = rd32le(state + ST_BLOCK_INDEX);
    if (block_index >= 4u) return L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT;
    uint8_t *current = state + ST_BLOCKS + block_index * 66u;
    int rc;
    if (mod == 0u) {
        rc = digest_table_transform(&ctx->tables, 0x340u, 0x420u,
                    input, input, current);
        if (rc) return rc;
        if (byte_count < 16u) goto add_length;
    } else {
        uint8_t shifted[72] = {0};
        uint8_t combined[66];
        rc = digest_table_transform(&ctx->tables, 0x7d1u, 0x420u,
                    input, input, shifted);
        if (rc) return rc;
        for (uint32_t i = 0; i < mod; ++i) {
            rc = digest_table_transform_with_skip(&ctx->tables, 0x1000000u, 0x1003e0u,
                        shifted, shifted, shifted);
            if (rc) return rc;
        }
        const uint8_t *padding = ctx->tables.digest_update_program +
                                 0x1070u + ((mod ^ 0x0fu) * 66u);
        rc = digest_table_transform(&ctx->tables, 0x0b5u, 0x420u,
                    current, padding, combined);
        if (rc) return rc;
        rc = digest_table_transform(&ctx->tables, 0x454u, 0x420u,
                    combined, shifted, current);
        if (rc) return rc;
        if (first > byte_count) goto add_length;
    }

    block_index++;
    wr32le(state + ST_BLOCK_INDEX, block_index);
    if (block_index == 4u) {
        rc = digest_compress_generated_state(ctx);
        if (rc) return rc;
        block_index = 0u;
        wr32le(state + ST_BLOCK_INDEX, 0u);
    }
    if (first < byte_count) {
        uint8_t rolling[66];
        uint8_t shifted[66];
        rc = digest_table_transform(&ctx->tables, 0xb7au, 0x420u,
                    input, input, rolling);
        if (rc) return rc;
        for (uint32_t i = mod; i < 16u; ++i) {
            memset(shifted, 0, 4u);
            shifted[3] = 6u;
            memcpy(shifted + 4u, rolling, 62u);
            rc = digest_table_transform(&ctx->tables, 0x60fu, 0x420u,
                        shifted, shifted, rolling);
            if (rc) return rc;
        }
        current = state + ST_BLOCKS + block_index * 66u;
        rc = digest_table_transform(&ctx->tables, 0xc43u, 0x420u,
                    rolling, rolling, current);
        if (rc) return rc;
    }

add_length:
    {
        uint32_t old = rd32le(state + ST_TOTAL_LO);
        uint32_t now = old + byte_count;
        wr32le(state + ST_TOTAL_LO, now);
        if (now < old) wr32le(state + ST_TOTAL_HI,
                              rd32le(state + ST_TOTAL_HI) + 1u);
    }
    return L3_AUTHORIZATION_DIGEST_OK;
}

static int digest_compress_generated_state(l3_authorization_digest_context *ctx) {
#ifdef L3_AUTHORIZATION_DIGEST_SKIP_COMPRESSION
    (void)ctx;
    return L3_AUTHORIZATION_DIGEST_OK;
#else
    /* FUN_f3f0c53c uses several deliberately overlapping stack views while
     * expanding its 16 input words.  Keep the original 0x650-byte address
     * geometry instead of relying on a compiler's layout of C locals. */
    uint8_t stack[0x650] = {0};
    uint8_t *lanes = ctx->state + ST_LANES;
    uint8_t *blocks = ctx->state + ST_BLOCKS;
    int rc;
#ifdef L3_AUTHORIZATION_DIGEST_DEBUG_TRACE
    unsigned trace_sequence = 0;
#define L3_TRACE(kind, base, p2, out)                                      \
    do {                                                                  \
        uint32_t trace_hash = 0x811c9dc5u;                                \
        uint32_t trace_len = (((uint32_t)(p2) << 14) >> 18) +             \
                             ((uint32_t)(p2) >> 18);                       \
        for (uint32_t trace_i = 0; trace_i < trace_len; ++trace_i)        \
            trace_hash = (trace_hash ^ (out)[trace_i]) * 0x01000193u;     \
        fprintf(stderr, "C53C %04u %c %08x %08x %u %08x\n",              \
                trace_sequence++, (kind), (unsigned)(base),               \
                (unsigned)(p2), (unsigned)trace_len, trace_hash);         \
    } while (0)
#else
#define L3_TRACE(kind, base, p2, out) do { (void)(out); } while (0)
#endif
#define L3_L(suffix) (stack + (0x650u - (suffix)))
#define L3_BN(base, p2, left, right, out)                                 \
    do {                                                                  \
        rc = digest_table_transform(&ctx->tables, (base), (p2),                           \
                    (left), (right), (out));                              \
        if (rc != L3_AUTHORIZATION_DIGEST_OK) return rc;                             \
        L3_TRACE('B', (base), (p2), (out));                               \
    } while (0)
#define L3_B(base, left, right, out)                                      \
    L3_BN((base), 0x120u, (left), (right), (out))
#define L3_C(p1, p2, left, right, out)                                    \
    do {                                                                  \
        rc = digest_table_transform_with_skip(&ctx->tables, (p1), (p2),                             \
                    (left), (right), (out));                              \
        if (rc != L3_AUTHORIZATION_DIGEST_OK) return rc;                             \
        L3_TRACE('C', (p1), (p2), (out));                                 \
    } while (0)

    uint8_t *s64c = L3_L(0x64c), *s63a = L3_L(0x63a);
    uint8_t *s628 = L3_L(0x628), *s616 = L3_L(0x616);
    uint8_t *s604 = L3_L(0x604), *s5f2 = L3_L(0x5f2);
    uint8_t *s5e0 = L3_L(0x5e0), *s5ce = L3_L(0x5ce);
    uint8_t *s5bc = L3_L(0x5bc), *s5aa = L3_L(0x5aa);
    uint8_t *s598 = L3_L(0x598), *s586 = L3_L(0x586);
    uint8_t *s574 = L3_L(0x574), *s562 = L3_L(0x562);
    uint8_t *s550 = L3_L(0x550), *s53e = L3_L(0x53e);
    uint8_t *s52c = L3_L(0x52c), *s52a = L3_L(0x52a);
    uint8_t *s522 = L3_L(0x522), *s49c = L3_L(0x49c);
    uint8_t *s442 = L3_L(0x442), *s41e = L3_L(0x41e);
    uint8_t *sbe = L3_L(0xbe), *sac = L3_L(0xac);
    uint8_t *s9a = L3_L(0x9a), *s78 = L3_L(0x78);
    uint8_t *s50 = L3_L(0x50), *s3e = L3_L(0x3e);

    /* Convert the four 66-byte buffered blocks into the 16 initial
     * 18-byte schedule words. */
    for (unsigned block = 0; block < 4u; ++block) {
        const uint8_t *in = blocks + block * 66u;
        uint8_t *out = s53e + block * 72u;
        L3_BN(0x4dcu, 0x220u, in, in, s78);
        L3_C(0x08000d4fu, 0x220u, in, in, s9a);
        L3_B(0x046u, s78, s78, out + 0x36u);
        L3_C(0x0400093fu, 0x120u, s78, s78, out + 0x24u);
        L3_B(0x21bu, s9a, s9a, out + 0x12u);
        L3_C(0x04000496u, 0x120u, s9a, s9a, out);
    }
#ifdef L3_AUTHORIZATION_DIGEST_DEBUG_TRACE
    fputs("C53C_AFTER18 ", stderr);
    for (unsigned trace_i = 0; trace_i < 34u; ++trace_i)
        fprintf(stderr, "%02x", s78[trace_i]);
    fputc('\n', stderr);
#endif

    /* Expand words 16..63.  The source windows intentionally overlap the
     * destination schedule just as they do in the native stack frame. */
    for (unsigned off = 0; off < 0x360u; off += 0x12u) {
        uint8_t *p442 = s442 + off;
        uint8_t *p52c = s52c + off;
        L3_C(0x02400bf2u, 0x240090u, p442, p442, s3e);
        L3_B(0xf02u, p442, p442, s9a);
        wr32le(s78, 0u);
        s78[4] = 0u;
        s78[5] = 0u;
        s78[6] = 5u;
        memcpy(s78 + 7u, s9a, 2u);
        memcpy(s78 + 9u, s9a + 2u, 4u);
        memcpy(s78 + 13u, s9a + 6u, 4u);
        s78[17] = s9a[10];
#ifdef L3_AUTHORIZATION_DIGEST_DEBUG_TRACE
        if (off == 0u) {
            fputs("C53C_S9A25 ", stderr);
            for (unsigned trace_i = 0; trace_i < 18u; ++trace_i)
                fprintf(stderr, "%02x", s9a[trace_i]);
            fputc('\n', stderr);
            fputs("C53C_INPUT26 ", stderr);
            for (unsigned trace_i = 0; trace_i < 18u; ++trace_i)
                fprintf(stderr, "%02x", s78[trace_i]);
            fputc('\n', stderr);
        }
#endif
        L3_B(0x11bu, s78, s78, s50);
        L3_B(0xe39u, s50, s3e, sbe);

        L3_C(0x02800cebu, 0x280080u, p442, p442, s3e);
        L3_B(0x5ebu, p442, p442, s9a);
        wr32le(s78, 0u);
        s78[4] = 0u;
        s78[5] = 0u;
        memcpy(s78 + 6u, s9a, 2u);
        s78[8] = s9a[2];
        memcpy(s78 + 9u, s9a + 3u, 4u);
        memcpy(s78 + 13u, s9a + 7u, 4u);
        s78[17] = s9a[11];
        L3_B(0xfe0u, s78, s78, s50);
        L3_B(0xa9fu, s50, s3e, s78);

        L3_C(0x014005c2u, 0x1400d0u, p442, p442, s9a);
        L3_B(0x442u, sbe, s78, s3e);
        L3_B(0x997u, s9a, s3e, sac);
        L3_B(0x430u, sac, s49c + off, sbe);

        L3_C(0x0100022du, 0x1000e0u, p52c, p52c, s3e);
        L3_B(0x32eu, p52c, p52c, s9a);
        wr32le(s78, 0u);
        memset(s78 + 4u, 0, 8u);
        memcpy(s78 + 12u, s9a, 4u);
        memcpy(s78 + 16u, s9a + 4u, 2u);
        L3_B(0x0f7u, s78, s78, s50);
        L3_B(0x104cu, s50, s3e, s550);

        L3_C(0x02400b17u, 0x240090u, p52c, p52c, s9a);
        s78[17] = s522[off];
        uint32_t upper = rd32le(s52a + off + 4u);
        uint32_t lower = rd32le(s52a + off);
        s78[13] = (uint8_t)upper;
        s78[14] = (uint8_t)(upper >> 8);
        s78[15] = (uint8_t)(upper >> 16);
        s78[16] = (uint8_t)(upper >> 24);
        s78[9] = (uint8_t)lower;
        s78[10] = (uint8_t)(lower >> 8);
        s78[11] = (uint8_t)(lower >> 16);
        s78[12] = (uint8_t)(lower >> 24);
        s78[7] = p52c[0];
        s78[8] = p52c[1];
        s78[6] = 1u;
        wr32le(s78, 0u);
        s78[4] = 0u;
        s78[5] = 0u;
        L3_B(0xc0du, s78, s78, s3e);
        L3_B(0xda3u, s3e, s9a, s50);
        L3_C(0x00800f38u, 0x80100u, p52c, p52c, s9a);
        L3_B(0x279u, s550, s50, s3e);
        L3_B(0xea5u, s9a, s3e, s78);
        L3_B(0x1c1u, s78, s53e + off, s9a);
        L3_B(0xa09u, sbe, s9a, s41e + off);
    }

    /* Decode the eight chaining lanes. */
    L3_B(0x40cu, lanes, lanes, s550);
    L3_B(0x103au, lanes + 0x12u, lanes + 0x12u, s562);
    L3_B(0xe81u, lanes + 0x24u, lanes + 0x24u, s574);
    L3_B(0x382u, lanes + 0x36u, lanes + 0x36u, s586);
    L3_B(0xf14u, lanes + 0x48u, lanes + 0x48u, s598);
    L3_B(0xbceu, lanes + 0x5au, lanes + 0x5au, s5aa);
    L3_B(0x59eu, lanes + 0x6cu, lanes + 0x6cu, s5bc);
    L3_B(0x5fdu, lanes + 0x7eu, lanes + 0x7eu, s5ce);

    /* The 64 generated-domain SHA-256 rounds. */
    for (unsigned round = 0; round < 0x480u; round += 0x12u) {
        L3_C(0x00c00ec9u, 0xc00f0u, s598, s598, s9a);
        memset(s78, 0, 13u);
        s78[13] = s598[0];
        s78[14] = s598[1];
        s78[15] = s598[2];
        s78[16] = s598[3];
        s78[17] = s598[4];
        s78[12] = 1u;
        L3_B(0x709u, s78, s78, s3e);
        L3_B(0xb44u, s3e, s9a, s5f2);

        L3_C(0x01800520u, 0x1800c0u, s598, s598, s3e);
        L3_B(0x5d9u, s598, s598, s9a);
        wr32le(s78, 0u);
        s78[4] = 0u;
        s78[5] = 0u;
        s78[6] = 0u;
        s78[7] = 0u;
        s78[8] = 0u;
        s78[9] = 3u;
        memcpy(s78 + 10u, s9a, 8u);
        L3_B(0x9e5u, s78, s78, s50);
        L3_B(0x1f7u, s50, s3e, s604);

        L3_C(0x034009a9u, 0x340050u, s598, s598, s3e);
        L3_B(0x31cu, s598, s598, s9a);
        s78[0] = 0u;
        s78[1] = 0u;
        s78[2] = 1u;
        memcpy(s78 + 3u, s9a, 15u);
        L3_B(0x73du, s78, s78, s50);
        L3_B(0xc31u, s50, s3e, s78);
        L3_B(0x91bu, s5f2, s604, s9a);
        L3_B(0x973u, s78, s9a, s5e0);
        L3_B(0xd2bu, s5ce, s5e0, s5f2);
        L3_B(0x813u, ctx->tables.digest_update_program + 0x144eu + round,
             s53e + round, s604);
        L3_B(0x3b8u, s5f2, s604, s616);
        L3_B(0x30au, s598, s5aa, s78);
        L3_B(0xe93u, ctx->tables.digest_update_program + 0x18ceu, s598, s9a);
        L3_B(0x825u, s9a, s5bc, s3e);
        L3_B(0xbbcu, s78, s3e, s628);
        L3_B(0x86du, s616, s628, sac);

        L3_C(0x004002e5u, 0x40110u, s550, s550, s9a);
        uint8_t old2 = s550[2], old1 = s550[1], old0 = s550[0];
        memset(s78, 0, 14u);
        s78[14] = 1u;
        s78[15] = old0;
        s78[16] = old1;
        s78[17] = old2;
        L3_B(0xe6fu, s78, s78, s3e);
        L3_B(0x209u, s3e, s9a, s63a);

        L3_C(0x01c0008au, 0x1c00b0u, s550, s550, s3e);
        L3_B(0x1016u, s550, s550, s9a);
        s78[17] = s9a[8];
        s78[10] = s9a[1];
        s78[11] = s9a[2];
        wr32le(s78, 0u);
        s78[4] = 0u;
        s78[5] = 0u;
        s78[6] = 0u;
        s78[7] = 0u;
        s78[8] = 2u;
        memcpy(s78 + 9u, s9a, 9u);
        L3_B(0x58cu, s78, s78, s50);
        L3_B(0x1028u, s50, s3e, s64c);

        L3_C(0x02c009c8u, 0x2c0070u, s550, s550, s9a);
        s78[17] = s550[12];
        s78[13] = s550[8];
        s78[14] = s550[9];
        s78[15] = s550[10];
        s78[16] = s550[11];
        s78[9] = s550[4];
        s78[10] = s550[5];
        s78[11] = s550[6];
        s78[12] = s550[7];
        s78[8] = s550[3];
        s78[7] = old2;
        s78[6] = old1;
        s78[5] = old0;
        s78[4] = 6u;
        wr32le(s78, 0u);
        L3_B(0x2c1u, s78, s78, s3e);
        L3_B(0x85bu, s3e, s9a, s50);
        L3_B(0xb68u, s63a, s64c, s9a);
        L3_B(0x1e5u, s50, s9a, s78);
        L3_B(0x3a6u, s550, s562, s3e);
        L3_B(0x849u, s550, s574, s50);
        L3_B(0x8c7u, s562, s574, s63a);
        L3_B(0xd19u, s3e, s50, s64c);
        L3_B(0x394u, s63a, s64c, s9a);
        L3_B(0x961u, s78, s9a, sbe);
        L3_B(0x1004u, s5bc, s5bc, s5ce);
        L3_B(0x837u, s5aa, s5aa, s5bc);
        L3_B(0x267u, s598, s598, s5aa);
        L3_B(0x87fu, s586, sac, s598);
        L3_B(0x9f7u, s574, s574, s586);
        L3_B(0x0a3u, s562, s562, s574);
        L3_B(0xab1u, s550, s550, s562);
        L3_B(0xedeu, sac, sbe, s550);
    }

    L3_B(0xb05u, lanes, s550, lanes);
    L3_B(0x1d3u, lanes + 0x12u, s562, lanes + 0x12u);
    L3_B(0x29du, lanes + 0x24u, s574, lanes + 0x24u);
    L3_B(0xe5du, lanes + 0x36u, s586, lanes + 0x36u);
    L3_B(0xa8du, lanes + 0x48u, s598, lanes + 0x48u);
    L3_B(0xff2u, lanes + 0x5au, s5aa, lanes + 0x5au);
    L3_B(0x41eu, lanes + 0x6cu, s5bc, lanes + 0x6cu);
    L3_B(0x673u, lanes + 0x7eu, s5ce, lanes + 0x7eu);

#undef L3_C
#undef L3_B
#undef L3_BN
#undef L3_TRACE
#undef L3_L
    return L3_AUTHORIZATION_DIGEST_OK;
#endif
}

int l3_authorization_digest_init(l3_authorization_digest_context *ctx,
                                const l3_authorization_digest_tables *tables,
                                uint32_t mode_bytes) {
    int rc = validate_digest_tables(tables);
    if (rc) return rc;
    if (!ctx || (mode_bytes != 0x20u && mode_bytes != 0x1cu)) {
        return L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->tables = *tables;
    static const uint16_t bases32[8] = {
        0x8a3u,0x4b8u,0x985u,0x8b5u,0xd3du,0x2d3u,0x12du,0x255u
    };
    static const uint16_t bases28[8] = {
        0xc97u,0x2afu,0xb56u,0x109u,0xbe0u,0x4cau,0xf4cu,0x891u
    };
    static const uint16_t lane_offsets[8] = {
        0x114u,0x126u,0x138u,0x14au,
        0x15cu,0x16eu,0x180u,0x192u
    };
    const uint16_t *bases = mode_bytes == 0x1cu ? bases28 : bases32;
    size_t source = mode_bytes == 0x1cu ? 0u : 0x90u;
    for (unsigned i = 0; i < 8u; ++i) {
        const uint8_t *constant = tables->digest_initialization_program + source + 18u*i;
        rc = digest_table_transform(tables, bases[i], 0x120u,
                    constant, constant, ctx->state + lane_offsets[i]);
        if (rc) return rc;
    }
    return L3_AUTHORIZATION_DIGEST_OK;
}

int l3_authorization_digest_update(l3_authorization_digest_context *ctx,
                                  const uint8_t *bytes, size_t len) {
    if (!ctx || (len && !bytes) || ctx->finalized) {
        return L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT;
    }
    if (len > UINT32_MAX) return L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT;
    sha256_shadow_update(ctx->state, bytes, len, 0x20u);
    return L3_AUTHORIZATION_DIGEST_OK;
}

int l3_authorization_digest_update_frame(l3_authorization_digest_context *ctx,
                                        const uint8_t frame66[66],
                                        size_t byte_count) {
    if (!ctx || !frame66 || byte_count > 16u || ctx->finalized) {
        return L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT;
    }
    uint8_t encoded[66];
    int rc = digest_table_transform(&ctx->tables, 0xdb5u, 0x420u,
                    frame66, frame66, encoded);
    if (rc) return rc;
    rc = digest_flush_pending_bytes(ctx);
    if (rc) return rc;
    return digest_absorb_encoded_block(ctx, encoded, (uint32_t)byte_count);
}

int l3_authorization_digest_finalize_frame82(l3_authorization_digest_context *ctx,
                                   uint8_t out82[0x82]) {
    if (!ctx || !out82 || ctx->finalized) return L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT;
    int rc = digest_flush_pending_bytes(ctx);
    if (rc) return rc;
    uint32_t total_lo = rd32le(ctx->state + ST_TOTAL_LO);
    uint32_t total_hi = rd32le(ctx->state + ST_TOTAL_HI);
    uint32_t block_index = rd32le(ctx->state + ST_BLOCK_INDEX);
    /* process1(6) always finalizes the four 16-byte frame values after its
     * four-byte domain prefix.  This is the native 68-byte finalizer branch;
     * reject other shapes instead of silently producing a non-native root. */
    if (total_hi != 0u || total_lo != 68u || block_index != 0u) {
        return L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT;
    }
    uint8_t temp[66];
    uint8_t *blocks = ctx->state + ST_BLOCKS;
    const uint8_t *padding_left = ctx->tables.digest_update_program +
                                  0x1070u + ((4u ^ 15u) * 66u);
    const uint8_t *padding_right = ctx->tables.digest_update_program +
                                   0x1922u + ((4u ^ 15u) * 66u);
    rc = digest_table_transform(&ctx->tables, 0x3cau, 0x420u,
                blocks, padding_left, temp);
    if (rc) return rc;
    rc = digest_table_transform(&ctx->tables, 0x6c7u, 0x420u,
                temp, padding_right, blocks);
    if (rc) return rc;
    for (unsigned i = 1u; i < 4u; ++i) {
        const uint8_t *zero_block = ctx->tables.digest_update_program + 0x1d00u;
        rc = digest_table_transform(&ctx->tables, 0x54au, 0x420u,
                    zero_block, zero_block, blocks + i * 66u);
        if (rc) return rc;
    }
    static const uint8_t length544_block[66] = {
        0x00,0x00,0x02,0x05,0x05,0x02,0x07,0x04,0x03,0x04,0x03,
        0x07,0x00,0x06,0x00,0x06,0x05,0x04,0x03,0x03,0x06,0x01,
        0x00,0x07,0x00,0x04,0x03,0x04,0x07,0x00,0x06,0x05,0x04,
        0x03,0x03,0x07,0x02,0x01,0x02,0x01,0x02,0x00,0x06,0x00,
        0x03,0x06,0x00,0x07,0x01,0x00,0x07,0x02,0x00,0x03,0x04,
        0x06,0x02,0x00,0x03,0x06,0x01,0x02,0x02,0x00,0x06,0x01
    };
    memcpy(blocks + 3u * 66u, length544_block, sizeof(length544_block));
    wr32le(ctx->state + ST_BLOCK_INDEX, 4u);
    wr32le(ctx->state + ST_TOTAL_LO, 544u);
    wr32le(ctx->state + ST_TOTAL_HI, 0u);
    rc = digest_compress_generated_state(ctx);
    if (rc) return rc;

    const uint8_t *lanes = ctx->state + ST_LANES;
    uint8_t a34[34], b34[34], c66[66], d34[34];
    uint8_t e50[50], f114[114], g130[130];
    uint8_t right34[34] = {0};
    right34[15] = 3u;
    memcpy(right34 + 16u, lanes + 5u * 18u, 18u);
    rc = digest_table_transform(&ctx->tables, 0x71bu, 0x400120u,
                lanes + 6u * 18u, right34, a34);
    if (rc) return rc;

    memset(right34, 0, sizeof(right34));
    memcpy(right34 + 16u, lanes + 3u * 18u, 18u);
    rc = digest_table_transform(&ctx->tables, 0x4feu, 0x400120u,
                lanes + 4u * 18u, right34, b34);
    if (rc) return rc;

    uint8_t right66[66] = {0};
    right66[31] = 6u;
    memcpy(right66 + 32u, b34, sizeof(b34));
    rc = digest_table_transform(&ctx->tables, 0x685u, 0x800220u,
                a34, right66, c66);
    if (rc) return rc;

    memset(right34, 0, sizeof(right34));
    right34[15] = 6u;
    memcpy(right34 + 16u, lanes + 18u, 18u);
    rc = digest_table_transform(&ctx->tables, 0x651u, 0x400120u,
                lanes + 2u * 18u, right34, d34);
    if (rc) return rc;

    uint8_t right50[50] = {0};
    memcpy(right50 + 32u, lanes, 18u);
    rc = digest_table_transform(&ctx->tables, 0x058u, 0x400220u,
                d34, right50, e50);
    if (rc) return rc;

    uint8_t right114[114] = {0};
    right114[63] = 7u;
    memcpy(right114 + 64u, e50, sizeof(e50));
    rc = digest_table_transform(&ctx->tables, 0xa1bu, 0xc00420u,
                c66, right114, f114);
    if (rc) return rc;

    uint8_t right130[130] = {0};
    memcpy(right130 + 16u, f114, sizeof(f114));
    rc = digest_table_transform(&ctx->tables, 0xf5eu, 0x1c00120u,
                lanes + 7u * 18u, right130, g130);
    if (rc) return rc;
    rc = digest_table_transform(&ctx->tables, 0x13fu, 0x820u,
                g130, g130, out82);
    if (rc) return rc;
    ctx->finalized = 1u;
    return L3_AUTHORIZATION_DIGEST_OK;
}
