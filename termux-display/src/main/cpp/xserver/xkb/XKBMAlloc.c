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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "inputstr.h"
#include <X11/keysym.h>
#define	XKBSRV_NEED_FILE_FUNCS
#include <xkbsrv.h>

/***====================================================================***/

Status
XkbAllocClientMap(XkbDescPtr xkb, unsigned which, unsigned nTotalTypes)
{
    register int i;
    XkbClientMapPtr map;

    if ((xkb == NULL) ||
        ((nTotalTypes > 0) && (nTotalTypes < XkbNumRequiredTypes)))
        return BadValue;
    if ((which & XkbKeySymsMask) &&
        ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
         (!XkbIsLegalKeycode(xkb->max_key_code)) ||
         (xkb->max_key_code < xkb->min_key_code))) {
        DebugF("bad keycode (%d,%d) in XkbAllocClientMap\n",
               xkb->min_key_code, xkb->max_key_code);
        return BadValue;
    }

    if (xkb->map == NULL) {
        map = calloc(1, sizeof(XkbClientMapRec));
        if (map == NULL)
            return BadAlloc;
        xkb->map = map;
    }
    else
        map = xkb->map;

    if ((which & XkbKeyTypesMask) && (nTotalTypes > 0)) {
        if (map->types == NULL) {
            map->types = calloc(nTotalTypes, sizeof(XkbKeyTypeRec));
            if (map->types == NULL)
                return BadAlloc;
            map->num_types = 0;
            map->size_types = nTotalTypes;
        }
        else if (map->size_types < nTotalTypes) {
            XkbKeyTypeRec *prev_types = map->types;

            map->types =
                reallocarray(map->types, nTotalTypes, sizeof(XkbKeyTypeRec));
            if (map->types == NULL) {
                free(prev_types);
                map->num_types = map->size_types = 0;
                return BadAlloc;
            }
            map->size_types = nTotalTypes;
            memset(&map->types[map->num_types], 0,
                   ((map->size_types -
                     map->num_types) * sizeof(XkbKeyTypeRec)));
        }
    }
    if (which & XkbKeySymsMask) {
        int nKeys = XkbNumKeys(xkb);

        if (map->syms == NULL) {
            map->size_syms = (nKeys * 15) / 10;
            map->syms = calloc(map->size_syms, sizeof(KeySym));
            if (!map->syms) {
                map->size_syms = 0;
                return BadAlloc;
            }
            map->num_syms = 1;
            map->syms[0] = NoSymbol;
        }
        if (map->key_sym_map == NULL) {
            i = xkb->max_key_code + 1;
            map->key_sym_map = calloc(i, sizeof(XkbSymMapRec));
            if (map->key_sym_map == NULL)
                return BadAlloc;
        }
    }
    if (which & XkbModifierMapMask) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadMatch;
        if (map->modmap == NULL) {
            i = xkb->max_key_code + 1;
            map->modmap = calloc(i, sizeof(unsigned char));
            if (map->modmap == NULL)
                return BadAlloc;
        }
    }
    return Success;
}

