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
#define	NEED_MAP_READERS
#include "Xlibint.h"
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"

Status
_XkbReadGetCompatMapReply(Display *dpy,
                          xkbGetCompatMapReply *rep,
                          XkbDescPtr xkb,
                          int *nread_rtrn)
{
    register int i;
    XkbReadBufferRec buf;

    if (!_XkbInitReadBuffer(dpy, &buf, (int) rep->length * 4))
        return BadAlloc;

    if (nread_rtrn)
        *nread_rtrn = (int) rep->length * 4;

    i = rep->firstSI + rep->nSI;
    if ((!xkb->compat) &&
        (XkbAllocCompatMap(xkb, XkbAllCompatMask, i) != Success))
        return BadAlloc;

    if (rep->nSI != 0) {
        XkbSymInterpretRec *syms;
        xkbSymInterpretWireDesc *wire;

        wire = (xkbSymInterpretWireDesc *) _XkbGetReadBufferPtr(&buf,
            rep->nSI * SIZEOF (xkbSymInterpretWireDesc));
        if (wire == NULL)
            goto BAILOUT;
        syms = &xkb->compat->sym_interpret[rep->firstSI];

        for (i = 0; i < rep->nSI; i++, syms++, wire++) {
            syms->sym = wire->sym;
            syms->mods = wire->mods;
            syms->match = wire->match;
            syms->virtual_mod = wire->virtualMod;
            syms->flags = wire->flags;
            syms->act = *((XkbAnyAction *) &wire->act);
        }
        xkb->compat->num_si += rep->nSI;
    }

    if (rep->groups & XkbAllGroupsMask) {
        register unsigned bit, nGroups;
        xkbModsWireDesc *wire;

        for (i = 0, nGroups = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
            if (rep->groups & bit)
                nGroups++;
        }
        wire = (xkbModsWireDesc *)
            _XkbGetReadBufferPtr(&buf, nGroups * SIZEOF(xkbModsWireDesc));
        if (wire == NULL)
            goto BAILOUT;
        for (i = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
            if ((rep->groups & bit) == 0)
                continue;
            xkb->compat->groups[i].mask = wire->mask;
            xkb->compat->groups[i].real_mods = wire->realMods;
            xkb->compat->groups[i].vmods = wire->virtualMods;
            wire++;
        }
    }
    i = _XkbFreeReadBuffer(&buf);
    if (i)
        fprintf(stderr, "CompatMapReply! Bad length (%d extra bytes)\n", i);
    if (i || buf.error)
        return BadLength;
    return Success;
 BAILOUT:
    _XkbFreeReadBuffer(&buf);
    return BadLength;
}

Status
XkbGetCompatMap(Display *dpy, unsigned which, XkbDescPtr xkb)
{
    register xkbGetCompatMapReq *req;
    xkbGetCompatMapReply rep;
    Status status;
    XkbInfoPtr xkbi;

    if ((!dpy) || (!xkb) || (dpy->flags & XlibDisplayNoXkb) ||
        ((xkb->dpy != NULL) && (xkb->dpy != dpy)) ||
        (!dpy->xkb_info && (!XkbUseExtension(dpy, NULL, NULL))))
        return BadAccess;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbGetCompatMap, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbGetCompatMap;
    req->deviceSpec = xkb->device_spec;
    if (which & XkbSymInterpMask)
        req->getAllSI = True;
    else
        req->getAllSI = False;
    req->firstSI = req->nSI = 0;

    if (which & XkbGroupCompatMask)
        req->groups = XkbAllGroupsMask;
    else
        req->groups = 0;

    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return BadLength;
    }
    if (xkb->dpy == NULL)
        xkb->dpy = dpy;
    if (xkb->device_spec == XkbUseCoreKbd)
        xkb->device_spec = rep.deviceID;

    status = _XkbReadGetCompatMapReply(dpy, &rep, xkb, NULL);
    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

static Bool
_XkbWriteSetCompatMap(Display *dpy, xkbSetCompatMapReq *req, XkbDescPtr xkb)
{
    CARD16 firstSI;
    CARD16 nSI;
    int size;
    register int i, nGroups;
    register unsigned bit;
    unsigned groups;
    char *buf;

    firstSI = req->firstSI;
    nSI = req->nSI;
    size = nSI * SIZEOF(xkbSymInterpretWireDesc);
    nGroups = 0;
    groups = req->groups;
    if (groups & XkbAllGroupsMask) {
        for (i = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
            if (groups & bit)
                nGroups++;
        }
        size += SIZEOF(xkbModsWireDesc) * nGroups;
    }
    req->length += size / 4;
    BufAlloc(char *, buf, size);

    if (!buf)
        return False;

    if (nSI) {
        XkbSymInterpretPtr sym = &xkb->compat->sym_interpret[firstSI];
        xkbSymInterpretWireDesc *wire = (xkbSymInterpretWireDesc *) buf;

        for (i = 0; i < nSI; i++, wire++, sym++) {
            wire->sym = (CARD32) sym->sym;
            wire->mods = sym->mods;
            wire->match = sym->match;
            wire->flags = sym->flags;
            wire->virtualMod = sym->virtual_mod;
            memcpy(&wire->act, &sym->act, sz_xkbActionWireDesc);
        }
        buf += nSI * SIZEOF(xkbSymInterpretWireDesc);
    }
    if (groups & XkbAllGroupsMask) {
        xkbModsWireDesc *out = (xkbModsWireDesc *) buf;

        for (i = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
            if ((groups & bit) != 0) {
                out->mask = xkb->compat->groups[i].mask;
                out->realMods = xkb->compat->groups[i].real_mods;
                out->virtualMods = xkb->compat->groups[i].vmods;
                out++;
            }
        }
        buf += nGroups * SIZEOF(xkbModsWireDesc);
    }
    return True;
}

Bool
XkbSetCompatMap(Display *dpy, unsigned which, XkbDescPtr xkb,
                Bool updateActions)
{
    register xkbSetCompatMapReq *req;
    Status ok;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) || (dpy != xkb->dpy) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    if ((!xkb->compat) ||
        ((which & XkbSymInterpMask) && (!xkb->compat->sym_interpret)))
        return False;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetCompatMap, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSetCompatMap;
    req->deviceSpec = xkb->device_spec;
    req->recomputeActions = updateActions;
    if (which & XkbSymInterpMask) {
        req->truncateSI = True;
        req->firstSI = 0;
        req->nSI = xkb->compat->num_si;
    }
    else {
        req->truncateSI = False;
        req->firstSI = 0;
        req->nSI = 0;
    }
    if (which & XkbGroupCompatMask)
        req->groups = XkbAllGroupsMask;
    else
        req->groups = 0;
    ok = _XkbWriteSetCompatMap(dpy, req, xkb);
    UnlockDisplay(dpy);
    SyncHandle();
    return ok;
}
