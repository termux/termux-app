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

static int
_XkbSizeKeyTypes(XkbDescPtr xkb, xkbSetMapReq *req)
{
    XkbKeyTypePtr map;
    int i, len;

    if (((req->present & XkbKeyTypesMask) == 0) || (req->nTypes == 0)) {
        req->present &= ~XkbKeyTypesMask;
        req->firstType = req->nTypes = 0;
        return 0;
    }
    len = 0;
    map = &xkb->map->types[req->firstType];
    for (i = 0; i < req->nTypes; i++, map++) {
        len += SIZEOF(xkbKeyTypeWireDesc);
        len += map->map_count * SIZEOF(xkbKTSetMapEntryWireDesc);
        if (map->preserve)
            len += map->map_count * SIZEOF(xkbModsWireDesc);
    }
    return len;
}

static void
_XkbWriteKeyTypes(Display *dpy, XkbDescPtr xkb, xkbSetMapReq *req)
{
    char *buf;
    XkbKeyTypePtr type;
    int i, n;
    xkbKeyTypeWireDesc *desc;

    if ((req->present & XkbKeyTypesMask) == 0)
        return;
    type = &xkb->map->types[req->firstType];
    for (i = 0; i < req->nTypes; i++, type++) {
        int sz = SIZEOF(xkbKeyTypeWireDesc);
        sz += type->map_count * SIZEOF(xkbKTSetMapEntryWireDesc);
        if (type->preserve)
            sz += type->map_count * SIZEOF(xkbModsWireDesc);
        BufAlloc(xkbKeyTypeWireDesc *, desc, sz);
        desc->mask = type->mods.mask;
        desc->realMods = type->mods.real_mods;
        desc->virtualMods = type->mods.vmods;
        desc->numLevels = type->num_levels;
        desc->nMapEntries = type->map_count;
        desc->preserve = (type->preserve != NULL);
        buf = (char *) &desc[1];
        if (desc->nMapEntries > 0) {
            xkbKTSetMapEntryWireDesc *wire = (xkbKTSetMapEntryWireDesc *) buf;

            for (n = 0; n < type->map_count; n++, wire++) {
                wire->level = type->map[n].level;
                wire->realMods = type->map[n].mods.real_mods;
                wire->virtualMods = type->map[n].mods.vmods;
            }
            buf = (char *) wire;
            if (type->preserve) {
                xkbModsWireDesc *pwire = (xkbModsWireDesc *) buf;

                for (n = 0; n < type->map_count; n++, pwire++) {
                    pwire->realMods = type->preserve[n].real_mods;
                    pwire->virtualMods = type->preserve[n].vmods;
                }
            }
        }
    }
    return;
}

static int
_XkbSizeKeySyms(XkbDescPtr xkb, xkbSetMapReq *req)
{
    int i, len;
    unsigned nSyms;

    if (((req->present & XkbKeySymsMask) == 0) || (req->nKeySyms == 0)) {
        req->present &= ~XkbKeySymsMask;
        req->firstKeySym = req->nKeySyms = 0;
        req->totalSyms = 0;
        return 0;
    }
    len = (int) (req->nKeySyms * sizeof(XkbSymMapRec));
    for (i = nSyms = 0; i < req->nKeySyms; i++) {
        nSyms += XkbKeyNumSyms(xkb, i + req->firstKeySym);
    }
    len += nSyms * sizeof(CARD32);
    req->totalSyms = nSyms;
    return len;
}

