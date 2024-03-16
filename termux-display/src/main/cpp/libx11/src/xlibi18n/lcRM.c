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
 * Bug fixes: Bruno Haible	XFree86 Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "XlcPubI.h"
#include <stdio.h>

/*
 * Default implementation of methods for Xrm parsing.
 */

/* ======================= Unibyte implementation ======================= */

/* Only for efficiency, to speed up things. */

/* This implementation must keep the locale, for lcname. */
typedef struct _UbStateRec {
    XLCd	lcd;
} UbStateRec, *UbState;

/* Sets the state to the initial state.
   Initiates a sequence of calls to mbchar. */
static void
ub_mbinit(
    XPointer state)
{
}

/* Transforms one multibyte character, and return a 'char' in the same
   parsing class. Returns the number of consumed bytes in *lenp. */
static char
ub_mbchar(
    XPointer state,
    const char *str,
    int	*lenp)
{
    *lenp = 1;
    return *str;
}

/* Terminates a sequence of calls to mbchar. */
static void
ub_mbfinish(
    XPointer state)
{
}

/* Returns the name of the state's locale, as a static string. */
static const char *
ub_lcname(
    XPointer state)
{
    return ((UbState) state)->lcd->core->name;
}

/* Frees the state, which was allocated by _XrmDefaultInitParseInfo. */
static void
ub_destroy(
    XPointer state)
{
    _XCloseLC(((UbState) state)->lcd);
    Xfree(state);
}

static const XrmMethodsRec ub_methods = {
    ub_mbinit,
    ub_mbchar,
    ub_mbfinish,
    ub_lcname,
    ub_destroy
};

/* ======================= Multibyte implementation ======================= */

/* This implementation uses an XlcConv from XlcNMultiByte to XlcNWideChar. */
typedef struct _MbStateRec {
    XLCd	lcd;
    XlcConv	conv;
} MbStateRec, *MbState;

/* Sets the state to the initial state.
   Initiates a sequence of calls to mbchar. */
static void
mb_mbinit(
    XPointer state)
{
    _XlcResetConverter(((MbState) state)->conv);
}

/* Transforms one multibyte character, and return a 'char' in the same
   parsing class. Returns the number of consumed bytes in *lenp. */
static char
mb_mbchar(
    XPointer state,
    const char *str,
    int	*lenp)
{
    XlcConv conv = ((MbState) state)->conv;
    const char *from;
    wchar_t *to, wc;
    int cur_max, i, from_left, to_left, ret;

    cur_max = XLC_PUBLIC(((MbState) state)->lcd, mb_cur_max);

    from = str;
    /* Determine from_left. Avoid overrun error which could occur if
       from_left > strlen(str). */
    from_left = cur_max;
    for (i = 0; i < cur_max; i++)
	if (str[i] == '\0') {
	    from_left = i;
	    break;
	}
    *lenp = from_left;

    to = &wc;
    to_left = 1;

    ret = _XlcConvert(conv, (XPointer *) &from, &from_left,
		      (XPointer *) &to, &to_left, NULL, 0);
    *lenp -= from_left;

    if (ret < 0 || to_left > 0) {
	/* Invalid or incomplete multibyte character seen. */
	*lenp = 1;
	return 0x7f;
    }
    /* Return a 'char' equivalent to wc. */
    return (wc >= 0 && wc <= 0x7f ? wc : 0x7f);
}

/* Terminates a sequence of calls to mbchar. */
static void
mb_mbfinish(
    XPointer state)
{
}

/* Returns the name of the state's locale, as a static string. */
static const char *
mb_lcname(
    XPointer state)
{
    return ((MbState) state)->lcd->core->name;
}

/* Frees the state, which was allocated by _XrmDefaultInitParseInfo. */
static void
mb_destroy(
    XPointer state)
{
    _XlcCloseConverter(((MbState) state)->conv);
    _XCloseLC(((MbState) state)->lcd);
    Xfree(state);
}

static const XrmMethodsRec mb_methods = {
    mb_mbinit,
    mb_mbchar,
    mb_mbfinish,
    mb_lcname,
    mb_destroy
};

/* ======================= Exported function ======================= */

XrmMethods
_XrmDefaultInitParseInfo(
    XLCd lcd,
    XPointer *rm_state)
{
    if (XLC_PUBLIC(lcd, mb_cur_max) == 1) {
	/* Unibyte case. */
	UbState state = Xmalloc(sizeof(UbStateRec));
	if (state == NULL)
	    return (XrmMethods) NULL;

	state->lcd = lcd;

	*rm_state = (XPointer) state;
	return &ub_methods;
    } else {
	/* Multibyte case. */
	MbState state = Xmalloc(sizeof(MbStateRec));
	if (state == NULL)
	    return (XrmMethods) NULL;

	state->lcd = lcd;
	state->conv = _XlcOpenConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar);
	if (state->conv == NULL) {
	    Xfree(state);
	    return (XrmMethods) NULL;
	}

	*rm_state = (XPointer) state;
	return &mb_methods;
    }
}
