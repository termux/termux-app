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

#include <stdint.h>
#include <X11/X.h>
#include "misc.h"
#include "resource.h"
#include <X11/Xproto.h>
#include <X11/extensions/XI2proto.h>
#include <X11/Xatom.h>
#include "windowstr.h"
#include "inputstr.h"
#include "eventconvert.h"
#include "exevents.h"
#include "exglobals.h"
#include "dixgrabs.h"
#include "eventstr.h"
#include "inpututils.h"
#include "mi.h"
#include "assert.h"

#include "tests-common.h"

/**
 * Init a device with axes.
 * Verify values set on the device.
 *
 * Result: All axes set to default values (usually 0).
 */
static void
dix_init_valuators(void)
{
    DeviceIntRec dev;
    ValuatorClassPtr val;
    AxisInfoPtr axis;
    const int num_axes = 2;
    int i;
    Atom atoms[MAX_VALUATORS] = { 0 };

    memset(&dev, 0, sizeof(DeviceIntRec));
    dev.type = MASTER_POINTER;  /* claim it's a master to stop ptracccel */

    assert(InitValuatorClassDeviceStruct(NULL, 0, atoms, 0, 0) == FALSE);
    assert(InitValuatorClassDeviceStruct(&dev, num_axes, atoms, 0, Absolute));

    val = dev.valuator;
    assert(val);
    assert(val->numAxes == num_axes);
    assert(val->numMotionEvents == 0);
    assert(val->axisVal);

    for (i = 0; i < num_axes; i++) {
        assert(val->axisVal[i] == 0);
        assert(val->axes->min_value == NO_AXIS_LIMITS);
        assert(val->axes->max_value == NO_AXIS_LIMITS);
        assert(val->axes->mode == Absolute);
    }

    assert(dev.last.numValuators == num_axes);

    /* invalid increment */
    assert(SetScrollValuator
           (&dev, 0, SCROLL_TYPE_VERTICAL, 0.0, SCROLL_FLAG_NONE) == FALSE);
    /* invalid type */
    assert(SetScrollValuator
           (&dev, 0, SCROLL_TYPE_VERTICAL - 1, 1.0, SCROLL_FLAG_NONE) == FALSE);
    assert(SetScrollValuator
           (&dev, 0, SCROLL_TYPE_HORIZONTAL + 1, 1.0,
            SCROLL_FLAG_NONE) == FALSE);
    /* invalid axisnum */
    assert(SetScrollValuator
           (&dev, 2, SCROLL_TYPE_HORIZONTAL, 1.0, SCROLL_FLAG_NONE) == FALSE);

    /* valid */
    assert(SetScrollValuator
           (&dev, 0, SCROLL_TYPE_VERTICAL, 3.0, SCROLL_FLAG_NONE) == TRUE);
    axis = &dev.valuator->axes[0];
    assert(axis->scroll.increment == 3.0);
    assert(axis->scroll.type == SCROLL_TYPE_VERTICAL);
    assert(axis->scroll.flags == 0);

    /* valid */
    assert(SetScrollValuator
           (&dev, 1, SCROLL_TYPE_HORIZONTAL, 2.0, SCROLL_FLAG_NONE) == TRUE);
    axis = &dev.valuator->axes[1];
    assert(axis->scroll.increment == 2.0);
    assert(axis->scroll.type == SCROLL_TYPE_HORIZONTAL);
    assert(axis->scroll.flags == 0);

    /* can add another non-preffered axis */
    assert(SetScrollValuator
           (&dev, 1, SCROLL_TYPE_VERTICAL, 5.0, SCROLL_FLAG_NONE) == TRUE);
    assert(SetScrollValuator
           (&dev, 0, SCROLL_TYPE_HORIZONTAL, 5.0, SCROLL_FLAG_NONE) == TRUE);

    /* can overwrite with Preferred */
    assert(SetScrollValuator
           (&dev, 1, SCROLL_TYPE_VERTICAL, 5.5, SCROLL_FLAG_PREFERRED) == TRUE);
    axis = &dev.valuator->axes[1];
    assert(axis->scroll.increment == 5.5);
    assert(axis->scroll.type == SCROLL_TYPE_VERTICAL);
    assert(axis->scroll.flags == SCROLL_FLAG_PREFERRED);

    assert(SetScrollValuator
           (&dev, 0, SCROLL_TYPE_HORIZONTAL, 8.8,
            SCROLL_FLAG_PREFERRED) == TRUE);
    axis = &dev.valuator->axes[0];
    assert(axis->scroll.increment == 8.8);
    assert(axis->scroll.type == SCROLL_TYPE_HORIZONTAL);
    assert(axis->scroll.flags == SCROLL_FLAG_PREFERRED);

    /* can overwrite as none */
    assert(SetScrollValuator(&dev, 0, SCROLL_TYPE_NONE, 5.0,
                             SCROLL_FLAG_NONE) == TRUE);
    axis = &dev.valuator->axes[0];
    assert(axis->scroll.type == SCROLL_TYPE_NONE);

    /* can overwrite axis with new settings */
    assert(SetScrollValuator
           (&dev, 0, SCROLL_TYPE_VERTICAL, 5.0, SCROLL_FLAG_NONE) == TRUE);
    axis = &dev.valuator->axes[0];
    assert(axis->scroll.type == SCROLL_TYPE_VERTICAL);
    assert(axis->scroll.increment == 5.0);
    assert(axis->scroll.flags == SCROLL_FLAG_NONE);
    assert(SetScrollValuator
           (&dev, 0, SCROLL_TYPE_VERTICAL, 3.0, SCROLL_FLAG_NONE) == TRUE);
    assert(axis->scroll.type == SCROLL_TYPE_VERTICAL);
    assert(axis->scroll.increment == 3.0);
    assert(axis->scroll.flags == SCROLL_FLAG_NONE);
}

/* just check the known success cases, and that error cases set the client's
 * error value correctly. */
static void
dix_check_grab_values(void)
{
    ClientRec client;
    GrabParameters param;
    int rc;

    memset(&client, 0, sizeof(client));

    param.grabtype = CORE;
    param.this_device_mode = GrabModeSync;
    param.other_devices_mode = GrabModeSync;
    param.modifiers = AnyModifier;
    param.ownerEvents = FALSE;

    rc = CheckGrabValues(&client, &param);
    assert(rc == Success);

    param.this_device_mode = GrabModeAsync;
    rc = CheckGrabValues(&client, &param);
    assert(rc == Success);

    param.this_device_mode = XIGrabModeTouch;
    rc = CheckGrabValues(&client, &param);
    assert(rc == Success);

    param.this_device_mode = XIGrabModeTouch + 1;
    rc = CheckGrabValues(&client, &param);
    assert(rc == BadValue);
    assert(client.errorValue == param.this_device_mode);
    assert(client.errorValue == XIGrabModeTouch + 1);

    param.this_device_mode = GrabModeSync;
    param.other_devices_mode = GrabModeAsync;
    rc = CheckGrabValues(&client, &param);

    param.this_device_mode = GrabModeSync;
    param.other_devices_mode = XIGrabModeTouch;
    rc = CheckGrabValues(&client, &param);
    assert(rc == Success);
    assert(rc == Success);

    param.other_devices_mode = XIGrabModeTouch + 1;
    rc = CheckGrabValues(&client, &param);
    assert(rc == BadValue);
    assert(client.errorValue == param.other_devices_mode);
    assert(client.errorValue == XIGrabModeTouch + 1);

    param.other_devices_mode = GrabModeSync;

    param.modifiers = 1 << 13;
    rc = CheckGrabValues(&client, &param);
    assert(rc == BadValue);
    assert(client.errorValue == param.modifiers);
    assert(client.errorValue == (1 << 13));

    param.modifiers = AnyModifier;
    param.ownerEvents = TRUE;
    rc = CheckGrabValues(&client, &param);
    assert(rc == Success);

    param.ownerEvents = 3;
    rc = CheckGrabValues(&client, &param);
    assert(rc == BadValue);
    assert(client.errorValue == param.ownerEvents);
    assert(client.errorValue == 3);
}

