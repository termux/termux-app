/*

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
*/

/**
 * @file
 * This file contains functions to move the pointer on the screen and/or
 * restrict its movement. These functions are divided into two sets:
 * Screen-specific functions that are used as function pointers from other
 * parts of the server (and end up heavily wrapped by e.g. animcur and
 * xfixes):
 *      miPointerConstrainCursor
 *      miPointerCursorLimits
 *      miPointerDisplayCursor
 *      miPointerRealizeCursor
 *      miPointerUnrealizeCursor
 *      miPointerSetCursorPosition
 *      miRecolorCursor
 *      miPointerDeviceInitialize
 *      miPointerDeviceCleanup
 * If wrapped, these are the last element in the wrapping chain. They may
 * call into sprite-specific code through further function pointers though.
 *
 * The second type of functions are those that are directly called by the
 * DIX, DDX and some drivers.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include   <X11/X.h>
#include   <X11/Xmd.h>
#include   <X11/Xproto.h>
#include   "misc.h"
#include   "windowstr.h"
#include   "pixmapstr.h"
#include   "mi.h"
#include   "scrnintstr.h"
#include   "mipointrst.h"
#include   "cursorstr.h"
#include   "dixstruct.h"
#include   "inputstr.h"
#include   "inpututils.h"
#include   "eventstr.h"

typedef struct {
    ScreenPtr pScreen;          /* current screen */
    ScreenPtr pSpriteScreen;    /* screen containing current sprite */
    CursorPtr pCursor;          /* current cursor */
    CursorPtr pSpriteCursor;    /* cursor on screen */
    BoxRec limits;              /* current constraints */
    Bool confined;              /* pointer can't change screens */
    int x, y;                   /* hot spot location */
    int devx, devy;             /* sprite position */
    Bool generateEvent;         /* generate an event during warping? */
} miPointerRec, *miPointerPtr;

DevPrivateKeyRec miPointerScreenKeyRec;

#define GetScreenPrivate(s) ((miPointerScreenPtr) \
    dixLookupPrivate(&(s)->devPrivates, miPointerScreenKey))
#define SetupScreen(s)	miPointerScreenPtr  pScreenPriv = GetScreenPrivate(s)

DevPrivateKeyRec miPointerPrivKeyRec;

#define MIPOINTER(dev) \
    (IsFloating(dev) ? \
        (miPointerPtr)dixLookupPrivate(&(dev)->devPrivates, miPointerPrivKey): \
        (miPointerPtr)dixLookupPrivate(&(GetMaster(dev, MASTER_POINTER))->devPrivates, miPointerPrivKey))

static Bool miPointerRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                   CursorPtr pCursor);
static Bool miPointerUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                     CursorPtr pCursor);
static Bool miPointerDisplayCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                   CursorPtr pCursor);
static void miPointerConstrainCursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                     BoxPtr pBox);
static void miPointerCursorLimits(DeviceIntPtr pDev, ScreenPtr pScreen,
                                  CursorPtr pCursor, BoxPtr pHotBox,
                                  BoxPtr pTopLeftBox);
static Bool miPointerSetCursorPosition(DeviceIntPtr pDev, ScreenPtr pScreen,
                                       int x, int y, Bool generateEvent);
static Bool miPointerCloseScreen(ScreenPtr pScreen);
static void miPointerMove(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y);
static Bool miPointerDeviceInitialize(DeviceIntPtr pDev, ScreenPtr pScreen);
static void miPointerDeviceCleanup(DeviceIntPtr pDev, ScreenPtr pScreen);
static void miPointerMoveNoEvent(DeviceIntPtr pDev, ScreenPtr pScreen, int x,
                                 int y);

static InternalEvent *mipointermove_events;   /* for WarpPointer MotionNotifies */

