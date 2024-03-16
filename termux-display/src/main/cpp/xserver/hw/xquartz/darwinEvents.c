/*
 * Darwin event queue and event handling
 *
 * Copyright 2007-2008 Apple Inc.
 * Copyright 2004 Kaleb S. KEITHLEY. All Rights Reserved.
 * Copyright (c) 2002-2004 Torrey T. Lyons. All Rights Reserved.
 *
 * This file is based on mieq.c by Keith Packard,
 * which contains the following copyright:
 * Copyright 1990, 1998  The Open Group
 *
 *
 * Copyright (c) 2002-2012 Apple Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#include "sanitizedCarbon.h"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "inputstr.h"
#include "inpututils.h"
#include "eventstr.h"
#include "mi.h"
#include "scrnintstr.h"
#include "mipointer.h"
#include "os.h"
#include "exglobals.h"

#include "darwin.h"
#include "quartz.h"
#include "quartzKeyboard.h"
#include "quartzRandR.h"
#include "darwinEvents.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include <IOKit/hidsystem/IOLLEvent.h>

#include <X11/extensions/applewmconst.h>
#include "applewmExt.h"

/* FIXME: Abstract this better */
extern Bool
QuartzModeEventHandler(int screenNum, XQuartzEvent *e, DeviceIntPtr dev);

int darwin_all_modifier_flags = 0;  // last known modifier state
int darwin_all_modifier_mask = 0;
int darwin_x11_modifier_mask = 0;

#define FD_ADD_MAX 128
static int fd_add[FD_ADD_MAX];
int fd_add_count = 0;
static pthread_mutex_t fd_add_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t fd_add_ready_cond = PTHREAD_COND_INITIALIZER;
static pthread_t fd_add_tid = NULL;

static BOOL mieqInitialized;
static pthread_mutex_t mieqInitializedMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mieqInitializedCond = PTHREAD_COND_INITIALIZER;

_X_NOTSAN
extern inline void
wait_for_mieq_init(void)
{
    if (!mieqInitialized) {
        pthread_mutex_lock(&mieqInitializedMutex);
        while (!mieqInitialized) {
            pthread_cond_wait(&mieqInitializedCond, &mieqInitializedMutex);
        }
        pthread_mutex_unlock(&mieqInitializedMutex);
    }
}

_X_NOTSAN
static inline void
signal_mieq_init(void)
{
    pthread_mutex_lock(&mieqInitializedMutex);
    mieqInitialized = TRUE;
    pthread_cond_broadcast(&mieqInitializedCond);
    pthread_mutex_unlock(&mieqInitializedMutex);
}

/*** Pthread Magics ***/
static pthread_t
create_thread(void *(*func)(void *), void *arg)
{
    pthread_attr_t attr;
    pthread_t tid;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, func, arg);
    pthread_attr_destroy(&attr);

    return tid;
}

/*
 * DarwinPressModifierKey
 * Press or release the given modifier key (one of NX_MODIFIERKEY_* constants)
 */
static void
DarwinPressModifierKey(int pressed, int key)
{
    int keycode = DarwinModifierNXKeyToNXKeycode(key, 0);

    if (keycode == 0) {
        ErrorF("DarwinPressModifierKey bad keycode: key=%d\n", key);
        return;
    }

    DarwinSendKeyboardEvents(pressed, keycode);
}

/*
 * DarwinUpdateModifiers
 *  Send events to update the modifier state.
 */

static int darwin_x11_modifier_mask_list[] = {
#ifdef NX_DEVICELCMDKEYMASK
    NX_DEVICELCTLKEYMASK,   NX_DEVICERCTLKEYMASK,
    NX_DEVICELSHIFTKEYMASK, NX_DEVICERSHIFTKEYMASK,
    NX_DEVICELCMDKEYMASK,   NX_DEVICERCMDKEYMASK,
    NX_DEVICELALTKEYMASK,   NX_DEVICERALTKEYMASK,
#else
    NX_CONTROLMASK,         NX_SHIFTMASK,          NX_COMMANDMASK,
    NX_ALTERNATEMASK,
#endif
    NX_ALPHASHIFTMASK,
    0
};

