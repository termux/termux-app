/*

Copyright 1991, 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

Copyright 1991, 1993 by Digital Equipment Corporation, Maynard, Massachusetts,
and Olivetti Research Limited, Cambridge, England.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital or Olivetti
not be used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  Digital and Olivetti
make no representations about the suitability of this software
for any purpose.  It is provided "as is" without express or implied warranty.

DIGITAL AND OLIVETTI DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL THEY BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xmd.h>
#include "scrnintstr.h"
#include "os.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "pixmapstr.h"
#include "resource.h"
#include "opaque.h"
#include <X11/extensions/syncproto.h>
#include "syncsrv.h"
#include "syncsdk.h"
#include "protocol-versions.h"
#include "inputstr.h"

#include <stdio.h>
#if !defined(WIN32)
#include <sys/time.h>
#endif

#include "extinit.h"

/*
 * Local Global Variables
 */
static int SyncEventBase;
static int SyncErrorBase;
static RESTYPE RTCounter = 0;
static RESTYPE RTAwait;
static RESTYPE RTAlarm;
static RESTYPE RTAlarmClient;
static RESTYPE RTFence;
static struct xorg_list SysCounterList;
static int SyncNumInvalidCounterWarnings = 0;

#define MAX_INVALID_COUNTER_WARNINGS	   5

static const char *WARN_INVALID_COUNTER_COMPARE =
    "Warning: Non-counter XSync object using Counter-only\n"
    "         comparison.  Result will never be true.\n";

static const char *WARN_INVALID_COUNTER_ALARM =
    "Warning: Non-counter XSync object used in alarm.  This is\n"
    "         the result of a programming error in the X server.\n";

#define IsSystemCounter(pCounter) \
    (pCounter && (pCounter->sync.client == NULL))

/* these are all the alarm attributes that pertain to the alarm's trigger */
#define XSyncCAAllTrigger \
    (XSyncCACounter | XSyncCAValueType | XSyncCAValue | XSyncCATestType)

static void SyncComputeBracketValues(SyncCounter *);

static void SyncInitServerTime(void);

static void SyncInitIdleTime(void);

static inline void*
SysCounterGetPrivate(SyncCounter *counter)
{
    BUG_WARN(!IsSystemCounter(counter));

    return counter->pSysCounterInfo ? counter->pSysCounterInfo->private : NULL;
}

static Bool
SyncCheckWarnIsCounter(const SyncObject * pSync, const char *warning)
{
    if (pSync && (SYNC_COUNTER != pSync->type)) {
        if (SyncNumInvalidCounterWarnings++ < MAX_INVALID_COUNTER_WARNINGS) {
            ErrorF("%s", warning);
            ErrorF("         Counter type: %d\n", pSync->type);
        }

        return FALSE;
    }

    return TRUE;
}

/*  Each counter maintains a simple linked list of triggers that are
 *  interested in the counter.  The two functions below are used to
 *  delete and add triggers on this list.
 */
void
SyncDeleteTriggerFromSyncObject(SyncTrigger * pTrigger)
{
    SyncTriggerList *pCur;
    SyncTriggerList *pPrev;
    SyncCounter *pCounter;

    /* pSync needs to be stored in pTrigger before calling here. */

    if (!pTrigger->pSync)
        return;

    pPrev = NULL;
    pCur = pTrigger->pSync->pTriglist;

    while (pCur) {
        if (pCur->pTrigger == pTrigger) {
            if (pPrev)
                pPrev->next = pCur->next;
            else
                pTrigger->pSync->pTriglist = pCur->next;

            free(pCur);
            break;
        }

        pPrev = pCur;
        pCur = pCur->next;
    }

    if (SYNC_COUNTER == pTrigger->pSync->type) {
        pCounter = (SyncCounter *) pTrigger->pSync;

        if (IsSystemCounter(pCounter))
            SyncComputeBracketValues(pCounter);
    }
    else if (SYNC_FENCE == pTrigger->pSync->type) {
        SyncFence *pFence = (SyncFence *) pTrigger->pSync;

        pFence->funcs.DeleteTrigger(pTrigger);
    }
}

int
SyncAddTriggerToSyncObject(SyncTrigger * pTrigger)
{
    SyncTriggerList *pCur;
    SyncCounter *pCounter;

    if (!pTrigger->pSync)
        return Success;

    /* don't do anything if it's already there */
    for (pCur = pTrigger->pSync->pTriglist; pCur; pCur = pCur->next) {
        if (pCur->pTrigger == pTrigger)
            return Success;
    }

    if (!(pCur = malloc(sizeof(SyncTriggerList))))
        return BadAlloc;

    pCur->pTrigger = pTrigger;
    pCur->next = pTrigger->pSync->pTriglist;
    pTrigger->pSync->pTriglist = pCur;

    if (SYNC_COUNTER == pTrigger->pSync->type) {
        pCounter = (SyncCounter *) pTrigger->pSync;

        if (IsSystemCounter(pCounter))
            SyncComputeBracketValues(pCounter);
    }
    else if (SYNC_FENCE == pTrigger->pSync->type) {
        SyncFence *pFence = (SyncFence *) pTrigger->pSync;

        pFence->funcs.AddTrigger(pTrigger);
    }

    return Success;
}

/*  Below are five possible functions that can be plugged into
 *  pTrigger->CheckTrigger for counter sync objects, corresponding to
 *  the four possible test-types, and the one possible function that
 *  can be plugged into pTrigger->CheckTrigger for fence sync objects.
 *  These functions are called after the sync object's state changes
 *  but are also passed the old state so they can inspect both the old
 *  and new values.  (PositiveTransition and NegativeTransition need to
 *  see both pieces of information.)  These functions return the truth
 *  value of the trigger.
 *
 *  All of them include the condition pTrigger->pSync == NULL.
 *  This is because the spec says that a trigger with a sync value
 *  of None is always TRUE.
 */

static Bool
SyncCheckTriggerPositiveComparison(SyncTrigger * pTrigger, int64_t oldval)
{
    SyncCounter *pCounter;

    /* Non-counter sync objects should never get here because they
     * never trigger this comparison. */
    if (!SyncCheckWarnIsCounter(pTrigger->pSync, WARN_INVALID_COUNTER_COMPARE))
        return FALSE;

    pCounter = (SyncCounter *) pTrigger->pSync;

    return pCounter == NULL || pCounter->value >= pTrigger->test_value;
}

static Bool
SyncCheckTriggerNegativeComparison(SyncTrigger * pTrigger, int64_t oldval)
{
    SyncCounter *pCounter;

    /* Non-counter sync objects should never get here because they
     * never trigger this comparison. */
    if (!SyncCheckWarnIsCounter(pTrigger->pSync, WARN_INVALID_COUNTER_COMPARE))
        return FALSE;

    pCounter = (SyncCounter *) pTrigger->pSync;

    return pCounter == NULL || pCounter->value <= pTrigger->test_value;
}

static Bool
SyncCheckTriggerPositiveTransition(SyncTrigger * pTrigger, int64_t oldval)
{
    SyncCounter *pCounter;

    /* Non-counter sync objects should never get here because they
     * never trigger this comparison. */
    if (!SyncCheckWarnIsCounter(pTrigger->pSync, WARN_INVALID_COUNTER_COMPARE))
        return FALSE;

    pCounter = (SyncCounter *) pTrigger->pSync;

    return (pCounter == NULL ||
            (oldval < pTrigger->test_value &&
             pCounter->value >= pTrigger->test_value));
}

static Bool
SyncCheckTriggerNegativeTransition(SyncTrigger * pTrigger, int64_t oldval)
{
    SyncCounter *pCounter;

    /* Non-counter sync objects should never get here because they
     * never trigger this comparison. */
    if (!SyncCheckWarnIsCounter(pTrigger->pSync, WARN_INVALID_COUNTER_COMPARE))
        return FALSE;

    pCounter = (SyncCounter *) pTrigger->pSync;

    return (pCounter == NULL ||
            (oldval > pTrigger->test_value &&
             pCounter->value <= pTrigger->test_value));
}

static Bool
SyncCheckTriggerFence(SyncTrigger * pTrigger, int64_t unused)
{
    SyncFence *pFence = (SyncFence *) pTrigger->pSync;

    (void) unused;

    return (pFence == NULL || pFence->funcs.CheckTriggered(pFence));
}

static int
SyncInitTrigger(ClientPtr client, SyncTrigger * pTrigger, XID syncObject,
                RESTYPE resType, Mask changes)
{
    SyncObject *pSync = pTrigger->pSync;
    SyncCounter *pCounter = NULL;
    int rc;
    Bool newSyncObject = FALSE;

    if (changes & XSyncCACounter) {
        if (syncObject == None)
            pSync = NULL;
        else if (Success != (rc = dixLookupResourceByType((void **) &pSync,
                                                          syncObject, resType,
                                                          client,
                                                          DixReadAccess))) {
            client->errorValue = syncObject;
            return rc;
        }
        if (pSync != pTrigger->pSync) { /* new counter for trigger */
            SyncDeleteTriggerFromSyncObject(pTrigger);
            pTrigger->pSync = pSync;
            newSyncObject = TRUE;
        }
    }

    /* if system counter, ask it what the current value is */

    if (pSync && SYNC_COUNTER == pSync->type) {
        pCounter = (SyncCounter *) pSync;

        if (IsSystemCounter(pCounter)) {
            (*pCounter->pSysCounterInfo->QueryValue) ((void *) pCounter,
                                                      &pCounter->value);
        }
    }

    if (changes & XSyncCAValueType) {
        if (pTrigger->value_type != XSyncRelative &&
            pTrigger->value_type != XSyncAbsolute) {
            client->errorValue = pTrigger->value_type;
            return BadValue;
        }
    }

    if (changes & XSyncCATestType) {

        if (pSync && SYNC_FENCE == pSync->type) {
            pTrigger->CheckTrigger = SyncCheckTriggerFence;
        }
        else {
            /* select appropriate CheckTrigger function */

            switch (pTrigger->test_type) {
            case XSyncPositiveTransition:
                pTrigger->CheckTrigger = SyncCheckTriggerPositiveTransition;
                break;
            case XSyncNegativeTransition:
                pTrigger->CheckTrigger = SyncCheckTriggerNegativeTransition;
                break;
            case XSyncPositiveComparison:
                pTrigger->CheckTrigger = SyncCheckTriggerPositiveComparison;
                break;
            case XSyncNegativeComparison:
                pTrigger->CheckTrigger = SyncCheckTriggerNegativeComparison;
                break;
            default:
                client->errorValue = pTrigger->test_type;
                return BadValue;
            }
        }
    }

    if (changes & (XSyncCAValueType | XSyncCAValue)) {
        if (pTrigger->value_type == XSyncAbsolute)
            pTrigger->test_value = pTrigger->wait_value;
        else {                  /* relative */
            Bool overflow;

            if (pCounter == NULL)
                return BadMatch;

            overflow = checked_int64_add(&pTrigger->test_value,
                                         pCounter->value, pTrigger->wait_value);
            if (overflow) {
                client->errorValue = pTrigger->wait_value >> 32;
                return BadValue;
            }
        }
    }

    /*  we wait until we're sure there are no errors before registering
     *  a new counter on a trigger
     */
    if (newSyncObject) {
        if ((rc = SyncAddTriggerToSyncObject(pTrigger)) != Success)
            return rc;
    }
    else if (pCounter && IsSystemCounter(pCounter)) {
        SyncComputeBracketValues(pCounter);
    }

    return Success;
}

