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
 * @file glx_has_extension_nocontext.c
 *
 * Catches a bug in early development where glXGetProcAddress() with
 * no context bound would fail out in dispatch.
 */

#include <stdio.h>
#include <assert.h>
#include <err.h>
#include "epoxy/gl.h"
#include "epoxy/glx.h"
#include <X11/Xlib.h>

#include "glx_common.h"

static Display *dpy;

int
main(int argc, char **argv)
{
    bool pass = true;

    dpy = get_display_or_skip();

    if (!epoxy_has_glx_extension(dpy, 0, "GLX_ARB_get_proc_address"))
        errx(1, "Implementation reported absence of GLX_ARB_get_proc_address");

    if (epoxy_has_glx_extension(dpy, 0, "GLX_ARB_ham_sandwich"))
        errx(1, "Implementation reported presence of GLX_ARB_ham_sandwich");

    return pass != true;
}
