/***********************************************************
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

#ifndef DIXSTRUCT_H
#define DIXSTRUCT_H

#include "client.h"
#include "dix.h"
#include "resource.h"
#include "cursor.h"
#include "gc.h"
#include "pixmap.h"
#include "privates.h"
#include <X11/Xmd.h>

/*
 * 	direct-mapped hash table, used by resource manager to store
 *      translation from client ids to server addresses.
 */

extern _X_EXPORT CallbackListPtr ClientStateCallback;

typedef struct {
    ClientPtr client;
    xConnSetupPrefix *prefix;
    xConnSetup *setup;
} NewClientInfoRec;

typedef void (*ReplySwapPtr) (ClientPtr /* pClient */ ,
                              int /* size */ ,
                              void * /* pbuf */ );

extern _X_EXPORT void
ReplyNotSwappd(ClientPtr /* pClient */ ,
               int /* size */ ,
               void * /* pbuf */ ) _X_NORETURN;

typedef enum { ClientStateInitial,
    ClientStateRunning,
    ClientStateRetained,
    ClientStateGone
} ClientState;

typedef struct _saveSet {
    struct _Window *windowPtr;
    Bool toRoot;
    Bool map;
} SaveSetElt;
#define SaveSetWindow(ss)   ((ss).windowPtr)
#define SaveSetToRoot(ss)   ((ss).toRoot)
#define SaveSetShouldMap(ss)	    ((ss).map)
#define SaveSetAssignWindow(ss,w)   ((ss).windowPtr = (w))
#define SaveSetAssignToRoot(ss,tr)  ((ss).toRoot = (tr))
#define SaveSetAssignMap(ss,m)      ((ss).map = (m))

typedef struct _Client {
    void *requestBuffer;
    void *osPrivate;             /* for OS layer, including scheduler */
    struct xorg_list ready;      /* List of clients ready to run */
    struct xorg_list output_pending; /* List of clients with output queued */
    Mask clientAsMask;
    short index;
    unsigned char majorOp, minorOp;
    unsigned int swapped:1;
    unsigned int local:1;
    unsigned int big_requests:1; /* supports large requests */
    unsigned int clientGone:1;
    unsigned int closeDownMode:2;
    unsigned int clientState:2;
    signed char smart_priority;
    short noClientException;      /* this client died or needs to be killed */
    int priority;
    ReplySwapPtr pSwapReplyFunc;
    XID errorValue;
    int sequence;
    int ignoreCount;            /* count for Attend/IgnoreClient */
    int numSaved;
    SaveSetElt *saveSet;
    int (**requestVector) (ClientPtr /* pClient */ );
    CARD32 req_len;             /* length of current request */
    unsigned int replyBytesRemaining;
    PrivateRec *devPrivates;
    unsigned short mapNotifyMask;
    unsigned short newKeyboardNotifyMask;
    unsigned char xkbClientFlags;
    KeyCode minKC, maxKC;

    int smart_start_tick;
    int smart_stop_tick;

    DeviceIntPtr clientPtr;
    ClientIdPtr clientIds;
    int req_fds;
} ClientRec;

static inline void
SetReqFds(ClientPtr client, int req_fds) {
    if (client->req_fds != 0 && req_fds != client->req_fds)
        LogMessage(X_ERROR, "Mismatching number of request fds %d != %d\n", req_fds, client->req_fds);
    client->req_fds = req_fds;
}

/*
 * Scheduling interface
 */
extern long SmartScheduleTime;
extern long SmartScheduleInterval;
extern long SmartScheduleSlice;
extern long SmartScheduleMaxSlice;
#ifdef HAVE_SETITIMER
extern Bool SmartScheduleSignalEnable;
#else
#define SmartScheduleSignalEnable FALSE
#endif
extern void SmartScheduleStartTimer(void);
extern void SmartScheduleStopTimer(void);

/* Client has requests queued or data on the network */
void mark_client_ready(ClientPtr client);

/*
 * Client has requests queued or data on the network, but awaits a
 * server grab release
 */
void mark_client_saved_ready(ClientPtr client);

/* Client has no requests queued and no data on network */
void mark_client_not_ready(ClientPtr client);

static inline Bool client_is_ready(ClientPtr client)
{
    return !xorg_list_is_empty(&client->ready);
}

Bool
clients_are_ready(void);

extern struct xorg_list output_pending_clients;

static inline void
output_pending_mark(ClientPtr client)
{
    if (!client->clientGone && xorg_list_is_empty(&client->output_pending))
        xorg_list_append(&client->output_pending, &output_pending_clients);
}

static inline void
output_pending_clear(ClientPtr client)
{
    xorg_list_del(&client->output_pending);
}

static inline Bool any_output_pending(void) {
    return !xorg_list_is_empty(&output_pending_clients);
}

#define SMART_MAX_PRIORITY  (20)
#define SMART_MIN_PRIORITY  (-20)

extern void SmartScheduleInit(void);

/* This prototype is used pervasively in Xext, dix */
#define DISPATCH_PROC(func) int func(ClientPtr /* client */)

typedef struct _WorkQueue {
    struct _WorkQueue *next;
    Bool (*function) (ClientPtr /* pClient */ ,
                      void *    /* closure */
        );
    ClientPtr client;
    void *closure;
} WorkQueueRec;

extern _X_EXPORT TimeStamp currentTime;

extern _X_EXPORT int
CompareTimeStamps(TimeStamp /*a */ ,
                  TimeStamp /*b */ );

extern _X_EXPORT TimeStamp
ClientTimeToServerTime(CARD32 /*c */ );

typedef struct _CallbackRec {
    CallbackProcPtr proc;
    void *data;
    Bool deleted;
    struct _CallbackRec *next;
} CallbackRec, *CallbackPtr;

typedef struct _CallbackList {
    int inCallback;
    Bool deleted;
    int numDeleted;
    CallbackPtr list;
} CallbackListRec;

/* proc vectors */

extern int (*InitialVector[3]) (ClientPtr /*client */ );

extern _X_EXPORT int (*ProcVector[256]) (ClientPtr /*client */ );

extern _X_EXPORT int (*SwappedProcVector[256]) (ClientPtr /*client */ );

extern ReplySwapPtr ReplySwapVector[256];

extern _X_EXPORT int
ProcBadRequest(ClientPtr /*client */ );

#endif                          /* DIXSTRUCT_H */
