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
 * 32 4-bit pixels per write
 */

#define PL_SHIFT    7
#define PL_UNIT	    (1 << PL_SHIFT)
#define PL_MASK	    (PL_UNIT - 1)

/*
 *  32->8 conversion:
 *
 *      7 6 5 4 3 2 1 0
 *      A B C D E F G H
 *
 *      3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * m    . . . H . . . G . . . F . . . E . . . D . . . C . . . B . . . A
 * m1   G . . . F . . . E . . . D . . . C . . . B . . . A . . . . . . .	    m << (7 - (p))
 * m2   . H . . . G . . . F . . . E . . . D . . . C . . . B . . . A . .	    (m >> (p)) << 2
 * m3   G               E               C               A                   m1 & 0x80808080
 * m4     H               F               D               B                 m2 & 0x40404040
 * m5   G H             E F             C D             A B                 m3 | m4
 * m6   G H             E F             C D     G H     A B     E F         m5 | (m5 >> 20)
 * m7   G H             E F             C D     G H     A B C D E F G H     m6 | (m6 >> 10)
 */

#if 0
#define GetBits(p,o,d) {\
    m = sha[o]; \
    m1 = m << (7 - (p)); \
    m2 = (m >> (p)) << 2; \
    m3 = m1 & 0x80808080; \
    m4 = m2 & 0x40404040; \
    m5 = m3 | m4; \
    m6 = m5 | (m5 >> 20); \
    d = m6 | (m6 >> 10); \
}
#else
#define GetBits(p,o,d) {\
    m = sha[o]; \
    m5 = ((m << (7 - (p))) & 0x80808080) | (((m >> (p)) << 2) & 0x40404040); \
    m6 = m5 | (m5 >> 20); \
    d = m6 | (m6 >> 10); \
}
#endif

void
shadowUpdatePlanar4(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    RegionPtr damage = DamageRegion(pBuf->pDamage);
    PixmapPtr pShadow = pBuf->pPixmap;
    int nbox = RegionNumRects(damage);
    BoxPtr pbox = RegionRects(damage);
    CARD32 *shaBase, *shaLine, *sha;
    FbStride shaStride;
    int scrBase, scrLine, scr;
    int shaBpp;
    _X_UNUSED int shaXoff, shaYoff;
    int x, y, w, h, width;
    int i;
    CARD32 *winBase = NULL, *win;
    CARD32 winSize;
    int plane;
    CARD32 m, m5, m6;
    CARD8 s1, s2, s3, s4;

    fbGetStipDrawable(&pShadow->drawable, shaBase, shaStride, shaBpp, shaXoff,
                      shaYoff);
    while (nbox--) {
        x = (pbox->x1) * shaBpp;
        y = (pbox->y1);
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
                        GetBits(plane, 1, s2);
                        GetBits(plane, 2, s3);
                        GetBits(plane, 3, s4);
                        *win++ = s1 | (s2 << 8) | (s3 << 16) | (s4 << 24);
                        sha += 4;
                    }
                }
            }
            shaLine += shaStride;
            y++;
        }
        pbox++;
    }
}
