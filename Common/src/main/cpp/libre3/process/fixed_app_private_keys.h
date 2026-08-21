#ifndef L3_FIXED_APP_PRIVATE_KEYS_H
#define L3_FIXED_APP_PRIVATE_KEYS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L3_APP_SECURITY_VERSION_COUNT = 2,
    L3_APP_PRIVATE_NATIVE_SCALAR_LEN = 35
};

/*
 * The two application private-key records embedded in KEYSCrypto.java are
 * fixed.  The recovered decoder maps them deterministically to these native
 * 35-byte scalar values.  Bytes 0..31, interpreted little-endian, are the
 * actual P-256 scalar consumed by the recovered scalar-multiply routine;
 * bytes 32..34 are zero.
 */
int l3_fixed_app_private_native_scalar35(unsigned security_version,
                                         uint8_t out35[L3_APP_PRIVATE_NATIVE_SCALAR_LEN]);

#ifdef __cplusplus
}
#endif

#endif
