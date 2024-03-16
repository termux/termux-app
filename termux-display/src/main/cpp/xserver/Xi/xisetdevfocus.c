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
/***********************************************************************
 *
 * Request to set and get an input device's focus.
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
#include "xisetdevfocus.h"

int _X_COLD
SProcXISetFocus(ClientPtr client)
{
    REQUEST(xXISetFocusReq);
    REQUEST_AT_LEAST_SIZE(xXISetFocusReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);
    swapl(&stuff->focus);
    swapl(&stuff->time);

    return ProcXISetFocus(client);
}

int _X_COLD
SProcXIGetFocus(ClientPtr client)
{
    REQUEST(xXIGetFocusReq);
    REQUEST_AT_LEAST_SIZE(xXIGetFocusReq);

    swaps(&stuff->length);
    swaps(&stuff->deviceid);

    return ProcXIGetFocus(client);
}

int
ProcXISetFocus(ClientPtr client)
{
    DeviceIntPtr dev;
    int ret;

    REQUEST(xXISetFocusReq);
    REQUEST_AT_LEAST_SIZE(xXISetFocusReq);

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixSetFocusAccess);
    if (ret != Success)
        return ret;
    if (!dev->focus)
        return BadDevice;

    return SetInputFocus(client, dev, stuff->focus, RevertToParent,
                         stuff->time, TRUE);
}

int
ProcXIGetFocus(ClientPtr client)
{
    xXIGetFocusReply rep;
    DeviceIntPtr dev;
    int ret;

    REQUEST(xXIGetFocusReq);
    REQUEST_AT_LEAST_SIZE(xXIGetFocusReq);

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixGetFocusAccess);
    if (ret != Success)
        return ret;
    if (!dev->focus)
        return BadDevice;

    rep = (xXIGetFocusReply) {
        .repType = X_Reply,
        .RepType = X_XIGetFocus,
        .sequenceNumber = client->sequence,
        .length = 0
    };

    if (dev->focus->win == NoneWin)
        rep.focus = None;
    else if (dev->focus->win == PointerRootWin)
        rep.focus = PointerRoot;
    else if (dev->focus->win == FollowKeyboardWin)
        rep.focus = FollowKeyboard;
    else
        rep.focus = dev->focus->win->drawable.id;

    WriteReplyToClient(client, sizeof(xXIGetFocusReply), &rep);
    return Success;
}

void
SRepXIGetFocus(ClientPtr client, int len, xXIGetFocusReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swapl(&rep->focus);
    WriteToClient(client, len, rep);
}
