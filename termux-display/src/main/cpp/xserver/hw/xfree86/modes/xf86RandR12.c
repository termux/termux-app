/*
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "os.h"
#include "globals.h"
#include "xf86Modes.h"
#include "xf86Priv.h"
#include "xf86DDC.h"
#include "mipointer.h"
#include "windowstr.h"
#include "inputstr.h"
#include <randrstr.h>
#include <X11/extensions/render.h>

#include "xf86cmap.h"
#include "xf86Crtc.h"
#include "xf86RandR12.h"

typedef struct _xf86RandR12Info {
    int virtualX;
    int virtualY;
    int mmWidth;
    int mmHeight;
    int maxX;
    int maxY;
    int pointerX;
    int pointerY;
    Rotation rotation;          /* current mode */
    Rotation supported_rotations;       /* driver supported */

    /* Compatibility with colormaps and XF86VidMode's gamma */
    int palette_red_size;
    int palette_green_size;
    int palette_blue_size;
    int palette_size;
    LOCO *palette;

    /* Used to wrap EnterVT so we can re-probe the outputs when a laptop unsuspends
     * (actually, any time that we switch back into our VT).
     *
     * See https://bugs.freedesktop.org/show_bug.cgi?id=21554
     */
    xf86EnterVTProc *orig_EnterVT;

    Bool                         panning;
    ConstrainCursorHarderProcPtr orig_ConstrainCursorHarder;
} XF86RandRInfoRec, *XF86RandRInfoPtr;

#ifdef RANDR_12_INTERFACE
static Bool xf86RandR12Init12(ScreenPtr pScreen);
static Bool xf86RandR12CreateScreenResources12(ScreenPtr pScreen);
#endif

static int xf86RandR12Generation;

static DevPrivateKeyRec xf86RandR12KeyRec;

#define XF86RANDRINFO(p) ((XF86RandRInfoPtr) \
    dixLookupPrivate(&(p)->devPrivates, &xf86RandR12KeyRec))

static int
xf86RandR12ModeRefresh(DisplayModePtr mode)
{
    if (mode->VRefresh)
        return (int) (mode->VRefresh + 0.5);
    else
        return (int) (mode->Clock * 1000.0 / mode->HTotal / mode->VTotal + 0.5);
}

/* Adapt panning area; return TRUE if panning area was valid without adaption */
static int
xf86RandR13VerifyPanningArea(xf86CrtcPtr crtc, int screenWidth,
                             int screenHeight)
{
    int ret = TRUE;

    if (crtc->version < 2)
        return FALSE;

    if (crtc->panningTotalArea.x2 <= crtc->panningTotalArea.x1) {
        /* Panning in X is disabled */
        if (crtc->panningTotalArea.x1 || crtc->panningTotalArea.x2)
            /* Illegal configuration -> fail/disable */
            ret = FALSE;
        crtc->panningTotalArea.x1 = crtc->panningTotalArea.x2 = 0;
        crtc->panningTrackingArea.x1 = crtc->panningTrackingArea.x2 = 0;
        crtc->panningBorder[0] = crtc->panningBorder[2] = 0;
    }
    else {
        /* Panning in X is enabled */
        if (crtc->panningTotalArea.x1 < 0) {
            /* Panning region outside screen -> move inside */
            crtc->panningTotalArea.x2 -= crtc->panningTotalArea.x1;
            crtc->panningTotalArea.x1 = 0;
            ret = FALSE;
        }
        if (crtc->panningTotalArea.x2 <
            crtc->panningTotalArea.x1 + crtc->mode.HDisplay) {
            /* Panning region smaller than displayed area -> crop to displayed area */
            crtc->panningTotalArea.x2 =
                crtc->panningTotalArea.x1 + crtc->mode.HDisplay;
            ret = FALSE;
        }
        if (crtc->panningTotalArea.x2 > screenWidth) {
            /* Panning region larger than screen -> move inside, then crop to screen */
            crtc->panningTotalArea.x1 -=
                crtc->panningTotalArea.x2 - screenWidth;
            crtc->panningTotalArea.x2 = screenWidth;
            ret = FALSE;
            if (crtc->panningTotalArea.x1 < 0)
                crtc->panningTotalArea.x1 = 0;
        }
        if (crtc->panningBorder[0] + crtc->panningBorder[2] >
            crtc->mode.HDisplay) {
            /* Borders too large -> set to 0 */
            crtc->panningBorder[0] = crtc->panningBorder[2] = 0;
            ret = FALSE;
        }
    }

    if (crtc->panningTotalArea.y2 <= crtc->panningTotalArea.y1) {
        /* Panning in Y is disabled */
        if (crtc->panningTotalArea.y1 || crtc->panningTotalArea.y2)
            /* Illegal configuration -> fail/disable */
            ret = FALSE;
        crtc->panningTotalArea.y1 = crtc->panningTotalArea.y2 = 0;
        crtc->panningTrackingArea.y1 = crtc->panningTrackingArea.y2 = 0;
        crtc->panningBorder[1] = crtc->panningBorder[3] = 0;
    }
    else {
        /* Panning in Y is enabled */
        if (crtc->panningTotalArea.y1 < 0) {
            /* Panning region outside screen -> move inside */
            crtc->panningTotalArea.y2 -= crtc->panningTotalArea.y1;
            crtc->panningTotalArea.y1 = 0;
            ret = FALSE;
        }
        if (crtc->panningTotalArea.y2 <
            crtc->panningTotalArea.y1 + crtc->mode.VDisplay) {
            /* Panning region smaller than displayed area -> crop to displayed area */
            crtc->panningTotalArea.y2 =
                crtc->panningTotalArea.y1 + crtc->mode.VDisplay;
            ret = FALSE;
        }
        if (crtc->panningTotalArea.y2 > screenHeight) {
            /* Panning region larger than screen -> move inside, then crop to screen */
            crtc->panningTotalArea.y1 -=
                crtc->panningTotalArea.y2 - screenHeight;
            crtc->panningTotalArea.y2 = screenHeight;
            ret = FALSE;
            if (crtc->panningTotalArea.y1 < 0)
                crtc->panningTotalArea.y1 = 0;
        }
        if (crtc->panningBorder[1] + crtc->panningBorder[3] >
            crtc->mode.VDisplay) {
            /* Borders too large -> set to 0 */
            crtc->panningBorder[1] = crtc->panningBorder[3] = 0;
            ret = FALSE;
        }
    }

    return ret;
}

/*
 * The heart of the panning operation:
 *
 * Given a frame buffer position (fb_x, fb_y),
 * and a crtc position (crtc_x, crtc_y),
 * and a transform matrix which maps frame buffer to crtc,
 * compute a panning position (pan_x, pan_y) that
 * makes the resulting transform line those two up
 */

static void
xf86ComputeCrtcPan(Bool transform_in_use,
                   struct pixman_f_transform *m,
                   double screen_x, double screen_y,
                   double crtc_x, double crtc_y,
                   int old_pan_x, int old_pan_y, int *new_pan_x, int *new_pan_y)
{
    if (transform_in_use) {
        /*
         * Given the current transform, M, the current position
         * on the Screen, S, and the desired position on the CRTC,
         * C, compute a translation, T, such that:
         *
         * M T S = C
         *
         * where T is of the form
         *
         * | 1 0 dx |
         * | 0 1 dy |
         * | 0 0 1  |
         *
         * M T S =
         *   | M00 Sx + M01 Sy + M00 dx + M01 dy + M02 |   | Cx F |
         *   | M10 Sx + M11 Sy + M10 dx + M11 dy + M12 | = | Cy F |
         *   | M20 Sx + M21 Sy + M20 dx + M21 dy + M22 |   |  F   |
         *
         * R = M S
         *
         *   Cx F = M00 dx + M01 dy + R0
         *   Cy F = M10 dx + M11 dy + R1
         *      F = M20 dx + M21 dy + R2
         *
         * Zero out dx, then dy
         *
         * F (Cx M10 - Cy M00) =
         *          (M10 M01 - M00 M11) dy + M10 R0 - M00 R1
         * F (M10 - Cy M20) =
         *          (M10 M21 - M20 M11) dy + M10 R2 - M20 R1
         *
         * F (Cx M11 - Cy M01) =
         *          (M11 M00 - M01 M10) dx + M11 R0 - M01 R1
         * F (M11 - Cy M21) =
         *          (M11 M20 - M21 M10) dx + M11 R2 - M21 R1
         *
         * Make some temporaries
         *
         * T = | Cx M10 - Cy M00 |
         *     | Cx M11 - Cy M01 |
         *
         * U = | M10 M01 - M00 M11 |
         *     | M11 M00 - M01 M10 |
         *
         * Q = | M10 R0 - M00 R1 |
         *     | M11 R0 - M01 R1 |
         *
         * P = | M10 - Cy M20 |
         *     | M11 - Cy M21 |
         *
         * W = | M10 M21 - M20 M11 |
         *     | M11 M20 - M21 M10 |
         *
         * V = | M10 R2 - M20 R1 |
         *         | M11 R2 - M21 R1 |
         *
         * Rewrite:
         *
         * F T0 = U0 dy + Q0
         * F P0 = W0 dy + V0
         * F T1 = U1 dx + Q1
         * F P1 = W1 dx + V1
         *
         * Solve for F (two ways)
         *
         * F (W0 T0 - U0 P0)  = W0 Q0 - U0 V0
         *
         *     W0 Q0 - U0 V0
         * F = -------------
         *     W0 T0 - U0 P0
         *
         * F (W1 T1 - U1 P1) = W1 Q1 - U1 V1
         *
         *     W1 Q1 - U1 V1
         * F = -------------
         *     W1 T1 - U1 P1
         *
         * We'll use which ever solution works (denominator != 0)
         *
         * Finally, solve for dx and dy:
         *
         * dx = (F T1 - Q1) / U1
         * dx = (F P1 - V1) / W1
         *
         * dy = (F T0 - Q0) / U0
         * dy = (F P0 - V0) / W0
         */
        double r[3];
        double q[2], u[2], t[2], v[2], w[2], p[2];
        double f;
        struct pict_f_vector d;
        int i;

        /* Get the un-normalized crtc coordinates again */
        for (i = 0; i < 3; i++)
            r[i] = m->m[i][0] * screen_x + m->m[i][1] * screen_y + m->m[i][2];

        /* Combine values into temporaries */
        for (i = 0; i < 2; i++) {
            q[i] = m->m[1][i] * r[0] - m->m[0][i] * r[1];
            u[i] = m->m[1][i] * m->m[0][1 - i] - m->m[0][i] * m->m[1][1 - i];
            t[i] = m->m[1][i] * crtc_x - m->m[0][i] * crtc_y;

            v[i] = m->m[1][i] * r[2] - m->m[2][i] * r[1];
            w[i] = m->m[1][i] * m->m[2][1 - i] - m->m[2][i] * m->m[1][1 - i];
            p[i] = m->m[1][i] - m->m[2][i] * crtc_y;
        }

        /* Find a way to compute f */
        f = 0;
        for (i = 0; i < 2; i++) {
            double a = w[i] * q[i] - u[i] * v[i];
            double b = w[i] * t[i] - u[i] * p[i];

            if (b != 0) {
                f = a / b;
                break;
            }
        }

        /* Solve for the resulting transform vector */
        for (i = 0; i < 2; i++) {
            if (u[i])
                d.v[1 - i] = (t[i] * f - q[i]) / u[i];
            else if (w[1])
                d.v[1 - i] = (p[i] * f - v[i]) / w[i];
            else
                d.v[1 - i] = 0;
        }
        *new_pan_x = old_pan_x - floor(d.v[0] + 0.5);
        *new_pan_y = old_pan_y - floor(d.v[1] + 0.5);
    }
    else {
        *new_pan_x = screen_x - crtc_x;
        *new_pan_y = screen_y - crtc_y;
    }
}

