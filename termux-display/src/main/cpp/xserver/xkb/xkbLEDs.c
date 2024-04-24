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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "inputstr.h"

#include <X11/extensions/XI.h>
#include <xkbsrv.h>
#include "xkb.h"

/***====================================================================***/

        /*
         * unsigned
         * XkbIndicatorsToUpdate(dev,changed,check_devs_rtrn)
         *
         * Given a keyboard and a set of state components that have changed,
         * this function returns the indicators on the default keyboard
         * feedback that might be affected.   It also reports whether or not
         * any extension devices might be affected in check_devs_rtrn.
         */

unsigned
XkbIndicatorsToUpdate(DeviceIntPtr dev,
                      unsigned long state_changes, Bool enable_changes)
{
    register unsigned update = 0;
    XkbSrvLedInfoPtr sli;

    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId, 0);

    if (!sli)
        return update;

    if (state_changes & (XkbModifierStateMask | XkbGroupStateMask))
        update |= sli->usesEffective;
    if (state_changes & (XkbModifierBaseMask | XkbGroupBaseMask))
        update |= sli->usesBase;
    if (state_changes & (XkbModifierLatchMask | XkbGroupLatchMask))
        update |= sli->usesLatched;
    if (state_changes & (XkbModifierLockMask | XkbGroupLockMask))
        update |= sli->usesLocked;
    if (state_changes & XkbCompatStateMask)
        update |= sli->usesCompat;
    if (enable_changes)
        update |= sli->usesControls;
    return update;
}

/***====================================================================***/

        /*
         * Bool
         *XkbApplyLEDChangeToKeyboard(xkbi,map,on,change)
         *
         * Some indicators "drive" the keyboard when their state is explicitly
         * changed, as described in section 9.2.1 of the XKB protocol spec.
         * This function updates the state and controls for the keyboard
         * specified by 'xkbi' to reflect any changes that are required
         * when the indicator described by 'map' is turned on or off.  The
         * extent of the changes is reported in change, which must be defined.
         */
static Bool
XkbApplyLEDChangeToKeyboard(XkbSrvInfoPtr xkbi,
                            XkbIndicatorMapPtr map,
                            Bool on, XkbChangesPtr change)
{
    Bool ctrlChange, stateChange;
    XkbStatePtr state;

    if ((map->flags & XkbIM_NoExplicit) ||
        ((map->flags & XkbIM_LEDDrivesKB) == 0))
        return FALSE;
    ctrlChange = stateChange = FALSE;
    if (map->ctrls) {
        XkbControlsPtr ctrls = xkbi->desc->ctrls;
        unsigned old;

        old = ctrls->enabled_ctrls;
        if (on)
            ctrls->enabled_ctrls |= map->ctrls;
        else
            ctrls->enabled_ctrls &= ~map->ctrls;
        if (old != ctrls->enabled_ctrls) {
            change->ctrls.changed_ctrls = XkbControlsEnabledMask;
            change->ctrls.enabled_ctrls_changes = old ^ ctrls->enabled_ctrls;
            ctrlChange = TRUE;
        }
    }
    state = &xkbi->state;
    if ((map->groups) && ((map->which_groups & (~XkbIM_UseBase)) != 0)) {
        register int i;
        register unsigned bit, match;

        if (on)
            match = (map->groups) & XkbAllGroupsMask;
        else
            match = (~map->groups) & XkbAllGroupsMask;
        if (map->which_groups & (XkbIM_UseLocked | XkbIM_UseEffective)) {
            for (i = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
                if (bit & match)
                    break;
            }
            if (map->which_groups & XkbIM_UseLatched)
                XkbLatchGroup(xkbi->device, 0); /* unlatch group */
            state->locked_group = i;
            stateChange = TRUE;
        }
        else if (map->which_groups & (XkbIM_UseLatched | XkbIM_UseEffective)) {
            for (i = 0, bit = 1; i < XkbNumKbdGroups; i++, bit <<= 1) {
                if (bit & match)
                    break;
            }
            state->locked_group = 0;
            XkbLatchGroup(xkbi->device, i);
            stateChange = TRUE;
        }
    }
    if ((map->mods.mask) && ((map->which_mods & (~XkbIM_UseBase)) != 0)) {
        if (map->which_mods & (XkbIM_UseLocked | XkbIM_UseEffective)) {
            register unsigned long old;

            old = state->locked_mods;
            if (on)
                state->locked_mods |= map->mods.mask;
            else
                state->locked_mods &= ~map->mods.mask;
            if (state->locked_mods != old)
                stateChange = TRUE;
        }
        if (map->which_mods & (XkbIM_UseLatched | XkbIM_UseEffective)) {
            register unsigned long newmods;

            newmods = state->latched_mods;
            if (on)
                newmods |= map->mods.mask;
            else
                newmods &= ~map->mods.mask;
            if (newmods != state->locked_mods) {
                newmods &= map->mods.mask;
                XkbLatchModifiers(xkbi->device, map->mods.mask, newmods);
                stateChange = TRUE;
            }
        }
    }
    return stateChange || ctrlChange;
}

        /*
         * Bool
         * ComputeAutoState(map,state,ctrls)
         *
         * This function reports the effect of applying the specified
         * indicator map given the specified state and controls, as
         * described in section 9.2 of the XKB protocol specification.
         */

