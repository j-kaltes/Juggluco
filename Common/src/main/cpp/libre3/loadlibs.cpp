/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Tue Aug 11 16:28:40 CEST 2026                                                */
#include "config.h"
#include "share/logs.hpp"
#include "fromjava.h"
#include "process/sensor_security_context.h"

#include <jni.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>

namespace {

class ByteArrayView final {
public:
    ByteArrayView(JNIEnv *env, jbyteArray array, std::size_t expected_size, bool nullable = false) noexcept
        : env_(env), array_(array) {
        if (!array_) {
            valid_ = nullable;
            return;
        }

        const jsize size = env_->GetArrayLength(array_);
        if (size < 0 || static_cast<std::size_t>(size) != expected_size) {
            return;
        }
        size_ = static_cast<std::size_t>(size);
        data_ = env_->GetByteArrayElements(array_, nullptr);
        valid_ = data_ != nullptr && !env_->ExceptionCheck();
    }

    ~ByteArrayView() {
        if (data_) {
            env_->ReleaseByteArrayElements(array_, data_, JNI_ABORT);
        }
    }

    ByteArrayView(const ByteArrayView &) = delete;
    ByteArrayView &operator=(const ByteArrayView &) = delete;

    [[nodiscard]] bool valid() const noexcept { return valid_; }
    [[nodiscard]] const std::uint8_t *data() const noexcept {
        return reinterpret_cast<const std::uint8_t *>(data_);
    }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

private:
    JNIEnv *env_{};
    jbyteArray array_{};
    jbyte *data_{};
    std::size_t size_{};
    bool valid_{};
};

using NativeBytes = std::unique_ptr<std::uint8_t, decltype(&std::free)>;

[[nodiscard]] l3_sensor_security_context *security_context_from_handle(jlong handle) noexcept {
    return reinterpret_cast<l3_sensor_security_context *>(static_cast<std::uintptr_t>(handle));
}

[[nodiscard]] jlong security_handle_from_context(l3_sensor_security_context *context) noexcept {
    return static_cast<jlong>(reinterpret_cast<std::uintptr_t>(context));
}

template<std::size_t ExpectedSize, class Producer>
[[nodiscard]] jbyteArray make_byte_array(JNIEnv *env, Producer &&producer) {
    std::uint8_t *raw = nullptr;
    std::size_t size = 0;
    const int rc = producer(&raw, &size);
    NativeBytes bytes{raw, &std::free};

    if (rc != 1 || !bytes || size != ExpectedSize) {
        return nullptr;
    }

    jbyteArray result = env->NewByteArray(static_cast<jsize>(ExpectedSize));
    if (!result) {
        return nullptr;
    }

    env->SetByteArrayRegion(result, 0, static_cast<jsize>(ExpectedSize),
                            reinterpret_cast<const jbyte *>(bytes.get()));
    return env->ExceptionCheck() ? nullptr : result;
}

} // namespace

/* Allocate the per-Libre3GattCallback security state on first use.  On a
 * reconnect, reset and reuse the existing state instead of allocating a new
 * object.  Returning 0 means creation/reset failed. */
extern "C" JNIEXPORT jlong JNICALL
fromjava(libre3BeginSecurityHandshake)(JNIEnv *, jclass, jlong context) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) {
        security = l3_sensor_security_context_create(nullptr);
        if (!security) {
#ifndef NOLOG
            LOGGER("libre3BeginSecurityHandshake(0)=0 create failed\n");
#endif
            return 0;
        }
    }

    const int rc = l3_sensor_security_begin_handshake(security);
    if (rc != 1) {
#ifndef NOLOG
        LOGGER("libre3BeginSecurityHandshake(%p) reset failed=%d\n",
               static_cast<void *>(security), rc);
#endif
        l3_sensor_security_context_destroy(security);
        return 0;
    }

#ifndef NOLOG
    LOGGER("libre3BeginSecurityHandshake(%p)=%p\n",
           static_cast<void *>(security_context_from_handle(context)), static_cast<void *>(security));
#endif
    return security_handle_from_context(security);
}

extern "C" JNIEXPORT void JNICALL
fromjava(libre3FreeSecurityContext)(JNIEnv *, jclass, jlong context) {
    l3_sensor_security_context *security = security_context_from_handle(context);
#ifndef NOLOG
    LOGGER("libre3FreeSecurityContext(%p)\n", static_cast<void *>(security));
#endif
    l3_sensor_security_context_destroy(security);
}

extern "C" JNIEXPORT jint JNICALL
fromjava(libre3LoadAppKeyAndSavedAuthorization)(JNIEnv *env, jclass, jlong context,
                                  jbyteArray appPrivateKey,
                                  jbyteArray savedAuthorization) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) return static_cast<jint>(L3_SECURITY_ERR_ARGUMENT);

    ByteArrayView key{env, appPrivateKey, L3_LEN_APP_PRIVATE_KEY};
    ByteArrayView saved{env, savedAuthorization, L3_LEN_SAVED_AUTHORIZATION, true};
    if (!key.valid() || !saved.valid()) {
        return static_cast<jint>(L3_SECURITY_ERR_ARGUMENT);
    }

    const int rc = l3_sensor_security_load_app_key_and_saved_authorization(
        security, key.data(), key.size(), saved.data(), saved.size());
