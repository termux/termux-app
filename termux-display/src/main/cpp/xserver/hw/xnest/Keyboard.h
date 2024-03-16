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

#ifndef XNESTKEYBOARD_H
#define XNESTKEYBOARD_H

#define XNEST_KEYBOARD_EVENT_MASK \
        (KeyPressMask | KeyReleaseMask | FocusChangeMask | KeymapStateMask)

extern DeviceIntPtr xnestKeyboardDevice;

void xnestBell(int volume, DeviceIntPtr pDev, void *ctrl, int cls);
void xnestChangeKeyboardControl(DeviceIntPtr pDev, KeybdCtrl * ctrl);
int xnestKeyboardProc(DeviceIntPtr pDev, int onoff);
void xnestUpdateModifierState(unsigned int state);

#endif                          /* XNESTKEYBOARD_H */
