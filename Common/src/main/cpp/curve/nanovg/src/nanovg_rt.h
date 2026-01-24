/*
Modified by  by Jaap Korthals Altes during a long conversation with ChatGPT,  
resulting in an extraordinary speed increase and in some cases a better look.

https://github.com/j-kaltes/nanovg-nanort

Fri Jan 23 10:43:32 CET 2026
*/

//
// NanoRT(Software raytracer) backend for NanoVG.
//
// Copyright (c) 2015 Syoyo Fujita.
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

//
// nanovg_rt.h is based on nanovg_gl2.h
//
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
//#define NANOVG_GL_USE_UNIFORMBUFFERnot 1
#ifndef NANOVG_RT_H
#define NANOVG_RT_H
#ifdef DOLOG
#include "share/logs.hpp"
#else
   #undef LOGGER
   #undef LOGAR
#define LOGGER(...) fprintf(stderr,__VA_ARGS__) 
#define LOGAR(...) fprintf(stderr,"%s\n",__VA_ARGS__)

#endif
/*
#define LOGGER_RT(...) LOGGER("nanovg_rt: " __VA_ARGS__) 
#define LOGAR_RT(...) LOGAR("nanovg_rt: " __VA_ARGS__)
*/
#define LOGGER_RT(...) 
#define LOGAR_RT(...) 
#include "nanort.h"
//#include "rasterize_triangles_aa.h"
#include <new>
#ifdef __cplusplus
extern "C" {
#endif

// Create flags

enum NVGcreateFlags {
  // Flag indicating if geometry based anti-aliasing is used (may not be needed
  // when using MSAA).
  NVG_ANTIALIAS = 1 << 0,
  // Flag indicating if strokes should be drawn using stencil buffer. The
  // rendering will be a little
  // slower, but path overlaps (i.e. self-intersecting or sharp turns) will be
  // drawn just once.
  NVG_STENCIL_STROKES = 1 << 1,
  // Flag indicating that additional debug checks are done.
  NVG_DEBUG = 1 << 2,
};

NVGcontext *nvgCreateRT(int flags, int w, int h);
void nvgDeleteRT(NVGcontext *ctx);
void nvgClearBackgroundRT(NVGcontext *ctx, float r, float g, float b, float a); // Clear background.
unsigned char *nvgReadPixelsRT(NVGcontext *ctx); // Returns RGBA8 pixel data.

// These are additional flags on top of NVGimageFlags.
enum NVGimageFlagsRT {
  NVG_IMAGE_NODELETE = 1 << 16, // Do not delete RT texture handle.
};

int nvrtCreateImageFromHandle(NVGcontext *ctx, unsigned int textureId, int w,
                              int h, int flags);
unsigned int nvrtImageHandle(NVGcontext *ctx, int image);

#ifdef __cplusplus
}
#endif

#endif /* NANOVG_RT_H */

#ifdef NANOVG_RT_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>

#include "nanovg.h"

//#include "rasterize_triangles_aa.h"
#include "nanovg.h"

namespace {

union fi {
  float f;
  unsigned int i;
};

inline unsigned int mask(int x) { return (1U << x) - 1; }

// from fmath.hpp
/*
  for given y > 0
  get f_y(x) := pow(x, y) for x >= 0
*/

class PowGenerator {
  enum { N = 11 };
  float tbl0_[256];
  struct {
    float app;
    float rev;
  } tbl1_[1 << N];

public:
  PowGenerator(float y) {
    for (int i = 0; i < 256; i++) {
      tbl0_[i] = ::powf(2, (i - 127) * y);
    }
    const double e = 1 / double(1 << 24);
    const double h = 1 / double(1 << N);
    const size_t n = 1U << N;
    for (size_t i = 0; i < n; i++) {
      double x = 1 + double(i) / n;
      double a = ::pow(x, (double)y);
      tbl1_[i].app = (float)a;
      double b = ::pow(x + h - e, (double)y);
      tbl1_[i].rev = (float)((b - a) / (h - e) / (1 << 23));
    }
  }
  float get(float x) const {
    fi fi;
    fi.f = x;
    int a = (fi.i >> 23) & mask(8);
    unsigned int b = fi.i & mask(23);
    unsigned int b1 = b & (mask(N) << (23 - N));
    unsigned int b2 = b & mask(23 - N);
    float f;
    int idx = b1 >> (23 - N);
    f = tbl0_[a] * (tbl1_[idx].app + float(b2) * tbl1_[idx].rev);
    return f;
  }
};

class TextureSampler {

public:
  typedef enum {
    FORMAT_BYTE = 0,
    FORMAT_FLOAT = 1,
  } Format;

  TextureSampler() : m_pow(1.0f) {
    // Make invalid texture
    m_width = -1;
    m_height = -1;
    m_image = NULL;
    m_components = -1;
  }

  TextureSampler(unsigned char *image, int width, int height, int components,
                 Format format, float gamma = 1.0f)
      : m_pow(gamma) {
    Set(image, width, height, components, format, gamma);
  }

  ~TextureSampler() {}

  void Set(const unsigned char *image, int width, int height, int components,
           Format format, float gamma = 1.0f) {
    m_width = width;
    m_height = height;
    m_image = image;
    m_invWidth = 1.0f / width;
    m_invHeight = 1.0f / height;
    m_components = components;
    m_format = format;
    m_gamma = gamma;
  }

  int width() const { return m_width; }

  int height() const { return m_height; }

  int components() const { return m_components; }

  const unsigned char *image() const { return m_image; }

  int format() const { return m_format; }

  float gamma() const { return m_gamma; }

  void fetch(float *rgba, float u, float v) const;

  bool IsValid() const {
    return (m_image != NULL) && (m_width > 0) && (m_height > 0);
  }

private:
  int m_width;
  int m_height;
  float m_invWidth;
  float m_invHeight;
  int m_components;
  const unsigned char *m_image;
  int m_format;
  float m_gamma;
  PowGenerator m_pow;
};

int inline fasterfloor(const float x) {
  if (x >= 0) {
    return (int)x;
  }

  int y = (int)x;
  if (std::abs(x - y) <= std::numeric_limits<float>::epsilon()) {
    // Do nothing.
  } else {
    y = y - 1;
  }

  return y;
}

#if __cplusplus>=202002L
#define mylerp        std::lerp
#else
inline float mylerp(float x, float y, float t) { 
     return x + t * (y - x); 
     }
#endif

// bool myisnan(float a) {
//  volatile float d = a;
//  return d != d;
//}

inline void FilterByteLerp(float *rgba, const unsigned char *image, int i00,
                           int i10, int i01, int i11, float dx, float dy,
                           int stride, const PowGenerator &p) {
  float texel[4][4];

  const float inv = 1.0f / 255.0f;
  if (stride == 4) {

    // Assume color is already degamma'ed.
    for (int i = 0; i < 4; i++) {
      texel[0][i] = (float)image[i00 + i] * inv;
      texel[1][i] = (float)image[i10 + i] * inv;
      texel[2][i] = (float)image[i01 + i] * inv;
      texel[3][i] = (float)image[i11 + i] * inv;
    }

    for (int i = 0; i < 4; i++) {
      rgba[i] = mylerp(mylerp(texel[0][i], texel[1][i], dx),
                     mylerp(texel[2][i], texel[3][i], dx), dy);
    }

  } else {

    for (int i = 0; i < stride; i++) {
      texel[0][i] = (float)image[i00 + i] * inv;
      texel[1][i] = (float)image[i10 + i] * inv;
      texel[2][i] = (float)image[i01 + i] * inv;
      texel[3][i] = (float)image[i11 + i] * inv;
    }

for (int i = 0; i < stride; i++) {
      //rgba[i] = texel[0][i]; // NEAREST
      rgba[i] = mylerp(
                  mylerp(texel[0][i], texel[1][i], dx),
                  mylerp(texel[2][i], texel[3][i], dx),
                  dy);
    }
  }

  if (stride < 4) {
    for (int i = stride; i < 4; i++) {
      rgba[i] = rgba[stride - 1];
    }
    // rgba[3] = 1.0;
  }
}
}

inline void FilterFloatLerp(float *rgba, const float *image, int i00, int i10,
                            int i01, int i11, float dx, float dy, int stride,
                            const PowGenerator &p) {
  float texel[4][4];

  if (stride == 4) {

    for (int i = 0; i < 4; i++) {
      texel[0][i] = image[i00 + i];
      texel[1][i] = image[i10 + i];
      texel[2][i] = image[i01 + i];
      texel[3][i] = image[i11 + i];
    }

    for (int i = 0; i < 4; i++) {
      rgba[i] = mylerp(mylerp(texel[0][i], texel[1][i], dx),
                     mylerp(texel[2][i], texel[3][i], dx), dy);
    }

  } else {

    for (int i = 0; i < stride; i++) {
      texel[0][i] = image[i00 + i];
      texel[1][i] = image[i10 + i];
      texel[2][i] = image[i01 + i];
      texel[3][i] = image[i11 + i]; // alpha is linear
    }

    for (int i = 0; i < stride; i++) {
      rgba[i] = mylerp(mylerp(texel[0][i], texel[1][i], dx),
                     mylerp(texel[2][i], texel[3][i], dx), dy);
    }
  }

  if (stride < 4) {
    rgba[3] = 1.0;
  }
}

