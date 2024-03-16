
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
 *	NAME
 *		CIELuv.c
 *
 *	DESCRIPTION
 *		This file contains routines that support the CIE L*u*v*
 *		color space to include conversions to and from the CIE
 *		XYZ space.
 *
 *	DOCUMENTATION
 *		"TekColor Color Management System, System Implementor's Manual"
 *		and
 *		Fred W. Billmeyer & Max Saltzman, "Principles of Color
 *		Technology", John Wily & Sons, Inc, 1981.
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
 *	FORWARD DECLARATIONS
 */

static int CIELuv_ParseString(register char *spec, XcmsColor *pColor);
static Status XcmsCIELuv_ValidSpec(XcmsColor *pColor);

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
 *	LOCAL VARIABLES
 */

    /*
     * NULL terminated list of functions applied to get from CIELuv to CIEXYZ
     */
static XcmsConversionProc Fl_CIELuv_to_CIEXYZ[] = {
    XcmsCIELuvToCIEuvY,
    XcmsCIEuvYToCIEXYZ,
    NULL
};

    /*
     * NULL terminated list of functions applied to get from CIEXYZ to CIELuv
     */
static XcmsConversionProc Fl_CIEXYZ_to_CIELuv[] = {
    XcmsCIEXYZToCIEuvY,
    XcmsCIEuvYToCIELuv,
    NULL
};

/*
 *	GLOBALS
 */

    /*
     * CIE Luv Color Space
     */
XcmsColorSpace	XcmsCIELuvColorSpace =
    {
	_XcmsCIELuv_prefix,	/* prefix */
	XcmsCIELuvFormat,		/* id */
	CIELuv_ParseString,	/* parseString */
	Fl_CIELuv_to_CIEXYZ,	/* to_CIEXYZ */
	Fl_CIEXYZ_to_CIELuv,	/* from_CIEXYZ */
	1
    };


/************************************************************************
 *									*
 *			 PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		CIELuv_ParseString
 *
 *	SYNOPSIS
 */
