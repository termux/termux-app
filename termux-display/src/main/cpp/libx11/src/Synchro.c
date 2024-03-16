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


static int _XSyncFunction(register Display *dpy)
{
	XSync(dpy,0);
	return 0;
}

int (*XSynchronize(Display *dpy, int onoff))(Display *)
{
        int (*temp)(Display *);
	int (*func)(Display *) = NULL;

	if (onoff)
	    func = _XSyncFunction;

	LockDisplay(dpy);
	if (dpy->flags & XlibDisplayPrivSync) {
	    temp = dpy->savedsynchandler;
	    dpy->savedsynchandler = func;
	} else {
	    temp = dpy->synchandler;
	    dpy->synchandler = func;
	}
	UnlockDisplay(dpy);
	return (temp);
}

int (*XSetAfterFunction(
     Display *dpy,
     int (*func)(
		 Display*
		 )
        ))(Display *)
{
        int (*temp)(Display *);

	LockDisplay(dpy);
	if (dpy->flags & XlibDisplayPrivSync) {
	    temp = dpy->savedsynchandler;
	    dpy->savedsynchandler = func;
	} else {
	    temp = dpy->synchandler;
	    dpy->synchandler = func;
	}
	UnlockDisplay(dpy);
	return (temp);
}

