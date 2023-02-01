/**
 * @file
 *
 * @copyright Copyright © 2020, Matjaž Guštin <dev@matjaz.it>
 * <https://matjaz.it>. All rights reserved.
 * @license BSD 3-clause license.
 */

#include "ascon.h"
#include "ascon_internal.h"

void
ascon_aead_init(ascon_aead_ctx_t* const ctx,
                const uint8_t* const key,
                const uint8_t* const nonce,
                const uint64_t iv)
{
    // Store the key in the context as it's required in the final step.
    ctx->k0 = bigendian_decode_u64(key);
    ctx->k1 = bigendian_decode_u64(key + sizeof(uint64_t));
    ctx->bufstate.sponge.x0 = iv;
    ctx->bufstate.sponge.x1 = ctx->k0;
    ctx->bufstate.sponge.x2 = ctx->k1;
    ctx->bufstate.sponge.x3 = bigendian_decode_u64(nonce);
    ctx->bufstate.sponge.x4 = bigendian_decode_u64(nonce + sizeof(uint64_t));
    ascon_permutation_a12(&ctx->bufstate.sponge);
    ctx->bufstate.sponge.x3 ^= ctx->k0;
    ctx->bufstate.sponge.x4 ^= ctx->k1;
    ctx->bufstate.buffer_len = 0;
}

void
ascon_aead128_80pq_finalise_assoc_data(ascon_aead_ctx_t* const ctx)
{
    // If there was at least some associated data obtained so far,
    // pad it and absorb any content of the buffer.
    // Note: this step is performed even if the buffer is now empty because
    // a state permutation is required if there was at least some associated
    // data absorbed beforehand.
    if (ctx->bufstate.flow_state == ASCON_FLOW_AEAD128_80pq_ASSOC_DATA_UPDATED)
    {
        ctx->bufstate.sponge.x0 ^= bigendian_decode_varlen(ctx->bufstate.buffer,
                                                           ctx->bufstate.buffer_len);
        ctx->bufstate.sponge.x0 ^= PADDING(ctx->bufstate.buffer_len);
        ascon_permutation_b6(&ctx->bufstate.sponge);
    }
    // Application of a constant at end of associated data for domain
    // separation. Done always, regardless if there was some associated
    // data or not.
    ctx->bufstate.sponge.x4 ^= 1U;
    ctx->bufstate.buffer_len = 0;
}

void
ascon_aead_generate_tag(ascon_aead_ctx_t* const ctx,
                        uint8_t* tag,
                        size_t tag_len)
{
    while (tag_len > ASCON_AEAD_TAG_MIN_SECURE_LEN)
    {
        // All bytes before the last 16
        // Note: converting the sponge uint64_t to bytes to then check them as
        // uint64_t is required, as the conversion to bytes ensures the
        // proper byte order regardless of the platform native endianness.
        bigendian_encode_u64(tag, ctx->bufstate.sponge.x3);
        bigendian_encode_u64(tag + sizeof(uint64_t), ctx->bufstate.sponge.x4);
        ascon_permutation_a12(&ctx->bufstate.sponge);
        tag_len -= ASCON_AEAD_TAG_MIN_SECURE_LEN;
        tag += ASCON_AEAD_TAG_MIN_SECURE_LEN;
    }
    // The last 16 or less bytes (also 0)
    uint_fast8_t remaining = (uint_fast8_t) MIN(sizeof(uint64_t), tag_len);
    bigendian_encode_varlen(tag, ctx->bufstate.sponge.x3, remaining);
    tag += remaining;
    // The last 8 or less bytes (also 0)
    tag_len -= remaining;
    remaining = (uint_fast8_t) MIN(sizeof(uint64_t), tag_len);
    bigendian_encode_varlen(tag, ctx->bufstate.sponge.x4, remaining);
}

/** @internal Simplistic clone of `memcmp() != 0`, true when NOT equal. */
inline static bool
small_neq(const uint8_t* restrict a, const uint8_t* restrict b, size_t amount)
{
    while (amount--)
    {
        if (*(a++) != *(b++)) { return true; }
    }
    return false;
}

