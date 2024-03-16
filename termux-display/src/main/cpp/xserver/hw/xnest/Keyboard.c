/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#endif

#ifdef WIN32
#include <X11/Xwindows.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include "screenint.h"
#include "inputstr.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "Keyboard.h"
#include "Args.h"
#include "Events.h"

#include <X11/extensions/XKB.h>
#include "xkbsrv.h"
#include <X11/extensions/XKBconfig.h>

extern Bool
 XkbQueryExtension(Display * /* dpy */ ,
                   int * /* opcodeReturn */ ,
                   int * /* eventBaseReturn */ ,
                   int * /* errorBaseReturn */ ,
                   int * /* majorRtrn */ ,
                   int *        /* minorRtrn */
    );

extern XkbDescPtr XkbGetKeyboard(Display * /* dpy */ ,
                                 unsigned int /* which */ ,
                                 unsigned int   /* deviceSpec */
    );

extern Status XkbGetControls(Display * /* dpy */ ,
                             unsigned long /* which */ ,
                             XkbDescPtr /* desc */
    );

DeviceIntPtr xnestKeyboardDevice = NULL;

void
xnestBell(int volume, DeviceIntPtr pDev, void *ctrl, int cls)
{
    XBell(xnestDisplay, volume);
}

void
DDXRingBell(int volume, int pitch, int duration)
{
    XBell(xnestDisplay, volume);
}

void
xnestChangeKeyboardControl(DeviceIntPtr pDev, KeybdCtrl * ctrl)
{
#if 0
    unsigned long value_mask;
    XKeyboardControl values;
    int i;

    value_mask = KBKeyClickPercent |
        KBBellPercent | KBBellPitch | KBBellDuration | KBAutoRepeatMode;

    values.key_click_percent = ctrl->click;
    values.bell_percent = ctrl->bell;
    values.bell_pitch = ctrl->bell_pitch;
    values.bell_duration = ctrl->bell_duration;
    values.auto_repeat_mode = ctrl->autoRepeat ?
        AutoRepeatModeOn : AutoRepeatModeOff;

    XChangeKeyboardControl(xnestDisplay, value_mask, &values);

    /*
       value_mask = KBKey | KBAutoRepeatMode;
       At this point, we need to walk through the vector and compare it
       to the current server vector.  If there are differences, report them.
     */

    value_mask = KBLed | KBLedMode;
    for (i = 1; i <= 32; i++) {
        values.led = i;
        values.led_mode =
            (ctrl->leds & (1 << (i - 1))) ? LedModeOn : LedModeOff;
        XChangeKeyboardControl(xnestDisplay, value_mask, &values);
    }
#endif
}