static void
_XkbWriteKeySyms(Display *dpy, XkbDescPtr xkb, xkbSetMapReq *req)
{
    register KeySym *pSym;
    CARD32 *outSym;
    XkbSymMapPtr symMap;
    xkbSymMapWireDesc *desc;
    register int i;

    if ((req->present & XkbKeySymsMask) == 0)
        return;
    symMap = &xkb->map->key_sym_map[req->firstKeySym];
    for (i = 0; i < req->nKeySyms; i++, symMap++) {
        BufAlloc(xkbSymMapWireDesc *, desc,
                 SIZEOF(xkbSymMapWireDesc) +
                 (XkbKeyNumSyms(xkb, i + req->firstKeySym) * sizeof(CARD32)));
        desc->ktIndex[0] = symMap->kt_index[0];
        desc->ktIndex[1] = symMap->kt_index[1];
        desc->ktIndex[2] = symMap->kt_index[2];
        desc->ktIndex[3] = symMap->kt_index[3];
        desc->groupInfo = symMap->group_info;
        desc->width = symMap->width;
        desc->nSyms = XkbKeyNumSyms(xkb, i + req->firstKeySym);
        outSym = (CARD32 *) &desc[1];
        if (desc->nSyms > 0) {
            pSym = XkbKeySymsPtr(xkb, i + req->firstKeySym);
            _XkbWriteCopyKeySyms(pSym, outSym, desc->nSyms);
        }
    }
    return;
}

static int
_XkbSizeKeyActions(XkbDescPtr xkb, xkbSetMapReq *req)
{
    int i, len, nActs;

    if (((req->present & XkbKeyActionsMask) == 0) || (req->nKeyActs == 0)) {
        req->present &= ~XkbKeyActionsMask;
        req->firstKeyAct = req->nKeyActs = 0;
        req->totalActs = 0;
        return 0;
    }
    for (nActs = i = 0; i < req->nKeyActs; i++) {
        if (xkb->server->key_acts[i + req->firstKeyAct] != 0)
            nActs += XkbKeyNumActions(xkb, i + req->firstKeyAct);
    }
    len = XkbPaddedSize(req->nKeyActs) + (nActs * SIZEOF(xkbActionWireDesc));
    req->totalActs = nActs;
    return len;
}

static void
_XkbWriteKeyActions(Display *dpy, XkbDescPtr xkb, xkbSetMapReq *req)
{
    register int i;
    int n;
    CARD8 *numDesc;
    XkbAction *actDesc;

    if ((req->present & XkbKeyActionsMask) == 0)
        return;
    n = XkbPaddedSize(req->nKeyActs);
    n += (req->totalActs * SIZEOF(xkbActionWireDesc));

    BufAlloc(CARD8 *, numDesc, n);
    for (i = 0; i < req->nKeyActs; i++) {
        if (xkb->server->key_acts[i + req->firstKeyAct] == 0)
            numDesc[i] = 0;
        else
            numDesc[i] = XkbKeyNumActions(xkb, (i + req->firstKeyAct));
    }
    actDesc = (XkbAction *) &numDesc[XkbPaddedSize(req->nKeyActs)];
    for (i = 0; i < req->nKeyActs; i++) {
        if (xkb->server->key_acts[i + req->firstKeyAct] != 0) {
            n = XkbKeyNumActions(xkb, (i + req->firstKeyAct));
            memcpy(actDesc, XkbKeyActionsPtr(xkb, (i + req->firstKeyAct)),
                   n * SIZEOF(xkbActionWireDesc));
            actDesc += n;
        }
    }
    return;
}

static int
_XkbSizeKeyBehaviors(XkbDescPtr xkb, xkbSetMapReq *req)
{
    register int i, first, last, nFound;

    if (((req->present & XkbKeyBehaviorsMask) == 0) || (req->nKeyBehaviors < 1)) {
        req->present &= ~XkbKeyBehaviorsMask;
        req->firstKeyBehavior = req->nKeyBehaviors = 0;
        req->totalKeyBehaviors = 0;
        return 0;
    }
    first = req->firstKeyBehavior;
    last = first + req->nKeyBehaviors - 1;
    for (i = first, nFound = 0; i <= last; i++) {
        if (xkb->server->behaviors[i].type != XkbKB_Default)
            nFound++;
    }
    req->totalKeyBehaviors = nFound;
    return (nFound * SIZEOF(xkbBehaviorWireDesc));
}

