
/*
 *                   XFree86 vbe module
 *               Copyright 2000 Egbert Eich
 *
 * The mode query/save/set/restore functions from the vesa driver
 * have been moved here.
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
 * Authors: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 */

#ifndef _VBE_H
#define _VBE_H
#include "xf86int10.h"
#include "xf86DDC.h"

typedef enum {
    DDC_UNCHECKED,
    DDC_NONE,
    DDC_1,
    DDC_2,
    DDC_1_2
} ddc_lvl;

typedef struct {
    xf86Int10InfoPtr pInt10;
    int version;
    void *memory;
    int real_mode_base;
    int num_pages;
    Bool init_int10;
    ddc_lvl ddc;
    Bool ddc_blank;
} vbeInfoRec, *vbeInfoPtr;

#define VBE_VERSION_MAJOR(x) *((CARD8*)(&x) + 1)
#define VBE_VERSION_MINOR(x) (CARD8)(x)

extern _X_EXPORT vbeInfoPtr VBEInit(xf86Int10InfoPtr pInt, int entityIndex);
extern _X_EXPORT vbeInfoPtr VBEExtendedInit(xf86Int10InfoPtr pInt,
                                            int entityIndex, int Flags);
extern _X_EXPORT void vbeFree(vbeInfoPtr pVbe);
extern _X_EXPORT xf86MonPtr vbeDoEDID(vbeInfoPtr pVbe, void *pDDCModule);

#pragma pack(1)

typedef struct vbeControllerInfoBlock {
    CARD8 VbeSignature[4];
    CARD16 VbeVersion;
    CARD32 OemStringPtr;
    CARD8 Capabilities[4];
    CARD32 VideoModePtr;
    CARD16 TotalMem;
    CARD16 OemSoftwareRev;
    CARD32 OemVendorNamePtr;
    CARD32 OemProductNamePtr;
    CARD32 OemProductRevPtr;
    CARD8 Scratch[222];
    CARD8 OemData[256];
} vbeControllerInfoRec, *vbeControllerInfoPtr;

#if defined(__GNUC__) || defined(__USLC__) || defined(__SUNPRO_C)
#pragma pack()                  /* All GCC versions recognise this syntax */
#else
#pragma pack(0)
#endif

#if !( defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)) )
#define __attribute__(a)
#endif

typedef struct _VbeInfoBlock VbeInfoBlock;
typedef struct _VbeModeInfoBlock VbeModeInfoBlock;
typedef struct _VbeCRTCInfoBlock VbeCRTCInfoBlock;

/*
 * INT 0
 */

struct _VbeInfoBlock {
    /* VESA 1.2 fields */
    CARD8 VESASignature[4];     /* VESA */
    CARD16 VESAVersion;         /* Higher byte major, lower byte minor */
                                        /*CARD32 */ char *OEMStringPtr;
                                        /* Pointer to OEM string */
    CARD8 Capabilities[4];      /* Capabilities of the video environment */

                                        /*CARD32 */ CARD16 *VideoModePtr;
                                        /* pointer to supported Super VGA modes */

    CARD16 TotalMemory;         /* Number of 64kb memory blocks on board */
    /* if not VESA 2, 236 scratch bytes follow (256 bytes total size) */

    /* VESA 2 fields */
    CARD16 OemSoftwareRev;      /* VBE implementation Software revision */
                                        /*CARD32 */ char *OemVendorNamePtr;
                                        /* Pointer to Vendor Name String */
                                                /*CARD32 */ char *OemProductNamePtr;
                                                /* Pointer to Product Name String */
                                        /*CARD32 */ char *OemProductRevPtr;
                                        /* Pointer to Product Revision String */
    CARD8 Reserved[222];        /* Reserved for VBE implementation */
    CARD8 OemData[256];         /* Data Area for OEM Strings */
} __attribute__ ((packed));

/* Return Super VGA Information */
extern _X_EXPORT VbeInfoBlock *VBEGetVBEInfo(vbeInfoPtr pVbe);
extern _X_EXPORT void VBEFreeVBEInfo(VbeInfoBlock * block);

/*
 * INT 1
 */

struct _VbeModeInfoBlock {
    CARD16 ModeAttributes;      /* mode attributes */
    CARD8 WinAAttributes;       /* window A attributes */
    CARD8 WinBAttributes;       /* window B attributes */
    CARD16 WinGranularity;      /* window granularity */
    CARD16 WinSize;             /* window size */
    CARD16 WinASegment;         /* window A start segment */
    CARD16 WinBSegment;         /* window B start segment */
    CARD32 WinFuncPtr;          /* real mode pointer to window function */
    CARD16 BytesPerScanline;    /* bytes per scanline */

