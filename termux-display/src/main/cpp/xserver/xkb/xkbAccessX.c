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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include "exglobals.h"
#include <X11/extensions/XIproto.h>
#include "inputstr.h"
#include "eventstr.h"
#include "inpututils.h"
#include <xkbsrv.h>
#if !defined(WIN32)
#include <sys/time.h>
#endif

int XkbDfltRepeatDelay = 660;
int XkbDfltRepeatInterval = 40;

#define	DFLT_TIMEOUT_CTRLS (XkbAX_KRGMask|XkbStickyKeysMask|XkbMouseKeysMask)
#define	DFLT_TIMEOUT_OPTS  (XkbAX_IndicatorFBMask)

unsigned short XkbDfltAccessXTimeout = 120;
unsigned int XkbDfltAccessXTimeoutMask = DFLT_TIMEOUT_CTRLS;
static unsigned int XkbDfltAccessXTimeoutValues = 0;
static unsigned int XkbDfltAccessXTimeoutOptionsMask = DFLT_TIMEOUT_OPTS;
static unsigned int XkbDfltAccessXTimeoutOptionsValues = 0;
unsigned int XkbDfltAccessXFeedback = XkbAccessXFeedbackMask;
unsigned short XkbDfltAccessXOptions =
    XkbAX_AllOptionsMask & ~(XkbAX_IndicatorFBMask | XkbAX_SKReleaseFBMask |
                             XkbAX_SKRejectFBMask);

void
AccessXComputeCurveFactor(XkbSrvInfoPtr xkbi, XkbControlsPtr ctrls)
{
    xkbi->mouseKeysCurve = 1.0 + (((double) ctrls->mk_curve) * 0.001);
    xkbi->mouseKeysCurveFactor = (((double) ctrls->mk_max_speed) /
                                  pow((double) ctrls->mk_time_to_max,
                                      xkbi->mouseKeysCurve));
    return;
}

void
AccessXInit(DeviceIntPtr keybd)
{
    XkbSrvInfoPtr xkbi = keybd->key->xkbInfo;
    XkbControlsPtr ctrls = xkbi->desc->ctrls;

    xkbi->shiftKeyCount = 0;
    xkbi->mouseKeysCounter = 0;
    xkbi->inactiveKey = 0;
    xkbi->slowKey = 0;
    xkbi->repeatKey = 0;
    xkbi->krgTimerActive = _OFF_TIMER;
    xkbi->beepType = _BEEP_NONE;
    xkbi->beepCount = 0;
    xkbi->mouseKeyTimer = NULL;
    xkbi->slowKeysTimer = NULL;
    xkbi->bounceKeysTimer = NULL;
    xkbi->repeatKeyTimer = NULL;
    xkbi->krgTimer = NULL;
    xkbi->beepTimer = NULL;
    xkbi->checkRepeat = NULL;
    ctrls->repeat_delay = XkbDfltRepeatDelay;
    ctrls->repeat_interval = XkbDfltRepeatInterval;
    ctrls->debounce_delay = 300;
    ctrls->slow_keys_delay = 300;
    ctrls->mk_delay = 160;
    ctrls->mk_interval = 40;
    ctrls->mk_time_to_max = 30;
    ctrls->mk_max_speed = 30;
    ctrls->mk_curve = 500;
    ctrls->mk_dflt_btn = 1;
    ctrls->ax_timeout = XkbDfltAccessXTimeout;
    ctrls->axt_ctrls_mask = XkbDfltAccessXTimeoutMask;
    ctrls->axt_ctrls_values = XkbDfltAccessXTimeoutValues;
    ctrls->axt_opts_mask = XkbDfltAccessXTimeoutOptionsMask;
    ctrls->axt_opts_values = XkbDfltAccessXTimeoutOptionsValues;
    if (XkbDfltAccessXTimeout)
        ctrls->enabled_ctrls |= XkbAccessXTimeoutMask;
    else
        ctrls->enabled_ctrls &= ~XkbAccessXTimeoutMask;
    ctrls->enabled_ctrls |= XkbDfltAccessXFeedback;
    ctrls->ax_options = XkbDfltAccessXOptions;
    AccessXComputeCurveFactor(xkbi, ctrls);
    return;
}

