/*
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2010, 2021 Red Hat, Inc.
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
 *
 * Copyright © 2002 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "xfixesint.h"
#include "protocol-versions.h"
#include "extinit.h"

static unsigned char XFixesReqCode;
int XFixesEventBase;
int XFixesErrorBase;

static DevPrivateKeyRec XFixesClientPrivateKeyRec;

#define XFixesClientPrivateKey (&XFixesClientPrivateKeyRec)

static int
ProcXFixesQueryVersion(ClientPtr client)
{
    int major, minor;
    XFixesClientPtr pXFixesClient = GetXFixesClient(client);
    xXFixesQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };

    REQUEST(xXFixesQueryVersionReq);

    REQUEST_SIZE_MATCH(xXFixesQueryVersionReq);

    if (version_compare(stuff->majorVersion, stuff->minorVersion,
                        SERVER_XFIXES_MAJOR_VERSION,
                        SERVER_XFIXES_MINOR_VERSION) < 0) {
        major = max(pXFixesClient->major_version, stuff->majorVersion);
        minor = stuff->minorVersion;
    }
    else {
        major = SERVER_XFIXES_MAJOR_VERSION;
        minor = SERVER_XFIXES_MINOR_VERSION;
    }

    pXFixesClient->major_version = major;
    rep.majorVersion = min(stuff->majorVersion, major);
    rep.minorVersion = minor;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.majorVersion);
        swapl(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xXFixesQueryVersionReply), &rep);
    return Success;
}

/* Major version controls available requests */
static const int version_requests[] = {
    X_XFixesQueryVersion,       /* before client sends QueryVersion */
    X_XFixesGetCursorImage,     /* Version 1 */
    X_XFixesChangeCursorByName, /* Version 2 */
    X_XFixesExpandRegion,       /* Version 3 */
    X_XFixesShowCursor,         /* Version 4 */
    X_XFixesDestroyPointerBarrier,      /* Version 5 */
    X_XFixesGetClientDisconnectMode,    /* Version 6 */
};

int (*ProcXFixesVector[XFixesNumberRequests]) (ClientPtr) = {
/*************** Version 1 ******************/
    ProcXFixesQueryVersion,
        ProcXFixesChangeSaveSet,
        ProcXFixesSelectSelectionInput,
        ProcXFixesSelectCursorInput, ProcXFixesGetCursorImage,
/*************** Version 2 ******************/
        ProcXFixesCreateRegion,
        ProcXFixesCreateRegionFromBitmap,
        ProcXFixesCreateRegionFromWindow,
        ProcXFixesCreateRegionFromGC,
        ProcXFixesCreateRegionFromPicture,
        ProcXFixesDestroyRegion,
        ProcXFixesSetRegion,
        ProcXFixesCopyRegion,
        ProcXFixesCombineRegion,
        ProcXFixesCombineRegion,
        ProcXFixesCombineRegion,
        ProcXFixesInvertRegion,
        ProcXFixesTranslateRegion,
        ProcXFixesRegionExtents,
        ProcXFixesFetchRegion,
        ProcXFixesSetGCClipRegion,
        ProcXFixesSetWindowShapeRegion,
        ProcXFixesSetPictureClipRegion,
        ProcXFixesSetCursorName,
        ProcXFixesGetCursorName,
        ProcXFixesGetCursorImageAndName,
        ProcXFixesChangeCursor, ProcXFixesChangeCursorByName,
/*************** Version 3 ******************/
        ProcXFixesExpandRegion,
/*************** Version 4 ****************/
        ProcXFixesHideCursor, ProcXFixesShowCursor,
/*************** Version 5 ****************/
        ProcXFixesCreatePointerBarrier, ProcXFixesDestroyPointerBarrier,
/*************** Version 6 ****************/
        ProcXFixesSetClientDisconnectMode, ProcXFixesGetClientDisconnectMode,
};

