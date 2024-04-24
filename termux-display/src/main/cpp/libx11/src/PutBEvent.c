/*

Copyright 1986, 1998  The Open Group

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

/* XPutBackEvent puts an event back at the head of the queue. */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"

int
_XPutBackEvent (
    Display *dpy,
    XEvent *event)
	{
	register _XQEvent *qelt;
	XEvent store = *event;

	if (!dpy->qfree) {
	    if ((dpy->qfree = Xmalloc (sizeof (_XQEvent))) == NULL) {
		return 0;
	    }
	    dpy->qfree->next = NULL;
	}

	/* unclaimed cookie? */
	if (_XIsEventCookie(dpy, event))
	{
	    XEvent copy = {0};
            /* if not claimed, then just fetch and store again */
	    if (!event->xcookie.data) {
		_XFetchEventCookie(dpy, &event->xcookie);
		store = *event;
	    } else { /* if claimed, copy, client must free */
		_XCopyEventCookie(dpy, &event->xcookie, &copy.xcookie);
		store = copy;
	    }
	}

	qelt = dpy->qfree;
	dpy->qfree = qelt->next;
	qelt->qserial_num = dpy->next_event_serial_num++;
	qelt->next = dpy->head;
	qelt->event = store;
	dpy->head = qelt;
	if (dpy->tail == NULL)
	    dpy->tail = qelt;
	dpy->qlen++;
	return 0;
	}

int
XPutBackEvent (
    register Display * dpy,
    register XEvent *event)
	{
	int ret;

	LockDisplay(dpy);
	ret = _XPutBackEvent(dpy, event);
	UnlockDisplay(dpy);
	return ret;
	}
