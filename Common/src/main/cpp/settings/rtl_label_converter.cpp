#include "rtl_label_converter.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace {

struct ShapeEntry {
    uint32_t base;
    uint32_t isolated;
    uint32_t final_form;
    uint32_t initial;
    uint32_t medial;
    bool joins_prev;
    bool joins_next;
};

static const ShapeEntry kShapeTable[] = {
    {0x0621, 0xFE80, 0,      0,      0,      false, false},
    {0x0622, 0xFE81, 0xFE82, 0,      0,      true,  false},
    {0x0623, 0xFE83, 0xFE84, 0,      0,      true,  false},
    {0x0624, 0xFE85, 0xFE86, 0,      0,      true,  false},
    {0x0625, 0xFE87, 0xFE88, 0,      0,      true,  false},
    {0x0626, 0xFE89, 0xFE8A, 0xFE8B, 0xFE8C, true,  true },
    {0x0627, 0xFE8D, 0xFE8E, 0,      0,      true,  false},
    {0x0628, 0xFE8F, 0xFE90, 0xFE91, 0xFE92, true,  true },
    {0x0629, 0xFE93, 0xFE94, 0,      0,      true,  false},
    {0x062A, 0xFE95, 0xFE96, 0xFE97, 0xFE98, true,  true },
    {0x062B, 0xFE99, 0xFE9A, 0xFE9B, 0xFE9C, true,  true },
    {0x062C, 0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0, true,  true },
    {0x062D, 0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4, true,  true },
    {0x062E, 0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8, true,  true },
    {0x062F, 0xFEA9, 0xFEAA, 0,      0,      true,  false},
    {0x0630, 0xFEAB, 0xFEAC, 0,      0,      true,  false},
    {0x0631, 0xFEAD, 0xFEAE, 0,      0,      true,  false},
    {0x0632, 0xFEAF, 0xFEB0, 0,      0,      true,  false},
    {0x0633, 0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4, true,  true },
    {0x0634, 0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8, true,  true },
    {0x0635, 0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC, true,  true },
    {0x0636, 0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0, true,  true },
    {0x0637, 0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4, true,  true },
    {0x0638, 0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8, true,  true },
    {0x0639, 0xFEC9, 0xFECA, 0xFECB, 0xFECC, true,  true },
    {0x063A, 0xFECD, 0xFECE, 0xFECF, 0xFED0, true,  true },
    {0x0640, 0x0640, 0x0640, 0x0640, 0x0640, true,  true },
    {0x0641, 0xFED1, 0xFED2, 0xFED3, 0xFED4, true,  true },
    {0x0642, 0xFED5, 0xFED6, 0xFED7, 0xFED8, true,  true },
    {0x0643, 0xFED9, 0xFEDA, 0xFEDB, 0xFEDC, true,  true },
    {0x0644, 0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0, true,  true },
    {0x0645, 0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4, true,  true },
    {0x0646, 0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8, true,  true },
    {0x0647, 0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC, true,  true },
    {0x0648, 0xFEED, 0xFEEE, 0,      0,      true,  false},
    {0x0649, 0xFEEF, 0xFEF0, 0,      0,      true,  false},
    {0x064A, 0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4, true,  true },

    {0x067E, 0xFB56, 0xFB57, 0xFB58, 0xFB59, true,  true },
    {0x0686, 0xFB7A, 0xFB7B, 0xFB7C, 0xFB7D, true,  true },
    {0x0688, 0xFB88, 0xFB89, 0,      0,      true,  false},
    {0x0691, 0xFB8C, 0xFB8D, 0,      0,      true,  false},
    {0x0698, 0xFB8A, 0xFB8B, 0,      0,      true,  false},
    {0x06A9, 0xFB8E, 0xFB8F, 0xFB90, 0xFB91, true,  true },
    {0x06AF, 0xFB92, 0xFB93, 0xFB94, 0xFB95, true,  true },
    {0x06BA, 0xFB9E, 0xFB9F, 0,      0,      true,  false},
    {0x06BE, 0xFBAA, 0xFBAB, 0xFBAC, 0xFBAD, true,  true },
    {0x06C1, 0xFBA6, 0xFBA7, 0xFBA8, 0xFBA9, true,  true },
    {0x06CC, 0xFBFC, 0xFBFD, 0xFBFE, 0xFBFF, true,  true },
};

struct LamAlefLigature {
    uint32_t alef;
    uint32_t isolated;
    uint32_t final_form;
};

