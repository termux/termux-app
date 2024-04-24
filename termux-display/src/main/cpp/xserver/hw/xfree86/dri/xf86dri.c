/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright 2000 VA Linux Systems, Inc.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Jens Owen <jens@tungstengraphics.com>
 *   Rickard E. (Rik) Faith <faith@valinux.com>
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <string.h>

#include "xf86.h"

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "extinit.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DRI_SERVER_
#include <X11/dri/xf86driproto.h>
#include "swaprep.h"
#include "xf86str.h"
#include "dri.h"
#include "sarea.h"
#include "dristruct.h"
#include "xf86drm.h"
#include "protocol-versions.h"
#include "xf86Extensions.h"

static int DRIErrorBase;

static void XF86DRIResetProc(ExtensionEntry *extEntry);

static unsigned char DRIReqCode = 0;

/*ARGSUSED*/
static void
XF86DRIResetProc(ExtensionEntry *extEntry)
{
    DRIReset();
}

static int
ProcXF86DRIQueryVersion(register ClientPtr client)
{
    xXF86DRIQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_XF86DRI_MAJOR_VERSION,
        .minorVersion = SERVER_XF86DRI_MINOR_VERSION,
        .patchVersion = SERVER_XF86DRI_PATCH_VERSION
    };

    REQUEST_SIZE_MATCH(xXF86DRIQueryVersionReq);
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
        swapl(&rep.patchVersion);
    }
    WriteToClient(client, sizeof(xXF86DRIQueryVersionReply), &rep);
    return Success;
}

static int
ProcXF86DRIQueryDirectRenderingCapable(register ClientPtr client)
{
    xXF86DRIQueryDirectRenderingCapableReply rep;
    Bool isCapable;

    REQUEST(xXF86DRIQueryDirectRenderingCapableReq);
    REQUEST_SIZE_MATCH(xXF86DRIQueryDirectRenderingCapableReq);
    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIQueryDirectRenderingCapable(screenInfo.screens[stuff->screen],
                                        &isCapable)) {
        return BadValue;
    }

    if (!client->local || client->swapped)
        isCapable = 0;

    rep = (xXF86DRIQueryDirectRenderingCapableReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .isCapable = isCapable
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }

    WriteToClient(client,
                  sizeof(xXF86DRIQueryDirectRenderingCapableReply),
                  &rep);
    return Success;
}

static int
ProcXF86DRIOpenConnection(register ClientPtr client)
{
    xXF86DRIOpenConnectionReply rep;
    drm_handle_t hSAREA;
    char *busIdString;
    CARD32 busIdStringLength = 0;

    REQUEST(xXF86DRIOpenConnectionReq);
    REQUEST_SIZE_MATCH(xXF86DRIOpenConnectionReq);
    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIOpenConnection(screenInfo.screens[stuff->screen],
                           &hSAREA, &busIdString)) {
        return BadValue;
    }

    if (busIdString)
        busIdStringLength = strlen(busIdString);

    rep = (xXF86DRIOpenConnectionReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(SIZEOF(xXF86DRIOpenConnectionReply) -
                                 SIZEOF(xGenericReply) +
                                 pad_to_int32(busIdStringLength)),
        .busIdStringLength = busIdStringLength,

        .hSAREALow = (CARD32) (hSAREA & 0xffffffff),
#if defined(LONG64) && !defined(__linux__)
        .hSAREAHigh = (CARD32) (hSAREA >> 32),
#else
        .hSAREAHigh = 0
#endif
    };

    WriteToClient(client, sizeof(xXF86DRIOpenConnectionReply), &rep);
    if (busIdStringLength)
        WriteToClient(client, busIdStringLength, busIdString);
    return Success;
}

