
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
 *		TekHVCMxVC.c
 *
 *	DESCRIPTION
 *		Source for the XcmsTekHVCQueryMaxVC() gamut boundary
 *		querying routine.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"

/*
 *	DEFINES
 */
#define MIN(x,y) ((x) > (y) ? (y) : (x))
#define MIN3(x,y,z) ((x) > (MIN((y), (z))) ? (MIN((y), (z))) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MAX3(x,y,z) ((x) > (MAX((y), (z))) ? (x) : (MAX((y), (z))))
#define START_V	    40.0
#define START_C	    120.0


/************************************************************************
 *									*
 *			 API PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		_XcmsTekHVCQueryMaxVCRGB - Compute maximum value/chroma.
 *
 *	SYNOPSIS
 */
Status
_XcmsTekHVCQueryMaxVCRGB(
    XcmsCCC	ccc,
    XcmsFloat	hue,
    XcmsColor   *pColor_return,
    XcmsRGBi    *pRGB_return)

/*
 *	DESCRIPTION
 *		Return the maximum chroma for a specified hue, and the
 *		corresponding value.  This is computed by a binary search of
 *		all possible chromas.  An assumption is made that there are
 *		no local maxima.  Use the unrounded Max Chroma because
 *		the difference check can be small.
 *
 *		NOTE:  No local CCC is used because this is a private
 *		       routine and all routines that call it are expected
 *		       to behave properly, i.e. send a local CCC with
 *		       no white adjust function and no gamut compression
 *		       function.
 *
 *		This routine only accepts hue as input and outputs
 *		HVC's and RGBi's.
 *
 *	RETURNS
 *		XcmsFailure - Failure
 *		XCMS_SUCCUSS - Succeeded
 *
 */
{
    XcmsFloat nSmall, nLarge;
    XcmsColor tmp;

    tmp.format = XcmsTekHVCFormat;
    tmp.spec.TekHVC.H = hue;
    /*  Use some unreachable color on the given hue */
    tmp.spec.TekHVC.V = START_V;
    tmp.spec.TekHVC.C = START_C;


    /*
     * Convert from HVC to RGB
     *
     * Note that the CIEXYZ to RGBi conversion routine must stuff the
     * out of bounds RGBi values in tmp when the ccc->gamutCompProc
     * is NULL.
     */
    if ((_XcmsConvertColorsWithWhitePt(ccc, &tmp,
	    &ccc->pPerScrnInfo->screenWhitePt, 1, XcmsRGBiFormat, (Bool *) NULL)
	    == XcmsFailure) && tmp.format != XcmsRGBiFormat) {
	return (XcmsFailure);
    }

    /* Now pick the smallest RGB */
    nSmall = MIN3(tmp.spec.RGBi.red,
		  tmp.spec.RGBi.green,
		  tmp.spec.RGBi.blue);
    /* Make the smallest RGB equal to zero */
    tmp.spec.RGBi.red   -= nSmall;
    tmp.spec.RGBi.green -= nSmall;
    tmp.spec.RGBi.blue  -= nSmall;

    /* Now pick the largest RGB */
    nLarge = MAX3(tmp.spec.RGBi.red,
		  tmp.spec.RGBi.green,
		  tmp.spec.RGBi.blue);
    /* Scale the RGB values based on the largest one */
    tmp.spec.RGBi.red   /= nLarge;
    tmp.spec.RGBi.green /= nLarge;
    tmp.spec.RGBi.blue  /= nLarge;
    tmp.format = XcmsRGBiFormat;

    /* If the calling routine wants RGB value give them the ones used. */
    if (pRGB_return) {
	pRGB_return->red   = tmp.spec.RGBi.red;
	pRGB_return->green = tmp.spec.RGBi.green;
	pRGB_return->blue  = tmp.spec.RGBi.blue;
    }

    /* Convert from RGBi to HVC */
    if (_XcmsConvertColorsWithWhitePt(ccc, &tmp,
	    &ccc->pPerScrnInfo->screenWhitePt, 1, XcmsTekHVCFormat, (Bool *) NULL)
	    == XcmsFailure) {
	return (XcmsFailure);
    }

    /* make sure to return the input hue */
    tmp.spec.TekHVC.H = hue;
    memcpy((char *)pColor_return, (char *)&tmp, sizeof(XcmsColor));
    return (XcmsSuccess);
}


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsTekHVCQueryMaxVC - Compute maximum value and chroma.
 *
 *	SYNOPSIS
 */
Status
XcmsTekHVCQueryMaxVC (
    XcmsCCC ccc,
    XcmsFloat hue,
    XcmsColor *pColor_return)

/*
 *	DESCRIPTION
 *		Return the maximum chroma for the specified hue, and the
 *		corresponding value.
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

    /*
     * Check Arguments
     */
    if (ccc == NULL || pColor_return == NULL) {
	return(XcmsFailure);
    }

    /*
     * Insure TekHVC installed
     */
    if (XcmsAddColorSpace(&XcmsTekHVCColorSpace) == XcmsFailure) {
	return(XcmsFailure);
    }

    /* Use my own CCC */
    memcpy ((char *)&myCCC, (char *)ccc, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;
    myCCC.gamutCompProc = (XcmsCompressionProc)NULL;

    while (hue < 0.0) {
	hue += 360.0;
    }
    while (hue >= 360.0) {
	hue -= 360.0;
    }

    return(_XcmsTekHVCQueryMaxVCRGB (&myCCC, hue, pColor_return,
	    (XcmsRGBi *)NULL));
}