/**
 * Convert various internal events to the matching core event and verify the
 * parameters.
 */
static void
dix_event_to_core(int type)
{
    DeviceEvent ev = {};
    xEvent *core;
    int time;
    int x, y;
    int rc;
    int state;
    int detail;
    int count;
    const int ROOT_WINDOW_ID = 0x100;

    /* EventToCore memsets the event to 0 */
#define test_event() \
    assert(rc == Success); \
    assert(core); \
    assert(count == 1); \
    assert(core->u.u.type == type); \
    assert(core->u.u.detail == detail); \
    assert(core->u.keyButtonPointer.time == time); \
    assert(core->u.keyButtonPointer.rootX == x); \
    assert(core->u.keyButtonPointer.rootY == y); \
    assert(core->u.keyButtonPointer.state == state); \
    assert(core->u.keyButtonPointer.eventX == 0); \
    assert(core->u.keyButtonPointer.eventY == 0); \
    assert(core->u.keyButtonPointer.root == ROOT_WINDOW_ID); \
    assert(core->u.keyButtonPointer.event == 0); \
    assert(core->u.keyButtonPointer.child == 0); \
    assert(core->u.keyButtonPointer.sameScreen == FALSE);

    x = 0;
    y = 0;
    time = 12345;
    state = 0;
    detail = 0;

    ev.header = 0xFF;
    ev.length = sizeof(DeviceEvent);
    ev.time = time;
    ev.root_y = x;
    ev.root_x = y;
    SetBit(ev.valuators.mask, 0);
    SetBit(ev.valuators.mask, 1);
    ev.root = ROOT_WINDOW_ID;
    ev.corestate = state;
    ev.detail.key = detail;

    ev.type = type;
    ev.detail.key = 0;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    test_event();

    x = 1;
    y = 2;
    ev.root_x = x;
    ev.root_y = y;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    test_event();

    x = 0x7FFF;
    y = 0x7FFF;
    ev.root_x = x;
    ev.root_y = y;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    test_event();

    x = 0x8000;                 /* too high */
    y = 0x8000;                 /* too high */
    ev.root_x = x;
    ev.root_y = y;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    assert(rc == Success);
    assert(core);
    assert(count == 1);
    assert(core->u.keyButtonPointer.rootX != x);
    assert(core->u.keyButtonPointer.rootY != y);

    x = 0x7FFF;
    y = 0x7FFF;
    ev.root_x = x;
    ev.root_y = y;
    time = 0;
    ev.time = time;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    test_event();

    detail = 1;
    ev.detail.key = detail;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    test_event();

    detail = 0xFF;              /* highest value */
    ev.detail.key = detail;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    test_event();

    detail = 0xFFF;             /* too big */
    ev.detail.key = detail;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    assert(rc == BadMatch);

    detail = 0xFF;              /* too big */
    ev.detail.key = detail;
    state = 0xFFFF;             /* highest value */
    ev.corestate = state;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    test_event();

    state = 0x10000;            /* too big */
    ev.corestate = state;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    assert(rc == Success);
    assert(core);
    assert(count == 1);
    assert(core->u.keyButtonPointer.state != state);
    assert(core->u.keyButtonPointer.state == (state & 0xFFFF));

#undef test_event
}

static void
dix_event_to_core_fail(int evtype, int expected_rc)
{
    DeviceEvent ev;
    xEvent *core;
    int rc;
    int count;

    ev.header = 0xFF;
    ev.length = sizeof(DeviceEvent);

    ev.type = evtype;
    rc = EventToCore((InternalEvent *) &ev, &core, &count);
    assert(rc == expected_rc);
}

static void
dix_event_to_core_conversion(void)
{
    dix_event_to_core_fail(0, BadImplementation);
    dix_event_to_core_fail(1, BadImplementation);
    dix_event_to_core_fail(ET_ProximityOut + 1, BadImplementation);
    dix_event_to_core_fail(ET_ProximityIn, BadMatch);
    dix_event_to_core_fail(ET_ProximityOut, BadMatch);

    dix_event_to_core(ET_KeyPress);
    dix_event_to_core(ET_KeyRelease);
    dix_event_to_core(ET_ButtonPress);
    dix_event_to_core(ET_ButtonRelease);
    dix_event_to_core(ET_Motion);
}

static void
_dix_test_xi_convert(DeviceEvent *ev, int expected_rc, int expected_count)
{
    xEvent *xi;
    int count = 0;
    int rc;

    rc = EventToXI((InternalEvent *) ev, &xi, &count);
    assert(rc == expected_rc);
    assert(count >= expected_count);
    if (count > 0) {
        deviceKeyButtonPointer *kbp = (deviceKeyButtonPointer *) xi;

        assert(kbp->type == IEventBase + ev->type);
        assert(kbp->detail == ev->detail.key);
        assert(kbp->time == ev->time);
        assert((kbp->deviceid & ~MORE_EVENTS) == ev->deviceid);
        assert(kbp->root_x == ev->root_x);
        assert(kbp->root_y == ev->root_y);
        assert(kbp->state == ev->corestate);
        assert(kbp->event_x == 0);
        assert(kbp->event_y == 0);
        assert(kbp->root == ev->root);
        assert(kbp->event == 0);
        assert(kbp->child == 0);
        assert(kbp->same_screen == FALSE);

        while (--count > 0) {
            deviceValuator *v = (deviceValuator *) &xi[count];

            assert(v->type == DeviceValuator);
            assert(v->num_valuators <= 6);
        }

        free(xi);
    }
}

/**
 * This tests for internal event → XI1 event conversion
 * - all conversions should generate the right XI event type
 * - right number of events generated
 * - extra events are valuators
 */