Bool
miPointerInitialize(ScreenPtr pScreen,
                    miPointerSpriteFuncPtr spriteFuncs,
                    miPointerScreenFuncPtr screenFuncs, Bool waitForUpdate)
{
    miPointerScreenPtr pScreenPriv;

    if (!dixRegisterPrivateKey(&miPointerScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (!dixRegisterPrivateKey(&miPointerPrivKeyRec, PRIVATE_DEVICE, 0))
        return FALSE;

    pScreenPriv = malloc(sizeof(miPointerScreenRec));
    if (!pScreenPriv)
        return FALSE;
    pScreenPriv->spriteFuncs = spriteFuncs;
    pScreenPriv->screenFuncs = screenFuncs;
    pScreenPriv->waitForUpdate = waitForUpdate;
    pScreenPriv->showTransparent = FALSE;
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = miPointerCloseScreen;
    dixSetPrivate(&pScreen->devPrivates, miPointerScreenKey, pScreenPriv);
    /*
     * set up screen cursor method table
     */
    pScreen->ConstrainCursor = miPointerConstrainCursor;
    pScreen->CursorLimits = miPointerCursorLimits;
    pScreen->DisplayCursor = miPointerDisplayCursor;
    pScreen->RealizeCursor = miPointerRealizeCursor;
    pScreen->UnrealizeCursor = miPointerUnrealizeCursor;
    pScreen->SetCursorPosition = miPointerSetCursorPosition;
    pScreen->RecolorCursor = miRecolorCursor;
    pScreen->DeviceCursorInitialize = miPointerDeviceInitialize;
    pScreen->DeviceCursorCleanup = miPointerDeviceCleanup;

    mipointermove_events = NULL;
    return TRUE;
}

/**
 * Destroy screen-specific information.
 *
 * @param index Screen index of the screen in screenInfo.screens[]
 * @param pScreen The actual screen pointer
 */
static Bool
miPointerCloseScreen(ScreenPtr pScreen)
{
    SetupScreen(pScreen);

    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    free((void *) pScreenPriv);
    FreeEventList(mipointermove_events, GetMaximumEventsNum());
    mipointermove_events = NULL;
    return (*pScreen->CloseScreen) (pScreen);
}

/*
 * DIX/DDX interface routines
 */

static Bool
miPointerRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    SetupScreen(pScreen);
    return (*pScreenPriv->spriteFuncs->RealizeCursor) (pDev, pScreen, pCursor);
}

static Bool
miPointerUnrealizeCursor(DeviceIntPtr pDev,
                         ScreenPtr pScreen, CursorPtr pCursor)
{
    SetupScreen(pScreen);
    return (*pScreenPriv->spriteFuncs->UnrealizeCursor) (pDev, pScreen,
                                                         pCursor);
}

static Bool
miPointerDisplayCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    miPointerPtr pPointer;

    /* return for keyboards */
    if (!IsPointerDevice(pDev))
        return FALSE;

    pPointer = MIPOINTER(pDev);

    pPointer->pCursor = pCursor;
    pPointer->pScreen = pScreen;
    miPointerUpdateSprite(pDev);
    return TRUE;
}

/**
 * Set up the constraints for the given device. This function does not
 * actually constrain the cursor but merely copies the given box to the
 * internal constraint storage.
 *
 * @param pDev The device to constrain to the box
 * @param pBox The rectangle to constrain the cursor to
 * @param pScreen Used for copying screen confinement
 */
static void
miPointerConstrainCursor(DeviceIntPtr pDev, ScreenPtr pScreen, BoxPtr pBox)
{
    miPointerPtr pPointer;

    pPointer = MIPOINTER(pDev);

    pPointer->limits = *pBox;
    pPointer->confined = PointerConfinedToScreen(pDev);
}

/**
 * Should calculate the box for the given cursor, based on screen and the
 * confinement given. But we assume that whatever box is passed in is valid
 * anyway.
 *
 * @param pDev The device to calculate the cursor limits for
 * @param pScreen The screen the confinement happens on
 * @param pCursor The screen the confinement happens on
 * @param pHotBox The confinement box for the cursor
 * @param[out] pTopLeftBox The new confinement box, always *pHotBox.
 */
static void
miPointerCursorLimits(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor,
                      BoxPtr pHotBox, BoxPtr pTopLeftBox)
{
    *pTopLeftBox = *pHotBox;
}

