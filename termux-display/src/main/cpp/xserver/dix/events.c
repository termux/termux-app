/************************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/

/*
 * Copyright (c) 2003-2005, Oracle and/or its affiliates. All rights reserved.
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

/** @file events.c
 * This file handles event delivery and a big part of the server-side protocol
 * handling (the parts for input devices).
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "misc.h"
#include "resource.h"
#include <X11/Xproto.h>
#include "windowstr.h"
#include "inputstr.h"
#include "inpututils.h"
#include "scrnintstr.h"
#include "cursorstr.h"

#include "dixstruct.h"
#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif
#include "globals.h"

#include <X11/extensions/XKBproto.h>
#include "xkbsrv.h"
#include "xace.h"
#include "probes.h"

#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2.h>
#include "exglobals.h"
#include "exevents.h"
#include "extnsionst.h"

#include "dixevents.h"
#include "dixgrabs.h"
#include "dispatch.h"

#include <X11/extensions/ge.h>
#include "geext.h"
#include "geint.h"

#include "eventstr.h"
#include "enterleave.h"
#include "eventconvert.h"
#include "mi.h"

/* Extension events type numbering starts at EXTENSION_EVENT_BASE.  */
#define NoSuchEvent 0x80000000  /* so doesn't match NoEventMask */
#define StructureAndSubMask ( StructureNotifyMask | SubstructureNotifyMask )
#define AllButtonsMask ( \
	Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask )
#define MotionMask ( \
	PointerMotionMask | Button1MotionMask | \
	Button2MotionMask | Button3MotionMask | Button4MotionMask | \
	Button5MotionMask | ButtonMotionMask )
#define PropagateMask ( \
	KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | \
	MotionMask )
#define PointerGrabMask ( \
	ButtonPressMask | ButtonReleaseMask | \
	EnterWindowMask | LeaveWindowMask | \
	PointerMotionHintMask | KeymapStateMask | \
	MotionMask )
#define AllModifiersMask ( \
	ShiftMask | LockMask | ControlMask | Mod1Mask | Mod2Mask | \
	Mod3Mask | Mod4Mask | Mod5Mask )
#define LastEventMask OwnerGrabButtonMask
#define AllEventMasks (LastEventMask|(LastEventMask-1))

/* @return the core event type or 0 if the event is not a core event */
static inline int
core_get_type(const xEvent *event)
{
    int type = event->u.u.type;

    return ((type & EXTENSION_EVENT_BASE) || type == GenericEvent) ? 0 : type;
}

/* @return the XI2 event type or 0 if the event is not a XI2 event */
static inline int
xi2_get_type(const xEvent *event)
{
    const xGenericEvent *e = (const xGenericEvent *) event;

    return (e->type != GenericEvent ||
            e->extension != IReqCode) ? 0 : e->evtype;
}

/**
 * Used to indicate a implicit passive grab created by a ButtonPress event.
 * See DeliverEventsToWindow().
 */
#define ImplicitGrabMask (1 << 7)

#define WID(w) ((w) ? ((w)->drawable.id) : 0)

#define XE_KBPTR (xE->u.keyButtonPointer)

CallbackListPtr EventCallback;
CallbackListPtr DeviceEventCallback;

#define DNPMCOUNT 8

Mask DontPropagateMasks[DNPMCOUNT];
static int DontPropagateRefCnts[DNPMCOUNT];

static void CheckVirtualMotion(DeviceIntPtr pDev, QdEventPtr qe,
                               WindowPtr pWin);
static void CheckPhysLimits(DeviceIntPtr pDev, CursorPtr cursor,
                            Bool generateEvents, Bool confineToScreen,
                            ScreenPtr pScreen);
static Bool IsWrongPointerBarrierClient(ClientPtr client,
                                        DeviceIntPtr dev,
                                        xEvent *event);

/** Key repeat hack. Do not use but in TryClientEvents */
extern BOOL EventIsKeyRepeat(xEvent *event);

/**
 * Main input device struct.
 *     inputInfo.pointer
 *     is the core pointer. Referred to as "virtual core pointer", "VCP",
 *     "core pointer" or inputInfo.pointer. The VCP is the first master
 *     pointer device and cannot be deleted.
 *
 *     inputInfo.keyboard
 *     is the core keyboard ("virtual core keyboard", "VCK", "core keyboard").
 *     See inputInfo.pointer.
 *
 *     inputInfo.devices
 *     linked list containing all devices including VCP and VCK.
 *
 *     inputInfo.off_devices
 *     Devices that have not been initialized and are thus turned off.
 *
 *     inputInfo.numDevices
 *     Total number of devices.
 *
 *     inputInfo.all_devices
 *     Virtual device used for XIAllDevices passive grabs. This device is
 *     not part of the inputInfo.devices list and mostly unset except for
 *     the deviceid. It exists because passivegrabs need a valid device
 *     reference.
 *
 *     inputInfo.all_master_devices
 *     Virtual device used for XIAllMasterDevices passive grabs. This device
 *     is not part of the inputInfo.devices list and mostly unset except for
 *     the deviceid. It exists because passivegrabs need a valid device
 *     reference.
 */
InputInfo inputInfo;

EventSyncInfoRec syncEvents;

static struct DeviceEventTime {
    Bool reset;
    TimeStamp time;
} lastDeviceEventTime[MAXDEVICES];

/**
 * The root window the given device is currently on.
 */
#define RootWindow(sprite) sprite->spriteTrace[0]

static xEvent *swapEvent = NULL;
static int swapEventLen = 0;

void
NotImplemented(xEvent *from, xEvent *to)
{
    FatalError("Not implemented");
}

/**
 * Convert the given event type from an XI event to a core event.
 * @param[in] The XI 1.x event type.
 * @return The matching core event type or 0 if there is none.
 */
int
XItoCoreType(int xitype)
{
    int coretype = 0;

    if (xitype == DeviceMotionNotify)
        coretype = MotionNotify;
    else if (xitype == DeviceButtonPress)
        coretype = ButtonPress;
    else if (xitype == DeviceButtonRelease)
        coretype = ButtonRelease;
    else if (xitype == DeviceKeyPress)
        coretype = KeyPress;
    else if (xitype == DeviceKeyRelease)
        coretype = KeyRelease;

    return coretype;
}

/**
 * @return true if the device owns a cursor, false if device shares a cursor
 * sprite with another device.
 */
Bool
DevHasCursor(DeviceIntPtr pDev)
{
    return pDev->spriteInfo->spriteOwner;
}

/*
 * @return true if a device is a pointer, check is the same as used by XI to
 * fill the 'use' field.
 */
Bool
IsPointerDevice(DeviceIntPtr dev)
{
    return (dev->type == MASTER_POINTER) ||
        (dev->valuator && dev->button) || (dev->valuator && !dev->key);
}

/*
 * @return true if a device is a keyboard, check is the same as used by XI to
 * fill the 'use' field.
 *
 * Some pointer devices have keys as well (e.g. multimedia keys). Try to not
 * count them as keyboard devices.
 */
Bool
IsKeyboardDevice(DeviceIntPtr dev)
{
    return (dev->type == MASTER_KEYBOARD) ||
        ((dev->key && dev->kbdfeed) && !IsPointerDevice(dev));
}

Bool
IsMaster(DeviceIntPtr dev)
{
    return dev->type == MASTER_POINTER || dev->type == MASTER_KEYBOARD;
}

Bool
IsFloating(DeviceIntPtr dev)
{
    return !IsMaster(dev) && GetMaster(dev, MASTER_KEYBOARD) == NULL;
}

/**
 * Max event opcode.
 */
extern int lastEvent;

#define CantBeFiltered NoEventMask
/**
 * Event masks for each event type.
 *
 * One set of filters for each device, initialized by memcpy of
 * default_filter in InitEvents.
 *
 * Filters are used whether a given event may be delivered to a client,
 * usually in the form of if (window-event-mask & filter); then deliver event.
 *
 * One notable filter is for PointerMotion/DevicePointerMotion events. Each
 * time a button is pressed, the filter is modified to also contain the
 * matching ButtonXMotion mask.
 */
Mask event_filters[MAXDEVICES][MAXEVENTS];

static const Mask default_filter[MAXEVENTS] = {
    NoSuchEvent,                /* 0 */
    NoSuchEvent,                /* 1 */
    KeyPressMask,               /* KeyPress */
    KeyReleaseMask,             /* KeyRelease */
    ButtonPressMask,            /* ButtonPress */
    ButtonReleaseMask,          /* ButtonRelease */
    PointerMotionMask,          /* MotionNotify (initial state) */
    EnterWindowMask,            /* EnterNotify */
    LeaveWindowMask,            /* LeaveNotify */
    FocusChangeMask,            /* FocusIn */
    FocusChangeMask,            /* FocusOut */
    KeymapStateMask,            /* KeymapNotify */
    ExposureMask,               /* Expose */
    CantBeFiltered,             /* GraphicsExpose */
    CantBeFiltered,             /* NoExpose */
    VisibilityChangeMask,       /* VisibilityNotify */
    SubstructureNotifyMask,     /* CreateNotify */
    StructureAndSubMask,        /* DestroyNotify */
    StructureAndSubMask,        /* UnmapNotify */
    StructureAndSubMask,        /* MapNotify */
    SubstructureRedirectMask,   /* MapRequest */
    StructureAndSubMask,        /* ReparentNotify */
    StructureAndSubMask,        /* ConfigureNotify */
    SubstructureRedirectMask,   /* ConfigureRequest */
    StructureAndSubMask,        /* GravityNotify */
    ResizeRedirectMask,         /* ResizeRequest */
    StructureAndSubMask,        /* CirculateNotify */
    SubstructureRedirectMask,   /* CirculateRequest */
    PropertyChangeMask,         /* PropertyNotify */
    CantBeFiltered,             /* SelectionClear */
    CantBeFiltered,             /* SelectionRequest */
    CantBeFiltered,             /* SelectionNotify */
    ColormapChangeMask,         /* ColormapNotify */
    CantBeFiltered,             /* ClientMessage */
    CantBeFiltered              /* MappingNotify */
};

/**
 * For the given event, return the matching event filter. This filter may then
 * be AND'ed with the selected event mask.
 *
 * For XI2 events, the returned filter is simply the byte containing the event
 * mask we're interested in. E.g. for a mask of (1 << 13), this would be
 * byte[1].
 *
 * @param[in] dev The device the event belongs to, may be NULL.
 * @param[in] event The event to get the filter for. Only the type of the
 *                  event matters, or the extension + evtype for GenericEvents.
 * @return The filter mask for the given event.
 *
 * @see GetEventMask
 */
Mask
GetEventFilter(DeviceIntPtr dev, xEvent *event)
{
    int evtype = 0;

    if (event->u.u.type != GenericEvent)
        return event_get_filter_from_type(dev, event->u.u.type);
    else if ((evtype = xi2_get_type(event)))
        return event_get_filter_from_xi2type(evtype);
    ErrorF("[dix] Unknown event type %d. No filter\n", event->u.u.type);
    return 0;
}

/**
 * Return the single byte of the device's XI2 mask that contains the mask
 * for the event_type.
 */
int
GetXI2MaskByte(XI2Mask *mask, DeviceIntPtr dev, int event_type)
{
    /* we just return the matching filter because that's the only use
     * for this mask anyway.
     */
    if (xi2mask_isset(mask, dev, event_type))
        return event_get_filter_from_xi2type(event_type);
    else
        return 0;
}

/**
 * @return TRUE if the mask is set for this event from this device on the
 * window, or FALSE otherwise.
 */
Bool
WindowXI2MaskIsset(DeviceIntPtr dev, WindowPtr win, xEvent *ev)
{
    OtherInputMasks *inputMasks = wOtherInputMasks(win);
    int evtype;

    if (!inputMasks || xi2_get_type(ev) == 0)
        return 0;

    evtype = ((xGenericEvent *) ev)->evtype;

    return xi2mask_isset(inputMasks->xi2mask, dev, evtype);
}

/**
 * When processing events we operate on InternalEvent pointers. They may actually refer to a
 * an instance of DeviceEvent, GestureEvent or any other event that comprises the InternalEvent
 * union. This works well in practice because we always look into event type before doing anything,
 * except in the case of copying the event. Any copying of InternalEvent should use this function
 * instead of doing *dst_event = *src_event whenever it's not clear whether source event actually
 * points to full InternalEvent instance.
 */
void
CopyPartialInternalEvent(InternalEvent* dst_event, const InternalEvent* src_event)
{
    memcpy(dst_event, src_event, src_event->any.length);
}

Mask
GetEventMask(DeviceIntPtr dev, xEvent *event, InputClients * other)
{
    int evtype;

    /* XI2 filters are only ever 8 bit, so let's return a 8 bit mask */
    if ((evtype = xi2_get_type(event))) {
        return GetXI2MaskByte(other->xi2mask, dev, evtype);
    }
    else if (core_get_type(event) != 0)
        return other->mask[XIAllDevices];
    else
        return other->mask[dev->id];
}

static CARD8 criticalEvents[32] = {
    0x7c, 0x30, 0x40            /* key, button, expose, and configure events */
};

static void
SyntheticMotion(DeviceIntPtr dev, int x, int y)
{
    int screenno = 0;

#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        screenno = dev->spriteInfo->sprite->screen->myNum;
#endif
    PostSyntheticMotion(dev, x, y, screenno,
                        (syncEvents.playingEvents) ? syncEvents.time.
                        milliseconds : currentTime.milliseconds);

}

#ifdef PANORAMIX
static void PostNewCursor(DeviceIntPtr pDev);

static Bool
XineramaSetCursorPosition(DeviceIntPtr pDev, int x, int y, Bool generateEvent)
{
    ScreenPtr pScreen;
    int i;
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    /* x,y are in Screen 0 coordinates.  We need to decide what Screen
       to send the message too and what the coordinates relative to
       that screen are. */

    pScreen = pSprite->screen;
    x += screenInfo.screens[0]->x;
    y += screenInfo.screens[0]->y;

    if (!point_on_screen(pScreen, x, y)) {
        FOR_NSCREENS(i) {
            if (i == pScreen->myNum)
                continue;
            if (point_on_screen(screenInfo.screens[i], x, y)) {
                pScreen = screenInfo.screens[i];
                break;
            }
        }
    }

    pSprite->screen = pScreen;
    pSprite->hotPhys.x = x - screenInfo.screens[0]->x;
    pSprite->hotPhys.y = y - screenInfo.screens[0]->y;
    x -= pScreen->x;
    y -= pScreen->y;

    return (*pScreen->SetCursorPosition) (pDev, pScreen, x, y, generateEvent);
}

static void
XineramaConstrainCursor(DeviceIntPtr pDev)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;
    ScreenPtr pScreen;
    BoxRec newBox;

    pScreen = pSprite->screen;
    newBox = pSprite->physLimits;

    /* Translate the constraining box to the screen
       the sprite is actually on */
    newBox.x1 += screenInfo.screens[0]->x - pScreen->x;
    newBox.x2 += screenInfo.screens[0]->x - pScreen->x;
    newBox.y1 += screenInfo.screens[0]->y - pScreen->y;
    newBox.y2 += screenInfo.screens[0]->y - pScreen->y;

    (*pScreen->ConstrainCursor) (pDev, pScreen, &newBox);
}

static Bool
XineramaSetWindowPntrs(DeviceIntPtr pDev, WindowPtr pWin)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if (pWin == screenInfo.screens[0]->root) {
        int i;

        FOR_NSCREENS(i)
            pSprite->windows[i] = screenInfo.screens[i]->root;
    }
    else {
        PanoramiXRes *win;
        int rc, i;

        rc = dixLookupResourceByType((void **) &win, pWin->drawable.id,
                                     XRT_WINDOW, serverClient, DixReadAccess);
        if (rc != Success)
            return FALSE;

        FOR_NSCREENS(i) {
            rc = dixLookupWindow(pSprite->windows + i, win->info[i].id,
                                 serverClient, DixReadAccess);
            if (rc != Success)  /* window is being unmapped */
                return FALSE;
        }
    }
    return TRUE;
}

static void
XineramaConfineCursorToWindow(DeviceIntPtr pDev,
                              WindowPtr pWin, Bool generateEvents)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    int x, y, off_x, off_y, i;

    assert(!noPanoramiXExtension);

    if (!XineramaSetWindowPntrs(pDev, pWin))
        return;

    i = PanoramiXNumScreens - 1;

    RegionCopy(&pSprite->Reg1, &pSprite->windows[i]->borderSize);
    off_x = screenInfo.screens[i]->x;
    off_y = screenInfo.screens[i]->y;

    while (i--) {
        x = off_x - screenInfo.screens[i]->x;
        y = off_y - screenInfo.screens[i]->y;

        if (x || y)
            RegionTranslate(&pSprite->Reg1, x, y);

        RegionUnion(&pSprite->Reg1, &pSprite->Reg1,
                    &pSprite->windows[i]->borderSize);

        off_x = screenInfo.screens[i]->x;
        off_y = screenInfo.screens[i]->y;
    }

    pSprite->hotLimits = *RegionExtents(&pSprite->Reg1);

    if (RegionNumRects(&pSprite->Reg1) > 1)
        pSprite->hotShape = &pSprite->Reg1;
    else
        pSprite->hotShape = NullRegion;

    pSprite->confined = FALSE;
    pSprite->confineWin =
        (pWin == screenInfo.screens[0]->root) ? NullWindow : pWin;

    CheckPhysLimits(pDev, pSprite->current, generateEvents, FALSE, NULL);
}

#endif                          /* PANORAMIX */

/**
 * Modifies the filter for the given protocol event type to the given masks.
 *
 * There's only two callers: UpdateDeviceState() and XI's SetMaskForExtEvent().
 * The latter initialises masks for the matching XI events, it's a once-off
 * setting.
 * UDS however changes the mask for MotionNotify and DeviceMotionNotify each
 * time a button is pressed to include the matching ButtonXMotion mask in the
 * filter.
 *
 * @param[in] deviceid The device to modify the filter for.
 * @param[in] mask The new filter mask.
 * @param[in] event Protocol event type.
 */
void
SetMaskForEvent(int deviceid, Mask mask, int event)
{
    if (deviceid < 0 || deviceid >= MAXDEVICES)
        FatalError("SetMaskForEvent: bogus device id");
    event_filters[deviceid][event] = mask;
}

void
SetCriticalEvent(int event)
{
    if (event >= MAXEVENTS)
        FatalError("SetCriticalEvent: bogus event number");
    criticalEvents[event >> 3] |= 1 << (event & 7);
}

void
ConfineToShape(DeviceIntPtr pDev, RegionPtr shape, int *px, int *py)
{
    BoxRec box;
    int x = *px, y = *py;
    int incx = 1, incy = 1;

    if (RegionContainsPoint(shape, x, y, &box))
        return;
    box = *RegionExtents(shape);
    /* this is rather crude */
    do {
        x += incx;
        if (x >= box.x2) {
            incx = -1;
            x = *px - 1;
        }
        else if (x < box.x1) {
            incx = 1;
            x = *px;
            y += incy;
            if (y >= box.y2) {
                incy = -1;
                y = *py - 1;
            }
            else if (y < box.y1)
                return;         /* should never get here! */
        }
    } while (!RegionContainsPoint(shape, x, y, &box));
    *px = x;
    *py = y;
}

static void
CheckPhysLimits(DeviceIntPtr pDev, CursorPtr cursor, Bool generateEvents,
                Bool confineToScreen, /* unused if PanoramiX on */
                ScreenPtr pScreen)    /* unused if PanoramiX on */
{
    HotSpot new;
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if (!cursor)
        return;
    new = pSprite->hotPhys;
#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        /* I don't care what the DDX has to say about it */
        pSprite->physLimits = pSprite->hotLimits;
    else
#endif
    {
        if (pScreen)
            new.pScreen = pScreen;
        else
            pScreen = new.pScreen;
        (*pScreen->CursorLimits) (pDev, pScreen, cursor, &pSprite->hotLimits,
                                  &pSprite->physLimits);
        pSprite->confined = confineToScreen;
        (*pScreen->ConstrainCursor) (pDev, pScreen, &pSprite->physLimits);
    }

    /* constrain the pointer to those limits */
    if (new.x < pSprite->physLimits.x1)
        new.x = pSprite->physLimits.x1;
    else if (new.x >= pSprite->physLimits.x2)
        new.x = pSprite->physLimits.x2 - 1;
    if (new.y < pSprite->physLimits.y1)
        new.y = pSprite->physLimits.y1;
    else if (new.y >= pSprite->physLimits.y2)
        new.y = pSprite->physLimits.y2 - 1;
    if (pSprite->hotShape)
        ConfineToShape(pDev, pSprite->hotShape, &new.x, &new.y);
    if ((
#ifdef PANORAMIX
            noPanoramiXExtension &&
#endif
            (pScreen != pSprite->hotPhys.pScreen)) ||
        (new.x != pSprite->hotPhys.x) || (new.y != pSprite->hotPhys.y)) {
#ifdef PANORAMIX
        if (!noPanoramiXExtension)
            XineramaSetCursorPosition(pDev, new.x, new.y, generateEvents);
        else
#endif
        {
            if (pScreen != pSprite->hotPhys.pScreen)
                pSprite->hotPhys = new;
            (*pScreen->SetCursorPosition)
                (pDev, pScreen, new.x, new.y, generateEvents);
        }
        if (!generateEvents)
            SyntheticMotion(pDev, new.x, new.y);
    }

#ifdef PANORAMIX
    /* Tell DDX what the limits are */
    if (!noPanoramiXExtension)
        XineramaConstrainCursor(pDev);
#endif
}

static void
CheckVirtualMotion(DeviceIntPtr pDev, QdEventPtr qe, WindowPtr pWin)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;
    RegionPtr reg = NULL;
    DeviceEvent *ev = NULL;

    if (qe) {
        ev = &qe->event->device_event;
        switch (ev->type) {
        case ET_Motion:
        case ET_ButtonPress:
        case ET_ButtonRelease:
        case ET_KeyPress:
        case ET_KeyRelease:
        case ET_ProximityIn:
        case ET_ProximityOut:
            pSprite->hot.pScreen = qe->pScreen;
            pSprite->hot.x = ev->root_x;
            pSprite->hot.y = ev->root_y;
            pWin =
                pDev->deviceGrab.grab ? pDev->deviceGrab.grab->
                confineTo : NullWindow;
            break;
        default:
            break;
        }
    }
    if (pWin) {
        BoxRec lims;

#ifdef PANORAMIX
        if (!noPanoramiXExtension) {
            int x, y, off_x, off_y, i;

            if (!XineramaSetWindowPntrs(pDev, pWin))
                return;

            i = PanoramiXNumScreens - 1;

            RegionCopy(&pSprite->Reg2, &pSprite->windows[i]->borderSize);
            off_x = screenInfo.screens[i]->x;
            off_y = screenInfo.screens[i]->y;

            while (i--) {
                x = off_x - screenInfo.screens[i]->x;
                y = off_y - screenInfo.screens[i]->y;

                if (x || y)
                    RegionTranslate(&pSprite->Reg2, x, y);

                RegionUnion(&pSprite->Reg2, &pSprite->Reg2,
                            &pSprite->windows[i]->borderSize);

                off_x = screenInfo.screens[i]->x;
                off_y = screenInfo.screens[i]->y;
            }
        }
        else
#endif
        {
            if (pSprite->hot.pScreen != pWin->drawable.pScreen) {
                pSprite->hot.pScreen = pWin->drawable.pScreen;
                pSprite->hot.x = pSprite->hot.y = 0;
            }
        }

        lims = *RegionExtents(&pWin->borderSize);
        if (pSprite->hot.x < lims.x1)
            pSprite->hot.x = lims.x1;
        else if (pSprite->hot.x >= lims.x2)
            pSprite->hot.x = lims.x2 - 1;
        if (pSprite->hot.y < lims.y1)
            pSprite->hot.y = lims.y1;
        else if (pSprite->hot.y >= lims.y2)
            pSprite->hot.y = lims.y2 - 1;

#ifdef PANORAMIX
        if (!noPanoramiXExtension) {
            if (RegionNumRects(&pSprite->Reg2) > 1)
                reg = &pSprite->Reg2;

        }
        else
#endif
        {
            if (wBoundingShape(pWin))
                reg = &pWin->borderSize;
        }

        if (reg)
            ConfineToShape(pDev, reg, &pSprite->hot.x, &pSprite->hot.y);

        if (qe && ev) {
            qe->pScreen = pSprite->hot.pScreen;
            ev->root_x = pSprite->hot.x;
            ev->root_y = pSprite->hot.y;
        }
    }
#ifdef PANORAMIX
    if (noPanoramiXExtension)   /* No typo. Only set the root win if disabled */
#endif
        RootWindow(pDev->spriteInfo->sprite) = pSprite->hot.pScreen->root;
}

static void
ConfineCursorToWindow(DeviceIntPtr pDev, WindowPtr pWin, Bool generateEvents,
                      Bool confineToScreen)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    if (syncEvents.playingEvents) {
        CheckVirtualMotion(pDev, (QdEventPtr) NULL, pWin);
        SyntheticMotion(pDev, pSprite->hot.x, pSprite->hot.y);
    }
    else {
        ScreenPtr pScreen = pWin->drawable.pScreen;

#ifdef PANORAMIX
        if (!noPanoramiXExtension) {
            XineramaConfineCursorToWindow(pDev, pWin, generateEvents);
            return;
        }
#endif
        pSprite->hotLimits = *RegionExtents(&pWin->borderSize);
        pSprite->hotShape = wBoundingShape(pWin) ? &pWin->borderSize
            : NullRegion;
        CheckPhysLimits(pDev, pSprite->current, generateEvents,
                        confineToScreen, pWin->drawable.pScreen);

        if (*pScreen->CursorConfinedTo)
            (*pScreen->CursorConfinedTo) (pDev, pScreen, pWin);
    }
}

