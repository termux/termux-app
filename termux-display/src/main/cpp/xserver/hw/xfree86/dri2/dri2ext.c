/*
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@redhat.com)
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/dri2proto.h>
#include <X11/extensions/xfixeswire.h>
#include "dixstruct.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "extnsionst.h"
#include "xfixes.h"
#include "dri2.h"
#include "dri2int.h"
#include "protocol-versions.h"

/* The only xf86 includes */
#include "xf86Module.h"
#include "xf86Extensions.h"

static int DRI2EventBase;


static Bool
validDrawable(ClientPtr client, XID drawable, Mask access_mode,
              DrawablePtr *pDrawable, int *status)
{
    *status = dixLookupDrawable(pDrawable, drawable, client,
                                M_DRAWABLE_WINDOW | M_DRAWABLE_PIXMAP,
                                access_mode);
    if (*status != Success) {
        client->errorValue = drawable;
        return FALSE;
    }

    return TRUE;
}

static int
ProcDRI2QueryVersion(ClientPtr client)
{
    REQUEST(xDRI2QueryVersionReq);
    xDRI2QueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = dri2_major,
        .minorVersion = dri2_minor
    };

    if (client->swapped)
        swaps(&stuff->length);

    REQUEST_SIZE_MATCH(xDRI2QueryVersionReq);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.majorVersion);
        swapl(&rep.minorVersion);
    }

    WriteToClient(client, sizeof(xDRI2QueryVersionReply), &rep);

    return Success;
}

static int
ProcDRI2Connect(ClientPtr client)
{
    REQUEST(xDRI2ConnectReq);
    xDRI2ConnectReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .driverNameLength = 0,
        .deviceNameLength = 0
    };
    DrawablePtr pDraw;
    int fd, status;
    const char *driverName;
    const char *deviceName;

    REQUEST_SIZE_MATCH(xDRI2ConnectReq);
    if (!validDrawable(client, stuff->window, DixGetAttrAccess,
                       &pDraw, &status))
        return status;

    if (!DRI2Connect(client, pDraw->pScreen,
                     stuff->driverType, &fd, &driverName, &deviceName))
        goto fail;

    rep.driverNameLength = strlen(driverName);
    rep.deviceNameLength = strlen(deviceName);
    rep.length = (rep.driverNameLength + 3) / 4 +
        (rep.deviceNameLength + 3) / 4;

 fail:
    WriteToClient(client, sizeof(xDRI2ConnectReply), &rep);
    WriteToClient(client, rep.driverNameLength, driverName);
    WriteToClient(client, rep.deviceNameLength, deviceName);

    return Success;
}

static int
ProcDRI2Authenticate(ClientPtr client)
{
    REQUEST(xDRI2AuthenticateReq);
    xDRI2AuthenticateReply rep;
    DrawablePtr pDraw;
    int status;

    REQUEST_SIZE_MATCH(xDRI2AuthenticateReq);
    if (!validDrawable(client, stuff->window, DixGetAttrAccess,
                       &pDraw, &status))
        return status;

    rep = (xDRI2AuthenticateReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .authenticated = DRI2Authenticate(client, pDraw->pScreen, stuff->magic)
    };
    WriteToClient(client, sizeof(xDRI2AuthenticateReply), &rep);

    return Success;
}

static void
DRI2InvalidateBuffersEvent(DrawablePtr pDraw, void *priv, XID id)
{
    ClientPtr client = priv;
    xDRI2InvalidateBuffers event = {
        .type = DRI2EventBase + DRI2_InvalidateBuffers,
        .drawable = id
    };

    WriteEventsToClient(client, 1, (xEvent *) &event);
}

static int
ProcDRI2CreateDrawable(ClientPtr client)
{
    REQUEST(xDRI2CreateDrawableReq);
    DrawablePtr pDrawable;
    int status;

    REQUEST_SIZE_MATCH(xDRI2CreateDrawableReq);

    if (!validDrawable(client, stuff->drawable, DixAddAccess,
                       &pDrawable, &status))
        return status;

    status = DRI2CreateDrawable(client, pDrawable, stuff->drawable,
                                DRI2InvalidateBuffersEvent, client);
    if (status != Success)
        return status;

    return Success;
}

static int
ProcDRI2DestroyDrawable(ClientPtr client)
{
    REQUEST(xDRI2DestroyDrawableReq);
    DrawablePtr pDrawable;
    int status;

    REQUEST_SIZE_MATCH(xDRI2DestroyDrawableReq);
    if (!validDrawable(client, stuff->drawable, DixRemoveAccess,
                       &pDrawable, &status))
        return status;

    return Success;
}

