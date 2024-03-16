/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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

/*
 * LCM() and scanLineWidth() are:
 *
 * Copyright 1997 through 2004 by Marc Aurele La France (TSI @ UQV), tsi@xfree86.org
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Copyright 1990,91,92,93 by Thomas Roell, Germany.
 * Copyright 1991,92,93    by SGCS (Snitily Graphics Consulting Services), USA.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this  permission notice appear
 * in supporting documentation, and that the name of Thomas Roell nor
 * SGCS be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * Thomas Roell nor SGCS makes no representations about the suitability
 * of this software for any purpose. It is provided "as is" without
 * express or implied warranty.
 *
 * THOMAS ROELL AND SGCS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THOMAS ROELL OR SGCS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Authors: Dirk Hohndel <hohndel@XFree86.Org>
 *          David Dawes <dawes@XFree86.Org>
 *          Marc La France <tsi@XFree86.Org>
 *          ... and others
 *
 * This file includes helper functions for mode related things.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "xf86Modes.h"
#include "xf86Crtc.h"
#include "os.h"
#include "servermd.h"
#include "globals.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "edid.h"

static void
printModeRejectMessage(int index, DisplayModePtr p, int status)
{
    const char *type;

    if (p->type & M_T_BUILTIN)
        type = "built-in ";
    else if (p->type & M_T_DEFAULT)
        type = "default ";
    else if (p->type & M_T_DRIVER)
        type = "driver ";
    else
        type = "";

    xf86DrvMsg(index, X_INFO, "Not using %smode \"%s\" (%s)\n", type, p->name,
               xf86ModeStatusToString(status));
}

/*
 * Find closest clock to given frequency (in kHz).  This assumes the
 * number of clocks is greater than zero.
 */
static int
xf86GetNearestClock(ScrnInfoPtr scrp, int freq, Bool allowDiv2,
                    int DivFactor, int MulFactor, int *divider)
{
    int nearestClock = 0, nearestDiv = 1;
    int minimumGap = abs(freq - scrp->clock[0]);
    int i, j, k, gap;

    if (allowDiv2)
        k = 2;
    else
        k = 1;

    /* Must set this here in case the best match is scrp->clock[0] */
    if (divider != NULL)
        *divider = 0;

    for (i = 0; i < scrp->numClocks; i++) {
        for (j = 1; j <= k; j++) {
            gap = abs((freq * j) - ((scrp->clock[i] * DivFactor) / MulFactor));
            if ((gap < minimumGap) || ((gap == minimumGap) && (j < nearestDiv))) {
                minimumGap = gap;
                nearestClock = i;
                nearestDiv = j;
                if (divider != NULL)
                    *divider = (j - 1) * V_CLKDIV2;
            }
        }
    }
    return nearestClock;
}

/*
 * xf86ModeStatusToString
 *
 * Convert a ModeStatus value to a printable message
 */

const char *
xf86ModeStatusToString(ModeStatus status)
{
    switch (status) {
    case MODE_OK:
        return "Mode OK";
    case MODE_HSYNC:
        return "hsync out of range";
    case MODE_VSYNC:
        return "vrefresh out of range";
    case MODE_H_ILLEGAL:
        return "illegal horizontal timings";
    case MODE_V_ILLEGAL:
        return "illegal vertical timings";
    case MODE_BAD_WIDTH:
        return "width requires unsupported line pitch";
    case MODE_NOMODE:
        return "no mode of this name";
    case MODE_NO_INTERLACE:
        return "interlace mode not supported";
    case MODE_NO_DBLESCAN:
        return "doublescan mode not supported";
    case MODE_NO_VSCAN:
        return "multiscan mode not supported";
    case MODE_MEM:
        return "insufficient memory for mode";
    case MODE_VIRTUAL_X:
        return "width too large for virtual size";
    case MODE_VIRTUAL_Y:
        return "height too large for virtual size";
    case MODE_MEM_VIRT:
        return "insufficient memory given virtual size";
    case MODE_NOCLOCK:
        return "no clock available for mode";
    case MODE_CLOCK_HIGH:
        return "mode clock too high";
    case MODE_CLOCK_LOW:
        return "mode clock too low";
    case MODE_CLOCK_RANGE:
        return "bad mode clock/interlace/doublescan";
    case MODE_BAD_HVALUE:
        return "horizontal timing out of range";
    case MODE_BAD_VVALUE:
        return "vertical timing out of range";
    case MODE_BAD_VSCAN:
        return "VScan value out of range";
    case MODE_HSYNC_NARROW:
        return "horizontal sync too narrow";
    case MODE_HSYNC_WIDE:
        return "horizontal sync too wide";
    case MODE_HBLANK_NARROW:
        return "horizontal blanking too narrow";
    case MODE_HBLANK_WIDE:
        return "horizontal blanking too wide";
    case MODE_VSYNC_NARROW:
        return "vertical sync too narrow";
    case MODE_VSYNC_WIDE:
        return "vertical sync too wide";
    case MODE_VBLANK_NARROW:
        return "vertical blanking too narrow";
    case MODE_VBLANK_WIDE:
        return "vertical blanking too wide";
    case MODE_PANEL:
        return "exceeds panel dimensions";
    case MODE_INTERLACE_WIDTH:
        return "width too large for interlaced mode";
    case MODE_ONE_WIDTH:
        return "all modes must have the same width";
    case MODE_ONE_HEIGHT:
        return "all modes must have the same height";
    case MODE_ONE_SIZE:
        return "all modes must have the same resolution";
    case MODE_NO_REDUCED:
        return "monitor doesn't support reduced blanking";
    case MODE_BANDWIDTH:
        return "mode requires too much memory bandwidth";
    case MODE_BAD:
        return "unknown reason";
    case MODE_ERROR:
        return "internal error";
    default:
        return "unknown";
    }
}

/*
 * xf86ShowClockRanges() -- Print the clock ranges allowed
 * and the clock values scaled by ClockMulFactor and ClockDivFactor
 */
void
xf86ShowClockRanges(ScrnInfoPtr scrp, ClockRangePtr clockRanges)
{
    ClockRangePtr cp;
    int MulFactor = 1;
    int DivFactor = 1;
    int i, j;
    int scaledClock;

    for (cp = clockRanges; cp != NULL; cp = cp->next) {
        DivFactor = max(1, cp->ClockDivFactor);
        MulFactor = max(1, cp->ClockMulFactor);
        if (scrp->progClock) {
            if (cp->minClock) {
                if (cp->maxClock) {
                    xf86DrvMsg(scrp->scrnIndex, X_INFO,
                               "Clock range: %6.2f to %6.2f MHz\n",
                               (double) cp->minClock / 1000.0,
                               (double) cp->maxClock / 1000.0);
                }
                else {
                    xf86DrvMsg(scrp->scrnIndex, X_INFO,
                               "Minimum clock: %6.2f MHz\n",
                               (double) cp->minClock / 1000.0);
                }
            }
            else {
                if (cp->maxClock) {
                    xf86DrvMsg(scrp->scrnIndex, X_INFO,
                               "Maximum clock: %6.2f MHz\n",
                               (double) cp->maxClock / 1000.0);
                }
            }
        }
        else if (DivFactor > 1 || MulFactor > 1) {
            j = 0;
            for (i = 0; i < scrp->numClocks; i++) {
                scaledClock = (scrp->clock[i] * DivFactor) / MulFactor;
                if (scaledClock >= cp->minClock && scaledClock <= cp->maxClock) {
                    if ((j % 8) == 0) {
                        if (j > 0)
                            xf86ErrorF("\n");
                        xf86DrvMsg(scrp->scrnIndex, X_INFO, "scaled clocks:");
                    }
                    xf86ErrorF(" %6.2f", (double) scaledClock / 1000.0);
                    j++;
                }
            }
            xf86ErrorF("\n");
        }
    }
}

static Bool
modeInClockRange(ClockRangePtr cp, DisplayModePtr p)
{
    return ((p->Clock >= cp->minClock) &&
            (p->Clock <= cp->maxClock) &&
            (cp->interlaceAllowed || !(p->Flags & V_INTERLACE)) &&
            (cp->doubleScanAllowed ||
             ((p->VScan <= 1) && !(p->Flags & V_DBLSCAN))));
}

/*
 * xf86FindClockRangeForMode()    [... like the name says ...]
 */
static ClockRangePtr
xf86FindClockRangeForMode(ClockRangePtr clockRanges, DisplayModePtr p)
{
    ClockRangePtr cp;

    for (cp = clockRanges;; cp = cp->next)
        if (!cp || modeInClockRange(cp, p))
            return cp;
}

/*
 * xf86HandleBuiltinMode() - handles built-in modes
 */
