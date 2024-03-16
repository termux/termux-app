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
#include "Xlcint.h"

XOM
XOpenOM(Display *dpy, XrmDatabase rdb, _Xconst char *res_name,
	_Xconst char *res_class)
{
    XLCd lcd = _XOpenLC((char *) NULL);

    if (lcd == NULL)
	return (XOM) NULL;

    if (lcd->methods->open_om)
	return (*lcd->methods->open_om)(lcd, dpy, rdb, res_name, res_class);

    return (XOM) NULL;
}

Status
XCloseOM(XOM om)
{
    XOC oc, next;
    XLCd lcd = om->core.lcd;

    next = om->core.oc_list;

    while ((oc = next)) {
	next = oc->core.next;
	(*oc->methods->destroy)(oc);
    }

    om->core.oc_list = NULL;

    _XCloseLC(lcd);

    return (*om->methods->close)(om);
}

char *
XSetOMValues(XOM om, ...)
{
    va_list var;
    XlcArgList args;
    char *ret;
    int num_args;

    va_start(var, om);
    _XlcCountVaList(var, &num_args);
    va_end(var);

    va_start(var, om);
    _XlcVaToArgList(var, num_args, &args);
    va_end(var);

    if (args == (XlcArgList) NULL)
	return (char *) NULL;

    ret = (*om->methods->set_values)(om, args, num_args);

    Xfree(args);

    return ret;
}

char *
XGetOMValues(XOM om, ...)
{
    va_list var;
    XlcArgList args;
    char *ret;
    int num_args;

    va_start(var, om);
    _XlcCountVaList(var, &num_args);
    va_end(var);

    va_start(var, om);
    _XlcVaToArgList(var, num_args, &args);
    va_end(var);

    if (args == (XlcArgList) NULL)
	return (char *) NULL;

    ret = (*om->methods->get_values)(om, args, num_args);

    Xfree(args);

    return ret;
}

Display *
XDisplayOfOM(XOM om)
{
    return om->core.display;
}

char *
XLocaleOfOM(XOM om)
{
    return om->core.lcd->core->name;
}
