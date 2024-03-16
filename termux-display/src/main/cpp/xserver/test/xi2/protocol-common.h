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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "scrnintstr.h"
#include "windowstr.h"
#include "exevents.h"
#include <assert.h>

#include "tests.h"

#ifndef PROTOCOL_COMMON_H
#define PROTOCOL_COMMON_H

/* Check default values in a reply */
#define reply_check_defaults(rep, len, type) \
    { \
        assert((len) >= sz_x##type##Reply); \
        assert((rep)->repType == X_Reply); \
        assert((rep)->RepType == X_##type); \
        assert((rep)->sequenceNumber == CLIENT_SEQUENCE); \
        assert((rep)->length >= (sz_x##type##Reply - 32)/4); \
    }

/* initialise default values for request */
#define request_init(req, type) \
    { \
        (req)->reqType = 128; /* doesn't matter */ \
        (req)->ReqType = X_##type; \
        (req)->length = (sz_x##type##Req >> 2); \
    }

/* Various defines used in the tests. Some tests may use different values
 * than these defaults */
/* default client index */
#define CLIENT_INDEX            1
/* default client mask for resources and windows */
#define CLIENT_MASK             ((CLIENT_INDEX) << CLIENTOFFSET)
/* default client sequence number for replies */
#define CLIENT_SEQUENCE         0x100
/* default root window id */
#define ROOT_WINDOW_ID          0x10
/* default client window id */
#define CLIENT_WINDOW_ID        0x100001
/* invalid window ID. use for BadWindow checks. */
#define INVALID_WINDOW_ID       0x111111
/* initial fake sprite position */
#define SPRITE_X                100
#define SPRITE_Y                200

/* Various structs used throughout the tests */

/* The default devices struct, contains one pointer + keyboard and the
 * matching master devices. Initialize with init_devices() if needed. */
struct devices {
    DeviceIntPtr vcp;
    DeviceIntPtr vck;
    DeviceIntPtr mouse;
    DeviceIntPtr kbd;

    int num_devices;
    int num_master_devices;
};

/**
 * The set of default devices available in all tests if necessary.
 */
extern struct devices devices;

/**
 * test-specific userdata, passed into the reply handler.
 */
extern void *global_userdata;

/**
 * The reply handler called from WriteToClient. Set this handler if you need
 * to check the reply values.
 */
extern void (*reply_handler) (ClientPtr client, int len, char *data, void *userdata);

/**
 * The default screen used for the windows. Initialized by init_simple().
 */
extern ScreenRec screen;

/**
 * Semi-initialized root window. initialized by init().
 */
extern WindowRec root;

/**
 * Semi-initialized top-level window. initialized by init().
 */
extern WindowRec window;

/* various simple functions for quick setup */
/**
 * Initialize the above struct with default devices and return the struct.
 * Usually not needed if you call ::init_simple.
 */
struct devices init_devices(void);

/**
 * Init a mostly zeroed out client with default values for index and mask.
 */
ClientRec init_client(int request_len, void *request_data);

/**
 * Init a mostly zeroed out window with the given window ID.
 * Usually not needed if you call ::init_simple which sets up root and
 * window.
 */
void init_window(WindowPtr window, WindowPtr parent, int id);

/**
 * Create a very simple setup that provides the minimum values for most
 * tests, including a screen, the root and client window and the default
 * device setup.
 */
void init_simple(void);

/* Declarations for various overrides in the test files. */
void __wrap_WriteToClient(ClientPtr client, int len, void *data);
int __wrap_XISetEventMask(DeviceIntPtr dev, WindowPtr win, ClientPtr client,
                          int len, unsigned char *mask);
int __wrap_dixLookupWindow(WindowPtr *win, XID id, ClientPtr client,
                           Mask access);
int __real_dixLookupWindow(WindowPtr *win, XID id, ClientPtr client,
                           Mask access);
Bool __wrap_AddResource(XID id, RESTYPE type, void *value);
int __wrap_dixLookupClient(ClientPtr *c, XID id, ClientPtr client, Mask access);
int __real_dixLookupClient(ClientPtr *c, XID id, ClientPtr client, Mask access);

#endif                          /* PROTOCOL_COMMON_H */
