/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

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
#elif defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <stdio.h>

#include <X11/Xos.h>
#include <X11/Xfuncs.h>


#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <X11/XKBlib.h>

#include <X11/extensions/XKBgeom.h>
#include "XKMformat.h"
#include "XKBfileInt.h"


#ifndef SEEK_SET
#define	SEEK_SET 0
#endif

/***====================================================================***/

static XPointer
XkmInsureSize(XPointer oldPtr, int oldCount, int *newCountRtrn, int elemSize)
{
    int newCount = *newCountRtrn;

    if (oldPtr == NULL) {
        if (newCount == 0)
            return NULL;
        oldPtr = (XPointer) _XkbCalloc(newCount, elemSize);
    }
    else if (oldCount < newCount) {
        oldPtr = (XPointer) _XkbRealloc(oldPtr, newCount * elemSize);
        if (oldPtr != NULL) {
            char *tmp = (char *) oldPtr;

            bzero(&tmp[oldCount * elemSize], (newCount - oldCount) * elemSize);
        }
    }
    else if (newCount < oldCount) {
        *newCountRtrn = oldCount;
    }
    return oldPtr;
}

#define	XkmInsureTypedSize(p,o,n,t) ((p)=((t *)XkmInsureSize((char *)(p),(o),(n),sizeof(t))))

static CARD8
XkmGetCARD8(FILE * file, int *pNRead)
{
    int tmp;

    tmp = getc(file);
    if (pNRead && (tmp != EOF))
        (*pNRead) += 1;
    return tmp;
}

static CARD16
XkmGetCARD16(FILE * file, int *pNRead)
{
    CARD16 val;

    if ((fread(&val, 2, 1, file) == 1) && (pNRead))
        (*pNRead) += 2;
    return val;
}

static CARD32
XkmGetCARD32(FILE * file, int *pNRead)
{
    CARD32 val;

    if ((fread(&val, 4, 1, file) == 1) && (pNRead))
        (*pNRead) += 4;
    return val;
}

static int
XkmSkipPadding(FILE * file, unsigned pad)
{
    int nRead = 0;

    for (unsigned int i = 0; i < pad; i++) {
        if (getc(file) != EOF)
            nRead++;
    }
    return nRead;
}

static int
XkmGetCountedString(FILE * file, char *str, int max_len)
{
    int count, nRead = 0;

    count = XkmGetCARD16(file, &nRead);
    if (count > 0) {
        int tmp;

        if (count > max_len) {
            tmp = fread(str, 1, max_len, file);
            while (tmp < count) {
                if ((getc(file)) != EOF)
                    tmp++;
                else
                    break;
            }
        }
        else {
            tmp = fread(str, 1, count, file);
        }
        nRead += tmp;
    }
    if (count >= max_len)
        str[max_len - 1] = '\0';
    else
        str[count] = '\0';
    count = XkbPaddedSize(nRead) - nRead;
    if (count > 0)
        nRead += XkmSkipPadding(file, count);
    return nRead;
}

/***====================================================================***/

static int
ReadXkmVirtualMods(FILE *file, XkbFileInfo *result, XkbChangesPtr changes)
{
    register unsigned int i, bit;
    unsigned int bound, named, tmp;
    int nRead = 0;
    XkbDescPtr xkb;

    xkb = result->xkb;
    if (XkbAllocServerMap(xkb, XkbVirtualModsMask, 0) != Success) {
        _XkbLibError(_XkbErrBadAlloc, "ReadXkmVirtualMods", 0);
        return -1;
    }
    bound = XkmGetCARD16(file, &nRead);
    named = XkmGetCARD16(file, &nRead);
    for (i = tmp = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (bound & bit) {
            xkb->server->vmods[i] = XkmGetCARD8(file, &nRead);
            if (changes)
                changes->map.vmods |= bit;
            tmp++;
        }
    }
    if ((i = XkbPaddedSize(tmp) - tmp) > 0)
        nRead += XkmSkipPadding(file, i);
    if (XkbAllocNames(xkb, XkbVirtualModNamesMask, 0, 0) != Success) {
        _XkbLibError(_XkbErrBadAlloc, "ReadXkmVirtualMods", 0);
        return -1;
    }
    for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        char name[100];

        if (named & bit) {
            if (nRead += XkmGetCountedString(file, name, 100)) {
                xkb->names->vmods[i] = XkbInternAtom(xkb->dpy, name, False);
                if (changes)
                    changes->names.changed_vmods |= bit;
            }
        }
    }
    return nRead;
}

/***====================================================================***/

static int
ReadXkmKeycodes(FILE *file, XkbFileInfo *result, XkbChangesPtr changes)
{
    unsigned int i;
    unsigned minKC, maxKC, nAl;
    int nRead = 0;
    char name[100];
    XkbKeyNamePtr pN;
    XkbDescPtr xkb;

    xkb = result->xkb;
    name[0] = '\0';
    nRead += XkmGetCountedString(file, name, 100);
    minKC = XkmGetCARD8(file, &nRead);
    maxKC = XkmGetCARD8(file, &nRead);
    if (xkb->min_key_code == 0) {
        xkb->min_key_code = minKC;
        xkb->max_key_code = maxKC;
    }
    else {
        if (minKC < xkb->min_key_code)
            xkb->min_key_code = minKC;
        if (maxKC > xkb->max_key_code) {
            _XkbLibError(_XkbErrBadValue, "ReadXkmKeycodes", maxKC);
            return -1;
        }
    }
    nAl = XkmGetCARD8(file, &nRead);
    nRead += XkmSkipPadding(file, 1);

#define WANTED (XkbKeycodesNameMask|XkbKeyNamesMask|XkbKeyAliasesMask)
    if (XkbAllocNames(xkb, WANTED, 0, nAl) != Success) {
        _XkbLibError(_XkbErrBadAlloc, "ReadXkmKeycodes", 0);
        return -1;
    }
    if (name[0] != '\0') {
        xkb->names->keycodes = XkbInternAtom(xkb->dpy, name, False);
    }

    for (pN = &xkb->names->keys[minKC], i = minKC; i <= maxKC; i++, pN++) {
        if (fread(pN, 1, XkbKeyNameLength, file) != XkbKeyNameLength) {
            _XkbLibError(_XkbErrBadLength, "ReadXkmKeycodes", 0);
            return -1;
        }
        nRead += XkbKeyNameLength;
    }
    if (nAl > 0) {
        XkbKeyAliasPtr pAl;

        for (pAl = xkb->names->key_aliases, i = 0; i < nAl; i++, pAl++) {
            int tmp;

            tmp = fread(pAl, 1, 2 * XkbKeyNameLength, file);
            if (tmp != 2 * XkbKeyNameLength) {
                _XkbLibError(_XkbErrBadLength, "ReadXkmKeycodes", 0);
                return -1;
            }
            nRead += 2 * XkbKeyNameLength;
        }
        if (changes)
            changes->names.changed |= XkbKeyAliasesMask;
    }
    if (changes)
        changes->names.changed |= XkbKeyNamesMask;
    return nRead;
}

