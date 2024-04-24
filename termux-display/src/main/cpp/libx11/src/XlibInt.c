/*

Copyright 1985, 1986, 1987, 1998  The Open Group

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

*/

/*
 *	XlibInt.c - Internal support routines for the C subroutine
 *	interface library (Xlib) to the X Window System Protocol V11.0.
 */

#ifdef WIN32
#define _XLIBINT_
#endif
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xprivate.h"
#include "reallocarray.h"
#include <X11/Xpoll.h>
#include <assert.h>
#include <stdio.h>
#ifdef WIN32
#include <direct.h>
#endif

/* Needed for FIONREAD on Solaris */
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

/* Needed for FIONREAD on Cygwin */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

/* Needed for ioctl() on Solaris */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef XTHREADS
#include "locking.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

/* these pointers get initialized by XInitThreads */
LockInfoPtr _Xglobal_lock = NULL;
void (*_XCreateMutex_fn)(LockInfoPtr) = NULL;
/* struct _XCVList *(*_XCreateCVL_fn)() = NULL; */
void (*_XFreeMutex_fn)(LockInfoPtr) = NULL;
void (*_XLockMutex_fn)(
    LockInfoPtr /* lock */
#if defined(XTHREADS_WARN) || defined(XTHREADS_FILE_LINE)
    , char * /* file */
    , int /* line */
#endif
    ) = NULL;
void (*_XUnlockMutex_fn)(
    LockInfoPtr /* lock */
#if defined(XTHREADS_WARN) || defined(XTHREADS_FILE_LINE)
    , char * /* file */
    , int /* line */
#endif
    ) = NULL;
xthread_t (*_Xthread_self_fn)(void) = NULL;

#define XThread_Self()	((*_Xthread_self_fn)())

#endif /* XTHREADS */

#ifdef WIN32
#define ECHECK(err) (WSAGetLastError() == err)
#define ESET(val) WSASetLastError(val)
#else
#ifdef __UNIXOS2__
#define ECHECK(err) (errno == err)
#define ESET(val)
#else
#define ECHECK(err) (errno == err)
#define ESET(val) errno = val
#endif
#endif

#ifdef __UNIXOS2__
#include <limits.h>
#define MAX_PATH _POSIX_PATH_MAX
#endif

/*
 * The following routines are internal routines used by Xlib for protocol
 * packet transmission and reception.
 *
 * _XIOError(Display *) will be called if any sort of system call error occurs.
 * This is assumed to be a fatal condition, i.e., XIOError should not return.
 *
 * _XError(Display *, xError *) will be called whenever an X_Error event is
 * received.  This is not assumed to be a fatal condition, i.e., it is
 * acceptable for this procedure to return.  However, XError should NOT
 * perform any operations (directly or indirectly) on the DISPLAY.
 *
 * Routines declared with a return type of 'Status' return 0 on failure,
 * and non 0 on success.  Routines with no declared return type don't
 * return anything.  Whenever possible routines that create objects return
 * the object they have created.
 */

#define POLLFD_CACHE_SIZE 5

/* initialize the struct array passed to poll() below */
Bool _XPollfdCacheInit(
    Display *dpy)
{
#ifdef USE_POLL
    struct pollfd *pfp;

    pfp = Xmalloc(POLLFD_CACHE_SIZE * sizeof(struct pollfd));
    if (!pfp)
	return False;
    pfp[0].fd = dpy->fd;
    pfp[0].events = POLLIN;

    dpy->filedes = (XPointer)pfp;
#endif
    return True;
}

void _XPollfdCacheAdd(
    Display *dpy,
    int fd)
{
#ifdef USE_POLL
    struct pollfd *pfp = (struct pollfd *)dpy->filedes;

    if (dpy->im_fd_length <= POLLFD_CACHE_SIZE) {
	pfp[dpy->im_fd_length].fd = fd;
	pfp[dpy->im_fd_length].events = POLLIN;
    }
#endif
}

/* ARGSUSED */
void _XPollfdCacheDel(
    Display *dpy,
    int fd)			/* not used */
{
#ifdef USE_POLL
    struct pollfd *pfp = (struct pollfd *)dpy->filedes;
    struct _XConnectionInfo *conni;

    /* just recalculate whole list */
    if (dpy->im_fd_length <= POLLFD_CACHE_SIZE) {
	int loc = 1;
	for (conni = dpy->im_fd_info; conni; conni=conni->next) {
	    pfp[loc].fd = conni->fd;
	    pfp[loc].events = POLLIN;
	    loc++;
	}
    }
#endif
}

static int sync_hazard(Display *dpy)
{
    /*
     * "span" and "hazard" need to be signed such that the ">=" comparison
     * works correctly in the case that hazard is greater than 65525
     */
    int64_t span = X_DPY_GET_REQUEST(dpy) - X_DPY_GET_LAST_REQUEST_READ(dpy);
    int64_t hazard = min((dpy->bufmax - dpy->buffer) / SIZEOF(xReq), 65535 - 10);
    return span >= 65535 - hazard - 10;
}

static
void sync_while_locked(Display *dpy)
{
#ifdef XTHREADS
    if (dpy->lock)
        (*dpy->lock->user_lock_display)(dpy);
#endif
    UnlockDisplay(dpy);
    SyncHandle();
    InternalLockDisplay(dpy, /* don't skip user locks */ 0);
#ifdef XTHREADS
    if (dpy->lock)
        (*dpy->lock->user_unlock_display)(dpy);
#endif
}

void _XSeqSyncFunction(
    register Display *dpy)
{
    xGetInputFocusReply rep;
    _X_UNUSED register xReq *req;

    if ((X_DPY_GET_REQUEST(dpy) - X_DPY_GET_LAST_REQUEST_READ(dpy)) >= (65535 - BUFSIZE/SIZEOF(xReq))) {
	GetEmptyReq(GetInputFocus, req);
	(void) _XReply (dpy, (xReply *)&rep, 0, xTrue);
	sync_while_locked(dpy);
    } else if (sync_hazard(dpy))
	_XSetPrivSyncFunction(dpy);
}

/* NOTE: only called if !XTHREADS, or when XInitThreads wasn't called. */
static int
_XPrivSyncFunction (Display *dpy)
{
#ifdef XTHREADS
    assert(!dpy->lock_fns);
#endif
    assert(dpy->synchandler == _XPrivSyncFunction);
    assert((dpy->flags & XlibDisplayPrivSync) != 0);
    dpy->synchandler = dpy->savedsynchandler;
    dpy->savedsynchandler = NULL;
    dpy->flags &= ~XlibDisplayPrivSync;
    if(dpy->synchandler)
        dpy->synchandler(dpy);
    _XIDHandler(dpy);
    _XSeqSyncFunction(dpy);
    return 0;
}

void _XSetPrivSyncFunction(Display *dpy)
{
#ifdef XTHREADS
    if (dpy->lock_fns)
        return;
#endif
    if (!(dpy->flags & XlibDisplayPrivSync)) {
	dpy->savedsynchandler = dpy->synchandler;
	dpy->synchandler = _XPrivSyncFunction;
	dpy->flags |= XlibDisplayPrivSync;
    }
}

void _XSetSeqSyncFunction(Display *dpy)
{
    if (sync_hazard(dpy))
	_XSetPrivSyncFunction (dpy);
}

#ifdef LONG64
void _XRead32(
    Display *dpy,
    long *data,
    long len)
{
    register int *buf;
    register long i;

    if (len) {
	(void) _XRead(dpy, (char *)data, len);
	i = len >> 2;
	buf = (int *)data + i;
	data += i;
	while (--i >= 0)
	    *--data = *--buf;
    }
}
#endif /* LONG64 */


/*
 * The hard part about this is that we only get 16 bits from a reply.
 * We have three values that will march along, with the following invariant:
 *	dpy->last_request_read <= rep->sequenceNumber <= dpy->request
 * We have to keep
 *	dpy->request - dpy->last_request_read < 2^16
 * or else we won't know for sure what value to use in events.  We do this
 * by forcing syncs when we get close.
 */

