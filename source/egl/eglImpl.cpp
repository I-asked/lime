#include "proctab.h"

#include "limeGl.h"
#include "gfb.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace lime {

LimeGLContext *g_currentContext = nullptr;
} // namespace lime

using namespace lime;

namespace {
#define NUM_SCREENS (2)

LimeDevice *g_screens[NUM_SCREENS] = {nullptr, nullptr};
} // namespace

extern "C" {

extern const unsigned long proctab_gl_sz;
extern proctab_entry *proctab_gl;

extern const unsigned long proctab_egl_sz;
extern proctab_entry *proctab_egl;

namespace {
EGLint g_eglErrorFlag = EGL_SUCCESS;

void setErrorEGL(EGLint error) {
  if (g_eglErrorFlag == EGL_SUCCESS) {
    g_eglErrorFlag = error;
  }
}

const struct ColorConfig {
  khronos_uint8_t r, g, b, a;
  GPU_COLORBUF internal;
} g_colorConfigs[] = {
  { 5, 5, 5, 0, GPU_RB_RGB565 },
  { 5, 5, 5, 1, GPU_RB_RGBA5551 },
  { 4, 4, 4, 4, GPU_RB_RGBA4 },
  { 8, 8, 8, 0, GPU_RB_RGB8 },
  { 8, 8, 8, 8, GPU_RB_RGBA8 },
  { 0xff, 0xff, 0xff, 0xff, (GPU_COLORBUF)0 },
};

const struct DepthConfig {
  khronos_uint8_t d, s;
  GPU_DEPTHBUF internal;
} g_depthConfigs[] = {
  { 16, 0, GPU_RB_DEPTH16 },
  { 24, 0, GPU_RB_DEPTH24 },
  { 24, 8, GPU_RB_DEPTH24_STENCIL8 },
  { 0xff, 0xff, (GPU_DEPTHBUF)0 },
};
} // namespace

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id) {
  if (display_id >= NUM_SCREENS)
    return EGL_NO_DISPLAY;

  return g_screens + display_id;
}

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major,
                                            EGLint *minor) {
  const EGLNativeDisplayType display_id = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (display_id >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  if (major)
    *major = 1;
  if (minor)
    *minor = 5;

  if (g_screens[display_id]) {
    return EGL_TRUE;
  }

  if (!(g_screens[0] || g_screens[1])) {
    gfbInitDefault();
  }

  g_screens[display_id] = new LimeDevice;

  return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy,
                                              const EGLint *attrib_list,
                                              EGLConfig *configs,
                                              EGLint config_size,
                                              EGLint *num_config) {
  const EGLNativeDisplayType display_id = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (display_id >= NUM_SCREENS) {
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
  const EGLNativeDisplayType display_id = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (display_id >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  if (share_context) {
    g_screens[display_id]->set_context(
        safe_cast(LimeGLContext *, share_context));
    return share_context;
  }

  auto cfg_set = *reinterpret_cast<uint16_t *>(config);
  auto context = new LimeGLContext(g_colorConfigs[cfg_set & 0xff].internal,
                                   g_depthConfigs[cfg_set >> 0x8].internal);
  g_screens[display_id]->set_context(context);

  return context;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
                       NativeWindowType window, const EGLint *attrib_list) {
  (void)window;
  (void)config;
  (void)attrib_list;
  const EGLNativeDisplayType display_id = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (display_id >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  return dpy;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy,
                                             EGLSurface surface) {
  (void)surface;
  const EGLNativeDisplayType display_id = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (display_id >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  LimeDevice *device = g_screens[display_id];
  if (device == nullptr) {
    setErrorEGL(EGL_NOT_INITIALIZED);
    return EGL_FALSE;
  }

  reinterpret_cast<LimeDevice *>(device)->flush();

  gfbFlushBuffers();
  gfbSwapBuffers();
  gspWaitForVBlank();

  return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
                                             EGLSurface read, EGLContext ctx) {
  (void)draw;
  (void)read;
  (void)ctx;
  const EGLNativeDisplayType display_id = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (display_id >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  lime::g_currentContext = g_screens[display_id]->context();
  return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy) {
  const EGLNativeDisplayType display_id = static_cast<EGLNativeDisplayType>(
      reinterpret_cast<char *>(dpy) - reinterpret_cast<char *>(g_screens));
  if (display_id >= NUM_SCREENS) {
    setErrorEGL(EGL_BAD_DISPLAY);
    return EGL_FALSE;
  }

  auto dev = g_screens[display_id];
  delete dev;

  if (!(g_screens[0] || g_screens[1])) {
    gfbExit();
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
