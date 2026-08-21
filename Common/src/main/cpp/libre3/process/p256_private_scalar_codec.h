#ifndef L3_PRIVATE_SCALAR_CODEC_H
#define L3_PRIVATE_SCALAR_CODEC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L3_PRIVATE_SCALAR_CODEC_OK = 0,
    L3_PRIVATE_SCALAR_CODEC_ERR_ARGUMENT = -1,
    L3_PRIVATE_SCALAR_CODEC_ERR_ZERO = -2
};

/* P-256 base point used by Libre 3 ephemeral-key generation, X || Y. */
extern const uint8_t l3_ephemeral_basepoint_public_xy64[64];

/* Convert a conventional 32-byte big-endian P-256 private scalar to the
 * recovered 35-byte little-endian representation used by the generated code. */
int l3_p256_encode_private_scalar_to_native35(
    const uint8_t scalar32_be[32],
    uint8_t native35_le[35]);

#ifdef __cplusplus
}
#endif
#endif