unsigned long
_XSetLastRequestRead(
    register Display *dpy,
    register xGenericReply *rep)
{
    register uint64_t	newseq, lastseq;

    lastseq = X_DPY_GET_LAST_REQUEST_READ(dpy);
    /*
     * KeymapNotify has no sequence number, but is always guaranteed
     * to immediately follow another event, except when generated via
     * SendEvent (hmmm).
     */
    if ((rep->type & 0x7f) == KeymapNotify)
	return(lastseq);

    newseq = (lastseq & ~((uint64_t)0xffff)) | rep->sequenceNumber;

    if (newseq < lastseq) {
	newseq += 0x10000;
	if (newseq > X_DPY_GET_REQUEST(dpy)) {
	    (void) fprintf (stderr,
	    "Xlib: sequence lost (0x%llx > 0x%llx) in reply type 0x%x!\n",
			    (unsigned long long)newseq,
			    (unsigned long long)(X_DPY_GET_REQUEST(dpy)),
			    (unsigned int) rep->type);
	    newseq -= 0x10000;
	}
    }

    X_DPY_SET_LAST_REQUEST_READ(dpy, newseq);
    return(newseq);
}

/*
 * Support for internal connections, such as an IM might use.
 * By Stephen Gildea, X Consortium, September 1993
 */

/* _XRegisterInternalConnection
 * Each IM (or Xlib extension) that opens a file descriptor that Xlib should
 * include in its select/poll mask must call this function to register the
 * fd with Xlib.  Any XConnectionWatchProc registered by XAddConnectionWatch
 * will also be called.
 *
 * Whenever Xlib detects input available on fd, it will call callback
 * with call_data to process it.  If non-Xlib code calls select/poll
 * and detects input available, it must call XProcessInternalConnection,
 * which will call the associated callback.
 *
 * Non-Xlib code can learn about these additional fds by calling
 * XInternalConnectionNumbers or, more typically, by registering
 * a XConnectionWatchProc with XAddConnectionWatch
 * to be called when fds are registered or unregistered.
 *
 * Returns True if registration succeeded, False if not, typically
 * because could not allocate memory.
 * Assumes Display locked when called.
 */
Status
_XRegisterInternalConnection(
    Display* dpy,
    int fd,
    _XInternalConnectionProc callback,
    XPointer call_data
)
{
    struct _XConnectionInfo *new_conni, **iptr;
    struct _XConnWatchInfo *watchers;
    XPointer *wd;

    new_conni = Xmalloc(sizeof(struct _XConnectionInfo));
    if (!new_conni)
	return 0;
    new_conni->watch_data = Xmallocarray(dpy->watcher_count, sizeof(XPointer));
    if (!new_conni->watch_data) {
	Xfree(new_conni);
	return 0;
    }
    new_conni->fd = fd;
    new_conni->read_callback = callback;
    new_conni->call_data = call_data;
    new_conni->next = NULL;
    /* link new structure onto end of list */
    for (iptr = &dpy->im_fd_info; *iptr; iptr = &(*iptr)->next)
	;
    *iptr = new_conni;
    dpy->im_fd_length++;
    _XPollfdCacheAdd(dpy, fd);

    for (watchers=dpy->conn_watchers, wd=new_conni->watch_data;
	 watchers;
	 watchers=watchers->next, wd++) {
	*wd = NULL;		/* for cleanliness */
	(*watchers->fn) (dpy, watchers->client_data, fd, True, wd);
    }

    return 1;
}

/* _XUnregisterInternalConnection
 * Each IM (or Xlib extension) that closes a file descriptor previously
 * registered with _XRegisterInternalConnection must call this function.
 * Any XConnectionWatchProc registered by XAddConnectionWatch
 * will also be called.
 *
 * Assumes Display locked when called.
 */
void
_XUnregisterInternalConnection(
    Display* dpy,
    int fd
)
{
    struct _XConnectionInfo *info_list, **prev;
    struct _XConnWatchInfo *watch;
    XPointer *wd;

    for (prev = &dpy->im_fd_info; (info_list = *prev);
	 prev = &info_list->next) {
	if (info_list->fd == fd) {
	    *prev = info_list->next;
	    dpy->im_fd_length--;
	    for (watch=dpy->conn_watchers, wd=info_list->watch_data;
		 watch;
		 watch=watch->next, wd++) {
		(*watch->fn) (dpy, watch->client_data, fd, False, wd);
	    }
	    Xfree (info_list->watch_data);
	    Xfree (info_list);
	    break;
	}
    }
    _XPollfdCacheDel(dpy, fd);
}

/* XInternalConnectionNumbers
 * Returns an array of fds and an array of corresponding call data.
 * Typically a XConnectionWatchProc registered with XAddConnectionWatch
 * will be used instead of this function to discover
 * additional fds to include in the select/poll mask.
 *
 * The list is allocated with Xmalloc and should be freed by the caller
 * with Xfree;
 */
Status
XInternalConnectionNumbers(
    Display *dpy,
    int **fd_return,
    int *count_return
)
{
    int count;
    struct _XConnectionInfo *info_list;
    int *fd_list;

    LockDisplay(dpy);
    count = 0;
    for (info_list=dpy->im_fd_info; info_list; info_list=info_list->next)
	count++;
    fd_list = Xmallocarray (count, sizeof(int));
    if (!fd_list) {
	UnlockDisplay(dpy);
	return 0;
    }
    count = 0;
    for (info_list=dpy->im_fd_info; info_list; info_list=info_list->next) {
	fd_list[count] = info_list->fd;
	count++;
    }
    UnlockDisplay(dpy);

    *fd_return = fd_list;
    *count_return = count;
    return 1;
}

void _XProcessInternalConnection(
    Display *dpy,
    struct _XConnectionInfo *conn_info)
{
    dpy->flags |= XlibDisplayProcConni;
    UnlockDisplay(dpy);
    (*conn_info->read_callback) (dpy, conn_info->fd, conn_info->call_data);
    LockDisplay(dpy);
    dpy->flags &= ~XlibDisplayProcConni;
}

/* XProcessInternalConnection
 * Call the _XInternalConnectionProc registered by _XRegisterInternalConnection
 * for this fd.
 * The Display is NOT locked during the call.
 */
void
XProcessInternalConnection(
    Display* dpy,
    int fd
)
{
    struct _XConnectionInfo *info_list;

    LockDisplay(dpy);
    for (info_list=dpy->im_fd_info; info_list; info_list=info_list->next) {
	if (info_list->fd == fd) {
	    _XProcessInternalConnection(dpy, info_list);
	    break;
	}
    }
    UnlockDisplay(dpy);
}

/* XAddConnectionWatch
 * Register a callback to be called whenever _XRegisterInternalConnection
 * or _XUnregisterInternalConnection is called.
 * Callbacks are called with the Display locked.
 * If any connections are already registered, the callback is immediately
 * called for each of them.
 */
Status
XAddConnectionWatch(
    Display* dpy,
    XConnectionWatchProc callback,
    XPointer client_data
)
{
    struct _XConnWatchInfo *new_watcher, **wptr;
    struct _XConnectionInfo *info_list;
    XPointer *wd_array;

    LockDisplay(dpy);

    /* allocate new watch data */
    for (info_list=dpy->im_fd_info; info_list; info_list=info_list->next) {
	wd_array = Xreallocarray(info_list->watch_data,
                                 dpy->watcher_count + 1, sizeof(XPointer));
	if (!wd_array) {
	    UnlockDisplay(dpy);
	    return 0;
	}
	info_list->watch_data = wd_array;
	wd_array[dpy->watcher_count] = NULL;	/* for cleanliness */
    }

    new_watcher = Xmalloc(sizeof(struct _XConnWatchInfo));
    if (!new_watcher) {
	UnlockDisplay(dpy);
	return 0;
    }
    new_watcher->fn = callback;
    new_watcher->client_data = client_data;
    new_watcher->next = NULL;

    /* link new structure onto end of list */
    for (wptr = &dpy->conn_watchers; *wptr; wptr = &(*wptr)->next)
	;
    *wptr = new_watcher;
    dpy->watcher_count++;

    /* call new watcher on all currently registered fds */
    for (info_list=dpy->im_fd_info; info_list; info_list=info_list->next) {
	(*callback) (dpy, client_data, info_list->fd, True,
		     info_list->watch_data + dpy->watcher_count - 1);
    }

    UnlockDisplay(dpy);
    return 1;
}

