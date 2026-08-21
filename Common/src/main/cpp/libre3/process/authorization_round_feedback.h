#ifndef L3_AUTHORIZATION_ROUND_FEEDBACK_H
#define L3_AUTHORIZATION_ROUND_FEEDBACK_H

#include <stdint.h>

int l3_authorization_materialize_round_output_a(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param5_words13[13]);

int l3_authorization_materialize_round_output_b(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param6_words13[13]);

int l3_authorization_materialize_round_output_c(
    const uint32_t vector_feedback_words13[13],
    uint32_t out_param7_words13[13]);

#endif /* L3_AUTHORIZATION_ROUND_FEEDBACK_H */
