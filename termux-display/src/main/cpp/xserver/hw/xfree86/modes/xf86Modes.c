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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <libxcvt/libxcvt.h>
#include "xf86Modes.h"
#include "xf86Priv.h"

extern XF86ConfigPtr xf86configptr;

/**
 * Calculates the horizontal sync rate of a mode.
 */
double
xf86ModeHSync(const DisplayModeRec * mode)
{
    double hsync = 0.0;

    if (mode->HSync > 0.0)
        hsync = mode->HSync;
    else if (mode->HTotal > 0)
        hsync = (float) mode->Clock / (float) mode->HTotal;

    return hsync;
}

/**
 * Calculates the vertical refresh rate of a mode.
 */
double
xf86ModeVRefresh(const DisplayModeRec * mode)
{
    double refresh = 0.0;

    if (mode->VRefresh > 0.0)
        refresh = mode->VRefresh;
    else if (mode->HTotal > 0 && mode->VTotal > 0) {
        refresh = mode->Clock * 1000.0 / mode->HTotal / mode->VTotal;
        if (mode->Flags & V_INTERLACE)
            refresh *= 2.0;
        if (mode->Flags & V_DBLSCAN)
            refresh /= 2.0;
        if (mode->VScan > 1)
            refresh /= (float) (mode->VScan);
    }
    return refresh;
}

int
xf86ModeWidth(const DisplayModeRec * mode, Rotation rotation)
{
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_180:
        return mode->HDisplay;
    case RR_Rotate_90:
    case RR_Rotate_270:
        return mode->VDisplay;
    default:
        return 0;
    }
}

int
xf86ModeHeight(const DisplayModeRec * mode, Rotation rotation)
{
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_180:
        return mode->VDisplay;
    case RR_Rotate_90:
    case RR_Rotate_270:
        return mode->HDisplay;
    default:
        return 0;
    }
}

/** Calculates the memory bandwidth (in MiB/sec) of a mode. */
unsigned int
xf86ModeBandwidth(DisplayModePtr mode, int depth)
{
    float a_active, a_total, active_percent, pixels_per_second;
    int bytes_per_pixel = bits_to_bytes(depth);

    if (!mode->HTotal || !mode->VTotal || !mode->Clock)
        return 0;

    a_active = mode->HDisplay * mode->VDisplay;
    a_total = mode->HTotal * mode->VTotal;
    active_percent = a_active / a_total;
    pixels_per_second = active_percent * mode->Clock * 1000.0;

    return (unsigned int) (pixels_per_second * bytes_per_pixel / (1024 * 1024));
}

/** Sets a default mode name of <width>x<height> on a mode. */
void
xf86SetModeDefaultName(DisplayModePtr mode)
{
    Bool interlaced = ! !(mode->Flags & V_INTERLACE);
    char *tmp;

    free((void *) mode->name);

    XNFasprintf(&tmp, "%dx%d%s", mode->HDisplay, mode->VDisplay,
                interlaced ? "i" : "");
    mode->name = tmp;
}

/*
 * xf86SetModeCrtc
 *
 * Initialises the Crtc parameters for a mode.  The initialisation includes
 * adjustments for interlaced and double scan modes.
 */
void
xf86SetModeCrtc(DisplayModePtr p, int adjustFlags)
{
    if ((p == NULL) || ((p->type & M_T_CRTC_C) == M_T_BUILTIN))
        return;

    p->CrtcHDisplay = p->HDisplay;
    p->CrtcHSyncStart = p->HSyncStart;
    p->CrtcHSyncEnd = p->HSyncEnd;
    p->CrtcHTotal = p->HTotal;
    p->CrtcHSkew = p->HSkew;
    p->CrtcVDisplay = p->VDisplay;
    p->CrtcVSyncStart = p->VSyncStart;
    p->CrtcVSyncEnd = p->VSyncEnd;
    p->CrtcVTotal = p->VTotal;
    if (p->Flags & V_INTERLACE) {
        if (adjustFlags & INTERLACE_HALVE_V) {
            p->CrtcVDisplay /= 2;
            p->CrtcVSyncStart /= 2;
            p->CrtcVSyncEnd /= 2;
            p->CrtcVTotal /= 2;
        }
        /* Force interlaced modes to have an odd VTotal */
        /* maybe we should only do this when INTERLACE_HALVE_V is set? */
        p->CrtcVTotal |= 1;
    }

    if (p->Flags & V_DBLSCAN) {
        p->CrtcVDisplay *= 2;
        p->CrtcVSyncStart *= 2;
        p->CrtcVSyncEnd *= 2;
        p->CrtcVTotal *= 2;
    }
    if (p->VScan > 1) {
        p->CrtcVDisplay *= p->VScan;
        p->CrtcVSyncStart *= p->VScan;
        p->CrtcVSyncEnd *= p->VScan;
        p->CrtcVTotal *= p->VScan;
    }
    p->CrtcVBlankStart = min(p->CrtcVSyncStart, p->CrtcVDisplay);
    p->CrtcVBlankEnd = max(p->CrtcVSyncEnd, p->CrtcVTotal);
    p->CrtcHBlankStart = min(p->CrtcHSyncStart, p->CrtcHDisplay);
    p->CrtcHBlankEnd = max(p->CrtcHSyncEnd, p->CrtcHTotal);

    p->CrtcHAdjusted = FALSE;
    p->CrtcVAdjusted = FALSE;
}