/* XRemoveConnectionWatch
 * Unregister a callback registered by XAddConnectionWatch.
 * Both callback and client_data must match what was passed to
 * XAddConnectionWatch.
 */
void
XRemoveConnectionWatch(
    Display* dpy,
    XConnectionWatchProc callback,
    XPointer client_data
)
{
    struct _XConnWatchInfo *watch;
    struct _XConnWatchInfo *previous = NULL;
    struct _XConnectionInfo *conni;
    int counter = 0;

    LockDisplay(dpy);
    for (watch=dpy->conn_watchers; watch; watch=watch->next) {
	if (watch->fn == callback  &&  watch->client_data == client_data) {
	    if (previous)
		previous->next = watch->next;
	    else
		dpy->conn_watchers = watch->next;
	    Xfree (watch);
	    dpy->watcher_count--;
	    /* remove our watch_data for each connection */
	    for (conni=dpy->im_fd_info; conni; conni=conni->next) {
		/* don't bother realloc'ing; these arrays are small anyway */
		/* overlapping */
		memmove(conni->watch_data+counter,
			conni->watch_data+counter+1,
			dpy->watcher_count - counter);
	    }
	    break;
	}
	previous = watch;
	counter++;
    }
    UnlockDisplay(dpy);
}

/* end of internal connections support */

/* Cookie jar implementation
   dpy->cookiejar is a linked list. _XEnq receives the events but leaves
   them in the normal EQ. _XStoreEvent returns the cookie event (minus
   data pointer) and adds it to the cookiejar. _XDeq just removes
   the entry like any other event but resets the data pointer for
   cookie events (to avoid double-free, the memory is re-used by Xlib).

   _XFetchEventCookie (called from XGetEventData) removes a cookie from the
   jar. _XFreeEventCookies removes all unclaimed cookies from the jar
   (called by XNextEvent).

   _XFreeDisplayStructure calls _XFreeEventCookies for each cookie in the
   normal EQ.
 */

#include "utlist.h"
struct stored_event {
    XGenericEventCookie ev;
    struct stored_event *prev;
    struct stored_event *next;
};

Bool
_XIsEventCookie(Display *dpy, XEvent *ev)
{
    return (ev->xcookie.type == GenericEvent &&
	    dpy->generic_event_vec[ev->xcookie.extension & 0x7F] != NULL);
}

/**
 * Free all events in the event list.
 */
void
_XFreeEventCookies(Display *dpy)
{
    struct stored_event **head, *e, *tmp;

    if (!dpy->cookiejar)
        return;

    head = (struct stored_event**)&dpy->cookiejar;

    DL_FOREACH_SAFE(*head, e, tmp) {
        XFree(e->ev.data);
        XFree(e);
    }
    dpy->cookiejar = NULL;
}

/**
 * Add an event to the display's event list. This event must be freed on the
 * next call to XNextEvent().
 */
void
_XStoreEventCookie(Display *dpy, XEvent *event)
{
    XGenericEventCookie* cookie = &event->xcookie;
    struct stored_event **head, *add;

    if (!_XIsEventCookie(dpy, event))
        return;

    head = (struct stored_event**)(&dpy->cookiejar);

    add = Xmalloc(sizeof(struct stored_event));
    if (!add) {
        ESET(ENOMEM);
        _XIOError(dpy);
        return;
    }
    add->ev = *cookie;
    DL_APPEND(*head, add);
    cookie->data = NULL; /* don't return data yet, must be claimed */
}

/**
 * Return the event with the given cookie and remove it from the list.
 */
Bool
_XFetchEventCookie(Display *dpy, XGenericEventCookie* ev)
{
    Bool ret = False;
    struct stored_event **head, *event;
    head = (struct stored_event**)&dpy->cookiejar;

    if (!_XIsEventCookie(dpy, (XEvent*)ev))
        return ret;

    DL_FOREACH(*head, event) {
        if (event->ev.cookie == ev->cookie &&
            event->ev.extension == ev->extension &&
            event->ev.evtype == ev->evtype) {
            *ev = event->ev;
            DL_DELETE(*head, event);
            Xfree(event);
            ret = True;
            break;
        }
    }

    return ret;
}

Bool
_XCopyEventCookie(Display *dpy, XGenericEventCookie *in, XGenericEventCookie *out)
{
    Bool ret = False;
    int extension;

    if (!_XIsEventCookie(dpy, (XEvent*)in) || !out)
        return ret;

    extension = in->extension & 0x7F;

    if (!dpy->generic_event_copy_vec[extension])
        return ret;

    ret = ((*dpy->generic_event_copy_vec[extension])(dpy, in, out));
    out->cookie = ret ? ++dpy->next_cookie  : 0;
    return ret;
}


/*
 * _XEnq - Place event packets on the display's queue.
 * note that no squishing of move events in V11, since there
 * is pointer motion hints....
 */
void _XEnq(
	register Display *dpy,
	register xEvent *event)
{
	register _XQEvent *qelt;
	int type, extension;

	if ((qelt = dpy->qfree)) {
		/* If dpy->qfree is non-NULL do this, else malloc a new one. */
		dpy->qfree = qelt->next;
	}
	else if ((qelt = Xmalloc(sizeof(_XQEvent))) == NULL) {
		/* Malloc call failed! */
		ESET(ENOMEM);
		_XIOError(dpy);
		return;
	}
	qelt->next = NULL;

	type = event->u.u.type & 0177;
	extension = ((xGenericEvent*)event)->extension;

	qelt->event.type = type;
	/* If an extension has registered a generic_event_vec handler, then
	 * it can handle event cookies. Otherwise, proceed with the normal
	 * event handlers.
	 *
	 * If the generic_event_vec is called, qelt->event is a event cookie
	 * with the data pointer and the "free" pointer set. Data pointer is
	 * some memory allocated by the extension.
	 */
        if (type == GenericEvent && dpy->generic_event_vec[extension & 0x7F]) {
	    XGenericEventCookie *cookie = &qelt->event.xcookie;
	    (*dpy->generic_event_vec[extension & 0x7F])(dpy, cookie, event);
	    cookie->cookie = ++dpy->next_cookie;

	    qelt->qserial_num = dpy->next_event_serial_num++;
	    if (dpy->tail)	dpy->tail->next = qelt;
	    else		dpy->head = qelt;

	    dpy->tail = qelt;
	    dpy->qlen++;
	} else if ((*dpy->event_vec[type])(dpy, &qelt->event, event)) {
	    qelt->qserial_num = dpy->next_event_serial_num++;
	    if (dpy->tail)	dpy->tail->next = qelt;
	    else 		dpy->head = qelt;

	    dpy->tail = qelt;
	    dpy->qlen++;
	} else {
	    /* ignored, or stashed away for many-to-one compression */
	    qelt->next = dpy->qfree;
	    dpy->qfree = qelt;
	}
}

/*
 * _XDeq - Remove event packet from the display's queue.
 */
void _XDeq(
    register Display *dpy,
    register _XQEvent *prev,	/* element before qelt */
    register _XQEvent *qelt)	/* element to be unlinked */
{
    if (prev) {
	if ((prev->next = qelt->next) == NULL)
	    dpy->tail = prev;
    } else {
	/* no prev, so removing first elt */
	if ((dpy->head = qelt->next) == NULL)
	    dpy->tail = NULL;
    }
    qelt->qserial_num = 0;
    qelt->next = dpy->qfree;
    dpy->qfree = qelt;
    dpy->qlen--;

    if (_XIsEventCookie(dpy, &qelt->event)) {
	XGenericEventCookie* cookie = &qelt->event.xcookie;
	/* dpy->qfree is re-used, reset memory to avoid double free on
	 * _XFreeDisplayStructure */
	cookie->data = NULL;
    }
}

/*
 * EventToWire in separate file in that often not needed.
 */

/*ARGSUSED*/
Bool
_XUnknownWireEvent(
    register Display *dpy,	/* pointer to display structure */
    register XEvent *re,	/* pointer to where event should be reformatted */
    register xEvent *event)	/* wire protocol event */
{
#ifdef notdef
	(void) fprintf(stderr,
	    "Xlib: unhandled wire event! event number = %d, display = %x\n.",
			event->u.u.type, dpy);
#endif
	return(False);
}

