/*
   quartzKeyboard.c: Keyboard support for Xquartz

   Copyright (c) 2003-2012 Apple Inc.
   Copyright (c) 2001-2004 Torrey T. Lyons. All Rights Reserved.
   Copyright 2004 Kaleb S. KEITHLEY. All Rights Reserved.

   Copyright (C) 1999,2000 by Eric Sunshine <sunshine@sunshineco.com>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
     3. The name of the author may not be used to endorse or promote products
        derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
   NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sanitizedCarbon.h"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#define HACK_MISSING   1
#define HACK_KEYPAD    1
#define HACK_BLACKLIST 1

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "quartz.h"
#include "darwin.h"
#include "darwinEvents.h"

#include "quartzKeyboard.h"

#include "X11Application.h"

#include <assert.h>
#include <pthread.h>

#include "xkbsrv.h"
#include "exevents.h"
#include "X11/keysym.h"
#include "keysym2ucs.h"

extern void
CopyKeyClass(DeviceIntPtr device, DeviceIntPtr master);

enum {
    MOD_COMMAND = 256,
    MOD_SHIFT = 512,
    MOD_OPTION = 2048,
    MOD_CONTROL = 4096,
};

#define UKEYSYM(u) ((u) | 0x01000000)

#if HACK_MISSING
/* Table of keycode->keysym mappings we use to fallback on for important
   keys that are often not in the Unicode mapping. */

const static struct {
    unsigned short keycode;
    KeySym keysym;
} known_keys[] = {
    { 55,  XK_Meta_L        },
    { 56,  XK_Shift_L       },
    { 57,  XK_Caps_Lock     },
    { 58,  XK_Alt_L         },
    { 59,  XK_Control_L     },

    { 60,  XK_Shift_R       },
    { 61,  XK_Alt_R         },
    { 62,  XK_Control_R     },
    { 63,  XK_Meta_R        },

    { 110, XK_Menu          },

    { 122, XK_F1            },
    { 120, XK_F2            },
    { 99,  XK_F3            },
    { 118, XK_F4            },
    { 96,  XK_F5            },
    { 97,  XK_F6            },
    { 98,  XK_F7            },
    { 100, XK_F8            },
    { 101, XK_F9            },
    { 109, XK_F10           },
    { 103, XK_F11           },
    { 111, XK_F12           },
    { 105, XK_F13           },
    { 107, XK_F14           },
    { 113, XK_F15           },
    { 106, XK_F16           },
    { 64,  XK_F17           },
    { 79,  XK_F18           },
    { 80,  XK_F19           },
    { 90,  XK_F20           },
};
#endif

#if HACK_KEYPAD
/* Table of keycode->old,new-keysym mappings we use to fixup the numeric
   keypad entries. */

const static struct {
    unsigned short keycode;
    KeySym normal, keypad;
} known_numeric_keys[] = {
    { 65, XK_period,   XK_KP_Decimal                              },
    { 67, XK_asterisk, XK_KP_Multiply                             },
    { 69, XK_plus,     XK_KP_Add                                  },
    { 75, XK_slash,    XK_KP_Divide                               },
    { 76, 0x01000003,  XK_KP_Enter                                },
    { 78, XK_minus,    XK_KP_Subtract                             },
    { 81, XK_equal,    XK_KP_Equal                                },
    { 82, XK_0,        XK_KP_0                                    },
    { 83, XK_1,        XK_KP_1                                    },
    { 84, XK_2,        XK_KP_2                                    },
    { 85, XK_3,        XK_KP_3                                    },
    { 86, XK_4,        XK_KP_4                                    },
    { 87, XK_5,        XK_KP_5                                    },
    { 88, XK_6,        XK_KP_6                                    },
    { 89, XK_7,        XK_KP_7                                    },
    { 91, XK_8,        XK_KP_8                                    },
    { 92, XK_9,        XK_KP_9                                    },
};
#endif

#if HACK_BLACKLIST
/* <rdar://problem/7824370> wine notepad produces wrong characters on shift+arrow
 * http://xquartz.macosforge.org/trac/ticket/295
 * http://developer.apple.com/legacy/mac/library/documentation/mac/Text/Text-579.html
 *
 * legacy Mac keycodes for arrow keys that shift-modify to math symbols
 */