/**
 * Fills in a copy of mode, removing all stale pointer references.
 * xf86ModesEqual will return true when comparing with original mode.
 */
void
xf86SaveModeContents(DisplayModePtr intern, const DisplayModeRec *mode)
{
    *intern = *mode;
    intern->prev = intern->next = NULL;
    intern->name = NULL;
    intern->PrivSize = 0;
    intern->PrivFlags = 0;
    intern->Private = NULL;
}

/**
 * Allocates and returns a copy of pMode, including pointers within pMode.
 */
DisplayModePtr
xf86DuplicateMode(const DisplayModeRec * pMode)
{
    DisplayModePtr pNew;

    pNew = xnfalloc(sizeof(DisplayModeRec));
    *pNew = *pMode;
    pNew->next = NULL;
    pNew->prev = NULL;

    if (pMode->name == NULL)
        xf86SetModeDefaultName(pNew);
    else
        pNew->name = xnfstrdup(pMode->name);

    return pNew;
}

/**
 * Duplicates every mode in the given list and returns a pointer to the first
 * mode.
 *
 * \param modeList doubly-linked mode list
 */
DisplayModePtr
xf86DuplicateModes(ScrnInfoPtr pScrn, DisplayModePtr modeList)
{
    DisplayModePtr first = NULL, last = NULL;
    DisplayModePtr mode;

    for (mode = modeList; mode != NULL; mode = mode->next) {
        DisplayModePtr new;

        new = xf86DuplicateMode(mode);

        /* Insert pNew into modeList */
        if (last) {
            last->next = new;
            new->prev = last;
        }
        else {
            first = new;
            new->prev = NULL;
        }
        new->next = NULL;
        last = new;
    }

    return first;
}

/**
 * Returns true if the given modes should program to the same timings.
 *
 * This doesn't use Crtc values, as it might be used on ModeRecs without the
 * Crtc values set.  So, it's assumed that the other numbers are enough.
 */
