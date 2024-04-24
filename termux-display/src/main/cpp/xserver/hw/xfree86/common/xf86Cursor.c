/*
 * Copyright (c) 1994-2003 by The XFree86 Project, Inc.
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

#include <X11/X.h>
#include <X11/Xmd.h>
#include "input.h"
#include "cursor.h"
#include "mipointer.h"
#include "scrnintstr.h"
#include "globals.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSproc.h"

#include <X11/extensions/XIproto.h>
#include "xf86Xinput.h"

#ifdef XFreeXDGA
#include "dgaproc.h"
#endif

typedef struct _xf86EdgeRec {
    short screen;
    short start;
    short end;
    DDXPointRec offset;
    struct _xf86EdgeRec *next;
} xf86EdgeRec, *xf86EdgePtr;

typedef struct {
    xf86EdgePtr left, right, up, down;
} xf86ScreenLayoutRec, *xf86ScreenLayoutPtr;

static Bool xf86CursorOffScreen(ScreenPtr *pScreen, int *x, int *y);
static void xf86CrossScreen(ScreenPtr pScreen, Bool entering);
static void xf86WarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y);

static void xf86PointerMoved(ScrnInfoPtr pScrn, int x, int y);

static miPointerScreenFuncRec xf86PointerScreenFuncs = {
    xf86CursorOffScreen,
    xf86CrossScreen,
    xf86WarpCursor,
};

static xf86ScreenLayoutRec xf86ScreenLayout[MAXSCREENS];

/*
 * xf86InitViewport --
 *      Initialize paning & zooming parameters, so that a driver must only
 *      check what resolutions are possible and whether the virtual area
 *      is valid if specified.
 */

void
xf86InitViewport(ScrnInfoPtr pScr)
{

    pScr->PointerMoved = xf86PointerMoved;

    /*
     * Compute the initial Viewport if necessary
     */
    if (pScr->display) {
        if (pScr->display->frameX0 < 0) {
            pScr->frameX0 = (pScr->virtualX - pScr->modes->HDisplay) / 2;
            pScr->frameY0 = (pScr->virtualY - pScr->modes->VDisplay) / 2;
        }
        else {
            pScr->frameX0 = pScr->display->frameX0;
            pScr->frameY0 = pScr->display->frameY0;
        }
    }

    pScr->frameX1 = pScr->frameX0 + pScr->modes->HDisplay - 1;
    pScr->frameY1 = pScr->frameY0 + pScr->modes->VDisplay - 1;

    /*
     * Now adjust the initial Viewport, so it lies within the virtual area
     */
    if (pScr->frameX1 >= pScr->virtualX) {
        pScr->frameX0 = pScr->virtualX - pScr->modes->HDisplay;
        pScr->frameX1 = pScr->frameX0 + pScr->modes->HDisplay - 1;
    }

    if (pScr->frameY1 >= pScr->virtualY) {
        pScr->frameY0 = pScr->virtualY - pScr->modes->VDisplay;
        pScr->frameY1 = pScr->frameY0 + pScr->modes->VDisplay - 1;
    }
}

/*
 * xf86SetViewport --
 *      Scroll the visual part of the screen so the pointer is visible.
 */

void
xf86SetViewport(ScreenPtr pScreen, int x, int y)
{
    ScrnInfoPtr pScr = xf86ScreenToScrn(pScreen);

    (*pScr->PointerMoved) (pScr, x, y);
}

static void
xf86PointerMoved(ScrnInfoPtr pScr, int x, int y)
{
    Bool frameChanged = FALSE;

    /*
     * check whether (x,y) belongs to the visual part of the screen
     * if not, change the base of the displayed frame occurring
     */
    if (pScr->frameX0 > x) {
        pScr->frameX0 = x;
        pScr->frameX1 = x + pScr->currentMode->HDisplay - 1;
        frameChanged = TRUE;
    }

    if (pScr->frameX1 < x) {
        pScr->frameX1 = x + 1;
        pScr->frameX0 = x - pScr->currentMode->HDisplay + 1;
        frameChanged = TRUE;
    }

    if (pScr->frameY0 > y) {
        pScr->frameY0 = y;
        pScr->frameY1 = y + pScr->currentMode->VDisplay - 1;
        frameChanged = TRUE;
    }

    if (pScr->frameY1 < y) {
        pScr->frameY1 = y;
        pScr->frameY0 = y - pScr->currentMode->VDisplay + 1;
        frameChanged = TRUE;
    }

    if (frameChanged && pScr->AdjustFrame != NULL)
        pScr->AdjustFrame(pScr, pScr->frameX0, pScr->frameY0);
}

