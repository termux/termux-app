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

#define	mapSize(m)	(sizeof(m)/sizeof(XkbKTMapEntryRec))
static XkbKTMapEntryRec map2Level[] = {
    { True, ShiftMask, {1, ShiftMask, 0} }
};

static XkbKTMapEntryRec mapAlpha[] = {
    { True, ShiftMask, {1, ShiftMask, 0} },
    { True, LockMask, {0, LockMask, 0} }
};

static XkbModsRec preAlpha[] = {
    {        0,        0, 0 },
    { LockMask, LockMask, 0 }
};

#define	NL_VMOD_MASK	0
static XkbKTMapEntryRec mapKeypad[] = {
    { True, ShiftMask, { 1, ShiftMask,            0 } },
    { False,        0, { 1,         0, NL_VMOD_MASK } }
};

static XkbKeyTypeRec canonicalTypes[XkbNumRequiredTypes] = {
    { { 0, 0, 0 },
      1,                             /* num_levels */
      0,                             /* map_count */
      NULL,         NULL,
      None,         NULL
    },
    { { ShiftMask, ShiftMask, 0 },
      2,                             /* num_levels */
      mapSize(map2Level),            /* map_count */
      map2Level,    NULL,
      None,         NULL
    },
    { { ShiftMask|LockMask, ShiftMask|LockMask, 0 },
      2,                            /* num_levels */
      mapSize(mapAlpha),            /* map_count */
      mapAlpha,     preAlpha,
      None,         NULL
    },
    { { ShiftMask, ShiftMask, NL_VMOD_MASK },
      2,                            /* num_levels */
      mapSize(mapKeypad),           /* map_count */
      mapKeypad,    NULL,
      None,         NULL
    }
};

Status
XkbInitCanonicalKeyTypes(XkbDescPtr xkb, unsigned which, int keypadVMod)
{
    XkbClientMapPtr map;
    XkbKeyTypePtr from, to;
    Status rtrn;

    if (!xkb)
        return BadMatch;
    rtrn = XkbAllocClientMap(xkb, XkbKeyTypesMask, XkbNumRequiredTypes);
    if (rtrn != Success)
        return rtrn;
    map = xkb->map;
    if ((which & XkbAllRequiredTypes) == 0)
        return Success;
    rtrn = Success;
    from = canonicalTypes;
    to = map->types;
    if (which & XkbOneLevelMask)
        rtrn = XkbCopyKeyType(&from[XkbOneLevelIndex], &to[XkbOneLevelIndex]);
    if ((which & XkbTwoLevelMask) && (rtrn == Success))
        rtrn = XkbCopyKeyType(&from[XkbTwoLevelIndex], &to[XkbTwoLevelIndex]);
    if ((which & XkbAlphabeticMask) && (rtrn == Success))
        rtrn =
            XkbCopyKeyType(&from[XkbAlphabeticIndex], &to[XkbAlphabeticIndex]);
    if ((which & XkbKeypadMask) && (rtrn == Success)) {
        XkbKeyTypePtr type;

        rtrn = XkbCopyKeyType(&from[XkbKeypadIndex], &to[XkbKeypadIndex]);
        type = &to[XkbKeypadIndex];
        if ((keypadVMod >= 0) && (keypadVMod < XkbNumVirtualMods) &&
            (rtrn == Success)) {
            type->mods.vmods = (1 << keypadVMod);
            type->map[0].active = True;
            type->map[0].mods.mask = ShiftMask;
            type->map[0].mods.real_mods = ShiftMask;
            type->map[0].mods.vmods = 0;
            type->map[0].level = 1;
            type->map[1].active = False;
            type->map[1].mods.mask = 0;
            type->map[1].mods.real_mods = 0;
            type->map[1].mods.vmods = (1 << keypadVMod);
            type->map[1].level = 1;
        }
    }
    return Success;
}

/***====================================================================***/

#define	CORE_SYM(i)	(i<map_width?core_syms[i]:NoSymbol)
#define	XKB_OFFSET(g,l)	(((g)*groupsWidth)+(l))

