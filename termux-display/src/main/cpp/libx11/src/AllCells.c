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

Status XAllocColorCells(
    register Display *dpy,
    Colormap cmap,
    Bool contig,
    unsigned long *masks, /* LISTofCARD32 */ /* RETURN */
    unsigned int nplanes, /* CARD16 */
    unsigned long *pixels, /* LISTofCARD32 */ /* RETURN */
    unsigned int ncolors) /* CARD16 */
{

    Status status;
    xAllocColorCellsReply rep;
    register xAllocColorCellsReq *req;
    LockDisplay(dpy);
    GetReq(AllocColorCells, req);

    req->cmap = cmap;
    req->colors = ncolors;
    req->planes = nplanes;
    req->contiguous = contig;

    status = _XReply(dpy, (xReply *)&rep, 0, xFalse);

    if (status) {
	if ((rep.nPixels > ncolors) || (rep.nMasks > nplanes)) {
	    _XEatDataWords(dpy, rep.length);
	    status = 0; /* Failure */
	} else {
	    _XRead32 (dpy, (long *) pixels, 4L * (long) (rep.nPixels));
	    _XRead32 (dpy, (long *) masks, 4L * (long) (rep.nMasks));
	}
    }

    UnlockDisplay(dpy);
    SyncHandle();
    return(status);
}