/***====================================================================***/

static int
ReadXkmKeyTypes(FILE *file, XkbFileInfo *result, XkbChangesPtr changes)
{
    register unsigned i, n;
    unsigned num_types;
    int nRead = 0;
    int tmp;
    XkbKeyTypePtr type;
    xkmKeyTypeDesc wire;
    XkbKTMapEntryPtr entry;
    xkmKTMapEntryDesc wire_entry;
    char buf[100];
    XkbDescPtr xkb;

    xkb = result->xkb;
    if ((tmp = XkmGetCountedString(file, buf, 100)) < 1) {
        _XkbLibError(_XkbErrBadLength, "ReadXkmKeyTypes", 0);
        return -1;
    }
    nRead += tmp;
    if (buf[0] != '\0') {
        if (XkbAllocNames(xkb, XkbTypesNameMask, 0, 0) != Success) {
            _XkbLibError(_XkbErrBadAlloc, "ReadXkmKeyTypes", 0);
            return -1;
        }
        xkb->names->types = XkbInternAtom(xkb->dpy, buf, False);
    }
    num_types = XkmGetCARD16(file, &nRead);
    nRead += XkmSkipPadding(file, 2);
    if (num_types < 1)
        return nRead;
    if (XkbAllocClientMap(xkb, XkbKeyTypesMask, num_types) != Success) {
        _XkbLibError(_XkbErrBadAlloc, "ReadXkmKeyTypes", 0);
        return nRead;
    }
    xkb->map->num_types = num_types;
    if (num_types < XkbNumRequiredTypes) {
        _XkbLibError(_XkbErrMissingReqTypes, "ReadXkmKeyTypes", 0);
        return -1;
    }
    type = xkb->map->types;
    for (i = 0; i < num_types; i++, type++) {
        if ((int) fread(&wire, SIZEOF(xkmKeyTypeDesc), 1, file) < 1) {
            _XkbLibError(_XkbErrBadLength, "ReadXkmKeyTypes", 0);
            return -1;
        }
        nRead += SIZEOF(xkmKeyTypeDesc);
        if (((i == XkbOneLevelIndex) && (wire.numLevels != 1)) ||
            (((i == XkbTwoLevelIndex) || (i == XkbAlphabeticIndex) ||
              ((i) == XkbKeypadIndex)) && (wire.numLevels != 2))) {
            _XkbLibError(_XkbErrBadTypeWidth, "ReadXkmKeyTypes", i);
            return -1;
        }
        tmp = wire.nMapEntries;
        XkmInsureTypedSize(type->map, type->map_count, &tmp, XkbKTMapEntryRec);
        if ((wire.nMapEntries > 0) && (type->map == NULL)) {
            _XkbLibError(_XkbErrBadValue, "ReadXkmKeyTypes", wire.nMapEntries);
            return -1;
        }
        for (n = 0, entry = type->map; n < wire.nMapEntries; n++, entry++) {
            if (fread(&wire_entry, SIZEOF(xkmKTMapEntryDesc), 1, file) <
                (int) 1) {
                _XkbLibError(_XkbErrBadLength, "ReadXkmKeyTypes", 0);
                return -1;
            }
            nRead += SIZEOF(xkmKTMapEntryDesc);
            entry->active = (wire_entry.virtualMods == 0);
            entry->level = wire_entry.level;
            entry->mods.mask = wire_entry.realMods;
            entry->mods.real_mods = wire_entry.realMods;
            entry->mods.vmods = wire_entry.virtualMods;
        }
        nRead += XkmGetCountedString(file, buf, 100);
        if (((i == XkbOneLevelIndex) && (strcmp(buf, "ONE_LEVEL") != 0)) ||
            ((i == XkbTwoLevelIndex) && (strcmp(buf, "TWO_LEVEL") != 0)) ||
            ((i == XkbAlphabeticIndex) && (strcmp(buf, "ALPHABETIC") != 0)) ||
            ((i == XkbKeypadIndex) && (strcmp(buf, "KEYPAD") != 0))) {
            _XkbLibError(_XkbErrBadTypeName, "ReadXkmKeyTypes", 0);
            return -1;
        }
        if (buf[0] != '\0') {
            type->name = XkbInternAtom(xkb->dpy, buf, False);
        }
        else
            type->name = None;

        if (wire.preserve) {
            xkmModsDesc p_entry;
            XkbModsPtr pre;

            XkmInsureTypedSize(type->preserve, type->map_count, &tmp,
                               XkbModsRec);
            if (type->preserve == NULL) {
                _XkbLibError(_XkbErrBadMatch, "ReadXkmKeycodes", 0);
                return -1;
            }
            for (n = 0, pre = type->preserve; n < wire.nMapEntries; n++, pre++) {
                if (fread(&p_entry, SIZEOF(xkmModsDesc), 1, file) < 1) {
                    _XkbLibError(_XkbErrBadLength, "ReadXkmKeycodes", 0);
                    return -1;
                }
                nRead += SIZEOF(xkmModsDesc);
                pre->mask = p_entry.realMods;
                pre->real_mods = p_entry.realMods;
                pre->vmods = p_entry.virtualMods;
            }
        }
        if (wire.nLevelNames > 0) {
            int width = wire.numLevels;

            if (wire.nLevelNames > (unsigned) width) {
                _XkbLibError(_XkbErrBadMatch, "ReadXkmKeycodes", 0);
                return -1;
            }
            XkmInsureTypedSize(type->level_names, type->num_levels, &width,
                               Atom);
            if (type->level_names != NULL) {
                for (n = 0; n < wire.nLevelNames; n++) {
                    if ((tmp = XkmGetCountedString(file, buf, 100)) < 1)
                        return -1;
                    nRead += tmp;
                    if (strlen(buf) == 0)
                        type->level_names[n] = None;
                    else
                        type->level_names[n] = XkbInternAtom(xkb->dpy, buf, 0);
                }
            }
        }
        type->mods.mask = wire.realMods;
        type->mods.real_mods = wire.realMods;
        type->mods.vmods = wire.virtualMods;
        type->num_levels = wire.numLevels;
        type->map_count = wire.nMapEntries;
    }
    if (changes) {
        changes->map.changed |= XkbKeyTypesMask;
        changes->map.first_type = 0;
        changes->map.num_types = xkb->map->num_types;
    }
    return nRead;
}

