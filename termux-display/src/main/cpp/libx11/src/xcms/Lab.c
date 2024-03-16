
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
 *	    CIELab.c
 *
 *	DESCRIPTION
 *		This file contains routines that support the CIE L*a*b*
 *		color space to include conversions to and from the CIE
 *		XYZ space.  These conversions are from Principles of
 *		Color Technology Second Edition, Fred W. Billmeyer, Jr.
 *		and Max Saltzman, John Wiley & Sons, Inc., 1981.
 *
 *		Note that the range for L* is 0 to 1.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xos.h>
#include <stdio.h> /* sscanf */
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"

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
#define DIV16BY116	0.137931

/*
 *	FORWARD DECLARATIONS
 */

static int CIELab_ParseString(register char *spec, XcmsColor *pColor);
static Status XcmsCIELab_ValidSpec(XcmsColor *pColor);


/*
 *	LOCAL VARIABLES
 */


    /*
     * NULL terminated list of functions applied to get from CIELab to CIEXYZ
     */
static XcmsConversionProc Fl_CIELab_to_CIEXYZ[] = {
    XcmsCIELabToCIEXYZ,
    NULL
};

    /*
     * NULL terminated list of functions applied to get from CIEXYZ to CIELab
     */
static XcmsConversionProc Fl_CIEXYZ_to_CIELab[] = {
    XcmsCIEXYZToCIELab,
    NULL
};


/*
 *	GLOBALS
 */
    /*
     * CIE Lab Color Space
     */
XcmsColorSpace	XcmsCIELabColorSpace =
    {
	_XcmsCIELab_prefix,	/* prefix */
	XcmsCIELabFormat,		/* id */
	CIELab_ParseString,	/* parseString */
	Fl_CIELab_to_CIEXYZ,	/* to_CIEXYZ */
	Fl_CIEXYZ_to_CIELab,	/* from_CIEXYZ */
	1
    };


/************************************************************************
 *									*
 *			 PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		CIELab_ParseString
 *
 *	SYNOPSIS
 */
static int
CIELab_ParseString(
    register char *spec,
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		This routines takes a string and attempts to convert
 *		it into a XcmsColor structure with XcmsCIELabFormat.
 *		The assumed CIELab string syntax is:
 *		    CIELab:<L>/<a>/<b>
 *		Where L, a, and b are in string input format for floats
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
    if (strncmp(spec, _XcmsCIELab_prefix, (size_t)n) != 0) {
	return(XcmsFailure);
    }

    /*
     * Attempt to parse the value portion.
     */
    if (sscanf(spec + n + 1, "%lf/%lf/%lf",
	    &pColor->spec.CIELab.L_star,
	    &pColor->spec.CIELab.a_star,
	    &pColor->spec.CIELab.b_star) != 3) {
        char *s; /* Maybe failed due to locale */
        int f;
        if ((s = strdup(spec))) {
            for (f = 0; s[f]; ++f)
                if (s[f] == '.')
                    s[f] = ',';
                else if (s[f] == ',')
                    s[f] = '.';
	    if (sscanf(s + n + 1, "%lf/%lf/%lf",
		       &pColor->spec.CIELab.L_star,
		       &pColor->spec.CIELab.a_star,
		       &pColor->spec.CIELab.b_star) != 3) {
                free(s);
                return(XcmsFailure);
            }
            free(s);
        } else
	    return(XcmsFailure);
    }
    pColor->format = XcmsCIELabFormat;
    pColor->pixel = 0;

    return(XcmsCIELab_ValidSpec(pColor));
}



/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELab_ValidSpec
 *
 *	SYNOPSIS
 */
