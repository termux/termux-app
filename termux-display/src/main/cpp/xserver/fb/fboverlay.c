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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "fb.h"
#include "fboverlay.h"
#include "shmint.h"

static DevPrivateKeyRec fbOverlayScreenPrivateKeyRec;

#define fbOverlayScreenPrivateKey (&fbOverlayScreenPrivateKeyRec)

DevPrivateKey
fbOverlayGetScreenPrivateKey(void)
{
    return fbOverlayScreenPrivateKey;
}

/*
 * Replace this if you want something supporting
 * multiple overlays with the same depth
 */
Bool
fbOverlayCreateWindow(WindowPtr pWin)
{
    FbOverlayScrPrivPtr pScrPriv = fbOverlayGetScrPriv(pWin->drawable.pScreen);
    int i;
    PixmapPtr pPixmap;

    if (pWin->drawable.class != InputOutput)
        return TRUE;

    for (i = 0; i < pScrPriv->nlayers; i++) {
        pPixmap = pScrPriv->layer[i].u.run.pixmap;
        if (pWin->drawable.depth == pPixmap->drawable.depth) {
            dixSetPrivate(&pWin->devPrivates, fbGetWinPrivateKey(pWin), pPixmap);
            /*
             * Make sure layer keys are written correctly by
             * having non-root layers set to full while the
             * root layer is set to empty.  This will cause
             * all of the layers to get painted when the root
             * is mapped
             */
            if (!pWin->parent) {
                RegionEmpty(&pScrPriv->layer[i].u.run.region);
            }
            return TRUE;
        }
    }
    return FALSE;
}

Bool
fbOverlayCloseScreen(ScreenPtr pScreen)
{
    FbOverlayScrPrivPtr pScrPriv = fbOverlayGetScrPriv(pScreen);
    int i;

    for (i = 0; i < pScrPriv->nlayers; i++) {
        (*pScreen->DestroyPixmap) (pScrPriv->layer[i].u.run.pixmap);
        RegionUninit(&pScrPriv->layer[i].u.run.region);
    }
    return TRUE;
}

/*
 * Return layer containing this window
 */
int
fbOverlayWindowLayer(WindowPtr pWin)
{
    FbOverlayScrPrivPtr pScrPriv = fbOverlayGetScrPriv(pWin->drawable.pScreen);
    int i;

    for (i = 0; i < pScrPriv->nlayers; i++)
        if (dixLookupPrivate(&pWin->devPrivates, fbGetWinPrivateKey(pWin)) ==
            (void *) pScrPriv->layer[i].u.run.pixmap)
            return i;
    return 0;
}

Bool
fbOverlayCreateScreenResources(ScreenPtr pScreen)
{
    int i;
    FbOverlayScrPrivPtr pScrPriv = fbOverlayGetScrPriv(pScreen);
    PixmapPtr pPixmap;
    void *pbits;
    int width;
    int depth;
    BoxRec box;

    if (!miCreateScreenResources(pScreen))
        return FALSE;

    box.x1 = 0;
    box.y1 = 0;
    box.x2 = pScreen->width;
    box.y2 = pScreen->height;
    for (i = 0; i < pScrPriv->nlayers; i++) {
        pbits = pScrPriv->layer[i].u.init.pbits;
        width = pScrPriv->layer[i].u.init.width;
        depth = pScrPriv->layer[i].u.init.depth;
        pPixmap = (*pScreen->CreatePixmap) (pScreen, 0, 0, depth, 0);
        if (!pPixmap)
            return FALSE;
        if (!(*pScreen->ModifyPixmapHeader) (pPixmap, pScreen->width,
                                             pScreen->height, depth,
                                             BitsPerPixel(depth),
                                             PixmapBytePad(width, depth),
                                             pbits))
            return FALSE;
        pScrPriv->layer[i].u.run.pixmap = pPixmap;
        RegionInit(&pScrPriv->layer[i].u.run.region, &box, 0);
    }
    pScreen->devPrivate = pScrPriv->layer[0].u.run.pixmap;
    return TRUE;
}

void
fbOverlayPaintKey(DrawablePtr pDrawable,
                  RegionPtr pRegion, CARD32 pixel, int layer)
{
    fbFillRegionSolid(pDrawable, pRegion, 0,
                      fbReplicatePixel(pixel, pDrawable->bitsPerPixel));
}

/*
 * Track visible region for each layer
 */
void
fbOverlayUpdateLayerRegion(ScreenPtr pScreen, int layer, RegionPtr prgn)
{
    FbOverlayScrPrivPtr pScrPriv = fbOverlayGetScrPriv(pScreen);
    int i;
    RegionRec rgnNew;

    if (!prgn || !RegionNotEmpty(prgn))
        return;
    for (i = 0; i < pScrPriv->nlayers; i++) {
        if (i == layer) {
            /* add new piece to this fb */
            RegionUnion(&pScrPriv->layer[i].u.run.region,
                        &pScrPriv->layer[i].u.run.region, prgn);
        }
        else if (RegionNotEmpty(&pScrPriv->layer[i].u.run.region)) {
            /* paint new piece with chroma key */
            RegionNull(&rgnNew);
            RegionIntersect(&rgnNew, prgn, &pScrPriv->layer[i].u.run.region);
            (*pScrPriv->PaintKey) (&pScrPriv->layer[i].u.run.pixmap->drawable,
                                   &rgnNew, pScrPriv->layer[i].key, i);
            RegionUninit(&rgnNew);
            /* remove piece from other fbs */
            RegionSubtract(&pScrPriv->layer[i].u.run.region,
                           &pScrPriv->layer[i].u.run.region, prgn);
        }
    }
}

