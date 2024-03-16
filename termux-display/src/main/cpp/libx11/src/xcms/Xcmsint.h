
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
 *	DESCRIPTION
 *		Private include file for Color Management System.
 *		(i.e., for API internal use only)
 *
 */

#ifndef _XCMSINT_H_
#define _XCMSINT_H_

#include <X11/Xcms.h>

/*
 *	DEFINES
 */

	/*
	 * Private Status Value
	 */
#define	_XCMS_NEWNAME	-1

	/*
	 * Color Space ID's are of XcmsColorFormat type.
	 *
	 *	bit 31
	 *	    0 == Device-Independent
	 *	    1 == Device-Dependent
	 *
	 *	bit 30:
         *          0 == Registered with X Consortium
         *          1 == Unregistered
         */
#define       XCMS_DD_ID(id)          ((id) & (XcmsColorFormat)0x80000000)
#define       XCMS_DI_ID(id)          (!((id) & (XcmsColorFormat)0x80000000))
#define       XCMS_UNREG_ID(id)       ((id) & (XcmsColorFormat)0x40000000)
#define       XCMS_REG_ID(id)         (!((id) & (XcmsColorFormat)0x40000000))
#define       XCMS_FIRST_REG_DI_ID    (XcmsColorFormat)0x00000001
#define       XCMS_FIRST_UNREG_DI_ID  (XcmsColorFormat)0x40000000
#define       XCMS_FIRST_REG_DD_ID    (XcmsColorFormat)0x80000000
#define       XCMS_FIRST_UNREG_DD_ID  (XcmsColorFormat)0xc0000000

/*
 *	TYPEDEFS
 */

    /*
     * Structure for caching Colormap info.
     *    This is provided for the Xlib modifications to:
     *		XAllocNamedColor()
     *		XLookupColor()
     *		XParseColor()
     *		XStoreNamedColor()
     */
typedef struct _XcmsCmapRec {
    Colormap cmapID;
    Display *dpy;
    Window windowID;
    Visual *visual;
    struct _XcmsCCC *ccc;
    struct _XcmsCmapRec *pNext;
} XcmsCmapRec;

    /*
     * Intensity Record (i.e., value / intensity tuple)
     */
typedef struct _IntensityRec {
    unsigned short value;
    XcmsFloat intensity;
} IntensityRec;

    /*
     * Intensity Table
     */
typedef struct _IntensityTbl {
    IntensityRec *pBase;
    unsigned int nEntries;
} IntensityTbl;

    /*
     * Structure for storing per-Visual Intensity Tables (aka gamma maps).
     */
typedef struct _XcmsIntensityMap {
    VisualID visualID;
    XPointer	screenData;	/* pointer to corresponding Screen Color*/
				/*	Characterization Data		*/
    void (*pFreeScreenData)(XPointer pScreenDataTemp);	/* Function that frees a Screen		*/
				/*   structure.				*/
    struct _XcmsIntensityMap *pNext;
} XcmsIntensityMap;


    /*
     * Structure for storing "registered" color space prefix/ID
     */
typedef struct _XcmsRegColorSpaceEntry {
    const char *prefix;	/* Color Space prefix (e.g., "CIEXYZ:") */
    XcmsColorFormat id;	/* Color Space ID (e.g., XcmsCIEXYZFormat) */
} XcmsRegColorSpaceEntry;


    /*
     * Xcms Per Display (i.e. connection) related data
     */
typedef struct _XcmsPerDpyInfo {

    XcmsCCC paDefaultCCC; /* based on default visual of screen */
	    /*
	     * Pointer to an array of XcmsCCC structures, one for
	     * each screen.
	     */
    XcmsCmapRec *pClientCmaps;	/* Pointer to linked list of XcmsCmapRec's */

} XcmsPerDpyInfo, *XcmsPerDpyInfoPtr;

/*
 *	DEFINES
 */

#define XDCCC_NUMBER	0x8000000L	/* 2**27 per XDCCC */

#ifdef GRAY
#define XDCCC_SCREENWHITEPT_ATOM_NAME	"XDCCC_GRAY_SCREENWHITEPOINT"
#define XDCCC_GRAY_CORRECT_ATOM_NAME	"XDCCC_GRAY_CORRECTION"
#endif /* GRAY */

#ifndef _ConversionValues
typedef struct _ConversionValues {
    IntensityTbl IntensityTbl;
} ConversionValues;
#endif

#ifdef GRAY
typedef struct {
    IntensityTbl *IntensityTbl;
} GRAY_SCCData;
#endif /* GRAY */

/*
 *	DEFINES
 */

