
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
 *		XcmsCvCols.c
 *
 *	DESCRIPTION
 *		Xcms API routine that converts between the
 *		device-independent color spaces.
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

/*
 *      LOCAL DEFINES
 */
#define	DD_FORMAT	0x01
#define	DI_FORMAT	0x02
#define	MIX_FORMAT	0x04
#ifndef MAX
#  define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif


/************************************************************************
 *									*
 *			 PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		EqualCIEXYZ
 *
 *	SYNOPSIS
 */
static int
EqualCIEXYZ(
    XcmsColor *p1, XcmsColor *p2)
/*
 *	DESCRIPTION
 *		Compares two XcmsColor structures that are in XcmsCIEXYZFormat
 *
 *	RETURNS
 *		Returns 1 if equal; 0 otherwise.
 *
 */
{
    if (p1->format != XcmsCIEXYZFormat || p2->format != XcmsCIEXYZFormat) {
	return(0);
    }
    if ((p1->spec.CIEXYZ.X != p2->spec.CIEXYZ.X)
	    || (p1->spec.CIEXYZ.Y != p2->spec.CIEXYZ.Y)
	    || (p1->spec.CIEXYZ.Z != p2->spec.CIEXYZ.Z)) {
	return(0);
    }
    return(1);
}


/*
 *	NAME
 *		XcmsColorSpace
 *
 *	SYNOPSIS
 */
static XcmsColorSpace *
ColorSpaceOfID(
    XcmsCCC ccc,
    XcmsColorFormat	id)
/*
 *	DESCRIPTION
 *		Returns a pointer to the color space structure
 *		(XcmsColorSpace) associated with the specified color space
 *		ID.
 *
 *	RETURNS
 *		Pointer to matching XcmsColorSpace structure if found;
 *		otherwise NULL.
 */
{
    XcmsColorSpace	**papColorSpaces;

    if (ccc == NULL) {
	return(NULL);
    }

    /*
     * First try Device-Independent color spaces
     */
    papColorSpaces = _XcmsDIColorSpaces;
    if (papColorSpaces != NULL) {
	while (*papColorSpaces != NULL) {
	    if ((*papColorSpaces)->id == id) {
		return(*papColorSpaces);
	    }
	    papColorSpaces++;
	}
    }

    /*
     * Next try Device-Dependent color spaces
     */
    papColorSpaces = ((XcmsFunctionSet *)ccc->pPerScrnInfo->functionSet)->DDColorSpaces;
    if (papColorSpaces != NULL) {
	while (*papColorSpaces != NULL) {
	    if ((*papColorSpaces)->id == id) {
		return(*papColorSpaces);
	    }
	    papColorSpaces++;
	}
    }

    return(NULL);
}


/*
 *	NAME
 *		ValidDIColorSpaceID
 *
 *	SYNOPSIS
 */
static int
ValidDIColorSpaceID(
    XcmsColorFormat id)
/*
 *	DESCRIPTION
 *		Determines if the specified color space ID is a valid
 *		Device-Independent color space in the specified Color
 *		Conversion Context.
 *
 *	RETURNS
 *		Returns zero if not valid; otherwise non-zero.
 */
{
    XcmsColorSpace **papRec;
    papRec = _XcmsDIColorSpaces;
    if (papRec != NULL) {
	while (*papRec != NULL) {
	    if ((*papRec)->id == id) {
		return(1);
	    }
	    papRec++;
	}
    }
    return(0);
}


/*
 *	NAME
 *		ValidDDColorSpaceID
 *
 *	SYNOPSIS
 */
static int
ValidDDColorSpaceID(
    XcmsCCC ccc,
    XcmsColorFormat id)
