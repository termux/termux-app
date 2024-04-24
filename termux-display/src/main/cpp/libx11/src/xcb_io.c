/* Copyright (C) 2003-2006 Jamey Sharp, Josh Triplett
 * This file is licensed under the MIT license. See the file COPYING. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Xlibint.h"
#include "locking.h"
#include "Xprivate.h"
#include "Xxcbint.h"
#include <xcb/xcbext.h>

#include <assert.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#define xcb_fail_assert(_message, _var) { \
	unsigned int _var = 1; \
	fprintf(stderr, "[xcb] Aborting, sorry about that.\n"); \
	assert(!_var); \
}

#define throw_thread_fail_assert(_message, _var) { \
	fprintf(stderr, "[xcb] " _message "\n"); \
        if (_Xglobal_lock) { \
            fprintf(stderr, "[xcb] You called XInitThreads, this is not your fault\n"); \
        } else { \
            fprintf(stderr, "[xcb] Most likely this is a multi-threaded client " \
                            "and XInitThreads has not been called\n"); \
        } \
	xcb_fail_assert(_message, _var); \
}

/* XXX: It would probably be most useful if we stored the last-processed
 *      request, so we could find the offender from the message. */
#define throw_extlib_fail_assert(_message, _var) { \
	fprintf(stderr, "[xcb] " _message "\n"); \
	fprintf(stderr, "[xcb] This is most likely caused by a broken X " \
	                "extension library\n"); \
	xcb_fail_assert(_message, _var); \
}

static void return_socket(void *closure)
{
	Display *dpy = closure;
	InternalLockDisplay(dpy, /* don't skip user locks */ 0);
	_XSend(dpy, NULL, 0);
	dpy->bufmax = dpy->buffer;
	UnlockDisplay(dpy);
}

static Bool require_socket(Display *dpy)
{
	if(dpy->bufmax == dpy->buffer)
	{
		uint64_t sent;
		int flags = 0;
		/* if we don't own the event queue, we have to ask XCB
		 * to set our errors aside for us. */
		if(dpy->xcb->event_owner != XlibOwnsEventQueue)
			flags = XCB_REQUEST_CHECKED;
		if(!xcb_take_socket(dpy->xcb->connection, return_socket, dpy,
		                    flags, &sent)) {
			_XIOError(dpy);
			return False;
		}
		dpy->xcb->last_flushed = sent;
		X_DPY_SET_REQUEST(dpy, sent);
		dpy->bufmax = dpy->xcb->real_bufmax;
	}
	return True;
}

/* Call internal connection callbacks for any fds that are currently
 * ready to read. This function will not block unless one of the
 * callbacks blocks.
 *
 * This code borrowed from _XWaitForReadable. Inverse call tree:
 * _XRead
 *  _XWaitForWritable
 *   _XFlush
 *   _XSend
 *  _XEventsQueued
 *  _XReadEvents
 *  _XRead[0-9]+
 *   _XAllocIDs
 *  _XReply
 *  _XEatData
 * _XReadPad
 */
static Bool check_internal_connections(Display *dpy)
{
	struct _XConnectionInfo *ilist;
	fd_set r_mask;
	struct timeval tv;
	int result;
	int highest_fd = -1;

	if(dpy->flags & XlibDisplayProcConni || !dpy->im_fd_info)
		return True;

	FD_ZERO(&r_mask);
	for(ilist = dpy->im_fd_info; ilist; ilist = ilist->next)
	{
		assert(ilist->fd >= 0);
		FD_SET(ilist->fd, &r_mask);
		if(ilist->fd > highest_fd)
			highest_fd = ilist->fd;
	}
	assert(highest_fd >= 0);

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	result = select(highest_fd + 1, &r_mask, NULL, NULL, &tv);

	if(result == -1)
	{
		if(errno != EINTR) {
			_XIOError(dpy);
			return False;
		}

		return True;
	}

	for(ilist = dpy->im_fd_info; result && ilist; ilist = ilist->next)
		if(FD_ISSET(ilist->fd, &r_mask))
		{
			_XProcessInternalConnection(dpy, ilist);
			--result;
		}

	return True;
}

