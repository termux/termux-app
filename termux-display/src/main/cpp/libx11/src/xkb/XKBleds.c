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

#define NEED_MAP_READERS
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"

Status
XkbGetIndicatorState(Display *dpy, unsigned deviceSpec, unsigned *pStateRtrn)
{
    register xkbGetIndicatorStateReq *req;
    xkbGetIndicatorStateReply rep;
    XkbInfoPtr xkbi;
    Bool ok;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbGetIndicatorState, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbGetIndicatorState;
    req->deviceSpec = deviceSpec;
    ok = _XReply(dpy, (xReply *) &rep, 0, xFalse);
    if (ok && (pStateRtrn != NULL))
        *pStateRtrn = rep.state;
    UnlockDisplay(dpy);
    SyncHandle();
    return (ok ? Success : BadImplementation);
}

Status
_XkbReadGetIndicatorMapReply(Display *dpy,
                             xkbGetIndicatorMapReply *rep,
                             XkbDescPtr xkb,
                             int *nread_rtrn)
{
    XkbIndicatorPtr leds;
    XkbReadBufferRec buf;

    if ((!xkb->indicators) && (XkbAllocIndicatorMaps(xkb) != Success))
        return BadAlloc;
    leds = xkb->indicators;

    leds->phys_indicators = rep->realIndicators;
    if (rep->length > 0) {
        register int left;

        if (!_XkbInitReadBuffer(dpy, &buf, (int) rep->length * 4))
            return BadAlloc;
        if (nread_rtrn)
            *nread_rtrn = (int) rep->length * 4;
        if (rep->which) {
            register int i, bit;

            left = (int) rep->which;
            for (i = 0, bit = 1; (i < XkbNumIndicators) && (left);
                 i++, bit <<= 1) {
                if (left & bit) {
                    xkbIndicatorMapWireDesc *wire;

                    wire = (xkbIndicatorMapWireDesc *)
                        _XkbGetReadBufferPtr(&buf,
                                             SIZEOF(xkbIndicatorMapWireDesc));
                    if (wire == NULL) {
                        _XkbFreeReadBuffer(&buf);
                        return BadAlloc;
                    }
                    leds->maps[i].flags = wire->flags;
                    leds->maps[i].which_groups = wire->whichGroups;
                    leds->maps[i].groups = wire->groups;
                    leds->maps[i].which_mods = wire->whichMods;
                    leds->maps[i].mods.mask = wire->mods;
                    leds->maps[i].mods.real_mods = wire->realMods;
                    leds->maps[i].mods.vmods = wire->virtualMods;
                    leds->maps[i].ctrls = wire->ctrls;
                    left &= ~bit;
                }
            }
        }
        left = _XkbFreeReadBuffer(&buf);
    }
    return Success;
}

