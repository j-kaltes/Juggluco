#ifndef L3_AUTHORIZATION_DIGEST_H
#define L3_AUTHORIZATION_DIGEST_H

#include <stddef.h>
#include <stdint.h>
#include "authorization_packed_tables.h"

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTHORIZATION_DIGEST_OK 0
#define L3_AUTHORIZATION_DIGEST_ERR_ARGUMENT -1
#define L3_AUTHORIZATION_DIGEST_ERR_TABLE_SIZE -2

#define L3_AUTHORIZATION_DIGEST_FRAME_LEN 0x42u

typedef struct {
    const uint8_t *digest_update_program;
    size_t digest_update_program_len;
    uint32_t digest_update_program_format;
    const uint8_t *digest_initialization_program;
    size_t digest_initialization_program_len;
    uint32_t digest_initialization_program_format;
} l3_authorization_digest_tables;

typedef struct {
    l3_authorization_digest_tables tables;
    uint8_t state[0x210];
    uint32_t finalized;
} l3_authorization_digest_context;

int l3_authorization_digest_init(l3_authorization_digest_context *ctx,
                                const l3_authorization_digest_tables *tables,
                                uint32_t mode_bytes);
int l3_authorization_digest_update(l3_authorization_digest_context *ctx,
                                  const uint8_t *bytes,
                                  size_t len);
int l3_authorization_digest_update_frame(l3_authorization_digest_context *ctx,
                                        const uint8_t frame66[L3_AUTHORIZATION_DIGEST_FRAME_LEN],
                                        size_t byte_count);
int l3_authorization_digest_finalize_frame82(l3_authorization_digest_context *ctx,
                                   uint8_t out82[0x82]);

#ifdef __cplusplus
}
#endif

#endif
