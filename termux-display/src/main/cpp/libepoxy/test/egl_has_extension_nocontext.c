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
 * @file egl_has_extension_nocontext.c
 *
 * Catches a bug in early development where eglGetProcAddress() with
 * no context bound would fail out in dispatch.
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

int
main(int argc, char **argv)
{
    bool pass = true;

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

    if (!epoxy_has_egl_extension(dpy, an_extension))
        errx(1, "Implementation reported absence of %s", an_extension);

    free(an_extension);

    if (epoxy_has_egl_extension(dpy, "GLX_ARB_ham_sandwich"))
        errx(1, "Implementation reported presence of GLX_ARB_ham_sandwich");

    return pass != true;
}
