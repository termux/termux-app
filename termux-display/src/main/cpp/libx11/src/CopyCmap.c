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
#else
#define XCMS 1
#endif
#include "Xlibint.h"

#if XCMS
#include "Xcmsint.h"

/* cmsCmap.c */
extern XcmsCmapRec * _XcmsCopyCmapRecAndFree(Display *dpy,
					     Colormap src_cmap,
					     Colormap copy_cmap);
#endif

Colormap XCopyColormapAndFree(
    register Display *dpy,
    Colormap src_cmap)
{
    Colormap mid;
    register xCopyColormapAndFreeReq *req;

    LockDisplay(dpy);
    GetReq(CopyColormapAndFree, req);

    mid = req->mid = XAllocID(dpy);
    req->srcCmap = src_cmap;

    /* re-lock the display to keep XID handling in sync */
    UnlockDisplay(dpy);
    SyncHandle();
    LockDisplay(dpy);

#if XCMS
    _XcmsCopyCmapRecAndFree(dpy, src_cmap, mid);
#endif

    UnlockDisplay(dpy);
    SyncHandle();

    return(mid);
}