Bool
PointerConfinedToScreen(DeviceIntPtr pDev)
{
    return pDev->spriteInfo->sprite->confined;
}

/**
 * Update the sprite cursor to the given cursor.
 *
 * ChangeToCursor() will display the new cursor and free the old cursor (if
 * applicable). If the provided cursor is already the updated cursor, nothing
 * happens.
 */
static void
ChangeToCursor(DeviceIntPtr pDev, CursorPtr cursor)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;
    ScreenPtr pScreen;

    if (cursor != pSprite->current) {
        if ((pSprite->current->bits->xhot != cursor->bits->xhot) ||
            (pSprite->current->bits->yhot != cursor->bits->yhot))
            CheckPhysLimits(pDev, cursor, FALSE, pSprite->confined,
                            (ScreenPtr) NULL);
#ifdef PANORAMIX
        /* XXX: is this really necessary?? (whot) */
        if (!noPanoramiXExtension)
            pScreen = pSprite->screen;
        else
#endif
            pScreen = pSprite->hotPhys.pScreen;

        (*pScreen->DisplayCursor) (pDev, pScreen, cursor);
        FreeCursor(pSprite->current, (Cursor) 0);
        pSprite->current = RefCursor(cursor);
    }
}

/**
 * @returns true if b is a descendent of a
 */
Bool
IsParent(WindowPtr a, WindowPtr b)
{
    for (b = b->parent; b; b = b->parent)
        if (b == a)
            return TRUE;
    return FALSE;
}

/**
 * Update the cursor displayed on the screen.
 *
 * Called whenever a cursor may have changed shape or position.
 */
static void
PostNewCursor(DeviceIntPtr pDev)
{
    WindowPtr win;
    GrabPtr grab = pDev->deviceGrab.grab;
    SpritePtr pSprite = pDev->spriteInfo->sprite;
    CursorPtr pCursor;

    if (syncEvents.playingEvents)
        return;
    if (grab) {
        if (grab->cursor) {
            ChangeToCursor(pDev, grab->cursor);
            return;
        }
        if (IsParent(grab->window, pSprite->win))
            win = pSprite->win;
        else
            win = grab->window;
    }
    else
        win = pSprite->win;
    for (; win; win = win->parent) {
        if (win->optional) {
            pCursor = WindowGetDeviceCursor(win, pDev);
            if (!pCursor && win->optional->cursor != NullCursor)
                pCursor = win->optional->cursor;
            if (pCursor) {
                ChangeToCursor(pDev, pCursor);
                return;
            }
        }
    }
}

/**
 * @param dev device which you want to know its current root window
 * @return root window where dev's sprite is located
 */
WindowPtr
GetCurrentRootWindow(DeviceIntPtr dev)
{
    return RootWindow(dev->spriteInfo->sprite);
}

/**
 * @return window underneath the cursor sprite.
 */
WindowPtr
GetSpriteWindow(DeviceIntPtr pDev)
{
    return pDev->spriteInfo->sprite->win;
}

/**
 * @return current sprite cursor.
 */
CursorPtr
GetSpriteCursor(DeviceIntPtr pDev)
{
    return pDev->spriteInfo->sprite->current;
}

/**
 * Set x/y current sprite position in screen coordinates.
 */
void
GetSpritePosition(DeviceIntPtr pDev, int *px, int *py)
{
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    *px = pSprite->hotPhys.x;
    *py = pSprite->hotPhys.y;
}

#ifdef PANORAMIX
int
XineramaGetCursorScreen(DeviceIntPtr pDev)
{
    if (!noPanoramiXExtension) {
        return pDev->spriteInfo->sprite->screen->myNum;
    }
    else {
        return 0;
    }
}
#endif                          /* PANORAMIX */

#define TIMESLOP (5 * 60 * 1000)        /* 5 minutes */

static void
MonthChangedOrBadTime(CARD32 *ms)
{
    /* If the ddx/OS is careless about not processing timestamped events from
     * different sources in sorted order, then it's possible for time to go
     * backwards when it should not.  Here we ensure a decent time.
     */
    if ((currentTime.milliseconds - *ms) > TIMESLOP)
        currentTime.months++;
    else
        *ms = currentTime.milliseconds;
}

void
NoticeTime(const DeviceIntPtr dev, TimeStamp time)
{
    currentTime = time;
    lastDeviceEventTime[XIAllDevices].time = currentTime;
    lastDeviceEventTime[dev->id].time = currentTime;

    LastEventTimeToggleResetFlag(dev->id, TRUE);
    LastEventTimeToggleResetFlag(XIAllDevices, TRUE);
}

static void
NoticeTimeMillis(const DeviceIntPtr dev, CARD32 *ms)
{
    TimeStamp time;
    if (*ms < currentTime.milliseconds)
        MonthChangedOrBadTime(ms);
    time.months = currentTime.months;
    time.milliseconds = *ms;
    NoticeTime(dev, time);
}

void
NoticeEventTime(InternalEvent *ev, DeviceIntPtr dev)
{
    if (!syncEvents.playingEvents)
        NoticeTimeMillis(dev, &ev->any.time);
}

TimeStamp
LastEventTime(int deviceid)
{
    return lastDeviceEventTime[deviceid].time;
}

Bool
LastEventTimeWasReset(int deviceid)
{
    return lastDeviceEventTime[deviceid].reset;
}

void
LastEventTimeToggleResetFlag(int deviceid, Bool state)
{
    lastDeviceEventTime[deviceid].reset = state;
}

void
LastEventTimeToggleResetAll(Bool state)
{
    DeviceIntPtr dev;
    nt_list_for_each_entry(dev, inputInfo.devices, next) {
        LastEventTimeToggleResetFlag(dev->id, FALSE);
    }
    LastEventTimeToggleResetFlag(XIAllDevices, FALSE);
    LastEventTimeToggleResetFlag(XIAllMasterDevices, FALSE);
}

/**************************************************************************
 *            The following procedures deal with synchronous events       *
 **************************************************************************/

/**
 * EnqueueEvent is a device's processInputProc if a device is frozen.
 * Instead of delivering the events to the client, the event is tacked onto a
 * linked list for later delivery.
 */
void
EnqueueEvent(InternalEvent *ev, DeviceIntPtr device)
{
    QdEventPtr tail = NULL;
    QdEventPtr qe;
    SpritePtr pSprite = device->spriteInfo->sprite;
    int eventlen;
    DeviceEvent *event = &ev->device_event;

    if (!xorg_list_is_empty(&syncEvents.pending))
        tail = xorg_list_last_entry(&syncEvents.pending, QdEventRec, next);

    NoticeTimeMillis(device, &ev->any.time);

    /* Fix for key repeating bug. */
    if (device->key != NULL && device->key->xkbInfo != NULL &&
        event->type == ET_KeyRelease)
        AccessXCancelRepeatKey(device->key->xkbInfo, event->detail.key);

    if (DeviceEventCallback) {
        DeviceEventInfoRec eventinfo;

        /*  The RECORD spec says that the root window field of motion events
         *  must be valid.  At this point, it hasn't been filled in yet, so
         *  we do it here.  The long expression below is necessary to get
         *  the current root window; the apparently reasonable alternative
         *  GetCurrentRootWindow()->drawable.id doesn't give you the right
         *  answer on the first motion event after a screen change because
         *  the data that GetCurrentRootWindow relies on hasn't been
         *  updated yet.
         */
        if (ev->any.type == ET_Motion)
            ev->device_event.root = pSprite->hotPhys.pScreen->root->drawable.id;

        eventinfo.event = ev;
        eventinfo.device = device;
        CallCallbacks(&DeviceEventCallback, (void *) &eventinfo);
    }

    if (event->type == ET_Motion) {
#ifdef PANORAMIX
        if (!noPanoramiXExtension) {
            event->root_x += pSprite->screen->x - screenInfo.screens[0]->x;
            event->root_y += pSprite->screen->y - screenInfo.screens[0]->y;
        }
#endif
        pSprite->hotPhys.x = event->root_x;
        pSprite->hotPhys.y = event->root_y;
        /* do motion compression, but not if from different devices */
        if (tail &&
            (tail->event->any.type == ET_Motion) &&
            (tail->device == device) &&
            (tail->pScreen == pSprite->hotPhys.pScreen)) {
            DeviceEvent *tailev = &tail->event->device_event;

            tailev->root_x = pSprite->hotPhys.x;
            tailev->root_y = pSprite->hotPhys.y;
            tailev->time = event->time;
            tail->months = currentTime.months;
            return;
        }
    }

    eventlen = sizeof(InternalEvent);

    qe = malloc(sizeof(QdEventRec) + eventlen);
    if (!qe)
        return;
    xorg_list_init(&qe->next);
    qe->device = device;
    qe->pScreen = pSprite->hotPhys.pScreen;
    qe->months = currentTime.months;
    qe->event = (InternalEvent *) (qe + 1);
    CopyPartialInternalEvent(qe->event, (InternalEvent *)event);
    xorg_list_append(&qe->next, &syncEvents.pending);
}

/**
 * Run through the list of events queued up in syncEvents.
 * For each event do:
 * If the device for this event is not frozen anymore, take it and process it
 * as usually.
 * After that, check if there's any devices in the list that are not frozen.
 * If there is none, we're done. If there is at least one device that is not
 * frozen, then re-run from the beginning of the event queue.
 */
void
PlayReleasedEvents(void)
{
    QdEventPtr tmp;
    QdEventPtr qe;
    DeviceIntPtr dev;
    DeviceIntPtr pDev;

 restart:
    xorg_list_for_each_entry_safe(qe, tmp, &syncEvents.pending, next) {
        if (!qe->device->deviceGrab.sync.frozen) {
            xorg_list_del(&qe->next);
            pDev = qe->device;
            if (qe->event->any.type == ET_Motion)
                CheckVirtualMotion(pDev, qe, NullWindow);
            syncEvents.time.months = qe->months;
            syncEvents.time.milliseconds = qe->event->any.time;
#ifdef PANORAMIX
            /* Translate back to the sprite screen since processInputProc
               will translate from sprite screen to screen 0 upon reentry
               to the DIX layer */
            if (!noPanoramiXExtension) {
                DeviceEvent *ev = &qe->event->device_event;

                switch (ev->type) {
                case ET_Motion:
                case ET_ButtonPress:
                case ET_ButtonRelease:
                case ET_KeyPress:
                case ET_KeyRelease:
                case ET_ProximityIn:
                case ET_ProximityOut:
                case ET_TouchBegin:
                case ET_TouchUpdate:
                case ET_TouchEnd:
                    ev->root_x += screenInfo.screens[0]->x -
                        pDev->spriteInfo->sprite->screen->x;
                    ev->root_y += screenInfo.screens[0]->y -
                        pDev->spriteInfo->sprite->screen->y;
                    break;
                default:
                    break;
                }

            }
#endif
            (*qe->device->public.processInputProc) (qe->event, qe->device);
            free(qe);
            for (dev = inputInfo.devices; dev && dev->deviceGrab.sync.frozen;
                 dev = dev->next);
            if (!dev)
                break;

            /* Playing the event may have unfrozen another device. */
            /* So to play it safe, restart at the head of the queue */
            goto restart;
        }
    }
}

/**
 * Freeze or thaw the given devices. The device's processing proc is
 * switched to either the real processing proc (in case of thawing) or an
 * enqueuing processing proc (usually EnqueueEvent()).
 *
 * @param dev The device to freeze/thaw
 * @param frozen True to freeze or false to thaw.
 */
static void
FreezeThaw(DeviceIntPtr dev, Bool frozen)
{
    dev->deviceGrab.sync.frozen = frozen;
    if (frozen)
        dev->public.processInputProc = dev->public.enqueueInputProc;
    else
        dev->public.processInputProc = dev->public.realInputProc;
}

/**
 * Unfreeze devices and replay all events to the respective clients.
 *
 * ComputeFreezes takes the first event in the device's frozen event queue. It
 * runs up the sprite tree (spriteTrace) and searches for the window to replay
 * the events from. If it is found, it checks for passive grabs one down from
 * the window or delivers the events.
 */
static void
ComputeFreezes(void)
{
    DeviceIntPtr replayDev = syncEvents.replayDev;
    GrabPtr grab;
    DeviceIntPtr dev;

    for (dev = inputInfo.devices; dev; dev = dev->next)
        FreezeThaw(dev, dev->deviceGrab.sync.other ||
                   (dev->deviceGrab.sync.state >= FROZEN));
    if (syncEvents.playingEvents ||
        (!replayDev && xorg_list_is_empty(&syncEvents.pending)))
        return;
    syncEvents.playingEvents = TRUE;
    if (replayDev) {
        InternalEvent *event = replayDev->deviceGrab.sync.event;

        syncEvents.replayDev = (DeviceIntPtr) NULL;

        if (!CheckDeviceGrabs(replayDev, event,
                              syncEvents.replayWin)) {
            if (IsTouchEvent(event)) {
                TouchPointInfoPtr ti =
                    TouchFindByClientID(replayDev, event->device_event.touchid);
                BUG_WARN(!ti);

                TouchListenerAcceptReject(replayDev, ti, 0, XIRejectTouch);
            }
            else if (IsGestureEvent(event)) {
                GestureInfoPtr gi =
                    GestureFindActiveByEventType(replayDev, event->any.type);
                if (gi) {
                    GestureEmitGestureEndToOwner(replayDev, gi);
                    GestureEndGesture(gi);
                }
                ProcessGestureEvent(event, replayDev);
            }
            else {
                WindowPtr w = XYToWindow(replayDev->spriteInfo->sprite,
                                         event->device_event.root_x,
                                         event->device_event.root_y);
                if (replayDev->focus && !IsPointerEvent(event))
                    DeliverFocusedEvent(replayDev, event, w);
                else
                    DeliverDeviceEvents(w, event, NullGrab,
                                        NullWindow, replayDev);
            }
        }
    }
    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (!dev->deviceGrab.sync.frozen) {
            PlayReleasedEvents();
            break;
        }
    }
    syncEvents.playingEvents = FALSE;
    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (DevHasCursor(dev)) {
            /* the following may have been skipped during replay,
               so do it now */
            if ((grab = dev->deviceGrab.grab) && grab->confineTo) {
                if (grab->confineTo->drawable.pScreen !=
                    dev->spriteInfo->sprite->hotPhys.pScreen)
                    dev->spriteInfo->sprite->hotPhys.x =
                        dev->spriteInfo->sprite->hotPhys.y = 0;
                ConfineCursorToWindow(dev, grab->confineTo, TRUE, TRUE);
            }
            else
                ConfineCursorToWindow(dev,
                                      dev->spriteInfo->sprite->hotPhys.pScreen->
                                      root, TRUE, FALSE);
            PostNewCursor(dev);
        }
    }
}

#ifdef RANDR
void
ScreenRestructured(ScreenPtr pScreen)
{
    GrabPtr grab;
    DeviceIntPtr pDev;

    for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
        if (!IsFloating(pDev) && !DevHasCursor(pDev))
            continue;

        /* GrabDevice doesn't have a confineTo field, so we don't need to
         * worry about it. */
        if ((grab = pDev->deviceGrab.grab) && grab->confineTo) {
            if (grab->confineTo->drawable.pScreen
                != pDev->spriteInfo->sprite->hotPhys.pScreen)
                pDev->spriteInfo->sprite->hotPhys.x =
                    pDev->spriteInfo->sprite->hotPhys.y = 0;
            ConfineCursorToWindow(pDev, grab->confineTo, TRUE, TRUE);
        }
        else
            ConfineCursorToWindow(pDev,
                                  pDev->spriteInfo->sprite->hotPhys.pScreen->
                                  root, TRUE, FALSE);
    }
}
#endif

static void
CheckGrabForSyncs(DeviceIntPtr thisDev, Bool thisMode, Bool otherMode)
{
    GrabPtr grab = thisDev->deviceGrab.grab;
    DeviceIntPtr dev;

    if (thisMode == GrabModeSync)
        thisDev->deviceGrab.sync.state = FROZEN_NO_EVENT;
    else {                      /* free both if same client owns both */
        thisDev->deviceGrab.sync.state = THAWED;
        if (thisDev->deviceGrab.sync.other &&
            (CLIENT_BITS(thisDev->deviceGrab.sync.other->resource) ==
             CLIENT_BITS(grab->resource)))
            thisDev->deviceGrab.sync.other = NullGrab;
    }

    if (IsMaster(thisDev)) {
        dev = GetPairedDevice(thisDev);
        if (otherMode == GrabModeSync)
            dev->deviceGrab.sync.other = grab;
        else {                  /* free both if same client owns both */
            if (dev->deviceGrab.sync.other &&
                (CLIENT_BITS(dev->deviceGrab.sync.other->resource) ==
                 CLIENT_BITS(grab->resource)))
                dev->deviceGrab.sync.other = NullGrab;
        }
    }
    ComputeFreezes();
}

/**
 * Save the device's master device id. This needs to be done
 * if a client directly grabs a slave device that is attached to a master. For
 * the duration of the grab, the device is detached, ungrabbing re-attaches it
 * though.
 *
 * We store the ID of the master device only in case the master disappears
 * while the device has a grab.
 */
static void
DetachFromMaster(DeviceIntPtr dev)
{
    if (IsFloating(dev))
        return;

    dev->saved_master_id = GetMaster(dev, MASTER_ATTACHED)->id;

    AttachDevice(NULL, dev, NULL);
}

static void
ReattachToOldMaster(DeviceIntPtr dev)
{
    DeviceIntPtr master = NULL;

    if (IsMaster(dev))
        return;

    dixLookupDevice(&master, dev->saved_master_id, serverClient, DixUseAccess);

    if (master) {
        AttachDevice(serverClient, dev, master);
        dev->saved_master_id = 0;
    }
}

/**
 * Update touch records when an explicit grab is activated. Any touches owned by
 * the grabbing client are updated so the listener state reflects the new grab.
 */
static void
UpdateTouchesForGrab(DeviceIntPtr mouse)
{
    int i;

    if (!mouse->touch || mouse->deviceGrab.fromPassiveGrab)
        return;

    for (i = 0; i < mouse->touch->num_touches; i++) {
        TouchPointInfoPtr ti = mouse->touch->touches + i;
        TouchListener *listener = &ti->listeners[0];
        GrabPtr grab = mouse->deviceGrab.grab;

        if (ti->active &&
            CLIENT_BITS(listener->listener) == grab->resource) {
            if (grab->grabtype == CORE || grab->grabtype == XI ||
                !xi2mask_isset(grab->xi2mask, mouse, XI_TouchBegin)) {
                /*  Note that the grab will override any current listeners and if these listeners
                    already received touch events then this is the place to send touch end event
                    to complete the touch sequence.

                    Unfortunately GTK3 menu widget implementation relies on not getting touch end
                    event, so we can't fix the current behavior.
                */
                listener->type = TOUCH_LISTENER_POINTER_GRAB;
            } else {
                listener->type = TOUCH_LISTENER_GRAB;
            }

            listener->listener = grab->resource;
            listener->level = grab->grabtype;
            listener->window = grab->window;
            listener->state = TOUCH_LISTENER_IS_OWNER;

            if (listener->grab)
                FreeGrab(listener->grab);
            listener->grab = AllocGrab(grab);
        }
    }
}

/**
 * Update gesture records when an explicit grab is activated. Any gestures owned
 * by the grabbing client are updated so the listener state reflects the new
 * grab.
 */
static void
UpdateGesturesForGrab(DeviceIntPtr mouse)
{
    if (!mouse->gesture || mouse->deviceGrab.fromPassiveGrab)
        return;

    GestureInfoPtr gi = &mouse->gesture->gesture;
    GestureListener *listener = &gi->listener;
    GrabPtr grab = mouse->deviceGrab.grab;

    if (gi->active && CLIENT_BITS(listener->listener) == grab->resource) {
        if (grab->grabtype == CORE || grab->grabtype == XI ||
            !xi2mask_isset(grab->xi2mask, mouse, GetXI2Type(gi->type))) {

            if (listener->type == GESTURE_LISTENER_REGULAR) {
                /* if the listener already got any events relating to the gesture, we must send
                   a gesture end because the grab overrides the previous listener and won't
                   itself send any gesture events.
                */
                GestureEmitGestureEndToOwner(mouse, gi);
            }
            listener->type = GESTURE_LISTENER_NONGESTURE_GRAB;
        } else {
            listener->type = GESTURE_LISTENER_GRAB;
        }

        listener->listener = grab->resource;
        listener->window = grab->window;

        if (listener->grab)
            FreeGrab(listener->grab);
        listener->grab = AllocGrab(grab);
    }
}

/**
 * Activate a pointer grab on the given device. A pointer grab will cause all
 * core pointer events of this device to be delivered to the grabbing client only.
 * No other device will send core events to the grab client while the grab is
 * on, but core events will be sent to other clients.
 * Can cause the cursor to change if a grab cursor is set.
 *
 * Note that parameter autoGrab may be (True & ImplicitGrabMask) if the grab
 * is an implicit grab caused by a ButtonPress event.
 *
 * @param mouse The device to grab.
 * @param grab The grab structure, needs to be setup.
 * @param autoGrab True if the grab was caused by a button down event and not
 * explicitly by a client.
 */
void
ActivatePointerGrab(DeviceIntPtr mouse, GrabPtr grab,
                    TimeStamp time, Bool autoGrab)
{
    GrabInfoPtr grabinfo = &mouse->deviceGrab;
    GrabPtr oldgrab = grabinfo->grab;
    WindowPtr oldWin = (grabinfo->grab) ?
        grabinfo->grab->window : mouse->spriteInfo->sprite->win;
    Bool isPassive = autoGrab & ~ImplicitGrabMask;

    /* slave devices need to float for the duration of the grab. */
    if (grab->grabtype == XI2 &&
        !(autoGrab & ImplicitGrabMask) && !IsMaster(mouse))
        DetachFromMaster(mouse);

    if (grab->confineTo) {
        if (grab->confineTo->drawable.pScreen
            != mouse->spriteInfo->sprite->hotPhys.pScreen)
            mouse->spriteInfo->sprite->hotPhys.x =
                mouse->spriteInfo->sprite->hotPhys.y = 0;
        ConfineCursorToWindow(mouse, grab->confineTo, FALSE, TRUE);
    }
    if (! (grabinfo->grab && oldWin == grabinfo->grab->window
			  && oldWin == grab->window))
        DoEnterLeaveEvents(mouse, mouse->id, oldWin, grab->window, NotifyGrab);
    mouse->valuator->motionHintWindow = NullWindow;
    if (syncEvents.playingEvents)
        grabinfo->grabTime = syncEvents.time;
    else
        grabinfo->grabTime = time;
    grabinfo->grab = AllocGrab(grab);
    grabinfo->fromPassiveGrab = isPassive;
    grabinfo->implicitGrab = autoGrab & ImplicitGrabMask;
    PostNewCursor(mouse);
    UpdateTouchesForGrab(mouse);
    UpdateGesturesForGrab(mouse);
    CheckGrabForSyncs(mouse, (Bool) grab->pointerMode,
                      (Bool) grab->keyboardMode);
    if (oldgrab)
        FreeGrab(oldgrab);
}

/**
 * Delete grab on given device, update the sprite.
 *
 * Extension devices are set up for ActivateKeyboardGrab().
 */
void
DeactivatePointerGrab(DeviceIntPtr mouse)
{
    GrabPtr grab = mouse->deviceGrab.grab;
    DeviceIntPtr dev;
    Bool wasPassive = mouse->deviceGrab.fromPassiveGrab;
    Bool wasImplicit = (mouse->deviceGrab.fromPassiveGrab &&
                        mouse->deviceGrab.implicitGrab);
    XID grab_resource = grab->resource;
    int i;

    /* If an explicit grab was deactivated, we must remove it from the head of
     * all the touches' listener lists. */
    for (i = 0; !wasPassive && mouse->touch && i < mouse->touch->num_touches; i++) {
        TouchPointInfoPtr ti = mouse->touch->touches + i;
        if (ti->active && TouchResourceIsOwner(ti, grab_resource)) {
            int mode = XIRejectTouch;
            /* Rejecting will generate a TouchEnd, but we must not
               emulate a ButtonRelease here. So pretend the listener
               already has the end event */
            if (grab->grabtype == CORE || grab->grabtype == XI ||
                    !xi2mask_isset(grab->xi2mask, mouse, XI_TouchBegin)) {
                mode = XIAcceptTouch;
                /* NOTE: we set the state here, but
                 * ProcessTouchOwnershipEvent() will still call
                 * TouchEmitTouchEnd for this listener. The other half of
                 * this hack is in DeliverTouchEndEvent */
                ti->listeners[0].state = TOUCH_LISTENER_HAS_END;
            }
            TouchListenerAcceptReject(mouse, ti, 0, mode);
        }
    }

    TouchRemovePointerGrab(mouse);

    mouse->valuator->motionHintWindow = NullWindow;
    mouse->deviceGrab.grab = NullGrab;
    mouse->deviceGrab.sync.state = NOT_GRABBED;
    mouse->deviceGrab.fromPassiveGrab = FALSE;

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (dev->deviceGrab.sync.other == grab)
            dev->deviceGrab.sync.other = NullGrab;
    }

    /* in case of explicit gesture grab, send end event to the grab client */
    if (!wasPassive && mouse->gesture) {
        GestureInfoPtr gi = &mouse->gesture->gesture;
        if (gi->active && GestureResourceIsOwner(gi, grab_resource)) {
            GestureEmitGestureEndToOwner(mouse, gi);
            GestureEndGesture(gi);
        }
    }

    DoEnterLeaveEvents(mouse, mouse->id, grab->window,
                       mouse->spriteInfo->sprite->win, NotifyUngrab);
    if (grab->confineTo)
        ConfineCursorToWindow(mouse, GetCurrentRootWindow(mouse), FALSE, FALSE);
    PostNewCursor(mouse);

    if (!wasImplicit && grab->grabtype == XI2)
        ReattachToOldMaster(mouse);

    ComputeFreezes();

    FreeGrab(grab);
}