static void
xf86RandR13Pan(xf86CrtcPtr crtc, int x, int y)
{
    int newX, newY;
    int width, height;
    Bool panned = FALSE;

    if (crtc->version < 2)
        return;

    if (!crtc->enabled ||
        (crtc->panningTotalArea.x2 <= crtc->panningTotalArea.x1 &&
         crtc->panningTotalArea.y2 <= crtc->panningTotalArea.y1))
        return;

    newX = crtc->x;
    newY = crtc->y;
    width = crtc->mode.HDisplay;
    height = crtc->mode.VDisplay;

    if ((crtc->panningTrackingArea.x2 <= crtc->panningTrackingArea.x1 ||
         (x >= crtc->panningTrackingArea.x1 &&
          x < crtc->panningTrackingArea.x2)) &&
        (crtc->panningTrackingArea.y2 <= crtc->panningTrackingArea.y1 ||
         (y >= crtc->panningTrackingArea.y1 &&
          y < crtc->panningTrackingArea.y2))) {
        struct pict_f_vector c;

        /*
         * Pre-clip the mouse position to the panning area so that we don't
         * push the crtc outside. This doesn't deal with changes to the
         * panning values, only mouse position changes.
         */
        if (crtc->panningTotalArea.x2 > crtc->panningTotalArea.x1) {
            if (x < crtc->panningTotalArea.x1)
                x = crtc->panningTotalArea.x1;
            if (x >= crtc->panningTotalArea.x2)
                x = crtc->panningTotalArea.x2 - 1;
        }
        if (crtc->panningTotalArea.y2 > crtc->panningTotalArea.y1) {
            if (y < crtc->panningTotalArea.y1)
                y = crtc->panningTotalArea.y1;
            if (y >= crtc->panningTotalArea.y2)
                y = crtc->panningTotalArea.y2 - 1;
        }

        c.v[0] = x;
        c.v[1] = y;
        c.v[2] = 1.0;
        if (crtc->transform_in_use) {
            pixman_f_transform_point(&crtc->f_framebuffer_to_crtc, &c);
        }
        else {
            c.v[0] -= crtc->x;
            c.v[1] -= crtc->y;
        }

        if (crtc->panningTotalArea.x2 > crtc->panningTotalArea.x1) {
            if (c.v[0] < crtc->panningBorder[0]) {
                c.v[0] = crtc->panningBorder[0];
                panned = TRUE;
            }
            if (c.v[0] >= width - crtc->panningBorder[2]) {
                c.v[0] = width - crtc->panningBorder[2] - 1;
                panned = TRUE;
            }
        }
        if (crtc->panningTotalArea.y2 > crtc->panningTotalArea.y1) {
            if (c.v[1] < crtc->panningBorder[1]) {
                c.v[1] = crtc->panningBorder[1];
                panned = TRUE;
            }
            if (c.v[1] >= height - crtc->panningBorder[3]) {
                c.v[1] = height - crtc->panningBorder[3] - 1;
                panned = TRUE;
            }
        }
        if (panned)
            xf86ComputeCrtcPan(crtc->transform_in_use,
                               &crtc->f_framebuffer_to_crtc,
                               x, y, c.v[0], c.v[1], newX, newY, &newX, &newY);
    }

    /*
     * Ensure that the crtc is within the panning region.
     *
     * XXX This computation only works when we do not have a transform
     * in use.
     */
    if (!crtc->transform_in_use) {
        /* Validate against [xy]1 after [xy]2, to be sure that results are > 0 for [xy]1 > 0 */
        if (crtc->panningTotalArea.x2 > crtc->panningTotalArea.x1) {
            if (newX > crtc->panningTotalArea.x2 - width)
                newX = crtc->panningTotalArea.x2 - width;
            if (newX < crtc->panningTotalArea.x1)
                newX = crtc->panningTotalArea.x1;
        }
        if (crtc->panningTotalArea.y2 > crtc->panningTotalArea.y1) {
            if (newY > crtc->panningTotalArea.y2 - height)
                newY = crtc->panningTotalArea.y2 - height;
            if (newY < crtc->panningTotalArea.y1)
                newY = crtc->panningTotalArea.y1;
        }
    }
    if (newX != crtc->x || newY != crtc->y)
        xf86CrtcSetOrigin(crtc, newX, newY);
}

static Bool
xf86RandR12GetInfo(ScreenPtr pScreen, Rotation * rotations)
{
    RRScreenSizePtr pSize;
    ScrnInfoPtr scrp = xf86ScreenToScrn(pScreen);
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    DisplayModePtr mode;
    int maxX = 0, maxY = 0;

    *rotations = randrp->supported_rotations;

    if (randrp->virtualX == -1 || randrp->virtualY == -1) {
        randrp->virtualX = scrp->virtualX;
        randrp->virtualY = scrp->virtualY;
    }

    /* Re-probe the outputs for new monitors or modes */
    if (scrp->vtSema) {
        xf86ProbeOutputModes(scrp, 0, 0);
        xf86SetScrnInfoModes(scrp);
    }

    for (mode = scrp->modes;; mode = mode->next) {
        int refresh = xf86RandR12ModeRefresh(mode);

        if (randrp->maxX == 0 || randrp->maxY == 0) {
            if (maxX < mode->HDisplay)
                maxX = mode->HDisplay;
            if (maxY < mode->VDisplay)
                maxY = mode->VDisplay;
        }
        pSize = RRRegisterSize(pScreen,
                               mode->HDisplay, mode->VDisplay,
                               randrp->mmWidth, randrp->mmHeight);
        if (!pSize)
            return FALSE;
        RRRegisterRate(pScreen, pSize, refresh);

        if (xf86ModesEqual(mode, scrp->currentMode)) {
            RRSetCurrentConfig(pScreen, randrp->rotation, refresh, pSize);
        }
        if (mode->next == scrp->modes)
            break;
    }

    if (randrp->maxX == 0 || randrp->maxY == 0) {
        randrp->maxX = maxX;
        randrp->maxY = maxY;
    }

    return TRUE;
}

