
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
 *		XcmsLRGB.c
 *
 *	DESCRIPTION
 *		This file contains the conversion routines:
 *		    1. CIE XYZ to RGB intensity
 *		    2. RGB intensity to device RGB
 *		    3. device RGB to RGB intensity
 *		    4. RGB intensity to CIE XYZ
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"

/*
 *      LOCAL DEFINES
 *		#define declarations local to this package.
 */
#define EPS	0.001
#ifndef MIN
#define MIN(x,y) ((x) > (y) ? (y) : (x))
#endif /* MIN */
#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif /* MAX */
#ifndef MIN3
#define MIN3(x,y,z) ((x) > (MIN((y), (z))) ? (MIN((y), (z))) : (x))
#endif /* MIN3 */
#ifndef MAX3
#define MAX3(x,y,z) ((x) > (MAX((y), (z))) ? (x) : (MAX((y), (z))))
#endif /* MAX3 */

/*
 *      LOCAL TYPEDEFS
 *              typedefs local to this package (for use with local vars).
 *
 */

/*
 *      FORWARD DECLARATIONS
 */
static void LINEAR_RGB_FreeSCCData(XPointer pScreenDataTemp);
static int LINEAR_RGB_InitSCCData(Display *dpy,
    int screenNumber, XcmsPerScrnInfo *pPerScrnInfo);
static int XcmsLRGB_RGB_ParseString(register char *spec, XcmsColor *pColor);
static int XcmsLRGB_RGBi_ParseString(register char *spec, XcmsColor *pColor);
static Status
_XcmsGetTableType0(
    IntensityTbl *pTbl,
    int	  format,
    char **pChar,
    unsigned long *pCount);
static Status
_XcmsGetTableType1(
    IntensityTbl *pTbl,
    int	  format,
    char **pChar,
    unsigned long *pCount);

/*
 *      LOCALS VARIABLES
 *		Variables local to this package.
 *		    Usage example:
 *		        static int	ExampleLocalVar;
 */

static unsigned short const MASK[17] = {
    0x0000,	/*  0 bitsPerRGB */
    0x8000,	/*  1 bitsPerRGB */
    0xc000,	/*  2 bitsPerRGB */
    0xe000,	/*  3 bitsPerRGB */
    0xf000,	/*  4 bitsPerRGB */
    0xf800,	/*  5 bitsPerRGB */
    0xfc00,	/*  6 bitsPerRGB */
    0xfe00,	/*  7 bitsPerRGB */
    0xff00,	/*  8 bitsPerRGB */
    0xff80,	/*  9 bitsPerRGB */
    0xffc0,	/* 10 bitsPerRGB */
    0xffe0,	/* 11 bitsPerRGB */
    0xfff0,	/* 12 bitsPerRGB */
    0xfff8,	/* 13 bitsPerRGB */
    0xfffc,	/* 14 bitsPerRGB */
    0xfffe,	/* 15 bitsPerRGB */
    0xffff	/* 16 bitsPerRGB */
};


    /*
     * A NULL terminated array of function pointers that when applied
     * in series will convert an XcmsColor structure from XcmsRGBFormat
     * to XcmsCIEXYZFormat.
     */
static XcmsConversionProc Fl_RGB_to_CIEXYZ[] = {
    (XcmsConversionProc)XcmsRGBToRGBi,
    (XcmsConversionProc)XcmsRGBiToCIEXYZ,
    NULL
};

    /*
     * A NULL terminated array of function pointers that when applied
     * in series will convert an XcmsColor structure from XcmsCIEXYZFormat
     * to XcmsRGBFormat.
     */
static XcmsConversionProc Fl_CIEXYZ_to_RGB[] = {
    (XcmsConversionProc)XcmsCIEXYZToRGBi,
    (XcmsConversionProc)XcmsRGBiToRGB,
    NULL
};

    /*
     * A NULL terminated array of function pointers that when applied
     * in series will convert an XcmsColor structure from XcmsRGBiFormat
     * to XcmsCIEXYZFormat.
     */
static XcmsConversionProc Fl_RGBi_to_CIEXYZ[] = {
    (XcmsConversionProc)XcmsRGBiToCIEXYZ,
    NULL
};

    /*
     * A NULL terminated array of function pointers that when applied
     * in series will convert an XcmsColor structure from XcmsCIEXYZFormat
     * to XcmsRGBiFormat.
     */
static XcmsConversionProc Fl_CIEXYZ_to_RGBi[] = {
    (XcmsConversionProc)XcmsCIEXYZToRGBi,
    NULL
};

    /*
     * RGBi Color Spaces
     */
XcmsColorSpace	XcmsRGBiColorSpace =
    {
	_XcmsRGBi_prefix,	/* prefix */
	XcmsRGBiFormat,		/* id */
	XcmsLRGB_RGBi_ParseString,	/* parseString */
	Fl_RGBi_to_CIEXYZ,	/* to_CIEXYZ */
	Fl_CIEXYZ_to_RGBi,	/* from_CIEXYZ */
	1
    };

    /*
     * RGB Color Spaces
     */
XcmsColorSpace	XcmsRGBColorSpace =
    {
	_XcmsRGB_prefix,		/* prefix */
	XcmsRGBFormat,		/* id */
	XcmsLRGB_RGB_ParseString,	/* parseString */
	Fl_RGB_to_CIEXYZ,	/* to_CIEXYZ */
	Fl_CIEXYZ_to_RGB,	/* from_CIEXYZ */
	1
    };

    /*
     * Device-Independent Color Spaces known to the
     * LINEAR_RGB Screen Color Characteristics Function Set.
     */
static XcmsColorSpace	*DDColorSpaces[] = {
    &XcmsRGBColorSpace,
    &XcmsRGBiColorSpace,
    NULL
};


/*
 *      GLOBALS
 *              Variables declared in this package that are allowed
 *		to be used globally.
 */

    /*
     * LINEAR_RGB Screen Color Characteristics Function Set.
     */
XcmsFunctionSet	XcmsLinearRGBFunctionSet =
    {
	&DDColorSpaces[0],	/* pDDColorSpaces */
	LINEAR_RGB_InitSCCData,	/* pInitScrnFunc */
	LINEAR_RGB_FreeSCCData	/* pFreeSCCData */
    };

/*
 *	DESCRIPTION
 *		Contents of Default SCCData should be replaced if other
 *		data should be used as default.
 *
 *
 */

/*
 * NAME		Tektronix 19" (Sony) CRT
 * PART_NUMBER		119-2451-00
 * MODEL		Tek4300, Tek4800
 */

