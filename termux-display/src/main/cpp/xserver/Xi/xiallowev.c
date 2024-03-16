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
 * Request to allow some device events.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "mi.h"
#include "eventstr.h"
#include <X11/extensions/XI2.h>
#include <X11/extensions/XI2proto.h>

#include "exglobals.h"          /* BadDevice */
#include "exevents.h"
#include "xiallowev.h"

int _X_COLD
SProcXIAllowEvents(ClientPtr client)
{
    REQUEST(xXIAllowEventsReq);
    REQUEST_AT_LEAST_SIZE(xXIAllowEventsReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);
    swapl(&stuff->time);
    if (stuff->length > 3) {
        xXI2_2AllowEventsReq *req_xi22 = (xXI2_2AllowEventsReq *) stuff;

        REQUEST_AT_LEAST_SIZE(xXI2_2AllowEventsReq);
        swapl(&req_xi22->touchid);
        swapl(&req_xi22->grab_window);
    }

    return ProcXIAllowEvents(client);
}

int
ProcXIAllowEvents(ClientPtr client)
{
    TimeStamp time;
    DeviceIntPtr dev;
    int ret = Success;
    XIClientPtr xi_client;
    Bool have_xi22 = FALSE;

    REQUEST(xXI2_2AllowEventsReq);

    xi_client = dixLookupPrivate(&client->devPrivates, XIClientPrivateKey);

    if (version_compare(xi_client->major_version,
                        xi_client->minor_version, 2, 2) >= 0) {
        REQUEST_AT_LEAST_SIZE(xXI2_2AllowEventsReq);
        have_xi22 = TRUE;
    }
    else {
        REQUEST_AT_LEAST_SIZE(xXIAllowEventsReq);
    }

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixGetAttrAccess);
    if (ret != Success)
        return ret;

    time = ClientTimeToServerTime(stuff->time);

    switch (stuff->mode) {
    case XIReplayDevice:
        AllowSome(client, time, dev, NOT_GRABBED);
        break;
    case XISyncDevice:
        AllowSome(client, time, dev, FREEZE_NEXT_EVENT);
        break;
    case XIAsyncDevice:
        AllowSome(client, time, dev, THAWED);
        break;
    case XIAsyncPairedDevice:
        if (IsMaster(dev))
            AllowSome(client, time, dev, THAW_OTHERS);
        break;
    case XISyncPair:
        if (IsMaster(dev))
            AllowSome(client, time, dev, FREEZE_BOTH_NEXT_EVENT);
        break;
    case XIAsyncPair:
        if (IsMaster(dev))
            AllowSome(client, time, dev, THAWED_BOTH);
        break;
    case XIRejectTouch:
    case XIAcceptTouch:
    {
        int rc;
        WindowPtr win;

        if (!have_xi22)
            return BadValue;

        rc = dixLookupWindow(&win, stuff->grab_window, client, DixReadAccess);
        if (rc != Success)
            return rc;

        ret = TouchAcceptReject(client, dev, stuff->mode, stuff->touchid,
                                stuff->grab_window, &client->errorValue);
    }
        break;
    default:
        client->errorValue = stuff->mode;
        ret = BadValue;
    }

    return ret;
}