static int darwin_all_modifier_mask_additions[] = { NX_SECONDARYFNMASK, 0 };

static void
DarwinUpdateModifiers(int pressed,                    // KeyPress or KeyRelease
                      int flags)                      // modifier flags that have changed
{
    int *f;
    int key;

    /* Capslock is special.  This mask is the state of capslock (on/off),
     * not the state of the button.  Hopefully we can find a better solution.
     */
    if (NX_ALPHASHIFTMASK & flags) {
        DarwinPressModifierKey(KeyPress, NX_MODIFIERKEY_ALPHALOCK);
        DarwinPressModifierKey(KeyRelease, NX_MODIFIERKEY_ALPHALOCK);
    }

    for (f = darwin_x11_modifier_mask_list; *f; f++)
        if (*f & flags && *f != NX_ALPHASHIFTMASK) {
            key = DarwinModifierNXMaskToNXKey(*f);
            if (key == -1)
                ErrorF("DarwinUpdateModifiers: Unsupported NXMask: 0x%x\n",
                       *f);
            else
                DarwinPressModifierKey(pressed, key);
        }
}

/* Generic handler for Xquartz-specifc events.  When possible, these should
   be moved into their own individual functions and set as handlers using
   mieqSetHandler. */

static void
DarwinEventHandler(int screenNum, InternalEvent *ie, DeviceIntPtr dev)
{
    XQuartzEvent *e = &(ie->xquartz_event);

    switch (e->subtype) {
    case kXquartzControllerNotify:
        DEBUG_LOG("kXquartzControllerNotify\n");
        AppleWMSendEvent(AppleWMControllerNotify,
                         AppleWMControllerNotifyMask,
                         e->data[0],
                         e->data[1]);
        break;

    case kXquartzPasteboardNotify:
        DEBUG_LOG("kXquartzPasteboardNotify\n");
        AppleWMSendEvent(AppleWMPasteboardNotify,
                         AppleWMPasteboardNotifyMask,
                         e->data[0],
                         e->data[1]);
        break;

    case kXquartzActivate:
        DEBUG_LOG("kXquartzActivate\n");
        QuartzShow();
        AppleWMSendEvent(AppleWMActivationNotify,
                         AppleWMActivationNotifyMask,
                         AppleWMIsActive, 0);
        break;

    case kXquartzDeactivate:
        DEBUG_LOG("kXquartzDeactivate\n");
        AppleWMSendEvent(AppleWMActivationNotify,
                         AppleWMActivationNotifyMask,
                         AppleWMIsInactive, 0);
        QuartzHide();
        break;

    case kXquartzReloadPreferences:
        DEBUG_LOG("kXquartzReloadPreferences\n");
        AppleWMSendEvent(AppleWMActivationNotify,
                         AppleWMActivationNotifyMask,
                         AppleWMReloadPreferences, 0);
        break;

    case kXquartzToggleFullscreen:
        DEBUG_LOG("kXquartzToggleFullscreen\n");
        if (XQuartzIsRootless)
            ErrorF(
                "Ignoring kXquartzToggleFullscreen because of rootless mode.");
        else
            QuartzRandRToggleFullscreen();
        break;

    case kXquartzSetRootless:
        DEBUG_LOG("kXquartzSetRootless\n");
        if (e->data[0]) {
            QuartzRandRSetFakeRootless();
        }
        else {
            QuartzRandRSetFakeFullscreen(FALSE);
        }
        break;

    case kXquartzSetRootClip:
        QuartzSetRootClip(e->data[0]);
        break;

    case kXquartzQuit:
        GiveUp(0);
        break;

    case kXquartzSpaceChanged:
        DEBUG_LOG("kXquartzSpaceChanged\n");
        QuartzSpaceChanged(e->data[0]);
        break;

    case kXquartzListenOnOpenFD:
        ErrorF("Calling ListenOnOpenFD() for new fd: %d\n", (int)e->data[0]);
        ListenOnOpenFD((int)e->data[0], 1);
        break;

    case kXquartzReloadKeymap:
        DarwinKeyboardReloadHandler();
        break;

    case kXquartzDisplayChanged:
        DEBUG_LOG("kXquartzDisplayChanged\n");
        QuartzUpdateScreens();

        /* Update our RandR info */
        QuartzRandRUpdateFakeModes(TRUE);
        break;

    default:
        if (!QuartzModeEventHandler(screenNum, e, dev))
            ErrorF("Unknown application defined event type %d.\n", e->subtype);
    }
}

