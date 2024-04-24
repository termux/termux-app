/*
 * Copyright © 2013 Keith Packard
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

#include "present_priv.h"
#include "randrstr.h"
#include <protocol-versions.h>

static int
proc_present_query_version(ClientPtr client)
{
    REQUEST(xPresentQueryVersionReq);
    xPresentQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_PRESENT_MAJOR_VERSION,
        .minorVersion = SERVER_PRESENT_MINOR_VERSION
    };

    REQUEST_SIZE_MATCH(xPresentQueryVersionReq);
    /* From presentproto:
     *
     * The client sends the highest supported version to the server
     * and the server sends the highest version it supports, but no
     * higher than the requested version.
     */

    if (rep.majorVersion > stuff->majorVersion ||
        rep.minorVersion > stuff->minorVersion) {
        rep.majorVersion = stuff->majorVersion;
        rep.minorVersion = stuff->minorVersion;
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.majorVersion);
        swapl(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(rep), &rep);
    return Success;
}

#define VERIFY_FENCE_OR_NONE(fence_ptr, fence_id, client, access) do {  \
        if ((fence_id) == None)                                         \
            (fence_ptr) = NULL;                                         \
        else {                                                          \
            int __rc__ = SyncVerifyFence(&fence_ptr, fence_id, client, access); \
            if (__rc__ != Success)                                      \
                return __rc__;                                          \
        }                                                               \
    } while (0)

#define VERIFY_CRTC_OR_NONE(crtc_ptr, crtc_id, client, access) do {     \
        if ((crtc_id) == None)                                          \
            (crtc_ptr) = NULL;                                          \
        else {                                                          \
            VERIFY_RR_CRTC(crtc_id, crtc_ptr, access);                  \
        }                                                               \
    } while (0)

static int
proc_present_pixmap(ClientPtr client)
{
    REQUEST(xPresentPixmapReq);
    WindowPtr           window;
    PixmapPtr           pixmap;
    RegionPtr           valid = NULL;
    RegionPtr           update = NULL;
    SyncFence           *wait_fence;
    SyncFence           *idle_fence;
    RRCrtcPtr           target_crtc;
    int                 ret;
    int                 nnotifies;
    present_notify_ptr  notifies = NULL;

    REQUEST_AT_LEAST_SIZE(xPresentPixmapReq);
    ret = dixLookupWindow(&window, stuff->window, client, DixWriteAccess);
    if (ret != Success)
        return ret;
    ret = dixLookupResourceByType((void **) &pixmap, stuff->pixmap, RT_PIXMAP, client, DixReadAccess);
    if (ret != Success)
        return ret;

    if (window->drawable.depth != pixmap->drawable.depth)
        return BadMatch;

    VERIFY_REGION_OR_NONE(valid, stuff->valid, client, DixReadAccess);
    VERIFY_REGION_OR_NONE(update, stuff->update, client, DixReadAccess);

    VERIFY_CRTC_OR_NONE(target_crtc, stuff->target_crtc, client, DixReadAccess);

    VERIFY_FENCE_OR_NONE(wait_fence, stuff->wait_fence, client, DixReadAccess);
    VERIFY_FENCE_OR_NONE(idle_fence, stuff->idle_fence, client, DixWriteAccess);

    if (stuff->options & ~(PresentAllOptions)) {
        client->errorValue = stuff->options;
        return BadValue;
    }

    /*
     * Check to see if remainder is sane
     */
    if (stuff->divisor == 0) {
        if (stuff->remainder != 0) {
            client->errorValue = (CARD32) stuff->remainder;
            return BadValue;
        }
    } else {
        if (stuff->remainder >= stuff->divisor) {
            client->errorValue = (CARD32) stuff->remainder;
            return BadValue;
        }
    }

    nnotifies = (client->req_len << 2) - sizeof (xPresentPixmapReq);
    if (nnotifies % sizeof (xPresentNotify))
        return BadLength;

    nnotifies /= sizeof (xPresentNotify);
    if (nnotifies) {
        ret = present_create_notifies(client, nnotifies, (xPresentNotify *) (stuff + 1), &notifies);
        if (ret != Success)
            return ret;
    }

    ret = present_pixmap(window, pixmap, stuff->serial, valid, update,
                         stuff->x_off, stuff->y_off, target_crtc,
                         wait_fence, idle_fence, stuff->options,
                         stuff->target_msc, stuff->divisor, stuff->remainder, notifies, nnotifies);
    if (ret != Success)
        present_destroy_notifies(notifies, nnotifies);
    return ret;
}

static int
proc_present_notify_msc(ClientPtr client)
{
    REQUEST(xPresentNotifyMSCReq);
    WindowPtr   window;
    int         rc;

    REQUEST_SIZE_MATCH(xPresentNotifyMSCReq);
    rc = dixLookupWindow(&window, stuff->window, client, DixReadAccess);
    if (rc != Success)
        return rc;

    /*
     * Check to see if remainder is sane
     */
    if (stuff->divisor == 0) {
        if (stuff->remainder != 0) {
            client->errorValue = (CARD32) stuff->remainder;
            return BadValue;
        }
    } else {
        if (stuff->remainder >= stuff->divisor) {
            client->errorValue = (CARD32) stuff->remainder;
            return BadValue;
        }
    }

    return present_notify_msc(window, stuff->serial,
                              stuff->target_msc, stuff->divisor, stuff->remainder);
}