static PendingRequest *append_pending_request(Display *dpy, uint64_t sequence)
{
	PendingRequest *node = malloc(sizeof(PendingRequest));
	assert(node);
	node->next = NULL;
	node->sequence = sequence;
	node->reply_waiter = 0;
	if(dpy->xcb->pending_requests_tail)
	{
		if (XLIB_SEQUENCE_COMPARE(dpy->xcb->pending_requests_tail->sequence,
		                          >=, node->sequence))
			throw_thread_fail_assert("Unknown sequence number "
			                         "while appending request",
			                         xcb_xlib_unknown_seq_number);
		if (dpy->xcb->pending_requests_tail->next != NULL)
			throw_thread_fail_assert("Unknown request in queue "
			                         "while appending request",
			                         xcb_xlib_unknown_req_pending);
		dpy->xcb->pending_requests_tail->next = node;
	}
	else
		dpy->xcb->pending_requests = node;
	dpy->xcb->pending_requests_tail = node;
	return node;
}

static void dequeue_pending_request(Display *dpy, PendingRequest *req)
{
	if (req != dpy->xcb->pending_requests)
		throw_thread_fail_assert("Unknown request in queue while "
		                         "dequeuing",
		                         xcb_xlib_unknown_req_in_deq);

	dpy->xcb->pending_requests = req->next;
	if(!dpy->xcb->pending_requests)
	{
		if (req != dpy->xcb->pending_requests_tail)
			throw_thread_fail_assert("Unknown request in queue "
			                         "while dequeuing",
			                         xcb_xlib_unknown_req_in_deq);
		dpy->xcb->pending_requests_tail = NULL;
	}
	else if (XLIB_SEQUENCE_COMPARE(req->sequence, >=,
	                               dpy->xcb->pending_requests->sequence))
		throw_thread_fail_assert("Unknown sequence number while "
		                         "dequeuing request",
		                         xcb_xlib_threads_sequence_lost);

	free(req);
}

static int handle_error(Display *dpy, xError *err, Bool in_XReply)
{
	_XExtension *ext;
	int ret_code;
	/* Oddly, Xlib only allows extensions to suppress errors when
	 * those errors were seen by _XReply. */
	if(in_XReply)
		/*
		 * we better see if there is an extension who may
		 * want to suppress the error.
		 */
		for(ext = dpy->ext_procs; ext; ext = ext->next)
			if(ext->error && (*ext->error)(dpy, err, &ext->codes, &ret_code))
				return ret_code;
	_XError(dpy, err);
	return 0;
}

/* Widen a 32-bit sequence number into a 64bit (uint64_t) sequence number.
 * Treating the comparison as a 1 and shifting it avoids a conditional branch.
 */
static void widen(uint64_t *wide, unsigned int narrow)
{
	uint64_t new = (*wide & ~((uint64_t)0xFFFFFFFFUL)) | narrow;
	/* If just copying the upper dword of *wide makes the number
	 * go down by more than 2^31, then it means that the lower
	 * dword has wrapped (or we have skipped 2^31 requests, which
	 * is hopefully improbable), so we add a carry. */
	uint64_t wraps = new + (1UL << 31) < *wide;
	*wide = new + (wraps << 32);
}

/* Thread-safety rules:
 *
 * At most one thread can be reading from XCB's event queue at a time.
 * If you are not the current event-reading thread and you need to find
 * out if an event is available, you must wait.
 *
 * The same rule applies for reading replies.
 *
 * A single thread cannot be both the the event-reading and the
 * reply-reading thread at the same time.
 *
 * We always look at both the current event and the first pending reply
 * to decide which to process next.
 *
 * We always process all responses in sequence-number order, which may
 * mean waiting for another thread (either the event_waiter or the
 * reply_waiter) to handle an earlier response before we can process or
 * return a later one. If so, we wait on the corresponding condition
 * variable for that thread to process the response and wake us up.
 */

