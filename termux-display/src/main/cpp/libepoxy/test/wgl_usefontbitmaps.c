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
    HGLRC ctx;
    GLuint dlist[2] = {100, 101};
    const char *string = "some string";

    ctx = wglCreateContext(hdc);
    if (!ctx) {
        fputs("Failed to create wgl context\n", stderr);
        return 1;
    }
    if (!wglMakeCurrent(hdc, ctx)) {
        fputs("Failed to make context current\n", stderr);
        return 1;
    }

    /* First, use the #ifdeffed variant of the function */
    wglUseFontBitmaps(hdc, 0, 255, dlist[0]);
    glListBase(dlist[1]);
    glCallLists(strlen(string), GL_UNSIGNED_BYTE, string);

    /* Now, use the specific version, manually. */
#ifdef UNICODE
    wglUseFontBitmapsW(hdc, 0, 255, dlist[0]);
#else
    wglUseFontBitmapsA(hdc, 0, 255, dlist[0]);
#endif
    glListBase(dlist[1]);
    glCallLists(strlen(string), GL_UNSIGNED_BYTE, string);

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
