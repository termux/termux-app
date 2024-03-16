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

/*
 * Stipple masks are independent of bit/byte order as long
 * as bitorder == byteorder.  FB doesn't handle the case
 * where these differ
 */
#define BitsMask(x,w)	((FB_ALLONES << ((x) & FB_MASK)) & \
			 (FB_ALLONES >> ((FB_UNIT - ((x) + (w))) & FB_MASK)))

#define Mask(x,w)	BitsMask((x)*(w),(w))

#define SelMask(b,n,w)	((((b) >> n) & 1) * Mask(n,w))

#define C1(b,w) \
    (SelMask(b,0,w))

#define C2(b,w) \
    (SelMask(b,0,w) | \
     SelMask(b,1,w))

#define C4(b,w) \
    (SelMask(b,0,w) | \
     SelMask(b,1,w) | \
     SelMask(b,2,w) | \
     SelMask(b,3,w))

#define C8(b,w) \
    (SelMask(b,0,w) | \
     SelMask(b,1,w) | \
     SelMask(b,2,w) | \
     SelMask(b,3,w) | \
     SelMask(b,4,w) | \
     SelMask(b,5,w) | \
     SelMask(b,6,w) | \
     SelMask(b,7,w))

static const FbBits fbStipple8Bits[256] = {
    C8(0, 4), C8(1, 4), C8(2, 4), C8(3, 4), C8(4, 4), C8(5, 4),
    C8(6, 4), C8(7, 4), C8(8, 4), C8(9, 4), C8(10, 4), C8(11, 4),
    C8(12, 4), C8(13, 4), C8(14, 4), C8(15, 4), C8(16, 4), C8(17, 4),
    C8(18, 4), C8(19, 4), C8(20, 4), C8(21, 4), C8(22, 4), C8(23, 4),
    C8(24, 4), C8(25, 4), C8(26, 4), C8(27, 4), C8(28, 4), C8(29, 4),
    C8(30, 4), C8(31, 4), C8(32, 4), C8(33, 4), C8(34, 4), C8(35, 4),
    C8(36, 4), C8(37, 4), C8(38, 4), C8(39, 4), C8(40, 4), C8(41, 4),
    C8(42, 4), C8(43, 4), C8(44, 4), C8(45, 4), C8(46, 4), C8(47, 4),
    C8(48, 4), C8(49, 4), C8(50, 4), C8(51, 4), C8(52, 4), C8(53, 4),
    C8(54, 4), C8(55, 4), C8(56, 4), C8(57, 4), C8(58, 4), C8(59, 4),
    C8(60, 4), C8(61, 4), C8(62, 4), C8(63, 4), C8(64, 4), C8(65, 4),
    C8(66, 4), C8(67, 4), C8(68, 4), C8(69, 4), C8(70, 4), C8(71, 4),
    C8(72, 4), C8(73, 4), C8(74, 4), C8(75, 4), C8(76, 4), C8(77, 4),
    C8(78, 4), C8(79, 4), C8(80, 4), C8(81, 4), C8(82, 4), C8(83, 4),
    C8(84, 4), C8(85, 4), C8(86, 4), C8(87, 4), C8(88, 4), C8(89, 4),
    C8(90, 4), C8(91, 4), C8(92, 4), C8(93, 4), C8(94, 4), C8(95, 4),
    C8(96, 4), C8(97, 4), C8(98, 4), C8(99, 4), C8(100, 4), C8(101, 4),
    C8(102, 4), C8(103, 4), C8(104, 4), C8(105, 4), C8(106, 4), C8(107, 4),
    C8(108, 4), C8(109, 4), C8(110, 4), C8(111, 4), C8(112, 4), C8(113, 4),
    C8(114, 4), C8(115, 4), C8(116, 4), C8(117, 4), C8(118, 4), C8(119, 4),
    C8(120, 4), C8(121, 4), C8(122, 4), C8(123, 4), C8(124, 4), C8(125, 4),
    C8(126, 4), C8(127, 4), C8(128, 4), C8(129, 4), C8(130, 4), C8(131, 4),
    C8(132, 4), C8(133, 4), C8(134, 4), C8(135, 4), C8(136, 4), C8(137, 4),
    C8(138, 4), C8(139, 4), C8(140, 4), C8(141, 4), C8(142, 4), C8(143, 4),
    C8(144, 4), C8(145, 4), C8(146, 4), C8(147, 4), C8(148, 4), C8(149, 4),
    C8(150, 4), C8(151, 4), C8(152, 4), C8(153, 4), C8(154, 4), C8(155, 4),
    C8(156, 4), C8(157, 4), C8(158, 4), C8(159, 4), C8(160, 4), C8(161, 4),
    C8(162, 4), C8(163, 4), C8(164, 4), C8(165, 4), C8(166, 4), C8(167, 4),
    C8(168, 4), C8(169, 4), C8(170, 4), C8(171, 4), C8(172, 4), C8(173, 4),
    C8(174, 4), C8(175, 4), C8(176, 4), C8(177, 4), C8(178, 4), C8(179, 4),
    C8(180, 4), C8(181, 4), C8(182, 4), C8(183, 4), C8(184, 4), C8(185, 4),
    C8(186, 4), C8(187, 4), C8(188, 4), C8(189, 4), C8(190, 4), C8(191, 4),
    C8(192, 4), C8(193, 4), C8(194, 4), C8(195, 4), C8(196, 4), C8(197, 4),
    C8(198, 4), C8(199, 4), C8(200, 4), C8(201, 4), C8(202, 4), C8(203, 4),
    C8(204, 4), C8(205, 4), C8(206, 4), C8(207, 4), C8(208, 4), C8(209, 4),
    C8(210, 4), C8(211, 4), C8(212, 4), C8(213, 4), C8(214, 4), C8(215, 4),
    C8(216, 4), C8(217, 4), C8(218, 4), C8(219, 4), C8(220, 4), C8(221, 4),
    C8(222, 4), C8(223, 4), C8(224, 4), C8(225, 4), C8(226, 4), C8(227, 4),
    C8(228, 4), C8(229, 4), C8(230, 4), C8(231, 4), C8(232, 4), C8(233, 4),
    C8(234, 4), C8(235, 4), C8(236, 4), C8(237, 4), C8(238, 4), C8(239, 4),
    C8(240, 4), C8(241, 4), C8(242, 4), C8(243, 4), C8(244, 4), C8(245, 4),
    C8(246, 4), C8(247, 4), C8(248, 4), C8(249, 4), C8(250, 4), C8(251, 4),
    C8(252, 4), C8(253, 4), C8(254, 4), C8(255, 4),
};

