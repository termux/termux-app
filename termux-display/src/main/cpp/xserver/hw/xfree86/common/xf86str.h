
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * This file contains definitions of the public XFree86 data structures/types.
 * Any data structures that video drivers need to access should go here.
 */

#ifndef _XF86STR_H
#define _XF86STR_H

#include "misc.h"
#include "input.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "colormapst.h"
#include "xf86Module.h"
#include "xf86Opt.h"
#include "displaymode.h"

/**
 * Integer type that is of the size of the addressable memory (machine size).
 * On most platforms \c uintptr_t will suffice.  However, on some mixed
 * 32-bit / 64-bit platforms, such as 32-bit binaries on 64-bit PowerPC, this
 * must be 64-bits.
 */
#include <inttypes.h>
#if defined(__powerpc__)
typedef uint64_t memType;
#else
typedef uintptr_t memType;
#endif

/* Video mode flags */

typedef enum {
    V_PHSYNC = 0x0001,
    V_NHSYNC = 0x0002,
    V_PVSYNC = 0x0004,
    V_NVSYNC = 0x0008,
    V_INTERLACE = 0x0010,
    V_DBLSCAN = 0x0020,
    V_CSYNC = 0x0040,
    V_PCSYNC = 0x0080,
    V_NCSYNC = 0x0100,
    V_HSKEW = 0x0200,           /* hskew provided */
    V_BCAST = 0x0400,
    V_PIXMUX = 0x1000,
    V_DBLCLK = 0x2000,
    V_CLKDIV2 = 0x4000
} ModeFlags;

typedef enum {
    INTERLACE_HALVE_V = 0x0001  /* Halve V values for interlacing */
} CrtcAdjustFlags;

/* Flags passed to ChipValidMode() */
typedef enum {
    MODECHECK_INITIAL = 0,
    MODECHECK_FINAL = 1
} ModeCheckFlags;

/*
 * The mode sets are, from best to worst: USERDEF, DRIVER, and DEFAULT/BUILTIN.
 * Preferred will bubble a mode to the top within a set.
 */
#define M_T_BUILTIN 0x01        /* built-in mode */
#define M_T_CLOCK_C (0x02 | M_T_BUILTIN)        /* built-in mode - configure clock */
#define M_T_CRTC_C  (0x04 | M_T_BUILTIN)        /* built-in mode - configure CRTC  */
#define M_T_CLOCK_CRTC_C  (M_T_CLOCK_C | M_T_CRTC_C)
                               /* built-in mode - configure CRTC and clock */
#define M_T_PREFERRED 0x08      /* preferred mode within a set */
#define M_T_DEFAULT 0x10        /* (VESA) default modes */
#define M_T_USERDEF 0x20        /* One of the modes from the config file */
#define M_T_DRIVER  0x40        /* Supplied by the driver (EDID, etc) */
#define M_T_USERPREF 0x80       /* mode preferred by the user config */

/* The monitor description */

#define MAX_HSYNC 8
#define MAX_VREFRESH 8

typedef struct {
    float hi, lo;
} range;

typedef struct {
    CARD32 red, green, blue;
} rgb;

typedef struct {
    float red, green, blue;
} Gamma;

/* The permitted gamma range is 1 / GAMMA_MAX <= g <= GAMMA_MAX */
#define GAMMA_MAX	10.0
#define GAMMA_MIN	(1.0 / GAMMA_MAX)
#define GAMMA_ZERO	(GAMMA_MIN / 100.0)

typedef struct {
    const char *id;
    const char *vendor;
    const char *model;
    int nHsync;
    range hsync[MAX_HSYNC];
    int nVrefresh;
    range vrefresh[MAX_VREFRESH];
    DisplayModePtr Modes;       /* Start of the monitor's mode list */
    DisplayModePtr Last;        /* End of the monitor's mode list */
    Gamma gamma;                /* Gamma of the monitor */
    int widthmm;
    int heightmm;
    void *options;
    void *DDC;
    Bool reducedblanking;       /* Allow CVT reduced blanking modes? */
    int maxPixClock;            /* in kHz, like mode->Clock */
} MonRec, *MonPtr;

/* the list of clock ranges */
typedef struct x_ClockRange {
    struct x_ClockRange *next;
    int minClock;               /* (kHz) */
    int maxClock;               /* (kHz) */
    int clockIndex;             /* -1 for programmable clocks */
    Bool interlaceAllowed;
    Bool doubleScanAllowed;
    int ClockMulFactor;
    int ClockDivFactor;
    int PrivFlags;
} ClockRange, *ClockRangePtr;