/*
 * xf86LockZoom --
 *	Enable/disable ZoomViewport
 */

void
xf86LockZoom(ScreenPtr pScreen, Bool lock)
{
    ScrnInfoPtr pScr = xf86ScreenToScrn(pScreen);
    pScr->zoomLocked = lock;
}

/*
 * xf86SwitchMode --
 *	This is called by both keyboard processing and the VidMode extension to
 *	set a new mode.
 */

Bool
xf86SwitchMode(ScreenPtr pScreen, DisplayModePtr mode)
{
    ScrnInfoPtr pScr = xf86ScreenToScrn(pScreen);
    ScreenPtr pCursorScreen;
    Bool Switched;
    int px, py;
    DeviceIntPtr dev, it;

    if (!pScr->vtSema || !mode || !pScr->SwitchMode)
        return FALSE;

#ifdef XFreeXDGA
    if (DGAActive(pScr->scrnIndex))
        return FALSE;
#endif

    if (mode == pScr->currentMode)
        return TRUE;

    if (mode->HDisplay > pScr->virtualX || mode->VDisplay > pScr->virtualY)
        return FALSE;

    /* Let's take an educated guess for which pointer to take here. And about as
       educated as it gets is to take the first pointer we find.
     */
    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (IsPointerDevice(dev) && dev->spriteInfo->spriteOwner)
            break;
    }

    pCursorScreen = miPointerGetScreen(dev);
    if (pScreen == pCursorScreen)
        miPointerGetPosition(dev, &px, &py);

    input_lock();
    Switched = (*pScr->SwitchMode) (pScr, mode);
    if (Switched) {
        pScr->currentMode = mode;

        /*
         * Adjust frame for new display size.
         * Frame is centered around cursor position if cursor is on same screen.
         */
        if (pScreen == pCursorScreen)
            pScr->frameX0 = px - (mode->HDisplay / 2) + 1;
        else
            pScr->frameX0 =
                (pScr->frameX0 + pScr->frameX1 + 1 - mode->HDisplay) / 2;

        if (pScr->frameX0 < 0)
            pScr->frameX0 = 0;

        pScr->frameX1 = pScr->frameX0 + mode->HDisplay - 1;
        if (pScr->frameX1 >= pScr->virtualX) {
            pScr->frameX0 = pScr->virtualX - mode->HDisplay;
            pScr->frameX1 = pScr->virtualX - 1;
        }

        if (pScreen == pCursorScreen)
            pScr->frameY0 = py - (mode->VDisplay / 2) + 1;
        else
            pScr->frameY0 =
                (pScr->frameY0 + pScr->frameY1 + 1 - mode->VDisplay) / 2;

        if (pScr->frameY0 < 0)
            pScr->frameY0 = 0;

        pScr->frameY1 = pScr->frameY0 + mode->VDisplay - 1;
        if (pScr->frameY1 >= pScr->virtualY) {
            pScr->frameY0 = pScr->virtualY - mode->VDisplay;
            pScr->frameY1 = pScr->virtualY - 1;
        }
    }
    input_unlock();

    if (pScr->AdjustFrame)
        (*pScr->AdjustFrame) (pScr, pScr->frameX0, pScr->frameY0);

    /* The original code centered the frame around the cursor if possible.
     * Since this is hard to achieve with multiple cursors, we do the following:
     *   - center around the first pointer
     *   - move all other pointers to the nearest edge on the screen (or leave
     *   them unmodified if they are within the boundaries).
     */
    if (pScreen == pCursorScreen) {
        xf86WarpCursor(dev, pScreen, px, py);
    }

    for (it = inputInfo.devices; it; it = it->next) {
        if (it == dev)
            continue;

        if (IsPointerDevice(it) && it->spriteInfo->spriteOwner) {
            pCursorScreen = miPointerGetScreen(it);
            if (pScreen == pCursorScreen) {
                miPointerGetPosition(it, &px, &py);
                if (px < pScr->frameX0)
                    px = pScr->frameX0;
                else if (px > pScr->frameX1)
                    px = pScr->frameX1;

                if (py < pScr->frameY0)
                    py = pScr->frameY0;
                else if (py > pScr->frameY1)
                    py = pScr->frameY1;

                xf86WarpCursor(it, pScreen, px, py);
            }
        }
    }

    return Switched;
}

