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

Colormap *XListInstalledColormaps(
    register Display *dpy,
    Window win,
    int *n)  /* RETURN */
{
    unsigned long nbytes;
    Colormap *cmaps;
    xListInstalledColormapsReply rep;
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(ListInstalledColormaps, win, req);

    if(_XReply(dpy, (xReply *) &rep, 0, xFalse) == 0) {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    *n = 0;
	    return((Colormap *) NULL);
	}

    if (rep.nColormaps) {
	cmaps = Xmallocarray(rep.nColormaps, sizeof(Colormap));
	if (! cmaps) {
	    _XEatDataWords(dpy, rep.length);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return((Colormap *) NULL);
	}
	nbytes = rep.nColormaps << 2;
	_XRead32 (dpy, (long *) cmaps, nbytes);
    }
    else cmaps = (Colormap *) NULL;

    *n = rep.nColormaps;
    UnlockDisplay(dpy);
    SyncHandle();
    return(cmaps);
}