static int
send_buffers_reply(ClientPtr client, DrawablePtr pDrawable,
                   DRI2BufferPtr * buffers, int count, int width, int height)
{
    xDRI2GetBuffersReply rep;
    int skip = 0;
    int i;

    if (buffers == NULL)
        return BadAlloc;

    if (pDrawable->type == DRAWABLE_WINDOW) {
        for (i = 0; i < count; i++) {
            /* Do not send the real front buffer of a window to the client.
             */
            if (buffers[i]->attachment == DRI2BufferFrontLeft) {
                skip++;
                continue;
            }
        }
    }

    rep = (xDRI2GetBuffersReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = (count - skip) * sizeof(xDRI2Buffer) / 4,
        .width = width,
        .height = height,
        .count = count - skip
    };
    WriteToClient(client, sizeof(xDRI2GetBuffersReply), &rep);

    for (i = 0; i < count; i++) {
        xDRI2Buffer buffer;

        /* Do not send the real front buffer of a window to the client.
         */
        if ((pDrawable->type == DRAWABLE_WINDOW)
            && (buffers[i]->attachment == DRI2BufferFrontLeft)) {
            continue;
        }

        buffer.attachment = buffers[i]->attachment;
        buffer.name = buffers[i]->name;
        buffer.pitch = buffers[i]->pitch;
        buffer.cpp = buffers[i]->cpp;
        buffer.flags = buffers[i]->flags;
        WriteToClient(client, sizeof(xDRI2Buffer), &buffer);
    }
    return Success;
}

static int
ProcDRI2GetBuffers(ClientPtr client)
{
    REQUEST(xDRI2GetBuffersReq);
    DrawablePtr pDrawable;
    DRI2BufferPtr *buffers;
    int status, width, height, count;
    unsigned int *attachments;

    REQUEST_AT_LEAST_SIZE(xDRI2GetBuffersReq);
    /* stuff->count is a count of CARD32 attachments that follows */
    if (stuff->count > (INT_MAX / sizeof(CARD32)))
        return BadLength;
    REQUEST_FIXED_SIZE(xDRI2GetBuffersReq, stuff->count * sizeof(CARD32));

    if (!validDrawable(client, stuff->drawable, DixReadAccess | DixWriteAccess,
                       &pDrawable, &status))
        return status;

    if (DRI2ThrottleClient(client, pDrawable))
        return Success;

    attachments = (unsigned int *) &stuff[1];
    buffers = DRI2GetBuffers(pDrawable, &width, &height,
                             attachments, stuff->count, &count);

    return send_buffers_reply(client, pDrawable, buffers, count, width, height);

}

static int
ProcDRI2GetBuffersWithFormat(ClientPtr client)
{
    REQUEST(xDRI2GetBuffersReq);
    DrawablePtr pDrawable;
    DRI2BufferPtr *buffers;
    int status, width, height, count;
    unsigned int *attachments;

    REQUEST_AT_LEAST_SIZE(xDRI2GetBuffersReq);
    /* stuff->count is a count of pairs of CARD32s (attachments & formats)
       that follows */
    if (stuff->count > (INT_MAX / (2 * sizeof(CARD32))))
        return BadLength;
    REQUEST_FIXED_SIZE(xDRI2GetBuffersReq,
                       stuff->count * (2 * sizeof(CARD32)));
    if (!validDrawable(client, stuff->drawable, DixReadAccess | DixWriteAccess,
                       &pDrawable, &status))
        return status;

    if (DRI2ThrottleClient(client, pDrawable))
        return Success;

    attachments = (unsigned int *) &stuff[1];
    buffers = DRI2GetBuffersWithFormat(pDrawable, &width, &height,
                                       attachments, stuff->count, &count);

    return send_buffers_reply(client, pDrawable, buffers, count, width, height);
}

static int
ProcDRI2CopyRegion(ClientPtr client)
{
    REQUEST(xDRI2CopyRegionReq);
    xDRI2CopyRegionReply rep;
    DrawablePtr pDrawable;
    int status;
    RegionPtr pRegion;

    REQUEST_SIZE_MATCH(xDRI2CopyRegionReq);

    if (!validDrawable(client, stuff->drawable, DixWriteAccess,
                       &pDrawable, &status))
        return status;

    VERIFY_REGION(pRegion, stuff->region, client, DixReadAccess);

    status = DRI2CopyRegion(pDrawable, pRegion, stuff->dest, stuff->src);
    if (status != Success)
        return status;

    /* CopyRegion needs to be a round trip to make sure the X server
     * queues the swap buffer rendering commands before the DRI client
     * continues rendering.  The reply has a bitmask to signal the
     * presence of optional return values as well, but we're not using
     * that yet.
     */

    rep = (xDRI2CopyRegionReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };

    WriteToClient(client, sizeof(xDRI2CopyRegionReply), &rep);

    return Success;
}

