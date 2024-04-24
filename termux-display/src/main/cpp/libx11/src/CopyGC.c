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
XCopyGC (
     register Display *dpy,
     GC srcGC,
     unsigned long mask,		/* which ones to set initially */
     GC destGC)
{
    register XGCValues *destgv = &destGC->values,
    		       *srcgv = &srcGC->values;
    register xCopyGCReq *req;
    register _XExtension *ext;

    LockDisplay(dpy);

    mask &= (1L << (GCLastBit + 1)) - 1;
    /* if some of the source values to be copied are "dirty", flush them
       out before sending the CopyGC request. */
    if (srcGC->dirty & mask)
         _XFlushGCCache(dpy, srcGC);

    /* mark the copied values "not dirty" in the destination. */
    destGC->dirty &= ~mask;

    GetReq(CopyGC, req);
    req->srcGC = srcGC->gid;
    req->dstGC = destGC->gid;
    req->mask = mask;

    if (mask & GCFunction)
    	destgv->function = srcgv->function;

    if (mask & GCPlaneMask)
        destgv->plane_mask = srcgv->plane_mask;

    if (mask & GCForeground)
        destgv->foreground = srcgv->foreground;

    if (mask & GCBackground)
        destgv->background = srcgv->background;

    if (mask & GCLineWidth)
        destgv->line_width = srcgv->line_width;

    if (mask & GCLineStyle)
        destgv->line_style = srcgv->line_style;

    if (mask & GCCapStyle)
        destgv->cap_style = srcgv->cap_style;

    if (mask & GCJoinStyle)
        destgv->join_style = srcgv->join_style;

    if (mask & GCFillStyle)
    	destgv->fill_style = srcgv->fill_style;

    if (mask & GCFillRule)
        destgv->fill_rule = srcgv->fill_rule;

    if (mask & GCArcMode)
        destgv->arc_mode = srcgv->arc_mode;

    if (mask & GCTile)
        destgv->tile = srcgv->tile;

    if (mask & GCStipple)
        destgv->stipple = srcgv->stipple;

    if (mask & GCTileStipXOrigin)
        destgv->ts_x_origin = srcgv->ts_x_origin;

    if (mask & GCTileStipYOrigin)
        destgv->ts_y_origin = srcgv->ts_y_origin;

    if (mask & GCFont)
        destgv->font = srcgv->font;

    if (mask & GCSubwindowMode)
        destgv->subwindow_mode = srcgv->subwindow_mode;

    if (mask & GCGraphicsExposures)
        destgv->graphics_exposures = srcgv->graphics_exposures;

    if (mask & GCClipXOrigin)
        destgv->clip_x_origin = srcgv->clip_x_origin;

    if (mask & GCClipYOrigin)
        destgv->clip_y_origin = srcgv->clip_y_origin;

    if (mask & GCClipMask) {
	destGC->rects = srcGC->rects;
        destgv->clip_mask = srcgv->clip_mask;
	}

    if (mask & GCDashOffset)
        destgv->dash_offset = srcgv->dash_offset;

    if (mask & GCDashList) {
	destGC->dashes = srcGC->dashes;
        destgv->dashes = srcgv->dashes;
	}
    /* call out to any extensions interested */
    for (ext = dpy->ext_procs; ext; ext = ext->next)
	if (ext->copy_GC) (*ext->copy_GC)(dpy, destGC, &ext->codes);
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
    }