/**
 * Activate a keyboard grab on the given device.
 *
 * Extension devices have ActivateKeyboardGrab() set as their grabbing proc.
 */
void
ActivateKeyboardGrab(DeviceIntPtr keybd, GrabPtr grab, TimeStamp time,
                     Bool passive)
{
    GrabInfoPtr grabinfo = &keybd->deviceGrab;
    GrabPtr oldgrab = grabinfo->grab;
    WindowPtr oldWin;

    /* slave devices need to float for the duration of the grab. */
    if (grab->grabtype == XI2 && keybd->enabled &&
        !(passive & ImplicitGrabMask) && !IsMaster(keybd))
        DetachFromMaster(keybd);

    if (!keybd->enabled)
        oldWin = NULL;
    else if (grabinfo->grab)
        oldWin = grabinfo->grab->window;
    else if (keybd->focus)
        oldWin = keybd->focus->win;
    else
        oldWin = keybd->spriteInfo->sprite->win;
    if (oldWin == FollowKeyboardWin)
        oldWin = keybd->focus->win;
    if (keybd->valuator)
        keybd->valuator->motionHintWindow = NullWindow;
    if (oldWin &&
	! (grabinfo->grab && oldWin == grabinfo->grab->window
			  && oldWin == grab->window))
        DoFocusEvents(keybd, oldWin, grab->window, NotifyGrab);
    if (syncEvents.playingEvents)
        grabinfo->grabTime = syncEvents.time;
    else
        grabinfo->grabTime = time;
    grabinfo->grab = AllocGrab(grab);
    grabinfo->fromPassiveGrab = passive;
    grabinfo->implicitGrab = passive & ImplicitGrabMask;
    CheckGrabForSyncs(keybd, (Bool) grab->keyboardMode,
                      (Bool) grab->pointerMode);
    if (oldgrab)
        FreeGrab(oldgrab);
}

/**
 * Delete keyboard grab for the given device.
 */
void
DeactivateKeyboardGrab(DeviceIntPtr keybd)
{
    GrabPtr grab = keybd->deviceGrab.grab;
    DeviceIntPtr dev;
    WindowPtr focusWin;
    Bool wasImplicit = (keybd->deviceGrab.fromPassiveGrab &&
                        keybd->deviceGrab.implicitGrab);

    if (keybd->valuator)
        keybd->valuator->motionHintWindow = NullWindow;
    keybd->deviceGrab.grab = NullGrab;
    keybd->deviceGrab.sync.state = NOT_GRABBED;
    keybd->deviceGrab.fromPassiveGrab = FALSE;

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (dev->deviceGrab.sync.other == grab)
            dev->deviceGrab.sync.other = NullGrab;
    }

    if (keybd->focus)
        focusWin = keybd->focus->win;
    else if (keybd->spriteInfo->sprite)
        focusWin = keybd->spriteInfo->sprite->win;
    else
        focusWin = NullWindow;

    if (focusWin == FollowKeyboardWin)
        focusWin = inputInfo.keyboard->focus->win;

    DoFocusEvents(keybd, grab->window, focusWin, NotifyUngrab);

    if (!wasImplicit && grab->grabtype == XI2)
        ReattachToOldMaster(keybd);

    ComputeFreezes();

    FreeGrab(grab);
}

void
AllowSome(ClientPtr client, TimeStamp time, DeviceIntPtr thisDev, int newState)
{
    Bool thisGrabbed, otherGrabbed, othersFrozen, thisSynced;
    TimeStamp grabTime;
    DeviceIntPtr dev;
    GrabInfoPtr devgrabinfo, grabinfo = &thisDev->deviceGrab;

    thisGrabbed = grabinfo->grab && SameClient(grabinfo->grab, client);
    thisSynced = FALSE;
    otherGrabbed = FALSE;
    othersFrozen = FALSE;
    grabTime = grabinfo->grabTime;
    for (dev = inputInfo.devices; dev; dev = dev->next) {
        devgrabinfo = &dev->deviceGrab;

        if (dev == thisDev)
            continue;
        if (devgrabinfo->grab && SameClient(devgrabinfo->grab, client)) {
            if (!(thisGrabbed || otherGrabbed) ||
                (CompareTimeStamps(devgrabinfo->grabTime, grabTime) == LATER))
                grabTime = devgrabinfo->grabTime;
            otherGrabbed = TRUE;
            if (grabinfo->sync.other == devgrabinfo->grab)
                thisSynced = TRUE;
            if (devgrabinfo->sync.state >= FROZEN)
                othersFrozen = TRUE;
        }
    }
    if (!((thisGrabbed && grabinfo->sync.state >= FROZEN) || thisSynced))
        return;
    if ((CompareTimeStamps(time, currentTime) == LATER) ||
        (CompareTimeStamps(time, grabTime) == EARLIER))
        return;
    switch (newState) {
    case THAWED:               /* Async */
        if (thisGrabbed)
            grabinfo->sync.state = THAWED;
        if (thisSynced)
            grabinfo->sync.other = NullGrab;
        ComputeFreezes();
        break;
    case FREEZE_NEXT_EVENT:    /* Sync */
        if (thisGrabbed) {
            grabinfo->sync.state = FREEZE_NEXT_EVENT;
            if (thisSynced)
                grabinfo->sync.other = NullGrab;
            ComputeFreezes();
        }
        break;
    case THAWED_BOTH:          /* AsyncBoth */
        if (othersFrozen) {
            for (dev = inputInfo.devices; dev; dev = dev->next) {
                devgrabinfo = &dev->deviceGrab;
                if (devgrabinfo->grab && SameClient(devgrabinfo->grab, client))
                    devgrabinfo->sync.state = THAWED;
                if (devgrabinfo->sync.other &&
                    SameClient(devgrabinfo->sync.other, client))
                    devgrabinfo->sync.other = NullGrab;
            }
            ComputeFreezes();
        }
        break;
    case FREEZE_BOTH_NEXT_EVENT:       /* SyncBoth */
        if (othersFrozen) {
            for (dev = inputInfo.devices; dev; dev = dev->next) {
                devgrabinfo = &dev->deviceGrab;
                if (devgrabinfo->grab && SameClient(devgrabinfo->grab, client))
                    devgrabinfo->sync.state = FREEZE_BOTH_NEXT_EVENT;
                if (devgrabinfo->sync.other
                    && SameClient(devgrabinfo->sync.other, client))
                    devgrabinfo->sync.other = NullGrab;
            }
            ComputeFreezes();
        }
        break;
    case NOT_GRABBED:          /* Replay */
        if (thisGrabbed && grabinfo->sync.state == FROZEN_WITH_EVENT) {
            if (thisSynced)
                grabinfo->sync.other = NullGrab;
            syncEvents.replayDev = thisDev;
            syncEvents.replayWin = grabinfo->grab->window;
            (*grabinfo->DeactivateGrab) (thisDev);
            syncEvents.replayDev = (DeviceIntPtr) NULL;
        }
        break;
    case THAW_OTHERS:          /* AsyncOthers */
        if (othersFrozen) {
            for (dev = inputInfo.devices; dev; dev = dev->next) {
                if (dev == thisDev)
                    continue;
                devgrabinfo = &dev->deviceGrab;
                if (devgrabinfo->grab && SameClient(devgrabinfo->grab, client))
                    devgrabinfo->sync.state = THAWED;
                if (devgrabinfo->sync.other
                    && SameClient(devgrabinfo->sync.other, client))
                    devgrabinfo->sync.other = NullGrab;
            }
            ComputeFreezes();
        }
        break;
    }

    /* We've unfrozen the grab. If the grab was a touch grab, we're now the
     * owner and expected to accept/reject it. Reject == ReplayPointer which
     * we've handled in ComputeFreezes() (during DeactivateGrab) above,
     * anything else is accept.
     */
    if (newState != NOT_GRABBED /* Replay */ &&
        IsTouchEvent(grabinfo->sync.event)) {
        TouchAcceptAndEnd(thisDev, grabinfo->sync.event->device_event.touchid);
    }
}

/**
 * Server-side protocol handling for AllowEvents request.
 *
 * Release some events from a frozen device.
 */
int
ProcAllowEvents(ClientPtr client)
{
    TimeStamp time;
    DeviceIntPtr mouse = NULL;
    DeviceIntPtr keybd = NULL;

    REQUEST(xAllowEventsReq);

    REQUEST_SIZE_MATCH(xAllowEventsReq);
    UpdateCurrentTime();
    time = ClientTimeToServerTime(stuff->time);

    mouse = PickPointer(client);
    keybd = PickKeyboard(client);

    switch (stuff->mode) {
    case ReplayPointer:
        AllowSome(client, time, mouse, NOT_GRABBED);
        break;
    case SyncPointer:
        AllowSome(client, time, mouse, FREEZE_NEXT_EVENT);
        break;
    case AsyncPointer:
        AllowSome(client, time, mouse, THAWED);
        break;
    case ReplayKeyboard:
        AllowSome(client, time, keybd, NOT_GRABBED);
        break;
    case SyncKeyboard:
        AllowSome(client, time, keybd, FREEZE_NEXT_EVENT);
        break;
    case AsyncKeyboard:
        AllowSome(client, time, keybd, THAWED);
        break;
    case SyncBoth:
        AllowSome(client, time, keybd, FREEZE_BOTH_NEXT_EVENT);
        break;
    case AsyncBoth:
        AllowSome(client, time, keybd, THAWED_BOTH);
        break;
    default:
        client->errorValue = stuff->mode;
        return BadValue;
    }
    return Success;
}

/**
 * Deactivate grabs from any device that has been grabbed by the client.
 */
void
ReleaseActiveGrabs(ClientPtr client)
{
    DeviceIntPtr dev;
    Bool done;

    /* XXX CloseDownClient should remove passive grabs before
     * releasing active grabs.
     */
    do {
        done = TRUE;
        for (dev = inputInfo.devices; dev; dev = dev->next) {
            if (dev->deviceGrab.grab &&
                SameClient(dev->deviceGrab.grab, client)) {
                (*dev->deviceGrab.DeactivateGrab) (dev);
                done = FALSE;
            }
        }
    } while (!done);
}

/**************************************************************************
 *            The following procedures deal with delivering events        *
 **************************************************************************/

/**
 * Deliver the given events to the given client.
 *
 * More than one event may be delivered at a time. This is the case with
 * DeviceMotionNotifies which may be followed by DeviceValuator events.
 *
 * TryClientEvents() is the last station before actually writing the events to
 * the socket. Anything that is not filtered here, will get delivered to the
 * client.
 * An event is only delivered if
 *   - mask and filter match up.
 *   - no other client has a grab on the device that caused the event.
 *
 *
 * @param client The target client to deliver to.
 * @param dev The device the event came from. May be NULL.
 * @param pEvents The events to be delivered.
 * @param count Number of elements in pEvents.
 * @param mask Event mask as set by the window.
 * @param filter Mask based on event type.
 * @param grab Possible grab on the device that caused the event.
 *
 * @return 1 if event was delivered, 0 if not or -1 if grab was not set by the
 * client.
 */
int
TryClientEvents(ClientPtr client, DeviceIntPtr dev, xEvent *pEvents,
                int count, Mask mask, Mask filter, GrabPtr grab)
{
    int type;

#ifdef DEBUG_EVENTS
    ErrorF("[dix] Event([%d, %d], mask=0x%lx), client=%d%s",
           pEvents->u.u.type, pEvents->u.u.detail, mask,
           client ? client->index : -1,
           (client && client->clientGone) ? " (gone)" : "");
#endif

    if (!client || client == serverClient || client->clientGone) {
#ifdef DEBUG_EVENTS
        ErrorF(" not delivered to fake/dead client\n");
#endif
        return 0;
    }

    if (filter != CantBeFiltered && !(mask & filter)) {
#ifdef DEBUG_EVENTS
        ErrorF(" filtered\n");
#endif
        return 0;
    }

    if (grab && !SameClient(grab, client)) {
#ifdef DEBUG_EVENTS
        ErrorF(" not delivered due to grab\n");
#endif
        return -1;              /* don't send, but notify caller */
    }

    type = pEvents->u.u.type;
    if (type == MotionNotify) {
        if (mask & PointerMotionHintMask) {
            if (WID(dev->valuator->motionHintWindow) ==
                pEvents->u.keyButtonPointer.event) {
#ifdef DEBUG_EVENTS
                ErrorF("[dix] \n");
                ErrorF("[dix] motionHintWindow == keyButtonPointer.event\n");
#endif
                return 1;       /* don't send, but pretend we did */
            }
            pEvents->u.u.detail = NotifyHint;
        }
        else {
            pEvents->u.u.detail = NotifyNormal;
        }
    }
    else if (type == DeviceMotionNotify) {
        if (MaybeSendDeviceMotionNotifyHint((deviceKeyButtonPointer *) pEvents,
                                            mask) != 0)
            return 1;
    }
    else if (type == KeyPress) {
        if (EventIsKeyRepeat(pEvents)) {
            if (!_XkbWantsDetectableAutoRepeat(client)) {
                xEvent release = *pEvents;

                release.u.u.type = KeyRelease;
                WriteEventsToClient(client, 1, &release);
#ifdef DEBUG_EVENTS
                ErrorF(" (plus fake core release for repeat)");
#endif
            }
            else {
#ifdef DEBUG_EVENTS
                ErrorF(" (detectable autorepeat for core)");
#endif
            }
        }

    }
    else if (type == DeviceKeyPress) {
        if (EventIsKeyRepeat(pEvents)) {
            if (!_XkbWantsDetectableAutoRepeat(client)) {
                deviceKeyButtonPointer release =
                    *(deviceKeyButtonPointer *) pEvents;
                release.type = DeviceKeyRelease;
#ifdef DEBUG_EVENTS
                ErrorF(" (plus fake xi1 release for repeat)");
#endif
                WriteEventsToClient(client, 1, (xEvent *) &release);
            }
            else {
#ifdef DEBUG_EVENTS
                ErrorF(" (detectable autorepeat for core)");
#endif
            }
        }
    }

    if (BitIsOn(criticalEvents, type)) {
        if (client->smart_priority < SMART_MAX_PRIORITY)
            client->smart_priority++;
        SetCriticalOutputPending();
    }

    WriteEventsToClient(client, count, pEvents);
#ifdef DEBUG_EVENTS
    ErrorF("[dix]  delivered\n");
#endif
    return 1;
}

static BOOL
ActivateImplicitGrab(DeviceIntPtr dev, ClientPtr client, WindowPtr win,
                     xEvent *event, Mask deliveryMask)
{
    GrabPtr tempGrab;
    OtherInputMasks *inputMasks;
    CARD8 type = event->u.u.type;
    enum InputLevel grabtype;

    if (type == ButtonPress)
        grabtype = CORE;
    else if (type == DeviceButtonPress)
        grabtype = XI;
    else if ((type = xi2_get_type(event)) == XI_ButtonPress)
        grabtype = XI2;
    else
        return FALSE;

    tempGrab = AllocGrab(NULL);
    if (!tempGrab)
        return FALSE;
    tempGrab->next = NULL;
    tempGrab->device = dev;
    tempGrab->resource = client->clientAsMask;
    tempGrab->window = win;
    tempGrab->ownerEvents = (deliveryMask & OwnerGrabButtonMask) ? TRUE : FALSE;
    tempGrab->eventMask = deliveryMask;
    tempGrab->keyboardMode = GrabModeAsync;
    tempGrab->pointerMode = GrabModeAsync;
    tempGrab->confineTo = NullWindow;
    tempGrab->cursor = NullCursor;
    tempGrab->type = type;
    tempGrab->grabtype = grabtype;

    /* get the XI and XI2 device mask */
    inputMasks = wOtherInputMasks(win);
    tempGrab->deviceMask = (inputMasks) ? inputMasks->inputEvents[dev->id] : 0;

    if (inputMasks)
        xi2mask_merge(tempGrab->xi2mask, inputMasks->xi2mask);

    (*dev->deviceGrab.ActivateGrab) (dev, tempGrab,
                                     currentTime, TRUE | ImplicitGrabMask);
    FreeGrab(tempGrab);
    return TRUE;
}

/**
 * Attempt event delivery to the client owning the window.
 */
static enum EventDeliveryState
DeliverToWindowOwner(DeviceIntPtr dev, WindowPtr win,
                     xEvent *events, int count, Mask filter, GrabPtr grab)
{
    /* if nobody ever wants to see this event, skip some work */
    if (filter != CantBeFiltered &&
        !((wOtherEventMasks(win) | win->eventMask) & filter))
        return EVENT_SKIP;

    if (IsInterferingGrab(wClient(win), dev, events))
        return EVENT_SKIP;

    if (!XaceHook(XACE_RECEIVE_ACCESS, wClient(win), win, events, count)) {
        int attempt = TryClientEvents(wClient(win), dev, events,
                                      count, win->eventMask,
                                      filter, grab);

        if (attempt > 0)
            return EVENT_DELIVERED;
        if (attempt < 0)
            return EVENT_REJECTED;
    }

    return EVENT_NOT_DELIVERED;
}

/**
 * Get the list of clients that should be tried for event delivery on the
 * given window.
 *
 * @return 1 if the client list should be traversed, zero if the event
 * should be skipped.
 */
static Bool
GetClientsForDelivery(DeviceIntPtr dev, WindowPtr win,
                      xEvent *events, Mask filter, InputClients ** iclients)
{
    int rc = 0;

    if (core_get_type(events) != 0)
        *iclients = (InputClients *) wOtherClients(win);
    else if (xi2_get_type(events) != 0) {
        OtherInputMasks *inputMasks = wOtherInputMasks(win);

        /* Has any client selected for the event? */
        if (!WindowXI2MaskIsset(dev, win, events))
            goto out;
        *iclients = inputMasks->inputClients;
    }
    else {
        OtherInputMasks *inputMasks = wOtherInputMasks(win);

        /* Has any client selected for the event? */
        if (!inputMasks || !(inputMasks->inputEvents[dev->id] & filter))
            goto out;

        *iclients = inputMasks->inputClients;
    }

    rc = 1;
 out:
    return rc;
}

/**
 * Try delivery on each client in inputclients, provided the event mask
 * accepts it and there is no interfering core grab..
 */
static enum EventDeliveryState
DeliverEventToInputClients(DeviceIntPtr dev, InputClients * inputclients,
                           WindowPtr win, xEvent *events,
                           int count, Mask filter, GrabPtr grab,
                           ClientPtr *client_return, Mask *mask_return)
{
    int attempt;
    enum EventDeliveryState rc = EVENT_NOT_DELIVERED;
    Bool have_device_button_grab_class_client = FALSE;

    for (; inputclients; inputclients = inputclients->next) {
        Mask mask;
        ClientPtr client = rClient(inputclients);

        if (IsInterferingGrab(client, dev, events))
            continue;

        if (IsWrongPointerBarrierClient(client, dev, events))
            continue;

        mask = GetEventMask(dev, events, inputclients);

        if (XaceHook(XACE_RECEIVE_ACCESS, client, win, events, count))
            /* do nothing */ ;
        else if ((attempt = TryClientEvents(client, dev,
                                            events, count,
                                            mask, filter, grab))) {
            if (attempt > 0) {
                /*
                 * The order of clients is arbitrary therefore if one
                 * client belongs to DeviceButtonGrabClass make sure to
                 * catch it.
                 */
                if (!have_device_button_grab_class_client) {
                    rc = EVENT_DELIVERED;
                    *client_return = client;
                    *mask_return = mask;
                    /* Success overrides non-success, so if we've been
                     * successful on one client, return that */
                    if (mask & DeviceButtonGrabMask)
                        have_device_button_grab_class_client = TRUE;
                }
            } else if (rc == EVENT_NOT_DELIVERED)
                rc = EVENT_REJECTED;
        }
    }

    return rc;
}

/**
 * Deliver events to clients registered on the window.
 *
 * @param client_return On successful delivery, set to the recipient.
 * @param mask_return On successful delivery, set to the recipient's event
 * mask for this event.
 */
static enum EventDeliveryState
DeliverEventToWindowMask(DeviceIntPtr dev, WindowPtr win, xEvent *events,
                         int count, Mask filter, GrabPtr grab,
                         ClientPtr *client_return, Mask *mask_return)
{
    InputClients *iclients;

    if (!GetClientsForDelivery(dev, win, events, filter, &iclients))
        return EVENT_SKIP;

    return DeliverEventToInputClients(dev, iclients, win, events, count, filter,
                                      grab, client_return, mask_return);

}

/**
 * Deliver events to a window. At this point, we do not yet know if the event
 * actually needs to be delivered. May activate a grab if the event is a
 * button press.
 *
 * Core events are always delivered to the window owner. If the filter is
 * something other than CantBeFiltered, the event is also delivered to other
 * clients with the matching mask on the window.
 *
 * More than one event may be delivered at a time. This is the case with
 * DeviceMotionNotifies which may be followed by DeviceValuator events.
 *
 * @param pWin The window that would get the event.
 * @param pEvents The events to be delivered.
 * @param count Number of elements in pEvents.
 * @param filter Mask based on event type.
 * @param grab Possible grab on the device that caused the event.
 *
 * @return a positive number if at least one successful delivery has been
 * made, 0 if no events were delivered, or a negative number if the event
 * has not been delivered _and_ rejected by at least one client.
 */
int
DeliverEventsToWindow(DeviceIntPtr pDev, WindowPtr pWin, xEvent
                      *pEvents, int count, Mask filter, GrabPtr grab)
{
    int deliveries = 0, nondeliveries = 0;
    ClientPtr client = NullClient;
    Mask deliveryMask = 0;      /* If a grab occurs due to a button press, then
                                   this mask is the mask of the grab. */
    int type = pEvents->u.u.type;

    /* Deliver to window owner */
    if ((filter == CantBeFiltered) || core_get_type(pEvents) != 0) {
        enum EventDeliveryState rc;

        rc = DeliverToWindowOwner(pDev, pWin, pEvents, count, filter, grab);

        switch (rc) {
        case EVENT_SKIP:
            return 0;
        case EVENT_REJECTED:
            nondeliveries--;
            break;
        case EVENT_DELIVERED:
            /* We delivered to the owner, with our event mask */
            deliveries++;
            client = wClient(pWin);
            deliveryMask = pWin->eventMask;
            break;
        case EVENT_NOT_DELIVERED:
            break;
        }
    }

    /* CantBeFiltered means only window owner gets the event */
    if (filter != CantBeFiltered) {
        enum EventDeliveryState rc;

        rc = DeliverEventToWindowMask(pDev, pWin, pEvents, count, filter,
                                      grab, &client, &deliveryMask);

        switch (rc) {
        case EVENT_SKIP:
            return 0;
        case EVENT_REJECTED:
            nondeliveries--;
            break;
        case EVENT_DELIVERED:
            deliveries++;
            break;
        case EVENT_NOT_DELIVERED:
            break;
        }
    }

    if (deliveries) {
        /*
         * Note that since core events are delivered first, an implicit grab may
         * be activated on a core grab, stopping the XI events.
         */
        if (!grab &&
            ActivateImplicitGrab(pDev, client, pWin, pEvents, deliveryMask))
            /* grab activated */ ;
        else if (type == MotionNotify)
            pDev->valuator->motionHintWindow = pWin;
        else if (type == DeviceMotionNotify || type == DeviceButtonPress)
            CheckDeviceGrabAndHintWindow(pWin, type,
                                         (deviceKeyButtonPointer *) pEvents,
                                         grab, client, deliveryMask);
        return deliveries;
    }
    return nondeliveries;
}

/**
 * Filter out raw events for XI 2.0 and XI 2.1 clients.
 *
 * If there is a grab on the device, 2.0 clients only get raw events if they
 * have the grab. 2.1+ clients get raw events in all cases.
 *
 * @return TRUE if the event should be discarded, FALSE otherwise.
 */
static BOOL
FilterRawEvents(const ClientPtr client, const GrabPtr grab, WindowPtr root)
{
    XIClientPtr client_xi_version;
    int cmp;

    /* device not grabbed -> don't filter */
    if (!grab)
        return FALSE;

    client_xi_version =
        dixLookupPrivate(&client->devPrivates, XIClientPrivateKey);

    cmp = version_compare(client_xi_version->major_version,
                          client_xi_version->minor_version, 2, 0);
    /* XI 2.0: if device is grabbed, skip
       XI 2.1: if device is grabbed by us, skip, we've already delivered */
    if (cmp == 0)
        return TRUE;

    return (grab->window != root) ? FALSE : SameClient(grab, client);
}

