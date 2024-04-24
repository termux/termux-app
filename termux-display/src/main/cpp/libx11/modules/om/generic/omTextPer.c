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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "XomGeneric.h"
#include <stdio.h>

static Status
_XomGenericTextPerCharExtents(
    XOC oc,
    XOMTextType type,
    XPointer text,
    int length,
    XRectangle *ink_buf,
    XRectangle *logical_buf,
    int buf_size,
    int *num_chars,
    XRectangle *overall_ink,
    XRectangle *overall_logical)
{
    XlcConv conv;
    XFontStruct *font;
    Bool is_xchar2b;
    XPointer args[2];
    XChar2b xchar2b_buf[BUFSIZ], *xchar2b_ptr;
    char *xchar_ptr = NULL;
    XCharStruct *def, *cs, overall;
    int buf_len, left, require_num;
    int logical_ascent, logical_descent;
    Bool first = True;

    conv = _XomInitConverter(oc, type);
    if (conv == NULL)
	return 0;

    bzero((char *) &overall, sizeof(XCharStruct));
    logical_ascent = logical_descent = require_num = *num_chars = 0;

    args[0] = (XPointer) &font;
    args[1] = (XPointer) &is_xchar2b;

    while (length > 0) {
	xchar2b_ptr = xchar2b_buf;
	left = buf_len = BUFSIZ;

	if (_XomConvert(oc, conv, (XPointer *) &text, &length,
			(XPointer *) &xchar2b_ptr, &left, args, 2) < 0)
	    break;
	buf_len -= left;

	if (require_num) {
	    require_num += buf_len;
	    continue;
	}
	if (buf_size < buf_len) {
	    require_num = *num_chars + buf_len;
	    continue;
	}
	buf_size -= buf_len;

	if (first) {
	    logical_ascent = font->ascent;
	    logical_descent = font->descent;
	} else {
	    logical_ascent = max(logical_ascent, font->ascent);
	    logical_descent = max(logical_descent, font->descent);
	}

	if (is_xchar2b) {
	    CI_GET_DEFAULT_INFO_2D(font, def)
	    xchar2b_ptr = xchar2b_buf;
	} else {
	    CI_GET_DEFAULT_INFO_1D(font, def)
	    xchar_ptr = (char *) xchar2b_buf;
	}

	while (buf_len-- > 0) {
	    if (is_xchar2b) {
		CI_GET_CHAR_INFO_2D(font, xchar2b_ptr->byte1,
				    xchar2b_ptr->byte2, def, cs)
		xchar2b_ptr++;
	    } else {
		CI_GET_CHAR_INFO_1D(font, *xchar_ptr, def, cs)
		xchar_ptr++;
	    }
	    if (cs == NULL)
		continue;

	    ink_buf->x = overall.width + cs->lbearing;
	    ink_buf->y = -(cs->ascent);
	    ink_buf->width = cs->rbearing - cs->lbearing;
	    ink_buf->height = cs->ascent + cs->descent;
	    ink_buf++;

	    logical_buf->x = overall.width;
	    logical_buf->y = -(font->ascent);
	    logical_buf->width = cs->width;
	    logical_buf->height = font->ascent + font->descent;
	    logical_buf++;

	    if (first) {
		overall = *cs;
		first = False;
	    } else {
		overall.ascent = max(overall.ascent, cs->ascent);
		overall.descent = max(overall.descent, cs->descent);
		overall.lbearing = min(overall.lbearing,
				       overall.width + cs->lbearing);
		overall.rbearing = max(overall.rbearing,
				       overall.width + cs->rbearing);
		overall.width += cs->width;
	    }

	    (*num_chars)++;
	}
    }

    if (require_num) {
	*num_chars = require_num;
	return 0;
    } else {
	if (overall_ink) {
	    overall_ink->x = overall.lbearing;
	    overall_ink->y = -(overall.ascent);
	    overall_ink->width = overall.rbearing - overall.lbearing;
	    overall_ink->height = overall.ascent + overall.descent;
	}

	if (overall_logical) {
	    overall_logical->x = 0;
	    overall_logical->y = -(logical_ascent);
	    overall_logical->width = overall.width;
	    overall_logical->height = logical_ascent + logical_descent;
	}
    }

    return 1;
}

Status
_XmbGenericTextPerCharExtents(XOC oc, _Xconst char *text, int length,
			      XRectangle *ink_buf, XRectangle *logical_buf,
			      int buf_size, int *num_chars,
			      XRectangle *overall_ink,
			      XRectangle *overall_logical)
{
    return _XomGenericTextPerCharExtents(oc, XOMMultiByte, (XPointer) text,
					 length, ink_buf, logical_buf, buf_size,
					 num_chars, overall_ink,
					 overall_logical);
}

Status
_XwcGenericTextPerCharExtents(XOC oc, _Xconst wchar_t *text, int length,
			      XRectangle *ink_buf, XRectangle *logical_buf,
			      int buf_size, int *num_chars,
			      XRectangle *overall_ink,
			      XRectangle *overall_logical)
{
    return _XomGenericTextPerCharExtents(oc, XOMWideChar, (XPointer) text,
					 length, ink_buf, logical_buf, buf_size,
					 num_chars, overall_ink,
					 overall_logical);
}

Status
_Xutf8GenericTextPerCharExtents(XOC oc, _Xconst char *text, int length,
				XRectangle *ink_buf, XRectangle *logical_buf,
				int buf_size, int *num_chars,
				XRectangle *overall_ink,
				XRectangle *overall_logical)
{
    return _XomGenericTextPerCharExtents(oc, XOMUtf8String, (XPointer) text,
					 length, ink_buf, logical_buf, buf_size,
					 num_chars, overall_ink,
					 overall_logical);
}