/************************************************************************/
/*									*/
/* AccessXKeyboardEvent							*/
/*									*/
/*	Generate a synthetic keyboard event.				*/
/*									*/
/************************************************************************/
static void
AccessXKeyboardEvent(DeviceIntPtr keybd, int type, BYTE keyCode, Bool isRepeat)
{
    DeviceEvent event;

    init_device_event(&event, keybd, GetTimeInMillis(), EVENT_SOURCE_NORMAL);
    event.type = type;
    event.detail.key = keyCode;
    event.key_repeat = isRepeat;

    if (xkbDebugFlags & 0x8) {
        DebugF("[xkb] AXKE: Key %d %s\n", keyCode,
               (event.type == ET_KeyPress ? "down" : "up"));
    }

    XkbProcessKeyboardEvent(&event, keybd);
    return;
}                               /* AccessXKeyboardEvent */

/************************************************************************/
/*									*/
/* AccessXKRGTurnOn							*/
/*									*/
/*	Turn the keyboard response group on.				*/
/*									*/
/************************************************************************/
static void
AccessXKRGTurnOn(DeviceIntPtr dev, CARD16 KRGControl, xkbControlsNotify * pCN)
{
    XkbSrvInfoPtr xkbi = dev->key->xkbInfo;
    XkbControlsPtr ctrls = xkbi->desc->ctrls;
    XkbControlsRec old;
    XkbEventCauseRec cause;
    XkbSrvLedInfoPtr sli;

    old = *ctrls;
    ctrls->enabled_ctrls |= (KRGControl & XkbAX_KRGMask);
    if (XkbComputeControlsNotify(dev, &old, ctrls, pCN, FALSE))
        XkbSendControlsNotify(dev, pCN);
    cause.kc = pCN->keycode;
    cause.event = pCN->eventType;
    cause.mjr = pCN->requestMajor;
    cause.mnr = pCN->requestMinor;
    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId, 0);
    XkbUpdateIndicators(dev, sli->usesControls, TRUE, NULL, &cause);
    if (XkbAX_NeedFeedback(ctrls, XkbAX_FeatureFBMask))
        XkbDDXAccessXBeep(dev, _BEEP_FEATURE_ON, KRGControl);
    return;

}                               /* AccessXKRGTurnOn */

/************************************************************************/
/*									*/
/* AccessXKRGTurnOff							*/
/*									*/
/*	Turn the keyboard response group off.				*/
/*									*/
/************************************************************************/
static void
AccessXKRGTurnOff(DeviceIntPtr dev, xkbControlsNotify * pCN)
{
    XkbSrvInfoPtr xkbi = dev->key->xkbInfo;
    XkbControlsPtr ctrls = xkbi->desc->ctrls;
    XkbControlsRec old;
    XkbEventCauseRec cause;
    XkbSrvLedInfoPtr sli;

    old = *ctrls;
    ctrls->enabled_ctrls &= ~XkbAX_KRGMask;
    if (XkbComputeControlsNotify(dev, &old, ctrls, pCN, FALSE))
        XkbSendControlsNotify(dev, pCN);
    cause.kc = pCN->keycode;
    cause.event = pCN->eventType;
    cause.mjr = pCN->requestMajor;
    cause.mnr = pCN->requestMinor;
    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId, 0);
    XkbUpdateIndicators(dev, sli->usesControls, TRUE, NULL, &cause);
    if (XkbAX_NeedFeedback(ctrls, XkbAX_FeatureFBMask)) {
        unsigned changes = old.enabled_ctrls ^ ctrls->enabled_ctrls;

        XkbDDXAccessXBeep(dev, _BEEP_FEATURE_OFF, changes);
    }
    return;

}                               /* AccessXKRGTurnOff */