static Bool
ComputeAutoState(XkbIndicatorMapPtr map,
                 XkbStatePtr state, XkbControlsPtr ctrls)
{
    Bool on;
    CARD8 mods, group;

    on = FALSE;
    mods = group = 0;
    if (map->which_mods & XkbIM_UseAnyMods) {
        if (map->which_mods & XkbIM_UseBase)
            mods |= state->base_mods;
        if (map->which_mods & XkbIM_UseLatched)
            mods |= state->latched_mods;
        if (map->which_mods & XkbIM_UseLocked)
            mods |= state->locked_mods;
        if (map->which_mods & XkbIM_UseEffective)
            mods |= state->mods;
        if (map->which_mods & XkbIM_UseCompat)
            mods |= state->compat_state;
        on = ((map->mods.mask & mods) != 0);
        on = on || ((mods == 0) && (map->mods.mask == 0) &&
                    (map->mods.vmods == 0));
    }
    if (map->which_groups & XkbIM_UseAnyGroup) {
        if (map->which_groups & XkbIM_UseBase)
            group |= (1L << state->base_group);
        if (map->which_groups & XkbIM_UseLatched)
            group |= (1L << state->latched_group);
        if (map->which_groups & XkbIM_UseLocked)
            group |= (1L << state->locked_group);
        if (map->which_groups & XkbIM_UseEffective)
            group |= (1L << state->group);
        on = on || (((map->groups & group) != 0) || (map->groups == 0));
    }
    if (map->ctrls)
        on = on || (ctrls->enabled_ctrls & map->ctrls);
    return on;
}

static void
XkbUpdateLedAutoState(DeviceIntPtr dev,
                      XkbSrvLedInfoPtr sli,
                      unsigned maps_to_check,
                      xkbExtensionDeviceNotify * ed,
                      XkbChangesPtr changes, XkbEventCausePtr cause)
{
    DeviceIntPtr kbd;
    XkbStatePtr state;
    XkbControlsPtr ctrls;
    XkbChangesRec my_changes;
    xkbExtensionDeviceNotify my_ed;
    register unsigned i, bit, affected;
    register XkbIndicatorMapPtr map;
    unsigned oldState;

    if ((maps_to_check == 0) || (sli->maps == NULL) || (sli->mapsPresent == 0))
        return;

    if (dev->key && dev->key->xkbInfo)
        kbd = dev;
    else
        kbd = inputInfo.keyboard;

    state = &kbd->key->xkbInfo->state;
    ctrls = kbd->key->xkbInfo->desc->ctrls;
    affected = maps_to_check;
    oldState = sli->effectiveState;
    sli->autoState &= ~affected;
    for (i = 0, bit = 1; (i < XkbNumIndicators) && (affected); i++, bit <<= 1) {
        if ((affected & bit) == 0)
            continue;
        affected &= ~bit;
        map = &sli->maps[i];
        if ((!(map->flags & XkbIM_NoAutomatic)) &&
            ComputeAutoState(map, state, ctrls))
            sli->autoState |= bit;
    }
    sli->effectiveState = (sli->autoState | sli->explicitState);
    affected = sli->effectiveState ^ oldState;
    if (affected == 0)
        return;

    if (ed == NULL) {
        ed = &my_ed;
        memset((char *) ed, 0, sizeof(xkbExtensionDeviceNotify));
    }
    else if ((ed->reason & XkbXI_IndicatorsMask) &&
             ((ed->ledClass != sli->class) || (ed->ledID != sli->id))) {
        XkbFlushLedEvents(dev, kbd, sli, ed, changes, cause);
    }

    if ((kbd == dev) && (sli->flags & XkbSLI_IsDefault)) {
        if (changes == NULL) {
            changes = &my_changes;
            memset((char *) changes, 0, sizeof(XkbChangesRec));
        }
        changes->indicators.state_changes |= affected;
    }

    ed->reason |= XkbXI_IndicatorStateMask;
    ed->ledClass = sli->class;
    ed->ledID = sli->id;
    ed->ledsDefined = sli->namesPresent | sli->mapsPresent;
    ed->ledState = sli->effectiveState;
    ed->unsupported = 0;
    ed->supported = XkbXI_AllFeaturesMask;

    if (changes != &my_changes)
        changes = NULL;
    if (ed != &my_ed)
        ed = NULL;
    if (changes || ed)
        XkbFlushLedEvents(dev, kbd, sli, ed, changes, cause);
    return;
}

