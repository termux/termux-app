
/*

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include "Xatomtype.h"
#include "reallocarray.h"
#include <X11/Xatom.h>

Status XGetRGBColormaps (
    Display *dpy,
    Window w,
    XStandardColormap **stdcmap,	/* RETURN */
    int *count,				/* RETURN */
    Atom property)			/* XA_RGB_BEST_MAP, etc. */
{
    register int i;			/* iterator variable */
    xPropStandardColormap *data = NULL;	 /* data read in from prop */
    Atom actual_type;			/* how the prop was actually stored */
    int actual_format;			/* ditto */
    unsigned long leftover;		/* how much was left over */
    unsigned long nitems;		/* number of 32bits read */
    int ncmaps;				/* number of structs this makes */
    Bool old_style = False;		/* if was too short */
    VisualID def_visual = None;		/* visual to use if prop too short */
    XStandardColormap *cmaps;		/* return value */


    if (XGetWindowProperty (dpy, w, property, 0L, 1000000L, False,
			    XA_RGB_COLOR_MAP, &actual_type, &actual_format,
			    &nitems, &leftover, (unsigned char **)&data)
	!= Success)
      return False;

    /* if wrong type or format, or too small for us, then punt */
    if ((actual_type != XA_RGB_COLOR_MAP) || (actual_format != 32) ||
	(nitems < OldNumPropStandardColormapElements)) {
	Xfree (data);
	return False;
    }

    /*
     * See how many properties were found; if pre-ICCCM then assume
     * default visual and a kill id of 1.
     */
    if (nitems < NumPropStandardColormapElements) {
	ncmaps = 1;
	old_style = True;
	if (nitems < (NumPropStandardColormapElements - 1)) {
	    Screen *sp = _XScreenOfWindow (dpy, w);

	    if (!sp) {
		Xfree (data);
		return False;
	    }
	    def_visual = sp->root_visual->visualid;
	}
    } else {
	/*
	 * make sure we have an integral number of colormaps
	 */
	ncmaps = (nitems / NumPropStandardColormapElements);
	if ((((unsigned long) ncmaps) * NumPropStandardColormapElements) !=
	    nitems) {
	    Xfree (data);
	    return False;
	}
    }


    /*
     * allocate array
     */
    cmaps = Xmallocarray (ncmaps, sizeof (XStandardColormap));
    if (!cmaps) {
	Xfree (data);
	return False;
    }


    /*
     * and fill it in, handling compatibility with pre-ICCCM short stdcmaps
     */
    {
	register XStandardColormap *map;
	register xPropStandardColormap *prop;

	for (i = ncmaps, map = cmaps, prop = data; i > 0; i--, map++, prop++) {
	    map->colormap   = prop->colormap;
	    map->red_max    = prop->red_max;
	    map->red_mult   = prop->red_mult;
	    map->green_max  = prop->green_max;
	    map->green_mult = prop->green_mult;
	    map->blue_max   = prop->blue_max;
	    map->blue_mult  = prop->blue_mult;
	    map->base_pixel = prop->base_pixel;
	    map->visualid   = (def_visual ? def_visual : prop->visualid);
	    map->killid     = (old_style ? None : prop->killid);
	}
    }
    Xfree (data);
    *stdcmap = cmaps;
    *count = ncmaps;
    return True;
}

