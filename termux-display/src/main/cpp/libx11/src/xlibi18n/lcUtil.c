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
#include <X11/Xlib.h>
#include "XlcPublic.h"

/* Don't use <ctype.h> here because it is locale dependent. */

#define set_toupper(ch) \
  if (ch >= 'a' && ch <= 'z') \
    ch = (unsigned char) (ch - 'a' + 'A');

/* Compares two ISO 8859-1 strings, ignoring case of ASCII letters.
   Like strcasecmp in an ASCII locale. */
int
_XlcCompareISOLatin1(
    const char *str1,
    const char *str2)
{
    unsigned char ch1, ch2;

    for ( ; ; str1++, str2++) {
	ch1 = (unsigned char) *str1;
	ch2 = (unsigned char) *str2;
	if (ch1 == '\0' || ch2 == '\0')
	    break;
	set_toupper(ch1);
	set_toupper(ch2);
	if (ch1 != ch2)
	    break;
    }

    return ch1 - ch2;
}

/* Compares two ISO 8859-1 strings, at most len bytes of each, ignoring
   case of ASCII letters. Like strncasecmp in an ASCII locale. */
int
_XlcNCompareISOLatin1(
    const char *str1,
    const char *str2,
    int len)
{
    unsigned char ch1, ch2;

    for ( ; ; str1++, str2++, len--) {
	if (len == 0)
	    return 0;
	ch1 = (unsigned char) *str1;
	ch2 = (unsigned char) *str2;
	if (ch1 == '\0' || ch2 == '\0')
	    break;
	set_toupper(ch1);
	set_toupper(ch2);
	if (ch1 != ch2)
	    break;
    }

    return ch1 - ch2;
}
