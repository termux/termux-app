/**
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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
 */

/* Test relies on assert() */
#undef NDEBUG

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

/*
 * Protocol testing for ChangeDeviceControl request.
 */
#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XIproto.h>
#include "inputstr.h"
#include "chgdctl.h"

#include "protocol-common.h"

extern ClientRec client_window;
static ClientRec client_request;

static void
reply_ChangeDeviceControl(ClientPtr client, int len, char *data, void *userdata)
{
    xChangeDeviceControlReply *rep = (xChangeDeviceControlReply *) data;

    if (client->swapped) {
        swapl(&rep->length);
        swaps(&rep->sequenceNumber);
    }

    reply_check_defaults(rep, len, ChangeDeviceControl);

    /* XXX: check status code in reply */
}

static void
request_ChangeDeviceControl(ClientPtr client, xChangeDeviceControlReq * req,
                            xDeviceCtl *ctl, int error)
{
    int rc;

    client_request.req_len = req->length;
    rc = ProcXChangeDeviceControl(&client_request);
    assert(rc == error);

    /* XXX: ChangeDeviceControl doesn't seem to fill in errorValue to check */

    client_request.swapped = TRUE;
    swaps(&req->length);
    swaps(&req->control);
    swaps(&ctl->length);
    swaps(&ctl->control);
    /* XXX: swap other contents of ctl, depending on type */
    rc = SProcXChangeDeviceControl(&client_request);
    assert(rc == error);
}

static unsigned char *data[4096];       /* the request buffer */

static void
test_ChangeDeviceControl(void)
{
    xChangeDeviceControlReq *request = (xChangeDeviceControlReq *) data;
    xDeviceCtl *control = (xDeviceCtl *) (&request[1]);

    request_init(request, ChangeDeviceControl);

    reply_handler = reply_ChangeDeviceControl;

    client_request = init_client(request->length, request);

    printf("Testing invalid lengths:\n");
    printf(" -- no control struct\n");
    request_ChangeDeviceControl(&client_request, request, control, BadLength);

    printf(" -- xDeviceResolutionCtl\n");
    request_init(request, ChangeDeviceControl);
    request->control = DEVICE_RESOLUTION;
    control->length = (sizeof(xDeviceResolutionCtl) >> 2);
    request->length += control->length - 2;
    request_ChangeDeviceControl(&client_request, request, control, BadLength);

    printf(" -- xDeviceEnableCtl\n");
    request_init(request, ChangeDeviceControl);
    request->control = DEVICE_ENABLE;
    control->length = (sizeof(xDeviceEnableCtl) >> 2);
    request->length += control->length - 2;
    request_ChangeDeviceControl(&client_request, request, control, BadLength);

    /* XXX: Test functionality! */
}

int
protocol_xchangedevicecontrol_test(void)
{
    init_simple();

    test_ChangeDeviceControl();

    return 0;
}
