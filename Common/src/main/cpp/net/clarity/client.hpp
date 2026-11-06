// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "protocol.hpp"
#include <functional>
namespace clarity {
// The bulk service has already stored this installation/sequence. Keep this
// distinct from other HTTP failures so the worker can finish the retained post.
class DuplicatePostSequence : public Error {
  public:
    using Error::Error;
};
struct HttpResult {
    int status;
    std::string body;
    std::string location;
};
bool isDuplicatePostSequence(const HttpResult &response);
// Bounded, redacted diagnostics for failed requests only. No raw response or
// request/token contents are logged. The returned text is safe for Status.
std::string httpFailure(std::string_view operation, const HttpResult &response,
                        const Json &config);
using Transport =
    std::function<HttpResult(const std::string &, const std::string &, const std::string &)>;
// A transport callback makes the real client testable against an offline peer.
HttpResult nativePost(const std::string &url, const std::string &body, const std::string &headers);
// The callback sends exactly one POST; this layer handles only 307/308 redirects.
HttpResult postWithRedirects(const std::string &url, const std::string &body,
                             const std::string &headers, const Transport &singlePost);
void validateConfig(const Json &config);
// Repairs v3-v6 UUID placeholders deterministically. The caller saves changes
// before any session request; account/installation IDs and queued data stay put.
bool updateDeviceIdentity(Json &config, Provider &provider);
Json sessionRequest(const Json &config, const std::string &sessionId);
void refreshAccess(Json &config, const Transport &post,
                   const std::function<void(const Json &)> &save, bool force = false);
// dataPost contains exact previously persisted wire bytes. The separately
// checked account comes from the native outbox header.
std::string uploadBytes(Json &config, std::string_view dataPost, std::string_view account,
                   const Transport &post, const std::function<void(const Json &)> &save,
                   const std::function<void()> &checkEnabled);
class Outbox;
// Finish one retained batch on a verified receipt or an explicit bulk
// DuplicatePostSequence. Other errors leave its records pending.
void uploadPending(Json &config, Outbox &outbox, const Transport &post,
                   const std::function<void(const Json &)> &save,
                   const std::function<void()> &checkEnabled);
std::string upload(Json &config, const Json &dataPost, const Transport &post,
                   const std::function<void(const Json &)> &save,
                   const std::function<void()> &checkEnabled);
} // namespace clarity
