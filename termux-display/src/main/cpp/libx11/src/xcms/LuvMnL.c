
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
 *	NAME
 *		CIELuvMnL.c
 *
 *	DESCRIPTION
 *		Source for the XcmsCIELuvQueryMinL() gamut boundary
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
#define EPS		(XcmsFloat)0.001
#define START_L_STAR	(XcmsFloat)40.0


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELuvQueryMinL - Compute max Lstar for a hue and chroma
 *
 *	SYNOPSIS
 */
Status
XcmsCIELuvQueryMinL(
    XcmsCCC ccc,
    XcmsFloat hue_angle,	    /* hue angle in degrees */
    XcmsFloat chroma,
    XcmsColor *pColor_return)
/*
 *	DESCRIPTION
 *		Return the maximum Lstar for a specified hue_angle and chroma.
 *
 *	ASSUMPTIONS
 *		This routine assumes that the white point associated with
 *		the color specification is the Screen White Point.  The
 *		Screen White Point will also be associated with the
 *		returned color specification.
 *
 *	RETURNS
 *		XcmsFailure - Failure
 *              XcmsSuccess - Succeeded with no modifications
 *
 */
{
    XcmsCCCRec	myCCC;
    XcmsColor   max_lc, tmp, prev;
    XcmsFloat   max_chroma, tmp_chroma;
    XcmsFloat   hue, nT, nChroma, lastChroma, prevChroma;
    XcmsFloat   rFactor;
    XcmsRGBi    rgb_saved;
    int         nCount, nMaxCount;

    /*
     * Check Arguments
     */
    if (ccc == NULL || pColor_return == NULL) {
	return(XcmsFailure);
    }

    /* setup the CCC to use for the conversions. */
    memcpy ((char *) &myCCC, (char *) ccc, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;
    myCCC.gamutCompProc = (XcmsCompressionProc) NULL;

    while (hue_angle < 0.0) {
	hue_angle += 360.0;
    }
    while (hue_angle >= 360.0) {
	hue_angle -= 360.0;
    }

    hue = radians(hue_angle);
    tmp.spec.CIELuv.L_star = START_L_STAR;
    tmp.spec.CIELuv.u_star = XCMS_CIEUSTAROFHUE(hue, chroma);
    tmp.spec.CIELuv.v_star = XCMS_CIEVSTAROFHUE(hue, chroma);
    tmp.pixel = pColor_return->pixel;
    tmp.format = XcmsCIELuvFormat;

    /* Step 1: Obtain the maximum L_star and chroma for this hue. */
    if (_XcmsCIELuvQueryMaxLCRGB(&myCCC, hue, &max_lc, &rgb_saved)
	    == XcmsFailure) {
	return(XcmsFailure);
    }

    max_chroma = XCMS_CIELUV_PMETRIC_CHROMA(max_lc.spec.CIELuv.u_star,
					    max_lc.spec.CIELuv.v_star);

    if (max_chroma <= chroma) {
	/*
	 *  If the chroma is greater than the chroma for the
	 *  maximum L/chroma point then the L_star is the
	 *  the L_star for the maximum L_star/chroma point.
	 *  This is an error but I return the best approximation I can.
         *  Thus the inconsistency.
	 */
	memcpy ((char *) pColor_return, (char *) &max_lc, sizeof (XcmsColor));
	return(XcmsSuccess);
    }

    /*
     *  If the chroma is equal to the chroma for the
     *  maximum L_star/chroma point then the L_star is the
     *  the L_star for the maximum L* and chroma point.
     */
    /* if (max_chroma == chroma) {
     *  memcpy ((char *) pColor_return, (char *) &max_lc, sizeof (XcmsColor));
     *	return(XcmsSuccess);
     *    }
     */

    /* must do a bisection here to compute the maximum L* */
    /* save the structure input so that any elements that */
    /* are not touched are recopied later in the routine. */
    nChroma = chroma;
    tmp_chroma = max_chroma;
    lastChroma = -1.0;
    nMaxCount = MAXBISECTCOUNT;
    rFactor = 1.0;

    for (nCount = 0; nCount < nMaxCount; nCount++) {
	prevChroma = lastChroma;
	lastChroma = tmp_chroma;
	nT = (nChroma - max_chroma) / max_chroma * rFactor;
	memcpy ((char *)&prev, (char *)&tmp, sizeof(XcmsColor));
	tmp.spec.RGBi.red   = rgb_saved.red + (rgb_saved.red * nT);
	tmp.spec.RGBi.green = rgb_saved.green + (rgb_saved.green * nT);
	tmp.spec.RGBi.blue  = rgb_saved.blue + (rgb_saved.blue * nT);
	tmp.format = XcmsRGBiFormat;

	/* convert from RGB to CIELuv */
	if (_XcmsConvertColorsWithWhitePt(&myCCC, &tmp,
		ScreenWhitePointOfCCC(&myCCC), 1, XcmsCIELuvFormat,
		(Bool *) NULL) == XcmsFailure) {
	    return(XcmsFailure);
	}

	/* Now check the return against what is expected */
	tmp_chroma = XCMS_CIELUV_PMETRIC_CHROMA(tmp.spec.CIELuv.u_star,
						tmp.spec.CIELuv.v_star);
	if (tmp_chroma <= chroma + EPS && tmp_chroma >= chroma - EPS) {
	    /* Found It! */
	    memcpy ((char *) pColor_return, (char *) &tmp, sizeof (XcmsColor));
	    return(XcmsSuccess);
	}
	nChroma += chroma - tmp_chroma;
	if (nChroma > max_chroma) {
	    nChroma = max_chroma;
	    rFactor *= 0.5;  /* selective relaxation employed */
	} else if (nChroma < 0.0) {
	    if (XCMS_FABS(lastChroma - chroma) <
		XCMS_FABS(tmp_chroma - chroma)) {
		memcpy ((char *)pColor_return, (char *)&prev,
			sizeof(XcmsColor));
	    } else {
		memcpy ((char *)pColor_return, (char *)&tmp,
 			sizeof(XcmsColor));
	    }
	    return(XcmsSuccess);
	} else if (tmp_chroma <= prevChroma + EPS &&
		   tmp_chroma >= prevChroma - EPS) {
	    rFactor *= 0.5;  /* selective relaxation employed */
	}
    }

    if (nCount >= nMaxCount) {
	if (XCMS_FABS(lastChroma - chroma) <
	    XCMS_FABS(tmp_chroma - chroma)) {
		memcpy ((char *)pColor_return, (char *)&prev,
 			sizeof(XcmsColor));
	    } else {
		memcpy ((char *)pColor_return, (char *)&tmp,
 			sizeof(XcmsColor));
	}
    }
    memcpy ((char *) pColor_return, (char *) &tmp, sizeof (XcmsColor));
    return(XcmsSuccess);
}