/*
 *	DESCRIPTION
 *		Determines if the specified color space ID is a valid
 *		Device-Dependent color space in the specified Color
 *		Conversion Context.
 *
 *	RETURNS
 *		Returns zero if not valid; otherwise non-zero.
 */
{
    XcmsColorSpace **papRec;

    if (ccc->pPerScrnInfo->state != XcmsInitNone) {
	papRec = ((XcmsFunctionSet *)ccc->pPerScrnInfo->functionSet)->DDColorSpaces;
	while (*papRec != NULL) {
	    if ((*papRec)->id == id) {
		return(1);
	    }
	    papRec++;
	}
    }
    return(0);
}


/*
 *	NAME
 *		ConvertMixedColors - Convert XcmsColor structures
 *
 *	SYNOPSIS
 */
static Status
ConvertMixedColors(
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    XcmsColor *pWhitePt,
    unsigned int nColors,
    XcmsColorFormat targetFormat,
    unsigned char format_flag)
/*
 *	DESCRIPTION
 *		This routine will only convert the following types of
 *		batches:
 *			DI to DI
 *			DD to DD
 *			DD to CIEXYZ
 *		In other words, it will not convert the following types of
 *		batches:
 *			DI to DD
 *			DD to DI(not CIEXYZ)
 *
 *		format_flag:
 *		    0x01 : convert Device-Dependent only specifications to the
 *			target format.
 *		    0x02 : convert Device-Independent only specifications to the
 *			target format.
 *		    0x03 : convert all specifications to the target format.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if none of the color specifications were
 *			compressed in the conversion process
 *		XcmsSuccessWithCompression if at least one of the
 *			color specifications were compressed in the
 *			conversion process.
 *
 */
{
    XcmsColor *pColor, *pColors_start;
    XcmsColorFormat format;
    Status retval_tmp;
    Status retval = XcmsSuccess;
    unsigned int iColors;
    unsigned int nBatch;

    /*
     * Convert array of mixed color specifications in batches of
     * contiguous formats to the target format
     */
    iColors = 0;
    while (iColors < nColors) {
	/*
	 * Find contiguous array of color specifications with the
	 * same format
	 */
	pColor = pColors_start = pColors_in_out + iColors;
	format = pColors_start->format;
	nBatch = 0;
	while (iColors < nColors && pColor->format == format) {
		pColor++;
		nBatch++;
		iColors++;
	}
	if (format != targetFormat) {
	    /*
	     * Need to convert this batch from current format to target format.
	     */
	    if (XCMS_DI_ID(format) && (format_flag & DI_FORMAT) &&
		XCMS_DI_ID(targetFormat)) {
		/*
		 * DI->DI
		 *
		 * Format of interest is Device-Independent,
		 * This batch contains Device-Independent specifications, and
		 * the Target format is Device-Independent.
		 */
		retval_tmp = _XcmsDIConvertColors(ccc, pColors_start, pWhitePt,
			nBatch, targetFormat);
	    } else if (XCMS_DD_ID(format) && (format_flag & DD_FORMAT) &&
		    (targetFormat == XcmsCIEXYZFormat)) {
		/*
		 * DD->CIEXYZ
		 *
		 * Format of interest is Device-Dependent,
		 * This batch contains Device-Dependent specifications, and
		 * the Target format is CIEXYZ.
		 *
		 * Since DD->CIEXYZ we can use NULL instead of pCompressed.
		 */
		if ((ccc->whitePtAdjProc != NULL) && !_XcmsEqualWhitePts(ccc,
			pWhitePt, ScreenWhitePointOfCCC(ccc))) {
		    /*
		     * Need to call WhiteAdjustProc (Screen White Point to
		     *   White Point).
		     */
		    retval_tmp = (*ccc->whitePtAdjProc)(ccc,
			    ScreenWhitePointOfCCC(ccc), pWhitePt,
			    XcmsCIEXYZFormat, pColors_start, nBatch,
			    (Bool *)NULL);
		} else {
		    retval_tmp = _XcmsDDConvertColors(ccc, pColors_start,
			    nBatch, XcmsCIEXYZFormat, (Bool *)NULL);
		}
	    } else if (XCMS_DD_ID(format) && (format_flag & DD_FORMAT) &&
		    XCMS_DD_ID(targetFormat)) {
		/*
		 * DD->DD(not CIEXYZ)
		 *
		 * Format of interest is Device-Dependent,
		 * This batch contains Device-Dependent specifications, and
		 * the Target format is Device-Dependent and not CIEXYZ.
		 */
		retval_tmp = _XcmsDDConvertColors(ccc, pColors_start, nBatch,
			targetFormat, (Bool *)NULL);
	    } else {
		/*
		 * This routine is called for the wrong reason.
		 */
		return(XcmsFailure);
	    }
	    if (retval_tmp == XcmsFailure) {
		return(XcmsFailure);
	    }
	    retval = MAX(retval, retval_tmp);
	}
    }
    return(retval);
}


