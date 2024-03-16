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

/***====================================================================***/

XkbDescPtr
XkbGetKeyboardByName(Display *dpy,
                     unsigned deviceSpec,
                     XkbComponentNamesPtr names,
                     unsigned want,
                     unsigned need,
                     Bool load)
{
    register xkbGetKbdByNameReq *req;
    xkbGetKbdByNameReply rep;
    int len, extraLen = 0;
    char *str;
    XkbDescPtr xkb;
    int mapLen, codesLen, typesLen, compatLen;
    int symsLen, geomLen;
    XkbInfoPtr xkbi;

    if ((dpy == NULL) || (dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return NULL;

    xkbi = dpy->xkb_info;
    xkb = (XkbDescRec *) _XkbCalloc(1, sizeof(XkbDescRec));
    if (!xkb)
        return NULL;
    xkb->device_spec = deviceSpec;
    xkb->map = (XkbClientMapRec *) _XkbCalloc(1, sizeof(XkbClientMapRec));
    xkb->dpy = dpy;

    LockDisplay(dpy);
    GetReq(kbGetKbdByName, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbGetKbdByName;
    req->deviceSpec = xkb->device_spec;
    req->want = want;
    req->need = need;
    req->load = load;

    mapLen = codesLen = typesLen = compatLen = symsLen = geomLen = 0;
    if (names) {
        if (names->keymap)
            mapLen = (int) strlen(names->keymap);
        if (names->keycodes)
            codesLen = (int) strlen(names->keycodes);
        if (names->types)
            typesLen = (int) strlen(names->types);
        if (names->compat)
            compatLen = (int) strlen(names->compat);
        if (names->symbols)
            symsLen = (int) strlen(names->symbols);
        if (names->geometry)
            geomLen = (int) strlen(names->geometry);
        if (mapLen > 255)
            mapLen = 255;
        if (codesLen > 255)
            codesLen = 255;
        if (typesLen > 255)
            typesLen = 255;
        if (compatLen > 255)
            compatLen = 255;
        if (symsLen > 255)
            symsLen = 255;
        if (geomLen > 255)
            geomLen = 255;
    }
    else
        mapLen = codesLen = typesLen = compatLen = symsLen = geomLen = 0;

    len = mapLen + codesLen + typesLen + compatLen + symsLen + geomLen + 6;
    len = XkbPaddedSize(len);
    req->length += len / 4;
    BufAlloc(char *, str, len);

    *str++ = mapLen;
    if (mapLen > 0) {
        memcpy(str, names->keymap, (size_t) mapLen);
        str += mapLen;
    }
    *str++ = codesLen;
    if (codesLen > 0) {
        memcpy(str, names->keycodes, (size_t) codesLen);
        str += codesLen;
    }
    *str++ = typesLen;
    if (typesLen > 0) {
        memcpy(str, names->types, (size_t) typesLen);
        str += typesLen;
    }
    *str++ = compatLen;
    if (compatLen > 0) {
        memcpy(str, names->compat, (size_t) compatLen);
        str += compatLen;
    }
    *str++ = symsLen;
    if (symsLen > 0) {
        memcpy(str, names->symbols, (size_t) symsLen);
        str += symsLen;
    }
    *str++ = geomLen;
    if (geomLen > 0) {
        memcpy(str, names->geometry, (size_t) geomLen);
        str += geomLen;
    }
    if ((!_XReply(dpy, (xReply *) &rep, 0, xFalse)) || (!rep.reported))
        goto BAILOUT;
    extraLen = (int) rep.length * 4;

    xkb->device_spec = rep.deviceID;
    xkb->min_key_code = rep.minKeyCode;
    xkb->max_key_code = rep.maxKeyCode;
    if (rep.reported & (XkbGBN_SymbolsMask | XkbGBN_TypesMask)) {
        xkbGetMapReply mrep;
        Status status;
        int nread = 0;

        _XRead(dpy, (char *) &mrep, SIZEOF(xkbGetMapReply));
        extraLen -= SIZEOF(xkbGetMapReply);
        status = _XkbReadGetMapReply(dpy, &mrep, xkb, &nread);
        extraLen -= nread;
        if (status != Success)
            goto BAILOUT;
    }
    if (rep.reported & XkbGBN_CompatMapMask) {
        xkbGetCompatMapReply crep;
        Status status;
        int nread = 0;

        _XRead(dpy, (char *) &crep, SIZEOF(xkbGetCompatMapReply));
        extraLen -= SIZEOF(xkbGetCompatMapReply);
        status = _XkbReadGetCompatMapReply(dpy, &crep, xkb, &nread);
        extraLen -= nread;
        if (status != Success)
            goto BAILOUT;
    }
    if (rep.reported & XkbGBN_IndicatorMapMask) {
        xkbGetIndicatorMapReply irep;
        Status status;
        int nread = 0;

        _XRead(dpy, (char *) &irep, SIZEOF(xkbGetIndicatorMapReply));
        extraLen -= SIZEOF(xkbGetIndicatorMapReply);
        status = _XkbReadGetIndicatorMapReply(dpy, &irep, xkb, &nread);
        extraLen -= nread;
        if (status != Success)
            goto BAILOUT;
    }
    if (rep.reported & (XkbGBN_KeyNamesMask | XkbGBN_OtherNamesMask)) {
        xkbGetNamesReply nrep;
        Status status;
        int nread = 0;

        _XRead(dpy, (char *) &nrep, SIZEOF(xkbGetNamesReply));
        extraLen -= SIZEOF(xkbGetNamesReply);
        status = _XkbReadGetNamesReply(dpy, &nrep, xkb, &nread);
        extraLen -= nread;
        if (status != Success)
            goto BAILOUT;
    }
    if (rep.reported & XkbGBN_GeometryMask) {
        xkbGetGeometryReply grep;
        Status status;
        int nread = 0;

        _XRead(dpy, (char *) &grep, SIZEOF(xkbGetGeometryReply));
        extraLen -= SIZEOF(xkbGetGeometryReply);
        status = _XkbReadGetGeometryReply(dpy, &grep, xkb, &nread);
        extraLen -= nread;
        if (status != Success)
            goto BAILOUT;
    }
    if (extraLen > 0)
        goto BAILOUT;
    UnlockDisplay(dpy);
    SyncHandle();
    return xkb;
 BAILOUT:
    if (xkb != NULL)
        XkbFreeKeyboard(xkb, XkbAllComponentsMask, xTrue);
    if (extraLen > 0)
        _XEatData(dpy, extraLen);
    UnlockDisplay(dpy);
    SyncHandle();
    return NULL;
}

XkbDescPtr
XkbGetKeyboard(Display *dpy, unsigned which, unsigned deviceSpec)
{
    return XkbGetKeyboardByName(dpy, deviceSpec, NULL, which, which, False);
}
