/* Copyright (C) 2009 TightVNC Team
 * Copyright (C) 2009 Red Hat, Inc.
 * Copyright 2013-2018 Pierre Ossman for Cendio AB
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "UnusedValue"
#pragma ide diagnostic ignored "ConstantParameter"
#pragma ide diagnostic ignored "misc-no-recursion"
#pragma ide diagnostic ignored "OCDFAInspection"
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>

#include <globals.h>
#include "xkbsrv.h"
#include "xkbstr.h"
#include "eventstr.h"
#include "scrnintstr.h"
#include "mi.h"

#include <X11/keysym.h>
#include <selection.h>

#ifndef KEYBOARD_OR_FLOAT
#define KEYBOARD_OR_FLOAT MASTER_KEYBOARD
#endif

#define unused __attribute__((__unused__))

extern DeviceIntPtr lorieKeyboard;

static const KeyCode fakeKeys[] = {
    92, 203, 204, 205, 206, 207
    };

static KeySym pressedKeys[256] = {0};

/* altKeysym is a table of alternative keysyms which have the same meaning. */

static struct altKeysym_t {
	KeySym a, b;
} altKeysym[] = {
		{ XK_Shift_L,		XK_Shift_R },
		{ XK_Control_L,		XK_Control_R },
		{ XK_Meta_L,		XK_Meta_R },
		{ XK_Alt_L,		XK_Alt_R },
		{ XK_Super_L,		XK_Super_R },
		{ XK_Hyper_L,		XK_Hyper_R },
		{ XK_KP_Space,		XK_space },
		{ XK_KP_Tab,		XK_Tab },
		{ XK_KP_Enter,		XK_Return },
		{ XK_KP_F1,		XK_F1 },
		{ XK_KP_F2,		XK_F2 },
		{ XK_KP_F3,		XK_F3 },
		{ XK_KP_F4,		XK_F4 },
		{ XK_KP_Home,		XK_Home },
		{ XK_KP_Left,		XK_Left },
		{ XK_KP_Up,		XK_Up },
		{ XK_KP_Right,		XK_Right },
		{ XK_KP_Down,		XK_Down },
		{ XK_KP_Page_Up,	XK_Page_Up },
		{ XK_KP_Page_Down,	XK_Page_Down },
		{ XK_KP_End,		XK_End },
		{ XK_KP_Begin,		XK_Begin },
		{ XK_KP_Insert,		XK_Insert },
		{ XK_KP_Delete,		XK_Delete },
		{ XK_KP_Equal,		XK_equal },
		{ XK_KP_Multiply,	XK_asterisk },
		{ XK_KP_Add,		XK_plus },
		{ XK_KP_Separator,	XK_comma },
		{ XK_KP_Subtract,	XK_minus },
		{ XK_KP_Decimal,	XK_period },
		{ XK_KP_Divide,		XK_slash },
		{ XK_KP_0,		XK_0 },
		{ XK_KP_1,		XK_1 },
		{ XK_KP_2,		XK_2 },
		{ XK_KP_3,		XK_3 },
		{ XK_KP_4,		XK_4 },
		{ XK_KP_5,		XK_5 },
		{ XK_KP_6,		XK_6 },
		{ XK_KP_7,		XK_7 },
		{ XK_KP_8,		XK_8 },
		{ XK_KP_9,		XK_9 },
		{ XK_ISO_Level3_Shift,	XK_Mode_switch },
};

void lorieKeysymKeyboardEvent(KeySym keysym, int down);
KeyCode lorieKeysymToKeycode(KeySym keysym, unsigned state, unsigned *new_state);