static ModeStatus
xf86HandleBuiltinMode(ScrnInfoPtr scrp,
                      DisplayModePtr p,
                      DisplayModePtr modep,
                      ClockRangePtr clockRanges, Bool allowDiv2)
{
    ClockRangePtr cp;
    int extraFlags = 0;
    int MulFactor = 1;
    int DivFactor = 1;
    int clockIndex;

    /* Reject previously rejected modes */
    if (p->status != MODE_OK)
        return p->status;

    /* Reject previously considered modes */
    if (p->prev)
        return MODE_NOMODE;

    if ((p->type & M_T_CLOCK_C) == M_T_CLOCK_C) {
        /* Check clock is in range */
        cp = xf86FindClockRangeForMode(clockRanges, p);
        if (cp == NULL) {
            modep->type = p->type;
            p->status = MODE_CLOCK_RANGE;
            return MODE_CLOCK_RANGE;
        }
        DivFactor = cp->ClockDivFactor;
        MulFactor = cp->ClockMulFactor;
        if (!scrp->progClock) {
            clockIndex = xf86GetNearestClock(scrp, p->Clock, allowDiv2,
                                             cp->ClockDivFactor,
                                             cp->ClockMulFactor, &extraFlags);
            modep->Clock = (scrp->clock[clockIndex] * DivFactor)
                / MulFactor;
            modep->ClockIndex = clockIndex;
            modep->SynthClock = scrp->clock[clockIndex];
            if (extraFlags & V_CLKDIV2) {
                modep->Clock /= 2;
                modep->SynthClock /= 2;
            }
        }
        else {
            modep->Clock = p->Clock;
            modep->ClockIndex = -1;
            modep->SynthClock = (modep->Clock * MulFactor)
                / DivFactor;
        }
        modep->PrivFlags = cp->PrivFlags;
    }
    else {
        if (!scrp->progClock) {
            modep->Clock = p->Clock;
            modep->ClockIndex = p->ClockIndex;
            modep->SynthClock = p->SynthClock;
        }
        else {
            modep->Clock = p->Clock;
            modep->ClockIndex = -1;
            modep->SynthClock = p->SynthClock;
        }
        modep->PrivFlags = p->PrivFlags;
    }
    modep->type = p->type;
    modep->HDisplay = p->HDisplay;
    modep->HSyncStart = p->HSyncStart;
    modep->HSyncEnd = p->HSyncEnd;
    modep->HTotal = p->HTotal;
    modep->HSkew = p->HSkew;
    modep->VDisplay = p->VDisplay;
    modep->VSyncStart = p->VSyncStart;
    modep->VSyncEnd = p->VSyncEnd;
    modep->VTotal = p->VTotal;
    modep->VScan = p->VScan;
    modep->Flags = p->Flags | extraFlags;
    modep->CrtcHDisplay = p->CrtcHDisplay;
    modep->CrtcHBlankStart = p->CrtcHBlankStart;
    modep->CrtcHSyncStart = p->CrtcHSyncStart;
    modep->CrtcHSyncEnd = p->CrtcHSyncEnd;
    modep->CrtcHBlankEnd = p->CrtcHBlankEnd;
    modep->CrtcHTotal = p->CrtcHTotal;
    modep->CrtcHSkew = p->CrtcHSkew;
    modep->CrtcVDisplay = p->CrtcVDisplay;
    modep->CrtcVBlankStart = p->CrtcVBlankStart;
    modep->CrtcVSyncStart = p->CrtcVSyncStart;
    modep->CrtcVSyncEnd = p->CrtcVSyncEnd;
    modep->CrtcVBlankEnd = p->CrtcVBlankEnd;
    modep->CrtcVTotal = p->CrtcVTotal;
    modep->CrtcHAdjusted = p->CrtcHAdjusted;
    modep->CrtcVAdjusted = p->CrtcVAdjusted;
    modep->HSync = p->HSync;
    modep->VRefresh = p->VRefresh;
    modep->Private = p->Private;
    modep->PrivSize = p->PrivSize;

    p->prev = modep;

    return MODE_OK;
}

/*
 * xf86LookupMode
 *
 * This function returns a mode from the given list which matches the
 * given name.  When multiple modes with the same name are available,
 * the method of picking the matching mode is determined by the
 * strategy selected.
 *
 * This function takes the following parameters:
 *    scrp         ScrnInfoPtr
 *    modep        pointer to the returned mode, which must have the name
 *                 field filled in.
 *    clockRanges  a list of clock ranges.   This is optional when all the
 *                 modes are built-in modes.
 *    strategy     how to decide which mode to use from multiple modes with
 *                 the same name
 *
 * In addition, the following fields from the ScrnInfoRec are used:
 *    modePool     the list of monitor modes compatible with the driver
 *    clocks       a list of discrete clocks
 *    numClocks    number of discrete clocks
 *    progClock    clock is programmable
 *
 * If a mode was found, its values are filled in to the area pointed to
 * by modep,  If a mode was not found the return value indicates the
 * reason.
 */

