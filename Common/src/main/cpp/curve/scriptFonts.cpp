// Best-effort Android system font fallback loading for NanoVG, API 21+.
//
// This version uses a small parameter bundle:
//   struct FontUse { NVGcontext* vg; int whitefont; int blackfont; bool* scriptEnabled; };
//
// Persist scriptEnabled[SCR_COUNT] if you want remembered scripts to survive restart.
// After restart:
//   1) recreate your base fonts in the new NVGcontext
//   2) restore scriptEnabled[]
//   3) call applyfonts(FontUse{vg, whitefont, blackfont, scriptEnabled})
//
// Notes:
// - This simplified version tracks only scripts, not already-loaded font files.
// - Therefore some fallback font files may be loaded more than once.
// - whitefont and blackfont must be valid NanoVG font ids in the target NVGcontext.

#ifndef JUGGLUCO_APP
#define LINUXFONTS
#endif

#include <nanovg.h>

#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include "logs.hpp"

#ifndef SYSTEM_FONT_PATH
#define SYSTEM_FONT_PATH "/system/fonts/"
#endif

#include "scriptFonts.hpp"

struct FontCandidate {
    const char* path;
    int index; // -1 = normal font file, >=0 = TTC face index
};

#if defined(LINUXFONTS)
#define FC(file)        FontCandidate{ "/usr/share/fonts/truetype/noto/" file, -1 }
#define FCI(file,i)     FontCandidate{ "/usr/share/fonts/truetype/noto/" file,  i }
#define FC_CJK(file)    FontCandidate{ "/usr/share/fonts/opentype/noto/" file, -1 }
#define FCI_CJK(file,i) FontCandidate{ "/usr/share/fonts/opentype/noto/" file,  i }
#define FC_DJ(file)     FontCandidate{ "/usr/share/fonts/truetype/dejavu/" file, -1 }
#else
#define FC(file)        FontCandidate{ SYSTEM_FONT_PATH file, -1 }
#define FCI(file,i)     FontCandidate{ SYSTEM_FONT_PATH file,  i }
#define FC_CJK(file)    FC(file)
#define FCI_CJK(file,i) FCI(file,i)
#define FC_DJ(file)     FC(file)
#endif

namespace {

static bool inRange(uint32_t cp, uint32_t a, uint32_t b) {
    return cp >= a && cp <= b;
}

static uint32_t nextUtf8Codepoint(const char*& s) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s);
    if (*p == 0) return 0;

    uint32_t cp = 0xFFFD;
    const unsigned char c0 = *p++;

    if (c0 < 0x80) {
        cp = c0;
    } else if ((c0 & 0xE0) == 0xC0) {
        const unsigned char c1 = *p;
        if ((c1 & 0xC0) == 0x80) {
            ++p;
            cp = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
            if (cp < 0x80) cp = 0xFFFD;
        }
    } else if ((c0 & 0xF0) == 0xE0) {
        const unsigned char c1 = p[0];
        const unsigned char c2 = p[1];
        if (((c1 & 0xC0) == 0x80) && ((c2 & 0xC0) == 0x80)) {
            p += 2;
            cp = ((c0 & 0x0F) << 12) |
                 ((c1 & 0x3F) << 6) |
                 (c2 & 0x3F);
            if (cp < 0x800) cp = 0xFFFD;
        }
    } else if ((c0 & 0xF8) == 0xF0) {
        const unsigned char c1 = p[0];
        const unsigned char c2 = p[1];
        const unsigned char c3 = p[2];
        if (((c1 & 0xC0) == 0x80) &&
            ((c2 & 0xC0) == 0x80) &&
            ((c3 & 0xC0) == 0x80)) {
            p += 3;
            cp = ((c0 & 0x07) << 18) |
                 ((c1 & 0x3F) << 12) |
                 ((c2 & 0x3F) << 6) |
                 (c3 & 0x3F);
            if (cp < 0x10000 || cp > 0x10FFFF) cp = 0xFFFD;
        }
    }

    s = reinterpret_cast<const char*>(p);
    return cp;
}

