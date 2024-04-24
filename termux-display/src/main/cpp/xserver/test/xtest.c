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
#include <X11/Xatom.h>
#include "input.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "exevents.h"
#include "extinit.h"
#include "xkbsrv.h"
#include "xserver-properties.h"
#include "syncsrv.h"

#include "tests-common.h"

/**
 */

/* from Xext/xtest.c */
extern DeviceIntPtr xtestpointer, xtestkeyboard;

/* Needed for the screen setup, otherwise we crash during sprite initialization */
static Bool
device_cursor_init(DeviceIntPtr dev, ScreenPtr screen)
{
    return TRUE;
}

static void
device_cursor_cleanup(DeviceIntPtr dev, ScreenPtr screen)
{
}

static void
xtest_init_devices(void)
{
    ScreenRec screen = {0};
    ClientRec server_client = {0};
    WindowRec root = {{0}};
    WindowOptRec optional = {0};

    /* random stuff that needs initialization */
    root.drawable.id = 0xab;
    root.optional = &optional;
    screen.root = &root;
    screenInfo.numScreens = 1;
    screenInfo.screens[0] = &screen;
    screen.myNum = 0;
    screen.id = 100;
    screen.width = 640;
    screen.height = 480;
    screen.DeviceCursorInitialize = device_cursor_init;
    screen.DeviceCursorCleanup = device_cursor_cleanup;
    dixResetPrivates();
    serverClient = &server_client;
    InitClient(serverClient, 0, (void *) NULL);
    if (!InitClientResources(serverClient)) /* for root resources */
        FatalError("couldn't init server resources");
    InitAtoms();
    SyncExtensionInit();

    /* this also inits the xtest devices */
    InitCoreDevices();

    assert(xtestpointer);
    assert(xtestkeyboard);
    assert(IsXTestDevice(xtestpointer, NULL));
    assert(IsXTestDevice(xtestkeyboard, NULL));
    assert(IsXTestDevice(xtestpointer, inputInfo.pointer));

    assert(IsXTestDevice(xtestkeyboard, inputInfo.keyboard));
    assert(GetXTestDevice(inputInfo.pointer) == xtestpointer);

    assert(GetXTestDevice(inputInfo.keyboard) == xtestkeyboard);
}

/**
 * Each xtest devices has a property attached marking it. This property
 * cannot be changed.
 */
static void
xtest_properties(void)
{
    int rc;
    char value = 1;
    XIPropertyValuePtr prop;
    Atom xtest_prop = XIGetKnownProperty(XI_PROP_XTEST_DEVICE);

    rc = XIGetDeviceProperty(xtestpointer, xtest_prop, &prop);
    assert(rc == Success);
    assert(prop);

    rc = XIGetDeviceProperty(xtestkeyboard, xtest_prop, &prop);
    assert(rc == Success);
    assert(prop != NULL);

    rc = XIChangeDeviceProperty(xtestpointer, xtest_prop,
                                XA_INTEGER, 8, PropModeReplace, 1, &value,
                                FALSE);
    assert(rc == BadAccess);
    rc = XIChangeDeviceProperty(xtestkeyboard, xtest_prop,
                                XA_INTEGER, 8, PropModeReplace, 1, &value,
                                FALSE);
    assert(rc == BadAccess);
}

int
xtest_test(void)
{
    xtest_init_devices();
    xtest_properties();

    return 0;
}
