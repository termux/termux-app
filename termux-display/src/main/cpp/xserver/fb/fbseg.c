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
#include "miline.h"

#define fbBresShiftMask(mask,dir,bpp) ((bpp == FB_STIP_UNIT) ? 0 : \
					((dir < 0) ? FbStipLeft(mask,bpp) : \
					 FbStipRight(mask,bpp)))

static void
fbBresSolid(DrawablePtr pDrawable,
            GCPtr pGC,
            int dashOffset,
            int signdx,
            int signdy,
            int axis, int x1, int y1, int e, int e1, int e3, int len)
{
    FbStip *dst;
    FbStride dstStride;
    int dstBpp;
    int dstXoff, dstYoff;
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);
    FbStip and = (FbStip) pPriv->and;
    FbStip xor = (FbStip) pPriv->xor;
    FbStip mask, mask0;
    FbStip bits;

    fbGetStipDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);
    dst += ((y1 + dstYoff) * dstStride);
    x1 = (x1 + dstXoff) * dstBpp;
    dst += x1 >> FB_STIP_SHIFT;
    x1 &= FB_STIP_MASK;
    mask0 = FbStipMask(0, dstBpp);
    mask = FbStipRight(mask0, x1);
    if (signdx < 0)
        mask0 = FbStipRight(mask0, FB_STIP_UNIT - dstBpp);
    if (signdy < 0)
        dstStride = -dstStride;
    if (axis == X_AXIS) {
        bits = 0;
        while (len--) {
            bits |= mask;
            mask = fbBresShiftMask(mask, signdx, dstBpp);
            if (!mask) {
                WRITE(dst, FbDoMaskRRop(READ(dst), and, xor, bits));
                bits = 0;
                dst += signdx;
                mask = mask0;
            }
            e += e1;
            if (e >= 0) {
                if (bits) {
                    WRITE(dst, FbDoMaskRRop (READ(dst), and, xor, bits));
                    bits = 0;
                }
                dst += dstStride;
                e += e3;
            }
        }
        if (bits)
            WRITE(dst, FbDoMaskRRop(READ(dst), and, xor, bits));
    }
    else {
        while (len--) {
            WRITE(dst, FbDoMaskRRop(READ(dst), and, xor, mask));
            dst += dstStride;
            e += e1;
            if (e >= 0) {
                e += e3;
                mask = fbBresShiftMask(mask, signdx, dstBpp);
                if (!mask) {
                    dst += signdx;
                    mask = mask0;
                }
            }
        }
    }

    fbFinishAccess(pDrawable);
}

static void
fbBresDash(DrawablePtr pDrawable,
           GCPtr pGC,
           int dashOffset,
           int signdx,
           int signdy, int axis, int x1, int y1, int e, int e1, int e3, int len)
{
    FbStip *dst;
    FbStride dstStride;
    int dstBpp;
    int dstXoff, dstYoff;
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);
    FbStip and = (FbStip) pPriv->and;
    FbStip xor = (FbStip) pPriv->xor;
    FbStip bgand = (FbStip) pPriv->bgand;
    FbStip bgxor = (FbStip) pPriv->bgxor;
    FbStip mask, mask0;

    FbDashDeclare;
    int dashlen;
    Bool even;
    Bool doOdd;

    fbGetStipDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);
    doOdd = pGC->lineStyle == LineDoubleDash;

    FbDashInit(pGC, pPriv, dashOffset, dashlen, even);

    dst += ((y1 + dstYoff) * dstStride);
    x1 = (x1 + dstXoff) * dstBpp;
    dst += x1 >> FB_STIP_SHIFT;
    x1 &= FB_STIP_MASK;
    mask0 = FbStipMask(0, dstBpp);
    mask = FbStipRight(mask0, x1);
    if (signdx < 0)
        mask0 = FbStipRight(mask0, FB_STIP_UNIT - dstBpp);
    if (signdy < 0)
        dstStride = -dstStride;
    while (len--) {
        if (even)
            WRITE(dst, FbDoMaskRRop(READ(dst), and, xor, mask));
        else if (doOdd)
            WRITE(dst, FbDoMaskRRop(READ(dst), bgand, bgxor, mask));
        if (axis == X_AXIS) {
            mask = fbBresShiftMask(mask, signdx, dstBpp);
            if (!mask) {
                dst += signdx;
                mask = mask0;
            }
            e += e1;
            if (e >= 0) {
                dst += dstStride;
                e += e3;
            }
        }
        else {
            dst += dstStride;
            e += e1;
            if (e >= 0) {
                e += e3;
                mask = fbBresShiftMask(mask, signdx, dstBpp);
                if (!mask) {
                    dst += signdx;
                    mask = mask0;
                }
            }
        }
        FbDashStep(dashlen, even);
    }

    fbFinishAccess(pDrawable);
}

