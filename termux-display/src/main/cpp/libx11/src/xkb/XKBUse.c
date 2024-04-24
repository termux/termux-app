/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include "Xlibint.h"
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"

static Bool _XkbIgnoreExtension = False;

void
XkbNoteMapChanges(XkbMapChangesPtr old,
                  XkbMapNotifyEvent *new,
                  unsigned wanted)
{
    int first, oldLast, newLast;

    wanted &= new->changed;

    if (wanted & XkbKeyTypesMask) {
        if (old->changed & XkbKeyTypesMask) {
            first = old->first_type;
            oldLast = old->first_type + old->num_types - 1;
            newLast = new->first_type + new->num_types - 1;

            if (new->first_type < first)
                first = new->first_type;
            if (oldLast > newLast)
                newLast = oldLast;
            old->first_type = first;
            old->num_types = newLast - first + 1;
        }
        else {
            old->first_type = new->first_type;
            old->num_types = new->num_types;
        }
    }
    if (wanted & XkbKeySymsMask) {
        if (old->changed & XkbKeySymsMask) {
            first = old->first_key_sym;
            oldLast = old->first_key_sym + old->num_key_syms - 1;
            newLast = new->first_key_sym + new->num_key_syms - 1;

            if (new->first_key_sym < first)
                first = new->first_key_sym;
            if (oldLast > newLast)
                newLast = oldLast;
            old->first_key_sym = first;
            old->num_key_syms = newLast - first + 1;
        }
        else {
            old->first_key_sym = new->first_key_sym;
            old->num_key_syms = new->num_key_syms;
        }
    }
    if (wanted & XkbKeyActionsMask) {
        if (old->changed & XkbKeyActionsMask) {
            first = old->first_key_act;
            oldLast = old->first_key_act + old->num_key_acts - 1;
            newLast = new->first_key_act + new->num_key_acts - 1;

            if (new->first_key_act < first)
                first = new->first_key_act;
            if (oldLast > newLast)
                newLast = oldLast;
            old->first_key_act = first;
            old->num_key_acts = newLast - first + 1;
        }
        else {
            old->first_key_act = new->first_key_act;
            old->num_key_acts = new->num_key_acts;
        }
    }
    if (wanted & XkbKeyBehaviorsMask) {
        if (old->changed & XkbKeyBehaviorsMask) {
            first = old->first_key_behavior;
            oldLast = old->first_key_behavior + old->num_key_behaviors - 1;
            newLast = new->first_key_behavior + new->num_key_behaviors - 1;

            if (new->first_key_behavior < first)
                first = new->first_key_behavior;
            if (oldLast > newLast)
                newLast = oldLast;
            old->first_key_behavior = first;
            old->num_key_behaviors = newLast - first + 1;
        }
        else {
            old->first_key_behavior = new->first_key_behavior;
            old->num_key_behaviors = new->num_key_behaviors;
        }
    }
    if (wanted & XkbVirtualModsMask) {
        old->vmods |= new->vmods;
    }
    if (wanted & XkbExplicitComponentsMask) {
        if (old->changed & XkbExplicitComponentsMask) {
            first = old->first_key_explicit;
            oldLast = old->first_key_explicit + old->num_key_explicit - 1;
            newLast = new->first_key_explicit + new->num_key_explicit - 1;

            if (new->first_key_explicit < first)
                first = new->first_key_explicit;
            if (oldLast > newLast)
                newLast = oldLast;
            old->first_key_explicit = first;
            old->num_key_explicit = newLast - first + 1;
        }
        else {
            old->first_key_explicit = new->first_key_explicit;
            old->num_key_explicit = new->num_key_explicit;
        }
    }
    if (wanted & XkbModifierMapMask) {
        if (old->changed & XkbModifierMapMask) {
            first = old->first_modmap_key;
            oldLast = old->first_modmap_key + old->num_modmap_keys - 1;
            newLast = new->first_modmap_key + new->num_modmap_keys - 1;

            if (new->first_modmap_key < first)
                first = new->first_modmap_key;
            if (oldLast > newLast)
                newLast = oldLast;
            old->first_modmap_key = first;
            old->num_modmap_keys = newLast - first + 1;
        }
        else {
            old->first_modmap_key = new->first_modmap_key;
            old->num_modmap_keys = new->num_modmap_keys;
        }
    }
    if (wanted & XkbVirtualModMapMask) {
        if (old->changed & XkbVirtualModMapMask) {
            first = old->first_vmodmap_key;
            oldLast = old->first_vmodmap_key + old->num_vmodmap_keys - 1;
            newLast = new->first_vmodmap_key + new->num_vmodmap_keys - 1;

            if (new->first_vmodmap_key < first)
                first = new->first_vmodmap_key;
            if (oldLast > newLast)
                newLast = oldLast;
            old->first_vmodmap_key = first;
            old->num_vmodmap_keys = newLast - first + 1;
        }
        else {
            old->first_vmodmap_key = new->first_vmodmap_key;
            old->num_vmodmap_keys = new->num_vmodmap_keys;
        }
    }
    old->changed |= wanted;
    return;
}

