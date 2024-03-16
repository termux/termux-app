/*
 * Copyright © 2008 Red Hat, Inc.
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
 * Authors: Peter Hutterer
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI2proto.h>
#include "inputstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "exglobals.h"
#include "enterleave.h"
#include "eventconvert.h"
#include "xkbsrv.h"
#include "inpututils.h"

/**
 * @file
 * This file describes the model for sending core enter/leave events and
 * focus in/out in the case of multiple pointers/keyboard foci.
 *
 * Since we can't send more than one Enter or Leave/Focus in or out event per
 * window to a core client without confusing it, this is a rather complicated
 * approach.
 *
 * For a full description of the enter/leave model from a window's
 * perspective, see
 * http://lists.freedesktop.org/archives/xorg/2008-August/037606.html
 *
 * For a full description of the focus in/out model from a window's
 * perspective, see
 * https://lists.freedesktop.org/archives/xorg/2008-December/041684.html
 *
 * Additional notes:
 * - The core protocol spec says that "In a LeaveNotify event, if a child of the
 * event window contains the initial position of the pointer, then the child
 * component is set to that child. Otherwise, it is None.  For an EnterNotify
 * event, if a child of the event window contains the final pointer position,
 * then the child component is set to that child. Otherwise, it is None."
 *
 * By inference, this means that only NotifyVirtual or NotifyNonlinearVirtual
 * events may have a subwindow set to other than None.
 *
 * - NotifyPointer events may be sent if the focus changes from window A to
 * B. The assumption used in this model is that NotifyPointer events are only
 * sent for the pointer paired with the keyboard that is involved in the focus
 * events. For example, if F(W) changes because of keyboard 2, then
 * NotifyPointer events are only sent for pointer 2.
 */

static WindowPtr PointerWindows[MAXDEVICES];
static WindowPtr FocusWindows[MAXDEVICES];

/**
 * Return TRUE if 'win' has a pointer within its boundaries, excluding child
 * window.
 */
static BOOL
HasPointer(DeviceIntPtr dev, WindowPtr win)
{
    int i;

    /* FIXME: The enter/leave model does not cater for grabbed devices. For
     * now, a quickfix: if the device about to send an enter/leave event to
     * a window is grabbed, assume there is no pointer in that window.
     * Fixes fdo 27804.
     * There isn't enough beer in my fridge to fix this properly.
     */
    if (dev->deviceGrab.grab)
        return FALSE;

    for (i = 0; i < MAXDEVICES; i++)
        if (PointerWindows[i] == win)
            return TRUE;

    return FALSE;
}

/**
 * Return TRUE if at least one keyboard focus is set to 'win' (excluding
 * descendants of win).
 */
static BOOL
HasFocus(WindowPtr win)
{
    int i;

    for (i = 0; i < MAXDEVICES; i++)
        if (FocusWindows[i] == win)
            return TRUE;

    return FALSE;
}

/**
 * Return the window the device dev is currently on.
 */
static WindowPtr
PointerWin(DeviceIntPtr dev)
{
    return PointerWindows[dev->id];
}

/**
 * Search for the first window below 'win' that has a pointer directly within
 * its boundaries (excluding boundaries of its own descendants).
 *
 * @return The child window that has the pointer within its boundaries or
 *         NULL.
 */
static WindowPtr
FirstPointerChild(WindowPtr win)
{
    int i;

    for (i = 0; i < MAXDEVICES; i++) {
        if (PointerWindows[i] && IsParent(win, PointerWindows[i]))
            return PointerWindows[i];
    }

    return NULL;
}

/**
 * Search for the first window below 'win' that has a focus directly within
 * its boundaries (excluding boundaries of its own descendants).
 *
 * @return The child window that has the pointer within its boundaries or
 *         NULL.
 */
static WindowPtr
FirstFocusChild(WindowPtr win)
{
    int i;

    for (i = 0; i < MAXDEVICES; i++) {
        if (FocusWindows[i] && FocusWindows[i] != PointerRootWin &&
            IsParent(win, FocusWindows[i]))
            return FocusWindows[i];
    }

    return NULL;
}

/**
 * Set the presence flag for dev to mark that it is now in 'win'.
 */
void
EnterWindow(DeviceIntPtr dev, WindowPtr win, int mode)
{
    PointerWindows[dev->id] = win;
}

/**
 * Unset the presence flag for dev to mark that it is not in 'win' anymore.
 */
void
LeaveWindow(DeviceIntPtr dev)
{
    PointerWindows[dev->id] = NULL;
}

/**
 * Set the presence flag for dev to mark that it is now in 'win'.
 */
void
SetFocusIn(DeviceIntPtr dev, WindowPtr win)
{
    FocusWindows[dev->id] = win;
}

/**
 * Unset the presence flag for dev to mark that it is not in 'win' anymore.
 */
void
SetFocusOut(DeviceIntPtr dev)
{
    FocusWindows[dev->id] = NULL;
}

/**
 * Return the common ancestor of 'a' and 'b' (if one exists).
 * @param a A window with the same ancestor as b.
 * @param b A window with the same ancestor as a.
 * @return The window that is the first ancestor of both 'a' and 'b', or the
 *         NullWindow if they do not have a common ancestor.
 */
static WindowPtr
CommonAncestor(WindowPtr a, WindowPtr b)
{
    for (b = b->parent; b; b = b->parent)
        if (IsParent(b, a))
            return b;
    return NullWindow;
}

/**
 * Send enter notifies to all windows between 'ancestor' and 'child' (excluding
 * both). Events are sent running up the window hierarchy. This function
 * recurses.
 */
static void
DeviceEnterNotifies(DeviceIntPtr dev,
                    int sourceid,
                    WindowPtr ancestor, WindowPtr child, int mode, int detail)
{
    WindowPtr parent = child->parent;

    if (ancestor == parent)
        return;
    DeviceEnterNotifies(dev, sourceid, ancestor, parent, mode, detail);
    DeviceEnterLeaveEvent(dev, sourceid, XI_Enter, mode, detail, parent,
                          child->drawable.id);
}

/**
 * Send enter notifies to all windows between 'ancestor' and 'child' (excluding
 * both). Events are sent running down the window hierarchy. This function
 * recurses.
 */
