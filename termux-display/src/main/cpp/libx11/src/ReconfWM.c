/*

Copyright 1986, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"

#define AllMaskBits (CWX|CWY|CWWidth|CWHeight|\
                     CWBorderWidth|CWSibling|CWStackMode)

Status XReconfigureWMWindow (
    register Display *dpy,
    Window w,
    int screen,
    unsigned int mask,
    XWindowChanges *changes)
{
    Window root = RootWindow (dpy, screen);
    _XAsyncHandler async;
    _XAsyncErrorState async_state;

    /*
     * Only need to go through the trouble if we are actually changing the
     * stacking mode.
     */
    if (!(mask & CWStackMode)) {
	XConfigureWindow (dpy, w, mask, changes);
	return True;
    }


    /*
     * We need to inline XConfigureWindow and XSync so that everything is done
     * while the display is locked.
     */

    LockDisplay(dpy);

    /*
     * XConfigureWindow (dpy, w, mask, changes);
     */
    {
	unsigned long values[7];
	register unsigned long *value = values;
	long nvalues;
	register xConfigureWindowReq *req;

	GetReq(ConfigureWindow, req);

	async_state.min_sequence_number = dpy->request;
	async_state.max_sequence_number = dpy->request;
	async_state.error_code = BadMatch;
	async_state.major_opcode = X_ConfigureWindow;
	async_state.minor_opcode = 0;
	async_state.error_count = 0;
	async.next = dpy->async_handlers;
	async.handler = _XAsyncErrorHandler;
	async.data = (XPointer)&async_state;
	dpy->async_handlers = &async;

	req->window = w;
	mask &= AllMaskBits;
	req->mask = mask;

	if (mask & CWX) *value++ = changes->x;
	if (mask & CWY) *value++ = changes->y;
	if (mask & CWWidth) *value++ = changes->width;
	if (mask & CWHeight) *value++ = changes->height;
	if (mask & CWBorderWidth) *value++ = changes->border_width;
	if (mask & CWSibling) *value++ = changes->sibling;
	if (mask & CWStackMode) *value++ = changes->stack_mode;
	req->length += (nvalues = value - values);
	nvalues <<= 2;			/* watch out for macros... */
	Data32 (dpy, (long *) values, nvalues);
    }

    /*
     * XSync (dpy, 0)
     */
    {
	xGetInputFocusReply rep;
	_X_UNUSED register xReq *req;

	GetEmptyReq(GetInputFocus, req);
	(void) _XReply (dpy, (xReply *)&rep, 0, xTrue);
    }

    DeqAsyncHandler(dpy, &async);
    UnlockDisplay(dpy);
    SyncHandle();


    /*
     * If the request succeeded, then everything is okay; otherwise, send event
     */
    if (!async_state.error_count)
        return True;
    else {
        XConfigureRequestEvent ev = {
            .type		= ConfigureRequest,
            .window		= w,
            .parent		= root,
            .value_mask		= (mask & AllMaskBits),
            .x			= changes->x,
            .y			= changes->y,
            .width		= changes->width,
            .height		= changes->height,
            .border_width	= changes->border_width,
            .above		= changes->sibling,
            .detail		= changes->stack_mode,
        };
        return (XSendEvent (dpy, root, False,
                            SubstructureRedirectMask|SubstructureNotifyMask,
                            (XEvent *)&ev));
    }
}
