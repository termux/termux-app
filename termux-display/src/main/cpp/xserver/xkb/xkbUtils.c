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
/*

Copyright © 2008 Red Hat Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "os.h"
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#define	XK_CYRILLIC
#include <X11/keysym.h>
#include "misc.h"
#include "inputstr.h"
#include "eventstr.h"

#define	XKBSRV_NEED_FILE_FUNCS
#include <xkbsrv.h>
#include "xkbgeom.h"
#include "xkb.h"

/***====================================================================***/

int
_XkbLookupAnyDevice(DeviceIntPtr *pDev, int id, ClientPtr client,
                    Mask access_mode, int *xkb_err)
{
    int rc = XkbKeyboardErrorCode;

    if (id == XkbUseCoreKbd)
        id = PickKeyboard(client)->id;
    else if (id == XkbUseCorePtr)
        id = PickPointer(client)->id;

    rc = dixLookupDevice(pDev, id, client, access_mode);
    if (rc != Success)
        *xkb_err = XkbErr_BadDevice;

    return rc;
}

int
_XkbLookupKeyboard(DeviceIntPtr *pDev, int id, ClientPtr client,
                   Mask access_mode, int *xkb_err)
{
    DeviceIntPtr dev;
    int rc;

    if (id == XkbDfltXIId)
        id = XkbUseCoreKbd;

    rc = _XkbLookupAnyDevice(pDev, id, client, access_mode, xkb_err);
    if (rc != Success)
        return rc;

    dev = *pDev;
    if (!dev->key || !dev->key->xkbInfo) {
        *pDev = NULL;
        *xkb_err = XkbErr_BadClass;
        return XkbKeyboardErrorCode;
    }
    return Success;
}

int
_XkbLookupBellDevice(DeviceIntPtr *pDev, int id, ClientPtr client,
                     Mask access_mode, int *xkb_err)
{
    DeviceIntPtr dev;
    int rc;

    rc = _XkbLookupAnyDevice(pDev, id, client, access_mode, xkb_err);
    if (rc != Success)
        return rc;

    dev = *pDev;
    if (!dev->kbdfeed && !dev->bell) {
        *pDev = NULL;
        *xkb_err = XkbErr_BadClass;
        return XkbKeyboardErrorCode;
    }
    return Success;
}

int
_XkbLookupLedDevice(DeviceIntPtr *pDev, int id, ClientPtr client,
                    Mask access_mode, int *xkb_err)
{
    DeviceIntPtr dev;
    int rc;

    if (id == XkbDfltXIId)
        id = XkbUseCorePtr;

    rc = _XkbLookupAnyDevice(pDev, id, client, access_mode, xkb_err);
    if (rc != Success)
        return rc;

    dev = *pDev;
    if (!dev->kbdfeed && !dev->leds) {
        *pDev = NULL;
        *xkb_err = XkbErr_BadClass;
        return XkbKeyboardErrorCode;
    }
    return Success;
}

int
_XkbLookupButtonDevice(DeviceIntPtr *pDev, int id, ClientPtr client,
                       Mask access_mode, int *xkb_err)
{
    DeviceIntPtr dev;
    int rc;

    rc = _XkbLookupAnyDevice(pDev, id, client, access_mode, xkb_err);
    if (rc != Success)
        return rc;

    dev = *pDev;
    if (!dev->button) {
        *pDev = NULL;
        *xkb_err = XkbErr_BadClass;
        return XkbKeyboardErrorCode;
    }
    return Success;
}

void
XkbSetActionKeyMods(XkbDescPtr xkb, XkbAction *act, unsigned mods)
{
    register unsigned tmp;

    switch (act->type) {
    case XkbSA_SetMods:
    case XkbSA_LatchMods:
    case XkbSA_LockMods:
        if (act->mods.flags & XkbSA_UseModMapMods)
            act->mods.real_mods = act->mods.mask = mods;
        if ((tmp = XkbModActionVMods(&act->mods)) != 0)
            act->mods.mask |= XkbMaskForVMask(xkb, tmp);
        break;
    case XkbSA_ISOLock:
        if (act->iso.flags & XkbSA_UseModMapMods)
            act->iso.real_mods = act->iso.mask = mods;
        if ((tmp = XkbModActionVMods(&act->iso)) != 0)
            act->iso.mask |= XkbMaskForVMask(xkb, tmp);
        break;
    }
    return;
}

unsigned
XkbMaskForVMask(XkbDescPtr xkb, unsigned vmask)
{
    register int i, bit;
    register unsigned mask;

    for (mask = i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (vmask & bit)
            mask |= xkb->server->vmods[i];
    }
    return mask;
}

/***====================================================================***/

void
XkbUpdateKeyTypesFromCore(DeviceIntPtr pXDev,
                          KeySymsPtr pCore,
                          KeyCode first, CARD8 num, XkbChangesPtr changes)
{
    XkbDescPtr xkb;
    unsigned key, nG, explicit;
    int types[XkbNumKbdGroups];
    KeySym tsyms[XkbMaxSymsPerKey] = {NoSymbol}, *syms;
    XkbMapChangesPtr mc;

    xkb = pXDev->key->xkbInfo->desc;
    if (first + num - 1 > xkb->max_key_code) {
        /* 1/12/95 (ef) -- XXX! should allow XKB structures to grow */
        num = xkb->max_key_code - first + 1;
    }

    mc = (changes ? (&changes->map) : NULL);

    syms = &pCore->map[(first - pCore->minKeyCode) * pCore->mapWidth];
    for (key = first; key < (first + num); key++, syms += pCore->mapWidth) {
        explicit = xkb->server->explicit[key] & XkbExplicitKeyTypesMask;
        types[XkbGroup1Index] = XkbKeyKeyTypeIndex(xkb, key, XkbGroup1Index);
        types[XkbGroup2Index] = XkbKeyKeyTypeIndex(xkb, key, XkbGroup2Index);
        types[XkbGroup3Index] = XkbKeyKeyTypeIndex(xkb, key, XkbGroup3Index);
        types[XkbGroup4Index] = XkbKeyKeyTypeIndex(xkb, key, XkbGroup4Index);
        nG = XkbKeyTypesForCoreSymbols(xkb, pCore->mapWidth, syms, explicit,
                                       types, tsyms);
        XkbChangeTypesOfKey(xkb, key, nG, XkbAllGroupsMask, types, mc);
        memcpy((char *) XkbKeySymsPtr(xkb, key), (char *) tsyms,
               XkbKeyNumSyms(xkb, key) * sizeof(KeySym));
    }
    if (changes->map.changed & XkbKeySymsMask) {
        CARD8 oldLast, newLast;

        oldLast = changes->map.first_key_sym + changes->map.num_key_syms - 1;
        newLast = first + num - 1;

        if (first < changes->map.first_key_sym)
            changes->map.first_key_sym = first;
        if (oldLast > newLast)
            newLast = oldLast;
        changes->map.num_key_syms = newLast - changes->map.first_key_sym + 1;
    }
    else {
        changes->map.changed |= XkbKeySymsMask;
        changes->map.first_key_sym = first;
        changes->map.num_key_syms = num;
    }
    return;
}