static xcb_generic_reply_t *poll_for_event(Display *dpy, Bool queued_only)
{
	/* Make sure the Display's sequence numbers are valid */
	if (!require_socket(dpy))
		return NULL;

	/* Precondition: This thread can safely get events from XCB. */
	assert(dpy->xcb->event_owner == XlibOwnsEventQueue && !dpy->xcb->event_waiter);

	if(!dpy->xcb->next_event) {
		if(queued_only)
			dpy->xcb->next_event = xcb_poll_for_queued_event(dpy->xcb->connection);
		else
			dpy->xcb->next_event = xcb_poll_for_event(dpy->xcb->connection);
	}

	if(dpy->xcb->next_event)
	{
		PendingRequest *req = dpy->xcb->pending_requests;
		xcb_generic_event_t *event = dpy->xcb->next_event;
		uint64_t event_sequence = X_DPY_GET_LAST_REQUEST_READ(dpy);
		widen(&event_sequence, event->full_sequence);
		if(!req || XLIB_SEQUENCE_COMPARE(event_sequence, <, req->sequence)
		        || (event->response_type != X_Error && event_sequence == req->sequence))
		{
			uint64_t request = X_DPY_GET_REQUEST(dpy);
			if (XLIB_SEQUENCE_COMPARE(event_sequence, >, request))
			{
				throw_thread_fail_assert("Unknown sequence "
				                         "number while "
							 "processing queue",
				                xcb_xlib_threads_sequence_lost);
			}
			X_DPY_SET_LAST_REQUEST_READ(dpy, event_sequence);
			dpy->xcb->next_event = NULL;
			return (xcb_generic_reply_t *) event;
		}
	}
	return NULL;
}

static xcb_generic_reply_t *poll_for_response(Display *dpy)
{
	void *response;
	xcb_generic_reply_t *event;
	PendingRequest *req;

	while(1)
	{
		xcb_generic_error_t *error = NULL;
		uint64_t request;
		Bool poll_queued_only = dpy->xcb->next_response != NULL;

		/* Step 1: is there an event in our queue before the next
		 * reply/error? Return that first.
		 *
		 * If we don't have a reply/error saved from an earlier
		 * invocation we check incoming events too, otherwise only
		 * the ones already queued.
		 */
		response = poll_for_event(dpy, poll_queued_only);
		if(response)
			break;

		/* Step 2:
		 * Response is NULL, i.e. we have no events.
		 * If we are not waiting for a reply or some other thread
		 * had dibs on the next reply, exit.
		 */
		req = dpy->xcb->pending_requests;
		if(!req || req->reply_waiter)
			break;

		/* Step 3:
		 * We have some response (error or reply) related to req
		 * saved from an earlier invocation of this function. Let's
		 * use that one.
		 */
		if(dpy->xcb->next_response)
		{
			if (((xcb_generic_reply_t*)dpy->xcb->next_response)->response_type == X_Error)
			{
				error = dpy->xcb->next_response;
				response = NULL;
			}
			else
			{
				response = dpy->xcb->next_response;
				error = NULL;
			}
			dpy->xcb->next_response = NULL;
		}
		else
		{
			/* Step 4: pull down the next response from the wire. This
			 * should be the 99% case.
			 * xcb_poll_for_reply64() may also pull down events that
			 * happened before the reply.
			 */
			if(!xcb_poll_for_reply64(dpy->xcb->connection, req->sequence,
						 &response, &error)) {
				/* if there is no reply/error, xcb_poll_for_reply64
				 * may have read events. Return that. */
				response = poll_for_event(dpy, True);
				break;
			}

			/* Step 5: we have a new response, but we may also have some
			 * events that happened before that response. Return those
			 * first and save our reply/error for the next invocation.
			 */
			event = poll_for_event(dpy, True);
			if(event)
			{
				dpy->xcb->next_response = error ? error : response;
				response = event;
				break;
			}
		}

		/* Step 6: actually handle the reply/error now... */
		request = X_DPY_GET_REQUEST(dpy);
		if(XLIB_SEQUENCE_COMPARE(req->sequence, >, request))
		{
			throw_thread_fail_assert("Unknown sequence number "
			                         "while awaiting reply",
			                        xcb_xlib_threads_sequence_lost);
		}
		X_DPY_SET_LAST_REQUEST_READ(dpy, req->sequence);
		if(response)
			break;
		dequeue_pending_request(dpy, req);
		if(error)
			return (xcb_generic_reply_t *) error;
	}
	return response;
}