void TextureSampler::fetch(float *rgba, float u, float v) const {
  if (!IsValid()) { rgba[0]=rgba[1]=rgba[2]=0.0f; rgba[3]=1.0f; return; }

  // Clamp, do NOT wrap/fract for NanoVG atlases.
  float uu = std::max(0.0f, std::min(1.0f, u));
  float vv = std::max(0.0f, std::min(1.0f, v));


    // Map to texel coordinate space [0 .. w-1], [0 .. h-1]
    float px = uu * float(m_width  - 1);
    float py = vv * float(m_height - 1);

    int x0 = (int)floorf(px);
    int y0 = (int)floorf(py);

    float dx = px - float(x0);
    float dy = py - float(y0);

    int x1 = (x0 + 1 < m_width)  ? (x0 + 1) : x0;
    int y1 = (y0 + 1 < m_height) ? (y0 + 1) : y0;

  // Clamp indices
  if (x0 < 0) { x0 = 0; dx = 0.0f; }
  if (y0 < 0) { y0 = 0; dy = 0.0f; }
  if (x1 >= m_width)  x1 = m_width  - 1;
  if (y1 >= m_height) y1 = m_height - 1;
  if (x0 >= m_width)  x0 = m_width  - 1;
  if (y0 >= m_height) y0 = m_height - 1;

  int stride = m_components;
  int i00 = stride * (y0 * m_width + x0);
  int i10 = stride * (y0 * m_width + x1);
  int i01 = stride * (y1 * m_width + x0);
  int i11 = stride * (y1 * m_width + x1);

  if (m_format == FORMAT_BYTE) {
    FilterByteLerp(rgba, m_image, i00, i10, i01, i11, dx, dy, stride, m_pow);
  } else if (m_format == FORMAT_FLOAT) {
    FilterFloatLerp(rgba, reinterpret_cast<const float *>(m_image),
                    i00, i10, i01, i11, dx, dy, stride, m_pow);
  } else {
    rgba[0]=1.0f; rgba[1]=0.0f; rgba[2]=1.0f; rgba[3]=1.0f;
  }
}

void colorize_material_id(unsigned char col[3], unsigned int mid) {
  unsigned char table[7][3] = {{255, 0, 0},
                       {0, 0, 255},
                       {0, 255, 0},
                       {255, 0, 255},
                       {0, 255, 255},
                       {255, 255, 0},
                       {255, 255, 255}};

  int id = mid % 7;

  col[0] = table[id][0];
  col[1] = table[id][1];
  col[2] = table[id][2];
}

enum RTNVGuniformLoc {
  RTNVG_LOC_VIEWSIZE,
  RTNVG_LOC_TEX,
  RTNVG_LOC_FRAG,
  RTNVG_MAX_LOCS
};

enum RTNVGshaderType {
  NSVG_SHADER_FILLGRAD,
  NSVG_SHADER_FILLIMG,
  NSVG_SHADER_SIMPLE,
  NSVG_SHADER_IMG
};

#if NANOVG_GL_USE_UNIFORMBUFFERnot
enum RTNVGuniformBindings {
  RTNVG_FRAG_BINDING = 0,
};
#endif

struct RTNVGshader {
  unsigned int prog;
  unsigned int frag;
  unsigned int vert;
  int loc[RTNVG_MAX_LOCS];
};
typedef struct RTNVGshader RTNVGshader;

struct RTNVGtexture {
  int id;
  unsigned int tex;
  int width, height;
  int type;
  int flags;
  unsigned char *data;
  unsigned char *dataBase; // allocation base for guard bytes
  size_t dataSize;
};
typedef struct RTNVGtexture RTNVGtexture;

enum RTNVGcallType {
  RTNVG_NONE = 0,
  RTNVG_FILL,
  RTNVG_CONVEXFILL,
  RTNVG_STROKE,
  RTNVG_TRIANGLES,
};

struct RTNVGcall {
  int type;
  int image;
  int pathOffset;
  int pathCount;
  int triangleOffset;
  int triangleCount;
  int uniformOffset;
  float strokeWidth;
};
typedef struct RTNVGcall RTNVGcall;

struct RTNVGpath {
  int fillOffset;
  int fillCount;
  int strokeOffset;
  int strokeCount;
  int winding;
};
typedef struct RTNVGpath RTNVGpath;

struct RTNVGfragUniforms {
#if NANOVG_GL_USE_UNIFORMBUFFERnot
  float scissorMat[12]; // matrices are actually 3 vec4s
  float paintMat[12];
  struct NVGcolor innerCol;
  struct NVGcolor outerCol;
  float scissorExt[2];
  float scissorScale[2];
  float extent[2];
  float radius;
  float feather;
  float strokeMult;
  float strokeThr;
  int texType;
  int type;
#else
// note: after modifying layout or size of uniform array,
// don't forget to also update the fragment shader source!
#define NANOVG_GL_UNIFORMARRAY_SIZE 11
  union {
    struct {
      float scissorMat[12]; // matrices are actually 3 vec4s
      float paintMat[12];
      struct NVGcolor innerCol;
      struct NVGcolor outerCol;
      float scissorExt[2];
      float scissorScale[2];
      float extent[2];
      float radius;
      float feather;
      float strokeMult;
      float strokeThr;
      float texType;
      float type;
    };
    float uniformArray[NANOVG_GL_UNIFORMARRAY_SIZE][4];
  };
#endif
};
typedef struct RTNVGfragUniforms RTNVGfragUniforms;

struct RTNVGcontext {
  RTNVGshader shader;
  RTNVGtexture *textures;
  float view[2];
  int ntextures;
  int ctextures;
  int textureId;
  unsigned int vertBuf;
#if NANOVG_GL_USE_UNIFORMBUFFERnot
  unsigned int fragBuf;
#endif
  int fragSize;
  int flags;

  // Per frame buffers
  RTNVGcall *calls;
  int ccalls;
  int ncalls;
  RTNVGpath *paths;
  int cpaths;
  int npaths;
  struct NVGvertex *verts;
  unsigned char *vertsBase; // allocation base for guard bytes
  size_t vertsSizeBytes;
  int cverts;
  int nverts;
  unsigned char *uniforms;
  unsigned char *uniformsBase; // allocation base for guard bytes
  size_t uniformsSizeBytes;
  int cuniforms;
  int nuniforms;

// cached state
#if NANOVG_GL_USE_STATE_FILTERnot
  unsigned int boundTexture;
  unsigned int stencilMask;
  int stencilFunc;
  int stencilFuncRef;
  unsigned int stencilFuncMask;
#endif

  unsigned char *pixels; // RGBA (guarded)
  unsigned char *pixelsBase; // allocation base for guard bytes
  size_t pixelsSize;
  int width;
  int height;
  std::vector<unsigned char> cov;
};
typedef struct RTNVGcontext RTNVGcontext;

static int rtnvg__maxi(int a, int b) { return a > b ? a : b; }

#ifdef NANOVG_GLES2not
static unsigned int rtnvg__nearestPow2(unsigned int num) {
  unsigned n = num > 0 ? num - 1 : 0;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}
#endif




static RTNVGtexture *rtnvg__allocTexture(RTNVGcontext *rt) {
  RTNVGtexture *tex = NULL;
  int i;

  for (i = 0; i < rt->ntextures; i++) {
    if (rt->textures[i].id == 0) {
      tex = &rt->textures[i];
      break;
    }
  }
  if (tex == NULL) {
    if (rt->ntextures + 1 > rt->ctextures) {
      RTNVGtexture *textures;
      int ctextures = rtnvg__maxi(rt->ntextures + 1, 4) +
                      rt->ctextures / 2; // 1.5x Overallocate
      textures = (RTNVGtexture *)realloc(rt->textures,
                                         sizeof(RTNVGtexture) * ctextures);
      if (textures == NULL)
        return NULL;
      rt->textures = textures;
      rt->ctextures = ctextures;
    }
    tex = &rt->textures[rt->ntextures++];
  }

  memset(tex, 0, sizeof(*tex));
  tex->id = ++rt->textureId;

  return tex;
}

static RTNVGtexture *rtnvg__findTexture(RTNVGcontext *rt, int id) {
  int i;
  for (i = 0; i < rt->ntextures; i++)
    if (rt->textures[i].id == id)
      return &rt->textures[i];
  return NULL;
}

