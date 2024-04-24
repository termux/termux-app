
/*
 *
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
 *	NAME
 *		XcmsProp.c
 *
 *	DESCRIPTION
 *		This utility routines for manipulating properties.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xatom.h>
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"


/************************************************************************
 *									*
 *			API PRIVATE ROUTINES				*
 *									*
 ************************************************************************/


/*
 *	NAME
 *		_XcmsGetElement -- get an element value from the property passed
 *
 *	SYNOPSIS
 */
unsigned long
_XcmsGetElement(
    int             format,
    char            **pValue,
    unsigned long   *pCount)
/*
 *	DESCRIPTION
 *	    Get the next element from the property and return it.
 *	    Also increment the pointer the amount needed.
 *
 *	Returns
 *	    unsigned long
 */
{
    unsigned long value;

    switch (format) {
      case 32:
	value = *((unsigned long *)(*pValue)) & 0xFFFFFFFF;
	*pValue += sizeof(unsigned long);
	*pCount -= 1;
	break;
      case 16:
	value = *((unsigned short *)(*pValue));
	*pValue += sizeof(unsigned short);
	*pCount -= 1;
	break;
      case 8:
	value = *((unsigned char *) (*pValue));
	*pValue += 1;
	*pCount -= 1;
	break;
      default:
	value = 0;
	break;
    }
    return(value);
}


/*
 *	NAME
 *		_XcmsGetProperty -- Determine the existence of a property
 *
 *	SYNOPSIS
 */
int
_XcmsGetProperty(
    Display *pDpy,
    Window  w,
    Atom property,
    int             *pFormat,
    unsigned long   *pNItems,
    unsigned long   *pNBytes,
    char            **pValue)
/*
 *	DESCRIPTION
 *
 *	Returns
 *	    0 if property does not exist.
 *	    1 if property exists.
 */
{
    char *prop_ret;
    int format_ret;
    long len = 6516;
    unsigned long nitems_ret, after_ret;
    Atom atom_ret;
    int xgwp_ret;

    while (True) {
	xgwp_ret = XGetWindowProperty (pDpy, w, property, 0, len, False,
				       XA_INTEGER, &atom_ret, &format_ret,
				       &nitems_ret, &after_ret,
				       (unsigned char **)&prop_ret);
	if (xgwp_ret == Success && after_ret > 0) {
	    len += nitems_ret * (format_ret >> 3);
	    XFree (prop_ret);
	} else {
	    break;
	}
    }
    if (xgwp_ret != Success || format_ret == 0 || nitems_ret == 0) {
	/* the property does not exist or is of an unexpected type or
           getting window property failed */
	XFree (prop_ret);
	return(XcmsFailure);
    }

    *pFormat = format_ret;
    *pNItems = nitems_ret;
    *pNBytes = nitems_ret * (format_ret >> 3);
    *pValue = prop_ret;
    return(XcmsSuccess);
}