static void handle_response(Display *dpy, xcb_generic_reply_t *response, Bool in_XReply)
{
	_XAsyncHandler *async, *next;
	switch(response->response_type)
	{
	case X_Reply:
		for(async = dpy->async_handlers; async; async = next)
		{
			next = async->next;
			if(async->handler(dpy, (xReply *) response, (char *) response, sizeof(xReply) + (response->length << 2), async->data))
				break;
		}
		break;

	case X_Error:
		handle_error(dpy, (xError *) response, in_XReply);
		break;

	default: /* event */
		/* GenericEvents may be > 32 bytes. In this case, the
		 * event struct is trailed by the additional bytes. the
		 * xcb_generic_event_t struct uses 4 bytes for internal
		 * numbering, so we need to shift the trailing data to
		 * be after the first 32 bytes. */
		if(response->response_type == GenericEvent && ((xcb_ge_event_t *) response)->length)
		{
			xcb_ge_event_t *event = (xcb_ge_event_t *) response;
			memmove(&event->full_sequence, &event[1], event->length * 4);
		}
		_XEnq(dpy, (xEvent *) response);
		break;
	}
	free(response);
}

int _XEventsQueued(Display *dpy, int mode)
{
	xcb_generic_reply_t *response;
	if(dpy->flags & XlibDisplayIOError)
		return 0;
	if(dpy->xcb->event_owner != XlibOwnsEventQueue)
		return 0;

	if(mode == QueuedAfterFlush)
		_XSend(dpy, NULL, 0);
	else if (!check_internal_connections(dpy))
		return 0;

	/* If another thread is blocked waiting for events, then we must
	 * let that thread pick up the next event. Since it blocked, we
	 * can reasonably claim there are no new events right now. */
	if(!dpy->xcb->event_waiter)
	{
		while((response = poll_for_response(dpy)))
			handle_response(dpy, response, False);
		if(xcb_connection_has_error(dpy->xcb->connection)) {
			_XIOError(dpy);
			return 0;
		}
	}
	return dpy->qlen;
}

/* _XReadEvents - Flush the output queue,
 * then read as many events as possible (but at least 1) and enqueue them
 */
void _XReadEvents(Display *dpy)
{
	xcb_generic_reply_t *response;
	unsigned long serial;

	if(dpy->flags & XlibDisplayIOError)
		return;
	_XSend(dpy, NULL, 0);
	if(dpy->xcb->event_owner != XlibOwnsEventQueue)
		return;
	if (!check_internal_connections(dpy))
		return;

	serial = dpy->next_event_serial_num;
	while(serial == dpy->next_event_serial_num || dpy->qlen == 0)
	{
		if(dpy->xcb->event_waiter)
		{
			ConditionWait(dpy, dpy->xcb->event_notify);
			/* Maybe the other thread got us an event. */
			continue;
		}

		if(!dpy->xcb->next_event)
		{
			xcb_generic_event_t *event;
			dpy->xcb->event_waiter = 1;
			UnlockDisplay(dpy);
			event = xcb_wait_for_event(dpy->xcb->connection);
			/* It appears that classic Xlib respected user
			 * locks when waking up after waiting for
			 * events. However, if this thread did not have
			 * any user locks, and another thread takes a
			 * user lock and tries to read events, then we'd
			 * deadlock. So we'll choose to let the thread
			 * that got in first consume events, despite the
			 * later thread's user locks. */
			InternalLockDisplay(dpy, /* ignore user locks */ 1);
			dpy->xcb->event_waiter = 0;
			ConditionBroadcast(dpy, dpy->xcb->event_notify);
			if(!event)
			{
				_XIOError(dpy);
				return;
			}
			dpy->xcb->next_event = event;
		}

		/* We've established most of the conditions for
		 * poll_for_response to return non-NULL. The exceptions
		 * are connection shutdown, and finding that another
		 * thread is waiting for the next reply we'd like to
		 * process. */

		response = poll_for_response(dpy);
		if(response)
			handle_response(dpy, response, False);
		else if(dpy->xcb->pending_requests->reply_waiter)
		{ /* need braces around ConditionWait */
			ConditionWait(dpy, dpy->xcb->reply_notify);
		}
		else
		{
		        _XIOError(dpy);
		        return;
		}
	}

	/* The preceding loop established that there is no
	 * event_waiter--unless we just called ConditionWait because of
	 * a reply_waiter, in which case another thread may have become
	 * the event_waiter while we slept unlocked. */
	if(!dpy->xcb->event_waiter)
		while((response = poll_for_response(dpy)))
			handle_response(dpy, response, False);
	if(xcb_connection_has_error(dpy->xcb->connection))
		_XIOError(dpy);
}