static Bool
xf86RandR12SetMode(ScreenPtr pScreen,
                   DisplayModePtr mode,
                   Bool useVirtual, int mmWidth, int mmHeight)
{
    ScrnInfoPtr scrp = xf86ScreenToScrn(pScreen);
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    int oldWidth = pScreen->width;
    int oldHeight = pScreen->height;
    int oldmmWidth = pScreen->mmWidth;
    int oldmmHeight = pScreen->mmHeight;
    WindowPtr pRoot = pScreen->root;
    DisplayModePtr currentMode = NULL;
    Bool ret = TRUE;

    if (pRoot)
        (*scrp->EnableDisableFBAccess) (scrp, FALSE);
    if (useVirtual) {
        scrp->virtualX = randrp->virtualX;
        scrp->virtualY = randrp->virtualY;
    }
    else {
        scrp->virtualX = mode->HDisplay;
        scrp->virtualY = mode->VDisplay;
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
    if (scrp->currentMode == mode) {
        /* Save current mode */
        currentMode = scrp->currentMode;
        /* Reset, just so we ensure the drivers SwitchMode is called */
        scrp->currentMode = NULL;
    }
    /*
     * We know that if the driver failed to SwitchMode to the rotated
     * version, then it should revert back to its prior mode.
     */
    if (!xf86SwitchMode(pScreen, mode)) {
        ret = FALSE;
        scrp->virtualX = pScreen->width = oldWidth;
        scrp->virtualY = pScreen->height = oldHeight;
        pScreen->mmWidth = oldmmWidth;
        pScreen->mmHeight = oldmmHeight;
        scrp->currentMode = currentMode;
    }

    /*
     * Make sure the layout is correct
     */
    xf86ReconfigureLayout();

    /*
     * Make sure the whole screen is visible
     */
    xf86SetViewport(pScreen, pScreen->width, pScreen->height);
    xf86SetViewport(pScreen, 0, 0);
    if (pRoot)
        (*scrp->EnableDisableFBAccess) (scrp, TRUE);
    return ret;
}

Bool
xf86RandR12SetConfig(ScreenPtr pScreen,
                     Rotation rotation, int rate, RRScreenSizePtr pSize)
{
    ScrnInfoPtr scrp = xf86ScreenToScrn(pScreen);
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    DisplayModePtr mode;
    int pos[MAXDEVICES][2];
    Bool useVirtual = FALSE;
    int maxX = 0, maxY = 0;
    Rotation oldRotation = randrp->rotation;
    DeviceIntPtr dev;
    Bool view_adjusted = FALSE;

    randrp->rotation = rotation;

    if (randrp->virtualX == -1 || randrp->virtualY == -1) {
        randrp->virtualX = scrp->virtualX;
        randrp->virtualY = scrp->virtualY;
    }

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (!IsMaster(dev) && !IsFloating(dev))
            continue;

        miPointerGetPosition(dev, &pos[dev->id][0], &pos[dev->id][1]);
    }

    for (mode = scrp->modes;; mode = mode->next) {
        if (randrp->maxX == 0 || randrp->maxY == 0) {
            if (maxX < mode->HDisplay)
                maxX = mode->HDisplay;
            if (maxY < mode->VDisplay)
                maxY = mode->VDisplay;
        }
        if (mode->HDisplay == pSize->width &&
            mode->VDisplay == pSize->height &&
            (rate == 0 || xf86RandR12ModeRefresh(mode) == rate))
            break;
        if (mode->next == scrp->modes) {
            if (pSize->width == randrp->virtualX &&
                pSize->height == randrp->virtualY) {
                mode = scrp->modes;
                useVirtual = TRUE;
                break;
            }
            if (randrp->maxX == 0 || randrp->maxY == 0) {
                randrp->maxX = maxX;
                randrp->maxY = maxY;
            }
            return FALSE;
        }
    }

    if (randrp->maxX == 0 || randrp->maxY == 0) {
        randrp->maxX = maxX;
        randrp->maxY = maxY;
    }

    if (!xf86RandR12SetMode(pScreen, mode, useVirtual, pSize->mmWidth,
                            pSize->mmHeight)) {
        randrp->rotation = oldRotation;
        return FALSE;
    }

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

#define PANNING_ENABLED(crtc)                                           \
    ((crtc)->panningTotalArea.x2 > (crtc)->panningTotalArea.x1 ||       \
     (crtc)->panningTotalArea.y2 > (crtc)->panningTotalArea.y1)

static Bool
xf86RandR12ScreenSetSize(ScreenPtr pScreen,
                         CARD16 width,
                         CARD16 height, CARD32 mmWidth, CARD32 mmHeight)
{
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    WindowPtr pRoot = pScreen->root;
    PixmapPtr pScrnPix;
    Bool ret = FALSE;
    int c;

    if (randrp->virtualX == -1 || randrp->virtualY == -1) {
        randrp->virtualX = pScrn->virtualX;
        randrp->virtualY = pScrn->virtualY;
    }
    if (pRoot && pScrn->vtSema)
        (*pScrn->EnableDisableFBAccess) (pScrn, FALSE);

    /* Let the driver update virtualX and virtualY */
    if (!(*config->funcs->resize) (pScrn, width, height))
        goto finish;

    ret = TRUE;
    /* Update panning information */
    for (c = 0; c < config->num_crtc; c++) {
        xf86CrtcPtr crtc = config->crtc[c];

	if (PANNING_ENABLED (crtc)) {
            if (crtc->panningTotalArea.x2 > crtc->panningTrackingArea.x1)
                crtc->panningTotalArea.x2 += width - pScreen->width;
            if (crtc->panningTotalArea.y2 > crtc->panningTrackingArea.y1)
                crtc->panningTotalArea.y2 += height - pScreen->height;
            if (crtc->panningTrackingArea.x2 > crtc->panningTrackingArea.x1)
                crtc->panningTrackingArea.x2 += width - pScreen->width;
            if (crtc->panningTrackingArea.y2 > crtc->panningTrackingArea.y1)
                crtc->panningTrackingArea.y2 += height - pScreen->height;
            xf86RandR13VerifyPanningArea(crtc, width, height);
            xf86RandR13Pan(crtc, randrp->pointerX, randrp->pointerY);
        }
    }

    pScrnPix = (*pScreen->GetScreenPixmap) (pScreen);
    pScreen->width = pScrnPix->drawable.width = width;
    pScreen->height = pScrnPix->drawable.height = height;
    randrp->mmWidth = pScreen->mmWidth = mmWidth;
    randrp->mmHeight = pScreen->mmHeight = mmHeight;

    xf86SetViewport(pScreen, pScreen->width - 1, pScreen->height - 1);
    xf86SetViewport(pScreen, 0, 0);

 finish:
    update_desktop_dimensions();

    if (pRoot && pScrn->vtSema)
        (*pScrn->EnableDisableFBAccess) (pScrn, TRUE);
#if RANDR_12_INTERFACE
    if (pScreen->root && ret)
        RRScreenSizeNotify(pScreen);
#endif
    return ret;
}

Rotation
xf86RandR12GetRotation(ScreenPtr pScreen)
{
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);

    return randrp->rotation;
}

Bool
xf86RandR12CreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr config;
    XF86RandRInfoPtr randrp;
    int c;
    int width, height;
    int mmWidth, mmHeight;

#ifdef PANORAMIX
    /* XXX disable RandR when using Xinerama */
    if (!noPanoramiXExtension)
        return TRUE;
#endif

    config = XF86_CRTC_CONFIG_PTR(pScrn);
    randrp = XF86RANDRINFO(pScreen);
    /*
     * Compute size of screen
     */
    width = 0;
    height = 0;
    for (c = 0; c < config->num_crtc; c++) {
        xf86CrtcPtr crtc = config->crtc[c];
        int crtc_width = crtc->x + xf86ModeWidth(&crtc->mode, crtc->rotation);
        int crtc_height = crtc->y + xf86ModeHeight(&crtc->mode, crtc->rotation);

        if (crtc->enabled) {
            if (crtc_width > width)
                width = crtc_width;
            if (crtc_height > height)
                height = crtc_height;
            if (crtc->panningTotalArea.x2 > width)
                width = crtc->panningTotalArea.x2;
            if (crtc->panningTotalArea.y2 > height)
                height = crtc->panningTotalArea.y2;
        }
    }

    if (width && height) {
        /*
         * Compute physical size of screen
         */
        if (monitorResolution) {
            mmWidth = width * 25.4 / monitorResolution;
            mmHeight = height * 25.4 / monitorResolution;
        }
        else {
            xf86OutputPtr output = xf86CompatOutput(pScrn);

            if (output &&
                output->conf_monitor &&
                (output->conf_monitor->mon_width > 0 &&
                 output->conf_monitor->mon_height > 0)) {
                /*
                 * Prefer user configured DisplaySize
                 */
                mmWidth = output->conf_monitor->mon_width;
                mmHeight = output->conf_monitor->mon_height;
            }
            else {
                /*
                 * Otherwise, just set the screen to DEFAULT_DPI
                 */
                mmWidth = width * 25.4 / DEFAULT_DPI;
                mmHeight = height * 25.4 / DEFAULT_DPI;
            }
        }
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "Setting screen physical size to %d x %d\n",
                   mmWidth, mmHeight);
        /*
         * This is the initial setting of the screen size.
         * We have to pre-set it here, otherwise panning would be adapted
         * to the new screen size.
         */
        pScreen->width = width;
        pScreen->height = height;
        xf86RandR12ScreenSetSize(pScreen, width, height, mmWidth, mmHeight);
    }

    if (randrp->virtualX == -1 || randrp->virtualY == -1) {
        randrp->virtualX = pScrn->virtualX;
        randrp->virtualY = pScrn->virtualY;
    }
    xf86CrtcSetScreenSubpixelOrder(pScreen);
#if RANDR_12_INTERFACE
    if (xf86RandR12CreateScreenResources12(pScreen))
        return TRUE;
#endif
    return TRUE;
}

