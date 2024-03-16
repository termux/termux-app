/*
 * Copyright (c) 1998-2001 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#if defined(_XOPEN_SOURCE) || defined(__sun) && defined(__SVR4)
#include <math.h>
#else
#define _XOPEN_SOURCE           /* to get prototype for pow on some systems */
#include <math.h>
#undef _XOPEN_SOURCE
#endif

#include <X11/X.h>
#include "misc.h"
#include <X11/Xproto.h>
#include "colormapst.h"
#include "scrnintstr.h"

#include "resource.h"

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86str.h"
#include "micmap.h"
#include "xf86RandR12.h"
#include "xf86Crtc.h"

#ifdef XFreeXDGA
#include <X11/extensions/xf86dgaproto.h>
#include "dgaproc.h"
#endif

#include "xf86cmap.h"

#define SCREEN_PROLOGUE(pScreen, field) ((pScreen)->field = \
    ((CMapScreenPtr)dixLookupPrivate(&(pScreen)->devPrivates, CMapScreenKey))->field)
#define SCREEN_EPILOGUE(pScreen, field, wrapper)\
    ((pScreen)->field = wrapper)

#define LOAD_PALETTE(pmap) \
    ((pmap == GetInstalledmiColormap(pmap->pScreen)) && \
     ((pScreenPriv->flags & CMAP_LOAD_EVEN_IF_OFFSCREEN) || \
      xf86ScreenToScrn(pmap->pScreen)->vtSema || pScreenPriv->isDGAmode))

typedef struct _CMapLink {
    ColormapPtr cmap;
    struct _CMapLink *next;
} CMapLink, *CMapLinkPtr;

typedef struct {
    CloseScreenProcPtr CloseScreen;
    CreateColormapProcPtr CreateColormap;
    DestroyColormapProcPtr DestroyColormap;
    InstallColormapProcPtr InstallColormap;
    StoreColorsProcPtr StoreColors;
    Bool (*EnterVT) (ScrnInfoPtr);
    Bool (*SwitchMode) (ScrnInfoPtr, DisplayModePtr);
    int (*SetDGAMode) (ScrnInfoPtr, int, DGADevicePtr);
    xf86ChangeGammaProc *ChangeGamma;
    int maxColors;
    int sigRGBbits;
    int gammaElements;
    LOCO *gamma;
    int *PreAllocIndices;
    CMapLinkPtr maps;
    unsigned int flags;
    Bool isDGAmode;
} CMapScreenRec, *CMapScreenPtr;

typedef struct {
    int numColors;
    LOCO *colors;
    Bool recalculate;
    int overscan;
} CMapColormapRec, *CMapColormapPtr;

static DevPrivateKeyRec CMapScreenKeyRec;

#define CMapScreenKeyRegistered dixPrivateKeyRegistered(&CMapScreenKeyRec)
#define CMapScreenKey (&CMapScreenKeyRec)
static DevPrivateKeyRec CMapColormapKeyRec;

#define CMapColormapKey (&CMapColormapKeyRec)

static void CMapInstallColormap(ColormapPtr);
static void CMapStoreColors(ColormapPtr, int, xColorItem *);
static Bool CMapCloseScreen(ScreenPtr);
static Bool CMapCreateColormap(ColormapPtr);
static void CMapDestroyColormap(ColormapPtr);

static Bool CMapEnterVT(ScrnInfoPtr);
static Bool CMapSwitchMode(ScrnInfoPtr, DisplayModePtr);

#ifdef XFreeXDGA
static int CMapSetDGAMode(ScrnInfoPtr, int, DGADevicePtr);
#endif
static int CMapChangeGamma(ScrnInfoPtr, Gamma);

static void ComputeGamma(ScrnInfoPtr, CMapScreenPtr);
static Bool CMapAllocateColormapPrivate(ColormapPtr);
static void CMapRefreshColors(ColormapPtr, int, int *);
static void CMapSetOverscan(ColormapPtr, int, int *);
static void CMapReinstallMap(ColormapPtr);
static void CMapUnwrapScreen(ScreenPtr pScreen);

