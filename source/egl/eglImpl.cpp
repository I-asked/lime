#include "lime.h"
#include "macros.h"
#include "proctab.h"

#include <3ds.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace lime {

LimeGLContext *g_currentContext = nullptr;
} // namespace lime

using namespace lime;

namespace {
#define NUM_SCREENS (2)

#define GPU_CMD_SIZE (0x40000)

u32 *g_gpuCmd[2]{nullptr};
u8 g_activeGpuCmd = 0;

LightEvent g_swapEvent;

gxCmdQueue_s g_gxQueue;

LimeDevice *g_screens[NUM_SCREENS] = {nullptr, nullptr};

bool g_swappable[NUM_SCREENS] = {false};

void swap_buffers(void *arg) {
  (void)arg;
  if (g_swappable[GFX_TOP]) {
    gfxScreenSwapBuffers(GFX_TOP, gfxIs3D());
    g_swappable[GFX_TOP] = false;
  }
  if (g_swappable[GFX_BOTTOM]) {
    gfxScreenSwapBuffers(GFX_TOP, false);
    g_swappable[GFX_BOTTOM] = false;
  }
  LightEvent_Signal(&g_swapEvent);
}
} // namespace

extern "C" {

extern const unsigned long proctab_gl_sz;
extern proctab_entry *proctab_gl;

extern const unsigned long proctab_egl_sz;
extern proctab_entry *proctab_egl;

namespace {
#ifndef LIME_NO_EGL_ERROR_CHECKS
EGLint g_eglErrorFlag = EGL_SUCCESS;
#endif

_inline void setErrorEGL(EGLint error) {
#ifndef LIME_NO_EGL_ERROR_CHECKS
  if (g_eglErrorFlag == EGL_SUCCESS) {
    g_eglErrorFlag = error;
  }
#else
  (void)error;
#endif
}

const struct ColorConfig {
  khronos_uint8_t r, g, b, a;
  GPU_COLORBUF internal;
} g_colorConfigs[] = {
    {8, 8, 8, 8, GPU_RB_RGBA8},    {5, 6, 5, 0, GPU_RB_RGB565},
    {5, 5, 5, 1, GPU_RB_RGBA5551}, {4, 4, 4, 4, GPU_RB_RGBA4},
    {8, 8, 8, 0, GPU_RB_RGB8},     {0xff, 0xff, 0xff, 0xff, (GPU_COLORBUF)0},
};

const struct DepthConfig {
  khronos_uint8_t d, s;
  GPU_DEPTHBUF internal;
} g_depthConfigs[] = {
    {24, 8, GPU_RB_DEPTH24_STENCIL8},
    {16, 0, GPU_RB_DEPTH16},
    {24, 0, GPU_RB_DEPTH24},
    {0xff, 0xff, (GPU_DEPTHBUF)0},
};
} // namespace

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType displayId) {
  if (displayId >= NUM_SCREENS)
    return EGL_NO_DISPLAY;

  consoleDebugInit(debugDevice_SVC);

  return g_screens + displayId;
}

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major,
                                            EGLint *minor) {
  const EGLNativeDisplayType displayId = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (displayId >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  if (major)
    *major = 1;
  if (minor)
    *minor = 5;

  if (g_screens[displayId]) {
    return EGL_TRUE;
  }

  if (!(g_screens[0] || g_screens[1])) {
    gfxInit(GSP_RGBA8_OES, GSP_RGBA8_OES, false);
    g_gpuCmd[0] = reinterpret_cast<u32 *>(linearAlloc(GPU_CMD_SIZE * 4));
    g_gpuCmd[1] = reinterpret_cast<u32 *>(linearAlloc(GPU_CMD_SIZE * 4));

    GPU_Init(nullptr);
    GPU_Reset(nullptr, g_gpuCmd[0], GPU_CMD_SIZE);

    g_gxQueue.maxEntries = 32;
    g_gxQueue.entries = new gxCmdEntry_s[g_gxQueue.maxEntries];

    GX_BindQueue(&g_gxQueue);
    gxCmdQueueRun(&g_gxQueue);

    gspSetEventCallback(GSPGPU_EVENT_PPF, swap_buffers, nullptr, false);

    LightEvent_Init(&g_swapEvent, RESET_ONESHOT);
    LightEvent_Signal(&g_swapEvent);
  }

  g_screens[displayId] = new LimeDevice(displayId);

  return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy,
                                              const EGLint *attrib_list,
                                              EGLConfig *configs,
                                              EGLint config_size,
                                              EGLint *num_config) {
  const EGLNativeDisplayType displayId = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (displayId >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  int alpha_size = 0, blue_size = 0, green_size = 0, red_size = 0,
      depth_size = 0, stencil_size = 0;
  for (; EGL_NONE != *attrib_list; attrib_list += 2) {
    switch (attrib_list[0]) {
    case EGL_ALPHA_SIZE:
      alpha_size = attrib_list[1];
      break;
    case EGL_BLUE_SIZE:
      blue_size = attrib_list[1];
      break;
    case EGL_GREEN_SIZE:
      green_size = attrib_list[1];
      break;
    case EGL_RED_SIZE:
      red_size = attrib_list[1];
      break;
    case EGL_DEPTH_SIZE:
      depth_size = attrib_list[1];
      break;
    case EGL_STENCIL_SIZE:
      stencil_size = attrib_list[1];
      break;
    case EGL_BUFFER_SIZE:
    case EGL_CONFIG_CAVEAT:
    case EGL_CONFIG_ID:
    case EGL_LEVEL:
    case EGL_MAX_PBUFFER_HEIGHT:
    case EGL_MAX_PBUFFER_PIXELS:
    case EGL_MAX_PBUFFER_WIDTH:
    case EGL_NATIVE_RENDERABLE:
    case EGL_NATIVE_VISUAL_ID:
    case EGL_NATIVE_VISUAL_TYPE:
    case EGL_SAMPLES:
    case EGL_SAMPLE_BUFFERS:
    case EGL_SURFACE_TYPE:
    case EGL_TRANSPARENT_TYPE:
    case EGL_TRANSPARENT_BLUE_VALUE:
    case EGL_TRANSPARENT_GREEN_VALUE:
    case EGL_TRANSPARENT_RED_VALUE:
    case EGL_BIND_TO_TEXTURE_RGB:
    case EGL_BIND_TO_TEXTURE_RGBA:
    case EGL_MIN_SWAP_INTERVAL:
    case EGL_MAX_SWAP_INTERVAL:
      break;
    default:
      setErrorEGL(EGL_BAD_ATTRIBUTE);
      return EGL_FALSE;
    }
  }

  *num_config = 0;
  for (int dci = 0; g_depthConfigs[dci].d != 0xff; ++dci) {
    if (stencil_size != EGL_DONT_CARE && stencil_size > g_depthConfigs[dci].s) {
      continue;
    }
    if (depth_size != EGL_DONT_CARE && depth_size > g_depthConfigs[dci].d) {
      continue;
    }
    for (int cci = 0; g_colorConfigs[cci].r != 0xff; ++cci) {
      if (alpha_size != EGL_DONT_CARE && alpha_size > g_colorConfigs[cci].a) {
        continue;
      }
      if (blue_size != EGL_DONT_CARE && blue_size > g_colorConfigs[cci].b) {
        continue;
      }
      if (green_size != EGL_DONT_CARE && green_size > g_colorConfigs[cci].g) {
        continue;
      }
      if (red_size != EGL_DONT_CARE && red_size > g_colorConfigs[cci].r) {
        continue;
      }

      if (*num_config >= config_size) {
        goto configsChosen;
      }

      configs[(*num_config)++] = new uint16_t(cci | (dci << 8));
    }
  }

configsChosen:

  return EGL_TRUE;
}

EGLAPI EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config,
                                               EGLContext share_context,
                                               const EGLint *attrib_list) {
  (void)attrib_list;
  const EGLNativeDisplayType displayId = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (displayId >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  if (share_context) {
    g_screens[displayId]->set_context(
        safe_cast(LimeGLContext *, share_context));
    return share_context;
  }

  auto cfg_set = *reinterpret_cast<uint16_t *>(config);
  auto context = new LimeGLContext(g_screens[displayId]->lcd_width(),
                                   g_screens[displayId]->lcd_height(),
                                   g_colorConfigs[cfg_set & 0xff].internal,
                                   g_depthConfigs[cfg_set >> 0x8].internal);
  g_screens[displayId]->set_context(context);

  return context;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy,
                                             EGLSurface surface) {
  (void)surface;
  const EGLNativeDisplayType displayId = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (displayId >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  LimeDevice *device = g_screens[displayId];
  if (device == nullptr) {
    setErrorEGL(EGL_NOT_INITIALIZED);
    return EGL_FALSE;
  }

  u32 *split;
  u32 splitSize;

  GPUCMD_Split(&split, &splitSize);

  extern u32 __ctru_linear_heap;
  extern u32 __ctru_linear_heap_size;
  GX_FlushCacheRegions((u32 *)__ctru_linear_heap, __ctru_linear_heap_size,
                       nullptr, 0, nullptr, 0);

  gxCmdQueueWait(nullptr, -1);
  gxCmdQueueClear(&g_gxQueue);

  g_activeGpuCmd = !g_activeGpuCmd;
  GPU_Reset(nullptr, g_gpuCmd[g_activeGpuCmd], GPU_CMD_SIZE);

  GPUCMD_SetBufferOffset(0);

  GX_ProcessCommandList(split, splitSize * 4, 0x0);

  LightEvent_Wait(&g_swapEvent);

  reinterpret_cast<LimeDevice *>(device)->flush();

  g_swappable[displayId] = true;

  return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
                                             EGLSurface read, EGLContext ctx) {
  (void)draw;
  (void)read;
  (void)ctx;
  const EGLNativeDisplayType displayId = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (displayId >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }
  if (g_screens[displayId] == nullptr) {
    setErrorEGL(EGL_NOT_INITIALIZED);
    return EGL_FALSE;
  }

  g_currentContext = g_screens[displayId]->context();
  return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy) {
  const EGLNativeDisplayType displayId = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (displayId >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  auto dev = g_screens[displayId];
  delete dev;

  if (!(g_screens[0] || g_screens[1])) {
    gfxExit();
  }

  return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy,
                                                EGLContext ctx) {
  (void)dpy;
  (void)ctx;
  return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy,
                                                EGLSurface surface) {
  (void)dpy;
  (void)surface;
  return EGL_TRUE;
}

EGLAPI EGLint EGLAPIENTRY eglGetError(void) {
  EGLint error = g_eglErrorFlag;
  g_eglErrorFlag = EGL_SUCCESS;

  return error;
}

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress(const char *procname) {
  proctab_entry *pe = reinterpret_cast<proctab_entry *>(
      bsearch(procname, proctab_gl, proctab_gl_sz, sizeof(proctab_gl[0]),
              [](const void *key, const void *el) -> int {
                return std::bit_cast<int8_t>(
                    reinterpret_cast<const proctab_entry *>(el)->name <=>
                    reinterpret_cast<const char *>(key));
              }));
  if (pe) {
    return pe->proc;
  }
  pe = reinterpret_cast<proctab_entry *>(
      bsearch(procname, proctab_gl, proctab_gl_sz, sizeof(proctab_gl[0]),
              [](const void *key, const void *el) -> int {
                return std::bit_cast<int8_t>(
                    reinterpret_cast<const proctab_entry *>(el)->name <=>
                    reinterpret_cast<const char *>(key));
              }));
  if (pe) {
    return pe->proc;
  }
  return nullptr;
}
}
