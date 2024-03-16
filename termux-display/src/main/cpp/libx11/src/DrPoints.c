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

int
XDrawPoints(
    register Display *dpy,
    Drawable d,
    GC gc,
    XPoint *points,
    int n_points,
    int mode) /* CoordMode */
{
    register xPolyPointReq *req;
    register long nbytes;
    int n;
    int xoff, yoff;
    XPoint pt;

    xoff = yoff = 0;
    LockDisplay(dpy);
    FlushGC(dpy, gc);
    while (n_points) {
	GetReq(PolyPoint, req);
	req->drawable = (CARD32) d;
	req->gc = (CARD32) gc->gid;
	req->coordMode = (BYTE) mode;
	n = n_points;
	if (!dpy->bigreq_size && n > (dpy->max_request_size - req->length))
	    n = (int) (dpy->max_request_size - req->length);
	SetReqLen(req, n, n);
	nbytes = ((long)n) << 2; /* watch out for macros... */
	if (xoff || yoff) {
	    pt.x = xoff + points->x;
	    pt.y = yoff + points->y;
	    Data16 (dpy, (short *) &pt, 4);
	    if (nbytes > 4) {
		Data16 (dpy, (short *) (points + 1), nbytes - 4);
	    }
	} else {
	    Data16 (dpy, (short *) points, nbytes);
	}
	n_points -= n;
	if (n_points && (mode == CoordModePrevious)) {
	    register XPoint *pptr = points;
	    points += n;
	    while (pptr != points) {
		xoff += pptr->x;
		yoff += pptr->y;
		pptr++;
	    }
	} else
	    points += n;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