static void
CoreEnterNotifies(DeviceIntPtr dev,
                  WindowPtr ancestor, WindowPtr child, int mode, int detail)
{
    WindowPtr parent = child->parent;

    if (ancestor == parent)
        return;
    CoreEnterNotifies(dev, ancestor, parent, mode, detail);

    /* Case 3:
       A is above W, B is a descendant

       Classically: The move generates an EnterNotify on W with a detail of
       Virtual or NonlinearVirtual

       MPX:
       Case 3A: There is at least one other pointer on W itself
       P(W) doesn't change, so the event should be suppressed
       Case 3B: Otherwise, if there is at least one other pointer in a
       descendant
       P(W) stays on the same descendant, or changes to a different
       descendant. The event should be suppressed.
       Case 3C: Otherwise:
       P(W) moves from a window above W to a descendant. The subwindow
       field is set to the child containing the descendant. The detail
       may need to be changed from Virtual to NonlinearVirtual depending
       on the previous P(W). */

    if (!HasPointer(dev, parent) && !FirstPointerChild(parent))
        CoreEnterLeaveEvent(dev, EnterNotify, mode, detail, parent,
                            child->drawable.id);
}

static void
CoreLeaveNotifies(DeviceIntPtr dev,
                  WindowPtr child, WindowPtr ancestor, int mode, int detail)
{
    WindowPtr win;

    if (ancestor == child)
        return;

    for (win = child->parent; win != ancestor; win = win->parent) {
        /*Case 7:
           A is a descendant of W, B is above W

           Classically: A LeaveNotify is generated on W with a detail of Virtual
           or NonlinearVirtual.

           MPX:
           Case 3A: There is at least one other pointer on W itself
           P(W) doesn't change, the event should be suppressed.
           Case 3B: Otherwise, if there is at least one other pointer in a
           descendant
           P(W) stays on the same descendant, or changes to a different
           descendant. The event should be suppressed.
           Case 3C: Otherwise:
           P(W) changes from the descendant of W to a window above W.
           The detail may need to be changed from Virtual to NonlinearVirtual
           or vice-versa depending on the new P(W). */

        /* If one window has a pointer or a child with a pointer, skip some
         * work and exit. */
        if (HasPointer(dev, win) || FirstPointerChild(win))
            return;

        CoreEnterLeaveEvent(dev, LeaveNotify, mode, detail, win,
                            child->drawable.id);

        child = win;
    }
}

/**
 * Send leave notifies to all windows between 'child' and 'ancestor'.
 * Events are sent running up the hierarchy.
 */
static void
DeviceLeaveNotifies(DeviceIntPtr dev,
                    int sourceid,
                    WindowPtr child, WindowPtr ancestor, int mode, int detail)
{
    WindowPtr win;

    if (ancestor == child)
        return;
    for (win = child->parent; win != ancestor; win = win->parent) {
        DeviceEnterLeaveEvent(dev, sourceid, XI_Leave, mode, detail, win,
                              child->drawable.id);
        child = win;
    }
}

/**
 * Pointer dev moves from A to B and A neither a descendant of B nor is
 * B a descendant of A.
 */
static void
CoreEnterLeaveNonLinear(DeviceIntPtr dev, WindowPtr A, WindowPtr B, int mode)
{
    WindowPtr X = CommonAncestor(A, B);

    /* Case 4:
       A is W, B is above W

       Classically: The move generates a LeaveNotify on W with a detail of
       Ancestor or Nonlinear

       MPX:
       Case 3A: There is at least one other pointer on W itself
       P(W) doesn't change, the event should be suppressed
       Case 3B: Otherwise, if there is at least one other pointer in a
       descendant of W
       P(W) changes from W to a descendant of W. The subwindow field
       is set to the child containing the new P(W), the detail field
       is set to Inferior
       Case 3C: Otherwise:
       The pointer window moves from W to a window above W.
       The detail may need to be changed from Ancestor to Nonlinear or
       vice versa depending on the the new P(W)
     */

    if (!HasPointer(dev, A)) {
        WindowPtr child = FirstPointerChild(A);

        if (child)
            CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyInferior, A,
                                None);
        else
            CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyNonlinear, A,
                                None);
    }

    CoreLeaveNotifies(dev, A, X, mode, NotifyNonlinearVirtual);

    /*
       Case 9:
       A is a descendant of W, B is a descendant of W

       Classically: No events are generated on W
       MPX: The pointer window stays the same or moves to a different
       descendant of W. No events should be generated on W.

       Therefore, no event to X.
     */

    CoreEnterNotifies(dev, X, B, mode, NotifyNonlinearVirtual);

    /* Case 2:
       A is above W, B=W

       Classically: The move generates an EnterNotify on W with a detail of
       Ancestor or Nonlinear

       MPX:
       Case 2A: There is at least one other pointer on W itself
       P(W) doesn't change, so the event should be suppressed
       Case 2B: Otherwise, if there is at least one other pointer in a
       descendant
       P(W) moves from a descendant to W. detail is changed to Inferior,
       subwindow is set to the child containing the previous P(W)
       Case 2C: Otherwise:
       P(W) changes from a window above W to W itself.
       The detail may need to be changed from Ancestor to Nonlinear
       or vice-versa depending on the previous P(W). */

    if (!HasPointer(dev, B)) {
        WindowPtr child = FirstPointerChild(B);

        if (child)
            CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyInferior, B,
                                None);
        else
            CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyNonlinear, B,
                                None);
    }
}

/**
 * Pointer dev moves from A to B and A is a descendant of B.
 */
static void
CoreEnterLeaveToAncestor(DeviceIntPtr dev, WindowPtr A, WindowPtr B, int mode)
{
    /* Case 4:
       A is W, B is above W

       Classically: The move generates a LeaveNotify on W with a detail of
       Ancestor or Nonlinear

       MPX:
       Case 3A: There is at least one other pointer on W itself
       P(W) doesn't change, the event should be suppressed
       Case 3B: Otherwise, if there is at least one other pointer in a
       descendant of W
       P(W) changes from W to a descendant of W. The subwindow field
       is set to the child containing the new P(W), the detail field
       is set to Inferior
       Case 3C: Otherwise:
       The pointer window moves from W to a window above W.
       The detail may need to be changed from Ancestor to Nonlinear or
       vice versa depending on the the new P(W)
     */
    if (!HasPointer(dev, A)) {
        WindowPtr child = FirstPointerChild(A);

        if (child)
            CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyInferior, A,
                                None);
        else
            CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyAncestor, A,
                                None);
    }

    CoreLeaveNotifies(dev, A, B, mode, NotifyVirtual);

    /* Case 8:
       A is a descendant of W, B is W

       Classically: A EnterNotify is generated on W with a detail of
       NotifyInferior

       MPX:
       Case 3A: There is at least one other pointer on W itself
       P(W) doesn't change, the event should be suppressed
       Case 3B: Otherwise:
       P(W) changes from a descendant to W itself. The subwindow
       field should be set to the child containing the old P(W) <<< WRONG */

    if (!HasPointer(dev, B))
        CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyInferior, B, None);

}

