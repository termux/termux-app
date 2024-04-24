/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*****************************************************************
 * OS Dependent input routines:
 *
 *  WaitForSomething
 *  TimerForce, TimerSet, TimerCheck, TimerFree
 *
 *****************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef WIN32
#include <X11/Xwinsock.h>
#endif
#include <X11/Xos.h>            /* for strings, fcntl, time */
#include <errno.h>
#include <stdio.h>
#include <X11/X.h>
#include "misc.h"

#include "osdep.h"
#include "dixstruct.h"
#include "opaque.h"
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif
#include "busfault.h"

#ifdef WIN32
/* Error codes from windows sockets differ from fileio error codes  */
#undef EINTR
#define EINTR WSAEINTR
#undef EINVAL
#define EINVAL WSAEINVAL
#undef EBADF
#define EBADF WSAENOTSOCK
/* Windows select does not set errno. Use GetErrno as wrapper for
   WSAGetLastError */
#define GetErrno WSAGetLastError
#else
/* This is just a fallback to errno to hide the differences between unix and
   Windows in the code */
#define GetErrno() errno
#endif

#ifdef DPMSExtension
#include <X11/extensions/dpmsconst.h>
#endif

struct _OsTimerRec {
    struct xorg_list list;
    CARD32 expires;
    CARD32 delta;
    OsTimerCallback callback;
    void *arg;
};

static void DoTimer(OsTimerPtr timer, CARD32 now);
static void DoTimers(CARD32 now);
static void CheckAllTimers(void);
static volatile struct xorg_list timers;

static inline OsTimerPtr
first_timer(void)
{
    /* inline xorg_list_is_empty which can't handle volatile */
    if (timers.next == &timers)
        return NULL;
    return xorg_list_first_entry(&timers, struct _OsTimerRec, list);
}

/*
 * Compute timeout until next timer, running
 * any expired timers
 */
static int
check_timers(void)
{
    OsTimerPtr timer;

    if ((timer = first_timer()) != NULL) {
        CARD32 now = GetTimeInMillis();
        int timeout = timer->expires - now;

        if (timeout <= 0) {
            DoTimers(now);
        } else {
            /* Make sure the timeout is sane */
            if (timeout < timer->delta + 250)
                return timeout;

            /* time has rewound.  reset the timers. */
            CheckAllTimers();
        }

        return 0;
    }
    return -1;
}

/*****************
 * WaitForSomething:
 *     Make the server suspend until there is
 *	1. data from clients or
 *	2. input events available or
 *	3. ddx notices something of interest (graphics
 *	   queue ready, etc.) or
 *	4. clients that have buffered replies/events are ready
 *
 *     If the time between INPUT events is
 *     greater than ScreenSaverTime, the display is turned off (or
 *     saved, depending on the hardware).  So, WaitForSomething()
 *     has to handle this also (that's why the select() has a timeout.
 *     For more info on ClientsWithInput, see ReadRequestFromClient().
 *     pClientsReady is an array to store ready client->index values into.
 *****************/

Bool
WaitForSomething(Bool are_ready)
{
    int i;
    int timeout;
    int pollerr;
    static Bool were_ready;
    Bool timer_is_running;

    timer_is_running = were_ready;

    if (were_ready && !are_ready) {
        timer_is_running = FALSE;
        SmartScheduleStopTimer();
    }

    were_ready = FALSE;

#ifdef BUSFAULT
    busfault_check();
#endif

    /* We need a while loop here to handle
       crashed connections and the screen saver timeout */
    while (1) {
        /* deal with any blocked jobs */
        if (workQueue) {
            ProcessWorkQueue();
        }

        timeout = check_timers();
        are_ready = clients_are_ready();

        if (are_ready)
            timeout = 0;

        BlockHandler(&timeout);
        if (NewOutputPending)
            FlushAllOutput();
        /* keep this check close to select() call to minimize race */
        if (dispatchException)
            i = -1;
        else
            i = ospoll_wait(server_poll, timeout);
        pollerr = GetErrno();
        WakeupHandler(i);
        if (i <= 0) {           /* An error or timeout occurred */
            if (dispatchException)
                return FALSE;
            if (i < 0) {
                if (pollerr != EINTR && !ETEST(pollerr)) {
                    ErrorF("WaitForSomething(): poll: %s\n",
                           strerror(pollerr));
                }
            }
        } else
            are_ready = clients_are_ready();

        if (InputCheckPending())
            return FALSE;

        if (are_ready) {
            were_ready = TRUE;
            if (!timer_is_running)
                SmartScheduleStartTimer();
            return TRUE;
        }
    }
}

