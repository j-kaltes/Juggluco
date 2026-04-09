#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Returns required UTF-8 byte count, excluding trailing '\0'.
// Writes only if output_capacity is large enough for the full string plus '\0'.

int32_t rtl_to_visual_utf8(
    const char* logical_utf8,
    char* output,
    size_t output_capacity);

int32_t rtl_to_logical_utf8(
    const char* visual_utf8,
    char* output,
    size_t output_capacity);

int32_t rtl_visual_form_utf8(
    const char* user_input_utf8,
    char* output,
    size_t output_capacity);

int32_t rtl_logical_form_utf8(
    const char* stored_label_utf8,
    char* output,
    size_t output_capacity);

int rtl_is_rtl_utf8(const char* utf8);

#ifdef __cplusplus
}
#endif