Status
XkbAllocServerMap(XkbDescPtr xkb, unsigned which, unsigned nNewActions)
{
    register int i;
    XkbServerMapPtr map;

    if (xkb == NULL)
        return BadMatch;
    if (xkb->server == NULL) {
        map = calloc(1, sizeof(XkbServerMapRec));
        if (map == NULL)
            return BadAlloc;
        for (i = 0; i < XkbNumVirtualMods; i++) {
            map->vmods[i] = XkbNoModifierMask;
        }
        xkb->server = map;
    }
    else
        map = xkb->server;
    if (which & XkbExplicitComponentsMask) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadMatch;
        if (map->explicit == NULL) {
            i = xkb->max_key_code + 1;
            map->explicit = calloc(i, sizeof(unsigned char));
            if (map->explicit == NULL)
                return BadAlloc;
        }
    }
    if (which & XkbKeyActionsMask) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadMatch;
        if (nNewActions < 1)
            nNewActions = 1;
        if (map->acts == NULL) {
            map->acts = calloc((nNewActions + 1), sizeof(XkbAction));
            if (map->acts == NULL)
                return BadAlloc;
            map->num_acts = 1;
            map->size_acts = nNewActions + 1;
        }
        else if ((map->size_acts - map->num_acts) < nNewActions) {
            unsigned need;
            XkbAction *prev_acts = map->acts;

            need = map->num_acts + nNewActions;
            map->acts = reallocarray(map->acts, need, sizeof(XkbAction));
            if (map->acts == NULL) {
                free(prev_acts);
                map->num_acts = map->size_acts = 0;
                return BadAlloc;
            }
            map->size_acts = need;
            memset(&map->acts[map->num_acts], 0,
                   ((map->size_acts - map->num_acts) * sizeof(XkbAction)));
        }
        if (map->key_acts == NULL) {
            i = xkb->max_key_code + 1;
            map->key_acts = calloc(i, sizeof(unsigned short));
            if (map->key_acts == NULL)
                return BadAlloc;
        }
    }
    if (which & XkbKeyBehaviorsMask) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadMatch;
        if (map->behaviors == NULL) {
            i = xkb->max_key_code + 1;
            map->behaviors = calloc(i, sizeof(XkbBehavior));
            if (map->behaviors == NULL)
                return BadAlloc;
        }
    }
    if (which & XkbVirtualModMapMask) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadMatch;
        if (map->vmodmap == NULL) {
            i = xkb->max_key_code + 1;
            map->vmodmap = calloc(i, sizeof(unsigned short));
            if (map->vmodmap == NULL)
                return BadAlloc;
        }
    }
    return Success;
}

/***====================================================================***/

static Status
XkbCopyKeyType(XkbKeyTypePtr from, XkbKeyTypePtr into)
{
    if ((!from) || (!into))
        return BadMatch;
    free(into->map);
    into->map = NULL;
    free(into->preserve);
    into->preserve = NULL;
    free(into->level_names);
    into->level_names = NULL;
    *into = *from;
    if ((from->map) && (into->map_count > 0)) {
        into->map = calloc(into->map_count, sizeof(XkbKTMapEntryRec));
        if (!into->map)
            return BadAlloc;
        memcpy(into->map, from->map,
               into->map_count * sizeof(XkbKTMapEntryRec));
    }
    if ((from->preserve) && (into->map_count > 0)) {
        into->preserve = calloc(into->map_count, sizeof(XkbModsRec));
        if (!into->preserve)
            return BadAlloc;
        memcpy(into->preserve, from->preserve,
               into->map_count * sizeof(XkbModsRec));
    }
    if ((from->level_names) && (into->num_levels > 0)) {
        into->level_names = calloc(into->num_levels, sizeof(Atom));
        if (!into->level_names)
            return BadAlloc;
        memcpy(into->level_names, from->level_names,
               into->num_levels * sizeof(Atom));
    }
    return Success;
}

Status
XkbCopyKeyTypes(XkbKeyTypePtr from, XkbKeyTypePtr into, int num_types)
{
    register int i, rtrn;

    if ((!from) || (!into) || (num_types < 0))
        return BadMatch;
    for (i = 0; i < num_types; i++) {
        if ((rtrn = XkbCopyKeyType(from++, into++)) != Success)
            return rtrn;
    }
    return Success;
}