/*  AlarmNotify events happen in response to actions taken on an Alarm or
 *  the counter used by the alarm.  AlarmNotify may be sent to multiple
 *  clients.  The alarm maintains a list of clients interested in events.
 */
static void
SyncSendAlarmNotifyEvents(SyncAlarm * pAlarm)
{
    SyncAlarmClientList *pcl;
    xSyncAlarmNotifyEvent ane;
    SyncTrigger *pTrigger = &pAlarm->trigger;
    SyncCounter *pCounter;

    if (!SyncCheckWarnIsCounter(pTrigger->pSync, WARN_INVALID_COUNTER_ALARM))
        return;

    pCounter = (SyncCounter *) pTrigger->pSync;

    UpdateCurrentTime();

    ane = (xSyncAlarmNotifyEvent) {
        .type = SyncEventBase + XSyncAlarmNotify,
        .kind = XSyncAlarmNotify,
        .alarm = pAlarm->alarm_id,
        .alarm_value_hi = pTrigger->test_value >> 32,
        .alarm_value_lo = pTrigger->test_value,
        .time = currentTime.milliseconds,
        .state = pAlarm->state
    };

    if (pTrigger->pSync && SYNC_COUNTER == pTrigger->pSync->type) {
        ane.counter_value_hi = pCounter->value >> 32;
        ane.counter_value_lo = pCounter->value;
    }
    else {
        /* XXX what else can we do if there's no counter? */
        ane.counter_value_hi = ane.counter_value_lo = 0;
    }

    /* send to owner */
    if (pAlarm->events)
        WriteEventsToClient(pAlarm->client, 1, (xEvent *) &ane);

    /* send to other interested clients */
    for (pcl = pAlarm->pEventClients; pcl; pcl = pcl->next)
        WriteEventsToClient(pcl->client, 1, (xEvent *) &ane);
}

/*  CounterNotify events only occur in response to an Await.  The events
 *  go only to the Awaiting client.
 */
static void
SyncSendCounterNotifyEvents(ClientPtr client, SyncAwait ** ppAwait,
                            int num_events)
{
    xSyncCounterNotifyEvent *pEvents, *pev;
    int i;

    if (client->clientGone)
        return;
    pev = pEvents = calloc(num_events, sizeof(xSyncCounterNotifyEvent));
    if (!pEvents)
        return;
    UpdateCurrentTime();
    for (i = 0; i < num_events; i++, ppAwait++, pev++) {
        SyncTrigger *pTrigger = &(*ppAwait)->trigger;

        pev->type = SyncEventBase + XSyncCounterNotify;
        pev->kind = XSyncCounterNotify;
        pev->counter = pTrigger->pSync->id;
        pev->wait_value_lo = pTrigger->test_value;
        pev->wait_value_hi = pTrigger->test_value >> 32;
        if (SYNC_COUNTER == pTrigger->pSync->type) {
            SyncCounter *pCounter = (SyncCounter *) pTrigger->pSync;

            pev->counter_value_lo = pCounter->value;
            pev->counter_value_hi = pCounter->value >> 32;
        }
        else {
            pev->counter_value_lo = 0;
            pev->counter_value_hi = 0;
        }

        pev->time = currentTime.milliseconds;
        pev->count = num_events - i - 1;        /* events remaining */
        pev->destroyed = pTrigger->pSync->beingDestroyed;
    }
    /* swapping will be taken care of by this */
    WriteEventsToClient(client, num_events, (xEvent *) pEvents);
    free(pEvents);
}

/* This function is called when an alarm's counter is destroyed.
 * It is plugged into pTrigger->CounterDestroyed (for alarm triggers).
 */
static void
SyncAlarmCounterDestroyed(SyncTrigger * pTrigger)
{
    SyncAlarm *pAlarm = (SyncAlarm *) pTrigger;

    pAlarm->state = XSyncAlarmInactive;
    SyncSendAlarmNotifyEvents(pAlarm);
    pTrigger->pSync = NULL;
}

/*  This function is called when an alarm "goes off."
 *  It is plugged into pTrigger->TriggerFired (for alarm triggers).
 */
static void
SyncAlarmTriggerFired(SyncTrigger * pTrigger)
{
    SyncAlarm *pAlarm = (SyncAlarm *) pTrigger;
    SyncCounter *pCounter;
    int64_t new_test_value;

    if (!SyncCheckWarnIsCounter(pTrigger->pSync, WARN_INVALID_COUNTER_ALARM))
        return;

    pCounter = (SyncCounter *) pTrigger->pSync;

    /* no need to check alarm unless it's active */
    if (pAlarm->state != XSyncAlarmActive)
        return;

    /*  " if the counter value is None, or if the delta is 0 and
     *    the test-type is PositiveComparison or NegativeComparison,
     *    no change is made to value (test-value) and the alarm
     *    state is changed to Inactive before the event is generated."
     */
    if (pCounter == NULL || (pAlarm->delta == 0
                             && (pAlarm->trigger.test_type ==
                                 XSyncPositiveComparison ||
                                 pAlarm->trigger.test_type ==
                                 XSyncNegativeComparison)))
        pAlarm->state = XSyncAlarmInactive;

    new_test_value = pAlarm->trigger.test_value;

    if (pAlarm->state == XSyncAlarmActive) {
        Bool overflow;
        int64_t oldvalue;
        SyncTrigger *paTrigger = &pAlarm->trigger;
        SyncCounter *paCounter;

        if (!SyncCheckWarnIsCounter(paTrigger->pSync,
                                    WARN_INVALID_COUNTER_ALARM))
            return;

        paCounter = (SyncCounter *) pTrigger->pSync;

        /* "The alarm is updated by repeatedly adding delta to the
         *  value of the trigger and re-initializing it until it
         *  becomes FALSE."
         */
        oldvalue = paTrigger->test_value;

        /* XXX really should do something smarter here */

        do {
            overflow = checked_int64_add(&paTrigger->test_value,
                                         paTrigger->test_value, pAlarm->delta);
        } while (!overflow &&
                 (*paTrigger->CheckTrigger) (paTrigger, paCounter->value));

        new_test_value = paTrigger->test_value;
        paTrigger->test_value = oldvalue;

        /* "If this update would cause value to fall outside the range
         *  for an INT64...no change is made to value (test-value) and
         *  the alarm state is changed to Inactive before the event is
         *  generated."
         */
        if (overflow) {
            new_test_value = oldvalue;
            pAlarm->state = XSyncAlarmInactive;
        }
    }
    /*  The AlarmNotify event has to have the "new state of the alarm"
     *  which we can't be sure of until this point.  However, it has
     *  to have the "old" trigger test value.  That's the reason for
     *  all the newvalue/oldvalue shuffling above.  After we send the
     *  events, give the trigger its new test value.
     */
    SyncSendAlarmNotifyEvents(pAlarm);
    pTrigger->test_value = new_test_value;
}

/*  This function is called when an Await unblocks, either as a result
 *  of the trigger firing OR the counter being destroyed.
 *  It goes into pTrigger->TriggerFired AND pTrigger->CounterDestroyed
 *  (for Await triggers).
 */
static void
SyncAwaitTriggerFired(SyncTrigger * pTrigger)
{
    SyncAwait *pAwait = (SyncAwait *) pTrigger;
    int numwaits;
    SyncAwaitUnion *pAwaitUnion;
    SyncAwait **ppAwait;
    int num_events = 0;

    pAwaitUnion = (SyncAwaitUnion *) pAwait->pHeader;
    numwaits = pAwaitUnion->header.num_waitconditions;
    ppAwait = xallocarray(numwaits, sizeof(SyncAwait *));
    if (!ppAwait)
        goto bail;

    pAwait = &(pAwaitUnion + 1)->await;

    /* "When a client is unblocked, all the CounterNotify events for
     *  the Await request are generated contiguously. If count is 0
     *  there are no more events to follow for this request. If
     *  count is n, there are at least n more events to follow."
     *
     *  Thus, it is best to find all the counters for which events
     *  need to be sent first, so that an accurate count field can
     *  be stored in the events.
     */
    for (; numwaits; numwaits--, pAwait++) {
        int64_t diff;
        Bool overflow, diffgreater, diffequal;

        /* "A CounterNotify event with the destroyed flag set to TRUE is
         *  always generated if the counter for one of the triggers is
         *  destroyed."
         */
        if (pAwait->trigger.pSync->beingDestroyed) {
            ppAwait[num_events++] = pAwait;
            continue;
        }

        if (SYNC_COUNTER == pAwait->trigger.pSync->type) {
            SyncCounter *pCounter = (SyncCounter *) pAwait->trigger.pSync;

            /* "The difference between the counter and the test value is
             *  calculated by subtracting the test value from the value of
             *  the counter."
             */
            overflow = checked_int64_subtract(&diff, pCounter->value,
                                              pAwait->trigger.test_value);

            /* "If the difference lies outside the range for an INT64, an
             *  event is not generated."
             */
            if (overflow)
                continue;
            diffgreater = diff > pAwait->event_threshold;
            diffequal = diff == pAwait->event_threshold;

            /* "If the test-type is PositiveTransition or
             *  PositiveComparison, a CounterNotify event is generated if
             *  the difference is at least event-threshold. If the test-type
             *  is NegativeTransition or NegativeComparison, a CounterNotify
             *  event is generated if the difference is at most
             *  event-threshold."
             */

            if (((pAwait->trigger.test_type == XSyncPositiveComparison ||
                  pAwait->trigger.test_type == XSyncPositiveTransition)
                 && (diffgreater || diffequal))
                ||
                ((pAwait->trigger.test_type == XSyncNegativeComparison ||
                  pAwait->trigger.test_type == XSyncNegativeTransition)
                 && (!diffgreater)      /* less or equal */
                )
                ) {
                ppAwait[num_events++] = pAwait;
            }
        }
    }
    if (num_events)
        SyncSendCounterNotifyEvents(pAwaitUnion->header.client, ppAwait,
                                    num_events);
    free(ppAwait);

 bail:
    /* unblock the client */
    AttendClient(pAwaitUnion->header.client);
    /* delete the await */
    FreeResource(pAwaitUnion->header.delete_id, RT_NONE);
}

