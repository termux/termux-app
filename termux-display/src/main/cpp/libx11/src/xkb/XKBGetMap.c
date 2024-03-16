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

#define	NEED_MAP_READERS
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"

static Status
_XkbReadKeyTypes(XkbReadBufferPtr buf, XkbDescPtr xkb, xkbGetMapReply *rep)
{
    int i, n, lastMapCount;
    XkbKeyTypePtr type;

    if (rep->nTypes > 0) {
        n = rep->firstType + rep->nTypes;
        if (xkb->map->num_types >= n)
            n = xkb->map->num_types;
        else if (XkbAllocClientMap(xkb, XkbKeyTypesMask, n) != Success)
            return BadAlloc;

        type = &xkb->map->types[rep->firstType];
        for (i = 0; i < (int) rep->nTypes; i++, type++) {
            xkbKeyTypeWireDesc *desc;
            register int ndx;

            ndx = i + rep->firstType;
            if (ndx >= xkb->map->num_types)
                xkb->map->num_types = ndx + 1;

            desc = (xkbKeyTypeWireDesc *)
                _XkbGetReadBufferPtr(buf, SIZEOF(xkbKeyTypeWireDesc));
            if (desc == NULL)
                return BadLength;

            lastMapCount = type->map_count;
            if (desc->nMapEntries > 0) {
                if ((type->map == NULL) ||
                    (desc->nMapEntries > type->map_count)) {
                    _XkbResizeArray(type->map, type->map_count,
                                  desc->nMapEntries, XkbKTMapEntryRec);
                    if (type->map == NULL) {
                        return BadAlloc;
                    }
                }
            }
            else if (type->map != NULL) {
                Xfree(type->map);
                type->map_count = 0;
                type->map = NULL;
            }

            if (desc->preserve && (desc->nMapEntries > 0)) {
                if ((!type->preserve) || (desc->nMapEntries > lastMapCount)) {
                    _XkbResizeArray(type->preserve, lastMapCount,
                                    desc->nMapEntries, XkbModsRec);
                    if (type->preserve == NULL) {
                        return BadAlloc;
                    }
                }
            }
            else if (type->preserve != NULL) {
                Xfree(type->preserve);
                type->preserve = NULL;
            }

            type->mods.mask = desc->mask;
            type->mods.real_mods = desc->realMods;
            type->mods.vmods = desc->virtualMods;
            type->num_levels = desc->numLevels;
            type->map_count = desc->nMapEntries;
            if (desc->nMapEntries > 0) {
                register xkbKTMapEntryWireDesc *wire;
                register XkbKTMapEntryPtr entry;
                register int size;

                size = type->map_count * SIZEOF(xkbKTMapEntryWireDesc);
                wire =
                    (xkbKTMapEntryWireDesc *) _XkbGetReadBufferPtr(buf, size);
                if (wire == NULL)
                    return BadLength;
                entry = type->map;
                for (n = 0; n < type->map_count; n++, wire++, entry++) {
                    entry->active = wire->active;
                    entry->level = wire->level;
                    entry->mods.mask = wire->mask;
                    entry->mods.real_mods = wire->realMods;
                    entry->mods.vmods = wire->virtualMods;
                }

                if (desc->preserve) {
                    register xkbModsWireDesc *pwire;
                    register XkbModsPtr preserve;
                    register int sz;

                    sz = desc->nMapEntries * SIZEOF(xkbModsWireDesc);
                    pwire = (xkbModsWireDesc *) _XkbGetReadBufferPtr(buf, sz);
                    if (pwire == NULL)
                        return BadLength;
                    preserve = type->preserve;
                    for (n = 0; n < desc->nMapEntries; n++, pwire++, preserve++) {
                        preserve->mask = pwire->mask;
                        preserve->vmods = pwire->virtualMods;
                        preserve->real_mods = pwire->realMods;
                    }
                }
            }
        }
    }
    return Success;
}

