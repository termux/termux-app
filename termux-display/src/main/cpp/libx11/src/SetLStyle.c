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

int
XSetLineAttributes(
    register Display *dpy,
    GC gc,
    unsigned int linewidth, /* CARD16 */
    int linestyle,
    int capstyle,
    int joinstyle)
{
    XGCValues *gv = &gc->values;

    LockDisplay(dpy);
    if (linewidth != gv->line_width) {
	gv->line_width = linewidth;
	gc->dirty |= GCLineWidth;
    }
    if (linestyle != gv->line_style) {
	gv->line_style = linestyle;
	gc->dirty |= GCLineStyle;
    }
    if (capstyle != gv->cap_style) {
	gv->cap_style = capstyle;
	gc->dirty |= GCCapStyle;
    }
    if (joinstyle != gv->join_style) {
	gv->join_style = joinstyle;
	gc->dirty |= GCJoinStyle;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}
