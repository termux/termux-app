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

typedef struct _WAttrsState {
    uint64_t attr_seq;
    uint64_t geom_seq;
    XWindowAttributes *attr;
} _XWAttrsState;

static Bool
_XWAttrsHandler(
    register Display *dpy,
    register xReply *rep,
    char *buf,
    int len,
    XPointer data)
{
    register _XWAttrsState *state;
    xGetWindowAttributesReply replbuf;
    register xGetWindowAttributesReply *repl;
    register XWindowAttributes *attr;
    uint64_t last_request_read = X_DPY_GET_LAST_REQUEST_READ(dpy);

    state = (_XWAttrsState *)data;
    if (last_request_read != state->attr_seq) {
	if (last_request_read == state->geom_seq &&
	    !state->attr &&
	    rep->generic.type == X_Error &&
	    rep->error.errorCode == BadDrawable)
	    return True;
	return False;
    }
    if (rep->generic.type == X_Error) {
	state->attr = (XWindowAttributes *)NULL;
	return False;
    }
    repl = (xGetWindowAttributesReply *)
	_XGetAsyncReply(dpy, (char *)&replbuf, rep, buf, len,
		     (SIZEOF(xGetWindowAttributesReply) - SIZEOF(xReply)) >> 2,
			True);
    attr = state->attr;
    attr->class = repl->class;
    attr->bit_gravity = repl->bitGravity;
    attr->win_gravity = repl->winGravity;
    attr->backing_store = repl->backingStore;
    attr->backing_planes = repl->backingBitPlanes;
    attr->backing_pixel = repl->backingPixel;
    attr->save_under = repl->saveUnder;
    attr->colormap = repl->colormap;
    attr->map_installed = repl->mapInstalled;
    attr->map_state = repl->mapState;
    attr->all_event_masks = repl->allEventMasks;
    attr->your_event_mask = repl->yourEventMask;
    attr->do_not_propagate_mask = repl->doNotPropagateMask;
    attr->override_redirect = repl->override;
    attr->visual = _XVIDtoVisual (dpy, repl->visualID);
    return True;
}

Status
_XGetWindowAttributes(
    Display *dpy,
    Window w,
    XWindowAttributes *attr)
{
    xGetGeometryReply rep;
    register xResourceReq *req;
    register int i;
    register Screen *sp;
    _XAsyncHandler async;
    _XWAttrsState async_state;

    GetResReq(GetWindowAttributes, w, req);

    async_state.attr_seq = X_DPY_GET_REQUEST(dpy);
    async_state.geom_seq = 0;
    async_state.attr = attr;
    async.next = dpy->async_handlers;
    async.handler = _XWAttrsHandler;
    async.data = (XPointer)&async_state;
    dpy->async_handlers = &async;

    GetResReq(GetGeometry, w, req);

    async_state.geom_seq = X_DPY_GET_REQUEST(dpy);

    if (!_XReply (dpy, (xReply *)&rep, 0, xTrue)) {
	DeqAsyncHandler(dpy, &async);
	return (0);
	}
    DeqAsyncHandler(dpy, &async);
    if (!async_state.attr) {
	return (0);
    }
    attr->x = cvtINT16toInt (rep.x);
    attr->y = cvtINT16toInt (rep.y);
    attr->width = rep.width;
    attr->height = rep.height;
    attr->border_width = rep.borderWidth;
    attr->depth = rep.depth;
    attr->root = rep.root;
    /* find correct screen so that applications find it easier.... */
    for (i = 0; i < dpy->nscreens; i++) {
	sp = &dpy->screens[i];
	if (sp->root == attr->root) {
	    attr->screen = sp;
	    break;
	}
    }
    return(1);
}

Status
XGetWindowAttributes(
    Display *dpy,
    Window w,
    XWindowAttributes *attr)
{
    Status ret;

    LockDisplay(dpy);
    ret = _XGetWindowAttributes(dpy, w, attr);
    UnlockDisplay(dpy);
    SyncHandle();

    return ret;
}

