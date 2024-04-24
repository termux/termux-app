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

#define AllMaskBits (CWX|CWY|CWWidth|CWHeight|\
		     CWBorderWidth|CWSibling|CWStackMode)

int
XConfigureWindow(
    register Display *dpy,
    Window w,
    unsigned int mask,
    XWindowChanges *changes)
{
    unsigned long values[7];
    register unsigned long *value = values;
    long nvalues;
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReq(ConfigureWindow, req);
    req->window = w;
    mask &= AllMaskBits;
    req->mask = mask;

    if (mask & CWX)
	*value++ = changes->x;

    if (mask & CWY)
    	*value++ = changes->y;

    if (mask & CWWidth)
    	*value++ = changes->width;

    if (mask & CWHeight)
    	*value++ = changes->height;

    if (mask & CWBorderWidth)
    	*value++ = changes->border_width;

    if (mask & CWSibling)
	*value++ = changes->sibling;

    if (mask & CWStackMode)
        *value++ = changes->stack_mode;

    req->length += (nvalues = value - values);

    nvalues <<= 2;			/* watch out for macros... */
    Data32 (dpy, (long *) values, nvalues);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
    }
