
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
 *
 *
 *	NAME
 *		CIEXYZ.c
 *
 *	DESCRIPTION
 *		CIE XYZ Color Space
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xos.h>
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"

#include <stdio.h> /* sscanf */

/*
 *	DEFINES
 *		Internal definitions that need NOT be exported to any package
 *		or program using this package.
 */
#ifdef DBL_EPSILON
#  define XMY_DBL_EPSILON DBL_EPSILON
#else
#  define XMY_DBL_EPSILON 0.00001
#endif

/*
 *      FORWARD DECLARATIONS
 */
static int CIEXYZ_ParseString(register char *spec, XcmsColor *pColor);

/*
 *      LOCALS VARIABLES
 */

static XcmsConversionProc Fl_CIEXYZ_to_CIEXYZ[] = {
    NULL
};



/*
 *      GLOBALS
 *              Variables declared in this package that are allowed
 *		to be used globally.
 */
    /*
     * CIE XYZ Color Space
     */
XcmsColorSpace	XcmsCIEXYZColorSpace =
    {
	_XcmsCIEXYZ_prefix,		/* prefix */
	XcmsCIEXYZFormat,		/* id */
	CIEXYZ_ParseString,	/* parseString */
	Fl_CIEXYZ_to_CIEXYZ,	/* to_CIEXYZ */
	Fl_CIEXYZ_to_CIEXYZ,	/* from_CIEXYZ */
	1
    };


/************************************************************************
 *									*
 *			PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		CIEXYZ_ParseString
 *
 *	SYNOPSIS
 */
static int
CIEXYZ_ParseString(
    register char *spec,
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		This routines takes a string and attempts to convert
 *		it into a XcmsColor structure with XcmsCIEXYZFormat.
 *		The assumed CIEXYZ string syntax is:
 *		    CIEXYZ:<X>/<Y>/<Z>
 *		Where X, Y, and Z are in string input format for floats
 *		consisting of:
 *		    a. an optional sign
 *		    b. a string of numbers possibly containing a decimal point,
 *		    c. an optional exponent field containing an 'E' or 'e'
 *			followed by a possibly signed integer string.
 *
 *	RETURNS
 */
{
    size_t n;
    char *pchar;

    if ((pchar = strchr(spec, ':')) == NULL) {
	return(XcmsFailure);
    }
    n = (size_t)(pchar - spec);

    /*
     * Check for proper prefix.
     */
    if (strncmp(spec, _XcmsCIEXYZ_prefix, n) != 0) {
	return(XcmsFailure);
    }

    /*
     * Attempt to parse the value portion.
     */
    if (sscanf(spec + n + 1, "%lf/%lf/%lf",
	    &pColor->spec.CIEXYZ.X,
	    &pColor->spec.CIEXYZ.Y,
	    &pColor->spec.CIEXYZ.Z) != 3) {
	char *s; /* Maybe failed due to locale */
	int f;
	if ((s = strdup(spec))) {
	    for (f = 0; s[f]; ++f)
		if (s[f] == '.')
		    s[f] = ',';
		else if (s[f] == ',')
		    s[f] = '.';
	    if (sscanf(s + n + 1, "%lf/%lf/%lf",
		       &pColor->spec.CIEXYZ.X,
		       &pColor->spec.CIEXYZ.Y,
		       &pColor->spec.CIEXYZ.Z) != 3) {
		free(s);
		return(XcmsFailure);
	    }
	    free(s);
	} else
	    return(XcmsFailure);
    }
    pColor->format = XcmsCIEXYZFormat;
    pColor->pixel = 0;
    return(_XcmsCIEXYZ_ValidSpec(pColor));
}


/************************************************************************
 *									*
 *			PUBLIC ROUTINES 				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELab_ValidSpec
 *
 *	SYNOPSIS
 */
Status
_XcmsCIEXYZ_ValidSpec(
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		Checks if color specification valid for CIE XYZ
 *
 *	RETURNS
 *		XcmsFailure if invalid,
 *		XcmsSuccess if valid.
 *
 */
{
    if (pColor->format != XcmsCIEXYZFormat
	    ||
	    (pColor->spec.CIEXYZ.Y < 0.0 - XMY_DBL_EPSILON)
	    ||
	    (pColor->spec.CIEXYZ.Y > 1.0 + XMY_DBL_EPSILON)) {
	return(XcmsFailure);
    }
    return(XcmsSuccess);
}
