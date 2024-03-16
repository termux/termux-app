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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xfuncs.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include "XKMformat.h"
#include "XKBfileInt.h"

typedef struct _XkmInfo {
    unsigned short bound_vmods;
    unsigned short named_vmods;
    unsigned char  num_bound;
    unsigned char  group_compat;
    unsigned short num_group_compat;
    unsigned short num_leds;
    int            total_vmodmaps;
} XkmInfo;

/***====================================================================***/

#define	xkmPutCARD8(f,v)	(putc(v,f),1)

static int
xkmPutCARD16(FILE *file, unsigned val)
{
    CARD16 tmp = val;

    fwrite(&tmp, 2, 1, file);
    return 2;
}

static int
xkmPutCARD32(FILE *file, unsigned long val)
{
    CARD32 tmp = val;

    fwrite(&tmp, 4, 1, file);
    return 4;
}

static int
xkmPutPadding(FILE *file, unsigned pad)
{
    int i;

    for (i = 0; i < pad; i++) {
        putc('\0', file);
    }
    return pad;
}

static int
xkmPutCountedBytes(FILE *file, char *ptr, unsigned count)
{
    register int nOut;
    register unsigned pad;

    if (count == 0)
        return xkmPutCARD32(file, (unsigned long) 0);

    xkmPutCARD16(file, count);
    nOut = fwrite(ptr, 1, count, file);
    if (nOut < 0)
        return 2;
    nOut = count + 2;
    pad = XkbPaddedSize(nOut) - nOut;
    if (pad)
        xkmPutPadding(file, pad);
    return nOut + pad;
}

static unsigned
xkmSizeCountedString(char *str)
{
    if (str == NULL)
        return 4;
    return XkbPaddedSize(strlen(str) + 2);
}

static int
xkmPutCountedString(FILE *file, char *str)
{
    if (str == NULL)
        return xkmPutCARD32(file, (unsigned long) 0);
    return xkmPutCountedBytes(file, str, strlen(str));
}

#define	xkmSizeCountedAtomString(d,a)	\
	xkmSizeCountedString(XkbAtomGetString((d),(a)))

#define	xkmPutCountedAtomString(d,f,a)	\
	xkmPutCountedString((f),XkbAtomGetString((d),(a)))

/***====================================================================***/

static unsigned
SizeXKMVirtualMods(XkbFileInfo *result, XkmInfo *info,
                   xkmSectionInfo *toc, int *offset_inout)
{
    Display *dpy;
    XkbDescPtr xkb;
    unsigned nBound, bound;
    unsigned nNamed, named, szNames;
    register unsigned i, bit;

    xkb = result->xkb;
    if ((!xkb) || (!xkb->names) || (!xkb->server)) {
        _XkbLibError(_XkbErrMissingVMods, "SizeXKMVirtualMods", 0);
        return 0;
    }
    dpy = xkb->dpy;
    bound = named = 0;
    for (i = nBound = nNamed = szNames = 0, bit = 1; i < XkbNumVirtualMods;
         i++, bit <<= 1) {
        if (xkb->server->vmods[i] != XkbNoModifierMask) {
            bound |= bit;
            nBound++;
        }
        if (xkb->names->vmods[i] != None) {
            named |= bit;
            szNames += xkmSizeCountedAtomString(dpy, xkb->names->vmods[i]);
            nNamed++;
        }
    }
    info->num_bound = nBound;
    info->bound_vmods = bound;
    info->named_vmods = named;
    if ((nBound == 0) && (nNamed == 0))
        return 0;
    toc->type = XkmVirtualModsIndex;
    toc->format = MSBFirst;
    toc->size = 4 + XkbPaddedSize(nBound) + szNames + SIZEOF(xkmSectionInfo);
    toc->offset = *offset_inout;
    (*offset_inout) += toc->size;
    return 1;
}

static unsigned
WriteXKMVirtualMods(FILE *file, XkbFileInfo *result, XkmInfo *info)
{
    register unsigned int i, bit;
    XkbDescPtr xkb;
    Display *dpy;
    unsigned size = 0;

    xkb = result->xkb;
    dpy = xkb->dpy;
    size += xkmPutCARD16(file, info->bound_vmods);
    size += xkmPutCARD16(file, info->named_vmods);
    for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (info->bound_vmods & bit)
            size += xkmPutCARD8(file, xkb->server->vmods[i]);
    }
    if ((i = XkbPaddedSize(info->num_bound) - info->num_bound) > 0)
        size += xkmPutPadding(file, i);
    for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (info->named_vmods & bit) {
            register char *name;

            name = XkbAtomGetString(dpy, xkb->names->vmods[i]);
            size += xkmPutCountedString(file, name);
        }
    }
    return size;
}

/***====================================================================***/

static unsigned
SizeXKMKeycodes(XkbFileInfo *result, xkmSectionInfo *toc, int *offset_inout)
{
    XkbDescPtr xkb;
    Atom kcName;
    int size = 0;
    Display *dpy;

    xkb = result->xkb;
    if ((!xkb) || (!xkb->names) || (!xkb->names->keys)) {
        _XkbLibError(_XkbErrMissingNames, "SizeXKMKeycodes", 0);
        return 0;
    }
    dpy = xkb->dpy;
    kcName = xkb->names->keycodes;
    size += 4;                  /* min and max keycode */
    size += xkmSizeCountedAtomString(dpy, kcName);
    size += XkbNumKeys(xkb) * sizeof(XkbKeyNameRec);
    if (xkb->names->num_key_aliases > 0) {
        if (xkb->names->key_aliases != NULL)
            size += xkb->names->num_key_aliases * sizeof(XkbKeyAliasRec);
        else
            xkb->names->num_key_aliases = 0;
    }
    toc->type = XkmKeyNamesIndex;
    toc->format = MSBFirst;
    toc->size = size + SIZEOF(xkmSectionInfo);
    toc->offset = (*offset_inout);
    (*offset_inout) += toc->size;
    return 1;
}

