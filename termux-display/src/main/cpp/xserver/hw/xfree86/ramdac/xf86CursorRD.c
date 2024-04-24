
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86CursorPriv.h"
#include "colormapst.h"
#include "cursorstr.h"

/* FIXME: This was added with the ABI change of the miPointerSpriteFuncs for
 * MPX.
 * inputInfo is needed to pass the core pointer as the default argument into
 * the cursor functions.
 *
 * Externing inputInfo is not the nice way to do it but it works.
 */
#include "inputstr.h"

DevPrivateKeyRec xf86CursorScreenKeyRec;

/* sprite functions */

static Bool xf86CursorRealizeCursor(DeviceIntPtr, ScreenPtr, CursorPtr);
static Bool xf86CursorUnrealizeCursor(DeviceIntPtr, ScreenPtr, CursorPtr);
static void xf86CursorSetCursor(DeviceIntPtr, ScreenPtr, CursorPtr, int, int);
static void xf86CursorMoveCursor(DeviceIntPtr, ScreenPtr, int, int);
static Bool xf86DeviceCursorInitialize(DeviceIntPtr, ScreenPtr);
static void xf86DeviceCursorCleanup(DeviceIntPtr, ScreenPtr);

static miPointerSpriteFuncRec xf86CursorSpriteFuncs = {
    xf86CursorRealizeCursor,
    xf86CursorUnrealizeCursor,
    xf86CursorSetCursor,
    xf86CursorMoveCursor,
    xf86DeviceCursorInitialize,
    xf86DeviceCursorCleanup
};

/* Screen functions */

static void xf86CursorInstallColormap(ColormapPtr);
static void xf86CursorRecolorCursor(DeviceIntPtr pDev, ScreenPtr, CursorPtr,
                                    Bool);
static Bool xf86CursorCloseScreen(ScreenPtr);
static void xf86CursorQueryBestSize(int, unsigned short *, unsigned short *,
                                    ScreenPtr);

/* ScrnInfoRec functions */

static void xf86CursorEnableDisableFBAccess(ScrnInfoPtr, Bool);
static Bool xf86CursorSwitchMode(ScrnInfoPtr, DisplayModePtr);

