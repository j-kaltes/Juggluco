#pragma once
#include <cstddef>
#include <cstdint>

namespace libre3nfc {
// Response prefix: success byte, one or more A5 bytes, then 00.
inline size_t nfcPayloadOffset(const uint8_t *data, size_t size,
                               size_t payloadSize) {
  if (size < 3 || data[0] != 0 || data[1] != 0xA5)
    return size;
  size_t pos = 2;
  while (pos < size && data[pos] == 0xA5)
    ++pos;
  if (pos == size || data[pos++] != 0 || size - pos < payloadSize)
    return size;
  return pos;
}
} // namespace libre3nfc