static ModeStatus
xf86LookupMode(ScrnInfoPtr scrp, DisplayModePtr modep,
               ClockRangePtr clockRanges, LookupModeFlags strategy)
{
    DisplayModePtr p, bestMode = NULL;
    ClockRangePtr cp;
    int i, k, gap, minimumGap = CLOCK_TOLERANCE + 1;
    double refresh, bestRefresh = 0.0;
    Bool found = FALSE;
    int extraFlags = 0;
    int clockIndex = -1;
    int MulFactor = 1;
    int DivFactor = 1;
    int ModePrivFlags = 0;
    ModeStatus status = MODE_NOMODE;
    Bool allowDiv2 = (strategy & LOOKUP_CLKDIV2) != 0;
    int n;

    const int types[] = {
        M_T_BUILTIN | M_T_PREFERRED,
        M_T_BUILTIN,
        M_T_USERDEF | M_T_PREFERRED,
        M_T_USERDEF,
        M_T_DRIVER | M_T_PREFERRED,
        M_T_DRIVER,
        0
    };
    const int ntypes = ARRAY_SIZE(types);

    strategy &= ~(LOOKUP_CLKDIV2 | LOOKUP_OPTIONAL_TOLERANCES);

    /* Some sanity checking */
    if (scrp == NULL || scrp->modePool == NULL ||
        (!scrp->progClock && scrp->numClocks == 0)) {
        ErrorF("xf86LookupMode: called with invalid scrnInfoRec\n");
        return MODE_ERROR;
    }
    if (modep == NULL || modep->name == NULL) {
        ErrorF("xf86LookupMode: called with invalid modep\n");
        return MODE_ERROR;
    }
    for (cp = clockRanges; cp != NULL; cp = cp->next) {
        /* DivFactor and MulFactor must be > 0 */
        cp->ClockDivFactor = max(1, cp->ClockDivFactor);
        cp->ClockMulFactor = max(1, cp->ClockMulFactor);
    }

    /* Scan the mode pool for matching names */
    for (n = 0; n < ntypes; n++) {
        int type = types[n];

        for (p = scrp->modePool; p != NULL; p = p->next) {

            /* scan through the modes in the sort order above */
            if ((p->type & type) != type)
                continue;

            if (strcmp(p->name, modep->name) == 0) {

                /* Skip over previously rejected modes */
                if (p->status != MODE_OK) {
                    if (!found)
                        status = p->status;
                    continue;
                }

                /* Skip over previously considered modes */
                if (p->prev)
                    continue;

                if (p->type & M_T_BUILTIN) {
                    return xf86HandleBuiltinMode(scrp, p, modep, clockRanges,
                                                 allowDiv2);
                }

                /* Check clock is in range */
                cp = xf86FindClockRangeForMode(clockRanges, p);
                if (cp == NULL) {
                    /*
                     * XXX Could do more here to provide a more detailed
                     * reason for not finding a mode.
                     */
                    p->status = MODE_CLOCK_RANGE;
                    if (!found)
                        status = MODE_CLOCK_RANGE;
                    continue;
                }

                /*
                 * If programmable clock and strategy is not
                 * LOOKUP_BEST_REFRESH, the required mode has been found,
                 * otherwise record the refresh and continue looking.
                 */
                if (scrp->progClock) {
                    found = TRUE;
                    if (strategy != LOOKUP_BEST_REFRESH) {
                        bestMode = p;
                        DivFactor = cp->ClockDivFactor;
                        MulFactor = cp->ClockMulFactor;
                        ModePrivFlags = cp->PrivFlags;
                        break;
                    }
                    refresh = xf86ModeVRefresh(p);
                    if (p->Flags & V_INTERLACE)
                        refresh /= INTERLACE_REFRESH_WEIGHT;
                    if (refresh > bestRefresh) {
                        bestMode = p;
                        DivFactor = cp->ClockDivFactor;
                        MulFactor = cp->ClockMulFactor;
                        ModePrivFlags = cp->PrivFlags;
                        bestRefresh = refresh;
                    }
                    continue;
                }

                /*
                 * Clock is in range, so if it is not a programmable clock, find
                 * a matching clock.
                 */

                i = xf86GetNearestClock(scrp, p->Clock, allowDiv2,
                                        cp->ClockDivFactor, cp->ClockMulFactor,
                                        &k);
                /*
                 * If the clock is too far from the requested clock, this
                 * mode is no good.
                 */
                if (k & V_CLKDIV2)
                    gap = abs((p->Clock * 2) -
                              ((scrp->clock[i] * cp->ClockDivFactor) /
                               cp->ClockMulFactor));
                else
                    gap = abs(p->Clock -
                              ((scrp->clock[i] * cp->ClockDivFactor) /
                               cp->ClockMulFactor));
                if (gap > minimumGap) {
                    p->status = MODE_NOCLOCK;
                    if (!found)
                        status = MODE_NOCLOCK;
                    continue;
                }
                found = TRUE;

                if (strategy == LOOKUP_BEST_REFRESH) {
                    refresh = xf86ModeVRefresh(p);
                    if (p->Flags & V_INTERLACE)
                        refresh /= INTERLACE_REFRESH_WEIGHT;
                    if (refresh > bestRefresh) {
                        bestMode = p;
                        DivFactor = cp->ClockDivFactor;
                        MulFactor = cp->ClockMulFactor;
                        ModePrivFlags = cp->PrivFlags;
                        extraFlags = k;
                        clockIndex = i;
                        bestRefresh = refresh;
                    }
                    continue;
                }
                if (strategy == LOOKUP_CLOSEST_CLOCK) {
                    if (gap < minimumGap) {
                        bestMode = p;
                        DivFactor = cp->ClockDivFactor;
                        MulFactor = cp->ClockMulFactor;
                        ModePrivFlags = cp->PrivFlags;
                        extraFlags = k;
                        clockIndex = i;
                        minimumGap = gap;
                    }
                    continue;
                }
                /*
                 * If strategy is neither LOOKUP_BEST_REFRESH or
                 * LOOKUP_CLOSEST_CLOCK the required mode has been found.
                 */
                bestMode = p;
                DivFactor = cp->ClockDivFactor;
                MulFactor = cp->ClockMulFactor;
                ModePrivFlags = cp->PrivFlags;
                extraFlags = k;
                clockIndex = i;
                break;
            }
        }
        if (found)
            break;
    }
    if (!found || bestMode == NULL)
        return status;

    /* Fill in the mode parameters */
    if (scrp->progClock) {
        modep->Clock = bestMode->Clock;
        modep->ClockIndex = -1;
        modep->SynthClock = (modep->Clock * MulFactor) / DivFactor;
    }
    else {
        modep->Clock = (scrp->clock[clockIndex] * DivFactor) / MulFactor;
        modep->ClockIndex = clockIndex;
        modep->SynthClock = scrp->clock[clockIndex];
        if (extraFlags & V_CLKDIV2) {
            modep->Clock /= 2;
            modep->SynthClock /= 2;
        }
    }
    modep->type = bestMode->type;
    modep->PrivFlags = ModePrivFlags;
    modep->HDisplay = bestMode->HDisplay;
    modep->HSyncStart = bestMode->HSyncStart;
    modep->HSyncEnd = bestMode->HSyncEnd;
    modep->HTotal = bestMode->HTotal;
    modep->HSkew = bestMode->HSkew;
    modep->VDisplay = bestMode->VDisplay;
    modep->VSyncStart = bestMode->VSyncStart;
    modep->VSyncEnd = bestMode->VSyncEnd;
    modep->VTotal = bestMode->VTotal;
    modep->VScan = bestMode->VScan;
    modep->Flags = bestMode->Flags | extraFlags;
    modep->CrtcHDisplay = bestMode->CrtcHDisplay;
    modep->CrtcHBlankStart = bestMode->CrtcHBlankStart;
    modep->CrtcHSyncStart = bestMode->CrtcHSyncStart;
    modep->CrtcHSyncEnd = bestMode->CrtcHSyncEnd;
    modep->CrtcHBlankEnd = bestMode->CrtcHBlankEnd;
    modep->CrtcHTotal = bestMode->CrtcHTotal;
    modep->CrtcHSkew = bestMode->CrtcHSkew;
    modep->CrtcVDisplay = bestMode->CrtcVDisplay;
    modep->CrtcVBlankStart = bestMode->CrtcVBlankStart;
    modep->CrtcVSyncStart = bestMode->CrtcVSyncStart;
    modep->CrtcVSyncEnd = bestMode->CrtcVSyncEnd;
    modep->CrtcVBlankEnd = bestMode->CrtcVBlankEnd;
    modep->CrtcVTotal = bestMode->CrtcVTotal;
    modep->CrtcHAdjusted = bestMode->CrtcHAdjusted;
    modep->CrtcVAdjusted = bestMode->CrtcVAdjusted;
    modep->HSync = bestMode->HSync;
    modep->VRefresh = bestMode->VRefresh;
    modep->Private = bestMode->Private;
    modep->PrivSize = bestMode->PrivSize;

    bestMode->prev = modep;

    return MODE_OK;
}

/*
 * xf86CheckModeForMonitor
 *
 * This function takes a mode and monitor description, and determines
 * if the mode is valid for the monitor.
 */
ModeStatus
xf86CheckModeForMonitor(DisplayModePtr mode, MonPtr monitor)
{
    int i;

    /* Sanity checks */
    if (mode == NULL || monitor == NULL) {
        ErrorF("xf86CheckModeForMonitor: called with invalid parameters\n");
        return MODE_ERROR;
    }

    DebugF("xf86CheckModeForMonitor(%p %s, %p %s)\n",
           mode, mode->name, monitor, monitor->id);

    /* Some basic mode validity checks */
    if (0 >= mode->HDisplay || mode->HDisplay > mode->HSyncStart ||
        mode->HSyncStart >= mode->HSyncEnd || mode->HSyncEnd >= mode->HTotal)
        return MODE_H_ILLEGAL;

    if (0 >= mode->VDisplay || mode->VDisplay > mode->VSyncStart ||
        mode->VSyncStart >= mode->VSyncEnd || mode->VSyncEnd >= mode->VTotal)
        return MODE_V_ILLEGAL;

    if (monitor->nHsync > 0) {
        /* Check hsync against the allowed ranges */
        float hsync = xf86ModeHSync(mode);

        for (i = 0; i < monitor->nHsync; i++)
            if ((hsync > monitor->hsync[i].lo * (1.0 - SYNC_TOLERANCE)) &&
                (hsync < monitor->hsync[i].hi * (1.0 + SYNC_TOLERANCE)))
                break;

        /* Now see whether we ran out of sync ranges without finding a match */
        if (i == monitor->nHsync)
            return MODE_HSYNC;
    }

    if (monitor->nVrefresh > 0) {
        /* Check vrefresh against the allowed ranges */
        float vrefrsh = xf86ModeVRefresh(mode);

        for (i = 0; i < monitor->nVrefresh; i++)
            if ((vrefrsh > monitor->vrefresh[i].lo * (1.0 - SYNC_TOLERANCE)) &&
                (vrefrsh < monitor->vrefresh[i].hi * (1.0 + SYNC_TOLERANCE)))
                break;

        /* Now see whether we ran out of refresh ranges without finding a match */
        if (i == monitor->nVrefresh)
            return MODE_VSYNC;
    }

    /* Force interlaced modes to have an odd VTotal */
    if (mode->Flags & V_INTERLACE)
        mode->CrtcVTotal = mode->VTotal |= 1;

    /*
     * This code stops cvt -r modes, and only cvt -r modes, from hitting 15y+
     * old CRTs which might, when there is a lot of solar flare activity and
     * when the celestial bodies are unfavourably aligned, implode trying to
     * sync to it. It's called "Protecting the user from doing anything stupid".
     * -- libv
     */

    if (xf86ModeIsReduced(mode)) {
        if (!monitor->reducedblanking && !(mode->type & M_T_DRIVER))
            return MODE_NO_REDUCED;
    }

    if ((monitor->maxPixClock) && (mode->Clock > monitor->maxPixClock))
        return MODE_CLOCK_HIGH;

    return MODE_OK;
}

/*
 * xf86CheckModeSize
 *
 * An internal routine to check if a mode fits in video memory.  This tries to
 * avoid overflows that would otherwise occur when video memory size is greater
 * than 256MB.
 */
static Bool
xf86CheckModeSize(ScrnInfoPtr scrp, int w, int x, int y)
{
    int bpp = scrp->fbFormat.bitsPerPixel, pad = scrp->fbFormat.scanlinePad;
    int lineWidth, lastWidth;

    if (scrp->depth == 4)
        pad *= 4;               /* 4 planes */

    /* Sanity check */
    if ((w < 0) || (x < 0) || (y <= 0))
        return FALSE;

    lineWidth = (((w * bpp) + pad - 1) / pad) * pad;
    lastWidth = x * bpp;

    /*
     * At this point, we need to compare
     *
     *  (lineWidth * (y - 1)) + lastWidth
     *
     * against
     *
     *  scrp->videoRam * (1024 * 8)
     *
     * These are bit quantities.  To avoid overflows, do the comparison in
     * terms of BITMAP_SCANLINE_PAD units.  This assumes BITMAP_SCANLINE_PAD
     * is a power of 2.  We currently use 32, which limits us to a video
     * memory size of 8GB.
     */

    lineWidth = (lineWidth + (BITMAP_SCANLINE_PAD - 1)) / BITMAP_SCANLINE_PAD;
    lastWidth = (lastWidth + (BITMAP_SCANLINE_PAD - 1)) / BITMAP_SCANLINE_PAD;

    if ((lineWidth * (y - 1) + lastWidth) >
        (scrp->videoRam * ((1024 * 8) / BITMAP_SCANLINE_PAD)))
        return FALSE;

    return TRUE;
}

