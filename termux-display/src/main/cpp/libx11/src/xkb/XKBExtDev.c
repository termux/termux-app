/************************************************************
Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

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
#define	NEED_MAP_READERS
#include "Xlibint.h"
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"
#include <X11/extensions/XI.h>

/***====================================================================***/

void
XkbNoteDeviceChanges(XkbDeviceChangesPtr old,
                     XkbExtensionDeviceNotifyEvent *new,
                     unsigned int wanted)
{
    if ((!old) || (!new) || (!wanted) || ((new->reason & wanted) == 0))
        return;
    if ((wanted & new->reason) & XkbXI_ButtonActionsMask) {
        if (old->changed & XkbXI_ButtonActionsMask) {
            int first, last, newLast;

            if (new->first_btn < old->first_btn)
                first = new->first_btn;
            else
                first = old->first_btn;
            last = old->first_btn + old->num_btns - 1;
            newLast = new->first_btn + new->num_btns - 1;
            if (newLast > last)
                last = newLast;
            old->first_btn = first;
            old->num_btns = (last - first) + 1;
        }
        else {
            old->changed |= XkbXI_ButtonActionsMask;
            old->first_btn = new->first_btn;
            old->num_btns = new->num_btns;
        }
    }
    if ((wanted & new->reason) & XkbXI_IndicatorsMask) {
        XkbDeviceLedChangesPtr this;

        if (old->changed & XkbXI_IndicatorsMask) {
            XkbDeviceLedChangesPtr found = NULL;

            for (this = &old->leds; this && (!found); this = this->next) {
                if ((this->led_class == new->led_class) &&
                    (this->led_id == new->led_id)) {
                    found = this;
                }
            }
            if (!found) {
                found = _XkbTypedCalloc(1, XkbDeviceLedChangesRec);
                if (!found)
                    return;
                found->next = old->leds.next;
                found->led_class = new->led_class;
                found->led_id = new->led_id;
                old->leds.next = found;
            }
            if ((wanted & new->reason) & XkbXI_IndicatorNamesMask)
                found->defined = new->leds_defined;
        }
        else {
            old->changed |= ((wanted & new->reason) & XkbXI_IndicatorsMask);
            old->leds.led_class = new->led_class;
            old->leds.led_id = new->led_id;
            old->leds.defined = new->leds_defined;
            if (old->leds.next) {
                XkbDeviceLedChangesPtr next;

                for (this = old->leds.next; this; this = next) {
                    next = this->next;
                    _XkbFree(this);
                }
                old->leds.next = NULL;
            }
        }
    }
    return;
}

/***====================================================================***/

static Status
_XkbReadDeviceLedInfo(XkbReadBufferPtr buf,
                      unsigned present,
                      XkbDeviceInfoPtr devi)
{
    register unsigned i, bit;
    XkbDeviceLedInfoPtr devli;
    xkbDeviceLedsWireDesc *wireli;

    wireli = _XkbGetTypedRdBufPtr(buf, 1, xkbDeviceLedsWireDesc);
    if (!wireli)
        return BadLength;
    devli = XkbAddDeviceLedInfo(devi, wireli->ledClass, wireli->ledID);
    if (!devli)
        return BadAlloc;
    devli->phys_indicators = wireli->physIndicators;

    if (present & XkbXI_IndicatorStateMask)
        devli->state = wireli->state;

    if (present & XkbXI_IndicatorNamesMask) {
        devli->names_present = wireli->namesPresent;
        if (devli->names_present) {
            for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
                if (wireli->namesPresent & bit) {
                    if (!_XkbCopyFromReadBuffer(buf,
                                                (char *) &devli->names[i], 4))
                        return BadLength;
                }
            }
        }
    }

    if (present & XkbXI_IndicatorMapsMask) {
        devli->maps_present = wireli->mapsPresent;
        if (devli->maps_present) {
            XkbIndicatorMapPtr im;
            xkbIndicatorMapWireDesc *wireim;

            for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
                if (wireli->mapsPresent & bit) {
                    wireim =
                        _XkbGetTypedRdBufPtr(buf, 1, xkbIndicatorMapWireDesc);
                    if (!wireim)
                        return BadAlloc;
                    im = &devli->maps[i];
                    im->flags = wireim->flags;
                    im->which_groups = wireim->whichGroups;
                    im->groups = wireim->groups;
                    im->which_mods = wireim->whichMods;
                    im->mods.mask = wireim->mods;
                    im->mods.real_mods = wireim->realMods;
                    im->mods.vmods = wireim->virtualMods;
                    im->ctrls = wireim->ctrls;
                }
            }
        }
    }
    return Success;
}