static unsigned
WriteXKMKeycodes(FILE *file, XkbFileInfo *result)
{
    XkbDescPtr xkb;
    Atom kcName;
    char *start;
    Display *dpy;
    unsigned tmp, size = 0;

    xkb = result->xkb;
    dpy = xkb->dpy;
    kcName = xkb->names->keycodes;
    start = xkb->names->keys[xkb->min_key_code].name;

    size += xkmPutCountedString(file, XkbAtomGetString(dpy, kcName));
    size += xkmPutCARD8(file, xkb->min_key_code);
    size += xkmPutCARD8(file, xkb->max_key_code);
    size += xkmPutCARD8(file, xkb->names->num_key_aliases);
    size += xkmPutPadding(file, 1);
    tmp = fwrite(start, sizeof(XkbKeyNameRec), XkbNumKeys(xkb), file);
    size += tmp * sizeof(XkbKeyNameRec);
    if (xkb->names->num_key_aliases > 0) {
        tmp = fwrite((char *) xkb->names->key_aliases,
                     sizeof(XkbKeyAliasRec), xkb->names->num_key_aliases, file);
        size += tmp * sizeof(XkbKeyAliasRec);
    }
    return size;
}

/***====================================================================***/

static unsigned
SizeXKMKeyTypes(XkbFileInfo *result, xkmSectionInfo *toc, int *offset_inout)
{
    register unsigned i, n, size;
    XkbKeyTypePtr type;
    XkbDescPtr xkb;
    Display *dpy;
    char *name;

    xkb = result->xkb;
    if ((!xkb) || (!xkb->map) || (!xkb->map->types)) {
        _XkbLibError(_XkbErrMissingTypes, "SizeXKBKeyTypes", 0);
        return 0;
    }
    dpy = xkb->dpy;
    if (xkb->map->num_types < XkbNumRequiredTypes) {
        _XkbLibError(_XkbErrMissingReqTypes, "SizeXKBKeyTypes", 0);
        return 0;
    }
    if (xkb->names)
        name = XkbAtomGetString(dpy, xkb->names->types);
    else
        name = NULL;
    size = xkmSizeCountedString(name);
    size += 4;                  /* room for # of key types + padding */
    for (i = 0, type = xkb->map->types; i < xkb->map->num_types; i++, type++) {
        size += SIZEOF(xkmKeyTypeDesc);
        size += SIZEOF(xkmKTMapEntryDesc) * type->map_count;
        size += xkmSizeCountedAtomString(dpy, type->name);
        if (type->preserve)
            size += SIZEOF(xkmModsDesc) * type->map_count;
        if (type->level_names) {
            Atom *names;

            names = type->level_names;
            for (n = 0; n < (unsigned) type->num_levels; n++) {
                size += xkmSizeCountedAtomString(dpy, names[n]);
            }
        }
    }
    toc->type = XkmTypesIndex;
    toc->format = MSBFirst;
    toc->size = size + SIZEOF(xkmSectionInfo);
    toc->offset = (*offset_inout);
    (*offset_inout) += toc->size;
    return 1;
}

static unsigned
WriteXKMKeyTypes(FILE *file, XkbFileInfo *result)
{
    register unsigned i, n;
    XkbDescPtr xkb;
    XkbKeyTypePtr type;
    xkmKeyTypeDesc wire;
    XkbKTMapEntryPtr entry;
    xkmKTMapEntryDesc wire_entry;
    Atom *names;
    Display *dpy;
    unsigned tmp, size = 0;
    char *name;

    xkb = result->xkb;
    dpy = xkb->dpy;
    if (xkb->names)
        name = XkbAtomGetString(dpy, xkb->names->types);
    else
        name = NULL;
    size += xkmPutCountedString(file, name);
    size += xkmPutCARD16(file, xkb->map->num_types);
    size += xkmPutPadding(file, 2);
    type = xkb->map->types;
    for (i = 0; i < xkb->map->num_types; i++, type++) {
        wire.realMods = type->mods.real_mods;
        wire.virtualMods = type->mods.vmods;
        wire.numLevels = type->num_levels;
        wire.nMapEntries = type->map_count;
        wire.preserve = (type->preserve != NULL);
        if (type->level_names != NULL)
            wire.nLevelNames = type->num_levels;
        else
            wire.nLevelNames = 0;
        tmp = fwrite(&wire, SIZEOF(xkmKeyTypeDesc), 1, file);
        size += tmp * SIZEOF(xkmKeyTypeDesc);
        for (n = 0, entry = type->map; n < type->map_count; n++, entry++) {
            wire_entry.level = entry->level;
            wire_entry.realMods = entry->mods.real_mods;
            wire_entry.virtualMods = entry->mods.vmods;
            tmp = fwrite(&wire_entry, SIZEOF(xkmKTMapEntryDesc), 1, file);
            size += tmp * SIZEOF(xkmKTMapEntryDesc);
        }
        size += xkmPutCountedString(file, XkbAtomGetString(dpy, type->name));
        if (type->preserve) {
            xkmModsDesc p_entry;

            XkbModsPtr pre;

            for (n = 0, pre = type->preserve; n < type->map_count; n++, pre++) {
                p_entry.realMods = pre->real_mods;
                p_entry.virtualMods = pre->vmods;
                tmp = fwrite(&p_entry, SIZEOF(xkmModsDesc), 1, file);
                size += tmp * SIZEOF(xkmModsDesc);
            }
        }
        if (type->level_names != NULL) {
            names = type->level_names;
            for (n = 0; n < wire.nLevelNames; n++) {
                size +=
                    xkmPutCountedString(file, XkbAtomGetString(dpy, names[n]));
            }
        }
    }
    return size;
}

/***====================================================================***/

