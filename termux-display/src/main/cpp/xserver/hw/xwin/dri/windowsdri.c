/*
 * Copyright © 2014 Jon Turney
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/windowsdristr.h>

#include "dixstruct.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#include "swaprep.h"
#include "protocol-versions.h"
#include "windowsdri.h"
#include "glx/dri_helpers.h"

static int WindowsDRIErrorBase = 0;
static unsigned char WindowsDRIReqCode = 0;
static int WindowsDRIEventBase = 0;

static void
WindowsDRIResetProc(ExtensionEntry* extEntry)
{
}

static int
ProcWindowsDRIQueryVersion(ClientPtr client)
{
    xWindowsDRIQueryVersionReply rep;

    REQUEST_SIZE_MATCH(xWindowsDRIQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = SERVER_WINDOWSDRI_MAJOR_VERSION;
    rep.minorVersion = SERVER_WINDOWSDRI_MINOR_VERSION;
    rep.patchVersion = SERVER_WINDOWSDRI_PATCH_VERSION;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
        swapl(&rep.patchVersion);
    }
    WriteToClient(client, sizeof(xWindowsDRIQueryVersionReply), &rep);
    return Success;
}

static int
ProcWindowsDRIQueryDirectRenderingCapable(ClientPtr client)
{
    xWindowsDRIQueryDirectRenderingCapableReply rep;

    REQUEST(xWindowsDRIQueryDirectRenderingCapableReq);
    REQUEST_SIZE_MATCH(xWindowsDRIQueryDirectRenderingCapableReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    if (!client->local)
        rep.isCapable = 0;
    else
        rep.isCapable = glxWinGetScreenAiglxIsActive(screenInfo.screens[stuff->screen]);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }

    WriteToClient(client,
                  sizeof(xWindowsDRIQueryDirectRenderingCapableReply),
                  &rep);
    return Success;
}

static int
ProcWindowsDRIQueryDrawable(ClientPtr client)
{
    xWindowsDRIQueryDrawableReply rep;
    int rc;

    REQUEST(xWindowsDRIQueryDrawableReq);
    REQUEST_SIZE_MATCH(xWindowsDRIQueryDrawableReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    rc = glxWinQueryDrawable(client, stuff->drawable, &(rep.drawable_type), &(rep.handle));

    if (rc)
        return rc;

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.handle);
        swapl(&rep.drawable_type);
    }

    WriteToClient(client, sizeof(xWindowsDRIQueryDrawableReply), &rep);
    return Success;
}

static int
ProcWindowsDRIFBConfigToPixelFormat(ClientPtr client)
{
    xWindowsDRIFBConfigToPixelFormatReply rep;

    REQUEST(xWindowsDRIFBConfigToPixelFormatReq);
    REQUEST_SIZE_MATCH(xWindowsDRIFBConfigToPixelFormatReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;

    rep.pixelFormatIndex = glxWinFBConfigIDToPixelFormatIndex(stuff->screen, stuff->fbConfigID);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.pixelFormatIndex);
    }

    WriteToClient(client, sizeof(xWindowsDRIFBConfigToPixelFormatReply), &rep);
    return Success;
}

/* dispatch */

static int
ProcWindowsDRIDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_WindowsDRIQueryVersion:
        return ProcWindowsDRIQueryVersion(client);

    case X_WindowsDRIQueryDirectRenderingCapable:
        return ProcWindowsDRIQueryDirectRenderingCapable(client);
    }

    if (!client->local)
        return WindowsDRIErrorBase + WindowsDRIClientNotLocal;

    switch (stuff->data) {
    case X_WindowsDRIQueryDrawable:
        return ProcWindowsDRIQueryDrawable(client);

    case X_WindowsDRIFBConfigToPixelFormat:
        return ProcWindowsDRIFBConfigToPixelFormat(client);

    default:
        return BadRequest;
    }
}

static void
SNotifyEvent(xWindowsDRINotifyEvent *from,
             xWindowsDRINotifyEvent *to)
{
    to->type = from->type;
    to->kind = from->kind;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->time, to->time);
}

static int
SProcWindowsDRIQueryVersion(ClientPtr client)
{
    REQUEST(xWindowsDRIQueryVersionReq);
    swaps(&stuff->length);
    return ProcWindowsDRIQueryVersion(client);
}

static int
SProcWindowsDRIQueryDirectRenderingCapable(ClientPtr client)
{
    REQUEST(xWindowsDRIQueryDirectRenderingCapableReq);
    swaps(&stuff->length);
    swapl(&stuff->screen);
    return ProcWindowsDRIQueryDirectRenderingCapable(client);
}

static int
SProcWindowsDRIQueryDrawable(ClientPtr client)
{
    REQUEST(xWindowsDRIQueryDrawableReq);
    swaps(&stuff->length);
    swapl(&stuff->screen);
    swapl(&stuff->drawable);
    return ProcWindowsDRIQueryDrawable(client);
}

static int
SProcWindowsDRIFBConfigToPixelFormat(ClientPtr client)
{
    REQUEST(xWindowsDRIFBConfigToPixelFormatReq);
    swaps(&stuff->length);
    swapl(&stuff->screen);
    swapl(&stuff->fbConfigID);
    return ProcWindowsDRIFBConfigToPixelFormat(client);
}

static int
SProcWindowsDRIDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_WindowsDRIQueryVersion:
        return SProcWindowsDRIQueryVersion(client);

    case X_WindowsDRIQueryDirectRenderingCapable:
        return SProcWindowsDRIQueryDirectRenderingCapable(client);
    }

    if (!client->local)
        return WindowsDRIErrorBase + WindowsDRIClientNotLocal;

    switch (stuff->data) {
    case X_WindowsDRIQueryDrawable:
        return SProcWindowsDRIQueryDrawable(client);

    case X_WindowsDRIFBConfigToPixelFormat:
        return SProcWindowsDRIFBConfigToPixelFormat(client);

    default:
        return BadRequest;
    }
}

void
WindowsDRIExtensionInit(void)
{
    ExtensionEntry* extEntry;

    if ((extEntry = AddExtension(WINDOWSDRINAME,
                                 WindowsDRINumberEvents,
                                 WindowsDRINumberErrors,
                                 ProcWindowsDRIDispatch,
                                 SProcWindowsDRIDispatch,
                                 WindowsDRIResetProc,
                                 StandardMinorOpcode))) {
        size_t i;
        WindowsDRIReqCode = (unsigned char)extEntry->base;
        WindowsDRIErrorBase = extEntry->errorBase;
        WindowsDRIEventBase = extEntry->eventBase;
        for (i = 0; i < WindowsDRINumberEvents; i++)
            EventSwapVector[WindowsDRIEventBase + i] = (EventSwapPtr)SNotifyEvent;
    }
}