void
XkbUpdateDescActions(XkbDescPtr xkb,
                     KeyCode first, CARD8 num, XkbChangesPtr changes)
{
    register unsigned key;

    for (key = first; key < (first + num); key++) {
        XkbApplyCompatMapToKey(xkb, key, changes);
    }

    if (changes->map.changed & (XkbVirtualModMapMask | XkbModifierMapMask)) {
        unsigned char newVMods[XkbNumVirtualMods];
        register unsigned bit, i;
        unsigned present;

        memset(newVMods, 0, XkbNumVirtualMods);
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
    if (changes->map.changed & XkbVirtualModsMask)
        XkbApplyVirtualModChanges(xkb, changes->map.vmods, changes);

    if (changes->map.changed & XkbKeyActionsMask) {
        CARD8 oldLast, newLast;

        oldLast = changes->map.first_key_act + changes->map.num_key_acts - 1;
        newLast = first + num - 1;

        if (first < changes->map.first_key_act)
            changes->map.first_key_act = first;
        if (newLast > oldLast)
            newLast = oldLast;
        changes->map.num_key_acts = newLast - changes->map.first_key_act + 1;
    }
    else {
        changes->map.changed |= XkbKeyActionsMask;
        changes->map.first_key_act = first;
        changes->map.num_key_acts = num;
    }
    return;
}

void
XkbUpdateActions(DeviceIntPtr pXDev,
                 KeyCode first,
                 CARD8 num,
                 XkbChangesPtr changes,
                 unsigned *needChecksRtrn, XkbEventCausePtr cause)
{
    XkbSrvInfoPtr xkbi;
    XkbDescPtr xkb;
    CARD8 *repeat;

    if (needChecksRtrn)
        *needChecksRtrn = 0;
    xkbi = pXDev->key->xkbInfo;
    xkb = xkbi->desc;
    repeat = xkb->ctrls->per_key_repeat;

    /* before letting XKB do any changes, copy the current core values */
    if (pXDev->kbdfeed)
        memcpy(repeat, pXDev->kbdfeed->ctrl.autoRepeats, XkbPerKeyBitArraySize);

    XkbUpdateDescActions(xkb, first, num, changes);

    if ((pXDev->kbdfeed) &&
        (changes->ctrls.changed_ctrls & XkbPerKeyRepeatMask)) {
        /* now copy the modified changes back to core */
        memcpy(pXDev->kbdfeed->ctrl.autoRepeats, repeat, XkbPerKeyBitArraySize);
        if (pXDev->kbdfeed->CtrlProc)
            (*pXDev->kbdfeed->CtrlProc) (pXDev, &pXDev->kbdfeed->ctrl);
    }
    return;
}

KeySymsPtr
XkbGetCoreMap(DeviceIntPtr keybd)
{
    register int key, tmp;
    int maxSymsPerKey, maxGroup1Width;
    XkbDescPtr xkb;
    KeySymsPtr syms;
    int maxNumberOfGroups;

    if (!keybd || !keybd->key || !keybd->key->xkbInfo)
        return NULL;

    xkb = keybd->key->xkbInfo->desc;
    maxSymsPerKey = maxGroup1Width = 0;
    maxNumberOfGroups = 0;

    /* determine sizes */
    for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
        if (XkbKeycodeInRange(xkb, key)) {
            int nGroups;
            int w;

            nGroups = XkbKeyNumGroups(xkb, key);
            tmp = 0;
            if (nGroups > 0) {
                if ((w = XkbKeyGroupWidth(xkb, key, XkbGroup1Index)) <= 2)
                    tmp += 2;
                else
                    tmp += w + 2;
                /* remember highest G1 width */
                if (w > maxGroup1Width)
                    maxGroup1Width = w;
            }
            if (nGroups > 1) {
                if (tmp <= 2) {
                    if ((w = XkbKeyGroupWidth(xkb, key, XkbGroup2Index)) < 2)
                        tmp += 2;
                    else
                        tmp += w;
                }
                else {
                    if ((w = XkbKeyGroupWidth(xkb, key, XkbGroup2Index)) > 2)
                        tmp += w - 2;
                }
            }
            if (nGroups > 2)
                tmp += XkbKeyGroupWidth(xkb, key, XkbGroup3Index);
            if (nGroups > 3)
                tmp += XkbKeyGroupWidth(xkb, key, XkbGroup4Index);
            if (tmp > maxSymsPerKey)
                maxSymsPerKey = tmp;
            if (nGroups > maxNumberOfGroups)
                maxNumberOfGroups = nGroups;
        }
    }

    if (maxSymsPerKey <= 0)
        return NULL;

    syms = calloc(1, sizeof(*syms));
    if (!syms)
        return NULL;

    /* See Section 12.4 of the XKB Protocol spec. Because of the
     * single-group distribution for multi-group keyboards, we have to
     * have enough symbols for the largest group 1 to replicate across the
     * number of groups on the keyboard. e.g. a single-group key with 4
     * symbols on a keyboard that has 3 groups -> 12 syms per key */
    if (maxSymsPerKey < maxNumberOfGroups * maxGroup1Width)
        maxSymsPerKey = maxNumberOfGroups * maxGroup1Width;

    syms->mapWidth = maxSymsPerKey;
    syms->minKeyCode = xkb->min_key_code;
    syms->maxKeyCode = xkb->max_key_code;

    tmp = syms->mapWidth * (xkb->max_key_code - xkb->min_key_code + 1);
    syms->map = calloc(tmp, sizeof(*syms->map));
    if (!syms->map) {
        free(syms);
        return NULL;
    }

    for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
        KeySym *pCore, *pXKB;
        unsigned nGroups, groupWidth, n, nOut;

        nGroups = XkbKeyNumGroups(xkb, key);
        n = (key - xkb->min_key_code) * syms->mapWidth;
        pCore = &syms->map[n];
        pXKB = XkbKeySymsPtr(xkb, key);
        nOut = 2;
        if (nGroups > 0) {
            groupWidth = XkbKeyGroupWidth(xkb, key, XkbGroup1Index);
            if (groupWidth > 0)
                pCore[0] = pXKB[0];
            if (groupWidth > 1)
                pCore[1] = pXKB[1];
            for (n = 2; n < groupWidth; n++)
                pCore[2 + n] = pXKB[n];
            if (groupWidth > 2)
                nOut = groupWidth;
        }

        /* See XKB Protocol Sec, Section 12.4.
           A 1-group key with ABCDE on a 2 group keyboard must be
           duplicated across all groups as ABABCDECDE.
         */
        if (nGroups == 1) {
            int idx, j;

            groupWidth = XkbKeyGroupWidth(xkb, key, XkbGroup1Index);

            /* AB..CDE... -> ABABCDE... */
            if (groupWidth > 0 && syms->mapWidth >= 3)
                pCore[2] = pCore[0];
            if (groupWidth > 1 && syms->mapWidth >= 4)
                pCore[3] = pCore[1];

            /* ABABCDE... -> ABABCDECDE */
            idx = 2 + groupWidth;
            while (groupWidth > 2 && idx < syms->mapWidth &&
                   idx < groupWidth * 2) {
                pCore[idx] = pCore[idx - groupWidth + 2];
                idx++;
            }
            idx = 2 * groupWidth;
            if (idx < 4)
                idx = 4;
            /* 3 or more groups: ABABCDECDEABCDEABCDE */
            for (j = 3; j <= maxNumberOfGroups; j++)
                for (n = 0; n < groupWidth && idx < maxSymsPerKey; n++)
                    pCore[idx++] = pXKB[n];
        }

        pXKB += XkbKeyGroupsWidth(xkb, key);
        nOut += 2;
        if (nGroups > 1) {
            groupWidth = XkbKeyGroupWidth(xkb, key, XkbGroup2Index);
            if (groupWidth > 0)
                pCore[2] = pXKB[0];
            if (groupWidth > 1)
                pCore[3] = pXKB[1];
            for (n = 2; n < groupWidth; n++) {
                pCore[nOut + (n - 2)] = pXKB[n];
            }
            if (groupWidth > 2)
                nOut += (groupWidth - 2);
        }
        pXKB += XkbKeyGroupsWidth(xkb, key);
        for (n = XkbGroup3Index; n < nGroups; n++) {
            register int s;

            groupWidth = XkbKeyGroupWidth(xkb, key, n);
            for (s = 0; s < groupWidth; s++) {
                pCore[nOut++] = pXKB[s];
            }
            pXKB += XkbKeyGroupsWidth(xkb, key);
        }
    }

    return syms;
}

void
XkbSetRepeatKeys(DeviceIntPtr pXDev, int key, int onoff)
{
    if (pXDev && pXDev->key && pXDev->key->xkbInfo) {
        xkbControlsNotify cn;
        XkbControlsPtr ctrls = pXDev->key->xkbInfo->desc->ctrls;
        XkbControlsRec old;

        old = *ctrls;

        if (key == -1) {        /* global autorepeat setting changed */
            if (onoff)
                ctrls->enabled_ctrls |= XkbRepeatKeysMask;
            else
                ctrls->enabled_ctrls &= ~XkbRepeatKeysMask;
        }
        else if (pXDev->kbdfeed) {
            ctrls->per_key_repeat[key / 8] =
                pXDev->kbdfeed->ctrl.autoRepeats[key / 8];
        }

        if (XkbComputeControlsNotify(pXDev, &old, ctrls, &cn, TRUE))
            XkbSendControlsNotify(pXDev, &cn);
    }
    return;
}

/* Applies a change to a single device, does not traverse the device tree. */
void
XkbApplyMappingChange(DeviceIntPtr kbd, KeySymsPtr map, KeyCode first_key,
                      CARD8 num_keys, CARD8 *modmap, ClientPtr client)
{
    XkbDescPtr xkb = kbd->key->xkbInfo->desc;
    XkbEventCauseRec cause;
    XkbChangesRec changes;
    unsigned int check;

    memset(&changes, 0, sizeof(changes));
    memset(&cause, 0, sizeof(cause));

    if (map && first_key && num_keys) {
        check = 0;
        XkbSetCauseCoreReq(&cause, X_ChangeKeyboardMapping, client);

        XkbUpdateKeyTypesFromCore(kbd, map, first_key, num_keys, &changes);
        XkbUpdateActions(kbd, first_key, num_keys, &changes, &check, &cause);

        if (check)
            XkbCheckSecondaryEffects(kbd->key->xkbInfo, 1, &changes, &cause);
    }

    if (modmap) {
        /* A keymap change can imply a modmap change, se we prefer the
         * former. */
        if (!cause.mjr)
            XkbSetCauseCoreReq(&cause, X_SetModifierMapping, client);

        check = 0;
        num_keys = xkb->max_key_code - xkb->min_key_code + 1;
        changes.map.changed |= XkbModifierMapMask;
        changes.map.first_modmap_key = xkb->min_key_code;
        changes.map.num_modmap_keys = num_keys;
        memcpy(kbd->key->xkbInfo->desc->map->modmap, modmap, MAP_LENGTH);
        XkbUpdateActions(kbd, xkb->min_key_code, num_keys, &changes, &check,
                         &cause);

        if (check)
            XkbCheckSecondaryEffects(kbd->key->xkbInfo, 1, &changes, &cause);
    }

    XkbSendNotification(kbd, &changes, &cause);
}

