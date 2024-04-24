/*
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "os.h"
#include "globals.h"
#include "xf86.h"
#include "xf86str.h"
#include "xf86Priv.h"
#include "xf86DDC.h"
#include "mipointer.h"
#include <randrstr.h>
#include "inputstr.h"

typedef struct _xf86RandRInfo {
    CloseScreenProcPtr CloseScreen;
    int virtualX;
    int virtualY;
    int mmWidth;
    int mmHeight;
    Rotation rotation;
} XF86RandRInfoRec, *XF86RandRInfoPtr;

static DevPrivateKeyRec xf86RandRKeyRec;
static DevPrivateKey xf86RandRKey;

#define XF86RANDRINFO(p) ((XF86RandRInfoPtr)dixLookupPrivate(&(p)->devPrivates, xf86RandRKey))

static int
xf86RandRModeRefresh(DisplayModePtr mode)
{
    if (mode->VRefresh)
        return (int) (mode->VRefresh + 0.5);
    else if (mode->Clock == 0)
        return 0;
    else
        return (int) (mode->Clock * 1000.0 / mode->HTotal / mode->VTotal + 0.5);
}

static Bool
xf86RandRGetInfo(ScreenPtr pScreen, Rotation * rotations)
{
    RRScreenSizePtr pSize;
    ScrnInfoPtr scrp = xf86ScreenToScrn(pScreen);
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    DisplayModePtr mode;
    int refresh0 = 60;
    xorgRRModeMM RRModeMM;

    *rotations = RR_Rotate_0;

    for (mode = scrp->modes; mode != NULL; mode = mode->next) {
        int refresh = xf86RandRModeRefresh(mode);

        if (mode == scrp->modes)
            refresh0 = refresh;

        RRModeMM.mode = mode;
        RRModeMM.virtX = randrp->virtualX;
        RRModeMM.virtY = randrp->virtualY;
        RRModeMM.mmWidth = randrp->mmWidth;
        RRModeMM.mmHeight = randrp->mmHeight;

        if (scrp->DriverFunc) {
            (*scrp->DriverFunc) (scrp, RR_GET_MODE_MM, &RRModeMM);
        }

        pSize = RRRegisterSize(pScreen,
                               mode->HDisplay, mode->VDisplay,
                               RRModeMM.mmWidth, RRModeMM.mmHeight);
        if (!pSize)
            return FALSE;
        RRRegisterRate(pScreen, pSize, refresh);
        if (mode == scrp->currentMode &&
            mode->HDisplay == scrp->virtualX &&
            mode->VDisplay == scrp->virtualY)
            RRSetCurrentConfig(pScreen, randrp->rotation, refresh, pSize);
        if (mode->next == scrp->modes)
            break;
    }
    if (scrp->currentMode->HDisplay != randrp->virtualX ||
        scrp->currentMode->VDisplay != randrp->virtualY) {
        mode = scrp->modes;

        RRModeMM.mode = NULL;
        RRModeMM.virtX = randrp->virtualX;
        RRModeMM.virtY = randrp->virtualY;
        RRModeMM.mmWidth = randrp->mmWidth;
        RRModeMM.mmHeight = randrp->mmHeight;

        if (scrp->DriverFunc) {
            (*scrp->DriverFunc) (scrp, RR_GET_MODE_MM, &RRModeMM);
        }

        pSize = RRRegisterSize(pScreen,
                               randrp->virtualX, randrp->virtualY,
                               RRModeMM.mmWidth, RRModeMM.mmHeight);
        if (!pSize)
            return FALSE;
        RRRegisterRate(pScreen, pSize, refresh0);
        if (scrp->virtualX == randrp->virtualX &&
            scrp->virtualY == randrp->virtualY) {
            RRSetCurrentConfig(pScreen, randrp->rotation, refresh0, pSize);
        }
    }

    /* If there is driver support for randr, let it set our supported rotations */
    if (scrp->DriverFunc) {
        xorgRRRotation RRRotation;

        RRRotation.RRRotations = *rotations;
        if (!(*scrp->DriverFunc) (scrp, RR_GET_INFO, &RRRotation))
            return TRUE;
        *rotations = RRRotation.RRRotations;
    }

    return TRUE;
}