int
XkbKeyTypesForCoreSymbols(XkbDescPtr xkb,
                          int map_width,
                          KeySym *core_syms,
                          unsigned int protected,
                          int *types_inout,
                          KeySym *xkb_syms_rtrn)
{
    register int i;
    unsigned int empty;
    int nSyms[XkbNumKbdGroups];
    int nGroups, tmp, groupsWidth;

    /* Section 12.2 of the protocol describes this process in more detail */
    /* Step 1:  find the # of symbols in the core mapping per group */
    groupsWidth = 2;
    for (i = 0; i < XkbNumKbdGroups; i++) {
        if ((protected & (1 << i)) && (types_inout[i] < xkb->map->num_types)) {
            nSyms[i] = xkb->map->types[types_inout[i]].num_levels;
            if (nSyms[i] > groupsWidth)
                groupsWidth = nSyms[i];
        }
        else {
            types_inout[i] = XkbTwoLevelIndex;  /* don't really know, yet */
            nSyms[i] = 2;
        }
    }
    if (nSyms[XkbGroup1Index] < 2)
        nSyms[XkbGroup1Index] = 2;
    if (nSyms[XkbGroup2Index] < 2)
        nSyms[XkbGroup2Index] = 2;
    /* Step 2:  Copy the symbols from the core ordering to XKB ordering */
    /*          symbols in the core are in the order:                   */
    /*          G1L1 G1L2 G2L1 G2L2 [G1L[3-n]] [G2L[3-n]] [G3L*] [G3L*] */
    xkb_syms_rtrn[XKB_OFFSET(XkbGroup1Index, 0)] = CORE_SYM(0);
    xkb_syms_rtrn[XKB_OFFSET(XkbGroup1Index, 1)] = CORE_SYM(1);
    for (i = 2; i < nSyms[XkbGroup1Index]; i++) {
        xkb_syms_rtrn[XKB_OFFSET(XkbGroup1Index, i)] = CORE_SYM(2 + i);
    }
    xkb_syms_rtrn[XKB_OFFSET(XkbGroup2Index, 0)] = CORE_SYM(2);
    xkb_syms_rtrn[XKB_OFFSET(XkbGroup2Index, 1)] = CORE_SYM(3);
    tmp = 2 + (nSyms[XkbGroup1Index] - 2);      /* offset to extra group2 syms */
    for (i = 2; i < nSyms[XkbGroup2Index]; i++) {
        xkb_syms_rtrn[XKB_OFFSET(XkbGroup2Index, i)] = CORE_SYM(tmp + i);
    }
    tmp = nSyms[XkbGroup1Index] + nSyms[XkbGroup2Index];
    if ((tmp >= map_width) &&
        ((protected & (XkbExplicitKeyType3Mask | XkbExplicitKeyType4Mask)) ==
         0)) {
        nSyms[XkbGroup3Index] = 0;
        nSyms[XkbGroup4Index] = 0;
        nGroups = 2;
    }
    else {
        nGroups = 3;
        for (i = 0; i < nSyms[XkbGroup3Index]; i++, tmp++) {
            xkb_syms_rtrn[XKB_OFFSET(XkbGroup3Index, i)] = CORE_SYM(tmp);
        }
        if ((tmp < map_width) || (protected & XkbExplicitKeyType4Mask)) {
            nGroups = 4;
            for (i = 0; i < nSyms[XkbGroup4Index]; i++, tmp++) {
                xkb_syms_rtrn[XKB_OFFSET(XkbGroup4Index, i)] = CORE_SYM(tmp);
            }
        }
        else {
            nSyms[XkbGroup4Index] = 0;
        }
    }
    /* steps 3&4: alphanumeric expansion,  assign canonical types */
    empty = 0;
    for (i = 0; i < nGroups; i++) {
        KeySym *syms;

        syms = &xkb_syms_rtrn[XKB_OFFSET(i, 0)];
        if ((nSyms[i] > 1) && (syms[1] == NoSymbol) && (syms[0] != NoSymbol)) {
            KeySym upper, lower;

            XConvertCase(syms[0], &lower, &upper);
            if (upper != lower) {
                xkb_syms_rtrn[XKB_OFFSET(i, 0)] = lower;
                xkb_syms_rtrn[XKB_OFFSET(i, 1)] = upper;
                if ((protected & (1 << i)) == 0)
                    types_inout[i] = XkbAlphabeticIndex;
            }
            else if ((protected & (1 << i)) == 0) {
                types_inout[i] = XkbOneLevelIndex;
                /*      nSyms[i]=       1; */
            }
        }
        if (((protected & (1 << i)) == 0) &&
            (types_inout[i] == XkbTwoLevelIndex)) {
            if (IsKeypadKey(syms[0]) || IsKeypadKey(syms[1]))
                types_inout[i] = XkbKeypadIndex;
            else {
                KeySym upper, lower;

                XConvertCase(syms[0], &lower, &upper);
                if ((syms[0] == lower) && (syms[1] == upper))
                    types_inout[i] = XkbAlphabeticIndex;
            }
        }
        if (syms[0] == NoSymbol) {
            register int n;
            Bool found;

            for (n = 1, found = False; (!found) && (n < nSyms[i]); n++) {
                found = (syms[n] != NoSymbol);
            }
            if (!found)
                empty |= (1 << i);
        }
    }
    /* step 5: squoosh out empty groups */
    if (empty) {
        for (i = nGroups - 1; i >= 0; i--) {
            if (((empty & (1 << i)) == 0) || (protected & (1 << i)))
                break;
            nGroups--;
        }
    }
    if (nGroups < 1)
        return 0;

    /* step 6: replicate group 1 into group two, if necessary */
    if ((nGroups > 1) &&
        ((empty & (XkbGroup1Mask | XkbGroup2Mask)) == XkbGroup2Mask)) {
        if ((protected & (XkbExplicitKeyType1Mask | XkbExplicitKeyType2Mask)) ==
            0) {
            nSyms[XkbGroup2Index] = nSyms[XkbGroup1Index];
            types_inout[XkbGroup2Index] = types_inout[XkbGroup1Index];
            memcpy((char *) &xkb_syms_rtrn[2], (char *) xkb_syms_rtrn,
                   2 * sizeof(KeySym));
        }
        else if (types_inout[XkbGroup1Index] == types_inout[XkbGroup2Index]) {
            memcpy((char *) &xkb_syms_rtrn[nSyms[XkbGroup1Index]],
                   (char *) xkb_syms_rtrn,
                   nSyms[XkbGroup1Index] * sizeof(KeySym));
        }
    }

    /* step 7: check for all groups identical or all width 1 */
    if (nGroups > 1) {
        Bool sameType, allOneLevel;

        allOneLevel = (xkb->map->types[types_inout[0]].num_levels == 1);
        for (i = 1, sameType = True; (allOneLevel || sameType) && (i < nGroups);
             i++) {
            sameType = (sameType &&
                        (types_inout[i] == types_inout[XkbGroup1Index]));
            if (allOneLevel)
                allOneLevel = (xkb->map->types[types_inout[i]].num_levels == 1);
        }
        if ((sameType) &&
            (!(protected &
               (XkbExplicitKeyTypesMask & ~XkbExplicitKeyType1Mask)))) {
            register int s;
            Bool identical;

            for (i = 1, identical = True; identical && (i < nGroups); i++) {
                KeySym *syms;

                syms = &xkb_syms_rtrn[XKB_OFFSET(i, 0)];
                for (s = 0; identical && (s < nSyms[i]); s++) {
                    if (syms[s] != xkb_syms_rtrn[s])
                        identical = False;
                }
            }
            if (identical)
                nGroups = 1;
        }
        if (allOneLevel && (nGroups > 1)) {
            KeySym *syms;

            syms = &xkb_syms_rtrn[nSyms[XkbGroup1Index]];
            nSyms[XkbGroup1Index] = 1;
            for (i = 1; i < nGroups; i++) {
                xkb_syms_rtrn[i] = syms[0];
                syms += nSyms[i];
                nSyms[i] = 1;
            }
        }
    }
    return nGroups;
}