/*
 * xf86InitialCheckModeForDriver
 *
 * This function checks if a mode satisfies a driver's initial requirements:
 *   -  mode size fits within the available pixel area (memory)
 *   -  width lies within the range of supported line pitches
 *   -  mode size fits within virtual size (if fixed)
 *   -  horizontal timings are in range
 *
 * This function takes the following parameters:
 *    scrp         ScrnInfoPtr
 *    mode         mode to check
 *    maxPitch     (optional) maximum line pitch
 *    virtualX     (optional) virtual width requested
 *    virtualY     (optional) virtual height requested
 *
 * In addition, the following fields from the ScrnInfoRec are used:
 *    monitor      pointer to structure for monitor section
 *    fbFormat     pixel format for the framebuffer
 *    videoRam     video memory size (in kB)
 */

static ModeStatus
xf86InitialCheckModeForDriver(ScrnInfoPtr scrp, DisplayModePtr mode,
                              ClockRangePtr clockRanges,
                              LookupModeFlags strategy,
                              int maxPitch, int virtualX, int virtualY)
{
    ClockRangePtr cp;
    ModeStatus status;
    Bool allowDiv2 = (strategy & LOOKUP_CLKDIV2) != 0;
    int i, needDiv2;

    /* Sanity checks */
    if (!scrp || !mode || !clockRanges) {
        ErrorF("xf86InitialCheckModeForDriver: "
               "called with invalid parameters\n");
        return MODE_ERROR;
    }

    DebugF("xf86InitialCheckModeForDriver(%p, %p %s, %p, 0x%x, %d, %d, %d)\n",
           scrp, mode, mode->name, clockRanges, strategy, maxPitch, virtualX,
           virtualY);

    /* Some basic mode validity checks */
    if (0 >= mode->HDisplay || mode->HDisplay > mode->HSyncStart ||
        mode->HSyncStart >= mode->HSyncEnd || mode->HSyncEnd >= mode->HTotal)
        return MODE_H_ILLEGAL;

    if (0 >= mode->VDisplay || mode->VDisplay > mode->VSyncStart ||
        mode->VSyncStart >= mode->VSyncEnd || mode->VSyncEnd >= mode->VTotal)
        return MODE_V_ILLEGAL;

    if (!xf86CheckModeSize(scrp, mode->HDisplay, mode->HDisplay,
                           mode->VDisplay))
        return MODE_MEM;

    if (maxPitch > 0 && mode->HDisplay > maxPitch)
        return MODE_BAD_WIDTH;

    if (virtualX > 0 && mode->HDisplay > virtualX)
        return MODE_VIRTUAL_X;

    if (virtualY > 0 && mode->VDisplay > virtualY)
        return MODE_VIRTUAL_Y;

    /*
     * The use of the DisplayModeRec's Crtc* and SynthClock elements below is
     * provisional, in that they are later reused by the driver at mode-set
     * time.  Here, they are temporarily enlisted to contain the mode timings
     * as seen by the CRT or panel (rather than the CRTC).  The driver's
     * ValidMode() is allowed to modify these so it can deal with such things
     * as mode stretching and/or centering.  The driver should >NOT< modify the
     * user-supplied values as these are reported back when mode validation is
     * said and done.
     */
    /*
     * NOTE: We (ab)use the mode->Crtc* values here to store timing
     * information for the calculation of Hsync and Vrefresh. Before
     * these values are calculated the driver is given the opportunity
     * to either set these HSync and VRefresh itself or modify the timing
     * values.
     * The difference to the final calculation is small but imortand:
     * here we pass the flag INTERLACE_HALVE_V regardless if the driver
     * sets it or not. This way our calculation of VRefresh has the same
     * effect as if we do if (flags & V_INTERLACE) refresh *= 2.0
     * This dual use of the mode->Crtc* values will certainly create
     * confusion and is bad software design. However since it's part of
     * the driver API it's hard to change.
     */

    if (scrp->ValidMode) {

        xf86SetModeCrtc(mode, INTERLACE_HALVE_V);

        cp = xf86FindClockRangeForMode(clockRanges, mode);
        if (!cp)
            return MODE_CLOCK_RANGE;

        if (cp->ClockMulFactor < 1)
            cp->ClockMulFactor = 1;
        if (cp->ClockDivFactor < 1)
            cp->ClockDivFactor = 1;

        /*
         * XXX  The effect of clock dividers and multipliers on the monitor's
         *      pixel clock needs to be verified.
         */
        if (scrp->progClock) {
            mode->SynthClock = mode->Clock;
        }
        else {
            i = xf86GetNearestClock(scrp, mode->Clock, allowDiv2,
                                    cp->ClockDivFactor, cp->ClockMulFactor,
                                    &needDiv2);
            mode->SynthClock = (scrp->clock[i] * cp->ClockDivFactor) /
                cp->ClockMulFactor;
            if (needDiv2 & V_CLKDIV2)
                mode->SynthClock /= 2;
        }

        status = (*scrp->ValidMode) (scrp, mode, FALSE,
                                     MODECHECK_INITIAL);
        if (status != MODE_OK)
            return status;

        if (mode->HSync <= 0.0)
            mode->HSync = (float) mode->SynthClock / (float) mode->CrtcHTotal;
        if (mode->VRefresh <= 0.0)
            mode->VRefresh = (mode->SynthClock * 1000.0)
                / (mode->CrtcHTotal * mode->CrtcVTotal);
    }

    mode->HSync = xf86ModeHSync(mode);
    mode->VRefresh = xf86ModeVRefresh(mode);

    /* Assume it is OK */
    return MODE_OK;
}

/*
 * xf86CheckModeForDriver
 *
 * This function is for checking modes while the server is running (for
 * use mainly by the VidMode extension).
 *
 * This function checks if a mode satisfies a driver's requirements:
 *   -  width lies within the line pitch
 *   -  mode size fits within virtual size
 *   -  horizontal/vertical timings are in range
 *
 * This function takes the following parameters:
 *    scrp         ScrnInfoPtr
 *    mode         mode to check
 *    flags        not (currently) used
 *
 * In addition, the following fields from the ScrnInfoRec are used:
 *    virtualX     virtual width
 *    virtualY     virtual height
 *    clockRanges  allowable clock ranges
 */

ModeStatus
xf86CheckModeForDriver(ScrnInfoPtr scrp, DisplayModePtr mode, int flags)
{
    ClockRangePtr cp;
    int i, k, gap, minimumGap = CLOCK_TOLERANCE + 1;
    int extraFlags = 0;
    int clockIndex = -1;
    int MulFactor = 1;
    int DivFactor = 1;
    int ModePrivFlags = 0;
    ModeStatus status = MODE_NOMODE;

    /* Some sanity checking */
    if (scrp == NULL || (!scrp->progClock && scrp->numClocks == 0)) {
        ErrorF("xf86CheckModeForDriver: called with invalid scrnInfoRec\n");
        return MODE_ERROR;
    }
    if (mode == NULL) {
        ErrorF("xf86CheckModeForDriver: called with invalid modep\n");
        return MODE_ERROR;
    }

    /* Check the mode size */
    if (mode->HDisplay > scrp->virtualX)
        return MODE_VIRTUAL_X;

    if (mode->VDisplay > scrp->virtualY)
        return MODE_VIRTUAL_Y;

    for (cp = scrp->clockRanges; cp != NULL; cp = cp->next) {
        /* DivFactor and MulFactor must be > 0 */
        cp->ClockDivFactor = max(1, cp->ClockDivFactor);
        cp->ClockMulFactor = max(1, cp->ClockMulFactor);
    }

    if (scrp->progClock) {
        /* Check clock is in range */
        for (cp = scrp->clockRanges; cp != NULL; cp = cp->next) {
            if (modeInClockRange(cp, mode))
                break;
        }
        if (cp == NULL) {
            return MODE_CLOCK_RANGE;
        }
        /*
         * If programmable clock the required mode has been found
         */
        DivFactor = cp->ClockDivFactor;
        MulFactor = cp->ClockMulFactor;
        ModePrivFlags = cp->PrivFlags;
    }
    else {
        status = MODE_CLOCK_RANGE;
        /* Check clock is in range */
        for (cp = scrp->clockRanges; cp != NULL; cp = cp->next) {
            if (modeInClockRange(cp, mode)) {
                /*
                 * Clock is in range, so if it is not a programmable clock,
                 * find a matching clock.
                 */

                i = xf86GetNearestClock(scrp, mode->Clock, 0,
                                        cp->ClockDivFactor, cp->ClockMulFactor,
                                        &k);
                /*
                 * If the clock is too far from the requested clock, this
                 * mode is no good.
                 */
                if (k & V_CLKDIV2)
                    gap = abs((mode->Clock * 2) -
                              ((scrp->clock[i] * cp->ClockDivFactor) /
                               cp->ClockMulFactor));
                else
                    gap = abs(mode->Clock -
                              ((scrp->clock[i] * cp->ClockDivFactor) /
                               cp->ClockMulFactor));
                if (gap > minimumGap) {
                    status = MODE_NOCLOCK;
                    continue;
                }

                DivFactor = cp->ClockDivFactor;
                MulFactor = cp->ClockMulFactor;
                ModePrivFlags = cp->PrivFlags;
                extraFlags = k;
                clockIndex = i;
                break;
            }
        }
        if (cp == NULL)
            return status;
    }

    /* Fill in the mode parameters */
    if (scrp->progClock) {
        mode->ClockIndex = -1;
        mode->SynthClock = (mode->Clock * MulFactor) / DivFactor;
    }
    else {
        mode->Clock = (scrp->clock[clockIndex] * DivFactor) / MulFactor;
        mode->ClockIndex = clockIndex;
        mode->SynthClock = scrp->clock[clockIndex];
        if (extraFlags & V_CLKDIV2) {
            mode->Clock /= 2;
            mode->SynthClock /= 2;
        }
    }
    mode->PrivFlags = ModePrivFlags;

    return MODE_OK;
}