static void
dix_event_to_xi1_conversion(void)
{
    DeviceEvent ev = { 0 };
    int time;
    int x, y;
    int state;
    int detail;
    const int ROOT_WINDOW_ID = 0x100;
    int deviceid;

    IEventBase = 80;
    DeviceValuator = IEventBase - 1;
    DeviceKeyPress = IEventBase + ET_KeyPress;
    DeviceKeyRelease = IEventBase + ET_KeyRelease;
    DeviceButtonPress = IEventBase + ET_ButtonPress;
    DeviceButtonRelease = IEventBase + ET_ButtonRelease;
    DeviceMotionNotify = IEventBase + ET_Motion;
    DeviceFocusIn = IEventBase + ET_FocusIn;
    DeviceFocusOut = IEventBase + ET_FocusOut;
    ProximityIn = IEventBase + ET_ProximityIn;
    ProximityOut = IEventBase + ET_ProximityOut;

    /* EventToXI callocs */
    x = 0;
    y = 0;
    time = 12345;
    state = 0;
    detail = 0;
    deviceid = 4;

    ev.header = 0xFF;

    ev.header = 0xFF;
    ev.length = sizeof(DeviceEvent);
    ev.time = time;
    ev.root_y = x;
    ev.root_x = y;
    SetBit(ev.valuators.mask, 0);
    SetBit(ev.valuators.mask, 1);
    ev.root = ROOT_WINDOW_ID;
    ev.corestate = state;
    ev.detail.key = detail;
    ev.deviceid = deviceid;

    /* test all types for bad match */
    ev.type = ET_KeyPress;
    _dix_test_xi_convert(&ev, Success, 1);
    ev.type = ET_KeyRelease;
    _dix_test_xi_convert(&ev, Success, 1);
    ev.type = ET_ButtonPress;
    _dix_test_xi_convert(&ev, Success, 1);
    ev.type = ET_ButtonRelease;
    _dix_test_xi_convert(&ev, Success, 1);
    ev.type = ET_Motion;
    _dix_test_xi_convert(&ev, Success, 1);
    ev.type = ET_ProximityIn;
    _dix_test_xi_convert(&ev, Success, 1);
    ev.type = ET_ProximityOut;
    _dix_test_xi_convert(&ev, Success, 1);

    /* No axes */
    ClearBit(ev.valuators.mask, 0);
    ClearBit(ev.valuators.mask, 1);
    ev.type = ET_KeyPress;
    _dix_test_xi_convert(&ev, Success, 1);
    ev.type = ET_KeyRelease;
    _dix_test_xi_convert(&ev, Success, 1);
    ev.type = ET_ButtonPress;
    _dix_test_xi_convert(&ev, Success, 1);
    ev.type = ET_ButtonRelease;
    _dix_test_xi_convert(&ev, Success, 1);
    ev.type = ET_Motion;
    _dix_test_xi_convert(&ev, BadMatch, 0);
    ev.type = ET_ProximityIn;
    _dix_test_xi_convert(&ev, BadMatch, 0);
    ev.type = ET_ProximityOut;
    _dix_test_xi_convert(&ev, BadMatch, 0);

    /* more than 6 axes → 2 valuator events */
    SetBit(ev.valuators.mask, 0);
    SetBit(ev.valuators.mask, 1);
    SetBit(ev.valuators.mask, 2);
    SetBit(ev.valuators.mask, 3);
    SetBit(ev.valuators.mask, 4);
    SetBit(ev.valuators.mask, 5);
    SetBit(ev.valuators.mask, 6);
    ev.type = ET_KeyPress;
    _dix_test_xi_convert(&ev, Success, 2);
    ev.type = ET_KeyRelease;
    _dix_test_xi_convert(&ev, Success, 2);
    ev.type = ET_ButtonPress;
    _dix_test_xi_convert(&ev, Success, 2);
    ev.type = ET_ButtonRelease;
    _dix_test_xi_convert(&ev, Success, 2);
    ev.type = ET_Motion;
    _dix_test_xi_convert(&ev, Success, 2);
    ev.type = ET_ProximityIn;
    _dix_test_xi_convert(&ev, Success, 2);
    ev.type = ET_ProximityOut;
    _dix_test_xi_convert(&ev, Success, 2);

    /* keycode too high */
    ev.type = ET_KeyPress;
    ev.detail.key = 256;
    _dix_test_xi_convert(&ev, Success, 0);

    /* deviceid too high */
    ev.type = ET_KeyPress;
    ev.detail.key = 18;
    ev.deviceid = 128;
    _dix_test_xi_convert(&ev, Success, 0);
}

static void
xi2_struct_sizes(void)
{
#define compare(req) \
    assert(sizeof(req) == sz_##req);

    compare(xXIQueryVersionReq);
    compare(xXIWarpPointerReq);
    compare(xXIChangeCursorReq);
    compare(xXIChangeHierarchyReq);
    compare(xXISetClientPointerReq);
    compare(xXIGetClientPointerReq);
    compare(xXISelectEventsReq);
    compare(xXIQueryVersionReq);
    compare(xXIQueryDeviceReq);
    compare(xXISetFocusReq);
    compare(xXIGetFocusReq);
    compare(xXIGrabDeviceReq);
    compare(xXIUngrabDeviceReq);
    compare(xXIAllowEventsReq);
    compare(xXIPassiveGrabDeviceReq);
    compare(xXIPassiveUngrabDeviceReq);
    compare(xXIListPropertiesReq);
    compare(xXIChangePropertyReq);
    compare(xXIDeletePropertyReq);
    compare(xXIGetPropertyReq);
    compare(xXIGetSelectedEventsReq);
#undef compare
}

static void
dix_grab_matching(void)
{
    DeviceIntRec xi_all_devices, xi_all_master_devices, dev1, dev2;
    GrabRec a, b;
    BOOL rc;

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));

    /* different grabtypes must fail */
    a.grabtype = CORE;
    b.grabtype = XI2;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = XI;
    b.grabtype = XI2;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = XI;
    b.grabtype = CORE;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    /* XI2 grabs for different devices must fail, regardless of ignoreDevice
     * XI2 grabs for master devices must fail against a slave */
    memset(&xi_all_devices, 0, sizeof(DeviceIntRec));
    memset(&xi_all_master_devices, 0, sizeof(DeviceIntRec));
    memset(&dev1, 0, sizeof(DeviceIntRec));
    memset(&dev2, 0, sizeof(DeviceIntRec));

    xi_all_devices.id = XIAllDevices;
    xi_all_master_devices.id = XIAllMasterDevices;
    dev1.id = 10;
    dev1.type = SLAVE;
    dev2.id = 11;
    dev2.type = SLAVE;

    inputInfo.all_devices = &xi_all_devices;
    inputInfo.all_master_devices = &xi_all_master_devices;
    a.grabtype = XI2;
    b.grabtype = XI2;
    a.device = &dev1;
    b.device = &dev2;

    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);

    a.device = &dev2;
    b.device = &dev1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&a, &b, TRUE);
    assert(rc == FALSE);

    a.device = inputInfo.all_master_devices;
    b.device = &dev1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&a, &b, TRUE);
    assert(rc == FALSE);

    a.device = &dev1;
    b.device = inputInfo.all_master_devices;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&a, &b, TRUE);
    assert(rc == FALSE);

    /* ignoreDevice FALSE must fail for different devices for CORE and XI */
    a.grabtype = XI;
    b.grabtype = XI;
    a.device = &dev1;
    b.device = &dev2;
    a.modifierDevice = &dev1;
    b.modifierDevice = &dev1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);

    a.grabtype = CORE;
    b.grabtype = CORE;
    a.device = &dev1;
    b.device = &dev2;
    a.modifierDevice = &dev1;
    b.modifierDevice = &dev1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);

    /* ignoreDevice FALSE must fail for different modifier devices for CORE
     * and XI */
    a.grabtype = XI;
    b.grabtype = XI;
    a.device = &dev1;
    b.device = &dev1;
    a.modifierDevice = &dev1;
    b.modifierDevice = &dev2;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);

    a.grabtype = CORE;
    b.grabtype = CORE;
    a.device = &dev1;
    b.device = &dev1;
    a.modifierDevice = &dev1;
    b.modifierDevice = &dev2;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);

    /* different event type must fail */
    a.grabtype = XI2;
    b.grabtype = XI2;
    a.device = &dev1;
    b.device = &dev1;
    a.modifierDevice = &dev1;
    b.modifierDevice = &dev1;
    a.type = XI_KeyPress;
    b.type = XI_KeyRelease;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&a, &b, TRUE);
    assert(rc == FALSE);

    a.grabtype = CORE;
    b.grabtype = CORE;
    a.device = &dev1;
    b.device = &dev1;
    a.modifierDevice = &dev1;
    b.modifierDevice = &dev1;
    a.type = XI_KeyPress;
    b.type = XI_KeyRelease;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&a, &b, TRUE);
    assert(rc == FALSE);

    a.grabtype = XI;
    b.grabtype = XI;
    a.device = &dev1;
    b.device = &dev1;
    a.modifierDevice = &dev1;
    b.modifierDevice = &dev1;
    a.type = XI_KeyPress;
    b.type = XI_KeyRelease;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&a, &b, TRUE);
    assert(rc == FALSE);

    /* different modifiers must fail */
    a.grabtype = XI2;
    b.grabtype = XI2;
    a.device = &dev1;
    b.device = &dev1;
    a.modifierDevice = &dev1;
    b.modifierDevice = &dev1;
    a.type = XI_KeyPress;
    b.type = XI_KeyPress;
    a.modifiersDetail.exact = 1;
    b.modifiersDetail.exact = 2;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = CORE;
    b.grabtype = CORE;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = XI;
    b.grabtype = XI;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    /* AnyModifier must fail for XI2 */
    a.grabtype = XI2;
    b.grabtype = XI2;
    a.modifiersDetail.exact = AnyModifier;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    /* XIAnyModifier must fail for CORE and XI */
    a.grabtype = XI;
    b.grabtype = XI;
    a.modifiersDetail.exact = XIAnyModifier;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = CORE;
    b.grabtype = CORE;
    a.modifiersDetail.exact = XIAnyModifier;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    /* different detail must fail */
    a.grabtype = XI2;
    b.grabtype = XI2;
    a.detail.exact = 1;
    b.detail.exact = 2;
    a.modifiersDetail.exact = 1;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = XI;
    b.grabtype = XI;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = CORE;
    b.grabtype = CORE;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    /* detail of AnyModifier must fail */
    a.grabtype = XI2;
    b.grabtype = XI2;
    a.detail.exact = AnyModifier;
    b.detail.exact = 1;
    a.modifiersDetail.exact = 1;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = CORE;
    b.grabtype = CORE;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = XI;
    b.grabtype = XI;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    /* detail of XIAnyModifier must fail */
    a.grabtype = XI2;
    b.grabtype = XI2;
    a.detail.exact = XIAnyModifier;
    b.detail.exact = 1;
    a.modifiersDetail.exact = 1;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = CORE;
    b.grabtype = CORE;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    a.grabtype = XI;
    b.grabtype = XI;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == FALSE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == FALSE);

    /* XIAnyModifier or AnyModifer must succeed */
    a.grabtype = XI2;
    b.grabtype = XI2;
    a.detail.exact = 1;
    b.detail.exact = 1;
    a.modifiersDetail.exact = XIAnyModifier;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == TRUE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == TRUE);

    a.grabtype = CORE;
    b.grabtype = CORE;
    a.detail.exact = 1;
    b.detail.exact = 1;
    a.modifiersDetail.exact = AnyModifier;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == TRUE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == TRUE);

    a.grabtype = XI;
    b.grabtype = XI;
    a.detail.exact = 1;
    b.detail.exact = 1;
    a.modifiersDetail.exact = AnyModifier;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == TRUE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == TRUE);

    /* AnyKey or XIAnyKeycode must succeed */
    a.grabtype = XI2;
    b.grabtype = XI2;
    a.detail.exact = XIAnyKeycode;
    b.detail.exact = 1;
    a.modifiersDetail.exact = 1;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == TRUE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == TRUE);

    a.grabtype = CORE;
    b.grabtype = CORE;
    a.detail.exact = AnyKey;
    b.detail.exact = 1;
    a.modifiersDetail.exact = 1;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == TRUE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == TRUE);

    a.grabtype = XI;
    b.grabtype = XI;
    a.detail.exact = AnyKey;
    b.detail.exact = 1;
    a.modifiersDetail.exact = 1;
    b.modifiersDetail.exact = 1;
    rc = GrabMatchesSecond(&a, &b, FALSE);
    assert(rc == TRUE);
    rc = GrabMatchesSecond(&b, &a, FALSE);
    assert(rc == TRUE);
}

