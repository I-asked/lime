#include "limeGl.h"

#include "gpu.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

using namespace lime;

extern "C" {

#ifdef LIME_GLES

GL_API void GL_APIENTRY glLoadMatrixx(const GLfixed *m) {
  float mf[16];
  std::uninitialized_copy(m, m + 16, mf);

  g_currentContext->current_matrix().top() = glm::make_mat4(mf) / 65536.f;
}

GL_API void GL_APIENTRY glMultMatrixx(const GLfixed *m) {
  float mf[16];
  std::uninitialized_copy(m, m + 16, mf);

  g_currentContext->current_matrix().top() *= glm::make_mat4(mf) / 65536.f;
}

#else

GL_API void GL_APIENTRY glLoadMatrixd(const GLdouble *m) {
  float mf[16];
  std::uninitialized_copy(m, m + 16, mf);

  g_currentContext->current_matrix().top() = glm::make_mat4(mf);
}

GL_API void GL_APIENTRY glMultMatrixd(const GLdouble *m) {
  float mf[16];
  std::uninitialized_copy(m, m + 16, mf);

  g_currentContext->current_matrix().top() *= glm::make_mat4(mf);
}

#endif

GL_API void GL_APIENTRY glLoadMatrixf(const GLfloat *m) {
  g_currentContext->current_matrix().top() = glm::make_mat4(m);
}

GL_API void GL_APIENTRY glMultMatrixf(const GLfloat *m) {
  g_currentContext->current_matrix().top() *= glm::make_mat4(m);
}

GL_API void GL_APIENTRY glPopMatrix(void) {
  g_currentContext->current_matrix().pop();
}

GL_API void GL_APIENTRY glPushMatrix(void) {
  g_currentContext->current_matrix().push(g_currentContext->m_projection.top());
}

GL_API void GL_APIENTRY glMatrixMode(GLenum mode) {
  switch (mode) {
  case GL_PROJECTION:
  case GL_MODELVIEW:
  case GL_TEXTURE:
    g_currentContext->m_matrixMode = mode;
    break;
  default:
    setErrorGL(GL_INVALID_ENUM);
  }
}
}