Status
XkbResizeKeyType(XkbDescPtr xkb,
                 int type_ndx,
                 int map_count, Bool want_preserve, int new_num_lvls)
{
    XkbKeyTypePtr type;
    KeyCode matchingKeys[XkbMaxKeyCount], nMatchingKeys;

    if ((type_ndx < 0) || (type_ndx >= xkb->map->num_types) || (map_count < 0)
        || (new_num_lvls < 1))
        return BadValue;
    switch (type_ndx) {
    case XkbOneLevelIndex:
        if (new_num_lvls != 1)
            return BadMatch;
        break;
    case XkbTwoLevelIndex:
    case XkbAlphabeticIndex:
    case XkbKeypadIndex:
        if (new_num_lvls != 2)
            return BadMatch;
        break;
    }
    type = &xkb->map->types[type_ndx];
    if (map_count == 0) {
        free(type->map);
        type->map = NULL;
        free(type->preserve);
        type->preserve = NULL;
        type->map_count = 0;
    }
    else {
        XkbKTMapEntryRec *prev_map = type->map;

        if ((map_count > type->map_count) || (type->map == NULL))
            type->map =
                reallocarray(type->map, map_count, sizeof(XkbKTMapEntryRec));
        if (!type->map) {
            free(prev_map);
            return BadAlloc;
        }
        if (want_preserve) {
            XkbModsRec *prev_preserve = type->preserve;

            if ((map_count > type->map_count) || (type->preserve == NULL)) {
                type->preserve = reallocarray(type->preserve,
                                              map_count, sizeof(XkbModsRec));
            }
            if (!type->preserve) {
                free(prev_preserve);
                return BadAlloc;
            }
        }
        else {
            free(type->preserve);
            type->preserve = NULL;
        }
        type->map_count = map_count;
    }

    if ((new_num_lvls > type->num_levels) || (type->level_names == NULL)) {
        Atom *prev_level_names = type->level_names;

        type->level_names = reallocarray(type->level_names,
                                         new_num_lvls, sizeof(Atom));
        if (!type->level_names) {
            free(prev_level_names);
            return BadAlloc;
        }
    }
    /*
     * Here's the theory:
     *    If the width of the type changed, we might have to resize the symbol
     * maps for any keys that use the type for one or more groups.  This is
     * expensive, so we'll try to cull out any keys that are obviously okay:
     * In any case:
     *    - keys that have a group width <= the old width are okay (because
     *      they could not possibly have been associated with the old type)
     * If the key type increased in size:
     *    - keys that already have a group width >= to the new width are okay
     *    + keys that have a group width >= the old width but < the new width
     *      might have to be enlarged.
     * If the key type decreased in size:
     *    - keys that have a group width > the old width don't have to be
     *      resized (because they must have some other wider type associated
     *      with some group).
     *    + keys that have a group width == the old width might have to be
     *      shrunk.
     * The possibilities marked with '+' require us to examine the key types
     * associated with each group for the key.
     */
    memset(matchingKeys, 0, XkbMaxKeyCount * sizeof(KeyCode));
    nMatchingKeys = 0;
    if (new_num_lvls > type->num_levels) {
        int nTotal;
        KeySym *newSyms;
        int width, match, nResize;
        register int i, g, nSyms;

        nResize = 0;
        for (nTotal = 1, i = xkb->min_key_code; i <= xkb->max_key_code; i++) {
            width = XkbKeyGroupsWidth(xkb, i);
            if (width < type->num_levels || width >= new_num_lvls) {
                nTotal += XkbKeyNumSyms(xkb,i);
                continue;
            }
            for (match = 0, g = XkbKeyNumGroups(xkb, i) - 1;
                 (g >= 0) && (!match); g--) {
                if (XkbKeyKeyTypeIndex(xkb, i, g) == type_ndx) {
                    matchingKeys[nMatchingKeys++] = i;
                    match = 1;
                }
            }
            if (!match)
                nTotal += XkbKeyNumSyms(xkb, i);
            else {
                nTotal += XkbKeyNumGroups(xkb, i) * new_num_lvls;
                nResize++;
            }
        }
        if (nResize > 0) {
            int nextMatch;

            xkb->map->size_syms = (nTotal * 15) / 10;
            newSyms = calloc(xkb->map->size_syms, sizeof(KeySym));
            if (newSyms == NULL)
                return BadAlloc;
            nextMatch = 0;
            nSyms = 1;
            for (i = xkb->min_key_code; i <= xkb->max_key_code; i++) {
                if (matchingKeys[nextMatch] == i) {
                    KeySym *pOld;

                    nextMatch++;
                    width = XkbKeyGroupsWidth(xkb, i);
                    pOld = XkbKeySymsPtr(xkb, i);
                    for (g = XkbKeyNumGroups(xkb, i) - 1; g >= 0; g--) {
                        memcpy(&newSyms[nSyms + (new_num_lvls * g)],
                               &pOld[width * g], width * sizeof(KeySym));
                    }
                    xkb->map->key_sym_map[i].offset = nSyms;
                    nSyms += XkbKeyNumGroups(xkb, i) * new_num_lvls;
                }
                else {
                    memcpy(&newSyms[nSyms], XkbKeySymsPtr(xkb, i),
                           XkbKeyNumSyms(xkb, i) * sizeof(KeySym));
                    xkb->map->key_sym_map[i].offset = nSyms;
                    nSyms += XkbKeyNumSyms(xkb, i);
                }
            }
            type->num_levels = new_num_lvls;
            free(xkb->map->syms);
            xkb->map->syms = newSyms;
            xkb->map->num_syms = nSyms;
            return Success;
        }
    }
    else if (new_num_lvls < type->num_levels) {
        int width, match;
        register int g, i;

        for (i = xkb->min_key_code; i <= xkb->max_key_code; i++) {
            width = XkbKeyGroupsWidth(xkb, i);
            if (width < type->num_levels)
                continue;
            for (match = 0, g = XkbKeyNumGroups(xkb, i) - 1;
                 (g >= 0) && (!match); g--) {
                if (XkbKeyKeyTypeIndex(xkb, i, g) == type_ndx) {
                    matchingKeys[nMatchingKeys++] = i;
                    match = 1;
                }
            }
        }
    }
    if (nMatchingKeys > 0) {
        int key, firstClear;
        register int i, g;

        if (new_num_lvls > type->num_levels)
            firstClear = type->num_levels;
        else
            firstClear = new_num_lvls;
        for (i = 0; i < nMatchingKeys; i++) {
            KeySym *pSyms;
            int width, nClear;

            key = matchingKeys[i];
            width = XkbKeyGroupsWidth(xkb, key);
            nClear = width - firstClear;
            pSyms = XkbKeySymsPtr(xkb, key);
            for (g = XkbKeyNumGroups(xkb, key) - 1; g >= 0; g--) {
                if (XkbKeyKeyTypeIndex(xkb, key, g) == type_ndx) {
                    if (nClear > 0)
                        memset(&pSyms[g * width + firstClear], 0,
                               nClear * sizeof(KeySym));
                }
            }
        }
    }
    type->num_levels = new_num_lvls;
    return Success;
}

