/*
 * Copyright 1992, 1993 by TOSHIBA Corp.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. TOSHIBA make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * TOSHIBA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *			   	mopi@osa.ilab.toshiba.co.jp
 */
/*
 * Copyright 1995 by FUJITSU LIMITED
 * This is source code modified by FUJITSU LIMITED under the Joint
 * Development Agreement for the CDE/Motif PST.
 */
/*
 * Modifiers: Jeff Walls, Paul Anderson (HEWLETT-PACKARD)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "XomGeneric.h"
#include <stdio.h>

/* For VW/UDC */

static int
is_rotate(
    XOC		oc,
    XFontStruct	*font)
{
    XOCGenericPart	*gen = XOC_GENERIC(oc);
    FontSet		font_set;
    VRotate		vrotate;
    int			font_set_count;
    int			vrotate_num;

    font_set = gen->font_set;
    font_set_count = gen->font_set_num;
    for( ; font_set_count-- ; font_set++) {
	if((font_set->vrotate_num > 0) && (font_set->vrotate)) {
	    vrotate = font_set->vrotate;
	    vrotate_num = font_set->vrotate_num;
	    for( ; vrotate_num-- ; vrotate++)
		if(vrotate->font == font)
		    return True;
	}
    }
    return False;
}

static int
is_codemap(
    XOC		oc,
    XFontStruct	*font)
{
    XOCGenericPart	*gen = XOC_GENERIC(oc);
    FontSet		font_set;
    FontData		vmap;
    int			font_set_count;
    int			vmap_num;

    font_set = gen->font_set;
    font_set_count = gen->font_set_num;
    for( ; font_set_count-- ; font_set++) {
	if(font_set->vmap_num > 0) {
	    vmap = font_set->vmap;
	    vmap_num = font_set->vmap_num;
	    for( ; vmap_num-- ; vmap++)
		if(vmap->font == font)
		    return True;
	}
    }
    return False;
}

static int
draw_vertical(
    Display	*dpy,
    Drawable	d,
    XOC		oc,
    GC		gc,
    XFontStruct	*font,
    Bool	is_xchar2b,
    int		x, int y,
    XPointer	text,
    int		length)
{
    XChar2b	*buf2b;
    char	*buf;
    int		wx = 0, wy = 0;
    int		direction = 0;
    int		font_ascent_return = 0, font_descent_return = 0;
    int		i;
    XCharStruct	overall;

    wy = y;
    if (is_xchar2b) {
	for(i = 0, buf2b = (XChar2b *) text ; i < length ; i++, buf2b++) {
	    if(is_rotate(oc, font) == True) {
		XTextExtents16(font, buf2b, 1,
			       &direction, &font_ascent_return,
			       &font_descent_return, &overall);
		wx = x - (int)((overall.rbearing - overall.lbearing) >> 1) -
			 (int) overall.lbearing;
		wy += overall.ascent;
		XDrawString16(dpy, d, gc, wx, wy, buf2b, 1);
		wy += overall.descent;
	    } else {
		wx = x - (int)((font->max_bounds.rbearing -
				font->min_bounds.lbearing) >> 1) -
			 (int) font->min_bounds.lbearing;
		wy += font->max_bounds.ascent;
		XDrawString16(dpy, d, gc, wx, wy, buf2b, 1);
		wy += font->max_bounds.descent;
	    }
	}
    } else {
	for(i = 0, buf = (char *)text ; i < length && *buf ; i++, buf++) {
	    if(is_rotate(oc, font) == True) {
		XTextExtents(font, buf, 1,
			     &direction, &font_ascent_return,
			     &font_descent_return, &overall);
		wx = x - (int)((overall.rbearing - overall.lbearing) >> 1) -
			 (int) overall.lbearing;
		wy += overall.ascent;
		XDrawString(dpy, d, gc, wx, wy, buf, 1);
		wy += overall.descent;
	    } else {
		wx = x - (int)((font->max_bounds.rbearing -
				font->min_bounds.lbearing) >> 1) -
			 (int) font->min_bounds.lbearing;
		wy += font->max_bounds.ascent;
		XDrawString(dpy, d, gc, wx, wy, buf, 1);
		wy += font->max_bounds.descent;
	    }
	}
    }
    return wy;
}

#define VMAP          0
#define VROTATE       1
#define FONTSCOPE     2

