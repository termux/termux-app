/*
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifndef _PICTURESTR_H_
#define _PICTURESTR_H_

#include "scrnintstr.h"
#include "glyphstr.h"
#include "resource.h"
#include "privates.h"

typedef struct _DirectFormat {
    CARD16 red, redMask;
    CARD16 green, greenMask;
    CARD16 blue, blueMask;
    CARD16 alpha, alphaMask;
} DirectFormatRec;

typedef struct _IndexFormat {
    VisualID vid;
    ColormapPtr pColormap;
    int nvalues;
    xIndexValue *pValues;
    void *devPrivate;
} IndexFormatRec;

typedef struct _PictFormat {
    CARD32 id;
    CARD32 format;              /* except bpp */
    unsigned char type;
    unsigned char depth;
    DirectFormatRec direct;
    IndexFormatRec index;
} PictFormatRec;

typedef struct pixman_vector PictVector, *PictVectorPtr;
typedef struct pixman_transform PictTransform, *PictTransformPtr;

#define pict_f_vector pixman_f_vector
#define pict_f_transform pixman_f_transform

#define PICT_GRADIENT_STOPTABLE_SIZE 1024
#define SourcePictTypeSolidFill 0
#define SourcePictTypeLinear 1
#define SourcePictTypeRadial 2
#define SourcePictTypeConical 3

typedef struct _PictSolidFill {
    unsigned int type;
    CARD32 color;
    xRenderColor fullcolor;
} PictSolidFill, *PictSolidFillPtr;

typedef struct _PictGradientStop {
    xFixed x;
    xRenderColor color;
} PictGradientStop, *PictGradientStopPtr;

typedef struct _PictGradient {
    unsigned int type;
    int nstops;
    PictGradientStopPtr stops;
} PictGradient, *PictGradientPtr;

typedef struct _PictLinearGradient {
    unsigned int type;
    int nstops;
    PictGradientStopPtr stops;
    xPointFixed p1;
    xPointFixed p2;
} PictLinearGradient, *PictLinearGradientPtr;

typedef struct _PictCircle {
    xFixed x;
    xFixed y;
    xFixed radius;
} PictCircle, *PictCirclePtr;

typedef struct _PictRadialGradient {
    unsigned int type;
    int nstops;
    PictGradientStopPtr stops;
    PictCircle c1;
    PictCircle c2;
} PictRadialGradient, *PictRadialGradientPtr;

typedef struct _PictConicalGradient {
    unsigned int type;
    int nstops;
    PictGradientStopPtr stops;
    xPointFixed center;
    xFixed angle;
} PictConicalGradient, *PictConicalGradientPtr;

typedef union _SourcePict {
    unsigned int type;
    PictSolidFill solidFill;
    PictGradient gradient;
    PictLinearGradient linear;
    PictRadialGradient radial;
    PictConicalGradient conical;
} SourcePict, *SourcePictPtr;

typedef struct _Picture {
    DrawablePtr pDrawable;
    PictFormatPtr pFormat;
    PictFormatShort format;     /* PICT_FORMAT */
    int refcnt;
    CARD32 id;
    unsigned int repeat:1;
    unsigned int graphicsExposures:1;
    unsigned int subWindowMode:1;
    unsigned int polyEdge:1;
    unsigned int polyMode:1;
    unsigned int freeCompClip:1;
    unsigned int componentAlpha:1;
    unsigned int repeatType:2;
    unsigned int filter:3;
    unsigned int stateChanges:CPLastBit;
    unsigned int unused:18 - CPLastBit;

    PicturePtr pNext;           /* chain on same drawable */

    PicturePtr alphaMap;
    DDXPointRec alphaOrigin;

    DDXPointRec clipOrigin;
    RegionPtr clientClip;

    unsigned long serialNumber;

    RegionPtr pCompositeClip;

    PrivateRec *devPrivates;

    PictTransform *transform;

    SourcePictPtr pSourcePict;
    xFixed *filter_params;
    int filter_nparams;
} PictureRec;

