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
#include "Cr.h"

static XGCValues const initial_GC = {
    GXcopy, 	/* function */
    AllPlanes,	/* plane_mask */
    0L,		/* foreground */
    1L,		/* background */
    0,		/* line_width */
    LineSolid,	/* line_style */
    CapButt,	/* cap_style */
    JoinMiter,	/* join_style */
    FillSolid,	/* fill_style */
    EvenOddRule,/* fill_rule */
    ArcPieSlice,/* arc_mode */
    (Pixmap)~0L,/* tile, impossible (unknown) resource */
    (Pixmap)~0L,/* stipple, impossible (unknown) resource */
    0,		/* ts_x_origin */
    0,		/* ts_y_origin */
    (Font)~0L,	/* font, impossible (unknown) resource */
    ClipByChildren, /* subwindow_mode */
    True,	/* graphics_exposures */
    0,		/* clip_x_origin */
    0,		/* clip_y_origin */
    None,	/* clip_mask */
    0,		/* dash_offset */
    4		/* dashes (list [4,4]) */
};

static void _XGenerateGCList(
    register Display *dpy,
    GC gc,
    xReq *req);

GC XCreateGC (
     register Display *dpy,
     Drawable d,		/* Window or Pixmap for which depth matches */
     unsigned long valuemask,	/* which ones to set initially */
     XGCValues *values)		/* the values themselves */
{
    register GC gc;
    register xCreateGCReq *req;
    register _XExtension *ext;

    LockDisplay(dpy);
    if ((gc = Xmalloc (sizeof(struct _XGC))) == NULL) {
	UnlockDisplay(dpy);
	SyncHandle();
	return (NULL);
    }
    gc->rects = 0;
    gc->dashes = 0;
    gc->ext_data = NULL;
    gc->values = initial_GC;
    gc->dirty = 0L;

    valuemask &= (1L << (GCLastBit + 1)) - 1;
    if (valuemask) _XUpdateGCCache (gc, valuemask, values);

    GetReq(CreateGC, req);
    req->drawable = d;
    req->gc = gc->gid = XAllocID(dpy);

    if ((req->mask = gc->dirty))
        _XGenerateGCList (dpy, gc, (xReq *) req);
    /* call out to any extensions interested */
    for (ext = dpy->ext_procs; ext; ext = ext->next)
	if (ext->create_GC) (*ext->create_GC)(dpy, gc, &ext->codes);
    gc->dirty = 0L; /* allow extensions to see dirty bits */
    UnlockDisplay(dpy);
    SyncHandle();
    return (gc);
    }

/*
 * GenerateGCList looks at the GC dirty bits, and appends all the required
 * long words to the request being generated.
 */

static void
_XGenerateGCList (
    register Display *dpy,
    GC gc,
    xReq *req)
    {
    unsigned long values[32];
    register unsigned long *value = values;
    long nvalues;
    register XGCValues *gv = &gc->values;
    register unsigned long dirty = gc->dirty;

    /*
     * Note: The order of these tests are critical; the order must be the
     * same as the GC mask bits in the word.
     */
    if (dirty & GCFunction)          *value++ = gv->function;
    if (dirty & GCPlaneMask)         *value++ = gv->plane_mask;
    if (dirty & GCForeground)        *value++ = gv->foreground;
    if (dirty & GCBackground)        *value++ = gv->background;
    if (dirty & GCLineWidth)         *value++ = gv->line_width;
    if (dirty & GCLineStyle)         *value++ = gv->line_style;
    if (dirty & GCCapStyle)          *value++ = gv->cap_style;
    if (dirty & GCJoinStyle)         *value++ = gv->join_style;
    if (dirty & GCFillStyle)         *value++ = gv->fill_style;
    if (dirty & GCFillRule)          *value++ = gv->fill_rule;
    if (dirty & GCTile)              *value++ = gv->tile;
    if (dirty & GCStipple)           *value++ = gv->stipple;
    if (dirty & GCTileStipXOrigin)   *value++ = gv->ts_x_origin;
    if (dirty & GCTileStipYOrigin)   *value++ = gv->ts_y_origin;
    if (dirty & GCFont)              *value++ = gv->font;
    if (dirty & GCSubwindowMode)     *value++ = gv->subwindow_mode;
    if (dirty & GCGraphicsExposures) *value++ = gv->graphics_exposures;
    if (dirty & GCClipXOrigin)       *value++ = gv->clip_x_origin;
    if (dirty & GCClipYOrigin)       *value++ = gv->clip_y_origin;
    if (dirty & GCClipMask)          *value++ = gv->clip_mask;
    if (dirty & GCDashOffset)        *value++ = gv->dash_offset;
    if (dirty & GCDashList)          *value++ = gv->dashes;
    if (dirty & GCArcMode)           *value++ = gv->arc_mode;

    req->length += (nvalues = value - values);

    /*
     * note: Data is a macro that uses its arguments multiple
     * times, so "nvalues" is changed in a separate assignment
     * statement
     */

    nvalues <<= 2;
    Data32 (dpy, (long *) values, nvalues);

    }