Bool
xf86RandR12Init(ScreenPtr pScreen)
{
    rrScrPrivPtr rp;
    XF86RandRInfoPtr randrp;

#ifdef PANORAMIX
    /* XXX disable RandR when using Xinerama */
    if (!noPanoramiXExtension) {
        if (xf86NumScreens == 1)
            noPanoramiXExtension = TRUE;
        else
            return TRUE;
    }
#endif

    if (xf86RandR12Generation != serverGeneration)
        xf86RandR12Generation = serverGeneration;

    if (!dixRegisterPrivateKey(&xf86RandR12KeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    randrp = malloc(sizeof(XF86RandRInfoRec));
    if (!randrp)
        return FALSE;

    if (!RRScreenInit(pScreen)) {
        free(randrp);
        return FALSE;
    }
    rp = rrGetScrPriv(pScreen);
    rp->rrGetInfo = xf86RandR12GetInfo;
    rp->rrSetConfig = xf86RandR12SetConfig;

    randrp->virtualX = -1;
    randrp->virtualY = -1;
    randrp->mmWidth = pScreen->mmWidth;
    randrp->mmHeight = pScreen->mmHeight;

    randrp->rotation = RR_Rotate_0;     /* initial rotated mode */

    randrp->supported_rotations = RR_Rotate_0;

    randrp->maxX = randrp->maxY = 0;

    randrp->palette_size = 0;
    randrp->palette = NULL;

    dixSetPrivate(&pScreen->devPrivates, &xf86RandR12KeyRec, randrp);

#if RANDR_12_INTERFACE
    if (!xf86RandR12Init12(pScreen))
        return FALSE;
#endif
    return TRUE;
}

void
xf86RandR12CloseScreen(ScreenPtr pScreen)
{
    XF86RandRInfoPtr randrp;

    if (!dixPrivateKeyRegistered(&xf86RandR12KeyRec))
        return;

    randrp = XF86RANDRINFO(pScreen);
#if RANDR_12_INTERFACE
    xf86ScreenToScrn(pScreen)->EnterVT = randrp->orig_EnterVT;
    pScreen->ConstrainCursorHarder = randrp->orig_ConstrainCursorHarder;
#endif

    free(randrp->palette);
    free(randrp);
}

void
xf86RandR12SetRotations(ScreenPtr pScreen, Rotation rotations)
{
    XF86RandRInfoPtr randrp;

#if RANDR_12_INTERFACE
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    int c;
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
#endif

    if (!dixPrivateKeyRegistered(&xf86RandR12KeyRec))
        return;

    randrp = XF86RANDRINFO(pScreen);
#if RANDR_12_INTERFACE
    for (c = 0; c < config->num_crtc; c++) {
        xf86CrtcPtr crtc = config->crtc[c];

        RRCrtcSetRotations(crtc->randr_crtc, rotations);
    }
#endif
    randrp->supported_rotations = rotations;
}

void
xf86RandR12SetTransformSupport(ScreenPtr pScreen, Bool transforms)
{
#if RANDR_13_INTERFACE
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    int c;
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);

    if (!dixPrivateKeyRegistered(&xf86RandR12KeyRec))
        return;

    for (c = 0; c < config->num_crtc; c++) {
        xf86CrtcPtr crtc = config->crtc[c];

        RRCrtcSetTransformSupport(crtc->randr_crtc, transforms);
    }
#endif
}

void
xf86RandR12GetOriginalVirtualSize(ScrnInfoPtr pScrn, int *x, int *y)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);

    if (xf86RandR12Generation != serverGeneration ||
        XF86RANDRINFO(pScreen)->virtualX == -1) {
        *x = pScrn->virtualX;
        *y = pScrn->virtualY;
    }
    else {
        XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);

        *x = randrp->virtualX;
        *y = randrp->virtualY;
    }
}

#if RANDR_12_INTERFACE

#define FLAG_BITS (RR_HSyncPositive | \
		   RR_HSyncNegative | \
		   RR_VSyncPositive | \
		   RR_VSyncNegative | \
		   RR_Interlace | \
		   RR_DoubleScan | \
		   RR_CSync | \
		   RR_CSyncPositive | \
		   RR_CSyncNegative | \
		   RR_HSkewPresent | \
		   RR_BCast | \
		   RR_PixelMultiplex | \
		   RR_DoubleClock | \
		   RR_ClockDivideBy2)

static Bool
xf86RandRModeMatches(RRModePtr randr_mode, DisplayModePtr mode)
{
#if 0
    if (match_name) {
        /* check for same name */
        int len = strlen(mode->name);

        if (randr_mode->mode.nameLength != len)
            return FALSE;
        if (memcmp(randr_mode->name, mode->name, len) != 0)
            return FALSE;
    }
#endif

    /* check for same timings */
    if (randr_mode->mode.dotClock / 1000 != mode->Clock)
        return FALSE;
    if (randr_mode->mode.width != mode->HDisplay)
        return FALSE;
    if (randr_mode->mode.hSyncStart != mode->HSyncStart)
        return FALSE;
    if (randr_mode->mode.hSyncEnd != mode->HSyncEnd)
        return FALSE;
    if (randr_mode->mode.hTotal != mode->HTotal)
        return FALSE;
    if (randr_mode->mode.hSkew != mode->HSkew)
        return FALSE;
    if (randr_mode->mode.height != mode->VDisplay)
        return FALSE;
    if (randr_mode->mode.vSyncStart != mode->VSyncStart)
        return FALSE;
    if (randr_mode->mode.vSyncEnd != mode->VSyncEnd)
        return FALSE;
    if (randr_mode->mode.vTotal != mode->VTotal)
        return FALSE;

    /* check for same flags (using only the XF86 valid flag bits) */
    if ((randr_mode->mode.modeFlags & FLAG_BITS) != (mode->Flags & FLAG_BITS))
        return FALSE;

    /* everything matches */
    return TRUE;
}

static Bool
xf86RandR12CrtcNotify(RRCrtcPtr randr_crtc)
{
    ScreenPtr pScreen = randr_crtc->pScreen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    RRModePtr randr_mode = NULL;
    int x;
    int y;
    Rotation rotation;
    int numOutputs;
    RROutputPtr *randr_outputs;
    RROutputPtr randr_output;
    xf86CrtcPtr crtc = randr_crtc->devPrivate;
    xf86OutputPtr output;
    int i, j;
    DisplayModePtr mode = &crtc->mode;
    Bool ret;

    randr_outputs = xallocarray(config->num_output, sizeof(RROutputPtr));
    if (!randr_outputs)
        return FALSE;
    x = crtc->x;
    y = crtc->y;
    rotation = crtc->rotation;
    numOutputs = 0;
    randr_mode = NULL;
    for (i = 0; i < config->num_output; i++) {
        output = config->output[i];
        if (output->crtc == crtc) {
            randr_output = output->randr_output;
            randr_outputs[numOutputs++] = randr_output;
            /*
             * We make copies of modes, so pointer equality
             * isn't sufficient
             */
            for (j = 0; j < randr_output->numModes + randr_output->numUserModes;
                 j++) {
                RRModePtr m =
                    (j <
                     randr_output->numModes ? randr_output->
                     modes[j] : randr_output->userModes[j -
                                                        randr_output->
                                                        numModes]);

                if (xf86RandRModeMatches(m, mode)) {
                    randr_mode = m;
                    break;
                }
            }
        }
    }
    ret = RRCrtcNotify(randr_crtc, randr_mode, x, y,
                       rotation,
                       crtc->transformPresent ? &crtc->transform : NULL,
                       numOutputs, randr_outputs);
    free(randr_outputs);
    return ret;
}

/*
 * Convert a RandR mode to a DisplayMode
 */
static void
xf86RandRModeConvert(ScrnInfoPtr scrn,
                     RRModePtr randr_mode, DisplayModePtr mode)
{
    memset(mode, 0, sizeof(DisplayModeRec));
    mode->status = MODE_OK;

    mode->Clock = randr_mode->mode.dotClock / 1000;

    mode->HDisplay = randr_mode->mode.width;
    mode->HSyncStart = randr_mode->mode.hSyncStart;
    mode->HSyncEnd = randr_mode->mode.hSyncEnd;
    mode->HTotal = randr_mode->mode.hTotal;
    mode->HSkew = randr_mode->mode.hSkew;

    mode->VDisplay = randr_mode->mode.height;
    mode->VSyncStart = randr_mode->mode.vSyncStart;
    mode->VSyncEnd = randr_mode->mode.vSyncEnd;
    mode->VTotal = randr_mode->mode.vTotal;
    mode->VScan = 0;

    mode->Flags = randr_mode->mode.modeFlags & FLAG_BITS;

    xf86SetModeCrtc(mode, scrn->adjustFlags);
}

static Bool
xf86RandR12CrtcSet(ScreenPtr pScreen,
                   RRCrtcPtr randr_crtc,
                   RRModePtr randr_mode,
                   int x,
                   int y,
                   Rotation rotation,
                   int num_randr_outputs, RROutputPtr * randr_outputs)
{
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    xf86CrtcPtr crtc = randr_crtc->devPrivate;
    RRTransformPtr transform;
    Bool changed = FALSE;
    int o, ro;
    xf86CrtcPtr *save_crtcs;
    Bool save_enabled = crtc->enabled;

    if (!crtc->scrn->vtSema)
        return FALSE;

    save_crtcs = xallocarray(config->num_output, sizeof(xf86CrtcPtr));
    if ((randr_mode != NULL) != crtc->enabled)
        changed = TRUE;
    else if (randr_mode && !xf86RandRModeMatches(randr_mode, &crtc->mode))
        changed = TRUE;

    if (rotation != crtc->rotation)
        changed = TRUE;

    if (crtc->current_scanout != randr_crtc->scanout_pixmap ||
        crtc->current_scanout_back != randr_crtc->scanout_pixmap_back)
        changed = TRUE;

    transform = RRCrtcGetTransform(randr_crtc);
    if ((transform != NULL) != crtc->transformPresent)
        changed = TRUE;
    else if (transform &&
             !RRTransformEqual(transform, &crtc->transform))
        changed = TRUE;

    if (x != crtc->x || y != crtc->y)
        changed = TRUE;
    for (o = 0; o < config->num_output; o++) {
        xf86OutputPtr output = config->output[o];
        xf86CrtcPtr new_crtc;

        save_crtcs[o] = output->crtc;

        if (output->crtc == crtc)
            new_crtc = NULL;
        else
            new_crtc = output->crtc;
        for (ro = 0; ro < num_randr_outputs; ro++)
            if (output->randr_output == randr_outputs[ro]) {
                new_crtc = crtc;
                break;
            }
        if (new_crtc != output->crtc) {
            changed = TRUE;
            output->crtc = new_crtc;
        }
    }
    for (ro = 0; ro < num_randr_outputs; ro++)
        if (randr_outputs[ro]->pendingProperties)
            changed = TRUE;

    /* XXX need device-independent mode setting code through an API */
    if (changed) {
        crtc->enabled = randr_mode != NULL;

        if (randr_mode) {
            DisplayModeRec mode;

            xf86RandRModeConvert(pScrn, randr_mode, &mode);
            if (!xf86CrtcSetModeTransform
                (crtc, &mode, rotation, transform, x, y)) {
                crtc->enabled = save_enabled;
                for (o = 0; o < config->num_output; o++) {
                    xf86OutputPtr output = config->output[o];

                    output->crtc = save_crtcs[o];
                }
                free(save_crtcs);
                return FALSE;
            }
            xf86RandR13VerifyPanningArea(crtc, pScreen->width, pScreen->height);
            xf86RandR13Pan(crtc, randrp->pointerX, randrp->pointerY);
            randrp->panning = PANNING_ENABLED (crtc);
            /*
             * Save the last successful setting for EnterVT
             */
            xf86SaveModeContents(&crtc->desiredMode, &mode);
            crtc->desiredRotation = rotation;
            crtc->current_scanout = randr_crtc->scanout_pixmap;
            crtc->current_scanout_back = randr_crtc->scanout_pixmap_back;
            if (transform) {
                crtc->desiredTransform = *transform;
                crtc->desiredTransformPresent = TRUE;
            }
            else
                crtc->desiredTransformPresent = FALSE;

            crtc->desiredX = x;
            crtc->desiredY = y;
        }
        xf86DisableUnusedFunctions(pScrn);
    }
    free(save_crtcs);
    return xf86RandR12CrtcNotify(randr_crtc);
}

