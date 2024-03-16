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

int
_XomGenericTextExtents(
    XOC oc,
    XOMTextType type,
    XPointer text,
    int length,
    XRectangle *overall_ink,
    XRectangle *overall_logical)
{
    XlcConv conv;
    XFontStruct *font;
    Bool is_xchar2b;
    XPointer args[2];
    XChar2b xchar2b_buf[BUFSIZ], *buf;
    int direction, logical_ascent, logical_descent, tmp_ascent, tmp_descent;
    XCharStruct overall, tmp_overall;
    int buf_len, left;
    Bool first = True;

    conv = _XomInitConverter(oc, type);
    if (conv == NULL)
	return 0;

    bzero((char *) &overall, sizeof(XCharStruct));
    logical_ascent = logical_descent = 0;

    args[0] = (XPointer) &font;
    args[1] = (XPointer) &is_xchar2b;

    while (length > 0) {
	buf = xchar2b_buf;
	left = buf_len = BUFSIZ;

	if (_XomConvert(oc, conv, (XPointer *) &text, &length,
			(XPointer *) &buf, &left, args, 2) < 0)
	    break;
	buf_len -= left;

	if (is_xchar2b)
	    XTextExtents16(font, xchar2b_buf, buf_len, &direction,
			   &tmp_ascent, &tmp_descent, &tmp_overall);
	else
	    XTextExtents(font, (char *) xchar2b_buf, buf_len, &direction,
			 &tmp_ascent, &tmp_descent, &tmp_overall);

	if (first) {	/* initialize overall */
	    overall = tmp_overall;
	    logical_ascent = tmp_ascent;
	    logical_descent = tmp_descent;
	    first = False;
	} else {
	    overall.lbearing = min(overall.lbearing,
				   overall.width + tmp_overall.lbearing);
	    overall.rbearing = max(overall.rbearing,
				   overall.width + tmp_overall.rbearing);
	    overall.ascent = max(overall.ascent, tmp_overall.ascent);
	    overall.descent = max(overall.descent, tmp_overall.descent);
	    overall.width += tmp_overall.width;
	    logical_ascent = max(logical_ascent, tmp_ascent);
	    logical_descent = max(logical_descent, tmp_descent);
	}
    }

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

    return overall.width;
}

int
_XmbGenericTextExtents(XOC oc, _Xconst char *text, int length,
		       XRectangle *overall_ink, XRectangle *overall_logical)
{
    return _XomGenericTextExtents(oc, XOMMultiByte, (XPointer) text, length,
				  overall_ink, overall_logical);
}

int
_XwcGenericTextExtents(XOC oc, _Xconst wchar_t *text, int length,
		       XRectangle *overall_ink, XRectangle *overall_logical)
{
    return _XomGenericTextExtents(oc, XOMWideChar, (XPointer) text, length,
				  overall_ink, overall_logical);
}

int
_Xutf8GenericTextExtents(XOC oc, _Xconst char *text, int length,
			 XRectangle *overall_ink, XRectangle *overall_logical)
{
    return _XomGenericTextExtents(oc, XOMUtf8String, (XPointer) text, length,
				  overall_ink, overall_logical);
}