KeySym *
XkbResizeKeySyms(XkbDescPtr xkb, int key, int needed)
{
    register int i, nSyms, nKeySyms;
    unsigned nOldSyms;
    KeySym *newSyms;

    if (needed == 0) {
        xkb->map->key_sym_map[key].offset = 0;
        return xkb->map->syms;
    }
    nOldSyms = XkbKeyNumSyms(xkb, key);
    if (nOldSyms >= (unsigned) needed) {
        return XkbKeySymsPtr(xkb, key);
    }
    if (xkb->map->size_syms - xkb->map->num_syms >= (unsigned) needed) {
        if (nOldSyms > 0) {
            memcpy(&xkb->map->syms[xkb->map->num_syms], XkbKeySymsPtr(xkb, key),
                   nOldSyms * sizeof(KeySym));
        }
        if ((needed - nOldSyms) > 0) {
            memset(&xkb->map->
                   syms[xkb->map->num_syms + XkbKeyNumSyms(xkb, key)], 0,
                   (needed - nOldSyms) * sizeof(KeySym));
        }
        xkb->map->key_sym_map[key].offset = xkb->map->num_syms;
        xkb->map->num_syms += needed;
        return &xkb->map->syms[xkb->map->key_sym_map[key].offset];
    }
    xkb->map->size_syms += (needed > 32 ? needed : 32);
    newSyms = calloc(xkb->map->size_syms, sizeof(KeySym));
    if (newSyms == NULL)
        return NULL;
    newSyms[0] = NoSymbol;
    nSyms = 1;
    for (i = xkb->min_key_code; i <= (int) xkb->max_key_code; i++) {
        int nCopy;

        nCopy = nKeySyms = XkbKeyNumSyms(xkb, i);
        if ((nKeySyms == 0) && (i != key))
            continue;
        if (i == key)
            nKeySyms = needed;
        if (nCopy != 0)
            memcpy(&newSyms[nSyms], XkbKeySymsPtr(xkb, i),
                   nCopy * sizeof(KeySym));
        if (nKeySyms > nCopy)
            memset(&newSyms[nSyms + nCopy], 0,
                   (nKeySyms - nCopy) * sizeof(KeySym));
        xkb->map->key_sym_map[i].offset = nSyms;
        nSyms += nKeySyms;
    }
    free(xkb->map->syms);
    xkb->map->syms = newSyms;
    xkb->map->num_syms = nSyms;
    return &xkb->map->syms[xkb->map->key_sym_map[key].offset];
}

