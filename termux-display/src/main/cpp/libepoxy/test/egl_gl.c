/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * @file egl_gl.c
 *
 * Tests that epoxy works with EGL using desktop OpenGL.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <err.h>
#include <dlfcn.h>
#include "epoxy/gl.h"
#include "epoxy/egl.h"
#include "epoxy/glx.h"

#include "egl_common.h"
#include "glx_common.h"
#include "dlwrap.h"

static bool
make_egl_current_and_test(EGLDisplay *dpy, EGLContext ctx)
{
    const char *string;
    GLuint shader;
    bool pass = true;

    eglMakeCurrent(dpy, NULL, NULL, ctx);

    if (!epoxy_is_desktop_gl()) {
        fputs("Claimed to be desktop\n", stderr);
        pass = false;
    }

    if (epoxy_gl_version() < 20) {
        fprintf(stderr, "Claimed to be GL version %d\n",
                epoxy_gl_version());
        pass = false;
    }

    string = (const char *)glGetString(GL_VERSION);
    printf("GL version: %s\n", string);

    shader = glCreateShader(GL_FRAGMENT_SHADER);
    pass = glIsShader(shader);

    return pass;
}

static void
init_egl(EGLDisplay **out_dpy, EGLContext *out_ctx)
{
    EGLDisplay *dpy = get_egl_display_or_skip();
    static const EGLint config_attribs[] = {
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_RED_SIZE, 1,
	EGL_GREEN_SIZE, 1,
	EGL_BLUE_SIZE, 1,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
	EGL_NONE
    };
    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext ctx;
    EGLConfig cfg;
    EGLint count;

    if (!epoxy_has_egl_extension(dpy, "EGL_KHR_surfaceless_context"))
        errx(77, "Test requires EGL_KHR_surfaceless_context");

    if (!eglBindAPI(EGL_OPENGL_API))
        errx(77, "Couldn't initialize EGL with desktop GL\n");

    if (!eglChooseConfig(dpy, config_attribs, &cfg, 1, &count))
        errx(77, "Couldn't get an EGLConfig\n");

    ctx = eglCreateContext(dpy, cfg, NULL, context_attribs);
    if (!ctx)
        errx(77, "Couldn't create a GL context\n");

    *out_dpy = dpy;
    *out_ctx = ctx;
}

int
main(int argc, char **argv)
{
    bool pass = true;
    EGLDisplay *egl_dpy;
    EGLContext egl_ctx;

    /* Force epoxy to have loaded both EGL and GLX libs already -- we
     * can't assume anything about symbol resolution based on having
     * EGL or GLX loaded.
     */
    (void)glXGetCurrentContext();
    (void)eglGetCurrentContext();

    init_egl(&egl_dpy, &egl_ctx);
    pass = make_egl_current_and_test(egl_dpy, egl_ctx) && pass;

    return pass != true;
}
