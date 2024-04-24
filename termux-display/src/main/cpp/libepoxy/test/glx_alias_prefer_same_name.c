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

static int last_call;

#define CORE_FUNC_VAL 100
#define EXT_FUNC_VAL 101

void
override_GL_glBindTexture(GLenum target);
void
override_GL_glBindTextureEXT(GLenum target);

void
override_GL_glBindTexture(GLenum target)
{
    last_call = CORE_FUNC_VAL;
}

void
override_GL_glBindTextureEXT(GLenum target)
{
    last_call = EXT_FUNC_VAL;
}

int
main(int argc, char **argv)
{
    bool pass = true;

    dpy = get_display_or_skip();
    make_glx_context_current_or_skip(dpy);

    if (!epoxy_has_gl_extension("GL_EXT_texture_object"))
        errx(77, "Test requires GL_EXT_texture_object");

    glBindTexture(GL_TEXTURE_2D, 1);
    pass = pass && last_call == CORE_FUNC_VAL;
    glBindTextureEXT(GL_TEXTURE_2D, 1);
    pass = pass && last_call == EXT_FUNC_VAL;

    return pass != true;
}