static IntensityRec const Default_RGB_RedTuples[] = {
    /* {unsigned short value, XcmsFloat intensity} */
            { 0x0000,    0.000000 },
            { 0x0909,    0.000000 },
            { 0x0a0a,    0.000936 },
            { 0x0f0f,    0.001481 },
            { 0x1414,    0.002329 },
            { 0x1919,    0.003529 },
            { 0x1e1e,    0.005127 },
            { 0x2323,    0.007169 },
            { 0x2828,    0.009699 },
            { 0x2d2d,    0.012759 },
            { 0x3232,    0.016392 },
            { 0x3737,    0.020637 },
            { 0x3c3c,    0.025533 },
            { 0x4141,    0.031119 },
            { 0x4646,    0.037431 },
            { 0x4b4b,    0.044504 },
            { 0x5050,    0.052373 },
            { 0x5555,    0.061069 },
            { 0x5a5a,    0.070624 },
            { 0x5f5f,    0.081070 },
            { 0x6464,    0.092433 },
            { 0x6969,    0.104744 },
            { 0x6e6e,    0.118026 },
            { 0x7373,    0.132307 },
            { 0x7878,    0.147610 },
            { 0x7d7d,    0.163958 },
            { 0x8282,    0.181371 },
            { 0x8787,    0.199871 },
            { 0x8c8c,    0.219475 },
            { 0x9191,    0.240202 },
            { 0x9696,    0.262069 },
            { 0x9b9b,    0.285089 },
            { 0xa0a0,    0.309278 },
            { 0xa5a5,    0.334647 },
            { 0xaaaa,    0.361208 },
            { 0xafaf,    0.388971 },
            { 0xb4b4,    0.417945 },
            { 0xb9b9,    0.448138 },
            { 0xbebe,    0.479555 },
            { 0xc3c3,    0.512202 },
            { 0xc8c8,    0.546082 },
            { 0xcdcd,    0.581199 },
            { 0xd2d2,    0.617552 },
            { 0xd7d7,    0.655144 },
            { 0xdcdc,    0.693971 },
            { 0xe1e1,    0.734031 },
            { 0xe6e6,    0.775322 },
            { 0xebeb,    0.817837 },
            { 0xf0f0,    0.861571 },
            { 0xf5f5,    0.906515 },
            { 0xfafa,    0.952662 },
            { 0xffff,    1.000000 }
};

static IntensityRec const Default_RGB_GreenTuples[] = {
    /* {unsigned short value, XcmsFloat intensity} */
            { 0x0000,    0.000000 },
            { 0x1313,    0.000000 },
            { 0x1414,    0.000832 },
            { 0x1919,    0.001998 },
            { 0x1e1e,    0.003612 },
            { 0x2323,    0.005736 },
            { 0x2828,    0.008428 },
            { 0x2d2d,    0.011745 },
            { 0x3232,    0.015740 },
            { 0x3737,    0.020463 },
            { 0x3c3c,    0.025960 },
            { 0x4141,    0.032275 },
            { 0x4646,    0.039449 },
            { 0x4b4b,    0.047519 },
            { 0x5050,    0.056520 },
            { 0x5555,    0.066484 },
            { 0x5a5a,    0.077439 },
            { 0x5f5f,    0.089409 },
            { 0x6464,    0.102418 },
            { 0x6969,    0.116485 },
            { 0x6e6e,    0.131625 },
            { 0x7373,    0.147853 },
            { 0x7878,    0.165176 },
            { 0x7d7d,    0.183604 },
            { 0x8282,    0.203140 },
            { 0x8787,    0.223783 },
            { 0x8c8c,    0.245533 },
            { 0x9191,    0.268384 },
            { 0x9696,    0.292327 },
            { 0x9b9b,    0.317351 },
            { 0xa0a0,    0.343441 },
            { 0xa5a5,    0.370580 },
            { 0xaaaa,    0.398747 },
            { 0xafaf,    0.427919 },
            { 0xb4b4,    0.458068 },
            { 0xb9b9,    0.489165 },
            { 0xbebe,    0.521176 },
            { 0xc3c3,    0.554067 },
            { 0xc8c8,    0.587797 },
            { 0xcdcd,    0.622324 },
            { 0xd2d2,    0.657604 },
            { 0xd7d7,    0.693588 },
            { 0xdcdc,    0.730225 },
            { 0xe1e1,    0.767459 },
            { 0xe6e6,    0.805235 },
            { 0xebeb,    0.843491 },
            { 0xf0f0,    0.882164 },
            { 0xf5f5,    0.921187 },
            { 0xfafa,    0.960490 },
            { 0xffff,    1.000000 }
};

static IntensityRec const Default_RGB_BlueTuples[] = {
    /* {unsigned short value, XcmsFloat intensity} */
            { 0x0000,    0.000000 },
            { 0x0e0e,    0.000000 },
            { 0x0f0f,    0.001341 },
            { 0x1414,    0.002080 },
            { 0x1919,    0.003188 },
            { 0x1e1e,    0.004729 },
            { 0x2323,    0.006766 },
            { 0x2828,    0.009357 },
            { 0x2d2d,    0.012559 },
            { 0x3232,    0.016424 },
            { 0x3737,    0.021004 },
            { 0x3c3c,    0.026344 },
            { 0x4141,    0.032489 },
            { 0x4646,    0.039481 },
            { 0x4b4b,    0.047357 },
            { 0x5050,    0.056154 },
            { 0x5555,    0.065903 },
            { 0x5a5a,    0.076634 },
            { 0x5f5f,    0.088373 },
            { 0x6464,    0.101145 },
            { 0x6969,    0.114968 },
            { 0x6e6e,    0.129862 },
            { 0x7373,    0.145841 },
            { 0x7878,    0.162915 },
            { 0x7d7d,    0.181095 },
            { 0x8282,    0.200386 },
            { 0x8787,    0.220791 },
            { 0x8c8c,    0.242309 },
            { 0x9191,    0.264937 },
            { 0x9696,    0.288670 },
            { 0x9b9b,    0.313499 },
            { 0xa0a0,    0.339410 },
            { 0xa5a5,    0.366390 },
            { 0xaaaa,    0.394421 },
            { 0xafaf,    0.423481 },
            { 0xb4b4,    0.453547 },
            { 0xb9b9,    0.484592 },
            { 0xbebe,    0.516587 },
            { 0xc3c3,    0.549498 },
            { 0xc8c8,    0.583291 },
            { 0xcdcd,    0.617925 },
            { 0xd2d2,    0.653361 },
            { 0xd7d7,    0.689553 },
            { 0xdcdc,    0.726454 },
            { 0xe1e1,    0.764013 },
            { 0xe6e6,    0.802178 },
            { 0xebeb,    0.840891 },
            { 0xf0f0,    0.880093 },
            { 0xf5f5,    0.919723 },
            { 0xfafa,    0.959715 },
	    { 0xffff,    1.00000 }
};

static IntensityTbl Default_RGB_RedTbl = {
    /* IntensityRec *pBase */
	(IntensityRec *) Default_RGB_RedTuples,
    /* unsigned int nEntries */
	52
};

static IntensityTbl Default_RGB_GreenTbl = {
    /* IntensityRec *pBase */
	(IntensityRec *)Default_RGB_GreenTuples,
    /* unsigned int nEntries */
	50
};

static IntensityTbl Default_RGB_BlueTbl = {
    /* IntensityRec *pBase */
	(IntensityRec *)Default_RGB_BlueTuples,
    /* unsigned int nEntries */
	51
};

static LINEAR_RGB_SCCData Default_RGB_SCCData = {
    /* XcmsFloat XYZtoRGBmatrix[3][3] */
  {
    { 3.48340481253539000, -1.52176374927285200, -0.55923133354049780 },
    {-1.07152751306193600,  1.96593795204372400,  0.03673691339553462 },
    { 0.06351179790497788, -0.20020501000496480,  0.81070942031648220 }
  },

    /* XcmsFloat RGBtoXYZmatrix[3][3] */
  {
    { 0.38106149108714790, 0.32025712365352110, 0.24834578525933100 },
    { 0.20729745115140850, 0.68054638776373240, 0.11215616108485920 },
    { 0.02133944350088028, 0.14297193020246480, 1.24172892629665500 }
  },

    /* IntensityTbl *pRedTbl */
	&Default_RGB_RedTbl,

    /* IntensityTbl *pGreenTbl */
	&Default_RGB_GreenTbl,

    /* IntensityTbl *pBlueTbl */
	&Default_RGB_BlueTbl
};

