#pragma once

#include "macros.h"
#include "utils.h"
#include "vertex.h"

#include "gpu.h"

#include <3ds.h>
#include <glm/ext/vector_float4.hpp>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <memory>

#ifdef LIME_GLES
#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <list>
#include <stack>

namespace lime {

using MatStack = std::stack<glm::mat4, std::list<glm::mat4>>;

_inline static void SetAttributeBuffers(u8 totalAttributes, u32 *baseAddress,
                                        u64 attributeFormats, u16 attributeMask,
                                        u64 attributePermutation) {
  u32 bufferOffsets[]{0};
  GPU_SetAttributeBuffers(totalAttributes, baseAddress, attributeFormats,
                          attributeMask, attributePermutation, 1, bufferOffsets,
                          &attributePermutation, &totalAttributes);
}

template <typename VboDeleter = linearFree_t>
_inline void DrawPrimitives(GPU_Primitive_t prim,
                            std::unique_ptr<Vertex3D, VboDeleter> &vertices,
                            u32 offset, u32 count) {
  SetAttributeBuffers(
      4, reinterpret_cast<u32 *>(osConvertVirtToPhys(vertices.get())),
      Vertex3D::format, 0xffc, 0x3210);
  GPU_DrawArray(prim, offset, count);
  GPU_FinishDrawing();
}

struct LimeGLContext {
  LimeGLContext(unsigned width, unsigned height, GPU_COLORBUF cbf,
                GPU_DEPTHBUF dbf);

  shaderProgram_s m_shader;
  DVLB_s *m_dvlb = nullptr;

  _inline u32 *draw_target(GLenum typ = GL_COLOR_BUFFER_BIT) {
    if (typ == GL_COLOR_BUFFER_BIT) {
      return (m_side == GFX_RIGHT) ? m_bufRight : m_bufLeft;
    }
    if (typ == GL_DEPTH_BUFFER_BIT) {
      return (m_side == GFX_RIGHT) ? m_dbufRight : m_dbufLeft;
    }
    return nullptr;
  }

  _inline MatStack &current_matrix() {
    switch (m_matrixMode) {
    case GL_PROJECTION:
      return m_projection;
    case GL_MODELVIEW:
      return m_modelview;
    case GL_TEXTURE:
      return m_texture;
    default:
      abort();
    }
  }

  void init3D();

#ifndef LIME_NO_GL_ERROR_CHECKS
  GLenum m_error;
#endif

  std::unique_ptr<Vertex3D, linearFree_t> m_clearVbo;

  gfx3dSide_t m_side;

  GPU_COLORBUF m_colorBufferFormat;
  GPU_DEPTHBUF m_depthBufferFormat;

  GLenum m_matrixMode;

  GLfloat m_clearDepth;
  glm::vec4 m_clearColor;

  unsigned m_width, m_height;

  MatStack m_projection, m_modelview, m_texture;

  u32 *m_bufLeft;
  u32 *m_dbufLeft;
  u32 *m_bufRight;
  u32 *m_dbufRight;

  union {
    struct {
      bool viewport : 1;
      bool scissor : 1;
      bool cull : 1;
      bool blend : 1;
      bool alpha : 1;
      bool depth : 1;
      bool texture : 1;
      bool texenv : 1;
      bool fog : 1;
      bool matrixmode : 1;
    };
    u32 value;
  } m_dirty;
  static_assert(sizeof(m_dirty) == sizeof(m_dirty.value),
                "m_dirty fits in u32");
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
#ifndef LIME_NO_GL_ERROR_CHECKS
  if (g_currentContext->m_error == GL_NO_ERROR) {
    g_currentContext->m_error = error;
  }
#else
  (void)error;
#endif
}

} // namespace lime