/* Stolen from libX11 */
static Bool
XkbTranslateKeyCode(register XkbDescPtr xkb, KeyCode key,
                    register unsigned int mods, unsigned int *mods_rtrn,
                    KeySym *keysym_rtrn) {
	XkbKeyTypeRec *type;
	int col,nKeyGroups;
	unsigned preserve,effectiveGroup;
	KeySym *syms;

	if (mods_rtrn!=NULL)
		*mods_rtrn = 0;

	nKeyGroups= XkbKeyNumGroups(xkb,key);
	if ((!XkbKeycodeInRange(xkb,key))||(nKeyGroups==0)) {
		if (keysym_rtrn!=NULL)
			*keysym_rtrn = NoSymbol;
		return FALSE;
	}

	syms = XkbKeySymsPtr(xkb,key);

	/* find the offset of the effective group */
	col = 0;
	effectiveGroup= XkbGroupForCoreState(mods);
	if ( effectiveGroup>=nKeyGroups ) {
		unsigned groupInfo= XkbKeyGroupInfo(xkb,key);
		switch (XkbOutOfRangeGroupAction(groupInfo)) {
		default:
			effectiveGroup %= nKeyGroups;
			break;
		case XkbClampIntoRange:
			effectiveGroup = nKeyGroups-1;
			break;
		case XkbRedirectIntoRange:
			effectiveGroup = XkbOutOfRangeGroupNumber(groupInfo);
			if (effectiveGroup>=nKeyGroups)
				effectiveGroup= 0;
			break;
		}
	}
	col= (int) effectiveGroup * XkbKeyGroupsWidth(xkb,key);
	type = XkbKeyKeyType(xkb,key,effectiveGroup);

	preserve= 0;
	if (type->map) { /* find the column (shift level) within the group */
		register int i;
		register XkbKTMapEntryPtr entry;
		for (i=0,entry=type->map;i<type->map_count;i++,entry++) {
			if ((entry->active)&&((mods&type->mods.mask)==entry->mods.mask)) {
				col+= entry->level;
				if (type->preserve)
					preserve= type->preserve[i].mask;
				break;
			}
		}
	}

	if (keysym_rtrn!=NULL)
		*keysym_rtrn= syms[col];
	if (mods_rtrn)
		*mods_rtrn= type->mods.mask&(~preserve);

	return (syms[col]!=NoSymbol);
}

static XkbAction *XkbKeyActionPtr(XkbDescPtr xkb, KeyCode key, unsigned int mods) {
	XkbKeyTypeRec *type;
	int col,nKeyGroups;
	unsigned effectiveGroup;
	unused XkbAction *acts;

	if (!XkbKeyHasActions(xkb, key))
		return NULL;

	nKeyGroups= XkbKeyNumGroups(xkb,key);
	if ((!XkbKeycodeInRange(xkb,key))||(nKeyGroups==0))
		return NULL;

	acts = XkbKeyActionsPtr(xkb,key);

	/* find the offset of the effective group */
	col = 0;
	effectiveGroup= XkbGroupForCoreState(mods);
	if ( effectiveGroup>=nKeyGroups ) {
		unsigned groupInfo= XkbKeyGroupInfo(xkb,key);
		switch (XkbOutOfRangeGroupAction(groupInfo)) {
		default:
			effectiveGroup %= nKeyGroups;
			break;
		case XkbClampIntoRange:
			effectiveGroup = nKeyGroups-1;
			break;
		case XkbRedirectIntoRange:
			effectiveGroup = XkbOutOfRangeGroupNumber(groupInfo);
			if (effectiveGroup>=nKeyGroups)
				effectiveGroup= 0;
			break;
		}
	}
	col= (int) effectiveGroup * XkbKeyGroupsWidth(xkb,key);
	type = XkbKeyKeyType(xkb,key,effectiveGroup);

	if (type->map) { /* find the column (shift level) within the group */
		register int i;
		register XkbKTMapEntryPtr entry;
		for (i=0,entry=type->map;i<type->map_count;i++,entry++) {
			if ((entry->active)&&((mods&type->mods.mask)==entry->mods.mask)) {
				col+= entry->level;
				break;
			}
		}
	}

	return &acts[col];
}

static unsigned XkbKeyEffectiveGroup(XkbDescPtr xkb, KeyCode key, unsigned int mods) {
	int nKeyGroups;
	unsigned effectiveGroup;

	nKeyGroups= XkbKeyNumGroups(xkb,key);
	if ((!XkbKeycodeInRange(xkb,key))||(nKeyGroups==0))
		return 0;

	effectiveGroup= XkbGroupForCoreState(mods);
	if ( effectiveGroup>=nKeyGroups ) {
		unsigned groupInfo= XkbKeyGroupInfo(xkb,key);
		switch (XkbOutOfRangeGroupAction(groupInfo)) {
		default:
			effectiveGroup %= nKeyGroups;
			break;
		case XkbClampIntoRange:
			effectiveGroup = nKeyGroups-1;
			break;
		case XkbRedirectIntoRange:
			effectiveGroup = XkbOutOfRangeGroupNumber(groupInfo);
			if (effectiveGroup>=nKeyGroups)
				effectiveGroup= 0;
			break;
		}
	}

	return effectiveGroup;
}

