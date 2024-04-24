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

XkbInternAtomFunc       _XkbInternAtomFunc  = XInternAtom;
XkbGetAtomNameFunc      _XkbGetAtomNameFunc = XGetAtomName;

Bool
XkbQueryExtension(Display *dpy,
                  int *opcodeReturn,
                  int *eventBaseReturn,
                  int *errorBaseReturn,
                  int *majorReturn,
                  int *minorReturn)
{
    if (!XkbUseExtension(dpy, majorReturn, minorReturn))
        return False;
    if (opcodeReturn)
        *opcodeReturn = dpy->xkb_info->codes->major_opcode;
    if (eventBaseReturn)
        *eventBaseReturn = dpy->xkb_info->codes->first_event;
    if (errorBaseReturn)
        *errorBaseReturn = dpy->xkb_info->codes->first_error;
    if (majorReturn)
        *majorReturn = dpy->xkb_info->srv_major;
    if (minorReturn)
        *minorReturn = dpy->xkb_info->srv_minor;
    return True;
}

Bool
XkbLibraryVersion(int *libMajorRtrn, int *libMinorRtrn)
{
    int supported;

    if (*libMajorRtrn != XkbMajorVersion) {
        /* version 0.65 is (almost) compatible with 1.00 */
        if ((XkbMajorVersion == 1) &&
            (((*libMajorRtrn) == 0) && ((*libMinorRtrn) == 65)))
            supported = True;
        else
            supported = False;
    }
    else {
        supported = True;
    }

    *libMajorRtrn = XkbMajorVersion;
    *libMinorRtrn = XkbMinorVersion;
    return supported;
}