Bool
xf86ModesEqual(const DisplayModeRec * pMode1, const DisplayModeRec * pMode2)
{
    if (pMode1->Clock == pMode2->Clock &&
        pMode1->HDisplay == pMode2->HDisplay &&
        pMode1->HSyncStart == pMode2->HSyncStart &&
        pMode1->HSyncEnd == pMode2->HSyncEnd &&
        pMode1->HTotal == pMode2->HTotal &&
        pMode1->HSkew == pMode2->HSkew &&
        pMode1->VDisplay == pMode2->VDisplay &&
        pMode1->VSyncStart == pMode2->VSyncStart &&
        pMode1->VSyncEnd == pMode2->VSyncEnd &&
        pMode1->VTotal == pMode2->VTotal &&
        pMode1->VScan == pMode2->VScan && pMode1->Flags == pMode2->Flags) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

static void
add(char **p, const char *new)
{
    *p = xnfrealloc(*p, strlen(*p) + strlen(new) + 2);
    strcat(*p, " ");
    strcat(*p, new);
}

/**
 * Print out a modeline.
 *
 * The mode type bits are informational except for the capitalized U
 * and P bits which give sort order priority.  Letter map:
 *
 * USERPREF, U, user preferred is set from the xorg.conf Monitor
 * Option "PreferredMode" or from the Screen Display Modes statement.
 * This unique modeline is moved to the head of the list after sorting.
 *
 * DRIVER, e, is set by the video driver, EDID or flat panel native.
 *
 * USERDEF, z, a configured zoom mode Ctrl+Alt+Keypad-{Plus,Minus}.
 *
 * DEFAULT, d, a compiled-in default.
 *
 * PREFERRED, P, driver preferred is set by the video device driver,
 * e.g. the EDID detailed timing modeline.  This is a true sort
 * priority and multiple P modes form a sorted sublist at the list
 * head.
 *
 * BUILTIN, b, a hardware fixed CRTC mode.
 *
 * See modes/xf86Crtc.c: xf86ProbeOutputModes().
 */
void
xf86PrintModeline(int scrnIndex, DisplayModePtr mode)
{
    char tmp[256];
    char *flags = xnfcalloc(1, 1);

#define TBITS 6
    const char tchar[TBITS + 1] = "UezdPb";

    int tbit[TBITS] = {
        M_T_USERPREF, M_T_DRIVER, M_T_USERDEF,
        M_T_DEFAULT, M_T_PREFERRED, M_T_BUILTIN
    };
    char type[TBITS + 2];       /* +1 for leading space */

#undef TBITS
    int tlen = 0;

    if (mode->type) {
        int i;

        type[tlen++] = ' ';
        for (i = 0; tchar[i]; i++)
            if (mode->type & tbit[i])
                type[tlen++] = tchar[i];
    }
    type[tlen] = '\0';

    if (mode->HSkew) {
        snprintf(tmp, 256, "hskew %i", mode->HSkew);
        add(&flags, tmp);
    }
    if (mode->VScan) {
        snprintf(tmp, 256, "vscan %i", mode->VScan);
        add(&flags, tmp);
    }
    if (mode->Flags & V_INTERLACE)
        add(&flags, "interlace");
    if (mode->Flags & V_CSYNC)
        add(&flags, "composite");
    if (mode->Flags & V_DBLSCAN)
        add(&flags, "doublescan");
    if (mode->Flags & V_BCAST)
        add(&flags, "bcast");
    if (mode->Flags & V_PHSYNC)
        add(&flags, "+hsync");
    if (mode->Flags & V_NHSYNC)
        add(&flags, "-hsync");
    if (mode->Flags & V_PVSYNC)
        add(&flags, "+vsync");
    if (mode->Flags & V_NVSYNC)
        add(&flags, "-vsync");
    if (mode->Flags & V_PCSYNC)
        add(&flags, "+csync");
    if (mode->Flags & V_NCSYNC)
        add(&flags, "-csync");
#if 0
    if (mode->Flags & V_CLKDIV2)
        add(&flags, "vclk/2");
#endif
    xf86DrvMsg(scrnIndex, X_INFO,
               "Modeline \"%s\"x%.01f  %6.2f  %i %i %i %i  %i %i %i %i%s"
               " (%.01f kHz%s)\n",
               mode->name, mode->VRefresh, mode->Clock / 1000.,
               mode->HDisplay, mode->HSyncStart, mode->HSyncEnd, mode->HTotal,
               mode->VDisplay, mode->VSyncStart, mode->VSyncEnd, mode->VTotal,
               flags, xf86ModeHSync(mode), type);
    free(flags);
}

/**
 * Marks as bad any modes with unsupported flags.
 *
 * \param modeList doubly-linked list of modes.
 * \param flags flags supported by the driver.
 *
 * \bug only V_INTERLACE and V_DBLSCAN are supported.  Is that enough?
 */
void
xf86ValidateModesFlags(ScrnInfoPtr pScrn, DisplayModePtr modeList, int flags)
{
    DisplayModePtr mode;

    if (flags == (V_INTERLACE | V_DBLSCAN))
        return;

    for (mode = modeList; mode != NULL; mode = mode->next) {
        if (mode->Flags & V_INTERLACE && !(flags & V_INTERLACE))
            mode->status = MODE_NO_INTERLACE;
        if (mode->Flags & V_DBLSCAN && !(flags & V_DBLSCAN))
            mode->status = MODE_NO_DBLESCAN;
    }
}

/**
 * Marks as bad any modes extending beyond the given max X, Y, or pitch.
 *
 * \param modeList doubly-linked list of modes.
 */
void
xf86ValidateModesSize(ScrnInfoPtr pScrn, DisplayModePtr modeList,
                      int maxX, int maxY, int maxPitch)
{
    DisplayModePtr mode;

    if (maxPitch <= 0)
        maxPitch = MAXINT;
    if (maxX <= 0)
        maxX = MAXINT;
    if (maxY <= 0)
        maxY = MAXINT;

    for (mode = modeList; mode != NULL; mode = mode->next) {
        if ((xf86ModeWidth(mode, RR_Rotate_0) > maxPitch ||
             xf86ModeWidth(mode, RR_Rotate_0) > maxX ||
             xf86ModeHeight(mode, RR_Rotate_0) > maxY) &&
            (xf86ModeWidth(mode, RR_Rotate_90) > maxPitch ||
             xf86ModeWidth(mode, RR_Rotate_90) > maxX ||
             xf86ModeHeight(mode, RR_Rotate_90) > maxY)) {
            if (xf86ModeWidth(mode, RR_Rotate_0) > maxPitch ||
                xf86ModeWidth(mode, RR_Rotate_90) > maxPitch)
                mode->status = MODE_BAD_WIDTH;

            if (xf86ModeWidth(mode, RR_Rotate_0) > maxX ||
                xf86ModeWidth(mode, RR_Rotate_90) > maxX)
                mode->status = MODE_VIRTUAL_X;

            if (xf86ModeHeight(mode, RR_Rotate_0) > maxY ||
                xf86ModeHeight(mode, RR_Rotate_90) > maxY)
                mode->status = MODE_VIRTUAL_Y;
        }

        if (mode->next == modeList)
            break;
    }
}

/**
 * Marks as bad any modes that aren't supported by the given monitor's
 * hsync and vrefresh ranges.
 *
 * \param modeList doubly-linked list of modes.
 */
void
xf86ValidateModesSync(ScrnInfoPtr pScrn, DisplayModePtr modeList, MonPtr mon)
{
    DisplayModePtr mode;

    for (mode = modeList; mode != NULL; mode = mode->next) {
        Bool bad;
        int i;

        bad = TRUE;
        for (i = 0; i < mon->nHsync; i++) {
            if (xf86ModeHSync(mode) >= mon->hsync[i].lo * (1 - SYNC_TOLERANCE)
                && xf86ModeHSync(mode) <=
                mon->hsync[i].hi * (1 + SYNC_TOLERANCE)) {
                bad = FALSE;
            }
        }
        if (bad)
            mode->status = MODE_HSYNC;

        bad = TRUE;
        for (i = 0; i < mon->nVrefresh; i++) {
            if (xf86ModeVRefresh(mode) >=
                mon->vrefresh[i].lo * (1 - SYNC_TOLERANCE) &&
                xf86ModeVRefresh(mode) <=
                mon->vrefresh[i].hi * (1 + SYNC_TOLERANCE)) {
                bad = FALSE;
            }
        }
        if (bad)
            mode->status = MODE_VSYNC;

        if (mode->next == modeList)
            break;
    }
}

/**
 * Marks as bad any modes extending beyond outside of the given clock ranges.
 *
 * \param modeList doubly-linked list of modes.
 * \param min pointer to minimums of clock ranges
 * \param max pointer to maximums of clock ranges
 * \param n_ranges number of ranges.
 */
void
xf86ValidateModesClocks(ScrnInfoPtr pScrn, DisplayModePtr modeList,
                        int *min, int *max, int n_ranges)
{
    DisplayModePtr mode;
    int i;

    for (mode = modeList; mode != NULL; mode = mode->next) {
        Bool good = FALSE;

        for (i = 0; i < n_ranges; i++) {
            if (mode->Clock >= min[i] * (1 - SYNC_TOLERANCE) &&
                mode->Clock <= max[i] * (1 + SYNC_TOLERANCE)) {
                good = TRUE;
                break;
            }
        }
        if (!good)
            mode->status = MODE_CLOCK_RANGE;
    }
}

/**
 * If the user has specified a set of mode names to use, mark as bad any modes
 * not listed.
 *
 * The user mode names specified are prefixes to names of modes, so "1024x768"
 * will match modes named "1024x768", "1024x768x75", "1024x768-good", but
 * "1024x768x75" would only match "1024x768x75" from that list.
 *
 * MODE_BAD is used as the rejection flag, for lack of a better flag.
 *
 * \param modeList doubly-linked list of modes.
 */
void
xf86ValidateModesUserConfig(ScrnInfoPtr pScrn, DisplayModePtr modeList)
{
    DisplayModePtr mode;

    if (pScrn->display->modes[0] == NULL)
        return;

    for (mode = modeList; mode != NULL; mode = mode->next) {
        int i;
        Bool good = FALSE;

        for (i = 0; pScrn->display->modes[i] != NULL; i++) {
            if (strncmp(pScrn->display->modes[i], mode->name,
                        strlen(pScrn->display->modes[i])) == 0) {
                good = TRUE;
                break;
            }
        }
        if (!good)
            mode->status = MODE_BAD;
    }
}

/**
 * Marks as bad any modes exceeding the given bandwidth.
 *
 * \param modeList doubly-linked list of modes.
 * \param bandwidth bandwidth in MHz.
 * \param depth color depth.
 */
void
xf86ValidateModesBandwidth(ScrnInfoPtr pScrn, DisplayModePtr modeList,
                           unsigned int bandwidth, int depth)
{
    DisplayModePtr mode;

    for (mode = modeList; mode != NULL; mode = mode->next) {
        if (xf86ModeBandwidth(mode, depth) > bandwidth)
            mode->status = MODE_BANDWIDTH;
    }
}

Bool
xf86ModeIsReduced(const DisplayModeRec * mode)
{
    if ((((mode->HDisplay * 5 / 4) & ~0x07) > mode->HTotal) &&
        ((mode->HTotal - mode->HDisplay) == 160) &&
        ((mode->HSyncEnd - mode->HDisplay) == 80) &&
        ((mode->HSyncEnd - mode->HSyncStart) == 32) &&
        ((mode->VSyncStart - mode->VDisplay) == 3))
        return TRUE;
    return FALSE;
}

/**
 * Marks as bad any reduced-blanking modes.
 *
 * \param modeList doubly-linked list of modes.
 */
void
xf86ValidateModesReducedBlanking(ScrnInfoPtr pScrn, DisplayModePtr modeList)
{
    for (; modeList != NULL; modeList = modeList->next)
        if (xf86ModeIsReduced(modeList))
            modeList->status = MODE_NO_REDUCED;
}

/**
 * Frees any modes from the list with a status other than MODE_OK.
 *
 * \param modeList pointer to a doubly-linked or circular list of modes.
 * \param verbose determines whether the reason for mode invalidation is
 *	  printed.
 */
void
xf86PruneInvalidModes(ScrnInfoPtr pScrn, DisplayModePtr * modeList,
                      Bool verbose)
{
    DisplayModePtr mode;

    for (mode = *modeList; mode != NULL;) {
        DisplayModePtr next = mode->next, first = *modeList;

        if (mode->status != MODE_OK) {
            if (verbose) {
                const char *type = "";

                if (mode->type & M_T_BUILTIN)
                    type = "built-in ";
                else if (mode->type & M_T_DEFAULT)
                    type = "default ";
                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                           "Not using %smode \"%s\" (%s)\n", type, mode->name,
                           xf86ModeStatusToString(mode->status));
            }
            xf86DeleteMode(modeList, mode);
        }

        if (next == first)
            break;
        mode = next;
    }
}