/************************************************************************
 *									*
 *			PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		LINEAR_RGB_InitSCCData()
 *
 *	SYNOPSIS
 */
static Status
LINEAR_RGB_InitSCCData(
    Display *dpy,
    int screenNumber,
    XcmsPerScrnInfo *pPerScrnInfo)
/*
 *	DESCRIPTION
 *
 *	RETURNS
 *		XcmsFailure if failed.
 *		XcmsSuccess if succeeded.
 *
 */
{
    Atom  CorrectAtom = XInternAtom (dpy, XDCCC_CORRECT_ATOM_NAME, True);
    Atom  MatrixAtom  = XInternAtom (dpy, XDCCC_MATRIX_ATOM_NAME, True);
    int	  format_return, count, cType, nTables;
    unsigned long nitems, nbytes_return;
    char *property_return, *pChar;
    XcmsFloat *pValue;
#ifdef ALLDEBUG
    IntensityRec *pIRec;
#endif /* ALLDEBUG */
    VisualID visualID;

    LINEAR_RGB_SCCData *pScreenData, *pScreenDefaultData;
    XcmsIntensityMap *pNewMap;

    /*
     * Allocate memory for pScreenData
     */
    if (!(pScreenData = pScreenDefaultData = (LINEAR_RGB_SCCData *)
		      Xcalloc (1, sizeof(LINEAR_RGB_SCCData)))) {
	return(XcmsFailure);
    }

    /*
     *  1. Get the XYZ->RGB and RGB->XYZ matrices
     */

    if (MatrixAtom == None ||
	!_XcmsGetProperty (dpy, RootWindow(dpy, screenNumber), MatrixAtom,
	   &format_return, &nitems, &nbytes_return, &property_return) ||
	   nitems != 18 || format_return != 32) {
	/*
	 * As per the XDCCC, there must be 18 data items and each must be
	 * in 32 bits !
	 */
	goto FreeSCCData;

    } else {

	/*
	 * RGBtoXYZ and XYZtoRGB matrices
	 */
	pValue = (XcmsFloat *) pScreenData;
	pChar = property_return;
	for (count = 0; count < 18; count++) {
	    *pValue++ = (long)_XcmsGetElement(format_return, &pChar,
		    &nitems) / (XcmsFloat)XDCCC_NUMBER;
	}
	Xfree (property_return);
	pPerScrnInfo->screenWhitePt.spec.CIEXYZ.X =
		pScreenData->RGBtoXYZmatrix[0][0] +
		pScreenData->RGBtoXYZmatrix[0][1] +
		pScreenData->RGBtoXYZmatrix[0][2];
	pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Y =
		pScreenData->RGBtoXYZmatrix[1][0] +
		pScreenData->RGBtoXYZmatrix[1][1] +
		pScreenData->RGBtoXYZmatrix[1][2];
	pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Z =
		pScreenData->RGBtoXYZmatrix[2][0] +
		pScreenData->RGBtoXYZmatrix[2][1] +
		pScreenData->RGBtoXYZmatrix[2][2];

	/*
	 * Compute the Screen White Point
	 */
	if ((pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Y < (1.0 - EPS) )
		|| (pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Y > (1.0 + EPS))) {
	    goto FreeSCCData;
	} else {
	    pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Y = 1.0;
	}
	pPerScrnInfo->screenWhitePt.format = XcmsCIEXYZFormat;
	pPerScrnInfo->screenWhitePt.pixel = 0;

#ifdef PDEBUG
	printf ("RGB to XYZ Matrix values:\n");
	printf ("       %f %f %f\n       %f %f %f\n       %f %f %f\n",
		pScreenData->RGBtoXYZmatrix[0][0],
		pScreenData->RGBtoXYZmatrix[0][1],
		pScreenData->RGBtoXYZmatrix[0][2],
		pScreenData->RGBtoXYZmatrix[1][0],
		pScreenData->RGBtoXYZmatrix[1][1],
		pScreenData->RGBtoXYZmatrix[1][2],
		pScreenData->RGBtoXYZmatrix[2][0],
		pScreenData->RGBtoXYZmatrix[2][1],
		pScreenData->RGBtoXYZmatrix[2][2]);
	printf ("XYZ to RGB Matrix values:\n");
	printf ("       %f %f %f\n       %f %f %f\n       %f %f %f\n",
		pScreenData->XYZtoRGBmatrix[0][0],
		pScreenData->XYZtoRGBmatrix[0][1],
		pScreenData->XYZtoRGBmatrix[0][2],
		pScreenData->XYZtoRGBmatrix[1][0],
		pScreenData->XYZtoRGBmatrix[1][1],
		pScreenData->XYZtoRGBmatrix[1][2],
		pScreenData->XYZtoRGBmatrix[2][0],
		pScreenData->XYZtoRGBmatrix[2][1],
		pScreenData->XYZtoRGBmatrix[2][2]);
	printf ("Screen White Pt value: %f %f %f\n",
		pPerScrnInfo->screenWhitePt.spec.CIEXYZ.X,
		pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Y,
		pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Z);
#endif /* PDEBUG */
    }

    /*
     *	2. Get the Intensity Profile
     */
    if (CorrectAtom == None ||
	!_XcmsGetProperty (dpy, RootWindow(dpy, screenNumber), CorrectAtom,
	   &format_return, &nitems, &nbytes_return, &property_return)) {
	goto FreeSCCData;
    }

    pChar = property_return;

    while (nitems) {
	switch (format_return) {
	  case 8:
	    /*
	     * Must have at least:
	     *		VisualID0
	     *		VisualID1
	     *		VisualID2
	     *		VisualID3
	     *		type
	     *		count
	     *		length
	     *		intensity1
	     *		intensity2
	     */
	    if (nitems < 9) {
		goto Free_property_return;
	    }
	    count = 3;
	    break;
	  case 16:
	    /*
	     * Must have at least:
	     *		VisualID0
	     *		VisualID3
	     *		type
	     *		count
	     *		length
	     *		intensity1
	     *		intensity2
	     */
	    if (nitems < 7) {
		goto Free_property_return;
	    }
	    count = 1;
	    break;
	  case 32:
	    /*
	     * Must have at least:
	     *		VisualID0
	     *		type
	     *		count
	     *		length
	     *		intensity1
	     *		intensity2
	     */
	    if (nitems < 6) {
		goto Free_property_return;
	    }
	    count = 0;
	    break;
	  default:
	    goto Free_property_return;
	}

	/*
	 * Get VisualID
	 */
	visualID = _XcmsGetElement(format_return, &pChar, &nitems);
	while (count--) {
	    visualID = visualID << format_return;
	    visualID |= _XcmsGetElement(format_return, &pChar, &nitems);
	}

	if (visualID == 0) {
	    /*
	     * This is a shared intensity table
	     */
	    pScreenData = pScreenDefaultData;
	} else {
	    /*
	     * This is a per-Visual intensity table
	     */
	    if (!(pScreenData = (LINEAR_RGB_SCCData *)
			      Xcalloc (1, sizeof(LINEAR_RGB_SCCData)))) {
		goto Free_property_return;
	    }
	    /* copy matrices */
	    memcpy((char *)pScreenData, (char *)pScreenDefaultData,
		   18 * sizeof(XcmsFloat));

	    /* Create, initialize, and add map */
	    if (!(pNewMap = (XcmsIntensityMap *)
			      Xcalloc (1, sizeof(XcmsIntensityMap)))) {
		Xfree(pScreenData);
		goto Free_property_return;
	    }
	    pNewMap->visualID = visualID;
	    pNewMap->screenData = (XPointer)pScreenData;
	    pNewMap->pFreeScreenData = LINEAR_RGB_FreeSCCData;
	    pNewMap->pNext =
		    (XcmsIntensityMap *)dpy->cms.perVisualIntensityMaps;
	    dpy->cms.perVisualIntensityMaps = (XPointer)pNewMap;
	    dpy->free_funcs->intensityMaps = _XcmsFreeIntensityMaps;
	}

	cType = _XcmsGetElement(format_return, &pChar, &nitems);
	nTables = _XcmsGetElement(format_return, &pChar, &nitems);

	if (cType == 0) {

	    /* Red Intensity Table */
	    if (!(pScreenData->pRedTbl = (IntensityTbl *)
		    Xcalloc (1, sizeof(IntensityTbl)))) {
		goto Free_property_return;
	    }
	    if (_XcmsGetTableType0(pScreenData->pRedTbl, format_return, &pChar,
		    &nitems) == XcmsFailure) {
		goto FreeRedTbl;
	    }

	    if (nTables == 1) {
		/* Green Intensity Table */
		pScreenData->pGreenTbl = pScreenData->pRedTbl;
		/* Blue Intensity Table */
		pScreenData->pBlueTbl = pScreenData->pRedTbl;
	    } else {
		/* Green Intensity Table */
		if (!(pScreenData->pGreenTbl = (IntensityTbl *)
			Xcalloc (1, sizeof(IntensityTbl)))) {
		    goto FreeRedTblElements;
		}
		if (_XcmsGetTableType0(pScreenData->pGreenTbl, format_return, &pChar,
			&nitems) == XcmsFailure) {
		    goto FreeGreenTbl;
		}

		/* Blue Intensity Table */
		if (!(pScreenData->pBlueTbl = (IntensityTbl *)
			Xcalloc (1, sizeof(IntensityTbl)))) {
		    goto FreeGreenTblElements;
		}
		if (_XcmsGetTableType0(pScreenData->pBlueTbl, format_return, &pChar,
			&nitems) == XcmsFailure) {
		    goto FreeBlueTbl;
		}
	    }
	} else if (cType == 1) {
	    /* Red Intensity Table */
	    if (!(pScreenData->pRedTbl = (IntensityTbl *)
		    Xcalloc (1, sizeof(IntensityTbl)))) {
		goto Free_property_return;
	    }
	    if (_XcmsGetTableType1(pScreenData->pRedTbl, format_return, &pChar,
		    &nitems) == XcmsFailure) {
		goto FreeRedTbl;
	    }

	    if (nTables == 1) {

		/* Green Intensity Table */
		pScreenData->pGreenTbl = pScreenData->pRedTbl;
		/* Blue Intensity Table */
		pScreenData->pBlueTbl = pScreenData->pRedTbl;

	    } else {

		/* Green Intensity Table */
		if (!(pScreenData->pGreenTbl = (IntensityTbl *)
			Xcalloc (1, sizeof(IntensityTbl)))) {
		    goto FreeRedTblElements;
		}
		if (_XcmsGetTableType1(pScreenData->pGreenTbl, format_return, &pChar,
			&nitems) == XcmsFailure) {
		    goto FreeGreenTbl;
		}

		/* Blue Intensity Table */
		if (!(pScreenData->pBlueTbl = (IntensityTbl *)
			Xcalloc (1, sizeof(IntensityTbl)))) {
		    goto FreeGreenTblElements;
		}
		if (_XcmsGetTableType1(pScreenData->pBlueTbl, format_return, &pChar,
			&nitems) == XcmsFailure) {
		    goto FreeBlueTbl;
		}
	    }
	} else {
	    goto Free_property_return;
	}

#ifdef ALLDEBUG
	printf ("Intensity Table  RED    %d\n", pScreenData->pRedTbl->nEntries);
	pIRec = (IntensityRec *) pScreenData->pRedTbl->pBase;
	for (count = 0; count < pScreenData->pRedTbl->nEntries; count++, pIRec++) {
	    printf ("\t0x%4x\t%f\n", pIRec->value, pIRec->intensity);
	}
	if (pScreenData->pGreenTbl->pBase != pScreenData->pRedTbl->pBase) {
	    printf ("Intensity Table  GREEN  %d\n", pScreenData->pGreenTbl->nEntries);
	    pIRec = (IntensityRec *)pScreenData->pGreenTbl->pBase;
	    for (count = 0; count < pScreenData->pGreenTbl->nEntries; count++, pIRec++) {
		printf ("\t0x%4x\t%f\n", pIRec->value, pIRec->intensity);
	    }
	}
	if (pScreenData->pBlueTbl->pBase != pScreenData->pRedTbl->pBase) {
	    printf ("Intensity Table  BLUE   %d\n", pScreenData->pBlueTbl->nEntries);
	    pIRec = (IntensityRec *) pScreenData->pBlueTbl->pBase;
	    for (count = 0; count < pScreenData->pBlueTbl->nEntries; count++, pIRec++) {
		printf ("\t0x%4x\t%f\n", pIRec->value, pIRec->intensity);
	    }
	}
#endif /* ALLDEBUG */
    }

    Xfree (property_return);

    /* Free the old memory and use the new structure created. */
    LINEAR_RGB_FreeSCCData(pPerScrnInfo->screenData);

    pPerScrnInfo->functionSet = (XPointer) &XcmsLinearRGBFunctionSet;

    pPerScrnInfo->screenData = (XPointer) pScreenData;

    pPerScrnInfo->state = XcmsInitSuccess;

    return(XcmsSuccess);

FreeBlueTblElements: _X_UNUSED
    Xfree(pScreenData->pBlueTbl->pBase);

FreeBlueTbl:
    Xfree(pScreenData->pBlueTbl);

FreeGreenTblElements:
    Xfree(pScreenData->pGreenTbl->pBase);

FreeGreenTbl:
    Xfree(pScreenData->pGreenTbl);

FreeRedTblElements:
    Xfree(pScreenData->pRedTbl->pBase);

FreeRedTbl:
    Xfree(pScreenData->pRedTbl);

Free_property_return:
    Xfree (property_return);

FreeSCCData:
    Xfree(pScreenDefaultData);
    pPerScrnInfo->state = XcmsInitNone;
    return(XcmsFailure);
}


