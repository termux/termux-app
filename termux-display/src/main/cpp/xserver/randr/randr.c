/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett-Packard Company
 * Copyright © 2006 Intel Corporation
 * Copyright © 2017 Keith Packard
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
 *
 * Author:  Jim Gettys, Hewlett-Packard Company, Inc.
 *	    Keith Packard, Intel Corporation
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "randrstr.h"
#include "extinit.h"

/* From render.h */
#ifndef SubPixelUnknown
#define SubPixelUnknown 0
#endif

#define RR_VALIDATE
static int RRNScreens;

#define wrap(priv,real,mem,func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define unwrap(priv,real,mem) {\
    real->mem = priv->mem; \
}

static int ProcRRDispatch(ClientPtr pClient);
static int SProcRRDispatch(ClientPtr pClient);

int RREventBase;
int RRErrorBase;
RESTYPE RRClientType, RREventType;      /* resource types for event masks */
DevPrivateKeyRec RRClientPrivateKeyRec;

DevPrivateKeyRec rrPrivKeyRec;

static void
RRClientCallback(CallbackListPtr *list, void *closure, void *data)
{
    NewClientInfoRec *clientinfo = (NewClientInfoRec *) data;
    ClientPtr pClient = clientinfo->client;

    rrClientPriv(pClient);
    RRTimesPtr pTimes = (RRTimesPtr) (pRRClient + 1);
    int i;

    pRRClient->major_version = 0;
    pRRClient->minor_version = 0;
    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];

        rrScrPriv(pScreen);

        if (pScrPriv) {
            pTimes[i].setTime = pScrPriv->lastSetTime;
            pTimes[i].configTime = pScrPriv->lastConfigTime;
        }
    }
}

static Bool
RRCloseScreen(ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    int j;
    RRLeasePtr lease, next;

    unwrap(pScrPriv, pScreen, CloseScreen);

    xorg_list_for_each_entry_safe(lease, next, &pScrPriv->leases, list)
        RRTerminateLease(lease);
    for (j = pScrPriv->numCrtcs - 1; j >= 0; j--)
        RRCrtcDestroy(pScrPriv->crtcs[j]);
    for (j = pScrPriv->numOutputs - 1; j >= 0; j--)
        RROutputDestroy(pScrPriv->outputs[j]);

    if (pScrPriv->provider)
        RRProviderDestroy(pScrPriv->provider);

    RRMonitorClose(pScreen);

    free(pScrPriv->crtcs);
    free(pScrPriv->outputs);
    free(pScrPriv);
    RRNScreens -= 1;            /* ok, one fewer screen with RandR running */
    return (*pScreen->CloseScreen) (pScreen);
}

static void
SRRScreenChangeNotifyEvent(xRRScreenChangeNotifyEvent * from,
                           xRRScreenChangeNotifyEvent * to)
{
    to->type = from->type;
    to->rotation = from->rotation;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->configTimestamp, to->configTimestamp);
    cpswapl(from->root, to->root);
    cpswapl(from->window, to->window);
    cpswaps(from->sizeID, to->sizeID);
    cpswaps(from->subpixelOrder, to->subpixelOrder);
    cpswaps(from->widthInPixels, to->widthInPixels);
    cpswaps(from->heightInPixels, to->heightInPixels);
    cpswaps(from->widthInMillimeters, to->widthInMillimeters);
    cpswaps(from->heightInMillimeters, to->heightInMillimeters);
}

static void
SRRCrtcChangeNotifyEvent(xRRCrtcChangeNotifyEvent * from,
                         xRRCrtcChangeNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->window, to->window);
    cpswapl(from->crtc, to->crtc);
    cpswapl(from->mode, to->mode);
    cpswaps(from->rotation, to->rotation);
    /* pad1 */
    cpswaps(from->x, to->x);
    cpswaps(from->y, to->y);
    cpswaps(from->width, to->width);
    cpswaps(from->height, to->height);
}

static void
SRROutputChangeNotifyEvent(xRROutputChangeNotifyEvent * from,
                           xRROutputChangeNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->configTimestamp, to->configTimestamp);
    cpswapl(from->window, to->window);
    cpswapl(from->output, to->output);
    cpswapl(from->crtc, to->crtc);
    cpswapl(from->mode, to->mode);
    cpswaps(from->rotation, to->rotation);
    to->connection = from->connection;
    to->subpixelOrder = from->subpixelOrder;
}

