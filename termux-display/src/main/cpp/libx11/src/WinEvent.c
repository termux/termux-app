/*

Copyright 1985, 1998  The Open Group

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

extern long const _Xevent_to_mask[];
#define AllPointers (PointerMotionMask|PointerMotionHintMask|ButtonMotionMask)
#define AllButtons (Button1MotionMask|Button2MotionMask|Button3MotionMask|\
		    Button4MotionMask|Button5MotionMask)

/*
 * Return the next event in the queue
 * for the given window matching one of the events in the mask.
 * Events earlier in the queue are not discarded.
 * If none found, flush, and then wait until an event arrives which
 * matches.
 */

int
XWindowEvent (
        register Display *dpy,
	Window w,		/* Selected window. */
	long mask,		/* Selected event mask. */
	register XEvent *event)	/* XEvent to be filled in. */
{
	register _XQEvent *prev, *qelt;
	unsigned long qe_serial = 0;

        LockDisplay(dpy);

	/* Delete unclaimed cookies */
	_XFreeEventCookies(dpy);

	prev = NULL;
	while (1) {
	    for (qelt = prev ? prev->next : dpy->head;
		 qelt;
		 prev = qelt, qelt = qelt->next) {
		if ((qelt->event.xany.window == w) &&
		    (qelt->event.type < GenericEvent) &&
		    (_Xevent_to_mask[qelt->event.type] & mask) &&
		    ((qelt->event.type != MotionNotify) ||
		     (mask & AllPointers) ||
		     (mask & AllButtons & qelt->event.xmotion.state))) {
		    *event = qelt->event;
		    _XDeq(dpy, prev, qelt);
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
