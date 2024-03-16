/***********************************************************

Copyright 1987, 1998  The Open Group

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


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include "Xatomtype.h"
#include <X11/Xatom.h>

/*
 * 				    WARNING
 *
 * This is a pre-ICCCM routine.  It must not reference any of the new fields
 * in the XStandardColormap structure.
 */

Status XGetStandardColormap (
    Display *dpy,
    Window w,
    XStandardColormap *cmap,
    Atom property)		/* XA_RGB_BEST_MAP, etc. */
{
    Status stat;			/* return value */
    XStandardColormap *stdcmaps;	/* will get malloced value */
    int nstdcmaps;			/* count of above */

    stat = XGetRGBColormaps (dpy, w, &stdcmaps, &nstdcmaps, property);
    if (stat) {
	XStandardColormap *use;

	if (nstdcmaps > 1) {
	    VisualID vid;
	    Screen *sp = _XScreenOfWindow (dpy, w);
	    int i;

	    if (!sp) {
		Xfree (stdcmaps);
		return False;
	    }
	    vid = sp->root_visual->visualid;

	    for (i = 0; i < nstdcmaps; i++) {
		if (stdcmaps[i].visualid == vid) break;
	    }

	    if (i == nstdcmaps) {	/* not found */
		Xfree (stdcmaps);
		return False;
	    }
	    use = &stdcmaps[i];
	} else {
	    use = stdcmaps;
	}

	/*
	 * assign only those fields which were in the pre-ICCCM version
	 */
	cmap->colormap	 = use->colormap;
	cmap->red_max	 = use->red_max;
	cmap->red_mult	 = use->red_mult;
	cmap->green_max	 = use->green_max;
	cmap->green_mult = use->green_mult;
	cmap->blue_max	 = use->blue_max;
	cmap->blue_mult	 = use->blue_mult;
	cmap->base_pixel = use->base_pixel;

	Xfree (stdcmaps);	/* don't need allocated memory */
    }
    return stat;
}