Bool
_XUnknownWireEventCookie(
    Display *dpy,	/* pointer to display structure */
    XGenericEventCookie *re,	/* pointer to where event should be reformatted */
    xEvent *event)	/* wire protocol event */
{
#ifdef notdef
	fprintf(stderr,
	    "Xlib: unhandled wire cookie event! extension number = %d, display = %x\n.",
			((xGenericEvent*)event)->extension, dpy);
#endif
	return(False);
}

Bool
_XUnknownCopyEventCookie(
    Display *dpy,	/* pointer to display structure */
    XGenericEventCookie *in,	/* source */
    XGenericEventCookie *out)	/* destination */
{
#ifdef notdef
	fprintf(stderr,
	    "Xlib: unhandled cookie event copy! extension number = %d, display = %x\n.",
			in->extension, dpy);
#endif
	return(False);
}

/*ARGSUSED*/
Status
_XUnknownNativeEvent(
    register Display *dpy,	/* pointer to display structure */
    register XEvent *re,	/* pointer to where event should be reformatted */
    register xEvent *event)	/* wire protocol event */
{
#ifdef notdef
	(void) fprintf(stderr,
 	   "Xlib: unhandled native event! event number = %d, display = %x\n.",
			re->type, dpy);
#endif
	return(0);
}
/*
 * reformat a wire event into an XEvent structure of the right type.
 */