/************************************************************************
 *									*
 *			 API PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		_XcmsEqualWhitePts
 *
 *	SYNOPSIS
 */
int
_XcmsEqualWhitePts(XcmsCCC ccc, XcmsColor *pWhitePt1, XcmsColor *pWhitePt2)
/*
 *	DESCRIPTION
 *
 *	RETURNS
 *		Returns 0 if not equal; otherwise 1.
 *
 */
{
    XcmsColor tmp1, tmp2;

    memcpy((char *)&tmp1, (char *)pWhitePt1, sizeof(XcmsColor));
    memcpy((char *)&tmp2, (char *)pWhitePt2, sizeof(XcmsColor));

    if (tmp1.format != XcmsCIEXYZFormat) {
	if (_XcmsDIConvertColors(ccc, &tmp1, (XcmsColor *) NULL, 1,
		XcmsCIEXYZFormat)==0) {
	    return(0);
	}
    }

    if (tmp2.format != XcmsCIEXYZFormat) {
	if (_XcmsDIConvertColors(ccc, &tmp2, (XcmsColor *) NULL, 1,
		XcmsCIEXYZFormat)==0) {
	    return(0);
	}
    }

    return (EqualCIEXYZ(&tmp1, &tmp2));
}


/*
 *	NAME
 *		_XcmsDIConvertColors - Convert XcmsColor structures
 *
 *	SYNOPSIS
 */
Status
_XcmsDIConvertColors(
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    XcmsColor *pWhitePt,
    unsigned int nColors,
    XcmsColorFormat newFormat)
