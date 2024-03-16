/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett-Packard Company
 * Copyright © 2006 Intel Corporation
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, Hewlett-Packard Company, Inc.
 *	    Keith Packard, Intel Corporation
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _RANDRSTR_H_
#define _RANDRSTR_H_

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "extnsionst.h"
#include "servermd.h"
#include "rrtransform.h"
#include <X11/extensions/randr.h>
#include <X11/extensions/randrproto.h>
#include <X11/extensions/render.h>      /* we share subpixel order information */
#include "picturestr.h"
#include <X11/Xfuncproto.h>

/* required for ABI compatibility for now */
#define RANDR_10_INTERFACE 1
#define RANDR_12_INTERFACE 1
#define RANDR_13_INTERFACE 1    /* requires RANDR_12_INTERFACE */
#define RANDR_GET_CRTC_INTERFACE 1

#define RANDR_INTERFACE_VERSION 0x0104

typedef XID RRMode;
typedef XID RROutput;
typedef XID RRCrtc;
typedef XID RRProvider;
typedef XID RRLease;

extern int RREventBase, RRErrorBase;

extern int (*ProcRandrVector[RRNumberRequests]) (ClientPtr);
extern int (*SProcRandrVector[RRNumberRequests]) (ClientPtr);

/*
 * Modeline for a monitor. Name follows directly after this struct
 */

#define RRModeName(pMode) ((char *) (pMode + 1))
typedef struct _rrMode RRModeRec, *RRModePtr;
typedef struct _rrPropertyValue RRPropertyValueRec, *RRPropertyValuePtr;
typedef struct _rrProperty RRPropertyRec, *RRPropertyPtr;
typedef struct _rrCrtc RRCrtcRec, *RRCrtcPtr;
typedef struct _rrOutput RROutputRec, *RROutputPtr;
typedef struct _rrProvider RRProviderRec, *RRProviderPtr;
typedef struct _rrMonitor RRMonitorRec, *RRMonitorPtr;
typedef struct _rrLease RRLeaseRec, *RRLeasePtr;

struct _rrMode {
    int refcnt;
    xRRModeInfo mode;
    char *name;
    ScreenPtr userScreen;
};

struct _rrPropertyValue {
    Atom type;                  /* ignored by server */
    short format;               /* format of data for swapping - 8,16,32 */
    long size;                  /* size of data in (format/8) bytes */
    void *data;                 /* private to client */
};

struct _rrProperty {
    RRPropertyPtr next;
    ATOM propertyName;
    Bool is_pending;
    Bool range;
    Bool immutable;
    int num_valid;
    INT32 *valid_values;
    RRPropertyValueRec current, pending;
};

struct _rrCrtc {
    RRCrtc id;
    ScreenPtr pScreen;
    RRModePtr mode;
    int x, y;
    Rotation rotation;
    Rotation rotations;
    Bool changed;
    int numOutputs;
    RROutputPtr *outputs;
    int gammaSize;
    CARD16 *gammaRed;
    CARD16 *gammaBlue;
    CARD16 *gammaGreen;
    void *devPrivate;
    Bool transforms;
    RRTransformRec client_pending_transform;
    RRTransformRec client_current_transform;
    PictTransform transform;
    struct pict_f_transform f_transform;
    struct pict_f_transform f_inverse;

    PixmapPtr scanout_pixmap;
    PixmapPtr scanout_pixmap_back;
};

struct _rrOutput {
    RROutput id;
    ScreenPtr pScreen;
    char *name;
    int nameLength;
    CARD8 connection;
    CARD8 subpixelOrder;
    int mmWidth;
    int mmHeight;
    RRCrtcPtr crtc;
    int numCrtcs;
    RRCrtcPtr *crtcs;
    int numClones;
    RROutputPtr *clones;
    int numModes;
    int numPreferred;
    RRModePtr *modes;
    int numUserModes;
    RRModePtr *userModes;
    Bool changed;
    Bool nonDesktop;
    RRPropertyPtr properties;
    Bool pendingProperties;
    void *devPrivate;
};

struct _rrProvider {
    RRProvider id;
    ScreenPtr pScreen;
    uint32_t capabilities;
    char *name;
    int nameLength;
    RRPropertyPtr properties;
    Bool pendingProperties;
    Bool changed;
    struct _rrProvider *offload_sink;
    struct _rrProvider *output_source;
};

typedef struct _rrMonitorGeometry {
    BoxRec box;
    CARD32 mmWidth;
    CARD32 mmHeight;
} RRMonitorGeometryRec, *RRMonitorGeometryPtr;