static XkbSymInterpretPtr
_XkbFindMatchingInterp(XkbDescPtr xkb,
                       KeySym sym,
                       unsigned int real_mods,
                       unsigned int level)
{
    register unsigned i;
    XkbSymInterpretPtr interp, rtrn;
    CARD8 mods;

    rtrn = NULL;
    interp = xkb->compat->sym_interpret;
    for (i = 0; i < xkb->compat->num_si; i++, interp++) {
        if ((interp->sym == NoSymbol) || (sym == interp->sym)) {
            int match;

            if ((level == 0) || ((interp->match & XkbSI_LevelOneOnly) == 0))
                mods = real_mods;
            else
                mods = 0;
            switch (interp->match & XkbSI_OpMask) {
            case XkbSI_NoneOf:
                match = ((interp->mods & mods) == 0);
                break;
            case XkbSI_AnyOfOrNone:
                match = ((mods == 0) || ((interp->mods & mods) != 0));
                break;
            case XkbSI_AnyOf:
                match = ((interp->mods & mods) != 0);
                break;
            case XkbSI_AllOf:
                match = ((interp->mods & mods) == interp->mods);
                break;
            case XkbSI_Exactly:
                match = (interp->mods == mods);
                break;
            default:
                match = 0;
                break;
            }
            if (match) {
                if (interp->sym != NoSymbol) {
                    return interp;
                }
                else if (rtrn == NULL) {
                    rtrn = interp;
                }
            }
        }
    }
    return rtrn;
}

static void
_XkbAddKeyChange(KeyCode *pFirst, unsigned char *pNum, KeyCode newKey)
{
    KeyCode last;

    last = (*pFirst) + (*pNum);
    if (newKey < *pFirst) {
        *pFirst = newKey;
        *pNum = (last - newKey) + 1;
    }
    else if (newKey > last) {
        *pNum = (last - *pFirst) + 1;
    }
    return;
}

