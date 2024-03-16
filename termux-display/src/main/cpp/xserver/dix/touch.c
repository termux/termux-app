/*
 * Copyright © 2011 Collabra Ltd.
 * Copyright © 2011 Red Hat, Inc.
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
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
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

#define TOUCH_HISTORY_SIZE 100

Bool touchEmulatePointer = TRUE;

/**
 * Some documentation about touch points:
 * The driver submits touch events with its own (unique) touch point ID.
 * The driver may re-use those IDs, the DDX doesn't care. It just passes on
 * the data to the DIX. In the server, the driver's ID is referred to as the
 * DDX id anyway.
 *
 * On a TouchBegin, we create a DDXTouchPointInfo that contains the DDX id
 * and the client ID that this touchpoint will have. The client ID is the
 * one visible on the protocol.
 *
 * TouchUpdate and TouchEnd will only be processed if there is an active
 * touchpoint with the same DDX id.
 *
 * The DDXTouchPointInfo struct is stored dev->last.touches. When the event
 * being processed, it becomes a TouchPointInfo in dev->touch-touches which
 * contains amongst other things the sprite trace and delivery information.
 */

/**
 * Check which devices need a bigger touch event queue and grow their
 * last.touches by half its current size.
 *
 * @param client Always the serverClient
 * @param closure Always NULL
 *
 * @return Always True. If we fail to grow we probably will topple over soon
 * anyway and re-executing this won't help.
 */

static Bool
TouchResizeQueue(DeviceIntPtr dev)
{
    DDXTouchPointInfoPtr tmp;
    size_t size;

    /* Grow sufficiently so we don't need to do it often */
    size = dev->last.num_touches + dev->last.num_touches / 2 + 1;

    tmp = reallocarray(dev->last.touches, size, sizeof(*dev->last.touches));
    if (tmp) {
        int j;

        dev->last.touches = tmp;
        for (j = dev->last.num_touches; j < size; j++)
            TouchInitDDXTouchPoint(dev, &dev->last.touches[j]);
        dev->last.num_touches = size;
        return TRUE;
    }
    return FALSE;
}

/**
 * Given the DDX-facing ID (which is _not_ DeviceEvent::detail.touch), find the
 * associated DDXTouchPointInfoRec.
 *
 * @param dev The device to create the touch point for
 * @param ddx_id Touch id assigned by the driver/ddx
 * @param create Create the touchpoint if it cannot be found
 */
DDXTouchPointInfoPtr
TouchFindByDDXID(DeviceIntPtr dev, uint32_t ddx_id, Bool create)
{
    DDXTouchPointInfoPtr ti;
    int i;

    if (!dev->touch)
        return NULL;

    for (i = 0; i < dev->last.num_touches; i++) {
        ti = &dev->last.touches[i];
        if (ti->active && ti->ddx_id == ddx_id)
            return ti;
    }

    return create ? TouchBeginDDXTouch(dev, ddx_id) : NULL;
}

/**
 * Given a unique DDX ID for a touchpoint, create a touchpoint record and
 * return it.
 *
 * If no other touch points are active, mark new touchpoint for pointer
 * emulation.
 *
 * Returns NULL on failure (i.e. if another touch with that ID is already active,
 * allocation failure).
 */
DDXTouchPointInfoPtr
TouchBeginDDXTouch(DeviceIntPtr dev, uint32_t ddx_id)
{
    static int next_client_id = 1;
    int i;
    TouchClassPtr t = dev->touch;
    DDXTouchPointInfoPtr ti = NULL;
    Bool emulate_pointer;

    if (!t)
        return NULL;

    emulate_pointer = touchEmulatePointer && (t->mode == XIDirectTouch);

    /* Look for another active touchpoint with the same DDX ID. DDX
     * touchpoints must be unique. */
    if (TouchFindByDDXID(dev, ddx_id, FALSE))
        return NULL;

    for (;;) {
        for (i = 0; i < dev->last.num_touches; i++) {
            /* Only emulate pointer events on the first touch */
            if (dev->last.touches[i].active)
                emulate_pointer = FALSE;
            else if (!ti)           /* ti is now first non-active touch rec */
                ti = &dev->last.touches[i];

            if (!emulate_pointer && ti)
                break;
        }
        if (ti)
            break;
        if (!TouchResizeQueue(dev))
            break;
    }

    if (ti) {
        int client_id;

        ti->active = TRUE;
        ti->ddx_id = ddx_id;
        client_id = next_client_id;
        next_client_id++;
        if (next_client_id == 0)
            next_client_id = 1;
        ti->client_id = client_id;
        ti->emulate_pointer = emulate_pointer;
    }
    return ti;
}