static int64_t
SyncUpdateCounter(SyncCounter *pCounter, int64_t newval)
{
    int64_t oldval = pCounter->value;
    pCounter->value = newval;
    return oldval;
}

/*  This function should always be used to change a counter's value so that
 *  any triggers depending on the counter will be checked.
 */
void
SyncChangeCounter(SyncCounter * pCounter, int64_t newval)
{
    SyncTriggerList *ptl, *pnext;
    int64_t oldval;

    oldval = SyncUpdateCounter(pCounter, newval);

    /* run through triggers to see if any become true */
    for (ptl = pCounter->sync.pTriglist; ptl; ptl = pnext) {
        pnext = ptl->next;
        if ((*ptl->pTrigger->CheckTrigger) (ptl->pTrigger, oldval))
            (*ptl->pTrigger->TriggerFired) (ptl->pTrigger);
    }

    if (IsSystemCounter(pCounter)) {
        SyncComputeBracketValues(pCounter);
    }
}

/* loosely based on dix/events.c/EventSelectForWindow */
static Bool
SyncEventSelectForAlarm(SyncAlarm * pAlarm, ClientPtr client, Bool wantevents)
{
    SyncAlarmClientList *pClients;

    if (client == pAlarm->client) {     /* alarm owner */
        pAlarm->events = wantevents;
        return Success;
    }

    /* see if the client is already on the list (has events selected) */

    for (pClients = pAlarm->pEventClients; pClients; pClients = pClients->next) {
        if (pClients->client == client) {
            /* client's presence on the list indicates desire for
             * events.  If the client doesn't want events, remove it
             * from the list.  If the client does want events, do
             * nothing, since it's already got them.
             */
            if (!wantevents) {
                FreeResource(pClients->delete_id, RT_NONE);
            }
            return Success;
        }
    }

    /*  if we get here, this client does not currently have
     *  events selected on the alarm
     */

    if (!wantevents)
        /* client doesn't want events, and we just discovered that it
         * doesn't have them, so there's nothing to do.
         */
        return Success;

    /* add new client to pAlarm->pEventClients */

    pClients = malloc(sizeof(SyncAlarmClientList));
    if (!pClients)
        return BadAlloc;

    /*  register it as a resource so it will be cleaned up
     *  if the client dies
     */

    pClients->delete_id = FakeClientID(client->index);

    /* link it into list after we know all the allocations succeed */
    pClients->next = pAlarm->pEventClients;
    pAlarm->pEventClients = pClients;
    pClients->client = client;

    if (!AddResource(pClients->delete_id, RTAlarmClient, pAlarm))
        return BadAlloc;

    return Success;
}

/*
 * ** SyncChangeAlarmAttributes ** This is used by CreateAlarm and ChangeAlarm
 */
static int
SyncChangeAlarmAttributes(ClientPtr client, SyncAlarm * pAlarm, Mask mask,
                          CARD32 *values)
{
    int status;
    XSyncCounter counter;
    Mask origmask = mask;

    counter = pAlarm->trigger.pSync ? pAlarm->trigger.pSync->id : None;

    while (mask) {
        int index2 = lowbit(mask);

        mask &= ~index2;
        switch (index2) {
        case XSyncCACounter:
            mask &= ~XSyncCACounter;
            /* sanity check in SyncInitTrigger */
            counter = *values++;
            break;

        case XSyncCAValueType:
            mask &= ~XSyncCAValueType;
            /* sanity check in SyncInitTrigger */
            pAlarm->trigger.value_type = *values++;
            break;

        case XSyncCAValue:
            mask &= ~XSyncCAValue;
            pAlarm->trigger.wait_value = ((int64_t)values[0] << 32) | values[1];
            values += 2;
            break;

        case XSyncCATestType:
            mask &= ~XSyncCATestType;
            /* sanity check in SyncInitTrigger */
            pAlarm->trigger.test_type = *values++;
            break;

        case XSyncCADelta:
            mask &= ~XSyncCADelta;
            pAlarm->delta = ((int64_t)values[0] << 32) | values[1];
            values += 2;
            break;

        case XSyncCAEvents:
            mask &= ~XSyncCAEvents;
            if ((*values != xTrue) && (*values != xFalse)) {
                client->errorValue = *values;
                return BadValue;
            }
            status = SyncEventSelectForAlarm(pAlarm, client,
                                             (Bool) (*values++));
            if (status != Success)
                return status;
            break;

        default:
            client->errorValue = mask;
            return BadValue;
        }
    }

    /* "If the test-type is PositiveComparison or PositiveTransition
     *  and delta is less than zero, or if the test-type is
     *  NegativeComparison or NegativeTransition and delta is
     *  greater than zero, a Match error is generated."
     */
    if (origmask & (XSyncCADelta | XSyncCATestType)) {
        if ((((pAlarm->trigger.test_type == XSyncPositiveComparison) ||
              (pAlarm->trigger.test_type == XSyncPositiveTransition))
             && pAlarm->delta < 0)
            ||
            (((pAlarm->trigger.test_type == XSyncNegativeComparison) ||
              (pAlarm->trigger.test_type == XSyncNegativeTransition))
             && pAlarm->delta > 0)
            ) {
            return BadMatch;
        }
    }

    /* postpone this until now, when we're sure nothing else can go wrong */
    if ((status = SyncInitTrigger(client, &pAlarm->trigger, counter, RTCounter,
                                  origmask & XSyncCAAllTrigger)) != Success)
        return status;

    /* XXX spec does not really say to do this - needs clarification */
    pAlarm->state = XSyncAlarmActive;
    return Success;
}

SyncObject *
SyncCreate(ClientPtr client, XID id, unsigned char type)
{
    SyncObject *pSync;
    RESTYPE resType;

    switch (type) {
    case SYNC_COUNTER:
        pSync = malloc(sizeof(SyncCounter));
        resType = RTCounter;
        break;
    case SYNC_FENCE:
        pSync = (SyncObject *) dixAllocateObjectWithPrivates(SyncFence,
                                                             PRIVATE_SYNC_FENCE);
        resType = RTFence;
        break;
    default:
        return NULL;
    }

    if (!pSync)
        return NULL;

    pSync->initialized = FALSE;

    if (!AddResource(id, resType, (void *) pSync))
        return NULL;

    pSync->client = client;
    pSync->id = id;
    pSync->pTriglist = NULL;
    pSync->beingDestroyed = FALSE;
    pSync->type = type;

    return pSync;
}

int
SyncCreateFenceFromFD(ClientPtr client, DrawablePtr pDraw, XID id, int fd, BOOL initially_triggered)
{
#ifdef HAVE_XSHMFENCE
    SyncFence  *pFence;
    int         status;

    pFence = (SyncFence *) SyncCreate(client, id, SYNC_FENCE);
    if (!pFence)
        return BadAlloc;

    status = miSyncInitFenceFromFD(pDraw, pFence, fd, initially_triggered);
    if (status != Success) {
        FreeResource(pFence->sync.id, RT_NONE);
        return status;
    }

    return Success;
#else
    return BadImplementation;
#endif
}

int
SyncFDFromFence(ClientPtr client, DrawablePtr pDraw, SyncFence *pFence)
{
#ifdef HAVE_XSHMFENCE
    return miSyncFDFromFence(pDraw, pFence);
#else
    return BadImplementation;
#endif
}

static SyncCounter *
SyncCreateCounter(ClientPtr client, XSyncCounter id, int64_t initialvalue)
{
    SyncCounter *pCounter;

    if (!(pCounter = (SyncCounter *) SyncCreate(client, id, SYNC_COUNTER)))
        return NULL;

    pCounter->value = initialvalue;
    pCounter->pSysCounterInfo = NULL;

    pCounter->sync.initialized = TRUE;

    return pCounter;
}

static int FreeCounter(void *, XID);

/*
 * ***** System Counter utilities
 */

SyncCounter*
SyncCreateSystemCounter(const char *name,
                        int64_t initial,
                        int64_t resolution,
                        SyncCounterType counterType,
                        SyncSystemCounterQueryValue QueryValue,
                        SyncSystemCounterBracketValues BracketValues
    )
{
    SyncCounter *pCounter = SyncCreateCounter(NULL, FakeClientID(0), initial);

    if (pCounter) {
        SysCounterInfo *psci;

        psci = malloc(sizeof(SysCounterInfo));
        if (!psci) {
            FreeResource(pCounter->sync.id, RT_NONE);
            return pCounter;
        }
        pCounter->pSysCounterInfo = psci;
        psci->pCounter = pCounter;
        psci->name = strdup(name);
        psci->resolution = resolution;
        psci->counterType = counterType;
        psci->QueryValue = QueryValue;
        psci->BracketValues = BracketValues;
        psci->private = NULL;
        psci->bracket_greater = LLONG_MAX;
        psci->bracket_less = LLONG_MIN;
        xorg_list_add(&psci->entry, &SysCounterList);
    }
    return pCounter;
}

void
SyncDestroySystemCounter(void *pSysCounter)
{
    SyncCounter *pCounter = (SyncCounter *) pSysCounter;

    FreeResource(pCounter->sync.id, RT_NONE);
}

