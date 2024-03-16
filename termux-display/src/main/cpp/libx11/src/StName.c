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
#include <limits.h>
#include <X11/Xlibint.h>
#include <X11/Xatom.h>

int
XStoreName (
    register Display *dpy,
    Window w,
    _Xconst char *name)
{
    if (name != NULL && strlen(name) >= USHRT_MAX)
        return 0;
    return XChangeProperty(dpy, w, XA_WM_NAME, XA_STRING, /*  */
			   8, PropModeReplace, (_Xconst unsigned char *)name,
			   name ? (int) strlen(name) : 0);
}

int
XSetIconName (
    register Display *dpy,
    Window w,
    _Xconst char *icon_name)
{
    if (icon_name != NULL && strlen(icon_name) >= USHRT_MAX)
        return 0;
    return XChangeProperty(dpy, w, XA_WM_ICON_NAME, XA_STRING, 8,
                           PropModeReplace, (_Xconst unsigned char *)icon_name,
			   icon_name ? (int) strlen(icon_name) : 0);
}
