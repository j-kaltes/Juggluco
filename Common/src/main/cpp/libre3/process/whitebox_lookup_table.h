#ifndef L3_WHITEBOX_LOOKUP_TABLE_H
#define L3_WHITEBOX_LOOKUP_TABLE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L3_WHITEBOX_LOOKUP_TABLE_LEN 0x20000u

uint8_t l3_whitebox_lookup_byte(size_t index);

#ifdef __cplusplus
}
#endif

#endif
