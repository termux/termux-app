
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
 *		CIELabGcC.c
 *
 *	DESCRIPTION
 *		Source for XcmsCIELabClipuv() gamut compression routine.
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
 *		XcmsCIELabClipab - Reduce the chroma for a hue and L*
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsCIELabClipab (
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    unsigned int nColors,
    unsigned int i,
    Bool *pCompressed)
/*
 *	DESCRIPTION
 *		Reduce the Chroma for a specific hue and chroma to
 *		to bring the given color into the gamut of the
 *		specified device.  As required of gamut compression
 *		functions, this routine returns pColor_in_out
 *		in XcmsCIEXYZFormat on successful completion.
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
    XcmsColor *pColor;

    /*
     * Color specification passed as input can be assumed to:
     *	1. Be in XcmsCIEXYZFormat
     *	2. Already be white point adjusted for the Screen White Point.
     *	    This means that the white point now associated with this
     *	    color spec is the Screen White Point (even if the
     *	    ccc->clientWhitePt differs).
     */

    pColor = pColors_in_out + i;

    if (ccc->visual->class < PseudoColor) {
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
    } else {
	if (pColor->format != XcmsCIELabFormat) {
	    if (_XcmsDIConvertColors(ccc, pColor,
		    &ccc->pPerScrnInfo->screenWhitePt, 1, XcmsCIELabFormat)
		    == XcmsFailure) {
		return(XcmsFailure);
	    }
	}
	if (XcmsCIELabQueryMaxC(ccc,
		degrees(XCMS_CIELAB_PMETRIC_HUE(pColor->spec.CIELab.a_star,
						pColor->spec.CIELab.b_star)),
		pColor->spec.CIELab.L_star,
		pColor) == XcmsFailure) {
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