/**
 * Deliver a raw event to the grab owner (if any) and to all root windows.
 *
 * Raw event delivery differs between XI 2.0 and XI 2.1.
 * XI 2.0: events delivered to the grabbing client (if any) OR to all root
 * windows
 * XI 2.1: events delivered to all root windows, regardless of grabbing
 * state.
 */
void
DeliverRawEvent(RawDeviceEvent *ev, DeviceIntPtr device)
{
    GrabPtr grab = device->deviceGrab.grab;
    xEvent *xi;
    int i, rc;
    int filter;

    rc = EventToXI2((InternalEvent *) ev, (xEvent **) &xi);
    if (rc != Success) {
        ErrorF("[Xi] %s: XI2 conversion failed in %s (%d)\n",
               __func__, device->name, rc);
        return;
    }

    if (grab)
        DeliverGrabbedEvent((InternalEvent *) ev, device, FALSE);

    filter = GetEventFilter(device, xi);

    for (i = 0; i < screenInfo.numScreens; i++) {
        WindowPtr root;
        InputClients *inputclients;

        root = screenInfo.screens[i]->root;
        if (!GetClientsForDelivery(device, root, xi, filter, &inputclients))
            continue;

        for (; inputclients; inputclients = inputclients->next) {
            ClientPtr c;        /* unused */
            Mask m;             /* unused */
            InputClients ic = *inputclients;

            /* Because we run through the list manually, copy the actual
             * list, shorten the copy to only have one client and then pass
             * that down to DeliverEventToInputClients. This way we avoid
             * double events on XI 2.1 clients that have a grab on the
             * device.
             */
            ic.next = NULL;

            if (!FilterRawEvents(rClient(&ic), grab, root))
                DeliverEventToInputClients(device, &ic, root, xi, 1,
                                           filter, NULL, &c, &m);
        }
    }

    free(xi);
}

/* If the event goes to dontClient, don't send it and return 0.  if
   send works,  return 1 or if send didn't work, return 2.
   Only works for core events.
*/

#ifdef PANORAMIX
static int
XineramaTryClientEventsResult(ClientPtr client,
                              GrabPtr grab, Mask mask, Mask filter)
{
    if ((client) && (client != serverClient) && (!client->clientGone) &&
        ((filter == CantBeFiltered) || (mask & filter))) {
        if (grab && !SameClient(grab, client))
            return -1;
        else
            return 1;
    }
    return 0;
}
#endif

/**
 * Try to deliver events to the interested parties.
 *
 * @param pWin The window that would get the event.
 * @param pEvents The events to be delivered.
 * @param count Number of elements in pEvents.
 * @param filter Mask based on event type.
 * @param dontClient Don't deliver to the dontClient.
 */
int
MaybeDeliverEventsToClient(WindowPtr pWin, xEvent *pEvents,
                           int count, Mask filter, ClientPtr dontClient)
{
    OtherClients *other;

    if (pWin->eventMask & filter) {
        if (wClient(pWin) == dontClient)
            return 0;
#ifdef PANORAMIX
        if (!noPanoramiXExtension && pWin->drawable.pScreen->myNum)
            return XineramaTryClientEventsResult(wClient(pWin), NullGrab,
                                                 pWin->eventMask, filter);
#endif
        if (XaceHook(XACE_RECEIVE_ACCESS, wClient(pWin), pWin, pEvents, count))
            return 1;           /* don't send, but pretend we did */
        return TryClientEvents(wClient(pWin), NULL, pEvents, count,
                               pWin->eventMask, filter, NullGrab);
    }
    for (other = wOtherClients(pWin); other; other = other->next) {
        if (other->mask & filter) {
            if (SameClient(other, dontClient))
                return 0;
#ifdef PANORAMIX
            if (!noPanoramiXExtension && pWin->drawable.pScreen->myNum)
                return XineramaTryClientEventsResult(rClient(other), NullGrab,
                                                     other->mask, filter);
#endif
            if (XaceHook(XACE_RECEIVE_ACCESS, rClient(other), pWin, pEvents,
                         count))
                return 1;       /* don't send, but pretend we did */
            return TryClientEvents(rClient(other), NULL, pEvents, count,
                                   other->mask, filter, NullGrab);
        }
    }
    return 2;
}

static Window
FindChildForEvent(SpritePtr pSprite, WindowPtr event)
{
    WindowPtr w = DeepestSpriteWin(pSprite);
    Window child = None;

    /* If the search ends up past the root should the child field be
       set to none or should the value in the argument be passed
       through. It probably doesn't matter since everyone calls
       this function with child == None anyway. */
    while (w) {
        /* If the source window is same as event window, child should be
           none.  Don't bother going all all the way back to the root. */

        if (w == event) {
            child = None;
            break;
        }

        if (w->parent == event) {
            child = w->drawable.id;
            break;
        }
        w = w->parent;
    }
    return child;
}

static void
FixUpXI2DeviceEventFromWindow(SpritePtr pSprite, int evtype,
                              xXIDeviceEvent *event, WindowPtr pWin, Window child)
{
    event->root = RootWindow(pSprite)->drawable.id;
    event->event = pWin->drawable.id;

    if (evtype == XI_TouchOwnership) {
        event->child = child;
        return;
    }

    if (pSprite->hot.pScreen == pWin->drawable.pScreen) {
        event->event_x = event->root_x - double_to_fp1616(pWin->drawable.x);
        event->event_y = event->root_y - double_to_fp1616(pWin->drawable.y);
        event->child = child;
    }
    else {
        event->event_x = 0;
        event->event_y = 0;
        event->child = None;
    }

    if (event->evtype == XI_Enter || event->evtype == XI_Leave ||
        event->evtype == XI_FocusIn || event->evtype == XI_FocusOut)
        ((xXIEnterEvent *) event)->same_screen =
            (pSprite->hot.pScreen == pWin->drawable.pScreen);
}

static void
FixUpXI2PinchEventFromWindow(SpritePtr pSprite, xXIGesturePinchEvent *event,
                             WindowPtr pWin, Window child)
{
    event->root = RootWindow(pSprite)->drawable.id;
    event->event = pWin->drawable.id;

    if (pSprite->hot.pScreen == pWin->drawable.pScreen) {
        event->event_x = event->root_x - double_to_fp1616(pWin->drawable.x);
        event->event_y = event->root_y - double_to_fp1616(pWin->drawable.y);
        event->child = child;
    }
    else {
        event->event_x = 0;
        event->event_y = 0;
        event->child = None;
    }
}

static void
FixUpXI2SwipeEventFromWindow(SpritePtr pSprite, xXIGestureSwipeEvent *event,
                             WindowPtr pWin, Window child)
{
    event->root = RootWindow(pSprite)->drawable.id;
    event->event = pWin->drawable.id;

    if (pSprite->hot.pScreen == pWin->drawable.pScreen) {
        event->event_x = event->root_x - double_to_fp1616(pWin->drawable.x);
        event->event_y = event->root_y - double_to_fp1616(pWin->drawable.y);
        event->child = child;
    }
    else {
        event->event_x = 0;
        event->event_y = 0;
        event->child = None;
    }
}

/**
 * Adjust event fields to comply with the window properties.
 *
 * @param xE Event to be modified in place
 * @param pWin The window to get the information from.
 * @param child Child window setting for event (if applicable)
 * @param calcChild If True, calculate the child window.
 */
void
FixUpEventFromWindow(SpritePtr pSprite,
                     xEvent *xE, WindowPtr pWin, Window child, Bool calcChild)
{
    int evtype;

    if (calcChild)
        child = FindChildForEvent(pSprite, pWin);

    if ((evtype = xi2_get_type(xE))) {
        switch (evtype) {
        case XI_RawKeyPress:
        case XI_RawKeyRelease:
        case XI_RawButtonPress:
        case XI_RawButtonRelease:
        case XI_RawMotion:
        case XI_RawTouchBegin:
        case XI_RawTouchUpdate:
        case XI_RawTouchEnd:
        case XI_DeviceChanged:
        case XI_HierarchyChanged:
        case XI_PropertyEvent:
        case XI_BarrierHit:
        case XI_BarrierLeave:
            return;
        case XI_GesturePinchBegin:
        case XI_GesturePinchUpdate:
        case XI_GesturePinchEnd:
            FixUpXI2PinchEventFromWindow(pSprite,
                                         (xXIGesturePinchEvent*) xE, pWin, child);
            break;
        case XI_GestureSwipeBegin:
        case XI_GestureSwipeUpdate:
        case XI_GestureSwipeEnd:
            FixUpXI2SwipeEventFromWindow(pSprite,
                                         (xXIGestureSwipeEvent*) xE, pWin, child);
            break;
        default:
            FixUpXI2DeviceEventFromWindow(pSprite, evtype,
                                          (xXIDeviceEvent*) xE, pWin, child);
            break;
        }
    }
    else {
        XE_KBPTR.root = RootWindow(pSprite)->drawable.id;
        XE_KBPTR.event = pWin->drawable.id;
        if (pSprite->hot.pScreen == pWin->drawable.pScreen) {
            XE_KBPTR.sameScreen = xTrue;
            XE_KBPTR.child = child;
            XE_KBPTR.eventX = XE_KBPTR.rootX - pWin->drawable.x;
            XE_KBPTR.eventY = XE_KBPTR.rootY - pWin->drawable.y;
        }
        else {
            XE_KBPTR.sameScreen = xFalse;
            XE_KBPTR.child = None;
            XE_KBPTR.eventX = 0;
            XE_KBPTR.eventY = 0;
        }
    }
}

/**
 * Check if a given event is deliverable at all on a given window.
 *
 * This function only checks if any client wants it, not for a specific
 * client.
 *
 * @param[in] dev The device this event is being sent for.
 * @param[in] evtype The event type of the event that is to be sent.
 * @param[in] win The current event window.
 *
 * @return Bitmask of ::EVENT_XI2_MASK, ::EVENT_XI1_MASK, ::EVENT_CORE_MASK, and
 *         ::EVENT_DONT_PROPAGATE_MASK.
 */
int
EventIsDeliverable(DeviceIntPtr dev, int evtype, WindowPtr win)
{
    int rc = 0;
    int filter = 0;
    int type;
    OtherInputMasks *inputMasks = wOtherInputMasks(win);

    if ((type = GetXI2Type(evtype)) != 0) {
        if (inputMasks && xi2mask_isset(inputMasks->xi2mask, dev, type))
            rc |= EVENT_XI2_MASK;
    }

    if ((type = GetXIType(evtype)) != 0) {
        filter = event_get_filter_from_type(dev, type);

        /* Check for XI mask */
        if (inputMasks &&
            (inputMasks->deliverableEvents[dev->id] & filter) &&
            (inputMasks->inputEvents[dev->id] & filter))
            rc |= EVENT_XI1_MASK;

        /* Check for XI DontPropagate mask */
        if (inputMasks && (inputMasks->dontPropagateMask[dev->id] & filter))
            rc |= EVENT_DONT_PROPAGATE_MASK;

    }

    if ((type = GetCoreType(evtype)) != 0) {
        filter = event_get_filter_from_type(dev, type);

        /* Check for core mask */
        if ((win->deliverableEvents & filter) &&
            ((wOtherEventMasks(win) | win->eventMask) & filter))
            rc |= EVENT_CORE_MASK;

        /* Check for core DontPropagate mask */
        if (filter & wDontPropagateMask(win))
            rc |= EVENT_DONT_PROPAGATE_MASK;
    }

    return rc;
}

static int
DeliverEvent(DeviceIntPtr dev, xEvent *xE, int count,
             WindowPtr win, Window child, GrabPtr grab)
{
    SpritePtr pSprite = dev->spriteInfo->sprite;
    Mask filter;
    int deliveries = 0;

    if (XaceHook(XACE_SEND_ACCESS, NULL, dev, win, xE, count) == Success) {
        filter = GetEventFilter(dev, xE);
        FixUpEventFromWindow(pSprite, xE, win, child, FALSE);
        deliveries = DeliverEventsToWindow(dev, win, xE, count, filter, grab);
    }

    return deliveries;
}

static int
DeliverOneEvent(InternalEvent *event, DeviceIntPtr dev, enum InputLevel level,
                WindowPtr win, Window child, GrabPtr grab)
{
    xEvent *xE = NULL;
    int count = 0;
    int deliveries = 0;
    int rc;

    switch (level) {
    case XI2:
        rc = EventToXI2(event, &xE);
        count = 1;
        break;
    case XI:
        rc = EventToXI(event, &xE, &count);
        break;
    case CORE:
        rc = EventToCore(event, &xE, &count);
        break;
    default:
        rc = BadImplementation;
        break;
    }

    if (rc == Success) {
        deliveries = DeliverEvent(dev, xE, count, win, child, grab);
        free(xE);
    }
    else
        BUG_WARN_MSG(rc != BadMatch,
                     "%s: conversion to level %d failed with rc %d\n",
                     dev->name, level, rc);
    return deliveries;
}

/**
 * Deliver events caused by input devices.
 *
 * For events from a non-grabbed, non-focus device, DeliverDeviceEvents is
 * called directly from the processInputProc.
 * For grabbed devices, DeliverGrabbedEvent is called first, and _may_ call
 * DeliverDeviceEvents.
 * For focused events, DeliverFocusedEvent is called first, and _may_ call
 * DeliverDeviceEvents.
 *
 * @param pWin Window to deliver event to.
 * @param event The events to deliver, not yet in wire format.
 * @param grab Possible grab on a device.
 * @param stopAt Don't recurse up to the root window.
 * @param dev The device that is responsible for the event.
 *
 * @see DeliverGrabbedEvent
 * @see DeliverFocusedEvent
 */
int
DeliverDeviceEvents(WindowPtr pWin, InternalEvent *event, GrabPtr grab,
                    WindowPtr stopAt, DeviceIntPtr dev)
{
    Window child = None;
    int deliveries = 0;
    int mask;

    verify_internal_event(event);

    while (pWin) {
        if ((mask = EventIsDeliverable(dev, event->any.type, pWin))) {
            /* XI2 events first */
            if (mask & EVENT_XI2_MASK) {
                deliveries =
                    DeliverOneEvent(event, dev, XI2, pWin, child, grab);
                if (deliveries > 0)
                    break;
            }

            /* XI events */
            if (mask & EVENT_XI1_MASK) {
                deliveries = DeliverOneEvent(event, dev, XI, pWin, child, grab);
                if (deliveries > 0)
                    break;
            }

            /* Core event */
            if ((mask & EVENT_CORE_MASK) && IsMaster(dev) && dev->coreEvents) {
                deliveries =
                    DeliverOneEvent(event, dev, CORE, pWin, child, grab);
                if (deliveries > 0)
                    break;
            }

        }

        if ((deliveries < 0) || (pWin == stopAt) ||
            (mask & EVENT_DONT_PROPAGATE_MASK)) {
            deliveries = 0;
            break;
        }

        child = pWin->drawable.id;
        pWin = pWin->parent;
    }

    return deliveries;
}

/**
 * Deliver event to a window and its immediate parent. Used for most window
 * events (CreateNotify, ConfigureNotify, etc.). Not useful for events that
 * propagate up the tree or extension events
 *
 * In case of a ReparentNotify event, the event will be delivered to the
 * otherParent as well.
 *
 * @param pWin Window to deliver events to.
 * @param xE Events to deliver.
 * @param count number of events in xE.
 * @param otherParent Used for ReparentNotify events.
 */
int
DeliverEvents(WindowPtr pWin, xEvent *xE, int count, WindowPtr otherParent)
{
    DeviceIntRec dummy;
    int deliveries;

#ifdef PANORAMIX
    if (!noPanoramiXExtension && pWin->drawable.pScreen->myNum)
        return count;
#endif

    if (!count)
        return 0;

    dummy.id = XIAllDevices;

    switch (xE->u.u.type) {
    case DestroyNotify:
    case UnmapNotify:
    case MapNotify:
    case MapRequest:
    case ReparentNotify:
    case ConfigureNotify:
    case ConfigureRequest:
    case GravityNotify:
    case CirculateNotify:
    case CirculateRequest:
        xE->u.destroyNotify.event = pWin->drawable.id;
        break;
    }

    switch (xE->u.u.type) {
    case DestroyNotify:
    case UnmapNotify:
    case MapNotify:
    case ReparentNotify:
    case ConfigureNotify:
    case GravityNotify:
    case CirculateNotify:
        break;
    default:
    {
        Mask filter;

        filter = GetEventFilter(&dummy, xE);
        return DeliverEventsToWindow(&dummy, pWin, xE, count, filter, NullGrab);
    }
    }

    deliveries = DeliverEventsToWindow(&dummy, pWin, xE, count,
                                       StructureNotifyMask, NullGrab);
    if (pWin->parent) {
        xE->u.destroyNotify.event = pWin->parent->drawable.id;
        deliveries += DeliverEventsToWindow(&dummy, pWin->parent, xE, count,
                                            SubstructureNotifyMask, NullGrab);
        if (xE->u.u.type == ReparentNotify) {
            xE->u.destroyNotify.event = otherParent->drawable.id;
            deliveries += DeliverEventsToWindow(&dummy,
                                                otherParent, xE, count,
                                                SubstructureNotifyMask,
                                                NullGrab);
        }
    }
    return deliveries;
}

Bool
PointInBorderSize(WindowPtr pWin, int x, int y)
{
    BoxRec box;

    if (RegionContainsPoint(&pWin->borderSize, x, y, &box))
        return TRUE;

#ifdef PANORAMIX
    if (!noPanoramiXExtension &&
        XineramaSetWindowPntrs(inputInfo.pointer, pWin)) {
        SpritePtr pSprite = inputInfo.pointer->spriteInfo->sprite;
        int i;

        FOR_NSCREENS_FORWARD_SKIP(i) {
            if (RegionContainsPoint(&pSprite->windows[i]->borderSize,
                                    x + screenInfo.screens[0]->x -
                                    screenInfo.screens[i]->x,
                                    y + screenInfo.screens[0]->y -
                                    screenInfo.screens[i]->y, &box))
                return TRUE;
        }
    }
#endif
    return FALSE;
}

/**
 * Traversed from the root window to the window at the position x/y. While
 * traversing, it sets up the traversal history in the spriteTrace array.
 * After completing, the spriteTrace history is set in the following way:
 *   spriteTrace[0] ... root window
 *   spriteTrace[1] ... top level window that encloses x/y
 *       ...
 *   spriteTrace[spriteTraceGood - 1] ... window at x/y
 *
 * @returns the window at the given coordinates.
 */
WindowPtr
XYToWindow(SpritePtr pSprite, int x, int y)
{
    ScreenPtr pScreen = RootWindow(pSprite)->drawable.pScreen;

    return (*pScreen->XYToWindow)(pScreen, pSprite, x, y);
}

/**
 * Ungrab a currently FocusIn grabbed device and grab the device on the
 * given window. If the win given is the NoneWin, the device is ungrabbed if
 * applicable and FALSE is returned.
 *
 * @returns TRUE if the device has been grabbed, or FALSE otherwise.
 */
BOOL
ActivateFocusInGrab(DeviceIntPtr dev, WindowPtr old, WindowPtr win)
{
    BOOL rc = FALSE;
    InternalEvent event;

    if (dev->deviceGrab.grab) {
        if (!dev->deviceGrab.fromPassiveGrab ||
            dev->deviceGrab.grab->type != XI_FocusIn ||
            dev->deviceGrab.grab->window == win ||
            IsParent(dev->deviceGrab.grab->window, win))
            return FALSE;
        DoEnterLeaveEvents(dev, dev->id, old, win, XINotifyPassiveUngrab);
        (*dev->deviceGrab.DeactivateGrab) (dev);
    }

    if (win == NoneWin || win == PointerRootWin)
        return FALSE;

    event = (InternalEvent) {
        .device_event.header = ET_Internal,
        .device_event.type = ET_FocusIn,
        .device_event.length = sizeof(DeviceEvent),
        .device_event.time = GetTimeInMillis(),
        .device_event.deviceid = dev->id,
        .device_event.sourceid = dev->id,
        .device_event.detail.button = 0
    };
    rc = (CheckPassiveGrabsOnWindow(win, dev, &event, FALSE,
                                    TRUE) != NULL);
    if (rc)
        DoEnterLeaveEvents(dev, dev->id, old, win, XINotifyPassiveGrab);
    return rc;
}

/**
 * Ungrab a currently Enter grabbed device and grab the device for the given
 * window.
 *
 * @returns TRUE if the device has been grabbed, or FALSE otherwise.
 */
static BOOL
ActivateEnterGrab(DeviceIntPtr dev, WindowPtr old, WindowPtr win)
{
    BOOL rc = FALSE;
    InternalEvent event;

    if (dev->deviceGrab.grab) {
        if (!dev->deviceGrab.fromPassiveGrab ||
            dev->deviceGrab.grab->type != XI_Enter ||
            dev->deviceGrab.grab->window == win ||
            IsParent(dev->deviceGrab.grab->window, win))
            return FALSE;
        DoEnterLeaveEvents(dev, dev->id, old, win, XINotifyPassiveUngrab);
        (*dev->deviceGrab.DeactivateGrab) (dev);
    }

    event = (InternalEvent) {
        .device_event.header = ET_Internal,
        .device_event.type = ET_Enter,
        .device_event.length = sizeof(DeviceEvent),
        .device_event.time = GetTimeInMillis(),
        .device_event.deviceid = dev->id,
        .device_event.sourceid = dev->id,
        .device_event.detail.button = 0
    };
    rc = (CheckPassiveGrabsOnWindow(win, dev, &event, FALSE,
                                    TRUE) != NULL);
    if (rc)
        DoEnterLeaveEvents(dev, dev->id, old, win, XINotifyPassiveGrab);
    return rc;
}

/**
 * Update the sprite coordinates based on the event. Update the cursor
 * position, then update the event with the new coordinates that may have been
 * changed. If the window underneath the sprite has changed, change to new
 * cursor and send enter/leave events.
 *
 * CheckMotion() will not do anything and return FALSE if the event is not a
 * pointer event.
 *
 * @return TRUE if the sprite has moved or FALSE otherwise.
 */
Bool
CheckMotion(DeviceEvent *ev, DeviceIntPtr pDev)
{
    WindowPtr prevSpriteWin, newSpriteWin;
    SpritePtr pSprite = pDev->spriteInfo->sprite;

    verify_internal_event((InternalEvent *) ev);

    prevSpriteWin = pSprite->win;

    if (ev && !syncEvents.playingEvents) {
        /* GetPointerEvents() guarantees that pointer events have the correct
           rootX/Y set already. */
        switch (ev->type) {
        case ET_ButtonPress:
        case ET_ButtonRelease:
        case ET_Motion:
        case ET_TouchBegin:
        case ET_TouchUpdate:
        case ET_TouchEnd:
            break;
        default:
            /* all other events return FALSE */
            return FALSE;
        }

#ifdef PANORAMIX
        if (!noPanoramiXExtension) {
            /* Motion events entering DIX get translated to Screen 0
               coordinates.  Replayed events have already been
               translated since they've entered DIX before */
            ev->root_x += pSprite->screen->x - screenInfo.screens[0]->x;
            ev->root_y += pSprite->screen->y - screenInfo.screens[0]->y;
        }
        else
#endif
        {
            if (pSprite->hot.pScreen != pSprite->hotPhys.pScreen) {
                pSprite->hot.pScreen = pSprite->hotPhys.pScreen;
                RootWindow(pDev->spriteInfo->sprite) =
                    pSprite->hot.pScreen->root;
            }
        }

        pSprite->hot.x = ev->root_x;
        pSprite->hot.y = ev->root_y;
        if (pSprite->hot.x < pSprite->physLimits.x1)
            pSprite->hot.x = pSprite->physLimits.x1;
        else if (pSprite->hot.x >= pSprite->physLimits.x2)
            pSprite->hot.x = pSprite->physLimits.x2 - 1;
        if (pSprite->hot.y < pSprite->physLimits.y1)
            pSprite->hot.y = pSprite->physLimits.y1;
        else if (pSprite->hot.y >= pSprite->physLimits.y2)
            pSprite->hot.y = pSprite->physLimits.y2 - 1;
        if (pSprite->hotShape)
            ConfineToShape(pDev, pSprite->hotShape, &pSprite->hot.x,
                           &pSprite->hot.y);
        pSprite->hotPhys = pSprite->hot;

        if ((pSprite->hotPhys.x != ev->root_x) ||
            (pSprite->hotPhys.y != ev->root_y)) {
#ifdef PANORAMIX
            if (!noPanoramiXExtension) {
                XineramaSetCursorPosition(pDev, pSprite->hotPhys.x,
                                          pSprite->hotPhys.y, FALSE);
            }
            else
#endif
            {
                (*pSprite->hotPhys.pScreen->SetCursorPosition) (pDev,
                                                                pSprite->
                                                                hotPhys.pScreen,
                                                                pSprite->
                                                                hotPhys.x,
                                                                pSprite->
                                                                hotPhys.y,
                                                                FALSE);
            }
        }

        ev->root_x = pSprite->hot.x;
        ev->root_y = pSprite->hot.y;
    }

    newSpriteWin = XYToWindow(pSprite, pSprite->hot.x, pSprite->hot.y);

    if (newSpriteWin != prevSpriteWin) {
        int sourceid;

        if (!ev) {
            UpdateCurrentTimeIf();
            sourceid = pDev->id;        /* when from WindowsRestructured */
        }
        else
            sourceid = ev->sourceid;

        if (prevSpriteWin != NullWindow) {
            if (!ActivateEnterGrab(pDev, prevSpriteWin, newSpriteWin))
                DoEnterLeaveEvents(pDev, sourceid, prevSpriteWin,
                                   newSpriteWin, NotifyNormal);
        }
        /* set pSprite->win after ActivateEnterGrab, otherwise
           sprite window == grab_window and no enter/leave events are
           sent. */
        pSprite->win = newSpriteWin;
        PostNewCursor(pDev);
        return FALSE;
    }
    return TRUE;
}