static Status
_XkbReadKeySyms(XkbReadBufferPtr buf, XkbDescPtr xkb, xkbGetMapReply *rep)
{
    register int i;
    XkbClientMapPtr map;
    int size = xkb->max_key_code + 1;

    if (((unsigned short) rep->firstKeySym + rep->nKeySyms) > size)
        return BadLength;

    map = xkb->map;
    if (map->key_sym_map == NULL) {
        register int offset;
        XkbSymMapPtr oldMap;
        xkbSymMapWireDesc *newMap;

        map->key_sym_map = _XkbTypedCalloc(size, XkbSymMapRec);
        if (map->key_sym_map == NULL)
            return BadAlloc;
        if (map->syms == NULL) {
            int sz;

            sz = (rep->totalSyms * 12) / 10;
            sz = ((sz + (unsigned) 128) / 128) * 128;
            map->syms = _XkbTypedCalloc(sz, KeySym);
            if (map->syms == NULL)
                return BadAlloc;
            map->size_syms = sz;
        }
        offset = 1;
        oldMap = &map->key_sym_map[rep->firstKeySym];
        for (i = 0; i < (int) rep->nKeySyms; i++, oldMap++) {
            newMap = (xkbSymMapWireDesc *)
                _XkbGetReadBufferPtr(buf, SIZEOF(xkbSymMapWireDesc));
            if (newMap == NULL)
                return BadLength;
            oldMap->kt_index[0] = newMap->ktIndex[0];
            oldMap->kt_index[1] = newMap->ktIndex[1];
            oldMap->kt_index[2] = newMap->ktIndex[2];
            oldMap->kt_index[3] = newMap->ktIndex[3];
            oldMap->group_info = newMap->groupInfo;
            oldMap->width = newMap->width;
            oldMap->offset = offset;
            if (offset + newMap->nSyms >= map->size_syms) {
                register int sz;

                sz = map->size_syms + 128;
                _XkbResizeArray(map->syms, map->size_syms, sz, KeySym);
                if (map->syms == NULL) {
                    map->size_syms = 0;
                    return BadAlloc;
                }
                map->size_syms = sz;
            }
            if (newMap->nSyms > 0) {
                _XkbReadBufferCopyKeySyms(buf, (KeySym *) &map->syms[offset],
                                          newMap->nSyms);
                offset += newMap->nSyms;
            }
            else {
                map->syms[offset] = 0;
            }
        }
        map->num_syms = offset;
    }
    else {
        XkbSymMapPtr oldMap = &map->key_sym_map[rep->firstKeySym];

        for (i = 0; i < (int) rep->nKeySyms; i++, oldMap++) {
            xkbSymMapWireDesc *newMap;
            KeySym *newSyms;
            int tmp;

            newMap = (xkbSymMapWireDesc *)
                _XkbGetReadBufferPtr(buf, SIZEOF(xkbSymMapWireDesc));
            if (newMap == NULL)
                return BadLength;

            if (newMap->nSyms > 0)
                tmp = newMap->nSyms;
            else
                tmp = 0;

            newSyms = XkbResizeKeySyms(xkb, i + rep->firstKeySym, tmp);
            if (newSyms == NULL)
                return BadAlloc;
            if (newMap->nSyms > 0)
                _XkbReadBufferCopyKeySyms(buf, newSyms, newMap->nSyms);
            else
                newSyms[0] = NoSymbol;
            oldMap->kt_index[0] = newMap->ktIndex[0];
            oldMap->kt_index[1] = newMap->ktIndex[1];
            oldMap->kt_index[2] = newMap->ktIndex[2];
            oldMap->kt_index[3] = newMap->ktIndex[3];
            oldMap->group_info = newMap->groupInfo;
            oldMap->width = newMap->width;
        }
    }
    return Success;
}

static Status
_XkbReadKeyActions(XkbReadBufferPtr buf, XkbDescPtr info, xkbGetMapReply *rep)
{
    int i;
    CARD8 numDescBuf[248];
    CARD8 *numDesc = NULL;
    register int nKeyActs;
    Status ret = Success;

    if ((nKeyActs = rep->nKeyActs) > 0) {
        XkbSymMapPtr symMap;

        if (nKeyActs < sizeof numDescBuf)
            numDesc = numDescBuf;
        else
            numDesc = Xmallocarray(nKeyActs, sizeof(CARD8));

        if (!_XkbCopyFromReadBuffer(buf, (char *) numDesc, nKeyActs)) {
            ret = BadLength;
            goto done;
        }
        i = XkbPaddedSize(nKeyActs) - nKeyActs;
        if ((i > 0) && (!_XkbSkipReadBufferData(buf, i))) {
            ret = BadLength;
            goto done;
        }
        symMap = &info->map->key_sym_map[rep->firstKeyAct];
        for (i = 0; i < (int) rep->nKeyActs; i++, symMap++) {
            if (numDesc[i] == 0) {
                if ((i + rep->firstKeyAct) > (info->max_key_code + 1)) {
                    ret = BadLength;
                    goto done;
                }
                info->server->key_acts[i + rep->firstKeyAct] = 0;
            }
            else {
                XkbAction *newActs;

                /* 8/16/93 (ef) -- XXX! Verify size here (numdesc must be */
                /*                 either zero or XkbKeyNumSyms(info,key) */
                newActs = XkbResizeKeyActions(info, i + rep->firstKeyAct,
                                              numDesc[i]);
                if (newActs == NULL) {
                    ret = BadAlloc;
                    goto done;
                }
                if (!_XkbCopyFromReadBuffer(buf, (char *) newActs,
                            (int) (numDesc[i] * sizeof(XkbAction)))) {
                    ret = BadLength;
                    goto done;
                }
            }
        }
    }
 done:
    if (numDesc != NULL && numDesc != numDescBuf)
        Xfree(numDesc);
    return ret;
}

