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

/* precompute the maximum size of batching request allowed */

#define wsize (SIZEOF(xPolySegmentReq) + WLNSPERBATCH * SIZEOF(xSegment))
#define zsize (SIZEOF(xPolySegmentReq) + ZLNSPERBATCH * SIZEOF(xSegment))

int
XDrawLine (
    register Display *dpy,
    Drawable d,
    GC gc,
    int x1,
    int y1,
    int x2,
    int y2)
{
    register xSegment *segment;

    LockDisplay(dpy);
    FlushGC(dpy, gc);

    {
    register xPolySegmentReq *req = (xPolySegmentReq *) dpy->last_req;

    /* if same as previous request, with same drawable, batch requests */
    if (
          (req->reqType == X_PolySegment)
       && (req->drawable == d)
       && (req->gc == gc->gid)
       && ((dpy->bufptr + SIZEOF(xSegment)) <= dpy->bufmax)
       && (((char *)dpy->bufptr - (char *)req) < (gc->values.line_width ?
						  wsize : zsize)) ) {
	 req->length += SIZEOF(xSegment) >> 2;
         segment = (xSegment *) dpy->bufptr;
	 dpy->bufptr += SIZEOF(xSegment);
	 }

    else {
	GetReqExtra (PolySegment, SIZEOF(xSegment), req);
	req->drawable = d;
	req->gc = gc->gid;
	segment = (xSegment *) NEXTPTR(req,xPolySegmentReq);
	}

    segment->x1 = x1;
    segment->y1 = y1;
    segment->x2 = x2;
    segment->y2 = y2;

    UnlockDisplay(dpy);
    SyncHandle();
    }
    return 1;
}

