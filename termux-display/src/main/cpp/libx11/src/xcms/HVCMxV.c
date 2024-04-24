
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
 *		TekHVCMxV.c
 *
 *	DESCRIPTION
 *		Source for the XcmsTekHVCQueryMaxV() gamut boundary
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
#define EPS	    0.001


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsTekHVCQueryMaxV - Compute maximum value for a hue and chroma
 *
 *	SYNOPSIS
 */
Status
XcmsTekHVCQueryMaxV(
    XcmsCCC ccc,
    XcmsFloat hue,
    XcmsFloat chroma,
    XcmsColor *pColor_return)
/*
 *	DESCRIPTION
 *		Return the maximum value for a specified hue and chroma.
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
    XcmsCCCRec myCCC;
    XcmsColor   tmp;
    XcmsColor   max_vc;
    XcmsRGBi    rgb_saved;
    int         nCount, nMaxCount;
    XcmsFloat   nT, nChroma, savedChroma, lastValue, lastChroma, prevChroma;
    XcmsFloat   rFactor;
    XcmsFloat	ftmp1, ftmp2;

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

    /* setup the CCC to use for the conversions. */
    memcpy ((char *) &myCCC, (char *) ccc, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;
    myCCC.gamutCompProc = (XcmsCompressionProc) NULL;

    tmp.spec.TekHVC.H = hue;
    tmp.spec.TekHVC.V = 0.0;
    tmp.spec.TekHVC.C = chroma;
    tmp.pixel = pColor_return->pixel;
    tmp.format = XcmsTekHVCFormat;

    if (!_XcmsTekHVC_CheckModify (&tmp)) {
	return(XcmsFailure);
    }

    /* Step 1: compute the maximum value and chroma for this hue. */
    /*         This copy may be overkill but it preserves the pixel etc. */
    memcpy((char *)&max_vc, (char *)&tmp, sizeof(XcmsColor));
    hue = max_vc.spec.TekHVC.H;
    if (_XcmsTekHVCQueryMaxVCRGB(&myCCC, max_vc.spec.TekHVC.H, &max_vc, &rgb_saved)
	    == XcmsFailure) {
	return(XcmsFailure);
    }

    if (max_vc.spec.TekHVC.C < tmp.spec.TekHVC.C) {
	/*
	 *  If the chroma is greater than the chroma for the
	 *  maximum value/chroma point then the value is the
	 *  the value for the maximum value, chroma point.
	 *  This is an error but it I return the best approximation I can.
         *  Thus the inconsistency.
	 */
	tmp.spec.TekHVC.C = max_vc.spec.TekHVC.C;
	tmp.spec.TekHVC.V = max_vc.spec.TekHVC.V;
	memcpy ((char *) pColor_return, (char *) &tmp, sizeof (XcmsColor));
	return(XcmsSuccess);
    } else if (max_vc.spec.TekHVC.C == tmp.spec.TekHVC.C) {
	/*
	 *  If the chroma is equal to the chroma for the
	 *  maximum value/chroma point then the value is the
	 *  the value for the maximum value, chroma point.
	 */
	tmp.spec.TekHVC.V = max_vc.spec.TekHVC.V;
	memcpy ((char *) pColor_return, (char *) &tmp, sizeof (XcmsColor));
	return(XcmsSuccess);
    } else {
	/* must do a bisection here to compute the maximum value */
	/* save the structure input so that any elements that */
	/* are not touched are recopied later in the routine. */
	nChroma = savedChroma = tmp.spec.TekHVC.C;
	tmp.spec.TekHVC.C = max_vc.spec.TekHVC.C;
	tmp.spec.TekHVC.V = max_vc.spec.TekHVC.V;
	lastChroma = -1.0;
	lastValue = -1.0;
	nMaxCount = MAXBISECTCOUNT;
	rFactor = 1.0;

	for (nCount = 0; nCount < nMaxCount; nCount++) {
	    prevChroma = lastChroma;
	    lastValue =  tmp.spec.TekHVC.V;
	    lastChroma = tmp.spec.TekHVC.C;
	    nT = (1.0 - (nChroma / max_vc.spec.TekHVC.C)) * rFactor;
	    tmp.spec.RGBi.red   = rgb_saved.red * (1.0 - nT) + nT;
	    tmp.spec.RGBi.green = rgb_saved.green * (1.0 - nT) + nT;
	    tmp.spec.RGBi.blue  = rgb_saved.blue * (1.0 - nT) + nT;
	    tmp.format = XcmsRGBiFormat;

	    /* convert from RGB to HVC */
	    if (_XcmsConvertColorsWithWhitePt(&myCCC, &tmp,
		    &myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsTekHVCFormat,
		    (Bool *) NULL) == XcmsFailure) {
		return(XcmsFailure);
	    }

	    /* Now check the return against what is expected */
	    if (tmp.spec.TekHVC.C <= savedChroma + EPS &&
		tmp.spec.TekHVC.C >= savedChroma - EPS) {
		tmp.spec.TekHVC.H = hue;  /* use the saved hue */
		memcpy ((char *) pColor_return, (char *) &tmp, sizeof (XcmsColor));
		return(XcmsSuccess);
	    }
	    nChroma += savedChroma - tmp.spec.TekHVC.C;
	    if (nChroma > max_vc.spec.TekHVC.C) {
		nChroma = max_vc.spec.TekHVC.C;
		rFactor *= 0.5;  /* selective relaxation employed */
	    } else if (nChroma < 0.0) {
		/* avoid using fabs */
		ftmp1 = lastChroma - savedChroma;
		if (ftmp1 < 0.0)
		    ftmp1 = -ftmp1;
		ftmp2 = tmp.spec.TekHVC.C - savedChroma;
		if (ftmp2 < 0.0)
		    ftmp2 = -ftmp2;
		if (ftmp1 < ftmp2) {
		    tmp.spec.TekHVC.V = lastValue;
		    tmp.spec.TekHVC.C = lastChroma;
		}
		/* make sure to return the input hue */
		tmp.spec.TekHVC.H = hue;
		if (!_XcmsTekHVC_CheckModify(&tmp)) {
		    return(XcmsFailure);
		}
		memcpy ((char *) pColor_return, (char *) &tmp, sizeof (XcmsColor));
		return(XcmsSuccess);
	    } else if (tmp.spec.TekHVC.C <= prevChroma + EPS &&
		       tmp.spec.TekHVC.C >= prevChroma - EPS) {
		rFactor *= 0.5;  /* selective relaxation employed */
	    }
	}
	if (nCount >= nMaxCount) {
	    /* avoid using fabs */
	    ftmp1 = lastChroma - savedChroma;
	    if (ftmp1 < 0.0)
		ftmp1 = -ftmp1;
	    ftmp2 = tmp.spec.TekHVC.C - savedChroma;
	    if (ftmp2 < 0.0)
		ftmp2 = -ftmp2;
	    if (ftmp1 < ftmp2) {
		    tmp.spec.TekHVC.V = lastValue;
		    tmp.spec.TekHVC.C = lastChroma;
	    }
	}
    }

    /* make sure to return the input hue */
    tmp.spec.TekHVC.H = hue;
    memcpy ((char *) pColor_return, (char *) &tmp, sizeof (XcmsColor));
    return(XcmsSuccess);
}
