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

static bool
test_gl_version(void)
{
    int version = epoxy_gl_version();
    if (version < 12) {
        fprintf(stderr,
                "Reported GL version %d, should be at least 12\n",
                version);
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

static bool
test_glx_extension_supported(void)
{
    if (!epoxy_has_glx_extension(dpy, 0, "GLX_ARB_get_proc_address")) {
        fputs("Incorrectly reported no support for GLX_ARB_get_proc_address "
              "(should always be present in Linux ABI)\n",
              stderr);
        return false;
    }

    if (epoxy_has_glx_extension(dpy, 0, "GLX_EXT_ham_sandwich")) {
        fputs("Incorrectly reported support for GLX_EXT_ham_sandwich\n",
              stderr);
        return false;
    }

    return true;
}

int
main(int argc, char **argv)
{
    bool pass = true;

    dpy = get_display_or_skip();
    make_glx_context_current_or_skip(dpy);

    pass = test_gl_version() && pass;
    pass = test_glx_version() && pass;
    pass = test_glx_extension_supported() && pass;

    return pass != true;
}