static void
test_bits_to_byte(int i)
{
    int expected_bytes;

    expected_bytes = (i + 7) / 8;

    assert(bits_to_bytes(i) >= i / 8);
    assert((bits_to_bytes(i) * 8) - i <= 7);
    assert(expected_bytes == bits_to_bytes(i));
}

static void
test_bytes_to_int32(int i)
{
    int expected_4byte;

    expected_4byte = (i + 3) / 4;

    assert(bytes_to_int32(i) <= i);
    assert((bytes_to_int32(i) * 4) - i <= 3);
    assert(expected_4byte == bytes_to_int32(i));
}

static void
test_pad_to_int32(int i)
{
    int expected_bytes;

    expected_bytes = ((i + 3) / 4) * 4;

    assert(pad_to_int32(i) >= i);
    assert(pad_to_int32(i) - i <= 3);
    assert(expected_bytes == pad_to_int32(i));
}

static void
test_padding_for_int32(int i)
{
    static const int padlength[4] = { 0, 3, 2, 1 };
    int expected_bytes = (((i + 3) / 4) * 4) - i;

    assert(padding_for_int32(i) >= 0);
    assert(padding_for_int32(i) <= 3);
    assert(padding_for_int32(i) == expected_bytes);
    assert(padding_for_int32(i) == padlength[i & 3]);
    assert((padding_for_int32(i) + i) == pad_to_int32(i));
}

static void
include_byte_padding_macros(void)
{
    printf("Testing bits_to_bytes()\n");

    /* the macros don't provide overflow protection */
    test_bits_to_byte(0);
    test_bits_to_byte(1);
    test_bits_to_byte(2);
    test_bits_to_byte(7);
    test_bits_to_byte(8);
    test_bits_to_byte(0xFF);
    test_bits_to_byte(0x100);
    test_bits_to_byte(INT_MAX - 9);
    test_bits_to_byte(INT_MAX - 8);

    printf("Testing bytes_to_int32()\n");

    test_bytes_to_int32(0);
    test_bytes_to_int32(1);
    test_bytes_to_int32(2);
    test_bytes_to_int32(7);
    test_bytes_to_int32(8);
    test_bytes_to_int32(0xFF);
    test_bytes_to_int32(0x100);
    test_bytes_to_int32(0xFFFF);
    test_bytes_to_int32(0x10000);
    test_bytes_to_int32(0xFFFFFF);
    test_bytes_to_int32(0x1000000);
    test_bytes_to_int32(INT_MAX - 4);
    test_bytes_to_int32(INT_MAX - 3);

    printf("Testing pad_to_int32()\n");

    test_pad_to_int32(0);
    test_pad_to_int32(1);
    test_pad_to_int32(2);
    test_pad_to_int32(3);
    test_pad_to_int32(7);
    test_pad_to_int32(8);
    test_pad_to_int32(0xFF);
    test_pad_to_int32(0x100);
    test_pad_to_int32(0xFFFF);
    test_pad_to_int32(0x10000);
    test_pad_to_int32(0xFFFFFF);
    test_pad_to_int32(0x1000000);
    test_pad_to_int32(INT_MAX - 4);
    test_pad_to_int32(INT_MAX - 3);

    printf("Testing padding_for_int32()\n");

    test_padding_for_int32(0);
    test_padding_for_int32(1);
    test_padding_for_int32(2);
    test_padding_for_int32(3);
    test_padding_for_int32(7);
    test_padding_for_int32(8);
    test_padding_for_int32(0xFF);
    test_padding_for_int32(0x100);
    test_padding_for_int32(0xFFFF);
    test_padding_for_int32(0x10000);
    test_padding_for_int32(0xFFFFFF);
    test_padding_for_int32(0x1000000);
    test_padding_for_int32(INT_MAX - 4);
    test_padding_for_int32(INT_MAX - 3);
}

