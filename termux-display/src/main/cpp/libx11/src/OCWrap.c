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

XOC
XCreateOC(XOM om, ...)
{
    va_list var;
    XlcArgList args;
    XOC oc;
    int num_args;

    va_start(var, om);
    _XlcCountVaList(var, &num_args);
    va_end(var);

    va_start(var, om);
    _XlcVaToArgList(var, num_args, &args);
    va_end(var);

    if (args == (XlcArgList) NULL)
	return (XOC) NULL;

    oc = (*om->methods->create_oc)(om, args, num_args);

    Xfree(args);

    if (oc) {
	oc->core.next = om->core.oc_list;
	om->core.oc_list = oc;
    }

    return oc;
}

void
XDestroyOC(XOC oc)
{
    XOC prev, oc_list;

    prev = oc_list = oc->core.om->core.oc_list;
    if (oc_list == oc)
	oc->core.om->core.oc_list = oc_list->core.next;
    else {
	while ((oc_list = oc_list->core.next)) {
	    if (oc_list == oc) {
		prev->core.next = oc_list->core.next;
		break;
	    }
	    prev = oc_list;
	}
    }

    (*oc->methods->destroy)(oc);
}

XOM
XOMOfOC(XOC oc)
{
    return oc->core.om;
}

char *
XSetOCValues(XOC oc, ...)
{
    va_list var;
    XlcArgList args;
    char *ret;
    int num_args;

    va_start(var, oc);
    _XlcCountVaList(var, &num_args);
    va_end(var);

    va_start(var, oc);
    _XlcVaToArgList(var, num_args, &args);
    va_end(var);

    if (args == (XlcArgList) NULL)
	return (char *) NULL;

    ret = (*oc->methods->set_values)(oc, args, num_args);

    Xfree(args);

    return ret;
}

char *
XGetOCValues(XOC oc, ...)
{
    va_list var;
    XlcArgList args;
    char *ret;
    int num_args;

    va_start(var, oc);
    _XlcCountVaList(var, &num_args);
    va_end(var);

    va_start(var, oc);
    _XlcVaToArgList(var, num_args, &args);
    va_end(var);

    if (args == (XlcArgList) NULL)
	return (char *) NULL;

    ret = (*oc->methods->get_values)(oc, args, num_args);

    Xfree(args);

    return ret;
}
