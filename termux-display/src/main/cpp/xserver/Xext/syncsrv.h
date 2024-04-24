/*

Copyright 1991, 1993, 1994, 1998  The Open Group

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

*/

/***********************************************************
Copyright 1991,1993 by Digital Equipment Corporation, Maynard, Massachusetts,
and Olivetti Research Limited, Cambridge, England.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital or Olivetti
not be used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL AND OLIVETTI DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL THEY BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

#ifndef _SYNCSRV_H_
#define _SYNCSRV_H_

#include "list.h"
#include "misync.h"
#include "misyncstr.h"

/*
 * The System Counter interface
 */

typedef enum {
    XSyncCounterNeverChanges,
    XSyncCounterNeverIncreases,
    XSyncCounterNeverDecreases,
    XSyncCounterUnrestricted
} SyncCounterType;

typedef void (*SyncSystemCounterQueryValue)(void *counter,
                                            int64_t *value_return
    );
typedef void (*SyncSystemCounterBracketValues)(void *counter,
                                               int64_t *pbracket_less,
                                               int64_t *pbracket_greater
    );

typedef struct _SysCounterInfo {
    SyncCounter *pCounter;
    char *name;
    int64_t resolution;
    int64_t bracket_greater;
    int64_t bracket_less;
    SyncCounterType counterType;        /* how can this counter change */
    SyncSystemCounterQueryValue QueryValue;
    SyncSystemCounterBracketValues BracketValues;
    void *private;
    struct xorg_list entry;
} SysCounterInfo;

typedef struct _SyncAlarmClientList {
    ClientPtr client;
    XID delete_id;
    struct _SyncAlarmClientList *next;
} SyncAlarmClientList;

typedef struct _SyncAlarm {
    SyncTrigger trigger;
    ClientPtr client;
    XSyncAlarm alarm_id;
    int64_t delta;
    int events;
    int state;
    SyncAlarmClientList *pEventClients;
} SyncAlarm;

typedef struct {
    ClientPtr client;
    CARD32 delete_id;
    int num_waitconditions;
} SyncAwaitHeader;

typedef struct {
    SyncTrigger trigger;
    int64_t event_threshold;
    SyncAwaitHeader *pHeader;
} SyncAwait;

typedef union {
    SyncAwaitHeader header;
    SyncAwait await;
} SyncAwaitUnion;

extern SyncCounter* SyncCreateSystemCounter(const char *name,
                                            int64_t initial_value,
                                            int64_t resolution,
                                            SyncCounterType counterType,
                                            SyncSystemCounterQueryValue QueryValue,
                                            SyncSystemCounterBracketValues BracketValues
    );

extern void SyncChangeCounter(SyncCounter *pCounter,
                              int64_t new_value);

extern void SyncDestroySystemCounter(void *pCounter);

extern SyncCounter *SyncInitDeviceIdleTime(DeviceIntPtr dev);
extern void SyncRemoveDeviceIdleTime(SyncCounter *counter);

int
SyncCreateFenceFromFD(ClientPtr client, DrawablePtr pDraw, XID id, int fd, BOOL initially_triggered);

int
SyncFDFromFence(ClientPtr client, DrawablePtr pDraw, SyncFence *fence);

void
SyncDeleteTriggerFromSyncObject(SyncTrigger * pTrigger);

int
SyncAddTriggerToSyncObject(SyncTrigger * pTrigger);

#endif                          /* _SYNCSRV_H_ */