static int
inferVirtualSize(ScrnInfoPtr scrp, DisplayModePtr modes, int *vx, int *vy)
{
    float aspect = 0.0;
    MonPtr mon = scrp->monitor;
    xf86MonPtr DDC;
    int x = 0, y = 0;
    DisplayModePtr mode;

    if (!mon)
        return 0;
    DDC = mon->DDC;

    if (DDC && DDC->ver.revision >= 4) {
        /* For 1.4, we might actually get native pixel format.  How novel. */
        if (PREFERRED_TIMING_MODE(DDC->features.msc)) {
            for (mode = modes; mode; mode = mode->next) {
                if (mode->type & (M_T_DRIVER | M_T_PREFERRED)) {
                    x = mode->HDisplay;
                    y = mode->VDisplay;
                    goto found;
                }
            }
        }
        /*
         * Even if we don't, we might get aspect ratio from extra CVT info
         * or from the monitor size fields.  TODO.
         */
    }

    /*
     * Technically this triggers if either is zero.  That wasn't legal
     * before EDID 1.4, but right now we'll get that wrong. TODO.
     */
    if (!aspect) {
        if (!mon->widthmm || !mon->heightmm)
            aspect = 4.0 / 3.0;
        else
            aspect = (float) mon->widthmm / (float) mon->heightmm;
    }

    /* find the largest M_T_DRIVER mode with that aspect ratio */
    for (mode = modes; mode; mode = mode->next) {
        float mode_aspect, metaspect;

        if (!(mode->type & (M_T_DRIVER | M_T_USERDEF)))
            continue;
        mode_aspect = (float) mode->HDisplay / (float) mode->VDisplay;
        metaspect = aspect / mode_aspect;
        /* 5% slop or so, since we only get size in centimeters */
        if (fabs(1.0 - metaspect) < 0.05) {
            if ((mode->HDisplay > x) && (mode->VDisplay > y)) {
                x = mode->HDisplay;
                y = mode->VDisplay;
            }
        }
    }

    if (!x || !y) {
        xf86DrvMsg(scrp->scrnIndex, X_WARNING,
                   "Unable to estimate virtual size\n");
        return 0;
    }

 found:
    *vx = x;
    *vy = y;

    xf86DrvMsg(scrp->scrnIndex, X_INFO,
               "Estimated virtual size for aspect ratio %.4f is %dx%d\n",
               aspect, *vx, *vy);

    return 1;
}

/* Least common multiple */
static unsigned int
LCM(unsigned int x, unsigned int y)
{
    unsigned int m = x, n = y, o;

    while ((o = m % n)) {
        m = n;
        n = o;
    }

    return (x / n) * y;
}

/*
 * Given various screen attributes, determine the minimum scanline width such
 * that each scanline is server and DDX padded and any pixels with embedded
 * bank boundaries are off-screen.  This function returns -1 if such a width
 * cannot exist.
 */
static int
scanLineWidth(unsigned int xsize,       /* pixels */
              unsigned int ysize,       /* pixels */
              unsigned int width,       /* pixels */
              unsigned long BankSize,   /* char's */
              PixmapFormatRec * pBankFormat, unsigned int nWidthUnit    /* bits */
    )
{
    unsigned long nBitsPerBank, nBitsPerScanline, nBitsPerScanlinePadUnit;
    unsigned long minBitsPerScanline, maxBitsPerScanline;

    /* Sanity checks */

    if (!nWidthUnit || !pBankFormat)
        return -1;

    nBitsPerBank = BankSize * 8;
    if (nBitsPerBank % pBankFormat->scanlinePad)
        return -1;

    if (xsize > width)
        width = xsize;
    nBitsPerScanlinePadUnit = LCM(pBankFormat->scanlinePad, nWidthUnit);
    nBitsPerScanline =
        (((width * pBankFormat->bitsPerPixel) + nBitsPerScanlinePadUnit - 1) /
         nBitsPerScanlinePadUnit) * nBitsPerScanlinePadUnit;
    width = nBitsPerScanline / pBankFormat->bitsPerPixel;

    if (!xsize || !(nBitsPerBank % pBankFormat->bitsPerPixel))
        return (int) width;

    /*
     * Scanlines will be server-pad aligned at this point.  They will also be
     * a multiple of nWidthUnit bits long.  Ensure that pixels with embedded
     * bank boundaries are off-screen.
     *
     * It seems reasonable to limit total frame buffer size to 1/16 of the
     * theoretical maximum address space size.  On a machine with 32-bit
     * addresses (to 8-bit quantities) this turns out to be 256MB.  Not only
     * does this provide a simple limiting condition for the loops below, but
     * it also prevents unsigned long wraparounds.
     */
    if (!ysize)
        return -1;

    minBitsPerScanline = xsize * pBankFormat->bitsPerPixel;
    if (minBitsPerScanline > nBitsPerBank)
        return -1;

    if (ysize == 1)
        return (int) width;

    maxBitsPerScanline =
        (((unsigned long) (-1) >> 1) - minBitsPerScanline) / (ysize - 1);
    while (nBitsPerScanline <= maxBitsPerScanline) {
        unsigned long BankBase, BankUnit;

        BankUnit = ((nBitsPerBank + nBitsPerScanline - 1) / nBitsPerBank) *
            nBitsPerBank;
        if (!(BankUnit % nBitsPerScanline))
            return (int) width;

        for (BankBase = BankUnit;; BankBase += nBitsPerBank) {
            unsigned long x, y;

            y = BankBase / nBitsPerScanline;
            if (y >= ysize)
                return (int) width;

            x = BankBase % nBitsPerScanline;
            if (!(x % pBankFormat->bitsPerPixel))
                continue;

            if (x < minBitsPerScanline) {
                /*
                 * Skip ahead certain widths by dividing the excess scanline
                 * amongst the y's.
                 */
                y *= nBitsPerScanlinePadUnit;
                nBitsPerScanline += ((x + y - 1) / y) * nBitsPerScanlinePadUnit;
                width = nBitsPerScanline / pBankFormat->bitsPerPixel;
                break;
            }

            if (BankBase != BankUnit)
                continue;

            if (!(nBitsPerScanline % x))
                return (int) width;

            BankBase = ((nBitsPerScanline - minBitsPerScanline) /
                        (nBitsPerScanline - x)) * BankUnit;
        }
    }

    return -1;
}

/*
 * xf86ValidateModes
 *
 * This function takes a set of mode names, modes and limiting conditions,
 * and selects a set of modes and parameters based on those conditions.
 *
 * This function takes the following parameters:
 *    scrp         ScrnInfoPtr
 *    availModes   the list of modes available for the monitor
 *    modeNames    (optional) list of mode names that the screen is requesting
 *    clockRanges  a list of clock ranges
 *    linePitches  (optional) a list of line pitches
 *    minPitch     (optional) minimum line pitch (in pixels)
 *    maxPitch     (optional) maximum line pitch (in pixels)
 *    pitchInc     (mandatory) pitch increment (in bits)
 *    minHeight    (optional) minimum virtual height (in pixels)
 *    maxHeight    (optional) maximum virtual height (in pixels)
 *    virtualX     (optional) virtual width requested (in pixels)
 *    virtualY     (optional) virtual height requested (in pixels)
 *    apertureSize size of video aperture (in bytes)
 *    strategy     how to decide which mode to use from multiple modes with
 *                 the same name
 *
 * In addition, the following fields from the ScrnInfoRec are used:
 *    clocks       a list of discrete clocks
 *    numClocks    number of discrete clocks
 *    progClock    clock is programmable
 *    monitor      pointer to structure for monitor section
 *    fbFormat     format of the framebuffer
 *    videoRam     video memory size
 *    xInc         horizontal timing increment (defaults to 8 pixels)
 *
 * The function fills in the following ScrnInfoRec fields:
 *    modePool     A subset of the modes available to the monitor which
 *		   are compatible with the driver.
 *    modes        one mode entry for each of the requested modes, with the
 *                 status field filled in to indicate if the mode has been
 *                 accepted or not.
 *    virtualX     the resulting virtual width
 *    virtualY     the resulting virtual height
 *    displayWidth the resulting line pitch
 *
 * The function's return value is the number of matching modes found, or -1
 * if an unrecoverable error was encountered.
 */

