
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
 *
 *	NAME
 *		CIELabMxVC.c
 *
 *	DESCRIPTION
 *		Source for the XcmsCIELabQueryMaxLC() gamut boundary
 *		querying routine.
 *
 *	DOCUMENTATION
 *		"TekColor Color Management System, System Implementor's Manual"
 *		and
 *		Fred W. Billmeyer & Max Saltzman, "Principles of Color
 *		Technology", John Wily & Sons, Inc, 1981.
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
#define MIN(x,y)	((x) > (y) ? (y) : (x))
#define MIN3(x,y,z)	((x) > (MIN((y), (z))) ? (MIN((y), (z))) : (x))
#define MAX(x,y)	((x) > (y) ? (x) : (y))
#define MAX3(x,y,z)	((x) > (MAX((y), (z))) ? (x) : (MAX((y), (z))))
#define START_LSTAR	(XcmsFloat)40.0
#define START_CHROMA	(XcmsFloat)3.6


/************************************************************************
 *									*
 *			 API PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		_XcmsCIELabQueryMaxLCRGB - Compute maximum L* and chroma.
 *
 *	SYNOPSIS
 */
Status
_XcmsCIELabQueryMaxLCRGB(
    XcmsCCC	ccc,
    XcmsFloat	hue,		    /* hue in radians */
    XcmsColor   *pColor_return,
    XcmsRGBi    *pRGB_return)
/*
 *	DESCRIPTION
 *		Return the maximum psychometric chroma for a specified
 *		hue, and the corresponding L*.  This is computed
 *		by a binary search of all possible chromas.  An assumption
 *		is made that there are no local maxima.  Use the unrounded
 *		Max psychometric chroma because the difference check can be
 *		small.
 *
 *		NOTE:  No local CCC is used because this is a private
 *		       routine and all routines that call it are expected
 *		       to behave properly, i.e. send a local CCC with
 *		       no white adjust function and no gamut compression
 *		       function.
 *
 *		This routine only accepts hue in radians as input and outputs
 *		Lab and RGBi.
 *
 *	RETURNS
 *		XcmsFailure - Failure
 *		XcmsSuccess - Succeeded
 *
 */
{
    XcmsFloat nSmall, nLarge;
    XcmsColor tmp;

    tmp.format = XcmsCIELabFormat;
    /*  Use some unreachable color on the given hue */
    tmp.spec.CIELab.L_star = START_LSTAR;
    tmp.spec.CIELab.a_star = XCMS_CIEASTAROFHUE(hue, START_CHROMA);
    tmp.spec.CIELab.b_star = XCMS_CIEBSTAROFHUE(hue, START_CHROMA);
    /*
     * Convert from Lab to RGB
     *
     * Note that the CIEXYZ to RGBi conversion routine must stuff the
     * out of bounds RGBi values in tmp when the ccc->gamutCompProc
     * is NULL.
     */
    if ((_XcmsConvertColorsWithWhitePt(ccc, &tmp, ScreenWhitePointOfCCC(ccc),
		               (unsigned int)1, XcmsRGBiFormat, (Bool *) NULL)
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

    /* Convert from RGBi to Lab */
    if (_XcmsConvertColorsWithWhitePt(ccc, &tmp,
	       ScreenWhitePointOfCCC(ccc), 1, XcmsCIELabFormat, (Bool *) NULL)
	    == XcmsFailure) {
	return (XcmsFailure);
    }

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
 *		XcmsCIELabQueryMaxLC - Compute maximum L* and chroma.
 *
 *	SYNOPSIS
 */
Status
XcmsCIELabQueryMaxLC (
    XcmsCCC ccc,
    XcmsFloat hue_angle,	    /* hue_angle in degrees */
    XcmsColor *pColor_return)

/*
 *	DESCRIPTION
 *		Return the point of maximum chroma for the specified
 *		hue_angle.
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

    /* Use my own CCC */
    memcpy ((char *)&myCCC, (char *)ccc, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;
    myCCC.gamutCompProc = (XcmsCompressionProc)NULL;

    while (hue_angle < 0.0) {
	hue_angle += 360.0;
    }
    while (hue_angle >= 360.0) {
	hue_angle -= 360.0;
    }

    return(_XcmsCIELabQueryMaxLCRGB (&myCCC, radians(hue_angle), pColor_return,
	    (XcmsRGBi *)NULL));
}