void
XkbUpdateAllDeviceIndicators(XkbChangesPtr changes, XkbEventCausePtr cause)
{
    DeviceIntPtr edev;
    XkbSrvLedInfoPtr sli;

    for (edev = inputInfo.devices; edev != NULL; edev = edev->next) {
        if (edev->kbdfeed) {
            KbdFeedbackPtr kf;

            for (kf = edev->kbdfeed; kf != NULL; kf = kf->next) {
                if ((kf->xkb_sli == NULL) || (kf->xkb_sli->maps == NULL))
                    continue;
                sli = kf->xkb_sli;
                XkbUpdateLedAutoState(edev, sli, sli->mapsPresent, NULL,
                                      changes, cause);

            }
        }
        if (edev->leds) {
            LedFeedbackPtr lf;

            for (lf = edev->leds; lf != NULL; lf = lf->next) {
                if ((lf->xkb_sli == NULL) || (lf->xkb_sli->maps == NULL))
                    continue;
                sli = lf->xkb_sli;
                XkbUpdateLedAutoState(edev, sli, sli->mapsPresent, NULL,
                                      changes, cause);

            }
        }
    }
    return;
}

/***====================================================================***/

        /*
         * void
         * XkbSetIndicators(dev,affect,values,cause)
         *
         * Attempts to change the indicators specified in 'affect' to the
         * states specified in 'values' for the default keyboard feedback
         * on the keyboard specified by 'dev.'   Attempts to change indicator
         * state might be ignored or have no affect, depending on the XKB
         * indicator map for any affected indicators, as described in section
         * 9.2 of the XKB protocol specification.
         *
         * If 'changes' is non-NULL, this function notes any changes to the
         * keyboard state, controls, or indicator state that result from this
         * attempted change.   If 'changes' is NULL, this function generates
         * XKB events to report any such changes to interested clients.
         *
         * If 'cause' is non-NULL, it specifies the reason for the change,
         * as reported in some XKB events.   If it is NULL, this function
         * assumes that the change is the result of a core protocol
         * ChangeKeyboardMapping request.
         */

void
XkbSetIndicators(DeviceIntPtr dev,
                 CARD32 affect, CARD32 values, XkbEventCausePtr cause)
{
    XkbSrvLedInfoPtr sli;
    XkbChangesRec changes;
    xkbExtensionDeviceNotify ed;
    unsigned side_affected;

    memset((char *) &changes, 0, sizeof(XkbChangesRec));
    memset((char *) &ed, 0, sizeof(xkbExtensionDeviceNotify));
    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId, 0);
    sli->explicitState &= ~affect;
    sli->explicitState |= (affect & values);
    XkbApplyLedStateChanges(dev, sli, affect, &ed, &changes, cause);

    side_affected = 0;
    if (changes.state_changes != 0)
        side_affected |=
            XkbIndicatorsToUpdate(dev, changes.state_changes, FALSE);
    if (changes.ctrls.enabled_ctrls_changes)
        side_affected |= sli->usesControls;

    if (side_affected) {
        XkbUpdateLedAutoState(dev, sli, side_affected, &ed, &changes, cause);
        affect |= side_affected;
    }
    if (changes.state_changes || changes.ctrls.enabled_ctrls_changes)
        XkbUpdateAllDeviceIndicators(NULL, cause);

    XkbFlushLedEvents(dev, dev, sli, &ed, &changes, cause);
    return;
}

/***====================================================================***/

