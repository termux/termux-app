/*
 * Copyright 2008 Red Hat, Inc.
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
 * Author: Peter Hutterer
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "dixstruct.h"
#include "windowstr.h"
#include "exglobals.h"
#include "exevents.h"
#include <X11/extensions/XI2proto.h>
#include "inpututils.h"

#include "xiselectev.h"

/**
 * Ruleset:
 * - if A has XIAllDevices, B may select on device X
 * - If A has XIAllDevices, B may select on XIAllMasterDevices
 * - If A has XIAllMasterDevices, B may select on device X
 * - If A has XIAllMasterDevices, B may select on XIAllDevices
 * - if A has device X, B may select on XIAllDevices/XIAllMasterDevices
 */
static int
check_for_touch_selection_conflicts(ClientPtr B, WindowPtr win, int deviceid,
                                    int evtype)
{
    OtherInputMasks *inputMasks = wOtherInputMasks(win);
    InputClients *A = NULL;

    if (inputMasks)
        A = inputMasks->inputClients;
    for (; A; A = A->next) {
        DeviceIntPtr tmp;

        if (CLIENT_ID(A->resource) == B->index)
            continue;

        if (deviceid == XIAllDevices)
            tmp = inputInfo.all_devices;
        else if (deviceid == XIAllMasterDevices)
            tmp = inputInfo.all_master_devices;
        else
            dixLookupDevice(&tmp, deviceid, serverClient, DixReadAccess);
        if (!tmp)
            return BadImplementation;       /* this shouldn't happen */

        /* A has XIAllDevices */
        if (xi2mask_isset_for_device(A->xi2mask, inputInfo.all_devices, evtype)) {
            if (deviceid == XIAllDevices)
                return BadAccess;
        }

        /* A has XIAllMasterDevices */
        if (xi2mask_isset_for_device(A->xi2mask, inputInfo.all_master_devices, evtype)) {
            if (deviceid == XIAllMasterDevices)
                return BadAccess;
        }

        /* A has this device */
        if (xi2mask_isset_for_device(A->xi2mask, tmp, evtype))
            return BadAccess;
    }

    return Success;
}


/**
 * Check the given mask (in len bytes) for invalid mask bits.
 * Invalid mask bits are any bits above XI2LastEvent.
 *
 * @return BadValue if at least one invalid bit is set or Success otherwise.
 */
int
XICheckInvalidMaskBits(ClientPtr client, unsigned char *mask, int len)
{
    if (len >= XIMaskLen(XI2LASTEVENT)) {
        int i;

        for (i = XI2LASTEVENT + 1; i < len * 8; i++) {
            if (BitIsOn(mask, i)) {
                client->errorValue = i;
                return BadValue;
            }
        }
    }

    return Success;
}

int _X_COLD
SProcXISelectEvents(ClientPtr client)
{
    int i;
    int len;
    xXIEventMask *evmask;

    REQUEST(xXISelectEventsReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xXISelectEventsReq);
    swapl(&stuff->win);
    swaps(&stuff->num_masks);

    len = stuff->length - bytes_to_int32(sizeof(xXISelectEventsReq));
    evmask = (xXIEventMask *) &stuff[1];
    for (i = 0; i < stuff->num_masks; i++) {
        if (len < bytes_to_int32(sizeof(xXIEventMask)))
            return BadLength;
        len -= bytes_to_int32(sizeof(xXIEventMask));
        swaps(&evmask->deviceid);
        swaps(&evmask->mask_len);
        if (len < evmask->mask_len)
            return BadLength;
        len -= evmask->mask_len;
        evmask =
            (xXIEventMask *) (((char *) &evmask[1]) + evmask->mask_len * 4);
    }

    return (ProcXISelectEvents(client));
}

