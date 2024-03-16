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
#include <stdio.h>
#include "locking.h"

#ifdef XTHREADS
LockInfoPtr _conv_lock;
#endif

typedef struct _XlcConverterListRec {
    XLCd from_lcd;
    const char *from;
    XrmQuark from_type;
    XLCd to_lcd;
    const char *to;
    XrmQuark to_type;
    XlcOpenConverterProc converter;
    struct _XlcConverterListRec *next;
} XlcConverterListRec, *XlcConverterList;

static XlcConverterList conv_list = NULL;

static void
close_converter(
    XlcConv conv)
{
    (*conv->methods->close)(conv);
}

static XlcConv
get_converter(
    XLCd from_lcd,
    XrmQuark from_type,
    XLCd to_lcd,
    XrmQuark to_type)
{
    XlcConverterList list, prev = NULL;
    XlcConv conv = NULL;

    _XLockMutex(_conv_lock);

    for (list = conv_list; list; list = list->next) {
	if (list->from_lcd == from_lcd && list->to_lcd == to_lcd
	    && list->from_type == from_type && list->to_type == to_type) {

	    if (prev && prev != conv_list) {	/* XXX */
		prev->next = list->next;
		list->next = conv_list;
		conv_list = list;
	    }

	    conv = (*list->converter)(from_lcd, list->from, to_lcd, list->to);
	    break;
	}

	prev = list;
    }

    _XUnlockMutex(_conv_lock);

    return conv;
}

Bool
_XlcSetConverter(
    XLCd from_lcd,
    const char *from,
    XLCd to_lcd,
    const char *to,
    XlcOpenConverterProc converter)
{
    XlcConverterList list;
    XrmQuark from_type, to_type;

    from_type = XrmStringToQuark(from);
    to_type = XrmStringToQuark(to);

    _XLockMutex(_conv_lock);

    for (list = conv_list; list; list = list->next) {
	if (list->from_lcd == from_lcd && list->to_lcd == to_lcd
	    && list->from_type == from_type && list->to_type == to_type) {

	    list->converter = converter;
	    goto ret;
	}
    }

    list = Xmalloc(sizeof(XlcConverterListRec));
    if (list == NULL)
	goto ret;

    list->from_lcd = from_lcd;
    list->from = from;
    list->from_type = from_type;
    list->to_lcd = to_lcd;
    list->to = to;
    list->to_type = to_type;
    list->converter = converter;
    list->next = conv_list;
    conv_list = list;

ret:
    _XUnlockMutex(_conv_lock);
    return list != NULL;
}

typedef struct _ConvRec {
    XlcConv from_conv;
    XlcConv to_conv;
} ConvRec, *Conv;

static int
indirect_convert(
    XlcConv lc_conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    Conv conv = (Conv) lc_conv->state;
    XlcConv from_conv = conv->from_conv;
    XlcConv to_conv = conv->to_conv;
    XlcCharSet charset;
    char buf[BUFSIZ], *cs;
    XPointer tmp_args[1];
    int cs_left, ret, length, unconv_num = 0;

    if (from == NULL || *from == NULL) {
	if (from_conv->methods->reset)
	    (*from_conv->methods->reset)(from_conv);

	if (to_conv->methods->reset)
	    (*to_conv->methods->reset)(to_conv);

	return 0;
    }

    while (*from_left > 0) {
	cs = buf;
	cs_left = BUFSIZ;
	tmp_args[0] = (XPointer) &charset;

	ret = (*from_conv->methods->convert)(from_conv, from, from_left, &cs,
					     &cs_left, tmp_args, 1);
	if (ret < 0)
	    break;

	unconv_num += ret;

	length = cs - buf;
	if (length > 0) {
	    cs_left = length;
	    cs = buf;

	    tmp_args[0] = (XPointer) charset;

	    ret = (*to_conv->methods->convert)(to_conv, &cs, &cs_left, to, to_left,
					       tmp_args, 1);
	    if (ret < 0) {
		unconv_num += length / (charset->char_size > 0 ? charset->char_size : 1);
		continue;
	    }

	    unconv_num += ret;

	    if (*to_left < 1)
		break;
	}
    }

    return unconv_num;
}

