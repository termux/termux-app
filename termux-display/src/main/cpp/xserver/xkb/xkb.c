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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "inputstr.h"
#define	XKBSRV_NEED_FILE_FUNCS
#include <xkbsrv.h>
#include "extnsionst.h"
#include "extinit.h"
#include "xace.h"
#include "xkb.h"
#include "protocol-versions.h"

#include <X11/extensions/XI.h>
#include <X11/extensions/XKMformat.h>

int XkbEventBase;
static int XkbErrorBase;
int XkbReqCode;
int XkbKeyboardErrorCode;
CARD32 xkbDebugFlags = 0;
static CARD32 xkbDebugCtrls = 0;

static RESTYPE RT_XKBCLIENT;

/***====================================================================***/

#define	CHK_DEVICE(dev, id, client, access_mode, lf) {\
    int why;\
    int tmprc = lf(&(dev), id, client, access_mode, &why);\
    if (tmprc != Success) {\
	client->errorValue = _XkbErrCode2(why, id);\
	return tmprc;\
    }\
}

#define	CHK_KBD_DEVICE(dev, id, client, mode) \
    CHK_DEVICE(dev, id, client, mode, _XkbLookupKeyboard)
#define	CHK_LED_DEVICE(dev, id, client, mode) \
    CHK_DEVICE(dev, id, client, mode, _XkbLookupLedDevice)
#define	CHK_BELL_DEVICE(dev, id, client, mode) \
    CHK_DEVICE(dev, id, client, mode, _XkbLookupBellDevice)
#define	CHK_ANY_DEVICE(dev, id, client, mode) \
    CHK_DEVICE(dev, id, client, mode, _XkbLookupAnyDevice)

#define	CHK_ATOM_ONLY2(a,ev,er) {\
	if (((a)==None)||(!ValidAtom((a)))) {\
	    (ev)= (XID)(a);\
	    return er;\
	}\
}
#define	CHK_ATOM_ONLY(a) \
	CHK_ATOM_ONLY2(a,client->errorValue,BadAtom)

#define	CHK_ATOM_OR_NONE3(a,ev,er,ret) {\
	if (((a)!=None)&&(!ValidAtom((a)))) {\
	    (ev)= (XID)(a);\
	    (er)= BadAtom;\
	    return ret;\
	}\
}
#define	CHK_ATOM_OR_NONE2(a,ev,er) {\
	if (((a)!=None)&&(!ValidAtom((a)))) {\
	    (ev)= (XID)(a);\
	    return er;\
	}\
}
#define	CHK_ATOM_OR_NONE(a) \
	CHK_ATOM_OR_NONE2(a,client->errorValue,BadAtom)

#define	CHK_MASK_LEGAL3(err,mask,legal,ev,er,ret)	{\
	if ((mask)&(~(legal))) { \
	    (ev)= _XkbErrCode2((err),((mask)&(~(legal))));\
	    (er)= BadValue;\
	    return ret;\
	}\
}
#define	CHK_MASK_LEGAL2(err,mask,legal,ev,er)	{\
	if ((mask)&(~(legal))) { \
	    (ev)= _XkbErrCode2((err),((mask)&(~(legal))));\
	    return er;\
	}\
}
#define	CHK_MASK_LEGAL(err,mask,legal) \
	CHK_MASK_LEGAL2(err,mask,legal,client->errorValue,BadValue)

#define	CHK_MASK_MATCH(err,affect,value) {\
	if ((value)&(~(affect))) { \
	    client->errorValue= _XkbErrCode2((err),((value)&(~(affect))));\
	    return BadMatch;\
	}\
}
#define	CHK_MASK_OVERLAP(err,m1,m2) {\
	if ((m1)&(m2)) { \
	    client->errorValue= _XkbErrCode2((err),((m1)&(m2)));\
	    return BadMatch;\
	}\
}
#define	CHK_KEY_RANGE2(err,first,num,x,ev,er) {\
	if (((unsigned)(first)+(num)-1)>(x)->max_key_code) {\
	    (ev)=_XkbErrCode4(err,(first),(num),(x)->max_key_code);\
	    return er;\
	}\
	else if ( (first)<(x)->min_key_code ) {\
	    (ev)=_XkbErrCode3(err+1,(first),xkb->min_key_code);\
	    return er;\
	}\
}
#define	CHK_KEY_RANGE(err,first,num,x)  \
	CHK_KEY_RANGE2(err,first,num,x,client->errorValue,BadValue)

#define	CHK_REQ_KEY_RANGE2(err,first,num,r,ev,er) {\
	if (((unsigned)(first)+(num)-1)>(r)->maxKeyCode) {\
	    (ev)=_XkbErrCode4(err,(first),(num),(r)->maxKeyCode);\
	    return er;\
	}\
	else if ( (first)<(r)->minKeyCode ) {\
	    (ev)=_XkbErrCode3(err+1,(first),(r)->minKeyCode);\
	    return er;\
	}\
}
#define	CHK_REQ_KEY_RANGE(err,first,num,r)  \
	CHK_REQ_KEY_RANGE2(err,first,num,r,client->errorValue,BadValue)

static Bool
_XkbCheckRequestBounds(ClientPtr client, void *stuff, void *from, void *to) {
    char *cstuff = (char *)stuff;
    char *cfrom = (char *)from;
    char *cto = (char *)to;

    return cfrom < cto &&
           cfrom >= cstuff &&
           cfrom < cstuff + ((size_t)client->req_len << 2) &&
           cto >= cstuff &&
           cto <= cstuff + ((size_t)client->req_len << 2);
}

/***====================================================================***/

int
ProcXkbUseExtension(ClientPtr client)
{
    REQUEST(xkbUseExtensionReq);
    xkbUseExtensionReply rep;
    int supported;

    REQUEST_SIZE_MATCH(xkbUseExtensionReq);
    if (stuff->wantedMajor != SERVER_XKB_MAJOR_VERSION) {
        /* pre-release version 0.65 is compatible with 1.00 */
        supported = ((SERVER_XKB_MAJOR_VERSION == 1) &&
                     (stuff->wantedMajor == 0) && (stuff->wantedMinor == 65));
    }
    else
        supported = 1;

    if ((supported) && (!(client->xkbClientFlags & _XkbClientInitialized))) {
        client->xkbClientFlags = _XkbClientInitialized;
        if (stuff->wantedMajor == 0)
            client->xkbClientFlags |= _XkbClientIsAncient;
    }
    else if (xkbDebugFlags & 0x1) {
        ErrorF
            ("[xkb] Rejecting client %d (0x%lx) (wants %d.%02d, have %d.%02d)\n",
             client->index, (long) client->clientAsMask, stuff->wantedMajor,
             stuff->wantedMinor, SERVER_XKB_MAJOR_VERSION,
             SERVER_XKB_MINOR_VERSION);
    }
    rep = (xkbUseExtensionReply) {
        .type = X_Reply,
        .supported = supported,
        .sequenceNumber = client->sequence,
        .length = 0,
        .serverMajor = SERVER_XKB_MAJOR_VERSION,
        .serverMinor = SERVER_XKB_MINOR_VERSION
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swaps(&rep.serverMajor);
        swaps(&rep.serverMinor);
    }
    WriteToClient(client, SIZEOF(xkbUseExtensionReply), &rep);
    return Success;
}

/***====================================================================***/

int
ProcXkbSelectEvents(ClientPtr client)
{
    unsigned legal;
    DeviceIntPtr dev;
    XkbInterestPtr masks;

    REQUEST(xkbSelectEventsReq);

    REQUEST_AT_LEAST_SIZE(xkbSelectEventsReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_ANY_DEVICE(dev, stuff->deviceSpec, client, DixUseAccess);

    if (((stuff->affectWhich & XkbMapNotifyMask) != 0) && (stuff->affectMap)) {
        client->mapNotifyMask &= ~stuff->affectMap;
        client->mapNotifyMask |= (stuff->affectMap & stuff->map);
    }
    if ((stuff->affectWhich & (~XkbMapNotifyMask)) == 0)
        return Success;

    masks = XkbFindClientResource((DevicePtr) dev, client);
    if (!masks) {
        XID id = FakeClientID(client->index);

        if (!AddResource(id, RT_XKBCLIENT, dev))
            return BadAlloc;
        masks = XkbAddClientResource((DevicePtr) dev, client, id);
    }
    if (masks) {
        union {
            CARD8 *c8;
            CARD16 *c16;
            CARD32 *c32;
        } from, to;
        register unsigned bit, ndx, maskLeft, dataLeft, size;

        from.c8 = (CARD8 *) &stuff[1];
        dataLeft = (stuff->length * 4) - SIZEOF(xkbSelectEventsReq);
        maskLeft = (stuff->affectWhich & (~XkbMapNotifyMask));
        for (ndx = 0, bit = 1; (maskLeft != 0); ndx++, bit <<= 1) {
            if ((bit & maskLeft) == 0)
                continue;
            maskLeft &= ~bit;
            switch (ndx) {
            case XkbNewKeyboardNotify:
                to.c16 = &client->newKeyboardNotifyMask;
                legal = XkbAllNewKeyboardEventsMask;
                size = 2;
                break;
            case XkbStateNotify:
                to.c16 = &masks->stateNotifyMask;
                legal = XkbAllStateEventsMask;
                size = 2;
                break;
            case XkbControlsNotify:
                to.c32 = &masks->ctrlsNotifyMask;
                legal = XkbAllControlEventsMask;
                size = 4;
                break;
            case XkbIndicatorStateNotify:
                to.c32 = &masks->iStateNotifyMask;
                legal = XkbAllIndicatorEventsMask;
                size = 4;
                break;
            case XkbIndicatorMapNotify:
                to.c32 = &masks->iMapNotifyMask;
                legal = XkbAllIndicatorEventsMask;
                size = 4;
                break;
            case XkbNamesNotify:
                to.c16 = &masks->namesNotifyMask;
                legal = XkbAllNameEventsMask;
                size = 2;
                break;
            case XkbCompatMapNotify:
                to.c8 = &masks->compatNotifyMask;
                legal = XkbAllCompatMapEventsMask;
                size = 1;
                break;
            case XkbBellNotify:
                to.c8 = &masks->bellNotifyMask;
                legal = XkbAllBellEventsMask;
                size = 1;
                break;
            case XkbActionMessage:
                to.c8 = &masks->actionMessageMask;
                legal = XkbAllActionMessagesMask;
                size = 1;
                break;
            case XkbAccessXNotify:
                to.c16 = &masks->accessXNotifyMask;
                legal = XkbAllAccessXEventsMask;
                size = 2;
                break;
            case XkbExtensionDeviceNotify:
                to.c16 = &masks->extDevNotifyMask;
                legal = XkbAllExtensionDeviceEventsMask;
                size = 2;
                break;
            default:
                client->errorValue = _XkbErrCode2(33, bit);
                return BadValue;
            }

            if (stuff->clear & bit) {
                if (size == 2)
                    to.c16[0] = 0;
                else if (size == 4)
                    to.c32[0] = 0;
                else
                    to.c8[0] = 0;
            }
            else if (stuff->selectAll & bit) {
                if (size == 2)
                    to.c16[0] = ~0;
                else if (size == 4)
                    to.c32[0] = ~0;
                else
                    to.c8[0] = ~0;
            }
            else {
                if (dataLeft < (size * 2))
                    return BadLength;
                if (size == 2) {
                    CHK_MASK_MATCH(ndx, from.c16[0], from.c16[1]);
                    CHK_MASK_LEGAL(ndx, from.c16[0], legal);
                    to.c16[0] &= ~from.c16[0];
                    to.c16[0] |= (from.c16[0] & from.c16[1]);
                }
                else if (size == 4) {
                    CHK_MASK_MATCH(ndx, from.c32[0], from.c32[1]);
                    CHK_MASK_LEGAL(ndx, from.c32[0], legal);
                    to.c32[0] &= ~from.c32[0];
                    to.c32[0] |= (from.c32[0] & from.c32[1]);
                }
                else {
                    CHK_MASK_MATCH(ndx, from.c8[0], from.c8[1]);
                    CHK_MASK_LEGAL(ndx, from.c8[0], legal);
                    to.c8[0] &= ~from.c8[0];
                    to.c8[0] |= (from.c8[0] & from.c8[1]);
                    size = 2;
                }
                from.c8 += (size * 2);
                dataLeft -= (size * 2);
            }
        }
        if (dataLeft > 2) {
            ErrorF("[xkb] Extra data (%d bytes) after SelectEvents\n",
                   dataLeft);
            return BadLength;
        }
        return Success;
    }
    return BadAlloc;
}

/***====================================================================***/
/**
 * Ring a bell on the given device for the given client.
 */
static int
_XkbBell(ClientPtr client, DeviceIntPtr dev, WindowPtr pWin,
         int bellClass, int bellID, int pitch, int duration,
         int percent, int forceSound, int eventOnly, Atom name)
{
    int base;
    void *ctrl;
    int oldPitch, oldDuration;
    int newPercent;

    if (bellClass == KbdFeedbackClass) {
        KbdFeedbackPtr k;

        if (bellID == XkbDfltXIId)
            k = dev->kbdfeed;
        else {
            for (k = dev->kbdfeed; k; k = k->next) {
                if (k->ctrl.id == bellID)
                    break;
            }
        }
        if (!k) {
            client->errorValue = _XkbErrCode2(0x5, bellID);
            return BadValue;
        }
        base = k->ctrl.bell;
        ctrl = (void *) &(k->ctrl);
        oldPitch = k->ctrl.bell_pitch;
        oldDuration = k->ctrl.bell_duration;
        if (pitch != 0) {
            if (pitch == -1)
                k->ctrl.bell_pitch = defaultKeyboardControl.bell_pitch;
            else
                k->ctrl.bell_pitch = pitch;
        }
        if (duration != 0) {
            if (duration == -1)
                k->ctrl.bell_duration = defaultKeyboardControl.bell_duration;
            else
                k->ctrl.bell_duration = duration;
        }
    }
    else if (bellClass == BellFeedbackClass) {
        BellFeedbackPtr b;

        if (bellID == XkbDfltXIId)
            b = dev->bell;
        else {
            for (b = dev->bell; b; b = b->next) {
                if (b->ctrl.id == bellID)
                    break;
            }
        }
        if (!b) {
            client->errorValue = _XkbErrCode2(0x6, bellID);
            return BadValue;
        }
        base = b->ctrl.percent;
        ctrl = (void *) &(b->ctrl);
        oldPitch = b->ctrl.pitch;
        oldDuration = b->ctrl.duration;
        if (pitch != 0) {
            if (pitch == -1)
                b->ctrl.pitch = defaultKeyboardControl.bell_pitch;
            else
                b->ctrl.pitch = pitch;
        }
        if (duration != 0) {
            if (duration == -1)
                b->ctrl.duration = defaultKeyboardControl.bell_duration;
            else
                b->ctrl.duration = duration;
        }
    }
    else {
        client->errorValue = _XkbErrCode2(0x7, bellClass);
        return BadValue;
    }

    newPercent = (base * percent) / 100;
    if (percent < 0)
        newPercent = base + newPercent;
    else
        newPercent = base - newPercent + percent;

    XkbHandleBell(forceSound, eventOnly,
                  dev, newPercent, ctrl, bellClass, name, pWin, client);
    if ((pitch != 0) || (duration != 0)) {
        if (bellClass == KbdFeedbackClass) {
            KbdFeedbackPtr k;

            k = (KbdFeedbackPtr) ctrl;
            if (pitch != 0)
                k->ctrl.bell_pitch = oldPitch;
            if (duration != 0)
                k->ctrl.bell_duration = oldDuration;
        }
        else {
            BellFeedbackPtr b;

            b = (BellFeedbackPtr) ctrl;
            if (pitch != 0)
                b->ctrl.pitch = oldPitch;
            if (duration != 0)
                b->ctrl.duration = oldDuration;
        }
    }

    return Success;
}

int
ProcXkbBell(ClientPtr client)
{
    REQUEST(xkbBellReq);
    DeviceIntPtr dev;
    WindowPtr pWin;
    int rc;

    REQUEST_SIZE_MATCH(xkbBellReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_BELL_DEVICE(dev, stuff->deviceSpec, client, DixBellAccess);
    CHK_ATOM_OR_NONE(stuff->name);

    /* device-independent checks request for sane values */
    if ((stuff->forceSound) && (stuff->eventOnly)) {
        client->errorValue =
            _XkbErrCode3(0x1, stuff->forceSound, stuff->eventOnly);
        return BadMatch;
    }
    if (stuff->percent < -100 || stuff->percent > 100) {
        client->errorValue = _XkbErrCode2(0x2, stuff->percent);
        return BadValue;
    }
    if (stuff->duration < -1) {
        client->errorValue = _XkbErrCode2(0x3, stuff->duration);
        return BadValue;
    }
    if (stuff->pitch < -1) {
        client->errorValue = _XkbErrCode2(0x4, stuff->pitch);
        return BadValue;
    }

    if (stuff->bellClass == XkbDfltXIClass) {
        if (dev->kbdfeed != NULL)
            stuff->bellClass = KbdFeedbackClass;
        else
            stuff->bellClass = BellFeedbackClass;
    }

    if (stuff->window != None) {
        rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
        if (rc != Success) {
            client->errorValue = stuff->window;
            return rc;
        }
    }
    else
        pWin = NULL;

    /* Client wants to ring a bell on the core keyboard?
       Ring the bell on the core keyboard (which does nothing, but if that
       fails the client is screwed anyway), and then on all extension devices.
       Fail if the core keyboard fails but not the extension devices.  this
       may cause some keyboards to ding and others to stay silent. Fix
       your client to use explicit keyboards to avoid this.

       dev is the device the client requested.
     */
    rc = _XkbBell(client, dev, pWin, stuff->bellClass, stuff->bellID,
                  stuff->pitch, stuff->duration, stuff->percent,
                  stuff->forceSound, stuff->eventOnly, stuff->name);

    if ((rc == Success) && ((stuff->deviceSpec == XkbUseCoreKbd) ||
                            (stuff->deviceSpec == XkbUseCorePtr))) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {
                rc = XaceHook(XACE_DEVICE_ACCESS, client, other, DixBellAccess);
                if (rc == Success)
                    _XkbBell(client, other, pWin, stuff->bellClass,
                             stuff->bellID, stuff->pitch, stuff->duration,
                             stuff->percent, stuff->forceSound,
                             stuff->eventOnly, stuff->name);
            }
        }
        rc = Success;           /* reset to success, that's what we got for the VCK */
    }

    return rc;
}

/***====================================================================***/

int
ProcXkbGetState(ClientPtr client)
{
    REQUEST(xkbGetStateReq);
    DeviceIntPtr dev;
    xkbGetStateReply rep;
    XkbStateRec *xkb;

    REQUEST_SIZE_MATCH(xkbGetStateReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixGetAttrAccess);

    xkb = &dev->key->xkbInfo->state;
    rep = (xkbGetStateReply) {
        .type = X_Reply,
        .deviceID = dev->id,
        .sequenceNumber = client->sequence,
        .length = 0,
        .mods = XkbStateFieldFromRec(xkb) & 0xff,
        .baseMods = xkb->base_mods,
        .latchedMods = xkb->latched_mods,
        .lockedMods = xkb->locked_mods,
        .group = xkb->group,
        .lockedGroup = xkb->locked_group,
        .baseGroup = xkb->base_group,
        .latchedGroup = xkb->latched_group,
        .compatState = xkb->compat_state,
        .ptrBtnState = xkb->ptr_buttons
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swaps(&rep.ptrBtnState);
    }
    WriteToClient(client, SIZEOF(xkbGetStateReply), &rep);
    return Success;
}

/***====================================================================***/

int
ProcXkbLatchLockState(ClientPtr client)
{
    int status;
    DeviceIntPtr dev, tmpd;
    XkbStateRec oldState, *newState;
    CARD16 changed;
    xkbStateNotify sn;
    XkbEventCauseRec cause;

    REQUEST(xkbLatchLockStateReq);
    REQUEST_SIZE_MATCH(xkbLatchLockStateReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixSetAttrAccess);
    CHK_MASK_MATCH(0x01, stuff->affectModLocks, stuff->modLocks);
    CHK_MASK_MATCH(0x01, stuff->affectModLatches, stuff->modLatches);

    status = Success;

    for (tmpd = inputInfo.devices; tmpd; tmpd = tmpd->next) {
        if ((tmpd == dev) ||
            (!IsMaster(tmpd) && GetMaster(tmpd, MASTER_KEYBOARD) == dev)) {
            if (!tmpd->key || !tmpd->key->xkbInfo)
                continue;

            oldState = tmpd->key->xkbInfo->state;
            newState = &tmpd->key->xkbInfo->state;
            if (stuff->affectModLocks) {
                newState->locked_mods &= ~stuff->affectModLocks;
                newState->locked_mods |=
                    (stuff->affectModLocks & stuff->modLocks);
            }
            if (status == Success && stuff->lockGroup)
                newState->locked_group = stuff->groupLock;
            if (status == Success && stuff->affectModLatches)
                status = XkbLatchModifiers(tmpd, stuff->affectModLatches,
                                           stuff->modLatches);
            if (status == Success && stuff->latchGroup)
                status = XkbLatchGroup(tmpd, stuff->groupLatch);

            if (status != Success)
                return status;

            XkbComputeDerivedState(tmpd->key->xkbInfo);

            changed = XkbStateChangedFlags(&oldState, newState);
            if (changed) {
                sn.keycode = 0;
                sn.eventType = 0;
                sn.requestMajor = XkbReqCode;
                sn.requestMinor = X_kbLatchLockState;
                sn.changed = changed;
                XkbSendStateNotify(tmpd, &sn);
                changed = XkbIndicatorsToUpdate(tmpd, changed, FALSE);
                if (changed) {
                    XkbSetCauseXkbReq(&cause, X_kbLatchLockState, client);
                    XkbUpdateIndicators(tmpd, changed, TRUE, NULL, &cause);
                }
            }
        }
    }

    return Success;
}

/***====================================================================***/

int
ProcXkbGetControls(ClientPtr client)
{
    xkbGetControlsReply rep;
    XkbControlsPtr xkb;
    DeviceIntPtr dev;

    REQUEST(xkbGetControlsReq);
    REQUEST_SIZE_MATCH(xkbGetControlsReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixGetAttrAccess);

    xkb = dev->key->xkbInfo->desc->ctrls;
    rep = (xkbGetControlsReply) {
        .type = X_Reply,
        .deviceID = ((DeviceIntPtr) dev)->id,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(SIZEOF(xkbGetControlsReply) -
                                 SIZEOF(xGenericReply)),
        .mkDfltBtn = xkb->mk_dflt_btn,
        .numGroups = xkb->num_groups,
        .groupsWrap = xkb->groups_wrap,
        .internalMods = xkb->internal.mask,
        .ignoreLockMods = xkb->ignore_lock.mask,
        .internalRealMods = xkb->internal.real_mods,
        .ignoreLockRealMods = xkb->ignore_lock.real_mods,
        .internalVMods = xkb->internal.vmods,
        .ignoreLockVMods = xkb->ignore_lock.vmods,
        .repeatDelay = xkb->repeat_delay,
        .repeatInterval = xkb->repeat_interval,
        .slowKeysDelay = xkb->slow_keys_delay,
        .debounceDelay = xkb->debounce_delay,
        .mkDelay = xkb->mk_delay,
        .mkInterval = xkb->mk_interval,
        .mkTimeToMax = xkb->mk_time_to_max,
        .mkMaxSpeed = xkb->mk_max_speed,
        .mkCurve = xkb->mk_curve,
        .axOptions = xkb->ax_options,
        .axTimeout = xkb->ax_timeout,
        .axtOptsMask = xkb->axt_opts_mask,
        .axtOptsValues = xkb->axt_opts_values,
        .axtCtrlsMask = xkb->axt_ctrls_mask,
        .axtCtrlsValues = xkb->axt_ctrls_values,
        .enabledCtrls = xkb->enabled_ctrls,
    };
    memcpy(rep.perKeyRepeat, xkb->per_key_repeat, XkbPerKeyBitArraySize);
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.internalVMods);
        swaps(&rep.ignoreLockVMods);
        swapl(&rep.enabledCtrls);
        swaps(&rep.repeatDelay);
        swaps(&rep.repeatInterval);
        swaps(&rep.slowKeysDelay);
        swaps(&rep.debounceDelay);
        swaps(&rep.mkDelay);
        swaps(&rep.mkInterval);
        swaps(&rep.mkTimeToMax);
        swaps(&rep.mkMaxSpeed);
        swaps(&rep.mkCurve);
        swaps(&rep.axTimeout);
        swapl(&rep.axtCtrlsMask);
        swapl(&rep.axtCtrlsValues);
        swaps(&rep.axtOptsMask);
        swaps(&rep.axtOptsValues);
        swaps(&rep.axOptions);
    }
    WriteToClient(client, SIZEOF(xkbGetControlsReply), &rep);
    return Success;
}

int
ProcXkbSetControls(ClientPtr client)
{
    DeviceIntPtr dev, tmpd;
    XkbSrvInfoPtr xkbi;
    XkbControlsPtr ctrl;
    XkbControlsRec new, old;
    xkbControlsNotify cn;
    XkbEventCauseRec cause;
    XkbSrvLedInfoPtr sli;

    REQUEST(xkbSetControlsReq);
    REQUEST_SIZE_MATCH(xkbSetControlsReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixManageAccess);
    CHK_MASK_LEGAL(0x01, stuff->changeCtrls, XkbAllControlsMask);

    for (tmpd = inputInfo.devices; tmpd; tmpd = tmpd->next) {
        if (!tmpd->key || !tmpd->key->xkbInfo)
            continue;
        if ((tmpd == dev) ||
            (!IsMaster(tmpd) && GetMaster(tmpd, MASTER_KEYBOARD) == dev)) {
            xkbi = tmpd->key->xkbInfo;
            ctrl = xkbi->desc->ctrls;
            new = *ctrl;
            XkbSetCauseXkbReq(&cause, X_kbSetControls, client);

            if (stuff->changeCtrls & XkbInternalModsMask) {
                CHK_MASK_MATCH(0x02, stuff->affectInternalMods,
                               stuff->internalMods);
                CHK_MASK_MATCH(0x03, stuff->affectInternalVMods,
                               stuff->internalVMods);

                new.internal.real_mods &= ~(stuff->affectInternalMods);
                new.internal.real_mods |= (stuff->affectInternalMods &
                                           stuff->internalMods);
                new.internal.vmods &= ~(stuff->affectInternalVMods);
                new.internal.vmods |= (stuff->affectInternalVMods &
                                       stuff->internalVMods);
                new.internal.mask = new.internal.real_mods |
                    XkbMaskForVMask(xkbi->desc, new.internal.vmods);
            }

            if (stuff->changeCtrls & XkbIgnoreLockModsMask) {
                CHK_MASK_MATCH(0x4, stuff->affectIgnoreLockMods,
                               stuff->ignoreLockMods);
                CHK_MASK_MATCH(0x5, stuff->affectIgnoreLockVMods,
                               stuff->ignoreLockVMods);

                new.ignore_lock.real_mods &= ~(stuff->affectIgnoreLockMods);
                new.ignore_lock.real_mods |= (stuff->affectIgnoreLockMods &
                                              stuff->ignoreLockMods);
                new.ignore_lock.vmods &= ~(stuff->affectIgnoreLockVMods);
                new.ignore_lock.vmods |= (stuff->affectIgnoreLockVMods &
                                          stuff->ignoreLockVMods);
                new.ignore_lock.mask = new.ignore_lock.real_mods |
                    XkbMaskForVMask(xkbi->desc, new.ignore_lock.vmods);
            }

            CHK_MASK_MATCH(0x06, stuff->affectEnabledCtrls,
                           stuff->enabledCtrls);
            if (stuff->affectEnabledCtrls) {
                CHK_MASK_LEGAL(0x07, stuff->affectEnabledCtrls,
                               XkbAllBooleanCtrlsMask);

                new.enabled_ctrls &= ~(stuff->affectEnabledCtrls);
                new.enabled_ctrls |= (stuff->affectEnabledCtrls &
                                      stuff->enabledCtrls);
            }

            if (stuff->changeCtrls & XkbRepeatKeysMask) {
                if (stuff->repeatDelay < 1 || stuff->repeatInterval < 1) {
                    client->errorValue = _XkbErrCode3(0x08, stuff->repeatDelay,
                                                      stuff->repeatInterval);
                    return BadValue;
                }

                new.repeat_delay = stuff->repeatDelay;
                new.repeat_interval = stuff->repeatInterval;
            }

            if (stuff->changeCtrls & XkbSlowKeysMask) {
                if (stuff->slowKeysDelay < 1) {
                    client->errorValue = _XkbErrCode2(0x09,
                                                      stuff->slowKeysDelay);
                    return BadValue;
                }

                new.slow_keys_delay = stuff->slowKeysDelay;
            }

            if (stuff->changeCtrls & XkbBounceKeysMask) {
                if (stuff->debounceDelay < 1) {
                    client->errorValue = _XkbErrCode2(0x0A,
                                                      stuff->debounceDelay);
                    return BadValue;
                }

                new.debounce_delay = stuff->debounceDelay;
            }

            if (stuff->changeCtrls & XkbMouseKeysMask) {
                if (stuff->mkDfltBtn > XkbMaxMouseKeysBtn) {
                    client->errorValue = _XkbErrCode2(0x0B, stuff->mkDfltBtn);
                    return BadValue;
                }

                new.mk_dflt_btn = stuff->mkDfltBtn;
            }

            if (stuff->changeCtrls & XkbMouseKeysAccelMask) {
                if (stuff->mkDelay < 1 || stuff->mkInterval < 1 ||
                    stuff->mkTimeToMax < 1 || stuff->mkMaxSpeed < 1 ||
                    stuff->mkCurve < -1000) {
                    client->errorValue = _XkbErrCode2(0x0C, 0);
                    return BadValue;
                }

                new.mk_delay = stuff->mkDelay;
                new.mk_interval = stuff->mkInterval;
                new.mk_time_to_max = stuff->mkTimeToMax;
                new.mk_max_speed = stuff->mkMaxSpeed;
                new.mk_curve = stuff->mkCurve;
                AccessXComputeCurveFactor(xkbi, &new);
            }

            if (stuff->changeCtrls & XkbGroupsWrapMask) {
                unsigned act, num;

                act = XkbOutOfRangeGroupAction(stuff->groupsWrap);
                switch (act) {
                case XkbRedirectIntoRange:
                    num = XkbOutOfRangeGroupNumber(stuff->groupsWrap);
                    if (num >= new.num_groups) {
                        client->errorValue = _XkbErrCode3(0x0D, new.num_groups,
                                                          num);
                        return BadValue;
                    }
                case XkbWrapIntoRange:
                case XkbClampIntoRange:
                    break;
                default:
                    client->errorValue = _XkbErrCode2(0x0E, act);
                    return BadValue;
                }

                new.groups_wrap = stuff->groupsWrap;
            }

            CHK_MASK_LEGAL(0x0F, stuff->axOptions, XkbAX_AllOptionsMask);
            if (stuff->changeCtrls & XkbAccessXKeysMask) {
                new.ax_options = stuff->axOptions & XkbAX_AllOptionsMask;
            }
            else {
                if (stuff->changeCtrls & XkbStickyKeysMask) {
                    new.ax_options &= ~(XkbAX_SKOptionsMask);
                    new.ax_options |= (stuff->axOptions & XkbAX_SKOptionsMask);
                }

                if (stuff->changeCtrls & XkbAccessXFeedbackMask) {
                    new.ax_options &= ~(XkbAX_FBOptionsMask);
                    new.ax_options |= (stuff->axOptions & XkbAX_FBOptionsMask);
                }
            }

            if (stuff->changeCtrls & XkbAccessXTimeoutMask) {
                if (stuff->axTimeout < 1) {
                    client->errorValue = _XkbErrCode2(0x10, stuff->axTimeout);
                    return BadValue;
                }
                CHK_MASK_MATCH(0x11, stuff->axtCtrlsMask,
                               stuff->axtCtrlsValues);
                CHK_MASK_LEGAL(0x12, stuff->axtCtrlsMask,
                               XkbAllBooleanCtrlsMask);
                CHK_MASK_MATCH(0x13, stuff->axtOptsMask, stuff->axtOptsValues);
                CHK_MASK_LEGAL(0x14, stuff->axtOptsMask, XkbAX_AllOptionsMask);
                new.ax_timeout = stuff->axTimeout;
                new.axt_ctrls_mask = stuff->axtCtrlsMask;
                new.axt_ctrls_values = (stuff->axtCtrlsValues &
                                        stuff->axtCtrlsMask);
                new.axt_opts_mask = stuff->axtOptsMask;
                new.axt_opts_values = (stuff->axtOptsValues &
                                       stuff->axtOptsMask);
            }

            if (stuff->changeCtrls & XkbPerKeyRepeatMask) {
                memcpy(new.per_key_repeat, stuff->perKeyRepeat,
                       XkbPerKeyBitArraySize);
                if (xkbi->repeatKey &&
                    !BitIsOn(new.per_key_repeat, xkbi->repeatKey)) {
                    AccessXCancelRepeatKey(xkbi, xkbi->repeatKey);
                }
            }

            old = *ctrl;
            *ctrl = new;
            XkbDDXChangeControls(tmpd, &old, ctrl);

            if (XkbComputeControlsNotify(tmpd, &old, ctrl, &cn, FALSE)) {
                cn.keycode = 0;
                cn.eventType = 0;
                cn.requestMajor = XkbReqCode;
                cn.requestMinor = X_kbSetControls;
                XkbSendControlsNotify(tmpd, &cn);
            }

            sli = XkbFindSrvLedInfo(tmpd, XkbDfltXIClass, XkbDfltXIId, 0);
            if (sli)
                XkbUpdateIndicators(tmpd, sli->usesControls, TRUE, NULL,
                                    &cause);

            /* If sticky keys were disabled, clear all locks and latches */
            if ((old.enabled_ctrls & XkbStickyKeysMask) &&
                !(ctrl->enabled_ctrls & XkbStickyKeysMask))
                XkbClearAllLatchesAndLocks(tmpd, xkbi, TRUE, &cause);
        }
    }

    return Success;
}

