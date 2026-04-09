#include <unicode/ubiditransform.h>
#include <unicode/ustring.h>
#include <unicode/ushape.h>
#include <unicode/utypes.h>

#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class ParseError : public std::runtime_error {
public:
    ParseError(size_t p, std::string msg) : std::runtime_error(std::move(msg)), pos(p) {}
    size_t pos;
};

struct SourceLoc {
    size_t line = 1;
    size_t column = 1;
    size_t line_begin = 0;
    size_t line_end = 0;
};

struct PrintfToken {
    char32_t placeholder = 0;
    std::string original;
    std::string utf8_placeholder;
};

static std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open input: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void write_file(const std::string& path, const std::string& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open output: " + path);
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!f) throw std::runtime_error("failed writing output: " + path);
}

[[noreturn]] static void fail_at(size_t pos, const std::string& msg) {
    throw ParseError(pos, msg);
}

static SourceLoc locate(const std::string& s, size_t pos) {
    if (pos > s.size()) pos = s.size();
    SourceLoc loc;
    size_t i = 0;
    size_t line = 1;
    size_t line_begin = 0;
    while (i < pos) {
        if (s[i] == '\n') {
            ++line;
            line_begin = i + 1;
        }
        ++i;
    }
    size_t line_end = pos;
    while (line_end < s.size() && s[line_end] != '\n') ++line_end;
    loc.line = line;
    loc.column = pos - line_begin + 1;
    loc.line_begin = line_begin;
    loc.line_end = line_end;
    return loc;
}

static std::string format_parse_error(const std::string& path, const std::string& src, const ParseError& e) {
    const SourceLoc loc = locate(src, e.pos);
    std::string line = src.substr(loc.line_begin, loc.line_end - loc.line_begin);

    std::string caret;
    caret.reserve(loc.column + 8);
    for (size_t i = 1; i < loc.column; ++i) {
        const char ch = line[i - 1];
        caret.push_back(ch == '\t' ? '\t' : ' ');
    }
    caret += '^';

    std::ostringstream out;
    out << path << ':' << loc.line << ':' << loc.column << ": " << e.what() << "\n";
    out << line << "\n";
    out << caret;
    return out.str();
}

static bool is_ident_char(unsigned char c) {
    return std::isalnum(c) || c == '_';
}

static size_t skip_ws(const std::string& s, size_t i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    return i;
}

static void append_utf8(std::string& out, char32_t cp) {
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        if (0xD800 <= cp && cp <= 0xDFFF) {
            throw std::runtime_error("invalid surrogate code point");
        }
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        throw std::runtime_error("code point out of range");
    }
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static char32_t parse_hex_escape(const std::string& s, size_t& i, int count) {
    char32_t cp = 0;
    for (int k = 0; k < count; ++k) {
        if (i >= s.size()) fail_at(i, "truncated hex escape");
        int v = hex_value(s[i++]);
        if (v < 0) fail_at(i - 1, "invalid hex escape");
        cp = (cp << 4) | static_cast<char32_t>(v);
    }
    return cp;
}

struct LiteralStart {
    bool ok = false;
    bool raw = false;
    size_t prefix_len = 0;
};

static LiteralStart detect_string_literal_start(const std::string& s, size_t i) {
    LiteralStart st;
    if (i >= s.size()) return st;

    if (s.compare(i, 3, "u8R") == 0 && i + 3 < s.size() && s[i + 3] == '"') {
        st.ok = true; st.raw = true; st.prefix_len = 3; return st;
    }
    if (s.compare(i, 1, "R") == 0 && i + 1 < s.size() && s[i + 1] == '"') {
        st.ok = true; st.raw = true; st.prefix_len = 1; return st;
    }
    if (s.compare(i, 2, "u8") == 0 && i + 2 < s.size() && s[i + 2] == '"') {
        st.ok = true; st.raw = false; st.prefix_len = 2; return st;
    }
    if (s[i] == '"') {
        st.ok = true; st.raw = false; st.prefix_len = 0; return st;
    }
    return st;
}

