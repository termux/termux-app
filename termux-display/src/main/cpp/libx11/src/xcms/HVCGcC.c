
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
 *		TekHVCGcC.c
 *
 *	DESCRIPTION
 *		Source for XcmsTekHVCClipC() gamut compression routine.
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
 *		XcmsTekHVCClipC - Reduce the chroma for a hue and value
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsTekHVCClipC (
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    unsigned int nColors,
    unsigned int i,
    Bool *pCompressed)
/*
 *	DESCRIPTION
 *		Reduce the Chroma for a specific hue and value to
 *		to bring the given color into the gamut of the
 *		specified device.  As required of gamut compression
 *		functions in Xcms, this routine returns pColor_in_out
 *		in XcmsCIEXYZFormat on successful completion.
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
    XcmsColor *pColor;

    /*
     * Color specification passed as input can be assumed to:
     *	1. Be in XcmsCIEXYZFormat
     *	2. Already be white point adjusted for the Screen White Point.
     *	    This means that the white point now associated with this
     *	    color spec is the Screen White Point (even if the
     *	    ccc->clientWhitePt differs).
     */

    /*
     * Insure TekHVC installed
     */
    if (XcmsAddColorSpace(&XcmsTekHVCColorSpace) == XcmsFailure) {
	return(XcmsFailure);
    }

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
	if (pColor->format != XcmsTekHVCFormat) {
	    if (_XcmsDIConvertColors(ccc, pColor,
		    &ccc->pPerScrnInfo->screenWhitePt, 1, XcmsTekHVCFormat)
		    == XcmsFailure) {
		return(XcmsFailure);
	    }
	}
	if (XcmsTekHVCQueryMaxC(ccc,
		pColor->spec.TekHVC.H,
		pColor->spec.TekHVC.V,
		pColor)
		== XcmsFailure) {
	    return(XcmsFailure);
	}
	retval = _XcmsDIConvertColors(ccc, pColor,
		&ccc->pPerScrnInfo->screenWhitePt, 1, XcmsCIEXYZFormat);
	if (retval != XcmsFailure && pCompressed != NULL) {
	    *(pCompressed + i) = True;
	}
	return(retval);
    }
}