static unsigned
_ExtendRange(unsigned int old_flags,
             unsigned int flag,
             KeyCode newKC, KeyCode *old_min, unsigned char *old_num)
{
    if ((old_flags & flag) == 0) {
        old_flags |= flag;
        *old_min = newKC;
        *old_num = 1;
    }
    else {
        int last = (*old_min) + (*old_num) - 1;

        if (newKC < *old_min) {
            *old_min = newKC;
            *old_num = (last - newKC) + 1;
        }
        else if (newKC > last) {
            *old_num = (newKC - (*old_min)) + 1;
        }
    }
    return old_flags;
}

Status
XkbChangeKeycodeRange(XkbDescPtr xkb,
                      int minKC, int maxKC, XkbChangesPtr changes)
{
    int tmp;

    if ((!xkb) || (minKC < XkbMinLegalKeyCode) || (maxKC > XkbMaxLegalKeyCode))
        return BadValue;
    if (minKC > maxKC)
        return BadMatch;
    if (minKC < xkb->min_key_code) {
        if (changes)
            changes->map.min_key_code = minKC;
        tmp = xkb->min_key_code - minKC;
        if (xkb->map) {
            if (xkb->map->key_sym_map) {
                memset((char *) &xkb->map->key_sym_map[minKC], 0,
                       tmp * sizeof(XkbSymMapRec));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                        XkbKeySymsMask, minKC,
                                                        &changes->map.
                                                        first_key_sym,
                                                        &changes->map.
                                                        num_key_syms);
                }
            }
            if (xkb->map->modmap) {
                memset((char *) &xkb->map->modmap[minKC], 0, tmp);
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                        XkbModifierMapMask,
                                                        minKC,
                                                        &changes->map.
                                                        first_modmap_key,
                                                        &changes->map.
                                                        num_modmap_keys);
                }
            }
        }
        if (xkb->server) {
            if (xkb->server->behaviors) {
                memset((char *) &xkb->server->behaviors[minKC], 0,
                       tmp * sizeof(XkbBehavior));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                        XkbKeyBehaviorsMask,
                                                        minKC,
                                                        &changes->map.
                                                        first_key_behavior,
                                                        &changes->map.
                                                        num_key_behaviors);
                }
            }
            if (xkb->server->key_acts) {
                memset((char *) &xkb->server->key_acts[minKC], 0,
                       tmp * sizeof(unsigned short));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                        XkbKeyActionsMask,
                                                        minKC,
                                                        &changes->map.
                                                        first_key_act,
                                                        &changes->map.
                                                        num_key_acts);
                }
            }
            if (xkb->server->vmodmap) {
                memset((char *) &xkb->server->vmodmap[minKC], 0,
                       tmp * sizeof(unsigned short));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                        XkbVirtualModMapMask,
                                                        minKC,
                                                        &changes->map.
                                                        first_modmap_key,
                                                        &changes->map.
                                                        num_vmodmap_keys);
                }
            }
        }
        if ((xkb->names) && (xkb->names->keys)) {
            memset((char *) &xkb->names->keys[minKC], 0,
                   tmp * sizeof(XkbKeyNameRec));
            if (changes) {
                changes->names.changed = _ExtendRange(changes->names.changed,
                                                      XkbKeyNamesMask, minKC,
                                                      &changes->names.first_key,
                                                      &changes->names.num_keys);
            }
        }
        xkb->min_key_code = minKC;
    }
    if (maxKC > xkb->max_key_code) {
        if (changes)
            changes->map.max_key_code = maxKC;
        tmp = maxKC - xkb->max_key_code;
        if (xkb->map) {
            if (xkb->map->key_sym_map) {
                XkbSymMapRec *prev_key_sym_map = xkb->map->key_sym_map;

                xkb->map->key_sym_map = reallocarray(xkb->map->key_sym_map,
                                                     maxKC + 1,
                                                     sizeof(XkbSymMapRec));
                if (!xkb->map->key_sym_map) {
                    free(prev_key_sym_map);
                    return BadAlloc;
                }
                memset((char *) &xkb->map->key_sym_map[xkb->max_key_code], 0,
                       tmp * sizeof(XkbSymMapRec));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                        XkbKeySymsMask, maxKC,
                                                        &changes->map.
                                                        first_key_sym,
                                                        &changes->map.
                                                        num_key_syms);
                }
            }
            if (xkb->map->modmap) {
                unsigned char *prev_modmap = xkb->map->modmap;

                xkb->map->modmap = reallocarray(xkb->map->modmap,
                                                maxKC + 1,
                                                sizeof(unsigned char));
                if (!xkb->map->modmap) {
                    free(prev_modmap);
                    return BadAlloc;
                }
                memset((char *) &xkb->map->modmap[xkb->max_key_code], 0, tmp);
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                        XkbModifierMapMask,
                                                        maxKC,
                                                        &changes->map.
                                                        first_modmap_key,
                                                        &changes->map.
                                                        num_modmap_keys);
                }
            }
        }
        if (xkb->server) {
            if (xkb->server->behaviors) {
                XkbBehavior *prev_behaviors = xkb->server->behaviors;

                xkb->server->behaviors = reallocarray(xkb->server->behaviors,
                                                      maxKC + 1,
                                                      sizeof(XkbBehavior));
                if (!xkb->server->behaviors) {
                    free(prev_behaviors);
                    return BadAlloc;
                }
                memset((char *) &xkb->server->behaviors[xkb->max_key_code], 0,
                       tmp * sizeof(XkbBehavior));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                        XkbKeyBehaviorsMask,
                                                        maxKC,
                                                        &changes->map.
                                                        first_key_behavior,
                                                        &changes->map.
                                                        num_key_behaviors);
                }
            }
            if (xkb->server->key_acts) {
                unsigned short *prev_key_acts = xkb->server->key_acts;

                xkb->server->key_acts = reallocarray(xkb->server->key_acts,
                                                     maxKC + 1,
                                                     sizeof(unsigned short));
                if (!xkb->server->key_acts) {
                    free(prev_key_acts);
                    return BadAlloc;
                }
                memset((char *) &xkb->server->key_acts[xkb->max_key_code], 0,
                       tmp * sizeof(unsigned short));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                        XkbKeyActionsMask,
                                                        maxKC,
                                                        &changes->map.
                                                        first_key_act,
                                                        &changes->map.
                                                        num_key_acts);
                }
            }
            if (xkb->server->vmodmap) {
                unsigned short *prev_vmodmap = xkb->server->vmodmap;

                xkb->server->vmodmap = reallocarray(xkb->server->vmodmap,
                                                    maxKC + 1,
                                                    sizeof(unsigned short));
                if (!xkb->server->vmodmap) {
                    free(prev_vmodmap);
                    return BadAlloc;
                }
                memset((char *) &xkb->server->vmodmap[xkb->max_key_code], 0,
                       tmp * sizeof(unsigned short));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                        XkbVirtualModMapMask,
                                                        maxKC,
                                                        &changes->map.
                                                        first_modmap_key,
                                                        &changes->map.
                                                        num_vmodmap_keys);
                }
            }
        }
        if ((xkb->names) && (xkb->names->keys)) {
            XkbKeyNameRec *prev_keys = xkb->names->keys;

            xkb->names->keys = reallocarray(xkb->names->keys,
                                            maxKC + 1, sizeof(XkbKeyNameRec));
            if (!xkb->names->keys) {
                free(prev_keys);
                return BadAlloc;
            }
            memset((char *) &xkb->names->keys[xkb->max_key_code], 0,
                   tmp * sizeof(XkbKeyNameRec));
            if (changes) {
                changes->names.changed = _ExtendRange(changes->names.changed,
                                                      XkbKeyNamesMask, maxKC,
                                                      &changes->names.first_key,
                                                      &changes->names.num_keys);
            }
        }
        xkb->max_key_code = maxKC;
    }
    return Success;
}