const static unsigned short keycode_blacklist[] = { 66, 70, 72, 77 };
#endif

/* Table mapping normal keysyms to their dead equivalents.
   FIXME: all the unicode keysyms (apart from circumflex) were guessed. */

const static struct {
    KeySym normal, dead;
} dead_keys[] = {
    { XK_grave,       XK_dead_grave                                },
    { XK_apostrophe,  XK_dead_acute                                }, /* US:"=" on a Czech keyboard */
    { XK_acute,       XK_dead_acute                                },
    { UKEYSYM(0x384), XK_dead_acute                                }, /* US:";" on a Greek keyboard */
    //    {XK_Greek_accentdieresis, XK_dead_diaeresis},   /* US:"opt+;" on a Greek keyboard ... replace with dead_accentdieresis if there is one */
    { XK_asciicircum, XK_dead_circumflex                           },
    { UKEYSYM(0x2c6), XK_dead_circumflex                           }, /* MODIFIER LETTER CIRCUMFLEX ACCENT */
    { XK_asciitilde,  XK_dead_tilde                                },
    { UKEYSYM(0x2dc), XK_dead_tilde                                }, /* SMALL TILDE */
    { XK_macron,      XK_dead_macron                               },
    { XK_breve,       XK_dead_breve                                },
    { XK_abovedot,    XK_dead_abovedot                             },
    { XK_diaeresis,   XK_dead_diaeresis                            },
    { UKEYSYM(0x2da), XK_dead_abovering                            }, /* DOT ABOVE */
    { XK_doubleacute, XK_dead_doubleacute                          },
    { XK_caron,       XK_dead_caron                                },
    { XK_cedilla,     XK_dead_cedilla                              },
    { XK_ogonek,      XK_dead_ogonek                               },
    { UKEYSYM(0x269), XK_dead_iota                                 }, /* LATIN SMALL LETTER IOTA */
    { UKEYSYM(0x2ec), XK_dead_voiced_sound                         }, /* MODIFIER LETTER VOICING */
    /*  {XK_semivoiced_sound, XK_dead_semivoiced_sound}, */
    { UKEYSYM(0x323), XK_dead_belowdot                             }, /* COMBINING DOT BELOW */
    { UKEYSYM(0x309), XK_dead_hook                                 }, /* COMBINING HOOK ABOVE */
    { UKEYSYM(0x31b), XK_dead_horn                                 }, /* COMBINING HORN */
};

typedef struct darwinKeyboardInfo_struct {
    CARD8 modMap[MAP_LENGTH];
    KeySym keyMap[MAP_LENGTH * GLYPHS_PER_KEY];
    unsigned char modifierKeycodes[32][2];
} darwinKeyboardInfo;

darwinKeyboardInfo keyInfo;
pthread_mutex_t keyInfo_mutex = PTHREAD_MUTEX_INITIALIZER;

static void
DarwinChangeKeyboardControl(DeviceIntPtr device, KeybdCtrl *ctrl)
{
    // FIXME: to be implemented
    // keyclick, bell volume / pitch, autorepead, LED's
}

//-----------------------------------------------------------------------------
// Utility functions to help parse Darwin keymap
//-----------------------------------------------------------------------------

/*
 * DarwinBuildModifierMaps
 *      Use the keyMap field of keyboard info structure to populate
 *      the modMap and modifierKeycodes fields.
 */