static void
SyncComputeBracketValues(SyncCounter * pCounter)
{
    SyncTriggerList *pCur;
    SyncTrigger *pTrigger;
    SysCounterInfo *psci;
    int64_t *pnewgtval = NULL;
    int64_t *pnewltval = NULL;
    SyncCounterType ct;

    if (!pCounter)
        return;

    psci = pCounter->pSysCounterInfo;
    ct = pCounter->pSysCounterInfo->counterType;
    if (ct == XSyncCounterNeverChanges)
        return;

    psci->bracket_greater = LLONG_MAX;
    psci->bracket_less = LLONG_MIN;

    for (pCur = pCounter->sync.pTriglist; pCur; pCur = pCur->next) {
        pTrigger = pCur->pTrigger;

        if (pTrigger->test_type == XSyncPositiveComparison &&
            ct != XSyncCounterNeverIncreases) {
            if (pCounter->value < pTrigger->test_value &&
                pTrigger->test_value < psci->bracket_greater) {
                psci->bracket_greater = pTrigger->test_value;
                pnewgtval = &psci->bracket_greater;
            }
            else if (pCounter->value > pTrigger->test_value &&
                     pTrigger->test_value > psci->bracket_less) {
                    psci->bracket_less = pTrigger->test_value;
                    pnewltval = &psci->bracket_less;
            }
        }
        else if (pTrigger->test_type == XSyncNegativeComparison &&
                 ct != XSyncCounterNeverDecreases) {
            if (pCounter->value > pTrigger->test_value &&
                pTrigger->test_value > psci->bracket_less) {
                psci->bracket_less = pTrigger->test_value;
                pnewltval = &psci->bracket_less;
            }
            else if (pCounter->value < pTrigger->test_value &&
                     pTrigger->test_value < psci->bracket_greater) {
                    psci->bracket_greater = pTrigger->test_value;
                    pnewgtval = &psci->bracket_greater;
            }
        }
        else if (pTrigger->test_type == XSyncNegativeTransition &&
                 ct != XSyncCounterNeverIncreases) {
            if (pCounter->value >= pTrigger->test_value &&
                pTrigger->test_value > psci->bracket_less) {
                    /*
                     * If the value is exactly equal to our threshold, we want one
                     * more event in the negative direction to ensure we pick up
                     * when the value is less than this threshold.
                     */
                    psci->bracket_less = pTrigger->test_value;
                    pnewltval = &psci->bracket_less;
            }
            else if (pCounter->value < pTrigger->test_value &&
                     pTrigger->test_value < psci->bracket_greater) {
                    psci->bracket_greater = pTrigger->test_value;
                    pnewgtval = &psci->bracket_greater;
            }
        }
        else if (pTrigger->test_type == XSyncPositiveTransition &&
                 ct != XSyncCounterNeverDecreases) {
            if (pCounter->value <= pTrigger->test_value &&
                pTrigger->test_value < psci->bracket_greater) {
                    /*
                     * If the value is exactly equal to our threshold, we
                     * want one more event in the positive direction to
                     * ensure we pick up when the value *exceeds* this
                     * threshold.
                     */
                    psci->bracket_greater = pTrigger->test_value;
                    pnewgtval = &psci->bracket_greater;
            }
            else if (pCounter->value > pTrigger->test_value &&
                     pTrigger->test_value > psci->bracket_less) {
                    psci->bracket_less = pTrigger->test_value;
                    pnewltval = &psci->bracket_less;
            }
        }
    }                           /* end for each trigger */

    (*psci->BracketValues) ((void *) pCounter, pnewltval, pnewgtval);

}

/*
 * *****  Resource delete functions
 */

/* ARGSUSED */
static int
FreeAlarm(void *addr, XID id)
{
    SyncAlarm *pAlarm = (SyncAlarm *) addr;

    pAlarm->state = XSyncAlarmDestroyed;

    SyncSendAlarmNotifyEvents(pAlarm);

    /* delete event selections */

    while (pAlarm->pEventClients)
        FreeResource(pAlarm->pEventClients->delete_id, RT_NONE);

    SyncDeleteTriggerFromSyncObject(&pAlarm->trigger);

    free(pAlarm);
    return Success;
}

/*
 * ** Cleanup after the destruction of a Counter
 */
/* ARGSUSED */
static int
FreeCounter(void *env, XID id)
{
    SyncCounter *pCounter = (SyncCounter *) env;

    pCounter->sync.beingDestroyed = TRUE;

    if (pCounter->sync.initialized) {
        SyncTriggerList *ptl, *pnext;

        /* tell all the counter's triggers that counter has been destroyed */
        for (ptl = pCounter->sync.pTriglist; ptl; ptl = pnext) {
            (*ptl->pTrigger->CounterDestroyed) (ptl->pTrigger);
            pnext = ptl->next;
            free(ptl); /* destroy the trigger list as we go */
        }
        if (IsSystemCounter(pCounter)) {
            xorg_list_del(&pCounter->pSysCounterInfo->entry);
            free(pCounter->pSysCounterInfo->name);
            free(pCounter->pSysCounterInfo->private);
            free(pCounter->pSysCounterInfo);
        }
    }

    free(pCounter);
    return Success;
}

/*
 * ** Cleanup after Await
 */
/* ARGSUSED */
static int
FreeAwait(void *addr, XID id)
{
    SyncAwaitUnion *pAwaitUnion = (SyncAwaitUnion *) addr;
    SyncAwait *pAwait;
    int numwaits;

    pAwait = &(pAwaitUnion + 1)->await; /* first await on list */

    /* remove triggers from counters */

    for (numwaits = pAwaitUnion->header.num_waitconditions; numwaits;
         numwaits--, pAwait++) {
        /* If the counter is being destroyed, FreeCounter will delete
         * the trigger list itself, so don't do it here.
         */
        SyncObject *pSync = pAwait->trigger.pSync;

        if (pSync && !pSync->beingDestroyed)
            SyncDeleteTriggerFromSyncObject(&pAwait->trigger);
    }
    free(pAwaitUnion);
    return Success;
}

/* loosely based on dix/events.c/OtherClientGone */
static int
FreeAlarmClient(void *value, XID id)
{
    SyncAlarm *pAlarm = (SyncAlarm *) value;
    SyncAlarmClientList *pCur, *pPrev;

    for (pPrev = NULL, pCur = pAlarm->pEventClients;
         pCur; pPrev = pCur, pCur = pCur->next) {
        if (pCur->delete_id == id) {
            if (pPrev)
                pPrev->next = pCur->next;
            else
                pAlarm->pEventClients = pCur->next;
            free(pCur);
            return Success;
        }
    }
    FatalError("alarm client not on event list");
 /*NOTREACHED*/}

/*
 * *****  Proc functions
 */

/*
 * ** Initialize the extension
 */
static int
ProcSyncInitialize(ClientPtr client)
{
    xSyncInitializeReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_SYNC_MAJOR_VERSION,
        .minorVersion = SERVER_SYNC_MINOR_VERSION,
    };

    REQUEST_SIZE_MATCH(xSyncInitializeReq);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
    }
    WriteToClient(client, sizeof(rep), &rep);
    return Success;
}

/*
 * ** Get list of system counters available through the extension
 */
static int
ProcSyncListSystemCounters(ClientPtr client)
{
    xSyncListSystemCountersReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .nCounters = 0,
    };
    SysCounterInfo *psci;
    int len = 0;
    xSyncSystemCounter *list = NULL, *walklist = NULL;

    REQUEST_SIZE_MATCH(xSyncListSystemCountersReq);

    xorg_list_for_each_entry(psci, &SysCounterList, entry) {
        /* pad to 4 byte boundary */
        len += pad_to_int32(sz_xSyncSystemCounter + strlen(psci->name));
        ++rep.nCounters;
    }

    if (len) {
        walklist = list = malloc(len);
        if (!list)
            return BadAlloc;
    }

    rep.length = bytes_to_int32(len);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.nCounters);
    }

    xorg_list_for_each_entry(psci, &SysCounterList, entry) {
        int namelen;
        char *pname_in_reply;

        walklist->counter = psci->pCounter->sync.id;
        walklist->resolution_hi = psci->resolution >> 32;
        walklist->resolution_lo = psci->resolution;
        namelen = strlen(psci->name);
        walklist->name_length = namelen;

        if (client->swapped) {
            swapl(&walklist->counter);
            swapl(&walklist->resolution_hi);
            swapl(&walklist->resolution_lo);
            swaps(&walklist->name_length);
        }

        pname_in_reply = ((char *) walklist) + sz_xSyncSystemCounter;
        strncpy(pname_in_reply, psci->name, namelen);
        walklist = (xSyncSystemCounter *) (((char *) walklist) +
                                           pad_to_int32(sz_xSyncSystemCounter +
                                                        namelen));
    }

    WriteToClient(client, sizeof(rep), &rep);
    if (len) {
        WriteToClient(client, len, list);
        free(list);
    }

    return Success;
}

/*
 * ** Set client Priority
 */
static int
ProcSyncSetPriority(ClientPtr client)
{
    REQUEST(xSyncSetPriorityReq);
    ClientPtr priorityclient;
    int rc;

    REQUEST_SIZE_MATCH(xSyncSetPriorityReq);

    if (stuff->id == None)
        priorityclient = client;
    else {
        rc = dixLookupClient(&priorityclient, stuff->id, client,
                             DixSetAttrAccess);
        if (rc != Success)
            return rc;
    }

    if (priorityclient->priority != stuff->priority) {
        priorityclient->priority = stuff->priority;

        /*  The following will force the server back into WaitForSomething
         *  so that the change in this client's priority is immediately
         *  reflected.
         */
        isItTimeToYield = TRUE;
        dispatchException |= DE_PRIORITYCHANGE;
    }
    return Success;
}

/*
 * ** Get client Priority
 */
static int
ProcSyncGetPriority(ClientPtr client)
{
    REQUEST(xSyncGetPriorityReq);
    xSyncGetPriorityReply rep;
    ClientPtr priorityclient;
    int rc;

    REQUEST_SIZE_MATCH(xSyncGetPriorityReq);

    if (stuff->id == None)
        priorityclient = client;
    else {
        rc = dixLookupClient(&priorityclient, stuff->id, client,
                             DixGetAttrAccess);
        if (rc != Success)
            return rc;
    }

    rep = (xSyncGetPriorityReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .priority = priorityclient->priority
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.priority);
    }

    WriteToClient(client, sizeof(xSyncGetPriorityReply), &rep);

    return Success;
}

/*
 * ** Create a new counter
 */
static int
ProcSyncCreateCounter(ClientPtr client)
{
    REQUEST(xSyncCreateCounterReq);
    int64_t initial;

    REQUEST_SIZE_MATCH(xSyncCreateCounterReq);

    LEGAL_NEW_RESOURCE(stuff->cid, client);

    initial = ((int64_t)stuff->initial_value_hi << 32) | stuff->initial_value_lo;

    if (!SyncCreateCounter(client, stuff->cid, initial))
        return BadAlloc;

    return Success;
}

/*
 * ** Set Counter value
 */
static int
ProcSyncSetCounter(ClientPtr client)
{
    REQUEST(xSyncSetCounterReq);
    SyncCounter *pCounter;
    int64_t newvalue;
    int rc;

    REQUEST_SIZE_MATCH(xSyncSetCounterReq);

    rc = dixLookupResourceByType((void **) &pCounter, stuff->cid, RTCounter,
                                 client, DixWriteAccess);
    if (rc != Success)
        return rc;

    if (IsSystemCounter(pCounter)) {
        client->errorValue = stuff->cid;
        return BadAccess;
    }

    newvalue = ((int64_t)stuff->value_hi << 32) | stuff->value_lo;
    SyncChangeCounter(pCounter, newvalue);
    return Success;
}

