/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* Author: Todd Newman  (aided and abetted by Mr. Drewry) */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xprotostr.h>

#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "mi.h"
#include "regionstr.h"
#include <X11/Xmd.h>
#include "servermd.h"

#ifdef __MINGW32__
#define ffs __builtin_ffs
#endif

/* MICOPYAREA -- public entry for the CopyArea request
 * For each rectangle in the source region
 *     get the pixels with GetSpans
 *     set them in the destination with SetSpans
 * We let SetSpans worry about clipping to the destination.
 */
_X_COLD RegionPtr
miCopyArea(DrawablePtr pSrcDrawable,
           DrawablePtr pDstDrawable,
           GCPtr pGC,
           int xIn, int yIn, int widthSrc, int heightSrc, int xOut, int yOut)
{
    DDXPointPtr ppt, pptFirst;
    unsigned int *pwidthFirst, *pwidth, *pbits;
    BoxRec srcBox, *prect;

    /* may be a new region, or just a copy */
    RegionPtr prgnSrcClip;

    /* non-0 if we've created a src clip */
    RegionPtr prgnExposed;
    int realSrcClip = 0;
    int srcx, srcy, dstx, dsty, i, j, y, width, height, xMin, xMax, yMin, yMax;
    unsigned int *ordering;
    int numRects;
    BoxPtr boxes;

    srcx = xIn + pSrcDrawable->x;
    srcy = yIn + pSrcDrawable->y;

    /* If the destination isn't realized, this is easy */
    if (pDstDrawable->type == DRAWABLE_WINDOW &&
        !((WindowPtr) pDstDrawable)->realized)
        return NULL;

    /* clip the source */
    if (pSrcDrawable->type == DRAWABLE_PIXMAP) {
        BoxRec box;

        box.x1 = pSrcDrawable->x;
        box.y1 = pSrcDrawable->y;
        box.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
        box.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;

        prgnSrcClip = RegionCreate(&box, 1);
        realSrcClip = 1;
    }
    else {
        if (pGC->subWindowMode == IncludeInferiors) {
            prgnSrcClip = NotClippedByChildren((WindowPtr) pSrcDrawable);
            realSrcClip = 1;
        }
        else
            prgnSrcClip = &((WindowPtr) pSrcDrawable)->clipList;
    }

    /* If the src drawable is a window, we need to translate the srcBox so
     * that we can compare it with the window's clip region later on. */
    srcBox.x1 = srcx;
    srcBox.y1 = srcy;
    srcBox.x2 = srcx + widthSrc;
    srcBox.y2 = srcy + heightSrc;

    dstx = xOut;
    dsty = yOut;
    if (pGC->miTranslate) {
        dstx += pDstDrawable->x;
        dsty += pDstDrawable->y;
    }

    pptFirst = ppt = xallocarray(heightSrc, sizeof(DDXPointRec));
    pwidthFirst = pwidth = xallocarray(heightSrc, sizeof(unsigned int));
    numRects = RegionNumRects(prgnSrcClip);
    boxes = RegionRects(prgnSrcClip);
    ordering = xallocarray(numRects, sizeof(unsigned int));
    if (!pptFirst || !pwidthFirst || !ordering) {
        free(ordering);
        free(pwidthFirst);
        free(pptFirst);
        if (realSrcClip)
            RegionDestroy(prgnSrcClip);
        return NULL;
    }

    /* If not the same drawable then order of move doesn't matter.
       Following assumes that boxes are sorted from top
       to bottom and left to right.
     */
    if ((pSrcDrawable != pDstDrawable) &&
        ((pGC->subWindowMode != IncludeInferiors) ||
         (pSrcDrawable->type == DRAWABLE_PIXMAP) ||
         (pDstDrawable->type == DRAWABLE_PIXMAP)))
        for (i = 0; i < numRects; i++)
            ordering[i] = i;
    else {                      /* within same drawable, must sequence moves carefully! */
        if (dsty <= srcBox.y1) {        /* Scroll up or stationary vertical.
                                           Vertical order OK */
            if (dstx <= srcBox.x1)      /* Scroll left or stationary horizontal.
                                           Horizontal order OK as well */
                for (i = 0; i < numRects; i++)
                    ordering[i] = i;
            else {              /* scroll right. must reverse horizontal banding of rects. */
                for (i = 0, j = 1, xMax = 0; i < numRects; j = i + 1, xMax = i) {
                    /* find extent of current horizontal band */
                    y = boxes[i].y1;    /* band has this y coordinate */
                    while ((j < numRects) && (boxes[j].y1 == y))
                        j++;
                    /* reverse the horizontal band in the output ordering */
                    for (j--; j >= xMax; j--, i++)
                        ordering[i] = j;
                }
            }
        }
        else {                  /* Scroll down. Must reverse vertical banding. */
            if (dstx < srcBox.x1) {     /* Scroll left. Horizontal order OK. */
                for (i = numRects - 1, j = i - 1, yMin = i, yMax = 0;
                     i >= 0; j = i - 1, yMin = i) {
                    /* find extent of current horizontal band */
                    y = boxes[i].y1;    /* band has this y coordinate */
                    while ((j >= 0) && (boxes[j].y1 == y))
                        j--;
                    /* reverse the horizontal band in the output ordering */
                    for (j++; j <= yMin; j++, i--, yMax++)
                        ordering[yMax] = j;
                }
            }
            else                /* Scroll right or horizontal stationary.
                                   Reverse horizontal order as well (if stationary, horizontal
                                   order can be swapped without penalty and this is faster
                                   to compute). */
                for (i = 0, j = numRects - 1; i < numRects; i++, j--)
                    ordering[i] = j;
        }
    }

    for (i = 0; i < numRects; i++) {
        prect = &boxes[ordering[i]];
        xMin = max(prect->x1, srcBox.x1);
        xMax = min(prect->x2, srcBox.x2);
        yMin = max(prect->y1, srcBox.y1);
        yMax = min(prect->y2, srcBox.y2);
        /* is there anything visible here? */
        if (xMax <= xMin || yMax <= yMin)
            continue;

        ppt = pptFirst;
        pwidth = pwidthFirst;
        y = yMin;
        height = yMax - yMin;
        width = xMax - xMin;

        for (j = 0; j < height; j++) {
            /* We must untranslate before calling GetSpans */
            ppt->x = xMin;
            ppt++->y = y++;
            *pwidth++ = width;
        }
        pbits = xallocarray(height, PixmapBytePad(width, pSrcDrawable->depth));
        if (pbits) {
            (*pSrcDrawable->pScreen->GetSpans) (pSrcDrawable, width, pptFirst,
                                                (int *) pwidthFirst, height,
                                                (char *) pbits);
            ppt = pptFirst;
            pwidth = pwidthFirst;
            xMin -= (srcx - dstx);
            y = yMin - (srcy - dsty);
            for (j = 0; j < height; j++) {
                ppt->x = xMin;
                ppt++->y = y++;
                *pwidth++ = width;
            }

            (*pGC->ops->SetSpans) (pDstDrawable, pGC, (char *) pbits, pptFirst,
                                   (int *) pwidthFirst, height, TRUE);
            free(pbits);
        }
    }
    prgnExposed = miHandleExposures(pSrcDrawable, pDstDrawable, pGC, xIn, yIn,
                                    widthSrc, heightSrc, xOut, yOut);
    if (realSrcClip)
        RegionDestroy(prgnSrcClip);

    free(ordering);
    free(pwidthFirst);
    free(pptFirst);
    return prgnExposed;
}