/**
 * Adds the new mode into the mode list, and returns the new list
 *
 * \param modes doubly-linked mode list.
 */
DisplayModePtr
xf86ModesAdd(DisplayModePtr modes, DisplayModePtr new)
{
    if (modes == NULL)
        return new;

    if (new) {
        DisplayModePtr mode = modes;

        while (mode->next)
            mode = mode->next;

        mode->next = new;
        new->prev = mode;
    }

    return modes;
}

/**
 * Build a mode list from a list of config file modes
 */
static DisplayModePtr
xf86GetConfigModes(XF86ConfModeLinePtr conf_mode)
{
    DisplayModePtr head = NULL, prev = NULL, mode;

    for (; conf_mode; conf_mode = (XF86ConfModeLinePtr) conf_mode->list.next) {
        mode = calloc(1, sizeof(DisplayModeRec));
        if (!mode)
            continue;
        mode->name = xstrdup(conf_mode->ml_identifier);
        if (!mode->name) {
            free(mode);
            continue;
        }
        mode->type = 0;
        mode->Clock = conf_mode->ml_clock;
        mode->HDisplay = conf_mode->ml_hdisplay;
        mode->HSyncStart = conf_mode->ml_hsyncstart;
        mode->HSyncEnd = conf_mode->ml_hsyncend;
        mode->HTotal = conf_mode->ml_htotal;
        mode->VDisplay = conf_mode->ml_vdisplay;
        mode->VSyncStart = conf_mode->ml_vsyncstart;
        mode->VSyncEnd = conf_mode->ml_vsyncend;
        mode->VTotal = conf_mode->ml_vtotal;
        mode->Flags = conf_mode->ml_flags;
        mode->HSkew = conf_mode->ml_hskew;
        mode->VScan = conf_mode->ml_vscan;

        mode->prev = prev;
        mode->next = NULL;
        if (prev)
            prev->next = mode;
        else
            head = mode;
        prev = mode;
    }
    return head;
}