static int loadFirstAvailable(NVGcontext* vg,
                              std::initializer_list<FontCandidate> candidates) {
    static int aliasSerial = 0;

    for (const FontCandidate& c : candidates) {
        char alias[32];
        std::snprintf(alias, sizeof(alias), "fb_%d", aliasSerial++);

        const int id = (c.index >= 0)
            ? nvgCreateFontAtIndex(vg, alias, c.path, c.index)
            : nvgCreateFont(vg, alias, c.path);

        if (id != -1) {
            LOGGER("load font %s\n",c.path);
            return id;
            }
    }
    return -1;
}

static bool attachFallbacks(const FontUse& f,
                            std::initializer_list<FontCandidate> regularCandidates,
                            std::initializer_list<FontCandidate> boldCandidates) {
    bool loaded = false;

    const int regular = loadFirstAvailable(f.vg, regularCandidates);
    const int bold    = loadFirstAvailable(f.vg, boldCandidates);

    if (regular != -1) {
        nvgAddFallbackFontId(f.vg, f.whitefont, regular);
        loaded = true;
    }
    if (bold != -1) {
        nvgAddFallbackFontId(f.vg, f.blackfont, bold);
        loaded = true;
    }

    return loaded;
}

static bool loadScriptToContext(const FontUse& f, Script s) {
    switch (s) {
        case SCR_ARABIC:
            return attachFallbacks(f,
                { FC("NotoNaskhArabic-Regular.ttf"), FC("NotoNaskh-Regular.ttf"),
                  FC("DroidSansArabic.ttf"), FC("DroidSansFallback.ttf") },
                { FC("NotoNaskhArabic-Bold.ttf"), FC("NotoNaskh-Bold.ttf"),
                  FC("DroidSansArabic.ttf"), FC("DroidSansFallback.ttf") });

        case SCR_HEBREW:
            return attachFallbacks(f,
                { FC("NotoSansHebrew-Regular.ttf"), FC("DroidSansFallback.ttf") },
                { FC("NotoSansHebrew-Bold.ttf"),    FC("DroidSansFallback.ttf") });

        case SCR_DEVANAGARI:
            return attachFallbacks(f,
                { FC("NotoSansDevanagari-Regular.ttf"), FC("NotoSansDevanagariUI-Regular.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansDevanagari-Bold.ttf"),    FC("NotoSansDevanagariUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_BENGALI:
            return attachFallbacks(f,
                { FC("NotoSansBengali-Regular.ttf"), FC("NotoSansBengaliUI-Regular.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansBengali-Bold.ttf"),    FC("NotoSansBengaliUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_GURMUKHI:
            return attachFallbacks(f,
                { FC("NotoSansGurmukhi-Regular.ttf"), FC("NotoSansGurmukhiUI-Regular.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansGurmukhi-Bold.ttf"),    FC("NotoSansGurmukhiUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_GUJARATI:
            return attachFallbacks(f,
                { FC("NotoSansGujarati-Regular.ttf"), FC("NotoSansGujaratiUI-Regular.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansGujarati-Bold.ttf"),    FC("NotoSansGujaratiUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_ORIYA:
            return attachFallbacks(f,
                { FC("NotoSansOriya-Regular.ttf"), FC("NotoSansOriyaUI-Regular.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansOriya-Bold.ttf"),    FC("NotoSansOriyaUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_TAMIL:
            return attachFallbacks(f,
                { FC("NotoSansTamil-Regular.ttf"), FC("NotoSansTamilUI-Regular.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansTamil-Bold.ttf"),    FC("NotoSansTamilUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_TELUGU:
            return attachFallbacks(f,
                { FC("NotoSansTelugu-Regular.ttf"), FC("NotoSansTeluguUI-Regular.ttf"),
                  FC("NotoSansTelugu.ttf"), FC("NotoSansTeluguUI.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansTelugu-Bold.ttf"),    FC("NotoSansTeluguUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_KANNADA:
            return attachFallbacks(f,
                { FC("NotoSansKannada-Regular.ttf"), FC("NotoSansKannadaUI-Regular.ttf"),
                  FC("NotoSansKannada.ttf"), FC("NotoSansKannadaUI.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansKannada-Bold.ttf"),    FC("NotoSansKannadaUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_MALAYALAM:
            return attachFallbacks(f,
                { FC("NotoSansMalayalam-Regular.ttf"), FC("NotoSansMalayalamUI-Regular.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansMalayalam-Bold.ttf"),    FC("NotoSansMalayalamUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_SINHALA:
            return attachFallbacks(f,
                { FC("NotoSansSinhala-Regular.ttf"), FC("DroidSansFallback.ttf") },
                { FC("NotoSansSinhala-Bold.ttf"),    FC("DroidSansFallback.ttf") });

        case SCR_THAI:
            return attachFallbacks(f,
                { FC("NotoSansThai-Regular.ttf"), FC("DroidSansFallback.ttf") },
                { FC("NotoSansThai-Bold.ttf"),    FC("DroidSansFallback.ttf") });

        case SCR_LAO:
            return attachFallbacks(f,
                { FC("NotoSansLao-Regular.ttf"), FC("NotoSansLaoUI-Regular.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansLao-Bold.ttf"),    FC("NotoSansLaoUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_MYANMAR:
            return attachFallbacks(f,
                { FC("NotoSansMyanmar-Regular.ttf"), FC("NotoSansMyanmarUI-Regular.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansMyanmar-Bold.ttf"),    FC("NotoSansMyanmarUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_KHMER:
            return attachFallbacks(f,
                { FC("NotoSansKhmer-Regular.ttf"), FC("NotoSansKhmerUI-Regular.ttf"),
                  FC("DroidSansFallback.ttf") },
                { FC("NotoSansKhmer-Bold.ttf"),    FC("NotoSansKhmerUI-Bold.ttf"),
                  FC("DroidSansFallback.ttf") });

        case SCR_ARMENIAN:
            return attachFallbacks(f,
                { FC("NotoSansArmenian-Regular.ttf"), FC("DroidSansFallback.ttf") },
                { FC("NotoSansArmenian-Bold.ttf"),    FC("DroidSansFallback.ttf") });

        case SCR_GEORGIAN:
            return attachFallbacks(f,
                { FC("NotoSansGeorgian-Regular.ttf"), FC("DroidSansFallback.ttf") },
                { FC("NotoSansGeorgian-Bold.ttf"),    FC("DroidSansFallback.ttf") });

        case SCR_ETHIOPIC:
            return attachFallbacks(f,
                { FC("NotoSansEthiopic-Regular.ttf"), FC("DroidSansFallback.ttf") },
                { FC("NotoSansEthiopic-Bold.ttf"),    FC("DroidSansFallback.ttf") });

        case SCR_CJK:
            return attachFallbacks(f,
#if defined(LINUXFONTS)
                { FC_CJK("NotoSansCJK-Regular.ttc"), FC_CJK("NotoSerifCJK-Regular.ttc"),
                  FC_DJ("DejaVuSans.ttf") },
                { FC_CJK("NotoSansCJK-Bold.ttc"), FC_CJK("NotoSansCJK-Regular.ttc"),
                  FC_CJK("NotoSerifCJK-Bold.ttc"), FC_DJ("DejaVuSans-Bold.ttf") });
#else
                { FC_CJK("NotoSansCJK-Regular.ttc"), FC("DroidSansFallback.ttf") },
                { FC_CJK("NotoSansCJK-Regular.ttc"), FC("DroidSansFallback.ttf") });
#endif


        case SCR_GENERIC_FALLBACK:
            return attachFallbacks(f,
#if defined(LINUXFONTS)
                { FC_DJ("DejaVuSans.ttf"), FC("NotoSans-Regular.ttf") },
                { FC_DJ("DejaVuSans-Bold.ttf"), FC("NotoSans-Bold.ttf") });
#else
                { FC("DroidSansFallback.ttf") },
                { FC("DroidSansFallback.ttf") });
#endif

        default:
            return false;
    }
}



static bool isArabic(uint32_t cp) {
    return inRange(cp, 0x0600, 0x06FF) || inRange(cp, 0x0750, 0x077F) ||
           inRange(cp, 0x08A0, 0x08FF) || inRange(cp, 0xFB50, 0xFDFF) ||
           inRange(cp, 0xFE70, 0xFEFF) || inRange(cp, 0x1EE00, 0x1EEFF);
}

static bool isHebrew(uint32_t cp) {
    return inRange(cp, 0x0590, 0x05FF) || inRange(cp, 0xFB1D, 0xFB4F);
}

static bool isDevanagari(uint32_t cp) {
    return inRange(cp, 0x0900, 0x097F) || inRange(cp, 0xA8E0, 0xA8FF);
}

static bool isBengali(uint32_t cp)   { return inRange(cp, 0x0980, 0x09FF); }
static bool isGurmukhi(uint32_t cp)  { return inRange(cp, 0x0A00, 0x0A7F); }
static bool isGujarati(uint32_t cp)  { return inRange(cp, 0x0A80, 0x0AFF); }
static bool isOriya(uint32_t cp)     { return inRange(cp, 0x0B00, 0x0B7F); }
static bool isTamil(uint32_t cp)     { return inRange(cp, 0x0B80, 0x0BFF); }
static bool isTelugu(uint32_t cp)    { return inRange(cp, 0x0C00, 0x0C7F); }
static bool isKannada(uint32_t cp)   { return inRange(cp, 0x0C80, 0x0CFF); }
static bool isMalayalam(uint32_t cp) { return inRange(cp, 0x0D00, 0x0D7F); }
static bool isSinhala(uint32_t cp)   { return inRange(cp, 0x0D80, 0x0DFF); }
static bool isThai(uint32_t cp)      { return inRange(cp, 0x0E00, 0x0E7F); }
static bool isLao(uint32_t cp)       { return inRange(cp, 0x0E80, 0x0EFF); }

static bool isMyanmar(uint32_t cp) {
    return inRange(cp, 0x1000, 0x109F) || inRange(cp, 0xA9E0, 0xA9FF) ||
           inRange(cp, 0xAA60, 0xAA7F);
}

static bool isKhmer(uint32_t cp) {
    return inRange(cp, 0x1780, 0x17FF) || inRange(cp, 0x19E0, 0x19FF);
}

static bool isArmenian(uint32_t cp) {
    return inRange(cp, 0x0530, 0x058F) || inRange(cp, 0xFB13, 0xFB17);
}

static bool isGeorgian(uint32_t cp) {
    return inRange(cp, 0x10A0, 0x10FF) || inRange(cp, 0x2D00, 0x2D2F) ||
           inRange(cp, 0x1C90, 0x1CBF);
}

static bool isEthiopic(uint32_t cp) {
    return inRange(cp, 0x1200, 0x137F) || inRange(cp, 0x1380, 0x139F) ||
           inRange(cp, 0x2D80, 0x2DDF) || inRange(cp, 0xAB01, 0xAB2E);
}

static bool isHiraganaOrKatakana(uint32_t cp) {
    return inRange(cp, 0x3040, 0x309F) || inRange(cp, 0x30A0, 0x30FF) ||
           inRange(cp, 0x31F0, 0x31FF) || inRange(cp, 0xFF66, 0xFF9D);
}

static bool isHangul(uint32_t cp) {
    return inRange(cp, 0x1100, 0x11FF) || inRange(cp, 0x3130, 0x318F) ||
           inRange(cp, 0xA960, 0xA97F) || inRange(cp, 0xAC00, 0xD7AF) ||
           inRange(cp, 0xD7B0, 0xD7FF);
}

static bool isBopomofo(uint32_t cp) {
    return inRange(cp, 0x3100, 0x312F) || inRange(cp, 0x31A0, 0x31BF);
}

static bool isHan(uint32_t cp) {
    return inRange(cp, 0x3400, 0x4DBF) || inRange(cp, 0x4E00, 0x9FFF) ||
           inRange(cp, 0xF900, 0xFAFF) || inRange(cp, 0x20000, 0x2A6DF) ||
           inRange(cp, 0x2A700, 0x2B73F) || inRange(cp, 0x2B740, 0x2B81F) ||
           inRange(cp, 0x2B820, 0x2CEAF) || inRange(cp, 0x2CEB0, 0x2EBEF) ||
           inRange(cp, 0x30000, 0x3134F);
}

} // namespace
#if 0
static const char * scriptname[] {
    "ARABIC",
    "HEBREW",
    "DEVANAGARI",
    "BENGALI",
    "GURMUKHI",
    "GUJARATI",
    "ORIYA",
    "TAMIL",
    "TELUGU",
    "KANNADA",
    "MALAYALAM",
    "SINHALA",
    "THAI",
    "LAO",
    "MYANMAR",
    "KHMER",
    "ARMENIAN",
    "GEORGIAN",
    "ETHIOPIC",
    "CJK",
    "GENERIC_FALLBACK",
    "COUNT"
};
#endif
bool enableScript(const FontUse& f, Script s) {
    const int si = static_cast<int>(s);
    if(f.scriptEnabled[si])  {
      //  LOGGER("enableScript %s already enabled\n",scriptname[s]);
        return true;
        }
   // LOGGER("enableScript %s new\n",scriptname[s]);
    const bool loaded = loadScriptToContext(f, s);
    f.scriptEnabled[si] = loaded;
    return loaded;
}

void applyfonts(const FontUse& f) {
    if (!f.vg || !f.scriptEnabled) return;

    for (int s = 0; s < SCR_COUNT; ++s) {
        if(f.scriptEnabled[s]) {
          loadScriptToContext(f,static_cast<Script>(s));
        }
    }
}

void useFontsForName(const FontUse& f, const char* name) {
    if (!f.vg || !f.scriptEnabled || !name || !*name) return;

    bool hasHan = false;
    bool hasKana = false;
    bool hasHangul = false;
    bool hasBopomofo = false;
    bool needGenericFallback = false;

    const char* p = name;
    while (*p) {
        const uint32_t cp = nextUtf8Codepoint(p);
        if (cp == 0) break;

        if (cp < 0x80) {
            continue;
        } else if (isArabic(cp)) {
            enableScript(f, SCR_ARABIC);
        } else if (isHebrew(cp)) {
            enableScript(f, SCR_HEBREW);
        } else if (isDevanagari(cp)) {
            enableScript(f, SCR_DEVANAGARI);
        } else if (isBengali(cp)) {
            enableScript(f, SCR_BENGALI);
        } else if (isGurmukhi(cp)) {
            enableScript(f, SCR_GURMUKHI);
        } else if (isGujarati(cp)) {
            enableScript(f, SCR_GUJARATI);
        } else if (isOriya(cp)) {
            enableScript(f, SCR_ORIYA);
        } else if (isTamil(cp)) {
            enableScript(f, SCR_TAMIL);
        } else if (isTelugu(cp)) {
            enableScript(f, SCR_TELUGU);
        } else if (isKannada(cp)) {
            enableScript(f, SCR_KANNADA);
        } else if (isMalayalam(cp)) {
            enableScript(f, SCR_MALAYALAM);
        } else if (isSinhala(cp)) {
            enableScript(f, SCR_SINHALA);
        } else if (isThai(cp)) {
            enableScript(f, SCR_THAI);
        } else if (isLao(cp)) {
            enableScript(f, SCR_LAO);
        } else if (isMyanmar(cp)) {
            enableScript(f, SCR_MYANMAR);
        } else if (isKhmer(cp)) {
            enableScript(f, SCR_KHMER);
        } else if (isArmenian(cp)) {
            enableScript(f, SCR_ARMENIAN);
        } else if (isGeorgian(cp)) {
            enableScript(f, SCR_GEORGIAN);
        } else if (isEthiopic(cp)) {
            enableScript(f, SCR_ETHIOPIC);
        } else if (isHiraganaOrKatakana(cp)) {
            hasKana = true;
        } else if (isHangul(cp)) {
            hasHangul = true;
        } else if (isBopomofo(cp)) {
            hasBopomofo = true;
        } else if (isHan(cp)) {
            hasHan = true;
        } else {
            needGenericFallback = true;
        }
    }

    if (hasHangul||hasKana||hasBopomofo||hasHan) {
         enableScript(f, SCR_CJK);
        }


    if (needGenericFallback) {
        enableScript(f, SCR_GENERIC_FALLBACK);
    }
}