Bool
xf86ColormapAllocatePrivates(ScrnInfoPtr pScrn)
{
    if (!dixRegisterPrivateKey(&CMapScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (!dixRegisterPrivateKey(&CMapColormapKeyRec, PRIVATE_COLORMAP, 0))
        return FALSE;
    return TRUE;
}

Bool
xf86HandleColormaps(ScreenPtr pScreen,
                    int maxColors,
                    int sigRGBbits,
                    xf86LoadPaletteProc * loadPalette,
                    xf86SetOverscanProc * setOverscan, unsigned int flags)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    ColormapPtr pDefMap = NULL;
    CMapScreenPtr pScreenPriv;
    LOCO *gamma;
    int *indices;
    int elements;

    if (!maxColors || !sigRGBbits ||
        (!loadPalette && !xf86_crtc_supports_gamma(pScrn)))
        return FALSE;

    elements = 1 << sigRGBbits;

    if (!(gamma = xallocarray(elements, sizeof(LOCO))))
        return FALSE;

    if (!(indices = xallocarray(maxColors, sizeof(int)))) {
        free(gamma);
        return FALSE;
    }

    if (!(pScreenPriv = malloc(sizeof(CMapScreenRec)))) {
        free(gamma);
        free(indices);
        return FALSE;
    }

    dixSetPrivate(&pScreen->devPrivates, &CMapScreenKeyRec, pScreenPriv);

    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreenPriv->CreateColormap = pScreen->CreateColormap;
    pScreenPriv->DestroyColormap = pScreen->DestroyColormap;
    pScreenPriv->InstallColormap = pScreen->InstallColormap;
    pScreenPriv->StoreColors = pScreen->StoreColors;
    pScreen->CloseScreen = CMapCloseScreen;
    pScreen->CreateColormap = CMapCreateColormap;
    pScreen->DestroyColormap = CMapDestroyColormap;
    pScreen->InstallColormap = CMapInstallColormap;
    pScreen->StoreColors = CMapStoreColors;

    pScrn->LoadPalette = loadPalette;
    pScrn->SetOverscan = setOverscan;
    pScreenPriv->maxColors = maxColors;
    pScreenPriv->sigRGBbits = sigRGBbits;
    pScreenPriv->gammaElements = elements;
    pScreenPriv->gamma = gamma;
    pScreenPriv->PreAllocIndices = indices;
    pScreenPriv->maps = NULL;
    pScreenPriv->flags = flags;
    pScreenPriv->isDGAmode = FALSE;

    pScreenPriv->EnterVT = pScrn->EnterVT;
    pScreenPriv->SwitchMode = pScrn->SwitchMode;
    pScreenPriv->SetDGAMode = pScrn->SetDGAMode;
    pScreenPriv->ChangeGamma = pScrn->ChangeGamma;

    if (!(flags & CMAP_LOAD_EVEN_IF_OFFSCREEN)) {
        pScrn->EnterVT = CMapEnterVT;
        if ((flags & CMAP_RELOAD_ON_MODE_SWITCH) && pScrn->SwitchMode)
            pScrn->SwitchMode = CMapSwitchMode;
    }
#ifdef XFreeXDGA
    pScrn->SetDGAMode = CMapSetDGAMode;
#endif
    pScrn->ChangeGamma = CMapChangeGamma;

    ComputeGamma(pScrn, pScreenPriv);

    /* get the default map */
    dixLookupResourceByType((void **) &pDefMap, pScreen->defColormap,
                            RT_COLORMAP, serverClient, DixInstallAccess);

    if (!CMapAllocateColormapPrivate(pDefMap)) {
        CMapUnwrapScreen(pScreen);
        return FALSE;
    }

    if (xf86_crtc_supports_gamma(pScrn)) {
        pScrn->LoadPalette = xf86RandR12LoadPalette;

        if (!xf86RandR12InitGamma(pScrn, elements)) {
            CMapUnwrapScreen(pScreen);
            return FALSE;
        }
    }

    /* Force the initial map to be loaded */
    SetInstalledmiColormap(pScreen, NULL);
    CMapInstallColormap(pDefMap);
    return TRUE;
}

/**** Screen functions ****/

static Bool
CMapCloseScreen(ScreenPtr pScreen)
{
    CMapUnwrapScreen(pScreen);

    return (*pScreen->CloseScreen) (pScreen);
}

static Bool
CMapColormapUseMax(VisualPtr pVisual, CMapScreenPtr pScreenPriv)
{
    if (pVisual->nplanes > 16)
        return TRUE;
    return ((1 << pVisual->nplanes) > pScreenPriv->maxColors);
}

static Bool
CMapAllocateColormapPrivate(ColormapPtr pmap)
{
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pmap->pScreen->devPrivates,
                                         CMapScreenKey);
    CMapColormapPtr pColPriv;
    CMapLinkPtr pLink;
    int numColors;
    LOCO *colors;

    if (CMapColormapUseMax(pmap->pVisual, pScreenPriv))
        numColors = pmap->pVisual->ColormapEntries;
    else
        numColors = 1 << pmap->pVisual->nplanes;

    if (!(colors = xallocarray(numColors, sizeof(LOCO))))
        return FALSE;

    if (!(pColPriv = malloc(sizeof(CMapColormapRec)))) {
        free(colors);
        return FALSE;
    }

    dixSetPrivate(&pmap->devPrivates, CMapColormapKey, pColPriv);

    pColPriv->numColors = numColors;
    pColPriv->colors = colors;
    pColPriv->recalculate = TRUE;
    pColPriv->overscan = -1;

    /* add map to list */
    pLink = malloc(sizeof(CMapLink));
    if (pLink) {
        pLink->cmap = pmap;
        pLink->next = pScreenPriv->maps;
        pScreenPriv->maps = pLink;
    }

    return TRUE;
}