/*
 * _XSend - Flush the buffer and send the client data. 32 bit word aligned
 * transmission is used, if size is not 0 mod 4, extra bytes are transmitted.
 *
 * Note that the connection must not be read from once the data currently
 * in the buffer has been written.
 */
void _XSend(Display *dpy, const char *data, long size)
{
	static const xReq dummy_request;
	static char const pad[3];
	struct iovec vec[3];
	uint64_t requests;
	uint64_t dpy_request;
	_XExtension *ext;
	xcb_connection_t *c = dpy->xcb->connection;
	if(dpy->flags & XlibDisplayIOError)
		return;

	if(dpy->bufptr == dpy->buffer && !size)
		return;

	/* append_pending_request does not alter the dpy request number
	 * therefore we can get it outside of the loop and the if
	 */
	dpy_request = X_DPY_GET_REQUEST(dpy);
	/* iff we asked XCB to set aside errors, we must pick those up
	 * eventually. iff there are async handlers, we may have just
	 * issued requests that will generate replies. in either case,
	 * we need to remember to check later. */
	if(dpy->xcb->event_owner != XlibOwnsEventQueue || dpy->async_handlers)
	{
		uint64_t sequence;
		for(sequence = dpy->xcb->last_flushed + 1; sequence <= dpy_request; ++sequence)
			append_pending_request(dpy, sequence);
	}
	requests = dpy_request - dpy->xcb->last_flushed;
	dpy->xcb->last_flushed = dpy_request;

	vec[0].iov_base = dpy->buffer;
	vec[0].iov_len = dpy->bufptr - dpy->buffer;
	vec[1].iov_base = (char *)data;
	vec[1].iov_len = size;
	vec[2].iov_base = (char *)pad;
	vec[2].iov_len = -size & 3;

	for(ext = dpy->flushes; ext; ext = ext->next_flush)
	{
		int i;
		for(i = 0; i < 3; ++i)
			if(vec[i].iov_len)
				ext->before_flush(dpy, &ext->codes, vec[i].iov_base, vec[i].iov_len);
	}

	if(xcb_writev(c, vec, 3, requests) < 0) {
		_XIOError(dpy);
		return;
	}
	dpy->bufptr = dpy->buffer;
	dpy->last_req = (char *) &dummy_request;

	if (!check_internal_connections(dpy))
		return;

	_XSetSeqSyncFunction(dpy);
}

/*
 * _XFlush - Flush the X request buffer.  If the buffer is empty, no
 * action is taken.
 */
void _XFlush(Display *dpy)
{
	if (!require_socket(dpy))
		return;

	_XSend(dpy, NULL, 0);

	_XEventsQueued(dpy, QueuedAfterReading);
}

static const XID inval_id = ~0UL;

void _XIDHandler(Display *dpy)
{
	if (dpy->xcb->next_xid == inval_id)
		_XAllocIDs(dpy, &dpy->xcb->next_xid, 1);
}

/* _XAllocID - resource ID allocation routine. */
XID _XAllocID(Display *dpy)
{
	XID ret = dpy->xcb->next_xid;
	assert (ret != inval_id);
	dpy->xcb->next_xid = inval_id;
	_XSetPrivSyncFunction(dpy);
	return ret;
}