static Status
_XkbReadGetDeviceInfoReply(Display *dpy,
                           xkbGetDeviceInfoReply *rep,
                           XkbDeviceInfoPtr devi)
{
    XkbReadBufferRec buf;
    XkbAction *act;
    int tmp;

    if (!_XkbInitReadBuffer(dpy, &buf, (int) rep->length * 4))
        return BadAlloc;

    if ((rep->totalBtns > 0) && (rep->totalBtns != devi->num_btns)) {
        tmp = XkbResizeDeviceButtonActions(devi, rep->totalBtns);
        if (tmp != Success)
            return tmp;
    }
    if (rep->nBtnsWanted > 0) {
        if (((unsigned short) rep->firstBtnWanted + rep->nBtnsWanted)
            >= devi->num_btns)
            goto BAILOUT;
        act = &devi->btn_acts[rep->firstBtnWanted];
        bzero((char *) act, (rep->nBtnsWanted * sizeof(XkbAction)));
    }

    _XkbFree(devi->name);
    if (!_XkbGetReadBufferCountedString(&buf, &devi->name))
        goto BAILOUT;
    if (rep->nBtnsRtrn > 0) {
        int size;

        if (((unsigned short) rep->firstBtnRtrn + rep->nBtnsRtrn)
            >= devi->num_btns)
            goto BAILOUT;
        act = &devi->btn_acts[rep->firstBtnRtrn];
        size = rep->nBtnsRtrn * SIZEOF(xkbActionWireDesc);
        if (!_XkbCopyFromReadBuffer(&buf, (char *) act, size))
            goto BAILOUT;
    }
    if (rep->nDeviceLedFBs > 0) {
        register int i;

        for (i = 0; i < rep->nDeviceLedFBs; i++) {
            if ((tmp = _XkbReadDeviceLedInfo(&buf, rep->present, devi))
                != Success)
                return tmp;
        }
    }
    tmp = _XkbFreeReadBuffer(&buf);
    if (tmp)
        fprintf(stderr, "GetDeviceInfo! Bad length (%d extra bytes)\n", tmp);
    if (tmp || buf.error)
        return BadLength;
    return Success;
 BAILOUT:
    _XkbFreeReadBuffer(&buf);
    return BadLength;
}

XkbDeviceInfoPtr
XkbGetDeviceInfo(Display *dpy,
                 unsigned which,
                 unsigned deviceSpec,
                 unsigned class,
                 unsigned id)
{
    register xkbGetDeviceInfoReq *req;
    xkbGetDeviceInfoReply rep;
    Status status;
    XkbDeviceInfoPtr devi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return NULL;
    LockDisplay(dpy);
    GetReq(kbGetDeviceInfo, req);
    req->reqType = dpy->xkb_info->codes->major_opcode;
    req->xkbReqType = X_kbGetDeviceInfo;
    req->deviceSpec = deviceSpec;
    req->wanted = which;
    req->allBtns = ((which & XkbXI_ButtonActionsMask) != 0);
    req->firstBtn = req->nBtns = 0;
    req->ledClass = class;
    req->ledID = id;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return NULL;
    }
    devi = XkbAllocDeviceInfo(rep.deviceID, rep.totalBtns, rep.nDeviceLedFBs);
    if (devi) {
        devi->supported = rep.supported;
        devi->unsupported = rep.unsupported;
        devi->type = rep.devType;
        devi->has_own_state = rep.hasOwnState;
        devi->dflt_kbd_fb = rep.dfltKbdFB;
        devi->dflt_led_fb = rep.dfltLedFB;
        status = _XkbReadGetDeviceInfoReply(dpy, &rep, devi);
        if (status != Success) {
            XkbFreeDeviceInfo(devi, XkbXI_AllDeviceFeaturesMask, True);
            devi = NULL;
        }
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return devi;
}