static int
proc_present_select_input (ClientPtr client)
{
    REQUEST(xPresentSelectInputReq);
    WindowPtr window;
    int rc;

    REQUEST_SIZE_MATCH(xPresentSelectInputReq);

    rc = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    if (stuff->eventMask & ~PresentAllEvents) {
        client->errorValue = stuff->eventMask;
        return BadValue;
    }
    return present_select_input(client, stuff->eid, window, stuff->eventMask);
}

static int
proc_present_query_capabilities (ClientPtr client)
{
    REQUEST(xPresentQueryCapabilitiesReq);
    xPresentQueryCapabilitiesReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
    };
    WindowPtr   window;
    RRCrtcPtr   crtc = NULL;
    int         r;

    REQUEST_SIZE_MATCH(xPresentQueryCapabilitiesReq);
    r = dixLookupWindow(&window, stuff->target, client, DixGetAttrAccess);
    switch (r) {
    case Success:
        crtc = present_get_crtc(window);
        break;
    case BadWindow:
        VERIFY_RR_CRTC(stuff->target, crtc, DixGetAttrAccess);
        break;
    default:
        return r;
    }

    rep.capabilities = present_query_capabilities(crtc);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.capabilities);
    }
    WriteToClient(client, sizeof(rep), &rep);
    return Success;
}

static int (*proc_present_vector[PresentNumberRequests]) (ClientPtr) = {
    proc_present_query_version,            /* 0 */
    proc_present_pixmap,                   /* 1 */
    proc_present_notify_msc,               /* 2 */
    proc_present_select_input,             /* 3 */
    proc_present_query_capabilities,       /* 4 */
};

int
proc_present_dispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= PresentNumberRequests || !proc_present_vector[stuff->data])
        return BadRequest;
    return (*proc_present_vector[stuff->data]) (client);
}

static int _X_COLD
sproc_present_query_version(ClientPtr client)
{
    REQUEST(xPresentQueryVersionReq);
    REQUEST_SIZE_MATCH(xPresentQueryVersionReq);

    swaps(&stuff->length);
    swapl(&stuff->majorVersion);
    swapl(&stuff->minorVersion);
    return (*proc_present_vector[stuff->presentReqType]) (client);
}

static int _X_COLD
sproc_present_pixmap(ClientPtr client)
{
    REQUEST(xPresentPixmapReq);
    REQUEST_AT_LEAST_SIZE(xPresentPixmapReq);

    swaps(&stuff->length);
    swapl(&stuff->window);
    swapl(&stuff->pixmap);
    swapl(&stuff->valid);
    swapl(&stuff->update);
    swaps(&stuff->x_off);
    swaps(&stuff->y_off);
    swapll(&stuff->target_msc);
    swapll(&stuff->divisor);
    swapll(&stuff->remainder);
    swapl(&stuff->idle_fence);
    return (*proc_present_vector[stuff->presentReqType]) (client);
}

static int _X_COLD
sproc_present_notify_msc(ClientPtr client)
{
    REQUEST(xPresentNotifyMSCReq);
    REQUEST_SIZE_MATCH(xPresentNotifyMSCReq);

    swaps(&stuff->length);
    swapl(&stuff->window);
    swapll(&stuff->target_msc);
    swapll(&stuff->divisor);
    swapll(&stuff->remainder);
    return (*proc_present_vector[stuff->presentReqType]) (client);
}

static int _X_COLD
sproc_present_select_input (ClientPtr client)
{
    REQUEST(xPresentSelectInputReq);
    REQUEST_SIZE_MATCH(xPresentSelectInputReq);

    swaps(&stuff->length);
    swapl(&stuff->window);
    swapl(&stuff->eventMask);
    return (*proc_present_vector[stuff->presentReqType]) (client);
}

static int _X_COLD
sproc_present_query_capabilities (ClientPtr client)
{
    REQUEST(xPresentQueryCapabilitiesReq);
    REQUEST_SIZE_MATCH(xPresentQueryCapabilitiesReq);
    swaps(&stuff->length);
    swapl(&stuff->target);
    return (*proc_present_vector[stuff->presentReqType]) (client);
}

static int (*sproc_present_vector[PresentNumberRequests]) (ClientPtr) = {
    sproc_present_query_version,           /* 0 */
    sproc_present_pixmap,                  /* 1 */
    sproc_present_notify_msc,              /* 2 */
    sproc_present_select_input,            /* 3 */
    sproc_present_query_capabilities,      /* 4 */
};

int _X_COLD
sproc_present_dispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data >= PresentNumberRequests || !sproc_present_vector[stuff->data])
        return BadRequest;
    return (*sproc_present_vector[stuff->data]) (client);
}
