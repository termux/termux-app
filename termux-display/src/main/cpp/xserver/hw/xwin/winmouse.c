/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

#include "inputstr.h"
#include "exevents.h"           /* for button/axes labels */
#include "xserver-properties.h"
#include "inpututils.h"

/* Peek the internal button mapping */
static CARD8 const *g_winMouseButtonMap = NULL;

/*
 * Local prototypes
 */

static void
 winMouseCtrl(DeviceIntPtr pDevice, PtrCtrl * pCtrl);

static void
winMouseCtrl(DeviceIntPtr pDevice, PtrCtrl * pCtrl)
{
}

/*
 * See Porting Layer Definition - p. 18
 * This is known as a DeviceProc
 */

int
winMouseProc(DeviceIntPtr pDeviceInt, int iState)
{
    int lngMouseButtons, i;
    int lngWheelEvents = 4;
    CARD8 *map;
    DevicePtr pDevice = (DevicePtr) pDeviceInt;
    Atom btn_labels[9];
    Atom axes_labels[2];

    switch (iState) {
    case DEVICE_INIT:
        /* Get number of mouse buttons */
        lngMouseButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);
        winMsg(X_PROBED, "%d mouse buttons found\n", lngMouseButtons);

        /* Mapping of windows events to X events:
         * LEFT:1 MIDDLE:2 RIGHT:3
         * SCROLL_UP:4 SCROLL_DOWN:5
         * TILT_LEFT:6 TILT_RIGHT:7
         * XBUTTON 1:8 XBUTTON 2:9 (most commonly 'back' and 'forward')
         * ...
         *
         * The current Windows API only defines 2 extra buttons, so we don't
         * expect more than 5 buttons to be reported, but more than that
         * should be handled correctly
         */

        /*
         * To map scroll wheel correctly we need at least the 3 normal buttons
         */
        if (lngMouseButtons < 3)
            lngMouseButtons = 3;

        /* allocate memory:
         * number of buttons + 4 x mouse wheel event + 1 extra (offset for map)
         */
        map = malloc(sizeof(CARD8) * (lngMouseButtons + lngWheelEvents + 1));

        /* initialize button map */
        map[0] = 0;
        for (i = 1; i <= lngMouseButtons + lngWheelEvents; i++)
            map[i] = i;

        btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
        btn_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
        btn_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
        btn_labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
        btn_labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
        btn_labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
        btn_labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);
        btn_labels[7] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_BACK);
        btn_labels[8] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_FORWARD);

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);

        InitPointerDeviceStruct(pDevice,
                                map,
                                lngMouseButtons + lngWheelEvents,
                                btn_labels,
                                winMouseCtrl,
                                GetMotionHistorySize(), 2, axes_labels);
        free(map);

        g_winMouseButtonMap = pDeviceInt->button->map;
        break;

    case DEVICE_ON:
        pDevice->on = TRUE;
        break;

    case DEVICE_CLOSE:
        g_winMouseButtonMap = NULL;

    case DEVICE_OFF:
        pDevice->on = FALSE;
        break;
    }
    return Success;
}

/* Handle the mouse wheel */
int
winMouseWheel(int *iTotalDeltaZ, int iDeltaZ, int iButtonUp, int iButtonDown)
{
    int button;

    /* Do we have any previous delta stored? */
    if ((*iTotalDeltaZ > 0 && iDeltaZ > 0)
        || (*iTotalDeltaZ < 0 && iDeltaZ < 0)) {
        /* Previous delta and of same sign as current delta */
        iDeltaZ += *iTotalDeltaZ;
        *iTotalDeltaZ = 0;
    }
    else {
        /*
         * Previous delta of different sign, or zero.
         * We will set it to zero for either case,
         * as blindly setting takes just as much time
         * as checking, then setting if necessary :)
         */
        *iTotalDeltaZ = 0;
    }

    /*
     * Only process this message if the wheel has moved further than
     * WHEEL_DELTA
     */
    if (iDeltaZ >= WHEEL_DELTA || (-1 * iDeltaZ) >= WHEEL_DELTA) {
        *iTotalDeltaZ = 0;

        /* Figure out how many whole deltas of the wheel we have */
        iDeltaZ /= WHEEL_DELTA;
    }
    else {
        /*
         * Wheel has not moved past WHEEL_DELTA threshold;
         * we will store the wheel delta until the threshold
         * has been reached.
         */
        *iTotalDeltaZ = iDeltaZ;
        return 0;
    }

    /* Set the button to indicate up or down wheel delta */
    if (iDeltaZ > 0) {
        button = iButtonUp;
    }
    else {
        button = iButtonDown;
    }

    /*
     * Flip iDeltaZ to positive, if negative,
     * because always need to generate a *positive* number of
     * button clicks for the Z axis.
     */
    if (iDeltaZ < 0) {
        iDeltaZ *= -1;
    }

    /* Generate X input messages for each wheel delta we have seen */
    while (iDeltaZ--) {
        /* Push the wheel button */
        winMouseButtonsSendEvent(ButtonPress, button);

        /* Release the wheel button */
        winMouseButtonsSendEvent(ButtonRelease, button);
    }

    return 0;
}