static void
DarwinBuildModifierMaps(darwinKeyboardInfo *info)
{
    int i;
    KeySym *k;

    memset(info->modMap, NoSymbol, sizeof(info->modMap));
    memset(info->modifierKeycodes, 0, sizeof(info->modifierKeycodes));

    for (i = 0; i < NUM_KEYCODES; i++) {
        k = info->keyMap + i * GLYPHS_PER_KEY;

        switch (*k) {
        case XK_Shift_L:
            info->modifierKeycodes[NX_MODIFIERKEY_SHIFT][0] = i;
            info->modMap[MIN_KEYCODE + i] = ShiftMask;
            break;

        case XK_Shift_R:
#ifdef NX_MODIFIERKEY_RSHIFT
            info->modifierKeycodes[NX_MODIFIERKEY_RSHIFT][0] = i;
#else
            info->modifierKeycodes[NX_MODIFIERKEY_SHIFT][0] = i;
#endif
            info->modMap[MIN_KEYCODE + i] = ShiftMask;
            break;

        case XK_Control_L:
            info->modifierKeycodes[NX_MODIFIERKEY_CONTROL][0] = i;
            info->modMap[MIN_KEYCODE + i] = ControlMask;
            break;

        case XK_Control_R:
#ifdef NX_MODIFIERKEY_RCONTROL
            info->modifierKeycodes[NX_MODIFIERKEY_RCONTROL][0] = i;
#else
            info->modifierKeycodes[NX_MODIFIERKEY_CONTROL][0] = i;
#endif
            info->modMap[MIN_KEYCODE + i] = ControlMask;
            break;

        case XK_Caps_Lock:
            info->modifierKeycodes[NX_MODIFIERKEY_ALPHALOCK][0] = i;
            info->modMap[MIN_KEYCODE + i] = LockMask;
            break;

        case XK_Alt_L:
            info->modifierKeycodes[NX_MODIFIERKEY_ALTERNATE][0] = i;
            info->modMap[MIN_KEYCODE + i] = Mod1Mask;
            if (!XQuartzOptionSendsAlt)
                *k = XK_Mode_switch;     // Yes, this is ugly.  This needs to be cleaned up when we integrate quartzKeyboard with this code and refactor.
            break;

        case XK_Alt_R:
#ifdef NX_MODIFIERKEY_RALTERNATE
            info->modifierKeycodes[NX_MODIFIERKEY_RALTERNATE][0] = i;
#else
            info->modifierKeycodes[NX_MODIFIERKEY_ALTERNATE][0] = i;
#endif
            if (!XQuartzOptionSendsAlt)
                *k = XK_Mode_switch;     // Yes, this is ugly.  This needs to be cleaned up when we integrate quartzKeyboard with this code and refactor.
            info->modMap[MIN_KEYCODE + i] = Mod1Mask;
            break;

        case XK_Mode_switch:
            ErrorF(
                "DarwinBuildModifierMaps: XK_Mode_switch encountered, unable to determine side.\n");
            info->modifierKeycodes[NX_MODIFIERKEY_ALTERNATE][0] = i;
#ifdef NX_MODIFIERKEY_RALTERNATE
            info->modifierKeycodes[NX_MODIFIERKEY_RALTERNATE][0] = i;
#endif
            info->modMap[MIN_KEYCODE + i] = Mod1Mask;
            break;

        case XK_Meta_L:
            info->modifierKeycodes[NX_MODIFIERKEY_COMMAND][0] = i;
            info->modMap[MIN_KEYCODE + i] = Mod2Mask;
            break;

        case XK_Meta_R:
#ifdef NX_MODIFIERKEY_RCOMMAND
            info->modifierKeycodes[NX_MODIFIERKEY_RCOMMAND][0] = i;
#else
            info->modifierKeycodes[NX_MODIFIERKEY_COMMAND][0] = i;
#endif
            info->modMap[MIN_KEYCODE + i] = Mod2Mask;
            break;

        case XK_Num_Lock:
            info->modMap[MIN_KEYCODE + i] = Mod3Mask;
            break;
        }
    }
}

/*
 * DarwinKeyboardInit
 *      Get the Darwin keyboard map and compute an equivalent
 *      X keyboard map and modifier map. Set the new keyboard
 *      device structure.
 */
void
DarwinKeyboardInit(DeviceIntPtr pDev)
{
    // Open a shared connection to the HID System.
    // Note that the Event Status Driver is really just a wrapper
    // for a kIOHIDParamConnectType connection.
    assert(darwinParamConnect = NXOpenEventStatus());

    InitKeyboardDeviceStruct(pDev, NULL, NULL, DarwinChangeKeyboardControl);

    DarwinKeyboardReloadHandler();

    CopyKeyClass(pDev, inputInfo.keyboard);
}

/* Set the repeat rates based on global preferences and keycodes for modifiers.
 * Precondition: Has the keyInfo_mutex lock.
 */