/***====================================================================***/

static int
XkbSizeKeyTypes(XkbDescPtr xkb, xkbGetMapReply * rep)
{
    XkbKeyTypeRec *type;
    unsigned i, len;

    len = 0;
    if (((rep->present & XkbKeyTypesMask) == 0) || (rep->nTypes < 1) ||
        (!xkb) || (!xkb->map) || (!xkb->map->types)) {
        rep->present &= ~XkbKeyTypesMask;
        rep->firstType = rep->nTypes = 0;
        return 0;
    }
    type = &xkb->map->types[rep->firstType];
    for (i = 0; i < rep->nTypes; i++, type++) {
        len += SIZEOF(xkbKeyTypeWireDesc);
        if (type->map_count > 0) {
            len += (type->map_count * SIZEOF(xkbKTMapEntryWireDesc));
            if (type->preserve)
                len += (type->map_count * SIZEOF(xkbModsWireDesc));
        }
    }
    return len;
}

static char *
XkbWriteKeyTypes(XkbDescPtr xkb,
                 xkbGetMapReply * rep, char *buf, ClientPtr client)
{
    XkbKeyTypePtr type;
    unsigned i;
    xkbKeyTypeWireDesc *wire;

    type = &xkb->map->types[rep->firstType];
    for (i = 0; i < rep->nTypes; i++, type++) {
        register unsigned n;

        wire = (xkbKeyTypeWireDesc *) buf;
        wire->mask = type->mods.mask;
        wire->realMods = type->mods.real_mods;
        wire->virtualMods = type->mods.vmods;
        wire->numLevels = type->num_levels;
        wire->nMapEntries = type->map_count;
        wire->preserve = (type->preserve != NULL);
        if (client->swapped) {
            swaps(&wire->virtualMods);
        }

        buf = (char *) &wire[1];
        if (wire->nMapEntries > 0) {
            xkbKTMapEntryWireDesc *ewire;
            XkbKTMapEntryPtr entry;

            ewire = (xkbKTMapEntryWireDesc *) buf;
            entry = type->map;
            for (n = 0; n < type->map_count; n++, ewire++, entry++) {
                ewire->active = entry->active;
                ewire->mask = entry->mods.mask;
                ewire->level = entry->level;
                ewire->realMods = entry->mods.real_mods;
                ewire->virtualMods = entry->mods.vmods;
                if (client->swapped) {
                    swaps(&ewire->virtualMods);
                }
            }
            buf = (char *) ewire;
            if (type->preserve != NULL) {
                xkbModsWireDesc *pwire;
                XkbModsPtr preserve;

                pwire = (xkbModsWireDesc *) buf;
                preserve = type->preserve;
                for (n = 0; n < type->map_count; n++, pwire++, preserve++) {
                    pwire->mask = preserve->mask;
                    pwire->realMods = preserve->real_mods;
                    pwire->virtualMods = preserve->vmods;
                    if (client->swapped) {
                        swaps(&pwire->virtualMods);
                    }
                }
                buf = (char *) pwire;
            }
        }
    }
    return buf;
}

static int
XkbSizeKeySyms(XkbDescPtr xkb, xkbGetMapReply * rep)
{
    XkbSymMapPtr symMap;
    unsigned i, len;
    unsigned nSyms, nSymsThisKey;

    if (((rep->present & XkbKeySymsMask) == 0) || (rep->nKeySyms < 1) ||
        (!xkb) || (!xkb->map) || (!xkb->map->key_sym_map)) {
        rep->present &= ~XkbKeySymsMask;
        rep->firstKeySym = rep->nKeySyms = 0;
        rep->totalSyms = 0;
        return 0;
    }
    len = rep->nKeySyms * SIZEOF(xkbSymMapWireDesc);
    symMap = &xkb->map->key_sym_map[rep->firstKeySym];
    for (i = nSyms = 0; i < rep->nKeySyms; i++, symMap++) {
        if (symMap->offset != 0) {
            nSymsThisKey = XkbNumGroups(symMap->group_info) * symMap->width;
            nSyms += nSymsThisKey;
        }
    }
    len += nSyms * 4;
    rep->totalSyms = nSyms;
    return len;
}

static int
XkbSizeVirtualMods(XkbDescPtr xkb, xkbGetMapReply * rep)
{
    register unsigned i, nMods, bit;

    if (((rep->present & XkbVirtualModsMask) == 0) || (rep->virtualMods == 0) ||
        (!xkb) || (!xkb->server)) {
        rep->present &= ~XkbVirtualModsMask;
        rep->virtualMods = 0;
        return 0;
    }
    for (i = nMods = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (rep->virtualMods & bit)
            nMods++;
    }
    return XkbPaddedSize(nMods);
}

static char *
XkbWriteKeySyms(XkbDescPtr xkb, xkbGetMapReply * rep, char *buf,
                ClientPtr client)
{
    register KeySym *pSym;
    XkbSymMapPtr symMap;
    xkbSymMapWireDesc *outMap;
    register unsigned i;

    symMap = &xkb->map->key_sym_map[rep->firstKeySym];
    for (i = 0; i < rep->nKeySyms; i++, symMap++) {
        outMap = (xkbSymMapWireDesc *) buf;
        outMap->ktIndex[0] = symMap->kt_index[0];
        outMap->ktIndex[1] = symMap->kt_index[1];
        outMap->ktIndex[2] = symMap->kt_index[2];
        outMap->ktIndex[3] = symMap->kt_index[3];
        outMap->groupInfo = symMap->group_info;
        outMap->width = symMap->width;
        outMap->nSyms = symMap->width * XkbNumGroups(symMap->group_info);
        buf = (char *) &outMap[1];
        if (outMap->nSyms == 0)
            continue;

        pSym = &xkb->map->syms[symMap->offset];
        memcpy((char *) buf, (char *) pSym, outMap->nSyms * 4);
        if (client->swapped) {
            register int nSyms = outMap->nSyms;

            swaps(&outMap->nSyms);
            while (nSyms-- > 0) {
                swapl((int *) buf);
                buf += 4;
            }
        }
        else
            buf += outMap->nSyms * 4;
    }
    return buf;
}

static int
XkbSizeKeyActions(XkbDescPtr xkb, xkbGetMapReply * rep)
{
    unsigned i, len, nActs;
    register KeyCode firstKey;

    if (((rep->present & XkbKeyActionsMask) == 0) || (rep->nKeyActs < 1) ||
        (!xkb) || (!xkb->server) || (!xkb->server->key_acts)) {
        rep->present &= ~XkbKeyActionsMask;
        rep->firstKeyAct = rep->nKeyActs = 0;
        rep->totalActs = 0;
        return 0;
    }
    firstKey = rep->firstKeyAct;
    for (nActs = i = 0; i < rep->nKeyActs; i++) {
        if (xkb->server->key_acts[i + firstKey] != 0)
            nActs += XkbKeyNumActions(xkb, i + firstKey);
    }
    len = XkbPaddedSize(rep->nKeyActs) + (nActs * SIZEOF(xkbActionWireDesc));
    rep->totalActs = nActs;
    return len;
}

static char *
XkbWriteKeyActions(XkbDescPtr xkb, xkbGetMapReply * rep, char *buf,
                   ClientPtr client)
{
    unsigned i;
    CARD8 *numDesc;
    XkbAnyAction *actDesc;

    numDesc = (CARD8 *) buf;
    for (i = 0; i < rep->nKeyActs; i++) {
        if (xkb->server->key_acts[i + rep->firstKeyAct] == 0)
            numDesc[i] = 0;
        else
            numDesc[i] = XkbKeyNumActions(xkb, (i + rep->firstKeyAct));
    }
    buf += XkbPaddedSize(rep->nKeyActs);

    actDesc = (XkbAnyAction *) buf;
    for (i = 0; i < rep->nKeyActs; i++) {
        if (xkb->server->key_acts[i + rep->firstKeyAct] != 0) {
            unsigned int num;

            num = XkbKeyNumActions(xkb, (i + rep->firstKeyAct));
            memcpy((char *) actDesc,
                   (char *) XkbKeyActionsPtr(xkb, (i + rep->firstKeyAct)),
                   num * SIZEOF(xkbActionWireDesc));
            actDesc += num;
        }
    }
    buf = (char *) actDesc;
    return buf;
}

static int
XkbSizeKeyBehaviors(XkbDescPtr xkb, xkbGetMapReply * rep)
{
    unsigned i, len, nBhvr;
    XkbBehavior *bhv;

    if (((rep->present & XkbKeyBehaviorsMask) == 0) || (rep->nKeyBehaviors < 1)
        || (!xkb) || (!xkb->server) || (!xkb->server->behaviors)) {
        rep->present &= ~XkbKeyBehaviorsMask;
        rep->firstKeyBehavior = rep->nKeyBehaviors = 0;
        rep->totalKeyBehaviors = 0;
        return 0;
    }
    bhv = &xkb->server->behaviors[rep->firstKeyBehavior];
    for (nBhvr = i = 0; i < rep->nKeyBehaviors; i++, bhv++) {
        if (bhv->type != XkbKB_Default)
            nBhvr++;
    }
    len = nBhvr * SIZEOF(xkbBehaviorWireDesc);
    rep->totalKeyBehaviors = nBhvr;
    return len;
}

static char *
XkbWriteKeyBehaviors(XkbDescPtr xkb, xkbGetMapReply * rep, char *buf,
                     ClientPtr client)
{
    unsigned i;
    xkbBehaviorWireDesc *wire;
    XkbBehavior *pBhvr;

    wire = (xkbBehaviorWireDesc *) buf;
    pBhvr = &xkb->server->behaviors[rep->firstKeyBehavior];
    for (i = 0; i < rep->nKeyBehaviors; i++, pBhvr++) {
        if (pBhvr->type != XkbKB_Default) {
            wire->key = i + rep->firstKeyBehavior;
            wire->type = pBhvr->type;
            wire->data = pBhvr->data;
            wire++;
        }
    }
    buf = (char *) wire;
    return buf;
}

static int
XkbSizeExplicit(XkbDescPtr xkb, xkbGetMapReply * rep)
{
    unsigned i, len, nRtrn;

    if (((rep->present & XkbExplicitComponentsMask) == 0) ||
        (rep->nKeyExplicit < 1) || (!xkb) || (!xkb->server) ||
        (!xkb->server->explicit)) {
        rep->present &= ~XkbExplicitComponentsMask;
        rep->firstKeyExplicit = rep->nKeyExplicit = 0;
        rep->totalKeyExplicit = 0;
        return 0;
    }
    for (nRtrn = i = 0; i < rep->nKeyExplicit; i++) {
        if (xkb->server->explicit[i + rep->firstKeyExplicit] != 0)
            nRtrn++;
    }
    rep->totalKeyExplicit = nRtrn;
    len = XkbPaddedSize(nRtrn * 2);     /* two bytes per non-zero explicit component */
    return len;
}

static char *
XkbWriteExplicit(XkbDescPtr xkb, xkbGetMapReply * rep, char *buf,
                 ClientPtr client)
{
    unsigned i;
    char *start;
    unsigned char *pExp;

    start = buf;
    pExp = &xkb->server->explicit[rep->firstKeyExplicit];
    for (i = 0; i < rep->nKeyExplicit; i++, pExp++) {
        if (*pExp != 0) {
            *buf++ = i + rep->firstKeyExplicit;
            *buf++ = *pExp;
        }
    }
    i = XkbPaddedSize(buf - start) - (buf - start);     /* pad to word boundary */
    return buf + i;
}

static int
XkbSizeModifierMap(XkbDescPtr xkb, xkbGetMapReply * rep)
{
    unsigned i, len, nRtrn;

    if (((rep->present & XkbModifierMapMask) == 0) || (rep->nModMapKeys < 1) ||
        (!xkb) || (!xkb->map) || (!xkb->map->modmap)) {
        rep->present &= ~XkbModifierMapMask;
        rep->firstModMapKey = rep->nModMapKeys = 0;
        rep->totalModMapKeys = 0;
        return 0;
    }
    for (nRtrn = i = 0; i < rep->nModMapKeys; i++) {
        if (xkb->map->modmap[i + rep->firstModMapKey] != 0)
            nRtrn++;
    }
    rep->totalModMapKeys = nRtrn;
    len = XkbPaddedSize(nRtrn * 2);     /* two bytes per non-zero modmap component */
    return len;
}

static char *
XkbWriteModifierMap(XkbDescPtr xkb, xkbGetMapReply * rep, char *buf,
                    ClientPtr client)
{
    unsigned i;
    char *start;
    unsigned char *pMap;

    start = buf;
    pMap = &xkb->map->modmap[rep->firstModMapKey];
    for (i = 0; i < rep->nModMapKeys; i++, pMap++) {
        if (*pMap != 0) {
            *buf++ = i + rep->firstModMapKey;
            *buf++ = *pMap;
        }
    }
    i = XkbPaddedSize(buf - start) - (buf - start);     /* pad to word boundary */
    return buf + i;
}

static int
XkbSizeVirtualModMap(XkbDescPtr xkb, xkbGetMapReply * rep)
{
    unsigned i, len, nRtrn;

    if (((rep->present & XkbVirtualModMapMask) == 0) || (rep->nVModMapKeys < 1)
        || (!xkb) || (!xkb->server) || (!xkb->server->vmodmap)) {
        rep->present &= ~XkbVirtualModMapMask;
        rep->firstVModMapKey = rep->nVModMapKeys = 0;
        rep->totalVModMapKeys = 0;
        return 0;
    }
    for (nRtrn = i = 0; i < rep->nVModMapKeys; i++) {
        if (xkb->server->vmodmap[i + rep->firstVModMapKey] != 0)
            nRtrn++;
    }
    rep->totalVModMapKeys = nRtrn;
    len = nRtrn * SIZEOF(xkbVModMapWireDesc);
    return len;
}

static char *
XkbWriteVirtualModMap(XkbDescPtr xkb, xkbGetMapReply * rep, char *buf,
                      ClientPtr client)
{
    unsigned i;
    xkbVModMapWireDesc *wire;
    unsigned short *pMap;

    wire = (xkbVModMapWireDesc *) buf;
    pMap = &xkb->server->vmodmap[rep->firstVModMapKey];
    for (i = 0; i < rep->nVModMapKeys; i++, pMap++) {
        if (*pMap != 0) {
            wire->key = i + rep->firstVModMapKey;
            wire->vmods = *pMap;
            wire++;
        }
    }
    return (char *) wire;
}

static Status
XkbComputeGetMapReplySize(XkbDescPtr xkb, xkbGetMapReply * rep)
{
    int len;

    rep->minKeyCode = xkb->min_key_code;
    rep->maxKeyCode = xkb->max_key_code;
    len = XkbSizeKeyTypes(xkb, rep);
    len += XkbSizeKeySyms(xkb, rep);
    len += XkbSizeKeyActions(xkb, rep);
    len += XkbSizeKeyBehaviors(xkb, rep);
    len += XkbSizeVirtualMods(xkb, rep);
    len += XkbSizeExplicit(xkb, rep);
    len += XkbSizeModifierMap(xkb, rep);
    len += XkbSizeVirtualModMap(xkb, rep);
    rep->length += (len / 4);
    return Success;
}

static int
XkbSendMap(ClientPtr client, XkbDescPtr xkb, xkbGetMapReply * rep)
{
    unsigned i, len;
    char *desc, *start;

    len = (rep->length * 4) - (SIZEOF(xkbGetMapReply) - SIZEOF(xGenericReply));
    start = desc = calloc(1, len);
    if (!start)
        return BadAlloc;
    if (rep->nTypes > 0)
        desc = XkbWriteKeyTypes(xkb, rep, desc, client);
    if (rep->nKeySyms > 0)
        desc = XkbWriteKeySyms(xkb, rep, desc, client);
    if (rep->nKeyActs > 0)
        desc = XkbWriteKeyActions(xkb, rep, desc, client);
    if (rep->totalKeyBehaviors > 0)
        desc = XkbWriteKeyBehaviors(xkb, rep, desc, client);
    if (rep->virtualMods) {
        register int sz, bit;

        for (i = sz = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
            if (rep->virtualMods & bit) {
                desc[sz++] = xkb->server->vmods[i];
            }
        }
        desc += XkbPaddedSize(sz);
    }
    if (rep->totalKeyExplicit > 0)
        desc = XkbWriteExplicit(xkb, rep, desc, client);
    if (rep->totalModMapKeys > 0)
        desc = XkbWriteModifierMap(xkb, rep, desc, client);
    if (rep->totalVModMapKeys > 0)
        desc = XkbWriteVirtualModMap(xkb, rep, desc, client);
    if ((desc - start) != (len)) {
        ErrorF
            ("[xkb] BOGUS LENGTH in write keyboard desc, expected %d, got %ld\n",
             len, (unsigned long) (desc - start));
    }
    if (client->swapped) {
        swaps(&rep->sequenceNumber);
        swapl(&rep->length);
        swaps(&rep->present);
        swaps(&rep->totalSyms);
        swaps(&rep->totalActs);
    }
    WriteToClient(client, (i = SIZEOF(xkbGetMapReply)), rep);
    WriteToClient(client, len, start);
    free((char *) start);
    return Success;
}

int
ProcXkbGetMap(ClientPtr client)
{
    DeviceIntPtr dev;
    xkbGetMapReply rep;
    XkbDescRec *xkb;
    int n, status;

    REQUEST(xkbGetMapReq);
    REQUEST_SIZE_MATCH(xkbGetMapReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixGetAttrAccess);
    CHK_MASK_OVERLAP(0x01, stuff->full, stuff->partial);
    CHK_MASK_LEGAL(0x02, stuff->full, XkbAllMapComponentsMask);
    CHK_MASK_LEGAL(0x03, stuff->partial, XkbAllMapComponentsMask);

    xkb = dev->key->xkbInfo->desc;
    rep = (xkbGetMapReply) {
        .type = X_Reply,
        .deviceID = dev->id,
        .sequenceNumber = client->sequence,
        .length = (SIZEOF(xkbGetMapReply) - SIZEOF(xGenericReply)) >> 2,
        .present = stuff->partial | stuff->full,
        .minKeyCode = xkb->min_key_code,
        .maxKeyCode = xkb->max_key_code
    };

    if (stuff->full & XkbKeyTypesMask) {
        rep.firstType = 0;
        rep.nTypes = xkb->map->num_types;
    }
    else if (stuff->partial & XkbKeyTypesMask) {
        if (((unsigned) stuff->firstType + stuff->nTypes) > xkb->map->num_types) {
            client->errorValue = _XkbErrCode4(0x04, xkb->map->num_types,
                                              stuff->firstType, stuff->nTypes);
            return BadValue;
        }
        rep.firstType = stuff->firstType;
        rep.nTypes = stuff->nTypes;
    }
    else
        rep.nTypes = 0;
    rep.totalTypes = xkb->map->num_types;

    n = XkbNumKeys(xkb);
    if (stuff->full & XkbKeySymsMask) {
        rep.firstKeySym = xkb->min_key_code;
        rep.nKeySyms = n;
    }
    else if (stuff->partial & XkbKeySymsMask) {
        CHK_KEY_RANGE(0x05, stuff->firstKeySym, stuff->nKeySyms, xkb);
        rep.firstKeySym = stuff->firstKeySym;
        rep.nKeySyms = stuff->nKeySyms;
    }
    else
        rep.nKeySyms = 0;
    rep.totalSyms = 0;

    if (stuff->full & XkbKeyActionsMask) {
        rep.firstKeyAct = xkb->min_key_code;
        rep.nKeyActs = n;
    }
    else if (stuff->partial & XkbKeyActionsMask) {
        CHK_KEY_RANGE(0x07, stuff->firstKeyAct, stuff->nKeyActs, xkb);
        rep.firstKeyAct = stuff->firstKeyAct;
        rep.nKeyActs = stuff->nKeyActs;
    }
    else
        rep.nKeyActs = 0;
    rep.totalActs = 0;

    if (stuff->full & XkbKeyBehaviorsMask) {
        rep.firstKeyBehavior = xkb->min_key_code;
        rep.nKeyBehaviors = n;
    }
    else if (stuff->partial & XkbKeyBehaviorsMask) {
        CHK_KEY_RANGE(0x09, stuff->firstKeyBehavior, stuff->nKeyBehaviors, xkb);
        rep.firstKeyBehavior = stuff->firstKeyBehavior;
        rep.nKeyBehaviors = stuff->nKeyBehaviors;
    }
    else
        rep.nKeyBehaviors = 0;
    rep.totalKeyBehaviors = 0;

    if (stuff->full & XkbVirtualModsMask)
        rep.virtualMods = ~0;
    else if (stuff->partial & XkbVirtualModsMask)
        rep.virtualMods = stuff->virtualMods;

    if (stuff->full & XkbExplicitComponentsMask) {
        rep.firstKeyExplicit = xkb->min_key_code;
        rep.nKeyExplicit = n;
    }
    else if (stuff->partial & XkbExplicitComponentsMask) {
        CHK_KEY_RANGE(0x0B, stuff->firstKeyExplicit, stuff->nKeyExplicit, xkb);
        rep.firstKeyExplicit = stuff->firstKeyExplicit;
        rep.nKeyExplicit = stuff->nKeyExplicit;
    }
    else
        rep.nKeyExplicit = 0;
    rep.totalKeyExplicit = 0;

    if (stuff->full & XkbModifierMapMask) {
        rep.firstModMapKey = xkb->min_key_code;
        rep.nModMapKeys = n;
    }
    else if (stuff->partial & XkbModifierMapMask) {
        CHK_KEY_RANGE(0x0D, stuff->firstModMapKey, stuff->nModMapKeys, xkb);
        rep.firstModMapKey = stuff->firstModMapKey;
        rep.nModMapKeys = stuff->nModMapKeys;
    }
    else
        rep.nModMapKeys = 0;
    rep.totalModMapKeys = 0;

    if (stuff->full & XkbVirtualModMapMask) {
        rep.firstVModMapKey = xkb->min_key_code;
        rep.nVModMapKeys = n;
    }
    else if (stuff->partial & XkbVirtualModMapMask) {
        CHK_KEY_RANGE(0x0F, stuff->firstVModMapKey, stuff->nVModMapKeys, xkb);
        rep.firstVModMapKey = stuff->firstVModMapKey;
        rep.nVModMapKeys = stuff->nVModMapKeys;
    }
    else
        rep.nVModMapKeys = 0;
    rep.totalVModMapKeys = 0;

    if ((status = XkbComputeGetMapReplySize(xkb, &rep)) != Success)
        return status;
    return XkbSendMap(client, xkb, &rep);
}

/***====================================================================***/

static int
CheckKeyTypes(ClientPtr client,
              XkbDescPtr xkb,
              xkbSetMapReq * req,
              xkbKeyTypeWireDesc ** wireRtrn,
              int *nMapsRtrn, CARD8 *mapWidthRtrn, Bool doswap)
{
    unsigned nMaps;
    register unsigned i, n;
    register CARD8 *map;
    register xkbKeyTypeWireDesc *wire = *wireRtrn;

    if (req->firstType > ((unsigned) xkb->map->num_types)) {
        *nMapsRtrn = _XkbErrCode3(0x01, req->firstType, xkb->map->num_types);
        return 0;
    }
    if (req->flags & XkbSetMapResizeTypes) {
        nMaps = req->firstType + req->nTypes;
        if (nMaps < XkbNumRequiredTypes) {      /* canonical types must be there */
            *nMapsRtrn = _XkbErrCode4(0x02, req->firstType, req->nTypes, 4);
            return 0;
        }
    }
    else if (req->present & XkbKeyTypesMask) {
        nMaps = xkb->map->num_types;
        if ((req->firstType + req->nTypes) > nMaps) {
            *nMapsRtrn = req->firstType + req->nTypes;
            return 0;
        }
    }
    else {
        *nMapsRtrn = xkb->map->num_types;
        for (i = 0; i < xkb->map->num_types; i++) {
            mapWidthRtrn[i] = xkb->map->types[i].num_levels;
        }
        return 1;
    }

    for (i = 0; i < req->firstType; i++) {
        mapWidthRtrn[i] = xkb->map->types[i].num_levels;
    }
    for (i = 0; i < req->nTypes; i++) {
        unsigned width;

        if (client->swapped && doswap) {
            swaps(&wire->virtualMods);
        }
        n = i + req->firstType;
        width = wire->numLevels;
        if (width < 1) {
            *nMapsRtrn = _XkbErrCode3(0x04, n, width);
            return 0;
        }
        else if ((n == XkbOneLevelIndex) && (width != 1)) {     /* must be width 1 */
            *nMapsRtrn = _XkbErrCode3(0x05, n, width);
            return 0;
        }
        else if ((width != 2) &&
                 ((n == XkbTwoLevelIndex) || (n == XkbKeypadIndex) ||
                  (n == XkbAlphabeticIndex))) {
            /* TWO_LEVEL, ALPHABETIC and KEYPAD must be width 2 */
            *nMapsRtrn = _XkbErrCode3(0x05, n, width);
            return 0;
        }
        if (wire->nMapEntries > 0) {
            xkbKTSetMapEntryWireDesc *mapWire;
            xkbModsWireDesc *preWire;

            mapWire = (xkbKTSetMapEntryWireDesc *) &wire[1];
            preWire = (xkbModsWireDesc *) &mapWire[wire->nMapEntries];
            for (n = 0; n < wire->nMapEntries; n++) {
                if (client->swapped && doswap) {
                    swaps(&mapWire[n].virtualMods);
                }
                if (mapWire[n].realMods & (~wire->realMods)) {
                    *nMapsRtrn = _XkbErrCode4(0x06, n, mapWire[n].realMods,
                                              wire->realMods);
                    return 0;
                }
                if (mapWire[n].virtualMods & (~wire->virtualMods)) {
                    *nMapsRtrn = _XkbErrCode3(0x07, n, mapWire[n].virtualMods);
                    return 0;
                }
                if (mapWire[n].level >= wire->numLevels) {
                    *nMapsRtrn = _XkbErrCode4(0x08, n, wire->numLevels,
                                              mapWire[n].level);
                    return 0;
                }
                if (wire->preserve) {
                    if (client->swapped && doswap) {
                        swaps(&preWire[n].virtualMods);
                    }
                    if (preWire[n].realMods & (~mapWire[n].realMods)) {
                        *nMapsRtrn = _XkbErrCode4(0x09, n, preWire[n].realMods,
                                                  mapWire[n].realMods);
                        return 0;
                    }
                    if (preWire[n].virtualMods & (~mapWire[n].virtualMods)) {
                        *nMapsRtrn =
                            _XkbErrCode3(0x0a, n, preWire[n].virtualMods);
                        return 0;
                    }
                }
            }
            if (wire->preserve)
                map = (CARD8 *) &preWire[wire->nMapEntries];
            else
                map = (CARD8 *) &mapWire[wire->nMapEntries];
        }
        else
            map = (CARD8 *) &wire[1];
        mapWidthRtrn[i + req->firstType] = wire->numLevels;
        wire = (xkbKeyTypeWireDesc *) map;
    }
    for (i = req->firstType + req->nTypes; i < nMaps; i++) {
        mapWidthRtrn[i] = xkb->map->types[i].num_levels;
    }
    *nMapsRtrn = nMaps;
    *wireRtrn = wire;
    return 1;
}