struct _rrMonitor {
    Atom name;
    ScreenPtr pScreen;
    int numOutputs;
    RROutput *outputs;
    Bool primary;
    Bool automatic;
    RRMonitorGeometryRec geometry;
};

typedef enum _rrLeaseState { RRLeaseCreating, RRLeaseRunning, RRLeaseTerminating } RRLeaseState;

struct _rrLease {
    struct xorg_list list;
    ScreenPtr screen;
    RRLease id;
    RRLeaseState state;
    void *devPrivate;
    int numCrtcs;
    RRCrtcPtr *crtcs;
    int numOutputs;
    RROutputPtr *outputs;
};

#if RANDR_12_INTERFACE
typedef Bool (*RRScreenSetSizeProcPtr) (ScreenPtr pScreen,
                                        CARD16 width,
                                        CARD16 height,
                                        CARD32 mmWidth, CARD32 mmHeight);

typedef Bool (*RRCrtcSetProcPtr) (ScreenPtr pScreen,
                                  RRCrtcPtr crtc,
                                  RRModePtr mode,
                                  int x,
                                  int y,
                                  Rotation rotation,
                                  int numOutputs, RROutputPtr * outputs);

typedef Bool (*RRCrtcSetGammaProcPtr) (ScreenPtr pScreen, RRCrtcPtr crtc);

typedef Bool (*RRCrtcGetGammaProcPtr) (ScreenPtr pScreen, RRCrtcPtr crtc);

typedef Bool (*RROutputSetPropertyProcPtr) (ScreenPtr pScreen,
                                            RROutputPtr output,
                                            Atom property,
                                            RRPropertyValuePtr value);

typedef Bool (*RROutputValidateModeProcPtr) (ScreenPtr pScreen,
                                             RROutputPtr output,
                                             RRModePtr mode);

typedef void (*RRModeDestroyProcPtr) (ScreenPtr pScreen, RRModePtr mode);

#endif

#if RANDR_13_INTERFACE
typedef Bool (*RROutputGetPropertyProcPtr) (ScreenPtr pScreen,
                                            RROutputPtr output, Atom property);
typedef Bool (*RRGetPanningProcPtr) (ScreenPtr pScrn,
                                     RRCrtcPtr crtc,
                                     BoxPtr totalArea,
                                     BoxPtr trackingArea, INT16 *border);
typedef Bool (*RRSetPanningProcPtr) (ScreenPtr pScrn,
                                     RRCrtcPtr crtc,
                                     BoxPtr totalArea,
                                     BoxPtr trackingArea, INT16 *border);

#endif                          /* RANDR_13_INTERFACE */

typedef Bool (*RRProviderGetPropertyProcPtr) (ScreenPtr pScreen,
                                            RRProviderPtr provider, Atom property);
typedef Bool (*RRProviderSetPropertyProcPtr) (ScreenPtr pScreen,
                                              RRProviderPtr provider,
                                              Atom property,
                                              RRPropertyValuePtr value);

typedef Bool (*RRGetInfoProcPtr) (ScreenPtr pScreen, Rotation * rotations);
typedef Bool (*RRCloseScreenProcPtr) (ScreenPtr pscreen);

typedef Bool (*RRProviderSetOutputSourceProcPtr)(ScreenPtr pScreen,
                                          RRProviderPtr provider,
                                          RRProviderPtr output_source);

typedef Bool (*RRProviderSetOffloadSinkProcPtr)(ScreenPtr pScreen,
                                         RRProviderPtr provider,
                                         RRProviderPtr offload_sink);


typedef void (*RRProviderDestroyProcPtr)(ScreenPtr pScreen,
                                         RRProviderPtr provider);

/* Additions for 1.6 */

typedef int (*RRCreateLeaseProcPtr)(ScreenPtr screen,
                                    RRLeasePtr lease,
                                    int *fd);

typedef void (*RRTerminateLeaseProcPtr)(ScreenPtr screen,
                                        RRLeasePtr lease);

/* These are for 1.0 compatibility */

typedef struct _rrRefresh {
    CARD16 rate;
    RRModePtr mode;
} RRScreenRate, *RRScreenRatePtr;

typedef struct _rrScreenSize {
    int id;
    short width, height;
    short mmWidth, mmHeight;
    int nRates;
    RRScreenRatePtr pRates;
} RRScreenSize, *RRScreenSizePtr;

#ifdef RANDR_10_INTERFACE

typedef Bool (*RRSetConfigProcPtr) (ScreenPtr pScreen,
                                    Rotation rotation,
                                    int rate, RRScreenSizePtr pSize);

#endif

typedef Bool (*RRCrtcSetScanoutPixmapProcPtr)(RRCrtcPtr crtc, PixmapPtr pixmap);

