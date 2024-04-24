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

#include <errno.h>
#include <stdint.h>
#include "extinit.h"            /* for XInputExtensionInit */
#include "exglobals.h"
#include "xkbsrv.h"             /* for XkbInitPrivates */
#include "xserver-properties.h"
#include "syncsrv.h"
#include <X11/extensions/XI2.h>

#define INSIDE_PROTOCOL_COMMON
#include "protocol-common.h"

struct devices devices;
ScreenRec screen;
WindowRec root;
WindowRec window;
static ClientRec server_client;

void *global_userdata;

void (*reply_handler) (ClientPtr client, int len, char *data, void *userdata);

int enable_GrabButton_wrap = 1;
int enable_XISetEventMask_wrap = 1;

static void
fake_init_sprite(DeviceIntPtr dev)
{
    SpritePtr sprite;

    sprite = dev->spriteInfo->sprite;

    sprite->spriteTraceSize = 10;
    sprite->spriteTrace = calloc(sprite->spriteTraceSize, sizeof(WindowPtr));
    sprite->spriteTraceGood = 1;
    sprite->spriteTrace[0] = &root;
    sprite->hot.x = SPRITE_X;
    sprite->hot.y = SPRITE_Y;
    sprite->hotPhys.x = sprite->hot.x;
    sprite->hotPhys.y = sprite->hot.y;
    sprite->win = &window;
    sprite->hotPhys.pScreen = &screen;
    sprite->physLimits.x1 = 0;
    sprite->physLimits.y1 = 0;
    sprite->physLimits.x2 = screen.width;
    sprite->physLimits.y2 = screen.height;
}

/* This is essentially CorePointerProc with ScrollAxes added */
static int
TestPointerProc(DeviceIntPtr pDev, int what)
{
#define NBUTTONS 10
#define NAXES 4
    BYTE map[NBUTTONS + 1];
    int i = 0;
    Atom btn_labels[NBUTTONS] = { 0 };
    Atom axes_labels[NAXES] = { 0 };

    switch (what) {
    case DEVICE_INIT:
        for (i = 1; i <= NBUTTONS; i++)
            map[i] = i;

        btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
        btn_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
        btn_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
        btn_labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
        btn_labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
        btn_labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
        btn_labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);
        /* don't know about the rest */

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);
        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_VSCROLL);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_HSCROLL);

        if (!InitPointerDeviceStruct
            ((DevicePtr) pDev, map, NBUTTONS, btn_labels,
             (PtrCtrlProcPtr) NoopDDA, GetMotionHistorySize(), NAXES,
             axes_labels)) {
            ErrorF("Could not initialize device '%s'. Out of memory.\n",
                   pDev->name);
            return BadAlloc;
        }
        pDev->valuator->axisVal[0] = screenInfo.screens[0]->width / 2;
        pDev->last.valuators[0] = pDev->valuator->axisVal[0];
        pDev->valuator->axisVal[1] = screenInfo.screens[0]->height / 2;
        pDev->last.valuators[1] = pDev->valuator->axisVal[1];

        /* protocol-xiquerydevice.c relies on these increment */
        SetScrollValuator(pDev, 2, SCROLL_TYPE_VERTICAL, 2.4, SCROLL_FLAG_NONE);
        SetScrollValuator(pDev, 3, SCROLL_TYPE_HORIZONTAL, 3.5,
                          SCROLL_FLAG_PREFERRED);
        break;

    case DEVICE_CLOSE:
        break;

    default:
        break;
    }

    return Success;

#undef NBUTTONS
#undef NAXES
}

/**
 * Create and init 2 master devices (VCP + VCK) and two slave devices, one
 * default mouse, one default keyboard.
 */
struct devices
init_devices(void)
{
    ClientRec client;
    struct devices local_devices;
    int ret;

    /*
     * Put a unique name in display pointer so that when tests are run in
     * parallel, their xkbcomp outputs to /tmp/server-<display>.xkm don't
     * stomp on each other.
     */
#ifdef HAVE_GETPROGNAME
    display = getprogname();
#elif HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME
    display = program_invocation_short_name;
#endif

    client = init_client(0, NULL);

    AllocDevicePair(&client, "Virtual core", &local_devices.vcp, &local_devices.vck,
                    CorePointerProc, CoreKeyboardProc, TRUE);
    inputInfo.pointer = local_devices.vcp;

    inputInfo.keyboard = local_devices.vck;
    ret = ActivateDevice(local_devices.vcp, FALSE);
    assert(ret == Success);
    /* This may fail if xkbcomp fails or xkb-config is not found. */
    ret = ActivateDevice(local_devices.vck, FALSE);
    assert(ret == Success);
    EnableDevice(local_devices.vcp, FALSE);
    EnableDevice(local_devices.vck, FALSE);

    AllocDevicePair(&client, "", &local_devices.mouse, &local_devices.kbd,
                    TestPointerProc, CoreKeyboardProc, FALSE);
    ret = ActivateDevice(local_devices.mouse, FALSE);
    assert(ret == Success);
    ret = ActivateDevice(local_devices.kbd, FALSE);
    assert(ret == Success);
    EnableDevice(local_devices.mouse, FALSE);
    EnableDevice(local_devices.kbd, FALSE);

    local_devices.num_devices = 4;
    local_devices.num_master_devices = 2;

    fake_init_sprite(local_devices.mouse);
    fake_init_sprite(local_devices.vcp);

    return local_devices;
}

