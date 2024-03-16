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

int
epoxy_conservative_egl_version(void)
{
    EGLDisplay dpy = eglGetCurrentDisplay();

    if (!dpy)
        return 14;

    return epoxy_egl_version(dpy);
}

/**
 * @brief Returns the version of OpenGL we are using
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
 * @param The EGL display
 *
 * @return The encoded version of EGL we are using
 *
 * @see epoxy_gl_version()
 */
int
epoxy_egl_version(EGLDisplay dpy)
{
    int major, minor;
    const char *version_string;
    int ret;

    version_string = eglQueryString(dpy, EGL_VERSION);
    if (!version_string)
        return 0;

    ret = sscanf(version_string, "%d.%d", &major, &minor);
    assert(ret == 2);
    return major * 10 + minor;
}

bool
epoxy_conservative_has_egl_extension(const char *ext)
{
    return epoxy_has_egl_extension(eglGetCurrentDisplay(), ext);
}

/**
 * @brief Returns true if the given EGL extension is supported in the current context.
 *
 * @param dpy The EGL display
 * @param extension The name of the EGL extension
 *
 * @return `true` if the extension is available
 *
 * @see epoxy_has_gl_extension()
 * @see epoxy_has_glx_extension()
 */
bool
epoxy_has_egl_extension(EGLDisplay dpy, const char *ext)
{
    return epoxy_extension_in_string(eglQueryString(dpy, EGL_EXTENSIONS), ext) || epoxy_extension_in_string(eglQueryString(NULL, EGL_EXTENSIONS), ext);
}

/**
 * @brief Checks whether EGL is available.
 *
 * @return `true` if EGL is available
 *
 * @newin{1,4}
 */
bool
epoxy_has_egl(void)
{
#if !PLATFORM_HAS_EGL
    return false;
#else
    if (epoxy_load_egl(false, true)) {
        EGLDisplay* (* pf_eglGetCurrentDisplay) (void);

        pf_eglGetCurrentDisplay = epoxy_conservative_egl_dlsym("eglGetCurrentDisplay", false);
        if (pf_eglGetCurrentDisplay)
            return true;
    }

    return false;
#endif /* PLATFORM_HAS_EGL */
}
