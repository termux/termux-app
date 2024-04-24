#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _DISMODEPROC_H_
#define _DISMODEPROC_H_

#include "scrnintstr.h"

#define MAXCLOCKS   128

/* These are possible return values for xf86CheckMode() and ValidMode() */
typedef enum {
    MODE_OK = 0,                /* Mode OK */
    MODE_HSYNC,                 /* hsync out of range */
    MODE_VSYNC,                 /* vsync out of range */
    MODE_H_ILLEGAL,             /* mode has illegal horizontal timings */
    MODE_V_ILLEGAL,             /* mode has illegal horizontal timings */
    MODE_BAD_WIDTH,             /* requires an unsupported linepitch */
    MODE_NOMODE,                /* no mode with a matching name */
    MODE_NO_INTERLACE,          /* interlaced mode not supported */
    MODE_NO_DBLESCAN,           /* doublescan mode not supported */
    MODE_NO_VSCAN,              /* multiscan mode not supported */
    MODE_MEM,                   /* insufficient video memory */
    MODE_VIRTUAL_X,             /* mode width too large for specified virtual size */
    MODE_VIRTUAL_Y,             /* mode height too large for specified virtual size */
    MODE_MEM_VIRT,              /* insufficient video memory given virtual size */
    MODE_NOCLOCK,               /* no fixed clock available */
    MODE_CLOCK_HIGH,            /* clock required is too high */
    MODE_CLOCK_LOW,             /* clock required is too low */
    MODE_CLOCK_RANGE,           /* clock/mode isn't in a ClockRange */
    MODE_BAD_HVALUE,            /* horizontal timing was out of range */
    MODE_BAD_VVALUE,            /* vertical timing was out of range */
    MODE_BAD_VSCAN,             /* VScan value out of range */
    MODE_HSYNC_NARROW,          /* horizontal sync too narrow */
    MODE_HSYNC_WIDE,            /* horizontal sync too wide */
    MODE_HBLANK_NARROW,         /* horizontal blanking too narrow */
    MODE_HBLANK_WIDE,           /* horizontal blanking too wide */
    MODE_VSYNC_NARROW,          /* vertical sync too narrow */
    MODE_VSYNC_WIDE,            /* vertical sync too wide */
    MODE_VBLANK_NARROW,         /* vertical blanking too narrow */
    MODE_VBLANK_WIDE,           /* vertical blanking too wide */
    MODE_PANEL,                 /* exceeds panel dimensions */
    MODE_INTERLACE_WIDTH,       /* width too large for interlaced mode */
    MODE_ONE_WIDTH,             /* only one width is supported */
    MODE_ONE_HEIGHT,            /* only one height is supported */
    MODE_ONE_SIZE,              /* only one resolution is supported */
    MODE_NO_REDUCED,            /* monitor doesn't accept reduced blanking */
    MODE_BANDWIDTH,             /* mode requires too much memory bandwidth */
    MODE_BAD = -2,              /* unspecified reason */
    MODE_ERROR = -1             /* error condition */
} ModeStatus;

/* Video mode */
typedef struct _DisplayModeRec {
    struct _DisplayModeRec *prev;
    struct _DisplayModeRec *next;
    const char *name;           /* identifier for the mode */
    ModeStatus status;
    int type;

    /* These are the values that the user sees/provides */
    int Clock;                  /* pixel clock freq (kHz) */
    int HDisplay;               /* horizontal timing */
    int HSyncStart;
    int HSyncEnd;
    int HTotal;
    int HSkew;
    int VDisplay;               /* vertical timing */
    int VSyncStart;
    int VSyncEnd;
    int VTotal;
    int VScan;
    int Flags;

    /* These are the values the hardware uses */
    int ClockIndex;
    int SynthClock;             /* Actual clock freq to
                                 * be programmed  (kHz) */
    int CrtcHDisplay;
    int CrtcHBlankStart;
    int CrtcHSyncStart;
    int CrtcHSyncEnd;
    int CrtcHBlankEnd;
    int CrtcHTotal;
    int CrtcHSkew;
    int CrtcVDisplay;
    int CrtcVBlankStart;
    int CrtcVSyncStart;
    int CrtcVSyncEnd;
    int CrtcVBlankEnd;
    int CrtcVTotal;
    Bool CrtcHAdjusted;
    Bool CrtcVAdjusted;
    int PrivSize;
    INT32 *Private;
    int PrivFlags;

    float HSync, VRefresh;
} DisplayModeRec, *DisplayModePtr;

#endif