static Bool
xf86RandRSetMode(ScreenPtr pScreen,
                 DisplayModePtr mode,
                 Bool useVirtual, int mmWidth, int mmHeight)
{
    ScrnInfoPtr scrp = xf86ScreenToScrn(pScreen);
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    int oldWidth = pScreen->width;
    int oldHeight = pScreen->height;
    int oldmmWidth = pScreen->mmWidth;
    int oldmmHeight = pScreen->mmHeight;
    int oldVirtualX = scrp->virtualX;
    int oldVirtualY = scrp->virtualY;
    WindowPtr pRoot = pScreen->root;
    Bool ret = TRUE;

    if (pRoot && scrp->vtSema)
        (*scrp->EnableDisableFBAccess) (scrp, FALSE);
    if (useVirtual) {
        scrp->virtualX = randrp->virtualX;
        scrp->virtualY = randrp->virtualY;
    }
    else {
        scrp->virtualX = mode->HDisplay;
        scrp->virtualY = mode->VDisplay;
    }

    /*
     * The DIX forgets the physical dimensions we passed into RRRegisterSize, so
     * reconstruct them if possible.
     */
    if (scrp->DriverFunc) {
        xorgRRModeMM RRModeMM;

        RRModeMM.mode = mode;
        RRModeMM.virtX = scrp->virtualX;
        RRModeMM.virtY = scrp->virtualY;
        RRModeMM.mmWidth = mmWidth;
        RRModeMM.mmHeight = mmHeight;

        (*scrp->DriverFunc) (scrp, RR_GET_MODE_MM, &RRModeMM);

        mmWidth = RRModeMM.mmWidth;
        mmHeight = RRModeMM.mmHeight;
    }
    if (randrp->rotation & (RR_Rotate_90 | RR_Rotate_270)) {
        /* If the screen is rotated 90 or 270 degrees, swap the sizes. */
        pScreen->width = scrp->virtualY;
        pScreen->height = scrp->virtualX;
        pScreen->mmWidth = mmHeight;
        pScreen->mmHeight = mmWidth;
    }
    else {
        pScreen->width = scrp->virtualX;
        pScreen->height = scrp->virtualY;
        pScreen->mmWidth = mmWidth;
        pScreen->mmHeight = mmHeight;
    }
    if (!xf86SwitchMode(pScreen, mode)) {
        pScreen->width = oldWidth;
        pScreen->height = oldHeight;
        pScreen->mmWidth = oldmmWidth;
        pScreen->mmHeight = oldmmHeight;
        scrp->virtualX = oldVirtualX;
        scrp->virtualY = oldVirtualY;
        ret = FALSE;
    }
    /*
     * Make sure the layout is correct
     */
    xf86ReconfigureLayout();

    if (scrp->vtSema) {
        /*
         * Make sure the whole screen is visible
         */
        xf86SetViewport (pScreen, pScreen->width, pScreen->height);
        xf86SetViewport (pScreen, 0, 0);
        if (pRoot)
            (*scrp->EnableDisableFBAccess) (scrp, TRUE);
    }
    return ret;
}

static Bool
xf86RandRSetConfig(ScreenPtr pScreen,
                   Rotation rotation, int rate, RRScreenSizePtr pSize)
{
    ScrnInfoPtr scrp = xf86ScreenToScrn(pScreen);
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    DisplayModePtr mode;
    int pos[MAXDEVICES][2];
    Bool useVirtual = FALSE;
    Rotation oldRotation = randrp->rotation;
    DeviceIntPtr dev;
    Bool view_adjusted = FALSE;

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (!IsMaster(dev) && !IsFloating(dev))
            continue;

        miPointerGetPosition(dev, &pos[dev->id][0], &pos[dev->id][1]);
    }

    for (mode = scrp->modes;; mode = mode->next) {
        if (mode->HDisplay == pSize->width &&
            mode->VDisplay == pSize->height &&
            (rate == 0 || xf86RandRModeRefresh(mode) == rate))
            break;
        if (mode->next == scrp->modes) {
            if (pSize->width == randrp->virtualX &&
                pSize->height == randrp->virtualY) {
                mode = scrp->modes;
                useVirtual = TRUE;
                break;
            }
            return FALSE;
        }
    }

    if (randrp->rotation != rotation) {

        /* Have the driver do its thing. */
        if (scrp->DriverFunc) {
            xorgRRRotation RRRotation;

            RRRotation.RRConfig.rotation = rotation;
            RRRotation.RRConfig.rate = rate;
            RRRotation.RRConfig.width = pSize->width;
            RRRotation.RRConfig.height = pSize->height;

            /*
             * Currently we need to rely on HW support for rotation.
             */
            if (!(*scrp->DriverFunc) (scrp, RR_SET_CONFIG, &RRRotation))
                return FALSE;
        }
        else
            return FALSE;

        randrp->rotation = rotation;
    }

    if (!xf86RandRSetMode
        (pScreen, mode, useVirtual, pSize->mmWidth, pSize->mmHeight)) {
        if (randrp->rotation != oldRotation) {
            /* Have the driver undo its thing. */
            if (scrp->DriverFunc) {
                xorgRRRotation RRRotation;

                RRRotation.RRConfig.rotation = oldRotation;
                RRRotation.RRConfig.rate =
                    xf86RandRModeRefresh(scrp->currentMode);
                RRRotation.RRConfig.width = scrp->virtualX;
                RRRotation.RRConfig.height = scrp->virtualY;
                (*scrp->DriverFunc) (scrp, RR_SET_CONFIG, &RRRotation);
            }

            randrp->rotation = oldRotation;
        }
        return FALSE;
    }

    update_desktop_dimensions();

    /*
     * Move the cursor back where it belongs; SwitchMode repositions it
     * FIXME: duplicated code, see modes/xf86RandR12.c
     */
    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (!IsMaster(dev) && !IsFloating(dev))
            continue;

        if (pScreen == miPointerGetScreen(dev)) {
            int px = pos[dev->id][0];
            int py = pos[dev->id][1];

            px = (px >= pScreen->width ? (pScreen->width - 1) : px);
            py = (py >= pScreen->height ? (pScreen->height - 1) : py);

            /* Setting the viewpoint makes only sense on one device */
            if (!view_adjusted && IsMaster(dev)) {
                xf86SetViewport(pScreen, px, py);
                view_adjusted = TRUE;
            }

            (*pScreen->SetCursorPosition) (dev, pScreen, px, py, FALSE);
        }
    }

    return TRUE;
}