    /* Mandatory information for VBE 1.2 and above */
    CARD16 XResolution;         /* horizontal resolution in pixels or characters */
    CARD16 YResolution;         /* vertical resolution in pixels or characters */
    CARD8 XCharSize;            /* character cell width in pixels */
    CARD8 YCharSize;            /* character cell height in pixels */
    CARD8 NumberOfPlanes;       /* number of memory planes */
    CARD8 BitsPerPixel;         /* bits per pixel */
    CARD8 NumberOfBanks;        /* number of banks */
    CARD8 MemoryModel;          /* memory model type */
    CARD8 BankSize;             /* bank size in KB */
    CARD8 NumberOfImages;       /* number of images */
    CARD8 Reserved;             /* 1 *//* reserved for page function */

    /* Direct color fields (required for direct/6 and YUV/7 memory models) */
    CARD8 RedMaskSize;          /* size of direct color red mask in bits */
    CARD8 RedFieldPosition;     /* bit position of lsb of red mask */
    CARD8 GreenMaskSize;        /* size of direct color green mask in bits */
    CARD8 GreenFieldPosition;   /* bit position of lsb of green mask */
    CARD8 BlueMaskSize;         /* size of direct color blue mask in bits */
    CARD8 BlueFieldPosition;    /* bit position of lsb of blue mask */
    CARD8 RsvdMaskSize;         /* size of direct color reserved mask in bits */
    CARD8 RsvdFieldPosition;    /* bit position of lsb of reserved mask */
    CARD8 DirectColorModeInfo;  /* direct color mode attributes */

    /* Mandatory information for VBE 2.0 and above */
    CARD32 PhysBasePtr;         /* physical address for flat memory frame buffer */
    CARD32 Reserved32;          /* 0 *//* Reserved - always set to 0 */
    CARD16 Reserved16;          /* 0 *//* Reserved - always set to 0 */

    /* Mandatory information for VBE 3.0 and above */
    CARD16 LinBytesPerScanLine; /* bytes per scan line for linear modes */
    CARD8 BnkNumberOfImagePages;        /* number of images for banked modes */
    CARD8 LinNumberOfImagePages;        /* number of images for linear modes */
    CARD8 LinRedMaskSize;       /* size of direct color red mask (linear modes) */
    CARD8 LinRedFieldPosition;  /* bit position of lsb of red mask (linear modes) */
    CARD8 LinGreenMaskSize;     /* size of direct color green mask (linear modes) */
    CARD8 LinGreenFieldPosition;        /* bit position of lsb of green mask (linear modes) */
    CARD8 LinBlueMaskSize;      /* size of direct color blue mask (linear modes) */
    CARD8 LinBlueFieldPosition; /* bit position of lsb of blue mask (linear modes) */
    CARD8 LinRsvdMaskSize;      /* size of direct color reserved mask (linear modes) */
    CARD8 LinRsvdFieldPosition; /* bit position of lsb of reserved mask (linear modes) */
    CARD32 MaxPixelClock;       /* maximum pixel clock (in Hz) for graphics mode */
    CARD8 Reserved2[189];       /* remainder of VbeModeInfoBlock */
} __attribute__ ((packed));

/* Return VBE Mode Information */
extern _X_EXPORT VbeModeInfoBlock *VBEGetModeInfo(vbeInfoPtr pVbe, int mode);
extern _X_EXPORT void VBEFreeModeInfo(VbeModeInfoBlock * block);

/*
 * INT2
 */

#define CRTC_DBLSCAN	(1<<0)
#define CRTC_INTERLACE	(1<<1)
#define CRTC_NHSYNC	(1<<2)
#define CRTC_NVSYNC	(1<<3)

struct _VbeCRTCInfoBlock {
    CARD16 HorizontalTotal;     /* Horizontal total in pixels */
    CARD16 HorizontalSyncStart; /* Horizontal sync start in pixels */
    CARD16 HorizontalSyncEnd;   /* Horizontal sync end in pixels */
    CARD16 VerticalTotal;       /* Vertical total in lines */
    CARD16 VerticalSyncStart;   /* Vertical sync start in lines */
    CARD16 VerticalSyncEnd;     /* Vertical sync end in lines */
    CARD8 Flags;                /* Flags (Interlaced, Double Scan etc) */
    CARD32 PixelClock;          /* Pixel clock in units of Hz */
    CARD16 RefreshRate;         /* Refresh rate in units of 0.01 Hz */
    CARD8 Reserved[40];         /* remainder of ModeInfoBlock */
} __attribute__ ((packed));

/* VbeCRTCInfoBlock is in the VESA 3.0 specs */

extern _X_EXPORT Bool VBESetVBEMode(vbeInfoPtr pVbe, int mode,
                                    VbeCRTCInfoBlock * crtc);

/*
 * INT 3
 */

extern _X_EXPORT Bool VBEGetVBEMode(vbeInfoPtr pVbe, int *mode);