typedef Bool (*RRStartFlippingPixmapTrackingProcPtr)(RRCrtcPtr, DrawablePtr,
                                                     PixmapPtr, PixmapPtr,
                                                     int x, int y,
                                                     int dst_x, int dst_y,
                                                     Rotation rotation);

typedef Bool (*RREnableSharedPixmapFlippingProcPtr)(RRCrtcPtr,
                                                    PixmapPtr front,
                                                    PixmapPtr back);

typedef void (*RRDisableSharedPixmapFlippingProcPtr)(RRCrtcPtr);


typedef struct _rrScrPriv {
    /*
     * 'public' part of the structure; DDXen fill this in
     * as they initialize
     */
#if RANDR_10_INTERFACE
    RRSetConfigProcPtr rrSetConfig;
#endif
    RRGetInfoProcPtr rrGetInfo;
#if RANDR_12_INTERFACE
    RRScreenSetSizeProcPtr rrScreenSetSize;
    RRCrtcSetProcPtr rrCrtcSet;
    RRCrtcSetGammaProcPtr rrCrtcSetGamma;
    RRCrtcGetGammaProcPtr rrCrtcGetGamma;
    RROutputSetPropertyProcPtr rrOutputSetProperty;
    RROutputValidateModeProcPtr rrOutputValidateMode;
    RRModeDestroyProcPtr rrModeDestroy;
#endif
#if RANDR_13_INTERFACE
    RROutputGetPropertyProcPtr rrOutputGetProperty;
    RRGetPanningProcPtr rrGetPanning;
    RRSetPanningProcPtr rrSetPanning;
#endif
    /* TODO #if RANDR_15_INTERFACE */
    RRCrtcSetScanoutPixmapProcPtr rrCrtcSetScanoutPixmap;

    RRStartFlippingPixmapTrackingProcPtr rrStartFlippingPixmapTracking;
    RREnableSharedPixmapFlippingProcPtr rrEnableSharedPixmapFlipping;
    RRDisableSharedPixmapFlippingProcPtr rrDisableSharedPixmapFlipping;

    RRProviderSetOutputSourceProcPtr rrProviderSetOutputSource;
    RRProviderSetOffloadSinkProcPtr rrProviderSetOffloadSink;
    RRProviderGetPropertyProcPtr rrProviderGetProperty;
    RRProviderSetPropertyProcPtr rrProviderSetProperty;

    RRCreateLeaseProcPtr rrCreateLease;
    RRTerminateLeaseProcPtr rrTerminateLease;

    /*
     * Private part of the structure; not considered part of the ABI
     */
    TimeStamp lastSetTime;      /* last changed by client */
    TimeStamp lastConfigTime;   /* possible configs changed */
    RRCloseScreenProcPtr CloseScreen;

    Bool changed;               /* some config changed */
    Bool configChanged;         /* configuration changed */
    Bool layoutChanged;         /* screen layout changed */
    Bool resourcesChanged;      /* screen resources change */
    Bool leasesChanged;         /* leases change */

    CARD16 minWidth, minHeight;
    CARD16 maxWidth, maxHeight;
    CARD16 width, height;       /* last known screen size */
    CARD16 mmWidth, mmHeight;   /* last known screen size */

    int numOutputs;
    RROutputPtr *outputs;
    RROutputPtr primaryOutput;

    int numCrtcs;
    RRCrtcPtr *crtcs;

    /* Last known pointer position */
    RRCrtcPtr pointerCrtc;

#ifdef RANDR_10_INTERFACE
    /*
     * Configuration information
     */
    Rotation rotations;
    CARD16 reqWidth, reqHeight;

    int nSizes;
    RRScreenSizePtr pSizes;

    Rotation rotation;
    int rate;
    int size;
#endif
    Bool discontiguous;

    RRProviderPtr provider;

    RRProviderDestroyProcPtr rrProviderDestroy;

    int numMonitors;
    RRMonitorPtr *monitors;

    struct xorg_list leases;
} rrScrPrivRec, *rrScrPrivPtr;

extern _X_EXPORT DevPrivateKeyRec rrPrivKeyRec;

#define rrPrivKey (&rrPrivKeyRec)

#define rrGetScrPriv(pScr)  ((rrScrPrivPtr)dixLookupPrivate(&(pScr)->devPrivates, rrPrivKey))
#define rrScrPriv(pScr)	rrScrPrivPtr    pScrPriv = rrGetScrPriv(pScr)
#define SetRRScreen(s,p) dixSetPrivate(&(s)->devPrivates, rrPrivKey, p)

/*
 * each window has a list of clients requesting
 * RRNotify events.  Each client has a resource
 * for each window it selects RRNotify input for,
 * this resource is used to delete the RRNotifyRec
 * entry from the per-window queue.
 */