void
XkbDisableComputedAutoRepeats(DeviceIntPtr dev, unsigned key)
{
    XkbSrvInfoPtr xkbi = dev->key->xkbInfo;
    xkbMapNotify mn;

    xkbi->desc->server->explicit[key] |= XkbExplicitAutoRepeatMask;
    memset(&mn, 0, sizeof(mn));
    mn.changed = XkbExplicitComponentsMask;
    mn.firstKeyExplicit = key;
    mn.nKeyExplicit = 1;
    XkbSendMapNotify(dev, &mn);
    return;
}

unsigned
XkbStateChangedFlags(XkbStatePtr old, XkbStatePtr new)
{
    int changed;

    changed = (old->group != new->group ? XkbGroupStateMask : 0);
    changed |= (old->base_group != new->base_group ? XkbGroupBaseMask : 0);
    changed |=
        (old->latched_group != new->latched_group ? XkbGroupLatchMask : 0);
    changed |= (old->locked_group != new->locked_group ? XkbGroupLockMask : 0);
    changed |= (old->mods != new->mods ? XkbModifierStateMask : 0);
    changed |= (old->base_mods != new->base_mods ? XkbModifierBaseMask : 0);
    changed |=
        (old->latched_mods != new->latched_mods ? XkbModifierLatchMask : 0);
    changed |= (old->locked_mods != new->locked_mods ? XkbModifierLockMask : 0);
    changed |=
        (old->compat_state != new->compat_state ? XkbCompatStateMask : 0);
    changed |= (old->grab_mods != new->grab_mods ? XkbGrabModsMask : 0);
    if (old->compat_grab_mods != new->compat_grab_mods)
        changed |= XkbCompatGrabModsMask;
    changed |= (old->lookup_mods != new->lookup_mods ? XkbLookupModsMask : 0);
    if (old->compat_lookup_mods != new->compat_lookup_mods)
        changed |= XkbCompatLookupModsMask;
    changed |=
        (old->ptr_buttons != new->ptr_buttons ? XkbPointerButtonMask : 0);
    return changed;
}

static void
XkbComputeCompatState(XkbSrvInfoPtr xkbi)
{
    CARD16 grp_mask;
    XkbStatePtr state = &xkbi->state;
    XkbCompatMapPtr map;
    XkbControlsPtr ctrls;

    if (!state || !xkbi->desc || !xkbi->desc->ctrls || !xkbi->desc->compat)
        return;

    map = xkbi->desc->compat;
    grp_mask = map->groups[state->group].mask;
    state->compat_state = state->mods | grp_mask;
    state->compat_lookup_mods = state->lookup_mods | grp_mask;
    ctrls= xkbi->desc->ctrls;

    if (ctrls->enabled_ctrls & XkbIgnoreGroupLockMask) {
	unsigned char grp = state->base_group+state->latched_group;
	if (grp >= ctrls->num_groups)
	    grp = XkbAdjustGroup(XkbCharToInt(grp), ctrls);
        grp_mask = map->groups[grp].mask;
    }
    state->compat_grab_mods = state->grab_mods | grp_mask;
    return;
}

unsigned
XkbAdjustGroup(int group, XkbControlsPtr ctrls)
{
    unsigned act;

    act = XkbOutOfRangeGroupAction(ctrls->groups_wrap);
    if (group < 0) {
        while (group < 0) {
            if (act == XkbClampIntoRange) {
                group = XkbGroup1Index;
            }
            else if (act == XkbRedirectIntoRange) {
                int newGroup;

                newGroup = XkbOutOfRangeGroupNumber(ctrls->groups_wrap);
                if (newGroup >= ctrls->num_groups)
                    group = XkbGroup1Index;
                else
                    group = newGroup;
            }
            else {
                group += ctrls->num_groups;
            }
        }
    }
    else if (group >= ctrls->num_groups) {
        if (act == XkbClampIntoRange) {
            group = ctrls->num_groups - 1;
        }
        else if (act == XkbRedirectIntoRange) {
            int newGroup;

            newGroup = XkbOutOfRangeGroupNumber(ctrls->groups_wrap);
            if (newGroup >= ctrls->num_groups)
                group = XkbGroup1Index;
            else
                group = newGroup;
        }
        else {
            group %= ctrls->num_groups;
        }
    }
    return group;
}

void
XkbComputeDerivedState(XkbSrvInfoPtr xkbi)
{
    XkbStatePtr state = &xkbi->state;
    XkbControlsPtr ctrls = xkbi->desc->ctrls;
    unsigned char grp;

    if (!state || !ctrls)
        return;

    state->mods = (state->base_mods | state->latched_mods | state->locked_mods);
    state->lookup_mods = state->mods & (~ctrls->internal.mask);
    state->grab_mods = state->lookup_mods & (~ctrls->ignore_lock.mask);
    state->grab_mods |=
        ((state->base_mods | state->latched_mods) & ctrls->ignore_lock.mask);

    grp = state->locked_group;
    if (grp >= ctrls->num_groups)
        state->locked_group = XkbAdjustGroup(XkbCharToInt(grp), ctrls);

    grp = state->locked_group + state->base_group + state->latched_group;
    if (grp >= ctrls->num_groups)
        state->group = XkbAdjustGroup(XkbCharToInt(grp), ctrls);
    else
        state->group = grp;
    XkbComputeCompatState(xkbi);
    return;
}

/***====================================================================***/

void
XkbCheckSecondaryEffects(XkbSrvInfoPtr xkbi,
                         unsigned which,
                         XkbChangesPtr changes, XkbEventCausePtr cause)
{
    if (which & XkbStateNotifyMask) {
        XkbStateRec old;

        old = xkbi->state;
        changes->state_changes |= XkbStateChangedFlags(&old, &xkbi->state);
        XkbComputeDerivedState(xkbi);
    }
    if (which & XkbIndicatorStateNotifyMask)
        XkbUpdateIndicators(xkbi->device, XkbAllIndicatorsMask, TRUE, changes,
                            cause);
    return;
}

/***====================================================================***/

Bool
XkbEnableDisableControls(XkbSrvInfoPtr xkbi,
                         unsigned long change,
                         unsigned long newValues,
                         XkbChangesPtr changes, XkbEventCausePtr cause)
{
    XkbControlsPtr ctrls;
    unsigned old;
    XkbSrvLedInfoPtr sli;

    ctrls = xkbi->desc->ctrls;
    old = ctrls->enabled_ctrls;
    ctrls->enabled_ctrls &= ~change;
    ctrls->enabled_ctrls |= (change & newValues);
    if (old == ctrls->enabled_ctrls)
        return FALSE;
    if (cause != NULL) {
        xkbControlsNotify cn;

        cn.numGroups = ctrls->num_groups;
        cn.changedControls = XkbControlsEnabledMask;
        cn.enabledControls = ctrls->enabled_ctrls;
        cn.enabledControlChanges = (ctrls->enabled_ctrls ^ old);
        cn.keycode = cause->kc;
        cn.eventType = cause->event;
        cn.requestMajor = cause->mjr;
        cn.requestMinor = cause->mnr;
        XkbSendControlsNotify(xkbi->device, &cn);
    }
    else {
        /* Yes, this really should be an XOR.  If ctrls->enabled_ctrls_changes */
        /* is non-zero, the controls in question changed already in "this" */
        /* request and this change merely undoes the previous one.  By the */
        /* same token, we have to figure out whether or not ControlsEnabled */
        /* should be set or not in the changes structure */
        changes->ctrls.enabled_ctrls_changes ^= (ctrls->enabled_ctrls ^ old);
        if (changes->ctrls.enabled_ctrls_changes)
            changes->ctrls.changed_ctrls |= XkbControlsEnabledMask;
        else
            changes->ctrls.changed_ctrls &= ~XkbControlsEnabledMask;
    }
    sli = XkbFindSrvLedInfo(xkbi->device, XkbDfltXIClass, XkbDfltXIId, 0);
    XkbUpdateIndicators(xkbi->device, sli->usesControls, TRUE, changes, cause);
    return TRUE;
}

/***====================================================================***/

#define	MAX_TOC	16

XkbGeometryPtr
XkbLookupNamedGeometry(DeviceIntPtr dev, Atom name, Bool *shouldFree)
{
    XkbSrvInfoPtr xkbi = dev->key->xkbInfo;
    XkbDescPtr xkb = xkbi->desc;

    *shouldFree = 0;
    if (name == None) {
        if (xkb->geom != NULL)
            return xkb->geom;
        name = xkb->names->geometry;
    }
    if ((xkb->geom != NULL) && (xkb->geom->name == name))
        return xkb->geom;
    *shouldFree = 1;
    return NULL;
}

