#include "config.h"
#include "share/logs.hpp"
#include "fromjava.h"
#include "process/sensor_security_context.h"
#include "share/hexstr.hpp"

#include <jni.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>

extern "C" __attribute__((used, visibility("default")))
const char L3_PROCESS_BUILD_ID[] =
    "L3_PROCESS_BUILD_ID=libre3_process_app_garmin_final_size7_2026-08-15 "
    "features=mont32,r3_import,a_minus3,round_notrace,parent_live6860,"
    "final_ws_live7756,shared_scratch1,fixed_core132,hidden_gc"
#if defined(L3_LIBRE3_SIZE3_IPO)
    ",ipo_lto"
#else
    ",ipo_not_linked"
#endif
#if defined(L3_LIBRE3_SIZE7_SAFE_ICF)
    ",icf_safe"
#else
    ",icf_not_linked"
#endif
#if defined(L3_LIBRE3_SIZE3_MINIMAL_UNWIND)
    ",minimal_unwind"
#else
    ",unwind_kept"
#endif
    ",direct_authroot,digest_safe,f407_words176,lazy_f407_output"
    ",f407_direct_byte,ccm60_scratch "
    "base=size6_sensor_ok";


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

[[nodiscard]] l3_sensor_security_context *security_context_from_handle(jlong handle) noexcept {
    return reinterpret_cast<l3_sensor_security_context *>(static_cast<std::uintptr_t>(handle));
}

[[nodiscard]] jlong security_handle_from_context(l3_sensor_security_context *context) noexcept {
    return static_cast<jlong>(reinterpret_cast<std::uintptr_t>(context));
}




template<std::size_t ExpectedSize, class Producer>
[[nodiscard]] jbyteArray make_fixed_byte_array(JNIEnv *env, Producer &&producer) {
    constexpr std::size_t GuardSize = 64;
    auto bytes = std::make_unique<std::uint8_t[]>(ExpectedSize + GuardSize);
    std::fill(bytes.get(), bytes.get() + ExpectedSize + GuardSize, 0xA5u);

    const int rc = producer(bytes.get());
    if (rc != 1) {
        return nullptr;
    }

#ifndef NOLOG
    for (std::size_t i = ExpectedSize; i < ExpectedSize + GuardSize; ++i) {
        if (bytes[i] != 0xA5u) {
            LOGGER("L3BUG fixed output overflow size=%zu guard_offset=%zu\n",
                   ExpectedSize, i - ExpectedSize);
            return nullptr;
        }
    }
#endif

    jbyteArray result = env->NewByteArray(static_cast<jsize>(ExpectedSize));
    if (!result) {
        return nullptr;
    }

    env->SetByteArrayRegion(result, 0, static_cast<jsize>(ExpectedSize),
                            reinterpret_cast<const jbyte *>(bytes.get()));
    return env->ExceptionCheck() ? nullptr : result;
}

void log_l3_process_build_once() noexcept {
#ifndef NOLOG
    static bool logged = false;
    if (!logged) {
        logged = true;
        LOGGER("L3BUILD %s\n", L3_PROCESS_BUILD_ID);
    }
#endif
}


#ifndef NOLOG
void log_hex_bytes(const char *label, const std::uint8_t *data, std::size_t len) noexcept {
    static constexpr char hex[] = "0123456789ABCDEF";
    if (!label) return;
    if (!data && len) {
        LOGGER("%s <null len=%zu>\n", label, len);
        return;
    }
    char buf[2 * 96 + 1];
    std::size_t off = 0;
    while (off < len) {
        const std::size_t n = std::min<std::size_t>(96, len - off);
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint8_t b = data[off + i];
            buf[2 * i] = hex[b >> 4];
            buf[2 * i + 1] = hex[b & 0x0f];
        }
        buf[2 * n] = '\0';
        LOGGER("%s[%zu..%zu)#%zu %s\n", label, off, off + n, len, buf);
        off += n;
    }
    if (len == 0) {
        LOGGER("%s#0\n", label);
    }
}

void log_authorization_root(const char *stage, const l3_sensor_security_context *security) noexcept {
    if (!stage || !security) return;
    uint32_t meta[4] = {0, 0, 0, 0};
    std::uint8_t bytes[256];
    std::size_t len = 0;
    const int rc = l3_sensor_security_debug_copy_authorization_root(security, meta, bytes, sizeof(bytes), &len);
    LOGGER("L3AUTH %s root rc=%d len=%zu kind=%08X tag=%08X meta0=%08X meta1=%08X\n",
           stage, rc, len, meta[0], meta[1], meta[2], meta[3]);
    if (rc == 1 && len <= sizeof(bytes)) {
        log_hex_bytes("L3AUTH root", bytes, len);
    }
}
#endif

} // namespace

/* Allocate the per-Libre3GattCallback security state on first use.  On a
 * reconnect, reset and reuse the existing state instead of allocating a new
 * object.  Returning 0 means creation/reset failed. */
