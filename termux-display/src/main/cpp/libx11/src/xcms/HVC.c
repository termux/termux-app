
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
 *		TekHVC.c
 *
 *	DESCRIPTION
 *		This file contains routines that support the TekHVC
 *		color space to include conversions to and from the CIE
 *		XYZ space.
 *
 *	DOCUMENTATION
 *		"TekColor Color Management System, System Implementor's Manual"
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"
#include <X11/Xos.h>
#include <math.h>
#include "Cv.h"

#include <stdio.h>

/*
 *	DEFINES
 */
#define u_BR    0.7127          /* u' Best Red */
#define v_BR    0.4931          /* v' Best Red */
#define EPS     0.001
#define CHROMA_SCALE_FACTOR   7.50725
#ifndef PI
#  ifdef M_PI
#    define PI	M_PI
#  else
#    define PI       3.14159265358979323846264338327950
#  endif
#endif
#ifndef degrees
#  define degrees(r) ((XcmsFloat)(r) * 180.0 / PI)
#endif /* degrees */
#ifndef radians
#  define radians(d) ((XcmsFloat)(d) * PI / 180.0)
#endif /* radians */

/*************************************************************************
 * Note: The DBL_EPSILON for ANSI is 1e-5 so my checks need to take
 *       this into account.  If your DBL_EPSILON is different then
 *       adjust this define.
 *
 *       Also note that EPS is the error factor in the calculations
 *       This may need to be the same as XMY_DBL_EPSILON in
 *       some implementations.
 **************************************************************************/
#ifdef DBL_EPSILON
#  define XMY_DBL_EPSILON DBL_EPSILON
#else
#  define XMY_DBL_EPSILON 0.00001
#endif

/*
 *	FORWARD DECLARATIONS
 */
static int TekHVC_ParseString(register char *spec, XcmsColor *pColor);
static Status XcmsTekHVC_ValidSpec(XcmsColor *pColor);

/*
 *	LOCAL VARIABLES
 */

    /*
     * NULL terminated list of functions applied to get from TekHVC to CIEXYZ
     */
static XcmsConversionProc Fl_TekHVC_to_CIEXYZ[] = {
    XcmsTekHVCToCIEuvY,
    XcmsCIEuvYToCIEXYZ,
    NULL
};

    /*
     * NULL terminated list of functions applied to get from CIEXYZ to TekHVC
     */
static XcmsConversionProc Fl_CIEXYZ_to_TekHVC[] = {
    XcmsCIEXYZToCIEuvY,
    XcmsCIEuvYToTekHVC,
    NULL
};

/*
 *	GLOBALS
 */

    /*
     * TekHVC Color Space
     */
XcmsColorSpace	XcmsTekHVCColorSpace =
    {
	_XcmsTekHVC_prefix,	/* prefix */
	XcmsTekHVCFormat,		/* id */
	TekHVC_ParseString,	/* parseString */
	Fl_TekHVC_to_CIEXYZ,	/* to_CIEXYZ */
	Fl_CIEXYZ_to_TekHVC,	/* from_CIEXYZ */
	1
    };




/************************************************************************
 *									*
 *			 PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		TekHVC_ParseString
 *
 *	SYNOPSIS
 */
static int
TekHVC_ParseString(
    register char *spec,
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		This routines takes a string and attempts to convert
 *		it into a XcmsColor structure with XcmsTekHVCFormat.
 *		The assumed TekHVC string syntax is:
 *		    TekHVC:<H>/<V>/<C>
 *		Where H, V, and C are in string input format for floats
 *		consisting of:
 *		    a. an optional sign
 *		    b. a string of numbers possibly containing a decimal point,
 *		    c. an optional exponent field containing an 'E' or 'e'
 *			followed by a possibly signed integer string.
 *
 *	RETURNS
 *		XcmsFailure if invalid;
 *		XcmsSuccess if valid.
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
    if (strncmp(spec, _XcmsTekHVC_prefix, n) != 0) {
	return(XcmsFailure);
    }

    /*
     * Attempt to parse the value portion.
     */
    if (sscanf(spec + n + 1, "%lf/%lf/%lf",
	    &pColor->spec.TekHVC.H,
	    &pColor->spec.TekHVC.V,
	    &pColor->spec.TekHVC.C) != 3) {
        char *s; /* Maybe failed due to locale */
        int f;
        if ((s = strdup(spec))) {
            for (f = 0; s[f]; ++f)
                if (s[f] == '.')
                    s[f] = ',';
                else if (s[f] == ',')
                    s[f] = '.';
	    if (sscanf(s + n + 1, "%lf/%lf/%lf",
		       &pColor->spec.TekHVC.H,
		       &pColor->spec.TekHVC.V,
		       &pColor->spec.TekHVC.C) != 3) {
                free(s);
                return(XcmsFailure);
            }
            free(s);
        } else
	    return(XcmsFailure);
    }
    pColor->format = XcmsTekHVCFormat;
    pColor->pixel = 0;
    return(XcmsTekHVC_ValidSpec(pColor));
}


