
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
 *		XcmsAddSF.c
 *
 *	DESCRIPTION
 *		Source for XcmsAddFunctionSet
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
#define NextUnregDdCsID(lastid) \
	    (XCMS_UNREG_ID(lastid) ? ++lastid : XCMS_FIRST_UNREG_DD_ID)
#define MIN(x,y) ((x) > (y) ? (y) : (x))


/*
 *	NAME
 *		XcmsAddFunctionSet - Add an Screen Color Characterization
 *					Function Set
 *
 *	SYNOPSIS
 */
Status
XcmsAddFunctionSet(XcmsFunctionSet *pNewFS)
/*
 *	DESCRIPTION
 *		Additional Screen Color Characterization Function Sets are
 *		managed on a global basis.  This means that with exception
 *		of the provided DD color spaces:
 *			    RGB and RGBi
 *		DD color spaces may have different XcmsColorFormat IDs between
 *		clients.  So, you must be careful when using XcmsColorFormat
 *		across clients!  Use the routines XcmsFormatOfPrefix()
 *		and XcmsPrefixOfFormat() appropriately.
 *
 *	RETURNS
 *		XcmsSuccess if succeeded, otherwise XcmsFailure
 *
 *	CAVEATS
 *		Additional Screen Color Characterization Function Sets
 *		should be added prior to any use of the routine
 *		XcmsCreateCCC().  If not, XcmsCCC structures created
 *		prior to the call of this routines will not have had
 *		a chance to initialize using the added Screen Color
 *		Characterization Function Set.
 */
{
    XcmsFunctionSet **papSCCFuncSets = _XcmsSCCFuncSets;
    XcmsColorSpace **papNewCSs;
    XcmsColorSpace *pNewCS, **paptmpCS;
    XcmsColorFormat lastID = 0;


    if (papSCCFuncSets != NULL) {
	if ((papNewCSs = pNewFS->DDColorSpaces) == NULL) {
	    /*
	     * Error, new Screen Color Characterization Function Set
	     *	missing color spaces
	     */
	    return(XcmsFailure);
	}
	while ((pNewCS = *papNewCSs++) != NULL) {
	    if ((pNewCS->id = _XcmsRegFormatOfPrefix(pNewCS->prefix)) != 0) {
		if (XCMS_DI_ID(pNewCS->id)) {
		    /* This is a Device-Independent Color Space */
		    return(XcmsFailure);
		}
		/*
		 * REGISTERED DD Color Space
		 *    therefore use the registered ID.
		 */
	    } else {
		/*
		 * UNREGISTERED DD Color Space
		 *    then see if the color space is already in
		 *    _XcmsDDColorSpaces.
		 *	    a. If same prefix, then use the same ID.
		 *	    b. Otherwise, use a new ID.
		 */
		for (paptmpCS = _XcmsDDColorSpaces; *paptmpCS != NULL;
			paptmpCS++){
		    lastID = MIN(lastID, (*paptmpCS)->id);
		    if (strcmp(pNewCS->prefix, (*paptmpCS)->prefix) == 0) {
			pNewCS->id = (*paptmpCS)->id;
			break;
		    }
		}
		if (pNewCS->id == 0) {
		    /* still haven't found one */
		    pNewCS->id = NextUnregDdCsID(lastID);
		    if ((paptmpCS = (XcmsColorSpace **)_XcmsPushPointerArray(
		   	    (XPointer *) _XcmsDDColorSpaces,
			    (XPointer) pNewCS,
			    (XPointer *) _XcmsDDColorSpacesInit)) == NULL) {
			return(XcmsFailure);
		    }
		    _XcmsDDColorSpaces = paptmpCS;
		}
	    }
	}
    }
    if ((papSCCFuncSets = (XcmsFunctionSet **)
	    _XcmsPushPointerArray((XPointer *) _XcmsSCCFuncSets,
	    (XPointer) pNewFS,
	    (XPointer *)_XcmsSCCFuncSetsInit)) == NULL) {
	return(XcmsFailure);
    }
    _XcmsSCCFuncSets = papSCCFuncSets;

    return(XcmsSuccess);
}
