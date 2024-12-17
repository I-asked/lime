#include "limeGl.h"

#include "gpu.h"

#include "vshader_shbin.h"

#define DISPLAY_TRANSFER_FLAGS                                                 \
  (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) |                       \
   GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

namespace lime {

shaderProgram_s g_shader;
DVLB_s *g_dvlb = nullptr;

void global_init() {
  if (!g_dvlb) {
    g_dvlb = DVLB_ParseFile(reinterpret_cast<u32 *>(const_cast<u8 *>(vshader_shbin)),
                            vshader_shbin_size);
    shaderProgramInit(&g_shader);
    shaderProgramSetVsh(&g_shader, g_dvlb->DVLE);
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