/************************************************************************/
/*									*/
/* AccessXStickyKeysTurnOn						*/
/*									*/
/*	Turn StickyKeys on.						*/
/*									*/
/************************************************************************/
static void
AccessXStickyKeysTurnOn(DeviceIntPtr dev, xkbControlsNotify * pCN)
{
    XkbSrvInfoPtr xkbi = dev->key->xkbInfo;
    XkbControlsPtr ctrls = xkbi->desc->ctrls;
    XkbControlsRec old;
    XkbEventCauseRec cause;
    XkbSrvLedInfoPtr sli;

    old = *ctrls;
    ctrls->enabled_ctrls |= XkbStickyKeysMask;
    xkbi->shiftKeyCount = 0;
    if (XkbComputeControlsNotify(dev, &old, ctrls, pCN, FALSE))
        XkbSendControlsNotify(dev, pCN);
    cause.kc = pCN->keycode;
    cause.event = pCN->eventType;
    cause.mjr = pCN->requestMajor;
    cause.mnr = pCN->requestMinor;
    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId, 0);
    XkbUpdateIndicators(dev, sli->usesControls, TRUE, NULL, &cause);
    if (XkbAX_NeedFeedback(ctrls, XkbAX_FeatureFBMask)) {
        XkbDDXAccessXBeep(dev, _BEEP_FEATURE_ON, XkbStickyKeysMask);
    }
    return;

}                               /* AccessXStickyKeysTurnOn */

/************************************************************************/
/*									*/
/* AccessXStickyKeysTurnOff						*/
/*									*/
/*	Turn StickyKeys off.						*/
/*									*/
/************************************************************************/
static void
AccessXStickyKeysTurnOff(DeviceIntPtr dev, xkbControlsNotify * pCN)
{
    XkbSrvInfoPtr xkbi = dev->key->xkbInfo;
    XkbControlsPtr ctrls = xkbi->desc->ctrls;
    XkbControlsRec old;
    XkbEventCauseRec cause;
    XkbSrvLedInfoPtr sli;

    old = *ctrls;
    ctrls->enabled_ctrls &= ~XkbStickyKeysMask;
    xkbi->shiftKeyCount = 0;
    if (XkbComputeControlsNotify(dev, &old, ctrls, pCN, FALSE))
        XkbSendControlsNotify(dev, pCN);

    cause.kc = pCN->keycode;
    cause.event = pCN->eventType;
    cause.mjr = pCN->requestMajor;
    cause.mnr = pCN->requestMinor;
    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId, 0);
    XkbUpdateIndicators(dev, sli->usesControls, TRUE, NULL, &cause);
    if (XkbAX_NeedFeedback(ctrls, XkbAX_FeatureFBMask)) {
        XkbDDXAccessXBeep(dev, _BEEP_FEATURE_OFF, XkbStickyKeysMask);
    }
#ifndef NO_CLEAR_LATCHES_FOR_STICKY_KEYS_OFF
    XkbClearAllLatchesAndLocks(dev, xkbi, FALSE, &cause);
#endif
    return;
}                               /* AccessXStickyKeysTurnOff */

static CARD32
AccessXKRGExpire(OsTimerPtr timer, CARD32 now, void *arg)
{
    xkbControlsNotify cn;
    DeviceIntPtr dev = arg;
    XkbSrvInfoPtr xkbi = dev->key->xkbInfo;

    if (xkbi->krgTimerActive == _KRG_WARN_TIMER) {
        XkbDDXAccessXBeep(dev, _BEEP_SLOW_WARN, XkbStickyKeysMask);
        xkbi->krgTimerActive = _KRG_TIMER;
        return 4000;
    }
    xkbi->krgTimerActive = _OFF_TIMER;
    cn.keycode = xkbi->slowKeyEnableKey;
    cn.eventType = KeyPress;
    cn.requestMajor = 0;
    cn.requestMinor = 0;
    if (xkbi->desc->ctrls->enabled_ctrls & XkbSlowKeysMask) {
        AccessXKRGTurnOff(dev, &cn);
        LogMessage(X_INFO, "XKB SlowKeys are disabled.\n");
    }
    else {
        AccessXKRGTurnOn(dev, XkbSlowKeysMask, &cn);
        LogMessage(X_INFO, "XKB SlowKeys are now enabled. Hold shift to disable.\n");
    }

    xkbi->slowKeyEnableKey = 0;
    return 0;
}

