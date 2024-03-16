
/*
 * Copyright 1990, 1991 by OMRON Corporation, NTT Software Corporation,
 *                      and Nippon Telegraph and Telephone Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of OMRON, NTT Software, and NTT
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. OMRON, NTT Software,
 * and NTT make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * OMRON, NTT SOFTWARE, AND NTT, DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL OMRON, NTT SOFTWARE, OR NTT, BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *	Authors: Li Yuhong		OMRON Corporation
 *		 Tetsuya Kato		NTT Software Corporation
 *		 Hiroshi Kuribayashi	OMRON Corporation
 *
 */
/*

Copyright 1987,1998  The Open Group

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
#include <X11/Xlocale.h>
#include <X11/Xos.h>
#include "XlcPubI.h"

#define MAXLOCALE	64	/* buffer size of locale name */


#if defined(__APPLE__) || defined(__CYGWIN__)
char *
_Xsetlocale(
    int           category,
    _Xconst char  *name
)
{
    return setlocale(category, name);
}
#endif /* __APPLE__ || __CYGWIN__ */

/*
 * _XlcMapOSLocaleName is an implementation dependent routine that derives
 * the LC_CTYPE locale name as used in the sample implementation from that
 * returned by setlocale.
 *
 * Should match the code in Xt ExtractLocaleName.
 *
 * This function name is a bit of a misnomer. Even the siname parameter
 * name is a misnomer. On most modern operating systems this function is
 * a no-op, simply returning the osname; but on older operating systems
 * like Ultrix, or HPUX 9.x and earlier, when you set LANG=german.88591
 * then the string returned by setlocale(LC_ALL, "") will look something
 * like: "german.88591 german.88591 ... german.88591". Then this function
 * will pick out the LC_CTYPE component and return a pointer to that.
 */

char *
_XlcMapOSLocaleName(
    char *osname,
    char *siname)
{
#if defined(hpux) || defined(CSRG_BASED) || defined(sun) || defined(SVR4) || defined(sgi) || defined(__osf__) || defined(AIXV3) || defined(ultrix) || defined(WIN32) || defined(__UNIXOS2__) || defined(linux)
# ifdef hpux
#  ifndef _LastCategory
   /* HPUX 9 and earlier */
#   define SKIPCOUNT 2
#   define STARTCHAR ':'
#   define ENDCHAR ';'
#  else
   /* HPUX 10 */
#   define ENDCHAR ' '
#  endif
# else
#  ifdef ultrix
#   define SKIPCOUNT 2
#   define STARTCHAR '\001'
#   define ENDCHAR '\001'
#  else
#   if defined(WIN32) || defined(__UNIXOS2__)
#    define SKIPCOUNT 1
#    define STARTCHAR '='
#    define ENDCHAR ';'
#    define WHITEFILL
#   else
#    if defined(__osf__) || (defined(AIXV3) && !defined(AIXV4))
#     define STARTCHAR ' '
#     define ENDCHAR ' '
#    else
#     if defined(linux)
#      define STARTSTR "LC_CTYPE="
#      define ENDCHAR ';'
#     else
#      if !defined(sun) || defined(SVR4)
#       define STARTCHAR '/'
#       define ENDCHAR '/'
#      endif
#     endif
#    endif
#   endif
#  endif
# endif

    char           *start;
    char           *end;
    int             len;
# ifdef SKIPCOUNT
    int		    n;
# endif

    start = osname;
# ifdef SKIPCOUNT
    for (n = SKIPCOUNT;
	 --n >= 0 && start && (start = strchr (start, STARTCHAR));
	 start++)
	;
    if (!start)
	start = osname;
# endif
# ifdef STARTCHAR
    if (start && (start = strchr (start, STARTCHAR)))
# elif  defined (STARTSTR)
    if (start && (start = strstr (start,STARTSTR)))
# endif
    {
# ifdef STARTCHAR
	start++;
# elif defined (STARTSTR)
	start += strlen(STARTSTR);
# endif
	if ((end = strchr (start, ENDCHAR))) {
	    len = end - start;
	    if (len >= MAXLOCALE)
		len = MAXLOCALE - 1;
	    strncpy(siname, start, (size_t) len);
	    *(siname + len) = '\0';
# ifdef WHITEFILL
	    for (start = siname; start = strchr(start, ' '); )
		*start++ = '-';
# endif
	    return siname;
	} else  /* if no ENDCHAR is found we are at the end of the line */
	    return start;
    }
# ifdef WHITEFILL
    if (strchr(osname, ' ')) {
	len = strlen(osname);
	if (len >= MAXLOCALE - 1)
	    len = MAXLOCALE - 1;
	strncpy(siname, osname, len);
	*(siname + len) = '\0';
	for (start = siname; start = strchr(start, ' '); )
	    *start++ = '-';
	return siname;
    }
# endif
# undef STARTCHAR
# undef ENDCHAR
# undef WHITEFILL
#endif
    return osname;
}