/**
 * Set the device's cursor position to the x/y position on the given screen.
 * Generates and event if required.
 *
 * This function is called from:
 *    - sprite init code to place onto initial position
 *    - the various WarpPointer implementations (core, XI, Xinerama, dmx,…)
 *    - during the cursor update path in CheckMotion
 *    - in the Xinerama part of NewCurrentScreen
 *    - when a RandR/RandR1.2 mode was applied (it may have moved the pointer, so
 *      it's set back to the original pos)
 *
 * @param pDev The device to move
 * @param pScreen The screen the device is on
 * @param x The x coordinate in per-screen coordinates
 * @param y The y coordinate in per-screen coordinates
 * @param generateEvent True if the pointer movement should generate an
 * event.
 *
 * @return TRUE in all cases
 */
static Bool
miPointerSetCursorPosition(DeviceIntPtr pDev, ScreenPtr pScreen,
                           int x, int y, Bool generateEvent)
{
    SetupScreen(pScreen);
    miPointerPtr pPointer = MIPOINTER(pDev);

    pPointer->generateEvent = generateEvent;

    if (pScreen->ConstrainCursorHarder)
        pScreen->ConstrainCursorHarder(pDev, pScreen, Absolute, &x, &y);

    /* device dependent - must pend signal and call miPointerWarpCursor */
    (*pScreenPriv->screenFuncs->WarpCursor) (pDev, pScreen, x, y);
    if (!generateEvent)
        miPointerUpdateSprite(pDev);
    return TRUE;
}

void
miRecolorCursor(DeviceIntPtr pDev, ScreenPtr pScr,
                CursorPtr pCurs, Bool displayed)
{
    /*
     * This is guaranteed to correct any color-dependent state which may have
     * been bound up in private state created by RealizeCursor
     */
    pScr->UnrealizeCursor(pDev, pScr, pCurs);
    pScr->RealizeCursor(pDev, pScr, pCurs);
    if (displayed)
        pScr->DisplayCursor(pDev, pScr, pCurs);
}

/**
 * Set up sprite information for the device.
 * This function will be called once for each device after it is initialized
 * in the DIX.
 *
 * @param pDev The newly created device
 * @param pScreen The initial sprite scree.
 */
static Bool
miPointerDeviceInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    miPointerPtr pPointer;

    SetupScreen(pScreen);

    pPointer = malloc(sizeof(miPointerRec));
    if (!pPointer)
        return FALSE;

    pPointer->pScreen = NULL;
    pPointer->pSpriteScreen = NULL;
    pPointer->pCursor = NULL;
    pPointer->pSpriteCursor = NULL;
    pPointer->limits.x1 = 0;
    pPointer->limits.x2 = 32767;
    pPointer->limits.y1 = 0;
    pPointer->limits.y2 = 32767;
    pPointer->confined = FALSE;
    pPointer->x = 0;
    pPointer->y = 0;
    pPointer->generateEvent = FALSE;

    if (!((*pScreenPriv->spriteFuncs->DeviceCursorInitialize) (pDev, pScreen))) {
        free(pPointer);
        return FALSE;
    }

    dixSetPrivate(&pDev->devPrivates, miPointerPrivKey, pPointer);
    return TRUE;
}

/**
 * Clean up after device.
 * This function will be called once before the device is freed in the DIX
 *
 * @param pDev The device to be removed from the server
 * @param pScreen Current screen of the device
 */
static void
miPointerDeviceCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    SetupScreen(pScreen);

    if (!IsMaster(pDev) && !IsFloating(pDev))
        return;

    (*pScreenPriv->spriteFuncs->DeviceCursorCleanup) (pDev, pScreen);
    free(dixLookupPrivate(&pDev->devPrivates, miPointerPrivKey));
    dixSetPrivate(&pDev->devPrivates, miPointerPrivKey, NULL);
}

/**
 * Warp the pointer to the given position on the given screen. May generate
 * an event, depending on whether we're coming from miPointerSetPosition.
 *
 * Once signals are ignored, the WarpCursor function can call this
 *
 * @param pDev The device to warp
 * @param pScreen Screen to warp on
 * @param x The x coordinate in per-screen coordinates
 * @param y The y coordinate in per-screen coordinates
 */

