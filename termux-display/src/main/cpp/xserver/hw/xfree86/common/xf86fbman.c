
/*
 * Copyright (c) 1998-2001 by The XFree86 Project, Inc.
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

#include "misc.h"
#include "xf86.h"

#include <X11/X.h>
#include "scrnintstr.h"
#include "regionstr.h"
#include "xf86fbman.h"

/*
#define DEBUG
*/

static DevPrivateKeyRec xf86FBManagerKeyRec;
static DevPrivateKey xf86FBManagerKey;

Bool
xf86RegisterOffscreenManager(ScreenPtr pScreen, FBManagerFuncsPtr funcs)
{

    xf86FBManagerKey = &xf86FBManagerKeyRec;

    if (!dixRegisterPrivateKey(&xf86FBManagerKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    dixSetPrivate(&pScreen->devPrivates, xf86FBManagerKey, funcs);

    return TRUE;
}

Bool
xf86FBManagerRunning(ScreenPtr pScreen)
{
    if (xf86FBManagerKey == NULL)
        return FALSE;

    if (!dixLookupPrivate(&pScreen->devPrivates, xf86FBManagerKey))
        return FALSE;

    return TRUE;
}

Bool
xf86RegisterFreeBoxCallback(ScreenPtr pScreen,
                            FreeBoxCallbackProcPtr FreeBoxCallback,
                            void *devPriv)
{
    FBManagerFuncsPtr funcs;

    if (xf86FBManagerKey == NULL)
        return FALSE;
    if (!(funcs = (FBManagerFuncsPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                       xf86FBManagerKey)))
        return FALSE;

    return (*funcs->RegisterFreeBoxCallback) (pScreen, FreeBoxCallback,
                                              devPriv);
}

FBAreaPtr
xf86AllocateOffscreenArea(ScreenPtr pScreen,
                          int w, int h,
                          int gran,
                          MoveAreaCallbackProcPtr moveCB,
                          RemoveAreaCallbackProcPtr removeCB, void *privData)
{
    FBManagerFuncsPtr funcs;

    if (xf86FBManagerKey == NULL)
        return NULL;
    if (!(funcs = (FBManagerFuncsPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                       xf86FBManagerKey)))
        return NULL;

    return (*funcs->AllocateOffscreenArea) (pScreen, w, h, gran, moveCB,
                                            removeCB, privData);
}

FBLinearPtr
xf86AllocateOffscreenLinear(ScreenPtr pScreen,
                            int length,
                            int gran,
                            MoveLinearCallbackProcPtr moveCB,
                            RemoveLinearCallbackProcPtr removeCB,
                            void *privData)
{
    FBManagerFuncsPtr funcs;

    if (xf86FBManagerKey == NULL)
        return NULL;
    if (!(funcs = (FBManagerFuncsPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                       xf86FBManagerKey)))
        return NULL;

    return (*funcs->AllocateOffscreenLinear) (pScreen, length, gran, moveCB,
                                              removeCB, privData);
}

void
xf86FreeOffscreenArea(FBAreaPtr area)
{
    FBManagerFuncsPtr funcs;

    if (!area)
        return;

    if (xf86FBManagerKey == NULL)
        return;
    if (!
        (funcs =
         (FBManagerFuncsPtr) dixLookupPrivate(&area->pScreen->devPrivates,
                                              xf86FBManagerKey)))
        return;

    (*funcs->FreeOffscreenArea) (area);

    return;
}

void
xf86FreeOffscreenLinear(FBLinearPtr linear)
{
    FBManagerFuncsPtr funcs;

    if (!linear)
        return;

    if (xf86FBManagerKey == NULL)
        return;
    if (!
        (funcs =
         (FBManagerFuncsPtr) dixLookupPrivate(&linear->pScreen->devPrivates,
                                              xf86FBManagerKey)))
        return;

    (*funcs->FreeOffscreenLinear) (linear);

    return;
}

Bool
xf86ResizeOffscreenArea(FBAreaPtr resize, int w, int h)
{
    FBManagerFuncsPtr funcs;

    if (!resize)
        return FALSE;

    if (xf86FBManagerKey == NULL)
        return FALSE;
    if (!
        (funcs =
         (FBManagerFuncsPtr) dixLookupPrivate(&resize->pScreen->devPrivates,
                                              xf86FBManagerKey)))
        return FALSE;

    return (*funcs->ResizeOffscreenArea) (resize, w, h);
}

Bool
xf86ResizeOffscreenLinear(FBLinearPtr resize, int size)
{
    FBManagerFuncsPtr funcs;

    if (!resize)
        return FALSE;

    if (xf86FBManagerKey == NULL)
        return FALSE;
    if (!
        (funcs =
         (FBManagerFuncsPtr) dixLookupPrivate(&resize->pScreen->devPrivates,
                                              xf86FBManagerKey)))
        return FALSE;

    return (*funcs->ResizeOffscreenLinear) (resize, size);
}

Bool
xf86QueryLargestOffscreenArea(ScreenPtr pScreen,
                              int *w, int *h,
                              int gran, int preferences, int severity)
{
    FBManagerFuncsPtr funcs;

    *w = 0;
    *h = 0;

    if (xf86FBManagerKey == NULL)
        return FALSE;
    if (!(funcs = (FBManagerFuncsPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                       xf86FBManagerKey)))
        return FALSE;

    return (*funcs->QueryLargestOffscreenArea) (pScreen, w, h, gran,
                                                preferences, severity);
}

Bool
xf86QueryLargestOffscreenLinear(ScreenPtr pScreen,
                                int *size, int gran, int severity)
{
    FBManagerFuncsPtr funcs;

    *size = 0;

    if (xf86FBManagerKey == NULL)
        return FALSE;
    if (!(funcs = (FBManagerFuncsPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                       xf86FBManagerKey)))
        return FALSE;

    return (*funcs->QueryLargestOffscreenLinear) (pScreen, size, gran,
                                                  severity);
}

Bool
xf86PurgeUnlockedOffscreenAreas(ScreenPtr pScreen)
{
    FBManagerFuncsPtr funcs;

    if (xf86FBManagerKey == NULL)
        return FALSE;
    if (!(funcs = (FBManagerFuncsPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                       xf86FBManagerKey)))
        return FALSE;

    return (*funcs->PurgeOffscreenAreas) (pScreen);
}

/************************************************************\

   Below is a specific implementation of an offscreen manager.

\************************************************************/

static DevPrivateKeyRec xf86FBScreenKeyRec;

#define xf86FBScreenKey (&xf86FBScreenKeyRec)

typedef struct _FBLink {
    FBArea area;
    struct _FBLink *next;
} FBLink, *FBLinkPtr;

typedef struct _FBLinearLink {
    FBLinear linear;
    int free;                   /* need to add free here as FBLinear is publicly accessible */
    FBAreaPtr area;             /* only used if allocation came from XY area */
    struct _FBLinearLink *next;
} FBLinearLink, *FBLinearLinkPtr;

typedef struct {
    ScreenPtr pScreen;
    RegionPtr InitialBoxes;
    RegionPtr FreeBoxes;
    FBLinkPtr UsedAreas;
    int NumUsedAreas;
    FBLinearLinkPtr LinearAreas;
    CloseScreenProcPtr CloseScreen;
    int NumCallbacks;
    FreeBoxCallbackProcPtr *FreeBoxesUpdateCallback;
    DevUnion *devPrivates;
} FBManager, *FBManagerPtr;

static void
SendCallFreeBoxCallbacks(FBManagerPtr offman)
{
    int i = offman->NumCallbacks;

    while (i--) {
        (*offman->FreeBoxesUpdateCallback[i]) (offman->pScreen,
                                               offman->FreeBoxes,
                                               offman->devPrivates[i].ptr);
    }
}

static Bool
localRegisterFreeBoxCallback(ScreenPtr pScreen,
                             FreeBoxCallbackProcPtr FreeBoxCallback,
                             void *devPriv)
{
    FBManagerPtr offman;
    FreeBoxCallbackProcPtr *newCallbacks;
    DevUnion *newPrivates;

    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    newCallbacks = reallocarray(offman->FreeBoxesUpdateCallback,
                                offman->NumCallbacks + 1,
                                sizeof(FreeBoxCallbackProcPtr));
    if (!newCallbacks)
        return FALSE;
    else
        offman->FreeBoxesUpdateCallback = newCallbacks;

    newPrivates = reallocarray(offman->devPrivates,
                               offman->NumCallbacks + 1,
                               sizeof(DevUnion));
    if (!newPrivates)
        return FALSE;
    else
        offman->devPrivates = newPrivates;

    offman->FreeBoxesUpdateCallback[offman->NumCallbacks] = FreeBoxCallback;
    offman->devPrivates[offman->NumCallbacks].ptr = devPriv;
    offman->NumCallbacks++;

    SendCallFreeBoxCallbacks(offman);

    return TRUE;
}

static FBAreaPtr
AllocateArea(FBManagerPtr offman,
             int w, int h,
             int granularity,
             MoveAreaCallbackProcPtr moveCB,
             RemoveAreaCallbackProcPtr removeCB, void *privData)
{
    ScreenPtr pScreen = offman->pScreen;
    FBLinkPtr link = NULL;
    FBAreaPtr area = NULL;
    RegionRec NewReg;
    int i, x = 0, num;
    BoxPtr boxp;

    if (granularity <= 1)
        granularity = 0;

    boxp = RegionRects(offman->FreeBoxes);
    num = RegionNumRects(offman->FreeBoxes);

    /* look through the free boxes */
    for (i = 0; i < num; i++, boxp++) {
        x = boxp->x1;
        if (granularity > 1)
            x = ((x + granularity - 1) / granularity) * granularity;

        if (((boxp->y2 - boxp->y1) < h) || ((boxp->x2 - x) < w))
            continue;

        link = malloc(sizeof(FBLink));
        if (!link)
            return NULL;

        area = &(link->area);
        link->next = offman->UsedAreas;
        offman->UsedAreas = link;
        offman->NumUsedAreas++;
        break;
    }

    /* try to boot a removable one out if we are not expendable ourselves */
    if (!area && !removeCB) {
        link = offman->UsedAreas;

        while (link) {
            if (!link->area.RemoveAreaCallback) {
                link = link->next;
                continue;
            }

            boxp = &(link->area.box);
            x = boxp->x1;
            if (granularity > 1)
                x = ((x + granularity - 1) / granularity) * granularity;

            if (((boxp->y2 - boxp->y1) < h) || ((boxp->x2 - x) < w)) {
                link = link->next;
                continue;
            }

            /* bye, bye */
            (*link->area.RemoveAreaCallback) (&link->area);
            RegionInit(&NewReg, &(link->area.box), 1);
            RegionUnion(offman->FreeBoxes, offman->FreeBoxes, &NewReg);
            RegionUninit(&NewReg);

            area = &(link->area);
            break;
        }
    }

    if (area) {
        area->pScreen = pScreen;
        area->granularity = granularity;
        area->box.x1 = x;
        area->box.x2 = x + w;
        area->box.y1 = boxp->y1;
        area->box.y2 = boxp->y1 + h;
        area->MoveAreaCallback = moveCB;
        area->RemoveAreaCallback = removeCB;
        area->devPrivate.ptr = privData;

        RegionInit(&NewReg, &(area->box), 1);
        RegionSubtract(offman->FreeBoxes, offman->FreeBoxes, &NewReg);
        RegionUninit(&NewReg);
    }

    return area;
}

static FBAreaPtr
localAllocateOffscreenArea(ScreenPtr pScreen,
                           int w, int h,
                           int gran,
                           MoveAreaCallbackProcPtr moveCB,
                           RemoveAreaCallbackProcPtr removeCB, void *privData)
{
    FBManagerPtr offman;
    FBAreaPtr area = NULL;

    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    if ((area = AllocateArea(offman, w, h, gran, moveCB, removeCB, privData)))
        SendCallFreeBoxCallbacks(offman);

    return area;
}

static void
localFreeOffscreenArea(FBAreaPtr area)
{
    FBManagerPtr offman;
    FBLinkPtr pLink, pLinkPrev = NULL;
    RegionRec FreedRegion;
    ScreenPtr pScreen;

    pScreen = area->pScreen;
    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    pLink = offman->UsedAreas;
    if (!pLink)
        return;

    while (&(pLink->area) != area) {
        pLinkPrev = pLink;
        pLink = pLink->next;
        if (!pLink)
            return;
    }

    /* put the area back into the pool */
    RegionInit(&FreedRegion, &(pLink->area.box), 1);
    RegionUnion(offman->FreeBoxes, offman->FreeBoxes, &FreedRegion);
    RegionUninit(&FreedRegion);

    if (pLinkPrev)
        pLinkPrev->next = pLink->next;
    else
        offman->UsedAreas = pLink->next;

    free(pLink);
    offman->NumUsedAreas--;

    SendCallFreeBoxCallbacks(offman);
}

static Bool
localResizeOffscreenArea(FBAreaPtr resize, int w, int h)
{
    FBManagerPtr offman;
    ScreenPtr pScreen;
    BoxRec OrigArea;
    RegionRec FreedReg;
    FBAreaPtr area = NULL;
    FBLinkPtr pLink, newLink, pLinkPrev = NULL;

    pScreen = resize->pScreen;
    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    /* find this link */
    if (!(pLink = offman->UsedAreas))
        return FALSE;

    while (&(pLink->area) != resize) {
        pLinkPrev = pLink;
        pLink = pLink->next;
        if (!pLink)
            return FALSE;
    }

    OrigArea.x1 = resize->box.x1;
    OrigArea.x2 = resize->box.x2;
    OrigArea.y1 = resize->box.y1;
    OrigArea.y2 = resize->box.y2;

    /* if it's smaller, this is easy */

    if ((w <= (resize->box.x2 - resize->box.x1)) &&
        (h <= (resize->box.y2 - resize->box.y1))) {
        RegionRec NewReg;

        resize->box.x2 = resize->box.x1 + w;
        resize->box.y2 = resize->box.y1 + h;

        if ((resize->box.y2 == OrigArea.y2) && (resize->box.x2 == OrigArea.x2))
            return TRUE;

        RegionInit(&FreedReg, &OrigArea, 1);
        RegionInit(&NewReg, &(resize->box), 1);
        RegionSubtract(&FreedReg, &FreedReg, &NewReg);
        RegionUnion(offman->FreeBoxes, offman->FreeBoxes, &FreedReg);
        RegionUninit(&FreedReg);
        RegionUninit(&NewReg);

        SendCallFreeBoxCallbacks(offman);

        return TRUE;
    }

    /* otherwise we remove the old region */

    RegionInit(&FreedReg, &OrigArea, 1);
    RegionUnion(offman->FreeBoxes, offman->FreeBoxes, &FreedReg);

    /* remove the old link */
    if (pLinkPrev)
        pLinkPrev->next = pLink->next;
    else
        offman->UsedAreas = pLink->next;

    /* and try to add a new one */

    if ((area = AllocateArea(offman, w, h, resize->granularity,
                             resize->MoveAreaCallback,
                             resize->RemoveAreaCallback,
                             resize->devPrivate.ptr))) {

        /* copy data over to our link and replace the new with old */
        memcpy(resize, area, sizeof(FBArea));

        pLinkPrev = NULL;
        newLink = offman->UsedAreas;

        while (&(newLink->area) != area) {
            pLinkPrev = newLink;
            newLink = newLink->next;
        }

        if (pLinkPrev)
            pLinkPrev->next = newLink->next;
        else
            offman->UsedAreas = newLink->next;

        pLink->next = offman->UsedAreas;
        offman->UsedAreas = pLink;

        free(newLink);

        /* AllocateArea added one but we really only exchanged one */
        offman->NumUsedAreas--;
    }
    else {
        /* reinstate the old region */
        RegionSubtract(offman->FreeBoxes, offman->FreeBoxes, &FreedReg);
        RegionUninit(&FreedReg);

        pLink->next = offman->UsedAreas;
        offman->UsedAreas = pLink;
        return FALSE;
    }

    RegionUninit(&FreedReg);

    SendCallFreeBoxCallbacks(offman);

    return TRUE;
}

static Bool
localQueryLargestOffscreenArea(ScreenPtr pScreen,
                               int *width, int *height,
                               int granularity, int preferences, int severity)
{
    FBManagerPtr offman;
    RegionPtr newRegion = NULL;
    BoxPtr pbox;
    int nbox;
    int x, w, h, area, oldArea;

    *width = *height = oldArea = 0;

    if (granularity <= 1)
        granularity = 0;

    if ((preferences < 0) || (preferences > 3))
        return FALSE;

    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    if (severity < 0)
        severity = 0;
    if (severity > 2)
        severity = 2;

    switch (severity) {
    case 2:
        if (offman->NumUsedAreas) {
            FBLinkPtr pLink;
            RegionRec tmpRegion;

            newRegion = RegionCreate(NULL, 1);
            RegionCopy(newRegion, offman->InitialBoxes);
            pLink = offman->UsedAreas;

            while (pLink) {
                if (!pLink->area.RemoveAreaCallback) {
                    RegionInit(&tmpRegion, &(pLink->area.box), 1);
                    RegionSubtract(newRegion, newRegion, &tmpRegion);
                    RegionUninit(&tmpRegion);
                }
                pLink = pLink->next;
            }

            nbox = RegionNumRects(newRegion);
            pbox = RegionRects(newRegion);
            break;
        }
    case 1:
        if (offman->NumUsedAreas) {
            FBLinkPtr pLink;
            RegionRec tmpRegion;

            newRegion = RegionCreate(NULL, 1);
            RegionCopy(newRegion, offman->FreeBoxes);
            pLink = offman->UsedAreas;

            while (pLink) {
                if (pLink->area.RemoveAreaCallback) {
                    RegionInit(&tmpRegion, &(pLink->area.box), 1);
                    RegionAppend(newRegion, &tmpRegion);
                    RegionUninit(&tmpRegion);
                }
                pLink = pLink->next;
            }

            nbox = RegionNumRects(newRegion);
            pbox = RegionRects(newRegion);
            break;
        }
    default:
        nbox = RegionNumRects(offman->FreeBoxes);
        pbox = RegionRects(offman->FreeBoxes);
        break;
    }

    while (nbox--) {
        x = pbox->x1;
        if (granularity > 1)
            x = ((x + granularity - 1) / granularity) * granularity;

        w = pbox->x2 - x;
        h = pbox->y2 - pbox->y1;
        area = w * h;

        if (w > 0) {
            Bool gotIt = FALSE;

            switch (preferences) {
            case FAVOR_AREA_THEN_WIDTH:
                if ((area > oldArea) || ((area == oldArea) && (w > *width)))
                    gotIt = TRUE;
                break;
            case FAVOR_AREA_THEN_HEIGHT:
                if ((area > oldArea) || ((area == oldArea) && (h > *height)))
                    gotIt = TRUE;
                break;
            case FAVOR_WIDTH_THEN_AREA:
                if ((w > *width) || ((w == *width) && (area > oldArea)))
                    gotIt = TRUE;
                break;
            case FAVOR_HEIGHT_THEN_AREA:
                if ((h > *height) || ((h == *height) && (area > oldArea)))
                    gotIt = TRUE;
                break;
            }
            if (gotIt) {
                *width = w;
                *height = h;
                oldArea = area;
            }
        }
        pbox++;
    }

    if (newRegion)
        RegionDestroy(newRegion);

    return TRUE;
}

static Bool
localPurgeUnlockedOffscreenAreas(ScreenPtr pScreen)
{
    FBManagerPtr offman;
    FBLinkPtr pLink, tmp, pPrev = NULL;
    RegionRec FreedRegion;
    Bool anyUsed = FALSE;

    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    pLink = offman->UsedAreas;
    if (!pLink)
        return TRUE;

    while (pLink) {
        if (pLink->area.RemoveAreaCallback) {
            (*pLink->area.RemoveAreaCallback) (&pLink->area);

            RegionInit(&FreedRegion, &(pLink->area.box), 1);
            RegionAppend(offman->FreeBoxes, &FreedRegion);
            RegionUninit(&FreedRegion);

            if (pPrev)
                pPrev->next = pLink->next;
            else
                offman->UsedAreas = pLink->next;

            tmp = pLink;
            pLink = pLink->next;
            free(tmp);
            offman->NumUsedAreas--;
            anyUsed = TRUE;
        }
        else {
            pPrev = pLink;
            pLink = pLink->next;
        }
    }

    if (anyUsed) {
        RegionValidate(offman->FreeBoxes, &anyUsed);
        SendCallFreeBoxCallbacks(offman);
    }

    return TRUE;
}

static void
LinearMoveCBWrapper(FBAreaPtr from, FBAreaPtr to)
{
    /* this will never get called */
}

static void
LinearRemoveCBWrapper(FBAreaPtr area)
{
    FBManagerPtr offman;
    FBLinearLinkPtr pLink, pLinkPrev = NULL;
    ScreenPtr pScreen = area->pScreen;

    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    pLink = offman->LinearAreas;
    if (!pLink)
        return;

    while (pLink->area != area) {
        pLinkPrev = pLink;
        pLink = pLink->next;
        if (!pLink)
            return;
    }

    /* give the user the callback it is expecting */
    (*pLink->linear.RemoveLinearCallback) (&(pLink->linear));

    if (pLinkPrev)
        pLinkPrev->next = pLink->next;
    else
        offman->LinearAreas = pLink->next;

    free(pLink);
}

static void
DumpDebug(FBLinearLinkPtr pLink)
{
#ifdef DEBUG
    if (!pLink)
        ErrorF("MMmm, PLINK IS NULL!\n");

    while (pLink) {
        ErrorF("  Offset:%08x, Size:%08x, %s,%s\n",
               pLink->linear.offset,
               pLink->linear.size,
               pLink->free ? "Free" : "Used", pLink->area ? "Area" : "Linear");

        pLink = pLink->next;
    }
#endif
}

static FBLinearPtr
AllocateLinear(FBManagerPtr offman, int size, int granularity, void *privData)
{
    ScreenPtr pScreen = offman->pScreen;
    FBLinearLinkPtr linear = NULL;
    FBLinearLinkPtr newlink = NULL;
    int offset, end;

    if (size <= 0)
        return NULL;

    if (!offman->LinearAreas)
        return NULL;

    linear = offman->LinearAreas;
    while (linear) {
        /* Make sure we get a free area that's not an XY fallback case */
        if (!linear->area && linear->free) {
            offset = linear->linear.offset;
            if (granularity > 1)
                offset =
                    ((offset + granularity - 1) / granularity) * granularity;
            end = offset + size;
            if (end <= (linear->linear.offset + linear->linear.size))
                break;
        }
        linear = linear->next;
    }
    if (!linear)
        return NULL;

    /* break left */
    if (offset > linear->linear.offset) {
        newlink = malloc(sizeof(FBLinearLink));
        if (!newlink)
            return NULL;
        newlink->area = NULL;
        newlink->linear.offset = offset;
        newlink->linear.size =
            linear->linear.size - (offset - linear->linear.offset);
        newlink->free = 1;
        newlink->next = linear->next;
        linear->linear.size -= newlink->linear.size;
        linear->next = newlink;
        linear = newlink;
    }

    /* break right */
    if (size < linear->linear.size) {
        newlink = malloc(sizeof(FBLinearLink));
        if (!newlink)
            return NULL;
        newlink->area = NULL;
        newlink->linear.offset = offset + size;
        newlink->linear.size = linear->linear.size - size;
        newlink->free = 1;
        newlink->next = linear->next;
        linear->linear.size = size;
        linear->next = newlink;
    }

    /* p = middle block */
    linear->linear.granularity = granularity;
    linear->free = 0;
    linear->linear.pScreen = pScreen;
    linear->linear.MoveLinearCallback = NULL;
    linear->linear.RemoveLinearCallback = NULL;
    linear->linear.devPrivate.ptr = NULL;

    DumpDebug(offman->LinearAreas);

    return &(linear->linear);
}

static FBLinearPtr
localAllocateOffscreenLinear(ScreenPtr pScreen,
                             int length,
                             int gran,
                             MoveLinearCallbackProcPtr moveCB,
                             RemoveLinearCallbackProcPtr removeCB,
                             void *privData)
{
    FBManagerPtr offman;
    FBLinearLinkPtr link;
    FBAreaPtr area;
    FBLinearPtr linear = NULL;
    BoxPtr extents;
    int w, h, pitch;

    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);

    /* Try to allocate from linear memory first...... */
    DebugF("ALLOCATING LINEAR\n");
    if ((linear = AllocateLinear(offman, length, gran, privData)))
        return linear;

    DebugF("NOPE, ALLOCATING AREA\n");

    if (!(link = malloc(sizeof(FBLinearLink))))
        return NULL;

    /* No linear available, so try and pinch some from the XY areas */
    extents = RegionExtents(offman->InitialBoxes);
    pitch = extents->x2 - extents->x1;

    if (gran > 1) {
        if (gran > pitch) {
            /* we can't match the specified alignment with XY allocations */
            free(link);
            return NULL;
        }

        if (pitch % gran) {
            /* pitch and granularity aren't a perfect match, let's allocate
             * a bit more so we can align later on
             */
            length += gran - 1;
        }
    }

    if (length < pitch) {       /* special case */
        w = length;
        h = 1;
    }
    else {
        w = pitch;
        h = (length + pitch - 1) / pitch;
    }

    if ((area = localAllocateOffscreenArea(pScreen, w, h, gran,
                                           moveCB ? LinearMoveCBWrapper : NULL,
                                           removeCB ? LinearRemoveCBWrapper :
                                           NULL, privData))) {
        link->area = area;
        link->free = 0;
        link->next = offman->LinearAreas;
        offman->LinearAreas = link;
        linear = &(link->linear);
        linear->pScreen = pScreen;
        linear->size = h * w;
        linear->offset = (pitch * area->box.y1) + area->box.x1;
        if (gran > 1)
            linear->offset = ((linear->offset + gran - 1) / gran) * gran;
        linear->granularity = gran;
        linear->MoveLinearCallback = moveCB;
        linear->RemoveLinearCallback = removeCB;
        linear->devPrivate.ptr = privData;
    }
    else
        free(link);

    DumpDebug(offman->LinearAreas);

    return linear;
}

static void
localFreeOffscreenLinear(FBLinearPtr linear)
{
    FBManagerPtr offman;
    FBLinearLinkPtr pLink, pLinkPrev = NULL;
    ScreenPtr pScreen = linear->pScreen;

    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    pLink = offman->LinearAreas;
    if (!pLink)
        return;

    while (&(pLink->linear) != linear) {
        pLinkPrev = pLink;
        pLink = pLink->next;
        if (!pLink)
            return;
    }

    if (pLink->area) {          /* really an XY area */
        DebugF("FREEING AREA\n");
        localFreeOffscreenArea(pLink->area);
        if (pLinkPrev)
            pLinkPrev->next = pLink->next;
        else
            offman->LinearAreas = pLink->next;
        free(pLink);
        DumpDebug(offman->LinearAreas);
        return;
    }

    pLink->free = 1;

    if (pLink->next && pLink->next->free) {
        FBLinearLinkPtr p = pLink->next;

        pLink->linear.size += p->linear.size;
        pLink->next = p->next;
        free(p);
    }

    if (pLinkPrev) {
        if (pLinkPrev->next && pLinkPrev->next->free && !pLinkPrev->area) {
            FBLinearLinkPtr p = pLinkPrev->next;

            pLinkPrev->linear.size += p->linear.size;
            pLinkPrev->next = p->next;
            free(p);
        }
    }

    DebugF("FREEING LINEAR\n");
    DumpDebug(offman->LinearAreas);
}

static Bool
localResizeOffscreenLinear(FBLinearPtr resize, int length)
{
    FBManagerPtr offman;
    FBLinearLinkPtr pLink;
    ScreenPtr pScreen = resize->pScreen;

    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    pLink = offman->LinearAreas;
    if (!pLink)
        return FALSE;

    while (&(pLink->linear) != resize) {
        pLink = pLink->next;
        if (!pLink)
            return FALSE;
    }

    /* This could actually be a lot smarter and try to move allocations
       from XY to linear when available.  For now if it was XY, we keep
       it XY */

    if (pLink->area) {          /* really an XY area */
        BoxPtr extents;
        int pitch, w, h;

        extents = RegionExtents(offman->InitialBoxes);
        pitch = extents->x2 - extents->x1;

        if (length < pitch) {   /* special case */
            w = length;
            h = 1;
        }
        else {
            w = pitch;
            h = (length + pitch - 1) / pitch;
        }

        if (localResizeOffscreenArea(pLink->area, w, h)) {
            resize->size = h * w;
            resize->offset =
                (pitch * pLink->area->box.y1) + pLink->area->box.x1;
            return TRUE;
        }
    }
    else {
        /* TODO!!!! resize the linear area */
    }

    return FALSE;
}

static Bool
localQueryLargestOffscreenLinear(ScreenPtr pScreen,
                                 int *size, int gran, int priority)
{
    FBManagerPtr offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                          xf86FBScreenKey);
    FBLinearLinkPtr pLink;
    FBLinearLinkPtr pLinkRet;

    *size = 0;

    pLink = offman->LinearAreas;

    if (pLink && !pLink->area) {
        pLinkRet = pLink;
        while (pLink) {
            if (pLink->free) {
                if (pLink->linear.size > pLinkRet->linear.size)
                    pLinkRet = pLink;
            }
            pLink = pLink->next;
        }

        if (pLinkRet->free) {
            *size = pLinkRet->linear.size;
            return TRUE;
        }
    }
    else {
        int w, h;

        if (localQueryLargestOffscreenArea(pScreen, &w, &h, gran,
                                           FAVOR_WIDTH_THEN_AREA, priority)) {
            BoxPtr extents;

            extents = RegionExtents(offman->InitialBoxes);
            if ((extents->x2 - extents->x1) == w)
                *size = w * h;
            return TRUE;
        }
    }

    return FALSE;
}

