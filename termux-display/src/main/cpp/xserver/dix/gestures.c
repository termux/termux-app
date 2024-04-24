/*
 * Copyright © 2011 Collabra Ltd.
 * Copyright © 2011 Red Hat, Inc.
 * Copyright © 2020 Povilas Kanapickas  <povilas@radix.lt>
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"
#include "scrnintstr.h"
#include "dixgrabs.h"

#include "eventstr.h"
#include "exevents.h"
#include "exglobals.h"
#include "inpututils.h"
#include "eventconvert.h"
#include "windowstr.h"
#include "mi.h"

#define GESTURE_HISTORY_SIZE 100

Bool
GestureInitGestureInfo(GestureInfoPtr gi)
{
    memset(gi, 0, sizeof(*gi));

    gi->sprite.spriteTrace = calloc(32, sizeof(*gi->sprite.spriteTrace));
    if (!gi->sprite.spriteTrace) {
        return FALSE;
    }
    gi->sprite.spriteTraceSize = 32;
    gi->sprite.spriteTrace[0] = screenInfo.screens[0]->root;
    gi->sprite.hot.pScreen = screenInfo.screens[0];
    gi->sprite.hotPhys.pScreen = screenInfo.screens[0];

    return TRUE;
}

/**
 * Given an event type returns the associated gesture event info.
 */
GestureInfoPtr
GestureFindActiveByEventType(DeviceIntPtr dev, int type)
{
    GestureClassPtr g = dev->gesture;
    enum EventType type_to_expect = GestureTypeToBegin(type);

    if (!g || type_to_expect == 0 || !g->gesture.active ||
        g->gesture.type != type_to_expect) {
        return NULL;
    }

    return &g->gesture;
}

/**
 * Sets up gesture info for a new gesture. Returns NULL on failure.
 */
GestureInfoPtr
GestureBeginGesture(DeviceIntPtr dev, InternalEvent *ev)
{
    GestureClassPtr g = dev->gesture;
    enum EventType gesture_type = GestureTypeToBegin(ev->any.type);

    /* Note that we ignore begin events when an existing gesture is active */
    if (!g || gesture_type == 0 || g->gesture.active)
        return NULL;

    g->gesture.type = gesture_type;

    if (!GestureBuildSprite(dev, &g->gesture))
        return NULL;

    g->gesture.active = TRUE;
    g->gesture.num_touches = ev->gesture_event.num_touches;
    g->gesture.sourceid = ev->gesture_event.sourceid;
    g->gesture.has_listener = FALSE;
    return &g->gesture;
}

/**
 * Releases a gesture: this must only be called after all events
 * related to that gesture have been sent and finalised.
 */
void
GestureEndGesture(GestureInfoPtr gi)
{
    if (gi->has_listener) {
        if (gi->listener.grab) {
            FreeGrab(gi->listener.grab);
            gi->listener.grab = NULL;
        }
        gi->listener.listener = 0;
        gi->has_listener = FALSE;
    }

    gi->active = FALSE;
    gi->num_touches = 0;
    gi->sprite.spriteTraceGood = 0;
}

/**
 * Ensure a window trace is present in gi->sprite, constructing one for
 * Gesture{Pinch,Swipe}Begin events.
 */
Bool
GestureBuildSprite(DeviceIntPtr sourcedev, GestureInfoPtr gi)
{
    SpritePtr sprite = &gi->sprite;

    if (!sourcedev->spriteInfo->sprite)
        return FALSE;

    if (!CopySprite(sourcedev->spriteInfo->sprite, sprite))
        return FALSE;

    if (sprite->spriteTraceGood <= 0)
        return FALSE;

    return TRUE;
}

/**
 * @returns TRUE if the specified grab or selection is the current owner of
 * the gesture sequence.
 */
Bool
GestureResourceIsOwner(GestureInfoPtr gi, XID resource)
{
    return (gi->listener.listener == resource);
}

void
GestureAddListener(GestureInfoPtr gi, XID resource, int resource_type,
                   enum GestureListenerType type, WindowPtr window, const GrabPtr grab)
{
    GrabPtr g = NULL;

    BUG_RETURN(gi->has_listener);

    /* We need a copy of the grab, not the grab itself since that may be deleted by
     * a UngrabButton request and leaves us with a dangling pointer */
    if (grab)
        g = AllocGrab(grab);

    gi->listener.listener = resource;
    gi->listener.resource_type = resource_type;
    gi->listener.type = type;
    gi->listener.window = window;
    gi->listener.grab = g;
    gi->has_listener = TRUE;
}

static void
GestureAddGrabListener(DeviceIntPtr dev, GestureInfoPtr gi, GrabPtr grab)
{
    enum GestureListenerType type;

    /* FIXME: owner_events */

    if (grab->grabtype == XI2) {
        if (xi2mask_isset(grab->xi2mask, dev, XI_GesturePinchBegin) ||
            xi2mask_isset(grab->xi2mask, dev, XI_GestureSwipeBegin)) {
            type = GESTURE_LISTENER_GRAB;
        } else
            type = GESTURE_LISTENER_NONGESTURE_GRAB;
    }
    else if (grab->grabtype == XI || grab->grabtype == CORE) {
        type = GESTURE_LISTENER_NONGESTURE_GRAB;
    }
    else {
        BUG_RETURN_MSG(1, "Unsupported grab type\n");
    }

    /* grab listeners are always RT_NONE since we keep the grab pointer */
    GestureAddListener(gi, grab->resource, RT_NONE, type, grab->window, grab);
}