void
miPointerWarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    miPointerPtr pPointer;
    BOOL changedScreen = FALSE;

    pPointer = MIPOINTER(pDev);

    if (pPointer->pScreen != pScreen) {
        mieqSwitchScreen(pDev, pScreen, TRUE);
        changedScreen = TRUE;
    }

    if (pPointer->generateEvent)
        miPointerMove(pDev, pScreen, x, y);
    else
        miPointerMoveNoEvent(pDev, pScreen, x, y);

    /* Don't call USFS if we use Xinerama, otherwise the root window is
     * updated to the second screen, and we never receive any events.
     * (FDO bug #18668) */
    if (changedScreen
#ifdef PANORAMIX
        && noPanoramiXExtension
#endif
        )
        UpdateSpriteForScreen(pDev, pScreen);
}

/**
 * Synchronize the sprite with the cursor.
 *
 * @param pDev The device to sync
 */
void
miPointerUpdateSprite(DeviceIntPtr pDev)
{
    ScreenPtr pScreen;
    miPointerScreenPtr pScreenPriv;
    CursorPtr pCursor;
    int x, y, devx, devy;
    miPointerPtr pPointer;

    if (!pDev || !pDev->coreEvents)
        return;

    pPointer = MIPOINTER(pDev);

    if (!pPointer)
        return;

    pScreen = pPointer->pScreen;
    if (!pScreen)
        return;

    x = pPointer->x;
    y = pPointer->y;
    devx = pPointer->devx;
    devy = pPointer->devy;

    pScreenPriv = GetScreenPrivate(pScreen);
    /*
     * if the cursor has switched screens, disable the sprite
     * on the old screen
     */
    if (pScreen != pPointer->pSpriteScreen) {
        if (pPointer->pSpriteScreen) {
            miPointerScreenPtr pOldPriv;

            pOldPriv = GetScreenPrivate(pPointer->pSpriteScreen);
            if (pPointer->pCursor) {
                (*pOldPriv->spriteFuncs->SetCursor)
                    (pDev, pPointer->pSpriteScreen, NullCursor, 0, 0);
            }
            (*pOldPriv->screenFuncs->CrossScreen) (pPointer->pSpriteScreen,
                                                   FALSE);
        }
        (*pScreenPriv->screenFuncs->CrossScreen) (pScreen, TRUE);
        (*pScreenPriv->spriteFuncs->SetCursor)
            (pDev, pScreen, pPointer->pCursor, x, y);
        pPointer->devx = x;
        pPointer->devy = y;
        pPointer->pSpriteCursor = pPointer->pCursor;
        pPointer->pSpriteScreen = pScreen;
    }
    /*
     * if the cursor has changed, display the new one
     */
    else if (pPointer->pCursor != pPointer->pSpriteCursor) {
        pCursor = pPointer->pCursor;
        if (!pCursor ||
            (pCursor->bits->emptyMask && !pScreenPriv->showTransparent))
            pCursor = NullCursor;
        (*pScreenPriv->spriteFuncs->SetCursor) (pDev, pScreen, pCursor, x, y);

        pPointer->devx = x;
        pPointer->devy = y;
        pPointer->pSpriteCursor = pPointer->pCursor;
    }
    else if (x != devx || y != devy) {
        pPointer->devx = x;
        pPointer->devy = y;
        if (pPointer->pCursor && !pPointer->pCursor->bits->emptyMask)
            (*pScreenPriv->spriteFuncs->MoveCursor) (pDev, pScreen, x, y);
    }
}

/**
 * Invalidate the current sprite and force it to be reloaded on next cursor setting
 * operation
 *
 * @param pDev The device to invalidate the sprite fore
 */
void
miPointerInvalidateSprite(DeviceIntPtr pDev)
{
    miPointerPtr pPointer;

    pPointer = MIPOINTER(pDev);
    pPointer->pSpriteCursor = (CursorPtr) 1;
}

/**
 * Set the device to the coordinates on the given screen.
 *
 * @param pDev The device to move
 * @param screen_no Index of the screen to move to
 * @param x The x coordinate in per-screen coordinates
 * @param y The y coordinate in per-screen coordinates
 */
