#ifndef L3_OPENSSL_LOADER_H
#define L3_OPENSSL_LOADER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*l3_opencrypto_fn)(void);

bool l3_openssl_symbols_init(void);
#if defined(L3_CRYPTO_TEST_HOOKS)
void l3_openssl_symbols_reset_for_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* L3_OPENSSL_LOADER_H */