static FBManagerFuncs xf86FBManFuncs = {
    localAllocateOffscreenArea,
    localFreeOffscreenArea,
    localResizeOffscreenArea,
    localQueryLargestOffscreenArea,
    localRegisterFreeBoxCallback,
    localAllocateOffscreenLinear,
    localFreeOffscreenLinear,
    localResizeOffscreenLinear,
    localQueryLargestOffscreenLinear,
    localPurgeUnlockedOffscreenAreas
};

static Bool
xf86FBCloseScreen(ScreenPtr pScreen)
{
    FBLinkPtr pLink, tmp;
    FBLinearLinkPtr pLinearLink, tmp2;
    FBManagerPtr offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                          xf86FBScreenKey);

    pScreen->CloseScreen = offman->CloseScreen;

    pLink = offman->UsedAreas;
    while (pLink) {
        tmp = pLink;
        pLink = pLink->next;
        free(tmp);
    }

    pLinearLink = offman->LinearAreas;
    while (pLinearLink) {
        tmp2 = pLinearLink;
        pLinearLink = pLinearLink->next;
        free(tmp2);
    }

    RegionDestroy(offman->InitialBoxes);
    RegionDestroy(offman->FreeBoxes);

    free(offman->FreeBoxesUpdateCallback);
    free(offman->devPrivates);
    free(offman);
    dixSetPrivate(&pScreen->devPrivates, xf86FBScreenKey, NULL);

    return (*pScreen->CloseScreen) (pScreen);
}

