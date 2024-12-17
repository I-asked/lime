#include <3ds.h>
#include <cmath>

#include <EGL/egl.h>
#include <GLES/gl.h>

void DrawGLScene() {
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int main()
{
    hidInit();

    auto dpy = eglGetDisplay(GFX_TOP);
    eglInitialize(dpy, nullptr, nullptr);
    EGLint attrs[] {
      EGL_ALPHA_SIZE, 8,
      EGL_RED_SIZE, 8,
      EGL_STENCIL_SIZE, 8,
      EGL_NONE,
    };
    EGLConfig conf;
    EGLint num_conf = 0;
    eglChooseConfig(dpy, attrs, &conf, 1, &num_conf);
    auto ctx = eglCreateContext(dpy, conf, nullptr, nullptr);

    eglMakeCurrent(dpy, nullptr, nullptr, ctx);

    while (aptMainLoop())
    {
        hidScanInput();

        if (keysDown() & KEY_START)
            break;

        DrawGLScene();

        eglSwapBuffers(dpy, nullptr);
    }

    // Exit services
    hidExit();
    eglDestroyContext(dpy, ctx);
    eglTerminate(dpy);
    return 0;
}