void
TouchEndDDXTouch(DeviceIntPtr dev, DDXTouchPointInfoPtr ti)
{
    TouchClassPtr t = dev->touch;

    if (!t)
        return;

    ti->active = FALSE;
}

void
TouchInitDDXTouchPoint(DeviceIntPtr dev, DDXTouchPointInfoPtr ddxtouch)
{
    memset(ddxtouch, 0, sizeof(*ddxtouch));
    ddxtouch->valuators = valuator_mask_new(dev->valuator->numAxes);
}

Bool
TouchInitTouchPoint(TouchClassPtr t, ValuatorClassPtr v, int index)
{
    TouchPointInfoPtr ti;

    if (index >= t->num_touches)
        return FALSE;
    ti = &t->touches[index];

    memset(ti, 0, sizeof(*ti));

    ti->valuators = valuator_mask_new(v->numAxes);
    if (!ti->valuators)
        return FALSE;

    ti->sprite.spriteTrace = calloc(32, sizeof(*ti->sprite.spriteTrace));
    if (!ti->sprite.spriteTrace) {
        valuator_mask_free(&ti->valuators);
        return FALSE;
    }
    ti->sprite.spriteTraceSize = 32;
    ti->sprite.spriteTrace[0] = screenInfo.screens[0]->root;
    ti->sprite.hot.pScreen = screenInfo.screens[0];
    ti->sprite.hotPhys.pScreen = screenInfo.screens[0];

    ti->client_id = -1;

    return TRUE;
}

void
TouchFreeTouchPoint(DeviceIntPtr device, int index)
{
    TouchPointInfoPtr ti;
    int i;

    if (!device->touch || index >= device->touch->num_touches)
        return;
    ti = &device->touch->touches[index];

    if (ti->active)
        TouchEndTouch(device, ti);

    for (i = 0; i < ti->num_listeners; i++)
        TouchRemoveListener(ti, ti->listeners[0].listener);

    valuator_mask_free(&ti->valuators);
    free(ti->sprite.spriteTrace);
    ti->sprite.spriteTrace = NULL;
    free(ti->listeners);
    ti->listeners = NULL;
    free(ti->history);
    ti->history = NULL;
    ti->history_size = 0;
    ti->history_elements = 0;
}

/**
 * Given a client-facing ID (e.g. DeviceEvent::detail.touch), find the
 * associated TouchPointInfoRec.
 */
TouchPointInfoPtr
TouchFindByClientID(DeviceIntPtr dev, uint32_t client_id)
{
    TouchClassPtr t = dev->touch;
    TouchPointInfoPtr ti;
    int i;

    if (!t)
        return NULL;

    for (i = 0; i < t->num_touches; i++) {
        ti = &t->touches[i];
        if (ti->active && ti->client_id == client_id)
            return ti;
    }

    return NULL;
}

/**
 * Given a unique ID for a touchpoint, create a touchpoint record in the
 * server.
 *
 * Returns NULL on failure (i.e. if another touch with that ID is already active,
 * allocation failure).
 */
