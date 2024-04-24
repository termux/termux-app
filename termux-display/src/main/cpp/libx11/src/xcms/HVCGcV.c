
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
 *		TekHVCGcV.c
 *
 *	DESCRIPTION
 *		Source for XcmsTekHVCClipV() gamut compression routine.
 *
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
 *		XcmsTekHVCClipV - Return the closest value
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsTekHVCClipV (
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    unsigned int nColors,
    unsigned int i,
    Bool *pCompressed)
/*
 *	DESCRIPTION
 *		Return the closest value for a specific hue and chroma.
 *		This routine takes any color as input and outputs
 *		a CIE XYZ color.
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
    XcmsColor  *pColor;
    XcmsColor  hvc_max;
    XcmsCCCRec myCCC;
    Status retval;

    /*
     * Insure TekHVC installed
     */
    if (XcmsAddColorSpace(&XcmsTekHVCColorSpace) == XcmsFailure) {
	return(XcmsFailure);
    }

    /* Use my own CCC */
    memcpy ((char *)&myCCC, (char *)ccc, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;/* Inherit Screen WP */
    myCCC.gamutCompProc = (XcmsCompressionProc)NULL;/* no gamut compression */

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
	return(XcmsFailure);
    } else {
	/* Convert from CIEXYZ to TekHVC format */
	if (_XcmsDIConvertColors(&myCCC, pColor,
		&myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsTekHVCFormat)
		== XcmsFailure) {
	    return(XcmsFailure);
	}

	/* check to make sure we have a valid TekHVC number */
	if (!_XcmsTekHVC_CheckModify (pColor)) {
	    return (XcmsFailure);
	}

	/* Step 1: compute the maximum value and chroma for this hue. */
	/*         This copy may be overkill but it preserves the pixel etc. */
	memcpy((char *)&hvc_max, (char *)pColor, sizeof(XcmsColor));
	if (_XcmsTekHVCQueryMaxVCRGB (&myCCC, hvc_max.spec.TekHVC.H, &hvc_max,
		(XcmsRGBi *)NULL) == XcmsFailure) {
	    return (XcmsFailure);
	}

	/* Now check and return the appropriate value */
	if (pColor->spec.TekHVC.C == hvc_max.spec.TekHVC.C) {
	    /* When the chroma input is equal to the maximum chroma */
	    /* merely return the value for that chroma. */
	    pColor->spec.TekHVC.V = hvc_max.spec.TekHVC.V;
	    if (!_XcmsTekHVC_CheckModify (pColor)) {
		return (XcmsFailure);
	    }
	    retval = _XcmsDIConvertColors(&myCCC, pColor,
		    &myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsCIEXYZFormat);
	} else if (pColor->spec.TekHVC.C > hvc_max.spec.TekHVC.C) {
	    /* When the chroma input is greater than the maximum chroma */
	    /* merely return the value and chroma for the given hue. */
	    pColor->spec.TekHVC.C = hvc_max.spec.TekHVC.C;
	    pColor->spec.TekHVC.V = hvc_max.spec.TekHVC.V;
	    return (XcmsFailure);
	} else if (pColor->spec.TekHVC.V < hvc_max.spec.TekHVC.V) {
	    /* When the value input is less than the maximum value point */
	    /* compute the intersection of the line from 0,0 to max_V, max_C */
	    /* using the chroma input. */
	    pColor->spec.TekHVC.V = pColor->spec.TekHVC.C *
				    hvc_max.spec.TekHVC.V / hvc_max.spec.TekHVC.C;
	    if (pColor->spec.TekHVC.V >= hvc_max.spec.TekHVC.V) {
		pColor->spec.TekHVC.C = hvc_max.spec.TekHVC.C;
		pColor->spec.TekHVC.V = hvc_max.spec.TekHVC.V;
	    }
	    if (!_XcmsTekHVC_CheckModify (pColor)) {
		return (XcmsFailure);
	    }
	    retval = _XcmsDIConvertColors(&myCCC, pColor,
		    &myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsCIEXYZFormat);
	} else {
	    /* When the value input is greater than the maximum value point */
	    /* use HvcMaxValue to find the maximum value for the given chroma. */
	    if (pColor->format != XcmsTekHVCFormat) {
		if (_XcmsDIConvertColors(ccc, pColor,
			&ccc->pPerScrnInfo->screenWhitePt, 1, XcmsCIEXYZFormat)
			== XcmsFailure) {
		    return(XcmsFailure);
		}
	    }
	    if (XcmsTekHVCQueryMaxV(&myCCC,
		    pColor->spec.TekHVC.H,
		    pColor->spec.TekHVC.C,
		    pColor)
		    == XcmsFailure) {
		return (XcmsFailure);
	    }
	    retval = _XcmsDIConvertColors(&myCCC, pColor,
		    &myCCC.pPerScrnInfo->screenWhitePt, 1, XcmsCIEXYZFormat);
	}
	if (retval != XcmsFailure && pCompressed != NULL) {
	    *(pCompressed + i) = True;
	}
	return(retval);
    }
}