/*
 * The driverFunc. xorgDriverFuncOp specifies the action driver should
 * perform. If requested option is not supported function should return
 * FALSE. pointer can be used to pass arguments to the function or
 * to return data to the caller.
 */
typedef struct _ScrnInfoRec *ScrnInfoPtr;

/* do not change order */
typedef enum {
    RR_GET_INFO,
    RR_SET_CONFIG,
    RR_GET_MODE_MM,
    GET_REQUIRED_HW_INTERFACES = 10,
    SUPPORTS_SERVER_FDS = 11,
} xorgDriverFuncOp;

typedef Bool xorgDriverFuncProc(ScrnInfoPtr, xorgDriverFuncOp, void *);

/* RR_GET_INFO, RR_SET_CONFIG */
typedef struct {
    int rotation;
    int rate;
    int width;
    int height;
} xorgRRConfig;

typedef union {
    short RRRotations;
    xorgRRConfig RRConfig;
} xorgRRRotation, *xorgRRRotationPtr;

/* RR_GET_MODE_MM */
typedef struct {
    DisplayModePtr mode;
    int virtX;
    int virtY;
    int mmWidth;
    int mmHeight;
} xorgRRModeMM, *xorgRRModeMMPtr;

/* GET_REQUIRED_HW_INTERFACES */
#define HW_IO 1
#define HW_MMIO 2
#define HW_SKIP_CONSOLE 4
#define NEED_IO_ENABLED(x) (x & HW_IO)

typedef CARD32 xorgHWFlags;

/*
 * The driver list struct.  This contains the information required for each
 * driver before a ScrnInfoRec has been allocated.
 */
struct _DriverRec;

struct _SymTabRec;
struct _PciChipsets;

struct pci_device;
struct xf86_platform_device;

typedef struct _DriverRec {
    int driverVersion;
    const char *driverName;
    void (*Identify) (int flags);
    Bool (*Probe) (struct _DriverRec * drv, int flags);
    const OptionInfoRec *(*AvailableOptions) (int chipid, int bustype);
    void *module;
    int refCount;
    xorgDriverFuncProc *driverFunc;

    const struct pci_id_match *supported_devices;
    Bool (*PciProbe) (struct _DriverRec * drv, int entity_num,
                      struct pci_device * dev, intptr_t match_data);
    Bool (*platformProbe) (struct _DriverRec * drv, int entity_num, int flags,
                           struct xf86_platform_device * dev, intptr_t match_data);
} DriverRec, *DriverPtr;

/*
 * platform probe flags
 */
#define PLATFORM_PROBE_GPU_SCREEN 1

/*
 *  AddDriver flags
 */
#define HaveDriverFuncs 1

/*
 * These are the private bus types.  New types can be added here.  Types
 * required for the public interface should be added to xf86str.h, with
 * function prototypes added to xf86.h.
 */

/* Tolerate prior #include <linux/input.h> */
#if defined(__linux__)
#undef BUS_NONE
#undef BUS_PCI
#undef BUS_SBUS
#undef BUS_PLATFORM
#undef BUS_USB
#undef BUS_last
#endif

typedef enum {
    BUS_NONE,
    BUS_PCI,
    BUS_SBUS,
    BUS_PLATFORM,
    BUS_USB,
    BUS_last                    /* Keep last */
} BusType;

typedef struct {
    int fbNum;
} SbusBusId;

typedef struct _bus {
    BusType type;
    union {
        struct pci_device *pci;
        SbusBusId sbus;
        struct xf86_platform_device *plat;
    } id;
} BusRec, *BusPtr;

typedef enum {
    DAC_BPP8 = 0,
    DAC_BPP16,
    DAC_BPP24,
    DAC_BPP32,
    MAXDACSPEEDS
} DacSpeedIndex;

typedef struct {
    const char *identifier;
    const char *vendor;
    const char *board;
    const char *chipset;
    const char *ramdac;
    const char *driver;
    struct _confscreenrec *myScreenSection;
    Bool claimed;
    int dacSpeeds[MAXDACSPEEDS];
    int numclocks;
    int clock[MAXCLOCKS];
    const char *clockchip;
    const char *busID;
    Bool active;
    Bool inUse;
    int videoRam;
    unsigned long MemBase;      /* Frame buffer base address */
    unsigned long IOBase;
    int chipID;
    int chipRev;
    void *options;
    int irq;
    int screen;                 /* For multi-CRTC cards */
} GDevRec, *GDevPtr;