/***====================================================================***/

        /*
         * void
         * XkbUpdateIndicators(dev,update,check_edevs,changes,cause)
         *
         * Applies the indicator maps for any indicators specified in
         * 'update' from the default keyboard feedback on the device
         * specified by 'dev.'
         *
         * If 'changes' is NULL, this function generates and XKB events
         * required to report the necessary changes, otherwise it simply
         * notes the indicators with changed state.
         *
         * If 'check_edevs' is TRUE, this function also checks the indicator
         * maps for any open extension devices that have them, and updates
         * the state of any extension device indicators as necessary.
         */

void
XkbUpdateIndicators(DeviceIntPtr dev,
                    register CARD32 update,
                    Bool check_edevs,
                    XkbChangesPtr changes, XkbEventCausePtr cause)
{
    XkbSrvLedInfoPtr sli;

    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId, 0);
    XkbUpdateLedAutoState(dev, sli, update, NULL, changes, cause);
    if (check_edevs)
        XkbUpdateAllDeviceIndicators(changes, cause);
    return;
}

/***====================================================================***/

/***====================================================================***/

        /*
         * void
         * XkbCheckIndicatorMaps(dev,sli,which)
         *
         * Updates the 'indicator accelerators' for the indicators specified
         * by 'which' in the feedback specified by 'sli.' The indicator
         * accelerators are internal to the server and are used to simplify
         * and speed up the process of figuring out which indicators might
         * be affected by a particular change in keyboard state or controls.
         */

void
XkbCheckIndicatorMaps(DeviceIntPtr dev, XkbSrvLedInfoPtr sli, unsigned which)
{
    register unsigned i, bit;
    XkbIndicatorMapPtr map;
    XkbDescPtr xkb;

    if ((sli->flags & XkbSLI_HasOwnState) == 0)
        return;

    sli->usesBase &= ~which;
    sli->usesLatched &= ~which;
    sli->usesLocked &= ~which;
    sli->usesEffective &= ~which;
    sli->usesCompat &= ~which;
    sli->usesControls &= ~which;
    sli->mapsPresent &= ~which;

    xkb = dev->key->xkbInfo->desc;
    for (i = 0, bit = 1, map = sli->maps; i < XkbNumIndicators;
         i++, bit <<= 1, map++) {
        if (which & bit) {
            CARD8 what;

            if (!map || !XkbIM_InUse(map))
                continue;
            sli->mapsPresent |= bit;

            what = (map->which_mods | map->which_groups);
            if (what & XkbIM_UseBase)
                sli->usesBase |= bit;
            if (what & XkbIM_UseLatched)
                sli->usesLatched |= bit;
            if (what & XkbIM_UseLocked)
                sli->usesLocked |= bit;
            if (what & XkbIM_UseEffective)
                sli->usesEffective |= bit;
            if (what & XkbIM_UseCompat)
                sli->usesCompat |= bit;
            if (map->ctrls)
                sli->usesControls |= bit;

            map->mods.mask = map->mods.real_mods;
            if (map->mods.vmods != 0) {
                map->mods.mask |= XkbMaskForVMask(xkb, map->mods.vmods);
            }
        }
    }
    sli->usedComponents = 0;
    if (sli->usesBase)
        sli->usedComponents |= XkbModifierBaseMask | XkbGroupBaseMask;
    if (sli->usesLatched)
        sli->usedComponents |= XkbModifierLatchMask | XkbGroupLatchMask;
    if (sli->usesLocked)
        sli->usedComponents |= XkbModifierLockMask | XkbGroupLockMask;
    if (sli->usesEffective)
        sli->usedComponents |= XkbModifierStateMask | XkbGroupStateMask;
    if (sli->usesCompat)
        sli->usedComponents |= XkbCompatStateMask;
    return;
}

/***====================================================================***/

        /*
         * XkbSrvLedInfoPtr
         * XkbAllocSrvLedInfo(dev,kf,lf,needed_parts)
         *
         * Allocates an XkbSrvLedInfoPtr for the feedback specified by either
         * 'kf' or 'lf' on the keyboard specified by 'dev.'
         *
         * If 'needed_parts' is non-zero, this function makes sure that any
         * of the parts speicified therein are allocated.
         */