static void
SRROutputPropertyNotifyEvent(xRROutputPropertyNotifyEvent * from,
                             xRROutputPropertyNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->window, to->window);
    cpswapl(from->output, to->output);
    cpswapl(from->atom, to->atom);
    cpswapl(from->timestamp, to->timestamp);
    to->state = from->state;
    /* pad1 */
    /* pad2 */
    /* pad3 */
    /* pad4 */
}

static void
SRRProviderChangeNotifyEvent(xRRProviderChangeNotifyEvent * from,
                         xRRProviderChangeNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->window, to->window);
    cpswapl(from->provider, to->provider);
}

static void
SRRProviderPropertyNotifyEvent(xRRProviderPropertyNotifyEvent * from,
                               xRRProviderPropertyNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->window, to->window);
    cpswapl(from->provider, to->provider);
    cpswapl(from->atom, to->atom);
    cpswapl(from->timestamp, to->timestamp);
    to->state = from->state;
    /* pad1 */
    /* pad2 */
    /* pad3 */
    /* pad4 */
}

static void _X_COLD
SRRResourceChangeNotifyEvent(xRRResourceChangeNotifyEvent * from,
                             xRRResourceChangeNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->window, to->window);
}

static void _X_COLD
SRRLeaseNotifyEvent(xRRLeaseNotifyEvent * from,
                    xRRLeaseNotifyEvent * to)
{
    to->type = from->type;
    to->subCode = from->subCode;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->window, to->window);
    cpswapl(from->lease, to->lease);
    to->created = from->created;
}

static void _X_COLD
SRRNotifyEvent(xEvent *from, xEvent *to)
{
    switch (from->u.u.detail) {
    case RRNotify_CrtcChange:
        SRRCrtcChangeNotifyEvent((xRRCrtcChangeNotifyEvent *) from,
                                 (xRRCrtcChangeNotifyEvent *) to);
        break;
    case RRNotify_OutputChange:
        SRROutputChangeNotifyEvent((xRROutputChangeNotifyEvent *) from,
                                   (xRROutputChangeNotifyEvent *) to);
        break;
    case RRNotify_OutputProperty:
        SRROutputPropertyNotifyEvent((xRROutputPropertyNotifyEvent *) from,
                                     (xRROutputPropertyNotifyEvent *) to);
        break;
    case RRNotify_ProviderChange:
        SRRProviderChangeNotifyEvent((xRRProviderChangeNotifyEvent *) from,
                                   (xRRProviderChangeNotifyEvent *) to);
        break;
    case RRNotify_ProviderProperty:
        SRRProviderPropertyNotifyEvent((xRRProviderPropertyNotifyEvent *) from,
                                       (xRRProviderPropertyNotifyEvent *) to);
        break;
    case RRNotify_ResourceChange:
        SRRResourceChangeNotifyEvent((xRRResourceChangeNotifyEvent *) from,
                                   (xRRResourceChangeNotifyEvent *) to);
        break;
    case RRNotify_Lease:
        SRRLeaseNotifyEvent((xRRLeaseNotifyEvent *) from,
                            (xRRLeaseNotifyEvent *) to);
        break;
    default:
        break;
    }
}

static int RRGeneration;