Bool
XkbGetIndicatorMap(Display *dpy, unsigned long which, XkbDescPtr xkb)
{
    register xkbGetIndicatorMapReq *req;
    xkbGetIndicatorMapReply rep;
    XkbInfoPtr xkbi;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;
    if ((!which) || (!xkb))
        return BadValue;

    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    if (!xkb->indicators) {
        xkb->indicators = _XkbTypedCalloc(1, XkbIndicatorRec);
        if (!xkb->indicators) {
            UnlockDisplay(dpy);
            SyncHandle();
            return BadAlloc;
        }
    }
    GetReq(kbGetIndicatorMap, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbGetIndicatorMap;
    req->deviceSpec = xkb->device_spec;
    req->which = (CARD32) which;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return BadValue;
    }
    status = _XkbReadGetIndicatorMapReply(dpy, &rep, xkb, NULL);
    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

Bool
XkbSetIndicatorMap(Display *dpy, unsigned long which, XkbDescPtr xkb)
{
    register xkbSetIndicatorMapReq *req;
    register int i, bit;
    int nMaps;
    xkbIndicatorMapWireDesc *wire;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    if ((!xkb) || (!which) || (!xkb->indicators))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetIndicatorMap, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSetIndicatorMap;
    req->deviceSpec = xkb->device_spec;
    req->which = (CARD32) which;
    for (i = nMaps = 0, bit = 1; i < 32; i++, bit <<= 1) {
        if (which & bit)
            nMaps++;
    }
    req->length += (nMaps * sizeof(XkbIndicatorMapRec)) / 4;
    BufAlloc(xkbIndicatorMapWireDesc *, wire,
             (nMaps * SIZEOF(xkbIndicatorMapWireDesc)));
    for (i = 0, bit = 1; i < 32; i++, bit <<= 1) {
        if (which & bit) {
            wire->flags = xkb->indicators->maps[i].flags;
            wire->whichGroups = xkb->indicators->maps[i].which_groups;
            wire->groups = xkb->indicators->maps[i].groups;
            wire->whichMods = xkb->indicators->maps[i].which_mods;
            wire->mods = xkb->indicators->maps[i].mods.real_mods;
            wire->virtualMods = xkb->indicators->maps[i].mods.vmods;
            wire->ctrls = xkb->indicators->maps[i].ctrls;
            wire++;
        }
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbGetNamedDeviceIndicator(Display *dpy,
                           unsigned device,
                           unsigned class,
                           unsigned id,
                           Atom name,
                           int *pNdxRtrn,
                           Bool *pStateRtrn,
                           XkbIndicatorMapPtr pMapRtrn,
                           Bool *pRealRtrn)
{
    register xkbGetNamedIndicatorReq *req;
    xkbGetNamedIndicatorReply rep;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) || (name == None) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbGetNamedIndicator, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbGetNamedIndicator;
    req->deviceSpec = device;
    req->ledClass = class;
    req->ledID = id;
    req->indicator = (CARD32) name;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return False;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    if ((!rep.found) || (!rep.supported))
        return False;
    if (pNdxRtrn != NULL)
        *pNdxRtrn = rep.ndx;
    if (pStateRtrn != NULL)
        *pStateRtrn = rep.on;
    if (pMapRtrn != NULL) {
        pMapRtrn->flags = rep.flags;
        pMapRtrn->which_groups = rep.whichGroups;
        pMapRtrn->groups = rep.groups;
        pMapRtrn->which_mods = rep.whichMods;
        pMapRtrn->mods.mask = rep.mods;
        pMapRtrn->mods.real_mods = rep.realMods;
        pMapRtrn->mods.vmods = rep.virtualMods;
        pMapRtrn->ctrls = rep.ctrls;
    }
    if (pRealRtrn != NULL)
        *pRealRtrn = rep.realIndicator;
    return True;
}

Bool
XkbGetNamedIndicator(Display *dpy,
                     Atom name,
                     int *pNdxRtrn,
                     Bool *pStateRtrn,
                     XkbIndicatorMapPtr pMapRtrn,
                     Bool *pRealRtrn)
{
    return XkbGetNamedDeviceIndicator(dpy, XkbUseCoreKbd,
                                      XkbDfltXIClass, XkbDfltXIId,
                                      name, pNdxRtrn, pStateRtrn,
                                      pMapRtrn, pRealRtrn);
}

Bool
XkbSetNamedDeviceIndicator(Display *dpy,
                           unsigned device,
                           unsigned class,
                           unsigned id,
                           Atom name,
                           Bool changeState,
                           Bool state,
                           Bool createNewMap,
                           XkbIndicatorMapPtr pMap)
{
    register xkbSetNamedIndicatorReq *req;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) || (name == None) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetNamedIndicator, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSetNamedIndicator;
    req->deviceSpec = device;
    req->ledClass = class;
    req->ledID = id;
    req->indicator = (CARD32) name;
    req->setState = changeState;
    if (req->setState)
        req->on = state;
    else
        req->on = False;
    if (pMap != NULL) {
        req->setMap = True;
        req->createMap = createNewMap;
        req->flags = pMap->flags;
        req->whichGroups = pMap->which_groups;
        req->groups = pMap->groups;
        req->whichMods = pMap->which_mods;
        req->realMods = pMap->mods.real_mods;
        req->virtualMods = pMap->mods.vmods;
        req->ctrls = pMap->ctrls;
    }
    else {
        req->setMap = False;
        req->createMap = False;
        req->flags = 0;
        req->whichGroups = 0;
        req->groups = 0;
        req->whichMods = 0;
        req->realMods = 0;
        req->virtualMods = 0;
        req->ctrls = 0;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbSetNamedIndicator(Display *dpy,
                     Atom name,
                     Bool changeState,
                     Bool state,
                     Bool createNewMap,
                     XkbIndicatorMapPtr pMap)
{
    return XkbSetNamedDeviceIndicator(dpy, XkbUseCoreKbd,
                                      XkbDfltXIClass, XkbDfltXIId,
                                      name, changeState, state,
                                      createNewMap, pMap);
}