/**
 * Build a mode list from a monitor configuration
 */
DisplayModePtr
xf86GetMonitorModes(ScrnInfoPtr pScrn, XF86ConfMonitorPtr conf_monitor)
{
    DisplayModePtr modes = NULL;
    XF86ConfModesLinkPtr modes_link;

    if (!conf_monitor)
        return NULL;

    /*
     * first we collect the mode lines from the UseModes directive
     */
    for (modes_link = conf_monitor->mon_modes_sect_lst;
         modes_link; modes_link = modes_link->list.next) {
        /* If this modes link hasn't been resolved, go look it up now */
        if (!modes_link->ml_modes)
            modes_link->ml_modes = xf86findModes(modes_link->ml_modes_str,
                                                 xf86configptr->conf_modes_lst);
        if (modes_link->ml_modes)
            modes = xf86ModesAdd(modes,
                                 xf86GetConfigModes(modes_link->ml_modes->
                                                    mon_modeline_lst));
    }

    return xf86ModesAdd(modes,
                        xf86GetConfigModes(conf_monitor->mon_modeline_lst));
}

/**
 * Build a mode list containing all of the default modes
 */
DisplayModePtr
xf86GetDefaultModes(void)
{
    DisplayModePtr head = NULL, mode;
    int i;

    for (i = 0; i < xf86NumDefaultModes; i++) {
        const DisplayModeRec *defMode = &xf86DefaultModes[i];

        mode = xf86DuplicateMode(defMode);
        head = xf86ModesAdd(head, mode);
    }
    return head;
}