static int
CheckKeySyms(ClientPtr client,
             XkbDescPtr xkb,
             xkbSetMapReq * req,
             int nTypes,
             CARD8 *mapWidths,
             CARD16 *symsPerKey, xkbSymMapWireDesc ** wireRtrn, int *errorRtrn, Bool doswap)
{
    register unsigned i;
    XkbSymMapPtr map;
    xkbSymMapWireDesc *wire = *wireRtrn;

    if (!(XkbKeySymsMask & req->present))
        return 1;
    CHK_REQ_KEY_RANGE2(0x11, req->firstKeySym, req->nKeySyms, req, (*errorRtrn),
                       0);
    for (i = 0; i < req->nKeySyms; i++) {
        KeySym *pSyms;
        register unsigned nG;

        if (client->swapped && doswap) {
            swaps(&wire->nSyms);
        }
        nG = XkbNumGroups(wire->groupInfo);
        if (nG > XkbNumKbdGroups) {
            *errorRtrn = _XkbErrCode3(0x14, i + req->firstKeySym, nG);
            return 0;
        }
        if (nG > 0) {
            register int g, w;

            for (g = w = 0; g < nG; g++) {
                if (wire->ktIndex[g] >= (unsigned) nTypes) {
                    *errorRtrn = _XkbErrCode4(0x15, i + req->firstKeySym, g,
                                              wire->ktIndex[g]);
                    return 0;
                }
                if (mapWidths[wire->ktIndex[g]] > w)
                    w = mapWidths[wire->ktIndex[g]];
            }
            if (wire->width != w) {
                *errorRtrn =
                    _XkbErrCode3(0x16, i + req->firstKeySym, wire->width);
                return 0;
            }
            w *= nG;
            symsPerKey[i + req->firstKeySym] = w;
            if (w != wire->nSyms) {
                *errorRtrn =
                    _XkbErrCode4(0x16, i + req->firstKeySym, wire->nSyms, w);
                return 0;
            }
        }
        else if (wire->nSyms != 0) {
            *errorRtrn = _XkbErrCode3(0x17, i + req->firstKeySym, wire->nSyms);
            return 0;
        }
        pSyms = (KeySym *) &wire[1];
        wire = (xkbSymMapWireDesc *) &pSyms[wire->nSyms];
    }

    map = &xkb->map->key_sym_map[i];
    for (; i <= (unsigned) xkb->max_key_code; i++, map++) {
        register int g, nG, w;

        nG = XkbKeyNumGroups(xkb, i);
        for (w = g = 0; g < nG; g++) {
            if (map->kt_index[g] >= (unsigned) nTypes) {
                *errorRtrn = _XkbErrCode4(0x18, i, g, map->kt_index[g]);
                return 0;
            }
            if (mapWidths[map->kt_index[g]] > w)
                w = mapWidths[map->kt_index[g]];
        }
        symsPerKey[i] = w * nG;
    }
    *wireRtrn = wire;
    return 1;
}

static int
CheckKeyActions(XkbDescPtr xkb,
                xkbSetMapReq * req,
                int nTypes,
                CARD8 *mapWidths,
                CARD16 *symsPerKey, CARD8 **wireRtrn, int *nActsRtrn)
{
    int nActs;
    CARD8 *wire = *wireRtrn;
    register unsigned i;

    if (!(XkbKeyActionsMask & req->present))
        return 1;
    CHK_REQ_KEY_RANGE2(0x21, req->firstKeyAct, req->nKeyActs, req, (*nActsRtrn),
                       0);
    for (nActs = i = 0; i < req->nKeyActs; i++) {
        if (wire[0] != 0) {
            if (wire[0] == symsPerKey[i + req->firstKeyAct])
                nActs += wire[0];
            else {
                *nActsRtrn = _XkbErrCode3(0x23, i + req->firstKeyAct, wire[0]);
                return 0;
            }
        }
        wire++;
    }
    if (req->nKeyActs % 4)
        wire += 4 - (req->nKeyActs % 4);
    *wireRtrn = (CARD8 *) (((XkbAnyAction *) wire) + nActs);
    *nActsRtrn = nActs;
    return 1;
}

static int
CheckKeyBehaviors(XkbDescPtr xkb,
                  xkbSetMapReq * req,
                  xkbBehaviorWireDesc ** wireRtrn, int *errorRtrn)
{
    register xkbBehaviorWireDesc *wire = *wireRtrn;
    register XkbServerMapPtr server = xkb->server;
    register unsigned i;
    unsigned first, last;

    if (((req->present & XkbKeyBehaviorsMask) == 0) || (req->nKeyBehaviors < 1)) {
        req->present &= ~XkbKeyBehaviorsMask;
        req->nKeyBehaviors = 0;
        return 1;
    }
    first = req->firstKeyBehavior;
    last = req->firstKeyBehavior + req->nKeyBehaviors - 1;
    if (first < req->minKeyCode) {
        *errorRtrn = _XkbErrCode3(0x31, first, req->minKeyCode);
        return 0;
    }
    if (last > req->maxKeyCode) {
        *errorRtrn = _XkbErrCode3(0x32, last, req->maxKeyCode);
        return 0;
    }

    for (i = 0; i < req->totalKeyBehaviors; i++, wire++) {
        if ((wire->key < first) || (wire->key > last)) {
            *errorRtrn = _XkbErrCode4(0x33, first, last, wire->key);
            return 0;
        }
        if ((wire->type & XkbKB_Permanent) &&
            ((server->behaviors[wire->key].type != wire->type) ||
             (server->behaviors[wire->key].data != wire->data))) {
            *errorRtrn = _XkbErrCode3(0x33, wire->key, wire->type);
            return 0;
        }
        if ((wire->type == XkbKB_RadioGroup) &&
            ((wire->data & (~XkbKB_RGAllowNone)) > XkbMaxRadioGroups)) {
            *errorRtrn = _XkbErrCode4(0x34, wire->key, wire->data,
                                      XkbMaxRadioGroups);
            return 0;
        }
        if ((wire->type == XkbKB_Overlay1) || (wire->type == XkbKB_Overlay2)) {
            CHK_KEY_RANGE2(0x35, wire->key, 1, xkb, *errorRtrn, 0);
        }
    }
    *wireRtrn = wire;
    return 1;
}

static int
CheckVirtualMods(XkbDescRec * xkb,
                 xkbSetMapReq * req, CARD8 **wireRtrn, int *errorRtrn)
{
    register CARD8 *wire = *wireRtrn;
    register unsigned i, nMods, bit;

    if (((req->present & XkbVirtualModsMask) == 0) || (req->virtualMods == 0))
        return 1;
    for (i = nMods = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (req->virtualMods & bit)
            nMods++;
    }
    *wireRtrn = (wire + XkbPaddedSize(nMods));
    return 1;
}

static int
CheckKeyExplicit(XkbDescPtr xkb,
                 xkbSetMapReq * req, CARD8 **wireRtrn, int *errorRtrn)
{
    register CARD8 *wire = *wireRtrn;
    CARD8 *start;
    register unsigned i;
    int first, last;

    if (((req->present & XkbExplicitComponentsMask) == 0) ||
        (req->nKeyExplicit < 1)) {
        req->present &= ~XkbExplicitComponentsMask;
        req->nKeyExplicit = 0;
        return 1;
    }
    first = req->firstKeyExplicit;
    last = first + req->nKeyExplicit - 1;
    if (first < req->minKeyCode) {
        *errorRtrn = _XkbErrCode3(0x51, first, req->minKeyCode);
        return 0;
    }
    if (last > req->maxKeyCode) {
        *errorRtrn = _XkbErrCode3(0x52, last, req->maxKeyCode);
        return 0;
    }
    start = wire;
    for (i = 0; i < req->totalKeyExplicit; i++, wire += 2) {
        if ((wire[0] < first) || (wire[0] > last)) {
            *errorRtrn = _XkbErrCode4(0x53, first, last, wire[0]);
            return 0;
        }
        if (wire[1] & (~XkbAllExplicitMask)) {
            *errorRtrn = _XkbErrCode3(0x52, ~XkbAllExplicitMask, wire[1]);
            return 0;
        }
    }
    wire += XkbPaddedSize(wire - start) - (wire - start);
    *wireRtrn = wire;
    return 1;
}

static int
CheckModifierMap(XkbDescPtr xkb, xkbSetMapReq * req, CARD8 **wireRtrn,
                 int *errRtrn)
{
    register CARD8 *wire = *wireRtrn;
    CARD8 *start;
    register unsigned i;
    int first, last;

    if (((req->present & XkbModifierMapMask) == 0) || (req->nModMapKeys < 1)) {
        req->present &= ~XkbModifierMapMask;
        req->nModMapKeys = 0;
        return 1;
    }
    first = req->firstModMapKey;
    last = first + req->nModMapKeys - 1;
    if (first < req->minKeyCode) {
        *errRtrn = _XkbErrCode3(0x61, first, req->minKeyCode);
        return 0;
    }
    if (last > req->maxKeyCode) {
        *errRtrn = _XkbErrCode3(0x62, last, req->maxKeyCode);
        return 0;
    }
    start = wire;
    for (i = 0; i < req->totalModMapKeys; i++, wire += 2) {
        if ((wire[0] < first) || (wire[0] > last)) {
            *errRtrn = _XkbErrCode4(0x63, first, last, wire[0]);
            return 0;
        }
    }
    wire += XkbPaddedSize(wire - start) - (wire - start);
    *wireRtrn = wire;
    return 1;
}

static int
CheckVirtualModMap(XkbDescPtr xkb,
                   xkbSetMapReq * req,
                   xkbVModMapWireDesc ** wireRtrn, int *errRtrn)
{
    register xkbVModMapWireDesc *wire = *wireRtrn;
    register unsigned i;
    int first, last;

    if (((req->present & XkbVirtualModMapMask) == 0) || (req->nVModMapKeys < 1)) {
        req->present &= ~XkbVirtualModMapMask;
        req->nVModMapKeys = 0;
        return 1;
    }
    first = req->firstVModMapKey;
    last = first + req->nVModMapKeys - 1;
    if (first < req->minKeyCode) {
        *errRtrn = _XkbErrCode3(0x71, first, req->minKeyCode);
        return 0;
    }
    if (last > req->maxKeyCode) {
        *errRtrn = _XkbErrCode3(0x72, last, req->maxKeyCode);
        return 0;
    }
    for (i = 0; i < req->totalVModMapKeys; i++, wire++) {
        if ((wire->key < first) || (wire->key > last)) {
            *errRtrn = _XkbErrCode4(0x73, first, last, wire->key);
            return 0;
        }
    }
    *wireRtrn = wire;
    return 1;
}

static char *
SetKeyTypes(XkbDescPtr xkb,
            xkbSetMapReq * req,
            xkbKeyTypeWireDesc * wire, XkbChangesPtr changes)
{
    register unsigned i;
    unsigned first, last;
    CARD8 *map;

    if ((unsigned) (req->firstType + req->nTypes) > xkb->map->size_types) {
        i = req->firstType + req->nTypes;
        if (XkbAllocClientMap(xkb, XkbKeyTypesMask, i) != Success) {
            return NULL;
        }
    }
    if ((unsigned) (req->firstType + req->nTypes) > xkb->map->num_types)
        xkb->map->num_types = req->firstType + req->nTypes;

    for (i = 0; i < req->nTypes; i++) {
        XkbKeyTypePtr pOld;
        register unsigned n;

        if (XkbResizeKeyType(xkb, i + req->firstType, wire->nMapEntries,
                             wire->preserve, wire->numLevels) != Success) {
            return NULL;
        }
        pOld = &xkb->map->types[i + req->firstType];
        map = (CARD8 *) &wire[1];

        pOld->mods.real_mods = wire->realMods;
        pOld->mods.vmods = wire->virtualMods;
        pOld->num_levels = wire->numLevels;
        pOld->map_count = wire->nMapEntries;

        pOld->mods.mask = pOld->mods.real_mods |
            XkbMaskForVMask(xkb, pOld->mods.vmods);

        if (wire->nMapEntries) {
            xkbKTSetMapEntryWireDesc *mapWire;
            xkbModsWireDesc *preWire;
            unsigned tmp;

            mapWire = (xkbKTSetMapEntryWireDesc *) map;
            preWire = (xkbModsWireDesc *) &mapWire[wire->nMapEntries];
            for (n = 0; n < wire->nMapEntries; n++) {
                pOld->map[n].active = 1;
                pOld->map[n].mods.mask = mapWire[n].realMods;
                pOld->map[n].mods.real_mods = mapWire[n].realMods;
                pOld->map[n].mods.vmods = mapWire[n].virtualMods;
                pOld->map[n].level = mapWire[n].level;
                if (mapWire[n].virtualMods != 0) {
                    tmp = XkbMaskForVMask(xkb, mapWire[n].virtualMods);
                    pOld->map[n].active = (tmp != 0);
                    pOld->map[n].mods.mask |= tmp;
                }
                if (wire->preserve) {
                    pOld->preserve[n].real_mods = preWire[n].realMods;
                    pOld->preserve[n].vmods = preWire[n].virtualMods;
                    tmp = XkbMaskForVMask(xkb, preWire[n].virtualMods);
                    pOld->preserve[n].mask = preWire[n].realMods | tmp;
                }
            }
            if (wire->preserve)
                map = (CARD8 *) &preWire[wire->nMapEntries];
            else
                map = (CARD8 *) &mapWire[wire->nMapEntries];
        }
        else
            map = (CARD8 *) &wire[1];
        wire = (xkbKeyTypeWireDesc *) map;
    }
    first = req->firstType;
    last = first + req->nTypes - 1;     /* last changed type */
    if (changes->map.changed & XkbKeyTypesMask) {
        int oldLast;

        oldLast = changes->map.first_type + changes->map.num_types - 1;
        if (changes->map.first_type < first)
            first = changes->map.first_type;
        if (oldLast > last)
            last = oldLast;
    }
    changes->map.changed |= XkbKeyTypesMask;
    changes->map.first_type = first;
    changes->map.num_types = (last - first) + 1;
    return (char *) wire;
}

static char *
SetKeySyms(ClientPtr client,
           XkbDescPtr xkb,
           xkbSetMapReq * req,
           xkbSymMapWireDesc * wire, XkbChangesPtr changes, DeviceIntPtr dev)
{
    register unsigned i, s;
    XkbSymMapPtr oldMap;
    KeySym *newSyms;
    KeySym *pSyms;
    unsigned first, last;

    oldMap = &xkb->map->key_sym_map[req->firstKeySym];
    for (i = 0; i < req->nKeySyms; i++, oldMap++) {
        pSyms = (KeySym *) &wire[1];
        if (wire->nSyms > 0) {
            newSyms = XkbResizeKeySyms(xkb, i + req->firstKeySym, wire->nSyms);
            for (s = 0; s < wire->nSyms; s++) {
                newSyms[s] = pSyms[s];
            }
            if (client->swapped) {
                for (s = 0; s < wire->nSyms; s++) {
                    swapl(&newSyms[s]);
                }
            }
        }
        if (XkbKeyHasActions(xkb, i + req->firstKeySym))
            XkbResizeKeyActions(xkb, i + req->firstKeySym,
                                XkbNumGroups(wire->groupInfo) * wire->width);
        oldMap->kt_index[0] = wire->ktIndex[0];
        oldMap->kt_index[1] = wire->ktIndex[1];
        oldMap->kt_index[2] = wire->ktIndex[2];
        oldMap->kt_index[3] = wire->ktIndex[3];
        oldMap->group_info = wire->groupInfo;
        oldMap->width = wire->width;
        wire = (xkbSymMapWireDesc *) &pSyms[wire->nSyms];
    }
    first = req->firstKeySym;
    last = first + req->nKeySyms - 1;
    if (changes->map.changed & XkbKeySymsMask) {
        int oldLast =
            (changes->map.first_key_sym + changes->map.num_key_syms - 1);
        if (changes->map.first_key_sym < first)
            first = changes->map.first_key_sym;
        if (oldLast > last)
            last = oldLast;
    }
    changes->map.changed |= XkbKeySymsMask;
    changes->map.first_key_sym = first;
    changes->map.num_key_syms = (last - first + 1);

    s = 0;
    for (i = xkb->min_key_code; i <= xkb->max_key_code; i++) {
        if (XkbKeyNumGroups(xkb, i) > s)
            s = XkbKeyNumGroups(xkb, i);
    }
    if (s != xkb->ctrls->num_groups) {
        xkbControlsNotify cn;
        XkbControlsRec old;

        cn.keycode = 0;
        cn.eventType = 0;
        cn.requestMajor = XkbReqCode;
        cn.requestMinor = X_kbSetMap;
        old = *xkb->ctrls;
        xkb->ctrls->num_groups = s;
        if (XkbComputeControlsNotify(dev, &old, xkb->ctrls, &cn, FALSE))
            XkbSendControlsNotify(dev, &cn);
    }
    return (char *) wire;
}

static char *
SetKeyActions(XkbDescPtr xkb,
              xkbSetMapReq * req, CARD8 *wire, XkbChangesPtr changes)
{
    register unsigned i, first, last;
    CARD8 *nActs = wire;
    XkbAction *newActs;

    wire += XkbPaddedSize(req->nKeyActs);
    for (i = 0; i < req->nKeyActs; i++) {
        if (nActs[i] == 0)
            xkb->server->key_acts[i + req->firstKeyAct] = 0;
        else {
            newActs = XkbResizeKeyActions(xkb, i + req->firstKeyAct, nActs[i]);
            memcpy((char *) newActs, (char *) wire,
                   nActs[i] * SIZEOF(xkbActionWireDesc));
            wire += nActs[i] * SIZEOF(xkbActionWireDesc);
        }
    }
    first = req->firstKeyAct;
    last = (first + req->nKeyActs - 1);
    if (changes->map.changed & XkbKeyActionsMask) {
        int oldLast;

        oldLast = changes->map.first_key_act + changes->map.num_key_acts - 1;
        if (changes->map.first_key_act < first)
            first = changes->map.first_key_act;
        if (oldLast > last)
            last = oldLast;
    }
    changes->map.changed |= XkbKeyActionsMask;
    changes->map.first_key_act = first;
    changes->map.num_key_acts = (last - first + 1);
    return (char *) wire;
}

static char *
SetKeyBehaviors(XkbSrvInfoPtr xkbi,
                xkbSetMapReq * req,
                xkbBehaviorWireDesc * wire, XkbChangesPtr changes)
{
    register unsigned i;
    int maxRG = -1;
    XkbDescPtr xkb = xkbi->desc;
    XkbServerMapPtr server = xkb->server;
    unsigned first, last;

    first = req->firstKeyBehavior;
    last = req->firstKeyBehavior + req->nKeyBehaviors - 1;
    memset(&server->behaviors[first], 0,
           req->nKeyBehaviors * sizeof(XkbBehavior));
    for (i = 0; i < req->totalKeyBehaviors; i++) {
        if ((server->behaviors[wire->key].type & XkbKB_Permanent) == 0) {
            server->behaviors[wire->key].type = wire->type;
            server->behaviors[wire->key].data = wire->data;
            if ((wire->type == XkbKB_RadioGroup) &&
                (((int) wire->data) > maxRG))
                maxRG = wire->data + 1;
        }
        wire++;
    }

    if (maxRG > (int) xkbi->nRadioGroups) {
        if (xkbi->radioGroups)
            xkbi->radioGroups = reallocarray(xkbi->radioGroups, maxRG,
                                             sizeof(XkbRadioGroupRec));
        else
            xkbi->radioGroups = calloc(maxRG, sizeof(XkbRadioGroupRec));
        if (xkbi->radioGroups) {
            if (xkbi->nRadioGroups)
                memset(&xkbi->radioGroups[xkbi->nRadioGroups], 0,
                       (maxRG - xkbi->nRadioGroups) * sizeof(XkbRadioGroupRec));
            xkbi->nRadioGroups = maxRG;
        }
        else
            xkbi->nRadioGroups = 0;
        /* should compute members here */
    }
    if (changes->map.changed & XkbKeyBehaviorsMask) {
        unsigned oldLast;

        oldLast = changes->map.first_key_behavior +
            changes->map.num_key_behaviors - 1;
        if (changes->map.first_key_behavior < req->firstKeyBehavior)
            first = changes->map.first_key_behavior;
        if (oldLast > last)
            last = oldLast;
    }
    changes->map.changed |= XkbKeyBehaviorsMask;
    changes->map.first_key_behavior = first;
    changes->map.num_key_behaviors = (last - first + 1);
    return (char *) wire;
}

static char *
SetVirtualMods(XkbSrvInfoPtr xkbi, xkbSetMapReq * req, CARD8 *wire,
               XkbChangesPtr changes)
{
    register int i, bit, nMods;
    XkbServerMapPtr srv = xkbi->desc->server;

    if (((req->present & XkbVirtualModsMask) == 0) || (req->virtualMods == 0))
        return (char *) wire;
    for (i = nMods = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (req->virtualMods & bit) {
            if (srv->vmods[i] != wire[nMods]) {
                changes->map.changed |= XkbVirtualModsMask;
                changes->map.vmods |= bit;
                srv->vmods[i] = wire[nMods];
            }
            nMods++;
        }
    }
    return (char *) (wire + XkbPaddedSize(nMods));
}

static char *
SetKeyExplicit(XkbSrvInfoPtr xkbi, xkbSetMapReq * req, CARD8 *wire,
               XkbChangesPtr changes)
{
    register unsigned i, first, last;
    XkbServerMapPtr xkb = xkbi->desc->server;
    CARD8 *start;

    start = wire;
    first = req->firstKeyExplicit;
    last = req->firstKeyExplicit + req->nKeyExplicit - 1;
    memset(&xkb->explicit[first], 0, req->nKeyExplicit);
    for (i = 0; i < req->totalKeyExplicit; i++, wire += 2) {
        xkb->explicit[wire[0]] = wire[1];
    }
    if (first > 0) {
        if (changes->map.changed & XkbExplicitComponentsMask) {
            int oldLast;

            oldLast = changes->map.first_key_explicit +
                changes->map.num_key_explicit - 1;
            if (changes->map.first_key_explicit < first)
                first = changes->map.first_key_explicit;
            if (oldLast > last)
                last = oldLast;
        }
        changes->map.first_key_explicit = first;
        changes->map.num_key_explicit = (last - first) + 1;
    }
    wire += XkbPaddedSize(wire - start) - (wire - start);
    return (char *) wire;
}

static char *
SetModifierMap(XkbSrvInfoPtr xkbi,
               xkbSetMapReq * req, CARD8 *wire, XkbChangesPtr changes)
{
    register unsigned i, first, last;
    XkbClientMapPtr xkb = xkbi->desc->map;
    CARD8 *start;

    start = wire;
    first = req->firstModMapKey;
    last = req->firstModMapKey + req->nModMapKeys - 1;
    memset(&xkb->modmap[first], 0, req->nModMapKeys);
    for (i = 0; i < req->totalModMapKeys; i++, wire += 2) {
        xkb->modmap[wire[0]] = wire[1];
    }
    if (first > 0) {
        if (changes->map.changed & XkbModifierMapMask) {
            int oldLast;

            oldLast = changes->map.first_modmap_key +
                changes->map.num_modmap_keys - 1;
            if (changes->map.first_modmap_key < first)
                first = changes->map.first_modmap_key;
            if (oldLast > last)
                last = oldLast;
        }
        changes->map.first_modmap_key = first;
        changes->map.num_modmap_keys = (last - first) + 1;
    }
    wire += XkbPaddedSize(wire - start) - (wire - start);
    return (char *) wire;
}

static char *
SetVirtualModMap(XkbSrvInfoPtr xkbi,
                 xkbSetMapReq * req,
                 xkbVModMapWireDesc * wire, XkbChangesPtr changes)
{
    register unsigned i, first, last;
    XkbServerMapPtr srv = xkbi->desc->server;

    first = req->firstVModMapKey;
    last = req->firstVModMapKey + req->nVModMapKeys - 1;
    memset(&srv->vmodmap[first], 0, req->nVModMapKeys * sizeof(unsigned short));
    for (i = 0; i < req->totalVModMapKeys; i++, wire++) {
        srv->vmodmap[wire->key] = wire->vmods;
    }
    if (first > 0) {
        if (changes->map.changed & XkbVirtualModMapMask) {
            int oldLast;

            oldLast = changes->map.first_vmodmap_key +
                changes->map.num_vmodmap_keys - 1;
            if (changes->map.first_vmodmap_key < first)
                first = changes->map.first_vmodmap_key;
            if (oldLast > last)
                last = oldLast;
        }
        changes->map.first_vmodmap_key = first;
        changes->map.num_vmodmap_keys = (last - first) + 1;
    }
    return (char *) wire;
}

#define _add_check_len(new) \
    if (len > UINT32_MAX - (new) || len > req_len - (new)) goto bad; \
    else len += new

/**
 * Check the length of the SetMap request
 */
static int
_XkbSetMapCheckLength(xkbSetMapReq *req)
{
    size_t len = sz_xkbSetMapReq, req_len = req->length << 2;
    xkbKeyTypeWireDesc *keytype;
    xkbSymMapWireDesc *symmap;
    BOOL preserve;
    int i, map_count, nSyms;

    if (req_len < len)
        goto bad;
    /* types */
    if (req->present & XkbKeyTypesMask) {
        keytype = (xkbKeyTypeWireDesc *)(req + 1);
        for (i = 0; i < req->nTypes; i++) {
            _add_check_len(XkbPaddedSize(sz_xkbKeyTypeWireDesc));
            _add_check_len(keytype->nMapEntries
                           * sz_xkbKTSetMapEntryWireDesc);
            preserve = keytype->preserve;
            map_count = keytype->nMapEntries;
            if (preserve) {
                _add_check_len(map_count * sz_xkbModsWireDesc);
            }
            keytype += 1;
            keytype = (xkbKeyTypeWireDesc *)
                      ((xkbKTSetMapEntryWireDesc *)keytype + map_count);
            if (preserve)
                keytype = (xkbKeyTypeWireDesc *)
                          ((xkbModsWireDesc *)keytype + map_count);
        }
    }
    /* syms */
    if (req->present & XkbKeySymsMask) {
        symmap = (xkbSymMapWireDesc *)((char *)req + len);
        for (i = 0; i < req->nKeySyms; i++) {
            _add_check_len(sz_xkbSymMapWireDesc);
            nSyms = symmap->nSyms;
            _add_check_len(nSyms*sizeof(CARD32));
            symmap += 1;
            symmap = (xkbSymMapWireDesc *)((CARD32 *)symmap + nSyms);
        }
    }
    /* actions */
    if (req->present & XkbKeyActionsMask) {
        _add_check_len(req->totalActs * sz_xkbActionWireDesc 
                       + XkbPaddedSize(req->nKeyActs));
    }
    /* behaviours */
    if (req->present & XkbKeyBehaviorsMask) {
        _add_check_len(req->totalKeyBehaviors * sz_xkbBehaviorWireDesc);
    }
    /* vmods */
    if (req->present & XkbVirtualModsMask) {
        _add_check_len(XkbPaddedSize(Ones(req->virtualMods)));
    }
    /* explicit */
    if (req->present & XkbExplicitComponentsMask) {
        /* two bytes per non-zero explicit componen */
        _add_check_len(XkbPaddedSize(req->totalKeyExplicit * sizeof(CARD16)));
    }
    /* modmap */
    if (req->present & XkbModifierMapMask) {
         /* two bytes per non-zero modmap component */
        _add_check_len(XkbPaddedSize(req->totalModMapKeys * sizeof(CARD16)));
    }
    /* vmodmap */
    if (req->present & XkbVirtualModMapMask) {
        _add_check_len(req->totalVModMapKeys * sz_xkbVModMapWireDesc);
    }
    if (len == req_len)
        return Success;
bad:
    ErrorF("[xkb] BOGUS LENGTH in SetMap: expected %ld got %ld\n",
           len, req_len);
    return BadLength;
}


/**
 * Check if the given request can be applied to the given device but don't
 * actually do anything, except swap values when client->swapped and doswap are both true.
 */
