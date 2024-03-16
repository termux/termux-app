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

#include <err.h>
#include <epoxy/egl.h>
#include <X11/Xlib.h>
#include "egl_common.h"

/**
 * Do whatever it takes to get us an EGL display for the system.
 *
 * This needs to be ported to other window systems.
 */
EGLDisplay *
get_egl_display_or_skip(void)
{
    Display *dpy = XOpenDisplay(NULL);
    EGLint major, minor;
    EGLDisplay *edpy;
    bool ok;

    if (!dpy)
        errx(77, "couldn't open display\n");

    edpy = eglGetDisplay(dpy);
    if (!edpy)
        errx(1, "Couldn't get EGL display for X11 Display.\n");

    ok = eglInitialize(edpy, &major, &minor);
    if (!ok)
        errx(1, "eglInitialize() failed\n");

    return edpy;
}
