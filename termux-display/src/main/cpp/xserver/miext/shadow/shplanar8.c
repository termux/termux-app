/*
 *
 * Copyright © 2000 Keith Packard
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

#include    <X11/X.h>
#include    "scrnintstr.h"
#include    "windowstr.h"
#include    <X11/fonts/font.h>
#include    "dixfontstr.h"
#include    <X11/fonts/fontstruct.h>
#include    "mi.h"
#include    "regionstr.h"
#include    "globals.h"
#include    "gcstruct.h"
#include    "shadow.h"
#include    "fb.h"

/*
 * Expose 8bpp depth 4
 */

/*
 *  32->8 conversion:
 *
 *      7 6 5 4 3 2 1 0
 *      A B C D E F G H
 *
 *      3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * m1   D x x x x x x x C x x x x x x x B x x x x x x x A x x x x x x x     sha[0] << (7-(p))
 * m2   x x x x H x x x x x x x G x x x x x x x F x x x x x x x E x x x     sha[1] << (3-(p))
 * m3   D               C               B               A                   m1 & 0x80808080
 * m4           H               G               F               E           m2 & 0x08080808
 * m5   D       H       C       G       B       F       A       E	    m3 | m4
 * m6                     D       H       C       G       B       F         m5 >> 9
 * m7   D       H       C D     G H     B C     F G     A B     E F         m5 | m6
 * m8                                       D       H       C D     G H     m7 >> 18
 * m9   D       H       C D     G H     B C D   F G H   A B C D E F G H     m7 | m8
 */

#define PL_SHIFT    8
#define PL_UNIT	    (1 << PL_SHIFT)
#define PL_MASK	    (PL_UNIT - 1)

#if 0
#define GetBits(p,o,d) { \
    CARD32	m1,m2,m3,m4,m5,m6,m7,m8; \
    m1 = sha[o] << (7 - (p)); \
    m2 = sha[(o)+1] << (3 - (p)); \
    m3 = m1 & 0x80808080; \
    m4 = m2 & 0x08080808; \
    m5 = m3 | m4; \
    m6 = m5 >> 9; \
    m7 = m5 | m6; \
    m8 = m7 >> 18; \
    d = m7 | m8; \
}
#else
#define GetBits(p,o,d) { \
    CARD32	m5,m7; \
    m5 = ((sha[o] << (7 - (p))) & 0x80808080) | ((sha[(o)+1] << (3 - (p))) & 0x08080808); \
    m7 = m5 | (m5 >> 9); \
    d = m7 | (m7 >> 18); \
}
#endif

void
shadowUpdatePlanar4x8(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    RegionPtr damage = DamageRegion(pBuf->pDamage);
    PixmapPtr pShadow = pBuf->pPixmap;
    int nbox = RegionNumRects(damage);
    BoxPtr pbox = RegionRects(damage);
    CARD32 *shaBase, *shaLine, *sha;
    CARD8 s1, s2, s3, s4;
    FbStride shaStride;
    int scrBase, scrLine, scr;
    int shaBpp;
    _X_UNUSED int shaXoff, shaYoff;
    int x, y, w, h, width;
    int i;
    CARD32 *winBase = NULL, *win;
    CARD32 winSize;
    int plane;

    fbGetStipDrawable(&pShadow->drawable, shaBase, shaStride, shaBpp, shaXoff,
                      shaYoff);
    while (nbox--) {
        x = pbox->x1 * shaBpp;
        y = pbox->y1;
        w = (pbox->x2 - pbox->x1) * shaBpp;
        h = pbox->y2 - pbox->y1;

        w = (w + (x & PL_MASK) + PL_MASK) >> PL_SHIFT;
        x &= ~PL_MASK;

        scrLine = (x >> PL_SHIFT);
        shaLine = shaBase + y * shaStride + (x >> FB_SHIFT);

        while (h--) {
            for (plane = 0; plane < 4; plane++) {
                width = w;
                scr = scrLine;
                sha = shaLine;
                winSize = 0;
                scrBase = 0;
                while (width) {
                    /* how much remains in this window */
                    i = scrBase + winSize - scr;
                    if (i <= 0 || scr < scrBase) {
                        winBase = (CARD32 *) (*pBuf->window) (pScreen,
                                                              y,
                                                              (scr << 4) |
                                                              (plane),
                                                              SHADOW_WINDOW_WRITE,
                                                              &winSize,
                                                              pBuf->closure);
                        if (!winBase)
                            return;
                        winSize >>= 2;
                        scrBase = scr;
                        i = winSize;
                    }
                    win = winBase + (scr - scrBase);
                    if (i > width)
                        i = width;
                    width -= i;
                    scr += i;

                    while (i--) {
                        GetBits(plane, 0, s1);
                        GetBits(plane, 2, s2);
                        GetBits(plane, 4, s3);
                        GetBits(plane, 6, s4);
                        *win++ = s1 | (s2 << 8) | (s3 << 16) | (s4 << 24);
                        sha += 8;
                    }
                }
            }
            shaLine += shaStride;
            y++;
        }
        pbox++;
    }
}