TouchPointInfoPtr
TouchBeginTouch(DeviceIntPtr dev, int sourceid, uint32_t touchid,
                Bool emulate_pointer)
{
    int i;
    TouchClassPtr t = dev->touch;
    TouchPointInfoPtr ti;
    void *tmp;

    if (!t)
        return NULL;

    /* Look for another active touchpoint with the same client ID.  It's
     * technically legitimate for a touchpoint to still exist with the same
     * ID but only once the 32 bits wrap over and you've used up 4 billion
     * touch ids without lifting that one finger off once. In which case
     * you deserve a medal or something, but not error handling code. */
    if (TouchFindByClientID(dev, touchid))
        return NULL;

 try_find_touch:
    for (i = 0; i < t->num_touches; i++) {
        ti = &t->touches[i];
        if (!ti->active) {
            ti->active = TRUE;
            ti->client_id = touchid;
            ti->sourceid = sourceid;
            ti->emulate_pointer = emulate_pointer;
            return ti;
        }
    }

    /* If we get here, then we've run out of touches: enlarge dev->touch and
     * try again. */
    tmp = reallocarray(t->touches, t->num_touches + 1, sizeof(*ti));
    if (tmp) {
        t->touches = tmp;
        t->num_touches++;
        if (TouchInitTouchPoint(t, dev->valuator, t->num_touches - 1))
            goto try_find_touch;
    }

    return NULL;
}

/**
 * Releases a touchpoint for use: this must only be called after all events
 * related to that touchpoint have been sent and finalised.  Called from
 * ProcessTouchEvent and friends.  Not by you.
 */
void
TouchEndTouch(DeviceIntPtr dev, TouchPointInfoPtr ti)
{
    int i;

    if (ti->emulate_pointer) {
        GrabPtr grab;

        if ((grab = dev->deviceGrab.grab)) {
            if (dev->deviceGrab.fromPassiveGrab &&
                !dev->button->buttonsDown &&
                !dev->touch->buttonsDown && GrabIsPointerGrab(grab))
                (*dev->deviceGrab.DeactivateGrab) (dev);
        }
    }

    for (i = 0; i < ti->num_listeners; i++)
        TouchRemoveListener(ti, ti->listeners[0].listener);

    ti->active = FALSE;
    ti->pending_finish = FALSE;
    ti->sprite.spriteTraceGood = 0;
    free(ti->listeners);
    ti->listeners = NULL;
    ti->num_listeners = 0;
    ti->num_grabs = 0;
    ti->client_id = 0;

    TouchEventHistoryFree(ti);

    valuator_mask_zero(ti->valuators);
}

/**
 * Allocate the event history for this touch pointer. Calling this on a
 * touchpoint that already has an event history does nothing but counts as
 * as success.
 *
 * @return TRUE on success, FALSE on allocation errors
 */
Bool
TouchEventHistoryAllocate(TouchPointInfoPtr ti)
{
    if (ti->history)
        return TRUE;

    ti->history = calloc(TOUCH_HISTORY_SIZE, sizeof(*ti->history));
    ti->history_elements = 0;
    if (ti->history)
        ti->history_size = TOUCH_HISTORY_SIZE;
    return ti->history != NULL;
}

void
TouchEventHistoryFree(TouchPointInfoPtr ti)
{
    free(ti->history);
    ti->history = NULL;
    ti->history_size = 0;
    ti->history_elements = 0;
}

/**
 * Store the given event on the event history (if one exists)
 * A touch event history consists of one TouchBegin and several TouchUpdate
 * events (if applicable) but no TouchEnd event.
 * If more than one TouchBegin is pushed onto the stack, the push is
 * ignored, calling this function multiple times for the TouchBegin is
 * valid.
 */
void
TouchEventHistoryPush(TouchPointInfoPtr ti, const DeviceEvent *ev)
{
    if (!ti->history)
        return;

    switch (ev->type) {
    case ET_TouchBegin:
        /* don't store the same touchbegin twice */
        if (ti->history_elements > 0)
            return;
        break;
    case ET_TouchUpdate:
        break;
    case ET_TouchEnd:
        return;                 /* no TouchEnd events in the history */
    default:
        return;
    }

    /* We only store real events in the history */
    if (ev->flags & (TOUCH_CLIENT_ID | TOUCH_REPLAYING))
        return;

    ti->history[ti->history_elements++] = *ev;
    /* FIXME: proper overflow fixes */
    if (ti->history_elements > ti->history_size - 1) {
        ti->history_elements = ti->history_size - 1;
        DebugF("source device %d: history size %zu overflowing for touch %u\n",
               ti->sourceid, ti->history_size, ti->client_id);
    }
}

