#define DEBUG_VERB 2
/*
 * Copyright © 2002 David Dawes
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
 * THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the author(s) shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * the author(s).
 *
 * Authors: David Dawes <dawes@xfree86.org>
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdio.h>
#include <string.h>

#include "xf86.h"
#include "vbe.h"
#include "vbeModes.h"

static int
GetDepthFlag(vbeInfoPtr pVbe, int id)
{
    VbeModeInfoBlock *mode;
    int bpp;

    if ((mode = VBEGetModeInfo(pVbe, id)) == NULL)
        return 0;

    if (VBE_MODE_USABLE(mode, 0)) {
        int depth;

        if (VBE_MODE_COLOR(mode)) {
            depth = mode->RedMaskSize + mode->GreenMaskSize +
                mode->BlueMaskSize;
        }
        else {
            depth = 1;
        }
        bpp = mode->BitsPerPixel;
        VBEFreeModeInfo(mode);
        mode = NULL;
        switch (depth) {
        case 1:
            return V_DEPTH_1;
        case 4:
            return V_DEPTH_4;
        case 8:
            return V_DEPTH_8;
        case 15:
            return V_DEPTH_15;
        case 16:
            return V_DEPTH_16;
        case 24:
            switch (bpp) {
            case 24:
                return V_DEPTH_24_24;
            case 32:
                return V_DEPTH_24_32;
            }
        }
    }
    if (mode)
        VBEFreeModeInfo(mode);
    return 0;
}

/*
 * Find supported mode depths.
 */
int
VBEFindSupportedDepths(vbeInfoPtr pVbe, VbeInfoBlock * vbe, int *flags24,
                       int modeTypes)
{
    int i = 0;
    int depths = 0;

    if (modeTypes & V_MODETYPE_VBE) {
        while (vbe->VideoModePtr[i] != 0xffff) {
            depths |= GetDepthFlag(pVbe, vbe->VideoModePtr[i++]);
        }
    }

    /*
     * XXX This possibly only works with VBE 3.0 and later.
     */
    if (modeTypes & V_MODETYPE_VGA) {
        for (i = 0; i < 0x7F; i++) {
            depths |= GetDepthFlag(pVbe, i);
        }
    }

    if (flags24) {
        if (depths & V_DEPTH_24_24)
            *flags24 |= Support24bppFb;
        if (depths & V_DEPTH_24_32)
            *flags24 |= Support32bppFb;
    }

    return depths;
}