static Bool
CMapCreateColormap(ColormapPtr pmap)
{
    ScreenPtr pScreen = pmap->pScreen;
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates, CMapScreenKey);
    Bool ret = FALSE;

    pScreen->CreateColormap = pScreenPriv->CreateColormap;
    if ((*pScreen->CreateColormap) (pmap)) {
        if (CMapAllocateColormapPrivate(pmap))
            ret = TRUE;
    }
    pScreen->CreateColormap = CMapCreateColormap;

    return ret;
}

static void
CMapDestroyColormap(ColormapPtr cmap)
{
    ScreenPtr pScreen = cmap->pScreen;
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates, CMapScreenKey);
    CMapColormapPtr pColPriv =
        (CMapColormapPtr) dixLookupPrivate(&cmap->devPrivates, CMapColormapKey);
    CMapLinkPtr prevLink = NULL, pLink = pScreenPriv->maps;

    if (pColPriv) {
        free(pColPriv->colors);
        free(pColPriv);
    }

    /* remove map from list */
    while (pLink) {
        if (pLink->cmap == cmap) {
            if (prevLink)
                prevLink->next = pLink->next;
            else
                pScreenPriv->maps = pLink->next;
            free(pLink);
            break;
        }
        prevLink = pLink;
        pLink = pLink->next;
    }

    if (pScreenPriv->DestroyColormap) {
        pScreen->DestroyColormap = pScreenPriv->DestroyColormap;
        (*pScreen->DestroyColormap) (cmap);
        pScreen->DestroyColormap = CMapDestroyColormap;
    }
}

static void
CMapStoreColors(ColormapPtr pmap, int ndef, xColorItem * pdefs)
{
    ScreenPtr pScreen = pmap->pScreen;
    VisualPtr pVisual = pmap->pVisual;
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates, CMapScreenKey);
    int *indices = pScreenPriv->PreAllocIndices;
    int num = ndef;

    /* At the moment this isn't necessary since there's nobody below us */
    pScreen->StoreColors = pScreenPriv->StoreColors;
    (*pScreen->StoreColors) (pmap, ndef, pdefs);
    pScreen->StoreColors = CMapStoreColors;

    /* should never get here for these */
    if ((pVisual->class == TrueColor) ||
        (pVisual->class == StaticColor) || (pVisual->class == StaticGray))
        return;

    if (pVisual->class == DirectColor) {
        CMapColormapPtr pColPriv =
            (CMapColormapPtr) dixLookupPrivate(&pmap->devPrivates,
                                               CMapColormapKey);
        int i;

        if (CMapColormapUseMax(pVisual, pScreenPriv)) {
            int index;

            num = 0;
            while (ndef--) {
                if (pdefs[ndef].flags & DoRed) {
                    index = (pdefs[ndef].pixel & pVisual->redMask) >>
                        pVisual->offsetRed;
                    i = num;
                    while (i--)
                        if (indices[i] == index)
                            break;
                    if (i == -1)
                        indices[num++] = index;
                }
                if (pdefs[ndef].flags & DoGreen) {
                    index = (pdefs[ndef].pixel & pVisual->greenMask) >>
                        pVisual->offsetGreen;
                    i = num;
                    while (i--)
                        if (indices[i] == index)
                            break;
                    if (i == -1)
                        indices[num++] = index;
                }
                if (pdefs[ndef].flags & DoBlue) {
                    index = (pdefs[ndef].pixel & pVisual->blueMask) >>
                        pVisual->offsetBlue;
                    i = num;
                    while (i--)
                        if (indices[i] == index)
                            break;
                    if (i == -1)
                        indices[num++] = index;
                }
            }

        }
        else {
            /* not really as overkill as it seems */
            num = pColPriv->numColors;
            for (i = 0; i < pColPriv->numColors; i++)
                indices[i] = i;
        }
    }
    else {
        while (ndef--)
            indices[ndef] = pdefs[ndef].pixel;
    }

    CMapRefreshColors(pmap, num, indices);
}