void
TouchEventHistoryReplay(TouchPointInfoPtr ti, DeviceIntPtr dev, XID resource)
{
    int i;

    if (!ti->history)
        return;

    DeliverDeviceClassesChangedEvent(ti->sourceid, ti->history[0].time);

    for (i = 0; i < ti->history_elements; i++) {
        DeviceEvent *ev = &ti->history[i];

        ev->flags |= TOUCH_REPLAYING;
        ev->resource = resource;
        /* FIXME:
           We're replaying ti->history which contains the TouchBegin +
           all TouchUpdates for ti. This needs to be passed on to the next
           listener. If that is a touch listener, everything is dandy.
           If the TouchBegin however triggers a sync passive grab, the
           TouchUpdate events must be sent to EnqueueEvent so the events end
           up in syncEvents.pending to be forwarded correctly in a
           subsequent ComputeFreeze().

           However, if we just send them to EnqueueEvent the sync'ing device
           prevents handling of touch events for ownership listeners who
           want the events right here, right now.
         */
        dev->public.processInputProc((InternalEvent*)ev, dev);
    }
}

Bool
TouchBuildDependentSpriteTrace(DeviceIntPtr dev, SpritePtr sprite)
{
    int i;
    TouchClassPtr t = dev->touch;
    SpritePtr srcsprite;

    /* All touches should have the same sprite trace, so find and reuse an
     * existing touch's sprite if possible, else use the device's sprite. */
    for (i = 0; i < t->num_touches; i++)
        if (!t->touches[i].pending_finish &&
            t->touches[i].sprite.spriteTraceGood > 0)
            break;
    if (i < t->num_touches)
        srcsprite = &t->touches[i].sprite;
    else if (dev->spriteInfo->sprite)
        srcsprite = dev->spriteInfo->sprite;
    else
        return FALSE;

    return CopySprite(srcsprite, sprite);
}

/**
 * Ensure a window trace is present in ti->sprite, constructing one for
 * TouchBegin events.
 */
Bool
TouchBuildSprite(DeviceIntPtr sourcedev, TouchPointInfoPtr ti,
                 InternalEvent *ev)
{
    TouchClassPtr t = sourcedev->touch;
    SpritePtr sprite = &ti->sprite;

    if (t->mode == XIDirectTouch) {
        /* Focus immediately under the touchpoint in direct touch mode.
         * XXX: Do we need to handle crossing screens here? */
        sprite->spriteTrace[0] =
            sourcedev->spriteInfo->sprite->hotPhys.pScreen->root;
        XYToWindow(sprite, ev->device_event.root_x, ev->device_event.root_y);
    }
    else if (!TouchBuildDependentSpriteTrace(sourcedev, sprite))
        return FALSE;

    if (sprite->spriteTraceGood <= 0)
        return FALSE;

    /* Mark which grabs/event selections we're delivering to: max one grab per
     * window plus the bottom-most event selection, plus any active grab. */
    ti->listeners = calloc(sprite->spriteTraceGood + 2, sizeof(*ti->listeners));
    if (!ti->listeners) {
        sprite->spriteTraceGood = 0;
        return FALSE;
    }
    ti->num_listeners = 0;

    return TRUE;
}

/**
 * Copy the touch event into the pointer_event, switching the required
 * fields to make it a correct pointer event.
 *
 * @param event The original touch event
 * @param[in] motion_event The respective motion event
 * @param[in] button_event The respective button event (if any)
 *
 * @returns The number of converted events.
 * @retval 0 An error occurred
 * @retval 1 only the motion event is valid
 * @retval 2 motion and button event are valid
 */
