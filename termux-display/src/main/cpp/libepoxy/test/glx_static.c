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
 * @file glx_static.c
 *
 * Simple touch-test of using epoxy when linked statically.  On Linux,
 * the ifunc support we'd like to use has some significant behavior
 * changes depending on whether it's a static build or shared library
 * build.
 *
 * Note that if configured without --enable-static, this test will end
 * up dynamically linked anyway, defeating the test.
 */

#include <stdio.h>
#include <assert.h>
#include "epoxy/gl.h"
#include "epoxy/glx.h"
#include <X11/Xlib.h>
#include <dlfcn.h>

#include "glx_common.h"

int
main(int argc, char **argv)
{
    bool pass = true;
    int val;

#if NEEDS_TO_BE_STATIC
    if (dlsym(NULL, "epoxy_glCompileShader")) {
        fputs("glx_static requires epoxy built with --enable-static\n", stderr);
        return 77;
    }
#endif

    Display *dpy = get_display_or_skip();
    make_glx_context_current_or_skip(dpy);

    glEnable(GL_LIGHTING);
    val = 0;
    glGetIntegerv(GL_LIGHTING, &val);
    if (!val) {
        fputs("Enabling GL_LIGHTING didn't stick.\n", stderr);
        pass = false;
    }

    return pass != true;
}