/*
 *	DESCRIPTION
 *		Convert XcmsColor structures to another Device-Independent
 *		form.
 *
 *		Here are some assumptions that this routine makes:
 *		1. The calling routine has already checked if
 *		    pColors_in_out->format == newFormat, therefore
 *		    there is no need to check again here.
 *		2. The calling routine has already checked nColors,
 *		    therefore this routine assumes nColors > 0.
 *		3. The calling routine may want to convert only between
 *			CIExyY <-> CIEXYZ <-> CIEuvY
 *		    therefore, this routine allows pWhitePt to equal NULL.
 *
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded.
 *
 */
{
    XcmsColorSpace *pFrom, *pTo;
    XcmsDIConversionProc *src_to_CIEXYZ, *src_from_CIEXYZ;
    XcmsDIConversionProc *dest_to_CIEXYZ, *dest_from_CIEXYZ;
    XcmsDIConversionProc *to_CIEXYZ_stop, *from_CIEXYZ_start;
    XcmsDIConversionProc *tmp;

    /*
     * Allow pWhitePt to equal NULL.  This appropriate when converting
     *    anywhere between:
     *		CIExyY <-> CIEXYZ <-> CIEuvY
     */

    if (pColors_in_out == NULL ||
	    !ValidDIColorSpaceID(pColors_in_out->format) ||
	    !ValidDIColorSpaceID(newFormat)) {
	return(XcmsFailure);
    }

    /*
     * Get a handle on the function list for the current specification format
     */
    if ((pFrom = ColorSpaceOfID(ccc, pColors_in_out->format))
	    == NULL) {
	return(XcmsFailure);
    }

    /*
     * Get a handle on the function list for the new specification format
     */
    if ((pTo = ColorSpaceOfID(ccc, newFormat)) == NULL) {
	return(XcmsFailure);
    }

    src_to_CIEXYZ = pFrom->to_CIEXYZ;
    src_from_CIEXYZ = pFrom->from_CIEXYZ;
    dest_to_CIEXYZ = pTo->to_CIEXYZ;
    dest_from_CIEXYZ = pTo->from_CIEXYZ;

    if (pTo->inverse_flag && pFrom->inverse_flag) {
	/*
	 * Find common function pointers
	 */
	for (to_CIEXYZ_stop = src_to_CIEXYZ; *to_CIEXYZ_stop; to_CIEXYZ_stop++){
	    for (tmp = dest_to_CIEXYZ; *tmp; tmp++) {
		if (*to_CIEXYZ_stop == *tmp) {
		    goto Continue;
		}
	    }
	}

Continue:

	/*
	 * Execute the functions to CIEXYZ, stopping short as necessary
	 */
	while (src_to_CIEXYZ != to_CIEXYZ_stop) {
	    if ((*src_to_CIEXYZ++)(ccc, pWhitePt, pColors_in_out,
		    nColors) == XcmsFailure) {
		return(XcmsFailure);
	    }
	}

	/*
	 * Determine where to start on the from_CIEXYZ path.
	 */
	from_CIEXYZ_start = dest_from_CIEXYZ;
	tmp = src_from_CIEXYZ;
	while ((*from_CIEXYZ_start == *tmp) && (*from_CIEXYZ_start != NULL)) {
	    from_CIEXYZ_start++;
	    tmp++;
	}

    } else {
	/*
	 * The function in at least one of the Color Spaces are not
	 * complementary, i.e.,
	 *	for an i, 0 <= i < n elements
	 *	from_CIEXYZ[i] is not the inverse of to_CIEXYZ[i]
	 *
	 * Execute the functions all the way to CIEXYZ
	 */
	while (*src_to_CIEXYZ) {
	    if ((*src_to_CIEXYZ++)(ccc, pWhitePt, pColors_in_out,
		    nColors) == XcmsFailure) {
		return(XcmsFailure);
	    }
	}

	/*
	 * Determine where to start on the from_CIEXYZ path.
	 */
	from_CIEXYZ_start = dest_from_CIEXYZ;
    }


    /*
     * Execute the functions from CIEXYZ.
     */
    while (*from_CIEXYZ_start) {
	if ((*from_CIEXYZ_start++)(ccc, pWhitePt, pColors_in_out,
		nColors) == XcmsFailure) {
	    return(XcmsFailure);
	}
    }

    return(XcmsSuccess);
}


/*
 *	NAME
 *		_XcmsDDConvertColors - Convert XcmsColor structures
 *
 *	SYNOPSIS
 */
Status
_XcmsDDConvertColors(
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    unsigned int nColors,
    XcmsColorFormat newFormat,
    Bool *pCompressed)