/*
 *	NAME
 *		LINEAR_RGB_FreeSCCData()
 *
 *	SYNOPSIS
 */
static void
LINEAR_RGB_FreeSCCData(
    XPointer pScreenDataTemp)
/*
 *	DESCRIPTION
 *
 *	RETURNS
 *		0 if failed.
 *		1 if succeeded with no modifications.
 *
 */
{
    LINEAR_RGB_SCCData *pScreenData = (LINEAR_RGB_SCCData *) pScreenDataTemp;

    if (pScreenData && pScreenData != &Default_RGB_SCCData) {
	if (pScreenData->pRedTbl) {
	    if (pScreenData->pGreenTbl) {
		if (pScreenData->pRedTbl->pBase !=
		    pScreenData->pGreenTbl->pBase) {
		    if (pScreenData->pGreenTbl->pBase) {
			Xfree (pScreenData->pGreenTbl->pBase);
		    }
		}
		if (pScreenData->pGreenTbl != pScreenData->pRedTbl) {
		    Xfree (pScreenData->pGreenTbl);
		}
	    }
	    if (pScreenData->pBlueTbl) {
		if (pScreenData->pRedTbl->pBase !=
		    pScreenData->pBlueTbl->pBase) {
		    if (pScreenData->pBlueTbl->pBase) {
			Xfree (pScreenData->pBlueTbl->pBase);
		    }
		}
		if (pScreenData->pBlueTbl != pScreenData->pRedTbl) {
		    Xfree (pScreenData->pBlueTbl);
		}
	    }
	    if (pScreenData->pRedTbl->pBase) {
		Xfree (pScreenData->pRedTbl->pBase);
	    }
	    Xfree (pScreenData->pRedTbl);
	}
	Xfree (pScreenData);
    }
}