static void
xi_unregister_handlers(void)
{
    DeviceIntRec dev;
    int handler;

    memset(&dev, 0, sizeof(dev));

    handler = XIRegisterPropertyHandler(&dev, NULL, NULL, NULL);
    assert(handler == 1);
    handler = XIRegisterPropertyHandler(&dev, NULL, NULL, NULL);
    assert(handler == 2);
    handler = XIRegisterPropertyHandler(&dev, NULL, NULL, NULL);
    assert(handler == 3);

    printf("Unlinking from front.\n");

    XIUnregisterPropertyHandler(&dev, 4);       /* NOOP */
    assert(dev.properties.handlers->id == 3);
    XIUnregisterPropertyHandler(&dev, 3);
    assert(dev.properties.handlers->id == 2);
    XIUnregisterPropertyHandler(&dev, 2);
    assert(dev.properties.handlers->id == 1);
    XIUnregisterPropertyHandler(&dev, 1);
    assert(dev.properties.handlers == NULL);

    handler = XIRegisterPropertyHandler(&dev, NULL, NULL, NULL);
    assert(handler == 4);
    handler = XIRegisterPropertyHandler(&dev, NULL, NULL, NULL);
    assert(handler == 5);
    handler = XIRegisterPropertyHandler(&dev, NULL, NULL, NULL);
    assert(handler == 6);
    XIUnregisterPropertyHandler(&dev, 3);       /* NOOP */
    assert(dev.properties.handlers->next->next->next == NULL);
    XIUnregisterPropertyHandler(&dev, 4);
    assert(dev.properties.handlers->next->next == NULL);
    XIUnregisterPropertyHandler(&dev, 5);
    assert(dev.properties.handlers->next == NULL);
    XIUnregisterPropertyHandler(&dev, 6);
    assert(dev.properties.handlers == NULL);

    handler = XIRegisterPropertyHandler(&dev, NULL, NULL, NULL);
    assert(handler == 7);
    handler = XIRegisterPropertyHandler(&dev, NULL, NULL, NULL);
    assert(handler == 8);
    handler = XIRegisterPropertyHandler(&dev, NULL, NULL, NULL);
    assert(handler == 9);

    XIDeleteAllDeviceProperties(&dev);
    assert(dev.properties.handlers == NULL);
    XIUnregisterPropertyHandler(&dev, 7);       /* NOOP */

}

static void
cmp_attr_fields(InputAttributes * attr1, InputAttributes * attr2)
{
    char **tags1, **tags2;

    assert(attr1);
    assert(attr2);
    assert(attr1 != attr2);
    assert(attr1->flags == attr2->flags);

    if (attr1->product != NULL) {
        assert(attr1->product != attr2->product);
        assert(strcmp(attr1->product, attr2->product) == 0);
    }
    else
        assert(attr2->product == NULL);

    if (attr1->vendor != NULL) {
        assert(attr1->vendor != attr2->vendor);
        assert(strcmp(attr1->vendor, attr2->vendor) == 0);
    }
    else
        assert(attr2->vendor == NULL);

    if (attr1->device != NULL) {
        assert(attr1->device != attr2->device);
        assert(strcmp(attr1->device, attr2->device) == 0);
    }
    else
        assert(attr2->device == NULL);

    if (attr1->pnp_id != NULL) {
        assert(attr1->pnp_id != attr2->pnp_id);
        assert(strcmp(attr1->pnp_id, attr2->pnp_id) == 0);
    }
    else
        assert(attr2->pnp_id == NULL);

    if (attr1->usb_id != NULL) {
        assert(attr1->usb_id != attr2->usb_id);
        assert(strcmp(attr1->usb_id, attr2->usb_id) == 0);
    }
    else
        assert(attr2->usb_id == NULL);

    tags1 = attr1->tags;
    tags2 = attr2->tags;

    /* if we don't have any tags, skip the tag checking bits */
    if (!tags1) {
        assert(!tags2);
        return;
    }

    /* Don't lug around empty arrays */
    assert(*tags1);
    assert(*tags2);

    /* check for identical content, but duplicated */
    while (*tags1) {
        assert(*tags1 != *tags2);
        assert(strcmp(*tags1, *tags2) == 0);
        tags1++;
        tags2++;
    }

    /* ensure tags1 and tags2 have the same no of elements */
    assert(!*tags2);

    /* check for not sharing memory */
    tags1 = attr1->tags;
    while (*tags1) {
        tags2 = attr2->tags;
        while (*tags2)
            assert(*tags1 != *tags2++);

        tags1++;
    }
}

static void
dix_input_attributes(void)
{
    InputAttributes *orig;
    InputAttributes *new;

    new = DuplicateInputAttributes(NULL);
    assert(!new);

    orig = calloc(1, sizeof(InputAttributes));
    assert(orig);

    new = DuplicateInputAttributes(orig);
    assert(memcmp(orig, new, sizeof(InputAttributes)) == 0);

    orig->product = xnfstrdup("product name");
    new = DuplicateInputAttributes(orig);
    cmp_attr_fields(orig, new);
    FreeInputAttributes(new);

    orig->vendor = xnfstrdup("vendor name");
    new = DuplicateInputAttributes(orig);
    cmp_attr_fields(orig, new);
    FreeInputAttributes(new);

    orig->device = xnfstrdup("device path");
    new = DuplicateInputAttributes(orig);
    cmp_attr_fields(orig, new);
    FreeInputAttributes(new);

    orig->pnp_id = xnfstrdup("PnPID");
    new = DuplicateInputAttributes(orig);
    cmp_attr_fields(orig, new);
    FreeInputAttributes(new);

    orig->usb_id = xnfstrdup("USBID");
    new = DuplicateInputAttributes(orig);
    cmp_attr_fields(orig, new);
    FreeInputAttributes(new);

    orig->flags = 0xF0;
    new = DuplicateInputAttributes(orig);
    cmp_attr_fields(orig, new);
    FreeInputAttributes(new);

    orig->tags = xstrtokenize("tag1 tag2 tag3", " ");
    new = DuplicateInputAttributes(orig);
    cmp_attr_fields(orig, new);
    FreeInputAttributes(new);

    FreeInputAttributes(orig);
}

static void
dix_input_valuator_masks(void)
{
    ValuatorMask *mask = NULL, *copy;
    int nvaluators = MAX_VALUATORS;
    double valuators[nvaluators];
    int val_ranged[nvaluators];
    int i;
    int first_val, num_vals;

    for (i = 0; i < nvaluators; i++) {
        valuators[i] = i + 0.5;
        val_ranged[i] = i;
    }

    mask = valuator_mask_new(nvaluators);
    assert(mask != NULL);
    assert(valuator_mask_size(mask) == 0);
    assert(valuator_mask_num_valuators(mask) == 0);

    for (i = 0; i < nvaluators; i++) {
        assert(!valuator_mask_isset(mask, i));
        valuator_mask_set_double(mask, i, valuators[i]);
        assert(valuator_mask_isset(mask, i));
        assert(valuator_mask_get(mask, i) == trunc(valuators[i]));
        assert(valuator_mask_get_double(mask, i) == valuators[i]);
        assert(valuator_mask_size(mask) == i + 1);
        assert(valuator_mask_num_valuators(mask) == i + 1);
    }

    for (i = 0; i < nvaluators; i++) {
        assert(valuator_mask_isset(mask, i));
        valuator_mask_unset(mask, i);
        /* we're removing valuators from the front, so size should stay the
         * same until the last bit is removed */
        if (i < nvaluators - 1)
            assert(valuator_mask_size(mask) == nvaluators);
        assert(!valuator_mask_isset(mask, i));
    }

    assert(valuator_mask_size(mask) == 0);
    valuator_mask_zero(mask);
    assert(valuator_mask_size(mask) == 0);
    assert(valuator_mask_num_valuators(mask) == 0);
    for (i = 0; i < nvaluators; i++)
        assert(!valuator_mask_isset(mask, i));

    first_val = 5;
    num_vals = 6;

    valuator_mask_set_range(mask, first_val, num_vals, val_ranged);
    assert(valuator_mask_size(mask) == first_val + num_vals);
    assert(valuator_mask_num_valuators(mask) == num_vals);
    for (i = 0; i < nvaluators; i++) {
        double val;

        if (i < first_val || i >= first_val + num_vals) {
            assert(!valuator_mask_isset(mask, i));
            assert(!valuator_mask_fetch_double(mask, i, &val));
        }
        else {
            assert(valuator_mask_isset(mask, i));
            assert(valuator_mask_get(mask, i) == val_ranged[i - first_val]);
            assert(valuator_mask_get_double(mask, i) ==
                   val_ranged[i - first_val]);
            assert(valuator_mask_fetch_double(mask, i, &val));
            assert(val_ranged[i - first_val] == val);
        }
    }

    copy = valuator_mask_new(nvaluators);
    valuator_mask_copy(copy, mask);
    assert(mask != copy);
    assert(valuator_mask_size(mask) == valuator_mask_size(copy));
    assert(valuator_mask_num_valuators(mask) ==
           valuator_mask_num_valuators(copy));

    for (i = 0; i < nvaluators; i++) {
        double a, b;

        assert(valuator_mask_isset(mask, i) == valuator_mask_isset(copy, i));

        if (!valuator_mask_isset(mask, i))
            continue;

        assert(valuator_mask_get(mask, i) == valuator_mask_get(copy, i));
        assert(valuator_mask_get_double(mask, i) ==
               valuator_mask_get_double(copy, i));
        assert(valuator_mask_fetch_double(mask, i, &a));
        assert(valuator_mask_fetch_double(copy, i, &b));
        assert(a == b);
    }

    valuator_mask_free(&mask);
    assert(mask == NULL);
}

