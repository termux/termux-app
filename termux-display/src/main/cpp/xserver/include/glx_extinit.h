/*
 * Copyright (C) 1994-2003 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
 * NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall not
 * be used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from the XFree86 Project.
 */

#ifndef GLX_EXT_INIT_H
#define GLX_EXT_INIT_H

/* this is separate due to sdksyms pulling in extinit.h */
/* XXX this comment no longer makes sense i think */
#ifdef GLXEXT
typedef struct __GLXprovider __GLXprovider;
typedef struct __GLXscreen __GLXscreen;
struct __GLXprovider {
    __GLXscreen *(*screenProbe) (ScreenPtr pScreen);
    const char *name;
    __GLXprovider *next;
};
extern __GLXprovider __glXDRISWRastProvider;

void GlxPushProvider(__GLXprovider * provider);
Bool xorgGlxCreateVendor(void);
#else
static inline Bool xorgGlxCreateVendor(void) { return TRUE; }
#endif


#endif
