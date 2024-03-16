
/* xf86DDC.h
 *
 * This file contains all information to interpret a standard EDIC block
 * transmitted by a display device via DDC (Display Data Channel). So far
 * there is no information to deal with optional EDID blocks.
 * DDC is a Trademark of VESA (Video Electronics Standard Association).
 *
 * Copyright 1998 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */

#ifndef XF86_DDC_H
#define XF86_DDC_H

#include "edid.h"
#include "xf86i2c.h"
#include "xf86str.h"

/* speed up / slow down */
typedef enum {
    DDC_SLOW,
    DDC_FAST
} xf86ddcSpeed;

typedef void (*DDC1SetSpeedProc) (ScrnInfoPtr, xf86ddcSpeed);

extern _X_EXPORT xf86MonPtr xf86DoEDID_DDC1(ScrnInfoPtr pScrn,
                                            DDC1SetSpeedProc DDC1SetSpeed,
                                            unsigned
                                            int (*DDC1Read) (ScrnInfoPtr)
    );

extern _X_EXPORT xf86MonPtr xf86DoEDID_DDC2(ScrnInfoPtr pScrn, I2CBusPtr pBus);

extern _X_EXPORT xf86MonPtr xf86DoEEDID(ScrnInfoPtr pScrn, I2CBusPtr pBus, Bool);

extern _X_EXPORT xf86MonPtr xf86PrintEDID(xf86MonPtr monPtr);

extern _X_EXPORT xf86MonPtr xf86InterpretEDID(int screenIndex, Uchar * block);

extern _X_EXPORT xf86MonPtr xf86InterpretEEDID(int screenIndex, Uchar * block);

extern _X_EXPORT void
 xf86EdidMonitorSet(int scrnIndex, MonPtr Monitor, xf86MonPtr DDC);

extern _X_EXPORT Bool xf86SetDDCproperties(ScrnInfoPtr pScreen, xf86MonPtr DDC);

extern _X_EXPORT Bool
 xf86MonitorIsHDMI(xf86MonPtr mon);

extern _X_EXPORT Bool
gtf_supported(xf86MonPtr mon);

extern _X_EXPORT DisplayModePtr
FindDMTMode(int hsize, int vsize, int refresh, Bool rb);

extern _X_EXPORT const DisplayModeRec DMTModes[];

/*
 * Quirks to work around broken EDID data from various monitors.
 */
typedef enum {
    DDC_QUIRK_NONE = 0,
    /* First detailed mode is bogus, prefer largest mode at 60hz */
    DDC_QUIRK_PREFER_LARGE_60 = 1 << 0,
    /* 135MHz clock is too high, drop a bit */
    DDC_QUIRK_135_CLOCK_TOO_HIGH = 1 << 1,
    /* Prefer the largest mode at 75 Hz */
    DDC_QUIRK_PREFER_LARGE_75 = 1 << 2,
    /* Convert detailed timing's horizontal from units of cm to mm */
    DDC_QUIRK_DETAILED_H_IN_CM = 1 << 3,
    /* Convert detailed timing's vertical from units of cm to mm */
    DDC_QUIRK_DETAILED_V_IN_CM = 1 << 4,
    /* Detailed timing descriptors have bogus size values, so just take the
     * maximum size and use that.
     */
    DDC_QUIRK_DETAILED_USE_MAXIMUM_SIZE = 1 << 5,
    /* Monitor forgot to set the first detailed is preferred bit. */
    DDC_QUIRK_FIRST_DETAILED_PREFERRED = 1 << 6,
    /* use +hsync +vsync for detailed mode */
    DDC_QUIRK_DETAILED_SYNC_PP = 1 << 7,
    /* Force single-link DVI bandwidth limit */
    DDC_QUIRK_DVI_SINGLE_LINK = 1 << 8,
} ddc_quirk_t;

typedef void (*handle_detailed_fn) (struct detailed_monitor_section *, void *);

void xf86ForEachDetailedBlock(xf86MonPtr mon, handle_detailed_fn, void *data);

ddc_quirk_t xf86DDCDetectQuirks(int scrnIndex, xf86MonPtr DDC, Bool verbose);

void xf86DetTimingApplyQuirks(struct detailed_monitor_section *det_mon,
                              ddc_quirk_t quirks, int hsize, int vsize);

typedef void (*handle_video_fn) (struct cea_video_block *, void *);

void xf86ForEachVideoBlock(xf86MonPtr, handle_video_fn, void *);

struct cea_data_block *xf86MonitorFindHDMIBlock(xf86MonPtr mon);

#endif
