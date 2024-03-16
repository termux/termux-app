/*
 * Copyright (C) 2009 Apple, Inc.
 * Copyright (c) 2001-2004 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifndef _DARWIN_FB_H
#define _DARWIN_FB_H

#include "scrnintstr.h"

typedef struct {
    void                *framebuffer;
    int x;
    int y;
    int width;
    int height;
    int pitch;
    int depth;
    int visuals;
    int bitsPerRGB;
    int bitsPerPixel;
    int preferredCVC;
    Pixel redMask;
    Pixel greenMask;
    Pixel blueMask;
} DarwinFramebufferRec, *DarwinFramebufferPtr;

#define MASK_LH(l, h)       (((1 << (1 + (h) - (l))) - 1) << (l))
#define BM_ARGB(a, r, g, b) MASK_LH(0, (b) - 1)
#define GM_ARGB(a, r, g, b) MASK_LH(b, (b) + (g) - 1)
#define RM_ARGB(a, r, g, b) MASK_LH((b) + (g), (b) + (g) + (r) - 1)
#define AM_ARGB(a, r, g, b) MASK_LH((b) + (g) + (r), \
                                    (b) + (g) + (r) + (a) - 1)

#endif  /* _DARWIN_FB_H */
