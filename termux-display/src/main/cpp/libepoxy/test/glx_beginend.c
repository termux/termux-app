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
#include <assert.h>
#include "epoxy/gl.h"
#include "epoxy/glx.h"
#include <X11/Xlib.h>

#include "glx_common.h"

static Display *dpy;
static bool has_argb2101010;

static bool
test_with_epoxy(void)
{
    glBegin(GL_TRIANGLES);
    {
        /* Hit a base entrypoint that won't call gl_version() */
        glVertex2f(0, 0);

        /* Hit an entrypoint that will call probably call gl_version() */
        glMultiTexCoord4f(GL_TEXTURE0, 0.0, 0.0, 0.0, 0.0);

        /* Hit an entrypoint that will probably call
         * epoxy_conservative_has_extension();
         */
        if (has_argb2101010) {
            glTexCoordP4ui(GL_UNSIGNED_INT_2_10_10_10_REV, 0);
        }
    }
    glEnd();

    /* No error should have been generated in the process. */
    return glGetError() == 0;
}



#undef glBegin
#undef glEnd
extern void glBegin(GLenum primtype);
extern void glEnd(void);

static bool
test_without_epoxy(void)
{
    glBegin(GL_TRIANGLES);
    {
        /* Hit a base entrypoint that won't call gl_version() */
        glVertex4f(0, 0, 0, 0);

        /* Hit an entrypoint that will call probably call gl_version() */
        glMultiTexCoord3f(GL_TEXTURE0, 0.0, 0.0, 0.0);

        /* Hit an entrypoint that will probably call
         * epoxy_conservative_has_extension();
         */
        if (has_argb2101010) {
            glTexCoordP3ui(GL_UNSIGNED_INT_2_10_10_10_REV, 0);
        }
    }
    glEnd();

    /* We can't make any assertions about error presence this time
     * around. This test is just trying to catch segfaults.
     */
    return true;
}

int
main(int argc, char **argv)
{
    bool pass = true;

    dpy = get_display_or_skip();
    make_glx_context_current_or_skip(dpy);

    has_argb2101010 =
        epoxy_has_gl_extension("GL_ARB_vertex_type_2_10_10_10_rev");

    pass = pass && test_with_epoxy();
    pass = pass && test_without_epoxy();

    return pass != true;
}
