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
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include <xkbsrv.h>
#include <X11/extensions/XI.h>

/*#define FALLING_TONE	1*/
/*#define RISING_TONE	1*/
#define FALLING_TONE	10
#define RISING_TONE	10
#define	SHORT_TONE	50
#define	SHORT_DELAY	60
#define	LONG_TONE	75
#define	VERY_LONG_TONE	100
#define	LONG_DELAY	85
#define CLICK_DURATION	1

#define	DEEP_PITCH	250
#define	LOW_PITCH	500
#define	MID_PITCH	1000
#define	HIGH_PITCH	2000
#define CLICK_PITCH	1500

static unsigned long atomGeneration = 0;
static Atom featureOn;
static Atom featureOff;
static Atom featureChange;
static Atom ledOn;
static Atom ledOff;
static Atom ledChange;
static Atom slowWarn;
static Atom slowPress;
static Atom slowReject;
static Atom slowAccept;
static Atom slowRelease;
static Atom stickyLatch;
static Atom stickyLock;
static Atom stickyUnlock;
static Atom bounceReject;
static char doesPitch = 1;

#define	FEATURE_ON	"AX_FeatureOn"
#define	FEATURE_OFF	"AX_FeatureOff"
#define	FEATURE_CHANGE	"AX_FeatureChange"
#define	LED_ON		"AX_IndicatorOn"
#define	LED_OFF		"AX_IndicatorOff"
#define	LED_CHANGE	"AX_IndicatorChange"
#define	SLOW_WARN	"AX_SlowKeysWarning"
#define	SLOW_PRESS	"AX_SlowKeyPress"
#define	SLOW_REJECT	"AX_SlowKeyReject"
#define	SLOW_ACCEPT	"AX_SlowKeyAccept"
#define	SLOW_RELEASE	"AX_SlowKeyRelease"
#define	STICKY_LATCH	"AX_StickyLatch"
#define	STICKY_LOCK	"AX_StickyLock"
#define	STICKY_UNLOCK	"AX_StickyUnlock"
#define	BOUNCE_REJECT	"AX_BounceKeyReject"

#define	MAKE_ATOM(a)	MakeAtom(a,sizeof(a)-1,TRUE)

static void
_XkbDDXBeepInitAtoms(void)
{
    featureOn = MAKE_ATOM(FEATURE_ON);
    featureOff = MAKE_ATOM(FEATURE_OFF);
    featureChange = MAKE_ATOM(FEATURE_CHANGE);
    ledOn = MAKE_ATOM(LED_ON);
    ledOff = MAKE_ATOM(LED_OFF);
    ledChange = MAKE_ATOM(LED_CHANGE);
    slowWarn = MAKE_ATOM(SLOW_WARN);
    slowPress = MAKE_ATOM(SLOW_PRESS);
    slowReject = MAKE_ATOM(SLOW_REJECT);
    slowAccept = MAKE_ATOM(SLOW_ACCEPT);
    slowRelease = MAKE_ATOM(SLOW_RELEASE);
    stickyLatch = MAKE_ATOM(STICKY_LATCH);
    stickyLock = MAKE_ATOM(STICKY_LOCK);
    stickyUnlock = MAKE_ATOM(STICKY_UNLOCK);
    bounceReject = MAKE_ATOM(BOUNCE_REJECT);
    return;
}