/* MIGETPLANE -- gets a bitmap representing one plane of pDraw
 * A helper used for CopyPlane and XY format GetImage
 * No clever strategy here, we grab a scanline at a time, pull out the
 * bits and then stuff them in a 1 bit deep map.
 */
/*
 * This should be replaced with something more general.  mi shouldn't have to
 * care about such things as scanline padding et alia.
 */
_X_COLD static MiBits *
miGetPlane(DrawablePtr pDraw, int planeNum,     /* number of the bitPlane */
           int sx, int sy, int w, int h, MiBits * result)
{
    int i, j, k, width, bitsPerPixel, widthInBytes;
    DDXPointRec pt = { 0, 0 };
    MiBits pixel;
    MiBits bit;
    unsigned char *pCharsOut = NULL;

#if BITMAP_SCANLINE_UNIT == 8
#define OUT_TYPE unsigned char
#endif
#if BITMAP_SCANLINE_UNIT == 16
#define OUT_TYPE CARD16
#endif
#if BITMAP_SCANLINE_UNIT == 32
#define OUT_TYPE CARD32
#endif
#if BITMAP_SCANLINE_UNIT == 64
#define OUT_TYPE CARD64
#endif

    OUT_TYPE *pOut;
    int delta = 0;

    sx += pDraw->x;
    sy += pDraw->y;
    widthInBytes = BitmapBytePad(w);
    if (!result)
        result = calloc(h, widthInBytes);
    if (!result)
        return NULL;
    bitsPerPixel = pDraw->bitsPerPixel;
    pOut = (OUT_TYPE *) result;
    if (bitsPerPixel == 1) {
        pCharsOut = (unsigned char *) result;
        width = w;
    }
    else {
        delta = (widthInBytes / (BITMAP_SCANLINE_UNIT / 8)) -
            (w / BITMAP_SCANLINE_UNIT);
        width = 1;
#if IMAGE_BYTE_ORDER == MSBFirst
        planeNum += (32 - bitsPerPixel);
#endif
    }
    pt.y = sy;
    for (i = h; --i >= 0; pt.y++) {
        pt.x = sx;
        if (bitsPerPixel == 1) {
            (*pDraw->pScreen->GetSpans) (pDraw, width, &pt, &width, 1,
                                         (char *) pCharsOut);
            pCharsOut += widthInBytes;
        }
        else {
            k = 0;
            for (j = w; --j >= 0; pt.x++) {
                /* Fetch the next pixel */
                (*pDraw->pScreen->GetSpans) (pDraw, width, &pt, &width, 1,
                                             (char *) &pixel);
                /*
                 * Now get the bit and insert into a bitmap in XY format.
                 */
                bit = (pixel >> planeNum) & 1;
#if 0
                /* XXX assuming bit order == byte order */
#if BITMAP_BIT_ORDER == LSBFirst
                bit <<= k;
#else
                bit <<= ((BITMAP_SCANLINE_UNIT - 1) - k);
#endif
#else
                /* XXX assuming byte order == LSBFirst */
                if (screenInfo.bitmapBitOrder == LSBFirst)
                    bit <<= k;
                else
                    bit <<= ((screenInfo.bitmapScanlineUnit - 1) -
                             (k % screenInfo.bitmapScanlineUnit)) +
                        ((k / screenInfo.bitmapScanlineUnit) *
                         screenInfo.bitmapScanlineUnit);
#endif
                *pOut |= (OUT_TYPE) bit;
                k++;
                if (k == BITMAP_SCANLINE_UNIT) {
                    pOut++;
                    k = 0;
                }
            }
            pOut += delta;
        }
    }
    return result;

}