static unsigned lorieGetKeyboardState(void) {
	DeviceIntPtr master;

	master = GetMaster(lorieKeyboard, KEYBOARD_OR_FLOAT);
	return XkbStateFieldFromRec(&master->key->xkbInfo->state);
}

static unsigned lorieGetLevelThreeMask(void) {
	unsigned state;
	KeyCode keycode;
	XkbDescPtr xkb;
	XkbAction *act;

	/* Group state is still important */
	state = lorieGetKeyboardState();
	state &= ~0xff;

	keycode = lorieKeysymToKeycode(XK_ISO_Level3_Shift, state, NULL);
	if (keycode == 0) {
		keycode = lorieKeysymToKeycode(XK_Mode_switch, state, NULL);
		if (keycode == 0)
			return 0;
	}

	xkb = GetMaster(lorieKeyboard, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;

	act = XkbKeyActionPtr(xkb, keycode, state);
	if (act == NULL)
		return 0;
	if (act->type != XkbSA_SetMods)
		return 0;

	if (act->mods.flags & XkbSA_UseModMapMods)
		return xkb->map->modmap[keycode];
	else
		return act->mods.mask;
}

static KeyCode loriePressShift(void) {
	unsigned state;

	XkbDescPtr xkb;
	unsigned int key;

	state = lorieGetKeyboardState();
	if (state & ShiftMask)
		return 0;

	xkb = GetMaster(lorieKeyboard, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		XkbAction *act;
		unsigned char mask;

		act = XkbKeyActionPtr(xkb, key, state);
		if (act == NULL)
			continue;

		if (act->type != XkbSA_SetMods)
			continue;

		if (act->mods.flags & XkbSA_UseModMapMods)
			mask = xkb->map->modmap[key];
		else
			mask = act->mods.mask;

		if ((mask & ShiftMask) == ShiftMask)
			return key;
	}

	return 0;
}

static size_t lorieReleaseShift(KeyCode *keys, size_t maxKeys) {
	size_t count;

	unsigned state;

	DeviceIntPtr master;
	XkbDescPtr xkb;
	unsigned int key;

	state = lorieGetKeyboardState();
	if (!(state & ShiftMask))
		return 0;

	count = 0;

	master = GetMaster(lorieKeyboard, KEYBOARD_OR_FLOAT);
	xkb = master->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		XkbAction *act;
		unsigned char mask;

		if (!key_is_down(master, (int) key, KEY_PROCESSED))
			continue;

		act = XkbKeyActionPtr(xkb, key, state);
		if (act == NULL)
			continue;

		if (act->type != XkbSA_SetMods)
			continue;

		if (act->mods.flags & XkbSA_UseModMapMods)
			mask = xkb->map->modmap[key];
		else
			mask = act->mods.mask;

		if (!(mask & ShiftMask))
			continue;

		if (count >= maxKeys)
			return 0;

		keys[count++] = key;
	}

	return count;
}