static void
_XkbSetActionKeyMods(XkbDescPtr xkb, XkbAction *act, unsigned mods)
{
    unsigned tmp;

    switch (act->type) {
    case XkbSA_SetMods:
    case XkbSA_LatchMods:
    case XkbSA_LockMods:
        if (act->mods.flags & XkbSA_UseModMapMods)
            act->mods.real_mods = act->mods.mask = mods;
        if ((tmp = XkbModActionVMods(&act->mods)) != 0) {
            XkbVirtualModsToReal(xkb, tmp, &tmp);
            act->mods.mask |= tmp;
        }
        break;
    case XkbSA_ISOLock:
        if (act->iso.flags & XkbSA_UseModMapMods)
            act->iso.real_mods = act->iso.mask = mods;
        if ((tmp = XkbModActionVMods(&act->iso)) != 0) {
            XkbVirtualModsToReal(xkb, tmp, &tmp);
            act->iso.mask |= tmp;
        }
        break;
    }
    return;
}

#define	IBUF_SIZE	8

Bool
XkbApplyCompatMapToKey(XkbDescPtr xkb, KeyCode key, XkbChangesPtr changes)
{
    KeySym *syms;
    unsigned char explicit, mods;
    XkbSymInterpretPtr *interps, ibuf[IBUF_SIZE];
    int n, nSyms, found;
    unsigned changed, tmp;

    if ((!xkb) || (!xkb->map) || (!xkb->map->key_sym_map) ||
        (!xkb->compat) || (!xkb->compat->sym_interpret) ||
        (key < xkb->min_key_code) || (key > xkb->max_key_code)) {
        return False;
    }
    if (((!xkb->server) || (!xkb->server->key_acts)) &&
        (XkbAllocServerMap(xkb, XkbAllServerInfoMask, 0) != Success)) {
        return False;
    }
    changed = 0;                /* keeps track of what has changed in _this_ call */
    explicit = xkb->server->explicit[key];
    if (explicit & XkbExplicitInterpretMask)    /* nothing to do */
        return True;
    mods = (xkb->map->modmap ? xkb->map->modmap[key] : 0);
    nSyms = XkbKeyNumSyms(xkb, key);
    syms = XkbKeySymsPtr(xkb, key);
    if (nSyms > IBUF_SIZE) {
        interps = _XkbTypedCalloc(nSyms, XkbSymInterpretPtr);
        if (interps == NULL) {
            interps = ibuf;
            nSyms = IBUF_SIZE;
        }
    }
    else {
        interps = ibuf;
    }
    found = 0;
    for (n = 0; n < nSyms; n++) {
        unsigned level = (n % XkbKeyGroupsWidth(xkb, key));

        interps[n] = NULL;
        if (syms[n] != NoSymbol) {
            interps[n] = _XkbFindMatchingInterp(xkb, syms[n], mods, level);
            if (interps[n] && interps[n]->act.type != XkbSA_NoAction)
                found++;
            else
                interps[n] = NULL;
        }
    }
    /* 1/28/96 (ef) -- XXX! WORKING HERE */
    if (!found) {
        if (xkb->server->key_acts[key] != 0) {
            xkb->server->key_acts[key] = 0;
            changed |= XkbKeyActionsMask;
        }
    }
    else {
        XkbAction *pActs;
        unsigned int new_vmodmask;

        changed |= XkbKeyActionsMask;
        pActs = XkbResizeKeyActions(xkb, key, nSyms);
        if (!pActs) {
            if (nSyms > IBUF_SIZE)
                Xfree(interps);
            return False;
        }
        new_vmodmask = 0;
        for (n = 0; n < nSyms; n++) {
            if (interps[n]) {
                unsigned effMods;

                pActs[n] = *((XkbAction *) &interps[n]->act);
                if ((n == 0) || ((interps[n]->match & XkbSI_LevelOneOnly) == 0)) {
                    effMods = mods;
                    if (interps[n]->virtual_mod != XkbNoModifier)
                        new_vmodmask |= (1 << interps[n]->virtual_mod);
                }
                else
                    effMods = 0;
                _XkbSetActionKeyMods(xkb, &pActs[n], effMods);
            }
            else
                pActs[n].type = XkbSA_NoAction;
        }
        if (((explicit & XkbExplicitVModMapMask) == 0) &&
            (xkb->server->vmodmap[key] != new_vmodmask)) {
            changed |= XkbVirtualModMapMask;
            xkb->server->vmodmap[key] = new_vmodmask;
        }
        if (interps[0]) {
            if ((interps[0]->flags & XkbSI_LockingKey) &&
                ((explicit & XkbExplicitBehaviorMask) == 0)) {
                xkb->server->behaviors[key].type = XkbKB_Lock;
                changed |= XkbKeyBehaviorsMask;
            }
            if (((explicit & XkbExplicitAutoRepeatMask) == 0) && (xkb->ctrls)) {
                CARD8 old;

                old = xkb->ctrls->per_key_repeat[key / 8];
                if (interps[0]->flags & XkbSI_AutoRepeat)
                    xkb->ctrls->per_key_repeat[key / 8] |= (1 << (key % 8));
                else
                    xkb->ctrls->per_key_repeat[key / 8] &= ~(1 << (key % 8));
                if (changes && (old != xkb->ctrls->per_key_repeat[key / 8]))
                    changes->ctrls.changed_ctrls |= XkbPerKeyRepeatMask;
            }
        }
    }
    if ((!found) || (interps[0] == NULL)) {
        if (((explicit & XkbExplicitAutoRepeatMask) == 0) && (xkb->ctrls)) {
            CARD8 old;

            old = xkb->ctrls->per_key_repeat[key / 8];
#ifdef RETURN_SHOULD_REPEAT
            if (*XkbKeySymsPtr(xkb, key) != XK_Return)
#endif
                xkb->ctrls->per_key_repeat[key / 8] |= (1 << (key % 8));
            if (changes && (old != xkb->ctrls->per_key_repeat[key / 8]))
                changes->ctrls.changed_ctrls |= XkbPerKeyRepeatMask;
        }
        if (((explicit & XkbExplicitBehaviorMask) == 0) &&
            (xkb->server->behaviors[key].type == XkbKB_Lock)) {
            xkb->server->behaviors[key].type = XkbKB_Default;
            changed |= XkbKeyBehaviorsMask;
        }
    }
    if (changes) {
        XkbMapChangesPtr mc;

        mc = &changes->map;
        tmp = (changed & mc->changed);
        if (tmp & XkbKeyActionsMask)
            _XkbAddKeyChange(&mc->first_key_act, &mc->num_key_acts, key);
        else if (changed & XkbKeyActionsMask) {
            mc->changed |= XkbKeyActionsMask;
            mc->first_key_act = key;
            mc->num_key_acts = 1;
        }
        if (tmp & XkbKeyBehaviorsMask) {
            _XkbAddKeyChange(&mc->first_key_behavior, &mc->num_key_behaviors,
                             key);
        }
        else if (changed & XkbKeyBehaviorsMask) {
            mc->changed |= XkbKeyBehaviorsMask;
            mc->first_key_behavior = key;
            mc->num_key_behaviors = 1;
        }
        if (tmp & XkbVirtualModMapMask)
            _XkbAddKeyChange(&mc->first_vmodmap_key, &mc->num_vmodmap_keys,
                             key);
        else if (changed & XkbVirtualModMapMask) {
            mc->changed |= XkbVirtualModMapMask;
            mc->first_vmodmap_key = key;
            mc->num_vmodmap_keys = 1;
        }
        mc->changed |= changed;
    }
    if (interps != ibuf)
        _XkbFree(interps);
    return True;
}

