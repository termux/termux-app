/*
 *
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifndef _PICTURE_H_
#define _PICTURE_H_

#include "privates.h"

#include <pixman.h>

typedef struct _DirectFormat *DirectFormatPtr;
typedef struct _PictFormat *PictFormatPtr;
typedef struct _Picture *PicturePtr;

/*
 * While the protocol is generous in format support, the
 * sample implementation allows only packed RGB and GBR
 * representations for data to simplify software rendering,
 */
#define PICT_FORMAT(bpp,type,a,r,g,b)	PIXMAN_FORMAT(bpp, type, a, r, g, b)

/*
 * gray/color formats use a visual index instead of argb
 */
#define PICT_VISFORMAT(bpp,type,vi)	(((bpp) << 24) |  \
					 ((type) << 16) | \
					 ((vi)))

#define PICT_FORMAT_BPP(f)	PIXMAN_FORMAT_BPP(f)
#define PICT_FORMAT_TYPE(f)	PIXMAN_FORMAT_TYPE(f)
#define PICT_FORMAT_A(f)	PIXMAN_FORMAT_A(f)
#define PICT_FORMAT_R(f)	PIXMAN_FORMAT_R(f)
#define PICT_FORMAT_G(f)	PIXMAN_FORMAT_G(f)
#define PICT_FORMAT_B(f)	PIXMAN_FORMAT_B(f)
#define PICT_FORMAT_RGB(f)	PIXMAN_FORMAT_RGB(f)
#define PICT_FORMAT_VIS(f)	PIXMAN_FORMAT_VIS(f)

#define PICT_TYPE_OTHER		PIXMAN_TYPE_OTHER
#define PICT_TYPE_A		PIXMAN_TYPE_A
#define PICT_TYPE_ARGB		PIXMAN_TYPE_ARGB
#define PICT_TYPE_ABGR		PIXMAN_TYPE_ABGR
#define PICT_TYPE_COLOR		PIXMAN_TYPE_COLOR
#define PICT_TYPE_GRAY		PIXMAN_TYPE_GRAY
#define PICT_TYPE_BGRA		PIXMAN_TYPE_BGRA

#define PICT_FORMAT_COLOR(f)	PIXMAN_FORMAT_COLOR(f)

/* 32bpp formats */
typedef enum _PictFormatShort {
    PICT_a2r10g10b10 = PIXMAN_a2r10g10b10,
    PICT_x2r10g10b10 = PIXMAN_x2r10g10b10,
    PICT_a2b10g10r10 = PIXMAN_a2b10g10r10,
    PICT_x2b10g10r10 = PIXMAN_x2b10g10r10,

    PICT_a8r8g8b8 = PIXMAN_a8r8g8b8,
    PICT_x8r8g8b8 = PIXMAN_x8r8g8b8,
    PICT_a8b8g8r8 = PIXMAN_a8b8g8r8,
    PICT_x8b8g8r8 = PIXMAN_x8b8g8r8,
    PICT_b8g8r8a8 = PIXMAN_b8g8r8a8,
    PICT_b8g8r8x8 = PIXMAN_b8g8r8x8,

/* 24bpp formats */
    PICT_r8g8b8 = PIXMAN_r8g8b8,
    PICT_b8g8r8 = PIXMAN_b8g8r8,

/* 16bpp formats */
    PICT_r5g6b5 = PIXMAN_r5g6b5,
    PICT_b5g6r5 = PIXMAN_b5g6r5,

    PICT_a1r5g5b5 = PIXMAN_a1r5g5b5,
    PICT_x1r5g5b5 = PIXMAN_x1r5g5b5,
    PICT_a1b5g5r5 = PIXMAN_a1b5g5r5,
    PICT_x1b5g5r5 = PIXMAN_x1b5g5r5,
    PICT_a4r4g4b4 = PIXMAN_a4r4g4b4,
    PICT_x4r4g4b4 = PIXMAN_x4r4g4b4,
    PICT_a4b4g4r4 = PIXMAN_a4b4g4r4,
    PICT_x4b4g4r4 = PIXMAN_x4b4g4r4,

/* 8bpp formats */
    PICT_a8 = PIXMAN_a8,
    PICT_r3g3b2 = PIXMAN_r3g3b2,
    PICT_b2g3r3 = PIXMAN_b2g3r3,
    PICT_a2r2g2b2 = PIXMAN_a2r2g2b2,
    PICT_a2b2g2r2 = PIXMAN_a2b2g2r2,

    PICT_c8 = PIXMAN_c8,
    PICT_g8 = PIXMAN_g8,

    PICT_x4a4 = PIXMAN_x4a4,

    PICT_x4c4 = PIXMAN_x4c4,
    PICT_x4g4 = PIXMAN_x4g4,

/* 4bpp formats */
    PICT_a4 = PIXMAN_a4,
    PICT_r1g2b1 = PIXMAN_r1g2b1,
    PICT_b1g2r1 = PIXMAN_b1g2r1,
    PICT_a1r1g1b1 = PIXMAN_a1r1g1b1,
    PICT_a1b1g1r1 = PIXMAN_a1b1g1r1,

    PICT_c4 = PIXMAN_c4,
    PICT_g4 = PIXMAN_g4,

/* 1bpp formats */
    PICT_a1 = PIXMAN_a1,

    PICT_g1 = PIXMAN_g1
} PictFormatShort;

