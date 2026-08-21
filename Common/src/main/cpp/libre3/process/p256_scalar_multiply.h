#ifndef L3_EC_SCALAR_MULTIPLY_H
#define L3_EC_SCALAR_MULTIPLY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_EC_FIELD_BYTES 35u

/* P-256 reconstruction of FUN_f3f3c248/FUN_f3f3c480.  Values use the
 * recovered 35-byte little-endian representation; P-256's standard a == -3
 * formula is fixed because both live callers use that curve. */
int l3_p256_scalar_multiply_point(
    const uint8_t x35[L3_EC_FIELD_BYTES],
    const uint8_t y35[L3_EC_FIELD_BYTES],
    const uint8_t scalar35[L3_EC_FIELD_BYTES],
    uint8_t out_x35[L3_EC_FIELD_BYTES],
    uint8_t out_y35[L3_EC_FIELD_BYTES]);

/* Derive the 64-byte public point used by Libre 3 selector 5 from a
 * conventional 32-byte big-endian private scalar.  The protocol does not use
 * OpenSSL's standard P-256 generator at this boundary: it uses the recovered
 * Libre base point.  This
 * implementation is self-contained and therefore does not need EC_KEY,
 * EC_POINT or BIGNUM entry points from libcrypto. */
int l3_p256_public64_from_private32_libre_basepoint(
    const uint8_t private32_be[32],
    uint8_t public64_be[64]);

/* FUN_f3fbc5c0 preamble through both FUN_f3fc2110 calls.  Only bytes 0..0x22
 * of each 0x42-byte input frame are consumed. */
int l3_authorization_core_initial_lanes10(
    const uint8_t left66[0x42],
    const uint8_t right66[0x42],
    const uint8_t scalar35[L3_EC_FIELD_BYTES],
    int32_t out_first_lanes10[10],
    int32_t out_second_lanes10[10]);

#ifdef __cplusplus
}
#endif

#endif
