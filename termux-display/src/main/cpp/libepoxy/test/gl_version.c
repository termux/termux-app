/*
 * Copyright © 2018 Broadcom
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
#include <assert.h>
#include "epoxy/gl.h"

GLenum mock_enum;
const char *mock_gl_version;
const char *mock_glsl_version;

static const GLubyte * EPOXY_CALLSPEC override_glGetString(GLenum name)
{
    switch (name) {
    case GL_VERSION:
        return (GLubyte *)mock_gl_version;
    case GL_SHADING_LANGUAGE_VERSION:
        return (GLubyte *)mock_glsl_version;
    default:
        assert(!"unexpected glGetString() enum");
        return 0;
    }
}

static bool
test_version(const char *gl_string, int gl_version,
             const char *glsl_string, int glsl_version)
{
    int epoxy_version;

    mock_gl_version = gl_string;
    mock_glsl_version = glsl_string;

    epoxy_version = epoxy_gl_version();
    if (epoxy_version != gl_version) {
        fprintf(stderr,
                "glGetString(GL_VERSION) = \"%s\" returned epoxy_gl_version() "
                "%d instead of %d\n", gl_string, epoxy_version, gl_version);
        return false;
    }


    epoxy_version = epoxy_glsl_version();
    if (epoxy_version != glsl_version) {
        fprintf(stderr,
                "glGetString() = \"%s\" returned epoxy_glsl_version() "
                "%d instead of %d\n", glsl_string, epoxy_version, glsl_version);
        return false;
    }

    return true;
}

int
main(int argc, char **argv)
{
    bool pass = true;

    epoxy_glGetString = override_glGetString;

    pass = pass && test_version("3.0 Mesa 13.0.6", 30,
                                "1.30", 130);
    pass = pass && test_version("OpenGL ES 2.0 Mesa 20.1.0-devel (git-4bb19a330e)", 20,
                                "OpenGL ES GLSL ES 1.0.16", 100);
    pass = pass && test_version("OpenGL ES 3.2 Mesa 18.3.0-devel", 32,
                                "OpenGL ES GLSL ES 3.20", 320);
    pass = pass && test_version("4.5.0 NVIDIA 384.130", 45,
                                "4.50", 450);

    return pass != true;
}
