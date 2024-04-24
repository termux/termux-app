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

void
shadowUpdatePacked(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    RegionPtr damage = DamageRegion(pBuf->pDamage);
    PixmapPtr pShadow = pBuf->pPixmap;
    int nbox = RegionNumRects(damage);
    BoxPtr pbox = RegionRects(damage);
    FbBits *shaBase, *shaLine, *sha;
    FbStride shaStride;
    int scrBase, scrLine, scr;
    int shaBpp;
    _X_UNUSED int shaXoff, shaYoff;
    int x, y, w, h, width;
    int i;
    FbBits *winBase = NULL, *win;
    CARD32 winSize;

    fbGetDrawable(&pShadow->drawable, shaBase, shaStride, shaBpp, shaXoff,
                  shaYoff);
    while (nbox--) {
        x = pbox->x1 * shaBpp;
        y = pbox->y1;
        w = (pbox->x2 - pbox->x1) * shaBpp;
        h = pbox->y2 - pbox->y1;

        scrLine = (x >> FB_SHIFT);
        shaLine = shaBase + y * shaStride + (x >> FB_SHIFT);

        x &= FB_MASK;
        w = (w + x + FB_MASK) >> FB_SHIFT;

        while (h--) {
            winSize = 0;
            scrBase = 0;
            width = w;
            scr = scrLine;
            sha = shaLine;
            while (width) {
                /* how much remains in this window */
                i = scrBase + winSize - scr;
                if (i <= 0 || scr < scrBase) {
                    winBase = (FbBits *) (*pBuf->window) (pScreen,
                                                          y,
                                                          scr * sizeof(FbBits),
                                                          SHADOW_WINDOW_WRITE,
                                                          &winSize,
                                                          pBuf->closure);
                    if (!winBase)
                        return;
                    scrBase = scr;
                    winSize /= sizeof(FbBits);
                    i = winSize;
                }
                win = winBase + (scr - scrBase);
                if (i > width)
                    i = width;
                width -= i;
                scr += i;
                memcpy(win, sha, i * sizeof(FbBits));
                sha += i;
            }
            shaLine += shaStride;
            y++;
        }
        pbox++;
    }
}