static CARD32
AccessXRepeatKeyExpire(OsTimerPtr timer, CARD32 now, void *arg)
{
    DeviceIntPtr dev = (DeviceIntPtr) arg;
    XkbSrvInfoPtr xkbi = dev->key->xkbInfo;

    if (xkbi->repeatKey == 0)
        return 0;

    if (xkbi->checkRepeat == NULL || xkbi->checkRepeat (dev, xkbi, xkbi->repeatKey))
        AccessXKeyboardEvent(dev, ET_KeyPress, xkbi->repeatKey, TRUE);

    return xkbi->desc->ctrls->repeat_interval;
}

void
AccessXCancelRepeatKey(XkbSrvInfoPtr xkbi, KeyCode key)
{
    if (xkbi->repeatKey == key)
        xkbi->repeatKey = 0;
    return;
}

static CARD32
AccessXSlowKeyExpire(OsTimerPtr timer, CARD32 now, void *arg)
{
    DeviceIntPtr keybd;
    XkbSrvInfoPtr xkbi;
    XkbDescPtr xkb;
    XkbControlsPtr ctrls;

    keybd = (DeviceIntPtr) arg;
    xkbi = keybd->key->xkbInfo;
    xkb = xkbi->desc;
    ctrls = xkb->ctrls;
    if (xkbi->slowKey != 0) {
        xkbAccessXNotify ev;
        KeySym *sym = XkbKeySymsPtr(xkb, xkbi->slowKey);

        ev.detail = XkbAXN_SKAccept;
        ev.keycode = xkbi->slowKey;
        ev.slowKeysDelay = ctrls->slow_keys_delay;
        ev.debounceDelay = ctrls->debounce_delay;
        XkbSendAccessXNotify(keybd, &ev);
        if (XkbAX_NeedFeedback(ctrls, XkbAX_SKAcceptFBMask))
            XkbDDXAccessXBeep(keybd, _BEEP_SLOW_ACCEPT, XkbSlowKeysMask);
        AccessXKeyboardEvent(keybd, ET_KeyPress, xkbi->slowKey, FALSE);
        /* check for magic sequences */
        if ((ctrls->enabled_ctrls & XkbAccessXKeysMask) &&
            ((sym[0] == XK_Shift_R) || (sym[0] == XK_Shift_L)))
            xkbi->shiftKeyCount++;

        /* Start repeating if necessary.  Stop autorepeating if the user
         * presses a non-modifier key that doesn't autorepeat.
         */
        if (keybd->kbdfeed->ctrl.autoRepeat &&
            ((xkbi->slowKey != xkbi->mouseKey) || (!xkbi->mouseKeysAccel)) &&
            (ctrls->enabled_ctrls & XkbRepeatKeysMask)) {
            if (BitIsOn(keybd->kbdfeed->ctrl.autoRepeats, xkbi->slowKey)) {
                xkbi->repeatKey = xkbi->slowKey;
                xkbi->repeatKeyTimer = TimerSet(xkbi->repeatKeyTimer,
                                                0, ctrls->repeat_delay,
                                                AccessXRepeatKeyExpire,
                                                (void *) keybd);
            }
        }
    }
    return 0;
}

static CARD32
AccessXBounceKeyExpire(OsTimerPtr timer, CARD32 now, void *arg)
{
    XkbSrvInfoPtr xkbi = ((DeviceIntPtr) arg)->key->xkbInfo;

    xkbi->inactiveKey = 0;
    return 0;
}