/* MIOPQSTIPDRAWABLE -- use pbits as an opaque stipple for pDraw.
 * Drawing through the clip mask we SetSpans() the bits into a
 * bitmap and stipple those bits onto the destination drawable by doing a
 * PolyFillRect over the whole drawable,
 * then we invert the bitmap by copying it onto itself with an alu of
 * GXinvert, invert the foreground/background colors of the gc, and draw
 * the background bits.
 * Note how the clipped out bits of the bitmap are always the background
 * color so that the stipple never causes FillRect to draw them.
 */
_X_COLD static void
miOpqStipDrawable(DrawablePtr pDraw, GCPtr pGC, RegionPtr prgnSrc,
                  MiBits * pbits, int srcx, int w, int h, int dstx, int dsty)
{
    int oldfill, i;
    unsigned long oldfg;
    int *pwidth, *pwidthFirst;
    ChangeGCVal gcv[6];
    PixmapPtr pStipple, pPixmap;
    DDXPointRec oldOrg;
    GCPtr pGCT;
    DDXPointPtr ppt, pptFirst;
    xRectangle rect;
    RegionPtr prgnSrcClip;

    pPixmap = (*pDraw->pScreen->CreatePixmap)
        (pDraw->pScreen, w + srcx, h, 1, CREATE_PIXMAP_USAGE_SCRATCH);
    if (!pPixmap)
        return;

    /* Put the image into a 1 bit deep pixmap */
    pGCT = GetScratchGC(1, pDraw->pScreen);
    if (!pGCT) {
        (*pDraw->pScreen->DestroyPixmap) (pPixmap);
        return;
    }
    /* First set the whole pixmap to 0 */
    gcv[0].val = 0;
    ChangeGC(NullClient, pGCT, GCBackground, gcv);
    ValidateGC((DrawablePtr) pPixmap, pGCT);
    miClearDrawable((DrawablePtr) pPixmap, pGCT);
    ppt = pptFirst = xallocarray(h, sizeof(DDXPointRec));
    pwidth = pwidthFirst = xallocarray(h, sizeof(int));
    if (!pptFirst || !pwidthFirst) {
        free(pwidthFirst);
        free(pptFirst);
        FreeScratchGC(pGCT);
        return;
    }

    /* we need a temporary region because ChangeClip must be assumed
       to destroy what it's sent.  note that this means we don't
       have to free prgnSrcClip ourselves.
     */
    prgnSrcClip = RegionCreate(NULL, 0);
    RegionCopy(prgnSrcClip, prgnSrc);
    RegionTranslate(prgnSrcClip, srcx, 0);
    (*pGCT->funcs->ChangeClip) (pGCT, CT_REGION, prgnSrcClip, 0);
    ValidateGC((DrawablePtr) pPixmap, pGCT);

    /* Since we know pDraw is always a pixmap, we never need to think
     * about translation here */
    for (i = 0; i < h; i++) {
        ppt->x = 0;
        ppt++->y = i;
        *pwidth++ = w + srcx;
    }

    (*pGCT->ops->SetSpans) ((DrawablePtr) pPixmap, pGCT, (char *) pbits,
                            pptFirst, pwidthFirst, h, TRUE);
    free(pwidthFirst);
    free(pptFirst);

    /* Save current values from the client GC */
    oldfill = pGC->fillStyle;
    pStipple = pGC->stipple;
    if (pStipple)
        pStipple->refcnt++;
    oldOrg = pGC->patOrg;

    /* Set a new stipple in the drawable */
    gcv[0].val = FillStippled;
    gcv[1].ptr = pPixmap;
    gcv[2].val = dstx - srcx;
    gcv[3].val = dsty;

    ChangeGC(NullClient, pGC,
             GCFillStyle | GCStipple | GCTileStipXOrigin | GCTileStipYOrigin,
             gcv);
    ValidateGC(pDraw, pGC);

    /* Fill the drawable with the stipple.  This will draw the
     * foreground color wherever 1 bits are set, leaving everything
     * with 0 bits untouched.  Note that the part outside the clip
     * region is all 0s.  */
    rect.x = dstx;
    rect.y = dsty;
    rect.width = w;
    rect.height = h;
    (*pGC->ops->PolyFillRect) (pDraw, pGC, 1, &rect);

    /* Invert the tiling pixmap. This sets 0s for 1s and 1s for 0s, only
     * within the clipping region, the part outside is still all 0s */
    gcv[0].val = GXinvert;
    ChangeGC(NullClient, pGCT, GCFunction, gcv);
    ValidateGC((DrawablePtr) pPixmap, pGCT);
    (*pGCT->ops->CopyArea) ((DrawablePtr) pPixmap, (DrawablePtr) pPixmap,
                            pGCT, 0, 0, w + srcx, h, 0, 0);

    /* Swap foreground and background colors on the GC for the drawable.
     * Now when we fill the drawable, we will fill in the "Background"
     * values */
    oldfg = pGC->fgPixel;
    gcv[0].val = pGC->bgPixel;
    gcv[1].val = oldfg;
    gcv[2].ptr = pPixmap;
    ChangeGC(NullClient, pGC, GCForeground | GCBackground | GCStipple, gcv);
    ValidateGC(pDraw, pGC);
    /* PolyFillRect might have bashed the rectangle */
    rect.x = dstx;
    rect.y = dsty;
    rect.width = w;
    rect.height = h;
    (*pGC->ops->PolyFillRect) (pDraw, pGC, 1, &rect);

    /* Now put things back */
    if (pStipple)
        pStipple->refcnt--;
    gcv[0].val = oldfg;
    gcv[1].val = pGC->fgPixel;
    gcv[2].val = oldfill;
    gcv[3].ptr = pStipple;
    gcv[4].val = oldOrg.x;
    gcv[5].val = oldOrg.y;
    ChangeGC(NullClient, pGC,
             GCForeground | GCBackground | GCFillStyle | GCStipple |
             GCTileStipXOrigin | GCTileStipYOrigin, gcv);

    ValidateGC(pDraw, pGC);
    /* put what we hope is a smaller clip region back in the scratch gc */
    (*pGCT->funcs->ChangeClip) (pGCT, CT_NONE, NULL, 0);
    FreeScratchGC(pGCT);
    (*pDraw->pScreen->DestroyPixmap) (pPixmap);

}