/* _XAllocIDs - multiple resource ID allocation routine. */
void _XAllocIDs(Display *dpy, XID *ids, int count)
{
	int i;
#ifdef XTHREADS
	if (dpy->lock)
		(*dpy->lock->user_lock_display)(dpy);
	UnlockDisplay(dpy);
#endif
	for (i = 0; i < count; i++)
		ids[i] = xcb_generate_id(dpy->xcb->connection);
#ifdef XTHREADS
	InternalLockDisplay(dpy, /* don't skip user locks */ 0);
	if (dpy->lock)
		(*dpy->lock->user_unlock_display)(dpy);
#endif
}

static void _XFreeReplyData(Display *dpy, Bool force)
{
	if(!force && dpy->xcb->reply_consumed < dpy->xcb->reply_length)
		return;
	free(dpy->xcb->reply_data);
	dpy->xcb->reply_data = NULL;
}

/*
 * _XReply - Wait for a reply packet and copy its contents into the
 * specified rep.
 * extra: number of 32-bit words expected after the reply
 * discard: should I discard data following "extra" words?
 */
Status _XReply(Display *dpy, xReply *rep, int extra, Bool discard)
{
	xcb_generic_error_t *error;
	xcb_connection_t *c = dpy->xcb->connection;
	char *reply;
	PendingRequest *current;
	uint64_t dpy_request;

	if (dpy->xcb->reply_data)
		throw_extlib_fail_assert("Extra reply data still left in queue",
		                         xcb_xlib_extra_reply_data_left);

	if(dpy->flags & XlibDisplayIOError)
		return 0;

	_XSend(dpy, NULL, 0);
	dpy_request = X_DPY_GET_REQUEST(dpy);
	if(dpy->xcb->pending_requests_tail
	   && dpy->xcb->pending_requests_tail->sequence == dpy_request)
		current = dpy->xcb->pending_requests_tail;
	else
		current = append_pending_request(dpy, dpy_request);
	/* Don't let any other thread get this reply. */
	current->reply_waiter = 1;

	while(1)
	{
		PendingRequest *req = dpy->xcb->pending_requests;
		xcb_generic_reply_t *response;

		if(req != current && req->reply_waiter)
		{
			ConditionWait(dpy, dpy->xcb->reply_notify);
			/* Another thread got this reply. */
			continue;
		}
		req->reply_waiter = 1;
		UnlockDisplay(dpy);
		response = xcb_wait_for_reply64(c, req->sequence, &error);
		/* Any user locks on another thread must have been taken
		 * while we slept in xcb_wait_for_reply64. Classic Xlib
		 * ignored those user locks in this case, so we do too. */
		InternalLockDisplay(dpy, /* ignore user locks */ 1);

		/* We have the response we're looking for. Now, before
		 * letting anyone else process this sequence number, we
		 * need to process any events that should have come
		 * earlier. */

		if(dpy->xcb->event_owner == XlibOwnsEventQueue)
		{
			xcb_generic_reply_t *event;

			/* Assume event queue is empty if another thread is blocking
			 * waiting for event. */
			if(!dpy->xcb->event_waiter)
			{
				while((event = poll_for_response(dpy)))
					handle_response(dpy, event, True);
                        }
		}

		req->reply_waiter = 0;
		ConditionBroadcast(dpy, dpy->xcb->reply_notify);
		dpy_request = X_DPY_GET_REQUEST(dpy);
		if(XLIB_SEQUENCE_COMPARE(req->sequence, >, dpy_request)) {
			throw_thread_fail_assert("Unknown sequence number "
			                         "while processing reply",
			                        xcb_xlib_threads_sequence_lost);
		}
		X_DPY_SET_LAST_REQUEST_READ(dpy, req->sequence);
		if(!response)
			dequeue_pending_request(dpy, req);

		if(req == current)
		{
			reply = (char *) response;
			break;
		}

		if(error)
			handle_response(dpy, (xcb_generic_reply_t *) error, True);
		else if(response)
			handle_response(dpy, response, True);
	}
	if (!check_internal_connections(dpy))
		return 0;

	if(dpy->xcb->next_event && dpy->xcb->next_event->response_type == X_Error)
	{
		xcb_generic_event_t *event = dpy->xcb->next_event;
		uint64_t last_request_read = X_DPY_GET_LAST_REQUEST_READ(dpy);
		uint64_t event_sequence = last_request_read;
		widen(&event_sequence, event->full_sequence);
		if(event_sequence == last_request_read)
		{
			error = (xcb_generic_error_t *) event;
			dpy->xcb->next_event = NULL;
		}
	}

	if(error)
	{
		int ret_code;

		/* Xlib is evil and assumes that even errors will be
		 * copied into rep. */
		memcpy(rep, error, 32);

		/* do not die on "no such font", "can't allocate",
		   "can't grab" failures */
		switch(error->error_code)
		{
			case BadName:
				switch(error->major_code)
				{
					case X_LookupColor:
					case X_AllocNamedColor:
						free(error);
						return 0;
				}
				break;
			case BadFont:
				if(error->major_code == X_QueryFont) {
					free(error);
					return 0;
				}
				break;
			case BadAlloc:
			case BadAccess:
				free(error);
				return 0;
		}

		ret_code = handle_error(dpy, (xError *) error, True);
		free(error);
		return ret_code;
	}

	/* it's not an error, but we don't have a reply, so it's an I/O
	 * error. */
	if(!reply) {
		_XIOError(dpy);
		return 0;
	}

	/* there's no error and we have a reply. */
	dpy->xcb->reply_data = reply;
	dpy->xcb->reply_consumed = sizeof(xReply) + (extra * 4);
	dpy->xcb->reply_length = sizeof(xReply);
	if(dpy->xcb->reply_data[0] == 1)
		dpy->xcb->reply_length += (((xcb_generic_reply_t *) dpy->xcb->reply_data)->length * 4);

	/* error: Xlib asks too much. give them what we can anyway. */
	if(dpy->xcb->reply_length < dpy->xcb->reply_consumed)
		dpy->xcb->reply_consumed = dpy->xcb->reply_length;

	memcpy(rep, dpy->xcb->reply_data, dpy->xcb->reply_consumed);
	_XFreeReplyData(dpy, discard);
	return 1;
}