/************************************************************************
 *									*
 *			API PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		_XcmsGetTableType0
 *
 *	SYNOPSIS
 */
static Status
_XcmsGetTableType0(
    IntensityTbl *pTbl,
    int	  format,
    char **pChar,
    unsigned long *pCount)
/*
 *	DESCRIPTION
 *
 *	RETURNS
 *		XcmsFailure if failed.
 *		XcmsSuccess if succeeded.
 *
 */
{
    unsigned int nElements;
    IntensityRec *pIRec;

    nElements = pTbl->nEntries =
	    _XcmsGetElement(format, pChar, pCount) + 1;
    if (!(pIRec = pTbl->pBase = (IntensityRec *)
	  Xcalloc (nElements, sizeof(IntensityRec)))) {
	return(XcmsFailure);
    }

    switch (format) {
      case 8:
	for (; nElements--; pIRec++) {
	    /* 0xFFFF/0xFF = 0x101 */
	    pIRec->value = _XcmsGetElement (format, pChar, pCount) * 0x101;
	    pIRec->intensity =
		    _XcmsGetElement (format, pChar, pCount) / (XcmsFloat)255.0;
	}
	break;
      case 16:
	for (; nElements--; pIRec++) {
	    pIRec->value = _XcmsGetElement (format, pChar, pCount);
	    pIRec->intensity = _XcmsGetElement (format, pChar, pCount)
		    / (XcmsFloat)65535.0;
	}
	break;
      case 32:
	for (; nElements--; pIRec++) {
	    pIRec->value = _XcmsGetElement (format, pChar, pCount);
	    pIRec->intensity = _XcmsGetElement (format, pChar, pCount)
		    / (XcmsFloat)4294967295.0;
	}
	break;
      default:
	return(XcmsFailure);
    }
    return(XcmsSuccess);
}


/*
 *	NAME
 *		_XcmsGetTableType1
 *
 *	SYNOPSIS
 */
static Status
_XcmsGetTableType1(
    IntensityTbl *pTbl,
    int	  format,
    char **pChar,
    unsigned long *pCount)
/*
 *	DESCRIPTION
 *
 *	RETURNS
 *		XcmsFailure if failed.
 *		XcmsSuccess if succeeded.
 *
 */
{
    unsigned int count;
    unsigned int max_index;
    IntensityRec *pIRec;

    max_index = _XcmsGetElement(format, pChar, pCount);
    pTbl->nEntries = max_index + 1;
    if (!(pIRec = pTbl->pBase = (IntensityRec *)
	  Xcalloc (max_index+1, sizeof(IntensityRec)))) {
	return(XcmsFailure);
    }

    switch (format) {
      case 8:
	for (count = 0; count < max_index+1; count++, pIRec++) {
	    pIRec->value = (count * 65535) / max_index;
	    pIRec->intensity = _XcmsGetElement (format, pChar, pCount)
		    / (XcmsFloat)255.0;
	}
	break;
      case 16:
	for (count = 0; count < max_index+1; count++, pIRec++) {
	    pIRec->value = (count * 65535) / max_index;
	    pIRec->intensity = _XcmsGetElement (format, pChar, pCount)
		    / (XcmsFloat)65535.0;
	}
	break;
      case 32:
	for (count = 0; count < max_index+1; count++, pIRec++) {
	    pIRec->value = (count * 65535) / max_index;
	    pIRec->intensity = _XcmsGetElement (format, pChar, pCount)
		    / (XcmsFloat)4294967295.0;
	}
	break;
      default:
	return(XcmsFailure);
    }

    return(XcmsSuccess);
}


/*
 *	NAME
 *		ValueCmp
 *
 *	SYNOPSIS
 */
static int
_XcmsValueCmp(
    IntensityRec *p1, IntensityRec *p2)
/*
 *	DESCRIPTION
 *		Compares the value component of two IntensityRec
 *		structures.
 *
 *	RETURNS
 *		0 if p1->value is equal to p2->value
 *		< 0 if p1->value is less than p2->value
 *		> 0 if p1->value is greater than p2->value
 *
 */
{
    return (p1->value - p2->value);
}


/*
 *	NAME
 *		IntensityCmp
 *
 *	SYNOPSIS
 */
static int
_XcmsIntensityCmp(
    IntensityRec *p1, IntensityRec *p2)
/*
 *	DESCRIPTION
 *		Compares the intensity component of two IntensityRec
 *		structures.
 *
 *	RETURNS
 *		0 if equal;
 *		< 0 if first precedes second
 *		> 0 if first succeeds second
 *
 */
{
    if (p1->intensity < p2->intensity) {
	return (-1);
    }
    if (p1->intensity > p2->intensity) {
	return (XcmsSuccess);
    }
    return (XcmsFailure);
}

/*
 *	NAME
 *		ValueInterpolation
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
static int
_XcmsValueInterpolation(
    IntensityRec *key, IntensityRec *lo, IntensityRec *hi, IntensityRec *answer,
    int bitsPerRGB)
/*
 *	DESCRIPTION
 *		Based on a given value, performs a linear interpolation
 *		on the intensities between two IntensityRec structures.
 *		Note that the bitsPerRGB parameter is ignored.
 *
 *	RETURNS
 *		Returns 0 if failed; otherwise non-zero.
 */
{
    XcmsFloat ratio;

    ratio = ((XcmsFloat)key->value - (XcmsFloat)lo->value) /
	((XcmsFloat)hi->value - (XcmsFloat)lo->value);
    answer->value = key->value;
    answer->intensity = (hi->intensity - lo->intensity) * ratio;
    answer->intensity += lo->intensity;
    return (XcmsSuccess);
}

/*
 *	NAME
 *		IntensityInterpolation
 *
 *	SYNOPSIS
 */
static int
_XcmsIntensityInterpolation(
    IntensityRec *key, IntensityRec *lo, IntensityRec *hi, IntensityRec *answer,
    int bitsPerRGB)
/*
 *	DESCRIPTION
 *		Based on a given intensity, performs a linear interpolation
 *		on the values between two IntensityRec structures.
 *		The bitsPerRGB parameter is necessary to perform rounding
 *		to the correct number of significant bits.
 *
 *	RETURNS
 *		Returns 0 if failed; otherwise non-zero.
 */
{
    XcmsFloat ratio;
    long target, up, down;
    int shift = 16 - bitsPerRGB;
    int max_color = (1 << bitsPerRGB) - 1;

    ratio = (key->intensity - lo->intensity) / (hi->intensity - lo->intensity);
    answer->intensity = key->intensity;
    target = hi->value - lo->value;
    target *= ratio;
    target += lo->value;

    /*
     * Ok now, lets find the closest in respects to bits per RGB
     */
    up = ((target >> shift) * 0xFFFF) / max_color;
    if (up < target) {
	down = up;
	up = (MIN((down >> shift) + 1, max_color) * 0xFFFF) / max_color;
    } else {
	down = (MAX((up >> shift) - 1, 0) * 0xFFFF) / max_color;
    }
    answer->value = ((up - target) < (target - down) ? up : down);
    answer->value &= MASK[bitsPerRGB];
    return (XcmsSuccess);
}