Bool
RRInit(void)
{
    if (RRGeneration != serverGeneration) {
        if (!RRModeInit())
            return FALSE;
        if (!RRCrtcInit())
            return FALSE;
        if (!RROutputInit())
            return FALSE;
        if (!RRProviderInit())
            return FALSE;
        if (!RRLeaseInit())
            return FALSE;
        RRGeneration = serverGeneration;
    }
    if (!dixRegisterPrivateKey(&rrPrivKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    return TRUE;
}

Bool
RRScreenInit(ScreenPtr pScreen)
{
    rrScrPrivPtr pScrPriv;

    if (!RRInit())
        return FALSE;

    pScrPriv = (rrScrPrivPtr) calloc(1, sizeof(rrScrPrivRec));
    if (!pScrPriv)
        return FALSE;

    SetRRScreen(pScreen, pScrPriv);

    /*
     * Calling function best set these function vectors
     */
    pScrPriv->rrGetInfo = 0;
    pScrPriv->maxWidth = pScrPriv->minWidth = pScreen->width;
    pScrPriv->maxHeight = pScrPriv->minHeight = pScreen->height;

    pScrPriv->width = pScreen->width;
    pScrPriv->height = pScreen->height;
    pScrPriv->mmWidth = pScreen->mmWidth;
    pScrPriv->mmHeight = pScreen->mmHeight;
#if RANDR_12_INTERFACE
    pScrPriv->rrScreenSetSize = NULL;
    pScrPriv->rrCrtcSet = NULL;
    pScrPriv->rrCrtcSetGamma = NULL;
#endif
#if RANDR_10_INTERFACE
    pScrPriv->rrSetConfig = 0;
    pScrPriv->rotations = RR_Rotate_0;
    pScrPriv->reqWidth = pScreen->width;
    pScrPriv->reqHeight = pScreen->height;
    pScrPriv->nSizes = 0;
    pScrPriv->pSizes = NULL;
    pScrPriv->rotation = RR_Rotate_0;
    pScrPriv->rate = 0;
    pScrPriv->size = 0;
#endif

    /*
     * This value doesn't really matter -- any client must call
     * GetScreenInfo before reading it which will automatically update
     * the time
     */
    pScrPriv->lastSetTime = currentTime;
    pScrPriv->lastConfigTime = currentTime;

    wrap(pScrPriv, pScreen, CloseScreen, RRCloseScreen);

    pScreen->ConstrainCursorHarder = RRConstrainCursorHarder;
    pScreen->ReplaceScanoutPixmap = RRReplaceScanoutPixmap;
    pScrPriv->numOutputs = 0;
    pScrPriv->outputs = NULL;
    pScrPriv->numCrtcs = 0;
    pScrPriv->crtcs = NULL;

    xorg_list_init(&pScrPriv->leases);

    RRMonitorInit(pScreen);

    RRNScreens += 1;            /* keep count of screens that implement randr */
    return TRUE;
}

 /*ARGSUSED*/ static int
RRFreeClient(void *data, XID id)
{
    RREventPtr pRREvent;
    WindowPtr pWin;
    RREventPtr *pHead, pCur, pPrev;

    pRREvent = (RREventPtr) data;
    pWin = pRREvent->window;
    dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                            RREventType, serverClient, DixDestroyAccess);
    if (pHead) {
        pPrev = 0;
        for (pCur = *pHead; pCur && pCur != pRREvent; pCur = pCur->next)
            pPrev = pCur;
        if (pCur) {
            if (pPrev)
                pPrev->next = pRREvent->next;
            else
                *pHead = pRREvent->next;
        }
    }
    free((void *) pRREvent);
    return 1;
}

 /*ARGSUSED*/ static int
RRFreeEvents(void *data, XID id)
{
    RREventPtr *pHead, pCur, pNext;

    pHead = (RREventPtr *) data;
    for (pCur = *pHead; pCur; pCur = pNext) {
        pNext = pCur->next;
        FreeResource(pCur->clientResource, RRClientType);
        free((void *) pCur);
    }
    free((void *) pHead);
    return 1;
}

void
RRExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (RRNScreens == 0)
        return;

    if (!dixRegisterPrivateKey(&RRClientPrivateKeyRec, PRIVATE_CLIENT,
                               sizeof(RRClientRec) +
                               screenInfo.numScreens * sizeof(RRTimesRec)))
        return;
    if (!AddCallback(&ClientStateCallback, RRClientCallback, 0))
        return;

    RRClientType = CreateNewResourceType(RRFreeClient, "RandRClient");
    if (!RRClientType)
        return;
    RREventType = CreateNewResourceType(RRFreeEvents, "RandREvent");
    if (!RREventType)
        return;
    extEntry = AddExtension(RANDR_NAME, RRNumberEvents, RRNumberErrors,
                            ProcRRDispatch, SProcRRDispatch,
                            NULL, StandardMinorOpcode);
    if (!extEntry)
        return;
    RRErrorBase = extEntry->errorBase;
    RREventBase = extEntry->eventBase;
    EventSwapVector[RREventBase + RRScreenChangeNotify] = (EventSwapPtr)
        SRRScreenChangeNotifyEvent;
    EventSwapVector[RREventBase + RRNotify] = (EventSwapPtr)
        SRRNotifyEvent;

    RRModeInitErrorValue();
    RRCrtcInitErrorValue();
    RROutputInitErrorValue();
    RRProviderInitErrorValue();
#ifdef PANORAMIX
    RRXineramaExtensionInit();
#endif
}

void
RRResourcesChanged(ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    pScrPriv->resourcesChanged = TRUE;

    RRSetChanged(pScreen);
}