static Status
_XkbReadKeyBehaviors(XkbReadBufferPtr buf, XkbDescPtr xkb, xkbGetMapReply *rep)
{
    register int i;

    if (rep->totalKeyBehaviors > 0) {
        int size = xkb->max_key_code + 1;

        if (((int) rep->firstKeyBehavior + rep->nKeyBehaviors) > size)
            return BadLength;
        if (xkb->server->behaviors == NULL) {
            xkb->server->behaviors = _XkbTypedCalloc(size, XkbBehavior);
            if (xkb->server->behaviors == NULL)
                return BadAlloc;
        }
        else {
            bzero(&xkb->server->behaviors[rep->firstKeyBehavior],
                  (rep->nKeyBehaviors * sizeof(XkbBehavior)));
        }
        for (i = 0; i < rep->totalKeyBehaviors; i++) {
            xkbBehaviorWireDesc *wire;

            wire = (xkbBehaviorWireDesc *) _XkbGetReadBufferPtr(buf,
                                               SIZEOF(xkbBehaviorWireDesc));
            if (wire == NULL || wire->key >= size)
                return BadLength;
            xkb->server->behaviors[wire->key].type = wire->type;
            xkb->server->behaviors[wire->key].data = wire->data;
        }
    }
    return Success;
}

static Status
_XkbReadVirtualMods(XkbReadBufferPtr buf, XkbDescPtr xkb, xkbGetMapReply *rep)
{
    if (rep->virtualMods) {
        register int i, bit, nVMods;
        register char *data;

        for (i = nVMods = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
            if (rep->virtualMods & bit)
                nVMods++;
        }
        data = _XkbGetReadBufferPtr(buf, XkbPaddedSize(nVMods));
        if (data == NULL)
            return BadLength;
        for (i = 0, bit = 1; (i < XkbNumVirtualMods) && (nVMods > 0);
             i++, bit <<= 1) {
            if (rep->virtualMods & bit) {
                xkb->server->vmods[i] = *data++;
                nVMods--;
            }
        }
    }
    return Success;
}

static Status
_XkbReadExplicitComponents(XkbReadBufferPtr buf,
                           XkbDescPtr xkb,
                           xkbGetMapReply *rep)
{
    register int i;
    unsigned char *wire;

    if (rep->totalKeyExplicit > 0) {
        int size = xkb->max_key_code + 1;

        if (((int) rep->firstKeyExplicit + rep->nKeyExplicit) > size)
            return BadLength;
        if (xkb->server->explicit == NULL) {
            xkb->server->explicit = _XkbTypedCalloc(size, unsigned char);

            if (xkb->server->explicit == NULL)
                return BadAlloc;
        }
        else {
            bzero(&xkb->server->explicit[rep->firstKeyExplicit],
                  rep->nKeyExplicit);
        }
        i = XkbPaddedSize(2 * rep->totalKeyExplicit);
        wire = (unsigned char *) _XkbGetReadBufferPtr(buf, i);
        if (!wire)
            return BadLength;
        for (i = 0; i < rep->totalKeyExplicit; i++, wire += 2) {
            if (wire[0] > xkb->max_key_code || wire[1] > xkb->max_key_code)
                return BadLength;
            xkb->server->explicit[wire[0]] = wire[1];
        }
    }
    return Success;
}

