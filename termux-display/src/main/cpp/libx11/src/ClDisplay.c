
/*

Copyright 1985, 1990, 1998  The Open Group

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
#include "Xxcbint.h"
#include "Xlib.h"
#include "Xlibint.h"
#include "Xintconn.h"

/*
 * XCloseDisplay - XSync the connection to the X Server, close the connection,
 * and free all associated storage.  Extension close procs should only free
 * memory and must be careful about the types of requests they generate.
 */

int
XCloseDisplay (
	register Display *dpy)
{
	register _XExtension *ext;
	register int i;

	if (!(dpy->flags & XlibDisplayClosing))
	{
	    dpy->flags |= XlibDisplayClosing;
	    for (i = 0; i < dpy->nscreens; i++) {
		    register Screen *sp = &dpy->screens[i];
		    XFreeGC (dpy, sp->default_gc);
	    }
	    if (dpy->cursor_font != None) {
		XUnloadFont (dpy, dpy->cursor_font);
	    }
	    XSync(dpy, 1);  /* throw away pending events, catch errors */
	    /* call out to any extensions interested */
	    for (ext = dpy->ext_procs; ext; ext = ext->next) {
		if (ext->close_display)
		    (*ext->close_display)(dpy, &ext->codes);
	    }
	    /* if the closes generated more protocol, sync them up */
	    if (X_DPY_GET_REQUEST(dpy) != X_DPY_GET_LAST_REQUEST_READ(dpy))
		XSync(dpy, 1);
	}
	xcb_disconnect(dpy->xcb->connection);
	_XFreeDisplayStructure (dpy);
	return 0;
}
