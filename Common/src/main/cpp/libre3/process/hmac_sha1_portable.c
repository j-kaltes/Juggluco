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
/*      Sun Aug 30 10:21:11 CEST 2026                                                */

#include "hmac_sha1_portable.h"

#include <string.h>

typedef struct l3_sha1_context {
    uint32_t h[5];
    uint64_t byte_count;
    uint8_t block[64];
    size_t used;
} l3_sha1_context;

static uint32_t rol32(uint32_t value, unsigned shift) {
    return (value << shift) | (value >> (32u - shift));
}

static uint32_t load_be32(const uint8_t p[4]) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static void store_be32(uint8_t p[4], uint32_t value) {
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

static void sha1_transform(l3_sha1_context *ctx, const uint8_t block[64]) {
    uint32_t w[16];
    for (unsigned i = 0; i < 16u; ++i) {
        w[i] = load_be32(block + i * 4u);
    }

    uint32_t a = ctx->h[0];
    uint32_t b = ctx->h[1];
    uint32_t c = ctx->h[2];
    uint32_t d = ctx->h[3];
    uint32_t e = ctx->h[4];

    for (unsigned i = 0; i < 80u; ++i) {
        if (i >= 16u) {
            const unsigned slot = i & 15u;
            w[slot] = rol32(w[(i - 3u) & 15u] ^
                            w[(i - 8u) & 15u] ^
                            w[(i - 14u) & 15u] ^
                            w[slot], 1u);
        }

        uint32_t f;
        uint32_t k;
        if (i < 20u) {
            f = (b & c) | ((~b) & d);
            k = 0x5a827999u;
        } else if (i < 40u) {
            f = b ^ c ^ d;
            k = 0x6ed9eba1u;
        } else if (i < 60u) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdcu;
        } else {
            f = b ^ c ^ d;
            k = 0xca62c1d6u;
        }

        const uint32_t next = rol32(a, 5u) + f + e + k + w[i & 15u];
        e = d;
        d = c;
        c = rol32(b, 30u);
        b = a;
        a = next;
    }

    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
}

static void sha1_init(l3_sha1_context *ctx) {
    ctx->h[0] = 0x67452301u;
    ctx->h[1] = 0xefcdab89u;
    ctx->h[2] = 0x98badcfeu;
    ctx->h[3] = 0x10325476u;
    ctx->h[4] = 0xc3d2e1f0u;
    ctx->byte_count = 0u;
    ctx->used = 0u;
}

static void sha1_update(l3_sha1_context *ctx,
                        const uint8_t *data,
                        size_t len) {
    ctx->byte_count += (uint64_t)len;
    while (len != 0u) {
        size_t take = sizeof(ctx->block) - ctx->used;
        if (take > len) take = len;
        memcpy(ctx->block + ctx->used, data, take);
        ctx->used += take;
        data += take;
        len -= take;
        if (ctx->used == sizeof(ctx->block)) {
            sha1_transform(ctx, ctx->block);
            ctx->used = 0u;
        }
    }
}

static void sha1_final(l3_sha1_context *ctx, uint8_t out20[20]) {
    const uint64_t bit_count = ctx->byte_count * 8u;
    ctx->block[ctx->used++] = 0x80u;
    if (ctx->used > 56u) {
        memset(ctx->block + ctx->used, 0, 64u - ctx->used);
        sha1_transform(ctx, ctx->block);
        ctx->used = 0u;
    }
    memset(ctx->block + ctx->used, 0, 56u - ctx->used);
    for (unsigned i = 0; i < 8u; ++i) {
        ctx->block[63u - i] = (uint8_t)(bit_count >> (i * 8u));
    }
    sha1_transform(ctx, ctx->block);
    for (unsigned i = 0; i < 5u; ++i) {
        store_be32(out20 + i * 4u, ctx->h[i]);
    }
}

static void secure_zero(void *memory, size_t len) {
    volatile uint8_t *p = (volatile uint8_t *)memory;
    while (len-- != 0u) *p++ = 0u;
}

int l3_hmac_sha1(const uint8_t *key,
                 size_t key_len,
                 const uint8_t *data,
                 size_t data_len,
                 uint8_t out20[20]) {
    if ((!key && key_len) || (!data && data_len) || !out20) return -1;

    uint8_t key_block[64];
    uint8_t inner_digest[20];
    memset(key_block, 0, sizeof(key_block));
    if (key_len > sizeof(key_block)) {
        l3_sha1_context key_hash;
        sha1_init(&key_hash);
        sha1_update(&key_hash, key, key_len);
        sha1_final(&key_hash, key_block);
        secure_zero(&key_hash, sizeof(key_hash));
    } else if (key_len != 0u) {
        memcpy(key_block, key, key_len);
    }

    for (unsigned i = 0; i < sizeof(key_block); ++i) key_block[i] ^= 0x36u;
    l3_sha1_context ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, key_block, sizeof(key_block));
    sha1_update(&ctx, data, data_len);
    sha1_final(&ctx, inner_digest);

    for (unsigned i = 0; i < sizeof(key_block); ++i) key_block[i] ^= (0x36u ^ 0x5cu);
    sha1_init(&ctx);
    sha1_update(&ctx, key_block, sizeof(key_block));
    sha1_update(&ctx, inner_digest, sizeof(inner_digest));
    sha1_final(&ctx, out20);

    secure_zero(&ctx, sizeof(ctx));
    secure_zero(key_block, sizeof(key_block));
    secure_zero(inner_digest, sizeof(inner_digest));
    return 0;
}
