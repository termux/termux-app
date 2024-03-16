/*
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
#include    "dixfontstr.h"
#include    "mi.h"
#include    "regionstr.h"
#include    "globals.h"
#include    "gcstruct.h"
#include    "shadow.h"

static DevPrivateKeyRec shadowScrPrivateKeyRec;
#define shadowScrPrivateKey (&shadowScrPrivateKeyRec)

#define shadowGetBuf(pScr) ((shadowBufPtr) \
    dixLookupPrivate(&(pScr)->devPrivates, shadowScrPrivateKey))
#define shadowBuf(pScr)            shadowBufPtr pBuf = shadowGetBuf(pScr)

#define wrap(priv, real, mem) {\
    priv->mem = real->mem; \
    real->mem = shadow##mem; \
}

#define unwrap(priv, real, mem) {\
    real->mem = priv->mem; \
}

static void
shadowRedisplay(ScreenPtr pScreen)
{
    shadowBuf(pScreen);
    RegionPtr pRegion;

    if (!pBuf || !pBuf->pDamage || !pBuf->update)
        return;
    pRegion = DamageRegion(pBuf->pDamage);
    if (RegionNotEmpty(pRegion)) {
        (*pBuf->update) (pScreen, pBuf);
        DamageEmpty(pBuf->pDamage);
    }
}

static void
shadowBlockHandler(ScreenPtr pScreen, void *timeout)
{
    shadowBuf(pScreen);

    shadowRedisplay(pScreen);

    unwrap(pBuf, pScreen, BlockHandler);
    pScreen->BlockHandler(pScreen, timeout);
    wrap(pBuf, pScreen, BlockHandler);
}

static void
shadowGetImage(DrawablePtr pDrawable, int sx, int sy, int w, int h,
               unsigned int format, unsigned long planeMask, char *pdstLine)
{
    ScreenPtr pScreen = pDrawable->pScreen;

    shadowBuf(pScreen);

    /* Many apps use GetImage to sync with the visible frame buffer */
    if (pDrawable->type == DRAWABLE_WINDOW)
        shadowRedisplay(pScreen);
    unwrap(pBuf, pScreen, GetImage);
    pScreen->GetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
    wrap(pBuf, pScreen, GetImage);
}

static Bool
shadowCloseScreen(ScreenPtr pScreen)
{
    shadowBuf(pScreen);

    unwrap(pBuf, pScreen, GetImage);
    unwrap(pBuf, pScreen, CloseScreen);
    unwrap(pBuf, pScreen, BlockHandler);
    shadowRemove(pScreen, pBuf->pPixmap);
    DamageDestroy(pBuf->pDamage);
    if (pBuf->pPixmap)
        pScreen->DestroyPixmap(pBuf->pPixmap);
    free(pBuf);
    return pScreen->CloseScreen(pScreen);
}

Bool
shadowSetup(ScreenPtr pScreen)
{
    shadowBufPtr pBuf;

    if (!dixRegisterPrivateKey(&shadowScrPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (!DamageSetup(pScreen))
        return FALSE;

    pBuf = malloc(sizeof(shadowBufRec));
    if (!pBuf)
        return FALSE;
    pBuf->pDamage = DamageCreate((DamageReportFunc) NULL,
                                 (DamageDestroyFunc) NULL,
                                 DamageReportNone, TRUE, pScreen, pScreen);
    if (!pBuf->pDamage) {
        free(pBuf);
        return FALSE;
    }

    wrap(pBuf, pScreen, CloseScreen);
    wrap(pBuf, pScreen, GetImage);
    wrap(pBuf, pScreen, BlockHandler);
    pBuf->update = 0;
    pBuf->window = 0;
    pBuf->pPixmap = 0;
    pBuf->closure = 0;
    pBuf->randr = 0;

    dixSetPrivate(&pScreen->devPrivates, shadowScrPrivateKey, pBuf);
    return TRUE;
}

Bool
shadowAdd(ScreenPtr pScreen, PixmapPtr pPixmap, ShadowUpdateProc update,
          ShadowWindowProc window, int randr, void *closure)
{
    shadowBuf(pScreen);

    /*
     * Map simple rotation values to bitmasks; fortunately,
     * these are all unique
     */
    switch (randr) {
    case 0:
        randr = SHADOW_ROTATE_0;
        break;
    case 90:
        randr = SHADOW_ROTATE_90;
        break;
    case 180:
        randr = SHADOW_ROTATE_180;
        break;
    case 270:
        randr = SHADOW_ROTATE_270;
        break;
    }
    pBuf->update = update;
    pBuf->window = window;
    pBuf->randr = randr;
    pBuf->closure = closure;
    pBuf->pPixmap = pPixmap;
    DamageRegister(&pPixmap->drawable, pBuf->pDamage);
    return TRUE;
}

void
shadowRemove(ScreenPtr pScreen, PixmapPtr pPixmap)
{
    shadowBuf(pScreen);

    if (pBuf->pPixmap) {
        DamageUnregister(pBuf->pDamage);
        pBuf->update = 0;
        pBuf->window = 0;
        pBuf->randr = 0;
        pBuf->closure = 0;
        pBuf->pPixmap = 0;
    }
}