int
ProcXISelectEvents(ClientPtr client)
{
    int rc, num_masks;
    WindowPtr win;
    DeviceIntPtr dev;
    DeviceIntRec dummy;
    xXIEventMask *evmask;
    int *types = NULL;
    int len;

    REQUEST(xXISelectEventsReq);
    REQUEST_AT_LEAST_SIZE(xXISelectEventsReq);

    if (stuff->num_masks == 0)
        return BadValue;

    rc = dixLookupWindow(&win, stuff->win, client, DixReceiveAccess);
    if (rc != Success)
        return rc;

    len = sz_xXISelectEventsReq;

    /* check request validity */
    evmask = (xXIEventMask *) &stuff[1];
    num_masks = stuff->num_masks;
    while (num_masks--) {
        len += sizeof(xXIEventMask) + evmask->mask_len * 4;

        if (bytes_to_int32(len) > stuff->length)
            return BadLength;

        if (evmask->deviceid != XIAllDevices &&
            evmask->deviceid != XIAllMasterDevices)
            rc = dixLookupDevice(&dev, evmask->deviceid, client, DixUseAccess);
        else {
            /* XXX: XACE here? */
        }
        if (rc != Success)
            return rc;

        /* hierarchy event mask is not allowed on devices */
        if (evmask->deviceid != XIAllDevices && evmask->mask_len >= 1) {
            unsigned char *bits = (unsigned char *) &evmask[1];

            if (BitIsOn(bits, XI_HierarchyChanged)) {
                client->errorValue = XI_HierarchyChanged;
                return BadValue;
            }
        }

        /* Raw events may only be selected on root windows */
        if (win->parent && evmask->mask_len >= 1) {
            unsigned char *bits = (unsigned char *) &evmask[1];

            if (BitIsOn(bits, XI_RawKeyPress) ||
                BitIsOn(bits, XI_RawKeyRelease) ||
                BitIsOn(bits, XI_RawButtonPress) ||
                BitIsOn(bits, XI_RawButtonRelease) ||
                BitIsOn(bits, XI_RawMotion) ||
                BitIsOn(bits, XI_RawTouchBegin) ||
                BitIsOn(bits, XI_RawTouchUpdate) ||
                BitIsOn(bits, XI_RawTouchEnd)) {
                client->errorValue = XI_RawKeyPress;
                return BadValue;
            }
        }

        if (evmask->mask_len >= 1) {
            unsigned char *bits = (unsigned char *) &evmask[1];

            /* All three touch events must be selected at once */
            if ((BitIsOn(bits, XI_TouchBegin) ||
                 BitIsOn(bits, XI_TouchUpdate) ||
                 BitIsOn(bits, XI_TouchOwnership) ||
                 BitIsOn(bits, XI_TouchEnd)) &&
                (!BitIsOn(bits, XI_TouchBegin) ||
                 !BitIsOn(bits, XI_TouchUpdate) ||
                 !BitIsOn(bits, XI_TouchEnd))) {
                client->errorValue = XI_TouchBegin;
                return BadValue;
            }

            /* All three pinch gesture events must be selected at once */
            if ((BitIsOn(bits, XI_GesturePinchBegin) ||
                 BitIsOn(bits, XI_GesturePinchUpdate) ||
                 BitIsOn(bits, XI_GesturePinchEnd)) &&
                (!BitIsOn(bits, XI_GesturePinchBegin) ||
                 !BitIsOn(bits, XI_GesturePinchUpdate) ||
                 !BitIsOn(bits, XI_GesturePinchEnd))) {
                client->errorValue = XI_GesturePinchBegin;
                return BadValue;
            }

            /* All three swipe gesture events must be selected at once. Note
               that the XI_GestureSwipeEnd is at index 32 which is on the next
               4-byte mask element */
            if (evmask->mask_len == 1 &&
                (BitIsOn(bits, XI_GestureSwipeBegin) ||
                 BitIsOn(bits, XI_GestureSwipeUpdate)))
            {
                client->errorValue = XI_GestureSwipeBegin;
                return BadValue;
            }

            if (evmask->mask_len >= 2 &&
                (BitIsOn(bits, XI_GestureSwipeBegin) ||
                 BitIsOn(bits, XI_GestureSwipeUpdate) ||
                 BitIsOn(bits, XI_GestureSwipeEnd)) &&
                (!BitIsOn(bits, XI_GestureSwipeBegin) ||
                 !BitIsOn(bits, XI_GestureSwipeUpdate) ||
                 !BitIsOn(bits, XI_GestureSwipeEnd))) {
                client->errorValue = XI_GestureSwipeBegin;
                return BadValue;
            }

            /* Only one client per window may select for touch or gesture events
             * on the same devices, including master devices.
             * XXX: This breaks if a device goes from floating to attached. */
            if (BitIsOn(bits, XI_TouchBegin)) {
                rc = check_for_touch_selection_conflicts(client,
                                                         win,
                                                         evmask->deviceid,
                                                         XI_TouchBegin);
                if (rc != Success)
                    return rc;
            }
            if (BitIsOn(bits, XI_GesturePinchBegin)) {
                rc = check_for_touch_selection_conflicts(client,
                                                         win,
                                                         evmask->deviceid,
                                                         XI_GesturePinchBegin);
                if (rc != Success)
                    return rc;
            }
            if (BitIsOn(bits, XI_GestureSwipeBegin)) {
                rc = check_for_touch_selection_conflicts(client,
                                                         win,
                                                         evmask->deviceid,
                                                         XI_GestureSwipeBegin);
                if (rc != Success)
                    return rc;
            }
        }

        if (XICheckInvalidMaskBits(client, (unsigned char *) &evmask[1],
                                   evmask->mask_len * 4) != Success)
            return BadValue;

        evmask =
            (xXIEventMask *) (((unsigned char *) evmask) +
                              evmask->mask_len * 4);
        evmask++;
    }

    if (bytes_to_int32(len) != stuff->length)
        return BadLength;

    /* Set masks on window */
    evmask = (xXIEventMask *) &stuff[1];
    num_masks = stuff->num_masks;
    while (num_masks--) {
        if (evmask->deviceid == XIAllDevices ||
            evmask->deviceid == XIAllMasterDevices) {
            dummy.id = evmask->deviceid;
            dev = &dummy;
        }
        else
            dixLookupDevice(&dev, evmask->deviceid, client, DixUseAccess);
        if (XISetEventMask(dev, win, client, evmask->mask_len * 4,
                           (unsigned char *) &evmask[1]) != Success)
            return BadAlloc;
        evmask =
            (xXIEventMask *) (((unsigned char *) evmask) +
                              evmask->mask_len * 4);
        evmask++;
    }

    RecalculateDeliverableEvents(win);

    free(types);
    return Success;
}