typedef Bool (*PictFilterValidateParamsProcPtr) (ScreenPtr pScreen, int id,
                                                 xFixed * params, int nparams,
                                                 int *width, int *height);
typedef struct {
    char *name;
    int id;
    PictFilterValidateParamsProcPtr ValidateParams;
    int width, height;
} PictFilterRec, *PictFilterPtr;

#define PictFilterNearest	0
#define PictFilterBilinear	1

#define PictFilterFast		2
#define PictFilterGood		3
#define PictFilterBest		4

#define PictFilterConvolution	5
/* if you add an 8th filter, expand the filter bitfield above */

typedef struct {
    char *alias;
    int alias_id;
    int filter_id;
} PictFilterAliasRec, *PictFilterAliasPtr;

typedef int (*CreatePictureProcPtr) (PicturePtr pPicture);
typedef void (*DestroyPictureProcPtr) (PicturePtr pPicture);
typedef int (*ChangePictureClipProcPtr) (PicturePtr pPicture,
                                         int clipType, void *value, int n);
typedef void (*DestroyPictureClipProcPtr) (PicturePtr pPicture);

typedef int (*ChangePictureTransformProcPtr) (PicturePtr pPicture,
                                              PictTransform * transform);

typedef int (*ChangePictureFilterProcPtr) (PicturePtr pPicture,
                                           int filter,
                                           xFixed * params, int nparams);

typedef void (*DestroyPictureFilterProcPtr) (PicturePtr pPicture);

typedef void (*ChangePictureProcPtr) (PicturePtr pPicture, Mask mask);
typedef void (*ValidatePictureProcPtr) (PicturePtr pPicture, Mask mask);
typedef void (*CompositeProcPtr) (CARD8 op,
                                  PicturePtr pSrc,
                                  PicturePtr pMask,
                                  PicturePtr pDst,
                                  INT16 xSrc,
                                  INT16 ySrc,
                                  INT16 xMask,
                                  INT16 yMask,
                                  INT16 xDst,
                                  INT16 yDst, CARD16 width, CARD16 height);

typedef void (*GlyphsProcPtr) (CARD8 op,
                               PicturePtr pSrc,
                               PicturePtr pDst,
                               PictFormatPtr maskFormat,
                               INT16 xSrc,
                               INT16 ySrc,
                               int nlists,
                               GlyphListPtr lists, GlyphPtr * glyphs);

typedef void (*CompositeRectsProcPtr) (CARD8 op,
                                       PicturePtr pDst,
                                       xRenderColor * color,
                                       int nRect, xRectangle *rects);

typedef void (*RasterizeTrapezoidProcPtr) (PicturePtr pMask,
                                           xTrapezoid * trap,
                                           int x_off, int y_off);

typedef void (*TrapezoidsProcPtr) (CARD8 op,
                                   PicturePtr pSrc,
                                   PicturePtr pDst,
                                   PictFormatPtr maskFormat,
                                   INT16 xSrc,
                                   INT16 ySrc, int ntrap, xTrapezoid * traps);

typedef void (*TrianglesProcPtr) (CARD8 op,
                                  PicturePtr pSrc,
                                  PicturePtr pDst,
                                  PictFormatPtr maskFormat,
                                  INT16 xSrc,
                                  INT16 ySrc, int ntri, xTriangle * tris);

typedef void (*TriStripProcPtr) (CARD8 op,
                                 PicturePtr pSrc,
                                 PicturePtr pDst,
                                 PictFormatPtr maskFormat,
                                 INT16 xSrc,
                                 INT16 ySrc, int npoint, xPointFixed * points);

typedef void (*TriFanProcPtr) (CARD8 op,
                               PicturePtr pSrc,
                               PicturePtr pDst,
                               PictFormatPtr maskFormat,
                               INT16 xSrc,
                               INT16 ySrc, int npoint, xPointFixed * points);

