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

#include "fb.h"

static void
fbTile(FbBits * dst, FbStride dstStride, int dstX, int width, int height,
       FbBits * tile, FbStride tileStride, int tileWidth, int tileHeight,
       int alu, FbBits pm, int bpp, int xRot, int yRot)
{
    int tileX, tileY;
    int widthTmp;
    int h, w;
    int x, y;

    modulus(-yRot, tileHeight, tileY);
    y = 0;
    while (height) {
        h = tileHeight - tileY;
        if (h > height)
            h = height;
        height -= h;
        widthTmp = width;
        x = dstX;
        modulus(dstX - xRot, tileWidth, tileX);
        while (widthTmp) {
            w = tileWidth - tileX;
            if (w > widthTmp)
                w = widthTmp;
            widthTmp -= w;
            fbBlt(tile + tileY * tileStride,
                  tileStride,
                  tileX,
                  dst + y * dstStride,
                  dstStride, x, w, h, alu, pm, bpp, FALSE, FALSE);
            x += w;
            tileX = 0;
        }
        y += h;
        tileY = 0;
    }
}

static void
fbStipple(FbBits * dst, FbStride dstStride,
          int dstX, int dstBpp,
          int width, int height,
          FbStip * stip, FbStride stipStride,
          int stipWidth, int stipHeight,
          FbBits fgand, FbBits fgxor,
          FbBits bgand, FbBits bgxor,
          int xRot, int yRot)
{
    int stipX, stipY, sx;
    int widthTmp;
    int h, w;
    int x, y;

    modulus(-yRot, stipHeight, stipY);
    modulus(dstX / dstBpp - xRot, stipWidth, stipX);
    y = 0;
    while (height) {
        h = stipHeight - stipY;
        if (h > height)
            h = height;
        height -= h;
        widthTmp = width;
        x = dstX;
        sx = stipX;
        while (widthTmp) {
            w = (stipWidth - sx) * dstBpp;
            if (w > widthTmp)
                w = widthTmp;
            widthTmp -= w;
            fbBltOne(stip + stipY * stipStride,
                     stipStride,
                     sx,
                     dst + y * dstStride,
                     dstStride, x, dstBpp, w, h, fgand, fgxor, bgand, bgxor);
            x += w;
            sx = 0;
        }
        y += h;
        stipY = 0;
    }
}

