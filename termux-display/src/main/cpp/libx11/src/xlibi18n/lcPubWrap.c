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
#include "XlcPubI.h"

char *
_XGetLCValues(XLCd lcd, ...)
{
    va_list var;
    XlcArgList args;
    char *ret;
    int num_args;
    XLCdPublicMethodsPart *methods = XLC_PUBLIC_METHODS(lcd);

    va_start(var, lcd);
    _XlcCountVaList(var, &num_args);
    va_end(var);

    va_start(var, lcd);
    _XlcVaToArgList(var, num_args, &args);
    va_end(var);

    if (args == (XlcArgList) NULL)
	return (char *) NULL;

    ret = (*methods->get_values)(lcd, args, num_args);

    Xfree(args);

    return ret;
}

void
_XlcDestroyLC(
    XLCd lcd)
{
    XLCdPublicMethods methods = (XLCdPublicMethods) lcd->methods;

    (*methods->pub.destroy)(lcd);
}

XLCd
_XlcCreateLC(
    const char *name,
    XLCdMethods methods)
{
    XLCdPublicMethods pub_methods = (XLCdPublicMethods) methods;
    XLCd lcd;

    lcd = (*pub_methods->pub.create)(name, methods);
    if (lcd == NULL)
	return (XLCd) NULL;

    if (lcd->core->name == NULL) {
	lcd->core->name = strdup(name);
	if (lcd->core->name == NULL)
	    goto err;
    }

    if (lcd->methods == NULL)
	lcd->methods = methods;

    if ((*pub_methods->pub.initialize)(lcd) == False)
	goto err;

    return lcd;

err:
    _XlcDestroyLC(lcd);

    return (XLCd) NULL;
}
