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
 * 2000
 * Modifier: Ivan Pascal        The XFree86 Project
 */

/*
 * The default locale loader.
 * Supports: one byte per char (iso8859 like) locales.
 * How: converts bytes to wide characters in a 1:1 manner.
 * Platforms: all systems.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "XlcGeneric.h"

#ifndef MB_LEN_MAX
#define MB_LEN_MAX 6
#endif

#if !defined(Lynx_22) && !defined(X_LOCALE)
#define STDCVT
#endif

#define GR	0x80
#define GL	0x7f

typedef struct _StateRec *State;
typedef struct _StateRec {
    CodeSet     GL_codeset;
    CodeSet     GR_codeset;
    wchar_t     wc_mask;
    wchar_t     wc_encode_mask;
    Bool        (*MBtoWC) (State state, const char *ch, wchar_t *wc);
    Bool        (*WCtoMB) (State state, wchar_t wc, char *ch);
} StateRec;

static
Bool MBtoWCdef(
    State    state,
    const char *ch,
    wchar_t  *wc)
{
    wchar_t wc_encoding;
    CodeSet codeset = (*ch & GR) ? state->GR_codeset : state->GL_codeset;
    if (!codeset)
	return False;
    wc_encoding = codeset->wc_encoding;
    *wc = ((wchar_t) *ch & state->wc_mask) | wc_encoding;
    return True;
}

#ifdef STDCVT
static
Bool MBtoWCstd(
    State   state,
    const char *ch,
    wchar_t *wc)
{
    return (mbtowc(wc, ch, 1) == 1);
}
#endif

static
Bool WCtoMBdef(
    State   state,
    wchar_t wc,
    char    *ch)
{
    wchar_t wc_encoding = wc & state->wc_encode_mask;
    CodeSet codeset;

    codeset = state->GL_codeset;
    if (codeset && (wc_encoding == codeset->wc_encoding)) {
	*ch = wc & state->wc_mask;
	return True;
    }
    codeset = state->GR_codeset;
    if (codeset && (wc_encoding == codeset->wc_encoding)) {
	*ch = (wc & state->wc_mask) | GR;
	return True;
    }
    return False;
}

#ifdef STDCVT
static
Bool WCtoMBstd(
    State   state,
    wchar_t wc,
    char    *ch)
{
    return (wctomb(ch, wc) == 1);
}
#endif

static
XlcCharSet get_charset(
    State  state,
    char   side)
{
    CodeSet codeset = side ? state->GR_codeset : state->GL_codeset;
    if (codeset) {
	int i;
	XlcCharSet charset;
	for (i = 0; i < codeset->num_charsets; i++) {
	    charset = codeset->charset_list[i];
	    if (*charset->ct_sequence != '\0')
		return charset;
	}
	return *(codeset->charset_list);
    }
    return (XlcCharSet) NULL;
}

static int
def_mbstowcs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    const char *src;
    wchar_t *dst = (wchar_t *) *to;
    State state = (State) conv->state;
    int unconv = 0;

    if (from == NULL || *from == NULL)
	return 0;

    src = (const char *) *from;

    while (*from_left && *to_left) {
	(*from_left)--;
	if (state->MBtoWC (state, src++, dst)) {
	    dst++;
	    (*to_left)--;
	} else {
	    unconv++;
	}
    }
    *from = (XPointer) src;
    *to = (XPointer) dst;
    return unconv;
}

static int
def_wcstombs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    const wchar_t *src;
    char  *dst = (char *) *to;
    State state = (State) conv->state;
    char ch[MB_LEN_MAX];
    int unconv = 0;

    if (from == NULL || *from == NULL)
	return 0;

    src = (const wchar_t *) *from;

    while (*from_left && *to_left) {
	(*from_left)--;
	if (state->WCtoMB (state, *src++, ch)) {
	    *dst++ = *ch;
	    (*to_left)--;
	} else {
	    unconv++;
	}
    }
    *from = (XPointer) src;
    *to = (XPointer) dst;
    return unconv;
}

static int
mbstostr(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    const char *src;
    char *dst = (char *) *to;
    CodeSet codeset;
    State state = (State) conv->state;
    char ch;
    int unconv = 0;

    if (from == NULL || *from == NULL)
	return 0;

    src = (const char *) *from;

    while (*from_left && *to_left) {
	ch = *src++;
	(*from_left)--;

	codeset = (ch & GR) ? state->GR_codeset : state->GL_codeset;
	if (codeset && codeset->string_encoding) {
	    *dst++ = ch;
	    (*to_left)--;
	} else {
	    unconv++;
	}
    }
    *from = (XPointer) src;
    *to = (XPointer) dst;
    return unconv;
}

static int
wcstostr(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    const wchar_t *src;
    char *dst = (char *) *to;
    CodeSet codeset;
    State state = (State) conv->state;
    char ch[MB_LEN_MAX];
    int unconv = 0;

    if (from == NULL || *from == NULL)
	return 0;

    src = (const wchar_t *) *from;

    while (*from_left && *to_left) {
	(*from_left)--;
	if (state->WCtoMB (state, *src++, ch)) {
	    codeset = (*ch & GR) ? state->GR_codeset : state->GL_codeset;
	    if (codeset && codeset->string_encoding) {
		*dst++ = *ch;
		(*to_left)--;
	    } else {
		unconv++;
	    }
	} else {
	    unconv++;
	}
    }
    *from = (XPointer) src;
    *to = (XPointer) dst;
    return unconv;
}

static int
mbstocs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    const char *src;
    char *dst = (char *) *to;
    int length;
    State state = (State) conv->state;
    char cur_side;
    int unconv = 0;

    if (from == NULL || *from == NULL)
	return 0;

    src = (const char *) *from;
    length = min(*from_left, *to_left);

    cur_side = *src & GR;
    while (length) {
	if ((char) (*src & GR) != cur_side)
	    break;
	*dst++ = *src++;
	length--;
    }

    if (num_args > 0) {
	XlcCharSet      charset = get_charset(state, cur_side);
	if (charset) {
	    *((XlcCharSet *) args[0]) = charset;
	} else {
	    dst = *to;
	    unconv = -1;
	}
    }
    *from_left -= src - (char *) *from;
    *to_left -= dst - (char *) *to;
    *from = (XPointer) src;
    *to = (XPointer) dst;
    return unconv;
}

static int
wcstocs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    const wchar_t *src;
    char *dst = (char *) *to;
    State state = (State) conv->state;
    char cur_side = 0, ch[MB_LEN_MAX];
    int unconv = 0;
    Bool found = False;

    if (from == NULL || *from == NULL)
	return 0;

    src = (const wchar_t *) *from;

    while (*from_left) {
	if ((found = state->WCtoMB (state, *src, ch)))
	    break;
	unconv++;
	src++;
	(*from_left)--;
    }

    if (found) {
	cur_side = *ch & GR;
	while (*from_left && *to_left) {
	    (*from_left)--;
	    if (state->WCtoMB (state, *src++, ch)) {
		if ((char) (*ch & GR) != cur_side) {
		    src--;
		    (*from_left)++;
		    break;
		} else {
		    *dst++ = *ch;
		    (*to_left)--;
		}
	    } else {
		unconv++;
	    }
	}
    } else {
	unconv++;
    }

    if (num_args > 0) {
	XlcCharSet charset = get_charset(state, cur_side);
	if (charset) {
	    *((XlcCharSet *) args[0]) = charset;
	} else {
	    unconv = -1;
	}
    }
    *from = (XPointer) src;
    *to = (XPointer) dst;
    return unconv;
}

static int
cstombs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    const char *src;
    char *dst = (char *) *to;
    CodeSet codeset;
    XlcCharSet charset;
    State state = (State) conv->state;
    unsigned char cur_side = 0;
    int i;
    Bool found = False;

    if (from == NULL || *from == NULL)
	return 0;

    src = (const char *) *from;

    if (num_args > 0) {
	charset = (XlcCharSet) args[0];
	if (charset == NULL)
	    return -1;
    } else {
	return -1;
    }

    if ((charset->side == XlcGL) || (charset->side == XlcGLGR)) {
	codeset = state->GL_codeset;
	if (codeset) {
	    for (i = 0; i < codeset->num_charsets; i++)
		if (charset == codeset->charset_list[i]) {
		    found = True;
		    cur_side = 0;
		    break;
		}
	}
    }
    if (!found && ((charset->side == XlcGR) || (charset->side == XlcGLGR))) {
	codeset = state->GR_codeset;
	if (codeset) {
	    for (i = 0; i < codeset->num_charsets; i++)
		if (charset == codeset->charset_list[i]) {
		    found = True;
		    cur_side = GR;
		    break;
		}
	}
    }
    if (found) {
	int length = min(*from_left, *to_left);
	while (length) {
	    *dst++ = *src++ | cur_side;
	    length--;
	}
    } else {
	return -1;
    }

    *from_left -= src - (char *) *from;
    *to_left -= dst - (char *) *to;
    *from = (XPointer) src;
    *to = (XPointer) dst;
    return 0;
}

static int
cstowcs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    const char *src;
    wchar_t *dst = (wchar_t *) *to;
    CodeSet codeset;
    XlcCharSet charset;
    State state = (State) conv->state;
    Bool found = False;
    int i, unconv = 0;
    unsigned char cur_side = 0;

    if (from == NULL || *from == NULL)
	return 0;

    src = (const char *) *from;

    if (num_args > 0) {
	charset = (XlcCharSet) args[0];
	if (charset == NULL)
	    return -1;
    } else {
	return -1;
    }

    if ((charset->side == XlcGL) || (charset->side == XlcGLGR)) {
	codeset = state->GL_codeset;
	if (codeset) {
	    for (i = 0; i < codeset->num_charsets; i++)
		if (charset == codeset->charset_list[i]) {
		    found = True;
		    cur_side = 0;
		    break;
		}
	}
    }
    if (!found && ((charset->side == XlcGR) || (charset->side == XlcGLGR))) {
	codeset = state->GR_codeset;
	if (codeset) {
	    for (i = 0; i < codeset->num_charsets; i++)
		if (charset == codeset->charset_list[i]) {
		    found = True;
		    cur_side = GR;
		    break;
		}
	}
    }
    if (found) {
	char ch;
	while (*from_left && *to_left) {
	    ch = *src++ | cur_side;
	    (*from_left)--;
	    if (state->MBtoWC (state, &ch, dst)) {
		dst++;
		(*to_left)--;
	    } else {
		unconv++;
	    }
	}
    } else {
	return -1;
    }
    *from = (XPointer) src;
    *to = (XPointer) dst;
    return unconv;
}

static int
strtombs(
    XlcConv conv,
    XPointer *from,
    int *from_left,
    XPointer *to,
    int *to_left,
    XPointer *args,
    int num_args)
{
    const char *src;
    char *dst = (char *) *to;
    int length;

    if (from == NULL || *from == NULL)
	return 0;

    src = (const char *) *from;
    length = min(*from_left, *to_left);
    while (length) {
	*dst++ = *src++;
	length--;
    }

    *from_left -= src - (char *) *from;
    *to_left -= dst - (char *) *to;
    *from = (XPointer) src;
    *to = (XPointer) dst;
    return 0;
}

static void
close_converter(
    XlcConv conv)
{

    Xfree(conv->state);
    Xfree(conv);
}

static XlcConv
create_conv(
    XLCd lcd,
    XlcConvMethods methods)
{
    XlcConv conv;
    State state;

    conv = Xcalloc(1, sizeof(XlcConvRec));
    if (conv == NULL)
	return (XlcConv) NULL;

    state = Xmalloc(sizeof(StateRec));
    if (state == NULL) {
	close_converter(conv);
	return (XlcConv) NULL;
    }
    state->GL_codeset = XLC_GENERIC(lcd, initial_state_GL);
    state->GR_codeset = XLC_GENERIC(lcd, initial_state_GR);
    state->wc_mask = (1 << XLC_GENERIC(lcd, wc_shift_bits)) - 1;
    state->wc_encode_mask = XLC_GENERIC(lcd, wc_encode_mask);

#ifdef STDCVT
    if (XLC_GENERIC(lcd, use_stdc_env) == True)
	state->MBtoWC = &MBtoWCstd;
    else
#endif
	state->MBtoWC = &MBtoWCdef;

#ifdef STDCVT
    if (XLC_GENERIC(lcd, use_stdc_env) == True)
	state->WCtoMB = &WCtoMBstd;
    else
#endif
	state->WCtoMB = &WCtoMBdef;

    conv->methods = methods;
    conv->state = (XPointer) state;

    return conv;
}

static XlcConvMethodsRec mbstowcs_methods = {
    close_converter,
    def_mbstowcs,
    NULL
};

static XlcConv
open_mbstowcs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &mbstowcs_methods);
}

static XlcConvMethodsRec mbstostr_methods = {
    close_converter,
    mbstostr,
    NULL
};

static XlcConv
open_mbstostr(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &mbstostr_methods);
}

static XlcConvMethodsRec mbstocs_methods = {
    close_converter,
    mbstocs,
    NULL
};

static XlcConv
open_mbstocs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &mbstocs_methods);
}

static XlcConvMethodsRec wcstombs_methods = {
    close_converter,
    def_wcstombs,
    NULL
};

static XlcConv
open_wcstombs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &wcstombs_methods);
}

static XlcConvMethodsRec wcstostr_methods = {
    close_converter,
    wcstostr,
    NULL
};

static XlcConv
open_wcstostr(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &wcstostr_methods);
}

static XlcConvMethodsRec wcstocs_methods = {
    close_converter,
    wcstocs,
    NULL
};

static XlcConv
open_wcstocs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &wcstocs_methods);
}

static XlcConvMethodsRec strtombs_methods = {
    close_converter,
    strtombs,
    NULL
};

static XlcConv
open_strtombs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &strtombs_methods);
}

static XlcConvMethodsRec cstombs_methods = {
    close_converter,
    cstombs,
    NULL
};

static XlcConv
open_cstombs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &cstombs_methods);
}

static XlcConvMethodsRec cstowcs_methods = {
    close_converter,
    cstowcs,
    NULL
};

static XlcConv
open_cstowcs(
    XLCd from_lcd,
    const char *from_type,
    XLCd to_lcd,
    const char *to_type)
{
    return create_conv(from_lcd, &cstowcs_methods);
}

XLCd
_XlcDefaultLoader(
    const char *name)
{
    XLCd lcd;

    lcd = _XlcCreateLC(name, _XlcGenericMethods);
    if (lcd == NULL)
	return lcd;

    if (XLC_PUBLIC(lcd, mb_cur_max) != 1){
        _XlcDestroyLC(lcd);
        return (XLCd) NULL;
    }

    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar, open_mbstowcs);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNString, open_mbstostr);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNCharSet, open_mbstocs);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNChar, open_mbstocs);

    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNMultiByte, open_wcstombs);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNString, open_wcstostr);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNCharSet, open_wcstocs);

    _XlcSetConverter(lcd, XlcNString, lcd, XlcNMultiByte, open_strtombs);
    _XlcSetConverter(lcd, XlcNString, lcd, XlcNWideChar, open_mbstowcs);

    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNMultiByte, open_cstombs);
    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNWideChar, open_cstowcs);

    _XlcAddUtf8Converters(lcd);

    return lcd;
}