void
fbFill(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int width, int height)
{
    FbBits *dst;
    FbStride dstStride;
    int dstBpp;
    int dstXoff, dstYoff;
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);

    fbGetDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);

    switch (pGC->fillStyle) {
    case FillSolid:
#ifndef FB_ACCESS_WRAPPER
        if (pPriv->and || !pixman_fill((uint32_t *) dst, dstStride, dstBpp,
                                       x + dstXoff, y + dstYoff,
                                       width, height, pPriv->xor))
#endif
            fbSolid(dst + (y + dstYoff) * dstStride,
                    dstStride,
                    (x + dstXoff) * dstBpp,
                    dstBpp, width * dstBpp, height, pPriv->and, pPriv->xor);
        break;
    case FillStippled:
    case FillOpaqueStippled:{
        PixmapPtr pStip = pGC->stipple;
        int stipWidth = pStip->drawable.width;
        int stipHeight = pStip->drawable.height;

        if (dstBpp == 1) {
            int alu;
            FbBits *stip;
            FbStride stipStride;
            int stipBpp;
            _X_UNUSED int stipXoff, stipYoff;

            if (pGC->fillStyle == FillStippled)
                alu = FbStipple1Rop(pGC->alu, pGC->fgPixel);
            else
                alu = FbOpaqueStipple1Rop(pGC->alu, pGC->fgPixel, pGC->bgPixel);
            fbGetDrawable(&pStip->drawable, stip, stipStride, stipBpp, stipXoff,
                          stipYoff);
            fbTile(dst + (y + dstYoff) * dstStride, dstStride, x + dstXoff,
                   width, height, stip, stipStride, stipWidth, stipHeight, alu,
                   pPriv->pm, dstBpp, (pGC->patOrg.x + pDrawable->x + dstXoff),
                   pGC->patOrg.y + pDrawable->y - y);
            fbFinishAccess(&pStip->drawable);
        }
        else {
            FbStip *stip;
            FbStride stipStride;
            int stipBpp;
            _X_UNUSED int stipXoff, stipYoff;
            FbBits fgand, fgxor, bgand, bgxor;

            fgand = pPriv->and;
            fgxor = pPriv->xor;
            if (pGC->fillStyle == FillStippled) {
                bgand = fbAnd(GXnoop, (FbBits) 0, FB_ALLONES);
                bgxor = fbXor(GXnoop, (FbBits) 0, FB_ALLONES);
            }
            else {
                bgand = pPriv->bgand;
                bgxor = pPriv->bgxor;
            }

            fbGetStipDrawable(&pStip->drawable, stip, stipStride, stipBpp,
                              stipXoff, stipYoff);
            fbStipple(dst + (y + dstYoff) * dstStride, dstStride,
                      (x + dstXoff) * dstBpp, dstBpp, width * dstBpp, height,
                      stip, stipStride, stipWidth, stipHeight,
                      fgand, fgxor, bgand, bgxor,
                      pGC->patOrg.x + pDrawable->x + dstXoff,
                      pGC->patOrg.y + pDrawable->y - y);
            fbFinishAccess(&pStip->drawable);
        }
        break;
    }
    case FillTiled:{
        PixmapPtr pTile = pGC->tile.pixmap;
        FbBits *tile;
        FbStride tileStride;
        int tileBpp;
        int tileWidth;
        int tileHeight;
        _X_UNUSED int tileXoff, tileYoff;

        fbGetDrawable(&pTile->drawable, tile, tileStride, tileBpp, tileXoff,
                      tileYoff);
        tileWidth = pTile->drawable.width;
        tileHeight = pTile->drawable.height;
        fbTile(dst + (y + dstYoff) * dstStride,
               dstStride,
               (x + dstXoff) * dstBpp,
               width * dstBpp, height,
               tile,
               tileStride,
               tileWidth * tileBpp,
               tileHeight,
               pGC->alu,
               pPriv->pm,
               dstBpp,
               (pGC->patOrg.x + pDrawable->x + dstXoff) * dstBpp,
               pGC->patOrg.y + pDrawable->y - y);
        fbFinishAccess(&pTile->drawable);
        break;
    }
    }
    fbValidateDrawable(pDrawable);
    fbFinishAccess(pDrawable);
}

void
fbSolidBoxClipped(DrawablePtr pDrawable,
                  RegionPtr pClip,
                  int x1, int y1, int x2, int y2, FbBits and, FbBits xor)
{
    FbBits *dst;
    FbStride dstStride;
    int dstBpp;
    int dstXoff, dstYoff;
    BoxPtr pbox;
    int nbox;
    int partX1, partX2, partY1, partY2;

    fbGetDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);

    for (nbox = RegionNumRects(pClip), pbox = RegionRects(pClip);
         nbox--; pbox++) {
        partX1 = pbox->x1;
        if (partX1 < x1)
            partX1 = x1;

        partX2 = pbox->x2;
        if (partX2 > x2)
            partX2 = x2;

        if (partX2 <= partX1)
            continue;

        partY1 = pbox->y1;
        if (partY1 < y1)
            partY1 = y1;

        partY2 = pbox->y2;
        if (partY2 > y2)
            partY2 = y2;

        if (partY2 <= partY1)
            continue;

#ifndef FB_ACCESS_WRAPPER
        if (and || !pixman_fill((uint32_t *) dst, dstStride, dstBpp,
                                partX1 + dstXoff, partY1 + dstYoff,
                                (partX2 - partX1), (partY2 - partY1), xor))
#endif
            fbSolid(dst + (partY1 + dstYoff) * dstStride,
                    dstStride,
                    (partX1 + dstXoff) * dstBpp,
                    dstBpp,
                    (partX2 - partX1) * dstBpp, (partY2 - partY1), and, xor);
    }
    fbFinishAccess(pDrawable);
}
