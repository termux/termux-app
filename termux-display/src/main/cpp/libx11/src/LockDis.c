
/*

Copyright 1993, 1998  The Open Group

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

/*
 * Author: Stephen Gildea, MIT X Consortium
 *
 * XLockDis.c - multi-thread application-level locking routines
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#ifdef XTHREADS
#include "locking.h"
#endif

void
XLockDisplay(
    register Display* dpy)
{
#ifdef XTHREADS
    LockDisplay(dpy);
    if (dpy->lock)
	(*dpy->lock->user_lock_display)(dpy);
    /*
     * We want the threads in the reply queue to all get out before
     * XLockDisplay returns, in case they have any side effects the
     * caller of XLockDisplay was trying to protect against.
     * XLockDisplay puts itself at the head of the event waiters queue
     * to wait for all the replies to come in.
     */
    if (dpy->lock && dpy->lock->reply_awaiters) {
	struct _XCVList *cvl;

	cvl = (*dpy->lock->create_cvl)(dpy);

	/* stuff ourselves on the head of the queue */
	cvl->next = dpy->lock->event_awaiters;
	dpy->lock->event_awaiters = cvl;

	while (dpy->lock->reply_awaiters)
	    ConditionWait(dpy, cvl->cv);
	UnlockNextEventReader(dpy); /* pass the signal on */
    }
    UnlockDisplay(dpy);
#endif
}

void
XUnlockDisplay(
    register Display* dpy)
{
#ifdef XTHREADS
    LockDisplay(dpy);
    if (dpy->lock)
	(*dpy->lock->user_unlock_display)(dpy);
    UnlockDisplay(dpy);
#endif
}
