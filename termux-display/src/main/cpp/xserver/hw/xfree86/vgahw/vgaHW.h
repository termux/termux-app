
/*
 * Copyright (c) 1997,1998 The XFree86 Project, Inc.
 *
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Author: Dirk Hohndel
 */

#ifndef _VGAHW_H
#define _VGAHW_H

#include <X11/X.h>
#include "misc.h"
#include "input.h"
#include "scrnintstr.h"
#include "colormapst.h"

#include "xf86str.h"
#include "xf86Pci.h"

#include "xf86DDC.h"

#include "globals.h"
#include <X11/extensions/dpmsconst.h>

extern _X_EXPORT int vgaHWGetIndex(void);

/*
 * access macro
 */
#define VGAHWPTR(p) ((vgaHWPtr)((p)->privates[vgaHWGetIndex()].ptr))

/* Standard VGA registers */
#define VGA_ATTR_INDEX		0x3C0
#define VGA_ATTR_DATA_W		0x3C0
#define VGA_ATTR_DATA_R		0x3C1
#define VGA_IN_STAT_0		0x3C2   /* read */
#define VGA_MISC_OUT_W		0x3C2   /* write */
#define VGA_ENABLE		0x3C3
#define VGA_SEQ_INDEX		0x3C4
#define VGA_SEQ_DATA		0x3C5
#define VGA_DAC_MASK		0x3C6
#define VGA_DAC_READ_ADDR	0x3C7
#define VGA_DAC_WRITE_ADDR	0x3C8
#define VGA_DAC_DATA		0x3C9
#define VGA_FEATURE_R		0x3CA   /* read */
#define VGA_MISC_OUT_R		0x3CC   /* read */
#define VGA_GRAPH_INDEX		0x3CE
#define VGA_GRAPH_DATA		0x3CF

#define VGA_IOBASE_MONO		0x3B0
#define VGA_IOBASE_COLOR	0x3D0

#define VGA_CRTC_INDEX_OFFSET	0x04
#define VGA_CRTC_DATA_OFFSET	0x05
#define VGA_IN_STAT_1_OFFSET	0x0A    /* read */
#define VGA_FEATURE_W_OFFSET	0x0A    /* write */

/* default number of VGA registers stored internally */
#define VGA_NUM_CRTC 25
#define VGA_NUM_SEQ 5
#define VGA_NUM_GFX 9
#define VGA_NUM_ATTR 21

/* Flags for vgaHWSave() and vgaHWRestore() */
#define VGA_SR_MODE		0x01
#define VGA_SR_FONTS		0x02
#define VGA_SR_CMAP		0x04
#define VGA_SR_ALL		(VGA_SR_MODE | VGA_SR_FONTS | VGA_SR_CMAP)

/* Defaults for the VGA memory window */
#define VGA_DEFAULT_PHYS_ADDR	0xA0000
#define VGA_DEFAULT_MEM_SIZE	(64 * 1024)

/*
 * vgaRegRec contains settings of standard VGA registers.
 */
typedef struct {
    unsigned char MiscOutReg;   /* */
    unsigned char *CRTC;        /* Crtc Controller */
    unsigned char *Sequencer;   /* Video Sequencer */
    unsigned char *Graphics;    /* Video Graphics */
    unsigned char *Attribute;   /* Video Attribute */
    unsigned char DAC[768];     /* Internal Colorlookuptable */
    unsigned char numCRTC;      /* number of CRTC registers, def=VGA_NUM_CRTC */
    unsigned char numSequencer; /* number of seq registers, def=VGA_NUM_SEQ */
    unsigned char numGraphics;  /* number of gfx registers, def=VGA_NUM_GFX */
    unsigned char numAttribute; /* number of attr registers, def=VGA_NUM_ATTR */
} vgaRegRec, *vgaRegPtr;

typedef struct _vgaHWRec *vgaHWPtr;

typedef void (*vgaHWWriteIndexProcPtr) (vgaHWPtr hwp, CARD8 indx, CARD8 value);
typedef CARD8 (*vgaHWReadIndexProcPtr) (vgaHWPtr hwp, CARD8 indx);
typedef void (*vgaHWWriteProcPtr) (vgaHWPtr hwp, CARD8 value);
typedef CARD8 (*vgaHWReadProcPtr) (vgaHWPtr hwp);
typedef void (*vgaHWMiscProcPtr) (vgaHWPtr hwp);