Bool
xf86InitFBManager(ScreenPtr pScreen, BoxPtr FullBox)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    RegionRec ScreenRegion;
    RegionRec FullRegion;
    BoxRec ScreenBox;
    Bool ret;

    ScreenBox.x1 = 0;
    ScreenBox.y1 = 0;
    ScreenBox.x2 = pScrn->virtualX;
    ScreenBox.y2 = pScrn->virtualY;

    if ((FullBox->x1 > ScreenBox.x1) || (FullBox->y1 > ScreenBox.y1) ||
        (FullBox->x2 < ScreenBox.x2) || (FullBox->y2 < ScreenBox.y2)) {
        return FALSE;
    }

    if (FullBox->y2 < FullBox->y1)
        return FALSE;
    if (FullBox->x2 < FullBox->x1)
        return FALSE;

    RegionInit(&ScreenRegion, &ScreenBox, 1);
    RegionInit(&FullRegion, FullBox, 1);

    RegionSubtract(&FullRegion, &FullRegion, &ScreenRegion);

    ret = xf86InitFBManagerRegion(pScreen, &FullRegion);

    RegionUninit(&ScreenRegion);
    RegionUninit(&FullRegion);

    return ret;
}

Bool
xf86InitFBManagerArea(ScreenPtr pScreen, int PixelArea, int Verbosity)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    xRectangle Rect[3];
    RegionPtr pRegion, pScreenRegion;
    int nRect;
    Bool ret = FALSE;

    if (PixelArea < (pScrn->displayWidth * pScrn->virtualY))
        return FALSE;

    Rect[0].x = Rect[0].y = 0;
    Rect[0].width = pScrn->displayWidth;
    Rect[0].height = PixelArea / pScrn->displayWidth;
    nRect = 1;

    /* Add a possible partial scanline */
    if ((Rect[1].height = Rect[1].width = PixelArea % pScrn->displayWidth)) {
        Rect[1].x = 0;
        Rect[1].y = Rect[0].height;
        Rect[1].height = 1;
        nRect++;
    }

    /* Factor out virtual resolution */
    pRegion = RegionFromRects(nRect, Rect, 0);
    if (pRegion) {
        if (!RegionNar(pRegion)) {
            Rect[2].x = Rect[2].y = 0;
            Rect[2].width = pScrn->virtualX;
            Rect[2].height = pScrn->virtualY;

            pScreenRegion = RegionFromRects(1, &Rect[2], 0);
            if (pScreenRegion) {
                if (!RegionNar(pScreenRegion)) {
                    RegionSubtract(pRegion, pRegion, pScreenRegion);

                    ret = xf86InitFBManagerRegion(pScreen, pRegion);

                    if (ret && xf86GetVerbosity() >= Verbosity) {
                        int scrnIndex = pScrn->scrnIndex;

                        xf86DrvMsgVerb(scrnIndex, X_INFO, Verbosity,
                                       "Largest offscreen areas (with overlaps):\n");

                        if (Rect[2].width < Rect[0].width) {
                            xf86DrvMsgVerb(scrnIndex, X_INFO, Verbosity,
                                           "\t%d x %d rectangle at %d,0\n",
                                           Rect[0].width - Rect[2].width,
                                           Rect[0].height, Rect[2].width);
                        }
                        if (Rect[2].width < Rect[1].width) {
                            xf86DrvMsgVerb(scrnIndex, X_INFO, Verbosity,
                                           "\t%d x %d rectangle at %d,0\n",
                                           Rect[1].width - Rect[2].width,
                                           Rect[0].height + Rect[1].height,
                                           Rect[2].width);
                        }
                        if (Rect[2].height < Rect[0].height) {
                            xf86DrvMsgVerb(scrnIndex, X_INFO, Verbosity,
                                           "\t%d x %d rectangle at 0,%d\n",
                                           Rect[0].width,
                                           Rect[0].height - Rect[2].height,
                                           Rect[2].height);
                        }
                        if (Rect[1].height) {
                            xf86DrvMsgVerb(scrnIndex, X_INFO, Verbosity,
                                           "\t%d x %d rectangle at 0,%d\n",
                                           Rect[1].width,
                                           Rect[0].height - Rect[2].height +
                                           Rect[1].height, Rect[2].height);
                        }
                    }
                }

                RegionDestroy(pScreenRegion);
            }
        }

        RegionDestroy(pRegion);
    }

    return ret;
}

