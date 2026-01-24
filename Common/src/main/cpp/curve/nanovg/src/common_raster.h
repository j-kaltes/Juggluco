/*
Contains additions to nanovg_rt.h by Jaap Korthals Altes during a long conversation with 
ChatGPT,  resulting in an extraordinary speed increase and in some cases a better look.

https://github.com/j-kaltes/nanovg-nanort

Fri Jan 23 10:43:32 CET 2026

*/
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

struct EdgeFP {
    int x;
    int dxdy;
    int yStart;
    int yMax;
    int winding;   // +1 or -1
};
// Forward declarations (include your real headers before this if you prefer)
struct RTNVGfragUniforms;
struct RTNVGcontext;
static void rtnvg__shade(float color[4], RTNVGcontext *rt,
                         RTNVGfragUniforms *frag, float x, float y, float tu,
                         float tv, int imageId);

// ---------- basic helpers ----------
static inline unsigned char cr_clamp_u8(int v) {
  return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

static inline unsigned char cr_f2b(float v) {
  // expects v ~ [0..1], clamps anyway
  return cr_clamp_u8((int)lroundf(v * 255.0f));
}

// ---------- premultiplied solid extraction ----------
static inline void cr_solidPremulBytesFromFrag(
    const RTNVGfragUniforms* frag,
    unsigned char& srcR, unsigned char& srcG, unsigned char& srcB, unsigned char& srcA)
{
  // In nanovg_rt, convertPaint() stores premultiplied colors in frag->innerCol.
  // So r/g/b are already multiplied by a.
  srcA = cr_f2b(frag->innerCol.a);
  srcR = cr_f2b(frag->innerCol.r);
  srcG = cr_f2b(frag->innerCol.g);
  srcB = cr_f2b(frag->innerCol.b);
}

// ---------- blend: premultiplied solid over dst with coverage ----------
static inline void cr_blendSolidPremulCov(
    unsigned char* dstRGBA,
    unsigned char srcR, unsigned char srcG, unsigned char srcB, unsigned char srcA,
    unsigned char cov /*0..255*/)
{
  // Effective alpha after coverage
  int a  = (srcA * (int)cov + 127) / 255; // 0..255
  int ia = 255 - a;

  // Scale premultiplied RGB by coverage cov/255
  dstRGBA[0] = (unsigned char)(((int)srcR * (int)cov + (int)dstRGBA[0] * ia + 127) / 255);
  dstRGBA[1] = (unsigned char)(((int)srcG * (int)cov + (int)dstRGBA[1] * ia + 127) / 255);
  dstRGBA[2] = (unsigned char)(((int)srcB * (int)cov + (int)dstRGBA[2] * ia + 127) / 255);

  // Keep alpha consistent with "over"
  dstRGBA[3] = (unsigned char)(a + (((int)dstRGBA[3] * ia + 127) / 255));

  // If your framebuffer is logically opaque and you want speed, you can instead:
  // dstRGBA[3] = 255;
}

// ---------- triangle raster core (1x or 2x2 MSAA) ----------
static inline float cr_edgeFn(float ax, float ay, float bx, float by, float px, float py) {
  return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

static inline bool isTopLeft(float ax,float ay,float bx,float by) {
  // Edge is top-left if it goes up, or is horizontal and goes left
  return (ay < by) || (ay == by && ax > bx);
}

static inline bool insideTL(float e, bool tl) {
  // include if e>0, or e==0 on top-left edges only
  return (e > 0.0f) || (e == 0.0f && tl);
}
// Rasterize one triangle (solid premul) into rgba.
// supersample: 1 => 1 sample/pixel, 2 => 2x2 MSAA
static inline void cr_rasterize_one_triangle_solid(
    int width, int height,
    float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    unsigned char* rgba,
    unsigned char srcR, unsigned char srcG, unsigned char srcB, unsigned char srcA,
    int supersample)
{
  float area = cr_edgeFn(x0, y0, x1, y1, x2, y2);
  if (!std::isfinite(area) || area == 0.0f) return;

  // Make CCW
  float ax=x0, ay=y0, bx=x1, by=y1, cx=x2, cy=y2;
  if (area < 0.0f) { std::swap(bx, cx); std::swap(by, cy); }

  int minx = (int)std::floor(std::min({ax, bx, cx}));
  int maxx = (int)std::ceil (std::max({ax, bx, cx}));
  int miny = (int)std::floor(std::min({ay, by, cy}));
  int maxy = (int)std::ceil (std::max({ay, by, cy}));

  if (maxx <= 0 || maxy <= 0 || minx >= width || miny >= height) return;
  minx = std::max(minx, 0);
  miny = std::max(miny, 0);
  maxx = std::min(maxx, width);
  maxy = std::min(maxy, height);

//  const float eps = -1e-7f;

/*  int spp = (supersample == 2) ? 4 : 1;
  const float sx4[4] = {0.25f, 0.75f, 0.25f, 0.75f};
  const float sy4[4] = {0.25f, 0.25f, 0.75f, 0.75f};
  */
int spp;
const float* sx;
const float* sy;

static const float sx1[1] = {0.5f};
static const float sy1[1] = {0.5f};

static const float sx2[4] = {0.25f, 0.75f, 0.25f, 0.75f};
static const float sy2[4] = {0.25f, 0.25f, 0.75f, 0.75f};

// 4x4 uniform grid at pixel centers of subcells
static const float sx4x4[16] = {
  0.125f, 0.375f, 0.625f, 0.875f,
  0.125f, 0.375f, 0.625f, 0.875f,
  0.125f, 0.375f, 0.625f, 0.875f,
  0.125f, 0.375f, 0.625f, 0.875f
};
static const float sy4x4[16] = {
  0.125f, 0.125f, 0.125f, 0.125f,
  0.375f, 0.375f, 0.375f, 0.375f,
  0.625f, 0.625f, 0.625f, 0.625f,
  0.875f, 0.875f, 0.875f, 0.875f
};

if (supersample <= 1) {
  spp = 1;  sx = sx1;   sy = sy1;
} else if (supersample == 2) {
  spp = 4;  sx = sx2;   sy = sy2;
} else { // supersample >= 4
  spp = 16; sx = sx4x4; sy = sy4x4;
}


  for (int py = miny; py < maxy; ++py) {
    int row = py * width;
    for (int px = minx; px < maxx; ++px) {
      int covered = 0;

        for (int s = 0; s < spp; ++s) {
          float fx = px + sx[s];
          float fy = py + sy[s];
          float e01 = cr_edgeFn(ax, ay, bx, by, fx, fy);
          float e12 = cr_edgeFn(bx, by, cx, cy, fx, fy);
          float e20 = cr_edgeFn(cx, cy, ax, ay, fx, fy);
//          if (e01 >= eps && e12 >= eps && e20 >= eps) covered++;
bool tl01 = isTopLeft(ax,ay, bx,by);
bool tl12 = isTopLeft(bx,by, cx,cy);
bool tl20 = isTopLeft(cx,cy, ax,ay);

if (insideTL(e01, tl01) && insideTL(e12, tl12) && insideTL(e20, tl20)) covered++;



        }
      if (!covered) continue;
      unsigned char cov = (unsigned char)((covered * 255 + spp / 2) / spp);
      unsigned char* dst = &rgba[4 * (row + px)];
      cr_blendSolidPremulCov(dst, srcR, srcG, srcB, srcA, cov);
    }
  }
}



// Blend premultiplied solid with float coverage 0..1




static inline void cr_rasterize_one_triangle_cov(
    int width, int height,
    float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    unsigned char* cov,
    int supersample)
{
  float area = cr_edgeFn(x0, y0, x1, y1, x2, y2);
  if (!std::isfinite(area) || area == 0.0f) return;

  // Make CCW
  float ax=x0, ay=y0, bx=x1, by=y1, cx=x2, cy=y2;
  if (area < 0.0f) { std::swap(bx, cx); std::swap(by, cy); }

  int minx = (int)std::floor(std::min({ax, bx, cx}));
  int maxx = (int)std::ceil (std::max({ax, bx, cx}));
  int miny = (int)std::floor(std::min({ay, by, cy}));
  int maxy = (int)std::ceil (std::max({ay, by, cy}));

  if (maxx <= 0 || maxy <= 0 || minx >= width || miny >= height) return;
  minx = std::max(minx, 0);
  miny = std::max(miny, 0);
  maxx = std::min(maxx, width);
  maxy = std::min(maxy, height);

  // Sample pattern selection (as you have it)
  int spp;
  const float* sx;
  const float* sy;

  static const float sx1[1] = {0.5f};
  static const float sy1[1] = {0.5f};

  static const float sx2[4] = {0.25f, 0.75f, 0.25f, 0.75f};
  static const float sy2[4] = {0.25f, 0.25f, 0.75f, 0.75f};

  static const float sx4x4[16] = {
    0.125f, 0.375f, 0.625f, 0.875f,
    0.125f, 0.375f, 0.625f, 0.875f,
    0.125f, 0.375f, 0.625f, 0.875f,
    0.125f, 0.375f, 0.625f, 0.875f
  };
  static const float sy4x4[16] = {
    0.125f, 0.125f, 0.125f, 0.125f,
    0.375f, 0.375f, 0.375f, 0.375f,
    0.625f, 0.625f, 0.625f, 0.625f,
    0.875f, 0.875f, 0.875f, 0.875f
  };

  if (supersample <= 1) { spp = 1;  sx = sx1;   sy = sy1; }
  else if (supersample == 2) { spp = 4;  sx = sx2;   sy = sy2; }
  else { spp = 16; sx = sx4x4; sy = sy4x4; }

  // Compute TL flags ONCE per edge
  const bool tl01 = isTopLeft(ax,ay, bx,by);
  const bool tl12 = isTopLeft(bx,by, cx,cy);
  const bool tl20 = isTopLeft(cx,cy, ax,ay);

  for (int py = miny; py < maxy; ++py) {
    int row = py * width;
    for (int px = minx; px < maxx; ++px) {
      int covered = 0;

      for (int s = 0; s < spp; ++s) {
        float fx = px + sx[s];
        float fy = py + sy[s];

        float e01 = cr_edgeFn(ax, ay, bx, by, fx, fy);
        float e12 = cr_edgeFn(bx, by, cx, cy, fx, fy);
        float e20 = cr_edgeFn(cx, cy, ax, ay, fx, fy);

        if (insideTL(e01, tl01) && insideTL(e12, tl12) && insideTL(e20, tl20))
          covered++;
      }

      if (!covered) continue;

      unsigned char c = (unsigned char)((covered * 255 + spp/2) / spp);

      // For stroke strips: union coverage (avoid self-overlap darkening)
      unsigned char& dst = cov[row + px];
      if (c > dst) dst = c;
    }
  }
}


static inline unsigned char sample_alpha_nearest(const RTNVGtexture* tex, float u, float v) {
  u = std::max(0.0f, std::min(1.0f, u));
  v = std::max(0.0f, std::min(1.0f, v));
  int x = (int)lroundf(u * float(tex->width  - 1));
  int y = (int)lroundf(v * float(tex->height - 1));
  return tex->data[y * tex->width + x];
}
static inline unsigned char clamp_u8_int(int v) {
  return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v));
}
static inline void cr_alphaBlendPremulCov_u8(unsigned char* dst, const float col[4], unsigned char cov)
{



  // col[] is premultiplied float in 0..1 (as produced by rtnvg__shade)
  // cov is 0..255 coverage
  const int c = (int)cov;              // 0..255

  // Convert premul float to premul byte, scaled by coverage:
  // srcByte = round(col * cov)
  int srcR = (int)lroundf(col[0] * (float)c);
  int srcG = (int)lroundf(col[1] * (float)c);
  int srcB = (int)lroundf(col[2] * (float)c);
  int srcA = (int)lroundf(col[3] * (float)c);

  if (srcA <= 0) return;
  if (srcA > 255) srcA = 255;

  const int ia = 255 - srcA;

  // out = src + dst*(1-srcA)
  dst[0] = (unsigned char)std::min(255, srcR + ((dst[0] * ia + 127) / 255));
  dst[1] = (unsigned char)std::min(255, srcG + ((dst[1] * ia + 127) / 255));
  dst[2] = (unsigned char)std::min(255, srcB + ((dst[2] * ia + 127) / 255));
  dst[3] = (unsigned char)std::min(255, srcA + ((dst[3] * ia + 127) / 255));
}