static const LamAlefLigature kLamAlef[] = {
    {0x0622, 0xFEF5, 0xFEF6},
    {0x0623, 0xFEF7, 0xFEF8},
    {0x0625, 0xFEF9, 0xFEFA},
    {0x0627, 0xFEFB, 0xFEFC},
};

const ShapeEntry* find_shape_entry(uint32_t cp) {
    for (const auto& e : kShapeTable) {
        if (e.base == cp) return &e;
    }
    return nullptr;
}

bool is_transparent_mark(uint32_t cp) {
    return (cp >= 0x064B && cp <= 0x065F) || cp == 0x0670 ||
           (cp >= 0x06D6 && cp <= 0x06ED);
}

bool is_rtl_codepoint(uint32_t cp) {
    return (cp >= 0x0590 && cp <= 0x08FF) ||
           (cp >= 0xFB1D && cp <= 0xFDFF) ||
           (cp >= 0xFE70 && cp <= 0xFEFF) ||
           (cp >= 0x10800 && cp <= 0x10FFF) ||
           (cp >= 0x1E800 && cp <= 0x1EEFF);
}

bool is_ascii_ltrish(uint32_t cp) {
    if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) return true;
    if (cp >= '0' && cp <= '9') return true;
    switch (cp) {
        case '!': case '"': case '#': case '$': case '%': case '&':
        case '\'': case '*': case '+': case ',': case '-': case '.':
        case '/': case ':': case ';': case '<': case '=': case '>':
        case '?': case '@': case '^': case '_': case '`': case '|':
        case '~':
            return true;
        default:
            return false;
    }
}

bool is_digit_like(uint32_t cp) {
    return (cp >= '0' && cp <= '9') ||
           (cp >= 0x0660 && cp <= 0x0669) ||
           (cp >= 0x06F0 && cp <= 0x06F9);
}

bool is_ltrish_run_char(uint32_t cp) {
    if (is_ascii_ltrish(cp) || is_digit_like(cp)) return true;
    if (cp >= 0x00C0 && cp <= 0x02AF) return true;
    return false;
}

uint32_t mirror_char(uint32_t cp) {
    switch (cp) {
        case '(': return ')';
        case ')': return '(';
        case '[': return ']';
        case ']': return '[';
        case '{': return '}';
        case '}': return '{';
        case '<': return '>';
        case '>': return '<';
        case 0x00AB: return 0x00BB;
        case 0x00BB: return 0x00AB;
        case 0x2039: return 0x203A;
        case 0x203A: return 0x2039;
        default: return cp;
    }
}

bool decode_utf8(const char* s, std::vector<uint32_t>& out) {
    out.clear();
    if (!s) return false;

    const unsigned char* p = reinterpret_cast<const unsigned char*>(s);
    while (*p) {
        uint32_t cp = 0;

        if (*p < 0x80) {
            cp = *p++;
        } else if ((*p & 0xE0) == 0xC0) {
            if ((p[1] & 0xC0) != 0x80) return false;
            cp = ((*p & 0x1F) << 6) | (p[1] & 0x3F);
            p += 2;
            if (cp < 0x80) return false;
        } else if ((*p & 0xF0) == 0xE0) {
            if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) return false;
            cp = ((*p & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            p += 3;
            if (cp < 0x800) return false;
        } else if ((*p & 0xF8) == 0xF0) {
            if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 || (p[3] & 0xC0) != 0x80) {
                return false;
            }
            cp = ((*p & 0x07) << 18) |
                 ((p[1] & 0x3F) << 12) |
                 ((p[2] & 0x3F) << 6) |
                 (p[3] & 0x3F);
            p += 4;
            if (cp < 0x10000 || cp > 0x10FFFF) return false;
        } else {
            return false;
        }

        out.push_back(cp);
    }

    return true;
}

size_t encoded_utf8_bytes(const std::vector<uint32_t>& cps) {
    size_t bytes = 0;
    for (uint32_t cp : cps) {
        if (cp <= 0x7F) bytes += 1;
        else if (cp <= 0x7FF) bytes += 2;
        else if (cp <= 0xFFFF) bytes += 3;
        else bytes += 4;
    }
    return bytes;
}

