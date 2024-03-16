
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
 *		CIELabMxC.c
 *
 *	DESCRIPTION
 *		Source for the XcmsCIELabQueryMaxC() gamut boundary
 *		querying routine.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"
#include <math.h>
#include "Cv.h"

/*
 *	DEFINES
 */
#define MAXBISECTCOUNT	100
#define EPS	        (XcmsFloat)0.001
#define START_CHROMA	(XcmsFloat)3.6
#define TOPL		(XcmsFloat)100.0


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELabQueryMaxC - max chroma for a hue_angle and L_star
 *
 *	SYNOPSIS
 */
Status
XcmsCIELabQueryMaxC(
    XcmsCCC ccc,
    XcmsFloat hue_angle,	    /* hue angle in degrees */
    XcmsFloat L_star,
    XcmsColor *pColor_return)
/*
 *	DESCRIPTION
 *		Return the maximum chroma for a specific hue_angle and L_star.
 *		The returned format is in XcmsCIELabFormat.
 *
 *
 *	ASSUMPTIONS
 *		This routine assumes that the white point associated with
 *		the color specification is the Screen White Point.  The
 *		Screen White Point will also be associated with the
 *		returned color specification.
 *
 *	RETURNS
 *		XcmsFailure - Failure
 *              XcmsSuccess - Succeeded
 *
 */
{
    XcmsCCCRec	myCCC;
    XcmsColor  tmp;
    XcmsColor  max_lc;
    XcmsFloat n_L_star, last_L_star, prev_L_star;
    XcmsFloat hue, lastaStar, lastbStar, /*lastChroma,*/ maxDist, nT, rFactor;
    XcmsRGBi   rgb_saved;
    int nCount, nMaxCount;

    /*
     * Check Arguments
     */
    if (ccc == NULL || pColor_return == NULL) {
	return(XcmsFailure);
    }

    /* Use my own CCC and inherit screen white Pt */
    memcpy ((char *)&myCCC, (char *)ccc, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;
    myCCC.gamutCompProc = (XcmsCompressionProc)NULL;/* no gamut comp func */

    while (hue_angle < 0.0) {
	hue_angle += 360.0;
    }
    while (hue_angle >= 360.0) {
	hue_angle -= 360.0;
    }

    hue = radians(hue_angle);
    tmp.spec.CIELab.L_star = L_star;
    tmp.spec.CIELab.a_star = XCMS_CIEASTAROFHUE(hue, START_CHROMA);
    tmp.spec.CIELab.b_star = XCMS_CIEBSTAROFHUE(hue, START_CHROMA);
    tmp.pixel = pColor_return->pixel;
    tmp.format = XcmsCIELabFormat;

    /* Step 1: compute the maximum L_star and chroma for this hue. */
    memcpy((char *)&max_lc, (char *)&tmp, sizeof(XcmsColor));
    if (_XcmsCIELabQueryMaxLCRGB(&myCCC, hue, &max_lc, &rgb_saved)
	== XcmsFailure) {
	    return(XcmsFailure);
    }

    /*
     * Step 2:  Do a bisection here to compute the maximum chroma
     *          Note the differences between when the point to be found
     *          is above the maximum LC point and when it is below.
     */
    if (L_star <= max_lc.spec.CIELab.L_star) {
	maxDist = max_lc.spec.CIELab.L_star;
    } else {
	maxDist = TOPL - max_lc.spec.CIELab.L_star;
    }

    n_L_star = L_star;
    last_L_star = -1.0;
    nMaxCount = MAXBISECTCOUNT;
    rFactor = 1.0;

    for (nCount = 0; nCount < nMaxCount; nCount++) {
	prev_L_star =  last_L_star;
	last_L_star =  tmp.spec.CIELab.L_star;
/*	lastChroma = XCMS_CIELAB_PMETRIC_CHROMA(tmp.spec.CIELab.a_star,  */
/*						tmp.spec.CIELab.b_star); */
	lastaStar = tmp.spec.CIELab.a_star;
	lastbStar = tmp.spec.CIELab.b_star;
	nT = (n_L_star - max_lc.spec.CIELab.L_star) / maxDist * rFactor;
	if (nT > 0) {
	    tmp.spec.RGBi.red = rgb_saved.red * (1.0 - nT) + nT;
	    tmp.spec.RGBi.green = rgb_saved.green * (1.0 - nT) + nT;
	    tmp.spec.RGBi.blue  = rgb_saved.blue * (1.0 - nT) + nT;
	} else {
	    tmp.spec.RGBi.red = rgb_saved.red + (rgb_saved.red * nT);
	    tmp.spec.RGBi.green = rgb_saved.green + (rgb_saved.green * nT);
	    tmp.spec.RGBi.blue = rgb_saved.blue + (rgb_saved.blue * nT);
	}
	tmp.format = XcmsRGBiFormat;

	/* convert from RGB to CIELab */
	if (_XcmsConvertColorsWithWhitePt(&myCCC, &tmp,
			    ScreenWhitePointOfCCC(&myCCC), 1, XcmsCIELabFormat,
			    (Bool *) NULL) == XcmsFailure) {
	    return(XcmsFailure);
	}

	/*
	 * Now check if we've reached the target L_star
	 */
	/* printf("result Lstar = %lf\n", tmp.spec.CIELab.L_star); */
	if (tmp.spec.CIELab.L_star <= L_star + EPS &&
	    tmp.spec.CIELab.L_star >= L_star - EPS) {
		memcpy((char *)pColor_return, (char *)&tmp, sizeof(XcmsColor));
		return(XcmsSuccess);
	}
	if (nT > 0) {
	    n_L_star += ((TOPL - n_L_star) *
			 (L_star - tmp.spec.CIELab.L_star)) / (TOPL - L_star);
	} else {
	    n_L_star *= L_star / tmp.spec.CIELuv.L_star;
	}
	if (tmp.spec.CIELab.L_star <= prev_L_star + EPS &&
	    tmp.spec.CIELab.L_star >= prev_L_star - EPS) {
		rFactor *= 0.5;  /* selective relaxation employed */
		/* printf("rFactor = %lf\n", rFactor); */
	}
    }
    if (XCMS_FABS(last_L_star - L_star) <
	XCMS_FABS(tmp.spec.CIELab.L_star - L_star)) {
	    tmp.spec.CIELab.a_star = lastaStar;
	    tmp.spec.CIELab.b_star = lastbStar;
/*	    tmp.spec.CIELab.a_star = XCMS_CIEASTAROFHUE(hue, lastChroma); */
/*	    tmp.spec.CIELab.b_star = XCMS_CIEBSTAROFHUE(hue, lastChroma); */
    }
    tmp.spec.CIELab.L_star = L_star;
    memcpy((char *)pColor_return, (char *)&tmp, sizeof(XcmsColor));
    return(XcmsSuccess);
}