/*
 * ** Change Counter value
 */
static int
ProcSyncChangeCounter(ClientPtr client)
{
    REQUEST(xSyncChangeCounterReq);
    SyncCounter *pCounter;
    int64_t newvalue;
    Bool overflow;
    int rc;

    REQUEST_SIZE_MATCH(xSyncChangeCounterReq);

    rc = dixLookupResourceByType((void **) &pCounter, stuff->cid, RTCounter,
                                 client, DixWriteAccess);
    if (rc != Success)
        return rc;

    if (IsSystemCounter(pCounter)) {
        client->errorValue = stuff->cid;
        return BadAccess;
    }

    newvalue = (int64_t)stuff->value_hi << 32 | stuff->value_lo;
    overflow = checked_int64_add(&newvalue, newvalue, pCounter->value);
    if (overflow) {
        /* XXX 64 bit value can't fit in 32 bits; do the best we can */
        client->errorValue = stuff->value_hi;
        return BadValue;
    }
    SyncChangeCounter(pCounter, newvalue);
    return Success;
}

/*
 * ** Destroy a counter
 */
static int
ProcSyncDestroyCounter(ClientPtr client)
{
    REQUEST(xSyncDestroyCounterReq);
    SyncCounter *pCounter;
    int rc;

    REQUEST_SIZE_MATCH(xSyncDestroyCounterReq);

    rc = dixLookupResourceByType((void **) &pCounter, stuff->counter,
                                 RTCounter, client, DixDestroyAccess);
    if (rc != Success)
        return rc;

    if (IsSystemCounter(pCounter)) {
        client->errorValue = stuff->counter;
        return BadAccess;
    }
    FreeResource(pCounter->sync.id, RT_NONE);
    return Success;
}

static SyncAwaitUnion *
SyncAwaitPrologue(ClientPtr client, int items)
{
    SyncAwaitUnion *pAwaitUnion;

    /*  all the memory for the entire await list is allocated
     *  here in one chunk
     */
    pAwaitUnion = xallocarray(items + 1, sizeof(SyncAwaitUnion));
    if (!pAwaitUnion)
        return NULL;

    /* first item is the header, remainder are real wait conditions */

    pAwaitUnion->header.delete_id = FakeClientID(client->index);
    pAwaitUnion->header.client = client;
    pAwaitUnion->header.num_waitconditions = 0;

    if (!AddResource(pAwaitUnion->header.delete_id, RTAwait, pAwaitUnion))
        return NULL;

    return pAwaitUnion;
}

static void
SyncAwaitEpilogue(ClientPtr client, int items, SyncAwaitUnion * pAwaitUnion)
{
    SyncAwait *pAwait;
    int i;

    IgnoreClient(client);

    /* see if any of the triggers are already true */

    pAwait = &(pAwaitUnion + 1)->await; /* skip over header */
    for (i = 0; i < items; i++, pAwait++) {
        int64_t value;

        /*  don't have to worry about NULL counters because the request
         *  errors before we get here out if they occur
         */
        switch (pAwait->trigger.pSync->type) {
        case SYNC_COUNTER:
            value = ((SyncCounter *) pAwait->trigger.pSync)->value;
            break;
        default:
            value = 0;
        }

        if ((*pAwait->trigger.CheckTrigger) (&pAwait->trigger, value)) {
            (*pAwait->trigger.TriggerFired) (&pAwait->trigger);
            break;              /* once is enough */
        }
    }
}

/*
 * ** Await
 */
static int
ProcSyncAwait(ClientPtr client)
{
    REQUEST(xSyncAwaitReq);
    int len, items;
    int i;
    xSyncWaitCondition *pProtocolWaitConds;
    SyncAwaitUnion *pAwaitUnion;
    SyncAwait *pAwait;
    int status;

    REQUEST_AT_LEAST_SIZE(xSyncAwaitReq);

    len = client->req_len << 2;
    len -= sz_xSyncAwaitReq;
    items = len / sz_xSyncWaitCondition;

    if (items * sz_xSyncWaitCondition != len) {
        return BadLength;
    }
    if (items == 0) {
        client->errorValue = items;     /* XXX protocol change */
        return BadValue;
    }

    if (!(pAwaitUnion = SyncAwaitPrologue(client, items)))
        return BadAlloc;

    /* don't need to do any more memory allocation for this request! */

    pProtocolWaitConds = (xSyncWaitCondition *) &stuff[1];

    pAwait = &(pAwaitUnion + 1)->await; /* skip over header */
    for (i = 0; i < items; i++, pProtocolWaitConds++, pAwait++) {
        if (pProtocolWaitConds->counter == None) {      /* XXX protocol change */
            /*  this should take care of removing any triggers created by
             *  this request that have already been registered on sync objects
             */
            FreeResource(pAwaitUnion->header.delete_id, RT_NONE);
            client->errorValue = pProtocolWaitConds->counter;
            return SyncErrorBase + XSyncBadCounter;
        }

        /* sanity checks are in SyncInitTrigger */
        pAwait->trigger.pSync = NULL;
        pAwait->trigger.value_type = pProtocolWaitConds->value_type;
        pAwait->trigger.wait_value =
            ((int64_t)pProtocolWaitConds->wait_value_hi << 32) |
            pProtocolWaitConds->wait_value_lo;
        pAwait->trigger.test_type = pProtocolWaitConds->test_type;

        status = SyncInitTrigger(client, &pAwait->trigger,
                                 pProtocolWaitConds->counter, RTCounter,
                                 XSyncCAAllTrigger);
        if (status != Success) {
            /*  this should take care of removing any triggers created by
             *  this request that have already been registered on sync objects
             */
            FreeResource(pAwaitUnion->header.delete_id, RT_NONE);
            return status;
        }
        /* this is not a mistake -- same function works for both cases */
        pAwait->trigger.TriggerFired = SyncAwaitTriggerFired;
        pAwait->trigger.CounterDestroyed = SyncAwaitTriggerFired;
        pAwait->event_threshold =
            ((int64_t) pProtocolWaitConds->event_threshold_hi << 32) |
            pProtocolWaitConds->event_threshold_lo;

        pAwait->pHeader = &pAwaitUnion->header;
        pAwaitUnion->header.num_waitconditions++;
    }

    SyncAwaitEpilogue(client, items, pAwaitUnion);

    return Success;
}

/*
 * ** Query a counter
 */
static int
ProcSyncQueryCounter(ClientPtr client)
{
    REQUEST(xSyncQueryCounterReq);
    xSyncQueryCounterReply rep;
    SyncCounter *pCounter;
    int rc;

    REQUEST_SIZE_MATCH(xSyncQueryCounterReq);

    rc = dixLookupResourceByType((void **) &pCounter, stuff->counter,
                                 RTCounter, client, DixReadAccess);
    if (rc != Success)
        return rc;

    /* if system counter, ask it what the current value is */
    if (IsSystemCounter(pCounter)) {
        (*pCounter->pSysCounterInfo->QueryValue) ((void *) pCounter,
                                                  &pCounter->value);
    }

    rep = (xSyncQueryCounterReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .value_hi = pCounter->value >> 32,
        .value_lo = pCounter->value
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.value_hi);
        swapl(&rep.value_lo);
    }
    WriteToClient(client, sizeof(xSyncQueryCounterReply), &rep);
    return Success;
}

/*
 * ** Create Alarm
 */
static int
ProcSyncCreateAlarm(ClientPtr client)
{
    REQUEST(xSyncCreateAlarmReq);
    SyncAlarm *pAlarm;
    int status;
    unsigned long len, vmask;
    SyncTrigger *pTrigger;

    REQUEST_AT_LEAST_SIZE(xSyncCreateAlarmReq);

    LEGAL_NEW_RESOURCE(stuff->id, client);

    vmask = stuff->valueMask;
    len = client->req_len - bytes_to_int32(sizeof(xSyncCreateAlarmReq));
    /* the "extra" call to Ones accounts for the presence of 64 bit values */
    if (len != (Ones(vmask) + Ones(vmask & (XSyncCAValue | XSyncCADelta))))
        return BadLength;

    if (!(pAlarm = malloc(sizeof(SyncAlarm)))) {
        return BadAlloc;
    }

    /* set up defaults */

    pTrigger = &pAlarm->trigger;
    pTrigger->pSync = NULL;
    pTrigger->value_type = XSyncAbsolute;
    pTrigger->wait_value = 0;
    pTrigger->test_type = XSyncPositiveComparison;
    pTrigger->TriggerFired = SyncAlarmTriggerFired;
    pTrigger->CounterDestroyed = SyncAlarmCounterDestroyed;
    status = SyncInitTrigger(client, pTrigger, None, RTCounter,
                             XSyncCAAllTrigger);
    if (status != Success) {
        free(pAlarm);
        return status;
    }

    pAlarm->client = client;
    pAlarm->alarm_id = stuff->id;
    pAlarm->delta = 1;
    pAlarm->events = TRUE;
    pAlarm->state = XSyncAlarmInactive;
    pAlarm->pEventClients = NULL;
    status = SyncChangeAlarmAttributes(client, pAlarm, vmask,
                                       (CARD32 *) &stuff[1]);
    if (status != Success) {
        free(pAlarm);
        return status;
    }

    if (!AddResource(stuff->id, RTAlarm, pAlarm))
        return BadAlloc;

    /*  see if alarm already triggered.  NULL counter will not trigger
     *  in CreateAlarm and sets alarm state to Inactive.
     */

    if (!pTrigger->pSync) {
        pAlarm->state = XSyncAlarmInactive;     /* XXX protocol change */
    }
    else {
        SyncCounter *pCounter;

        if (!SyncCheckWarnIsCounter(pTrigger->pSync,
                                    WARN_INVALID_COUNTER_ALARM)) {
            FreeResource(stuff->id, RT_NONE);
            return BadAlloc;
        }

        pCounter = (SyncCounter *) pTrigger->pSync;

        if ((*pTrigger->CheckTrigger) (pTrigger, pCounter->value))
            (*pTrigger->TriggerFired) (pTrigger);
    }

    return Success;
}

/*
 * ** Change Alarm
 */