int _XRead(Display *dpy, char *data, long size)
{
	assert(size >= 0);
	if(size == 0)
		return 0;
	if(dpy->xcb->reply_data == NULL ||
	   dpy->xcb->reply_consumed + size > dpy->xcb->reply_length)
		throw_extlib_fail_assert("Too much data requested from _XRead",
		                         xcb_xlib_too_much_data_requested);
	memcpy(data, dpy->xcb->reply_data + dpy->xcb->reply_consumed, size);
	dpy->xcb->reply_consumed += size;
	_XFreeReplyData(dpy, False);
	return 0;
}

/*
 * _XReadPad - Read bytes from the socket taking into account incomplete
 * reads.  If the number of bytes is not 0 mod 4, read additional pad
 * bytes.
 */
void _XReadPad(Display *dpy, char *data, long size)
{
	_XRead(dpy, data, size);
	dpy->xcb->reply_consumed += -size & 3;
	_XFreeReplyData(dpy, False);
}

/* Read and discard "n" 8-bit bytes of data */
void _XEatData(Display *dpy, unsigned long n)
{
	dpy->xcb->reply_consumed += n;
	_XFreeReplyData(dpy, False);
}

/*
 * Read and discard "n" 32-bit words of data
 * Matches the units of the length field in X protocol replies, and provides
 * a single implementation of overflow checking to avoid having to replicate
 * those checks in every caller.
 */
void _XEatDataWords(Display *dpy, unsigned long n)
{
	if (n < ((INT_MAX - dpy->xcb->reply_consumed) >> 2))
		dpy->xcb->reply_consumed += (n << 2);
	else
		/* Overflow would happen, so just eat the rest of the reply */
		dpy->xcb->reply_consumed = dpy->xcb->reply_length;
	_XFreeReplyData(dpy, False);
}

unsigned long
_XNextRequest(Display *dpy)
{
    /* This will update dpy->request. The assumption is that the next thing
     * that the application will do is make a request so there's little
     * overhead.
     */
    require_socket(dpy);
    return NextRequest(dpy);
}