Status
XkbGetDeviceInfoChanges(Display *dpy,
                        XkbDeviceInfoPtr devi,
                        XkbDeviceChangesPtr changes)
{
    register xkbGetDeviceInfoReq *req;
    xkbGetDeviceInfoReply rep;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadMatch;
    if ((changes->changed & XkbXI_AllDeviceFeaturesMask) == 0)
        return Success;
    changes->changed &= ~XkbXI_AllDeviceFeaturesMask;
    status = Success;
    LockDisplay(dpy);
    while ((changes->changed) && (status == Success)) {
        GetReq(kbGetDeviceInfo, req);
        req->reqType = dpy->xkb_info->codes->major_opcode;
        req->xkbReqType = X_kbGetDeviceInfo;
        req->deviceSpec = devi->device_spec;
        req->wanted = changes->changed;
        req->allBtns = False;
        if (changes->changed & XkbXI_ButtonActionsMask) {
            req->firstBtn = changes->first_btn;
            req->nBtns = changes->num_btns;
            changes->changed &= ~XkbXI_ButtonActionsMask;
        }
        else
            req->firstBtn = req->nBtns = 0;
        if (changes->changed & XkbXI_IndicatorsMask) {
            req->ledClass = changes->leds.led_class;
            req->ledID = changes->leds.led_id;
            if (changes->leds.next == NULL)
                changes->changed &= ~XkbXI_IndicatorsMask;
            else {
                XkbDeviceLedChangesPtr next;

                next = changes->leds.next;
                changes->leds = *next;
                _XkbFree(next);
            }
        }
        else {
            req->ledClass = XkbDfltXIClass;
            req->ledID = XkbDfltXIId;
        }
        if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
            status = BadLength;
            break;
        }
        devi->supported |= rep.supported;
        devi->unsupported |= rep.unsupported;
        devi->type = rep.devType;
        status = _XkbReadGetDeviceInfoReply(dpy, &rep, devi);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

Status
XkbGetDeviceButtonActions(Display *dpy,
                          XkbDeviceInfoPtr devi,
                          Bool all,
                          unsigned int first,
                          unsigned int num)
{
    register xkbGetDeviceInfoReq *req;
    xkbGetDeviceInfoReply rep;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadMatch;
    if (!devi)
        return BadValue;
    LockDisplay(dpy);
    GetReq(kbGetDeviceInfo, req);
    req->reqType = dpy->xkb_info->codes->major_opcode;
    req->xkbReqType = X_kbGetDeviceInfo;
    req->deviceSpec = devi->device_spec;
    req->wanted = XkbXI_ButtonActionsMask;
    req->allBtns = all;
    req->firstBtn = first;
    req->nBtns = num;
    req->ledClass = XkbDfltXIClass;
    req->ledID = XkbDfltXIId;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return BadLength;
    }
    devi->type = rep.devType;
    devi->supported = rep.supported;
    devi->unsupported = rep.unsupported;
    status = _XkbReadGetDeviceInfoReply(dpy, &rep, devi);
    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

Status
XkbGetDeviceLedInfo(Display *dpy,
                    XkbDeviceInfoPtr devi,
                    unsigned int ledClass,
                    unsigned int ledId,
                    unsigned int which)
{
    register xkbGetDeviceInfoReq *req;
    xkbGetDeviceInfoReply rep;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadMatch;
    if (((which & XkbXI_IndicatorsMask) == 0) ||
        (which & (~XkbXI_IndicatorsMask)))
        return BadMatch;
    if (!devi)
        return BadValue;
    LockDisplay(dpy);
    GetReq(kbGetDeviceInfo, req);
    req->reqType = dpy->xkb_info->codes->major_opcode;
    req->xkbReqType = X_kbGetDeviceInfo;
    req->deviceSpec = devi->device_spec;
    req->wanted = which;
    req->allBtns = False;
    req->firstBtn = req->nBtns = 0;
    req->ledClass = ledClass;
    req->ledID = ledId;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return BadLength;
    }
    devi->type = rep.devType;
    devi->supported = rep.supported;
    devi->unsupported = rep.unsupported;
    status = _XkbReadGetDeviceInfoReply(dpy, &rep, devi);
    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

/***====================================================================***/

typedef struct _LedInfoStuff {
    Bool used;
    XkbDeviceLedInfoPtr devli;
} LedInfoStuff;

typedef struct _SetLedStuff {
    unsigned wanted;
    int num_info;
    int dflt_class;
    LedInfoStuff *dflt_kbd_fb;
    LedInfoStuff *dflt_led_fb;
    LedInfoStuff *info;
} SetLedStuff;

static void
_InitLedStuff(SetLedStuff *stuff, unsigned wanted, XkbDeviceInfoPtr devi)
{
    int i;
    register XkbDeviceLedInfoPtr devli;

    bzero(stuff, sizeof(SetLedStuff));
    stuff->wanted = wanted;
    stuff->dflt_class = XkbXINone;
    if ((devi->num_leds < 1) || ((wanted & XkbXI_IndicatorsMask) == 0))
        return;
    stuff->info = _XkbTypedCalloc(devi->num_leds, LedInfoStuff);
    if (!stuff->info)
        return;
    stuff->num_info = devi->num_leds;
    for (devli = &devi->leds[0], i = 0; i < devi->num_leds; i++, devli++) {
        stuff->info[i].devli = devli;
        if (devli->led_class == KbdFeedbackClass) {
            stuff->dflt_class = KbdFeedbackClass;
            if (stuff->dflt_kbd_fb == NULL)
                stuff->dflt_kbd_fb = &stuff->info[i];
        }
        else if (devli->led_class == LedFeedbackClass) {
            if (stuff->dflt_class == XkbXINone)
                stuff->dflt_class = LedFeedbackClass;
            if (stuff->dflt_led_fb == NULL)
                stuff->dflt_led_fb = &stuff->info[i];
        }
    }
    return;
}

static void
_FreeLedStuff(SetLedStuff * stuff)
{
    if (stuff->num_info > 0)
        _XkbFree(stuff->info);
    bzero(stuff, sizeof(SetLedStuff));
    return;
}

static int
_XkbSizeLedInfo(unsigned changed, XkbDeviceLedInfoPtr devli)
{
    register int i, size;
    register unsigned bit, namesNeeded, mapsNeeded;

    size = SIZEOF(xkbDeviceLedsWireDesc);
    namesNeeded = mapsNeeded = 0;
    if (changed & XkbXI_IndicatorNamesMask)
        namesNeeded = devli->names_present;
    if (changed & XkbXI_IndicatorMapsMask)
        mapsNeeded = devli->maps_present;
    if ((namesNeeded) || (mapsNeeded)) {
        for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
            if (namesNeeded & bit)
                size += 4;      /* atoms are 4 bytes on the wire */
            if (mapsNeeded & bit)
                size += SIZEOF(xkbIndicatorMapWireDesc);
        }
    }
    return size;
}

