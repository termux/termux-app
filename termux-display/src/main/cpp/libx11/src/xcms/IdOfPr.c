
/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 *
 * This file is a component of an X Window System-specific implementation
 * of Xcms based on the TekColor Color Management System.  Permission is
 * hereby granted to use, copy, modify, sell, and otherwise distribute this
 * software and its documentation for any purpose and without fee, provided
 * that this copyright, permission, and disclaimer notice is reproduced in
 * all copies of this software and in supporting documentation.  TekColor
 * is a trademark of Tektronix, Inc.
 *
 * Tektronix makes no representation about the suitability of this software
 * for any purpose.  It is provided "as is" and with all faults.
 *
 * TEKTRONIX DISCLAIMS ALL WARRANTIES APPLICABLE TO THIS SOFTWARE,
 * INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL TEKTRONIX BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA, OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR THE PERFORMANCE OF THIS SOFTWARE.
 *
 *
 *	NAME
 *		XcmsIdOfPr.c
 *
 *	DESCRIPTION
 *		Source for XcmsFormatOfPrefix()
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"


/*
 *	NAME
 *		XcmsFormatOfPrefix
 *
 *	SYNOPSIS
 */
XcmsColorFormat
XcmsFormatOfPrefix(char *prefix)
/*
 *	DESCRIPTION
 *		Returns the Color Space ID for the specified prefix
 *		if the color space is found in the Color Conversion
 *		Context.
 *
 *	RETURNS
 *		Color Space ID if found; zero otherwise.
 */
{
    XcmsColorSpace	**papColorSpaces;
    char		string_buf[64];
    char		*string_lowered;
    size_t		len;

    /*
     * While copying prefix to string_lowered, convert to lowercase
     */
    if ((len = strlen(prefix)) >= sizeof(string_buf)) {
	string_lowered = Xmalloc(len+1);
    } else {
	string_lowered = string_buf;
    }
    _XcmsCopyISOLatin1Lowered(string_lowered, prefix);

    /*
     * First try Device-Independent color spaces
     */
    papColorSpaces = _XcmsDIColorSpaces;
    if (papColorSpaces != NULL) {
	while (*papColorSpaces != NULL) {
	    if (strcmp((*papColorSpaces)->prefix, string_lowered) == 0) {
		if (len >= sizeof(string_buf)) Xfree(string_lowered);
		return((*papColorSpaces)->id);
	    }
	    papColorSpaces++;
	}
    }

    /*
     * Next try Device-Dependent color spaces
     */
    papColorSpaces = _XcmsDDColorSpaces;
    if (papColorSpaces != NULL) {
	while (*papColorSpaces != NULL) {
	    if (strcmp((*papColorSpaces)->prefix, string_lowered) == 0) {
		if (len >= sizeof(string_buf)) Xfree(string_lowered);
		return((*papColorSpaces)->id);
	    }
	    papColorSpaces++;
	}
    }

    if (len >= sizeof(string_buf)) Xfree(string_lowered);
    return(XcmsUndefinedFormat);
}
