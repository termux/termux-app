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
 * Return the next event in the queue,
 * BUT do not remove it from the queue.
 * If none found, flush and wait until there is an event to peek.
 */

int
XPeekEvent (
	register Display *dpy,
	register XEvent *event)
{
	XEvent copy;
	LockDisplay(dpy);
	if (dpy->head == NULL)
	    _XReadEvents(dpy);
	*event = (dpy->head)->event;
	if (_XCopyEventCookie(dpy, &event->xcookie, &copy.xcookie)) {
	    _XStoreEventCookie(dpy, &copy);
	    *event = copy;
	}
	UnlockDisplay(dpy);
	return 1;
}