void
DarwinListenOnOpenFD(int fd)
{
    ErrorF("DarwinListenOnOpenFD: %d\n", fd);

    pthread_mutex_lock(&fd_add_lock);
    if (fd_add_count < FD_ADD_MAX)
        fd_add[fd_add_count++] = fd;
    else
        ErrorF("FD Addition buffer at max.  Dropping fd addition request.\n");

    pthread_cond_broadcast(&fd_add_ready_cond);
    pthread_mutex_unlock(&fd_add_lock);
}

static void *
DarwinProcessFDAdditionQueue_thread(void *args)
{
    /* TODO: Possibly adjust this to no longer be a race... maybe trigger this
     *       once a client connects and claims to be the WM.
     *
     * From ajax:
     * There's already an internal callback chain for setting selection [in 1.5]
     * ownership.  See the CallSelectionCallback at the bottom of
     * ProcSetSelectionOwner, and xfixes/select.c for an example of how to hook
     * into it.
     */

    struct timespec sleep_for;
    struct timespec sleep_remaining;

    sleep_for.tv_sec = 3;
    sleep_for.tv_nsec = 0;

    ErrorF(
        "X11.app: DarwinProcessFDAdditionQueue_thread: Sleeping to allow xinitrc to catchup.\n");
    while (nanosleep(&sleep_for, &sleep_remaining) != 0) {
        sleep_for = sleep_remaining;
    }

    pthread_mutex_lock(&fd_add_lock);
    while (true) {
        while (fd_add_count) {
            DarwinSendDDXEvent(kXquartzListenOnOpenFD, 1,
                               fd_add[--fd_add_count]);
        }
        pthread_cond_wait(&fd_add_ready_cond, &fd_add_lock);
    }

    return NULL;
}

Bool
DarwinEQInit(void)
{
    int *p;

    for (p = darwin_x11_modifier_mask_list; *p; p++) {
        darwin_x11_modifier_mask |= *p;
    }

    darwin_all_modifier_mask = darwin_x11_modifier_mask;
    for (p = darwin_all_modifier_mask_additions; *p; p++) {
        darwin_all_modifier_mask |= *p;
    }

    mieqInit();
    mieqSetHandler(ET_XQuartz, DarwinEventHandler);

    if (!fd_add_tid)
        fd_add_tid = create_thread(DarwinProcessFDAdditionQueue_thread, NULL);

    signal_mieq_init();

    return TRUE;
}

void
DarwinEQFini(void)
{
    mieqFini();
}

/*
 * ProcessInputEvents
 *  Read and process events from the event queue until it is empty.
 */
void
ProcessInputEvents(void)
{
    char nullbyte;
    int x = sizeof(nullbyte);

    mieqProcessInputEvents();

    // Empty the signaling pipe
    while (x == sizeof(nullbyte)) {
        x = read(darwinEventReadFD, &nullbyte, sizeof(nullbyte));
    }
}

/* Sends a null byte down darwinEventWriteFD, which will cause the
   Dispatch() event loop to check out event queue */
static void
DarwinPokeEQ(void)
{
    char nullbyte = 0;
    //  <daniels> oh, i ... er ... christ.
    write(darwinEventWriteFD, &nullbyte, sizeof(nullbyte));
}

