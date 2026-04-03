#pragma once
#include <stdint.h>
extern uint8_t crc8_maxim(const uint8_t *data, size_t len);
extern uint16_t crc16_ccitt_false(const uint8_t *data, size_t len);
extern uint32_t crc32_normal(const uint8_t *buf, int len, uint32_t crc);
extern void addcrc16(uint8_t *data, size_t len);
