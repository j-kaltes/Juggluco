#ifndef L3_DERIVED_POINT_TAIL_TRANSFORM_H
#define L3_DERIVED_POINT_TAIL_TRANSFORM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_DERIVED_POINT_TAIL_TRANSFORM_RAW_LEN 0x28u
#define L3_DERIVED_POINT_TAIL_TRANSFORM_OUTPUT_LEN 0x4cu

enum {
    L3_DERIVED_POINT_TAIL_TRANSFORM_OK = 0,
    L3_DERIVED_POINT_TAIL_TRANSFORM_ERR_ARGUMENT = -1
};

/*
 * Convert the 40-byte output of the generated authorization finalizer into the
 * 76-byte encoded state consumed by the generated frame normalizer.
 *
 * This is the recovered affine/tail transformation that sits between those
 * two stages.  The original binary address is intentionally not part of the API.
 */
int l3_authorization_transform_shared_point_tail(
    const uint8_t raw40[L3_DERIVED_POINT_TAIL_TRANSFORM_RAW_LEN],
    uint8_t encoded76[L3_DERIVED_POINT_TAIL_TRANSFORM_OUTPUT_LEN]);

#ifdef __cplusplus
}
#endif
#endif
