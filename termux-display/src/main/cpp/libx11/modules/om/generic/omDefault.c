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
 *  (c) Copyright 1995 FUJITSU LIMITED
 *  This is source code modified by FUJITSU LIMITED under the Joint
 *  Development Agreement for the CDE/Motif PST.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "XomGeneric.h"
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>

#define DefineLocalBuf		char local_buf[BUFSIZ]
#define AllocLocalBuf(length)	(length > BUFSIZ ? (char *)Xmalloc(length) : local_buf)
#define FreeLocalBuf(ptr)	if (ptr != local_buf) Xfree(ptr)

static Bool
wcs_to_mbs(
    XOC oc,
    char *to,
    _Xconst wchar_t *from,
    int length)
{
    XlcConv conv;
    int to_left, ret;

    conv = _XomInitConverter(oc, XOMWideChar);
    if (conv == NULL)
	return False;

    to_left = length;
    ret = _XlcConvert(conv, (XPointer *) &from, &length, (XPointer *) &to,
		      &to_left, NULL, 0);
    if (ret != 0 || length > 0)
	return False;

    return True;
}

static Bool
utf8_to_mbs(
    XOC oc,
    char *to,
    _Xconst char *from,
    int length)
{
    XlcConv conv;
    int to_left, ret;

    conv = _XomInitConverter(oc, XOMUtf8String);
    if (conv == NULL)
	return False;

    to_left = length;
    ret = _XlcConvert(conv, (XPointer *) &from, &length, (XPointer *) &to,
		      &to_left, NULL, 0);
    if (ret != 0 || length > 0)
	return False;

    return True;
}

int
_XmbDefaultTextEscapement(XOC oc, _Xconst char *text, int length)
{
    return XTextWidth(*oc->core.font_info.font_struct_list, text, length);
}

int
_XwcDefaultTextEscapement(XOC oc, _Xconst wchar_t *text, int length)
{
    DefineLocalBuf;
    char *buf = AllocLocalBuf(length);
    int ret;

    if (buf == NULL)
	return 0;

    if (wcs_to_mbs(oc, buf, text, length) == False) {
	ret = 0;
	goto err;
    }

    ret = _XmbDefaultTextEscapement(oc, buf, length);

err:
    FreeLocalBuf(buf);

    return ret;
}

int
_Xutf8DefaultTextEscapement(XOC oc, _Xconst char *text, int length)
{
    DefineLocalBuf;
    char *buf = AllocLocalBuf(length);
    int ret;

    if (buf == NULL)
	return 0;

    if (utf8_to_mbs(oc, buf, text, length) == False) {
	ret = 0;
	goto err;
    }

    ret = _XmbDefaultTextEscapement(oc, buf, length);

err:
    FreeLocalBuf(buf);

    return ret;
}

int
_XmbDefaultTextExtents(XOC oc, _Xconst char *text, int length,
		       XRectangle *overall_ink, XRectangle *overall_logical)
{
    int direction, logical_ascent, logical_descent;
    XCharStruct overall;

    XTextExtents(*oc->core.font_info.font_struct_list, text, length, &direction,
		 &logical_ascent, &logical_descent, &overall);

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
_XwcDefaultTextExtents(XOC oc, _Xconst wchar_t *text, int length,
		       XRectangle *overall_ink, XRectangle *overall_logical)
{
    DefineLocalBuf;
    char *buf = AllocLocalBuf(length);
    int ret;

    if (buf == NULL)
	return 0;

    if (wcs_to_mbs(oc, buf, text, length) == False) {
	ret = 0;
	goto err;
    }

    ret = _XmbDefaultTextExtents(oc, buf, length, overall_ink, overall_logical);

err:
    FreeLocalBuf(buf);

    return ret;
}

int
_Xutf8DefaultTextExtents(XOC oc, _Xconst char *text, int length,
			 XRectangle *overall_ink, XRectangle *overall_logical)
{
    DefineLocalBuf;
    char *buf = AllocLocalBuf(length);
    int ret;

    if (buf == NULL)
	return 0;

    if (utf8_to_mbs(oc, buf, text, length) == False) {
	ret = 0;
	goto err;
    }

    ret = _XmbDefaultTextExtents(oc, buf, length, overall_ink, overall_logical);

err:
    FreeLocalBuf(buf);

    return ret;
}

Status
_XmbDefaultTextPerCharExtents(XOC oc, _Xconst char *text, int length,
			      XRectangle *ink_buf, XRectangle *logical_buf,
			      int buf_size, int *num_chars,
			      XRectangle *overall_ink,
			      XRectangle *overall_logical)
{
    XFontStruct *font = *oc->core.font_info.font_struct_list;
    XCharStruct *def, *cs, overall;
    Bool first = True;

    if (buf_size < length)
	return 0;

    bzero((char *) &overall, sizeof(XCharStruct));
    *num_chars = 0;

    CI_GET_DEFAULT_INFO_1D(font, def)

    while (length-- > 0) {
	CI_GET_CHAR_INFO_1D(font, *text, def, cs)
	text++;
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
	    overall.lbearing = min(overall.lbearing, overall.width +
				   cs->lbearing);
	    overall.rbearing = max(overall.rbearing, overall.width +
				   cs->rbearing);
	    overall.width += cs->width;
	}

	(*num_chars)++;
    }

    if (overall_ink) {
	overall_ink->x = overall.lbearing;
	overall_ink->y = -(overall.ascent);
	overall_ink->width = overall.rbearing - overall.lbearing;
	overall_ink->height = overall.ascent + overall.descent;
    }

    if (overall_logical) {
	overall_logical->x = 0;
	overall_logical->y = -(font->ascent);
	overall_logical->width = overall.width;
	overall_logical->height = font->ascent + font->descent;
    }

    return 1;
}

