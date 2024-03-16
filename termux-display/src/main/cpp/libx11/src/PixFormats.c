/*

Copyright 1989, 1998  The Open Group

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
#include <stdio.h>

/*
 * XListPixmapFormats - return info from connection setup
 */

XPixmapFormatValues *XListPixmapFormats (
    Display *dpy,
    int *count)	/* RETURN */
{
    XPixmapFormatValues *formats =
	Xmallocarray(dpy->nformats, sizeof (XPixmapFormatValues));

    if (formats) {
	register int i;
	register XPixmapFormatValues *f;
	register ScreenFormat *sf;

	/*
	 * copy data from internal Xlib data structure in display
	 */
	for (i = dpy->nformats, f = formats, sf = dpy->pixmap_format; i > 0;
	     i--, f++, sf++) {
	    f->depth = sf->depth;
	    f->bits_per_pixel = sf->bits_per_pixel;
	    f->scanline_pad = sf->scanline_pad;
	}

	*count = dpy->nformats;
    }
    return formats;
}