static std::string parse_raw_string_literal(const std::string& s, size_t& i, size_t prefix_len) {
    const size_t start = i;
    size_t p = i + prefix_len;
    if (p >= s.size() || s[p] != '"') fail_at(start, "expected raw string literal");
    ++p;

    const size_t delim_begin = p;
    while (p < s.size() && s[p] != '(') {
        if (s[p] == '\\' || s[p] == ' ' || s[p] == '\t' || s[p] == '\n' || s[p] == '\r') {
            fail_at(p, "invalid raw string delimiter");
        }
        ++p;
    }
    if (p >= s.size()) fail_at(start, "unterminated raw string delimiter");

    const std::string delim = s.substr(delim_begin, p - delim_begin);
    ++p;
    const size_t body_begin = p;

    std::string closing = ")" + delim + '"';
    const size_t close = s.find(closing, p);
    if (close == std::string::npos) fail_at(start, "unterminated raw string literal");

    i = close + closing.size();
    return s.substr(body_begin, close - body_begin);
}

static std::string parse_escaped_string_literal(const std::string& s, size_t& i, size_t prefix_len) {
    const size_t start = i;
    size_t p = i + prefix_len;
    if (p >= s.size() || s[p] != '"') fail_at(start, "expected string literal");
    ++p;

    std::string out;
    while (p < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[p++]);
        if (c == '"') {
            i = p;
            return out;
        }
        if (c != '\\') {
            out.push_back(static_cast<char>(c));
            continue;
        }

        if (p >= s.size()) fail_at(p - 1, "truncated escape sequence");
        char e = s[p++];

        switch (e) {
            case '\\': out.push_back('\\'); break;
            case '"':  out.push_back('"');  break;
            case '\'': out.push_back('\''); break;
            case 'n':  out.push_back('\n'); break;
            case 'r':  out.push_back('\r'); break;
            case 't':  out.push_back('\t'); break;
            case 'b':  out.push_back('\b'); break;
            case 'f':  out.push_back('\f'); break;
            case 'v':  out.push_back('\v'); break;
            case 'a':  out.push_back('\a'); break;
            case '?':  out.push_back('?');  break;

            case 'x': {
                if (p >= s.size() || hex_value(s[p]) < 0) {
                    fail_at(p, "expected hex digits after \\x");
                }
                unsigned value = 0;
                while (p < s.size()) {
                    int v = hex_value(s[p]);
                    if (v < 0) break;
                    value = (value << 4) | static_cast<unsigned>(v);
                    ++p;
                }
                if (value <= 0xFF) out.push_back(static_cast<char>(value));
                else append_utf8(out, static_cast<char32_t>(value));
                break;
            }

            case 'u': {
                char32_t cp = parse_hex_escape(s, p, 4);
                append_utf8(out, cp);
                break;
            }

            case 'U': {
                char32_t cp = parse_hex_escape(s, p, 8);
                append_utf8(out, cp);
                break;
            }

            default:
                if (e >= '0' && e <= '7') {
                    unsigned value = static_cast<unsigned>(e - '0');
                    for (int n = 0; n < 2; ++n) {
                        if (p < s.size() && s[p] >= '0' && s[p] <= '7') {
                            value = (value << 3) | static_cast<unsigned>(s[p] - '0');
                            ++p;
                        } else {
                            break;
                        }
                    }
                    out.push_back(static_cast<char>(value & 0xFF));
                } else {
                    fail_at(p - 1, std::string("unsupported escape: \\") + e);
                }
        }
    }

    fail_at(start, "unterminated string literal");
}

static std::string parse_cpp_string_literal(const std::string& s, size_t& i) {
    const LiteralStart st = detect_string_literal_start(s, i);
    if (!st.ok) fail_at(i, "expected string literal");
    if (st.raw) return parse_raw_string_literal(s, i, st.prefix_len);
    return parse_escaped_string_literal(s, i, st.prefix_len);
}

static bool looks_like_string_literal_start(const std::string& s, size_t i) {
    return detect_string_literal_start(s, i).ok;
}

