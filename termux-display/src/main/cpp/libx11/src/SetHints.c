
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
#include <limits.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include "Xatomtype.h"
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include "reallocarray.h"

#define safestrlen(s) ((s) ? strlen(s) : 0)

int
XSetSizeHints(		/* old routine */
	Display *dpy,
	Window w,
	XSizeHints *hints,
        Atom property)
{
	xPropSizeHints prop;
	memset(&prop, 0, sizeof(prop));
	prop.flags = (hints->flags & (USPosition|USSize|PAllHints));
	if (hints->flags & (USPosition|PPosition)) {
	    prop.x = hints->x;
	    prop.y = hints->y;
	}
	if (hints->flags & (USSize|PSize)) {
	    prop.width = hints->width;
	    prop.height = hints->height;
	}
	if (hints->flags & PMinSize) {
	    prop.minWidth = hints->min_width;
	    prop.minHeight = hints->min_height;
	}
	if (hints->flags & PMaxSize) {
	    prop.maxWidth  = hints->max_width;
	    prop.maxHeight = hints->max_height;
	}
	if (hints->flags & PResizeInc) {
	    prop.widthInc = hints->width_inc;
	    prop.heightInc = hints->height_inc;
	}
	if (hints->flags & PAspect) {
	    prop.minAspectX = hints->min_aspect.x;
	    prop.minAspectY = hints->min_aspect.y;
	    prop.maxAspectX = hints->max_aspect.x;
	    prop.maxAspectY = hints->max_aspect.y;
	}
	return XChangeProperty (dpy, w, property, XA_WM_SIZE_HINTS, 32,
				PropModeReplace, (unsigned char *) &prop,
				OldNumPropSizeElements);
}

/*
 * XSetWMHints sets the property
 *	WM_HINTS 	type: WM_HINTS	format:32
 */

int
XSetWMHints (
	Display *dpy,
	Window w,
	XWMHints *wmhints)
{
	xPropWMHints prop;
	memset(&prop, 0, sizeof(prop));
	prop.flags = wmhints->flags;
	if (wmhints->flags & InputHint)
	    prop.input = (wmhints->input == True ? 1 : 0);
	if (wmhints->flags & StateHint)
	    prop.initialState = wmhints->initial_state;
	if (wmhints->flags & IconPixmapHint)
	    prop.iconPixmap = wmhints->icon_pixmap;
	if (wmhints->flags & IconWindowHint)
	    prop.iconWindow = wmhints->icon_window;
	if (wmhints->flags & IconPositionHint) {
	    prop.iconX = wmhints->icon_x;
	    prop.iconY = wmhints->icon_y;
	}
	if (wmhints->flags & IconMaskHint)
	    prop.iconMask = wmhints->icon_mask;
	if (wmhints->flags & WindowGroupHint)
	    prop.windowGroup = wmhints->window_group;
	return XChangeProperty (dpy, w, XA_WM_HINTS, XA_WM_HINTS, 32,
				PropModeReplace, (unsigned char *) &prop,
				NumPropWMHintsElements);
}



/*
 * XSetZoomHints sets the property
 *	WM_ZOOM_HINTS 	type: WM_SIZE_HINTS format: 32
 */

int
XSetZoomHints (
	Display *dpy,
	Window w,
	XSizeHints *zhints)
{
	return XSetSizeHints (dpy, w, zhints, XA_WM_ZOOM_HINTS);
}


/*
 * XSetNormalHints sets the property
 *	WM_NORMAL_HINTS 	type: WM_SIZE_HINTS format: 32
 */

int
XSetNormalHints (			/* old routine */
	Display *dpy,
	Window w,
	XSizeHints *hints)
{
	return XSetSizeHints (dpy, w, hints, XA_WM_NORMAL_HINTS);
}



/*
 * Note, the following is one of the few cases were we really do want sizeof
 * when examining a protocol structure.  This is because the XChangeProperty
 * routine will take care of converting to host to network data structures.
 */

