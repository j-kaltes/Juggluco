#pragma once
// dp_activation.cpp -- C++26 reference impl of DPGetActivationCommandData
//
//   void DPGetActivationCommandData(std::span<std::byte, 10> bArr,
//                                   std::int64_t p4, std::int64_t p5);
//
// On return:
//   bArr[0..3] = static_cast<uint32_t>(p4 - 1)        little-endian
//   bArr[4..7] = static_cast<uint32_t>(p5)            little-endian
//   bArr[8..9] = CRC16 of the first 8 bytes           little-endian
//
// CRC-16 parameters (deduced from samples):
//   width  = 16
//   poly   = 0x1021
//   init   = 0xFFFF
//   refin  = true        (each input byte is bit-reversed before XOR)
//   refout = false       (final register is NOT reflected)
//   xorout = 0x0000
//   ----------------------------------------------------------------
//   == CRC-16/MCRF4XX with the two output bytes byte-swapped, OR
//      CRC-16/X-25 without the final 0xFFFF XOR and without RefOut.
//   Not a named standard variant; bytes are then stored little-endian.
//
// Build:  g++-14 -std=c++26 -O2 -Wall -Wextra -Wpedantic dp_activation.cpp -o dp_activation
// (any compiler with C++26 <print> + <bit> + <span>; replace <print> with
//  iostream if your toolchain doesn't have it yet)

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace dp {

// Bit-reverse a byte. C++23 added std::byteswap (whole-word) but no per-byte
// bit-reverse, so we keep this small helper.
constexpr std::uint8_t bitrev8(std::uint8_t b) noexcept {
    b = static_cast<std::uint8_t>(((b & 0xF0u) >> 4) | ((b & 0x0Fu) << 4));
    b = static_cast<std::uint8_t>(((b & 0xCCu) >> 2) | ((b & 0x33u) << 2));
    b = static_cast<std::uint8_t>(((b & 0xAAu) >> 1) | ((b & 0x55u) << 1));
    return b;
}

// CRC parameters: poly=0x1021, init=0xFFFF, refin=true, refout=false, xorout=0.
//
// Note: a byte-at-a-time table only folds correctly when refin==refout. Because
// the JNI uses the asymmetric combination refin=true, refout=false we keep the
// bit-by-bit form — for the 8-byte payload the cost is negligible.
[[nodiscard]] constexpr std::uint16_t
crc16_dp(std::span<const std::byte> data) noexcept {
    std::uint16_t crc = 0xFFFFu;
    for (std::byte b : data) {
        crc = static_cast<std::uint16_t>(crc ^ (std::uint16_t{bitrev8(std::to_integer<std::uint8_t>(b))} << 8));
        for (int k = 0; k < 8; ++k) {
            crc = (crc & 0x8000u) ? static_cast<std::uint16_t>((crc << 1) ^ 0x1021u)
                                  : static_cast<std::uint16_t>( crc << 1);
        }
    }
    return crc;
}

// The reproduced JNI body.  bArr must reference an exactly-10-byte buffer.
inline void DPGetActivationCommandData(std::span<std::byte, 10> bArr,
                                       std::int64_t p4,
                                       std::int64_t p5) noexcept {
    const std::uint32_t a = static_cast<std::uint32_t>(p4 - 1);
    const std::uint32_t b = static_cast<std::uint32_t>(p5);
    // Little-endian store; std::endian::native is virtually always little on
    // the platforms this code runs on, but std::byteswap keeps us honest.
    auto store_le_u32 = [](std::span<std::byte, 4> dst, std::uint32_t v) {
        if constexpr (std::endian::native == std::endian::big) v = std::byteswap(v);
        std::memcpy(dst.data(), &v, 4);
    };
    store_le_u32(bArr.subspan<0, 4>(), a);
    store_le_u32(bArr.subspan<4, 4>(), b);
    const std::uint16_t crc = crc16_dp(bArr.subspan<0, 8>());
    bArr[8] = std::byte{static_cast<std::uint8_t>(crc & 0xFFu)};
    bArr[9] = std::byte{static_cast<std::uint8_t>(crc >> 8)};
}

} // namespace dp

// Compile-time verification that the CRC implementation is correct.
namespace dp {
static_assert(crc16_dp(std::span<const std::byte, 8>(
    std::array<std::byte, 8>{ std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0},
                              std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0} })) == 0x313Eu);
static_assert(crc16_dp(std::span<const std::byte, 8>(
    std::array<std::byte, 8>{ std::byte{1}, std::byte{0}, std::byte{0}, std::byte{0},
                              std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0} })) == 0xCCBFu);
static_assert(crc16_dp(std::span<const std::byte, 8>(
    std::array<std::byte, 8>{ std::byte{8}, std::byte{0}, std::byte{0}, std::byte{0},
                              std::byte{12}, std::byte{0}, std::byte{0}, std::byte{0} })) == 0x2063u);
}