/*
 * xf86ZoomViewport --
 *      Reinitialize the visual part of the screen for another mode.
 */

void
xf86ZoomViewport(ScreenPtr pScreen, int zoom)
{
    ScrnInfoPtr pScr = xf86ScreenToScrn(pScreen);
    DisplayModePtr mode;

    if (pScr->zoomLocked || !(mode = pScr->currentMode))
        return;

    do {
        if (zoom > 0)
            mode = mode->next;
        else
            mode = mode->prev;
    } while (mode != pScr->currentMode && !(mode->type & M_T_USERDEF));

    (void) xf86SwitchMode(pScreen, mode);
}

static xf86EdgePtr
FindEdge(xf86EdgePtr edge, int val)
{
    while (edge && (edge->end <= val))
        edge = edge->next;

    if (edge && (edge->start <= val))
        return edge;

    return NULL;
}

/*
 * xf86CursorOffScreen --
 *      Check whether it is necessary to switch to another screen
 */

static Bool
xf86CursorOffScreen(ScreenPtr *pScreen, int *x, int *y)
{
    xf86EdgePtr edge;
    int tmp;

    if (screenInfo.numScreens == 1)
        return FALSE;

    if (*x < 0) {
        tmp = *y;
        if (tmp < 0)
            tmp = 0;
        if (tmp >= (*pScreen)->height)
            tmp = (*pScreen)->height - 1;

        if ((edge = xf86ScreenLayout[(*pScreen)->myNum].left))
            edge = FindEdge(edge, tmp);

        if (!edge)
            *x = 0;
        else {
            *x += edge->offset.x;
            *y += edge->offset.y;
            *pScreen = xf86Screens[edge->screen]->pScreen;
        }
    }

    if (*x >= (*pScreen)->width) {
        tmp = *y;
        if (tmp < 0)
            tmp = 0;
        if (tmp >= (*pScreen)->height)
            tmp = (*pScreen)->height - 1;

        if ((edge = xf86ScreenLayout[(*pScreen)->myNum].right))
            edge = FindEdge(edge, tmp);

        if (!edge)
            *x = (*pScreen)->width - 1;
        else {
            *x += edge->offset.x;
            *y += edge->offset.y;
            *pScreen = xf86Screens[edge->screen]->pScreen;
        }
    }

    if (*y < 0) {
        tmp = *x;
        if (tmp < 0)
            tmp = 0;
        if (tmp >= (*pScreen)->width)
            tmp = (*pScreen)->width - 1;

        if ((edge = xf86ScreenLayout[(*pScreen)->myNum].up))
            edge = FindEdge(edge, tmp);

        if (!edge)
            *y = 0;
        else {
            *x += edge->offset.x;
            *y += edge->offset.y;
            *pScreen = xf86Screens[edge->screen]->pScreen;
        }
    }

    if (*y >= (*pScreen)->height) {
        tmp = *x;
        if (tmp < 0)
            tmp = 0;
        if (tmp >= (*pScreen)->width)
            tmp = (*pScreen)->width - 1;

        if ((edge = xf86ScreenLayout[(*pScreen)->myNum].down))
            edge = FindEdge(edge, tmp);

        if (!edge)
            *y = (*pScreen)->height - 1;
        else {
            *x += edge->offset.x;
            *y += edge->offset.y;
            (*pScreen) = xf86Screens[edge->screen]->pScreen;
        }
    }

    return TRUE;
}

/*
 * xf86CrossScreen --
 *      Switch to another screen
 *
 *	Currently nothing special happens, but mi assumes the CrossScreen
 *	method exists.
 */

static void
xf86CrossScreen(ScreenPtr pScreen, Bool entering)
{
}

/*
 * xf86WarpCursor --
 *      Warp possible to another screen
 */