static void
RRDeliverResourceEvent(ClientPtr client, WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    rrScrPriv(pScreen);

    xRRResourceChangeNotifyEvent re = {
        .type = RRNotify + RREventBase,
        .subCode = RRNotify_ResourceChange,
        .timestamp = pScrPriv->lastSetTime.milliseconds,
        .window = pWin->drawable.id
    };

    WriteEventsToClient(client, 1, (xEvent *) &re);
}

static int
TellChanged(WindowPtr pWin, void *value)
{
    RREventPtr *pHead, pRREvent;
    ClientPtr client;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    ScreenPtr iter;
    rrScrPrivPtr pSecondaryScrPriv;

    rrScrPriv(pScreen);
    int i;

    dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                            RREventType, serverClient, DixReadAccess);
    if (!pHead)
        return WT_WALKCHILDREN;

    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) {
        client = pRREvent->client;
        if (client == serverClient || client->clientGone)
            continue;

        if (pRREvent->mask & RRScreenChangeNotifyMask)
            RRDeliverScreenEvent(client, pWin, pScreen);

        if (pRREvent->mask & RRCrtcChangeNotifyMask) {
            for (i = 0; i < pScrPriv->numCrtcs; i++) {
                RRCrtcPtr crtc = pScrPriv->crtcs[i];

                if (crtc->changed)
                    RRDeliverCrtcEvent(client, pWin, crtc);
            }

            xorg_list_for_each_entry(iter, &pScreen->secondary_list, secondary_head) {
                if (!iter->is_output_secondary)
                    continue;

                pSecondaryScrPriv = rrGetScrPriv(iter);
                for (i = 0; i < pSecondaryScrPriv->numCrtcs; i++) {
                    RRCrtcPtr crtc = pSecondaryScrPriv->crtcs[i];

                    if (crtc->changed)
                        RRDeliverCrtcEvent(client, pWin, crtc);
                }
            }
        }

        if (pRREvent->mask & RROutputChangeNotifyMask) {
            for (i = 0; i < pScrPriv->numOutputs; i++) {
                RROutputPtr output = pScrPriv->outputs[i];

                if (output->changed)
                    RRDeliverOutputEvent(client, pWin, output);
            }

            xorg_list_for_each_entry(iter, &pScreen->secondary_list, secondary_head) {
                if (!iter->is_output_secondary)
                    continue;

                pSecondaryScrPriv = rrGetScrPriv(iter);
                for (i = 0; i < pSecondaryScrPriv->numOutputs; i++) {
                    RROutputPtr output = pSecondaryScrPriv->outputs[i];

                    if (output->changed)
                        RRDeliverOutputEvent(client, pWin, output);
                }
            }
        }

        if (pRREvent->mask & RRProviderChangeNotifyMask) {
            xorg_list_for_each_entry(iter, &pScreen->secondary_list, secondary_head) {
                pSecondaryScrPriv = rrGetScrPriv(iter);
                if (pSecondaryScrPriv->provider->changed)
                    RRDeliverProviderEvent(client, pWin, pSecondaryScrPriv->provider);
            }
        }

        if (pRREvent->mask & RRResourceChangeNotifyMask) {
            if (pScrPriv->resourcesChanged) {
                RRDeliverResourceEvent(client, pWin);
            }
        }

        if (pRREvent->mask & RRLeaseNotifyMask) {
            if (pScrPriv->leasesChanged) {
                RRDeliverLeaseEvent(client, pWin);
            }
        }
    }
    return WT_WALKCHILDREN;
}

void
RRSetChanged(ScreenPtr pScreen)
{
    /* set changed bits on the primary screen only */
    ScreenPtr primary;
    rrScrPriv(pScreen);
    rrScrPrivPtr primarysp;

    if (pScreen->isGPU) {
        primary = pScreen->current_primary;
        if (!primary)
            return;
        primarysp = rrGetScrPriv(primary);
    }
    else {
        primary = pScreen;
        primarysp = pScrPriv;
    }

    primarysp->changed = TRUE;
}

/*
 * Something changed; send events and adjust pointer position
 */