static void
xf86RandR12CrtcComputeGamma(xf86CrtcPtr crtc, LOCO *palette,
                            int palette_red_size, int palette_green_size,
                            int palette_blue_size, CARD16 *gamma_red,
                            CARD16 *gamma_green, CARD16 *gamma_blue,
                            int gamma_size)
{
    int gamma_slots;
    unsigned shift;
    int i, j;
    CARD32 value = 0;

    for (shift = 0; (gamma_size << shift) < (1 << 16); shift++);

    if (crtc->gamma_size >= palette_red_size) {
        /* Upsampling of smaller palette to larger hw lut size */
        gamma_slots = crtc->gamma_size / palette_red_size;
        for (i = 0; i < palette_red_size; i++) {
            value = palette[i].red;
            if (gamma_red)
                value = gamma_red[value];
            else
                value <<= shift;

            for (j = 0; j < gamma_slots; j++)
                crtc->gamma_red[i * gamma_slots + j] = value;
        }

        /* Replicate last value until end of crtc for gamma_size not a power of 2 */
        for (j = i * gamma_slots; j < crtc->gamma_size; j++)
                crtc->gamma_red[j] = value;
    } else {
        /* Downsampling of larger palette to smaller hw lut size */
        for (i = 0; i < crtc->gamma_size; i++) {
            value = palette[i * (palette_red_size - 1) / (crtc->gamma_size - 1)].red;
            if (gamma_red)
                value = gamma_red[value];
            else
                value <<= shift;

            crtc->gamma_red[i] = value;
        }
    }

    if (crtc->gamma_size >= palette_green_size) {
        /* Upsampling of smaller palette to larger hw lut size */
        gamma_slots = crtc->gamma_size / palette_green_size;
        for (i = 0; i < palette_green_size; i++) {
            value = palette[i].green;
            if (gamma_green)
                value = gamma_green[value];
            else
                value <<= shift;

            for (j = 0; j < gamma_slots; j++)
                crtc->gamma_green[i * gamma_slots + j] = value;
        }

        /* Replicate last value until end of crtc for gamma_size not a power of 2 */
        for (j = i * gamma_slots; j < crtc->gamma_size; j++)
            crtc->gamma_green[j] = value;
    } else {
        /* Downsampling of larger palette to smaller hw lut size */
        for (i = 0; i < crtc->gamma_size; i++) {
            value = palette[i * (palette_green_size - 1) / (crtc->gamma_size - 1)].green;
            if (gamma_green)
                value = gamma_green[value];
            else
                value <<= shift;

            crtc->gamma_green[i] = value;
        }
    }

    if (crtc->gamma_size >= palette_blue_size) {
        /* Upsampling of smaller palette to larger hw lut size */
        gamma_slots = crtc->gamma_size / palette_blue_size;
        for (i = 0; i < palette_blue_size; i++) {
            value = palette[i].blue;
            if (gamma_blue)
                value = gamma_blue[value];
            else
                value <<= shift;

            for (j = 0; j < gamma_slots; j++)
                crtc->gamma_blue[i * gamma_slots + j] = value;
        }

        /* Replicate last value until end of crtc for gamma_size not a power of 2 */
        for (j = i * gamma_slots; j < crtc->gamma_size; j++)
            crtc->gamma_blue[j] = value;
    } else {
        /* Downsampling of larger palette to smaller hw lut size */
        for (i = 0; i < crtc->gamma_size; i++) {
            value = palette[i * (palette_blue_size - 1) / (crtc->gamma_size - 1)].blue;
            if (gamma_blue)
                value = gamma_blue[value];
            else
                value <<= shift;

            crtc->gamma_blue[i] = value;
        }
    }
}

static void
xf86RandR12CrtcReloadGamma(xf86CrtcPtr crtc)
{
    if (!crtc->scrn->vtSema || !crtc->funcs->gamma_set)
        return;

    /* Only set it when the crtc is actually running.
     * Otherwise it will be set when it's activated.
     */
    if (crtc->active)
        crtc->funcs->gamma_set(crtc, crtc->gamma_red, crtc->gamma_green,
                               crtc->gamma_blue, crtc->gamma_size);
}

static Bool
xf86RandR12CrtcSetGamma(ScreenPtr pScreen, RRCrtcPtr randr_crtc)
{
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    xf86CrtcPtr crtc = randr_crtc->devPrivate;
    int max_size = crtc->gamma_size;

    if (crtc->funcs->gamma_set == NULL)
        return FALSE;

    if (randrp->palette_size) {
        xf86RandR12CrtcComputeGamma(crtc, randrp->palette,
                                    randrp->palette_red_size,
                                    randrp->palette_green_size,
                                    randrp->palette_blue_size,
                                    randr_crtc->gammaRed,
                                    randr_crtc->gammaGreen,
                                    randr_crtc->gammaBlue,
                                    randr_crtc->gammaSize);
    } else {
        if (max_size > randr_crtc->gammaSize)
            max_size = randr_crtc->gammaSize;

        memcpy(crtc->gamma_red, randr_crtc->gammaRed,
               max_size * sizeof(crtc->gamma_red[0]));
        memcpy(crtc->gamma_green, randr_crtc->gammaGreen,
               max_size * sizeof(crtc->gamma_green[0]));
        memcpy(crtc->gamma_blue, randr_crtc->gammaBlue,
               max_size * sizeof(crtc->gamma_blue[0]));
    }

    xf86RandR12CrtcReloadGamma(crtc);

    return TRUE;
}

static void
init_one_component(CARD16 *comp, unsigned size, float gamma)
{
    int i;
    unsigned shift;

    for (shift = 0; (size << shift) < (1 << 16); shift++);

    if (gamma == 1.0) {
        for (i = 0; i < size; i++)
            comp[i] = i << shift;
    } else {
        for (i = 0; i < size; i++)
            comp[i] = (CARD16) (pow((double) i / (double) (size - 1),
                                   1. / (double) gamma) *
                               (double) (size - 1) * (1 << shift));
    }
}

static Bool
xf86RandR12CrtcInitGamma(xf86CrtcPtr crtc, float gamma_red, float gamma_green,
                         float gamma_blue)
{
    unsigned size = crtc->randr_crtc->gammaSize;
    CARD16 *red, *green, *blue;

    if (!crtc->funcs->gamma_set &&
        (gamma_red != 1.0f || gamma_green != 1.0f || gamma_blue != 1.0f))
        return FALSE;

    red = xallocarray(size, 3 * sizeof(CARD16));
    if (!red)
        return FALSE;

    green = red + size;
    blue = green + size;

    init_one_component(red, size, gamma_red);
    init_one_component(green, size, gamma_green);
    init_one_component(blue, size, gamma_blue);

    RRCrtcGammaSet(crtc->randr_crtc, red, green, blue);
    free(red);

    return TRUE;
}

static Bool
xf86RandR12OutputInitGamma(xf86OutputPtr output)
{
    XF86ConfMonitorPtr mon = output->conf_monitor;
    float gamma_red = 1.0, gamma_green = 1.0, gamma_blue = 1.0;

    if (!mon)
        return TRUE;

    /* Get configured values, where they exist. */
    if (mon->mon_gamma_red >= GAMMA_MIN && mon->mon_gamma_red <= GAMMA_MAX)
        gamma_red = mon->mon_gamma_red;

    if (mon->mon_gamma_green >= GAMMA_MIN && mon->mon_gamma_green <= GAMMA_MAX)
        gamma_green = mon->mon_gamma_green;

    if (mon->mon_gamma_blue >= GAMMA_MIN && mon->mon_gamma_blue <= GAMMA_MAX)
        gamma_blue = mon->mon_gamma_blue;

    /* Don't set gamma 1.0 if another cloned output on this CRTC already set a
     * different gamma
     */
    if (gamma_red != 1.0 || gamma_green != 1.0 || gamma_blue != 1.0) {
        if (!output->crtc->randr_crtc) {
            xf86DrvMsg(output->scrn->scrnIndex, X_WARNING,
                       "Gamma correction for output %s not possible because "
                       "RandR is disabled\n", output->name);
            return TRUE;
        }

        xf86DrvMsg(output->scrn->scrnIndex, X_INFO,
                   "Output %s wants gamma correction (%.1f, %.1f, %.1f)\n",
                   output->name, gamma_red, gamma_green, gamma_blue);
        return xf86RandR12CrtcInitGamma(output->crtc, gamma_red, gamma_green,
                                        gamma_blue);
    }

    return TRUE;
}