typedef int (*comparProcp)(
    char *p1,
    char *p2);
typedef int (*interpolProcp)(
    char *key,
    char *lo,
    char *hi,
    char *answer,
    int bitsPerRGB);

/*
 *	NAME
 *		_XcmsTableSearch
 *
 *	SYNOPSIS
 */
static int
_XcmsTableSearch(
    char *key,
    int bitsPerRGB,
    char *base,
    unsigned nel,
    unsigned nKeyPtrSize,
    int (*compar)(
        char *p1,
        char *p2),
    int (*interpol)(
        char *key,
        char *lo,
        char *hi,
        char *answer,
        int bitsPerRGB),
    char *answer)

/*
 *	DESCRIPTION
 *		A binary search through the specified table.
 *
 *	RETURNS
 *		Returns 0 if failed; otherwise non-zero.
 *
 */
{
    char *hi, *lo, *mid, *last;
    int result;

    last = hi = base + ((nel - 1) * nKeyPtrSize);
    mid = lo = base;

    /* use only the significants bits, then scale into 16 bits */
    ((IntensityRec *)key)->value = ((unsigned long)
	    (((IntensityRec *)key)->value >> (16 - bitsPerRGB)) * 0xFFFF)
	    / ((1 << bitsPerRGB) - 1);

    /* Special case so that zero intensity always maps to zero value */
    if ((*compar) (key,lo) <= 0) {
	memcpy (answer, lo, nKeyPtrSize);
	((IntensityRec *)answer)->value &= MASK[bitsPerRGB];
	return XcmsSuccess;
    }
    while (mid != last) {
	last = mid;
	mid = lo + (((unsigned)(hi - lo) / nKeyPtrSize) / 2) * nKeyPtrSize;
	result = (*compar) (key, mid);
	if (result == 0) {

	    memcpy(answer, mid, nKeyPtrSize);
	    ((IntensityRec *)answer)->value &= MASK[bitsPerRGB];
	    return (XcmsSuccess);
	} else if (result < 0) {
	    hi = mid;
	} else {
	    lo = mid;
	}
    }

    /*
     * If we got to here, we didn't find a solution, so we
     * need to apply interpolation.
     */
    return ((*interpol)(key, lo, hi, answer, bitsPerRGB));
}


/*
 *      NAME
 *		_XcmsMatVec - multiply a 3 x 3 by a 3 x 1 vector
 *
 *	SYNOPSIS
 */
static void _XcmsMatVec(
    XcmsFloat *pMat, XcmsFloat *pIn, XcmsFloat *pOut)
/*
 *      DESCRIPTION
 *		Multiply the passed vector by the passed matrix to return a
 *		vector. Matrix is 3x3, vectors are of length 3.
 *
 *	RETURNS
 *		void
 */
{
    int i, j;

    for (i = 0; i < 3; i++) {
	pOut[i] = 0.0;
	for (j = 0; j < 3; j++)
	    pOut[i] += *(pMat+(i*3)+j) * pIn[j];
    }
}


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/


/*
 *	NAME
 *		XcmsLRGB_RGB_ParseString
 *
 *	SYNOPSIS
 */
static int
XcmsLRGB_RGB_ParseString(
    register char *spec,
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		This routines takes a string and attempts to convert
 *		it into a XcmsColor structure with XcmsRGBFormat.
 *
 *	RETURNS
 *		0 if failed, non-zero otherwise.
 */
{
    register int n, i;
    unsigned short r, g, b;
    char c;
    char *pchar;
    unsigned short *pShort;

    /*
     * Check for old # format
     */
    if (*spec == '#') {
	/*
	 * Attempt to parse the value portion.
	 */
	spec++;
	n = (int)strlen(spec);
	if (n != 3 && n != 6 && n != 9 && n != 12) {
	    return(XcmsFailure);
	}

	n /= 3;
	g = b = 0;
	do {
	    r = g;
	    g = b;
	    b = 0;
	    for (i = n; --i >= 0; ) {
		c = *spec++;
		b <<= 4;
		if (c >= '0' && c <= '9')
		    b |= c - '0';
		/* assume string in lowercase
		else if (c >= 'A' && c <= 'F')
		    b |= c - ('A' - 10);
		*/
		else if (c >= 'a' && c <= 'f')
		    b |= c - ('a' - 10);
		else return (XcmsFailure);
	    }
	} while (*spec != '\0');

	/*
	 * Succeeded !
	 */
	n <<= 2;
	n = 16 - n;
	/* shift instead of scale, to match old broken semantics */
	pColor->spec.RGB.red = r << n;
	pColor->spec.RGB.green = g << n;
	pColor->spec.RGB.blue =  b << n;
    } else {
	if ((pchar = strchr(spec, ':')) == NULL) {
	    return(XcmsFailure);
	}
	n = (int)(pchar - spec);

	/*
	 * Check for proper prefix.
	 */
	if (strncmp(spec, _XcmsRGB_prefix, (size_t)n) != 0) {
	    return(XcmsFailure);
	}

	/*
	 * Attempt to parse the value portion.
	 */
	spec += (n + 1);
	pShort = &pColor->spec.RGB.red;
	for (i = 0; i < 3; i++, pShort++, spec++) {
	    n = 0;
	    *pShort = 0;
	    while (*spec != '/' && *spec != '\0') {
		if (++n > 4) {
		    return(XcmsFailure);
		}
		c = *spec++;
		*pShort <<= 4;
		if (c >= '0' && c <= '9')
		    *pShort |= c - '0';
		/* assume string in lowercase
		else if (c >= 'A' && c <= 'F')
		    *pShort |= c - ('A' - 10);
		*/
		else if (c >= 'a' && c <= 'f')
		    *pShort |= c - ('a' - 10);
		else return (XcmsFailure);
	    }
	    if (n == 0)
		return (XcmsFailure);
	    if (n < 4) {
		*pShort = ((unsigned long)*pShort * 0xFFFF) / ((1 << n*4) - 1);
	    }
	}
    }
    pColor->format = XcmsRGBFormat;
    pColor->pixel = 0;
    return (XcmsSuccess);
}


/*
 *	NAME
 *		XcmsLRGB_RGBi_ParseString
 *
 *	SYNOPSIS
 */
static int
XcmsLRGB_RGBi_ParseString(
    register char *spec,
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		This routines takes a string and attempts to convert
 *		it into a XcmsColor structure with XcmsRGBiFormat.
 *		The assumed RGBi string syntax is:
 *		    RGBi:<r>/<g>/<b>
 *		Where r, g, and b are in string input format for floats
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
    size_t n;
    char *pchar;

    if ((pchar = strchr(spec, ':')) == NULL) {
	return(XcmsFailure);
    }
    n = (size_t)(pchar - spec);

    /*
     * Check for proper prefix.
     */
    if (strncmp(spec, _XcmsRGBi_prefix, n) != 0) {
	return(XcmsFailure);
    }

    /*
     * Attempt to parse the value portion.
     */
    if (sscanf(spec + n + 1, "%lf/%lf/%lf",
	    &pColor->spec.RGBi.red,
	    &pColor->spec.RGBi.green,
	    &pColor->spec.RGBi.blue) != 3) {
        char *s; /* Maybe failed due to locale */
        int f;
        if ((s = strdup(spec))) {
            for (f = 0; s[f]; ++f)
                if (s[f] == '.')
                    s[f] = ',';
                else if (s[f] == ',')
                    s[f] = '.';
	    if (sscanf(s + n + 1, "%lf/%lf/%lf",
		       &pColor->spec.RGBi.red,
		       &pColor->spec.RGBi.green,
		       &pColor->spec.RGBi.blue) != 3) {
                free(s);
                return(XcmsFailure);
            }
            free(s);
        } else
	    return(XcmsFailure);
    }

    /*
     * Succeeded !
     */
    pColor->format = XcmsRGBiFormat;
    pColor->pixel = 0;
    return (XcmsSuccess);
}


/*
 *	NAME
 *		XcmsCIEXYZToRGBi - convert CIE XYZ to RGB
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsCIEXYZToRGBi(
    XcmsCCC ccc,
    XcmsColor *pXcmsColors_in_out,/* pointer to XcmsColors to convert 	*/
    unsigned int nColors,	/* Number of colors			*/
    Bool *pCompressed)		/* pointer to an array of Bool		*/
