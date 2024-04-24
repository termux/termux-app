/*

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

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
static XColor _Xconst foreground = { 0,    0,     0,     0  };  /* black */
static XColor _Xconst background = { 0, 65535, 65535, 65535 };  /* white */

Cursor XCreateFontCursor(
	Display *dpy,
	unsigned int which)
{
	/*
	 * the cursor font contains the shape glyph followed by the mask
	 * glyph; so character position 0 contains a shape, 1 the mask for 0,
	 * 2 a shape, etc.  <X11/cursorfont.h> contains hash define names
	 * for all of these.
	 */

	if (dpy->cursor_font == None) {
	    dpy->cursor_font = XLoadFont (dpy, CURSORFONT);
	    if (dpy->cursor_font == None) return None;
	}

	return XCreateGlyphCursor (dpy, dpy->cursor_font, dpy->cursor_font,
				   which, which + 1, &foreground, &background);
}