/***====================================================================***/

static int
ReadXkmCompatMap(FILE *file, XkbFileInfo *result, XkbChangesPtr changes)
{
    unsigned num_si, groups;
    char name[100];
    XkbSymInterpretPtr interp;
    xkmSymInterpretDesc wire;
    unsigned tmp;
    int nRead = 0;
    XkbDescPtr xkb;
    XkbCompatMapPtr compat;

    xkb = result->xkb;
    if ((tmp = XkmGetCountedString(file, name, 100)) < 1) {
        _XkbLibError(_XkbErrBadLength, "ReadXkmCompatMap", 0);
        return -1;
    }
    nRead += tmp;
    if (name[0] != '\0') {
        if (XkbAllocNames(xkb, XkbCompatNameMask, 0, 0) != Success) {
            _XkbLibError(_XkbErrBadAlloc, "ReadXkmCompatMap", 0);
            return -1;
        }
        xkb->names->compat = XkbInternAtom(xkb->dpy, name, False);
    }
    num_si = XkmGetCARD16(file, &nRead);
    groups = XkmGetCARD8(file, &nRead);
    nRead += XkmSkipPadding(file, 1);
    if (XkbAllocCompatMap(xkb, XkbAllCompatMask, num_si) != Success)
        return -1;
    compat = xkb->compat;
    compat->num_si = num_si;
    interp = compat->sym_interpret;
    for (unsigned int i = 0; i < num_si; i++, interp++) {
        tmp = fread(&wire, SIZEOF(xkmSymInterpretDesc), 1, file);
        nRead += tmp * SIZEOF(xkmSymInterpretDesc);
        interp->sym = wire.sym;
        interp->mods = wire.mods;
        interp->match = wire.match;
        interp->virtual_mod = wire.virtualMod;
        interp->flags = wire.flags;
        interp->act.type = wire.actionType;
        interp->act.data[0] = wire.actionData[0];
        interp->act.data[1] = wire.actionData[1];
        interp->act.data[2] = wire.actionData[2];
        interp->act.data[3] = wire.actionData[3];
        interp->act.data[4] = wire.actionData[4];
        interp->act.data[5] = wire.actionData[5];
        interp->act.data[6] = wire.actionData[6];
    }
    if ((num_si > 0) && (changes)) {
        changes->compat.first_si = 0;
        changes->compat.num_si = num_si;
    }
    if (groups) {
        unsigned int bit = 1;

        for (int i = 0; i < XkbNumKbdGroups; i++, bit <<= 1) {
            xkmModsDesc md;

            if (groups & bit) {
                tmp = fread(&md, SIZEOF(xkmModsDesc), 1, file);
                nRead += tmp * SIZEOF(xkmModsDesc);
                xkb->compat->groups[i].real_mods = md.realMods;
                xkb->compat->groups[i].vmods = md.virtualMods;
                if (md.virtualMods != 0) {
                    unsigned mask;

                    if (XkbVirtualModsToReal(xkb, md.virtualMods, &mask))
                        xkb->compat->groups[i].mask = md.realMods | mask;
                }
                else
                    xkb->compat->groups[i].mask = md.realMods;
            }
        }
        if (changes)
            changes->compat.changed_groups |= groups;
    }
    return nRead;
}

static int
ReadXkmIndicators(FILE *file, XkbFileInfo *result, XkbChangesPtr changes)
{
    register unsigned nLEDs;
    xkmIndicatorMapDesc wire;
    char buf[100];
    unsigned tmp;
    int nRead = 0;
    XkbDescPtr xkb;

    xkb = result->xkb;
    if ((xkb->indicators == NULL) && (XkbAllocIndicatorMaps(xkb) != Success)) {
        _XkbLibError(_XkbErrBadAlloc, "indicator rec", 0);
        return -1;
    }
    if (XkbAllocNames(xkb, XkbIndicatorNamesMask, 0, 0) != Success) {
        _XkbLibError(_XkbErrBadAlloc, "indicator names", 0);
        return -1;
    }
    nLEDs = XkmGetCARD8(file, &nRead);
    nRead += XkmSkipPadding(file, 3);
    xkb->indicators->phys_indicators = XkmGetCARD32(file, &nRead);
    while (nLEDs-- > 0) {
        Atom name;
        XkbIndicatorMapPtr map;

        if ((tmp = XkmGetCountedString(file, buf, 100)) < 1) {
            _XkbLibError(_XkbErrBadLength, "ReadXkmIndicators", 0);
            return -1;
        }
        nRead += tmp;
        if (buf[0] != '\0')
            name = XkbInternAtom(xkb->dpy, buf, False);
        else
            name = None;
        if ((tmp = fread(&wire, SIZEOF(xkmIndicatorMapDesc), 1, file)) < 1) {
            _XkbLibError(_XkbErrBadLength, "ReadXkmIndicators", 0);
            return -1;
        }
        nRead += tmp * SIZEOF(xkmIndicatorMapDesc);
        if (xkb->names) {
            xkb->names->indicators[wire.indicator - 1] = name;
            if (changes)
                changes->names.changed_indicators |=
                    (1 << (wire.indicator - 1));
        }
        map = &xkb->indicators->maps[wire.indicator - 1];
        map->flags = wire.flags;
        map->which_groups = wire.which_groups;
        map->groups = wire.groups;
        map->which_mods = wire.which_mods;
        map->mods.mask = wire.real_mods;
        map->mods.real_mods = wire.real_mods;
        map->mods.vmods = wire.vmods;
        map->ctrls = wire.ctrls;
    }
    return nRead;
}