#ifndef NOLOG
    LOGGER("libre3LoadAppKeyAndSavedAuthorization(%p,key#%zu,saved#%zu)=%d\n",
           static_cast<void *>(security), key.size(), saved.size(), rc);
#endif
    return static_cast<jint>(rc);
}

extern "C" JNIEXPORT jint JNICALL
fromjava(libre3AcceptPatchCertificate)(JNIEnv *env, jclass, jlong context,
                                       jbyteArray patchCertificate) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) return static_cast<jint>(L3_SECURITY_ERR_ARGUMENT);

    ByteArrayView certificate{env, patchCertificate, L3_LEN_PATCH_CERTIFICATE};
    if (!certificate.valid()) {
        return static_cast<jint>(L3_SECURITY_ERR_ARGUMENT);
    }

    const int rc = l3_sensor_security_set_patch_certificate(
        security, certificate.data(), certificate.size());
#ifndef NOLOG
    LOGGER("libre3AcceptPatchCertificate(%p,cert#%zu)=%d\n",
           static_cast<void *>(security), certificate.size(), rc);
#endif
    return static_cast<jint>(rc);
}

extern "C" JNIEXPORT jbyteArray JNICALL
fromjava(libre3CreateEphemeralPublicKey)(JNIEnv *env, jclass, jlong context) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) return nullptr;

    return make_byte_array<L3_LEN_EPHEMERAL_PUBLIC_KEY>(env,
        [security](std::uint8_t **out, std::size_t *size) {
            return l3_sensor_security_create_ephemeral_public_key(security, out, size);
        });
}

extern "C" JNIEXPORT jint JNICALL
fromjava(libre3DeriveAuthorizationRoot)(JNIEnv *env, jclass, jlong context,
                                        jbyteArray patchEphemeralPublicKey) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) return static_cast<jint>(L3_SECURITY_ERR_ARGUMENT);

    ByteArrayView publicKey{env, patchEphemeralPublicKey, L3_LEN_PATCH_PUBLIC_KEY};
    if (!publicKey.valid()) {
        return static_cast<jint>(L3_SECURITY_ERR_ARGUMENT);
    }

    const int rc = l3_sensor_security_derive_authorization_root(
        security, publicKey.data(), publicKey.size());
#ifndef NOLOG
    LOGGER("libre3DeriveAuthorizationRoot(%p,publicKey#%zu)=%d\n",
           static_cast<void *>(security), publicKey.size(), rc);
#endif
    return static_cast<jint>(rc);
}

extern "C" JNIEXPORT jbyteArray JNICALL
fromjava(libre3EncryptChallengeReply)(JNIEnv *env, jclass, jlong context,
                                      jbyteArray nonce,
                                      jbyteArray plaintext) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) return nullptr;

    ByteArrayView nonceView{env, nonce, L3_LEN_CHALLENGE_NONCE};
    ByteArrayView plainView{env, plaintext, L3_LEN_CHALLENGE_REPLY_PLAIN};
    if (!nonceView.valid() || !plainView.valid()) {
        return nullptr;
    }

    return make_byte_array<L3_LEN_CHALLENGE_REPLY_CRYPT>(env,
        [&](std::uint8_t **out, std::size_t *size) {
            return l3_sensor_security_encrypt_challenge_reply(
                security,
                nonceView.data(), nonceView.size(),
                plainView.data(), plainView.size(), out, size);
        });
}

extern "C" JNIEXPORT jbyteArray JNICALL
fromjava(libre3DecryptChallengeResponse)(JNIEnv *env, jclass, jlong context,
                                         jbyteArray nonce,
                                         jbyteArray ciphertext) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) return nullptr;

    ByteArrayView nonceView{env, nonce, L3_LEN_CHALLENGE_NONCE};
    ByteArrayView cipherView{env, ciphertext, L3_LEN_CHALLENGE_RESPONSE_CRYPT};
    if (!nonceView.valid() || !cipherView.valid()) {
        return nullptr;
    }

    return make_byte_array<L3_LEN_CHALLENGE_RESPONSE_PLAIN>(env,
        [&](std::uint8_t **out, std::size_t *size) {
            return l3_sensor_security_decrypt_challenge_response(
                security,
                nonceView.data(), nonceView.size(),
                cipherView.data(), cipherView.size(), out, size);
        });
}

extern "C" JNIEXPORT jbyteArray JNICALL
fromjava(libre3ExportSavedAuthorization)(JNIEnv *env, jclass, jlong context) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) return nullptr;

    return make_byte_array<L3_LEN_SAVED_AUTHORIZATION>(env,
        [security](std::uint8_t **out, std::size_t *size) {
            return l3_sensor_security_export_saved_authorization(security, out, size);
        });
}

extern bool libre3initialized;
bool libre3initialized = true;
