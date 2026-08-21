#ifndef L3_AUTHORIZATION_PACKED_TABLES_H
#define L3_AUTHORIZATION_PACKED_TABLES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L3_AUTH_TABLE_RAW_U8 = 0,
    L3_AUTH_TABLE_PACK6 = 1,
    L3_AUTH_TABLE_PACK3 = 2,
    L3_AUTH_TABLE_PACK6_PREFIX5375_RAW_TAIL = 3,
    L3_AUTH_TABLE_PACK6_PREFIX7503_RAW_TAIL = 4
};

static inline uint8_t l3_auth_table_unpack_bits(const uint8_t *data,
                                                 size_t pos,
                                                 unsigned bits) {
    const size_t bit = pos * (size_t)bits;
    const size_t byte = bit >> 3;
    const unsigned shift = (unsigned)(bit & 7u);
    uint32_t value = (uint32_t)data[byte];
    if (shift + bits > 8u) {
        value |= (uint32_t)data[byte + 1u] << 8;
    }
    if (shift + bits > 16u) {
        value |= (uint32_t)data[byte + 2u] << 16;
    }
    return (uint8_t)((value >> shift) & ((1u << bits) - 1u));
}

static inline size_t l3_auth_table_pack6_bytes(size_t logical_values) {
    return (logical_values * 6u + 7u) >> 3;
}

static inline uint8_t l3_auth_table_read_byte(const uint8_t *data,
                                               uint32_t format,
                                               size_t pos) {
    switch (format) {
    case L3_AUTH_TABLE_PACK6:
        return l3_auth_table_unpack_bits(data, pos, 6u);
    case L3_AUTH_TABLE_PACK3:
        return l3_auth_table_unpack_bits(data, pos, 3u);
    case L3_AUTH_TABLE_PACK6_PREFIX5375_RAW_TAIL: {
        const size_t prefix = 5375u;
        if (pos < prefix) return l3_auth_table_unpack_bits(data, pos, 6u);
        return data[l3_auth_table_pack6_bytes(prefix) + (pos - prefix)];
    }
    case L3_AUTH_TABLE_PACK6_PREFIX7503_RAW_TAIL: {
        const size_t prefix = 7503u;
        if (pos < prefix) return l3_auth_table_unpack_bits(data, pos, 6u);
        return data[l3_auth_table_pack6_bytes(prefix) + (pos - prefix)];
    }
    case L3_AUTH_TABLE_RAW_U8:
    default:
        return data[pos];
    }
}

#ifdef __cplusplus
}
#endif

#endif