static void
dix_valuator_mode(void)
{
    DeviceIntRec dev;
    const int num_axes = MAX_VALUATORS;
    int i;
    Atom atoms[MAX_VALUATORS] = { 0 };

    memset(&dev, 0, sizeof(DeviceIntRec));
    dev.type = MASTER_POINTER;  /* claim it's a master to stop ptracccel */

    assert(InitValuatorClassDeviceStruct(NULL, 0, atoms, 0, 0) == FALSE);
    assert(InitValuatorClassDeviceStruct(&dev, num_axes, atoms, 0, Absolute));

    for (i = 0; i < num_axes; i++) {
        assert(valuator_get_mode(&dev, i) == Absolute);
        valuator_set_mode(&dev, i, Relative);
        assert(dev.valuator->axes[i].mode == Relative);
        assert(valuator_get_mode(&dev, i) == Relative);
    }

    valuator_set_mode(&dev, VALUATOR_MODE_ALL_AXES, Absolute);
    for (i = 0; i < num_axes; i++)
        assert(valuator_get_mode(&dev, i) == Absolute);

    valuator_set_mode(&dev, VALUATOR_MODE_ALL_AXES, Relative);
    for (i = 0; i < num_axes; i++)
        assert(valuator_get_mode(&dev, i) == Relative);
}

static void
dix_input_valuator_masks_unaccel(void)
{
    ValuatorMask *mask = NULL;
    double x, ux;

    /* set mask normally */
    mask = valuator_mask_new(MAX_VALUATORS);
    assert(!valuator_mask_has_unaccelerated(mask));
    valuator_mask_set_double(mask, 0, 1.0);
    assert(!valuator_mask_has_unaccelerated(mask));
    valuator_mask_unset(mask, 0);
    assert(!valuator_mask_has_unaccelerated(mask));

    /* all unset, now set accel mask */
    valuator_mask_set_unaccelerated(mask, 0, 1.0, 2.0);
    assert(valuator_mask_has_unaccelerated(mask));
    assert(valuator_mask_isset(mask, 0));
    assert(!valuator_mask_isset(mask, 1));
    assert(valuator_mask_get_accelerated(mask, 0) ==  1.0);
    assert(valuator_mask_get_unaccelerated(mask, 0) ==  2.0);
    assert(valuator_mask_fetch_unaccelerated(mask, 0, &x, &ux));
    assert(x == 1.0);
    assert(ux == 2.0);
    x = 0xff;
    ux = 0xfe;
    assert(!valuator_mask_fetch_unaccelerated(mask, 1, &x, &ux));
    assert(x == 0xff);
    assert(ux == 0xfe);

    /* all unset, now set normally again */
    valuator_mask_unset(mask, 0);
    assert(!valuator_mask_has_unaccelerated(mask));
    assert(!valuator_mask_isset(mask, 0));
    valuator_mask_set_double(mask, 0, 1.0);
    assert(!valuator_mask_has_unaccelerated(mask));
    valuator_mask_unset(mask, 0);
    assert(!valuator_mask_has_unaccelerated(mask));

    valuator_mask_zero(mask);
    assert(!valuator_mask_has_unaccelerated(mask));

    valuator_mask_set_unaccelerated(mask, 0, 1.0, 2.0);
    valuator_mask_set_unaccelerated(mask, 1, 3.0, 4.5);
    assert(valuator_mask_isset(mask, 0));
    assert(valuator_mask_isset(mask, 1));
    assert(!valuator_mask_isset(mask, 2));
    assert(valuator_mask_has_unaccelerated(mask));
    assert(valuator_mask_get_accelerated(mask, 0) == 1.0);
    assert(valuator_mask_get_accelerated(mask, 1) == 3.0);
    assert(valuator_mask_get_unaccelerated(mask, 0) == 2.0);
    assert(valuator_mask_get_unaccelerated(mask, 1) == 4.5);
    assert(valuator_mask_fetch_unaccelerated(mask, 0, &x, &ux));
    assert(x == 1.0);
    assert(ux == 2.0);
    assert(valuator_mask_fetch_unaccelerated(mask, 1, &x, &ux));
    assert(x == 3.0);
    assert(ux == 4.5);

    valuator_mask_free(&mask);
}

static void
include_bit_test_macros(void)
{
    uint8_t mask[9] = { 0 };
    int i;

    for (i = 0; i < ARRAY_SIZE(mask); i++) {
        assert(BitIsOn(mask, i) == 0);
        SetBit(mask, i);
        assert(BitIsOn(mask, i) == 1);
        assert(! !(mask[i / 8] & (1 << (i % 8))));
        assert(CountBits(mask, sizeof(mask)) == 1);
        ClearBit(mask, i);
        assert(BitIsOn(mask, i) == 0);
    }
}

/**
 * Ensure that val->axisVal and val->axes are aligned on doubles.
 */
static void
dix_valuator_alloc(void)
{
    ValuatorClassPtr v = NULL;
    int num_axes = 0;

    while (num_axes < 5) {
        v = AllocValuatorClass(v, num_axes);

        assert(v);
        assert(v->numAxes == num_axes);
#if !defined(__i386__) && !defined(__m68k__) && !defined(__sh__)
        /* must be double-aligned on 64 bit */
        assert(offsetof(struct _ValuatorClassRec, axisVal) % sizeof(double) == 0);
        assert(offsetof(struct _ValuatorClassRec, axes) % sizeof(double) == 0);
#endif
        num_axes++;
    }

    free(v);
}