static DisplayModePtr
CheckMode(ScrnInfoPtr pScrn, vbeInfoPtr pVbe, VbeInfoBlock * vbe, int id,
          int flags)
{
    CARD16 major;
    VbeModeInfoBlock *mode;
    DisplayModePtr pMode;
    VbeModeInfoData *data;
    Bool modeOK = FALSE;

    major = (unsigned) (vbe->VESAVersion >> 8);

    if ((mode = VBEGetModeInfo(pVbe, id)) == NULL)
        return NULL;

    /* Does the mode match the depth/bpp? */
    /* Some BIOS's set BitsPerPixel to 15 instead of 16 for 15/16 */
    if (VBE_MODE_USABLE(mode, flags) &&
        ((pScrn->bitsPerPixel == 1 && !VBE_MODE_COLOR(mode)) ||
         (mode->BitsPerPixel > 8 &&
          (mode->RedMaskSize + mode->GreenMaskSize +
           mode->BlueMaskSize) == pScrn->depth &&
          mode->BitsPerPixel == pScrn->bitsPerPixel) ||
         (mode->BitsPerPixel == 15 && pScrn->depth == 15) ||
         (mode->BitsPerPixel <= 8 &&
          mode->BitsPerPixel == pScrn->bitsPerPixel))) {
        modeOK = TRUE;
        xf86ErrorFVerb(DEBUG_VERB, "*");
    }

    xf86ErrorFVerb(DEBUG_VERB,
                   "Mode: %x (%dx%d)\n", id, mode->XResolution,
                   mode->YResolution);
    xf86ErrorFVerb(DEBUG_VERB, "	ModeAttributes: 0x%x\n",
                   mode->ModeAttributes);
    xf86ErrorFVerb(DEBUG_VERB, "	WinAAttributes: 0x%x\n",
                   mode->WinAAttributes);
    xf86ErrorFVerb(DEBUG_VERB, "	WinBAttributes: 0x%x\n",
                   mode->WinBAttributes);
    xf86ErrorFVerb(DEBUG_VERB, "	WinGranularity: %d\n",
                   mode->WinGranularity);
    xf86ErrorFVerb(DEBUG_VERB, "	WinSize: %d\n", mode->WinSize);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	WinASegment: 0x%x\n", mode->WinASegment);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	WinBSegment: 0x%x\n", mode->WinBSegment);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	WinFuncPtr: 0x%lx\n", (unsigned long) mode->WinFuncPtr);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	BytesPerScanline: %d\n", mode->BytesPerScanline);
    xf86ErrorFVerb(DEBUG_VERB, "	XResolution: %d\n", mode->XResolution);
    xf86ErrorFVerb(DEBUG_VERB, "	YResolution: %d\n", mode->YResolution);
    xf86ErrorFVerb(DEBUG_VERB, "	XCharSize: %d\n", mode->XCharSize);
    xf86ErrorFVerb(DEBUG_VERB, "	YCharSize: %d\n", mode->YCharSize);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	NumberOfPlanes: %d\n", mode->NumberOfPlanes);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	BitsPerPixel: %d\n", mode->BitsPerPixel);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	NumberOfBanks: %d\n", mode->NumberOfBanks);
    xf86ErrorFVerb(DEBUG_VERB, "	MemoryModel: %d\n", mode->MemoryModel);
    xf86ErrorFVerb(DEBUG_VERB, "	BankSize: %d\n", mode->BankSize);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	NumberOfImages: %d\n", mode->NumberOfImages);
    xf86ErrorFVerb(DEBUG_VERB, "	RedMaskSize: %d\n", mode->RedMaskSize);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	RedFieldPosition: %d\n", mode->RedFieldPosition);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	GreenMaskSize: %d\n", mode->GreenMaskSize);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	GreenFieldPosition: %d\n", mode->GreenFieldPosition);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	BlueMaskSize: %d\n", mode->BlueMaskSize);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	BlueFieldPosition: %d\n", mode->BlueFieldPosition);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	RsvdMaskSize: %d\n", mode->RsvdMaskSize);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	RsvdFieldPosition: %d\n", mode->RsvdFieldPosition);
    xf86ErrorFVerb(DEBUG_VERB,
                   "	DirectColorModeInfo: %d\n", mode->DirectColorModeInfo);
    if (major >= 2) {
        xf86ErrorFVerb(DEBUG_VERB,
                       "	PhysBasePtr: 0x%lx\n",
                       (unsigned long) mode->PhysBasePtr);
        if (major >= 3) {
            xf86ErrorFVerb(DEBUG_VERB,
                           "	LinBytesPerScanLine: %d\n",
                           mode->LinBytesPerScanLine);
            xf86ErrorFVerb(DEBUG_VERB, "	BnkNumberOfImagePages: %d\n",
                           mode->BnkNumberOfImagePages);
            xf86ErrorFVerb(DEBUG_VERB, "	LinNumberOfImagePages: %d\n",
                           mode->LinNumberOfImagePages);
            xf86ErrorFVerb(DEBUG_VERB, "	LinRedMaskSize: %d\n",
                           mode->LinRedMaskSize);
            xf86ErrorFVerb(DEBUG_VERB, "	LinRedFieldPosition: %d\n",
                           mode->LinRedFieldPosition);
            xf86ErrorFVerb(DEBUG_VERB, "	LinGreenMaskSize: %d\n",
                           mode->LinGreenMaskSize);
            xf86ErrorFVerb(DEBUG_VERB, "	LinGreenFieldPosition: %d\n",
                           mode->LinGreenFieldPosition);
            xf86ErrorFVerb(DEBUG_VERB, "	LinBlueMaskSize: %d\n",
                           mode->LinBlueMaskSize);
            xf86ErrorFVerb(DEBUG_VERB, "	LinBlueFieldPosition: %d\n",
                           mode->LinBlueFieldPosition);
            xf86ErrorFVerb(DEBUG_VERB, "	LinRsvdMaskSize: %d\n",
                           mode->LinRsvdMaskSize);
            xf86ErrorFVerb(DEBUG_VERB, "	LinRsvdFieldPosition: %d\n",
                           mode->LinRsvdFieldPosition);
            xf86ErrorFVerb(DEBUG_VERB, "	MaxPixelClock: %ld\n",
                           (unsigned long) mode->MaxPixelClock);
        }
    }

    if (!modeOK) {
        VBEFreeModeInfo(mode);
        return NULL;
    }
    pMode = xnfcalloc(sizeof(DisplayModeRec), 1);

    pMode->status = MODE_OK;
    pMode->type = M_T_BUILTIN;

    /* for adjust frame */
    pMode->HDisplay = mode->XResolution;
    pMode->VDisplay = mode->YResolution;

    data = xnfcalloc(sizeof(VbeModeInfoData), 1);
    data->mode = id;
    data->data = mode;
    pMode->PrivSize = sizeof(VbeModeInfoData);
    pMode->Private = (INT32 *) data;
    pMode->next = NULL;
    return pMode;
}