/**
 * Windows have restructured, we need to update the sprite position and the
 * sprite's cursor.
 */
void
WindowsRestructured(void)
{
    DeviceIntPtr pDev = inputInfo.devices;

    while (pDev) {
        if (IsMaster(pDev) || IsFloating(pDev))
            CheckMotion(NULL, pDev);
        pDev = pDev->next;
    }
}

#ifdef PANORAMIX
/* This was added to support reconfiguration under Xdmx.  The problem is
 * that if the 0th screen (i.e., screenInfo.screens[0]) is moved to an origin
 * other than 0,0, the information in the private sprite structure must
 * be updated accordingly, or XYToWindow (and other routines) will not
 * compute correctly. */
void
ReinitializeRootWindow(WindowPtr win, int xoff, int yoff)
{
    GrabPtr grab;
    DeviceIntPtr pDev;
    SpritePtr pSprite;

    if (noPanoramiXExtension)
        return;

    pDev = inputInfo.devices;
    while (pDev) {
        if (DevHasCursor(pDev)) {
            pSprite = pDev->spriteInfo->sprite;
            pSprite->hot.x -= xoff;
            pSprite->hot.y -= yoff;

            pSprite->hotPhys.x -= xoff;
            pSprite->hotPhys.y -= yoff;

            pSprite->hotLimits.x1 -= xoff;
            pSprite->hotLimits.y1 -= yoff;
            pSprite->hotLimits.x2 -= xoff;
            pSprite->hotLimits.y2 -= yoff;

            if (RegionNotEmpty(&pSprite->Reg1))
                RegionTranslate(&pSprite->Reg1, xoff, yoff);
            if (RegionNotEmpty(&pSprite->Reg2))
                RegionTranslate(&pSprite->Reg2, xoff, yoff);

            /* FIXME: if we call ConfineCursorToWindow, must we do anything else? */
            if ((grab = pDev->deviceGrab.grab) && grab->confineTo) {
                if (grab->confineTo->drawable.pScreen
                    != pSprite->hotPhys.pScreen)
                    pSprite->hotPhys.x = pSprite->hotPhys.y = 0;
                ConfineCursorToWindow(pDev, grab->confineTo, TRUE, TRUE);
            }
            else
                ConfineCursorToWindow(pDev,
                                      pSprite->hotPhys.pScreen->root,
                                      TRUE, FALSE);

        }
        pDev = pDev->next;
    }
}
#endif

/**
 * Initialize a sprite for the given device and set it to some sane values. If
 * the device already has a sprite alloc'd, don't realloc but just reset to
 * default values.
 * If a window is supplied, the sprite will be initialized with the window's
 * cursor and positioned in the center of the window's screen. The root window
 * is a good choice to pass in here.
 *
 * It's a good idea to call it only for pointer devices, unless you have a
 * really talented keyboard.
 *
 * @param pDev The device to initialize.
 * @param pWin The window where to generate the sprite in.
 *
 */
void
InitializeSprite(DeviceIntPtr pDev, WindowPtr pWin)
{
    SpritePtr pSprite;
    ScreenPtr pScreen;
    CursorPtr pCursor;

    if (!pDev->spriteInfo->sprite) {
        DeviceIntPtr it;

        pDev->spriteInfo->sprite = (SpritePtr) calloc(1, sizeof(SpriteRec));
        if (!pDev->spriteInfo->sprite)
            FatalError("InitializeSprite: failed to allocate sprite struct");

        /* We may have paired another device with this device before our
         * device had a actual sprite. We need to check for this and reset the
         * sprite field for all paired devices.
         *
         * The VCK is always paired with the VCP before the VCP has a sprite.
         */
        for (it = inputInfo.devices; it; it = it->next) {
            if (it->spriteInfo->paired == pDev)
                it->spriteInfo->sprite = pDev->spriteInfo->sprite;
        }
        if (inputInfo.keyboard->spriteInfo->paired == pDev)
            inputInfo.keyboard->spriteInfo->sprite = pDev->spriteInfo->sprite;
    }

    pSprite = pDev->spriteInfo->sprite;
    pDev->spriteInfo->spriteOwner = TRUE;

    pScreen = (pWin) ? pWin->drawable.pScreen : (ScreenPtr) NULL;
    pSprite->hot.pScreen = pScreen;
    pSprite->hotPhys.pScreen = pScreen;
    if (pScreen) {
        pSprite->hotPhys.x = pScreen->width / 2;
        pSprite->hotPhys.y = pScreen->height / 2;
        pSprite->hotLimits.x2 = pScreen->width;
        pSprite->hotLimits.y2 = pScreen->height;
    }

    pSprite->hot = pSprite->hotPhys;
    pSprite->win = pWin;

    if (pWin) {
        pCursor = wCursor(pWin);
        pSprite->spriteTrace = (WindowPtr *) calloc(1, 32 * sizeof(WindowPtr));
        if (!pSprite->spriteTrace)
            FatalError("Failed to allocate spriteTrace");
        pSprite->spriteTraceSize = 32;

        RootWindow(pDev->spriteInfo->sprite) = pWin;
        pSprite->spriteTraceGood = 1;

        pSprite->pEnqueueScreen = pScreen;
        pSprite->pDequeueScreen = pSprite->pEnqueueScreen;

    }
    else {
        pCursor = NullCursor;
        pSprite->spriteTrace = NULL;
        pSprite->spriteTraceSize = 0;
        pSprite->spriteTraceGood = 0;
        pSprite->pEnqueueScreen = screenInfo.screens[0];
        pSprite->pDequeueScreen = pSprite->pEnqueueScreen;
    }
    pCursor = RefCursor(pCursor);
    if (pSprite->current)
        FreeCursor(pSprite->current, None);
    pSprite->current = pCursor;

    if (pScreen) {
        (*pScreen->RealizeCursor) (pDev, pScreen, pSprite->current);
        (*pScreen->CursorLimits) (pDev, pScreen, pSprite->current,
                                  &pSprite->hotLimits, &pSprite->physLimits);
        pSprite->confined = FALSE;

        (*pScreen->ConstrainCursor) (pDev, pScreen, &pSprite->physLimits);
        (*pScreen->SetCursorPosition) (pDev, pScreen, pSprite->hot.x,
                                       pSprite->hot.y, FALSE);
        (*pScreen->DisplayCursor) (pDev, pScreen, pSprite->current);
    }
#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
        pSprite->hotLimits.x1 = -screenInfo.screens[0]->x;
        pSprite->hotLimits.y1 = -screenInfo.screens[0]->y;
        pSprite->hotLimits.x2 = PanoramiXPixWidth - screenInfo.screens[0]->x;
        pSprite->hotLimits.y2 = PanoramiXPixHeight - screenInfo.screens[0]->y;
        pSprite->physLimits = pSprite->hotLimits;
        pSprite->confineWin = NullWindow;
        pSprite->hotShape = NullRegion;
        pSprite->screen = pScreen;
        /* gotta UNINIT these someplace */
        RegionNull(&pSprite->Reg1);
        RegionNull(&pSprite->Reg2);
    }
#endif
}

void FreeSprite(DeviceIntPtr dev)
{
    if (DevHasCursor(dev) && dev->spriteInfo->sprite) {
        if (dev->spriteInfo->sprite->current)
            FreeCursor(dev->spriteInfo->sprite->current, None);
        free(dev->spriteInfo->sprite->spriteTrace);
        free(dev->spriteInfo->sprite);
    }
    dev->spriteInfo->sprite = NULL;
}


/**
 * Update the mouse sprite info when the server switches from a pScreen to another.
 * Otherwise, the pScreen of the mouse sprite is never updated when we switch
 * from a pScreen to another. Never updating the pScreen of the mouse sprite
 * implies that windows that are in pScreen whose pScreen->myNum >0 will never
 * get pointer events. This is  because in CheckMotion(), sprite.hotPhys.pScreen
 * always points to the first pScreen it has been set by
 * DefineInitialRootWindow().
 *
 * Calling this function is useful for use cases where the server
 * has more than one pScreen.
 * This function is similar to DefineInitialRootWindow() but it does not
 * reset the mouse pointer position.
 * @param win must be the new pScreen we are switching to.
 */
void
UpdateSpriteForScreen(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    SpritePtr pSprite = NULL;
    WindowPtr win = NULL;
    CursorPtr pCursor;

    if (!pScreen)
        return;

    if (!pDev->spriteInfo->sprite)
        return;

    pSprite = pDev->spriteInfo->sprite;

    win = pScreen->root;

    pSprite->hotPhys.pScreen = pScreen;
    pSprite->hot = pSprite->hotPhys;
    pSprite->hotLimits.x2 = pScreen->width;
    pSprite->hotLimits.y2 = pScreen->height;
    pSprite->win = win;
    pCursor = RefCursor(wCursor(win));
    if (pSprite->current)
        FreeCursor(pSprite->current, 0);
    pSprite->current = pCursor;
    pSprite->spriteTraceGood = 1;
    pSprite->spriteTrace[0] = win;
    (*pScreen->CursorLimits) (pDev,
                              pScreen,
                              pSprite->current,
                              &pSprite->hotLimits, &pSprite->physLimits);
    pSprite->confined = FALSE;
    (*pScreen->ConstrainCursor) (pDev, pScreen, &pSprite->physLimits);
    (*pScreen->DisplayCursor) (pDev, pScreen, pSprite->current);

#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
        pSprite->hotLimits.x1 = -screenInfo.screens[0]->x;
        pSprite->hotLimits.y1 = -screenInfo.screens[0]->y;
        pSprite->hotLimits.x2 = PanoramiXPixWidth - screenInfo.screens[0]->x;
        pSprite->hotLimits.y2 = PanoramiXPixHeight - screenInfo.screens[0]->y;
        pSprite->physLimits = pSprite->hotLimits;
        pSprite->screen = pScreen;
    }
#endif
}

/*
 * This does not take any shortcuts, and even ignores its argument, since
 * it does not happen very often, and one has to walk up the tree since
 * this might be a newly instantiated cursor for an intermediate window
 * between the one the pointer is in and the one that the last cursor was
 * instantiated from.
 */
void
WindowHasNewCursor(WindowPtr pWin)
{
    DeviceIntPtr pDev;

    for (pDev = inputInfo.devices; pDev; pDev = pDev->next)
        if (DevHasCursor(pDev))
            PostNewCursor(pDev);
}

void
NewCurrentScreen(DeviceIntPtr pDev, ScreenPtr newScreen, int x, int y)
{
    DeviceIntPtr ptr;
    SpritePtr pSprite;

    ptr =
        IsFloating(pDev) ? pDev :
        GetXTestDevice(GetMaster(pDev, MASTER_POINTER));
    pSprite = ptr->spriteInfo->sprite;

    pSprite->hotPhys.x = x;
    pSprite->hotPhys.y = y;
#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
        pSprite->hotPhys.x += newScreen->x - screenInfo.screens[0]->x;
        pSprite->hotPhys.y += newScreen->y - screenInfo.screens[0]->y;
        if (newScreen != pSprite->screen) {
            pSprite->screen = newScreen;
            /* Make sure we tell the DDX to update its copy of the screen */
            if (pSprite->confineWin)
                XineramaConfineCursorToWindow(ptr, pSprite->confineWin, TRUE);
            else
                XineramaConfineCursorToWindow(ptr, screenInfo.screens[0]->root,
                                              TRUE);
            /* if the pointer wasn't confined, the DDX won't get
               told of the pointer warp so we reposition it here */
            if (!syncEvents.playingEvents)
                (*pSprite->screen->SetCursorPosition) (ptr,
                                                       pSprite->screen,
                                                       pSprite->hotPhys.x +
                                                       screenInfo.screens[0]->
                                                       x - pSprite->screen->x,
                                                       pSprite->hotPhys.y +
                                                       screenInfo.screens[0]->
                                                       y - pSprite->screen->y,
                                                       FALSE);
        }
    }
    else
#endif
    if (newScreen != pSprite->hotPhys.pScreen)
        ConfineCursorToWindow(ptr, newScreen->root, TRUE, FALSE);
}

#ifdef PANORAMIX

static Bool
XineramaPointInWindowIsVisible(WindowPtr pWin, int x, int y)
{
    BoxRec box;
    int i, xoff, yoff;

    if (!pWin->realized)
        return FALSE;

    if (RegionContainsPoint(&pWin->borderClip, x, y, &box))
        return TRUE;

    if (!XineramaSetWindowPntrs(inputInfo.pointer, pWin))
         return FALSE;

    xoff = x + screenInfo.screens[0]->x;
    yoff = y + screenInfo.screens[0]->y;

    FOR_NSCREENS_FORWARD_SKIP(i) {
        pWin = inputInfo.pointer->spriteInfo->sprite->windows[i];

        x = xoff - screenInfo.screens[i]->x;
        y = yoff - screenInfo.screens[i]->y;

        if (RegionContainsPoint(&pWin->borderClip, x, y, &box)
            && (!wInputShape(pWin) ||
                RegionContainsPoint(wInputShape(pWin),
                                    x - pWin->drawable.x,
                                    y - pWin->drawable.y, &box)))
            return TRUE;

    }

    return FALSE;
}

static int
XineramaWarpPointer(ClientPtr client)
{
    WindowPtr dest = NULL;
    int x, y, rc;
    SpritePtr pSprite = PickPointer(client)->spriteInfo->sprite;

    REQUEST(xWarpPointerReq);

    if (stuff->dstWid != None) {
        rc = dixLookupWindow(&dest, stuff->dstWid, client, DixReadAccess);
        if (rc != Success)
            return rc;
    }
    x = pSprite->hotPhys.x;
    y = pSprite->hotPhys.y;

    if (stuff->srcWid != None) {
        int winX, winY;
        XID winID = stuff->srcWid;
        WindowPtr source;

        rc = dixLookupWindow(&source, winID, client, DixReadAccess);
        if (rc != Success)
            return rc;

        winX = source->drawable.x;
        winY = source->drawable.y;
        if (source == screenInfo.screens[0]->root) {
            winX -= screenInfo.screens[0]->x;
            winY -= screenInfo.screens[0]->y;
        }
        if (x < winX + stuff->srcX ||
            y < winY + stuff->srcY ||
            (stuff->srcWidth != 0 &&
             winX + stuff->srcX + (int) stuff->srcWidth < x) ||
            (stuff->srcHeight != 0 &&
             winY + stuff->srcY + (int) stuff->srcHeight < y) ||
            !XineramaPointInWindowIsVisible(source, x, y))
            return Success;
    }
    if (dest) {
        x = dest->drawable.x;
        y = dest->drawable.y;
        if (dest == screenInfo.screens[0]->root) {
            x -= screenInfo.screens[0]->x;
            y -= screenInfo.screens[0]->y;
        }
    }

    x += stuff->dstX;
    y += stuff->dstY;

    if (x < pSprite->physLimits.x1)
        x = pSprite->physLimits.x1;
    else if (x >= pSprite->physLimits.x2)
        x = pSprite->physLimits.x2 - 1;
    if (y < pSprite->physLimits.y1)
        y = pSprite->physLimits.y1;
    else if (y >= pSprite->physLimits.y2)
        y = pSprite->physLimits.y2 - 1;
    if (pSprite->hotShape)
        ConfineToShape(PickPointer(client), pSprite->hotShape, &x, &y);

    XineramaSetCursorPosition(PickPointer(client), x, y, TRUE);

    return Success;
}

#endif

/**
 * Server-side protocol handling for WarpPointer request.
 * Warps the cursor position to the coordinates given in the request.
 */
int
ProcWarpPointer(ClientPtr client)
{
    WindowPtr dest = NULL;
    int x, y, rc;
    ScreenPtr newScreen;
    DeviceIntPtr dev, tmp;
    SpritePtr pSprite;

    REQUEST(xWarpPointerReq);
    REQUEST_SIZE_MATCH(xWarpPointerReq);

    dev = PickPointer(client);

    for (tmp = inputInfo.devices; tmp; tmp = tmp->next) {
        if (GetMaster(tmp, MASTER_ATTACHED) == dev) {
            rc = XaceHook(XACE_DEVICE_ACCESS, client, dev, DixWriteAccess);
            if (rc != Success)
                return rc;
        }
    }

    if (dev->lastSlave)
        dev = dev->lastSlave;
    pSprite = dev->spriteInfo->sprite;

#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        return XineramaWarpPointer(client);
#endif

    if (stuff->dstWid != None) {
        rc = dixLookupWindow(&dest, stuff->dstWid, client, DixGetAttrAccess);
        if (rc != Success)
            return rc;
    }
    x = pSprite->hotPhys.x;
    y = pSprite->hotPhys.y;

    if (stuff->srcWid != None) {
        int winX, winY;
        XID winID = stuff->srcWid;
        WindowPtr source;

        rc = dixLookupWindow(&source, winID, client, DixGetAttrAccess);
        if (rc != Success)
            return rc;

        winX = source->drawable.x;
        winY = source->drawable.y;
        if (source->drawable.pScreen != pSprite->hotPhys.pScreen ||
            x < winX + stuff->srcX ||
            y < winY + stuff->srcY ||
            (stuff->srcWidth != 0 &&
             winX + stuff->srcX + (int) stuff->srcWidth < x) ||
            (stuff->srcHeight != 0 &&
             winY + stuff->srcY + (int) stuff->srcHeight < y) ||
            (source->parent && !PointInWindowIsVisible(source, x, y)))
            return Success;
    }
    if (dest) {
        x = dest->drawable.x;
        y = dest->drawable.y;
        newScreen = dest->drawable.pScreen;
    }
    else
        newScreen = pSprite->hotPhys.pScreen;

    x += stuff->dstX;
    y += stuff->dstY;

    if (x < 0)
        x = 0;
    else if (x >= newScreen->width)
        x = newScreen->width - 1;
    if (y < 0)
        y = 0;
    else if (y >= newScreen->height)
        y = newScreen->height - 1;

    if (newScreen == pSprite->hotPhys.pScreen) {
        if (x < pSprite->physLimits.x1)
            x = pSprite->physLimits.x1;
        else if (x >= pSprite->physLimits.x2)
            x = pSprite->physLimits.x2 - 1;
        if (y < pSprite->physLimits.y1)
            y = pSprite->physLimits.y1;
        else if (y >= pSprite->physLimits.y2)
            y = pSprite->physLimits.y2 - 1;
        if (pSprite->hotShape)
            ConfineToShape(dev, pSprite->hotShape, &x, &y);
        (*newScreen->SetCursorPosition) (dev, newScreen, x, y, TRUE);
    }
    else if (!PointerConfinedToScreen(dev)) {
        NewCurrentScreen(dev, newScreen, x, y);
    }
    if (*newScreen->CursorWarpedTo)
        (*newScreen->CursorWarpedTo) (dev, newScreen, client,
                                      dest, pSprite, x, y);
    return Success;
}

static Bool
BorderSizeNotEmpty(DeviceIntPtr pDev, WindowPtr pWin)
{
    if (RegionNotEmpty(&pWin->borderSize))
        return TRUE;

#ifdef PANORAMIX
    if (!noPanoramiXExtension && XineramaSetWindowPntrs(pDev, pWin)) {
        int i;

        FOR_NSCREENS_FORWARD_SKIP(i) {
            if (RegionNotEmpty
                (&pDev->spriteInfo->sprite->windows[i]->borderSize))
                return TRUE;
        }
    }
#endif
    return FALSE;
}

/**
 * Activate the given passive grab. If the grab is activated successfully, the
 * event has been delivered to the client.
 *
 * @param device The device of the event to check.
 * @param grab The grab to check.
 * @param event The current device event.
 * @param real_event The original event, in case of touch emulation. The
 * real event is the one stored in the sync queue.
 *
 * @return Whether the grab has been activated.
 */
Bool
ActivatePassiveGrab(DeviceIntPtr device, GrabPtr grab, InternalEvent *event,
                    InternalEvent *real_event)
{
    SpritePtr pSprite = device->spriteInfo->sprite;
    xEvent *xE = NULL;
    int count;
    int rc;

    /* The only consumers of corestate are Xi 1.x and core events, which
     * are guaranteed to come from DeviceEvents. */
    if (grab->grabtype == XI || grab->grabtype == CORE) {
        DeviceIntPtr gdev;

        event->device_event.corestate &= 0x1f00;

        if (grab->grabtype == CORE)
            gdev = GetMaster(device, KEYBOARD_OR_FLOAT);
        else
            gdev = grab->modifierDevice;

        if (gdev && gdev->key && gdev->key->xkbInfo)
            event->device_event.corestate |=
                gdev->key->xkbInfo->state.grab_mods & (~0x1f00);
    }

    if (grab->grabtype == CORE) {
        rc = EventToCore(event, &xE, &count);
        if (rc != Success) {
            BUG_WARN_MSG(rc != BadMatch, "[dix] %s: core conversion failed"
                         "(%d, %d).\n", device->name, event->any.type, rc);
            return FALSE;
        }
    }
    else if (grab->grabtype == XI2) {
        rc = EventToXI2(event, &xE);
        if (rc != Success) {
            if (rc != BadMatch)
                BUG_WARN_MSG(rc != BadMatch, "[dix] %s: XI2 conversion failed"
                             "(%d, %d).\n", device->name, event->any.type, rc);
            return FALSE;
        }
        count = 1;
    }
    else {
        rc = EventToXI(event, &xE, &count);
        if (rc != Success) {
            if (rc != BadMatch)
                BUG_WARN_MSG(rc != BadMatch, "[dix] %s: XI conversion failed"
                             "(%d, %d).\n", device->name, event->any.type, rc);
            return FALSE;
        }
    }

    ActivateGrabNoDelivery(device, grab, event, real_event);

    if (xE) {
        FixUpEventFromWindow(pSprite, xE, grab->window, None, TRUE);

        /* XXX: XACE? */
        TryClientEvents(rClient(grab), device, xE, count,
                        GetEventFilter(device, xE),
                        GetEventFilter(device, xE), grab);
    }

    free(xE);
    return TRUE;
}

/**
 * Activates a grab without event delivery.
 *
 * @param device The device of the event to check.
 * @param grab The grab to check.
 * @param event The current device event.
 * @param real_event The original event, in case of touch emulation. The
 * real event is the one stored in the sync queue.
 */
void ActivateGrabNoDelivery(DeviceIntPtr dev, GrabPtr grab,
                            InternalEvent *event, InternalEvent *real_event)
{
    GrabInfoPtr grabinfo = &dev->deviceGrab;
    (*grabinfo->ActivateGrab) (dev, grab,
                               ClientTimeToServerTime(event->any.time), TRUE);

    if (grabinfo->sync.state == FROZEN_NO_EVENT)
        grabinfo->sync.state = FROZEN_WITH_EVENT;
    CopyPartialInternalEvent(grabinfo->sync.event, real_event);
}

static BOOL
CoreGrabInterferes(DeviceIntPtr device, GrabPtr grab)
{
    DeviceIntPtr other;
    BOOL interfering = FALSE;

    for (other = inputInfo.devices; other; other = other->next) {
        GrabPtr othergrab = other->deviceGrab.grab;

        if (othergrab && othergrab->grabtype == CORE &&
            SameClient(grab, rClient(othergrab)) &&
            ((IsPointerDevice(grab->device) &&
              IsPointerDevice(othergrab->device)) ||
             (IsKeyboardDevice(grab->device) &&
              IsKeyboardDevice(othergrab->device)))) {
            interfering = TRUE;
            break;
        }
    }

    return interfering;
}

enum MatchFlags {
    NO_MATCH = 0x0,
    CORE_MATCH = 0x1,
    XI_MATCH = 0x2,
    XI2_MATCH = 0x4,
};

/**
 * Match the grab against the temporary grab on the given input level.
 * Modifies the temporary grab pointer.
 *
 * @param grab The grab to match against
 * @param tmp The temporary grab to use for matching
 * @param level The input level we want to match on
 * @param event_type Wire protocol event type
 *
 * @return The respective matched flag or 0 for no match
 */
static enum MatchFlags
MatchForType(const GrabPtr grab, GrabPtr tmp, enum InputLevel level,
             int event_type)
{
    enum MatchFlags match;
    BOOL ignore_device = FALSE;
    int grabtype;
    int evtype;

    switch (level) {
    case XI2:
        grabtype = XI2;
        evtype = GetXI2Type(event_type);
        BUG_WARN(!evtype);
        match = XI2_MATCH;
        break;
    case XI:
        grabtype = XI;
        evtype = GetXIType(event_type);
        match = XI_MATCH;
        break;
    case CORE:
        grabtype = CORE;
        evtype = GetCoreType(event_type);
        match = CORE_MATCH;
        ignore_device = TRUE;
        break;
    default:
        return NO_MATCH;
    }

    tmp->grabtype = grabtype;
    tmp->type = evtype;

    if (tmp->type && GrabMatchesSecond(tmp, grab, ignore_device))
        return match;

    return NO_MATCH;
}

/**
 * Check an individual grab against an event to determine if a passive grab
 * should be activated.
 *
 * @param device The device of the event to check.
 * @param grab The grab to check.
 * @param event The current device event.
 * @param checkCore Check for core grabs too.
 * @param tempGrab A pre-allocated temporary grab record for matching. This
 *        must have the window and device values filled in.
 *
 * @return Whether the grab matches the event.
 */