void
XkbConvertCase(register KeySym sym, KeySym * lower, KeySym * upper)
{
    *lower = sym;
    *upper = sym;
    switch (sym >> 8) {
    case 0:                    /* Latin 1 */
        if ((sym >= XK_A) && (sym <= XK_Z))
            *lower += (XK_a - XK_A);
        else if ((sym >= XK_a) && (sym <= XK_z))
            *upper -= (XK_a - XK_A);
        else if ((sym >= XK_Agrave) && (sym <= XK_Odiaeresis))
            *lower += (XK_agrave - XK_Agrave);
        else if ((sym >= XK_agrave) && (sym <= XK_odiaeresis))
            *upper -= (XK_agrave - XK_Agrave);
        else if ((sym >= XK_Ooblique) && (sym <= XK_Thorn))
            *lower += (XK_oslash - XK_Ooblique);
        else if ((sym >= XK_oslash) && (sym <= XK_thorn))
            *upper -= (XK_oslash - XK_Ooblique);
        break;
    case 1:                    /* Latin 2 */
        /* Assume the KeySym is a legal value (ignore discontinuities) */
        if (sym == XK_Aogonek)
            *lower = XK_aogonek;
        else if (sym >= XK_Lstroke && sym <= XK_Sacute)
            *lower += (XK_lstroke - XK_Lstroke);
        else if (sym >= XK_Scaron && sym <= XK_Zacute)
            *lower += (XK_scaron - XK_Scaron);
        else if (sym >= XK_Zcaron && sym <= XK_Zabovedot)
            *lower += (XK_zcaron - XK_Zcaron);
        else if (sym == XK_aogonek)
            *upper = XK_Aogonek;
        else if (sym >= XK_lstroke && sym <= XK_sacute)
            *upper -= (XK_lstroke - XK_Lstroke);
        else if (sym >= XK_scaron && sym <= XK_zacute)
            *upper -= (XK_scaron - XK_Scaron);
        else if (sym >= XK_zcaron && sym <= XK_zabovedot)
            *upper -= (XK_zcaron - XK_Zcaron);
        else if (sym >= XK_Racute && sym <= XK_Tcedilla)
            *lower += (XK_racute - XK_Racute);
        else if (sym >= XK_racute && sym <= XK_tcedilla)
            *upper -= (XK_racute - XK_Racute);
        break;
    case 2:                    /* Latin 3 */
        /* Assume the KeySym is a legal value (ignore discontinuities) */
        if (sym >= XK_Hstroke && sym <= XK_Hcircumflex)
            *lower += (XK_hstroke - XK_Hstroke);
        else if (sym >= XK_Gbreve && sym <= XK_Jcircumflex)
            *lower += (XK_gbreve - XK_Gbreve);
        else if (sym >= XK_hstroke && sym <= XK_hcircumflex)
            *upper -= (XK_hstroke - XK_Hstroke);
        else if (sym >= XK_gbreve && sym <= XK_jcircumflex)
            *upper -= (XK_gbreve - XK_Gbreve);
        else if (sym >= XK_Cabovedot && sym <= XK_Scircumflex)
            *lower += (XK_cabovedot - XK_Cabovedot);
        else if (sym >= XK_cabovedot && sym <= XK_scircumflex)
            *upper -= (XK_cabovedot - XK_Cabovedot);
        break;
    case 3:                    /* Latin 4 */
        /* Assume the KeySym is a legal value (ignore discontinuities) */
        if (sym >= XK_Rcedilla && sym <= XK_Tslash)
            *lower += (XK_rcedilla - XK_Rcedilla);
        else if (sym >= XK_rcedilla && sym <= XK_tslash)
            *upper -= (XK_rcedilla - XK_Rcedilla);
        else if (sym == XK_ENG)
            *lower = XK_eng;
        else if (sym == XK_eng)
            *upper = XK_ENG;
        else if (sym >= XK_Amacron && sym <= XK_Umacron)
            *lower += (XK_amacron - XK_Amacron);
        else if (sym >= XK_amacron && sym <= XK_umacron)
            *upper -= (XK_amacron - XK_Amacron);
        break;
    case 6:                    /* Cyrillic */
        /* Assume the KeySym is a legal value (ignore discontinuities) */
        if (sym >= XK_Serbian_DJE && sym <= XK_Cyrillic_DZHE)
            *lower -= (XK_Serbian_DJE - XK_Serbian_dje);
        else if (sym >= XK_Serbian_dje && sym <= XK_Cyrillic_dzhe)
            *upper += (XK_Serbian_DJE - XK_Serbian_dje);
        else if (sym >= XK_Cyrillic_YU && sym <= XK_Cyrillic_HARDSIGN)
            *lower -= (XK_Cyrillic_YU - XK_Cyrillic_yu);
        else if (sym >= XK_Cyrillic_yu && sym <= XK_Cyrillic_hardsign)
            *upper += (XK_Cyrillic_YU - XK_Cyrillic_yu);
        break;
    case 7:                    /* Greek */
        /* Assume the KeySym is a legal value (ignore discontinuities) */
        if (sym >= XK_Greek_ALPHAaccent && sym <= XK_Greek_OMEGAaccent)
            *lower += (XK_Greek_alphaaccent - XK_Greek_ALPHAaccent);
        else if (sym >= XK_Greek_alphaaccent && sym <= XK_Greek_omegaaccent &&
                 sym != XK_Greek_iotaaccentdieresis &&
                 sym != XK_Greek_upsilonaccentdieresis)
            *upper -= (XK_Greek_alphaaccent - XK_Greek_ALPHAaccent);
        else if (sym >= XK_Greek_ALPHA && sym <= XK_Greek_OMEGA)
            *lower += (XK_Greek_alpha - XK_Greek_ALPHA);
        else if (sym >= XK_Greek_alpha && sym <= XK_Greek_omega &&
                 sym != XK_Greek_finalsmallsigma)
            *upper -= (XK_Greek_alpha - XK_Greek_ALPHA);
        break;
    }
}