static const FbBits fbStipple4Bits[16] = {
    C4(0, 8), C4(1, 8), C4(2, 8), C4(3, 8), C4(4, 8), C4(5, 8),
    C4(6, 8), C4(7, 8), C4(8, 8), C4(9, 8), C4(10, 8), C4(11, 8),
    C4(12, 8), C4(13, 8), C4(14, 8), C4(15, 8),
};

static const FbBits fbStipple2Bits[4] = {
    C2(0, 16), C2(1, 16), C2(2, 16), C2(3, 16),
};

static const FbBits fbStipple1Bits[2] = {
    C1(0, 32), C1(1, 32),
};

#ifdef __clang__
/* shift overflow is intentional */
#pragma clang diagnostic ignored "-Wshift-overflow"
#endif

/*
 *  Example: srcX = 13 dstX = 8	(FB unit 32 dstBpp 8)
 *
 *	**** **** **** **** **** **** **** ****
 *			^
 *	********  ********  ********  ********
 *		  ^
 *  leftShift = 12
 *  rightShift = 20
 *
 *  Example: srcX = 0 dstX = 8 (FB unit 32 dstBpp 8)
 *
 *	**** **** **** **** **** **** **** ****
 *	^		
 *	********  ********  ********  ********
 *		  ^
 *
 *  leftShift = 24
 *  rightShift = 8
 */

#define LoadBits {\
    if (leftShift) { \
	bitsRight = (src < srcEnd ? READ(src++) : 0); \
	bits = (FbStipLeft (bitsLeft, leftShift) | \
		FbStipRight(bitsRight, rightShift)); \
	bitsLeft = bitsRight; \
    } else \
	bits = (src < srcEnd ? READ(src++) : 0); \
}

