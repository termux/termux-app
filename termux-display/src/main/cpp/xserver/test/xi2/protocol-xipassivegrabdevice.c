/**
 * Copyright © 2011 Red Hat, Inc.
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
 * Protocol testing for XIPassiveGrab request.
 */
#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XI2proto.h>
#include "inputstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "xipassivegrab.h"
#include "exevents.h"
#include "exglobals.h"

#include "protocol-common.h"

extern ClientRec client_window;
static ClientRec client_request;

#define N_MODS 7
static uint32_t modifiers[N_MODS] = { 1, 2, 3, 4, 5, 6, 7 };

static struct test_data {
    int num_modifiers;
} testdata;

int __wrap_GrabButton(ClientPtr client, DeviceIntPtr dev,
                      DeviceIntPtr modifier_device, int button,
                      GrabParameters *param, enum InputLevel grabtype,
                      GrabMask *mask);
int __real_GrabButton(ClientPtr client, DeviceIntPtr dev,
                      DeviceIntPtr modifier_device, int button,
                      GrabParameters *param, enum InputLevel grabtype,
                      GrabMask *mask);
static void reply_XIPassiveGrabDevice_data(ClientPtr client, int len,
                                           char *data, void *closure);

int
__wrap_GrabButton(ClientPtr client, DeviceIntPtr dev,
                  DeviceIntPtr modifier_device, int button,
                  GrabParameters *param, enum InputLevel grabtype,
                  GrabMask *mask)
{
    if (!enable_GrabButton_wrap)
        __real_GrabButton(client, dev, modifier_device, button, param, grabtype, mask);

    /* Fail every odd modifier */
    if (param->modifiers % 2)
        return BadAccess;

    return Success;
}

static void
reply_XIPassiveGrabDevice(ClientPtr client, int len, char *data, void *closure)
{
    xXIPassiveGrabDeviceReply *rep = (xXIPassiveGrabDeviceReply *) data;

    if (client->swapped) {
        swaps(&rep->sequenceNumber);
        swapl(&rep->length);
        swaps(&rep->num_modifiers);

        testdata.num_modifiers = rep->num_modifiers;
    }

    reply_check_defaults(rep, len, XIPassiveGrabDevice);

    /* ProcXIPassiveGrabDevice sends the data in two batches, let the second
     * handler handle the modifier data */
    if (rep->num_modifiers > 0)
        reply_handler = reply_XIPassiveGrabDevice_data;
}

static void
reply_XIPassiveGrabDevice_data(ClientPtr client, int len, char *data,
                               void *closure)
{
    int i;

    xXIGrabModifierInfo *mods = (xXIGrabModifierInfo *) data;

    for (i = 0; i < testdata.num_modifiers; i++, mods++) {
        if (client->swapped)
            swapl(&mods->modifiers);

        /* 1 - 7 is the range we use for the global modifiers array
         * above */
        assert(mods->modifiers > 0);
        assert(mods->modifiers <= 7);
        assert(mods->modifiers % 2 == 1);       /* because we fail odd ones */
        assert(mods->status != Success);
        assert(mods->pad0 == 0);
        assert(mods->pad1 == 0);
    }

    reply_handler = reply_XIPassiveGrabDevice;
}

static void
request_XIPassiveGrabDevice(ClientPtr client, xXIPassiveGrabDeviceReq * req,
                            int error, int errval)
{
    int rc;
    int local_modifiers;
    int mask_len;

    client_request.req_len = req->length;
    rc = ProcXIPassiveGrabDevice(&client_request);
    assert(rc == error);

    if (rc != Success)
        assert(client_request.errorValue == errval);

    client_request.swapped = TRUE;
    swaps(&req->length);
    swapl(&req->time);
    swapl(&req->grab_window);
    swapl(&req->cursor);
    swapl(&req->detail);
    swaps(&req->deviceid);
    local_modifiers = req->num_modifiers;
    swaps(&req->num_modifiers);
    mask_len = req->mask_len;
    swaps(&req->mask_len);

    while (local_modifiers--) {
        CARD32 *mod = (CARD32 *) (req + 1) + mask_len + local_modifiers;

        swapl(mod);
    }

    rc = SProcXIPassiveGrabDevice(&client_request);
    assert(rc == error);

    if (rc != Success)
        assert(client_request.errorValue == errval);
}

static unsigned char *data[4096];       /* the request buffer */
static void
test_XIPassiveGrabDevice(void)
{
    int i;
    xXIPassiveGrabDeviceReq *request = (xXIPassiveGrabDeviceReq *) data;
    unsigned char *mask;

    request_init(request, XIPassiveGrabDevice);

    request->grab_window = CLIENT_WINDOW_ID;

    reply_handler = reply_XIPassiveGrabDevice;
    client_request = init_client(request->length, request);

    printf("Testing invalid device\n");
    request->deviceid = 12;
    request_XIPassiveGrabDevice(&client_request, request, BadDevice,
                                request->deviceid);

    printf("Testing invalid length\n");
    request->length -= 2;
    request_XIPassiveGrabDevice(&client_request, request, BadLength,
                                client_request.errorValue);
    /* re-init request since swapped length test leaves some values swapped */
    request_init(request, XIPassiveGrabDevice);
    request->grab_window = CLIENT_WINDOW_ID;
    request->deviceid = XIAllMasterDevices;

    printf("Testing invalid grab types\n");
    for (i = XIGrabtypeGestureSwipeBegin + 1; i < 0xFF; i++) {
        request->grab_type = i;
        request_XIPassiveGrabDevice(&client_request, request, BadValue,
                                    request->grab_type);
    }

    printf("Testing invalid grab type + detail combinations\n");
    request->grab_type = XIGrabtypeEnter;
    request->detail = 1;
    request_XIPassiveGrabDevice(&client_request, request, BadValue,
                                request->detail);

    request->grab_type = XIGrabtypeFocusIn;
    request_XIPassiveGrabDevice(&client_request, request, BadValue,
                                request->detail);

    request->detail = 0;

    printf("Testing invalid masks\n");
    mask = (unsigned char *) &request[1];

    request->mask_len = bytes_to_int32(XI2LASTEVENT + 1);
    request->length += request->mask_len;
    SetBit(mask, XI2LASTEVENT + 1);
    request_XIPassiveGrabDevice(&client_request, request, BadValue,
                                XI2LASTEVENT + 1);

    ClearBit(mask, XI2LASTEVENT + 1);

    /* tested all special cases now, test a few valid cases */

    /* no modifiers */
    request->deviceid = XIAllDevices;
    request->grab_type = XIGrabtypeButton;
    request->detail = XIAnyButton;
    request_XIPassiveGrabDevice(&client_request, request, Success, 0);

    /* Set a few random masks to make sure we handle modifiers correctly */
    SetBit(mask, XI_ButtonPress);
    SetBit(mask, XI_KeyPress);
    SetBit(mask, XI_Enter);

    /* some modifiers */
    request->num_modifiers = N_MODS;
    request->length += N_MODS;
    memcpy((uint32_t *) (request + 1) + request->mask_len, modifiers,
           sizeof(modifiers));
    request_XIPassiveGrabDevice(&client_request, request, Success, 0);
}

int
protocol_xipassivegrabdevice_test(void)
{
    init_simple();

    test_XIPassiveGrabDevice();

    return 0;
}
