#pragma once

#include "utils.h"

#include <3ds.h>

#pragma GCC poison gfxBottomFramebufferMaxSize gfxBottomFramebuffers
#pragma GCC poison gfxConfigScreen gfxCurBuf gfxExit gfxFlushBuffers gfxFramebufferFormats
#pragma GCC poison gfxGetFramebuffer gfxGetScreenFormat gfxInit gfxInitDefault gfxIs3D
#pragma GCC poison gfxIsDoubleBuf gfxIsVram gfxIsWide gfxPresentFramebuffer
#pragma GCC poison gfxScreenSwapBuffers gfxSet3D gfxSetDoubleBuffering gfxSetScreenFormat
#pragma GCC poison gfxSetWide gfxSwapBuffers gfxSwapBuffersGpu gfxTopFramebufferMaxSize
#pragma GCC poison gfxTopFramebuffers gfxTopMode

#ifdef __cplusplus
extern "C" {
#endif

extern u8 *gfbTopFramebuffers[2];
extern u8 *gfbBottomFramebuffers[2];
extern u32 gfbTopFramebufferMaxSize;
extern u32 gfbBottomFramebufferMaxSize;
extern GSPGPU_FramebufferFormat gfbFramebufferFormats[2];

extern enum gfbTopMode_t {
  MODE_2D = 0,
  MODE_3D = 1,
  MODE_WIDE = 2,
} gfbTopMode;
extern bool gfbIsVram;
extern u8 gfbCurBuf[2];
extern u8 gfbIsDoubleBuf[2];

extern void (*screenFree)(void *);
extern void *(*screenAlloc)(size_t);

_inline static void gfbSet3D(bool enable) {
  gfbTopMode = enable ? MODE_3D : MODE_2D;
}

_inline static bool gfbIs3D(void) { return gfbTopMode == MODE_3D; }

_inline static void gfbSetWide(bool enable) {
  gfbTopMode = enable ? MODE_WIDE : MODE_2D;
}

_inline static bool gfbIsWide(void) { return gfbTopMode == MODE_WIDE; }

_inline static void gfbSetScreenFormat(gfxScreen_t screen,
                                       GSPGPU_FramebufferFormat format) {
  u32 reqSize = GSP_SCREEN_WIDTH * gspGetBytesPerPixel(format);
  u8 **framebuffers;
  u32 *maxSize;

  if (screen == GFX_TOP) {
    reqSize *= GSP_SCREEN_HEIGHT_TOP_2X;
    framebuffers = gfbTopFramebuffers;
    maxSize = &gfbTopFramebufferMaxSize;
  } else // GFX_BOTTOM
  {
    reqSize *= GSP_SCREEN_HEIGHT_BOTTOM;
    framebuffers = gfbBottomFramebuffers;
    maxSize = &gfbBottomFramebufferMaxSize;
  }

  if (*maxSize < reqSize) {
    if (framebuffers[0])
      screenFree(framebuffers[0]);
    if (framebuffers[1])
      screenFree(framebuffers[1]);
    framebuffers[0] = (u8 *)screenAlloc(reqSize);
    framebuffers[1] = (u8 *)screenAlloc(reqSize);
    *maxSize = reqSize;
  }

  gfbFramebufferFormats[screen] = format;
}

_inline static GSPGPU_FramebufferFormat gfbGetScreenFormat(gfxScreen_t screen) {
  return gfbFramebufferFormats[screen];
}

_inline static void gfbSetDoubleBuffering(gfxScreen_t screen, bool enable) {
  gfbIsDoubleBuf[screen] =
      enable ? 1 : 0; // make sure they're the integer values '1' and '0'
}

_inline static void gfbPresentFramebuffer(gfxScreen_t screen, u8 id,
                                          bool hasStereo) {
  u32 stride =
      GSP_SCREEN_WIDTH * gspGetBytesPerPixel(gfbFramebufferFormats[screen]);
  u32 mode = gfbFramebufferFormats[screen];

  const u8 *fb_a, *fb_b;
  if (screen == GFX_TOP) {
    fb_a = gfbTopFramebuffers[id];
    switch (gfbTopMode) {
    default:
    case MODE_2D:
      mode |= BIT(6);
      fb_b = fb_a;
      break;
    case MODE_3D:
      mode |= BIT(5);
      fb_b = hasStereo ? (fb_a + gfbTopFramebufferMaxSize / 2) : fb_a;
      break;
    case MODE_WIDE:
      fb_b = fb_a;
      break;
    }
  } else {
    fb_a = gfbBottomFramebuffers[id];
    fb_b = fb_a;
  }

  if (!gfbIsVram)
    mode |= 1 << 8;
  else
    mode |= 3 << 8;

  gspPresentBuffer(screen, id, fb_a, fb_b, stride, mode);
}

_inline static void gfbInit(GSPGPU_FramebufferFormat topFormat,
                            GSPGPU_FramebufferFormat bottomFormat,
                            bool vrambuffers) {
  if (vrambuffers) {
    screenAlloc = vramAlloc;
    screenFree = vramFree;
    gfbIsVram = true;
  } else {
    screenAlloc = linearAlloc;
    screenFree = linearFree;
    gfbIsVram = false;
  }

  // Initialize GSP
  gspInit();

  // Initialize configuration
  gfbSet3D(false);
  gfbSetScreenFormat(GFX_TOP, topFormat);
  gfbSetScreenFormat(GFX_BOTTOM, bottomFormat);
  gfbSetDoubleBuffering(GFX_TOP, true);
  gfbSetDoubleBuffering(GFX_BOTTOM, true);

  // Present the framebuffers
  gfbCurBuf[0] = gfbCurBuf[1] = 0;
  gfbPresentFramebuffer(GFX_TOP, 0, false);
  gfbPresentFramebuffer(GFX_BOTTOM, 0, false);

  // Wait for VBlank and turn the LCD on
  gspWaitForVBlank();
  GSPGPU_SetLcdForceBlack(0x0);
}

_inline static void gfbInitDefault(void) {
  gfbInit(GSP_BGR8_OES, GSP_BGR8_OES, false);
}

_inline static void gfbExit(void) {
  if (screenFree == NULL)
    return;

  if (gspHasGpuRight()) {
    // Wait for VBlank and turn the LCD off
    gspWaitForVBlank();
    GSPGPU_SetLcdForceBlack(0x1);
  }

  // Free framebuffers
  screenFree(gfbTopFramebuffers[0]);
  screenFree(gfbTopFramebuffers[1]);
  screenFree(gfbBottomFramebuffers[0]);
  screenFree(gfbBottomFramebuffers[1]);
  gfbTopFramebuffers[0] = gfbTopFramebuffers[1] = NULL;
  gfbBottomFramebuffers[0] = gfbBottomFramebuffers[1] = NULL;
  gfbTopFramebufferMaxSize = gfbBottomFramebufferMaxSize = 0;

  // Deinitialize GSP
  gspExit();

  screenFree = NULL;
}

_inline static u8 *gfbGetFramebuffer(gfxScreen_t screen, gfx3dSide_t side,
                                     u16 *width, u16 *height) {
  unsigned id = gfbCurBuf[screen] ^ gfbIsDoubleBuf[screen];
  unsigned scr_width = GSP_SCREEN_WIDTH;
  unsigned scr_height;
  u8 *fb;

  if (screen == GFX_TOP) {
    fb = gfbTopFramebuffers[id];
    scr_height = GSP_SCREEN_HEIGHT_TOP;
    switch (gfbTopMode) {
    default:
    case MODE_2D:
      break;
    case MODE_3D:
      if (side != GFX_LEFT)
        fb += gfbTopFramebufferMaxSize / 2;
      break;
    case MODE_WIDE:
      scr_height = GSP_SCREEN_HEIGHT_TOP_2X;
      break;
    }
  } else // GFX_BOTTOM
  {
    fb = gfbBottomFramebuffers[id];
    scr_height = GSP_SCREEN_HEIGHT_BOTTOM;
  }

  if (width)
    *width = scr_width;
  if (height)
    *height = scr_height;

  return fb;
}

_inline static void gfbFlushBuffers(void) {
  const u32 baseSize =
      GSP_SCREEN_WIDTH * gspGetBytesPerPixel(gfbGetScreenFormat(GFX_TOP));
  const u32 topSize = GSP_SCREEN_HEIGHT_TOP * baseSize;
  const u32 topSize2x = GSP_SCREEN_HEIGHT_TOP_2X * baseSize;
  const u32 bottomSize = GSP_SCREEN_HEIGHT_BOTTOM * baseSize;

  GSPGPU_FlushDataCache(gfbGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL),
                        gfbTopMode == MODE_WIDE ? topSize2x : topSize);
  if (gfbTopMode == MODE_3D)
    GSPGPU_FlushDataCache(gfbGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL),
                          topSize);
  GSPGPU_FlushDataCache(gfbGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL),
                        bottomSize);
}

_inline static void gfbScreenSwapBuffers(gfxScreen_t scr, bool hasStereo) {
  gfbCurBuf[scr] ^= gfbIsDoubleBuf[scr];
  gfbPresentFramebuffer(scr, gfbCurBuf[scr], hasStereo);
}

_inline static void gfbConfigScreen(gfxScreen_t scr) {
  gfbScreenSwapBuffers(scr, true);
}

_inline static void gfbSwapBuffers(void) {
  gfbScreenSwapBuffers(GFX_TOP, true);
  gfbScreenSwapBuffers(GFX_BOTTOM, true);
}

#ifdef __cplusplus
}
#endif