static void
DarwinKeyboardSetRepeat(DeviceIntPtr pDev, int initialKeyRepeatValue,
                        int keyRepeatValue)
{
    if (initialKeyRepeatValue == 300000) { // off
        /* Turn off repeats globally */
        XkbSetRepeatKeys(pDev, -1, AutoRepeatModeOff);
    }
    else {
        int i;
        XkbControlsPtr ctrl;
        XkbControlsRec old;

        /* Turn on repeats globally */
        XkbSetRepeatKeys(pDev, -1, AutoRepeatModeOn);

        /* Setup the bit mask for individual key repeats */
        ctrl = pDev->key->xkbInfo->desc->ctrls;
        old = *ctrl;

        ctrl->repeat_delay = initialKeyRepeatValue * 15;
        ctrl->repeat_interval = keyRepeatValue * 15;

        /* Turn off key-repeat for modifier keys, on for others */
        /* First set them all on */
        for (i = 0; i < XkbPerKeyBitArraySize; i++)
            ctrl->per_key_repeat[i] = -1;

        /* Now turn off the modifiers */
        for (i = 0; i < 32; i++) {
            unsigned char keycode;

            keycode = keyInfo.modifierKeycodes[i][0];
            if (keycode)
                ClearBit(ctrl->per_key_repeat, keycode + MIN_KEYCODE);

            keycode = keyInfo.modifierKeycodes[i][1];
            if (keycode)
                ClearBit(ctrl->per_key_repeat, keycode + MIN_KEYCODE);
        }

        /* Hurray for data duplication */
        if (pDev->kbdfeed)
            memcpy(pDev->kbdfeed->ctrl.autoRepeats, ctrl->per_key_repeat,
                   XkbPerKeyBitArraySize);

        //ErrorF("per_key_repeat =\n");
        //for(i=0; i < XkbPerKeyBitArraySize; i++)
        //    ErrorF("%02x%s", ctrl->per_key_repeat[i], (i + 1) & 7 ? "" : "\n");

        /* And now we notify the puppies about the changes */
        XkbDDXChangeControls(pDev, &old, ctrl);
    }
}

void
DarwinKeyboardReloadHandler(void)
{
    KeySymsRec keySyms;
    CFIndex initialKeyRepeatValue, keyRepeatValue;
    BOOL ok;
    DeviceIntPtr pDev;
    const char *xmodmap = PROJECTROOT "/bin/xmodmap";
    const char *sysmodmap = PROJECTROOT "/lib/X11/xinit/.Xmodmap";
    const char *homedir = getenv("HOME");
    char usermodmap[PATH_MAX], cmd[PATH_MAX];

    DEBUG_LOG("DarwinKeyboardReloadHandler\n");

    /* Get our key repeat settings from GlobalPreferences */
    (void)CFPreferencesAppSynchronize(CFSTR(".GlobalPreferences"));

    initialKeyRepeatValue =
        CFPreferencesGetAppIntegerValue(CFSTR("InitialKeyRepeat"),
                                        CFSTR(".GlobalPreferences"), &ok);
    if (!ok)
        initialKeyRepeatValue = 35;

    keyRepeatValue = CFPreferencesGetAppIntegerValue(CFSTR(
                                                         "KeyRepeat"),
                                                     CFSTR(
                                                         ".GlobalPreferences"),
                                                     &ok);
    if (!ok)
        keyRepeatValue = 6;

    pthread_mutex_lock(&keyInfo_mutex);
    {
        /* Initialize our keySyms */
        keySyms.map = keyInfo.keyMap;
        keySyms.mapWidth = GLYPHS_PER_KEY;
        keySyms.minKeyCode = MIN_KEYCODE;
        keySyms.maxKeyCode = MAX_KEYCODE;

        // TODO: We should build the entire XkbDescRec and use XkbCopyKeymap
        /* Apply the mappings to darwinKeyboard */
        XkbApplyMappingChange(darwinKeyboard, &keySyms, keySyms.minKeyCode,
                              keySyms.maxKeyCode - keySyms.minKeyCode + 1,
                              keyInfo.modMap, serverClient);
        DarwinKeyboardSetRepeat(darwinKeyboard, initialKeyRepeatValue,
                                keyRepeatValue);

        /* Apply the mappings to the core keyboard */
        for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
            if ((pDev->coreEvents ||
                 pDev == inputInfo.keyboard) && pDev->key) {
                XkbApplyMappingChange(
                    pDev, &keySyms, keySyms.minKeyCode,
                    keySyms.maxKeyCode -
                    keySyms.minKeyCode + 1,
                    keyInfo.modMap, serverClient);
                DarwinKeyboardSetRepeat(pDev, initialKeyRepeatValue,
                                        keyRepeatValue);
            }
        }
    } pthread_mutex_unlock(&keyInfo_mutex);

    /* Modify with xmodmap */
    if (access(xmodmap, F_OK) == 0) {
        /* Check for system .Xmodmap */
        if (access(sysmodmap, F_OK) == 0) {
            if (snprintf(cmd, sizeof(cmd), "%s %s", xmodmap,
                         sysmodmap) < sizeof(cmd)) {
                X11ApplicationLaunchClient(cmd);
            }
            else {
                ErrorF(
                    "X11.app: Unable to create / execute xmodmap command line");
            }
        }

        /* Check for user's local .Xmodmap */
        if ((homedir != NULL) &&
            (snprintf(usermodmap, sizeof(usermodmap), "%s/.Xmodmap",
                      homedir) < sizeof(usermodmap))) {
            if (access(usermodmap, F_OK) == 0) {
                if (snprintf(cmd, sizeof(cmd), "%s %s", xmodmap,
                             usermodmap) < sizeof(cmd)) {
                    X11ApplicationLaunchClient(cmd);
                }
                else {
                    ErrorF(
                        "X11.app: Unable to create / execute xmodmap command line");
                }
            }
        }
        else {
            ErrorF("X11.app: Unable to determine path to user's .Xmodmap");
        }
    }
}