static void
_XkbWriteKeyBehaviors(Display *dpy, XkbDescPtr xkb, xkbSetMapReq *req)
{
    register int i, first, last;
    xkbBehaviorWireDesc *wire;
    char *buf;

    if ((req->present & XkbKeyBehaviorsMask) == 0)
        return;
    first = req->firstKeyBehavior;
    last = first + req->nKeyBehaviors - 1;

    i = req->totalKeyBehaviors * SIZEOF(xkbBehaviorWireDesc);
    BufAlloc(char *, buf, i);
    wire = (xkbBehaviorWireDesc *) buf;
    for (i = first; i <= last; i++) {
        if (xkb->server->behaviors[i].type != XkbKB_Default) {
            wire->key = i;
            wire->type = xkb->server->behaviors[i].type;
            wire->data = xkb->server->behaviors[i].data;
            buf += SIZEOF(xkbBehaviorWireDesc);
            wire = (xkbBehaviorWireDesc *) buf;
        }
    }
    return;
}

static unsigned
_XkbSizeVirtualMods(xkbSetMapReq *req)
{
    register int i, bit, nMods;

    if (((req->present & XkbVirtualModsMask) == 0) || (req->virtualMods == 0)) {
        req->present &= ~XkbVirtualModsMask;
        req->virtualMods = 0;
        return 0;
    }
    for (i = nMods = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (req->virtualMods & bit)
            nMods++;
    }
    return XkbPaddedSize(nMods);
}

static void
_XkbWriteVirtualMods(Display *dpy,
                     XkbDescPtr xkb,
                     xkbSetMapReq *req,
                     unsigned size)
{
    register int i, bit;
    CARD8 *vmods;

    /* This was req->present&XkbVirtualModsMask==0, and '==' beats '&' */
    if (((req->present & XkbVirtualModsMask) == 0) || (size < 1))
        return;
    BufAlloc(CARD8 *, vmods, size);
    for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (req->virtualMods & bit)
            *vmods++ = xkb->server->vmods[i];
    }
    return;
}

static int
_XkbSizeKeyExplicit(XkbDescPtr xkb, xkbSetMapReq *req)
{
    register int i, first, last, nFound;

    if (((req->present & XkbExplicitComponentsMask) == 0) ||
        (req->nKeyExplicit == 0)) {
        req->present &= ~XkbExplicitComponentsMask;
        req->firstKeyExplicit = req->nKeyExplicit = 0;
        req->totalKeyExplicit = 0;
        return 0;
    }
    first = req->firstKeyExplicit;
    last = first + req->nKeyExplicit - 1;

    for (i = first, nFound = 0; i <= last; i++) {
        if (xkb->server->explicit[i] != 0)
            nFound++;
    }
    req->totalKeyExplicit = nFound;
    return XkbPaddedSize((nFound * 2));
}

static void
_XkbWriteKeyExplicit(Display *dpy, XkbDescPtr xkb, xkbSetMapReq *req)
{
    register int i, first, last;
    CARD8 *wire;

    if ((req->present & XkbExplicitComponentsMask) == 0)
        return;
    first = req->firstKeyExplicit;
    last = first + req->nKeyExplicit - 1;
    i = XkbPaddedSize((req->totalKeyExplicit * 2));
    BufAlloc(CARD8 *, wire, i);
    for (i = first; i <= last; i++) {
        if (xkb->server->explicit[i] != 0) {
            wire[0] = i;
            wire[1] = xkb->server->explicit[i];
            wire += 2;
        }
    }
    return;
}

static int
_XkbSizeModifierMap(XkbDescPtr xkb, xkbSetMapReq *req)
{
    register int i, first, last, nFound;

    if (((req->present & XkbModifierMapMask) == 0) || (req->nModMapKeys == 0)) {
        req->present &= ~XkbModifierMapMask;
        req->firstModMapKey = req->nModMapKeys = 0;
        req->totalModMapKeys = 0;
        return 0;
    }
    first = req->firstModMapKey;
    last = first + req->nModMapKeys - 1;

    for (i = first, nFound = 0; i <= last; i++) {
        if (xkb->map->modmap[i] != 0)
            nFound++;
    }
    req->totalModMapKeys = nFound;
    return XkbPaddedSize((nFound * 2));
}