Bool
XkbUpdateMapFromCore(XkbDescPtr xkb,
                     KeyCode first_key,
                     int num_keys,
                     int map_width,
                     KeySym *core_keysyms,
                     XkbChangesPtr changes)
{
    register int key, last_key;
    KeySym *syms;

    syms = &core_keysyms[(first_key - xkb->min_key_code) * map_width];
    if (changes) {
        if (changes->map.changed & XkbKeySymsMask) {
            _XkbAddKeyChange(&changes->map.first_key_sym,
                             &changes->map.num_key_syms, first_key);
            if (num_keys > 1) {
                _XkbAddKeyChange(&changes->map.first_key_sym,
                                 &changes->map.num_key_syms,
                                 first_key + num_keys - 1);
            }
        }
        else {
            changes->map.changed |= XkbKeySymsMask;
            changes->map.first_key_sym = first_key;
            changes->map.num_key_syms = num_keys;
        }
    }
    last_key = first_key + num_keys - 1;
    for (key = first_key; key <= last_key; key++, syms += map_width) {
        XkbMapChangesPtr mc;
        unsigned explicit;
        KeySym tsyms[XkbMaxSymsPerKey];
        int types[XkbNumKbdGroups];
        int nG;

        explicit = xkb->server->explicit[key] & XkbExplicitKeyTypesMask;
        types[XkbGroup1Index] = XkbKeyKeyTypeIndex(xkb, key, XkbGroup1Index);
        types[XkbGroup2Index] = XkbKeyKeyTypeIndex(xkb, key, XkbGroup2Index);
        types[XkbGroup3Index] = XkbKeyKeyTypeIndex(xkb, key, XkbGroup3Index);
        types[XkbGroup4Index] = XkbKeyKeyTypeIndex(xkb, key, XkbGroup4Index);
        nG = XkbKeyTypesForCoreSymbols(xkb, map_width, syms, explicit, types,
                                       tsyms);
        if (changes)
            mc = &changes->map;
        else
            mc = NULL;
        XkbChangeTypesOfKey(xkb, key, nG, XkbAllGroupsMask, types, mc);
        memcpy((char *) XkbKeySymsPtr(xkb, key), (char *) tsyms,
               XkbKeyNumSyms(xkb, key) * sizeof(KeySym));
        XkbApplyCompatMapToKey(xkb, key, changes);
    }

    if ((xkb->map->modmap != NULL) && (changes) &&
        (changes->map.changed & (XkbVirtualModMapMask | XkbModifierMapMask))) {
        unsigned char newVMods[XkbNumVirtualMods];
        register unsigned bit, i;
        unsigned present;

        bzero(newVMods, XkbNumVirtualMods);
        present = 0;
        for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
            if (xkb->server->vmodmap[key] == 0)
                continue;
            for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
                if (bit & xkb->server->vmodmap[key]) {
                    present |= bit;
                    newVMods[i] |= xkb->map->modmap[key];
                }
            }
        }
        for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
            if ((bit & present) && (newVMods[i] != xkb->server->vmods[i])) {
                changes->map.changed |= XkbVirtualModsMask;
                changes->map.vmods |= bit;
                xkb->server->vmods[i] = newVMods[i];
            }
        }
    }
    if (changes && (changes->map.changed & XkbVirtualModsMask))
        XkbApplyVirtualModChanges(xkb, changes->map.vmods, changes);
    return True;
}