/*
 * vgaHWRec contains per-screen information required by the vgahw module.
 *
 * Note, the palette referred to by the paletteEnabled, enablePalette and
 * disablePalette is the 16-entry (+overscan) EGA-compatible palette accessed
 * via the first 17 attribute registers and not the main 8-bit palette.
 */
typedef struct _vgaHWRec {
    void *Base;               /* Address of "VGA" memory */
    int MapSize;                /* Size of "VGA" memory */
    unsigned long MapPhys;      /* phys location of VGA mem */
    int IOBase;                 /* I/O Base address */
    CARD8 *MMIOBase;            /* Pointer to MMIO start */
    int MMIOOffset;             /* base + offset + vgareg
                                   = mmioreg */
    void *FontInfo1;          /* save area for fonts in
                                   plane 2 */
    void *FontInfo2;          /* save area for fonts in
                                   plane 3 */
    void *TextInfo;           /* save area for text */
    vgaRegRec SavedReg;         /* saved registers */
    vgaRegRec ModeReg;          /* register settings for
                                   current mode */
    Bool ShowOverscan;
    Bool paletteEnabled;
    Bool cmapSaved;
    ScrnInfoPtr pScrn;
    vgaHWWriteIndexProcPtr writeCrtc;
    vgaHWReadIndexProcPtr readCrtc;
    vgaHWWriteIndexProcPtr writeGr;
    vgaHWReadIndexProcPtr readGr;
    vgaHWReadProcPtr readST00;
    vgaHWReadProcPtr readST01;
    vgaHWReadProcPtr readFCR;
    vgaHWWriteProcPtr writeFCR;
    vgaHWWriteIndexProcPtr writeAttr;
    vgaHWReadIndexProcPtr readAttr;
    vgaHWWriteIndexProcPtr writeSeq;
    vgaHWReadIndexProcPtr readSeq;
    vgaHWWriteProcPtr writeMiscOut;
    vgaHWReadProcPtr readMiscOut;
    vgaHWMiscProcPtr enablePalette;
    vgaHWMiscProcPtr disablePalette;
    vgaHWWriteProcPtr writeDacMask;
    vgaHWReadProcPtr readDacMask;
    vgaHWWriteProcPtr writeDacWriteAddr;
    vgaHWWriteProcPtr writeDacReadAddr;
    vgaHWWriteProcPtr writeDacData;
    vgaHWReadProcPtr readDacData;
    void *ddc;
    struct pci_io_handle *io;
    vgaHWReadProcPtr readEnable;
    vgaHWWriteProcPtr writeEnable;
    struct pci_device *dev;
} vgaHWRec;

/* Some macros that VGA drivers can use in their ChipProbe() function */
#define OVERSCAN 0x11           /* Index of OverScan register */

/* Flags that define how overscan correction should take place */
#define KGA_FIX_OVERSCAN  1     /* overcan correction required */
#define KGA_ENABLE_ON_ZERO 2    /* if possible enable display at beginning */
                              /* of next scanline/frame                  */
#define KGA_BE_TOT_DEC 4        /* always fix problem by setting blank end */
                              /* to total - 1                            */
#define BIT_PLANE 3             /* Which plane we write to in mono mode */
#define BITS_PER_GUN 6
#define COLORMAP_SIZE 256

#define DACDelay(hw) \
	do { \
	    (hw)->readST01((hw)); \
	    (hw)->readST01((hw)); \
	} while (0)

/* Function Prototypes */

/* vgaHW.c */

typedef void vgaHWProtectProc(ScrnInfoPtr, Bool);
typedef void vgaHWBlankScreenProc(ScrnInfoPtr, Bool);