/* MICOPYPLANE -- public entry for the CopyPlane request.
 * strategy:
 * First build up a bitmap out of the bits requested
 * build a source clip
 * Use the bitmap we've built up as a Stipple for the destination
 */
_X_COLD RegionPtr
miCopyPlane(DrawablePtr pSrcDrawable,
            DrawablePtr pDstDrawable,
            GCPtr pGC,
            int srcx,
            int srcy,
            int width, int height, int dstx, int dsty, unsigned long bitPlane)
{
    MiBits *ptile;
    BoxRec box;
    RegionPtr prgnSrc, prgnExposed;

    /* incorporate the source clip */

    box.x1 = srcx + pSrcDrawable->x;
    box.y1 = srcy + pSrcDrawable->y;
    box.x2 = box.x1 + width;
    box.y2 = box.y1 + height;
    /* clip to visible drawable */
    if (box.x1 < pSrcDrawable->x)
        box.x1 = pSrcDrawable->x;
    if (box.y1 < pSrcDrawable->y)
        box.y1 = pSrcDrawable->y;
    if (box.x2 > pSrcDrawable->x + (int) pSrcDrawable->width)
        box.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
    if (box.y2 > pSrcDrawable->y + (int) pSrcDrawable->height)
        box.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;
    if (box.x1 > box.x2)
        box.x2 = box.x1;
    if (box.y1 > box.y2)
        box.y2 = box.y1;
    prgnSrc = RegionCreate(&box, 1);

    if (pSrcDrawable->type != DRAWABLE_PIXMAP) {
        /* clip to visible drawable */

        if (pGC->subWindowMode == IncludeInferiors) {
            RegionPtr clipList = NotClippedByChildren((WindowPtr) pSrcDrawable);

            RegionIntersect(prgnSrc, prgnSrc, clipList);
            RegionDestroy(clipList);
        }
        else
            RegionIntersect(prgnSrc, prgnSrc,
                            &((WindowPtr) pSrcDrawable)->clipList);
    }

    box = *RegionExtents(prgnSrc);
    RegionTranslate(prgnSrc, -box.x1, -box.y1);

    if ((box.x2 > box.x1) && (box.y2 > box.y1)) {
        /* minimize the size of the data extracted */
        /* note that we convert the plane mask bitPlane into a plane number */
        box.x1 -= pSrcDrawable->x;
        box.x2 -= pSrcDrawable->x;
        box.y1 -= pSrcDrawable->y;
        box.y2 -= pSrcDrawable->y;
        ptile = miGetPlane(pSrcDrawable, ffs(bitPlane) - 1,
                           box.x1, box.y1,
                           box.x2 - box.x1, box.y2 - box.y1, (MiBits *) NULL);
        if (ptile) {
            miOpqStipDrawable(pDstDrawable, pGC, prgnSrc, ptile, 0,
                              box.x2 - box.x1, box.y2 - box.y1,
                              dstx + box.x1 - srcx, dsty + box.y1 - srcy);
            free(ptile);
        }
    }
    prgnExposed = miHandleExposures(pSrcDrawable, pDstDrawable, pGC, srcx, srcy,
                                    width, height, dstx, dsty);
    RegionDestroy(prgnSrc);
    return prgnExposed;
}