static void
load_swap_reply(xDRI2SwapBuffersReply * rep, CARD64 sbc)
{
    rep->swap_hi = sbc >> 32;
    rep->swap_lo = sbc & 0xffffffff;
}

static CARD64
vals_to_card64(CARD32 lo, CARD32 hi)
{
    return (CARD64) hi << 32 | lo;
}

static void
DRI2SwapEvent(ClientPtr client, void *data, int type, CARD64 ust, CARD64 msc,
              CARD32 sbc)
{
    DrawablePtr pDrawable = data;
    xDRI2BufferSwapComplete2 event = {
        .type = DRI2EventBase + DRI2_BufferSwapComplete,
        .event_type = type,
        .drawable = pDrawable->id,
        .ust_hi = (CARD64) ust >> 32,
        .ust_lo = ust & 0xffffffff,
        .msc_hi = (CARD64) msc >> 32,
        .msc_lo = msc & 0xffffffff,
        .sbc = sbc
    };

    WriteEventsToClient(client, 1, (xEvent *) &event);
}

static int
ProcDRI2SwapBuffers(ClientPtr client)
{
    REQUEST(xDRI2SwapBuffersReq);
    xDRI2SwapBuffersReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    DrawablePtr pDrawable;
    CARD64 target_msc, divisor, remainder, swap_target;
    int status;

    REQUEST_SIZE_MATCH(xDRI2SwapBuffersReq);

    if (!validDrawable(client, stuff->drawable,
                       DixReadAccess | DixWriteAccess, &pDrawable, &status))
        return status;

    /*
     * Ensures an out of control client can't exhaust our swap queue, and
     * also orders swaps.
     */
    if (DRI2ThrottleClient(client, pDrawable))
        return Success;

    target_msc = vals_to_card64(stuff->target_msc_lo, stuff->target_msc_hi);
    divisor = vals_to_card64(stuff->divisor_lo, stuff->divisor_hi);
    remainder = vals_to_card64(stuff->remainder_lo, stuff->remainder_hi);

    status = DRI2SwapBuffers(client, pDrawable, target_msc, divisor, remainder,
                             &swap_target, DRI2SwapEvent, pDrawable);
    if (status != Success)
        return BadDrawable;

    load_swap_reply(&rep, swap_target);

    WriteToClient(client, sizeof(xDRI2SwapBuffersReply), &rep);

    return Success;
}

static void
load_msc_reply(xDRI2MSCReply * rep, CARD64 ust, CARD64 msc, CARD64 sbc)
{
    rep->ust_hi = ust >> 32;
    rep->ust_lo = ust & 0xffffffff;
    rep->msc_hi = msc >> 32;
    rep->msc_lo = msc & 0xffffffff;
    rep->sbc_hi = sbc >> 32;
    rep->sbc_lo = sbc & 0xffffffff;
}

static int
ProcDRI2GetMSC(ClientPtr client)
{
    REQUEST(xDRI2GetMSCReq);
    xDRI2MSCReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    DrawablePtr pDrawable;
    CARD64 ust, msc, sbc;
    int status;

    REQUEST_SIZE_MATCH(xDRI2GetMSCReq);

    if (!validDrawable(client, stuff->drawable, DixReadAccess, &pDrawable,
                       &status))
        return status;

    status = DRI2GetMSC(pDrawable, &ust, &msc, &sbc);
    if (status != Success)
        return status;

    load_msc_reply(&rep, ust, msc, sbc);

    WriteToClient(client, sizeof(xDRI2MSCReply), &rep);

    return Success;
}

static int
ProcDRI2WaitMSC(ClientPtr client)
{
    REQUEST(xDRI2WaitMSCReq);
    DrawablePtr pDrawable;
    CARD64 target, divisor, remainder;
    int status;

    /* FIXME: in restart case, client may be gone at this point */

    REQUEST_SIZE_MATCH(xDRI2WaitMSCReq);

    if (!validDrawable(client, stuff->drawable, DixReadAccess, &pDrawable,
                       &status))
        return status;

    target = vals_to_card64(stuff->target_msc_lo, stuff->target_msc_hi);
    divisor = vals_to_card64(stuff->divisor_lo, stuff->divisor_hi);
    remainder = vals_to_card64(stuff->remainder_lo, stuff->remainder_hi);

    status = DRI2WaitMSC(client, pDrawable, target, divisor, remainder);
    if (status != Success)
        return status;

    return Success;
}

int
ProcDRI2WaitMSCReply(ClientPtr client, CARD64 ust, CARD64 msc, CARD64 sbc)
{
    xDRI2MSCReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };

    load_msc_reply(&rep, ust, msc, sbc);

    WriteToClient(client, sizeof(xDRI2MSCReply), &rep);

    return Success;
}

