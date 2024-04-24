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

void
fbCopyNtoN(DrawablePtr pSrcDrawable,
           DrawablePtr pDstDrawable,
           GCPtr pGC,
           BoxPtr pbox,
           int nbox,
           int dx,
           int dy, Bool reverse, Bool upsidedown, Pixel bitplane, void *closure)
{
    CARD8 alu = pGC ? pGC->alu : GXcopy;
    FbBits pm = pGC ? fbGetGCPrivate(pGC)->pm : FB_ALLONES;
    FbBits *src;
    FbStride srcStride;
    int srcBpp;
    int srcXoff, srcYoff;
    FbBits *dst;
    FbStride dstStride;
    int dstBpp;
    int dstXoff, dstYoff;

    fbGetDrawable(pSrcDrawable, src, srcStride, srcBpp, srcXoff, srcYoff);
    fbGetDrawable(pDstDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);

    while (nbox--) {
#ifndef FB_ACCESS_WRAPPER       /* pixman_blt() doesn't support accessors yet */
        if (pm == FB_ALLONES && alu == GXcopy && !reverse && !upsidedown) {
            if (!pixman_blt
                ((uint32_t *) src, (uint32_t *) dst, srcStride, dstStride,
                 srcBpp, dstBpp, (pbox->x1 + dx + srcXoff),
                 (pbox->y1 + dy + srcYoff), (pbox->x1 + dstXoff),
                 (pbox->y1 + dstYoff), (pbox->x2 - pbox->x1),
                 (pbox->y2 - pbox->y1)))
                goto fallback;
            else
                goto next;
        }
 fallback:
#endif
        fbBlt(src + (pbox->y1 + dy + srcYoff) * srcStride,
              srcStride,
              (pbox->x1 + dx + srcXoff) * srcBpp,
              dst + (pbox->y1 + dstYoff) * dstStride,
              dstStride,
              (pbox->x1 + dstXoff) * dstBpp,
              (pbox->x2 - pbox->x1) * dstBpp,
              (pbox->y2 - pbox->y1), alu, pm, dstBpp, reverse, upsidedown);
#ifndef FB_ACCESS_WRAPPER
 next:
#endif
        pbox++;
    }
    fbFinishAccess(pDstDrawable);
    fbFinishAccess(pSrcDrawable);
}

void
fbCopy1toN(DrawablePtr pSrcDrawable,
           DrawablePtr pDstDrawable,
           GCPtr pGC,
           BoxPtr pbox,
           int nbox,
           int dx,
           int dy, Bool reverse, Bool upsidedown, Pixel bitplane, void *closure)
{
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);
    FbBits *src;
    FbStride srcStride;
    int srcBpp;
    int srcXoff, srcYoff;
    FbBits *dst;
    FbStride dstStride;
    int dstBpp;
    int dstXoff, dstYoff;

    fbGetDrawable(pSrcDrawable, src, srcStride, srcBpp, srcXoff, srcYoff);
    fbGetDrawable(pDstDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);

    while (nbox--) {
        if (dstBpp == 1) {
            fbBlt(src + (pbox->y1 + dy + srcYoff) * srcStride,
                  srcStride,
                  (pbox->x1 + dx + srcXoff) * srcBpp,
                  dst + (pbox->y1 + dstYoff) * dstStride,
                  dstStride,
                  (pbox->x1 + dstXoff) * dstBpp,
                  (pbox->x2 - pbox->x1) * dstBpp,
                  (pbox->y2 - pbox->y1),
                  FbOpaqueStipple1Rop(pGC->alu,
                                      pGC->fgPixel, pGC->bgPixel),
                  pPriv->pm, dstBpp, reverse, upsidedown);
        }
        else {
            fbBltOne((FbStip *) (src + (pbox->y1 + dy + srcYoff) * srcStride),
                     srcStride * (FB_UNIT / FB_STIP_UNIT),
                     (pbox->x1 + dx + srcXoff),
                     dst + (pbox->y1 + dstYoff) * dstStride,
                     dstStride,
                     (pbox->x1 + dstXoff) * dstBpp,
                     dstBpp,
                     (pbox->x2 - pbox->x1) * dstBpp,
                     (pbox->y2 - pbox->y1),
                     pPriv->and, pPriv->xor, pPriv->bgand, pPriv->bgxor);
        }
        pbox++;
    }

    fbFinishAccess(pDstDrawable);
    fbFinishAccess(pSrcDrawable);
}

