#include "openssl_symbols.h"
#include "openssl_loader.h"

#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef LOGGER
#define LOGGER(...) fprintf(stderr, __VA_ARGS__)
#endif
#ifndef LOGAR
#define LOGAR(...) fprintf(stderr, __VA_ARGS__)
#endif

/*
 * App builds provide this as a normal C++ symbol.  Do not use a weak C symbol
 * and do not go through RTLD symbol probing: the production app should satisfy
 * this reference exactly the same way the surrounding C++ code does.
 *
 * Host/unit-test builds provide a local fallback unless the build explicitly
 * says that the embedding application provides opencrypto().
 */
#if defined(__ANDROID__) || defined(L3_APP_PROVIDES_OPENCRYPTO)
extern void *opencrypto(void);
#else
void *opencrypto(void) {
#if defined(L3_CRYPTO_TEST_HOOKS)
    if (getenv("L3_CRYPTO_DISABLE_FALLBACK_DLOPEN")) return NULL;
#endif
    void *h = dlopen("libcrypto.so", RTLD_NOW | RTLD_LOCAL);
    if (!h) h = dlopen("libcrypto.so.3", RTLD_NOW | RTLD_LOCAL);
    if (!h) h = dlopen("libcrypto.so.1.1", RTLD_NOW | RTLD_LOCAL);
    return h;
}
#endif

static void *g_forced_crypto_handle = NULL;
static l3_opencrypto_fn g_forced_opencrypto = NULL;
static bool g_ok = false;



#if defined(L3_CRYPTO_TEST_HOOKS)
extern "C" void l3_openssl_symbols_reset_for_test(void) {
    g_forced_crypto_handle = NULL;
    g_forced_opencrypto = NULL;
    g_ok = false;
#define symtest(name) name##ptr = NULL
#include "openssl_required_symbols.inc"
#undef symtest
}
#endif

static void *resolve_crypto_handle(void) {
    if (g_forced_crypto_handle) return g_forced_crypto_handle;
    if (g_forced_opencrypto) {
        void *h = g_forced_opencrypto();
        if (h) return h;
    }
    return opencrypto();
}

extern "C" bool l3_openssl_symbols_init(void) {
    if (g_ok) return true;

    void *handle = resolve_crypto_handle();
    if (!handle) {
        LOGAR("opencrypto() failed\n");
        return false;
    }

#define hgetsym(handle_, name) (*((void **)&name##ptr) = dlsym((handle_), #name))
#define getsym(name) hgetsym(handle, name)
#define symtest(name) do { \
        if (!name##ptr && !getsym(name)) { \
            LOGGER(#name ": %s\n", dlerror()); \
        } \
    } while (0)
#include "openssl_required_symbols.inc"
#undef symtest
#undef getsym
#undef hgetsym

    bool ok = true;
#define symtest(name) do { \
        if (!name##ptr) { \
            LOGGER("required " #name ": unresolved\n"); \
            ok = false; \
        } \
    } while (0)
#include "openssl_required_symbols.inc"
#undef symtest

    g_ok = ok;
    return ok;
}