typedef struct {
    int frameX0;
    int frameY0;
    int virtualX;
    int virtualY;
    int depth;
    int fbbpp;
    rgb weight;
    rgb blackColour;
    rgb whiteColour;
    int defaultVisual;
    const char **modes;
    void *options;
} DispRec, *DispPtr;

typedef struct _confxvportrec {
    const char *identifier;
    void *options;
} confXvPortRec, *confXvPortPtr;

typedef struct _confxvadaptrec {
    const char *identifier;
    int numports;
    confXvPortPtr ports;
    void *options;
} confXvAdaptorRec, *confXvAdaptorPtr;

#define MAX_GPUDEVICES 4
typedef struct _confscreenrec {
    const char *id;
    int screennum;
    int defaultdepth;
    int defaultbpp;
    int defaultfbbpp;
    MonPtr monitor;
    GDevPtr device;
    int numdisplays;
    DispPtr *displays;
    int numxvadaptors;
    confXvAdaptorPtr xvadaptors;
    void *options;

    int num_gpu_devices;
    GDevPtr gpu_devices[MAX_GPUDEVICES];
} confScreenRec, *confScreenPtr;

typedef enum {
    PosObsolete = -1,
    PosAbsolute = 0,
    PosRightOf,
    PosLeftOf,
    PosAbove,
    PosBelow,
    PosRelative
} PositionType;

typedef struct _screenlayoutrec {
    confScreenPtr screen;
    const char *topname;
    confScreenPtr top;
    const char *bottomname;
    confScreenPtr bottom;
    const char *leftname;
    confScreenPtr left;
    const char *rightname;
    confScreenPtr right;
    PositionType where;
    int x;
    int y;
    const char *refname;
    confScreenPtr refscreen;
} screenLayoutRec, *screenLayoutPtr;

typedef struct _InputInfoRec InputInfoRec;

typedef struct _serverlayoutrec {
    const char *id;
    screenLayoutPtr screens;
    GDevPtr inactives;
    InputInfoRec **inputs;      /* NULL terminated */
    void *options;
} serverLayoutRec, *serverLayoutPtr;

typedef struct _confdribufferrec {
    int count;
    int size;
    enum {
        XF86DRI_WC_HINT = 0x0001        /* Placeholder: not implemented */
    } flags;
} confDRIBufferRec, *confDRIBufferPtr;

typedef struct _confdrirec {
    int group;
    int mode;
    int bufs_count;
    confDRIBufferRec *bufs;
} confDRIRec, *confDRIPtr;

#define NUM_RESERVED_INTS		4
#define NUM_RESERVED_POINTERS		4
#define NUM_RESERVED_FUNCS		4

/* let clients know they can use this */
#define XF86_SCRN_HAS_PREFER_CLONE 1

typedef void *(*funcPointer) (void);

/* Power management events: so far we only support APM */

typedef enum {
    XF86_APM_UNKNOWN = -1,
    XF86_APM_SYS_STANDBY,
    XF86_APM_SYS_SUSPEND,
    XF86_APM_CRITICAL_SUSPEND,
    XF86_APM_USER_STANDBY,
    XF86_APM_USER_SUSPEND,
    XF86_APM_STANDBY_RESUME,
    XF86_APM_NORMAL_RESUME,
    XF86_APM_CRITICAL_RESUME,
    XF86_APM_LOW_BATTERY,
    XF86_APM_POWER_STATUS_CHANGE,
    XF86_APM_UPDATE_TIME,
    XF86_APM_CAPABILITY_CHANGED,
    XF86_APM_STANDBY_FAILED,
    XF86_APM_SUSPEND_FAILED
} pmEvent;

typedef enum {
    PM_WAIT,
    PM_CONTINUE,
    PM_FAILED,
    PM_NONE
} pmWait;