void
miPointerSetScreen(DeviceIntPtr pDev, int screen_no, int x, int y)
{
    ScreenPtr pScreen;
    miPointerPtr pPointer;

    pPointer = MIPOINTER(pDev);

    pScreen = screenInfo.screens[screen_no];
    mieqSwitchScreen(pDev, pScreen, FALSE);
    NewCurrentScreen(pDev, pScreen, x, y);

    pPointer->limits.x2 = pScreen->width;
    pPointer->limits.y2 = pScreen->height;
}

/**
 * @return The current screen of the given device or NULL.
 */
ScreenPtr
miPointerGetScreen(DeviceIntPtr pDev)
{
    miPointerPtr pPointer = MIPOINTER(pDev);

    return (pPointer) ? pPointer->pScreen : NULL;
}

/* Controls whether the cursor image should be updated immediately when
   moved (FALSE) or if something else will be responsible for updating
   it later (TRUE).  Returns current setting.
   Caller is responsible for calling OsBlockSignal first.
*/
Bool
miPointerSetWaitForUpdate(ScreenPtr pScreen, Bool wait)
{
    SetupScreen(pScreen);
    Bool prevWait = pScreenPriv->waitForUpdate;

    pScreenPriv->waitForUpdate = wait;
    return prevWait;
}

/* Move the pointer on the current screen,  and update the sprite. */
static void
miPointerMoveNoEvent(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    miPointerPtr pPointer;

    SetupScreen(pScreen);

    pPointer = MIPOINTER(pDev);

    /* Hack: We mustn't call into ->MoveCursor for anything but the
     * VCP, as this may cause a non-HW rendered cursor to be rendered while
     * not holding the input lock. This would race with building the command
     * buffer for other rendering.
     */
    if (GetMaster(pDev, MASTER_POINTER) == inputInfo.pointer
        &&!pScreenPriv->waitForUpdate && pScreen == pPointer->pSpriteScreen) {
        pPointer->devx = x;
        pPointer->devy = y;
        if (pPointer->pCursor && !pPointer->pCursor->bits->emptyMask)
            (*pScreenPriv->spriteFuncs->MoveCursor) (pDev, pScreen, x, y);
    }

    pPointer->x = x;
    pPointer->y = y;
    pPointer->pScreen = pScreen;
}

/**
 * Set the devices' cursor position to the given x/y position.
 *
 * This function is called during the pointer update path in
 * GetPointerEvents and friends (and the same in the xwin DDX).
 *
 * The coordinates provided are always absolute. The parameter mode whether
 * it was relative or absolute movement that landed us at those coordinates.
 *
 * If the cursor was constrained by a barrier, ET_Barrier* events may be
 * generated and appended to the InternalEvent list provided.
 *
 * @param pDev The device to move
 * @param mode Movement mode (Absolute or Relative)
 * @param[in,out] screenx The x coordinate in desktop coordinates
 * @param[in,out] screeny The y coordinate in desktop coordinates
 * @param[in,out] nevents The number of events in events (before/after)
 * @param[in,out] events The list of events before/after being constrained
 */