static int
ProcXF86DRIAuthConnection(register ClientPtr client)
{
    xXF86DRIAuthConnectionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .authenticated = 1
    };

    REQUEST(xXF86DRIAuthConnectionReq);
    REQUEST_SIZE_MATCH(xXF86DRIAuthConnectionReq);
    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIAuthConnection(screenInfo.screens[stuff->screen], stuff->magic)) {
        ErrorF("Failed to authenticate %lu\n", (unsigned long) stuff->magic);
        rep.authenticated = 0;
    }
    WriteToClient(client, sizeof(xXF86DRIAuthConnectionReply), &rep);
    return Success;
}

static int
ProcXF86DRICloseConnection(register ClientPtr client)
{
    REQUEST(xXF86DRICloseConnectionReq);
    REQUEST_SIZE_MATCH(xXF86DRICloseConnectionReq);
    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    DRICloseConnection(screenInfo.screens[stuff->screen]);

    return Success;
}

static int
ProcXF86DRIGetClientDriverName(register ClientPtr client)
{
    xXF86DRIGetClientDriverNameReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .clientDriverNameLength = 0
    };
    char *clientDriverName;

    REQUEST(xXF86DRIGetClientDriverNameReq);
    REQUEST_SIZE_MATCH(xXF86DRIGetClientDriverNameReq);
    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    DRIGetClientDriverName(screenInfo.screens[stuff->screen],
                           (int *) &rep.ddxDriverMajorVersion,
                           (int *) &rep.ddxDriverMinorVersion,
                           (int *) &rep.ddxDriverPatchVersion,
                           &clientDriverName);

    if (clientDriverName)
        rep.clientDriverNameLength = strlen(clientDriverName);
    rep.length = bytes_to_int32(SIZEOF(xXF86DRIGetClientDriverNameReply) -
                                SIZEOF(xGenericReply) +
                                pad_to_int32(rep.clientDriverNameLength));

    WriteToClient(client, sizeof(xXF86DRIGetClientDriverNameReply), &rep);
    if (rep.clientDriverNameLength)
        WriteToClient(client, rep.clientDriverNameLength, clientDriverName);
    return Success;
}

static int
ProcXF86DRICreateContext(register ClientPtr client)
{
    xXF86DRICreateContextReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    ScreenPtr pScreen;

    REQUEST(xXF86DRICreateContextReq);
    REQUEST_SIZE_MATCH(xXF86DRICreateContextReq);
    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    pScreen = screenInfo.screens[stuff->screen];

    if (!DRICreateContext(pScreen,
                          NULL,
                          stuff->context, (drm_context_t *) &rep.hHWContext)) {
        return BadValue;
    }

    WriteToClient(client, sizeof(xXF86DRICreateContextReply), &rep);
    return Success;
}

static int
ProcXF86DRIDestroyContext(register ClientPtr client)
{
    REQUEST(xXF86DRIDestroyContextReq);
    REQUEST_SIZE_MATCH(xXF86DRIDestroyContextReq);
    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIDestroyContext(screenInfo.screens[stuff->screen], stuff->context)) {
        return BadValue;
    }

    return Success;
}

static int
ProcXF86DRICreateDrawable(ClientPtr client)
{
    xXF86DRICreateDrawableReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    DrawablePtr pDrawable;
    int rc;

    REQUEST(xXF86DRICreateDrawableReq);
    REQUEST_SIZE_MATCH(xXF86DRICreateDrawableReq);
    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;

    if (!DRICreateDrawable(screenInfo.screens[stuff->screen], client,
                           pDrawable, (drm_drawable_t *) &rep.hHWDrawable)) {
        return BadValue;
    }

    WriteToClient(client, sizeof(xXF86DRICreateDrawableReply), &rep);
    return Success;
}

static int
ProcXF86DRIDestroyDrawable(register ClientPtr client)
{
    REQUEST(xXF86DRIDestroyDrawableReq);
    DrawablePtr pDrawable;
    int rc;

    REQUEST_SIZE_MATCH(xXF86DRIDestroyDrawableReq);

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;

    if (!DRIDestroyDrawable(screenInfo.screens[stuff->screen], client,
                            pDrawable)) {
        return BadValue;
    }

    return Success;
}

