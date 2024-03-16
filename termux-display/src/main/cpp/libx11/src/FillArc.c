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

#define size (SIZEOF(xPolyFillArcReq) + FARCSPERBATCH * SIZEOF(xArc))

int
XFillArc(
    register Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y, /* INT16 */
    unsigned int width,
    unsigned int height, /* CARD16 */
    int angle1,
    int angle2) /* INT16 */
{
    xArc *arc;

    LockDisplay(dpy);
    FlushGC(dpy, gc);

    {
    register xPolyFillArcReq *req = (xPolyFillArcReq *) dpy->last_req;

    /* if same as previous request, with same drawable, batch requests */
    if (
          (req->reqType == X_PolyFillArc)
       && (req->drawable == d)
       && (req->gc == gc->gid)
       && ((dpy->bufptr + SIZEOF(xArc)) <= dpy->bufmax)
       && (((char *)dpy->bufptr - (char *)req) < size) ) {
	 req->length += SIZEOF(xArc) >> 2;
         arc = (xArc *) dpy->bufptr;
	 dpy->bufptr += SIZEOF(xArc);
	 }

    else {
	GetReqExtra(PolyFillArc, SIZEOF(xArc), req);

	req->drawable = d;
	req->gc = gc->gid;
	arc = (xArc *) NEXTPTR(req,xPolyFillArcReq);
	}
    arc->x = x;
    arc->y = y;
    arc->width = width;
    arc->height = height;
    arc->angle1 = angle1;
    arc->angle2 = angle2;

    }
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
