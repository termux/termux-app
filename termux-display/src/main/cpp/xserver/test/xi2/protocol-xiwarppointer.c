/**
 * Copyright © 2009 Red Hat, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

/* Test relies on assert() */
#undef NDEBUG

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

/*
 * Protocol testing for XIWarpPointer request.
 */
#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XI2proto.h>
#include "inputstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "xiwarppointer.h"
#include "exevents.h"
#include "exglobals.h"

#include "protocol-common.h"

static int expected_x = SPRITE_X;
static int expected_y = SPRITE_Y;

extern ClientRec client_window;

/**
 * This function overrides the one in the screen rec.
 */
static Bool
ScreenSetCursorPosition(DeviceIntPtr dev, ScreenPtr scr,
                        int x, int y, Bool generateEvent)
{
    assert(x == expected_x);
    assert(y == expected_y);
    return TRUE;
}

static void
request_XIWarpPointer(ClientPtr client, xXIWarpPointerReq * req, int error)
{
    int rc;

    rc = ProcXIWarpPointer(client);
    assert(rc == error);

    if (rc == BadDevice)
        assert(client->errorValue == req->deviceid);
    else if (rc == BadWindow)
        assert(client->errorValue == req->dst_win ||
               client->errorValue == req->src_win);

    client->swapped = TRUE;

    swapl(&req->src_win);
    swapl(&req->dst_win);
    swapl(&req->src_x);
    swapl(&req->src_y);
    swapl(&req->dst_x);
    swapl(&req->dst_y);
    swaps(&req->src_width);
    swaps(&req->src_height);
    swaps(&req->deviceid);

    rc = SProcXIWarpPointer(client);
    assert(rc == error);

    if (rc == BadDevice)
        assert(client->errorValue == req->deviceid);
    else if (rc == BadWindow)
        assert(client->errorValue == req->dst_win ||
               client->errorValue == req->src_win);

    client->swapped = FALSE;
}

static void
test_XIWarpPointer(void)
{
    int i;
    ClientRec client_request;
    xXIWarpPointerReq request;

    memset(&request, 0, sizeof(request));

    request_init(&request, XIWarpPointer);

    client_request = init_client(request.length, &request);

    request.deviceid = XIAllDevices;
    request_XIWarpPointer(&client_request, &request, BadDevice);

    request.deviceid = XIAllMasterDevices;
    request_XIWarpPointer(&client_request, &request, BadDevice);

    request.src_win = root.drawable.id;
    request.dst_win = root.drawable.id;
    request.deviceid = devices.vcp->id;
    request_XIWarpPointer(&client_request, &request, Success);
    request.deviceid = devices.vck->id;
    request_XIWarpPointer(&client_request, &request, BadDevice);
    request.deviceid = devices.mouse->id;
    request_XIWarpPointer(&client_request, &request, BadDevice);
    request.deviceid = devices.kbd->id;
    request_XIWarpPointer(&client_request, &request, BadDevice);

    devices.mouse->master = NULL;       /* Float, kind-of */
    request.deviceid = devices.mouse->id;
    request_XIWarpPointer(&client_request, &request, Success);

    for (i = devices.kbd->id + 1; i <= 0xFFFF; i++) {
        request.deviceid = i;
        request_XIWarpPointer(&client_request, &request, BadDevice);
    }

    request.src_win = window.drawable.id;
    request.deviceid = devices.vcp->id;
    request_XIWarpPointer(&client_request, &request, Success);

    request.deviceid = devices.mouse->id;
    request_XIWarpPointer(&client_request, &request, Success);

    request.src_win = root.drawable.id;
    request.dst_win = 0xFFFF;   /* invalid window */
    request_XIWarpPointer(&client_request, &request, BadWindow);

    request.src_win = 0xFFFF;   /* invalid window */
    request.dst_win = root.drawable.id;
    request_XIWarpPointer(&client_request, &request, BadWindow);

    request.src_win = None;
    request.dst_win = None;

    request.dst_y = 0;
    expected_y = SPRITE_Y;

    request.dst_x = 1 << 16;
    expected_x = SPRITE_X + 1;
    request.deviceid = devices.vcp->id;
    request_XIWarpPointer(&client_request, &request, Success);

    request.dst_x = -1 << 16;
    expected_x = SPRITE_X - 1;
    request.deviceid = devices.vcp->id;
    request_XIWarpPointer(&client_request, &request, Success);

    request.dst_x = 0;
    expected_x = SPRITE_X;

    request.dst_y = 1 << 16;
    expected_y = SPRITE_Y + 1;
    request.deviceid = devices.vcp->id;
    request_XIWarpPointer(&client_request, &request, Success);

    request.dst_y = -1 << 16;
    expected_y = SPRITE_Y - 1;
    request.deviceid = devices.vcp->id;
    request_XIWarpPointer(&client_request, &request, Success);

    /* FIXME: src_x/y checks */

    client_request.req_len -= 2; /* invalid length */
    request_XIWarpPointer(&client_request, &request, BadLength);
}

int
protocol_xiwarppointer_test(void)
{
    init_simple();
    screen.SetCursorPosition = ScreenSetCursorPosition;

    test_XIWarpPointer();

    return 0;
}
