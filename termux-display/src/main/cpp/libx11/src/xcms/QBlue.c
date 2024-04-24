
/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 *
 * This file is a component of an X Window System-specific implementation
 * of Xcms based on the TekColor Color Management System.  Permission is
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
 *	NAME
 *		XcmsQBlue.c - Query Blue
 *
 *	DESCRIPTION
 *		Routine to obtain a color specification for full
 *		blue intensity and zero red and green intensities.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcms.h"



/************************************************************************
 *									*
 *			PUBLIC INTERFACES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsQueryBlue
 *
 *	SYNOPSIS
 */

Status
XcmsQueryBlue(
    XcmsCCC ccc,
    XcmsColorFormat target_format,
    XcmsColor *pColor_ret)
/*
 *	DESCRIPTION
 *		Returns the color specification in the target format for
 *		full intensity blue and zero intensity red and green.
 *
 *	RETURNS
 *		Returns XcmsSuccess, if failed; otherwise XcmsFailure
 *
 */
{
    XcmsColor tmp;

    tmp.format = XcmsRGBiFormat;
    tmp.pixel = 0;
    tmp.spec.RGBi.red = 0.0;
    tmp.spec.RGBi.green = 0.0;
    tmp.spec.RGBi.blue = 1.0;
    if (XcmsConvertColors(ccc, &tmp, 1, target_format, NULL) != XcmsSuccess) {
	return(XcmsFailure);
    }
    memcpy((char *)pColor_ret, (char *)&tmp, sizeof(XcmsColor));
    return(XcmsSuccess);
}
