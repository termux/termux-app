
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
 *	NAME
 *		TekHVCMxVs.c
 *
 *	DESCRIPTION
 *		Source for the XcmsTekHVCQueryMaxVSamples() gamut boundary
 *		querying routine.
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
 *		XcmsTekHVCQueryMaxVSamples - Compute a set of value/chroma
 *						pairs.
 *
 *	SYNOPSIS
 */
Status
XcmsTekHVCQueryMaxVSamples(
    XcmsCCC ccc,
    XcmsFloat hue,
    XcmsColor *pColor_in_out,
    unsigned int nSamples)

/*
 *	DESCRIPTION
 *		Return a set of values and chromas for the input Hue.
 *		This routine will take any color as input.
 *		It returns TekHVC colors.
 *
 *		Since this routine works with the value within
 *		pColor_in_out intermediate results may be returned
 *		even though it may be invalid.
 *
 *	ASSUMPTIONS
 *		This routine assumes that the white point associated with
 *		the color specification is the Screen White Point.  The
 *		Screen White Point will also be associated with the
 *		returned color specification.
 *
 *	RETURNS
 *		XcmsFailure - Failure
 *		XcmsSuccess - Succeeded
 *
 */
{
    XcmsCCCRec myCCC;
    XcmsColor *pHVC;
    XcmsRGBi rgb_saved;
    unsigned short nI;
    XcmsFloat nT;

    /*
     * Check Arguments
     */
    if (ccc == NULL || pColor_in_out == NULL || nSamples == 0) {
	return(XcmsFailure);
    }

    /*
     * Insure TekHVC installed
     */
    if (XcmsAddColorSpace(&XcmsTekHVCColorSpace) == XcmsFailure) {
	return(XcmsFailure);
    }

    /* setup the CCC to use for the conversions. */
    memcpy ((char *) &myCCC, (char *) ccc, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;
    myCCC.gamutCompProc = (XcmsCompressionProc) NULL;

    /* Step 1: compute the maximum value and chroma for this hue. */


    /* save the Hue for use later. */
    while (hue < 0.0) {
	hue += 360.0;
    }
    while (hue > 360.0) {
	hue -= 360.0;
    }
    pColor_in_out->spec.TekHVC.H = hue;
    pColor_in_out->format = XcmsTekHVCFormat;

    /* Get the maximum value and chroma point for this hue */
    if (_XcmsTekHVCQueryMaxVCRGB(&myCCC, pColor_in_out->spec.TekHVC.H,
	    pColor_in_out, (XcmsRGBi *)&rgb_saved) == XcmsFailure) {
	return (XcmsFailure);
    }

    /* Step 2:  Convert each of the RGBi's to HVC's */
    pHVC = pColor_in_out;
    for (nI = 0; nI < nSamples; nI++, pHVC++) {
	nT = (XcmsFloat) nI / (XcmsFloat) nSamples;
	pHVC->spec.RGBi.red   = rgb_saved.red * (1.0 - nT) + nT;
	pHVC->spec.RGBi.green = rgb_saved.green * (1.0 - nT) + nT;
	pHVC->spec.RGBi.blue  = rgb_saved.blue * (1.0 - nT) + nT;
	pHVC->format          = XcmsRGBiFormat;
	pHVC->pixel           = pColor_in_out->pixel;
	/* convert from RGB to HVC */
	if (_XcmsConvertColorsWithWhitePt(&myCCC, pHVC,
		&myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsTekHVCFormat,
		(Bool *) NULL) == XcmsFailure) {
	    return(XcmsFailure);
	}

	/* make sure to return the input hue */
	pHVC->spec.TekHVC.H = hue;
    }

    return(XcmsSuccess);
}
