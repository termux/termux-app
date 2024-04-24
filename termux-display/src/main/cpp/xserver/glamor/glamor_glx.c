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

#include <epoxy/glx.h>
#include "glamor_context.h"

/**
 * @file glamor_glx.c
 *
 * GLX context management for glamor.
 *
 * This has to be kept separate from the server sources because of
 * Xlib's conflicting definition of CARD32 and similar typedefs.
 */

static void
glamor_glx_make_current(struct glamor_context *glamor_ctx)
{
    /* There's only a single global dispatch table in Mesa.  EGL, GLX,
     * and AIGLX's direct dispatch table manipulation don't talk to
     * each other.  We need to set the context to NULL first to avoid
     * GLX's no-op context change fast path when switching back to
     * GLX.
     */
    glXMakeCurrent(glamor_ctx->display, None, None);

    glXMakeCurrent(glamor_ctx->display, glamor_ctx->drawable_xid,
                   glamor_ctx->ctx);
}


Bool
glamor_glx_screen_init(struct glamor_context *glamor_ctx)
{
    glamor_ctx->ctx = glXGetCurrentContext();
    if (!glamor_ctx->ctx)
        return False;

    glamor_ctx->display = glXGetCurrentDisplay();
    if (!glamor_ctx->display)
        return False;

    glamor_ctx->drawable_xid = glXGetCurrentDrawable();

    glamor_ctx->make_current = glamor_glx_make_current;

    return True;
}