Bool
xf86InitCursor(ScreenPtr pScreen, xf86CursorInfoPtr infoPtr)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CursorScreenPtr ScreenPriv;
    miPointerScreenPtr PointPriv;

    if (!xf86InitHardwareCursor(pScreen, infoPtr))
        return FALSE;

    if (!dixRegisterPrivateKey(&xf86CursorScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    ScreenPriv = calloc(1, sizeof(xf86CursorScreenRec));
    if (!ScreenPriv)
        return FALSE;

    dixSetPrivate(&pScreen->devPrivates, xf86CursorScreenKey, ScreenPriv);

    ScreenPriv->SWCursor = TRUE;
    ScreenPriv->isUp = FALSE;
    ScreenPriv->CurrentCursor = NULL;
    ScreenPriv->CursorInfoPtr = infoPtr;
    ScreenPriv->PalettedCursor = FALSE;
    ScreenPriv->pInstalledMap = NULL;

    ScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = xf86CursorCloseScreen;
    ScreenPriv->QueryBestSize = pScreen->QueryBestSize;
    pScreen->QueryBestSize = xf86CursorQueryBestSize;
    ScreenPriv->RecolorCursor = pScreen->RecolorCursor;
    pScreen->RecolorCursor = xf86CursorRecolorCursor;

    if ((infoPtr->pScrn->bitsPerPixel == 8) &&
        !(infoPtr->Flags & HARDWARE_CURSOR_TRUECOLOR_AT_8BPP)) {
        ScreenPriv->InstallColormap = pScreen->InstallColormap;
        pScreen->InstallColormap = xf86CursorInstallColormap;
        ScreenPriv->PalettedCursor = TRUE;
    }

    PointPriv = dixLookupPrivate(&pScreen->devPrivates, miPointerScreenKey);

    ScreenPriv->showTransparent = PointPriv->showTransparent;
    if (infoPtr->Flags & HARDWARE_CURSOR_SHOW_TRANSPARENT)
        PointPriv->showTransparent = TRUE;
    else
        PointPriv->showTransparent = FALSE;
    ScreenPriv->spriteFuncs = PointPriv->spriteFuncs;
    PointPriv->spriteFuncs = &xf86CursorSpriteFuncs;

    ScreenPriv->EnableDisableFBAccess = pScrn->EnableDisableFBAccess;
    ScreenPriv->SwitchMode = pScrn->SwitchMode;

    ScreenPriv->ForceHWCursorCount = 0;
    ScreenPriv->HWCursorForced = FALSE;

    pScrn->EnableDisableFBAccess = xf86CursorEnableDisableFBAccess;
    if (pScrn->SwitchMode)
        pScrn->SwitchMode = xf86CursorSwitchMode;

    return TRUE;
}

/***** Screen functions *****/

static Bool
xf86CursorCloseScreen(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    miPointerScreenPtr PointPriv =
        (miPointerScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                              miPointerScreenKey);
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    if (ScreenPriv->isUp && pScrn->vtSema)
        xf86SetCursor(pScreen, NullCursor, ScreenPriv->x, ScreenPriv->y);

    if (ScreenPriv->CurrentCursor)
        FreeCursor(ScreenPriv->CurrentCursor, None);

    pScreen->CloseScreen = ScreenPriv->CloseScreen;
    pScreen->QueryBestSize = ScreenPriv->QueryBestSize;
    pScreen->RecolorCursor = ScreenPriv->RecolorCursor;
    if (ScreenPriv->InstallColormap)
        pScreen->InstallColormap = ScreenPriv->InstallColormap;

    PointPriv->spriteFuncs = ScreenPriv->spriteFuncs;
    PointPriv->showTransparent = ScreenPriv->showTransparent;

    pScrn->EnableDisableFBAccess = ScreenPriv->EnableDisableFBAccess;
    pScrn->SwitchMode = ScreenPriv->SwitchMode;

    free(ScreenPriv->transparentData);
    free(ScreenPriv);

    return (*pScreen->CloseScreen) (pScreen);
}

static void
xf86CursorQueryBestSize(int class,
                        unsigned short *width,
                        unsigned short *height, ScreenPtr pScreen)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    if (class == CursorShape) {
        if (*width > ScreenPriv->CursorInfoPtr->MaxWidth)
            *width = ScreenPriv->CursorInfoPtr->MaxWidth;
        if (*height > ScreenPriv->CursorInfoPtr->MaxHeight)
            *height = ScreenPriv->CursorInfoPtr->MaxHeight;
    }
    else
        (*ScreenPriv->QueryBestSize) (class, width, height, pScreen);
}

static void
xf86CursorInstallColormap(ColormapPtr pMap)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pMap->pScreen->devPrivates,
                                               xf86CursorScreenKey);

    ScreenPriv->pInstalledMap = pMap;

    (*ScreenPriv->InstallColormap) (pMap);
}

static void
xf86CursorRecolorCursor(DeviceIntPtr pDev,
                        ScreenPtr pScreen, CursorPtr pCurs, Bool displayed)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    if (!displayed)
        return;

    if (ScreenPriv->SWCursor)
        (*ScreenPriv->RecolorCursor) (pDev, pScreen, pCurs, displayed);
    else
        xf86RecolorCursor(pScreen, pCurs, displayed);
}

/***** ScrnInfoRec functions *********/

static void
xf86CursorEnableDisableFBAccess(ScrnInfoPtr pScrn, Bool enable)
{
    DeviceIntPtr pDev = inputInfo.pointer;

    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    if (!enable && ScreenPriv->CurrentCursor != NullCursor) {
        CursorPtr currentCursor = RefCursor(ScreenPriv->CurrentCursor);

        xf86CursorSetCursor(pDev, pScreen, NullCursor, ScreenPriv->x,
                            ScreenPriv->y);
        ScreenPriv->isUp = FALSE;
        ScreenPriv->SWCursor = TRUE;
        ScreenPriv->SavedCursor = currentCursor;
    }

    if (ScreenPriv->EnableDisableFBAccess)
        (*ScreenPriv->EnableDisableFBAccess) (pScrn, enable);

    if (enable && ScreenPriv->SavedCursor) {
        /*
         * Re-set current cursor so drivers can react to FB access having been
         * temporarily disabled.
         */
        xf86CursorSetCursor(pDev, pScreen, ScreenPriv->SavedCursor,
                            ScreenPriv->x, ScreenPriv->y);
        UnrefCursor(ScreenPriv->SavedCursor);
        ScreenPriv->SavedCursor = NULL;
    }
}

