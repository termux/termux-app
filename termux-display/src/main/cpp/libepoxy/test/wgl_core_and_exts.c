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

#include <stdio.h>

#include "wgl_common.h"
#include <epoxy/gl.h>

static int
test_function(HDC hdc)
{
    bool pass = true;
    int val;
    HGLRC ctx;

    ctx = wglCreateContext(hdc);
    if (!ctx) {
        fputs("Failed to create wgl context\n", stderr);
        return 1;
    }
    if (!wglMakeCurrent(hdc, ctx)) {
        fputs("Failed to make context current\n", stderr);
        return 1;
    }

    /* GL 1.0 APIs are available as symbols in opengl32.dll. */
    glEnable(GL_LIGHTING);
    val = 0;
    glGetIntegerv(GL_LIGHTING, &val);
    if (!val) {
        fputs("Enabling GL_LIGHTING didn't stick.\n", stderr);
        pass = false;
    }

    if (epoxy_gl_version() >= 15 ||
        epoxy_has_gl_extension("GL_ARB_vertex_buffer_object")) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 1234);

        val = 0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &val);
        if (val != 1234) {
            printf("GL_ELEMENT_ARRAY_BUFFER_BINDING didn't stick: %d\n", val);
            pass = false;
        }
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(ctx);

    return !pass;
}

int
main(int argc, char **argv)
{
    make_window_and_test(test_function);

    /* UNREACHED */
    return 1;
}
