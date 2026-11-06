// SPDX-License-Identifier: GPL-3.0-or-later
#include "client.hpp"
#include "auth.hpp"
#include "certificate.hpp"
#include "outbox.hpp"
#include <ctime>
namespace clarity {
void validateDexcomUrl(const std::string &s) {
    if (!s.starts_with("https://") || s.size() > 1024 || s.find_first_of("\r\n\t @#") != s.npos)
        throw Error("Invalid Clarity HTTPS URL");
    auto end = s.find('/', 8);
    if (end == s.npos || end == 8)
        throw Error("Invalid Clarity HTTPS URL");
    auto host = s.substr(8, end - 8);
    if (host.find(':') != host.npos ||
        (!host.ends_with(".dexcom.com") && !host.ends_with(".dexcom.eu")))
        throw Error("Expected a Dexcom endpoint");
}
static void token(const std::string &s) {
    if (s.empty() || s.size() > 32768 || s.find_first_of("\r\n\0", 0, 3) != s.npos)
        throw Error("Invalid authentication data");
}
void validateConfig(const Json &c) {
    if (c.at("schema") != "juggluco-clarity-setup/1")
        throw Error("Unsupported Clarity setup file");
    for (auto k : {"account", "appInstanceId", "installationId"})
        if (!isUuid(c.at(k).get<std::string>()))
            throw Error("Invalid Clarity identity");
    for (auto k : {"sessionUrl", "bulkUrl", "tokenUrl"})
        validateDexcomUrl(c.at(k));
    if (c.contains("certificateUrl")) {
        const auto url = c.at("certificateUrl").get<std::string>();
        validateDexcomUrl(url);
        if (!url.ends_with("/Certificate/UserCert"))
            throw Error("Wrong Dexcom certificate service path");
    }
    if (!c.at("bulkUrl").get<std::string>().ends_with("/bulkData/v1/patient/postBulkData") ||
        !c.at("sessionUrl").get<std::string>().ends_with("/security/v1/patient/updateSessionKeys"))
        throw Error("Wrong Clarity service paths");
    if (!c.at("tokenUrl").get<std::string>().ends_with("/connect/token"))
        throw Error("Wrong OAuth token path");
    token(c.at("accessToken"));
    for (auto k : {"deviceId", "hardwareId", "clientId"})
        token(c.at(k));
    if (!c.at("runtimeInfo").is_object())
        throw Error("Missing Clarity runtime information");
    for (auto k : {"AppName", "AppVersion", "DeviceManufacturer", "DeviceModel", "DeviceOsName",
                   "DeviceOsVersion", "AppNumber"})
        token(c.at("runtimeInfo").at(k));
}
bool updateDeviceIdentity(Json &c, Provider &provider) {
    std::string hardware = c.at("hardwareId");
    const bool legacy = isUuid(hardware);
    if (legacy) {
        hardware.erase(std::remove(hardware.begin(), hardware.end(), '-'), hardware.end());
        hardware = base64(unhex(hardware), true);
    }
    const auto device = provider.deviceId(hardware);
    if (c.at("hardwareId") == hardware && c.value("deviceId", "") == device)
        return false;
    c["hardwareId"] = hardware;
    c["deviceId"] = device;
    diagnostic("device identity derived with native engine signing key (legacy migration=%d)",
               legacy);
    return true;
}
Json sessionRequest(const Json &c, const std::string &sessionId) {
    return {{"DeviceId", c.at("deviceId")},
            {"HardwareId", c.at("hardwareId")},
            {"ClientOAuthToken", c.at("accessToken")},
            {"RuntimeInfo", c.at("runtimeInfo")},
            {"SessionRequestId", sessionId}};
}
void refreshAccess(Json &c, const Transport &post, const std::function<void(const Json &)> &save,
                   bool force) {
    if (!force && c.value("tokenExpiresAt", int64_t(0)) > time(nullptr) + 120)
        return;
    diagnostic("refreshing Dexcom access token (forced=%d)", force);
    std::string refresh = c.value("refreshToken", "");
    if (refresh.empty())
        throw Error("Sign in to Dexcom again");
    token(refresh);
    std::string request =
        "grant_type=refresh_token&client_id=" + formEncode(c.at("clientId").get<std::string>()) +
        "&refresh_token=" + formEncode(refresh);
    if (c.contains("clientSecret"))
        request += "&client_secret=" + formEncode(c.at("clientSecret").get<std::string>());
    auto r = post(c.at("tokenUrl"), request, tokenHeaders(c));
    if (r.status != 200)
        throw Error(httpFailure("Dexcom token refresh", r, c));
    auto j = Json::parse(r.body);
    std::string access = j.at("access_token");
    token(access);
    // Check the token's account binding when a sub claim is supplied. TLS
    // authenticates this token response; this is not an offline JWT verifier.
    auto a = access.find('.'), b = access.find('.', a == access.npos ? 0 : a + 1);
    if (a != access.npos && b != access.npos) {
        auto claims = Json::parse(text(unbase64(access.substr(a + 1, b - a - 1), true)));
        if (claims.contains("sub") && claims["sub"] != c["account"])
            throw Error("Refreshed token account mismatch");
        if (claims.contains("client_id") && claims["client_id"] != c["clientId"])
            throw Error("Refreshed token client mismatch");
    }
    c["accessToken"] = access;
    c["tokenExpiresAt"] = int64_t(time(nullptr)) + j.at("expires_in").get<int64_t>();
    if (j.contains("refresh_token")) {
        std::string value = j["refresh_token"];
        token(value);
        c["refreshToken"] = value;
    }
    // Commit a rotated refresh token before attempting any subsequent request.
    save(c);
    diagnostic("refreshed Dexcom tokens saved");
    diagnostic("refreshed token permissions: %s", tokenPermissionSummary(access).c_str());
}
std::string uploadBytes(Json &c, std::string_view data, std::string_view dataAccount, const Transport &post,
                   const std::function<void(const Json &)> &save,
                   const std::function<void()> &checkEnabled) {
    validateConfig(c);
    if (dataAccount != c.at("account").get<std::string>() || data.empty())
        throw Error("Pending batch belongs to another account or is empty");
    checkEnabled();
    refreshAccess(c, post, save);
    diagnostic("initializing native provider for upload");
    Provider provider;
    if (updateDeviceIdentity(c, provider)) {
        save(c);
        diagnostic("corrected device identity saved before session bootstrap");
    }
    ensureUserCertificate(c, provider, post, save, checkEnabled);
    const auto identity = provider.identity();
    std::string account = c.at("account");
    auto headers = [&](const Json &h) {
        return "\r\nContent-Type: application/json\r\nAccept: "
               "application/json\r\nAuthorization: "
               "Bearer " +
               c.at("accessToken").get<std::string>() + "\r\nX-Account-Id: " + account +
               "\r\nX-Software-Id: " + identity.at("SoftwareId").get<std::string>() +
               "\r\nX-Trace-Id: " + h.at("RequestId").get<std::string>() +
               "\r\nX-Security-Schema: SignedMessage";
    };
    // Each bounded batch gets fresh session keys and a fresh native provider.
    // Unloading it also releases the adapter's arena and all opaque contexts.
    const auto sessionId = uuid();
    auto h = messageHeader(c, identity, false, uuid());
    auto request = sessionRequest(c, sessionId);
    checkEnabled();
    diagnostic("requesting upload session keys");
    diagnostic("upload token permissions: %s",
               tokenPermissionSummary(c.at("accessToken").get<std::string>()).c_str());
    auto r = post(c.at("sessionUrl"), seal(provider, h, request.dump()), headers(h));
    if (r.status == 401) {
        httpFailure("Clarity session bootstrap", r, c);
        refreshAccess(c, post, save, true);
        throw Error("Access token refreshed; retry pending batch");
    }
    if (r.status != 200)
        throw Error(httpFailure("Clarity session bootstrap", r, c));
    auto response = openResponse(provider, r.body, h.at("RequestId"), account);
    diagnostic("session response signature and identity verified; importing session keys");
    auto keys = Json::parse(text(response.plain));
    // PriorSessionRequestId describes the previous session. The observed
    // bootstrap response does NOT echo this request's new SessionRequestId.
    // openResponse already verifies the signed RequestId and AccountId.
    provider.importSession(
        unbase64(keys.at("EncKey").get<std::string>(), true),
        unbase64(keys.at("SigKey").get<std::string>(), true),
        unbase64(response.header.at("EncKey").at("IV").get<std::string>(), true));
    h = messageHeader(c, identity, true, uuid());
    auto wire = seal(provider, h, data);
    checkEnabled();
    diagnostic("sending pending bulk batch");
    r = post(c.at("bulkUrl"), wire, headers(h));
    if (r.status == 401) {
        httpFailure("Clarity upload", r, c);
        refreshAccess(c, post, save, true);
        throw Error("Access token refreshed; retry pending batch");
    }
    if (r.status != 200) {
        const auto failure = httpFailure("Clarity upload", r, c);
        if (isDuplicatePostSequence(r))
            throw DuplicatePostSequence(failure);
        throw Error(failure);
    }
    response = openResponse(provider, r.body, h.at("RequestId"), account);
    auto ack = text(response.plain);
    if (!isUuid(ack))
        throw Error("Unrecognized Clarity acknowledgment");
    diagnostic("bulk response signature, account, request and acknowledgment verified");
    return ack;
}
void uploadPending(Json &c, Outbox &outbox, const Transport &post,
                   const std::function<void(const Json &)> &save,
                   const std::function<void()> &checkEnabled) {
    try {
        auto receipt = uploadBytes(c, outbox.pendingBody(), outbox.account(), post, save, checkEnabled);
        outbox.acknowledge(receipt);
    } catch (const DuplicatePostSequence &) {
        diagnostic("bulk service already stored the retained post; saving its local progress without resending records");
        outbox.acknowledgeAlreadyStored();
    }
}
std::string upload(Json &c, const Json &data, const Transport &post,
                   const std::function<void(const Json &)> &save,
                   const std::function<void()> &checkEnabled) {
    return uploadBytes(c, data.dump(), data.at("DataPost").at("AccountID").get<std::string>(),
                       post, save, checkEnabled);
}
} // namespace clarity
