#if defined(LIBRE3) && !defined(__aarch64__) &&  defined(__arm__) 
#include "libre3_old_policy_bypass.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define L3_OLD_POLICY_DECISION_OFFSET ((uintptr_t)0x1126u)
#define L3_OLD_POLICY_PATCH_SIZE ((size_t)2u)

/* .text:f3edb668 in the supplied older runtime-patched listing. */
static const uint8_t k_process1_prefix[16] = {
    0xf0, 0xb5, 0x03, 0xaf, 0x2d, 0xe9, 0x00, 0x0f,
    0x99, 0xb0, 0x0a, 0x93, 0x80, 0x46, 0x06, 0x92
};

/* .text:f3edc78e: mov.eq r0,#3 selects the policy-call state. */
static const uint8_t k_policy_call[L3_OLD_POLICY_PATCH_SIZE] = {
    0x03, 0x20
};

/* mov.eq r0,#15 selects the ordinary no-policy state on equality too. */
static const uint8_t k_return_stub[L3_OLD_POLICY_PATCH_SIZE] = {
    0x0f, 0x20
};

static uintptr_t g_policy_target;
static uintptr_t g_page_start;
static size_t g_page_span;
static uint8_t g_original[L3_OLD_POLICY_PATCH_SIZE];
static int g_installed;

static int page_window(uintptr_t address, size_t length,
                       uintptr_t *page_start, size_t *page_span) {
    long signed_page_size;
    uintptr_t page_size;
    uintptr_t first;
    uintptr_t after;
    uintptr_t last_page;

    if (!address || !length || !page_start || !page_span) {
        return L3_OLD_POLICY_BYPASS_ERR_ARGUMENT;
    }
    signed_page_size = sysconf(_SC_PAGESIZE);
    if (signed_page_size <= 0) {
        return L3_OLD_POLICY_BYPASS_ERR_PAGE_SIZE;
    }
    page_size = (uintptr_t)signed_page_size;
    if (length > (size_t)(UINTPTR_MAX - address)) {
        return L3_OLD_POLICY_BYPASS_ERR_ARGUMENT;
    }
    after = address + (uintptr_t)length;
    first = address - address % page_size;
    if (after > UINTPTR_MAX - (page_size - 1u)) {
        return L3_OLD_POLICY_BYPASS_ERR_ARGUMENT;
    }
    last_page = ((after + page_size - 1u) / page_size) * page_size;
    if (last_page < first || last_page - first > (uintptr_t)SIZE_MAX) {
        return L3_OLD_POLICY_BYPASS_ERR_ARGUMENT;
    }
    *page_start = first;
    *page_span = (size_t)(last_page - first);
    return L3_OLD_POLICY_BYPASS_OK;
}

static int make_writable(uintptr_t page_start, size_t page_span) {
    if (mprotect((void *)page_start, page_span,
                 PROT_READ | PROT_WRITE | PROT_EXEC) == 0) {
        return L3_OLD_POLICY_BYPASS_OK;
    }
    if (mprotect((void *)page_start, page_span,
                 PROT_READ | PROT_WRITE) == 0) {
        return L3_OLD_POLICY_BYPASS_OK;
    }
    return L3_OLD_POLICY_BYPASS_ERR_PROTECTION;
}

static int make_executable(uintptr_t page_start, size_t page_span) {
    return mprotect((void *)page_start, page_span,
                    PROT_READ | PROT_EXEC) == 0
        ? L3_OLD_POLICY_BYPASS_OK
        : L3_OLD_POLICY_BYPASS_ERR_PROTECTION;
}

static void publish_code(uintptr_t address, const uint8_t *bytes,
                         size_t length) {
    uint8_t *destination = (uint8_t *)address;
    memcpy(destination, bytes, length);
#if defined(__GNUC__) || defined(__clang__)
    __sync_synchronize();
    __builtin___clear_cache((char *)destination,
                            (char *)destination + length);
    __sync_synchronize();
#endif
}