int
xf86ValidateModes(ScrnInfoPtr scrp, DisplayModePtr availModes,
                  const char **modeNames, ClockRangePtr clockRanges,
                  int *linePitches, int minPitch, int maxPitch, int pitchInc,
                  int minHeight, int maxHeight, int virtualX, int virtualY,
                  int apertureSize, LookupModeFlags strategy)
{
    DisplayModePtr p, q, r, new, last, *endp;
    int i, numModes = 0;
    ModeStatus status;
    int linePitch = -1, virtX = 0, virtY = 0;
    int newLinePitch, newVirtX, newVirtY;
    int modeSize;               /* in pixels */
    Bool validateAllDefaultModes = FALSE;
    Bool userModes = FALSE;
    int saveType;
    PixmapFormatRec *BankFormat;
    ClockRangePtr cp;
    int numTimings = 0;
    range hsync[MAX_HSYNC];
    range vrefresh[MAX_VREFRESH];
    Bool inferred_virtual = FALSE;

    DebugF
        ("xf86ValidateModes(%p, %p, %p, %p,\n\t\t  %p, %d, %d, %d, %d, %d, %d, %d, %d, 0x%x)\n",
         scrp, availModes, modeNames, clockRanges, linePitches, minPitch,
         maxPitch, pitchInc, minHeight, maxHeight, virtualX, virtualY,
         apertureSize, strategy);

    /* Some sanity checking */
    if (scrp == NULL || scrp->name == NULL || !scrp->monitor ||
        (!scrp->progClock && scrp->numClocks == 0)) {
        ErrorF("xf86ValidateModes: called with invalid scrnInfoRec\n");
        return -1;
    }
    if (linePitches != NULL && linePitches[0] <= 0) {
        ErrorF("xf86ValidateModes: called with invalid linePitches\n");
        return -1;
    }
    if (pitchInc <= 0) {
        ErrorF("xf86ValidateModes: called with invalid pitchInc\n");
        return -1;
    }
    if ((virtualX > 0) != (virtualY > 0)) {
        ErrorF("xf86ValidateModes: called with invalid virtual resolution\n");
        return -1;
    }

    /*
     * If requested by the driver, allow missing hsync and/or vrefresh ranges
     * in the monitor section.
     */
    if (strategy & LOOKUP_OPTIONAL_TOLERANCES) {
        strategy &= ~LOOKUP_OPTIONAL_TOLERANCES;
    }
    else {
        const char *type = "";
        Bool specified = FALSE;

        if (scrp->monitor->nHsync <= 0) {
            if (numTimings > 0) {
                scrp->monitor->nHsync = numTimings;
                for (i = 0; i < numTimings; i++) {
                    scrp->monitor->hsync[i].lo = hsync[i].lo;
                    scrp->monitor->hsync[i].hi = hsync[i].hi;
                }
            }
            else {
                scrp->monitor->hsync[0].lo = 31.5;
                scrp->monitor->hsync[0].hi = 48.0;
                scrp->monitor->nHsync = 1;
            }
            type = "default ";
        }
        else {
            specified = TRUE;
        }
        for (i = 0; i < scrp->monitor->nHsync; i++) {
            if (scrp->monitor->hsync[i].lo == scrp->monitor->hsync[i].hi)
                xf86DrvMsg(scrp->scrnIndex, X_INFO,
                           "%s: Using %shsync value of %.2f kHz\n",
                           scrp->monitor->id, type, scrp->monitor->hsync[i].lo);
            else
                xf86DrvMsg(scrp->scrnIndex, X_INFO,
                           "%s: Using %shsync range of %.2f-%.2f kHz\n",
                           scrp->monitor->id, type,
                           scrp->monitor->hsync[i].lo,
                           scrp->monitor->hsync[i].hi);
        }

        type = "";
        if (scrp->monitor->nVrefresh <= 0) {
            if (numTimings > 0) {
                scrp->monitor->nVrefresh = numTimings;
                for (i = 0; i < numTimings; i++) {
                    scrp->monitor->vrefresh[i].lo = vrefresh[i].lo;
                    scrp->monitor->vrefresh[i].hi = vrefresh[i].hi;
                }
            }
            else {
                scrp->monitor->vrefresh[0].lo = 50;
                scrp->monitor->vrefresh[0].hi = 70;
                scrp->monitor->nVrefresh = 1;
            }
            type = "default ";
        }
        else {
            specified = TRUE;
        }
        for (i = 0; i < scrp->monitor->nVrefresh; i++) {
            if (scrp->monitor->vrefresh[i].lo == scrp->monitor->vrefresh[i].hi)
                xf86DrvMsg(scrp->scrnIndex, X_INFO,
                           "%s: Using %svrefresh value of %.2f Hz\n",
                           scrp->monitor->id, type,
                           scrp->monitor->vrefresh[i].lo);
            else
                xf86DrvMsg(scrp->scrnIndex, X_INFO,
                           "%s: Using %svrefresh range of %.2f-%.2f Hz\n",
                           scrp->monitor->id, type,
                           scrp->monitor->vrefresh[i].lo,
                           scrp->monitor->vrefresh[i].hi);
        }

        type = "";
        if (!scrp->monitor->maxPixClock && !specified) {
            type = "default ";
            scrp->monitor->maxPixClock = 65000.0;
        }
        if (scrp->monitor->maxPixClock) {
            xf86DrvMsg(scrp->scrnIndex, X_INFO,
                       "%s: Using %smaximum pixel clock of %.2f MHz\n",
                       scrp->monitor->id, type,
                       (float) scrp->monitor->maxPixClock / 1000.0);
        }
    }

    /*
     * Store the clockRanges for later use by the VidMode extension.
     */
    nt_list_for_each_entry(cp, clockRanges, next) {
        ClockRangePtr newCR = xnfalloc(sizeof(ClockRange));
        memcpy(newCR, cp, sizeof(ClockRange));
        newCR->next = NULL;
        if (scrp->clockRanges == NULL)
            scrp->clockRanges = newCR;
        else
            nt_list_append(newCR, scrp->clockRanges, ClockRange, next);
    }

    /* Determine which pixmap format to pass to scanLineWidth() */
    if (scrp->depth > 4)
        BankFormat = &scrp->fbFormat;
    else
        BankFormat = xf86GetPixFormat(scrp, 1); /* >not< scrp->depth! */

    if (scrp->xInc <= 0)
        scrp->xInc = 8;         /* Suitable for VGA and others */

#define _VIRTUALX(x) ((((x) + scrp->xInc - 1) / scrp->xInc) * scrp->xInc)

    /*
     * Determine maxPitch if it wasn't given explicitly.  Note linePitches
     * always takes precedence if is non-NULL.  In that case the minPitch and
     * maxPitch values passed are ignored.
     */
    if (linePitches) {
        minPitch = maxPitch = linePitches[0];
        for (i = 1; linePitches[i] > 0; i++) {
            if (linePitches[i] > maxPitch)
                maxPitch = linePitches[i];
            if (linePitches[i] < minPitch)
                minPitch = linePitches[i];
        }
    }

    /*
     * Initialise virtX and virtY if the values are fixed.
     */
    if (virtualY > 0) {
        if (maxHeight > 0 && virtualY > maxHeight) {
            xf86DrvMsg(scrp->scrnIndex, X_ERROR,
                       "Virtual height (%d) is too large for the hardware "
                       "(max %d)\n", virtualY, maxHeight);
            return -1;
        }

        if (minHeight > 0 && virtualY < minHeight) {
            xf86DrvMsg(scrp->scrnIndex, X_ERROR,
                       "Virtual height (%d) is too small for the hardware "
                       "(min %d)\n", virtualY, minHeight);
            return -1;
        }

        virtualX = _VIRTUALX(virtualX);
        if (linePitches != NULL) {
            for (i = 0; linePitches[i] != 0; i++) {
                if ((linePitches[i] >= virtualX) &&
                    (linePitches[i] ==
                     scanLineWidth(virtualX, virtualY, linePitches[i],
                                   apertureSize, BankFormat, pitchInc))) {
                    linePitch = linePitches[i];
                    break;
                }
            }
        }
        else {
            linePitch = scanLineWidth(virtualX, virtualY, minPitch,
                                      apertureSize, BankFormat, pitchInc);
        }

        if ((linePitch < minPitch) || (linePitch > maxPitch)) {
            xf86DrvMsg(scrp->scrnIndex, X_ERROR,
                       "Virtual width (%d) is too large for the hardware "
                       "(max %d)\n", virtualX, maxPitch);
            return -1;
        }

        if (!xf86CheckModeSize(scrp, linePitch, virtualX, virtualY)) {
            xf86DrvMsg(scrp->scrnIndex, X_ERROR,
                       "Virtual size (%dx%d) (pitch %d) exceeds video memory\n",
                       virtualX, virtualY, linePitch);
            return -1;
        }

        virtX = virtualX;
        virtY = virtualY;
    }
    else if (!modeNames || !*modeNames) {
        /* No virtual size given in the config, try to infer */
        /* XXX this doesn't take m{in,ax}Pitch into account; oh well */
        inferred_virtual = inferVirtualSize(scrp, availModes, &virtX, &virtY);
        if (inferred_virtual)
            linePitch = scanLineWidth(virtX, virtY, minPitch, apertureSize,
                                      BankFormat, pitchInc);
    }

    /* Print clock ranges and scaled clocks */
    xf86ShowClockRanges(scrp, clockRanges);

    /*
     * If scrp->modePool hasn't been setup yet, set it up now.  This allows the
     * modes that the driver definitely can't use to be weeded out early.  Note
     * that a modePool mode's prev field is used to hold a pointer to the
     * member of the scrp->modes list for which a match was considered.
     */
    if (scrp->modePool == NULL) {
        q = NULL;
        for (p = availModes; p != NULL; p = p->next) {
            status = xf86InitialCheckModeForDriver(scrp, p, clockRanges,
                                                   strategy, maxPitch,
                                                   virtX, virtY);

            if (status == MODE_OK) {
                status = xf86CheckModeForMonitor(p, scrp->monitor);
            }

            if (status == MODE_OK) {
                new = xnfalloc(sizeof(DisplayModeRec));
                *new = *p;
                new->next = NULL;
                if (!q) {
                    scrp->modePool = new;
                }
                else {
                    q->next = new;
                }
                new->prev = NULL;
                q = new;
                q->name = xnfstrdup(p->name);
                q->status = MODE_OK;
            }
            else {
                printModeRejectMessage(scrp->scrnIndex, p, status);
            }
        }

        if (scrp->modePool == NULL) {
            xf86DrvMsg(scrp->scrnIndex, X_WARNING, "Mode pool is empty\n");
            return 0;
        }
    }
    else {
        for (p = scrp->modePool; p != NULL; p = p->next) {
            p->prev = NULL;
            p->status = MODE_OK;
        }
    }

    /*
     * Allocate one entry in scrp->modes for each named mode.
     */
    while (scrp->modes)
        xf86DeleteMode(&scrp->modes, scrp->modes);
    endp = &scrp->modes;
    last = NULL;
    if (modeNames != NULL) {
        for (i = 0; modeNames[i] != NULL; i++) {
            userModes = TRUE;
            new = xnfcalloc(1, sizeof(DisplayModeRec));
            new->prev = last;
            new->type = M_T_USERDEF;
            new->name = xnfstrdup(modeNames[i]);
            if (new->prev)
                new->prev->next = new;
            *endp = last = new;
            endp = &new->next;
        }
    }

    /* Lookup each mode */
#ifdef PANORAMIX
    if (noPanoramiXExtension)
        validateAllDefaultModes = TRUE;
#endif

    for (p = scrp->modes;; p = p->next) {
        Bool repeat;

        /*
         * If the supplied mode names don't produce a valid mode, scan through
         * unconsidered modePool members until one survives validation.  This
         * is done in decreasing order by mode pixel area.
         */

        if (p == NULL) {
            if ((numModes > 0) && !validateAllDefaultModes)
                break;

            validateAllDefaultModes = TRUE;
            r = NULL;
            modeSize = 0;
            for (q = scrp->modePool; q != NULL; q = q->next) {
                if ((q->prev == NULL) && (q->status == MODE_OK)) {
                    /*
                     * Deal with the case where this mode wasn't considered
                     * because of a builtin mode of the same name.
                     */
                    for (p = scrp->modes; p != NULL; p = p->next) {
                        if ((p->status != MODE_OK) && !strcmp(p->name, q->name))
                            break;
                    }

                    if (p != NULL)
                        q->prev = p;
                    else {
                        /*
                         * A quick check to not allow default modes with
                         * horizontal timing parameters that CRTs may have
                         * problems with.
                         */
                        if (!scrp->monitor->reducedblanking &&
                            (q->type & M_T_DEFAULT) &&
                            ((double) q->HTotal / (double) q->HDisplay) < 1.15)
                            continue;

                        if (modeSize < (q->HDisplay * q->VDisplay)) {
                            r = q;
                            modeSize = q->HDisplay * q->VDisplay;
                        }
                    }
                }
            }

            if (r == NULL)
                break;

            p = xnfcalloc(1, sizeof(DisplayModeRec));
            p->prev = last;
            p->name = xnfstrdup(r->name);
            if (!userModes)
                p->type = M_T_USERDEF;
            if (p->prev)
                p->prev->next = p;
            *endp = last = p;
            endp = &p->next;
        }

        repeat = FALSE;
 lookupNext:
        if (repeat && ((status = p->status) != MODE_OK))
            printModeRejectMessage(scrp->scrnIndex, p, status);
        saveType = p->type;
        status = xf86LookupMode(scrp, p, clockRanges, strategy);
        if (repeat && status == MODE_NOMODE)
            continue;
        if (status != MODE_OK)
            printModeRejectMessage(scrp->scrnIndex, p, status);
        if (status == MODE_ERROR) {
            ErrorF("xf86ValidateModes: "
                   "unexpected result from xf86LookupMode()\n");
            return -1;
        }
        if (status != MODE_OK) {
            if (p->status == MODE_OK)
                p->status = status;
            continue;
        }
        p->type |= saveType;
        repeat = TRUE;

        newLinePitch = linePitch;
        newVirtX = virtX;
        newVirtY = virtY;

        /*
         * Don't let non-user defined modes increase the virtual size
         */
        if (!(p->type & M_T_USERDEF) && (numModes > 0)) {
            if (p->HDisplay > virtX) {
                p->status = MODE_VIRTUAL_X;
                goto lookupNext;
            }
            if (p->VDisplay > virtY) {
                p->status = MODE_VIRTUAL_Y;
                goto lookupNext;
            }
        }
        /*
         * Adjust virtual width and height if the mode is too large for the
         * current values and if they are not fixed.
         */
        if (virtualX <= 0 && p->HDisplay > newVirtX)
            newVirtX = _VIRTUALX(p->HDisplay);
        if (virtualY <= 0 && p->VDisplay > newVirtY) {
            if (maxHeight > 0 && p->VDisplay > maxHeight) {
                p->status = MODE_VIRTUAL_Y;     /* ? */
                goto lookupNext;
            }
            newVirtY = p->VDisplay;
        }

        /*
         * If virtual resolution is to be increased, revalidate it.
         */
        if ((virtX != newVirtX) || (virtY != newVirtY)) {
            if (linePitches != NULL) {
                newLinePitch = -1;
                for (i = 0; linePitches[i] != 0; i++) {
                    if ((linePitches[i] >= newVirtX) &&
                        (linePitches[i] >= linePitch) &&
                        (linePitches[i] ==
                         scanLineWidth(newVirtX, newVirtY, linePitches[i],
                                       apertureSize, BankFormat, pitchInc))) {
                        newLinePitch = linePitches[i];
                        break;
                    }
                }
            }
            else {
                if (linePitch < minPitch)
                    linePitch = minPitch;
                newLinePitch = scanLineWidth(newVirtX, newVirtY, linePitch,
                                             apertureSize, BankFormat,
                                             pitchInc);
            }
            if ((newLinePitch < minPitch) || (newLinePitch > maxPitch)) {
                p->status = MODE_BAD_WIDTH;
                goto lookupNext;
            }

            /*
             * Check that the pixel area required by the new virtual height
             * and line pitch isn't too large.
             */
            if (!xf86CheckModeSize(scrp, newLinePitch, newVirtX, newVirtY)) {
                p->status = MODE_MEM_VIRT;
                goto lookupNext;
            }
        }

        if (scrp->ValidMode) {
            /*
             * Give the driver a final say, passing it the proposed virtual
             * geometry.
             */
            scrp->virtualX = newVirtX;
            scrp->virtualY = newVirtY;
            scrp->displayWidth = newLinePitch;
            p->status = (scrp->ValidMode) (scrp, p, FALSE,
                                           MODECHECK_FINAL);

            if (p->status != MODE_OK) {
                goto lookupNext;
            }
        }

        /* Mode has passed all the tests */
        virtX = newVirtX;
        virtY = newVirtY;
        linePitch = newLinePitch;
        p->status = MODE_OK;
        numModes++;
    }

    /*
     * If we estimated the virtual size above, we may have filtered away all
     * the modes that maximally match that size; scan again to find out and
     * fix up if so.
     */
    if (inferred_virtual) {
        int vx = 0, vy = 0;

        for (p = scrp->modes; p; p = p->next) {
            if (p->HDisplay > vx && p->VDisplay > vy) {
                vx = p->HDisplay;
                vy = p->VDisplay;
            }
        }
        if (vx < virtX || vy < virtY) {
            const int types[] = {
                M_T_BUILTIN | M_T_PREFERRED,
                M_T_BUILTIN,
                M_T_DRIVER | M_T_PREFERRED,
                M_T_DRIVER,
                0
            };
            const int ntypes = ARRAY_SIZE(types);
            int n;

            /*
             * We did not find the estimated virtual size. So now we want to
             * find the largest mode available, but we want to search in the
             * modes in the order of "types" listed above.
             */
            for (n = 0; n < ntypes; n++) {
                int type = types[n];

                vx = 0;
                vy = 0;
                for (p = scrp->modes; p; p = p->next) {
                    /* scan through the modes in the sort order above */
                    if ((p->type & type) != type)
                        continue;
                    if (p->HDisplay > vx && p->VDisplay > vy) {
                        vx = p->HDisplay;
                        vy = p->VDisplay;
                    }
                }
                if (vx && vy)
                    /* Found one */
                    break;
            }
            xf86DrvMsg(scrp->scrnIndex, X_WARNING,
                       "Shrinking virtual size estimate from %dx%d to %dx%d\n",
                       virtX, virtY, vx, vy);
            virtX = _VIRTUALX(vx);
            virtY = vy;
            for (p = scrp->modes; p; p = p->next) {
                if (numModes > 0) {
                    if (p->HDisplay > virtX)
                        p->status = MODE_VIRTUAL_X;
                    if (p->VDisplay > virtY)
                        p->status = MODE_VIRTUAL_Y;
                    if (p->status != MODE_OK) {
                        numModes--;
                        printModeRejectMessage(scrp->scrnIndex, p, p->status);
                    }
                }
            }
            if (linePitches != NULL) {
                for (i = 0; linePitches[i] != 0; i++) {
                    if ((linePitches[i] >= virtX) &&
                        (linePitches[i] ==
                         scanLineWidth(virtX, virtY, linePitches[i],
                                       apertureSize, BankFormat, pitchInc))) {
                        linePitch = linePitches[i];
                        break;
                    }
                }
            }
            else {
                linePitch = scanLineWidth(virtX, virtY, minPitch,
                                          apertureSize, BankFormat, pitchInc);
            }
        }
    }

    /* Update the ScrnInfoRec parameters */

    scrp->virtualX = virtX;
    scrp->virtualY = virtY;
    scrp->displayWidth = linePitch;

    if (numModes <= 0)
        return 0;

    /* Make the mode list into a circular list by joining up the ends */
    p = scrp->modes;
    while (p->next != NULL)
        p = p->next;
    /* p is now the last mode on the list */
    p->next = scrp->modes;
    scrp->modes->prev = p;

    if (minHeight > 0 && virtY < minHeight) {
        xf86DrvMsg(scrp->scrnIndex, X_ERROR,
                   "Virtual height (%d) is too small for the hardware "
                   "(min %d)\n", virtY, minHeight);
        return -1;
    }

    return numModes;
}