typedef Bool (*InitIndexedProcPtr) (ScreenPtr pScreen, PictFormatPtr pFormat);

typedef void (*CloseIndexedProcPtr) (ScreenPtr pScreen, PictFormatPtr pFormat);

typedef void (*UpdateIndexedProcPtr) (ScreenPtr pScreen,
                                      PictFormatPtr pFormat,
                                      int ndef, xColorItem * pdef);

typedef void (*AddTrapsProcPtr) (PicturePtr pPicture,
                                 INT16 xOff,
                                 INT16 yOff, int ntrap, xTrap * traps);

typedef void (*AddTrianglesProcPtr) (PicturePtr pPicture,
                                     INT16 xOff,
                                     INT16 yOff, int ntri, xTriangle * tris);

typedef Bool (*RealizeGlyphProcPtr) (ScreenPtr pScreen, GlyphPtr glyph);

typedef void (*UnrealizeGlyphProcPtr) (ScreenPtr pScreen, GlyphPtr glyph);

typedef struct _PictureScreen {
    PictFormatPtr formats;
    PictFormatPtr fallback;
    int nformats;

    CreatePictureProcPtr CreatePicture;
    DestroyPictureProcPtr DestroyPicture;
    ChangePictureClipProcPtr ChangePictureClip;
    DestroyPictureClipProcPtr DestroyPictureClip;

    ChangePictureProcPtr ChangePicture;
    ValidatePictureProcPtr ValidatePicture;

    CompositeProcPtr Composite;
    GlyphsProcPtr Glyphs;       /* unused */
    CompositeRectsProcPtr CompositeRects;

    DestroyWindowProcPtr DestroyWindow;
    CloseScreenProcPtr CloseScreen;

    StoreColorsProcPtr StoreColors;

    InitIndexedProcPtr InitIndexed;
    CloseIndexedProcPtr CloseIndexed;
    UpdateIndexedProcPtr UpdateIndexed;

    int subpixel;

    PictFilterPtr filters;
    int nfilters;
    PictFilterAliasPtr filterAliases;
    int nfilterAliases;

    /**
     * Called immediately after a picture's transform is changed through the
     * SetPictureTransform request.  Not called for source-only pictures.
     */
    ChangePictureTransformProcPtr ChangePictureTransform;

    /**
     * Called immediately after a picture's transform is changed through the
     * SetPictureFilter request.  Not called for source-only pictures.
     */
    ChangePictureFilterProcPtr ChangePictureFilter;

    DestroyPictureFilterProcPtr DestroyPictureFilter;

    TrapezoidsProcPtr Trapezoids;
    TrianglesProcPtr Triangles;

    RasterizeTrapezoidProcPtr RasterizeTrapezoid;

    AddTrianglesProcPtr AddTriangles;

    AddTrapsProcPtr AddTraps;

    RealizeGlyphProcPtr RealizeGlyph;
    UnrealizeGlyphProcPtr UnrealizeGlyph;

#define PICTURE_SCREEN_VERSION 2
    TriStripProcPtr TriStrip;
    TriFanProcPtr TriFan;
} PictureScreenRec, *PictureScreenPtr;

extern _X_EXPORT DevPrivateKeyRec PictureScreenPrivateKeyRec;
#define PictureScreenPrivateKey (&PictureScreenPrivateKeyRec)

extern _X_EXPORT DevPrivateKeyRec PictureWindowPrivateKeyRec;
#define	PictureWindowPrivateKey (&PictureWindowPrivateKeyRec)

extern RESTYPE PictureType;
extern RESTYPE PictFormatType;
extern RESTYPE GlyphSetType;