static int
ProcXF86DRIGetDrawableInfo(register ClientPtr client)
{
    xXF86DRIGetDrawableInfoReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    DrawablePtr pDrawable;
    int X, Y, W, H;
    drm_clip_rect_t *pClipRects, *pClippedRects;
    drm_clip_rect_t *pBackClipRects;
    int backX, backY, rc;

    REQUEST(xXF86DRIGetDrawableInfoReq);
    REQUEST_SIZE_MATCH(xXF86DRIGetDrawableInfoReq);
    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;

    if (!DRIGetDrawableInfo(screenInfo.screens[stuff->screen],
                            pDrawable,
                            (unsigned int *) &rep.drawableTableIndex,
                            (unsigned int *) &rep.drawableTableStamp,
                            (int *) &X,
                            (int *) &Y,
                            (int *) &W,
                            (int *) &H,
                            (int *) &rep.numClipRects,
                            &pClipRects,
                            &backX,
                            &backY,
                            (int *) &rep.numBackClipRects, &pBackClipRects)) {
        return BadValue;
    }

    rep.drawableX = X;
    rep.drawableY = Y;
    rep.drawableWidth = W;
    rep.drawableHeight = H;
    rep.length = (SIZEOF(xXF86DRIGetDrawableInfoReply) - SIZEOF(xGenericReply));

    rep.backX = backX;
    rep.backY = backY;

    if (rep.numBackClipRects)
        rep.length += sizeof(drm_clip_rect_t) * rep.numBackClipRects;

    pClippedRects = pClipRects;

    if (rep.numClipRects) {
        /* Clip cliprects to screen dimensions (redirected windows) */
        pClippedRects = xallocarray(rep.numClipRects, sizeof(drm_clip_rect_t));

        if (pClippedRects) {
            ScreenPtr pScreen = screenInfo.screens[stuff->screen];
            int i, j;

            for (i = 0, j = 0; i < rep.numClipRects; i++) {
                pClippedRects[j].x1 = max(pClipRects[i].x1, 0);
                pClippedRects[j].y1 = max(pClipRects[i].y1, 0);
                pClippedRects[j].x2 = min(pClipRects[i].x2, pScreen->width);
                pClippedRects[j].y2 = min(pClipRects[i].y2, pScreen->height);

                if (pClippedRects[j].x1 < pClippedRects[j].x2 &&
                    pClippedRects[j].y1 < pClippedRects[j].y2) {
                    j++;
                }
            }

            rep.numClipRects = j;
        }
        else {
            rep.numClipRects = 0;
        }

        rep.length += sizeof(drm_clip_rect_t) * rep.numClipRects;
    }

    rep.length = bytes_to_int32(rep.length);

    WriteToClient(client, sizeof(xXF86DRIGetDrawableInfoReply), &rep);

    if (rep.numClipRects) {
        WriteToClient(client,
                      sizeof(drm_clip_rect_t) * rep.numClipRects,
                      pClippedRects);
        free(pClippedRects);
    }

    if (rep.numBackClipRects) {
        WriteToClient(client,
                      sizeof(drm_clip_rect_t) * rep.numBackClipRects,
                      pBackClipRects);
    }

    return Success;
}

static int
ProcXF86DRIGetDeviceInfo(register ClientPtr client)
{
    xXF86DRIGetDeviceInfoReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    drm_handle_t hFrameBuffer;
    void *pDevPrivate;

    REQUEST(xXF86DRIGetDeviceInfoReq);
    REQUEST_SIZE_MATCH(xXF86DRIGetDeviceInfoReq);
    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIGetDeviceInfo(screenInfo.screens[stuff->screen],
                          &hFrameBuffer,
                          (int *) &rep.framebufferOrigin,
                          (int *) &rep.framebufferSize,
                          (int *) &rep.framebufferStride,
                          (int *) &rep.devPrivateSize, &pDevPrivate)) {
        return BadValue;
    }

    rep.hFrameBufferLow = (CARD32) (hFrameBuffer & 0xffffffff);