static Bool
CheckPassiveGrab(DeviceIntPtr device, GrabPtr grab, InternalEvent *event,
                 Bool checkCore, GrabPtr tempGrab)
{
    DeviceIntPtr gdev;
    XkbSrvInfoPtr xkbi = NULL;
    enum MatchFlags match = 0;
    int emulated_type = 0;

    gdev = grab->modifierDevice;
    if (grab->grabtype == CORE) {
        gdev = GetMaster(device, KEYBOARD_OR_FLOAT);
    }
    else if (grab->grabtype == XI2) {
        /* if the device is an attached slave device, gdev must be the
         * attached master keyboard. Since the slave may have been
         * reattached after the grab, the modifier device may not be the
         * same. */
        if (!IsMaster(grab->device) && !IsFloating(device))
            gdev = GetMaster(device, MASTER_KEYBOARD);
    }

    if (gdev && gdev->key)
        xkbi = gdev->key->xkbInfo;
    tempGrab->modifierDevice = grab->modifierDevice;
    tempGrab->modifiersDetail.exact = xkbi ? xkbi->state.grab_mods : 0;

    /* Check for XI2 and XI grabs first */
    match = MatchForType(grab, tempGrab, XI2, event->any.type);

    if (!match && IsTouchEvent(event) &&
        (event->device_event.flags & TOUCH_POINTER_EMULATED)) {
        emulated_type = TouchGetPointerEventType(event);
        match = MatchForType(grab, tempGrab, XI2, emulated_type);
    }

    if (!match)
        match = MatchForType(grab, tempGrab, XI, event->any.type);

    if (!match && emulated_type)
        match = MatchForType(grab, tempGrab, XI, emulated_type);

    if (!match && checkCore) {
        match = MatchForType(grab, tempGrab, CORE, event->any.type);
        if (!match && emulated_type)
            match = MatchForType(grab, tempGrab, CORE, emulated_type);
    }

    if (!match || (grab->confineTo &&
                   (!grab->confineTo->realized ||
                    !BorderSizeNotEmpty(device, grab->confineTo))))
        return FALSE;

    /* In some cases a passive core grab may exist, but the client
     * already has a core grab on some other device. In this case we
     * must not get the grab, otherwise we may never ungrab the
     * device.
     */

    if (grab->grabtype == CORE) {
        /* A passive grab may have been created for a different device
           than it is assigned to at this point in time.
           Update the grab's device and modifier device to reflect the
           current state.
           Since XGrabDeviceButton requires to specify the
           modifierDevice explicitly, we don't override this choice.
         */
        if (grab->type < GenericEvent) {
            grab->device = device;
            grab->modifierDevice = GetMaster(device, MASTER_KEYBOARD);
        }

        if (CoreGrabInterferes(device, grab))
            return FALSE;
    }

    return TRUE;
}

/**
 * "CheckPassiveGrabsOnWindow" checks to see if the event passed in causes a
 * passive grab set on the window to be activated.
 * If activate is true and a passive grab is found, it will be activated,
 * and the event will be delivered to the client.
 *
 * @param pWin The window that may be subject to a passive grab.
 * @param device Device that caused the event.
 * @param event The current device event.
 * @param checkCore Check for core grabs too.
 * @param activate If a grab is found, activate it and deliver the event.
 */

GrabPtr
CheckPassiveGrabsOnWindow(WindowPtr pWin,
                          DeviceIntPtr device,
                          InternalEvent *event, BOOL checkCore, BOOL activate)
{
    GrabPtr grab = wPassiveGrabs(pWin);
    GrabPtr tempGrab;

    if (!grab)
        return NULL;

    tempGrab = AllocGrab(NULL);
    if (tempGrab == NULL)
        return NULL;

    /* Fill out the grab details, but leave the type for later before
     * comparing */
    switch (event->any.type) {
    case ET_KeyPress:
    case ET_KeyRelease:
        tempGrab->detail.exact = event->device_event.detail.key;
        break;
    case ET_ButtonPress:
    case ET_ButtonRelease:
    case ET_TouchBegin:
    case ET_TouchEnd:
        tempGrab->detail.exact = event->device_event.detail.button;
        break;
    default:
        tempGrab->detail.exact = 0;
        break;
    }
    tempGrab->window = pWin;
    tempGrab->device = device;
    tempGrab->detail.pMask = NULL;
    tempGrab->modifiersDetail.pMask = NULL;
    tempGrab->next = NULL;

    for (; grab; grab = grab->next) {
        if (!CheckPassiveGrab(device, grab, event, checkCore, tempGrab))
            continue;

        if (activate && !ActivatePassiveGrab(device, grab, event, event))
            continue;

        break;
    }

    FreeGrab(tempGrab);
    return grab;
}

/**
 * CheckDeviceGrabs handles both keyboard and pointer events that may cause
 * a passive grab to be activated.
 *
 * If the event is a keyboard event, the ancestors of the focus window are
 * traced down and tried to see if they have any passive grabs to be
 * activated.  If the focus window itself is reached and its descendants
 * contain the pointer, the ancestors of the window that the pointer is in
 * are then traced down starting at the focus window, otherwise no grabs are
 * activated.
 * If the event is a pointer event, the ancestors of the window that the
 * pointer is in are traced down starting at the root until CheckPassiveGrabs
 * causes a passive grab to activate or all the windows are
 * tried. PRH
 *
 * If a grab is activated, the event has been sent to the client already!
 *
 * The event we pass in must always be an XI event. From this, we then emulate
 * the core event and then check for grabs.
 *
 * @param device The device that caused the event.
 * @param xE The event to handle (Device{Button|Key}Press).
 * @param count Number of events in list.
 * @return TRUE if a grab has been activated or false otherwise.
*/

Bool
CheckDeviceGrabs(DeviceIntPtr device, InternalEvent *ievent, WindowPtr ancestor)
{
    int i;
    WindowPtr pWin = NULL;
    FocusClassPtr focus =
        IsPointerEvent(ievent) ? NULL : device->focus;
    BOOL sendCore = (IsMaster(device) && device->coreEvents);
    Bool ret = FALSE;
    DeviceEvent *event = &ievent->device_event;

    if (event->type != ET_ButtonPress && event->type != ET_KeyPress)
        return FALSE;

    if (event->type == ET_ButtonPress && (device->button->buttonsDown != 1))
        return FALSE;

    if (device->deviceGrab.grab)
        return FALSE;

    i = 0;
    if (ancestor) {
        while (i < device->spriteInfo->sprite->spriteTraceGood)
            if (device->spriteInfo->sprite->spriteTrace[i++] == ancestor)
                break;
        if (i == device->spriteInfo->sprite->spriteTraceGood)
            goto out;
    }

    if (focus) {
        for (; i < focus->traceGood; i++) {
            pWin = focus->trace[i];
            if (CheckPassiveGrabsOnWindow(pWin, device, ievent,
                                          sendCore, TRUE)) {
                ret = TRUE;
                goto out;
            }
        }

        if ((focus->win == NoneWin) ||
            (i >= device->spriteInfo->sprite->spriteTraceGood) ||
            (pWin && pWin != device->spriteInfo->sprite->spriteTrace[i - 1]))
            goto out;
    }

    for (; i < device->spriteInfo->sprite->spriteTraceGood; i++) {
        pWin = device->spriteInfo->sprite->spriteTrace[i];
        if (CheckPassiveGrabsOnWindow(pWin, device, ievent,
                                      sendCore, TRUE)) {
            ret = TRUE;
            goto out;
        }
    }

 out:
    if (ret == TRUE && event->type == ET_KeyPress)
        device->deviceGrab.activatingKey = event->detail.key;
    return ret;
}

/**
 * Called for keyboard events to deliver event to whatever client owns the
 * focus.
 *
 * The event is delivered to the keyboard's focus window, the root window or
 * to the window owning the input focus.
 *
 * @param keybd The keyboard originating the event.
 * @param event The event, not yet in wire format.
 * @param window Window underneath the sprite.
 */
void
DeliverFocusedEvent(DeviceIntPtr keybd, InternalEvent *event, WindowPtr window)
{
    DeviceIntPtr ptr;
    WindowPtr focus = keybd->focus->win;
    BOOL sendCore = (IsMaster(keybd) && keybd->coreEvents);
    xEvent *core = NULL, *xE = NULL, *xi2 = NULL;
    int count, rc;
    int deliveries = 0;

    if (focus == FollowKeyboardWin)
        focus = inputInfo.keyboard->focus->win;
    if (!focus)
        return;
    if (focus == PointerRootWin) {
        DeliverDeviceEvents(window, event, NullGrab, NullWindow, keybd);
        return;
    }
    if ((focus == window) || IsParent(focus, window)) {
        if (DeliverDeviceEvents(window, event, NullGrab, focus, keybd))
            return;
    }

    /* just deliver it to the focus window */
    ptr = GetMaster(keybd, POINTER_OR_FLOAT);

    rc = EventToXI2(event, &xi2);
    if (rc == Success) {
        /* XXX: XACE */
        int filter = GetEventFilter(keybd, xi2);

        FixUpEventFromWindow(ptr->spriteInfo->sprite, xi2, focus, None, FALSE);
        deliveries = DeliverEventsToWindow(keybd, focus, xi2, 1,
                                           filter, NullGrab);
        if (deliveries > 0)
            goto unwind;
    }
    else if (rc != BadMatch)
        ErrorF
            ("[dix] %s: XI2 conversion failed in DFE (%d, %d). Skipping delivery.\n",
             keybd->name, event->any.type, rc);

    rc = EventToXI(event, &xE, &count);
    if (rc == Success &&
        XaceHook(XACE_SEND_ACCESS, NULL, keybd, focus, xE, count) == Success) {
        FixUpEventFromWindow(ptr->spriteInfo->sprite, xE, focus, None, FALSE);
        deliveries = DeliverEventsToWindow(keybd, focus, xE, count,
                                           GetEventFilter(keybd, xE), NullGrab);

        if (deliveries > 0)
            goto unwind;
    }
    else if (rc != BadMatch)
        ErrorF
            ("[dix] %s: XI conversion failed in DFE (%d, %d). Skipping delivery.\n",
             keybd->name, event->any.type, rc);

    if (sendCore) {
        rc = EventToCore(event, &core, &count);
        if (rc == Success) {
            if (XaceHook(XACE_SEND_ACCESS, NULL, keybd, focus, core, count) ==
                Success) {
                FixUpEventFromWindow(keybd->spriteInfo->sprite, core, focus,
                                     None, FALSE);
                deliveries =
                    DeliverEventsToWindow(keybd, focus, core, count,
                                          GetEventFilter(keybd, core),
                                          NullGrab);
            }
        }
        else if (rc != BadMatch)
            ErrorF
                ("[dix] %s: core conversion failed DFE (%d, %d). Skipping delivery.\n",
                 keybd->name, event->any.type, rc);
    }

 unwind:
    free(core);
    free(xE);
    free(xi2);
    return;
}

int
DeliverOneGrabbedEvent(InternalEvent *event, DeviceIntPtr dev,
                       enum InputLevel level)
{
    SpritePtr pSprite = dev->spriteInfo->sprite;
    int rc;
    xEvent *xE = NULL;
    int count = 0;
    int deliveries = 0;
    Mask mask;
    GrabInfoPtr grabinfo = &dev->deviceGrab;
    GrabPtr grab = grabinfo->grab;
    Mask filter;

    if (grab->grabtype != level)
        return 0;

    switch (level) {
    case XI2:
        rc = EventToXI2(event, &xE);
        count = 1;
        if (rc == Success) {
            int evtype = xi2_get_type(xE);

            mask = GetXI2MaskByte(grab->xi2mask, dev, evtype);
            filter = GetEventFilter(dev, xE);
        }
        break;
    case XI:
        if (grabinfo->fromPassiveGrab && grabinfo->implicitGrab)
            mask = grab->deviceMask;
        else
            mask = grab->eventMask;
        rc = EventToXI(event, &xE, &count);
        if (rc == Success)
            filter = GetEventFilter(dev, xE);
        break;
    case CORE:
        rc = EventToCore(event, &xE, &count);
        mask = grab->eventMask;
        if (rc == Success)
            filter = GetEventFilter(dev, xE);
        break;
    default:
        BUG_WARN_MSG(1, "Invalid input level %d\n", level);
        return 0;
    }

    if (rc == Success) {
        FixUpEventFromWindow(pSprite, xE, grab->window, None, TRUE);
        if (XaceHook(XACE_SEND_ACCESS, 0, dev,
                     grab->window, xE, count) ||
            XaceHook(XACE_RECEIVE_ACCESS, rClient(grab),
                     grab->window, xE, count))
            deliveries = 1;     /* don't send, but pretend we did */
        else if (level != CORE || !IsInterferingGrab(rClient(grab), dev, xE)) {
            deliveries = TryClientEvents(rClient(grab), dev,
                                         xE, count, mask, filter, grab);
        }
    }
    else
        BUG_WARN_MSG(rc != BadMatch,
                     "%s: conversion to mode %d failed on %d with %d\n",
                     dev->name, level, event->any.type, rc);

    free(xE);
    return deliveries;
}

/**
 * Deliver an event from a device that is currently grabbed. Uses
 * DeliverDeviceEvents() for further delivery if a ownerEvents is set on the
 * grab. If not, TryClientEvents() is used.
 *
 * @param deactivateGrab True if the device's grab should be deactivated.
 *
 * @return The number of events delivered.
 */
int
DeliverGrabbedEvent(InternalEvent *event, DeviceIntPtr thisDev,
                    Bool deactivateGrab)
{
    GrabPtr grab;
    GrabInfoPtr grabinfo;
    int deliveries = 0;
    SpritePtr pSprite = thisDev->spriteInfo->sprite;
    BOOL sendCore = FALSE;

    grabinfo = &thisDev->deviceGrab;
    grab = grabinfo->grab;

    if (grab->ownerEvents) {
        WindowPtr focus;

        /* Hack: Some pointer device have a focus class. So we need to check
         * for the type of event, to see if we really want to deliver it to
         * the focus window. For pointer events, the answer is no.
         */
        if (IsPointerEvent(event))
            focus = PointerRootWin;
        else if (thisDev->focus) {
            focus = thisDev->focus->win;
            if (focus == FollowKeyboardWin)
                focus = inputInfo.keyboard->focus->win;
        }
        else
            focus = PointerRootWin;
        if (focus == PointerRootWin)
            deliveries = DeliverDeviceEvents(pSprite->win, event, grab,
                                             NullWindow, thisDev);
        else if (focus && (focus == pSprite->win ||
                           IsParent(focus, pSprite->win)))
            deliveries = DeliverDeviceEvents(pSprite->win, event, grab, focus,
                                             thisDev);
        else if (focus)
            deliveries = DeliverDeviceEvents(focus, event, grab, focus,
                                             thisDev);
    }
    if (!deliveries) {
        sendCore = (IsMaster(thisDev) && thisDev->coreEvents);
        /* try core event */
        if ((sendCore && grab->grabtype == CORE) || grab->grabtype != CORE)
            deliveries = DeliverOneGrabbedEvent(event, thisDev, grab->grabtype);

        if (deliveries && (event->any.type == ET_Motion))
            thisDev->valuator->motionHintWindow = grab->window;
    }
    if (deliveries && !deactivateGrab &&
        (event->any.type == ET_KeyPress ||
         event->any.type == ET_KeyRelease ||
         event->any.type == ET_ButtonPress ||
         event->any.type == ET_ButtonRelease)) {
        FreezeThisEventIfNeededForSyncGrab(thisDev, event);
    }

    return deliveries;
}

void
FreezeThisEventIfNeededForSyncGrab(DeviceIntPtr thisDev, InternalEvent *event)
{
    GrabInfoPtr grabinfo = &thisDev->deviceGrab;
    GrabPtr grab = grabinfo->grab;
    DeviceIntPtr dev;

    switch (grabinfo->sync.state) {
    case FREEZE_BOTH_NEXT_EVENT:
        dev = GetPairedDevice(thisDev);
        if (dev) {
            FreezeThaw(dev, TRUE);
            if ((dev->deviceGrab.sync.state == FREEZE_BOTH_NEXT_EVENT) &&
                (CLIENT_BITS(grab->resource) ==
                 CLIENT_BITS(dev->deviceGrab.grab->resource)))
                dev->deviceGrab.sync.state = FROZEN_NO_EVENT;
            else
                dev->deviceGrab.sync.other = grab;
        }
        /* fall through */
    case FREEZE_NEXT_EVENT:
        grabinfo->sync.state = FROZEN_WITH_EVENT;
        FreezeThaw(thisDev, TRUE);
        CopyPartialInternalEvent(grabinfo->sync.event, event);
        break;
    }
}

/* This function is used to set the key pressed or key released state -
   this is only used when the pressing of keys does not cause
   the device's processInputProc to be called, as in for example Mouse Keys.
*/
void
FixKeyState(DeviceEvent *event, DeviceIntPtr keybd)
{
    int key = event->detail.key;

    if (event->type == ET_KeyPress) {
        DebugF("FixKeyState: Key %d %s\n", key,
               ((event->type == ET_KeyPress) ? "down" : "up"));
    }

    if (event->type == ET_KeyPress)
        set_key_down(keybd, key, KEY_PROCESSED);
    else if (event->type == ET_KeyRelease)
        set_key_up(keybd, key, KEY_PROCESSED);
    else
        FatalError("Impossible keyboard event");
}

#define AtMostOneClient \
	(SubstructureRedirectMask | ResizeRedirectMask | ButtonPressMask)
#define ManagerMask \
	(SubstructureRedirectMask | ResizeRedirectMask)

/**
 * Recalculate which events may be deliverable for the given window.
 * Recalculated mask is used for quicker determination which events may be
 * delivered to a window.
 *
 * The otherEventMasks on a WindowOptional is the combination of all event
 * masks set by all clients on the window.
 * deliverableEventMask is the combination of the eventMask and the
 * otherEventMask plus the events that may be propagated to the parent.
 *
 * Traverses to siblings and parents of the window.
 */
void
RecalculateDeliverableEvents(WindowPtr pWin)
{
    OtherClients *others;
    WindowPtr pChild;

    pChild = pWin;
    while (1) {
        if (pChild->optional) {
            pChild->optional->otherEventMasks = 0;
            for (others = wOtherClients(pChild); others; others = others->next) {
                pChild->optional->otherEventMasks |= others->mask;
            }
        }
        pChild->deliverableEvents = pChild->eventMask |
            wOtherEventMasks(pChild);
        if (pChild->parent)
            pChild->deliverableEvents |=
                (pChild->parent->deliverableEvents &
                 ~wDontPropagateMask(pChild) & PropagateMask);
        if (pChild->firstChild) {
            pChild = pChild->firstChild;
            continue;
        }
        while (!pChild->nextSib && (pChild != pWin))
            pChild = pChild->parent;
        if (pChild == pWin)
            break;
        pChild = pChild->nextSib;
    }
}

/**
 *
 *  \param value must conform to DeleteType
 */
int
OtherClientGone(void *value, XID id)
{
    OtherClientsPtr other, prev;
    WindowPtr pWin = (WindowPtr) value;

    prev = 0;
    for (other = wOtherClients(pWin); other; other = other->next) {
        if (other->resource == id) {
            if (prev)
                prev->next = other->next;
            else {
                if (!(pWin->optional->otherClients = other->next))
                    CheckWindowOptionalNeed(pWin);
            }
            free(other);
            RecalculateDeliverableEvents(pWin);
            return Success;
        }
        prev = other;
    }
    FatalError("client not on event list");
}

int
EventSelectForWindow(WindowPtr pWin, ClientPtr client, Mask mask)
{
    Mask check;
    OtherClients *others;
    DeviceIntPtr dev;
    int rc;

    if (mask & ~AllEventMasks) {
        client->errorValue = mask;
        return BadValue;
    }
    check = (mask & ManagerMask);
    if (check) {
        rc = XaceHook(XACE_RESOURCE_ACCESS, client, pWin->drawable.id,
                      RT_WINDOW, pWin, RT_NONE, NULL, DixManageAccess);
        if (rc != Success)
            return rc;
    }
    check = (mask & AtMostOneClient);
    if (check & (pWin->eventMask | wOtherEventMasks(pWin))) {
        /* It is illegal for two different clients to select on any of the
           events for AtMostOneClient. However, it is OK, for some client to
           continue selecting on one of those events.  */
        if ((wClient(pWin) != client) && (check & pWin->eventMask))
            return BadAccess;
        for (others = wOtherClients(pWin); others; others = others->next) {
            if (!SameClient(others, client) && (check & others->mask))
                return BadAccess;
        }
    }
    if (wClient(pWin) == client) {
        check = pWin->eventMask;
        pWin->eventMask = mask;
    }
    else {
        for (others = wOtherClients(pWin); others; others = others->next) {
            if (SameClient(others, client)) {
                check = others->mask;
                if (mask == 0) {
                    FreeResource(others->resource, RT_NONE);
                    return Success;
                }
                else
                    others->mask = mask;
                goto maskSet;
            }
        }
        check = 0;
        if (!pWin->optional && !MakeWindowOptional(pWin))
            return BadAlloc;
        others = malloc(sizeof(OtherClients));
        if (!others)
            return BadAlloc;
        others->mask = mask;
        others->resource = FakeClientID(client->index);
        others->next = pWin->optional->otherClients;
        pWin->optional->otherClients = others;
        if (!AddResource(others->resource, RT_OTHERCLIENT, (void *) pWin))
            return BadAlloc;
    }
 maskSet:
    if ((mask & PointerMotionHintMask) && !(check & PointerMotionHintMask)) {
        for (dev = inputInfo.devices; dev; dev = dev->next) {
            if (dev->valuator && dev->valuator->motionHintWindow == pWin)
                dev->valuator->motionHintWindow = NullWindow;
        }
    }
    RecalculateDeliverableEvents(pWin);
    return Success;
}

int
EventSuppressForWindow(WindowPtr pWin, ClientPtr client,
                       Mask mask, Bool *checkOptional)
{
    int i, freed;

    if (mask & ~PropagateMask) {
        client->errorValue = mask;
        return BadValue;
    }
    if (pWin->dontPropagate)
        DontPropagateRefCnts[pWin->dontPropagate]--;
    if (!mask)
        i = 0;
    else {
        for (i = DNPMCOUNT, freed = 0; --i > 0;) {
            if (!DontPropagateRefCnts[i])
                freed = i;
            else if (mask == DontPropagateMasks[i])
                break;
        }
        if (!i && freed) {
            i = freed;
            DontPropagateMasks[i] = mask;
        }
    }
    if (i || !mask) {
        pWin->dontPropagate = i;
        if (i)
            DontPropagateRefCnts[i]++;
        if (pWin->optional) {
            pWin->optional->dontPropagateMask = mask;
            *checkOptional = TRUE;
        }
    }
    else {
        if (!pWin->optional && !MakeWindowOptional(pWin)) {
            if (pWin->dontPropagate)
                DontPropagateRefCnts[pWin->dontPropagate]++;
            return BadAlloc;
        }
        pWin->dontPropagate = 0;
        pWin->optional->dontPropagateMask = mask;
    }
    RecalculateDeliverableEvents(pWin);
    return Success;
}

/**
 * Assembles an EnterNotify or LeaveNotify and sends it event to the client.
 * Uses the paired keyboard to get some additional information.
 */
void
CoreEnterLeaveEvent(DeviceIntPtr mouse,
                    int type,
                    int mode, int detail, WindowPtr pWin, Window child)
{
    xEvent event = {
        .u.u.type = type,
        .u.u.detail = detail
    };
    WindowPtr focus;
    DeviceIntPtr keybd;
    GrabPtr grab = mouse->deviceGrab.grab;
    Mask mask;

    keybd = GetMaster(mouse, KEYBOARD_OR_FLOAT);

    if ((pWin == mouse->valuator->motionHintWindow) &&
        (detail != NotifyInferior))
        mouse->valuator->motionHintWindow = NullWindow;
    if (grab) {
        mask = (pWin == grab->window) ? grab->eventMask : 0;
        if (grab->ownerEvents)
            mask |= EventMaskForClient(pWin, rClient(grab));
    }
    else {
        mask = pWin->eventMask | wOtherEventMasks(pWin);
    }

    event.u.enterLeave.time = currentTime.milliseconds;
    event.u.enterLeave.rootX = mouse->spriteInfo->sprite->hot.x;
    event.u.enterLeave.rootY = mouse->spriteInfo->sprite->hot.y;
    /* Counts on the same initial structure of crossing & button events! */
    FixUpEventFromWindow(mouse->spriteInfo->sprite, &event, pWin, None, FALSE);
    /* Enter/Leave events always set child */
    event.u.enterLeave.child = child;
    event.u.enterLeave.flags = event.u.keyButtonPointer.sameScreen ?
        ELFlagSameScreen : 0;
    event.u.enterLeave.state =
        mouse->button ? (mouse->button->state & 0x1f00) : 0;
    if (keybd)
        event.u.enterLeave.state |=
            XkbGrabStateFromRec(&keybd->key->xkbInfo->state);
    event.u.enterLeave.mode = mode;
    focus = (keybd) ? keybd->focus->win : None;
    if ((focus != NoneWin) &&
        ((pWin == focus) || (focus == PointerRootWin) || IsParent(focus, pWin)))
        event.u.enterLeave.flags |= ELFlagFocus;

    if ((mask & GetEventFilter(mouse, &event))) {
        if (grab)
            TryClientEvents(rClient(grab), mouse, &event, 1, mask,
                            GetEventFilter(mouse, &event), grab);
        else
            DeliverEventsToWindow(mouse, pWin, &event, 1,
                                  GetEventFilter(mouse, &event), NullGrab);
    }

    if ((type == EnterNotify) && (mask & KeymapStateMask)) {
        xKeymapEvent ke = {
            .type = KeymapNotify
        };
        ClientPtr client = grab ? rClient(grab) : wClient(pWin);
        int rc;

        rc = XaceHook(XACE_DEVICE_ACCESS, client, keybd, DixReadAccess);
        if (rc == Success)
            memcpy((char *) &ke.map[0], (char *) &keybd->key->down[1], 31);

        if (grab)
            TryClientEvents(rClient(grab), keybd, (xEvent *) &ke, 1,
                            mask, KeymapStateMask, grab);
        else
            DeliverEventsToWindow(mouse, pWin, (xEvent *) &ke, 1,
                                  KeymapStateMask, NullGrab);
    }
}