static std::string parse_string_literal_sequence(const std::string& s, size_t& i, const char* macro_name) {
    std::string out;
    bool any = false;
    for (;;) {
        i = skip_ws(s, i);
        if (!looks_like_string_literal_start(s, i)) break;
        out += parse_cpp_string_literal(s, i);
        any = true;
    }
    if (!any) fail_at(i, std::string(macro_name) + "() expects one or more string literals");
    return out;
}

static std::string to_hex_literal(const std::string& utf8) {
    static const char* hex = "0123456789ABCDEF";
    std::string out = "\"";
    for (unsigned char b : utf8) {
        out += "\\x";
        out += hex[(b >> 4) & 0xF];
        out += hex[b & 0xF];
    }
    out += '"';
    return out;
}

static std::string logical_to_visual_legacy_arabic(const std::string& utf8) {
    UErrorCode err = U_ZERO_ERROR;

    int32_t src16_len = 0;
    u_strFromUTF8(nullptr, 0, &src16_len,
                  utf8.data(), static_cast<int32_t>(utf8.size()), &err);
    if (err != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(err)) {
        throw std::runtime_error("u_strFromUTF8 length probe failed");
    }

    err = U_ZERO_ERROR;
    std::vector<UChar> src16(static_cast<size_t>(src16_len) + 1);
    u_strFromUTF8(src16.data(), static_cast<int32_t>(src16.size()), nullptr,
                  utf8.data(), static_cast<int32_t>(utf8.size()), &err);
    if (U_FAILURE(err)) {
        throw std::runtime_error("u_strFromUTF8 failed");
    }

    std::vector<UChar> dst16(src16.size() * 4 + 32);

    err = U_ZERO_ERROR;
    UBiDiTransform* tr = ubiditransform_open(&err);
    if (U_FAILURE(err) || !tr) {
        throw std::runtime_error("ubiditransform_open failed");
    }

    int32_t out16_len = ubiditransform_transform(
        tr,
        src16.data(), src16_len,
        dst16.data(), static_cast<int32_t>(dst16.size()),
        UBIDI_DEFAULT_RTL, UBIDI_LOGICAL,
        UBIDI_LTR,         UBIDI_VISUAL,
        UBIDI_MIRRORING_ON,
        U_SHAPE_LETTERS_SHAPE | U_SHAPE_LAMALEF_RESIZE,
        &err
    );

    ubiditransform_close(tr);

    if (U_FAILURE(err)) {
        throw std::runtime_error("ubiditransform_transform failed");
    }

    int32_t out8_len = 0;
    err = U_ZERO_ERROR;
    u_strToUTF8(nullptr, 0, &out8_len, dst16.data(), out16_len, &err);
    if (err != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(err)) {
        throw std::runtime_error("u_strToUTF8 length probe failed");
    }

    std::string out(static_cast<size_t>(out8_len), '\0');
    err = U_ZERO_ERROR;
    u_strToUTF8(out.data(), out8_len, nullptr, dst16.data(), out16_len, &err);
    if (U_FAILURE(err)) {
        throw std::runtime_error("u_strToUTF8 failed");
    }

    return out;
}

static bool is_ascii_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_printf_conv_char(char c) {
    switch (c) {
        case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
        case 'f': case 'F': case 'e': case 'E': case 'g': case 'G':
        case 'a': case 'A': case 'c': case 'C': case 's': case 'S':
        case 'p': case 'n': case 'm':
            return true;
        default:
            return false;
    }
}