Bool
_XWireToEvent(
    register Display *dpy,	/* pointer to display structure */
    register XEvent *re,	/* pointer to where event should be reformatted */
    register xEvent *event)	/* wire protocol event */
{

	re->type = event->u.u.type & 0x7f;
	((XAnyEvent *)re)->serial = _XSetLastRequestRead(dpy,
					(xGenericReply *)event);
	((XAnyEvent *)re)->send_event = ((event->u.u.type & 0x80) != 0);
	((XAnyEvent *)re)->display = dpy;

	/* Ignore the leading bit of the event type since it is set when a
		client sends an event rather than the server. */

	switch (event-> u.u.type & 0177) {
	      case KeyPress:
	      case KeyRelease:
	        {
			register XKeyEvent *ev = (XKeyEvent*) re;
			ev->root 	= event->u.keyButtonPointer.root;
			ev->window 	= event->u.keyButtonPointer.event;
			ev->subwindow 	= event->u.keyButtonPointer.child;
			ev->time 	= event->u.keyButtonPointer.time;
			ev->x 		= cvtINT16toInt(event->u.keyButtonPointer.eventX);
			ev->y 		= cvtINT16toInt(event->u.keyButtonPointer.eventY);
			ev->x_root 	= cvtINT16toInt(event->u.keyButtonPointer.rootX);
			ev->y_root 	= cvtINT16toInt(event->u.keyButtonPointer.rootY);
			ev->state	= event->u.keyButtonPointer.state;
			ev->same_screen	= event->u.keyButtonPointer.sameScreen;
			ev->keycode 	= event->u.u.detail;
		}
	      	break;
	      case ButtonPress:
	      case ButtonRelease:
	        {
			register XButtonEvent *ev =  (XButtonEvent *) re;
			ev->root 	= event->u.keyButtonPointer.root;
			ev->window 	= event->u.keyButtonPointer.event;
			ev->subwindow 	= event->u.keyButtonPointer.child;
			ev->time 	= event->u.keyButtonPointer.time;
			ev->x 		= cvtINT16toInt(event->u.keyButtonPointer.eventX);
			ev->y 		= cvtINT16toInt(event->u.keyButtonPointer.eventY);
			ev->x_root 	= cvtINT16toInt(event->u.keyButtonPointer.rootX);
			ev->y_root 	= cvtINT16toInt(event->u.keyButtonPointer.rootY);
			ev->state	= event->u.keyButtonPointer.state;
			ev->same_screen	= event->u.keyButtonPointer.sameScreen;
			ev->button 	= event->u.u.detail;
		}
	        break;
	      case MotionNotify:
	        {
			register XMotionEvent *ev =   (XMotionEvent *)re;
			ev->root 	= event->u.keyButtonPointer.root;
			ev->window 	= event->u.keyButtonPointer.event;
			ev->subwindow 	= event->u.keyButtonPointer.child;
			ev->time 	= event->u.keyButtonPointer.time;
			ev->x 		= cvtINT16toInt(event->u.keyButtonPointer.eventX);
			ev->y 		= cvtINT16toInt(event->u.keyButtonPointer.eventY);
			ev->x_root 	= cvtINT16toInt(event->u.keyButtonPointer.rootX);
			ev->y_root 	= cvtINT16toInt(event->u.keyButtonPointer.rootY);
			ev->state	= event->u.keyButtonPointer.state;
			ev->same_screen	= event->u.keyButtonPointer.sameScreen;
			ev->is_hint 	= event->u.u.detail;
		}
	        break;
	      case EnterNotify:
	      case LeaveNotify:
		{
			register XCrossingEvent *ev   = (XCrossingEvent *) re;
			ev->root	= event->u.enterLeave.root;
			ev->window	= event->u.enterLeave.event;
			ev->subwindow	= event->u.enterLeave.child;
			ev->time	= event->u.enterLeave.time;
			ev->x		= cvtINT16toInt(event->u.enterLeave.eventX);
			ev->y		= cvtINT16toInt(event->u.enterLeave.eventY);
			ev->x_root	= cvtINT16toInt(event->u.enterLeave.rootX);
			ev->y_root	= cvtINT16toInt(event->u.enterLeave.rootY);
			ev->state	= event->u.enterLeave.state;
			ev->mode	= event->u.enterLeave.mode;
			ev->same_screen = (event->u.enterLeave.flags &
				ELFlagSameScreen) && True;
			ev->focus	= (event->u.enterLeave.flags &
			  	ELFlagFocus) && True;
			ev->detail	= event->u.u.detail;
		}
		  break;
	      case FocusIn:
	      case FocusOut:
		{
			register XFocusChangeEvent *ev = (XFocusChangeEvent *) re;
			ev->window 	= event->u.focus.window;
			ev->mode	= event->u.focus.mode;
			ev->detail	= event->u.u.detail;
		}
		  break;
	      case KeymapNotify:
		{
			register XKeymapEvent *ev = (XKeymapEvent *) re;
			ev->window	= None;
			memcpy(&ev->key_vector[1],
			       (char *)((xKeymapEvent *) event)->map,
			       sizeof (((xKeymapEvent *) event)->map));
		}
		break;
	      case Expose:
		{
			register XExposeEvent *ev = (XExposeEvent *) re;
			ev->window	= event->u.expose.window;
			ev->x		= event->u.expose.x;
			ev->y		= event->u.expose.y;
			ev->width	= event->u.expose.width;
			ev->height	= event->u.expose.height;
			ev->count	= event->u.expose.count;
		}
		break;
	      case GraphicsExpose:
		{
		    register XGraphicsExposeEvent *ev =
			(XGraphicsExposeEvent *) re;
		    ev->drawable	= event->u.graphicsExposure.drawable;
		    ev->x		= event->u.graphicsExposure.x;
		    ev->y		= event->u.graphicsExposure.y;
		    ev->width		= event->u.graphicsExposure.width;
		    ev->height		= event->u.graphicsExposure.height;
		    ev->count		= event->u.graphicsExposure.count;
		    ev->major_code	= event->u.graphicsExposure.majorEvent;
		    ev->minor_code	= event->u.graphicsExposure.minorEvent;
		}
		break;
	      case NoExpose:
		{
		    register XNoExposeEvent *ev = (XNoExposeEvent *) re;
		    ev->drawable	= event->u.noExposure.drawable;
		    ev->major_code	= event->u.noExposure.majorEvent;
		    ev->minor_code	= event->u.noExposure.minorEvent;
		}
		break;
	      case VisibilityNotify:
		{
		    register XVisibilityEvent *ev = (XVisibilityEvent *) re;
		    ev->window		= event->u.visibility.window;
		    ev->state		= event->u.visibility.state;
		}
		break;
	      case CreateNotify:
		{
		    register XCreateWindowEvent *ev =
			 (XCreateWindowEvent *) re;
		    ev->window		= event->u.createNotify.window;
		    ev->parent		= event->u.createNotify.parent;
		    ev->x		= cvtINT16toInt(event->u.createNotify.x);
		    ev->y		= cvtINT16toInt(event->u.createNotify.y);
		    ev->width		= event->u.createNotify.width;
		    ev->height		= event->u.createNotify.height;
		    ev->border_width	= event->u.createNotify.borderWidth;
		    ev->override_redirect	= event->u.createNotify.override;
		}
		break;
	      case DestroyNotify:
		{
		    register XDestroyWindowEvent *ev =
				(XDestroyWindowEvent *) re;
		    ev->window		= event->u.destroyNotify.window;
		    ev->event		= event->u.destroyNotify.event;
		}
		break;
	      case UnmapNotify:
		{
		    register XUnmapEvent *ev = (XUnmapEvent *) re;
		    ev->window		= event->u.unmapNotify.window;
		    ev->event		= event->u.unmapNotify.event;
		    ev->from_configure	= event->u.unmapNotify.fromConfigure;
		}
		break;
	      case MapNotify:
		{
		    register XMapEvent *ev = (XMapEvent *) re;
		    ev->window		= event->u.mapNotify.window;
		    ev->event		= event->u.mapNotify.event;
		    ev->override_redirect	= event->u.mapNotify.override;
		}
		break;
	      case MapRequest:
		{
		    register XMapRequestEvent *ev = (XMapRequestEvent *) re;
		    ev->window		= event->u.mapRequest.window;
		    ev->parent		= event->u.mapRequest.parent;
		}
		break;
	      case ReparentNotify:
		{
		    register XReparentEvent *ev = (XReparentEvent *) re;
		    ev->event		= event->u.reparent.event;
		    ev->window		= event->u.reparent.window;
		    ev->parent		= event->u.reparent.parent;
		    ev->x		= cvtINT16toInt(event->u.reparent.x);
		    ev->y		= cvtINT16toInt(event->u.reparent.y);
		    ev->override_redirect	= event->u.reparent.override;
		}
		break;
	      case ConfigureNotify:
		{
		    register XConfigureEvent *ev = (XConfigureEvent *) re;
		    ev->event	= event->u.configureNotify.event;
		    ev->window	= event->u.configureNotify.window;
		    ev->above	= event->u.configureNotify.aboveSibling;
		    ev->x	= cvtINT16toInt(event->u.configureNotify.x);
		    ev->y	= cvtINT16toInt(event->u.configureNotify.y);
		    ev->width	= event->u.configureNotify.width;
		    ev->height	= event->u.configureNotify.height;
		    ev->border_width  = event->u.configureNotify.borderWidth;
		    ev->override_redirect = event->u.configureNotify.override;
		}
		break;
	      case ConfigureRequest:
		{
		    register XConfigureRequestEvent *ev =
		        (XConfigureRequestEvent *) re;
		    ev->window		= event->u.configureRequest.window;
		    ev->parent		= event->u.configureRequest.parent;
		    ev->above		= event->u.configureRequest.sibling;
		    ev->x		= cvtINT16toInt(event->u.configureRequest.x);
		    ev->y		= cvtINT16toInt(event->u.configureRequest.y);
		    ev->width		= event->u.configureRequest.width;
		    ev->height		= event->u.configureRequest.height;
		    ev->border_width	= event->u.configureRequest.borderWidth;
		    ev->value_mask	= event->u.configureRequest.valueMask;
		    ev->detail  	= event->u.u.detail;
		}
		break;
	      case GravityNotify:
		{
		    register XGravityEvent *ev = (XGravityEvent *) re;
		    ev->window		= event->u.gravity.window;
		    ev->event		= event->u.gravity.event;
		    ev->x		= cvtINT16toInt(event->u.gravity.x);
		    ev->y		= cvtINT16toInt(event->u.gravity.y);
		}
		break;
	      case ResizeRequest:
		{
		    register XResizeRequestEvent *ev =
			(XResizeRequestEvent *) re;
		    ev->window		= event->u.resizeRequest.window;
		    ev->width		= event->u.resizeRequest.width;
		    ev->height		= event->u.resizeRequest.height;
		}
		break;
	      case CirculateNotify:
		{
		    register XCirculateEvent *ev = (XCirculateEvent *) re;
		    ev->window		= event->u.circulate.window;
		    ev->event		= event->u.circulate.event;
		    ev->place		= event->u.circulate.place;
		}
		break;
	      case CirculateRequest:
		{
		    register XCirculateRequestEvent *ev =
		        (XCirculateRequestEvent *) re;
		    ev->window		= event->u.circulate.window;
		    ev->parent		= event->u.circulate.event;
		    ev->place		= event->u.circulate.place;
		}
		break;
	      case PropertyNotify:
		{
		    register XPropertyEvent *ev = (XPropertyEvent *) re;
		    ev->window		= event->u.property.window;
		    ev->atom		= event->u.property.atom;
		    ev->time		= event->u.property.time;
		    ev->state		= event->u.property.state;
		}
		break;
	      case SelectionClear:
		{
		    register XSelectionClearEvent *ev =
			 (XSelectionClearEvent *) re;
		    ev->window		= event->u.selectionClear.window;
		    ev->selection	= event->u.selectionClear.atom;
		    ev->time		= event->u.selectionClear.time;
		}
		break;
	      case SelectionRequest:
		{
		    register XSelectionRequestEvent *ev =
		        (XSelectionRequestEvent *) re;
		    ev->owner		= event->u.selectionRequest.owner;
		    ev->requestor	= event->u.selectionRequest.requestor;
		    ev->selection	= event->u.selectionRequest.selection;
		    ev->target		= event->u.selectionRequest.target;
		    ev->property	= event->u.selectionRequest.property;
		    ev->time		= event->u.selectionRequest.time;
		}
		break;
	      case SelectionNotify:
		{
		    register XSelectionEvent *ev = (XSelectionEvent *) re;
		    ev->requestor	= event->u.selectionNotify.requestor;
		    ev->selection	= event->u.selectionNotify.selection;
		    ev->target		= event->u.selectionNotify.target;
		    ev->property	= event->u.selectionNotify.property;
		    ev->time		= event->u.selectionNotify.time;
		}
		break;
	      case ColormapNotify:
		{
		    register XColormapEvent *ev = (XColormapEvent *) re;
		    ev->window		= event->u.colormap.window;
		    ev->colormap	= event->u.colormap.colormap;
		    ev->new		= event->u.colormap.new;
		    ev->state		= event->u.colormap.state;
	        }
		break;
	      case ClientMessage:
		{
		   register int i;
		   register XClientMessageEvent *ev
		   			= (XClientMessageEvent *) re;
		   ev->window		= event->u.clientMessage.window;
		   ev->format		= event->u.u.detail;
		   switch (ev->format) {
			case 8:
			   ev->message_type = event->u.clientMessage.u.b.type;
			   for (i = 0; i < 20; i++)
			     ev->data.b[i] = event->u.clientMessage.u.b.bytes[i];
			   break;
			case 16:
			   ev->message_type = event->u.clientMessage.u.s.type;
			   ev->data.s[0] = cvtINT16toShort(event->u.clientMessage.u.s.shorts0);
			   ev->data.s[1] = cvtINT16toShort(event->u.clientMessage.u.s.shorts1);
			   ev->data.s[2] = cvtINT16toShort(event->u.clientMessage.u.s.shorts2);
			   ev->data.s[3] = cvtINT16toShort(event->u.clientMessage.u.s.shorts3);
			   ev->data.s[4] = cvtINT16toShort(event->u.clientMessage.u.s.shorts4);
			   ev->data.s[5] = cvtINT16toShort(event->u.clientMessage.u.s.shorts5);
			   ev->data.s[6] = cvtINT16toShort(event->u.clientMessage.u.s.shorts6);
			   ev->data.s[7] = cvtINT16toShort(event->u.clientMessage.u.s.shorts7);
			   ev->data.s[8] = cvtINT16toShort(event->u.clientMessage.u.s.shorts8);
			   ev->data.s[9] = cvtINT16toShort(event->u.clientMessage.u.s.shorts9);
			   break;
			case 32:
			   ev->message_type = event->u.clientMessage.u.l.type;
			   ev->data.l[0] = cvtINT32toLong(event->u.clientMessage.u.l.longs0);
			   ev->data.l[1] = cvtINT32toLong(event->u.clientMessage.u.l.longs1);
			   ev->data.l[2] = cvtINT32toLong(event->u.clientMessage.u.l.longs2);
			   ev->data.l[3] = cvtINT32toLong(event->u.clientMessage.u.l.longs3);
			   ev->data.l[4] = cvtINT32toLong(event->u.clientMessage.u.l.longs4);
			   break;
			default: /* XXX should never occur */
				break;
		    }
	        }
		break;
	      case MappingNotify:
		{
		   register XMappingEvent *ev = (XMappingEvent *)re;
		   ev->window		= 0;
		   ev->first_keycode 	= event->u.mappingNotify.firstKeyCode;
		   ev->request 		= event->u.mappingNotify.request;
		   ev->count 		= event->u.mappingNotify.count;
		}
		break;
	      default:
		return(_XUnknownWireEvent(dpy, re, event));
	}
	return(True);
}