static unsigned
SizeXKMCompatMap(XkbFileInfo *result, XkmInfo *info,
                 xkmSectionInfo *toc, int *offset_inout)
{
    XkbDescPtr xkb;
    char *name;
    int size;
    register int i;
    unsigned groups, nGroups;
    Display *dpy;

    xkb = result->xkb;
    if ((!xkb) || (!xkb->compat) || (!xkb->compat->sym_interpret)) {
        _XkbLibError(_XkbErrMissingCompatMap, "SizeXKMCompatMap", 0);
        return 0;
    }
    dpy = xkb->dpy;
    if (xkb->names)
        name = XkbAtomGetString(dpy, xkb->names->compat);
    else
        name = NULL;

    for (i = groups = nGroups = 0; i < XkbNumKbdGroups; i++) {
        if ((xkb->compat->groups[i].real_mods != 0) ||
            (xkb->compat->groups[i].vmods != 0)) {
            groups |= (1 << i);
            nGroups++;
        }
    }
    info->group_compat = groups;
    info->num_group_compat = nGroups;
    size = 4;                   /* room for num_si and group_compat mask */
    size += xkmSizeCountedString(name);
    size += (SIZEOF(xkmSymInterpretDesc) * xkb->compat->num_si);
    size += (SIZEOF(xkmModsDesc) * nGroups);
    toc->type = XkmCompatMapIndex;
    toc->format = MSBFirst;
    toc->size = size + SIZEOF(xkmSectionInfo);
    toc->offset = (*offset_inout);
    (*offset_inout) += toc->size;
    return 1;
}

static unsigned
WriteXKMCompatMap(FILE *file, XkbFileInfo *result, XkmInfo *info)
{
    register unsigned i;
    char *name;
    XkbDescPtr xkb;
    XkbSymInterpretPtr interp;
    xkmSymInterpretDesc wire;
    Display *dpy;
    unsigned tmp, size = 0;

    xkb = result->xkb;
    dpy = xkb->dpy;
    if (xkb->names)
        name = XkbAtomGetString(dpy, xkb->names->compat);
    else
        name = NULL;
    size += xkmPutCountedString(file, name);
    size += xkmPutCARD16(file, xkb->compat->num_si);
    size += xkmPutCARD8(file, info->group_compat);
    size += xkmPutPadding(file, 1);
    interp = xkb->compat->sym_interpret;
    for (i = 0; i < xkb->compat->num_si; i++, interp++) {
        wire.sym = interp->sym;
        wire.mods = interp->mods;
        wire.match = interp->match;
        wire.virtualMod = interp->virtual_mod;
        wire.flags = interp->flags;
        wire.actionType = interp->act.type;
        wire.actionData[0] = interp->act.data[0];
        wire.actionData[1] = interp->act.data[1];
        wire.actionData[2] = interp->act.data[2];
        wire.actionData[3] = interp->act.data[3];
        wire.actionData[4] = interp->act.data[4];
        wire.actionData[5] = interp->act.data[5];
        wire.actionData[6] = interp->act.data[6];
        tmp = fwrite(&wire, SIZEOF(xkmSymInterpretDesc), 1, file);
        size += tmp * SIZEOF(xkmSymInterpretDesc);
    }
    if (info->group_compat) {
        register unsigned bit;

        xkmModsDesc modsWire;

        for (i = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
            if (info->group_compat & bit) {
                modsWire.realMods = xkb->compat->groups[i].real_mods;
                modsWire.virtualMods = xkb->compat->groups[i].vmods;
                fwrite(&modsWire, SIZEOF(xkmModsDesc), 1, file);
                size += SIZEOF(xkmModsDesc);
            }
        }
    }
    return size;
}

/***====================================================================***/

static unsigned
SizeXKMSymbols(XkbFileInfo *result, XkmInfo *info,
               xkmSectionInfo *toc, int *offset_inout)
{
    Display *dpy;
    XkbDescPtr xkb;
    unsigned size;
    register int i, nSyms;
    char *name;

    xkb = result->xkb;
    if ((!xkb) || (!xkb->map) || ((!xkb->map->syms))) {
        _XkbLibError(_XkbErrMissingSymbols, "SizeXKMSymbols", 0);
        return 0;
    }
    dpy = xkb->dpy;
    if (xkb->names && (xkb->names->symbols != None))
        name = XkbAtomGetString(dpy, xkb->names->symbols);
    else
        name = NULL;
    size = xkmSizeCountedString(name);
    size += 4;                  /* min and max keycode, group names mask */
    for (i = 0; i < XkbNumKbdGroups; i++) {
        if (xkb->names->groups[i] != None)
            size += xkmSizeCountedAtomString(dpy, xkb->names->groups[i]);
    }
    info->total_vmodmaps = 0;
    for (i = xkb->min_key_code; i <= (int) xkb->max_key_code; i++) {
        nSyms = XkbKeyNumSyms(xkb, i);
        size += SIZEOF(xkmKeySymMapDesc) + (nSyms * 4);
        if (xkb->server) {
            if (xkb->server->explicit[i] & XkbExplicitKeyTypesMask) {
                register int g;

                for (g = XkbKeyNumGroups(xkb, i) - 1; g >= 0; g--) {
                    if (xkb->server->explicit[i] & (1 << g)) {
                        XkbKeyTypePtr type;
                        char *name;

                        type = XkbKeyKeyType(xkb, i, g);
                        name = XkbAtomGetString(dpy, type->name);
                        if (name != NULL)
                            size += xkmSizeCountedString(name);
                    }
                }
            }
            if (XkbKeyHasActions(xkb, i))
                size += nSyms * SIZEOF(xkmActionDesc);
            if (xkb->server->behaviors[i].type != XkbKB_Default)
                size += SIZEOF(xkmBehaviorDesc);
            if (xkb->server->vmodmap && (xkb->server->vmodmap[i] != 0))
                info->total_vmodmaps++;
        }
    }
    size += info->total_vmodmaps * SIZEOF(xkmVModMapDesc);
    toc->type = XkmSymbolsIndex;
    toc->format = MSBFirst;
    toc->size = size + SIZEOF(xkmSectionInfo);
    toc->offset = (*offset_inout);
    (*offset_inout) += toc->size;
    return 1;
}