Bool
xf86InitFBManagerRegion(ScreenPtr pScreen, RegionPtr FullRegion)
{
    FBManagerPtr offman;

    if (RegionNil(FullRegion))
        return FALSE;

    if (!dixRegisterPrivateKey(&xf86FBScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (!xf86RegisterOffscreenManager(pScreen, &xf86FBManFuncs))
        return FALSE;

    offman = malloc(sizeof(FBManager));
    if (!offman)
        return FALSE;

    dixSetPrivate(&pScreen->devPrivates, xf86FBScreenKey, offman);

    offman->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = xf86FBCloseScreen;

    offman->InitialBoxes = RegionCreate(NULL, 1);
    offman->FreeBoxes = RegionCreate(NULL, 1);

    RegionCopy(offman->InitialBoxes, FullRegion);
    RegionCopy(offman->FreeBoxes, FullRegion);

    offman->pScreen = pScreen;
    offman->UsedAreas = NULL;
    offman->LinearAreas = NULL;
    offman->NumUsedAreas = 0;
    offman->NumCallbacks = 0;
    offman->FreeBoxesUpdateCallback = NULL;
    offman->devPrivates = NULL;

    return TRUE;
}

Bool
xf86InitFBManagerLinear(ScreenPtr pScreen, int offset, int size)
{
    FBManagerPtr offman;
    FBLinearLinkPtr link;
    FBLinearPtr linear;

    if (size <= 0)
        return FALSE;

    /* we expect people to have called the Area setup first for pixmap cache */
    if (!dixLookupPrivate(&pScreen->devPrivates, xf86FBScreenKey))
        return FALSE;

    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    offman->LinearAreas = malloc(sizeof(FBLinearLink));
    if (!offman->LinearAreas)
        return FALSE;

    link = offman->LinearAreas;
    link->area = NULL;
    link->next = NULL;
    link->free = 1;
    linear = &(link->linear);
    linear->pScreen = pScreen;
    linear->size = size;
    linear->offset = offset;
    linear->granularity = 0;
    linear->MoveLinearCallback = NULL;
    linear->RemoveLinearCallback = NULL;
    linear->devPrivate.ptr = NULL;

    return TRUE;
}

/* This is an implementation specific function and should
   disappear after the next release.  People should use the
   real linear functions instead */

FBAreaPtr
xf86AllocateLinearOffscreenArea(ScreenPtr pScreen,
                                int length,
                                int gran,
                                MoveAreaCallbackProcPtr moveCB,
                                RemoveAreaCallbackProcPtr removeCB,
                                void *privData)
{
    FBManagerFuncsPtr funcs;
    FBManagerPtr offman;
    BoxPtr extents;
    int w, h;

    if (xf86FBManagerKey == NULL)
        return NULL;
    if (!(funcs = (FBManagerFuncsPtr) dixLookupPrivate(&pScreen->devPrivates,
                                                       xf86FBManagerKey)))
        return NULL;

    offman = (FBManagerPtr) dixLookupPrivate(&pScreen->devPrivates,
                                             xf86FBScreenKey);
    extents = RegionExtents(offman->InitialBoxes);
    w = extents->x2 - extents->x1;

    if (gran > 1) {
        if (gran > w)
            return NULL;

        if (w % gran)
            length += gran - 1;
    }

    if (length <= w) {          /* special case */
        h = 1;
        w = length;
    }
    else {
        h = (length + w - 1) / w;
    }

    return (*funcs->AllocateOffscreenArea) (pScreen, w, h, gran, moveCB,
                                            removeCB, privData);
}