static XkbKeyTypePtr
FindTypeForKey(XkbDescPtr xkb, Atom name, unsigned width, KeySym *syms)
{
    if ((!xkb) || (!xkb->map))
        return NULL;
    if (name != None) {
        register unsigned i;

        for (i = 0; i < xkb->map->num_types; i++) {
            if (xkb->map->types[i].name == name) {
#ifdef DEBUG
                if (xkb->map->types[i].num_levels != width)
                    fprintf(stderr,
                            "Group width mismatch between key and type\n");
#endif
                return &xkb->map->types[i];
            }
        }
    }
    if ((width < 2) || ((syms != NULL) && (syms[1] == NoSymbol)))
        return &xkb->map->types[XkbOneLevelIndex];
    if (syms != NULL) {
        if (XkbKSIsLower(syms[0]) && XkbKSIsUpper(syms[1]))
            return &xkb->map->types[XkbAlphabeticIndex];
        else if (XkbKSIsKeypad(syms[0]) || XkbKSIsKeypad(syms[1]))
            return &xkb->map->types[XkbKeypadIndex];
    }
    return &xkb->map->types[XkbTwoLevelIndex];
}

static int
ReadXkmSymbols(FILE *file, XkbFileInfo *result)
{
    register int i, g, s, totalVModMaps;
    xkmKeySymMapDesc wireMap;
    char buf[100];
    unsigned minKC, maxKC, groupNames, tmp;
    int nRead = 0;
    XkbDescPtr xkb;

    xkb = result->xkb;
    if ((tmp = XkmGetCountedString(file, buf, 100)) < 1)
        return -1;
    nRead += tmp;
    minKC = XkmGetCARD8(file, &nRead);
    maxKC = XkmGetCARD8(file, &nRead);
    groupNames = XkmGetCARD8(file, &nRead);
    totalVModMaps = XkmGetCARD8(file, &nRead);
    if (XkbAllocNames(xkb,
                      XkbSymbolsNameMask | XkbPhysSymbolsNameMask |
                      XkbGroupNamesMask, 0, 0) != Success) {
        _XkbLibError(_XkbErrBadAlloc, "physical names", 0);
        return -1;
    }
    if ((buf[0] != '\0') && (xkb->names)) {
        Atom name;

        name = XkbInternAtom(xkb->dpy, buf, 0);
        xkb->names->symbols = name;
        xkb->names->phys_symbols = name;
    }
    for (i = 0, g = 1; i < XkbNumKbdGroups; i++, g <<= 1) {
        if (groupNames & g) {
            if ((tmp = XkmGetCountedString(file, buf, 100)) < 1)
                return -1;
            nRead += tmp;
            if ((buf[0] != '\0') && (xkb->names)) {
                Atom name;

                name = XkbInternAtom(xkb->dpy, buf, 0);
                xkb->names->groups[i] = name;
            }
            else
                xkb->names->groups[i] = None;
        }
    }
    if (XkbAllocServerMap(xkb, XkbAllServerInfoMask, 0) != Success) {
        _XkbLibError(_XkbErrBadAlloc, "server map", 0);
        return -1;
    }
    if (XkbAllocClientMap(xkb, XkbAllClientInfoMask, 0) != Success) {
        _XkbLibError(_XkbErrBadAlloc, "client map", 0);
        return -1;
    }
    if (XkbAllocControls(xkb, XkbAllControlsMask) != Success) {
        _XkbLibError(_XkbErrBadAlloc, "controls", 0);
        return -1;
    }
    if ((xkb->map == NULL) || (xkb->server == NULL))
        return -1;
    if (xkb->min_key_code < 8)
        xkb->min_key_code = minKC;
    if (xkb->max_key_code < 8)
        xkb->max_key_code = maxKC;
    if ((minKC >= 8) && (minKC < xkb->min_key_code))
        xkb->min_key_code = minKC;
    if ((maxKC >= 8) && (maxKC > xkb->max_key_code)) {
        _XkbLibError(_XkbErrBadValue, "keys in symbol map", maxKC);
        return -1;
    }
    for (i = minKC; i <= (int) maxKC; i++) {
        Atom typeName[XkbNumKbdGroups];
        XkbKeyTypePtr type[XkbNumKbdGroups];

        if ((tmp = fread(&wireMap, SIZEOF(xkmKeySymMapDesc), 1, file)) < 1) {
            _XkbLibError(_XkbErrBadLength, "ReadXkmSymbols", 0);
            return -1;
        }
        nRead += tmp * SIZEOF(xkmKeySymMapDesc);
        bzero((char *) typeName, XkbNumKbdGroups * sizeof(Atom));
        bzero((char *) type, XkbNumKbdGroups * sizeof(XkbKeyTypePtr));
        if (wireMap.flags & XkmKeyHasTypes) {
            register int g;

            for (g = 0; g < XkbNumKbdGroups; g++) {
                if ((wireMap.flags & (1 << g)) &&
                    ((tmp = XkmGetCountedString(file, buf, 100)) > 0)) {
                    typeName[g] = XkbInternAtom(xkb->dpy, buf, 1);
                    nRead += tmp;
                }
                type[g] = FindTypeForKey(xkb, typeName[g], wireMap.width, NULL);
                if (type[g] == NULL) {
                    _XkbLibError(_XkbErrMissingTypes, "ReadXkmSymbols", 0);
                    return -1;
                }
                if (typeName[g] == type[g]->name)
                    xkb->server->explicit[i] |= (1 << g);
            }
        }
        if (wireMap.flags & XkmRepeatingKey) {
            xkb->ctrls->per_key_repeat[i / 8] |= (1 << (i % 8));
            xkb->server->explicit[i] |= XkbExplicitAutoRepeatMask;
        }
        else if (wireMap.flags & XkmNonRepeatingKey) {
            xkb->ctrls->per_key_repeat[i / 8] &= ~(1 << (i % 8));
            xkb->server->explicit[i] |= XkbExplicitAutoRepeatMask;
        }
        xkb->map->modmap[i] = wireMap.modifier_map;
        if (XkbNumGroups(wireMap.num_groups) > 0) {
            KeySym *sym;
            int nSyms;

            if (XkbNumGroups(wireMap.num_groups) > xkb->ctrls->num_groups)
                xkb->ctrls->num_groups = wireMap.num_groups;
            nSyms = XkbNumGroups(wireMap.num_groups) * wireMap.width;
            sym = XkbResizeKeySyms(xkb, i, nSyms);
            if (!sym)
                return -1;
            for (s = 0; s < nSyms; s++) {
                *sym++ = XkmGetCARD32(file, &nRead);
            }
            if (wireMap.flags & XkmKeyHasActions) {
                XkbAction *act;

                act = XkbResizeKeyActions(xkb, i, nSyms);
                for (s = 0; s < nSyms; s++, act++) {
                    tmp = fread(act, SIZEOF(xkmActionDesc), 1, file);
                    nRead += tmp * SIZEOF(xkmActionDesc);
                }
                xkb->server->explicit[i] |= XkbExplicitInterpretMask;
            }
        }
        for (g = 0; g < XkbNumGroups(wireMap.num_groups); g++) {
            if (((xkb->server->explicit[i] & (1 << g)) == 0) ||
                (type[g] == NULL)) {
                KeySym *tmpSyms;

                tmpSyms = XkbKeySymsPtr(xkb, i) + (wireMap.width * g);
                type[g] = FindTypeForKey(xkb, None, wireMap.width, tmpSyms);
            }
            xkb->map->key_sym_map[i].kt_index[g] =
                type[g] - (&xkb->map->types[0]);
        }
        xkb->map->key_sym_map[i].group_info = wireMap.num_groups;
        xkb->map->key_sym_map[i].width = wireMap.width;
        if (wireMap.flags & XkmKeyHasBehavior) {
            xkmBehaviorDesc b;

            tmp = fread(&b, SIZEOF(xkmBehaviorDesc), 1, file);
            nRead += tmp * SIZEOF(xkmBehaviorDesc);
            xkb->server->behaviors[i].type = b.type;
            xkb->server->behaviors[i].data = b.data;
            xkb->server->explicit[i] |= XkbExplicitBehaviorMask;
        }
    }
    if (totalVModMaps > 0) {
        xkmVModMapDesc v;

        for (i = 0; i < totalVModMaps; i++) {
            tmp = fread(&v, SIZEOF(xkmVModMapDesc), 1, file);
            nRead += tmp * SIZEOF(xkmVModMapDesc);
            if (tmp > 0)
                xkb->server->vmodmap[v.key] = v.vmods;
        }
    }
    return nRead;
}

