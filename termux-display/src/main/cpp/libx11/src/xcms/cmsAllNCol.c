
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
 *		XcmsAlNCol.c
 *
 *	DESCRIPTION
 *		Source for XcmsAllocNamedColor
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
 *		XcmsAllocNamedColor -
 *
 *	SYNOPSIS
 */
Status
XcmsAllocNamedColor (
    Display *dpy,
    Colormap cmap,
    _Xconst char *colorname,
    XcmsColor *pColor_scrn_return,
    XcmsColor *pColor_exact_return,
    XcmsColorFormat result_format)
/*
 *	DESCRIPTION
 *		Finds the color specification associated with the color
 *		name in the Device-Independent Color Name Database, then
 *		converts that color specification to an RGB format.  This
 *		RGB value is then used in a call to XAllocColor to allocate
 *		a read-only color cell.
 *
 *	RETURNS
 *		0 if failed to parse string or find any entry in the database.
 *		1 if succeeded in converting color name to XcmsColor.
 *		2 if succeeded in converting color name to another color name.
 *
 */
{
    long nbytes;
    xAllocNamedColorReply rep;
    xAllocNamedColorReq *req;
    XColor hard_def;
    XColor exact_def;
    Status retval1 = 1;
    Status retval2 = XcmsSuccess;
    XcmsColor tmpColor;
    XColor XColor_in_out;
    XcmsCCC ccc;

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
     * 1. Convert string to a XcmsColor using Xcms and i18n mechanism
     */
    if ((retval1 = _XcmsResolveColorString(ccc, &colorname,
	    &tmpColor, result_format)) == XcmsFailure) {
	return(XcmsFailure);
    }
    if (retval1 == _XCMS_NEWNAME) {
	goto PassToServer;
    }
    memcpy((char *)pColor_exact_return, (char *)&tmpColor, sizeof(XcmsColor));

    /*
     * 2. Convert tmpColor to RGB
     *	Assume pColor_exact_return is now adjusted to Client White Point
     */
    if ((retval2 = XcmsConvertColors(ccc, &tmpColor,
	    1, XcmsRGBFormat, (Bool *) NULL)) == XcmsFailure) {
	return(XcmsFailure);
    }

    /*
     * 3. Convert to XColor and call XAllocColor
     */
    _XcmsRGB_to_XColor(&tmpColor, &XColor_in_out, 1);
    if (XAllocColor(ccc->dpy, cmap, &XColor_in_out) == 0) {
	return(XcmsFailure);
    }

    /*
     * 4. pColor_scrn_return
     *
     * Now convert to the target format.
     *    We can ignore the return value because we're already in a
     *    device-dependent format.
     */
    _XColor_to_XcmsRGB(ccc, &XColor_in_out, pColor_scrn_return, 1);
    if (result_format != XcmsRGBFormat) {
	if (result_format == XcmsUndefinedFormat) {
	    result_format = pColor_exact_return->format;
	}
	if (XcmsConvertColors(ccc, pColor_scrn_return, 1, result_format,
		(Bool *) NULL) == XcmsFailure) {
	    return(XcmsFailure);
	}
    }

    return(retval1 > retval2 ? retval1 : retval2);

PassToServer:
    /*
     * All previous methods failed, so lets pass it to the server
     * for parsing.
     */
    dpy = ccc->dpy;
    LockDisplay(dpy);
    GetReq(AllocNamedColor, req);

    req->cmap = cmap;
    nbytes = req->nbytes = (CARD16) strlen(colorname);
    req->length += (nbytes + 3) >> 2; /* round up to mult of 4 */

    _XSend(dpy, colorname, nbytes);
       /* _XSend is more efficient that Data, since _XReply follows */

    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay(dpy);
        SyncHandle();
        return (0);
    }

    exact_def.red = rep.exactRed;
    exact_def.green = rep.exactGreen;
    exact_def.blue = rep.exactBlue;

    hard_def.red = rep.screenRed;
    hard_def.green = rep.screenGreen;
    hard_def.blue = rep.screenBlue;

    exact_def.pixel = hard_def.pixel = rep.pixel;

    UnlockDisplay(dpy);
    SyncHandle();

    /*
     * Now convert to the target format.
     */
    _XColor_to_XcmsRGB(ccc, &exact_def, pColor_exact_return, 1);
    _XColor_to_XcmsRGB(ccc, &hard_def, pColor_scrn_return, 1);
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