Status
_XwcDefaultTextPerCharExtents(XOC oc, _Xconst wchar_t *text, int length,
			      XRectangle *ink_buf, XRectangle *logical_buf,
			      int buf_size, int *num_chars,
			      XRectangle *overall_ink,
			      XRectangle *overall_logical)
{
    DefineLocalBuf;
    char *buf = AllocLocalBuf(length);
    Status ret;

    if (buf == NULL)
	return 0;

    if (wcs_to_mbs(oc, buf, text, length) == False) {
	ret = 0;
	goto err;
    }

    ret = _XmbDefaultTextPerCharExtents(oc, buf, length, ink_buf, logical_buf,
					buf_size, num_chars, overall_ink,
					overall_logical);

err:
    FreeLocalBuf(buf);

    return ret;
}

Status
_Xutf8DefaultTextPerCharExtents(XOC oc, _Xconst char *text, int length,
				XRectangle *ink_buf, XRectangle *logical_buf,
				int buf_size, int *num_chars,
				XRectangle *overall_ink,
				XRectangle *overall_logical)
{
    DefineLocalBuf;
    char *buf = AllocLocalBuf(length);
    Status ret;

    if (buf == NULL)
	return 0;

    if (utf8_to_mbs(oc, buf, text, length) == False) {
	ret = 0;
	goto err;
    }

    ret = _XmbDefaultTextPerCharExtents(oc, buf, length, ink_buf, logical_buf,
					buf_size, num_chars, overall_ink,
					overall_logical);

err:
    FreeLocalBuf(buf);

    return ret;
}

int
_XmbDefaultDrawString(Display *dpy, Drawable d, XOC oc, GC gc, int x, int y,
		      _Xconst char *text, int length)
{
    XFontStruct *font = *oc->core.font_info.font_struct_list;

    XSetFont(dpy, gc, font->fid);
    XDrawString(dpy, d, gc, x, y, text, length);

    return XTextWidth(font, text, length);
}

int
_XwcDefaultDrawString(Display *dpy, Drawable d, XOC oc, GC gc, int x, int y,
		      _Xconst wchar_t *text, int length)
{
    DefineLocalBuf;
    char *buf = AllocLocalBuf(length);
    int ret;

    if (buf == NULL)
	return 0;

    if (wcs_to_mbs(oc, buf, text, length) == False) {
	ret = 0;
	goto err;
    }

    ret = _XmbDefaultDrawString(dpy, d, oc, gc, x, y, buf, length);

err:
    FreeLocalBuf(buf);

    return ret;
}

int
_Xutf8DefaultDrawString(Display *dpy, Drawable d, XOC oc, GC gc, int x, int y,
			_Xconst char *text, int length)
{
    DefineLocalBuf;
    char *buf = AllocLocalBuf(length);
    int ret;

    if (buf == NULL)
	return 0;

    if (utf8_to_mbs(oc, buf, text, length) == False) {
	ret = 0;
	goto err;
    }

    ret = _XmbDefaultDrawString(dpy, d, oc, gc, x, y, buf, length);

err:
    FreeLocalBuf(buf);

    return ret;
}

void
_XmbDefaultDrawImageString(Display *dpy, Drawable d, XOC oc, GC gc, int x,
			   int y, _Xconst char *text, int length)
{
    XSetFont(dpy, gc, (*oc->core.font_info.font_struct_list)->fid);
    XDrawImageString(dpy, d, gc, x, y, text, length);
}

void
_XwcDefaultDrawImageString(Display *dpy, Drawable d, XOC oc, GC gc, int x,
			   int y, _Xconst wchar_t *text, int length)
{
    DefineLocalBuf;
    char *buf = AllocLocalBuf(length);

    if (buf == NULL)
	return;

    if (wcs_to_mbs(oc, buf, text, length) == False)
	goto err;

    _XmbDefaultDrawImageString(dpy, d, oc, gc, x, y, buf, length);

err:
    FreeLocalBuf(buf);
}

void
_Xutf8DefaultDrawImageString(Display *dpy, Drawable d, XOC oc, GC gc, int x,
			     int y, _Xconst char *text, int length)
{
    DefineLocalBuf;
    char *buf = AllocLocalBuf(length);

    if (buf == NULL)
	return;

    if (utf8_to_mbs(oc, buf, text, length) == False)
	goto err;

    _XmbDefaultDrawImageString(dpy, d, oc, gc, x, y, buf, length);

err:
    FreeLocalBuf(buf);
}
