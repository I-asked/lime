#include "gfb.h"

#include <cstddef>

extern "C" {

u8 *gfbTopFramebuffers[2];
u8 *gfbBottomFramebuffers[2];
u32 gfbTopFramebufferMaxSize;
u32 gfbBottomFramebufferMaxSize;
GSPGPU_FramebufferFormat gfbFramebufferFormats[2];

gfbTopMode_t gfbTopMode;
bool gfbIsVram;
u8 gfbCurBuf[2];
u8 gfbIsDoubleBuf[2];

void (*screenFree)(void *);
void *(*screenAlloc)(size_t);

}
