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

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "dispatch_common.h"

/**
 * If we can determine the GLX version from the current context, then
 * return that, otherwise return a version that will just send us on
 * to dlsym() or get_proc_address().
 */
int
epoxy_conservative_glx_version(void)
{
    Display *dpy = glXGetCurrentDisplay();
    GLXContext ctx = glXGetCurrentContext();
    int screen;

    if (!dpy || !ctx)
        return 14;

    glXQueryContext(dpy, ctx, GLX_SCREEN, &screen);

    return epoxy_glx_version(dpy, screen);
}


/**
 * @brief Returns the version of GLX we are using
 *
 * The version is encoded as:
 *
 * ```
 *
 *   version = major * 10 + minor
 *
 * ```
 *
 * So it can be easily used for version comparisons.
 *
 * @param dpy The X11 display
 * @param screen The X11 screen
 *
 * @return The encoded version of GLX we are using
 *
 * @see epoxy_gl_version()
 */
int
epoxy_glx_version(Display *dpy, int screen)
{
    int server_major, server_minor;
    int client_major, client_minor;
    int server, client;
    const char *version_string;
    int ret;

    version_string = glXQueryServerString(dpy, screen, GLX_VERSION);
    if (!version_string)
        return 0;

    ret = sscanf(version_string, "%d.%d", &server_major, &server_minor);
    assert(ret == 2);
    server = server_major * 10 + server_minor;

    version_string = glXGetClientString(dpy, GLX_VERSION);
    if (!version_string)
        return 0;

    ret = sscanf(version_string, "%d.%d", &client_major, &client_minor);
    assert(ret == 2);
    client = client_major * 10 + client_minor;

    if (client < server)
        return client;
    else
        return server;
}

/**
 * If we can determine the GLX extension support from the current
 * context, then return that, otherwise give the answer that will just
 * send us on to get_proc_address().
 */
bool
epoxy_conservative_has_glx_extension(const char *ext)
{
    Display *dpy = glXGetCurrentDisplay();
    GLXContext ctx = glXGetCurrentContext();
    int screen;

    if (!dpy || !ctx)
        return true;

    glXQueryContext(dpy, ctx, GLX_SCREEN, &screen);

    return epoxy_has_glx_extension(dpy, screen, ext);
}

/**
 * @brief Returns true if the given GLX extension is supported in the current context.
 *
 * @param dpy The X11 display
 * @param screen The X11 screen
 * @param extension The name of the GLX extension
 *
 * @return `true` if the extension is available
 *
 * @see epoxy_has_gl_extension()
 * @see epoxy_has_egl_extension()
 */
bool
epoxy_has_glx_extension(Display *dpy, int screen, const char *ext)
{
    /* No, you can't just use glXGetClientString or
     * glXGetServerString() here.  Those each tell you about one half
     * of what's needed for an extension to be supported, and
     * glXQueryExtensionsString() is what gives you the intersection
     * of the two.
     */
    return epoxy_extension_in_string(glXQueryExtensionsString(dpy, screen), ext);
}

/**
 * @brief Checks whether GLX is available.
 *
 * @param dpy The X11 display
 *
 * @return `true` if GLX is available
 *
 * @newin{1,4}
 */
bool
epoxy_has_glx(Display *dpy)
{
#if !PLATFORM_HAS_GLX
    return false;
#else
    if (epoxy_load_glx(false, true)) {
        Bool (* pf_glXQueryExtension) (Display *, int *, int *);
        int error_base, event_base;

        pf_glXQueryExtension = epoxy_conservative_glx_dlsym("glXQueryExtension", false);
        if (pf_glXQueryExtension && pf_glXQueryExtension(dpy, &error_base, &event_base))
            return true;
    }

    return false;
#endif /* !PLATFORM_HAS_GLX */
}