/* MIGETIMAGE -- public entry for the GetImage Request
 * We're getting the image into a memory buffer. While we have to use GetSpans
 * to read a line from the device (since we don't know what that looks like),
 * we can just write into the destination buffer
 *
 * two different strategies are used, depending on whether we're getting the
 * image in Z format or XY format
 * Z format:
 * Line at a time, GetSpans a line into the destination buffer, then if the
 * planemask is not all ones, we do a SetSpans into a temporary buffer (to get
 * bits turned off) and then another GetSpans to get stuff back (because
 * pixmaps are opaque, and we are passed in the memory to write into).  This is
 * pretty ugly and slow but works.  Life is hard.
 * XY format:
 * get the single plane specified in planemask
 */
_X_COLD void
miGetImage(DrawablePtr pDraw, int sx, int sy, int w, int h,
           unsigned int format, unsigned long planeMask, char *pDst)
{
    unsigned char depth;
    int i, linelength, width, srcx, srcy;
    DDXPointRec pt = { 0, 0 };
    PixmapPtr pPixmap = NULL;
    GCPtr pGC = NULL;

    depth = pDraw->depth;
    if (format == ZPixmap) {
        if ((((1LL << depth) - 1) & planeMask) != (1LL << depth) - 1) {
            ChangeGCVal gcv;
            xPoint xpt;

            pGC = GetScratchGC(depth, pDraw->pScreen);
            if (!pGC)
                return;
            pPixmap = (*pDraw->pScreen->CreatePixmap)
                (pDraw->pScreen, w, 1, depth, CREATE_PIXMAP_USAGE_SCRATCH);
            if (!pPixmap) {
                FreeScratchGC(pGC);
                return;
            }
            /*
             * Clear the pixmap before doing anything else
             */
            ValidateGC((DrawablePtr) pPixmap, pGC);
            xpt.x = xpt.y = 0;
            width = w;
            (*pGC->ops->FillSpans) ((DrawablePtr) pPixmap, pGC, 1, &xpt, &width,
                                    TRUE);

            /* alu is already GXCopy */
            gcv.val = (XID) planeMask;
            ChangeGC(NullClient, pGC, GCPlaneMask, &gcv);
            ValidateGC((DrawablePtr) pPixmap, pGC);
        }

        linelength = PixmapBytePad(w, depth);
        srcx = sx + pDraw->x;
        srcy = sy + pDraw->y;
        for (i = 0; i < h; i++) {
            pt.x = srcx;
            pt.y = srcy + i;
            width = w;
            (*pDraw->pScreen->GetSpans) (pDraw, w, &pt, &width, 1, pDst);
            if (pPixmap) {
                pt.x = 0;
                pt.y = 0;
                width = w;
                (*pGC->ops->SetSpans) ((DrawablePtr) pPixmap, pGC, pDst,
                                       &pt, &width, 1, TRUE);
                (*pDraw->pScreen->GetSpans) ((DrawablePtr) pPixmap, w, &pt,
                                             &width, 1, pDst);
            }
            pDst += linelength;
        }
        if (pPixmap) {
            (*pGC->pScreen->DestroyPixmap) (pPixmap);
            FreeScratchGC(pGC);
        }
    }
    else {
        (void) miGetPlane(pDraw, ffs(planeMask) - 1, sx, sy, w, h,
                          (MiBits *) pDst);
    }
}