void
RRTellChanged(ScreenPtr pScreen)
{
    ScreenPtr primary;
    rrScrPriv(pScreen);
    rrScrPrivPtr primarysp;
    int i;
    ScreenPtr iter;
    rrScrPrivPtr pSecondaryScrPriv;

    if (pScreen->isGPU) {
        primary = pScreen->current_primary;
        if (!primary)
            return;
        primarysp = rrGetScrPriv(primary);
    }
    else {
        primary = pScreen;
        primarysp = pScrPriv;
    }

    /* If there's no root window yet, can't send events */
    if (!primary->root)
        return;

    xorg_list_for_each_entry(iter, &primary->secondary_list, secondary_head) {
        pSecondaryScrPriv = rrGetScrPriv(iter);

        if (!iter->is_output_secondary)
            continue;

        if (CompareTimeStamps(primarysp->lastSetTime,
                              pSecondaryScrPriv->lastSetTime) == EARLIER) {
            primarysp->lastSetTime = pSecondaryScrPriv->lastSetTime;
        }
    }

    if (primarysp->changed) {
        UpdateCurrentTimeIf();
        if (primarysp->configChanged) {
            primarysp->lastConfigTime = currentTime;
            primarysp->configChanged = FALSE;
        }
        pScrPriv->changed = FALSE;
        primarysp->changed = FALSE;

        WalkTree(primary, TellChanged, (void *) primary);

        primarysp->resourcesChanged = FALSE;

        for (i = 0; i < pScrPriv->numOutputs; i++)
            pScrPriv->outputs[i]->changed = FALSE;
        for (i = 0; i < pScrPriv->numCrtcs; i++)
            pScrPriv->crtcs[i]->changed = FALSE;

        xorg_list_for_each_entry(iter, &primary->secondary_list, secondary_head) {
            pSecondaryScrPriv = rrGetScrPriv(iter);
            pSecondaryScrPriv->provider->changed = FALSE;
            if (iter->is_output_secondary) {
                for (i = 0; i < pSecondaryScrPriv->numOutputs; i++)
                    pSecondaryScrPriv->outputs[i]->changed = FALSE;
                for (i = 0; i < pSecondaryScrPriv->numCrtcs; i++)
                    pSecondaryScrPriv->crtcs[i]->changed = FALSE;
            }
        }

        if (primarysp->layoutChanged) {
            pScrPriv->layoutChanged = FALSE;
            RRPointerScreenConfigured(primary);
            RRSendConfigNotify(primary);
        }
    }
}

/*
 * Return the first output which is connected to an active CRTC
 * Used in emulating 1.0 behaviour
 */
RROutputPtr
RRFirstOutput(ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    RROutputPtr output;
    int i, j;

    if (!pScrPriv)
        return NULL;

    if (pScrPriv->primaryOutput && pScrPriv->primaryOutput->crtc)
        return pScrPriv->primaryOutput;

    for (i = 0; i < pScrPriv->numCrtcs; i++) {
        RRCrtcPtr crtc = pScrPriv->crtcs[i];

        for (j = 0; j < pScrPriv->numOutputs; j++) {
            output = pScrPriv->outputs[j];
            if (output->crtc == crtc)
                return output;
        }
    }
    return NULL;
}

RRCrtcPtr
RRFirstEnabledCrtc(ScreenPtr pScreen)
{
    rrScrPriv(pScreen);
    RROutputPtr output;
    int i, j;

    if (!pScrPriv)
        return NULL;

    if (pScrPriv->primaryOutput && pScrPriv->primaryOutput->crtc &&
        pScrPriv->primaryOutput->pScreen == pScreen)
        return pScrPriv->primaryOutput->crtc;

    for (i = 0; i < pScrPriv->numCrtcs; i++) {
        RRCrtcPtr crtc = pScrPriv->crtcs[i];

        for (j = 0; j < pScrPriv->numOutputs; j++) {
            output = pScrPriv->outputs[j];
            if (output->crtc == crtc && crtc->mode)
                return crtc;
        }
    }
    return NULL;
}


CARD16
RRVerticalRefresh(xRRModeInfo * mode)
{
    CARD32 refresh;
    CARD32 dots = mode->hTotal * mode->vTotal;

    if (!dots)
        return 0;
    refresh = (mode->dotClock + dots / 2) / dots;
    if (refresh > 0xffff)
        refresh = 0xffff;
    return (CARD16) refresh;
}

static int
ProcRRDispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= RRNumberRequests || !ProcRandrVector[stuff->data])
        return BadRequest;
    UpdateCurrentTimeIf();
    return (*ProcRandrVector[stuff->data]) (client);
}

static int _X_COLD
SProcRRDispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= RRNumberRequests || !SProcRandrVector[stuff->data])
        return BadRequest;
    UpdateCurrentTimeIf();
    return (*SProcRandrVector[stuff->data]) (client);
}