int
TouchConvertToPointerEvent(const InternalEvent *event,
                           InternalEvent *motion_event,
                           InternalEvent *button_event)
{
    int ptrtype;
    int nevents = 0;

    BUG_RETURN_VAL(!event, 0);
    BUG_RETURN_VAL(!motion_event, 0);

    switch (event->any.type) {
    case ET_TouchUpdate:
        nevents = 1;
        break;
    case ET_TouchBegin:
        nevents = 2;            /* motion + press */
        ptrtype = ET_ButtonPress;
        break;
    case ET_TouchEnd:
        nevents = 2;            /* motion + release */
        ptrtype = ET_ButtonRelease;
        break;
    default:
        BUG_WARN_MSG(1, "Invalid event type %d\n", event->any.type);
        return 0;
    }

    BUG_WARN_MSG(!(event->device_event.flags & TOUCH_POINTER_EMULATED),
                 "Non-emulating touch event\n");

    motion_event->device_event = event->device_event;
    motion_event->any.type = ET_Motion;
    motion_event->device_event.detail.button = 0;
    motion_event->device_event.flags = XIPointerEmulated;

    if (nevents > 1) {
        BUG_RETURN_VAL(!button_event, 0);
        button_event->device_event = event->device_event;
        button_event->any.type = ptrtype;
        button_event->device_event.flags = XIPointerEmulated;
        /* detail is already correct */
    }

    return nevents;
}

/**
 * Return the corresponding pointer emulation internal event type for the given
 * touch event or 0 if no such event type exists.
 */
int
TouchGetPointerEventType(const InternalEvent *event)
{
    int type = 0;

    switch (event->any.type) {
    case ET_TouchBegin:
        type = ET_ButtonPress;
        break;
    case ET_TouchUpdate:
        type = ET_Motion;
        break;
    case ET_TouchEnd:
        type = ET_ButtonRelease;
        break;
    default:
        break;
    }
    return type;
}

/**
 * @returns TRUE if the specified grab or selection is the current owner of
 * the touch sequence.
 */
Bool
TouchResourceIsOwner(TouchPointInfoPtr ti, XID resource)
{
    return (ti->listeners[0].listener == resource);
}

/**
 * Add the resource to this touch's listeners.
 */
void
TouchAddListener(TouchPointInfoPtr ti, XID resource, int resource_type,
                 enum InputLevel level, enum TouchListenerType type,
                 enum TouchListenerState state, WindowPtr window,
                 const GrabPtr grab)
{
    GrabPtr g = NULL;

    /* We need a copy of the grab, not the grab itself since that may be
     * deleted by a UngrabButton request and leaves us with a dangling
     * pointer */
    if (grab)
        g = AllocGrab(grab);

    ti->listeners[ti->num_listeners].listener = resource;
    ti->listeners[ti->num_listeners].resource_type = resource_type;
    ti->listeners[ti->num_listeners].level = level;
    ti->listeners[ti->num_listeners].state = state;
    ti->listeners[ti->num_listeners].type = type;
    ti->listeners[ti->num_listeners].window = window;
    ti->listeners[ti->num_listeners].grab = g;
    if (grab)
        ti->num_grabs++;
    ti->num_listeners++;
}

/**
 * Remove the resource from this touch's listeners.
 *
 * @return TRUE if the resource was removed, FALSE if the resource was not
 * in the list
 */
Bool
TouchRemoveListener(TouchPointInfoPtr ti, XID resource)
{
    int i;

    for (i = 0; i < ti->num_listeners; i++) {
        int j;
        TouchListener *listener = &ti->listeners[i];

        if (listener->listener != resource)
            continue;

        if (listener->grab) {
            FreeGrab(listener->grab);
            listener->grab = NULL;
            ti->num_grabs--;
        }

        for (j = i; j < ti->num_listeners - 1; j++)
            ti->listeners[j] = ti->listeners[j + 1];
        ti->num_listeners--;
        ti->listeners[ti->num_listeners].listener = 0;
        ti->listeners[ti->num_listeners].state = TOUCH_LISTENER_AWAITING_BEGIN;

        return TRUE;
    }
    return FALSE;
}

static void
TouchAddGrabListener(DeviceIntPtr dev, TouchPointInfoPtr ti,
                     InternalEvent *ev, GrabPtr grab)
{
    enum TouchListenerType type = TOUCH_LISTENER_GRAB;

    /* FIXME: owner_events */

    if (grab->grabtype == XI2) {
        if (!xi2mask_isset(grab->xi2mask, dev, XI_TouchOwnership))
            TouchEventHistoryAllocate(ti);
        if (!xi2mask_isset(grab->xi2mask, dev, XI_TouchBegin))
            type = TOUCH_LISTENER_POINTER_GRAB;
    }
    else if (grab->grabtype == XI || grab->grabtype == CORE) {
        TouchEventHistoryAllocate(ti);
        type = TOUCH_LISTENER_POINTER_GRAB;
    }

    /* grab listeners are always RT_NONE since we keep the grab pointer */
    TouchAddListener(ti, grab->resource, RT_NONE, grab->grabtype,
                     type, TOUCH_LISTENER_AWAITING_BEGIN, grab->window, grab);
}