typedef struct _RREvent *RREventPtr;

typedef struct _RREvent {
    RREventPtr next;
    ClientPtr client;
    WindowPtr window;
    XID clientResource;
    int mask;
} RREventRec;

typedef struct _RRTimes {
    TimeStamp setTime;
    TimeStamp configTime;
} RRTimesRec, *RRTimesPtr;

typedef struct _RRClient {
    int major_version;
    int minor_version;
/*  RRTimesRec	times[0]; */
} RRClientRec, *RRClientPtr;

extern RESTYPE RRClientType, RREventType;     /* resource types for event masks */
extern DevPrivateKeyRec RRClientPrivateKeyRec;

#define RRClientPrivateKey (&RRClientPrivateKeyRec)
extern _X_EXPORT RESTYPE RRCrtcType, RRModeType, RROutputType, RRProviderType, RRLeaseType;

#define VERIFY_RR_OUTPUT(id, ptr, a)\
    {\
	int rc = dixLookupResourceByType((void **)&(ptr), id,\
	                                 RROutputType, client, a);\
	if (rc != Success) {\
	    client->errorValue = id;\
	    return rc;\
	}\
    }

#define VERIFY_RR_CRTC(id, ptr, a)\
    {\
	int rc = dixLookupResourceByType((void **)&(ptr), id,\
	                                 RRCrtcType, client, a);\
	if (rc != Success) {\
	    client->errorValue = id;\
	    return rc;\
	}\
    }

#define VERIFY_RR_MODE(id, ptr, a)\
    {\
	int rc = dixLookupResourceByType((void **)&(ptr), id,\
	                                 RRModeType, client, a);\
	if (rc != Success) {\
	    client->errorValue = id;\
	    return rc;\
	}\
    }

#define VERIFY_RR_PROVIDER(id, ptr, a)\
    {\
        int rc = dixLookupResourceByType((void **)&(ptr), id,\
                                         RRProviderType, client, a);\
        if (rc != Success) {\
            client->errorValue = id;\
            return rc;\
        }\
    }

#define VERIFY_RR_LEASE(id, ptr, a)\
    {\
        int rc = dixLookupResourceByType((void **)&(ptr), id,\
                                         RRLeaseType, client, a);\
        if (rc != Success) {\
            client->errorValue = id;\
            return rc;\
        }\
    }

#define GetRRClient(pClient)    ((RRClientPtr)dixLookupPrivate(&(pClient)->devPrivates, RRClientPrivateKey))
#define rrClientPriv(pClient)	RRClientPtr pRRClient = GetRRClient(pClient)

#ifdef RANDR_12_INTERFACE
/*
 * Set the range of sizes for the screen
 */
extern _X_EXPORT void

RRScreenSetSizeRange(ScreenPtr pScreen,
                     CARD16 minWidth,
                     CARD16 minHeight, CARD16 maxWidth, CARD16 maxHeight);
#endif

/* rrscreen.c */
/*
 * Notify the extension that the screen size has been changed.
 * The driver is responsible for calling this whenever it has changed
 * the size of the screen
 */
extern _X_EXPORT void
 RRScreenSizeNotify(ScreenPtr pScreen);

/*
 * Request that the screen be resized
 */
extern _X_EXPORT Bool

RRScreenSizeSet(ScreenPtr pScreen,
                CARD16 width, CARD16 height, CARD32 mmWidth, CARD32 mmHeight);

/*
 * Send ConfigureNotify event to root window when 'something' happens
 */
extern _X_EXPORT void
 RRSendConfigNotify(ScreenPtr pScreen);

/*
 * screen dispatch
 */
extern _X_EXPORT int
 ProcRRGetScreenSizeRange(ClientPtr client);

extern _X_EXPORT int
 ProcRRSetScreenSize(ClientPtr client);

extern _X_EXPORT int
 ProcRRGetScreenResources(ClientPtr client);

extern _X_EXPORT int
 ProcRRGetScreenResourcesCurrent(ClientPtr client);

extern _X_EXPORT int
 ProcRRSetScreenConfig(ClientPtr client);

extern _X_EXPORT int
 ProcRRGetScreenInfo(ClientPtr client);

/*
 * Deliver a ScreenNotify event
 */
extern _X_EXPORT void
 RRDeliverScreenEvent(ClientPtr client, WindowPtr pWin, ScreenPtr pScreen);

extern _X_EXPORT void
 RRResourcesChanged(ScreenPtr pScreen);

/* randr.c */
/* set a screen change on the primary screen */
extern _X_EXPORT void
RRSetChanged(ScreenPtr pScreen);