static int rtnvg__deleteTexture(RTNVGcontext *rt, int id) {
  int i;
  for (i = 0; i < rt->ntextures; i++) {
    if (rt->textures[i].id == id) {
      if (rt->textures[i].tex != 0 &&
          (rt->textures[i].flags & NVG_IMAGE_NODELETE) == 0) {
        free(rt->textures[i].dataBase);
        // glDeleteTextures(1, &rt->textures[i].tex);
      }
      memset(&rt->textures[i], 0, sizeof(rt->textures[i]));
      return 1;
    }
  }
  return 0;
}





static int rtnvg__renderCreate(void *uptr) {
  RTNVGcontext *rt = (RTNVGcontext *)uptr;
  int align = 4;



//  rt->fragSize = sizeof(RTNVGfragUniforms) + align - sizeof(RTNVGfragUniforms) % align;
  rt->fragSize = (int)((sizeof(RTNVGfragUniforms) + align - 1) / align) * align;


  return 1;
}
static int rtnvg__renderCreateTexture(void *uptr, int type, int w, int h,
                                      int imageFlags,
                                      const unsigned char *data) {
  // printf("createTexture\n");
  RTNVGcontext *rt = (RTNVGcontext *)uptr;
  RTNVGtexture *tex = rtnvg__allocTexture(rt);

  if (tex == NULL)
    return 0;

  // glGenTextures(1, &tex->tex);
  tex->width = w;
  tex->height = h;
  tex->type = type;
  tex->flags = imageFlags;

  // Retain texture image.
  int components = 1;
  if (tex->type == NVG_TEXTURE_RGBA) {
    components = 4;
  }
  tex->data = (unsigned char *)malloc(tex->width * tex->height * components);
  if (data != NULL) {
    memcpy(tex->data, data, tex->width * tex->height * components);
  }

  // @todo { mip mapping }


  return tex->id;
}



static int rtnvg__renderDeleteTexture(void *uptr, int image) {
  RTNVGcontext *rt = (RTNVGcontext *)uptr;
  return rtnvg__deleteTexture(rt, image);
}
  static int rtnvg__renderUpdateTexture(void *uptr, int image, int x, int y,
                                        int w, int h, const unsigned char *data) {
    RTNVGcontext *rt = (RTNVGcontext *)uptr;
    RTNVGtexture *tex = rtnvg__findTexture(rt, image);
    if (!tex || !tex->data || !data) return 0;

    const int comps = (tex->type == NVG_TEXTURE_RGBA) ? 4 : 1;

    // Clamp destination rect to texture bounds
    int x0 = x, y0 = y, w0 = w, h0 = h;
    if (x0 < 0) { w0 += x0; x0 = 0; }
    if (y0 < 0) { h0 += y0; y0 = 0; }
    if (x0 + w0 > tex->width)  w0 = tex->width  - x0;
    if (y0 + h0 > tex->height) h0 = tex->height - y0;
    if (w0 <= 0 || h0 <= 0) return 1;

    const int dstStride = tex->width * comps;
    const int srcStride = tex->width * comps; // source is full atlas
    const unsigned char *srcBase = data + y0 * srcStride + x0 * comps;
    unsigned char *dstBase = tex->data + y0 * dstStride + x0 * comps;

    for (int row = 0; row < h0; ++row) {
      memcpy(dstBase + row * dstStride,
             srcBase + row * srcStride,
             (size_t)w0 * (size_t)comps);
    }

    return 1;
  }




static int rtnvg__renderGetTextureSize(void *uptr, int image, int *w, int *h) {
  RTNVGcontext *rt = (RTNVGcontext *)uptr;
  RTNVGtexture *tex = rtnvg__findTexture(rt, image);
  if (tex == NULL)
    return 0;
  *w = tex->width;
  *h = tex->height;
  return 1;
}

static void rtnvg__xformToMat3x4(float *m3, float *t) {
  m3[0] = t[0];
  m3[1] = t[1];
  m3[2] = 0.0f;
  m3[3] = 0.0f;
  m3[4] = t[2];
  m3[5] = t[3];
  m3[6] = 0.0f;
  m3[7] = 0.0f;
  m3[8] = t[4];
  m3[9] = t[5];
  m3[10] = 1.0f;
  m3[11] = 0.0f;
}

static NVGcolor rtnvg__premulColor(NVGcolor c) {
  c.r *= c.a;
  c.g *= c.a;
  c.b *= c.a;
  return c;
}

static int rtnvg__convertPaint(RTNVGcontext *rt, RTNVGfragUniforms *frag,
                               NVGpaint *paint, NVGscissor *scissor,
                               float width, float fringe, float strokeThr) {
  // printf("convertPaint\n");
  RTNVGtexture *tex = NULL;
  float invxform[6];

  memset(frag, 0, sizeof(*frag));

  frag->innerCol = rtnvg__premulColor(paint->innerColor);
  frag->outerCol = rtnvg__premulColor(paint->outerColor);

  if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f) {
    memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
    frag->scissorExt[0] = 1.0f;
    frag->scissorExt[1] = 1.0f;
    frag->scissorScale[0] = 1.0f;
    frag->scissorScale[1] = 1.0f;
  } else {
    nvgTransformInverse(invxform, scissor->xform);
    rtnvg__xformToMat3x4(frag->scissorMat, invxform);
    frag->scissorExt[0] = scissor->extent[0];
    frag->scissorExt[1] = scissor->extent[1];
    frag->scissorScale[0] = sqrtf(scissor->xform[0] * scissor->xform[0] +
                                  scissor->xform[2] * scissor->xform[2]) /
                            fringe;
    frag->scissorScale[1] = sqrtf(scissor->xform[1] * scissor->xform[1] +
                                  scissor->xform[3] * scissor->xform[3]) /
                            fringe;
  }

  memcpy(frag->extent, paint->extent, sizeof(frag->extent));
  frag->strokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
  frag->strokeThr = strokeThr;

  if (paint->image != 0) {
    tex = rtnvg__findTexture(rt, paint->image);
    if (tex == NULL)
      return 0;
    if ((tex->flags & NVG_IMAGE_FLIPY) != 0) {
      float flipped[6];
      nvgTransformScale(flipped, 1.0f, -1.0f);
      nvgTransformMultiply(flipped, paint->xform);
      nvgTransformInverse(invxform, flipped);
    } else {
      nvgTransformInverse(invxform, paint->xform);
    }
    frag->type = NSVG_SHADER_FILLIMG;

    if (tex->type == NVG_TEXTURE_RGBA)
      frag->texType = (tex->flags & NVG_IMAGE_PREMULTIPLIED) ? 0.0f : 1.0f;
    else
      frag->texType = 2;
    LOGGER_RT("frag->texType = %f tex->flags & NVG_IMAGE_PREMULTIPLIED=%d\n", frag->texType,tex->flags & NVG_IMAGE_PREMULTIPLIED);

  } else {
    frag->type = NSVG_SHADER_FILLGRAD;
    frag->radius = paint->radius;
    frag->feather = paint->feather;
    nvgTransformInverse(invxform, paint->xform);
  }

  rtnvg__xformToMat3x4(frag->paintMat, invxform);

  return 1;
}

static RTNVGfragUniforms *nvg__fragUniformPtr(RTNVGcontext *rt, int i);


static void rtnvg__renderViewport(void *uptr, float width, float height, float devicePixelRatio) {
  (void)devicePixelRatio;
  RTNVGcontext *rt = (RTNVGcontext *)uptr;
  rt->view[0] = width;
  rt->view[1] = height;
}

static float rtnvg__sdroundrect(float pt[2], float ext[2], float rad) {
  float ext2[2];
  ext2[0] = ext[0] - rad;
  ext2[1] = ext[1] - rad;
  float d[2];
  d[0] = fabsf(pt[0]) - ext2[0];
  d[1] = fabsf(pt[1]) - ext2[1];
  // printf("d = %f, %f\n", d[0], d[1]);
  float max_d[2];
  max_d[0] = (d[0] < 0.0f) ? 0.0f : d[0];
  max_d[1] = (d[1] < 0.0f) ? 0.0f : d[1];

  float d_val = (d[0] > d[1]) ? d[0] : d[1];
  d_val = (d_val < 0.0f) ? d_val : 0.0f;
  // printf("d_val = %f\n", d_val);

  float val = d_val + sqrtf(max_d[0] * max_d[0] + max_d[1] * max_d[1]) - rad;
  // printf("val = %f\n", val);

  return val;
}

static unsigned char ftouc(float x) {
  int i = (int)(x * 255.0f);
  i = (i < 0) ? 0 : i;
  i = (i > 255) ? 255 : i;
  return (unsigned char)i;
}

static float uctof(unsigned char x) { return (float)x / 255.0f; }