int
XSetIconSizes (
	Display *dpy,
	Window w,	/* typically, root */
	XIconSize *list,
	int count) 	/* number of items on the list */
{
	register int i;
	xPropIconSize *pp, *prop;

	if ((prop = pp = Xmallocarray (count, sizeof(xPropIconSize)))) {
	    for (i = 0; i < count; i++) {
		pp->minWidth  = list->min_width;
		pp->minHeight = list->min_height;
		pp->maxWidth  = list->max_width;
		pp->maxHeight = list->max_height;
		pp->widthInc  = list->width_inc;
		pp->heightInc = list->height_inc;
		pp += 1;
		list += 1;
	    }
	    XChangeProperty (dpy, w, XA_WM_ICON_SIZE, XA_WM_ICON_SIZE, 32,
			     PropModeReplace, (unsigned char *) prop,
			     count * NumPropIconSizeElements);
	    Xfree (prop);
	}
	return 1;
}

int
XSetCommand (
	Display *dpy,
	Window w,
	char **argv,
	int argc)
{
	register int i;
	size_t nbytes;
	register char *buf, *bp;
	for (i = 0, nbytes = 0; i < argc; i++) {
		nbytes += safestrlen(argv[i]) + 1;
		if (nbytes >= USHRT_MAX)
                    return 1;
	}
	if ((bp = buf = Xmalloc(nbytes))) {
	    /* copy arguments into single buffer */
	    for (i = 0; i < argc; i++) {
		if (argv[i]) {
		    (void) strcpy(bp, argv[i]);
		    bp += strlen(argv[i]) + 1;
		}
		else
		    *bp++ = '\0';
	    }
	    XChangeProperty (dpy, w, XA_WM_COMMAND, XA_STRING, 8,
			     PropModeReplace, (unsigned char *)buf, nbytes);
	    Xfree(buf);
	}
	return 1;
}
/*
 * XSetStandardProperties sets the following properties:
 *	WM_NAME		  type: STRING		format: 8
 *	WM_ICON_NAME	  type: STRING		format: 8
 *	WM_HINTS	  type: WM_HINTS	format: 32
 *	WM_COMMAND	  type: STRING
 *	WM_NORMAL_HINTS	  type: WM_SIZE_HINTS 	format: 32
 */

int
XSetStandardProperties (
    	Display *dpy,
    	Window w,		/* window to decorate */
    	_Xconst char *name,	/* name of application */
    	_Xconst char *icon_string,/* name string for icon */
	Pixmap icon_pixmap,	/* pixmap to use as icon, or None */
    	char **argv,		/* command to be used to restart application */
    	int argc,		/* count of arguments */
    	XSizeHints *hints)	/* size hints for window in its normal state */
{
	XWMHints phints;
	phints.flags = 0;

	if (name != NULL) XStoreName (dpy, w, name);

        if (safestrlen(icon_string) >= USHRT_MAX)
            return 1;
	if (icon_string != NULL) {
	    XChangeProperty (dpy, w, XA_WM_ICON_NAME, XA_STRING, 8,
                             PropModeReplace,
                             (_Xconst unsigned char *)icon_string,
                             (int)safestrlen(icon_string));
		}

	if (icon_pixmap != None) {
		phints.icon_pixmap = icon_pixmap;
		phints.flags |= IconPixmapHint;
		}
	if (argv != NULL) XSetCommand(dpy, w, argv, argc);

	if (hints != NULL) XSetNormalHints(dpy, w, hints);

	if (phints.flags != 0) XSetWMHints(dpy, w, &phints);

	return 1;
}

int
XSetTransientForHint(
	Display *dpy,
	Window w,
	Window propWindow)
{
	return XChangeProperty(dpy, w, XA_WM_TRANSIENT_FOR, XA_WINDOW, 32,
			       PropModeReplace, (unsigned char *) &propWindow, 1);
}

int
XSetClassHint(
	Display *dpy,
	Window w,
	XClassHint *classhint)
{
	char *class_string;
	char *s;
	size_t len_nm, len_cl;

	len_nm = safestrlen(classhint->res_name);
	len_cl = safestrlen(classhint->res_class);
        if (len_nm + len_cl >= USHRT_MAX)
            return 1;
	if ((class_string = s = Xmalloc(len_nm + len_cl + 2))) {
	    if (len_nm) {
		strcpy(s, classhint->res_name);
		s += len_nm + 1;
	    }
	    else
		*s++ = '\0';
	    if (len_cl)
		strcpy(s, classhint->res_class);
	    else
		*s = '\0';
	    XChangeProperty(dpy, w, XA_WM_CLASS, XA_STRING, 8,
			    PropModeReplace, (unsigned char *) class_string,
			    len_nm+len_cl+2);
	    Xfree(class_string);
	}
	return 1;
}