static Bool
_SizeMatches(SetLedStuff *stuff,
             XkbDeviceLedChangesPtr changes,
             int *sz_rtrn,
             int *nleds_rtrn)
{
    int i, nMatch, class, id;
    LedInfoStuff *linfo;
    Bool match;

    nMatch = 0;
    class = changes->led_class;
    id = changes->led_id;
    if (class == XkbDfltXIClass)
        class = stuff->dflt_class;
    for (i = 0, linfo = &stuff->info[0]; i < stuff->num_info; i++, linfo++) {
        XkbDeviceLedInfoPtr devli;
        LedInfoStuff *dflt;

        devli = linfo->devli;
        match = ((class == devli->led_class) || (class == XkbAllXIClasses));
        if (devli->led_class == KbdFeedbackClass)
            dflt = stuff->dflt_kbd_fb;
        else
            dflt = stuff->dflt_led_fb;
        match = (match && (id == devli->led_id)) ||
            (id == XkbAllXIIds) ||
            ((id == XkbDfltXIId) && (linfo == dflt));
        if (match) {
            if (!linfo->used) {
                *sz_rtrn += _XkbSizeLedInfo(stuff->wanted, devli);
                *nleds_rtrn += 1;
                linfo->used = True;
                if ((class != XkbAllXIClasses) && (id != XkbAllXIIds))
                    return True;
            }
            nMatch++;
            linfo->used = True;
        }
    }
    return (nMatch > 0);
}

/***====================================================================***/


static Status
_XkbSetDeviceInfoSize(XkbDeviceInfoPtr devi,
                      XkbDeviceChangesPtr changes,
                      SetLedStuff *stuff,
                      int *sz_rtrn,
                      int *num_leds_rtrn)
{
    *sz_rtrn = 0;
    if ((changes->changed & XkbXI_ButtonActionsMask) && (changes->num_btns > 0)) {
        if (!XkbXI_LegalDevBtn
            (devi, (changes->first_btn + changes->num_btns - 1)))
            return BadMatch;
        *sz_rtrn += changes->num_btns * SIZEOF(xkbActionWireDesc);
    }
    else {
        changes->changed &= ~XkbXI_ButtonActionsMask;
        changes->first_btn = changes->num_btns = 0;
    }
    if ((changes->changed & XkbXI_IndicatorsMask) &&
        XkbLegalXILedClass(changes->leds.led_class)) {
        XkbDeviceLedChangesPtr leds;

        for (leds = &changes->leds; leds != NULL; leds = leds->next) {
            if (!_SizeMatches(stuff, leds, sz_rtrn, num_leds_rtrn))
                return BadMatch;
        }
    }
    else {
        changes->changed &= ~XkbXI_IndicatorsMask;
        *num_leds_rtrn = 0;
    }
    return Success;
}

