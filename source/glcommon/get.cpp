#include "lime.h"

GL_API const GLubyte *GL_APIENTRY glGetString(GLenum name) {
  switch (name) {
  case GL_VENDOR:
    return "JulaDDR, et al."_ucs;
#ifdef LIME_GLES
  case GL_RENDERER:
    return "LimES"_ucs;
  case GL_VERSION:
    return "OpenGL ES 1.1 (Lime)"_ucs;
  case GL_EXTENSIONS:
    return ""_ucs;
#else // LIME_GLES
  case GL_RENDERER:
    return "Lime"_ucs;
  case GL_VERSION:
    return "1.1 (Lime)"_ucs;
  case GL_EXTENSIONS:
    return ""_ucs;
#endif // LIME_GLES
  default:
    lime::setErrorGL(GL_INVALID_ENUM);
    return nullptr;
  }
}