/*
 * Reset size back to original
 */
static Bool
xf86RandRCloseScreen(ScreenPtr pScreen)
{
    ScrnInfoPtr scrp = xf86ScreenToScrn(pScreen);
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);

    scrp->virtualX = pScreen->width = randrp->virtualX;
    scrp->virtualY = pScreen->height = randrp->virtualY;
    scrp->currentMode = scrp->modes;
    pScreen->CloseScreen = randrp->CloseScreen;
    free(randrp);
    dixSetPrivate(&pScreen->devPrivates, xf86RandRKey, NULL);
    return (*pScreen->CloseScreen) (pScreen);
}

Rotation
xf86GetRotation(ScreenPtr pScreen)
{
    if (xf86RandRKey == NULL)
        return RR_Rotate_0;

    return XF86RANDRINFO(pScreen)->rotation;
}

/* Function to change RandR's idea of the virtual screen size */
Bool
xf86RandRSetNewVirtualAndDimensions(ScreenPtr pScreen,
                                    int newvirtX, int newvirtY, int newmmWidth,
                                    int newmmHeight, Bool resetMode)
{
    XF86RandRInfoPtr randrp;

    if (xf86RandRKey == NULL)
        return FALSE;

    randrp = XF86RANDRINFO(pScreen);
    if (randrp == NULL)
        return FALSE;

    if (newvirtX > 0)
        randrp->virtualX = newvirtX;

    if (newvirtY > 0)
        randrp->virtualY = newvirtY;

    if (newmmWidth > 0)
        randrp->mmWidth = newmmWidth;

    if (newmmHeight > 0)
        randrp->mmHeight = newmmHeight;

    /* This is only for during server start */
    if (resetMode) {
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
        return (xf86RandRSetMode(pScreen,
                                 pScrn->currentMode,
                                 TRUE, pScreen->mmWidth, pScreen->mmHeight));
    }

    return TRUE;
}

Bool
xf86RandRInit(ScreenPtr pScreen)
{
    rrScrPrivPtr rp;
    XF86RandRInfoPtr randrp;
    ScrnInfoPtr scrp = xf86ScreenToScrn(pScreen);

#ifdef PANORAMIX
    /* XXX disable RandR when using Xinerama */
    if (!noPanoramiXExtension)
        return TRUE;
#endif

    xf86RandRKey = &xf86RandRKeyRec;

    if (!dixRegisterPrivateKey(&xf86RandRKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    randrp = malloc(sizeof(XF86RandRInfoRec));
    if (!randrp)
        return FALSE;

    if (!RRScreenInit(pScreen)) {
        free(randrp);
        return FALSE;
    }
    rp = rrGetScrPriv(pScreen);
    rp->rrGetInfo = xf86RandRGetInfo;
    rp->rrSetConfig = xf86RandRSetConfig;

    randrp->virtualX = scrp->virtualX;
    randrp->virtualY = scrp->virtualY;
    randrp->mmWidth = pScreen->mmWidth;
    randrp->mmHeight = pScreen->mmHeight;

    randrp->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = xf86RandRCloseScreen;

    randrp->rotation = RR_Rotate_0;

    dixSetPrivate(&pScreen->devPrivates, xf86RandRKey, randrp);
    return TRUE;
}