bool encode_utf8_to_buffer(const std::vector<uint32_t>& cps, char* output, size_t output_capacity) {
    const size_t needed = encoded_utf8_bytes(cps);

    if (!output || output_capacity == 0) {
        return false;
    }
    if (output_capacity <= needed) {
        output[0] = '\0';
        return false;
    }

    char* w = output;
    for (uint32_t cp : cps) {
        if (cp <= 0x7F) {
            *w++ = static_cast<char>(cp);
        } else if (cp <= 0x7FF) {
            *w++ = static_cast<char>(0xC0 | (cp >> 6));
            *w++ = static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            *w++ = static_cast<char>(0xE0 | (cp >> 12));
            *w++ = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            *w++ = static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            *w++ = static_cast<char>(0xF0 | (cp >> 18));
            *w++ = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            *w++ = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            *w++ = static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    *w = '\0';
    return true;
}

int32_t copy_input_if_possible(const char* input, char* output, size_t output_capacity) {
    if (!input) {
        if (output && output_capacity > 0) output[0] = '\0';
        return 0;
    }

    const size_t n = std::strlen(input);
    if (output && output_capacity > n) {
        std::memcpy(output, input, n + 1);
    } else if (output && output_capacity > 0) {
        output[0] = '\0';
    }
    return static_cast<int32_t>(n);
}

int find_prev_joinable(const std::vector<uint32_t>& cps, int i) {
    for (int j = i - 1; j >= 0; --j) {
        if (cps[j] == '\n' || cps[j] == '\r') break;
        if (!is_transparent_mark(cps[j])) return j;
    }
    return -1;
}

int find_next_joinable(const std::vector<uint32_t>& cps, int i) {
    for (int j = i + 1; j < static_cast<int>(cps.size()); ++j) {
        if (cps[j] == '\n' || cps[j] == '\r') break;
        if (!is_transparent_mark(cps[j])) return j;
    }
    return -1;
}

bool connects(const ShapeEntry* left, const ShapeEntry* right) {
    return left && right && left->joins_next && right->joins_prev;
}

void shape_arabic(const std::vector<uint32_t>& in, std::vector<uint32_t>& out) {
    out.clear();

    for (int i = 0; i < static_cast<int>(in.size()); ++i) {
        uint32_t cp = in[i];

        if (cp == '\n' || cp == '\r') {
            out.push_back(cp);
            continue;
        }

        if (is_transparent_mark(cp)) {
            out.push_back(cp);
            continue;
        }

        if (cp == 0x0644 && i + 1 < static_cast<int>(in.size())) {
            uint32_t next = in[i + 1];
            for (const auto& lig : kLamAlef) {
                if (next == lig.alef) {
                    int prev_i = find_prev_joinable(in, i);
                    const ShapeEntry* prev_e = (prev_i >= 0) ? find_shape_entry(in[prev_i]) : nullptr;
                    const ShapeEntry* lam_e  = find_shape_entry(cp);
                    const bool join_prev = connects(prev_e, lam_e);

                    out.push_back(join_prev ? lig.final_form : lig.isolated);
                    ++i;
                    goto next_char;
                }
            }
        }

        {
            const ShapeEntry* e = find_shape_entry(cp);
            if (!e) {
                out.push_back(cp);
                continue;
            }

            const int prev_i = find_prev_joinable(in, i);
            const int next_i = find_next_joinable(in, i);

            const ShapeEntry* prev_e = (prev_i >= 0) ? find_shape_entry(in[prev_i]) : nullptr;
            const ShapeEntry* next_e = (next_i >= 0) ? find_shape_entry(in[next_i]) : nullptr;

            const bool join_prev = connects(prev_e, e);
            const bool join_next = connects(e, next_e);

            uint32_t shaped = e->isolated;
            if (join_prev && join_next && e->medial) shaped = e->medial;
            else if (join_prev && e->final_form)     shaped = e->final_form;
            else if (join_next && e->initial)        shaped = e->initial;
            else                                     shaped = e->isolated;

            out.push_back(shaped);
        }

    next_char:
        ;
    }
}

std::unordered_map<uint32_t, std::vector<uint32_t>> make_unshape_map() {
    std::unordered_map<uint32_t, std::vector<uint32_t>> m;

    for (const auto& e : kShapeTable) {
        m[e.isolated] = {e.base};
        if (e.final_form) m[e.final_form] = {e.base};
        if (e.initial)    m[e.initial]    = {e.base};
        if (e.medial)     m[e.medial]     = {e.base};
    }

    m[0xFEF5] = {0x0644, 0x0622};
    m[0xFEF6] = {0x0644, 0x0622};
    m[0xFEF7] = {0x0644, 0x0623};
    m[0xFEF8] = {0x0644, 0x0623};
    m[0xFEF9] = {0x0644, 0x0625};
    m[0xFEFA] = {0x0644, 0x0625};
    m[0xFEFB] = {0x0644, 0x0627};
    m[0xFEFC] = {0x0644, 0x0627};

    return m;
}

void unshape_arabic(const std::vector<uint32_t>& in, std::vector<uint32_t>& out) {
    static const auto unshape_map = make_unshape_map();

    out.clear();
    for (uint32_t cp : in) {
        auto it = unshape_map.find(cp);
        if (it == unshape_map.end()) {
            out.push_back(cp);
        } else {
            out.insert(out.end(), it->second.begin(), it->second.end());
        }
    }
}

void reverse_range(std::vector<uint32_t>& cps, size_t begin, size_t end) {
    while (begin < end) {
        std::swap(cps[begin], cps[end]);
        ++begin;
        --end;
    }
}

void reorder_line_visual(std::vector<uint32_t>& line, bool mirror) {
    std::reverse(line.begin(), line.end());

    if (mirror) {
        for (auto& cp : line) {
            cp = mirror_char(cp);
        }
    }

    for (size_t i = 0; i < line.size();) {
        if (!is_ltrish_run_char(line[i])) {
            ++i;
            continue;
        }

        size_t j = i + 1;
        while (j < line.size() && is_ltrish_run_char(line[j])) {
            ++j;
        }

        reverse_range(line, i, j - 1);
        i = j;
    }
}

void process_lines(const std::vector<uint32_t>& input,
                   std::vector<uint32_t>& output,
                   bool to_visual) {
    output.clear();

    std::vector<uint32_t> line;
    std::vector<uint32_t> temp;

    auto flush_line = [&]() {
        if (to_visual) {
            shape_arabic(line, temp);
        } else {
            temp = line;
        }

        reorder_line_visual(temp, true);

        if (!to_visual) {
            std::vector<uint32_t> unshaped;
            unshape_arabic(temp, unshaped);
            output.insert(output.end(), unshaped.begin(), unshaped.end());
        } else {
            output.insert(output.end(), temp.begin(), temp.end());
        }

        line.clear();
    };

    for (size_t i = 0; i < input.size(); ++i) {
        const uint32_t cp = input[i];

        if (cp == '\r') {
            flush_line();
            output.push_back(cp);
            if (i + 1 < input.size() && input[i + 1] == '\n') {
                output.push_back(input[++i]);
            }
        } else if (cp == '\n') {
            flush_line();
            output.push_back(cp);
        } else {
            line.push_back(cp);
        }
    }

    flush_line();
}

int32_t transform_utf8(const char* input,
                       char* output,
                       size_t output_capacity,
                       bool to_visual) {
    if (!input) {
        if (output && output_capacity > 0) output[0] = '\0';
        return 0;
    }

    if (!rtl_is_rtl_utf8(input)) {
        return copy_input_if_possible(input, output, output_capacity);
    }

    std::vector<uint32_t> in;
    if (!decode_utf8(input, in)) {
        return copy_input_if_possible(input, output, output_capacity);
    }

    std::vector<uint32_t> out;
    process_lines(in, out, to_visual);

    const size_t needed = encoded_utf8_bytes(out);
    encode_utf8_to_buffer(out, output, output_capacity);
    return static_cast<int32_t>(needed);
}

} // namespace

extern "C" int rtl_is_rtl_utf8(const char* utf8) {
    std::vector<uint32_t> cps;
    if (!decode_utf8(utf8, cps)) return 0;

    for (uint32_t cp : cps) {
        if (is_rtl_codepoint(cp)) return 1;
    }
    return 0;
}

extern "C" int32_t rtl_to_visual_utf8(
    const char* logical_utf8,
    char* output,
    size_t output_capacity) {
    return transform_utf8(logical_utf8, output, output_capacity, true);
}

extern "C" int32_t rtl_to_logical_utf8(
    const char* visual_utf8,
    char* output,
    size_t output_capacity) {
    return transform_utf8(visual_utf8, output, output_capacity, false);
}

extern "C" int32_t rtl_visual_form_utf8(
    const char* user_input_utf8,
    char* output,
    size_t output_capacity) {
    return rtl_to_visual_utf8(user_input_utf8, output, output_capacity);
}

extern "C" int32_t rtl_logical_form_utf8(
    const char* stored_label_utf8,
    char* output,
    size_t output_capacity) {
    return rtl_to_logical_utf8(stored_label_utf8, output, output_capacity);
}

#ifdef TESTRTL
#include <stdio.h>
int main() {
    char input[]{ "12 دم"};
    constexpr const int maxbuf=12;
    char output[maxbuf];
    int lenvis=rtl_to_visual_utf8(input,output,maxbuf);
    char back[maxbuf];
    int  lenlog=rtl_to_logical_utf8(output,back,lenvis);
    puts(back);
    }
#endif
