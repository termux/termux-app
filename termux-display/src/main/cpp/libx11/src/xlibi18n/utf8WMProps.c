/*

Copyright 1991, 1998  The Open Group

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
/*
 * Copyright 2000 by Bruno Haible
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of Bruno Haible not
 * be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Bruno Haible
 * makes no representations about the suitability of this software for
 * any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * Bruno Haible DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL Bruno Haible BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>

void
Xutf8SetWMProperties (
    Display *dpy,
    Window w,
    _Xconst char *windowName,
    _Xconst char *iconName,
    char **argv,
    int argc,
    XSizeHints *sizeHints,
    XWMHints *wmHints,
    XClassHint *classHints)
{
    XTextProperty wname, iname;
    XTextProperty *wprop = NULL;
    XTextProperty *iprop = NULL;

    if (windowName &&
	Xutf8TextListToTextProperty(dpy, (char**)&windowName, 1,
				    XStdICCTextStyle, &wname) >= Success)
	wprop = &wname;
    if (iconName &&
	Xutf8TextListToTextProperty(dpy, (char**)&iconName, 1,
				    XStdICCTextStyle, &iname) >= Success)
	iprop = &iname;
    XSetWMProperties(dpy, w, wprop, iprop, argv, argc,
		     sizeHints, wmHints, classHints);
    if (wprop)
	Xfree(wname.value);
    if (iprop)
	Xfree(iname.value);

    /* Note: The WM_LOCALE_NAME property is set by XSetWMProperties. */
}