static void
CMapInstallColormap(ColormapPtr pmap)
{
    ScreenPtr pScreen = pmap->pScreen;
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates, CMapScreenKey);

    if (pmap == GetInstalledmiColormap(pmap->pScreen))
        return;

    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    (*pScreen->InstallColormap) (pmap);
    pScreen->InstallColormap = CMapInstallColormap;

    /* Important. We let the lower layers, namely DGA,
       overwrite the choice of Colormap to install */
    if (GetInstalledmiColormap(pmap->pScreen))
        pmap = GetInstalledmiColormap(pmap->pScreen);

    if (!(pScreenPriv->flags & CMAP_PALETTED_TRUECOLOR) &&
        (pmap->pVisual->class == TrueColor) &&
        CMapColormapUseMax(pmap->pVisual, pScreenPriv))
        return;

    if (LOAD_PALETTE(pmap))
        CMapReinstallMap(pmap);
}

/**** ScrnInfoRec functions ****/

static Bool
CMapEnterVT(ScrnInfoPtr pScrn)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    Bool ret;
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates, CMapScreenKey);

    pScrn->EnterVT = pScreenPriv->EnterVT;
    ret = (*pScreenPriv->EnterVT) (pScrn);
    pScreenPriv->EnterVT = pScrn->EnterVT;
    pScrn->EnterVT = CMapEnterVT;
    if (ret) {
        if (GetInstalledmiColormap(pScreen))
            CMapReinstallMap(GetInstalledmiColormap(pScreen));
        return TRUE;
    }
    return FALSE;
}

static Bool
CMapSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates, CMapScreenKey);

    if ((*pScreenPriv->SwitchMode) (pScrn, mode)) {
        if (GetInstalledmiColormap(pScreen))
            CMapReinstallMap(GetInstalledmiColormap(pScreen));
        return TRUE;
    }
    return FALSE;
}

#ifdef XFreeXDGA
static int
CMapSetDGAMode(ScrnInfoPtr pScrn, int num, DGADevicePtr dev)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates, CMapScreenKey);
    int ret;

    ret = (*pScreenPriv->SetDGAMode) (pScrn, num, dev);

    pScreenPriv->isDGAmode = DGAActive(pScrn->scrnIndex);

    if (!pScreenPriv->isDGAmode && GetInstalledmiColormap(pScreen)
        && xf86ScreenToScrn(pScreen)->vtSema)
        CMapReinstallMap(GetInstalledmiColormap(pScreen));

    return ret;
}
#endif

/**** Utilities ****/

static void
CMapReinstallMap(ColormapPtr pmap)
{
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pmap->pScreen->devPrivates,
                                         CMapScreenKey);
    CMapColormapPtr cmapPriv =
        (CMapColormapPtr) dixLookupPrivate(&pmap->devPrivates, CMapColormapKey);
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pmap->pScreen);
    int i = cmapPriv->numColors;
    int *indices = pScreenPriv->PreAllocIndices;

    while (i--)
        indices[i] = i;

    if (cmapPriv->recalculate)
        CMapRefreshColors(pmap, cmapPriv->numColors, indices);
    else {
        (*pScrn->LoadPalette) (pScrn, cmapPriv->numColors,
                               indices, cmapPriv->colors, pmap->pVisual);
        if (pScrn->SetOverscan) {
#ifdef DEBUGOVERSCAN
            ErrorF("SetOverscan() called from CMapReinstallMap\n");
#endif
            pScrn->SetOverscan(pScrn, cmapPriv->overscan);
        }
    }

    cmapPriv->recalculate = FALSE;
}