/*
 * Enqueue a mouse button event
 */

void
winMouseButtonsSendEvent(int iEventType, int iButton)
{
    ValuatorMask mask;

    if (g_winMouseButtonMap)
        iButton = g_winMouseButtonMap[iButton];

    valuator_mask_zero(&mask);
    QueuePointerEvents(g_pwinPointer, iEventType, iButton,
                       POINTER_RELATIVE, &mask);

#if CYGDEBUG
    ErrorF("winMouseButtonsSendEvent: iEventType: %d, iButton: %d\n",
           iEventType, iButton);
#endif
}

/*
 * Decide what to do with a Windows mouse message
 */

int
winMouseButtonsHandle(ScreenPtr pScreen,
                      int iEventType, int iButton, WPARAM wParam)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;

    /* Send button events right away if emulate 3 buttons is off */
    if (pScreenInfo->iE3BTimeout == WIN_E3B_OFF) {
        /* Emulate 3 buttons is off, send the button event */
        winMouseButtonsSendEvent(iEventType, iButton);
        return 0;
    }

    /* Emulate 3 buttons is on, let the fun begin */
    if (iEventType == ButtonPress
        && pScreenPriv->iE3BCachedPress == 0
        && !pScreenPriv->fE3BFakeButton2Sent) {
        /*
         * Button was pressed, no press is cached,
         * and there is no fake button 2 release pending.
         */

        /* Store button press type */
        pScreenPriv->iE3BCachedPress = iButton;

        /*
         * Set a timer to send this button press if the other button
         * is not pressed within the timeout time.
         */
        SetTimer(pScreenPriv->hwndScreen,
                 WIN_E3B_TIMER_ID, pScreenInfo->iE3BTimeout, NULL);
    }
    else if (iEventType == ButtonPress
             && pScreenPriv->iE3BCachedPress != 0
             && pScreenPriv->iE3BCachedPress != iButton
             && !pScreenPriv->fE3BFakeButton2Sent) {
        /*
         * Button press is cached, other button was pressed,
         * and there is no fake button 2 release pending.
         */

        /* Mouse button was cached and other button was pressed */
        KillTimer(pScreenPriv->hwndScreen, WIN_E3B_TIMER_ID);
        pScreenPriv->iE3BCachedPress = 0;

        /* Send fake middle button */
        winMouseButtonsSendEvent(ButtonPress, Button2);

        /* Indicate that a fake middle button event was sent */
        pScreenPriv->fE3BFakeButton2Sent = TRUE;
    }
    else if (iEventType == ButtonRelease
             && pScreenPriv->iE3BCachedPress == iButton) {
        /*
         * Cached button was released before timer ran out,
         * and before the other mouse button was pressed.
         */
        KillTimer(pScreenPriv->hwndScreen, WIN_E3B_TIMER_ID);
        pScreenPriv->iE3BCachedPress = 0;

        /* Send cached press, then send release */
        winMouseButtonsSendEvent(ButtonPress, iButton);
        winMouseButtonsSendEvent(ButtonRelease, iButton);
    }
    else if (iEventType == ButtonRelease
             && pScreenPriv->fE3BFakeButton2Sent && !(wParam & MK_LBUTTON)
             && !(wParam & MK_RBUTTON)) {
        /*
         * Fake button 2 was sent and both mouse buttons have now been released
         */
        pScreenPriv->fE3BFakeButton2Sent = FALSE;

        /* Send middle mouse button release */
        winMouseButtonsSendEvent(ButtonRelease, Button2);
    }
    else if (iEventType == ButtonRelease
             && pScreenPriv->iE3BCachedPress == 0
             && !pScreenPriv->fE3BFakeButton2Sent) {
        /*
         * Button was release, no button is cached,
         * and there is no fake button 2 release is pending.
         */
        winMouseButtonsSendEvent(ButtonRelease, iButton);
    }

    return 0;
}

/**
 * Enqueue a motion event.
 *
 */
void
winEnqueueMotion(int x, int y)
{
    int valuators[2];
    ValuatorMask mask;

    valuators[0] = x;
    valuators[1] = y;

    valuator_mask_set_range(&mask, 0, 2, valuators);
    QueuePointerEvents(g_pwinPointer, MotionNotify, 0,
                       POINTER_ABSOLUTE | POINTER_SCREEN, &mask);

}
