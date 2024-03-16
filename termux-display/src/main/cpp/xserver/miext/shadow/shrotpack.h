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

/*
 * Thanks to Daniel Chemko <dchemko@intrinsyc.com> for making the 90 and 180
 * orientations work.
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

#define DANDEBUG         0

#if ROTATE == 270

#define SCRLEFT(x,y,w,h)    (pScreen->height - ((y) + (h)))
#define SCRY(x,y,w,h)	    (x)
#define SCRWIDTH(x,y,w,h)   (h)
#define FIRSTSHA(x,y,w,h)   (((y) + (h) - 1) * shaStride + (x))
#define STEPDOWN(x,y,w,h)   ((w)--)
#define NEXTY(x,y,w,h)	    ((x)++)
#define SHASTEPX(stride)    -(stride)
#define SHASTEPY(stride)    (1)

#elif ROTATE == 90

#define SCRLEFT(x,y,w,h)    (y)
#define SCRY(x,y,w,h)	    (pScreen->width - ((x) + (w)) - 1)
#define SCRWIDTH(x,y,w,h)   (h)
#define FIRSTSHA(x,y,w,h)   ((y) * shaStride + (x + w - 1))
#define STEPDOWN(x,y,w,h)   ((w)--)
#define NEXTY(x,y,w,h)	    ((void)(x))
#define SHASTEPX(stride)    (stride)
#define SHASTEPY(stride)    (-1)

#elif ROTATE == 180

#define SCRLEFT(x,y,w,h)    (pScreen->width - ((x) + (w)))
#define SCRY(x,y,w,h)	    (pScreen->height - ((y) + (h)) - 1)
#define SCRWIDTH(x,y,w,h)   (w)
#define FIRSTSHA(x,y,w,h)   ((y + h - 1) * shaStride + (x + w - 1))
#define STEPDOWN(x,y,w,h)   ((h)--)
#define NEXTY(x,y,w,h)	    ((void)(y))
#define SHASTEPX(stride)    (-1)
#define SHASTEPY(stride)    -(stride)

#else

#define SCRLEFT(x,y,w,h)    (x)
#define SCRY(x,y,w,h)	    (y)
#define SCRWIDTH(x,y,w,h)   (w)
#define FIRSTSHA(x,y,w,h)   ((y) * shaStride + (x))
#define STEPDOWN(x,y,w,h)   ((h)--)
#define NEXTY(x,y,w,h)	    ((y)++)
#define SHASTEPX(stride)    (1)
#define SHASTEPY(stride)    (stride)

#endif

void
FUNC(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    RegionPtr damage = DamageRegion(pBuf->pDamage);
    PixmapPtr pShadow = pBuf->pPixmap;
    int nbox = RegionNumRects(damage);
    BoxPtr pbox = RegionRects(damage);
    FbBits *shaBits;
    Data *shaBase, *shaLine, *sha;
    FbStride shaStride;
    int scrBase, scrLine, scr;
    int shaBpp;
    _X_UNUSED int shaXoff, shaYoff;
    int x, y, w, h, width;
    int i;
    Data *winBase = NULL, *win;
    CARD32 winSize;

    fbGetDrawable(&pShadow->drawable, shaBits, shaStride, shaBpp, shaXoff,
                  shaYoff);
    shaBase = (Data *) shaBits;
    shaStride = shaStride * sizeof(FbBits) / sizeof(Data);
#if (DANDEBUG > 1)
    ErrorF
        ("-> Entering Shadow Update:\r\n   |- Origins: pShadow=%x, pScreen=%x, damage=%x\r\n   |- Metrics: shaStride=%d, shaBase=%x, shaBpp=%d\r\n   |                                                     \n",
         pShadow, pScreen, damage, shaStride, shaBase, shaBpp);
#endif
    while (nbox--) {
        x = pbox->x1;
        y = pbox->y1;
        w = (pbox->x2 - pbox->x1);
        h = pbox->y2 - pbox->y1;

#if (DANDEBUG > 2)
        ErrorF
            ("   |-> Redrawing box - Metrics: X=%d, Y=%d, Width=%d, Height=%d\n",
             x, y, w, h);
#endif
        scrLine = SCRLEFT(x, y, w, h);
        shaLine = shaBase + FIRSTSHA(x, y, w, h);

        while (STEPDOWN(x, y, w, h)) {
            winSize = 0;
            scrBase = 0;
            width = SCRWIDTH(x, y, w, h);
            scr = scrLine;
            sha = shaLine;
#if (DANDEBUG > 3)
            ErrorF("   |   |-> StepDown - Metrics: width=%d, scr=%x, sha=%x\n",
                   width, scr, sha);
#endif
            while (width) {
                /*  how much remains in this window */
                i = scrBase + winSize - scr;
                if (i <= 0 || scr < scrBase) {
                    winBase = (Data *) (*pBuf->window) (pScreen,
                                                        SCRY(x, y, w, h),
                                                        scr * sizeof(Data),
                                                        SHADOW_WINDOW_WRITE,
                                                        &winSize,
                                                        pBuf->closure);
                    if (!winBase)
                        return;
                    scrBase = scr;
                    winSize /= sizeof(Data);
                    i = winSize;
#if(DANDEBUG > 4)
                    ErrorF
                        ("   |   |   |-> Starting New Line - Metrics: winBase=%x, scrBase=%x, winSize=%d\r\n   |   |   |   Xstride=%d, Ystride=%d, w=%d h=%d\n",
                         winBase, scrBase, winSize, SHASTEPX(shaStride),
                         SHASTEPY(shaStride), w, h);
#endif
                }
                win = winBase + (scr - scrBase);
                if (i > width)
                    i = width;
                width -= i;
                scr += i;
#if(DANDEBUG > 5)
                ErrorF
                    ("   |   |   |-> Writing Line - Metrics: win=%x, sha=%x\n",
                     win, sha);
#endif
                while (i--) {
#if(DANDEBUG > 6)
                    ErrorF
                        ("   |   |   |-> Writing Pixel - Metrics: win=%x, sha=%d, remaining=%d\n",
                         win, sha, i);
#endif
                    *win++ = *sha;
                    sha += SHASTEPX(shaStride);
                }               /*  i */
            }                   /*  width */
            shaLine += SHASTEPY(shaStride);
            NEXTY(x, y, w, h);
        }                       /*  STEPDOWN */
        pbox++;
    }                           /*  nbox */
}