/**
 * Pointer dev moves from A to B and B is a descendant of A.
 */
static void
CoreEnterLeaveToDescendant(DeviceIntPtr dev, WindowPtr A, WindowPtr B, int mode)
{
    /* Case 6:
       A is W, B is a descendant of W

       Classically: A LeaveNotify is generated on W with a detail of
       NotifyInferior

       MPX:
       Case 3A: There is at least one other pointer on W itself
       P(W) doesn't change, the event should be suppressed
       Case 3B: Otherwise:
       P(W) changes from W to a descendant of W. The subwindow field
       is set to the child containing the new P(W) <<< THIS IS WRONG */

    if (!HasPointer(dev, A))
        CoreEnterLeaveEvent(dev, LeaveNotify, mode, NotifyInferior, A, None);

    CoreEnterNotifies(dev, A, B, mode, NotifyVirtual);

    /* Case 2:
       A is above W, B=W

       Classically: The move generates an EnterNotify on W with a detail of
       Ancestor or Nonlinear

       MPX:
       Case 2A: There is at least one other pointer on W itself
       P(W) doesn't change, so the event should be suppressed
       Case 2B: Otherwise, if there is at least one other pointer in a
       descendant
       P(W) moves from a descendant to W. detail is changed to Inferior,
       subwindow is set to the child containing the previous P(W)
       Case 2C: Otherwise:
       P(W) changes from a window above W to W itself.
       The detail may need to be changed from Ancestor to Nonlinear
       or vice-versa depending on the previous P(W). */

    if (!HasPointer(dev, B)) {
        WindowPtr child = FirstPointerChild(B);

        if (child)
            CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyInferior, B,
                                None);
        else
            CoreEnterLeaveEvent(dev, EnterNotify, mode, NotifyAncestor, B,
                                None);
    }
}

static void
CoreEnterLeaveEvents(DeviceIntPtr dev, WindowPtr from, WindowPtr to, int mode)
{
    if (!IsMaster(dev))
        return;

    LeaveWindow(dev);

    if (IsParent(from, to))
        CoreEnterLeaveToDescendant(dev, from, to, mode);
    else if (IsParent(to, from))
        CoreEnterLeaveToAncestor(dev, from, to, mode);
    else
        CoreEnterLeaveNonLinear(dev, from, to, mode);

    EnterWindow(dev, to, mode);
}

static void
DeviceEnterLeaveEvents(DeviceIntPtr dev,
                       int sourceid, WindowPtr from, WindowPtr to, int mode)
{
    if (IsParent(from, to)) {
        DeviceEnterLeaveEvent(dev, sourceid, XI_Leave, mode, NotifyInferior,
                              from, None);
        DeviceEnterNotifies(dev, sourceid, from, to, mode, NotifyVirtual);
        DeviceEnterLeaveEvent(dev, sourceid, XI_Enter, mode, NotifyAncestor, to,
                              None);
    }
    else if (IsParent(to, from)) {
        DeviceEnterLeaveEvent(dev, sourceid, XI_Leave, mode, NotifyAncestor,
                              from, None);
        DeviceLeaveNotifies(dev, sourceid, from, to, mode, NotifyVirtual);
        DeviceEnterLeaveEvent(dev, sourceid, XI_Enter, mode, NotifyInferior, to,
                              None);
    }
    else {                      /* neither from nor to is descendent of the other */
        WindowPtr common = CommonAncestor(to, from);

        /* common == NullWindow ==> different screens */
        DeviceEnterLeaveEvent(dev, sourceid, XI_Leave, mode, NotifyNonlinear,
                              from, None);
        DeviceLeaveNotifies(dev, sourceid, from, common, mode,
                            NotifyNonlinearVirtual);
        DeviceEnterNotifies(dev, sourceid, common, to, mode,
                            NotifyNonlinearVirtual);
        DeviceEnterLeaveEvent(dev, sourceid, XI_Enter, mode, NotifyNonlinear,
                              to, None);
    }
}

/**
 * Figure out if enter/leave events are necessary and send them to the
 * appropriate windows.
 *
 * @param fromWin Window the sprite moved out of.
 * @param toWin Window the sprite moved into.
 */
void
DoEnterLeaveEvents(DeviceIntPtr pDev,
                   int sourceid, WindowPtr fromWin, WindowPtr toWin, int mode)
{
    if (!IsPointerDevice(pDev))
        return;

    if (fromWin == toWin)
        return;

    if (mode != XINotifyPassiveGrab && mode != XINotifyPassiveUngrab)
        CoreEnterLeaveEvents(pDev, fromWin, toWin, mode);
    DeviceEnterLeaveEvents(pDev, sourceid, fromWin, toWin, mode);
}

static void
FixDeviceValuator(DeviceIntPtr dev, deviceValuator * ev, ValuatorClassPtr v,
                  int first)
{
    int nval = v->numAxes - first;

    ev->type = DeviceValuator;
    ev->deviceid = dev->id;
    ev->num_valuators = nval < 3 ? nval : 3;
    ev->first_valuator = first;
    switch (ev->num_valuators) {
    case 3:
        ev->valuator2 = v->axisVal[first + 2];
    case 2:
        ev->valuator1 = v->axisVal[first + 1];
    case 1:
        ev->valuator0 = v->axisVal[first];
        break;
    }
    first += ev->num_valuators;
}

static void
FixDeviceStateNotify(DeviceIntPtr dev, deviceStateNotify * ev, KeyClassPtr k,
                     ButtonClassPtr b, ValuatorClassPtr v, int first)
{
    ev->type = DeviceStateNotify;
    ev->deviceid = dev->id;
    ev->time = currentTime.milliseconds;
    ev->classes_reported = 0;
    ev->num_keys = 0;
    ev->num_buttons = 0;
    ev->num_valuators = 0;

    if (b) {
        ev->classes_reported |= (1 << ButtonClass);
        ev->num_buttons = b->numButtons;
        memcpy((char *) ev->buttons, (char *) b->down, 4);
    }
    else if (k) {
        ev->classes_reported |= (1 << KeyClass);
        ev->num_keys = k->xkbInfo->desc->max_key_code -
            k->xkbInfo->desc->min_key_code;
        memmove((char *) &ev->keys[0], (char *) k->down, 4);
    }
    if (v) {
        int nval = v->numAxes - first;

        ev->classes_reported |= (1 << ValuatorClass);
        ev->classes_reported |= valuator_get_mode(dev, 0) << ModeBitsShift;
        ev->num_valuators = nval < 3 ? nval : 3;
        switch (ev->num_valuators) {
        case 3:
            ev->valuator2 = v->axisVal[first + 2];
        case 2:
            ev->valuator1 = v->axisVal[first + 1];
        case 1:
            ev->valuator0 = v->axisVal[first];
            break;
        }
    }
}


