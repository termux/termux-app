/*
 * Copyright 2018  Emmanuele Bassi
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
 * @file epoxy_api.c
 *
 * Tests the Epoxy API using EGL.
 */

#ifdef __sun
#define __EXTENSIONS__
#else
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <err.h>
#include "epoxy/gl.h"
#include "epoxy/egl.h"

#include "egl_common.h"

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

    if (epoxy_glsl_version() < 100) {
        fprintf(stderr, "Claimed to have GLSL version %d\n",
                epoxy_glsl_version());
        pass = false;
    }

    string = (const char *)glGetString(GL_VERSION);
    printf("GL version: %s - Epoxy: %d\n", string, epoxy_gl_version());

    string = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("GLSL version: %s - Epoxy: %d\n", string, epoxy_glsl_version());

    shader = glCreateShader(GL_FRAGMENT_SHADER);
    pass = glIsShader(shader);

    return pass;
}

static void
init_egl(EGLDisplay *dpy, EGLContext *out_ctx)
{
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

    *out_ctx = ctx;
}

int
main(int argc, char **argv)
{
    bool pass = true;

    EGLContext egl_ctx;
    EGLDisplay *dpy = get_egl_display_or_skip();
    const char *extensions = eglQueryString(dpy, EGL_EXTENSIONS);
    char *first_space;
    char *an_extension;

    /* We don't have any extensions guaranteed by the ABI, so for the
     * touch test we just check if the first one is reported to be there.
     */
    first_space = strstr(extensions, " ");
    if (first_space) {
        an_extension = strndup(extensions, first_space - extensions);
    } else {
        an_extension = strdup(extensions);
    }

    if (!epoxy_extension_in_string(extensions, an_extension))
        errx(1, "Implementation reported absence of %s", an_extension);

    free(an_extension);

    init_egl(dpy, &egl_ctx);
    pass = make_egl_current_and_test(dpy, egl_ctx);

    return pass != true;
}