/* ARGSUSED */
static void
xf86WarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    input_lock();
    miPointerWarpCursor(pDev, pScreen, x, y);

    xf86Info.currentScreen = pScreen;
    input_unlock();
}

void *
xf86GetPointerScreenFuncs(void)
{
    return (void *) &xf86PointerScreenFuncs;
}

static xf86EdgePtr
AddEdge(xf86EdgePtr edge,
        short min, short max, short dx, short dy, short screen)
{
    xf86EdgePtr pEdge = edge, pPrev = NULL, pNew;

    while (1) {
        while (pEdge && (min >= pEdge->end)) {
            pPrev = pEdge;
            pEdge = pEdge->next;
        }

        if (!pEdge) {
            if (!(pNew = malloc(sizeof(xf86EdgeRec))))
                break;

            pNew->screen = screen;
            pNew->start = min;
            pNew->end = max;
            pNew->offset.x = dx;
            pNew->offset.y = dy;
            pNew->next = NULL;

            if (pPrev)
                pPrev->next = pNew;
            else
                edge = pNew;

            break;
        }
        else if (min < pEdge->start) {
            if (!(pNew = malloc(sizeof(xf86EdgeRec))))
                break;

            pNew->screen = screen;
            pNew->start = min;
            pNew->offset.x = dx;
            pNew->offset.y = dy;
            pNew->next = pEdge;

            if (pPrev)
                pPrev->next = pNew;
            else
                edge = pNew;

            if (max <= pEdge->start) {
                pNew->end = max;
                break;
            }
            else {
                pNew->end = pEdge->start;
                min = pEdge->end;
            }
        }
        else
            min = pEdge->end;

        pPrev = pEdge;
        pEdge = pEdge->next;

        if (max <= min)
            break;
    }

    return edge;
}

static void
FillOutEdge(xf86EdgePtr pEdge, int limit)
{
    xf86EdgePtr pNext;
    int diff;

    if (pEdge->start > 0)
        pEdge->start = 0;

    while ((pNext = pEdge->next)) {
        diff = pNext->start - pEdge->end;
        if (diff > 0) {
            pEdge->end += diff >> 1;
            pNext->start -= diff - (diff >> 1);
        }
        pEdge = pNext;
    }

    if (pEdge->end < limit)
        pEdge->end = limit;
}

/*
 * xf86InitOrigins() can deal with a maximum of 32 screens
 * on 32 bit architectures, 64 on 64 bit architectures.
 */