static float fclamp(float x, float minval, float maxval) {
  float y = (x < minval) ? minval : x;
  y = (y > maxval) ? maxval : y;
  return y;
}
static void rtnvg__alphaBlend(unsigned char *dst, const float col[4]) {
  // @todo { linear space compisition? }

  float d0 = uctof(dst[0]);
  float d1 = uctof(dst[1]);
  float d2 = uctof(dst[2]);
  float d3 = uctof(dst[3]);

  float alpha = fclamp(col[3], 0.0f, 1.0f);
  float r0 = col[0] + d0 * (1.0f - alpha);
  float r1 = col[1] + d1 * (1.0f - alpha);
  float r2 = col[2] + d2 * (1.0f - alpha);
  float r3 = col[3] + d3 * (1.0f - alpha);

  dst[0] = ftouc(r0);
  dst[1] = ftouc(r1);
  dst[2] = ftouc(r2);
  dst[3] = ftouc(r3);
}
static float rtnvg__scissorMask(float scissorMat[12], float scissorExt[2],
                                float scissorScale[2], float x, float y) {

  //(abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);
  //	sc = vec2(0.5,0.5) - sc * scissorScale;
  //	return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);

  float pp[2];
  pp[0] = scissorMat[0] * x + scissorMat[4] * y + scissorMat[8];
  pp[1] = scissorMat[1] * x + scissorMat[5] * y + scissorMat[9];

  float sc[2];
  sc[0] = fabsf(pp[0]) - scissorExt[0];
  sc[1] = fabsf(pp[1]) - scissorExt[1];

  sc[0] = 0.5f - sc[0] * scissorScale[0];
  sc[1] = 0.5f - sc[1] * scissorScale[1];

  return fclamp(sc[0], 0.0f, 1.0f) * fclamp(sc[1], 0.0f, 1.0f);
}

#if 1


#include "common_raster.h"
static void rtnvg__shade(float color[4], RTNVGcontext *rt,
                         RTNVGfragUniforms *frag, float x, float y, float tu,
                         float tv, int imageId) {


  float scissor = rtnvg__scissorMask(frag->scissorMat, frag->scissorExt,
                                     frag->scissorScale, x, y);
  float pt[2];
  pt[0] = frag->paintMat[0] * x + frag->paintMat[4] * y + frag->paintMat[8];
  pt[1] = frag->paintMat[1] * x + frag->paintMat[5] * y + frag->paintMat[9];
  color[0] = color[1] = color[2] = 0;
  color[3] = 1.0f;

  int type = (int)frag->type;
  if (type == 0) { // grad fill
                   // Calculate gradient color using box gradient

    // printf("feather = %f, rad = %f\n", frag->feather, frag->radius);
    // printf("extent = %f, %f\n", frag->extent[0], frag->extent[1]);
    float d = fclamp((rtnvg__sdroundrect(pt, frag->extent, frag->radius) +
                      frag->feather * 0.5f) /
                         (float)frag->feather,
                     0.0f, 1.0f);
    // printf("d = %f\n", (rtnvg__sdroundrect(pt, frag->extent, frag->radius) +
    // frag->feather*0.5f) / (float)frag->feather);
    color[0] = frag->innerCol.r * (1.0f - d) + frag->outerCol.r * d;
    color[1] = frag->innerCol.g * (1.0f - d) + frag->outerCol.g * d;
    color[2] = frag->innerCol.b * (1.0f - d) + frag->outerCol.b * d;
    color[3] = frag->innerCol.a * (1.0f - d) + frag->outerCol.a * d;

    float strokeAlpha = 1.0f; // @fixme.
    color[0] *= strokeAlpha * scissor;
    color[1] *= strokeAlpha * scissor;
    color[2] *= strokeAlpha * scissor;
    color[3] *= strokeAlpha * scissor;
  } else if (type == 1) { // Image
                          // Calculate color from texture

    int texType = (int)frag->texType;
    RTNVGtexture *tex = rtnvg__findTexture(rt, imageId);
LOGGER_RT("shade: type=%d texType=%d tex->type=%d flags=%d\n",
       (int)frag->type, (int)frag->texType, tex->type, tex->flags);
    TextureSampler sampler;
    int components = 1;
    if (tex->type == NVG_TEXTURE_RGBA) {
      components = 4;
    }
    sampler.Set(tex->data, tex->width, tex->height, components,
                TextureSampler::FORMAT_BYTE);

    float tcol[4];
 float su = pt[0] / frag->extent[0];
  float sv = pt[1] / frag->extent[1];

  if (texType == 2 && tex->type == NVG_TEXTURE_ALPHA) {
    unsigned char a = sample_alpha_nearest(tex, su, sv);
    float fa = a * (1.0f / 255.0f);
    tcol[0] = fa;
    tcol[1] = fa;
    tcol[2] = fa;
    tcol[3] = fa;
  } else {
    sampler.fetch(tcol, su, sv);
  }



if (texType == 2) { // alpha only
  color[0] = frag->innerCol.r * tcol[0];
  color[1] = frag->innerCol.g * tcol[0];
  color[2] = frag->innerCol.b * tcol[0];
  color[3] = frag->innerCol.a * tcol[0];
} else if (texType == 0) {
  // premultiplied texture: tcol.rgb already multiplied by tcol.a
  color[0] = frag->innerCol.r * tcol[0];
  color[1] = frag->innerCol.g * tcol[1];
  color[2] = frag->innerCol.b * tcol[2];
  color[3] = frag->innerCol.a * tcol[3];
} else { // texType == 1 (or default): straight alpha texture
  color[0] = frag->innerCol.r * tcol[0] * tcol[3];
  color[1] = frag->innerCol.g * tcol[1] * tcol[3];
  color[2] = frag->innerCol.b * tcol[2] * tcol[3];
  color[3] = frag->innerCol.a * tcol[3];
}




    float strokeAlpha = 1.0f; // @fixme.
    color[0] *= strokeAlpha * scissor;
    color[1] *= strokeAlpha * scissor;
    color[2] *= strokeAlpha * scissor;
    color[3] *= strokeAlpha * scissor;

  } else if (type == 3) { // textured tri
    int texType = (int)frag->texType;

    RTNVGtexture *tex = rtnvg__findTexture(rt, imageId);
LOGGER_RT("shade: type=%d texType=%d tex->type=%d flags=%d\n",
       (int)frag->type, (int)frag->texType, tex->type, tex->flags);
    TextureSampler sampler;
    int components = 1;
    if (tex->type == NVG_TEXTURE_RGBA) {
      components = 4;
    }
    sampler.Set(tex->data, tex->width, tex->height, components,
                TextureSampler::FORMAT_BYTE);
    // printf("texId = %d, data = %p, w = %d\n", rt->textureId, tex->data,
    // tex->width);
    float tcol[4];
  // nanovg_rt.h, in type==3 (textured tri) path
  if (texType == 2 && tex->type == NVG_TEXTURE_ALPHA) {
    unsigned char a = sample_alpha_nearest(tex, tu, tv);
    float fa = a * (1.0f / 255.0f);
    tcol[0] = fa;
    tcol[1] = fa;
    tcol[2] = fa;
    tcol[3] = fa;
  } else {
    sampler.fetch(tcol, tu, tv);
  }


if (texType == 2) { // alpha only
LOGGER_RT("rtnvg__shade x=%f y=%f tu=%f tv=%f imageId=%d\n", x,  y,  tu, tv,  imageId);
  color[0] = frag->innerCol.r * tcol[0];
  color[1] = frag->innerCol.g * tcol[0];
  color[2] = frag->innerCol.b * tcol[0];
  color[3] = frag->innerCol.a * tcol[0];
    for(int i=0;i<4;++i)
        LOGGER_RT("rtnvg__shade tcol[%d]=%f\n",i,tcol[i]);


//tu,tv min/max seen for a glyph draw

//a few tcol[0] 
 
} else if (texType == 0) {
  // premultiplied texture: tcol.rgb already multiplied by tcol.a
  color[0] = frag->innerCol.r * tcol[0];
  color[1] = frag->innerCol.g * tcol[1];
  color[2] = frag->innerCol.b * tcol[2];
  color[3] = frag->innerCol.a * tcol[3];
} else { // texType == 1 (or default): straight alpha texture
  color[0] = frag->innerCol.r * tcol[0] * tcol[3];
  color[1] = frag->innerCol.g * tcol[1] * tcol[3];
  color[2] = frag->innerCol.b * tcol[2] * tcol[3];
  color[3] = frag->innerCol.a * tcol[3];
}

 float strokeAlpha = 1.0f; // @fixme.
    color[0] *= strokeAlpha * scissor;
    color[1] *= strokeAlpha * scissor;
    color[2] *= strokeAlpha * scissor;
    color[3] *= strokeAlpha * scissor;
  }
}
#endif
struct RTNVGpath;
struct RTNVGcontext;
struct RTNVGfragUniforms;

#include "common_raster.h"