//-----------------------------------------------------------------------------
// Modifier translation functions
//
// There are three different ways to specify a Mac modifier key:
// keycode - specifies hardware key, read from keymapping
// key     - NX_MODIFIERKEY_*, really an index
// mask    - NX_*MASK, mask for modifier flags in event record
// Left and right side have different keycodes but the same key and mask.
//-----------------------------------------------------------------------------

/*
 * DarwinModifierNXKeyToNXKeycode
 *      Return the keycode for an NX_MODIFIERKEY_* modifier.
 *      side = 0 for left or 1 for right.
 *      Returns 0 if key+side is not a known modifier.
 */
int
DarwinModifierNXKeyToNXKeycode(int key, int side)
{
    int retval;
    pthread_mutex_lock(&keyInfo_mutex);
    retval = keyInfo.modifierKeycodes[key][side];
    pthread_mutex_unlock(&keyInfo_mutex);

    return retval;
}

/*
 * DarwinModifierNXKeycodeToNXKey
 *      Returns -1 if keycode+side is not a modifier key
 *      outSide may be NULL, else it gets 0 for left and 1 for right.
 */
int
DarwinModifierNXKeycodeToNXKey(unsigned char keycode, int *outSide)
{
    int key, side;

    keycode += MIN_KEYCODE;

    // search modifierKeycodes for this keycode+side
    pthread_mutex_lock(&keyInfo_mutex);
    for (key = 0; key < NX_NUMMODIFIERS; key++) {
        for (side = 0; side <= 1; side++) {
            if (keyInfo.modifierKeycodes[key][side] == keycode) break;
        }
    }
    pthread_mutex_unlock(&keyInfo_mutex);

    if (key == NX_NUMMODIFIERS) {
        return -1;
    }
    if (outSide) *outSide = side;

    return key;
}

/*
 * DarwinModifierNXMaskToNXKey
 *      Returns -1 if mask is not a known modifier mask.
 */
