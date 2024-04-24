/*
 * Copyright © 2014 Jon TURNEY
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

#ifndef indirect_h
#define indirect_h

#include <X11/Xwindows.h>
#include <GL/wglext.h>
#include <glx/extension_string.h>

/* ---------------------------------------------------------------------- */
/*
 *   structure definitions
 */

typedef struct __GLXWinContext __GLXWinContext;
typedef struct __GLXWinDrawable __GLXWinDrawable;
typedef struct __GLXWinScreen glxWinScreen;
typedef struct __GLXWinConfig GLXWinConfig;

struct __GLXWinContext {
    __GLXcontext base;
    HGLRC ctx;                  /* Windows GL Context */
    __GLXWinContext *shareContext;      /* Context with which we will share display lists and textures */
    HWND hwnd;                  /* For detecting when HWND has changed */
};

struct __GLXWinDrawable {
    __GLXdrawable base;
    __GLXWinContext *drawContext;
    __GLXWinContext *readContext;

    /* If this drawable is GLX_DRAWABLE_PBUFFER */
    HPBUFFERARB hPbuffer;

    /* If this drawable is GLX_DRAWABLE_PIXMAP */
    HDC dibDC;
    HANDLE hSection;            /* file mapping handle */
    HBITMAP hDIB;
    HBITMAP hOldDIB;            /* original DIB for DC */
    void *pOldBits;             /* original pBits for this drawable's pixmap */
};

struct __GLXWinScreen {
    __GLXscreen base;

    Bool has_WGL_ARB_multisample;
    Bool has_WGL_ARB_pixel_format;
    Bool has_WGL_ARB_pbuffer;
    Bool has_WGL_ARB_render_texture;
    Bool has_WGL_ARB_make_current_read;
    Bool has_WGL_ARB_framebuffer_sRGB;

    /* wrapped screen functions */
    RealizeWindowProcPtr RealizeWindow;
    UnrealizeWindowProcPtr UnrealizeWindow;
    CopyWindowProcPtr CopyWindow;
};

struct __GLXWinConfig {
    __GLXconfig base;
    int pixelFormatIndex;
};

/* ---------------------------------------------------------------------- */
/*
 *   function prototypes
 */

void
glxWinDeferredCreateDrawable(__GLXWinDrawable *draw, __GLXconfig *config);

#endif /* indirect_h */