static int
_XkbSetMapChecks(ClientPtr client, DeviceIntPtr dev, xkbSetMapReq * req,
                 char *values, Bool doswap)
{
    XkbSrvInfoPtr xkbi;
    XkbDescPtr xkb;
    int error;
    int nTypes = 0, nActions;
    CARD8 mapWidths[XkbMaxLegalKeyCode + 1] = { 0 };
    CARD16 symsPerKey[XkbMaxLegalKeyCode + 1] = { 0 };
    XkbSymMapPtr map;
    int i;

    if (!dev->key)
        return 0;

    xkbi = dev->key->xkbInfo;
    xkb = xkbi->desc;

    if ((xkb->min_key_code != req->minKeyCode) ||
        (xkb->max_key_code != req->maxKeyCode)) {
        if (client->xkbClientFlags & _XkbClientIsAncient) {
            /* pre 1.0 versions of Xlib have a bug */
            req->minKeyCode = xkb->min_key_code;
            req->maxKeyCode = xkb->max_key_code;
        }
        else {
            if (!XkbIsLegalKeycode(req->minKeyCode)) {
                client->errorValue =
                    _XkbErrCode3(2, req->minKeyCode, req->maxKeyCode);
                return BadValue;
            }
            if (req->minKeyCode > req->maxKeyCode) {
                client->errorValue =
                    _XkbErrCode3(3, req->minKeyCode, req->maxKeyCode);
                return BadMatch;
            }
        }
    }

    /* nTypes/mapWidths/symsPerKey must be filled for further tests below,
     * regardless of client-side flags */

    if (!CheckKeyTypes(client, xkb, req, (xkbKeyTypeWireDesc **) &values,
		       &nTypes, mapWidths, doswap)) {
	    client->errorValue = nTypes;
	    return BadValue;
    }

    map = &xkb->map->key_sym_map[xkb->min_key_code];
    for (i = xkb->min_key_code; i < xkb->max_key_code; i++, map++) {
        register int g, ng, w;

        ng = XkbNumGroups(map->group_info);
        for (w = g = 0; g < ng; g++) {
            if (map->kt_index[g] >= (unsigned) nTypes) {
                client->errorValue = _XkbErrCode4(0x13, i, g, map->kt_index[g]);
                return BadValue;
            }
            if (mapWidths[map->kt_index[g]] > w)
                w = mapWidths[map->kt_index[g]];
        }
        symsPerKey[i] = w * ng;
    }

    if ((req->present & XkbKeySymsMask) &&
        (!CheckKeySyms(client, xkb, req, nTypes, mapWidths, symsPerKey,
                       (xkbSymMapWireDesc **) &values, &error, doswap))) {
        client->errorValue = error;
        return BadValue;
    }

    if ((req->present & XkbKeyActionsMask) &&
        (!CheckKeyActions(xkb, req, nTypes, mapWidths, symsPerKey,
                          (CARD8 **) &values, &nActions))) {
        client->errorValue = nActions;
        return BadValue;
    }

    if ((req->present & XkbKeyBehaviorsMask) &&
        (!CheckKeyBehaviors
         (xkb, req, (xkbBehaviorWireDesc **) &values, &error))) {
        client->errorValue = error;
        return BadValue;
    }

    if ((req->present & XkbVirtualModsMask) &&
        (!CheckVirtualMods(xkb, req, (CARD8 **) &values, &error))) {
        client->errorValue = error;
        return BadValue;
    }
    if ((req->present & XkbExplicitComponentsMask) &&
        (!CheckKeyExplicit(xkb, req, (CARD8 **) &values, &error))) {
        client->errorValue = error;
        return BadValue;
    }
    if ((req->present & XkbModifierMapMask) &&
        (!CheckModifierMap(xkb, req, (CARD8 **) &values, &error))) {
        client->errorValue = error;
        return BadValue;
    }
    if ((req->present & XkbVirtualModMapMask) &&
        (!CheckVirtualModMap
         (xkb, req, (xkbVModMapWireDesc **) &values, &error))) {
        client->errorValue = error;
        return BadValue;
    }

    if (((values - ((char *) req)) / 4) != req->length) {
        ErrorF("[xkb] Internal error! Bad length in XkbSetMap (after check)\n");
        client->errorValue = values - ((char *) &req[1]);
        return BadLength;
    }

    return Success;
}

/**
 * Apply the given request on the given device.
 */
static int
_XkbSetMap(ClientPtr client, DeviceIntPtr dev, xkbSetMapReq * req, char *values)
{
    XkbEventCauseRec cause;
    XkbChangesRec change;
    Bool sentNKN;
    XkbSrvInfoPtr xkbi;
    XkbDescPtr xkb;

    if (!dev->key)
        return Success;

    xkbi = dev->key->xkbInfo;
    xkb = xkbi->desc;

    XkbSetCauseXkbReq(&cause, X_kbSetMap, client);
    memset(&change, 0, sizeof(change));
    sentNKN = FALSE;
    if ((xkb->min_key_code != req->minKeyCode) ||
        (xkb->max_key_code != req->maxKeyCode)) {
        Status status;
        xkbNewKeyboardNotify nkn;

        nkn.deviceID = nkn.oldDeviceID = dev->id;
        nkn.oldMinKeyCode = xkb->min_key_code;
        nkn.oldMaxKeyCode = xkb->max_key_code;
        status = XkbChangeKeycodeRange(xkb, req->minKeyCode,
                                       req->maxKeyCode, &change);
        if (status != Success)
            return status;      /* oh-oh. what about the other keyboards? */
        nkn.minKeyCode = xkb->min_key_code;
        nkn.maxKeyCode = xkb->max_key_code;
        nkn.requestMajor = XkbReqCode;
        nkn.requestMinor = X_kbSetMap;
        nkn.changed = XkbNKN_KeycodesMask;
        XkbSendNewKeyboardNotify(dev, &nkn);
        sentNKN = TRUE;
    }

    if (req->present & XkbKeyTypesMask) {
        values = SetKeyTypes(xkb, req, (xkbKeyTypeWireDesc *) values, &change);
        if (!values)
            goto allocFailure;
    }
    if (req->present & XkbKeySymsMask) {
        values =
            SetKeySyms(client, xkb, req, (xkbSymMapWireDesc *) values, &change,
                       dev);
        if (!values)
            goto allocFailure;
    }
    if (req->present & XkbKeyActionsMask) {
        values = SetKeyActions(xkb, req, (CARD8 *) values, &change);
        if (!values)
            goto allocFailure;
    }
    if (req->present & XkbKeyBehaviorsMask) {
        values =
            SetKeyBehaviors(xkbi, req, (xkbBehaviorWireDesc *) values, &change);
        if (!values)
            goto allocFailure;
    }
    if (req->present & XkbVirtualModsMask)
        values = SetVirtualMods(xkbi, req, (CARD8 *) values, &change);
    if (req->present & XkbExplicitComponentsMask)
        values = SetKeyExplicit(xkbi, req, (CARD8 *) values, &change);
    if (req->present & XkbModifierMapMask)
        values = SetModifierMap(xkbi, req, (CARD8 *) values, &change);
    if (req->present & XkbVirtualModMapMask)
        values =
            SetVirtualModMap(xkbi, req, (xkbVModMapWireDesc *) values, &change);
    if (((values - ((char *) req)) / 4) != req->length) {
        ErrorF("[xkb] Internal error! Bad length in XkbSetMap (after set)\n");
        client->errorValue = values - ((char *) &req[1]);
        return BadLength;
    }
    if (req->flags & XkbSetMapRecomputeActions) {
        KeyCode first, last, firstMM, lastMM;

        if (change.map.num_key_syms > 0) {
            first = change.map.first_key_sym;
            last = first + change.map.num_key_syms - 1;
        }
        else
            first = last = 0;
        if (change.map.num_modmap_keys > 0) {
            firstMM = change.map.first_modmap_key;
            lastMM = firstMM + change.map.num_modmap_keys - 1;
        }
        else
            firstMM = lastMM = 0;
        if ((last > 0) && (lastMM > 0)) {
            if (firstMM < first)
                first = firstMM;
            if (lastMM > last)
                last = lastMM;
        }
        else if (lastMM > 0) {
            first = firstMM;
            last = lastMM;
        }
        if (last > 0) {
            unsigned check = 0;

            XkbUpdateActions(dev, first, (last - first + 1), &change, &check,
                             &cause);
            if (check)
                XkbCheckSecondaryEffects(xkbi, check, &change, &cause);
        }
    }
    if (!sentNKN)
        XkbSendNotification(dev, &change, &cause);

    return Success;
 allocFailure:
    return BadAlloc;
}

int
ProcXkbSetMap(ClientPtr client)
{
    DeviceIntPtr dev, master;
    char *tmp;
    int rc;

    REQUEST(xkbSetMapReq);
    REQUEST_AT_LEAST_SIZE(xkbSetMapReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixManageAccess);
    CHK_MASK_LEGAL(0x01, stuff->present, XkbAllMapComponentsMask);

    /* first verify the request length carefully */
    rc = _XkbSetMapCheckLength(stuff);
    if (rc != Success)
        return rc;

    tmp = (char *) &stuff[1];

    /* Check if we can to the SetMap on the requested device. If this
       succeeds, do the same thing for all extension devices (if needed).
       If any of them fails, fail.  */
    rc = _XkbSetMapChecks(client, dev, stuff, tmp, TRUE);

    if (rc != Success)
        return rc;

    master = GetMaster(dev, MASTER_KEYBOARD);

    if (stuff->deviceSpec == XkbUseCoreKbd) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {
                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success) {
                    rc = _XkbSetMapChecks(client, other, stuff, tmp, FALSE);
                    if (rc != Success)
                        return rc;
                }
            }
        }
    } else {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if (other != dev && GetMaster(other, MASTER_KEYBOARD) != dev &&
                (other != master || dev != master->lastSlave))
                continue;

            rc = _XkbSetMapChecks(client, other, stuff, tmp, FALSE);
            if (rc != Success)
                return rc;
        }
    }

    /* We know now that we will succeed with the SetMap. In theory anyway. */
    rc = _XkbSetMap(client, dev, stuff, tmp);
    if (rc != Success)
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {
                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success)
                    _XkbSetMap(client, other, stuff, tmp);
                /* ignore rc. if the SetMap failed although the check above
                   reported true there isn't much we can do. we still need to
                   set all other devices, hoping that at least they stay in
                   sync. */
            }
        }
    } else {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if (other != dev && GetMaster(other, MASTER_KEYBOARD) != dev &&
                (other != master || dev != master->lastSlave))
                continue;

            _XkbSetMap(client, other, stuff, tmp); //ignore rc
        }
    }

    return Success;
}

/***====================================================================***/

static Status
XkbComputeGetCompatMapReplySize(XkbCompatMapPtr compat,
                                xkbGetCompatMapReply * rep)
{
    unsigned size, nGroups;

    nGroups = 0;
    if (rep->groups != 0) {
        register int i, bit;

        for (i = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
            if (rep->groups & bit)
                nGroups++;
        }
    }
    size = nGroups * SIZEOF(xkbModsWireDesc);
    size += (rep->nSI * SIZEOF(xkbSymInterpretWireDesc));
    rep->length = size / 4;
    return Success;
}

static int
XkbSendCompatMap(ClientPtr client,
                 XkbCompatMapPtr compat, xkbGetCompatMapReply * rep)
{
    char *data;
    int size;

    if (rep->length > 0) {
        data = xallocarray(rep->length, 4);
        if (data) {
            register unsigned i, bit;
            xkbModsWireDesc *grp;
            XkbSymInterpretPtr sym = &compat->sym_interpret[rep->firstSI];
            xkbSymInterpretWireDesc *wire = (xkbSymInterpretWireDesc *) data;

            size = rep->length * 4;

            for (i = 0; i < rep->nSI; i++, sym++, wire++) {
                wire->sym = sym->sym;
                wire->mods = sym->mods;
                wire->match = sym->match;
                wire->virtualMod = sym->virtual_mod;
                wire->flags = sym->flags;
                memcpy((char *) &wire->act, (char *) &sym->act,
                       sz_xkbActionWireDesc);
                if (client->swapped) {
                    swapl(&wire->sym);
                }
            }
            if (rep->groups) {
                grp = (xkbModsWireDesc *) wire;
                for (i = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
                    if (rep->groups & bit) {
                        grp->mask = compat->groups[i].mask;
                        grp->realMods = compat->groups[i].real_mods;
                        grp->virtualMods = compat->groups[i].vmods;
                        if (client->swapped) {
                            swaps(&grp->virtualMods);
                        }
                        grp++;
                    }
                }
                wire = (xkbSymInterpretWireDesc *) grp;
            }
        }
        else
            return BadAlloc;
    }
    else
        data = NULL;

    if (client->swapped) {
        swaps(&rep->sequenceNumber);
        swapl(&rep->length);
        swaps(&rep->firstSI);
        swaps(&rep->nSI);
        swaps(&rep->nTotalSI);
    }

    WriteToClient(client, SIZEOF(xkbGetCompatMapReply), rep);
    if (data) {
        WriteToClient(client, size, data);
        free((char *) data);
    }
    return Success;
}

int
ProcXkbGetCompatMap(ClientPtr client)
{
    xkbGetCompatMapReply rep;
    DeviceIntPtr dev;
    XkbDescPtr xkb;
    XkbCompatMapPtr compat;

    REQUEST(xkbGetCompatMapReq);
    REQUEST_SIZE_MATCH(xkbGetCompatMapReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixGetAttrAccess);

    xkb = dev->key->xkbInfo->desc;
    compat = xkb->compat;

    rep = (xkbGetCompatMapReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .deviceID = dev->id,
        .firstSI = stuff->firstSI,
        .nSI = stuff->nSI
    };
    if (stuff->getAllSI) {
        rep.firstSI = 0;
        rep.nSI = compat->num_si;
    }
    else if ((((unsigned) stuff->nSI) > 0) &&
             ((unsigned) (stuff->firstSI + stuff->nSI - 1) >= compat->num_si)) {
        client->errorValue = _XkbErrCode2(0x05, compat->num_si);
        return BadValue;
    }
    rep.nTotalSI = compat->num_si;
    rep.groups = stuff->groups;
    XkbComputeGetCompatMapReplySize(compat, &rep);
    return XkbSendCompatMap(client, compat, &rep);
}

/**
 * Apply the given request on the given device.
 * If dryRun is TRUE, then value checks are performed, but the device isn't
 * modified.
 */
static int
_XkbSetCompatMap(ClientPtr client, DeviceIntPtr dev,
                 xkbSetCompatMapReq * req, char *data, BOOL dryRun)
{
    XkbSrvInfoPtr xkbi;
    XkbDescPtr xkb;
    XkbCompatMapPtr compat;
    int nGroups;
    unsigned i, bit;

    xkbi = dev->key->xkbInfo;
    xkb = xkbi->desc;
    compat = xkb->compat;

    if ((req->nSI > 0) || (req->truncateSI)) {
        xkbSymInterpretWireDesc *wire;

        if (req->firstSI > compat->num_si) {
            client->errorValue = _XkbErrCode2(0x02, compat->num_si);
            return BadValue;
        }
        wire = (xkbSymInterpretWireDesc *) data;
        wire += req->nSI;
        data = (char *) wire;
    }

    nGroups = 0;
    if (req->groups != 0) {
        for (i = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
            if (req->groups & bit)
                nGroups++;
        }
    }
    data += nGroups * SIZEOF(xkbModsWireDesc);
    if (((data - ((char *) req)) / 4) != req->length) {
        return BadLength;
    }

    /* Done all the checks we can do */
    if (dryRun)
        return Success;

    data = (char *) &req[1];
    if (req->nSI > 0) {
        xkbSymInterpretWireDesc *wire = (xkbSymInterpretWireDesc *) data;
        XkbSymInterpretPtr sym;
        unsigned int skipped = 0;

        if ((unsigned) (req->firstSI + req->nSI) > compat->num_si) {
            compat->num_si = req->firstSI + req->nSI;
            compat->sym_interpret = reallocarray(compat->sym_interpret,
                                                 compat->num_si,
                                                 sizeof(XkbSymInterpretRec));
            if (!compat->sym_interpret) {
                compat->num_si = 0;
                return BadAlloc;
            }
        }
        else if (req->truncateSI) {
            compat->num_si = req->firstSI + req->nSI;
        }
        sym = &compat->sym_interpret[req->firstSI];
        for (i = 0; i < req->nSI; i++, wire++) {
            if (client->swapped) {
                swapl(&wire->sym);
            }
            if (wire->sym == NoSymbol && wire->match == XkbSI_AnyOfOrNone &&
                (wire->mods & 0xff) == 0xff &&
                wire->act.type == XkbSA_XFree86Private) {
                ErrorF("XKB: Skipping broken Any+AnyOfOrNone(All) -> Private "
                       "action from client\n");
                skipped++;
                continue;
            }
            sym->sym = wire->sym;
            sym->mods = wire->mods;
            sym->match = wire->match;
            sym->flags = wire->flags;
            sym->virtual_mod = wire->virtualMod;
            memcpy((char *) &sym->act, (char *) &wire->act,
                   SIZEOF(xkbActionWireDesc));
            sym++;
        }
        if (skipped) {
            if (req->firstSI + req->nSI < compat->num_si)
                memmove(sym, sym + skipped,
                        (compat->num_si - req->firstSI - req->nSI) *
                        sizeof(*sym));
            compat->num_si -= skipped;
        }
        data = (char *) wire;
    }
    else if (req->truncateSI) {
        compat->num_si = req->firstSI;
    }

    if (req->groups != 0) {
        xkbModsWireDesc *wire = (xkbModsWireDesc *) data;

        for (i = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
            if (req->groups & bit) {
                if (client->swapped) {
                    swaps(&wire->virtualMods);
                }
                compat->groups[i].mask = wire->realMods;
                compat->groups[i].real_mods = wire->realMods;
                compat->groups[i].vmods = wire->virtualMods;
                if (wire->virtualMods != 0) {
                    unsigned tmp;

                    tmp = XkbMaskForVMask(xkb, wire->virtualMods);
                    compat->groups[i].mask |= tmp;
                }
                data += SIZEOF(xkbModsWireDesc);
                wire = (xkbModsWireDesc *) data;
            }
        }
    }
    i = XkbPaddedSize((data - ((char *) req)));
    if ((i / 4) != req->length) {
        ErrorF("[xkb] Internal length error on read in _XkbSetCompatMap\n");
        return BadLength;
    }

    if (dev->xkb_interest) {
        xkbCompatMapNotify ev;

        ev.deviceID = dev->id;
        ev.changedGroups = req->groups;
        ev.firstSI = req->firstSI;
        ev.nSI = req->nSI;
        ev.nTotalSI = compat->num_si;
        XkbSendCompatMapNotify(dev, &ev);
    }

    if (req->recomputeActions) {
        XkbChangesRec change;
        unsigned check;
        XkbEventCauseRec cause;

        XkbSetCauseXkbReq(&cause, X_kbSetCompatMap, client);
        memset(&change, 0, sizeof(XkbChangesRec));
        XkbUpdateActions(dev, xkb->min_key_code, XkbNumKeys(xkb), &change,
                         &check, &cause);
        if (check)
            XkbCheckSecondaryEffects(xkbi, check, &change, &cause);
        XkbSendNotification(dev, &change, &cause);
    }
    return Success;
}

int
ProcXkbSetCompatMap(ClientPtr client)
{
    DeviceIntPtr dev;
    char *data;
    int rc;

    REQUEST(xkbSetCompatMapReq);
    REQUEST_AT_LEAST_SIZE(xkbSetCompatMapReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixManageAccess);

    data = (char *) &stuff[1];

    /* check first using a dry-run */
    rc = _XkbSetCompatMap(client, dev, stuff, data, TRUE);
    if (rc != Success)
        return rc;
    if (stuff->deviceSpec == XkbUseCoreKbd) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {
                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success) {
                    /* dry-run */
                    rc = _XkbSetCompatMap(client, other, stuff, data, TRUE);
                    if (rc != Success)
                        return rc;
                }
            }
        }
    }

    /* Yay, the dry-runs succeed. Let's apply */
    rc = _XkbSetCompatMap(client, dev, stuff, data, FALSE);
    if (rc != Success)
        return rc;
    if (stuff->deviceSpec == XkbUseCoreKbd) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {
                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success) {
                    rc = _XkbSetCompatMap(client, other, stuff, data, FALSE);
                    if (rc != Success)
                        return rc;
                }
            }
        }
    }

    return Success;
}

/***====================================================================***/

int
ProcXkbGetIndicatorState(ClientPtr client)
{
    xkbGetIndicatorStateReply rep;
    XkbSrvLedInfoPtr sli;
    DeviceIntPtr dev;

    REQUEST(xkbGetIndicatorStateReq);
    REQUEST_SIZE_MATCH(xkbGetIndicatorStateReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixReadAccess);

    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId,
                            XkbXI_IndicatorStateMask);
    if (!sli)
        return BadAlloc;

    rep = (xkbGetIndicatorStateReply) {
        .type = X_Reply,
        .deviceID = dev->id,
        .sequenceNumber = client->sequence,
        .length = 0,
        .state = sli->effectiveState
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.state);
    }
    WriteToClient(client, SIZEOF(xkbGetIndicatorStateReply), &rep);
    return Success;
}

/***====================================================================***/

static Status
XkbComputeGetIndicatorMapReplySize(XkbIndicatorPtr indicators,
                                   xkbGetIndicatorMapReply * rep)
{
    register int i, bit;
    int nIndicators;

    rep->realIndicators = indicators->phys_indicators;
    for (i = nIndicators = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
        if (rep->which & bit)
            nIndicators++;
    }
    rep->length = (nIndicators * SIZEOF(xkbIndicatorMapWireDesc)) / 4;
    rep->nIndicators = nIndicators;
    return Success;
}

static int
XkbSendIndicatorMap(ClientPtr client,
                    XkbIndicatorPtr indicators, xkbGetIndicatorMapReply * rep)
{
    int length;
    CARD8 *map;
    register int i;
    register unsigned bit;

    if (rep->length > 0) {
        CARD8 *to;

        to = map = xallocarray(rep->length, 4);
        if (map) {
            xkbIndicatorMapWireDesc *wire = (xkbIndicatorMapWireDesc *) to;

            length = rep->length * 4;

            for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
                if (rep->which & bit) {
                    wire->flags = indicators->maps[i].flags;
                    wire->whichGroups = indicators->maps[i].which_groups;
                    wire->groups = indicators->maps[i].groups;
                    wire->whichMods = indicators->maps[i].which_mods;
                    wire->mods = indicators->maps[i].mods.mask;
                    wire->realMods = indicators->maps[i].mods.real_mods;
                    wire->virtualMods = indicators->maps[i].mods.vmods;
                    wire->ctrls = indicators->maps[i].ctrls;
                    if (client->swapped) {
                        swaps(&wire->virtualMods);
                        swapl(&wire->ctrls);
                    }
                    wire++;
                }
            }
            to = (CARD8 *) wire;
            if ((to - map) != length) {
                client->errorValue = _XkbErrCode2(0xff, length);
                free(map);
                return BadLength;
            }
        }
        else
            return BadAlloc;
    }
    else
        map = NULL;
    if (client->swapped) {
        swaps(&rep->sequenceNumber);
        swapl(&rep->length);
        swapl(&rep->which);
        swapl(&rep->realIndicators);
    }
    WriteToClient(client, SIZEOF(xkbGetIndicatorMapReply), rep);
    if (map) {
        WriteToClient(client, length, map);
        free((char *) map);
    }
    return Success;
}

int
ProcXkbGetIndicatorMap(ClientPtr client)
{
    xkbGetIndicatorMapReply rep;
    DeviceIntPtr dev;
    XkbDescPtr xkb;
    XkbIndicatorPtr leds;

    REQUEST(xkbGetIndicatorMapReq);
    REQUEST_SIZE_MATCH(xkbGetIndicatorMapReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixGetAttrAccess);

    xkb = dev->key->xkbInfo->desc;
    leds = xkb->indicators;

    rep = (xkbGetIndicatorMapReply) {
        .type = X_Reply,
        .deviceID = dev->id,
        .sequenceNumber = client->sequence,
        .length = 0,
        .which = stuff->which
    };
    XkbComputeGetIndicatorMapReplySize(leds, &rep);
    return XkbSendIndicatorMap(client, leds, &rep);
}

/**
 * Apply the given map to the given device. Which specifies which components
 * to apply.
 */
static int
_XkbSetIndicatorMap(ClientPtr client, DeviceIntPtr dev,
                    int which, xkbIndicatorMapWireDesc * desc)
{
    XkbSrvInfoPtr xkbi;
    XkbSrvLedInfoPtr sli;
    XkbEventCauseRec cause;
    int i, bit;

    xkbi = dev->key->xkbInfo;

    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId,
                            XkbXI_IndicatorMapsMask);
    if (!sli)
        return BadAlloc;

    for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
        if (which & bit) {
            sli->maps[i].flags = desc->flags;
            sli->maps[i].which_groups = desc->whichGroups;
            sli->maps[i].groups = desc->groups;
            sli->maps[i].which_mods = desc->whichMods;
            sli->maps[i].mods.mask = desc->mods;
            sli->maps[i].mods.real_mods = desc->mods;
            sli->maps[i].mods.vmods = desc->virtualMods;
            sli->maps[i].ctrls = desc->ctrls;
            if (desc->virtualMods != 0) {
                unsigned tmp;

                tmp = XkbMaskForVMask(xkbi->desc, desc->virtualMods);
                sli->maps[i].mods.mask = desc->mods | tmp;
            }
            desc++;
        }
    }

    XkbSetCauseXkbReq(&cause, X_kbSetIndicatorMap, client);
    XkbApplyLedMapChanges(dev, sli, which, NULL, NULL, &cause);

    return Success;
}

int
ProcXkbSetIndicatorMap(ClientPtr client)
{
    int i, bit;
    int nIndicators;
    DeviceIntPtr dev;
    xkbIndicatorMapWireDesc *from;
    int rc;

    REQUEST(xkbSetIndicatorMapReq);
    REQUEST_AT_LEAST_SIZE(xkbSetIndicatorMapReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixSetAttrAccess);

    if (stuff->which == 0)
        return Success;

    for (nIndicators = i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
        if (stuff->which & bit)
            nIndicators++;
    }
    if (stuff->length != ((SIZEOF(xkbSetIndicatorMapReq) +
                           (nIndicators * SIZEOF(xkbIndicatorMapWireDesc))) /
                          4)) {
        return BadLength;
    }

    from = (xkbIndicatorMapWireDesc *) &stuff[1];
    for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
        if (stuff->which & bit) {
            if (client->swapped) {
                swaps(&from->virtualMods);
                swapl(&from->ctrls);
            }
            CHK_MASK_LEGAL(i, from->whichGroups, XkbIM_UseAnyGroup);
            CHK_MASK_LEGAL(i, from->whichMods, XkbIM_UseAnyMods);
            from++;
        }
    }

    from = (xkbIndicatorMapWireDesc *) &stuff[1];
    rc = _XkbSetIndicatorMap(client, dev, stuff->which, from);
    if (rc != Success)
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {
                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixSetAttrAccess);
                if (rc == Success)
                    _XkbSetIndicatorMap(client, other, stuff->which, from);
            }
        }
    }

    return Success;
}

/***====================================================================***/