static int
DrawStringWithFontSet(
    Display *dpy,
    Drawable d,
    XOC oc,
    FontSet fs,
    GC gc,
    int x, int y,
    XPointer text,
    int length)
{
    XFontStruct *font;
    Bool is_xchar2b;
    unsigned char *ptr;
    int ptr_len, char_len = 0;
    FontData fd;
    int ret = 0;

    ptr = (unsigned char *)text;
    is_xchar2b = fs->is_xchar2b;

    while (length > 0) {
        fd = _XomGetFontDataFromFontSet(fs,
			ptr,length,&ptr_len,is_xchar2b,FONTSCOPE);
	if(ptr_len <= 0)
	    break;

       /* First, see if the "Best Match" font for the FontSet was set.
	* If it was, use that font.  If it was not set, then use the
	* font defined by font_set->font_data[0] (which is what
	* _XomGetFontDataFromFontSet() always seems to return for
	* non-VW text).  Note that given the new algorithm in
	* parse_fontname() and parse_fontdata(), fs->font will
	* *always* contain good data.   We should probably remove
	* the check for "fd->font", but we won't :-) -- jjw/pma (HP)
	*/
        if((font = fs->font) == (XFontStruct *) NULL){

	    if(fd == (FontData) NULL ||
	       (font = fd->font) == (XFontStruct *) NULL)
		break;
        }

	switch(oc->core.orientation) {
	  case XOMOrientation_LTR_TTB:
	  case XOMOrientation_RTL_TTB:
            XSetFont(dpy, gc, font->fid);

	    if (is_xchar2b) {
		char_len = ptr_len / sizeof(XChar2b);
		XDrawString16(dpy, d, gc, x, y, (XChar2b *)ptr, char_len);
		x += XTextWidth16(font, (XChar2b *)ptr, char_len);
            } else {
		char_len = ptr_len;
		XDrawString(dpy, d, gc, x, y, (char *)ptr, char_len);
		x += XTextWidth(font, (char *)ptr, char_len);
	    }
	    break;
	  case XOMOrientation_TTB_RTL:
	  case XOMOrientation_TTB_LTR:
	    if(fs->font == font) {
		fd = _XomGetFontDataFromFontSet(fs,
			ptr,length,&ptr_len,is_xchar2b,VMAP);
		if(ptr_len <= 0)
		    break;
		if(fd == (FontData) NULL ||
		   (font = fd->font) == (XFontStruct *) NULL)
		    break;

		if(is_codemap(oc, fd->font) == False) {
		    fd = _XomGetFontDataFromFontSet(fs,
			     ptr,length,&ptr_len,is_xchar2b,VROTATE);
		    if(ptr_len <= 0)
			break;
		    if(fd == (FontData) NULL ||
		       (font = fd->font) == (XFontStruct *) NULL)
			break;
		}
	    }

	    if(is_xchar2b)
		char_len = ptr_len / sizeof(XChar2b);
	    else
		char_len = ptr_len;
            XSetFont(dpy, gc, font->fid);
	    y = draw_vertical(dpy, d, oc, gc, font, is_xchar2b, x, y,
			       (char *)ptr, char_len);
	    break;

	  case XOMOrientation_Context:
	    /* never used? */
	    break;
	}

	if(char_len <= 0)
	    break;

        length -= char_len;
        ptr += ptr_len;
    }

    switch(oc->core.orientation) {
      case XOMOrientation_LTR_TTB:
      case XOMOrientation_RTL_TTB:
	ret = x;
	break;
      case XOMOrientation_TTB_RTL:
      case XOMOrientation_TTB_LTR:
	ret = y;
	break;
      case XOMOrientation_Context:
	/* not used? */
	break;
    }
    return ret;
}

/* For VW/UDC */

int
_XomGenericDrawString(
    Display *dpy,
    Drawable d,
    XOC oc,
    GC gc,
    int x, int y,
    XOMTextType type,
    XPointer text,
    int length)
{
    XlcConv conv;
    XFontStruct *font;
    Bool is_xchar2b;
/* VW/UDC */
    XPointer args[3];
    FontSet fs;
/* VW/UDC */
    XChar2b xchar2b_buf[BUFSIZ], *buf;
    int start_x = x;
    int start_y = y;
    int left = 0, buf_len = 0;
    int next = 0;

    conv = _XomInitConverter(oc, type);
    if (conv == NULL)
	return -1;

    args[0] = (XPointer) &font;
    args[1] = (XPointer) &is_xchar2b;
    args[2] = (XPointer) &fs;

    while (length > 0) {
	buf = xchar2b_buf;
	left = buf_len = BUFSIZ;

	if (_XomConvert(oc, conv, (XPointer *) &text, &length,
			(XPointer *) &buf, &left, args, 3) < 0)
	    break;
	buf_len -= left;

/* For VW/UDC */
	next = DrawStringWithFontSet(dpy, d, oc, fs, gc, x, y,
				     (XPointer)xchar2b_buf, buf_len);

	switch(oc->core.orientation) {
	  case XOMOrientation_LTR_TTB:
	  case XOMOrientation_RTL_TTB:
	    x = next;
	    break;
	  case XOMOrientation_TTB_RTL:
	  case XOMOrientation_TTB_LTR:
	    y = next;
	    break;
          case XOMOrientation_Context:
	    /* not used */
	    break;
	}
/* For VW/UDC */
    }

    x -= start_x;
    y -= start_y;

    return x;
}

int
_XmbGenericDrawString(Display *dpy, Drawable d, XOC oc, GC gc, int x, int y,
		      _Xconst char *text, int length)
{
    return _XomGenericDrawString(dpy, d, oc, gc, x, y, XOMMultiByte,
				 (XPointer) text, length);
}

int
_XwcGenericDrawString(Display *dpy, Drawable d, XOC oc, GC gc, int x, int y,
		      _Xconst wchar_t *text, int length)
{
    return _XomGenericDrawString(dpy, d, oc, gc, x, y, XOMWideChar,
				 (XPointer) text, length);
}

int
_Xutf8GenericDrawString(Display *dpy, Drawable d, XOC oc, GC gc, int x, int y,
			_Xconst char *text, int length)
{
    return _XomGenericDrawString(dpy, d, oc, gc, x, y, XOMUtf8String,
				 (XPointer) text, length);
}