static void
_XkbWriteModifierMap(Display *dpy, XkbDescPtr xkb, xkbSetMapReq *req)
{
    register int i, first, last;
    CARD8 *wire;

    if ((req->present & XkbModifierMapMask) == 0)
        return;
    first = req->firstModMapKey;
    last = first + req->nModMapKeys - 1;
    if (req->totalModMapKeys > 0) {
        i = XkbPaddedSize((req->totalModMapKeys * 2));
        BufAlloc(CARD8 *, wire, i);

        for (i = first; i <= last; i++) {
            if (xkb->map->modmap[i] != 0) {
                wire[0] = i;
                wire[1] = xkb->map->modmap[i];
                wire += 2;
            }
        }
    }
    return;
}

static int
_XkbSizeVirtualModMap(XkbDescPtr xkb, xkbSetMapReq *req)
{
    register int i, first, last, nFound;

    if (((req->present & XkbVirtualModMapMask) == 0) ||
        (req->nVModMapKeys == 0)) {
        req->present &= ~XkbVirtualModMapMask;
        req->firstVModMapKey = req->nVModMapKeys = 0;
        req->totalVModMapKeys = 0;
        return 0;
    }
    first = req->firstVModMapKey;
    last = first + req->nVModMapKeys - 1;

    for (i = first, nFound = 0; i <= last; i++) {
        if (xkb->server->vmodmap[i] != 0)
            nFound++;
    }
    req->totalVModMapKeys = nFound;
    return nFound * SIZEOF(xkbVModMapWireDesc);
}

static void
_XkbWriteVirtualModMap(Display *dpy, XkbDescPtr xkb, xkbSetMapReq *req)
{
    register int i, first, last;
    xkbVModMapWireDesc *wire;

    if ((req->present & XkbVirtualModMapMask) == 0)
        return;
    first = req->firstVModMapKey;
    last = first + req->nVModMapKeys - 1;
    if (req->totalVModMapKeys > 0) {
        i = req->totalVModMapKeys * SIZEOF(xkbVModMapWireDesc);
        BufAlloc(xkbVModMapWireDesc *, wire, i);
        for (i = first; i <= last; i++) {
            if (xkb->server->vmodmap[i] != 0) {
                wire->key = i;
                wire->vmods = xkb->server->vmodmap[i];
                wire++;
            }
        }
    }
    return;
}

static void
SendSetMap(Display *dpy, XkbDescPtr xkb, xkbSetMapReq *req)
{
    xkbSetMapReq tmp;
    unsigned szMods;

    req->length += _XkbSizeKeyTypes(xkb, req) / 4;
    req->length += _XkbSizeKeySyms(xkb, req) / 4;
    req->length += _XkbSizeKeyActions(xkb, req) / 4;
    req->length += _XkbSizeKeyBehaviors(xkb, req) / 4;
    szMods = _XkbSizeVirtualMods(req);
    req->length += szMods / 4;
    req->length += _XkbSizeKeyExplicit(xkb, req) / 4;
    req->length += _XkbSizeModifierMap(xkb, req) / 4;
    req->length += _XkbSizeVirtualModMap(xkb, req) / 4;

    tmp = *req;
    if (tmp.nTypes > 0)
        _XkbWriteKeyTypes(dpy, xkb, &tmp);
    if (tmp.nKeySyms > 0)
        _XkbWriteKeySyms(dpy, xkb, &tmp);
    if (tmp.nKeyActs)
        _XkbWriteKeyActions(dpy, xkb, &tmp);
    if (tmp.totalKeyBehaviors > 0)
        _XkbWriteKeyBehaviors(dpy, xkb, &tmp);
    if (tmp.virtualMods)
        _XkbWriteVirtualMods(dpy, xkb, &tmp, szMods);
    if (tmp.totalKeyExplicit > 0)
        _XkbWriteKeyExplicit(dpy, xkb, &tmp);
    if (tmp.totalModMapKeys > 0)
        _XkbWriteModifierMap(dpy, xkb, &tmp);
    if (tmp.totalVModMapKeys > 0)
        _XkbWriteVirtualModMap(dpy, xkb, &tmp);
    return;
}