/*
 * Copy only areas in each layer containing real bits
 */
void
fbOverlayCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    FbOverlayScrPrivPtr pScrPriv = fbOverlayGetScrPriv(pScreen);
    RegionRec rgnDst;
    int dx, dy;
    int i;
    RegionRec layerRgn[FB_OVERLAY_MAX];
    PixmapPtr pPixmap;

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;

    /*
     * Clip to existing bits
     */
    RegionTranslate(prgnSrc, -dx, -dy);
    RegionNull(&rgnDst);
    RegionIntersect(&rgnDst, &pWin->borderClip, prgnSrc);
    RegionTranslate(&rgnDst, dx, dy);
    /*
     * Compute the portion of each fb affected by this copy
     */
    for (i = 0; i < pScrPriv->nlayers; i++) {
        RegionNull(&layerRgn[i]);
        RegionIntersect(&layerRgn[i], &rgnDst,
                        &pScrPriv->layer[i].u.run.region);
        if (RegionNotEmpty(&layerRgn[i])) {
            RegionTranslate(&layerRgn[i], -dx, -dy);
            pPixmap = pScrPriv->layer[i].u.run.pixmap;
            miCopyRegion(&pPixmap->drawable, &pPixmap->drawable,
                         0,
                         &layerRgn[i], dx, dy, pScrPriv->CopyWindow, 0,
                         (void *) (long) i);
        }
    }
    /*
     * Update regions
     */
    for (i = 0; i < pScrPriv->nlayers; i++) {
        if (RegionNotEmpty(&layerRgn[i]))
            fbOverlayUpdateLayerRegion(pScreen, i, &layerRgn[i]);

        RegionUninit(&layerRgn[i]);
    }
    RegionUninit(&rgnDst);
}

void
fbOverlayWindowExposures(WindowPtr pWin, RegionPtr prgn)
{
    fbOverlayUpdateLayerRegion(pWin->drawable.pScreen,
                               fbOverlayWindowLayer(pWin), prgn);
    miWindowExposures(pWin, prgn);
}

Bool
fbOverlaySetupScreen(ScreenPtr pScreen,
                     void *pbits1,
                     void *pbits2,
                     int xsize,
                     int ysize,
                     int dpix,
                     int dpiy, int width1, int width2, int bpp1, int bpp2)
{
    return fbSetupScreen(pScreen,
                         pbits1, xsize, ysize, dpix, dpiy, width1, bpp1);
}

Bool
fbOverlayFinishScreenInit(ScreenPtr pScreen,
                          void *pbits1,
                          void *pbits2,
                          int xsize,
                          int ysize,
                          int dpix,
                          int dpiy,
                          int width1,
                          int width2,
                          int bpp1, int bpp2, int depth1, int depth2)
{
    VisualPtr visuals;
    DepthPtr depths;
    int nvisuals;
    int ndepths;
    VisualID defaultVisual;
    FbOverlayScrPrivPtr pScrPriv;

    if (!dixRegisterPrivateKey
        (&fbOverlayScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (bpp1 == 24 || bpp2 == 24)
        return FALSE;

    pScrPriv = malloc(sizeof(FbOverlayScrPrivRec));
    if (!pScrPriv)
        return FALSE;

    if (!fbInitVisuals(&visuals, &depths, &nvisuals, &ndepths, &depth1,
                       &defaultVisual, ((unsigned long) 1 << (bpp1 - 1)) |
                       ((unsigned long) 1 << (bpp2 - 1)), 8)) {
        free(pScrPriv);
        return FALSE;
    }
    if (!miScreenInit(pScreen, 0, xsize, ysize, dpix, dpiy, 0,
                      depth1, ndepths, depths,
                      defaultVisual, nvisuals, visuals)) {
        free(pScrPriv);
        return FALSE;
    }
    /* MI thinks there's no frame buffer */
#ifdef MITSHM
    ShmRegisterFbFuncs(pScreen);
#endif
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 2;

    pScrPriv->nlayers = 2;
    pScrPriv->PaintKey = fbOverlayPaintKey;
    pScrPriv->CopyWindow = fbCopyWindowProc;
    pScrPriv->layer[0].u.init.pbits = pbits1;
    pScrPriv->layer[0].u.init.width = width1;
    pScrPriv->layer[0].u.init.depth = depth1;

    pScrPriv->layer[1].u.init.pbits = pbits2;
    pScrPriv->layer[1].u.init.width = width2;
    pScrPriv->layer[1].u.init.depth = depth2;
    dixSetPrivate(&pScreen->devPrivates, fbOverlayScreenPrivateKey, pScrPriv);

    /* overwrite miCloseScreen with our own */
    pScreen->CloseScreen = fbOverlayCloseScreen;
    pScreen->CreateScreenResources = fbOverlayCreateScreenResources;
    pScreen->CreateWindow = fbOverlayCreateWindow;
    pScreen->WindowExposures = fbOverlayWindowExposures;
    pScreen->CopyWindow = fbOverlayCopyWindow;

    return TRUE;
}
