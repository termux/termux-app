
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
#include <X11/Xlibint.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include "Xatomtype.h"
#include <X11/Xatom.h>
#include <stdio.h>

Status XGetSizeHints (
	Display *dpy,
	Window w,
	XSizeHints *hints,
        Atom property)
{
	xPropSizeHints *prop = NULL;
        Atom actual_type;
        int actual_format;
        unsigned long leftover;
        unsigned long nitems;
	if (XGetWindowProperty(dpy, w, property, 0L,
			       (long) OldNumPropSizeElements,
	    False, XA_WM_SIZE_HINTS, &actual_type, &actual_format,
            &nitems, &leftover, (unsigned char **)&prop)
            != Success) return (0);

        if ((actual_type != XA_WM_SIZE_HINTS) ||
	    (nitems < OldNumPropSizeElements) || (actual_format != 32)) {
		Xfree (prop);
                return(0);
		}
	hints->flags	  = (prop->flags & (USPosition|USSize|PAllHints));
	hints->x 	  = cvtINT32toInt (prop->x);
	hints->y 	  = cvtINT32toInt (prop->y);
	hints->width      = cvtINT32toInt (prop->width);
	hints->height     = cvtINT32toInt (prop->height);
	hints->min_width  = cvtINT32toInt (prop->minWidth);
	hints->min_height = cvtINT32toInt (prop->minHeight);
	hints->max_width  = cvtINT32toInt (prop->maxWidth);
	hints->max_height = cvtINT32toInt (prop->maxHeight);
	hints->width_inc  = cvtINT32toInt (prop->widthInc);
	hints->height_inc = cvtINT32toInt (prop->heightInc);
	hints->min_aspect.x = cvtINT32toInt (prop->minAspectX);
	hints->min_aspect.y = cvtINT32toInt (prop->minAspectY);
	hints->max_aspect.x = cvtINT32toInt (prop->maxAspectX);
	hints->max_aspect.y = cvtINT32toInt (prop->maxAspectY);
	Xfree(prop);
	return(1);
}

/*
 * must return a pointer to the hint, in malloc'd memory, or routine is not
 * extensible; any use of the caller's memory would cause things to be stepped
 * on.
 */

XWMHints *XGetWMHints (
	Display *dpy,
	Window w)
{
	xPropWMHints *prop = NULL;
	register XWMHints *hints;
        Atom actual_type;
        int actual_format;
        unsigned long leftover;
        unsigned long nitems;
	if (XGetWindowProperty(dpy, w, XA_WM_HINTS,
	    0L, (long)NumPropWMHintsElements,
	    False, XA_WM_HINTS, &actual_type, &actual_format,
            &nitems, &leftover, (unsigned char **)&prop)
            != Success) return (NULL);

	/* If the property is undefined on the window, return null pointer. */
	/* pre-R3 bogusly truncated window_group, don't fail on them */

        if ((actual_type != XA_WM_HINTS) ||
	    (nitems < (NumPropWMHintsElements - 1)) || (actual_format != 32)) {
		Xfree (prop);
                return(NULL);
		}
	/* static copies not allowed in library, due to reentrancy constraint*/
	if ((hints = Xcalloc (1, sizeof(XWMHints)))) {
	    hints->flags = prop->flags;
	    hints->input = (prop->input ? True : False);
	    hints->initial_state = cvtINT32toInt (prop->initialState);
	    hints->icon_pixmap = prop->iconPixmap;
	    hints->icon_window = prop->iconWindow;
	    hints->icon_x = cvtINT32toInt (prop->iconX);
	    hints->icon_y = cvtINT32toInt (prop->iconY);
	    hints->icon_mask = prop->iconMask;
	    if (nitems >= NumPropWMHintsElements)
		hints->window_group = prop->windowGroup;
	    else
		hints->window_group = 0;
	}
	Xfree (prop);
	return(hints);
}

Status
XGetZoomHints (
	Display *dpy,
	Window w,
	XSizeHints *zhints)
{
	return (XGetSizeHints(dpy, w, zhints, XA_WM_ZOOM_HINTS));
}

Status
XGetNormalHints (
	Display *dpy,
	Window w,
	XSizeHints *hints)
{
	return (XGetSizeHints(dpy, w, hints, XA_WM_NORMAL_HINTS));
}


/*
 * XGetIconSizes reads the property
 *	ICONSIZE_ATOM	type: ICONSIZE_ATOM format: 32
 */