static void
DeliverStateNotifyEvent(DeviceIntPtr dev, WindowPtr win)
{
    int evcount = 1;
    deviceStateNotify *ev, *sev;
    deviceKeyStateNotify *kev;
    deviceButtonStateNotify *bev;

    KeyClassPtr k;
    ButtonClassPtr b;
    ValuatorClassPtr v;
    int nval = 0, nkeys = 0, nbuttons = 0, first = 0;

    if (!(wOtherInputMasks(win)) ||
        !(wOtherInputMasks(win)->inputEvents[dev->id] & DeviceStateNotifyMask))
        return;

    if ((b = dev->button) != NULL) {
        nbuttons = b->numButtons;
        if (nbuttons > 32)
            evcount++;
    }
    if ((k = dev->key) != NULL) {
        nkeys = k->xkbInfo->desc->max_key_code - k->xkbInfo->desc->min_key_code;
        if (nkeys > 32)
            evcount++;
        if (nbuttons > 0) {
            evcount++;
        }
    }
    if ((v = dev->valuator) != NULL) {
        nval = v->numAxes;

        if (nval > 3)
            evcount++;
        if (nval > 6) {
            if (!(k && b))
                evcount++;
            if (nval > 9)
                evcount += ((nval - 7) / 3);
        }
    }

    sev = ev = xallocarray(evcount, sizeof(xEvent));
    FixDeviceStateNotify(dev, ev, NULL, NULL, NULL, first);

    if (b != NULL) {
        FixDeviceStateNotify(dev, ev++, NULL, b, v, first);
        first += 3;
        nval -= 3;
        if (nbuttons > 32) {
            (ev - 1)->deviceid |= MORE_EVENTS;
            bev = (deviceButtonStateNotify *) ev++;
            bev->type = DeviceButtonStateNotify;
            bev->deviceid = dev->id;
            memcpy((char *) &bev->buttons[4], (char *) &b->down[4],
                   DOWN_LENGTH - 4);
        }
        if (nval > 0) {
            (ev - 1)->deviceid |= MORE_EVENTS;
            FixDeviceValuator(dev, (deviceValuator *) ev++, v, first);
            first += 3;
            nval -= 3;
        }
    }

    if (k != NULL) {
        FixDeviceStateNotify(dev, ev++, k, NULL, v, first);
        first += 3;
        nval -= 3;
        if (nkeys > 32) {
            (ev - 1)->deviceid |= MORE_EVENTS;
            kev = (deviceKeyStateNotify *) ev++;
            kev->type = DeviceKeyStateNotify;
            kev->deviceid = dev->id;
            memmove((char *) &kev->keys[0], (char *) &k->down[4], 28);
        }
        if (nval > 0) {
            (ev - 1)->deviceid |= MORE_EVENTS;
            FixDeviceValuator(dev, (deviceValuator *) ev++, v, first);
            first += 3;
            nval -= 3;
        }
    }

    while (nval > 0) {
        FixDeviceStateNotify(dev, ev++, NULL, NULL, v, first);
        first += 3;
        nval -= 3;
        if (nval > 0) {
            (ev - 1)->deviceid |= MORE_EVENTS;
            FixDeviceValuator(dev, (deviceValuator *) ev++, v, first);
            first += 3;
            nval -= 3;
        }
    }

    DeliverEventsToWindow(dev, win, (xEvent *) sev, evcount,
                          DeviceStateNotifyMask, NullGrab);
    free(sev);
}

void
DeviceFocusEvent(DeviceIntPtr dev, int type, int mode, int detail,
                 WindowPtr pWin)
{
    deviceFocus event;
    xXIFocusInEvent *xi2event;
    DeviceIntPtr mouse;
    int btlen, len, i;

    mouse = IsFloating(dev) ? dev : GetMaster(dev, MASTER_POINTER);

    /* XI 2 event */
    btlen = (mouse->button) ? bits_to_bytes(mouse->button->numButtons) : 0;
    btlen = bytes_to_int32(btlen);
    len = sizeof(xXIFocusInEvent) + btlen * 4;

    xi2event = calloc(1, len);
    xi2event->type = GenericEvent;
    xi2event->extension = IReqCode;
    xi2event->evtype = type;
    xi2event->length = bytes_to_int32(len - sizeof(xEvent));
    xi2event->buttons_len = btlen;
    xi2event->detail = detail;
    xi2event->time = currentTime.milliseconds;
    xi2event->deviceid = dev->id;
    xi2event->sourceid = dev->id;       /* a device doesn't change focus by itself */
    xi2event->mode = mode;
    xi2event->root_x = double_to_fp1616(mouse->spriteInfo->sprite->hot.x);
    xi2event->root_y = double_to_fp1616(mouse->spriteInfo->sprite->hot.y);

    for (i = 0; mouse && mouse->button && i < mouse->button->numButtons; i++)
        if (BitIsOn(mouse->button->down, i))
            SetBit(&xi2event[1], mouse->button->map[i]);

    if (dev->key) {
        xi2event->mods.base_mods = dev->key->xkbInfo->state.base_mods;
        xi2event->mods.latched_mods = dev->key->xkbInfo->state.latched_mods;
        xi2event->mods.locked_mods = dev->key->xkbInfo->state.locked_mods;
        xi2event->mods.effective_mods = dev->key->xkbInfo->state.mods;

        xi2event->group.base_group = dev->key->xkbInfo->state.base_group;
        xi2event->group.latched_group = dev->key->xkbInfo->state.latched_group;
        xi2event->group.locked_group = dev->key->xkbInfo->state.locked_group;
        xi2event->group.effective_group = dev->key->xkbInfo->state.group;
    }

    FixUpEventFromWindow(dev->spriteInfo->sprite, (xEvent *) xi2event, pWin,
                         None, FALSE);

    DeliverEventsToWindow(dev, pWin, (xEvent *) xi2event, 1,
                          GetEventFilter(dev, (xEvent *) xi2event), NullGrab);

    free(xi2event);

    /* XI 1.x event */
    event = (deviceFocus) {
        .deviceid = dev->id,
        .mode = mode,
        .type = (type == XI_FocusIn) ? DeviceFocusIn : DeviceFocusOut,
        .detail = detail,
        .window = pWin->drawable.id,
        .time = currentTime.milliseconds
    };

    DeliverEventsToWindow(dev, pWin, (xEvent *) &event, 1,
                          DeviceFocusChangeMask, NullGrab);

    if (event.type == DeviceFocusIn)
        DeliverStateNotifyEvent(dev, pWin);
}