static bool parse_printf_token_at(const std::string& s, size_t start, size_t& end) {
    if (start >= s.size() || s[start] != '%') return false;
    size_t p = start + 1;
    if (p >= s.size()) return false;

    if (s[p] == '%') {
        end = p + 1;
        return true;
    }

    // Optional positional parameter: n$
    size_t save = p;
    while (p < s.size() && is_ascii_digit(s[p])) ++p;
    if (!(p > save && p < s.size() && s[p] == '$')) {
        p = save;
    } else {
        ++p;
    }

    while (p < s.size()) {
        char c = s[p];
        if (c == '-' || c == '+' || c == ' ' || c == '#' || c == '0' || c == '\'') {
            ++p;
        } else {
            break;
        }
    }

    if (p < s.size() && s[p] == '*') {
        ++p;
        size_t d = p;
        while (p < s.size() && is_ascii_digit(s[p])) ++p;
        if (p > d && p < s.size() && s[p] == '$') ++p;
    } else {
        while (p < s.size() && is_ascii_digit(s[p])) ++p;
    }

    if (p < s.size() && s[p] == '.') {
        ++p;
        if (p < s.size() && s[p] == '*') {
            ++p;
            size_t d = p;
            while (p < s.size() && is_ascii_digit(s[p])) ++p;
            if (p > d && p < s.size() && s[p] == '$') ++p;
        } else {
            while (p < s.size() && is_ascii_digit(s[p])) ++p;
        }
    }

    if (p + 1 < s.size()) {
        if ((s[p] == 'h' && s[p + 1] == 'h') || (s[p] == 'l' && s[p + 1] == 'l')) {
            p += 2;
        }
    }
    if (p < s.size()) {
        if (s[p] == 'h' || s[p] == 'l' || s[p] == 'j' || s[p] == 'z' || s[p] == 't' || s[p] == 'L' || s[p] == 'q') {
            ++p;
        }
    }

    if (p < s.size() && is_printf_conv_char(s[p])) {
        end = p + 1;
        return true;
    }
    return false;
}

static std::string protect_printf_tokens(const std::string& s, std::vector<PrintfToken>& tokens, bool& found_any) {
    std::string masked;
    found_any = false;
    masked.reserve(s.size());

    for (size_t i = 0; i < s.size();) {
        if (s[i] == '%') {
            size_t end = i;
            if (parse_printf_token_at(s, i, end)) {
                found_any = true;
                const size_t idx = tokens.size();
                if (idx >= 0x1900) {
                    throw std::runtime_error("too many printf format tokens in one string");
                }
                PrintfToken tok;
                tok.placeholder = static_cast<char32_t>(0xE000 + idx);
                tok.original = s.substr(i, end - i);
                append_utf8(tok.utf8_placeholder, tok.placeholder);
                masked += tok.utf8_placeholder;
                tokens.push_back(std::move(tok));
                i = end;
                continue;
            }
        }
        masked.push_back(s[i]);
        ++i;
    }

    return masked;
}

static std::string restore_printf_tokens(const std::string& s, const std::vector<PrintfToken>& tokens) {
    std::string out;
    out.reserve(s.size() + tokens.size() * 4);

    for (size_t i = 0; i < s.size();) {
        bool matched = false;
        for (const auto& tok : tokens) {
            if (!tok.utf8_placeholder.empty() && s.compare(i, tok.utf8_placeholder.size(), tok.utf8_placeholder) == 0) {
                out += tok.original;
                i += tok.utf8_placeholder.size();
                matched = true;
                break;
            }
        }
        if (!matched) {
            out.push_back(s[i]);
            ++i;
        }
    }

    return out;
}

static std::string logical_to_visual_preserving_printf(const std::string& utf8, bool& found_any) {
    std::vector<PrintfToken> tokens;
    std::string masked = protect_printf_tokens(utf8, tokens, found_any);
    std::string visual = logical_to_visual_legacy_arabic(masked);
    return restore_printf_tokens(visual, tokens);
}