static KeyCode loriePressLevelThree(void) {
	unsigned state, mask;

	KeyCode keycode;
	XkbDescPtr xkb;
	XkbAction *act;

	mask = lorieGetLevelThreeMask();
	if (mask == 0)
		return 0;

	state = lorieGetKeyboardState();
	if (state & mask)
		return 0;

	keycode = lorieKeysymToKeycode(XK_ISO_Level3_Shift, state, NULL);
	if (keycode == 0) {
		keycode = lorieKeysymToKeycode(XK_Mode_switch, state, NULL);
		if (keycode == 0)
			return 0;
	}

	xkb = GetMaster(lorieKeyboard, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;

	act = XkbKeyActionPtr(xkb, keycode, state);
	if (act == NULL)
		return 0;
	if (act->type != XkbSA_SetMods)
		return 0;

	return keycode;
}

static size_t lorieReleaseLevelThree(KeyCode *keys, size_t maxKeys) {
	size_t count;

	unsigned state, mask;

	DeviceIntPtr master;
	XkbDescPtr xkb;
	unsigned int key;

	mask = lorieGetLevelThreeMask();
	if (mask == 0)
		return 0;

	state = lorieGetKeyboardState();
	if (!(state & mask))
		return 0;

	count = 0;

	master = GetMaster(lorieKeyboard, KEYBOARD_OR_FLOAT);
	xkb = master->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		XkbAction *act;
		unsigned char key_mask;

		if (!key_is_down(master, (int) key, KEY_PROCESSED))
			continue;

		act = XkbKeyActionPtr(xkb, key, state);
		if (act == NULL)
			continue;

		if (act->type != XkbSA_SetMods)
			continue;

		if (act->mods.flags & XkbSA_UseModMapMods)
			key_mask = xkb->map->modmap[key];
		else
			key_mask = act->mods.mask;

		if (!(key_mask & mask))
			continue;

		if (count >= maxKeys)
			return 0;

		keys[count++] = key;
	}

	return count;
}

KeyCode lorieKeysymToKeycode(KeySym keysym, unsigned state, unsigned *new_state) {
	XkbDescPtr xkb;
	unsigned int key; // KeyCode has insufficient range for the loop
	KeyCode fallback;
	KeySym ks;
	unsigned level_three_mask;

	if (new_state != NULL)
		*new_state = state;

	fallback = 0;
	xkb = GetMaster(lorieKeyboard, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		unsigned int state_out;
		KeySym dummy;
		size_t fakeIdx;

		XkbTranslateKeyCode(xkb, key, state, &state_out, &ks);
		if (ks == NoSymbol)
			continue;

		/*
		 * Despite every known piece of documentation on
		 * XkbTranslateKeyCode() stating that mods_rtrn returns
		 * the unconsumed modifiers, in reality it always
		 * returns the _potentially consumed_ modifiers.
		 */
		state_out = state & ~state_out;
		if (state_out & LockMask)
			XkbConvertCase(ks, &dummy, &ks);

		if (ks != keysym)
			continue;

		/*
		 * Some keys are never sent by a real keyboard and are
		 * used in the default layouts as a fallback for
		 * modifiers. Make sure we use them last as some
		 * applications can be confused by these normally
		 * unused keys.
		 */
		for (fakeIdx = 0; fakeIdx < ARRAY_SIZE(fakeKeys); fakeIdx++) {
			if (key == fakeKeys[fakeIdx]) {
				if (fallback == 0)
					fallback = key;
				break;
			}
		}
		if (fakeIdx < ARRAY_SIZE(fakeKeys))
			continue;

		return key;
	}

	/* Use the fallback key, if one was found */
	if (fallback != 0)
		return fallback;

	if (new_state == NULL)
		return 0;

	*new_state = (state & ~ShiftMask) | ((state & ShiftMask) ? 0 : ShiftMask);
	key = lorieKeysymToKeycode(keysym, *new_state, NULL);
	if (key != 0)
		return key;

	level_three_mask = lorieGetLevelThreeMask();
	if (level_three_mask == 0)
		return 0;

	*new_state = (state & ~level_three_mask) | 
	             ((state & level_three_mask) ? 0 : level_three_mask);
	key = lorieKeysymToKeycode(keysym, *new_state, NULL);
	if (key != 0)
		return key;

	*new_state = (state & ~(ShiftMask | level_three_mask)) | 
	             ((state & ShiftMask) ? 0 : ShiftMask) |
	             ((state & level_three_mask) ? 0 : level_three_mask);
	key = lorieKeysymToKeycode(keysym, *new_state, NULL);
	if (key != 0)
		return key;

	return 0;
}

