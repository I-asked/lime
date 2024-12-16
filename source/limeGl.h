#pragma once

#include "utils.h"

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

struct LimeGLContext {
  GLenum m_error;
};

extern LimeGLContext *g_currentContext;

_inline static void setErrorGL(GLenum error) {
  if (g_currentContext->m_error == GL_NO_ERROR) {
    g_currentContext->m_error = error;
  }
}