XkbSrvLedInfoPtr
XkbAllocSrvLedInfo(DeviceIntPtr dev,
                   KbdFeedbackPtr kf, LedFeedbackPtr lf, unsigned needed_parts)
{
    XkbSrvLedInfoPtr sli;
    Bool checkAccel;
    Bool checkNames;

    sli = NULL;
    checkAccel = checkNames = FALSE;
    if ((kf != NULL) && (kf->xkb_sli == NULL)) {
        kf->xkb_sli = sli = calloc(1, sizeof(XkbSrvLedInfoRec));
        if (sli == NULL)
            return NULL;        /* ALLOCATION ERROR */
        if (dev->key && dev->key->xkbInfo)
            sli->flags = XkbSLI_HasOwnState;
        else
            sli->flags = 0;
        sli->class = KbdFeedbackClass;
        sli->id = kf->ctrl.id;
        sli->fb.kf = kf;

        sli->autoState = 0;
        sli->explicitState = kf->ctrl.leds;
        sli->effectiveState = kf->ctrl.leds;

        if ((kf == dev->kbdfeed) && (dev->key) && (dev->key->xkbInfo)) {
            XkbDescPtr xkb;

            xkb = dev->key->xkbInfo->desc;
            sli->flags |= XkbSLI_IsDefault;
            sli->physIndicators = xkb->indicators->phys_indicators;
            sli->names = xkb->names->indicators;
            sli->maps = xkb->indicators->maps;
            checkNames = checkAccel = TRUE;
        }
        else {
            sli->physIndicators = XkbAllIndicatorsMask;
            sli->names = NULL;
            sli->maps = NULL;
        }
    }
    else if ((kf != NULL) && ((kf->xkb_sli->flags & XkbSLI_IsDefault) != 0)) {
        XkbDescPtr xkb;

        xkb = dev->key->xkbInfo->desc;
        sli = kf->xkb_sli;
        sli->physIndicators = xkb->indicators->phys_indicators;
        if (xkb->names->indicators != sli->names) {
            checkNames = TRUE;
            sli->names = xkb->names->indicators;
        }
        if (xkb->indicators->maps != sli->maps) {
            checkAccel = TRUE;
            sli->maps = xkb->indicators->maps;
        }
    }
    else if ((lf != NULL) && (lf->xkb_sli == NULL)) {
        lf->xkb_sli = sli = calloc(1, sizeof(XkbSrvLedInfoRec));
        if (sli == NULL)
            return NULL;        /* ALLOCATION ERROR */
        if (dev->key && dev->key->xkbInfo)
            sli->flags = XkbSLI_HasOwnState;
        else
            sli->flags = 0;
        sli->class = LedFeedbackClass;
        sli->id = lf->ctrl.id;
        sli->fb.lf = lf;

        sli->physIndicators = lf->ctrl.led_mask;
        sli->autoState = 0;
        sli->explicitState = lf->ctrl.led_values;
        sli->effectiveState = lf->ctrl.led_values;
        sli->maps = NULL;
        sli->names = NULL;
    }
    else
        return NULL;
    if ((sli->names == NULL) && (needed_parts & XkbXI_IndicatorNamesMask))
        sli->names = calloc(XkbNumIndicators, sizeof(Atom));
    if ((sli->maps == NULL) && (needed_parts & XkbXI_IndicatorMapsMask))
        sli->maps = calloc(XkbNumIndicators, sizeof(XkbIndicatorMapRec));
    if (checkNames) {
        register unsigned i, bit;

        sli->namesPresent = 0;
        for (i = 0, bit = 1; i < XkbNumIndicators; i++, bit <<= 1) {
            if (sli->names[i] != None)
                sli->namesPresent |= bit;
        }
    }
    if (checkAccel)
        XkbCheckIndicatorMaps(dev, sli, XkbAllIndicatorsMask);
    return sli;
}

void
XkbFreeSrvLedInfo(XkbSrvLedInfoPtr sli)
{
    if ((sli->flags & XkbSLI_IsDefault) == 0) {
        free(sli->maps);
        free(sli->names);
    }
    sli->maps = NULL;
    sli->names = NULL;
    free(sli);
    return;
}

/*
 * XkbSrvLedInfoPtr
 * XkbCopySrvLedInfo(dev,src,kf,lf)
 *
 * Takes the given XkbSrvLedInfoPtr and duplicates it. A deep copy is made,
 * thus the new copy behaves like the original one and can be freed with
 * XkbFreeSrvLedInfo.
 */
