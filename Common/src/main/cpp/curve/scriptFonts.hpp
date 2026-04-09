#pragma once

#include <nanovg.h>

struct FontUse {
    NVGcontext* vg;
    int whitefont;
    int blackfont;
    bool* scriptEnabled; // array of size SCR_COUNT
};

enum Script {
    SCR_ARABIC = 0,
    SCR_HEBREW,
    SCR_DEVANAGARI,
    SCR_BENGALI,
    SCR_GURMUKHI,
    SCR_GUJARATI,
    SCR_ORIYA,
    SCR_TAMIL,
    SCR_TELUGU,
    SCR_KANNADA,
    SCR_MALAYALAM,
    SCR_SINHALA,
    SCR_THAI,
    SCR_LAO,
    SCR_MYANMAR,
    SCR_KHMER,
    SCR_ARMENIAN,
    SCR_GEORGIAN,
    SCR_ETHIOPIC,
    SCR_CJK,
    SCR_GENERIC_FALLBACK,
    SCR_COUNT
};

// Re-apply any remembered/enabled scripts to a fresh NanoVG context.
void applyfonts(const FontUse& f);

// Detect scripts used in a UTF-8 string and attach needed fallback fonts.
void useFontsForName(const FontUse& f, const char* name);

// Explicitly attach fallback fonts for a specific script.
bool enableScript(const FontUse& f, Script s);
