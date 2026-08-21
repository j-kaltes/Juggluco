#include "p256_private_scalar_codec.h"

#include <stddef.h>
#include <string.h>

const uint8_t l3_ephemeral_basepoint_public_xy64[64] = {
    0xa4,0x93,0x2d,0xb0,0xeb,0xeb,0x0b,0x5a,
    0xf5,0x6a,0x2f,0xf9,0x78,0x34,0x39,0x7b,
    0x82,0x1a,0x1a,0xef,0x82,0x01,0xcc,0x00,
    0x3a,0x7d,0x46,0x39,0xe6,0x43,0x98,0x0d,
    0xdf,0x01,0x6d,0xb7,0x3e,0xcb,0x01,0x9a,
    0xb7,0x06,0xbd,0xe9,0xf4,0xdf,0x7b,0x35,
    0x7f,0x36,0xc8,0x84,0xea,0x6c,0xa4,0x78,
    0x5c,0x83,0x2f,0x6f,0x5f,0x9d,0x1b,0x04
};

/* Little-word form of the P-256 group order. */
static const uint32_t k_order[8] = {
    0xfc632551u, 0xf3b9cac2u, 0xa7179e84u, 0xbce6faadu,
    0xffffffffu, 0xffffffffu, 0x00000000u, 0xffffffffu
};

/* tail = scalar * k_tail_multiplier (mod n). */
static const uint32_t k_tail_multiplier[8] = {
    0xd2bd727bu, 0x00bcd12eu, 0xac4dbc14u, 0xd0ea04c3u,
    0x7fe4d559u, 0x54f1b8f9u, 0xc923ab21u, 0x0c42b581u
};

static int words_compare(const uint32_t a[8], const uint32_t b[8]) {
    for (size_t i = 8; i-- > 0;) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    return 0;
}

static void words_sub(uint32_t a[8], const uint32_t b[8]) {
    uint64_t borrow = 0;
    for (size_t i = 0; i < 8; ++i) {
        const uint64_t sub = (uint64_t)b[i] + borrow;
        const uint64_t old = a[i];
        a[i] = (uint32_t)(old - sub);
        borrow = old < sub;
    }
}

static void words_reduce_once(uint32_t a[8]) {
    if (words_compare(a, k_order) >= 0) words_sub(a, k_order);
}

static int words_is_zero(const uint32_t a[8]) {
    uint32_t combined = 0;
    for (size_t i = 0; i < 8; ++i) combined |= a[i];
    return combined == 0;
}

static void words_add_mod(uint32_t out[8], const uint32_t a[8],
                          const uint32_t b[8]) {
    uint32_t sum[8];
    uint64_t carry = 0;
    for (size_t i = 0; i < 8; ++i) {
        const uint64_t v = (uint64_t)a[i] + b[i] + carry;
        sum[i] = (uint32_t)v;
        carry = v >> 32;
    }
    if (carry || words_compare(sum, k_order) >= 0) {
        uint64_t borrow = 0;
        for (size_t i = 0; i < 8; ++i) {
            const uint64_t sub = (uint64_t)k_order[i] + borrow;
            const uint64_t old = sum[i];
            sum[i] = (uint32_t)(old - sub);
            borrow = old < sub;
        }
        (void)borrow;
    }
    memcpy(out, sum, sizeof(sum));
}

static void words_mul_constant_mod(const uint32_t input[8],
                                   const uint32_t constant[8],
                                   uint32_t out[8]) {
    uint32_t result[8] = {0};
    uint32_t addend[8];
    memcpy(addend, input, sizeof(addend));
    for (unsigned bit = 0; bit < 256u; ++bit) {
        if ((constant[bit >> 5] >> (bit & 31u)) & 1u) {
            words_add_mod(result, result, addend);
        }
        words_add_mod(addend, addend, addend);
    }
    memcpy(out, result, sizeof(result));
}

static void scalar_be_to_words(const uint8_t bytes[32], uint32_t words[8]) {
    for (size_t i = 0; i < 8; ++i) {
        const size_t off = 28u - i * 4u;
        words[i] = ((uint32_t)bytes[off] << 24) |
                   ((uint32_t)bytes[off + 1u] << 16) |
                   ((uint32_t)bytes[off + 2u] << 8) |
                   bytes[off + 3u];
    }
}

static void words_to_tail_le(const uint32_t words[8], uint8_t bytes[35]) {
    for (size_t i = 0; i < 8; ++i) {
        const size_t off = i * 4u;
        bytes[off] = (uint8_t)words[i];
        bytes[off + 1u] = (uint8_t)(words[i] >> 8);
        bytes[off + 2u] = (uint8_t)(words[i] >> 16);
        bytes[off + 3u] = (uint8_t)(words[i] >> 24);
    }
    bytes[32] = bytes[33] = bytes[34] = 0;
}

int l3_p256_encode_private_scalar_to_native35(const uint8_t scalar32_be[32],
                                    uint8_t tail35_le[35]) {
    uint32_t scalar[8], tail[8];
    if (!scalar32_be || !tail35_le) return L3_PRIVATE_SCALAR_CODEC_ERR_ARGUMENT;
    scalar_be_to_words(scalar32_be, scalar);
    words_reduce_once(scalar);
    if (words_is_zero(scalar)) return L3_PRIVATE_SCALAR_CODEC_ERR_ZERO;
    words_mul_constant_mod(scalar, k_tail_multiplier, tail);
    words_to_tail_le(tail, tail35_le);
    return L3_PRIVATE_SCALAR_CODEC_OK;
}
