/*
 * Copyright © 2009 Red Hat, Inc.
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

/***********************************************************************
 *
 * Request to grab or ungrab input device.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include <X11/extensions/XI2.h>
#include <X11/extensions/XI2proto.h>

#include "exglobals.h"          /* BadDevice */
#include "exevents.h"
#include "xigrabdev.h"
#include "inpututils.h"

int _X_COLD
SProcXIGrabDevice(ClientPtr client)
{
    REQUEST(xXIGrabDeviceReq);
    /*
     * Check here for at least the length of the struct we swap, then
     * let ProcXIGrabDevice check the full size after we swap mask_len.
     */
    REQUEST_AT_LEAST_SIZE(xXIGrabDeviceReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);
    swapl(&stuff->grab_window);
    swapl(&stuff->cursor);
    swapl(&stuff->time);
    swaps(&stuff->mask_len);

    return ProcXIGrabDevice(client);
}

int
ProcXIGrabDevice(ClientPtr client)
{
    DeviceIntPtr dev;
    xXIGrabDeviceReply rep;
    int ret = Success;
    uint8_t status;
    GrabMask mask = { 0 };
    int mask_len;
    unsigned int keyboard_mode;
    unsigned int pointer_mode;

    REQUEST(xXIGrabDeviceReq);
    REQUEST_FIXED_SIZE(xXIGrabDeviceReq, ((size_t) stuff->mask_len) * 4);

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixGrabAccess);
    if (ret != Success)
        return ret;

    if (!IsMaster(dev))
        stuff->paired_device_mode = GrabModeAsync;

    if (IsKeyboardDevice(dev)) {
        keyboard_mode = stuff->grab_mode;
        pointer_mode = stuff->paired_device_mode;
    }
    else {
        keyboard_mode = stuff->paired_device_mode;
        pointer_mode = stuff->grab_mode;
    }

    if (XICheckInvalidMaskBits(client, (unsigned char *) &stuff[1],
                               stuff->mask_len * 4) != Success)
        return BadValue;

    mask.xi2mask = xi2mask_new();
    if (!mask.xi2mask)
        return BadAlloc;

    mask_len = min(xi2mask_mask_size(mask.xi2mask), stuff->mask_len * 4);
    /* FIXME: I think the old code was broken here */
    xi2mask_set_one_mask(mask.xi2mask, dev->id, (unsigned char *) &stuff[1],
                         mask_len);

    ret = GrabDevice(client, dev, pointer_mode,
                     keyboard_mode,
                     stuff->grab_window,
                     stuff->owner_events,
                     stuff->time,
                     &mask, XI2, stuff->cursor, None /* confineTo */ ,
                     &status);

    xi2mask_free(&mask.xi2mask);

    if (ret != Success)
        return ret;

    rep = (xXIGrabDeviceReply) {
        .repType = X_Reply,
        .RepType = X_XIGrabDevice,
        .sequenceNumber = client->sequence,
        .length = 0,
        .status = status
    };

    WriteReplyToClient(client, sizeof(rep), &rep);
    return ret;
}

int _X_COLD
SProcXIUngrabDevice(ClientPtr client)
{
    REQUEST(xXIUngrabDeviceReq);
    REQUEST_SIZE_MATCH(xXIUngrabDeviceReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);
    swapl(&stuff->time);

    return ProcXIUngrabDevice(client);
}

int
ProcXIUngrabDevice(ClientPtr client)
{
    DeviceIntPtr dev;
    GrabPtr grab;
    int ret = Success;
    TimeStamp time;

    REQUEST(xXIUngrabDeviceReq);
    REQUEST_SIZE_MATCH(xXIUngrabDeviceReq);

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixGetAttrAccess);
    if (ret != Success)
        return ret;

    grab = dev->deviceGrab.grab;

    time = ClientTimeToServerTime(stuff->time);
    if ((CompareTimeStamps(time, currentTime) != LATER) &&
        (CompareTimeStamps(time, dev->deviceGrab.grabTime) != EARLIER) &&
        (grab) && SameClient(grab, client) && grab->grabtype == XI2)
        (*dev->deviceGrab.DeactivateGrab) (dev);

    return Success;
}

void _X_COLD
SRepXIGrabDevice(ClientPtr client, int size, xXIGrabDeviceReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    WriteToClient(client, size, rep);
}
