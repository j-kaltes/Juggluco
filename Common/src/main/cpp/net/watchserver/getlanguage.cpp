#include <string_view>
#include <unordered_map>
#include <cctype>
#include <cstdint>

struct jugglucotext; 

static inline unsigned char lower_ascii(unsigned char c) {
    return static_cast<unsigned char>(std::tolower(c));
}

static inline uint16_t make_lang_key_lower(char a, char b) {
    return static_cast<uint16_t>(lower_ascii(static_cast<unsigned char>(a))) |
           (static_cast<uint16_t>(lower_ascii(static_cast<unsigned char>(b))) << 8);
}

static inline std::string_view trim_sv(std::string_view s) {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
        ++b;

    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
        --e;

    return s.substr(b, e - b);
}

// Parses q-values like 1, 1.0, 0.8, 0.75, 0.123
// Returns 0..1000. Invalid q defaults to 1000.
static inline int parse_qvalue(std::string_view s) {
    s = trim_sv(s);
    if (s.empty())
        return 1000;

    if (s[0] == '1') {
        if (s.size() == 1)
            return 1000;
        if (s[1] != '.')
            return 1000;
        for (size_t i = 2; i < s.size(); ++i) {
            if (s[i] != '0')
                return 1000;
        }
        return 1000;
    }

    if (s[0] == '0') {
        if (s.size() == 1)
            return 0;
        if (s[1] != '.')
            return 1000;

        int value = 0;
        int scale = 100;
        for (size_t i = 2; i < s.size() && scale > 0; ++i) {
            unsigned char ch = static_cast<unsigned char>(s[i]);
            if (!std::isdigit(ch))
                break;
            value += (ch - '0') * scale;
            scale /= 10;
        }
        return value;
    }

    return 1000;
}

static inline int extract_qvalue(std::string_view item) {
    size_t pos = 0;
    while ((pos = item.find(';', pos)) != std::string_view::npos) {
        ++pos;
        while (pos < item.size() && std::isspace(static_cast<unsigned char>(item[pos])))
            ++pos;

        if (pos >= item.size())
            break;

        if (item[pos] == 'q' || item[pos] == 'Q') {
            ++pos;
            while (pos < item.size() && std::isspace(static_cast<unsigned char>(item[pos])))
                ++pos;

            if (pos < item.size() && item[pos] == '=') {
                ++pos;
                return parse_qvalue(item.substr(pos));
            }
        }

        while (pos < item.size() && item[pos] != ';')
            ++pos;
    }
    return 1000; // default q=1.0
}

//Default to 0
uint16_t choose_language( std::string_view accept_language, const std::unordered_map<uint16_t, const jugglucotext*>& langmap) {


    if (langmap.empty())
        return 0;

    uint16_t best_key = 0;
    int best_q = -1;

    size_t pos = 0;
    while (pos < accept_language.size()) {
        size_t comma = accept_language.find(',', pos);
        std::string_view item = (comma == std::string_view::npos)
            ? accept_language.substr(pos)
            : accept_language.substr(pos, comma - pos);

        item = trim_sv(item);

        if (!item.empty() && item[0] != '*') {
            size_t i = 0;
            while (i < item.size() && std::isalpha(static_cast<unsigned char>(item[i])))
                ++i;

            const size_t primary_len = i;
            if (primary_len == 2) {
                uint16_t key = make_lang_key_lower(item[0], item[1]);
                int q = extract_qvalue(item);

                if (q > 0 && langmap.find(key) != langmap.end()) {
                    if (q > best_q) {
                        best_q = q;
                        best_key = key;
                    }
                }
            }
        }

        if (comma == std::string_view::npos)
            break;
        pos = comma + 1;
    }

  return best_key;

}