void
DarwinInputReleaseButtonsAndKeys(DeviceIntPtr pDev)
{
    input_lock();
    {
        int i;
        if (pDev->button) {
            for (i = 0; i < pDev->button->numButtons; i++) {
                if (BitIsOn(pDev->button->down, i)) {
                    QueuePointerEvents(pDev, ButtonRelease, i,
                                       POINTER_ABSOLUTE,
                                       NULL);
                }
            }
        }

        if (pDev->key) {
            for (i = 0; i < NUM_KEYCODES; i++) {
                if (BitIsOn(pDev->key->down, i + MIN_KEYCODE)) {
                    QueueKeyboardEvents(pDev, KeyRelease, i + MIN_KEYCODE);
                }
            }
        }
        DarwinPokeEQ();
    } input_unlock();
}

void
DarwinSendTabletEvents(DeviceIntPtr pDev, int ev_type, int ev_button,
                       double pointer_x, double pointer_y,
                       double pressure, double tilt_x,
                       double tilt_y)
{
    ScreenPtr screen;
    ValuatorMask valuators;

    screen = miPointerGetScreen(pDev);
    if (!screen) {
        DEBUG_LOG("%s called before screen was initialized\n",
                  __FUNCTION__);
        return;
    }

    /* Fix offset between darwin and X screens */
    pointer_x -= darwinMainScreenX + screen->x;
    pointer_y -= darwinMainScreenY + screen->y;

    /* Adjust our pointer location to the [0,1] range */
    pointer_x = pointer_x / (double)screenInfo.width;
    pointer_y = pointer_y / (double)screenInfo.height;

    valuator_mask_zero(&valuators);
    valuator_mask_set_double(&valuators, 0, XQUARTZ_VALUATOR_LIMIT * pointer_x);
    valuator_mask_set_double(&valuators, 1, XQUARTZ_VALUATOR_LIMIT * pointer_y);
    valuator_mask_set_double(&valuators, 2, XQUARTZ_VALUATOR_LIMIT * pressure);
    valuator_mask_set_double(&valuators, 3, XQUARTZ_VALUATOR_LIMIT * tilt_x);
    valuator_mask_set_double(&valuators, 4, XQUARTZ_VALUATOR_LIMIT * tilt_y);

    input_lock();
    {
        if (ev_type == ProximityIn || ev_type == ProximityOut) {
            QueueProximityEvents(pDev, ev_type, &valuators);
        } else {
            QueuePointerEvents(pDev, ev_type, ev_button, POINTER_ABSOLUTE,
                               &valuators);
        }
        DarwinPokeEQ();
    } input_unlock();
}