Bool
XkbSelectEvents(Display *dpy,
                unsigned int deviceSpec,
                unsigned int affect,
                unsigned int selectAll)
{
    register xkbSelectEventsReq *req;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    xkbi->selected_events &= ~affect;
    xkbi->selected_events |= (affect & selectAll);
    GetReq(kbSelectEvents, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSelectEvents;
    req->deviceSpec = deviceSpec;
    req->affectWhich = (CARD16) affect;
    req->clear = affect & (~selectAll);
    req->selectAll = affect & selectAll;
    if (affect & XkbMapNotifyMask) {
        req->affectMap = XkbAllMapComponentsMask;
        /* the implicit support needs the client info */
        /* even if the client itself doesn't want it */
        if (selectAll & XkbMapNotifyMask)
            req->map = XkbAllMapEventsMask;
        else
            req->map = XkbAllClientInfoMask;
        if (selectAll & XkbMapNotifyMask)
            xkbi->selected_map_details = XkbAllMapEventsMask;
        else
            xkbi->selected_map_details = 0;
    }
    if (affect & XkbNewKeyboardNotifyMask) {
        if (selectAll & XkbNewKeyboardNotifyMask)
            xkbi->selected_nkn_details = XkbAllNewKeyboardEventsMask;
        else
            xkbi->selected_nkn_details = 0;
        if (!(xkbi->xlib_ctrls & XkbLC_IgnoreNewKeyboards)) {
            /* we want it, even if the client doesn't.  Don't mess */
            /* around with details -- ask for all of them and throw */
            /* away the ones we don't need */
            req->selectAll |= XkbNewKeyboardNotifyMask;
        }
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbSelectEventDetails(Display *dpy,
                      unsigned deviceSpec,
                      unsigned eventType,
                      unsigned long int affect,
                      unsigned long int details)
{
    register xkbSelectEventsReq *req;
    XkbInfoPtr  xkbi;
    int         size = 0;
    char        *out;
    union {
        CARD8   *c8;
        CARD16  *c16;
        CARD32  *c32;
    } u;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    if (affect & details)
        xkbi->selected_events |= (1 << eventType);
    else
        xkbi->selected_events &= ~(1 << eventType);
    GetReq(kbSelectEvents, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSelectEvents;
    req->deviceSpec = deviceSpec;
    req->clear = req->selectAll = 0;
    if (eventType == XkbMapNotify) {
        /* we need all of the client info, even if the application */
        /* doesn't.   Make sure that we always request the stuff */
        /* that the implicit support needs, and just filter out anything */
        /* the client doesn't want later */
        req->affectMap = (CARD16) affect;
        req->map = (CARD16) details | (XkbAllClientInfoMask & affect);
        req->affectWhich = XkbMapNotifyMask;
        xkbi->selected_map_details &= ~affect;
        xkbi->selected_map_details |= (details & affect);
    }
    else {
        req->affectMap = req->map = 0;
        req->affectWhich = (1 << eventType);
        switch (eventType) {
        case XkbNewKeyboardNotify:
            xkbi->selected_nkn_details &= ~affect;
            xkbi->selected_nkn_details |= (details & affect);
            if (!(xkbi->xlib_ctrls & XkbLC_IgnoreNewKeyboards))
                details = (affect & XkbAllNewKeyboardEventsMask);
        case XkbStateNotify:
        case XkbNamesNotify:
        case XkbAccessXNotify:
        case XkbExtensionDeviceNotify:
            size = 2;
            req->length += 1;
            break;
        case XkbControlsNotify:
        case XkbIndicatorStateNotify:
        case XkbIndicatorMapNotify:
            size = 4;
            req->length += 2;
            break;
        case XkbBellNotify:
        case XkbActionMessage:
        case XkbCompatMapNotify:
            size = 1;
            req->length += 1;
            break;
        }
        BufAlloc(char *, out, (((size * 2) + (unsigned) 3) / 4) * 4);

        u.c8 = (CARD8 *) out;
        if (size == 2) {
            u.c16[0] = (CARD16) affect;
            u.c16[1] = (CARD16) details;
        }
        else if (size == 4) {
            u.c32[0] = (CARD32) affect;
            u.c32[1] = (CARD32) details;
        }
        else {
            u.c8[0] = (CARD8) affect;
            u.c8[1] = (CARD8) details;
        }
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbLockModifiers(Display *dpy,
                 unsigned int deviceSpec,
                 unsigned int affect,
                 unsigned int values)
{
    register xkbLatchLockStateReq *req;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbLatchLockState, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbLatchLockState;
    req->deviceSpec = deviceSpec;
    req->affectModLocks = affect;
    req->modLocks = values;
    req->lockGroup = False;
    req->groupLock = 0;

    req->affectModLatches = req->modLatches = 0;
    req->latchGroup = False;
    req->groupLatch = 0;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbLatchModifiers(Display *dpy,
                  unsigned int deviceSpec,
                  unsigned int affect,
                  unsigned int values)
{
    register xkbLatchLockStateReq *req;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbLatchLockState, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbLatchLockState;
    req->deviceSpec = deviceSpec;

    req->affectModLatches = affect;
    req->modLatches = values;
    req->latchGroup = False;
    req->groupLatch = 0;

    req->affectModLocks = req->modLocks = 0;
    req->lockGroup = False;
    req->groupLock = 0;

    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbLockGroup(Display *dpy, unsigned int deviceSpec, unsigned int group)
{
    register xkbLatchLockStateReq *req;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbLatchLockState, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbLatchLockState;
    req->deviceSpec = deviceSpec;
    req->affectModLocks = 0;
    req->modLocks = 0;
    req->lockGroup = True;
    req->groupLock = group;

    req->affectModLatches = req->modLatches = 0;
    req->latchGroup = False;
    req->groupLatch = 0;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbLatchGroup(Display *dpy, unsigned int deviceSpec, unsigned int group)
{
    register xkbLatchLockStateReq *req;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbLatchLockState, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbLatchLockState;
    req->deviceSpec = deviceSpec;

    req->affectModLatches = 0;
    req->modLatches = 0;
    req->latchGroup = True;
    req->groupLatch = group;

    req->affectModLocks = req->modLocks = 0;
    req->lockGroup = False;
    req->groupLock = 0;

    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

unsigned
XkbSetXlibControls(Display *dpy, unsigned affect, unsigned values)
{
    if (!dpy->xkb_info)
        XkbUseExtension(dpy, NULL, NULL);
    if (!dpy->xkb_info)
        return 0;
    affect &= XkbLC_AllControls;
    dpy->xkb_info->xlib_ctrls &= ~affect;
    dpy->xkb_info->xlib_ctrls |= (affect & values);
    return dpy->xkb_info->xlib_ctrls;
}

unsigned
XkbGetXlibControls(Display *dpy)
{
    if (!dpy->xkb_info)
        XkbUseExtension(dpy, NULL, NULL);
    if (!dpy->xkb_info)
        return 0;
    return dpy->xkb_info->xlib_ctrls;
}

unsigned int
XkbXlibControlsImplemented(void)
{
#ifdef __sgi
    return XkbLC_AllControls;
#else
    return XkbLC_AllControls & ~XkbLC_AllComposeControls;
#endif
}

Bool
XkbSetDebuggingFlags(Display *dpy,
                     unsigned int mask,
                     unsigned int flags,
                     char *msg,
                     unsigned int ctrls_mask,
                     unsigned int ctrls,
                     unsigned int *rtrn_flags,
                     unsigned int *rtrn_ctrls)
{
    register xkbSetDebuggingFlagsReq *req;
    xkbSetDebuggingFlagsReply rep;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetDebuggingFlags, req);
    req->reqType        = xkbi->codes->major_opcode;
    req->xkbReqType     = X_kbSetDebuggingFlags;
    req->affectFlags    = mask;
    req->flags          = flags;
    req->affectCtrls    = ctrls_mask;
    req->ctrls          = ctrls;

    if (msg) {
        char *out;

        req->msgLength = (CARD16) (strlen(msg) + 1);
        req->length += (req->msgLength + (unsigned) 3) >> 2;
        BufAlloc(char *, out, ((req->msgLength + (unsigned) 3) / 4) * 4);
        memcpy(out, msg, req->msgLength);
    }
    else
        req->msgLength = 0;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return False;
    }
    if (rtrn_flags)
        *rtrn_flags = rep.currentFlags;
    if (rtrn_ctrls)
        *rtrn_ctrls = rep.currentCtrls;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbComputeEffectiveMap(XkbDescPtr xkb,
                       XkbKeyTypePtr type,
                       unsigned char *map_rtrn)
{
    register int i;
    unsigned tmp;
    XkbKTMapEntryPtr entry = NULL;

    if ((!xkb) || (!type) || (!xkb->server))
        return False;

    if (type->mods.vmods != 0) {
        if (!XkbVirtualModsToReal(xkb, type->mods.vmods, &tmp))
            return False;

        type->mods.mask = tmp | type->mods.real_mods;
        entry = type->map;
        for (i = 0; i < type->map_count; i++, entry++) {
            tmp = 0;
            if (entry->mods.vmods != 0) {
                if (!XkbVirtualModsToReal(xkb, entry->mods.vmods, &tmp))
                    return False;
                if (tmp == 0) {
                    entry->active = False;
                    continue;
                }
            }
            entry->active = True;
            entry->mods.mask = (entry->mods.real_mods | tmp) & type->mods.mask;
        }
    }
    else {
        type->mods.mask = type->mods.real_mods;
    }
    if (map_rtrn != NULL) {
        bzero(map_rtrn, type->mods.mask + 1);
        for (i = 0; i < type->map_count; i++) {
            if (entry && entry->active) {
                map_rtrn[type->map[i].mods.mask] = type->map[i].level;
            }
        }
    }
    return True;
}

Status
XkbGetState(Display *dpy, unsigned deviceSpec, XkbStatePtr rtrn)
{
    register xkbGetStateReq *req;
    xkbGetStateReply rep;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbGetState, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbGetState;
    req->deviceSpec = deviceSpec;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return BadImplementation;
    }
    rtrn->mods                  = rep.mods;
    rtrn->base_mods             = rep.baseMods;
    rtrn->latched_mods          = rep.latchedMods;
    rtrn->locked_mods           = rep.lockedMods;
    rtrn->group                 = rep.group;
    rtrn->base_group            = rep.baseGroup;
    rtrn->latched_group         = rep.latchedGroup;
    rtrn->locked_group          = rep.lockedGroup;
    rtrn->compat_state          = rep.compatState;
    rtrn->grab_mods             = rep.grabMods;
    rtrn->compat_grab_mods      = rep.compatGrabMods;
    rtrn->lookup_mods           = rep.lookupMods;
    rtrn->compat_lookup_mods    = rep.compatLookupMods;
    rtrn->ptr_buttons           = rep.ptrBtnState;
    UnlockDisplay(dpy);
    SyncHandle();
    return Success;
}

Bool
XkbSetDetectableAutoRepeat(Display *dpy, Bool detectable, Bool *supported)
{
    register xkbPerClientFlagsReq *req;
    xkbPerClientFlagsReply rep;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbPerClientFlags, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbPerClientFlags;
    req->deviceSpec = XkbUseCoreKbd;
    req->change = XkbPCF_DetectableAutoRepeatMask;
    if (detectable)
        req->value = XkbPCF_DetectableAutoRepeatMask;
    else
        req->value = 0;
    req->ctrlsToChange = req->autoCtrls = req->autoCtrlValues = 0;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    if (supported != NULL)
        *supported = ((rep.supported & XkbPCF_DetectableAutoRepeatMask) != 0);
    return ((rep.value & XkbPCF_DetectableAutoRepeatMask) != 0);
}

Bool
XkbGetDetectableAutoRepeat(Display *dpy, Bool *supported)
{
    register xkbPerClientFlagsReq *req;
    xkbPerClientFlagsReply rep;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbPerClientFlags, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbPerClientFlags;
    req->deviceSpec = XkbUseCoreKbd;
    req->change = 0;
    req->value = 0;
    req->ctrlsToChange = req->autoCtrls = req->autoCtrlValues = 0;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    if (supported != NULL)
        *supported = ((rep.supported & XkbPCF_DetectableAutoRepeatMask) != 0);
    return ((rep.value & XkbPCF_DetectableAutoRepeatMask) != 0);
}

Bool
XkbSetAutoResetControls(Display *dpy,
                        unsigned changes,
                        unsigned *auto_ctrls,
                        unsigned *auto_values)
{
    register xkbPerClientFlagsReq *req;
    xkbPerClientFlagsReply rep;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbPerClientFlags, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbPerClientFlags;
    req->change = XkbPCF_AutoResetControlsMask;
    req->deviceSpec = XkbUseCoreKbd;
    req->value = XkbPCF_AutoResetControlsMask;
    req->ctrlsToChange = changes;
    req->autoCtrls = *auto_ctrls;
    req->autoCtrlValues = *auto_values;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    *auto_ctrls = rep.autoCtrls;
    *auto_values = rep.autoCtrlValues;
    return ((rep.value & XkbPCF_AutoResetControlsMask) != 0);
}

Bool
XkbGetAutoResetControls(Display *dpy,
                        unsigned *auto_ctrls,
                        unsigned *auto_ctrl_values)
{
    register xkbPerClientFlagsReq *req;
    xkbPerClientFlagsReply rep;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbPerClientFlags, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbPerClientFlags;
    req->deviceSpec = XkbUseCoreKbd;
    req->change = 0;
    req->value = 0;
    req->ctrlsToChange = req->autoCtrls = req->autoCtrlValues = 0;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    if (auto_ctrls)
        *auto_ctrls = rep.autoCtrls;
    if (auto_ctrl_values)
        *auto_ctrl_values = rep.autoCtrlValues;
    return ((rep.value & XkbPCF_AutoResetControlsMask) != 0);
}

Bool
XkbSetPerClientControls(Display *dpy, unsigned change, unsigned *values)
{
    register xkbPerClientFlagsReq *req;
    xkbPerClientFlagsReply rep;
    XkbInfoPtr xkbi;
    unsigned value_hold = *values;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)) ||
        (change & ~(XkbPCF_GrabsUseXKBStateMask |
                    XkbPCF_LookupStateWhenGrabbed |
                    XkbPCF_SendEventUsesXKBState)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbPerClientFlags, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbPerClientFlags;
    req->change = change;
    req->deviceSpec = XkbUseCoreKbd;
    req->value = *values;
    req->ctrlsToChange = req->autoCtrls = req->autoCtrlValues = 0;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    *values = rep.value;
    return ((rep.value & value_hold) != 0);
}

Bool
XkbGetPerClientControls(Display *dpy, unsigned *ctrls)
{
    register xkbPerClientFlagsReq *req;
    xkbPerClientFlagsReply rep;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)) ||
        (ctrls == NULL))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbPerClientFlags, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbPerClientFlags;
    req->deviceSpec = XkbUseCoreKbd;
    req->change = 0;
    req->value = 0;
    req->ctrlsToChange = req->autoCtrls = req->autoCtrlValues = 0;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    *ctrls = (rep.value & (XkbPCF_GrabsUseXKBStateMask |
                           XkbPCF_LookupStateWhenGrabbed |
                           XkbPCF_SendEventUsesXKBState));
    return (True);
}

Display *
XkbOpenDisplay(_Xconst char *name,
               int *ev_rtrn,
               int *err_rtrn,
               int *major_rtrn,
               int *minor_rtrn,
               int *reason)
{
    Display *dpy;
    int major_num, minor_num;

    if ((major_rtrn != NULL) && (minor_rtrn != NULL)) {
        if (!XkbLibraryVersion(major_rtrn, minor_rtrn)) {
            if (reason != NULL)
                *reason = XkbOD_BadLibraryVersion;
            return NULL;
        }
    }
    else {
        major_num = XkbMajorVersion;
        minor_num = XkbMinorVersion;
        major_rtrn = &major_num;
        minor_rtrn = &minor_num;
    }
    dpy = XOpenDisplay(name);
    if (dpy == NULL) {
        if (reason != NULL)
            *reason = XkbOD_ConnectionRefused;
        return NULL;
    }
    if (!XkbQueryExtension(dpy, NULL, ev_rtrn, err_rtrn,
                           major_rtrn, minor_rtrn)) {
        if (reason != NULL) {
            if ((*major_rtrn != 0) || (*minor_rtrn != 0))
                *reason = XkbOD_BadServerVersion;
            else
                *reason = XkbOD_NonXkbServer;
        }
        XCloseDisplay(dpy);
        return NULL;
    }
    if (reason != NULL)
        *reason = XkbOD_Success;
    return dpy;
}

void
XkbSetAtomFuncs(XkbInternAtomFunc getAtom, XkbGetAtomNameFunc getName)
{
    _XkbInternAtomFunc = (getAtom ? getAtom : XInternAtom);
    _XkbGetAtomNameFunc = (getName ? getName : XGetAtomName);
    return;
}