XkbSrvLedInfoPtr
XkbCopySrvLedInfo(DeviceIntPtr from,
                  XkbSrvLedInfoPtr src, KbdFeedbackPtr kf, LedFeedbackPtr lf)
{
    XkbSrvLedInfoPtr sli_new = NULL;

    if (!src)
        goto finish;

    sli_new = calloc(1, sizeof(XkbSrvLedInfoRec));
    if (!sli_new)
        goto finish;

    memcpy(sli_new, src, sizeof(XkbSrvLedInfoRec));
    if (sli_new->class == KbdFeedbackClass)
        sli_new->fb.kf = kf;
    else
        sli_new->fb.lf = lf;

    if (!(sli_new->flags & XkbSLI_IsDefault)) {
        sli_new->names = calloc(XkbNumIndicators, sizeof(Atom));
        sli_new->maps = calloc(XkbNumIndicators, sizeof(XkbIndicatorMapRec));
    }                           /* else sli_new->names/maps is pointing to
                                   dev->key->xkbInfo->desc->names->indicators;
                                   dev->key->xkbInfo->desc->names->indicators; */

 finish:
    return sli_new;
}

/***====================================================================***/

        /*
         * XkbSrvLedInfoPtr
         * XkbFindSrvLedInfo(dev,class,id,needed_parts)
         *
         * Finds the XkbSrvLedInfoPtr for the specified 'class' and 'id'
         * on the device specified by 'dev.'   If the class and id specify
         * a valid device feedback, this function returns the existing
         * feedback or allocates a new one.
         *
         */

XkbSrvLedInfoPtr
XkbFindSrvLedInfo(DeviceIntPtr dev,
                  unsigned class, unsigned id, unsigned needed_parts)
{
    XkbSrvLedInfoPtr sli;

    /* optimization to check for most common case */
    if (((class == XkbDfltXIClass) && (id == XkbDfltXIId)) && (dev->kbdfeed)) {
        if (dev->kbdfeed->xkb_sli == NULL) {
            dev->kbdfeed->xkb_sli =
                XkbAllocSrvLedInfo(dev, dev->kbdfeed, NULL, needed_parts);
        }
        return dev->kbdfeed->xkb_sli;
    }

    sli = NULL;
    if (class == XkbDfltXIClass) {
        if (dev->kbdfeed)
            class = KbdFeedbackClass;
        else if (dev->leds)
            class = LedFeedbackClass;
        else
            return NULL;
    }
    if (class == KbdFeedbackClass) {
        KbdFeedbackPtr kf;

        for (kf = dev->kbdfeed; kf != NULL; kf = kf->next) {
            if ((id == XkbDfltXIId) || (id == kf->ctrl.id)) {
                if (kf->xkb_sli == NULL)
                    kf->xkb_sli =
                        XkbAllocSrvLedInfo(dev, kf, NULL, needed_parts);
                sli = kf->xkb_sli;
                break;
            }
        }
    }
    else if (class == LedFeedbackClass) {
        LedFeedbackPtr lf;

        for (lf = dev->leds; lf != NULL; lf = lf->next) {
            if ((id == XkbDfltXIId) || (id == lf->ctrl.id)) {
                if (lf->xkb_sli == NULL)
                    lf->xkb_sli =
                        XkbAllocSrvLedInfo(dev, NULL, lf, needed_parts);
                sli = lf->xkb_sli;
                break;
            }
        }
    }
    if (sli) {
        if ((sli->names == NULL) && (needed_parts & XkbXI_IndicatorNamesMask))
            sli->names = calloc(XkbNumIndicators, sizeof(Atom));
        if ((sli->maps == NULL) && (needed_parts & XkbXI_IndicatorMapsMask))
            sli->maps = calloc(XkbNumIndicators, sizeof(XkbIndicatorMapRec));
    }
    return sli;
}

/***====================================================================***/

void
XkbFlushLedEvents(DeviceIntPtr dev,
                  DeviceIntPtr kbd,
                  XkbSrvLedInfoPtr sli,
                  xkbExtensionDeviceNotify * ed,
                  XkbChangesPtr changes, XkbEventCausePtr cause)
{
    if (changes) {
        if (changes->indicators.state_changes)
            XkbDDXUpdateDeviceIndicators(dev, sli, sli->effectiveState);
        XkbSendNotification(kbd, changes, cause);
        memset((char *) changes, 0, sizeof(XkbChangesRec));

        if (XkbAX_NeedFeedback
            (kbd->key->xkbInfo->desc->ctrls, XkbAX_IndicatorFBMask)) {
            if (sli->effectiveState)
                /* it appears that the which parameter is not used */
                XkbDDXAccessXBeep(dev, _BEEP_LED_ON, XkbAccessXFeedbackMask);
            else
                XkbDDXAccessXBeep(dev, _BEEP_LED_OFF, XkbAccessXFeedbackMask);
        }
    }
    if (ed) {
        if (ed->reason) {
            if ((dev != kbd) && (ed->reason & XkbXI_IndicatorStateMask))
                XkbDDXUpdateDeviceIndicators(dev, sli, sli->effectiveState);
            XkbSendExtensionDeviceNotify(dev, cause->client, ed);
        }
        memset((char *) ed, 0, sizeof(XkbExtensionDeviceNotify));
    }
    return;
}

