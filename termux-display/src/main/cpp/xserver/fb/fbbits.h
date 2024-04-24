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

/*
 * This file defines functions for drawing some primitives using
 * underlying datatypes instead of masks
 */

#define isClipped(c,ul,lr)  (((c) | ((c) - (ul)) | ((lr) - (c))) & 0x80008000)

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef BITSSTORE
#define STORE(b,x)  BITSSTORE(b,x)
#else
#define STORE(b,x)  WRITE((b), (x))
#endif

#ifdef BITSRROP
#define RROP(b,a,x)	BITSRROP(b,a,x)
#else
#define RROP(b,a,x)	WRITE((b), FbDoRRop (READ(b), (a), (x)))
#endif

#ifdef BITSUNIT
#define UNIT BITSUNIT
#define USE_SOLID
#else
#define UNIT BITS
#endif

/*
 * Define the following before including this file:
 *
 *  BRESSOLID	name of function for drawing a solid segment
 *  BRESDASH	name of function for drawing a dashed segment
 *  DOTS	name of function for drawing dots
 *  ARC		name of function for drawing a solid arc
 *  BITS	type of underlying unit
 */

#ifdef BRESSOLID
void
BRESSOLID(DrawablePtr pDrawable,
          GCPtr pGC,
          int dashOffset,
          int signdx,
          int signdy, int axis, int x1, int y1, int e, int e1, int e3, int len)
{
    FbBits *dst;
    FbStride dstStride;
    int dstBpp;
    int dstXoff, dstYoff;
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);
    UNIT *bits;
    FbStride bitsStride;
    FbStride majorStep, minorStep;
    BITS xor = (BITS) pPriv->xor;

    fbGetDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);
    bits =
        ((UNIT *) (dst + ((y1 + dstYoff) * dstStride))) + (x1 + dstXoff);
    bitsStride = dstStride * (sizeof(FbBits) / sizeof(UNIT));
    if (signdy < 0)
        bitsStride = -bitsStride;
    if (axis == X_AXIS) {
        majorStep = signdx;
        minorStep = bitsStride;
    }
    else {
        majorStep = bitsStride;
        minorStep = signdx;
    }
    while (len--) {
        STORE(bits, xor);
        bits += majorStep;
        e += e1;
        if (e >= 0) {
            bits += minorStep;
            e += e3;
        }
    }

    fbFinishAccess(pDrawable);
}
#endif

#ifdef BRESDASH
void
BRESDASH(DrawablePtr pDrawable,
         GCPtr pGC,
         int dashOffset,
         int signdx,
         int signdy, int axis, int x1, int y1, int e, int e1, int e3, int len)
{
    FbBits *dst;
    FbStride dstStride;
    int dstBpp;
    int dstXoff, dstYoff;
    FbGCPrivPtr pPriv = fbGetGCPrivate(pGC);
    UNIT *bits;
    FbStride bitsStride;
    FbStride majorStep, minorStep;
    BITS xorfg, xorbg;

    FbDashDeclare;
    int dashlen;
    Bool even;
    Bool doOdd;

    fbGetDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);
    doOdd = pGC->lineStyle == LineDoubleDash;
    xorfg = (BITS) pPriv->xor;
    xorbg = (BITS) pPriv->bgxor;

    FbDashInit(pGC, pPriv, dashOffset, dashlen, even);

    bits =
        ((UNIT *) (dst + ((y1 + dstYoff) * dstStride))) + (x1 + dstXoff);
    bitsStride = dstStride * (sizeof(FbBits) / sizeof(UNIT));
    if (signdy < 0)
        bitsStride = -bitsStride;
    if (axis == X_AXIS) {
        majorStep = signdx;
        minorStep = bitsStride;
    }
    else {
        majorStep = bitsStride;
        minorStep = signdx;
    }
    if (dashlen >= len)
        dashlen = len;
    if (doOdd) {
        if (!even)
            goto doubleOdd;
        for (;;) {
            len -= dashlen;
            while (dashlen--) {
                STORE(bits, xorfg);
                bits += majorStep;
                if ((e += e1) >= 0) {
                    e += e3;
                    bits += minorStep;
                }
            }
            if (!len)
                break;

            FbDashNextEven(dashlen);

            if (dashlen >= len)
                dashlen = len;
 doubleOdd:
            len -= dashlen;
            while (dashlen--) {
                STORE(bits, xorbg);
                bits += majorStep;
                if ((e += e1) >= 0) {
                    e += e3;
                    bits += minorStep;
                }
            }
            if (!len)
                break;

            FbDashNextOdd(dashlen);

            if (dashlen >= len)
                dashlen = len;
        }
    }
    else {
        if (!even)
            goto onOffOdd;
        for (;;) {
            len -= dashlen;
            while (dashlen--) {
                STORE(bits, xorfg);
                bits += majorStep;
                if ((e += e1) >= 0) {
                    e += e3;
                    bits += minorStep;
                }
            }
            if (!len)
                break;

            FbDashNextEven(dashlen);

            if (dashlen >= len)
                dashlen = len;
 onOffOdd:
            len -= dashlen;
            while (dashlen--) {
                bits += majorStep;
                if ((e += e1) >= 0) {
                    e += e3;
                    bits += minorStep;
                }
            }
            if (!len)
                break;

            FbDashNextOdd(dashlen);

            if (dashlen >= len)
                dashlen = len;
        }
    }

    fbFinishAccess(pDrawable);
}
#endif