/**
 * Send focus out events to all windows between 'child' and 'ancestor'.
 * Events are sent running up the hierarchy.
 */
static void
DeviceFocusOutEvents(DeviceIntPtr dev,
                     WindowPtr child, WindowPtr ancestor, int mode, int detail)
{
    WindowPtr win;

    if (ancestor == child)
        return;
    for (win = child->parent; win != ancestor; win = win->parent)
        DeviceFocusEvent(dev, XI_FocusOut, mode, detail, win);
}

/**
 * Send enter notifies to all windows between 'ancestor' and 'child' (excluding
 * both). Events are sent running up the window hierarchy. This function
 * recurses.
 */
static void
DeviceFocusInEvents(DeviceIntPtr dev,
                    WindowPtr ancestor, WindowPtr child, int mode, int detail)
{
    WindowPtr parent = child->parent;

    if (ancestor == parent || !parent)
        return;
    DeviceFocusInEvents(dev, ancestor, parent, mode, detail);
    DeviceFocusEvent(dev, XI_FocusIn, mode, detail, parent);
}

/**
 * Send FocusIn events to all windows between 'ancestor' and 'child' (excluding
 * both). Events are sent running down the window hierarchy. This function
 * recurses.
 */
static void
CoreFocusInEvents(DeviceIntPtr dev,
                  WindowPtr ancestor, WindowPtr child, int mode, int detail)
{
    WindowPtr parent = child->parent;

    if (ancestor == parent)
        return;
    CoreFocusInEvents(dev, ancestor, parent, mode, detail);

    /* Case 3:
       A is above W, B is a descendant

       Classically: The move generates an FocusIn on W with a detail of
       Virtual or NonlinearVirtual

       MPX:
       Case 3A: There is at least one other focus on W itself
       F(W) doesn't change, so the event should be suppressed
       Case 3B: Otherwise, if there is at least one other focus in a
       descendant
       F(W) stays on the same descendant, or changes to a different
       descendant. The event should be suppressed.
       Case 3C: Otherwise:
       F(W) moves from a window above W to a descendant. The detail may
       need to be changed from Virtual to NonlinearVirtual depending
       on the previous F(W). */

    if (!HasFocus(parent) && !FirstFocusChild(parent))
        CoreFocusEvent(dev, FocusIn, mode, detail, parent);
}

static void
CoreFocusOutEvents(DeviceIntPtr dev,
                   WindowPtr child, WindowPtr ancestor, int mode, int detail)
{
    WindowPtr win;

    if (ancestor == child)
        return;

    for (win = child->parent; win != ancestor; win = win->parent) {
        /*Case 7:
           A is a descendant of W, B is above W

           Classically: A FocusOut is generated on W with a detail of Virtual
           or NonlinearVirtual.

           MPX:
           Case 3A: There is at least one other focus on W itself
           F(W) doesn't change, the event should be suppressed.
           Case 3B: Otherwise, if there is at least one other focus in a
           descendant
           F(W) stays on the same descendant, or changes to a different
           descendant. The event should be suppressed.
           Case 3C: Otherwise:
           F(W) changes from the descendant of W to a window above W.
           The detail may need to be changed from Virtual to NonlinearVirtual
           or vice-versa depending on the new P(W). */

        /* If one window has a focus or a child with a focuspointer, skip some
         * work and exit. */
        if (HasFocus(win) || FirstFocusChild(win))
            return;

        CoreFocusEvent(dev, FocusOut, mode, detail, win);
    }
}

/**
 * Send FocusOut(NotifyPointer) events from the current pointer window (which
 * is a descendant of pwin_parent) up to (excluding) pwin_parent.
 *
 * NotifyPointer events are only sent for the device paired with dev.
 *
 * If the current pointer window is a descendant of 'exclude' or an ancestor of
 * 'exclude', no events are sent. If the current pointer IS 'exclude', events
 * are sent!
 */
static void
CoreFocusOutNotifyPointerEvents(DeviceIntPtr dev,
                                WindowPtr pwin_parent,
                                WindowPtr exclude, int mode, int inclusive)
{
    WindowPtr P, stopAt;

    P = PointerWin(GetMaster(dev, POINTER_OR_FLOAT));

    if (!P)
        return;
    if (!IsParent(pwin_parent, P))
        if (!(pwin_parent == P && inclusive))
            return;

    if (exclude != None && exclude != PointerRootWin &&
        (IsParent(exclude, P) || IsParent(P, exclude)))
        return;

    stopAt = (inclusive) ? pwin_parent->parent : pwin_parent;

    for (; P && P != stopAt; P = P->parent)
        CoreFocusEvent(dev, FocusOut, mode, NotifyPointer, P);
}

/**
 * DO NOT CALL DIRECTLY.
 * Recursion helper for CoreFocusInNotifyPointerEvents.
 */
static void
CoreFocusInRecurse(DeviceIntPtr dev,
                   WindowPtr win, WindowPtr stopAt, int mode, int inclusive)
{
    if ((!inclusive && win == stopAt) || !win)
        return;

    CoreFocusInRecurse(dev, win->parent, stopAt, mode, inclusive);
    CoreFocusEvent(dev, FocusIn, mode, NotifyPointer, win);
}

/**
 * Send FocusIn(NotifyPointer) events from pwin_parent down to
 * including the current pointer window (which is a descendant of pwin_parent).
 *
 * @param pwin The pointer window.
 * @param exclude If the pointer window is a child of 'exclude', no events are
 *                sent.
 * @param inclusive If TRUE, pwin_parent will receive the event too.
 */
static void
CoreFocusInNotifyPointerEvents(DeviceIntPtr dev,
                               WindowPtr pwin_parent,
                               WindowPtr exclude, int mode, int inclusive)
{
    WindowPtr P;

    P = PointerWin(GetMaster(dev, POINTER_OR_FLOAT));

    if (!P || P == exclude || (pwin_parent != P && !IsParent(pwin_parent, P)))
        return;

    if (exclude != None && (IsParent(exclude, P) || IsParent(P, exclude)))
        return;

    CoreFocusInRecurse(dev, P, pwin_parent, mode, inclusive);
}

/**
 * Focus of dev moves from A to B and A neither a descendant of B nor is
 * B a descendant of A.
 */
