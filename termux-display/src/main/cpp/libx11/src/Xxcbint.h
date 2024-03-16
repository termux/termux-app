/* Copyright (C) 2003-2006 Jamey Sharp, Josh Triplett
 * This file is licensed under the MIT license. See the file COPYING. */

#ifndef XXCBINT_H
#define XXCBINT_H

#include <assert.h>
#include <stdint.h>
#include <X11/Xlibint.h>
#ifdef WIN32
#include <X11/Xwindows.h>
#endif
#include <X11/Xlib-xcb.h>
#include "locking.h"

#define XLIB_SEQUENCE_COMPARE(a,op,b)	(((int64_t) (a) - (int64_t) (b)) op 0)

typedef struct PendingRequest PendingRequest;
struct PendingRequest {
	PendingRequest *next;
	uint64_t sequence;
	unsigned reply_waiter;
};

typedef struct _X11XCBPrivate {
	xcb_connection_t *connection;
	PendingRequest *pending_requests;
	PendingRequest *pending_requests_tail;
	xcb_generic_event_t *next_event;
	void *next_response;
	char *real_bufmax;
	char *reply_data;
	int reply_length;
	int reply_consumed;
	uint64_t last_flushed;
	enum XEventQueueOwner event_owner;
	XID next_xid;

	/* handle simultaneous threads waiting for responses */
	xcondition_t event_notify;
	int event_waiter;
	xcondition_t reply_notify;
} _X11XCBPrivate;

/* xcb_disp.c */

int _XConnectXCB(Display *dpy, _Xconst char *display, int *screenp);
void _XFreeX11XCBStructure(Display *dpy);

_X_HIDDEN
unsigned long _XNextRequest(Display *dpy);

#endif /* XXCBINT_H */