static Bool
_XkbCopyClientMap(XkbDescPtr src, XkbDescPtr dst)
{
    void *tmp = NULL;
    int i;
    XkbKeyTypePtr stype = NULL, dtype = NULL;

    /* client map */
    if (src->map) {
        if (!dst->map) {
            tmp = calloc(1, sizeof(XkbClientMapRec));
            if (!tmp)
                return FALSE;
            dst->map = tmp;
        }

        if (src->map->syms) {
            if (src->map->size_syms != dst->map->size_syms) {
                tmp = reallocarray(dst->map->syms,
                                   src->map->size_syms, sizeof(KeySym));
                if (!tmp)
                    return FALSE;
                dst->map->syms = tmp;

            }
            memcpy(dst->map->syms, src->map->syms,
                   src->map->size_syms * sizeof(KeySym));
        }
        else {
            free(dst->map->syms);
            dst->map->syms = NULL;
        }
        dst->map->num_syms = src->map->num_syms;
        dst->map->size_syms = src->map->size_syms;

        if (src->map->key_sym_map) {
            if (src->max_key_code != dst->max_key_code) {
                tmp = reallocarray(dst->map->key_sym_map,
                                   src->max_key_code + 1, sizeof(XkbSymMapRec));
                if (!tmp)
                    return FALSE;
                dst->map->key_sym_map = tmp;
            }
            memcpy(dst->map->key_sym_map, src->map->key_sym_map,
                   (src->max_key_code + 1) * sizeof(XkbSymMapRec));
        }
        else {
            free(dst->map->key_sym_map);
            dst->map->key_sym_map = NULL;
        }

        if (src->map->types && src->map->num_types) {
            if (src->map->num_types > dst->map->size_types ||
                !dst->map->types || !dst->map->size_types) {
                if (dst->map->types && dst->map->size_types) {
                    tmp = reallocarray(dst->map->types, src->map->num_types,
                                       sizeof(XkbKeyTypeRec));
                    if (!tmp)
                        return FALSE;
                    dst->map->types = tmp;
                    memset(dst->map->types + dst->map->num_types, 0,
                           (src->map->num_types - dst->map->num_types) *
                           sizeof(XkbKeyTypeRec));
                }
                else {
                    tmp = calloc(src->map->num_types, sizeof(XkbKeyTypeRec));
                    if (!tmp)
                        return FALSE;
                    dst->map->types = tmp;
                }
            }
            else if (src->map->num_types < dst->map->num_types &&
                     dst->map->types) {
                for (i = src->map->num_types, dtype = (dst->map->types + i);
                     i < dst->map->num_types; i++, dtype++) {
                    free(dtype->level_names);
                    dtype->level_names = NULL;
                    dtype->num_levels = 0;
                    if (dtype->map_count) {
                        free(dtype->map);
                        free(dtype->preserve);
                    }
                }
            }

            stype = src->map->types;
            dtype = dst->map->types;
            for (i = 0; i < src->map->num_types; i++, dtype++, stype++) {
                if (stype->num_levels && stype->level_names) {
                    if (stype->num_levels != dtype->num_levels &&
                        dtype->num_levels && dtype->level_names &&
                        i < dst->map->num_types) {
                        tmp = reallocarray(dtype->level_names,
                                           stype->num_levels, sizeof(Atom));
                        if (!tmp)
                            continue;
                        dtype->level_names = tmp;
                    }
                    else if (!dtype->num_levels || !dtype->level_names ||
                             i >= dst->map->num_types) {
                        tmp = malloc(stype->num_levels * sizeof(Atom));
                        if (!tmp)
                            continue;
                        dtype->level_names = tmp;
                    }
                    dtype->num_levels = stype->num_levels;
                    memcpy(dtype->level_names, stype->level_names,
                           stype->num_levels * sizeof(Atom));
                }
                else {
                    if (dtype->num_levels && dtype->level_names &&
                        i < dst->map->num_types)
                        free(dtype->level_names);
                    dtype->num_levels = 0;
                    dtype->level_names = NULL;
                }

                dtype->name = stype->name;
                memcpy(&dtype->mods, &stype->mods, sizeof(XkbModsRec));

                if (stype->map_count) {
                    if (stype->map) {
                        if (stype->map_count != dtype->map_count &&
                            dtype->map_count && dtype->map &&
                            i < dst->map->num_types) {
                            tmp = reallocarray(dtype->map,
                                               stype->map_count,
                                               sizeof(XkbKTMapEntryRec));
                            if (!tmp)
                                return FALSE;
                            dtype->map = tmp;
                        }
                        else if (!dtype->map_count || !dtype->map ||
                                 i >= dst->map->num_types) {
                            tmp = xallocarray(stype->map_count,
                                              sizeof(XkbKTMapEntryRec));
                            if (!tmp)
                                return FALSE;
                            dtype->map = tmp;
                        }

                        memcpy(dtype->map, stype->map,
                               stype->map_count * sizeof(XkbKTMapEntryRec));
                    }
                    else {
                        if (dtype->map && i < dst->map->num_types)
                            free(dtype->map);
                        dtype->map = NULL;
                    }

                    if (stype->preserve) {
                        if (stype->map_count != dtype->map_count &&
                            dtype->map_count && dtype->preserve &&
                            i < dst->map->num_types) {
                            tmp = reallocarray(dtype->preserve,
                                               stype->map_count,
                                               sizeof(XkbModsRec));
                            if (!tmp)
                                return FALSE;
                            dtype->preserve = tmp;
                        }
                        else if (!dtype->preserve || !dtype->map_count ||
                                 i >= dst->map->num_types) {
                            tmp = xallocarray(stype->map_count,
                                              sizeof(XkbModsRec));
                            if (!tmp)
                                return FALSE;
                            dtype->preserve = tmp;
                        }

                        memcpy(dtype->preserve, stype->preserve,
                               stype->map_count * sizeof(XkbModsRec));
                    }
                    else {
                        if (dtype->preserve && i < dst->map->num_types)
                            free(dtype->preserve);
                        dtype->preserve = NULL;
                    }

                    dtype->map_count = stype->map_count;
                }
                else {
                    if (dtype->map_count && i < dst->map->num_types) {
                        free(dtype->map);
                        free(dtype->preserve);
                    }
                    dtype->map_count = 0;
                    dtype->map = NULL;
                    dtype->preserve = NULL;
                }
            }

            dst->map->size_types = src->map->num_types;
            dst->map->num_types = src->map->num_types;
        }
        else {
            if (dst->map->types) {
                for (i = 0, dtype = dst->map->types; i < dst->map->num_types;
                     i++, dtype++) {
                    free(dtype->level_names);
                    if (dtype->map && dtype->map_count)
                        free(dtype->map);
                    if (dtype->preserve && dtype->map_count)
                        free(dtype->preserve);
                }
            }
            free(dst->map->types);
            dst->map->types = NULL;
            dst->map->num_types = 0;
            dst->map->size_types = 0;
        }

        if (src->map->modmap) {
            if (src->max_key_code != dst->max_key_code) {
                tmp = realloc(dst->map->modmap, src->max_key_code + 1);
                if (!tmp)
                    return FALSE;
                dst->map->modmap = tmp;
            }
            memcpy(dst->map->modmap, src->map->modmap, src->max_key_code + 1);
        }
        else {
            free(dst->map->modmap);
            dst->map->modmap = NULL;
        }
    }
    else {
        if (dst->map)
            XkbFreeClientMap(dst, XkbAllClientInfoMask, TRUE);
    }

    return TRUE;
}

static Bool
_XkbCopyServerMap(XkbDescPtr src, XkbDescPtr dst)
{
    void *tmp = NULL;

    /* server map */
    if (src->server) {
        if (!dst->server) {
            tmp = calloc(1, sizeof(XkbServerMapRec));
            if (!tmp)
                return FALSE;
            dst->server = tmp;
        }

        if (src->server->explicit) {
            if (src->max_key_code != dst->max_key_code) {
                tmp = realloc(dst->server->explicit, src->max_key_code + 1);
                if (!tmp)
                    return FALSE;
                dst->server->explicit = tmp;
            }
            memcpy(dst->server->explicit, src->server->explicit,
                   src->max_key_code + 1);
        }
        else {
            free(dst->server->explicit);
            dst->server->explicit = NULL;
        }

        if (src->server->acts) {
            if (src->server->size_acts != dst->server->size_acts) {
                tmp = reallocarray(dst->server->acts,
                                   src->server->size_acts, sizeof(XkbAction));
                if (!tmp)
                    return FALSE;
                dst->server->acts = tmp;
            }
            memcpy(dst->server->acts, src->server->acts,
                   src->server->size_acts * sizeof(XkbAction));
        }
        else {
            free(dst->server->acts);
            dst->server->acts = NULL;
        }
        dst->server->size_acts = src->server->size_acts;
        dst->server->num_acts = src->server->num_acts;

        if (src->server->key_acts) {
            if (src->max_key_code != dst->max_key_code) {
                tmp = reallocarray(dst->server->key_acts,
                                   src->max_key_code + 1, sizeof(unsigned short));
                if (!tmp)
                    return FALSE;
                dst->server->key_acts = tmp;
            }
            memcpy(dst->server->key_acts, src->server->key_acts,
                   (src->max_key_code + 1) * sizeof(unsigned short));
        }
        else {
            free(dst->server->key_acts);
            dst->server->key_acts = NULL;
        }

        if (src->server->behaviors) {
            if (src->max_key_code != dst->max_key_code) {
                tmp = reallocarray(dst->server->behaviors,
                                   src->max_key_code + 1, sizeof(XkbBehavior));
                if (!tmp)
                    return FALSE;
                dst->server->behaviors = tmp;
            }
            memcpy(dst->server->behaviors, src->server->behaviors,
                   (src->max_key_code + 1) * sizeof(XkbBehavior));
        }
        else {
            free(dst->server->behaviors);
            dst->server->behaviors = NULL;
        }

        memcpy(dst->server->vmods, src->server->vmods, XkbNumVirtualMods);

        if (src->server->vmodmap) {
            if (src->max_key_code != dst->max_key_code) {
                tmp = reallocarray(dst->server->vmodmap,
                                   src->max_key_code + 1, sizeof(unsigned short));
                if (!tmp)
                    return FALSE;
                dst->server->vmodmap = tmp;
            }
            memcpy(dst->server->vmodmap, src->server->vmodmap,
                   (src->max_key_code + 1) * sizeof(unsigned short));
        }
        else {
            free(dst->server->vmodmap);
            dst->server->vmodmap = NULL;
        }
    }
    else {
        if (dst->server)
            XkbFreeServerMap(dst, XkbAllServerInfoMask, TRUE);
    }

    return TRUE;
}

static Bool
_XkbCopyNames(XkbDescPtr src, XkbDescPtr dst)
{
    void *tmp = NULL;

    /* names */
    if (src->names) {
        if (!dst->names) {
            dst->names = calloc(1, sizeof(XkbNamesRec));
            if (!dst->names)
                return FALSE;
        }

        if (src->names->keys) {
            if (src->max_key_code != dst->max_key_code) {
                tmp = reallocarray(dst->names->keys, src->max_key_code + 1,
                                   sizeof(XkbKeyNameRec));
                if (!tmp)
                    return FALSE;
                dst->names->keys = tmp;
            }
            memcpy(dst->names->keys, src->names->keys,
                   (src->max_key_code + 1) * sizeof(XkbKeyNameRec));
        }
        else {
            free(dst->names->keys);
            dst->names->keys = NULL;
        }

        if (src->names->num_key_aliases) {
            if (src->names->num_key_aliases != dst->names->num_key_aliases) {
                tmp = reallocarray(dst->names->key_aliases,
                                   src->names->num_key_aliases,
                                   sizeof(XkbKeyAliasRec));
                if (!tmp)
                    return FALSE;
                dst->names->key_aliases = tmp;
            }
            memcpy(dst->names->key_aliases, src->names->key_aliases,
                   src->names->num_key_aliases * sizeof(XkbKeyAliasRec));
        }
        else {
            free(dst->names->key_aliases);
            dst->names->key_aliases = NULL;
        }
        dst->names->num_key_aliases = src->names->num_key_aliases;

        if (src->names->num_rg) {
            if (src->names->num_rg != dst->names->num_rg) {
                tmp = reallocarray(dst->names->radio_groups,
                                   src->names->num_rg, sizeof(Atom));
                if (!tmp)
                    return FALSE;
                dst->names->radio_groups = tmp;
            }
            memcpy(dst->names->radio_groups, src->names->radio_groups,
                   src->names->num_rg * sizeof(Atom));
        }
        else {
            free(dst->names->radio_groups);
            dst->names->radio_groups = NULL;
        }
        dst->names->num_rg = src->names->num_rg;

        dst->names->keycodes = src->names->keycodes;
        dst->names->geometry = src->names->geometry;
        dst->names->symbols = src->names->symbols;
        dst->names->types = src->names->types;
        dst->names->compat = src->names->compat;
        dst->names->phys_symbols = src->names->phys_symbols;

        memcpy(dst->names->vmods, src->names->vmods,
               XkbNumVirtualMods * sizeof(Atom));
        memcpy(dst->names->indicators, src->names->indicators,
               XkbNumIndicators * sizeof(Atom));
        memcpy(dst->names->groups, src->names->groups,
               XkbNumKbdGroups * sizeof(Atom));
    }
    else {
        if (dst->names)
            XkbFreeNames(dst, XkbAllNamesMask, TRUE);
    }

    return TRUE;
}

