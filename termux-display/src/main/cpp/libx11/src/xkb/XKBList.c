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

#define	NEED_MAP_READERS
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"

/***====================================================================***/

static void
_FreeComponentNames(int num, XkbComponentNamePtr names)
{
    int i;
    XkbComponentNamePtr tmp;

    if ((num < 1) || (names == NULL))
        return;
    for (i = 0, tmp = names; i < num; i++, tmp++) {
        if (tmp->name) {
            _XkbFree(tmp->name);
            tmp->name = NULL;
        }
    }
    _XkbFree(names);
    return;
}

/***====================================================================***/

static XkbComponentNamePtr
_ReadListing(XkbReadBufferPtr buf, int count, Status * status_rtrn)
{
    XkbComponentNamePtr first, this;
    register int i;
    CARD16 *flags;
    int slen, wlen;
    char *str;

    if (count < 1)
        return NULL;
    first = _XkbTypedCalloc(count, XkbComponentNameRec);
    if (!first)
        return NULL;
    for (this = first, i = 0; i < count; i++, this++) {
        flags = (CARD16 *) _XkbGetReadBufferPtr(buf, 2 * sizeof(CARD16));
        if (!flags)
            goto BAILOUT;
        this->flags = flags[0];
        slen = flags[1];
        wlen = ((slen + 1) / 2) * 2;    /* pad to 2 byte boundary */
        this->name = _XkbTypedCalloc(slen + 1, char);

        if (!this->name)
            goto BAILOUT;
        str = (char *) _XkbGetReadBufferPtr(buf, wlen);
        if (!str)
            goto BAILOUT;
        memcpy(this->name, str, (size_t) slen);
    }
    return first;
 BAILOUT:
    *status_rtrn = BadAlloc;
    _FreeComponentNames(i, first);
    return NULL;
}

/***====================================================================***/

XkbComponentListPtr
XkbListComponents(Display *dpy,
                  unsigned deviceSpec,
                  XkbComponentNamesPtr ptrns,
                  int *max_inout)
{
    register xkbListComponentsReq *req;
    xkbListComponentsReply rep;
    XkbInfoPtr xkbi;
    XkbComponentListPtr list;
    XkbReadBufferRec buf;
    int left;
    char *str;
    int extraLen, len, mapLen, codesLen, typesLen, compatLen, symsLen, geomLen;

    if ((dpy == NULL) || (dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)) ||
        (ptrns == NULL) || (max_inout == NULL))
        return NULL;

    xkbi = dpy->xkb_info;
    LockDisplay(dpy);
    GetReq(kbListComponents, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbListComponents;
    req->deviceSpec = deviceSpec;
    req->maxNames = *max_inout;

    mapLen = codesLen = typesLen = compatLen = symsLen = geomLen = 0;
    if (ptrns->keymap)
        mapLen = (int) strlen(ptrns->keymap);
    if (ptrns->keycodes)
        codesLen = (int) strlen(ptrns->keycodes);
    if (ptrns->types)
        typesLen = (int) strlen(ptrns->types);
    if (ptrns->compat)
        compatLen = (int) strlen(ptrns->compat);
    if (ptrns->symbols)
        symsLen = (int) strlen(ptrns->symbols);
    if (ptrns->geometry)
        geomLen = (int) strlen(ptrns->geometry);
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

    len = mapLen + codesLen + typesLen + compatLen + symsLen + geomLen + 6;
    len = XkbPaddedSize(len);
    req->length += len / 4;
    BufAlloc(char *, str, len);

    *str++ = mapLen;
    if (mapLen > 0) {
        memcpy(str, ptrns->keymap, (size_t) mapLen);
        str += mapLen;
    }
    *str++ = codesLen;
    if (codesLen > 0) {
        memcpy(str, ptrns->keycodes, (size_t) codesLen);
        str += codesLen;
    }
    *str++ = typesLen;
    if (typesLen > 0) {
        memcpy(str, ptrns->types, (size_t) typesLen);
        str += typesLen;
    }
    *str++ = compatLen;
    if (compatLen > 0) {
        memcpy(str, ptrns->compat, (size_t) compatLen);
        str += compatLen;
    }
    *str++ = symsLen;
    if (symsLen > 0) {
        memcpy(str, ptrns->symbols, (size_t) symsLen);
        str += symsLen;
    }
    *str++ = geomLen;
    if (geomLen > 0) {
        memcpy(str, ptrns->geometry, (size_t) geomLen);
        str += geomLen;
    }
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse))
        goto BAILOUT;
    extraLen = (int) rep.length * 4;
    *max_inout = rep.extra;
    if (extraLen == 0) {        /* no matches, but we don't want to report a failure */
        list = _XkbTypedCalloc(1, XkbComponentListRec);
        UnlockDisplay(dpy);
        SyncHandle();
        return list;
    }
    if (_XkbInitReadBuffer(dpy, &buf, extraLen)) {
        Status status = Success;

        list = _XkbTypedCalloc(1, XkbComponentListRec);
        if (!list) {
            _XkbFreeReadBuffer(&buf);
            goto BAILOUT;
        }
        list->num_keymaps = rep.nKeymaps;
        list->num_keycodes = rep.nKeycodes;
        list->num_types = rep.nTypes;
        list->num_compat = rep.nCompatMaps;
        list->num_symbols = rep.nSymbols;
        list->num_geometry = rep.nGeometries;
        if ((status == Success) && (list->num_keymaps > 0))
            list->keymaps = _ReadListing(&buf, list->num_keymaps, &status);
        if ((status == Success) && (list->num_keycodes > 0))
            list->keycodes = _ReadListing(&buf, list->num_keycodes, &status);
        if ((status == Success) && (list->num_types > 0))
            list->types = _ReadListing(&buf, list->num_types, &status);
        if ((status == Success) && (list->num_compat > 0))
            list->compat = _ReadListing(&buf, list->num_compat, &status);
        if ((status == Success) && (list->num_symbols > 0))
            list->symbols = _ReadListing(&buf, list->num_symbols, &status);
        if ((status == Success) && (list->num_geometry > 0))
            list->geometry = _ReadListing(&buf, list->num_geometry, &status);
        left = _XkbFreeReadBuffer(&buf);
        if ((status != Success) || (buf.error) || (left > 2)) {
            XkbFreeComponentList(list);
            goto BAILOUT;
        }
        UnlockDisplay(dpy);
        SyncHandle();
        return list;
    }
 BAILOUT:
    UnlockDisplay(dpy);
    SyncHandle();
    return NULL;
}

void
XkbFreeComponentList(XkbComponentListPtr list)
{
    if (list) {
        if (list->keymaps)
            _FreeComponentNames(list->num_keymaps, list->keymaps);
        if (list->keycodes)
            _FreeComponentNames(list->num_keycodes, list->keycodes);
        if (list->types)
            _FreeComponentNames(list->num_types, list->types);
        if (list->compat)
            _FreeComponentNames(list->num_compat, list->compat);
        if (list->symbols)
            _FreeComponentNames(list->num_symbols, list->symbols);
        if (list->geometry)
            _FreeComponentNames(list->num_geometry, list->geometry);
        bzero((char *) list, sizeof(XkbComponentListRec));
        _XkbFree(list);
    }
    return;
}