static int
ReadXkmGeomDoodad(FILE *file, Display *dpy,
                  XkbGeometryPtr geom, XkbSectionPtr section)
{
    XkbDoodadPtr doodad;
    xkmDoodadDesc doodadWire;
    char buf[100];
    unsigned tmp;
    int nRead = 0;

    nRead += XkmGetCountedString(file, buf, 100);
    tmp = fread(&doodadWire, SIZEOF(xkmDoodadDesc), 1, file);
    nRead += SIZEOF(xkmDoodadDesc) * tmp;
    doodad = XkbAddGeomDoodad(geom, section, XkbInternAtom(dpy, buf, False));
    if (!doodad)
        return nRead;
    doodad->any.type = doodadWire.any.type;
    doodad->any.priority = doodadWire.any.priority;
    doodad->any.top = doodadWire.any.top;
    doodad->any.left = doodadWire.any.left;
    switch (doodadWire.any.type) {
    case XkbOutlineDoodad:
    case XkbSolidDoodad:
        doodad->shape.angle = doodadWire.shape.angle;
        doodad->shape.color_ndx = doodadWire.shape.color_ndx;
        doodad->shape.shape_ndx = doodadWire.shape.shape_ndx;
        break;
    case XkbTextDoodad:
        doodad->text.angle = doodadWire.text.angle;
        doodad->text.width = doodadWire.text.width;
        doodad->text.height = doodadWire.text.height;
        doodad->text.color_ndx = doodadWire.text.color_ndx;
        nRead += XkmGetCountedString(file, buf, 100);
        doodad->text.text = _XkbDupString(buf);
        nRead += XkmGetCountedString(file, buf, 100);
        doodad->text.font = _XkbDupString(buf);
        break;
    case XkbIndicatorDoodad:
        doodad->indicator.shape_ndx = doodadWire.indicator.shape_ndx;
        doodad->indicator.on_color_ndx = doodadWire.indicator.on_color_ndx;
        doodad->indicator.off_color_ndx = doodadWire.indicator.off_color_ndx;
        break;
    case XkbLogoDoodad:
        doodad->logo.angle = doodadWire.logo.angle;
        doodad->logo.color_ndx = doodadWire.logo.color_ndx;
        doodad->logo.shape_ndx = doodadWire.logo.shape_ndx;
        nRead += XkmGetCountedString(file, buf, 100);
        doodad->logo.logo_name = _XkbDupString(buf);
        break;
    default:
        /* report error? */
        return nRead;
    }
    return nRead;
}

static int
ReadXkmGeomOverlay(FILE *file, Display *dpy,
                   XkbGeometryPtr geom, XkbSectionPtr section)
{
    char buf[100];
    unsigned tmp;
    int nRead = 0;
    XkbOverlayPtr ol;
    XkbOverlayRowPtr row;
    xkmOverlayDesc olWire;
    xkmOverlayRowDesc rowWire;
    register int r;

    nRead += XkmGetCountedString(file, buf, 100);
    tmp = fread(&olWire, SIZEOF(xkmOverlayDesc), 1, file);
    nRead += tmp * SIZEOF(xkmOverlayDesc);
    ol = XkbAddGeomOverlay(section, XkbInternAtom(dpy, buf, False),
                           olWire.num_rows);
    if (!ol)
        return nRead;
    for (r = 0; r < olWire.num_rows; r++) {
        int k;
        xkmOverlayKeyDesc keyWire;

        tmp = fread(&rowWire, SIZEOF(xkmOverlayRowDesc), 1, file);
        nRead += tmp * SIZEOF(xkmOverlayRowDesc);
        row = XkbAddGeomOverlayRow(ol, rowWire.row_under, rowWire.num_keys);
        if (!row) {
            _XkbLibError(_XkbErrBadAlloc, "ReadXkmGeomOverlay", 0);
            return nRead;
        }
        for (k = 0; k < rowWire.num_keys; k++) {
            tmp = fread(&keyWire, SIZEOF(xkmOverlayKeyDesc), 1, file);
            nRead += tmp * SIZEOF(xkmOverlayKeyDesc);
            memcpy(row->keys[k].over.name, keyWire.over, XkbKeyNameLength);
            memcpy(row->keys[k].under.name, keyWire.under, XkbKeyNameLength);
        }
        row->num_keys = rowWire.num_keys;
    }
    return nRead;
}