int l3_old_policy_bypass_install(uintptr_t in_memory_process1) {
    uintptr_t process1_code;
    uintptr_t target;
    uintptr_t page_start;
    size_t page_span;
    int rc;

    if (!in_memory_process1) return L3_OLD_POLICY_BYPASS_ERR_ARGUMENT;
    process1_code = in_memory_process1 & ~(uintptr_t)1u;
    if (!process1_code ||
        L3_OLD_POLICY_DECISION_OFFSET > UINTPTR_MAX - process1_code) {
        return L3_OLD_POLICY_BYPASS_ERR_ARGUMENT;
    }
    target = process1_code + L3_OLD_POLICY_DECISION_OFFSET;

    if (g_installed) {
        if (g_policy_target == target &&
            memcmp((const void *)target, k_return_stub,
                   sizeof(k_return_stub)) == 0) {
            return L3_OLD_POLICY_BYPASS_OK;
        }
        return L3_OLD_POLICY_BYPASS_ERR_STATE;
    }

    if (memcmp((const void *)process1_code, k_process1_prefix,
               sizeof(k_process1_prefix)) != 0 ||
        memcmp((const void *)target, k_policy_call,
               sizeof(k_policy_call)) != 0) {
        return L3_OLD_POLICY_BYPASS_ERR_LAYOUT;
    }

    rc = page_window(target, sizeof(k_return_stub),
                     &page_start, &page_span);
    if (rc != L3_OLD_POLICY_BYPASS_OK) return rc;

    memcpy(g_original, (const void *)target, sizeof(g_original));
    rc = make_writable(page_start, page_span);
    if (rc != L3_OLD_POLICY_BYPASS_OK) {
        memset(g_original, 0, sizeof(g_original));
        return rc;
    }

    publish_code(target, k_return_stub, sizeof(k_return_stub));
    rc = make_executable(page_start, page_span);
    if (rc != L3_OLD_POLICY_BYPASS_OK) {
        publish_code(target, g_original, sizeof(g_original));
        (void)make_executable(page_start, page_span);
        memset(g_original, 0, sizeof(g_original));
        return rc;
    }

    if (memcmp((const void *)target, k_return_stub,
               sizeof(k_return_stub)) != 0) {
        return L3_OLD_POLICY_BYPASS_ERR_STATE;
    }

    g_policy_target = target;
    g_page_start = page_start;
    g_page_span = page_span;
    g_installed = 1;
    return L3_OLD_POLICY_BYPASS_OK;
}

int l3_old_policy_bypass_restore(void) {
    int rc;
    if (!g_installed) return L3_OLD_POLICY_BYPASS_OK;
    if (!g_policy_target || !g_page_start || !g_page_span ||
        memcmp((const void *)g_policy_target, k_return_stub,
               sizeof(k_return_stub)) != 0) {
        return L3_OLD_POLICY_BYPASS_ERR_STATE;
    }

    rc = make_writable(g_page_start, g_page_span);
    if (rc != L3_OLD_POLICY_BYPASS_OK) return rc;
    publish_code(g_policy_target, g_original, sizeof(g_original));
    rc = make_executable(g_page_start, g_page_span);
    if (rc != L3_OLD_POLICY_BYPASS_OK) return rc;

    memset(g_original, 0, sizeof(g_original));
    g_policy_target = 0;
    g_page_start = 0;
    g_page_span = 0;
    g_installed = 0;
    return L3_OLD_POLICY_BYPASS_OK;
}

int l3_old_policy_bypass_is_installed(void) {
    return g_installed && g_policy_target &&
           memcmp((const void *)g_policy_target, k_return_stub,
                  sizeof(k_return_stub)) == 0;
}

uintptr_t l3_old_policy_bypass_target(void) {
    return g_installed ? g_policy_target : (uintptr_t)0u;
}
#endif