Bool
xf86RandR12InitGamma(ScrnInfoPtr pScrn, unsigned gammaSize) {
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    int o, c;

    /* Set default gamma for all CRTCs
     * This is done to avoid problems later on with cloned outputs
     */
    for (c = 0; c < config->num_crtc; c++) {
        xf86CrtcPtr crtc = config->crtc[c];

        if (!crtc->randr_crtc)
            continue;

        if (!RRCrtcGammaSetSize(crtc->randr_crtc, gammaSize) ||
            !xf86RandR12CrtcInitGamma(crtc, 1.0f, 1.0f, 1.0f))
            return FALSE;
    }

    /* Set initial gamma per monitor configuration
     */
    for (o = 0; o < config->num_output; o++) {
        xf86OutputPtr output = config->output[o];

        if (output->crtc &&
            !xf86RandR12OutputInitGamma(output))
            xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                       "Initial gamma correction for output %s: failed.\n",
                       output->name);
    }

    return TRUE;
}

static Bool
xf86RandR12OutputSetProperty(ScreenPtr pScreen,
                             RROutputPtr randr_output,
                             Atom property, RRPropertyValuePtr value)
{
    xf86OutputPtr output = randr_output->devPrivate;

    /* If we don't have any property handler, then we don't care what the
     * user is setting properties to.
     */
    if (output->funcs->set_property == NULL)
        return TRUE;

    /*
     * This function gets called even when vtSema is FALSE, as
     * drivers will need to remember the correct value to apply
     * when the VT switch occurs
     */
    return output->funcs->set_property(output, property, value);
}

static Bool
xf86RandR13OutputGetProperty(ScreenPtr pScreen,
                             RROutputPtr randr_output, Atom property)
{
    xf86OutputPtr output = randr_output->devPrivate;

    if (output->funcs->get_property == NULL)
        return TRUE;

    /* Should be safe even w/o vtSema */
    return output->funcs->get_property(output, property);
}

static Bool
xf86RandR12OutputValidateMode(ScreenPtr pScreen,
                              RROutputPtr randr_output, RRModePtr randr_mode)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86OutputPtr output = randr_output->devPrivate;
    DisplayModeRec mode;

    xf86RandRModeConvert(pScrn, randr_mode, &mode);
    /*
     * This function may be called when vtSema is FALSE, so
     * the underlying function must either avoid touching the hardware
     * or return FALSE when vtSema is FALSE
     */
    if (output->funcs->mode_valid(output, &mode) != MODE_OK)
        return FALSE;
    return TRUE;
}

static void
xf86RandR12ModeDestroy(ScreenPtr pScreen, RRModePtr randr_mode)
{
}

/**
 * Given a list of xf86 modes and a RandR Output object, construct
 * RandR modes and assign them to the output
 */
static Bool
xf86RROutputSetModes(RROutputPtr randr_output, DisplayModePtr modes)
{
    DisplayModePtr mode;
    RRModePtr *rrmodes = NULL;
    int nmode = 0;
    int npreferred = 0;
    Bool ret = TRUE;
    int pref;

    for (mode = modes; mode; mode = mode->next)
        nmode++;

    if (nmode) {
        rrmodes = xallocarray(nmode, sizeof(RRModePtr));

        if (!rrmodes)
            return FALSE;
        nmode = 0;

        for (pref = 1; pref >= 0; pref--) {
            for (mode = modes; mode; mode = mode->next) {
                if ((pref != 0) == ((mode->type & M_T_PREFERRED) != 0)) {
                    xRRModeInfo modeInfo;
                    RRModePtr rrmode;

                    modeInfo.nameLength = strlen(mode->name);
                    modeInfo.width = mode->HDisplay;
                    modeInfo.dotClock = mode->Clock * 1000;
                    modeInfo.hSyncStart = mode->HSyncStart;
                    modeInfo.hSyncEnd = mode->HSyncEnd;
                    modeInfo.hTotal = mode->HTotal;
                    modeInfo.hSkew = mode->HSkew;

                    modeInfo.height = mode->VDisplay;
                    modeInfo.vSyncStart = mode->VSyncStart;
                    modeInfo.vSyncEnd = mode->VSyncEnd;
                    modeInfo.vTotal = mode->VTotal;
                    modeInfo.modeFlags = mode->Flags;

                    rrmode = RRModeGet(&modeInfo, mode->name);
                    if (rrmode) {
                        rrmodes[nmode++] = rrmode;
                        npreferred += pref;
                    }
                }
            }
        }
    }

    ret = RROutputSetModes(randr_output, rrmodes, nmode, npreferred);
    free(rrmodes);
    return ret;
}

/*
 * Mirror the current mode configuration to RandR
 */
static Bool
xf86RandR12SetInfo12(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    RROutputPtr *clones;
    RRCrtcPtr *crtcs;
    int ncrtc;
    int o, c, l;
    int nclone;

    clones = xallocarray(config->num_output, sizeof(RROutputPtr));
    crtcs = xallocarray(config->num_crtc, sizeof(RRCrtcPtr));
    for (o = 0; o < config->num_output; o++) {
        xf86OutputPtr output = config->output[o];

        ncrtc = 0;
        for (c = 0; c < config->num_crtc; c++)
            if (output->possible_crtcs & (1 << c))
                crtcs[ncrtc++] = config->crtc[c]->randr_crtc;

        if (!RROutputSetCrtcs(output->randr_output, crtcs, ncrtc)) {
            free(crtcs);
            free(clones);
            return FALSE;
        }

        RROutputSetPhysicalSize(output->randr_output,
                                output->mm_width, output->mm_height);
        xf86RROutputSetModes(output->randr_output, output->probed_modes);

        switch (output->status) {
        case XF86OutputStatusConnected:
            RROutputSetConnection(output->randr_output, RR_Connected);
            break;
        case XF86OutputStatusDisconnected:
	    if (xf86OutputForceEnabled(output))
                RROutputSetConnection(output->randr_output, RR_Connected);
	    else
                RROutputSetConnection(output->randr_output, RR_Disconnected);
            break;
        case XF86OutputStatusUnknown:
            RROutputSetConnection(output->randr_output, RR_UnknownConnection);
            break;
        }

        RROutputSetSubpixelOrder(output->randr_output, output->subpixel_order);

        /*
         * Valid clones
         */
        nclone = 0;
        for (l = 0; l < config->num_output; l++) {
            xf86OutputPtr clone = config->output[l];

            if (l != o && (output->possible_clones & (1 << l)))
                clones[nclone++] = clone->randr_output;
        }
        if (!RROutputSetClones(output->randr_output, clones, nclone)) {
            free(crtcs);
            free(clones);
            return FALSE;
        }
    }
    free(crtcs);
    free(clones);
    return TRUE;
}

/*
 * Query the hardware for the current state, then mirror
 * that to RandR
 */
static Bool
xf86RandR12GetInfo12(ScreenPtr pScreen, Rotation * rotations)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

    if (!pScrn->vtSema)
        return TRUE;
    xf86ProbeOutputModes(pScrn, 0, 0);
    xf86SetScrnInfoModes(pScrn);
    return xf86RandR12SetInfo12(pScreen);
}

static Bool
xf86RandR12CreateObjects12(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    int c;
    int o;

    if (!RRInit())
        return FALSE;

    /*
     * Configure crtcs
     */
    for (c = 0; c < config->num_crtc; c++) {
        xf86CrtcPtr crtc = config->crtc[c];

        crtc->randr_crtc = RRCrtcCreate(pScreen, crtc);
    }
    /*
     * Configure outputs
     */
    for (o = 0; o < config->num_output; o++) {
        xf86OutputPtr output = config->output[o];

        output->randr_output = RROutputCreate(pScreen, output->name,
                                              strlen(output->name), output);

        if (output->funcs->create_resources != NULL)
            output->funcs->create_resources(output);
        RRPostPendingProperties(output->randr_output);
    }

    if (config->name) {
        config->randr_provider = RRProviderCreate(pScreen, config->name,
                                                  strlen(config->name));

        RRProviderSetCapabilities(config->randr_provider, pScrn->capabilities);
    }

    return TRUE;
}

static void
xf86RandR12CreateMonitors(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    int o, ot;
    int ht, vt;
    int ret;
    char buf[25];

    for (o = 0; o < config->num_output; o++) {
        xf86OutputPtr output = config->output[o];
        struct xf86CrtcTileInfo *tile_info = &output->tile_info, *this_tile;
        RRMonitorPtr monitor;
        int output_num, num_outputs;
        if (!tile_info->group_id)
            continue;

        if (tile_info->tile_h_loc ||
            tile_info->tile_v_loc)
            continue;

        num_outputs = tile_info->num_h_tile * tile_info->num_v_tile;

        monitor = RRMonitorAlloc(num_outputs);
        if (!monitor)
            return;
        monitor->pScreen = pScreen;
        snprintf(buf, 25, "Auto-Monitor-%d", tile_info->group_id);
        monitor->name = MakeAtom(buf, strlen(buf), TRUE);
        monitor->primary = 0;
        monitor->automatic = TRUE;
        memset(&monitor->geometry.box, 0, sizeof(monitor->geometry.box));

        output_num = 0;
        for (ht = 0; ht < tile_info->num_h_tile; ht++) {
            for (vt = 0; vt < tile_info->num_v_tile; vt++) {

                for (ot = 0; ot < config->num_output; ot++) {
                    this_tile = &config->output[ot]->tile_info;

                    if (this_tile->group_id != tile_info->group_id)
                        continue;

                    if (this_tile->tile_h_loc != ht ||
                        this_tile->tile_v_loc != vt)
                        continue;

                    monitor->outputs[output_num] = config->output[ot]->randr_output->id;
                    output_num++;

                }

            }
        }

        ret = RRMonitorAdd(serverClient, pScreen, monitor);
        if (ret) {
            RRMonitorFree(monitor);
            return;
        }
    }
}