int
ProcXkbGetNamedIndicator(ClientPtr client)
{
    DeviceIntPtr dev;
    xkbGetNamedIndicatorReply rep;
    register int i = 0;
    XkbSrvLedInfoPtr sli;
    XkbIndicatorMapPtr map = NULL;

    REQUEST(xkbGetNamedIndicatorReq);
    REQUEST_SIZE_MATCH(xkbGetNamedIndicatorReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_LED_DEVICE(dev, stuff->deviceSpec, client, DixReadAccess);
    CHK_ATOM_ONLY(stuff->indicator);

    sli = XkbFindSrvLedInfo(dev, stuff->ledClass, stuff->ledID, 0);
    if (!sli)
        return BadAlloc;

    i = 0;
    map = NULL;
    if ((sli->names) && (sli->maps)) {
        for (i = 0; i < XkbNumIndicators; i++) {
            if (stuff->indicator == sli->names[i]) {
                map = &sli->maps[i];
                break;
            }
        }
    }

    rep = (xkbGetNamedIndicatorReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .deviceID = dev->id,
        .indicator = stuff->indicator
    };
    if (map != NULL) {
        rep.found = TRUE;
        rep.on = ((sli->effectiveState & (1 << i)) != 0);
        rep.realIndicator = ((sli->physIndicators & (1 << i)) != 0);
        rep.ndx = i;
        rep.flags = map->flags;
        rep.whichGroups = map->which_groups;
        rep.groups = map->groups;
        rep.whichMods = map->which_mods;
        rep.mods = map->mods.mask;
        rep.realMods = map->mods.real_mods;
        rep.virtualMods = map->mods.vmods;
        rep.ctrls = map->ctrls;
        rep.supported = TRUE;
    }
    else {
        rep.found = FALSE;
        rep.on = FALSE;
        rep.realIndicator = FALSE;
        rep.ndx = XkbNoIndicator;
        rep.flags = 0;
        rep.whichGroups = 0;
        rep.groups = 0;
        rep.whichMods = 0;
        rep.mods = 0;
        rep.realMods = 0;
        rep.virtualMods = 0;
        rep.ctrls = 0;
        rep.supported = TRUE;
    }
    if (client->swapped) {
        swapl(&rep.length);
        swaps(&rep.sequenceNumber);
        swapl(&rep.indicator);
        swaps(&rep.virtualMods);
        swapl(&rep.ctrls);
    }

    WriteToClient(client, SIZEOF(xkbGetNamedIndicatorReply), &rep);
    return Success;
}

/**
 * Find the IM on the device.
 * Returns the map, or NULL if the map doesn't exist.
 * If the return value is NULL, led_return is undefined. Otherwise, led_return
 * is set to the led index of the map.
 */
static XkbIndicatorMapPtr
_XkbFindNamedIndicatorMap(XkbSrvLedInfoPtr sli, Atom indicator, int *led_return)
{
    XkbIndicatorMapPtr map;

    /* search for the right indicator */
    map = NULL;
    if (sli->names && sli->maps) {
        int led;

        for (led = 0; (led < XkbNumIndicators) && (map == NULL); led++) {
            if (sli->names[led] == indicator) {
                map = &sli->maps[led];
                *led_return = led;
                break;
            }
        }
    }

    return map;
}

/**
 * Creates an indicator map on the device. If dryRun is TRUE, it only checks
 * if creation is possible, but doesn't actually create it.
 */
static int
_XkbCreateIndicatorMap(DeviceIntPtr dev, Atom indicator,
                       int ledClass, int ledID,
                       XkbIndicatorMapPtr * map_return, int *led_return,
                       Bool dryRun)
{
    XkbSrvLedInfoPtr sli;
    XkbIndicatorMapPtr map;
    int led;

    sli = XkbFindSrvLedInfo(dev, ledClass, ledID, XkbXI_IndicatorsMask);
    if (!sli)
        return BadAlloc;

    map = _XkbFindNamedIndicatorMap(sli, indicator, &led);

    if (!map) {
        /* find first unused indicator maps and assign the name to it */
        for (led = 0, map = NULL; (led < XkbNumIndicators) && (map == NULL);
             led++) {
            if ((sli->names) && (sli->maps) && (sli->names[led] == None) &&
                (!XkbIM_InUse(&sli->maps[led]))) {
                map = &sli->maps[led];
                if (!dryRun)
                    sli->names[led] = indicator;
                break;
            }
        }
    }

    if (!map)
        return BadAlloc;

    *led_return = led;
    *map_return = map;
    return Success;
}

static int
_XkbSetNamedIndicator(ClientPtr client, DeviceIntPtr dev,
                      xkbSetNamedIndicatorReq * stuff)
{
    unsigned int extDevReason;
    unsigned int statec, namec, mapc;
    XkbSrvLedInfoPtr sli;
    int led = 0;
    XkbIndicatorMapPtr map;
    DeviceIntPtr kbd;
    XkbEventCauseRec cause;
    xkbExtensionDeviceNotify ed;
    XkbChangesRec changes;
    int rc;

    rc = _XkbCreateIndicatorMap(dev, stuff->indicator, stuff->ledClass,
                                stuff->ledID, &map, &led, FALSE);
    if (rc != Success || !map)  /* oh-oh */
        return rc;

    sli = XkbFindSrvLedInfo(dev, stuff->ledClass, stuff->ledID,
                            XkbXI_IndicatorsMask);
    if (!sli)
        return BadAlloc;

    namec = mapc = statec = 0;
    extDevReason = 0;

    namec |= (1 << led);
    sli->namesPresent |= ((stuff->indicator != None) ? (1 << led) : 0);
    extDevReason |= XkbXI_IndicatorNamesMask;

    if (stuff->setMap) {
        map->flags = stuff->flags;
        map->which_groups = stuff->whichGroups;
        map->groups = stuff->groups;
        map->which_mods = stuff->whichMods;
        map->mods.mask = stuff->realMods;
        map->mods.real_mods = stuff->realMods;
        map->mods.vmods = stuff->virtualMods;
        map->ctrls = stuff->ctrls;
        mapc |= (1 << led);
    }

    if ((stuff->setState) && ((map->flags & XkbIM_NoExplicit) == 0)) {
        if (stuff->on)
            sli->explicitState |= (1 << led);
        else
            sli->explicitState &= ~(1 << led);
        statec |= ((sli->effectiveState ^ sli->explicitState) & (1 << led));
    }

    memset((char *) &ed, 0, sizeof(xkbExtensionDeviceNotify));
    memset((char *) &changes, 0, sizeof(XkbChangesRec));
    XkbSetCauseXkbReq(&cause, X_kbSetNamedIndicator, client);
    if (namec)
        XkbApplyLedNameChanges(dev, sli, namec, &ed, &changes, &cause);
    if (mapc)
        XkbApplyLedMapChanges(dev, sli, mapc, &ed, &changes, &cause);
    if (statec)
        XkbApplyLedStateChanges(dev, sli, statec, &ed, &changes, &cause);

    kbd = dev;
    if ((sli->flags & XkbSLI_HasOwnState) == 0)
        kbd = inputInfo.keyboard;
    XkbFlushLedEvents(dev, kbd, sli, &ed, &changes, &cause);

    return Success;
}

int
ProcXkbSetNamedIndicator(ClientPtr client)
{
    int rc;
    DeviceIntPtr dev;
    int led = 0;
    XkbIndicatorMapPtr map;

    REQUEST(xkbSetNamedIndicatorReq);
    REQUEST_SIZE_MATCH(xkbSetNamedIndicatorReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_LED_DEVICE(dev, stuff->deviceSpec, client, DixSetAttrAccess);
    CHK_ATOM_ONLY(stuff->indicator);
    CHK_MASK_LEGAL(0x10, stuff->whichGroups, XkbIM_UseAnyGroup);
    CHK_MASK_LEGAL(0x11, stuff->whichMods, XkbIM_UseAnyMods);

    /* Dry-run for checks */
    rc = _XkbCreateIndicatorMap(dev, stuff->indicator,
                                stuff->ledClass, stuff->ledID,
                                &map, &led, TRUE);
    if (rc != Success || !map)  /* couldn't be created or didn't exist */
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd ||
        stuff->deviceSpec == XkbUseCorePtr) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev && (other->kbdfeed ||
                                                             other->leds) &&
                (XaceHook(XACE_DEVICE_ACCESS, client, other, DixSetAttrAccess)
                 == Success)) {
                rc = _XkbCreateIndicatorMap(other, stuff->indicator,
                                            stuff->ledClass, stuff->ledID, &map,
                                            &led, TRUE);
                if (rc != Success || !map)
                    return rc;
            }
        }
    }

    /* All checks passed, let's do it */
    rc = _XkbSetNamedIndicator(client, dev, stuff);
    if (rc != Success)
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd ||
        stuff->deviceSpec == XkbUseCorePtr) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev && (other->kbdfeed ||
                                                             other->leds) &&
                (XaceHook(XACE_DEVICE_ACCESS, client, other, DixSetAttrAccess)
                 == Success)) {
                _XkbSetNamedIndicator(client, other, stuff);
            }
        }
    }

    return Success;
}

/***====================================================================***/

static CARD32
_XkbCountAtoms(Atom *atoms, int maxAtoms, int *count)
{
    register unsigned int i, bit, nAtoms;
    register CARD32 atomsPresent;

    for (i = nAtoms = atomsPresent = 0, bit = 1; i < maxAtoms; i++, bit <<= 1) {
        if (atoms[i] != None) {
            atomsPresent |= bit;
            nAtoms++;
        }
    }
    if (count)
        *count = nAtoms;
    return atomsPresent;
}

static char *
_XkbWriteAtoms(char *wire, Atom *atoms, int maxAtoms, int swap)
{
    register unsigned int i;
    Atom *atm;

    atm = (Atom *) wire;
    for (i = 0; i < maxAtoms; i++) {
        if (atoms[i] != None) {
            *atm = atoms[i];
            if (swap) {
                swapl(atm);
            }
            atm++;
        }
    }
    return (char *) atm;
}

static Status
XkbComputeGetNamesReplySize(XkbDescPtr xkb, xkbGetNamesReply * rep)
{
    register unsigned which, length;
    register int i;

    rep->minKeyCode = xkb->min_key_code;
    rep->maxKeyCode = xkb->max_key_code;
    which = rep->which;
    length = 0;
    if (xkb->names != NULL) {
        if (which & XkbKeycodesNameMask)
            length++;
        if (which & XkbGeometryNameMask)
            length++;
        if (which & XkbSymbolsNameMask)
            length++;
        if (which & XkbPhysSymbolsNameMask)
            length++;
        if (which & XkbTypesNameMask)
            length++;
        if (which & XkbCompatNameMask)
            length++;
    }
    else
        which &= ~XkbComponentNamesMask;

    if (xkb->map != NULL) {
        if (which & XkbKeyTypeNamesMask)
            length += xkb->map->num_types;
        rep->nTypes = xkb->map->num_types;
        if (which & XkbKTLevelNamesMask) {
            XkbKeyTypePtr pType = xkb->map->types;
            int nKTLevels = 0;

            length += XkbPaddedSize(xkb->map->num_types) / 4;
            for (i = 0; i < xkb->map->num_types; i++, pType++) {
                if (pType->level_names != NULL)
                    nKTLevels += pType->num_levels;
            }
            rep->nKTLevels = nKTLevels;
            length += nKTLevels;
        }
    }
    else {
        rep->nTypes = 0;
        rep->nKTLevels = 0;
        which &= ~(XkbKeyTypeNamesMask | XkbKTLevelNamesMask);
    }

    rep->minKeyCode = xkb->min_key_code;
    rep->maxKeyCode = xkb->max_key_code;
    rep->indicators = 0;
    rep->virtualMods = 0;
    rep->groupNames = 0;
    if (xkb->names != NULL) {
        if (which & XkbIndicatorNamesMask) {
            int nLeds;

            rep->indicators =
                _XkbCountAtoms(xkb->names->indicators, XkbNumIndicators,
                               &nLeds);
            length += nLeds;
            if (nLeds == 0)
                which &= ~XkbIndicatorNamesMask;
        }

        if (which & XkbVirtualModNamesMask) {
            int nVMods;

            rep->virtualMods =
                _XkbCountAtoms(xkb->names->vmods, XkbNumVirtualMods, &nVMods);
            length += nVMods;
            if (nVMods == 0)
                which &= ~XkbVirtualModNamesMask;
        }

        if (which & XkbGroupNamesMask) {
            int nGroups;

            rep->groupNames =
                _XkbCountAtoms(xkb->names->groups, XkbNumKbdGroups, &nGroups);
            length += nGroups;
            if (nGroups == 0)
                which &= ~XkbGroupNamesMask;
        }

        if ((which & XkbKeyNamesMask) && (xkb->names->keys))
            length += rep->nKeys;
        else
            which &= ~XkbKeyNamesMask;

        if ((which & XkbKeyAliasesMask) &&
            (xkb->names->key_aliases) && (xkb->names->num_key_aliases > 0)) {
            rep->nKeyAliases = xkb->names->num_key_aliases;
            length += rep->nKeyAliases * 2;
        }
        else {
            which &= ~XkbKeyAliasesMask;
            rep->nKeyAliases = 0;
        }

        if ((which & XkbRGNamesMask) && (xkb->names->num_rg > 0))
            length += xkb->names->num_rg;
        else
            which &= ~XkbRGNamesMask;
    }
    else {
        which &= ~(XkbIndicatorNamesMask | XkbVirtualModNamesMask);
        which &= ~(XkbGroupNamesMask | XkbKeyNamesMask | XkbKeyAliasesMask);
        which &= ~XkbRGNamesMask;
    }

    rep->length = length;
    rep->which = which;
    return Success;
}

static int
XkbSendNames(ClientPtr client, XkbDescPtr xkb, xkbGetNamesReply * rep)
{
    register unsigned i, length, which;
    char *start;
    char *desc;

    length = rep->length * 4;
    which = rep->which;
    if (client->swapped) {
        swaps(&rep->sequenceNumber);
        swapl(&rep->length);
        swapl(&rep->which);
        swaps(&rep->virtualMods);
        swapl(&rep->indicators);
    }

    start = desc = calloc(1, length);
    if (!start)
        return BadAlloc;
    if (xkb->names) {
        if (which & XkbKeycodesNameMask) {
            *((CARD32 *) desc) = xkb->names->keycodes;
            if (client->swapped) {
                swapl((int *) desc);
            }
            desc += 4;
        }
        if (which & XkbGeometryNameMask) {
            *((CARD32 *) desc) = xkb->names->geometry;
            if (client->swapped) {
                swapl((int *) desc);
            }
            desc += 4;
        }
        if (which & XkbSymbolsNameMask) {
            *((CARD32 *) desc) = xkb->names->symbols;
            if (client->swapped) {
                swapl((int *) desc);
            }
            desc += 4;
        }
        if (which & XkbPhysSymbolsNameMask) {
            register CARD32 *atm = (CARD32 *) desc;

            atm[0] = (CARD32) xkb->names->phys_symbols;
            if (client->swapped) {
                swapl(&atm[0]);
            }
            desc += 4;
        }
        if (which & XkbTypesNameMask) {
            *((CARD32 *) desc) = (CARD32) xkb->names->types;
            if (client->swapped) {
                swapl((int *) desc);
            }
            desc += 4;
        }
        if (which & XkbCompatNameMask) {
            *((CARD32 *) desc) = (CARD32) xkb->names->compat;
            if (client->swapped) {
                swapl((int *) desc);
            }
            desc += 4;
        }
        if (which & XkbKeyTypeNamesMask) {
            register CARD32 *atm = (CARD32 *) desc;
            register XkbKeyTypePtr type = xkb->map->types;

            for (i = 0; i < xkb->map->num_types; i++, atm++, type++) {
                *atm = (CARD32) type->name;
                if (client->swapped) {
                    swapl(atm);
                }
            }
            desc = (char *) atm;
        }
        if (which & XkbKTLevelNamesMask && xkb->map) {
            XkbKeyTypePtr type = xkb->map->types;
            register CARD32 *atm;

            for (i = 0; i < rep->nTypes; i++, type++) {
                *desc++ = type->num_levels;
            }
            desc += XkbPaddedSize(rep->nTypes) - rep->nTypes;

            atm = (CARD32 *) desc;
            type = xkb->map->types;
            for (i = 0; i < xkb->map->num_types; i++, type++) {
                register unsigned l;

                if (type->level_names) {
                    for (l = 0; l < type->num_levels; l++, atm++) {
                        *atm = type->level_names[l];
                        if (client->swapped) {
                            swapl(atm);
                        }
                    }
                    desc += type->num_levels * 4;
                }
            }
        }
        if (which & XkbIndicatorNamesMask) {
            desc =
                _XkbWriteAtoms(desc, xkb->names->indicators, XkbNumIndicators,
                               client->swapped);
        }
        if (which & XkbVirtualModNamesMask) {
            desc = _XkbWriteAtoms(desc, xkb->names->vmods, XkbNumVirtualMods,
                                  client->swapped);
        }
        if (which & XkbGroupNamesMask) {
            desc = _XkbWriteAtoms(desc, xkb->names->groups, XkbNumKbdGroups,
                                  client->swapped);
        }
        if (which & XkbKeyNamesMask) {
            for (i = 0; i < rep->nKeys; i++, desc += sizeof(XkbKeyNameRec)) {
                *((XkbKeyNamePtr) desc) = xkb->names->keys[i + rep->firstKey];
            }
        }
        if (which & XkbKeyAliasesMask) {
            XkbKeyAliasPtr pAl;

            pAl = xkb->names->key_aliases;
            for (i = 0; i < rep->nKeyAliases;
                 i++, pAl++, desc += 2 * XkbKeyNameLength) {
                *((XkbKeyAliasPtr) desc) = *pAl;
            }
        }
        if ((which & XkbRGNamesMask) && (rep->nRadioGroups > 0)) {
            register CARD32 *atm = (CARD32 *) desc;

            for (i = 0; i < rep->nRadioGroups; i++, atm++) {
                *atm = (CARD32) xkb->names->radio_groups[i];
                if (client->swapped) {
                    swapl(atm);
                }
            }
            desc += rep->nRadioGroups * 4;
        }
    }

    if ((desc - start) != (length)) {
        ErrorF("[xkb] BOGUS LENGTH in write names, expected %d, got %ld\n",
               length, (unsigned long) (desc - start));
    }
    WriteToClient(client, SIZEOF(xkbGetNamesReply), rep);
    WriteToClient(client, length, start);
    free((char *) start);
    return Success;
}

int
ProcXkbGetNames(ClientPtr client)
{
    DeviceIntPtr dev;
    XkbDescPtr xkb;
    xkbGetNamesReply rep;

    REQUEST(xkbGetNamesReq);
    REQUEST_SIZE_MATCH(xkbGetNamesReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixGetAttrAccess);
    CHK_MASK_LEGAL(0x01, stuff->which, XkbAllNamesMask);

    xkb = dev->key->xkbInfo->desc;
    rep = (xkbGetNamesReply) {
        .type = X_Reply,
        .deviceID = dev->id,
        .sequenceNumber = client->sequence,
        .length = 0,
        .which = stuff->which,
        .nTypes = xkb->map->num_types,
        .firstKey = xkb->min_key_code,
        .nKeys = XkbNumKeys(xkb),
        .nKeyAliases = xkb->names ? xkb->names->num_key_aliases : 0,
        .nRadioGroups = xkb->names ? xkb->names->num_rg : 0
    };
    XkbComputeGetNamesReplySize(xkb, &rep);
    return XkbSendNames(client, xkb, &rep);
}

/***====================================================================***/

static CARD32 *
_XkbCheckAtoms(CARD32 *wire, int nAtoms, int swapped, Atom *pError)
{
    register int i;

    for (i = 0; i < nAtoms; i++, wire++) {
        if (swapped) {
            swapl(wire);
        }
        if ((((Atom) *wire) != None) && (!ValidAtom((Atom) *wire))) {
            *pError = ((Atom) *wire);
            return NULL;
        }
    }
    return wire;
}

static CARD32 *
_XkbCheckMaskedAtoms(CARD32 *wire, int nAtoms, CARD32 present, int swapped,
                     Atom *pError)
{
    register unsigned i, bit;

    for (i = 0, bit = 1; (i < nAtoms) && (present); i++, bit <<= 1) {
        if ((present & bit) == 0)
            continue;
        if (swapped) {
            swapl(wire);
        }
        if ((((Atom) *wire) != None) && (!ValidAtom(((Atom) *wire)))) {
            *pError = (Atom) *wire;
            return NULL;
        }
        wire++;
    }
    return wire;
}

static Atom *
_XkbCopyMaskedAtoms(Atom *wire, Atom *dest, int nAtoms, CARD32 present)
{
    register int i, bit;

    for (i = 0, bit = 1; (i < nAtoms) && (present); i++, bit <<= 1) {
        if ((present & bit) == 0)
            continue;
        dest[i] = *wire++;
    }
    return wire;
}

static Bool
_XkbCheckTypeName(Atom name, int typeNdx)
{
    const char *str;

    str = NameForAtom(name);
    if ((strcmp(str, "ONE_LEVEL") == 0) || (strcmp(str, "TWO_LEVEL") == 0) ||
        (strcmp(str, "ALPHABETIC") == 0) || (strcmp(str, "KEYPAD") == 0))
        return FALSE;
    return TRUE;
}

/**
 * Check the device-dependent data in the request against the device. Returns
 * Success, or the appropriate error code.
 */
