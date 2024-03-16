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

#include <string.h>

#include "fb.h"

void
fbPutImage(DrawablePtr pDrawable,
           GCPtr pGC,
           int depth,
           int x, int y, int w, int h, int leftPad, int format, char *pImage)
{
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);
    unsigned long i;
    FbStride srcStride;
    FbStip *src = (FbStip *) pImage;

    x += pDrawable->x;
    y += pDrawable->y;

    switch (format) {
    case XYBitmap:
        srcStride = BitmapBytePad(w + leftPad) / sizeof(FbStip);
        fbPutXYImage(pDrawable,
                     fbGetCompositeClip(pGC),
                     pPriv->fg,
                     pPriv->bg,
                     pPriv->pm,
                     pGC->alu, TRUE, x, y, w, h, src, srcStride, leftPad);
        break;
    case XYPixmap:
        srcStride = BitmapBytePad(w + leftPad) / sizeof(FbStip);
        for (i = (unsigned long) 1 << (pDrawable->depth - 1); i; i >>= 1) {
            if (i & pGC->planemask) {
                fbPutXYImage(pDrawable,
                             fbGetCompositeClip(pGC),
                             FB_ALLONES,
                             0,
                             fbReplicatePixel(i, pDrawable->bitsPerPixel),
                             pGC->alu,
                             TRUE, x, y, w, h, src, srcStride, leftPad);
            }
            src += srcStride * h;
        }
        break;
    case ZPixmap:
        srcStride = PixmapBytePad(w, pDrawable->depth) / sizeof(FbStip);
        fbPutZImage(pDrawable, fbGetCompositeClip(pGC),
                    pGC->alu, pPriv->pm, x, y, w, h, src, srcStride);
    }
}

void
fbPutZImage(DrawablePtr pDrawable,
            RegionPtr pClip,
            int alu,
            FbBits pm,
            int x,
            int y, int width, int height, FbStip * src, FbStride srcStride)
{
    FbStip *dst;
    FbStride dstStride;
    int dstBpp;
    int dstXoff, dstYoff;
    int nbox;
    BoxPtr pbox;
    int x1, y1, x2, y2;

    fbGetStipDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);

    for (nbox = RegionNumRects(pClip),
         pbox = RegionRects(pClip); nbox--; pbox++) {
        x1 = x;
        y1 = y;
        x2 = x + width;
        y2 = y + height;
        if (x1 < pbox->x1)
            x1 = pbox->x1;
        if (y1 < pbox->y1)
            y1 = pbox->y1;
        if (x2 > pbox->x2)
            x2 = pbox->x2;
        if (y2 > pbox->y2)
            y2 = pbox->y2;
        if (x1 >= x2 || y1 >= y2)
            continue;
        fbBltStip(src + (y1 - y) * srcStride,
                  srcStride,
                  (x1 - x) * dstBpp,
                  dst + (y1 + dstYoff) * dstStride,
                  dstStride,
                  (x1 + dstXoff) * dstBpp,
                  (x2 - x1) * dstBpp, (y2 - y1), alu, pm, dstBpp);
    }

    fbFinishAccess(pDrawable);
}