void
AdjustWaitForDelay(void *waitTime, int newdelay)
{
    int *timeoutp = waitTime;
    int timeout = *timeoutp;

    if (timeout < 0 || newdelay < timeout)
        *timeoutp = newdelay;
}

static inline Bool timer_pending(OsTimerPtr timer) {
    return !xorg_list_is_empty(&timer->list);
}

/* If time has rewound, re-run every affected timer.
 * Timers might drop out of the list, so we have to restart every time. */
static void
CheckAllTimers(void)
{
    OsTimerPtr timer;
    CARD32 now;

    input_lock();
 start:
    now = GetTimeInMillis();

    xorg_list_for_each_entry(timer, &timers, list) {
        if (timer->expires - now > timer->delta + 250) {
            DoTimer(timer, now);
            goto start;
        }
    }
    input_unlock();
}

static void
DoTimer(OsTimerPtr timer, CARD32 now)
{
    CARD32 newTime;

    xorg_list_del(&timer->list);
    newTime = (*timer->callback) (timer, now, timer->arg);
    if (newTime)
        TimerSet(timer, 0, newTime, timer->callback, timer->arg);
}

static void
DoTimers(CARD32 now)
{
    OsTimerPtr  timer;

    input_lock();
    while ((timer = first_timer())) {
        if ((int) (timer->expires - now) > 0)
            break;
        DoTimer(timer, now);
    }
    input_unlock();
}

OsTimerPtr
TimerSet(OsTimerPtr timer, int flags, CARD32 millis,
         OsTimerCallback func, void *arg)
{
    OsTimerPtr existing;
    CARD32 now = GetTimeInMillis();

    if (!timer) {
        timer = calloc(1, sizeof(struct _OsTimerRec));
        if (!timer)
            return NULL;
        xorg_list_init(&timer->list);
    }
    else {
        input_lock();
        if (timer_pending(timer)) {
            xorg_list_del(&timer->list);
            if (flags & TimerForceOld)
                (void) (*timer->callback) (timer, now, timer->arg);
        }
        input_unlock();
    }
    if (!millis)
        return timer;
    if (flags & TimerAbsolute) {
        timer->delta = millis - now;
    }
    else {
        timer->delta = millis;
        millis += now;
    }
    timer->expires = millis;
    timer->callback = func;
    timer->arg = arg;
    input_lock();

    /* Sort into list */
    xorg_list_for_each_entry(existing, &timers, list)
        if ((int) (existing->expires - millis) > 0)
            break;
    /* This even works at the end of the list -- existing->list will be timers */
    xorg_list_append(&timer->list, &existing->list);

    /* Check to see if the timer is ready to run now */
    if ((int) (millis - now) <= 0)
        DoTimer(timer, now);

    input_unlock();
    return timer;
}

Bool
TimerForce(OsTimerPtr timer)
{
    int pending;

    input_lock();
    pending = timer_pending(timer);
    if (pending)
        DoTimer(timer, GetTimeInMillis());
    input_unlock();
    return pending;
}

void
TimerCancel(OsTimerPtr timer)
{
    if (!timer)
        return;
    input_lock();
    xorg_list_del(&timer->list);
    input_unlock();
}

void
TimerFree(OsTimerPtr timer)
{
    if (!timer)
        return;
    TimerCancel(timer);
    free(timer);
}

void
TimerCheck(void)
{
    DoTimers(GetTimeInMillis());
}

void
TimerInit(void)
{
    static Bool been_here;
    OsTimerPtr timer, tmp;

    if (!been_here) {
        been_here = TRUE;
        xorg_list_init((struct xorg_list*) &timers);
    }

    xorg_list_for_each_entry_safe(timer, tmp, &timers, list) {
        xorg_list_del(&timer->list);
        free(timer);
    }
}

