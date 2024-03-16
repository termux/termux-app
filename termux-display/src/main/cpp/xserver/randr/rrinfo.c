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

#ifdef RANDR_10_INTERFACE
static RRModePtr
RROldModeAdd(RROutputPtr output, RRScreenSizePtr size, int refresh)
{
    ScreenPtr pScreen = output->pScreen;

    rrScrPriv(pScreen);
    xRRModeInfo modeInfo;
    char name[100];
    RRModePtr mode;
    int i;
    RRModePtr *modes;

    memset(&modeInfo, '\0', sizeof(modeInfo));
    snprintf(name, sizeof(name), "%dx%d", size->width, size->height);

    modeInfo.width = size->width;
    modeInfo.height = size->height;
    modeInfo.hTotal = size->width;
    modeInfo.vTotal = size->height;
    modeInfo.dotClock = ((CARD32) size->width * (CARD32) size->height *
                         (CARD32) refresh);
    modeInfo.nameLength = strlen(name);
    mode = RRModeGet(&modeInfo, name);
    if (!mode)
        return NULL;
    for (i = 0; i < output->numModes; i++)
        if (output->modes[i] == mode) {
            RRModeDestroy(mode);
            return mode;
        }

    if (output->numModes)
        modes = reallocarray(output->modes,
                             output->numModes + 1, sizeof(RRModePtr));
    else
        modes = malloc(sizeof(RRModePtr));
    if (!modes) {
        RRModeDestroy(mode);
        FreeResource(mode->mode.id, 0);
        return NULL;
    }
    modes[output->numModes++] = mode;
    output->modes = modes;
    output->changed = TRUE;
    pScrPriv->changed = TRUE;
    pScrPriv->configChanged = TRUE;
    return mode;
}

static void
RRScanOldConfig(ScreenPtr pScreen, Rotation rotations)
{
    rrScrPriv(pScreen);
    RROutputPtr output;
    RRCrtcPtr crtc;
    RRModePtr mode, newMode = NULL;
    int i;
    CARD16 minWidth = MAXSHORT, minHeight = MAXSHORT;
    CARD16 maxWidth = 0, maxHeight = 0;
    CARD16 width, height;

    /*
     * First time through, create a crtc and output and hook
     * them together
     */
    if (pScrPriv->numOutputs == 0 && pScrPriv->numCrtcs == 0) {
        crtc = RRCrtcCreate(pScreen, NULL);
        if (!crtc)
            return;
        output = RROutputCreate(pScreen, "default", 7, NULL);
        if (!output)
            return;
        RROutputSetCrtcs(output, &crtc, 1);
        RROutputSetConnection(output, RR_Connected);
        RROutputSetSubpixelOrder(output, PictureGetSubpixelOrder(pScreen));
    }

    output = pScrPriv->outputs[0];
    if (!output)
        return;
    crtc = pScrPriv->crtcs[0];
    if (!crtc)
        return;

    /* check rotations */
    if (rotations != crtc->rotations) {
        crtc->rotations = rotations;
        crtc->changed = TRUE;
        pScrPriv->changed = TRUE;
    }

    /* regenerate mode list */
    for (i = 0; i < pScrPriv->nSizes; i++) {
        RRScreenSizePtr size = &pScrPriv->pSizes[i];
        int r;

        if (size->nRates) {
            for (r = 0; r < size->nRates; r++) {
                mode = RROldModeAdd(output, size, size->pRates[r].rate);
                if (i == pScrPriv->size &&
                    size->pRates[r].rate == pScrPriv->rate) {
                    newMode = mode;
                }
            }
            free(size->pRates);
        }
        else {
            mode = RROldModeAdd(output, size, 0);
            if (i == pScrPriv->size)
                newMode = mode;
        }
    }
    if (pScrPriv->nSizes)
        free(pScrPriv->pSizes);
    pScrPriv->pSizes = NULL;
    pScrPriv->nSizes = 0;

    /* find size bounds */
    for (i = 0; i < output->numModes + output->numUserModes; i++) {
        mode = (i < output->numModes ?
                          output->modes[i] :
                          output->userModes[i - output->numModes]);
        width = mode->mode.width;
        height = mode->mode.height;

        if (width < minWidth)
            minWidth = width;
        if (width > maxWidth)
            maxWidth = width;
        if (height < minHeight)
            minHeight = height;
        if (height > maxHeight)
            maxHeight = height;
    }

    RRScreenSetSizeRange(pScreen, minWidth, minHeight, maxWidth, maxHeight);

    /* notice current mode */
    if (newMode)
        RRCrtcNotify(crtc, newMode, 0, 0, pScrPriv->rotation, NULL, 1, &output);
}
#endif

/*
 * Poll the driver for changed information
 */