void
fbBltOne(FbStip * src, FbStride srcStride,      /* FbStip units per scanline */
         int srcX,              /* bit position of source */
         FbBits * dst, FbStride dstStride,      /* FbBits units per scanline */
         int dstX,              /* bit position of dest */
         int dstBpp,            /* bits per destination unit */
         int width,             /* width in bits of destination */
         int height,            /* height in scanlines */
         FbBits fgand,          /* rrop values */
         FbBits fgxor, FbBits bgand, FbBits bgxor)
{
    const FbBits *fbBits;
    FbBits *srcEnd;
    int pixelsPerDst;           /* dst pixels per FbBits */
    int unitsPerSrc;            /* src patterns per FbStip */
    int leftShift, rightShift;  /* align source with dest */
    FbBits startmask, endmask;  /* dest scanline masks */
    FbStip bits = 0, bitsLeft, bitsRight;       /* source bits */
    FbStip left;
    FbBits mask;
    int nDst;                   /* dest longwords (w.o. end) */
    int w;
    int n, nmiddle;
    int dstS;                   /* stipple-relative dst X coordinate */
    Bool copy;                  /* accelerate dest-invariant */
    Bool transparent;           /* accelerate 0 nop */
    int srcinc;                 /* source units consumed */
    Bool endNeedsLoad = FALSE;  /* need load for endmask */
    int startbyte, endbyte;

    /*
     * Do not read past the end of the buffer!
     */
    srcEnd = src + height * srcStride;

    /*
     * Number of destination units in FbBits == number of stipple pixels
     * used each time
     */
    pixelsPerDst = FB_UNIT / dstBpp;

    /*
     * Number of source stipple patterns in FbStip
     */
    unitsPerSrc = FB_STIP_UNIT / pixelsPerDst;

    copy = FALSE;
    transparent = FALSE;
    if (bgand == 0 && fgand == 0)
        copy = TRUE;
    else if (bgand == FB_ALLONES && bgxor == 0)
        transparent = TRUE;

    /*
     * Adjust source and dest to nearest FbBits boundary
     */
    src += srcX >> FB_STIP_SHIFT;
    dst += dstX >> FB_SHIFT;
    srcX &= FB_STIP_MASK;
    dstX &= FB_MASK;

    FbMaskBitsBytes(dstX, width, copy,
                    startmask, startbyte, nmiddle, endmask, endbyte);

    /*
     * Compute effective dest alignment requirement for
     * source -- must align source to dest unit boundary
     */
    dstS = dstX / dstBpp;
    /*
     * Compute shift constants for effective alignment
     */
    if (srcX >= dstS) {
        leftShift = srcX - dstS;
        rightShift = FB_STIP_UNIT - leftShift;
    }
    else {
        rightShift = dstS - srcX;
        leftShift = FB_STIP_UNIT - rightShift;
    }
    /*
     * Get pointer to stipple mask array for this depth
     */
    fbBits = 0;                 /* unused */
    switch (pixelsPerDst) {
    case 8:
        fbBits = fbStipple8Bits;
        break;
    case 4:
        fbBits = fbStipple4Bits;
        break;
    case 2:
        fbBits = fbStipple2Bits;
        break;
    case 1:
        fbBits = fbStipple1Bits;
        break;
    default:
        return;
    }

    /*
     * Compute total number of destination words written, but
     * don't count endmask
     */
    nDst = nmiddle;
    if (startmask)
        nDst++;

    dstStride -= nDst;

    /*
     * Compute total number of source words consumed
     */

    srcinc = (nDst + unitsPerSrc - 1) / unitsPerSrc;

    if (srcX > dstS)
        srcinc++;
    if (endmask) {
        endNeedsLoad = nDst % unitsPerSrc == 0;
        if (endNeedsLoad)
            srcinc++;
    }

    srcStride -= srcinc;

    /*
     * Copy rectangle
     */
    while (height--) {
        w = nDst;               /* total units across scanline */
        n = unitsPerSrc;        /* units avail in single stipple */
        if (n > w)
            n = w;

        bitsLeft = 0;
        if (srcX > dstS)
            bitsLeft = READ(src++);
        if (n) {
            /*
             * Load first set of stipple bits
             */
            LoadBits;

            /*
             * Consume stipple bits for startmask
             */
            if (startmask) {
                mask = fbBits[FbLeftStipBits(bits, pixelsPerDst)];
                if (mask || !transparent)
                    FbDoLeftMaskByteStippleRRop(dst, mask,
                                                fgand, fgxor, bgand, bgxor,
                                                startbyte, startmask);
                bits = FbStipLeft(bits, pixelsPerDst);
                dst++;
                n--;
                w--;
            }
            /*
             * Consume stipple bits across scanline
             */
            for (;;) {
                w -= n;
                if (copy) {
                    while (n--) {
                        mask = fbBits[FbLeftStipBits(bits, pixelsPerDst)];
                        WRITE(dst, FbOpaqueStipple(mask, fgxor, bgxor));
                        dst++;
                        bits = FbStipLeft(bits, pixelsPerDst);
                    }
                }
                else {
                    while (n--) {
                        left = FbLeftStipBits(bits, pixelsPerDst);
                        if (left || !transparent) {
                            mask = fbBits[left];
                            WRITE(dst, FbStippleRRop(READ(dst), mask, fgand,
                                                     fgxor, bgand, bgxor));
                        }
                        dst++;
                        bits = FbStipLeft(bits, pixelsPerDst);
                    }
                }
                if (!w)
                    break;
                /*
                 * Load another set and reset number of available units
                 */
                LoadBits;
                n = unitsPerSrc;
                if (n > w)
                    n = w;
            }
        }
        /*
         * Consume stipple bits for endmask
         */
        if (endmask) {
            if (endNeedsLoad) {
                LoadBits;
            }
            mask = fbBits[FbLeftStipBits(bits, pixelsPerDst)];
            if (mask || !transparent)
                FbDoRightMaskByteStippleRRop(dst, mask, fgand, fgxor,
                                             bgand, bgxor, endbyte, endmask);
        }
        dst += dstStride;
        src += srcStride;
    }
}

