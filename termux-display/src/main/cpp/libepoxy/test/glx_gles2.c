/*
 * Copyright © 2013 Intel Corporation
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
 * @file glx_gles2.c
 *
 * Catches a bug where a GLES2 context using
 * GLX_EXT_create_context_es2_profile would try to find the symbols in
 * libGLESv2.so.2 instead of libGL.so.1.
 */

#include <stdio.h>
#include <assert.h>
#include <err.h>
#include "epoxy/gl.h"
#include "epoxy/glx.h"
#include <X11/Xlib.h>

#include "glx_common.h"

static Display *dpy;

GLuint
override_GLES2_glCreateShader(GLenum target);

GLuint
override_GLES2_glCreateShader(GLenum target)
{
    return 0;
}

void
override_GLES2_glGenQueries(GLsizei n, GLuint *ids);

void
override_GLES2_glGenQueries(GLsizei n, GLuint *ids)
{
    int i;
    for (i = 0; i < n; i++)
        ids[i] = 0;
}

int
main(int argc, char **argv)
{
    bool pass = true;
    XVisualInfo *vis;
    Window win;
    GLXContext ctx;
    GLXFBConfig config;
    int context_attribs[] = {
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_ES2_PROFILE_BIT_EXT,
        GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        0
    };
    GLuint shader;

    dpy = get_display_or_skip();

    if (!epoxy_has_glx_extension(dpy, 0, "GLX_EXT_create_context_es2_profile"))
        errx(77, "Test requires GLX_EXT_create_context_es2_profile");

    vis = get_glx_visual(dpy);
    config = get_fbconfig_for_visinfo(dpy, vis);
    win = get_glx_window(dpy, vis, false);

    ctx = glXCreateContextAttribsARB(dpy, config, NULL, true,
                                     context_attribs);

    glXMakeCurrent(dpy, win, ctx);

    if (epoxy_is_desktop_gl()) {
        errx(1, "GLES2 context creation made a desktop context\n");
    }

    if (epoxy_gl_version() < 20) {
        errx(1, "GLES2 context creation made a version %f context\n",
             epoxy_gl_version() / 10.0f);
    }

    /* Test using an entrypoint that's in GLES2, but not the desktop GL ABI. */
    shader = glCreateShader(GL_FRAGMENT_SHADER);
    if (shader == 0)
        errx(1, "glCreateShader() failed\n");
    glDeleteShader(shader);

    if (epoxy_gl_version() >= 30) {
        GLuint q = 0;

        glGenQueries(1, &q);
        if (!q)
            errx(1, "glGenQueries() failed\n");
        glDeleteQueries(1, &q);
    }

    return pass != true;
}