static Bool
_XkbCopyCompat(XkbDescPtr src, XkbDescPtr dst)
{
    void *tmp = NULL;

    /* compat */
    if (src->compat) {
        if (!dst->compat) {
            dst->compat = calloc(1, sizeof(XkbCompatMapRec));
            if (!dst->compat)
                return FALSE;
        }

        if (src->compat->sym_interpret && src->compat->num_si) {
            if (src->compat->num_si != dst->compat->size_si) {
                tmp = reallocarray(dst->compat->sym_interpret,
                                   src->compat->num_si,
                                   sizeof(XkbSymInterpretRec));
                if (!tmp)
                    return FALSE;
                dst->compat->sym_interpret = tmp;
            }
            memcpy(dst->compat->sym_interpret, src->compat->sym_interpret,
                   src->compat->num_si * sizeof(XkbSymInterpretRec));

            dst->compat->num_si = src->compat->num_si;
            dst->compat->size_si = src->compat->num_si;
        }
        else {
            if (dst->compat->sym_interpret && dst->compat->size_si)
                free(dst->compat->sym_interpret);

            dst->compat->sym_interpret = NULL;
            dst->compat->num_si = 0;
            dst->compat->size_si = 0;
        }

        memcpy(dst->compat->groups, src->compat->groups,
               XkbNumKbdGroups * sizeof(XkbModsRec));
    }
    else {
        if (dst->compat)
            XkbFreeCompatMap(dst, XkbAllCompatMask, TRUE);
    }

    return TRUE;
}