/**
 * Add one listener if there is a grab on the given window.
 */
static void
GestureAddPassiveGrabListener(DeviceIntPtr dev, GestureInfoPtr gi, WindowPtr win, InternalEvent *ev)
{
    Bool activate = FALSE;
    Bool check_core = FALSE;

    GrabPtr grab = CheckPassiveGrabsOnWindow(win, dev, ev, check_core,
                                             activate);
    if (!grab)
        return;

    /* We'll deliver later in gesture-specific code */
    ActivateGrabNoDelivery(dev, grab, ev, ev);
    GestureAddGrabListener(dev, gi, grab);
}

static void
GestureAddRegularListener(DeviceIntPtr dev, GestureInfoPtr gi, WindowPtr win, InternalEvent *ev)
{
    InputClients *iclients = NULL;
    OtherInputMasks *inputMasks = NULL;
    uint16_t evtype = GetXI2Type(ev->any.type);
    int mask;

    mask = EventIsDeliverable(dev, ev->any.type, win);
    if (!mask)
        return;

    inputMasks = wOtherInputMasks(win);

    if (mask & EVENT_XI2_MASK) {
        nt_list_for_each_entry(iclients, inputMasks->inputClients, next) {
            if (!xi2mask_isset(iclients->xi2mask, dev, evtype))
                continue;

            GestureAddListener(gi, iclients->resource, RT_INPUTCLIENT,
                               GESTURE_LISTENER_REGULAR, win, NULL);
            return;
        }
    }
}

void
GestureSetupListener(DeviceIntPtr dev, GestureInfoPtr gi, InternalEvent *ev)
{
    int i;
    SpritePtr sprite = &gi->sprite;
    WindowPtr win;

    /* Any current grab will consume all gesture events */
    if (dev->deviceGrab.grab) {
        GestureAddGrabListener(dev, gi, dev->deviceGrab.grab);
        return;
    }

    /* Find passive grab that would be activated by this event, if any. If we're handling
     * ReplayDevice then the search starts from the descendant of the grab window, otherwise
     * the search starts at the root window. The search ends at deepest child window. */
    i = 0;
    if (syncEvents.playingEvents) {
        while (i < dev->spriteInfo->sprite->spriteTraceGood) {
            if (dev->spriteInfo->sprite->spriteTrace[i++] == syncEvents.replayWin)
                break;
        }
    }

    for (; i < sprite->spriteTraceGood; i++) {
        win = sprite->spriteTrace[i];
        GestureAddPassiveGrabListener(dev, gi, win, ev);
        if (gi->has_listener)
            return;
    }

    /* Find the first client with an applicable event selection,
     * going from deepest child window back up to the root window. */
    for (i = sprite->spriteTraceGood - 1; i >= 0; i--) {
        win = sprite->spriteTrace[i];
        GestureAddRegularListener(dev, gi, win, ev);
        if (gi->has_listener)
            return;
    }
}

/* As gesture grabs don't turn into active grabs with their own resources, we
 * need to walk all the gestures and remove this grab from listener */
void
GestureListenerGone(XID resource)
{
    GestureInfoPtr gi;
    DeviceIntPtr dev;
    InternalEvent *events = InitEventList(GetMaximumEventsNum());

    if (!events)
        FatalError("GestureListenerGone: couldn't allocate events\n");

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (!dev->gesture)
            continue;

        gi = &dev->gesture->gesture;
        if (!gi->active)
            continue;

        if (CLIENT_BITS(gi->listener.listener) == resource)
            GestureEndGesture(gi);
    }

    FreeEventList(events, GetMaximumEventsNum());
}

/**
 * End physically active gestures for a device.
 */
void
GestureEndActiveGestures(DeviceIntPtr dev)
{
    GestureClassPtr g = dev->gesture;
    InternalEvent *eventlist;

    if (!g)
        return;

    eventlist = InitEventList(GetMaximumEventsNum());

    input_lock();
    mieqProcessInputEvents();
    if (g->gesture.active) {
        int j;
        int type = GetXI2Type(GestureTypeToEnd(g->gesture.type));
        int nevents = GetGestureEvents(eventlist, dev, type, g->gesture.num_touches,
                                       0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

        for (j = 0; j < nevents; j++)
            mieqProcessDeviceEvent(dev, eventlist + j, NULL);
    }
    input_unlock();

    FreeEventList(eventlist, GetMaximumEventsNum());
}

/**
 * Generate and deliver a Gesture{Pinch,Swipe}End event to the owner.
 *
 * @param dev The device to deliver the event for.
 * @param gi The gesture record to deliver the event for.
 */
void
GestureEmitGestureEndToOwner(DeviceIntPtr dev, GestureInfoPtr gi)
{
    InternalEvent event;
    /* We're not processing a gesture end for a frozen device */
    if (dev->deviceGrab.sync.frozen)
        return;

    DeliverDeviceClassesChangedEvent(gi->sourceid, GetTimeInMillis());
    InitGestureEvent(&event, dev, GetTimeInMillis(), GestureTypeToEnd(gi->type),
                     0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    DeliverGestureEventToOwner(dev, gi, &event);
}