static int lorieIsAffectedByNumLock(KeyCode keycode) {
	unsigned state;

	KeyCode numlock_keycode;
	unsigned numlock_mask;

	XkbDescPtr xkb;
	XkbAction *act;

	XkbKeyTypeRec *type;

	/* Group state is still important */
	state = lorieGetKeyboardState();
	state &= ~0xff;

	/*
	 * Not sure if hunting for a virtual modifier called "NumLock",
	 * or following the keysym Num_Lock is the best approach. We
	 * try the latter.
	 */
	numlock_keycode = lorieKeysymToKeycode(XK_Num_Lock, state, NULL);
	if (numlock_keycode == 0)
		return 0;

	xkb = GetMaster(lorieKeyboard, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;

	act = XkbKeyActionPtr(xkb, numlock_keycode, state);
	if (act == NULL)
		return 0;
	if (act->type != XkbSA_LockMods)
		return 0;

	if (act->mods.flags & XkbSA_UseModMapMods)
		numlock_mask = xkb->map->modmap[keycode];
	else
		numlock_mask = act->mods.mask;

	type = XkbKeyKeyType(xkb, keycode, XkbKeyEffectiveGroup(xkb, keycode, state));
	if ((type->mods.mask & numlock_mask) == 0)
		return 0;

	return 1;
}

static KeyCode lorieAddKeysym(KeySym keysym, unused unsigned state) {
	DeviceIntPtr master;
	XkbDescPtr xkb;
	unsigned int key;

	XkbEventCauseRec cause;
	XkbChangesRec changes;

	int types[1];
	KeySym *syms;
	KeySym upper, lower;

	master = GetMaster(lorieKeyboard, KEYBOARD_OR_FLOAT);
	xkb = master->key->xkbInfo->desc;

	static int curFakeKeyIdx = 0;
	key = fakeKeys[curFakeKeyIdx++];
	if (curFakeKeyIdx >= ARRAY_SIZE(fakeKeys))
		curFakeKeyIdx = 0;

	memset(&changes, 0, sizeof(changes));
	memset(&cause, 0, sizeof(cause));

	XkbSetCauseUnknown(&cause)

	/*
	 * Tools like xkbcomp get confused if there isn't a name
	 * assigned to the keycode we're trying to use.
	 */
	if (xkb->names && xkb->names->keys &&
	    (xkb->names->keys[key].name[0] == '\0')) {
		xkb->names->keys[key].name[0] = 'I';
		xkb->names->keys[key].name[1] = '0' + (key / 100) % 10;
		xkb->names->keys[key].name[2] = '0' + (key /  10) % 10;
		xkb->names->keys[key].name[3] = '0' + (key /   1) % 10;

		changes.names.changed |= XkbKeyNamesMask;
		changes.names.first_key = key;
		changes.names.num_keys = 1;
	}

	XkbConvertCase(keysym, &lower, &upper);
	types[XkbGroup1Index] = XkbAlphabeticIndex;

	XkbChangeTypesOfKey(xkb, (int) key, 1, XkbGroup1Mask, types, &changes.map);

	syms = XkbKeySymsPtr(xkb, key);
	syms[0] = lower;
	syms[1] = upper;

	changes.map.changed |= XkbKeySymsMask;
	changes.map.first_key_sym = key;
	changes.map.num_key_syms = 1;

	XkbSendNotification(master, &changes, &cause);

	return key;
}

/*
 * lorieKeysymKeyboardEvent() - work out the best keycode corresponding
 * to the keysym sent by the viewer. This is basically impossible in
 * the general case, but we make a best effort by assuming that all
 * useful keysyms can be reached using just the Shift and
 * Level 3 (AltGr) modifiers. For core keyboards this is basically
 * always true, and should be true for most sane, western XKB layouts.
 */
void lorieKeysymKeyboardEvent(KeySym keysym, int down) {
    int i;
    unsigned state, new_state;
    KeyCode keycode;

    unsigned level_three_mask;
    KeyCode shift_press, level_three_press;
    KeyCode shift_release[8], level_three_release[8];
    size_t shift_release_count, level_three_release_count;

    /*
     * Release events must match the press event, so look up what
     * keycode we sent for the press.
     */
    if (!down) {
        for (i = 0;i < 256;i++) {
            if (pressedKeys[i] == keysym) {
                pressedKeys[i] = NoSymbol;
                QueueKeyboardEvents(lorieKeyboard, KeyRelease, i); // "keycode"
                mieqProcessInputEvents();
                return;
            }
        }

        /*
         * This can happen quite often as we ignore some
         * key presses.
         */
        LogMessageVerb(X_DEBUG, -1, "Unexpected release of keysym 0x%x\n", keysym);
        return;
    }

    /*
     * Since we are checking the current state to determine if we need
     * to fake modifiers, we must make sure that everything put on the
     * input queue is processed before we start. Otherwise, shift may be
     * stuck down.
     */
    mieqProcessInputEvents();

    state = lorieGetKeyboardState();

    keycode = lorieKeysymToKeycode(keysym, state, &new_state);

    /* Try some equivalent keysyms if we couldn't find a perfect match */
    if (keycode == 0) {
        for (i = 0;i < sizeof(altKeysym)/sizeof(altKeysym[0]);i++) {
            KeySym altsym;

            if (altKeysym[i].a == keysym)
                altsym = altKeysym[i].b;
            else if (altKeysym[i].b == keysym)
                altsym = altKeysym[i].a;
            else
                continue;

            keycode = lorieKeysymToKeycode(altsym, state, &new_state);
            if (keycode != 0)
                break;
        }
    }

    /* No matches. Will have to add a new entry... */
    if (keycode == 0) {
        keycode = lorieAddKeysym(keysym, state);
        if (keycode == 0) {
                LogMessageVerb(X_ERROR, -1, "Failure adding new keysym 0x%x\n", keysym);
            return;
        }

        LogMessageVerb(X_INFO, 0, "Added unknown keysym 0x%x to keycode %d\n",
                 keysym, keycode);

        /*
         * The state given to addKeysym() is just a hint and
         * the actual result might still require some state
         * changes.
         */
        keycode = lorieKeysymToKeycode(keysym, state, &new_state);
        if (keycode == 0) {
            LogMessageVerb(X_ERROR, -1, "Newly added keysym 0x%x cannot be generated\n", keysym);
            return;
        }
    }

    /*
     * X11 generally lets shift toggle the keys on the numeric pad
     * the same way NumLock does. This is however not the case on
     * other systems like Windows. As a result, some applications
     * get confused when we do a fake shift to get the same effect
     * that having NumLock active would produce.
     *
     * Not all clients have proper NumLock synchronisation (so we
     * can avoid faking shift) so we try to avoid the fake shifts
     * if we can use an alternative keysym.
     */
    if (((state & ShiftMask) != (new_state & ShiftMask)) && lorieIsAffectedByNumLock(keycode)) {
        KeyCode keycode2;
        unsigned new_state2;

        LogMessageVerb(X_DEBUG, 0, "Finding alternative to keysym 0x%x to avoid fake shift for numpad\n", keysym);

        for (i = 0;i < sizeof(altKeysym)/sizeof(altKeysym[0]);i++) {
            KeySym altsym;

            if (altKeysym[i].a == keysym)
                altsym = altKeysym[i].b;
            else if (altKeysym[i].b == keysym)
                altsym = altKeysym[i].a;
            else
                continue;

            keycode2 = lorieKeysymToKeycode(altsym, state, &new_state2);
            if (keycode2 == 0)
                continue;

            if (((state & ShiftMask) != (new_state2 & ShiftMask)) &&
					lorieIsAffectedByNumLock(keycode2))
                continue;

            break;
        }

        if (i == sizeof(altKeysym)/sizeof(altKeysym[0]))
            LogMessageVerb(X_DEBUG, 0, "No alternative keysym found\n");
        else {
            keycode = keycode2;
            new_state = new_state2;
        }
    }

    /*
     * "Shifted Tab" is a bit of a mess. Some systems have varying,
     * special keysyms for this symbol. VNC mandates that clients
     * should always send the plain XK_Tab keysym and the server
     * should deduce the meaning based on current Shift state.
     * To comply with this, we will find the keycode that sends
     * XK_Tab, and make sure that Shift isn't cleared. This can
     * possibly result in a different keysym than XK_Tab, but that
     * is the desired behaviour.
     *
     * Note: We never get ISO_Left_Tab here because it's already
     *       been translated in VNCSConnectionST.
     */
    if (keysym == XK_Tab && (state & ShiftMask))
        new_state |= ShiftMask;

    /*
     * We need a bigger state change than just shift,
     * so we need to know what the mask is for level 3 shifts.
     */
    if ((new_state & ~ShiftMask) != (state & ~ShiftMask))
        level_three_mask = lorieGetLevelThreeMask();
    else
        level_three_mask = 0;

    shift_press = level_three_press = 0;
    shift_release_count = level_three_release_count = 0;

    /* Need a fake press or release of shift? */
    if (!(state & ShiftMask) && (new_state & ShiftMask)) {
        shift_press = loriePressShift();
        if (shift_press == 0) {
            LogMessageVerb(X_ERROR, -1, "Unable to find a modifier key for Shift\n");
            return;
        }

        QueueKeyboardEvents(lorieKeyboard, KeyPress, shift_press); // "temp shift"
    } else if ((state & ShiftMask) && !(new_state & ShiftMask)) {
        shift_release_count = lorieReleaseShift(shift_release,
                                              sizeof(shift_release)/sizeof(*shift_release));
        if (shift_release_count == 0) {
            LogMessageVerb(X_ERROR, -1, "Unable to find the modifier key(s) for releasing Shift\n");
            return;
        }

        for (i = 0;i < shift_release_count;i++)
            QueueKeyboardEvents(lorieKeyboard, KeyRelease, shift_release[i]); // "temp shift"
    }

    /* Need a fake press or release of level three shift? */
    if (!(state & level_three_mask) && (new_state & level_three_mask)) {
        level_three_press = loriePressLevelThree();
        if (level_three_press == 0) {
            LogMessageVerb(X_ERROR, -1, "Unable to find a modifier key for ISO_Level3_Shift/Mode_Switch\n");
            return;
        }

        QueueKeyboardEvents(lorieKeyboard, KeyPress, level_three_press); // "temp level 3 shift"
    } else if ((state & level_three_mask) && !(new_state & level_three_mask)) {
        level_three_release_count = lorieReleaseLevelThree(level_three_release,
                                                         sizeof(level_three_release)/sizeof(*level_three_release));
        if (level_three_release_count == 0) {
            LogMessageVerb(X_ERROR, -1, "Unable to find the modifier key(s) for releasing ISO_Level3_Shift/Mode_Switch\n");
            return;
        }

        for (i = 0;i < level_three_release_count;i++)
            QueueKeyboardEvents(lorieKeyboard, KeyRelease, level_three_release[i]); // "temp level 3 shift"
    }

    /* Now press the actual key */
    QueueKeyboardEvents(lorieKeyboard, KeyPress, keycode); // "keycode"

    /* And store the mapping so that we can do a proper release later */
    for (i = 0;i < 256;i++) {
        if (i == keycode)
            continue;
        if (pressedKeys[i] == keysym) {
            LogMessageVerb(X_ERROR, -1, "Keysym 0x%x generated by both keys %d and %d", keysym, i, keycode);
            pressedKeys[i] = NoSymbol;
        }
    }

    pressedKeys[keycode] = keysym;

    /* Undo any fake level three shift */
    if (level_three_press != 0)
        QueueKeyboardEvents(lorieKeyboard, KeyRelease, level_three_press); // "temp level 3 shift"
    else if (level_three_release_count != 0) {
        for (i = 0;i < level_three_release_count;i++)
			QueueKeyboardEvents(lorieKeyboard, KeyPress, level_three_release[i]); // "temp level 3 shift"
    }

    /* Undo any fake shift */
    if (shift_press != 0)
		QueueKeyboardEvents(lorieKeyboard, KeyRelease, shift_press); // "temp shift"
    else if (shift_release_count != 0) {
        for (i = 0;i < shift_release_count;i++)
			QueueKeyboardEvents(lorieKeyboard, KeyPress, shift_release[i]); // "temp shift"
    }

    /*
     * When faking a modifier we are putting a keycode (which can
     * currently activate the desired modifier) on the input
     * queue. A future modmap change can change the mapping so
     * that this keycode means something else entirely. Guard
     * against this by processing the queue now.
     */
    mieqProcessInputEvents();
}