int
DarwinModifierNXMaskToNXKey(int mask)
{
    switch (mask) {
    case NX_ALPHASHIFTMASK:
        return NX_MODIFIERKEY_ALPHALOCK;

    case NX_SHIFTMASK:
        return NX_MODIFIERKEY_SHIFT;

#ifdef NX_DEVICELSHIFTKEYMASK
    case NX_DEVICELSHIFTKEYMASK:
        return NX_MODIFIERKEY_SHIFT;

    case NX_DEVICERSHIFTKEYMASK:
        return NX_MODIFIERKEY_RSHIFT;

#endif
    case NX_CONTROLMASK:
        return NX_MODIFIERKEY_CONTROL;

#ifdef NX_DEVICELCTLKEYMASK
    case NX_DEVICELCTLKEYMASK:
        return NX_MODIFIERKEY_CONTROL;

    case NX_DEVICERCTLKEYMASK:
        return NX_MODIFIERKEY_RCONTROL;

#endif
    case NX_ALTERNATEMASK:
        return NX_MODIFIERKEY_ALTERNATE;

#ifdef NX_DEVICELALTKEYMASK
    case NX_DEVICELALTKEYMASK:
        return NX_MODIFIERKEY_ALTERNATE;

    case NX_DEVICERALTKEYMASK:
        return NX_MODIFIERKEY_RALTERNATE;

#endif
    case NX_COMMANDMASK:
        return NX_MODIFIERKEY_COMMAND;

#ifdef NX_DEVICELCMDKEYMASK
    case NX_DEVICELCMDKEYMASK:
        return NX_MODIFIERKEY_COMMAND;

    case NX_DEVICERCMDKEYMASK:
        return NX_MODIFIERKEY_RCOMMAND;

#endif
    case NX_NUMERICPADMASK:
        return NX_MODIFIERKEY_NUMERICPAD;

    case NX_HELPMASK:
        return NX_MODIFIERKEY_HELP;

    case NX_SECONDARYFNMASK:
        return NX_MODIFIERKEY_SECONDARYFN;
    }
    return -1;
}

/*
 * DarwinModifierNXKeyToNXMask
 *      Returns 0 if key is not a known modifier key.
 */
int
DarwinModifierNXKeyToNXMask(int key)
{
    switch (key) {
    case NX_MODIFIERKEY_ALPHALOCK:
        return NX_ALPHASHIFTMASK;

#ifdef NX_DEVICELSHIFTKEYMASK
    case NX_MODIFIERKEY_SHIFT:
        return NX_DEVICELSHIFTKEYMASK;

    case NX_MODIFIERKEY_RSHIFT:
        return NX_DEVICERSHIFTKEYMASK;

    case NX_MODIFIERKEY_CONTROL:
        return NX_DEVICELCTLKEYMASK;

    case NX_MODIFIERKEY_RCONTROL:
        return NX_DEVICERCTLKEYMASK;

    case NX_MODIFIERKEY_ALTERNATE:
        return NX_DEVICELALTKEYMASK;

    case NX_MODIFIERKEY_RALTERNATE:
        return NX_DEVICERALTKEYMASK;

    case NX_MODIFIERKEY_COMMAND:
        return NX_DEVICELCMDKEYMASK;

    case NX_MODIFIERKEY_RCOMMAND:
        return NX_DEVICERCMDKEYMASK;

#else
    case NX_MODIFIERKEY_SHIFT:
        return NX_SHIFTMASK;

    case NX_MODIFIERKEY_CONTROL:
        return NX_CONTROLMASK;

    case NX_MODIFIERKEY_ALTERNATE:
        return NX_ALTERNATEMASK;

    case NX_MODIFIERKEY_COMMAND:
        return NX_COMMANDMASK;

#endif
    case NX_MODIFIERKEY_NUMERICPAD:
        return NX_NUMERICPADMASK;

    case NX_MODIFIERKEY_HELP:
        return NX_HELPMASK;

    case NX_MODIFIERKEY_SECONDARYFN:
        return NX_SECONDARYFNMASK;
    }
    return 0;
}

/*
 * DarwinModifierStringToNXMask
 *      Returns 0 if string is not a known modifier.
 */
