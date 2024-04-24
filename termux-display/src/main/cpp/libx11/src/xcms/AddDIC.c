
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
 *		XcmsAddDIC.c
 *
 *	DESCRIPTION
 *		Source for XcmsAddColorSpace
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
 *      DEFINES
 */
#define NextUnregDiCsID(lastid) \
	    (XCMS_UNREG_ID(lastid) ? ++lastid : XCMS_FIRST_UNREG_DI_ID)
#define MAX(x,y) ((x) < (y) ? (y) : (x))


/*
 *	NAME
 *		XcmsAddColorSpace - Add a Device-Independent Color Space
 *
 *	SYNOPSIS
 */
Status
XcmsAddColorSpace(XcmsColorSpace *pCS)
/*
 *	DESCRIPTION
 *		DI Color Spaces are managed on a global basis.
 *		This means that with exception of the provided DI color spaces:
 *			CIEXYZ, CIExyY, CIELab, CIEuvY, CIELuv, and TekHVC
 *		DI color spaces may have different XcmsColorFormat IDs between
 *		clients.  So, you must be careful when using XcmsColor
 *		structures between clients!  Use the routines XcmsFormatOfPrefix()
 *		and XcmsPrefixOfFormat() appropriately.
 *
 *	RETURNS
 *		XcmsSuccess if succeeded, otherwise XcmsFailure
 */
{
    XcmsColorSpace **papColorSpaces;
    XcmsColorSpace *ptmpCS;
    XcmsColorFormat lastID = 0;

    if ((pCS->id = _XcmsRegFormatOfPrefix(pCS->prefix)) != 0) {
	if (XCMS_DD_ID(pCS->id)) {
	    /* This is a Device-Dependent Color Space */
	    return(XcmsFailure);
	}
	/*
	 * REGISTERED DI Color Space
	 *    then see if the color space has already been added to the
	 *    system:
	 *	    a. If the same ID/prefix and same XcmsColorSpec is found,
	 *		then its a duplicate, so return success.
	 *	    b. If same ID/prefix but different XcmsColorSpec is
	 *		found, then add the color space to the front of the
	 *		list using the same ID.  This allows one to override
	 *		an existing DI Color Space.
	 *	    c. Otherwise none found so just add the color space.
	 */
	if ((papColorSpaces = _XcmsDIColorSpaces) != NULL) {
	    while ((ptmpCS = *papColorSpaces++) != NULL) {
		if (pCS->id == ptmpCS->id) {
		    if (pCS == ptmpCS) {
			/* a. duplicate*/
			return(XcmsSuccess);
		    }
		    /* b. same ID/prefix but different XcmsColorSpace */
		    break;
		}
	    }
	}
	/* c. None found */
    } else {
	/*
	 * UNREGISTERED DI Color Space
	 *    then see if the color space has already been added to the
	 *    system:
	 *	    a. If same prefix and XcmsColorSpec, then
	 *		its a duplicate ... return success.
	 *	    b. If same prefix but different XcmsColorSpec, then
	 *		add the color space to the front of the list using
	 *		the same ID.  This allows one to override an existing
	 *		DI Color Space.
	 *	    c. Otherwise none found so, add the color space using the
	 *		next unregistered ID for the connection.
	 */
	if ((papColorSpaces = _XcmsDIColorSpaces) != NULL) {
	    while ((ptmpCS = *papColorSpaces++) != NULL) {
		lastID = MAX(lastID, ptmpCS->id);
		if (strcmp(pCS->prefix, ptmpCS->prefix) == 0) {
		    if (pCS == ptmpCS) {
			/* a. duplicate */
			return(XcmsSuccess);
		    }
		    /* b. same prefix but different XcmsColorSpec */
		    pCS->id = ptmpCS->id;
		    goto AddColorSpace;
		}
	    }
	}
	/* c. None found */
	pCS->id = NextUnregDiCsID(lastID);
    }


AddColorSpace:
    if ((papColorSpaces = (XcmsColorSpace **)
	    _XcmsPushPointerArray((XPointer *)_XcmsDIColorSpaces,
	    (XPointer)pCS,
	    (XPointer *)_XcmsDIColorSpacesInit)) == NULL) {
	return(XcmsFailure);
    }
    _XcmsDIColorSpaces = papColorSpaces;
    return(XcmsSuccess);
}