bool
ascon_aead_is_tag_valid(ascon_aead_ctx_t* const ctx,
                        const uint8_t* expected_tag,
                        size_t expected_tag_len)
{
    uint8_t computed_tag_chunk[ASCON_RATE];
    while (expected_tag_len > ASCON_AEAD_TAG_MIN_SECURE_LEN)
    {
        // All bytes before the last 16
        // Note: converting the sponge uint64_t to bytes to then check them as
        // uint64_t is required, as the conversion to bytes ensures the
        // proper tag's byte order regardless of the platform's endianness.
        bigendian_encode_u64(computed_tag_chunk, ctx->bufstate.sponge.x3);
        if (small_neq(computed_tag_chunk, expected_tag, sizeof(computed_tag_chunk)))
        {
            return ASCON_TAG_INVALID;
        }
        expected_tag += sizeof(computed_tag_chunk);
        expected_tag_len -= sizeof(computed_tag_chunk);
        bigendian_encode_u64(computed_tag_chunk, ctx->bufstate.sponge.x4);
        if (small_neq(computed_tag_chunk, expected_tag, sizeof(computed_tag_chunk)))
        {
            return ASCON_TAG_INVALID;
        }
        expected_tag += sizeof(computed_tag_chunk);
        expected_tag_len -= sizeof(computed_tag_chunk);
        ascon_permutation_a12(&ctx->bufstate.sponge);
    }
    // The last 16 or less bytes (also 0)
    size_t remaining = MIN(sizeof(computed_tag_chunk), expected_tag_len);
    bigendian_encode_varlen(computed_tag_chunk, ctx->bufstate.sponge.x3, (uint_fast8_t) remaining);
    if (small_neq(computed_tag_chunk, expected_tag, remaining)) { return ASCON_TAG_INVALID; }
    expected_tag += remaining;
    // The last 8 or less bytes (also 0)
    expected_tag_len -= remaining;
    remaining = MIN(sizeof(computed_tag_chunk), expected_tag_len);
    bigendian_encode_varlen(computed_tag_chunk, ctx->bufstate.sponge.x4, (uint_fast8_t) remaining);
    if (small_neq(computed_tag_chunk, expected_tag, remaining)) { return ASCON_TAG_INVALID; }
    return ASCON_TAG_OK;
}

ASCON_API void
ascon_aead_cleanup(ascon_aead_ctx_t* const ctx)
{
#ifdef ASCON_INPUT_ASSERTS
    ASCON_ASSERT(ctx != NULL);
#endif
    // Manual cleanup using volatile pointers to have more assurance the
    // cleanup will not be removed by the optimiser.
    ((volatile ascon_aead_ctx_t*) ctx)->bufstate.sponge.x0 = 0U;
    ((volatile ascon_aead_ctx_t*) ctx)->bufstate.sponge.x1 = 0U;
    ((volatile ascon_aead_ctx_t*) ctx)->bufstate.sponge.x2 = 0U;
    ((volatile ascon_aead_ctx_t*) ctx)->bufstate.sponge.x3 = 0U;
    ((volatile ascon_aead_ctx_t*) ctx)->bufstate.sponge.x4 = 0U;
    for (uint_fast8_t i = 0; i < ASCON_DOUBLE_RATE; i++)
    {
        ((volatile ascon_aead_ctx_t*) ctx)->bufstate.buffer[i] = 0U;
    }
    ((volatile ascon_aead_ctx_t*) ctx)->bufstate.buffer_len = 0U;
    ((volatile ascon_aead_ctx_t*) ctx)->bufstate.flow_state = ASCON_FLOW_CLEANED;
    // Clearing also the padding to set the whole context to be all-zeros.
    // Makes it easier to check for initialisation and provides a known
    // state after cleanup, initialising all memory.
    for (uint_fast8_t i = 0U; i < 6U; i++)
    {
        ((volatile ascon_aead_ctx_t*) ctx)->bufstate.pad[i] = 0U;
    }
    ((volatile ascon_aead_ctx_t*) ctx)->k0 = 0U;
    ((volatile ascon_aead_ctx_t*) ctx)->k1 = 0U;
    ((volatile ascon_aead_ctx_t*) ctx)->k2 = 0U;
}