static Bool
_XkbCopyGeom(XkbDescPtr src, XkbDescPtr dst)
{
    void *tmp = NULL;
    int i = 0, j = 0, k = 0;
    XkbColorPtr scolor = NULL, dcolor = NULL;
    XkbDoodadPtr sdoodad = NULL, ddoodad = NULL;
    XkbOutlinePtr soutline = NULL, doutline = NULL;
    XkbPropertyPtr sprop = NULL, dprop = NULL;
    XkbRowPtr srow = NULL, drow = NULL;
    XkbSectionPtr ssection = NULL, dsection = NULL;
    XkbShapePtr sshape = NULL, dshape = NULL;

    /* geometry */
    if (src->geom) {
        if (!dst->geom) {
            dst->geom = calloc(sizeof(XkbGeometryRec), 1);
            if (!dst->geom)
                return FALSE;
        }

        /* properties */
        if (src->geom->num_properties) {
            /* If we've got more properties in the destination than
             * the source, run through and free all the excess ones
             * first. */
            if (src->geom->num_properties < dst->geom->sz_properties) {
                for (i = src->geom->num_properties, dprop =
                     dst->geom->properties + i; i < dst->geom->num_properties;
                     i++, dprop++) {
                    free(dprop->name);
                    free(dprop->value);
                }
            }

            /* Reallocate and clear all new items if the buffer grows. */
            if (!XkbGeomRealloc
                ((void **) &dst->geom->properties, dst->geom->sz_properties,
                 src->geom->num_properties, sizeof(XkbPropertyRec),
                 XKB_GEOM_CLEAR_EXCESS))
                return FALSE;
            /* We don't set num_properties as we need it to try and avoid
             * too much reallocing. */
            dst->geom->sz_properties = src->geom->num_properties;

            for (i = 0,
                 sprop = src->geom->properties,
                 dprop = dst->geom->properties;
                 i < src->geom->num_properties; i++, sprop++, dprop++) {
                if (i < dst->geom->num_properties) {
                    if (strlen(sprop->name) != strlen(dprop->name)) {
                        tmp = realloc(dprop->name, strlen(sprop->name) + 1);
                        if (!tmp)
                            return FALSE;
                        dprop->name = tmp;
                    }
                    if (strlen(sprop->value) != strlen(dprop->value)) {
                        tmp = realloc(dprop->value, strlen(sprop->value) + 1);
                        if (!tmp)
                            return FALSE;
                        dprop->value = tmp;
                    }
                    strcpy(dprop->name, sprop->name);
                    strcpy(dprop->value, sprop->value);
                }
                else {
                    dprop->name = xstrdup(sprop->name);
                    dprop->value = xstrdup(sprop->value);
                }
            }

            /* ... which is already src->geom->num_properties. */
            dst->geom->num_properties = dst->geom->sz_properties;
        }
        else {
            if (dst->geom->sz_properties) {
                for (i = 0, dprop = dst->geom->properties;
                     i < dst->geom->num_properties; i++, dprop++) {
                    free(dprop->name);
                    free(dprop->value);
                }
                free(dst->geom->properties);
                dst->geom->properties = NULL;
            }

            dst->geom->num_properties = 0;
            dst->geom->sz_properties = 0;
        }

        /* colors */
        if (src->geom->num_colors) {
            if (src->geom->num_colors < dst->geom->sz_colors) {
                for (i = src->geom->num_colors, dcolor = dst->geom->colors + i;
                     i < dst->geom->num_colors; i++, dcolor++) {
                    free(dcolor->spec);
                }
            }

            /* Reallocate and clear all new items if the buffer grows. */
            if (!XkbGeomRealloc
                ((void **) &dst->geom->colors, dst->geom->sz_colors,
                 src->geom->num_colors, sizeof(XkbColorRec),
                 XKB_GEOM_CLEAR_EXCESS))
                return FALSE;
            dst->geom->sz_colors = src->geom->num_colors;

            for (i = 0,
                 scolor = src->geom->colors,
                 dcolor = dst->geom->colors;
                 i < src->geom->num_colors; i++, scolor++, dcolor++) {
                if (i < dst->geom->num_colors) {
                    if (strlen(scolor->spec) != strlen(dcolor->spec)) {
                        tmp = realloc(dcolor->spec, strlen(scolor->spec) + 1);
                        if (!tmp)
                            return FALSE;
                        dcolor->spec = tmp;
                    }
                    strcpy(dcolor->spec, scolor->spec);
                }
                else {
                    dcolor->spec = xstrdup(scolor->spec);
                }
                dcolor->pixel = scolor->pixel;
            }

            dst->geom->num_colors = dst->geom->sz_colors;
        }
        else {
            if (dst->geom->sz_colors) {
                for (i = 0, dcolor = dst->geom->colors;
                     i < dst->geom->num_colors; i++, dcolor++) {
                    free(dcolor->spec);
                }
                free(dst->geom->colors);
                dst->geom->colors = NULL;
            }

            dst->geom->num_colors = 0;
            dst->geom->sz_colors = 0;
        }

        /* shapes */
        /* shapes break down into outlines, which break down into points. */
        if (dst->geom->num_shapes) {
            for (i = 0, dshape = dst->geom->shapes;
                 i < dst->geom->num_shapes; i++, dshape++) {
                for (j = 0, doutline = dshape->outlines;
                     j < dshape->num_outlines; j++, doutline++) {
                    if (doutline->sz_points)
                        free(doutline->points);
                }

                if (dshape->sz_outlines) {
                    free(dshape->outlines);
                    dshape->outlines = NULL;
                }

                dshape->num_outlines = 0;
                dshape->sz_outlines = 0;
            }
        }

        if (src->geom->num_shapes) {
            /* Reallocate and clear all items. */
            if (!XkbGeomRealloc
                ((void **) &dst->geom->shapes, dst->geom->sz_shapes,
                 src->geom->num_shapes, sizeof(XkbShapeRec),
                 XKB_GEOM_CLEAR_ALL))
                return FALSE;

            for (i = 0, sshape = src->geom->shapes, dshape = dst->geom->shapes;
                 i < src->geom->num_shapes; i++, sshape++, dshape++) {
                if (sshape->num_outlines) {
                    tmp = calloc(sshape->num_outlines, sizeof(XkbOutlineRec));
                    if (!tmp)
                        return FALSE;
                    dshape->outlines = tmp;

                    for (j = 0,
                         soutline = sshape->outlines,
                         doutline = dshape->outlines;
                         j < sshape->num_outlines;
                         j++, soutline++, doutline++) {
                        if (soutline->num_points) {
                            tmp = xallocarray(soutline->num_points,
                                              sizeof(XkbPointRec));
                            if (!tmp)
                                return FALSE;
                            doutline->points = tmp;

                            memcpy(doutline->points, soutline->points,
                                   soutline->num_points * sizeof(XkbPointRec));

                            doutline->corner_radius = soutline->corner_radius;
                        }

                        doutline->num_points = soutline->num_points;
                        doutline->sz_points = soutline->num_points;
                    }
                }

                dshape->num_outlines = sshape->num_outlines;
                dshape->sz_outlines = sshape->num_outlines;
                dshape->name = sshape->name;
                dshape->bounds = sshape->bounds;

                dshape->approx = NULL;
                if (sshape->approx && sshape->num_outlines > 0) {

                    const ptrdiff_t approx_idx =
                        sshape->approx - sshape->outlines;

                    if (approx_idx < dshape->num_outlines) {
                        dshape->approx = dshape->outlines + approx_idx;
                    }
                    else {
                        LogMessage(X_WARNING, "XKB: approx outline "
                                   "index is out of range\n");
                    }
                }

                dshape->primary = NULL;
                if (sshape->primary && sshape->num_outlines > 0) {

                    const ptrdiff_t primary_idx =
                        sshape->primary - sshape->outlines;

                    if (primary_idx < dshape->num_outlines) {
                        dshape->primary = dshape->outlines + primary_idx;
                    }
                    else {
                        LogMessage(X_WARNING, "XKB: primary outline "
                                   "index is out of range\n");
                    }
                }
            }

            dst->geom->num_shapes = src->geom->num_shapes;
            dst->geom->sz_shapes = src->geom->num_shapes;
        }
        else {
            if (dst->geom->sz_shapes) {
                free(dst->geom->shapes);
            }
            dst->geom->shapes = NULL;
            dst->geom->num_shapes = 0;
            dst->geom->sz_shapes = 0;
        }

        /* sections */
        /* sections break down into doodads, and also into rows, which break
         * down into keys. */
        if (dst->geom->num_sections) {
            for (i = 0, dsection = dst->geom->sections;
                 i < dst->geom->num_sections; i++, dsection++) {
                for (j = 0, drow = dsection->rows;
                     j < dsection->num_rows; j++, drow++) {
                    if (drow->num_keys)
                        free(drow->keys);
                }

                if (dsection->num_rows)
                    free(dsection->rows);

                /* cut and waste from geom/doodad below. */
                for (j = 0, ddoodad = dsection->doodads;
                     j < dsection->num_doodads; j++, ddoodad++) {
                    if (ddoodad->any.type == XkbTextDoodad) {
                        free(ddoodad->text.text);
                        ddoodad->text.text = NULL;
                        free(ddoodad->text.font);
                        ddoodad->text.font = NULL;
                    }
                    else if (ddoodad->any.type == XkbLogoDoodad) {
                        free(ddoodad->logo.logo_name);
                        ddoodad->logo.logo_name = NULL;
                    }
                }

                free(dsection->doodads);
            }

            dst->geom->num_sections = 0;
        }

        if (src->geom->num_sections) {
            /* Reallocate and clear all items. */
            if (!XkbGeomRealloc
                ((void **) &dst->geom->sections, dst->geom->sz_sections,
                 src->geom->num_sections, sizeof(XkbSectionRec),
                 XKB_GEOM_CLEAR_ALL))
                return FALSE;
            dst->geom->num_sections = src->geom->num_sections;
            dst->geom->sz_sections = src->geom->num_sections;

            for (i = 0,
                 ssection = src->geom->sections,
                 dsection = dst->geom->sections;
                 i < src->geom->num_sections; i++, ssection++, dsection++) {
                *dsection = *ssection;
                if (ssection->num_rows) {
                    tmp = calloc(ssection->num_rows, sizeof(XkbRowRec));
                    if (!tmp)
                        return FALSE;
                    dsection->rows = tmp;
                }
                dsection->num_rows = ssection->num_rows;
                dsection->sz_rows = ssection->num_rows;

                for (j = 0, srow = ssection->rows, drow = dsection->rows;
                     j < ssection->num_rows; j++, srow++, drow++) {
                    if (srow->num_keys) {
                        tmp = xallocarray(srow->num_keys, sizeof(XkbKeyRec));
                        if (!tmp)
                            return FALSE;
                        drow->keys = tmp;
                        memcpy(drow->keys, srow->keys,
                               srow->num_keys * sizeof(XkbKeyRec));
                    }
                    drow->num_keys = srow->num_keys;
                    drow->sz_keys = srow->num_keys;
                    drow->top = srow->top;
                    drow->left = srow->left;
                    drow->vertical = srow->vertical;
                    drow->bounds = srow->bounds;
                }

                if (ssection->num_doodads) {
                    tmp = calloc(ssection->num_doodads, sizeof(XkbDoodadRec));
                    if (!tmp)
                        return FALSE;
                    dsection->doodads = tmp;
                }
                else {
                    dsection->doodads = NULL;
                }

                dsection->sz_doodads = ssection->num_doodads;
                for (k = 0,
                     sdoodad = ssection->doodads,
                     ddoodad = dsection->doodads;
                     k < ssection->num_doodads; k++, sdoodad++, ddoodad++) {
                    memcpy(ddoodad, sdoodad, sizeof(XkbDoodadRec));
                    if (sdoodad->any.type == XkbTextDoodad) {
                        if (sdoodad->text.text)
                            ddoodad->text.text = strdup(sdoodad->text.text);
                        if (sdoodad->text.font)
                            ddoodad->text.font = strdup(sdoodad->text.font);
                    }
                    else if (sdoodad->any.type == XkbLogoDoodad) {
                        if (sdoodad->logo.logo_name)
                            ddoodad->logo.logo_name =
                                strdup(sdoodad->logo.logo_name);
                    }
                }
                dsection->overlays = NULL;
                dsection->sz_overlays = 0;
                dsection->num_overlays = 0;
            }
        }
        else {
            if (dst->geom->sz_sections) {
                free(dst->geom->sections);
            }

            dst->geom->sections = NULL;
            dst->geom->num_sections = 0;
            dst->geom->sz_sections = 0;
        }

        /* doodads */
        if (dst->geom->num_doodads) {
            for (i = src->geom->num_doodads,
                 ddoodad = dst->geom->doodads +
                 src->geom->num_doodads;
                 i < dst->geom->num_doodads; i++, ddoodad++) {
                if (ddoodad->any.type == XkbTextDoodad) {
                    free(ddoodad->text.text);
                    ddoodad->text.text = NULL;
                    free(ddoodad->text.font);
                    ddoodad->text.font = NULL;
                }
                else if (ddoodad->any.type == XkbLogoDoodad) {
                    free(ddoodad->logo.logo_name);
                    ddoodad->logo.logo_name = NULL;
                }
            }
            dst->geom->num_doodads = 0;
        }

        if (src->geom->num_doodads) {
            /* Reallocate and clear all items. */
            if (!XkbGeomRealloc
                ((void **) &dst->geom->doodads, dst->geom->sz_doodads,
                 src->geom->num_doodads, sizeof(XkbDoodadRec),
                 XKB_GEOM_CLEAR_ALL))
                return FALSE;

            dst->geom->sz_doodads = src->geom->num_doodads;

            for (i = 0,
                 sdoodad = src->geom->doodads,
                 ddoodad = dst->geom->doodads;
                 i < src->geom->num_doodads; i++, sdoodad++, ddoodad++) {
                memcpy(ddoodad, sdoodad, sizeof(XkbDoodadRec));
                if (sdoodad->any.type == XkbTextDoodad) {
                    if (sdoodad->text.text)
                        ddoodad->text.text = strdup(sdoodad->text.text);
                    if (sdoodad->text.font)
                        ddoodad->text.font = strdup(sdoodad->text.font);
                }
                else if (sdoodad->any.type == XkbLogoDoodad) {
                    if (sdoodad->logo.logo_name)
                        ddoodad->logo.logo_name =
                            strdup(sdoodad->logo.logo_name);
                }
            }

            dst->geom->num_doodads = dst->geom->sz_doodads;
        }
        else {
            if (dst->geom->sz_doodads) {
                free(dst->geom->doodads);
            }

            dst->geom->doodads = NULL;
            dst->geom->num_doodads = 0;
            dst->geom->sz_doodads = 0;
        }

        /* key aliases */
        if (src->geom->num_key_aliases) {
            /* Reallocate but don't clear any items. There is no need
             * to clear anything because data is immediately copied
             * over the whole memory area with memcpy. */
            if (!XkbGeomRealloc
                ((void **) &dst->geom->key_aliases, dst->geom->sz_key_aliases,
                 src->geom->num_key_aliases, 2 * XkbKeyNameLength,
                 XKB_GEOM_CLEAR_NONE))
                return FALSE;

            dst->geom->sz_key_aliases = src->geom->num_key_aliases;

            memcpy(dst->geom->key_aliases, src->geom->key_aliases,
                   src->geom->num_key_aliases * 2 * XkbKeyNameLength);

            dst->geom->num_key_aliases = dst->geom->sz_key_aliases;
        }
        else {
            free(dst->geom->key_aliases);
            dst->geom->key_aliases = NULL;
            dst->geom->num_key_aliases = 0;
            dst->geom->sz_key_aliases = 0;
        }

        /* font */
        if (src->geom->label_font) {
            if (!dst->geom->label_font) {
                tmp = malloc(strlen(src->geom->label_font) + 1);
                if (!tmp)
                    return FALSE;
                dst->geom->label_font = tmp;
            }
            else if (strlen(src->geom->label_font) !=
                     strlen(dst->geom->label_font)) {
                tmp = realloc(dst->geom->label_font,
                              strlen(src->geom->label_font) + 1);
                if (!tmp)
                    return FALSE;
                dst->geom->label_font = tmp;
            }

            strcpy(dst->geom->label_font, src->geom->label_font);
            i = XkbGeomColorIndex(src->geom, src->geom->label_color);
            dst->geom->label_color = &(dst->geom->colors[i]);
            i = XkbGeomColorIndex(src->geom, src->geom->base_color);
            dst->geom->base_color = &(dst->geom->colors[i]);
        }
        else {
            free(dst->geom->label_font);
            dst->geom->label_font = NULL;
            dst->geom->label_color = NULL;
            dst->geom->base_color = NULL;
        }

        dst->geom->name = src->geom->name;
        dst->geom->width_mm = src->geom->width_mm;
        dst->geom->height_mm = src->geom->height_mm;
    }
    else {
        if (dst->geom) {
            /* I LOVE THE DIFFERENT CALL SIGNATURE.  REALLY, I DO. */
            XkbFreeGeometry(dst->geom, XkbGeomAllMask, TRUE);
            dst->geom = NULL;
        }
    }

    return TRUE;
}