static char *
_XkbWriteLedInfo(char *wire, unsigned changed, XkbDeviceLedInfoPtr devli)
{
    register int i;
    register unsigned bit, namesNeeded, mapsNeeded;
    xkbDeviceLedsWireDesc *lwire;

    namesNeeded = mapsNeeded = 0;
    if (changed & XkbXI_IndicatorNamesMask)
        namesNeeded = devli->names_present;
    if (changed & XkbXI_IndicatorMapsMask)
        mapsNeeded = devli->maps_present;

    lwire = (xkbDeviceLedsWireDesc *) wire;
    lwire->ledClass = devli->led_class;
    lwire->ledID = devli->led_id;
    lwire->namesPresent = namesNeeded;
    lwire->mapsPresent = mapsNeeded;
    lwire->physIndicators = devli->phys_indicators;
    lwire->state = devli->state;
    wire = (char *) &lwire[1];
    if (namesNeeded) {
        CARD32 *awire = (CARD32 *) wire;

        for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
            if (namesNeeded & bit) {
                *awire = (CARD32) devli->names[i];
                awire++;
            }
        }
        wire = (char *) awire;
    }
    if (mapsNeeded) {
        xkbIndicatorMapWireDesc *mwire = (xkbIndicatorMapWireDesc *) wire;

        for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
            if (mapsNeeded & bit) {
                XkbIndicatorMapPtr map = &devli->maps[i];

                mwire->flags = map->flags;
                mwire->whichGroups = map->which_groups;
                mwire->groups = map->groups;
                mwire->whichMods = map->which_mods;
                mwire->mods = map->mods.mask;
                mwire->realMods = map->mods.real_mods;
                mwire->virtualMods = map->mods.vmods;
                mwire->ctrls = map->ctrls;
                mwire++;
            }
        }
        wire = (char *) mwire;
    }
    return wire;
}


static int
_XkbWriteSetDeviceInfo(char *wire,
                       XkbDeviceChangesPtr changes,
                       SetLedStuff *stuff,
                       XkbDeviceInfoPtr devi)
{
    char *start = wire;

    if (changes->changed & XkbXI_ButtonActionsMask) {
        int size = changes->num_btns * SIZEOF(xkbActionWireDesc);

        memcpy(wire, (char *) &devi->btn_acts[changes->first_btn], (size_t) size);
        wire += size;
    }
    if (changes->changed & XkbXI_IndicatorsMask) {
        register int i;
        register LedInfoStuff *linfo;

        for (i = 0, linfo = &stuff->info[0]; i < stuff->num_info; i++, linfo++) {
            if (linfo->used) {
                register char *new_wire;

                new_wire = _XkbWriteLedInfo(wire, stuff->wanted, linfo->devli);
                if (!new_wire)
                    return wire - start;
                wire = new_wire;
            }
        }
    }
    return wire - start;
}

