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
 * Copyright 1991 by the Open Software Foundation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Open Software Foundation
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Open Software
 * Foundation makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * OPEN SOFTWARE FOUNDATION DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL OPEN SOFTWARE FOUNDATIONN BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *		 M. Collins		OSF
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xlcint.h"

void
XmbDrawText(
    Display            *dpy,
    Drawable            d,
    GC                  gc,
    int                 x,
    int                 y,
    XmbTextItem        *text_items,
    int                 nitems)
{
    register XFontSet fs = NULL;
    register XmbTextItem *p = text_items;
    register int i = nitems;
    register int esc;

    /* ignore leading items with no fontset */
    while (i && !p->font_set) {
	i--;
	p++;
    }

    for (; --i >= 0; p++) {
	if (p->font_set)
	    fs = p->font_set;
	x += p->delta;
	esc = (*fs->methods->mb_draw_string) (dpy, d, fs, gc, x, y,
					      p->chars, p->nchars);
	if (!esc)
	    esc = fs->methods->mb_escapement (fs, p->chars, p->nchars);
	x += esc;
    }
}

void
XmbDrawString(
    Display            *dpy,
    Drawable            d,
    XFontSet            font_set,
    GC                  gc,
    int                 x,
    int                 y,
    _Xconst char       *text,
    int                 text_len)
{
    (void)(*font_set->methods->mb_draw_string) (dpy, d, font_set, gc, x, y,
						text, text_len);
}


void
XmbDrawImageString(
    Display            *dpy,
    Drawable            d,
    XFontSet            font_set,
    GC                  gc,
    int                 x,
    int                 y,
    _Xconst char       *text,
    int                 text_len)
{
    (*font_set->methods->mb_draw_image_string) (dpy, d, font_set, gc, x, y,
						text, text_len);
}

int
XmbTextEscapement(
    XFontSet        font_set,
    _Xconst char   *text,
    int             text_len)
{
    return (*font_set->methods->mb_escapement) (font_set, text, text_len);
}

int
XmbTextExtents(
    XFontSet        font_set,
    _Xconst char   *text,
    int             text_len,
    XRectangle     *overall_ink_extents,
    XRectangle     *overall_logical_extents)
{
    return (*font_set->methods->mb_extents) (font_set, text, text_len,
					     overall_ink_extents,
					     overall_logical_extents);
}

Status
XmbTextPerCharExtents(
    XFontSet        font_set,
    _Xconst char   *text,
    int             text_len,
    XRectangle     *ink_extents_buffer,
    XRectangle     *logical_extents_buffer,
    int             buffer_size,
    int            *num_chars,
    XRectangle     *max_ink_extents,
    XRectangle     *max_logical_extents)
{
    return (*font_set->methods->mb_extents_per_char)
	     (font_set, text, text_len,
	      ink_extents_buffer, logical_extents_buffer,
	      buffer_size, num_chars, max_ink_extents, max_logical_extents);
}
