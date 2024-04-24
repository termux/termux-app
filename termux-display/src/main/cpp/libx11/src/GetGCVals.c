
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

/*
 * All gc fields except GCClipMask and GCDashList
 */
#define ValidGCValuesBits (GCFunction | GCPlaneMask | GCForeground | \
			   GCBackground | GCLineWidth | GCLineStyle | \
			   GCCapStyle | GCJoinStyle | GCFillStyle | \
			   GCFillRule | GCTile | GCStipple | \
			   GCTileStipXOrigin | GCTileStipYOrigin | \
			   GCFont | GCSubwindowMode | GCGraphicsExposures | \
			   GCClipXOrigin | GCClipYOrigin | GCDashOffset | \
			   GCArcMode)

/*ARGSUSED*/
Status XGetGCValues (
    Display *dpy,
    GC gc,
    unsigned long valuemask,
    XGCValues *values)
{
    if (valuemask == ValidGCValuesBits) {
	char dashes = values->dashes;
	Pixmap clip_mask = values->clip_mask;
	*values = gc->values;
	values->dashes = dashes;
	values->clip_mask = clip_mask;
	return True;
    }

    if (valuemask & ~ValidGCValuesBits) return False;

    if (valuemask & GCFunction)
      values->function = gc->values.function;

    if (valuemask & GCPlaneMask)
      values->plane_mask = gc->values.plane_mask;

    if (valuemask & GCForeground)
      values->foreground = gc->values.foreground;

    if (valuemask & GCBackground)
      values->background = gc->values.background;

    if (valuemask & GCLineWidth)
      values->line_width = gc->values.line_width;

    if (valuemask & GCLineStyle)
      values->line_style = gc->values.line_style;

    if (valuemask & GCCapStyle)
      values->cap_style = gc->values.cap_style;

    if (valuemask & GCJoinStyle)
      values->join_style = gc->values.join_style;

    if (valuemask & GCFillStyle)
      values->fill_style = gc->values.fill_style;

    if (valuemask & GCFillRule)
      values->fill_rule = gc->values.fill_rule;

    if (valuemask & GCTile)
      values->tile = gc->values.tile;

    if (valuemask & GCStipple)
      values->stipple = gc->values.stipple;

    if (valuemask & GCTileStipXOrigin)
      values->ts_x_origin = gc->values.ts_x_origin;

    if (valuemask & GCTileStipYOrigin)
      values->ts_y_origin = gc->values.ts_y_origin;

    if (valuemask & GCFont)
      values->font = gc->values.font;

    if (valuemask & GCSubwindowMode)
      values->subwindow_mode = gc->values.subwindow_mode;

    if (valuemask & GCGraphicsExposures)
      values->graphics_exposures = gc->values.graphics_exposures;

    if (valuemask & GCClipXOrigin)
      values->clip_x_origin = gc->values.clip_x_origin;

    if (valuemask & GCClipYOrigin)
      values->clip_y_origin = gc->values.clip_y_origin;

    if (valuemask & GCDashOffset)

      values->dash_offset = gc->values.dash_offset;

    if (valuemask & GCArcMode)
      values->arc_mode = gc->values.arc_mode;

    return True;
}
