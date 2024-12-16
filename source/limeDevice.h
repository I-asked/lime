#pragma once

#include "limeGl.h"

struct LimeDevice final {
  _inline void set_context(LimeGLContext *ctx) { m_context = ctx; }
  _inline LimeGLContext *context() { return m_context; }

  void flush();

private:
  LimeGLContext *m_context;
};