static int
SocketBytesReadable(Display *dpy)
{
    int bytes = 0, last_error;
#ifdef WIN32
    last_error = WSAGetLastError();
    ioctlsocket(ConnectionNumber(dpy), FIONREAD, &bytes);
    WSASetLastError(last_error);
#else
    last_error = errno;
    ioctl(ConnectionNumber(dpy), FIONREAD, &bytes);
    errno = last_error;
#endif
    return bytes;
}

_X_NORETURN void _XDefaultIOErrorExit(
	Display *dpy,
	void *user_data)
{
    exit(1);
    /*NOTREACHED*/
}

/*
 * _XDefaultIOError - Default fatal system error reporting routine.  Called
 * when an X internal system error is encountered.
 */
_X_NORETURN int _XDefaultIOError(
	Display *dpy)
{
	int killed = ECHECK(EPIPE);

	/*
	 * If the socket was closed on the far end, the final recvmsg in
	 * xcb will have thrown EAGAIN because we're non-blocking. Detect
	 * this to get the more informative error message.
	 */
	if (ECHECK(EAGAIN) && SocketBytesReadable(dpy) <= 0)
	    killed = True;

	if (killed) {
	    fprintf (stderr,
                     "X connection to %s broken (explicit kill or server shutdown).\r\n",
                     DisplayString (dpy));
	} else {
            fprintf (stderr,
                     "XIO:  fatal IO error %d (%s) on X server \"%s\"\r\n",
#ifdef WIN32
                      WSAGetLastError(), strerror(WSAGetLastError()),
#else
                      errno, strerror (errno),
#endif
                      DisplayString (dpy));
	    fprintf (stderr,
		     "      after %lu requests (%lu known processed) with %d events remaining.\r\n",
		     NextRequest(dpy) - 1, LastKnownRequestProcessed(dpy),
		     QLength(dpy));
        }

	exit(1);
	/*NOTREACHED*/
}


static int _XPrintDefaultError(
    Display *dpy,
    XErrorEvent *event,
    FILE *fp)
{
    char buffer[BUFSIZ];
    char mesg[BUFSIZ];
    char number[32];
    const char *mtype = "XlibMessage";
    register _XExtension *ext = (_XExtension *)NULL;
    _XExtension *bext = (_XExtension *)NULL;
    XGetErrorText(dpy, event->error_code, buffer, BUFSIZ);
    XGetErrorDatabaseText(dpy, mtype, "XError", "X Error", mesg, BUFSIZ);
    (void) fprintf(fp, "%s:  %s\n  ", mesg, buffer);
    XGetErrorDatabaseText(dpy, mtype, "MajorCode", "Request Major code %d",
	mesg, BUFSIZ);
    (void) fprintf(fp, mesg, event->request_code);
    if (event->request_code < 128) {
	snprintf(number, sizeof(number), "%d", event->request_code);
	XGetErrorDatabaseText(dpy, "XRequest", number, "", buffer, BUFSIZ);
    } else {
	for (ext = dpy->ext_procs;
	     ext && (ext->codes.major_opcode != event->request_code);
	     ext = ext->next)
	  ;
	if (ext) {
	    strncpy(buffer, ext->name, BUFSIZ);
	    buffer[BUFSIZ - 1] = '\0';
        } else
	    buffer[0] = '\0';
    }
    (void) fprintf(fp, " (%s)\n", buffer);
    if (event->request_code >= 128) {
	XGetErrorDatabaseText(dpy, mtype, "MinorCode", "Request Minor code %d",
			      mesg, BUFSIZ);
	fputs("  ", fp);
	(void) fprintf(fp, mesg, event->minor_code);
	if (ext) {
	    snprintf(mesg, sizeof(mesg), "%s.%d", ext->name, event->minor_code);
	    XGetErrorDatabaseText(dpy, "XRequest", mesg, "", buffer, BUFSIZ);
	    (void) fprintf(fp, " (%s)", buffer);
	}
	fputs("\n", fp);
    }
    if (event->error_code >= 128) {
	/* kludge, try to find the extension that caused it */
	buffer[0] = '\0';
	for (ext = dpy->ext_procs; ext; ext = ext->next) {
	    if (ext->error_string)
		(*ext->error_string)(dpy, event->error_code, &ext->codes,
				     buffer, BUFSIZ);
	    if (buffer[0]) {
		bext = ext;
		break;
	    }
	    if (ext->codes.first_error &&
		ext->codes.first_error < (int)event->error_code &&
		(!bext || ext->codes.first_error > bext->codes.first_error))
		bext = ext;
	}
	if (bext)
	    snprintf(buffer, sizeof(buffer), "%s.%d", bext->name,
                     event->error_code - bext->codes.first_error);
	else
	    strcpy(buffer, "Value");
	XGetErrorDatabaseText(dpy, mtype, buffer, "", mesg, BUFSIZ);
	if (mesg[0]) {
	    fputs("  ", fp);
	    (void) fprintf(fp, mesg, event->resourceid);
	    fputs("\n", fp);
	}
	/* let extensions try to print the values */
	for (ext = dpy->ext_procs; ext; ext = ext->next) {
	    if (ext->error_values)
		(*ext->error_values)(dpy, event, fp);
	}
    } else if ((event->error_code == BadWindow) ||
	       (event->error_code == BadPixmap) ||
	       (event->error_code == BadCursor) ||
	       (event->error_code == BadFont) ||
	       (event->error_code == BadDrawable) ||
	       (event->error_code == BadColor) ||
	       (event->error_code == BadGC) ||
	       (event->error_code == BadIDChoice) ||
	       (event->error_code == BadValue) ||
	       (event->error_code == BadAtom)) {
	if (event->error_code == BadValue)
	    XGetErrorDatabaseText(dpy, mtype, "Value", "Value 0x%x",
				  mesg, BUFSIZ);
	else if (event->error_code == BadAtom)
	    XGetErrorDatabaseText(dpy, mtype, "AtomID", "AtomID 0x%x",
				  mesg, BUFSIZ);
	else
	    XGetErrorDatabaseText(dpy, mtype, "ResourceID", "ResourceID 0x%x",
				  mesg, BUFSIZ);
	fputs("  ", fp);
	(void) fprintf(fp, mesg, event->resourceid);
	fputs("\n", fp);
    }
    XGetErrorDatabaseText(dpy, mtype, "ErrorSerial", "Error Serial #%d",
			  mesg, BUFSIZ);
    fputs("  ", fp);
    (void) fprintf(fp, mesg, event->serial);
    XGetErrorDatabaseText(dpy, mtype, "CurrentSerial", "Current Serial #%lld",
			  mesg, BUFSIZ);
    fputs("\n  ", fp);
    (void) fprintf(fp, mesg, (unsigned long long)(X_DPY_GET_REQUEST(dpy)));
    fputs("\n", fp);
    if (event->error_code == BadImplementation) return 0;
    return 1;
}