static inline void cr_shadeAvg2x2(float out[4], RTNVGcontext* rt, RTNVGfragUniforms* frag, int imageId, int x, int y)
{
  // 2x2 in-pixel positions
  static const float sx[4] = {0.25f, 0.75f, 0.25f, 0.75f};
  static const float sy[4] = {0.25f, 0.25f, 0.75f, 0.75f};

  out[0] = out[1] = out[2] = out[3] = 0.0f;

  for (int i = 0; i < 4; ++i) {
    float col[4];
    float px = x + sx[i];
    float py = y + sy[i];
    rtnvg__shade(col, rt, frag, px, py, 0.0f, 0.0f, imageId);
    out[0] += col[0];
    out[1] += col[1];
    out[2] += col[2];
    out[3] += col[3];
  }

  const float inv = 1.0f / 4.0f;
  out[0] *= inv; out[1] *= inv; out[2] *= inv; out[3] *= inv;
}
static void scanlineFillFP_CoverageAndShade(
    RTNVGcontext *rt,
    int imageId,
    int width, int height,
    const std::vector<std::vector<EdgeFP>> &edgeTable,
    RTNVGfragUniforms *frag,
    unsigned char *rgba,
    int supersample)
{
  const int FP_SHIFT = 16;
  const int FP_ONE = 1 << FP_SHIFT;
  const int heightSS = height * supersample;
  const int fullAdd = (255 + supersample / 2) / supersample;

  // Coverage buffer: 0..255 per pixel
  std::vector<unsigned char> &cov = rt->cov;
  cov.assign(width * height, 0);

  std::vector<EdgeFP> activeEdges;
  activeEdges.reserve(256);

  for (int y = 0; y < heightSS; ++y) {
    for (const EdgeFP &e : edgeTable[y]) activeEdges.push_back(e);

    activeEdges.erase(
      std::remove_if(activeEdges.begin(), activeEdges.end(),
                     [y](const EdgeFP &ae){ return ae.yMax <= y; }),
      activeEdges.end()
    );
    if (activeEdges.empty()) continue;

    // Insertion sort with tie-break
    for (size_t i = 1; i < activeEdges.size(); ++i) {
      EdgeFP key = activeEdges[i];
      int j = (int)i - 1;
      while (j >= 0 && (activeEdges[j].x > key.x ||
             (activeEdges[j].x == key.x && activeEdges[j].dxdy > key.dxdy))) {
        activeEdges[j + 1] = activeEdges[j];
        --j;
      }
      activeEdges[j + 1] = key;
    }

    int winding = 0;
    for (size_t i = 0; i + 1 < activeEdges.size(); ++i) {
      winding += activeEdges[i].winding;
      if (winding == 0) continue;

      int xStartFP = activeEdges[i].x;
      int xEndFP   = activeEdges[i + 1].x;

      constexpr const float xEps = 1e-6f;
      int xStart = std::max(0, (int)std::ceil(((float)xStartFP / (float)FP_ONE) - xEps));
      int xEnd   = std::min(width, (int)std::ceil(((float)xEndFP   / (float)FP_ONE) - xEps));
      if (xStart >= xEnd) continue;

      int py  = y / supersample;
      int row = py * width;

      if (xEnd == xStart + 1) {
        float frac = float(xEndFP - xStartFP) / float(FP_ONE);
        frac = std::clamp(frac, 0.0f, 1.0f);
        int add = (int)lroundf(frac * 255.0f / supersample);
        int idx = row + xStart;
        if (idx < 0 || idx >= width * height) {
          LOGGER_RT("COV OOB single idx=%d w=%d h=%d\n", idx, width, height);
          continue;
        }
        int v = cov[idx] + add;
        cov[idx] = (unsigned char)(v > 255 ? 255 : v);
        continue;
      }

      // Left fractional
      {
        int x = xStart;
        float frac = 1.0f - float(xStartFP & (FP_ONE - 1)) / FP_ONE;
        frac = std::clamp(frac, 0.0f, 1.0f);
        int add = (int)lroundf(frac * 255.0f / supersample);
        int idx = row + x;
        if (idx < 0 || idx >= width * height) {
          LOGGER_RT("COV OOB left idx=%d w=%d h=%d\n", idx, width, height);
          continue;
        }
        int v = cov[idx] + add;
        cov[idx] = (unsigned char)(v > 255 ? 255 : v);
      }

      // Middle
      for (int x = xStart + 1; x < xEnd - 1; ++x) {
        int idx = row + x;
        if (idx < 0 || idx >= width * height) {
          LOGGER_RT("COV OOB mid idx=%d w=%d h=%d\n", idx, width, height);
          continue;
        }
        int v = cov[idx] + fullAdd;
        cov[idx] = (unsigned char)(v > 255 ? 255 : v);
      }

      // Right fractional
      if (xEnd - 1 > xStart) {
        int x = xEnd - 1;
        float frac = float(xEndFP & (FP_ONE - 1)) / FP_ONE;
        frac = std::clamp(frac, 0.0f, 1.0f);
        int add = (int)lroundf(frac * 255.0f / supersample);
        int idx = row + x;
        if (idx < 0 || idx >= width * height) {
          LOGGER_RT("COV OOB right idx=%d w=%d h=%d\n", idx, width, height);
          continue;
        }
        int v = cov[idx] + add;
        cov[idx] = (unsigned char)(v > 255 ? 255 : v);
      }
    }

    for (EdgeFP &ae : activeEdges) ae.x += ae.dxdy;
  }

  // Shade + blend pass (full fidelity)
  for (int y = 0; y < height; ++y) {
    int row = y * width;
    for (int x = 0; x < width; ++x) {
      unsigned char c = cov[row + x];
      if (!c) continue;

      float col[4];
//      rtnvg__shade(col, rt, frag, x + 0.5f, y + 0.5f, 0.0f, 0.0f, imageId);
      if ((int)frag->type != NSVG_SHADER_SIMPLE) {
            cr_shadeAvg2x2(col, rt, frag, imageId, x, y);
          } else {
            rtnvg__shade(col, rt, frag, x + 0.5f, y + 0.5f, 0.0f, 0.0f, imageId);

          }


      unsigned char* dst = &rgba[4 * (row + x)];
      cr_alphaBlendPremulCov_u8(dst, col, c);
    }
  }
}