void
_XkbNoteCoreMapChanges(XkbMapChangesPtr old,
                       XMappingEvent *new,
                       unsigned int wanted)
{
    int first, oldLast, newLast;

    if ((new->request == MappingKeyboard) && (wanted & XkbKeySymsMask)) {
        if (old->changed & XkbKeySymsMask) {
            first = old->first_key_sym;
            oldLast = old->first_key_sym + old->num_key_syms - 1;
            newLast = new->first_keycode + new->count - 1;

            if (new->first_keycode < first)
                first = new->first_keycode;
            if (oldLast > newLast)
                newLast = oldLast;
            old->first_key_sym = first;
            old->num_key_syms = newLast - first + 1;
        }
        else {
            old->changed |= XkbKeySymsMask;
            old->first_key_sym = new->first_keycode;
            old->num_key_syms = new->count;
        }
    }
    return;
}

static Bool
wire_to_event(Display *dpy, XEvent *re, xEvent *event)
{
    xkbEvent *xkbevent = (xkbEvent *) event;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    xkbi = dpy->xkb_info;
    if (((event->u.u.type & 0x7f) - xkbi->codes->first_event) != XkbEventCode)
        return False;

    switch (xkbevent->u.any.xkbType) {
    case XkbStateNotify:
    {
        xkbStateNotify *sn = (xkbStateNotify *) event;

        if (xkbi->selected_events & XkbStateNotifyMask) {
            XkbStateNotifyEvent *sev = (XkbStateNotifyEvent *) re;

            sev->type = XkbEventCode + xkbi->codes->first_event;
            sev->xkb_type = XkbStateNotify;
            sev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            sev->send_event = ((event->u.u.type & 0x80) != 0);
            sev->display = dpy;
            sev->time = sn->time;
            sev->device = sn->deviceID;
            sev->keycode = sn->keycode;
            sev->event_type = sn->eventType;
            sev->req_major = sn->requestMajor;
            sev->req_minor = sn->requestMinor;
            sev->changed = sn->changed;
            sev->group = sn->group;
            sev->base_group = sn->baseGroup;
            sev->latched_group = sn->latchedGroup;
            sev->locked_group = sn->lockedGroup;
            sev->mods = sn->mods;
            sev->base_mods = sn->baseMods;
            sev->latched_mods = sn->latchedMods;
            sev->locked_mods = sn->lockedMods;
            sev->compat_state = sn->compatState;
            sev->grab_mods = sn->grabMods;
            sev->compat_grab_mods = sn->compatGrabMods;
            sev->lookup_mods = sn->lookupMods;
            sev->compat_lookup_mods = sn->compatLookupMods;
            sev->ptr_buttons = sn->ptrBtnState;
            return True;
        }
    }
        break;
    case XkbMapNotify:
    {
        xkbMapNotify *mn = (xkbMapNotify *) event;

        if ((xkbi->selected_events & XkbMapNotifyMask) &&
            (xkbi->selected_map_details & mn->changed)) {
            XkbMapNotifyEvent *mev = (XkbMapNotifyEvent *) re;

            mev->type = XkbEventCode + xkbi->codes->first_event;
            mev->xkb_type = XkbMapNotify;
            mev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            mev->send_event = ((event->u.u.type & 0x80) != 0);
            mev->display = dpy;
            mev->time = mn->time;
            mev->device = mn->deviceID;
            mev->changed = mn->changed;
            mev->min_key_code = mn->minKeyCode;
            mev->max_key_code = mn->maxKeyCode;
            mev->first_type = mn->firstType;
            mev->num_types = mn->nTypes;
            mev->first_key_sym = mn->firstKeySym;
            mev->num_key_syms = mn->nKeySyms;
            mev->first_key_act = mn->firstKeyAct;
            mev->num_key_acts = mn->nKeyActs;
            mev->first_key_behavior = mn->firstKeyBehavior;
            mev->num_key_behaviors = mn->nKeyBehaviors;
            mev->vmods = mn->virtualMods;
            mev->first_key_explicit = mn->firstKeyExplicit;
            mev->num_key_explicit = mn->nKeyExplicit;
            mev->first_modmap_key = mn->firstModMapKey;
            mev->num_modmap_keys = mn->nModMapKeys;
            mev->first_vmodmap_key = mn->firstVModMapKey;
            mev->num_vmodmap_keys = mn->nVModMapKeys;
            XkbNoteMapChanges(&xkbi->changes, mev, XKB_XLIB_MAP_MASK);
            if (xkbi->changes.changed)
                xkbi->flags |= XkbMapPending;
            return True;
        }
        else if (mn->nKeySyms > 0) {
            register XMappingEvent *ev = (XMappingEvent *) re;

            ev->type = MappingNotify;
            ev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            ev->send_event = ((event->u.u.type & 0x80) != 0);
            ev->display = dpy;
            ev->window = 0;
            ev->first_keycode = mn->firstKeySym;
            ev->request = MappingKeyboard;
            ev->count = mn->nKeySyms;
            _XkbNoteCoreMapChanges(&xkbi->changes, ev, XKB_XLIB_MAP_MASK);
            if (xkbi->changes.changed)
                xkbi->flags |= XkbMapPending;
            return True;
        }
    }
        break;
    case XkbControlsNotify:
    {
        if (xkbi->selected_events & XkbControlsNotifyMask) {
            xkbControlsNotify *cn = (xkbControlsNotify *) event;
            XkbControlsNotifyEvent *cev = (XkbControlsNotifyEvent *) re;

            cev->type = XkbEventCode + xkbi->codes->first_event;
            cev->xkb_type = XkbControlsNotify;
            cev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            cev->send_event = ((event->u.u.type & 0x80) != 0);
            cev->display = dpy;
            cev->time = cn->time;
            cev->device = cn->deviceID;
            cev->changed_ctrls = cn->changedControls;
            cev->enabled_ctrls = cn->enabledControls;
            cev->enabled_ctrl_changes = cn->enabledControlChanges;
            cev->keycode = cn->keycode;
            cev->num_groups = cn->numGroups;
            cev->event_type = cn->eventType;
            cev->req_major = cn->requestMajor;
            cev->req_minor = cn->requestMinor;
            return True;
        }
    }
        break;
    case XkbIndicatorMapNotify:
    {
        if (xkbi->selected_events & XkbIndicatorMapNotifyMask) {
            xkbIndicatorNotify *in = (xkbIndicatorNotify *) event;
            XkbIndicatorNotifyEvent *iev = (XkbIndicatorNotifyEvent *) re;

            iev->type = XkbEventCode + xkbi->codes->first_event;
            iev->xkb_type = XkbIndicatorMapNotify;
            iev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            iev->send_event = ((event->u.u.type & 0x80) != 0);
            iev->display = dpy;
            iev->time = in->time;
            iev->device = in->deviceID;
            iev->changed = in->changed;
            iev->state = in->state;
            return True;
        }
    }
        break;
    case XkbIndicatorStateNotify:
    {
        if (xkbi->selected_events & XkbIndicatorStateNotifyMask) {
            xkbIndicatorNotify *in = (xkbIndicatorNotify *) event;
            XkbIndicatorNotifyEvent *iev = (XkbIndicatorNotifyEvent *) re;

            iev->type = XkbEventCode + xkbi->codes->first_event;
            iev->xkb_type = XkbIndicatorStateNotify;
            iev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            iev->send_event = ((event->u.u.type & 0x80) != 0);
            iev->display = dpy;
            iev->time = in->time;
            iev->device = in->deviceID;
            iev->changed = in->changed;
            iev->state = in->state;
            return True;
        }
    }
        break;
    case XkbBellNotify:
    {
        if (xkbi->selected_events & XkbBellNotifyMask) {
            xkbBellNotify *bn = (xkbBellNotify *) event;
            XkbBellNotifyEvent *bev = (XkbBellNotifyEvent *) re;

            bev->type = XkbEventCode + xkbi->codes->first_event;
            bev->xkb_type = XkbBellNotify;
            bev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            bev->send_event = ((event->u.u.type & 0x80) != 0);
            bev->display = dpy;
            bev->time = bn->time;
            bev->device = bn->deviceID;
            bev->percent = bn->percent;
            bev->pitch = bn->pitch;
            bev->duration = bn->duration;
            bev->bell_class = bn->bellClass;
            bev->bell_id = bn->bellID;
            bev->name = bn->name;
            bev->window = bn->window;
            bev->event_only = bn->eventOnly;
            return True;
        }
    }
        break;
    case XkbAccessXNotify:
    {
        if (xkbi->selected_events & XkbAccessXNotifyMask) {
            xkbAccessXNotify *axn = (xkbAccessXNotify *) event;
            XkbAccessXNotifyEvent *axev = (XkbAccessXNotifyEvent *) re;

            axev->type = XkbEventCode + xkbi->codes->first_event;
            axev->xkb_type = XkbAccessXNotify;
            axev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            axev->send_event = ((event->u.u.type & 0x80) != 0);
            axev->display = dpy;
            axev->time = axn->time;
            axev->device = axn->deviceID;
            axev->detail = axn->detail;
            axev->keycode = axn->keycode;
            axev->sk_delay = axn->slowKeysDelay;
            axev->debounce_delay = axn->debounceDelay;
            return True;
        }
    }
        break;
    case XkbNamesNotify:
    {
        if (xkbi->selected_events & XkbNamesNotifyMask) {
            xkbNamesNotify *nn = (xkbNamesNotify *) event;
            XkbNamesNotifyEvent *nev = (XkbNamesNotifyEvent *) re;

            nev->type = XkbEventCode + xkbi->codes->first_event;
            nev->xkb_type = XkbNamesNotify;
            nev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            nev->send_event = ((event->u.u.type & 0x80) != 0);
            nev->display = dpy;
            nev->time = nn->time;
            nev->device = nn->deviceID;
            nev->changed = nn->changed;
            nev->first_type = nn->firstType;
            nev->num_types = nn->nTypes;
            nev->first_lvl = nn->firstLevelName;
            nev->num_lvls = nn->nLevelNames;
            nev->num_aliases = nn->nAliases;
            nev->num_radio_groups = nn->nRadioGroups;
            nev->changed_vmods = nn->changedVirtualMods;
            nev->changed_groups = nn->changedGroupNames;
            nev->changed_indicators = nn->changedIndicators;
            nev->first_key = nn->firstKey;
            nev->num_keys = nn->nKeys;
            return True;
        }
    }
        break;
    case XkbCompatMapNotify:
    {
        if (xkbi->selected_events & XkbCompatMapNotifyMask) {
            xkbCompatMapNotify *cmn = (xkbCompatMapNotify *) event;
            XkbCompatMapNotifyEvent *cmev = (XkbCompatMapNotifyEvent *) re;

            cmev->type = XkbEventCode + xkbi->codes->first_event;
            cmev->xkb_type = XkbCompatMapNotify;
            cmev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            cmev->send_event = ((event->u.u.type & 0x80) != 0);
            cmev->display = dpy;
            cmev->time = cmn->time;
            cmev->device = cmn->deviceID;
            cmev->changed_groups = cmn->changedGroups;
            cmev->first_si = cmn->firstSI;
            cmev->num_si = cmn->nSI;
            cmev->num_total_si = cmn->nTotalSI;
            return True;
        }
    }
        break;
    case XkbActionMessage:
    {
        if (xkbi->selected_events & XkbActionMessageMask) {
            xkbActionMessage *am = (xkbActionMessage *) event;
            XkbActionMessageEvent *amev = (XkbActionMessageEvent *) re;

            amev->type = XkbEventCode + xkbi->codes->first_event;
            amev->xkb_type = XkbActionMessage;
            amev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            amev->send_event = ((event->u.u.type & 0x80) != 0);
            amev->display = dpy;
            amev->time = am->time;
            amev->device = am->deviceID;
            amev->keycode = am->keycode;
            amev->press = am->press;
            amev->key_event_follows = am->keyEventFollows;
            amev->group = am->group;
            amev->mods = am->mods;
            memcpy(amev->message, am->message, XkbActionMessageLength);
            amev->message[XkbActionMessageLength] = '\0';
            return True;
        }
    }
        break;
    case XkbExtensionDeviceNotify:
    {
        if (xkbi->selected_events & XkbExtensionDeviceNotifyMask) {
            xkbExtensionDeviceNotify *ed = (xkbExtensionDeviceNotify *) event;
            XkbExtensionDeviceNotifyEvent *edev
                = (XkbExtensionDeviceNotifyEvent *) re;

            edev->type = XkbEventCode + xkbi->codes->first_event;
            edev->xkb_type = XkbExtensionDeviceNotify;
            edev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            edev->send_event = ((event->u.u.type & 0x80) != 0);
            edev->display = dpy;
            edev->time = ed->time;
            edev->device = ed->deviceID;
            edev->led_class = ed->ledClass;
            edev->led_id = ed->ledID;
            edev->reason = ed->reason;
            edev->supported = ed->supported;
            edev->leds_defined = ed->ledsDefined;
            edev->led_state = ed->ledState;
            edev->first_btn = ed->firstBtn;
            edev->num_btns = ed->nBtns;
            edev->unsupported = ed->unsupported;
            return True;
        }
    }
        break;
    case XkbNewKeyboardNotify:
    {
        xkbNewKeyboardNotify *nkn = (xkbNewKeyboardNotify *) event;

        if ((xkbi->selected_events & XkbNewKeyboardNotifyMask) &&
            (xkbi->selected_nkn_details & nkn->changed)) {
            XkbNewKeyboardNotifyEvent *nkev = (XkbNewKeyboardNotifyEvent *) re;

            nkev->type = XkbEventCode + xkbi->codes->first_event;
            nkev->xkb_type = XkbNewKeyboardNotify;
            nkev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            nkev->send_event = ((event->u.u.type & 0x80) != 0);
            nkev->display = dpy;
            nkev->time = nkn->time;
            nkev->device = nkn->deviceID;
            nkev->old_device = nkn->oldDeviceID;
            nkev->min_key_code = nkn->minKeyCode;
            nkev->max_key_code = nkn->maxKeyCode;
            nkev->old_min_key_code = nkn->oldMinKeyCode;
            nkev->old_max_key_code = nkn->oldMaxKeyCode;
            nkev->req_major = nkn->requestMajor;
            nkev->req_minor = nkn->requestMinor;
            nkev->changed = nkn->changed;
            if ((xkbi->desc) && (nkev->send_event == 0) &&
                ((xkbi->desc->device_spec == nkev->old_device) ||
                 (nkev->device != nkev->old_device))) {
                xkbi->flags = XkbMapPending | XkbXlibNewKeyboard;
            }
            return True;
        }
        else if (nkn->changed & (XkbNKN_KeycodesMask | XkbNKN_DeviceIDMask)) {
            register XMappingEvent *ev = (XMappingEvent *) re;

            ev->type = MappingNotify;
            ev->serial = _XSetLastRequestRead(dpy, (xGenericReply *) event);
            ev->send_event = ((event->u.u.type & 0x80) != 0);
            ev->display = dpy;
            ev->window = 0;
            ev->first_keycode = dpy->min_keycode;
            ev->request = MappingKeyboard;
            ev->count = (dpy->max_keycode - dpy->min_keycode) + 1;
            if ((xkbi->desc) && (ev->send_event == 0) &&
                ((xkbi->desc->device_spec == nkn->oldDeviceID) ||
                 (nkn->deviceID != nkn->oldDeviceID))) {
                xkbi->flags |= XkbMapPending | XkbXlibNewKeyboard;
            }
            return True;
        }
    }
        break;
    default:
#ifdef DEBUG
        fprintf(stderr, "Got unknown XKEYBOARD event (%d, base=%d)\n",
                re->type, xkbi->codes->first_event);
#endif
        break;
    }
    return False;
}

