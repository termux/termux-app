/*

Copyright 1987, 1988, 1993, 1998  The Open Group

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

/***********************************************************
Copyright 1988 by Wyse Technology, Inc., San Jose, Ca.,
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Wyse not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

WYSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
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
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xlocale.h>


/*
 * XSetWMProperties sets the following properties:
 *	WM_NAME		  type: TEXT		format: varies?
 *	WM_ICON_NAME	  type: TEXT		format: varies?
 *	WM_HINTS	  type: WM_HINTS	format: 32
 *	WM_COMMAND	  type: TEXT		format: varies?
 *	WM_CLIENT_MACHINE type: TEXT		format: varies?
 *	WM_NORMAL_HINTS	  type: WM_SIZE_HINTS 	format: 32
 *	WM_CLASS	  type: STRING/STRING	format: 8
  *	WM_LOCALE_NAME	  type: STRING		format: 8
 */

void XSetWMProperties (
     Display *dpy,
     Window w,			/* window to decorate */
     XTextProperty *windowName,	/* name of application */
     XTextProperty *iconName,	/* name string for icon */
     char **argv,		/* command line */
     int argc,			/* size of command line */
     XSizeHints *sizeHints,	/* size hints for window in its normal state */
     XWMHints *wmHints,		/* miscellaneous window manager hints */
     XClassHint *classHints)	/* resource name and class */
{
    XTextProperty textprop;
    char hostName[256];
    int len = _XGetHostname (hostName, sizeof hostName);
    char *locale;

    /* set names of window and icon */
    if (windowName) XSetWMName (dpy, w, windowName);
    if (iconName) XSetWMIconName (dpy, w, iconName);

    /* set the command if given */
    if (argv) {
	/*
	 * for UNIX and other operating systems which use nul-terminated
	 * arrays of STRINGs.
	 */
	XSetCommand (dpy, w, argv, argc);
    }

    /* set the name of the machine on which this application is running */
    textprop.value = (unsigned char *) hostName;
    textprop.encoding = XA_STRING;
    textprop.format = 8;
    textprop.nitems = (unsigned long) len;
    XSetWMClientMachine (dpy, w, &textprop);

    /* set hints about how geometry and window manager interaction */
    if (sizeHints) XSetWMNormalHints (dpy, w, sizeHints);
    if (wmHints) XSetWMHints (dpy, w, wmHints);
    if (classHints) {
	XClassHint tmp;

	if (!classHints->res_name) {
	    tmp.res_name = getenv ("RESOURCE_NAME");
	    if (!tmp.res_name && argv && argv[0]) {
		/*
		 * UNIX uses /dir/subdir/.../basename; other operating
		 * systems will have to change this.
		 */
		char *cp = strrchr (argv[0], '/');
#ifdef __UNIXOS2__
		char *os2_cp = strrchr (argv[0],'\\');
		char *dot_cp = strrchr (argv[0],'.');
		if (os2_cp && (os2_cp > cp)) {
		    if(dot_cp && (dot_cp > os2_cp)) *dot_cp = '\0';
		    cp=os2_cp;
		}
#endif
		tmp.res_name = (cp ? cp + 1 : argv[0]);
	    }
	    tmp.res_class = classHints->res_class;
	    classHints = &tmp;
	}
	XSetClassHint (dpy, w, classHints);
    }

    locale = setlocale(LC_CTYPE, (char *)NULL);
    if (locale)
        XChangeProperty (dpy, w, XInternAtom(dpy, "WM_LOCALE_NAME", False),
        XA_STRING, 8, PropModeReplace,
        (unsigned char *)locale, (int) strlen(locale));
}

