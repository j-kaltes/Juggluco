/**
 * @file
 * 64-bit optimised implementation of Ascon80pq AEAD cipher.
 *
 * @license Creative Commons Zero (CC0) 1.0
 * @authors see AUTHORS.md file
 */

#include "ascon.h"
#include "ascon_internal.h"

ASCON_API void
ascon_aead80pq_encrypt(uint8_t* ciphertext,
                       uint8_t* tag,
                       const uint8_t* key,
                       const uint8_t* nonce,
                       const uint8_t* assoc_data,
                       const uint8_t* plaintext,
                       size_t assoc_data_len,
                       size_t plaintext_len,
                       size_t tag_len)
{
#ifdef ASCON_INPUT_ASSERTS
    ASCON_ASSERT(plaintext_len == 0 || ciphertext != NULL);
    ASCON_ASSERT(tag_len != 0 || tag != NULL);
    ASCON_ASSERT(key != NULL);
    ASCON_ASSERT(nonce != NULL);
    ASCON_ASSERT(assoc_data_len == 0 || assoc_data != NULL);
    ASCON_ASSERT(plaintext_len == 0 || plaintext != NULL);
#endif
    ascon_aead_ctx_t ctx;
    ascon_aead80pq_init(&ctx, key, nonce);
    ascon_aead80pq_assoc_data_update(&ctx, assoc_data, assoc_data_len);
    const size_t new_ct_bytes = ascon_aead80pq_encrypt_update(&ctx, ciphertext,
                                                              plaintext,
                                                              plaintext_len);
    ascon_aead80pq_encrypt_final(&ctx, ciphertext + new_ct_bytes,
                                 tag, tag_len);
}

ASCON_API bool
ascon_aead80pq_decrypt(uint8_t* plaintext,
                       const uint8_t* key,
                       const uint8_t* nonce,
                       const uint8_t* assoc_data,
                       const uint8_t* ciphertext,
                       const uint8_t* expected_tag,
                       size_t assoc_data_len,
                       size_t ciphertext_len,
                       size_t expected_tag_len)
{
#ifdef ASCON_INPUT_ASSERTS
    ASCON_ASSERT(ciphertext_len == 0 || plaintext != NULL);
    ASCON_ASSERT(key != NULL);
    ASCON_ASSERT(nonce != NULL);
    ASCON_ASSERT(assoc_data_len == 0 || assoc_data != NULL);
    ASCON_ASSERT(ciphertext_len == 0 || ciphertext != NULL);
    ASCON_ASSERT(expected_tag_len != 0 || expected_tag != NULL);
#endif
    ascon_aead_ctx_t ctx;
    bool is_tag_valid;
    ascon_aead80pq_init(&ctx, key, nonce);
    ascon_aead80pq_assoc_data_update(&ctx, assoc_data, assoc_data_len);
    const size_t new_pt_bytes = ascon_aead80pq_decrypt_update(&ctx,
                                                              plaintext,
                                                              ciphertext,
                                                              ciphertext_len);
    ascon_aead80pq_decrypt_final(&ctx, plaintext + new_pt_bytes,
                                 &is_tag_valid, expected_tag, expected_tag_len);
    return is_tag_valid;
}

ASCON_API void
ascon_aead80pq_init(ascon_aead_ctx_t* const ctx,
                    const uint8_t* const key,
                    const uint8_t* const nonce)
{
#ifdef ASCON_INPUT_ASSERTS
    ASCON_ASSERT(ctx != NULL);
    ASCON_ASSERT(key != NULL);
    ASCON_ASSERT(nonce != NULL);
#endif
    // Store the key in the context as it's required in the final step.
    ctx->k0 = bigendian_decode_u64(key) >> 32U;
    ctx->k1 = bigendian_decode_u64(key + 4U);
    ctx->k2 = bigendian_decode_u64(key + 4U + sizeof(uint64_t));
    ctx->bufstate.sponge.x0 = AEAD80pq_IV | ctx->k0;
    ctx->bufstate.sponge.x1 = ctx->k1;
    ctx->bufstate.sponge.x2 = ctx->k2;
    ctx->bufstate.sponge.x3 = bigendian_decode_u64(nonce);
    ctx->bufstate.sponge.x4 = bigendian_decode_u64(nonce + sizeof(uint64_t));
    ascon_permutation_a12(&ctx->bufstate.sponge);
    ctx->bufstate.sponge.x2 ^= ctx->k0;
    ctx->bufstate.sponge.x3 ^= ctx->k1;
    ctx->bufstate.sponge.x4 ^= ctx->k2;
    ctx->bufstate.buffer_len = 0;
    ctx->bufstate.flow_state = ASCON_FLOW_AEAD128_80pq_INITIALISED;
}

ASCON_API void
ascon_aead80pq_assoc_data_update(ascon_aead_ctx_t* ctx,
                                 const uint8_t* assoc_data,
                                 size_t assoc_data_len)
{
    ascon_aead128_assoc_data_update(ctx, assoc_data, assoc_data_len);
}

ASCON_API size_t
ascon_aead80pq_encrypt_update(ascon_aead_ctx_t* ctx,
                              uint8_t* ciphertext,
                              const uint8_t* plaintext,
                              size_t plaintext_len)
{
    return ascon_aead128_encrypt_update(ctx, ciphertext, plaintext, plaintext_len);
}

