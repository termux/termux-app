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
 * Protocol testing for XISelectEvents request.
 *
 * Test approach:
 *
 * Wrap XISetEventMask to intercept when the server tries to apply the event
 * mask. Ensure that the mask passed in is equivalent to the one supplied by
 * the client. Ensure that invalid devices and invalid masks return errors
 * as appropriate.
 *
 * Tests included:
 * BadValue for num_masks < 0
 * BadWindow for invalid windows
 * BadDevice for non-existing devices
 * BadImplemenation for devices >= 0xFF
 * BadValue if HierarchyChanged bit is set for devices other than
 *          XIAllDevices
 * BadValue for invalid mask bits
 * Sucecss for excessive mask lengths
 *
 */

#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XI2proto.h>
#include "inputstr.h"
#include "windowstr.h"
#include "extinit.h"            /* for XInputExtensionInit */
#include "scrnintstr.h"
#include "exglobals.h"
#include "xiselectev.h"

#include "protocol-common.h"

static unsigned char *data[4096 * 20];  /* the request data buffer */

extern ClientRec client_window;

int
__real_XISetEventMask(DeviceIntPtr dev, WindowPtr win, ClientPtr client,
                      int len, unsigned char *mask);

int
__wrap_XISetEventMask(DeviceIntPtr dev, WindowPtr win, ClientPtr client,
                      int len, unsigned char *mask)
{
    if (!enable_XISetEventMask_wrap)
        return __real_XISetEventMask(dev, win, client, len, mask);

    return Success;
}

static void
request_XISelectEvent(xXISelectEventsReq * req, int error)
{
    int i;
    int rc;
    ClientRec client;
    xXIEventMask *mask, *next;

    req->length = (sz_xXISelectEventsReq / 4);
    mask = (xXIEventMask *) &req[1];
    for (i = 0; i < req->num_masks; i++) {
        req->length += sizeof(xXIEventMask) / 4 + mask->mask_len;
        mask = (xXIEventMask *) ((char *) &mask[1] + mask->mask_len * 4);
    }

    client = init_client(req->length, req);

    rc = ProcXISelectEvents(&client);
    assert(rc == error);

    client.swapped = TRUE;

    mask = (xXIEventMask *) &req[1];
    for (i = 0; i < req->num_masks; i++) {
        next = (xXIEventMask *) ((char *) &mask[1] + mask->mask_len * 4);
        swaps(&mask->deviceid);
        swaps(&mask->mask_len);
        mask = next;
    }

    swapl(&req->win);
    swaps(&req->length);
    swaps(&req->num_masks);
    rc = SProcXISelectEvents(&client);
    assert(rc == error);
}

static void
_set_bit(unsigned char *bits, int bit)
{
    SetBit(bits, bit);
    if (bit >= XI_TouchBegin && bit <= XI_TouchOwnership) {
        SetBit(bits, XI_TouchBegin);
        SetBit(bits, XI_TouchUpdate);
        SetBit(bits, XI_TouchEnd);
    }
    if (bit >= XI_GesturePinchBegin && bit <= XI_GesturePinchEnd) {
        SetBit(bits, XI_GesturePinchBegin);
        SetBit(bits, XI_GesturePinchUpdate);
        SetBit(bits, XI_GesturePinchEnd);
    }
    if (bit >= XI_GestureSwipeBegin && bit <= XI_GestureSwipeEnd) {
        SetBit(bits, XI_GestureSwipeBegin);
        SetBit(bits, XI_GestureSwipeUpdate);
        SetBit(bits, XI_GestureSwipeEnd);
    }
}

static void
_clear_bit(unsigned char *bits, int bit)
{
    ClearBit(bits, bit);
    if (bit >= XI_TouchBegin && bit <= XI_TouchOwnership) {
        ClearBit(bits, XI_TouchBegin);
        ClearBit(bits, XI_TouchUpdate);
        ClearBit(bits, XI_TouchEnd);
    }
    if (bit >= XI_GesturePinchBegin && bit <= XI_GesturePinchEnd) {
        ClearBit(bits, XI_GesturePinchBegin);
        ClearBit(bits, XI_GesturePinchUpdate);
        ClearBit(bits, XI_GesturePinchEnd);
    }
    if (bit >= XI_GestureSwipeBegin && bit <= XI_GestureSwipeEnd) {
        ClearBit(bits, XI_GestureSwipeBegin);
        ClearBit(bits, XI_GestureSwipeUpdate);
        ClearBit(bits, XI_GestureSwipeEnd);
    }
}