static Bool
xf86CursorSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    Bool ret;
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    if (ScreenPriv->isUp) {
        xf86SetCursor(pScreen, NullCursor, ScreenPriv->x, ScreenPriv->y);
        ScreenPriv->isUp = FALSE;
    }

    ret = (*ScreenPriv->SwitchMode) (pScrn, mode);

    /*
     * Cannot restore cursor here because the new frame[XY][01] haven't been
     * calculated yet.  However, because the hardware cursor was removed above,
     * ensure the cursor is repainted by miPointerWarpCursor().
     */
    ScreenPriv->CursorToRestore = ScreenPriv->CurrentCursor;
    miPointerSetWaitForUpdate(pScreen, FALSE);  /* Force cursor repaint */

    return ret;
}

/****** miPointerSpriteFunctions *******/

static Bool
xf86CursorRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCurs)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    if (CursorRefCount(pCurs) <= 1)
        dixSetScreenPrivate(&pCurs->devPrivates, CursorScreenKey, pScreen,
                            NULL);

    return (*ScreenPriv->spriteFuncs->RealizeCursor) (pDev, pScreen, pCurs);
}

static Bool
xf86CursorUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCurs)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    if (CursorRefCount(pCurs) <= 1) {
        free(dixLookupScreenPrivate
             (&pCurs->devPrivates, CursorScreenKey, pScreen));
        dixSetScreenPrivate(&pCurs->devPrivates, CursorScreenKey, pScreen,
                            NULL);
    }

    return (*ScreenPriv->spriteFuncs->UnrealizeCursor) (pDev, pScreen, pCurs);
}

static void
xf86CursorSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCurs,
                    int x, int y)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);
    xf86CursorInfoPtr infoPtr = ScreenPriv->CursorInfoPtr;

    if (pCurs == NullCursor) {  /* means we're supposed to remove the cursor */
        if (ScreenPriv->SWCursor ||
            !(GetMaster(pDev, MASTER_POINTER) == inputInfo.pointer))
            (*ScreenPriv->spriteFuncs->SetCursor) (pDev, pScreen, NullCursor, x,
                                                   y);
        else if (ScreenPriv->isUp) {
            xf86SetCursor(pScreen, NullCursor, x, y);
            ScreenPriv->isUp = FALSE;
        }
        if (ScreenPriv->CurrentCursor)
            FreeCursor(ScreenPriv->CurrentCursor, None);
        ScreenPriv->CurrentCursor = NullCursor;
        return;
    }

    /* only update for VCP, otherwise we get cursor jumps when removing a
       sprite. The second cursor is never HW rendered anyway. */
    if (GetMaster(pDev, MASTER_POINTER) == inputInfo.pointer) {
        CursorPtr cursor = RefCursor(pCurs);
        if (ScreenPriv->CurrentCursor)
            FreeCursor(ScreenPriv->CurrentCursor, None);
        ScreenPriv->CurrentCursor = cursor;
        ScreenPriv->x = x;
        ScreenPriv->y = y;
        ScreenPriv->CursorToRestore = NULL;
        ScreenPriv->HotX = cursor->bits->xhot;
        ScreenPriv->HotY = cursor->bits->yhot;

        if (!infoPtr->pScrn->vtSema) {
            cursor = RefCursor(cursor);
            if (ScreenPriv->SavedCursor)
                FreeCursor(ScreenPriv->SavedCursor, None);
            ScreenPriv->SavedCursor = cursor;
            return;
        }

        if (infoPtr->pScrn->vtSema &&
            (ScreenPriv->ForceHWCursorCount ||
             xf86CheckHWCursor(pScreen, cursor, infoPtr))) {

            if (ScreenPriv->SWCursor)   /* remove the SW cursor */
                (*ScreenPriv->spriteFuncs->SetCursor) (pDev, pScreen,
                                                       NullCursor, x, y);

            if (xf86SetCursor(pScreen, cursor, x, y)) {
                ScreenPriv->SWCursor = FALSE;
                ScreenPriv->isUp = TRUE;

                miPointerSetWaitForUpdate(pScreen, !infoPtr->pScrn->silkenMouse);
                return;
            }
        }

        miPointerSetWaitForUpdate(pScreen, TRUE);

        if (ScreenPriv->isUp) {
            /* Remove the HW cursor, or make it transparent */
            if (infoPtr->Flags & HARDWARE_CURSOR_SHOW_TRANSPARENT) {
                xf86SetTransparentCursor(pScreen);
            }
            else {
                xf86SetCursor(pScreen, NullCursor, x, y);
                ScreenPriv->isUp = FALSE;
            }
        }

        if (!ScreenPriv->SWCursor)
            ScreenPriv->SWCursor = TRUE;

    }

    if (pCurs->bits->emptyMask && !ScreenPriv->showTransparent)
        pCurs = NullCursor;

    (*ScreenPriv->spriteFuncs->SetCursor) (pDev, pScreen, pCurs, x, y);
}