static CARD32
AccessXTimeoutExpire(OsTimerPtr timer, CARD32 now, void *arg)
{
    DeviceIntPtr dev = (DeviceIntPtr) arg;
    XkbSrvInfoPtr xkbi = dev->key->xkbInfo;
    XkbControlsPtr ctrls = xkbi->desc->ctrls;
    XkbControlsRec old;
    xkbControlsNotify cn;
    XkbEventCauseRec cause;
    XkbSrvLedInfoPtr sli;

    if (xkbi->lastPtrEventTime) {
        unsigned timeToWait = (ctrls->ax_timeout * 1000);
        unsigned timeElapsed = (now - xkbi->lastPtrEventTime);

        if (timeToWait > timeElapsed)
            return timeToWait - timeElapsed;
    }
    old = *ctrls;
    xkbi->shiftKeyCount = 0;
    ctrls->enabled_ctrls &= ~ctrls->axt_ctrls_mask;
    ctrls->enabled_ctrls |= (ctrls->axt_ctrls_values & ctrls->axt_ctrls_mask);
    if (ctrls->axt_opts_mask) {
        ctrls->ax_options &= ~ctrls->axt_opts_mask;
        ctrls->ax_options |= (ctrls->axt_opts_values & ctrls->axt_opts_mask);
    }
    if (XkbComputeControlsNotify(dev, &old, ctrls, &cn, FALSE)) {
        cn.keycode = 0;
        cn.eventType = 0;
        cn.requestMajor = 0;
        cn.requestMinor = 0;
        XkbSendControlsNotify(dev, &cn);
    }
    XkbSetCauseUnknown(&cause);
    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId, 0);
    XkbUpdateIndicators(dev, sli->usesControls, TRUE, NULL, &cause);
    if (ctrls->ax_options != old.ax_options) {
        unsigned set, cleared, bell;

        set = ctrls->ax_options & (~old.ax_options);
        cleared = (~ctrls->ax_options) & old.ax_options;
        if (set && cleared)
            bell = _BEEP_FEATURE_CHANGE;
        else if (set)
            bell = _BEEP_FEATURE_ON;
        else
            bell = _BEEP_FEATURE_OFF;
        XkbDDXAccessXBeep(dev, bell, XkbAccessXTimeoutMask);
    }
    xkbi->krgTimerActive = _OFF_TIMER;
    return 0;
}