static int
_XkbSetNamesCheck(ClientPtr client, DeviceIntPtr dev,
                  xkbSetNamesReq * stuff, CARD32 *data)
{
    XkbDescRec *xkb;
    CARD32 *tmp;
    Atom bad = None;

    tmp = data;
    xkb = dev->key->xkbInfo->desc;

    if (stuff->which & XkbKeyTypeNamesMask) {
        int i;
        CARD32 *old;

        if (stuff->nTypes < 1) {
            client->errorValue = _XkbErrCode2(0x02, stuff->nTypes);
            return BadValue;
        }
        if ((unsigned) (stuff->firstType + stuff->nTypes - 1) >=
            xkb->map->num_types) {
            client->errorValue =
                _XkbErrCode4(0x03, stuff->firstType, stuff->nTypes,
                             xkb->map->num_types);
            return BadValue;
        }
        if (((unsigned) stuff->firstType) <= XkbLastRequiredType) {
            client->errorValue = _XkbErrCode2(0x04, stuff->firstType);
            return BadAccess;
        }
        if (!_XkbCheckRequestBounds(client, stuff, tmp, tmp + stuff->nTypes))
            return BadLength;
        old = tmp;
        tmp = _XkbCheckAtoms(tmp, stuff->nTypes, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
        for (i = 0; i < stuff->nTypes; i++, old++) {
            if (!_XkbCheckTypeName((Atom) *old, stuff->firstType + i))
                client->errorValue = _XkbErrCode2(0x05, i);
        }
    }
    if (stuff->which & XkbKTLevelNamesMask) {
        unsigned i;
        XkbKeyTypePtr type;
        CARD8 *width;

        if (stuff->nKTLevels < 1) {
            client->errorValue = _XkbErrCode2(0x05, stuff->nKTLevels);
            return BadValue;
        }
        if ((unsigned) (stuff->firstKTLevel + stuff->nKTLevels - 1) >=
            xkb->map->num_types) {
            client->errorValue = _XkbErrCode4(0x06, stuff->firstKTLevel,
                                              stuff->nKTLevels,
                                              xkb->map->num_types);
            return BadValue;
        }
        width = (CARD8 *) tmp;
        tmp = (CARD32 *) (((char *) tmp) + XkbPaddedSize(stuff->nKTLevels));
        if (!_XkbCheckRequestBounds(client, stuff, width, tmp))
            return BadLength;
        type = &xkb->map->types[stuff->firstKTLevel];
        for (i = 0; i < stuff->nKTLevels; i++, type++) {
            if (width[i] == 0)
                continue;
            else if (width[i] != type->num_levels) {
                client->errorValue = _XkbErrCode4(0x07, i + stuff->firstKTLevel,
                                                  type->num_levels, width[i]);
                return BadMatch;
            }
            if (!_XkbCheckRequestBounds(client, stuff, tmp, tmp + width[i]))
                return BadLength;
            tmp = _XkbCheckAtoms(tmp, width[i], client->swapped, &bad);
            if (!tmp) {
                client->errorValue = bad;
                return BadAtom;
            }
        }
    }
    if (stuff->which & XkbIndicatorNamesMask) {
        if (stuff->indicators == 0) {
            client->errorValue = 0x08;
            return BadMatch;
        }
        if (!_XkbCheckRequestBounds(client, stuff, tmp,
                                    tmp + Ones(stuff->indicators)))
            return BadLength;
        tmp = _XkbCheckMaskedAtoms(tmp, XkbNumIndicators, stuff->indicators,
                                   client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbVirtualModNamesMask) {
        if (stuff->virtualMods == 0) {
            client->errorValue = 0x09;
            return BadMatch;
        }
        if (!_XkbCheckRequestBounds(client, stuff, tmp,
                                    tmp + Ones(stuff->virtualMods)))
            return BadLength;
        tmp = _XkbCheckMaskedAtoms(tmp, XkbNumVirtualMods,
                                   (CARD32) stuff->virtualMods,
                                   client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbGroupNamesMask) {
        if (stuff->groupNames == 0) {
            client->errorValue = 0x0a;
            return BadMatch;
        }
        if (!_XkbCheckRequestBounds(client, stuff, tmp,
                                    tmp + Ones(stuff->groupNames)))
            return BadLength;
        tmp = _XkbCheckMaskedAtoms(tmp, XkbNumKbdGroups,
                                   (CARD32) stuff->groupNames,
                                   client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (stuff->which & XkbKeyNamesMask) {
        if (stuff->firstKey < (unsigned) xkb->min_key_code) {
            client->errorValue = _XkbErrCode3(0x0b, xkb->min_key_code,
                                              stuff->firstKey);
            return BadValue;
        }
        if (((unsigned) (stuff->firstKey + stuff->nKeys - 1) >
             xkb->max_key_code) || (stuff->nKeys < 1)) {
            client->errorValue =
                _XkbErrCode4(0x0c, xkb->max_key_code, stuff->firstKey,
                             stuff->nKeys);
            return BadValue;
        }
        if (!_XkbCheckRequestBounds(client, stuff, tmp, tmp + stuff->nKeys))
            return BadLength;
        tmp += stuff->nKeys;
    }
    if ((stuff->which & XkbKeyAliasesMask) && (stuff->nKeyAliases > 0)) {
        if (!_XkbCheckRequestBounds(client, stuff, tmp,
                                    tmp + (stuff->nKeyAliases * 2)))
            return BadLength;
        tmp += stuff->nKeyAliases * 2;
    }
    if (stuff->which & XkbRGNamesMask) {
        if (stuff->nRadioGroups < 1) {
            client->errorValue = _XkbErrCode2(0x0d, stuff->nRadioGroups);
            return BadValue;
        }
        if (!_XkbCheckRequestBounds(client, stuff, tmp,
                                    tmp + stuff->nRadioGroups))
            return BadLength;
        tmp = _XkbCheckAtoms(tmp, stuff->nRadioGroups, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if ((tmp - ((CARD32 *) stuff)) != stuff->length) {
        client->errorValue = stuff->length;
        return BadLength;
    }

    return Success;
}

static int
_XkbSetNames(ClientPtr client, DeviceIntPtr dev, xkbSetNamesReq * stuff)
{
    XkbDescRec *xkb;
    XkbNamesRec *names;
    CARD32 *tmp;
    xkbNamesNotify nn;

    tmp = (CARD32 *) &stuff[1];
    xkb = dev->key->xkbInfo->desc;
    names = xkb->names;

    if (XkbAllocNames(xkb, stuff->which, stuff->nRadioGroups,
                      stuff->nKeyAliases) != Success) {
        return BadAlloc;
    }

    memset(&nn, 0, sizeof(xkbNamesNotify));
    nn.changed = stuff->which;
    tmp = (CARD32 *) &stuff[1];
    if (stuff->which & XkbKeycodesNameMask)
        names->keycodes = *tmp++;
    if (stuff->which & XkbGeometryNameMask)
        names->geometry = *tmp++;
    if (stuff->which & XkbSymbolsNameMask)
        names->symbols = *tmp++;
    if (stuff->which & XkbPhysSymbolsNameMask)
        names->phys_symbols = *tmp++;
    if (stuff->which & XkbTypesNameMask)
        names->types = *tmp++;
    if (stuff->which & XkbCompatNameMask)
        names->compat = *tmp++;
    if ((stuff->which & XkbKeyTypeNamesMask) && (stuff->nTypes > 0)) {
        register unsigned i;
        register XkbKeyTypePtr type;

        type = &xkb->map->types[stuff->firstType];
        for (i = 0; i < stuff->nTypes; i++, type++) {
            type->name = *tmp++;
        }
        nn.firstType = stuff->firstType;
        nn.nTypes = stuff->nTypes;
    }
    if (stuff->which & XkbKTLevelNamesMask) {
        register XkbKeyTypePtr type;
        register unsigned i;
        CARD8 *width;

        width = (CARD8 *) tmp;
        tmp = (CARD32 *) (((char *) tmp) + XkbPaddedSize(stuff->nKTLevels));
        type = &xkb->map->types[stuff->firstKTLevel];
        for (i = 0; i < stuff->nKTLevels; i++, type++) {
            if (width[i] > 0) {
                if (type->level_names) {
                    register unsigned n;

                    for (n = 0; n < width[i]; n++) {
                        type->level_names[n] = tmp[n];
                    }
                }
                tmp += width[i];
            }
        }
        nn.firstLevelName = 0;
        nn.nLevelNames = stuff->nTypes;
    }
    if (stuff->which & XkbIndicatorNamesMask) {
        tmp = _XkbCopyMaskedAtoms(tmp, names->indicators, XkbNumIndicators,
                                  stuff->indicators);
        nn.changedIndicators = stuff->indicators;
    }
    if (stuff->which & XkbVirtualModNamesMask) {
        tmp = _XkbCopyMaskedAtoms(tmp, names->vmods, XkbNumVirtualMods,
                                  stuff->virtualMods);
        nn.changedVirtualMods = stuff->virtualMods;
    }
    if (stuff->which & XkbGroupNamesMask) {
        tmp = _XkbCopyMaskedAtoms(tmp, names->groups, XkbNumKbdGroups,
                                  stuff->groupNames);
        nn.changedVirtualMods = stuff->groupNames;
    }
    if (stuff->which & XkbKeyNamesMask) {
        memcpy((char *) &names->keys[stuff->firstKey], (char *) tmp,
               stuff->nKeys * XkbKeyNameLength);
        tmp += stuff->nKeys;
        nn.firstKey = stuff->firstKey;
        nn.nKeys = stuff->nKeys;
    }
    if (stuff->which & XkbKeyAliasesMask) {
        if (stuff->nKeyAliases > 0) {
            register int na = stuff->nKeyAliases;

            if (XkbAllocNames(xkb, XkbKeyAliasesMask, 0, na) != Success)
                return BadAlloc;
            memcpy((char *) names->key_aliases, (char *) tmp,
                   stuff->nKeyAliases * sizeof(XkbKeyAliasRec));
            tmp += stuff->nKeyAliases * 2;
        }
        else if (names->key_aliases != NULL) {
            free(names->key_aliases);
            names->key_aliases = NULL;
            names->num_key_aliases = 0;
        }
        nn.nAliases = names->num_key_aliases;
    }
    if (stuff->which & XkbRGNamesMask) {
        if (stuff->nRadioGroups > 0) {
            register unsigned i, nrg;

            nrg = stuff->nRadioGroups;
            if (XkbAllocNames(xkb, XkbRGNamesMask, nrg, 0) != Success)
                return BadAlloc;

            for (i = 0; i < stuff->nRadioGroups; i++) {
                names->radio_groups[i] = tmp[i];
            }
            tmp += stuff->nRadioGroups;
        }
        else if (names->radio_groups) {
            free(names->radio_groups);
            names->radio_groups = NULL;
            names->num_rg = 0;
        }
        nn.nRadioGroups = names->num_rg;
    }
    if (nn.changed) {
        Bool needExtEvent;

        needExtEvent = (nn.changed & XkbIndicatorNamesMask) != 0;
        XkbSendNamesNotify(dev, &nn);
        if (needExtEvent) {
            XkbSrvLedInfoPtr sli;
            xkbExtensionDeviceNotify edev;
            register int i;
            register unsigned bit;

            sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId,
                                    XkbXI_IndicatorsMask);
            sli->namesPresent = 0;
            for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
                if (names->indicators[i] != None)
                    sli->namesPresent |= bit;
            }
            memset(&edev, 0, sizeof(xkbExtensionDeviceNotify));
            edev.reason = XkbXI_IndicatorNamesMask;
            edev.ledClass = KbdFeedbackClass;
            edev.ledID = dev->kbdfeed->ctrl.id;
            edev.ledsDefined = sli->namesPresent | sli->mapsPresent;
            edev.ledState = sli->effectiveState;
            edev.firstBtn = 0;
            edev.nBtns = 0;
            edev.supported = XkbXI_AllFeaturesMask;
            edev.unsupported = 0;
            XkbSendExtensionDeviceNotify(dev, client, &edev);
        }
    }
    return Success;
}

int
ProcXkbSetNames(ClientPtr client)
{
    DeviceIntPtr dev;
    CARD32 *tmp;
    Atom bad;
    int rc;

    REQUEST(xkbSetNamesReq);
    REQUEST_AT_LEAST_SIZE(xkbSetNamesReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixManageAccess);
    CHK_MASK_LEGAL(0x01, stuff->which, XkbAllNamesMask);

    /* check device-independent stuff */
    tmp = (CARD32 *) &stuff[1];

    if (!_XkbCheckRequestBounds(client, stuff, tmp, tmp + 1))
        return BadLength;
    if (stuff->which & XkbKeycodesNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (!_XkbCheckRequestBounds(client, stuff, tmp, tmp + 1))
        return BadLength;
    if (stuff->which & XkbGeometryNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (!_XkbCheckRequestBounds(client, stuff, tmp, tmp + 1))
        return BadLength;
    if (stuff->which & XkbSymbolsNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (!_XkbCheckRequestBounds(client, stuff, tmp, tmp + 1))
        return BadLength;
    if (stuff->which & XkbPhysSymbolsNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (!_XkbCheckRequestBounds(client, stuff, tmp, tmp + 1))
        return BadLength;
    if (stuff->which & XkbTypesNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }
    if (!_XkbCheckRequestBounds(client, stuff, tmp, tmp + 1))
        return BadLength;
    if (stuff->which & XkbCompatNameMask) {
        tmp = _XkbCheckAtoms(tmp, 1, client->swapped, &bad);
        if (!tmp) {
            client->errorValue = bad;
            return BadAtom;
        }
    }

    /* start of device-dependent tests */
    rc = _XkbSetNamesCheck(client, dev, stuff, tmp);
    if (rc != Success)
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {

                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success) {
                    rc = _XkbSetNamesCheck(client, other, stuff, tmp);
                    if (rc != Success)
                        return rc;
                }
            }
        }
    }

    /* everything is okay -- update names */

    rc = _XkbSetNames(client, dev, stuff);
    if (rc != Success)
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {

                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success)
                    _XkbSetNames(client, other, stuff);
            }
        }
    }

    /* everything is okay -- update names */

    return Success;
}

/***====================================================================***/

#include "xkbgeom.h"

#define	XkbSizeCountedString(s)  ((s)?((((2+strlen(s))+3)/4)*4):4)

/**
 * Write the zero-terminated string str into wire as a pascal string with a
 * 16-bit length field prefixed before the actual string.
 *
 * @param wire The destination array, usually the wire struct
 * @param str The source string as zero-terminated C string
 * @param swap If TRUE, the length field is swapped.
 *
 * @return The input string in the format <string length><string> with a
 * (swapped) 16 bit string length, non-zero terminated.
 */
static char *
XkbWriteCountedString(char *wire, const char *str, Bool swap)
{
    CARD16 len, *pLen, paddedLen;

    if (!str)
        return wire;

    len = strlen(str);
    pLen = (CARD16 *) wire;
    *pLen = len;
    if (swap) {
        swaps(pLen);
    }
    paddedLen = pad_to_int32(sizeof(len) + len) - sizeof(len);
    strncpy(&wire[sizeof(len)], str, paddedLen);
    wire += sizeof(len) + paddedLen;
    return wire;
}

static int
XkbSizeGeomProperties(XkbGeometryPtr geom)
{
    register int i, size;
    XkbPropertyPtr prop;

    for (size = i = 0, prop = geom->properties; i < geom->num_properties;
         i++, prop++) {
        size += XkbSizeCountedString(prop->name);
        size += XkbSizeCountedString(prop->value);
    }
    return size;
}

static char *
XkbWriteGeomProperties(char *wire, XkbGeometryPtr geom, Bool swap)
{
    register int i;
    register XkbPropertyPtr prop;

    for (i = 0, prop = geom->properties; i < geom->num_properties; i++, prop++) {
        wire = XkbWriteCountedString(wire, prop->name, swap);
        wire = XkbWriteCountedString(wire, prop->value, swap);
    }
    return wire;
}

static int
XkbSizeGeomKeyAliases(XkbGeometryPtr geom)
{
    return geom->num_key_aliases * (2 * XkbKeyNameLength);
}

static char *
XkbWriteGeomKeyAliases(char *wire, XkbGeometryPtr geom, Bool swap)
{
    register int sz;

    sz = geom->num_key_aliases * (XkbKeyNameLength * 2);
    if (sz > 0) {
        memcpy(wire, (char *) geom->key_aliases, sz);
        wire += sz;
    }
    return wire;
}

static int
XkbSizeGeomColors(XkbGeometryPtr geom)
{
    register int i, size;
    register XkbColorPtr color;

    for (i = size = 0, color = geom->colors; i < geom->num_colors; i++, color++) {
        size += XkbSizeCountedString(color->spec);
    }
    return size;
}

static char *
XkbWriteGeomColors(char *wire, XkbGeometryPtr geom, Bool swap)
{
    register int i;
    register XkbColorPtr color;

    for (i = 0, color = geom->colors; i < geom->num_colors; i++, color++) {
        wire = XkbWriteCountedString(wire, color->spec, swap);
    }
    return wire;
}

static int
XkbSizeGeomShapes(XkbGeometryPtr geom)
{
    register int i, size;
    register XkbShapePtr shape;

    for (i = size = 0, shape = geom->shapes; i < geom->num_shapes; i++, shape++) {
        register int n;
        register XkbOutlinePtr ol;

        size += SIZEOF(xkbShapeWireDesc);
        for (n = 0, ol = shape->outlines; n < shape->num_outlines; n++, ol++) {
            size += SIZEOF(xkbOutlineWireDesc);
            size += ol->num_points * SIZEOF(xkbPointWireDesc);
        }
    }
    return size;
}

static char *
XkbWriteGeomShapes(char *wire, XkbGeometryPtr geom, Bool swap)
{
    int i;
    XkbShapePtr shape;
    xkbShapeWireDesc *shapeWire;

    for (i = 0, shape = geom->shapes; i < geom->num_shapes; i++, shape++) {
        register int o;
        XkbOutlinePtr ol;
        xkbOutlineWireDesc *olWire;

        shapeWire = (xkbShapeWireDesc *) wire;
        shapeWire->name = shape->name;
        shapeWire->nOutlines = shape->num_outlines;
        if (shape->primary != NULL)
            shapeWire->primaryNdx = XkbOutlineIndex(shape, shape->primary);
        else
            shapeWire->primaryNdx = XkbNoShape;
        if (shape->approx != NULL)
            shapeWire->approxNdx = XkbOutlineIndex(shape, shape->approx);
        else
            shapeWire->approxNdx = XkbNoShape;
        shapeWire->pad = 0;
        if (swap) {
            swapl(&shapeWire->name);
        }
        wire = (char *) &shapeWire[1];
        for (o = 0, ol = shape->outlines; o < shape->num_outlines; o++, ol++) {
            register int p;
            XkbPointPtr pt;
            xkbPointWireDesc *ptWire;

            olWire = (xkbOutlineWireDesc *) wire;
            olWire->nPoints = ol->num_points;
            olWire->cornerRadius = ol->corner_radius;
            olWire->pad = 0;
            wire = (char *) &olWire[1];
            ptWire = (xkbPointWireDesc *) wire;
            for (p = 0, pt = ol->points; p < ol->num_points; p++, pt++) {
                ptWire[p].x = pt->x;
                ptWire[p].y = pt->y;
                if (swap) {
                    swaps(&ptWire[p].x);
                    swaps(&ptWire[p].y);
                }
            }
            wire = (char *) &ptWire[ol->num_points];
        }
    }
    return wire;
}

static int
XkbSizeGeomDoodads(int num_doodads, XkbDoodadPtr doodad)
{
    register int i, size;

    for (i = size = 0; i < num_doodads; i++, doodad++) {
        size += SIZEOF(xkbAnyDoodadWireDesc);
        if (doodad->any.type == XkbTextDoodad) {
            size += XkbSizeCountedString(doodad->text.text);
            size += XkbSizeCountedString(doodad->text.font);
        }
        else if (doodad->any.type == XkbLogoDoodad) {
            size += XkbSizeCountedString(doodad->logo.logo_name);
        }
    }
    return size;
}

static char *
XkbWriteGeomDoodads(char *wire, int num_doodads, XkbDoodadPtr doodad, Bool swap)
{
    register int i;
    xkbDoodadWireDesc *doodadWire;

    for (i = 0; i < num_doodads; i++, doodad++) {
        doodadWire = (xkbDoodadWireDesc *) wire;
        wire = (char *) &doodadWire[1];
        memset(doodadWire, 0, SIZEOF(xkbDoodadWireDesc));
        doodadWire->any.name = doodad->any.name;
        doodadWire->any.type = doodad->any.type;
        doodadWire->any.priority = doodad->any.priority;
        doodadWire->any.top = doodad->any.top;
        doodadWire->any.left = doodad->any.left;
        if (swap) {
            swapl(&doodadWire->any.name);
            swaps(&doodadWire->any.top);
            swaps(&doodadWire->any.left);
        }
        switch (doodad->any.type) {
        case XkbOutlineDoodad:
        case XkbSolidDoodad:
            doodadWire->shape.angle = doodad->shape.angle;
            doodadWire->shape.colorNdx = doodad->shape.color_ndx;
            doodadWire->shape.shapeNdx = doodad->shape.shape_ndx;
            if (swap) {
                swaps(&doodadWire->shape.angle);
            }
            break;
        case XkbTextDoodad:
            doodadWire->text.angle = doodad->text.angle;
            doodadWire->text.width = doodad->text.width;
            doodadWire->text.height = doodad->text.height;
            doodadWire->text.colorNdx = doodad->text.color_ndx;
            if (swap) {
                swaps(&doodadWire->text.angle);
                swaps(&doodadWire->text.width);
                swaps(&doodadWire->text.height);
            }
            wire = XkbWriteCountedString(wire, doodad->text.text, swap);
            wire = XkbWriteCountedString(wire, doodad->text.font, swap);
            break;
        case XkbIndicatorDoodad:
            doodadWire->indicator.shapeNdx = doodad->indicator.shape_ndx;
            doodadWire->indicator.onColorNdx = doodad->indicator.on_color_ndx;
            doodadWire->indicator.offColorNdx = doodad->indicator.off_color_ndx;
            break;
        case XkbLogoDoodad:
            doodadWire->logo.angle = doodad->logo.angle;
            doodadWire->logo.colorNdx = doodad->logo.color_ndx;
            doodadWire->logo.shapeNdx = doodad->logo.shape_ndx;
            wire = XkbWriteCountedString(wire, doodad->logo.logo_name, swap);
            break;
        default:
            ErrorF("[xkb] Unknown doodad type %d in XkbWriteGeomDoodads\n",
                   doodad->any.type);
            ErrorF("[xkb] Ignored\n");
            break;
        }
    }
    return wire;
}

static char *
XkbWriteGeomOverlay(char *wire, XkbOverlayPtr ol, Bool swap)
{
    register int r;
    XkbOverlayRowPtr row;
    xkbOverlayWireDesc *olWire;

    olWire = (xkbOverlayWireDesc *) wire;
    olWire->name = ol->name;
    olWire->nRows = ol->num_rows;
    olWire->pad1 = 0;
    olWire->pad2 = 0;
    if (swap) {
        swapl(&olWire->name);
    }
    wire = (char *) &olWire[1];
    for (r = 0, row = ol->rows; r < ol->num_rows; r++, row++) {
        unsigned int k;
        XkbOverlayKeyPtr key;
        xkbOverlayRowWireDesc *rowWire;

        rowWire = (xkbOverlayRowWireDesc *) wire;
        rowWire->rowUnder = row->row_under;
        rowWire->nKeys = row->num_keys;
        rowWire->pad1 = 0;
        wire = (char *) &rowWire[1];
        for (k = 0, key = row->keys; k < row->num_keys; k++, key++) {
            xkbOverlayKeyWireDesc *keyWire;

            keyWire = (xkbOverlayKeyWireDesc *) wire;
            memcpy(keyWire->over, key->over.name, XkbKeyNameLength);
            memcpy(keyWire->under, key->under.name, XkbKeyNameLength);
            wire = (char *) &keyWire[1];
        }
    }
    return wire;
}

static int
XkbSizeGeomSections(XkbGeometryPtr geom)
{
    register int i, size;
    XkbSectionPtr section;

    for (i = size = 0, section = geom->sections; i < geom->num_sections;
         i++, section++) {
        size += SIZEOF(xkbSectionWireDesc);
        if (section->rows) {
            int r;
            XkbRowPtr row;

            for (r = 0, row = section->rows; r < section->num_rows; row++, r++) {
                size += SIZEOF(xkbRowWireDesc);
                size += row->num_keys * SIZEOF(xkbKeyWireDesc);
            }
        }
        if (section->doodads)
            size += XkbSizeGeomDoodads(section->num_doodads, section->doodads);
        if (section->overlays) {
            int o;
            XkbOverlayPtr ol;

            for (o = 0, ol = section->overlays; o < section->num_overlays;
                 o++, ol++) {
                int r;
                XkbOverlayRowPtr row;

                size += SIZEOF(xkbOverlayWireDesc);
                for (r = 0, row = ol->rows; r < ol->num_rows; r++, row++) {
                    size += SIZEOF(xkbOverlayRowWireDesc);
                    size += row->num_keys * SIZEOF(xkbOverlayKeyWireDesc);
                }
            }
        }
    }
    return size;
}

static char *
XkbWriteGeomSections(char *wire, XkbGeometryPtr geom, Bool swap)
{
    register int i;
    XkbSectionPtr section;
    xkbSectionWireDesc *sectionWire;

    for (i = 0, section = geom->sections; i < geom->num_sections;
         i++, section++) {
        sectionWire = (xkbSectionWireDesc *) wire;
        sectionWire->name = section->name;
        sectionWire->top = section->top;
        sectionWire->left = section->left;
        sectionWire->width = section->width;
        sectionWire->height = section->height;
        sectionWire->angle = section->angle;
        sectionWire->priority = section->priority;
        sectionWire->nRows = section->num_rows;
        sectionWire->nDoodads = section->num_doodads;
        sectionWire->nOverlays = section->num_overlays;
        sectionWire->pad = 0;
        if (swap) {
            swapl(&sectionWire->name);
            swaps(&sectionWire->top);
            swaps(&sectionWire->left);
            swaps(&sectionWire->width);
            swaps(&sectionWire->height);
            swaps(&sectionWire->angle);
        }
        wire = (char *) &sectionWire[1];
        if (section->rows) {
            int r;
            XkbRowPtr row;
            xkbRowWireDesc *rowWire;

            for (r = 0, row = section->rows; r < section->num_rows; r++, row++) {
                rowWire = (xkbRowWireDesc *) wire;
                rowWire->top = row->top;
                rowWire->left = row->left;
                rowWire->nKeys = row->num_keys;
                rowWire->vertical = row->vertical;
                rowWire->pad = 0;
                if (swap) {
                    swaps(&rowWire->top);
                    swaps(&rowWire->left);
                }
                wire = (char *) &rowWire[1];
                if (row->keys) {
                    int k;
                    XkbKeyPtr key;
                    xkbKeyWireDesc *keyWire;

                    keyWire = (xkbKeyWireDesc *) wire;
                    for (k = 0, key = row->keys; k < row->num_keys; k++, key++) {
                        memcpy(keyWire[k].name, key->name.name,
                               XkbKeyNameLength);
                        keyWire[k].gap = key->gap;
                        keyWire[k].shapeNdx = key->shape_ndx;
                        keyWire[k].colorNdx = key->color_ndx;
                        if (swap) {
                            swaps(&keyWire[k].gap);
                        }
                    }
                    wire = (char *) &keyWire[row->num_keys];
                }
            }
        }
        if (section->doodads) {
            wire = XkbWriteGeomDoodads(wire,
                                       section->num_doodads, section->doodads,
                                       swap);
        }
        if (section->overlays) {
            register int o;

            for (o = 0; o < section->num_overlays; o++) {
                wire = XkbWriteGeomOverlay(wire, &section->overlays[o], swap);
            }
        }
    }
    return wire;
}

static Status
XkbComputeGetGeometryReplySize(XkbGeometryPtr geom,
                               xkbGetGeometryReply * rep, Atom name)
{
    int len;

    if (geom != NULL) {
        len = XkbSizeCountedString(geom->label_font);
        len += XkbSizeGeomProperties(geom);
        len += XkbSizeGeomColors(geom);
        len += XkbSizeGeomShapes(geom);
        len += XkbSizeGeomSections(geom);
        len += XkbSizeGeomDoodads(geom->num_doodads, geom->doodads);
        len += XkbSizeGeomKeyAliases(geom);
        rep->length = len / 4;
        rep->found = TRUE;
        rep->name = geom->name;
        rep->widthMM = geom->width_mm;
        rep->heightMM = geom->height_mm;
        rep->nProperties = geom->num_properties;
        rep->nColors = geom->num_colors;
        rep->nShapes = geom->num_shapes;
        rep->nSections = geom->num_sections;
        rep->nDoodads = geom->num_doodads;
        rep->nKeyAliases = geom->num_key_aliases;
        rep->baseColorNdx = XkbGeomColorIndex(geom, geom->base_color);
        rep->labelColorNdx = XkbGeomColorIndex(geom, geom->label_color);
    }
    else {
        rep->length = 0;
        rep->found = FALSE;
        rep->name = name;
        rep->widthMM = rep->heightMM = 0;
        rep->nProperties = rep->nColors = rep->nShapes = 0;
        rep->nSections = rep->nDoodads = 0;
        rep->nKeyAliases = 0;
        rep->labelColorNdx = rep->baseColorNdx = 0;
    }
    return Success;
}
static int
XkbSendGeometry(ClientPtr client,
                XkbGeometryPtr geom, xkbGetGeometryReply * rep, Bool freeGeom)
{
    char *desc, *start;
    int len;

    if (geom != NULL) {
        start = desc = xallocarray(rep->length, 4);
        if (!start)
            return BadAlloc;
        len = rep->length * 4;
        desc = XkbWriteCountedString(desc, geom->label_font, client->swapped);
        if (rep->nProperties > 0)
            desc = XkbWriteGeomProperties(desc, geom, client->swapped);
        if (rep->nColors > 0)
            desc = XkbWriteGeomColors(desc, geom, client->swapped);
        if (rep->nShapes > 0)
            desc = XkbWriteGeomShapes(desc, geom, client->swapped);
        if (rep->nSections > 0)
            desc = XkbWriteGeomSections(desc, geom, client->swapped);
        if (rep->nDoodads > 0)
            desc = XkbWriteGeomDoodads(desc, geom->num_doodads, geom->doodads,
                                       client->swapped);
        if (rep->nKeyAliases > 0)
            desc = XkbWriteGeomKeyAliases(desc, geom, client->swapped);
        if ((desc - start) != (len)) {
            ErrorF
                ("[xkb] BOGUS LENGTH in XkbSendGeometry, expected %d, got %ld\n",
                 len, (unsigned long) (desc - start));
        }
    }
    else {
        len = 0;
        start = NULL;
    }
    if (client->swapped) {
        swaps(&rep->sequenceNumber);
        swapl(&rep->length);
        swapl(&rep->name);
        swaps(&rep->widthMM);
        swaps(&rep->heightMM);
        swaps(&rep->nProperties);
        swaps(&rep->nColors);
        swaps(&rep->nShapes);
        swaps(&rep->nSections);
        swaps(&rep->nDoodads);
        swaps(&rep->nKeyAliases);
    }
    WriteToClient(client, SIZEOF(xkbGetGeometryReply), rep);
    if (len > 0)
        WriteToClient(client, len, start);
    if (start != NULL)
        free((char *) start);
    if (freeGeom)
        XkbFreeGeometry(geom, XkbGeomAllMask, TRUE);
    return Success;
}

int
ProcXkbGetGeometry(ClientPtr client)
{
    DeviceIntPtr dev;
    xkbGetGeometryReply rep;
    XkbGeometryPtr geom;
    Bool shouldFree;
    Status status;

    REQUEST(xkbGetGeometryReq);
    REQUEST_SIZE_MATCH(xkbGetGeometryReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixGetAttrAccess);
    CHK_ATOM_OR_NONE(stuff->name);

    geom = XkbLookupNamedGeometry(dev, stuff->name, &shouldFree);
    rep = (xkbGetGeometryReply) {
        .type = X_Reply,
        .deviceID = dev->id,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    status = XkbComputeGetGeometryReplySize(geom, &rep, stuff->name);
    if (status != Success)
        return status;
    else
        return XkbSendGeometry(client, geom, &rep, shouldFree);
}

/***====================================================================***/

static Status
_GetCountedString(char **wire_inout, ClientPtr client, char **str)
{
    char *wire, *next;
    CARD16 len;

    wire = *wire_inout;

    if (client->req_len <
        bytes_to_int32(wire + 2 - (char *) client->requestBuffer))
        return BadValue;

    len = *(CARD16 *) wire;
    if (client->swapped) {
        swaps(&len);
    }
    next = wire + XkbPaddedSize(len + 2);
    /* Check we're still within the size of the request */
    if (client->req_len <
        bytes_to_int32(next - (char *) client->requestBuffer))
        return BadValue;
    *str = malloc(len + 1);
    if (!*str)
        return BadAlloc;
    memcpy(*str, &wire[2], len);
    *(*str + len) = '\0';
    *wire_inout = next;
    return Success;
}

static Status
_CheckSetDoodad(char **wire_inout, xkbSetGeometryReq *req,
                XkbGeometryPtr geom, XkbSectionPtr section, ClientPtr client)
{
    char *wire;
    xkbDoodadWireDesc *dWire;
    xkbAnyDoodadWireDesc any;
    xkbTextDoodadWireDesc text;
    XkbDoodadPtr doodad;
    Status status;

    dWire = (xkbDoodadWireDesc *) (*wire_inout);
    if (!_XkbCheckRequestBounds(client, req, dWire, dWire + 1))
        return BadLength;

    any = dWire->any;
    wire = (char *) &dWire[1];
    if (client->swapped) {
        swapl(&any.name);
        swaps(&any.top);
        swaps(&any.left);
        swaps(&any.angle);
    }
    CHK_ATOM_ONLY(dWire->any.name);
    doodad = XkbAddGeomDoodad(geom, section, any.name);
    if (!doodad)
        return BadAlloc;
    doodad->any.type = dWire->any.type;
    doodad->any.priority = dWire->any.priority;
    doodad->any.top = any.top;
    doodad->any.left = any.left;
    doodad->any.angle = any.angle;
    switch (doodad->any.type) {
    case XkbOutlineDoodad:
    case XkbSolidDoodad:
        if (dWire->shape.colorNdx >= geom->num_colors) {
            client->errorValue = _XkbErrCode3(0x40, geom->num_colors,
                                              dWire->shape.colorNdx);
            return BadMatch;
        }
        if (dWire->shape.shapeNdx >= geom->num_shapes) {
            client->errorValue = _XkbErrCode3(0x41, geom->num_shapes,
                                              dWire->shape.shapeNdx);
            return BadMatch;
        }
        doodad->shape.color_ndx = dWire->shape.colorNdx;
        doodad->shape.shape_ndx = dWire->shape.shapeNdx;
        break;
    case XkbTextDoodad:
        if (dWire->text.colorNdx >= geom->num_colors) {
            client->errorValue = _XkbErrCode3(0x42, geom->num_colors,
                                              dWire->text.colorNdx);
            return BadMatch;
        }
        text = dWire->text;
        if (client->swapped) {
            swaps(&text.width);
            swaps(&text.height);
        }
        doodad->text.width = text.width;
        doodad->text.height = text.height;
        doodad->text.color_ndx = dWire->text.colorNdx;
        status = _GetCountedString(&wire, client, &doodad->text.text);
        if (status != Success)
            return status;
        status = _GetCountedString(&wire, client, &doodad->text.font);
        if (status != Success) {
            free (doodad->text.text);
            return status;
        }
        break;
    case XkbIndicatorDoodad:
        if (dWire->indicator.onColorNdx >= geom->num_colors) {
            client->errorValue = _XkbErrCode3(0x43, geom->num_colors,
                                              dWire->indicator.onColorNdx);
            return BadMatch;
        }
        if (dWire->indicator.offColorNdx >= geom->num_colors) {
            client->errorValue = _XkbErrCode3(0x44, geom->num_colors,
                                              dWire->indicator.offColorNdx);
            return BadMatch;
        }
        if (dWire->indicator.shapeNdx >= geom->num_shapes) {
            client->errorValue = _XkbErrCode3(0x45, geom->num_shapes,
                                              dWire->indicator.shapeNdx);
            return BadMatch;
        }
        doodad->indicator.shape_ndx = dWire->indicator.shapeNdx;
        doodad->indicator.on_color_ndx = dWire->indicator.onColorNdx;
        doodad->indicator.off_color_ndx = dWire->indicator.offColorNdx;
        break;
    case XkbLogoDoodad:
        if (dWire->logo.colorNdx >= geom->num_colors) {
            client->errorValue = _XkbErrCode3(0x46, geom->num_colors,
                                              dWire->logo.colorNdx);
            return BadMatch;
        }
        if (dWire->logo.shapeNdx >= geom->num_shapes) {
            client->errorValue = _XkbErrCode3(0x47, geom->num_shapes,
                                              dWire->logo.shapeNdx);
            return BadMatch;
        }
        doodad->logo.color_ndx = dWire->logo.colorNdx;
        doodad->logo.shape_ndx = dWire->logo.shapeNdx;
        status = _GetCountedString(&wire, client, &doodad->logo.logo_name);
        if (status != Success)
            return status;
        break;
    default:
        client->errorValue = _XkbErrCode2(0x4F, dWire->any.type);
        return BadValue;
    }
    *wire_inout = wire;
    return Success;
}

static Status
_CheckSetOverlay(char **wire_inout, xkbSetGeometryReq *req,
                 XkbGeometryPtr geom, XkbSectionPtr section, ClientPtr client)
{
    register int r;
    char *wire;
    XkbOverlayPtr ol;
    xkbOverlayWireDesc *olWire;
    xkbOverlayRowWireDesc *rWire;

    wire = *wire_inout;
    olWire = (xkbOverlayWireDesc *) wire;
    if (!_XkbCheckRequestBounds(client, req, olWire, olWire + 1))
        return BadLength;

    if (client->swapped) {
        swapl(&olWire->name);
    }
    CHK_ATOM_ONLY(olWire->name);
    ol = XkbAddGeomOverlay(section, olWire->name, olWire->nRows);
    rWire = (xkbOverlayRowWireDesc *) &olWire[1];
    for (r = 0; r < olWire->nRows; r++) {
        register int k;
        xkbOverlayKeyWireDesc *kWire;
        XkbOverlayRowPtr row;

        if (!_XkbCheckRequestBounds(client, req, rWire, rWire + 1))
            return BadLength;

        if (rWire->rowUnder > section->num_rows) {
            client->errorValue = _XkbErrCode4(0x20, r, section->num_rows,
                                              rWire->rowUnder);
            return BadMatch;
        }
        row = XkbAddGeomOverlayRow(ol, rWire->rowUnder, rWire->nKeys);
        kWire = (xkbOverlayKeyWireDesc *) &rWire[1];
        for (k = 0; k < rWire->nKeys; k++, kWire++) {
            if (!_XkbCheckRequestBounds(client, req, kWire, kWire + 1))
                return BadLength;

            if (XkbAddGeomOverlayKey(ol, row,
                                     (char *) kWire->over,
                                     (char *) kWire->under) == NULL) {
                client->errorValue = _XkbErrCode3(0x21, r, k);
                return BadMatch;
            }
        }
        rWire = (xkbOverlayRowWireDesc *) kWire;
    }
    olWire = (xkbOverlayWireDesc *) rWire;
    wire = (char *) olWire;
    *wire_inout = wire;
    return Success;
}

static Status
_CheckSetSections(XkbGeometryPtr geom,
                  xkbSetGeometryReq * req, char **wire_inout, ClientPtr client)
{
    Status status;
    register int s;
    char *wire;
    xkbSectionWireDesc *sWire;
    XkbSectionPtr section;

    wire = *wire_inout;
    if (req->nSections < 1)
        return Success;
    sWire = (xkbSectionWireDesc *) wire;
    for (s = 0; s < req->nSections; s++) {
        register int r;
        xkbRowWireDesc *rWire;

        if (!_XkbCheckRequestBounds(client, req, sWire, sWire + 1))
            return BadLength;

        if (client->swapped) {
            swapl(&sWire->name);
            swaps(&sWire->top);
            swaps(&sWire->left);
            swaps(&sWire->width);
            swaps(&sWire->height);
            swaps(&sWire->angle);
        }
        CHK_ATOM_ONLY(sWire->name);
        section = XkbAddGeomSection(geom, sWire->name, sWire->nRows,
                                    sWire->nDoodads, sWire->nOverlays);
        if (!section)
            return BadAlloc;
        section->priority = sWire->priority;
        section->top = sWire->top;
        section->left = sWire->left;
        section->width = sWire->width;
        section->height = sWire->height;
        section->angle = sWire->angle;
        rWire = (xkbRowWireDesc *) &sWire[1];
        for (r = 0; r < sWire->nRows; r++) {
            register int k;
            XkbRowPtr row;
            xkbKeyWireDesc *kWire;

            if (!_XkbCheckRequestBounds(client, req, rWire, rWire + 1))
                return BadLength;

            if (client->swapped) {
                swaps(&rWire->top);
                swaps(&rWire->left);
            }
            row = XkbAddGeomRow(section, rWire->nKeys);
            if (!row)
                return BadAlloc;
            row->top = rWire->top;
            row->left = rWire->left;
            row->vertical = rWire->vertical;
            kWire = (xkbKeyWireDesc *) &rWire[1];
            for (k = 0; k < rWire->nKeys; k++, kWire++) {
                XkbKeyPtr key;

                if (!_XkbCheckRequestBounds(client, req, kWire, kWire + 1))
                    return BadLength;

                key = XkbAddGeomKey(row);
                if (!key)
                    return BadAlloc;
                memcpy(key->name.name, kWire->name, XkbKeyNameLength);
                key->gap = kWire->gap;
                key->shape_ndx = kWire->shapeNdx;
                key->color_ndx = kWire->colorNdx;
                if (key->shape_ndx >= geom->num_shapes) {
                    client->errorValue = _XkbErrCode3(0x10, key->shape_ndx,
                                                      geom->num_shapes);
                    return BadMatch;
                }
                if (key->color_ndx >= geom->num_colors) {
                    client->errorValue = _XkbErrCode3(0x11, key->color_ndx,
                                                      geom->num_colors);
                    return BadMatch;
                }
            }
            rWire = (xkbRowWireDesc *)kWire;
        }
        wire = (char *) rWire;
        if (sWire->nDoodads > 0) {
            register int d;

            for (d = 0; d < sWire->nDoodads; d++) {
                status = _CheckSetDoodad(&wire, req, geom, section, client);
                if (status != Success)
                    return status;
            }
        }
        if (sWire->nOverlays > 0) {
            register int o;

            for (o = 0; o < sWire->nOverlays; o++) {
                status = _CheckSetOverlay(&wire, req, geom, section, client);
                if (status != Success)
                    return status;
            }
        }
        sWire = (xkbSectionWireDesc *) wire;
    }
    wire = (char *) sWire;
    *wire_inout = wire;
    return Success;
}

static Status
_CheckSetShapes(XkbGeometryPtr geom,
                xkbSetGeometryReq * req, char **wire_inout, ClientPtr client)
{
    register int i;
    char *wire;

    wire = *wire_inout;
    if (req->nShapes < 1) {
        client->errorValue = _XkbErrCode2(0x06, req->nShapes);
        return BadValue;
    }
    else {
        xkbShapeWireDesc *shapeWire;
        XkbShapePtr shape;
        register int o;

        shapeWire = (xkbShapeWireDesc *) wire;
        for (i = 0; i < req->nShapes; i++) {
            xkbOutlineWireDesc *olWire;
            XkbOutlinePtr ol;

            if (!_XkbCheckRequestBounds(client, req, shapeWire, shapeWire + 1))
                return BadLength;

            shape =
                XkbAddGeomShape(geom, shapeWire->name, shapeWire->nOutlines);
            if (!shape)
                return BadAlloc;
            olWire = (xkbOutlineWireDesc *) (&shapeWire[1]);
            for (o = 0; o < shapeWire->nOutlines; o++) {
                register int p;
                XkbPointPtr pt;
                xkbPointWireDesc *ptWire;

                if (!_XkbCheckRequestBounds(client, req, olWire, olWire + 1))
                    return BadLength;

                ol = XkbAddGeomOutline(shape, olWire->nPoints);
                if (!ol)
                    return BadAlloc;
                ol->corner_radius = olWire->cornerRadius;
                ptWire = (xkbPointWireDesc *) &olWire[1];
                for (p = 0, pt = ol->points; p < olWire->nPoints; p++, pt++, ptWire++) {
                    if (!_XkbCheckRequestBounds(client, req, ptWire, ptWire + 1))
                        return BadLength;

                    pt->x = ptWire->x;
                    pt->y = ptWire->y;
                    if (client->swapped) {
                        swaps(&pt->x);
                        swaps(&pt->y);
                    }
                }
                ol->num_points = olWire->nPoints;
                olWire = (xkbOutlineWireDesc *)ptWire;
            }
            if (shapeWire->primaryNdx != XkbNoShape)
                shape->primary = &shape->outlines[shapeWire->primaryNdx];
            if (shapeWire->approxNdx != XkbNoShape)
                shape->approx = &shape->outlines[shapeWire->approxNdx];
            shapeWire = (xkbShapeWireDesc *) olWire;
        }
        wire = (char *) shapeWire;
    }
    if (geom->num_shapes != req->nShapes) {
        client->errorValue = _XkbErrCode3(0x07, geom->num_shapes, req->nShapes);
        return BadMatch;
    }

    *wire_inout = wire;
    return Success;
}

static Status
_CheckSetGeom(XkbGeometryPtr geom, xkbSetGeometryReq * req, ClientPtr client)
{
    register int i;
    Status status;
    char *wire;

    wire = (char *) &req[1];
    status = _GetCountedString(&wire, client, &geom->label_font);
    if (status != Success)
        return status;

    for (i = 0; i < req->nProperties; i++) {
        char *name, *val;

        status = _GetCountedString(&wire, client, &name);
        if (status != Success)
            return status;
        status = _GetCountedString(&wire, client, &val);
        if (status != Success) {
            free(name);
            return status;
        }
        if (XkbAddGeomProperty(geom, name, val) == NULL) {
            free(name);
            free(val);
            return BadAlloc;
        }
        free(name);
        free(val);
    }

    if (req->nColors < 2) {
        client->errorValue = _XkbErrCode3(0x01, 2, req->nColors);
        return BadValue;
    }
    if (req->baseColorNdx > req->nColors) {
        client->errorValue =
            _XkbErrCode3(0x03, req->nColors, req->baseColorNdx);
        return BadMatch;
    }
    if (req->labelColorNdx > req->nColors) {
        client->errorValue =
            _XkbErrCode3(0x03, req->nColors, req->labelColorNdx);
        return BadMatch;
    }
    if (req->labelColorNdx == req->baseColorNdx) {
        client->errorValue = _XkbErrCode3(0x04, req->baseColorNdx,
                                          req->labelColorNdx);
        return BadMatch;
    }

    for (i = 0; i < req->nColors; i++) {
        char *name;

        status = _GetCountedString(&wire, client, &name);
        if (status != Success)
            return status;
        if (!XkbAddGeomColor(geom, name, geom->num_colors)) {
            free(name);
            return BadAlloc;
        }
        free(name);
    }
    if (req->nColors != geom->num_colors) {
        client->errorValue = _XkbErrCode3(0x05, req->nColors, geom->num_colors);
        return BadMatch;
    }
    geom->label_color = &geom->colors[req->labelColorNdx];
    geom->base_color = &geom->colors[req->baseColorNdx];

    if ((status = _CheckSetShapes(geom, req, &wire, client)) != Success)
        return status;

    if ((status = _CheckSetSections(geom, req, &wire, client)) != Success)
        return status;

    for (i = 0; i < req->nDoodads; i++) {
        status = _CheckSetDoodad(&wire, req, geom, NULL, client);
        if (status != Success)
            return status;
    }

    for (i = 0; i < req->nKeyAliases; i++) {
        if (!_XkbCheckRequestBounds(client, req, wire, wire + XkbKeyNameLength))
                return BadLength;

        if (XkbAddGeomKeyAlias(geom, &wire[XkbKeyNameLength], wire) == NULL)
            return BadAlloc;
        wire += 2 * XkbKeyNameLength;
    }
    return Success;
}

static int
_XkbSetGeometry(ClientPtr client, DeviceIntPtr dev, xkbSetGeometryReq * stuff)
{
    XkbDescPtr xkb;
    Bool new_name;
    xkbNewKeyboardNotify nkn;
    XkbGeometryPtr geom, old;
    XkbGeometrySizesRec sizes;
    Status status;

    xkb = dev->key->xkbInfo->desc;
    old = xkb->geom;
    xkb->geom = NULL;

    sizes.which = XkbGeomAllMask;
    sizes.num_properties = stuff->nProperties;
    sizes.num_colors = stuff->nColors;
    sizes.num_shapes = stuff->nShapes;
    sizes.num_sections = stuff->nSections;
    sizes.num_doodads = stuff->nDoodads;
    sizes.num_key_aliases = stuff->nKeyAliases;
    if ((status = XkbAllocGeometry(xkb, &sizes)) != Success) {
        xkb->geom = old;
        return status;
    }
    geom = xkb->geom;
    geom->name = stuff->name;
    geom->width_mm = stuff->widthMM;
    geom->height_mm = stuff->heightMM;
    if ((status = _CheckSetGeom(geom, stuff, client)) != Success) {
        XkbFreeGeometry(geom, XkbGeomAllMask, TRUE);
        xkb->geom = old;
        return status;
    }
    new_name = (xkb->names->geometry != geom->name);
    xkb->names->geometry = geom->name;
    if (old)
        XkbFreeGeometry(old, XkbGeomAllMask, TRUE);
    if (new_name) {
        xkbNamesNotify nn;

        memset(&nn, 0, sizeof(xkbNamesNotify));
        nn.changed = XkbGeometryNameMask;
        XkbSendNamesNotify(dev, &nn);
    }
    nkn.deviceID = nkn.oldDeviceID = dev->id;
    nkn.minKeyCode = nkn.oldMinKeyCode = xkb->min_key_code;
    nkn.maxKeyCode = nkn.oldMaxKeyCode = xkb->max_key_code;
    nkn.requestMajor = XkbReqCode;
    nkn.requestMinor = X_kbSetGeometry;
    nkn.changed = XkbNKN_GeometryMask;
    XkbSendNewKeyboardNotify(dev, &nkn);
    return Success;
}

int
ProcXkbSetGeometry(ClientPtr client)
{
    DeviceIntPtr dev;
    int rc;

    REQUEST(xkbSetGeometryReq);
    REQUEST_AT_LEAST_SIZE(xkbSetGeometryReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixManageAccess);
    CHK_ATOM_OR_NONE(stuff->name);

    rc = _XkbSetGeometry(client, dev, stuff);
    if (rc != Success)
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if ((other != dev) && other->key && !IsMaster(other) &&
                GetMaster(other, MASTER_KEYBOARD) == dev) {
                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success)
                    _XkbSetGeometry(client, other, stuff);
            }
        }
    }

    return Success;
}

/***====================================================================***/

int
ProcXkbPerClientFlags(ClientPtr client)
{
    DeviceIntPtr dev;
    xkbPerClientFlagsReply rep;
    XkbInterestPtr interest;
    Mask access_mode = DixGetAttrAccess | DixSetAttrAccess;

    REQUEST(xkbPerClientFlagsReq);
    REQUEST_SIZE_MATCH(xkbPerClientFlagsReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, access_mode);
    CHK_MASK_LEGAL(0x01, stuff->change, XkbPCF_AllFlagsMask);
    CHK_MASK_MATCH(0x02, stuff->change, stuff->value);

    interest = XkbFindClientResource((DevicePtr) dev, client);
    if (stuff->change) {
        client->xkbClientFlags &= ~stuff->change;
        client->xkbClientFlags |= stuff->value;
    }
    if (stuff->change & XkbPCF_AutoResetControlsMask) {
        Bool want;

        want = stuff->value & XkbPCF_AutoResetControlsMask;
        if (interest && !want) {
            interest->autoCtrls = interest->autoCtrlValues = 0;
        }
        else if (want && (!interest)) {
            XID id = FakeClientID(client->index);

            if (!AddResource(id, RT_XKBCLIENT, dev))
                return BadAlloc;
            interest = XkbAddClientResource((DevicePtr) dev, client, id);
            if (!interest)
                return BadAlloc;
        }
        if (interest && want) {
            register unsigned affect;

            affect = stuff->ctrlsToChange;

            CHK_MASK_LEGAL(0x03, affect, XkbAllBooleanCtrlsMask);
            CHK_MASK_MATCH(0x04, affect, stuff->autoCtrls);
            CHK_MASK_MATCH(0x05, stuff->autoCtrls, stuff->autoCtrlValues);

            interest->autoCtrls &= ~affect;
            interest->autoCtrlValues &= ~affect;
            interest->autoCtrls |= stuff->autoCtrls & affect;
            interest->autoCtrlValues |= stuff->autoCtrlValues & affect;
        }
    }

    rep = (xkbPerClientFlagsReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .supported = XkbPCF_AllFlagsMask,
        .value = client->xkbClientFlags & XkbPCF_AllFlagsMask,
        .autoCtrls = interest ? interest->autoCtrls : 0,
        .autoCtrlValues =  interest ? interest->autoCtrlValues : 0,
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.supported);
        swapl(&rep.value);
        swapl(&rep.autoCtrls);
        swapl(&rep.autoCtrlValues);
    }
    WriteToClient(client, SIZEOF(xkbPerClientFlagsReply), &rep);
    return Success;
}