static int
ReadXkmGeomSection(FILE *file, Display *dpy, XkbGeometryPtr geom)
{
    register int i;
    XkbSectionPtr section;
    xkmSectionDesc sectionWire;
    unsigned tmp;
    int nRead = 0;
    char buf[100];
    Atom nameAtom;

    nRead += XkmGetCountedString(file, buf, 100);
    nameAtom = XkbInternAtom(dpy, buf, False);
    tmp = fread(&sectionWire, SIZEOF(xkmSectionDesc), 1, file);
    nRead += SIZEOF(xkmSectionDesc) * tmp;
    section = XkbAddGeomSection(geom, nameAtom, sectionWire.num_rows,
                                sectionWire.num_doodads,
                                sectionWire.num_overlays);
    if (!section) {
        _XkbLibError(_XkbErrBadAlloc, "ReadXkmGeomSection", 0);
        return nRead;
    }
    section->top = sectionWire.top;
    section->left = sectionWire.left;
    section->width = sectionWire.width;
    section->height = sectionWire.height;
    section->angle = sectionWire.angle;
    section->priority = sectionWire.priority;
    if (sectionWire.num_rows > 0) {
        register int k;
        XkbRowPtr row;
        xkmRowDesc rowWire;
        XkbKeyPtr key;
        xkmKeyDesc keyWire;

        for (i = 0; i < sectionWire.num_rows; i++) {
            tmp = fread(&rowWire, SIZEOF(xkmRowDesc), 1, file);
            nRead += SIZEOF(xkmRowDesc) * tmp;
            row = XkbAddGeomRow(section, rowWire.num_keys);
            if (!row) {
                _XkbLibError(_XkbErrBadAlloc, "ReadXkmKeycodes", 0);
                return nRead;
            }
            row->top = rowWire.top;
            row->left = rowWire.left;
            row->vertical = rowWire.vertical;
            for (k = 0; k < rowWire.num_keys; k++) {
                tmp = fread(&keyWire, SIZEOF(xkmKeyDesc), 1, file);
                nRead += SIZEOF(xkmKeyDesc) * tmp;
                key = XkbAddGeomKey(row);
                if (!key) {
                    _XkbLibError(_XkbErrBadAlloc, "ReadXkmGeomSection", 0);
                    return nRead;
                }
                memcpy(key->name.name, keyWire.name, XkbKeyNameLength);
                key->gap = keyWire.gap;
                key->shape_ndx = keyWire.shape_ndx;
                key->color_ndx = keyWire.color_ndx;
            }
        }
    }
    if (sectionWire.num_doodads > 0) {
        for (i = 0; i < sectionWire.num_doodads; i++) {
            tmp = ReadXkmGeomDoodad(file, dpy, geom, section);
            nRead += tmp;
            if (tmp < 1)
                return nRead;
        }
    }
    if (sectionWire.num_overlays > 0) {
        for (i = 0; i < sectionWire.num_overlays; i++) {
            tmp = ReadXkmGeomOverlay(file, dpy, geom, section);
            nRead += tmp;
            if (tmp < 1)
                return nRead;
        }
    }
    return nRead;
}