/***====================================================================***/

void
XkbApplyLedNameChanges(DeviceIntPtr dev,
                       XkbSrvLedInfoPtr sli,
                       unsigned changed_names,
                       xkbExtensionDeviceNotify * ed,
                       XkbChangesPtr changes, XkbEventCausePtr cause)
{
    DeviceIntPtr kbd;
    XkbChangesRec my_changes;
    xkbExtensionDeviceNotify my_ed;

    if (changed_names == 0)
        return;
    if (dev->key && dev->key->xkbInfo)
        kbd = dev;
    else
        kbd = inputInfo.keyboard;

    if (ed == NULL) {
        ed = &my_ed;
        memset((char *) ed, 0, sizeof(xkbExtensionDeviceNotify));
    }
    else if ((ed->reason & XkbXI_IndicatorsMask) &&
             ((ed->ledClass != sli->class) || (ed->ledID != sli->id))) {
        XkbFlushLedEvents(dev, kbd, sli, ed, changes, cause);
    }

    if ((kbd == dev) && (sli->flags & XkbSLI_IsDefault)) {
        if (changes == NULL) {
            changes = &my_changes;
            memset((char *) changes, 0, sizeof(XkbChangesRec));
        }
        changes->names.changed |= XkbIndicatorNamesMask;
        changes->names.changed_indicators |= changed_names;
    }

    ed->reason |= XkbXI_IndicatorNamesMask;
    ed->ledClass = sli->class;
    ed->ledID = sli->id;
    ed->ledsDefined = sli->namesPresent | sli->mapsPresent;
    ed->ledState = sli->effectiveState;
    ed->unsupported = 0;
    ed->supported = XkbXI_AllFeaturesMask;

    if (changes != &my_changes)
        changes = NULL;
    if (ed != &my_ed)
        ed = NULL;
    if (changes || ed)
        XkbFlushLedEvents(dev, kbd, sli, ed, changes, cause);
    return;
}

/***====================================================================***/

        /*
         * void
         * XkbApplyLedMapChanges(dev,sli,changed_maps,changes,cause)
         *
         * Handles all of the secondary effects of the changes to the
         * feedback specified by 'sli' on the device specified by 'dev.'
         *
         * If 'changed_maps' specifies any indicators, this function generates
         * XkbExtensionDeviceNotify events and possibly IndicatorMapNotify
         * events to report the changes, and recalculates the effective
         * state of each indicator with a changed map.  If any indicators
         * change state, the server generates XkbExtensionDeviceNotify and
         * XkbIndicatorStateNotify events as appropriate.
         *
         * If 'changes' is non-NULL, this function updates it to reflect
         * any changes to the keyboard state or controls or to the 'core'
         * indicator names, maps, or state.   If 'changes' is NULL, this
         * function generates XKB events as needed to report the changes.
         * If 'dev' is not a keyboard device, any changes are reported
         * for the core keyboard.
         *
         * The 'cause' specifies the reason for the event (key event or
         * request) for the change, as reported in some XKB events.
         */

