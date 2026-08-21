#include "whitebox_lookup_table.h"
#include "whitebox_lookup_compressed.inc"

static size_t l3_whitebox_lookup_block_index(size_t block) {
    const size_t off = (block >> 1u) * 3u;
    const uint8_t b0 = k_whitebox_block_index12[off + 0u];
    const uint8_t b1 = k_whitebox_block_index12[off + 1u];
    const uint8_t b2 = k_whitebox_block_index12[off + 2u];
    if ((block & 1u) == 0u) {
        return (size_t)b0 | (((size_t)b1 & 0x0fu) << 8u);
    }
    return (((size_t)b1 >> 4u) & 0x0fu) | ((size_t)b2 << 4u);
}

uint8_t l3_whitebox_lookup_byte(size_t index) {
    if (index >= L3_WHITEBOX_LOOKUP_TABLE_LEN) return 0u;
    const size_t block = index >> 3u;
    const size_t dict_index = l3_whitebox_lookup_block_index(block);
    const uint64_t word = k_whitebox_dictionary64[dict_index];
    return (uint8_t)(word >> ((index & 7u) * 8u));
}