#ifdef DOTS
void
DOTS(FbBits * dst,
     FbStride dstStride,
     int dstBpp,
     BoxPtr pBox,
     xPoint * ptsOrig,
     int npt, int xorg, int yorg, int xoff, int yoff, FbBits and, FbBits xor)
{
    INT32 *pts = (INT32 *) ptsOrig;
    UNIT *bits = (UNIT *) dst;
    UNIT *point;
    BITS bxor = (BITS) xor;
    BITS band = (BITS) and;
    FbStride bitsStride = dstStride * (sizeof(FbBits) / sizeof(UNIT));
    INT32 ul, lr;
    INT32 pt;

    ul = coordToInt(pBox->x1 - xorg, pBox->y1 - yorg);
    lr = coordToInt(pBox->x2 - xorg - 1, pBox->y2 - yorg - 1);

    bits += bitsStride * (yorg + yoff) + (xorg + xoff);

    if (and == 0) {
        while (npt--) {
            pt = *pts++;
            if (!isClipped(pt, ul, lr)) {
                point = bits + intToY(pt) * bitsStride + intToX(pt);
                STORE(point, bxor);
            }
        }
    }
    else {
        while (npt--) {
            pt = *pts++;
            if (!isClipped(pt, ul, lr)) {
                point = bits + intToY(pt) * bitsStride + intToX(pt);
                RROP(point, band, bxor);
            }
        }
    }
}
#endif

#ifdef ARC

#define ARCCOPY(d)  STORE(d,xorBits)
#define ARCRROP(d)  RROP(d,andBits,xorBits)