int
_XUpdateGCCache (
    register GC gc,
    register unsigned long mask,
    register XGCValues *attr)
{
    register XGCValues *gv = &gc->values;

    if (mask & GCFunction)
        if (gv->function != attr->function) {
	  gv->function = attr->function;
	  gc->dirty |= GCFunction;
	}

    if (mask & GCPlaneMask)
        if (gv->plane_mask != attr->plane_mask) {
            gv->plane_mask = attr->plane_mask;
	    gc->dirty |= GCPlaneMask;
	  }

    if (mask & GCForeground)
        if (gv->foreground != attr->foreground) {
            gv->foreground = attr->foreground;
	    gc->dirty |= GCForeground;
	  }

    if (mask & GCBackground)
        if (gv->background != attr->background) {
            gv->background = attr->background;
	    gc->dirty |= GCBackground;
	  }

    if (mask & GCLineWidth)
        if (gv->line_width != attr->line_width) {
            gv->line_width = attr->line_width;
	    gc->dirty |= GCLineWidth;
	  }

    if (mask & GCLineStyle)
        if (gv->line_style != attr->line_style) {
            gv->line_style = attr->line_style;
	    gc->dirty |= GCLineStyle;
	  }

    if (mask & GCCapStyle)
        if (gv->cap_style != attr->cap_style) {
            gv->cap_style = attr->cap_style;
	    gc->dirty |= GCCapStyle;
	  }

    if (mask & GCJoinStyle)
        if (gv->join_style != attr->join_style) {
            gv->join_style = attr->join_style;
	    gc->dirty |= GCJoinStyle;
	  }

    if (mask & GCFillStyle)
        if (gv->fill_style != attr->fill_style) {
            gv->fill_style = attr->fill_style;
	    gc->dirty |= GCFillStyle;
	  }

    if (mask & GCFillRule)
        if (gv->fill_rule != attr->fill_rule) {
    	    gv->fill_rule = attr->fill_rule;
	    gc->dirty |= GCFillRule;
	  }

    if (mask & GCArcMode)
        if (gv->arc_mode != attr->arc_mode) {
	    gv->arc_mode = attr->arc_mode;
	    gc->dirty |= GCArcMode;
	  }

    /* always write through tile change, since client may have changed pixmap contents */
    if (mask & GCTile) {
	    gv->tile = attr->tile;
	    gc->dirty |= GCTile;
	  }

    /* always write through stipple change, since client may have changed pixmap contents */
    if (mask & GCStipple) {
	    gv->stipple = attr->stipple;
	    gc->dirty |= GCStipple;
	  }

    if (mask & GCTileStipXOrigin)
        if (gv->ts_x_origin != attr->ts_x_origin) {
    	    gv->ts_x_origin = attr->ts_x_origin;
	    gc->dirty |= GCTileStipXOrigin;
	  }

    if (mask & GCTileStipYOrigin)
        if (gv->ts_y_origin != attr->ts_y_origin) {
	    gv->ts_y_origin = attr->ts_y_origin;
	    gc->dirty |= GCTileStipYOrigin;
	  }

    if (mask & GCFont)
        if (gv->font != attr->font) {
	    gv->font = attr->font;
	    gc->dirty |= GCFont;
	  }

    if (mask & GCSubwindowMode)
        if (gv->subwindow_mode != attr->subwindow_mode) {
	    gv->subwindow_mode = attr->subwindow_mode;
	    gc->dirty |= GCSubwindowMode;
	  }

    if (mask & GCGraphicsExposures)
        if (gv->graphics_exposures != attr->graphics_exposures) {
	    gv->graphics_exposures = attr->graphics_exposures;
	    gc->dirty |= GCGraphicsExposures;
	  }

    if (mask & GCClipXOrigin)
        if (gv->clip_x_origin != attr->clip_x_origin) {
	    gv->clip_x_origin = attr->clip_x_origin;
	    gc->dirty |= GCClipXOrigin;
	  }

    if (mask & GCClipYOrigin)
        if (gv->clip_y_origin != attr->clip_y_origin) {
	    gv->clip_y_origin = attr->clip_y_origin;
	    gc->dirty |= GCClipYOrigin;
	  }

    /* always write through mask change, since client may have changed pixmap contents */
    if (mask & GCClipMask) {
	    gv->clip_mask = attr->clip_mask;
	    gc->dirty |= GCClipMask;
	    gc->rects = 0;
	  }

    if (mask & GCDashOffset)
        if (gv->dash_offset != attr->dash_offset) {
	    gv->dash_offset = attr->dash_offset;
	    gc->dirty |= GCDashOffset;
	  }

    if (mask & GCDashList)
        if ((gv->dashes != attr->dashes) || (gc->dashes == True)) {
            gv->dashes = attr->dashes;
	    gc->dirty |= GCDashList;
	    gc->dashes = 0;
	    }
    return 0;
}

/* can only call when display is already locked. */

void _XFlushGCCache(
     Display *dpy,
     GC gc)
{
    register xChangeGCReq *req;
    register _XExtension *ext;

    if (gc->dirty) {
        GetReq(ChangeGC, req);
        req->gc = gc->gid;
	req->mask = gc->dirty;
        _XGenerateGCList (dpy, gc, (xReq *) req);
	/* call out to any extensions interested */
	for (ext = dpy->ext_procs; ext; ext = ext->next)
	    if (ext->flush_GC) (*ext->flush_GC)(dpy, gc, &ext->codes);
	gc->dirty = 0L; /* allow extensions to see dirty bits */
    }
}

void
XFlushGC(
    Display *dpy,
    GC gc)
{
    FlushGC(dpy, gc);
}

GContext XGContextFromGC(GC gc)
{
    return (gc->gid);
}