static int
ReadXkmGeometry(FILE *file, XkbFileInfo *result)
{
    register int i;
    char buf[100];
    unsigned tmp;
    int nRead = 0;
    xkmGeometryDesc wireGeom;
    XkbGeometryPtr geom;
    XkbGeometrySizesRec sizes;

    nRead += XkmGetCountedString(file, buf, 100);
    tmp = fread(&wireGeom, SIZEOF(xkmGeometryDesc), 1, file);
    nRead += tmp * SIZEOF(xkmGeometryDesc);
    sizes.which = XkbGeomAllMask;
    sizes.num_properties = wireGeom.num_properties;
    sizes.num_colors = wireGeom.num_colors;
    sizes.num_shapes = wireGeom.num_shapes;
    sizes.num_sections = wireGeom.num_sections;
    sizes.num_doodads = wireGeom.num_doodads;
    sizes.num_key_aliases = wireGeom.num_key_aliases;
    if (XkbAllocGeometry(result->xkb, &sizes) != Success) {
        _XkbLibError(_XkbErrBadAlloc, "ReadXkmGeometry", 0);
        return nRead;
    }
    geom = result->xkb->geom;
    geom->name = XkbInternAtom(result->xkb->dpy, buf, False);
    geom->width_mm = wireGeom.width_mm;
    geom->height_mm = wireGeom.height_mm;
    nRead += XkmGetCountedString(file, buf, 100);
    geom->label_font = _XkbDupString(buf);
    if (wireGeom.num_properties > 0) {
        char val[1024];

        for (i = 0; i < wireGeom.num_properties; i++) {
            nRead += XkmGetCountedString(file, buf, 100);
            nRead += XkmGetCountedString(file, val, 1024);
            if (XkbAddGeomProperty(geom, buf, val) == NULL) {
                _XkbLibError(_XkbErrBadAlloc, "ReadXkmGeometry", 0);
                return nRead;
            }
        }
    }
    if (wireGeom.num_colors > 0) {
        for (i = 0; i < wireGeom.num_colors; i++) {
            nRead += XkmGetCountedString(file, buf, 100);
            if (XkbAddGeomColor(geom, buf, i) == NULL) {
                _XkbLibError(_XkbErrBadAlloc, "ReadXkmGeometry", 0);
                return nRead;
            }
        }
    }
    geom->base_color = &geom->colors[wireGeom.base_color_ndx];
    geom->label_color = &geom->colors[wireGeom.label_color_ndx];
    if (wireGeom.num_shapes > 0) {
        XkbShapePtr shape;
        xkmShapeDesc shapeWire;
        Atom nameAtom;

        for (i = 0; i < wireGeom.num_shapes; i++) {
            register int n;
            XkbOutlinePtr ol;
            xkmOutlineDesc olWire;

            nRead += XkmGetCountedString(file, buf, 100);
            nameAtom = XkbInternAtom(result->xkb->dpy, buf, False);
            tmp = fread(&shapeWire, SIZEOF(xkmShapeDesc), 1, file);
            nRead += tmp * SIZEOF(xkmShapeDesc);
            shape = XkbAddGeomShape(geom, nameAtom, shapeWire.num_outlines);
            if (!shape) {
                _XkbLibError(_XkbErrBadAlloc, "ReadXkmGeometry", 0);
                return nRead;
            }
            for (n = 0; n < shapeWire.num_outlines; n++) {
                register int p;
                xkmPointDesc ptWire;

                tmp = fread(&olWire, SIZEOF(xkmOutlineDesc), 1, file);
                nRead += tmp * SIZEOF(xkmOutlineDesc);
                ol = XkbAddGeomOutline(shape, olWire.num_points);
                if (!ol) {
                    _XkbLibError(_XkbErrBadAlloc, "ReadXkmGeometry", 0);
                    return nRead;
                }
                ol->num_points = olWire.num_points;
                ol->corner_radius = olWire.corner_radius;
                for (p = 0; p < olWire.num_points; p++) {
                    tmp = fread(&ptWire, SIZEOF(xkmPointDesc), 1, file);
                    nRead += tmp * SIZEOF(xkmPointDesc);
                    ol->points[p].x = ptWire.x;
                    ol->points[p].y = ptWire.y;
                    if (ptWire.x < shape->bounds.x1)
                        shape->bounds.x1 = ptWire.x;
                    if (ptWire.x > shape->bounds.x2)
                        shape->bounds.x2 = ptWire.x;
                    if (ptWire.y < shape->bounds.y1)
                        shape->bounds.y1 = ptWire.y;
                    if (ptWire.y > shape->bounds.y2)
                        shape->bounds.y2 = ptWire.y;
                }
            }
            if (shapeWire.primary_ndx != XkbNoShape)
                shape->primary = &shape->outlines[shapeWire.primary_ndx];
            if (shapeWire.approx_ndx != XkbNoShape)
                shape->approx = &shape->outlines[shapeWire.approx_ndx];
        }
    }
    if (wireGeom.num_sections > 0) {
        for (i = 0; i < wireGeom.num_sections; i++) {
            tmp = ReadXkmGeomSection(file, result->xkb->dpy, geom);
            nRead += tmp;
            if (tmp == 0)
                return nRead;
        }
    }
    if (wireGeom.num_doodads > 0) {
        for (i = 0; i < wireGeom.num_doodads; i++) {
            tmp = ReadXkmGeomDoodad(file, result->xkb->dpy, geom, NULL);
            nRead += tmp;
            if (tmp == 0)
                return nRead;
        }
    }
    if ((wireGeom.num_key_aliases > 0) && (geom->key_aliases)) {
        int sz = XkbKeyNameLength * 2;
        int num = wireGeom.num_key_aliases;

        if (fread(geom->key_aliases, sz, num, file) != num) {
            _XkbLibError(_XkbErrBadLength, "ReadXkmGeometry", 0);
            return -1;
        }
        nRead += (num * sz);
        geom->num_key_aliases = num;
    }
    return nRead;
}

Bool
XkmProbe(FILE *file)
{
    unsigned hdr, tmp;
    int nRead = 0;

    hdr = (('x' << 24) | ('k' << 16) | ('m' << 8) | XkmFileVersion);
    tmp = XkmGetCARD32(file, &nRead);
    if (tmp != hdr) {
        if ((tmp & (~0xff)) == (hdr & (~0xff))) {
            _XkbLibError(_XkbErrBadFileVersion, "XkmProbe", tmp & 0xff);
        }
        return 0;
    }
    return 1;
}

Bool
XkmReadTOC(FILE *file, xkmFileInfo *file_info,
           int max_toc, xkmSectionInfo *toc)
{
    unsigned hdr, tmp;
    int nRead = 0;
    unsigned i, size_toc;

    hdr = (('x' << 24) | ('k' << 16) | ('m' << 8) | XkmFileVersion);
    tmp = XkmGetCARD32(file, &nRead);
    if (tmp != hdr) {
        if ((tmp & (~0xff)) == (hdr & (~0xff))) {
            _XkbLibError(_XkbErrBadFileVersion, "XkmReadTOC", tmp & 0xff);
        }
        else {
            _XkbLibError(_XkbErrBadFileType, "XkmReadTOC", tmp);
        }
        return 0;
    }
    fread(file_info, SIZEOF(xkmFileInfo), 1, file);
    size_toc = file_info->num_toc;
    if (size_toc > max_toc) {
#ifdef DEBUG
        fprintf(stderr, "Warning! Too many TOC entries; last %d ignored\n",
                size_toc - max_toc);
#endif
        size_toc = max_toc;
    }
    for (i = 0; i < size_toc; i++) {
        fread(&toc[i], SIZEOF(xkmSectionInfo), 1, file);
    }
    return 1;
}

xkmSectionInfo *
XkmFindTOCEntry(xkmFileInfo *finfo, xkmSectionInfo *toc, unsigned type)
{
    register int i;

    for (i = 0; i < finfo->num_toc; i++) {
        if (toc[i].type == type)
            return &toc[i];
    }
    return NULL;
}