static void
dix_get_master(void)
{
    DeviceIntRec vcp, vck;
    DeviceIntRec ptr, kbd;
    DeviceIntRec floating;
    SpriteInfoRec vcp_sprite, vck_sprite;
    SpriteInfoRec ptr_sprite, kbd_sprite;
    SpriteInfoRec floating_sprite;

    memset(&vcp, 0, sizeof(vcp));
    memset(&vck, 0, sizeof(vck));
    memset(&ptr, 0, sizeof(ptr));
    memset(&kbd, 0, sizeof(kbd));
    memset(&floating, 0, sizeof(floating));

    memset(&vcp_sprite, 0, sizeof(vcp_sprite));
    memset(&vck_sprite, 0, sizeof(vck_sprite));
    memset(&ptr_sprite, 0, sizeof(ptr_sprite));
    memset(&kbd_sprite, 0, sizeof(kbd_sprite));
    memset(&floating_sprite, 0, sizeof(floating_sprite));

    vcp.type = MASTER_POINTER;
    vck.type = MASTER_KEYBOARD;
    ptr.type = SLAVE;
    kbd.type = SLAVE;
    floating.type = SLAVE;

    vcp.spriteInfo = &vcp_sprite;
    vck.spriteInfo = &vck_sprite;
    ptr.spriteInfo = &ptr_sprite;
    kbd.spriteInfo = &kbd_sprite;
    floating.spriteInfo = &floating_sprite;

    vcp_sprite.paired = &vck;
    vck_sprite.paired = &vcp;
    ptr_sprite.paired = &vcp;
    kbd_sprite.paired = &vck;
    floating_sprite.paired = &floating;

    vcp_sprite.spriteOwner = TRUE;
    floating_sprite.spriteOwner = TRUE;

    ptr.master = &vcp;
    kbd.master = &vck;

    assert(GetPairedDevice(&vcp) == &vck);
    assert(GetPairedDevice(&vck) == &vcp);
    assert(GetMaster(&ptr, MASTER_POINTER) == &vcp);
    assert(GetMaster(&ptr, MASTER_KEYBOARD) == &vck);
    assert(GetMaster(&kbd, MASTER_POINTER) == &vcp);
    assert(GetMaster(&kbd, MASTER_KEYBOARD) == &vck);
    assert(GetMaster(&ptr, MASTER_ATTACHED) == &vcp);
    assert(GetMaster(&kbd, MASTER_ATTACHED) == &vck);

    assert(GetPairedDevice(&floating) == &floating);
    assert(GetMaster(&floating, MASTER_POINTER) == NULL);
    assert(GetMaster(&floating, MASTER_KEYBOARD) == NULL);
    assert(GetMaster(&floating, MASTER_ATTACHED) == NULL);

    assert(GetMaster(&vcp, POINTER_OR_FLOAT) == &vcp);
    assert(GetMaster(&vck, POINTER_OR_FLOAT) == &vcp);
    assert(GetMaster(&ptr, POINTER_OR_FLOAT) == &vcp);
    assert(GetMaster(&kbd, POINTER_OR_FLOAT) == &vcp);

    assert(GetMaster(&vcp, KEYBOARD_OR_FLOAT) == &vck);
    assert(GetMaster(&vck, KEYBOARD_OR_FLOAT) == &vck);
    assert(GetMaster(&ptr, KEYBOARD_OR_FLOAT) == &vck);
    assert(GetMaster(&kbd, KEYBOARD_OR_FLOAT) == &vck);

    assert(GetMaster(&floating, KEYBOARD_OR_FLOAT) == &floating);
    assert(GetMaster(&floating, POINTER_OR_FLOAT) == &floating);
}

static void
input_option_test(void)
{
    InputOption *list = NULL;
    InputOption *opt;
    const char *val;

    printf("Testing input_option list interface\n");

    list = input_option_new(list, "key", "value");
    assert(list);
    opt = input_option_find(list, "key");
    val = input_option_get_value(opt);
    assert(strcmp(val, "value") == 0);

    list = input_option_new(list, "2", "v2");
    opt = input_option_find(list, "key");
    val = input_option_get_value(opt);
    assert(strcmp(val, "value") == 0);

    opt = input_option_find(list, "2");
    val = input_option_get_value(opt);
    assert(strcmp(val, "v2") == 0);

    list = input_option_new(list, "3", "v3");

    /* search, delete */
    opt = input_option_find(list, "key");
    val = input_option_get_value(opt);
    assert(strcmp(val, "value") == 0);
    list = input_option_free_element(list, "key");
    opt = input_option_find(list, "key");
    assert(opt == NULL);

    opt = input_option_find(list, "2");
    val = input_option_get_value(opt);
    assert(strcmp(val, "v2") == 0);
    list = input_option_free_element(list, "2");
    opt = input_option_find(list, "2");
    assert(opt == NULL);

    opt = input_option_find(list, "3");
    val = input_option_get_value(opt);
    assert(strcmp(val, "v3") == 0);
    list = input_option_free_element(list, "3");
    opt = input_option_find(list, "3");
    assert(opt == NULL);

    /* list deletion */
    list = input_option_new(list, "1", "v3");
    list = input_option_new(list, "2", "v3");
    list = input_option_new(list, "3", "v3");
    input_option_free_list(&list);

    assert(list == NULL);

    list = input_option_new(list, "1", "v1");
    list = input_option_new(list, "2", "v2");
    list = input_option_new(list, "3", "v3");

    /* value replacement */
    opt = input_option_find(list, "2");
    val = input_option_get_value(opt);
    assert(strcmp(val, "v2") == 0);
    input_option_set_value(opt, "foo");
    val = input_option_get_value(opt);
    assert(strcmp(val, "foo") == 0);
    opt = input_option_find(list, "2");
    val = input_option_get_value(opt);
    assert(strcmp(val, "foo") == 0);

    /* key replacement */
    input_option_set_key(opt, "bar");
    val = input_option_get_key(opt);
    assert(strcmp(val, "bar") == 0);
    opt = input_option_find(list, "bar");
    val = input_option_get_value(opt);
    assert(strcmp(val, "foo") == 0);

    /* value replacement in input_option_new */
    list = input_option_new(list, "bar", "foobar");
    opt = input_option_find(list, "bar");
    val = input_option_get_value(opt);
    assert(strcmp(val, "foobar") == 0);

    input_option_free_list(&list);
    assert(list == NULL);
}

static void
_test_double_fp16_values(double orig_d)
{
    FP1616 first_fp16, final_fp16;
    double final_d;

    if (orig_d > 0x7FFF) {
        printf("Test out of range\n");
        assert(0);
    }

    first_fp16 = double_to_fp1616(orig_d);
    final_d = fp1616_to_double(first_fp16);
    final_fp16 = double_to_fp1616(final_d);

    /* {
     *    char first_fp16_s[64];
     *    char final_fp16_s[64];
     *    snprintf(first_fp16_s, sizeof(first_fp16_s), "%d + %u * 2^-16", (first_fp16 & 0xffff0000) >> 16, first_fp16 & 0xffff);
     *    snprintf(final_fp16_s, sizeof(final_fp16_s), "%d + %u * 2^-16", (final_fp16 & 0xffff0000) >> 16, final_fp16 & 0xffff);
     *
     *    printf("FP16: original double: %f first fp16: %s, re-encoded double: %f, final fp16: %s\n", orig_d, first_fp16_s, final_d, final_fp16_s);
     * }
     */

    /* since we lose precision, we only do rough range testing */
    assert(final_d > orig_d - 0.1);
    assert(final_d < orig_d + 0.1);

    assert(memcmp(&first_fp16, &final_fp16, sizeof(FP1616)) == 0);

    if (orig_d > 0)
        _test_double_fp16_values(-orig_d);
}