static void
CMapRefreshColors(ColormapPtr pmap, int defs, int *indices)
{
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pmap->pScreen->devPrivates,
                                         CMapScreenKey);
    CMapColormapPtr pColPriv =
        (CMapColormapPtr) dixLookupPrivate(&pmap->devPrivates, CMapColormapKey);
    VisualPtr pVisual = pmap->pVisual;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pmap->pScreen);
    int numColors, i;
    LOCO *gamma, *colors;
    EntryPtr entry;
    int reds, greens, blues, maxValue, index, shift;

    numColors = pColPriv->numColors;
    shift = 16 - pScreenPriv->sigRGBbits;
    maxValue = (1 << pScreenPriv->sigRGBbits) - 1;
    gamma = pScreenPriv->gamma;
    colors = pColPriv->colors;

    reds = pVisual->redMask >> pVisual->offsetRed;
    greens = pVisual->greenMask >> pVisual->offsetGreen;
    blues = pVisual->blueMask >> pVisual->offsetBlue;

    switch (pVisual->class) {
    case StaticGray:
        for (i = 0; i < numColors; i++) {
            index = (i + 1) * maxValue / numColors;
            colors[i].red = gamma[index].red;
            colors[i].green = gamma[index].green;
            colors[i].blue = gamma[index].blue;
        }
        break;
    case TrueColor:
        if (CMapColormapUseMax(pVisual, pScreenPriv)) {
            for (i = 0; i <= reds; i++)
                colors[i].red = gamma[i * maxValue / reds].red;
            for (i = 0; i <= greens; i++)
                colors[i].green = gamma[i * maxValue / greens].green;
            for (i = 0; i <= blues; i++)
                colors[i].blue = gamma[i * maxValue / blues].blue;
            break;
        }
        for (i = 0; i < numColors; i++) {
            colors[i].red = gamma[((i >> pVisual->offsetRed) & reds) *
                                  maxValue / reds].red;
            colors[i].green = gamma[((i >> pVisual->offsetGreen) & greens) *
                                    maxValue / greens].green;
            colors[i].blue = gamma[((i >> pVisual->offsetBlue) & blues) *
                                   maxValue / blues].blue;
        }
        break;
    case StaticColor:
    case PseudoColor:
    case GrayScale:
        for (i = 0; i < defs; i++) {
            index = indices[i];
            entry = (EntryPtr) &pmap->red[index];

            if (entry->fShared) {
                colors[index].red =
                    gamma[entry->co.shco.red->color >> shift].red;
                colors[index].green =
                    gamma[entry->co.shco.green->color >> shift].green;
                colors[index].blue =
                    gamma[entry->co.shco.blue->color >> shift].blue;
            }
            else {
                colors[index].red = gamma[entry->co.local.red >> shift].red;
                colors[index].green =
                    gamma[entry->co.local.green >> shift].green;
                colors[index].blue = gamma[entry->co.local.blue >> shift].blue;
            }
        }
        break;
    case DirectColor:
        if (CMapColormapUseMax(pVisual, pScreenPriv)) {
            for (i = 0; i < defs; i++) {
                index = indices[i];
                if (index <= reds)
                    colors[index].red =
                        gamma[pmap->red[index].co.local.red >> shift].red;
                if (index <= greens)
                    colors[index].green =
                        gamma[pmap->green[index].co.local.green >> shift].green;
                if (index <= blues)
                    colors[index].blue =
                        gamma[pmap->blue[index].co.local.blue >> shift].blue;

            }
            break;
        }
        for (i = 0; i < defs; i++) {
            index = indices[i];

            colors[index].red = gamma[pmap->red[(index >> pVisual->
                                                 offsetRed) & reds].co.local.
                                      red >> shift].red;
            colors[index].green =
                gamma[pmap->green[(index >> pVisual->offsetGreen) & greens].co.
                      local.green >> shift].green;
            colors[index].blue =
                gamma[pmap->blue[(index >> pVisual->offsetBlue) & blues].co.
                      local.blue >> shift].blue;
        }
        break;
    }

    if (LOAD_PALETTE(pmap))
        (*pScrn->LoadPalette) (pScrn, defs, indices, colors, pmap->pVisual);

    if (pScrn->SetOverscan)
        CMapSetOverscan(pmap, defs, indices);

}

static Bool
CMapCompareColors(LOCO * color1, LOCO * color2)
{
    /* return TRUE if the color1 is "closer" to black than color2 */
#ifdef DEBUGOVERSCAN
    ErrorF("#%02x%02x%02x vs #%02x%02x%02x (%d vs %d)\n",
           color1->red, color1->green, color1->blue,
           color2->red, color2->green, color2->blue,
           color1->red + color1->green + color1->blue,
           color2->red + color2->green + color2->blue);
#endif
    return (color1->red + color1->green + color1->blue <
            color2->red + color2->green + color2->blue);
}