ScreenPtr
miPointerSetPosition(DeviceIntPtr pDev, int mode, double *screenx,
                     double *screeny,
                     int *nevents, InternalEvent* events)
{
    miPointerScreenPtr pScreenPriv;
    ScreenPtr pScreen;
    ScreenPtr newScreen;
    int x, y;
    Bool switch_screen = FALSE;
    Bool should_constrain_barriers = FALSE;
    int i;

    miPointerPtr pPointer;

    pPointer = MIPOINTER(pDev);
    pScreen = pPointer->pScreen;

    x = trunc(*screenx);
    y = trunc(*screeny);

    switch_screen = !point_on_screen(pScreen, x, y);

    /* Switch to per-screen coordinates for CursorOffScreen and
     * Pointer->limits */
    x -= pScreen->x;
    y -= pScreen->y;

    should_constrain_barriers = (mode == Relative);

    if (should_constrain_barriers) {
        /* coordinates after clamped to a barrier */
        int constrained_x, constrained_y;
        int current_x, current_y; /* current position in per-screen coord */

        current_x = MIPOINTER(pDev)->x - pScreen->x;
        current_y = MIPOINTER(pDev)->y - pScreen->y;

        input_constrain_cursor(pDev, pScreen,
                               current_x, current_y, x, y,
                               &constrained_x, &constrained_y,
                               nevents, events);

        x = constrained_x;
        y = constrained_y;
    }

    if (switch_screen) {
        pScreenPriv = GetScreenPrivate(pScreen);
        if (!pPointer->confined) {
            newScreen = pScreen;
            (*pScreenPriv->screenFuncs->CursorOffScreen) (&newScreen, &x, &y);
            if (newScreen != pScreen) {
                pScreen = newScreen;
                mieqSwitchScreen(pDev, pScreen, FALSE);
                /* Smash the confine to the new screen */
                pPointer->limits.x2 = pScreen->width;
                pPointer->limits.y2 = pScreen->height;
            }
        }
    }
    /* Constrain the sprite to the current limits. */
    if (x < pPointer->limits.x1)
        x = pPointer->limits.x1;
    if (x >= pPointer->limits.x2)
        x = pPointer->limits.x2 - 1;
    if (y < pPointer->limits.y1)
        y = pPointer->limits.y1;
    if (y >= pPointer->limits.y2)
        y = pPointer->limits.y2 - 1;

    if (pScreen->ConstrainCursorHarder)
        pScreen->ConstrainCursorHarder(pDev, pScreen, mode, &x, &y);

    if (pPointer->x != x || pPointer->y != y || pPointer->pScreen != pScreen)
        miPointerMoveNoEvent(pDev, pScreen, x, y);

    /* check if we generated any barrier events and if so, update root x/y
     * to the fully constrained coords */
    if (should_constrain_barriers) {
        for (i = 0; i < *nevents; i++) {
            if (events[i].any.type == ET_BarrierHit ||
                events[i].any.type == ET_BarrierLeave) {
                events[i].barrier_event.root_x = x;
                events[i].barrier_event.root_y = y;
            }
        }
    }

    /* Convert to desktop coordinates again */
    x += pScreen->x;
    y += pScreen->y;

    /* In the event we actually change screen or we get confined, we just
     * drop the float component on the floor
     * FIXME: only drop remainder for ConstrainCursorHarder, not for screen
     * crossings */
    if (x != trunc(*screenx))
        *screenx = x;
    if (y != trunc(*screeny))
        *screeny = y;

    return pScreen;
}

/**
 * Get the current position of the device in desktop coordinates.
 *
 * @param x Return value for the current x coordinate in desktop coordinates.
 * @param y Return value for the current y coordinate in desktop coordinates.
 */
void
miPointerGetPosition(DeviceIntPtr pDev, int *x, int *y)
{
    *x = MIPOINTER(pDev)->x;
    *y = MIPOINTER(pDev)->y;
}

/**
 * Move the device's pointer to the x/y coordinates on the given screen.
 * This function generates and enqueues pointer events.
 *
 * @param pDev The device to move
 * @param pScreen The screen the device is on
 * @param x The x coordinate in per-screen coordinates
 * @param y The y coordinate in per-screen coordinates
 */
void
miPointerMove(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    int i, nevents;
    int valuators[2];
    ValuatorMask mask;

    miPointerMoveNoEvent(pDev, pScreen, x, y);

    /* generate motion notify */
    valuators[0] = x;
    valuators[1] = y;

    if (!mipointermove_events) {
        mipointermove_events = InitEventList(GetMaximumEventsNum());

        if (!mipointermove_events) {
            FatalError("Could not allocate event store.\n");
            return;
        }
    }

    valuator_mask_set_range(&mask, 0, 2, valuators);
    nevents = GetPointerEvents(mipointermove_events, pDev, MotionNotify, 0,
                               POINTER_SCREEN | POINTER_ABSOLUTE |
                               POINTER_NORAW, &mask);

    input_lock();
    for (i = 0; i < nevents; i++)
        mieqEnqueue(pDev, &mipointermove_events[i]);
    input_unlock();
}
