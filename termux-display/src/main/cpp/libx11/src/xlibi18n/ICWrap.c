/*
 * Copyright 1990, 1991 by OMRON Corporation, NTT Software Corporation,
 *                      and Nippon Telegraph and Telephone Corporation
 * Copyright 1991 by the Open Software Foundation
 * Copyright 1993 by the FUJITSU LIMITED
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of OMRON, NTT Software, NTT, and
 * Open Software Foundation not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission. OMRON, NTT Software, NTT, and Open Software
 * Foundation make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * OMRON, NTT SOFTWARE, NTT, AND OPEN SOFTWARE FOUNDATION
 * DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT
 * SHALL OMRON, NTT SOFTWARE, NTT, OR OPEN SOFTWARE FOUNDATION BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *	Authors: Li Yuhong		OMRON Corporation
 *		 Tatsuya Kato		NTT Software Corporation
 *		 Hiroshi Kuribayashi	OMRON Coproration
 *		 Muneiyoshi Suzuki	Nippon Telegraph and Telephone Co.
 *
 *		 M. Collins		OSF
 *		 Takashi Fujiwara	FUJITSU LIMITED
 */
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xlcint.h"
#include "reallocarray.h"

static int
_XIMNestedListToNestedList(
    XIMArg *nlist,   /* This is the new list */
    XIMArg *list)    /* The original list */
{
    register XIMArg *ptr = list;

    while (ptr->name) {
	if (!strcmp(ptr->name, XNVaNestedList)) {
	    nlist += _XIMNestedListToNestedList(nlist, (XIMArg *)ptr->value);
	} else {
	    nlist->name = ptr->name;
	    nlist->value = ptr->value;
	    ptr++;
	    nlist++;
	}
    }
    return (int) (ptr - list);
}

static void
_XIMCountNestedList(
    XIMArg *args,
    int *total_count)
{
    for (; args->name; args++) {
	if (!strcmp(args->name, XNVaNestedList))
	    _XIMCountNestedList((XIMArg *)args->value, total_count);
	else
	    ++(*total_count);
    }
}

static void
_XIMCountVaList(va_list var, int *total_count)
{
    char *attr;

    *total_count = 0;

    for (attr = va_arg(var, char*); attr; attr = va_arg(var, char*)) {
	if (!strcmp(attr, XNVaNestedList)) {
	    _XIMCountNestedList(va_arg(var, XIMArg*), total_count);
	} else {
	    (void)va_arg(var, XIMArg*);
	    ++(*total_count);
	}
    }
}

static void
_XIMVaToNestedList(va_list var, int max_count, XIMArg **args_return)
{
    XIMArg *args;
    char   *attr;

    if (max_count <= 0) {
	*args_return = (XIMArg *)NULL;
	return;
    }

    args = Xmallocarray((unsigned)max_count + 1, sizeof(XIMArg));
    *args_return = args;
    if (!args) return;

    for (attr = va_arg(var, char*); attr; attr = va_arg(var, char*)) {
	if (!strcmp(attr, XNVaNestedList)) {
	    args += _XIMNestedListToNestedList(args, va_arg(var, XIMArg*));
	} else {
	    args->name = attr;
	    args->value = va_arg(var, XPointer);
	    args++;
	}
    }
    args->name = (char*)NULL;
}

/*ARGSUSED*/
XVaNestedList
XVaCreateNestedList(int dummy, ...)
{
    va_list		var;
    XIMArg		*args = NULL;
    int			total_count;

    va_start(var, dummy);
    _XIMCountVaList(var, &total_count);
    va_end(var);

    va_start(var, dummy);
    _XIMVaToNestedList(var, total_count, &args);
    va_end(var);

    return (XVaNestedList)args;
}

char *
XSetIMValues(XIM im, ...)
{
    va_list var;
    int     total_count;
    XIMArg *args;
    char   *ret = NULL;

    /*
     * so count the stuff dangling here
     */
    va_start(var, im);
    _XIMCountVaList(var, &total_count);
    va_end(var);

    /*
     * now package it up so we can send it along
     */
    va_start(var, im);
    _XIMVaToNestedList(var, total_count, &args);
    va_end(var);

    if (im && im->methods)
	ret = (*im->methods->set_values) (im, args);
    Xfree(args);
    return ret;
}

char *
XGetIMValues(XIM im, ...)
{
    va_list var;
    int     total_count;
    XIMArg *args;
    char   *ret = NULL;

    /*
     * so count the stuff dangling here
     */
    va_start(var, im);
    _XIMCountVaList(var, &total_count);
    va_end(var);

    /*
     * now package it up so we can send it along
     */
    va_start(var, im);
    _XIMVaToNestedList(var, total_count, &args);
    va_end(var);

    if (im && im->methods)
	ret = (*im->methods->get_values) (im, args);
    Xfree(args);
    return ret;
}

