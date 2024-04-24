/*

Copyright 1985, 1987, 1998  The Open Group

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"

/*
 * Check existing events in queue to find if any match.  If so, return.
 * If not, flush buffer and see if any more events are readable. If one
 * matches, return.  If all else fails, tell the user no events found.
 */

Bool XCheckIfEvent (
	register Display *dpy,
	register XEvent *event,		/* XEvent to be filled in. */
	Bool (*predicate)(
			  Display*			/* display */,
			  XEvent*			/* event */,
			  char*				/* arg */
			  ),		/* function to call */
	char *arg)
{
	register _XQEvent *prev, *qelt;
	unsigned long qe_serial = 0;
	int n;			/* time through count */

	LockDisplay(dpy);
#ifdef XTHREADS
	dpy->ifevent_thread = xthread_self();
#endif
	dpy->in_ifevent++;
	prev = NULL;
	for (n = 3; --n >= 0;) {
	    for (qelt = prev ? prev->next : dpy->head;
		 qelt;
		 prev = qelt, qelt = qelt->next) {
		if(qelt->qserial_num > qe_serial
		   && (*predicate)(dpy, &qelt->event, arg)) {
		    *event = qelt->event;
		    _XDeq(dpy, prev, qelt);
		    _XStoreEventCookie(dpy, event);
		    dpy->in_ifevent--;
		    UnlockDisplay(dpy);
		    return True;
		}
	    }
	    if (prev)
		qe_serial = prev->qserial_num;
	    switch (n) {
	      case 2:
		_XEventsQueued(dpy, QueuedAfterReading);
		break;
	      case 1:
		_XFlush(dpy);
		break;
	    }
	    if (prev && prev->qserial_num != qe_serial)
		/* another thread has snatched this event */
		prev = NULL;
	}
	dpy->in_ifevent--;
	UnlockDisplay(dpy);
	return False;
}