Bool
XkbIgnoreExtension(Bool ignore)
{
    if (getenv("XKB_FORCE") != NULL) {
#ifdef DEBUG
        fprintf(stderr,
                "Forcing use of XKEYBOARD (overriding an IgnoreExtensions)\n");
#endif
        return False;
    }
#ifdef DEBUG
    else if (getenv("XKB_DEBUG") != NULL) {
        fprintf(stderr, "Explicitly %signoring XKEYBOARD\n",
                ignore ? "" : "not ");
    }
#endif
    _XkbIgnoreExtension = ignore;
    return True;
}

static void
_XkbFreeInfo(Display *dpy)
{
    XkbInfoPtr xkbi = dpy->xkb_info;

    if (xkbi) {
        if (xkbi->desc)
            XkbFreeKeyboard(xkbi->desc, XkbAllComponentsMask, True);
        Xfree(xkbi);
        dpy->xkb_info = NULL;
    }
}

Bool
XkbUseExtension(Display *dpy, int *major_rtrn, int *minor_rtrn)
{
    xkbUseExtensionReply rep;
    register xkbUseExtensionReq *req;
    XExtCodes *codes;
    int ev_base, forceIgnore;
    XkbInfoPtr xkbi;
    char *str;
    static int debugMsg;
    static int been_here = 0;

    if (dpy->xkb_info && !(dpy->flags & XlibDisplayNoXkb)) {
        if (major_rtrn)
            *major_rtrn = dpy->xkb_info->srv_major;
        if (minor_rtrn)
            *minor_rtrn = dpy->xkb_info->srv_minor;
        return True;
    }
    if (!been_here) {
        debugMsg = (getenv("XKB_DEBUG") != NULL);
        been_here = 1;
    }

    if (major_rtrn)
        *major_rtrn = 0;
    if (minor_rtrn)
        *minor_rtrn = 0;

    if (!dpy->xkb_info) {
        xkbi = _XkbTypedCalloc(1, XkbInfoRec);
        if (!xkbi)
            return False;
        dpy->xkb_info = xkbi;
        dpy->free_funcs->xkb = _XkbFreeInfo;

        xkbi->xlib_ctrls |= (XkbLC_ControlFallback | XkbLC_ConsumeLookupMods);
        if ((str = getenv("_XKB_OPTIONS_ENABLE")) != NULL) {
            if ((str = getenv("_XKB_LATIN1_LOOKUP")) != NULL) {
                if ((strcmp(str, "off") == 0) || (strcmp(str, "0") == 0))
                    xkbi->xlib_ctrls &= ~XkbLC_ForceLatin1Lookup;
                else
                    xkbi->xlib_ctrls |= XkbLC_ForceLatin1Lookup;
            }
            if ((str = getenv("_XKB_CONSUME_LOOKUP_MODS")) != NULL) {
                if ((strcmp(str, "off") == 0) || (strcmp(str, "0") == 0))
                    xkbi->xlib_ctrls &= ~XkbLC_ConsumeLookupMods;
                else
                    xkbi->xlib_ctrls |= XkbLC_ConsumeLookupMods;
            }
            if ((str = getenv("_XKB_CONSUME_SHIFT_AND_LOCK")) != NULL) {
                if ((strcmp(str, "off") == 0) || (strcmp(str, "0") == 0))
                    xkbi->xlib_ctrls &= ~XkbLC_AlwaysConsumeShiftAndLock;
                else
                    xkbi->xlib_ctrls |= XkbLC_AlwaysConsumeShiftAndLock;
            }
            if ((str = getenv("_XKB_IGNORE_NEW_KEYBOARDS")) != NULL) {
                if ((strcmp(str, "off") == 0) || (strcmp(str, "0") == 0))
                    xkbi->xlib_ctrls &= ~XkbLC_IgnoreNewKeyboards;
                else
                    xkbi->xlib_ctrls |= XkbLC_IgnoreNewKeyboards;
            }
            if ((str = getenv("_XKB_CONTROL_FALLBACK")) != NULL) {
                if ((strcmp(str, "off") == 0) || (strcmp(str, "0") == 0))
                    xkbi->xlib_ctrls &= ~XkbLC_ControlFallback;
                else
                    xkbi->xlib_ctrls |= XkbLC_ControlFallback;
            }
            if ((str = getenv("_XKB_COMP_LED")) != NULL) {
                if ((strcmp(str, "off") == 0) || (strcmp(str, "0") == 0))
                    xkbi->xlib_ctrls &= ~XkbLC_ComposeLED;
                else {
                    xkbi->xlib_ctrls |= XkbLC_ComposeLED;
                    if (strlen(str) > 0)
                        xkbi->composeLED = XInternAtom(dpy, str, False);
                }
            }
            if ((str = getenv("_XKB_COMP_FAIL_BEEP")) != NULL) {
                if ((strcmp(str, "off") == 0) || (strcmp(str, "0") == 0))
                    xkbi->xlib_ctrls &= ~XkbLC_BeepOnComposeFail;
                else
                    xkbi->xlib_ctrls |= XkbLC_BeepOnComposeFail;
            }
        }
        if ((xkbi->composeLED == None) &&
            ((xkbi->xlib_ctrls & XkbLC_ComposeLED) != 0))
            xkbi->composeLED = XInternAtom(dpy, "Compose", False);
#ifdef DEBUG
        if (debugMsg) {
            register unsigned c = xkbi->xlib_ctrls;

            fprintf(stderr,
                    "XKEYBOARD compose: beep on failure is %s, LED is %s\n",
                    ((c & XkbLC_BeepOnComposeFail) ? "on" : "off"),
                    ((c & XkbLC_ComposeLED) ? "on" : "off"));
            fprintf(stderr,
                    "XKEYBOARD XLookupString: %slatin-1, %s lookup modifiers\n",
                    ((c & XkbLC_ForceLatin1Lookup) ? "allow non-" : "force "),
                    ((c & XkbLC_ConsumeLookupMods) ? "consume" : "re-use"));
            fprintf(stderr,
                    "XKEYBOARD XLookupString: %sconsume shift and lock, %scontrol fallback\n",
                    ((c & XkbLC_AlwaysConsumeShiftAndLock) ? "always " :
                     "don't "), ((c & XkbLC_ControlFallback) ? "" : "no "));

        }
#endif
    }
    else
        xkbi = dpy->xkb_info;

    forceIgnore = (dpy->flags & XlibDisplayNoXkb) || dpy->keysyms;
    forceIgnore = forceIgnore && (major_rtrn == NULL) && (minor_rtrn == NULL);
    if (forceIgnore || _XkbIgnoreExtension || getenv("XKB_DISABLE")) {
        LockDisplay(dpy);
        dpy->flags |= XlibDisplayNoXkb;
        UnlockDisplay(dpy);
        if (debugMsg)
            fprintf(stderr, "XKEYBOARD extension disabled or missing\n");
        return False;
    }

    if ((codes = XInitExtension(dpy, XkbName)) == NULL) {
        LockDisplay(dpy);
        dpy->flags |= XlibDisplayNoXkb;
        UnlockDisplay(dpy);
        if (debugMsg)
            fprintf(stderr, "XKEYBOARD extension not present\n");
        return False;
    }
    xkbi->codes = codes;
    LockDisplay(dpy);

    GetReq(kbUseExtension, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbUseExtension;
    req->wantedMajor = XkbMajorVersion;
    req->wantedMinor = XkbMinorVersion;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse) || !rep.supported) {
        Bool fail = True;

        if (debugMsg)
            fprintf(stderr,
                    "XKEYBOARD version mismatch (want %d.%02d, got %d.%02d)\n",
                    XkbMajorVersion, XkbMinorVersion,
                    rep.serverMajor, rep.serverMinor);

        /* pre-release 0.65 is very close to 1.00 */
        if ((rep.serverMajor == 0) && (rep.serverMinor == 65)) {
            if (debugMsg)
                fprintf(stderr, "Trying to fall back to version 0.65...");
            GetReq(kbUseExtension, req);
            req->reqType = xkbi->codes->major_opcode;
            req->xkbReqType = X_kbUseExtension;
            req->wantedMajor = 0;
            req->wantedMinor = 65;
            if (_XReply(dpy, (xReply *) &rep, 0, xFalse) && rep.supported) {
                if (debugMsg)
                    fprintf(stderr, "succeeded\n");
                fail = False;
            }
            else if (debugMsg)
                fprintf(stderr, "failed\n");
        }
        if (fail) {
            dpy->flags |= XlibDisplayNoXkb;
            UnlockDisplay(dpy);
            SyncHandle();
            if (major_rtrn)
                *major_rtrn = rep.serverMajor;
            if (minor_rtrn)
                *minor_rtrn = rep.serverMinor;
            return False;
        }
    }
#ifdef DEBUG
    else if (forceIgnore) {
        fprintf(stderr,
                "Internal Error!  XkbUseExtension succeeded with forceIgnore set\n");
    }
#endif
    UnlockDisplay(dpy);
    xkbi->srv_major = rep.serverMajor;
    xkbi->srv_minor = rep.serverMinor;
    if (major_rtrn)
        *major_rtrn = rep.serverMajor;
    if (minor_rtrn)
        *minor_rtrn = rep.serverMinor;
    if (debugMsg)
        fprintf(stderr, "XKEYBOARD (version %d.%02d/%d.%02d) OK!\n",
                XkbMajorVersion, XkbMinorVersion,
                rep.serverMajor, rep.serverMinor);

    ev_base = codes->first_event;
    XESetWireToEvent(dpy, ev_base + XkbEventCode, wire_to_event);
    SyncHandle();
    return True;
}