extern _X_EXPORT void vgaHWSetStdFuncs(vgaHWPtr hwp);
extern _X_EXPORT void vgaHWSetMmioFuncs(vgaHWPtr hwp, CARD8 *base, int offset);
extern _X_EXPORT void vgaHWProtect(ScrnInfoPtr pScrn, Bool on);
extern _X_EXPORT vgaHWProtectProc *vgaHWProtectWeak(void);
extern _X_EXPORT Bool vgaHWSaveScreen(ScreenPtr pScreen, int mode);
extern _X_EXPORT void vgaHWBlankScreen(ScrnInfoPtr pScrn, Bool on);
extern _X_EXPORT vgaHWBlankScreenProc *vgaHWBlankScreenWeak(void);
extern _X_EXPORT void vgaHWSeqReset(vgaHWPtr hwp, Bool start);
extern _X_EXPORT void vgaHWRestoreFonts(ScrnInfoPtr scrninfp,
                                        vgaRegPtr restore);
extern _X_EXPORT void vgaHWRestoreMode(ScrnInfoPtr scrninfp, vgaRegPtr restore);
extern _X_EXPORT void vgaHWRestoreColormap(ScrnInfoPtr scrninfp,
                                           vgaRegPtr restore);
extern _X_EXPORT void vgaHWRestore(ScrnInfoPtr scrninfp, vgaRegPtr restore,
                                   int flags);
extern _X_EXPORT void vgaHWSaveFonts(ScrnInfoPtr scrninfp, vgaRegPtr save);
extern _X_EXPORT void vgaHWSaveMode(ScrnInfoPtr scrninfp, vgaRegPtr save);
extern _X_EXPORT void vgaHWSaveColormap(ScrnInfoPtr scrninfp, vgaRegPtr save);
extern _X_EXPORT void vgaHWSave(ScrnInfoPtr scrninfp, vgaRegPtr save,
                                int flags);
extern _X_EXPORT Bool vgaHWInit(ScrnInfoPtr scrnp, DisplayModePtr mode);
extern _X_EXPORT Bool vgaHWSetRegCounts(ScrnInfoPtr scrp, int numCRTC,
                                        int numSequencer, int numGraphics,
                                        int numAttribute);
extern _X_EXPORT Bool vgaHWCopyReg(vgaRegPtr dst, vgaRegPtr src);
extern _X_EXPORT Bool vgaHWGetHWRec(ScrnInfoPtr scrp);
extern _X_EXPORT void vgaHWFreeHWRec(ScrnInfoPtr scrp);
extern _X_EXPORT Bool vgaHWMapMem(ScrnInfoPtr scrp);
extern _X_EXPORT void vgaHWUnmapMem(ScrnInfoPtr scrp);
extern _X_EXPORT void vgaHWGetIOBase(vgaHWPtr hwp);
extern _X_EXPORT void vgaHWLock(vgaHWPtr hwp);
extern _X_EXPORT void vgaHWUnlock(vgaHWPtr hwp);
extern _X_EXPORT void vgaHWEnable(vgaHWPtr hwp);
extern _X_EXPORT void vgaHWDisable(vgaHWPtr hwp);
extern _X_EXPORT void vgaHWDPMSSet(ScrnInfoPtr pScrn, int PowerManagementMode,
                                   int flags);
extern _X_EXPORT Bool vgaHWHandleColormaps(ScreenPtr pScreen);
extern _X_EXPORT void vgaHWddc1SetSpeed(ScrnInfoPtr pScrn, xf86ddcSpeed speed);
extern _X_EXPORT CARD32 vgaHWHBlankKGA(DisplayModePtr mode, vgaRegPtr regp,
                                       int nBits, unsigned int Flags);
extern _X_EXPORT CARD32 vgaHWVBlankKGA(DisplayModePtr mode, vgaRegPtr regp,
                                       int nBits, unsigned int Flags);
extern _X_EXPORT Bool vgaHWAllocDefaultRegs(vgaRegPtr regp);

extern _X_EXPORT DDC1SetSpeedProc vgaHWddc1SetSpeedWeak(void);
extern _X_EXPORT SaveScreenProcPtr vgaHWSaveScreenWeak(void);
extern _X_EXPORT void xf86GetClocks(ScrnInfoPtr pScrn, int num,
                                    Bool (*ClockFunc) (ScrnInfoPtr, int),
                                    void (*ProtectRegs) (ScrnInfoPtr, Bool),
                                    void (*BlankScreen) (ScrnInfoPtr, Bool),
                                    unsigned long vertsyncreg, int maskval,
                                    int knownclkindex, int knownclkvalue);

#endif                          /* _VGAHW_H */
