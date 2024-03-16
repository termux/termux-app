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
#include <X11/keysym.h>
#include "XKBlibint.h"


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
#ifdef DEBUG
        fprintf(stderr, "bad keycode (%d,%d) in XkbAllocClientMap\n",
                xkb->min_key_code, xkb->max_key_code);
#endif
        return BadValue;
    }

    if (xkb->map == NULL) {
        map = _XkbTypedCalloc(1, XkbClientMapRec);
        if (map == NULL)
            return BadAlloc;
        xkb->map = map;
    }
    else
        map = xkb->map;

    if ((which & XkbKeyTypesMask) && (nTotalTypes > 0)) {
        if (map->types == NULL) {
            map->num_types = map->size_types = 0;
        }
        if ((map->types == NULL) || (map->size_types < nTotalTypes)) {
            _XkbResizeArray(map->types, map->size_types, nTotalTypes,
                            XkbKeyTypeRec);

            if (map->types == NULL) {
                map->num_types = map->size_types = 0;
                return BadAlloc;
            }
            map->size_types = nTotalTypes;
        }
    }
    if (which & XkbKeySymsMask) {
        int nKeys = XkbNumKeys(xkb);

        if (map->syms == NULL) {
            map->size_syms = (nKeys * 15) / 10;
            map->syms = _XkbTypedCalloc(map->size_syms, KeySym);
            if (!map->syms) {
                map->size_syms = 0;
                return BadAlloc;
            }
            map->num_syms = 1;
            map->syms[0] = NoSymbol;
        }
        if (map->key_sym_map == NULL) {
            i = xkb->max_key_code + 1;
            map->key_sym_map = _XkbTypedCalloc(i, XkbSymMapRec);
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
            map->modmap = _XkbTypedCalloc(i, unsigned char);
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
        map = _XkbTypedCalloc(1, XkbServerMapRec);
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
            map->explicit = _XkbTypedCalloc(i, unsigned char);
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
            map->num_acts = 1;
            map->size_acts = 0;
        }
        if ((map->acts == NULL) ||
            ((map->size_acts - map->num_acts) < nNewActions)) {
            unsigned need;

            need = map->num_acts + nNewActions;
            _XkbResizeArray(map->acts, map->size_acts, need, XkbAction);
            if (map->acts == NULL) {
                map->num_acts = map->size_acts = 0;
                return BadAlloc;
            }
            map->size_acts = need;
        }
        if (map->key_acts == NULL) {
            i = xkb->max_key_code + 1;
            map->key_acts = _XkbTypedCalloc(i, unsigned short);
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
            map->behaviors = _XkbTypedCalloc(i, XkbBehavior);
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
            map->vmodmap = _XkbTypedCalloc(i, unsigned short);
            if (map->vmodmap == NULL)
                return BadAlloc;
        }
    }
    return Success;
}

/***====================================================================***/

