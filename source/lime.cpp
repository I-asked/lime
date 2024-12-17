#include "lime.h"

#include "gpu.h"

#include "vshader_shbin.h"

#include <glm/gtc/type_ptr.hpp>

namespace lime {

LimeGLContext::LimeGLContext(unsigned width, unsigned height, GPU_COLORBUF cbf,
                             GPU_DEPTHBUF dbf)
    :
#ifndef LIME_NO_GL_ERROR_CHECKS
      m_error(GL_NO_ERROR),
#endif
      m_clearVbo(
          reinterpret_cast<Vertex3D *>(linearAlloc(sizeof(Vertex3D) * 4))),
      m_side(GFX_LEFT), m_colorBufferFormat(cbf), m_depthBufferFormat(dbf),
      m_matrixMode(GL_MODELVIEW), m_clearDepth(1.f), m_width(width),
      m_height(height) {
  m_bufLeft = reinterpret_cast<u32 *>(vramAlloc(width * height * 4));
  m_dbufLeft = reinterpret_cast<u32 *>(vramAlloc(width * height * 4));

  m_bufRight = nullptr;
  m_dbufRight = nullptr;

  m_dvlb =
      DVLB_ParseFile(reinterpret_cast<u32 *>(const_cast<u8 *>(vshader_shbin)),
                     vshader_shbin_size);
  shaderProgramInit(&m_shader);
  shaderProgramSetVsh(&m_shader, m_dvlb->DVLE);
}

void LimeGLContext::init3D() {
  if (!m_bufRight) {
    m_bufRight = reinterpret_cast<u32 *>(vramAlloc(m_width * m_height * 4));
    m_dbufRight = reinterpret_cast<u32 *>(vramAlloc(m_width * m_height * 4));
  }
}

void LimeDevice::flush() {
  GX_DisplayTransfer(m_context->draw_target(GL_COLOR_BUFFER_BIT),
                     GX_BUFFER_DIM(m_context->m_width, m_context->m_height),
                     lcd_buffer(), lcd_dimensions(),
                     GX_TRANSFER_IN_FORMAT(m_context->m_colorBufferFormat) |
                         GX_TRANSFER_OUT_FORMAT(gfxGetScreenFormat(m_screen)));
}
} // namespace lime
