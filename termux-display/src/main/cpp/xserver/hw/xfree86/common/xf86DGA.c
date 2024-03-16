/*
 * Copyright (c) 1995  Jon Tombs
 * Copyright (c) 1995, 1996, 1999  XFree86 Inc
 * Copyright (c) 1998-2002 by The XFree86 Project, Inc.
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
 *
 * Written by Mark Vojkovich
 */

/*
 * This is quite literally just two files glued together:
 * hw/xfree86/common/xf86DGA.c is the first part, and
 * hw/xfree86/dixmods/extmod/xf86dga2.c is the second part.  One day, if
 * someone actually cares about DGA, it'd be nice to clean this up.  But trust
 * me, I am not that person.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "xf86.h"
#include "xf86str.h"
#include "xf86Priv.h"
#include "dgaproc.h"
#include <X11/extensions/xf86dgaproto.h>
#include "colormapst.h"
#include "pixmapstr.h"
#include "inputstr.h"
#include "globals.h"
#include "servermd.h"
#include "micmap.h"
#include "xkbsrv.h"
#include "xf86Xinput.h"
#include "exglobals.h"
#include "exevents.h"
#include "eventstr.h"
#include "eventconvert.h"
#include "xf86Extensions.h"

#include "mi.h"

#include "misc.h"
#include "dixstruct.h"
#include "dixevents.h"
#include "extnsionst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "swaprep.h"
#include "dgaproc.h"
#include "protocol-versions.h"

#include <string.h>

#define DGA_PROTOCOL_OLD_SUPPORT 1

static DevPrivateKeyRec DGAScreenKeyRec;

#define DGAScreenKeyRegistered dixPrivateKeyRegistered(&DGAScreenKeyRec)

static Bool DGACloseScreen(ScreenPtr pScreen);
static void DGADestroyColormap(ColormapPtr pmap);
static void DGAInstallColormap(ColormapPtr pmap);
static void DGAUninstallColormap(ColormapPtr pmap);
static void DGAHandleEvent(int screen_num, InternalEvent *event,
                           DeviceIntPtr device);

static void
 DGACopyModeInfo(DGAModePtr mode, XDGAModePtr xmode);

static unsigned char DGAReqCode = 0;
static int DGAErrorBase;
static int DGAEventBase;

#define DGA_GET_SCREEN_PRIV(pScreen) ((DGAScreenPtr) \
    dixLookupPrivate(&(pScreen)->devPrivates, &DGAScreenKeyRec))

typedef struct _FakedVisualList {
    Bool free;
    VisualPtr pVisual;
    struct _FakedVisualList *next;
} FakedVisualList;

typedef struct {
    ScrnInfoPtr pScrn;
    int numModes;
    DGAModePtr modes;
    CloseScreenProcPtr CloseScreen;
    DestroyColormapProcPtr DestroyColormap;
    InstallColormapProcPtr InstallColormap;
    UninstallColormapProcPtr UninstallColormap;
    DGADevicePtr current;
    DGAFunctionPtr funcs;
    int input;
    ClientPtr client;
    int pixmapMode;
    FakedVisualList *fakedVisuals;
    ColormapPtr dgaColormap;
    ColormapPtr savedColormap;
    Bool grabMouse;
    Bool grabKeyboard;
} DGAScreenRec, *DGAScreenPtr;

Bool
DGAInit(ScreenPtr pScreen, DGAFunctionPtr funcs, DGAModePtr modes, int num)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    DGAScreenPtr pScreenPriv;
    int i;

    if (!funcs || !funcs->SetMode || !funcs->OpenFramebuffer)
        return FALSE;

    if (!modes || num <= 0)
        return FALSE;

    if (!dixRegisterPrivateKey(&DGAScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

    if (!pScreenPriv) {
        if (!(pScreenPriv = (DGAScreenPtr) malloc(sizeof(DGAScreenRec))))
            return FALSE;
        dixSetPrivate(&pScreen->devPrivates, &DGAScreenKeyRec, pScreenPriv);
        pScreenPriv->CloseScreen = pScreen->CloseScreen;
        pScreen->CloseScreen = DGACloseScreen;
        pScreenPriv->DestroyColormap = pScreen->DestroyColormap;
        pScreen->DestroyColormap = DGADestroyColormap;
        pScreenPriv->InstallColormap = pScreen->InstallColormap;
        pScreen->InstallColormap = DGAInstallColormap;
        pScreenPriv->UninstallColormap = pScreen->UninstallColormap;
        pScreen->UninstallColormap = DGAUninstallColormap;
    }

    pScreenPriv->pScrn = pScrn;
    pScreenPriv->numModes = num;
    pScreenPriv->modes = modes;
    pScreenPriv->current = NULL;

    pScreenPriv->funcs = funcs;
    pScreenPriv->input = 0;
    pScreenPriv->client = NULL;
    pScreenPriv->fakedVisuals = NULL;
    pScreenPriv->dgaColormap = NULL;
    pScreenPriv->savedColormap = NULL;
    pScreenPriv->grabMouse = FALSE;
    pScreenPriv->grabKeyboard = FALSE;

    for (i = 0; i < num; i++)
        modes[i].num = i + 1;

#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        for (i = 0; i < num; i++)
            modes[i].flags &= ~DGA_PIXMAP_AVAILABLE;
#endif

    return TRUE;
}

/* DGAReInitModes allows the driver to re-initialize
 * the DGA mode list.
 */

Bool
DGAReInitModes(ScreenPtr pScreen, DGAModePtr modes, int num)
{
    DGAScreenPtr pScreenPriv;
    int i;

    /* No DGA? Ignore call (but don't make it look like it failed) */
    if (!DGAScreenKeyRegistered)
        return TRUE;

    pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

    /* Same as above */
    if (!pScreenPriv)
        return TRUE;

    /* Can't do this while DGA is active */
    if (pScreenPriv->current)
        return FALSE;

    /* Quick sanity check */
    if (!num)
        modes = NULL;
    else if (!modes)
        num = 0;

    pScreenPriv->numModes = num;
    pScreenPriv->modes = modes;

    /* This practically disables DGA. So be it. */
    if (!num)
        return TRUE;

    for (i = 0; i < num; i++)
        modes[i].num = i + 1;

#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        for (i = 0; i < num; i++)
            modes[i].flags &= ~DGA_PIXMAP_AVAILABLE;
#endif

    return TRUE;
}

static void
FreeMarkedVisuals(ScreenPtr pScreen)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
    FakedVisualList *prev, *curr, *tmp;

    if (!pScreenPriv->fakedVisuals)
        return;

    prev = NULL;
    curr = pScreenPriv->fakedVisuals;

    while (curr) {
        if (curr->free) {
            tmp = curr;
            curr = curr->next;
            if (prev)
                prev->next = curr;
            else
                pScreenPriv->fakedVisuals = curr;
            free(tmp->pVisual);
            free(tmp);
        }
        else {
            prev = curr;
            curr = curr->next;
        }
    }
}

static Bool
DGACloseScreen(ScreenPtr pScreen)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

    mieqSetHandler(ET_DGAEvent, NULL);
    pScreenPriv->pScrn->SetDGAMode(pScreenPriv->pScrn, 0, NULL);
    FreeMarkedVisuals(pScreen);

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    pScreen->DestroyColormap = pScreenPriv->DestroyColormap;
    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    pScreen->UninstallColormap = pScreenPriv->UninstallColormap;

    free(pScreenPriv);

    return ((*pScreen->CloseScreen) (pScreen));
}