/*
 *	NAME
 *		ThetaOffset -- compute thetaOffset
 *
 *	SYNOPSIS
 */
static int
ThetaOffset(
    XcmsColor *pWhitePt,
    XcmsFloat *pThetaOffset)
/*
 *	DESCRIPTION
 *		This routine computes the theta offset of a given
 *		white point, i.e. XcmsColor.  It is used in both this
 *		conversion and the printer conversions.
 *
 *	RETURNS
 *		0 if failed.
 *		1 if succeeded with no modifications.
 *
 *	ASSUMPTIONS
 *		Assumes:
 *			pWhitePt != NULL
 *			pWhitePt->format == XcmsCIEuvYFormat
 *
 */
{
    double div, slopeuv;

    if (pWhitePt == NULL || pWhitePt->format != XcmsCIEuvYFormat) {
	return(0);
    }

    if ((div = u_BR - pWhitePt->spec.CIEuvY.u_prime) == 0.0) {
	return(0);
    }
    slopeuv = (v_BR - pWhitePt->spec.CIEuvY.v_prime) / div;
    *pThetaOffset = degrees(XCMS_ATAN(slopeuv));
    return(1);
}



/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsTekHVC_ValidSpec()
 *
 *	SYNOPSIS
 */
static int
XcmsTekHVC_ValidSpec(
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		Checks if values in the color specification are valid.
 *		Also brings hue into the range 0.0 <= Hue < 360.0
 *
 *	RETURNS
 *		0 if not valid.
 *		1 if valid.
 *
 */
{
    if (pColor->format != XcmsTekHVCFormat) {
	return(XcmsFailure);
    }
    if (pColor->spec.TekHVC.V < (0.0 - XMY_DBL_EPSILON)
	    || pColor->spec.TekHVC.V > (100.0 + XMY_DBL_EPSILON)
	    || (pColor->spec.TekHVC.C < 0.0 - XMY_DBL_EPSILON)) {
	return(XcmsFailure);
    }

    if (pColor->spec.TekHVC.V < 0.0) {
	    pColor->spec.TekHVC.V = 0.0 + XMY_DBL_EPSILON;
    } else if (pColor->spec.TekHVC.V > 100.0) {
	pColor->spec.TekHVC.V = 100.0 - XMY_DBL_EPSILON;
    }

    if (pColor->spec.TekHVC.C < 0.0) {
	pColor->spec.TekHVC.C = 0.0 - XMY_DBL_EPSILON;
    }

    while (pColor->spec.TekHVC.H < 0.0) {
	pColor->spec.TekHVC.H += 360.0;
    }
    while (pColor->spec.TekHVC.H >= 360.0) {
	pColor->spec.TekHVC.H -= 360.0;
    }
    return(XcmsSuccess);
}

/*
 *	NAME
 *		XcmsTekHVCToCIEuvY - convert TekHVC to CIEuvY
 *
 *	SYNOPSIS
 */
Status
XcmsTekHVCToCIEuvY(
    XcmsCCC ccc,
    XcmsColor *pHVC_WhitePt,
    XcmsColor *pColors_in_out,
    unsigned int nColors)
/*
 *	DESCRIPTION
 *		Transforms an array of TekHVC color specifications, given
 *		their associated white point, to CIECIEuvY.color
 *		specifications.
 *
 *	RETURNS
 *		XcmsFailure if failed, XcmsSuccess otherwise.
 *
 */
{
    XcmsFloat	thetaOffset;
    XcmsColor	*pColor = pColors_in_out;
    XcmsColor	whitePt;
    XcmsCIEuvY	uvY_return;
    XcmsFloat	tempHue, u, v;
    XcmsFloat	tmpVal;
    unsigned int i;

    /*
     * Check arguments
     */
    if (pHVC_WhitePt == NULL || pColors_in_out == NULL) {
	return(XcmsFailure);
    }

    /*
     * Make sure white point is in CIEuvY form
     */
    if (pHVC_WhitePt->format != XcmsCIEuvYFormat) {
	/* Make copy of the white point because we're going to modify it */
	memcpy((char *)&whitePt, (char *)pHVC_WhitePt, sizeof(XcmsColor));
	if (!_XcmsDIConvertColors(ccc, &whitePt, (XcmsColor *)NULL, 1,
		XcmsCIEuvYFormat)) {
	    return(XcmsFailure);
	}
	pHVC_WhitePt = &whitePt;
    }
    /* Make sure it is a white point, i.e., Y == 1.0 */
    if (pHVC_WhitePt->spec.CIEuvY.Y != 1.0) {
	return(XcmsFailure);
    }

    /* Get the thetaOffset */
    if (!ThetaOffset(pHVC_WhitePt, &thetaOffset)) {
	return(XcmsFailure);
    }

    /*
     * Now convert each XcmsColor structure to CIEXYZ form
     */
    for (i = 0; i < nColors; i++, pColor++) {

	/* Make sure original format is TekHVC and is valid */
	if (!XcmsTekHVC_ValidSpec(pColor)) {
	    return(XcmsFailure);
	}

	if (pColor->spec.TekHVC.V == 0.0 || pColor->spec.TekHVC.V == 100.0) {
	    if (pColor->spec.TekHVC.V == 100.0) {
		uvY_return.Y = 1.0;
	    } else { /* pColor->spec.TekHVC.V == 0.0 */
		uvY_return.Y = 0.0;
	    }
	    uvY_return.u_prime = pHVC_WhitePt->spec.CIEuvY.u_prime;
	    uvY_return.v_prime = pHVC_WhitePt->spec.CIEuvY.v_prime;
	} else {

	    /* Find the hue based on the white point offset */
	    tempHue = pColor->spec.TekHVC.H + thetaOffset;

	    while (tempHue < 0.0) {
		tempHue += 360.0;
	    }
	    while (tempHue >= 360.0) {
		tempHue -= 360.0;
	    }

	    tempHue = radians(tempHue);

	    /* Calculate u'v' for the obtained hue */
	    u = (XcmsFloat) ((XCMS_COS(tempHue) * pColor->spec.TekHVC.C) /
		    (pColor->spec.TekHVC.V * (double)CHROMA_SCALE_FACTOR));
	    v = (XcmsFloat) ((XCMS_SIN(tempHue) * pColor->spec.TekHVC.C) /
		    (pColor->spec.TekHVC.V * (double)CHROMA_SCALE_FACTOR));

	    /* Based on the white point get the offset from best red */
	    uvY_return.u_prime = u + pHVC_WhitePt->spec.CIEuvY.u_prime;
	    uvY_return.v_prime = v + pHVC_WhitePt->spec.CIEuvY.v_prime;

	    /* Calculate the Y value based on the L* = V. */
	    if (pColor->spec.TekHVC.V < 7.99953624) {
		uvY_return.Y = pColor->spec.TekHVC.V / 903.29;
	    } else {
		tmpVal = (pColor->spec.TekHVC.V + 16.0) / 116.0;
		uvY_return.Y = tmpVal * tmpVal * tmpVal; /* tmpVal ** 3 */
	    }
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
 *		XcmsCIEuvYToTekHVC - convert CIEuvY to TekHVC
 *
 *	SYNOPSIS
 */
Status
XcmsCIEuvYToTekHVC(
    XcmsCCC ccc,
    XcmsColor *pHVC_WhitePt,
    XcmsColor *pColors_in_out,
    unsigned int nColors)
/*
 *	DESCRIPTION
 *		Transforms an array of CIECIEuvY.color specifications, given
 *		their assiciated white point, to TekHVC specifications.
 *
 *	RETURNS
 *		XcmsFailure if failed, XcmsSuccess otherwise.
 *
 */
{
    XcmsFloat	theta, L2, u, v, nThetaLow, nThetaHigh;
    XcmsFloat	thetaOffset;
    XcmsColor	*pColor = pColors_in_out;
    XcmsColor	whitePt;
    XcmsTekHVC	HVC_return;
    unsigned int i;

    /*
     * Check arguments
     */
    if (pHVC_WhitePt == NULL || pColors_in_out == NULL) {
	return(XcmsFailure);
    }

    /*
     * Make sure white point is in CIEuvY form
     */
    if (pHVC_WhitePt->format != XcmsCIEuvYFormat) {
	/* Make copy of the white point because we're going to modify it */
	memcpy((char *)&whitePt, (char *)pHVC_WhitePt, sizeof(XcmsColor));
	if (!_XcmsDIConvertColors(ccc, &whitePt, (XcmsColor *)NULL, 1,
		XcmsCIEuvYFormat)) {
	    return(XcmsFailure);
	}
	pHVC_WhitePt = &whitePt;
    }
    /* Make sure it is a white point, i.e., Y == 1.0 */
    if (pHVC_WhitePt->spec.CIEuvY.Y != 1.0) {
	return(XcmsFailure);
    }
    if (!ThetaOffset(pHVC_WhitePt, &thetaOffset)) {
	return(XcmsFailure);
    }

    /*
     * Now convert each XcmsColor structure to CIEXYZ form
     */
    for (i = 0; i < nColors; i++, pColor++) {
	if (!_XcmsCIEuvY_ValidSpec(pColor)) {
	    return(XcmsFailure);
	}

	/* Use the white point offset to determine HVC */
	u = pColor->spec.CIEuvY.u_prime - pHVC_WhitePt->spec.CIEuvY.u_prime;
	v = pColor->spec.CIEuvY.v_prime - pHVC_WhitePt->spec.CIEuvY.v_prime;

	/* Calculate the offset */
	if (u == 0.0) {
	    theta = 0.0;
	} else {
	    theta = v / u;
	    theta = (XcmsFloat) XCMS_ATAN((double)theta);
	    theta = degrees(theta);
	}

	nThetaLow = 0.0;
	nThetaHigh = 360.0;
	if (u > 0.0 && v > 0.0) {
	    nThetaLow = 0.0;
	    nThetaHigh = 90.0;
	} else if (u < 0.0 && v > 0.0) {
	    nThetaLow = 90.0;
	    nThetaHigh = 180.0;
	} else if (u < 0.0 && v < 0.0) {
	    nThetaLow = 180.0;
	    nThetaHigh = 270.0;
	} else if (u > 0.0 && v < 0.0) {
	    nThetaLow = 270.0;
	    nThetaHigh = 360.0;
	}
	while (theta < nThetaLow) {
		theta += 90.0;
	}
	while (theta >= nThetaHigh) {
	    theta -= 90.0;
	}

	/* calculate the L value from the given Y */
	L2 = (pColor->spec.CIEuvY.Y < 0.008856)
	    ?
	    (pColor->spec.CIEuvY.Y * 903.29)
	    :
	    ((XcmsFloat)(XCMS_CUBEROOT(pColor->spec.CIEuvY.Y) * 116.0) - 16.0);
	HVC_return.C = L2 * CHROMA_SCALE_FACTOR * XCMS_SQRT((double) ((u * u) + (v * v)));
	if (HVC_return.C < 0.0) {
	    theta = 0.0;
	}
	HVC_return.V = L2;
	HVC_return.H = theta - thetaOffset;

	/*
	 * If this is within the error margin let some other routine later
	 * in the chain worry about the slop in the calculations.
	 */
	while (HVC_return.H < -EPS) {
	    HVC_return.H += 360.0;
	}
	while (HVC_return.H >= 360.0 + EPS) {
	    HVC_return.H -= 360.0;
	}

	/* Copy result to pColor */
	memcpy((char *)&pColor->spec, (char *)&HVC_return, sizeof(XcmsTekHVC));

	/* Identify that the format is now CIEuvY */
	pColor->format = XcmsTekHVCFormat;
    }
    return(XcmsSuccess);
}


/*
 *	NAME
 *		_XcmsTekHVC_CheckModify
 *
 *	SYNOPSIS
 */
int
_XcmsTekHVC_CheckModify(
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		Checks if values in the color specification are valid.
 *		If they are not it modifies the values.
 *		Also brings hue into the range 0.0 <= Hue < 360.0
 *
 *	RETURNS
 *		0 if not valid.
 *		1 if valid.
 *
 */
{
    int n;

    /* For now only use the TekHVC numbers as inputs */
    if (pColor->format != XcmsTekHVCFormat) {
	return(0);
    }

    if (pColor->spec.TekHVC.V < 0.0) {
	pColor->spec.TekHVC.V = 0.0 + XMY_DBL_EPSILON;
    } else if (pColor->spec.TekHVC.V > 100.0) {
	pColor->spec.TekHVC.V = 100.0 - XMY_DBL_EPSILON;
    }

    if (pColor->spec.TekHVC.C < 0.0) {
	pColor->spec.TekHVC.C = 0.0 - XMY_DBL_EPSILON;
    }

    if (pColor->spec.TekHVC.H < 0.0) {
	n = -pColor->spec.TekHVC.H / 360.0;
	pColor->spec.TekHVC.H += (n + 1) * 360.0;
	if (pColor->spec.TekHVC.H >= 360.0)
	    pColor->spec.TekHVC.H -= 360.0;
    } else if (pColor->spec.TekHVC.H >= 360.0) {
	n = pColor->spec.TekHVC.H / 360.0;
	pColor->spec.TekHVC.H -= n * 360.0;
    }
    return(1);
}