static void rtnvg__fill(RTNVGcontext *rt, RTNVGcall *call) {
  RTNVGpath *paths = &rt->paths[call->pathOffset];
  int npaths = call->pathCount;
LOGGER_RT("rtnvg__fill pathCount=%d\n",npaths);
for (int p=0; p<npaths; ++p) {
    int off = paths[p].fillOffset;
    int cnt = paths[p].fillCount;
    double A = 0.0;
    for (int i=0; i<cnt; ++i) {
      const NVGvertex& v0 = rt->verts[off+i];
      const NVGvertex& v1 = rt->verts[off+((i+1)%cnt)];
      A += double(v0.x) * double(v1.y) - double(v1.x) * double(v0.y);
    }
  LOGGER_RT("path[%d] fillCount=%d winding=%d Area=%f\n", p, paths[p].fillCount, paths[p].winding,A);
}
int ss = (rt->flags & NVG_ANTIALIAS) ? 2 : 1;
RTNVGfragUniforms* fragFill = nvg__fragUniformPtr(rt, call->uniformOffset + rt->fragSize);
if ((int)fragFill->type != NSVG_SHADER_SIMPLE) {
    ss = 4; // higher SS only for gradients/images
  }
auto edgeTable = buildEdgeTableFP_FromContours(rt, paths, npaths, rt->height, ss);


scanlineFillFP_CoverageAndShade(rt, call->image, rt->width, rt->height, edgeTable, fragFill, rt->pixels, ss);


}
static void rtnvg__convexFill(RTNVGcontext *rt, RTNVGcall *call) {
  RTNVGpath* paths = &rt->paths[call->pathOffset];
  int npaths = call->pathCount;


  RTNVGfragUniforms* frag0 = nvg__fragUniformPtr(rt, call->uniformOffset);

  RTNVGfragUniforms* frag1 = nvg__fragUniformPtr(rt, call->uniformOffset + rt->fragSize);

  LOGGER_RT("convexFill frag0->type=%f frag1->type=%f image=%d\n", frag0->type, frag1->type, call->image);
RTNVGfragUniforms*fragPaint;
  if ((int)frag0->type == NSVG_SHADER_SIMPLE && (int)frag1->type != NSVG_SHADER_SIMPLE) 
    fragPaint = frag1;
  else
    fragPaint=frag0; 

  int ss = (rt->flags & NVG_ANTIALIAS) ? 2 : 1;
  if ((int)fragPaint->type != NSVG_SHADER_SIMPLE) ss = 4;
  auto edgeTable = buildEdgeTableFP_FromContours(rt, paths, npaths, rt->height, ss);

  scanlineFillFP_CoverageAndShade(rt, call->image, rt->width, rt->height,

                                 edgeTable, fragPaint, rt->pixels, ss);
}