static void
DGADestroyColormap(ColormapPtr pmap)
{
    ScreenPtr pScreen = pmap->pScreen;
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
    VisualPtr pVisual = pmap->pVisual;

    if (pScreenPriv->fakedVisuals) {
        FakedVisualList *curr = pScreenPriv->fakedVisuals;

        while (curr) {
            if (curr->pVisual == pVisual) {
                /* We can't get rid of them yet since FreeColormap
                   still needs the pVisual during the cleanup */
                curr->free = TRUE;
                break;
            }
            curr = curr->next;
        }
    }

    if (pScreenPriv->DestroyColormap) {
        pScreen->DestroyColormap = pScreenPriv->DestroyColormap;
        (*pScreen->DestroyColormap) (pmap);
        pScreen->DestroyColormap = DGADestroyColormap;
    }
}

static void
DGAInstallColormap(ColormapPtr pmap)
{
    ScreenPtr pScreen = pmap->pScreen;
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

    if (pScreenPriv->current && pScreenPriv->dgaColormap) {
        if (pmap != pScreenPriv->dgaColormap) {
            pScreenPriv->savedColormap = pmap;
            pmap = pScreenPriv->dgaColormap;
        }
    }

    pScreen->InstallColormap = pScreenPriv->InstallColormap;
    (*pScreen->InstallColormap) (pmap);
    pScreen->InstallColormap = DGAInstallColormap;
}

static void
DGAUninstallColormap(ColormapPtr pmap)
{
    ScreenPtr pScreen = pmap->pScreen;
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

    if (pScreenPriv->current && pScreenPriv->dgaColormap) {
        if (pmap == pScreenPriv->dgaColormap) {
            pScreenPriv->dgaColormap = NULL;
        }
    }

    pScreen->UninstallColormap = pScreenPriv->UninstallColormap;
    (*pScreen->UninstallColormap) (pmap);
    pScreen->UninstallColormap = DGAUninstallColormap;
}

int
xf86SetDGAMode(ScrnInfoPtr pScrn, int num, DGADevicePtr devRet)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    DGAScreenPtr pScreenPriv;
    DGADevicePtr device;
    PixmapPtr pPix = NULL;
    DGAModePtr pMode = NULL;

    /* First check if DGAInit was successful on this screen */
    if (!DGAScreenKeyRegistered)
        return BadValue;
    pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
    if (!pScreenPriv)
        return BadValue;

    if (!num) {
        if (pScreenPriv->current) {
            PixmapPtr oldPix = pScreenPriv->current->pPix;

            if (oldPix) {
                if (oldPix->drawable.id)
                    FreeResource(oldPix->drawable.id, RT_NONE);
                else
                    (*pScreen->DestroyPixmap) (oldPix);
            }
            free(pScreenPriv->current);
            pScreenPriv->current = NULL;
            pScrn->vtSema = TRUE;
            (*pScreenPriv->funcs->SetMode) (pScrn, NULL);
            if (pScreenPriv->savedColormap) {
                (*pScreen->InstallColormap) (pScreenPriv->savedColormap);
                pScreenPriv->savedColormap = NULL;
            }
            pScreenPriv->dgaColormap = NULL;
            (*pScrn->EnableDisableFBAccess) (pScrn, TRUE);

            FreeMarkedVisuals(pScreen);
        }

        pScreenPriv->grabMouse = FALSE;
        pScreenPriv->grabKeyboard = FALSE;

        return Success;
    }

    if (!pScrn->vtSema && !pScreenPriv->current)        /* Really switched away */
        return BadAlloc;

    if ((num > 0) && (num <= pScreenPriv->numModes))
        pMode = &(pScreenPriv->modes[num - 1]);
    else
        return BadValue;

    if (!(device = (DGADevicePtr) malloc(sizeof(DGADeviceRec))))
        return BadAlloc;

    if (!pScreenPriv->current) {
        Bool oldVTSema = pScrn->vtSema;

        pScrn->vtSema = FALSE;  /* kludge until we rewrite VT switching */
        (*pScrn->EnableDisableFBAccess) (pScrn, FALSE);
        pScrn->vtSema = oldVTSema;
    }

    if (!(*pScreenPriv->funcs->SetMode) (pScrn, pMode)) {
        free(device);
        return BadAlloc;
    }

    pScrn->currentMode = pMode->mode;

    if (!pScreenPriv->current && !pScreenPriv->input) {
        /* if it's multihead we need to warp the cursor off of
           our screen so it doesn't get trapped  */
    }

    pScrn->vtSema = FALSE;

    if (pScreenPriv->current) {
        PixmapPtr oldPix = pScreenPriv->current->pPix;

        if (oldPix) {
            if (oldPix->drawable.id)
                FreeResource(oldPix->drawable.id, RT_NONE);
            else
                (*pScreen->DestroyPixmap) (oldPix);
        }
        free(pScreenPriv->current);
        pScreenPriv->current = NULL;
    }

    if (pMode->flags & DGA_PIXMAP_AVAILABLE) {
        if ((pPix = (*pScreen->CreatePixmap) (pScreen, 0, 0, pMode->depth, 0))) {
            (*pScreen->ModifyPixmapHeader) (pPix,
                                            pMode->pixmapWidth,
                                            pMode->pixmapHeight, pMode->depth,
                                            pMode->bitsPerPixel,
                                            pMode->bytesPerScanline,
                                            (void *) (pMode->address));
        }
    }

    devRet->mode = device->mode = pMode;
    devRet->pPix = device->pPix = pPix;
    pScreenPriv->current = device;
    pScreenPriv->pixmapMode = FALSE;
    pScreenPriv->grabMouse = TRUE;
    pScreenPriv->grabKeyboard = TRUE;

    mieqSetHandler(ET_DGAEvent, DGAHandleEvent);

    return Success;
}

/*********** exported ones ***************/

static void
DGASetInputMode(int index, Bool keyboard, Bool mouse)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

    if (pScreenPriv) {
        pScreenPriv->grabMouse = mouse;
        pScreenPriv->grabKeyboard = keyboard;

        mieqSetHandler(ET_DGAEvent, DGAHandleEvent);
    }
}

static Bool
DGAChangePixmapMode(int index, int *x, int *y, int mode)
{
    DGAScreenPtr pScreenPriv;
    DGADevicePtr pDev;
    DGAModePtr pMode;
    PixmapPtr pPix;

    if (!DGAScreenKeyRegistered)
        return FALSE;

    pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    if (!pScreenPriv || !pScreenPriv->current || !pScreenPriv->current->pPix)
        return FALSE;

    pDev = pScreenPriv->current;
    pPix = pDev->pPix;
    pMode = pDev->mode;

    if (mode) {
        int shift = 2;

        if (*x > (pMode->pixmapWidth - pMode->viewportWidth))
            *x = pMode->pixmapWidth - pMode->viewportWidth;
        if (*y > (pMode->pixmapHeight - pMode->viewportHeight))
            *y = pMode->pixmapHeight - pMode->viewportHeight;

        switch (xf86Screens[index]->bitsPerPixel) {
        case 16:
            shift = 1;
            break;
        case 32:
            shift = 0;
            break;
        default:
            break;
        }

        if (BITMAP_SCANLINE_PAD == 64)
            shift++;

        *x = (*x >> shift) << shift;

        pPix->drawable.x = *x;
        pPix->drawable.y = *y;
        pPix->drawable.width = pMode->viewportWidth;
        pPix->drawable.height = pMode->viewportHeight;
    }
    else {
        pPix->drawable.x = 0;
        pPix->drawable.y = 0;
        pPix->drawable.width = pMode->pixmapWidth;
        pPix->drawable.height = pMode->pixmapHeight;
    }
    pPix->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pScreenPriv->pixmapMode = mode;

    return TRUE;
}