typedef struct _PciChipsets {
    /**
     * Key used to match this device with its name in an array of
     * \c SymTabRec.
     */
    int numChipset;

    /**
     * This value is quirky.  Depending on the driver, it can take on one of
     * three meanings.  In drivers that have exactly one vendor ID (e.g.,
     * radeon, mga, i810) the low 16-bits are the device ID.
     *
     * In drivers that can have multiple vendor IDs (e.g., the glint driver
     * can have either 3dlabs' ID or TI's ID, the i740 driver can have either
     * Intel's ID or Real3D's ID, etc.) the low 16-bits are the device ID and
     * the high 16-bits are the vendor ID.
     *
     * In drivers that don't have a specific vendor (e.g., vga) contains the
     * device ID for either the generic VGA or generic 8514 devices.  This
     * turns out to be the same as the subclass and programming interface
     * value (e.g., the full 24-bit class for the VGA device is 0x030000 (or
     * 0x000101) and for 8514 is 0x030001).
     */
    int PCIid;

/* dummy place holders for drivers to build against old/new servers */
#define RES_UNDEFINED NULL
#define RES_EXCLUSIVE_VGA NULL
#define RES_SHARED_VGA NULL
    void *dummy;
} PciChipsets;

/* Entity properties */
typedef void (*EntityProc) (int entityIndex, void *private);

typedef struct _entityInfo {
    int index;
    BusRec location;
    int chipset;
    Bool active;
    GDevPtr device;
    DriverPtr driver;
} EntityInfoRec, *EntityInfoPtr;

/* DGA */

typedef struct {
    int num;                    /* A unique identifier for the mode (num > 0) */
    DisplayModePtr mode;
    int flags;                  /* DGA_CONCURRENT_ACCESS, etc... */
    int imageWidth;             /* linear accessible portion (pixels) */
    int imageHeight;
    int pixmapWidth;            /* Xlib accessible portion (pixels) */
    int pixmapHeight;           /* both fields ignored if no concurrent access */
    int bytesPerScanline;
    int byteOrder;              /* MSBFirst, LSBFirst */
    int depth;
    int bitsPerPixel;
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
    short visualClass;
    int viewportWidth;
    int viewportHeight;
    int xViewportStep;          /* viewport position granularity */
    int yViewportStep;
    int maxViewportX;           /* max viewport origin */
    int maxViewportY;
    int viewportFlags;          /* types of page flipping possible */
    int offset;                 /* offset into physical memory */
    unsigned char *address;     /* server's mapped framebuffer */
    int reserved1;
    int reserved2;
} DGAModeRec, *DGAModePtr;

typedef struct {
    DGAModePtr mode;
    PixmapPtr pPix;
} DGADeviceRec, *DGADevicePtr;

/*
 * Flags for driver Probe() functions.
 */
#define PROBE_DEFAULT	  0x00
#define PROBE_DETECT	  0x01
#define PROBE_TRYHARD	  0x02

/*
 * Driver entry point types
 */

typedef Bool xf86ProbeProc(DriverPtr, int);
typedef Bool xf86PreInitProc(ScrnInfoPtr, int);
typedef Bool xf86ScreenInitProc(ScreenPtr, int, char **);
typedef Bool xf86SwitchModeProc(ScrnInfoPtr, DisplayModePtr);
typedef void xf86AdjustFrameProc(ScrnInfoPtr, int, int);
typedef Bool xf86EnterVTProc(ScrnInfoPtr);
typedef void xf86LeaveVTProc(ScrnInfoPtr);
typedef void xf86FreeScreenProc(ScrnInfoPtr);
typedef ModeStatus xf86ValidModeProc(ScrnInfoPtr, DisplayModePtr, Bool, int);
typedef void xf86EnableDisableFBAccessProc(ScrnInfoPtr, Bool);
typedef int xf86SetDGAModeProc(ScrnInfoPtr, int, DGADevicePtr);
typedef int xf86ChangeGammaProc(ScrnInfoPtr, Gamma);
typedef void xf86PointerMovedProc(ScrnInfoPtr, int, int);
typedef Bool xf86PMEventProc(ScrnInfoPtr, pmEvent, Bool);
typedef void xf86DPMSSetProc(ScrnInfoPtr, int, int);
typedef void xf86LoadPaletteProc(ScrnInfoPtr, int, int *, LOCO *, VisualPtr);
typedef void xf86SetOverscanProc(ScrnInfoPtr, int);
typedef void xf86ModeSetProc(ScrnInfoPtr);

/*
 * ScrnInfoRec
 *
 * There is one of these for each screen, and it holds all the screen-specific
 * information.  Note: No fields are to be dependent on compile-time defines.
 */