static Bool
_XkbCopyIndicators(XkbDescPtr src, XkbDescPtr dst)
{
    /* indicators */
    if (src->indicators) {
        if (!dst->indicators) {
            dst->indicators = malloc(sizeof(XkbIndicatorRec));
            if (!dst->indicators)
                return FALSE;
        }
        memcpy(dst->indicators, src->indicators, sizeof(XkbIndicatorRec));
    }
    else {
        free(dst->indicators);
        dst->indicators = NULL;
    }
    return TRUE;
}

static Bool
_XkbCopyControls(XkbDescPtr src, XkbDescPtr dst)
{
    /* controls */
    if (src->ctrls) {
        if (!dst->ctrls) {
            dst->ctrls = malloc(sizeof(XkbControlsRec));
            if (!dst->ctrls)
                return FALSE;
        }
        memcpy(dst->ctrls, src->ctrls, sizeof(XkbControlsRec));
    }
    else {
        free(dst->ctrls);
        dst->ctrls = NULL;
    }
    return TRUE;
}

/**
 * Copy an XKB map from src to dst, reallocating when necessary: if some
 * map components are present in one, but not in the other, the destination
 * components will be allocated or freed as necessary.
 *
 * Basic map consistency is assumed on both sides, so maps with random
 * uninitialised data (e.g. names->radio_grous == NULL, names->num_rg == 19)
 * _will_ cause failures.  You've been warned.
 *
 * Returns TRUE on success, or FALSE on failure.  If this function fails,
 * dst may be in an inconsistent state: all its pointers are guaranteed
 * to remain valid, but part of the map may be from src and part from dst.
 *
 */

Bool
XkbCopyKeymap(XkbDescPtr dst, XkbDescPtr src)
{

    if (!src || !dst) {
        DebugF("XkbCopyKeymap: src (%p) or dst (%p) is NULL\n", src, dst);
        return FALSE;
    }

    if (src == dst)
        return TRUE;

    if (!_XkbCopyClientMap(src, dst)) {
        DebugF("XkbCopyKeymap: failed to copy client map\n");
        return FALSE;
    }
    if (!_XkbCopyServerMap(src, dst)) {
        DebugF("XkbCopyKeymap: failed to copy server map\n");
        return FALSE;
    }
    if (!_XkbCopyIndicators(src, dst)) {
        DebugF("XkbCopyKeymap: failed to copy indicators\n");
        return FALSE;
    }
    if (!_XkbCopyControls(src, dst)) {
        DebugF("XkbCopyKeymap: failed to copy controls\n");
        return FALSE;
    }
    if (!_XkbCopyNames(src, dst)) {
        DebugF("XkbCopyKeymap: failed to copy names\n");
        return FALSE;
    }
    if (!_XkbCopyCompat(src, dst)) {
        DebugF("XkbCopyKeymap: failed to copy compat map\n");
        return FALSE;
    }
    if (!_XkbCopyGeom(src, dst)) {
        DebugF("XkbCopyKeymap: failed to copy geometry\n");
        return FALSE;
    }

    dst->min_key_code = src->min_key_code;
    dst->max_key_code = src->max_key_code;

    return TRUE;
}

Bool
XkbDeviceApplyKeymap(DeviceIntPtr dst, XkbDescPtr desc)
{
    xkbNewKeyboardNotify nkn;
    Bool ret;

    if (!dst->key || !desc)
        return FALSE;

    memset(&nkn, 0, sizeof(xkbNewKeyboardNotify));
    nkn.oldMinKeyCode = dst->key->xkbInfo->desc->min_key_code;
    nkn.oldMaxKeyCode = dst->key->xkbInfo->desc->max_key_code;
    nkn.deviceID = dst->id;
    nkn.oldDeviceID = dst->id;
    nkn.minKeyCode = desc->min_key_code;
    nkn.maxKeyCode = desc->max_key_code;
    nkn.requestMajor = XkbReqCode;
    nkn.requestMinor = X_kbSetMap;      /* Near enough's good enough. */
    nkn.changed = XkbNKN_KeycodesMask;
    if (desc->geom)
        nkn.changed |= XkbNKN_GeometryMask;

    ret = XkbCopyKeymap(dst->key->xkbInfo->desc, desc);
    if (ret)
        XkbSendNewKeyboardNotify(dst, &nkn);

    return ret;
}

Bool
XkbCopyDeviceKeymap(DeviceIntPtr dst, DeviceIntPtr src)
{
    return XkbDeviceApplyKeymap(dst, src->key->xkbInfo->desc);
}

int
XkbGetEffectiveGroup(XkbSrvInfoPtr xkbi, XkbStatePtr xkbState, CARD8 keycode)
{
    XkbDescPtr xkb = xkbi->desc;
    int effectiveGroup = xkbState->group;

    if (!XkbKeycodeInRange(xkb, keycode))
        return -1;

    if (effectiveGroup == XkbGroup1Index)
        return effectiveGroup;

    if (XkbKeyNumGroups(xkb, keycode) > 1U) {
        if (effectiveGroup >= XkbKeyNumGroups(xkb, keycode)) {
            unsigned int gi = XkbKeyGroupInfo(xkb, keycode);

            switch (XkbOutOfRangeGroupAction(gi)) {
            default:
            case XkbWrapIntoRange:
                effectiveGroup %= XkbKeyNumGroups(xkb, keycode);
                break;
            case XkbClampIntoRange:
                effectiveGroup = XkbKeyNumGroups(xkb, keycode) - 1;
                break;
            case XkbRedirectIntoRange:
                effectiveGroup = XkbOutOfRangeGroupInfo(gi);
                if (effectiveGroup >= XkbKeyNumGroups(xkb, keycode))
                    effectiveGroup = 0;
                break;
            }
        }
    }
    else
        effectiveGroup = XkbGroup1Index;

    return effectiveGroup;
}

/* Merge the lockedPtrButtons from all attached SDs for the given master
 * device into the MD's state.
 */
void
XkbMergeLockedPtrBtns(DeviceIntPtr master)
{
    DeviceIntPtr d = inputInfo.devices;
    XkbSrvInfoPtr xkbi = NULL;

    if (!IsMaster(master))
        return;

    if (!master->key)
        return;

    xkbi = master->key->xkbInfo;
    xkbi->lockedPtrButtons = 0;

    for (; d; d = d->next) {
        if (IsMaster(d) || GetMaster(d, MASTER_KEYBOARD) != master || !d->key)
            continue;

        xkbi->lockedPtrButtons |= d->key->xkbInfo->lockedPtrButtons;
    }
}

void
XkbCopyControls(XkbDescPtr dst, XkbDescPtr src)
{
    int i, nG, nTG;

    if (!dst || !src)
        return;

    *dst->ctrls = *src->ctrls;

    for (nG = nTG = 0, i = dst->min_key_code; i <= dst->max_key_code; i++) {
        nG = XkbKeyNumGroups(dst, i);
        if (nG >= XkbNumKbdGroups) {
            nTG = XkbNumKbdGroups;
            break;
        }
        if (nG > nTG) {
            nTG = nG;
        }
    }
    dst->ctrls->num_groups = nTG;
}