/*
 * For dynamic indexed visuals (GrayScale and PseudoColor), these control the
 * selection of colors allocated for drawing to Pictures.  The default
 * policy depends on the size of the colormap:
 *
 * Size		Default Policy
 * ----------------------------
 *  < 64	PolicyMono
 *  < 256	PolicyGray
 *  256		PolicyColor (only on PseudoColor)
 *
 * The actual allocation code lives in miindex.c, and so is
 * austensibly server dependent, but that code does:
 *
 * PolicyMono	    Allocate no additional colors, use black and white
 * PolicyGray	    Allocate 13 gray levels (11 cells used)
 * PolicyColor	    Allocate a 4x4x4 cube and 13 gray levels (71 cells used)
 * PolicyAll	    Allocate as big a cube as possible, fill with gray (all)
 *
 * Here's a picture to help understand how many colors are
 * actually allocated (this is just the gray ramp):
 *
 *                 gray level
 * all   0000 1555 2aaa 4000 5555 6aaa 8000 9555 aaaa bfff d555 eaaa ffff
 * b/w   0000                                                        ffff
 * 4x4x4                     5555                aaaa
 * extra      1555 2aaa 4000      6aaa 8000 9555      bfff d555 eaaa
 *
 * The default colormap supplies two gray levels (black/white), the
 * 4x4x4 cube allocates another two and nine more are allocated to fill
 * in the 13 levels.  When the 4x4x4 cube is not allocated, a total of
 * 11 cells are allocated.
 */

#define PictureCmapPolicyInvalid    -1
#define PictureCmapPolicyDefault    0
#define PictureCmapPolicyMono	    1
#define PictureCmapPolicyGray	    2
#define PictureCmapPolicyColor	    3
#define PictureCmapPolicyAll	    4

extern int PictureCmapPolicy;

extern int PictureParseCmapPolicy(const char *name);

extern int RenderErrBase;

/* Fixed point updates from Carl Worth, USC, Information Sciences Institute */

typedef pixman_fixed_32_32_t xFixed_32_32;

typedef pixman_fixed_48_16_t xFixed_48_16;

#define MAX_FIXED_48_16		pixman_max_fixed_48_16
#define MIN_FIXED_48_16		pixman_min_fixed_48_16

typedef pixman_fixed_1_31_t xFixed_1_31;
typedef pixman_fixed_1_16_t xFixed_1_16;
typedef pixman_fixed_16_16_t xFixed_16_16;

/*
 * An unadorned "xFixed" is the same as xFixed_16_16,
 * (since it's quite common in the code)
 */
typedef pixman_fixed_t xFixed;

#define XFIXED_BITS	16

#define xFixedToInt(f)	pixman_fixed_to_int(f)
#define IntToxFixed(i)	pixman_int_to_fixed(i)
#define xFixedE		pixman_fixed_e
#define xFixed1		pixman_fixed_1
#define xFixed1MinusE	pixman_fixed_1_minus_e
#define xFixedFrac(f)	pixman_fixed_frac(f)
#define xFixedFloor(f)	pixman_fixed_floor(f)
#define xFixedCeil(f)	pixman_fixed_ceil(f)

#define xFixedFraction(f)	pixman_fixed_fraction(f)
#define xFixedMod2(f)		pixman_fixed_mod2(f)

/* whether 't' is a well defined not obviously empty trapezoid */
#define xTrapezoidValid(t)  ((t)->left.p1.y != (t)->left.p2.y && \
			     (t)->right.p1.y != (t)->right.p2.y && \
			     ((t)->bottom > (t)->top))

/*
 * Standard NTSC luminance conversions:
 *
 *  y = r * 0.299 + g * 0.587 + b * 0.114
 *
 * Approximate this for a bit more speed:
 *
 *  y = (r * 153 + g * 301 + b * 58) / 512
 *
 * This gives 17 bits of luminance; to get 15 bits, lop the low two
 */

#define CvtR8G8B8toY15(s)	(((((s) >> 16) & 0xff) * 153 + \
				  (((s) >>  8) & 0xff) * 301 + \
				  (((s)      ) & 0xff) * 58) >> 2)

#endif                          /* _PICTURE_H_ */