Status
XkbChangeTypesOfKey(XkbDescPtr xkb,
                    int key,
                    int nGroups,
                    unsigned groups,
                    int *newTypesIn,
                    XkbMapChangesPtr changes)
{
    XkbKeyTypePtr pOldType, pNewType;
    register int i;
    int width, nOldGroups, oldWidth, newTypes[XkbNumKbdGroups];

    if ((!xkb) || (!XkbKeycodeInRange(xkb, key)) || (!xkb->map) ||
        (!xkb->map->types) || ((groups & XkbAllGroupsMask) == 0) ||
        (nGroups > XkbNumKbdGroups)) {
        return BadMatch;
    }
    if (nGroups == 0) {
        for (i = 0; i < XkbNumKbdGroups; i++) {
            xkb->map->key_sym_map[key].kt_index[i] = XkbOneLevelIndex;
        }
        i = xkb->map->key_sym_map[key].group_info;
        i = XkbSetNumGroups(i, 0);
        xkb->map->key_sym_map[key].group_info = i;
        XkbResizeKeySyms(xkb, key, 0);
        return Success;
    }

    nOldGroups = XkbKeyNumGroups(xkb, key);
    oldWidth = XkbKeyGroupsWidth(xkb, key);
    for (width = i = 0; i < nGroups; i++) {
        if (groups & (1 << i))
            newTypes[i] = newTypesIn[i];
        else if (i < nOldGroups)
            newTypes[i] = XkbKeyKeyTypeIndex(xkb, key, i);
        else if (nOldGroups > 0)
            newTypes[i] = XkbKeyKeyTypeIndex(xkb, key, XkbGroup1Index);
        else
            newTypes[i] = XkbTwoLevelIndex;
        if (newTypes[i] > xkb->map->num_types)
            return BadMatch;
        pNewType = &xkb->map->types[newTypes[i]];
        if (pNewType->num_levels > width)
            width = pNewType->num_levels;
    }
    if ((xkb->ctrls) && (nGroups > xkb->ctrls->num_groups))
        xkb->ctrls->num_groups = nGroups;
    if ((width != oldWidth) || (nGroups != nOldGroups)) {
        KeySym oldSyms[XkbMaxSymsPerKey], *pSyms;
        int nCopy;

        if (nOldGroups == 0) {
            pSyms = XkbResizeKeySyms(xkb, key, width * nGroups);
            if (pSyms != NULL) {
                i = xkb->map->key_sym_map[key].group_info;
                i = XkbSetNumGroups(i, nGroups);
                xkb->map->key_sym_map[key].group_info = i;
                xkb->map->key_sym_map[key].width = width;
                for (i = 0; i < nGroups; i++) {
                    xkb->map->key_sym_map[key].kt_index[i] = newTypes[i];
                }
                return Success;
            }
            return BadAlloc;
        }
        pSyms = XkbKeySymsPtr(xkb, key);
        memcpy(oldSyms, pSyms, (size_t) XkbKeyNumSyms(xkb, key) * sizeof(KeySym));
        pSyms = XkbResizeKeySyms(xkb, key, width * nGroups);
        if (pSyms == NULL)
            return BadAlloc;
        bzero(pSyms, width * nGroups * sizeof(KeySym));
        for (i = 0; (i < nGroups) && (i < nOldGroups); i++) {
            pOldType = XkbKeyKeyType(xkb, key, i);
            pNewType = &xkb->map->types[newTypes[i]];
            if (pNewType->num_levels > pOldType->num_levels)
                nCopy = pOldType->num_levels;
            else
                nCopy = pNewType->num_levels;
            memcpy(&pSyms[i * width], &oldSyms[i * oldWidth],
                   nCopy * sizeof(KeySym));
        }
        if (XkbKeyHasActions(xkb, key)) {
            XkbAction oldActs[XkbMaxSymsPerKey], *pActs;

            pActs = XkbKeyActionsPtr(xkb, key);
            memcpy(oldActs, pActs, (size_t) XkbKeyNumSyms(xkb, key) * sizeof(XkbAction));
            pActs = XkbResizeKeyActions(xkb, key, width * nGroups);
            if (pActs == NULL)
                return BadAlloc;
            bzero(pActs, width * nGroups * sizeof(XkbAction));
            for (i = 0; (i < nGroups) && (i < nOldGroups); i++) {
                pOldType = XkbKeyKeyType(xkb, key, i);
                pNewType = &xkb->map->types[newTypes[i]];
                if (pNewType->num_levels > pOldType->num_levels)
                    nCopy = pOldType->num_levels;
                else
                    nCopy = pNewType->num_levels;
                memcpy(&pActs[i * width], &oldActs[i * oldWidth],
                       nCopy * sizeof(XkbAction));
            }
        }
        i = xkb->map->key_sym_map[key].group_info;
        i = XkbSetNumGroups(i, nGroups);
        xkb->map->key_sym_map[key].group_info = i;
        xkb->map->key_sym_map[key].width = width;
    }
    width = 0;
    for (i = 0; i < nGroups; i++) {
        xkb->map->key_sym_map[key].kt_index[i] = newTypes[i];
        if (xkb->map->types[newTypes[i]].num_levels > width)
            width = xkb->map->types[newTypes[i]].num_levels;
    }
    xkb->map->key_sym_map[key].width = width;
    if (changes != NULL) {
        if (changes->changed & XkbKeySymsMask) {
            _XkbAddKeyChange(&changes->first_key_sym, &changes->num_key_syms,
                             key);
        }
        else {
            changes->changed |= XkbKeySymsMask;
            changes->first_key_sym = key;
            changes->num_key_syms = 1;
        }
    }
    return Success;
}