void
ARC(FbBits * dst,
    FbStride dstStride,
    int dstBpp, xArc * arc, int drawX, int drawY, FbBits and, FbBits xor)
{
    UNIT *bits;
    FbStride bitsStride;
    miZeroArcRec info;
    Bool do360;
    int x;
    UNIT *yorgp, *yorgop;
    BITS andBits, xorBits;
    int yoffset, dyoffset;
    int y, a, b, d, mask;
    int k1, k3, dx, dy;

    bits = (UNIT *) dst;
    bitsStride = dstStride * (sizeof(FbBits) / sizeof(UNIT));
    andBits = (BITS) and;
    xorBits = (BITS) xor;
    do360 = miZeroArcSetup(arc, &info, TRUE);
    yorgp = bits + ((info.yorg + drawY) * bitsStride);
    yorgop = bits + ((info.yorgo + drawY) * bitsStride);
    info.xorg = (info.xorg + drawX);
    info.xorgo = (info.xorgo + drawX);
    MIARCSETUP();
    yoffset = y ? bitsStride : 0;
    dyoffset = 0;
    mask = info.initialMask;

    if (!(arc->width & 1)) {
        if (andBits == 0) {
            if (mask & 2)
                ARCCOPY(yorgp + info.xorgo);
            if (mask & 8)
                ARCCOPY(yorgop + info.xorgo);
        }
        else {
            if (mask & 2)
                ARCRROP(yorgp + info.xorgo);
            if (mask & 8)
                ARCRROP(yorgop + info.xorgo);
        }
    }
    if (!info.end.x || !info.end.y) {
        mask = info.end.mask;
        info.end = info.altend;
    }
    if (do360 && (arc->width == arc->height) && !(arc->width & 1)) {
        int xoffset = bitsStride;
        UNIT *yorghb = yorgp + (info.h * bitsStride) + info.xorg;
        UNIT *yorgohb = yorghb - info.h;

        yorgp += info.xorg;
        yorgop += info.xorg;
        yorghb += info.h;
        while (1) {
            if (andBits == 0) {
                ARCCOPY(yorgp + yoffset + x);
                ARCCOPY(yorgp + yoffset - x);
                ARCCOPY(yorgop - yoffset - x);
                ARCCOPY(yorgop - yoffset + x);
            }
            else {
                ARCRROP(yorgp + yoffset + x);
                ARCRROP(yorgp + yoffset - x);
                ARCRROP(yorgop - yoffset - x);
                ARCRROP(yorgop - yoffset + x);
            }
            if (a < 0)
                break;
            if (andBits == 0) {
                ARCCOPY(yorghb - xoffset - y);
                ARCCOPY(yorgohb - xoffset + y);
                ARCCOPY(yorgohb + xoffset + y);
                ARCCOPY(yorghb + xoffset - y);
            }
            else {
                ARCRROP(yorghb - xoffset - y);
                ARCRROP(yorgohb - xoffset + y);
                ARCRROP(yorgohb + xoffset + y);
                ARCRROP(yorghb + xoffset - y);
            }
            xoffset += bitsStride;
            MIARCCIRCLESTEP(yoffset += bitsStride;
                );
        }
        yorgp -= info.xorg;
        yorgop -= info.xorg;
        x = info.w;
        yoffset = info.h * bitsStride;
    }
    else if (do360) {
        while (y < info.h || x < info.w) {
            MIARCOCTANTSHIFT(dyoffset = bitsStride;
                );
            if (andBits == 0) {
                ARCCOPY(yorgp + yoffset + info.xorg + x);
                ARCCOPY(yorgp + yoffset + info.xorgo - x);
                ARCCOPY(yorgop - yoffset + info.xorgo - x);
                ARCCOPY(yorgop - yoffset + info.xorg + x);
            }
            else {
                ARCRROP(yorgp + yoffset + info.xorg + x);
                ARCRROP(yorgp + yoffset + info.xorgo - x);
                ARCRROP(yorgop - yoffset + info.xorgo - x);
                ARCRROP(yorgop - yoffset + info.xorg + x);
            }
            MIARCSTEP(yoffset += dyoffset;
                      , yoffset += bitsStride;
                );
        }
    }
    else {
        while (y < info.h || x < info.w) {
            MIARCOCTANTSHIFT(dyoffset = bitsStride;
                );
            if ((x == info.start.x) || (y == info.start.y)) {
                mask = info.start.mask;
                info.start = info.altstart;
            }
            if (andBits == 0) {
                if (mask & 1)
                    ARCCOPY(yorgp + yoffset + info.xorg + x);
                if (mask & 2)
                    ARCCOPY(yorgp + yoffset + info.xorgo - x);
                if (mask & 4)
                    ARCCOPY(yorgop - yoffset + info.xorgo - x);
                if (mask & 8)
                    ARCCOPY(yorgop - yoffset + info.xorg + x);
            }
            else {
                if (mask & 1)
                    ARCRROP(yorgp + yoffset + info.xorg + x);
                if (mask & 2)
                    ARCRROP(yorgp + yoffset + info.xorgo - x);
                if (mask & 4)
                    ARCRROP(yorgop - yoffset + info.xorgo - x);
                if (mask & 8)
                    ARCRROP(yorgop - yoffset + info.xorg + x);
            }
            if ((x == info.end.x) || (y == info.end.y)) {
                mask = info.end.mask;
                info.end = info.altend;
            }
            MIARCSTEP(yoffset += dyoffset;
                      , yoffset += bitsStride;
                );
        }
    }
    if ((x == info.start.x) || (y == info.start.y))
        mask = info.start.mask;
    if (andBits == 0) {
        if (mask & 1)
            ARCCOPY(yorgp + yoffset + info.xorg + x);
        if (mask & 4)
            ARCCOPY(yorgop - yoffset + info.xorgo - x);
        if (arc->height & 1) {
            if (mask & 2)
                ARCCOPY(yorgp + yoffset + info.xorgo - x);
            if (mask & 8)
                ARCCOPY(yorgop - yoffset + info.xorg + x);
        }
    }
    else {
        if (mask & 1)
            ARCRROP(yorgp + yoffset + info.xorg + x);
        if (mask & 4)
            ARCRROP(yorgop - yoffset + info.xorgo - x);
        if (arc->height & 1) {
            if (mask & 2)
                ARCRROP(yorgp + yoffset + info.xorgo - x);
            if (mask & 8)
                ARCRROP(yorgop - yoffset + info.xorg + x);
        }
    }
}

