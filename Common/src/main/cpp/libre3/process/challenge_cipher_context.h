#ifndef L3_CHALLENGE_CIPHER_CONTEXT_H
#define L3_CHALLENGE_CIPHER_CONTEXT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L3_CHALLENGE_BLOCK_CONTEXT_ENCODED_LEN = 0x42u,
    L3_CHALLENGE_BLOCK_CONTEXT_WORDS = 0x2cu
};

/* Static lookup material used to build and execute the generated challenge
 * block-cipher context.  `program` is the recovered f426 table region; the
 * transition table is the common 0x20000-byte white-box state table. */
typedef struct l3_challenge_block_tables {
    const uint8_t *program;
    size_t program_len;
    const uint8_t *state_transition_table;
    size_t state_transition_table_len;
} l3_challenge_block_tables;

/* Compact generated block context.  The recovered object stored 44 words plus
 * sixteen eagerly generated 256-byte output tables.  The tables are derived
 * from these words and static F407 data, so the portable implementation
 * computes their sixteen actually used bytes while encrypting a block. */
typedef struct l3_challenge_block_context {
    uint32_t words[L3_CHALLENGE_BLOCK_CONTEXT_WORDS];
} l3_challenge_block_context;

int l3_challenge_cipher_context_encrypt(
    const l3_challenge_block_tables *tables,
    const l3_challenge_block_context *context,
    const uint8_t input[16],
    uint8_t output[16]);

int l3_challenge_block_context_init(
    const l3_challenge_block_tables *tables,
    const uint8_t encoded_frame[L3_CHALLENGE_BLOCK_CONTEXT_ENCODED_LEN],
    l3_challenge_block_context *context);

#ifdef L3_CHALLENGE_CONTEXT_TEST_HOOKS
uint8_t l3_challenge_context_test_output_byte(
    const l3_challenge_block_tables *tables,
    const l3_challenge_block_context *context,
    unsigned lane,
    uint8_t value);
#endif

#ifdef __cplusplus
}
#endif

#endif