Bool
XkbSetMap(Display *dpy, unsigned which, XkbDescPtr xkb)
{
    register xkbSetMapReq *req;
    XkbInfoPtr xkbi;
    XkbServerMapPtr srv;
    XkbClientMapPtr map;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)) || (!xkb))
        return False;
    map = xkb->map;
    srv = xkb->server;

    if (((which & XkbKeyTypesMask) && ((!map) || (!map->types))) ||
        ((which & XkbKeySymsMask) &&
         ((!map) || (!map->syms) || (!map->key_sym_map))) ||
        ((which & XkbKeyActionsMask) && ((!srv) || (!srv->key_acts))) ||
        ((which & XkbKeyBehaviorsMask) && ((!srv) || (!srv->behaviors))) ||
        ((which & XkbVirtualModsMask) && (!srv)) ||
        ((which & XkbExplicitComponentsMask) && ((!srv) || (!srv->explicit))) ||
        ((which & XkbModifierMapMask) && ((!map) || (!map->modmap))) ||
        ((which & XkbVirtualModMapMask) && ((!srv) || (!srv->vmodmap))))
        return False;

    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetMap, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSetMap;
    req->deviceSpec = xkb->device_spec;
    req->present = which;
    req->flags = XkbSetMapAllFlags;
    req->minKeyCode = xkb->min_key_code;
    req->maxKeyCode = xkb->max_key_code;
    req->firstType = 0;
    if (which & XkbKeyTypesMask)
        req->nTypes = map->num_types;
    else
        req->nTypes = 0;
    if (which & XkbKeySymsMask) {
        req->firstKeySym = xkb->min_key_code;
        req->nKeySyms = XkbNumKeys(xkb);
    }
    if (which & XkbKeyActionsMask) {
        req->firstKeyAct = xkb->min_key_code;
        req->nKeyActs = XkbNumKeys(xkb);
    }
    if (which & XkbKeyBehaviorsMask) {
        req->firstKeyBehavior = xkb->min_key_code;
        req->nKeyBehaviors = XkbNumKeys(xkb);
    }
    if (which & XkbVirtualModsMask)
        req->virtualMods = ~0;
    if (which & XkbExplicitComponentsMask) {
        req->firstKeyExplicit = xkb->min_key_code;
        req->nKeyExplicit = XkbNumKeys(xkb);
    }
    if (which & XkbModifierMapMask) {
        req->firstModMapKey = xkb->min_key_code;
        req->nModMapKeys = XkbNumKeys(xkb);
    }
    if (which & XkbVirtualModMapMask) {
        req->firstVModMapKey = xkb->min_key_code;
        req->nVModMapKeys = XkbNumKeys(xkb);
    }
    SendSetMap(dpy, xkb, req);
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbChangeMap(Display *dpy, XkbDescPtr xkb, XkbMapChangesPtr changes)
{
    register xkbSetMapReq *req;
    XkbInfoPtr xkbi;
    XkbServerMapPtr srv;
    XkbClientMapPtr map;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)) ||
        (!xkb) || (!changes))
        return False;
    srv = xkb->server;
    map = xkb->map;

    if (((changes->changed & XkbKeyTypesMask) && ((!map) || (!map->types))) ||
        ((changes->changed & XkbKeySymsMask) && ((!map) || (!map->syms) ||
                                                 (!map->key_sym_map))) ||
        ((changes->changed & XkbKeyActionsMask) && ((!srv) || (!srv->key_acts)))
        || ((changes->changed & XkbKeyBehaviorsMask) &&
            ((!srv) || (!srv->behaviors))) ||
        ((changes->changed & XkbVirtualModsMask) && (!srv)) ||
        ((changes->changed & XkbExplicitComponentsMask) &&
         ((!srv) || (!srv->explicit))) ||
        ((changes->changed & XkbModifierMapMask) && ((!map) || (!map->modmap)))
        || ((changes->changed & XkbVirtualModMapMask) &&
            ((!srv) || (!srv->vmodmap))))
        return False;

    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetMap, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSetMap;
    req->deviceSpec = xkb->device_spec;
    req->present = changes->changed;
    req->flags = XkbSetMapRecomputeActions;
    req->minKeyCode = xkb->min_key_code;
    req->maxKeyCode = xkb->max_key_code;
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
    SendSetMap(dpy, xkb, req);
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}
