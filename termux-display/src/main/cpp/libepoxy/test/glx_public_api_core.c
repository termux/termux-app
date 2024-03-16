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
#include <stdlib.h>
#include <assert.h>
#include <err.h>
#include "epoxy/gl.h"
#include "epoxy/glx.h"
#include <X11/Xlib.h>

#include "glx_common.h"

static Display *dpy;

static bool
test_has_extensions(void)
{
    int num_extensions;

    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

    for (int i = 0; i < num_extensions; i++) {
        char *ext = (char *)glGetStringi(GL_EXTENSIONS, i);

        if (!epoxy_has_gl_extension(ext)) {
            fprintf(stderr, "GL implementation reported support for %s, "
                    "but epoxy didn't\n", ext);
            return false;
        }
    }

    if (epoxy_has_gl_extension("GL_ARB_ham_sandwich")) {
        fputs("epoxy implementation reported support for "
              "GL_ARB_ham_sandwich, but it shouldn't\n",
              stderr);
        return false;
    }

    return true;
}

static bool
test_gl_version(void)
{
    int gl_version, epoxy_version;
    int major, minor;

    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    gl_version = major * 10 + minor;

    if (gl_version < 32) {
        fprintf(stderr,
                "Implementation reported GL version %d, should be at least 32\n",
                gl_version);
        return false;
    }

    epoxy_version = epoxy_gl_version();
    if (epoxy_version != gl_version) {
        fprintf(stderr,
                "Epoxy reported GL version %d, should be %d\n",
                epoxy_version, gl_version);
        return false;
    }

    return true;
}

static bool
test_glx_version(void)
{
    int version = epoxy_glx_version(dpy, 0);
    const char *version_string;
    int ret;
    int server_major, server_minor;
    int client_major, client_minor;
    int server, client, expected;

    if (version < 13) {
        fprintf(stderr,
                "Reported GLX version %d, should be at least 13 "
                "according to Linux GL ABI\n",
                version);
        return false;
    }

    version_string = glXQueryServerString(dpy, 0, GLX_VERSION);
    ret = sscanf(version_string, "%d.%d", &server_major, &server_minor);
    assert(ret == 2);
    server = server_major * 10 + server_minor;

    version_string = glXGetClientString(dpy, GLX_VERSION);
    ret = sscanf(version_string, "%d.%d", &client_major, &client_minor);
    assert(ret == 2);
    client = client_major * 10 + client_minor;

    if (client < server)
        expected = client;
    else
        expected = server;

    if (version != expected) {
        fprintf(stderr,
                "Reported GLX version %d, should be %d (%s)\n",
                version, expected, version_string);
        return false;
    }

    return true;
}

static int
error_handler(Display *d, XErrorEvent *ev)
{
    return 0;
}

int
main(int argc, char **argv)
{
    bool pass = true;
    XVisualInfo *visinfo;
    Window win;
    GLXFBConfig config;
    static const int attribs[] = {
        GLX_CONTEXT_PROFILE_MASK_ARB,
        GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_MAJOR_VERSION_ARB,
        3,
        GLX_CONTEXT_MINOR_VERSION_ARB,
        2,
        None
    };
    GLXContext ctx;
    int (*old_handler)(Display *, XErrorEvent *);

    dpy = get_display_or_skip();

    if (!epoxy_has_glx_extension(dpy, 0, "GLX_ARB_create_context_profile"))
        errx(77, "Test requires GLX_ARB_create_context_profile");

    visinfo = get_glx_visual(dpy);
    win = get_glx_window(dpy, visinfo, false);
    config = get_fbconfig_for_visinfo(dpy, visinfo);

    old_handler = XSetErrorHandler(error_handler);
    ctx = glXCreateContextAttribsARB(dpy, config, NULL, True, attribs);
    if (ctx == None)
        errx(77, "glXCreateContext failed");
    XSetErrorHandler(old_handler);

    glXMakeCurrent(dpy, win, ctx);

    pass = test_gl_version() && pass;
    pass = test_glx_version() && pass;
    pass = test_has_extensions() && pass;

    return pass != true;
}