#undef ARCCOPY
#undef ARCRROP
#endif

#ifdef GLYPH
#if BITMAP_BIT_ORDER == LSBFirst
#define WRITE_ADDR1(n)	    (n)
#define WRITE_ADDR2(n)	    (n)
#define WRITE_ADDR4(n)	    (n)
#else
#define WRITE_ADDR1(n)	    ((n) ^ 3)
#define WRITE_ADDR2(n)	    ((n) ^ 2)
#define WRITE_ADDR4(n)	    ((n))
#endif

#define WRITE1(d,n,fg)	    WRITE(d + WRITE_ADDR1(n), (BITS) (fg))

#ifdef BITS2
#define WRITE2(d,n,fg)	    WRITE((BITS2 *) &((d)[WRITE_ADDR2(n)]), (BITS2) (fg))
#else
#define WRITE2(d,n,fg)	    (WRITE1(d,n,fg), WRITE1(d,(n)+1,fg))
#endif

#ifdef BITS4
#define WRITE4(d,n,fg)	    WRITE((BITS4 *) &((d)[WRITE_ADDR4(n)]), (BITS4) (fg))
#else
#define WRITE4(d,n,fg)	    (WRITE2(d,n,fg), WRITE2(d,(n)+2,fg))
#endif

void
GLYPH(FbBits * dstBits,
      FbStride dstStride,
      int dstBpp, FbStip * stipple, FbBits fg, int x, int height)
{
    int lshift;
    FbStip bits;
    BITS *dstLine;
    BITS *dst;
    int n;
    int shift;

    dstLine = (BITS *) dstBits;
    dstLine += x & ~3;
    dstStride *= (sizeof(FbBits) / sizeof(BITS));
    shift = x & 3;
    lshift = 4 - shift;
    while (height--) {
        bits = *stipple++;
        dst = (BITS *) dstLine;
        n = lshift;
        while (bits) {
            switch (FbStipMoveLsb(FbLeftStipBits(bits, n), 4, n)) {
            case 0:
                break;
            case 1:
                WRITE1(dst, 0, fg);
                break;
            case 2:
                WRITE1(dst, 1, fg);
                break;
            case 3:
                WRITE2(dst, 0, fg);
                break;
            case 4:
                WRITE1(dst, 2, fg);
                break;
            case 5:
                WRITE1(dst, 0, fg);
                WRITE1(dst, 2, fg);
                break;
            case 6:
                WRITE1(dst, 1, fg);
                WRITE1(dst, 2, fg);
                break;
            case 7:
                WRITE2(dst, 0, fg);
                WRITE1(dst, 2, fg);
                break;
            case 8:
                WRITE1(dst, 3, fg);
                break;
            case 9:
                WRITE1(dst, 0, fg);
                WRITE1(dst, 3, fg);
                break;
            case 10:
                WRITE1(dst, 1, fg);
                WRITE1(dst, 3, fg);
                break;
            case 11:
                WRITE2(dst, 0, fg);
                WRITE1(dst, 3, fg);
                break;
            case 12:
                WRITE2(dst, 2, fg);
                break;
            case 13:
                WRITE1(dst, 0, fg);
                WRITE2(dst, 2, fg);
                break;
            case 14:
                WRITE1(dst, 1, fg);
                WRITE2(dst, 2, fg);
                break;
            case 15:
                WRITE4(dst, 0, fg);
                break;
            }
            bits = FbStipLeft(bits, n);
            n = 4;
            dst += 4;
        }
        dstLine += dstStride;
    }
}

#undef WRITE_ADDR1
#undef WRITE_ADDR2
#undef WRITE_ADDR4
#undef WRITE1
#undef WRITE2
#undef WRITE4

#endif