static void
CoreFocusNonLinear(DeviceIntPtr dev, WindowPtr A, WindowPtr B, int mode)
{
    WindowPtr X = CommonAncestor(A, B);

    /* Case 4:
       A is W, B is above W

       Classically: The change generates a FocusOut on W with a detail of
       Ancestor or Nonlinear

       MPX:
       Case 3A: There is at least one other focus on W itself
       F(W) doesn't change, the event should be suppressed
       Case 3B: Otherwise, if there is at least one other focus in a
       descendant of W
       F(W) changes from W to a descendant of W. The detail field
       is set to Inferior
       Case 3C: Otherwise:
       The focus window moves from W to a window above W.
       The detail may need to be changed from Ancestor to Nonlinear or
       vice versa depending on the the new F(W)
     */

    if (!HasFocus(A)) {
        WindowPtr child = FirstFocusChild(A);

        if (child) {
            /* NotifyPointer P-A unless P is child or below */
            CoreFocusOutNotifyPointerEvents(dev, A, child, mode, FALSE);
            CoreFocusEvent(dev, FocusOut, mode, NotifyInferior, A);
        }
        else {
            /* NotifyPointer P-A */
            CoreFocusOutNotifyPointerEvents(dev, A, None, mode, FALSE);
            CoreFocusEvent(dev, FocusOut, mode, NotifyNonlinear, A);
        }
    }

    CoreFocusOutEvents(dev, A, X, mode, NotifyNonlinearVirtual);

    /*
       Case 9:
       A is a descendant of W, B is a descendant of W

       Classically: No events are generated on W
       MPX: The focus window stays the same or moves to a different
       descendant of W. No events should be generated on W.

       Therefore, no event to X.
     */

    CoreFocusInEvents(dev, X, B, mode, NotifyNonlinearVirtual);

    /* Case 2:
       A is above W, B=W

       Classically: The move generates an EnterNotify on W with a detail of
       Ancestor or Nonlinear

       MPX:
       Case 2A: There is at least one other focus on W itself
       F(W) doesn't change, so the event should be suppressed
       Case 2B: Otherwise, if there is at least one other focus in a
       descendant
       F(W) moves from a descendant to W. detail is changed to Inferior.
       Case 2C: Otherwise:
       F(W) changes from a window above W to W itself.
       The detail may need to be changed from Ancestor to Nonlinear
       or vice-versa depending on the previous F(W). */

    if (!HasFocus(B)) {
        WindowPtr child = FirstFocusChild(B);

        if (child) {
            CoreFocusEvent(dev, FocusIn, mode, NotifyInferior, B);
            /* NotifyPointer B-P unless P is child or below. */
            CoreFocusInNotifyPointerEvents(dev, B, child, mode, FALSE);
        }
        else {
            CoreFocusEvent(dev, FocusIn, mode, NotifyNonlinear, B);
            /* NotifyPointer B-P unless P is child or below. */
            CoreFocusInNotifyPointerEvents(dev, B, None, mode, FALSE);
        }
    }
}

/**
 * Focus of dev moves from A to B and A is a descendant of B.
 */
static void
CoreFocusToAncestor(DeviceIntPtr dev, WindowPtr A, WindowPtr B, int mode)
{
    /* Case 4:
       A is W, B is above W

       Classically: The change generates a FocusOut on W with a detail of
       Ancestor or Nonlinear

       MPX:
       Case 3A: There is at least one other focus on W itself
       F(W) doesn't change, the event should be suppressed
       Case 3B: Otherwise, if there is at least one other focus in a
       descendant of W
       F(W) changes from W to a descendant of W. The detail field
       is set to Inferior
       Case 3C: Otherwise:
       The focus window moves from W to a window above W.
       The detail may need to be changed from Ancestor to Nonlinear or
       vice versa depending on the the new F(W)
     */
    if (!HasFocus(A)) {
        WindowPtr child = FirstFocusChild(A);

        if (child) {
            /* NotifyPointer P-A unless P is child or below */
            CoreFocusOutNotifyPointerEvents(dev, A, child, mode, FALSE);
            CoreFocusEvent(dev, FocusOut, mode, NotifyInferior, A);
        }
        else
            CoreFocusEvent(dev, FocusOut, mode, NotifyAncestor, A);
    }

    CoreFocusOutEvents(dev, A, B, mode, NotifyVirtual);

    /* Case 8:
       A is a descendant of W, B is W

       Classically: A FocusOut is generated on W with a detail of
       NotifyInferior

       MPX:
       Case 3A: There is at least one other focus on W itself
       F(W) doesn't change, the event should be suppressed
       Case 3B: Otherwise:
       F(W) changes from a descendant to W itself. */

    if (!HasFocus(B)) {
        CoreFocusEvent(dev, FocusIn, mode, NotifyInferior, B);
        /* NotifyPointer B-P unless P is A or below. */
        CoreFocusInNotifyPointerEvents(dev, B, A, mode, FALSE);
    }
}

/**
 * Focus of dev moves from A to B and B is a descendant of A.
 */
static void
CoreFocusToDescendant(DeviceIntPtr dev, WindowPtr A, WindowPtr B, int mode)
{
    /* Case 6:
       A is W, B is a descendant of W

       Classically: A FocusOut is generated on W with a detail of
       NotifyInferior

       MPX:
       Case 3A: There is at least one other focus on W itself
       F(W) doesn't change, the event should be suppressed
       Case 3B: Otherwise:
       F(W) changes from W to a descendant of W. */

    if (!HasFocus(A)) {
        /* NotifyPointer P-A unless P is B or below */
        CoreFocusOutNotifyPointerEvents(dev, A, B, mode, FALSE);
        CoreFocusEvent(dev, FocusOut, mode, NotifyInferior, A);
    }

    CoreFocusInEvents(dev, A, B, mode, NotifyVirtual);

    /* Case 2:
       A is above W, B=W

       Classically: The move generates an FocusIn on W with a detail of
       Ancestor or Nonlinear

       MPX:
       Case 2A: There is at least one other focus on W itself
       F(W) doesn't change, so the event should be suppressed
       Case 2B: Otherwise, if there is at least one other focus in a
       descendant
       F(W) moves from a descendant to W. detail is changed to Inferior.
       Case 2C: Otherwise:
       F(W) changes from a window above W to W itself.
       The detail may need to be changed from Ancestor to Nonlinear
       or vice-versa depending on the previous F(W). */

    if (!HasFocus(B)) {
        WindowPtr child = FirstFocusChild(B);

        if (child) {
            CoreFocusEvent(dev, FocusIn, mode, NotifyInferior, B);
            /* NotifyPointer B-P unless P is child or below. */
            CoreFocusInNotifyPointerEvents(dev, B, child, mode, FALSE);
        }
        else
            CoreFocusEvent(dev, FocusIn, mode, NotifyAncestor, B);
    }
}