static int
ProcDRI2SwapInterval(ClientPtr client)
{
    REQUEST(xDRI2SwapIntervalReq);
    DrawablePtr pDrawable;
    int status;

    /* FIXME: in restart case, client may be gone at this point */

    REQUEST_SIZE_MATCH(xDRI2SwapIntervalReq);

    if (!validDrawable(client, stuff->drawable, DixReadAccess | DixWriteAccess,
                       &pDrawable, &status))
        return status;

    DRI2SwapInterval(pDrawable, stuff->interval);

    return Success;
}

static int
ProcDRI2WaitSBC(ClientPtr client)
{
    REQUEST(xDRI2WaitSBCReq);
    DrawablePtr pDrawable;
    CARD64 target;
    int status;

    REQUEST_SIZE_MATCH(xDRI2WaitSBCReq);

    if (!validDrawable(client, stuff->drawable, DixReadAccess, &pDrawable,
                       &status))
        return status;

    target = vals_to_card64(stuff->target_sbc_lo, stuff->target_sbc_hi);
    status = DRI2WaitSBC(client, pDrawable, target);

    return status;
}

static int
ProcDRI2GetParam(ClientPtr client)
{
    REQUEST(xDRI2GetParamReq);
    xDRI2GetParamReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    DrawablePtr pDrawable;
    CARD64 value;
    int status;

    REQUEST_SIZE_MATCH(xDRI2GetParamReq);

    if (!validDrawable(client, stuff->drawable, DixReadAccess,
                       &pDrawable, &status))
        return status;

    status = DRI2GetParam(client, pDrawable, stuff->param,
                          &rep.is_param_recognized, &value);
    rep.value_hi = value >> 32;
    rep.value_lo = value & 0xffffffff;

    if (status != Success)
        return status;

    WriteToClient(client, sizeof(xDRI2GetParamReply), &rep);

    return status;
}

static int
ProcDRI2Dispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_DRI2QueryVersion:
        return ProcDRI2QueryVersion(client);
    }

    if (!client->local)
        return BadRequest;

    switch (stuff->data) {
    case X_DRI2Connect:
        return ProcDRI2Connect(client);
    case X_DRI2Authenticate:
        return ProcDRI2Authenticate(client);
    case X_DRI2CreateDrawable:
        return ProcDRI2CreateDrawable(client);
    case X_DRI2DestroyDrawable:
        return ProcDRI2DestroyDrawable(client);
    case X_DRI2GetBuffers:
        return ProcDRI2GetBuffers(client);
    case X_DRI2CopyRegion:
        return ProcDRI2CopyRegion(client);
    case X_DRI2GetBuffersWithFormat:
        return ProcDRI2GetBuffersWithFormat(client);
    case X_DRI2SwapBuffers:
        return ProcDRI2SwapBuffers(client);
    case X_DRI2GetMSC:
        return ProcDRI2GetMSC(client);
    case X_DRI2WaitMSC:
        return ProcDRI2WaitMSC(client);
    case X_DRI2WaitSBC:
        return ProcDRI2WaitSBC(client);
    case X_DRI2SwapInterval:
        return ProcDRI2SwapInterval(client);
    case X_DRI2GetParam:
        return ProcDRI2GetParam(client);
    default:
        return BadRequest;
    }
}

static int _X_COLD
SProcDRI2Connect(ClientPtr client)
{
    REQUEST(xDRI2ConnectReq);
    xDRI2ConnectReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .driverNameLength = 0,
        .deviceNameLength = 0
    };

    /* If the client is swapped, it's not local.  Talk to the hand. */

    swaps(&stuff->length);
    if (sizeof(*stuff) / 4 != client->req_len)
        return BadLength;

    swaps(&rep.sequenceNumber);

    WriteToClient(client, sizeof(xDRI2ConnectReply), &rep);

    return Success;
}

static int _X_COLD
SProcDRI2Dispatch(ClientPtr client)
{
    REQUEST(xReq);

    /*
     * Only local clients are allowed DRI access, but remote clients
     * still need these requests to find out cleanly.
     */
    switch (stuff->data) {
    case X_DRI2QueryVersion:
        return ProcDRI2QueryVersion(client);
    case X_DRI2Connect:
        return SProcDRI2Connect(client);
    default:
        return BadRequest;
    }
}

void
DRI2ExtensionInit(void)
{
    ExtensionEntry *dri2Extension;

#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        return;
#endif

    dri2Extension = AddExtension(DRI2_NAME,
                                 DRI2NumberEvents,
                                 DRI2NumberErrors,
                                 ProcDRI2Dispatch,
                                 SProcDRI2Dispatch, NULL, StandardMinorOpcode);

    DRI2EventBase = dri2Extension->eventBase;

    DRI2ModuleSetup();
}
