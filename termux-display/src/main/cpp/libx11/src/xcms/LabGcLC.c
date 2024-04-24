
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
 *		CIELabGcLC.c
 *
 *	DESCRIPTION
 *		Source for XcmsCIELabClipLab() gamut
 *		compression function.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"
#include <math.h>
#include "Cv.h"

/*
 *	INTERNALS
 *		Internal defines that need NOT be exported to any package or
 *		program using this package.
 */
#define MAXBISECTCOUNT	100


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELabClipLab - Return the closest L* and chroma
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsCIELabClipLab (
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    unsigned int nColors,
    unsigned int i,
    Bool *pCompressed)
/*
 *	DESCRIPTION
 *		This routine will find the closest L* and chroma
 *		for a specific hue.  The color input is converted to
 *		CIE L*u*v* format and returned as CIE XYZ format.
 *
 *		Since this routine works with the L* within
 *		pColor_in_out intermediate results may be returned
 *		even though it may be invalid.
 *
 *	RETURNS
 *		XcmsFailure - Failure
 *              XcmsSuccess - Succeeded
 *
 */
{
    Status retval;
    XcmsCCCRec	myCCC;
    XcmsColor	*pColor;
    XcmsColor	Lab_max;
    XcmsFloat	hue, chroma, maxChroma;
    XcmsFloat	Chroma, bestChroma, Lstar, maxLstar, saveLstar;
    XcmsFloat	bestLstar, bestastar, bestbstar;
    XcmsFloat	nT, saveDist, tmpDist;
    XcmsRGBi	rgb_max;
    int		nCount, nMaxCount, nI, nILast;

    /* Use my own CCC */
    memcpy ((char *)&myCCC, (char *)ccc, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;/* inherit screen white */
    myCCC.gamutCompProc = (XcmsCompressionProc)NULL;/* no gamut compression func */

    /*
     * Color specification passed as input can be assumed to:
     *	1. Be in XcmsCIEXYZFormat
     *	2. Already be white point adjusted for the Screen White Point.
     *	    This means that the white point now associated with this
     *	    color spec is the Screen White Point (even if the
     *	    ccc->clientWhitePt differs).
     */

    pColor = pColors_in_out + i;

    if (ccc->visual->class < StaticColor) {
	/*
	 * GRAY !
	 */
	_XcmsDIConvertColors(ccc, pColor, ScreenWhitePointOfCCC(ccc),
		1, XcmsCIELabFormat);
	_XcmsDIConvertColors(ccc, pColor, ScreenWhitePointOfCCC(ccc),
		1, XcmsCIEXYZFormat);
	if (pCompressed) {
	    *(pCompressed + i) = True;
	}
	return(XcmsSuccess);
    }

    /* Convert from CIEXYZ to CIELab format */
    if (_XcmsDIConvertColors(&myCCC, pColor,
		            ScreenWhitePointOfCCC(&myCCC), 1, XcmsCIELabFormat)
		== XcmsFailure) {
	return(XcmsFailure);
    }

    /* Step 1: compute the maximum L* and chroma for this hue. */
    /*         This copy may be overkill but it preserves the pixel etc. */
    saveLstar = pColor->spec.CIELab.L_star;
    hue = XCMS_CIELAB_PMETRIC_HUE(pColor->spec.CIELab.a_star,
				  pColor->spec.CIELab.b_star);
    chroma = XCMS_CIELAB_PMETRIC_CHROMA(pColor->spec.CIELab.a_star,
					pColor->spec.CIELab.b_star);
    memcpy((char *)&Lab_max, (char *)pColor, sizeof(XcmsColor));
    if (_XcmsCIELabQueryMaxLCRGB (&myCCC, hue, &Lab_max, &rgb_max)
	    == XcmsFailure) {
	return (XcmsFailure);
    }
    maxLstar = Lab_max.spec.CIELab.L_star;

    /* Now check and return the appropriate L* */
    if (saveLstar == maxLstar) {
	/* When the L* input is equal to the maximum L* */
	/* merely return the maximum Lab point. */
	memcpy((char *)pColor, (char *)&Lab_max, sizeof(XcmsColor));
	retval = _XcmsDIConvertColors(&myCCC, pColor,
		           ScreenWhitePointOfCCC(&myCCC), 1, XcmsCIEXYZFormat);
    } else {
	/* return the closest point on the hue leaf. */
	/* must do a bisection here to compute the delta e. */
	maxChroma = XCMS_CIELAB_PMETRIC_CHROMA(Lab_max.spec.CIELab.a_star,
					       Lab_max.spec.CIELab.b_star);
	nMaxCount = MAXBISECTCOUNT;
	nI = nMaxCount / 2;
	bestLstar = Lstar =  pColor->spec.CIELab.L_star;
	bestastar = pColor->spec.CIELab.a_star;
	bestbstar = pColor->spec.CIELab.b_star;
	bestChroma = Chroma = chroma;
	saveDist = XCMS_SQRT(((Chroma - maxChroma) * (Chroma - maxChroma)) +
			     ((Lstar - maxLstar) * (Lstar - maxLstar)));
	for (nCount = 0; nCount < nMaxCount; nCount++) {
	    nT = (XcmsFloat) nI / (XcmsFloat) nMaxCount;
	    if (saveLstar > maxLstar) {
		pColor->spec.RGBi.red   = rgb_max.red * (1.0 - nT) + nT;
		pColor->spec.RGBi.green = rgb_max.green * (1.0 - nT) + nT;
		pColor->spec.RGBi.blue  = rgb_max.blue * (1.0 - nT) + nT;
	    } else {
		pColor->spec.RGBi.red   = rgb_max.red - (rgb_max.red * nT);
		pColor->spec.RGBi.green = rgb_max.green - (rgb_max.green * nT);
		pColor->spec.RGBi.blue  = rgb_max.blue - (rgb_max.blue * nT);
	    }
	    pColor->format = XcmsRGBiFormat;

	    /* Convert from RGBi to CIE Lab */
	    if (_XcmsConvertColorsWithWhitePt(&myCCC, pColor,
	                    ScreenWhitePointOfCCC(&myCCC), 1, XcmsCIELabFormat,
			    (Bool *) NULL) == XcmsFailure) {
		return (XcmsFailure);
	    }
	    chroma = XCMS_CIELAB_PMETRIC_CHROMA(pColor->spec.CIELab.a_star,
					        pColor->spec.CIELab.b_star);
	    tmpDist = XCMS_SQRT(((Chroma - chroma) * (Chroma - chroma)) +
			        ((Lstar - pColor->spec.CIELab.L_star) *
				 (Lstar - pColor->spec.CIELab.L_star)));
	    nILast = nI;
	    if (tmpDist > saveDist) {
		nI /= 2;
	    } else {
		nI = (nMaxCount + nI) / 2;
		saveDist = tmpDist;
		bestLstar = pColor->spec.CIELab.L_star;
		bestastar = pColor->spec.CIELab.a_star;
		bestbstar = pColor->spec.CIELab.b_star;
		bestChroma = chroma;
	    }
	    if (nI == nILast || nI == 0) {
		break;
	    }
	}
	if (bestChroma >= maxChroma) {
	    pColor->spec.CIELab.L_star = maxLstar;
	    pColor->spec.CIELab.a_star = Lab_max.spec.CIELab.a_star;
	    pColor->spec.CIELab.b_star = Lab_max.spec.CIELab.b_star;
	} else {
	    pColor->spec.CIELab.L_star = bestLstar;
	    pColor->spec.CIELab.a_star = bestastar;
	    pColor->spec.CIELab.b_star = bestbstar;
	}
	retval = _XcmsDIConvertColors(&myCCC, pColor,
		           ScreenWhitePointOfCCC(&myCCC), 1, XcmsCIEXYZFormat);

	if (retval != XcmsFailure && pCompressed != NULL) {
	    *(pCompressed + i) = True;
	}
    }
    return(retval);
}