/************************************************************************/
/*									*/
/* AccessXFilterPressEvent						*/
/*									*/
/* Filter events before they get any further if SlowKeys is turned on.	*/
/* In addition, this routine handles the ever so popular magic key	*/
/* acts for turning various accessibility features on/off.		*/
/*									*/
/* Returns TRUE if this routine has discarded the event.		*/
/* Returns FALSE if the event needs further processing.			*/
/*									*/
/************************************************************************/
Bool
AccessXFilterPressEvent(DeviceEvent *event, DeviceIntPtr keybd)
{
    XkbSrvInfoPtr xkbi = keybd->key->xkbInfo;
    XkbControlsPtr ctrls = xkbi->desc->ctrls;
    Bool ignoreKeyEvent = FALSE;
    KeyCode key = event->detail.key;
    KeySym *sym = XkbKeySymsPtr(xkbi->desc, key);

    if (ctrls->enabled_ctrls & XkbAccessXKeysMask) {
        /* check for magic sequences */
        if ((sym[0] == XK_Shift_R) || (sym[0] == XK_Shift_L)) {
            xkbi->slowKeyEnableKey = key;
            if (XkbAX_NeedFeedback(ctrls, XkbAX_SlowWarnFBMask)) {
                xkbi->krgTimerActive = _KRG_WARN_TIMER;
                xkbi->krgTimer = TimerSet(xkbi->krgTimer, 0, 4000,
                                          AccessXKRGExpire, (void *) keybd);
            }
            else {
                xkbi->krgTimerActive = _KRG_TIMER;
                xkbi->krgTimer = TimerSet(xkbi->krgTimer, 0, 8000,
                                          AccessXKRGExpire, (void *) keybd);
            }
            if (!(ctrls->enabled_ctrls & XkbSlowKeysMask)) {
                CARD32 now = GetTimeInMillis();

                if ((now - xkbi->lastShiftEventTime) > 15000)
                    xkbi->shiftKeyCount = 1;
                else
                    xkbi->shiftKeyCount++;
                xkbi->lastShiftEventTime = now;
            }
        }
        else {
            if (xkbi->krgTimerActive) {
                xkbi->krgTimer = TimerSet(xkbi->krgTimer, 0, 0, NULL, NULL);
                xkbi->krgTimerActive = _OFF_TIMER;
            }
        }
    }

    /* Don't transmit the KeyPress if SlowKeys is turned on;
     * The wakeup handler will synthesize one for us if the user
     * has held the key long enough.
     */
    if (ctrls->enabled_ctrls & XkbSlowKeysMask) {
        xkbAccessXNotify ev;

        /* If key was already pressed, ignore subsequent press events
         * from the server's autorepeat
         */
        if (xkbi->slowKey == key)
            return TRUE;
        ev.detail = XkbAXN_SKPress;
        ev.keycode = key;
        ev.slowKeysDelay = ctrls->slow_keys_delay;
        ev.debounceDelay = ctrls->debounce_delay;
        XkbSendAccessXNotify(keybd, &ev);
        if (XkbAX_NeedFeedback(ctrls, XkbAX_SKPressFBMask))
            XkbDDXAccessXBeep(keybd, _BEEP_SLOW_PRESS, XkbSlowKeysMask);
        xkbi->slowKey = key;
        xkbi->slowKeysTimer = TimerSet(xkbi->slowKeysTimer,
                                       0, ctrls->slow_keys_delay,
                                       AccessXSlowKeyExpire, (void *) keybd);
        ignoreKeyEvent = TRUE;
    }

    /* Don't transmit the KeyPress if BounceKeys is turned on
     * and the user pressed the same key within a given time period
     * from the last release.
     */
    else if ((ctrls->enabled_ctrls & XkbBounceKeysMask) &&
             (key == xkbi->inactiveKey)) {
        if (XkbAX_NeedFeedback(ctrls, XkbAX_BKRejectFBMask))
            XkbDDXAccessXBeep(keybd, _BEEP_BOUNCE_REJECT, XkbBounceKeysMask);
        ignoreKeyEvent = TRUE;
    }

    /* Start repeating if necessary.  Stop autorepeating if the user
     * presses a non-modifier key that doesn't autorepeat.
     */
    if (XkbDDXUsesSoftRepeat(keybd)) {
        if ((keybd->kbdfeed->ctrl.autoRepeat) &&
            ((ctrls->enabled_ctrls & (XkbSlowKeysMask | XkbRepeatKeysMask)) ==
             XkbRepeatKeysMask)) {
            if (BitIsOn(keybd->kbdfeed->ctrl.autoRepeats, key)) {
                if (xkbDebugFlags & 0x10)
                    DebugF("Starting software autorepeat...\n");
                if (xkbi->repeatKey == key)
                    ignoreKeyEvent = TRUE;
                else {
                    xkbi->repeatKey = key;
                    xkbi->repeatKeyTimer = TimerSet(xkbi->repeatKeyTimer,
                                                    0, ctrls->repeat_delay,
                                                    AccessXRepeatKeyExpire,
                                                    (void *) keybd);
                }
            }
        }
    }

    /* Check for two keys being pressed at the same time.  This section
     * essentially says the following:
     *
     *  If StickyKeys is on, and a modifier is currently being held down,
     *  and one of the following is true:  the current key is not a modifier
     *  or the currentKey is a modifier, but not the only modifier being
     *  held down, turn StickyKeys off if the TwoKeys off ctrl is set.
     */
    if ((ctrls->enabled_ctrls & XkbStickyKeysMask) &&
        (xkbi->state.base_mods != 0) &&
        (XkbAX_NeedOption(ctrls, XkbAX_TwoKeysMask))) {
        xkbControlsNotify cn;

        cn.keycode = key;
        cn.eventType = KeyPress;
        cn.requestMajor = 0;
        cn.requestMinor = 0;
        AccessXStickyKeysTurnOff(keybd, &cn);
    }

    if (!ignoreKeyEvent)
        XkbProcessKeyboardEvent(event, keybd);
    return ignoreKeyEvent;
}                               /* AccessXFilterPressEvent */