void
DeviceEnterLeaveEvent(DeviceIntPtr mouse,
                      int sourceid,
                      int type,
                      int mode, int detail, WindowPtr pWin, Window child)
{
    GrabPtr grab = mouse->deviceGrab.grab;
    xXIEnterEvent *event;
    WindowPtr focus;
    int filter;
    int btlen, len, i;
    DeviceIntPtr kbd;

    if ((mode == XINotifyPassiveGrab && type == XI_Leave) ||
        (mode == XINotifyPassiveUngrab && type == XI_Enter))
        return;

    btlen = (mouse->button) ? bits_to_bytes(mouse->button->numButtons) : 0;
    btlen = bytes_to_int32(btlen);
    len = sizeof(xXIEnterEvent) + btlen * 4;

    event = calloc(1, len);
    event->type = GenericEvent;
    event->extension = IReqCode;
    event->evtype = type;
    event->length = (len - sizeof(xEvent)) / 4;
    event->buttons_len = btlen;
    event->detail = detail;
    event->time = currentTime.milliseconds;
    event->deviceid = mouse->id;
    event->sourceid = sourceid;
    event->mode = mode;
    event->root_x = double_to_fp1616(mouse->spriteInfo->sprite->hot.x);
    event->root_y = double_to_fp1616(mouse->spriteInfo->sprite->hot.y);

    for (i = 0; mouse && mouse->button && i < mouse->button->numButtons; i++)
        if (BitIsOn(mouse->button->down, i))
            SetBit(&event[1], i);

    kbd = GetMaster(mouse, MASTER_KEYBOARD);
    if (kbd && kbd->key) {
        event->mods.base_mods = kbd->key->xkbInfo->state.base_mods;
        event->mods.latched_mods = kbd->key->xkbInfo->state.latched_mods;
        event->mods.locked_mods = kbd->key->xkbInfo->state.locked_mods;

        event->group.base_group = kbd->key->xkbInfo->state.base_group;
        event->group.latched_group = kbd->key->xkbInfo->state.latched_group;
        event->group.locked_group = kbd->key->xkbInfo->state.locked_group;
    }

    focus = (kbd) ? kbd->focus->win : None;
    if ((focus != NoneWin) &&
        ((pWin == focus) || (focus == PointerRootWin) || IsParent(focus, pWin)))
        event->focus = TRUE;

    FixUpEventFromWindow(mouse->spriteInfo->sprite, (xEvent *) event, pWin,
                         None, FALSE);

    filter = GetEventFilter(mouse, (xEvent *) event);

    if (grab && grab->grabtype == XI2) {
        Mask mask;

        mask = xi2mask_isset(grab->xi2mask, mouse, type);
        TryClientEvents(rClient(grab), mouse, (xEvent *) event, 1, mask, 1,
                        grab);
    }
    else {
        if (!WindowXI2MaskIsset(mouse, pWin, (xEvent *) event))
            goto out;
        DeliverEventsToWindow(mouse, pWin, (xEvent *) event, 1, filter,
                              NullGrab);
    }

 out:
    free(event);
}

void
CoreFocusEvent(DeviceIntPtr dev, int type, int mode, int detail, WindowPtr pWin)
{
    xEvent event = {
        .u.u.type = type,
        .u.u.detail = detail
    };
    event.u.focus.mode = mode;
    event.u.focus.window = pWin->drawable.id;

    DeliverEventsToWindow(dev, pWin, &event, 1,
                          GetEventFilter(dev, &event), NullGrab);
    if ((type == FocusIn) &&
        ((pWin->eventMask | wOtherEventMasks(pWin)) & KeymapStateMask)) {
        xKeymapEvent ke = {
            .type = KeymapNotify
        };
        ClientPtr client = wClient(pWin);
        int rc;

        rc = XaceHook(XACE_DEVICE_ACCESS, client, dev, DixReadAccess);
        if (rc == Success)
            memcpy((char *) &ke.map[0], (char *) &dev->key->down[1], 31);

        DeliverEventsToWindow(dev, pWin, (xEvent *) &ke, 1,
                              KeymapStateMask, NullGrab);
    }
}

/**
 * Set the input focus to the given window. Subsequent keyboard events will be
 * delivered to the given window.
 *
 * Usually called from ProcSetInputFocus as result of a client request. If so,
 * the device is the inputInfo.keyboard.
 * If called from ProcXSetInputFocus as result of a client xinput request, the
 * device is set to the device specified by the client.
 *
 * @param client Client that requested input focus change.
 * @param dev Focus device.
 * @param focusID The window to obtain the focus. Can be PointerRoot or None.
 * @param revertTo Specifies where the focus reverts to when window becomes
 * unviewable.
 * @param ctime Specifies the time.
 * @param followOK True if pointer is allowed to follow the keyboard.
 */
int
SetInputFocus(ClientPtr client,
              DeviceIntPtr dev,
              Window focusID, CARD8 revertTo, Time ctime, Bool followOK)
{
    FocusClassPtr focus;
    WindowPtr focusWin;
    int mode, rc;
    TimeStamp time;
    DeviceIntPtr keybd;         /* used for FollowKeyboard or FollowKeyboardWin */

    UpdateCurrentTime();
    if ((revertTo != RevertToParent) &&
        (revertTo != RevertToPointerRoot) &&
        (revertTo != RevertToNone) &&
        ((revertTo != RevertToFollowKeyboard) || !followOK)) {
        client->errorValue = revertTo;
        return BadValue;
    }
    time = ClientTimeToServerTime(ctime);

    keybd = GetMaster(dev, KEYBOARD_OR_FLOAT);

    if ((focusID == None) || (focusID == PointerRoot))
        focusWin = (WindowPtr) (long) focusID;
    else if ((focusID == FollowKeyboard) && followOK) {
        focusWin = keybd->focus->win;
    }
    else {
        rc = dixLookupWindow(&focusWin, focusID, client, DixSetAttrAccess);
        if (rc != Success)
            return rc;
        /* It is a match error to try to set the input focus to an
           unviewable window. */
        if (!focusWin->realized)
            return BadMatch;
    }
    rc = XaceHook(XACE_DEVICE_ACCESS, client, dev, DixSetFocusAccess);
    if (rc != Success)
        return Success;

    focus = dev->focus;
    if ((CompareTimeStamps(time, currentTime) == LATER) ||
        (CompareTimeStamps(time, focus->time) == EARLIER))
        return Success;
    mode = (dev->deviceGrab.grab) ? NotifyWhileGrabbed : NotifyNormal;
    if (focus->win == FollowKeyboardWin) {
        if (!ActivateFocusInGrab(dev, keybd->focus->win, focusWin))
            DoFocusEvents(dev, keybd->focus->win, focusWin, mode);
    }
    else {
        if (!ActivateFocusInGrab(dev, focus->win, focusWin))
            DoFocusEvents(dev, focus->win, focusWin, mode);
    }
    focus->time = time;
    focus->revert = revertTo;
    if (focusID == FollowKeyboard)
        focus->win = FollowKeyboardWin;
    else
        focus->win = focusWin;
    if ((focusWin == NoneWin) || (focusWin == PointerRootWin))
        focus->traceGood = 0;
    else {
        int depth = 0;
        WindowPtr pWin;

        for (pWin = focusWin; pWin; pWin = pWin->parent)
            depth++;
        if (depth > focus->traceSize) {
            focus->traceSize = depth + 1;
            focus->trace = reallocarray(focus->trace, focus->traceSize,
                                        sizeof(WindowPtr));
        }
        focus->traceGood = depth;
        for (pWin = focusWin, depth--; pWin; pWin = pWin->parent, depth--)
            focus->trace[depth] = pWin;
    }
    return Success;
}

/**
 * Server-side protocol handling for SetInputFocus request.
 *
 * Sets the input focus for the virtual core keyboard.
 */
int
ProcSetInputFocus(ClientPtr client)
{
    DeviceIntPtr kbd = PickKeyboard(client);

    REQUEST(xSetInputFocusReq);

    REQUEST_SIZE_MATCH(xSetInputFocusReq);

    return SetInputFocus(client, kbd, stuff->focus,
                         stuff->revertTo, stuff->time, FALSE);
}

/**
 * Server-side protocol handling for GetInputFocus request.
 *
 * Sends the current input focus for the client's keyboard back to the
 * client.
 */
int
ProcGetInputFocus(ClientPtr client)
{
    DeviceIntPtr kbd = PickKeyboard(client);
    xGetInputFocusReply rep;
    FocusClassPtr focus = kbd->focus;
    int rc;

    /* REQUEST(xReq); */
    REQUEST_SIZE_MATCH(xReq);

    rc = XaceHook(XACE_DEVICE_ACCESS, client, kbd, DixGetFocusAccess);
    if (rc != Success)
        return rc;

    rep = (xGetInputFocusReply) {
        .type = X_Reply,
        .length = 0,
        .sequenceNumber = client->sequence,
        .revertTo = focus->revert
    };

    if (focus->win == NoneWin)
        rep.focus = None;
    else if (focus->win == PointerRootWin)
        rep.focus = PointerRoot;
    else
        rep.focus = focus->win->drawable.id;

    WriteReplyToClient(client, sizeof(xGetInputFocusReply), &rep);
    return Success;
}

/**
 * Server-side protocol handling for GrabPointer request.
 *
 * Sets an active grab on the client's ClientPointer and returns success
 * status to client.
 */