Bool
DGAScreenAvailable(ScreenPtr pScreen)
{
    if (!DGAScreenKeyRegistered)
        return FALSE;

    if (DGA_GET_SCREEN_PRIV(pScreen))
        return TRUE;
    return FALSE;
}

static Bool
DGAAvailable(int index)
{
    ScreenPtr pScreen;

    assert(index < MAXSCREENS);
    pScreen = screenInfo.screens[index];
    return DGAScreenAvailable(pScreen);
}

Bool
DGAActive(int index)
{
    DGAScreenPtr pScreenPriv;

    if (!DGAScreenKeyRegistered)
        return FALSE;

    pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    if (pScreenPriv && pScreenPriv->current)
        return TRUE;

    return FALSE;
}

/* Called by the extension to initialize a mode */

static int
DGASetMode(int index, int num, XDGAModePtr mode, PixmapPtr *pPix)
{
    ScrnInfoPtr pScrn = xf86Screens[index];
    DGADeviceRec device;
    int ret;

    /* We rely on the extension to check that DGA is available */

    ret = (*pScrn->SetDGAMode) (pScrn, num, &device);
    if ((ret == Success) && num) {
        DGACopyModeInfo(device.mode, mode);
        *pPix = device.pPix;
    }

    return ret;
}

/* Called from the extension to let the DDX know which events are requested */

static void
DGASelectInput(int index, ClientPtr client, long mask)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    /* We rely on the extension to check that DGA is available */
    pScreenPriv->client = client;
    pScreenPriv->input = mask;
}

static int
DGAGetViewportStatus(int index)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    /* We rely on the extension to check that DGA is active */

    if (!pScreenPriv->funcs->GetViewport)
        return 0;

    return (*pScreenPriv->funcs->GetViewport) (pScreenPriv->pScrn);
}

static int
DGASetViewport(int index, int x, int y, int mode)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    if (pScreenPriv->funcs->SetViewport)
        (*pScreenPriv->funcs->SetViewport) (pScreenPriv->pScrn, x, y, mode);
    return Success;
}

static int
BitsClear(CARD32 data)
{
    int bits = 0;
    CARD32 mask;

    for (mask = 1; mask; mask <<= 1) {
        if (!(data & mask))
            bits++;
        else
            break;
    }

    return bits;
}

static int
DGACreateColormap(int index, ClientPtr client, int id, int mode, int alloc)
{
    ScreenPtr pScreen = screenInfo.screens[index];
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
    FakedVisualList *fvlp;
    VisualPtr pVisual;
    DGAModePtr pMode;
    ColormapPtr pmap;

    if (!mode || (mode > pScreenPriv->numModes))
        return BadValue;

    if ((alloc != AllocNone) && (alloc != AllocAll))
        return BadValue;

    pMode = &(pScreenPriv->modes[mode - 1]);

    if (!(pVisual = malloc(sizeof(VisualRec))))
        return BadAlloc;

    pVisual->vid = FakeClientID(0);
    pVisual->class = pMode->visualClass;
    pVisual->nplanes = pMode->depth;
    pVisual->ColormapEntries = 1 << pMode->depth;
    pVisual->bitsPerRGBValue = (pMode->depth + 2) / 3;

    switch (pVisual->class) {
    case PseudoColor:
    case GrayScale:
    case StaticGray:
        pVisual->bitsPerRGBValue = 8;   /* not quite */
        pVisual->redMask = 0;
        pVisual->greenMask = 0;
        pVisual->blueMask = 0;
        pVisual->offsetRed = 0;
        pVisual->offsetGreen = 0;
        pVisual->offsetBlue = 0;
        break;
    case DirectColor:
    case TrueColor:
        pVisual->ColormapEntries = 1 << pVisual->bitsPerRGBValue;
        /* fall through */
    case StaticColor:
        pVisual->redMask = pMode->red_mask;
        pVisual->greenMask = pMode->green_mask;
        pVisual->blueMask = pMode->blue_mask;
        pVisual->offsetRed = BitsClear(pVisual->redMask);
        pVisual->offsetGreen = BitsClear(pVisual->greenMask);
        pVisual->offsetBlue = BitsClear(pVisual->blueMask);
    }

    if (!(fvlp = malloc(sizeof(FakedVisualList)))) {
        free(pVisual);
        return BadAlloc;
    }

    fvlp->free = FALSE;
    fvlp->pVisual = pVisual;
    fvlp->next = pScreenPriv->fakedVisuals;
    pScreenPriv->fakedVisuals = fvlp;

    LEGAL_NEW_RESOURCE(id, client);

    return CreateColormap(id, pScreen, pVisual, &pmap, alloc, client->index);
}

/*  Called by the extension to install a colormap on DGA active screens */

static void
DGAInstallCmap(ColormapPtr cmap)
{
    ScreenPtr pScreen = cmap->pScreen;
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

    /* We rely on the extension to check that DGA is active */

    if (!pScreenPriv->dgaColormap)
        pScreenPriv->savedColormap = GetInstalledmiColormap(pScreen);

    pScreenPriv->dgaColormap = cmap;

    (*pScreen->InstallColormap) (cmap);
}

static int
DGASync(int index)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    /* We rely on the extension to check that DGA is active */

    if (pScreenPriv->funcs->Sync)
        (*pScreenPriv->funcs->Sync) (pScreenPriv->pScrn);

    return Success;
}

static int
DGAFillRect(int index, int x, int y, int w, int h, unsigned long color)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    /* We rely on the extension to check that DGA is active */

    if (pScreenPriv->funcs->FillRect &&
        (pScreenPriv->current->mode->flags & DGA_FILL_RECT)) {

        (*pScreenPriv->funcs->FillRect) (pScreenPriv->pScrn, x, y, w, h, color);
        return Success;
    }
    return BadMatch;
}

static int
DGABlitRect(int index, int srcx, int srcy, int w, int h, int dstx, int dsty)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    /* We rely on the extension to check that DGA is active */

    if (pScreenPriv->funcs->BlitRect &&
        (pScreenPriv->current->mode->flags & DGA_BLIT_RECT)) {

        (*pScreenPriv->funcs->BlitRect) (pScreenPriv->pScrn,
                                         srcx, srcy, w, h, dstx, dsty);
        return Success;
    }
    return BadMatch;
}

static int
DGABlitTransRect(int index,
                 int srcx, int srcy,
                 int w, int h, int dstx, int dsty, unsigned long color)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    /* We rely on the extension to check that DGA is active */

    if (pScreenPriv->funcs->BlitTransRect &&
        (pScreenPriv->current->mode->flags & DGA_BLIT_RECT_TRANS)) {

        (*pScreenPriv->funcs->BlitTransRect) (pScreenPriv->pScrn,
                                              srcx, srcy, w, h, dstx, dsty,
                                              color);
        return Success;
    }
    return BadMatch;
}

static int
DGAGetModes(int index)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    /* We rely on the extension to check that DGA is available */

    return pScreenPriv->numModes;
}

static int
DGAGetModeInfo(int index, XDGAModePtr mode, int num)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    /* We rely on the extension to check that DGA is available */

    if ((num <= 0) || (num > pScreenPriv->numModes))
        return BadValue;

    DGACopyModeInfo(&(pScreenPriv->modes[num - 1]), mode);

    return Success;
}

