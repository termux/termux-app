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
 * Protocol testing for XIGetSelectedEvents request.
 *
 * Tests include:
 * BadWindow on wrong window.
 * Zero-length masks if no masks are set.
 * Valid masks for valid devices.
 * Masks set on non-existent devices are not returned.
 *
 * Note that this test is not connected to the XISelectEvents request.
 */
#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XI2proto.h>
#include "inputstr.h"
#include "windowstr.h"
#include "extinit.h"            /* for XInputExtensionInit */
#include "scrnintstr.h"
#include "xiselectev.h"
#include "exevents.h"

#include "protocol-common.h"

static void reply_XIGetSelectedEvents(ClientPtr client, int len, char *data,
                                      void *userdata);
static void reply_XIGetSelectedEvents_data(ClientPtr client, int len,
                                           char *data, void *userdata);

static struct {
    int num_masks_expected;
    unsigned char mask[MAXDEVICES][XI2LASTEVENT];       /* intentionally bigger */
    int mask_len;
} test_data;

extern ClientRec client_window;

/* AddResource is called from XISetSEventMask, we don't need this */
Bool
__wrap_AddResource(XID id, RESTYPE type, void *value)
{
    return TRUE;
}

static void
reply_XIGetSelectedEvents(ClientPtr client, int len, char *data, void *userdata)
{
    xXIGetSelectedEventsReply *rep = (xXIGetSelectedEventsReply *) data;

    if (client->swapped) {
        swapl(&rep->length);
        swaps(&rep->sequenceNumber);
        swaps(&rep->num_masks);
    }

    reply_check_defaults(rep, len, XIGetSelectedEvents);

    assert(rep->num_masks == test_data.num_masks_expected);

    reply_handler = reply_XIGetSelectedEvents_data;
}

static void
reply_XIGetSelectedEvents_data(ClientPtr client, int len, char *data,
                               void *userdata)
{
    int i;
    xXIEventMask *mask;
    unsigned char *bitmask;

    mask = (xXIEventMask *) data;
    for (i = 0; i < test_data.num_masks_expected; i++) {
        if (client->swapped) {
            swaps(&mask->deviceid);
            swaps(&mask->mask_len);
        }

        assert(mask->deviceid < 6);
        assert(mask->mask_len <= (((XI2LASTEVENT + 8) / 8) + 3) / 4);

        bitmask = (unsigned char *) &mask[1];
        assert(memcmp(bitmask,
                      test_data.mask[mask->deviceid], mask->mask_len * 4) == 0);

        mask =
            (xXIEventMask *) ((char *) mask + mask->mask_len * 4 +
                              sizeof(xXIEventMask));
    }

}

static void
request_XIGetSelectedEvents(xXIGetSelectedEventsReq * req, int error)
{
    int rc;
    ClientRec client;

    client = init_client(req->length, req);

    reply_handler = reply_XIGetSelectedEvents;

    rc = ProcXIGetSelectedEvents(&client);
    assert(rc == error);

    reply_handler = reply_XIGetSelectedEvents;
    client.swapped = TRUE;
    swapl(&req->win);
    swaps(&req->length);
    rc = SProcXIGetSelectedEvents(&client);
    assert(rc == error);
}

static void
test_XIGetSelectedEvents(void)
{
    int i, j;
    xXIGetSelectedEventsReq request;
    ClientRec client = init_client(0, NULL);
    unsigned char *mask;
    DeviceIntRec dev;

    request_init(&request, XIGetSelectedEvents);

    printf("Testing for BadWindow on invalid window.\n");
    request.win = None;
    request_XIGetSelectedEvents(&request, BadWindow);

    printf("Testing for zero-length (unset) masks.\n");
    /* No masks set yet */
    test_data.num_masks_expected = 0;
    request.win = ROOT_WINDOW_ID;
    request_XIGetSelectedEvents(&request, Success);

    request.win = CLIENT_WINDOW_ID;
    request_XIGetSelectedEvents(&request, Success);

    memset(test_data.mask, 0, sizeof(test_data.mask));

    printf("Testing for valid masks\n");
    memset(&dev, 0, sizeof(dev));       /* dev->id is enough for XISetEventMask */
    request.win = ROOT_WINDOW_ID;

    /* devices 6 - MAXDEVICES don't exist, they mustn't be included in the
     * reply even if a mask is set */
    for (j = 0; j < MAXDEVICES; j++) {
        test_data.num_masks_expected = min(j + 1, devices.num_devices + 2);
        dev.id = j;
        mask = test_data.mask[j];
        /* bits one-by-one */
        for (i = 0; i < XI2LASTEVENT; i++) {
            SetBit(mask, i);
            XISetEventMask(&dev, &root, &client, (i + 8) / 8, mask);
            request_XIGetSelectedEvents(&request, Success);
            ClearBit(mask, i);
        }

        /* all valid mask bits */
        for (i = 0; i < XI2LASTEVENT; i++) {
            SetBit(mask, i);
            XISetEventMask(&dev, &root, &client, (i + 8) / 8, mask);
            request_XIGetSelectedEvents(&request, Success);
        }
    }

    printf("Testing removing all masks\n");
    /* Unset all masks one-by-one */
    for (j = MAXDEVICES - 1; j >= 0; j--) {
        if (j < devices.num_devices + 2)
            test_data.num_masks_expected--;

        mask = test_data.mask[j];
        memset(mask, 0, XI2LASTEVENT);

        dev.id = j;
        XISetEventMask(&dev, &root, &client, 0, NULL);

        request_XIGetSelectedEvents(&request, Success);
    }
}

int
protocol_xigetselectedevents_test(void)
{
    init_simple();
    enable_GrabButton_wrap = 0;
    enable_XISetEventMask_wrap = 0;

    test_XIGetSelectedEvents();

    return 0;
}