/*
 * Send all pending events
 */
extern _X_EXPORT void
 RRTellChanged(ScreenPtr pScreen);

/*
 * Poll the driver for changed information
 */
extern _X_EXPORT Bool
 RRGetInfo(ScreenPtr pScreen, Bool force_query);

extern _X_EXPORT Bool RRInit(void);

extern _X_EXPORT Bool RRScreenInit(ScreenPtr pScreen);

extern _X_EXPORT RROutputPtr RRFirstOutput(ScreenPtr pScreen);

extern _X_EXPORT RRCrtcPtr RRFirstEnabledCrtc(ScreenPtr pScreen);

extern _X_EXPORT Bool RROutputSetNonDesktop(RROutputPtr output, Bool non_desktop);

extern _X_EXPORT CARD16
 RRVerticalRefresh(xRRModeInfo * mode);

#ifdef RANDR_10_INTERFACE
/*
 * This is the old interface, deprecated but left
 * around for compatibility
 */

/*
 * Then, register the specific size with the screen
 */

extern _X_EXPORT RRScreenSizePtr
RRRegisterSize(ScreenPtr pScreen,
               short width, short height, short mmWidth, short mmHeight);

extern _X_EXPORT Bool
 RRRegisterRate(ScreenPtr pScreen, RRScreenSizePtr pSize, int rate);

/*
 * Finally, set the current configuration of the screen
 */

extern _X_EXPORT void

RRSetCurrentConfig(ScreenPtr pScreen,
                   Rotation rotation, int rate, RRScreenSizePtr pSize);

extern _X_EXPORT Rotation RRGetRotation(ScreenPtr pScreen);

#endif

/* rrcrtc.c */

/*
 * Notify the CRTC of some change; layoutChanged indicates that
 * some position or size element changed
 */
extern _X_EXPORT void
 RRCrtcChanged(RRCrtcPtr crtc, Bool layoutChanged);

/*
 * Create a CRTC
 */
extern _X_EXPORT RRCrtcPtr RRCrtcCreate(ScreenPtr pScreen, void *devPrivate);

/*
 * Tests if findCrtc belongs to pScreen or secondary screens
 */
extern _X_EXPORT Bool RRCrtcExists(ScreenPtr pScreen, RRCrtcPtr findCrtc);

/*
 * Set the allowed rotations on a CRTC
 */
extern _X_EXPORT void
 RRCrtcSetRotations(RRCrtcPtr crtc, Rotation rotations);

/*
 * Set whether transforms are allowed on a CRTC
 */
extern _X_EXPORT void
 RRCrtcSetTransformSupport(RRCrtcPtr crtc, Bool transforms);

/*
 * Notify the extension that the Crtc has been reconfigured,
 * the driver calls this whenever it has updated the mode
 */
extern _X_EXPORT Bool

RRCrtcNotify(RRCrtcPtr crtc,
             RRModePtr mode,
             int x,
             int y,
             Rotation rotation,
             RRTransformPtr transform, int numOutputs, RROutputPtr * outputs);

extern _X_EXPORT void
 RRDeliverCrtcEvent(ClientPtr client, WindowPtr pWin, RRCrtcPtr crtc);

/*
 * Request that the Crtc be reconfigured
 */
extern _X_EXPORT Bool

RRCrtcSet(RRCrtcPtr crtc,
          RRModePtr mode,
          int x,
          int y, Rotation rotation, int numOutput, RROutputPtr * outputs);

/*
 * Request that the Crtc gamma be changed
 */

extern _X_EXPORT Bool
 RRCrtcGammaSet(RRCrtcPtr crtc, CARD16 *red, CARD16 *green, CARD16 *blue);

/*
 * Request current gamma back from the DDX (if possible).
 * This includes gamma size.
 */

extern _X_EXPORT Bool
 RRCrtcGammaGet(RRCrtcPtr crtc);

/*
 * Notify the extension that the Crtc gamma has been changed
 * The driver calls this whenever it has changed the gamma values
 * in the RRCrtcRec
 */

extern _X_EXPORT Bool
 RRCrtcGammaNotify(RRCrtcPtr crtc);

/*
 * Set the size of the gamma table at server startup time
 */

extern _X_EXPORT Bool
 RRCrtcGammaSetSize(RRCrtcPtr crtc, int size);

/*
 * Return the area of the frame buffer scanned out by the crtc,
 * taking into account the current mode and rotation
 */

extern _X_EXPORT void
 RRCrtcGetScanoutSize(RRCrtcPtr crtc, int *width, int *height);

/*
 * Return crtc transform
 */
extern _X_EXPORT RRTransformPtr RRCrtcGetTransform(RRCrtcPtr crtc);