/*
 * Check the available BIOS modes, and extract those that match the
 * requirements into the modePool.  Note: modePool is a NULL-terminated
 * list.
 */

DisplayModePtr
VBEGetModePool(ScrnInfoPtr pScrn, vbeInfoPtr pVbe, VbeInfoBlock * vbe,
               int modeTypes)
{
    DisplayModePtr pMode, p = NULL, modePool = NULL;
    int i = 0;

    if (modeTypes & V_MODETYPE_VBE) {
        while (vbe->VideoModePtr[i] != 0xffff) {
            int id = vbe->VideoModePtr[i++];

            if ((pMode = CheckMode(pScrn, pVbe, vbe, id, modeTypes)) != NULL) {
                ModeStatus status = MODE_OK;

                /* Check the mode against a specified virtual size (if any) */
                if (pScrn->display->virtualX > 0 &&
                    pMode->HDisplay > pScrn->display->virtualX) {
                    status = MODE_VIRTUAL_X;
                }
                if (pScrn->display->virtualY > 0 &&
                    pMode->VDisplay > pScrn->display->virtualY) {
                    status = MODE_VIRTUAL_Y;
                }
                if (status != MODE_OK) {
                    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                               "Not using mode \"%dx%d\" (%s)\n",
                               pMode->HDisplay, pMode->VDisplay,
                               xf86ModeStatusToString(status));
                }
                else {
                    if (p == NULL) {
                        modePool = pMode;
                    }
                    else {
                        p->next = pMode;
                    }
                    pMode->prev = NULL;
                    p = pMode;
                }
            }
        }
    }
    if (modeTypes & V_MODETYPE_VGA) {
        for (i = 0; i < 0x7F; i++) {
            if ((pMode = CheckMode(pScrn, pVbe, vbe, i, modeTypes)) != NULL) {
                ModeStatus status = MODE_OK;

                /* Check the mode against a specified virtual size (if any) */
                if (pScrn->display->virtualX > 0 &&
                    pMode->HDisplay > pScrn->display->virtualX) {
                    status = MODE_VIRTUAL_X;
                }
                if (pScrn->display->virtualY > 0 &&
                    pMode->VDisplay > pScrn->display->virtualY) {
                    status = MODE_VIRTUAL_Y;
                }
                if (status != MODE_OK) {
                    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                               "Not using mode \"%dx%d\" (%s)\n",
                               pMode->HDisplay, pMode->VDisplay,
                               xf86ModeStatusToString(status));
                }
                else {
                    if (p == NULL) {
                        modePool = pMode;
                    }
                    else {
                        p->next = pMode;
                    }
                    pMode->prev = NULL;
                    p = pMode;
                }
            }
        }
    }
    return modePool;
}