/* MIPUTIMAGE -- public entry for the PutImage request
 * Here we benefit from knowing the format of the bits pointed to by pImage,
 * even if we don't know how pDraw represents them.
 * Three different strategies are used depending on the format
 * XYBitmap Format:
 * 	we just use the Opaque Stipple helper function to cover the destination
 * 	Note that this covers all the planes of the drawable with the
 *	foreground color (masked with the GC planemask) where there are 1 bits
 *	and the background color (masked with the GC planemask) where there are
 *	0 bits
 * XYPixmap format:
 *	what we're called with is a series of XYBitmaps, but we only want
 *	each XYPixmap to update 1 plane, instead of updating all of them.
 * 	we set the foreground color to be all 1s and the background to all 0s
 *	then for each plane, we set the plane mask to only effect that one
 *	plane and recursive call ourself with the format set to XYBitmap
 *	(This clever idea courtesy of RGD.)
 * ZPixmap format:
 *	This part is simple, just call SetSpans
 */
_X_COLD void
miPutImage(DrawablePtr pDraw, GCPtr pGC, int depth,
           int x, int y, int w, int h, int leftPad, int format, char *pImage)
{
    DDXPointPtr pptFirst, ppt;
    int *pwidthFirst, *pwidth;
    RegionPtr prgnSrc;
    BoxRec box;
    unsigned long oldFg, oldBg;
    ChangeGCVal gcv[3];
    unsigned long oldPlanemask;
    unsigned long i;
    long bytesPer;

    if (!w || !h)
        return;
    switch (format) {
    case XYBitmap:

        box.x1 = 0;
        box.y1 = 0;
        box.x2 = w;
        box.y2 = h;
        prgnSrc = RegionCreate(&box, 1);

        miOpqStipDrawable(pDraw, pGC, prgnSrc, (MiBits *) pImage,
                          leftPad, w, h, x, y);
        RegionDestroy(prgnSrc);
        break;

    case XYPixmap:
        depth = pGC->depth;
        oldPlanemask = pGC->planemask;
        oldFg = pGC->fgPixel;
        oldBg = pGC->bgPixel;
        gcv[0].val = (XID) ~0;
        gcv[1].val = (XID) 0;
        ChangeGC(NullClient, pGC, GCForeground | GCBackground, gcv);
        bytesPer = (long) h *BitmapBytePad(w + leftPad);

        for (i = (unsigned long) 1 << (depth - 1); i != 0; i >>= 1, pImage += bytesPer) {
            if (i & oldPlanemask) {
                gcv[0].val = (XID) i;
                ChangeGC(NullClient, pGC, GCPlaneMask, gcv);
                ValidateGC(pDraw, pGC);
                (*pGC->ops->PutImage) (pDraw, pGC, 1, x, y, w, h, leftPad,
                                       XYBitmap, (char *) pImage);
            }
        }
        gcv[0].val = (XID) oldPlanemask;
        gcv[1].val = (XID) oldFg;
        gcv[2].val = (XID) oldBg;
        ChangeGC(NullClient, pGC, GCPlaneMask | GCForeground | GCBackground,
                 gcv);
        ValidateGC(pDraw, pGC);
        break;

    case ZPixmap:
        ppt = pptFirst = xallocarray(h, sizeof(DDXPointRec));
        pwidth = pwidthFirst = xallocarray(h, sizeof(int));
        if (!pptFirst || !pwidthFirst) {
            free(pwidthFirst);
            free(pptFirst);
            return;
        }
        if (pGC->miTranslate) {
            x += pDraw->x;
            y += pDraw->y;
        }

        for (i = 0; i < h; i++) {
            ppt->x = x;
            ppt->y = y + i;
            ppt++;
            *pwidth++ = w;
        }

        (*pGC->ops->SetSpans) (pDraw, pGC, (char *) pImage, pptFirst,
                               pwidthFirst, h, TRUE);
        free(pwidthFirst);
        free(pptFirst);
        break;
    }
}