void
fbPutXYImage(DrawablePtr pDrawable,
             RegionPtr pClip,
             FbBits fg,
             FbBits bg,
             FbBits pm,
             int alu,
             Bool opaque,
             int x,
             int y,
             int width, int height, FbStip * src, FbStride srcStride, int srcX)
{
    FbBits *dst;
    FbStride dstStride;
    int dstBpp;
    int dstXoff, dstYoff;
    int nbox;
    BoxPtr pbox;
    int x1, y1, x2, y2;
    FbBits fgand = 0, fgxor = 0, bgand = 0, bgxor = 0;

    fbGetDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);

    if (dstBpp == 1) {
        if (opaque)
            alu = FbOpaqueStipple1Rop(alu, fg, bg);
        else
            alu = FbStipple1Rop(alu, fg);
    }
    else {
        fgand = fbAnd(alu, fg, pm);
        fgxor = fbXor(alu, fg, pm);
        if (opaque) {
            bgand = fbAnd(alu, bg, pm);
            bgxor = fbXor(alu, bg, pm);
        }
        else {
            bgand = fbAnd(GXnoop, (FbBits) 0, FB_ALLONES);
            bgxor = fbXor(GXnoop, (FbBits) 0, FB_ALLONES);
        }
    }

    for (nbox = RegionNumRects(pClip),
         pbox = RegionRects(pClip); nbox--; pbox++) {
        x1 = x;
        y1 = y;
        x2 = x + width;
        y2 = y + height;
        if (x1 < pbox->x1)
            x1 = pbox->x1;
        if (y1 < pbox->y1)
            y1 = pbox->y1;
        if (x2 > pbox->x2)
            x2 = pbox->x2;
        if (y2 > pbox->y2)
            y2 = pbox->y2;
        if (x1 >= x2 || y1 >= y2)
            continue;
        if (dstBpp == 1) {
            fbBltStip(src + (y1 - y) * srcStride,
                      srcStride,
                      (x1 - x) + srcX,
                      (FbStip *) (dst + (y1 + dstYoff) * dstStride),
                      FbBitsStrideToStipStride(dstStride),
                      (x1 + dstXoff) * dstBpp,
                      (x2 - x1) * dstBpp, (y2 - y1), alu, pm, dstBpp);
        }
        else {
            fbBltOne(src + (y1 - y) * srcStride,
                     srcStride,
                     (x1 - x) + srcX,
                     dst + (y1 + dstYoff) * dstStride,
                     dstStride,
                     (x1 + dstXoff) * dstBpp,
                     dstBpp,
                     (x2 - x1) * dstBpp, (y2 - y1), fgand, fgxor, bgand, bgxor);
        }
    }

    fbFinishAccess(pDrawable);
}

void
fbGetImage(DrawablePtr pDrawable,
           int x,
           int y,
           int w, int h, unsigned int format, unsigned long planeMask, char *d)
{
    FbBits *src;
    FbStride srcStride;
    int srcBpp;
    int srcXoff, srcYoff;
    FbStip *dst;
    FbStride dstStride;

    /*
     * XFree86 DDX empties the root borderClip when the VT is
     * switched away; this checks for that case
     */
    if (!fbDrawableEnabled(pDrawable))
        return;

    fbGetDrawable(pDrawable, src, srcStride, srcBpp, srcXoff, srcYoff);

    x += pDrawable->x;
    y += pDrawable->y;

    dst = (FbStip *) d;
    if (format == ZPixmap || srcBpp == 1) {
        FbBits pm;

        pm = fbReplicatePixel(planeMask, srcBpp);
        dstStride = PixmapBytePad(w, pDrawable->depth);
        dstStride /= sizeof(FbStip);
        fbBltStip((FbStip *) (src + (y + srcYoff) * srcStride),
                  FbBitsStrideToStipStride(srcStride),
                  (x + srcXoff) * srcBpp,
                  dst, dstStride, 0, w * srcBpp, h, GXcopy, FB_ALLONES, srcBpp);

        if (pm != FB_ALLONES) {
            for (int i = 0; i < dstStride * h; i++)
                dst[i] &= pm;
        }
    }
    else {
        dstStride = BitmapBytePad(w) / sizeof(FbStip);
        fbBltPlane(src + (y + srcYoff) * srcStride,
                   srcStride,
                   (x + srcXoff) * srcBpp,
                   srcBpp,
                   dst,
                   dstStride,
                   0,
                   w * srcBpp, h,
                   fbAndStip(GXcopy, FB_STIP_ALLONES, FB_STIP_ALLONES),
                   fbXorStip(GXcopy, FB_STIP_ALLONES, FB_STIP_ALLONES),
                   fbAndStip(GXcopy, 0, FB_STIP_ALLONES),
                   fbXorStip(GXcopy, 0, FB_STIP_ALLONES), planeMask);
    }

    fbFinishAccess(pDrawable);
}
