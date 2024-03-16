
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
 *		XcmsStCols.c
 *
 *	DESCRIPTION
 *		Source for XcmsStoreColors
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"
#include "reallocarray.h"


/************************************************************************
 *									*
 *			PUBLIC ROUTINES					*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsStoreColors - Store Colors
 *
 *	SYNOPSIS
 */
Status
XcmsStoreColors(
    Display *dpy,
    Colormap colormap,
    XcmsColor *pColors_in,
    unsigned int nColors,
    Bool *pCompressed)
/*
 *	DESCRIPTION
 *		Given device-dependent or device-independent color
 *		specifications, this routine will convert them to X RGB
 *		values then use it in a call to XStoreColors.
 *
 *	RETURNS
 *		XcmsFailure if failed;
 *		XcmsSuccess if it succeeded without gamut compression;
 *		XcmsSuccessWithCompression if it succeeded with gamut
 *			compression;
 *
 *		Since XStoreColors has no return value, this routine
 *		does not return color specifications of the colors actually
 *		stored.
 */
{
    XcmsColor Color1;
    XcmsColor *pColors_tmp;
    Status retval;

    /*
     * Make copy of array of color specifications so we don't
     * overwrite the contents.
     */
    if (nColors > 1) {
	pColors_tmp = Xmallocarray(nColors, sizeof(XcmsColor));
	if (pColors_tmp == NULL)
	    return(XcmsFailure);
    } else {
	pColors_tmp = &Color1;
    }
    memcpy((char *)pColors_tmp, (char *)pColors_in,
 	    nColors * sizeof(XcmsColor));

    /*
     * Call routine to store colors using the copied color structures
     */
    retval = _XcmsSetGetColors (XStoreColors, dpy, colormap,
	    pColors_tmp, nColors, XcmsRGBFormat, pCompressed);

    /*
     * Free copies as needed.
     */
    if (nColors > 1) {
	Xfree(pColors_tmp);
    }

    /*
     * Ah, finally return.
     */
    return(retval);
}