static void
DGACopyModeInfo(DGAModePtr mode, XDGAModePtr xmode)
{
    DisplayModePtr dmode = mode->mode;

    xmode->num = mode->num;
    xmode->name = dmode->name;
    xmode->VSync_num = (int) (dmode->VRefresh * 1000.0);
    xmode->VSync_den = 1000;
    xmode->flags = mode->flags;
    xmode->imageWidth = mode->imageWidth;
    xmode->imageHeight = mode->imageHeight;
    xmode->pixmapWidth = mode->pixmapWidth;
    xmode->pixmapHeight = mode->pixmapHeight;
    xmode->bytesPerScanline = mode->bytesPerScanline;
    xmode->byteOrder = mode->byteOrder;
    xmode->depth = mode->depth;
    xmode->bitsPerPixel = mode->bitsPerPixel;
    xmode->red_mask = mode->red_mask;
    xmode->green_mask = mode->green_mask;
    xmode->blue_mask = mode->blue_mask;
    xmode->visualClass = mode->visualClass;
    xmode->viewportWidth = mode->viewportWidth;
    xmode->viewportHeight = mode->viewportHeight;
    xmode->xViewportStep = mode->xViewportStep;
    xmode->yViewportStep = mode->yViewportStep;
    xmode->maxViewportX = mode->maxViewportX;
    xmode->maxViewportY = mode->maxViewportY;
    xmode->viewportFlags = mode->viewportFlags;
    xmode->reserved1 = mode->reserved1;
    xmode->reserved2 = mode->reserved2;
    xmode->offset = mode->offset;

    if (dmode->Flags & V_INTERLACE)
        xmode->flags |= DGA_INTERLACED;
    if (dmode->Flags & V_DBLSCAN)
        xmode->flags |= DGA_DOUBLESCAN;
}

Bool
DGAVTSwitch(void)
{
    ScreenPtr pScreen;
    int i;

    for (i = 0; i < screenInfo.numScreens; i++) {
        pScreen = screenInfo.screens[i];

        /* Alternatively, this could send events to DGA clients */

        if (DGAScreenKeyRegistered) {
            DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

            if (pScreenPriv && pScreenPriv->current)
                return FALSE;
        }
    }

    return TRUE;
}

Bool
DGAStealKeyEvent(DeviceIntPtr dev, int index, int key_code, int is_down)
{
    DGAScreenPtr pScreenPriv;
    DGAEvent event;

    if (!DGAScreenKeyRegistered)        /* no DGA */
        return FALSE;

    if (key_code < 8 || key_code > 255)
        return FALSE;

    pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    if (!pScreenPriv || !pScreenPriv->grabKeyboard)     /* no direct mode */
        return FALSE;

    event = (DGAEvent) {
        .header = ET_Internal,
        .type = ET_DGAEvent,
        .length = sizeof(event),
        .time = GetTimeInMillis(),
        .subtype = (is_down ? ET_KeyPress : ET_KeyRelease),
        .detail = key_code,
        .dx = 0,
        .dy = 0
    };
    mieqEnqueue(dev, (InternalEvent *) &event);

    return TRUE;
}

Bool
DGAStealMotionEvent(DeviceIntPtr dev, int index, int dx, int dy)
{
    DGAScreenPtr pScreenPriv;
    DGAEvent event;

    if (!DGAScreenKeyRegistered)        /* no DGA */
        return FALSE;

    pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    if (!pScreenPriv || !pScreenPriv->grabMouse)        /* no direct mode */
        return FALSE;

    event = (DGAEvent) {
        .header = ET_Internal,
        .type = ET_DGAEvent,
        .length = sizeof(event),
        .time = GetTimeInMillis(),
        .subtype = ET_Motion,
        .detail = 0,
        .dx = dx,
        .dy = dy
    };
    mieqEnqueue(dev, (InternalEvent *) &event);
    return TRUE;
}

Bool
DGAStealButtonEvent(DeviceIntPtr dev, int index, int button, int is_down)
{
    DGAScreenPtr pScreenPriv;
    DGAEvent event;

    if (!DGAScreenKeyRegistered)        /* no DGA */
        return FALSE;

    pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    if (!pScreenPriv || !pScreenPriv->grabMouse)
        return FALSE;

    event = (DGAEvent) {
        .header = ET_Internal,
        .type = ET_DGAEvent,
        .length = sizeof(event),
        .time = GetTimeInMillis(),
        .subtype = (is_down ? ET_ButtonPress : ET_ButtonRelease),
        .detail = button,
        .dx = 0,
        .dy = 0
    };
    mieqEnqueue(dev, (InternalEvent *) &event);

    return TRUE;
}

/* We have the power to steal or modify events that are about to get queued */

#define NoSuchEvent 0x80000000  /* so doesn't match NoEventMask */
static Mask filters[] = {
    NoSuchEvent,                /* 0 */
    NoSuchEvent,                /* 1 */
    KeyPressMask,               /* KeyPress */
    KeyReleaseMask,             /* KeyRelease */
    ButtonPressMask,            /* ButtonPress */
    ButtonReleaseMask,          /* ButtonRelease */
    PointerMotionMask,          /* MotionNotify (initial state) */
};

static void
DGAProcessKeyboardEvent(ScreenPtr pScreen, DGAEvent * event, DeviceIntPtr keybd)
{
    KeyClassPtr keyc = keybd->key;
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
    DeviceIntPtr pointer = GetMaster(keybd, POINTER_OR_FLOAT);
    DeviceEvent ev = {
        .header = ET_Internal,
        .length = sizeof(ev),
        .detail.key = event->detail,
        .type = event->subtype,
        .root_x = 0,
        .root_y = 0,
        .corestate = XkbStateFieldFromRec(&keyc->xkbInfo->state)
    };
    ev.corestate |= pointer->button->state;

    UpdateDeviceState(keybd, &ev);

    if (!IsMaster(keybd))
        return;

    /*
     * Deliver the DGA event
     */
    if (pScreenPriv->client) {
        dgaEvent de = {
            .u.event.time = event->time,
            .u.event.dx = event->dx,
            .u.event.dy = event->dy,
            .u.event.screen = pScreen->myNum,
            .u.event.state = ev.corestate
        };
        de.u.u.type = DGAEventBase + GetCoreType(ev.type);
        de.u.u.detail = event->detail;

        /* If the DGA client has selected input, then deliver based on the usual filter */
        TryClientEvents(pScreenPriv->client, keybd, (xEvent *) &de, 1,
                        filters[ev.type], pScreenPriv->input, 0);
    }
    else {
        /* If the keyboard is actively grabbed, deliver a grabbed core event */
        if (keybd->deviceGrab.grab && !keybd->deviceGrab.fromPassiveGrab) {
            ev.detail.key = event->detail;
            ev.time = event->time;
            ev.root_x = event->dx;
            ev.root_y = event->dy;
            ev.corestate = event->state;
            ev.deviceid = keybd->id;
            DeliverGrabbedEvent((InternalEvent *) &ev, keybd, FALSE);
        }
    }
}