static unsigned
WriteXKMSymbols(FILE *file, XkbFileInfo *result, XkmInfo *info)
{
    Display *dpy;
    XkbDescPtr xkb;
    register int i, n;
    xkmKeySymMapDesc wireMap;
    char *name;
    unsigned tmp, size = 0;

    xkb = result->xkb;
    dpy = xkb->dpy;
    if (xkb->names && (xkb->names->symbols != None))
        name = XkbAtomGetString(dpy, xkb->names->symbols);
    else
        name = NULL;
    size += xkmPutCountedString(file, name);
    for (tmp = i = 0; i < XkbNumKbdGroups; i++) {
        if (xkb->names->groups[i] != None)
            tmp |= (1 << i);
    }
    size += xkmPutCARD8(file, xkb->min_key_code);
    size += xkmPutCARD8(file, xkb->max_key_code);
    size += xkmPutCARD8(file, tmp);
    size += xkmPutCARD8(file, info->total_vmodmaps);
    for (i = 0, n = 1; i < XkbNumKbdGroups; i++, n <<= 1) {
        if ((tmp & n) == 0)
            continue;
        size += xkmPutCountedAtomString(dpy, file, xkb->names->groups[i]);
    }
    for (i = xkb->min_key_code; i <= (int) xkb->max_key_code; i++) {
        char *typeName[XkbNumKbdGroups];

        wireMap.width = XkbKeyGroupsWidth(xkb, i);
        wireMap.num_groups = XkbKeyGroupInfo(xkb, i);
        if (xkb->map && xkb->map->modmap)
            wireMap.modifier_map = xkb->map->modmap[i];
        else
            wireMap.modifier_map = 0;
        wireMap.flags = 0;
        bzero((char *) typeName, XkbNumKbdGroups * sizeof(char *));
        if (xkb->server) {
            if (xkb->server->explicit[i] & XkbExplicitKeyTypesMask) {
                register int g;

                for (g = 0; g < XkbKeyNumGroups(xkb, i); g++) {
                    if (xkb->server->explicit[i] & (1 << g)) {
                        XkbKeyTypePtr type;

                        type = XkbKeyKeyType(xkb, i, g);
                        typeName[g] = XkbAtomGetString(dpy, type->name);
                        if (typeName[g] != NULL)
                            wireMap.flags |= (1 << g);
                    }
                }
            }
            if (XkbKeyHasActions(xkb, i))
                wireMap.flags |= XkmKeyHasActions;
            if (xkb->server->behaviors[i].type != XkbKB_Default)
                wireMap.flags |= XkmKeyHasBehavior;
            if ((xkb->server->explicit[i] & XkbExplicitAutoRepeatMask) &&
                (xkb->ctrls != NULL)) {
                if (xkb->ctrls->per_key_repeat[(i / 8)] & (1 << (i % 8)))
                    wireMap.flags |= XkmRepeatingKey;
                else
                    wireMap.flags |= XkmNonRepeatingKey;
            }
        }
        tmp = fwrite(&wireMap, SIZEOF(xkmKeySymMapDesc), 1, file);
        size += tmp * SIZEOF(xkmKeySymMapDesc);
        if (xkb->server->explicit[i] & XkbExplicitKeyTypesMask) {
            register int g;

            for (g = 0; g < XkbNumKbdGroups; g++) {
                if (typeName[g] != NULL)
                    size += xkmPutCountedString(file, typeName[g]);
            }
        }
        if (XkbNumGroups(wireMap.num_groups) > 0) {
            KeySym *sym;

            sym = XkbKeySymsPtr(xkb, i);
            for (n = XkbKeyNumSyms(xkb, i); n > 0; n--, sym++) {
                size += xkmPutCARD32(file, (CARD32) *sym);
            }
            if (wireMap.flags & XkmKeyHasActions) {
                XkbAction *act;

                act = XkbKeyActionsPtr(xkb, i);
                for (n = XkbKeyNumActions(xkb, i); n > 0; n--, act++) {
                    tmp = fwrite(act, SIZEOF(xkmActionDesc), 1, file);
                    size += tmp * SIZEOF(xkmActionDesc);
                }
            }
        }
        if (wireMap.flags & XkmKeyHasBehavior) {
            xkmBehaviorDesc b;

            b.type = xkb->server->behaviors[i].type;
            b.data = xkb->server->behaviors[i].data;
            tmp = fwrite(&b, SIZEOF(xkmBehaviorDesc), 1, file);
            size += tmp * SIZEOF(xkmBehaviorDesc);
        }
    }
    if (info->total_vmodmaps > 0) {
        xkmVModMapDesc v;

        for (i = xkb->min_key_code; i <= xkb->max_key_code; i++) {
            if (xkb->server->vmodmap[i] != 0) {
                v.key = i;
                v.vmods = xkb->server->vmodmap[i];
                tmp = fwrite(&v, SIZEOF(xkmVModMapDesc), 1, file);
                size += tmp * SIZEOF(xkmVModMapDesc);
            }
        }
    }
    return size;
}

/***====================================================================***/

static unsigned
SizeXKMIndicators(XkbFileInfo *result, XkmInfo *info,
                  xkmSectionInfo *toc, int *offset_inout)
{
    Display *dpy;
    XkbDescPtr xkb;
    unsigned size;
    register unsigned i, nLEDs;

    xkb = result->xkb;
    if ((xkb == NULL) || (xkb->indicators == NULL)) {
/*	_XkbLibError(_XkbErrMissingIndicators,"SizeXKMIndicators",0);*/
        return 0;
    }
    dpy = xkb->dpy;
    nLEDs = 0;
    size = 8; /* number of indicator maps/physical indicators */
    if (xkb->indicators != NULL) {
        for (i = 0; i < XkbNumIndicators; i++) {
            XkbIndicatorMapPtr map = &xkb->indicators->maps[i];

            if ((map->flags != 0) || (map->which_groups != 0) ||
                (map->groups != 0) || (map->which_mods != 0) ||
                (map->mods.real_mods != 0) || (map->mods.vmods != 0) ||
                (map->ctrls != 0) ||
                (xkb->names && (xkb->names->indicators[i] != None))) {
                char *name;

                if (xkb->names && xkb->names->indicators[i] != None) {
                    name = XkbAtomGetString(dpy, xkb->names->indicators[i]);
                }
                else
                    name = NULL;
                size += xkmSizeCountedString(name);
                size += SIZEOF(xkmIndicatorMapDesc);
                nLEDs++;
            }
        }
    }
    info->num_leds = nLEDs;
    toc->type = XkmIndicatorsIndex;
    toc->format = MSBFirst;
    toc->size = size + SIZEOF(xkmSectionInfo);
    toc->offset = (*offset_inout);
    (*offset_inout) += toc->size;
    return 1;
}