/***====================================================================***/

Bool
XkbVirtualModsToReal(XkbDescPtr xkb, unsigned virtual_mask, unsigned *mask_rtrn)
{
    register int i, bit;
    register unsigned mask;

    if (xkb == NULL)
        return False;
    if (virtual_mask == 0) {
        *mask_rtrn = 0;
        return True;
    }
    if (xkb->server == NULL)
        return False;
    for (i = mask = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (virtual_mask & bit)
            mask |= xkb->server->vmods[i];
    }
    *mask_rtrn = mask;
    return True;
}

/***====================================================================***/

Bool
XkbUpdateActionVirtualMods(XkbDescPtr xkb, XkbAction *act, unsigned changed)
{
    unsigned int tmp;

    switch (act->type) {
    case XkbSA_SetMods:
    case XkbSA_LatchMods:
    case XkbSA_LockMods:
        if (((tmp = XkbModActionVMods(&act->mods)) & changed) != 0) {
            XkbVirtualModsToReal(xkb, tmp, &tmp);
            act->mods.mask = act->mods.real_mods;
            act->mods.mask |= tmp;
            return True;
        }
        break;
    case XkbSA_ISOLock:
        if ((((tmp = XkbModActionVMods(&act->iso)) != 0) & changed) != 0) {
            XkbVirtualModsToReal(xkb, tmp, &tmp);
            act->iso.mask = act->iso.real_mods;
            act->iso.mask |= tmp;
            return True;
        }
        break;
    }
    return False;
}

void
XkbUpdateKeyTypeVirtualMods(XkbDescPtr xkb,
                            XkbKeyTypePtr type,
                            unsigned int changed,
                            XkbChangesPtr changes)
{
    register unsigned int i;
    unsigned int mask = 0;

    XkbVirtualModsToReal(xkb, type->mods.vmods, &mask);
    type->mods.mask = type->mods.real_mods | mask;
    if ((type->map_count > 0) && (type->mods.vmods != 0)) {
        XkbKTMapEntryPtr entry;

        for (i = 0, entry = type->map; i < type->map_count; i++, entry++) {
            if (entry->mods.vmods != 0) {
                XkbVirtualModsToReal(xkb, entry->mods.vmods, &mask);
                entry->mods.mask = entry->mods.real_mods | mask;
                /* entry is active if vmods are bound */
                entry->active = (mask != 0);
            }
            else
                entry->active = 1;
        }
    }
    if (changes) {
        int type_ndx;

        type_ndx = type - xkb->map->types;
        if ((type_ndx < 0) || (type_ndx > xkb->map->num_types))
            return;
        if (changes->map.changed & XkbKeyTypesMask) {
            int last;

            last = changes->map.first_type + changes->map.num_types - 1;
            if (type_ndx < changes->map.first_type) {
                changes->map.first_type = type_ndx;
                changes->map.num_types = (last - type_ndx) + 1;
            }
            else if (type_ndx > last) {
                changes->map.num_types =
                    (type_ndx - changes->map.first_type) + 1;
            }
        }
        else {
            changes->map.changed |= XkbKeyTypesMask;
            changes->map.first_type = type_ndx;
            changes->map.num_types = 1;
        }
    }
    return;
}