#if defined(LONG64) && !defined(__linux__)
    rep.hFrameBufferHigh = (CARD32) (hFrameBuffer >> 32);
#else
    rep.hFrameBufferHigh = 0;
#endif

    if (rep.devPrivateSize) {
        rep.length = bytes_to_int32(SIZEOF(xXF86DRIGetDeviceInfoReply) -
                                    SIZEOF(xGenericReply) +
                                    pad_to_int32(rep.devPrivateSize));
    }

    WriteToClient(client, sizeof(xXF86DRIGetDeviceInfoReply), &rep);
    if (rep.length) {
        WriteToClient(client, rep.devPrivateSize, pDevPrivate);
    }
    return Success;
}

static int
ProcXF86DRIDispatch(register ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_XF86DRIQueryVersion:
        return ProcXF86DRIQueryVersion(client);
    case X_XF86DRIQueryDirectRenderingCapable:
        return ProcXF86DRIQueryDirectRenderingCapable(client);
    }

    if (!client->local)
        return DRIErrorBase + XF86DRIClientNotLocal;

    switch (stuff->data) {
    case X_XF86DRIOpenConnection:
        return ProcXF86DRIOpenConnection(client);
    case X_XF86DRICloseConnection:
        return ProcXF86DRICloseConnection(client);
    case X_XF86DRIGetClientDriverName:
        return ProcXF86DRIGetClientDriverName(client);
    case X_XF86DRICreateContext:
        return ProcXF86DRICreateContext(client);
    case X_XF86DRIDestroyContext:
        return ProcXF86DRIDestroyContext(client);
    case X_XF86DRICreateDrawable:
        return ProcXF86DRICreateDrawable(client);
    case X_XF86DRIDestroyDrawable:
        return ProcXF86DRIDestroyDrawable(client);
    case X_XF86DRIGetDrawableInfo:
        return ProcXF86DRIGetDrawableInfo(client);
    case X_XF86DRIGetDeviceInfo:
        return ProcXF86DRIGetDeviceInfo(client);
    case X_XF86DRIAuthConnection:
        return ProcXF86DRIAuthConnection(client);
        /* {Open,Close}FullScreen are deprecated now */
    default:
        return BadRequest;
    }
}

static int _X_COLD
SProcXF86DRIQueryVersion(register ClientPtr client)
{
    REQUEST(xXF86DRIQueryVersionReq);
    swaps(&stuff->length);
    return ProcXF86DRIQueryVersion(client);
}

static int _X_COLD
SProcXF86DRIQueryDirectRenderingCapable(register ClientPtr client)
{
    REQUEST(xXF86DRIQueryDirectRenderingCapableReq);
    REQUEST_SIZE_MATCH(xXF86DRIQueryDirectRenderingCapableReq);
    swaps(&stuff->length);
    swapl(&stuff->screen);
    return ProcXF86DRIQueryDirectRenderingCapable(client);
}

static int _X_COLD
SProcXF86DRIDispatch(register ClientPtr client)
{
    REQUEST(xReq);

    /*
     * Only local clients are allowed DRI access, but remote clients still need
     * these requests to find out cleanly.
     */
    switch (stuff->data) {
    case X_XF86DRIQueryVersion:
        return SProcXF86DRIQueryVersion(client);
    case X_XF86DRIQueryDirectRenderingCapable:
        return SProcXF86DRIQueryDirectRenderingCapable(client);
    default:
        return DRIErrorBase + XF86DRIClientNotLocal;
    }
}

void
XFree86DRIExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (DRIExtensionInit() &&
        (extEntry = AddExtension(XF86DRINAME,
                                 XF86DRINumberEvents,
                                 XF86DRINumberErrors,
                                 ProcXF86DRIDispatch,
                                 SProcXF86DRIDispatch,
                                 XF86DRIResetProc, StandardMinorOpcode))) {
        DRIReqCode = (unsigned char) extEntry->base;
        DRIErrorBase = extEntry->errorBase;
    }
}