/*
 * Check whether the pending and current transforms are the same
 */
extern _X_EXPORT Bool
 RRCrtcPendingTransform(RRCrtcPtr crtc);

/*
 * Destroy a Crtc at shutdown
 */
extern _X_EXPORT void
 RRCrtcDestroy(RRCrtcPtr crtc);

/*
 * Set the pending CRTC transformation
 */

extern _X_EXPORT int

RRCrtcTransformSet(RRCrtcPtr crtc,
                   PictTransformPtr transform,
                   struct pict_f_transform *f_transform,
                   struct pict_f_transform *f_inverse,
                   char *filter, int filter_len, xFixed * params, int nparams);

/*
 * Initialize crtc type
 */
extern _X_EXPORT Bool
 RRCrtcInit(void);

/*
 * Initialize crtc type error value
 */
extern _X_EXPORT void
 RRCrtcInitErrorValue(void);

/*
 * Detach and free a scanout pixmap
 */
extern _X_EXPORT void
 RRCrtcDetachScanoutPixmap(RRCrtcPtr crtc);

extern _X_EXPORT Bool
 RRReplaceScanoutPixmap(DrawablePtr pDrawable, PixmapPtr pPixmap, Bool enable);

/*
 * Return if the screen has any scanout_pixmap's attached
 */
extern _X_EXPORT Bool
 RRHasScanoutPixmap(ScreenPtr pScreen);

/*
 * Crtc dispatch
 */

extern _X_EXPORT int
 ProcRRGetCrtcInfo(ClientPtr client);

extern _X_EXPORT int
 ProcRRSetCrtcConfig(ClientPtr client);

extern _X_EXPORT int
 ProcRRGetCrtcGammaSize(ClientPtr client);

extern _X_EXPORT int
 ProcRRGetCrtcGamma(ClientPtr client);

extern _X_EXPORT int
 ProcRRSetCrtcGamma(ClientPtr client);

extern _X_EXPORT int
 ProcRRSetCrtcTransform(ClientPtr client);

extern _X_EXPORT int
 ProcRRGetCrtcTransform(ClientPtr client);

int
 ProcRRGetPanning(ClientPtr client);

int
 ProcRRSetPanning(ClientPtr client);

void
 RRConstrainCursorHarder(DeviceIntPtr, ScreenPtr, int, int *, int *);

/* rrdispatch.c */
extern _X_EXPORT Bool
 RRClientKnowsRates(ClientPtr pClient);

/* rrlease.c */
void
RRDeliverLeaseEvent(ClientPtr client, WindowPtr window);

extern _X_EXPORT void
RRLeaseTerminated(RRLeasePtr lease);

extern _X_EXPORT void
RRLeaseFree(RRLeasePtr lease);

extern _X_EXPORT Bool
RRCrtcIsLeased(RRCrtcPtr crtc);

extern _X_EXPORT Bool
RROutputIsLeased(RROutputPtr output);

void
RRTerminateLease(RRLeasePtr lease);

Bool
RRLeaseInit(void);

/* rrmode.c */
/*
 * Find, and if necessary, create a mode
 */

extern _X_EXPORT RRModePtr RRModeGet(xRRModeInfo * modeInfo, const char *name);

/*
 * Destroy a mode.
 */

extern _X_EXPORT void
 RRModeDestroy(RRModePtr mode);

/*
 * Return a list of modes that are valid for some output in pScreen
 */
extern _X_EXPORT RRModePtr *RRModesForScreen(ScreenPtr pScreen, int *num_ret);

/*
 * Initialize mode type
 */
extern _X_EXPORT Bool
 RRModeInit(void);

/*
 * Initialize mode type error value
 */
extern _X_EXPORT void
 RRModeInitErrorValue(void);

extern _X_EXPORT int
 ProcRRCreateMode(ClientPtr client);

extern _X_EXPORT int
 ProcRRDestroyMode(ClientPtr client);

extern _X_EXPORT int
 ProcRRAddOutputMode(ClientPtr client);

extern _X_EXPORT int
 ProcRRDeleteOutputMode(ClientPtr client);

/* rroutput.c */

/*
 * Notify the output of some change. configChanged indicates whether
 * any external configuration (mode list, clones, connected status)
 * has changed, or whether the change was strictly internal
 * (which crtc is in use)
 */
extern _X_EXPORT void
 RROutputChanged(RROutputPtr output, Bool configChanged);

/*
 * Create an output
 */

extern _X_EXPORT RROutputPtr
RROutputCreate(ScreenPtr pScreen,
               const char *name, int nameLength, void *devPrivate);

/*
 * Notify extension that output parameters have been changed
 */
