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

static void
_XQueryColors(
    register Display *dpy,
    Colormap cmap,
    XColor *defs, 		/* RETURN */
    int ncolors)
{
    register int i;
    xQueryColorsReply rep;
    register xQueryColorsReq *req;

    GetReq(QueryColors, req);

    req->cmap = (CARD32) cmap;
    SetReqLen(req, ncolors, ncolors); /* each pixel is a CARD32 */

    for (i = 0; i < ncolors; i++)
      Data32 (dpy, (long *)&defs[i].pixel, 4L);
       /* XXX this isn't very efficient */

    if (_XReply(dpy, (xReply *) &rep, 0, xFalse) != 0) {
	xrgb *color = Xmallocarray((size_t) ncolors, sizeof(xrgb));
	if (color != NULL) {
            unsigned long nbytes = (size_t) ncolors * SIZEOF(xrgb);

	    _XRead(dpy, (char *) color, (long) nbytes);

	    for (i = 0; i < ncolors; i++) {
		register XColor *def = &defs[i];
		register xrgb *rgb = &color[i];
		def->red = rgb->red;
		def->green = rgb->green;
		def->blue = rgb->blue;
		def->flags = DoRed | DoGreen | DoBlue;
	    }
	    Xfree(color);
	}
	else
	    _XEatDataWords(dpy, rep.length);
    }
}

int
XQueryColors(
    register Display * const dpy,
    const Colormap cmap,
    XColor *defs, 		/* RETURN */
    int ncolors)
{
    int n;

    if (dpy->bigreq_size > 0)
	n = (int) (dpy->bigreq_size - (sizeof (xQueryColorsReq) >> 2) - 1);
    else
	n = (int) (dpy->max_request_size - (sizeof (xQueryColorsReq) >> 2));

    LockDisplay(dpy);
    while (ncolors >= n) {
	_XQueryColors(dpy, cmap, defs, n);
	defs += n;
	ncolors -= n;
    }
    if (ncolors > 0)
	_XQueryColors(dpy, cmap, defs, ncolors);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