static void
CMapSetOverscan(ColormapPtr pmap, int defs, int *indices)
{
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pmap->pScreen->devPrivates,
                                         CMapScreenKey);
    CMapColormapPtr pColPriv =
        (CMapColormapPtr) dixLookupPrivate(&pmap->devPrivates, CMapColormapKey);
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pmap->pScreen);
    VisualPtr pVisual = pmap->pVisual;
    int i;
    LOCO *colors;
    int index;
    Bool newOverscan = FALSE;
    int overscan, tmpOverscan;

    colors = pColPriv->colors;
    overscan = pColPriv->overscan;

    /*
     * Search for a new overscan index in the following cases:
     *
     *   - The index hasn't yet been initialised.  In this case search
     *     for an index that is black or a close match to black.
     *
     *   - The colour of the old index is changed.  In this case search
     *     all indices for a black or close match to black.
     *
     *   - The colour of the old index wasn't black.  In this case only
     *     search the indices that were changed for a better match to black.
     */

    switch (pVisual->class) {
    case StaticGray:
    case TrueColor:
        /* Should only come here once.  Initialise the overscan index to 0 */
        overscan = 0;
        newOverscan = TRUE;
        break;
    case StaticColor:
        /*
         * Only come here once, but search for the overscan in the same way
         * as for the other cases.
         */
    case DirectColor:
    case PseudoColor:
    case GrayScale:
        if (overscan < 0 || overscan > pScreenPriv->maxColors - 1) {
            /* Uninitialised */
            newOverscan = TRUE;
        }
        else {
            /* Check if the overscan was changed */
            for (i = 0; i < defs; i++) {
                index = indices[i];
                if (index == overscan) {
                    newOverscan = TRUE;
                    break;
                }
            }
        }
        if (newOverscan) {
            /* The overscan is either uninitialised or it has been changed */

            if (overscan < 0 || overscan > pScreenPriv->maxColors - 1)
                tmpOverscan = pScreenPriv->maxColors - 1;
            else
                tmpOverscan = overscan;

            /* search all entries for a close match to black */
            for (i = pScreenPriv->maxColors - 1; i >= 0; i--) {
                if (colors[i].red == 0 && colors[i].green == 0 &&
                    colors[i].blue == 0) {
                    overscan = i;
#ifdef DEBUGOVERSCAN
                    ErrorF("Black found at index 0x%02x\n", i);
#endif
                    break;
                }
                else {
#ifdef DEBUGOVERSCAN
                    ErrorF("0x%02x: ", i);
#endif
                    if (CMapCompareColors(&colors[i], &colors[tmpOverscan])) {
                        tmpOverscan = i;
#ifdef DEBUGOVERSCAN
                        ErrorF("possible \"Black\" at index 0x%02x\n", i);
#endif
                    }
                }
            }
            if (i < 0)
                overscan = tmpOverscan;
        }
        else {
            /* Check of the old overscan wasn't black */
            if (colors[overscan].red != 0 || colors[overscan].green != 0 ||
                colors[overscan].blue != 0) {
                int oldOverscan = tmpOverscan = overscan;

                /* See of there is now a better match */
                for (i = 0; i < defs; i++) {
                    index = indices[i];
                    if (colors[index].red == 0 && colors[index].green == 0 &&
                        colors[index].blue == 0) {
                        overscan = index;
#ifdef DEBUGOVERSCAN
                        ErrorF("Black found at index 0x%02x\n", index);
#endif
                        break;
                    }
                    else {
#ifdef DEBUGOVERSCAN
                        ErrorF("0x%02x: ", index);
#endif
                        if (CMapCompareColors(&colors[index],
                                              &colors[tmpOverscan])) {
                            tmpOverscan = index;
#ifdef DEBUGOVERSCAN
                            ErrorF("possible \"Black\" at index 0x%02x\n",
                                   index);
#endif
                        }
                    }
                }
                if (i == defs)
                    overscan = tmpOverscan;
                if (overscan != oldOverscan)
                    newOverscan = TRUE;
            }
        }
        break;
    }
    if (newOverscan) {
        pColPriv->overscan = overscan;
        if (LOAD_PALETTE(pmap)) {
#ifdef DEBUGOVERSCAN
            ErrorF("SetOverscan() called from CmapSetOverscan\n");
#endif
            pScrn->SetOverscan(pScrn, overscan);
        }
    }
}

static void
CMapUnwrapScreen(ScreenPtr pScreen)
{
    CMapScreenPtr pScreenPriv =
        (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates, CMapScreenKey);
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->CreateColormap = pScreenPriv->CreateColormap;
    pScreen->DestroyColormap = pScreenPriv->DestroyColormap;
    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    pScreen->StoreColors = pScreenPriv->StoreColors;

    pScrn->EnterVT = pScreenPriv->EnterVT;
    pScrn->SwitchMode = pScreenPriv->SwitchMode;
    pScrn->SetDGAMode = pScreenPriv->SetDGAMode;
    pScrn->ChangeGamma = pScreenPriv->ChangeGamma;

    free(pScreenPriv->gamma);
    free(pScreenPriv->PreAllocIndices);
    free(pScreenPriv);
}