static int
ProcSyncChangeAlarm(ClientPtr client)
{
    REQUEST(xSyncChangeAlarmReq);
    SyncAlarm *pAlarm;
    SyncCounter *pCounter = NULL;
    long vmask;
    int len, status;

    REQUEST_AT_LEAST_SIZE(xSyncChangeAlarmReq);

    status = dixLookupResourceByType((void **) &pAlarm, stuff->alarm, RTAlarm,
                                     client, DixWriteAccess);
    if (status != Success)
        return status;

    vmask = stuff->valueMask;
    len = client->req_len - bytes_to_int32(sizeof(xSyncChangeAlarmReq));
    /* the "extra" call to Ones accounts for the presence of 64 bit values */
    if (len != (Ones(vmask) + Ones(vmask & (XSyncCAValue | XSyncCADelta))))
        return BadLength;

    if ((status = SyncChangeAlarmAttributes(client, pAlarm, vmask,
                                            (CARD32 *) &stuff[1])) != Success)
        return status;

    if (SyncCheckWarnIsCounter(pAlarm->trigger.pSync,
                               WARN_INVALID_COUNTER_ALARM))
        pCounter = (SyncCounter *) pAlarm->trigger.pSync;

    /*  see if alarm already triggered.  NULL counter WILL trigger
     *  in ChangeAlarm.
     */

    if (!pCounter ||
        (*pAlarm->trigger.CheckTrigger) (&pAlarm->trigger, pCounter->value)) {
        (*pAlarm->trigger.TriggerFired) (&pAlarm->trigger);
    }
    return Success;
}

static int
ProcSyncQueryAlarm(ClientPtr client)
{
    REQUEST(xSyncQueryAlarmReq);
    SyncAlarm *pAlarm;
    xSyncQueryAlarmReply rep;
    SyncTrigger *pTrigger;
    int rc;

    REQUEST_SIZE_MATCH(xSyncQueryAlarmReq);

    rc = dixLookupResourceByType((void **) &pAlarm, stuff->alarm, RTAlarm,
                                 client, DixReadAccess);
    if (rc != Success)
        return rc;

    pTrigger = &pAlarm->trigger;
    rep = (xSyncQueryAlarmReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length =
          bytes_to_int32(sizeof(xSyncQueryAlarmReply) - sizeof(xGenericReply)),
        .counter = (pTrigger->pSync) ? pTrigger->pSync->id : None,

#if 0  /* XXX unclear what to do, depends on whether relative value-types
        * are "consumed" immediately and are considered absolute from then
        * on.
        */
        .value_type = pTrigger->value_type,
        .wait_value_hi = pTrigger->wait_value >> 32,
        .wait_value_lo = pTrigger->wait_value,
#else
        .value_type = XSyncAbsolute,
        .wait_value_hi = pTrigger->test_value >> 32,
        .wait_value_lo = pTrigger->test_value,
#endif

        .test_type = pTrigger->test_type,
        .delta_hi = pAlarm->delta >> 32,
        .delta_lo = pAlarm->delta,
        .events = pAlarm->events,
        .state = pAlarm->state
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.counter);
        swapl(&rep.wait_value_hi);
        swapl(&rep.wait_value_lo);
        swapl(&rep.test_type);
        swapl(&rep.delta_hi);
        swapl(&rep.delta_lo);
    }

    WriteToClient(client, sizeof(xSyncQueryAlarmReply), &rep);
    return Success;
}

static int
ProcSyncDestroyAlarm(ClientPtr client)
{
    SyncAlarm *pAlarm;
    int rc;

    REQUEST(xSyncDestroyAlarmReq);

    REQUEST_SIZE_MATCH(xSyncDestroyAlarmReq);

    rc = dixLookupResourceByType((void **) &pAlarm, stuff->alarm, RTAlarm,
                                 client, DixDestroyAccess);
    if (rc != Success)
        return rc;

    FreeResource(stuff->alarm, RT_NONE);
    return Success;
}

static int
ProcSyncCreateFence(ClientPtr client)
{
    REQUEST(xSyncCreateFenceReq);
    DrawablePtr pDraw;
    SyncFence *pFence;
    int rc;

    REQUEST_SIZE_MATCH(xSyncCreateFenceReq);

    rc = dixLookupDrawable(&pDraw, stuff->d, client, M_ANY, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    LEGAL_NEW_RESOURCE(stuff->fid, client);

    if (!(pFence = (SyncFence *) SyncCreate(client, stuff->fid, SYNC_FENCE)))
        return BadAlloc;

    miSyncInitFence(pDraw->pScreen, pFence, stuff->initially_triggered);

    return Success;
}

static int
FreeFence(void *obj, XID id)
{
    SyncFence *pFence = (SyncFence *) obj;

    miSyncDestroyFence(pFence);

    return Success;
}

int
SyncVerifyFence(SyncFence ** ppSyncFence, XID fid, ClientPtr client, Mask mode)
{
    int rc = dixLookupResourceByType((void **) ppSyncFence, fid, RTFence,
                                     client, mode);

    if (rc != Success)
        client->errorValue = fid;

    return rc;
}

static int
ProcSyncTriggerFence(ClientPtr client)
{
    REQUEST(xSyncTriggerFenceReq);
    SyncFence *pFence;
    int rc;

    REQUEST_SIZE_MATCH(xSyncTriggerFenceReq);

    rc = dixLookupResourceByType((void **) &pFence, stuff->fid, RTFence,
                                 client, DixWriteAccess);
    if (rc != Success)
        return rc;

    miSyncTriggerFence(pFence);

    return Success;
}

static int
ProcSyncResetFence(ClientPtr client)
{
    REQUEST(xSyncResetFenceReq);
    SyncFence *pFence;
    int rc;

    REQUEST_SIZE_MATCH(xSyncResetFenceReq);

    rc = dixLookupResourceByType((void **) &pFence, stuff->fid, RTFence,
                                 client, DixWriteAccess);
    if (rc != Success)
        return rc;

    if (pFence->funcs.CheckTriggered(pFence) != TRUE)
        return BadMatch;

    pFence->funcs.Reset(pFence);

    return Success;
}

static int
ProcSyncDestroyFence(ClientPtr client)
{
    REQUEST(xSyncDestroyFenceReq);
    SyncFence *pFence;
    int rc;

    REQUEST_SIZE_MATCH(xSyncDestroyFenceReq);

    rc = dixLookupResourceByType((void **) &pFence, stuff->fid, RTFence,
                                 client, DixDestroyAccess);
    if (rc != Success)
        return rc;

    FreeResource(stuff->fid, RT_NONE);
    return Success;
}

static int
ProcSyncQueryFence(ClientPtr client)
{
    REQUEST(xSyncQueryFenceReq);
    xSyncQueryFenceReply rep;
    SyncFence *pFence;
    int rc;

    REQUEST_SIZE_MATCH(xSyncQueryFenceReq);

    rc = dixLookupResourceByType((void **) &pFence, stuff->fid,
                                 RTFence, client, DixReadAccess);
    if (rc != Success)
        return rc;

    rep = (xSyncQueryFenceReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,

        .triggered = pFence->funcs.CheckTriggered(pFence)
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }

    WriteToClient(client, sizeof(xSyncQueryFenceReply), &rep);
    return Success;
}

static int
ProcSyncAwaitFence(ClientPtr client)
{
    REQUEST(xSyncAwaitFenceReq);
    SyncAwaitUnion *pAwaitUnion;
    SyncAwait *pAwait;

    /* Use CARD32 rather than XSyncFence because XIDs are hard-coded to
     * CARD32 in protocol definitions */
    CARD32 *pProtocolFences;
    int status;
    int len;
    int items;
    int i;

    REQUEST_AT_LEAST_SIZE(xSyncAwaitFenceReq);

    len = client->req_len << 2;
    len -= sz_xSyncAwaitFenceReq;
    items = len / sizeof(CARD32);

    if (items * sizeof(CARD32) != len) {
        return BadLength;
    }
    if (items == 0) {
        client->errorValue = items;
        return BadValue;
    }

    if (!(pAwaitUnion = SyncAwaitPrologue(client, items)))
        return BadAlloc;

    /* don't need to do any more memory allocation for this request! */

    pProtocolFences = (CARD32 *) &stuff[1];

    pAwait = &(pAwaitUnion + 1)->await; /* skip over header */
    for (i = 0; i < items; i++, pProtocolFences++, pAwait++) {
        if (*pProtocolFences == None) {
            /*  this should take care of removing any triggers created by
             *  this request that have already been registered on sync objects
             */
            FreeResource(pAwaitUnion->header.delete_id, RT_NONE);
            client->errorValue = *pProtocolFences;
            return SyncErrorBase + XSyncBadFence;
        }

        pAwait->trigger.pSync = NULL;
        /* Provide acceptable values for these unused fields to
         * satisfy SyncInitTrigger's validation logic
         */
        pAwait->trigger.value_type = XSyncAbsolute;
        pAwait->trigger.wait_value = 0;
        pAwait->trigger.test_type = 0;

        status = SyncInitTrigger(client, &pAwait->trigger,
                                 *pProtocolFences, RTFence, XSyncCAAllTrigger);
        if (status != Success) {
            /*  this should take care of removing any triggers created by
             *  this request that have already been registered on sync objects
             */
            FreeResource(pAwaitUnion->header.delete_id, RT_NONE);
            return status;
        }
        /* this is not a mistake -- same function works for both cases */
        pAwait->trigger.TriggerFired = SyncAwaitTriggerFired;
        pAwait->trigger.CounterDestroyed = SyncAwaitTriggerFired;
        /* event_threshold is unused for fence syncs */
        pAwait->event_threshold = 0;
        pAwait->pHeader = &pAwaitUnion->header;
        pAwaitUnion->header.num_waitconditions++;
    }

    SyncAwaitEpilogue(client, items, pAwaitUnion);

    return Success;
}

/*
 * ** Given an extension request, call the appropriate request procedure
 */
static int
ProcSyncDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_SyncInitialize:
        return ProcSyncInitialize(client);
    case X_SyncListSystemCounters:
        return ProcSyncListSystemCounters(client);
    case X_SyncCreateCounter:
        return ProcSyncCreateCounter(client);
    case X_SyncSetCounter:
        return ProcSyncSetCounter(client);
    case X_SyncChangeCounter:
        return ProcSyncChangeCounter(client);
    case X_SyncQueryCounter:
        return ProcSyncQueryCounter(client);
    case X_SyncDestroyCounter:
        return ProcSyncDestroyCounter(client);
    case X_SyncAwait:
        return ProcSyncAwait(client);
    case X_SyncCreateAlarm:
        return ProcSyncCreateAlarm(client);
    case X_SyncChangeAlarm:
        return ProcSyncChangeAlarm(client);
    case X_SyncQueryAlarm:
        return ProcSyncQueryAlarm(client);
    case X_SyncDestroyAlarm:
        return ProcSyncDestroyAlarm(client);
    case X_SyncSetPriority:
        return ProcSyncSetPriority(client);
    case X_SyncGetPriority:
        return ProcSyncGetPriority(client);
    case X_SyncCreateFence:
        return ProcSyncCreateFence(client);
    case X_SyncTriggerFence:
        return ProcSyncTriggerFence(client);
    case X_SyncResetFence:
        return ProcSyncResetFence(client);
    case X_SyncDestroyFence:
        return ProcSyncDestroyFence(client);
    case X_SyncQueryFence:
        return ProcSyncQueryFence(client);
    case X_SyncAwaitFence:
        return ProcSyncAwaitFence(client);
    default:
        return BadRequest;
    }
}

