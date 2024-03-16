/************************************************************

Copyright 1987, 1989, 1998  The Open Group

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

Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/

/* XSERVER_DTRACE additions:
 * Copyright (c) 2005-2006, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#include <version-config.h>
#endif

#ifdef PANORAMIX_DEBUG
#include <stdio.h>
int ProcInitialConnection();
#endif

#include "windowstr.h"
#include <X11/fonts/fontstruct.h>
#include <X11/fonts/libxfont2.h>
#include "dixfontstr.h"
#include "gcstruct.h"
#include "selection.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "opaque.h"
#include "input.h"
#include "servermd.h"
#include "extnsionst.h"
#include "dixfont.h"
#include "dispatch.h"
#include "swaprep.h"
#include "swapreq.h"
#include "privates.h"
#include "xace.h"
#include "inputstr.h"
#include "xkbsrv.h"
#include "client.h"
#include "xfixesint.h"

#ifdef XSERVER_DTRACE
#include "registry.h"
#include "probes.h"
#endif

#define mskcnt ((MAXCLIENTS + 31) / 32)
#define BITMASK(i) (1U << ((i) & 31))
#define MASKIDX(i) ((i) >> 5)
#define MASKWORD(buf, i) buf[MASKIDX(i)]
#define BITSET(buf, i) MASKWORD(buf, i) |= BITMASK(i)
#define BITCLEAR(buf, i) MASKWORD(buf, i) &= ~BITMASK(i)
#define GETBIT(buf, i) (MASKWORD(buf, i) & BITMASK(i))

xConnSetupPrefix connSetupPrefix;

PaddingInfo PixmapWidthPaddingInfo[33];

static ClientPtr grabClient;
static ClientPtr currentClient; /* Client for the request currently being dispatched */

#define GrabNone 0
#define GrabActive 1
static int grabState = GrabNone;
static long grabWaiters[mskcnt];
CallbackListPtr ServerGrabCallback = NULL;
HWEventQueuePtr checkForInput[2];
int connBlockScreenStart;

static void KillAllClients(void);

static int nextFreeClientID;    /* always MIN free client ID */

static int nClients;            /* number of authorized clients */

CallbackListPtr ClientStateCallback;
OsTimerPtr dispatchExceptionTimer;

/* dispatchException & isItTimeToYield must be declared volatile since they
 * are modified by signal handlers - otherwise optimizer may assume it doesn't
 * need to actually check value in memory when used and may miss changes from
 * signal handlers.
 */
volatile char dispatchException = 0;
volatile char isItTimeToYield;

#define SAME_SCREENS(a, b) (\
    (a.pScreen == b.pScreen))

ClientPtr
GetCurrentClient(void)
{
    if (in_input_thread()) {
        static Bool warned;

        if (!warned) {
            ErrorF("[dix] Error GetCurrentClient called from input-thread\n");
            warned = TRUE;
        }

        return NULL;
    }

    return currentClient;
}

void
SetInputCheck(HWEventQueuePtr c0, HWEventQueuePtr c1)
{
    checkForInput[0] = c0;
    checkForInput[1] = c1;
}

void
UpdateCurrentTime(void)
{
    TimeStamp systime;

    /* To avoid time running backwards, we must call GetTimeInMillis before
     * calling ProcessInputEvents.
     */
    systime.months = currentTime.months;
    systime.milliseconds = GetTimeInMillis();
    if (systime.milliseconds < currentTime.milliseconds)
        systime.months++;
    if (InputCheckPending())
        ProcessInputEvents();
    if (CompareTimeStamps(systime, currentTime) == LATER)
        currentTime = systime;
}

/* Like UpdateCurrentTime, but can't call ProcessInputEvents */
void
UpdateCurrentTimeIf(void)
{
    TimeStamp systime;

    systime.months = currentTime.months;
    systime.milliseconds = GetTimeInMillis();
    if (systime.milliseconds < currentTime.milliseconds)
        systime.months++;
    if (CompareTimeStamps(systime, currentTime) == LATER)
        currentTime = systime;
}

#undef SMART_DEBUG

/* in milliseconds */
#define SMART_SCHEDULE_DEFAULT_INTERVAL	5
#define SMART_SCHEDULE_MAX_SLICE	15

#ifdef HAVE_SETITIMER
Bool SmartScheduleSignalEnable = TRUE;
#endif

long SmartScheduleSlice = SMART_SCHEDULE_DEFAULT_INTERVAL;
long SmartScheduleInterval = SMART_SCHEDULE_DEFAULT_INTERVAL;
long SmartScheduleMaxSlice = SMART_SCHEDULE_MAX_SLICE;
long SmartScheduleTime;
int SmartScheduleLatencyLimited = 0;
static ClientPtr SmartLastClient;
static int SmartLastIndex[SMART_MAX_PRIORITY - SMART_MIN_PRIORITY + 1];

#ifdef SMART_DEBUG
long SmartLastPrint;
#endif

void Dispatch(void);

static struct xorg_list ready_clients;
static struct xorg_list saved_ready_clients;
struct xorg_list output_pending_clients;

static void
init_client_ready(void)
{
    xorg_list_init(&ready_clients);
    xorg_list_init(&saved_ready_clients);
    xorg_list_init(&output_pending_clients);
}

Bool
clients_are_ready(void)
{
    return !xorg_list_is_empty(&ready_clients);
}

/* Client has requests queued or data on the network */
void
mark_client_ready(ClientPtr client)
{
    if (xorg_list_is_empty(&client->ready))
        xorg_list_append(&client->ready, &ready_clients);
}

/*
 * Client has requests queued or data on the network, but awaits a
 * server grab release
 */
void mark_client_saved_ready(ClientPtr client)
{
    if (xorg_list_is_empty(&client->ready))
        xorg_list_append(&client->ready, &saved_ready_clients);
}

/* Client has no requests queued and no data on network */
void
mark_client_not_ready(ClientPtr client)
{
    xorg_list_del(&client->ready);
}

static void
mark_client_grab(ClientPtr grab)
{
    ClientPtr   client, tmp;

    xorg_list_for_each_entry_safe(client, tmp, &ready_clients, ready) {
        if (client != grab) {
            xorg_list_del(&client->ready);
            xorg_list_append(&client->ready, &saved_ready_clients);
        }
    }
}

static void
mark_client_ungrab(void)
{
    ClientPtr   client, tmp;

    xorg_list_for_each_entry_safe(client, tmp, &saved_ready_clients, ready) {
        xorg_list_del(&client->ready);
        xorg_list_append(&client->ready, &ready_clients);
    }
}

static ClientPtr
SmartScheduleClient(void)
{
    ClientPtr pClient, best = NULL;
    int bestRobin, robin;
    long now = SmartScheduleTime;
    long idle;
    int nready = 0;

    bestRobin = 0;
    idle = 2 * SmartScheduleSlice;

    xorg_list_for_each_entry(pClient, &ready_clients, ready) {
        nready++;

        /* Praise clients which haven't run in a while */
        if ((now - pClient->smart_stop_tick) >= idle) {
            if (pClient->smart_priority < 0)
                pClient->smart_priority++;
        }

        /* check priority to select best client */
        robin =
            (pClient->index -
             SmartLastIndex[pClient->smart_priority -
                            SMART_MIN_PRIORITY]) & 0xff;

        /* pick the best client */
        if (!best ||
            pClient->priority > best->priority ||
            (pClient->priority == best->priority &&
             (pClient->smart_priority > best->smart_priority ||
              (pClient->smart_priority == best->smart_priority && robin > bestRobin))))
        {
            best = pClient;
            bestRobin = robin;
        }
#ifdef SMART_DEBUG
        if ((now - SmartLastPrint) >= 5000)
            fprintf(stderr, " %2d: %3d", pClient->index, pClient->smart_priority);
#endif
    }
#ifdef SMART_DEBUG
    if ((now - SmartLastPrint) >= 5000) {
        fprintf(stderr, " use %2d\n", best->index);
        SmartLastPrint = now;
    }
#endif
    SmartLastIndex[best->smart_priority - SMART_MIN_PRIORITY] = best->index;
    /*
     * Set current client pointer
     */
    if (SmartLastClient != best) {
        best->smart_start_tick = now;
        SmartLastClient = best;
    }
    /*
     * Adjust slice
     */
    if (nready == 1 && SmartScheduleLatencyLimited == 0) {
        /*
         * If it's been a long time since another client
         * has run, bump the slice up to get maximal
         * performance from a single client
         */
        if ((now - best->smart_start_tick) > 1000 &&
            SmartScheduleSlice < SmartScheduleMaxSlice) {
            SmartScheduleSlice += SmartScheduleInterval;
        }
    }
    else {
        SmartScheduleSlice = SmartScheduleInterval;
    }
    return best;
}

static CARD32
DispatchExceptionCallback(OsTimerPtr timer, CARD32 time, void *arg)
{
    dispatchException |= dispatchExceptionAtReset;

    /* Don't re-arm the timer */
    return 0;
}

static void
CancelDispatchExceptionTimer(void)
{
    TimerFree(dispatchExceptionTimer);
    dispatchExceptionTimer = NULL;
}

static void
SetDispatchExceptionTimer(void)
{
    /* The timer delay is only for terminate, not reset */
    if (!(dispatchExceptionAtReset & DE_TERMINATE)) {
        dispatchException |= dispatchExceptionAtReset;
        return;
    }

    CancelDispatchExceptionTimer();

    if (terminateDelay == 0)
        dispatchException |= dispatchExceptionAtReset;
    else
        dispatchExceptionTimer = TimerSet(dispatchExceptionTimer,
                                          0, terminateDelay * 1000 /* msec */,
                                          &DispatchExceptionCallback,
                                          NULL);
}

static Bool
ShouldDisconnectRemainingClients(void)
{
    int i;

    for (i = 1; i < currentMaxClients; i++) {
        if (clients[i]) {
            if (!XFixesShouldDisconnectClient(clients[i]))
                return FALSE;
        }
    }

    /* All remaining clients can be safely ignored */
    return TRUE;
}

void
EnableLimitedSchedulingLatency(void)
{
    ++SmartScheduleLatencyLimited;
    SmartScheduleSlice = SmartScheduleInterval;
}

void
DisableLimitedSchedulingLatency(void)
{
    --SmartScheduleLatencyLimited;

    /* protect against bugs */
    if (SmartScheduleLatencyLimited < 0)
        SmartScheduleLatencyLimited = 0;
}

void
Dispatch(void)
{
    int result;
    ClientPtr client;
    long start_tick;

    nextFreeClientID = 1;
    nClients = 0;

    SmartScheduleSlice = SmartScheduleInterval;
    init_client_ready();

    while (!dispatchException) {
        if (InputCheckPending()) {
            ProcessInputEvents();
            FlushIfCriticalOutputPending();
        }

        if (!WaitForSomething(clients_are_ready()))
            continue;

       /*****************
	*  Handle events in round robin fashion, doing input between
	*  each round
	*****************/

        if (!dispatchException && clients_are_ready()) {
            client = SmartScheduleClient();

            isItTimeToYield = FALSE;

            start_tick = SmartScheduleTime;
            while (!isItTimeToYield) {
                if (InputCheckPending())
                    ProcessInputEvents();

                FlushIfCriticalOutputPending();
                if ((SmartScheduleTime - start_tick) >= SmartScheduleSlice)
                {
                    /* Penalize clients which consume ticks */
                    if (client->smart_priority > SMART_MIN_PRIORITY)
                        client->smart_priority--;
                    break;
                }

                /* now, finally, deal with client requests */
                result = ReadRequestFromClient(client);
                if (result <= 0) {
                    if (result < 0)
                        CloseDownClient(client);
                    break;
                }

                client->sequence++;
                client->majorOp = ((xReq *) client->requestBuffer)->reqType;
                client->minorOp = 0;
                if (client->majorOp >= EXTENSION_BASE) {
                    ExtensionEntry *ext = GetExtensionEntry(client->majorOp);

                    if (ext)
                        client->minorOp = ext->MinorOpcode(client);
                }
#ifdef XSERVER_DTRACE
                if (XSERVER_REQUEST_START_ENABLED())
                    XSERVER_REQUEST_START(LookupMajorName(client->majorOp),
                                          client->majorOp,
                                          ((xReq *) client->requestBuffer)->length,
                                          client->index,
                                          client->requestBuffer);
#endif
                if (result > (maxBigRequestSize << 2))
                    result = BadLength;
                else {
                    result = XaceHookDispatch(client, client->majorOp);
                    if (result == Success) {
                        currentClient = client;
                        result =
                            (*client->requestVector[client->majorOp]) (client);
                        currentClient = NULL;
                    }
                }
                if (!SmartScheduleSignalEnable)
                    SmartScheduleTime = GetTimeInMillis();

#ifdef XSERVER_DTRACE
                if (XSERVER_REQUEST_DONE_ENABLED())
                    XSERVER_REQUEST_DONE(LookupMajorName(client->majorOp),
                                         client->majorOp, client->sequence,
                                         client->index, result);
#endif

                if (client->noClientException != Success) {
                    CloseDownClient(client);
                    break;
                }
                else if (result != Success) {
                    SendErrorToClient(client, client->majorOp,
                                      client->minorOp,
                                      client->errorValue, result);
                    break;
                }
            }
            FlushAllOutput();
            if (client == SmartLastClient)
                client->smart_stop_tick = SmartScheduleTime;
        }
        dispatchException &= ~DE_PRIORITYCHANGE;
    }
#if defined(DDXBEFORERESET)
    ddxBeforeReset();
#endif
    KillAllClients();
    dispatchException &= ~DE_RESET;
    SmartScheduleLatencyLimited = 0;
    ResetOsBuffers();
}

static int VendorRelease = VENDOR_RELEASE;

void
SetVendorRelease(int release)
{
    VendorRelease = release;
}

