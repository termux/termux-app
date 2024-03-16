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

#define size (SIZEOF(xPolyFillRectangleReq) + FRCTSPERBATCH * SIZEOF(xRectangle))

int
XFillRectangle(
    register Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y, /* INT16 */
    unsigned int width,
    unsigned int height) /* CARD16 */
{
    xRectangle *rect;

    LockDisplay(dpy);
    FlushGC(dpy, gc);

    {
    register xPolyFillRectangleReq *req
		= (xPolyFillRectangleReq *) dpy->last_req;

    /* if same as previous request, with same drawable, batch requests */
    if (
          (req->reqType == X_PolyFillRectangle)
       && (req->drawable == d)
       && (req->gc == gc->gid)
       && ((dpy->bufptr + SIZEOF(xRectangle)) <= dpy->bufmax)
       && (((char *)dpy->bufptr - (char *)req) < size) ) {
	 req->length += SIZEOF(xRectangle) >> 2;
         rect = (xRectangle *) dpy->bufptr;
	 dpy->bufptr += SIZEOF(xRectangle);
	 }

    else {
	GetReqExtra(PolyFillRectangle, SIZEOF(xRectangle), req);
	req->drawable = d;
	req->gc = gc->gid;
	rect = (xRectangle *) NEXTPTR(req,xPolyFillRectangleReq);
	}
    rect->x = x;
    rect->y = y;
    rect->width = width;
    rect->height = height;

    }
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
