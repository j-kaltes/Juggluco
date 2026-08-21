#ifndef L3_EPHEMERAL_KEYPAIR_STATE_H
#define L3_EPHEMERAL_KEYPAIR_STATE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Portable reconstruction of native selector-5 FUN_f3f64bc0 for one entropy
 * draw. Native reads 142 bytes, masks every byte with 7, and deterministically
 * materializes the 76-byte DB-key state. The context is the 0x1eb0 provider
 * context used by selector 5; dat429 is DAT_f4295564 (0x20000 bytes). */
int l3_p256_generate_ephemeral_keypair_state(const uint8_t *dat429, size_t dat429_len,
                                    const uint8_t context[0x1eb0],
                                    const uint8_t entropy142[142],
                                    uint8_t state76[76]);

#ifdef __cplusplus
}
#endif
#endif