static CARD32
_XkbDDXBeepExpire(OsTimerPtr timer, CARD32 now, void *arg)
{
    DeviceIntPtr dev = (DeviceIntPtr) arg;
    KbdFeedbackPtr feed;
    KeybdCtrl *ctrl;
    XkbSrvInfoPtr xkbInfo;
    CARD32 next;
    int pitch, duration;
    int oldPitch, oldDuration;
    Atom name;

    if ((dev == NULL) || (dev->key == NULL) || (dev->key->xkbInfo == NULL) ||
        (dev->kbdfeed == NULL))
        return 0;
    if (atomGeneration != serverGeneration) {
        _XkbDDXBeepInitAtoms();
        atomGeneration = serverGeneration;
    }

    feed = dev->kbdfeed;
    ctrl = &feed->ctrl;
    xkbInfo = dev->key->xkbInfo;
    next = 0;
    pitch = oldPitch = ctrl->bell_pitch;
    duration = oldDuration = ctrl->bell_duration;
    name = None;
    switch (xkbInfo->beepType) {
    default:
        ErrorF("[xkb] Unknown beep type %d\n", xkbInfo->beepType);
    case _BEEP_NONE:
        duration = 0;
        break;

        /* When an LED is turned on, we want a high-pitched beep.
         * When the LED it turned off, we want a low-pitched beep.
         * If we cannot do pitch, we want a single beep for on and two
         * beeps for off.
         */
    case _BEEP_LED_ON:
        if (name == None)
            name = ledOn;
        duration = SHORT_TONE;
        pitch = HIGH_PITCH;
        break;
    case _BEEP_LED_OFF:
        if (name == None)
            name = ledOff;
        duration = SHORT_TONE;
        pitch = LOW_PITCH;
        if (!doesPitch && xkbInfo->beepCount < 1)
            next = SHORT_DELAY;
        break;

        /* When a Feature is turned on, we want an up-siren.
         * When a Feature is turned off, we want a down-siren.
         * If we cannot do pitch, we want a single beep for on and two
         * beeps for off.
         */
    case _BEEP_FEATURE_ON:
        if (name == None)
            name = featureOn;
        if (xkbInfo->beepCount < 1) {
            pitch = LOW_PITCH;
            duration = VERY_LONG_TONE;
            if (doesPitch)
                next = SHORT_DELAY;
        }
        else {
            pitch = MID_PITCH;
            duration = SHORT_TONE;
        }
        break;

    case _BEEP_FEATURE_OFF:
        if (name == None)
            name = featureOff;
        if (xkbInfo->beepCount < 1) {
            pitch = MID_PITCH;
            if (doesPitch)
                duration = VERY_LONG_TONE;
            else
                duration = SHORT_TONE;
            next = SHORT_DELAY;
        }
        else {
            pitch = LOW_PITCH;
            duration = SHORT_TONE;
        }
        break;

        /* Two high beeps indicate an LED or Feature changed
         * state, but that another LED or Feature is also on.
         * [[[WDW - This is not in AccessDOS ]]]
         */
    case _BEEP_LED_CHANGE:
        if (name == None)
            name = ledChange;
    case _BEEP_FEATURE_CHANGE:
        if (name == None)
            name = featureChange;
        duration = SHORT_TONE;
        pitch = HIGH_PITCH;
        if (xkbInfo->beepCount < 1) {
            next = SHORT_DELAY;
        }
        break;

        /* Three high-pitched beeps are the warning that SlowKeys
         * is going to be turned on or off.
         */
    case _BEEP_SLOW_WARN:
        if (name == None)
            name = slowWarn;
        duration = SHORT_TONE;
        pitch = HIGH_PITCH;
        if (xkbInfo->beepCount < 2)
            next = SHORT_DELAY;
        break;

        /* Click on SlowKeys press and accept.
         * Deep pitch when a SlowKey or BounceKey is rejected.
         * [[[WDW - Rejects are not in AccessDOS ]]]
         * If we cannot do pitch, we want single beeps.
         */
    case _BEEP_SLOW_PRESS:
        if (name == None)
            name = slowPress;
    case _BEEP_SLOW_ACCEPT:
        if (name == None)
            name = slowAccept;
    case _BEEP_SLOW_RELEASE:
        if (name == None)
            name = slowRelease;
        duration = CLICK_DURATION;
        pitch = CLICK_PITCH;
        break;
    case _BEEP_BOUNCE_REJECT:
        if (name == None)
            name = bounceReject;
    case _BEEP_SLOW_REJECT:
        if (name == None)
            name = slowReject;
        duration = SHORT_TONE;
        pitch = DEEP_PITCH;
        break;

        /* Low followed by high pitch when a StickyKey is latched.
         * High pitch when a StickyKey is locked.
         * Low pitch when unlocked.
         * If we cannot do pitch, two beeps for latch, nothing for
         * lock, and two for unlock.
         */
    case _BEEP_STICKY_LATCH:
        if (name == None)
            name = stickyLatch;
        duration = SHORT_TONE;
        if (xkbInfo->beepCount < 1) {
            next = SHORT_DELAY;
            pitch = LOW_PITCH;
        }
        else
            pitch = HIGH_PITCH;
        break;
    case _BEEP_STICKY_LOCK:
        if (name == None)
            name = stickyLock;
        if (doesPitch) {
            duration = SHORT_TONE;
            pitch = HIGH_PITCH;
        }
        break;
    case _BEEP_STICKY_UNLOCK:
        if (name == None)
            name = stickyUnlock;
        duration = SHORT_TONE;
        pitch = LOW_PITCH;
        if (!doesPitch && xkbInfo->beepCount < 1)
            next = SHORT_DELAY;
        break;
    }
    if (timer == NULL && duration > 0) {
        CARD32 starttime = GetTimeInMillis();
        CARD32 elapsedtime;

        ctrl->bell_duration = duration;
        ctrl->bell_pitch = pitch;
        if (xkbInfo->beepCount == 0) {
            XkbHandleBell(0, 0, dev, ctrl->bell, (void *) ctrl,
                          KbdFeedbackClass, name, None, NULL);
        }
        else if (xkbInfo->desc->ctrls->enabled_ctrls & XkbAudibleBellMask) {
            (*dev->kbdfeed->BellProc) (ctrl->bell, dev, (void *) ctrl,
                                       KbdFeedbackClass);
        }
        ctrl->bell_duration = oldDuration;
        ctrl->bell_pitch = oldPitch;
        xkbInfo->beepCount++;

        /* Some DDX schedule the beep and return immediately, others don't
           return until the beep is completed.  We measure the time and if
           it's less than the beep duration, make sure not to schedule the
           next beep until after the current one finishes. */

        elapsedtime = GetTimeInMillis();
        if (elapsedtime > starttime) {  /* watch out for millisecond counter
                                           overflow! */
            elapsedtime -= starttime;
        }
        else {
            elapsedtime = 0;
        }
        if (elapsedtime < duration) {
            next += duration - elapsedtime;
        }

    }
    return next;
}

int
XkbDDXAccessXBeep(DeviceIntPtr dev, unsigned what, unsigned which)
{
    XkbSrvInfoRec *xkbInfo = dev->key->xkbInfo;
    CARD32 next;

    xkbInfo->beepType = what;
    xkbInfo->beepCount = 0;
    next = _XkbDDXBeepExpire(NULL, 0, (void *) dev);
    if (next > 0) {
        xkbInfo->beepTimer = TimerSet(xkbInfo->beepTimer,
                                      0, next,
                                      _XkbDDXBeepExpire, (void *) dev);
    }
    return 1;
}
