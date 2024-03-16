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

#include <X11/X.h>
#include <X11/Xproto.h>
#include "screenint.h"
#include "inputstr.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "mipointer.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "Pointer.h"
#include "Args.h"

#include "xserver-properties.h"
#include "exevents.h"           /* For XIGetKnownProperty */

DeviceIntPtr xnestPointerDevice = NULL;

void
xnestChangePointerControl(DeviceIntPtr pDev, PtrCtrl * ctrl)
{
    XChangePointerControl(xnestDisplay, True, True,
                          ctrl->num, ctrl->den, ctrl->threshold);
}

int
xnestPointerProc(DeviceIntPtr pDev, int onoff)
{
    CARD8 map[MAXBUTTONS];
    Atom btn_labels[MAXBUTTONS] = { 0 };
    Atom axes_labels[2] = { 0 };
    int nmap;
    int i;

    switch (onoff) {
    case DEVICE_INIT:
        nmap = XGetPointerMapping(xnestDisplay, map, MAXBUTTONS);
        for (i = 0; i <= nmap; i++)
            map[i] = i;         /* buttons are already mapped */

        btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
        btn_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
        btn_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
        btn_labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
        btn_labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
        btn_labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
        btn_labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);

        XGetPointerControl(xnestDisplay,
                           &defaultPointerControl.num,
                           &defaultPointerControl.den,
                           &defaultPointerControl.threshold);
        InitPointerDeviceStruct(&pDev->public, map, nmap, btn_labels,
                                xnestChangePointerControl,
                                GetMotionHistorySize(), 2, axes_labels);
        break;
    case DEVICE_ON:
        xnestEventMask |= XNEST_POINTER_EVENT_MASK;
        for (i = 0; i < xnestNumScreens; i++)
            XSelectInput(xnestDisplay, xnestDefaultWindows[i], xnestEventMask);
        break;
    case DEVICE_OFF:
        xnestEventMask &= ~XNEST_POINTER_EVENT_MASK;
        for (i = 0; i < xnestNumScreens; i++)
            XSelectInput(xnestDisplay, xnestDefaultWindows[i], xnestEventMask);
        break;
    case DEVICE_CLOSE:
        break;
    }
    return Success;
}