static void
DGAProcessPointerEvent(ScreenPtr pScreen, DGAEvent * event, DeviceIntPtr mouse)
{
    ButtonClassPtr butc = mouse->button;
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);
    DeviceIntPtr master = GetMaster(mouse, MASTER_KEYBOARD);
    DeviceEvent ev = {
        .header = ET_Internal,
        .length = sizeof(ev),
        .detail.key = event->detail,
        .type = event->subtype,
        .corestate = butc ? butc->state : 0
    };

    if (master && master->key)
        ev.corestate |= XkbStateFieldFromRec(&master->key->xkbInfo->state);

    UpdateDeviceState(mouse, &ev);

    if (!IsMaster(mouse))
        return;

    /*
     * Deliver the DGA event
     */
    if (pScreenPriv->client) {
        int coreEquiv = GetCoreType(ev.type);
        dgaEvent de = {
            .u.event.time = event->time,
            .u.event.dx = event->dx,
            .u.event.dy = event->dy,
            .u.event.screen = pScreen->myNum,
            .u.event.state = ev.corestate
        };
        de.u.u.type = DGAEventBase + coreEquiv;
        de.u.u.detail = event->detail;

        /* If the DGA client has selected input, then deliver based on the usual filter */
        TryClientEvents(pScreenPriv->client, mouse, (xEvent *) &de, 1,
                        filters[coreEquiv], pScreenPriv->input, 0);
    }
    else {
        /* If the pointer is actively grabbed, deliver a grabbed core event */
        if (mouse->deviceGrab.grab && !mouse->deviceGrab.fromPassiveGrab) {
            ev.detail.button = event->detail;
            ev.time = event->time;
            ev.root_x = event->dx;
            ev.root_y = event->dy;
            ev.corestate = event->state;
            /* DGA is core only, so valuators.data doesn't actually matter.
             * Mask must be set for EventToCore to create motion events. */
            SetBit(ev.valuators.mask, 0);
            SetBit(ev.valuators.mask, 1);
            DeliverGrabbedEvent((InternalEvent *) &ev, mouse, FALSE);
        }
    }
}

static Bool
DGAOpenFramebuffer(int index,
                   char **name,
                   unsigned char **mem, int *size, int *offset, int *flags)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    /* We rely on the extension to check that DGA is available */

    return (*pScreenPriv->funcs->OpenFramebuffer) (pScreenPriv->pScrn,
                                                   name, mem, size, offset,
                                                   flags);
}

static void
DGACloseFramebuffer(int index)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);

    /* We rely on the extension to check that DGA is available */
    if (pScreenPriv->funcs->CloseFramebuffer)
        (*pScreenPriv->funcs->CloseFramebuffer) (pScreenPriv->pScrn);
}

/*  For DGA 1.0 backwards compatibility only */

static int
DGAGetOldDGAMode(int index)
{
    DGAScreenPtr pScreenPriv = DGA_GET_SCREEN_PRIV(screenInfo.screens[index]);
    ScrnInfoPtr pScrn = pScreenPriv->pScrn;
    DGAModePtr mode;
    int i, w, h, p;

    /* We rely on the extension to check that DGA is available */

    w = pScrn->currentMode->HDisplay;
    h = pScrn->currentMode->VDisplay;
    p = pad_to_int32(pScrn->displayWidth * bits_to_bytes(pScrn->bitsPerPixel));

    for (i = 0; i < pScreenPriv->numModes; i++) {
        mode = &(pScreenPriv->modes[i]);

        if ((mode->viewportWidth == w) && (mode->viewportHeight == h) &&
            (mode->bytesPerScanline == p) &&
            (mode->bitsPerPixel == pScrn->bitsPerPixel) &&
            (mode->depth == pScrn->depth)) {

            return mode->num;
        }
    }

    return 0;
}

static void
DGAHandleEvent(int screen_num, InternalEvent *ev, DeviceIntPtr device)
{
    DGAEvent *event = &ev->dga_event;
    ScreenPtr pScreen = screenInfo.screens[screen_num];
    DGAScreenPtr pScreenPriv;

    /* no DGA */
    if (!DGAScreenKeyRegistered || noXFree86DGAExtension)
	return;
    pScreenPriv = DGA_GET_SCREEN_PRIV(pScreen);

    /* DGA not initialized on this screen */
    if (!pScreenPriv)
        return;

    switch (event->subtype) {
    case KeyPress:
    case KeyRelease:
        DGAProcessKeyboardEvent(pScreen, event, device);
        break;
    case MotionNotify:
    case ButtonPress:
    case ButtonRelease:
        DGAProcessPointerEvent(pScreen, event, device);
        break;
    default:
        break;
    }
}

static void XDGAResetProc(ExtensionEntry * extEntry);

static void DGAClientStateChange(CallbackListPtr *, void *, void *);

static DevPrivateKeyRec DGAScreenPrivateKeyRec;

#define DGAScreenPrivateKey (&DGAScreenPrivateKeyRec)
#define DGAScreenPrivateKeyRegistered (DGAScreenPrivateKeyRec.initialized)
static DevPrivateKeyRec DGAClientPrivateKeyRec;

#define DGAClientPrivateKey (&DGAClientPrivateKeyRec)
static int DGACallbackRefCount = 0;

/* This holds the client's version information */
typedef struct {
    int major;
    int minor;
} DGAPrivRec, *DGAPrivPtr;

#define DGA_GETCLIENT(idx) ((ClientPtr) \
    dixLookupPrivate(&screenInfo.screens[idx]->devPrivates, DGAScreenPrivateKey))
#define DGA_SETCLIENT(idx,p) \
    dixSetPrivate(&screenInfo.screens[idx]->devPrivates, DGAScreenPrivateKey, p)

#define DGA_GETPRIV(c) ((DGAPrivPtr) \
    dixLookupPrivate(&(c)->devPrivates, DGAClientPrivateKey))
#define DGA_SETPRIV(c,p) \
    dixSetPrivate(&(c)->devPrivates, DGAClientPrivateKey, p)

static void
XDGAResetProc(ExtensionEntry * extEntry)
{
    DeleteCallback(&ClientStateCallback, DGAClientStateChange, NULL);
    DGACallbackRefCount = 0;
}

