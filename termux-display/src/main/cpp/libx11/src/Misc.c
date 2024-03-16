/*

Copyright 1988, 1998  The Open Group

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

long XMaxRequestSize(Display *dpy)
{
    return dpy->max_request_size;
}

char *XResourceManagerString(Display *dpy)
{
    return dpy->xdefaults;
}

unsigned long XDisplayMotionBufferSize(Display *dpy)
{
    return dpy->motion_buffer;
}

int
XDisplayKeycodes(
    Display *dpy,
    int *min_keycode_return,
    int *max_keycode_return)
{
    *min_keycode_return = dpy->min_keycode;
    *max_keycode_return = dpy->max_keycode;
    return 1;
}

VisualID XVisualIDFromVisual(Visual *visual)
{
    return visual->visualid;
}

long XExtendedMaxRequestSize(Display *dpy)
{
    return dpy->bigreq_size;
}