int
DarwinModifierStringToNXMask(const char *str, int separatelr)
{
#ifdef NX_DEVICELSHIFTKEYMASK
    if (separatelr) {
        if (!strcasecmp(str,
                        "shift")) return NX_DEVICELSHIFTKEYMASK |
                   NX_DEVICERSHIFTKEYMASK;
        if (!strcasecmp(str,
                        "control")) return NX_DEVICELCTLKEYMASK |
                   NX_DEVICERCTLKEYMASK;
        if (!strcasecmp(str,
                        "option")) return NX_DEVICELALTKEYMASK |
                   NX_DEVICERALTKEYMASK;
        if (!strcasecmp(str,
                        "alt")) return NX_DEVICELALTKEYMASK |
                   NX_DEVICERALTKEYMASK;
        if (!strcasecmp(str,
                        "command")) return NX_DEVICELCMDKEYMASK |
                   NX_DEVICERCMDKEYMASK;
        if (!strcasecmp(str, "lshift")) return NX_DEVICELSHIFTKEYMASK;
        if (!strcasecmp(str, "rshift")) return NX_DEVICERSHIFTKEYMASK;
        if (!strcasecmp(str, "lcontrol")) return NX_DEVICELCTLKEYMASK;
        if (!strcasecmp(str, "rcontrol")) return NX_DEVICERCTLKEYMASK;
        if (!strcasecmp(str, "loption")) return NX_DEVICELALTKEYMASK;
        if (!strcasecmp(str, "roption")) return NX_DEVICERALTKEYMASK;
        if (!strcasecmp(str, "lalt")) return NX_DEVICELALTKEYMASK;
        if (!strcasecmp(str, "ralt")) return NX_DEVICERALTKEYMASK;
        if (!strcasecmp(str, "lcommand")) return NX_DEVICELCMDKEYMASK;
        if (!strcasecmp(str, "rcommand")) return NX_DEVICERCMDKEYMASK;
    }
    else {
#endif
    if (!strcasecmp(str, "shift")) return NX_SHIFTMASK;
    if (!strcasecmp(str, "control")) return NX_CONTROLMASK;
    if (!strcasecmp(str, "option")) return NX_ALTERNATEMASK;
    if (!strcasecmp(str, "alt")) return NX_ALTERNATEMASK;
    if (!strcasecmp(str, "command")) return NX_COMMANDMASK;
    if (!strcasecmp(str, "lshift")) return NX_SHIFTMASK;
    if (!strcasecmp(str, "rshift")) return NX_SHIFTMASK;
    if (!strcasecmp(str, "lcontrol")) return NX_CONTROLMASK;
    if (!strcasecmp(str, "rcontrol")) return NX_CONTROLMASK;
    if (!strcasecmp(str, "loption")) return NX_ALTERNATEMASK;
    if (!strcasecmp(str, "roption")) return NX_ALTERNATEMASK;
    if (!strcasecmp(str, "lalt")) return NX_ALTERNATEMASK;
    if (!strcasecmp(str, "ralt")) return NX_ALTERNATEMASK;
    if (!strcasecmp(str, "lcommand")) return NX_COMMANDMASK;
    if (!strcasecmp(str, "rcommand")) return NX_COMMANDMASK;
#ifdef NX_DEVICELSHIFTKEYMASK
}
#endif
    if (!strcasecmp(str, "lock")) return NX_ALPHASHIFTMASK;
    if (!strcasecmp(str, "fn")) return NX_SECONDARYFNMASK;
    if (!strcasecmp(str, "help")) return NX_HELPMASK;
    if (!strcasecmp(str, "numlock")) return NX_NUMERICPADMASK;
    return 0;
}

static KeySym
make_dead_key(KeySym in)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(dead_keys); i++)
        if (dead_keys[i].normal == in) return dead_keys[i].dead;

    return in;
}