static int
ProcXDGAQueryVersion(ClientPtr client)
{
    xXDGAQueryVersionReply rep;

    REQUEST_SIZE_MATCH(xXDGAQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = SERVER_XDGA_MAJOR_VERSION;
    rep.minorVersion = SERVER_XDGA_MINOR_VERSION;

    WriteToClient(client, sizeof(xXDGAQueryVersionReply), (char *) &rep);
    return Success;
}

static int
ProcXDGAOpenFramebuffer(ClientPtr client)
{
    REQUEST(xXDGAOpenFramebufferReq);
    xXDGAOpenFramebufferReply rep;
    char *deviceName;
    int nameSize;

    REQUEST_SIZE_MATCH(xXDGAOpenFramebufferReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (!DGAAvailable(stuff->screen))
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (!DGAOpenFramebuffer(stuff->screen, &deviceName,
                            (unsigned char **) (&rep.mem1),
                            (int *) &rep.size, (int *) &rep.offset,
                            (int *) &rep.extra)) {
        return BadAlloc;
    }

    nameSize = deviceName ? (strlen(deviceName) + 1) : 0;
    rep.length = bytes_to_int32(nameSize);

    WriteToClient(client, sizeof(xXDGAOpenFramebufferReply), (char *) &rep);
    if (rep.length)
        WriteToClient(client, nameSize, deviceName);

    return Success;
}

static int
ProcXDGACloseFramebuffer(ClientPtr client)
{
    REQUEST(xXDGACloseFramebufferReq);

    REQUEST_SIZE_MATCH(xXDGACloseFramebufferReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (!DGAAvailable(stuff->screen))
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    DGACloseFramebuffer(stuff->screen);

    return Success;
}

static int
ProcXDGAQueryModes(ClientPtr client)
{
    int i, num, size;

    REQUEST(xXDGAQueryModesReq);
    xXDGAQueryModesReply rep;
    xXDGAModeInfo info;
    XDGAModePtr mode;

    REQUEST_SIZE_MATCH(xXDGAQueryModesReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    rep.type = X_Reply;
    rep.length = 0;
    rep.number = 0;
    rep.sequenceNumber = client->sequence;

    if (!DGAAvailable(stuff->screen)) {
        rep.number = 0;
        rep.length = 0;
        WriteToClient(client, sz_xXDGAQueryModesReply, (char *) &rep);
        return Success;
    }

    if (!(num = DGAGetModes(stuff->screen))) {
        WriteToClient(client, sz_xXDGAQueryModesReply, (char *) &rep);
        return Success;
    }

    if (!(mode = xallocarray(num, sizeof(XDGAModeRec))))
        return BadAlloc;

    for (i = 0; i < num; i++)
        DGAGetModeInfo(stuff->screen, mode + i, i + 1);

    size = num * sz_xXDGAModeInfo;
    for (i = 0; i < num; i++)
        size += pad_to_int32(strlen(mode[i].name) + 1); /* plus NULL */

    rep.number = num;
    rep.length = bytes_to_int32(size);

    WriteToClient(client, sz_xXDGAQueryModesReply, (char *) &rep);

    for (i = 0; i < num; i++) {
        size = strlen(mode[i].name) + 1;

        info.byte_order = mode[i].byteOrder;
        info.depth = mode[i].depth;
        info.num = mode[i].num;
        info.bpp = mode[i].bitsPerPixel;
        info.name_size = (size + 3) & ~3L;
        info.vsync_num = mode[i].VSync_num;
        info.vsync_den = mode[i].VSync_den;
        info.flags = mode[i].flags;
        info.image_width = mode[i].imageWidth;
        info.image_height = mode[i].imageHeight;
        info.pixmap_width = mode[i].pixmapWidth;
        info.pixmap_height = mode[i].pixmapHeight;
        info.bytes_per_scanline = mode[i].bytesPerScanline;
        info.red_mask = mode[i].red_mask;
        info.green_mask = mode[i].green_mask;
        info.blue_mask = mode[i].blue_mask;
        info.visual_class = mode[i].visualClass;
        info.viewport_width = mode[i].viewportWidth;
        info.viewport_height = mode[i].viewportHeight;
        info.viewport_xstep = mode[i].xViewportStep;
        info.viewport_ystep = mode[i].yViewportStep;
        info.viewport_xmax = mode[i].maxViewportX;
        info.viewport_ymax = mode[i].maxViewportY;
        info.viewport_flags = mode[i].viewportFlags;
        info.reserved1 = mode[i].reserved1;
        info.reserved2 = mode[i].reserved2;

        WriteToClient(client, sz_xXDGAModeInfo, (char *) (&info));
        WriteToClient(client, size, mode[i].name);
    }

    free(mode);

    return Success;
}

static void
DGAClientStateChange(CallbackListPtr *pcbl, void *nulldata, void *calldata)
{
    NewClientInfoRec *pci = (NewClientInfoRec *) calldata;
    ClientPtr client = NULL;
    int i;

    for (i = 0; i < screenInfo.numScreens; i++) {
        if (DGA_GETCLIENT(i) == pci->client) {
            client = pci->client;
            break;
        }
    }

    if (client &&
        ((client->clientState == ClientStateGone) ||
         (client->clientState == ClientStateRetained))) {
        XDGAModeRec mode;
        PixmapPtr pPix;

        DGA_SETCLIENT(i, NULL);
        DGASelectInput(i, NULL, 0);
        DGASetMode(i, 0, &mode, &pPix);

        if (--DGACallbackRefCount == 0)
            DeleteCallback(&ClientStateCallback, DGAClientStateChange, NULL);
    }
}

static int
ProcXDGASetMode(ClientPtr client)
{
    REQUEST(xXDGASetModeReq);
    xXDGASetModeReply rep;
    XDGAModeRec mode;
    xXDGAModeInfo info;
    PixmapPtr pPix;
    ClientPtr owner;
    int size;

    REQUEST_SIZE_MATCH(xXDGASetModeReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    owner = DGA_GETCLIENT(stuff->screen);

    rep.type = X_Reply;
    rep.length = 0;
    rep.offset = 0;
    rep.flags = 0;
    rep.sequenceNumber = client->sequence;

    if (!DGAAvailable(stuff->screen))
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    if (owner && owner != client)
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    if (!stuff->mode) {
        if (owner) {
            if (--DGACallbackRefCount == 0)
                DeleteCallback(&ClientStateCallback, DGAClientStateChange,
                               NULL);
        }
        DGA_SETCLIENT(stuff->screen, NULL);
        DGASelectInput(stuff->screen, NULL, 0);
        DGASetMode(stuff->screen, 0, &mode, &pPix);
        WriteToClient(client, sz_xXDGASetModeReply, (char *) &rep);
        return Success;
    }

    if (Success != DGASetMode(stuff->screen, stuff->mode, &mode, &pPix))
        return BadValue;

    if (!owner) {
        if (DGACallbackRefCount++ == 0)
            AddCallback(&ClientStateCallback, DGAClientStateChange, NULL);
    }

    DGA_SETCLIENT(stuff->screen, client);

    if (pPix) {
        if (AddResource(stuff->pid, RT_PIXMAP, (void *) (pPix))) {
            pPix->drawable.id = (int) stuff->pid;
            rep.flags = DGA_PIXMAP_AVAILABLE;
        }
    }

    size = strlen(mode.name) + 1;

    info.byte_order = mode.byteOrder;
    info.depth = mode.depth;
    info.num = mode.num;
    info.bpp = mode.bitsPerPixel;
    info.name_size = (size + 3) & ~3L;
    info.vsync_num = mode.VSync_num;
    info.vsync_den = mode.VSync_den;
    info.flags = mode.flags;
    info.image_width = mode.imageWidth;
    info.image_height = mode.imageHeight;
    info.pixmap_width = mode.pixmapWidth;
    info.pixmap_height = mode.pixmapHeight;
    info.bytes_per_scanline = mode.bytesPerScanline;
    info.red_mask = mode.red_mask;
    info.green_mask = mode.green_mask;
    info.blue_mask = mode.blue_mask;
    info.visual_class = mode.visualClass;
    info.viewport_width = mode.viewportWidth;
    info.viewport_height = mode.viewportHeight;
    info.viewport_xstep = mode.xViewportStep;
    info.viewport_ystep = mode.yViewportStep;
    info.viewport_xmax = mode.maxViewportX;
    info.viewport_ymax = mode.maxViewportY;
    info.viewport_flags = mode.viewportFlags;
    info.reserved1 = mode.reserved1;
    info.reserved2 = mode.reserved2;

    rep.length = bytes_to_int32(sz_xXDGAModeInfo + info.name_size);

    WriteToClient(client, sz_xXDGASetModeReply, (char *) &rep);
    WriteToClient(client, sz_xXDGAModeInfo, (char *) (&info));
    WriteToClient(client, size, mode.name);

    return Success;
}

static int
ProcXDGASetViewport(ClientPtr client)
{
    REQUEST(xXDGASetViewportReq);

    REQUEST_SIZE_MATCH(xXDGASetViewportReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    DGASetViewport(stuff->screen, stuff->x, stuff->y, stuff->flags);

    return Success;
}

static int
ProcXDGAInstallColormap(ClientPtr client)
{
    ColormapPtr cmap;
    int rc;

    REQUEST(xXDGAInstallColormapReq);

    REQUEST_SIZE_MATCH(xXDGAInstallColormapReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    rc = dixLookupResourceByType((void **) &cmap, stuff->cmap, RT_COLORMAP,
                                 client, DixInstallAccess);
    if (rc != Success)
        return rc;
    DGAInstallCmap(cmap);
    return Success;
}

static int
ProcXDGASelectInput(ClientPtr client)
{
    REQUEST(xXDGASelectInputReq);

    REQUEST_SIZE_MATCH(xXDGASelectInputReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    if (DGA_GETCLIENT(stuff->screen) == client)
        DGASelectInput(stuff->screen, client, stuff->mask);

    return Success;
}

static int
ProcXDGAFillRectangle(ClientPtr client)
{
    REQUEST(xXDGAFillRectangleReq);

    REQUEST_SIZE_MATCH(xXDGAFillRectangleReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    if (Success != DGAFillRect(stuff->screen, stuff->x, stuff->y,
                               stuff->width, stuff->height, stuff->color))
        return BadMatch;

    return Success;
}

static int
ProcXDGACopyArea(ClientPtr client)
{
    REQUEST(xXDGACopyAreaReq);

    REQUEST_SIZE_MATCH(xXDGACopyAreaReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    if (Success != DGABlitRect(stuff->screen, stuff->srcx, stuff->srcy,
                               stuff->width, stuff->height, stuff->dstx,
                               stuff->dsty))
        return BadMatch;

    return Success;
}

static int
ProcXDGACopyTransparentArea(ClientPtr client)
{
    REQUEST(xXDGACopyTransparentAreaReq);

    REQUEST_SIZE_MATCH(xXDGACopyTransparentAreaReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    if (Success != DGABlitTransRect(stuff->screen, stuff->srcx, stuff->srcy,
                                    stuff->width, stuff->height, stuff->dstx,
                                    stuff->dsty, stuff->key))
        return BadMatch;

    return Success;
}

static int
ProcXDGAGetViewportStatus(ClientPtr client)
{
    REQUEST(xXDGAGetViewportStatusReq);
    xXDGAGetViewportStatusReply rep;

    REQUEST_SIZE_MATCH(xXDGAGetViewportStatusReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    rep.status = DGAGetViewportStatus(stuff->screen);

    WriteToClient(client, sizeof(xXDGAGetViewportStatusReply), (char *) &rep);
    return Success;
}

static int
ProcXDGASync(ClientPtr client)
{
    REQUEST(xXDGASyncReq);
    xXDGASyncReply rep;

    REQUEST_SIZE_MATCH(xXDGASyncReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    DGASync(stuff->screen);

    WriteToClient(client, sizeof(xXDGASyncReply), (char *) &rep);
    return Success;
}

static int
ProcXDGASetClientVersion(ClientPtr client)
{
    REQUEST(xXDGASetClientVersionReq);

    DGAPrivPtr pPriv;

    REQUEST_SIZE_MATCH(xXDGASetClientVersionReq);
    if ((pPriv = DGA_GETPRIV(client)) == NULL) {
        pPriv = malloc(sizeof(DGAPrivRec));
        /* XXX Need to look into freeing this */
        if (!pPriv)
            return BadAlloc;
        DGA_SETPRIV(client, pPriv);
    }
    pPriv->major = stuff->major;
    pPriv->minor = stuff->minor;

    return Success;
}

static int
ProcXDGAChangePixmapMode(ClientPtr client)
{
    REQUEST(xXDGAChangePixmapModeReq);
    xXDGAChangePixmapModeReply rep;
    int x, y;

    REQUEST_SIZE_MATCH(xXDGAChangePixmapModeReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    x = stuff->x;
    y = stuff->y;

    if (!DGAChangePixmapMode(stuff->screen, &x, &y, stuff->flags))
        return BadMatch;

    rep.x = x;
    rep.y = y;
    WriteToClient(client, sizeof(xXDGAChangePixmapModeReply), (char *) &rep);

    return Success;
}

static int
ProcXDGACreateColormap(ClientPtr client)
{
    REQUEST(xXDGACreateColormapReq);
    int result;

    REQUEST_SIZE_MATCH(xXDGACreateColormapReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    if (!stuff->mode)
        return BadValue;

    result = DGACreateColormap(stuff->screen, client, stuff->id,
                               stuff->mode, stuff->alloc);
    if (result != Success)
        return result;

    return Success;
}

/*
 *
 * Support for the old DGA protocol, used to live in xf86dga.c
 *
 */

#ifdef DGA_PROTOCOL_OLD_SUPPORT

static int
ProcXF86DGAGetVideoLL(ClientPtr client)
{
    REQUEST(xXF86DGAGetVideoLLReq);
    xXF86DGAGetVideoLLReply rep;
    XDGAModeRec mode;
    int num, offset, flags;
    char *name;

    REQUEST_SIZE_MATCH(xXF86DGAGetVideoLLReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (!DGAAvailable(stuff->screen))
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    if (!(num = DGAGetOldDGAMode(stuff->screen)))
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    /* get the parameters for the mode that best matches */
    DGAGetModeInfo(stuff->screen, &mode, num);

    if (!DGAOpenFramebuffer(stuff->screen, &name,
                            (unsigned char **) (&rep.offset),
                            (int *) (&rep.bank_size), &offset, &flags))
        return BadAlloc;

    rep.offset += mode.offset;
    rep.width = mode.bytesPerScanline / (mode.bitsPerPixel >> 3);
    rep.ram_size = rep.bank_size >> 10;

    WriteToClient(client, SIZEOF(xXF86DGAGetVideoLLReply), (char *) &rep);
    return Success;
}

static int
ProcXF86DGADirectVideo(ClientPtr client)
{
    int num;
    PixmapPtr pix;
    XDGAModeRec mode;
    ClientPtr owner;

    REQUEST(xXF86DGADirectVideoReq);

    REQUEST_SIZE_MATCH(xXF86DGADirectVideoReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (!DGAAvailable(stuff->screen))
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    owner = DGA_GETCLIENT(stuff->screen);

    if (owner && owner != client)
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    if (stuff->enable & XF86DGADirectGraphics) {
        if (!(num = DGAGetOldDGAMode(stuff->screen)))
            return DGAErrorBase + XF86DGANoDirectVideoMode;
    }
    else
        num = 0;

    if (Success != DGASetMode(stuff->screen, num, &mode, &pix))
        return DGAErrorBase + XF86DGAScreenNotActive;

    DGASetInputMode(stuff->screen,
                    (stuff->enable & XF86DGADirectKeyb) != 0,
                    (stuff->enable & XF86DGADirectMouse) != 0);

    /* We need to track the client and attach the teardown callback */
    if (stuff->enable &
        (XF86DGADirectGraphics | XF86DGADirectKeyb | XF86DGADirectMouse)) {
        if (!owner) {
            if (DGACallbackRefCount++ == 0)
                AddCallback(&ClientStateCallback, DGAClientStateChange, NULL);
        }

        DGA_SETCLIENT(stuff->screen, client);
    }
    else {
        if (owner) {
            if (--DGACallbackRefCount == 0)
                DeleteCallback(&ClientStateCallback, DGAClientStateChange,
                               NULL);
        }

        DGA_SETCLIENT(stuff->screen, NULL);
    }

    return Success;
}

static int
ProcXF86DGAGetViewPortSize(ClientPtr client)
{
    int num;
    XDGAModeRec mode;

    REQUEST(xXF86DGAGetViewPortSizeReq);
    xXF86DGAGetViewPortSizeReply rep;

    REQUEST_SIZE_MATCH(xXF86DGAGetViewPortSizeReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (!DGAAvailable(stuff->screen))
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    if (!(num = DGAGetOldDGAMode(stuff->screen)))
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    DGAGetModeInfo(stuff->screen, &mode, num);

    rep.width = mode.viewportWidth;
    rep.height = mode.viewportHeight;

    WriteToClient(client, SIZEOF(xXF86DGAGetViewPortSizeReply), (char *) &rep);
    return Success;
}

static int
ProcXF86DGASetViewPort(ClientPtr client)
{
    REQUEST(xXF86DGASetViewPortReq);

    REQUEST_SIZE_MATCH(xXF86DGASetViewPortReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    if (!DGAAvailable(stuff->screen))
        return DGAErrorBase + XF86DGANoDirectVideoMode;

    if (!DGAActive(stuff->screen))
        return DGAErrorBase + XF86DGADirectNotActivated;

    if (DGASetViewport(stuff->screen, stuff->x, stuff->y, DGA_FLIP_RETRACE)
        != Success)
        return DGAErrorBase + XF86DGADirectNotActivated;

    return Success;
}

static int
ProcXF86DGAGetVidPage(ClientPtr client)
{
    REQUEST(xXF86DGAGetVidPageReq);
    xXF86DGAGetVidPageReply rep;

    REQUEST_SIZE_MATCH(xXF86DGAGetVidPageReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.vpage = 0;              /* silently fail */

    WriteToClient(client, SIZEOF(xXF86DGAGetVidPageReply), (char *) &rep);
    return Success;
}

static int
ProcXF86DGASetVidPage(ClientPtr client)
{
    REQUEST(xXF86DGASetVidPageReq);

    REQUEST_SIZE_MATCH(xXF86DGASetVidPageReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    /* silently fail */

    return Success;
}

static int
ProcXF86DGAInstallColormap(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xXF86DGAInstallColormapReq);

    REQUEST_SIZE_MATCH(xXF86DGAInstallColormapReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    if (!DGAActive(stuff->screen))
        return DGAErrorBase + XF86DGADirectNotActivated;

    rc = dixLookupResourceByType((void **) &pcmp, stuff->id, RT_COLORMAP,
                                 client, DixInstallAccess);
    if (rc == Success) {
        DGAInstallCmap(pcmp);
        return Success;
    }
    else {
        return rc;
    }
}

static int
ProcXF86DGAQueryDirectVideo(ClientPtr client)
{
    REQUEST(xXF86DGAQueryDirectVideoReq);
    xXF86DGAQueryDirectVideoReply rep;

    REQUEST_SIZE_MATCH(xXF86DGAQueryDirectVideoReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.flags = 0;

    if (DGAAvailable(stuff->screen))
        rep.flags = XF86DGADirectPresent;

    WriteToClient(client, SIZEOF(xXF86DGAQueryDirectVideoReply), (char *) &rep);
    return Success;
}

static int
ProcXF86DGAViewPortChanged(ClientPtr client)
{
    REQUEST(xXF86DGAViewPortChangedReq);
    xXF86DGAViewPortChangedReply rep;

    REQUEST_SIZE_MATCH(xXF86DGAViewPortChangedReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (DGA_GETCLIENT(stuff->screen) != client)
        return DGAErrorBase + XF86DGADirectNotActivated;

    if (!DGAActive(stuff->screen))
        return DGAErrorBase + XF86DGADirectNotActivated;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.result = 1;

    WriteToClient(client, SIZEOF(xXF86DGAViewPortChangedReply), (char *) &rep);
    return Success;
}

#endif                          /* DGA_PROTOCOL_OLD_SUPPORT */

static int _X_COLD
SProcXDGADispatch(ClientPtr client)
{
    return DGAErrorBase + XF86DGAClientNotLocal;
}

#if 0
#define DGA_REQ_DEBUG
#endif

#ifdef DGA_REQ_DEBUG
static char *dgaMinor[] = {
    "QueryVersion",
    "GetVideoLL",
    "DirectVideo",
    "GetViewPortSize",
    "SetViewPort",
    "GetVidPage",
    "SetVidPage",
    "InstallColormap",
    "QueryDirectVideo",
    "ViewPortChanged",
    "10",
    "11",
    "QueryModes",
    "SetMode",
    "SetViewport",
    "InstallColormap",
    "SelectInput",
    "FillRectangle",
    "CopyArea",
    "CopyTransparentArea",
    "GetViewportStatus",
    "Sync",
    "OpenFramebuffer",
    "CloseFramebuffer",
    "SetClientVersion",
    "ChangePixmapMode",
    "CreateColormap",
};
#endif

static int
ProcXDGADispatch(ClientPtr client)
{
    REQUEST(xReq);

    if (!client->local)
        return DGAErrorBase + XF86DGAClientNotLocal;

#ifdef DGA_REQ_DEBUG
    if (stuff->data <= X_XDGACreateColormap)
        fprintf(stderr, "    DGA %s\n", dgaMinor[stuff->data]);
#endif

    switch (stuff->data) {
        /*
         * DGA2 Protocol
         */
    case X_XDGAQueryVersion:
        return ProcXDGAQueryVersion(client);
    case X_XDGAQueryModes:
        return ProcXDGAQueryModes(client);
    case X_XDGASetMode:
        return ProcXDGASetMode(client);
    case X_XDGAOpenFramebuffer:
        return ProcXDGAOpenFramebuffer(client);
    case X_XDGACloseFramebuffer:
        return ProcXDGACloseFramebuffer(client);
    case X_XDGASetViewport:
        return ProcXDGASetViewport(client);
    case X_XDGAInstallColormap:
        return ProcXDGAInstallColormap(client);
    case X_XDGASelectInput:
        return ProcXDGASelectInput(client);
    case X_XDGAFillRectangle:
        return ProcXDGAFillRectangle(client);
    case X_XDGACopyArea:
        return ProcXDGACopyArea(client);
    case X_XDGACopyTransparentArea:
        return ProcXDGACopyTransparentArea(client);
    case X_XDGAGetViewportStatus:
        return ProcXDGAGetViewportStatus(client);
    case X_XDGASync:
        return ProcXDGASync(client);
    case X_XDGASetClientVersion:
        return ProcXDGASetClientVersion(client);
    case X_XDGAChangePixmapMode:
        return ProcXDGAChangePixmapMode(client);
    case X_XDGACreateColormap:
        return ProcXDGACreateColormap(client);
        /*
         * Old DGA Protocol
         */
#ifdef DGA_PROTOCOL_OLD_SUPPORT
    case X_XF86DGAGetVideoLL:
        return ProcXF86DGAGetVideoLL(client);
    case X_XF86DGADirectVideo:
        return ProcXF86DGADirectVideo(client);
    case X_XF86DGAGetViewPortSize:
        return ProcXF86DGAGetViewPortSize(client);
    case X_XF86DGASetViewPort:
        return ProcXF86DGASetViewPort(client);
    case X_XF86DGAGetVidPage:
        return ProcXF86DGAGetVidPage(client);
    case X_XF86DGASetVidPage:
        return ProcXF86DGASetVidPage(client);
    case X_XF86DGAInstallColormap:
        return ProcXF86DGAInstallColormap(client);
    case X_XF86DGAQueryDirectVideo:
        return ProcXF86DGAQueryDirectVideo(client);
    case X_XF86DGAViewPortChanged:
        return ProcXF86DGAViewPortChanged(client);
#endif                          /* DGA_PROTOCOL_OLD_SUPPORT */
    default:
        return BadRequest;
    }
}

void
XFree86DGAExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (!dixRegisterPrivateKey(&DGAClientPrivateKeyRec, PRIVATE_CLIENT, 0))
        return;

    if (!dixRegisterPrivateKey(&DGAScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return;

    if ((extEntry = AddExtension(XF86DGANAME,
                                 XF86DGANumberEvents,
                                 XF86DGANumberErrors,
                                 ProcXDGADispatch,
                                 SProcXDGADispatch,
                                 XDGAResetProc, StandardMinorOpcode))) {
        int i;

        DGAReqCode = (unsigned char) extEntry->base;
        DGAErrorBase = extEntry->errorBase;
        DGAEventBase = extEntry->eventBase;
        for (i = KeyPress; i <= MotionNotify; i++)
            SetCriticalEvent(DGAEventBase + i);
    }
}