Bool
CreateConnectionBlock(void)
{
    xConnSetup setup;
    xWindowRoot root;
    xDepth depth;
    xVisualType visual;
    xPixmapFormat format;
    unsigned long vid;
    int i, j, k, lenofblock, sizesofar = 0;
    char *pBuf;
    const char VendorString[] = VENDOR_NAME;

    memset(&setup, 0, sizeof(xConnSetup));
    /* Leave off the ridBase and ridMask, these must be sent with
       connection */

    setup.release = VendorRelease;
    /*
     * per-server image and bitmap parameters are defined in Xmd.h
     */
    setup.imageByteOrder = screenInfo.imageByteOrder;

    setup.bitmapScanlineUnit = screenInfo.bitmapScanlineUnit;
    setup.bitmapScanlinePad = screenInfo.bitmapScanlinePad;

    setup.bitmapBitOrder = screenInfo.bitmapBitOrder;
    setup.motionBufferSize = NumMotionEvents();
    setup.numRoots = screenInfo.numScreens;
    setup.nbytesVendor = strlen(VendorString);
    setup.numFormats = screenInfo.numPixmapFormats;
    setup.maxRequestSize = MAX_REQUEST_SIZE;
    QueryMinMaxKeyCodes(&setup.minKeyCode, &setup.maxKeyCode);

    lenofblock = sizeof(xConnSetup) +
        pad_to_int32(setup.nbytesVendor) +
        (setup.numFormats * sizeof(xPixmapFormat)) +
        (setup.numRoots * sizeof(xWindowRoot));
    ConnectionInfo = malloc(lenofblock);
    if (!ConnectionInfo)
        return FALSE;

    memmove(ConnectionInfo, (char *) &setup, sizeof(xConnSetup));
    sizesofar = sizeof(xConnSetup);
    pBuf = ConnectionInfo + sizeof(xConnSetup);

    memmove(pBuf, VendorString, (int) setup.nbytesVendor);
    sizesofar += setup.nbytesVendor;
    pBuf += setup.nbytesVendor;
    i = padding_for_int32(setup.nbytesVendor);
    sizesofar += i;
    while (--i >= 0)
        *pBuf++ = 0;

    memset(&format, 0, sizeof(xPixmapFormat));
    for (i = 0; i < screenInfo.numPixmapFormats; i++) {
        format.depth = screenInfo.formats[i].depth;
        format.bitsPerPixel = screenInfo.formats[i].bitsPerPixel;
        format.scanLinePad = screenInfo.formats[i].scanlinePad;
        memmove(pBuf, (char *) &format, sizeof(xPixmapFormat));
        pBuf += sizeof(xPixmapFormat);
        sizesofar += sizeof(xPixmapFormat);
    }

    connBlockScreenStart = sizesofar;
    memset(&depth, 0, sizeof(xDepth));
    memset(&visual, 0, sizeof(xVisualType));
    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen;
        DepthPtr pDepth;
        VisualPtr pVisual;

        pScreen = screenInfo.screens[i];
        root.windowId = pScreen->root->drawable.id;
        root.defaultColormap = pScreen->defColormap;
        root.whitePixel = pScreen->whitePixel;
        root.blackPixel = pScreen->blackPixel;
        root.currentInputMask = 0;      /* filled in when sent */
        root.pixWidth = pScreen->width;
        root.pixHeight = pScreen->height;
        root.mmWidth = pScreen->mmWidth;
        root.mmHeight = pScreen->mmHeight;
        root.minInstalledMaps = pScreen->minInstalledCmaps;
        root.maxInstalledMaps = pScreen->maxInstalledCmaps;
        root.rootVisualID = pScreen->rootVisual;
        root.backingStore = pScreen->backingStoreSupport;
        root.saveUnders = FALSE;
        root.rootDepth = pScreen->rootDepth;
        root.nDepths = pScreen->numDepths;
        memmove(pBuf, (char *) &root, sizeof(xWindowRoot));
        sizesofar += sizeof(xWindowRoot);
        pBuf += sizeof(xWindowRoot);

        pDepth = pScreen->allowedDepths;
        for (j = 0; j < pScreen->numDepths; j++, pDepth++) {
            lenofblock += sizeof(xDepth) +
                (pDepth->numVids * sizeof(xVisualType));
            pBuf = (char *) realloc(ConnectionInfo, lenofblock);
            if (!pBuf) {
                free(ConnectionInfo);
                return FALSE;
            }
            ConnectionInfo = pBuf;
            pBuf += sizesofar;
            depth.depth = pDepth->depth;
            depth.nVisuals = pDepth->numVids;
            memmove(pBuf, (char *) &depth, sizeof(xDepth));
            pBuf += sizeof(xDepth);
            sizesofar += sizeof(xDepth);
            for (k = 0; k < pDepth->numVids; k++) {
                vid = pDepth->vids[k];
                for (pVisual = pScreen->visuals;
                     pVisual->vid != vid; pVisual++);
                visual.visualID = vid;
                visual.class = pVisual->class;
                visual.bitsPerRGB = pVisual->bitsPerRGBValue;
                visual.colormapEntries = pVisual->ColormapEntries;
                visual.redMask = pVisual->redMask;
                visual.greenMask = pVisual->greenMask;
                visual.blueMask = pVisual->blueMask;
                memmove(pBuf, (char *) &visual, sizeof(xVisualType));
                pBuf += sizeof(xVisualType);
                sizesofar += sizeof(xVisualType);
            }
        }
    }
    connSetupPrefix.success = xTrue;
    connSetupPrefix.length = lenofblock / 4;
    connSetupPrefix.majorVersion = X_PROTOCOL;
    connSetupPrefix.minorVersion = X_PROTOCOL_REVISION;
    return TRUE;
}

int
ProcBadRequest(ClientPtr client)
{
    return BadRequest;
}

int
ProcCreateWindow(ClientPtr client)
{
    WindowPtr pParent, pWin;

    REQUEST(xCreateWindowReq);
    int len, rc;

    REQUEST_AT_LEAST_SIZE(xCreateWindowReq);

    LEGAL_NEW_RESOURCE(stuff->wid, client);
    rc = dixLookupWindow(&pParent, stuff->parent, client, DixAddAccess);
    if (rc != Success)
        return rc;
    len = client->req_len - bytes_to_int32(sizeof(xCreateWindowReq));
    if (Ones(stuff->mask) != len)
        return BadLength;
    if (!stuff->width || !stuff->height) {
        client->errorValue = 0;
        return BadValue;
    }
    pWin = CreateWindow(stuff->wid, pParent, stuff->x,
                        stuff->y, stuff->width, stuff->height,
                        stuff->borderWidth, stuff->class,
                        stuff->mask, (XID *) &stuff[1],
                        (int) stuff->depth, client, stuff->visual, &rc);
    if (pWin) {
        Mask mask = pWin->eventMask;

        pWin->eventMask = 0;    /* subterfuge in case AddResource fails */
        if (!AddResource(stuff->wid, RT_WINDOW, (void *) pWin))
            return BadAlloc;
        pWin->eventMask = mask;
    }
    return rc;
}

int
ProcChangeWindowAttributes(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xChangeWindowAttributesReq);
    int len, rc;
    Mask access_mode = 0;

    REQUEST_AT_LEAST_SIZE(xChangeWindowAttributesReq);
    access_mode |= (stuff->valueMask & CWEventMask) ? DixReceiveAccess : 0;
    access_mode |= (stuff->valueMask & ~CWEventMask) ? DixSetAttrAccess : 0;
    rc = dixLookupWindow(&pWin, stuff->window, client, access_mode);
    if (rc != Success)
        return rc;
    len = client->req_len - bytes_to_int32(sizeof(xChangeWindowAttributesReq));
    if (len != Ones(stuff->valueMask))
        return BadLength;
    return ChangeWindowAttributes(pWin,
                                  stuff->valueMask, (XID *) &stuff[1], client);
}

int
ProcGetWindowAttributes(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xResourceReq);
    xGetWindowAttributesReply wa;
    int rc;

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupWindow(&pWin, stuff->id, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    memset(&wa, 0, sizeof(xGetWindowAttributesReply));
    GetWindowAttributes(pWin, client, &wa);
    WriteReplyToClient(client, sizeof(xGetWindowAttributesReply), &wa);
    return Success;
}

int
ProcDestroyWindow(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xResourceReq);
    int rc;

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupWindow(&pWin, stuff->id, client, DixDestroyAccess);
    if (rc != Success)
        return rc;
    if (pWin->parent) {
        rc = dixLookupWindow(&pWin, pWin->parent->drawable.id, client,
                             DixRemoveAccess);
        if (rc != Success)
            return rc;
        FreeResource(stuff->id, RT_NONE);
    }
    return Success;
}

int
ProcDestroySubwindows(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xResourceReq);
    int rc;

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupWindow(&pWin, stuff->id, client, DixRemoveAccess);
    if (rc != Success)
        return rc;
    DestroySubwindows(pWin, client);
    return Success;
}

int
ProcChangeSaveSet(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xChangeSaveSetReq);
    int rc;

    REQUEST_SIZE_MATCH(xChangeSaveSetReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixManageAccess);
    if (rc != Success)
        return rc;
    if (client->clientAsMask == (CLIENT_BITS(pWin->drawable.id)))
        return BadMatch;
    if ((stuff->mode == SetModeInsert) || (stuff->mode == SetModeDelete))
        return AlterSaveSetForClient(client, pWin, stuff->mode, FALSE, TRUE);
    client->errorValue = stuff->mode;
    return BadValue;
}

int
ProcReparentWindow(ClientPtr client)
{
    WindowPtr pWin, pParent;

    REQUEST(xReparentWindowReq);
    int rc;

    REQUEST_SIZE_MATCH(xReparentWindowReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixManageAccess);
    if (rc != Success)
        return rc;
    rc = dixLookupWindow(&pParent, stuff->parent, client, DixAddAccess);
    if (rc != Success)
        return rc;
    if (!SAME_SCREENS(pWin->drawable, pParent->drawable))
        return BadMatch;
    if ((pWin->backgroundState == ParentRelative) &&
        (pParent->drawable.depth != pWin->drawable.depth))
        return BadMatch;
    if ((pWin->drawable.class != InputOnly) &&
        (pParent->drawable.class == InputOnly))
        return BadMatch;
    return ReparentWindow(pWin, pParent,
                          (short) stuff->x, (short) stuff->y, client);
}

int
ProcMapWindow(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xResourceReq);
    int rc;

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupWindow(&pWin, stuff->id, client, DixShowAccess);
    if (rc != Success)
        return rc;
    MapWindow(pWin, client);
    /* update cache to say it is mapped */
    return Success;
}

int
ProcMapSubwindows(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xResourceReq);
    int rc;

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupWindow(&pWin, stuff->id, client, DixListAccess);
    if (rc != Success)
        return rc;
    MapSubwindows(pWin, client);
    /* update cache to say it is mapped */
    return Success;
}

int
ProcUnmapWindow(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xResourceReq);
    int rc;

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupWindow(&pWin, stuff->id, client, DixHideAccess);
    if (rc != Success)
        return rc;
    UnmapWindow(pWin, FALSE);
    /* update cache to say it is mapped */
    return Success;
}

int
ProcUnmapSubwindows(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xResourceReq);
    int rc;

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupWindow(&pWin, stuff->id, client, DixListAccess);
    if (rc != Success)
        return rc;
    UnmapSubwindows(pWin);
    return Success;
}

int
ProcConfigureWindow(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xConfigureWindowReq);
    int len, rc;

    REQUEST_AT_LEAST_SIZE(xConfigureWindowReq);
    rc = dixLookupWindow(&pWin, stuff->window, client,
                         DixManageAccess | DixSetAttrAccess);
    if (rc != Success)
        return rc;
    len = client->req_len - bytes_to_int32(sizeof(xConfigureWindowReq));
    if (Ones((Mask) stuff->mask) != len)
        return BadLength;
    return ConfigureWindow(pWin, (Mask) stuff->mask, (XID *) &stuff[1], client);
}

int
ProcCirculateWindow(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xCirculateWindowReq);
    int rc;

    REQUEST_SIZE_MATCH(xCirculateWindowReq);
    if ((stuff->direction != RaiseLowest) && (stuff->direction != LowerHighest)) {
        client->errorValue = stuff->direction;
        return BadValue;
    }
    rc = dixLookupWindow(&pWin, stuff->window, client, DixManageAccess);
    if (rc != Success)
        return rc;
    CirculateWindow(pWin, (int) stuff->direction, client);
    return Success;
}