static void
_test_double_fp32_values(double orig_d)
{
    FP3232 first_fp32, final_fp32;
    double final_d;

    if (orig_d > 0x7FFFFFFF) {
        printf("Test out of range\n");
        assert(0);
    }

    first_fp32 = double_to_fp3232(orig_d);
    final_d = fp3232_to_double(first_fp32);
    final_fp32 = double_to_fp3232(final_d);

    /* {
     *     char first_fp32_s[64];
     *     char final_fp32_s[64];
     *     snprintf(first_fp32_s, sizeof(first_fp32_s), "%d + %u * 2^-32", first_fp32.integral, first_fp32.frac);
     *     snprintf(final_fp32_s, sizeof(final_fp32_s), "%d + %u * 2^-32", first_fp32.integral, final_fp32.frac);
     *
     *     printf("FP32: original double: %f first fp32: %s, re-encoded double: %f, final fp32: %s\n", orig_d, first_fp32_s, final_d, final_fp32_s);
     * }
     */

    /* since we lose precision, we only do rough range testing */
    assert(final_d > orig_d - 0.1);
    assert(final_d < orig_d + 0.1);

    assert(memcmp(&first_fp32, &final_fp32, sizeof(FP3232)) == 0);

    if (orig_d > 0)
        _test_double_fp32_values(-orig_d);
}

static void
dix_double_fp_conversion(void)
{
    uint32_t i;

    printf("Testing double to FP1616/FP3232 conversions\n");

    _test_double_fp16_values(0);
    for (i = 1; i < 0x7FFF; i <<= 1) {
        double val;

        val = i;
        _test_double_fp16_values(val);
        _test_double_fp32_values(val);

        /* and some pseudo-random floating points */
        val = i - 0.00382;
        _test_double_fp16_values(val);
        _test_double_fp32_values(val);

        val = i + 0.00382;
        _test_double_fp16_values(val);
        _test_double_fp32_values(val);

        val = i + 0.05234;
        _test_double_fp16_values(val);
        _test_double_fp32_values(val);

        val = i + 0.12342;
        _test_double_fp16_values(val);
        _test_double_fp32_values(val);

        val = i + 0.27583;
        _test_double_fp16_values(val);
        _test_double_fp32_values(val);

        val = i + 0.50535;
        _test_double_fp16_values(val);
        _test_double_fp32_values(val);

        val = i + 0.72342;
        _test_double_fp16_values(val);
        _test_double_fp32_values(val);

        val = i + 0.80408;
        _test_double_fp16_values(val);
        _test_double_fp32_values(val);
    }

    for (i = 0x7FFFF; i < 0x7FFFFFFF; i <<= 1) {
        _test_double_fp32_values(i);
        /* and a few more random floating points, obtained
         * by faceplanting into the numpad repeatedly */
        _test_double_fp32_values(i + 0.010177);
        _test_double_fp32_values(i + 0.213841);
        _test_double_fp32_values(i + 0.348720);
        _test_double_fp32_values(i + 0.472020);
        _test_double_fp32_values(i + 0.572020);
        _test_double_fp32_values(i + 0.892929);
    }
}

/* The mieq test verifies that events added to the queue come out in the same
 * order that they went in.
 */
static uint32_t mieq_test_event_last_processed;

static void
mieq_test_event_handler(int screenNum, InternalEvent *ie, DeviceIntPtr dev)
{
    RawDeviceEvent *e = (RawDeviceEvent *) ie;

    assert(e->type == ET_RawMotion);
    assert(e->flags > mieq_test_event_last_processed);
    mieq_test_event_last_processed = e->flags;
}

static void
_mieq_test_generate_events(uint32_t start, uint32_t count)
{
    static DeviceIntRec dev;
    static SpriteInfoRec spriteInfo;
    static SpriteRec sprite;

    memset(&dev, 0, sizeof(dev));
    memset(&spriteInfo, 0, sizeof(spriteInfo));
    memset(&sprite, 0, sizeof(sprite));
    dev.spriteInfo = &spriteInfo;
    spriteInfo.sprite = &sprite;

    dev.enabled = 1;

    count += start;
    while (start < count) {
        RawDeviceEvent e = { 0 };
        e.header = ET_Internal;
        e.type = ET_RawMotion;
        e.length = sizeof(e);
        e.time = GetTimeInMillis();
        e.flags = start;

        mieqEnqueue(&dev, (InternalEvent *) &e);

        start++;
    }
}

#define mieq_test_generate_events(c) { _mieq_test_generate_events(next, c); next += c; }

static void
mieq_test(void)
{
    uint32_t next = 1;

    mieq_test_event_last_processed = 0;
    mieqInit();
    mieqSetHandler(ET_RawMotion, mieq_test_event_handler);

    /* Enough to fit the buffer but trigger a grow */
    mieq_test_generate_events(180);

    /* We should resize to 512 now */
    mieqProcessInputEvents();

    /* Some should now get dropped */
    mieq_test_generate_events(500);

    /* Tell us how many got dropped, 1024 now */
    mieqProcessInputEvents();

    /* Now make it 2048 */
    mieq_test_generate_events(900);
    mieqProcessInputEvents();

    /* Now make it 4096 (max) */
    mieq_test_generate_events(1950);
    mieqProcessInputEvents();

    /* Now overflow one last time with the maximal queue and reach the verbosity limit */
    mieq_test_generate_events(10000);
    mieqProcessInputEvents();

    mieqFini();
}

/* Simple check that we're replaying events in-order */
static void
process_input_proc(InternalEvent *ev, DeviceIntPtr device)
{
    static int last_evtype = -1;

    if (ev->any.header == 0xac)
        last_evtype = -1;

    assert(ev->any.type == ++last_evtype);
}

static void
dix_enqueue_events(void)
{
#define NEVENTS 5
    DeviceIntRec dev;
    InternalEvent ev[NEVENTS];
    SpriteInfoRec spriteInfo;
    SpriteRec sprite;
    QdEventPtr qe;
    int i;

    memset(&dev, 0, sizeof(dev));
    dev.public.processInputProc = process_input_proc;

    memset(&spriteInfo, 0, sizeof(spriteInfo));
    memset(&sprite, 0, sizeof(sprite));
    dev.spriteInfo = &spriteInfo;
    spriteInfo.sprite = &sprite;

    InitEvents();
    assert(xorg_list_is_empty(&syncEvents.pending));

    /* this way PlayReleasedEvents really runs through all events in the
     * queue */
    inputInfo.devices = &dev;

    /* to reset process_input_proc */
    ev[0].any.header = 0xac;

    for (i = 0; i < NEVENTS; i++) {
        ev[i].any.length = sizeof(*ev);
        ev[i].any.type = i;
        EnqueueEvent(&ev[i], &dev);
        assert(!xorg_list_is_empty(&syncEvents.pending));
        qe = xorg_list_last_entry(&syncEvents.pending, QdEventRec, next);
        assert(memcmp(qe->event, &ev[i], ev[i].any.length) == 0);
        qe = xorg_list_first_entry(&syncEvents.pending, QdEventRec, next);
        assert(memcmp(qe->event, &ev[0], ev[i].any.length) == 0);
    }

    /* calls process_input_proc */
    dev.deviceGrab.sync.frozen = 1;
    PlayReleasedEvents();
    assert(!xorg_list_is_empty(&syncEvents.pending));

    dev.deviceGrab.sync.frozen = 0;
    PlayReleasedEvents();
    assert(xorg_list_is_empty(&syncEvents.pending));

    inputInfo.devices = NULL;
}

int
input_test(void)
{
    dix_enqueue_events();
    dix_double_fp_conversion();
    dix_input_valuator_masks();
    dix_input_valuator_masks_unaccel();
    dix_input_attributes();
    dix_init_valuators();
    dix_event_to_core_conversion();
    dix_event_to_xi1_conversion();
    dix_check_grab_values();
    xi2_struct_sizes();
    dix_grab_matching();
    dix_valuator_mode();
    include_byte_padding_macros();
    include_bit_test_macros();
    xi_unregister_handlers();
    dix_valuator_alloc();
    dix_get_master();
    input_option_test();
    mieq_test();

    return 0;
}