typedef struct _ScrnInfoRec {
    int driverVersion;
    const char *driverName;     /* canonical name used in */
    /* the config file */
    ScreenPtr pScreen;          /* Pointer to the ScreenRec */
    int scrnIndex;              /* Number of this screen */
    Bool configured;            /* Is this screen valid */
    int origIndex;              /* initial number assigned to
                                 * this screen before
                                 * finalising the number of
                                 * available screens */

    /* Display-wide screenInfo values needed by this screen */
    int imageByteOrder;
    int bitmapScanlineUnit;
    int bitmapScanlinePad;
    int bitmapBitOrder;
    int numFormats;
    PixmapFormatRec formats[MAXFORMATS];
    PixmapFormatRec fbFormat;

    int bitsPerPixel;           /* fb bpp */
    int depth;                  /* depth of default visual */
    MessageType depthFrom;      /* set from config? */
    MessageType bitsPerPixelFrom;       /* set from config? */
    rgb weight;                 /* r/g/b weights */
    rgb mask;                   /* rgb masks */
    rgb offset;                 /* rgb offsets */
    int rgbBits;                /* Number of bits in r/g/b */
    Gamma gamma;                /* Gamma of the monitor */
    int defaultVisual;          /* default visual class */
    int virtualX;               /* Virtual width */
    int virtualY;               /* Virtual height */
    int xInc;                   /* Horizontal timing increment */
    int displayWidth;           /* memory pitch */
    int frameX0;                /* viewport position */
    int frameY0;
    int frameX1;
    int frameY1;
    int zoomLocked;             /* Disallow mode changes */
    DisplayModePtr modePool;    /* list of compatible modes */
    DisplayModePtr modes;       /* list of actual modes */
    DisplayModePtr currentMode; /* current mode
                                 * This was previously
                                 * overloaded with the modes
                                 * field, which is a pointer
                                 * into a circular list */
    confScreenPtr confScreen;   /* Screen config info */
    MonPtr monitor;             /* Monitor information */
    DispPtr display;            /* Display information */
    int *entityList;            /* List of device entities */
    int numEntities;
    int widthmm;                /* physical display dimensions
                                 * in mm */
    int heightmm;
    int xDpi;                   /* width DPI */
    int yDpi;                   /* height DPI */
    const char *name;           /* Name to prefix messages */
    void *driverPrivate;        /* Driver private area */
    DevUnion *privates;         /* Other privates can hook in
                                 * here */
    DriverPtr drv;              /* xf86DriverList[] entry */
    void *module;               /* Pointer to module head */
    int colorKey;
    int overlayFlags;

    /* Some of these may be moved out of here into the driver private area */

    const char *chipset;        /* chipset name */
    const char *ramdac;         /* ramdac name */
    const char *clockchip;      /* clock name */
    Bool progClock;             /* clock is programmable */
    int numClocks;              /* number of clocks */
    int clock[MAXCLOCKS];       /* list of clock frequencies */
    int videoRam;               /* amount of video ram (kb) */
    unsigned long memPhysBase;  /* Physical address of FB */
    unsigned long fbOffset;     /* Offset of FB in the above */
    void *options;

    /* Allow screens to be enabled/disabled individually */
    Bool vtSema;

    /* hw cursor moves from input thread */
    Bool silkenMouse;

    /* Storage for clockRanges and adjustFlags for use with the VidMode ext */
    ClockRangePtr clockRanges;
    int adjustFlags;

    /* initial rightof support disable */
    int                 preferClone;

    Bool is_gpu;
    uint32_t capabilities;

    int *entityInstanceList;
    struct pci_device *vgaDev;

    /*
     * Driver entry points.
     *
     */

    xf86ProbeProc *Probe;
    xf86PreInitProc *PreInit;
    xf86ScreenInitProc *ScreenInit;
    xf86SwitchModeProc *SwitchMode;
    xf86AdjustFrameProc *AdjustFrame;
    xf86EnterVTProc *EnterVT;
    xf86LeaveVTProc *LeaveVT;
    xf86FreeScreenProc *FreeScreen;
    xf86ValidModeProc *ValidMode;
    xf86EnableDisableFBAccessProc *EnableDisableFBAccess;
    xf86SetDGAModeProc *SetDGAMode;
    xf86ChangeGammaProc *ChangeGamma;
    xf86PointerMovedProc *PointerMoved;
    xf86PMEventProc *PMEvent;
    xf86DPMSSetProc *DPMSSet;
    xf86LoadPaletteProc *LoadPalette;
    xf86SetOverscanProc *SetOverscan;
    xorgDriverFuncProc *DriverFunc;
    xf86ModeSetProc *ModeSet;

    int reservedInt[NUM_RESERVED_INTS];
    void *reservedPtr[NUM_RESERVED_POINTERS];
    funcPointer reservedFuncs[NUM_RESERVED_FUNCS];
} ScrnInfoRec;