static void
close_indirect_converter(
    XlcConv lc_conv)
{
    Conv conv = (Conv) lc_conv->state;

    if (conv) {
	if (conv->from_conv)
	    close_converter(conv->from_conv);
	if (conv->to_conv)
	    close_converter(conv->to_conv);

	Xfree(conv);
    }

    Xfree(lc_conv);
}

static void
reset_indirect_converter(
    XlcConv lc_conv)
{
    Conv conv = (Conv) lc_conv->state;

    if (conv) {
	if (conv->from_conv && conv->from_conv->methods->reset)
	    (*conv->from_conv->methods->reset)(conv->from_conv);
	if (conv->to_conv && conv->to_conv->methods->reset)
	    (*conv->to_conv->methods->reset)(conv->to_conv);
    }
}

static XlcConvMethodsRec conv_methods = {
    close_indirect_converter,
    indirect_convert,
    reset_indirect_converter
} ;

static XlcConv
open_indirect_converter(
    XLCd from_lcd,
    const char *from,
    XLCd to_lcd,
    const char *to)
{
    XlcConv lc_conv, from_conv, to_conv;
    Conv conv;
    XrmQuark from_type, to_type;
    static XrmQuark QChar, QCharSet, QCTCharSet = (XrmQuark) 0;

    if (QCTCharSet == (XrmQuark) 0) {
	QCTCharSet = XrmStringToQuark(XlcNCTCharSet);
	QCharSet = XrmStringToQuark(XlcNCharSet);
	QChar = XrmStringToQuark(XlcNChar);
    }

    from_type = XrmStringToQuark(from);
    to_type = XrmStringToQuark(to);

    if (from_type == QCharSet || from_type == QChar || to_type == QCharSet ||
	to_type == QChar)
	return (XlcConv) NULL;

    lc_conv = Xmalloc(sizeof(XlcConvRec));
    if (lc_conv == NULL)
	return (XlcConv) NULL;

    lc_conv->methods = &conv_methods;

    lc_conv->state = Xcalloc(1, sizeof(ConvRec));
    if (lc_conv->state == NULL)
	goto err;

    conv = (Conv) lc_conv->state;

    from_conv = get_converter(from_lcd, from_type, from_lcd, QCTCharSet);
    if (from_conv == NULL)
	from_conv = get_converter(from_lcd, from_type, from_lcd, QCharSet);
    if (from_conv == NULL)
	from_conv = get_converter((XLCd)NULL, from_type, (XLCd)NULL, QCharSet);
    if (from_conv == NULL)
	from_conv = get_converter(from_lcd, from_type, from_lcd, QChar);
    if (from_conv == NULL)
	goto err;
    conv->from_conv = from_conv;

    to_conv = get_converter(to_lcd, QCTCharSet, to_lcd, to_type);
    if (to_conv == NULL)
	to_conv = get_converter(to_lcd, QCharSet, to_lcd, to_type);
    if (to_conv == NULL)
	to_conv = get_converter((XLCd) NULL, QCharSet, (XLCd) NULL, to_type);
    if (to_conv == NULL)
	goto err;
    conv->to_conv = to_conv;

    return lc_conv;

err:
    close_indirect_converter(lc_conv);

    return (XlcConv) NULL;
}

XlcConv
_XlcOpenConverter(
    XLCd from_lcd,
    const char *from,
    XLCd to_lcd,
    const char *to)
{
    XlcConv conv;
    XrmQuark from_type, to_type;

    from_type = XrmStringToQuark(from);
    to_type = XrmStringToQuark(to);

    if ((conv = get_converter(from_lcd, from_type, to_lcd, to_type)))
	return conv;

    return open_indirect_converter(from_lcd, from, to_lcd, to);
}

void
_XlcCloseConverter(
    XlcConv conv)
{
    close_converter(conv);
}

int
_XlcConvert(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    return (*conv->methods->convert)(conv, from, from_left, to, to_left, args,
				     num_args);
}

void
_XlcResetConverter(
    XlcConv conv)
{
    if (conv->methods->reset)
	(*conv->methods->reset)(conv);
}
