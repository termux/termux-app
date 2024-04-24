
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
 *		XcmsInt.c - Xcms API utility routines
 *
 *	DESCRIPTION
 *		Xcms Application Program Interface (API) utility
 *		routines for hanging information directly onto
 *		the Display structure.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"
#include "reallocarray.h"

#ifndef XCMSCOMPPROC
#  define XCMSCOMPPROC	XcmsTekHVCClipC
#endif

/* forward/static */
static void _XcmsFreeDefaultCCCs(Display *dpy);


/************************************************************************
 *									*
 *			   API PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		_XcmsCopyPointerArray
 *
 *	SYNOPSIS
 */
XPointer *
_XcmsCopyPointerArray(
    XPointer *pap)
/*
 *	DESCRIPTION
 *		Copies an array of NULL terminated pointers.
 *
 *	RETURNS
 *		Returns NULL if failed; otherwise the address to
 *		the copy.
 *
 */
{
    XPointer *newArray;
    char **tmp;
    int n;

    for (tmp = pap, n = 0; *tmp != NULL; tmp++, n++);
    n++; /* add 1 to include the NULL pointer */

    if ((newArray = Xmallocarray(n, sizeof(XPointer)))) {
	memcpy((char *)newArray, (char *)pap,
	       (unsigned)(n * sizeof(XPointer)));
    }
    return((XPointer *)newArray);
}

/*
 *	NAME
 *		_XcmsFreePointerArray
 *
 *	SYNOPSIS
 */
void
_XcmsFreePointerArray(
    XPointer *pap)
/*
 *	DESCRIPTION
 *		Frees an array of NULL terminated pointers.
 *
 *	RETURNS
 *		void
 *
 */
{
    Xfree(pap);
}

/*
 *	NAME
 *		_XcmsPushPointerArray
 *
 *	SYNOPSIS
 */
XPointer *
_XcmsPushPointerArray(
    XPointer *pap,
    XPointer p,
    XPointer *papNoFree)
/*
 *	DESCRIPTION
 *		Places the specified pointer at the head of an array of NULL
 *		terminated pointers.
 *
 *	RETURNS
 *		Returns NULL if failed; otherwise the address to
 *		the head of the array.
 *
 */
{
    XPointer *newArray;
    char **tmp;
    int n;

    for (tmp = pap, n = 0; *tmp != NULL; tmp++, n++);

    /* add 2: 1 for the new pointer and another for the NULL pointer */
    n += 2;

    if ((newArray = Xmallocarray(n, sizeof(XPointer)))) {
	memcpy((char *)(newArray+1),(char *)pap,
	       (unsigned)((n-1) * sizeof(XPointer)));
	*newArray = p;
    }
    if (pap != papNoFree) {
        _XcmsFreePointerArray(pap);
    }
    return((XPointer *)newArray);
}

/*
 *	NAME
 *		_XcmsInitDefaultCCCs
 *
 *	SYNOPSIS
 */
int
_XcmsInitDefaultCCCs(
    Display *dpy)
/*
 *	DESCRIPTION
 *		Initializes the Xcms per Display Info structure
 *		(XcmsPerDpyInfo).
 *
 *	RETURNS
 *		Returns 0 if failed; otherwise non-zero.
 *
 */
{
    int nScrn = ScreenCount(dpy);
    int i;
    XcmsCCC ccc;

    if (nScrn <= 0) {
	return(0);
    }

    /*
     * Create an array of XcmsCCC structures, one for each screen.
     * They serve as the screen's default CCC.
     */
    if (!(ccc = Xcalloc((unsigned)nScrn, sizeof(XcmsCCCRec)))) {
	return(0);
    }
    dpy->cms.defaultCCCs = (XPointer)ccc;
    dpy->free_funcs->defaultCCCs = _XcmsFreeDefaultCCCs;

    for (i = 0; i < nScrn; i++, ccc++) {
	ccc->dpy = dpy;
	ccc->screenNumber = i;
	ccc->visual = DefaultVisual(dpy, i);
	/*
	 * Used calloc to allocate memory so:
	 *	ccc->clientWhitePt->format == XcmsUndefinedFormat
	 *	ccc->gamutCompProc == NULL
	 *	ccc->whitePtAdjProc == NULL
	 *	ccc->pPerScrnInfo = NULL
	 *
	 * Don't need to create XcmsPerScrnInfo and its functionSet and
	 * pScreenData components until the default CCC is accessed.
	 * Note that the XcmsDefaultCCC routine calls _XcmsInitScrnInto
	 * to do this.
	 */
	ccc->gamutCompProc = XCMSCOMPPROC;
    }

    return(1);
}


/*
 *	NAME
 *		_XcmsFreeDefaultCCCs - Free Default CCCs and its PerScrnInfo
 *
 *	SYNOPSIS
 */
static void
_XcmsFreeDefaultCCCs(
    Display *dpy)
