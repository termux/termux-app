
/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 *
 * This file is a component of an X Window System-specific implementation
 * of XCMS based on the TekColor Color Management System.  Permission is
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
 *		CIELuvWpAj.c
 *
 *	DESCRIPTION
 *		This file contains routine(s) that support white point
 *		adjustment of color specifications in the CIE CIELuv.color
 *		space.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"



/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELuvWhiteShiftColors
 *
 *	SYNOPSIS
 */
Status
XcmsCIELuvWhiteShiftColors(
    XcmsCCC ccc,
    XcmsColor *pWhitePtFrom,
    XcmsColor *pWhitePtTo,
    XcmsColorFormat destSpecFmt,
    XcmsColor *pColors_in_out,
    unsigned int nColors,
    Bool *pCompressed)
/*
 *	DESCRIPTION
 *		Adjusts color specifications in an array of XcmsColor
 *		structures for white point differences.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded without gamut compression,
 *		XcmsSuccessWithCompression if succeeded with
 *			gamut compression.
 */
{
    if (pWhitePtFrom == NULL || pWhitePtTo == NULL || pColors_in_out == NULL) {
	return(XcmsFailure);
    }

    /*
     * Convert to CIELuv using pWhitePtFrom
     *    We can ignore return value for compression because we are converting
     *    to XcmsCIELuvFormat which is device-independent, not device-dependent.
     */
    if (_XcmsConvertColorsWithWhitePt(ccc, pColors_in_out, pWhitePtFrom,
	    nColors, XcmsCIELuvFormat, pCompressed) == XcmsFailure) {
	return(XcmsFailure);
    }

    /*
     * Convert from CIELuv to destSpecFmt using pWhitePtTo
     */
    return(_XcmsConvertColorsWithWhitePt(ccc, pColors_in_out, pWhitePtTo,
	    nColors, destSpecFmt, pCompressed));
}