static int
ProcXFixesDispatch(ClientPtr client)
{
    REQUEST(xXFixesReq);
    XFixesClientPtr pXFixesClient = GetXFixesClient(client);

    if (pXFixesClient->major_version >= ARRAY_SIZE(version_requests))
        return BadRequest;
    if (stuff->xfixesReqType > version_requests[pXFixesClient->major_version])
        return BadRequest;
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

static _X_COLD int
SProcXFixesQueryVersion(ClientPtr client)
{
    REQUEST(xXFixesQueryVersionReq);
    REQUEST_SIZE_MATCH(xXFixesQueryVersionReq);

    swaps(&stuff->length);
    swapl(&stuff->majorVersion);
    swapl(&stuff->minorVersion);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

static int (*SProcXFixesVector[XFixesNumberRequests]) (ClientPtr) = {
/*************** Version 1 ******************/
    SProcXFixesQueryVersion,
        SProcXFixesChangeSaveSet,
        SProcXFixesSelectSelectionInput,
        SProcXFixesSelectCursorInput, SProcXFixesGetCursorImage,
/*************** Version 2 ******************/
        SProcXFixesCreateRegion,
        SProcXFixesCreateRegionFromBitmap,
        SProcXFixesCreateRegionFromWindow,
        SProcXFixesCreateRegionFromGC,
        SProcXFixesCreateRegionFromPicture,
        SProcXFixesDestroyRegion,
        SProcXFixesSetRegion,
        SProcXFixesCopyRegion,
        SProcXFixesCombineRegion,
        SProcXFixesCombineRegion,
        SProcXFixesCombineRegion,
        SProcXFixesInvertRegion,
        SProcXFixesTranslateRegion,
        SProcXFixesRegionExtents,
        SProcXFixesFetchRegion,
        SProcXFixesSetGCClipRegion,
        SProcXFixesSetWindowShapeRegion,
        SProcXFixesSetPictureClipRegion,
        SProcXFixesSetCursorName,
        SProcXFixesGetCursorName,
        SProcXFixesGetCursorImageAndName,
        SProcXFixesChangeCursor, SProcXFixesChangeCursorByName,
/*************** Version 3 ******************/
        SProcXFixesExpandRegion,
/*************** Version 4 ****************/
        SProcXFixesHideCursor, SProcXFixesShowCursor,
/*************** Version 5 ****************/
        SProcXFixesCreatePointerBarrier, SProcXFixesDestroyPointerBarrier,
/*************** Version 6 ****************/
        SProcXFixesSetClientDisconnectMode, SProcXFixesGetClientDisconnectMode,
};

static _X_COLD int
SProcXFixesDispatch(ClientPtr client)
{
    REQUEST(xXFixesReq);
    XFixesClientPtr pXFixesClient = GetXFixesClient(client);

    if (pXFixesClient->major_version >= ARRAY_SIZE(version_requests))
        return BadRequest;
    if (stuff->xfixesReqType > version_requests[pXFixesClient->major_version])
        return BadRequest;
    return (*SProcXFixesVector[stuff->xfixesReqType]) (client);
}

void
XFixesExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (!dixRegisterPrivateKey
        (&XFixesClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(XFixesClientRec)))
        return;

    if (XFixesSelectionInit() &&
        XFixesCursorInit() &&
        XFixesRegionInit() &&
        XFixesClientDisconnectInit() &&
        (extEntry = AddExtension(XFIXES_NAME, XFixesNumberEvents,
                                 XFixesNumberErrors,
                                 ProcXFixesDispatch, SProcXFixesDispatch,
                                 NULL, StandardMinorOpcode)) != 0) {
        XFixesReqCode = (unsigned char) extEntry->base;
        XFixesEventBase = extEntry->eventBase;
        XFixesErrorBase = extEntry->errorBase;
        EventSwapVector[XFixesEventBase + XFixesSelectionNotify] =
            (EventSwapPtr) SXFixesSelectionNotifyEvent;
        EventSwapVector[XFixesEventBase + XFixesCursorNotify] =
            (EventSwapPtr) SXFixesCursorNotifyEvent;
        SetResourceTypeErrorValue(RegionResType, XFixesErrorBase + BadRegion);
        SetResourceTypeErrorValue(PointerBarrierType,
                                  XFixesErrorBase + BadBarrier);
    }
}

#ifdef PANORAMIX

int (*PanoramiXSaveXFixesVector[XFixesNumberRequests]) (ClientPtr);

void
PanoramiXFixesInit(void)
{
    int i;

    for (i = 0; i < XFixesNumberRequests; i++)
        PanoramiXSaveXFixesVector[i] = ProcXFixesVector[i];
    /*
     * Stuff in Xinerama aware request processing hooks
     */
    ProcXFixesVector[X_XFixesSetGCClipRegion] = PanoramiXFixesSetGCClipRegion;
    ProcXFixesVector[X_XFixesSetWindowShapeRegion] =
        PanoramiXFixesSetWindowShapeRegion;
    ProcXFixesVector[X_XFixesSetPictureClipRegion] =
        PanoramiXFixesSetPictureClipRegion;
}

void
PanoramiXFixesReset(void)
{
    int i;

    for (i = 0; i < XFixesNumberRequests; i++)
        ProcXFixesVector[i] = PanoramiXSaveXFixesVector[i];
}

#endif
