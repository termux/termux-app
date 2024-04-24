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

/**
 * @file glamor_context.h
 *
 * This is the struct of state required for context switching in
 * glamor.  It has to use types that don't require including either
 * server headers or Xlib headers, since it will be included by both
 * the server and the GLX (xlib) code.
 */

struct glamor_context {
    /** Either an EGLDisplay or an Xlib Display */
    void *display;

    /** Either a GLXContext or an EGLContext. */
    void *ctx;

    /** The EGLSurface we should MakeCurrent to */
    void *drawable;

    /** The GLXDrawable we should MakeCurrent to */
    uint32_t drawable_xid;

    void (*make_current)(struct glamor_context *glamor_ctx);
};

Bool glamor_glx_screen_init(struct glamor_context *glamor_ctx);
