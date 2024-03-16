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
#include "Xlibint.h"
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"


static xkbSetControlsReq *
_XkbGetSetControlsReq(Display *dpy, XkbInfoPtr xkbi, unsigned int deviceSpec)
{
    xkbSetControlsReq *req;

    GetReq(kbSetControls, req);
    bzero(req, SIZEOF(xkbSetControlsReq));
    req->reqType = xkbi->codes->major_opcode;
    req->length = (SIZEOF(xkbSetControlsReq) >> 2);
    req->xkbReqType = X_kbSetControls;
    req->deviceSpec = deviceSpec;
    return req;
}

Bool
XkbSetAutoRepeatRate(Display *dpy,
                     unsigned int deviceSpec,
                     unsigned int timeout,
                     unsigned int interval)
{
    register xkbSetControlsReq *req;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    req = _XkbGetSetControlsReq(dpy, dpy->xkb_info, deviceSpec);
    req->changeCtrls = XkbRepeatKeysMask;
    req->repeatDelay = timeout;
    req->repeatInterval = interval;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbGetAutoRepeatRate(Display *dpy,
                     unsigned int deviceSpec,
                     unsigned int *timeoutp,
                     unsigned int *intervalp)
{
    register xkbGetControlsReq *req;
    xkbGetControlsReply rep;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbGetControls, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbGetControls;
    req->deviceSpec = deviceSpec;
    if (!_XReply(dpy, (xReply *) &rep,
                 (SIZEOF(xkbGetControlsReply) - SIZEOF(xReply)) >> 2, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    *timeoutp = rep.repeatDelay;
    *intervalp = rep.repeatInterval;
    return True;
}

Bool
XkbSetServerInternalMods(Display *dpy,
                         unsigned deviceSpec,
                         unsigned affectReal,
                         unsigned realValues,
                         unsigned affectVirtual,
                         unsigned virtualValues)
{
    register xkbSetControlsReq *req;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    req = _XkbGetSetControlsReq(dpy, dpy->xkb_info, deviceSpec);
    req->affectInternalMods = affectReal;
    req->internalMods = realValues;
    req->affectInternalVMods = affectVirtual;
    req->internalVMods = virtualValues;
    req->changeCtrls = XkbInternalModsMask;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbSetIgnoreLockMods(Display *dpy,
                     unsigned int deviceSpec,
                     unsigned affectReal,
                     unsigned realValues,
                     unsigned affectVirtual,
                     unsigned virtualValues)
{
    register xkbSetControlsReq *req;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    req = _XkbGetSetControlsReq(dpy, dpy->xkb_info, deviceSpec);
    req->affectIgnoreLockMods = affectReal;
    req->ignoreLockMods = realValues;
    req->affectIgnoreLockVMods = affectVirtual;
    req->ignoreLockVMods = virtualValues;
    req->changeCtrls = XkbIgnoreLockModsMask;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbChangeEnabledControls(Display *dpy,
                         unsigned deviceSpec,
                         unsigned affect,
                         unsigned values)
{
    register xkbSetControlsReq *req;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    req = _XkbGetSetControlsReq(dpy, dpy->xkb_info, deviceSpec);
    req->affectEnabledCtrls = affect;
    req->enabledCtrls = (affect & values);
    req->changeCtrls = XkbControlsEnabledMask;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Status
XkbGetControls(Display *dpy, unsigned long which, XkbDescPtr xkb)
{
    register xkbGetControlsReq *req;
    xkbGetControlsReply rep;
    XkbControlsPtr ctrls;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;
    if ((!xkb) || (!which))
        return BadMatch;

    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbGetControls, req);
    if (!xkb->ctrls) {
        xkb->ctrls = _XkbTypedCalloc(1, XkbControlsRec);
        if (!xkb->ctrls) {
            UnlockDisplay(dpy);
            SyncHandle();
            return BadAlloc;
        }
    }
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbGetControls;
    req->deviceSpec = xkb->device_spec;
    if (!_XReply(dpy, (xReply *) &rep,
                 (SIZEOF(xkbGetControlsReply) - SIZEOF(xReply)) >> 2, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return BadImplementation;
    }
    if (xkb->device_spec == XkbUseCoreKbd)
        xkb->device_spec = rep.deviceID;
    ctrls = xkb->ctrls;
    if (which & XkbControlsEnabledMask)
        ctrls->enabled_ctrls = rep.enabledCtrls;
    ctrls->num_groups = rep.numGroups;
    if (which & XkbGroupsWrapMask)
        ctrls->groups_wrap = rep.groupsWrap;
    if (which & XkbInternalModsMask) {
        ctrls->internal.mask = rep.internalMods;
        ctrls->internal.real_mods = rep.internalRealMods;
        ctrls->internal.vmods = rep.internalVMods;
    }
    if (which & XkbIgnoreLockModsMask) {
        ctrls->ignore_lock.mask = rep.ignoreLockMods;
        ctrls->ignore_lock.real_mods = rep.ignoreLockRealMods;
        ctrls->ignore_lock.vmods = rep.ignoreLockVMods;
    }
    if (which & XkbRepeatKeysMask) {
        ctrls->repeat_delay = rep.repeatDelay;
        ctrls->repeat_interval = rep.repeatInterval;
    }
    if (which & XkbSlowKeysMask)
        ctrls->slow_keys_delay = rep.slowKeysDelay;
    if (which & XkbBounceKeysMask)
        ctrls->debounce_delay = rep.debounceDelay;
    if (which & XkbMouseKeysMask) {
        ctrls->mk_dflt_btn = rep.mkDfltBtn;
    }
    if (which & XkbMouseKeysAccelMask) {
        ctrls->mk_delay = rep.mkDelay;
        ctrls->mk_interval = rep.mkInterval;
        ctrls->mk_time_to_max = rep.mkTimeToMax;
        ctrls->mk_max_speed = rep.mkMaxSpeed;
        ctrls->mk_curve = rep.mkCurve;
    }
    if (which & XkbAccessXKeysMask)
        ctrls->ax_options = rep.axOptions;
    if (which & XkbStickyKeysMask) {
        ctrls->ax_options &= ~XkbAX_SKOptionsMask;
        ctrls->ax_options |= rep.axOptions & XkbAX_SKOptionsMask;
    }
    if (which & XkbAccessXFeedbackMask) {
        ctrls->ax_options &= ~XkbAX_FBOptionsMask;
        ctrls->ax_options |= rep.axOptions & XkbAX_FBOptionsMask;
    }
    if (which & XkbAccessXTimeoutMask) {
        ctrls->ax_timeout = rep.axTimeout;
        ctrls->axt_ctrls_mask = rep.axtCtrlsMask;
        ctrls->axt_ctrls_values = rep.axtCtrlsValues;
        ctrls->axt_opts_mask = rep.axtOptsMask;
        ctrls->axt_opts_values = rep.axtOptsValues;
    }
    if (which & XkbPerKeyRepeatMask) {
        memcpy(ctrls->per_key_repeat, rep.perKeyRepeat, XkbPerKeyBitArraySize);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return Success;
}

Bool
XkbSetControls(Display *dpy, unsigned long which, XkbDescPtr xkb)
{
    register xkbSetControlsReq *req;
    XkbControlsPtr ctrls;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    if ((!xkb) || (!xkb->ctrls))
        return False;

    ctrls = xkb->ctrls;
    LockDisplay(dpy);
    req = _XkbGetSetControlsReq(dpy, dpy->xkb_info, xkb->device_spec);
    req->changeCtrls = (CARD32) which;
    if (which & XkbInternalModsMask) {
        req->affectInternalMods = ~0;
        req->internalMods = ctrls->internal.real_mods;
        req->affectInternalVMods = ~0;
        req->internalVMods = ctrls->internal.vmods;
    }
    if (which & XkbIgnoreLockModsMask) {
        req->affectIgnoreLockMods = ~0;
        req->ignoreLockMods = ctrls->ignore_lock.real_mods;
        req->affectIgnoreLockVMods = ~0;
        req->ignoreLockVMods = ctrls->ignore_lock.vmods;
    }
    if (which & XkbControlsEnabledMask) {
        req->affectEnabledCtrls = XkbAllBooleanCtrlsMask;
        req->enabledCtrls = ctrls->enabled_ctrls;
    }
    if (which & XkbRepeatKeysMask) {
        req->repeatDelay = ctrls->repeat_delay;
        req->repeatInterval = ctrls->repeat_interval;
    }
    if (which & XkbSlowKeysMask)
        req->slowKeysDelay = ctrls->slow_keys_delay;
    if (which & XkbBounceKeysMask)
        req->debounceDelay = ctrls->debounce_delay;
    if (which & XkbMouseKeysMask) {
        req->mkDfltBtn = ctrls->mk_dflt_btn;
    }
    if (which & XkbGroupsWrapMask)
        req->groupsWrap = ctrls->groups_wrap;
    if (which &
        (XkbAccessXKeysMask | XkbStickyKeysMask | XkbAccessXFeedbackMask))
        req->axOptions = ctrls->ax_options;
    if (which & XkbMouseKeysAccelMask) {
        req->mkDelay = ctrls->mk_delay;
        req->mkInterval = ctrls->mk_interval;
        req->mkTimeToMax = ctrls->mk_time_to_max;
        req->mkMaxSpeed = ctrls->mk_max_speed;
        req->mkCurve = ctrls->mk_curve;
    }
    if (which & XkbAccessXTimeoutMask) {
        req->axTimeout = ctrls->ax_timeout;
        req->axtCtrlsMask = ctrls->axt_ctrls_mask;
        req->axtCtrlsValues = ctrls->axt_ctrls_values;
        req->axtOptsMask = ctrls->axt_opts_mask;
        req->axtOptsValues = ctrls->axt_opts_values;
    }
    if (which & XkbPerKeyRepeatMask) {
        memcpy(req->perKeyRepeat, ctrls->per_key_repeat, XkbPerKeyBitArraySize);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

/***====================================================================***/

void
XkbNoteControlsChanges(XkbControlsChangesPtr old,
                       XkbControlsNotifyEvent *new,
                       unsigned int wanted)
{
    old->changed_ctrls |= (new->changed_ctrls & wanted);
    if (new->changed_ctrls & XkbControlsEnabledMask & wanted)
        old->enabled_ctrls_changes ^= new->enabled_ctrl_changes;
    /* num_groups_changed?? */
    return;
}