/*
 * xf86DeleteMode
 *
 * This function removes a mode from a list of modes.
 *
 * There are different types of mode lists:
 *
 *  - singly linked linear lists, ending in NULL
 *  - doubly linked linear lists, starting and ending in NULL
 *  - doubly linked circular lists
 *
 */

void
xf86DeleteMode(DisplayModePtr * modeList, DisplayModePtr mode)
{
    /* Catch the easy/insane cases */
    if (modeList == NULL || *modeList == NULL || mode == NULL)
        return;

    /* If the mode is at the start of the list, move the start of the list */
    if (*modeList == mode)
        *modeList = mode->next;

    /* If mode is the only one on the list, set the list to NULL */
    if ((mode == mode->prev) && (mode == mode->next)) {
        *modeList = NULL;
    }
    else {
        if ((mode->prev != NULL) && (mode->prev->next == mode))
            mode->prev->next = mode->next;
        if ((mode->next != NULL) && (mode->next->prev == mode))
            mode->next->prev = mode->prev;
    }

    free((void *) mode->name);
    free(mode);
}

/*
 * xf86PruneDriverModes
 *
 * Remove modes from the driver's mode list which have been marked as
 * invalid.
 */

void
xf86PruneDriverModes(ScrnInfoPtr scrp)
{
    DisplayModePtr first, p, n;

    p = scrp->modes;
    if (p == NULL)
        return;

    do {
        if (!(first = scrp->modes))
            return;
        n = p->next;
        if (p->status != MODE_OK) {
            xf86DeleteMode(&(scrp->modes), p);
        }
        p = n;
    } while (p != NULL && p != first);

    /* modePool is no longer needed, turf it */
    while (scrp->modePool) {
        /*
         * A modePool mode's prev field is used to hold a pointer to the
         * member of the scrp->modes list for which a match was considered.
         * Clear that pointer first, otherwise xf86DeleteMode might get
         * confused
         */
        scrp->modePool->prev = NULL;
        xf86DeleteMode(&scrp->modePool, scrp->modePool);
    }
}

