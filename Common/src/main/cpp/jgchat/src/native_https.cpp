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
/*      Thu Oct 15 12:53:38 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#include "jgchat/ui_messages.hpp"
#include "jgchat/http.hpp"
#include "jgchat/http_response.hpp"
#include "jgchat/diagnostics.hpp"

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <arpa/inet.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#if defined(__ANDROID__) || defined(__ANDROID_API__)
extern void* openssl();
extern void* opencrypto();
#endif

namespace jgchat {
namespace {
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

template<class T> T symbol(void* handle, const char* name) {
    return reinterpret_cast<T>(dlsym(handle, name));
}
template<class T> T required(void* handle, const char* name) {
    auto value = symbol<T>(handle, name);
    if (!value) throw UiError(UiMessage::detail(UiCode::tls_function_unavailable, name));
    return value;
}
struct TlsApi {
    void* ssl_handle{};
    void* crypto_handle{};
    const SSL_METHOD* (*client_method)(){};
    SSL_CTX* (*ctx_new)(const SSL_METHOD*){};
    void (*ctx_free)(SSL_CTX*){};
    void (*ctx_verify)(SSL_CTX*, int, int (*)(int, X509_STORE_CTX*)){};
    int (*ctx_load_ca)(SSL_CTX*, const char*, const char*){};
    int (*ctx_default_ca)(SSL_CTX*){};
    long (*ctx_ctrl)(SSL_CTX*, int, long, void*){};
    int (*ctx_min_version)(SSL_CTX*, std::uint16_t){};
    SSL* (*ssl_new)(SSL_CTX*){};
    void (*ssl_free)(SSL*){};
    int (*set_fd)(SSL*, int){};
    int (*ssl_connect)(SSL*){};
    int (*ssl_read)(SSL*, void*, int){};
    int (*ssl_write)(SSL*, const void*, int){};
    int (*ssl_error)(const SSL*, int){};
    long (*verify_result)(const SSL*){};
    long (*ssl_ctrl)(SSL*, int, long, void*){};
    int (*sni)(SSL*, const char*){};
    int (*set_host)(SSL*, const char*){};
    X509* (*peer_certificate)(const SSL*){};
    int (*check_host)(X509*, const char*, std::size_t, unsigned int, char**){};
    int (*check_ip)(X509*, const char*, unsigned int){};
    void (*x509_free)(X509*){};
    void (*clear_errors)(){};

    TlsApi() {
#if defined(__ANDROID__) || defined(__ANDROID_API__)
        crypto_handle = ::opencrypto();
        ssl_handle = ::openssl();
#else
        // Never unload: SSL objects and the app's other threads may retain code
        // and thread-local TLS state associated with these shared libraries.
        for (const char* name : {"libcrypto.so.3", "libcrypto.so.1.1", "libcrypto.so"}) {
            crypto_handle = dlopen(name, RTLD_NOW | RTLD_LOCAL); if (crypto_handle) break;
        }
        for (const char* name : {"libssl.so.3", "libssl.so.1.1", "libssl.so"}) {
            ssl_handle = dlopen(name, RTLD_NOW | RTLD_LOCAL); if (ssl_handle) break;
        }
#endif
        if (!ssl_handle || !crypto_handle) throw jgchat::UiError(jgchat::UiCode::native_openssl_boringssl_libraries_unavailable);
        client_method = symbol<decltype(client_method)>(ssl_handle, "TLS_client_method");
        if (!client_method) client_method = required<decltype(client_method)>(ssl_handle, "SSLv23_client_method");
        if (auto init = symbol<int (*)()>(ssl_handle, "SSL_library_init")) {
            if (init() != 1) throw jgchat::UiError(jgchat::UiCode::native_tls_initialization_failed);
        }
#define SSL_FN(member, name) member = required<decltype(member)>(ssl_handle, name)
#define CRYPTO_FN(member, name) member = required<decltype(member)>(crypto_handle, name)
        SSL_FN(ctx_new, "SSL_CTX_new"); SSL_FN(ctx_free, "SSL_CTX_free");
        SSL_FN(ctx_verify, "SSL_CTX_set_verify"); SSL_FN(ctx_load_ca, "SSL_CTX_load_verify_locations");
        SSL_FN(ssl_new, "SSL_new"); SSL_FN(ssl_free, "SSL_free"); SSL_FN(set_fd, "SSL_set_fd");
        SSL_FN(ssl_connect, "SSL_connect"); SSL_FN(ssl_read, "SSL_read"); SSL_FN(ssl_write, "SSL_write");
        SSL_FN(ssl_error, "SSL_get_error"); SSL_FN(verify_result, "SSL_get_verify_result");
        CRYPTO_FN(x509_free, "X509_free"); CRYPTO_FN(clear_errors, "ERR_clear_error");
#undef SSL_FN
#undef CRYPTO_FN
        ctx_default_ca = symbol<decltype(ctx_default_ca)>(ssl_handle, "SSL_CTX_set_default_verify_paths");
        ctx_ctrl = symbol<decltype(ctx_ctrl)>(ssl_handle, "SSL_CTX_ctrl");
        ctx_min_version = symbol<decltype(ctx_min_version)>(ssl_handle, "SSL_CTX_set_min_proto_version");
        ssl_ctrl = symbol<decltype(ssl_ctrl)>(ssl_handle, "SSL_ctrl");
        sni = symbol<decltype(sni)>(ssl_handle, "SSL_set_tlsext_host_name");
        set_host = symbol<decltype(set_host)>(ssl_handle, "SSL_set1_host");
        peer_certificate = symbol<decltype(peer_certificate)>(ssl_handle, "SSL_get1_peer_certificate");
        if (!peer_certificate) peer_certificate = required<decltype(peer_certificate)>(ssl_handle, "SSL_get_peer_certificate");
        check_host = symbol<decltype(check_host)>(crypto_handle, "X509_check_host");
        check_ip = symbol<decltype(check_ip)>(crypto_handle, "X509_check_ip_asc");
        if (!set_host && !check_host) throw jgchat::UiError(jgchat::UiCode::native_tls_hostname_verification_unavailable);
        if (!sni && !ssl_ctrl) throw jgchat::UiError(jgchat::UiCode::native_tls_sni_unavailable);
        if (!ctx_min_version && !ctx_ctrl) throw jgchat::UiError(jgchat::UiCode::native_tls_minimum_version_control_unavailable);
    }
};
TlsApi& tls_api() { static TlsApi api; return api; }

struct Socket {
    int fd{-1};
    explicit Socket(int value = -1) : fd(value) {}
    ~Socket() { if (fd >= 0) ::close(fd); }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&& other) noexcept : fd(std::exchange(other.fd, -1)) {}
};

// OpenSSL socket BIO may use write(), which can raise SIGPIPE on disconnect.
// Block it only on this worker thread; consume our signal before restoring the
// prior mask. This does not change Juggluco's process-wide signal disposition.
class BlockSigpipe {
    sigset_t set_{}, prior_{};
    bool active_{false}, pending_before_{false};
public:
    BlockSigpipe() {
        sigemptyset(&set_); sigaddset(&set_, SIGPIPE);
        sigset_t pending{}; sigpending(&pending); pending_before_ = sigismember(&pending, SIGPIPE) == 1;
        if (pthread_sigmask(SIG_BLOCK, &set_, &prior_) != 0)
            throw jgchat::UiError(jgchat::UiCode::cannot_protect_native_tls_write_against_sigpipe);
        active_ = true;
    }
    ~BlockSigpipe() {
        if (!active_) return;
        if (!pending_before_ && sigismember(&prior_, SIGPIPE) != 1) {
            // SIGPIPE from a socket write is directed at this thread. It stays
            // pending while blocked, so sigwait cannot race another reader of
            // this thread's pending signal. Unlike sigtimedwait, sigwait exists
            // on Android before API 23.
            sigset_t pending{};
            if (sigpending(&pending) == 0 && sigismember(&pending, SIGPIPE) == 1) {
                int consumed = 0;
                sigwait(&set_, &consumed);
            }
        }
        pthread_sigmask(SIG_SETMASK, &prior_, nullptr);
    }
};

void check_deadline(Clock::time_point deadline, const std::atomic_bool& cancel) {
    if (cancel.load()) throw jgchat::UiError(jgchat::UiCode::native_https_cancelled);
    if (Clock::now() >= deadline) throw jgchat::UiError(jgchat::UiCode::native_https_timed_out);
}
void wait_socket(int socket, short events, Clock::time_point deadline, const std::atomic_bool& cancel) {
    for (;;) {
        check_deadline(deadline, cancel);
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now());
        pollfd fd{socket, events, 0};
        const int result = poll(&fd, 1, static_cast<int>(std::clamp<std::int64_t>(left.count(), 1, 50)));
        if (result > 0) {
            if (fd.revents & POLLNVAL) throw jgchat::UiError(jgchat::UiCode::native_https_socket_closed);
            return; // SSL/connect reports EOF/POLLERR with its actual operation.
        }
        if (result < 0 && errno != EINTR) throw jgchat::UiError(jgchat::UiCode::native_https_socket_poll_failed);
    }
}
Socket connect_socket(const HttpRequest& request, Clock::time_point deadline, const std::atomic_bool& cancel) {
    check_deadline(deadline, cancel);
    addrinfo hints{}; hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM;
    addrinfo* addresses = nullptr;
    // The only call without our own interruptible deadline: the platform DNS
    // resolver controls its timeout. We neither detach it nor retain references
    // to request/cancel after returning. Cancellation is checked immediately.
    const auto port = std::to_string(request.port);
    int result = getaddrinfo(request.host.c_str(), port.c_str(), &hints, &addresses);
    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> owned(addresses, freeaddrinfo);
    check_deadline(deadline, cancel);
    if (result != 0) throw jgchat::UiError(jgchat::UiCode::native_https_dns_resolution_failed);
    for (auto* address = addresses; address; address = address->ai_next) {
        check_deadline(deadline, cancel);
        Socket socket(::socket(address->ai_family, address->ai_socktype, address->ai_protocol));
        if (socket.fd < 0) continue;
        int flags = fcntl(socket.fd, F_GETFL, 0);
        if (flags < 0 || fcntl(socket.fd, F_SETFL, flags | O_NONBLOCK) < 0 ||
            fcntl(socket.fd, F_SETFD, FD_CLOEXEC) < 0) continue;
        result = ::connect(socket.fd, address->ai_addr, address->ai_addrlen);
        if (result == 0) return socket;
        if (errno != EINPROGRESS && errno != EWOULDBLOCK && errno != EINTR) continue;
        // An unusable IPv6 address must not consume the entire request budget
        // when the same hostname has a usable IPv4 address.
        try {
            wait_socket(socket.fd, POLLOUT, std::min(deadline, Clock::now() + 2s), cancel);
        } catch (const std::runtime_error&) {
            check_deadline(deadline, cancel);
            continue;
        }
        int error = 0; socklen_t size = sizeof(error);
        if (getsockopt(socket.fd, SOL_SOCKET, SO_ERROR, &error, &size) == 0 && error == 0) return socket;
    }
    throw jgchat::UiError(jgchat::UiCode::native_https_tcp_connection_failed);
}

void load_trust(TlsApi& api, SSL_CTX* context) {
#if defined(__ANDROID__) || defined(__ANDROID_API__)
    bool loaded = false;
    for (const char* path : {"/apex/com.android.conscrypt/cacerts", "/system/etc/security/cacerts"}) {
        // OpenSSL can accept a nonexistent lazy-load directory. Verify it is
        // accessible before considering it a usable trust-store location.
        DIR* directory = opendir(path);
        if (!directory) continue;
        closedir(directory);
        if (api.ctx_load_ca(context, nullptr, path) == 1) loaded = true;
    }
    if (!loaded) throw jgchat::UiError(jgchat::UiCode::android_system_ca_certificates_unavailable);
#else
    if (!api.ctx_default_ca || api.ctx_default_ca(context) != 1)
        throw jgchat::UiError(jgchat::UiCode::system_ca_certificates_unavailable);
#endif
}
// Retry a nonblocking TLS operation only on WANT_READ/WANT_WRITE.
void tls_retry(TlsApi& api, SSL* ssl, int result, int fd, Clock::time_point deadline,
               const std::atomic_bool& cancel, UiCode error) {
    const int reason = api.ssl_error(ssl, result);
    if (reason == SSL_ERROR_WANT_READ) wait_socket(fd, POLLIN, deadline, cancel);
    else if (reason == SSL_ERROR_WANT_WRITE) wait_socket(fd, POLLOUT, deadline, cancel);
    else throw UiError(error);
}

HttpResponse request_https(const HttpRequest& request, const std::atomic_bool& cancel) {
    DiagnosticOperation trace("native_https");
    trace.stage("validate_request");
    validate_http_request(request);
    const auto deadline = Clock::now() + request.timeout;
    check_deadline(deadline, cancel);
    trace.stage("tls_setup");
    auto& api = tls_api();
    BlockSigpipe no_sigpipe;
    std::unique_ptr<SSL_CTX, decltype(api.ctx_free)> context(api.ctx_new(api.client_method()), api.ctx_free);
    if (!context) throw jgchat::UiError(jgchat::UiCode::native_tls_context_allocation_failed);
    // BoringSSL exposes a real setter; OpenSSL exposes this as SSL_CTX_ctrl.
    constexpr int set_min_proto_version = 123;
    const auto min_ok = api.ctx_min_version ? api.ctx_min_version(context.get(), TLS1_2_VERSION) :
        api.ctx_ctrl(context.get(), set_min_proto_version, TLS1_2_VERSION, nullptr);
    if (min_ok != 1) throw jgchat::UiError(jgchat::UiCode::cannot_require_tls_1_2_or_newer);
    api.ctx_verify(context.get(), SSL_VERIFY_PEER, nullptr);
    load_trust(api, context.get());
    std::unique_ptr<SSL, decltype(api.ssl_free)> ssl(api.ssl_new(context.get()), api.ssl_free);
    if (!ssl) throw jgchat::UiError(jgchat::UiCode::native_tls_connection_allocation_failed);
    constexpr int set_sni = 55; // SSL_CTRL_SET_TLSEXT_HOSTNAME, OpenSSL ABI.
    const auto sni_ok = api.sni ? api.sni(ssl.get(), request.host.c_str()) :
        api.ssl_ctrl(ssl.get(), set_sni, 0, const_cast<char*>(request.host.c_str()));
    if (sni_ok != 1) throw jgchat::UiError(jgchat::UiCode::cannot_set_tls_server_name);
    in_addr ip4{};
    const bool ip_literal = inet_pton(AF_INET, request.host.c_str(), &ip4) == 1;
    if (ip_literal && !api.check_ip) throw jgchat::UiError(jgchat::UiCode::native_tls_ip_certificate_verification_unavailable);
    // Hostname verification is performed by the library when available and
    // independently against the peer certificate after the handshake below.
    if (!ip_literal && api.set_host && api.set_host(ssl.get(), request.host.c_str()) != 1)
        throw jgchat::UiError(jgchat::UiCode::cannot_configure_tls_hostname_verification);
    trace.stage("dns_and_tcp");
    auto socket = connect_socket(request, deadline, cancel);
    if (api.set_fd(ssl.get(), socket.fd) != 1) throw jgchat::UiError(jgchat::UiCode::cannot_attach_tls_socket);
    trace.stage("tls_handshake");
    for (;;) {
        check_deadline(deadline, cancel);
        api.clear_errors();
        const int result = api.ssl_connect(ssl.get());
        if (result == 1) break;
        tls_retry(api, ssl.get(), result, socket.fd, deadline, cancel,
                  UiCode::native_tls_handshake_or_certificate_chain_verification_failed);
    }
    if (api.verify_result(ssl.get()) != X509_V_OK) throw jgchat::UiError(jgchat::UiCode::tls_certificate_chain_verification_failed);
    std::unique_ptr<X509, decltype(api.x509_free)> certificate(api.peer_certificate(ssl.get()), api.x509_free);
    if (!certificate) throw jgchat::UiError(jgchat::UiCode::tls_peer_certificate_missing);
    if (ip_literal) {
        if (api.check_ip(certificate.get(), request.host.c_str(), 0) != 1)
            throw jgchat::UiError(jgchat::UiCode::tls_certificate_ip_mismatch);
    } else if (api.check_host) {
        if (api.check_host(certificate.get(), request.host.c_str(), request.host.size(), 0, nullptr) != 1)
            throw jgchat::UiError(jgchat::UiCode::tls_certificate_hostname_mismatch);
    } else if (!api.set_host) throw jgchat::UiError(jgchat::UiCode::tls_hostname_verification_unavailable);

    trace.stage("send_request");
    const auto output = serialize_http_request(request);
    for (std::size_t pos = 0; pos < output.size();) {
        check_deadline(deadline, cancel);
        api.clear_errors();
        const int result = api.ssl_write(ssl.get(), output.data() + pos, static_cast<int>(output.size() - pos));
        if (result > 0) pos += static_cast<std::size_t>(result);
        else tls_retry(api, ssl.get(), result, socket.fd, deadline, cancel, UiCode::native_tls_write_failed);
    }
    HttpResponseParser parser(request.max_response_bytes, request.on_body);
    std::array<char, 8192> buffer{};
    trace.stage("read_response");
    for (;;) {
        check_deadline(deadline, cancel);
        api.clear_errors();
        const int result = api.ssl_read(ssl.get(), buffer.data(), static_cast<int>(buffer.size()));
        if (result > 0) {
            parser.append(std::string_view(buffer.data(), static_cast<std::size_t>(result)));
            if (parser.complete()) { trace.success(); return parser.result(); }
        } else {
            const int reason = api.ssl_error(ssl.get(), result);
            if (reason == SSL_ERROR_ZERO_RETURN) {
                auto response = parser.finish(); trace.success(); return response;
            }
            // A TLS EOF without close_notify is not accepted for EOF-delimited
            // responses. A Content-Length/chunked response already returned.
            if (reason == SSL_ERROR_WANT_READ) wait_socket(socket.fd, POLLIN, deadline, cancel);
            else if (reason == SSL_ERROR_WANT_WRITE) wait_socket(socket.fd, POLLOUT, deadline, cancel);
            else throw jgchat::UiError(jgchat::UiCode::native_tls_read_failed_or_ended_without_close_notify);
        }
    }
}
}

HttpClient native_https() {
    return [](const HttpRequest& request, const std::atomic_bool& cancel) -> HttpResponse {
        try { return request_https(request, cancel); }
        catch (...) { return http_failure(current_ui_error()); }
    };
}
} // namespace jgchat
