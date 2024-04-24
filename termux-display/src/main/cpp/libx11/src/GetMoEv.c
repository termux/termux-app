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
#include "reallocarray.h"
#include <limits.h>

XTimeCoord *XGetMotionEvents(
    register Display *dpy,
    Window w,
    Time start,
    Time stop,
    int *nEvents)  /* RETURN */
{
    xGetMotionEventsReply rep;
    register xGetMotionEventsReq *req;
    XTimeCoord *tc = NULL;
    LockDisplay(dpy);
    GetReq(GetMotionEvents, req);
    req->window = w;
/* XXX is this right for all machines? */
    req->start = start;
    req->stop  = stop;
    if (!_XReply (dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
        SyncHandle();
	return (NULL);
	}

    if (rep.nEvents && (rep.nEvents < (INT_MAX / sizeof(XTimeCoord))))
	tc = Xmallocarray(rep.nEvents, sizeof(XTimeCoord));
    if (tc == NULL) {
	/* server returned either no events or a bad event count */
	*nEvents = 0;
	_XEatDataWords (dpy, rep.length);
    }
    else
    {
	register XTimeCoord *tcptr;
	unsigned int i;
	xTimecoord xtc;

	*nEvents = (int) rep.nEvents;
	for (i = rep.nEvents, tcptr = tc; i > 0; i--, tcptr++) {
	    _XRead (dpy, (char *) &xtc, SIZEOF (xTimecoord));
	    tcptr->time = xtc.time;
	    tcptr->x    = cvtINT16toShort (xtc.x);
	    tcptr->y    = cvtINT16toShort (xtc.y);
	}
    }

    UnlockDisplay(dpy);
    SyncHandle();
    return (tc);
}