Bool
RRGetInfo(ScreenPtr pScreen, Bool force_query)
{
    rrScrPriv(pScreen);
    Rotation rotations;
    int i;

    /* Return immediately if we don't need to re-query and we already have the
     * information.
     */
    if (!force_query) {
        if (pScrPriv->numCrtcs != 0 || pScrPriv->numOutputs != 0)
            return TRUE;
    }

    for (i = 0; i < pScrPriv->numOutputs; i++)
        pScrPriv->outputs[i]->changed = FALSE;
    for (i = 0; i < pScrPriv->numCrtcs; i++)
        pScrPriv->crtcs[i]->changed = FALSE;

    rotations = 0;
    pScrPriv->changed = FALSE;
    pScrPriv->configChanged = FALSE;

    if (!(*pScrPriv->rrGetInfo) (pScreen, &rotations))
        return FALSE;

#if RANDR_10_INTERFACE
    if (pScrPriv->nSizes)
        RRScanOldConfig(pScreen, rotations);
#endif
    RRTellChanged(pScreen);
    return TRUE;
}

/*
 * Register the range of sizes for the screen
 */
void
RRScreenSetSizeRange(ScreenPtr pScreen,
                     CARD16 minWidth,
                     CARD16 minHeight, CARD16 maxWidth, CARD16 maxHeight)
{
    rrScrPriv(pScreen);

    if (!pScrPriv)
        return;
    if (pScrPriv->minWidth == minWidth && pScrPriv->minHeight == minHeight &&
        pScrPriv->maxWidth == maxWidth && pScrPriv->maxHeight == maxHeight) {
        return;
    }

    pScrPriv->minWidth = minWidth;
    pScrPriv->minHeight = minHeight;
    pScrPriv->maxWidth = maxWidth;
    pScrPriv->maxHeight = maxHeight;
    RRSetChanged(pScreen);
    pScrPriv->configChanged = TRUE;
}

#ifdef RANDR_10_INTERFACE
static Bool
RRScreenSizeMatches(RRScreenSizePtr a, RRScreenSizePtr b)
{
    if (a->width != b->width)
        return FALSE;
    if (a->height != b->height)
        return FALSE;
    if (a->mmWidth != b->mmWidth)
        return FALSE;
    if (a->mmHeight != b->mmHeight)
        return FALSE;
    return TRUE;
}

RRScreenSizePtr
RRRegisterSize(ScreenPtr pScreen,
               short width, short height, short mmWidth, short mmHeight)
{
    rrScrPriv(pScreen);
    int i;
    RRScreenSize tmp;
    RRScreenSizePtr pNew;

    if (!pScrPriv)
        return 0;

    tmp.id = 0;
    tmp.width = width;
    tmp.height = height;
    tmp.mmWidth = mmWidth;
    tmp.mmHeight = mmHeight;
    tmp.pRates = 0;
    tmp.nRates = 0;
    for (i = 0; i < pScrPriv->nSizes; i++)
        if (RRScreenSizeMatches(&tmp, &pScrPriv->pSizes[i]))
            return &pScrPriv->pSizes[i];
    pNew = reallocarray(pScrPriv->pSizes,
                        pScrPriv->nSizes + 1, sizeof(RRScreenSize));
    if (!pNew)
        return 0;
    pNew[pScrPriv->nSizes++] = tmp;
    pScrPriv->pSizes = pNew;
    return &pNew[pScrPriv->nSizes - 1];
}

Bool
RRRegisterRate(ScreenPtr pScreen, RRScreenSizePtr pSize, int rate)
{
    rrScrPriv(pScreen);
    int i;
    RRScreenRatePtr pNew, pRate;

    if (!pScrPriv)
        return FALSE;

    for (i = 0; i < pSize->nRates; i++)
        if (pSize->pRates[i].rate == rate)
            return TRUE;

    pNew = reallocarray(pSize->pRates, pSize->nRates + 1, sizeof(RRScreenRate));
    if (!pNew)
        return FALSE;
    pRate = &pNew[pSize->nRates++];
    pRate->rate = rate;
    pSize->pRates = pNew;
    return TRUE;
}

Rotation
RRGetRotation(ScreenPtr pScreen)
{
    RROutputPtr output = RRFirstOutput(pScreen);

    if (!output)
        return RR_Rotate_0;

    return output->crtc->rotation;
}

void
RRSetCurrentConfig(ScreenPtr pScreen,
                   Rotation rotation, int rate, RRScreenSizePtr pSize)
{
    rrScrPriv(pScreen);

    if (!pScrPriv)
        return;
    pScrPriv->size = pSize - pScrPriv->pSizes;
    pScrPriv->rotation = rotation;
    pScrPriv->rate = rate;
}
#endif
