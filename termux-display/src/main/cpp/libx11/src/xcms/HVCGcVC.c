
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
 *		TekHVCGcVC.c
 *
 *	DESCRIPTION
 *		Source for XcmsTekHVCClipVC() gamut
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
 *		XcmsTekHVCClipVC - Return the closest value and chroma
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsTekHVCClipVC (
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    unsigned int nColors,
    unsigned int i,
    Bool *pCompressed)
/*
 *	DESCRIPTION
 *		This routine will find the closest value and chroma
 *		for a specific hue.  The color input is converted to
 *		HVC format and returned as CIE XYZ format.
 *
 *		Since this routine works with the value within
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
    XcmsColor  *pColor;
    XcmsColor  hvc_max;
    XcmsRGBi   rgb_max;
    int	      nCount, nMaxCount, nI, nILast;
    XcmsFloat  Chroma, Value, bestChroma, bestValue, nT, saveDist, tmpDist;

    /*
     * Insure TekHVC installed
     */
    if (XcmsAddColorSpace(&XcmsTekHVCColorSpace) == XcmsFailure) {
	return(XcmsFailure);
    }

    /* Use my own CCC */
    memcpy ((char *)&myCCC, (char *)ccc, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;/* inherit screen white pt */
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

    if (ccc->visual->class < StaticColor &&
	    FunctionSetOfCCC(ccc) != (XPointer) &XcmsLinearRGBFunctionSet) {
	/*
	 * GRAY !
	 */
	_XcmsDIConvertColors(ccc, pColor, &ccc->pPerScrnInfo->screenWhitePt,
		1, XcmsTekHVCFormat);
	pColor->spec.TekHVC.H = pColor->spec.TekHVC.C = 0.0;
	_XcmsDIConvertColors(ccc, pColor, &ccc->pPerScrnInfo->screenWhitePt,
		1, XcmsCIEXYZFormat);
	if (pCompressed) {
	    *(pCompressed + i) = True;
	}
	return(XcmsSuccess);
    } else {
	/* Convert from CIEXYZ to TekHVC format */
	if (_XcmsDIConvertColors(&myCCC, pColor,
		&myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsTekHVCFormat)
		== XcmsFailure) {
	    return(XcmsFailure);
	}

	if (!_XcmsTekHVC_CheckModify(pColor)) {
	    return (XcmsFailure);
	}

	/* Step 1: compute the maximum value and chroma for this hue. */
	/*         This copy may be overkill but it preserves the pixel etc. */
	memcpy((char *)&hvc_max, (char *)pColor, sizeof(XcmsColor));
	if (_XcmsTekHVCQueryMaxVCRGB (&myCCC, hvc_max.spec.TekHVC.H, &hvc_max,
		&rgb_max) == XcmsFailure) {
	    return (XcmsFailure);
	}

	/* Now check and return the appropriate value */
	if (pColor->spec.TekHVC.V == hvc_max.spec.TekHVC.V) {
	    /* When the value input is equal to the maximum value */
	    /* merely return the chroma for that value. */
	    pColor->spec.TekHVC.C = hvc_max.spec.TekHVC.C;
	    retval = _XcmsDIConvertColors(&myCCC, pColor,
		    &myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsCIEXYZFormat);
	}

	if (pColor->spec.TekHVC.V < hvc_max.spec.TekHVC.V) {
	    /* return the intersection of the perpendicular line through     */
	    /* the value and chroma given and the line from 0,0 and hvc_max. */
	    Chroma = pColor->spec.TekHVC.C;
	    Value = pColor->spec.TekHVC.V;
	    pColor->spec.TekHVC.C =
	       (Value + (hvc_max.spec.TekHVC.C / hvc_max.spec.TekHVC.V * Chroma)) /
	       ((hvc_max.spec.TekHVC.V / hvc_max.spec.TekHVC.C) +
		(hvc_max.spec.TekHVC.C / hvc_max.spec.TekHVC.V));
	    if (pColor->spec.TekHVC.C >= hvc_max.spec.TekHVC.C) {
		pColor->spec.TekHVC.C = hvc_max.spec.TekHVC.C;
		pColor->spec.TekHVC.V = hvc_max.spec.TekHVC.V;
	    } else {
		pColor->spec.TekHVC.V = pColor->spec.TekHVC.C *
				    hvc_max.spec.TekHVC.V / hvc_max.spec.TekHVC.C;
	    }
	    retval = _XcmsDIConvertColors(&myCCC, pColor,
		    &myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsCIEXYZFormat);

	    if (retval != XcmsFailure && pCompressed != NULL) {
		*(pCompressed + i) = True;
	    }
	    return (retval);
	}

	/* return the closest point on the upper part of the hue leaf. */
	/* must do a bisection here to compute the delta e. */
	nMaxCount = MAXBISECTCOUNT;
	nI =     nMaxCount / 2;
	bestValue = Value =  pColor->spec.TekHVC.V;
	bestChroma = Chroma = pColor->spec.TekHVC.C;
	saveDist = (XcmsFloat) XCMS_SQRT ((double) (((Chroma - hvc_max.spec.TekHVC.C) *
					       (Chroma - hvc_max.spec.TekHVC.C)) +
					      ((Value - hvc_max.spec.TekHVC.V) *
					       (Value - hvc_max.spec.TekHVC.V))));
	for (nCount = 0; nCount < nMaxCount; nCount++) {
	    nT = (XcmsFloat) nI / (XcmsFloat) nMaxCount;
	    pColor->spec.RGBi.red   = rgb_max.red * (1.0 - nT) + nT;
	    pColor->spec.RGBi.green = rgb_max.green * (1.0 - nT) + nT;
	    pColor->spec.RGBi.blue  = rgb_max.blue * (1.0 - nT) + nT;
	    pColor->format = XcmsRGBiFormat;

	    /* Convert from RGBi to HVC */
	    if (_XcmsConvertColorsWithWhitePt(&myCCC, pColor,
		    &myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsTekHVCFormat,
		    (Bool *) NULL)
		    == XcmsFailure) {
		return (XcmsFailure);
	    }
	    if (!_XcmsTekHVC_CheckModify(pColor)) {
		return (XcmsFailure);
	    }
	    tmpDist = (XcmsFloat) XCMS_SQRT ((double)
			(((Chroma - pColor->spec.TekHVC.C) *
			  (Chroma - pColor->spec.TekHVC.C)) +
			 ((Value - pColor->spec.TekHVC.V) *
			  (Value - pColor->spec.TekHVC.V))));
	    nILast = nI;
	    if (tmpDist > saveDist) {
		nI /= 2;
	    } else {
		nI = (nMaxCount + nI) / 2;
		saveDist = tmpDist;
		bestValue = pColor->spec.TekHVC.V;
		bestChroma = pColor->spec.TekHVC.C;
	    }
	    if (nI == nILast || nI == 0) {
		break;
	    }

	}

	if (bestChroma >= hvc_max.spec.TekHVC.C) {
	    pColor->spec.TekHVC.C = hvc_max.spec.TekHVC.C;
	    pColor->spec.TekHVC.V = hvc_max.spec.TekHVC.V;
	} else {
	    pColor->spec.TekHVC.C = bestChroma;
	    pColor->spec.TekHVC.V = bestValue;
	}
	if (!_XcmsTekHVC_CheckModify(pColor)) {
	    return (XcmsFailure);
	}
	retval = _XcmsDIConvertColors(&myCCC, pColor,
		&myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsCIEXYZFormat);

	if (retval != XcmsFailure && pCompressed != NULL) {
	    *(pCompressed + i) = True;
	}
	return(retval);
    }
}