Bool
XkbSetDeviceInfo(Display *dpy, unsigned which, XkbDeviceInfoPtr devi)
{
    register xkbSetDeviceInfoReq *req;
    Status ok = 0;
    int size, nLeds;
    XkbInfoPtr xkbi;
    XkbDeviceChangesRec changes;
    SetLedStuff lstuff;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    if ((!devi) || (which & (~XkbXI_AllDeviceFeaturesMask)) ||
        ((which & XkbXI_ButtonActionsMask) && (!XkbXI_DevHasBtnActs(devi))) ||
        ((which & XkbXI_IndicatorsMask) && (!XkbXI_DevHasLeds(devi))))
        return False;

    bzero((char *) &changes, sizeof(XkbDeviceChangesRec));
    changes.changed = which;
    changes.first_btn = 0;
    changes.num_btns = devi->num_btns;
    changes.leds.led_class = XkbAllXIClasses;
    changes.leds.led_id = XkbAllXIIds;
    changes.leds.defined = 0;
    size = nLeds = 0;
    _InitLedStuff(&lstuff, changes.changed, devi);
    if (_XkbSetDeviceInfoSize(devi, &changes, &lstuff, &size, &nLeds) !=
        Success)
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetDeviceInfo, req);
    req->length += size / 4;
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSetDeviceInfo;
    req->deviceSpec = devi->device_spec;
    req->firstBtn = changes.first_btn;
    req->nBtns = changes.num_btns;
    req->change = changes.changed;
    req->nDeviceLedFBs = nLeds;
    if (size > 0) {
        char *wire;

        BufAlloc(char *, wire, size);
        ok = (wire != NULL) &&
            (_XkbWriteSetDeviceInfo(wire, &changes, &lstuff, devi) == size);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    _FreeLedStuff(&lstuff);
    /* 12/11/95 (ef) -- XXX!! should clear changes here */
    return ok;
}

Bool
XkbChangeDeviceInfo(Display *dpy,
                    XkbDeviceInfoPtr devi,
                    XkbDeviceChangesPtr changes)
{
    register xkbSetDeviceInfoReq *req;
    Status ok = 0;
    int size, nLeds;
    XkbInfoPtr xkbi;
    SetLedStuff lstuff;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    if ((!devi) || (changes->changed & (~XkbXI_AllDeviceFeaturesMask)) ||
        ((changes->changed & XkbXI_ButtonActionsMask) &&
         (!XkbXI_DevHasBtnActs(devi))) ||
        ((changes->changed & XkbXI_IndicatorsMask) &&
         (!XkbXI_DevHasLeds(devi))))
        return False;

    size = nLeds = 0;
    _InitLedStuff(&lstuff, changes->changed, devi);
    if (_XkbSetDeviceInfoSize(devi, changes, &lstuff, &size, &nLeds) != Success)
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetDeviceInfo, req);
    req->length += size / 4;
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSetDeviceInfo;
    req->deviceSpec = devi->device_spec;
    req->firstBtn = changes->first_btn;
    req->nBtns = changes->num_btns;
    req->change = changes->changed;
    req->nDeviceLedFBs = nLeds;
    if (size > 0) {
        char *wire;

        BufAlloc(char *, wire, size);
        ok = (wire != NULL) &&
            (_XkbWriteSetDeviceInfo(wire, changes, &lstuff, devi) == size);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    _FreeLedStuff(&lstuff);
    /* 12/11/95 (ef) -- XXX!! should clear changes here */
    return ok;
}

Bool
XkbSetDeviceLedInfo(Display *dpy,
                    XkbDeviceInfoPtr devi,
                    unsigned ledClass,
                    unsigned ledID,
                    unsigned which)
{
    return False;
}

Bool
XkbSetDeviceButtonActions(Display *dpy,
                          XkbDeviceInfoPtr devi,
                          unsigned int first,
                          unsigned int nBtns)
{
    register xkbSetDeviceInfoReq *req;
    Status ok = 0;
    int size, nLeds;
    XkbInfoPtr xkbi;
    XkbDeviceChangesRec changes;
    SetLedStuff lstuff;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    if ((!devi) || (!XkbXI_DevHasBtnActs(devi)) ||
        (first + nBtns > devi->num_btns))
        return False;
    if (nBtns == 0)
        return True;

    bzero((char *) &changes, sizeof(XkbDeviceChangesRec));
    changes.changed = XkbXI_ButtonActionsMask;
    changes.first_btn = first;
    changes.num_btns = nBtns;
    changes.leds.led_class = XkbXINone;
    changes.leds.led_id = XkbXINone;
    changes.leds.defined = 0;
    size = nLeds = 0;
    if (_XkbSetDeviceInfoSize(devi, &changes, NULL, &size, &nLeds) != Success)
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetDeviceInfo, req);
    req->length += size / 4;
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSetDeviceInfo;
    req->deviceSpec = devi->device_spec;
    req->firstBtn = changes.first_btn;
    req->nBtns = changes.num_btns;
    req->change = changes.changed;
    req->nDeviceLedFBs = nLeds;
    if (size > 0) {
        char *wire;

        BufAlloc(char *, wire, size);
        ok = (wire != NULL) &&
            (_XkbWriteSetDeviceInfo(wire, &changes, &lstuff, devi) == size);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return ok;
}