int _XDefaultError(
	Display *dpy,
	XErrorEvent *event)
{
    if (_XPrintDefaultError (dpy, event, stderr) == 0) return 0;

    /*
     * Store in dpy flags that the client is exiting on an unhandled XError
     * (pretend it is an IOError, since the application is dying anyway it
     * does not make a difference).
     * This is useful for _XReply not to hang if the application makes Xlib
     * calls in _fini as part of process termination.
     */
    dpy->flags |= XlibDisplayIOError;

    exit(1);
    /*NOTREACHED*/
}

/*ARGSUSED*/
Bool _XDefaultWireError(Display *display, XErrorEvent *he, xError *we)
{
    return True;
}

/*
 * _XError - upcall internal or user protocol error handler
 */
int _XError (
    Display *dpy,
    register xError *rep)
{
    /*
     * X_Error packet encountered!  We need to unpack the error before
     * giving it to the user.
     */
    XEvent event; /* make it a large event */
    register _XAsyncHandler *async, *next;

    event.xerror.serial = _XSetLastRequestRead(dpy, (xGenericReply *)rep);

    for (async = dpy->async_handlers; async; async = next) {
	next = async->next;
	if ((*async->handler)(dpy, (xReply *)rep,
			      (char *)rep, SIZEOF(xError), async->data))
	    return 0;
    }

    event.xerror.display = dpy;
    event.xerror.type = X_Error;
    event.xerror.resourceid = rep->resourceID;
    event.xerror.error_code = rep->errorCode;
    event.xerror.request_code = rep->majorCode;
    event.xerror.minor_code = rep->minorCode;
    if (dpy->error_vec &&
	!(*dpy->error_vec[rep->errorCode])(dpy, &event.xerror, rep))
	return 0;
    if (_XErrorFunction != NULL) {
	int rtn_val;
#ifdef XTHREADS
	struct _XErrorThreadInfo thread_info = {
		.error_thread = xthread_self(),
		.next = dpy->error_threads
	}, **prev;
	dpy->error_threads = &thread_info;
	if (dpy->lock)
	    (*dpy->lock->user_lock_display)(dpy);
	UnlockDisplay(dpy);
#endif
	rtn_val = (*_XErrorFunction)(dpy, (XErrorEvent *)&event); /* upcall */
#ifdef XTHREADS
	LockDisplay(dpy);
	if (dpy->lock)
	    (*dpy->lock->user_unlock_display)(dpy);

	/* unlink thread_info from the list */
	for (prev = &dpy->error_threads; *prev != &thread_info; prev = &(*prev)->next)
		;
	*prev = thread_info.next;
#endif
	return rtn_val;
    } else {
	return _XDefaultError(dpy, (XErrorEvent *)&event);
    }
}

/*
 * _XIOError - call user connection error handler and exit
 */
int
_XIOError (
    Display *dpy)
{
    XIOErrorExitHandler exit_handler;
    void *exit_handler_data;

    dpy->flags |= XlibDisplayIOError;
#ifdef WIN32
    errno = WSAGetLastError();
#endif

    /* This assumes that the thread calling exit will call any atexit handlers.
     * If this does not hold, then an alternate solution would involve
     * registering an atexit handler to take over the lock, which would only
     * assume that the same thread calls all the atexit handlers. */
#ifdef XTHREADS
    if (dpy->lock)
	(*dpy->lock->user_lock_display)(dpy);
#endif
    exit_handler = dpy->exit_handler;
    exit_handler_data = dpy->exit_handler_data;
    UnlockDisplay(dpy);

    if (_XIOErrorFunction != NULL)
	(*_XIOErrorFunction)(dpy);
    else
	_XDefaultIOError(dpy);

    exit_handler(dpy, exit_handler_data);
    return 1;
}


/*
 * This routine can be used to (cheaply) get some memory within a single
 * Xlib routine for scratch space.  A single buffer is reused each time
 * if possible.  To be MT safe, you can only call this between a call to
 * GetReq* and a call to Data* or _XSend*, or in a context when the thread
 * is guaranteed to not unlock the display.
 */
char *_XAllocScratch(
	register Display *dpy,
	unsigned long nbytes)
{
	if (nbytes > dpy->scratch_length) {
	    Xfree (dpy->scratch_buffer);
	    dpy->scratch_buffer = Xmalloc(nbytes);
	    if (dpy->scratch_buffer)
		dpy->scratch_length = nbytes;
	    else dpy->scratch_length = 0;
	}
	return (dpy->scratch_buffer);
}

/*
 * Scratch space allocator you can call any time, multiple times, and be
 * MT safe, but you must hand the buffer back with _XFreeTemp.
 */
char *_XAllocTemp(
    register Display *dpy,
    unsigned long nbytes)
{
    char *buf;

    buf = _XAllocScratch(dpy, nbytes);
    dpy->scratch_buffer = NULL;
    dpy->scratch_length = 0;
    return buf;
}

void _XFreeTemp(
    register Display *dpy,
    char *buf,
    unsigned long nbytes)
{

    Xfree(dpy->scratch_buffer);
    dpy->scratch_buffer = buf;
    dpy->scratch_length = nbytes;
}

/*
 * Given a visual id, find the visual structure for this id on this display.
 */
Visual *_XVIDtoVisual(
	Display *dpy,
	VisualID id)
{
	register int i, j, k;
	register Screen *sp;
	register Depth *dp;
	register Visual *vp;
	for (i = 0; i < dpy->nscreens; i++) {
		sp = &dpy->screens[i];
		for (j = 0; j < sp->ndepths; j++) {
			dp = &sp->depths[j];
			/* if nvisuals == 0 then visuals will be NULL */
			for (k = 0; k < dp->nvisuals; k++) {
				vp = &dp->visuals[k];
				if (vp->visualid == id) return (vp);
			}
		}
	}
	return (NULL);
}

int
XFree (void *data)
{
	Xfree (data);
	return 1;
}

#ifdef _XNEEDBCOPYFUNC
void _Xbcopy(b1, b2, length)
    register char *b1, *b2;
    register length;
{
    if (b1 < b2) {
	b2 += length;
	b1 += length;
	while (length--)
	    *--b2 = *--b1;
    } else {
	while (length--)
	    *b2++ = *b1++;
    }
}
#endif

#ifdef DataRoutineIsProcedure
void Data(
	Display *dpy,
	_Xconst char *data,
	long len)
{
	if (dpy->bufptr + (len) <= dpy->bufmax) {
		memcpy(dpy->bufptr, data, (int)len);
		dpy->bufptr += ((len) + 3) & ~3;
	} else {
		_XSend(dpy, data, len);
	}
}
#endif /* DataRoutineIsProcedure */


#ifdef LONG64
int
_XData32(
    Display *dpy,
    _Xconst long *data,
    unsigned len)
{
    register int *buf;
    register long i;

    while (len) {
	buf = (int *)dpy->bufptr;
	i = dpy->bufmax - (char *)buf;
	if (!i) {
	    _XFlush(dpy);
	    continue;
	}
	if (len < i)
	    i = len;
	dpy->bufptr = (char *)buf + i;
	len -= i;
	i >>= 2;
	while (--i >= 0)
	    *buf++ = *data++;
    }
    return 0;
}
#endif /* LONG64 */



/* Make sure this produces the same string as DefineLocal/DefineSelf in xdm.
 * Otherwise, Xau will not be able to find your cookies in the Xauthority file.
 *
 * Note: POSIX says that the ``nodename'' member of utsname does _not_ have
 *       to have sufficient information for interfacing to the network,
 *       and so, you may be better off using gethostname (if it exists).
 */

#if (defined(_POSIX_SOURCE) && !defined(AIXV3) && !defined(__QNX__)) || defined(hpux) || defined(SVR4)
#define NEED_UTSNAME
#include <sys/utsname.h>
#else
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

/*
 * _XGetHostname - similar to gethostname but allows special processing.
 */
int _XGetHostname (
    char *buf,
    int maxlen)
{
    int len;

#ifdef NEED_UTSNAME
    struct utsname name;

    if (maxlen <= 0 || buf == NULL)
	return 0;

    uname (&name);
    len = (int) strlen (name.nodename);
    if (len >= maxlen) len = maxlen - 1;
    strncpy (buf, name.nodename, (size_t) len);
    buf[len] = '\0';
#else
    if (maxlen <= 0 || buf == NULL)
	return 0;

    buf[0] = '\0';
    (void) gethostname (buf, maxlen);
    buf [maxlen - 1] = '\0';
    len = (int) strlen(buf);
#endif /* NEED_UTSNAME */
    return len;
}