static BOOL
HasOtherPointer(WindowPtr win, DeviceIntPtr exclude)
{
    int i;

    for (i = 0; i < MAXDEVICES; i++)
        if (i != exclude->id && PointerWindows[i] == win)
            return TRUE;

    return FALSE;
}

/**
 * Focus moves from PointerRoot to None or from None to PointerRoot.
 * Assumption: Neither A nor B are valid windows.
 */
static void
CoreFocusPointerRootNoneSwitch(DeviceIntPtr dev,
                               WindowPtr A,     /* PointerRootWin or NoneWin */
                               WindowPtr B,     /* NoneWin or PointerRootWin */
                               int mode)
{
    WindowPtr root;
    int i;
    int nscreens = screenInfo.numScreens;

#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        nscreens = 1;
#endif

    for (i = 0; i < nscreens; i++) {
        root = screenInfo.screens[i]->root;
        if (!HasOtherPointer(root, GetMaster(dev, POINTER_OR_FLOAT)) &&
            !FirstFocusChild(root)) {
            /* If pointer was on PointerRootWin and changes to NoneWin, and
             * the pointer paired with dev is below the current root window,
             * do a NotifyPointer run. */
            if (dev->focus && dev->focus->win == PointerRootWin &&
                B != PointerRootWin) {
                WindowPtr ptrwin = PointerWin(GetMaster(dev, POINTER_OR_FLOAT));

                if (ptrwin && IsParent(root, ptrwin))
                    CoreFocusOutNotifyPointerEvents(dev, root, None, mode,
                                                    TRUE);
            }
            CoreFocusEvent(dev, FocusOut, mode,
                           A ? NotifyPointerRoot : NotifyDetailNone, root);
            CoreFocusEvent(dev, FocusIn, mode,
                           B ? NotifyPointerRoot : NotifyDetailNone, root);
            if (B == PointerRootWin)
                CoreFocusInNotifyPointerEvents(dev, root, None, mode, TRUE);
        }

    }
}

/**
 * Focus moves from window A to PointerRoot or to None.
 * Assumption: A is a valid window and not PointerRoot or None.
 */
static void
CoreFocusToPointerRootOrNone(DeviceIntPtr dev, WindowPtr A,
                             WindowPtr B,        /* PointerRootWin or NoneWin */
                             int mode)
{
    WindowPtr root;
    int i;
    int nscreens = screenInfo.numScreens;

#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        nscreens = 1;
#endif

    if (!HasFocus(A)) {
        WindowPtr child = FirstFocusChild(A);

        if (child) {
            /* NotifyPointer P-A unless P is B or below */
            CoreFocusOutNotifyPointerEvents(dev, A, B, mode, FALSE);
            CoreFocusEvent(dev, FocusOut, mode, NotifyInferior, A);
        }
        else {
            /* NotifyPointer P-A */
            CoreFocusOutNotifyPointerEvents(dev, A, None, mode, FALSE);
            CoreFocusEvent(dev, FocusOut, mode, NotifyNonlinear, A);
        }
    }

    /* NullWindow means we include the root window */
    CoreFocusOutEvents(dev, A, NullWindow, mode, NotifyNonlinearVirtual);

    for (i = 0; i < nscreens; i++) {
        root = screenInfo.screens[i]->root;
        if (!HasFocus(root) && !FirstFocusChild(root)) {
            CoreFocusEvent(dev, FocusIn, mode,
                           B ? NotifyPointerRoot : NotifyDetailNone, root);
            if (B == PointerRootWin)
                CoreFocusInNotifyPointerEvents(dev, root, None, mode, TRUE);
        }
    }
}

/**
 * Focus moves from PointerRoot or None to a window B.
 * Assumption: B is a valid window and not PointerRoot or None.
 */
static void
CoreFocusFromPointerRootOrNone(DeviceIntPtr dev,
                               WindowPtr A,   /* PointerRootWin or NoneWin */
                               WindowPtr B, int mode)
{
    WindowPtr root;
    int i;
    int nscreens = screenInfo.numScreens;

#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        nscreens = 1;
#endif

    for (i = 0; i < nscreens; i++) {
        root = screenInfo.screens[i]->root;
        if (!HasFocus(root) && !FirstFocusChild(root)) {
            /* If pointer was on PointerRootWin and changes to NoneWin, and
             * the pointer paired with dev is below the current root window,
             * do a NotifyPointer run. */
            if (dev->focus && dev->focus->win == PointerRootWin &&
                B != PointerRootWin) {
                WindowPtr ptrwin = PointerWin(GetMaster(dev, POINTER_OR_FLOAT));

                if (ptrwin)
                    CoreFocusOutNotifyPointerEvents(dev, root, None, mode,
                                                    TRUE);
            }
            CoreFocusEvent(dev, FocusOut, mode,
                           A ? NotifyPointerRoot : NotifyDetailNone, root);
        }
    }

    root = B;                   /* get B's root window */
    while (root->parent)
        root = root->parent;

    if (B != root) {
        CoreFocusEvent(dev, FocusIn, mode, NotifyNonlinearVirtual, root);
        CoreFocusInEvents(dev, root, B, mode, NotifyNonlinearVirtual);
    }

    if (!HasFocus(B)) {
        WindowPtr child = FirstFocusChild(B);

        if (child) {
            CoreFocusEvent(dev, FocusIn, mode, NotifyInferior, B);
            /* NotifyPointer B-P unless P is child or below. */
            CoreFocusInNotifyPointerEvents(dev, B, child, mode, FALSE);
        }
        else {
            CoreFocusEvent(dev, FocusIn, mode, NotifyNonlinear, B);
            /* NotifyPointer B-P unless P is child or below. */
            CoreFocusInNotifyPointerEvents(dev, B, None, mode, FALSE);
        }
    }

}

static void
CoreFocusEvents(DeviceIntPtr dev, WindowPtr from, WindowPtr to, int mode)
{
    if (!IsMaster(dev))
        return;

    SetFocusOut(dev);

    if (((to == NullWindow) || (to == PointerRootWin)) &&
        ((from == NullWindow) || (from == PointerRootWin)))
        CoreFocusPointerRootNoneSwitch(dev, from, to, mode);
    else if ((to == NullWindow) || (to == PointerRootWin))
        CoreFocusToPointerRootOrNone(dev, from, to, mode);
    else if ((from == NullWindow) || (from == PointerRootWin))
        CoreFocusFromPointerRootOrNone(dev, from, to, mode);
    else if (IsParent(from, to))
        CoreFocusToDescendant(dev, from, to, mode);
    else if (IsParent(to, from))
        CoreFocusToAncestor(dev, from, to, mode);
    else
        CoreFocusNonLinear(dev, from, to, mode);

    SetFocusIn(dev, to);
}

