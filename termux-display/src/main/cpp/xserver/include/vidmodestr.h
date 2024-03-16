#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _VIDMODEPROC_H_
#define _VIDMODEPROC_H_

#include "displaymode.h"

typedef enum {
    VIDMODE_H_DISPLAY,
    VIDMODE_H_SYNCSTART,
    VIDMODE_H_SYNCEND,
    VIDMODE_H_TOTAL,
    VIDMODE_H_SKEW,
    VIDMODE_V_DISPLAY,
    VIDMODE_V_SYNCSTART,
    VIDMODE_V_SYNCEND,
    VIDMODE_V_TOTAL,
    VIDMODE_FLAGS,
    VIDMODE_CLOCK
} VidModeSelectMode;

typedef enum {
    VIDMODE_MON_VENDOR,
    VIDMODE_MON_MODEL,
    VIDMODE_MON_NHSYNC,
    VIDMODE_MON_NVREFRESH,
    VIDMODE_MON_HSYNC_LO,
    VIDMODE_MON_HSYNC_HI,
    VIDMODE_MON_VREFRESH_LO,
    VIDMODE_MON_VREFRESH_HI
} VidModeSelectMonitor;

typedef union {
    const void *ptr;
    int i;
    float f;
} vidMonitorValue;

typedef Bool            (*VidModeExtensionInitProcPtr)       (ScreenPtr pScreen);
typedef vidMonitorValue (*VidModeGetMonitorValueProcPtr)     (ScreenPtr pScreen,
                                                              int valtyp,
                                                              int indx);
typedef Bool            (*VidModeGetEnabledProcPtr)          (void);
typedef Bool            (*VidModeGetAllowNonLocalProcPtr)    (void);
typedef Bool            (*VidModeGetCurrentModelineProcPtr)  (ScreenPtr pScreen,
                                                              DisplayModePtr *mode,
                                                              int *dotClock);
typedef Bool            (*VidModeGetFirstModelineProcPtr)    (ScreenPtr pScreen,
                                                              DisplayModePtr *mode,
                                                              int *dotClock);
typedef Bool            (*VidModeGetNextModelineProcPtr)     (ScreenPtr pScreen,
                                                              DisplayModePtr *mode,
                                                              int *dotClock);
typedef Bool            (*VidModeDeleteModelineProcPtr)      (ScreenPtr pScreen,
                                                              DisplayModePtr mode);
typedef Bool            (*VidModeZoomViewportProcPtr)        (ScreenPtr pScreen,
                                                              int zoom);
typedef Bool            (*VidModeGetViewPortProcPtr)         (ScreenPtr pScreen,
                                                              int *x,
                                                              int *y);
typedef Bool            (*VidModeSetViewPortProcPtr)         (ScreenPtr pScreen,
                                                              int x,
                                                              int y);
typedef Bool            (*VidModeSwitchModeProcPtr)          (ScreenPtr pScreen,
                                                              DisplayModePtr mode);
typedef Bool            (*VidModeLockZoomProcPtr)            (ScreenPtr pScreen,
                                                              Bool lock);
typedef int             (*VidModeGetNumOfClocksProcPtr)      (ScreenPtr pScreen,
                                                              Bool *progClock);
typedef Bool            (*VidModeGetClocksProcPtr)           (ScreenPtr pScreen,
                                                              int *Clocks);
typedef ModeStatus      (*VidModeCheckModeForMonitorProcPtr) (ScreenPtr pScreen,
                                                              DisplayModePtr mode);
typedef ModeStatus      (*VidModeCheckModeForDriverProcPtr)  (ScreenPtr pScreen,
                                                              DisplayModePtr mode);
typedef void            (*VidModeSetCrtcForModeProcPtr)      (ScreenPtr pScreen,
                                                              DisplayModePtr mode);
typedef Bool            (*VidModeAddModelineProcPtr)         (ScreenPtr pScreen,
                                                              DisplayModePtr mode);
typedef int             (*VidModeGetDotClockProcPtr)         (ScreenPtr pScreen,
                                                              int Clock);
typedef int             (*VidModeGetNumOfModesProcPtr)       (ScreenPtr pScreen);
typedef Bool            (*VidModeSetGammaProcPtr)            (ScreenPtr pScreen,
                                                              float red,
                                                              float green,
                                                              float blue);
typedef Bool            (*VidModeGetGammaProcPtr)            (ScreenPtr pScreen,
                                                              float *red,
                                                              float *green,
                                                              float *blue);
typedef Bool            (*VidModeSetGammaRampProcPtr)        (ScreenPtr pScreen,
                                                              int size,
                                                              CARD16 *red,
                                                              CARD16 *green,
                                                              CARD16 *blue);
typedef Bool            (*VidModeGetGammaRampProcPtr)        (ScreenPtr pScreen,
                                                              int size,
                                                              CARD16 *red,
                                                              CARD16 *green,
                                                              CARD16 *blue);
typedef int             (*VidModeGetGammaRampSizeProcPtr)    (ScreenPtr pScreen);

typedef struct {
    DisplayModePtr First;
    DisplayModePtr Next;
    int Flags;

    VidModeExtensionInitProcPtr       ExtensionInit;
    VidModeGetMonitorValueProcPtr     GetMonitorValue;
    VidModeGetCurrentModelineProcPtr  GetCurrentModeline;
    VidModeGetFirstModelineProcPtr    GetFirstModeline;
    VidModeGetNextModelineProcPtr     GetNextModeline;
    VidModeDeleteModelineProcPtr      DeleteModeline;
    VidModeZoomViewportProcPtr        ZoomViewport;
    VidModeGetViewPortProcPtr         GetViewPort;
    VidModeSetViewPortProcPtr         SetViewPort;
    VidModeSwitchModeProcPtr          SwitchMode;
    VidModeLockZoomProcPtr            LockZoom;
    VidModeGetNumOfClocksProcPtr      GetNumOfClocks;
    VidModeGetClocksProcPtr           GetClocks;
    VidModeCheckModeForMonitorProcPtr CheckModeForMonitor;
    VidModeCheckModeForDriverProcPtr  CheckModeForDriver;
    VidModeSetCrtcForModeProcPtr      SetCrtcForMode;
    VidModeAddModelineProcPtr         AddModeline;
    VidModeGetDotClockProcPtr         GetDotClock;
    VidModeGetNumOfModesProcPtr       GetNumOfModes;
    VidModeSetGammaProcPtr            SetGamma;
    VidModeGetGammaProcPtr            GetGamma;
    VidModeSetGammaRampProcPtr        SetGammaRamp;
    VidModeGetGammaRampProcPtr        GetGammaRamp;
    VidModeGetGammaRampSizeProcPtr    GetGammaRampSize;
} VidModeRec, *VidModePtr;

#ifdef XF86VIDMODE
void VidModeAddExtension(Bool allow_non_local);
VidModePtr VidModeGetPtr(ScreenPtr pScreen);
VidModePtr VidModeInit(ScreenPtr pScreen);
#endif /* XF86VIDMODE */

#endif