/* Create minimal client, with the given buffer and len as request buffer */
ClientRec
init_client(int len, void *data)
{
    ClientRec client = { 0 };

    /* we store the privates now and reassign it after the memset. this way
     * we can share them across multiple test runs and don't have to worry
     * about freeing them after each test run. */

    client.index = CLIENT_INDEX;
    client.clientAsMask = CLIENT_MASK;
    client.sequence = CLIENT_SEQUENCE;
    client.req_len = len;

    client.requestBuffer = data;
    dixAllocatePrivates(&client.devPrivates, PRIVATE_CLIENT);
    return client;
}

void
init_window(WindowPtr local_window, WindowPtr parent, int id)
{
    memset(local_window, 0, sizeof(*local_window));

    local_window->drawable.id = id;
    if (parent) {
        local_window->drawable.x = 30;
        local_window->drawable.y = 50;
        local_window->drawable.width = 100;
        local_window->drawable.height = 200;
    }
    local_window->parent = parent;
    local_window->optional = calloc(1, sizeof(WindowOptRec));
    assert(local_window->optional);
}

extern DevPrivateKeyRec miPointerScreenKeyRec;
extern DevPrivateKeyRec miPointerPrivKeyRec;

/* Needed for the screen setup, otherwise we crash during sprite initialization */
static Bool
device_cursor_init(DeviceIntPtr dev, ScreenPtr local_screen)
{
    return TRUE;
}

static void
device_cursor_cleanup(DeviceIntPtr dev, ScreenPtr local_screen)
{
}

static Bool
set_cursor_pos(DeviceIntPtr dev, ScreenPtr local_screen, int x, int y, Bool event)
{
    return TRUE;
}

void
init_simple(void)
{
    screenInfo.numScreens = 1;
    screenInfo.screens[0] = &screen;

    screen.myNum = 0;
    screen.id = 100;
    screen.width = 640;
    screen.height = 480;
    screen.DeviceCursorInitialize = device_cursor_init;
    screen.DeviceCursorCleanup = device_cursor_cleanup;
    screen.SetCursorPosition = set_cursor_pos;
    screen.root = &root;

    dixResetPrivates();
    InitAtoms();
    XkbInitPrivates();
    dixRegisterPrivateKey(&XIClientPrivateKeyRec, PRIVATE_CLIENT,
                          sizeof(XIClientRec));
    dixRegisterPrivateKey(&miPointerScreenKeyRec, PRIVATE_SCREEN, 0);
    dixRegisterPrivateKey(&miPointerPrivKeyRec, PRIVATE_DEVICE, 0);
    XInputExtensionInit();

    init_window(&root, NULL, ROOT_WINDOW_ID);
    init_window(&window, &root, CLIENT_WINDOW_ID);

    serverClient = &server_client;
    InitClient(serverClient, 0, (void *) NULL);
    if (!InitClientResources(serverClient)) /* for root resources */
        FatalError("couldn't init server resources");
    SyncExtensionInit();

    devices = init_devices();
}

void
__wrap_WriteToClient(ClientPtr client, int len, void *data)
{
    assert(reply_handler != NULL);

    (*reply_handler) (client, len, data, global_userdata);
}

/* dixLookupWindow requires a lot of setup not necessary for this test.
 * Simple wrapper that returns either one of the fake root window or the
 * fake client window. If the requested ID is neither of those wanted,
 * return whatever the real dixLookupWindow does.
 */
int
__wrap_dixLookupWindow(WindowPtr *win, XID id, ClientPtr client, Mask access)
{
    if (id == root.drawable.id) {
        *win = &root;
        return Success;
    }
    else if (id == window.drawable.id) {
        *win = &window;
        return Success;
    }

    return __real_dixLookupWindow(win, id, client, access);
}

extern ClientRec client_window;

int
__wrap_dixLookupClient(ClientPtr *pClient, XID rid, ClientPtr client,
                       Mask access)
{
    if (rid == ROOT_WINDOW_ID)
        return BadWindow;

    if (rid == CLIENT_WINDOW_ID) {
        *pClient = &client_window;
        return Success;
    }

    return __real_dixLookupClient(pClient, rid, client, access);
}