static unsigned
WriteXKMIndicators(FILE *file, XkbFileInfo *result, XkmInfo *info)
{
    Display *dpy;
    XkbDescPtr xkb;
    register unsigned i;
    xkmIndicatorMapDesc wire;
    unsigned tmp, size = 0;

    xkb = result->xkb;
    dpy = xkb->dpy;
    size += xkmPutCARD8(file, info->num_leds);
    size += xkmPutPadding(file, 3);
    size += xkmPutCARD32(file, xkb->indicators->phys_indicators);
    if (xkb->indicators != NULL) {
        for (i = 0; i < XkbNumIndicators; i++) {
            XkbIndicatorMapPtr map = &xkb->indicators->maps[i];

            if ((map->flags != 0) || (map->which_groups != 0) ||
                (map->groups != 0) || (map->which_mods != 0) ||
                (map->mods.real_mods != 0) || (map->mods.vmods != 0) ||
                (map->ctrls != 0) || (xkb->names &&
                                      (xkb->names->indicators[i] != None))) {
                char *name;

                if (xkb->names && xkb->names->indicators[i] != None) {
                    name = XkbAtomGetString(dpy, xkb->names->indicators[i]);
                }
                else
                    name = NULL;
                size += xkmPutCountedString(file, name);
                wire.indicator = i + 1;
                wire.flags = map->flags;
                wire.which_mods = map->which_mods;
                wire.real_mods = map->mods.real_mods;
                wire.vmods = map->mods.vmods;
                wire.which_groups = map->which_groups;
                wire.groups = map->groups;
                wire.ctrls = map->ctrls;
                tmp = fwrite(&wire, SIZEOF(xkmIndicatorMapDesc), 1, file);
                size += tmp * SIZEOF(xkmIndicatorMapDesc);
            }
        }
    }
    return size;
}

/***====================================================================***/

static unsigned
SizeXKMGeomDoodad(XkbFileInfo *result, XkbDoodadPtr doodad)
{
    unsigned size;

    size = SIZEOF(xkmAnyDoodadDesc);
    size += xkmSizeCountedAtomString(result->xkb->dpy, doodad->any.name);
    if (doodad->any.type == XkbTextDoodad) {
        size += xkmSizeCountedString(doodad->text.text);
        size += xkmSizeCountedString(doodad->text.font);
    }
    else if (doodad->any.type == XkbLogoDoodad) {
        size += xkmSizeCountedString(doodad->logo.logo_name);
    }
    return size;
}

static unsigned
SizeXKMGeomSection(XkbFileInfo *result, XkbSectionPtr section)
{
    register int i;
    unsigned size;

    size = SIZEOF(xkmSectionDesc);
    size += xkmSizeCountedAtomString(result->xkb->dpy, section->name);
    if (section->rows) {
        XkbRowPtr row;

        for (row = section->rows, i = 0; i < section->num_rows; i++, row++) {
            size += SIZEOF(xkmRowDesc);
            size += row->num_keys * SIZEOF(xkmKeyDesc);
        }
    }
    if (section->doodads) {
        XkbDoodadPtr doodad;

        for (doodad = section->doodads, i = 0; i < section->num_doodads;
             i++, doodad++) {
            size += SizeXKMGeomDoodad(result, doodad);
        }
    }
    if (section->overlays) {
        XkbOverlayPtr ol;

        for (ol = section->overlays, i = 0; i < section->num_overlays;
             i++, ol++) {
            register int r;
            XkbOverlayRowPtr row;

            size += xkmSizeCountedAtomString(result->xkb->dpy, ol->name);
            size += SIZEOF(xkmOverlayDesc);
            for (r = 0, row = ol->rows; r < ol->num_rows; r++, row++) {
                size += SIZEOF(xkmOverlayRowDesc);
                size += row->num_keys * SIZEOF(xkmOverlayKeyDesc);
            }
        }
    }
    return size;
}

static unsigned
SizeXKMGeometry(XkbFileInfo *result, xkmSectionInfo *toc, int *offset_inout)
{
    register int i;
    Display *dpy;
    XkbDescPtr xkb;
    XkbGeometryPtr geom;
    unsigned size;

    xkb = result->xkb;
    if ((!xkb) || (!xkb->geom))
        return 0;
    dpy = xkb->dpy;
    geom = xkb->geom;
    size = xkmSizeCountedAtomString(dpy, geom->name);
    size += SIZEOF(xkmGeometryDesc);
    size += xkmSizeCountedString(geom->label_font);
    if (geom->properties) {
        XkbPropertyPtr prop;

        for (i = 0, prop = geom->properties; i < geom->num_properties;
             i++, prop++) {
            size += xkmSizeCountedString(prop->name);
            size += xkmSizeCountedString(prop->value);
        }
    }
    if (geom->colors) {
        XkbColorPtr color;

        for (i = 0, color = geom->colors; i < geom->num_colors; i++, color++) {
            size += xkmSizeCountedString(color->spec);
        }
    }
    if (geom->shapes) {
        XkbShapePtr shape;

        for (i = 0, shape = geom->shapes; i < geom->num_shapes; i++, shape++) {
            register int n;
            register XkbOutlinePtr ol;

            size += xkmSizeCountedAtomString(dpy, shape->name);
            size += SIZEOF(xkmShapeDesc);
            for (n = 0, ol = shape->outlines; n < shape->num_outlines;
                 n++, ol++) {
                size += SIZEOF(xkmOutlineDesc);
                size += ol->num_points * SIZEOF(xkmPointDesc);
            }
        }
    }
    if (geom->sections) {
        XkbSectionPtr section;

        for (i = 0, section = geom->sections; i < geom->num_sections;
             i++, section++) {
            size += SizeXKMGeomSection(result, section);
        }
    }
    if (geom->doodads) {
        XkbDoodadPtr doodad;

        for (i = 0, doodad = geom->doodads; i < geom->num_doodads;
             i++, doodad++) {
            size += SizeXKMGeomDoodad(result, doodad);
        }
    }
    if (geom->key_aliases) {
        size += geom->num_key_aliases * (XkbKeyNameLength * 2);
    }
    toc->type = XkmGeometryIndex;
    toc->format = MSBFirst;
    toc->size = size + SIZEOF(xkmSectionInfo);
    toc->offset = (*offset_inout);
    (*offset_inout) += toc->size;
    return 1;
}