Bool
XkbApplyVirtualModChanges(XkbDescPtr xkb,
                          unsigned changed,
                          XkbChangesPtr changes)
{
    register int i;
    unsigned int checkState = 0;

    if ((!xkb) || (!xkb->map) || (changed == 0))
        return False;
    for (i = 0; i < xkb->map->num_types; i++) {
        if (xkb->map->types[i].mods.vmods & changed)
            XkbUpdateKeyTypeVirtualMods(xkb, &xkb->map->types[i], changed,
                                        changes);
    }
    if (changed & xkb->ctrls->internal.vmods) {
        unsigned int newMask;

        XkbVirtualModsToReal(xkb, xkb->ctrls->internal.vmods, &newMask);
        newMask |= xkb->ctrls->internal.real_mods;
        if (xkb->ctrls->internal.mask != newMask) {
            xkb->ctrls->internal.mask = newMask;
            if (changes) {
                changes->ctrls.changed_ctrls |= XkbInternalModsMask;
                checkState = True;
            }
        }
    }
    if (changed & xkb->ctrls->ignore_lock.vmods) {
        unsigned int newMask;

        XkbVirtualModsToReal(xkb, xkb->ctrls->ignore_lock.vmods, &newMask);
        newMask |= xkb->ctrls->ignore_lock.real_mods;
        if (xkb->ctrls->ignore_lock.mask != newMask) {
            xkb->ctrls->ignore_lock.mask = newMask;
            if (changes) {
                changes->ctrls.changed_ctrls |= XkbIgnoreLockModsMask;
                checkState = True;
            }
        }
    }
    if (xkb->indicators != NULL) {
        XkbIndicatorMapPtr map;

        map = &xkb->indicators->maps[0];
        for (i = 0; i < XkbNumIndicators; i++, map++) {
            if (map->mods.vmods & changed) {
                unsigned int newMask;

                XkbVirtualModsToReal(xkb, map->mods.vmods, &newMask);
                newMask |= map->mods.real_mods;
                if (newMask != map->mods.mask) {
                    map->mods.mask = newMask;
                    if (changes) {
                        changes->indicators.map_changes |= (1 << i);
                        checkState = True;
                    }
                }
            }
        }
    }
    if (xkb->compat != NULL) {
        XkbCompatMapPtr compat;

        compat = xkb->compat;
        for (i = 0; i < XkbNumKbdGroups; i++) {
            unsigned int newMask;

            XkbVirtualModsToReal(xkb, compat->groups[i].vmods, &newMask);
            newMask |= compat->groups[i].real_mods;
            if (compat->groups[i].mask != newMask) {
                compat->groups[i].mask = newMask;
                if (changes) {
                    changes->compat.changed_groups |= (1 << i);
                    checkState = True;
                }
            }
        }
    }
    if (xkb->map && xkb->server) {
        int highChange = 0, lowChange = -1;

        for (i = xkb->min_key_code; i <= xkb->max_key_code; i++) {
            if (XkbKeyHasActions(xkb, i)) {
                register XkbAction *pAct;
                register int n;

                pAct = XkbKeyActionsPtr(xkb, i);
                for (n = XkbKeyNumActions(xkb, i); n > 0; n--, pAct++) {
                    if ((pAct->type != XkbSA_NoAction) &&
                        XkbUpdateActionVirtualMods(xkb, pAct, changed)) {
                        if (lowChange < 0)
                            lowChange = i;
                        highChange = i;
                    }
                }
            }
        }
        if (changes && (lowChange > 0)) {       /* something changed */
            if (changes->map.changed & XkbKeyActionsMask) {
                int last;

                if (changes->map.first_key_act < lowChange)
                    lowChange = changes->map.first_key_act;
                last =
                    changes->map.first_key_act + changes->map.num_key_acts - 1;
                if (last > highChange)
                    highChange = last;
            }
            changes->map.changed |= XkbKeyActionsMask;
            changes->map.first_key_act = lowChange;
            changes->map.num_key_acts = (highChange - lowChange) + 1;
        }
    }
    return checkState;
}