#ifdef POLYLINE
void
POLYLINE(DrawablePtr pDrawable,
         GCPtr pGC, int mode, int npt, DDXPointPtr ptsOrig)
{
    INT32 *pts = (INT32 *) ptsOrig;
    int xoff = pDrawable->x;
    int yoff = pDrawable->y;
    unsigned int bias = miGetZeroLineBias(pDrawable->pScreen);
    BoxPtr pBox = RegionExtents(fbGetCompositeClip(pGC));

    FbBits *dst;
    int dstStride;
    int dstBpp;
    int dstXoff, dstYoff;

    UNIT *bits, *bitsBase;
    FbStride bitsStride;
    BITS xor = fbGetGCPrivate(pGC)->xor;
    BITS and = fbGetGCPrivate(pGC)->and;
    int dashoffset = 0;

    INT32 ul, lr;
    INT32 pt1, pt2;

    int e, e1, e3, len;
    int stepmajor, stepminor;
    int octant;

    if (mode == CoordModePrevious)
        fbFixCoordModePrevious(npt, ptsOrig);

    fbGetDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);
    bitsStride = dstStride * (sizeof(FbBits) / sizeof(UNIT));
    bitsBase =
        ((UNIT *) dst) + (yoff + dstYoff) * bitsStride + (xoff + dstXoff);
    ul = coordToInt(pBox->x1 - xoff, pBox->y1 - yoff);
    lr = coordToInt(pBox->x2 - xoff - 1, pBox->y2 - yoff - 1);

    pt1 = *pts++;
    npt--;
    pt2 = *pts++;
    npt--;
    for (;;) {
        if (isClipped(pt1, ul, lr) | isClipped(pt2, ul, lr)) {
            fbSegment(pDrawable, pGC,
                      intToX(pt1) + xoff, intToY(pt1) + yoff,
                      intToX(pt2) + xoff, intToY(pt2) + yoff,
                      npt == 0 && pGC->capStyle != CapNotLast, &dashoffset);
            if (!npt) {
                fbFinishAccess(pDrawable);
                return;
            }
            pt1 = pt2;
            pt2 = *pts++;
            npt--;
        }
        else {
            bits = bitsBase + intToY(pt1) * bitsStride + intToX(pt1);
            for (;;) {
                CalcLineDeltas(intToX(pt1), intToY(pt1),
                               intToX(pt2), intToY(pt2),
                               len, e1, stepmajor, stepminor, 1, bitsStride,
                               octant);
                if (len < e1) {
                    e3 = len;
                    len = e1;
                    e1 = e3;

                    e3 = stepminor;
                    stepminor = stepmajor;
                    stepmajor = e3;
                    SetYMajorOctant(octant);
                }
                e = -len;
                e1 <<= 1;
                e3 = e << 1;
                FIXUP_ERROR(e, octant, bias);
                if (and == 0) {
                    while (len--) {
                        STORE(bits, xor);
                        bits += stepmajor;
                        e += e1;
                        if (e >= 0) {
                            bits += stepminor;
                            e += e3;
                        }
                    }
                }
                else {
                    while (len--) {
                        RROP(bits, and, xor);
                        bits += stepmajor;
                        e += e1;
                        if (e >= 0) {
                            bits += stepminor;
                            e += e3;
                        }
                    }
                }
                if (!npt) {
                    if (pGC->capStyle != CapNotLast &&
                        pt2 != *((INT32 *) ptsOrig)) {
                        RROP(bits, and, xor);
                    }
                    fbFinishAccess(pDrawable);
                    return;
                }
                pt1 = pt2;
                pt2 = *pts++;
                --npt;
                if (isClipped(pt2, ul, lr))
                    break;
            }
        }
    }

    fbFinishAccess(pDrawable);
}
#endif

