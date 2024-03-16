/******************************************************************

              Copyright 1991, 1992 by Fuji Xerox Co.,Ltd.
	      Copyright 1993, 1994 by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Fuji Xerox Co.,Ltd.
, and that the name of FUJITSU LIMITED not be used in advertising or
publicity pertaining to distribution of the software without specific,
 written prior permission.
Fuji Xerox Co.,Ltd. , and FUJITSU LIMITED makes no representations about
the suitability of this software for any purpose.
It is provided "as is" without express or implied warranty.

FUJI XEROX CO.,LTD. AND FUJITSU LIMITED DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL FUJI XEROX CO.,LTD.
AND FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

  Author:   Kazunori Nishihara, Fuji Xerox Co.,Ltd.
                                kaz@ssdev.ksp.fujixerox.co.jp
  Modifier: Takashi Fujiwara    FUJITSU LIMITED
                                fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/
/* 2000 Modifier: Ivan Pascal	The XFree86 Project.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"
#include "XlcPubI.h"

#ifdef XKB
/*
 * rather than just call _XLookupString (i.e. the pre-XKB XLookupString)
 * do this because with XKB the event may have some funky modifiers that
 * _XLookupString doesn't grok.
 */
#include "XKBlib.h"
#define	XLOOKUPSTRING lookup_string
#else
#define	XLOOKUPSTRING XLookupString
#endif

typedef unsigned int ucs4_t;

typedef int (*ucstocsConvProc)(
    XPointer,
    unsigned char *,
    ucs4_t,
    int
);

struct SubstRec {
    const char encoding_name[8];
    const char charset_name[12];
};

static const struct SubstRec SubstTable[] = {
    {"STRING", "ISO8859-1"},
    {"TIS620", "TIS620-0"},
    {"UTF-8",  "ISO10646-1"}
};
#define num_substitute (sizeof SubstTable / sizeof SubstTable[0])

/*
 * Given the name of a charset, returns the pointer to convertors
 * from UCS char to specified charset char.
 * This converter is needed for _XimGetCharCode subroutine.
 */
XPointer
_XimGetLocaleCode (
    _Xconst char*	encoding_name)
{
    XPointer cvt = _Utf8GetConvByName(encoding_name);
    if (!cvt && encoding_name) {
       int i;
       for (i = 0; i < num_substitute; i++)
           if (!strcmp(encoding_name, SubstTable[i].encoding_name))
               return _Utf8GetConvByName(SubstTable[i].charset_name);
    }
    return cvt;
}

/*
 * Returns the locale dependent representation of a keysym.
 * The locale's encoding is passed in form of pointer to UCS converter.
 * The resulting multi-byte sequence is placed starting at buf (a buffer
 * with nbytes bytes, nbytes should be >= 8) and is NUL terminated.
 * Returns the length of the resulting multi-byte sequence, excluding the
 * terminating NUL byte. Return 0 if the keysym is not representable in the
 * locale
 */
/*ARGSUSED*/
int
_XimGetCharCode (
    XPointer            ucs_conv,
    KeySym 		keysym,
    unsigned char*	buf,
    int 		nbytes)
{
    int count = 0;
    ucstocsConvProc cvt = (ucstocsConvProc) ucs_conv;
    ucs4_t ucs4;

    if (keysym < 0x80) {
        buf[0] = (char) keysym;
        count = 1;
    } else if (cvt) {
        ucs4 = KeySymToUcs4(keysym);
        if (ucs4)
            count = (*cvt)((XPointer)NULL, buf, ucs4, nbytes);
    }

    if (count < 0)
       	count = 0;
    if (count>nbytes)
        return nbytes;
    if (count<nbytes)
        buf[count]= '\0';
    return count;
}

#ifdef XKB
static int lookup_string(
    XKeyEvent*		event,
    char*		buffer,
    int			nbytes,
    KeySym*		keysym,
    XComposeStatus*	status)
{
    int ret;
    unsigned ctrls = XkbGetXlibControls (event->display);
    XkbSetXlibControls (event->display,
			XkbLC_ForceLatin1Lookup, XkbLC_ForceLatin1Lookup);
    ret = XLookupString(event, (char *)buffer, nbytes, keysym, status);
    XkbSetXlibControls (event->display,
			XkbLC_ForceLatin1Lookup, ctrls);
    return ret;
}
#endif

#define BUF_SIZE (20)

