/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Sun Nov 15 11:43:27 CET 2026                                                 */

#include "backend.hpp"
#include "gen2/legacy.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <unistd.h>

namespace libre2_imp {
namespace {
bool secure_random(void *, uint8_t out[L2G2_RANDOM_BYTES]) {
    int fd;
    do {
        fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) return false;
    size_t offset = 0;
    while (offset < L2G2_RANDOM_BYTES) {
        const auto count = read(fd, out + offset, L2G2_RANDOM_BYTES - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) {
            close(fd);
            return false;
        }
        offset += size_t(count);
    }
    close(fd);
    return true;
}

std::mutex protocol_mutex;
libre2_gen2::LegacyProtocol &protocol() {
    static libre2_gen2::LegacyProtocol instance(secure_random);
    return instance;
}

class FrameBuffer {
public:
    // Enough for a complete FRAM read and its optional eight-byte header.
    static constexpr size_t inline_size = 352;

    void resize(size_t size) {
        if (size > inline_size) large_ = std::make_unique_for_overwrite<uint8_t[]>(size);
        size_ = size;
    }

    bool read(JNIEnv *env, jbyteArray array) {
        if (!array) return true;
        const jsize length = env->GetArrayLength(array);
        if (length < 0 || length > 8192 || env->ExceptionCheck()) return false;
        resize(size_t(length));
        if (length) env->GetByteArrayRegion(array, 0, length, reinterpret_cast<jbyte *>(data()));
        return !env->ExceptionCheck();
    }

    uint8_t *data() { return large_ ? large_.get() : small_.data(); }
    size_t size() const { return size_; }
    std::span<uint8_t> bytes() { return {data(), size_}; }

private:
    std::array<uint8_t, inline_size> small_;
    std::unique_ptr<uint8_t[]> large_;
    size_t size_ = 0;
};
} // namespace

jint protocol_p1(JNIEnv *env, jobject, jint operation, jint argument,
                 jbyteArray first, jbyteArray second, jbyteArray) {
    try {
        FrameBuffer parameters, response;
        if (!parameters.read(env, first) || !response.read(env, second)) {
            return L2G2_AUTHENTICATION_FAILED;
        }
        std::lock_guard lock(protocol_mutex);
        return protocol().control(static_cast<libre2_gen2::P1Operation>(operation),
                                  argument, parameters.bytes(), response.bytes());
    } catch (...) {
        return L2G2_AUTHENTICATION_FAILED;
    }
}

jbyteArray protocol_p2(JNIEnv *env, jobject, jint operation, jint argument,
                       jbyteArray first, jbyteArray second, jbyteArray) {
    try {
        FrameBuffer parameters, input, result;
        if (!parameters.read(env, first) || !input.read(env, second)) return nullptr;
        result.resize(std::max({FrameBuffer::inline_size, parameters.size(), input.size()}));
        size_t length;
        {
            std::lock_guard lock(protocol_mutex);
            length = protocol().write_result(static_cast<libre2_gen2::P2Operation>(operation),
                                             argument, parameters.bytes(), input.bytes(), result.bytes());
        }
        if (!length) return nullptr;
        const auto out = env->NewByteArray(jsize(length));
        if (out) env->SetByteArrayRegion(out, 0, jsize(length), reinterpret_cast<const jbyte *>(result.data()));
        return out;
    } catch (...) {
        return nullptr;
    }
}

} // namespace libre2_imp
