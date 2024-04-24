/*

Copyright 1986, 1998  The Open Group

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
 * return the next event in the queue that satisfies the predicate.
 * BUT do not remove it from the queue.
 * If none found, flush, and then wait until one satisfies the predicate.
 */

int
XPeekIfEvent (
	register Display *dpy,
	register XEvent *event,
	Bool (*predicate)(
			  Display*			/* display */,
			  XEvent*			/* event */,
			  char*				/* arg */
			  ),
	char *arg)
{
	register _XQEvent *prev, *qelt;
	unsigned long qe_serial = 0;

	LockDisplay(dpy);
#ifdef XTHREADS
	dpy->ifevent_thread = xthread_self();
#endif
	dpy->in_ifevent++;
	prev = NULL;
	while (1) {
	    for (qelt = prev ? prev->next : dpy->head;
		 qelt;
		 prev = qelt, qelt = qelt->next) {
		if(qelt->qserial_num > qe_serial
		   && (*predicate)(dpy, &qelt->event, arg)) {
		    XEvent copy;
		    *event = qelt->event;
		    if (_XCopyEventCookie(dpy, &event->xcookie, &copy.xcookie)) {
			_XStoreEventCookie(dpy, &copy);
			*event = copy;
		    }
		    dpy->in_ifevent--;
		    UnlockDisplay(dpy);
		    return 0;
		}
	    }
	    if (prev)
		qe_serial = prev->qserial_num;
	    _XReadEvents(dpy);
	    if (prev && prev->qserial_num != qe_serial)
		/* another thread has snatched this event */
		prev = NULL;
	}
}