extern "C" JNIEXPORT jlong JNICALL
fromjava(libre3BeginSecurityHandshake)(JNIEnv *, jclass, jlong context) {
    log_l3_process_build_once();
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
    LOGGER("libre3FreeSecurityContext(%p) delayed/no-op\n", static_cast<void *>(security));
#endif
    /* The full 2026-08-14 log showed a SIGBUS immediately at sensor termination
     * with x0 equal to this context pointer.  Java can still receive queued BLE
     * callbacks while free() is unwinding, so destroying the native object here
     * creates a race/use-after-free window.  Keep the small context allocation
     * alive until process exit for this stability build. */
    (void)security;
}

extern "C" JNIEXPORT jint JNICALL
fromjava(libre3SelectAppKeyAndSavedAuthorization)(JNIEnv *env, jclass, jlong context,
                                  jint securityVersion,
                                  jbyteArray savedAuthorization) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) return static_cast<jint>(L3_SECURITY_ERR_ARGUMENT);

    ByteArrayView saved{env, savedAuthorization, L3_LEN_SAVED_AUTHORIZATION, true};
    if (!saved.valid() || securityVersion < 0) {
        return static_cast<jint>(L3_SECURITY_ERR_ARGUMENT);
    }

    const int rc = l3_sensor_security_select_app_key_and_saved_authorization(
        security, static_cast<unsigned>(securityVersion), saved.data(), saved.size());
#ifndef NOLOG
    LOGGER("libre3SelectAppKeyAndSavedAuthorization(%p,version=%d,saved#%zu)=%d\n",
           static_cast<void *>(security), static_cast<int>(securityVersion), saved.size(), rc);
    log_authorization_root(saved.size() ? "after_saved_import" : "after_no_saved_select", security);
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
    log_hex_bytes("L3AUTH patch_cert", certificate.data(), certificate.size());
#endif
    return static_cast<jint>(rc);
}

extern "C" JNIEXPORT jbyteArray JNICALL
fromjava(libre3CreateEphemeralPublicKey)(JNIEnv *env, jclass, jlong context) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) return nullptr;
#ifndef NOLOG
    LOGGER("libre3CreateEphemeralPublicKey(%p) begin\n", static_cast<void *>(security));
#endif
    return make_fixed_byte_array<L3_LEN_EPHEMERAL_PUBLIC_KEY>(env,
        [security](std::uint8_t *out) {
            const int rc = l3_sensor_security_create_ephemeral_public_key_into(security, out);
#ifndef NOLOG
            LOGGER("libre3CreateEphemeralPublicKey(%p) rc=%d\n", static_cast<void *>(security), rc);
            if (rc == 1) log_hex_bytes("L3AUTH app_ephemeral_public64", out, L3_LEN_EPHEMERAL_PUBLIC_KEY);
#endif
            return rc;
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
    LOGGER("libre3DeriveAuthorizationRoot(%p,publicKey#%zu)=%d stage=%u\n",
           static_cast<void *>(security), publicKey.size(), rc,
           security->engine.authorization_root_stage);
    log_hex_bytes("L3AUTH patch_ephemeral_public65", publicKey.data(), publicKey.size());
    log_authorization_root("after_fresh_derive", security);
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

#ifndef NOLOG
    LOGGER("libre3EncryptChallengeReply(%p,nonce#%zu,plain#%zu) begin\n",
           static_cast<void *>(security), nonceView.size(), plainView.size());
#endif
    return make_fixed_byte_array<L3_LEN_CHALLENGE_REPLY_CRYPT>(env,
        [&](std::uint8_t *out) {
            const int rc = l3_sensor_security_encrypt_challenge_reply_into(
                security,
                nonceView.data(), nonceView.size(),
                plainView.data(), plainView.size(), out);
#ifndef NOLOG
            LOGGER("libre3EncryptChallengeReply(%p) rc=%d\n", static_cast<void *>(security), rc);
            if (rc == 1) log_hex_bytes("L3AUTH challenge_reply40", out, L3_LEN_CHALLENGE_REPLY_CRYPT);
#endif
            return rc;
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

    return make_fixed_byte_array<L3_LEN_CHALLENGE_RESPONSE_PLAIN>(env,
        [&](std::uint8_t *out) {
            return l3_sensor_security_decrypt_challenge_response_into(
                security,
                nonceView.data(), nonceView.size(),
                cipherView.data(), cipherView.size(), out);
        });
}

extern "C" JNIEXPORT jbyteArray JNICALL
fromjava(libre3ExportSavedAuthorization)(JNIEnv *env, jclass, jlong context) {
    l3_sensor_security_context *security = security_context_from_handle(context);
    if (!security) return nullptr;

    return make_fixed_byte_array<L3_LEN_SAVED_AUTHORIZATION>(env,
        [security](std::uint8_t *out) {
            const int rc = l3_sensor_security_export_saved_authorization_into(security, out);
#ifndef NOLOG
            LOGGER("libre3ExportSavedAuthorization(%p) rc=%d\n", static_cast<void *>(security), rc);
            if (rc == 1) log_hex_bytes("L3AUTH exported149", out, L3_LEN_SAVED_AUTHORIZATION);
#endif
            return rc;
        });
}

extern bool libre3initialized;
bool libre3initialized = true;