static void
request_XISelectEvents_masks(xXISelectEventsReq * req)
{
    int i, j;
    xXIEventMask *mask;
    int nmasks = XI2MASKSIZE;
    unsigned char *bits;

    mask = (xXIEventMask *) &req[1];
    req->win = ROOT_WINDOW_ID;

    /* if a clients submits more than 100 masks, consider it insane and untested */
    for (i = 1; i <= 1000; i++) {
        req->num_masks = i;
        mask->deviceid = XIAllDevices;

        /* Test 0:
         * mask_len is 0 -> Success
         */
        mask->mask_len = 0;
        request_XISelectEvent(req, Success);

        /* Test 1:
         * mask may be larger than needed for XI2LASTEVENT.
         * Test setting each valid mask bit, while leaving unneeded bits 0.
         * -> Success
         */
        bits = (unsigned char *) &mask[1];
        mask->mask_len = (nmasks + 3) / 4 * 10;
        memset(bits, 0, mask->mask_len * 4);
        for (j = 0; j <= XI2LASTEVENT; j++) {
            _set_bit(bits, j);
            request_XISelectEvent(req, Success);
            _clear_bit(bits, j);
        }

        /* Test 2:
         * mask may be larger than needed for XI2LASTEVENT.
         * Test setting all valid mask bits, while leaving unneeded bits 0.
         * -> Success
         */
        bits = (unsigned char *) &mask[1];
        mask->mask_len = (nmasks + 3) / 4 * 10;
        memset(bits, 0, mask->mask_len * 4);

        for (j = 0; j <= XI2LASTEVENT; j++) {
            _set_bit(bits, j);
            request_XISelectEvent(req, Success);
        }

        /* Test 3:
         * mask is larger than needed for XI2LASTEVENT. If any unneeded bit
         * is set -> BadValue
         */
        bits = (unsigned char *) &mask[1];
        mask->mask_len = (nmasks + 3) / 4 * 10;
        memset(bits, 0, mask->mask_len * 4);

        for (j = XI2LASTEVENT + 1; j < mask->mask_len * 4; j++) {
            _set_bit(bits, j);
            request_XISelectEvent(req, BadValue);
            _clear_bit(bits, j);
        }

        /* Test 4:
         * Mask len is a sensible length, only valid bits are set -> Success
         */
        bits = (unsigned char *) &mask[1];
        mask->mask_len = (nmasks + 3) / 4;
        memset(bits, 0, mask->mask_len * 4);
        for (j = 0; j <= XI2LASTEVENT; j++) {
            _set_bit(bits, j);
            request_XISelectEvent(req, Success);
        }

        /* Test 5:
         * Mask len is 1 and XI_GestureSwipeEnd is set outside the mask.
         * That bit should be ignored -> Success
         */
        bits = (unsigned char *) &mask[1];
        mask->mask_len = 1;
        memset(bits, 0, 5);
        SetBit(bits, XI_ButtonPress); // does not matter which one
        SetBit(bits, XI_GestureSwipeEnd);
        request_XISelectEvent(req, Success);

        /* Test 6:
         * HierarchyChanged bit is BadValue for devices other than
         * XIAllDevices
         */
        bits = (unsigned char *) &mask[1];
        mask->mask_len = (nmasks + 3) / 4;
        memset(bits, 0, mask->mask_len * 4);
        SetBit(bits, XI_HierarchyChanged);
        mask->deviceid = XIAllDevices;
        request_XISelectEvent(req, Success);
        for (j = 1; j < devices.num_devices; j++) {
            mask->deviceid = j;
            request_XISelectEvent(req, BadValue);
        }

        /* Test 7:
         * All bits set minus hierarchy changed bit -> Success
         */
        bits = (unsigned char *) &mask[1];
        mask->mask_len = (nmasks + 3) / 4;
        memset(bits, 0, mask->mask_len * 4);
        for (j = 0; j <= XI2LASTEVENT; j++)
            _set_bit(bits, j);
        _clear_bit(bits, XI_HierarchyChanged);
        for (j = 1; j < 6; j++) {
            mask->deviceid = j;
            request_XISelectEvent(req, Success);
        }

        mask =
            (xXIEventMask *) ((char *) mask + sizeof(xXIEventMask) +
                              mask->mask_len * 4);
    }
}

static void
test_XISelectEvents(void)
{
    int i;
    xXIEventMask *mask;
    xXISelectEventsReq *req;

    req = (xXISelectEventsReq *) data;

    request_init(req, XISelectEvents);

    printf("Testing for BadValue on zero-length masks\n");
    /* zero masks are BadValue, regardless of the window */
    req->num_masks = 0;

    req->win = None;
    request_XISelectEvent(req, BadValue);

    req->win = ROOT_WINDOW_ID;
    request_XISelectEvent(req, BadValue);

    req->win = CLIENT_WINDOW_ID;
    request_XISelectEvent(req, BadValue);

    printf("Testing for BadWindow.\n");
    /* None window is BadWindow, regardless of the masks.
     * We don't actually need to set the masks here, BadWindow must occur
     * before checking the masks.
     */
    req->win = None;
    req->num_masks = 1;
    request_XISelectEvent(req, BadWindow);

    req->num_masks = 2;
    request_XISelectEvent(req, BadWindow);

    req->num_masks = 0xFF;
    request_XISelectEvent(req, BadWindow);

    /* request size is 3, so 0xFFFC is the highest num_mask that doesn't
     * overflow req->length */
    req->num_masks = 0xFFFC;
    request_XISelectEvent(req, BadWindow);

    printf("Triggering num_masks/length overflow\n");
    req->win = ROOT_WINDOW_ID;
    /* Integer overflow - req->length can't hold that much */
    req->num_masks = 0xFFFF;
    request_XISelectEvent(req, BadLength);

    req->win = ROOT_WINDOW_ID;
    req->num_masks = 1;

    printf("Triggering bogus mask length error\n");
    mask = (xXIEventMask *) &req[1];
    mask->deviceid = 0;
    mask->mask_len = 0xFFFF;
    request_XISelectEvent(req, BadLength);

    /* testing various device ids */
    printf("Testing existing device ids.\n");
    for (i = 0; i < 6; i++) {
        mask = (xXIEventMask *) &req[1];
        mask->deviceid = i;
        mask->mask_len = 1;
        req->win = ROOT_WINDOW_ID;
        req->num_masks = 1;
        request_XISelectEvent(req, Success);
    }

    printf("Testing non-existing device ids.\n");
    for (i = 6; i <= 0xFFFF; i++) {
        req->win = ROOT_WINDOW_ID;
        req->num_masks = 1;
        mask = (xXIEventMask *) &req[1];
        mask->deviceid = i;
        mask->mask_len = 1;
        request_XISelectEvent(req, BadDevice);
    }

    request_XISelectEvents_masks(req);
}

int
protocol_xiselectevents_test(void)
{
    init_simple();

    test_XISelectEvents();

    return 0;
}