#define GetPictureScreen(s) ((PictureScreenPtr)dixLookupPrivate(&(s)->devPrivates, PictureScreenPrivateKey))
#define GetPictureScreenIfSet(s) (dixPrivateKeyRegistered(PictureScreenPrivateKey) ? GetPictureScreen(s) : NULL)
#define SetPictureScreen(s,p) dixSetPrivate(&(s)->devPrivates, PictureScreenPrivateKey, p)
#define GetPictureWindow(w) ((PicturePtr)dixLookupPrivate(&(w)->devPrivates, PictureWindowPrivateKey))
#define SetPictureWindow(w,p) dixSetPrivate(&(w)->devPrivates, PictureWindowPrivateKey, p)

#define VERIFY_PICTURE(pPicture, pid, client, mode) {\
    int tmprc = dixLookupResourceByType((void *)&(pPicture), pid,\
	                                PictureType, client, mode);\
    if (tmprc != Success)\
	return tmprc;\
}

#define VERIFY_ALPHA(pPicture, pid, client, mode) {\
    if (pid == None) \
	pPicture = 0; \
    else { \
	VERIFY_PICTURE(pPicture, pid, client, mode); \
    } \
} \

extern _X_EXPORT PictFormatPtr
 PictureWindowFormat(WindowPtr pWindow);

extern _X_EXPORT Bool
 PictureSetSubpixelOrder(ScreenPtr pScreen, int subpixel);

extern _X_EXPORT int
 PictureGetSubpixelOrder(ScreenPtr pScreen);

extern _X_EXPORT PictFormatPtr
PictureMatchVisual(ScreenPtr pScreen, int depth, VisualPtr pVisual);

extern _X_EXPORT PictFormatPtr
PictureMatchFormat(ScreenPtr pScreen, int depth, CARD32 format);

extern _X_EXPORT Bool
 PictureInit(ScreenPtr pScreen, PictFormatPtr formats, int nformats);

extern _X_EXPORT int
 PictureGetFilterId(const char *filter, int len, Bool makeit);

extern _X_EXPORT char *PictureGetFilterName(int id);

extern _X_EXPORT int
PictureAddFilter(ScreenPtr pScreen,
                 const char *filter,
                 PictFilterValidateParamsProcPtr ValidateParams,
                 int width, int height);

extern _X_EXPORT Bool
PictureSetFilterAlias(ScreenPtr pScreen, const char *filter, const char *alias);

extern _X_EXPORT Bool
 PictureSetDefaultFilters(ScreenPtr pScreen);

extern _X_EXPORT void
 PictureResetFilters(ScreenPtr pScreen);

extern _X_EXPORT PictFilterPtr
PictureFindFilter(ScreenPtr pScreen, char *name, int len);

extern _X_EXPORT int
SetPicturePictFilter(PicturePtr pPicture, PictFilterPtr pFilter,
                     xFixed * params, int nparams);

extern _X_EXPORT int
SetPictureFilter(PicturePtr pPicture, char *name, int len,
                 xFixed * params, int nparams);

extern _X_EXPORT Bool
 PictureFinishInit(void);

extern _X_EXPORT PicturePtr
CreatePicture(Picture pid,
              DrawablePtr pDrawable,
              PictFormatPtr pFormat,
              Mask mask, XID *list, ClientPtr client, int *error);

extern _X_EXPORT int
ChangePicture(PicturePtr pPicture,
              Mask vmask, XID *vlist, DevUnion *ulist, ClientPtr client);

extern _X_EXPORT int

SetPictureClipRects(PicturePtr pPicture,
                    int xOrigin, int yOrigin, int nRect, xRectangle *rects);

extern _X_EXPORT int
SetPictureClipRegion(PicturePtr pPicture,
                     int xOrigin, int yOrigin, RegionPtr pRegion);

extern _X_EXPORT int
 SetPictureTransform(PicturePtr pPicture, PictTransform * transform);

extern _X_EXPORT void
 ValidatePicture(PicturePtr pPicture);

extern _X_EXPORT int
 FreePicture(void *pPicture, XID pid);