static Status
XcmsCIELab_ValidSpec(
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		Checks if color specification valid for CIE L*a*b*.
 *
 *	RETURNS
 *		XcmsFailure if invalid,
 *		XcmsSuccess if valid.
 *
 */
{
    if (pColor->format != XcmsCIELabFormat
	    ||
	    (pColor->spec.CIELab.L_star < 0.0 - XMY_DBL_EPSILON)
	    ||
	    (pColor->spec.CIELab.L_star > 100.0 + XMY_DBL_EPSILON)) {
	return(XcmsFailure);
    }
    return(XcmsSuccess);
}


/*
 *	NAME
 *		XcmsCIELabToCIEXYZ - convert CIELab to CIEXYZ
 *
 *	SYNOPSIS
 */
Status
XcmsCIELabToCIEXYZ(
    XcmsCCC ccc,
    XcmsColor *pLab_WhitePt,
    XcmsColor *pColors_in_out,
    unsigned int nColors)
/*
 *	DESCRIPTION
 *		Converts color specifications in an array of XcmsColor
 *		structures from CIELab format to CIEXYZ format.
 *
 *		WARNING: This routine assumes that Yn = 1.0;
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded.
 *
 */
{
    XcmsCIEXYZ XYZ_return;
    XcmsFloat tmpFloat, tmpL;
    XcmsColor whitePt;
    unsigned int i;
    XcmsColor *pColor = pColors_in_out;

    /*
     * Check arguments
     */
    if (pLab_WhitePt == NULL || pColors_in_out == NULL) {
	return(XcmsFailure);
    }

    /*
     * Make sure white point is in CIEXYZ form, if not, convert it.
     */
    if (pLab_WhitePt->format != XcmsCIEXYZFormat) {
	/* Make a copy of the white point because we're going to modify it */
	memcpy((char *)&whitePt, (char *)pLab_WhitePt, sizeof(XcmsColor));
	if (!_XcmsDIConvertColors(ccc, &whitePt,
		(XcmsColor *)NULL, 1, XcmsCIEXYZFormat)) {
	    return(XcmsFailure);
	}
	pLab_WhitePt = &whitePt;
    }

    /*
     * Make sure it is a white point, i.e., Y == 1.0
     */
    if (pLab_WhitePt->spec.CIEXYZ.Y != 1.0) {
	return (0);
    }

    /*
     * Now convert each XcmsColor structure to CIEXYZ form
     */
    for (i = 0; i < nColors; i++, pColor++) {

	/* Make sure original format is CIELab */
	if (!XcmsCIELab_ValidSpec(pColor)) {
	    return(XcmsFailure);
	}

	/* Calculate Y: assume that Yn = 1.0 */
	tmpL = (pColor->spec.CIELab.L_star + 16.0) / 116.0;
	XYZ_return.Y = tmpL * tmpL * tmpL;

	if (XYZ_return.Y < 0.008856) {
	    /* Calculate Y: assume that Yn = 1.0 */
	    tmpL = pColor->spec.CIELab.L_star / 9.03292;

	    /* Calculate X */
	    XYZ_return.X = pLab_WhitePt->spec.CIEXYZ.X *
		    ((pColor->spec.CIELab.a_star / 3893.5) + tmpL);
	    /* Calculate Y */
	    XYZ_return.Y = tmpL;
	    /* Calculate Z */
	    XYZ_return.Z = pLab_WhitePt->spec.CIEXYZ.Z *
		    (tmpL - (pColor->spec.CIELab.b_star / 1557.4));
	} else {
	    /* Calculate X */
	    tmpFloat = tmpL + (pColor->spec.CIELab.a_star / 5.0);
	    XYZ_return.X = pLab_WhitePt->spec.CIEXYZ.X * tmpFloat * tmpFloat * tmpFloat;

	    /* Calculate Z */
	    tmpFloat = tmpL - (pColor->spec.CIELab.b_star / 2.0);
	    XYZ_return.Z = pLab_WhitePt->spec.CIEXYZ.Z * tmpFloat * tmpFloat * tmpFloat;
	}

	memcpy((char *)&pColor->spec.CIEXYZ, (char *)&XYZ_return,
	       sizeof(XcmsCIEXYZ));
	pColor->format = XcmsCIEXYZFormat;
    }

    return (1);
}


/*
 *	NAME
 *		XcmsCIEXYZToCIELab - convert CIEXYZ to CIELab
 *
 *	SYNOPSIS
 */
Status
XcmsCIEXYZToCIELab(
    XcmsCCC ccc,
    XcmsColor *pLab_WhitePt,
    XcmsColor *pColors_in_out,
    unsigned int nColors)
/*
 *	DESCRIPTION
 *		Converts color specifications in an array of XcmsColor
 *		structures from CIEXYZ format to CIELab format.
 *
 *		WARNING: This routine assumes that Yn = 1.0;
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded.
 *
 */
{
    XcmsCIELab Lab_return;
    XcmsFloat fX_Xn, fY_Yn, fZ_Zn;
    XcmsColor whitePt;
    unsigned int i;
    XcmsColor *pColor = pColors_in_out;

    /*
     * Check arguments
     */
    if (pLab_WhitePt == NULL || pColors_in_out == NULL) {
	return(XcmsFailure);
    }

    /*
     * Make sure white point is in CIEXYZ form, if not, convert it.
     */
    if (pLab_WhitePt->format != XcmsCIEXYZFormat) {
	/* Make a copy of the white point because we're going to modify it */
	memcpy((char *)&whitePt, (char *)pLab_WhitePt, sizeof(XcmsColor));
	if (!_XcmsDIConvertColors(ccc, &whitePt, (XcmsColor *)NULL,
		1, XcmsCIEXYZFormat)) {
	    return(XcmsFailure);
	}
	pLab_WhitePt = &whitePt;
    }

    /*
     * Make sure it is a white point, i.e., Y == 1.0
     */
    if (pLab_WhitePt->spec.CIEXYZ.Y != 1.0) {
	return(XcmsFailure);
    }

    /*
     * Now convert each XcmsColor structure to CIEXYZ form
     */
    for (i = 0; i < nColors; i++, pColor++) {

	/* Make sure original format is CIELab */
	if (!_XcmsCIEXYZ_ValidSpec(pColor)) {
	    return(XcmsFailure);
	}

	/* Calculate L*:  assume Yn = 1.0 */
	if (pColor->spec.CIEXYZ.Y < 0.008856) {
	    fY_Yn = (0.07787 * pColor->spec.CIEXYZ.Y) + DIV16BY116;
	    /* note fY_Yn used to compute Lab_return.a below */
	    Lab_return.L_star = 116.0 * (fY_Yn - DIV16BY116);
	} else {
	    fY_Yn = (XcmsFloat)XCMS_CUBEROOT(pColor->spec.CIEXYZ.Y);
	    /* note fY_Yn used to compute Lab_return.a_star below */
	    Lab_return.L_star = (116.0 * fY_Yn) - 16.0;
	}

	/* Calculate f(X/Xn) */
	if ((fX_Xn = pColor->spec.CIEXYZ.X / pLab_WhitePt->spec.CIEXYZ.X) < 0.008856) {
	    fX_Xn = (0.07787 * fX_Xn) + DIV16BY116;
	} else {
	    fX_Xn = (XcmsFloat) XCMS_CUBEROOT(fX_Xn);
	}

	/* Calculate f(Z/Zn) */
	if ((fZ_Zn = pColor->spec.CIEXYZ.Z / pLab_WhitePt->spec.CIEXYZ.Z) < 0.008856) {
	    fZ_Zn = (0.07787 * fZ_Zn) + DIV16BY116;
	} else {
	    fZ_Zn = (XcmsFloat) XCMS_CUBEROOT(fZ_Zn);
	}

	Lab_return.a_star = 5.0 * (fX_Xn - fY_Yn);
	Lab_return.b_star = 2.0 * (fY_Yn - fZ_Zn);

	memcpy((char *)&pColor->spec.CIELab, (char *)&Lab_return,
	       sizeof(XcmsCIELab));
	pColor->format = XcmsCIELabFormat;
    }

    return(XcmsSuccess);
}
