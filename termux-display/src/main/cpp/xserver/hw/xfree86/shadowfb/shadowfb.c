/*
   Copyright (C) 1999.  The XFree86 Project Inc.
   Copyright 2014 Red Hat, Inc.

   Written by Mark Vojkovich (mvojkovi@ucsd.edu)
   Pre-fb-write callbacks and RENDER support - Nolan Leake (nolan@vmware.com)
*/

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "pixmapstr.h"
#include "input.h"
#include <X11/fonts/font.h>
#include "mi.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "dixfontstr.h"
#include <X11/fonts/fontstruct.h>
#include "xf86.h"
#include "xf86str.h"
#include "shadowfb.h"

#include "picturestr.h"

static Bool ShadowCloseScreen(ScreenPtr pScreen);
static Bool ShadowCreateRootWindow(WindowPtr pWin);

typedef struct {
    ScrnInfoPtr pScrn;
    RefreshAreaFuncPtr preRefresh;
    RefreshAreaFuncPtr postRefresh;
    CloseScreenProcPtr CloseScreen;
    CreateWindowProcPtr CreateWindow;
} ShadowScreenRec, *ShadowScreenPtr;

static DevPrivateKeyRec ShadowScreenKeyRec;

static ShadowScreenPtr
shadowfbGetScreenPrivate(ScreenPtr pScreen)
{
    return dixLookupPrivate(&(pScreen)->devPrivates, &ShadowScreenKeyRec);
}

Bool
ShadowFBInit2(ScreenPtr pScreen,
              RefreshAreaFuncPtr preRefreshArea,
              RefreshAreaFuncPtr postRefreshArea)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    ShadowScreenPtr pPriv;

    if (!preRefreshArea && !postRefreshArea)
        return FALSE;

    if (!dixRegisterPrivateKey(&ShadowScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (!(pPriv = (ShadowScreenPtr) malloc(sizeof(ShadowScreenRec))))
        return FALSE;

    dixSetPrivate(&pScreen->devPrivates, &ShadowScreenKeyRec, pPriv);

    pPriv->pScrn = pScrn;
    pPriv->preRefresh = preRefreshArea;
    pPriv->postRefresh = postRefreshArea;

    pPriv->CloseScreen = pScreen->CloseScreen;
    pPriv->CreateWindow = pScreen->CreateWindow;

    pScreen->CloseScreen = ShadowCloseScreen;
    pScreen->CreateWindow = ShadowCreateRootWindow;

    return TRUE;
}

Bool
ShadowFBInit(ScreenPtr pScreen, RefreshAreaFuncPtr refreshArea)
{
    return ShadowFBInit2(pScreen, NULL, refreshArea);
}

/*
 * Note that we don't do DamageEmpty, or indeed look at the region inside the
 * DamagePtr at all.  This is an optimization, believe it or not.  The
 * incoming RegionPtr is the new damage, and if we were to empty the region
 * miext/damage would just have to waste time reallocating and re-unioning
 * it every time, whereas if we leave it around the union gets fast-pathed
 * away.
 */

static void
shadowfbReportPre(DamagePtr damage, RegionPtr reg, void *closure)
{
    ShadowScreenPtr pPriv = closure;

    if (!pPriv->pScrn->vtSema)
        return;

    pPriv->preRefresh(pPriv->pScrn, RegionNumRects(reg), RegionRects(reg));
}

static void
shadowfbReportPost(DamagePtr damage, RegionPtr reg, void *closure)
{
    ShadowScreenPtr pPriv = closure;

    if (!pPriv->pScrn->vtSema)
        return;

    pPriv->postRefresh(pPriv->pScrn, RegionNumRects(reg), RegionRects(reg));
}

static Bool
ShadowCreateRootWindow(WindowPtr pWin)
{
    Bool ret;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    ShadowScreenPtr pPriv = shadowfbGetScreenPrivate(pScreen);

    /* paranoia */
    if (pWin != pScreen->root)
        ErrorF("ShadowCreateRootWindow called unexpectedly\n");

    /* call down, but don't hook ourselves back in; we know the first time
     * we're called it's for the root window.
     */
    pScreen->CreateWindow = pPriv->CreateWindow;
    ret = pScreen->CreateWindow(pWin);

    /* this might look like it leaks, but the damage code reaps listeners
     * when their drawable disappears.
     */
    if (ret) {
        DamagePtr damage;

        if (pPriv->preRefresh) {
            damage = DamageCreate(shadowfbReportPre, NULL,
                                  DamageReportRawRegion,
                                  TRUE, pScreen, pPriv);
            DamageRegister(&pWin->drawable, damage);
        }

        if (pPriv->postRefresh) {
            damage = DamageCreate(shadowfbReportPost, NULL,
                                  DamageReportRawRegion,
                                  TRUE, pScreen, pPriv);
            DamageSetReportAfterOp(damage, TRUE);
            DamageRegister(&pWin->drawable, damage);
        }
    }

    return ret;
}

static Bool
ShadowCloseScreen(ScreenPtr pScreen)
{
    ShadowScreenPtr pPriv = shadowfbGetScreenPrivate(pScreen);

    pScreen->CloseScreen = pPriv->CloseScreen;

    free(pPriv);

    return (*pScreen->CloseScreen) (pScreen);
}
