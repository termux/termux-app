/*
 *
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

#include "fb.h"
#include	<X11/fonts/fontstruct.h>
#include	"dixfontstr.h"

static Bool
fbGlyphIn(RegionPtr pRegion, int x, int y, int width, int height)
{
    BoxRec box;
    BoxPtr pExtents = RegionExtents(pRegion);

    /*
     * Check extents by hand to avoid 16 bit overflows
     */
    if (x < (int) pExtents->x1)
        return FALSE;
    if ((int) pExtents->x2 < x + width)
        return FALSE;
    if (y < (int) pExtents->y1)
        return FALSE;
    if ((int) pExtents->y2 < y + height)
        return FALSE;
    box.x1 = x;
    box.x2 = x + width;
    box.y1 = y;
    box.y2 = y + height;
    return RegionContainsRect(pRegion, &box) == rgnIN;
}

void
fbPolyGlyphBlt(DrawablePtr pDrawable,
               GCPtr pGC,
               int x,
               int y,
               unsigned int nglyph, CharInfoPtr * ppci, void *pglyphBase)
{
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);
    CharInfoPtr pci;
    unsigned char *pglyph;      /* pointer bits in glyph */
    int gx, gy;
    int gWidth, gHeight;        /* width and height of glyph */
    FbStride gStride;           /* stride of glyph */
    void (*glyph) (FbBits *, FbStride, int, FbStip *, FbBits, int, int);
    FbBits *dst = 0;
    FbStride dstStride = 0;
    int dstBpp = 0;
    int dstXoff = 0, dstYoff = 0;

    glyph = 0;
    if (pGC->fillStyle == FillSolid && pPriv->and == 0) {
        dstBpp = pDrawable->bitsPerPixel;
        switch (dstBpp) {
        case 8:
            glyph = fbGlyph8;
            break;
        case 16:
            glyph = fbGlyph16;
            break;
        case 32:
            glyph = fbGlyph32;
            break;
        }
    }
    x += pDrawable->x;
    y += pDrawable->y;

    while (nglyph--) {
        pci = *ppci++;
        pglyph = FONTGLYPHBITS(pglyphBase, pci);
        gWidth = GLYPHWIDTHPIXELS(pci);
        gHeight = GLYPHHEIGHTPIXELS(pci);
        if (gWidth && gHeight) {
            gx = x + pci->metrics.leftSideBearing;
            gy = y - pci->metrics.ascent;
            if (glyph && gWidth <= sizeof(FbStip) * 8 &&
                fbGlyphIn(fbGetCompositeClip(pGC), gx, gy, gWidth, gHeight)) {
                fbGetDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff,
                              dstYoff);
                (*glyph) (dst + (gy + dstYoff) * dstStride, dstStride, dstBpp,
                          (FbStip *) pglyph, pPriv->xor, gx + dstXoff, gHeight);
                fbFinishAccess(pDrawable);
            }
            else {
                gStride = GLYPHWIDTHBYTESPADDED(pci) / sizeof(FbStip);
                fbPushImage(pDrawable,
                            pGC,
                            (FbStip *) pglyph,
                            gStride, 0, gx, gy, gWidth, gHeight);
            }
        }
        x += pci->metrics.characterWidth;
    }
}

void
fbImageGlyphBlt(DrawablePtr pDrawable,
                GCPtr pGC,
                int x,
                int y,
                unsigned int nglyph, CharInfoPtr * ppciInit, void *pglyphBase)
{
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);
    CharInfoPtr *ppci;
    CharInfoPtr pci;
    unsigned char *pglyph;      /* pointer bits in glyph */
    int gWidth, gHeight;        /* width and height of glyph */
    FbStride gStride;           /* stride of glyph */
    Bool opaque;
    int n;
    int gx, gy;
    void (*glyph) (FbBits *, FbStride, int, FbStip *, FbBits, int, int);
    FbBits *dst = 0;
    FbStride dstStride = 0;
    int dstBpp = 0;
    int dstXoff = 0, dstYoff = 0;

    glyph = 0;
    if (pPriv->and == 0) {
        dstBpp = pDrawable->bitsPerPixel;
        switch (dstBpp) {
        case 8:
            glyph = fbGlyph8;
            break;
        case 16:
            glyph = fbGlyph16;
            break;
        case 32:
            glyph = fbGlyph32;
            break;
        }
    }

    x += pDrawable->x;
    y += pDrawable->y;

    if (TERMINALFONT(pGC->font)
        && !glyph) {
        opaque = TRUE;
    }
    else {
        int xBack, widthBack;
        int yBack, heightBack;

        ppci = ppciInit;
        n = nglyph;
        widthBack = 0;
        while (n--)
            widthBack += (*ppci++)->metrics.characterWidth;

        xBack = x;
        if (widthBack < 0) {
            xBack += widthBack;
            widthBack = -widthBack;
        }
        yBack = y - FONTASCENT(pGC->font);
        heightBack = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);
        fbSolidBoxClipped(pDrawable,
                          fbGetCompositeClip(pGC),
                          xBack,
                          yBack,
                          xBack + widthBack,
                          yBack + heightBack,
                          fbAnd(GXcopy, pPriv->bg, pPriv->pm),
                          fbXor(GXcopy, pPriv->bg, pPriv->pm));
        opaque = FALSE;
    }

    ppci = ppciInit;
    while (nglyph--) {
        pci = *ppci++;
        pglyph = FONTGLYPHBITS(pglyphBase, pci);
        gWidth = GLYPHWIDTHPIXELS(pci);
        gHeight = GLYPHHEIGHTPIXELS(pci);
        if (gWidth && gHeight) {
            gx = x + pci->metrics.leftSideBearing;
            gy = y - pci->metrics.ascent;
            if (glyph && gWidth <= sizeof(FbStip) * 8 &&
                fbGlyphIn(fbGetCompositeClip(pGC), gx, gy, gWidth, gHeight)) {
                fbGetDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff,
                              dstYoff);
                (*glyph) (dst + (gy + dstYoff) * dstStride, dstStride, dstBpp,
                          (FbStip *) pglyph, pPriv->fg, gx + dstXoff, gHeight);
                fbFinishAccess(pDrawable);
            }
            else {
                gStride = GLYPHWIDTHBYTESPADDED(pci) / sizeof(FbStip);
                fbPutXYImage(pDrawable,
                             fbGetCompositeClip(pGC),
                             pPriv->fg,
                             pPriv->bg,
                             pPriv->pm,
                             GXcopy,
                             opaque,
                             gx,
                             gy,
                             gWidth, gHeight, (FbStip *) pglyph, gStride, 0);
            }
        }
        x += pci->metrics.characterWidth;
    }
}
