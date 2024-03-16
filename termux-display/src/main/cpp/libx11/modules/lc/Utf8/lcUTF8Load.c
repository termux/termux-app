/******************************************************************

              Copyright 1993 by SunSoft, Inc.
              Copyright 1999-2000 by Bruno Haible

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the names of SunSoft, Inc. and
Bruno Haible not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.  SunSoft, Inc. and Bruno Haible make no representations
about the suitability of this software for any purpose.  It is
provided "as is" without express or implied warranty.

SunSoft Inc. AND Bruno Haible DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL SunSoft, Inc. OR Bruno Haible BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/*
 * This file contains the UTF-8 locale loader.
 *     Supports: all locales with codeset UTF-8.
 *     Platforms: all systems.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "Xlibint.h"
#include "XlcPubI.h"
#include "XlcGeneric.h"

XLCd
_XlcUtf8Loader(
    const char *name)
{
    XLCd lcd;

    lcd = _XlcCreateLC(name, _XlcGenericMethods);
    if (lcd == (XLCd) NULL)
	return lcd;

    /* The official IANA name for UTF-8 is "UTF-8" in upper case with a dash. */
    if (!XLC_PUBLIC_PART(lcd)->codeset) {
	_XlcDestroyLC(lcd);
	return (XLCd) NULL;
    }
    else if (!_XlcCompareISOLatin1(XLC_PUBLIC_PART(lcd)->codeset, "UTF-8")) {
        _XlcAddUtf8LocaleConverters(lcd);
    }
    else if (!_XlcCompareISOLatin1(XLC_PUBLIC_PART(lcd)->codeset, "GB18030")) {
        _XlcAddGB18030LocaleConverters(lcd);
    }
    else {
	_XlcDestroyLC(lcd);
	return (XLCd) NULL;
    }

    _XlcAddUtf8Converters(lcd);

    return lcd;
}
