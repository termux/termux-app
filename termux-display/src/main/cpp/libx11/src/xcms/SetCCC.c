
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
 *		XcmsSetCCC.c - Color Conversion Context Setting Routines
 *
 *	DESCRIPTION
 *		Routines to set components of a Color Conversion
 *		Context structure.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcms.h"



/************************************************************************
 *									*
 *			PUBLIC INTERFACES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsSetWhitePoint
 *
 *	SYNOPSIS
 */

Status
XcmsSetWhitePoint(
    XcmsCCC ccc,
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		Sets the Client White Point in the specified CCC.
 *
 *	RETURNS
 *		Returns XcmsSuccess if succeeded; otherwise XcmsFailure.
 *
 */
{
    if (pColor == NULL || pColor->format == XcmsUndefinedFormat) {
	ccc->clientWhitePt.format = XcmsUndefinedFormat;
    } else if (pColor->format != XcmsCIEXYZFormat &&
	    pColor->format != XcmsCIEuvYFormat &&
	    pColor->format != XcmsCIExyYFormat) {
	return(XcmsFailure);
    } else {
	memcpy((char *)&ccc->clientWhitePt, (char *)pColor, sizeof(XcmsColor));
    }
    return(XcmsSuccess);
}


/*
 *	NAME
 *		XcmsSetCompressionProc
 *
 *	SYNOPSIS
 */

XcmsCompressionProc
XcmsSetCompressionProc(
    XcmsCCC ccc,
    XcmsCompressionProc compression_proc,
    XPointer client_data)
/*
 *	DESCRIPTION
 *		Set the specified CCC's compression function and client data.
 *
 *	RETURNS
 *		Returns the old compression function.
 *
 */
{
    XcmsCompressionProc old = ccc->gamutCompProc;

    ccc->gamutCompProc = compression_proc;
    ccc->gamutCompClientData = client_data;
    return(old);
}


/*
 *	NAME
 *		XcmsSetWhiteAdjustProc
 *
 *	SYNOPSIS
 */

XcmsWhiteAdjustProc
XcmsSetWhiteAdjustProc(
    XcmsCCC ccc,
    XcmsWhiteAdjustProc white_adjust_proc,
    XPointer client_data )
/*
 *	DESCRIPTION
 *		Set the specified CCC's white_adjust function and client data.
 *
 *	RETURNS
 *		Returns the old white_adjust function.
 *
 */
{
    XcmsWhiteAdjustProc old = ccc->whitePtAdjProc;

    ccc->whitePtAdjProc = white_adjust_proc;
    ccc->whitePtAdjClientData = client_data;
    return(old);
}