static void
ComputeGamma(ScrnInfoPtr pScrn, CMapScreenPtr priv)
{
    int elements = priv->gammaElements - 1;
    double RedGamma, GreenGamma, BlueGamma;
    int i;

#ifndef DONT_CHECK_GAMMA
    /* This check is to catch drivers that are not initialising pScrn->gamma */
    if (pScrn->gamma.red < GAMMA_MIN || pScrn->gamma.red > GAMMA_MAX ||
        pScrn->gamma.green < GAMMA_MIN || pScrn->gamma.green > GAMMA_MAX ||
        pScrn->gamma.blue < GAMMA_MIN || pScrn->gamma.blue > GAMMA_MAX) {

        xf86DrvMsgVerb(pScrn->scrnIndex, X_WARNING, 0,
                       "The %s driver didn't call xf86SetGamma() to initialise\n"
                       "\tthe gamma values.\n", pScrn->driverName);
        xf86DrvMsgVerb(pScrn->scrnIndex, X_WARNING, 0,
                       "PLEASE FIX THE `%s' DRIVER!\n",
                       pScrn->driverName);
        pScrn->gamma.red = 1.0;
        pScrn->gamma.green = 1.0;
        pScrn->gamma.blue = 1.0;
    }
#endif

    RedGamma = 1.0 / (double) pScrn->gamma.red;
    GreenGamma = 1.0 / (double) pScrn->gamma.green;
    BlueGamma = 1.0 / (double) pScrn->gamma.blue;

    for (i = 0; i <= elements; i++) {
        if (RedGamma == 1.0)
            priv->gamma[i].red = i;
        else
            priv->gamma[i].red = (CARD16) (pow((double) i / (double) elements,
                                               RedGamma) * (double) elements +
                                           0.5);

        if (GreenGamma == 1.0)
            priv->gamma[i].green = i;
        else
            priv->gamma[i].green = (CARD16) (pow((double) i / (double) elements,
                                                 GreenGamma) *
                                             (double) elements + 0.5);

        if (BlueGamma == 1.0)
            priv->gamma[i].blue = i;
        else
            priv->gamma[i].blue = (CARD16) (pow((double) i / (double) elements,
                                                BlueGamma) * (double) elements +
                                            0.5);
    }
}

int
CMapChangeGamma(ScrnInfoPtr pScrn, Gamma gamma)
{
    int ret = Success;
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    CMapColormapPtr pColPriv;
    CMapScreenPtr pScreenPriv;
    CMapLinkPtr pLink;

    /* Is this sufficient checking ? */
    if (!CMapScreenKeyRegistered)
        return BadImplementation;

    pScreenPriv = (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                   CMapScreenKey);
    if (!pScreenPriv)
        return BadImplementation;

    if (gamma.red < GAMMA_MIN || gamma.red > GAMMA_MAX ||
        gamma.green < GAMMA_MIN || gamma.green > GAMMA_MAX ||
        gamma.blue < GAMMA_MIN || gamma.blue > GAMMA_MAX)
        return BadValue;

    pScrn->gamma.red = gamma.red;
    pScrn->gamma.green = gamma.green;
    pScrn->gamma.blue = gamma.blue;

    ComputeGamma(pScrn, pScreenPriv);

    /* mark all colormaps on this screen */
    pLink = pScreenPriv->maps;
    while (pLink) {
        pColPriv = (CMapColormapPtr) dixLookupPrivate(&pLink->cmap->devPrivates,
                                                      CMapColormapKey);
        pColPriv->recalculate = TRUE;
        pLink = pLink->next;
    }

    if (GetInstalledmiColormap(pScreen) &&
        ((pScreenPriv->flags & CMAP_LOAD_EVEN_IF_OFFSCREEN) ||
         pScrn->vtSema || pScreenPriv->isDGAmode)) {
        ColormapPtr pMap = GetInstalledmiColormap(pScreen);

        if (!(pScreenPriv->flags & CMAP_PALETTED_TRUECOLOR) &&
            (pMap->pVisual->class == TrueColor) &&
            CMapColormapUseMax(pMap->pVisual, pScreenPriv)) {

            /* if the current map doesn't have a palette look
               for another map to change the gamma on. */

            pLink = pScreenPriv->maps;
            while (pLink) {
                if (pLink->cmap->pVisual->class == PseudoColor)
                    break;
                pLink = pLink->next;
            }

            if (pLink) {
                /* need to trick CMapRefreshColors() into thinking
                   this is the currently installed map */
                SetInstalledmiColormap(pScreen, pLink->cmap);
                CMapReinstallMap(pLink->cmap);
                SetInstalledmiColormap(pScreen, pMap);
            }
        }
        else
            CMapReinstallMap(pMap);
    }

    pScrn->ChangeGamma = pScreenPriv->ChangeGamma;
    if (pScrn->ChangeGamma)
        ret = pScrn->ChangeGamma(pScrn, gamma);
    pScrn->ChangeGamma = CMapChangeGamma;

    return ret;
}

