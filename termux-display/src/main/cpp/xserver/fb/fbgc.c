/*
 * Copyright © 1998 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "fb.h"

const GCFuncs fbGCFuncs = {
    fbValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip,
};

const GCOps fbGCOps = {
    fbFillSpans,
    fbSetSpans,
    fbPutImage,
    fbCopyArea,
    fbCopyPlane,
    fbPolyPoint,
    fbPolyLine,
    fbPolySegment,
    fbPolyRectangle,
    fbPolyArc,
    miFillPolygon,
    fbPolyFillRect,
    fbPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    fbImageGlyphBlt,
    fbPolyGlyphBlt,
    fbPushPixels
};

Bool
fbCreateGC(GCPtr pGC)
{
    pGC->ops = (GCOps *) &fbGCOps;
    pGC->funcs = (GCFuncs *) &fbGCFuncs;

    /* fb wants to translate before scan conversion */
    pGC->miTranslate = 1;
    pGC->fExpose = 1;

    return TRUE;
}

/*
 * Pad pixmap to FB_UNIT bits wide
 */
void
fbPadPixmap(PixmapPtr pPixmap)
{
    int width;
    FbBits *bits;
    FbBits b;
    FbBits mask;
    int height;
    int w;
    int stride;
    int bpp;
    _X_UNUSED int xOff, yOff;

    fbGetDrawable(&pPixmap->drawable, bits, stride, bpp, xOff, yOff);

    width = pPixmap->drawable.width * pPixmap->drawable.bitsPerPixel;
    height = pPixmap->drawable.height;
    mask = FbBitsMask(0, width);
    while (height--) {
        b = READ(bits) & mask;
        w = width;
        while (w < FB_UNIT) {
            b = b | FbScrRight(b, w);
            w <<= 1;
        }
        WRITE(bits, b);
        bits += stride;
    }

    fbFinishAccess(&pPixmap->drawable);
}

void
fbValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable)
{
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);
    FbBits mask;

    /*
     * if the client clip is different or moved OR the subwindowMode has
     * changed OR the window's clip has changed since the last validation
     * we need to recompute the composite clip
     */

    if ((changes &
         (GCClipXOrigin | GCClipYOrigin | GCClipMask | GCSubwindowMode)) ||
        (pDrawable->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS))
        ) {
        miComputeCompositeClip(pGC, pDrawable);
    }

    if (changes & GCTile) {
        if (!pGC->tileIsPixel &&
            FbEvenTile(pGC->tile.pixmap->drawable.width *
                       pDrawable->bitsPerPixel))
            fbPadPixmap(pGC->tile.pixmap);
    }
    if (changes & GCStipple) {
        if (pGC->stipple) {
            if (pGC->stipple->drawable.width * pDrawable->bitsPerPixel <
                FB_UNIT)
                fbPadPixmap(pGC->stipple);
        }
    }
    /*
     * Recompute reduced rop values
     */
    if (changes & (GCForeground | GCBackground | GCPlaneMask | GCFunction)) {
        int s;
        FbBits depthMask;

        mask = FbFullMask(pDrawable->bitsPerPixel);
        depthMask = FbFullMask(pDrawable->depth);

        pPriv->fg = pGC->fgPixel & mask;
        pPriv->bg = pGC->bgPixel & mask;

        if ((pGC->planemask & depthMask) == depthMask)
            pPriv->pm = mask;
        else
            pPriv->pm = pGC->planemask & mask;

        s = pDrawable->bitsPerPixel;
        while (s < FB_UNIT) {
            pPriv->fg |= pPriv->fg << s;
            pPriv->bg |= pPriv->bg << s;
            pPriv->pm |= pPriv->pm << s;
            s <<= 1;
        }
        pPriv->and = fbAnd(pGC->alu, pPriv->fg, pPriv->pm);
        pPriv->xor = fbXor(pGC->alu, pPriv->fg, pPriv->pm);
        pPriv->bgand = fbAnd(pGC->alu, pPriv->bg, pPriv->pm);
        pPriv->bgxor = fbXor(pGC->alu, pPriv->bg, pPriv->pm);
    }
    if (changes & GCDashList) {
        unsigned short n = pGC->numInDashList;
        unsigned char *dash = pGC->dash;
        unsigned int dashLength = 0;

        while (n--)
            dashLength += (unsigned int) *dash++;
        pPriv->dashLength = dashLength;
    }
}
