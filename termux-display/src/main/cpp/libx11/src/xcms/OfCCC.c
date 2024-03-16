
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
 *		XcmsOfCCC.c - Color Conversion Context Querying Routines
 *
 *	DESCRIPTION
 *		Routines to query components of a Color Conversion
 *		Context structure.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlib.h"
#include "Xcms.h"



/************************************************************************
 *									*
 *			PUBLIC INTERFACES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsDisplayOfCCC
 *
 *	SYNOPSIS
 */

Display *
XcmsDisplayOfCCC(
    XcmsCCC ccc)
/*
 *	DESCRIPTION
 *		Queries the Display of the specified CCC.
 *
 *	RETURNS
 *		Pointer to the Display.
 *
 */
{
    return(ccc->dpy);
}


/*
 *	NAME
 *		XcmsVisualOfCCC
 *
 *	SYNOPSIS
 */

Visual *
XcmsVisualOfCCC(
    XcmsCCC ccc)
/*
 *	DESCRIPTION
 *		Queries the Visual of the specified CCC.
 *
 *	RETURNS
 *		Pointer to the Visual.
 *
 */
{
    return(ccc->visual);
}


/*
 *	NAME
 *		XcmsScreenNumberOfCCC
 *
 *	SYNOPSIS
 */

int
XcmsScreenNumberOfCCC(
    XcmsCCC ccc)
/*
 *	DESCRIPTION
 *		Queries the screen number of the specified CCC.
 *
 *	RETURNS
 *		screen number.
 *
 */
{
    return(ccc->screenNumber);
}


/*
 *	NAME
 *		XcmsScreenWhitePointOfCCC
 *
 *	SYNOPSIS
 */

XcmsColor *
XcmsScreenWhitePointOfCCC(
    XcmsCCC ccc)
/*
 *	DESCRIPTION
 *		Queries the screen white point of the specified CCC.
 *
 *	RETURNS
 *		Pointer to the XcmsColor containing the screen white point.
 *
 */
{
    return(&ccc->pPerScrnInfo->screenWhitePt);
}


/*
 *	NAME
 *		XcmsClientWhitePointOfCCC
 *
 *	SYNOPSIS
 */

XcmsColor *
XcmsClientWhitePointOfCCC(
    XcmsCCC ccc)
/*
 *	DESCRIPTION
 *		Queries the client white point of the specified CCC.
 *
 *	RETURNS
 *		Pointer to the XcmsColor containing the client white point.
 *
 */
{
    return(&ccc->clientWhitePt);
}