#ifdef DPMSExtension

#define DPMS_CHECK_MODE(mode,time)\
    if (time > 0 && DPMSPowerLevel < mode && timeout >= time)\
	DPMSSet(serverClient, mode);

#define DPMS_CHECK_TIMEOUT(time)\
    if (time > 0 && (time - timeout) > 0)\
	return time - timeout;

static CARD32
NextDPMSTimeout(INT32 timeout)
{
    /*
     * Return the amount of time remaining until we should set
     * the next power level. Fallthroughs are intentional.
     */
    switch (DPMSPowerLevel) {
    case DPMSModeOn:
        DPMS_CHECK_TIMEOUT(DPMSStandbyTime)

    case DPMSModeStandby:
        DPMS_CHECK_TIMEOUT(DPMSSuspendTime)

    case DPMSModeSuspend:
        DPMS_CHECK_TIMEOUT(DPMSOffTime)

    default:                   /* DPMSModeOff */
        return 0;
    }
}
#endif                          /* DPMSExtension */

static CARD32
ScreenSaverTimeoutExpire(OsTimerPtr timer, CARD32 now, void *arg)
{
    INT32 timeout = now - LastEventTime(XIAllDevices).milliseconds;
    CARD32 nextTimeout = 0;

#ifdef DPMSExtension
    /*
     * Check each mode lowest to highest, since a lower mode can
     * have the same timeout as a higher one.
     */
    if (DPMSEnabled) {
        DPMS_CHECK_MODE(DPMSModeOff, DPMSOffTime)
            DPMS_CHECK_MODE(DPMSModeSuspend, DPMSSuspendTime)
            DPMS_CHECK_MODE(DPMSModeStandby, DPMSStandbyTime)

            nextTimeout = NextDPMSTimeout(timeout);
    }

    /*
     * Only do the screensaver checks if we're not in a DPMS
     * power saving mode
     */
    if (DPMSPowerLevel != DPMSModeOn)
        return nextTimeout;
#endif                          /* DPMSExtension */

    if (!ScreenSaverTime)
        return nextTimeout;

    if (timeout < ScreenSaverTime) {
        return nextTimeout > 0 ?
            min(ScreenSaverTime - timeout, nextTimeout) :
            ScreenSaverTime - timeout;
    }

    ResetOsBuffers();           /* not ideal, but better than nothing */
    dixSaveScreens(serverClient, SCREEN_SAVER_ON, ScreenSaverActive);

    if (ScreenSaverInterval > 0) {
        nextTimeout = nextTimeout > 0 ?
            min(ScreenSaverInterval, nextTimeout) : ScreenSaverInterval;
    }

    return nextTimeout;
}

static OsTimerPtr ScreenSaverTimer = NULL;

void
FreeScreenSaverTimer(void)
{
    if (ScreenSaverTimer) {
        TimerFree(ScreenSaverTimer);
        ScreenSaverTimer = NULL;
    }
}

void
SetScreenSaverTimer(void)
{
    CARD32 timeout = 0;

#ifdef DPMSExtension
    if (DPMSEnabled) {
        /*
         * A higher DPMS level has a timeout that's either less
         * than or equal to that of a lower DPMS level.
         */
        if (DPMSStandbyTime > 0)
            timeout = DPMSStandbyTime;

        else if (DPMSSuspendTime > 0)
            timeout = DPMSSuspendTime;

        else if (DPMSOffTime > 0)
            timeout = DPMSOffTime;
    }
#endif

    if (ScreenSaverTime > 0) {
        timeout = timeout > 0 ? min(ScreenSaverTime, timeout) : ScreenSaverTime;
    }

#ifdef SCREENSAVER
    if (timeout && !screenSaverSuspended) {
#else
    if (timeout) {
#endif
        ScreenSaverTimer = TimerSet(ScreenSaverTimer, 0, timeout,
                                    ScreenSaverTimeoutExpire, NULL);
    }
    else if (ScreenSaverTimer) {
        FreeScreenSaverTimer();
    }
}