/* Re-set the current cursor. This will switch between hardware and software
 * cursor depending on whether hardware cursor is currently supported
 * according to the driver.
 */
void
xf86CursorResetCursor(ScreenPtr pScreen)
{
    xf86CursorScreenPtr ScreenPriv;

    if (!inputInfo.pointer)
        return;

    if (!dixPrivateKeyRegistered(xf86CursorScreenKey))
        return;

    ScreenPriv = (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                        xf86CursorScreenKey);
    if (!ScreenPriv)
        return;

    xf86CursorSetCursor(inputInfo.pointer, pScreen, ScreenPriv->CurrentCursor,
                        ScreenPriv->x, ScreenPriv->y);
}

static void
xf86CursorMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    /* only update coordinate state for first sprite, otherwise we get jumps
       when removing a sprite. The second sprite is never HW rendered anyway */
    if (GetMaster(pDev, MASTER_POINTER) == inputInfo.pointer) {
        ScreenPriv->x = x;
        ScreenPriv->y = y;

        if (ScreenPriv->CursorToRestore)
            xf86CursorSetCursor(pDev, pScreen, ScreenPriv->CursorToRestore, x,
                                y);
        else if (ScreenPriv->SWCursor)
            (*ScreenPriv->spriteFuncs->MoveCursor) (pDev, pScreen, x, y);
        else if (ScreenPriv->isUp)
            xf86MoveCursor(pScreen, x, y);
    }
    else
        (*ScreenPriv->spriteFuncs->MoveCursor) (pDev, pScreen, x, y);
}

void
xf86ForceHWCursor(ScreenPtr pScreen, Bool on)
{
    DeviceIntPtr pDev = inputInfo.pointer;
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    if (on) {
        if (ScreenPriv->ForceHWCursorCount++ == 0) {
            if (ScreenPriv->SWCursor && ScreenPriv->CurrentCursor) {
                ScreenPriv->HWCursorForced = TRUE;
                xf86CursorSetCursor(pDev, pScreen, ScreenPriv->CurrentCursor,
                                    ScreenPriv->x, ScreenPriv->y);
            }
            else
                ScreenPriv->HWCursorForced = FALSE;
        }
    }
    else {
        if (--ScreenPriv->ForceHWCursorCount == 0) {
            if (ScreenPriv->HWCursorForced && ScreenPriv->CurrentCursor)
                xf86CursorSetCursor(pDev, pScreen, ScreenPriv->CurrentCursor,
                                    ScreenPriv->x, ScreenPriv->y);
        }
    }
}

CursorPtr
xf86CurrentCursor(ScreenPtr pScreen)
{
    xf86CursorScreenPtr ScreenPriv;

    if (pScreen->is_output_secondary)
        pScreen = pScreen->current_primary;

    ScreenPriv = dixLookupPrivate(&pScreen->devPrivates, xf86CursorScreenKey);
    return ScreenPriv->CurrentCursor;
}

xf86CursorInfoPtr
xf86CreateCursorInfoRec(void)
{
    return calloc(1, sizeof(xf86CursorInfoRec));
}

void
xf86DestroyCursorInfoRec(xf86CursorInfoPtr infoPtr)
{
    free(infoPtr);
}

/**
 * New cursor has been created. Do your initalizations here.
 */
static Bool
xf86DeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    int ret;
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    /* Init SW cursor */
    ret = (*ScreenPriv->spriteFuncs->DeviceCursorInitialize) (pDev, pScreen);

    return ret;
}

/**
 * Cursor has been removed. Clean up after yourself.
 */
static void
xf86DeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    /* Clean up SW cursor */
    (*ScreenPriv->spriteFuncs->DeviceCursorCleanup) (pDev, pScreen);
}