extern _X_EXPORT Bool
 RROutputSetClones(RROutputPtr output, RROutputPtr * clones, int numClones);

extern _X_EXPORT Bool

RROutputSetModes(RROutputPtr output,
                 RRModePtr * modes, int numModes, int numPreferred);

extern _X_EXPORT int
 RROutputAddUserMode(RROutputPtr output, RRModePtr mode);

extern _X_EXPORT int
 RROutputDeleteUserMode(RROutputPtr output, RRModePtr mode);

extern _X_EXPORT Bool
 RROutputSetCrtcs(RROutputPtr output, RRCrtcPtr * crtcs, int numCrtcs);

extern _X_EXPORT Bool
 RROutputSetConnection(RROutputPtr output, CARD8 connection);

extern _X_EXPORT Bool
 RROutputSetSubpixelOrder(RROutputPtr output, int subpixelOrder);

extern _X_EXPORT Bool
 RROutputSetPhysicalSize(RROutputPtr output, int mmWidth, int mmHeight);

extern _X_EXPORT void
 RRDeliverOutputEvent(ClientPtr client, WindowPtr pWin, RROutputPtr output);

extern _X_EXPORT void
 RROutputDestroy(RROutputPtr output);

extern _X_EXPORT int
 ProcRRGetOutputInfo(ClientPtr client);

extern _X_EXPORT int
 ProcRRSetOutputPrimary(ClientPtr client);

extern _X_EXPORT int
 ProcRRGetOutputPrimary(ClientPtr client);

/*
 * Initialize output type
 */
extern _X_EXPORT Bool
 RROutputInit(void);

/*
 * Initialize output type error value
 */
extern _X_EXPORT void
 RROutputInitErrorValue(void);

/* rrpointer.c */
extern _X_EXPORT void
 RRPointerMoved(ScreenPtr pScreen, int x, int y);

extern _X_EXPORT void
 RRPointerScreenConfigured(ScreenPtr pScreen);

/* rrproperty.c */

extern _X_EXPORT void
 RRDeleteAllOutputProperties(RROutputPtr output);

extern _X_EXPORT RRPropertyValuePtr
RRGetOutputProperty(RROutputPtr output, Atom property, Bool pending);

extern _X_EXPORT RRPropertyPtr
RRQueryOutputProperty(RROutputPtr output, Atom property);

extern _X_EXPORT void
 RRDeleteOutputProperty(RROutputPtr output, Atom property);

extern _X_EXPORT Bool
 RRPostPendingProperties(RROutputPtr output);

extern _X_EXPORT int

RRChangeOutputProperty(RROutputPtr output, Atom property, Atom type,
                       int format, int mode, unsigned long len,
                       const void *value, Bool sendevent, Bool pending);

extern _X_EXPORT int

RRConfigureOutputProperty(RROutputPtr output, Atom property,
                          Bool pending, Bool range, Bool immutable,
                          int num_values, const INT32 *values);
extern _X_EXPORT int
 ProcRRChangeOutputProperty(ClientPtr client);

extern _X_EXPORT int
 ProcRRGetOutputProperty(ClientPtr client);

extern _X_EXPORT int
 ProcRRListOutputProperties(ClientPtr client);

extern _X_EXPORT int
 ProcRRQueryOutputProperty(ClientPtr client);

extern _X_EXPORT int
 ProcRRConfigureOutputProperty(ClientPtr client);

extern _X_EXPORT int
 ProcRRDeleteOutputProperty(ClientPtr client);

/* rrprovider.c */
#define PRIME_SYNC_PROP         "PRIME Synchronization"
extern _X_EXPORT void
RRProviderInitErrorValue(void);

extern _X_EXPORT int
ProcRRGetProviders(ClientPtr client);

extern _X_EXPORT int
ProcRRGetProviderInfo(ClientPtr client);

extern _X_EXPORT int
ProcRRSetProviderOutputSource(ClientPtr client);

extern _X_EXPORT int
ProcRRSetProviderOffloadSink(ClientPtr client);

extern _X_EXPORT Bool
RRProviderInit(void);

extern _X_EXPORT RRProviderPtr
RRProviderCreate(ScreenPtr pScreen, const char *name,
                 int nameLength);

extern _X_EXPORT void
RRProviderDestroy (RRProviderPtr provider);

extern _X_EXPORT void
RRProviderSetCapabilities(RRProviderPtr provider, uint32_t capabilities);

extern _X_EXPORT Bool
RRProviderLookup(XID id, RRProviderPtr *provider_p);

extern _X_EXPORT void
RRDeliverProviderEvent(ClientPtr client, WindowPtr pWin, RRProviderPtr provider);