static inline float edge2(float ax,float ay,float bx,float by,float px,float py){
  return (px-ax)*(by-ay) - (py-ay)*(bx-ax);
}

static void drawTexturedTri_Text(
    RTNVGcontext* rt, RTNVGfragUniforms* frag, int imageId,
    const NVGvertex& a, const NVGvertex& b, const NVGvertex& c)
{
  // Make CCW
  NVGvertex v0=a, v1=b, v2=c;
  float area = edge2(v0.x,v0.y, v1.x,v1.y, v2.x,v2.y);
  if (!std::isfinite(area) || area == 0.0f) return;
  if (area < 0.0f) { std::swap(v1, v2); area = -area; }
  float invA = 1.0f / area;

  int minx = std::max(0, (int)floorf(std::min({v0.x,v1.x,v2.x})));
  int maxx = std::min(rt->width,  (int)ceilf (std::max({v0.x,v1.x,v2.x})));
  int miny = std::max(0, (int)floorf(std::min({v0.y,v1.y,v2.y})));
  int maxy = std::min(rt->height, (int)ceilf (std::max({v0.y,v1.y,v2.y})));
  if (minx >= maxx || miny >= maxy) return;

  unsigned char* rgba = rt->pixels;

  // IMPORTANT: no negative eps; for text prefer unbiased test.
  //ADDED:
//  const float eps = 0.0f;
  const bool tl01 = isTopLeft(v1.x, v1.y, v2.x, v2.y);
  const bool tl12 = isTopLeft(v2.x, v2.y, v0.x, v0.y);
  const bool tl20 = isTopLeft(v0.x, v0.y, v1.x, v1.y);
  //end ADDED

  for (int y=miny; y<maxy; ++y) {
    for (int x=minx; x<maxx; ++x) {
      float px = x + 0.5f, py = y + 0.5f;

      float w0 = edge2(v1.x,v1.y, v2.x,v2.y, px,py);
      float w1 = edge2(v2.x,v2.y, v0.x,v0.y, px,py);
      float w2 = edge2(v0.x,v0.y, v1.x,v1.y, px,py);
      if (!insideTL(w0, tl01) || !insideTL(w1, tl12) || !insideTL(w2, tl20)) continue;

//      if (w0 < eps || w1 < eps || w2 < eps) continue;

      w0 *= invA; w1 *= invA; w2 *= invA;

      float tu = w0*v0.u + w1*v1.u + w2*v2.u;
      float tv = w0*v0.v + w1*v1.v + w2*v2.v;

      float col[4];
      rtnvg__shade(col, rt, frag, px, py, tu, tv, imageId);

      rtnvg__alphaBlend(&rgba[4*(y*rt->width + x)], col);
    }
  }
}