static void
fbBresFill(DrawablePtr pDrawable,
           GCPtr pGC,
           int dashOffset,
           int signdx,
           int signdy, int axis, int x1, int y1, int e, int e1, int e3, int len)
{
    while (len--) {
        fbFill(pDrawable, pGC, x1, y1, 1, 1);
        if (axis == X_AXIS) {
            x1 += signdx;
            e += e1;
            if (e >= 0) {
                e += e3;
                y1 += signdy;
            }
        }
        else {
            y1 += signdy;
            e += e1;
            if (e >= 0) {
                e += e3;
                x1 += signdx;
            }
        }
    }
}

static void
fbSetFg(DrawablePtr pDrawable, GCPtr pGC, Pixel fg)
{
    if (fg != pGC->fgPixel) {
        ChangeGCVal val;

        val.val = fg;
        ChangeGC(NullClient, pGC, GCForeground, &val);
        ValidateGC(pDrawable, pGC);
    }
}

static void
fbBresFillDash(DrawablePtr pDrawable,
               GCPtr pGC,
               int dashOffset,
               int signdx,
               int signdy,
               int axis, int x1, int y1, int e, int e1, int e3, int len)
{
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);

    FbDashDeclare;
    int dashlen;
    Bool even;
    Bool doOdd;
    Bool doBg;
    Pixel fg, bg;

    fg = pGC->fgPixel;
    bg = pGC->bgPixel;

    /* whether to fill the odd dashes */
    doOdd = pGC->lineStyle == LineDoubleDash;
    /* whether to switch fg to bg when filling odd dashes */
    doBg = doOdd && (pGC->fillStyle == FillSolid ||
                     pGC->fillStyle == FillStippled);

    /* compute current dash position */
    FbDashInit(pGC, pPriv, dashOffset, dashlen, even);

    while (len--) {
        if (even || doOdd) {
            if (doBg) {
                if (even)
                    fbSetFg(pDrawable, pGC, fg);
                else
                    fbSetFg(pDrawable, pGC, bg);
            }
            fbFill(pDrawable, pGC, x1, y1, 1, 1);
        }
        if (axis == X_AXIS) {
            x1 += signdx;
            e += e1;
            if (e >= 0) {
                e += e3;
                y1 += signdy;
            }
        }
        else {
            y1 += signdy;
            e += e1;
            if (e >= 0) {
                e += e3;
                x1 += signdx;
            }
        }
        FbDashStep(dashlen, even);
    }
    if (doBg)
        fbSetFg(pDrawable, pGC, fg);
}

/*
 * For drivers that want to bail drawing some lines, this
 * function takes care of selecting the appropriate rasterizer
 * based on the contents of the specified GC.
 */

static FbBres *
fbSelectBres(DrawablePtr pDrawable, GCPtr pGC)
{
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);
    int dstBpp = pDrawable->bitsPerPixel;
    FbBres *bres;

    if (pGC->lineStyle == LineSolid) {
        bres = fbBresFill;
        if (pGC->fillStyle == FillSolid) {
            bres = fbBresSolid;
            if (pPriv->and == 0) {
                switch (dstBpp) {
                case 8:
                    bres = fbBresSolid8;
                    break;
                case 16:
                    bres = fbBresSolid16;
                    break;
                case 32:
                    bres = fbBresSolid32;
                    break;
                }
            }
        }
    }
    else {
        bres = fbBresFillDash;
        if (pGC->fillStyle == FillSolid) {
            bres = fbBresDash;
            if (pPriv->and == 0 &&
                (pGC->lineStyle == LineOnOffDash || pPriv->bgand == 0)) {
                switch (dstBpp) {
                case 8:
                    bres = fbBresDash8;
                    break;
                case 16:
                    bres = fbBresDash16;
                    break;
                case 32:
                    bres = fbBresDash32;
                    break;
                }
            }
        }
    }
    return bres;
}