static Status
_XkbReadModifierMap(XkbReadBufferPtr buf, XkbDescPtr xkb, xkbGetMapReply *rep)
{
    register int i;
    unsigned char *wire;

    if (rep->totalModMapKeys > 0) {
        if (((int) rep->firstModMapKey + rep->nModMapKeys) >
            (xkb->max_key_code + 1))
            return BadLength;
        if ((xkb->map->modmap == NULL) &&
            (XkbAllocClientMap(xkb, XkbModifierMapMask, 0) != Success)) {
            return BadAlloc;
        }
        else {
            bzero(&xkb->map->modmap[rep->firstModMapKey], rep->nModMapKeys);
        }
        i = XkbPaddedSize(2 * rep->totalModMapKeys);
        wire = (unsigned char *) _XkbGetReadBufferPtr(buf, i);
        if (!wire)
            return BadLength;
        for (i = 0; i < rep->totalModMapKeys; i++, wire += 2) {
            if (wire[0] > xkb->max_key_code || wire[1] > xkb->max_key_code)
                return BadLength;
            xkb->map->modmap[wire[0]] = wire[1];
        }
    }
    return Success;
}

static Status
_XkbReadVirtualModMap(XkbReadBufferPtr buf,
                      XkbDescPtr xkb,
                      xkbGetMapReply *rep)
{
    register int i;
    xkbVModMapWireDesc *wire;
    XkbServerMapPtr srv;

    if (rep->totalVModMapKeys > 0) {
        if (((int) rep->firstVModMapKey + rep->nVModMapKeys)
            > xkb->max_key_code + 1)
            return BadLength;
        if (((xkb->server == NULL) || (xkb->server->vmodmap == NULL)) &&
            (XkbAllocServerMap(xkb, XkbVirtualModMapMask, 0) != Success)) {
            return BadAlloc;
        }
        else {
            srv = xkb->server;
            if (rep->nVModMapKeys > rep->firstVModMapKey)
                bzero((char *) &srv->vmodmap[rep->firstVModMapKey],
                      (rep->nVModMapKeys - rep->firstVModMapKey) *
                      sizeof(unsigned short));
        }
        srv = xkb->server;
        i = rep->totalVModMapKeys * SIZEOF(xkbVModMapWireDesc);
        wire = (xkbVModMapWireDesc *) _XkbGetReadBufferPtr(buf, i);
        if (!wire)
            return BadLength;
        for (i = 0; i < rep->totalVModMapKeys; i++, wire++) {
            if ((wire->key >= xkb->min_key_code) &&
                (wire->key <= xkb->max_key_code))
                srv->vmodmap[wire->key] = wire->vmods;
        }
    }
    return Success;
}

static xkbGetMapReq *
_XkbGetGetMapReq(Display *dpy, XkbDescPtr xkb)
{
    xkbGetMapReq *req;

    GetReq(kbGetMap, req);
    req->reqType = dpy->xkb_info->codes->major_opcode;
    req->xkbReqType = X_kbGetMap;
    req->deviceSpec = xkb->device_spec;
    req->full = req->partial = 0;
    req->firstType = req->nTypes = 0;
    req->firstKeySym = req->nKeySyms = 0;
    req->firstKeyAct = req->nKeyActs = 0;
    req->firstKeyBehavior = req->nKeyBehaviors = 0;
    req->virtualMods = 0;
    req->firstKeyExplicit = req->nKeyExplicit = 0;
    req->firstModMapKey = req->nModMapKeys = 0;
    req->firstVModMapKey = req->nVModMapKeys = 0;
    return req;
}

Status
_XkbReadGetMapReply(Display *dpy,
                    xkbGetMapReply *rep,
                    XkbDescPtr xkb,
                    int *nread_rtrn)
{
    int extraData;
    unsigned mask;

    if (xkb->device_spec == XkbUseCoreKbd)
        xkb->device_spec = rep->deviceID;
    if (rep->maxKeyCode < rep->minKeyCode)
        return BadImplementation;
    xkb->min_key_code = rep->minKeyCode;
    xkb->max_key_code = rep->maxKeyCode;

    if (!xkb->map) {
        mask = rep->present & XkbAllClientInfoMask;
        if (mask && (XkbAllocClientMap(xkb, mask, rep->nTypes) != Success))
            return BadAlloc;
    }
    if (!xkb->server) {
        mask = rep->present & XkbAllServerInfoMask;
        if (mask && (XkbAllocServerMap(xkb, mask, rep->totalActs) != Success))
            return BadAlloc;
    }
    extraData = (int) (rep->length * 4);
    extraData -= (SIZEOF(xkbGetMapReply) - SIZEOF(xGenericReply));
    if (rep->length) {
        XkbReadBufferRec buf;
        int left;

        if (_XkbInitReadBuffer(dpy, &buf, extraData)) {
            Status status = Success;

            if (nread_rtrn != NULL)
                *nread_rtrn = extraData;
            if (status == Success)
                status = _XkbReadKeyTypes(&buf, xkb, rep);
            if (status == Success)
                status = _XkbReadKeySyms(&buf, xkb, rep);
            if (status == Success)
                status = _XkbReadKeyActions(&buf, xkb, rep);
            if (status == Success)
                status = _XkbReadKeyBehaviors(&buf, xkb, rep);
            if (status == Success)
                status = _XkbReadVirtualMods(&buf, xkb, rep);
            if (status == Success)
                status = _XkbReadExplicitComponents(&buf, xkb, rep);
            if (status == Success)
                status = _XkbReadModifierMap(&buf, xkb, rep);
            if (status == Success)
                status = _XkbReadVirtualModMap(&buf, xkb, rep);
            left = _XkbFreeReadBuffer(&buf);
            if (status != Success)
                return status;
            else if (left || buf.error)
                return BadLength;
        }
        else
            return BadAlloc;
    }
    return Success;
}

