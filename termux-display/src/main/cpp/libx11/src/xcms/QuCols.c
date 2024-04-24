
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
 *		XcmsQuCol.c
 *
 *	DESCRIPTION
 *		Source for XcmsQueryColors
 *
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
 *			PUBLIC ROUTINES					*
 *									*
 ************************************************************************/
/*
 *	NAME
 *		XcmsQueryColors - Query Colors
 *
 *	SYNOPSIS
 */
Status
XcmsQueryColors(
    Display *dpy,
    Colormap colormap,
    XcmsColor *pXcmsColors_in_out,
    unsigned int nColors,
    XcmsColorFormat result_format)
/*
 *	DESCRIPTION
 *		This routine uses XQueryColors to obtain the X RGB values
 *		stored in the specified colormap for the specified pixels.
 *		The X RGB values are then converted to the target format as
 *		specified by the format component of the XcmsColor structure.
 *
 *	RETURNS
 *		XcmsFailure if failed;
 *		XcmsSuccess if it succeeded.
 *
 *		Returns the color specifications of the colors stored in the
 *		specified pixels.
 */
{
    return(_XcmsSetGetColors(XQueryColors, dpy, colormap,
	    pXcmsColors_in_out, nColors, result_format, (Bool *) NULL));
}
