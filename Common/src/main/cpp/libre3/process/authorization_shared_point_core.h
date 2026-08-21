#ifndef L3_AUTHORIZATION_SHARED_POINT_CORE_H
#define L3_AUTHORIZATION_SHARED_POINT_CORE_H

#include <stddef.h>
#include <stdint.h>

#include "authorization_shared_point_frames.h"

#ifdef __cplusplus
extern "C" {
#endif

#define L3_AUTHORIZATION_SHARED_POINT_FRAME_LEN 0x42u
#define L3_AUTHORIZATION_SHARED_POINT_FRAME_COUNT 2u
#define L3_AUTHORIZATION_SHARED_POINT_OUTPUT_LEN \
    (L3_AUTHORIZATION_SHARED_POINT_FRAME_LEN * \
     L3_AUTHORIZATION_SHARED_POINT_FRAME_COUNT)

enum {
    L3_AUTHORIZATION_SHARED_POINT_OK = 0,
    L3_AUTHORIZATION_SHARED_POINT_ERR_ARGUMENT = -1,
    L3_AUTHORIZATION_SHARED_POINT_ERR_WORKSPACE = -2,
    L3_AUTHORIZATION_SHARED_POINT_ERR_ENGINE = -3
};

/* Shared fixed encoding tables used by the generic Android compatibility
   transforms as well as by the allocator-free fixed core. */
extern const l3_authorization_frame_tables
    l3_authorization_shared_point_frame_tables;

/* Fixed-shape, OpenSSL-free authorization leaf.  Scratch may be static or
   caller-owned and can be reused for the next point immediately after return. */
size_t l3_authorization_shared_point_workspace_size(void);
size_t l3_authorization_shared_point_workspace_alignment(void);

int l3_authorization_shared_point_frames_into(
    const uint8_t native_scalar35[35],
    const uint8_t peer_public64[64],
    void *workspace,
    size_t workspace_size,
    uint8_t out_frames[L3_AUTHORIZATION_SHARED_POINT_OUTPUT_LEN]);

#ifdef __cplusplus
}
#endif

#endif