static bool parse_macro_call(const std::string& src, size_t start, size_t& end, std::string& replacement) {
    enum class MacroKind { None, RTL, RTLFMT };
    MacroKind kind = MacroKind::None;
    const char* macro_name = nullptr;
    size_t macro_len = 0;

    if (src.compare(start, 6, "RTLFMT") == 0) {
        kind = MacroKind::RTLFMT;
        macro_name = "RTLFMT";
        macro_len = 6;
    } else if (src.compare(start, 3, "RTL") == 0) {
        kind = MacroKind::RTL;
        macro_name = "RTL";
        macro_len = 3;
    } else {
        return false;
    }

    if (start > 0 && is_ident_char(static_cast<unsigned char>(src[start - 1]))) return false;
    if (start + macro_len < src.size() && is_ident_char(static_cast<unsigned char>(src[start + macro_len]))) return false;

    size_t i = start + macro_len;
    i = skip_ws(src, i);
    if (i >= src.size() || src[i] != '(') return false;
    ++i;

    i = skip_ws(src, i);
    std::string logical = parse_string_literal_sequence(src, i, macro_name);
    i = skip_ws(src, i);

    if (i >= src.size() || src[i] != ')') {
        fail_at(i, std::string(macro_name) + "() missing closing ')'");
    }
    ++i;

    try {
        bool found_printf = false;
        std::string visual;
        if (kind == MacroKind::RTLFMT) {
            visual = logical_to_visual_preserving_printf(logical, found_printf);
        } else {
            visual = logical_to_visual_preserving_printf(logical, found_printf);
            if (!found_printf) {
                visual = logical_to_visual_legacy_arabic(logical);
            }
        }
        replacement = to_hex_literal(visual);
    } catch (const std::exception& e) {
        fail_at(start, std::string(macro_name) + "() failed: " + e.what());
    }

    end = i;
    return true;
}

static size_t skip_string_literal_token(const std::string& src, size_t i) {
    const size_t start = i;
    const LiteralStart st = detect_string_literal_start(src, i);
    if (!st.ok) return i;
    try {
        (void)parse_cpp_string_literal(src, i);
        return i;
    } catch (const ParseError&) {
        throw;
    } catch (...) {
        fail_at(start, "failed to skip string literal");
    }
}

static size_t skip_char_literal_token(const std::string& src, size_t i) {
    if (i >= src.size() || src[i] != '\'') return i;
    const size_t start = i;
    ++i;
    while (i < src.size()) {
        if (src[i] == '\\' && i + 1 < src.size()) {
            i += 2;
        } else if (src[i] == '\'') {
            return i + 1;
        } else {
            ++i;
        }
    }
    fail_at(start, "unterminated character literal");
}

static std::string transform_source(const std::string& src) {
    std::string out;
    out.reserve(src.size() + src.size() / 4);

    for (size_t i = 0; i < src.size();) {
        if (i + 1 < src.size() && src[i] == '/' && src[i + 1] == '/') {
            size_t j = i + 2;
            while (j < src.size() && src[j] != '\n') ++j;
            out.append(src, i, j - i);
            i = j;
            continue;
        }

        if (i + 1 < src.size() && src[i] == '/' && src[i + 1] == '*') {
            size_t j = i + 2;
            while (j + 1 < src.size() && !(src[j] == '*' && src[j + 1] == '/')) ++j;
            if (j + 1 >= src.size()) fail_at(i, "unterminated block comment");
            j += 2;
            out.append(src, i, j - i);
            i = j;
            continue;
        }

        if (src[i] == '\'') {
            size_t j = skip_char_literal_token(src, i);
            out.append(src, i, j - i);
            i = j;
            continue;
        }

        if (looks_like_string_literal_start(src, i)) {
            size_t j = skip_string_literal_token(src, i);
            out.append(src, i, j - i);
            i = j;
            continue;
        }

        if (src.compare(i, 6, "RTLFMT") == 0 || src.compare(i, 3, "RTL") == 0) {
            size_t end = i;
            std::string repl;
            if (parse_macro_call(src, i, end, repl)) {
                out += repl;
                i = end;
                continue;
            }
        }

        out.push_back(src[i]);
        ++i;
    }

    return out;
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "usage: rtlpp <input.in.cpp> <output.cpp>\n";
            return 2;
        }

        const std::string input_path = argv[1];
        const std::string output_path = argv[2];

        const std::string input = read_file(input_path);
        std::string output;
        output += "// GENERATED FILE - DO NOT EDIT\n";
        output += "// Source: " + input_path + "\n\n";
        output += transform_source(input);

        write_file(output_path, output);
        return 0;
    } catch (const ParseError& e) {
        try {
            std::cerr << "rtlpp: " << format_parse_error(argv[1], read_file(argv[1]), e) << "\n";
        } catch (...) {
            std::cerr << "rtlpp: parse error: " << e.what() << "\n";
        }
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "rtlpp: " << e.what() << "\n";
        return 1;
    }
}