/*
 *	DESCRIPTION
 *		Converts color specifications in an array of XcmsColor
 *		structures from RGB format to RGBi format.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded without gamut compression.
 *		XcmsSuccessWithCompression if succeeded with gamut
 *			compression.
 */
{
    LINEAR_RGB_SCCData *pScreenData;
    XcmsFloat tmp[3];
    int hasCompressed = 0;
    unsigned int i;
    XcmsColor *pColor = pXcmsColors_in_out;

    if (ccc == NULL) {
	return(XcmsFailure);
    }

    pScreenData = (LINEAR_RGB_SCCData *)ccc->pPerScrnInfo->screenData;

    /*
     * XcmsColors should be White Point Adjusted, if necessary, by now!
     */

    /*
     * NEW!!! for extended gamut compression
     *
     * 1. Need to zero out pCompressed
     *
     * 2. Need to save initial address of pColor
     *
     * 3. Need to save initial address of pCompressed
     */

    for (i = 0; i < nColors; i++) {

	/* Make sure format is XcmsCIEXYZFormat */
	if (pColor->format != XcmsCIEXYZFormat) {
	    return(XcmsFailure);
	}

	/* Multiply [A]-1 * [XYZ] to get RGB intensity */
	_XcmsMatVec((XcmsFloat *) pScreenData->XYZtoRGBmatrix,
		(XcmsFloat *) &pColor->spec, tmp);

	if ((MIN3 (tmp[0], tmp[1], tmp[2]) < -EPS) ||
	    (MAX3 (tmp[0], tmp[1], tmp[2]) > (1.0 + EPS))) {

	    /*
	     * RGBi out of screen's gamut
	     */

	    if (ccc->gamutCompProc == NULL) {
		/*
		 * Aha!! Here's that little trick that will allow
		 * gamut compression routines to get the out of bound
		 * RGBi.
		 */
		memcpy((char *)&pColor->spec, (char *)tmp, sizeof(tmp));
		pColor->format = XcmsRGBiFormat;
		return(XcmsFailure);
	    } else if ((*ccc->gamutCompProc)(ccc, pXcmsColors_in_out, nColors,
		    i, pCompressed) == 0) {
		return(XcmsFailure);
	    }

	    /*
	     * The gamut compression function should return colors in CIEXYZ
	     *	Also check again to if the new color is within gamut.
	     */
	    if (pColor->format != XcmsCIEXYZFormat) {
		return(XcmsFailure);
	    }
	    _XcmsMatVec((XcmsFloat *) pScreenData->XYZtoRGBmatrix,
		    (XcmsFloat *) &pColor->spec, tmp);
	    if ((MIN3 (tmp[0], tmp[1], tmp[2]) < -EPS) ||
		(MAX3 (tmp[0], tmp[1], tmp[2]) > (1.0 + EPS))) {
		return(XcmsFailure);
	    }
	    hasCompressed++;
	}
	memcpy((char *)&pColor->spec, (char *)tmp, sizeof(tmp));
	/* These if statements are done to ensure the fudge factor is */
	/* is taken into account. */
	if (pColor->spec.RGBi.red < 0.0) {
		pColor->spec.RGBi.red = 0.0;
	} else if (pColor->spec.RGBi.red > 1.0) {
		pColor->spec.RGBi.red = 1.0;
	}
	if (pColor->spec.RGBi.green < 0.0) {
		pColor->spec.RGBi.green = 0.0;
	} else if (pColor->spec.RGBi.green > 1.0) {
		pColor->spec.RGBi.green = 1.0;
	}
	if (pColor->spec.RGBi.blue < 0.0) {
		pColor->spec.RGBi.blue = 0.0;
	} else if (pColor->spec.RGBi.blue > 1.0) {
		pColor->spec.RGBi.blue = 1.0;
	}
	(pColor++)->format = XcmsRGBiFormat;
    }
    return (hasCompressed ? XcmsSuccessWithCompression : XcmsSuccess);
}


/*
 *	NAME
 *		LINEAR_RGBi_to_CIEXYZ - convert RGBi to CIEXYZ
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsRGBiToCIEXYZ(
    XcmsCCC ccc,
    XcmsColor *pXcmsColors_in_out,/* pointer to XcmsColors to convert 	*/
    unsigned int nColors,	/* Number of colors			*/
    Bool *pCompressed)		/* pointer to a bit array		*/
/*
 *	DESCRIPTION
 *		Converts color specifications in an array of XcmsColor
 *		structures from RGBi format to CIEXYZ format.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded.
 */
{
    LINEAR_RGB_SCCData *pScreenData;
    XcmsFloat tmp[3];

    /*
     * pCompressed ignored in this function.
     */

    if (ccc == NULL) {
	return(XcmsFailure);
    }

    pScreenData = (LINEAR_RGB_SCCData *)ccc->pPerScrnInfo->screenData;

    /*
     * XcmsColors should be White Point Adjusted, if necessary, by now!
     */

    while (nColors--) {

	/* Multiply [A]-1 * [XYZ] to get RGB intensity */
	_XcmsMatVec((XcmsFloat *) pScreenData->RGBtoXYZmatrix,
		(XcmsFloat *) &pXcmsColors_in_out->spec, tmp);

	memcpy((char *)&pXcmsColors_in_out->spec, (char *)tmp, sizeof(tmp));
	(pXcmsColors_in_out++)->format = XcmsCIEXYZFormat;
    }
    return(XcmsSuccess);
}


/*
 *	NAME
 *		XcmsRGBiToRGB
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsRGBiToRGB(
    XcmsCCC ccc,
    XcmsColor *pXcmsColors_in_out,/* pointer to XcmsColors to convert 	*/
    unsigned int nColors,	/* Number of colors			*/
    Bool *pCompressed)		/* pointer to a bit array		*/