static unsigned
WriteXKMGeomDoodad(FILE *file, XkbFileInfo *result, XkbDoodadPtr doodad)
{
    Display *dpy;
    XkbDescPtr xkb;
    xkmDoodadDesc doodadWire;
    unsigned tmp, size = 0;

    xkb = result->xkb;
    dpy = xkb->dpy;
    bzero((char *) &doodadWire, sizeof(doodadWire));
    doodadWire.any.type = doodad->any.type;
    doodadWire.any.priority = doodad->any.priority;
    doodadWire.any.top = doodad->any.top;
    doodadWire.any.left = doodad->any.left;
    switch (doodad->any.type) {
    case XkbOutlineDoodad:
    case XkbSolidDoodad:
        doodadWire.shape.angle = doodad->shape.angle;
        doodadWire.shape.color_ndx = doodad->shape.color_ndx;
        doodadWire.shape.shape_ndx = doodad->shape.shape_ndx;
        break;
    case XkbTextDoodad:
        doodadWire.text.angle = doodad->text.angle;
        doodadWire.text.width = doodad->text.width;
        doodadWire.text.height = doodad->text.height;
        doodadWire.text.color_ndx = doodad->text.color_ndx;
        break;
    case XkbIndicatorDoodad:
        doodadWire.indicator.shape_ndx = doodad->indicator.shape_ndx;
        doodadWire.indicator.on_color_ndx = doodad->indicator.on_color_ndx;
        doodadWire.indicator.off_color_ndx = doodad->indicator.off_color_ndx;
        break;
    case XkbLogoDoodad:
        doodadWire.logo.angle = doodad->logo.angle;
        doodadWire.logo.color_ndx = doodad->logo.color_ndx;
        doodadWire.logo.shape_ndx = doodad->logo.shape_ndx;
        break;
    default:
        _XkbLibError(_XkbErrIllegalDoodad, "WriteXKMGeomDoodad",
                     doodad->any.type);
        return 0;
    }
    size += xkmPutCountedAtomString(dpy, file, doodad->any.name);
    tmp = fwrite(&doodadWire, SIZEOF(xkmDoodadDesc), 1, file);
    size += tmp * SIZEOF(xkmDoodadDesc);
    if (doodad->any.type == XkbTextDoodad) {
        size += xkmPutCountedString(file, doodad->text.text);
        size += xkmPutCountedString(file, doodad->text.font);
    }
    else if (doodad->any.type == XkbLogoDoodad) {
        size += xkmPutCountedString(file, doodad->logo.logo_name);
    }
    return size;
}

static unsigned
WriteXKMGeomOverlay(FILE *file, XkbFileInfo *result, XkbOverlayPtr ol)
{
    register int r, k;
    Display *dpy;
    XkbDescPtr xkb;
    XkbOverlayRowPtr row;
    xkmOverlayDesc olWire;
    xkmOverlayRowDesc rowWire;
    xkmOverlayKeyDesc keyWire;
    unsigned tmp, size = 0;

    xkb = result->xkb;
    dpy = xkb->dpy;
    bzero((char *) &olWire, sizeof(olWire));
    bzero((char *) &rowWire, sizeof(rowWire));
    bzero((char *) &keyWire, sizeof(keyWire));
    size += xkmPutCountedAtomString(dpy, file, ol->name);
    olWire.num_rows = ol->num_rows;
    tmp = fwrite(&olWire, SIZEOF(xkmOverlayDesc), 1, file);
    size += tmp * SIZEOF(xkmOverlayDesc);
    for (r = 0, row = ol->rows; r < ol->num_rows; r++, row++) {
        XkbOverlayKeyPtr key;

        rowWire.row_under = row->row_under;
        rowWire.num_keys = row->num_keys;
        tmp = fwrite(&rowWire, SIZEOF(xkmOverlayRowDesc), 1, file);
        size += tmp * SIZEOF(xkmOverlayRowDesc);
        for (k = 0, key = row->keys; k < row->num_keys; k++, key++) {
            memcpy(keyWire.over, key->over.name, XkbKeyNameLength);
            memcpy(keyWire.under, key->under.name, XkbKeyNameLength);
            tmp = fwrite(&keyWire, SIZEOF(xkmOverlayKeyDesc), 1, file);
            size += tmp * SIZEOF(xkmOverlayKeyDesc);
        }
    }
    return size;
}

static unsigned
WriteXKMGeomSection(FILE *file, XkbFileInfo *result, XkbSectionPtr section)
{
    register int i;
    Display *dpy;
    XkbDescPtr xkb;
    xkmSectionDesc sectionWire;
    unsigned tmp, size = 0;

    xkb = result->xkb;
    dpy = xkb->dpy;
    size += xkmPutCountedAtomString(dpy, file, section->name);
    sectionWire.top = section->top;
    sectionWire.left = section->left;
    sectionWire.width = section->width;
    sectionWire.height = section->height;
    sectionWire.angle = section->angle;
    sectionWire.priority = section->priority;
    sectionWire.num_rows = section->num_rows;
    sectionWire.num_doodads = section->num_doodads;
    sectionWire.num_overlays = section->num_overlays;
    tmp = fwrite(&sectionWire, SIZEOF(xkmSectionDesc), 1, file);
    size += tmp * SIZEOF(xkmSectionDesc);
    if (section->rows) {
        register unsigned k;
        XkbRowPtr row;
        xkmRowDesc rowWire;
        XkbKeyPtr key;
        xkmKeyDesc keyWire;

        for (i = 0, row = section->rows; i < section->num_rows; i++, row++) {
            rowWire.top = row->top;
            rowWire.left = row->left;
            rowWire.num_keys = row->num_keys;
            rowWire.vertical = row->vertical;
            tmp = fwrite(&rowWire, SIZEOF(xkmRowDesc), 1, file);
            size += tmp * SIZEOF(xkmRowDesc);
            for (k = 0, key = row->keys; k < row->num_keys; k++, key++) {
                memcpy(keyWire.name, key->name.name, XkbKeyNameLength);
                keyWire.gap = key->gap;
                keyWire.shape_ndx = key->shape_ndx;
                keyWire.color_ndx = key->color_ndx;
                tmp = fwrite(&keyWire, SIZEOF(xkmKeyDesc), 1, file);
                size += tmp * SIZEOF(xkmKeyDesc);
            }
        }
    }
    if (section->doodads) {
        XkbDoodadPtr doodad;

        for (i = 0, doodad = section->doodads; i < section->num_doodads;
             i++, doodad++) {
            size += WriteXKMGeomDoodad(file, result, doodad);
        }
    }
    if (section->overlays) {
        XkbOverlayPtr ol;

        for (i = 0, ol = section->overlays; i < section->num_overlays;
             i++, ol++) {
            size += WriteXKMGeomOverlay(file, result, ol);
        }
    }
    return size;
}