Bool
XkmReadFileSection(FILE *           file,
                   xkmSectionInfo * toc,
                   XkbFileInfo *    result,
                   unsigned *       loaded_rtrn)
{
    xkmSectionInfo tmpTOC;
    int nRead;

    if ((!result) || (!result->xkb)) {
        _XkbLibError(_XkbErrBadMatch, "XkmReadFileSection", 0);
        return 0;
    }
    fseek(file, toc->offset, SEEK_SET);
    fread(&tmpTOC, SIZEOF(xkmSectionInfo), 1, file);
    nRead = SIZEOF(xkmSectionInfo);
    if ((tmpTOC.type != toc->type) || (tmpTOC.format != toc->format) ||
        (tmpTOC.size != toc->size) || (tmpTOC.offset != toc->offset)) {
        _XkbLibError(_XkbErrIllegalContents, "XkmReadFileSection", 0);
        return 0;
    }
    switch (tmpTOC.type) {
    case XkmVirtualModsIndex:
        nRead += ReadXkmVirtualMods(file, result, NULL);
        if ((loaded_rtrn) && (nRead >= 0))
            *loaded_rtrn |= XkmVirtualModsMask;
        break;
    case XkmTypesIndex:
        nRead += ReadXkmKeyTypes(file, result, NULL);
        if ((loaded_rtrn) && (nRead >= 0))
            *loaded_rtrn |= XkmTypesMask;
        break;
    case XkmCompatMapIndex:
        nRead += ReadXkmCompatMap(file, result, NULL);
        if ((loaded_rtrn) && (nRead >= 0))
            *loaded_rtrn |= XkmCompatMapMask;
        break;
    case XkmKeyNamesIndex:
        nRead += ReadXkmKeycodes(file, result, NULL);
        if ((loaded_rtrn) && (nRead >= 0))
            *loaded_rtrn |= XkmKeyNamesMask;
        break;
    case XkmSymbolsIndex:
        nRead += ReadXkmSymbols(file, result);
        if ((loaded_rtrn) && (nRead >= 0))
            *loaded_rtrn |= XkmSymbolsMask;
        break;
    case XkmIndicatorsIndex:
        nRead += ReadXkmIndicators(file, result, NULL);
        if ((loaded_rtrn) && (nRead >= 0))
            *loaded_rtrn |= XkmIndicatorsMask;
        break;
    case XkmGeometryIndex:
        nRead += ReadXkmGeometry(file, result);
        if ((loaded_rtrn) && (nRead >= 0))
            *loaded_rtrn |= XkmGeometryMask;
        break;
    default:
        _XkbLibError(_XkbErrBadImplementation,
                     XkbConfigText(tmpTOC.type, XkbMessage), 0);
        nRead = 0;
        break;
    }
    if (nRead != tmpTOC.size) {
        _XkbLibError(_XkbErrBadLength, XkbConfigText(tmpTOC.type, XkbMessage),
                     nRead - tmpTOC.size);
        return 0;
    }
    return (nRead >= 0);
}

char *
XkmReadFileSectionName(FILE *file, xkmSectionInfo *toc)
{
    xkmSectionInfo tmpTOC;
    char name[100];

    if ((!file) || (!toc))
        return NULL;
    switch (toc->type) {
    case XkmVirtualModsIndex:
    case XkmIndicatorsIndex:
        break;
    case XkmTypesIndex:
    case XkmCompatMapIndex:
    case XkmKeyNamesIndex:
    case XkmSymbolsIndex:
    case XkmGeometryIndex:
        fseek(file, toc->offset, SEEK_SET);
        fread(&tmpTOC, SIZEOF(xkmSectionInfo), 1, file);
        if ((tmpTOC.type != toc->type) || (tmpTOC.format != toc->format) ||
            (tmpTOC.size != toc->size) || (tmpTOC.offset != toc->offset)) {
            _XkbLibError(_XkbErrIllegalContents, "XkmReadFileSectionName", 0);
            return NULL;
        }
        if (XkmGetCountedString(file, name, 100) > 0)
            return _XkbDupString(name);
        break;
    default:
        _XkbLibError(_XkbErrBadImplementation,
                     XkbConfigText(tmpTOC.type, XkbMessage), 0);
        break;
    }
    return NULL;
}

/***====================================================================***/

#define	MAX_TOC	16
unsigned
XkmReadFile(FILE *file, unsigned need, unsigned want, XkbFileInfo *result)
{
    register unsigned i;
    xkmSectionInfo toc[MAX_TOC], tmpTOC;
    xkmFileInfo fileInfo;
    unsigned tmp, nRead = 0;
    unsigned which = need | want;

    if (!XkmReadTOC(file, &fileInfo, MAX_TOC, toc))
        return which;
    if ((fileInfo.present & need) != need) {
        _XkbLibError(_XkbErrIllegalContents, "XkmReadFile",
                     need & (~fileInfo.present));
        return which;
    }
    result->type = fileInfo.type;
    if (result->xkb == NULL)
        result->xkb = XkbAllocKeyboard();
    for (i = 0; i < fileInfo.num_toc; i++) {
        fseek(file, toc[i].offset, SEEK_SET);
        tmp = fread(&tmpTOC, SIZEOF(xkmSectionInfo), 1, file);
        nRead = tmp * SIZEOF(xkmSectionInfo);
        if ((tmpTOC.type != toc[i].type) || (tmpTOC.format != toc[i].format) ||
            (tmpTOC.size != toc[i].size) || (tmpTOC.offset != toc[i].offset)) {
            return which;
        }
        if ((which & (1 << tmpTOC.type)) == 0) {
            continue;
        }
        switch (tmpTOC.type) {
        case XkmVirtualModsIndex:
            tmp = ReadXkmVirtualMods(file, result, NULL);
            break;
        case XkmTypesIndex:
            tmp = ReadXkmKeyTypes(file, result, NULL);
            break;
        case XkmCompatMapIndex:
            tmp = ReadXkmCompatMap(file, result, NULL);
            break;
        case XkmKeyNamesIndex:
            tmp = ReadXkmKeycodes(file, result, NULL);
            break;
        case XkmIndicatorsIndex:
            tmp = ReadXkmIndicators(file, result, NULL);
            break;
        case XkmSymbolsIndex:
            tmp = ReadXkmSymbols(file, result);
            break;
        case XkmGeometryIndex:
            tmp = ReadXkmGeometry(file, result);
            break;
        default:
            _XkbLibError(_XkbErrBadImplementation,
                         XkbConfigText(tmpTOC.type, XkbMessage), 0);
            tmp = 0;
            break;
        }
        if (tmp > 0) {
            nRead += tmp;
            which &= ~(1 << toc[i].type);
            result->defined |= (1 << toc[i].type);
        }
        if (nRead != tmpTOC.size) {
            _XkbLibError(_XkbErrBadLength,
                         XkbConfigText(tmpTOC.type, XkbMessage),
                         nRead - tmpTOC.size);
        }
    }
    return which;
}