void
fbSegment(DrawablePtr pDrawable,
          GCPtr pGC,
          int x1, int y1, int x2, int y2, Bool drawLast, int *dashOffset)
{
    FbBres *bres;
    RegionPtr pClip = fbGetCompositeClip(pGC);
    BoxPtr pBox;
    int nBox;
    int adx;                    /* abs values of dx and dy */
    int ady;
    int signdx;                 /* sign of dx and dy */
    int signdy;
    int e, e1, e2, e3;          /* bresenham error and increments */
    int len;                    /* length of segment */
    int axis;                   /* major axis */
    int octant;
    int dashoff;
    int doff;
    unsigned int bias = miGetZeroLineBias(pDrawable->pScreen);
    unsigned int oc1;           /* outcode of point 1 */
    unsigned int oc2;           /* outcode of point 2 */

    nBox = RegionNumRects(pClip);
    pBox = RegionRects(pClip);

    bres = fbSelectBres(pDrawable, pGC);

    CalcLineDeltas(x1, y1, x2, y2, adx, ady, signdx, signdy, 1, 1, octant);

    if (adx > ady) {
        axis = X_AXIS;
        e1 = ady << 1;
        e2 = e1 - (adx << 1);
        e = e1 - adx;
        len = adx;
    }
    else {
        axis = Y_AXIS;
        e1 = adx << 1;
        e2 = e1 - (ady << 1);
        e = e1 - ady;
        SetYMajorOctant(octant);
        len = ady;
    }

    FIXUP_ERROR(e, octant, bias);

    /*
     * Adjust error terms to compare against zero
     */
    e3 = e2 - e1;
    e = e - e1;

    /* we have bresenham parameters and two points.
       all we have to do now is clip and draw.
     */

    if (drawLast)
        len++;
    dashoff = *dashOffset;
    *dashOffset = dashoff + len;
    while (nBox--) {
        oc1 = 0;
        oc2 = 0;
        OUTCODES(oc1, x1, y1, pBox);
        OUTCODES(oc2, x2, y2, pBox);
        if ((oc1 | oc2) == 0) {
            (*bres) (pDrawable, pGC, dashoff,
                     signdx, signdy, axis, x1, y1, e, e1, e3, len);
            break;
        }
        else if (oc1 & oc2) {
            pBox++;
        }
        else {
            int new_x1 = x1, new_y1 = y1, new_x2 = x2, new_y2 = y2;
            int clip1 = 0, clip2 = 0;
            int clipdx, clipdy;
            int err;

            if (miZeroClipLine(pBox->x1, pBox->y1, pBox->x2 - 1,
                               pBox->y2 - 1,
                               &new_x1, &new_y1, &new_x2, &new_y2,
                               adx, ady, &clip1, &clip2,
                               octant, bias, oc1, oc2) == -1) {
                pBox++;
                continue;
            }

            if (axis == X_AXIS)
                len = abs(new_x2 - new_x1);
            else
                len = abs(new_y2 - new_y1);
            if (clip2 != 0 || drawLast)
                len++;
            if (len) {
                /* unwind bresenham error term to first point */
                doff = dashoff;
                err = e;
                if (clip1) {
                    clipdx = abs(new_x1 - x1);
                    clipdy = abs(new_y1 - y1);
                    if (axis == X_AXIS) {
                        doff += clipdx;
                        err += e3 * clipdy + e1 * clipdx;
                    }
                    else {
                        doff += clipdy;
                        err += e3 * clipdx + e1 * clipdy;
                    }
                }
                (*bres) (pDrawable, pGC, doff,
                         signdx, signdy, axis, new_x1, new_y1,
                         err, e1, e3, len);
            }
            pBox++;
        }
    }                           /* while (nBox--) */
}