void
VBESetModeNames(DisplayModePtr pMode)
{
    if (!pMode)
        return;

    do {
        if (!pMode->name) {
            /* Catch "bad" modes. */
            if (pMode->HDisplay > 10000 || pMode->HDisplay < 0 ||
                pMode->VDisplay > 10000 || pMode->VDisplay < 0) {
                pMode->name = strdup("BADMODE");
            }
            else {
                char *tmp;
                XNFasprintf(&tmp, "%dx%d",
                            pMode->HDisplay, pMode->VDisplay);
                pMode->name = tmp;
            }
        }
        pMode = pMode->next;
    } while (pMode);
}

/*
 * Go through the monitor modes and selecting the best set of
 * parameters for each BIOS mode.  Note: This is only supported in
 * VBE version 3.0 or later.
 */
void
VBESetModeParameters(ScrnInfoPtr pScrn, vbeInfoPtr pVbe)
{
    DisplayModePtr pMode;
    VbeModeInfoData *data;

    pMode = pScrn->modes;
    do {
        DisplayModePtr p, best = NULL;
        ModeStatus status;

        for (p = pScrn->monitor->Modes; p != NULL; p = p->next) {
            if ((p->HDisplay != pMode->HDisplay) ||
                (p->VDisplay != pMode->VDisplay) ||
                (p->Flags & (V_INTERLACE | V_DBLSCAN | V_CLKDIV2)))
                continue;
            /* XXX could support the various V_ flags */
            status = xf86CheckModeForMonitor(p, pScrn->monitor);
            if (status != MODE_OK)
                continue;
            if (!best || (p->Clock > best->Clock))
                best = p;
        }

        if (best) {
            int clock;

            data = (VbeModeInfoData *) pMode->Private;
            pMode->HSync = (float) best->Clock * 1000.0 / best->HTotal + 0.5;
            pMode->VRefresh = pMode->HSync / best->VTotal + 0.5;
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                       "Attempting to use %dHz refresh for mode \"%s\" (%x)\n",
                       (int) pMode->VRefresh, pMode->name, data->mode);
            data->block = calloc(sizeof(VbeCRTCInfoBlock), 1);
            data->block->HorizontalTotal = best->HTotal;
            data->block->HorizontalSyncStart = best->HSyncStart;
            data->block->HorizontalSyncEnd = best->HSyncEnd;
            data->block->VerticalTotal = best->VTotal;
            data->block->VerticalSyncStart = best->VSyncStart;
            data->block->VerticalSyncEnd = best->VSyncEnd;
            data->block->Flags = ((best->Flags & V_NHSYNC) ? CRTC_NHSYNC : 0) |
                ((best->Flags & V_NVSYNC) ? CRTC_NVSYNC : 0);
            data->block->PixelClock = best->Clock * 1000;
            /* XXX May not have this. */
            clock = VBEGetPixelClock(pVbe, data->mode, data->block->PixelClock);
            DebugF("Setting clock %.2fMHz, closest is %.2fMHz\n",
                   (double) data->block->PixelClock / 1000000.0,
                   (double) clock / 1000000.0);
            if (clock)
                data->block->PixelClock = clock;
            data->mode |= (1 << 11);
            data->block->RefreshRate = ((double) (data->block->PixelClock) /
                                        (double) (best->HTotal *
                                                  best->VTotal)) * 100;
        }
        pMode = pMode->next;
    } while (pMode != pScrn->modes);
}

/*
 * These wrappers are to allow (temporary) functionality divergences.
 */
int
VBEValidateModes(ScrnInfoPtr scrp, DisplayModePtr availModes,
                 const char **modeNames, ClockRangePtr clockRanges,
                 int *linePitches, int minPitch, int maxPitch, int pitchInc,
                 int minHeight, int maxHeight, int virtualX, int virtualY,
                 int apertureSize, LookupModeFlags strategy)
{
    return xf86ValidateModes(scrp, availModes, modeNames, clockRanges,
                             linePitches, minPitch, maxPitch, pitchInc,
                             minHeight, maxHeight, virtualX, virtualY,
                             apertureSize, strategy);
}

void
VBEPrintModes(ScrnInfoPtr scrp)
{
    xf86PrintModes(scrp);
}