/*
 *	DESCRIPTION
 *		Converts color specifications in an array of XcmsColor
 *		structures from RGBi format to RGB format.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded without gamut compression.
 *		XcmsSuccessWithCompression if succeeded with gamut
 *			compression.
 */
{
    LINEAR_RGB_SCCData *pScreenData;
    XcmsRGB tmpRGB;
    IntensityRec keyIRec, answerIRec;

    /*
     * pCompressed ignored in this function.
     */

    if (ccc == NULL) {
	return(XcmsFailure);
    }

    pScreenData = (LINEAR_RGB_SCCData *)ccc->pPerScrnInfo->screenData;

    while (nColors--) {

	/* Make sure format is XcmsRGBiFormat */
	if (pXcmsColors_in_out->format != XcmsRGBiFormat) {
	    return(XcmsFailure);
	}

	keyIRec.intensity = pXcmsColors_in_out->spec.RGBi.red;
	if (!_XcmsTableSearch((char *)&keyIRec, ccc->visual->bits_per_rgb,
		(char *)pScreenData->pRedTbl->pBase,
		(unsigned)pScreenData->pRedTbl->nEntries,
		(unsigned)sizeof(IntensityRec),
		(comparProcp)_XcmsIntensityCmp, (interpolProcp)_XcmsIntensityInterpolation, (char *)&answerIRec)) {
	    return(XcmsFailure);
	}
	tmpRGB.red = answerIRec.value;

	keyIRec.intensity = pXcmsColors_in_out->spec.RGBi.green;
	if (!_XcmsTableSearch((char *)&keyIRec, ccc->visual->bits_per_rgb,
		(char *)pScreenData->pGreenTbl->pBase,
		(unsigned)pScreenData->pGreenTbl->nEntries,
		(unsigned)sizeof(IntensityRec),
		(comparProcp)_XcmsIntensityCmp, (interpolProcp)_XcmsIntensityInterpolation, (char *)&answerIRec)) {
	    return(XcmsFailure);
	}
	tmpRGB.green = answerIRec.value;

	keyIRec.intensity = pXcmsColors_in_out->spec.RGBi.blue;
	if (!_XcmsTableSearch((char *)&keyIRec, ccc->visual->bits_per_rgb,
		(char *)pScreenData->pBlueTbl->pBase,
		(unsigned)pScreenData->pBlueTbl->nEntries,
		(unsigned)sizeof(IntensityRec),
		(comparProcp)_XcmsIntensityCmp, (interpolProcp)_XcmsIntensityInterpolation, (char *)&answerIRec)) {
	    return(XcmsFailure);
	}
	tmpRGB.blue = answerIRec.value;

	memcpy((char *)&pXcmsColors_in_out->spec, (char *)&tmpRGB, sizeof(XcmsRGB));
	(pXcmsColors_in_out++)->format = XcmsRGBFormat;
    }
    return(XcmsSuccess);
}


/*
 *	NAME
 *		XcmsRGBToRGBi
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsRGBToRGBi(
    XcmsCCC ccc,
    XcmsColor *pXcmsColors_in_out,/* pointer to XcmsColors to convert 	*/
    unsigned int nColors,	/* Number of colors			*/
    Bool *pCompressed)		/* pointer to a bit array		*/
/*
 *	DESCRIPTION
 *		Converts color specifications in an array of XcmsColor
 *		structures from RGB format to RGBi format.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded.
 */
{
    LINEAR_RGB_SCCData *pScreenData;
    XcmsRGBi tmpRGBi;
    IntensityRec keyIRec, answerIRec;

    /*
     * pCompressed ignored in this function.
     */

    if (ccc == NULL) {
	return(XcmsFailure);
    }

    pScreenData = (LINEAR_RGB_SCCData *)ccc->pPerScrnInfo->screenData;

    while (nColors--) {

	/* Make sure format is XcmsRGBFormat */
	if (pXcmsColors_in_out->format != XcmsRGBFormat) {
	    return(XcmsFailure);
	}

	keyIRec.value = pXcmsColors_in_out->spec.RGB.red;
	if (!_XcmsTableSearch((char *)&keyIRec, ccc->visual->bits_per_rgb,
		(char *)pScreenData->pRedTbl->pBase,
		(unsigned)pScreenData->pRedTbl->nEntries,
		(unsigned)sizeof(IntensityRec),
		(comparProcp)_XcmsValueCmp, (interpolProcp)_XcmsValueInterpolation, (char *)&answerIRec)) {
	    return(XcmsFailure);
	}
	tmpRGBi.red = answerIRec.intensity;

	keyIRec.value = pXcmsColors_in_out->spec.RGB.green;
	if (!_XcmsTableSearch((char *)&keyIRec, ccc->visual->bits_per_rgb,
		(char *)pScreenData->pGreenTbl->pBase,
		(unsigned)pScreenData->pGreenTbl->nEntries,
		(unsigned)sizeof(IntensityRec),
		(comparProcp)_XcmsValueCmp, (interpolProcp)_XcmsValueInterpolation, (char *)&answerIRec)) {
	    return(XcmsFailure);
	}
	tmpRGBi.green = answerIRec.intensity;

	keyIRec.value = pXcmsColors_in_out->spec.RGB.blue;
	if (!_XcmsTableSearch((char *)&keyIRec, ccc->visual->bits_per_rgb,
		(char *)pScreenData->pBlueTbl->pBase,
		(unsigned)pScreenData->pBlueTbl->nEntries,
		(unsigned)sizeof(IntensityRec),
		(comparProcp)_XcmsValueCmp, (interpolProcp)_XcmsValueInterpolation, (char *)&answerIRec)) {
	    return(XcmsFailure);
	}
	tmpRGBi.blue = answerIRec.intensity;

	memcpy((char *)&pXcmsColors_in_out->spec, (char *)&tmpRGBi, sizeof(XcmsRGBi));
	(pXcmsColors_in_out++)->format = XcmsRGBiFormat;
    }
    return(XcmsSuccess);
}

/*
 *	NAME
 *		_XcmsInitScrnDefaultInfo
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
int
_XcmsLRGB_InitScrnDefault(
    Display *dpy,
    int screenNumber,
    XcmsPerScrnInfo *pPerScrnInfo)
/*
 *	DESCRIPTION
 *		Given a display and screen number, this routine attempts
 *		to initialize the Xcms per Screen Info structure
 *		(XcmsPerScrnInfo) with defaults.
 *
 *	RETURNS
 *		Returns zero if initialization failed; non-zero otherwise.
 */
{
    pPerScrnInfo->screenData = (XPointer)&Default_RGB_SCCData;
    pPerScrnInfo->screenWhitePt.spec.CIEXYZ.X =
		Default_RGB_SCCData.RGBtoXYZmatrix[0][0] +
		Default_RGB_SCCData.RGBtoXYZmatrix[0][1] +
		Default_RGB_SCCData.RGBtoXYZmatrix[0][2];
    pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Y =
		Default_RGB_SCCData.RGBtoXYZmatrix[1][0] +
		Default_RGB_SCCData.RGBtoXYZmatrix[1][1] +
		Default_RGB_SCCData.RGBtoXYZmatrix[1][2];
    pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Z =
		Default_RGB_SCCData.RGBtoXYZmatrix[2][0] +
		Default_RGB_SCCData.RGBtoXYZmatrix[2][1] +
		Default_RGB_SCCData.RGBtoXYZmatrix[2][2];
    if ((pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Y < (1.0 - EPS) )
	    || (pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Y > (1.0 + EPS))) {
	pPerScrnInfo->screenData = (XPointer)NULL;
	pPerScrnInfo->state = XcmsInitNone;
	return(0);
    }
    pPerScrnInfo->screenWhitePt.spec.CIEXYZ.Y = 1.0;
    pPerScrnInfo->screenWhitePt.format = XcmsCIEXYZFormat;
    pPerScrnInfo->screenWhitePt.pixel = 0;
    pPerScrnInfo->functionSet = (XPointer)&XcmsLinearRGBFunctionSet;
    pPerScrnInfo->state = XcmsInitFailure; /* default initialization */
    return(1);
}