/***====================================================================***/

/* all latin-1 alphanumerics, plus parens, minus, underscore, slash */
/* and wildcards */
static unsigned char componentSpecLegal[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xa7, 0xff, 0x87,
    0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
};

/* same as above but accepts percent, plus and bar too */
static unsigned char componentExprLegal[] = {
    0x00, 0x00, 0x00, 0x00, 0x20, 0xaf, 0xff, 0x87,
    0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x17,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
};

static char *
GetComponentSpec(unsigned char **pWire, Bool allowExpr, int *errRtrn)
{
    int len;
    register int i;
    unsigned char *wire, *str, *tmp, *legal;

    if (allowExpr)
        legal = &componentExprLegal[0];
    else
        legal = &componentSpecLegal[0];

    wire = *pWire;
    len = (*(unsigned char *) wire++);
    if (len > 0) {
        str = calloc(1, len + 1);
        if (str) {
            tmp = str;
            for (i = 0; i < len; i++) {
                if (legal[(*wire) / 8] & (1 << ((*wire) % 8)))
                    *tmp++ = *wire++;
                else
                    wire++;
            }
            if (tmp != str)
                *tmp++ = '\0';
            else {
                free(str);
                str = NULL;
            }
        }
        else {
            *errRtrn = BadAlloc;
        }
    }
    else {
        str = NULL;
    }
    *pWire = wire;
    return (char *) str;
}

/***====================================================================***/

int
ProcXkbListComponents(ClientPtr client)
{
    DeviceIntPtr dev;
    xkbListComponentsReply rep;
    unsigned len;
    unsigned char *str;
    uint8_t size;
    int i;

    REQUEST(xkbListComponentsReq);
    REQUEST_AT_LEAST_SIZE(xkbListComponentsReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, DixGetAttrAccess);

    /* The request is followed by six Pascal strings (i.e. size in characters
     * followed by a string pattern) describing what the client wants us to
     * list.  We don't care, but might as well check they haven't got the
     * length wrong. */
    str = (unsigned char *) &stuff[1];
    for (i = 0; i < 6; i++) {
        size = *((uint8_t *)str);
        len = (str + size + 1) - ((unsigned char *) stuff);
        if ((XkbPaddedSize(len) / 4) > stuff->length)
            return BadLength;
        str += (size + 1);
    }
    if ((XkbPaddedSize(len) / 4) != stuff->length)
        return BadLength;
    rep = (xkbListComponentsReply) {
        .type = X_Reply,
        .deviceID = dev->id,
        .sequenceNumber = client->sequence,
        .length = 0,
        .nKeymaps = 0,
        .nKeycodes = 0,
        .nTypes = 0,
        .nCompatMaps = 0,
        .nSymbols = 0,
        .nGeometries = 0,
        .extra = 0
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.nKeymaps);
        swaps(&rep.nKeycodes);
        swaps(&rep.nTypes);
        swaps(&rep.nCompatMaps);
        swaps(&rep.nSymbols);
        swaps(&rep.nGeometries);
        swaps(&rep.extra);
    }
    WriteToClient(client, SIZEOF(xkbListComponentsReply), &rep);
    return Success;
}

/***====================================================================***/
int
ProcXkbGetKbdByName(ClientPtr client)
{
    DeviceIntPtr dev;
    DeviceIntPtr tmpd;
    DeviceIntPtr master;
    xkbGetKbdByNameReply rep = { 0 };
    xkbGetMapReply mrep = { 0 };
    xkbGetCompatMapReply crep = { 0 };
    xkbGetIndicatorMapReply irep = { 0 };
    xkbGetNamesReply nrep = { 0 };
    xkbGetGeometryReply grep = { 0 };
    XkbComponentNamesRec names = { 0 };
    XkbDescPtr xkb, new;
    XkbEventCauseRec cause;
    unsigned char *str;
    char mapFile[PATH_MAX];
    unsigned len;
    unsigned fwant, fneed, reported;
    int status;
    Bool geom_changed;
    XkbSrvLedInfoPtr old_sli;
    XkbSrvLedInfoPtr sli;
    Mask access_mode = DixGetAttrAccess | DixManageAccess;

    REQUEST(xkbGetKbdByNameReq);
    REQUEST_AT_LEAST_SIZE(xkbGetKbdByNameReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_KBD_DEVICE(dev, stuff->deviceSpec, client, access_mode);
    master = GetMaster(dev, MASTER_KEYBOARD);

    xkb = dev->key->xkbInfo->desc;
    status = Success;
    str = (unsigned char *) &stuff[1];
    {
        char *keymap = GetComponentSpec(&str, TRUE, &status);  /* keymap, unsupported */
        if (keymap) {
            free(keymap);
            return BadMatch;
        }
    }
    names.keycodes = GetComponentSpec(&str, TRUE, &status);
    names.types = GetComponentSpec(&str, TRUE, &status);
    names.compat = GetComponentSpec(&str, TRUE, &status);
    names.symbols = GetComponentSpec(&str, TRUE, &status);
    names.geometry = GetComponentSpec(&str, TRUE, &status);
    if (status == Success) {
        len = str - ((unsigned char *) stuff);
        if ((XkbPaddedSize(len) / 4) != stuff->length)
            status = BadLength;
    }

    if (status != Success) {
        free(names.keycodes);
        free(names.types);
        free(names.compat);
        free(names.symbols);
        free(names.geometry);
        return status;
    }

    CHK_MASK_LEGAL(0x01, stuff->want, XkbGBN_AllComponentsMask);
    CHK_MASK_LEGAL(0x02, stuff->need, XkbGBN_AllComponentsMask);

    if (stuff->load)
        fwant = XkbGBN_AllComponentsMask;
    else
        fwant = stuff->want | stuff->need;
    if ((!names.compat) &&
        (fwant & (XkbGBN_CompatMapMask | XkbGBN_IndicatorMapMask))) {
        names.compat = Xstrdup("%");
    }
    if ((!names.types) && (fwant & (XkbGBN_TypesMask))) {
        names.types = Xstrdup("%");
    }
    if ((!names.symbols) && (fwant & XkbGBN_SymbolsMask)) {
        names.symbols = Xstrdup("%");
    }
    geom_changed = ((names.geometry != NULL) &&
                    (strcmp(names.geometry, "%") != 0));
    if ((!names.geometry) && (fwant & XkbGBN_GeometryMask)) {
        names.geometry = Xstrdup("%");
        geom_changed = FALSE;
    }

    memset(mapFile, 0, PATH_MAX);
    rep.type = X_Reply;
    rep.deviceID = dev->id;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.minKeyCode = xkb->min_key_code;
    rep.maxKeyCode = xkb->max_key_code;
    rep.loaded = FALSE;
    fwant =
        XkbConvertGetByNameComponents(TRUE, stuff->want) | XkmVirtualModsMask;
    fneed = XkbConvertGetByNameComponents(TRUE, stuff->need);
    rep.reported = XkbConvertGetByNameComponents(FALSE, fwant | fneed);
    if (stuff->load) {
        fneed |= XkmKeymapRequired;
        fwant |= XkmKeymapLegal;
    }
    if ((fwant | fneed) & XkmSymbolsMask) {
        fneed |= XkmKeyNamesIndex | XkmTypesIndex;
        fwant |= XkmIndicatorsIndex;
    }

    /* We pass dev in here so we can get the old names out if needed. */
    rep.found = XkbDDXLoadKeymapByNames(dev, &names, fwant, fneed, &new,
                                        mapFile, PATH_MAX);
    rep.newKeyboard = FALSE;
    rep.pad1 = rep.pad2 = rep.pad3 = rep.pad4 = 0;

    stuff->want |= stuff->need;
    if (new == NULL)
        rep.reported = 0;
    else {
        if (stuff->load)
            rep.loaded = TRUE;
        if (stuff->load ||
            ((rep.reported & XkbGBN_SymbolsMask) && (new->compat))) {
            XkbChangesRec changes;

            memset(&changes, 0, sizeof(changes));
            XkbUpdateDescActions(new,
                                 new->min_key_code, XkbNumKeys(new), &changes);
        }

        if (new->map == NULL)
            rep.reported &= ~(XkbGBN_SymbolsMask | XkbGBN_TypesMask);
        else if (rep.reported & (XkbGBN_SymbolsMask | XkbGBN_TypesMask)) {
            mrep.type = X_Reply;
            mrep.deviceID = dev->id;
            mrep.sequenceNumber = client->sequence;
            mrep.length =
                ((SIZEOF(xkbGetMapReply) - SIZEOF(xGenericReply)) >> 2);
            mrep.minKeyCode = new->min_key_code;
            mrep.maxKeyCode = new->max_key_code;
            mrep.present = 0;
            mrep.totalSyms = mrep.totalActs =
                mrep.totalKeyBehaviors = mrep.totalKeyExplicit =
                mrep.totalModMapKeys = mrep.totalVModMapKeys = 0;
            if (rep.reported & (XkbGBN_TypesMask | XkbGBN_ClientSymbolsMask)) {
                mrep.present |= XkbKeyTypesMask;
                mrep.firstType = 0;
                mrep.nTypes = mrep.totalTypes = new->map->num_types;
            }
            else {
                mrep.firstType = mrep.nTypes = 0;
                mrep.totalTypes = 0;
            }
            if (rep.reported & XkbGBN_ClientSymbolsMask) {
                mrep.present |= (XkbKeySymsMask | XkbModifierMapMask);
                mrep.firstKeySym = mrep.firstModMapKey = new->min_key_code;
                mrep.nKeySyms = mrep.nModMapKeys = XkbNumKeys(new);
            }
            else {
                mrep.firstKeySym = mrep.firstModMapKey = 0;
                mrep.nKeySyms = mrep.nModMapKeys = 0;
            }
            if (rep.reported & XkbGBN_ServerSymbolsMask) {
                mrep.present |= XkbAllServerInfoMask;
                mrep.virtualMods = ~0;
                mrep.firstKeyAct = mrep.firstKeyBehavior =
                    mrep.firstKeyExplicit = new->min_key_code;
                mrep.nKeyActs = mrep.nKeyBehaviors =
                    mrep.nKeyExplicit = XkbNumKeys(new);
                mrep.firstVModMapKey = new->min_key_code;
                mrep.nVModMapKeys = XkbNumKeys(new);
            }
            else {
                mrep.virtualMods = 0;
                mrep.firstKeyAct = mrep.firstKeyBehavior =
                    mrep.firstKeyExplicit = 0;
                mrep.nKeyActs = mrep.nKeyBehaviors = mrep.nKeyExplicit = 0;
            }
            XkbComputeGetMapReplySize(new, &mrep);
            rep.length += SIZEOF(xGenericReply) / 4 + mrep.length;
        }
        if (new->compat == NULL)
            rep.reported &= ~XkbGBN_CompatMapMask;
        else if (rep.reported & XkbGBN_CompatMapMask) {
            crep.type = X_Reply;
            crep.deviceID = dev->id;
            crep.sequenceNumber = client->sequence;
            crep.length = 0;
            crep.groups = XkbAllGroupsMask;
            crep.firstSI = 0;
            crep.nSI = crep.nTotalSI = new->compat->num_si;
            XkbComputeGetCompatMapReplySize(new->compat, &crep);
            rep.length += SIZEOF(xGenericReply) / 4 + crep.length;
        }
        if (new->indicators == NULL)
            rep.reported &= ~XkbGBN_IndicatorMapMask;
        else if (rep.reported & XkbGBN_IndicatorMapMask) {
            irep.type = X_Reply;
            irep.deviceID = dev->id;
            irep.sequenceNumber = client->sequence;
            irep.length = 0;
            irep.which = XkbAllIndicatorsMask;
            XkbComputeGetIndicatorMapReplySize(new->indicators, &irep);
            rep.length += SIZEOF(xGenericReply) / 4 + irep.length;
        }
        if (new->names == NULL)
            rep.reported &= ~(XkbGBN_OtherNamesMask | XkbGBN_KeyNamesMask);
        else if (rep.reported & (XkbGBN_OtherNamesMask | XkbGBN_KeyNamesMask)) {
            nrep.type = X_Reply;
            nrep.deviceID = dev->id;
            nrep.sequenceNumber = client->sequence;
            nrep.length = 0;
            nrep.minKeyCode = new->min_key_code;
            nrep.maxKeyCode = new->max_key_code;
            if (rep.reported & XkbGBN_OtherNamesMask) {
                nrep.which = XkbAllNamesMask;
                if (new->map != NULL)
                    nrep.nTypes = new->map->num_types;
                else
                    nrep.nTypes = 0;
                nrep.nKTLevels = 0;
                nrep.groupNames = XkbAllGroupsMask;
                nrep.virtualMods = XkbAllVirtualModsMask;
                nrep.indicators = XkbAllIndicatorsMask;
                nrep.nRadioGroups = new->names->num_rg;
            }
            else {
                nrep.which = 0;
                nrep.nTypes = 0;
                nrep.nKTLevels = 0;
                nrep.groupNames = 0;
                nrep.virtualMods = 0;
                nrep.indicators = 0;
                nrep.nRadioGroups = 0;
            }
            if (rep.reported & XkbGBN_KeyNamesMask) {
                nrep.which |= XkbKeyNamesMask;
                nrep.firstKey = new->min_key_code;
                nrep.nKeys = XkbNumKeys(new);
                nrep.nKeyAliases = new->names->num_key_aliases;
                if (nrep.nKeyAliases)
                    nrep.which |= XkbKeyAliasesMask;
            }
            else {
                nrep.which &= ~(XkbKeyNamesMask | XkbKeyAliasesMask);
                nrep.firstKey = nrep.nKeys = 0;
                nrep.nKeyAliases = 0;
            }
            XkbComputeGetNamesReplySize(new, &nrep);
            rep.length += SIZEOF(xGenericReply) / 4 + nrep.length;
        }
        if (new->geom == NULL)
            rep.reported &= ~XkbGBN_GeometryMask;
        else if (rep.reported & XkbGBN_GeometryMask) {
            grep.type = X_Reply;
            grep.deviceID = dev->id;
            grep.sequenceNumber = client->sequence;
            grep.length = 0;
            grep.found = TRUE;
            grep.pad = 0;
            grep.widthMM = grep.heightMM = 0;
            grep.nProperties = grep.nColors = grep.nShapes = 0;
            grep.nSections = grep.nDoodads = 0;
            grep.baseColorNdx = grep.labelColorNdx = 0;
            XkbComputeGetGeometryReplySize(new->geom, &grep, None);
            rep.length += SIZEOF(xGenericReply) / 4 + grep.length;
        }
    }

    reported = rep.reported;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.found);
        swaps(&rep.reported);
    }
    WriteToClient(client, SIZEOF(xkbGetKbdByNameReply), &rep);
    if (reported & (XkbGBN_SymbolsMask | XkbGBN_TypesMask))
        XkbSendMap(client, new, &mrep);
    if (reported & XkbGBN_CompatMapMask)
        XkbSendCompatMap(client, new->compat, &crep);
    if (reported & XkbGBN_IndicatorMapMask)
        XkbSendIndicatorMap(client, new->indicators, &irep);
    if (reported & (XkbGBN_KeyNamesMask | XkbGBN_OtherNamesMask))
        XkbSendNames(client, new, &nrep);
    if (reported & XkbGBN_GeometryMask)
        XkbSendGeometry(client, new->geom, &grep, FALSE);
    if (rep.loaded) {
        XkbDescPtr old_xkb;
        xkbNewKeyboardNotify nkn;

        old_xkb = xkb;
        xkb = new;
        dev->key->xkbInfo->desc = xkb;
        new = old_xkb;          /* so it'll get freed automatically */

        XkbCopyControls(xkb, old_xkb);

        nkn.deviceID = nkn.oldDeviceID = dev->id;
        nkn.minKeyCode = new->min_key_code;
        nkn.maxKeyCode = new->max_key_code;
        nkn.oldMinKeyCode = xkb->min_key_code;
        nkn.oldMaxKeyCode = xkb->max_key_code;
        nkn.requestMajor = XkbReqCode;
        nkn.requestMinor = X_kbGetKbdByName;
        nkn.changed = XkbNKN_KeycodesMask;
        if (geom_changed)
            nkn.changed |= XkbNKN_GeometryMask;
        XkbSendNewKeyboardNotify(dev, &nkn);

        /* Update the map and LED info on the device itself, as well as
         * any slaves if it's an MD, or its MD if it's an SD and was the
         * last device used on that MD. */
        for (tmpd = inputInfo.devices; tmpd; tmpd = tmpd->next) {
            if (tmpd != dev && GetMaster(tmpd, MASTER_KEYBOARD) != dev &&
                (tmpd != master || dev != master->lastSlave))
                continue;

            if (tmpd != dev)
                XkbDeviceApplyKeymap(tmpd, xkb);

            if (tmpd->kbdfeed && tmpd->kbdfeed->xkb_sli) {
                old_sli = tmpd->kbdfeed->xkb_sli;
                tmpd->kbdfeed->xkb_sli = NULL;
                sli = XkbAllocSrvLedInfo(tmpd, tmpd->kbdfeed, NULL, 0);
                if (sli) {
                    sli->explicitState = old_sli->explicitState;
                    sli->effectiveState = old_sli->effectiveState;
                }
                tmpd->kbdfeed->xkb_sli = sli;
                XkbFreeSrvLedInfo(old_sli);
            }
        }
    }
    if ((new != NULL) && (new != xkb)) {
        XkbFreeKeyboard(new, XkbAllComponentsMask, TRUE);
        new = NULL;
    }
    XkbFreeComponentNames(&names, FALSE);
    XkbSetCauseXkbReq(&cause, X_kbGetKbdByName, client);
    XkbUpdateAllDeviceIndicators(NULL, &cause);

    return Success;
}

/***====================================================================***/