Status XGetIconSizes (
	Display *dpy,
	Window w,	/* typically, root */
	XIconSize **size_list,	/* RETURN */
	int *count) 		/* RETURN number of items on the list */
{
	xPropIconSize *prop = NULL;
	register xPropIconSize *pp;
	register XIconSize *hp, *hints;
        Atom actual_type;
        int actual_format;
        unsigned long leftover;
        unsigned long nitems;
	register int i;

	if (XGetWindowProperty(dpy, w, XA_WM_ICON_SIZE, 0L, 60L,
	    False, XA_WM_ICON_SIZE, &actual_type, &actual_format,
            &nitems, &leftover, (unsigned char **)&prop)
            != Success) return (0);

	pp = prop;

        if ((actual_type != XA_WM_ICON_SIZE) ||
	    (nitems < NumPropIconSizeElements) ||
	    (nitems % NumPropIconSizeElements != 0) ||
	    (actual_format != 32)) {
		Xfree (prop);
                return(0);
		}

	/* static copies not allowed in library, due to reentrancy constraint*/

	nitems /= NumPropIconSizeElements;
	if (! (hp = hints = Xcalloc (nitems, sizeof(XIconSize)))) {
	    Xfree (prop);
	    return 0;
	}

	/* march down array putting things into native form */
	for (i = 0; i < nitems; i++) {
	    hp->min_width  = cvtINT32toInt (pp->minWidth);
	    hp->min_height = cvtINT32toInt (pp->minHeight);
	    hp->max_width  = cvtINT32toInt (pp->maxWidth);
	    hp->max_height = cvtINT32toInt (pp->maxHeight);
	    hp->width_inc  = cvtINT32toInt (pp->widthInc);
	    hp->height_inc = cvtINT32toInt (pp->heightInc);
	    hp += 1;
	    pp += 1;
	}
	*count = nitems;
	*size_list = hints;
	Xfree (prop);
	return(1);
}


Status XGetCommand (
    Display *dpy,
    Window w,
    char ***argvp,
    int *argcp)
{
    XTextProperty tp;
    int argc;
    char **argv;

    if (!XGetTextProperty (dpy, w, &tp, XA_WM_COMMAND)) return 0;

    if (tp.encoding != XA_STRING || tp.format != 8) {
	Xfree (tp.value);
	return 0;
    }


    /*
     * ignore final <NUL> if present since UNIX WM_COMMAND is nul-terminated
     */
    if (tp.nitems && (tp.value[tp.nitems - 1] == '\0')) tp.nitems--;


    /*
     * create a string list and return if successful
     */
    if (!XTextPropertyToStringList (&tp, &argv, &argc)) {
	Xfree (tp.value);
	return (0);
    }

    Xfree (tp.value);
    *argvp = argv;
    *argcp = argc;
    return 1;
}


Status
XGetTransientForHint(
	Display *dpy,
	Window w,
	Window *propWindow)
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long leftover;
    Window *data = NULL;
    if (XGetWindowProperty(dpy, w, XA_WM_TRANSIENT_FOR, 0L, 1L, False,
        XA_WINDOW,
	&actual_type,
	&actual_format, &nitems, &leftover, (unsigned char **) &data)
	!= Success) {
	*propWindow = None;
	return (0);
	}
    if ( (actual_type == XA_WINDOW) && (actual_format == 32) &&
	 (nitems != 0) ) {
	*propWindow = *data;
	Xfree( (char *) data);
	return (1);
	}
    *propWindow = None;
    Xfree( (char *) data);
    return(0);
}

Status
XGetClassHint(
	Display *dpy,
	Window w,
	XClassHint *classhint)	/* RETURN */
{
    int len_name, len_class;

    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long leftover;
    unsigned char *data = NULL;
    if (XGetWindowProperty(dpy, w, XA_WM_CLASS, 0L, (long)BUFSIZ, False,
        XA_STRING,
	&actual_type,
	&actual_format, &nitems, &leftover, &data) != Success)
           return (0);

   if ( (actual_type == XA_STRING) && (actual_format == 8) ) {
	len_name = (int) strlen((char *) data);
	if (! (classhint->res_name = Xmalloc(len_name + 1))) {
	    Xfree(data);
	    return (0);
	}
	strcpy(classhint->res_name, (char *) data);
	if (len_name == nitems) len_name--;
	len_class = (int) strlen((char *) (data+len_name+1));
	if (! (classhint->res_class = Xmalloc(len_class + 1))) {
	    Xfree(classhint->res_name);
	    classhint->res_name = (char *) NULL;
	    Xfree(data);
	    return (0);
	}
	strcpy(classhint->res_class, (char *) (data+len_name+1));
	Xfree( (char *) data);
	return(1);
	}
    Xfree( (char *) data);
    return(0);
}