static inline float edgeFn(float ax, float ay, float bx, float by, float px, float py) {
  return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

// Raster one triangle into cov[] with 2x2 MSAA if supersample==2 else 1 sample.
static inline void rasterTriCov(
    unsigned char* cov, int width, int height,
    float x0,float y0, float x1,float y1, float x2,float y2,
    int supersample,
    int minx,int miny,int maxx,int maxy)
{
  int spp = (supersample == 2) ? 4 : 1;
  const float sx4[4] = {0.25f, 0.75f, 0.25f, 0.75f};
  const float sy4[4] = {0.25f, 0.25f, 0.75f, 0.75f};

  float area = edgeFn(x0,y0,x1,y1,x2,y2);
  if (!std::isfinite(area) || area == 0.0f) return;

  // Make CCW
  if (area < 0.0f) { std::swap(x1,x2); std::swap(y1,y2); }

  const float eps = -1e-7f;

  for (int py = miny; py < maxy; ++py) {
    int row = py * width;
    for (int px = minx; px < maxx; ++px) {
      int covered = 0;
      for (int s = 0; s < spp; ++s) {
        float fx = px + (supersample == 2 ? sx4[s] : 0.5f);
        float fy = py + (supersample == 2 ? sy4[s] : 0.5f);

        float e01 = edgeFn(x0,y0,x1,y1,fx,fy);
        float e12 = edgeFn(x1,y1,x2,y2,fx,fy);
        float e20 = edgeFn(x2,y2,x0,y0,fx,fy);

        if (e01 >= eps && e12 >= eps && e20 >= eps) covered++;
      }
      if (!covered) continue;

      int add = (covered * 255 + spp/2) / spp;
      int idx = row + px;
      int v = cov[idx] + add;
      cov[idx] = (unsigned char)(v > 255 ? 255 : v);
    }
  }
}
static void rtnvg__stroke(RTNVGcontext *rt, RTNVGcall *call) {
  RTNVGpath *paths = &rt->paths[call->pathOffset];
  int npaths = call->pathCount;
  LOGGER_RT("rtnvg__stroke %d\n",npaths);
  unsigned char* rgba = rt->pixels;
  RTNVGfragUniforms* frag = nvg__fragUniformPtr(rt, call->uniformOffset);
int ss = (rt->flags & NVG_ANTIALIAS) ? 2 : 1;  // 4 is worth it for strokes

// Compute bbox of stroke strip to keep cov small (important)
float xmin=1e30f, ymin=1e30f, xmax=-1e30f, ymax=-1e30f;
for (int k = 0; k < npaths; ++k) {
  LOGGER_RT("rtnvg__stroke path[%d] fillCount=%d winding=%d \n", k, paths[k].fillCount, paths[k].winding);
  int off = paths[k].strokeOffset;
  int cnt = paths[k].strokeCount;
  for (int i = 0; i < cnt; ++i) {
    const NVGvertex& v = rt->verts[off + i];
    xmin = std::min(xmin, v.x);
    ymin = std::min(ymin, v.y);
    xmax = std::max(xmax, v.x);
    ymax = std::max(ymax, v.y);
  }
}
int minx = std::max(0, (int)floorf(xmin) - 2);
int miny = std::max(0, (int)floorf(ymin) - 2);
int maxx = std::min(rt->width,  (int)ceilf (xmax) + 2);
int maxy = std::min(rt->height, (int)ceilf (ymax) + 2);
if (minx >= maxx || miny >= maxy) return;

int bw = maxx - minx;
int bh = maxy - miny;
std::vector<unsigned char> cov(bw * bh, 0);

// Rasterize each triangle into cov (with coordinates shifted by bbox origin)
for (int k = 0; k < npaths; ++k) {
  int off = paths[k].strokeOffset;
  int cnt = paths[k].strokeCount;
  if (cnt < 3) continue;

  for (int i = 0; i + 2 < cnt; ++i) {
    const NVGvertex& a0 = rt->verts[off + i + 0];
    const NVGvertex& b0 = rt->verts[off + i + 1];
    const NVGvertex& c0 = rt->verts[off + i + 2];

    // strip winding
    float ax, ay, bx, by, cx, cy;
    if (i & 1) { ax = b0.x; ay = b0.y; bx = a0.x; by = a0.y; cx = c0.x; cy = c0.y; }
    else       { ax = a0.x; ay = a0.y; bx = b0.x; by = b0.y; cx = c0.x; cy = c0.y; }

    cr_rasterize_one_triangle_cov(
      bw, bh,
      ax - minx, ay - miny,
      bx - minx, by - miny,
      cx - minx, cy - miny,
      cov.data(),
      ss
    );
  }
}

// One blend pass
unsigned char srcR, srcG, srcB, srcA;
cr_solidPremulBytesFromFrag(frag, srcR, srcG, srcB, srcA);
for (int y = 0; y < bh; ++y) {
  int dstRow = (miny + y) * rt->width + minx;
  int srcRow = y * bw;
  float py = (miny + y) + 0.5f;

  for (int x = 0; x < bw; ++x) {
    unsigned char c = cov[srcRow + x];
    if (!c) continue;

    float px = (minx + x) + 0.5f;

    // Apply scissor (same logic as shader path)
    float sm = rtnvg__scissorMask(frag->scissorMat, frag->scissorExt, frag->scissorScale, px, py);
    if (sm <= 0.0f) continue;

    int cc = (int)lroundf((float)c * sm);
    if (cc <= 0) continue;
    if (cc > 255) cc = 255;

    unsigned char* dst = &rgba[4 * (dstRow + x)];
    cr_blendSolidPremulCov(dst, srcR, srcG, srcB, srcA, (unsigned char)cc);
  }
}

  LOGAR_RT("end rtnvg__stroke");
}






#include "common_raster.h"

static void rtnvg__triangles(RTNVGcontext *rt, RTNVGcall *call) {

  unsigned char* rgba = rt->pixels;
  RTNVGfragUniforms* frag = nvg__fragUniformPtr(rt, call->uniformOffset);

  int off = call->triangleOffset;
  int cnt = call->triangleCount;
LOGGER_RT("rtnvg__triangles off=%d cnt=%d call->image=%d\n",off,cnt,call->image);
if ( (int)frag->texType == 2) {
//if ((int)frag->type == 3 && (int)frag->texType == 2) {
  for (int i=0; i+2<cnt; i+=3) {
    const NVGvertex& v0 = rt->verts[off+i+0];
    const NVGvertex& v1 = rt->verts[off+i+1];
    const NVGvertex& v2 = rt->verts[off+i+2];
    drawTexturedTri_Text(rt, frag, call->image, v0, v1, v2);
  }
  return;
}


  int ss = (rt->flags & NVG_ANTIALIAS) ? 2 : 1;
// If this TRIANGLES call is a gradient fill, do coverage-first path to avoid seams.
  if (call->image == 0) {
      unsigned char srcR, srcG, srcB, srcA;
      cr_solidPremulBytesFromFrag(frag, srcR, srcG, srcB, srcA);
    // SOLID fast path

    for (int i = 0; i + 2 < cnt; i += 3) {
      const NVGvertex& v0 = rt->verts[off + i + 0];
      const NVGvertex& v1 = rt->verts[off + i + 1];
      const NVGvertex& v2 = rt->verts[off + i + 2];

      cr_rasterize_one_triangle_solid(rt->width, rt->height,
                                      v0.x, v0.y, v1.x, v1.y, v2.x, v2.y,
                                      rgba, srcR, srcG, srcB, srcA, ss);
    }




    return;
  }

RTNVGtexture* tex = (call->image != 0) ? rtnvg__findTexture(rt, call->image) : NULL;
if(!tex)
    return;
  const float eps = -1e-7f; 
constexpr const  auto edge = [](float ax,float ay,float bx,float by,float px,float py){
    return (px-ax)*(by-ay) - (py-ay)*(bx-ax);
  };

if (tex->type == NVG_TEXTURE_ALPHA && tex->data) {

  unsigned char srcR, srcG, srcB, srcA;
  cr_solidPremulBytesFromFrag(frag, srcR, srcG, srcB, srcA);

  for (int i = 0; i + 2 < cnt; i += 3) {
    NVGvertex &v0 = rt->verts[off + i + 0];
    NVGvertex &v1 = rt->verts[off + i + 1];
    NVGvertex &v2 = rt->verts[off + i + 2];

    float area = edge(v0.x,v0.y, v1.x,v1.y, v2.x,v2.y);
    if (!std::isfinite(area) || area == 0.0f) continue;
    if (area < 0.0f) { std::swap(v1, v2); area = -area; }

    int minx = std::max(0, (int)floorf(std::min({v0.x,v1.x,v2.x})));
    int maxx = std::min(rt->width,  (int)ceilf (std::max({v0.x,v1.x,v2.x})));
    int miny = std::max(0, (int)floorf(std::min({v0.y,v1.y,v2.y})));
    int maxy = std::min(rt->height, (int)ceilf (std::max({v0.y,v1.y,v2.y})));
    if (minx >= maxx || miny >= maxy) continue;

    float invA = 1.0f / area;

    for (int y = miny; y < maxy; ++y) {
      for (int x = minx; x < maxx; ++x) {
        float px = x + 0.5f, py = y + 0.5f;

        float w0 = edge(v1.x,v1.y, v2.x,v2.y, px,py);
        float w1 = edge(v2.x,v2.y, v0.x,v0.y, px,py);
        float w2 = edge(v0.x,v0.y, v1.x,v1.y, px,py);
        if (w0 < eps || w1 < eps || w2 < eps) continue;

        w0 *= invA; w1 *= invA; w2 *= invA;

        float tu = w0*v0.u + w1*v1.u + w2*v2.u;
        float tv = w0*v0.v + w1*v1.v + w2*v2.v;

//        unsigned char mask = sample_alpha_bilinear(tex, tu, tv);
        unsigned char mask = sample_alpha_nearest(tex, tu, tv);

        if (!mask) continue;

        unsigned char* dst = &rgba[4 * (y * rt->width + x)];
        cr_blendSolidPremulCov(dst, srcR, srcG, srcB, srcA, mask);
      }
    }
  }
  return;
}

// Fallback for non-alpha textures/patterns:
for (int i = 0; i + 2 < cnt; i += 3) {
  NVGvertex &v0 = rt->verts[off + i + 0];
  NVGvertex &v1 = rt->verts[off + i + 1];
  NVGvertex &v2 = rt->verts[off + i + 2];

  float area = edge(v0.x,v0.y, v1.x,v1.y, v2.x,v2.y);
  if (!std::isfinite(area) || area == 0.0f) continue;
  if (area < 0.0f) { std::swap(v1, v2); area = -area; }

  int minx = std::max(0, (int)floorf(std::min({v0.x,v1.x,v2.x})));
  int maxx = std::min(rt->width,  (int)ceilf (std::max({v0.x,v1.x,v2.x})));
  int miny = std::max(0, (int)floorf(std::min({v0.y,v1.y,v2.y})));
  int maxy = std::min(rt->height, (int)ceilf (std::max({v0.y,v1.y,v2.y})));
  if (minx >= maxx || miny >= maxy) continue;

  float invA = 1.0f / area;



  for (int y = miny; y < maxy; ++y) {
    for (int x = minx; x < maxx; ++x) {
      float px = x + 0.5f, py = y + 0.5f;

      float w0 = edge(v1.x,v1.y, v2.x,v2.y, px,py);
      float w1 = edge(v2.x,v2.y, v0.x,v0.y, px,py);
      float w2 = edge(v0.x,v0.y, v1.x,v1.y, px,py);
      if (w0 < eps || w1 < eps || w2 < eps) continue;


      w0 *= invA; w1 *= invA; w2 *= invA;

      float tu = w0*v0.u + w1*v1.u + w2*v2.u;
      float tv = w0*v0.v + w1*v1.v + w2*v2.v;

      float col[4];
      rtnvg__shade(col, rt, frag, px, py, tu, tv, call->image);
      rtnvg__alphaBlend(&rgba[4 * (y * rt->width + x)], col);
    }
  }
}

}
static void rtnvg__renderCancel(void *uptr) {
  // printf("__renderCancel\n");
  RTNVGcontext *rt = (RTNVGcontext *)uptr;
  rt->nverts = 0;
  rt->npaths = 0;
  rt->ncalls = 0;
  rt->nuniforms = 0;
}



static void rtnvg__renderFlush(void *uptr) {
  RTNVGcontext *rt = (RTNVGcontext *)uptr;

  LOGGER_RT("renderFlush: fragSize=%d sizeof(RTNVGfragUniforms)=%zu nuniforms=%d\n",
       rt->fragSize, sizeof(RTNVGfragUniforms), rt->nuniforms);

  if (rt->ncalls > 0) {
    for (int i = 0; i < rt->ncalls; i++) {
      RTNVGcall *call = &rt->calls[i];
      switch(call->type) {
           case RTNVG_FILL:  {
               rtnvg__fill(rt, call);};
               break;
           case  RTNVG_CONVEXFILL: rtnvg__convexFill(rt, call);break;
           case RTNVG_STROKE: rtnvg__stroke(rt, call);break;
           case RTNVG_TRIANGLES: rtnvg__triangles(rt, call);break;
         }
      }
    }

  // Reset calls
  rt->nverts = 0;
  rt->npaths = 0;
  rt->ncalls = 0;
  rt->nuniforms = 0;

  // Guard check for pixel buffer overruns.
  if (rt->pixelsBase) {
    const size_t guard = 16;
    for (size_t k = 0; k < guard; ++k) {
      if (rt->pixelsBase[k] != 0xCD ||
          rt->pixelsBase[guard + rt->pixelsSize + k] != 0xCD) {
        LOGGER_RT("PIXELS GUARD CORRUPTED at k=%zu\n", k);
        break;
      }
    }
  }
  // Guard check for verts buffer overruns.
  if (rt->vertsBase) {
    const size_t guard = 16;
    for (size_t k = 0; k < guard; ++k) {
      if (rt->vertsBase[rt->vertsSizeBytes + k] != 0xCD) {
        LOGGER_RT("VERTS GUARD CORRUPTED at k=%zu\n", k);
        break;
      }
    }
  }
  // Guard check for uniforms buffer overruns.
  if (rt->uniformsBase) {
    const size_t guard = 16;
    for (size_t k = 0; k < guard; ++k) {
      if (rt->uniformsBase[rt->uniformsSizeBytes + k] != 0xCD) {
        LOGGER_RT("UNIFORMS GUARD CORRUPTED at k=%zu\n", k);
        break;
      }
    }
  }
}

static int rtnvg__maxVertCount(const NVGpath *paths, int npaths) {
  int i, count = 0;
  for (i = 0; i < npaths; i++) {
    count += paths[i].nfill;
    count += paths[i].nstroke;
  }
  return count;
}

static RTNVGcall *rtnvg__allocCall(RTNVGcontext *rt) {
  RTNVGcall *ret = NULL;
  if (rt->ncalls + 1 > rt->ccalls) {
    RTNVGcall *calls;
    int ccalls =
        rtnvg__maxi(rt->ncalls + 1, 128) + rt->ccalls / 2; // 1.5x Overallocate
    calls = (RTNVGcall *)realloc(rt->calls, sizeof(RTNVGcall) * ccalls);
    if (calls == NULL)
      return NULL;
    rt->calls = calls;
    rt->ccalls = ccalls;
  }
  ret = &rt->calls[rt->ncalls++];
  memset(ret, 0, sizeof(RTNVGcall));
  return ret;
}

static int rtnvg__allocPaths(RTNVGcontext *rt, int n) {
  int ret = 0;
  if (rt->npaths + n > rt->cpaths) {
    RTNVGpath *paths;
    int cpaths =
        rtnvg__maxi(rt->npaths + n, 128) + rt->cpaths / 2; // 1.5x Overallocate
    paths = (RTNVGpath *)realloc(rt->paths, sizeof(RTNVGpath) * cpaths);
    if (paths == NULL)
      return -1;
    rt->paths = paths;
    rt->cpaths = cpaths;
  }
  ret = rt->npaths;
  rt->npaths += n;
  return ret;
}

static int rtnvg__allocVerts(RTNVGcontext *rt, int n) {
  // printf("alloc verts: %d\n", n);
  int ret = 0;
  if (rt->nverts + n > rt->cverts) {
    NVGvertex *verts;
    int cverts =
        rtnvg__maxi(rt->nverts + n, 4096) + rt->cverts / 2; // 1.5x Overallocate
    const size_t guard = 16;
    size_t bytes = (size_t)cverts * sizeof(NVGvertex);
    verts = (NVGvertex *)realloc(rt->vertsBase ? rt->vertsBase : (unsigned char*)rt->verts, bytes + guard);
    if (verts == NULL)
      return -1;
    rt->vertsBase = (unsigned char*)verts;
    rt->verts = verts;
    rt->vertsSizeBytes = bytes;
    memset(rt->vertsBase + bytes, 0xCD, guard);
    rt->cverts = cverts;
  }
  ret = rt->nverts;
  rt->nverts += n;
  return ret;
}

static int rtnvg__allocFragUniforms(RTNVGcontext *rt, int n) {
  int ret = 0, structSize = rt->fragSize;
  if (rt->nuniforms + n > rt->cuniforms) {
    unsigned char *uniforms;
    int cuniforms = rtnvg__maxi(rt->nuniforms + n, 128) +
                    rt->cuniforms / 2; // 1.5x Overallocate
    const size_t guard = 16;
    size_t bytes = (size_t)structSize * (size_t)cuniforms;
    uniforms = (unsigned char *)realloc(rt->uniformsBase ? rt->uniformsBase : rt->uniforms, bytes + guard);
    if (uniforms == NULL)
      return -1;
    rt->uniformsBase = uniforms;
    rt->uniforms = uniforms;
    rt->uniformsSizeBytes = bytes;
    memset(rt->uniformsBase + bytes, 0xCD, guard);
    rt->cuniforms = cuniforms;
  }
  ret = rt->nuniforms * structSize;
  rt->nuniforms += n;
  return ret;
}
static inline RTNVGfragUniforms* nvg__fragUniformPtr(RTNVGcontext* rt, int offsetBytes) {
  return (RTNVGfragUniforms*)(rt->uniforms + offsetBytes);
}

static void rtnvg__vset(NVGvertex *vtx, float x, float y, float u, float v) {
  // printf("v: (x, y, u, v) = %f, %f, %f, %f\n", x, y, u, v);
  vtx->x = x;
  vtx->y = y;
  vtx->u = u;
  vtx->v = v;
}

static void rtnvg__renderFill(void *uptr, NVGpaint *paint, NVGcompositeOperationState compositeOperation, NVGscissor *scissor,
                              float fringe, const float *bounds,
                              const NVGpath *paths, int npaths) {
  LOGAR_RT("__renderFill");
  RTNVGcontext *rt = (RTNVGcontext *)uptr;
  RTNVGcall *call = rtnvg__allocCall(rt);
  NVGvertex *quad;
  RTNVGfragUniforms *frag;
  int i, maxverts, offset;

  if (call == NULL)
    return;

  call->type = RTNVG_FILL;
  call->pathOffset = rtnvg__allocPaths(rt, npaths);
  if (call->pathOffset == -1)
    goto error;
  call->pathCount = npaths;
  call->image = paint->image;
  // TODO(syoyo): Implement
  // call->blendFunc = glnvg_blendCompositeOperation(compositeOperation)
  (void)compositeOperation;

  LOGGER_RT("pathOffset = %d\n", call->pathOffset);

  if (npaths == 1 && paths[0].convex)
    call->type = RTNVG_CONVEXFILL;

  // Allocate vertices for all the paths.
  maxverts = rtnvg__maxVertCount(paths, npaths) + 6;
  // printf("maxverts = %d\n", maxverts);
  offset = rtnvg__allocVerts(rt, maxverts);
  if (offset == -1)
    goto error;



  LOGGER_RT("npaths = %d\n", npaths);
  for (i = 0; i < npaths; i++) {
    RTNVGpath *copy = &rt->paths[call->pathOffset + i];
    const NVGpath *path = &paths[i];
    memset(copy, 0, sizeof(RTNVGpath));
    copy->winding = path->winding;
    if (path->nfill > 0) {
      copy->fillOffset = offset;
      copy->fillCount = path->nfill;
      memcpy(&rt->verts[offset], path->fill, sizeof(NVGvertex) * path->nfill);
      offset += path->nfill;
    }
    LOGGER_RT("nstroke=%d\n",path->nstroke);
    if(path->nstroke > 0) {
    #if 0
          for (int k = 0; k < std::min(path->nstroke, 8); k++) {
                   LOGGER_RT("src stroke[%d] x=%f y=%f u=%f v=%f\n", k,
                   path->stroke[k].x, path->stroke[k].y,
                   path->stroke[k].u, path->stroke[k].v);
                }
        #endif

      copy->strokeOffset = offset;
      copy->strokeCount = path->nstroke;
      memcpy(&rt->verts[offset], path->stroke,
             sizeof(NVGvertex) * path->nstroke);
      offset += path->nstroke;
    }
  }

  // Quad
  call->triangleOffset = offset;
  call->triangleCount = 6;
  quad = &rt->verts[call->triangleOffset];
  rtnvg__vset(&quad[0], bounds[0], bounds[3], 0.5f, 1.0f);
  rtnvg__vset(&quad[1], bounds[2], bounds[3], 0.5f, 1.0f);
  rtnvg__vset(&quad[2], bounds[2], bounds[1], 0.5f, 1.0f);

  rtnvg__vset(&quad[3], bounds[0], bounds[3], 0.5f, 1.0f);
  rtnvg__vset(&quad[4], bounds[2], bounds[1], 0.5f, 1.0f);
  rtnvg__vset(&quad[5], bounds[0], bounds[1], 0.5f, 1.0f);

  // Setup uniforms for draw calls
  if (call->type == RTNVG_FILL) {
    call->uniformOffset = rtnvg__allocFragUniforms(rt, 2);
    if (call->uniformOffset == -1)
      goto error;
    // Simple shader for stencil
    frag = nvg__fragUniformPtr(rt, call->uniformOffset);
    memset(frag, 0, sizeof(*frag));
    frag->strokeThr = -1.0f;
    frag->type = NSVG_SHADER_SIMPLE;
    // Fill shader
    rtnvg__convertPaint(
        rt, nvg__fragUniformPtr(rt, call->uniformOffset + rt->fragSize), paint,
        scissor, fringe, fringe, -1.0f);
  } else {
    call->uniformOffset = rtnvg__allocFragUniforms(rt, 1);
    if (call->uniformOffset == -1)
      goto error;
    // Fill shader
    rtnvg__convertPaint(rt, nvg__fragUniformPtr(rt, call->uniformOffset), paint,
                        scissor, fringe, fringe, -1.0f);
  }

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (rt->ncalls > 0)
    rt->ncalls--;
}

static void rtnvg__renderStroke(void *uptr, NVGpaint *paint,
                                NVGcompositeOperationState compositeOperation,
                                NVGscissor *scissor, float fringe,
                                float strokeWidth, const NVGpath *paths,
                                int npaths) {
  LOGGER_RT("renderStroke npaths=%d\n",npaths);
  RTNVGcontext *rt = (RTNVGcontext *)uptr;
  RTNVGcall *call = rtnvg__allocCall(rt);
  int i, maxverts, offset;

  if (call == NULL)
    return;
  call->strokeWidth = strokeWidth;

  call->type = RTNVG_STROKE;
  call->pathOffset = rtnvg__allocPaths(rt, npaths);
  if (call->pathOffset == -1)
    goto error;
  call->pathCount = npaths;
  call->image = paint->image;
  // TODO(syoyo): Implement
  // call->blendFunc = glnvg_blendCompositeOperation(compositeOperation)
  (void)compositeOperation;

  // Allocate vertices for all the paths.
  maxverts = rtnvg__maxVertCount(paths, npaths);
  offset = rtnvg__allocVerts(rt, maxverts);
  if (offset == -1)
    goto error;


  for (i = 0; i < npaths; i++) {
    RTNVGpath *copy = &rt->paths[call->pathOffset + i];
    const NVGpath *path = &paths[i];
    memset(copy, 0, sizeof(RTNVGpath));
    copy->winding = path->winding;
//    copy->winding = (path->winding == NVG_CCW) ? +1 : -1;
    LOGGER_RT("renderStroke: fringe=%f nstroke=%d\n", fringe, path->nstroke);
    if (path->nstroke) {
    #if 0
        for (int k = 0; k < std::min(path->nstroke, 8); k++) {
            LOGGER_RT("renderStroke: src stroke[%d] x=%f y=%f u=%f v=%f\n", k, path->stroke[k].x, path->stroke[k].y,
                 path->stroke[k].u, path->stroke[k].v);
            }
    #endif
      copy->strokeOffset = offset;
      copy->strokeCount = path->nstroke;
      memcpy(&rt->verts[offset], path->stroke,
             sizeof(NVGvertex) * path->nstroke);
      offset += path->nstroke;
    }
  }

  if (rt->flags & NVG_STENCIL_STROKES) {
    // Fill shader
    call->uniformOffset = rtnvg__allocFragUniforms(rt, 2);
    if (call->uniformOffset == -1)
      goto error;

    rtnvg__convertPaint(rt, nvg__fragUniformPtr(rt, call->uniformOffset), paint,
                        scissor, strokeWidth, fringe, -1.0f);
    rtnvg__convertPaint(
        rt, nvg__fragUniformPtr(rt, call->uniformOffset + rt->fragSize), paint,
        scissor, strokeWidth, fringe, 1.0f - 0.5f / 255.0f);

  } else {
    // Fill shader
    call->uniformOffset = rtnvg__allocFragUniforms(rt, 1);
    if (call->uniformOffset == -1)
      goto error;
    rtnvg__convertPaint(rt, nvg__fragUniformPtr(rt, call->uniformOffset), paint,
                        scissor, strokeWidth, fringe, -1.0f);
  }

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (rt->ncalls > 0)
    rt->ncalls--;
}

static void rtnvg__renderTriangles(void *uptr, NVGpaint *paint,
                                   NVGcompositeOperationState compositeOperation,
                                   NVGscissor *scissor, const NVGvertex *verts,
                                   int nverts,float fringeWidge) {
  // printf("renderTriangles\n");
  RTNVGcontext *rt = (RTNVGcontext *)uptr;
  RTNVGcall *call = rtnvg__allocCall(rt);
  RTNVGfragUniforms *frag;

  if (call == NULL)
    return;

  call->type = RTNVG_TRIANGLES;
  call->image = paint->image;
  // TODO(syoyo): Implement
  // call->blendFunc = glnvg_blendCompositeOperation(compositeOperation)
  (void)compositeOperation;

  // Allocate vertices for all the paths.
  call->triangleOffset = rtnvg__allocVerts(rt, nverts);
  if (call->triangleOffset == -1)
    goto error;
  call->triangleCount = nverts;

  memcpy(&rt->verts[call->triangleOffset], verts, sizeof(NVGvertex) * nverts);

  // Fill shader
  call->uniformOffset = rtnvg__allocFragUniforms(rt, 1);
  if (call->uniformOffset == -1)
    goto error;
  frag = nvg__fragUniformPtr(rt, call->uniformOffset);
  rtnvg__convertPaint(rt, frag, paint, scissor, 1.0f, 1.0f, -1.0f);
  frag->type = NSVG_SHADER_IMG;

  return;

error:
  // We get here if call alloc was ok, but something else is not.
  // Roll back the last call to prevent drawing it.
  if (rt->ncalls > 0)
    rt->ncalls--;
}

static void rtnvg__renderDelete(void *uptr) {
  // printf("__renderDelete\n");
  RTNVGcontext *rt = (RTNVGcontext *)uptr;
  int i;
  if (rt == NULL)
    return;


  // if (rt->vertBuf != 0)
  //	glDeleteBuffers(1, &rt->vertBuf);

  for (i = 0; i < rt->ntextures; i++) {
    // if (rt->textures[i].tex != 0 && (rt->textures[i].flags &
    // NVG_IMAGE_NODELETE) == 0)
    //	glDeleteTextures(1, &rt->textures[i].tex);
  }
  free(rt->textures);

  free(rt->paths);
  free(rt->vertsBase);
  free(rt->uniformsBase);
  free(rt->calls);

  delete rt;
}

NVGcontext *nvgCreateRT(int flags, int w, int h) {
  NVGcontext *ctx = NULL;


   RTNVGcontext *rt=new RTNVGcontext{.flags = flags, .width = w,.height=h};
  if (rt == NULL)
        return NULL;
//  memset(rt, 0, sizeof(RTNVGcontext));

  NVGparams params;
  memset(&params, 0, sizeof(params));
  params.renderCreate = rtnvg__renderCreate;
  params.renderCreateTexture = rtnvg__renderCreateTexture;
  params.renderDeleteTexture = rtnvg__renderDeleteTexture;
  params.renderUpdateTexture = rtnvg__renderUpdateTexture;
  params.renderGetTextureSize = rtnvg__renderGetTextureSize;
  params.renderViewport = rtnvg__renderViewport;
  params.renderCancel = rtnvg__renderCancel;
  params.renderFlush = rtnvg__renderFlush;
  params.renderFill = rtnvg__renderFill;
  params.renderStroke = rtnvg__renderStroke;
  params.renderTriangles = rtnvg__renderTriangles;
  params.renderDelete = rtnvg__renderDelete;
  params.userPtr = rt;
  params.edgeAntiAlias = flags & NVG_ANTIALIAS ? 1 : 0;

  rt->pixelsSize = (size_t)rt->width * (size_t)rt->height * 4u;
  const size_t guard = 16;
  rt->pixelsBase = (unsigned char *)malloc(rt->pixelsSize + guard * 2);
  rt->pixels = rt->pixelsBase + guard;
  memset(rt->pixelsBase, 0xCD, guard);
  memset(rt->pixelsBase + guard + rt->pixelsSize, 0xCD, guard);
  for (size_t i = 0; i < rt->width * rt->height; i++) {
    rt->pixels[4 * i + 0] = 0;
    rt->pixels[4 * i + 1] = 0;
    rt->pixels[4 * i + 2] = 0;
    rt->pixels[4 * i + 3] = 255;
  }

  ctx = nvgCreateInternal(&params);

  LOGGER_RT("nvgCreateRT: fragSize=%d sizeof(RTNVGfragUniforms)=%zu nuniforms=%d\n",
       rt->fragSize, sizeof(RTNVGfragUniforms), rt->nuniforms);
  if (ctx == NULL)
    goto error;

  return ctx;

error:
  free(rt->pixelsBase);
  // 'gl' is freed by nvgDeleteInternal.
  if (ctx != NULL)
    nvgDeleteInternal(ctx);
  return NULL;
}

void nvgDeleteRT(NVGcontext *ctx) {
  RTNVGcontext *rt = (RTNVGcontext *)nvgInternalParams(ctx)->userPtr;
  free(rt->pixelsBase);
  // printf("delete\n");
  nvgDeleteInternal(ctx);
}

int nvglCreateImageFromHandle(NVGcontext *ctx, unsigned int textureId, int w,
                              int h, int imageFlags) {
  RTNVGcontext *rt = (RTNVGcontext *)nvgInternalParams(ctx)->userPtr;
  RTNVGtexture *tex = rtnvg__allocTexture(rt);

  printf("Not supported.\n");
  exit(-1);
  if (tex == NULL)
    return 0;

  tex->type = NVG_TEXTURE_RGBA;
  tex->tex = textureId;
  tex->flags = imageFlags;
  tex->width = w;
  tex->height = h;

  return tex->id;
}

unsigned int nvglImageHandle(NVGcontext *ctx, int image) {
  RTNVGcontext *rt = (RTNVGcontext *)nvgInternalParams(ctx)->userPtr;
  RTNVGtexture *tex = rtnvg__findTexture(rt, image);
  return tex->tex;
}

void nvgClearBackgroundRT(NVGcontext *ctx, float r, float g, float b, float a) {
  RTNVGcontext *rt = (RTNVGcontext *)nvgInternalParams(ctx)->userPtr;
  unsigned char red = ftouc(r);
  unsigned char green = ftouc(g);
  unsigned char blue = ftouc(b);
  unsigned char alpha = ftouc(a);
  for (size_t i = 0; i < rt->width * rt->height; i++) {
    rt->pixels[4 * i + 0] = red;
    rt->pixels[4 * i + 1] = green;
    rt->pixels[4 * i + 2] = blue;
    rt->pixels[4 * i + 3] = alpha;
  }
}

unsigned char *nvgReadPixelsRT(NVGcontext *ctx) {
  RTNVGcontext *rt = (RTNVGcontext *)nvgInternalParams(ctx)->userPtr;
  return rt->pixels;
}

#endif /* NANOVG_RT_IMPLEMENTATION */
