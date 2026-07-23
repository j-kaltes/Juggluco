#ifndef L3_LIBRE3_OLD_POLICY_BYPASS_H
#define L3_LIBRE3_OLD_POLICY_BYPASS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    L3_OLD_POLICY_BYPASS_OK = 0,
    L3_OLD_POLICY_BYPASS_ERR_ARGUMENT = -1,
    L3_OLD_POLICY_BYPASS_ERR_LAYOUT = -2,
    L3_OLD_POLICY_BYPASS_ERR_STATE = -3,
    L3_OLD_POLICY_BYPASS_ERR_PAGE_SIZE = -4,
    L3_OLD_POLICY_BYPASS_ERR_PROTECTION = -5
};

/*
 * Make FUN_f3edc3e4's threshold match select its ordinary no-policy state.
 * The shared function itself still runs, including its invCount update, and
 * the policy-call basic block is never entered.
 * in_memory_process1 must retain its Thumb bit.
 *
 * Install only after runtime code generation is complete and while no thread
 * can execute process1/process2. The function verifies both code prefixes,
 * saves the original decision bytes and changes the selected state,
 * flushes the instruction cache, and restores the page to read/execute.
 */
int l3_old_policy_bypass_install(uintptr_t in_memory_process1);

/* Restore the exact saved policy-function bytes. Calls must again be stopped. */
int l3_old_policy_bypass_restore(void);

int l3_old_policy_bypass_is_installed(void);

/* Even code address of the patched decision instruction, or zero if absent. */
uintptr_t l3_old_policy_bypass_target(void);

#ifdef __cplusplus
}
#endif

#endif /* L3_LIBRE3_OLD_POLICY_BYPASS_H */