/**
 * Add one listener if there is a grab on the given window.
 */
static void
TouchAddPassiveGrabListener(DeviceIntPtr dev, TouchPointInfoPtr ti,
                            WindowPtr win, InternalEvent *ev)
{
    GrabPtr grab;
    Bool check_core = IsMaster(dev) && ti->emulate_pointer;

    /* FIXME: make CheckPassiveGrabsOnWindow only trigger on TouchBegin */
    grab = CheckPassiveGrabsOnWindow(win, dev, ev, check_core, FALSE);
    if (!grab)
        return;

    TouchAddGrabListener(dev, ti, ev, grab);
}

static Bool
TouchAddRegularListener(DeviceIntPtr dev, TouchPointInfoPtr ti,
                        WindowPtr win, InternalEvent *ev)
{
    InputClients *iclients = NULL;
    OtherInputMasks *inputMasks = NULL;
    uint16_t evtype = 0;        /* may be event type or emulated event type */
    enum TouchListenerType type = TOUCH_LISTENER_REGULAR;
    int mask;

    evtype = GetXI2Type(ev->any.type);
    mask = EventIsDeliverable(dev, ev->any.type, win);
    if (!mask && !ti->emulate_pointer)
        return FALSE;
    else if (!mask) {           /* now try for pointer event */
        mask = EventIsDeliverable(dev, TouchGetPointerEventType(ev), win);
        if (mask) {
            evtype = GetXI2Type(TouchGetPointerEventType(ev));
            type = TOUCH_LISTENER_POINTER_REGULAR;
        }
    }
    if (!mask)
        return FALSE;

    inputMasks = wOtherInputMasks(win);

    if (mask & EVENT_XI2_MASK) {
        nt_list_for_each_entry(iclients, inputMasks->inputClients, next) {
            if (!xi2mask_isset(iclients->xi2mask, dev, evtype))
                continue;

            if (!xi2mask_isset(iclients->xi2mask, dev, XI_TouchOwnership))
                TouchEventHistoryAllocate(ti);

            TouchAddListener(ti, iclients->resource, RT_INPUTCLIENT, XI2,
                             type, TOUCH_LISTENER_AWAITING_BEGIN, win, NULL);
            return TRUE;
        }
    }

    if (mask & EVENT_XI1_MASK) {
        int xitype = GetXIType(TouchGetPointerEventType(ev));
        Mask xi_filter = event_get_filter_from_type(dev, xitype);

        nt_list_for_each_entry(iclients, inputMasks->inputClients, next) {
            if (!(iclients->mask[dev->id] & xi_filter))
                continue;

            TouchEventHistoryAllocate(ti);
            TouchAddListener(ti, iclients->resource, RT_INPUTCLIENT, XI,
                             TOUCH_LISTENER_POINTER_REGULAR,
                             TOUCH_LISTENER_AWAITING_BEGIN,
                             win, NULL);
            return TRUE;
        }
    }

    if (mask & EVENT_CORE_MASK) {
        int coretype = GetCoreType(TouchGetPointerEventType(ev));
        Mask core_filter = event_get_filter_from_type(dev, coretype);
        OtherClients *oclients;

        /* window owner */
        if (IsMaster(dev) && (win->eventMask & core_filter)) {
            TouchEventHistoryAllocate(ti);
            TouchAddListener(ti, win->drawable.id, RT_WINDOW, CORE,
                             TOUCH_LISTENER_POINTER_REGULAR,
                             TOUCH_LISTENER_AWAITING_BEGIN,
                             win, NULL);
            return TRUE;
        }

        /* all others */
        nt_list_for_each_entry(oclients, wOtherClients(win), next) {
            if (!(oclients->mask & core_filter))
                continue;

            TouchEventHistoryAllocate(ti);
            TouchAddListener(ti, oclients->resource, RT_OTHERCLIENT, CORE,
                             type, TOUCH_LISTENER_AWAITING_BEGIN, win, NULL);
            return TRUE;
        }
    }

    return FALSE;
}