XkbAction *
XkbResizeKeyActions(XkbDescPtr xkb, int key, int needed)
{
    register int i, nActs;
    XkbAction *newActs;

    if (needed == 0) {
        xkb->server->key_acts[key] = 0;
        return NULL;
    }
    if (XkbKeyHasActions(xkb, key) &&
        (XkbKeyNumSyms(xkb, key) >= (unsigned) needed))
        return XkbKeyActionsPtr(xkb, key);
    if (xkb->server->size_acts - xkb->server->num_acts >= (unsigned) needed) {
        xkb->server->key_acts[key] = xkb->server->num_acts;
        xkb->server->num_acts += needed;
        return &xkb->server->acts[xkb->server->key_acts[key]];
    }
    xkb->server->size_acts = xkb->server->num_acts + needed + 8;
    newActs = calloc(xkb->server->size_acts, sizeof(XkbAction));
    if (newActs == NULL)
        return NULL;
    newActs[0].type = XkbSA_NoAction;
    nActs = 1;
    for (i = xkb->min_key_code; i <= (int) xkb->max_key_code; i++) {
        int nKeyActs, nCopy;

        if ((xkb->server->key_acts[i] == 0) && (i != key))
            continue;

        nCopy = nKeyActs = XkbKeyNumActions(xkb, i);
        if (i == key) {
            nKeyActs = needed;
            if (needed < nCopy)
                nCopy = needed;
        }

        if (nCopy > 0)
            memcpy(&newActs[nActs], XkbKeyActionsPtr(xkb, i),
                   nCopy * sizeof(XkbAction));
        if (nCopy < nKeyActs)
            memset(&newActs[nActs + nCopy], 0,
                   (nKeyActs - nCopy) * sizeof(XkbAction));
        xkb->server->key_acts[i] = nActs;
        nActs += nKeyActs;
    }
    free(xkb->server->acts);
    xkb->server->acts = newActs;
    xkb->server->num_acts = nActs;
    return &xkb->server->acts[xkb->server->key_acts[key]];
}