/*
 * _XScreenOfWindow - get the Screen of a given window
 */

Screen *_XScreenOfWindow(Display *dpy, Window w)
{
    register int i;
    Window root;
    int x, y;				/* dummy variables */
    unsigned int width, height, bw, depth;  /* dummy variables */

    if (XGetGeometry (dpy, w, &root, &x, &y, &width, &height,
		      &bw, &depth) == False) {
	return NULL;
    }
    for (i = 0; i < ScreenCount (dpy); i++) {	/* find root from list */
	if (root == RootWindow (dpy, i)) {
	    return ScreenOfDisplay (dpy, i);
	}
    }
    return NULL;
}


/*
 * WARNING: This implementation's pre-conditions and post-conditions
 * must remain compatible with the old macro-based implementations of
 * GetReq, GetReqExtra, GetResReq, and GetEmptyReq. The portions of the
 * Display structure affected by those macros are part of libX11's
 * ABI.
 */
void *_XGetRequest(Display *dpy, CARD8 type, size_t len)
{
    xReq *req;

    if (dpy->bufptr + len > dpy->bufmax)
	_XFlush(dpy);
    /* Request still too large, so do not allow it to overflow. */
    if (dpy->bufptr + len > dpy->bufmax) {
	fprintf(stderr,
		"Xlib: request %d length %zd would exceed buffer size.\n",
		type, len);
	/* Changes failure condition from overflow to NULL dereference. */
	return NULL;
    }

    if (len % 4)
	fprintf(stderr,
		"Xlib: request %d length %zd not a multiple of 4.\n",
		type, len);

    dpy->last_req = dpy->bufptr;

    req = (xReq*)dpy->bufptr;
    req->reqType = type;
    req->length = len / 4;
    dpy->bufptr += len;
    X_DPY_REQUEST_INCREMENT(dpy);
    return req;
}

#if defined(WIN32)

/*
 * These functions are intended to be used internally to Xlib only.
 * These functions will always prefix the path with a DOS drive in the
 * form "<drive-letter>:". As such, these functions are only suitable
 * for use by Xlib function that supply a root-based path to some
 * particular file, e.g. <ProjectRoot>/lib/X11/locale/locale.dir will
 * be converted to "C:/usr/X11R6.3/lib/X11/locale/locale.dir".
 */

static int access_file (path, pathbuf, len_pathbuf, pathret)
    char* path;
    char* pathbuf;
    int len_pathbuf;
    char** pathret;
{
    if (access (path, F_OK) == 0) {
	if (strlen (path) < len_pathbuf)
	    *pathret = pathbuf;
	else
	    *pathret = Xmalloc (strlen (path) + 1);
	if (*pathret) {
	    strcpy (*pathret, path);
	    return 1;
	}
    }
    return 0;
}

static int AccessFile (path, pathbuf, len_pathbuf, pathret)
    char* path;
    char* pathbuf;
    int len_pathbuf;
    char** pathret;
{
    unsigned long drives;
    int i, len;
    char* drive;
    char buf[MAX_PATH];
    char* bufp;

    /* just try the "raw" name first and see if it works */
    if (access_file (path, pathbuf, len_pathbuf, pathret))
	return 1;

    /* try the places set in the environment */
    drive = getenv ("_XBASEDRIVE");
#ifdef __UNIXOS2__
    if (!drive)
	drive = getenv ("X11ROOT");
#endif
    if (!drive)
	drive = "C:";
    len = strlen (drive) + strlen (path);
    if (len < MAX_PATH) bufp = buf;
    else bufp = Xmalloc (len + 1);
    strcpy (bufp, drive);
    strcat (bufp, path);
    if (access_file (bufp, pathbuf, len_pathbuf, pathret)) {
	if (bufp != buf) Xfree (bufp);
	return 1;
    }

#ifndef __UNIXOS2__
    /* one last place to look */
    drive = getenv ("HOMEDRIVE");
    if (drive) {
	len = strlen (drive) + strlen (path);
	if (len < MAX_PATH) bufp = buf;
	else bufp = Xmalloc (len + 1);
	strcpy (bufp, drive);
	strcat (bufp, path);
	if (access_file (bufp, pathbuf, len_pathbuf, pathret)) {
	    if (bufp != buf) Xfree (bufp);
	    return 1;
	}
    }

    /* tried everywhere else, go fishing */
#define C_DRIVE ('C' - 'A')
#define Z_DRIVE ('Z' - 'A')
    /* does OS/2 (with or with gcc-emx) have getdrives? */
    drives = _getdrives ();
    for (i = C_DRIVE; i <= Z_DRIVE; i++) { /* don't check on A: or B: */
	if ((1 << i) & drives) {
	    len = 2 + strlen (path);
	    if (len < MAX_PATH) bufp = buf;
	    else bufp = Xmalloc (len + 1);
	    *bufp = 'A' + i;
	    *(bufp + 1) = ':';
	    *(bufp + 2) = '\0';
	    strcat (bufp, path);
	    if (access_file (bufp, pathbuf, len_pathbuf, pathret)) {
		if (bufp != buf) Xfree (bufp);
		return 1;
	    }
	}
    }
#endif
    return 0;
}

int _XOpenFile(path, flags)
    _Xconst char* path;
    int flags;
{
    char buf[MAX_PATH];
    char* bufp = NULL;
    int ret = -1;
    UINT olderror = SetErrorMode (SEM_FAILCRITICALERRORS);

    if (AccessFile (path, buf, MAX_PATH, &bufp))
	ret = open (bufp, flags);

    (void) SetErrorMode (olderror);

    if (bufp != buf) Xfree (bufp);

    return ret;
}

int _XOpenFileMode(path, flags, mode)
    _Xconst char* path;
    int flags;
    mode_t mode;
{
    char buf[MAX_PATH];
    char* bufp = NULL;
    int ret = -1;
    UINT olderror = SetErrorMode (SEM_FAILCRITICALERRORS);

    if (AccessFile (path, buf, MAX_PATH, &bufp))
	ret = open (bufp, flags, mode);

    (void) SetErrorMode (olderror);

    if (bufp != buf) Xfree (bufp);

    return ret;
}

void* _XFopenFile(path, mode)
    _Xconst char* path;
    _Xconst char* mode;
{
    char buf[MAX_PATH];
    char* bufp = NULL;
    void* ret = NULL;
    UINT olderror = SetErrorMode (SEM_FAILCRITICALERRORS);

    if (AccessFile (path, buf, MAX_PATH, &bufp))
	ret = fopen (bufp, mode);

    (void) SetErrorMode (olderror);

    if (bufp != buf) Xfree (bufp);

    return ret;
}

int _XAccessFile(path)
    _Xconst char* path;
{
    char buf[MAX_PATH];
    char* bufp;
    int ret = -1;
    UINT olderror = SetErrorMode (SEM_FAILCRITICALERRORS);

    ret = AccessFile (path, buf, MAX_PATH, &bufp);

    (void) SetErrorMode (olderror);

    if (bufp != buf) Xfree (bufp);

    return ret;
}

#endif

#ifdef WIN32
#undef _Xdebug
int _Xdebug = 0;
int *_Xdebug_p = &_Xdebug;
void (**_XCreateMutex_fn_p)(LockInfoPtr) = &_XCreateMutex_fn;
void (**_XFreeMutex_fn_p)(LockInfoPtr) = &_XFreeMutex_fn;
void (**_XLockMutex_fn_p)(LockInfoPtr
#if defined(XTHREADS_WARN) || defined(XTHREADS_FILE_LINE)
    , char * /* file */
    , int /* line */
#endif
        ) = &_XLockMutex_fn;
void (**_XUnlockMutex_fn_p)(LockInfoPtr
#if defined(XTHREADS_WARN) || defined(XTHREADS_FILE_LINE)
    , char * /* file */
    , int /* line */
#endif
        ) = &_XUnlockMutex_fn;
LockInfoPtr *_Xglobal_lock_p = &_Xglobal_lock;
#endif