Status
XkbCopyKeyType(XkbKeyTypePtr from, XkbKeyTypePtr into)
{
    if ((!from) || (!into))
        return BadMatch;

    _XkbFree(into->map);
    into->map = NULL;

    _XkbFree(into->preserve);
    into->preserve = NULL;

    _XkbFree(into->level_names);
    into->level_names = NULL;

    *into = *from;
    if ((from->map) && (into->map_count > 0)) {
        into->map = _XkbTypedCalloc(into->map_count, XkbKTMapEntryRec);
        if (!into->map)
            return BadAlloc;
        memcpy(into->map, from->map,
               into->map_count * sizeof(XkbKTMapEntryRec));
    }
    if ((from->preserve) && (into->map_count > 0)) {
        into->preserve = _XkbTypedCalloc(into->map_count, XkbModsRec);
        if (!into->preserve)
            return BadAlloc;
        memcpy(into->preserve, from->preserve,
               into->map_count * sizeof(XkbModsRec));
    }
    if ((from->level_names) && (into->num_levels > 0)) {
        into->level_names = _XkbTypedCalloc(into->num_levels, Atom);
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

XkbKeyTypePtr
XkbAddKeyType(XkbDescPtr xkb,
              Atom name,
              int map_count,
              Bool want_preserve,
              int num_lvls)
{
    register int i;
    unsigned tmp;
    XkbKeyTypePtr type;
    XkbClientMapPtr map;

    if ((!xkb) || (num_lvls < 1))
        return NULL;
    map = xkb->map;
    if ((map) && (map->types)) {
        for (i = 0; i < map->num_types; i++) {
            if (map->types[i].name == name) {
                Status status =
                    XkbResizeKeyType(xkb, i, map_count, want_preserve,
                                     num_lvls);
                return (status == Success ? &map->types[i] : NULL);
            }
        }
    }
    if ((!map) || (!map->types) || (map->num_types < XkbNumRequiredTypes)) {
        tmp = XkbNumRequiredTypes + 1;
        if (XkbAllocClientMap(xkb, XkbKeyTypesMask, tmp) != Success)
            return NULL;
        if (!map)
            map = xkb->map;
        tmp = 0;
        if (map->num_types <= XkbKeypadIndex)
            tmp |= XkbKeypadMask;
        if (map->num_types <= XkbAlphabeticIndex)
            tmp |= XkbAlphabeticMask;
        if (map->num_types <= XkbTwoLevelIndex)
            tmp |= XkbTwoLevelMask;
        if (map->num_types <= XkbOneLevelIndex)
            tmp |= XkbOneLevelMask;
        if (XkbInitCanonicalKeyTypes(xkb, tmp, XkbNoModifier) == Success) {
            for (i = 0; i < map->num_types; i++) {
                Status status;

                if (map->types[i].name != name)
                    continue;
                status = XkbResizeKeyType(xkb, i, map_count, want_preserve,
                                          num_lvls);
                return (status == Success ? &map->types[i] : NULL);
            }
        }
    }
    if ((map->num_types <= map->size_types) &&
        (XkbAllocClientMap(xkb, XkbKeyTypesMask, map->num_types + 1) !=
         Success)) {
        return NULL;
    }
    type = &map->types[map->num_types];
    map->num_types++;
    bzero((char *) type, sizeof(XkbKeyTypeRec));
    type->num_levels = num_lvls;
    type->map_count = map_count;
    type->name = name;
    if (map_count > 0) {
        type->map = _XkbTypedCalloc(map_count, XkbKTMapEntryRec);
        if (!type->map) {
            map->num_types--;
            return NULL;
        }
        if (want_preserve) {
            type->preserve = _XkbTypedCalloc(map_count, XkbModsRec);
            if (!type->preserve) {
                _XkbFree(type->map);
                map->num_types--;
                return NULL;
            }
        }
    }
    return type;
}

Status
XkbResizeKeyType(XkbDescPtr xkb,
                 int type_ndx,
                 int map_count,
                 Bool want_preserve,
                 int new_num_lvls)
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
        _XkbFree(type->map);
        type->map = NULL;
        _XkbFree(type->preserve);
        type->preserve = NULL;
        type->map_count = 0;
    }
    else {
        if ((map_count > type->map_count) || (type->map == NULL))
            _XkbResizeArray(type->map, type->map_count, map_count,
                            XkbKTMapEntryRec);
        if (!type->map) {
            return BadAlloc;
        }
        if (want_preserve) {
            if ((map_count > type->map_count) || (type->preserve == NULL)) {
                _XkbResizeArray(type->preserve, type->map_count, map_count,
                                XkbModsRec);
            }
            if (!type->preserve) {
                return BadAlloc;
            }
        }
        else {
            _XkbFree(type->preserve);
            type->preserve = NULL;
        }
        type->map_count = map_count;
    }

    if ((new_num_lvls > type->num_levels) || (type->level_names == NULL)) {
        _XkbResizeArray(type->level_names, type->num_levels, new_num_lvls, Atom);
        if (!type->level_names) {
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
    bzero(matchingKeys, XkbMaxKeyCount * sizeof(KeyCode));
    nMatchingKeys = 0;
    if (new_num_lvls > type->num_levels) {
        int nTotal;
        KeySym *newSyms;
        int width, match, nResize;
        register int i, g, nSyms;

        nResize = 0;
        for (nTotal = 1, i = xkb->min_key_code; i <= xkb->max_key_code; i++) {
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
            if ((!match) || (width >= new_num_lvls))
                nTotal += XkbKeyNumSyms(xkb, i);
            else {
                nTotal += XkbKeyNumGroups(xkb, i) * new_num_lvls;
                nResize++;
            }
        }
        if (nResize > 0) {
            int nextMatch;

            xkb->map->size_syms = (nTotal * 12) / 10;
            newSyms = _XkbTypedCalloc(xkb->map->size_syms, KeySym);
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
            _XkbFree(xkb->map->syms);
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
                        bzero(&pSyms[g * width + firstClear],
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
            bzero(&xkb->map->syms[xkb->map->num_syms + XkbKeyNumSyms(xkb, key)],
                  (needed - nOldSyms) * sizeof(KeySym));
        }
        xkb->map->key_sym_map[key].offset = xkb->map->num_syms;
        xkb->map->num_syms += needed;
        return &xkb->map->syms[xkb->map->key_sym_map[key].offset];
    }
    xkb->map->size_syms += (needed > 32 ? needed : 32);
    newSyms = _XkbTypedCalloc(xkb->map->size_syms, KeySym);
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
            bzero(&newSyms[nSyms + nCopy], (nKeySyms - nCopy) * sizeof(KeySym));
        xkb->map->key_sym_map[i].offset = nSyms;
        nSyms += nKeySyms;
    }
    _XkbFree(xkb->map->syms);
    xkb->map->syms = newSyms;
    xkb->map->num_syms = nSyms;
    return &xkb->map->syms[xkb->map->key_sym_map[key].offset];
}

static unsigned
_ExtendRange(unsigned int old_flags,
             unsigned int flag,
             KeyCode newKC,
             KeyCode *old_min,
             unsigned char *old_num)
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
                      int minKC,
                      int maxKC,
                      XkbChangesPtr changes)
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
                bzero((char *) &xkb->map->key_sym_map[minKC],
                      tmp * sizeof(XkbSymMapRec));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                XkbKeySymsMask, minKC,
                                                &changes->map.first_key_sym,
                                                &changes->map.num_key_syms);
                }
            }
            if (xkb->map->modmap) {
                bzero((char *) &xkb->map->modmap[minKC], tmp);
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                XkbModifierMapMask, minKC,
                                                &changes->map.first_modmap_key,
                                                &changes->map.num_modmap_keys);
                }
            }
        }
        if (xkb->server) {
            if (xkb->server->behaviors) {
                bzero((char *) &xkb->server->behaviors[minKC],
                      tmp * sizeof(XkbBehavior));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                              XkbKeyBehaviorsMask, minKC,
                                              &changes->map.first_key_behavior,
                                              &changes->map.num_key_behaviors);
                }
            }
            if (xkb->server->key_acts) {
                bzero((char *) &xkb->server->key_acts[minKC],
                      tmp * sizeof(unsigned short));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                XkbKeyActionsMask, minKC,
                                                &changes->map.first_key_act,
                                                &changes->map.num_key_acts);
                }
            }
            if (xkb->server->vmodmap) {
                bzero((char *) &xkb->server->vmodmap[minKC],
                      tmp * sizeof(unsigned short));
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                XkbVirtualModMapMask, minKC,
                                                &changes->map.first_vmodmap_key,
                                                &changes->map.num_vmodmap_keys);
                }
            }
        }
        if ((xkb->names) && (xkb->names->keys)) {
            bzero((char *) &xkb->names->keys[minKC],
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
                _XkbResizeArray(xkb->map->key_sym_map, xkb->max_key_code + 1,
                                (maxKC + 1), XkbSymMapRec);
                if (!xkb->map->key_sym_map) {
                    return BadAlloc;
                }
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                XkbKeySymsMask, maxKC,
                                                &changes->map.first_key_sym,
                                                &changes->map.num_key_syms);
                }
            }
            if (xkb->map->modmap) {
                _XkbResizeArray(xkb->map->modmap, xkb->max_key_code + 1,
                                (maxKC + 1), unsigned char);
                if (!xkb->map->modmap) {
                    return BadAlloc;
                }
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                XkbModifierMapMask, maxKC,
                                                &changes->map.first_modmap_key,
                                                &changes->map.num_modmap_keys);
                }
            }
        }
        if (xkb->server) {
            if (xkb->server->behaviors) {
                _XkbResizeArray(xkb->server->behaviors, xkb->max_key_code + 1,
                                (maxKC + 1), XkbBehavior);
                if (!xkb->server->behaviors) {
                    return BadAlloc;
                }
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                XkbKeyBehaviorsMask, maxKC,
                                                &changes->map.first_key_behavior,
                                                &changes->map.num_key_behaviors);
                }
            }
            if (xkb->server->key_acts) {
                _XkbResizeArray(xkb->server->key_acts, xkb->max_key_code + 1,
                                (maxKC + 1), unsigned short);
                if (!xkb->server->key_acts) {
                    return BadAlloc;
                }
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                XkbKeyActionsMask, maxKC,
                                                &changes->map.first_key_act,
                                                &changes->map.num_key_acts);
                }
            }
            if (xkb->server->vmodmap) {
                _XkbResizeArray(xkb->server->vmodmap, xkb->max_key_code + 1,
                                (maxKC + 1), unsigned short);
                if (!xkb->server->vmodmap) {
                    return BadAlloc;
                }
                if (changes) {
                    changes->map.changed = _ExtendRange(changes->map.changed,
                                                XkbVirtualModMapMask, maxKC,
                                                &changes->map.first_vmodmap_key,
                                                &changes->map.num_vmodmap_keys);
                }
            }
        }
        if ((xkb->names) && (xkb->names->keys)) {
            _XkbResizeArray(xkb->names->keys, xkb->max_key_code + 1,
                            (maxKC + 1), XkbKeyNameRec);
            if (!xkb->names->keys) {
                return BadAlloc;
            }
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
    newActs = _XkbTypedCalloc(xkb->server->size_acts, XkbAction);
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
            bzero(&newActs[nActs + nCopy],
                  (nKeyActs - nCopy) * sizeof(XkbAction));
        xkb->server->key_acts[i] = nActs;
        nActs += nKeyActs;
    }
    _XkbFree(xkb->server->acts);
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
                    _XkbFree(type->map);
                    type->map = NULL;

                    _XkbFree(type->preserve);
                    type->preserve = NULL;

                    type->map_count = 0;

                    _XkbFree(type->level_names);
                    type->level_names = NULL;
                }
            }
            _XkbFree(map->types);
            map->num_types = map->size_types = 0;
            map->types = NULL;
        }
    }
    if (what & XkbKeySymsMask) {
        _XkbFree(map->key_sym_map);
        map->key_sym_map = NULL;

        _XkbFree(map->syms);
        map->size_syms = map->num_syms = 0;
        map->syms = NULL;
    }
    if (what & XkbModifierMapMask) {
        _XkbFree(map->modmap);
        map->modmap = NULL;
    }
    if (freeMap) {
        _XkbFree(xkb->map);
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
    if (what & XkbExplicitComponentsMask) {
        _XkbFree(map->explicit);
        map->explicit = NULL;
    }
    if (what & XkbKeyActionsMask) {
           _XkbFree(map->key_acts);
            map->key_acts = NULL;

           _XkbFree(map->acts);
            map->num_acts = map->size_acts = 0;
            map->acts = NULL;
    }
    if (what & XkbKeyBehaviorsMask) {
        _XkbFree(map->behaviors);
        map->behaviors = NULL;
    }
    if (what & XkbVirtualModMapMask)  {
        _XkbFree(map->vmodmap);
        map->vmodmap = NULL;
    }

    if (freeMap) {
        _XkbFree(xkb->server);
        xkb->server = NULL;
    }
    return;
}
