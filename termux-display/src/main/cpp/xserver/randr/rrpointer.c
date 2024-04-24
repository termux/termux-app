/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "randrstr.h"
#include "inputstr.h"

/*
 * When the pointer moves, check to see if the specified position is outside
 * any of theavailable CRTCs and move it to a 'sensible' place if so, where
 * sensible is the closest monitor to the departing edge.
 *
 * Returns whether the position was adjusted
 */

static Bool
RRCrtcContainsPosition(RRCrtcPtr crtc, int x, int y)
{
    RRModePtr mode = crtc->mode;
    int scan_width, scan_height;

    if (!mode)
        return FALSE;

    RRCrtcGetScanoutSize(crtc, &scan_width, &scan_height);

    if (crtc->x <= x && x < crtc->x + scan_width &&
        crtc->y <= y && y < crtc->y + scan_height)
        return TRUE;
    return FALSE;
}

/*
 * Find the CRTC nearest the specified position, ignoring 'skip'
 */
static void
RRPointerToNearestCrtc(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y,
                       RRCrtcPtr skip)
{
    rrScrPriv(pScreen);
    int c;
    RRCrtcPtr nearest = NULL;
    int best = 0;
    int best_dx = 0, best_dy = 0;

    for (c = 0; c < pScrPriv->numCrtcs; c++) {
        RRCrtcPtr crtc = pScrPriv->crtcs[c];
        RRModePtr mode = crtc->mode;
        int dx, dy;
        int dist;
        int scan_width, scan_height;

        if (!mode)
            continue;
        if (crtc == skip)
            continue;

        RRCrtcGetScanoutSize(crtc, &scan_width, &scan_height);

        if (x < crtc->x)
            dx = crtc->x - x;
        else if (x > crtc->x + scan_width - 1)
            dx = crtc->x + (scan_width - 1) - x;
        else
            dx = 0;
        if (y < crtc->y)
            dy = crtc->y - y;
        else if (y > crtc->y + scan_height - 1)
            dy = crtc->y + (scan_height - 1) - y;
        else
            dy = 0;
        dist = dx * dx + dy * dy;
        if (!nearest || dist < best) {
            nearest = crtc;
            best_dx = dx;
            best_dy = dy;
            best = dist;
        }
    }
    if (best_dx || best_dy)
        (*pScreen->SetCursorPosition) (pDev, pScreen, x + best_dx, y + best_dy,
                                       TRUE);
    pScrPriv->pointerCrtc = nearest;
}

void
RRPointerMoved(ScreenPtr pScreen, int x, int y)
{
    rrScrPriv(pScreen);
    RRCrtcPtr pointerCrtc = pScrPriv->pointerCrtc;
    int c;

    /* Check last known CRTC */
    if (pointerCrtc && RRCrtcContainsPosition(pointerCrtc, x, y))
        return;

    /* Check all CRTCs */
    for (c = 0; c < pScrPriv->numCrtcs; c++) {
        RRCrtcPtr crtc = pScrPriv->crtcs[c];

        if (RRCrtcContainsPosition(crtc, x, y)) {
            /* Remember containing CRTC */
            pScrPriv->pointerCrtc = crtc;
            return;
        }
    }

    /* None contain pointer, find nearest */
    ErrorF("RRPointerMoved: Untested, may cause \"bogus pointer event\"\n");
    RRPointerToNearestCrtc(inputInfo.pointer, pScreen, x, y, pointerCrtc);
}

/*
 * When the screen is reconfigured, move all pointers to the nearest
 * CRTC
 */
void
RRPointerScreenConfigured(ScreenPtr pScreen)
{
    WindowPtr pRoot;
    ScreenPtr pCurrentScreen;
    int x, y;
    DeviceIntPtr pDev;

    for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
        if (IsPointerDevice(pDev)) {
            pRoot = GetCurrentRootWindow(pDev);
            pCurrentScreen = pRoot ? pRoot->drawable.pScreen : NULL;

            if (pScreen == pCurrentScreen) {
                GetSpritePosition(pDev, &x, &y);
                RRPointerToNearestCrtc(pDev, pScreen, x, y, NULL);
            }
        }
    }
}
