// SPDX-License-Identifier: GPL-3.0-or-later
#include "client.hpp"
#include "auth.hpp"
#include <cctype>
namespace clarity {
namespace {
std::string lower(std::string_view s) {
    std::string out(s);
    for (char &c : out)
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
    return out;
}
void replaceAll(std::string &s, std::string_view value) {
    if (value.empty())
        return;
    for (size_t at = 0; (at = s.find(value, at)) != s.npos; at += 10)
        s.replace(at, value.size(), "[redacted]");
}
std::string redact(std::string s, const Json &config) {
    // Replace known values before bounding the output: a truncated token must
    // not escape because it no longer matches the complete secret.
    for (const char *key : {"accessToken", "refreshToken", "clientSecret", "account", "deviceId",
                             "hardwareId", "appInstanceId", "installationId", "code", "verifier"}) {
        auto it = config.find(key);
        if (it == config.end() || !it->is_string())
            continue;
        const auto &secret = it->get_ref<const std::string &>();
        replaceAll(s, secret);
        replaceAll(s, formEncode(secret));
    }
    // Drop markup, collapse whitespace, and suppress URLs, emails, JWTs,
    // UUIDs and other long opaque atoms even if they are not in our config.
    std::string clean;
    bool tag = false;
    for (unsigned char c : s) {
        if (c == '<') {
            tag = true;
            clean += ' ';
        } else if (c == '>') {
            tag = false;
        } else if (!tag)
            clean += c >= 32 && c < 127 ? char(c) : ' ';
    }
    auto atom = [](unsigned char c) {
        return std::isalnum(c) || std::string_view("_-.+/=@:%?&#").find(char(c)) !=
                                     std::string_view::npos;
    };
    std::string out;
    for (size_t i = 0; i < clean.size();) {
        if (atom(clean[i])) {
            const auto begin = i;
            while (i < clean.size() && atom(clean[i]))
                ++i;
            auto word = std::string_view(clean).substr(begin, i - begin);
            const auto folded = lower(word);
            if (word.size() >= 24 || word.find('@') != word.npos ||
                folded.starts_with("http:") || folded.starts_with("https:") ||
                folded.starts_with("bearer") || word.find('=') != word.npos)
                out += "[redacted]";
            else
                out += word;
        } else {
            char c = clean[i++];
            if (c != ' ' || (!out.empty() && out.back() != ' '))
                out += c;
        }
    }
    while (!out.empty() && out.back() == ' ')
        out.pop_back();
    if (out.size() > 640) {
        out.resize(640);
        out += "...";
    }
    return out;
}
void errorFields(const Json &j, std::string &out, unsigned depth = 0) {
    if (depth > 4 || out.size() > 8192)
        return;
    if (j.is_array()) {
        for (const auto &v : j)
            errorFields(v, out, depth + 1);
        return;
    }
    if (!j.is_object())
        return;
    for (auto it = j.begin(); it != j.end(); ++it) {
        const auto key = lower(it.key());
        const bool field = key == "code" || key == "subcode" || key == "errorcode" ||
                           key == "error" || key == "message" || key == "reason" ||
                           key == "title" || key == "detail" || key == "description" ||
                           key == "error_description" || key == "status" ||
                           key == "status_code" || key == "mtls_status";
        if (field && (it->is_string() || it->is_number() || it->is_boolean())) {
            if (!out.empty())
                out += "; ";
            out += key + ": " + (it->is_string() ? it->get<std::string>() : it->dump());
        } else if (key == "error" || key == "errors" || key == "innererror")
            errorFields(*it, out, depth + 1);
    }
}
} // namespace
bool isDuplicatePostSequence(const HttpResult &r) {
    // Match the structured bulk error code, not a substring in a diagnostic or
    // its nested exception text. Other 500 responses must retain the batch.
    if (r.status != 500 || r.body.empty() || r.body.size() > 64 * 1024)
        return false;
    const auto body = Json::parse(r.body, nullptr, false);
    if (!body.is_object())
        return false;
    bool found = false;
    for (auto it = body.begin(); it != body.end(); ++it)
        if (lower(it.key()) == "code") {
            if (!it->is_string() || it->get_ref<const std::string &>() != "DuplicatePostSequence")
                return false;
            found = true;
        }
    return found;
}
std::string httpFailure(std::string_view operation, const HttpResult &r, const Json &config) {
    auto message = std::string(operation) + " failed (HTTP " + std::to_string(r.status) + ")";
    std::string detail, format = "empty";
    try {
        if (r.body.size() > 64 * 1024) {
            format = "oversize";
            detail = "error body exceeds diagnostic limit";
        } else if (!r.body.empty()) {
            const auto j = Json::parse(r.body, nullptr, false);
            if (!j.is_discarded()) {
                format = "json";
                errorFields(j, detail);
                if (detail.empty())
                    detail = "no recognized error fields";
            } else {
                const auto start = r.body.find_first_not_of(" \r\n\t");
                format = start != r.body.npos && r.body[start] == '<' ? "html" : "text";
                detail = r.body;
            }
            detail = redact(std::move(detail), config);
        }
    } catch (...) {
        caught("format HTTP failure diagnostics");
        detail = "error detail unavailable";
    }
    diagnostic("%s: HTTP %d, %zu body bytes, format=%s, detail=%s", std::string(operation).c_str(),
               r.status, r.body.size(), format.c_str(), detail.c_str());
    if (!detail.empty())
        message += ": " + detail;
    return message;
}
} // namespace clarity