static void
DeviceFocusEvents(DeviceIntPtr dev, WindowPtr from, WindowPtr to, int mode)
{
    int out, in;                /* for holding details for to/from
                                   PointerRoot/None */
    int i;
    int nscreens = screenInfo.numScreens;
    SpritePtr sprite = dev->spriteInfo->sprite;

    if (from == to)
        return;
    out = (from == NoneWin) ? NotifyDetailNone : NotifyPointerRoot;
    in = (to == NoneWin) ? NotifyDetailNone : NotifyPointerRoot;
    /* wrong values if neither, but then not referenced */

#ifdef PANORAMIX
    if (!noPanoramiXExtension)
        nscreens = 1;
#endif

    if ((to == NullWindow) || (to == PointerRootWin)) {
        if ((from == NullWindow) || (from == PointerRootWin)) {
            if (from == PointerRootWin) {
                DeviceFocusEvent(dev, XI_FocusOut, mode, NotifyPointer,
                                 sprite->win);
                DeviceFocusOutEvents(dev, sprite->win,
                                     GetCurrentRootWindow(dev), mode,
                                     NotifyPointer);
            }
            /* Notify all the roots */
            for (i = 0; i < nscreens; i++)
                DeviceFocusEvent(dev, XI_FocusOut, mode, out,
                                 screenInfo.screens[i]->root);
        }
        else {
            if (IsParent(from, sprite->win)) {
                DeviceFocusEvent(dev, XI_FocusOut, mode, NotifyPointer,
                                 sprite->win);
                DeviceFocusOutEvents(dev, sprite->win, from, mode,
                                     NotifyPointer);
            }
            DeviceFocusEvent(dev, XI_FocusOut, mode, NotifyNonlinear, from);
            /* next call catches the root too, if the screen changed */
            DeviceFocusOutEvents(dev, from, NullWindow, mode,
                                 NotifyNonlinearVirtual);
        }
        /* Notify all the roots */
        for (i = 0; i < nscreens; i++)
            DeviceFocusEvent(dev, XI_FocusIn, mode, in,
                             screenInfo.screens[i]->root);
        if (to == PointerRootWin) {
            DeviceFocusInEvents(dev, GetCurrentRootWindow(dev), sprite->win,
                                mode, NotifyPointer);
            DeviceFocusEvent(dev, XI_FocusIn, mode, NotifyPointer, sprite->win);
        }
    }
    else {
        if ((from == NullWindow) || (from == PointerRootWin)) {
            if (from == PointerRootWin) {
                DeviceFocusEvent(dev, XI_FocusOut, mode, NotifyPointer,
                                 sprite->win);
                DeviceFocusOutEvents(dev, sprite->win,
                                     GetCurrentRootWindow(dev), mode,
                                     NotifyPointer);
            }
            for (i = 0; i < nscreens; i++)
                DeviceFocusEvent(dev, XI_FocusOut, mode, out,
                                 screenInfo.screens[i]->root);
            if (to->parent != NullWindow)
                DeviceFocusInEvents(dev, GetCurrentRootWindow(dev), to, mode,
                                    NotifyNonlinearVirtual);
            DeviceFocusEvent(dev, XI_FocusIn, mode, NotifyNonlinear, to);
            if (IsParent(to, sprite->win))
                DeviceFocusInEvents(dev, to, sprite->win, mode, NotifyPointer);
        }
        else {
            if (IsParent(to, from)) {
                DeviceFocusEvent(dev, XI_FocusOut, mode, NotifyAncestor, from);
                DeviceFocusOutEvents(dev, from, to, mode, NotifyVirtual);
                DeviceFocusEvent(dev, XI_FocusIn, mode, NotifyInferior, to);
                if ((IsParent(to, sprite->win)) &&
                    (sprite->win != from) &&
                    (!IsParent(from, sprite->win)) &&
                    (!IsParent(sprite->win, from)))
                    DeviceFocusInEvents(dev, to, sprite->win, mode,
                                        NotifyPointer);
            }
            else if (IsParent(from, to)) {
                if ((IsParent(from, sprite->win)) &&
                    (sprite->win != from) &&
                    (!IsParent(to, sprite->win)) &&
                    (!IsParent(sprite->win, to))) {
                    DeviceFocusEvent(dev, XI_FocusOut, mode, NotifyPointer,
                                     sprite->win);
                    DeviceFocusOutEvents(dev, sprite->win, from, mode,
                                         NotifyPointer);
                }
                DeviceFocusEvent(dev, XI_FocusOut, mode, NotifyInferior, from);
                DeviceFocusInEvents(dev, from, to, mode, NotifyVirtual);
                DeviceFocusEvent(dev, XI_FocusIn, mode, NotifyAncestor, to);
            }
            else {
                /* neither from or to is child of other */
                WindowPtr common = CommonAncestor(to, from);

                /* common == NullWindow ==> different screens */
                if (IsParent(from, sprite->win))
                    DeviceFocusOutEvents(dev, sprite->win, from, mode,
                                         NotifyPointer);
                DeviceFocusEvent(dev, XI_FocusOut, mode, NotifyNonlinear, from);
                if (from->parent != NullWindow)
                    DeviceFocusOutEvents(dev, from, common, mode,
                                         NotifyNonlinearVirtual);
                if (to->parent != NullWindow)
                    DeviceFocusInEvents(dev, common, to, mode,
                                        NotifyNonlinearVirtual);
                DeviceFocusEvent(dev, XI_FocusIn, mode, NotifyNonlinear, to);
                if (IsParent(to, sprite->win))
                    DeviceFocusInEvents(dev, to, sprite->win, mode,
                                        NotifyPointer);
            }
        }
    }
}

/**
 * Figure out if focus events are necessary and send them to the
 * appropriate windows.
 *
 * @param from Window the focus moved out of.
 * @param to Window the focus moved into.
 */
void
DoFocusEvents(DeviceIntPtr pDev, WindowPtr from, WindowPtr to, int mode)
{
    if (!IsKeyboardDevice(pDev))
        return;

    if (from == to && mode != NotifyGrab && mode != NotifyUngrab)
        return;

    CoreFocusEvents(pDev, from, to, mode);
    DeviceFocusEvents(pDev, from, to, mode);
}
