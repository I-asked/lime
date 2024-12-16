#include "limeGl.h"

#include "gpu_old.h"

extern "C" {

GL_API void GL_APIENTRY glClear(GLbitfield mask) {
  if (mask | GL_COLOR_BUFFER_BIT) {
    _nop();
  }
}

GL_API void GL_APIENTRY glClearColor(GLfloat red, GLfloat green, GLfloat blue,
                                     GLfloat alpha) {
  lime::g_currentContext->m_clearColor.r = 255 * _clampf(red);
  lime::g_currentContext->m_clearColor.g = 255 * _clampf(green);
  lime::g_currentContext->m_clearColor.b = 255 * _clampf(blue);
  lime::g_currentContext->m_clearColor.a = 255 * _clampf(alpha);
}

#ifdef LIME_GLES
GL_API void GL_APIENTRY glClearDepthf(GLfloat d) {
  lime::g_currentContext->m_clearDepth = d;
}
#else
GLAPI void GLAPIENTRY glClearDepth(GLclampd depth) {
  lime::g_currentContext->m_clearDepth = depth;
}
#endif
}