static std::vector<std::vector<EdgeFP>> buildEdgeTableFP_FromContours(
    RTNVGcontext *rt,
    RTNVGpath *paths,
    int npaths,
    int height,
    int supersample)
{
  const int FP_SHIFT = 16;
  const int FP_ONE   = 1 << FP_SHIFT;
  const int heightSS = height * supersample;

  std::vector<std::vector<EdgeFP>> edgeTable(heightSS);

  // Use opposite eps for start/end to enforce half-open y interval
  const float yEps = 1e-6f;

  auto yToStart = [&](float y) {
    return (int)std::ceil((y - yEps) * supersample);
  };
  auto yToEnd = [&](float y) {
    return (int)std::ceil((y + yEps) * supersample); // exclusive
  };

  auto emitEdge = [&](float x0, float y0, float x1, float y1) {
    if (fabsf(y0 - y1) < 1e-12f) return;

    EdgeFP e;

    if (y0 < y1) {
      e.winding = +1;
      e.yStart  = yToStart(y0);
      e.yMax    = yToEnd(y1);
      if (e.yStart >= e.yMax) return;

      float yStartF = float(e.yStart) / supersample;
      float dy      = yStartF - y0;

      // x in pixel-space FP
      e.x = (int)((x0 + dy * (x1 - x0) / (y1 - y0)) * FP_ONE);

      // advance one sub-scanline => divide by supersample
      e.dxdy = (int)((x1 - x0) * FP_ONE / ((y1 - y0) * supersample));
    } else {
      // flip
      e.winding = -1;
      e.yStart  = yToStart(y1);
      e.yMax    = yToEnd(y0);
      if (e.yStart >= e.yMax) return;

      float yStartF = float(e.yStart) / supersample;
      float dy      = yStartF - y1;

      e.x = (int)((x1 + dy * (x0 - x1) / (y0 - y1)) * FP_ONE);
      e.dxdy = (int)((x0 - x1) * FP_ONE / ((y0 - y1) * supersample));
    }


    // Vertical clip to [0, heightSS)
    if (e.yMax <= 0 || e.yStart >= heightSS) return;

    if (e.yStart < 0) {
      e.x += e.dxdy * (-e.yStart);
      e.yStart = 0;
    }
    if (e.yMax > heightSS) e.yMax = heightSS;
    if (e.yStart >= e.yMax) return;

    edgeTable[e.yStart].push_back(e);
  };

    auto samePt = [](const NVGvertex& a, const NVGvertex& b) {
      const float eps = 1e-6f;
      return (std::fabs(a.x - b.x) < eps) && (std::fabs(a.y - b.y) < eps);
    };
  for (int p = 0; p < npaths; ++p) {

        const RTNVGpath &path = paths[p];
        if (path.fillCount < 2) continue;

        int base = path.fillOffset;
        int cnt  = path.fillCount;


        for (int i = 0; i + 1 < cnt; ++i) {
          const NVGvertex &a = rt->verts[base + i];
          const NVGvertex &b = rt->verts[base + i + 1];
          emitEdge(a.x, a.y, b.x, b.y);
        }

        // Close only if last != first
        const NVGvertex &v0 = rt->verts[base + 0];
        const NVGvertex &vN = rt->verts[base + (cnt - 1)];
        if (!samePt(vN, v0)) {
          emitEdge(vN.x, vN.y, v0.x, v0.y);
        }

  }

  return edgeTable;
}