static int
GetGeometry(ClientPtr client, xGetGeometryReply * rep)
{
    DrawablePtr pDraw;
    int rc;

    REQUEST(xResourceReq);
    REQUEST_SIZE_MATCH(xResourceReq);

    rc = dixLookupDrawable(&pDraw, stuff->id, client, M_ANY, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rep->type = X_Reply;
    rep->length = 0;
    rep->sequenceNumber = client->sequence;
    rep->root = pDraw->pScreen->root->drawable.id;
    rep->depth = pDraw->depth;
    rep->width = pDraw->width;
    rep->height = pDraw->height;

    if (WindowDrawable(pDraw->type)) {
        WindowPtr pWin = (WindowPtr) pDraw;

        rep->x = pWin->origin.x - wBorderWidth(pWin);
        rep->y = pWin->origin.y - wBorderWidth(pWin);
        rep->borderWidth = pWin->borderWidth;
    }
    else {                      /* DRAWABLE_PIXMAP */

        rep->x = rep->y = rep->borderWidth = 0;
    }

    return Success;
}

int
ProcGetGeometry(ClientPtr client)
{
    xGetGeometryReply rep = { .type = X_Reply };
    int status;

    if ((status = GetGeometry(client, &rep)) != Success)
        return status;

    WriteReplyToClient(client, sizeof(xGetGeometryReply), &rep);
    return Success;
}

int
ProcQueryTree(ClientPtr client)
{
    xQueryTreeReply reply;
    int rc, numChildren = 0;
    WindowPtr pChild, pWin, pHead;
    Window *childIDs = (Window *) NULL;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupWindow(&pWin, stuff->id, client, DixListAccess);
    if (rc != Success)
        return rc;

    reply = (xQueryTreeReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .root = pWin->drawable.pScreen->root->drawable.id,
        .parent = (pWin->parent) ? pWin->parent->drawable.id : (Window) None
    };
    pHead = RealChildHead(pWin);
    for (pChild = pWin->lastChild; pChild != pHead; pChild = pChild->prevSib)
        numChildren++;
    if (numChildren) {
        int curChild = 0;

        childIDs = xallocarray(numChildren, sizeof(Window));
        if (!childIDs)
            return BadAlloc;
        for (pChild = pWin->lastChild; pChild != pHead;
             pChild = pChild->prevSib)
            childIDs[curChild++] = pChild->drawable.id;
    }

    reply.nChildren = numChildren;
    reply.length = bytes_to_int32(numChildren * sizeof(Window));

    WriteReplyToClient(client, sizeof(xQueryTreeReply), &reply);
    if (numChildren) {
        client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
        WriteSwappedDataToClient(client, numChildren * sizeof(Window),
                                 childIDs);
        free(childIDs);
    }

    return Success;
}

int
ProcInternAtom(ClientPtr client)
{
    Atom atom;
    char *tchar;

    REQUEST(xInternAtomReq);

    REQUEST_FIXED_SIZE(xInternAtomReq, stuff->nbytes);
    if ((stuff->onlyIfExists != xTrue) && (stuff->onlyIfExists != xFalse)) {
        client->errorValue = stuff->onlyIfExists;
        return BadValue;
    }
    tchar = (char *) &stuff[1];
    atom = MakeAtom(tchar, stuff->nbytes, !stuff->onlyIfExists);
    if (atom != BAD_RESOURCE) {
        xInternAtomReply reply = {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .length = 0,
            .atom = atom
        };
        WriteReplyToClient(client, sizeof(xInternAtomReply), &reply);
        return Success;
    }
    else
        return BadAlloc;
}

int
ProcGetAtomName(ClientPtr client)
{
    const char *str;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    if ((str = NameForAtom(stuff->id))) {
        int len = strlen(str);
        xGetAtomNameReply reply = {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .length = bytes_to_int32(len),
            .nameLength = len
        };

        WriteReplyToClient(client, sizeof(xGetAtomNameReply), &reply);
        WriteToClient(client, len, str);
        return Success;
    }
    else {
        client->errorValue = stuff->id;
        return BadAtom;
    }
}

int
ProcGrabServer(ClientPtr client)
{
    int rc;

    REQUEST_SIZE_MATCH(xReq);
    if (grabState != GrabNone && client != grabClient) {
        ResetCurrentRequest(client);
        client->sequence--;
        BITSET(grabWaiters, client->index);
        IgnoreClient(client);
        return Success;
    }
    rc = OnlyListenToOneClient(client);
    if (rc != Success)
        return rc;
    grabState = GrabActive;
    grabClient = client;
    mark_client_grab(client);

    if (ServerGrabCallback) {
        ServerGrabInfoRec grabinfo;

        grabinfo.client = client;
        grabinfo.grabstate = SERVER_GRABBED;
        CallCallbacks(&ServerGrabCallback, (void *) &grabinfo);
    }

    return Success;
}

static void
UngrabServer(ClientPtr client)
{
    int i;

    grabState = GrabNone;
    ListenToAllClients();
    mark_client_ungrab();
    for (i = mskcnt; --i >= 0 && !grabWaiters[i];);
    if (i >= 0) {
        i <<= 5;
        while (!GETBIT(grabWaiters, i))
            i++;
        BITCLEAR(grabWaiters, i);
        AttendClient(clients[i]);
    }

    if (ServerGrabCallback) {
        ServerGrabInfoRec grabinfo;

        grabinfo.client = client;
        grabinfo.grabstate = SERVER_UNGRABBED;
        CallCallbacks(&ServerGrabCallback, (void *) &grabinfo);
    }
}

int
ProcUngrabServer(ClientPtr client)
{
    REQUEST_SIZE_MATCH(xReq);
    UngrabServer(client);
    return Success;
}

int
ProcTranslateCoords(ClientPtr client)
{
    REQUEST(xTranslateCoordsReq);

    WindowPtr pWin, pDst;
    xTranslateCoordsReply rep;
    int rc;

    REQUEST_SIZE_MATCH(xTranslateCoordsReq);
    rc = dixLookupWindow(&pWin, stuff->srcWid, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    rc = dixLookupWindow(&pDst, stuff->dstWid, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rep = (xTranslateCoordsReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    if (!SAME_SCREENS(pWin->drawable, pDst->drawable)) {
        rep.sameScreen = xFalse;
        rep.child = None;
        rep.dstX = rep.dstY = 0;
    }
    else {
        INT16 x, y;

        rep.sameScreen = xTrue;
        rep.child = None;
        /* computing absolute coordinates -- adjust to destination later */
        x = pWin->drawable.x + stuff->srcX;
        y = pWin->drawable.y + stuff->srcY;
        pWin = pDst->firstChild;
        while (pWin) {
            BoxRec box;

            if ((pWin->mapped) &&
                (x >= pWin->drawable.x - wBorderWidth(pWin)) &&
                (x < pWin->drawable.x + (int) pWin->drawable.width +
                 wBorderWidth(pWin)) &&
                (y >= pWin->drawable.y - wBorderWidth(pWin)) &&
                (y < pWin->drawable.y + (int) pWin->drawable.height +
                 wBorderWidth(pWin))
                /* When a window is shaped, a further check
                 * is made to see if the point is inside
                 * borderSize
                 */
                && (!wBoundingShape(pWin) ||
                    RegionContainsPoint(&pWin->borderSize, x, y, &box))

                && (!wInputShape(pWin) ||
                    RegionContainsPoint(wInputShape(pWin),
                                        x - pWin->drawable.x,
                                        y - pWin->drawable.y, &box))
                ) {
                rep.child = pWin->drawable.id;
                pWin = (WindowPtr) NULL;
            }
            else
                pWin = pWin->nextSib;
        }
        /* adjust to destination coordinates */
        rep.dstX = x - pDst->drawable.x;
        rep.dstY = y - pDst->drawable.y;
    }
    WriteReplyToClient(client, sizeof(xTranslateCoordsReply), &rep);
    return Success;
}

int
ProcOpenFont(ClientPtr client)
{
    int err;

    REQUEST(xOpenFontReq);

    REQUEST_FIXED_SIZE(xOpenFontReq, stuff->nbytes);
    client->errorValue = stuff->fid;
    LEGAL_NEW_RESOURCE(stuff->fid, client);
    err = OpenFont(client, stuff->fid, (Mask) 0,
                   stuff->nbytes, (char *) &stuff[1]);
    if (err == Success) {
        return Success;
    }
    else
        return err;
}

int
ProcCloseFont(ClientPtr client)
{
    FontPtr pFont;
    int rc;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupResourceByType((void **) &pFont, stuff->id, RT_FONT,
                                 client, DixDestroyAccess);
    if (rc == Success) {
        FreeResource(stuff->id, RT_NONE);
        return Success;
    }
    else {
        client->errorValue = stuff->id;
        return rc;
    }
}

int
ProcQueryFont(ClientPtr client)
{
    xQueryFontReply *reply;
    FontPtr pFont;
    int rc;

    REQUEST(xResourceReq);
    REQUEST_SIZE_MATCH(xResourceReq);

    rc = dixLookupFontable(&pFont, stuff->id, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    {
        xCharInfo *pmax = FONTINKMAX(pFont);
        xCharInfo *pmin = FONTINKMIN(pFont);
        int nprotoxcistructs;
        int rlength;

        nprotoxcistructs = (pmax->rightSideBearing == pmin->rightSideBearing &&
                            pmax->leftSideBearing == pmin->leftSideBearing &&
                            pmax->descent == pmin->descent &&
                            pmax->ascent == pmin->ascent &&
                            pmax->characterWidth == pmin->characterWidth) ?
            0 : N2dChars(pFont);

        rlength = sizeof(xQueryFontReply) +
            FONTINFONPROPS(FONTCHARSET(pFont)) * sizeof(xFontProp) +
            nprotoxcistructs * sizeof(xCharInfo);
        reply = calloc(1, rlength);
        if (!reply) {
            return BadAlloc;
        }

        reply->type = X_Reply;
        reply->length = bytes_to_int32(rlength - sizeof(xGenericReply));
        reply->sequenceNumber = client->sequence;
        QueryFont(pFont, reply, nprotoxcistructs);

        WriteReplyToClient(client, rlength, reply);
        free(reply);
        return Success;
    }
}

int
ProcQueryTextExtents(ClientPtr client)
{
    xQueryTextExtentsReply reply;
    FontPtr pFont;
    ExtentInfoRec info;
    unsigned long length;
    int rc;

    REQUEST(xQueryTextExtentsReq);
    REQUEST_AT_LEAST_SIZE(xQueryTextExtentsReq);

    rc = dixLookupFontable(&pFont, stuff->fid, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    length = client->req_len - bytes_to_int32(sizeof(xQueryTextExtentsReq));
    length = length << 1;
    if (stuff->oddLength) {
        if (length == 0)
            return BadLength;
        length--;
    }
    if (!xfont2_query_text_extents(pFont, length, (unsigned char *) &stuff[1], &info))
        return BadAlloc;
    reply = (xQueryTextExtentsReply) {
        .type = X_Reply,
        .drawDirection = info.drawDirection,
        .sequenceNumber = client->sequence,
        .length = 0,
        .fontAscent = info.fontAscent,
        .fontDescent = info.fontDescent,
        .overallAscent = info.overallAscent,
        .overallDescent = info.overallDescent,
        .overallWidth = info.overallWidth,
        .overallLeft = info.overallLeft,
        .overallRight = info.overallRight
    };
    WriteReplyToClient(client, sizeof(xQueryTextExtentsReply), &reply);
    return Success;
}

int
ProcListFonts(ClientPtr client)
{
    REQUEST(xListFontsReq);

    REQUEST_FIXED_SIZE(xListFontsReq, stuff->nbytes);

    return ListFonts(client, (unsigned char *) &stuff[1], stuff->nbytes,
                     stuff->maxNames);
}

int
ProcListFontsWithInfo(ClientPtr client)
{
    REQUEST(xListFontsWithInfoReq);

    REQUEST_FIXED_SIZE(xListFontsWithInfoReq, stuff->nbytes);

    return StartListFontsWithInfo(client, stuff->nbytes,
                                  (unsigned char *) &stuff[1], stuff->maxNames);
}

/**
 *
 *  \param value must conform to DeleteType
 */
int
dixDestroyPixmap(void *value, XID pid)
{
    PixmapPtr pPixmap = (PixmapPtr) value;

    return (*pPixmap->drawable.pScreen->DestroyPixmap) (pPixmap);
}

int
ProcCreatePixmap(ClientPtr client)
{
    PixmapPtr pMap;
    DrawablePtr pDraw;

    REQUEST(xCreatePixmapReq);
    DepthPtr pDepth;
    int i, rc;

    REQUEST_SIZE_MATCH(xCreatePixmapReq);
    client->errorValue = stuff->pid;
    LEGAL_NEW_RESOURCE(stuff->pid, client);

    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, M_ANY,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;

    if (!stuff->width || !stuff->height) {
        client->errorValue = 0;
        return BadValue;
    }
    if (stuff->width > 32767 || stuff->height > 32767) {
        /* It is allowed to try and allocate a pixmap which is larger than
         * 32767 in either dimension. However, all of the framebuffer code
         * is buggy and does not reliably draw to such big pixmaps, basically
         * because the Region data structure operates with signed shorts
         * for the rectangles in it.
         *
         * Furthermore, several places in the X server computes the
         * size in bytes of the pixmap and tries to store it in an
         * integer. This integer can overflow and cause the allocated size
         * to be much smaller.
         *
         * So, such big pixmaps are rejected here with a BadAlloc
         */
        return BadAlloc;
    }
    if (stuff->depth != 1) {
        pDepth = pDraw->pScreen->allowedDepths;
        for (i = 0; i < pDraw->pScreen->numDepths; i++, pDepth++)
            if (pDepth->depth == stuff->depth)
                goto CreatePmap;
        client->errorValue = stuff->depth;
        return BadValue;
    }
 CreatePmap:
    pMap = (PixmapPtr) (*pDraw->pScreen->CreatePixmap)
        (pDraw->pScreen, stuff->width, stuff->height, stuff->depth, 0);
    if (pMap) {
        pMap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
        pMap->drawable.id = stuff->pid;
        /* security creation/labeling check */
        rc = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pid, RT_PIXMAP,
                      pMap, RT_NONE, NULL, DixCreateAccess);
        if (rc != Success) {
            (*pDraw->pScreen->DestroyPixmap) (pMap);
            return rc;
        }
        if (AddResource(stuff->pid, RT_PIXMAP, (void *) pMap))
            return Success;
    }
    return BadAlloc;
}

int
ProcFreePixmap(ClientPtr client)
{
    PixmapPtr pMap;
    int rc;

    REQUEST(xResourceReq);
    REQUEST_SIZE_MATCH(xResourceReq);

    rc = dixLookupResourceByType((void **) &pMap, stuff->id, RT_PIXMAP,
                                 client, DixDestroyAccess);
    if (rc == Success) {
        FreeResource(stuff->id, RT_NONE);
        return Success;
    }
    else {
        client->errorValue = stuff->id;
        return rc;
    }
}

int
ProcCreateGC(ClientPtr client)
{
    int error, rc;
    GC *pGC;
    DrawablePtr pDraw;
    unsigned len;

    REQUEST(xCreateGCReq);

    REQUEST_AT_LEAST_SIZE(xCreateGCReq);
    client->errorValue = stuff->gc;
    LEGAL_NEW_RESOURCE(stuff->gc, client);
    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;

    len = client->req_len - bytes_to_int32(sizeof(xCreateGCReq));
    if (len != Ones(stuff->mask))
        return BadLength;
    pGC = (GC *) CreateGC(pDraw, stuff->mask, (XID *) &stuff[1], &error,
                          stuff->gc, client);
    if (error != Success)
        return error;
    if (!AddResource(stuff->gc, RT_GC, (void *) pGC))
        return BadAlloc;
    return Success;
}

int
ProcChangeGC(ClientPtr client)
{
    GC *pGC;
    int result;
    unsigned len;

    REQUEST(xChangeGCReq);
    REQUEST_AT_LEAST_SIZE(xChangeGCReq);

    result = dixLookupGC(&pGC, stuff->gc, client, DixSetAttrAccess);
    if (result != Success)
        return result;

    len = client->req_len - bytes_to_int32(sizeof(xChangeGCReq));
    if (len != Ones(stuff->mask))
        return BadLength;

    return ChangeGCXIDs(client, pGC, stuff->mask, (CARD32 *) &stuff[1]);
}

int
ProcCopyGC(ClientPtr client)
{
    GC *dstGC;
    GC *pGC;
    int result;

    REQUEST(xCopyGCReq);
    REQUEST_SIZE_MATCH(xCopyGCReq);

    result = dixLookupGC(&pGC, stuff->srcGC, client, DixGetAttrAccess);
    if (result != Success)
        return result;
    result = dixLookupGC(&dstGC, stuff->dstGC, client, DixSetAttrAccess);
    if (result != Success)
        return result;
    if ((dstGC->pScreen != pGC->pScreen) || (dstGC->depth != pGC->depth))
        return BadMatch;
    if (stuff->mask & ~GCAllBits) {
        client->errorValue = stuff->mask;
        return BadValue;
    }
    return CopyGC(pGC, dstGC, stuff->mask);
}

int
ProcSetDashes(ClientPtr client)
{
    GC *pGC;
    int result;

    REQUEST(xSetDashesReq);

    REQUEST_FIXED_SIZE(xSetDashesReq, stuff->nDashes);
    if (stuff->nDashes == 0) {
        client->errorValue = 0;
        return BadValue;
    }

    result = dixLookupGC(&pGC, stuff->gc, client, DixSetAttrAccess);
    if (result != Success)
        return result;

    /* If there's an error, either there's no sensible errorValue,
     * or there was a dash segment of 0. */
    client->errorValue = 0;
    return SetDashes(pGC, stuff->dashOffset, stuff->nDashes,
                     (unsigned char *) &stuff[1]);
}

int
ProcSetClipRectangles(ClientPtr client)
{
    int nr, result;
    GC *pGC;

    REQUEST(xSetClipRectanglesReq);

    REQUEST_AT_LEAST_SIZE(xSetClipRectanglesReq);
    if ((stuff->ordering != Unsorted) && (stuff->ordering != YSorted) &&
        (stuff->ordering != YXSorted) && (stuff->ordering != YXBanded)) {
        client->errorValue = stuff->ordering;
        return BadValue;
    }
    result = dixLookupGC(&pGC, stuff->gc, client, DixSetAttrAccess);
    if (result != Success)
        return result;

    nr = (client->req_len << 2) - sizeof(xSetClipRectanglesReq);
    if (nr & 4)
        return BadLength;
    nr >>= 3;
    return SetClipRects(pGC, stuff->xOrigin, stuff->yOrigin,
                        nr, (xRectangle *) &stuff[1], (int) stuff->ordering);
}

int
ProcFreeGC(ClientPtr client)
{
    GC *pGC;
    int rc;

    REQUEST(xResourceReq);
    REQUEST_SIZE_MATCH(xResourceReq);

    rc = dixLookupGC(&pGC, stuff->id, client, DixDestroyAccess);
    if (rc != Success)
        return rc;

    FreeResource(stuff->id, RT_NONE);
    return Success;
}

int
ProcClearToBackground(ClientPtr client)
{
    REQUEST(xClearAreaReq);
    WindowPtr pWin;
    int rc;

    REQUEST_SIZE_MATCH(xClearAreaReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixWriteAccess);
    if (rc != Success)
        return rc;
    if (pWin->drawable.class == InputOnly) {
        client->errorValue = stuff->window;
        return BadMatch;
    }
    if ((stuff->exposures != xTrue) && (stuff->exposures != xFalse)) {
        client->errorValue = stuff->exposures;
        return BadValue;
    }
    (*pWin->drawable.pScreen->ClearToBackground) (pWin, stuff->x, stuff->y,
                                                  stuff->width, stuff->height,
                                                  (Bool) stuff->exposures);
    return Success;
}

/* send GraphicsExpose events, or a NoExpose event, based on the region */
void
SendGraphicsExpose(ClientPtr client, RegionPtr pRgn, XID drawable,
                     int major, int minor)
{
    if (pRgn && !RegionNil(pRgn)) {
        xEvent *pEvent;
        xEvent *pe;
        BoxPtr pBox;
        int i;
        int numRects;

        numRects = RegionNumRects(pRgn);
        pBox = RegionRects(pRgn);
        if (!(pEvent = calloc(numRects, sizeof(xEvent))))
            return;
        pe = pEvent;

        for (i = 1; i <= numRects; i++, pe++, pBox++) {
            pe->u.u.type = GraphicsExpose;
            pe->u.graphicsExposure.drawable = drawable;
            pe->u.graphicsExposure.x = pBox->x1;
            pe->u.graphicsExposure.y = pBox->y1;
            pe->u.graphicsExposure.width = pBox->x2 - pBox->x1;
            pe->u.graphicsExposure.height = pBox->y2 - pBox->y1;
            pe->u.graphicsExposure.count = numRects - i;
            pe->u.graphicsExposure.majorEvent = major;
            pe->u.graphicsExposure.minorEvent = minor;
        }
        /* GraphicsExpose is a "critical event", which TryClientEvents
         * handles specially. */
        TryClientEvents(client, NULL, pEvent, numRects,
                        (Mask) 0, NoEventMask, NullGrab);
        free(pEvent);
    }
    else {
        xEvent event = {
            .u.noExposure.drawable = drawable,
            .u.noExposure.majorEvent = major,
            .u.noExposure.minorEvent = minor
        };
        event.u.u.type = NoExpose;
        WriteEventsToClient(client, 1, &event);
    }
}

int
ProcCopyArea(ClientPtr client)
{
    DrawablePtr pDst;
    DrawablePtr pSrc;
    GC *pGC;

    REQUEST(xCopyAreaReq);
    RegionPtr pRgn;
    int rc;

    REQUEST_SIZE_MATCH(xCopyAreaReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->dstDrawable, pDst, DixWriteAccess);
    if (stuff->dstDrawable != stuff->srcDrawable) {
        rc = dixLookupDrawable(&pSrc, stuff->srcDrawable, client, 0,
                               DixReadAccess);
        if (rc != Success)
            return rc;
        if ((pDst->pScreen != pSrc->pScreen) || (pDst->depth != pSrc->depth)) {
            client->errorValue = stuff->dstDrawable;
            return BadMatch;
        }
    }
    else
        pSrc = pDst;

    pRgn = (*pGC->ops->CopyArea) (pSrc, pDst, pGC, stuff->srcX, stuff->srcY,
                                  stuff->width, stuff->height,
                                  stuff->dstX, stuff->dstY);
    if (pGC->graphicsExposures) {
        SendGraphicsExpose(client, pRgn, stuff->dstDrawable, X_CopyArea, 0);
        if (pRgn)
            RegionDestroy(pRgn);
    }

    return Success;
}

int
ProcCopyPlane(ClientPtr client)
{
    DrawablePtr psrcDraw, pdstDraw;
    GC *pGC;

    REQUEST(xCopyPlaneReq);
    RegionPtr pRgn;
    int rc;

    REQUEST_SIZE_MATCH(xCopyPlaneReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->dstDrawable, pdstDraw, DixWriteAccess);
    if (stuff->dstDrawable != stuff->srcDrawable) {
        rc = dixLookupDrawable(&psrcDraw, stuff->srcDrawable, client, 0,
                               DixReadAccess);
        if (rc != Success)
            return rc;

        if (pdstDraw->pScreen != psrcDraw->pScreen) {
            client->errorValue = stuff->dstDrawable;
            return BadMatch;
        }
    }
    else
        psrcDraw = pdstDraw;

    /* Check to see if stuff->bitPlane has exactly ONE good bit set */
    if (stuff->bitPlane == 0 || (stuff->bitPlane & (stuff->bitPlane - 1)) ||
        (stuff->bitPlane > (1L << (psrcDraw->depth - 1)))) {
        client->errorValue = stuff->bitPlane;
        return BadValue;
    }

    pRgn =
        (*pGC->ops->CopyPlane) (psrcDraw, pdstDraw, pGC, stuff->srcX,
                                stuff->srcY, stuff->width, stuff->height,
                                stuff->dstX, stuff->dstY, stuff->bitPlane);
    if (pGC->graphicsExposures) {
        SendGraphicsExpose(client, pRgn, stuff->dstDrawable, X_CopyPlane, 0);
        if (pRgn)
            RegionDestroy(pRgn);
    }
    return Success;
}

int
ProcPolyPoint(ClientPtr client)
{
    int npoint;
    GC *pGC;
    DrawablePtr pDraw;

    REQUEST(xPolyPointReq);

    REQUEST_AT_LEAST_SIZE(xPolyPointReq);
    if ((stuff->coordMode != CoordModeOrigin) &&
        (stuff->coordMode != CoordModePrevious)) {
        client->errorValue = stuff->coordMode;
        return BadValue;
    }
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    npoint = bytes_to_int32((client->req_len << 2) - sizeof(xPolyPointReq));
    if (npoint)
        (*pGC->ops->PolyPoint) (pDraw, pGC, stuff->coordMode, npoint,
                                (xPoint *) &stuff[1]);
    return Success;
}

int
ProcPolyLine(ClientPtr client)
{
    int npoint;
    GC *pGC;
    DrawablePtr pDraw;

    REQUEST(xPolyLineReq);

    REQUEST_AT_LEAST_SIZE(xPolyLineReq);
    if ((stuff->coordMode != CoordModeOrigin) &&
        (stuff->coordMode != CoordModePrevious)) {
        client->errorValue = stuff->coordMode;
        return BadValue;
    }
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    npoint = bytes_to_int32((client->req_len << 2) - sizeof(xPolyLineReq));
    if (npoint > 1)
        (*pGC->ops->Polylines) (pDraw, pGC, stuff->coordMode, npoint,
                                (DDXPointPtr) &stuff[1]);
    return Success;
}

int
ProcPolySegment(ClientPtr client)
{
    int nsegs;
    GC *pGC;
    DrawablePtr pDraw;

    REQUEST(xPolySegmentReq);

    REQUEST_AT_LEAST_SIZE(xPolySegmentReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    nsegs = (client->req_len << 2) - sizeof(xPolySegmentReq);
    if (nsegs & 4)
        return BadLength;
    nsegs >>= 3;
    if (nsegs)
        (*pGC->ops->PolySegment) (pDraw, pGC, nsegs, (xSegment *) &stuff[1]);
    return Success;
}

int
ProcPolyRectangle(ClientPtr client)
{
    int nrects;
    GC *pGC;
    DrawablePtr pDraw;

    REQUEST(xPolyRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyRectangleReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    nrects = (client->req_len << 2) - sizeof(xPolyRectangleReq);
    if (nrects & 4)
        return BadLength;
    nrects >>= 3;
    if (nrects)
        (*pGC->ops->PolyRectangle) (pDraw, pGC,
                                    nrects, (xRectangle *) &stuff[1]);
    return Success;
}

int
ProcPolyArc(ClientPtr client)
{
    int narcs;
    GC *pGC;
    DrawablePtr pDraw;

    REQUEST(xPolyArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyArcReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    narcs = (client->req_len << 2) - sizeof(xPolyArcReq);
    if (narcs % sizeof(xArc))
        return BadLength;
    narcs /= sizeof(xArc);
    if (narcs)
        (*pGC->ops->PolyArc) (pDraw, pGC, narcs, (xArc *) &stuff[1]);
    return Success;
}

int
ProcFillPoly(ClientPtr client)
{
    int things;
    GC *pGC;
    DrawablePtr pDraw;

    REQUEST(xFillPolyReq);

    REQUEST_AT_LEAST_SIZE(xFillPolyReq);
    if ((stuff->shape != Complex) && (stuff->shape != Nonconvex) &&
        (stuff->shape != Convex)) {
        client->errorValue = stuff->shape;
        return BadValue;
    }
    if ((stuff->coordMode != CoordModeOrigin) &&
        (stuff->coordMode != CoordModePrevious)) {
        client->errorValue = stuff->coordMode;
        return BadValue;
    }

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    things = bytes_to_int32((client->req_len << 2) - sizeof(xFillPolyReq));
    if (things)
        (*pGC->ops->FillPolygon) (pDraw, pGC, stuff->shape,
                                  stuff->coordMode, things,
                                  (DDXPointPtr) &stuff[1]);
    return Success;
}

int
ProcPolyFillRectangle(ClientPtr client)
{
    int things;
    GC *pGC;
    DrawablePtr pDraw;

    REQUEST(xPolyFillRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillRectangleReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    things = (client->req_len << 2) - sizeof(xPolyFillRectangleReq);
    if (things & 4)
        return BadLength;
    things >>= 3;

    if (things)
        (*pGC->ops->PolyFillRect) (pDraw, pGC, things,
                                   (xRectangle *) &stuff[1]);
    return Success;
}

int
ProcPolyFillArc(ClientPtr client)
{
    int narcs;
    GC *pGC;
    DrawablePtr pDraw;

    REQUEST(xPolyFillArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillArcReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    narcs = (client->req_len << 2) - sizeof(xPolyFillArcReq);
    if (narcs % sizeof(xArc))
        return BadLength;
    narcs /= sizeof(xArc);
    if (narcs)
        (*pGC->ops->PolyFillArc) (pDraw, pGC, narcs, (xArc *) &stuff[1]);
    return Success;
}

#ifdef MATCH_CLIENT_ENDIAN

int
ServerOrder(void)
{
    int whichbyte = 1;

    if (*((char *) &whichbyte))
        return LSBFirst;
    return MSBFirst;
}

#define ClientOrder(client) ((client)->swapped ? !ServerOrder() : ServerOrder())

void
ReformatImage(char *base, int nbytes, int bpp, int order)
{
    switch (bpp) {
    case 1:                    /* yuck */
        if (BITMAP_BIT_ORDER != order)
            BitOrderInvert((unsigned char *) base, nbytes);
#if IMAGE_BYTE_ORDER != BITMAP_BIT_ORDER && BITMAP_SCANLINE_UNIT != 8
        ReformatImage(base, nbytes, BITMAP_SCANLINE_UNIT, order);
#endif
        break;
    case 4:
        break;                  /* yuck */
    case 8:
        break;
    case 16:
        if (IMAGE_BYTE_ORDER != order)
            TwoByteSwap((unsigned char *) base, nbytes);
        break;
    case 32:
        if (IMAGE_BYTE_ORDER != order)
            FourByteSwap((unsigned char *) base, nbytes);
        break;
    }
}
#else
#define ReformatImage(b,n,bpp,o)
#endif

/* 64-bit server notes: the protocol restricts padding of images to
 * 8-, 16-, or 32-bits. We would like to have 64-bits for the server
 * to use internally. Removes need for internal alignment checking.
 * All of the PutImage functions could be changed individually, but
 * as currently written, they call other routines which require things
 * to be 64-bit padded on scanlines, so we changed things here.
 * If an image would be padded differently for 64- versus 32-, then
 * copy each scanline to a 64-bit padded scanline.
 * Also, we need to make sure that the image is aligned on a 64-bit
 * boundary, even if the scanlines are padded to our satisfaction.
 */
int
ProcPutImage(ClientPtr client)
{
    GC *pGC;
    DrawablePtr pDraw;
    long length;                /* length of scanline server padded */
    long lengthProto;           /* length of scanline protocol padded */
    char *tmpImage;

    REQUEST(xPutImageReq);

    REQUEST_AT_LEAST_SIZE(xPutImageReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    if (stuff->format == XYBitmap) {
        if ((stuff->depth != 1) ||
            (stuff->leftPad >= (unsigned int) screenInfo.bitmapScanlinePad))
            return BadMatch;
        length = BitmapBytePad(stuff->width + stuff->leftPad);
    }
    else if (stuff->format == XYPixmap) {
        if ((pDraw->depth != stuff->depth) ||
            (stuff->leftPad >= (unsigned int) screenInfo.bitmapScanlinePad))
            return BadMatch;
        length = BitmapBytePad(stuff->width + stuff->leftPad);
        length *= stuff->depth;
    }
    else if (stuff->format == ZPixmap) {
        if ((pDraw->depth != stuff->depth) || (stuff->leftPad != 0))
            return BadMatch;
        length = PixmapBytePad(stuff->width, stuff->depth);
    }
    else {
        client->errorValue = stuff->format;
        return BadValue;
    }

    tmpImage = (char *) &stuff[1];
    lengthProto = length;

    if (stuff->height != 0 && lengthProto >= (INT32_MAX / stuff->height))
        return BadLength;

    if ((bytes_to_int32(lengthProto * stuff->height) +
         bytes_to_int32(sizeof(xPutImageReq))) != client->req_len)
        return BadLength;

    ReformatImage(tmpImage, lengthProto * stuff->height,
                  stuff->format == ZPixmap ? BitsPerPixel(stuff->depth) : 1,
                  ClientOrder(client));

    (*pGC->ops->PutImage) (pDraw, pGC, stuff->depth, stuff->dstX, stuff->dstY,
                           stuff->width, stuff->height,
                           stuff->leftPad, stuff->format, tmpImage);

    return Success;
}

static int
DoGetImage(ClientPtr client, int format, Drawable drawable,
           int x, int y, int width, int height,
           Mask planemask)
{
    DrawablePtr pDraw, pBoundingDraw;
    int nlines, linesPerBuf, rc;
    int linesDone;

    /* coordinates relative to the bounding drawable */
    int relx, rely;
    long widthBytesLine, length;
    Mask plane = 0;
    char *pBuf;
    xGetImageReply xgi;
    RegionPtr pVisibleRegion = NULL;

    if ((format != XYPixmap) && (format != ZPixmap)) {
        client->errorValue = format;
        return BadValue;
    }
    rc = dixLookupDrawable(&pDraw, drawable, client, 0, DixReadAccess);
    if (rc != Success)
        return rc;

    memset(&xgi, 0, sizeof(xGetImageReply));

    relx = x;
    rely = y;

    if (pDraw->type == DRAWABLE_WINDOW) {
        WindowPtr pWin = (WindowPtr) pDraw;

        /* "If the drawable is a window, the window must be viewable ... or a
         * BadMatch error results" */
        if (!pWin->viewable)
            return BadMatch;

        /* If the drawable is a window, the rectangle must be contained within
         * its bounds (including the border). */
        if (x < -wBorderWidth(pWin) ||
            x + width > wBorderWidth(pWin) + (int) pDraw->width ||
            y < -wBorderWidth(pWin) ||
            y + height > wBorderWidth(pWin) + (int) pDraw->height)
            return BadMatch;

        relx += pDraw->x;
        rely += pDraw->y;

        if (pDraw->pScreen->GetWindowPixmap) {
            PixmapPtr pPix = (*pDraw->pScreen->GetWindowPixmap) (pWin);

            pBoundingDraw = &pPix->drawable;
#ifdef COMPOSITE
            relx -= pPix->screen_x;
            rely -= pPix->screen_y;
#endif
        }
        else {
            pBoundingDraw = (DrawablePtr) pDraw->pScreen->root;
        }

        xgi.visual = wVisual(pWin);
    }
    else {
        pBoundingDraw = pDraw;
        xgi.visual = None;
    }

    /* "If the drawable is a pixmap, the given rectangle must be wholly
     *  contained within the pixmap, or a BadMatch error results.  If the
     *  drawable is a window [...] it must be the case that if there were no
     *  inferiors or overlapping windows, the specified rectangle of the window
     *  would be fully visible on the screen and wholly contained within the
     *  outside edges of the window, or a BadMatch error results."
     *
     * We relax the window case slightly to mean that the rectangle must exist
     * within the bounds of the window's backing pixmap.  In particular, this
     * means that a GetImage request may succeed or fail with BadMatch depending
     * on whether any of its ancestor windows are redirected.  */
    if (relx < 0 || relx + width > (int) pBoundingDraw->width ||
        rely < 0 || rely + height > (int) pBoundingDraw->height)
        return BadMatch;

    xgi.type = X_Reply;
    xgi.sequenceNumber = client->sequence;
    xgi.depth = pDraw->depth;
    if (format == ZPixmap) {
        widthBytesLine = PixmapBytePad(width, pDraw->depth);
        length = widthBytesLine * height;

    }
    else {
        widthBytesLine = BitmapBytePad(width);
        plane = ((Mask) 1) << (pDraw->depth - 1);
        /* only planes asked for */
        length = widthBytesLine * height *
            Ones(planemask & (plane | (plane - 1)));

    }

    xgi.length = length;

    xgi.length = bytes_to_int32(xgi.length);
    if (widthBytesLine == 0 || height == 0)
        linesPerBuf = 0;
    else if (widthBytesLine >= IMAGE_BUFSIZE)
        linesPerBuf = 1;
    else {
        linesPerBuf = IMAGE_BUFSIZE / widthBytesLine;
        if (linesPerBuf > height)
            linesPerBuf = height;
    }
    length = linesPerBuf * widthBytesLine;
    if (linesPerBuf < height) {
        /* we have to make sure intermediate buffers don't need padding */
        while ((linesPerBuf > 1) &&
               (length & ((1L << LOG2_BYTES_PER_SCANLINE_PAD) - 1))) {
            linesPerBuf--;
            length -= widthBytesLine;
        }
        while (length & ((1L << LOG2_BYTES_PER_SCANLINE_PAD) - 1)) {
            linesPerBuf++;
            length += widthBytesLine;
        }
    }
    if (!(pBuf = calloc(1, length)))
        return BadAlloc;
    WriteReplyToClient(client, sizeof(xGetImageReply), &xgi);

    if (pDraw->type == DRAWABLE_WINDOW) {
        pVisibleRegion = &((WindowPtr) pDraw)->borderClip;
        pDraw->pScreen->SourceValidate(pDraw, x, y, width, height,
                                       IncludeInferiors);
    }

    if (linesPerBuf == 0) {
        /* nothing to do */
    }
    else if (format == ZPixmap) {
        linesDone = 0;
        while (height - linesDone > 0) {
            nlines = min(linesPerBuf, height - linesDone);
            (*pDraw->pScreen->GetImage) (pDraw,
                                         x,
                                         y + linesDone,
                                         width,
                                         nlines,
                                         format, planemask, (void *) pBuf);
            if (pVisibleRegion)
                XaceCensorImage(client, pVisibleRegion, widthBytesLine,
                                pDraw, x, y + linesDone, width,
                                nlines, format, pBuf);

            /* Note that this is NOT a call to WriteSwappedDataToClient,
               as we do NOT byte swap */
            ReformatImage(pBuf, (int) (nlines * widthBytesLine),
                          BitsPerPixel(pDraw->depth), ClientOrder(client));

            WriteToClient(client, (int) (nlines * widthBytesLine), pBuf);
            linesDone += nlines;
        }
    }
    else {                      /* XYPixmap */

        for (; plane; plane >>= 1) {
            if (planemask & plane) {
                linesDone = 0;
                while (height - linesDone > 0) {
                    nlines = min(linesPerBuf, height - linesDone);
                    (*pDraw->pScreen->GetImage) (pDraw,
                                                 x,
                                                 y + linesDone,
                                                 width,
                                                 nlines,
                                                 format, plane, (void *) pBuf);
                    if (pVisibleRegion)
                        XaceCensorImage(client, pVisibleRegion,
                                        widthBytesLine,
                                        pDraw, x, y + linesDone, width,
                                        nlines, format, pBuf);

                    /* Note: NOT a call to WriteSwappedDataToClient,
                       as we do NOT byte swap */
                    ReformatImage(pBuf, (int) (nlines * widthBytesLine),
                                  1, ClientOrder(client));

                    WriteToClient(client, (int)(nlines * widthBytesLine), pBuf);
                    linesDone += nlines;
                }
            }
        }
    }
    free(pBuf);
    return Success;
}

int
ProcGetImage(ClientPtr client)
{
    REQUEST(xGetImageReq);

    REQUEST_SIZE_MATCH(xGetImageReq);

    return DoGetImage(client, stuff->format, stuff->drawable,
                      stuff->x, stuff->y,
                      (int) stuff->width, (int) stuff->height,
                      stuff->planeMask);
}

int
ProcPolyText(ClientPtr client)
{
    int err;

    REQUEST(xPolyTextReq);
    DrawablePtr pDraw;
    GC *pGC;

    REQUEST_AT_LEAST_SIZE(xPolyTextReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);

    err = PolyText(client,
                   pDraw,
                   pGC,
                   (unsigned char *) &stuff[1],
                   ((unsigned char *) stuff) + (client->req_len << 2),
                   stuff->x, stuff->y, stuff->reqType, stuff->drawable);

    if (err == Success) {
        return Success;
    }
    else
        return err;
}

int
ProcImageText8(ClientPtr client)
{
    int err;
    DrawablePtr pDraw;
    GC *pGC;

    REQUEST(xImageTextReq);

    REQUEST_FIXED_SIZE(xImageTextReq, stuff->nChars);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);

    err = ImageText(client,
                    pDraw,
                    pGC,
                    stuff->nChars,
                    (unsigned char *) &stuff[1],
                    stuff->x, stuff->y, stuff->reqType, stuff->drawable);

    if (err == Success) {
        return Success;
    }
    else
        return err;
}

int
ProcImageText16(ClientPtr client)
{
    int err;
    DrawablePtr pDraw;
    GC *pGC;

    REQUEST(xImageTextReq);

    REQUEST_FIXED_SIZE(xImageTextReq, stuff->nChars << 1);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);

    err = ImageText(client,
                    pDraw,
                    pGC,
                    stuff->nChars,
                    (unsigned char *) &stuff[1],
                    stuff->x, stuff->y, stuff->reqType, stuff->drawable);

    if (err == Success) {
        return Success;
    }
    else
        return err;
}

int
ProcCreateColormap(ClientPtr client)
{
    VisualPtr pVisual;
    ColormapPtr pmap;
    Colormap mid;
    WindowPtr pWin;
    ScreenPtr pScreen;

    REQUEST(xCreateColormapReq);
    int i, result;

    REQUEST_SIZE_MATCH(xCreateColormapReq);

    if ((stuff->alloc != AllocNone) && (stuff->alloc != AllocAll)) {
        client->errorValue = stuff->alloc;
        return BadValue;
    }
    mid = stuff->mid;
    LEGAL_NEW_RESOURCE(mid, client);
    result = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (result != Success)
        return result;

    pScreen = pWin->drawable.pScreen;
    for (i = 0, pVisual = pScreen->visuals;
         i < pScreen->numVisuals; i++, pVisual++) {
        if (pVisual->vid != stuff->visual)
            continue;
        return CreateColormap(mid, pScreen, pVisual, &pmap,
                              (int) stuff->alloc, client->index);
    }
    client->errorValue = stuff->visual;
    return BadMatch;
}

int
ProcFreeColormap(ClientPtr client)
{
    ColormapPtr pmap;
    int rc;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupResourceByType((void **) &pmap, stuff->id, RT_COLORMAP,
                                 client, DixDestroyAccess);
    if (rc == Success) {
        /* Freeing a default colormap is a no-op */
        if (!(pmap->flags & IsDefault))
            FreeResource(stuff->id, RT_NONE);
        return Success;
    }
    else {
        client->errorValue = stuff->id;
        return rc;
    }
}

int
ProcCopyColormapAndFree(ClientPtr client)
{
    Colormap mid;
    ColormapPtr pSrcMap;

    REQUEST(xCopyColormapAndFreeReq);
    int rc;

    REQUEST_SIZE_MATCH(xCopyColormapAndFreeReq);
    mid = stuff->mid;
    LEGAL_NEW_RESOURCE(mid, client);
    rc = dixLookupResourceByType((void **) &pSrcMap, stuff->srcCmap,
                                 RT_COLORMAP, client,
                                 DixReadAccess | DixRemoveAccess);
    if (rc == Success)
        return CopyColormapAndFree(mid, pSrcMap, client->index);
    client->errorValue = stuff->srcCmap;
    return rc;
}

int
ProcInstallColormap(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xResourceReq);
    REQUEST_SIZE_MATCH(xResourceReq);

    rc = dixLookupResourceByType((void **) &pcmp, stuff->id, RT_COLORMAP,
                                 client, DixInstallAccess);
    if (rc != Success)
        goto out;

    rc = XaceHook(XACE_SCREEN_ACCESS, client, pcmp->pScreen, DixSetAttrAccess);
    if (rc != Success) {
        if (rc == BadValue)
            rc = BadColor;
        goto out;
    }

    (*(pcmp->pScreen->InstallColormap)) (pcmp);
    return Success;

 out:
    client->errorValue = stuff->id;
    return rc;
}

int
ProcUninstallColormap(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xResourceReq);
    REQUEST_SIZE_MATCH(xResourceReq);

    rc = dixLookupResourceByType((void **) &pcmp, stuff->id, RT_COLORMAP,
                                 client, DixUninstallAccess);
    if (rc != Success)
        goto out;

    rc = XaceHook(XACE_SCREEN_ACCESS, client, pcmp->pScreen, DixSetAttrAccess);
    if (rc != Success) {
        if (rc == BadValue)
            rc = BadColor;
        goto out;
    }

    if (pcmp->mid != pcmp->pScreen->defColormap)
        (*(pcmp->pScreen->UninstallColormap)) (pcmp);
    return Success;

 out:
    client->errorValue = stuff->id;
    return rc;
}

int
ProcListInstalledColormaps(ClientPtr client)
{
    xListInstalledColormapsReply *preply;
    int nummaps, rc;
    WindowPtr pWin;

    REQUEST(xResourceReq);
    REQUEST_SIZE_MATCH(xResourceReq);

    rc = dixLookupWindow(&pWin, stuff->id, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    rc = XaceHook(XACE_SCREEN_ACCESS, client, pWin->drawable.pScreen,
                  DixGetAttrAccess);
    if (rc != Success)
        return rc;

    preply = malloc(sizeof(xListInstalledColormapsReply) +
                    pWin->drawable.pScreen->maxInstalledCmaps *
                    sizeof(Colormap));
    if (!preply)
        return BadAlloc;

    preply->type = X_Reply;
    preply->sequenceNumber = client->sequence;
    nummaps = (*pWin->drawable.pScreen->ListInstalledColormaps)
        (pWin->drawable.pScreen, (Colormap *) &preply[1]);
    preply->nColormaps = nummaps;
    preply->length = nummaps;
    WriteReplyToClient(client, sizeof(xListInstalledColormapsReply), preply);
    client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
    WriteSwappedDataToClient(client, nummaps * sizeof(Colormap), &preply[1]);
    free(preply);
    return Success;
}

int
ProcAllocColor(ClientPtr client)
{
    ColormapPtr pmap;
    int rc;

    REQUEST(xAllocColorReq);

    REQUEST_SIZE_MATCH(xAllocColorReq);
    rc = dixLookupResourceByType((void **) &pmap, stuff->cmap, RT_COLORMAP,
                                 client, DixAddAccess);
    if (rc == Success) {
        xAllocColorReply acr = {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .length = 0,
            .red = stuff->red,
            .green = stuff->green,
            .blue = stuff->blue,
            .pixel = 0
        };
        if ((rc = AllocColor(pmap, &acr.red, &acr.green, &acr.blue,
                             &acr.pixel, client->index)))
            return rc;
#ifdef PANORAMIX
        if (noPanoramiXExtension || !pmap->pScreen->myNum)
#endif
            WriteReplyToClient(client, sizeof(xAllocColorReply), &acr);
        return Success;

    }
    else {
        client->errorValue = stuff->cmap;
        return rc;
    }
}

int
ProcAllocNamedColor(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xAllocNamedColorReq);

    REQUEST_FIXED_SIZE(xAllocNamedColorReq, stuff->nbytes);
    rc = dixLookupResourceByType((void **) &pcmp, stuff->cmap, RT_COLORMAP,
                                 client, DixAddAccess);
    if (rc == Success) {
        xAllocNamedColorReply ancr = {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .length = 0
        };
        if (OsLookupColor
            (pcmp->pScreen->myNum, (char *) &stuff[1], stuff->nbytes,
             &ancr.exactRed, &ancr.exactGreen, &ancr.exactBlue)) {
            ancr.screenRed = ancr.exactRed;
            ancr.screenGreen = ancr.exactGreen;
            ancr.screenBlue = ancr.exactBlue;
            ancr.pixel = 0;
            if ((rc = AllocColor(pcmp,
                                 &ancr.screenRed, &ancr.screenGreen,
                                 &ancr.screenBlue, &ancr.pixel, client->index)))
                return rc;
#ifdef PANORAMIX
            if (noPanoramiXExtension || !pcmp->pScreen->myNum)
#endif
                WriteReplyToClient(client, sizeof(xAllocNamedColorReply),
                                   &ancr);
            return Success;
        }
        else
            return BadName;

    }
    else {
        client->errorValue = stuff->cmap;
        return rc;
    }
}

int
ProcAllocColorCells(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xAllocColorCellsReq);

    REQUEST_SIZE_MATCH(xAllocColorCellsReq);
    rc = dixLookupResourceByType((void **) &pcmp, stuff->cmap, RT_COLORMAP,
                                 client, DixAddAccess);
    if (rc == Success) {
        int npixels, nmasks;
        long length;
        Pixel *ppixels, *pmasks;

        npixels = stuff->colors;
        if (!npixels) {
            client->errorValue = npixels;
            return BadValue;
        }
        if (stuff->contiguous != xTrue && stuff->contiguous != xFalse) {
            client->errorValue = stuff->contiguous;
            return BadValue;
        }
        nmasks = stuff->planes;
        length = ((long) npixels + (long) nmasks) * sizeof(Pixel);
        ppixels = malloc(length);
        if (!ppixels)
            return BadAlloc;
        pmasks = ppixels + npixels;

        if ((rc = AllocColorCells(client->index, pcmp, npixels, nmasks,
                                  (Bool) stuff->contiguous, ppixels, pmasks))) {
            free(ppixels);
            return rc;
        }
#ifdef PANORAMIX
        if (noPanoramiXExtension || !pcmp->pScreen->myNum)
#endif
        {
            xAllocColorCellsReply accr = {
                .type = X_Reply,
                .sequenceNumber = client->sequence,
                .length = bytes_to_int32(length),
                .nPixels = npixels,
                .nMasks = nmasks
            };
            WriteReplyToClient(client, sizeof(xAllocColorCellsReply), &accr);
            client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
            WriteSwappedDataToClient(client, length, ppixels);
        }
        free(ppixels);
        return Success;
    }
    else {
        client->errorValue = stuff->cmap;
        return rc;
    }
}

int
ProcAllocColorPlanes(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xAllocColorPlanesReq);

    REQUEST_SIZE_MATCH(xAllocColorPlanesReq);
    rc = dixLookupResourceByType((void **) &pcmp, stuff->cmap, RT_COLORMAP,
                                 client, DixAddAccess);
    if (rc == Success) {
        xAllocColorPlanesReply acpr;
        int npixels;
        long length;
        Pixel *ppixels;

        npixels = stuff->colors;
        if (!npixels) {
            client->errorValue = npixels;
            return BadValue;
        }
        if (stuff->contiguous != xTrue && stuff->contiguous != xFalse) {
            client->errorValue = stuff->contiguous;
            return BadValue;
        }
        acpr = (xAllocColorPlanesReply) {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .nPixels = npixels
        };
        length = (long) npixels *sizeof(Pixel);

        ppixels = malloc(length);
        if (!ppixels)
            return BadAlloc;
        if ((rc = AllocColorPlanes(client->index, pcmp, npixels,
                                   (int) stuff->red, (int) stuff->green,
                                   (int) stuff->blue, (Bool) stuff->contiguous,
                                   ppixels, &acpr.redMask, &acpr.greenMask,
                                   &acpr.blueMask))) {
            free(ppixels);
            return rc;
        }
        acpr.length = bytes_to_int32(length);
#ifdef PANORAMIX
        if (noPanoramiXExtension || !pcmp->pScreen->myNum)
#endif
        {
            WriteReplyToClient(client, sizeof(xAllocColorPlanesReply), &acpr);
            client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
            WriteSwappedDataToClient(client, length, ppixels);
        }
        free(ppixels);
        return Success;
    }
    else {
        client->errorValue = stuff->cmap;
        return rc;
    }
}

int
ProcFreeColors(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xFreeColorsReq);

    REQUEST_AT_LEAST_SIZE(xFreeColorsReq);
    rc = dixLookupResourceByType((void **) &pcmp, stuff->cmap, RT_COLORMAP,
                                 client, DixRemoveAccess);
    if (rc == Success) {
        int count;

        if (pcmp->flags & AllAllocated)
            return BadAccess;
        count = bytes_to_int32((client->req_len << 2) - sizeof(xFreeColorsReq));
        return FreeColors(pcmp, client->index, count,
                          (Pixel *) &stuff[1], (Pixel) stuff->planeMask);
    }
    else {
        client->errorValue = stuff->cmap;
        return rc;
    }
}

int
ProcStoreColors(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xStoreColorsReq);

    REQUEST_AT_LEAST_SIZE(xStoreColorsReq);
    rc = dixLookupResourceByType((void **) &pcmp, stuff->cmap, RT_COLORMAP,
                                 client, DixWriteAccess);
    if (rc == Success) {
        int count;

        count = (client->req_len << 2) - sizeof(xStoreColorsReq);
        if (count % sizeof(xColorItem))
            return BadLength;
        count /= sizeof(xColorItem);
        return StoreColors(pcmp, count, (xColorItem *) &stuff[1], client);
    }
    else {
        client->errorValue = stuff->cmap;
        return rc;
    }
}

int
ProcStoreNamedColor(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xStoreNamedColorReq);

    REQUEST_FIXED_SIZE(xStoreNamedColorReq, stuff->nbytes);
    rc = dixLookupResourceByType((void **) &pcmp, stuff->cmap, RT_COLORMAP,
                                 client, DixWriteAccess);
    if (rc == Success) {
        xColorItem def;

        if (OsLookupColor(pcmp->pScreen->myNum, (char *) &stuff[1],
                          stuff->nbytes, &def.red, &def.green, &def.blue)) {
            def.flags = stuff->flags;
            def.pixel = stuff->pixel;
            return StoreColors(pcmp, 1, &def, client);
        }
        return BadName;
    }
    else {
        client->errorValue = stuff->cmap;
        return rc;
    }
}

int
ProcQueryColors(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xQueryColorsReq);

    REQUEST_AT_LEAST_SIZE(xQueryColorsReq);
    rc = dixLookupResourceByType((void **) &pcmp, stuff->cmap, RT_COLORMAP,
                                 client, DixReadAccess);
    if (rc == Success) {
        int count;
        xrgb *prgbs;
        xQueryColorsReply qcr;

        count =
            bytes_to_int32((client->req_len << 2) - sizeof(xQueryColorsReq));
        prgbs = calloc(count, sizeof(xrgb));
        if (!prgbs && count)
            return BadAlloc;
        if ((rc =
             QueryColors(pcmp, count, (Pixel *) &stuff[1], prgbs, client))) {
            free(prgbs);
            return rc;
        }
        qcr = (xQueryColorsReply) {
            .type = X_Reply,
            .sequenceNumber = client->sequence,
            .length = bytes_to_int32(count * sizeof(xrgb)),
            .nColors = count
        };
        WriteReplyToClient(client, sizeof(xQueryColorsReply), &qcr);
        if (count) {
            client->pSwapReplyFunc = (ReplySwapPtr) SQColorsExtend;
            WriteSwappedDataToClient(client, count * sizeof(xrgb), prgbs);
        }
        free(prgbs);
        return Success;

    }
    else {
        client->errorValue = stuff->cmap;
        return rc;
    }
}

int
ProcLookupColor(ClientPtr client)
{
    ColormapPtr pcmp;
    int rc;

    REQUEST(xLookupColorReq);

    REQUEST_FIXED_SIZE(xLookupColorReq, stuff->nbytes);
    rc = dixLookupResourceByType((void **) &pcmp, stuff->cmap, RT_COLORMAP,
                                 client, DixReadAccess);
    if (rc == Success) {
        CARD16 exactRed, exactGreen, exactBlue;

        if (OsLookupColor
            (pcmp->pScreen->myNum, (char *) &stuff[1], stuff->nbytes,
             &exactRed, &exactGreen, &exactBlue)) {
            xLookupColorReply lcr = {
                .type = X_Reply,
                .sequenceNumber = client->sequence,
                .length = 0,
                .exactRed = exactRed,
                .exactGreen = exactGreen,
                .exactBlue = exactBlue,
                .screenRed = exactRed,
                .screenGreen = exactGreen,
                .screenBlue = exactBlue
            };
            (*pcmp->pScreen->ResolveColor) (&lcr.screenRed,
                                            &lcr.screenGreen,
                                            &lcr.screenBlue, pcmp->pVisual);
            WriteReplyToClient(client, sizeof(xLookupColorReply), &lcr);
            return Success;
        }
        return BadName;
    }
    else {
        client->errorValue = stuff->cmap;
        return rc;
    }
}

int
ProcCreateCursor(ClientPtr client)
{
    CursorPtr pCursor;
    PixmapPtr src;
    PixmapPtr msk;
    unsigned char *srcbits;
    unsigned char *mskbits;
    unsigned short width, height;
    long n;
    CursorMetricRec cm;
    int rc;

    REQUEST(xCreateCursorReq);

    REQUEST_SIZE_MATCH(xCreateCursorReq);
    LEGAL_NEW_RESOURCE(stuff->cid, client);

    rc = dixLookupResourceByType((void **) &src, stuff->source, RT_PIXMAP,
                                 client, DixReadAccess);
    if (rc != Success) {
        client->errorValue = stuff->source;
        return rc;
    }

    if (src->drawable.depth != 1)
        return (BadMatch);

    /* Find and validate cursor mask pixmap, if one is provided */
    if (stuff->mask != None) {
        rc = dixLookupResourceByType((void **) &msk, stuff->mask, RT_PIXMAP,
                                     client, DixReadAccess);
        if (rc != Success) {
            client->errorValue = stuff->mask;
            return rc;
        }

        if (src->drawable.width != msk->drawable.width
            || src->drawable.height != msk->drawable.height
            || src->drawable.depth != 1 || msk->drawable.depth != 1)
            return BadMatch;
    }
    else
        msk = NULL;

    width = src->drawable.width;
    height = src->drawable.height;

    if (stuff->x > width || stuff->y > height)
        return BadMatch;

    srcbits = calloc(BitmapBytePad(width), height);
    if (!srcbits)
        return BadAlloc;
    n = BitmapBytePad(width) * height;
    mskbits = malloc(n);
    if (!mskbits) {
        free(srcbits);
        return BadAlloc;
    }

    (*src->drawable.pScreen->GetImage) ((DrawablePtr) src, 0, 0, width, height,
                                        XYPixmap, 1, (void *) srcbits);
    if (msk == (PixmapPtr) NULL) {
        unsigned char *bits = mskbits;

        while (--n >= 0)
            *bits++ = ~0;
    }
    else {
        /* zeroing the (pad) bits helps some ddx cursor handling */
        memset((char *) mskbits, 0, n);
        (*msk->drawable.pScreen->GetImage) ((DrawablePtr) msk, 0, 0, width,
                                            height, XYPixmap, 1,
                                            (void *) mskbits);
    }
    cm.width = width;
    cm.height = height;
    cm.xhot = stuff->x;
    cm.yhot = stuff->y;
    rc = AllocARGBCursor(srcbits, mskbits, NULL, &cm,
                         stuff->foreRed, stuff->foreGreen, stuff->foreBlue,
                         stuff->backRed, stuff->backGreen, stuff->backBlue,
                         &pCursor, client, stuff->cid);

    if (rc != Success)
        goto bail;
    if (!AddResource(stuff->cid, RT_CURSOR, (void *) pCursor)) {
        rc = BadAlloc;
        goto bail;
    }

    return Success;
 bail:
    free(srcbits);
    free(mskbits);
    return rc;
}

int
ProcCreateGlyphCursor(ClientPtr client)
{
    CursorPtr pCursor;
    int res;

    REQUEST(xCreateGlyphCursorReq);

    REQUEST_SIZE_MATCH(xCreateGlyphCursorReq);
    LEGAL_NEW_RESOURCE(stuff->cid, client);

    res = AllocGlyphCursor(stuff->source, stuff->sourceChar,
                           stuff->mask, stuff->maskChar,
                           stuff->foreRed, stuff->foreGreen, stuff->foreBlue,
                           stuff->backRed, stuff->backGreen, stuff->backBlue,
                           &pCursor, client, stuff->cid);
    if (res != Success)
        return res;
    if (AddResource(stuff->cid, RT_CURSOR, (void *) pCursor))
        return Success;
    return BadAlloc;
}

int
ProcFreeCursor(ClientPtr client)
{
    CursorPtr pCursor;
    int rc;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupResourceByType((void **) &pCursor, stuff->id, RT_CURSOR,
                                 client, DixDestroyAccess);
    if (rc == Success) {
        FreeResource(stuff->id, RT_NONE);
        return Success;
    }
    else {
        client->errorValue = stuff->id;
        return rc;
    }
}

int
ProcQueryBestSize(ClientPtr client)
{
    xQueryBestSizeReply reply;
    DrawablePtr pDraw;
    ScreenPtr pScreen;
    int rc;

    REQUEST(xQueryBestSizeReq);
    REQUEST_SIZE_MATCH(xQueryBestSizeReq);

    if ((stuff->class != CursorShape) &&
        (stuff->class != TileShape) && (stuff->class != StippleShape)) {
        client->errorValue = stuff->class;
        return BadValue;
    }

    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, M_ANY,
                           DixGetAttrAccess);
    if (rc != Success)
        return rc;
    if (stuff->class != CursorShape && pDraw->type == UNDRAWABLE_WINDOW)
        return BadMatch;
    pScreen = pDraw->pScreen;
    rc = XaceHook(XACE_SCREEN_ACCESS, client, pScreen, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    (*pScreen->QueryBestSize) (stuff->class, &stuff->width,
                               &stuff->height, pScreen);
    reply = (xQueryBestSizeReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .width = stuff->width,
        .height = stuff->height
    };
    WriteReplyToClient(client, sizeof(xQueryBestSizeReply), &reply);
    return Success;
}

int
ProcSetScreenSaver(ClientPtr client)
{
    int rc, i, blankingOption, exposureOption;

    REQUEST(xSetScreenSaverReq);
    REQUEST_SIZE_MATCH(xSetScreenSaverReq);

    for (i = 0; i < screenInfo.numScreens; i++) {
        rc = XaceHook(XACE_SCREENSAVER_ACCESS, client, screenInfo.screens[i],
                      DixSetAttrAccess);
        if (rc != Success)
            return rc;
    }

    blankingOption = stuff->preferBlank;
    if ((blankingOption != DontPreferBlanking) &&
        (blankingOption != PreferBlanking) &&
        (blankingOption != DefaultBlanking)) {
        client->errorValue = blankingOption;
        return BadValue;
    }
    exposureOption = stuff->allowExpose;
    if ((exposureOption != DontAllowExposures) &&
        (exposureOption != AllowExposures) &&
        (exposureOption != DefaultExposures)) {
        client->errorValue = exposureOption;
        return BadValue;
    }
    if (stuff->timeout < -1) {
        client->errorValue = stuff->timeout;
        return BadValue;
    }
    if (stuff->interval < -1) {
        client->errorValue = stuff->interval;
        return BadValue;
    }

    if (blankingOption == DefaultBlanking)
        ScreenSaverBlanking = defaultScreenSaverBlanking;
    else
        ScreenSaverBlanking = blankingOption;
    if (exposureOption == DefaultExposures)
        ScreenSaverAllowExposures = defaultScreenSaverAllowExposures;
    else
        ScreenSaverAllowExposures = exposureOption;

    if (stuff->timeout >= 0)
        ScreenSaverTime = stuff->timeout * MILLI_PER_SECOND;
    else
        ScreenSaverTime = defaultScreenSaverTime;
    if (stuff->interval >= 0)
        ScreenSaverInterval = stuff->interval * MILLI_PER_SECOND;
    else
        ScreenSaverInterval = defaultScreenSaverInterval;

    SetScreenSaverTimer();
    return Success;
}

int
ProcGetScreenSaver(ClientPtr client)
{
    xGetScreenSaverReply rep;
    int rc, i;

    REQUEST_SIZE_MATCH(xReq);

    for (i = 0; i < screenInfo.numScreens; i++) {
        rc = XaceHook(XACE_SCREENSAVER_ACCESS, client, screenInfo.screens[i],
                      DixGetAttrAccess);
        if (rc != Success)
            return rc;
    }

    rep = (xGetScreenSaverReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .timeout = ScreenSaverTime / MILLI_PER_SECOND,
        .interval = ScreenSaverInterval / MILLI_PER_SECOND,
        .preferBlanking = ScreenSaverBlanking,
        .allowExposures = ScreenSaverAllowExposures
    };
    WriteReplyToClient(client, sizeof(xGetScreenSaverReply), &rep);
    return Success;
}

int
ProcChangeHosts(ClientPtr client)
{
    REQUEST(xChangeHostsReq);

    REQUEST_FIXED_SIZE(xChangeHostsReq, stuff->hostLength);

    if (stuff->mode == HostInsert)
        return AddHost(client, (int) stuff->hostFamily,
                       stuff->hostLength, (void *) &stuff[1]);
    if (stuff->mode == HostDelete)
        return RemoveHost(client, (int) stuff->hostFamily,
                          stuff->hostLength, (void *) &stuff[1]);
    client->errorValue = stuff->mode;
    return BadValue;
}

int
ProcListHosts(ClientPtr client)
{
    xListHostsReply reply;
    int len, nHosts, result;
    BOOL enabled;
    void *pdata;

    /* REQUEST(xListHostsReq); */

    REQUEST_SIZE_MATCH(xListHostsReq);

    /* untrusted clients can't list hosts */
    result = XaceHook(XACE_SERVER_ACCESS, client, DixReadAccess);
    if (result != Success)
        return result;

    result = GetHosts(&pdata, &nHosts, &len, &enabled);
    if (result != Success)
        return result;

    reply = (xListHostsReply) {
        .type = X_Reply,
        .enabled = enabled,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(len),
        .nHosts = nHosts
    };
    WriteReplyToClient(client, sizeof(xListHostsReply), &reply);
    if (nHosts) {
        client->pSwapReplyFunc = (ReplySwapPtr) SLHostsExtend;
        WriteSwappedDataToClient(client, len, pdata);
    }
    free(pdata);
    return Success;
}

int
ProcChangeAccessControl(ClientPtr client)
{
    REQUEST(xSetAccessControlReq);

    REQUEST_SIZE_MATCH(xSetAccessControlReq);
    if ((stuff->mode != EnableAccess) && (stuff->mode != DisableAccess)) {
        client->errorValue = stuff->mode;
        return BadValue;
    }
    return ChangeAccessControl(client, stuff->mode == EnableAccess);
}

/*********************
 * CloseDownRetainedResources
 *
 *    Find all clients that are gone and have terminated in RetainTemporary
 *    and destroy their resources.
 *********************/

static void
CloseDownRetainedResources(void)
{
    int i;
    ClientPtr client;

    for (i = 1; i < currentMaxClients; i++) {
        client = clients[i];
        if (client && (client->closeDownMode == RetainTemporary)
            && (client->clientGone))
            CloseDownClient(client);
    }
}

int
ProcKillClient(ClientPtr client)
{
    REQUEST(xResourceReq);
    ClientPtr killclient;
    int rc;

    REQUEST_SIZE_MATCH(xResourceReq);
    if (stuff->id == AllTemporary) {
        CloseDownRetainedResources();
        return Success;
    }

    rc = dixLookupClient(&killclient, stuff->id, client, DixDestroyAccess);
    if (rc == Success) {
        CloseDownClient(killclient);
        if (client == killclient) {
            /* force yield and return Success, so that Dispatch()
             * doesn't try to touch client
             */
            isItTimeToYield = TRUE;
        }
        return Success;
    }
    else
        return rc;
}

int
ProcSetFontPath(ClientPtr client)
{
    unsigned char *ptr;
    unsigned long nbytes, total;
    long nfonts;
    int n;

    REQUEST(xSetFontPathReq);

    REQUEST_AT_LEAST_SIZE(xSetFontPathReq);

    nbytes = (client->req_len << 2) - sizeof(xSetFontPathReq);
    total = nbytes;
    ptr = (unsigned char *) &stuff[1];
    nfonts = stuff->nFonts;
    while (--nfonts >= 0) {
        if ((total == 0) || (total < (n = (*ptr + 1))))
            return BadLength;
        total -= n;
        ptr += n;
    }
    if (total >= 4)
        return BadLength;
    return SetFontPath(client, stuff->nFonts, (unsigned char *) &stuff[1]);
}

int
ProcGetFontPath(ClientPtr client)
{
    xGetFontPathReply reply;
    int rc, stringLens, numpaths;
    unsigned char *bufferStart;

    /* REQUEST (xReq); */

    REQUEST_SIZE_MATCH(xReq);
    rc = GetFontPath(client, &numpaths, &stringLens, &bufferStart);
    if (rc != Success)
        return rc;

    reply = (xGetFontPathReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(stringLens + numpaths),
        .nPaths = numpaths
    };

    WriteReplyToClient(client, sizeof(xGetFontPathReply), &reply);
    if (stringLens || numpaths)
        WriteToClient(client, stringLens + numpaths, bufferStart);
    return Success;
}

int
ProcChangeCloseDownMode(ClientPtr client)
{
    int rc;

    REQUEST(xSetCloseDownModeReq);
    REQUEST_SIZE_MATCH(xSetCloseDownModeReq);

    rc = XaceHook(XACE_CLIENT_ACCESS, client, client, DixManageAccess);
    if (rc != Success)
        return rc;

    if ((stuff->mode == AllTemporary) ||
        (stuff->mode == RetainPermanent) || (stuff->mode == RetainTemporary)) {
        client->closeDownMode = stuff->mode;
        return Success;
    }
    else {
        client->errorValue = stuff->mode;
        return BadValue;
    }
}

int
ProcForceScreenSaver(ClientPtr client)
{
    int rc;

    REQUEST(xForceScreenSaverReq);

    REQUEST_SIZE_MATCH(xForceScreenSaverReq);

    if ((stuff->mode != ScreenSaverReset) && (stuff->mode != ScreenSaverActive)) {
        client->errorValue = stuff->mode;
        return BadValue;
    }
    rc = dixSaveScreens(client, SCREEN_SAVER_FORCER, (int) stuff->mode);
    if (rc != Success)
        return rc;
    return Success;
}

int
ProcNoOperation(ClientPtr client)
{
    REQUEST_AT_LEAST_SIZE(xReq);

    /* noop -- don't do anything */
    return Success;
}

/**********************
 * CloseDownClient
 *
 *  Client can either mark his resources destroy or retain.  If retained and
 *  then killed again, the client is really destroyed.
 *********************/

char dispatchExceptionAtReset = DE_RESET;
int terminateDelay = 0;

void
CloseDownClient(ClientPtr client)
{
    Bool really_close_down = client->clientGone ||
        client->closeDownMode == DestroyAll;

    if (!client->clientGone) {
        /* ungrab server if grabbing client dies */
        if (grabState != GrabNone && grabClient == client) {
            UngrabServer(client);
        }
        BITCLEAR(grabWaiters, client->index);
        DeleteClientFromAnySelections(client);
        ReleaseActiveGrabs(client);
        DeleteClientFontStuff(client);
        if (!really_close_down) {
            /*  This frees resources that should never be retained
             *  no matter what the close down mode is.  Actually we
             *  could do this unconditionally, but it's probably
             *  better not to traverse all the client's resources
             *  twice (once here, once a few lines down in
             *  FreeClientResources) in the common case of
             *  really_close_down == TRUE.
             */
            FreeClientNeverRetainResources(client);
            client->clientState = ClientStateRetained;
            if (ClientStateCallback) {
                NewClientInfoRec clientinfo;

                clientinfo.client = client;
                clientinfo.prefix = (xConnSetupPrefix *) NULL;
                clientinfo.setup = (xConnSetup *) NULL;
                CallCallbacks((&ClientStateCallback), (void *) &clientinfo);
            }
        }
        client->clientGone = TRUE;      /* so events aren't sent to client */
        if (ClientIsAsleep(client))
            ClientSignal(client);
        ProcessWorkQueueZombies();
        CloseDownConnection(client);
        output_pending_clear(client);
        mark_client_not_ready(client);

        /* If the client made it to the Running stage, nClients has
         * been incremented on its behalf, so we need to decrement it
         * now.  If it hasn't gotten to Running, nClients has *not*
         * been incremented, so *don't* decrement it.
         */
        if (client->clientState != ClientStateInitial) {
            --nClients;
        }
    }

    if (really_close_down) {
        if (client->clientState == ClientStateRunning && nClients == 0)
            SetDispatchExceptionTimer();

        client->clientState = ClientStateGone;
        if (ClientStateCallback) {
            NewClientInfoRec clientinfo;

            clientinfo.client = client;
            clientinfo.prefix = (xConnSetupPrefix *) NULL;
            clientinfo.setup = (xConnSetup *) NULL;
            CallCallbacks((&ClientStateCallback), (void *) &clientinfo);
        }
        TouchListenerGone(client->clientAsMask);
        GestureListenerGone(client->clientAsMask);
        FreeClientResources(client);
        /* Disable client ID tracking. This must be done after
         * ClientStateCallback. */
        ReleaseClientIds(client);
#ifdef XSERVER_DTRACE
        XSERVER_CLIENT_DISCONNECT(client->index);
#endif
        if (client->index < nextFreeClientID)
            nextFreeClientID = client->index;
        clients[client->index] = NullClient;
        SmartLastClient = NullClient;
        dixFreeObjectWithPrivates(client, PRIVATE_CLIENT);

        while (!clients[currentMaxClients - 1])
            currentMaxClients--;
    }

    if (ShouldDisconnectRemainingClients())
        SetDispatchExceptionTimer();
}

static void
KillAllClients(void)
{
    int i;

    for (i = 1; i < currentMaxClients; i++)
        if (clients[i]) {
            /* Make sure Retained clients are released. */
            clients[i]->closeDownMode = DestroyAll;
            CloseDownClient(clients[i]);
        }
}

void
InitClient(ClientPtr client, int i, void *ospriv)
{
    client->index = i;
    xorg_list_init(&client->ready);
    xorg_list_init(&client->output_pending);
    client->clientAsMask = ((Mask) i) << CLIENTOFFSET;
    client->closeDownMode = i ? DestroyAll : RetainPermanent;
    client->requestVector = InitialVector;
    client->osPrivate = ospriv;
    QueryMinMaxKeyCodes(&client->minKC, &client->maxKC);
    client->smart_start_tick = SmartScheduleTime;
    client->smart_stop_tick = SmartScheduleTime;
    client->clientIds = NULL;
}

/************************
 * int NextAvailableClient(ospriv)
 *
 * OS dependent portion can't assign client id's because of CloseDownModes.
 * Returns NULL if there are no free clients.
 *************************/

ClientPtr
NextAvailableClient(void *ospriv)
{
    int i;
    ClientPtr client;
    xReq data;

    i = nextFreeClientID;
    if (i == LimitClients)
        return (ClientPtr) NULL;
    clients[i] = client =
        dixAllocateObjectWithPrivates(ClientRec, PRIVATE_CLIENT);
    if (!client)
        return (ClientPtr) NULL;
    InitClient(client, i, ospriv);
    if (!InitClientResources(client)) {
        dixFreeObjectWithPrivates(client, PRIVATE_CLIENT);
        return (ClientPtr) NULL;
    }
    data.reqType = 1;
    data.length = bytes_to_int32(sz_xReq + sz_xConnClientPrefix);
    if (!InsertFakeRequest(client, (char *) &data, sz_xReq)) {
        FreeClientResources(client);
        dixFreeObjectWithPrivates(client, PRIVATE_CLIENT);
        return (ClientPtr) NULL;
    }
    if (i == currentMaxClients)
        currentMaxClients++;
    while ((nextFreeClientID < LimitClients) && clients[nextFreeClientID])
        nextFreeClientID++;

    /* Enable client ID tracking. This must be done before
     * ClientStateCallback. */
    ReserveClientIds(client);

    if (ClientStateCallback) {
        NewClientInfoRec clientinfo;

        clientinfo.client = client;
        clientinfo.prefix = (xConnSetupPrefix *) NULL;
        clientinfo.setup = (xConnSetup *) NULL;
        CallCallbacks((&ClientStateCallback), (void *) &clientinfo);
    }
    return client;
}

int
ProcInitialConnection(ClientPtr client)
{
    REQUEST(xReq);
    xConnClientPrefix *prefix;
    int whichbyte = 1;
    char order;

    prefix = (xConnClientPrefix *) ((char *)stuff + sz_xReq);
    order = prefix->byteOrder;
    if (order != 'l' && order != 'B' && order != 'r' && order != 'R')
	return client->noClientException = -1;
    if (((*(char *) &whichbyte) && (order == 'B' || order == 'R')) ||
	(!(*(char *) &whichbyte) && (order == 'l' || order == 'r'))) {
	client->swapped = TRUE;
	SwapConnClientPrefix(prefix);
    }
    stuff->reqType = 2;
    stuff->length += bytes_to_int32(prefix->nbytesAuthProto) +
        bytes_to_int32(prefix->nbytesAuthString);
    if (client->swapped) {
        swaps(&stuff->length);
    }
    if (order == 'r' || order == 'R') {
	client->local = FALSE;
    }
    ResetCurrentRequest(client);
    return Success;
}

static int
SendConnSetup(ClientPtr client, const char *reason)
{
    xWindowRoot *root;
    int i;
    int numScreens;
    char *lConnectionInfo;
    xConnSetupPrefix *lconnSetupPrefix;

    if (reason) {
        xConnSetupPrefix csp;

        csp.success = xFalse;
        csp.lengthReason = strlen(reason);
        csp.length = bytes_to_int32(csp.lengthReason);
        csp.majorVersion = X_PROTOCOL;
        csp.minorVersion = X_PROTOCOL_REVISION;
        if (client->swapped)
            WriteSConnSetupPrefix(client, &csp);
        else
            WriteToClient(client, sz_xConnSetupPrefix, &csp);
        WriteToClient(client, (int) csp.lengthReason, reason);
        return client->noClientException = -1;
    }

    numScreens = screenInfo.numScreens;
    lConnectionInfo = ConnectionInfo;
    lconnSetupPrefix = &connSetupPrefix;

    /* We're about to start speaking X protocol back to the client by
     * sending the connection setup info.  This means the authorization
     * step is complete, and we can count the client as an
     * authorized one.
     */
    nClients++;

    client->requestVector = client->swapped ? SwappedProcVector : ProcVector;
    client->sequence = 0;
    ((xConnSetup *) lConnectionInfo)->ridBase = client->clientAsMask;
    ((xConnSetup *) lConnectionInfo)->ridMask = RESOURCE_ID_MASK;
#ifdef MATCH_CLIENT_ENDIAN
    ((xConnSetup *) lConnectionInfo)->imageByteOrder = ClientOrder(client);
    ((xConnSetup *) lConnectionInfo)->bitmapBitOrder = ClientOrder(client);
#endif
    /* fill in the "currentInputMask" */
    root = (xWindowRoot *) (lConnectionInfo + connBlockScreenStart);
#ifdef PANORAMIX
    if (noPanoramiXExtension)
        numScreens = screenInfo.numScreens;
    else
        numScreens = ((xConnSetup *) ConnectionInfo)->numRoots;
#endif

    for (i = 0; i < numScreens; i++) {
        unsigned int j;
        xDepth *pDepth;
        WindowPtr pRoot = screenInfo.screens[i]->root;

        root->currentInputMask = pRoot->eventMask | wOtherEventMasks(pRoot);
        pDepth = (xDepth *) (root + 1);
        for (j = 0; j < root->nDepths; j++) {
            pDepth = (xDepth *) (((char *) (pDepth + 1)) +
                                 pDepth->nVisuals * sizeof(xVisualType));
        }
        root = (xWindowRoot *) pDepth;
    }

    if (client->swapped) {
        WriteSConnSetupPrefix(client, lconnSetupPrefix);
        WriteSConnectionInfo(client,
                             (unsigned long) (lconnSetupPrefix->length << 2),
                             lConnectionInfo);
    }
    else {
        WriteToClient(client, sizeof(xConnSetupPrefix), lconnSetupPrefix);
        WriteToClient(client, (int) (lconnSetupPrefix->length << 2),
		      lConnectionInfo);
    }
    client->clientState = ClientStateRunning;
    if (ClientStateCallback) {
        NewClientInfoRec clientinfo;

        clientinfo.client = client;
        clientinfo.prefix = lconnSetupPrefix;
        clientinfo.setup = (xConnSetup *) lConnectionInfo;
        CallCallbacks((&ClientStateCallback), (void *) &clientinfo);
    }
    CancelDispatchExceptionTimer();
    return Success;
}

int
ProcEstablishConnection(ClientPtr client)
{
    const char *reason;
    char *auth_proto, *auth_string;
    xConnClientPrefix *prefix;

    REQUEST(xReq);

    prefix = (xConnClientPrefix *) ((char *) stuff + sz_xReq);
    auth_proto = (char *) prefix + sz_xConnClientPrefix;
    auth_string = auth_proto + pad_to_int32(prefix->nbytesAuthProto);

    if ((client->req_len << 2) != sz_xReq + sz_xConnClientPrefix +
	pad_to_int32(prefix->nbytesAuthProto) +
	pad_to_int32(prefix->nbytesAuthString))
        reason = "Bad length";
    else if ((prefix->majorVersion != X_PROTOCOL) ||
        (prefix->minorVersion != X_PROTOCOL_REVISION))
        reason = "Protocol version mismatch";
    else
        reason = ClientAuthorized(client,
                                  (unsigned short) prefix->nbytesAuthProto,
                                  auth_proto,
                                  (unsigned short) prefix->nbytesAuthString,
                                  auth_string);

    return (SendConnSetup(client, reason));
}

void
SendErrorToClient(ClientPtr client, unsigned majorCode, unsigned minorCode,
                  XID resId, int errorCode)
{
    xError rep = {
        .type = X_Error,
        .errorCode = errorCode,
        .resourceID = resId,
        .minorCode = minorCode,
        .majorCode = majorCode
    };

    WriteEventsToClient(client, 1, (xEvent *) &rep);
}

void
MarkClientException(ClientPtr client)
{
    client->noClientException = -1;
}

/*
 * This array encodes the answer to the question "what is the log base 2
 * of the number of pixels that fit in a scanline pad unit?"
 * Note that ~0 is an invalid entry (mostly for the benefit of the reader).
 */
static int answer[6][4] = {
    /* pad   pad   pad     pad */
    /*  8     16    32    64 */

    {3, 4, 5, 6},               /* 1 bit per pixel */
    {1, 2, 3, 4},               /* 4 bits per pixel */
    {0, 1, 2, 3},               /* 8 bits per pixel */
    {~0, 0, 1, 2},              /* 16 bits per pixel */
    {~0, ~0, 0, 1},             /* 24 bits per pixel */
    {~0, ~0, 0, 1}              /* 32 bits per pixel */
};

/*
 * This array gives the answer to the question "what is the first index for
 * the answer array above given the number of bits per pixel?"
 * Note that ~0 is an invalid entry (mostly for the benefit of the reader).
 */
static int indexForBitsPerPixel[33] = {
    ~0, 0, ~0, ~0,              /* 1 bit per pixel */
    1, ~0, ~0, ~0,              /* 4 bits per pixel */
    2, ~0, ~0, ~0,              /* 8 bits per pixel */
    ~0, ~0, ~0, ~0,
    3, ~0, ~0, ~0,              /* 16 bits per pixel */
    ~0, ~0, ~0, ~0,
    4, ~0, ~0, ~0,              /* 24 bits per pixel */
    ~0, ~0, ~0, ~0,
    5                           /* 32 bits per pixel */
};

/*
 * This array gives the bytesperPixel value for cases where the number
 * of bits per pixel is a multiple of 8 but not a power of 2.
 */
static int answerBytesPerPixel[33] = {
    ~0, 0, ~0, ~0,              /* 1 bit per pixel */
    0, ~0, ~0, ~0,              /* 4 bits per pixel */
    0, ~0, ~0, ~0,              /* 8 bits per pixel */
    ~0, ~0, ~0, ~0,
    0, ~0, ~0, ~0,              /* 16 bits per pixel */
    ~0, ~0, ~0, ~0,
    3, ~0, ~0, ~0,              /* 24 bits per pixel */
    ~0, ~0, ~0, ~0,
    0                           /* 32 bits per pixel */
};

/*
 * This array gives the answer to the question "what is the second index for
 * the answer array above given the number of bits per scanline pad unit?"
 * Note that ~0 is an invalid entry (mostly for the benefit of the reader).
 */
static int indexForScanlinePad[65] = {
    ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0,
    0, ~0, ~0, ~0,              /* 8 bits per scanline pad unit */
    ~0, ~0, ~0, ~0,
    1, ~0, ~0, ~0,              /* 16 bits per scanline pad unit */
    ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0,
    2, ~0, ~0, ~0,              /* 32 bits per scanline pad unit */
    ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0,
    3                           /* 64 bits per scanline pad unit */
};

/*
	grow the array of screenRecs if necessary.
	call the device-supplied initialization procedure
with its screen number, a pointer to its ScreenRec, argc, and argv.
	return the number of successfully installed screens.

*/

static int init_screen(ScreenPtr pScreen, int i, Bool gpu)
{
    int scanlinepad, format, depth, bitsPerPixel, j, k;

    dixInitScreenSpecificPrivates(pScreen);

    if (!dixAllocatePrivates(&pScreen->devPrivates, PRIVATE_SCREEN)) {
        return -1;
    }
    pScreen->myNum = i;
    if (gpu) {
        pScreen->myNum += GPU_SCREEN_OFFSET;
        pScreen->isGPU = TRUE;
    }
    pScreen->totalPixmapSize = 0;       /* computed in CreateScratchPixmapForScreen */
    pScreen->ClipNotify = 0;    /* for R4 ddx compatibility */
    pScreen->CreateScreenResources = 0;

    xorg_list_init(&pScreen->pixmap_dirty_list);
    xorg_list_init(&pScreen->secondary_list);

    /*
     * This loop gets run once for every Screen that gets added,
     * but that's ok.  If the ddx layer initializes the formats
     * one at a time calling AddScreen() after each, then each
     * iteration will make it a little more accurate.  Worst case
     * we do this loop N * numPixmapFormats where N is # of screens.
     * Anyway, this must be called after InitOutput and before the
     * screen init routine is called.
     */
    for (format = 0; format < screenInfo.numPixmapFormats; format++) {
        depth = screenInfo.formats[format].depth;
        bitsPerPixel = screenInfo.formats[format].bitsPerPixel;
        scanlinepad = screenInfo.formats[format].scanlinePad;
        j = indexForBitsPerPixel[bitsPerPixel];
        k = indexForScanlinePad[scanlinepad];
        PixmapWidthPaddingInfo[depth].padPixelsLog2 = answer[j][k];
        PixmapWidthPaddingInfo[depth].padRoundUp =
            (scanlinepad / bitsPerPixel) - 1;
        j = indexForBitsPerPixel[8];    /* bits per byte */
        PixmapWidthPaddingInfo[depth].padBytesLog2 = answer[j][k];
        PixmapWidthPaddingInfo[depth].bitsPerPixel = bitsPerPixel;
        if (answerBytesPerPixel[bitsPerPixel]) {
            PixmapWidthPaddingInfo[depth].notPower2 = 1;
            PixmapWidthPaddingInfo[depth].bytesPerPixel =
                answerBytesPerPixel[bitsPerPixel];
        }
        else {
            PixmapWidthPaddingInfo[depth].notPower2 = 0;
        }
    }
    return 0;
}

int
AddScreen(Bool (*pfnInit) (ScreenPtr /*pScreen */ ,
                           int /*argc */ ,
                           char **      /*argv */
          ), int argc, char **argv)
{

    int i;
    ScreenPtr pScreen;
    Bool ret;

    i = screenInfo.numScreens;
    if (i == MAXSCREENS)
        return -1;

    pScreen = (ScreenPtr) calloc(1, sizeof(ScreenRec));
    if (!pScreen)
        return -1;

    ret = init_screen(pScreen, i, FALSE);
    if (ret != 0) {
        free(pScreen);
        return ret;
    }
    /* This is where screen specific stuff gets initialized.  Load the
       screen structure, call the hardware, whatever.
       This is also where the default colormap should be allocated and
       also pixel values for blackPixel, whitePixel, and the cursor
       Note that InitScreen is NOT allowed to modify argc, argv, or
       any of the strings pointed to by argv.  They may be passed to
       multiple screens.
     */
    screenInfo.screens[i] = pScreen;
    screenInfo.numScreens++;
    if (!(*pfnInit) (pScreen, argc, argv)) {
        dixFreeScreenSpecificPrivates(pScreen);
        dixFreePrivates(pScreen->devPrivates, PRIVATE_SCREEN);
        free(pScreen);
        screenInfo.numScreens--;
        return -1;
    }

    update_desktop_dimensions();

    dixRegisterScreenPrivateKey(&cursorScreenDevPriv, pScreen, PRIVATE_CURSOR,
                                0);

    return i;
}

int
AddGPUScreen(Bool (*pfnInit) (ScreenPtr /*pScreen */ ,
                              int /*argc */ ,
                              char **      /*argv */
                              ),
             int argc, char **argv)
{
    int i;
    ScreenPtr pScreen;
    Bool ret;

    i = screenInfo.numGPUScreens;
    if (i == MAXGPUSCREENS)
        return -1;

    pScreen = (ScreenPtr) calloc(1, sizeof(ScreenRec));
    if (!pScreen)
        return -1;

    ret = init_screen(pScreen, i, TRUE);
    if (ret != 0) {
        free(pScreen);
        return ret;
    }

    /* This is where screen specific stuff gets initialized.  Load the
       screen structure, call the hardware, whatever.
       This is also where the default colormap should be allocated and
       also pixel values for blackPixel, whitePixel, and the cursor
       Note that InitScreen is NOT allowed to modify argc, argv, or
       any of the strings pointed to by argv.  They may be passed to
       multiple screens.
     */
    screenInfo.gpuscreens[i] = pScreen;
    screenInfo.numGPUScreens++;
    if (!(*pfnInit) (pScreen, argc, argv)) {
        dixFreePrivates(pScreen->devPrivates, PRIVATE_SCREEN);
        free(pScreen);
        screenInfo.numGPUScreens--;
        return -1;
    }

    update_desktop_dimensions();

    /*
     * We cannot register the Screen PRIVATE_CURSOR key if cursors are already
     * created, because dix/privates.c does not have relocation code for
     * PRIVATE_CURSOR. Once this is fixed the if() can be removed and we can
     * register the Screen PRIVATE_CURSOR key unconditionally.
     */
    if (!dixPrivatesCreated(PRIVATE_CURSOR))
        dixRegisterScreenPrivateKey(&cursorScreenDevPriv, pScreen,
                                    PRIVATE_CURSOR, 0);

    return i;
}

void
RemoveGPUScreen(ScreenPtr pScreen)
{
    int idx, j;
    if (!pScreen->isGPU)
        return;

    idx = pScreen->myNum - GPU_SCREEN_OFFSET;
    for (j = idx; j < screenInfo.numGPUScreens - 1; j++) {
        screenInfo.gpuscreens[j] = screenInfo.gpuscreens[j + 1];
        screenInfo.gpuscreens[j]->myNum = j + GPU_SCREEN_OFFSET;
    }
    screenInfo.numGPUScreens--;

    /* this gets freed later in the resource list, but without
     * the screen existing it causes crashes - so remove it here */
    if (pScreen->defColormap)
        FreeResource(pScreen->defColormap, RT_COLORMAP);
    free(pScreen);

}

void
AttachUnboundGPU(ScreenPtr pScreen, ScreenPtr new)
{
    assert(new->isGPU);
    assert(!new->current_primary);
    xorg_list_add(&new->secondary_head, &pScreen->secondary_list);
    new->current_primary = pScreen;
}

void
DetachUnboundGPU(ScreenPtr secondary)
{
    assert(secondary->isGPU);
    assert(!secondary->is_output_secondary);
    assert(!secondary->is_offload_secondary);
    xorg_list_del(&secondary->secondary_head);
    secondary->current_primary = NULL;
}

void
AttachOutputGPU(ScreenPtr pScreen, ScreenPtr new)
{
    assert(new->isGPU);
    assert(!new->is_output_secondary);
    assert(new->current_primary == pScreen);
    new->is_output_secondary = TRUE;
    new->current_primary->output_secondarys++;
}

void
DetachOutputGPU(ScreenPtr secondary)
{
    assert(secondary->isGPU);
    assert(secondary->is_output_secondary);
    secondary->current_primary->output_secondarys--;
    secondary->is_output_secondary = FALSE;
}

void
AttachOffloadGPU(ScreenPtr pScreen, ScreenPtr new)
{
    assert(new->isGPU);
    assert(!new->is_offload_secondary);
    assert(new->current_primary == pScreen);
    new->is_offload_secondary = TRUE;
}

void
DetachOffloadGPU(ScreenPtr secondary)
{
    assert(secondary->isGPU);
    assert(secondary->is_offload_secondary);
    secondary->is_offload_secondary = FALSE;
}