int _X_COLD
SProcXIGetSelectedEvents(ClientPtr client)
{
    REQUEST(xXIGetSelectedEventsReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXIGetSelectedEventsReq);
    swapl(&stuff->win);

    return (ProcXIGetSelectedEvents(client));
}

int
ProcXIGetSelectedEvents(ClientPtr client)
{
    int rc, i;
    WindowPtr win;
    char *buffer = NULL;
    xXIGetSelectedEventsReply reply;
    OtherInputMasks *masks;
    InputClientsPtr others = NULL;
    xXIEventMask *evmask = NULL;
    DeviceIntPtr dev;

    REQUEST(xXIGetSelectedEventsReq);
    REQUEST_SIZE_MATCH(xXIGetSelectedEventsReq);

    rc = dixLookupWindow(&win, stuff->win, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    reply = (xXIGetSelectedEventsReply) {
        .repType = X_Reply,
        .RepType = X_XIGetSelectedEvents,
        .sequenceNumber = client->sequence,
        .length = 0,
        .num_masks = 0
    };

    masks = wOtherInputMasks(win);
    if (masks) {
        for (others = wOtherInputMasks(win)->inputClients; others;
             others = others->next) {
            if (SameClient(others, client)) {
                break;
            }
        }
    }

    if (!others) {
        WriteReplyToClient(client, sizeof(xXIGetSelectedEventsReply), &reply);
        return Success;
    }

    buffer =
        calloc(MAXDEVICES, sizeof(xXIEventMask) + pad_to_int32(XI2MASKSIZE));
    if (!buffer)
        return BadAlloc;

    evmask = (xXIEventMask *) buffer;
    for (i = 0; i < MAXDEVICES; i++) {
        int j;
        const unsigned char *devmask = xi2mask_get_one_mask(others->xi2mask, i);

        if (i > 2) {
            rc = dixLookupDevice(&dev, i, client, DixGetAttrAccess);
            if (rc != Success)
                continue;
        }

        for (j = xi2mask_mask_size(others->xi2mask) - 1; j >= 0; j--) {
            if (devmask[j] != 0) {
                int mask_len = (j + 4) / 4;     /* j is an index, hence + 4, not + 3 */

                evmask->deviceid = i;
                evmask->mask_len = mask_len;
                reply.num_masks++;
                reply.length += sizeof(xXIEventMask) / 4 + evmask->mask_len;

                if (client->swapped) {
                    swaps(&evmask->deviceid);
                    swaps(&evmask->mask_len);
                }

                memcpy(&evmask[1], devmask, j + 1);
                evmask = (xXIEventMask *) ((char *) evmask +
                                           sizeof(xXIEventMask) + mask_len * 4);
                break;
            }
        }
    }

    WriteReplyToClient(client, sizeof(xXIGetSelectedEventsReply), &reply);

    if (reply.num_masks)
        WriteToClient(client, reply.length * 4, buffer);

    free(buffer);
    return Success;
}

void
SRepXIGetSelectedEvents(ClientPtr client,
                        int len, xXIGetSelectedEventsReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swaps(&rep->num_masks);
    WriteToClient(client, len, rep);
}