extern _X_EXPORT void
CompositePicture(CARD8 op,
                 PicturePtr pSrc,
                 PicturePtr pMask,
                 PicturePtr pDst,
                 INT16 xSrc,
                 INT16 ySrc,
                 INT16 xMask,
                 INT16 yMask,
                 INT16 xDst, INT16 yDst, CARD16 width, CARD16 height);

extern _X_EXPORT void
CompositeGlyphs(CARD8 op,
                PicturePtr pSrc,
                PicturePtr pDst,
                PictFormatPtr maskFormat,
                INT16 xSrc,
                INT16 ySrc, int nlist, GlyphListPtr lists, GlyphPtr * glyphs);

extern _X_EXPORT void
CompositeRects(CARD8 op,
               PicturePtr pDst,
               xRenderColor * color, int nRect, xRectangle *rects);

extern _X_EXPORT void
CompositeTrapezoids(CARD8 op,
                    PicturePtr pSrc,
                    PicturePtr pDst,
                    PictFormatPtr maskFormat,
                    INT16 xSrc, INT16 ySrc, int ntrap, xTrapezoid * traps);

extern _X_EXPORT void
CompositeTriangles(CARD8 op,
                   PicturePtr pSrc,
                   PicturePtr pDst,
                   PictFormatPtr maskFormat,
                   INT16 xSrc,
                   INT16 ySrc, int ntriangles, xTriangle * triangles);

extern _X_EXPORT void
CompositeTriStrip(CARD8 op,
                  PicturePtr pSrc,
                  PicturePtr pDst,
                  PictFormatPtr maskFormat,
                  INT16 xSrc, INT16 ySrc, int npoints, xPointFixed * points);

extern _X_EXPORT void
CompositeTriFan(CARD8 op,
                PicturePtr pSrc,
                PicturePtr pDst,
                PictFormatPtr maskFormat,
                INT16 xSrc, INT16 ySrc, int npoints, xPointFixed * points);

Bool
 AnimCurInit(ScreenPtr pScreen);

int
AnimCursorCreate(CursorPtr *cursors, CARD32 *deltas, int ncursor,
                 CursorPtr *ppCursor, ClientPtr client, XID cid);

extern _X_EXPORT void
AddTraps(PicturePtr pPicture,
         INT16 xOff, INT16 yOff, int ntraps, xTrap * traps);

extern _X_EXPORT PicturePtr
CreateSolidPicture(Picture pid, xRenderColor * color, int *error);

extern _X_EXPORT PicturePtr
CreateLinearGradientPicture(Picture pid,
                            xPointFixed * p1,
                            xPointFixed * p2,
                            int nStops,
                            xFixed * stops, xRenderColor * colors, int *error);

extern _X_EXPORT PicturePtr
CreateRadialGradientPicture(Picture pid,
                            xPointFixed * inner,
                            xPointFixed * outer,
                            xFixed innerRadius,
                            xFixed outerRadius,
                            int nStops,
                            xFixed * stops, xRenderColor * colors, int *error);

extern _X_EXPORT PicturePtr
CreateConicalGradientPicture(Picture pid,
                             xPointFixed * center,
                             xFixed angle,
                             int nStops,
                             xFixed * stops, xRenderColor * colors, int *error);

#ifdef PANORAMIX
extern void PanoramiXRenderInit(void);
extern void PanoramiXRenderReset(void);
#endif

/*
 * matrix.c
 */

extern _X_EXPORT void
PictTransform_from_xRenderTransform(PictTransformPtr pict,
                                    xRenderTransform * render);

extern _X_EXPORT void
xRenderTransform_from_PictTransform(xRenderTransform * render,
                                    PictTransformPtr pict);

extern _X_EXPORT Bool
 PictureTransformPoint(PictTransformPtr transform, PictVectorPtr vector);

extern _X_EXPORT Bool
 PictureTransformPoint3d(PictTransformPtr transform, PictVectorPtr vector);

#endif                          /* _PICTURESTR_H_ */
