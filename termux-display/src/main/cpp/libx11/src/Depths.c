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
 * XListDepths - return info from connection setup
 */
int *XListDepths (
    Display *dpy,
    int scrnum,
    int *countp)
{
    Screen *scr;
    int count;
    int *depths;

    if (scrnum < 0 || scrnum >= dpy->nscreens) return NULL;

    scr = &dpy->screens[scrnum];
    if ((count = scr->ndepths) > 0) {
	register Depth *dp;
	register int i;

	depths = Xmallocarray (count, sizeof(int));
	if (!depths) return NULL;
	for (i = 0, dp = scr->depths; i < count; i++, dp++)
	  depths[i] = dp->depth;
    } else {
	/* a screen must have a depth */
	return NULL;
    }
    *countp = count;
    return depths;
}