void
xf86InitOrigins(void)
{
    unsigned long screensLeft, prevScreensLeft, mask;
    screenLayoutPtr screen;
    ScreenPtr pScreen, refScreen;
    int x1, x2, y1, y2, left, right, top, bottom;
    int i, j, ref, minX, minY, min, max;
    xf86ScreenLayoutPtr pLayout;
    Bool OldStyleConfig = FALSE;

    memset(xf86ScreenLayout, 0, MAXSCREENS * sizeof(xf86ScreenLayoutRec));

    screensLeft = prevScreensLeft = (1 << xf86NumScreens) - 1;

    while (1) {
        for (mask = screensLeft, i = 0; mask; mask >>= 1, i++) {
            if (!(mask & 1L))
                continue;

            screen = &xf86ConfigLayout.screens[i];

            if (screen->refscreen != NULL &&
                screen->refscreen->screennum >= xf86NumScreens) {
                screensLeft &= ~(1 << i);
                xf86Msg(X_WARNING,
                        "Not including screen \"%s\" in origins calculation.\n",
                        screen->screen->id);
                continue;
            }

            pScreen = xf86Screens[i]->pScreen;
            switch (screen->where) {
            case PosObsolete:
                OldStyleConfig = TRUE;
                pLayout = &xf86ScreenLayout[i];
                /* force edge lists */
                if (screen->left) {
                    ref = screen->left->screennum;
                    if (!xf86Screens[ref] || !xf86Screens[ref]->pScreen) {
                        ErrorF("Referenced uninitialized screen in Layout!\n");
                        break;
                    }
                    pLayout->left = AddEdge(pLayout->left,
                                            0, pScreen->height,
                                            xf86Screens[ref]->pScreen->width, 0,
                                            ref);
                }
                if (screen->right) {
                    ref = screen->right->screennum;
                    if (!xf86Screens[ref] || !xf86Screens[ref]->pScreen) {
                        ErrorF("Referenced uninitialized screen in Layout!\n");
                        break;
                    }
                    pLayout->right = AddEdge(pLayout->right,
                                             0, pScreen->height,
                                             -pScreen->width, 0, ref);
                }
                if (screen->top) {
                    ref = screen->top->screennum;
                    if (!xf86Screens[ref] || !xf86Screens[ref]->pScreen) {
                        ErrorF("Referenced uninitialized screen in Layout!\n");
                        break;
                    }
                    pLayout->up = AddEdge(pLayout->up,
                                          0, pScreen->width,
                                          0, xf86Screens[ref]->pScreen->height,
                                          ref);
                }
                if (screen->bottom) {
                    ref = screen->bottom->screennum;
                    if (!xf86Screens[ref] || !xf86Screens[ref]->pScreen) {
                        ErrorF("Referenced uninitialized screen in Layout!\n");
                        break;
                    }
                    pLayout->down = AddEdge(pLayout->down,
                                            0, pScreen->width, 0,
                                            -pScreen->height, ref);
                }
                /* we could also try to place it based on those
                   relative locations if we wanted to */
                screen->x = screen->y = 0;
                /* FALLTHROUGH */
            case PosAbsolute:
                pScreen->x = screen->x;
                pScreen->y = screen->y;
                screensLeft &= ~(1 << i);
                break;
            case PosRelative:
                ref = screen->refscreen->screennum;
                if (!xf86Screens[ref] || !xf86Screens[ref]->pScreen) {
                    ErrorF("Referenced uninitialized screen in Layout!\n");
                    break;
                }
                if (screensLeft & (1 << ref))
                    break;
                refScreen = xf86Screens[ref]->pScreen;
                pScreen->x = refScreen->x + screen->x;
                pScreen->y = refScreen->y + screen->y;
                screensLeft &= ~(1 << i);
                break;
            case PosRightOf:
                ref = screen->refscreen->screennum;
                if (!xf86Screens[ref] || !xf86Screens[ref]->pScreen) {
                    ErrorF("Referenced uninitialized screen in Layout!\n");
                    break;
                }
                if (screensLeft & (1 << ref))
                    break;
                refScreen = xf86Screens[ref]->pScreen;
                pScreen->x = refScreen->x + refScreen->width;
                pScreen->y = refScreen->y;
                screensLeft &= ~(1 << i);
                break;
            case PosLeftOf:
                ref = screen->refscreen->screennum;
                if (!xf86Screens[ref] || !xf86Screens[ref]->pScreen) {
                    ErrorF("Referenced uninitialized screen in Layout!\n");
                    break;
                }
                if (screensLeft & (1 << ref))
                    break;
                refScreen = xf86Screens[ref]->pScreen;
                pScreen->x = refScreen->x - pScreen->width;
                pScreen->y = refScreen->y;
                screensLeft &= ~(1 << i);
                break;
            case PosBelow:
                ref = screen->refscreen->screennum;
                if (!xf86Screens[ref] || !xf86Screens[ref]->pScreen) {
                    ErrorF("Referenced uninitialized screen in Layout!\n");
                    break;
                }
                if (screensLeft & (1 << ref))
                    break;
                refScreen = xf86Screens[ref]->pScreen;
                pScreen->x = refScreen->x;
                pScreen->y = refScreen->y + refScreen->height;
                screensLeft &= ~(1 << i);
                break;
            case PosAbove:
                ref = screen->refscreen->screennum;
                if (!xf86Screens[ref] || !xf86Screens[ref]->pScreen) {
                    ErrorF("Referenced uninitialized screen in Layout!\n");
                    break;
                }
                if (screensLeft & (1 << ref))
                    break;
                refScreen = xf86Screens[ref]->pScreen;
                pScreen->x = refScreen->x;
                pScreen->y = refScreen->y - pScreen->height;
                screensLeft &= ~(1 << i);
                break;
            default:
                ErrorF("Illegal placement keyword in Layout!\n");
                break;
            }

        }

        if (!screensLeft)
            break;

        if (screensLeft == prevScreensLeft) {
            /* All the remaining screens are referencing each other.
               Assign a value to one of them and go through again */
            i = 0;
            while (!((1 << i) & screensLeft)) {
                i++;
            }

            ref = xf86ConfigLayout.screens[i].refscreen->screennum;
            xf86Screens[ref]->pScreen->x = xf86Screens[ref]->pScreen->y = 0;
            screensLeft &= ~(1 << ref);
        }

        prevScreensLeft = screensLeft;
    }

    /* justify the topmost and leftmost to (0,0) */
    minX = xf86Screens[0]->pScreen->x;
    minY = xf86Screens[0]->pScreen->y;

    for (i = 1; i < xf86NumScreens; i++) {
        if (xf86Screens[i]->pScreen->x < minX)
            minX = xf86Screens[i]->pScreen->x;
        if (xf86Screens[i]->pScreen->y < minY)
            minY = xf86Screens[i]->pScreen->y;
    }

    if (minX || minY) {
        for (i = 0; i < xf86NumScreens; i++) {
            xf86Screens[i]->pScreen->x -= minX;
            xf86Screens[i]->pScreen->y -= minY;
        }
    }

    /* Create the edge lists */

    if (!OldStyleConfig) {
        for (i = 0; i < xf86NumScreens; i++) {
            pLayout = &xf86ScreenLayout[i];

            pScreen = xf86Screens[i]->pScreen;

            left = pScreen->x;
            right = left + pScreen->width;
            top = pScreen->y;
            bottom = top + pScreen->height;

            for (j = 0; j < xf86NumScreens; j++) {
                if (i == j)
                    continue;

                refScreen = xf86Screens[j]->pScreen;

                x1 = refScreen->x;
                x2 = x1 + refScreen->width;
                y1 = refScreen->y;
                y2 = y1 + refScreen->height;

                if ((bottom > y1) && (top < y2)) {
                    min = y1 - top;
                    if (min < 0)
                        min = 0;
                    max = pScreen->height - (bottom - y2);
                    if (max > pScreen->height)
                        max = pScreen->height;

                    if (((left - 1) >= x1) && ((left - 1) < x2))
                        pLayout->left = AddEdge(pLayout->left, min, max,
                                                pScreen->x - refScreen->x,
                                                pScreen->y - refScreen->y, j);

                    if ((right >= x1) && (right < x2))
                        pLayout->right = AddEdge(pLayout->right, min, max,
                                                 pScreen->x - refScreen->x,
                                                 pScreen->y - refScreen->y, j);
                }

                if ((left < x2) && (right > x1)) {
                    min = x1 - left;
                    if (min < 0)
                        min = 0;
                    max = pScreen->width - (right - x2);
                    if (max > pScreen->width)
                        max = pScreen->width;

                    if (((top - 1) >= y1) && ((top - 1) < y2))
                        pLayout->up = AddEdge(pLayout->up, min, max,
                                              pScreen->x - refScreen->x,
                                              pScreen->y - refScreen->y, j);

                    if ((bottom >= y1) && (bottom < y2))
                        pLayout->down = AddEdge(pLayout->down, min, max,
                                                pScreen->x - refScreen->x,
                                                pScreen->y - refScreen->y, j);
                }
            }
        }
    }

    if (!OldStyleConfig) {
        for (i = 0; i < xf86NumScreens; i++) {
            pLayout = &xf86ScreenLayout[i];
            pScreen = xf86Screens[i]->pScreen;
            if (pLayout->left)
                FillOutEdge(pLayout->left, pScreen->height);
            if (pLayout->right)
                FillOutEdge(pLayout->right, pScreen->height);
            if (pLayout->up)
                FillOutEdge(pLayout->up, pScreen->width);
            if (pLayout->down)
                FillOutEdge(pLayout->down, pScreen->width);
        }
    }

    update_desktop_dimensions();
}

void
xf86ReconfigureLayout(void)
{
    int i;

    for (i = 0; i < MAXSCREENS; i++) {
        xf86ScreenLayoutPtr sl = &xf86ScreenLayout[i];

        /* we don't have to zero these, xf86InitOrigins() takes care of that */
        free(sl->left);
        free(sl->right);
        free(sl->up);
        free(sl->down);
    }

    xf86InitOrigins();
}