static int
CIELuv_ParseString(
    register char *spec,
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		This routines takes a string and attempts to convert
 *		it into a XcmsColor structure with XcmsCIELuvFormat.
 *		The assumed CIELuv string syntax is:
 *		    CIELuv:<L>/<u>/<v>
 *		Where L, u, and v are in string input format for floats
 *		consisting of:
 *		    a. an optional sign
 *		    b. a string of numbers possibly containing a decimal point,
 *		    c. an optional exponent field containing an 'E' or 'e'
 *			followed by a possibly signed integer string.
 *
 *	RETURNS
 *		0 if failed, non-zero otherwise.
 */
{
    int n;
    char *pchar;

    if ((pchar = strchr(spec, ':')) == NULL) {
	return(XcmsFailure);
    }
    n = (int)(pchar - spec);

    /*
     * Check for proper prefix.
     */
    if (strncmp(spec, _XcmsCIELuv_prefix, (size_t)n) != 0) {
	return(XcmsFailure);
    }

    /*
     * Attempt to parse the value portion.
     */
    if (sscanf(spec + n + 1, "%lf/%lf/%lf",
	    &pColor->spec.CIELuv.L_star,
	    &pColor->spec.CIELuv.u_star,
	    &pColor->spec.CIELuv.v_star) != 3) {
        char *s; /* Maybe failed due to locale */
        int f;
        if ((s = strdup(spec))) {
            for (f = 0; s[f]; ++f)
                if (s[f] == '.')
                    s[f] = ',';
                else if (s[f] == ',')
                    s[f] = '.';
	    if (sscanf(s + n + 1, "%lf/%lf/%lf",
		       &pColor->spec.CIELuv.L_star,
		       &pColor->spec.CIELuv.u_star,
		       &pColor->spec.CIELuv.v_star) != 3) {
                free(s);
                return(XcmsFailure);
            }
            free(s);
        } else
	return(XcmsFailure);
    }
    pColor->format = XcmsCIELuvFormat;
    pColor->pixel = 0;
    return(XcmsCIELuv_ValidSpec(pColor));
}


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELuv_ValidSpec
 *
 *	SYNOPSIS
 */
static Status
XcmsCIELuv_ValidSpec(
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		Checks if color specification valid for CIE L*u*v*.
 *
 *	RETURNS
 *		XcmsFailure if invalid,
 *		XcmsSuccess if valid.
 *
 */
{
    if (pColor->format != XcmsCIELuvFormat
	    ||
	    (pColor->spec.CIELuv.L_star < 0.0 - XMY_DBL_EPSILON)
	    ||
	    (pColor->spec.CIELuv.L_star > 100.0 + XMY_DBL_EPSILON)) {
	return(XcmsFailure);
    }
    return(XcmsSuccess);
}


/*
 *	NAME
 *		XcmsCIELuvToCIEuvY - convert CIELuv to CIEuvY
 *
 *	SYNOPSIS
 */
Status
XcmsCIELuvToCIEuvY(
    XcmsCCC ccc,
    XcmsColor *pLuv_WhitePt,
    XcmsColor *pColors_in_out,
    unsigned int nColors)
/*
 *	DESCRIPTION
 *		Converts color specifications in an array of XcmsColor
 *		structures from CIELuv format to CIEuvY format.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded.
 *
 */
{
    XcmsColor	*pColor = pColors_in_out;
    XcmsColor	whitePt;
    XcmsCIEuvY	uvY_return;
    XcmsFloat	tmpVal;
    unsigned int i;

    /*
     * Check arguments
     */
    if (pLuv_WhitePt == NULL || pColors_in_out == NULL) {
	return(XcmsFailure);
    }

    /*
     * Make sure white point is in CIEuvY form
     */
    if (pLuv_WhitePt->format != XcmsCIEuvYFormat) {
	/* Make copy of the white point because we're going to modify it */
	memcpy((char *)&whitePt, (char *)pLuv_WhitePt, sizeof(XcmsColor));
	if (!_XcmsDIConvertColors(ccc, &whitePt, (XcmsColor *)NULL,
		1, XcmsCIEuvYFormat)) {
	    return(XcmsFailure);
	}
	pLuv_WhitePt = &whitePt;
    }
    /* Make sure it is a white point, i.e., Y == 1.0 */
    if (pLuv_WhitePt->spec.CIEuvY.Y != 1.0) {
	return(XcmsFailure);
    }

    /*
     * Now convert each XcmsColor structure to CIEXYZ form
     */
    for (i = 0; i < nColors; i++, pColor++) {

	/* Make sure original format is CIELuv and is valid */
	if (!XcmsCIELuv_ValidSpec(pColor)) {
	    return(XcmsFailure);
	}

	if (pColor->spec.CIELuv.L_star < 7.99953624) {
	    uvY_return.Y = pColor->spec.CIELuv.L_star / 903.29;
	} else {
	    tmpVal = (pColor->spec.CIELuv.L_star + 16.0) / 116.0;
	    uvY_return.Y = tmpVal * tmpVal * tmpVal; /* tmpVal ** 3 */
	}



	if (pColor->spec.CIELuv.L_star == 0.0) {
	    uvY_return.u_prime = pLuv_WhitePt->spec.CIEuvY.u_prime;
	    uvY_return.v_prime = pLuv_WhitePt->spec.CIEuvY.v_prime;
	} else {
	    tmpVal = 13.0 * (pColor->spec.CIELuv.L_star / 100.0);
	    uvY_return.u_prime = pColor->spec.CIELuv.u_star/tmpVal +
			    pLuv_WhitePt->spec.CIEuvY.u_prime;
	    uvY_return.v_prime = pColor->spec.CIELuv.v_star/tmpVal +
			    pLuv_WhitePt->spec.CIEuvY.v_prime;
	}
	/* Copy result to pColor */
	memcpy((char *)&pColor->spec, (char *)&uvY_return, sizeof(XcmsCIEuvY));

	/* Identify that the format is now CIEuvY */
	pColor->format = XcmsCIEuvYFormat;
    }
    return(XcmsSuccess);
}


/*
 *	NAME
 *		XcmsCIEuvYToCIELuv - convert CIEuvY to CIELuv
 *
 *	SYNOPSIS
 */
Status
XcmsCIEuvYToCIELuv(
    XcmsCCC ccc,
    XcmsColor *pLuv_WhitePt,
    XcmsColor *pColors_in_out,
    unsigned int nColors)
/*
 *	DESCRIPTION
 *		Converts color specifications in an array of XcmsColor
 *		structures from CIEuvY format to CIELab format.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded.
 *
 */
{
    XcmsColor	*pColor = pColors_in_out;
    XcmsColor	whitePt;
    XcmsCIELuv	Luv_return;
    XcmsFloat	tmpVal;
    unsigned int i;

    /*
     * Check arguments
     */
    if (pLuv_WhitePt == NULL || pColors_in_out == NULL) {
	return(XcmsFailure);
    }

    /*
     * Make sure white point is in CIEuvY form
     */
    if (pLuv_WhitePt->format != XcmsCIEuvYFormat) {
	/* Make copy of the white point because we're going to modify it */
	memcpy((char *)&whitePt, (char *)pLuv_WhitePt, sizeof(XcmsColor));
	if (!_XcmsDIConvertColors(ccc, &whitePt,
		(XcmsColor *)NULL, 1, XcmsCIEuvYFormat)) {
	    return(XcmsFailure);
	}
	pLuv_WhitePt = &whitePt;
    }
    /* Make sure it is a white point, i.e., Y == 1.0 */
    if (pLuv_WhitePt->spec.CIEuvY.Y != 1.0) {
	return(XcmsFailure);
    }

    /*
     * Now convert each XcmsColor structure to CIEXYZ form
     */
    for (i = 0; i < nColors; i++, pColor++) {

	if (!_XcmsCIEuvY_ValidSpec(pColor)) {
	    return(XcmsFailure);
	}

	/* Now convert the uvY to Luv */
	Luv_return.L_star =
	    (pColor->spec.CIEuvY.Y < 0.008856)
	    ?
	    (pColor->spec.CIEuvY.Y * 903.29)
	    :
	    ((XcmsFloat)(XCMS_CUBEROOT(pColor->spec.CIEuvY.Y) * 116.0) - 16.0);
	tmpVal = 13.0 * (Luv_return.L_star / 100.0);
	Luv_return.u_star = tmpVal *
	   (pColor->spec.CIEuvY.u_prime - pLuv_WhitePt->spec.CIEuvY.u_prime);
	Luv_return.v_star = tmpVal *
	   (pColor->spec.CIEuvY.v_prime - pLuv_WhitePt->spec.CIEuvY.v_prime);

	/* Copy result to pColor */
	memcpy((char *)&pColor->spec, (char *)&Luv_return, sizeof(XcmsCIELuv));

	/* Identify that the format is now CIEuvY */
	pColor->format = XcmsCIELuvFormat;
    }
    return(XcmsSuccess);
}