static Status
_XkbHandleGetMapReply(Display *dpy, XkbDescPtr xkb)
{
    xkbGetMapReply rep;

    if (!_XReply(dpy, (xReply *) &rep,
                 ((SIZEOF(xkbGetMapReply) - SIZEOF(xGenericReply)) >> 2),
                 xFalse)) {
        return BadImplementation;
    }
    return _XkbReadGetMapReply(dpy, &rep, xkb, NULL);
}

Status
XkbGetUpdatedMap(Display *dpy, unsigned which, XkbDescPtr xkb)
{
    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;
    if (which) {
        register xkbGetMapReq *req;
        Status status;

        LockDisplay(dpy);

        req = _XkbGetGetMapReq(dpy, xkb);
        req->full = which;
        status = _XkbHandleGetMapReply(dpy, xkb);

        UnlockDisplay(dpy);
        SyncHandle();
        return status;
    }
    return Success;
}

XkbDescPtr
XkbGetMap(Display *dpy, unsigned which, unsigned deviceSpec)
{
    XkbDescPtr xkb;

    xkb = _XkbTypedCalloc(1, XkbDescRec);
    if (xkb) {
        xkb->device_spec = deviceSpec;
        xkb->map = _XkbTypedCalloc(1, XkbClientMapRec);
        if ((xkb->map == NULL) ||
            ((which) && (XkbGetUpdatedMap(dpy, which, xkb) != Success))) {
            if (xkb->map) {
                Xfree(xkb->map);
                xkb->map = NULL;
            }
            Xfree(xkb);
            return NULL;
        }
        xkb->dpy = dpy;
    }
    return xkb;
}

