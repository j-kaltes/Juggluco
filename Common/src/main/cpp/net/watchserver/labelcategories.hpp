// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cmath>
#include <locale>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

namespace juggluco::web {
struct LabelCategory {
    int id;
    float weight;
};
struct AmountLabel {
    std::string_view name;
    LabelCategory nightscout, libreview;
};

inline const char* label_category_name(int id) {
    switch (id) {
        case 0: return "unassigned";
        case 1: return "rapid_insulin";
        case 2: return "long_acting_insulin";
        case 3: return "carbohydrate";
        case 4: return "note";
        case 5: return "do_not_send";
        default: return "unknown";
    }
}
inline void write_label_json_string(std::ostream& out, std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    out << '"';
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') out << '\\' << char(c);
        else if (c < 32) out << "\\u00" << hex[c >> 4] << hex[c & 15];
        else out << char(c);
    }
    out << '"';
}
inline void write_label_category(std::ostream& out, const LabelCategory& category) {
    out << "{\"category_id\":" << category.id << ",\"category\":\"" << label_category_name(category.id)
        << "\",\"carbohydrate_grams_per_value\":";
    if (category.id == 3 && std::isfinite(category.weight)) out << category.weight;
    else out << "null";
    out << '}';
}

inline std::string label_categories_json(std::span<const AmountLabel> labels, int blood_label,
                                         std::string_view blood_unit, double mg_dl_per_mmol_l) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out.precision(9);
    out << "{\"schema\":1,\"category_mapping\":\"current_settings\",\"blood_glucose_label_id\":";
    const bool has_blood_label = blood_label >= 0 && static_cast<std::size_t>(blood_label) < labels.size();
    if (has_blood_label) out << blood_label;
    else out << "null";
    out << ",\"blood_glucose_unit\":";
    write_label_json_string(out, blood_unit);
    out << ",\"mg_dl_per_mmol_l\":" << mg_dl_per_mmol_l << ",\"labels\":[";
    for (std::size_t i = 0; i < labels.size(); ++i) {
        const auto& label = labels[i];
        if (i) out << ',';
        out << "{\"id\":" << i << ",\"name\":";
        write_label_json_string(out, label.name);
        out << ",\"nightscout\":";
        write_label_category(out, label.nightscout);
        out << ",\"libreview\":";
        write_label_category(out, label.libreview);
        // Clarity uses the LibreView mapping except for the shared blood label.
        out << ",\"clarity_category\":\"" << (has_blood_label && i == static_cast<std::size_t>(blood_label)
            ? "blood_glucose" : label_category_name(label.libreview.id)) << "\"}";
    }
    out << "]}";
    return out.str();
}
}