typedef struct {
    Bool (*OpenFramebuffer) (ScrnInfoPtr pScrn,
                             char **name,
                             unsigned char **mem,
                             int *size, int *offset, int *extra);
    void (*CloseFramebuffer) (ScrnInfoPtr pScrn);
    Bool (*SetMode) (ScrnInfoPtr pScrn, DGAModePtr pMode);
    void (*SetViewport) (ScrnInfoPtr pScrn, int x, int y, int flags);
    int (*GetViewport) (ScrnInfoPtr pScrn);
    void (*Sync) (ScrnInfoPtr);
    void (*FillRect) (ScrnInfoPtr pScrn,
                      int x, int y, int w, int h, unsigned long color);
    void (*BlitRect) (ScrnInfoPtr pScrn,
                      int srcx, int srcy, int w, int h, int dstx, int dsty);
    void (*BlitTransRect) (ScrnInfoPtr pScrn,
                           int srcx, int srcy,
                           int w, int h,
                           int dstx, int dsty, unsigned long color);
} DGAFunctionRec, *DGAFunctionPtr;

typedef struct _SymTabRec {
    int token;                  /* id of the token */
    const char *name;           /* token name */
} SymTabRec, *SymTabPtr;

/* flags for xf86LookupMode */
typedef enum {
    LOOKUP_DEFAULT = 0,         /* Use default mode lookup method */
    LOOKUP_BEST_REFRESH,        /* Pick modes with best refresh */
    LOOKUP_CLOSEST_CLOCK,       /* Pick modes with the closest clock */
    LOOKUP_LIST_ORDER,          /* Pick first useful mode in list */
    LOOKUP_CLKDIV2 = 0x0100,    /* Allow half clocks */
    LOOKUP_OPTIONAL_TOLERANCES = 0x0200 /* Allow missing hsync/vrefresh */
} LookupModeFlags;

#define NoDepth24Support	0x00
#define Support24bppFb		0x01    /* 24bpp framebuffer supported */
#define Support32bppFb		0x02    /* 32bpp framebuffer supported */
#define SupportConvert24to32	0x04    /* Can convert 24bpp pixmap to 32bpp */
#define SupportConvert32to24	0x08    /* Can convert 32bpp pixmap to 24bpp */
#define PreferConvert24to32	0x10    /* prefer 24bpp pixmap to 32bpp conv */
#define PreferConvert32to24	0x20    /* prefer 32bpp pixmap to 24bpp conv */

/* For DPMS */
typedef void (*DPMSSetProcPtr) (ScrnInfoPtr, int, int);

/* Input handler proc */
typedef void (*InputHandlerProc) (int fd, void *data);

/* These are used by xf86GetClocks */
#define CLK_REG_SAVE		-1
#define CLK_REG_RESTORE		-2

/*
 * misc constants
 */
#define INTERLACE_REFRESH_WEIGHT	1.5
#define SYNC_TOLERANCE		0.01    /* 1 percent */
#define CLOCK_TOLERANCE		2000    /* Clock matching tolerance (2MHz) */

#define OVERLAY_8_32_DUALFB	0x00000001
#define OVERLAY_8_24_DUALFB	0x00000002
#define OVERLAY_8_16_DUALFB	0x00000004
#define OVERLAY_8_32_PLANAR	0x00000008

/* Values of xf86Info.mouseFlags */
#define MF_CLEAR_DTR       1
#define MF_CLEAR_RTS       2

/* Action Events */
typedef enum {
    ACTION_TERMINATE = 0,       /* Terminate Server */
    ACTION_NEXT_MODE = 10,      /* Switch to next video mode */
    ACTION_PREV_MODE,
    ACTION_SWITCHSCREEN = 100,  /* VT switch */
    ACTION_SWITCHSCREEN_NEXT,
    ACTION_SWITCHSCREEN_PREV,
} ActionEvent;

#endif                          /* _XF86STR_H */