void
DarwinSendPointerEvents(DeviceIntPtr pDev, int ev_type, int ev_button,
                        double pointer_x, double pointer_y,
                        double pointer_dx, double pointer_dy)
{
    static int darwinFakeMouseButtonDown = 0;
    ScreenPtr screen;
    ValuatorMask valuators;

    screen = miPointerGetScreen(pDev);
    if (!screen) {
        DEBUG_LOG("%s called before screen was initialized\n",
                  __FUNCTION__);
        return;
    }

    /* Handle fake click */
    if (ev_type == ButtonPress && darwinFakeButtons && ev_button == 1) {
        if (darwinFakeMouseButtonDown != 0) {
            /* We're currently "down" with another button, so release it first */
            DarwinSendPointerEvents(pDev, ButtonRelease,
                                    darwinFakeMouseButtonDown,
                                    pointer_x, pointer_y, 0.0, 0.0);
            darwinFakeMouseButtonDown = 0;
        }
        if (darwin_all_modifier_flags & darwinFakeMouse2Mask) {
            ev_button = 2;
            darwinFakeMouseButtonDown = 2;
            DarwinUpdateModKeys(
                darwin_all_modifier_flags & ~darwinFakeMouse2Mask);
        }
        else if (darwin_all_modifier_flags & darwinFakeMouse3Mask) {
            ev_button = 3;
            darwinFakeMouseButtonDown = 3;
            DarwinUpdateModKeys(
                darwin_all_modifier_flags & ~darwinFakeMouse3Mask);
        }
    }

    if (ev_type == ButtonRelease && ev_button == 1) {
        if (darwinFakeMouseButtonDown) {
            ev_button = darwinFakeMouseButtonDown;
        }

        if (darwinFakeMouseButtonDown == 2) {
            DarwinUpdateModKeys(
                darwin_all_modifier_flags & ~darwinFakeMouse2Mask);
        }
        else if (darwinFakeMouseButtonDown == 3) {
            DarwinUpdateModKeys(
                darwin_all_modifier_flags & ~darwinFakeMouse3Mask);
        }

        darwinFakeMouseButtonDown = 0;
    }

    /* Fix offset between darwin and X screens */
    pointer_x -= darwinMainScreenX + screen->x;
    pointer_y -= darwinMainScreenY + screen->y;

    valuator_mask_zero(&valuators);
    valuator_mask_set_double(&valuators, 0, pointer_x);
    valuator_mask_set_double(&valuators, 1, pointer_y);

    if (ev_type == MotionNotify) {
        if (pointer_dx != 0.0)
            valuator_mask_set_double(&valuators, 2, pointer_dx);
        if (pointer_dy != 0.0)
            valuator_mask_set_double(&valuators, 3, pointer_dy);
    }

    input_lock();
    {
        QueuePointerEvents(pDev, ev_type, ev_button, POINTER_ABSOLUTE,
                           &valuators);
        DarwinPokeEQ();
    } input_unlock();
}

void
DarwinSendKeyboardEvents(int ev_type, int keycode)
{
    input_lock();
    {
        QueueKeyboardEvents(darwinKeyboard, ev_type, keycode + MIN_KEYCODE);
        DarwinPokeEQ();
    } input_unlock();
}

/* Send the appropriate number of button clicks to emulate scroll wheel */
void
DarwinSendScrollEvents(double scroll_x, double scroll_y) {
    ScreenPtr screen;
    ValuatorMask valuators;

    screen = miPointerGetScreen(darwinPointer);
    if (!screen) {
        DEBUG_LOG(
            "DarwinSendScrollEvents called before screen was initialized\n");
        return;
    }

    valuator_mask_zero(&valuators);
    valuator_mask_set_double(&valuators, 4, scroll_y);
    valuator_mask_set_double(&valuators, 5, scroll_x);

    input_lock();
    {
        QueuePointerEvents(darwinPointer, MotionNotify, 0,
                           POINTER_RELATIVE, &valuators);
        DarwinPokeEQ();
    } input_unlock();
}

/* Send the appropriate KeyPress/KeyRelease events to GetKeyboardEvents to
   reflect changing modifier flags (alt, control, meta, etc) */
void
DarwinUpdateModKeys(int flags)
{
    DarwinUpdateModifiers(
        KeyRelease, darwin_all_modifier_flags & ~flags &
        darwin_x11_modifier_mask);
    DarwinUpdateModifiers(
        KeyPress, ~darwin_all_modifier_flags & flags &
        darwin_x11_modifier_mask);
    darwin_all_modifier_flags = flags;
}

/*
 * DarwinSendDDXEvent
 *  Send the X server thread a message by placing it on the event queue.
 */
void
DarwinSendDDXEvent(int type, int argc, ...)
{
    XQuartzEvent e;
    int i;
    va_list args;

    memset(&e, 0, sizeof(e));
    e.header = ET_Internal;
    e.type = ET_XQuartz;
    e.length = sizeof(e);
    e.time = GetTimeInMillis();
    e.subtype = type;

    if (argc > 0 && argc < XQUARTZ_EVENT_MAXARGS) {
        va_start(args, argc);
        for (i = 0; i < argc; i++)
            e.data[i] = (uint32_t)va_arg(args, uint32_t);
        va_end(args);
    }

    wait_for_mieq_init();

    input_lock();
    {
        mieqEnqueue(NULL, (InternalEvent *)&e);
        DarwinPokeEQ();
    } input_unlock();
}