int
xnestKeyboardProc(DeviceIntPtr pDev, int onoff)
{
    XModifierKeymap *modifier_keymap;
    KeySym *keymap;
    int mapWidth;
    int min_keycode, max_keycode;
    KeySymsRec keySyms;
    CARD8 modmap[MAP_LENGTH];
    int i, j;
    XKeyboardState values;
    XkbDescPtr xkb;
    int op, event, error, major, minor;

    switch (onoff) {
    case DEVICE_INIT:
        XDisplayKeycodes(xnestDisplay, &min_keycode, &max_keycode);
#ifdef _XSERVER64
        {
            KeySym64 *keymap64;
            int len;

            keymap64 = XGetKeyboardMapping(xnestDisplay,
                                           min_keycode,
                                           max_keycode - min_keycode + 1,
                                           &mapWidth);
            len = (max_keycode - min_keycode + 1) * mapWidth;
            keymap = xallocarray(len, sizeof(KeySym));
            for (i = 0; i < len; ++i)
                keymap[i] = keymap64[i];
            XFree(keymap64);
        }
#else
        keymap = XGetKeyboardMapping(xnestDisplay,
                                     min_keycode,
                                     max_keycode - min_keycode + 1, &mapWidth);
#endif

        memset(modmap, 0, sizeof(modmap));
        modifier_keymap = XGetModifierMapping(xnestDisplay);
        for (j = 0; j < 8; j++)
            for (i = 0; i < modifier_keymap->max_keypermod; i++) {
                CARD8 keycode;

                if ((keycode =
                     modifier_keymap->modifiermap[j *
                                                  modifier_keymap->
                                                  max_keypermod + i]))
                    modmap[keycode] |= 1 << j;
            }
        XFreeModifiermap(modifier_keymap);

        keySyms.minKeyCode = min_keycode;
        keySyms.maxKeyCode = max_keycode;
        keySyms.mapWidth = mapWidth;
        keySyms.map = keymap;

        if (XkbQueryExtension(xnestDisplay, &op, &event, &error, &major, &minor)
            == 0) {
            ErrorF("Unable to initialize XKEYBOARD extension.\n");
            goto XkbError;
        }
        xkb =
            XkbGetKeyboard(xnestDisplay, XkbGBN_AllComponentsMask,
                           XkbUseCoreKbd);
        if (xkb == NULL || xkb->geom == NULL) {
            ErrorF("Couldn't get keyboard.\n");
            goto XkbError;
        }
        XkbGetControls(xnestDisplay, XkbAllControlsMask, xkb);

        InitKeyboardDeviceStruct(pDev, NULL,
                                 xnestBell, xnestChangeKeyboardControl);

        XkbApplyMappingChange(pDev, &keySyms, keySyms.minKeyCode,
                              keySyms.maxKeyCode - keySyms.minKeyCode + 1,
                              modmap, serverClient);

        XkbDDXChangeControls(pDev, xkb->ctrls, xkb->ctrls);
        XkbFreeKeyboard(xkb, 0, False);
        free(keymap);
        break;
    case DEVICE_ON:
        xnestEventMask |= XNEST_KEYBOARD_EVENT_MASK;
        for (i = 0; i < xnestNumScreens; i++)
            XSelectInput(xnestDisplay, xnestDefaultWindows[i], xnestEventMask);
        break;
    case DEVICE_OFF:
        xnestEventMask &= ~XNEST_KEYBOARD_EVENT_MASK;
        for (i = 0; i < xnestNumScreens; i++)
            XSelectInput(xnestDisplay, xnestDefaultWindows[i], xnestEventMask);
        break;
    case DEVICE_CLOSE:
        break;
    }
    return Success;

 XkbError:
    XGetKeyboardControl(xnestDisplay, &values);
    memmove((char *) defaultKeyboardControl.autoRepeats,
            (char *) values.auto_repeats, sizeof(values.auto_repeats));

    InitKeyboardDeviceStruct(pDev, NULL, xnestBell, xnestChangeKeyboardControl);
    free(keymap);
    return Success;
}

void
xnestUpdateModifierState(unsigned int state)
{
    DeviceIntPtr pDev = xnestKeyboardDevice;
    KeyClassPtr keyc = pDev->key;
    int i;
    CARD8 mask;
    int xkb_state;

    if (!pDev)
        return;

    xkb_state = XkbStateFieldFromRec(&pDev->key->xkbInfo->state);
    state = state & 0xff;

    if (xkb_state == state)
        return;

    for (i = 0, mask = 1; i < 8; i++, mask <<= 1) {
        int key;

        /* Modifier is down, but shouldn't be */
        if ((xkb_state & mask) && !(state & mask)) {
            int count = keyc->modifierKeyCount[i];

            for (key = 0; key < MAP_LENGTH; key++)
                if (keyc->xkbInfo->desc->map->modmap[key] & mask) {
                    if (mask == LockMask) {
                        xnestQueueKeyEvent(KeyPress, key);
                        xnestQueueKeyEvent(KeyRelease, key);
                    }
                    else if (key_is_down(pDev, key, KEY_PROCESSED))
                        xnestQueueKeyEvent(KeyRelease, key);

                    if (--count == 0)
                        break;
                }
        }

        /* Modifier should be down, but isn't */
        if (!(xkb_state & mask) && (state & mask))
            for (key = 0; key < MAP_LENGTH; key++)
                if (keyc->xkbInfo->desc->map->modmap[key] & mask) {
                    xnestQueueKeyEvent(KeyPress, key);
                    if (mask == LockMask)
                        xnestQueueKeyEvent(KeyRelease, key);
                    break;
                }
    }
}