static unsigned
WriteXKMGeometry(FILE *file, XkbFileInfo *result)
{
    register int i;
    Display *dpy;
    XkbDescPtr xkb;
    XkbGeometryPtr geom;
    xkmGeometryDesc wire;
    unsigned tmp, size = 0;

    xkb = result->xkb;
    if ((!xkb) || (!xkb->geom))
        return 0;
    dpy = xkb->dpy;
    geom = xkb->geom;
    wire.width_mm = geom->width_mm;
    wire.height_mm = geom->height_mm;
    wire.base_color_ndx = XkbGeomColorIndex(geom, geom->base_color);
    wire.label_color_ndx = XkbGeomColorIndex(geom, geom->label_color);
    wire.num_properties = geom->num_properties;
    wire.num_colors = geom->num_colors;
    wire.num_shapes = geom->num_shapes;
    wire.num_sections = geom->num_sections;
    wire.num_doodads = geom->num_doodads;
    wire.num_key_aliases = geom->num_key_aliases;
    size += xkmPutCountedAtomString(dpy, file, geom->name);
    tmp = fwrite(&wire, SIZEOF(xkmGeometryDesc), 1, file);
    size += tmp * SIZEOF(xkmGeometryDesc);
    size += xkmPutCountedString(file, geom->label_font);
    if (geom->properties) {
        XkbPropertyPtr prop;

        for (i = 0, prop = geom->properties; i < geom->num_properties;
             i++, prop++) {
            size += xkmPutCountedString(file, prop->name);
            size += xkmPutCountedString(file, prop->value);
        }
    }
    if (geom->colors) {
        XkbColorPtr color;

        for (i = 0, color = geom->colors; i < geom->num_colors; i++, color++) {
            size += xkmPutCountedString(file, color->spec);
        }
    }
    if (geom->shapes) {
        XkbShapePtr shape;
        xkmShapeDesc shapeWire;

        for (i = 0, shape = geom->shapes; i < geom->num_shapes; i++, shape++) {
            register int n;
            XkbOutlinePtr ol;
            xkmOutlineDesc olWire;

            bzero((char *) &shapeWire, sizeof(xkmShapeDesc));
            size += xkmPutCountedAtomString(dpy, file, shape->name);
            shapeWire.num_outlines = shape->num_outlines;
            if (shape->primary != NULL)
                shapeWire.primary_ndx = XkbOutlineIndex(shape, shape->primary);
            else
                shapeWire.primary_ndx = XkbNoShape;
            if (shape->approx != NULL)
                shapeWire.approx_ndx = XkbOutlineIndex(shape, shape->approx);
            else
                shapeWire.approx_ndx = XkbNoShape;
            tmp = fwrite(&shapeWire, SIZEOF(xkmShapeDesc), 1, file);
            size += tmp * SIZEOF(xkmShapeDesc);
            for (n = 0, ol = shape->outlines; n < shape->num_outlines;
                 n++, ol++) {
                register int p;
                XkbPointPtr pt;
                xkmPointDesc ptWire;

                olWire.num_points = ol->num_points;
                olWire.corner_radius = ol->corner_radius;
                tmp = fwrite(&olWire, SIZEOF(xkmOutlineDesc), 1, file);
                size += tmp * SIZEOF(xkmOutlineDesc);
                for (p = 0, pt = ol->points; p < ol->num_points; p++, pt++) {
                    ptWire.x = pt->x;
                    ptWire.y = pt->y;
                    tmp = fwrite(&ptWire, SIZEOF(xkmPointDesc), 1, file);
                    size += tmp * SIZEOF(xkmPointDesc);
                }
            }
        }
    }
    if (geom->sections) {
        XkbSectionPtr section;

        for (i = 0, section = geom->sections; i < geom->num_sections;
             i++, section++) {
            size += WriteXKMGeomSection(file, result, section);
        }
    }
    if (geom->doodads) {
        XkbDoodadPtr doodad;

        for (i = 0, doodad = geom->doodads; i < geom->num_doodads;
             i++, doodad++) {
            size += WriteXKMGeomDoodad(file, result, doodad);
        }
    }
    if (geom->key_aliases) {
        tmp =
            fwrite(geom->key_aliases, 2 * XkbKeyNameLength,
                   geom->num_key_aliases, file);
        size += tmp * (2 * XkbKeyNameLength);
    }
    return size;
}

/***====================================================================***/