/*
 * Not very efficient, but simple -- copy a single plane
 * from an N bit image to a 1 bit image
 */

void
fbBltPlane(FbBits * src,
           FbStride srcStride,
           int srcX,
           int srcBpp,
           FbStip * dst,
           FbStride dstStride,
           int dstX,
           int width,
           int height,
           FbStip fgand,
           FbStip fgxor, FbStip bgand, FbStip bgxor, Pixel planeMask)
{
    FbBits *s;
    FbBits pm;
    FbBits srcMask;
    FbBits srcMaskFirst;
    FbBits srcMask0 = 0;
    FbBits srcBits;

    FbStip dstBits;
    FbStip *d;
    FbStip dstMask;
    FbStip dstMaskFirst;
    FbStip dstUnion;
    int w;
    int wt;

    if (!width)
        return;

    src += srcX >> FB_SHIFT;
    srcX &= FB_MASK;

    dst += dstX >> FB_STIP_SHIFT;
    dstX &= FB_STIP_MASK;

    w = width / srcBpp;

    pm = fbReplicatePixel(planeMask, srcBpp);
    srcMaskFirst = pm & FbBitsMask(srcX, srcBpp);
    srcMask0 = pm & FbBitsMask(0, srcBpp);

    dstMaskFirst = FbStipMask(dstX, 1);
    while (height--) {
        d = dst;
        dst += dstStride;
        s = src;
        src += srcStride;

        srcMask = srcMaskFirst;
        srcBits = READ(s++);

        dstMask = dstMaskFirst;
        dstUnion = 0;
        dstBits = 0;

        wt = w;

        while (wt--) {
            if (!srcMask) {
                srcBits = READ(s++);
                srcMask = srcMask0;
            }
            if (!dstMask) {
                WRITE(d, FbStippleRRopMask(READ(d), dstBits,
                                           fgand, fgxor, bgand, bgxor,
                                           dstUnion));
                d++;
                dstMask = FbStipMask(0, 1);
                dstUnion = 0;
                dstBits = 0;
            }
            if (srcBits & srcMask)
                dstBits |= dstMask;
            dstUnion |= dstMask;
            if (srcBpp == FB_UNIT)
                srcMask = 0;
            else
                srcMask = FbScrRight(srcMask, srcBpp);
            dstMask = FbStipRight(dstMask, 1);
        }
        if (dstUnion)
            WRITE(d, FbStippleRRopMask(READ(d), dstBits,
                                       fgand, fgxor, bgand, bgxor, dstUnion));
    }
}