ASCON_API size_t
ascon_aead80pq_encrypt_final(ascon_aead_ctx_t* const ctx,
                             uint8_t* const ciphertext,
                             uint8_t* tag,
                             size_t tag_len)
{
#ifdef ASCON_INPUT_ASSERTS
    ASCON_ASSERT(ctx != NULL);
    ASCON_ASSERT(ciphertext != NULL);
    ASCON_ASSERT(tag_len == 0 || tag != NULL);
    ASCON_ASSERT(ctx->bufstate.flow_state == ASCON_FLOW_AEAD128_80pq_INITIALISED
                 || ctx->bufstate.flow_state == ASCON_FLOW_AEAD128_80pq_ASSOC_DATA_UPDATED
                 || ctx->bufstate.flow_state == ASCON_FLOW_AEAD128_80pq_ENCRYPT_UPDATED);
#endif
    if (ctx->bufstate.flow_state != ASCON_FLOW_AEAD128_80pq_ENCRYPT_UPDATED)
    {
        // Finalise the associated data if not already done sos.
        ascon_aead128_80pq_finalise_assoc_data(ctx);
    }
    size_t freshly_generated_ciphertext_len = 0;
    // If there is any remaining less-than-a-block plaintext to be absorbed
    // cached in the buffer, pad it and absorb it.
    ctx->bufstate.sponge.x0 ^= bigendian_decode_varlen(ctx->bufstate.buffer,
                                                       ctx->bufstate.buffer_len);
    ctx->bufstate.sponge.x0 ^= PADDING(ctx->bufstate.buffer_len);
    // Squeeze out last ciphertext bytes, if any.
    bigendian_encode_varlen(ciphertext, ctx->bufstate.sponge.x0, ctx->bufstate.buffer_len);
    freshly_generated_ciphertext_len += ctx->bufstate.buffer_len;
    // End of encryption, start of tag generation.
    // Apply key twice more with a permutation to set the state for the tag.
    ctx->bufstate.sponge.x1 ^= ctx->k0 << 32U | ctx->k1 >> 32U;
    ctx->bufstate.sponge.x2 ^= ctx->k1 << 32U | ctx->k2 >> 32U;
    ctx->bufstate.sponge.x3 ^= ctx->k2 << 32U;
    ascon_permutation_a12(&ctx->bufstate.sponge);
    ctx->bufstate.sponge.x3 ^= ctx->k1;
    ctx->bufstate.sponge.x4 ^= ctx->k2;
    // Squeeze out tag into its buffer.
    ascon_aead_generate_tag(ctx, tag, tag_len);
    // Final security cleanup of the internal state, key and buffer.
    ascon_aead_cleanup(ctx);
    return freshly_generated_ciphertext_len;
}

ASCON_API size_t
ascon_aead80pq_decrypt_update(ascon_aead_ctx_t* ctx,
                              uint8_t* plaintext,
                              const uint8_t* ciphertext,
                              size_t ciphertext_len)
{
#ifdef ASCON_INPUT_ASSERTS
    ASCON_ASSERT(ctx != NULL);
    ASCON_ASSERT(ciphertext_len == 0 || ciphertext != NULL);
    ASCON_ASSERT(ciphertext_len == 0 || plaintext != NULL);
#endif
    return ascon_aead128_decrypt_update(ctx, plaintext, ciphertext, ciphertext_len);
}

ASCON_API size_t
ascon_aead80pq_decrypt_final(ascon_aead_ctx_t* const ctx,
                             uint8_t* plaintext,
                             bool* const is_tag_valid,
                             const uint8_t* const expected_tag,
                             const size_t expected_tag_len)
{
#ifdef ASCON_INPUT_ASSERTS
    ASCON_ASSERT(ctx != NULL);
    ASCON_ASSERT(plaintext != NULL);
    ASCON_ASSERT(expected_tag_len == 0 || expected_tag != NULL);
    ASCON_ASSERT(is_tag_valid != NULL);
#endif
    if (ctx->bufstate.flow_state != ASCON_FLOW_AEAD128_80pq_DECRYPT_UPDATED)
    {
        // Finalise the associated data if not already done sos.
        ascon_aead128_80pq_finalise_assoc_data(ctx);
    }
    size_t freshly_generated_plaintext_len = 0;
    // If there is any remaining less-than-a-block ciphertext to be absorbed
    // cached in the buffer, pad it and absorb it.
    // Absorb ciphertext in buffer
    const uint64_t c_0 = bigendian_decode_varlen(ctx->bufstate.buffer,
                                                 ctx->bufstate.buffer_len);
    // Squeeze out last plaintext bytes
    bigendian_encode_varlen(plaintext, ctx->bufstate.sponge.x0 ^ c_0,
                            ctx->bufstate.buffer_len);
    // Final state changes at decryption's end
    ctx->bufstate.sponge.x0 &= ~byte_mask(ctx->bufstate.buffer_len);
    ctx->bufstate.sponge.x0 |= c_0;
    ctx->bufstate.sponge.x0 ^= PADDING(ctx->bufstate.buffer_len);
    freshly_generated_plaintext_len += ctx->bufstate.buffer_len;
    // End of decryption, start of tag validation.
    // Apply key twice more with a permutation to set the state for the tag.
    ctx->bufstate.sponge.x1 ^= ctx->k0 << 32U | ctx->k1 >> 32U;
    ctx->bufstate.sponge.x2 ^= ctx->k1 << 32U | ctx->k2 >> 32U;
    ctx->bufstate.sponge.x3 ^= ctx->k2 << 32U;
    ascon_permutation_a12(&ctx->bufstate.sponge);
    ctx->bufstate.sponge.x3 ^= ctx->k1;
    ctx->bufstate.sponge.x4 ^= ctx->k2;
    // Validate tag with variable len
    *is_tag_valid = ascon_aead_is_tag_valid(ctx, expected_tag, expected_tag_len);
    // Final security cleanup of the internal state and key.
    ascon_aead_cleanup(ctx);
    return freshly_generated_plaintext_len;
}
