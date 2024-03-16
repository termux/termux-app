
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
 *		XcmsGlobls.c
 *
 *	DESCRIPTION
 *		Source file containing Xcms globals
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"

/*
 *      GLOBALS
 *              Variables declared in this package that are allowed
 *		to be used globally.
 */

    /*
     * Initial array of Device Independent Color Spaces
     */
XcmsColorSpace *_XcmsDIColorSpacesInit[] = {
    &XcmsCIEXYZColorSpace,
    &XcmsCIEuvYColorSpace,
    &XcmsCIExyYColorSpace,
    &XcmsCIELabColorSpace,
    &XcmsCIELuvColorSpace,
    &XcmsTekHVCColorSpace,
    &XcmsUNDEFINEDColorSpace,
    NULL
};
    /*
     * Pointer to the array of pointers to XcmsColorSpace structures for
     * Device-Independent Color Spaces that are currently accessible by
     * the color management system.  End of list is indicated by a NULL pointer.
     */
XcmsColorSpace **_XcmsDIColorSpaces = _XcmsDIColorSpacesInit;

    /*
     * Initial array of Device Dependent Color Spaces
     */
XcmsColorSpace *_XcmsDDColorSpacesInit[] = {
    &XcmsRGBColorSpace,
    &XcmsRGBiColorSpace,
    NULL
};
    /*
     * Pointer to the array of pointers to XcmsColorSpace structures for
     * Device-Dependent Color Spaces that are currently accessible by
     * the color management system.  End of list is indicated by a NULL pointer.
     */
XcmsColorSpace **_XcmsDDColorSpaces = &_XcmsDDColorSpacesInit[0];

    /*
     * Initial array of Screen Color Characterization Function Sets
     */
XcmsFunctionSet	*_XcmsSCCFuncSetsInit[] = {
	&XcmsLinearRGBFunctionSet,
#ifdef GRAY
	&XcmsGrayFunctionSet,
#endif /* GRAY */
	NULL};
    /*
     * Pointer to the array of pointers to XcmsSCCFuncSet structures
     * (Screen Color Characterization Function Sets) that are currently
     * accessible by the color management system.  End of list is
     * indicated by a NULL pointer.
     */
XcmsFunctionSet **_XcmsSCCFuncSets = _XcmsSCCFuncSetsInit;

    /*
     * X Consortium Registered Device-Independent Color Spaces
     *	Note that prefix must be in lowercase.
     */
const char	_XcmsCIEXYZ_prefix[] = "ciexyz";
const char	_XcmsCIEuvY_prefix[] = "cieuvy";
const char	_XcmsCIExyY_prefix[] = "ciexyy";
const char	_XcmsCIELab_prefix[] = "cielab";
const char 	_XcmsCIELuv_prefix[] = "cieluv";
const char	_XcmsTekHVC_prefix[] = "tekhvc";
    /*
     * Registered Device-Dependent Color Spaces
     */
const char	_XcmsRGBi_prefix[] = "rgbi";
const char	_XcmsRGB_prefix[] = "rgb";

XcmsRegColorSpaceEntry _XcmsRegColorSpaces[] = {
    { _XcmsCIEXYZ_prefix, XcmsCIEXYZFormat },
    { _XcmsCIEuvY_prefix, XcmsCIEuvYFormat },
    { _XcmsCIExyY_prefix, XcmsCIExyYFormat },
    { _XcmsCIELab_prefix, XcmsCIELabFormat },
    { _XcmsCIELuv_prefix, XcmsCIELuvFormat },
    { _XcmsTekHVC_prefix, XcmsTekHVCFormat },
    { _XcmsRGB_prefix, XcmsRGBFormat },
    { _XcmsRGBi_prefix,	XcmsRGBiFormat },
    { NULL, 0 }
};
