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


static Status
_XkbReadAtoms(XkbReadBufferPtr buf,
              Atom *atoms,
              int maxAtoms,
              CARD32 present)
{
    register int i, bit;

    for (i = 0, bit = 1; (i < maxAtoms) && (present); i++, bit <<= 1) {
        if (present & bit) {
            if (!_XkbReadBufferCopy32(buf, (long *) &atoms[i], 1))
                return BadLength;
            present &= ~bit;
        }
    }
    return Success;
}

Status
_XkbReadGetNamesReply(Display *dpy,
                      xkbGetNamesReply *rep,
                      XkbDescPtr xkb,
                      int *nread_rtrn)
{
    int i, len;
    XkbReadBufferRec buf;
    register XkbNamesPtr names;

    if (xkb->device_spec == XkbUseCoreKbd)
        xkb->device_spec = rep->deviceID;

    if ((xkb->names == NULL) &&
        (XkbAllocNames(xkb, rep->which,
                       rep->nRadioGroups, rep->nKeyAliases) != Success)) {
        return BadAlloc;
    }
    names = xkb->names;
    if (rep->length == 0)
        return Success;

    if (!_XkbInitReadBuffer(dpy, &buf, (int) rep->length * 4))
        return BadAlloc;
    if (nread_rtrn)
        *nread_rtrn = (int) rep->length * 4;

    if ((rep->which & XkbKeycodesNameMask) &&
        (!_XkbReadBufferCopy32(&buf, (long *) &names->keycodes, 1)))
        goto BAILOUT;
    if ((rep->which & XkbGeometryNameMask) &&
        (!_XkbReadBufferCopy32(&buf, (long *) &names->geometry, 1)))
        goto BAILOUT;
    if ((rep->which & XkbSymbolsNameMask) &&
        (!_XkbReadBufferCopy32(&buf, (long *) &names->symbols, 1)))
        goto BAILOUT;
    if ((rep->which & XkbPhysSymbolsNameMask) &&
        (!_XkbReadBufferCopy32(&buf, (long *) &names->phys_symbols, 1)))
        goto BAILOUT;
    if ((rep->which & XkbTypesNameMask) &&
        (!_XkbReadBufferCopy32(&buf, (long *) &names->types, 1)))
        goto BAILOUT;
    if ((rep->which & XkbCompatNameMask) &&
        (!_XkbReadBufferCopy32(&buf, (long *) &names->compat, 1)))
        goto BAILOUT;

    if (rep->which & XkbKeyTypeNamesMask) {
        XkbClientMapPtr map = xkb->map;
        XkbKeyTypePtr type;

        len = rep->nTypes * 4;
        if (map != NULL) {
            type = map->types;
            for (i = 0; (i < map->num_types) && (i < rep->nTypes); i++, type++) {
                if (!_XkbReadBufferCopy32(&buf, (long *) &type->name, 1))
                    goto BAILOUT;
                len -= 4;
            }
        }
        if ((len > 0) && (!_XkbSkipReadBufferData(&buf, len)))
            goto BAILOUT;
    }
    if (rep->which & XkbKTLevelNamesMask) {
        CARD8 *nLevels;
        XkbClientMapPtr map = xkb->map;

        nLevels =
            (CARD8 *) _XkbGetReadBufferPtr(&buf, XkbPaddedSize(rep->nTypes));
        if (nLevels == NULL)
            goto BAILOUT;
        if (map != NULL) {
            XkbKeyTypePtr type = map->types;

            for (i = 0; i < (int) rep->nTypes; i++, type++) {
                if (i >= map->num_types) {
                    if (!_XkbSkipReadBufferData(&buf, nLevels[i] * 4))
                        goto BAILOUT;
                    continue;
                }
                if ((nLevels[i] > 0) && (nLevels[i] != type->num_levels)) {
                    goto BAILOUT;
                }

                Xfree(type->level_names);
                if (nLevels[i] == 0) {
                    type->level_names = NULL;
                    continue;
                }
                type->level_names = _XkbTypedCalloc(nLevels[i], Atom);
                if (type->level_names != NULL) {
                    if (!_XkbReadBufferCopy32(&buf, (long *) type->level_names,
                                              nLevels[i]))
                        goto BAILOUT;
                }
                else {
                    _XkbSkipReadBufferData(&buf, nLevels[i] * 4);
                }
            }
        }
        else {
            for (i = 0; i < (int) rep->nTypes; i++) {
                _XkbSkipReadBufferData(&buf, nLevels[i] * 4);
            }
        }
    }
    if (rep->which & XkbIndicatorNamesMask) {
        if (_XkbReadAtoms(&buf, names->indicators, XkbNumIndicators,
                          rep->indicators) != Success)
            goto BAILOUT;
    }
    if (rep->which & XkbVirtualModNamesMask) {
        if (_XkbReadAtoms(&buf, names->vmods, XkbNumVirtualMods,
                          (CARD32) rep->virtualMods) != Success)
            goto BAILOUT;
    }
    if (rep->which & XkbGroupNamesMask) {
        if (_XkbReadAtoms(&buf, names->groups, XkbNumKbdGroups,
                          (CARD32) rep->groupNames) != Success)
            goto BAILOUT;
    }
    if (rep->which & XkbKeyNamesMask) {
        if (names->keys == NULL) {
            int nKeys;

            if (xkb->max_key_code == 0) {
                xkb->min_key_code = rep->minKeyCode;
                xkb->max_key_code = rep->maxKeyCode;
            }
            nKeys = xkb->max_key_code + 1;
            names->keys = _XkbTypedCalloc(nKeys, XkbKeyNameRec);
        }
        if (((int) rep->firstKey + rep->nKeys) > xkb->max_key_code + 1)
            goto BAILOUT;
        if (names->keys != NULL) {
            if (!_XkbCopyFromReadBuffer(&buf,
                                        (char *) &names->keys[rep->firstKey],
                                        rep->nKeys * XkbKeyNameLength))
                goto BAILOUT;
            names->num_keys = rep->nKeys;
        }
        else
            _XkbSkipReadBufferData(&buf, rep->nKeys * XkbKeyNameLength);
    }
    if (rep->which & XkbKeyAliasesMask && (rep->nKeyAliases > 0)) {
        if (XkbAllocNames(xkb, XkbKeyAliasesMask, 0, rep->nKeyAliases) !=
            Success)
            goto BAILOUT;
        if (!_XkbCopyFromReadBuffer(&buf, (char *) names->key_aliases,
                                    rep->nKeyAliases * XkbKeyNameLength * 2))
            goto BAILOUT;
    }
    if (rep->which & XkbRGNamesMask) {
        if (rep->nRadioGroups > 0) {
            Atom *rgNames;

            if ((names->radio_groups == NULL) ||
                (names->num_rg < rep->nRadioGroups)) {
                _XkbResizeArray(names->radio_groups, names->num_rg,
                                rep->nRadioGroups, Atom);
            }
            rgNames = names->radio_groups;
            if (!rgNames) {
                names->num_rg = 0;
                goto BAILOUT;
            }
            if (!_XkbReadBufferCopy32
                (&buf, (long *) rgNames, rep->nRadioGroups))
                goto BAILOUT;
            names->num_rg = rep->nRadioGroups;
        }
        else if (names->num_rg > 0) {
            names->num_rg = 0;
            Xfree(names->radio_groups);
        }
    }
    len = _XkbFreeReadBuffer(&buf);
    if (len != 0)
        return BadLength;
    else
        return Success;
 BAILOUT:
    _XkbFreeReadBuffer(&buf);
    return BadLength;
}