/*
 *	DESCRIPTION
 *		This routine frees the default XcmsCCC's associated with
 *		each screen and its associated substructures as necessary.
 *
 *	RETURNS
 *		void
 *
 *
 */
{
    int nScrn = ScreenCount(dpy);
    XcmsCCC ccc;
    int i;

    /*
     * Free Screen data in each DefaultCCC
     *		Do not use XcmsFreeCCC here because it will not free
     *		DefaultCCC's.
     */
    ccc = (XcmsCCC)dpy->cms.defaultCCCs;
    for (i = nScrn; i--; ccc++) {
	/*
	 * Check if XcmsPerScrnInfo exists.
	 *
	 * This is the only place where XcmsPerScrnInfo structures
	 * are freed since there is only one allocated per Screen.
	 * It just so happens that we place its reference in the
	 * default CCC.
	 */
	if (ccc->pPerScrnInfo) {
	    /* Check if SCCData exists */
	    if (ccc->pPerScrnInfo->state != XcmsInitNone
		    && ccc->pPerScrnInfo->screenData) {
		(*((XcmsFunctionSet *)ccc->pPerScrnInfo->functionSet)->screenFreeProc)
			(ccc->pPerScrnInfo->screenData);
	    }
	    Xfree(ccc->pPerScrnInfo);
	}
    }

    /*
     * Free the array of XcmsCCC structures
     */
    Xfree(dpy->cms.defaultCCCs);
    dpy->cms.defaultCCCs = (XPointer)NULL;
}



/*
 *	NAME
 *		_XcmsInitScrnInfo
 *
 *	SYNOPSIS
 */
int
_XcmsInitScrnInfo(
    register Display *dpy,
    int screenNumber)
/*
 *	DESCRIPTION
 *		Given a display and screen number, this routine attempts
 *		to initialize the Xcms per Screen Info structure
 *		(XcmsPerScrnInfo).
 *
 *	RETURNS
 *		Returns zero if initialization failed; non-zero otherwise.
 */
{
    XcmsFunctionSet **papSCCFuncSet = _XcmsSCCFuncSets;
    XcmsCCC defaultccc;

    /*
     * Check if the XcmsCCC's for each screen has been created.
     * Really don't need to be created until some routine uses the Xcms
     * API routines.
     */
    if ((XcmsCCC)dpy->cms.defaultCCCs == NULL) {
	if (!_XcmsInitDefaultCCCs(dpy)) {
	    return(0);
	}
    }

    defaultccc = (XcmsCCC)dpy->cms.defaultCCCs + screenNumber;

    /*
     * For each SCCFuncSet, try its pInitScrnFunc.
     *	If the function succeeds, then we got it!
     */

    if (!defaultccc->pPerScrnInfo) {
	/*
	 * This is one of two places where XcmsPerScrnInfo structures
	 * are allocated.  There is one allocated per Screen that is
	 * shared among visuals that do not have specific intensity
	 * tables.  Other XcmsPerScrnInfo structures are created
	 * for the latter (see XcmsCreateCCC).  The ones created
	 * here are referenced by the default CCC.
	 */
	if (!(defaultccc->pPerScrnInfo =
		Xcalloc(1, sizeof(XcmsPerScrnInfo)))) {
	    return(0);
	}
	defaultccc->pPerScrnInfo->state = XcmsInitNone;
    }

    while (*papSCCFuncSet != NULL) {
	if ((*(*papSCCFuncSet)->screenInitProc)(dpy, screenNumber,
		defaultccc->pPerScrnInfo)) {
	    defaultccc->pPerScrnInfo->state = XcmsInitSuccess;
	    return(1);
	}
	papSCCFuncSet++;
    }

    /*
     * Use Default SCCData
     */
    return(_XcmsLRGB_InitScrnDefault(dpy, screenNumber, defaultccc->pPerScrnInfo));
}


/*
 *	NAME
 *		_XcmsFreeIntensityMaps
 *
 *	SYNOPSIS
 */
void
_XcmsFreeIntensityMaps(
    Display *dpy)
/*
 *	DESCRIPTION
 *		Frees all XcmsIntensityMap structures in the linked list
 *		and sets dpy->cms.perVisualIntensityMaps to NULL.
 *
 *	RETURNS
 *		void
 *
 */
{
    XcmsIntensityMap *pNext, *pFree;

    pNext = (XcmsIntensityMap *)dpy->cms.perVisualIntensityMaps;
    while (pNext != NULL) {
	pFree = pNext;
	pNext = pNext->pNext;
	(*pFree->pFreeScreenData)(pFree->screenData);
	/* Now free the XcmsIntensityMap structure */
	Xfree(pFree);
    }
    dpy->cms.perVisualIntensityMaps = (XPointer)NULL;
}


/*
 *	NAME
 *		_XcmsGetIntensityMap
 *
 *	SYNOPSIS
 */
XcmsIntensityMap *
_XcmsGetIntensityMap(
    Display *dpy,
    Visual *visual)
/*
 *	DESCRIPTION
 *		Attempts to return a per-Visual intensity map.
 *
 *	RETURNS
 *		Pointer to the XcmsIntensityMap structure if found;
 *		otherwise NULL
 *
 */
{
    VisualID targetID = visual->visualid;
    XcmsIntensityMap *pNext;

    pNext = (XcmsIntensityMap *)dpy->cms.perVisualIntensityMaps;
    while (pNext != NULL) {
	if (targetID == pNext->visualID) {
	    return(pNext);
	}
	pNext = pNext->pNext;
    }
    return((XcmsIntensityMap *)NULL);
}
