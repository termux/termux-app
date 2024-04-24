
/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 *
 * This file is a component of an X Window System-specific implementation
 * of Xcms based on the TekColor Color Management System.  TekColor is a
 * trademark of Tektronix, Inc.  The term "TekHVC" designates a particular
 * color space that is the subject of U.S. Patent No. 4,985,853 (equivalent
 * foreign patents pending).  Permission is hereby granted to use, copy,
 * modify, sell, and otherwise distribute this software and its
 * documentation for any purpose and without fee, provided that:
 *
 * 1. This copyright, permission, and disclaimer notice is reproduced in
 *    all copies of this software and any modification thereof and in
 *    supporting documentation;
 * 2. Any color-handling application which displays TekHVC color
 *    cooordinates identifies these as TekHVC color coordinates in any
 *    interface that displays these coordinates and in any associated
 *    documentation;
 * 3. The term "TekHVC" is always used, and is only used, in association
 *    with the mathematical derivations of the TekHVC Color Space,
 *    including those provided in this file and any equivalent pathways and
 *    mathematical derivations, regardless of digital (e.g., floating point
 *    or integer) representation.
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
 *	DESCRIPTION
 *		TekHVCWpAj.c
 *
 *	DESCRIPTION
 *		This file contains routine(s) that support white point
 *		adjustment of color specifications in the TekHVC color
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
 *		XcmsTekHVCWhiteShiftColors
 *
 *	SYNOPSIS
 */
Status
XcmsTekHVCWhiteShiftColors(
    XcmsCCC ccc,
    XcmsColor *pWhitePtFrom,
    XcmsColor *pWhitePtTo,
    XcmsColorFormat destSpecFmt,
    XcmsColor *pColors_in_out,
    unsigned int nColors,
    Bool *pCompressed)
/*
 *	DESCRIPTION
 *		Convert color specifications in an array of XcmsColor structures
 *		for differences in white points.
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
     * Insure TekHVC installed
     */
    if (XcmsAddColorSpace(&XcmsTekHVCColorSpace) == XcmsFailure) {
	return(XcmsFailure);
    }

    /*
     * Convert to TekHVC using pWhitePtFrom
     *    We can ignore return value for compression because we are converting
     *    to XcmsTekHVCFormat which is device-independent, not device-dependent.
     */
    if (_XcmsConvertColorsWithWhitePt(ccc, pColors_in_out, pWhitePtFrom,
	    nColors, XcmsTekHVCFormat, pCompressed) == XcmsFailure) {
	return(XcmsFailure);
    }

    /*
     * Convert from TekHVC to destSpecFmt using pWhitePtTo
     */
    return(_XcmsConvertColorsWithWhitePt(ccc, pColors_in_out, pWhitePtTo,
	    nColors, destSpecFmt, pCompressed));
}