/*
 *	DESCRIPTION
 *		Convert XcmsColor structures:
 *
 *		1. From CIEXYZ to Device-Dependent formats (typically RGB and
 *			RGBi),
 *		    or
 *		2. Between Device-Dependent formats (typically RGB and RGBi).
 *
 *		Assumes that these specifications have already been white point
 *		adjusted if necessary from Client White Point to Screen
 *		White Point.  Therefore, the white point now associated
 *		with the specifications is the Screen White Point.
 *
 *		pCompressed may be NULL.  If so this indicates that the
 *		calling routine is not interested in knowing exactly which
 *		color was compressed, if any.
 *
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if none of the color specifications were
 *			compressed in the conversion process
 *		XcmsSuccessWithCompression if at least one of the
 *			color specifications were compressed in the
 *			conversion process.
 *
 */
{
    XcmsColorSpace *pFrom, *pTo;
    XcmsDDConversionProc *src_to_CIEXYZ, *src_from_CIEXYZ;
    XcmsDDConversionProc *dest_to_CIEXYZ, *dest_from_CIEXYZ;
    XcmsDDConversionProc *from_CIEXYZ_start, *to_CIEXYZ_stop;
    XcmsDDConversionProc *tmp;
    int	retval;
    int hasCompressed = 0;

    if (ccc == NULL || pColors_in_out == NULL) {
	return(XcmsFailure);
    }

    if (nColors == 0 || pColors_in_out->format == newFormat) {
	/* do nothing */
	return(XcmsSuccess);
    }

    if (((XcmsFunctionSet *)ccc->pPerScrnInfo->functionSet) == NULL) {
	return(XcmsFailure);	/* hmm, an internal error? */
    }

    /*
     * Its ok if pColors_in_out->format == XcmsCIEXYZFormat
     *	or
     * if newFormat == XcmsCIEXYZFormat
     */
    if ( !( ValidDDColorSpaceID(ccc, pColors_in_out->format)
	    ||
	    (pColors_in_out->format == XcmsCIEXYZFormat))
	 ||
	 !(ValidDDColorSpaceID(ccc, newFormat)
	    ||
	    newFormat == XcmsCIEXYZFormat)) {
	return(XcmsFailure);
    }

    if ((pFrom = ColorSpaceOfID(ccc, pColors_in_out->format)) == NULL){
	return(XcmsFailure);
    }

    if ((pTo = ColorSpaceOfID(ccc, newFormat)) == NULL) {
	return(XcmsFailure);
    }

    src_to_CIEXYZ = (XcmsDDConversionProc *)pFrom->to_CIEXYZ;
    src_from_CIEXYZ = (XcmsDDConversionProc *)pFrom->from_CIEXYZ;
    dest_to_CIEXYZ = (XcmsDDConversionProc *)pTo->to_CIEXYZ;
    dest_from_CIEXYZ = (XcmsDDConversionProc *)pTo->from_CIEXYZ;

    if (pTo->inverse_flag && pFrom->inverse_flag) {
	/*
	 * Find common function pointers
	 */
	for (to_CIEXYZ_stop = src_to_CIEXYZ; *to_CIEXYZ_stop; to_CIEXYZ_stop++){
	    for (tmp = dest_to_CIEXYZ; *tmp; tmp++) {
		if (*to_CIEXYZ_stop == *tmp) {
		    goto Continue;
		}
	    }
	}
Continue:

	/*
	 * Execute the functions
	 */
	while (src_to_CIEXYZ != to_CIEXYZ_stop) {
	    retval = (*src_to_CIEXYZ++)(ccc, pColors_in_out, nColors,
		    pCompressed);
	    if (retval == XcmsFailure) {
		return(XcmsFailure);
	    }
	    hasCompressed |= (retval == XcmsSuccessWithCompression);
	}

	/*
	 * Determine where to start on the from_CIEXYZ path.
	 */
	from_CIEXYZ_start = dest_from_CIEXYZ;
	tmp = src_from_CIEXYZ;
	while ((*from_CIEXYZ_start == *tmp) && (*from_CIEXYZ_start != NULL)) {
	    from_CIEXYZ_start++;
	    tmp++;
	}

    } else {
	/*
	 * The function in at least one of the Color Spaces are not
	 * complementary, i.e.,
	 *	for an i, 0 <= i < n elements
	 *	from_CIEXYZ[i] is not the inverse of to_CIEXYZ[i]
	 *
	 * Execute the functions all the way to CIEXYZ
	 */
	while (*src_to_CIEXYZ) {
	    retval = (*src_to_CIEXYZ++)(ccc, pColors_in_out, nColors,
		    pCompressed);
	    if (retval == XcmsFailure) {
		return(XcmsFailure);
	    }
	    hasCompressed |= (retval == XcmsSuccessWithCompression);
	}

	/*
	 * Determine where to start on the from_CIEXYZ path.
	 */
	from_CIEXYZ_start = dest_from_CIEXYZ;
    }

    while (*from_CIEXYZ_start) {
	retval = (*from_CIEXYZ_start++)(ccc, pColors_in_out, nColors,
		pCompressed);
	if (retval == XcmsFailure) {
	    return(XcmsFailure);
	}
	hasCompressed |= (retval == XcmsSuccessWithCompression);
    }

    return(hasCompressed ? XcmsSuccessWithCompression : XcmsSuccess);
}


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsConvertColors - Convert XcmsColor structures
 *
 *	SYNOPSIS
 */
