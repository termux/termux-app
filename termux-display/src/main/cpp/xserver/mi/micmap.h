
#include "colormapst.h"

#ifndef _MICMAP_H_
#define _MICMAP_H_

#define GetInstalledmiColormap(s) \
    ((ColormapPtr) dixLookupPrivate(&(s)->devPrivates, micmapScrPrivateKey))
#define SetInstalledmiColormap(s,c) \
    (dixSetPrivate(&(s)->devPrivates, micmapScrPrivateKey, c))

extern _X_EXPORT DevPrivateKeyRec micmapScrPrivateKeyRec;

#define micmapScrPrivateKey (&micmapScrPrivateKeyRec)

typedef Bool (*miInitVisualsProcPtr) (VisualPtr *, DepthPtr *, int *, int *,
                                      int *, VisualID *, unsigned long, int,
                                      int);

extern _X_EXPORT int miListInstalledColormaps(ScreenPtr pScreen,
                                              Colormap * pmaps);
extern _X_EXPORT void miInstallColormap(ColormapPtr pmap);
extern _X_EXPORT void miUninstallColormap(ColormapPtr pmap);

extern _X_EXPORT void miResolveColor(unsigned short *, unsigned short *,
                                     unsigned short *, VisualPtr);
extern _X_EXPORT Bool miInitializeColormap(ColormapPtr);
extern _X_EXPORT int miExpandDirectColors(ColormapPtr, int, xColorItem *,
                                          xColorItem *);
extern _X_EXPORT Bool miCreateDefColormap(ScreenPtr);
extern _X_EXPORT void miClearVisualTypes(void);
extern _X_EXPORT Bool miSetVisualTypes(int, int, int, int);
extern _X_EXPORT Bool miSetPixmapDepths(void);
extern _X_EXPORT Bool miSetVisualTypesAndMasks(int depth, int visuals,
                                               int bitsPerRGB, int preferredCVC,
                                               Pixel redMask, Pixel greenMask,
                                               Pixel blueMask);
extern _X_EXPORT int miGetDefaultVisualMask(int);
extern _X_EXPORT Bool miInitVisuals(VisualPtr *, DepthPtr *, int *, int *,
                                    int *, VisualID *, unsigned long, int, int);

#define MAX_PSEUDO_DEPTH	10
#define MIN_TRUE_DEPTH		6

#define StaticGrayMask	(1 << StaticGray)
#define GrayScaleMask	(1 << GrayScale)
#define StaticColorMask	(1 << StaticColor)
#define PseudoColorMask	(1 << PseudoColor)
#define TrueColorMask	(1 << TrueColor)
#define DirectColorMask	(1 << DirectColor)

#define ALL_VISUALS	(StaticGrayMask|\
			 GrayScaleMask|\
			 StaticColorMask|\
			 PseudoColorMask|\
			 TrueColorMask|\
			 DirectColorMask)

#define LARGE_VISUALS	(TrueColorMask|\
			 DirectColorMask)

#define SMALL_VISUALS	(StaticGrayMask|\
			 GrayScaleMask|\
			 StaticColorMask|\
			 PseudoColorMask)

#endif                          /* _MICMAP_H_ */