static int
ComputeDeviceLedInfoSize(DeviceIntPtr dev,
                         unsigned int what, XkbSrvLedInfoPtr sli)
{
    int nNames, nMaps;
    register unsigned n, bit;

    if (sli == NULL)
        return 0;
    nNames = nMaps = 0;
    if ((what & XkbXI_IndicatorNamesMask) == 0)
        sli->namesPresent = 0;
    if ((what & XkbXI_IndicatorMapsMask) == 0)
        sli->mapsPresent = 0;

    for (n = 0, bit = 1; n < XkbNumIndicators; n++, bit <<= 1) {
        if (sli->names && sli->names[n] != None) {
            sli->namesPresent |= bit;
            nNames++;
        }
        if (sli->maps && XkbIM_InUse(&sli->maps[n])) {
            sli->mapsPresent |= bit;
            nMaps++;
        }
    }
    return (nNames * 4) + (nMaps * SIZEOF(xkbIndicatorMapWireDesc));
}

static int
CheckDeviceLedFBs(DeviceIntPtr dev,
                  int class,
                  int id, xkbGetDeviceInfoReply * rep, ClientPtr client)
{
    int nFBs = 0;
    int length = 0;
    Bool classOk;

    if (class == XkbDfltXIClass) {
        if (dev->kbdfeed)
            class = KbdFeedbackClass;
        else if (dev->leds)
            class = LedFeedbackClass;
        else {
            client->errorValue = _XkbErrCode2(XkbErr_BadClass, class);
            return XkbKeyboardErrorCode;
        }
    }
    classOk = FALSE;
    if ((dev->kbdfeed) &&
        ((class == KbdFeedbackClass) || (class == XkbAllXIClasses))) {
        KbdFeedbackPtr kf;

        classOk = TRUE;
        for (kf = dev->kbdfeed; (kf); kf = kf->next) {
            if ((id != XkbAllXIIds) && (id != XkbDfltXIId) &&
                (id != kf->ctrl.id))
                continue;
            nFBs++;
            length += SIZEOF(xkbDeviceLedsWireDesc);
            if (!kf->xkb_sli)
                kf->xkb_sli = XkbAllocSrvLedInfo(dev, kf, NULL, 0);
            length += ComputeDeviceLedInfoSize(dev, rep->present, kf->xkb_sli);
            if (id != XkbAllXIIds)
                break;
        }
    }
    if ((dev->leds) &&
        ((class == LedFeedbackClass) || (class == XkbAllXIClasses))) {
        LedFeedbackPtr lf;

        classOk = TRUE;
        for (lf = dev->leds; (lf); lf = lf->next) {
            if ((id != XkbAllXIIds) && (id != XkbDfltXIId) &&
                (id != lf->ctrl.id))
                continue;
            nFBs++;
            length += SIZEOF(xkbDeviceLedsWireDesc);
            if (!lf->xkb_sli)
                lf->xkb_sli = XkbAllocSrvLedInfo(dev, NULL, lf, 0);
            length += ComputeDeviceLedInfoSize(dev, rep->present, lf->xkb_sli);
            if (id != XkbAllXIIds)
                break;
        }
    }
    if (nFBs > 0) {
        rep->nDeviceLedFBs = nFBs;
        rep->length += (length / 4);
        return Success;
    }
    if (classOk)
        client->errorValue = _XkbErrCode2(XkbErr_BadId, id);
    else
        client->errorValue = _XkbErrCode2(XkbErr_BadClass, class);
    return XkbKeyboardErrorCode;
}

static int
SendDeviceLedInfo(XkbSrvLedInfoPtr sli, ClientPtr client)
{
    xkbDeviceLedsWireDesc wire;
    int length;

    length = 0;
    wire.ledClass = sli->class;
    wire.ledID = sli->id;
    wire.namesPresent = sli->namesPresent;
    wire.mapsPresent = sli->mapsPresent;
    wire.physIndicators = sli->physIndicators;
    wire.state = sli->effectiveState;
    if (client->swapped) {
        swaps(&wire.ledClass);
        swaps(&wire.ledID);
        swapl(&wire.namesPresent);
        swapl(&wire.mapsPresent);
        swapl(&wire.physIndicators);
        swapl(&wire.state);
    }
    WriteToClient(client, SIZEOF(xkbDeviceLedsWireDesc), &wire);
    length += SIZEOF(xkbDeviceLedsWireDesc);
    if (sli->namesPresent | sli->mapsPresent) {
        register unsigned i, bit;

        if (sli->namesPresent) {
            CARD32 awire;

            for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
                if (sli->namesPresent & bit) {
                    awire = (CARD32) sli->names[i];
                    if (client->swapped) {
                        swapl(&awire);
                    }
                    WriteToClient(client, 4, &awire);
                    length += 4;
                }
            }
        }
        if (sli->mapsPresent) {
            for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
                xkbIndicatorMapWireDesc iwire;

                if (sli->mapsPresent & bit) {
                    iwire.flags = sli->maps[i].flags;
                    iwire.whichGroups = sli->maps[i].which_groups;
                    iwire.groups = sli->maps[i].groups;
                    iwire.whichMods = sli->maps[i].which_mods;
                    iwire.mods = sli->maps[i].mods.mask;
                    iwire.realMods = sli->maps[i].mods.real_mods;
                    iwire.virtualMods = sli->maps[i].mods.vmods;
                    iwire.ctrls = sli->maps[i].ctrls;
                    if (client->swapped) {
                        swaps(&iwire.virtualMods);
                        swapl(&iwire.ctrls);
                    }
                    WriteToClient(client, SIZEOF(xkbIndicatorMapWireDesc),
                                  &iwire);
                    length += SIZEOF(xkbIndicatorMapWireDesc);
                }
            }
        }
    }
    return length;
}

static int
SendDeviceLedFBs(DeviceIntPtr dev,
                 int class, int id, unsigned wantLength, ClientPtr client)
{
    int length = 0;

    if (class == XkbDfltXIClass) {
        if (dev->kbdfeed)
            class = KbdFeedbackClass;
        else if (dev->leds)
            class = LedFeedbackClass;
    }
    if ((dev->kbdfeed) &&
        ((class == KbdFeedbackClass) || (class == XkbAllXIClasses))) {
        KbdFeedbackPtr kf;

        for (kf = dev->kbdfeed; (kf); kf = kf->next) {
            if ((id == XkbAllXIIds) || (id == XkbDfltXIId) ||
                (id == kf->ctrl.id)) {
                length += SendDeviceLedInfo(kf->xkb_sli, client);
                if (id != XkbAllXIIds)
                    break;
            }
        }
    }
    if ((dev->leds) &&
        ((class == LedFeedbackClass) || (class == XkbAllXIClasses))) {
        LedFeedbackPtr lf;

        for (lf = dev->leds; (lf); lf = lf->next) {
            if ((id == XkbAllXIIds) || (id == XkbDfltXIId) ||
                (id == lf->ctrl.id)) {
                length += SendDeviceLedInfo(lf->xkb_sli, client);
                if (id != XkbAllXIIds)
                    break;
            }
        }
    }
    if (length == wantLength)
        return Success;
    else
        return BadLength;
}

int
ProcXkbGetDeviceInfo(ClientPtr client)
{
    DeviceIntPtr dev;
    xkbGetDeviceInfoReply rep;
    int status, nDeviceLedFBs;
    unsigned length, nameLen;
    CARD16 ledClass, ledID;
    unsigned wanted;
    char *str;

    REQUEST(xkbGetDeviceInfoReq);
    REQUEST_SIZE_MATCH(xkbGetDeviceInfoReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    wanted = stuff->wanted;

    CHK_ANY_DEVICE(dev, stuff->deviceSpec, client, DixGetAttrAccess);
    CHK_MASK_LEGAL(0x01, wanted, XkbXI_AllDeviceFeaturesMask);

    if ((!dev->button) || ((stuff->nBtns < 1) && (!stuff->allBtns)))
        wanted &= ~XkbXI_ButtonActionsMask;
    if ((!dev->kbdfeed) && (!dev->leds))
        wanted &= ~XkbXI_IndicatorsMask;

    nameLen = XkbSizeCountedString(dev->name);
    rep = (xkbGetDeviceInfoReply) {
        .type = X_Reply,
        .deviceID = dev->id,
        .sequenceNumber = client->sequence,
        .length = nameLen / 4,
        .present = wanted,
        .supported = XkbXI_AllDeviceFeaturesMask,
        .unsupported = 0,
        .nDeviceLedFBs = 0,
        .firstBtnWanted = 0,
        .nBtnsWanted = 0,
        .firstBtnRtrn = 0,
        .nBtnsRtrn = 0,
        .totalBtns = dev->button ? dev->button->numButtons : 0,
        .hasOwnState = (dev->key && dev->key->xkbInfo),
        .dfltKbdFB = dev->kbdfeed ? dev->kbdfeed->ctrl.id : XkbXINone,
        .dfltLedFB = dev->leds ? dev->leds->ctrl.id : XkbXINone,
        .devType = dev->xinput_type
    };

    ledClass = stuff->ledClass;
    ledID = stuff->ledID;

    if (wanted & XkbXI_ButtonActionsMask) {
        if (stuff->allBtns) {
            stuff->firstBtn = 0;
            stuff->nBtns = dev->button->numButtons;
        }

        if ((stuff->firstBtn + stuff->nBtns) > dev->button->numButtons) {
            client->errorValue = _XkbErrCode4(0x02, dev->button->numButtons,
                                              stuff->firstBtn, stuff->nBtns);
            return BadValue;
        }
        else {
            rep.firstBtnWanted = stuff->firstBtn;
            rep.nBtnsWanted = stuff->nBtns;
            if (dev->button->xkb_acts != NULL) {
                XkbAction *act;
                register int i;

                rep.firstBtnRtrn = stuff->firstBtn;
                rep.nBtnsRtrn = stuff->nBtns;
                act = &dev->button->xkb_acts[rep.firstBtnWanted];
                for (i = 0; i < rep.nBtnsRtrn; i++, act++) {
                    if (act->type != XkbSA_NoAction)
                        break;
                }
                rep.firstBtnRtrn += i;
                rep.nBtnsRtrn -= i;
                act =
                    &dev->button->xkb_acts[rep.firstBtnRtrn + rep.nBtnsRtrn -
                                           1];
                for (i = 0; i < rep.nBtnsRtrn; i++, act--) {
                    if (act->type != XkbSA_NoAction)
                        break;
                }
                rep.nBtnsRtrn -= i;
            }
            rep.length += (rep.nBtnsRtrn * SIZEOF(xkbActionWireDesc)) / 4;
        }
    }

    if (wanted & XkbXI_IndicatorsMask) {
        status = CheckDeviceLedFBs(dev, ledClass, ledID, &rep, client);
        if (status != Success)
            return status;
    }
    length = rep.length * 4;
    nDeviceLedFBs = rep.nDeviceLedFBs;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.present);
        swaps(&rep.supported);
        swaps(&rep.unsupported);
        swaps(&rep.nDeviceLedFBs);
        swaps(&rep.dfltKbdFB);
        swaps(&rep.dfltLedFB);
        swapl(&rep.devType);
    }
    WriteToClient(client, SIZEOF(xkbGetDeviceInfoReply), &rep);

    str = malloc(nameLen);
    if (!str)
        return BadAlloc;
    XkbWriteCountedString(str, dev->name, client->swapped);
    WriteToClient(client, nameLen, str);
    free(str);
    length -= nameLen;

    if (rep.nBtnsRtrn > 0) {
        int sz;
        xkbActionWireDesc *awire;

        sz = rep.nBtnsRtrn * SIZEOF(xkbActionWireDesc);
        awire = (xkbActionWireDesc *) &dev->button->xkb_acts[rep.firstBtnRtrn];
        WriteToClient(client, sz, awire);
        length -= sz;
    }
    if (nDeviceLedFBs > 0) {
        status = SendDeviceLedFBs(dev, ledClass, ledID, length, client);
        if (status != Success)
            return status;
    }
    else if (length != 0) {
        ErrorF("[xkb] Internal Error!  BadLength in ProcXkbGetDeviceInfo\n");
        ErrorF("[xkb]                  Wrote %d fewer bytes than expected\n",
               length);
        return BadLength;
    }
    return Success;
}

static char *
CheckSetDeviceIndicators(char *wire,
                         DeviceIntPtr dev,
                         int num, int *status_rtrn, ClientPtr client,
                         xkbSetDeviceInfoReq * stuff)
{
    xkbDeviceLedsWireDesc *ledWire;
    int i;
    XkbSrvLedInfoPtr sli;

    ledWire = (xkbDeviceLedsWireDesc *) wire;
    for (i = 0; i < num; i++) {
        if (!_XkbCheckRequestBounds(client, stuff, ledWire, ledWire + 1)) {
            *status_rtrn = BadLength;
            return (char *) ledWire;
        }

        if (client->swapped) {
            swaps(&ledWire->ledClass);
            swaps(&ledWire->ledID);
            swapl(&ledWire->namesPresent);
            swapl(&ledWire->mapsPresent);
            swapl(&ledWire->physIndicators);
        }

        sli = XkbFindSrvLedInfo(dev, ledWire->ledClass, ledWire->ledID,
                                XkbXI_IndicatorsMask);
        if (sli != NULL) {
            register int n;
            register unsigned bit;
            int nMaps, nNames;
            CARD32 *atomWire;
            xkbIndicatorMapWireDesc *mapWire;

            nMaps = nNames = 0;
            for (n = 0, bit = 1; n < XkbNumIndicators; n++, bit <<= 1) {
                if (ledWire->namesPresent & bit)
                    nNames++;
                if (ledWire->mapsPresent & bit)
                    nMaps++;
            }
            atomWire = (CARD32 *) &ledWire[1];
            if (nNames > 0) {
                for (n = 0; n < nNames; n++) {
                    if (!_XkbCheckRequestBounds(client, stuff, atomWire, atomWire + 1)) {
                        *status_rtrn = BadLength;
                        return (char *) atomWire;
                    }

                    if (client->swapped) {
                        swapl(atomWire);
                    }
                    CHK_ATOM_OR_NONE3(((Atom) (*atomWire)), client->errorValue,
                                      *status_rtrn, NULL);
                    atomWire++;
                }
            }
            mapWire = (xkbIndicatorMapWireDesc *) atomWire;
            if (nMaps > 0) {
                for (n = 0; n < nMaps; n++) {
                    if (!_XkbCheckRequestBounds(client, stuff, mapWire, mapWire + 1)) {
                        *status_rtrn = BadLength;
                        return (char *) mapWire;
                    }
                    if (client->swapped) {
                        swaps(&mapWire->virtualMods);
                        swapl(&mapWire->ctrls);
                    }
                    CHK_MASK_LEGAL3(0x21, mapWire->whichGroups,
                                    XkbIM_UseAnyGroup,
                                    client->errorValue, *status_rtrn, NULL);
                    CHK_MASK_LEGAL3(0x22, mapWire->whichMods, XkbIM_UseAnyMods,
                                    client->errorValue, *status_rtrn, NULL);
                    mapWire++;
                }
            }
            ledWire = (xkbDeviceLedsWireDesc *) mapWire;
        }
        else {
            /* SHOULD NEVER HAPPEN */
            return (char *) ledWire;
        }
    }
    return (char *) ledWire;
}

static char *
SetDeviceIndicators(char *wire,
                    DeviceIntPtr dev,
                    unsigned changed,
                    int num,
                    int *status_rtrn,
                    ClientPtr client,
                    xkbExtensionDeviceNotify * ev,
                    xkbSetDeviceInfoReq * stuff)
{
    xkbDeviceLedsWireDesc *ledWire;
    int i;
    XkbEventCauseRec cause;
    unsigned namec, mapc, statec;
    xkbExtensionDeviceNotify ed;
    XkbChangesRec changes;
    DeviceIntPtr kbd;

    memset((char *) &ed, 0, sizeof(xkbExtensionDeviceNotify));
    memset((char *) &changes, 0, sizeof(XkbChangesRec));
    XkbSetCauseXkbReq(&cause, X_kbSetDeviceInfo, client);
    ledWire = (xkbDeviceLedsWireDesc *) wire;
    for (i = 0; i < num; i++) {
        register int n;
        register unsigned bit;
        CARD32 *atomWire;
        xkbIndicatorMapWireDesc *mapWire;
        XkbSrvLedInfoPtr sli;

        namec = mapc = statec = 0;
        sli = XkbFindSrvLedInfo(dev, ledWire->ledClass, ledWire->ledID,
                                XkbXI_IndicatorMapsMask);
        if (!sli) {
            /* SHOULD NEVER HAPPEN!! */
            return (char *) ledWire;
        }

        atomWire = (CARD32 *) &ledWire[1];
        if (changed & XkbXI_IndicatorNamesMask) {
            namec = sli->namesPresent | ledWire->namesPresent;
            memset((char *) sli->names, 0, XkbNumIndicators * sizeof(Atom));
        }
        if (ledWire->namesPresent) {
            sli->namesPresent = ledWire->namesPresent;
            memset((char *) sli->names, 0, XkbNumIndicators * sizeof(Atom));
            for (n = 0, bit = 1; n < XkbNumIndicators; n++, bit <<= 1) {
                if (ledWire->namesPresent & bit) {
                    sli->names[n] = (Atom) *atomWire;
                    if (sli->names[n] == None)
                        ledWire->namesPresent &= ~bit;
                    atomWire++;
                }
            }
        }
        mapWire = (xkbIndicatorMapWireDesc *) atomWire;
        if (changed & XkbXI_IndicatorMapsMask) {
            mapc = sli->mapsPresent | ledWire->mapsPresent;
            sli->mapsPresent = ledWire->mapsPresent;
            memset((char *) sli->maps, 0,
                   XkbNumIndicators * sizeof(XkbIndicatorMapRec));
        }
        if (ledWire->mapsPresent) {
            for (n = 0, bit = 1; n < XkbNumIndicators; n++, bit <<= 1) {
                if (ledWire->mapsPresent & bit) {
                    sli->maps[n].flags = mapWire->flags;
                    sli->maps[n].which_groups = mapWire->whichGroups;
                    sli->maps[n].groups = mapWire->groups;
                    sli->maps[n].which_mods = mapWire->whichMods;
                    sli->maps[n].mods.mask = mapWire->mods;
                    sli->maps[n].mods.real_mods = mapWire->realMods;
                    sli->maps[n].mods.vmods = mapWire->virtualMods;
                    sli->maps[n].ctrls = mapWire->ctrls;
                    mapWire++;
                }
            }
        }
        if (changed & XkbXI_IndicatorStateMask) {
            statec = sli->effectiveState ^ ledWire->state;
            sli->explicitState &= ~statec;
            sli->explicitState |= (ledWire->state & statec);
        }
        if (namec)
            XkbApplyLedNameChanges(dev, sli, namec, &ed, &changes, &cause);
        if (mapc)
            XkbApplyLedMapChanges(dev, sli, mapc, &ed, &changes, &cause);
        if (statec)
            XkbApplyLedStateChanges(dev, sli, statec, &ed, &changes, &cause);

        kbd = dev;
        if ((sli->flags & XkbSLI_HasOwnState) == 0)
            kbd = inputInfo.keyboard;

        XkbFlushLedEvents(dev, kbd, sli, &ed, &changes, &cause);
        ledWire = (xkbDeviceLedsWireDesc *) mapWire;
    }
    return (char *) ledWire;
}

static int
_XkbSetDeviceInfoCheck(ClientPtr client, DeviceIntPtr dev,
                  xkbSetDeviceInfoReq * stuff)
{
    char *wire;

    wire = (char *) &stuff[1];
    if (stuff->change & XkbXI_ButtonActionsMask) {
        int sz = stuff->nBtns * SIZEOF(xkbActionWireDesc);
        if (!_XkbCheckRequestBounds(client, stuff, wire, (char *) wire + sz))
            return BadLength;

        if (!dev->button) {
            client->errorValue = _XkbErrCode2(XkbErr_BadClass, ButtonClass);
            return XkbKeyboardErrorCode;
        }
        if ((stuff->firstBtn + stuff->nBtns) > dev->button->numButtons) {
            client->errorValue =
                _XkbErrCode4(0x02, stuff->firstBtn, stuff->nBtns,
                             dev->button->numButtons);
            return BadMatch;
        }
        wire += sz;
    }
    if (stuff->change & XkbXI_IndicatorsMask) {
        int status = Success;

        wire = CheckSetDeviceIndicators(wire, dev, stuff->nDeviceLedFBs,
                                        &status, client, stuff);
        if (status != Success)
            return status;
    }
    if (((wire - ((char *) stuff)) / 4) != stuff->length)
        return BadLength;

    return Success;
}

static int
_XkbSetDeviceInfo(ClientPtr client, DeviceIntPtr dev,
                  xkbSetDeviceInfoReq * stuff)
{
    char *wire;
    xkbExtensionDeviceNotify ed;

    memset((char *) &ed, 0, SIZEOF(xkbExtensionDeviceNotify));
    ed.deviceID = dev->id;
    wire = (char *) &stuff[1];
    if (stuff->change & XkbXI_ButtonActionsMask) {
	int nBtns, sz, i;
        XkbAction *acts;
        DeviceIntPtr kbd;

        nBtns = dev->button->numButtons;
        acts = dev->button->xkb_acts;
        if (acts == NULL) {
            acts = calloc(nBtns, sizeof(XkbAction));
            if (!acts)
                return BadAlloc;
            dev->button->xkb_acts = acts;
        }
        if (stuff->firstBtn + stuff->nBtns > nBtns)
            return BadValue;
        sz = stuff->nBtns * SIZEOF(xkbActionWireDesc);
        memcpy((char *) &acts[stuff->firstBtn], (char *) wire, sz);
        wire += sz;
        ed.reason |= XkbXI_ButtonActionsMask;
        ed.firstBtn = stuff->firstBtn;
        ed.nBtns = stuff->nBtns;

        if (dev->key)
            kbd = dev;
        else
            kbd = inputInfo.keyboard;
        acts = &dev->button->xkb_acts[stuff->firstBtn];
        for (i = 0; i < stuff->nBtns; i++, acts++) {
            if (acts->type != XkbSA_NoAction)
                XkbSetActionKeyMods(kbd->key->xkbInfo->desc, acts, 0);
        }
    }
    if (stuff->change & XkbXI_IndicatorsMask) {
        int status = Success;

        wire = SetDeviceIndicators(wire, dev, stuff->change,
                                   stuff->nDeviceLedFBs, &status, client, &ed,
                                   stuff);
        if (status != Success)
            return status;
    }
    if ((stuff->change) && (ed.reason))
        XkbSendExtensionDeviceNotify(dev, client, &ed);
    return Success;
}

int
ProcXkbSetDeviceInfo(ClientPtr client)
{
    DeviceIntPtr dev;
    int rc;

    REQUEST(xkbSetDeviceInfoReq);
    REQUEST_AT_LEAST_SIZE(xkbSetDeviceInfoReq);

    if (!(client->xkbClientFlags & _XkbClientInitialized))
        return BadAccess;

    CHK_ANY_DEVICE(dev, stuff->deviceSpec, client, DixManageAccess);
    CHK_MASK_LEGAL(0x01, stuff->change, XkbXI_AllFeaturesMask);

    rc = _XkbSetDeviceInfoCheck(client, dev, stuff);

    if (rc != Success)
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd ||
        stuff->deviceSpec == XkbUseCorePtr) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if (((other != dev) && !IsMaster(other) &&
                 GetMaster(other, MASTER_KEYBOARD) == dev) &&
                ((stuff->deviceSpec == XkbUseCoreKbd && other->key) ||
                 (stuff->deviceSpec == XkbUseCorePtr && other->button))) {
                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success) {
                    rc = _XkbSetDeviceInfoCheck(client, other, stuff);
                    if (rc != Success)
                        return rc;
                }
            }
        }
    }

    /* checks done, apply */
    rc = _XkbSetDeviceInfo(client, dev, stuff);
    if (rc != Success)
        return rc;

    if (stuff->deviceSpec == XkbUseCoreKbd ||
        stuff->deviceSpec == XkbUseCorePtr) {
        DeviceIntPtr other;

        for (other = inputInfo.devices; other; other = other->next) {
            if (((other != dev) && !IsMaster(other) &&
                 GetMaster(other, MASTER_KEYBOARD) == dev) &&
                ((stuff->deviceSpec == XkbUseCoreKbd && other->key) ||
                 (stuff->deviceSpec == XkbUseCorePtr && other->button))) {
                rc = XaceHook(XACE_DEVICE_ACCESS, client, other,
                              DixManageAccess);
                if (rc == Success) {
                    rc = _XkbSetDeviceInfo(client, other, stuff);
                    if (rc != Success)
                        return rc;
                }
            }
        }
    }

    return Success;
}

/***====================================================================***/

int
ProcXkbSetDebuggingFlags(ClientPtr client)
{
    CARD32 newFlags, newCtrls, extraLength;
    xkbSetDebuggingFlagsReply rep;
    int rc;

    REQUEST(xkbSetDebuggingFlagsReq);
    REQUEST_AT_LEAST_SIZE(xkbSetDebuggingFlagsReq);

    rc = XaceHook(XACE_SERVER_ACCESS, client, DixDebugAccess);
    if (rc != Success)
        return rc;

    newFlags = xkbDebugFlags & (~stuff->affectFlags);
    newFlags |= (stuff->flags & stuff->affectFlags);
    newCtrls = xkbDebugCtrls & (~stuff->affectCtrls);
    newCtrls |= (stuff->ctrls & stuff->affectCtrls);
    if (xkbDebugFlags || newFlags || stuff->msgLength) {
        ErrorF("[xkb] XkbDebug: Setting debug flags to 0x%lx\n",
               (long) newFlags);
        if (newCtrls != xkbDebugCtrls)
            ErrorF("[xkb] XkbDebug: Setting debug controls to 0x%lx\n",
                   (long) newCtrls);
    }
    extraLength = (stuff->length << 2) - sz_xkbSetDebuggingFlagsReq;
    if (stuff->msgLength > 0) {
        char *msg;

        if (extraLength < XkbPaddedSize(stuff->msgLength)) {
            ErrorF
                ("[xkb] XkbDebug: msgLength= %d, length= %ld (should be %d)\n",
                 stuff->msgLength, (long) extraLength,
                 XkbPaddedSize(stuff->msgLength));
            return BadLength;
        }
        msg = (char *) &stuff[1];
        if (msg[stuff->msgLength - 1] != '\0') {
            ErrorF("[xkb] XkbDebug: message not null-terminated\n");
            return BadValue;
        }
        ErrorF("[xkb] XkbDebug: %s\n", msg);
    }
    xkbDebugFlags = newFlags;
    xkbDebugCtrls = newCtrls;

    rep = (xkbSetDebuggingFlagsReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .currentFlags = newFlags,
        .currentCtrls = newCtrls,
        .supportedFlags = ~0,
        .supportedCtrls = ~0
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.currentFlags);
        swapl(&rep.currentCtrls);
        swapl(&rep.supportedFlags);
        swapl(&rep.supportedCtrls);
    }
    WriteToClient(client, SIZEOF(xkbSetDebuggingFlagsReply), &rep);
    return Success;
}

/***====================================================================***/

static int
ProcXkbDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_kbUseExtension:
        return ProcXkbUseExtension(client);
    case X_kbSelectEvents:
        return ProcXkbSelectEvents(client);
    case X_kbBell:
        return ProcXkbBell(client);
    case X_kbGetState:
        return ProcXkbGetState(client);
    case X_kbLatchLockState:
        return ProcXkbLatchLockState(client);
    case X_kbGetControls:
        return ProcXkbGetControls(client);
    case X_kbSetControls:
        return ProcXkbSetControls(client);
    case X_kbGetMap:
        return ProcXkbGetMap(client);
    case X_kbSetMap:
        return ProcXkbSetMap(client);
    case X_kbGetCompatMap:
        return ProcXkbGetCompatMap(client);
    case X_kbSetCompatMap:
        return ProcXkbSetCompatMap(client);
    case X_kbGetIndicatorState:
        return ProcXkbGetIndicatorState(client);
    case X_kbGetIndicatorMap:
        return ProcXkbGetIndicatorMap(client);
    case X_kbSetIndicatorMap:
        return ProcXkbSetIndicatorMap(client);
    case X_kbGetNamedIndicator:
        return ProcXkbGetNamedIndicator(client);
    case X_kbSetNamedIndicator:
        return ProcXkbSetNamedIndicator(client);
    case X_kbGetNames:
        return ProcXkbGetNames(client);
    case X_kbSetNames:
        return ProcXkbSetNames(client);
    case X_kbGetGeometry:
        return ProcXkbGetGeometry(client);
    case X_kbSetGeometry:
        return ProcXkbSetGeometry(client);
    case X_kbPerClientFlags:
        return ProcXkbPerClientFlags(client);
    case X_kbListComponents:
        return ProcXkbListComponents(client);
    case X_kbGetKbdByName:
        return ProcXkbGetKbdByName(client);
    case X_kbGetDeviceInfo:
        return ProcXkbGetDeviceInfo(client);
    case X_kbSetDeviceInfo:
        return ProcXkbSetDeviceInfo(client);
    case X_kbSetDebuggingFlags:
        return ProcXkbSetDebuggingFlags(client);
    default:
        return BadRequest;
    }
}

static int
XkbClientGone(void *data, XID id)
{
    DevicePtr pXDev = (DevicePtr) data;

    if (!XkbRemoveResourceClient(pXDev, id)) {
        ErrorF
            ("[xkb] Internal Error! bad RemoveResourceClient in XkbClientGone\n");
    }
    return 1;
}

void
XkbExtensionInit(void)
{
    ExtensionEntry *extEntry;

    RT_XKBCLIENT = CreateNewResourceType(XkbClientGone, "XkbClient");
    if (!RT_XKBCLIENT)
        return;

    if (!XkbInitPrivates())
        return;

    if ((extEntry = AddExtension(XkbName, XkbNumberEvents, XkbNumberErrors,
                                 ProcXkbDispatch, SProcXkbDispatch,
                                 NULL, StandardMinorOpcode))) {
        XkbReqCode = (unsigned char) extEntry->base;
        XkbEventBase = (unsigned char) extEntry->eventBase;
        XkbErrorBase = (unsigned char) extEntry->errorBase;
        XkbKeyboardErrorCode = XkbErrorBase + XkbKeyboard;
    }
    return;
}
