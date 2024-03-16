
#ifndef _CV_H_
#define _CV_H_

/* variables */
extern const char      _XcmsCIEXYZ_prefix[];
extern const char      _XcmsCIEuvY_prefix[];
extern const char      _XcmsCIExyY_prefix[];
extern const char      _XcmsCIELab_prefix[];
extern const char      _XcmsCIELuv_prefix[];
extern const char      _XcmsTekHVC_prefix[];
extern const char      _XcmsRGBi_prefix[];
extern const char      _XcmsRGB_prefix[];

extern XcmsColorSpace  XcmsUNDEFINEDColorSpace;
extern XcmsColorSpace  XcmsTekHVCColorSpace;
extern XcmsColorSpace  XcmsCIEXYZColorSpace;
extern XcmsColorSpace  XcmsCIEuvYColorSpace;
extern XcmsColorSpace  XcmsCIExyYColorSpace;
extern XcmsColorSpace  XcmsCIELabColorSpace;
extern XcmsColorSpace  XcmsCIELuvColorSpace;
extern XcmsColorSpace  XcmsRGBColorSpace;
extern XcmsColorSpace  XcmsRGBiColorSpace;

extern XcmsColorSpace  *_XcmsDIColorSpacesInit[];
extern XcmsColorSpace  **_XcmsDIColorSpaces;

extern XcmsColorSpace  *_XcmsDDColorSpacesInit[];
extern XcmsColorSpace  **_XcmsDDColorSpaces;

extern XcmsFunctionSet XcmsLinearRGBFunctionSet;

extern XcmsFunctionSet *_XcmsSCCFuncSetsInit[];
extern XcmsFunctionSet **_XcmsSCCFuncSets;

extern XcmsRegColorSpaceEntry _XcmsRegColorSpaces[];

/* functions */
extern XPointer *
_XcmsCopyPointerArray(
    XPointer *pap);
extern void
_XcmsFreePointerArray(
    XPointer *pap);
extern XPointer *
_XcmsPushPointerArray(
    XPointer *pap,
    XPointer p,
    XPointer *papNoFree);
extern Status
_XcmsCIEXYZ_ValidSpec(
    XcmsColor *pColor);
extern Status
_XcmsCIEuvY_ValidSpec(
    XcmsColor *pColor);
extern int
_XcmsTekHVC_CheckModify(
    XcmsColor *pColor);

extern Status
_XcmsTekHVCQueryMaxVCRGB(
    XcmsCCC     ccc,
    XcmsFloat   hue,
    XcmsColor   *pColor_return,
    XcmsRGBi    *pRGB_return);
extern Status
_XcmsCIELabQueryMaxLCRGB(
    XcmsCCC     ccc,
    XcmsFloat   hue,                /* hue in radians */
    XcmsColor   *pColor_return,
    XcmsRGBi    *pRGB_return);
extern Status
_XcmsConvertColorsWithWhitePt(
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    XcmsColor *pWhitePt,
    unsigned int nColors,
    XcmsColorFormat newFormat,
    Bool *pCompressed);

extern Status
_XcmsDIConvertColors(
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    XcmsColor *pWhitePt,
    unsigned int nColors,
    XcmsColorFormat newFormat);
extern Status
_XcmsDDConvertColors(
    XcmsCCC ccc,
    XcmsColor *pColors_in_out,
    unsigned int nColors,
    XcmsColorFormat newFormat,
    Bool *pCompressed);
extern XcmsColorFormat
_XcmsRegFormatOfPrefix(
    _Xconst char *prefix);
extern void
_XColor_to_XcmsRGB(
    XcmsCCC ccc,
    XColor *pXColors,
    XcmsColor *pColors,
    unsigned int nColors);
extern Status
_XcmsSetGetColor(
    Status (*xColorProc)(
        Display*            /* display */,
        Colormap            /* colormap */,
        XColor*             /* screen_in_out */),
    Display *dpy,
    Colormap cmap,
    XcmsColor *pColors_in_out,
    XcmsColorFormat result_format,
    Bool *pCompressed);
extern Status
_XcmsSetGetColors(
    Status (*xColorProc)(
        Display*            /* display */,
        Colormap            /* colormap */,
        XColor*             /* screen_in_out */,
        int                 /* nColors */),
    Display *dpy,
    Colormap cmap,
    XcmsColor *pColors_in_out,
    int nColors,
    XcmsColorFormat result_format,
    Bool *pCompressed);
extern Status
_XcmsCIELuvQueryMaxLCRGB(
    XcmsCCC     ccc,
    XcmsFloat   hue,            /* hue in radians */
    XcmsColor   *pColor_return,
    XcmsRGBi    *pRGB_return);

extern XcmsIntensityMap *
_XcmsGetIntensityMap(
    Display *dpy,
    Visual *visual);
extern int
_XcmsInitDefaultCCCs(
    Display *dpy);
extern int
_XcmsInitScrnInfo(
    register Display *dpy,
    int screenNumber);
extern XcmsCmapRec *
_XcmsCopyCmapRecAndFree(
    Display *dpy,
    Colormap src_cmap,
    Colormap copy_cmap);
extern void
_XcmsCopyISOLatin1Lowered(
    char *dst,
    const char *src);
extern int
_XcmsEqualWhitePts(
    XcmsCCC ccc, XcmsColor *pWhitePt1, XcmsColor *pWhitePt2);
extern int
_XcmsLRGB_InitScrnDefault(
    Display *dpy,
    int screenNumber,
    XcmsPerScrnInfo *pPerScrnInfo);
extern void
_XcmsFreeIntensityMaps(
    Display *dpy);
extern int
_XcmsGetProperty(
    Display *pDpy,
    Window  w,
    Atom property,
    int             *pFormat,
    unsigned long   *pNItems,
    unsigned long   *pNBytes,
    char            **pValue);
extern unsigned long
_XcmsGetElement(
    int             format,
    char            **pValue,
    unsigned long   *pCount);
extern void
_XcmsUnresolveColor(
    XcmsCCC ccc,
    XcmsColor *pColor);
extern void
_XcmsResolveColor(
    XcmsCCC ccc,
    XcmsColor *pXcmsColor);

#endif /* _CV_H_ */
