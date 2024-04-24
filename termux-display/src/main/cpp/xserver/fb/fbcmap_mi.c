/*
 * Copyright (c) 1987, Oracle and/or its affiliates. All rights reserved.
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * This version of fbcmap.c is implemented in terms of mi functions.
 * These functions used to be in fbcmap.c and depended upon the symbol
 * XFree86Server being defined.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "fb.h"
#include "micmap.h"

int
fbListInstalledColormaps(ScreenPtr pScreen, Colormap * pmaps)
{
    return miListInstalledColormaps(pScreen, pmaps);
}

void
fbInstallColormap(ColormapPtr pmap)
{
    miInstallColormap(pmap);
}

void
fbUninstallColormap(ColormapPtr pmap)
{
    miUninstallColormap(pmap);
}

void
fbResolveColor(unsigned short *pred,
               unsigned short *pgreen, unsigned short *pblue, VisualPtr pVisual)
{
    miResolveColor(pred, pgreen, pblue, pVisual);
}

Bool
fbInitializeColormap(ColormapPtr pmap)
{
    return miInitializeColormap(pmap);
}

int
fbExpandDirectColors(ColormapPtr pmap,
                     int ndef, xColorItem * indefs, xColorItem * outdefs)
{
    return miExpandDirectColors(pmap, ndef, indefs, outdefs);
}

Bool
fbCreateDefColormap(ScreenPtr pScreen)
{
    return miCreateDefColormap(pScreen);
}

void
fbClearVisualTypes(void)
{
    miClearVisualTypes();
}

Bool
fbSetVisualTypes(int depth, int visuals, int bitsPerRGB)
{
    return miSetVisualTypes(depth, visuals, bitsPerRGB, -1);
}

Bool
fbSetVisualTypesAndMasks(int depth, int visuals, int bitsPerRGB,
                         Pixel redMask, Pixel greenMask, Pixel blueMask)
{
    return miSetVisualTypesAndMasks(depth, visuals, bitsPerRGB, -1,
                                    redMask, greenMask, blueMask);
}

/*
 * Given a list of formats for a screen, create a list
 * of visuals and depths for the screen which correspond to
 * the set which can be used with this version of fb.
 */
Bool
fbInitVisuals(VisualPtr * visualp,
              DepthPtr * depthp,
              int *nvisualp,
              int *ndepthp,
              int *rootDepthp,
              VisualID * defaultVisp, unsigned long sizes, int bitsPerRGB)
{
    return miInitVisuals(visualp, depthp, nvisualp, ndepthp, rootDepthp,
                         defaultVisp, sizes, bitsPerRGB, -1);
}
