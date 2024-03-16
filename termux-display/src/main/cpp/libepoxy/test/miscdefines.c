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

#include <epoxy/gl.h>

#ifdef BUILD_EGL
#include <epoxy/egl.h>
#endif

#ifdef BUILD_GLX
#include <epoxy/glx.h>
#endif

#if GL_VERSION_3_2 != 1
#error bad GL_VERSION_3_2
#endif

#if GL_ARB_ES2_compatibility != 1
#error bad GL_ARB_ES2_compatibility
#endif

#ifndef GLAPI
#error missing GLAPI
#endif

#ifndef GLAPIENTRY
#error missing GLAPIENTRY
#endif

#ifndef GLAPIENTRYP
#error missing GLAPIENTRYP
#endif

#ifndef APIENTRY
#error missing APIENTRY
#endif

#ifndef APIENTRYP
#error missing APIENTRYP
#endif

/* Do we want to export GL_GLEXT_VERSION? */

int main(int argc, char **argv)
{
    return 0;
}
