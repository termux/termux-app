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

/* can only call when display is locked. */
void _XSetClipRectangles (
    Display *dpy,
    GC gc,
    int clip_x_origin, int clip_y_origin,
    XRectangle *rectangles,
    int n,
    int ordering)
{
    register xSetClipRectanglesReq *req;
    register long len;
    unsigned long dirty;
    register _XExtension *ext;

    GetReq (SetClipRectangles, req);
    req->gc = (CARD32) gc->gid;
    req->xOrigin = (INT16) (gc->values.clip_x_origin = clip_x_origin);
    req->yOrigin = (INT16) (gc->values.clip_y_origin = clip_y_origin);
    req->ordering = (BYTE) ordering;
    len = ((long)n) << 1;
    SetReqLen(req, len, 1);
    len <<= 2;
    Data16 (dpy, (short *) rectangles, len);
    gc->rects = 1;
    dirty = (gc->dirty & (unsigned long) ~(GCClipMask | GCClipXOrigin | GCClipYOrigin));
    gc->dirty = GCClipMask | GCClipXOrigin | GCClipYOrigin;
    /* call out to any extensions interested */
    for (ext = dpy->ext_procs; ext; ext = ext->next)
	if (ext->flush_GC) (*ext->flush_GC)(dpy, gc, &ext->codes);
    gc->dirty = dirty;
}

int
XSetClipRectangles (
    register Display *dpy,
    GC gc,
    int clip_x_origin,
    int clip_y_origin,
    XRectangle *rectangles,
    int n,
    int ordering)
{
    LockDisplay(dpy);
    _XSetClipRectangles (dpy, gc, clip_x_origin, clip_y_origin, rectangles, n,
                    ordering);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}

