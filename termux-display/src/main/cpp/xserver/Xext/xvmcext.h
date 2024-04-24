
#ifndef _XVMC_H
#define _XVMC_H
#include <X11/extensions/Xv.h>
#include "xvdix.h"

typedef struct {
    int num_xvimages;
    int *xvimage_ids;
} XvMCImageIDList;

typedef struct {
    int surface_type_id;
    int chroma_format;
    int color_description;
    unsigned short max_width;
    unsigned short max_height;
    unsigned short subpicture_max_width;
    unsigned short subpicture_max_height;
    int mc_type;
    int flags;
    XvMCImageIDList *compatible_subpictures;
} XvMCSurfaceInfoRec, *XvMCSurfaceInfoPtr;

typedef struct {
    XID context_id;
    ScreenPtr pScreen;
    int adapt_num;
    int surface_type_id;
    unsigned short width;
    unsigned short height;
    CARD32 flags;
    int refcnt;
    void *port_priv;
    void *driver_priv;
} XvMCContextRec, *XvMCContextPtr;

typedef struct {
    XID surface_id;
    int surface_type_id;
    XvMCContextPtr context;
    void *driver_priv;
} XvMCSurfaceRec, *XvMCSurfacePtr;

typedef struct {
    XID subpicture_id;
    int xvimage_id;
    unsigned short width;
    unsigned short height;
    int num_palette_entries;
    int entry_bytes;
    char component_order[4];
    XvMCContextPtr context;
    void *driver_priv;
} XvMCSubpictureRec, *XvMCSubpicturePtr;

typedef int (*XvMCCreateContextProcPtr) (XvPortPtr port,
                                         XvMCContextPtr context,
                                         int *num_priv, CARD32 **priv);

typedef void (*XvMCDestroyContextProcPtr) (XvMCContextPtr context);

typedef int (*XvMCCreateSurfaceProcPtr) (XvMCSurfacePtr surface,
                                         int *num_priv, CARD32 **priv);

typedef void (*XvMCDestroySurfaceProcPtr) (XvMCSurfacePtr surface);

typedef int (*XvMCCreateSubpictureProcPtr) (XvMCSubpicturePtr subpicture,
                                            int *num_priv, CARD32 **priv);

typedef void (*XvMCDestroySubpictureProcPtr) (XvMCSubpicturePtr subpicture);

typedef struct {
    XvAdaptorPtr xv_adaptor;
    int num_surfaces;
    XvMCSurfaceInfoPtr *surfaces;
    int num_subpictures;
    XvImagePtr *subpictures;
    XvMCCreateContextProcPtr CreateContext;
    XvMCDestroyContextProcPtr DestroyContext;
    XvMCCreateSurfaceProcPtr CreateSurface;
    XvMCDestroySurfaceProcPtr DestroySurface;
    XvMCCreateSubpictureProcPtr CreateSubpicture;
    XvMCDestroySubpictureProcPtr DestroySubpicture;
} XvMCAdaptorRec, *XvMCAdaptorPtr;

extern int (*XvMCScreenInitProc)(ScreenPtr, int, XvMCAdaptorPtr);

extern _X_EXPORT int XvMCScreenInit(ScreenPtr pScreen,
                                    int num, XvMCAdaptorPtr adapt);

extern _X_EXPORT XvImagePtr XvMCFindXvImage(XvPortPtr pPort, CARD32 id);

extern _X_EXPORT int xf86XvMCRegisterDRInfo(ScreenPtr pScreen, const char *name,
                                            const char *busID, int major, int minor,
                                            int patchLevel);

#endif                          /* _XVMC_H */
