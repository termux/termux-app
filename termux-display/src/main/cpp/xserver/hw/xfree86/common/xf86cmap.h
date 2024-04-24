
/*
 * Copyright (c) 1998-2001 by The XFree86 Project, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifndef _XF86CMAP_H
#define _XF86CMAP_H

#include "xf86str.h"
#include "colormapst.h"

#define CMAP_PALETTED_TRUECOLOR		0x0000001
#define CMAP_RELOAD_ON_MODE_SWITCH	0x0000002
#define CMAP_LOAD_EVEN_IF_OFFSCREEN	0x0000004

extern _X_EXPORT Bool xf86HandleColormaps(ScreenPtr pScreen,
                                          int maxCol,
                                          int sigRGBbits,
                                          xf86LoadPaletteProc * loadPalette,
                                          xf86SetOverscanProc * setOverscan,
                                          unsigned int flags);

extern _X_EXPORT Bool xf86ColormapAllocatePrivates(ScrnInfoPtr pScrn);

extern _X_EXPORT int
 xf86ChangeGamma(ScreenPtr pScreen, Gamma newGamma);

extern _X_EXPORT int

xf86ChangeGammaRamp(ScreenPtr pScreen,
                    int size,
                    unsigned short *red,
                    unsigned short *green, unsigned short *blue);

extern _X_EXPORT int xf86GetGammaRampSize(ScreenPtr pScreen);

extern _X_EXPORT int

xf86GetGammaRamp(ScreenPtr pScreen,
                 int size,
                 unsigned short *red,
                 unsigned short *green, unsigned short *blue);

#endif                          /* _XF86CMAP_H */
