
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
 *		XcmsLkCol.c
 *
 *	DESCRIPTION
 *		Source for XcmsLookupColor
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"


/*
 *	NAME
 *		XcmsLookupColor -
 *
 *	SYNOPSIS
 */
Status
XcmsLookupColor (
    Display *dpy,
    Colormap cmap,
    _Xconst char *colorname,
    XcmsColor *pColor_exact_return,
    XcmsColor *pColor_scrn_return,
    XcmsColorFormat result_format)
/*
 *	DESCRIPTION
 *		The XcmsLookupColor function finds the color specification
 *		associated with a color name in the Device-Independent Color
 *		Name Database.
 *	RETURNS
 *		This function returns both the color specification found in the
 *		database (db specification) and the color specification for the
 *		color displayable by the specified screen (screen
 *		specification).  The calling routine sets the format for these
 *		returned specifications in the XcmsColor format component.
 *		If XcmsUndefinedFormat, the specification is returned in the
 *		format used to store the color in the database.
 */
{
    Status retval1 = XcmsSuccess;
    Status retval2 = XcmsSuccess;
    XcmsCCC ccc;
    register int n;
    xLookupColorReply reply;
    register xLookupColorReq *req;
    XColor def = {0,};
    XColor scr = {0,};

/*
 * 0. Check for invalid arguments.
 */
    if (dpy == NULL || colorname[0] == '\0' || pColor_scrn_return == 0
	    || pColor_exact_return == NULL) {
	return(XcmsFailure);
    }

    if ((ccc = XcmsCCCOfColormap(dpy, cmap)) == (XcmsCCC)NULL) {
	return(XcmsFailure);
    }

/*
 * 1. Convert string to a XcmsColor
 */
    if ((retval1 = _XcmsResolveColorString(ccc, &colorname,
	    pColor_exact_return, result_format)) == XcmsFailure) {
	return(XcmsFailure);
    }
    if (retval1 == _XCMS_NEWNAME) {
	goto PassToServer;
    }

/*
 * 2. pColor_scrn_return
 *	    Assume the pColor_exact_return has already been adjusted to
 *	    the Client White Point.
 *
 */
    /*
     * Convert to RGB, adjusting for white point differences if necessary.
     */
    memcpy((char *)pColor_scrn_return, (char *)pColor_exact_return,
	   sizeof(XcmsColor));
    if (pColor_scrn_return->format == XcmsRGBFormat) {
	retval2 = XcmsSuccess;
    } else if ((retval2 = XcmsConvertColors(ccc, pColor_scrn_return, 1,
	    XcmsRGBFormat, (Bool *)NULL)) == XcmsFailure) {
	return(XcmsFailure);
    }

    /*
     * Then, convert XcmsColor structure to the target specification
     *    format.  Note that we must use NULL instead of passing
     *    pCompressed.
     */

    if (result_format ==  XcmsUndefinedFormat) {
	result_format = pColor_exact_return->format;
    }
    if (result_format == XcmsRGBFormat) {
	_XcmsUnresolveColor(ccc, pColor_scrn_return);
    } else {
	_XcmsResolveColor(ccc, pColor_scrn_return);
	if (XcmsConvertColors(ccc, pColor_scrn_return, 1, result_format,
		(Bool *) NULL) == XcmsFailure) {
	    return(XcmsFailure);
	}
    }

    return(retval1 > retval2 ? retval1 : retval2);

PassToServer:
    /*
     * Xcms and i18n methods failed, so lets pass it to the server
     * for parsing.
     */

    LockDisplay(dpy);
    GetReq (LookupColor, req);
    req->cmap = cmap;
    req->nbytes = (CARD16)(n = (int)strlen(colorname));
    req->length += (n + 3) >> 2;
    Data (dpy, colorname, (long)n);
    if (!_XReply (dpy, (xReply *) &reply, 0, xTrue)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return (XcmsFailure);
	}
    def.red   = reply.exactRed;
    def.green = reply.exactGreen;
    def.blue  = reply.exactBlue;

    scr.red   = reply.screenRed;
    scr.green = reply.screenGreen;
    scr.blue  = reply.screenBlue;

    UnlockDisplay(dpy);
    SyncHandle();

    _XColor_to_XcmsRGB(ccc, &def, pColor_exact_return, 1);
    _XColor_to_XcmsRGB(ccc, &scr, pColor_scrn_return, 1);

    /*
     * Then, convert XcmsColor structure to the target specification
     *    format.  Note that we must use NULL instead of passing
     *    pCompressed.
     */

    if (result_format != XcmsRGBFormat
	    && result_format != XcmsUndefinedFormat) {
	if (XcmsConvertColors(ccc, pColor_exact_return, 1, result_format,
		(Bool *) NULL) == XcmsFailure) {
	    return(XcmsFailure);
	}
	if (XcmsConvertColors(ccc, pColor_scrn_return, 1, result_format,
		(Bool *) NULL) == XcmsFailure) {
	    return(XcmsFailure);
	}
    }

    return(XcmsSuccess);
}