Status
XkbGetKeyTypes(Display *dpy, unsigned first, unsigned num, XkbDescPtr xkb)
{
    register xkbGetMapReq *req;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;
    if ((num < 1) || (num > XkbMaxKeyTypes))
        return BadValue;

    LockDisplay(dpy);

    req = _XkbGetGetMapReq(dpy, xkb);
    req->firstType = first;
    req->nTypes = num;
    status = _XkbHandleGetMapReply(dpy, xkb);

    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

Status
XkbGetKeyActions(Display *dpy, unsigned first, unsigned num, XkbDescPtr xkb)
{
    register xkbGetMapReq *req;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;

    if ((num < 1) || (num > XkbMaxKeyCount))
        return BadValue;

    LockDisplay(dpy);

    req = _XkbGetGetMapReq(dpy, xkb);
    req->firstKeyAct = first;
    req->nKeyActs = num;
    status = _XkbHandleGetMapReply(dpy, xkb);

    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

Status
XkbGetKeySyms(Display *dpy, unsigned first, unsigned num, XkbDescPtr xkb)
{
    register xkbGetMapReq *req;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;

    if ((num < 1) || (num > XkbMaxKeyCount))
        return BadValue;

    LockDisplay(dpy);

    req = _XkbGetGetMapReq(dpy, xkb);
    req->firstKeySym = first;
    req->nKeySyms = num;
    status = _XkbHandleGetMapReply(dpy, xkb);

    UnlockDisplay(dpy);
    SyncHandle();

    return status;
}

Status
XkbGetKeyBehaviors(Display *dpy, unsigned first, unsigned num, XkbDescPtr xkb)
{
    register xkbGetMapReq *req;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;

    if ((num < 1) || (num > XkbMaxKeyCount))
        return BadValue;

    LockDisplay(dpy);

    req = _XkbGetGetMapReq(dpy, xkb);
    req->firstKeyBehavior = first;
    req->nKeyBehaviors = num;
    status = _XkbHandleGetMapReply(dpy, xkb);

    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

Status
XkbGetVirtualMods(Display *dpy, unsigned which, XkbDescPtr xkb)
{
    register xkbGetMapReq *req;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;

    LockDisplay(dpy);

    req = _XkbGetGetMapReq(dpy, xkb);
    req->virtualMods = which;
    status = _XkbHandleGetMapReply(dpy, xkb);

    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

Status
XkbGetKeyExplicitComponents(Display *dpy,
                            unsigned first,
                            unsigned num,
                            XkbDescPtr xkb)
{
    register xkbGetMapReq *req;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;

    if ((num < 1) || (num > XkbMaxKeyCount))
        return BadValue;

    LockDisplay(dpy);

    req = _XkbGetGetMapReq(dpy, xkb);
    req->firstKeyExplicit = first;
    req->nKeyExplicit = num;
    if ((xkb != NULL) && (xkb->server != NULL) &&
        (xkb->server->explicit != NULL)) {
        if ((num > 0) && (first >= xkb->min_key_code) &&
            (first + num <= xkb->max_key_code))
            bzero(&xkb->server->explicit[first], num);
    }
    if (xkb)
        status = _XkbHandleGetMapReply(dpy, xkb);
    else
        status = BadMatch;

    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

Status
XkbGetKeyModifierMap(Display *dpy,
                     unsigned first,
                     unsigned num,
                     XkbDescPtr xkb)
{
    register xkbGetMapReq *req;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;

    if ((num < 1) || (num > XkbMaxKeyCount))
        return BadValue;

    LockDisplay(dpy);

    req = _XkbGetGetMapReq(dpy, xkb);
    req->firstModMapKey = first;
    req->nModMapKeys = num;
    if ((xkb != NULL) && (xkb->map != NULL) && (xkb->map->modmap != NULL)) {
        if ((num > 0) && (first >= xkb->min_key_code) &&
            (first + num <= xkb->max_key_code))
            bzero(&xkb->map->modmap[first], num);
    }
    if (xkb)
        status = _XkbHandleGetMapReply(dpy, xkb);
    else
        status = BadMatch;

    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

Status
XkbGetKeyVirtualModMap(Display *dpy, unsigned first, unsigned num,
                       XkbDescPtr xkb)
{
    register xkbGetMapReq *req;
    Status status;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;

    if ((num < 1) || (num > XkbMaxKeyCount))
        return BadValue;

    LockDisplay(dpy);

    req = _XkbGetGetMapReq(dpy, xkb);
    req->firstVModMapKey = first;
    req->nVModMapKeys = num;
    if ((xkb != NULL) && (xkb->map != NULL) && (xkb->map->modmap != NULL)) {
        if ((num > 0) && (first >= xkb->min_key_code) &&
            (first + num <= xkb->max_key_code))
            bzero(&xkb->server->vmodmap[first], num * sizeof(unsigned short));
    }

    if (xkb)
        status = _XkbHandleGetMapReply(dpy, xkb);
    else
        status = BadMatch;

    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

Status
XkbGetMapChanges(Display *dpy, XkbDescPtr xkb, XkbMapChangesPtr changes)
{
    xkbGetMapReq *req;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;
    LockDisplay(dpy);
    if (changes->changed) {
        Status status = Success;

        req = _XkbGetGetMapReq(dpy, xkb);
        req->full = 0;
        req->partial = changes->changed;
        req->firstType = changes->first_type;
        req->nTypes = changes->num_types;
        req->firstKeySym = changes->first_key_sym;
        req->nKeySyms = changes->num_key_syms;
        req->firstKeyAct = changes->first_key_act;
        req->nKeyActs = changes->num_key_acts;
        req->firstKeyBehavior = changes->first_key_behavior;
        req->nKeyBehaviors = changes->num_key_behaviors;
        req->virtualMods = changes->vmods;
        req->firstKeyExplicit = changes->first_key_explicit;
        req->nKeyExplicit = changes->num_key_explicit;
        req->firstModMapKey = changes->first_modmap_key;
        req->nModMapKeys = changes->num_modmap_keys;
        req->firstVModMapKey = changes->first_vmodmap_key;
        req->nVModMapKeys = changes->num_vmodmap_keys;
        status = _XkbHandleGetMapReply(dpy, xkb);
        UnlockDisplay(dpy);
        SyncHandle();
        return status;
    }
    UnlockDisplay(dpy);
    return Success;
}