int
ProcGrabPointer(ClientPtr client)
{
    xGrabPointerReply rep;
    DeviceIntPtr device = PickPointer(client);
    GrabPtr grab;
    GrabMask mask;
    WindowPtr confineTo;
    BYTE status;

    REQUEST(xGrabPointerReq);
    int rc;

    REQUEST_SIZE_MATCH(xGrabPointerReq);
    UpdateCurrentTime();

    if (stuff->eventMask & ~PointerGrabMask) {
        client->errorValue = stuff->eventMask;
        return BadValue;
    }

    if (stuff->confineTo == None)
        confineTo = NullWindow;
    else {
        rc = dixLookupWindow(&confineTo, stuff->confineTo, client,
                             DixSetAttrAccess);
        if (rc != Success)
            return rc;
    }

    grab = device->deviceGrab.grab;

    if (grab && grab->confineTo && !confineTo)
        ConfineCursorToWindow(device, GetCurrentRootWindow(device), FALSE, FALSE);

    mask.core = stuff->eventMask;

    rc = GrabDevice(client, device, stuff->pointerMode, stuff->keyboardMode,
                    stuff->grabWindow, stuff->ownerEvents, stuff->time,
                    &mask, CORE, stuff->cursor, stuff->confineTo, &status);
    if (rc != Success)
        return rc;

    rep = (xGrabPointerReply) {
        .type = X_Reply,
        .status = status,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    WriteReplyToClient(client, sizeof(xGrabPointerReply), &rep);
    return Success;
}

/**
 * Server-side protocol handling for ChangeActivePointerGrab request.
 *
 * Changes properties of the grab hold by the client. If the client does not
 * hold an active grab on the device, nothing happens.
 */
int
ProcChangeActivePointerGrab(ClientPtr client)
{
    DeviceIntPtr device;
    GrabPtr grab;
    CursorPtr newCursor, oldCursor;

    REQUEST(xChangeActivePointerGrabReq);
    TimeStamp time;

    REQUEST_SIZE_MATCH(xChangeActivePointerGrabReq);
    if (stuff->eventMask & ~PointerGrabMask) {
        client->errorValue = stuff->eventMask;
        return BadValue;
    }
    if (stuff->cursor == None)
        newCursor = NullCursor;
    else {
        int rc = dixLookupResourceByType((void **) &newCursor, stuff->cursor,
                                         RT_CURSOR, client, DixUseAccess);

        if (rc != Success) {
            client->errorValue = stuff->cursor;
            return rc;
        }
    }

    device = PickPointer(client);
    grab = device->deviceGrab.grab;

    if (!grab)
        return Success;
    if (!SameClient(grab, client))
        return Success;
    UpdateCurrentTime();
    time = ClientTimeToServerTime(stuff->time);
    if ((CompareTimeStamps(time, currentTime) == LATER) ||
        (CompareTimeStamps(time, device->deviceGrab.grabTime) == EARLIER))
        return Success;
    oldCursor = grab->cursor;
    grab->cursor = RefCursor(newCursor);
    PostNewCursor(device);
    if (oldCursor)
        FreeCursor(oldCursor, (Cursor) 0);
    grab->eventMask = stuff->eventMask;
    return Success;
}

/**
 * Server-side protocol handling for UngrabPointer request.
 *
 * Deletes a pointer grab on a device the client has grabbed.
 */
int
ProcUngrabPointer(ClientPtr client)
{
    DeviceIntPtr device = PickPointer(client);
    GrabPtr grab;
    TimeStamp time;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    UpdateCurrentTime();
    grab = device->deviceGrab.grab;

    time = ClientTimeToServerTime(stuff->id);
    if ((CompareTimeStamps(time, currentTime) != LATER) &&
        (CompareTimeStamps(time, device->deviceGrab.grabTime) != EARLIER) &&
        (grab) && SameClient(grab, client))
        (*device->deviceGrab.DeactivateGrab) (device);
    return Success;
}

/**
 * Sets a grab on the given device.
 *
 * Called from ProcGrabKeyboard to work on the client's keyboard.
 * Called from ProcXGrabDevice to work on the device specified by the client.
 *
 * The parameters this_mode and other_mode represent the keyboard_mode and
 * pointer_mode parameters of XGrabKeyboard().
 * See man page for details on all the parameters
 *
 * @param client Client that owns the grab.
 * @param dev The device to grab.
 * @param this_mode GrabModeSync or GrabModeAsync
 * @param other_mode GrabModeSync or GrabModeAsync
 * @param status Return code to be returned to the caller.
 *
 * @returns Success or BadValue or BadAlloc.
 */
int
GrabDevice(ClientPtr client, DeviceIntPtr dev,
           unsigned pointer_mode, unsigned keyboard_mode, Window grabWindow,
           unsigned ownerEvents, Time ctime, GrabMask *mask,
           int grabtype, Cursor curs, Window confineToWin, CARD8 *status)
{
    WindowPtr pWin, confineTo;
    GrabPtr grab;
    TimeStamp time;
    Mask access_mode = DixGrabAccess;
    int rc;
    GrabInfoPtr grabInfo = &dev->deviceGrab;
    CursorPtr cursor;

    UpdateCurrentTime();
    if ((keyboard_mode != GrabModeSync) && (keyboard_mode != GrabModeAsync)) {
        client->errorValue = keyboard_mode;
        return BadValue;
    }
    if ((pointer_mode != GrabModeSync) && (pointer_mode != GrabModeAsync)) {
        client->errorValue = pointer_mode;
        return BadValue;
    }
    if ((ownerEvents != xFalse) && (ownerEvents != xTrue)) {
        client->errorValue = ownerEvents;
        return BadValue;
    }

    rc = dixLookupWindow(&pWin, grabWindow, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    if (confineToWin == None)
        confineTo = NullWindow;
    else {
        rc = dixLookupWindow(&confineTo, confineToWin, client,
                             DixSetAttrAccess);
        if (rc != Success)
            return rc;
    }

    if (curs == None)
        cursor = NullCursor;
    else {
        rc = dixLookupResourceByType((void **) &cursor, curs, RT_CURSOR,
                                     client, DixUseAccess);
        if (rc != Success) {
            client->errorValue = curs;
            return rc;
        }
        access_mode |= DixForceAccess;
    }

    if (keyboard_mode == GrabModeSync || pointer_mode == GrabModeSync)
        access_mode |= DixFreezeAccess;
    rc = XaceHook(XACE_DEVICE_ACCESS, client, dev, access_mode);
    if (rc != Success)
        return rc;

    time = ClientTimeToServerTime(ctime);
    grab = grabInfo->grab;
    if (grab && grab->grabtype != grabtype)
        *status = AlreadyGrabbed;
    else if (grab && !SameClient(grab, client))
        *status = AlreadyGrabbed;
    else if ((!pWin->realized) ||
             (confineTo &&
              !(confineTo->realized && BorderSizeNotEmpty(dev, confineTo))))
        *status = GrabNotViewable;
    else if ((CompareTimeStamps(time, currentTime) == LATER) ||
             (CompareTimeStamps(time, grabInfo->grabTime) == EARLIER))
        *status = GrabInvalidTime;
    else if (grabInfo->sync.frozen &&
             grabInfo->sync.other && !SameClient(grabInfo->sync.other, client))
        *status = GrabFrozen;
    else {
        GrabPtr tempGrab;

        tempGrab = AllocGrab(NULL);
        if (tempGrab == NULL)
            return BadAlloc;

        tempGrab->next = NULL;
        tempGrab->window = pWin;
        tempGrab->resource = client->clientAsMask;
        tempGrab->ownerEvents = ownerEvents;
        tempGrab->keyboardMode = keyboard_mode;
        tempGrab->pointerMode = pointer_mode;
        if (grabtype == CORE)
            tempGrab->eventMask = mask->core;
        else if (grabtype == XI)
            tempGrab->eventMask = mask->xi;
        else
            xi2mask_merge(tempGrab->xi2mask, mask->xi2mask);
        tempGrab->device = dev;
        tempGrab->cursor = RefCursor(cursor);
        tempGrab->confineTo = confineTo;
        tempGrab->grabtype = grabtype;
        (*grabInfo->ActivateGrab) (dev, tempGrab, time, FALSE);
        *status = GrabSuccess;

        FreeGrab(tempGrab);
    }
    return Success;
}

/**
 * Server-side protocol handling for GrabKeyboard request.
 *
 * Grabs the client's keyboard and returns success status to client.
 */
int
ProcGrabKeyboard(ClientPtr client)
{
    xGrabKeyboardReply rep;
    BYTE status;

    REQUEST(xGrabKeyboardReq);
    int result;
    DeviceIntPtr keyboard = PickKeyboard(client);
    GrabMask mask;

    REQUEST_SIZE_MATCH(xGrabKeyboardReq);
    UpdateCurrentTime();

    mask.core = KeyPressMask | KeyReleaseMask;

    result = GrabDevice(client, keyboard, stuff->pointerMode,
                        stuff->keyboardMode, stuff->grabWindow,
                        stuff->ownerEvents, stuff->time, &mask, CORE, None,
                        None, &status);

    if (result != Success)
        return result;

    rep = (xGrabKeyboardReply) {
        .type = X_Reply,
        .status = status,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    WriteReplyToClient(client, sizeof(xGrabKeyboardReply), &rep);
    return Success;
}

/**
 * Server-side protocol handling for UngrabKeyboard request.
 *
 * Deletes a possible grab on the client's keyboard.
 */
int
ProcUngrabKeyboard(ClientPtr client)
{
    DeviceIntPtr device = PickKeyboard(client);
    GrabPtr grab;
    TimeStamp time;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    UpdateCurrentTime();

    grab = device->deviceGrab.grab;

    time = ClientTimeToServerTime(stuff->id);
    if ((CompareTimeStamps(time, currentTime) != LATER) &&
        (CompareTimeStamps(time, device->deviceGrab.grabTime) != EARLIER) &&
        (grab) && SameClient(grab, client) && grab->grabtype == CORE)
        (*device->deviceGrab.DeactivateGrab) (device);
    return Success;
}

/**
 * Server-side protocol handling for QueryPointer request.
 *
 * Returns the current state and position of the client's ClientPointer to the
 * client.
 */
int
ProcQueryPointer(ClientPtr client)
{
    xQueryPointerReply rep;
    WindowPtr pWin, t;
    DeviceIntPtr mouse = PickPointer(client);
    DeviceIntPtr keyboard;
    SpritePtr pSprite;
    int rc;

    REQUEST(xResourceReq);
    REQUEST_SIZE_MATCH(xResourceReq);

    rc = dixLookupWindow(&pWin, stuff->id, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    rc = XaceHook(XACE_DEVICE_ACCESS, client, mouse, DixReadAccess);
    if (rc != Success && rc != BadAccess)
        return rc;

    keyboard = GetMaster(mouse, MASTER_KEYBOARD);

    pSprite = mouse->spriteInfo->sprite;
    if (mouse->valuator->motionHintWindow)
        MaybeStopHint(mouse, client);
    rep = (xQueryPointerReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .mask = event_get_corestate(mouse, keyboard),
        .root = (GetCurrentRootWindow(mouse))->drawable.id,
        .rootX = pSprite->hot.x,
        .rootY = pSprite->hot.y,
        .child = None
    };
    if (pSprite->hot.pScreen == pWin->drawable.pScreen) {
        rep.sameScreen = xTrue;
        rep.winX = pSprite->hot.x - pWin->drawable.x;
        rep.winY = pSprite->hot.y - pWin->drawable.y;
        for (t = pSprite->win; t; t = t->parent)
            if (t->parent == pWin) {
                rep.child = t->drawable.id;
                break;
            }
    }
    else {
        rep.sameScreen = xFalse;
        rep.winX = 0;
        rep.winY = 0;
    }

#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
        rep.rootX += screenInfo.screens[0]->x;
        rep.rootY += screenInfo.screens[0]->y;
        if (stuff->id == rep.root) {
            rep.winX += screenInfo.screens[0]->x;
            rep.winY += screenInfo.screens[0]->y;
        }
    }
#endif

    if (rc == BadAccess) {
        rep.mask = 0;
        rep.child = None;
        rep.rootX = 0;
        rep.rootY = 0;
        rep.winX = 0;
        rep.winY = 0;
    }

    WriteReplyToClient(client, sizeof(xQueryPointerReply), &rep);

    return Success;
}

/**
 * Initializes the device list and the DIX sprite to sane values. Allocates
 * trace memory used for quick window traversal.
 */
void
InitEvents(void)
{
    int i;
    QdEventPtr qe, tmp;

    inputInfo.numDevices = 0;
    inputInfo.devices = (DeviceIntPtr) NULL;
    inputInfo.off_devices = (DeviceIntPtr) NULL;
    inputInfo.keyboard = (DeviceIntPtr) NULL;
    inputInfo.pointer = (DeviceIntPtr) NULL;

    for (i = 0; i < MAXDEVICES; i++) {
        DeviceIntRec dummy;
        memcpy(&event_filters[i], default_filter, sizeof(default_filter));

        dummy.id = i;
        NoticeTime(&dummy, currentTime);
        LastEventTimeToggleResetFlag(i, FALSE);
    }

    syncEvents.replayDev = (DeviceIntPtr) NULL;
    syncEvents.replayWin = NullWindow;
    if (syncEvents.pending.next)
        xorg_list_for_each_entry_safe(qe, tmp, &syncEvents.pending, next)
            free(qe);
    xorg_list_init(&syncEvents.pending);
    syncEvents.playingEvents = FALSE;
    syncEvents.time.months = 0;
    syncEvents.time.milliseconds = 0;   /* hardly matters */
    currentTime.months = 0;
    currentTime.milliseconds = GetTimeInMillis();
    for (i = 0; i < DNPMCOUNT; i++) {
        DontPropagateMasks[i] = 0;
        DontPropagateRefCnts[i] = 0;
    }

    InputEventList = InitEventList(GetMaximumEventsNum());
    if (!InputEventList)
        FatalError("[dix] Failed to allocate input event list.\n");
}

void
CloseDownEvents(void)
{
    FreeEventList(InputEventList, GetMaximumEventsNum());
    InputEventList = NULL;
}

#define SEND_EVENT_BIT 0x80

/**
 * Server-side protocol handling for SendEvent request.
 *
 * Locates the window to send the event to and forwards the event.
 */
int
ProcSendEvent(ClientPtr client)
{
    WindowPtr pWin;
    WindowPtr effectiveFocus = NullWindow;      /* only set if dest==InputFocus */
    DeviceIntPtr dev = PickPointer(client);
    DeviceIntPtr keybd = GetMaster(dev, MASTER_KEYBOARD);
    SpritePtr pSprite = dev->spriteInfo->sprite;

    REQUEST(xSendEventReq);

    REQUEST_SIZE_MATCH(xSendEventReq);

    /* libXext and other extension libraries may set the bit indicating
     * that this event came from a SendEvent request so remove it
     * since otherwise the event type may fail the range checks
     * and cause an invalid BadValue error to be returned.
     *
     * This is safe to do since we later add the SendEvent bit (0x80)
     * back in once we send the event to the client */

    stuff->event.u.u.type &= ~(SEND_EVENT_BIT);

    /* The client's event type must be a core event type or one defined by an
       extension. */

    if (!((stuff->event.u.u.type > X_Reply &&
           stuff->event.u.u.type < LASTEvent) ||
          (stuff->event.u.u.type >= EXTENSION_EVENT_BASE &&
           stuff->event.u.u.type < (unsigned) lastEvent))) {
        client->errorValue = stuff->event.u.u.type;
        return BadValue;
    }
    /* Generic events can have variable size, but SendEvent request holds
       exactly 32B of event data. */
    if (stuff->event.u.u.type == GenericEvent) {
        client->errorValue = stuff->event.u.u.type;
        return BadValue;
    }
    if (stuff->event.u.u.type == ClientMessage &&
        stuff->event.u.u.detail != 8 &&
        stuff->event.u.u.detail != 16 && stuff->event.u.u.detail != 32) {
        client->errorValue = stuff->event.u.u.detail;
        return BadValue;
    }
    if (stuff->eventMask & ~AllEventMasks) {
        client->errorValue = stuff->eventMask;
        return BadValue;
    }

    if (stuff->destination == PointerWindow)
        pWin = pSprite->win;
    else if (stuff->destination == InputFocus) {
        WindowPtr inputFocus = (keybd) ? keybd->focus->win : NoneWin;

        if (inputFocus == NoneWin)
            return Success;

        /* If the input focus is PointerRootWin, send the event to where
           the pointer is if possible, then perhaps propagate up to root. */
        if (inputFocus == PointerRootWin)
            inputFocus = GetCurrentRootWindow(dev);

        if (IsParent(inputFocus, pSprite->win)) {
            effectiveFocus = inputFocus;
            pWin = pSprite->win;
        }
        else
            effectiveFocus = pWin = inputFocus;
    }
    else
        dixLookupWindow(&pWin, stuff->destination, client, DixSendAccess);

    if (!pWin)
        return BadWindow;
    if ((stuff->propagate != xFalse) && (stuff->propagate != xTrue)) {
        client->errorValue = stuff->propagate;
        return BadValue;
    }
    stuff->event.u.u.type |= SEND_EVENT_BIT;
    if (stuff->propagate) {
        for (; pWin; pWin = pWin->parent) {
            if (XaceHook(XACE_SEND_ACCESS, client, NULL, pWin,
                         &stuff->event, 1))
                return Success;
            if (DeliverEventsToWindow(dev, pWin,
                                      &stuff->event, 1, stuff->eventMask,
                                      NullGrab))
                return Success;
            if (pWin == effectiveFocus)
                return Success;
            stuff->eventMask &= ~wDontPropagateMask(pWin);
            if (!stuff->eventMask)
                break;
        }
    }
    else if (!XaceHook(XACE_SEND_ACCESS, client, NULL, pWin, &stuff->event, 1))
        DeliverEventsToWindow(dev, pWin, &stuff->event,
                              1, stuff->eventMask, NullGrab);
    return Success;
}

/**
 * Server-side protocol handling for UngrabKey request.
 *
 * Deletes a passive grab for the given key. Works on the
 * client's keyboard.
 */
int
ProcUngrabKey(ClientPtr client)
{
    REQUEST(xUngrabKeyReq);
    WindowPtr pWin;
    GrabPtr tempGrab;
    DeviceIntPtr keybd = PickKeyboard(client);
    int rc;

    REQUEST_SIZE_MATCH(xUngrabKeyReq);
    rc = dixLookupWindow(&pWin, stuff->grabWindow, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    if (((stuff->key > keybd->key->xkbInfo->desc->max_key_code) ||
         (stuff->key < keybd->key->xkbInfo->desc->min_key_code))
        && (stuff->key != AnyKey)) {
        client->errorValue = stuff->key;
        return BadValue;
    }
    if ((stuff->modifiers != AnyModifier) &&
        (stuff->modifiers & ~AllModifiersMask)) {
        client->errorValue = stuff->modifiers;
        return BadValue;
    }
    tempGrab = AllocGrab(NULL);
    if (!tempGrab)
        return BadAlloc;
    tempGrab->resource = client->clientAsMask;
    tempGrab->device = keybd;
    tempGrab->window = pWin;
    tempGrab->modifiersDetail.exact = stuff->modifiers;
    tempGrab->modifiersDetail.pMask = NULL;
    tempGrab->modifierDevice = keybd;
    tempGrab->type = KeyPress;
    tempGrab->grabtype = CORE;
    tempGrab->detail.exact = stuff->key;
    tempGrab->detail.pMask = NULL;
    tempGrab->next = NULL;

    if (!DeletePassiveGrabFromList(tempGrab))
        rc = BadAlloc;

    FreeGrab(tempGrab);

    return rc;
}

/**
 * Server-side protocol handling for GrabKey request.
 *
 * Creates a grab for the client's keyboard and adds it to the list of passive
 * grabs.
 */
int
ProcGrabKey(ClientPtr client)
{
    WindowPtr pWin;

    REQUEST(xGrabKeyReq);
    GrabPtr grab;
    DeviceIntPtr keybd = PickKeyboard(client);
    int rc;
    GrabParameters param;
    GrabMask mask;

    REQUEST_SIZE_MATCH(xGrabKeyReq);

    param = (GrabParameters) {
        .grabtype = CORE,
        .ownerEvents = stuff->ownerEvents,
        .this_device_mode = stuff->keyboardMode,
        .other_devices_mode = stuff->pointerMode,
        .modifiers = stuff->modifiers
    };

    rc = CheckGrabValues(client, &param);
    if (rc != Success)
        return rc;

    if (((stuff->key > keybd->key->xkbInfo->desc->max_key_code) ||
         (stuff->key < keybd->key->xkbInfo->desc->min_key_code))
        && (stuff->key != AnyKey)) {
        client->errorValue = stuff->key;
        return BadValue;
    }
    rc = dixLookupWindow(&pWin, stuff->grabWindow, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    mask.core = (KeyPressMask | KeyReleaseMask);

    grab = CreateGrab(client->index, keybd, keybd, pWin, CORE, &mask,
                      &param, KeyPress, stuff->key, NullWindow, NullCursor);
    if (!grab)
        return BadAlloc;
    return AddPassiveGrabToList(client, grab);
}

/**
 * Server-side protocol handling for GrabButton request.
 *
 * Creates a grab for the client's ClientPointer and adds it as a passive grab
 * to the list.
 */
int
ProcGrabButton(ClientPtr client)
{
    WindowPtr pWin, confineTo;

    REQUEST(xGrabButtonReq);
    CursorPtr cursor;
    GrabPtr grab;
    DeviceIntPtr ptr, modifierDevice;
    Mask access_mode = DixGrabAccess;
    GrabMask mask;
    GrabParameters param;
    int rc;

    REQUEST_SIZE_MATCH(xGrabButtonReq);
    UpdateCurrentTime();
    if ((stuff->pointerMode != GrabModeSync) &&
        (stuff->pointerMode != GrabModeAsync)) {
        client->errorValue = stuff->pointerMode;
        return BadValue;
    }
    if ((stuff->keyboardMode != GrabModeSync) &&
        (stuff->keyboardMode != GrabModeAsync)) {
        client->errorValue = stuff->keyboardMode;
        return BadValue;
    }
    if ((stuff->modifiers != AnyModifier) &&
        (stuff->modifiers & ~AllModifiersMask)) {
        client->errorValue = stuff->modifiers;
        return BadValue;
    }
    if ((stuff->ownerEvents != xFalse) && (stuff->ownerEvents != xTrue)) {
        client->errorValue = stuff->ownerEvents;
        return BadValue;
    }
    if (stuff->eventMask & ~PointerGrabMask) {
        client->errorValue = stuff->eventMask;
        return BadValue;
    }
    rc = dixLookupWindow(&pWin, stuff->grabWindow, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;
    if (stuff->confineTo == None)
        confineTo = NullWindow;
    else {
        rc = dixLookupWindow(&confineTo, stuff->confineTo, client,
                             DixSetAttrAccess);
        if (rc != Success)
            return rc;
    }
    if (stuff->cursor == None)
        cursor = NullCursor;
    else {
        rc = dixLookupResourceByType((void **) &cursor, stuff->cursor,
                                     RT_CURSOR, client, DixUseAccess);
        if (rc != Success) {
            client->errorValue = stuff->cursor;
            return rc;
        }
        access_mode |= DixForceAccess;
    }

    ptr = PickPointer(client);
    modifierDevice = GetMaster(ptr, MASTER_KEYBOARD);
    if (stuff->pointerMode == GrabModeSync ||
        stuff->keyboardMode == GrabModeSync)
        access_mode |= DixFreezeAccess;
    rc = XaceHook(XACE_DEVICE_ACCESS, client, ptr, access_mode);
    if (rc != Success)
        return rc;

    param = (GrabParameters) {
        .grabtype = CORE,
        .ownerEvents = stuff->ownerEvents,
        .this_device_mode = stuff->keyboardMode,
        .other_devices_mode = stuff->pointerMode,
        .modifiers = stuff->modifiers
    };

    mask.core = stuff->eventMask;

    grab = CreateGrab(client->index, ptr, modifierDevice, pWin,
                      CORE, &mask, &param, ButtonPress,
                      stuff->button, confineTo, cursor);
    if (!grab)
        return BadAlloc;
    return AddPassiveGrabToList(client, grab);
}

/**
 * Server-side protocol handling for UngrabButton request.
 *
 * Deletes a passive grab on the client's ClientPointer from the list.
 */
int
ProcUngrabButton(ClientPtr client)
{
    REQUEST(xUngrabButtonReq);
    WindowPtr pWin;
    GrabPtr tempGrab;
    int rc;
    DeviceIntPtr ptr;

    REQUEST_SIZE_MATCH(xUngrabButtonReq);
    UpdateCurrentTime();
    if ((stuff->modifiers != AnyModifier) &&
        (stuff->modifiers & ~AllModifiersMask)) {
        client->errorValue = stuff->modifiers;
        return BadValue;
    }
    rc = dixLookupWindow(&pWin, stuff->grabWindow, client, DixReadAccess);
    if (rc != Success)
        return rc;

    ptr = PickPointer(client);

    tempGrab = AllocGrab(NULL);
    if (!tempGrab)
        return BadAlloc;
    tempGrab->resource = client->clientAsMask;
    tempGrab->device = ptr;
    tempGrab->window = pWin;
    tempGrab->modifiersDetail.exact = stuff->modifiers;
    tempGrab->modifiersDetail.pMask = NULL;
    tempGrab->modifierDevice = GetMaster(ptr, MASTER_KEYBOARD);
    tempGrab->type = ButtonPress;
    tempGrab->detail.exact = stuff->button;
    tempGrab->grabtype = CORE;
    tempGrab->detail.pMask = NULL;
    tempGrab->next = NULL;

    if (!DeletePassiveGrabFromList(tempGrab))
        rc = BadAlloc;

    FreeGrab(tempGrab);
    return rc;
}

/**
 * Deactivate any grab that may be on the window, remove the focus.
 * Delete any XInput extension events from the window too. Does not change the
 * window mask. Use just before the window is deleted.
 *
 * If freeResources is set, passive grabs on the window are deleted.
 *
 * @param pWin The window to delete events from.
 * @param freeResources True if resources associated with the window should be
 * deleted.
 */
void
DeleteWindowFromAnyEvents(WindowPtr pWin, Bool freeResources)
{
    WindowPtr parent;
    DeviceIntPtr mouse = inputInfo.pointer;
    DeviceIntPtr keybd = inputInfo.keyboard;
    FocusClassPtr focus;
    OtherClientsPtr oc;
    GrabPtr passive;
    GrabPtr grab;

    /* Deactivate any grabs performed on this window, before making any
       input focus changes. */
    grab = mouse->deviceGrab.grab;
    if (grab && ((grab->window == pWin) || (grab->confineTo == pWin)))
        (*mouse->deviceGrab.DeactivateGrab) (mouse);

    /* Deactivating a keyboard grab should cause focus events. */
    grab = keybd->deviceGrab.grab;
    if (grab && (grab->window == pWin))
        (*keybd->deviceGrab.DeactivateGrab) (keybd);

    /* And now the real devices */
    for (mouse = inputInfo.devices; mouse; mouse = mouse->next) {
        grab = mouse->deviceGrab.grab;
        if (grab && ((grab->window == pWin) || (grab->confineTo == pWin)))
            (*mouse->deviceGrab.DeactivateGrab) (mouse);
    }

    for (keybd = inputInfo.devices; keybd; keybd = keybd->next) {
        if (IsKeyboardDevice(keybd)) {
            focus = keybd->focus;

            /* If the focus window is a root window (ie. has no parent)
               then don't delete the focus from it. */

            if ((pWin == focus->win) && (pWin->parent != NullWindow)) {
                int focusEventMode = NotifyNormal;

                /* If a grab is in progress, then alter the mode of focus events. */

                if (keybd->deviceGrab.grab)
                    focusEventMode = NotifyWhileGrabbed;

                switch (focus->revert) {
                case RevertToNone:
                    DoFocusEvents(keybd, pWin, NoneWin, focusEventMode);
                    focus->win = NoneWin;
                    focus->traceGood = 0;
                    break;
                case RevertToParent:
                    parent = pWin;
                    do {
                        parent = parent->parent;
                        focus->traceGood--;
                    } while (!parent->realized
                    /* This would be a good protocol change -- windows being
                       reparented during SaveSet processing would cause the
                       focus to revert to the nearest enclosing window which
                       will survive the death of the exiting client, instead
                       of ending up reverting to a dying window and thence
                       to None */
#ifdef NOTDEF
                             || wClient(parent)->clientGone
#endif
                        );
                    if (!ActivateFocusInGrab(keybd, pWin, parent))
                        DoFocusEvents(keybd, pWin, parent, focusEventMode);
                    focus->win = parent;
                    focus->revert = RevertToNone;
                    break;
                case RevertToPointerRoot:
                    if (!ActivateFocusInGrab(keybd, pWin, PointerRootWin))
                        DoFocusEvents(keybd, pWin, PointerRootWin,
                                      focusEventMode);
                    focus->win = PointerRootWin;
                    focus->traceGood = 0;
                    break;
                }
            }
        }

        if (IsPointerDevice(keybd)) {
            if (keybd->valuator->motionHintWindow == pWin)
                keybd->valuator->motionHintWindow = NullWindow;
        }
    }

    if (freeResources) {
        if (pWin->dontPropagate)
            DontPropagateRefCnts[pWin->dontPropagate]--;
        while ((oc = wOtherClients(pWin)))
            FreeResource(oc->resource, RT_NONE);
        while ((passive = wPassiveGrabs(pWin)))
            FreeResource(passive->resource, RT_NONE);
    }

    DeleteWindowFromAnyExtEvents(pWin, freeResources);
}

/**
 * Call this whenever some window at or below pWin has changed geometry. If
 * there is a grab on the window, the cursor will be re-confined into the
 * window.
 */
void
CheckCursorConfinement(WindowPtr pWin)
{
    GrabPtr grab;
    WindowPtr confineTo;
    DeviceIntPtr pDev;

#ifdef PANORAMIX
    if (!noPanoramiXExtension && pWin->drawable.pScreen->myNum)
        return;
#endif

    for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
        if (DevHasCursor(pDev)) {
            grab = pDev->deviceGrab.grab;
            if (grab && (confineTo = grab->confineTo)) {
                if (!BorderSizeNotEmpty(pDev, confineTo))
                    (*pDev->deviceGrab.DeactivateGrab) (pDev);
                else if ((pWin == confineTo) || IsParent(pWin, confineTo))
                    ConfineCursorToWindow(pDev, confineTo, TRUE, TRUE);
            }
        }
    }
}

Mask
EventMaskForClient(WindowPtr pWin, ClientPtr client)
{
    OtherClientsPtr other;

    if (wClient(pWin) == client)
        return pWin->eventMask;
    for (other = wOtherClients(pWin); other; other = other->next) {
        if (SameClient(other, client))
            return other->mask;
    }
    return 0;
}

/**
 * Server-side protocol handling for RecolorCursor request.
 */
int
ProcRecolorCursor(ClientPtr client)
{
    CursorPtr pCursor;
    int rc, nscr;
    ScreenPtr pscr;
    Bool displayed;
    SpritePtr pSprite = PickPointer(client)->spriteInfo->sprite;

    REQUEST(xRecolorCursorReq);

    REQUEST_SIZE_MATCH(xRecolorCursorReq);
    rc = dixLookupResourceByType((void **) &pCursor, stuff->cursor, RT_CURSOR,
                                 client, DixWriteAccess);
    if (rc != Success) {
        client->errorValue = stuff->cursor;
        return rc;
    }

    pCursor->foreRed = stuff->foreRed;
    pCursor->foreGreen = stuff->foreGreen;
    pCursor->foreBlue = stuff->foreBlue;

    pCursor->backRed = stuff->backRed;
    pCursor->backGreen = stuff->backGreen;
    pCursor->backBlue = stuff->backBlue;

    for (nscr = 0; nscr < screenInfo.numScreens; nscr++) {
        pscr = screenInfo.screens[nscr];
#ifdef PANORAMIX
        if (!noPanoramiXExtension)
            displayed = (pscr == pSprite->screen);
        else
#endif
            displayed = (pscr == pSprite->hotPhys.pScreen);
        (*pscr->RecolorCursor) (PickPointer(client), pscr, pCursor,
                                (pCursor == pSprite->current) && displayed);
    }
    return Success;
}

/**
 * Write the given events to a client, swapping the byte order if necessary.
 * To swap the byte ordering, a callback is called that has to be set up for
 * the given event type.
 *
 * In the case of DeviceMotionNotify trailed by DeviceValuators, the events
 * can be more than one. Usually it's just one event.
 *
 * Do not modify the event structure passed in. See comment below.
 *
 * @param pClient Client to send events to.
 * @param count Number of events.
 * @param events The event list.
 */
void
WriteEventsToClient(ClientPtr pClient, int count, xEvent *events)
{
#ifdef PANORAMIX
    xEvent eventCopy;
#endif
    xEvent *eventTo, *eventFrom;
    int i, eventlength = sizeof(xEvent);

    if (!pClient || pClient == serverClient || pClient->clientGone)
        return;

    for (i = 0; i < count; i++)
        if ((events[i].u.u.type & 0x7f) != KeymapNotify)
            events[i].u.u.sequenceNumber = pClient->sequence;

    /* Let XKB rewrite the state, as it depends on client preferences. */
    XkbFilterEvents(pClient, count, events);

#ifdef PANORAMIX
    if (!noPanoramiXExtension &&
        (screenInfo.screens[0]->x || screenInfo.screens[0]->y)) {
        switch (events->u.u.type) {
        case MotionNotify:
        case ButtonPress:
        case ButtonRelease:
        case KeyPress:
        case KeyRelease:
        case EnterNotify:
        case LeaveNotify:
            /*
               When multiple clients want the same event DeliverEventsToWindow
               passes the same event structure multiple times so we can't
               modify the one passed to us
             */
            count = 1;          /* should always be 1 */
            memcpy(&eventCopy, events, sizeof(xEvent));
            eventCopy.u.keyButtonPointer.rootX += screenInfo.screens[0]->x;
            eventCopy.u.keyButtonPointer.rootY += screenInfo.screens[0]->y;
            if (eventCopy.u.keyButtonPointer.event ==
                eventCopy.u.keyButtonPointer.root) {
                eventCopy.u.keyButtonPointer.eventX += screenInfo.screens[0]->x;
                eventCopy.u.keyButtonPointer.eventY += screenInfo.screens[0]->y;
            }
            events = &eventCopy;
            break;
        default:
            break;
        }
    }
#endif

    if (EventCallback) {
        EventInfoRec eventinfo;

        eventinfo.client = pClient;
        eventinfo.events = events;
        eventinfo.count = count;
        CallCallbacks(&EventCallback, (void *) &eventinfo);
    }
#ifdef XSERVER_DTRACE
    if (XSERVER_SEND_EVENT_ENABLED()) {
        for (i = 0; i < count; i++) {
            XSERVER_SEND_EVENT(pClient->index, events[i].u.u.type, &events[i]);
        }
    }
#endif
    /* Just a safety check to make sure we only have one GenericEvent, it just
     * makes things easier for me right now. (whot) */
    for (i = 1; i < count; i++) {
        if (events[i].u.u.type == GenericEvent) {
            ErrorF("[dix] TryClientEvents: Only one GenericEvent at a time.\n");
            return;
        }
    }

    if (events->u.u.type == GenericEvent) {
        eventlength += ((xGenericEvent *) events)->length * 4;
    }

    if (pClient->swapped) {
        if (eventlength > swapEventLen) {
            swapEventLen = eventlength;
            swapEvent = realloc(swapEvent, swapEventLen);
            if (!swapEvent) {
                FatalError("WriteEventsToClient: Out of memory.\n");
                return;
            }
        }

        for (i = 0; i < count; i++) {
            eventFrom = &events[i];
            eventTo = swapEvent;

            /* Remember to strip off the leading bit of type in case
               this event was sent with "SendEvent." */
            (*EventSwapVector[eventFrom->u.u.type & 0177])
                (eventFrom, eventTo);

            WriteToClient(pClient, eventlength, eventTo);
        }
    }
    else {
        /* only one GenericEvent, remember? that means either count is 1 and
         * eventlength is arbitrary or eventlength is 32 and count doesn't
         * matter. And we're all set. Woohoo. */
        WriteToClient(pClient, count * eventlength, events);
    }
}

/*
 * Set the client pointer for the given client.
 *
 * A client can have exactly one ClientPointer. Each time a
 * request/reply/event is processed and the choice of devices is ambiguous
 * (e.g. QueryPointer request), the server will pick the ClientPointer (see
 * PickPointer()).
 * If a keyboard is needed, the first keyboard paired with the CP is used.
 */
int
SetClientPointer(ClientPtr client, DeviceIntPtr device)
{
    int rc = XaceHook(XACE_DEVICE_ACCESS, client, device, DixUseAccess);

    if (rc != Success)
        return rc;

    if (!IsMaster(device)) {
        ErrorF("[dix] Need master device for ClientPointer. This is a bug.\n");
        return BadDevice;
    }
    else if (!device->spriteInfo->spriteOwner) {
        ErrorF("[dix] Device %d does not have a sprite. "
               "Cannot be ClientPointer\n", device->id);
        return BadDevice;
    }
    client->clientPtr = device;
    return Success;
}

/* PickPointer will pick an appropriate pointer for the given client.
 *
 * An "appropriate device" is (in order of priority):
 *  1) A device the given client has a core grab on.
 *  2) A device set as ClientPointer for the given client.
 *  3) The first master device.
 */
DeviceIntPtr
PickPointer(ClientPtr client)
{
    DeviceIntPtr it = inputInfo.devices;

    /* First, check if the client currently has a grab on a device. Even
     * keyboards count. */
    for (it = inputInfo.devices; it; it = it->next) {
        GrabPtr grab = it->deviceGrab.grab;

        if (grab && grab->grabtype == CORE && SameClient(grab, client)) {
            it = GetMaster(it, MASTER_POINTER);
            return it;          /* Always return a core grabbed device */
        }
    }

    if (!client->clientPtr) {
        it = inputInfo.devices;
        while (it) {
            if (IsMaster(it) && it->spriteInfo->spriteOwner) {
                client->clientPtr = it;
                break;
            }
            it = it->next;
        }
    }
    return client->clientPtr;
}

/* PickKeyboard will pick an appropriate keyboard for the given client by
 * searching the list of devices for the keyboard device that is paired with
 * the client's pointer.
 */
DeviceIntPtr
PickKeyboard(ClientPtr client)
{
    DeviceIntPtr ptr = PickPointer(client);
    DeviceIntPtr kbd = GetMaster(ptr, MASTER_KEYBOARD);

    if (!kbd) {
        ErrorF("[dix] ClientPointer not paired with a keyboard. This "
               "is a bug.\n");
    }

    return kbd;
}

/* A client that has one or more core grabs does not get core events from
 * devices it does not have a grab on. Legacy applications behave bad
 * otherwise because they are not used to it and the events interfere.
 * Only applies for core events.
 *
 * Return true if a core event from the device would interfere and should not
 * be delivered.
 */
Bool
IsInterferingGrab(ClientPtr client, DeviceIntPtr dev, xEvent *event)
{
    DeviceIntPtr it = inputInfo.devices;

    switch (event->u.u.type) {
    case KeyPress:
    case KeyRelease:
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
    case EnterNotify:
    case LeaveNotify:
        break;
    default:
        return FALSE;
    }

    if (dev->deviceGrab.grab && SameClient(dev->deviceGrab.grab, client))
        return FALSE;

    while (it) {
        if (it != dev) {
            if (it->deviceGrab.grab && SameClient(it->deviceGrab.grab, client)
                && !it->deviceGrab.fromPassiveGrab) {
                if ((IsPointerDevice(it) && IsPointerDevice(dev)) ||
                    (IsKeyboardDevice(it) && IsKeyboardDevice(dev)))
                    return TRUE;
            }
        }
        it = it->next;
    }

    return FALSE;
}

/* PointerBarrier events are only delivered to the client that created that
 * barrier */
static Bool
IsWrongPointerBarrierClient(ClientPtr client, DeviceIntPtr dev, xEvent *event)
{
    xXIBarrierEvent *ev = (xXIBarrierEvent*)event;

    if (ev->type != GenericEvent || ev->extension != IReqCode)
        return FALSE;

    if (ev->evtype != XI_BarrierHit && ev->evtype != XI_BarrierLeave)
        return FALSE;

    return client->index != CLIENT_ID(ev->barrier);
}