int
_XimLookupMBText(
    Xic			 ic,
    XKeyEvent*		event,
    char*		buffer,
    int			nbytes,
    KeySym*		keysym,
    XComposeStatus*	status)
{
    int count;
    KeySym symbol;
    Status	dummy;
    Xim	im = (Xim)ic->core.im;
    XimCommonPrivateRec* private = &im->private.common;
    unsigned char look[BUF_SIZE];
    ucs4_t ucs4;

    /* force a latin-1 lookup for compatibility */
    count = XLOOKUPSTRING(event, (char *)buffer, nbytes, &symbol, status);
    if (keysym != NULL) *keysym = symbol;
    if ((nbytes == 0) || (symbol == NoSymbol)) return count;

    if (count > 1) {
	memcpy(look, (char *)buffer,count);
	look[count] = '\0';
	if ((count = im->methods->ctstombs(ic->core.im,
				(char*) look, count,
				buffer, nbytes, &dummy)) < 0) {
	    count = 0;
	}
    } else if ((count == 0) ||
	       (count == 1 && (symbol > 0x7f && symbol < 0xff00))) {

        XPointer from = (XPointer) &ucs4;
        XPointer to =   (XPointer) look;
        int from_len = 1;
        int to_len = BUF_SIZE;
        XPointer args[1];
        XlcCharSet charset;
        args[0] = (XPointer) &charset;
        ucs4 = (ucs4_t) KeySymToUcs4(symbol);
        if (!ucs4)
            return 0;

	if (_XlcConvert(private->ucstoc_conv,
                        &from, &from_len, &to, &to_len,
                        args, 1 ) != 0) {
	    count = 0;
	} else {
            from = (XPointer) look;
            to =   (XPointer) buffer;
            from_len = BUF_SIZE - to_len;
            to_len = nbytes;
            args[0] = (XPointer) charset;
	    if (_XlcConvert(private->cstomb_conv,
                            &from, &from_len, &to, &to_len,
			    args, 1 ) != 0) {
		count = 0;
	    } else {
                count = nbytes - to_len;
	    }
	}
    }
    /* FIXME:
     * we should make sure that if the character is a Latin1 character
     * and it's on the right side, and we're in a non-Latin1 locale
     * that this is a valid Latin1 character for this locale.
     */
    return count;
}

int
_XimLookupWCText(
    Xic			 ic,
    XKeyEvent*		event,
    wchar_t*		buffer,
    int			nbytes,
    KeySym*		keysym,
    XComposeStatus*	status)
{
    int count;
    KeySym symbol;
    Status	dummy;
    Xim	im = (Xim)ic->core.im;
    XimCommonPrivateRec* private = &im->private.common;
    unsigned char look[BUF_SIZE];
    ucs4_t ucs4;

    /* force a latin-1 lookup for compatibility */
    count = XLOOKUPSTRING(event, (char *)look, nbytes, &symbol, status);
    if (keysym != NULL) *keysym = symbol;
    if ((nbytes == 0) || (symbol == NoSymbol)) return count;

    if (count > 1) {
	if ((count = im->methods->ctstowcs(ic->core.im,
				(char*) look, count,
				buffer, nbytes, &dummy)) < 0) {
	    count = 0;
	}
    } else if ((count == 0) ||
	       (count == 1 && (symbol > 0x7f && symbol < 0xff00))) {

        XPointer from = (XPointer) &ucs4;
        XPointer to =   (XPointer) look;
        int from_len = 1;
        int to_len = BUF_SIZE;
        XPointer args[1];
        XlcCharSet charset;
        args[0] = (XPointer) &charset;
        ucs4 = (ucs4_t) KeySymToUcs4(symbol);
        if (!ucs4)
            return 0;

	if (_XlcConvert(private->ucstoc_conv,
                        &from, &from_len, &to, &to_len,
                        args, 1 ) != 0) {
	    count = 0;
	} else {
            from = (XPointer) look;
            to =   (XPointer) buffer;
            from_len = BUF_SIZE - to_len;
            to_len = nbytes;
            args[0] = (XPointer) charset;

	    if (_XlcConvert(private->cstowc_conv,
                            &from, &from_len, &to, &to_len,
			    args, 1 ) != 0) {
		count = 0;
	    } else {
                count = nbytes - to_len;
	    }
	}
    } else
	/* FIXME:
	 * we should make sure that if the character is a Latin1 character
	 * and it's on the right side, and we're in a non-Latin1 locale
	 * that this is a valid Latin1 character for this locale.
	 */
	buffer[0] = look[0];

    return count;
}

int
_XimLookupUTF8Text(
    Xic			 ic,
    XKeyEvent*		event,
    char*		buffer,
    int			nbytes,
    KeySym*		keysym,
    XComposeStatus*	status)
{
    int count;
    KeySym symbol;
    Status	dummy;
    Xim	im = (Xim)ic->core.im;
    XimCommonPrivateRec* private = &im->private.common;
    unsigned char look[BUF_SIZE];
    ucs4_t ucs4;

    /* force a latin-1 lookup for compatibility */
    count = XLOOKUPSTRING(event, (char *)buffer, nbytes, &symbol, status);
    if (keysym != NULL) *keysym = symbol;
    if ((nbytes == 0) || (symbol == NoSymbol)) return count;

    if (count > 1) {
	memcpy(look, (char *)buffer,count);
	look[count] = '\0';
	if ((count = im->methods->ctstoutf8(ic->core.im,
				(char*) look, count,
				buffer, nbytes, &dummy)) < 0) {
	    count = 0;
	}
    } else if ((count == 0) ||
	       (count == 1 && (symbol > 0x7f && symbol < 0xff00))) {

        XPointer from = (XPointer) &ucs4;
        int from_len = 1;
        XPointer to = (XPointer) buffer;
        int to_len = nbytes;

        ucs4 = (ucs4_t) KeySymToUcs4(symbol);
        if (!ucs4)
            return 0;

	if (_XlcConvert(private->ucstoutf8_conv,
                        &from, &from_len, &to, &to_len,
                        NULL, 0) != 0) {
	    count = 0;
	} else {
            count = nbytes - to_len;
	}
    }
    /* FIXME:
     * we should make sure that if the character is a Latin1 character
     * and it's on the right side, and we're in a non-Latin1 locale
     * that this is a valid Latin1 character for this locale.
     */
    return count;
}
