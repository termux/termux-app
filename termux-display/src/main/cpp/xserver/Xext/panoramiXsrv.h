
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _PANORAMIXSRV_H_
#define _PANORAMIXSRV_H_

#include "panoramiX.h"

extern _X_EXPORT int PanoramiXNumScreens;
extern _X_EXPORT int PanoramiXPixWidth;
extern _X_EXPORT int PanoramiXPixHeight;
extern _X_EXPORT RegionRec PanoramiXScreenRegion;

extern _X_EXPORT VisualID PanoramiXTranslateVisualID(int screen, VisualID orig);
extern _X_EXPORT void PanoramiXConsolidate(void);
extern _X_EXPORT Bool PanoramiXCreateConnectionBlock(void);
extern _X_EXPORT PanoramiXRes *PanoramiXFindIDByScrnum(RESTYPE, XID, int);
extern _X_EXPORT Bool
XineramaRegisterConnectionBlockCallback(void (*func) (void));
extern _X_EXPORT int XineramaDeleteResource(void *, XID);

extern _X_EXPORT void XineramaReinitData(void);

extern _X_EXPORT RESTYPE XRC_DRAWABLE;
extern _X_EXPORT RESTYPE XRT_WINDOW;
extern _X_EXPORT RESTYPE XRT_PIXMAP;
extern _X_EXPORT RESTYPE XRT_GC;
extern _X_EXPORT RESTYPE XRT_COLORMAP;
extern _X_EXPORT RESTYPE XRT_PICTURE;

/*
 * Drivers are allowed to wrap this function.  Each wrapper can decide that the
 * two visuals are unequal, but if they are deemed equal, the wrapper must call
 * down and return FALSE if the wrapped function does.  This ensures that all
 * layers agree that the visuals are equal.  The first visual is always from
 * screen 0.
 */
typedef Bool (*XineramaVisualsEqualProcPtr) (VisualPtr, ScreenPtr, VisualPtr);
extern _X_EXPORT XineramaVisualsEqualProcPtr XineramaVisualsEqualPtr;

extern _X_EXPORT void XineramaGetImageData(DrawablePtr *pDrawables,
                                           int left,
                                           int top,
                                           int width,
                                           int height,
                                           unsigned int format,
                                           unsigned long planemask,
                                           char *data, int pitch, Bool isRoot);

static inline void
panoramix_setup_ids(PanoramiXRes * resource, ClientPtr client, XID base_id)
{
    int j;

    resource->info[0].id = base_id;
    FOR_NSCREENS_FORWARD_SKIP(j) {
        resource->info[j].id = FakeClientID(client->index);
    }
}

#endif                          /* _PANORAMIXSRV_H_ */