static void
TouchAddActiveGrabListener(DeviceIntPtr dev, TouchPointInfoPtr ti,
                           InternalEvent *ev, GrabPtr grab)
{
    if (!ti->emulate_pointer &&
        (grab->grabtype == CORE || grab->grabtype == XI))
        return;

    if (!ti->emulate_pointer &&
        grab->grabtype == XI2 &&
        !xi2mask_isset(grab->xi2mask, dev, XI_TouchBegin))
        return;

    TouchAddGrabListener(dev, ti, ev, grab);
}

void
TouchSetupListeners(DeviceIntPtr dev, TouchPointInfoPtr ti, InternalEvent *ev)
{
    int i;
    SpritePtr sprite = &ti->sprite;
    WindowPtr win;

    if (dev->deviceGrab.grab && !dev->deviceGrab.fromPassiveGrab)
        TouchAddActiveGrabListener(dev, ti, ev, dev->deviceGrab.grab);

    /* We set up an active touch listener for existing touches, but not any
     * passive grab or regular listeners. */
    if (ev->any.type != ET_TouchBegin)
        return;

    /* First, find all grabbing clients from the root window down
     * to the deepest child window. */
    for (i = 0; i < sprite->spriteTraceGood; i++) {
        win = sprite->spriteTrace[i];
        TouchAddPassiveGrabListener(dev, ti, win, ev);
    }

    /* Find the first client with an applicable event selection,
     * going from deepest child window back up to the root window. */
    for (i = sprite->spriteTraceGood - 1; i >= 0; i--) {
        Bool delivered;

        win = sprite->spriteTrace[i];
        delivered = TouchAddRegularListener(dev, ti, win, ev);
        if (delivered)
            return;
    }
}

/**
 * Remove the touch pointer grab from the device. Called from
 * DeactivatePointerGrab()
 */
void
TouchRemovePointerGrab(DeviceIntPtr dev)
{
    TouchPointInfoPtr ti;
    GrabPtr grab;
    InternalEvent *ev;

    if (!dev->touch)
        return;

    grab = dev->deviceGrab.grab;
    if (!grab)
        return;

    ev = dev->deviceGrab.sync.event;
    if (!IsTouchEvent(ev))
        return;

    ti = TouchFindByClientID(dev, ev->device_event.touchid);
    if (!ti)
        return;

    /* FIXME: missing a bit of code here... */
}

/* As touch grabs don't turn into active grabs with their own resources, we
 * need to walk all the touches and remove this grab from any delivery
 * lists. */
void
TouchListenerGone(XID resource)
{
    TouchPointInfoPtr ti;
    DeviceIntPtr dev;
    InternalEvent *events = InitEventList(GetMaximumEventsNum());
    int i, j, k, nev;

    if (!events)
        FatalError("TouchListenerGone: couldn't allocate events\n");

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (!dev->touch)
            continue;

        for (i = 0; i < dev->touch->num_touches; i++) {
            ti = &dev->touch->touches[i];
            if (!ti->active)
                continue;

            for (j = 0; j < ti->num_listeners; j++) {
                if (CLIENT_BITS(ti->listeners[j].listener) != resource)
                    continue;

                nev = GetTouchOwnershipEvents(events, dev, ti, XIRejectTouch,
                                              ti->listeners[j].listener, 0);
                for (k = 0; k < nev; k++)
                    mieqProcessDeviceEvent(dev, events + k, NULL);

                break;
            }
        }
    }

    FreeEventList(events, GetMaximumEventsNum());
}

