#pragma once

#include "utils.h"
#include "gfb.h"

#include <3ds.h>

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

union Color {
  struct {
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
  };
  GLuint value;
};

struct LimeGLContext {
  LimeGLContext(GPU_COLORBUF cbf, GPU_DEPTHBUF dbf)
    : m_colorBufferFormat(cbf), m_depthBufferFormat(dbf) {}

  GPU_COLORBUF m_colorBufferFormat;
  GPU_DEPTHBUF m_depthBufferFormat;

  GLenum m_error;
  GLfloat m_clearDepth;
  Color m_clearColor;
};

struct LimeDevice final {
  _inline void set_context(LimeGLContext *ctx) { m_context = ctx; }
  _inline LimeGLContext *context() { return m_context; }

  void flush();

private:
  LimeGLContext *m_context;
};

extern LimeGLContext *g_currentContext;

_inline static void setErrorGL(GLenum error) {
  if (g_currentContext->m_error == GL_NO_ERROR) {
    g_currentContext->m_error = error;
  }
}

}
