/*
 * Copyright © 1999 Keith Packard
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
#include "kdrive.h"

Bool
KdShadowFbAlloc(KdScreenInfo * screen, Bool rotate)
{
    int paddedWidth;
    void *buf;
    int width = rotate ? screen->height : screen->width;
    int height = rotate ? screen->width : screen->height;
    int bpp = screen->fb.bitsPerPixel;

    /* use fb computation for width */
    paddedWidth = ((width * bpp + FB_MASK) >> FB_SHIFT) * sizeof(FbBits);
    buf = xallocarray(paddedWidth, height);
    if (!buf)
        return FALSE;
    if (screen->fb.shadow)
        free(screen->fb.frameBuffer);
    screen->fb.shadow = TRUE;
    screen->fb.frameBuffer = buf;
    screen->fb.byteStride = paddedWidth;
    screen->fb.pixelStride = paddedWidth * 8 / bpp;
    return TRUE;
}

void
KdShadowFbFree(KdScreenInfo * screen)
{
    if (screen->fb.shadow) {
        free(screen->fb.frameBuffer);
        screen->fb.frameBuffer = 0;
        screen->fb.shadow = FALSE;
    }
}

Bool
KdShadowSet(ScreenPtr pScreen, int randr, ShadowUpdateProc update,
            ShadowWindowProc window)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;

    shadowRemove(pScreen, pScreen->GetScreenPixmap(pScreen));
    if (screen->fb.shadow) {
        return shadowAdd(pScreen, pScreen->GetScreenPixmap(pScreen),
                         update, window, randr, 0);
    }
    return TRUE;
}

void
KdShadowUnset(ScreenPtr pScreen)
{
    shadowRemove(pScreen, pScreen->GetScreenPixmap(pScreen));
}