int
TouchListenerAcceptReject(DeviceIntPtr dev, TouchPointInfoPtr ti, int listener,
                          int mode)
{
    InternalEvent *events;
    int nev;
    int i;

    BUG_RETURN_VAL(listener < 0, BadMatch);
    BUG_RETURN_VAL(listener >= ti->num_listeners, BadMatch);

    if (listener > 0) {
        if (mode == XIRejectTouch)
            TouchRejected(dev, ti, ti->listeners[listener].listener, NULL);
        else
            ti->listeners[listener].state = TOUCH_LISTENER_EARLY_ACCEPT;

        return Success;
    }

    events = InitEventList(GetMaximumEventsNum());
    BUG_RETURN_VAL_MSG(!events, BadAlloc, "Failed to allocate touch ownership events\n");

    nev = GetTouchOwnershipEvents(events, dev, ti, mode,
                                  ti->listeners[0].listener, 0);
    BUG_WARN_MSG(nev == 0, "Failed to get touch ownership events\n");

    for (i = 0; i < nev; i++)
        mieqProcessDeviceEvent(dev, events + i, NULL);

    FreeEventList(events, GetMaximumEventsNum());

    return nev ? Success : BadMatch;
}

int
TouchAcceptReject(ClientPtr client, DeviceIntPtr dev, int mode,
                  uint32_t touchid, Window grab_window, XID *error)
{
    TouchPointInfoPtr ti;
    int i;

    if (!dev->touch) {
        *error = dev->id;
        return BadDevice;
    }

    ti = TouchFindByClientID(dev, touchid);
    if (!ti) {
        *error = touchid;
        return BadValue;
    }

    for (i = 0; i < ti->num_listeners; i++) {
        if (CLIENT_ID(ti->listeners[i].listener) == client->index &&
            ti->listeners[i].window->drawable.id == grab_window)
            break;
    }
    if (i == ti->num_listeners)
        return BadAccess;

    return TouchListenerAcceptReject(dev, ti, i, mode);
}

/**
 * End physically active touches for a device.
 */
void
TouchEndPhysicallyActiveTouches(DeviceIntPtr dev)
{
    InternalEvent *eventlist = InitEventList(GetMaximumEventsNum());
    int i;

    input_lock();
    mieqProcessInputEvents();
    for (i = 0; i < dev->last.num_touches; i++) {
        DDXTouchPointInfoPtr ddxti = dev->last.touches + i;

        if (ddxti->active) {
            int j;
            int nevents = GetTouchEvents(eventlist, dev, ddxti->ddx_id,
                                         XI_TouchEnd, 0, NULL);

            for (j = 0; j < nevents; j++)
                mieqProcessDeviceEvent(dev, eventlist + j, NULL);
        }
    }
    input_unlock();

    FreeEventList(eventlist, GetMaximumEventsNum());
}

/**
 * Generate and deliver a TouchEnd event.
 *
 * @param dev The device to deliver the event for.
 * @param ti The touch point record to deliver the event for.
 * @param flags Internal event flags. The called does not need to provide
 *        TOUCH_CLIENT_ID and TOUCH_POINTER_EMULATED, this function will ensure
 *        they are set appropriately.
 * @param resource The client resource to deliver to, or 0 for all clients.
 */
void
TouchEmitTouchEnd(DeviceIntPtr dev, TouchPointInfoPtr ti, int flags, XID resource)
{
    InternalEvent event;

    /* We're not processing a touch end for a frozen device */
    if (dev->deviceGrab.sync.frozen)
        return;

    flags |= TOUCH_CLIENT_ID;
    if (ti->emulate_pointer)
        flags |= TOUCH_POINTER_EMULATED;
    DeliverDeviceClassesChangedEvent(ti->sourceid, GetTimeInMillis());
    GetDixTouchEnd(&event, dev, ti, flags);
    DeliverTouchEvents(dev, ti, &event, resource);
    if (ti->num_grabs == 0)
        UpdateDeviceState(dev, &event.device_event);
}

void
TouchAcceptAndEnd(DeviceIntPtr dev, int touchid)
{
    TouchPointInfoPtr ti = TouchFindByClientID(dev, touchid);
    if (!ti)
        return;

    TouchListenerAcceptReject(dev, ti, 0, XIAcceptTouch);
    if (ti->pending_finish)
        TouchEmitTouchEnd(dev, ti, 0, 0);
    if (ti->num_listeners <= 1)
        TouchEndTouch(dev, ti);
}