#define XDCCC_MATRIX_ATOM_NAME	"XDCCC_LINEAR_RGB_MATRICES"
#define XDCCC_CORRECT_ATOM_NAME "XDCCC_LINEAR_RGB_CORRECTION"

typedef struct {
    XcmsFloat XYZtoRGBmatrix[3][3];
    XcmsFloat RGBtoXYZmatrix[3][3];
    IntensityTbl *pRedTbl;
    IntensityTbl *pGreenTbl;
    IntensityTbl *pBlueTbl;
} LINEAR_RGB_SCCData;

/* function prototypes */
extern XcmsCmapRec *
_XcmsAddCmapRec(
    Display *dpy,
    Colormap cmap,
    Window windowID,
    Visual *visual);
extern void
_XcmsRGB_to_XColor(
    XcmsColor *pColors,
    XColor *pXColors,
    unsigned int nColors);
extern Status
_XcmsResolveColorString (
    XcmsCCC ccc,
    const char **color_string,
    XcmsColor *pColor_exact_return,
    XcmsColorFormat result_format);
extern void
_XUnresolveColor(
    XcmsCCC ccc,
    XColor *pXColor);
/*
 *	DESCRIPTION
 *		Include file for defining the math macros used in the
 *		XCMS source.  Instead of using math library routines
 *		directly, XCMS uses macros so that based on the
 *		definitions here, vendors and sites can specify exactly
 *		what routine will be called (those from libm.a or their
 *		custom routines).  If not defined to math library routines
 *		(e.g., sqrt in libm.a), then the client is not forced to
 *		be linked with -lm.
 */

#define XCMS_ATAN(x)		_XcmsArcTangent(x)
#define XCMS_COS(x)		_XcmsCosine(x)
#define XCMS_CUBEROOT(x)	_XcmsCubeRoot(x)
#define XCMS_FABS(x)		((x) < 0.0 ? -(x) : (x))
#define XCMS_SIN(x)		_XcmsSine(x)
#define XCMS_SQRT(x)		_XcmsSquareRoot(x)
#define XCMS_TAN(x)		(XCMS_SIN(x) / XCMS_COS(x))

double _XcmsArcTangent(double a);
double _XcmsCosine(double a);
double _XcmsCubeRoot(double a);
double _XcmsSine(double a);
double _XcmsSquareRoot(double a);

/*
 *  DEFINES FOR GAMUT COMPRESSION AND QUERY ROUTINES
 */
#ifndef PI
#  ifdef M_PI
#    define PI M_PI
#  else
#    define PI 3.14159265358979323846264338327950
#  endif /* M_PI */
#endif /* PI */
#ifndef degrees
#  define degrees(r) ((XcmsFloat)(r) * 180.0 / PI)
#endif /* degrees */
#ifndef radians
#  define radians(d) ((XcmsFloat)(d) * PI / 180.0)
#endif /* radians */

#define XCMS_CIEUSTAROFHUE(h,c)	\
((XCMS_COS((h)) == 0.0) ? (XcmsFloat)0.0 : (XcmsFloat) \
((XcmsFloat)(c) / (XcmsFloat)XCMS_SQRT((XCMS_TAN(h) * XCMS_TAN(h)) + \
(XcmsFloat)1.0)))
#define XCMS_CIEVSTAROFHUE(h,c)	\
((XCMS_COS((h)) == 0.0) ? (XcmsFloat)0.0 : (XcmsFloat) \
((XcmsFloat)(c) / (XcmsFloat)XCMS_SQRT(((XcmsFloat)1.0 / \
(XcmsFloat)(XCMS_TAN(h) * XCMS_TAN(h))) + (XcmsFloat)1.0)))
/* this hue is returned in radians */
#define XCMS_CIELUV_PMETRIC_HUE(u,v)	\
(((u) != 0.0) ? XCMS_ATAN( (v) / (u)) : ((v >= 0.0) ? PI / 2 : -(PI / 2)))
#define XCMS_CIELUV_PMETRIC_CHROMA(u,v)	XCMS_SQRT(((u)*(u)) + ((v)*(v)))

#define XCMS_CIEASTAROFHUE(h,c)		XCMS_CIEUSTAROFHUE((h), (c))
#define XCMS_CIEBSTAROFHUE(h,c)		XCMS_CIEVSTAROFHUE((h), (c))
#define XCMS_CIELAB_PMETRIC_HUE(a,b)	XCMS_CIELUV_PMETRIC_HUE((a), (b))
#define XCMS_CIELAB_PMETRIC_CHROMA(a,b)	XCMS_CIELUV_PMETRIC_CHROMA((a), (b))

#endif /* _XCMSINT_H_ */