void
fbCopyNto1(DrawablePtr pSrcDrawable,
           DrawablePtr pDstDrawable,
           GCPtr pGC,
           BoxPtr pbox,
           int nbox,
           int dx,
           int dy, Bool reverse, Bool upsidedown, Pixel bitplane, void *closure)
{
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);

    while (nbox--) {
        if (pDstDrawable->bitsPerPixel == 1) {
            FbBits *src;
            FbStride srcStride;
            int srcBpp;
            int srcXoff, srcYoff;

            FbStip *dst;
            FbStride dstStride;
            int dstBpp;
            int dstXoff, dstYoff;

            fbGetDrawable(pSrcDrawable, src, srcStride, srcBpp, srcXoff,
                          srcYoff);
            fbGetStipDrawable(pDstDrawable, dst, dstStride, dstBpp, dstXoff,
                              dstYoff);
            fbBltPlane(src + (pbox->y1 + dy + srcYoff) * srcStride, srcStride,
                       (pbox->x1 + dx + srcXoff) * srcBpp, srcBpp,
                       dst + (pbox->y1 + dstYoff) * dstStride, dstStride,
                       (pbox->x1 + dstXoff) * dstBpp,
                       (pbox->x2 - pbox->x1) * srcBpp, (pbox->y2 - pbox->y1),
                       (FbStip) pPriv->and, (FbStip) pPriv->xor,
                       (FbStip) pPriv->bgand, (FbStip) pPriv->bgxor, bitplane);
            fbFinishAccess(pDstDrawable);
            fbFinishAccess(pSrcDrawable);
        }
        else {
            FbBits *src;
            FbStride srcStride;
            int srcBpp;
            int srcXoff, srcYoff;

            FbBits *dst;
            FbStride dstStride;
            int dstBpp;
            int dstXoff, dstYoff;

            FbStip *tmp;
            FbStride tmpStride;
            int width, height;

            width = pbox->x2 - pbox->x1;
            height = pbox->y2 - pbox->y1;

            tmpStride = ((width + FB_STIP_MASK) >> FB_STIP_SHIFT);
            tmp = xallocarray(tmpStride * height, sizeof(FbStip));
            if (!tmp)
                return;

            fbGetDrawable(pSrcDrawable, src, srcStride, srcBpp, srcXoff,
                          srcYoff);
            fbGetDrawable(pDstDrawable, dst, dstStride, dstBpp, dstXoff,
                          dstYoff);

            fbBltPlane(src + (pbox->y1 + dy + srcYoff) * srcStride,
                       srcStride,
                       (pbox->x1 + dx + srcXoff) * srcBpp,
                       srcBpp,
                       tmp,
                       tmpStride,
                       0,
                       width * srcBpp,
                       height,
                       fbAndStip(GXcopy, FB_ALLONES, FB_ALLONES),
                       fbXorStip(GXcopy, FB_ALLONES, FB_ALLONES),
                       fbAndStip(GXcopy, 0, FB_ALLONES),
                       fbXorStip(GXcopy, 0, FB_ALLONES), bitplane);
            fbBltOne(tmp,
                     tmpStride,
                     0,
                     dst + (pbox->y1 + dstYoff) * dstStride,
                     dstStride,
                     (pbox->x1 + dstXoff) * dstBpp,
                     dstBpp,
                     width * dstBpp,
                     height,
                     pPriv->and, pPriv->xor, pPriv->bgand, pPriv->bgxor);
            free(tmp);

            fbFinishAccess(pDstDrawable);
            fbFinishAccess(pSrcDrawable);
        }
        pbox++;
    }
}

RegionPtr
fbCopyArea(DrawablePtr pSrcDrawable,
           DrawablePtr pDstDrawable,
           GCPtr pGC,
           int xIn, int yIn, int widthSrc, int heightSrc, int xOut, int yOut)
{
    return miDoCopy(pSrcDrawable, pDstDrawable, pGC, xIn, yIn,
                    widthSrc, heightSrc, xOut, yOut, fbCopyNtoN, 0, 0);
}

RegionPtr
fbCopyPlane(DrawablePtr pSrcDrawable,
            DrawablePtr pDstDrawable,
            GCPtr pGC,
            int xIn,
            int yIn,
            int widthSrc,
            int heightSrc, int xOut, int yOut, unsigned long bitplane)
{
    if (pSrcDrawable->bitsPerPixel > 1)
        return miDoCopy(pSrcDrawable, pDstDrawable, pGC,
                        xIn, yIn, widthSrc, heightSrc,
                        xOut, yOut, fbCopyNto1, (Pixel) bitplane, 0);
    else if (bitplane & 1)
        return miDoCopy(pSrcDrawable, pDstDrawable, pGC, xIn, yIn,
                        widthSrc, heightSrc, xOut, yOut, fbCopy1toN,
                        (Pixel) bitplane, 0);
    else
        return miHandleExposures(pSrcDrawable, pDstDrawable, pGC,
                                 xIn, yIn,
                                 widthSrc, heightSrc, xOut, yOut);
}