/*
 * Boring Swapping stuff ...
 */

static int _X_COLD
SProcSyncInitialize(ClientPtr client)
{
    REQUEST(xSyncInitializeReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncInitializeReq);

    return ProcSyncInitialize(client);
}

static int _X_COLD
SProcSyncListSystemCounters(ClientPtr client)
{
    REQUEST(xSyncListSystemCountersReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncListSystemCountersReq);

    return ProcSyncListSystemCounters(client);
}

static int _X_COLD
SProcSyncCreateCounter(ClientPtr client)
{
    REQUEST(xSyncCreateCounterReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncCreateCounterReq);
    swapl(&stuff->cid);
    swapl(&stuff->initial_value_lo);
    swapl(&stuff->initial_value_hi);

    return ProcSyncCreateCounter(client);
}

static int _X_COLD
SProcSyncSetCounter(ClientPtr client)
{
    REQUEST(xSyncSetCounterReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncSetCounterReq);
    swapl(&stuff->cid);
    swapl(&stuff->value_lo);
    swapl(&stuff->value_hi);

    return ProcSyncSetCounter(client);
}

static int _X_COLD
SProcSyncChangeCounter(ClientPtr client)
{
    REQUEST(xSyncChangeCounterReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncChangeCounterReq);
    swapl(&stuff->cid);
    swapl(&stuff->value_lo);
    swapl(&stuff->value_hi);

    return ProcSyncChangeCounter(client);
}

static int _X_COLD
SProcSyncQueryCounter(ClientPtr client)
{
    REQUEST(xSyncQueryCounterReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncQueryCounterReq);
    swapl(&stuff->counter);

    return ProcSyncQueryCounter(client);
}

static int _X_COLD
SProcSyncDestroyCounter(ClientPtr client)
{
    REQUEST(xSyncDestroyCounterReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncDestroyCounterReq);
    swapl(&stuff->counter);

    return ProcSyncDestroyCounter(client);
}

static int _X_COLD
SProcSyncAwait(ClientPtr client)
{
    REQUEST(xSyncAwaitReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xSyncAwaitReq);
    SwapRestL(stuff);

    return ProcSyncAwait(client);
}

static int _X_COLD
SProcSyncCreateAlarm(ClientPtr client)
{
    REQUEST(xSyncCreateAlarmReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xSyncCreateAlarmReq);
    swapl(&stuff->id);
    swapl(&stuff->valueMask);
    SwapRestL(stuff);

    return ProcSyncCreateAlarm(client);
}

static int _X_COLD
SProcSyncChangeAlarm(ClientPtr client)
{
    REQUEST(xSyncChangeAlarmReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xSyncChangeAlarmReq);
    swapl(&stuff->alarm);
    swapl(&stuff->valueMask);
    SwapRestL(stuff);
    return ProcSyncChangeAlarm(client);
}

static int _X_COLD
SProcSyncQueryAlarm(ClientPtr client)
{
    REQUEST(xSyncQueryAlarmReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncQueryAlarmReq);
    swapl(&stuff->alarm);

    return ProcSyncQueryAlarm(client);
}

static int _X_COLD
SProcSyncDestroyAlarm(ClientPtr client)
{
    REQUEST(xSyncDestroyAlarmReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncDestroyAlarmReq);
    swapl(&stuff->alarm);

    return ProcSyncDestroyAlarm(client);
}

static int _X_COLD
SProcSyncSetPriority(ClientPtr client)
{
    REQUEST(xSyncSetPriorityReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncSetPriorityReq);
    swapl(&stuff->id);
    swapl(&stuff->priority);

    return ProcSyncSetPriority(client);
}

static int _X_COLD
SProcSyncGetPriority(ClientPtr client)
{
    REQUEST(xSyncGetPriorityReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncGetPriorityReq);
    swapl(&stuff->id);

    return ProcSyncGetPriority(client);
}

static int _X_COLD
SProcSyncCreateFence(ClientPtr client)
{
    REQUEST(xSyncCreateFenceReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncCreateFenceReq);
    swapl(&stuff->fid);

    return ProcSyncCreateFence(client);
}

static int _X_COLD
SProcSyncTriggerFence(ClientPtr client)
{
    REQUEST(xSyncTriggerFenceReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncTriggerFenceReq);
    swapl(&stuff->fid);

    return ProcSyncTriggerFence(client);
}

static int _X_COLD
SProcSyncResetFence(ClientPtr client)
{
    REQUEST(xSyncResetFenceReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncResetFenceReq);
    swapl(&stuff->fid);

    return ProcSyncResetFence(client);
}

static int _X_COLD
SProcSyncDestroyFence(ClientPtr client)
{
    REQUEST(xSyncDestroyFenceReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncDestroyFenceReq);
    swapl(&stuff->fid);

    return ProcSyncDestroyFence(client);
}

static int _X_COLD
SProcSyncQueryFence(ClientPtr client)
{
    REQUEST(xSyncQueryFenceReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xSyncQueryFenceReq);
    swapl(&stuff->fid);

    return ProcSyncQueryFence(client);
}

static int _X_COLD
SProcSyncAwaitFence(ClientPtr client)
{
    REQUEST(xSyncAwaitFenceReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xSyncAwaitFenceReq);
    SwapRestL(stuff);

    return ProcSyncAwaitFence(client);
}

static int _X_COLD
SProcSyncDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_SyncInitialize:
        return SProcSyncInitialize(client);
    case X_SyncListSystemCounters:
        return SProcSyncListSystemCounters(client);
    case X_SyncCreateCounter:
        return SProcSyncCreateCounter(client);
    case X_SyncSetCounter:
        return SProcSyncSetCounter(client);
    case X_SyncChangeCounter:
        return SProcSyncChangeCounter(client);
    case X_SyncQueryCounter:
        return SProcSyncQueryCounter(client);
    case X_SyncDestroyCounter:
        return SProcSyncDestroyCounter(client);
    case X_SyncAwait:
        return SProcSyncAwait(client);
    case X_SyncCreateAlarm:
        return SProcSyncCreateAlarm(client);
    case X_SyncChangeAlarm:
        return SProcSyncChangeAlarm(client);
    case X_SyncQueryAlarm:
        return SProcSyncQueryAlarm(client);
    case X_SyncDestroyAlarm:
        return SProcSyncDestroyAlarm(client);
    case X_SyncSetPriority:
        return SProcSyncSetPriority(client);
    case X_SyncGetPriority:
        return SProcSyncGetPriority(client);
    case X_SyncCreateFence:
        return SProcSyncCreateFence(client);
    case X_SyncTriggerFence:
        return SProcSyncTriggerFence(client);
    case X_SyncResetFence:
        return SProcSyncResetFence(client);
    case X_SyncDestroyFence:
        return SProcSyncDestroyFence(client);
    case X_SyncQueryFence:
        return SProcSyncQueryFence(client);
    case X_SyncAwaitFence:
        return SProcSyncAwaitFence(client);
    default:
        return BadRequest;
    }
}

/*
 * Event Swapping
 */

static void _X_COLD
SCounterNotifyEvent(xSyncCounterNotifyEvent * from,
                    xSyncCounterNotifyEvent * to)
{
    to->type = from->type;
    to->kind = from->kind;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->counter, to->counter);
    cpswapl(from->wait_value_lo, to->wait_value_lo);
    cpswapl(from->wait_value_hi, to->wait_value_hi);
    cpswapl(from->counter_value_lo, to->counter_value_lo);
    cpswapl(from->counter_value_hi, to->counter_value_hi);
    cpswapl(from->time, to->time);
    cpswaps(from->count, to->count);
    to->destroyed = from->destroyed;
}

static void _X_COLD
SAlarmNotifyEvent(xSyncAlarmNotifyEvent * from, xSyncAlarmNotifyEvent * to)
{
    to->type = from->type;
    to->kind = from->kind;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->alarm, to->alarm);
    cpswapl(from->counter_value_lo, to->counter_value_lo);
    cpswapl(from->counter_value_hi, to->counter_value_hi);
    cpswapl(from->alarm_value_lo, to->alarm_value_lo);
    cpswapl(from->alarm_value_hi, to->alarm_value_hi);
    cpswapl(from->time, to->time);
    to->state = from->state;
}

/*
 * ** Close everything down. ** This is fairly simple for now.
 */
/* ARGSUSED */
static void
SyncResetProc(ExtensionEntry * extEntry)
{
    RTCounter = 0;
}

/*
 * ** Initialise the extension.
 */
