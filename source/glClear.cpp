#include "limeGl.h"

#include "gpu.h"
#include "vertex.h"
#include <memory>

#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace lime;

extern "C" {

GL_API void GL_APIENTRY glClear(GLbitfield mask) {
  int writeMask = 0;
  if (mask & GL_COLOR_BUFFER_BIT)
    writeMask |= GPU_WRITE_COLOR;
  if (mask & GL_DEPTH_BUFFER_BIT)
    writeMask |= GPU_WRITE_DEPTH;

  const auto &cc = g_currentContext->m_clearColor;
  const auto cd = -g_currentContext->m_clearDepth;

  GPU_SetViewport(reinterpret_cast<u32 *>(osConvertVirtToPhys(
                      g_currentContext->draw_target(GL_DEPTH_BUFFER_BIT))),
                  reinterpret_cast<u32 *>(osConvertVirtToPhys(
                      g_currentContext->draw_target(GL_COLOR_BUFFER_BIT))),
                  0, 0, g_currentContext->m_width, g_currentContext->m_height);

  GPU_DepthMap(-1.f, .0f);

  GPU_SetFaceCulling(GPU_CULL_NONE);
  GPU_SetStencilTest(false, GPU_NEVER, 0, 0xff, 0);
  GPU_SetStencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_KEEP);
  GPU_SetBlendingColor(0, 0, 0, 0);
  GPU_SetDepthTestAndWriteMask(true, GPU_ALWAYS, static_cast<GPU_WRITEMASK>(writeMask));

  GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST1, 0x1, 0);
  GPUCMD_AddWrite(GPUREG_EARLYDEPTH_TEST2, 0);

  GPU_SetAlphaBlending(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA,
                       GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA,
                       GPU_ONE_MINUS_SRC_ALPHA);
  GPU_SetAlphaTest(false, GPU_ALWAYS, 0);

  for (int i = 0; i < 4; ++i) {
    g_currentContext->m_clearVbo.get()[i].~Vertex3D();
  }
  constexpr auto wone = glm::vec4(.0f, .0f, .0f, 1.f);
  // clang-format off
  new (g_currentContext->m_clearVbo.get()) Vertex3D[4] {
      {glm::vec3(-1.f, -1.f, cd), cc, wone, wone},
      {glm::vec3( 1.f, -1.f, cd), cc, wone, wone},
      {glm::vec3(-1.f,  1.f, cd), cc, wone, wone},
      {glm::vec3( 1.f,  1.f, cd), cc, wone, wone},
  };
  // clang-format on

  GPU_SetTexEnv(
      0,
      GPU_TEVSOURCES(GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
      GPU_TEVSOURCES(GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
      GPU_TEVOPERANDS(0, 0, 0), GPU_TEVOPERANDS(0, 0, 0), GPU_REPLACE,
      GPU_REPLACE, 0xffffffff);
  GPU_SetDummyTexEnv(1);
  GPU_SetDummyTexEnv(2);
  GPU_SetDummyTexEnv(3);
  GPU_SetDummyTexEnv(4);
  GPU_SetDummyTexEnv(5);

  shaderProgramUse(&g_currentContext->m_shader);

  auto idt = glm::identity<glm::mat4>();

  GPU_SetFloatUniform(
      GPU_VERTEX_SHADER,
      shaderInstanceGetUniformLocation(g_currentContext->m_shader.vertexShader,
                                       "projection"),
      reinterpret_cast<u32 *>(glm::value_ptr(idt)), 4);
  GPU_SetFloatUniform(GPU_VERTEX_SHADER,
                      shaderInstanceGetUniformLocation(
                          g_currentContext->m_shader.vertexShader, "modelview"),
                      reinterpret_cast<u32 *>(glm::value_ptr(idt)), 4);

  DrawPrimitives(GPU_TRIANGLE_STRIP, g_currentContext->m_clearVbo, 0, 4);

  g_currentContext->m_dirty.value = -1;
}

GL_API void GL_APIENTRY glClearColor(GLfloat red, GLfloat green, GLfloat blue,
                                     GLfloat alpha) {
  g_currentContext->m_clearColor.r = _clampf(red);
  g_currentContext->m_clearColor.g = _clampf(green);
  g_currentContext->m_clearColor.b = _clampf(blue);
  g_currentContext->m_clearColor.a = _clampf(alpha);
}

#ifdef LIME_GLES

GL_API void GL_APIENTRY glClearColorx(GLfixed red, GLfixed green, GLfixed blue,
                                      GLfixed alpha) {
  g_currentContext->m_clearColor.r = _clampf(red / 65536.f);
  g_currentContext->m_clearColor.g = _clampf(green / 65536.f);
  g_currentContext->m_clearColor.b = _clampf(blue / 65536.f);
  g_currentContext->m_clearColor.a = _clampf(alpha / 65536.f);
}

GL_API void GL_APIENTRY glClearDepthx(GLfixed d) {
  g_currentContext->m_clearDepth = _clampf(d / 65536.f);
}

GL_API void GL_APIENTRY glClearDepthf(GLfloat d) {
  g_currentContext->m_clearDepth = _clampf(d);
}

#else

GLAPI void GLAPIENTRY glClearDepth(GLclampd depth) {
  g_currentContext->m_clearDepth = depth;
}

#endif
}
