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
 * @file cgl_epoxy_api.c
 *
 * Tests the Epoxy API using the CoreGraphics OpenGL framework.
 */

#include <epoxy/gl.h>
#include <Carbon/Carbon.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/CGLContext.h>

int
main (void)
{
    CGLPixelFormatAttribute attribs[] = {0};
    CGLPixelFormatObj pix;
    CGLContextObj ctx;
    const char *string;
    bool pass = true;
    int npix;
    GLint shader;

    CGLChoosePixelFormat(attribs, &pix, &npix);
    CGLCreateContext(pix, (void *) 0, &ctx);
    CGLSetCurrentContext(ctx);

    if (!epoxy_is_desktop_gl()) {
        fputs("Claimed not to be desktop\n", stderr);
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

    CGLSetCurrentContext(NULL);
    CGLReleaseContext(ctx);
    CGLReleasePixelFormat(pix);

    return pass != true;
}