extern _X_EXPORT void
RRProviderAutoConfigGpuScreen(ScreenPtr pScreen, ScreenPtr primaryScreen);

/* rrproviderproperty.c */

extern _X_EXPORT void
 RRDeleteAllProviderProperties(RRProviderPtr provider);

extern _X_EXPORT RRPropertyValuePtr
 RRGetProviderProperty(RRProviderPtr provider, Atom property, Bool pending);

extern _X_EXPORT RRPropertyPtr
 RRQueryProviderProperty(RRProviderPtr provider, Atom property);

extern _X_EXPORT void
 RRDeleteProviderProperty(RRProviderPtr provider, Atom property);

extern _X_EXPORT int
RRChangeProviderProperty(RRProviderPtr provider, Atom property, Atom type,
                       int format, int mode, unsigned long len,
                       void *value, Bool sendevent, Bool pending);

extern _X_EXPORT int
 RRConfigureProviderProperty(RRProviderPtr provider, Atom property,
                             Bool pending, Bool range, Bool immutable,
                             int num_values, INT32 *values);

extern _X_EXPORT Bool
 RRPostProviderPendingProperties(RRProviderPtr provider);

extern _X_EXPORT int
 ProcRRGetProviderProperty(ClientPtr client);

extern _X_EXPORT int
 ProcRRListProviderProperties(ClientPtr client);

extern _X_EXPORT int
 ProcRRQueryProviderProperty(ClientPtr client);

extern _X_EXPORT int
ProcRRConfigureProviderProperty(ClientPtr client);

extern _X_EXPORT int
ProcRRChangeProviderProperty(ClientPtr client);

extern _X_EXPORT int
 ProcRRDeleteProviderProperty(ClientPtr client);
/* rrxinerama.c */
#ifdef XINERAMA
extern _X_EXPORT void
 RRXineramaExtensionInit(void);
#endif

void
RRMonitorInit(ScreenPtr screen);

Bool
RRMonitorMakeList(ScreenPtr screen, Bool get_active, RRMonitorPtr *monitors_ret, int *nmon_ret);

int
RRMonitorCountList(ScreenPtr screen);

void
RRMonitorFreeList(RRMonitorPtr monitors, int nmon);

void
RRMonitorClose(ScreenPtr screen);

RRMonitorPtr
RRMonitorAlloc(int noutput);

int
RRMonitorAdd(ClientPtr client, ScreenPtr screen, RRMonitorPtr monitor);

void
RRMonitorFree(RRMonitorPtr monitor);

int
ProcRRGetMonitors(ClientPtr client);

int
ProcRRSetMonitor(ClientPtr client);

int
ProcRRDeleteMonitor(ClientPtr client);

int
ProcRRCreateLease(ClientPtr client);

int
ProcRRFreeLease(ClientPtr client);

#endif                          /* _RANDRSTR_H_ */

/*

randr extension implementation structure

Query state:
    ProcRRGetScreenInfo/ProcRRGetScreenResources
	RRGetInfo

	    • Request configuration from driver, either 1.0 or 1.2 style
	    • These functions only record state changes, all
	      other actions are pended until RRTellChanged is called

	    ->rrGetInfo
	    1.0:
		RRRegisterSize
		RRRegisterRate
		RRSetCurrentConfig
	    1.2:
		RRScreenSetSizeRange
		RROutputSetCrtcs
		RRModeGet
		RROutputSetModes
		RROutputSetConnection
		RROutputSetSubpixelOrder
		RROutputSetClones
		RRCrtcNotify

	• Must delay scanning configuration until after ->rrGetInfo returns
	  because some drivers will call SetCurrentConfig in the middle
	  of the ->rrGetInfo operation.

	1.0:

	    • Scan old configuration, mirror to new structures

	    RRScanOldConfig
		RRCrtcCreate
		RROutputCreate
		RROutputSetCrtcs
		RROutputSetConnection
		RROutputSetSubpixelOrder
		RROldModeAdd	• This adds modes one-at-a-time
		    RRModeGet
		RRCrtcNotify

	• send events, reset pointer if necessary

	RRTellChanged
	    WalkTree (sending events)

	    • when layout has changed:
		RRPointerScreenConfigured
		RRSendConfigNotify

Asynchronous state setting (1.2 only)
    When setting state asynchronously, the driver invokes the
    ->rrGetInfo function and then calls RRTellChanged to flush
    the changes to the clients and reset pointer if necessary

Set state

    ProcRRSetScreenConfig
	RRCrtcSet
	    1.2:
		->rrCrtcSet
		    RRCrtcNotify
	    1.0:
		->rrSetConfig
		RRCrtcNotify
	    RRTellChanged
 */