static Bool
xf86RandR12CreateScreenResources12(ScreenPtr pScreen)
{
    int c;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    rrScrPrivPtr rp = rrGetScrPriv(pScreen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);

    for (c = 0; c < config->num_crtc; c++)
        xf86RandR12CrtcNotify(config->crtc[c]->randr_crtc);

    RRScreenSetSizeRange(pScreen, config->minWidth, config->minHeight,
                         config->maxWidth, config->maxHeight);

    xf86RandR12CreateMonitors(pScreen);

    if (!pScreen->isGPU) {
        rp->primaryOutput = config->output[0]->randr_output;
        RROutputChanged(rp->primaryOutput, FALSE);
        rp->layoutChanged = TRUE;
    }

    return TRUE;
}

/*
 * Something happened within the screen configuration due
 * to DGA, VidMode or hot key. Tell RandR
 */

void
xf86RandR12TellChanged(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    int c;

    xf86RandR12SetInfo12(pScreen);
    for (c = 0; c < config->num_crtc; c++)
        xf86RandR12CrtcNotify(config->crtc[c]->randr_crtc);

    RRTellChanged(pScreen);
}

static void
xf86RandR12PointerMoved(ScrnInfoPtr pScrn, int x, int y)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    int c;

    randrp->pointerX = x;
    randrp->pointerY = y;
    for (c = 0; c < config->num_crtc; c++)
        xf86RandR13Pan(config->crtc[c], x, y);
}

static Bool
xf86RandR13GetPanning(ScreenPtr pScreen,
                      RRCrtcPtr randr_crtc,
                      BoxPtr totalArea, BoxPtr trackingArea, INT16 *border)
{
    xf86CrtcPtr crtc = randr_crtc->devPrivate;

    if (crtc->version < 2)
        return FALSE;
    if (totalArea)
        memcpy(totalArea, &crtc->panningTotalArea, sizeof(BoxRec));
    if (trackingArea)
        memcpy(trackingArea, &crtc->panningTrackingArea, sizeof(BoxRec));
    if (border)
        memcpy(border, crtc->panningBorder, 4 * sizeof(INT16));

    return TRUE;
}

static Bool
xf86RandR13SetPanning(ScreenPtr pScreen,
                      RRCrtcPtr randr_crtc,
                      BoxPtr totalArea, BoxPtr trackingArea, INT16 *border)
{
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    xf86CrtcPtr crtc = randr_crtc->devPrivate;
    BoxRec oldTotalArea;
    BoxRec oldTrackingArea;
    INT16 oldBorder[4];
    Bool oldPanning = randrp->panning;

    if (crtc->version < 2)
        return FALSE;

    memcpy(&oldTotalArea, &crtc->panningTotalArea, sizeof(BoxRec));
    memcpy(&oldTrackingArea, &crtc->panningTrackingArea, sizeof(BoxRec));
    memcpy(oldBorder, crtc->panningBorder, 4 * sizeof(INT16));

    if (totalArea)
        memcpy(&crtc->panningTotalArea, totalArea, sizeof(BoxRec));
    if (trackingArea)
        memcpy(&crtc->panningTrackingArea, trackingArea, sizeof(BoxRec));
    if (border)
        memcpy(crtc->panningBorder, border, 4 * sizeof(INT16));

    if (xf86RandR13VerifyPanningArea(crtc, pScreen->width, pScreen->height)) {
        xf86RandR13Pan(crtc, randrp->pointerX, randrp->pointerY);
        randrp->panning = PANNING_ENABLED (crtc);
        return TRUE;
    }
    else {
        /* Restore old settings */
        memcpy(&crtc->panningTotalArea, &oldTotalArea, sizeof(BoxRec));
        memcpy(&crtc->panningTrackingArea, &oldTrackingArea, sizeof(BoxRec));
        memcpy(crtc->panningBorder, oldBorder, 4 * sizeof(INT16));
        randrp->panning = oldPanning;
        return FALSE;
    }
}

/*
 * Compatibility with colormaps and XF86VidMode's gamma
 */
void
xf86RandR12LoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
                       LOCO *colors, VisualPtr pVisual)
{
    ScreenPtr pScreen = pScrn->pScreen;
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    int reds, greens, blues, index, palette_size;
    int c, i;

    if (pVisual->class == TrueColor || pVisual->class == DirectColor) {
        reds = (pVisual->redMask >> pVisual->offsetRed) + 1;
        greens = (pVisual->greenMask >> pVisual->offsetGreen) + 1;
        blues = (pVisual->blueMask >> pVisual->offsetBlue) + 1;
    } else {
        reds = greens = blues = pVisual->ColormapEntries;
    }

    palette_size = max(reds, max(greens, blues));

    if (dixPrivateKeyRegistered(rrPrivKey)) {
        XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);

        if (randrp->palette_size != palette_size) {
            randrp->palette = reallocarray(randrp->palette, palette_size,
                                           sizeof(colors[0]));
            if (!randrp->palette) {
                randrp->palette_size = 0;
                return;
            }

            randrp->palette_size = palette_size;
        }
        randrp->palette_red_size = reds;
        randrp->palette_green_size = greens;
        randrp->palette_blue_size = blues;

        for (i = 0; i < numColors; i++) {
            index = indices[i];

            if (index < reds)
                randrp->palette[index].red = colors[index].red;
            if (index < greens)
                randrp->palette[index].green = colors[index].green;
            if (index < blues)
                randrp->palette[index].blue = colors[index].blue;
        }
    }

    for (c = 0; c < config->num_crtc; c++) {
        xf86CrtcPtr crtc = config->crtc[c];
        RRCrtcPtr randr_crtc = crtc->randr_crtc;

        if (randr_crtc) {
            xf86RandR12CrtcComputeGamma(crtc, colors, reds, greens, blues,
                                        randr_crtc->gammaRed,
                                        randr_crtc->gammaGreen,
                                        randr_crtc->gammaBlue,
                                        randr_crtc->gammaSize);
        } else {
            xf86RandR12CrtcComputeGamma(crtc, colors, reds, greens, blues,
                                        NULL, NULL, NULL,
                                        xf86GetGammaRampSize(pScreen));
        }
        xf86RandR12CrtcReloadGamma(crtc);
    }
}

/*
 * Compatibility pScrn->ChangeGamma provider for ddx drivers which do not call
 * xf86HandleColormaps(). Note such drivers really should be fixed to call
 * xf86HandleColormaps() as this clobbers the per-CRTC gamma ramp of the CRTC
 * assigned to the RandR compatibility output.
 */
static int
xf86RandR12ChangeGamma(ScrnInfoPtr pScrn, Gamma gamma)
{
    RRCrtcPtr randr_crtc = xf86CompatRRCrtc(pScrn);
    int size;

    if (!randr_crtc || pScrn->LoadPalette == xf86RandR12LoadPalette)
        return Success;

    size = max(0, randr_crtc->gammaSize);
    if (!size)
        return Success;

    init_one_component(randr_crtc->gammaRed, size, gamma.red);
    init_one_component(randr_crtc->gammaGreen, size, gamma.green);
    init_one_component(randr_crtc->gammaBlue, size, gamma.blue);
    xf86RandR12CrtcSetGamma(xf86ScrnToScreen(pScrn), randr_crtc);

    pScrn->gamma = gamma;

    return Success;
}

static Bool
xf86RandR12EnterVT(ScrnInfoPtr pScrn)
{
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);
    rrScrPrivPtr rp = rrGetScrPriv(pScreen);
    Bool ret;
    int i;

    if (randrp->orig_EnterVT) {
        pScrn->EnterVT = randrp->orig_EnterVT;
        ret = pScrn->EnterVT(pScrn);
        randrp->orig_EnterVT = pScrn->EnterVT;
        pScrn->EnterVT = xf86RandR12EnterVT;
        if (!ret)
            return FALSE;
    }

    /* reload gamma */
    for (i = 0; i < rp->numCrtcs; i++)
        xf86RandR12CrtcReloadGamma(rp->crtcs[i]->devPrivate);

    return RRGetInfo(pScreen, TRUE);    /* force a re-probe of outputs and notify clients about changes */
}

static void
xf86DetachOutputGPU(ScreenPtr pScreen)
{
    rrScrPrivPtr rp = rrGetScrPriv(pScreen);
    int i;

    /* make sure there are no attached shared scanout pixmaps first */
    for (i = 0; i < rp->numCrtcs; i++)
        RRCrtcDetachScanoutPixmap(rp->crtcs[i]);

    DetachOutputGPU(pScreen);
}