#ifdef POLYSEGMENT
void
POLYSEGMENT(DrawablePtr pDrawable, GCPtr pGC, int nseg, xSegment * pseg)
{
    INT32 *pts = (INT32 *) pseg;
    int xoff = pDrawable->x;
    int yoff = pDrawable->y;
    unsigned int bias = miGetZeroLineBias(pDrawable->pScreen);
    BoxPtr pBox = RegionExtents(fbGetCompositeClip(pGC));

    FbBits *dst;
    int dstStride;
    int dstBpp;
    int dstXoff, dstYoff;

    UNIT *bits, *bitsBase;
    FbStride bitsStride;
    FbBits xorBits = fbGetGCPrivate(pGC)->xor;
    FbBits andBits = fbGetGCPrivate(pGC)->and;
    BITS xor = xorBits;
    BITS and = andBits;
    int dashoffset = 0;

    INT32 ul, lr;
    INT32 pt1, pt2;

    int e, e1, e3, len;
    int stepmajor, stepminor;
    int octant;
    Bool capNotLast;

    fbGetDrawable(pDrawable, dst, dstStride, dstBpp, dstXoff, dstYoff);
    bitsStride = dstStride * (sizeof(FbBits) / sizeof(UNIT));
    bitsBase =
        ((UNIT *) dst) + (yoff + dstYoff) * bitsStride + (xoff + dstXoff);
    ul = coordToInt(pBox->x1 - xoff, pBox->y1 - yoff);
    lr = coordToInt(pBox->x2 - xoff - 1, pBox->y2 - yoff - 1);

    capNotLast = pGC->capStyle == CapNotLast;

    while (nseg--) {
        pt1 = *pts++;
        pt2 = *pts++;
        if (isClipped(pt1, ul, lr) | isClipped(pt2, ul, lr)) {
            fbSegment(pDrawable, pGC,
                      intToX(pt1) + xoff, intToY(pt1) + yoff,
                      intToX(pt2) + xoff, intToY(pt2) + yoff,
                      !capNotLast, &dashoffset);
        }
        else {
            CalcLineDeltas(intToX(pt1), intToY(pt1),
                           intToX(pt2), intToY(pt2),
                           len, e1, stepmajor, stepminor, 1, bitsStride,
                           octant);
            if (e1 == 0 && len > 3) {
                int x1, x2;
                FbBits *dstLine;
                int dstX, width;
                FbBits startmask, endmask;
                int nmiddle;

                if (stepmajor < 0) {
                    x1 = intToX(pt2);
                    x2 = intToX(pt1) + 1;
                    if (capNotLast)
                        x1++;
                }
                else {
                    x1 = intToX(pt1);
                    x2 = intToX(pt2);
                    if (!capNotLast)
                        x2++;
                }
                dstX = (x1 + xoff + dstXoff) * (sizeof(UNIT) * 8);
                width = (x2 - x1) * (sizeof(UNIT) * 8);

                dstLine = dst + (intToY(pt1) + yoff + dstYoff) * dstStride;
                dstLine += dstX >> FB_SHIFT;
                dstX &= FB_MASK;
                FbMaskBits(dstX, width, startmask, nmiddle, endmask);
                if (startmask) {
                    WRITE(dstLine,
                          FbDoMaskRRop(READ(dstLine), andBits, xorBits,
                                       startmask));
                    dstLine++;
                }
                if (!andBits)
                    while (nmiddle--)
                        WRITE(dstLine++, xorBits);
                else
                    while (nmiddle--) {
                        WRITE(dstLine,
                              FbDoRRop(READ(dstLine), andBits, xorBits));
                        dstLine++;
                    }
                if (endmask)
                    WRITE(dstLine,
                          FbDoMaskRRop(READ(dstLine), andBits, xorBits,
                                       endmask));
            }
            else {
                bits = bitsBase + intToY(pt1) * bitsStride + intToX(pt1);
                if (len < e1) {
                    e3 = len;
                    len = e1;
                    e1 = e3;

                    e3 = stepminor;
                    stepminor = stepmajor;
                    stepmajor = e3;
                    SetYMajorOctant(octant);
                }
                e = -len;
                e1 <<= 1;
                e3 = e << 1;
                FIXUP_ERROR(e, octant, bias);
                if (!capNotLast)
                    len++;
                if (and == 0) {
                    while (len--) {
                        STORE(bits, xor);
                        bits += stepmajor;
                        e += e1;
                        if (e >= 0) {
                            bits += stepminor;
                            e += e3;
                        }
                    }
                }
                else {
                    while (len--) {
                        RROP(bits, and, xor);
                        bits += stepmajor;
                        e += e1;
                        if (e >= 0) {
                            bits += stepminor;
                            e += e3;
                        }
                    }
                }
            }
        }
    }

    fbFinishAccess(pDrawable);
}
#endif

#undef STORE
#undef RROP
#undef UNIT
#undef USE_SOLID

#undef isClipped