/*
 * Walk a mode list and prune out duplicates.  Will preserve the preferred
 * mode of an otherwise-duplicate pair.
 *
 * Probably best to call this on lists that are all of a single class
 * (driver, default, user, etc.), otherwise, which mode gets deleted is
 * not especially well defined.
 *
 * Returns the new list.
 */

DisplayModePtr
xf86PruneDuplicateModes(DisplayModePtr modes)
{
    DisplayModePtr m, n, o;

 top:
    for (m = modes; m; m = m->next) {
        for (n = m->next; n; n = o) {
            o = n->next;
            if (xf86ModesEqual(m, n)) {
                if (n->type & M_T_PREFERRED) {
                    xf86DeleteMode(&modes, m);
                    goto top;
                }
                else
                    xf86DeleteMode(&modes, n);
            }
        }
    }

    return modes;
}

/*
 * Generate a CVT standard mode from HDisplay, VDisplay and VRefresh.
 */
DisplayModePtr
xf86CVTMode(int HDisplay, int VDisplay, float VRefresh, Bool Reduced,
            Bool Interlaced)
{
    struct libxcvt_mode_info *libxcvt_mode_info;
    DisplayModeRec *Mode = xnfcalloc(1, sizeof(DisplayModeRec));

    libxcvt_mode_info =
        libxcvt_gen_mode_info(HDisplay, VDisplay, VRefresh, Reduced, Interlaced);

    Mode->VDisplay   = libxcvt_mode_info->vdisplay;
    Mode->HDisplay   = libxcvt_mode_info->hdisplay;
    Mode->Clock      = libxcvt_mode_info->dot_clock;
    Mode->HSyncStart = libxcvt_mode_info->hsync_start;
    Mode->HSyncEnd   = libxcvt_mode_info->hsync_end;
    Mode->HTotal     = libxcvt_mode_info->htotal;
    Mode->VSyncStart = libxcvt_mode_info->vsync_start;
    Mode->VSyncEnd   = libxcvt_mode_info->vsync_end;
    Mode->VTotal     = libxcvt_mode_info->vtotal;
    Mode->VRefresh   = libxcvt_mode_info->vrefresh;
    Mode->Flags      = libxcvt_mode_info->mode_flags;

    free(libxcvt_mode_info);

    return Mode;
}