/************************************************************************/
/*									*/
/* AccessXFilterReleaseEvent						*/
/*									*/
/* Filter events before they get any further if SlowKeys is turned on.	*/
/* In addition, this routine handles the ever so popular magic key	*/
/* acts for turning various accessibility features on/off.		*/
/*									*/
/* Returns TRUE if this routine has discarded the event.		*/
/* Returns FALSE if the event needs further processing.			*/
/*									*/
/************************************************************************/
Bool
AccessXFilterReleaseEvent(DeviceEvent *event, DeviceIntPtr keybd)
{
    XkbSrvInfoPtr xkbi = keybd->key->xkbInfo;
    XkbControlsPtr ctrls = xkbi->desc->ctrls;
    KeyCode key = event->detail.key;
    Bool ignoreKeyEvent = FALSE;

    /* Don't transmit the KeyRelease if BounceKeys is on and
     * this is the release of a key that was ignored due to
     * BounceKeys.
     */
    if (ctrls->enabled_ctrls & XkbBounceKeysMask) {
        if ((key != xkbi->mouseKey) && (!BitIsOn(keybd->key->down, key)))
            ignoreKeyEvent = TRUE;
        xkbi->inactiveKey = key;
        xkbi->bounceKeysTimer = TimerSet(xkbi->bounceKeysTimer, 0,
                                         ctrls->debounce_delay,
                                         AccessXBounceKeyExpire,
                                         (void *) keybd);
    }

    /* Don't transmit the KeyRelease if SlowKeys is turned on and
     * the user didn't hold the key long enough.  We know we passed
     * the key if the down bit was set by CoreProcessKeyboadEvent.
     */
    if (ctrls->enabled_ctrls & XkbSlowKeysMask) {
        xkbAccessXNotify ev;
        unsigned beep_type;
        unsigned mask;

        ev.keycode = key;
        ev.slowKeysDelay = ctrls->slow_keys_delay;
        ev.debounceDelay = ctrls->debounce_delay;
        if (BitIsOn(keybd->key->down, key) || (xkbi->mouseKey == key)) {
            ev.detail = XkbAXN_SKRelease;
            beep_type = _BEEP_SLOW_RELEASE;
            mask = XkbAX_SKReleaseFBMask;
        }
        else {
            ev.detail = XkbAXN_SKReject;
            beep_type = _BEEP_SLOW_REJECT;
            mask = XkbAX_SKRejectFBMask;
            ignoreKeyEvent = TRUE;
        }
        XkbSendAccessXNotify(keybd, &ev);
        if (XkbAX_NeedFeedback(ctrls, mask)) {
            XkbDDXAccessXBeep(keybd, beep_type, XkbSlowKeysMask);
        }
        if (xkbi->slowKey == key)
            xkbi->slowKey = 0;
    }

    /* Stop Repeating if the user releases the key that is currently
     * repeating.
     */
    if (xkbi->repeatKey == key) {
        xkbi->repeatKey = 0;
    }

    if ((ctrls->enabled_ctrls & XkbAccessXTimeoutMask) &&
        (ctrls->ax_timeout > 0)) {
        xkbi->lastPtrEventTime = 0;
        xkbi->krgTimer = TimerSet(xkbi->krgTimer, 0,
                                  ctrls->ax_timeout * 1000,
                                  AccessXTimeoutExpire, (void *) keybd);
        xkbi->krgTimerActive = _ALL_TIMEOUT_TIMER;
    }
    else if (xkbi->krgTimerActive != _OFF_TIMER) {
        xkbi->krgTimer = TimerSet(xkbi->krgTimer, 0, 0, NULL, NULL);
        xkbi->krgTimerActive = _OFF_TIMER;
    }

    /* Keep track of how many times the Shift key has been pressed.
     * If it has been pressed and released 5 times in a row, toggle
     * the state of StickyKeys.
     */
    if ((!ignoreKeyEvent) && (xkbi->shiftKeyCount)) {
        KeySym *pSym = XkbKeySymsPtr(xkbi->desc, key);

        if ((pSym[0] != XK_Shift_L) && (pSym[0] != XK_Shift_R)) {
            xkbi->shiftKeyCount = 0;
        }
        else if (xkbi->shiftKeyCount >= 5) {
            xkbControlsNotify cn;

            cn.keycode = key;
            cn.eventType = KeyPress;
            cn.requestMajor = 0;
            cn.requestMinor = 0;
            if (ctrls->enabled_ctrls & XkbStickyKeysMask)
                AccessXStickyKeysTurnOff(keybd, &cn);
            else
                AccessXStickyKeysTurnOn(keybd, &cn);
            xkbi->shiftKeyCount = 0;
        }
    }

    if (!ignoreKeyEvent)
        XkbProcessKeyboardEvent(event, keybd);
    return ignoreKeyEvent;

}                               /* AccessXFilterReleaseEvent */

