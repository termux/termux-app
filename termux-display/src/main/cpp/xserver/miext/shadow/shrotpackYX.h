/*
 * Copyright © 2004 Philip Blundell
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Philip Blundell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Philip Blundell makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * PHILIP BLUNDELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL PHILIP BLUNDELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include    <X11/X.h>
#include    "scrnintstr.h"
#include    "windowstr.h"
#include    "dixfontstr.h"
#include    "mi.h"
#include    "regionstr.h"
#include    "globals.h"
#include    "gcstruct.h"
#include    "shadow.h"
#include    "fb.h"

#if ROTATE == 270

#define WINSTEPX(stride)    (stride)
#define WINSTART(x,y)       (((pScreen->height - 1) - y) + (x * winStride))
#define WINSTEPY()	    -1

#elif ROTATE == 90

#define WINSTEPX(stride)    (-stride)
#define WINSTEPY()	    1
#define WINSTART(x,y)       (((pScreen->width - 1 - x) * winStride) + y)

#else

#error This rotation is not supported here

#endif

#ifdef __arm__
#define PREFETCH
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
    FbStride shaStride, winStride;
    int shaBpp;
    _X_UNUSED int shaXoff, shaYoff;
    int x, y, w, h;
    Data *winBase, *win, *winLine;
    CARD32 winSize;

    fbGetDrawable(&pShadow->drawable, shaBits, shaStride, shaBpp, shaXoff,
                  shaYoff);
    shaBase = (Data *) shaBits;
    shaStride = shaStride * sizeof(FbBits) / sizeof(Data);

    winBase = (Data *) (*pBuf->window) (pScreen, 0, 0,
                                        SHADOW_WINDOW_WRITE,
                                        &winSize, pBuf->closure);
    winStride = (Data *) (*pBuf->window) (pScreen, 1, 0,
                                          SHADOW_WINDOW_WRITE,
                                          &winSize, pBuf->closure) - winBase;

    while (nbox--) {
        x = pbox->x1;
        y = pbox->y1;
        w = (pbox->x2 - pbox->x1);
        h = pbox->y2 - pbox->y1;

        shaLine = shaBase + (y * shaStride) + x;
#ifdef PREFETCH
        __builtin_prefetch(shaLine);
#endif
        winLine = winBase + WINSTART(x, y);

        while (h--) {
            sha = shaLine;
            win = winLine;

            while (sha < (shaLine + w - 16)) {
#ifdef PREFETCH
                __builtin_prefetch(sha + shaStride);
#endif
                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);

                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);

                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);

                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);
                *win = *sha++;
                win += WINSTEPX(winStride);
            }

            while (sha < (shaLine + w)) {
                *win = *sha++;
                win += WINSTEPX(winStride);
            }

            y++;
            shaLine += shaStride;
            winLine += WINSTEPY();
        }
        pbox++;
    }                           /*  nbox */
}