/*
 * Create an input context within the input method,
 * and return a pointer to the input context.
 */

XIC
XCreateIC(XIM im, ...)
{
    va_list var;
    int     total_count;
    XIMArg *args;
    XIC     ic = NULL;

    /*
     * so count the stuff dangling here
     */
    va_start(var, im);
    _XIMCountVaList(var, &total_count);
    va_end(var);

    /*
     * now package it up so we can send it along
     */
    va_start(var, im);
    _XIMVaToNestedList(var, total_count, &args);
    va_end(var);

    if (im && im->methods)
	ic = (XIC) (*im->methods->create_ic) (im, args);
    Xfree(args);
    if (ic) {
	ic->core.next = im->core.ic_chain;
	im->core.ic_chain = ic;
    }
    return ic;
}

/*
 * Free the input context.
 */
void
XDestroyIC(XIC ic)
{
    XIM im = ic->core.im;
    XIC *prev;

    (*ic->methods->destroy) (ic);
    if (im) {
	for (prev = &im->core.ic_chain; *prev; prev = &(*prev)->core.next) {
	    if (*prev == ic) {
		*prev = ic->core.next;
		break;
	    }
	}
    }
    Xfree (ic);
}

char *
XGetICValues(XIC ic, ...)
{
    va_list var;
    int     total_count;
    XIMArg *args;
    char   *ret;

    if (!ic->core.im)
	return (char *) NULL;

    /*
     * so count the stuff dangling here
     */
    va_start(var, ic);
    _XIMCountVaList(var, &total_count);
    va_end(var);

    /*
     * now package it up so we can send it along
     */
    va_start(var, ic);
    _XIMVaToNestedList(var, total_count, &args);
    va_end(var);

    ret = (*ic->methods->get_values) (ic, args);
    Xfree(args);
    return ret;
}

char *
XSetICValues(XIC ic, ...)
{
    va_list var;
    int     total_count;
    XIMArg *args;
    char   *ret;

    if (!ic->core.im)
	return (char *) NULL;

    /*
     * so count the stuff dangling here
     */
    va_start(var, ic);
    _XIMCountVaList(var, &total_count);
    va_end(var);

    /*
     * now package it up so we can send it along
     */
    va_start(var, ic);
    _XIMVaToNestedList(var, total_count, &args);
    va_end(var);

    ret = (*ic->methods->set_values) (ic, args);
    Xfree(args);
    return ret;
}

/*
 * Require the input manager to focus the focus window attached to the ic
 * argument.
 */
void
XSetICFocus(XIC ic)
{
  if (ic && ic->core.im)
      (*ic->methods->set_focus) (ic);
}

/*
 * Require the input manager to unfocus the focus window attached to the ic
 * argument.
 */
void
XUnsetICFocus(XIC ic)
{
  if (ic->core.im)
      (*ic->methods->unset_focus) (ic);
}

/*
 * Return the XIM associated with the input context.
 */
XIM
XIMOfIC(XIC ic)
{
    return ic->core.im;
}

char *
XmbResetIC(XIC ic)
{
    if (ic->core.im)
	return (*ic->methods->mb_reset)(ic);
    return (char *)NULL;
}

wchar_t *
XwcResetIC(XIC ic)
{
    if (ic->core.im)
	return (*ic->methods->wc_reset)(ic);
    return (wchar_t *)NULL;
}

char *
Xutf8ResetIC(XIC ic)
{
    if (ic->core.im) {
	if (ic->methods->utf8_reset)
	    return (*ic->methods->utf8_reset)(ic);
	else if (ic->methods->mb_reset)
	    return (*ic->methods->mb_reset)(ic);
    }
    return (char *)NULL;
}

int
XmbLookupString(XIC ic, XKeyEvent *ev, char *buffer, int nbytes,
		KeySym *keysym, Status *status)
{
    if (ic->core.im)
	return (*ic->methods->mb_lookup_string) (ic, ev, buffer, nbytes,
						 keysym, status);
    return XLookupNone;
}

int
XwcLookupString(XIC ic, XKeyEvent *ev, wchar_t *buffer, int nchars,
		KeySym *keysym, Status *status)
{
    if (ic->core.im)
	return (*ic->methods->wc_lookup_string) (ic, ev, buffer, nchars,
						 keysym, status);
    return XLookupNone;
}

int
Xutf8LookupString(XIC ic, XKeyEvent *ev, char *buffer, int nbytes,
		  KeySym *keysym, Status *status)
{
    if (ic->core.im) {
	if (ic->methods->utf8_lookup_string)
	    return (*ic->methods->utf8_lookup_string) (ic, ev, buffer, nbytes,
						   	keysym, status);
	else if (ic->methods->mb_lookup_string)
	    return (*ic->methods->mb_lookup_string) (ic, ev, buffer, nbytes,
						   	keysym, status);
    }
    return XLookupNone;
}