/*ARGSUSED*/
static int
GetXKMKeyNamesTOC(XkbFileInfo *result, XkmInfo *info,
                  int max_toc, xkmSectionInfo *toc_rtrn)
{
    int num_toc;
    int total_size;

    total_size = num_toc = 0;
    if (SizeXKMKeycodes(result, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMIndicators(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    return num_toc;
}

/*ARGSUSED*/
static int
GetXKMTypesTOC(XkbFileInfo *result, XkmInfo *info,
               int max_toc, xkmSectionInfo *toc_rtrn)
{
    int num_toc;
    int total_size;

    total_size = num_toc = 0;
    if (SizeXKMVirtualMods(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMKeyTypes(result, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    return num_toc;
}

/*ARGSUSED*/
static int
GetXKMCompatMapTOC(XkbFileInfo *result, XkmInfo *info,
                   int max_toc, xkmSectionInfo *toc_rtrn)
{
    int num_toc;
    int total_size;

    total_size = num_toc = 0;
    if (SizeXKMVirtualMods(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMCompatMap(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMIndicators(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    return num_toc;
}

/*ARGSUSED*/
static int
GetXKMSemanticsTOC(XkbFileInfo *result, XkmInfo *info,
                   int max_toc, xkmSectionInfo *toc_rtrn)
{
    int num_toc;
    int total_size;

    total_size = num_toc = 0;
    if (SizeXKMVirtualMods(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMKeyTypes(result, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMCompatMap(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMIndicators(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    return num_toc;
}

/*ARGSUSED*/
static int
GetXKMLayoutTOC(XkbFileInfo *result, XkmInfo *info,
                int max_toc, xkmSectionInfo *toc_rtrn)
{
    int num_toc;
    int total_size;

    total_size = num_toc = 0;
    if (SizeXKMVirtualMods(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMKeycodes(result, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMKeyTypes(result, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMSymbols(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMIndicators(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMGeometry(result, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    return num_toc;
}

/*ARGSUSED*/
static int
GetXKMKeymapTOC(XkbFileInfo *result, XkmInfo *info,
                int max_toc, xkmSectionInfo *toc_rtrn)
{
    int num_toc;
    int total_size;

    total_size = num_toc = 0;
    if (SizeXKMVirtualMods(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMKeycodes(result, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMKeyTypes(result, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMCompatMap(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMSymbols(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMIndicators(result, info, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    if (SizeXKMGeometry(result, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    return num_toc;
}

/*ARGSUSED*/
static int
GetXKMGeometryTOC(XkbFileInfo *result, XkmInfo *info,
                  int max_toc, xkmSectionInfo *toc_rtrn)
{
    int num_toc;
    int total_size;

    total_size = num_toc = 0;
    if (SizeXKMGeometry(result, &toc_rtrn[num_toc], &total_size))
        num_toc++;
    return num_toc;
}

static Bool
WriteXKMFile(FILE *file, XkbFileInfo *result,
             int num_toc, xkmSectionInfo *toc, XkmInfo *info)
{
    register int i;
    unsigned tmp, size, total = 0;

    for (i = 0; i < num_toc; i++) {
        tmp = fwrite(&toc[i], SIZEOF(xkmSectionInfo), 1, file);
        total += tmp * SIZEOF(xkmSectionInfo);
        switch (toc[i].type) {
        case XkmTypesIndex:
            size = WriteXKMKeyTypes(file, result);
            break;
        case XkmCompatMapIndex:
            size = WriteXKMCompatMap(file, result, info);
            break;
        case XkmSymbolsIndex:
            size = WriteXKMSymbols(file, result, info);
            break;
        case XkmIndicatorsIndex:
            size = WriteXKMIndicators(file, result, info);
            break;
        case XkmKeyNamesIndex:
            size = WriteXKMKeycodes(file, result);
            break;
        case XkmGeometryIndex:
            size = WriteXKMGeometry(file, result);
            break;
        case XkmVirtualModsIndex:
            size = WriteXKMVirtualMods(file, result, info);
            break;
        default:
            _XkbLibError(_XkbErrIllegalTOCType, "WriteXKMFile", toc[i].type);
            return False;
        }
        size += SIZEOF(xkmSectionInfo);
        if (size != toc[i].size) {
            _XkbLibError(_XkbErrBadLength,
                         XkbConfigText(toc[i].type, XkbMessage),
                         size - toc[i].size);
            return False;
        }
    }
    return True;
}


#define	MAX_TOC	16

Bool
XkbWriteXKMFile(FILE *out, XkbFileInfo *result)
{
    Bool ok;
    XkbDescPtr xkb;
    XkmInfo info;
    int size_toc, i;
    unsigned hdr, present;
    xkmFileInfo fileInfo;
    xkmSectionInfo toc[MAX_TOC];

    int (*getTOC) (XkbFileInfo *        /* result */ ,
                   XkmInfo *            /* info */ ,
                   int                  /* max_to */ ,
                   xkmSectionInfo *     /* toc_rtrn */
        );

    switch (result->type) {
    case XkmKeyNamesIndex:
        getTOC = GetXKMKeyNamesTOC;
        break;
    case XkmTypesIndex:
        getTOC = GetXKMTypesTOC;
        break;
    case XkmCompatMapIndex:
        getTOC = GetXKMCompatMapTOC;
        break;
    case XkmSemanticsFile:
        getTOC = GetXKMSemanticsTOC;
        break;
    case XkmLayoutFile:
        getTOC = GetXKMLayoutTOC;
        break;
    case XkmKeymapFile:
        getTOC = GetXKMKeymapTOC;
        break;
    case XkmGeometryFile:
    case XkmGeometryIndex:
        getTOC = GetXKMGeometryTOC;
        break;
    default:
        _XkbLibError(_XkbErrIllegalContents,
                     XkbConfigText(result->type, XkbMessage), 0);
        return False;
    }
    xkb = result->xkb;

    bzero((char *) &info, sizeof(XkmInfo));
    size_toc = (*getTOC) (result, &info, MAX_TOC, toc);
    if (size_toc < 1) {
        _XkbLibError(_XkbErrEmptyFile, "XkbWriteXKMFile", 0);
        return False;
    }
    if (out == NULL) {
        _XkbLibError(_XkbErrFileCannotOpen, "XkbWriteXKMFile", 0);
        return False;
    }
    for (i = present = 0; i < size_toc; i++) {
        toc[i].offset += 4 + SIZEOF(xkmFileInfo);
        toc[i].offset += (size_toc * SIZEOF(xkmSectionInfo));
        if (toc[i].type <= XkmLastIndex) {
            present |= (1 << toc[i].type);
        }
#ifdef DEBUG
        else {
            fprintf(stderr, "Illegal section type %d\n", toc[i].type);
            fprintf(stderr, "Ignored\n");
        }
#endif
    }
    hdr = (('x' << 24) | ('k' << 16) | ('m' << 8) | XkmFileVersion);
    xkmPutCARD32(out, (unsigned long) hdr);
    fileInfo.type = result->type;
    fileInfo.min_kc = xkb->min_key_code;
    fileInfo.max_kc = xkb->max_key_code;
    fileInfo.num_toc = size_toc;
    fileInfo.present = present;
    fileInfo.pad = 0;
    fwrite(&fileInfo, SIZEOF(xkmFileInfo), 1, out);
    fwrite(toc, SIZEOF(xkmSectionInfo), size_toc, out);
    ok = WriteXKMFile(out, result, size_toc, toc, &info);
    return ok;
}