void
SyncExtensionInit(void)
{
    ExtensionEntry *extEntry;
    int s;

    for (s = 0; s < screenInfo.numScreens; s++)
        miSyncSetup(screenInfo.screens[s]);

    RTCounter = CreateNewResourceType(FreeCounter, "SyncCounter");
    xorg_list_init(&SysCounterList);
    RTAlarm = CreateNewResourceType(FreeAlarm, "SyncAlarm");
    RTAwait = CreateNewResourceType(FreeAwait, "SyncAwait");
    RTFence = CreateNewResourceType(FreeFence, "SyncFence");
    if (RTAwait)
        RTAwait |= RC_NEVERRETAIN;
    RTAlarmClient = CreateNewResourceType(FreeAlarmClient, "SyncAlarmClient");
    if (RTAlarmClient)
        RTAlarmClient |= RC_NEVERRETAIN;

    if (RTCounter == 0 || RTAwait == 0 || RTAlarm == 0 ||
        RTAlarmClient == 0 ||
        (extEntry = AddExtension(SYNC_NAME,
                                 XSyncNumberEvents, XSyncNumberErrors,
                                 ProcSyncDispatch, SProcSyncDispatch,
                                 SyncResetProc, StandardMinorOpcode)) == NULL) {
        ErrorF("Sync Extension %d.%d failed to Initialise\n",
               SYNC_MAJOR_VERSION, SYNC_MINOR_VERSION);
        return;
    }

    SyncEventBase = extEntry->eventBase;
    SyncErrorBase = extEntry->errorBase;
    EventSwapVector[SyncEventBase + XSyncCounterNotify] =
        (EventSwapPtr) SCounterNotifyEvent;
    EventSwapVector[SyncEventBase + XSyncAlarmNotify] =
        (EventSwapPtr) SAlarmNotifyEvent;

    SetResourceTypeErrorValue(RTCounter, SyncErrorBase + XSyncBadCounter);
    SetResourceTypeErrorValue(RTAlarm, SyncErrorBase + XSyncBadAlarm);
    SetResourceTypeErrorValue(RTFence, SyncErrorBase + XSyncBadFence);

    /*
     * Although SERVERTIME is implemented by the OS layer, we initialise it
     * here because doing it in OsInit() is too early. The resource database
     * is not initialised when OsInit() is called. This is just about OK
     * because there is always a servertime counter.
     */
    SyncInitServerTime();
    SyncInitIdleTime();

#ifdef DEBUG
    fprintf(stderr, "Sync Extension %d.%d\n",
            SYNC_MAJOR_VERSION, SYNC_MINOR_VERSION);
#endif
}

/*
 * ***** SERVERTIME implementation - should go in its own file in OS directory?
 */

static void *ServertimeCounter;
static int64_t Now;
static int64_t *pnext_time;

static void GetTime(void)
{
    unsigned long millis = GetTimeInMillis();
    unsigned long maxis = Now >> 32;

    if (millis < (Now & 0xffffffff))
        maxis++;

    Now = ((int64_t)maxis << 32) | millis;
}

/*
*** Server Block Handler
*** code inspired by multibuffer extension (now deprecated)
 */
/*ARGSUSED*/ static void
ServertimeBlockHandler(void *env, void *wt)
{
    unsigned long timeout;

    if (pnext_time) {
        GetTime();

        if (Now >= *pnext_time) {
            timeout = 0;
        }
        else {
            timeout = *pnext_time - Now;
        }
        AdjustWaitForDelay(wt, timeout);        /* os/utils.c */
    }
}

/*
*** Wakeup Handler
 */
/*ARGSUSED*/ static void
ServertimeWakeupHandler(void *env, int rc)
{
    if (pnext_time) {
        GetTime();

        if (Now >= *pnext_time) {
            SyncChangeCounter(ServertimeCounter, Now);
        }
    }
}

static void
ServertimeQueryValue(void *pCounter, int64_t *pValue_return)
{
    GetTime();
    *pValue_return = Now;
}

static void
ServertimeBracketValues(void *pCounter, int64_t *pbracket_less,
                        int64_t *pbracket_greater)
{
    if (!pnext_time && pbracket_greater) {
        RegisterBlockAndWakeupHandlers(ServertimeBlockHandler,
                                       ServertimeWakeupHandler, NULL);
    }
    else if (pnext_time && !pbracket_greater) {
        RemoveBlockAndWakeupHandlers(ServertimeBlockHandler,
                                     ServertimeWakeupHandler, NULL);
    }
    pnext_time = pbracket_greater;
}

static void
SyncInitServerTime(void)
{
    int64_t resolution = 4;

    Now = GetTimeInMillis();
    ServertimeCounter = SyncCreateSystemCounter("SERVERTIME", Now, resolution,
                                                XSyncCounterNeverDecreases,
                                                ServertimeQueryValue,
                                                ServertimeBracketValues);
    pnext_time = NULL;
}

/*
 * IDLETIME implementation
 */

typedef struct {
    int64_t *value_less;
    int64_t *value_greater;
    int deviceid;
} IdleCounterPriv;

static void
IdleTimeQueryValue(void *pCounter, int64_t *pValue_return)
{
    int deviceid;
    CARD32 idle;

    if (pCounter) {
        SyncCounter *counter = pCounter;
        IdleCounterPriv *priv = SysCounterGetPrivate(counter);
        deviceid = priv->deviceid;
    }
    else
        deviceid = XIAllDevices;
    idle = GetTimeInMillis() - LastEventTime(deviceid).milliseconds;
    *pValue_return = idle;
}

static void
IdleTimeBlockHandler(void *pCounter, void *wt)
{
    SyncCounter *counter = pCounter;
    IdleCounterPriv *priv = SysCounterGetPrivate(counter);
    int64_t *less = priv->value_less;
    int64_t *greater = priv->value_greater;
    int64_t idle, old_idle;
    SyncTriggerList *list = counter->sync.pTriglist;
    SyncTrigger *trig;

    if (!less && !greater)
        return;

    old_idle = counter->value;
    IdleTimeQueryValue(counter, &idle);
    counter->value = idle;      /* push, so CheckTrigger works */

    /**
     * There's an indefinite amount of time between ProcessInputEvents()
     * where the idle time is reset and the time we actually get here. idle
     * may be past the lower bracket if we dawdled with the events, so
     * check for whether we did reset and bomb out of select immediately.
     */
    if (less && idle > *less &&
        LastEventTimeWasReset(priv->deviceid)) {
        AdjustWaitForDelay(wt, 0);
    } else if (less && idle <= *less) {
        /*
         * We've been idle for less than the threshold value, and someone
         * wants to know about that, but now we need to know whether they
         * want level or edge trigger.  Check the trigger list against the
         * current idle time, and if any succeed, bomb out of select()
         * immediately so we can reschedule.
         */

        for (list = counter->sync.pTriglist; list; list = list->next) {
            trig = list->pTrigger;
            if (trig->CheckTrigger(trig, old_idle)) {
                AdjustWaitForDelay(wt, 0);
                break;
            }
        }
        /*
         * We've been called exactly on the idle time, but we have a
         * NegativeTransition trigger which requires a transition from an
         * idle time greater than this.  Schedule a wakeup for the next
         * millisecond so we won't miss a transition.
         */
        if (idle == *less)
            AdjustWaitForDelay(wt, 1);
    }
    else if (greater) {
        /*
         * There's a threshold in the positive direction.  If we've been
         * idle less than it, schedule a wakeup for sometime in the future.
         * If we've been idle more than it, and someone wants to know about
         * that level-triggered, schedule an immediate wakeup.
         */

        if (idle < *greater) {
            AdjustWaitForDelay(wt, *greater - idle);
        }
        else {
            for (list = counter->sync.pTriglist; list;
                 list = list->next) {
                trig = list->pTrigger;
                if (trig->CheckTrigger(trig, old_idle)) {
                    AdjustWaitForDelay(wt, 0);
                    break;
                }
            }
        }
    }

    counter->value = old_idle;  /* pop */
}

static void
IdleTimeCheckBrackets(SyncCounter *counter, int64_t idle,
                      int64_t *less, int64_t *greater)
{
    if ((greater && idle >= *greater) ||
        (less && idle <= *less)) {
        SyncChangeCounter(counter, idle);
    }
    else
        SyncUpdateCounter(counter, idle);
}

static void
IdleTimeWakeupHandler(void *pCounter, int rc)
{
    SyncCounter *counter = pCounter;
    IdleCounterPriv *priv = SysCounterGetPrivate(counter);
    int64_t *less = priv->value_less;
    int64_t *greater = priv->value_greater;
    int64_t idle;

    if (!less && !greater)
        return;

    IdleTimeQueryValue(pCounter, &idle);

    /*
      There is no guarantee for the WakeupHandler to be called within a specific
      timeframe. Idletime may go to 0, but by the time we get here, it may be
      non-zero and alarms for a pos. transition on 0 won't get triggered.
      https://bugs.freedesktop.org/show_bug.cgi?id=70476
      */
    if (LastEventTimeWasReset(priv->deviceid)) {
        LastEventTimeToggleResetFlag(priv->deviceid, FALSE);
        if (idle != 0) {
            IdleTimeCheckBrackets(counter, 0, less, greater);
            less = priv->value_less;
            greater = priv->value_greater;
        }
    }

    IdleTimeCheckBrackets(counter, idle, less, greater);
}

static void
IdleTimeBracketValues(void *pCounter, int64_t *pbracket_less,
                      int64_t *pbracket_greater)
{
    SyncCounter *counter = pCounter;
    IdleCounterPriv *priv = SysCounterGetPrivate(counter);
    int64_t *less = priv->value_less;
    int64_t *greater = priv->value_greater;
    Bool registered = (less || greater);

    if (registered && !pbracket_less && !pbracket_greater) {
        RemoveBlockAndWakeupHandlers(IdleTimeBlockHandler,
                                     IdleTimeWakeupHandler, pCounter);
    }
    else if (!registered && (pbracket_less || pbracket_greater)) {
        /* Reset flag must be zero so we don't force a idle timer reset on
           the first wakeup */
        LastEventTimeToggleResetAll(FALSE);
        RegisterBlockAndWakeupHandlers(IdleTimeBlockHandler,
                                       IdleTimeWakeupHandler, pCounter);
    }

    priv->value_greater = pbracket_greater;
    priv->value_less = pbracket_less;
}

static SyncCounter*
init_system_idle_counter(const char *name, int deviceid)
{
    int64_t resolution = 4;
    int64_t idle;
    SyncCounter *idle_time_counter;

    IdleTimeQueryValue(NULL, &idle);

    idle_time_counter = SyncCreateSystemCounter(name, idle, resolution,
                                                XSyncCounterUnrestricted,
                                                IdleTimeQueryValue,
                                                IdleTimeBracketValues);

    if (idle_time_counter != NULL) {
        IdleCounterPriv *priv = malloc(sizeof(IdleCounterPriv));

        priv->value_less = priv->value_greater = NULL;
        priv->deviceid = deviceid;

        idle_time_counter->pSysCounterInfo->private = priv;
    }

    return idle_time_counter;
}

static void
SyncInitIdleTime(void)
{
    init_system_idle_counter("IDLETIME", XIAllDevices);
}

SyncCounter*
SyncInitDeviceIdleTime(DeviceIntPtr dev)
{
    char timer_name[64];
    sprintf(timer_name, "DEVICEIDLETIME %d", dev->id);

    return init_system_idle_counter(timer_name, dev->id);
}

void SyncRemoveDeviceIdleTime(SyncCounter *counter)
{
    /* FreeAllResources() frees all system counters before the devices are
       shut down, check if there are any left before freeing the device's
       counter */
    if (counter && !xorg_list_is_empty(&SysCounterList))
        xorg_list_del(&counter->pSysCounterInfo->entry);
}