void
XkbApplyLedMapChanges(DeviceIntPtr dev,
                      XkbSrvLedInfoPtr sli,
                      unsigned changed_maps,
                      xkbExtensionDeviceNotify * ed,
                      XkbChangesPtr changes, XkbEventCausePtr cause)
{
    DeviceIntPtr kbd;
    XkbChangesRec my_changes;
    xkbExtensionDeviceNotify my_ed;

    if (changed_maps == 0)
        return;
    if (dev->key && dev->key->xkbInfo)
        kbd = dev;
    else
        kbd = inputInfo.keyboard;

    if (ed == NULL) {
        ed = &my_ed;
        memset((char *) ed, 0, sizeof(xkbExtensionDeviceNotify));
    }
    else if ((ed->reason & XkbXI_IndicatorsMask) &&
             ((ed->ledClass != sli->class) || (ed->ledID != sli->id))) {
        XkbFlushLedEvents(dev, kbd, sli, ed, changes, cause);
    }

    if ((kbd == dev) && (sli->flags & XkbSLI_IsDefault)) {
        if (changes == NULL) {
            changes = &my_changes;
            memset((char *) changes, 0, sizeof(XkbChangesRec));
        }
        changes->indicators.map_changes |= changed_maps;
    }

    XkbCheckIndicatorMaps(dev, sli, changed_maps);

    ed->reason |= XkbXI_IndicatorMapsMask;
    ed->ledClass = sli->class;
    ed->ledID = sli->id;
    ed->ledsDefined = sli->namesPresent | sli->mapsPresent;
    ed->ledState = sli->effectiveState;
    ed->unsupported = 0;
    ed->supported = XkbXI_AllFeaturesMask;

    XkbUpdateLedAutoState(dev, sli, changed_maps, ed, changes, cause);

    if (changes != &my_changes)
        changes = NULL;
    if (ed != &my_ed)
        ed = NULL;
    if (changes || ed)
        XkbFlushLedEvents(dev, kbd, sli, ed, changes, cause);
    return;
}

/***====================================================================***/

void
XkbApplyLedStateChanges(DeviceIntPtr dev,
                        XkbSrvLedInfoPtr sli,
                        unsigned changed_leds,
                        xkbExtensionDeviceNotify * ed,
                        XkbChangesPtr changes, XkbEventCausePtr cause)
{
    XkbSrvInfoPtr xkbi;
    DeviceIntPtr kbd;
    XkbChangesRec my_changes;
    xkbExtensionDeviceNotify my_ed;
    register unsigned i, bit, affected;
    XkbIndicatorMapPtr map;
    unsigned oldState;
    Bool kb_changed;

    if (changed_leds == 0)
        return;
    if (dev->key && dev->key->xkbInfo)
        kbd = dev;
    else
        kbd = inputInfo.keyboard;
    xkbi = kbd->key->xkbInfo;

    if (changes == NULL) {
        changes = &my_changes;
        memset((char *) changes, 0, sizeof(XkbChangesRec));
    }

    kb_changed = FALSE;
    affected = changed_leds;
    oldState = sli->effectiveState;
    for (i = 0, bit = 1; (i < XkbNumIndicators) && (affected); i++, bit <<= 1) {
        if ((affected & bit) == 0)
            continue;
        affected &= ~bit;
        map = &sli->maps[i];
        if (map->flags & XkbIM_NoExplicit) {
            sli->explicitState &= ~bit;
            continue;
        }
        if (map->flags & XkbIM_LEDDrivesKB) {
            Bool on = ((sli->explicitState & bit) != 0);

            if (XkbApplyLEDChangeToKeyboard(xkbi, map, on, changes))
                kb_changed = TRUE;
        }
    }
    sli->effectiveState = (sli->autoState | sli->explicitState);
    affected = sli->effectiveState ^ oldState;

    if (ed == NULL) {
        ed = &my_ed;
        memset((char *) ed, 0, sizeof(xkbExtensionDeviceNotify));
    }
    else if (affected && (ed->reason & XkbXI_IndicatorsMask) &&
             ((ed->ledClass != sli->class) || (ed->ledID != sli->id))) {
        XkbFlushLedEvents(dev, kbd, sli, ed, changes, cause);
    }

    if ((kbd == dev) && (sli->flags & XkbSLI_IsDefault))
        changes->indicators.state_changes |= affected;
    if (affected) {
        ed->reason |= XkbXI_IndicatorStateMask;
        ed->ledClass = sli->class;
        ed->ledID = sli->id;
        ed->ledsDefined = sli->namesPresent | sli->mapsPresent;
        ed->ledState = sli->effectiveState;
        ed->unsupported = 0;
        ed->supported = XkbXI_AllFeaturesMask;
    }

    if (kb_changed) {
        XkbComputeDerivedState(kbd->key->xkbInfo);
        XkbUpdateLedAutoState(dev, sli, sli->mapsPresent, ed, changes, cause);
    }

    if (changes != &my_changes)
        changes = NULL;
    if (ed != &my_ed)
        ed = NULL;
    if (changes || ed)
        XkbFlushLedEvents(dev, kbd, sli, ed, changes, cause);
    if (kb_changed)
        XkbUpdateAllDeviceIndicators(NULL, cause);
    return;
}