void
XkbFreeClientMap(XkbDescPtr xkb, unsigned what, Bool freeMap)
{
    XkbClientMapPtr map;

    if ((xkb == NULL) || (xkb->map == NULL))
        return;
    if (freeMap)
        what = XkbAllClientInfoMask;
    map = xkb->map;
    if (what & XkbKeyTypesMask) {
        if (map->types != NULL) {
            if (map->num_types > 0) {
                register int i;
                XkbKeyTypePtr type;

                for (i = 0, type = map->types; i < map->num_types; i++, type++) {
                    free(type->map);
                    type->map = NULL;
                    free(type->preserve);
                    type->preserve = NULL;
                    type->map_count = 0;
                    free(type->level_names);
                    type->level_names = NULL;
                }
            }
            free(map->types);
            map->num_types = map->size_types = 0;
            map->types = NULL;
        }
    }
    if (what & XkbKeySymsMask) {
        free(map->key_sym_map);
        map->key_sym_map = NULL;
        if (map->syms != NULL) {
            free(map->syms);
            map->size_syms = map->num_syms = 0;
            map->syms = NULL;
        }
    }
    if ((what & XkbModifierMapMask) && (map->modmap != NULL)) {
        free(map->modmap);
        map->modmap = NULL;
    }
    if (freeMap) {
        free(xkb->map);
        xkb->map = NULL;
    }
    return;
}

void
XkbFreeServerMap(XkbDescPtr xkb, unsigned what, Bool freeMap)
{
    XkbServerMapPtr map;

    if ((xkb == NULL) || (xkb->server == NULL))
        return;
    if (freeMap)
        what = XkbAllServerInfoMask;
    map = xkb->server;
    if ((what & XkbExplicitComponentsMask) && (map->explicit != NULL)) {
        free(map->explicit);
        map->explicit = NULL;
    }
    if (what & XkbKeyActionsMask) {
        free(map->key_acts);
        map->key_acts = NULL;
        if (map->acts != NULL) {
            free(map->acts);
            map->num_acts = map->size_acts = 0;
            map->acts = NULL;
        }
    }
    if ((what & XkbKeyBehaviorsMask) && (map->behaviors != NULL)) {
        free(map->behaviors);
        map->behaviors = NULL;
    }
    if ((what & XkbVirtualModMapMask) && (map->vmodmap != NULL)) {
        free(map->vmodmap);
        map->vmodmap = NULL;
    }

    if (freeMap) {
        free(xkb->server);
        xkb->server = NULL;
    }
    return;
}