Status
XkbGetNames(Display *dpy, unsigned which, XkbDescPtr xkb)
{
    register xkbGetNamesReq *req;
    xkbGetNamesReply rep;
    Status status;
    XkbInfoPtr xkbi;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    if (!xkb->names) {
        xkb->names = _XkbTypedCalloc(1, XkbNamesRec);
        if (!xkb->names) {
            UnlockDisplay(dpy);
            SyncHandle();
            return BadAlloc;
        }
    }
    GetReq(kbGetNames, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbGetNames;
    req->deviceSpec = xkb->device_spec;
    req->which = which;
    if (!_XReply(dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return BadImplementation;
    }

    status = _XkbReadGetNamesReply(dpy, &rep, xkb, NULL);
    UnlockDisplay(dpy);
    SyncHandle();
    return status;
}

/***====================================================================***/

static int
_XkbCountBits(int nBitsMax, unsigned long mask)
{
    register unsigned long y, nBits;

    y = (mask >> 1) & 033333333333;
    y = mask - y - ((y >> 1) & 033333333333);
    nBits = ((unsigned int) (((y + (y >> 3)) & 030707070707) % 077));

    /* nBitsMax really means max+1 */
    return (nBits < nBitsMax) ? nBits : (nBitsMax - 1);
}

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

static void
_XkbCopyAtoms(Display *dpy, Atom *atoms, CARD32 mask, int maxAtoms)
{
    register unsigned int i, bit;

    for (i = 0, bit = 1; i < maxAtoms; i++, bit <<= 1) {
        if (mask & bit)
            Data32(dpy, &atoms[i], 4);
    }
    return;
}

Bool
XkbSetNames(Display *dpy,
            unsigned int which,
            unsigned int firstType,
            unsigned int nTypes,
            XkbDescPtr xkb)
{
    register xkbSetNamesReq *req;
    int nLvlNames = 0;
    XkbInfoPtr xkbi;
    XkbNamesPtr names;
    unsigned firstLvlType, nLvlTypes;
    int nVMods, nLEDs, nRG, nKA, nGroups;
    int nKeys = 0, firstKey = 0, nAtoms;
    CARD32 leds, vmods, groups;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    if ((!xkb) || (!xkb->names))
        return False;
    firstLvlType = firstType;
    nLvlTypes = nTypes;
    if (nTypes < 1)
        which &= ~(XkbKTLevelNamesMask | XkbKeyTypeNamesMask);
    else if (firstType <= XkbLastRequiredType) {
        int adjust;

        adjust = XkbLastRequiredType - firstType + 1;
        firstType += adjust;
        nTypes -= adjust;
        if (nTypes < 1)
            which &= ~XkbKeyTypeNamesMask;
    }
    names = xkb->names;
    if (which & (XkbKTLevelNamesMask | XkbKeyTypeNamesMask)) {
        register int i;
        XkbKeyTypePtr type;

        if ((xkb->map == NULL) || (xkb->map->types == NULL) || (nTypes == 0) ||
            (firstType + nTypes > xkb->map->num_types) ||
            (firstLvlType + nLvlTypes > xkb->map->num_types))
            return False;
        if (which & XkbKTLevelNamesMask) {
            type = &xkb->map->types[firstLvlType];
            for (i = nLvlNames = 0; i < nLvlTypes; i++, type++) {
                if (type->level_names != NULL)
                    nLvlNames += type->num_levels;
            }
        }
    }

    nVMods = nLEDs = nRG = nKA = nAtoms = nGroups = 0;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetNames, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSetNames;
    req->deviceSpec = xkb->device_spec;
    req->firstType = firstType;
    req->nTypes = nTypes;
    req->firstKey = xkb->min_key_code;
    req->nKeys = xkb->max_key_code - xkb->min_key_code + 1;

    if (which & XkbKeycodesNameMask)
        nAtoms++;
    if (which & XkbGeometryNameMask)
        nAtoms++;
    if (which & XkbSymbolsNameMask)
        nAtoms++;
    if (which & XkbPhysSymbolsNameMask)
        nAtoms++;
    if (which & XkbTypesNameMask)
        nAtoms++;
    if (which & XkbCompatNameMask)
        nAtoms++;
    if (which & XkbKeyTypeNamesMask)
        nAtoms += nTypes;
    if (which & XkbKTLevelNamesMask) {
        req->firstKTLevel = firstLvlType;
        req->nKTLevels = nLvlTypes;
        req->length += XkbPaddedSize(nLvlTypes) / 4;    /* room for group widths */
        nAtoms += nLvlNames;
    }
    else
        req->firstKTLevel = req->nKTLevels = 0;

    if (which & XkbIndicatorNamesMask) {
        req->indicators = leds =
            _XkbCountAtoms(names->indicators, XkbNumIndicators, &nLEDs);
        if (nLEDs > 0)
            nAtoms += nLEDs;
        else
            which &= ~XkbIndicatorNamesMask;
    }
    else
        req->indicators = leds = 0;

    if (which & XkbVirtualModNamesMask) {
        vmods = req->virtualMods = (CARD16)
            _XkbCountAtoms(names->vmods, XkbNumVirtualMods, &nVMods);
        if (nVMods > 0)
            nAtoms += nVMods;
        else
            which &= ~XkbVirtualModNamesMask;
    }
    else
        vmods = req->virtualMods = 0;

    if (which & XkbGroupNamesMask) {
        groups = req->groupNames = (CARD8)
            _XkbCountAtoms(names->groups, XkbNumKbdGroups, &nGroups);
        if (nGroups > 0)
            nAtoms += nGroups;
        else
            which &= ~XkbGroupNamesMask;
    }
    else
        groups = req->groupNames = 0;

    if ((which & XkbKeyNamesMask) && (names->keys != NULL)) {
        firstKey = req->firstKey;
        nKeys = req->nKeys;
        nAtoms += nKeys;        /* technically not atoms, but 4 bytes wide */
    }
    else
        which &= ~XkbKeyNamesMask;

    if (which & XkbKeyAliasesMask) {
        nKA = ((names->key_aliases != NULL) ? names->num_key_aliases : 0);
        if (nKA > 0) {
            req->nKeyAliases = nKA;
            nAtoms += nKA * 2;  /* not atoms, but 8 bytes on the wire */
        }
        else {
            which &= ~XkbKeyAliasesMask;
            req->nKeyAliases = 0;
        }
    }
    else
        req->nKeyAliases = 0;

    if (which & XkbRGNamesMask) {
        nRG = names->num_rg;
        if (nRG > 0)
            nAtoms += nRG;
        else
            which &= ~XkbRGNamesMask;
    }

    req->which = which;
    req->nRadioGroups = nRG;
    req->length += (nAtoms * 4) / 4;

    if (which & XkbKeycodesNameMask)
        Data32(dpy, (long *) &names->keycodes, 4);
    if (which & XkbGeometryNameMask)
        Data32(dpy, (long *) &names->geometry, 4);
    if (which & XkbSymbolsNameMask)
        Data32(dpy, (long *) &names->symbols, 4);
    if (which & XkbPhysSymbolsNameMask)
        Data32(dpy, (long *) &names->phys_symbols, 4);
    if (which & XkbTypesNameMask)
        Data32(dpy, (long *) &names->types, 4);
    if (which & XkbCompatNameMask)
        Data32(dpy, (long *) &names->compat, 4);
    if (which & XkbKeyTypeNamesMask) {
        register int i;
        register XkbKeyTypePtr type;

        type = &xkb->map->types[firstType];
        for (i = 0; i < nTypes; i++, type++) {
            Data32(dpy, (long *) &type->name, 4);
        }
    }
    if (which & XkbKTLevelNamesMask) {
        XkbKeyTypePtr type;
        int i;
        char *tmp;

        BufAlloc(char *, tmp, XkbPaddedSize(nLvlTypes));
        type = &xkb->map->types[firstLvlType];
        for (i = 0; i < nLvlTypes; i++, type++) {
            *tmp++ = type->num_levels;
        }
        type = &xkb->map->types[firstLvlType];
        for (i = 0; i < nLvlTypes; i++, type++) {
            if (type->level_names != NULL)
                Data32(dpy, (long *) type->level_names, type->num_levels * 4);
        }
    }
    if (which & XkbIndicatorNamesMask)
        _XkbCopyAtoms(dpy, names->indicators, leds, XkbNumIndicators);
    if (which & XkbVirtualModNamesMask)
        _XkbCopyAtoms(dpy, names->vmods, vmods, XkbNumVirtualMods);
    if (which & XkbGroupNamesMask)
        _XkbCopyAtoms(dpy, names->groups, groups, XkbNumKbdGroups);
    if (which & XkbKeyNamesMask) {
        Data(dpy, (char *) &names->keys[firstKey], nKeys * XkbKeyNameLength);
    }
    if (which & XkbKeyAliasesMask) {
        Data(dpy, (char *) names->key_aliases, nKA * XkbKeyNameLength * 2);
    }
    if (which & XkbRGNamesMask) {
        Data32(dpy, (long *) names->radio_groups, nRG * 4);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

Bool
XkbChangeNames(Display *dpy, XkbDescPtr xkb, XkbNameChangesPtr changes)
{
    register xkbSetNamesReq *req;
    int nLvlNames = 0;
    XkbInfoPtr xkbi;
    XkbNamesPtr names;
    unsigned which, firstType, nTypes;
    unsigned firstLvlType, nLvlTypes;
    int nVMods, nLEDs, nRG, nKA, nGroups;
    int nKeys = 0, firstKey = 0, nAtoms;
    CARD32 leds = 0, vmods = 0, groups = 0;

    if ((dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return False;
    if ((!xkb) || (!xkb->names) || (!changes))
        return False;
    which = changes->changed;
    firstType = changes->first_type;
    nTypes = changes->num_types;
    firstLvlType = changes->first_lvl;
    nLvlTypes = changes->num_lvls;
    if (which & XkbKeyTypeNamesMask) {
        if (nTypes < 1)
            which &= ~XkbKeyTypeNamesMask;
        else if (firstType <= XkbLastRequiredType) {
            int adjust;

            adjust = XkbLastRequiredType - firstType + 1;
            firstType += adjust;
            nTypes -= adjust;
            if (nTypes < 1)
                which &= ~XkbKeyTypeNamesMask;
        }
    }
    else
        firstType = nTypes = 0;

    if (which & XkbKTLevelNamesMask) {
        if (nLvlTypes < 1)
            which &= ~XkbKTLevelNamesMask;
    }
    else
        firstLvlType = nLvlTypes = 0;

    names = xkb->names;
    if (which & (XkbKTLevelNamesMask | XkbKeyTypeNamesMask)) {
        register int i;

        if ((xkb->map == NULL) || (xkb->map->types == NULL) || (nTypes == 0) ||
            (firstType + nTypes > xkb->map->num_types) ||
            (firstLvlType + nLvlTypes > xkb->map->num_types))
            return False;
        if (which & XkbKTLevelNamesMask) {
            XkbKeyTypePtr type = &xkb->map->types[firstLvlType];

            for (i = nLvlNames = 0; i < nLvlTypes; i++, type++) {
                if (type->level_names != NULL)
                    nLvlNames += type->num_levels;
            }
        }
    }

    if (changes->num_keys < 1)
        which &= ~XkbKeyNamesMask;
    if ((which & XkbKeyNamesMask) == 0)
        changes->first_key = changes->num_keys = 0;
    else if ((changes->first_key < xkb->min_key_code) ||
             (changes->first_key + changes->num_keys > xkb->max_key_code)) {
        return False;
    }

    if ((which & XkbVirtualModNamesMask) == 0)
        changes->changed_vmods = 0;
    else if (changes->changed_vmods == 0)
        which &= ~XkbVirtualModNamesMask;

    if ((which & XkbIndicatorNamesMask) == 0)
        changes->changed_indicators = 0;
    else if (changes->changed_indicators == 0)
        which &= ~XkbIndicatorNamesMask;

    if ((which & XkbGroupNamesMask) == 0)
        changes->changed_groups = 0;
    else if (changes->changed_groups == 0)
        which &= ~XkbGroupNamesMask;

    nVMods = nLEDs = nRG = nKA = nAtoms = nGroups = 0;
    LockDisplay(dpy);
    xkbi = dpy->xkb_info;
    GetReq(kbSetNames, req);
    req->reqType = xkbi->codes->major_opcode;
    req->xkbReqType = X_kbSetNames;
    req->deviceSpec = xkb->device_spec;
    req->firstType = firstType;
    req->nTypes = nTypes;
    req->firstKey = changes->first_key;
    req->nKeys = changes->num_keys;

    if (which & XkbKeycodesNameMask)
        nAtoms++;
    if (which & XkbGeometryNameMask)
        nAtoms++;
    if (which & XkbSymbolsNameMask)
        nAtoms++;
    if (which & XkbPhysSymbolsNameMask)
        nAtoms++;
    if (which & XkbTypesNameMask)
        nAtoms++;
    if (which & XkbCompatNameMask)
        nAtoms++;
    if (which & XkbKeyTypeNamesMask)
        nAtoms += nTypes;
    if (which & XkbKTLevelNamesMask) {
        req->firstKTLevel = firstLvlType;
        req->nKTLevels = nLvlTypes;
        req->length += XkbPaddedSize(nLvlTypes) / 4;    /* room for group widths */
        nAtoms += nLvlNames;
    }
    else
        req->firstKTLevel = req->nKTLevels = 0;

    if (which & XkbIndicatorNamesMask) {
        leds = req->indicators = (CARD32) changes->changed_indicators;
        nLEDs = _XkbCountBits(XkbNumIndicators, changes->changed_indicators);
        if (nLEDs > 0)
            nAtoms += nLEDs;
        else
            which &= ~XkbIndicatorNamesMask;
    }
    else
        req->indicators = 0;

    if (which & XkbVirtualModNamesMask) {
        vmods = req->virtualMods = changes->changed_vmods;
        nVMods = _XkbCountBits(XkbNumVirtualMods,
                               (unsigned long) changes->changed_vmods);
        if (nVMods > 0)
            nAtoms += nVMods;
        else
            which &= ~XkbVirtualModNamesMask;
    }
    else
        req->virtualMods = 0;

    if (which & XkbGroupNamesMask) {
        groups = req->groupNames = changes->changed_groups;
        nGroups = _XkbCountBits(XkbNumKbdGroups,
                                (unsigned long) changes->changed_groups);
        if (nGroups > 0)
            nAtoms += nGroups;
        else
            which &= ~XkbGroupNamesMask;
    }
    else
        req->groupNames = 0;

    if ((which & XkbKeyNamesMask) && (names->keys != NULL)) {
        firstKey = req->firstKey;
        nKeys = req->nKeys;
        nAtoms += nKeys;        /* technically not atoms, but 4 bytes wide */
    }
    else
        which &= ~XkbKeyNamesMask;

    if (which & XkbKeyAliasesMask) {
        nKA = ((names->key_aliases != NULL) ? names->num_key_aliases : 0);
        if (nKA > 0)
            nAtoms += nKA * 2;  /* not atoms, but 8 bytes on the wire */
        else
            which &= ~XkbKeyAliasesMask;
    }

    if (which & XkbRGNamesMask) {
        nRG = names->num_rg;
        if (nRG > 0)
            nAtoms += nRG;
        else
            which &= ~XkbRGNamesMask;
    }

    req->which = which;
    req->nRadioGroups = nRG;
    req->length += (nAtoms * 4) / 4;

    if (which & XkbKeycodesNameMask)
        Data32(dpy, (long *) &names->keycodes, 4);
    if (which & XkbGeometryNameMask)
        Data32(dpy, (long *) &names->geometry, 4);
    if (which & XkbSymbolsNameMask)
        Data32(dpy, (long *) &names->symbols, 4);
    if (which & XkbPhysSymbolsNameMask)
        Data32(dpy, (long *) &names->phys_symbols, 4);
    if (which & XkbTypesNameMask)
        Data32(dpy, (long *) &names->types, 4);
    if (which & XkbCompatNameMask)
        Data32(dpy, (long *) &names->compat, 4);
    if (which & XkbKeyTypeNamesMask) {
        register int i;
        register XkbKeyTypePtr type;

        type = &xkb->map->types[firstType];
        for (i = 0; i < nTypes; i++, type++) {
            Data32(dpy, (long *) &type->name, 4);
        }
    }
    if (which & XkbKTLevelNamesMask) {
        XkbKeyTypePtr type;
        int i;
        char *tmp;

        BufAlloc(char *, tmp, XkbPaddedSize(nLvlTypes));
        type = &xkb->map->types[firstLvlType];
        for (i = 0; i < nLvlTypes; i++, type++) {
            *tmp++ = type->num_levels;
        }
        type = &xkb->map->types[firstLvlType];
        for (i = 0; i < nLvlTypes; i++, type++) {
            if (type->level_names != NULL)
                Data32(dpy, (long *) type->level_names, type->num_levels * 4);
        }
    }
    if (which & XkbIndicatorNamesMask)
        _XkbCopyAtoms(dpy, names->indicators, leds, XkbNumIndicators);
    if (which & XkbVirtualModNamesMask)
        _XkbCopyAtoms(dpy, names->vmods, vmods, XkbNumVirtualMods);
    if (which & XkbGroupNamesMask)
        _XkbCopyAtoms(dpy, names->groups, groups, XkbNumKbdGroups);
    if (which & XkbKeyNamesMask) {
        Data(dpy, (char *) &names->keys[firstKey], nKeys * XkbKeyNameLength);
    }
    if (which & XkbKeyAliasesMask) {
        Data(dpy, (char *) names->key_aliases, nKA * XkbKeyNameLength * 2);
    }
    if (which & XkbRGNamesMask) {
        Data32(dpy, (long *) names->radio_groups, nRG * 4);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

void
XkbNoteNameChanges(XkbNameChangesPtr old,
                   XkbNamesNotifyEvent *new,
                   unsigned int wanted)
{
    int first, last, old_last, new_last;

    if ((old == NULL) || (new == NULL))
        return;

    wanted &= new->changed;

    if (wanted == 0)
	return;

    if (wanted & XkbKeyTypeNamesMask) {
        if (old->changed & XkbKeyTypeNamesMask) {
            new_last = (new->first_type + new->num_types - 1);
            old_last = (old->first_type + old->num_types - 1);

            if (new->first_type < old->first_type)
                first = new->first_type;
            else
                first = old->first_type;

            if (old_last > new_last)
                last = old_last;
            else
                last = new_last;

            old->first_type = first;
            old->num_types = (last - first) + 1;
        }
        else {
            old->first_type = new->first_type;
            old->num_types = new->num_types;
        }
    }
    if (wanted & XkbKTLevelNamesMask) {
        if (old->changed & XkbKTLevelNamesMask) {
            new_last = (new->first_lvl + new->num_lvls - 1);
            old_last = (old->first_lvl + old->num_lvls - 1);

            if (new->first_lvl < old->first_lvl)
                first = new->first_lvl;
            else
                first = old->first_lvl;

            if (old_last > new_last)
                last = old_last;
            else
                last = new_last;

            old->first_lvl = first;
            old->num_lvls = (last - first) + 1;
        }
        else {
            old->first_lvl = new->first_lvl;
            old->num_lvls = new->num_lvls;
        }
    }
    if (wanted & XkbIndicatorNamesMask) {
        if (old->changed & XkbIndicatorNamesMask)
            old->changed_indicators |= new->changed_indicators;
        else
            old->changed_indicators = new->changed_indicators;
    }
    if (wanted & XkbKeyNamesMask) {
        if (old->changed & XkbKeyNamesMask) {
            new_last = (new->first_key + new->num_keys - 1);
            old_last = (old->first_key + old->num_keys - 1);

            first = old->first_key;

            if (new->first_key < old->first_key)
                first = new->first_key;
            if (old_last > new_last)
                new_last = old_last;

            old->first_key = first;
            old->num_keys = (new_last - first) + 1;
        }
        else {
            old->first_key = new->first_key;
            old->num_keys = new->num_keys;
        }
    }
    if (wanted & XkbVirtualModNamesMask) {
        if (old->changed & XkbVirtualModNamesMask)
            old->changed_vmods |= new->changed_vmods;
        else
            old->changed_vmods = new->changed_vmods;
    }
    if (wanted & XkbGroupNamesMask) {
        if (old->changed & XkbGroupNamesMask)
            old->changed_groups |= new->changed_groups;
        else
            old->changed_groups = new->changed_groups;
    }
    if (wanted & XkbRGNamesMask)
        old->num_rg = new->num_radio_groups;
    if (wanted & XkbKeyAliasesMask)
        old->num_aliases = new->num_aliases;
    old->changed |= wanted;
    return;
}