Status
XcmsConvertColors(
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    unsigned int nColors,
    XcmsColorFormat targetFormat,
    Bool *pCompressed)
/*
 *	DESCRIPTION
 *		Convert XcmsColor structures to another format
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded without gamut compression,
 *		XcmsSuccessWithCompression if succeeded with gamut
 *			compression.
 *
 */
{
    XcmsColor clientWhitePt;
    XcmsColor Color1;
    XcmsColor *pColors_tmp;
    int callWhiteAdjustProc = 0;
    XcmsColorFormat format;
    Status retval;
    unsigned char contents_flag = 0x00;
    unsigned int iColors;

    if (ccc == NULL || pColors_in_out == NULL ||
		!(ValidDIColorSpaceID(targetFormat) ||
		ValidDDColorSpaceID(ccc, targetFormat))) {
	return(XcmsFailure);
    }

    /*
     * Check formats in color specification array
     */
    format = pColors_in_out->format;
    for (pColors_tmp = pColors_in_out, iColors = nColors; iColors; pColors_tmp++, iColors--) {
	if (!(ValidDIColorSpaceID(pColors_tmp->format) ||
		ValidDDColorSpaceID(ccc, pColors_tmp->format))) {
	    return(XcmsFailure);
	}
	if (XCMS_DI_ID(pColors_tmp->format)) {
	    contents_flag |= DI_FORMAT;
	} else {
	    contents_flag |= DD_FORMAT;
	}
	if (pColors_tmp->format != format) {
	    contents_flag |= MIX_FORMAT;
	}
    }

    /*
     * Check if we need the Client White Point.
     */
    if ((contents_flag & DI_FORMAT) || XCMS_DI_ID(targetFormat)) {
	/* To proceed, we need to get the Client White Point */
	memcpy((char *)&clientWhitePt, (char *)&ccc->clientWhitePt,
	       sizeof(XcmsColor));
	if (clientWhitePt.format == XcmsUndefinedFormat) {
	    /*
	     * Client White Point is undefined, therefore set to the Screen
	     *   White Point.
	     * Since Client White Point == Screen White Point, WhiteAdjustProc
	     *   is not called.
	     */
	    memcpy((char *)&clientWhitePt,
		   (char *)&ccc->pPerScrnInfo->screenWhitePt,
		   sizeof(XcmsColor));
	} else if ((ccc->whitePtAdjProc != NULL) && !_XcmsEqualWhitePts(ccc,
		&clientWhitePt, ScreenWhitePointOfCCC(ccc))) {
	    /*
	     * Client White Point != Screen White Point, and WhiteAdjustProc
	     *   is not NULL, therefore, will need to call it when
	     *   converting between DI and DD specifications.
	     */
	    callWhiteAdjustProc = 1;
	}
    }

    /*
     * Make copy of array of color specifications
     */
    if (nColors > 1) {
	pColors_tmp = Xmallocarray(nColors, sizeof(XcmsColor));
	if (pColors_tmp == NULL)
	    return(XcmsFailure);
    } else {
	pColors_tmp = &Color1;
    }
    memcpy((char *)pColors_tmp, (char *)pColors_in_out,
	   nColors * sizeof(XcmsColor));

    /*
     * zero out pCompressed
     */
    if (pCompressed) {
	bzero((char *)pCompressed, nColors * sizeof(Bool));
    }

    if (contents_flag == DD_FORMAT || contents_flag == DI_FORMAT) {
	/*
	 * ENTIRE ARRAY IS IN ONE FORMAT.
	 */
	if (XCMS_DI_ID(format) && XCMS_DI_ID(targetFormat)) {
	    /*
	     * DI-to-DI only conversion
	     */
	    retval = _XcmsDIConvertColors(ccc, pColors_tmp,
		    &clientWhitePt, nColors, targetFormat);
	} else if (XCMS_DD_ID(format) && XCMS_DD_ID(targetFormat)) {
	    /*
	     * DD-to-DD only conversion
	     *   Since DD->DD there will be no compressed thus we can
	     *   pass NULL instead of pCompressed.
	     */
	    retval = _XcmsDDConvertColors(ccc, pColors_tmp, nColors,
		    targetFormat, (Bool *)NULL);
	} else {
	    /*
	     * Otherwise we have:
	     *    1. Device-Independent to Device-Dependent Conversion
	     *		OR
	     *    2. Device-Dependent to Device-Independent Conversion
	     *
	     *  We need to go from oldFormat -> CIEXYZ -> targetFormat
	     *	adjusting for white points as necessary.
	     */

	    if (XCMS_DI_ID(format)) {
		/*
		 *    1. Device-Independent to Device-Dependent Conversion
		 */
		if (callWhiteAdjustProc) {
		    /*
		     * White Point Adjustment
		     *		Client White Point to Screen White Point
		     */
		    retval = (*ccc->whitePtAdjProc)(ccc, &clientWhitePt,
			    ScreenWhitePointOfCCC(ccc), targetFormat,
			    pColors_tmp, nColors, pCompressed);
		} else {
		    if (_XcmsDIConvertColors(ccc, pColors_tmp,
			    &clientWhitePt, nColors, XcmsCIEXYZFormat)
			    == XcmsFailure) {
			goto Failure;
		    }
		    retval = _XcmsDDConvertColors(ccc, pColors_tmp, nColors,
			    targetFormat, pCompressed);
		}
	    } else {
		/*
		 *    2. Device-Dependent to Device-Independent Conversion
		 */
		if (callWhiteAdjustProc) {
		    /*
		     * White Point Adjustment
		     *		Screen White Point to Client White Point
		     */
		    retval = (*ccc->whitePtAdjProc)(ccc,
			    ScreenWhitePointOfCCC(ccc), &clientWhitePt,
			    targetFormat, pColors_tmp, nColors, pCompressed);
		} else {
		    /*
		     * Since DD->CIEXYZ, no compression takes place therefore
		     * we can pass NULL instead of pCompressed.
		     */
		    if (_XcmsDDConvertColors(ccc, pColors_tmp, nColors,
			    XcmsCIEXYZFormat, (Bool *)NULL) == XcmsFailure) {
			goto Failure;
		    }
		    retval = _XcmsDIConvertColors(ccc, pColors_tmp,
			    &clientWhitePt, nColors, targetFormat);
		}
	    }
	}
    } else {
	/*
	 * ARRAY HAS MIXED FORMATS.
	 */
	if ((contents_flag == (DI_FORMAT | MIX_FORMAT)) &&
		XCMS_DI_ID(targetFormat)) {
	    /*
	     * Convert from DI to DI in batches of contiguous formats
	     *
	     * Because DI->DI, WhiteAdjustProc not called.
	     */
	    retval = ConvertMixedColors(ccc, pColors_tmp, &clientWhitePt,
		    nColors, targetFormat, (unsigned char)DI_FORMAT);
	} else if ((contents_flag == (DD_FORMAT | MIX_FORMAT)) &&
		XCMS_DD_ID(targetFormat)) {
	    /*
	     * Convert from DD to DD in batches of contiguous formats
	     *
	     * Because DD->DD, WhiteAdjustProc not called.
	     */
	    retval = ConvertMixedColors(ccc, pColors_tmp,
		    (XcmsColor *)NULL, nColors, targetFormat,
		    (unsigned char)DD_FORMAT);
	} else if (XCMS_DI_ID(targetFormat)) {
	    /*
	     * We need to convert from DI-to-DI and DD-to-DI, therefore
	     *   1. convert DD specifications to CIEXYZ, then
	     *   2. convert all in batches to the target DI format.
	     *
	     * Note that ConvertMixedColors will call WhiteAdjustProc
	     * as necessary.
	     */

	    /*
	     * Convert only DD specifications in batches of contiguous formats
	     * to CIEXYZ
	     *
	     * Since DD->CIEXYZ, ConvertMixedColors will apply WhiteAdjustProc
	     * if required.
	     */
	    retval = ConvertMixedColors(ccc, pColors_tmp, &clientWhitePt,
		    nColors, XcmsCIEXYZFormat, (unsigned char)DD_FORMAT);

	    /*
	     * Because at this point we may have a mix of DI formats
	     * (e.g., CIEXYZ, CIELuv) we must convert the specs to the
	     * target DI format in batches of contiguous source formats.
	     */
	    retval = ConvertMixedColors(ccc, pColors_tmp, &clientWhitePt,
		    nColors, targetFormat, (unsigned char)DI_FORMAT);
	} else {
	    /*
	     * We need to convert from DI-to-DD and DD-to-DD, therefore
	     *   1. convert DI specifications to CIEXYZ, then
	     *   2. convert all to the DD target format.
	     *
	     *   This allows white point adjustment and gamut compression
	     *	 to be applied to all the color specifications in one
	     *   swoop if those functions do in fact modify the entire
	     *   group of color specifications.
	     */

	    /*
	     * Convert in batches to CIEXYZ
	     *
	     * If DD->CIEXYZ, ConvertMixedColors will apply WhiteAdjustProc
	     * if required.
	     */
	    if ((retval = ConvertMixedColors(ccc, pColors_tmp, &clientWhitePt,
		    nColors, XcmsCIEXYZFormat,
		    (unsigned char)(DI_FORMAT | DD_FORMAT))) == XcmsFailure) {
		goto Failure;
	    }

	    /*
	     * Convert all specifications (now in CIEXYZ format) to
	     * the target DD format.
	     * Since CIEXYZ->DD, compression MAY take place therefore
	     * we must pass pCompressed.
	     * Note that WhiteAdjustProc must be used if necessary.
	     */
	    if (callWhiteAdjustProc) {
		/*
		 * White Point Adjustment
		 *	Client White Point to Screen White Point
		 */
		retval = (*ccc->whitePtAdjProc)(ccc,
			&clientWhitePt, ScreenWhitePointOfCCC(ccc),
			targetFormat, pColors_tmp, nColors, pCompressed);
	    } else {
		retval = _XcmsDDConvertColors(ccc, pColors_tmp, nColors,
			targetFormat, pCompressed);
	    }
	}
    }

    if (retval != XcmsFailure) {
	memcpy((char *)pColors_in_out, (char *)pColors_tmp,
	       nColors * sizeof(XcmsColor));
    }
    if (nColors > 1) {
	Xfree(pColors_tmp);
    }
    return(retval);

Failure:
    if (nColors > 1) {
	Xfree(pColors_tmp);
    }
    return(XcmsFailure);
}


/*
 *	NAME
 *		XcmsRegFormatOfPrefix
 *
 *	SYNOPSIS
 */
XcmsColorFormat
_XcmsRegFormatOfPrefix(
    _Xconst char *prefix)
/*
 *	DESCRIPTION
 *		Returns a color space ID associated with the specified
 *		X Consortium registered color space prefix.
 *
 *	RETURNS
 *		The color space ID if found;
 *		otherwise NULL.
 */
{
    XcmsRegColorSpaceEntry *pEntry = _XcmsRegColorSpaces;

    while (pEntry->prefix != NULL) {
	if (strcmp(prefix, pEntry->prefix) == 0) {
	    return(pEntry->id);
	}
	pEntry++;
    }
    return(XcmsUndefinedFormat);
}
