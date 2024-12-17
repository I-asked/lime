#pragma once

#include "macros.h"
#include "utils.h"
#include "vertex.h"

#include "gpu.h"

#include <3ds.h>
#include <glm/ext/vector_float4.hpp>

#ifdef LIME_GLES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GL/gl.h>
#include <GL/glext.h>
#endif

namespace lime {

extern shaderProgram_s g_shader;
extern DVLB_s *g_dvlb;

void global_init();

_inline static void SetAttributeBuffers(u8 totalAttributes, u32 *baseAddress,
                                        u64 attributeFormats, u16 attributeMask,
                                        u64 attributePermutation) {
  u32 bufferOffsets[]{0};
  GPU_SetAttributeBuffers(totalAttributes, baseAddress, attributeFormats,
                          attributeMask, attributePermutation, 1,
                          bufferOffsets, &attributePermutation,
                          &totalAttributes);
}

struct LimeGLContext {
  LimeGLContext(unsigned width, unsigned height, GPU_COLORBUF cbf,
                GPU_DEPTHBUF dbf)
      : m_side(GFX_LEFT), m_colorBufferFormat(cbf), m_depthBufferFormat(dbf),
        m_error(GL_NO_ERROR), m_clearDepth(0.f), m_width(width),
        m_height(height) {
    global_init();

    m_bufLeft = reinterpret_cast<u32 *>(vramAlloc(width * height * 4));
    m_dbufLeft = reinterpret_cast<u32 *>(vramAlloc(width * height * 4));
    //GX_MemoryFill(
    //  m_bufLeft,  0, &m_bufLeft[width*height],  GX_FILL_TRIGGER | GX_FILL_32BIT_DEPTH,
    //  m_dbufLeft, 0, &m_dbufLeft[width*height], GX_FILL_TRIGGER | GX_FILL_32BIT_DEPTH);
    //gspWaitForPSC0();
  }

  _inline u32 *draw_target(GLenum typ = GL_COLOR_BUFFER_BIT) {
    if (typ == GL_COLOR_BUFFER_BIT) {
      return (m_side == GFX_RIGHT) ? m_bufRight : m_bufLeft;
    }
    if (typ == GL_DEPTH_BUFFER_BIT) {
      return (m_side == GFX_RIGHT) ? m_dbufRight : m_dbufLeft;
    }
    return nullptr;
  }

  gfx3dSide_t m_side;

  GPU_COLORBUF m_colorBufferFormat;
  GPU_DEPTHBUF m_depthBufferFormat;

  GLenum m_error;
  GLfloat m_clearDepth;
  glm::vec4 m_clearColor;

  unsigned m_width, m_height;

  u32 *m_bufLeft;
  u32 *m_dbufLeft;
  u32 *m_bufRight;
  u32 *m_dbufRight;
};

struct LimeDevice final {
  LimeDevice(gfxScreen_t screen) : m_screen(screen) {}

  _inline void set_context(LimeGLContext *ctx) { m_context = ctx; }
  _inline LimeGLContext *context() { return m_context; }

  _inline u32 *lcd_buffer() {
    return reinterpret_cast<u32 *>(
        gfxGetFramebuffer(m_screen, m_context->m_side, nullptr, nullptr));
  }

  _inline u32 lcd_dimensions() const {
    switch (m_screen) {
    case GFX_TOP:
      return GX_BUFFER_DIM(240, 400);
    case GFX_BOTTOM:
      return GX_BUFFER_DIM(240, 320);
    default:
      return 0;
    }
  }

  _inline u32 lcd_height() const {
    switch (m_screen) {
    case GFX_TOP:
      return 400;
    case GFX_BOTTOM:
      return 320;
    default:
      return 0;
    }
  }

  _inline u32 lcd_width() const { return 240; }

  void flush();

private:
  gfxScreen_t m_screen;
  LimeGLContext *m_context;
};

extern LimeGLContext *g_currentContext;

_inline static void setErrorGL(GLenum error) {
  if (g_currentContext->m_error == GL_NO_ERROR) {
    g_currentContext->m_error = error;
  }
}

} // namespace lime