/*
 * xf86SetCrtcForModes
 *
 * Goes through the screen's mode list, and initialises the Crtc
 * parameters for each mode.  The initialisation includes adjustments
 * for interlaced and double scan modes.
 */
void
xf86SetCrtcForModes(ScrnInfoPtr scrp, int adjustFlags)
{
    DisplayModePtr p;

    /*
     * Store adjustFlags for use with the VidMode extension. There is an
     * implicit assumption here that SetCrtcForModes is called once.
     */
    scrp->adjustFlags = adjustFlags;

    p = scrp->modes;
    if (p == NULL)
        return;

    do {
        xf86SetModeCrtc(p, adjustFlags);
        DebugF("%sMode %s: %d (%d) %d %d (%d) %d %d (%d) %d %d (%d) %d\n",
               (p->type & M_T_DEFAULT) ? "Default " : "",
               p->name, p->CrtcHDisplay, p->CrtcHBlankStart,
               p->CrtcHSyncStart, p->CrtcHSyncEnd, p->CrtcHBlankEnd,
               p->CrtcHTotal, p->CrtcVDisplay, p->CrtcVBlankStart,
               p->CrtcVSyncStart, p->CrtcVSyncEnd, p->CrtcVBlankEnd,
               p->CrtcVTotal);
        p = p->next;
    } while (p != NULL && p != scrp->modes);
}

void
xf86PrintModes(ScrnInfoPtr scrp)
{
    DisplayModePtr p;
    float hsync, refresh = 0;
    const char *desc, *desc2, *prefix, *uprefix;

    if (scrp == NULL)
        return;

    xf86DrvMsg(scrp->scrnIndex, X_INFO, "Virtual size is %dx%d (pitch %d)\n",
               scrp->virtualX, scrp->virtualY, scrp->displayWidth);

    p = scrp->modes;
    if (p == NULL)
        return;

    do {
        desc = desc2 = "";
        hsync = xf86ModeHSync(p);
        refresh = xf86ModeVRefresh(p);
        if (p->Flags & V_INTERLACE) {
            desc = " (I)";
        }
        if (p->Flags & V_DBLSCAN) {
            desc = " (D)";
        }
        if (p->VScan > 1) {
            desc2 = " (VScan)";
        }
        if (p->type & M_T_BUILTIN)
            prefix = "Built-in mode";
        else if (p->type & M_T_DEFAULT)
            prefix = "Default mode";
        else if (p->type & M_T_DRIVER)
            prefix = "Driver mode";
        else
            prefix = "Mode";
        if (p->type & M_T_USERDEF)
            uprefix = "*";
        else
            uprefix = " ";
        if (hsync == 0 || refresh == 0) {
            if (p->name)
                xf86DrvMsg(scrp->scrnIndex, X_CONFIG,
                           "%s%s \"%s\"\n", uprefix, prefix, p->name);
            else
                xf86DrvMsg(scrp->scrnIndex, X_PROBED,
                           "%s%s %dx%d (unnamed)\n",
                           uprefix, prefix, p->HDisplay, p->VDisplay);
        }
        else if (p->Clock == p->SynthClock) {
            xf86DrvMsg(scrp->scrnIndex, X_CONFIG,
                       "%s%s \"%s\": %.1f MHz, %.1f kHz, %.1f Hz%s%s\n",
                       uprefix, prefix, p->name, p->Clock / 1000.0,
                       hsync, refresh, desc, desc2);
        }
        else {
            xf86DrvMsg(scrp->scrnIndex, X_CONFIG,
                       "%s%s \"%s\": %.1f MHz (scaled from %.1f MHz), "
                       "%.1f kHz, %.1f Hz%s%s\n",
                       uprefix, prefix, p->name, p->Clock / 1000.0,
                       p->SynthClock / 1000.0, hsync, refresh, desc, desc2);
        }
        if (hsync != 0 && refresh != 0)
            xf86PrintModeline(scrp->scrnIndex, p);
        p = p->next;
    } while (p != NULL && p != scrp->modes);
}