/************************************************************************/
/*									*/
/* ProcessPointerEvent							*/
/*									*/
/* This routine merely sets the shiftKeyCount and clears the keyboard   */
/* response group timer (if necessary) on a mouse event.  This is so	*/
/* multiple shifts with just the mouse and shift-drags with the mouse	*/
/* don't accidentally turn on StickyKeys or the Keyboard Response Group.*/
/*									*/
/************************************************************************/
extern int xkbDevicePrivateIndex;
void
ProcessPointerEvent(InternalEvent *ev, DeviceIntPtr mouse)
{
    DeviceIntPtr dev;
    XkbSrvInfoPtr xkbi = NULL;
    unsigned changed = 0;
    ProcessInputProc backupproc;
    xkbDeviceInfoPtr xkbPrivPtr = XKBDEVICEINFO(mouse);
    DeviceEvent *event = &ev->device_event;

    dev = IsFloating(mouse) ? mouse : GetMaster(mouse, MASTER_KEYBOARD);

    if (dev && dev->key) {
        xkbi = dev->key->xkbInfo;
        xkbi->shiftKeyCount = 0;
        xkbi->lastPtrEventTime = event->time;
    }

    if (event->type == ET_ButtonPress) {
        changed |= XkbPointerButtonMask;
    }
    else if (event->type == ET_ButtonRelease) {
        if (IsMaster(dev)) {
            DeviceIntPtr source;
            int rc;

            rc = dixLookupDevice(&source, event->sourceid, serverClient,
                    DixWriteAccess);
            if (rc != Success)
                ErrorF("[xkb] bad sourceid '%d' on button release event.\n",
                        event->sourceid);
            else if (!IsXTestDevice(source, GetMaster(dev, MASTER_POINTER))) {
                DeviceIntPtr xtest_device;

                xtest_device = GetXTestDevice(GetMaster(dev, MASTER_POINTER));
                if (button_is_down(xtest_device, ev->device_event.detail.button, BUTTON_PROCESSED))
                    XkbFakeDeviceButton(dev, FALSE, event->detail.key);
            }
        }

        if (xkbi)
            xkbi->lockedPtrButtons &= ~(1 << (event->detail.key & 0x7));

        changed |= XkbPointerButtonMask;
    }

    UNWRAP_PROCESS_INPUT_PROC(mouse, xkbPrivPtr, backupproc);
    mouse->public.processInputProc(ev, mouse);
    COND_WRAP_PROCESS_INPUT_PROC(mouse, xkbPrivPtr, backupproc, xkbUnwrapProc);

    if (!xkbi)
        return;

    xkbi->state.ptr_buttons = (mouse->button) ? mouse->button->state : 0;

    /* clear any latched modifiers */
    if (xkbi->state.latched_mods && (event->type == ET_ButtonRelease)) {
        unsigned changed_leds;
        XkbStateRec oldState;
        XkbSrvLedInfoPtr sli;

        sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId, 0);
        oldState = xkbi->state;
        XkbLatchModifiers(dev, 0xFF, 0x00);

        XkbComputeDerivedState(xkbi);
        changed |= XkbStateChangedFlags(&oldState, &xkbi->state);
        if (changed & sli->usedComponents) {
            changed_leds = XkbIndicatorsToUpdate(dev, changed, FALSE);
            if (changed_leds) {
                XkbEventCauseRec cause;

                XkbSetCauseKey(&cause, (event->detail.key & 0x7), event->type);
                XkbUpdateIndicators(dev, changed_leds, TRUE, NULL, &cause);
            }
        }
    }

    if (((xkbi->flags & _XkbStateNotifyInProgress) == 0) && (changed != 0)) {
        xkbStateNotify sn;

        sn.keycode = event->detail.key;
        sn.eventType = event->type;
        sn.requestMajor = sn.requestMinor = 0;
        sn.changed = changed;
        XkbSendStateNotify(dev, &sn);
    }

}                               /* ProcessPointerEvent */