static Bool
xf86RandR14ProviderSetOutputSource(ScreenPtr pScreen,
                                   RRProviderPtr provider,
                                   RRProviderPtr source_provider)
{
    if (!source_provider) {
        if (provider->output_source) {
            xf86DetachOutputGPU(pScreen);
        }
        provider->output_source = NULL;
        return TRUE;
    }

    if (provider->output_source == source_provider)
        return TRUE;

    SetRootClip(source_provider->pScreen, ROOT_CLIP_NONE);

    AttachOutputGPU(source_provider->pScreen, pScreen);

    provider->output_source = source_provider;
    SetRootClip(source_provider->pScreen, ROOT_CLIP_FULL);
    return TRUE;
}

static Bool
xf86RandR14ProviderSetOffloadSink(ScreenPtr pScreen,
                                  RRProviderPtr provider,
                                  RRProviderPtr sink_provider)
{
    if (!sink_provider) {
        if (provider->offload_sink) {
            xf86DetachOutputGPU(pScreen);
        }

        provider->offload_sink = NULL;
        return TRUE;
    }

    if (provider->offload_sink == sink_provider)
        return TRUE;

    AttachOffloadGPU(sink_provider->pScreen, pScreen);

    provider->offload_sink = sink_provider;
    return TRUE;
}

static Bool
xf86RandR14ProviderSetProperty(ScreenPtr pScreen,
                             RRProviderPtr randr_provider,
                             Atom property, RRPropertyValuePtr value)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);

    /* If we don't have any property handler, then we don't care what the
     * user is setting properties to.
     */
    if (config->provider_funcs->set_property == NULL)
        return TRUE;

    /*
     * This function gets called even when vtSema is FALSE, as
     * drivers will need to remember the correct value to apply
     * when the VT switch occurs
     */
    return config->provider_funcs->set_property(pScrn, property, value);
}

static Bool
xf86RandR14ProviderGetProperty(ScreenPtr pScreen,
                               RRProviderPtr randr_provider, Atom property)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);

    if (config->provider_funcs->get_property == NULL)
        return TRUE;

    /* Should be safe even w/o vtSema */
    return config->provider_funcs->get_property(pScrn, property);
}

static Bool
xf86CrtcSetScanoutPixmap(RRCrtcPtr randr_crtc, PixmapPtr pixmap)
{
    xf86CrtcPtr crtc = randr_crtc->devPrivate;
    if (!crtc->funcs->set_scanout_pixmap)
        return FALSE;
    return crtc->funcs->set_scanout_pixmap(crtc, pixmap);
}

static void
xf86RandR13ConstrainCursorHarder(DeviceIntPtr dev, ScreenPtr screen, int mode, int *x, int *y)
{
    XF86RandRInfoPtr randrp = XF86RANDRINFO(screen);

    if (randrp->panning)
        return;

    if (randrp->orig_ConstrainCursorHarder) {
        screen->ConstrainCursorHarder = randrp->orig_ConstrainCursorHarder;
        screen->ConstrainCursorHarder(dev, screen, mode, x, y);
        screen->ConstrainCursorHarder = xf86RandR13ConstrainCursorHarder;
    }
}

static void
xf86RandR14ProviderDestroy(ScreenPtr screen, RRProviderPtr provider)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);

    if (config->randr_provider == provider) {
        if (config->randr_provider->offload_sink) {
            DetachOffloadGPU(screen);
            config->randr_provider->offload_sink = NULL;
            RRSetChanged(screen);
        }
        if (config->randr_provider->output_source) {
            xf86DetachOutputGPU(screen);
            config->randr_provider->output_source = NULL;
            RRSetChanged(screen);
        }
        if (screen->current_primary)
            DetachUnboundGPU(screen);
    }
    config->randr_provider = NULL;
}

static void
xf86CrtcCheckReset(xf86CrtcPtr crtc) {
    if (xf86CrtcInUse(crtc)) {
        RRTransformPtr transform;

        if (crtc->desiredTransformPresent)
            transform = &crtc->desiredTransform;
        else
            transform = NULL;
        xf86CrtcSetModeTransform(crtc, &crtc->desiredMode,
                                 crtc->desiredRotation, transform,
                                 crtc->desiredX, crtc->desiredY);
        xf86_crtc_show_cursor(crtc);
    }
}

void
xf86CrtcLeaseTerminated(RRLeasePtr lease)
{
    int c;
    int o;
    ScrnInfoPtr scrn = xf86ScreenToScrn(lease->screen);

    RRLeaseTerminated(lease);
    /*
     * Force a full mode set on any crtc in the expiring lease which
     * was running before the lease started
     */
    for (c = 0; c < lease->numCrtcs; c++) {
        RRCrtcPtr randr_crtc = lease->crtcs[c];
        xf86CrtcPtr crtc = randr_crtc->devPrivate;

        xf86CrtcCheckReset(crtc);
    }

    /* Check to see if any leased output is using a crtc which
     * was not reset in the above loop
     */
    for (o = 0; o < lease->numOutputs; o++) {
        RROutputPtr randr_output = lease->outputs[o];
        xf86OutputPtr output = randr_output->devPrivate;
        xf86CrtcPtr crtc = output->crtc;

        if (crtc) {
            for (c = 0; c < lease->numCrtcs; c++)
                if (lease->crtcs[c] == crtc->randr_crtc)
                    break;
            if (c != lease->numCrtcs)
                continue;
            xf86CrtcCheckReset(crtc);
        }
    }

    /* Power off if necessary */
    xf86DisableUnusedFunctions(scrn);

    RRLeaseFree(lease);
}

static Bool
xf86CrtcSoleOutput(xf86CrtcPtr crtc, xf86OutputPtr output)
{
    ScrnInfoPtr scrn = crtc->scrn;
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
    int o;

    for (o = 0; o < config->num_output; o++) {
        xf86OutputPtr other = config->output[o];

        if (other != output && other->crtc == crtc)
            return FALSE;
    }
    return TRUE;
}

void
xf86CrtcLeaseStarted(RRLeasePtr lease)
{
    int c;
    int o;

    for (c = 0; c < lease->numCrtcs; c++) {
        RRCrtcPtr randr_crtc = lease->crtcs[c];
        xf86CrtcPtr crtc = randr_crtc->devPrivate;

        if (crtc->enabled) {
            /*
             * Leave the primary plane enabled so we can
             * flip without blanking the screen. Hide
             * the cursor so it doesn't remain on the screen
             * while the lease is active
             */
            xf86_crtc_hide_cursor(crtc);
            crtc->enabled = FALSE;
        }
    }
    for (o = 0; o < lease->numOutputs; o++) {
        RROutputPtr randr_output = lease->outputs[o];
        xf86OutputPtr output = randr_output->devPrivate;
        xf86CrtcPtr crtc = output->crtc;

        if (crtc)
            if (xf86CrtcSoleOutput(crtc, output))
                crtc->enabled = FALSE;
    }
}

static int
xf86RandR16CreateLease(ScreenPtr screen, RRLeasePtr randr_lease, int *fd)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);

    if (config->funcs->create_lease)
        return config->funcs->create_lease(randr_lease, fd);
    else
        return BadMatch;
}


static void
xf86RandR16TerminateLease(ScreenPtr screen, RRLeasePtr randr_lease)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);

    if (config->funcs->terminate_lease)
        config->funcs->terminate_lease(randr_lease);
}

static Bool
xf86RandR12Init12(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    rrScrPrivPtr rp = rrGetScrPriv(pScreen);
    XF86RandRInfoPtr randrp = XF86RANDRINFO(pScreen);

    rp->rrGetInfo = xf86RandR12GetInfo12;
    rp->rrScreenSetSize = xf86RandR12ScreenSetSize;
    rp->rrCrtcSet = xf86RandR12CrtcSet;
    rp->rrCrtcSetGamma = xf86RandR12CrtcSetGamma;
    rp->rrOutputSetProperty = xf86RandR12OutputSetProperty;
    rp->rrOutputValidateMode = xf86RandR12OutputValidateMode;
#if RANDR_13_INTERFACE
    rp->rrOutputGetProperty = xf86RandR13OutputGetProperty;
    rp->rrGetPanning = xf86RandR13GetPanning;
    rp->rrSetPanning = xf86RandR13SetPanning;
#endif
    rp->rrModeDestroy = xf86RandR12ModeDestroy;
    rp->rrSetConfig = NULL;

    rp->rrProviderSetOutputSource = xf86RandR14ProviderSetOutputSource;
    rp->rrProviderSetOffloadSink = xf86RandR14ProviderSetOffloadSink;

    rp->rrProviderSetProperty = xf86RandR14ProviderSetProperty;
    rp->rrProviderGetProperty = xf86RandR14ProviderGetProperty;
    rp->rrCrtcSetScanoutPixmap = xf86CrtcSetScanoutPixmap;
    rp->rrProviderDestroy = xf86RandR14ProviderDestroy;

    rp->rrCreateLease = xf86RandR16CreateLease;
    rp->rrTerminateLease = xf86RandR16TerminateLease;

    pScrn->PointerMoved = xf86RandR12PointerMoved;
    pScrn->ChangeGamma = xf86RandR12ChangeGamma;

    randrp->orig_EnterVT = pScrn->EnterVT;
    pScrn->EnterVT = xf86RandR12EnterVT;

    randrp->panning = FALSE;
    randrp->orig_ConstrainCursorHarder = pScreen->ConstrainCursorHarder;
    pScreen->ConstrainCursorHarder = xf86RandR13ConstrainCursorHarder;

    if (!xf86RandR12CreateObjects12(pScreen))
        return FALSE;

    /*
     * Configure output modes
     */
    if (!xf86RandR12SetInfo12(pScreen))
        return FALSE;

    if (!xf86RandR12InitGamma(pScrn, 256))
        return FALSE;

    return TRUE;
}

#endif

Bool
xf86RandR12PreInit(ScrnInfoPtr pScrn)
{
    return TRUE;
}