static void
ComputeGammaRamp(CMapScreenPtr priv,
                 unsigned short *red,
                 unsigned short *green, unsigned short *blue)
{
    int elements = priv->gammaElements;
    LOCO *entry = priv->gamma;
    int shift = 16 - priv->sigRGBbits;

    while (elements--) {
        entry->red = *(red++) >> shift;
        entry->green = *(green++) >> shift;
        entry->blue = *(blue++) >> shift;
        entry++;
    }
}

int
xf86ChangeGammaRamp(ScreenPtr pScreen,
                    int size,
                    unsigned short *red,
                    unsigned short *green, unsigned short *blue)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    CMapColormapPtr pColPriv;
    CMapScreenPtr pScreenPriv;
    CMapLinkPtr pLink;

    if (!CMapScreenKeyRegistered)
        return BadImplementation;

    pScreenPriv = (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                   CMapScreenKey);
    if (!pScreenPriv)
        return BadImplementation;

    if (pScreenPriv->gammaElements != size)
        return BadValue;

    ComputeGammaRamp(pScreenPriv, red, green, blue);

    /* mark all colormaps on this screen */
    pLink = pScreenPriv->maps;
    while (pLink) {
        pColPriv = (CMapColormapPtr) dixLookupPrivate(&pLink->cmap->devPrivates,
                                                      CMapColormapKey);
        pColPriv->recalculate = TRUE;
        pLink = pLink->next;
    }

    if (GetInstalledmiColormap(pScreen) &&
        ((pScreenPriv->flags & CMAP_LOAD_EVEN_IF_OFFSCREEN) ||
         pScrn->vtSema || pScreenPriv->isDGAmode)) {
        ColormapPtr pMap = GetInstalledmiColormap(pScreen);

        if (!(pScreenPriv->flags & CMAP_PALETTED_TRUECOLOR) &&
            (pMap->pVisual->class == TrueColor) &&
            CMapColormapUseMax(pMap->pVisual, pScreenPriv)) {

            /* if the current map doesn't have a palette look
               for another map to change the gamma on. */

            pLink = pScreenPriv->maps;
            while (pLink) {
                if (pLink->cmap->pVisual->class == PseudoColor)
                    break;
                pLink = pLink->next;
            }

            if (pLink) {
                /* need to trick CMapRefreshColors() into thinking
                   this is the currently installed map */
                SetInstalledmiColormap(pScreen, pLink->cmap);
                CMapReinstallMap(pLink->cmap);
                SetInstalledmiColormap(pScreen, pMap);
            }
        }
        else
            CMapReinstallMap(pMap);
    }

    return Success;
}

int
xf86GetGammaRampSize(ScreenPtr pScreen)
{
    CMapScreenPtr pScreenPriv;

    if (!CMapScreenKeyRegistered)
        return 0;

    pScreenPriv = (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                   CMapScreenKey);
    if (!pScreenPriv)
        return 0;

    return pScreenPriv->gammaElements;
}

int
xf86GetGammaRamp(ScreenPtr pScreen,
                 int size,
                 unsigned short *red,
                 unsigned short *green, unsigned short *blue)
{
    CMapScreenPtr pScreenPriv;
    LOCO *entry;
    int shift, sigbits;

    if (!CMapScreenKeyRegistered)
        return BadImplementation;

    pScreenPriv = (CMapScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                   CMapScreenKey);
    if (!pScreenPriv)
        return BadImplementation;

    if (size > pScreenPriv->gammaElements)
        return BadValue;

    entry = pScreenPriv->gamma;
    sigbits = pScreenPriv->sigRGBbits;

    while (size--) {
        *red = entry->red << (16 - sigbits);
        *green = entry->green << (16 - sigbits);
        *blue = entry->blue << (16 - sigbits);
        shift = sigbits;
        while (shift < 16) {
            *red |= *red >> shift;
            *green |= *green >> shift;
            *blue |= *blue >> shift;
            shift += sigbits;
        }
        red++;
        green++;
        blue++;
        entry++;
    }

    return Success;
}

int
xf86ChangeGamma(ScreenPtr pScreen, Gamma gamma)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

    if (pScrn->ChangeGamma)
        return (*pScrn->ChangeGamma) (pScrn, gamma);

    return BadImplementation;
}