/*
 * INT 4
 */

/* Save/Restore Super VGA video state */
/* function values are (values stored in VESAPtr):
 *	0 := query & allocate amount of memory to save state
 *	1 := save state
 *	2 := restore state
 *
 *	function 0 called automatically if function 1 called without
 *	a previous call to function 0.
 */

typedef enum {
    MODE_QUERY,
    MODE_SAVE,
    MODE_RESTORE
} vbeSaveRestoreFunction;

extern _X_EXPORT Bool
VBESaveRestore(vbeInfoPtr pVbe, vbeSaveRestoreFunction function,
               void **memory, int *size, int *real_mode_pages);

/*
 * INT 5
 */

extern _X_EXPORT Bool
 VBEBankSwitch(vbeInfoPtr pVbe, unsigned int iBank, int window);

/*
 * INT 6
 */

typedef enum {
    SCANWID_SET,
    SCANWID_GET,
    SCANWID_SET_BYTES,
    SCANWID_GET_MAX
} vbeScanwidthCommand;

#define VBESetLogicalScanline(pVbe, width)	\
	VBESetGetLogicalScanlineLength(pVbe, SCANWID_SET, width, \
					NULL, NULL, NULL)
#define VBESetLogicalScanlineBytes(pVbe, width)	\
	VBESetGetLogicalScanlineLength(pVbe, SCANWID_SET_BYTES, width, \
					NULL, NULL, NULL)
#define VBEGetLogicalScanline(pVbe, pixels, bytes, max)	\
	VBESetGetLogicalScanlineLength(pVbe, SCANWID_GET, 0, \
					pixels, bytes, max)
#define VBEGetMaxLogicalScanline(pVbe, pixels, bytes, max)	\
	VBESetGetLogicalScanlineLength(pVbe, SCANWID_GET_MAX, 0, \
					pixels, bytes, max)
extern _X_EXPORT Bool VBESetGetLogicalScanlineLength(vbeInfoPtr pVbe,
                                                     vbeScanwidthCommand
                                                     command, int width,
                                                     int *pixels, int *bytes,
                                                     int *max);

/*
 * INT 7
 */

/* 16 bit code */
extern _X_EXPORT Bool VBESetDisplayStart(vbeInfoPtr pVbe, int x, int y,
                                         Bool wait_retrace);
extern _X_EXPORT Bool VBEGetDisplayStart(vbeInfoPtr pVbe, int *x, int *y);

/*
 * INT 8
 */

/* if bits is 0, then it is a GET */
extern _X_EXPORT int VBESetGetDACPaletteFormat(vbeInfoPtr pVbe, int bits);

/*
 * INT 9
 */

/*
 *  If getting a palette, the data argument is not used. It will return
 * the data.
 *  If setting a palette, it will return the pointer received on success,
 * NULL on failure.
 */
extern _X_EXPORT CARD32 *VBESetGetPaletteData(vbeInfoPtr pVbe, Bool set,
                                              int first, int num, CARD32 *data,
                                              Bool secondary,
                                              Bool wait_retrace);
#define VBEFreePaletteData(data)	free(data)

/*
 * INT A
 */

typedef struct _VBEpmi {
    int seg_tbl;
    int tbl_off;
    int tbl_len;
} VBEpmi;

extern _X_EXPORT VBEpmi *VBEGetVBEpmi(vbeInfoPtr pVbe);

#define VESAFreeVBEpmi(pmi)	free(pmi)

/* high level helper functions */

typedef struct _vbeModeInfoRec {
    int width;
    int height;
    int bpp;
    int n;
    struct _vbeModeInfoRec *next;
} vbeModeInfoRec, *vbeModeInfoPtr;

typedef struct {
    CARD8 *state;
    CARD8 *pstate;
    int statePage;
    int stateSize;
    int stateMode;
} vbeSaveRestoreRec, *vbeSaveRestorePtr;

extern _X_EXPORT void

VBEVesaSaveRestore(vbeInfoPtr pVbe, vbeSaveRestorePtr vbe_sr,
                   vbeSaveRestoreFunction function);

extern _X_EXPORT int VBEGetPixelClock(vbeInfoPtr pVbe, int mode, int Clock);
extern _X_EXPORT Bool VBEDPMSSet(vbeInfoPtr pVbe, int mode);

struct vbePanelID {
    short hsize;
    short vsize;
    short fptype;
    char redbpp;
    char greenbpp;
    char bluebpp;
    char reservedbpp;
    int reserved_offscreen_mem_size;
    int reserved_offscreen_mem_pointer;
    char reserved[14];
};

extern _X_EXPORT void VBEInterpretPanelID(ScrnInfoPtr pScrn,
                                          struct vbePanelID *data);
extern _X_EXPORT struct vbePanelID *VBEReadPanelID(vbeInfoPtr pVbe);

#endif