static Bool
QuartzReadSystemKeymap(darwinKeyboardInfo *info)
{
    __block const void *chr_data = NULL;
    int num_keycodes = NUM_KEYCODES;
    __block UInt32 keyboard_type;
    int i, j;
    OSStatus err;
    KeySym *k;

    dispatch_block_t getKeyboardData = ^{
        keyboard_type = LMGetKbdType();

        TISInputSourceRef currentKeyLayoutRef = TISCopyCurrentKeyboardLayoutInputSource();

        if (currentKeyLayoutRef) {
            CFDataRef currentKeyLayoutDataRef = (CFDataRef)TISGetInputSourceProperty(currentKeyLayoutRef,
                                                                                     kTISPropertyUnicodeKeyLayoutData);
            if (currentKeyLayoutDataRef)
                chr_data = CFDataGetBytePtr(currentKeyLayoutDataRef);

            CFRelease(currentKeyLayoutRef);
        }
    };

    /* This is an ugly ant-pattern, but it is more expedient to address the problem right now. */
    if (pthread_main_np()) {
        getKeyboardData();
    } else {
        dispatch_sync(dispatch_get_main_queue(), getKeyboardData);
    }

    if (chr_data == NULL) {
        ErrorF("Couldn't get uchr or kchr resource\n");
        return FALSE;
    }

    /* Scan the keycode range for the Unicode character that each
       key produces in the four shift states. Then convert that to
       an X11 keysym (which may just the bit that says "this is
       Unicode" if it can't find the real symbol.) */

    /* KeyTranslate is not available on 64-bit platforms; UCKeyTranslate
       must be used instead. */

    for (i = 0; i < num_keycodes; i++) {
        static const int mods[4] = {
            0, MOD_SHIFT, MOD_OPTION,
            MOD_OPTION | MOD_SHIFT
        };

        k = info->keyMap + i * GLYPHS_PER_KEY;

        for (j = 0; j < 4; j++) {
            UniChar s[8];
            UniCharCount len;
            UInt32 dead_key_state = 0, extra_dead = 0;

            err = UCKeyTranslate(chr_data, i, kUCKeyActionDown,
                                 mods[j] >> 8, keyboard_type, 0,
                                 &dead_key_state, 8, &len, s);
            if (err != noErr) continue;

            if (len == 0 && dead_key_state != 0) {
                /* Found a dead key. Work out which one it is, but
                   remembering that it's dead. */
                err = UCKeyTranslate(chr_data, i, kUCKeyActionDown,
                                     mods[j] >> 8, keyboard_type,
                                     kUCKeyTranslateNoDeadKeysMask,
                                     &extra_dead, 8, &len, s);
                if (err != noErr) continue;
            }

            /* Not sure why 0x0010 is there.
             * 0x0000 - <rdar://problem/7793566> 'Unicode Hex Input' ...
             */
            if (len > 0 && s[0] != 0x0010 && s[0] != 0x0000) {
                k[j] = ucs2keysym(s[0]);
                if (dead_key_state != 0) k[j] = make_dead_key(k[j]);
            }
        }

        if (k[3] == k[2]) k[3] = NoSymbol;
        if (k[1] == k[0]) k[1] = NoSymbol;
        if (k[0] == k[2] && k[1] == k[3]) k[2] = k[3] = NoSymbol;
        if (k[3] == k[0] && k[2] == k[1] && k[2] == NoSymbol) k[3] = NoSymbol;
    }

#if HACK_MISSING
    /* Fix up some things that are normally missing.. */

    for (i = 0; i < ARRAY_SIZE(known_keys); i++) {
        k = info->keyMap + known_keys[i].keycode * GLYPHS_PER_KEY;

        if (k[0] == NoSymbol && k[1] == NoSymbol
            && k[2] == NoSymbol && k[3] == NoSymbol)
            k[0] = known_keys[i].keysym;
    }
#endif

#if HACK_KEYPAD
    /* And some more things. We find the right symbols for the numeric
       keypad, but not the KP_ keysyms. So try to convert known keycodes. */
    for (i = 0; i < ARRAY_SIZE(known_numeric_keys); i++) {
        k = info->keyMap + known_numeric_keys[i].keycode * GLYPHS_PER_KEY;

        if (k[0] == known_numeric_keys[i].normal)
            k[0] = known_numeric_keys[i].keypad;
    }
#endif

#if HACK_BLACKLIST
    for (i = 0; i < ARRAY_SIZE(keycode_blacklist); i++) {
        k = info->keyMap + keycode_blacklist[i] * GLYPHS_PER_KEY;
        k[0] = k[1] = k[2] = k[3] = NoSymbol;
    }
#endif

    DarwinBuildModifierMaps(info);

    return TRUE;
}

Bool
QuartsResyncKeymap(Bool sendDDXEvent)
{
    Bool retval;
    /* Update keyInfo */
    pthread_mutex_lock(&keyInfo_mutex);
    memset(keyInfo.keyMap, 0, sizeof(keyInfo.keyMap));
    retval = QuartzReadSystemKeymap(&keyInfo);
    pthread_mutex_unlock(&keyInfo_mutex);

    /* Tell server thread to deal with new keyInfo */
    if (sendDDXEvent)
        DarwinSendDDXEvent(kXquartzReloadKeymap, 0);

    return retval;
}
