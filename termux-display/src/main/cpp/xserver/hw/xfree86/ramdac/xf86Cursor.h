
#ifndef _XF86CURSOR_H
#define _XF86CURSOR_H

#include "xf86str.h"
#include "mipointer.h"

typedef struct _xf86CursorInfoRec {
    ScrnInfoPtr pScrn;
    int Flags;
    int MaxWidth;
    int MaxHeight;
    void (*SetCursorColors) (ScrnInfoPtr pScrn, int bg, int fg);
    void (*SetCursorPosition) (ScrnInfoPtr pScrn, int x, int y);
    void (*LoadCursorImage) (ScrnInfoPtr pScrn, unsigned char *bits);
    Bool (*LoadCursorImageCheck) (ScrnInfoPtr pScrn, unsigned char *bits);
    void (*HideCursor) (ScrnInfoPtr pScrn);
    void (*ShowCursor) (ScrnInfoPtr pScrn);
    Bool (*ShowCursorCheck) (ScrnInfoPtr pScrn);
    unsigned char *(*RealizeCursor) (struct _xf86CursorInfoRec *, CursorPtr);
    Bool (*UseHWCursor) (ScreenPtr, CursorPtr);

    Bool (*UseHWCursorARGB) (ScreenPtr, CursorPtr);
    void (*LoadCursorARGB) (ScrnInfoPtr, CursorPtr);
    Bool (*LoadCursorARGBCheck) (ScrnInfoPtr, CursorPtr);

} xf86CursorInfoRec, *xf86CursorInfoPtr;

static inline Bool
xf86DriverHasLoadCursorImage(xf86CursorInfoPtr infoPtr)
{
    return infoPtr->LoadCursorImageCheck || infoPtr->LoadCursorImage;
}

static inline Bool
xf86DriverLoadCursorImage(xf86CursorInfoPtr infoPtr, unsigned char *bits)
{
    if(infoPtr->LoadCursorImageCheck)
        return infoPtr->LoadCursorImageCheck(infoPtr->pScrn, bits);
    infoPtr->LoadCursorImage(infoPtr->pScrn, bits);
    return TRUE;
}

static inline Bool
xf86DriverHasShowCursor(xf86CursorInfoPtr infoPtr)
{
    return infoPtr->ShowCursorCheck || infoPtr->ShowCursor;
}

static inline Bool
xf86DriverShowCursor(xf86CursorInfoPtr infoPtr)
{
    if(infoPtr->ShowCursorCheck)
        return infoPtr->ShowCursorCheck(infoPtr->pScrn);
    infoPtr->ShowCursor(infoPtr->pScrn);
    return TRUE;
}

static inline Bool
xf86DriverHasLoadCursorARGB(xf86CursorInfoPtr infoPtr)
{
    return infoPtr->LoadCursorARGBCheck || infoPtr->LoadCursorARGB;
}

static inline Bool
xf86DriverLoadCursorARGB(xf86CursorInfoPtr infoPtr, CursorPtr pCursor)
{
    if(infoPtr->LoadCursorARGBCheck)
        return infoPtr->LoadCursorARGBCheck(infoPtr->pScrn, pCursor);
    infoPtr->LoadCursorARGB(infoPtr->pScrn, pCursor);
    return TRUE;
}

extern _X_EXPORT Bool xf86InitCursor(ScreenPtr pScreen,
                                     xf86CursorInfoPtr infoPtr);
extern _X_EXPORT xf86CursorInfoPtr xf86CreateCursorInfoRec(void);
extern _X_EXPORT void xf86DestroyCursorInfoRec(xf86CursorInfoPtr);
extern _X_EXPORT void xf86CursorResetCursor(ScreenPtr pScreen);
extern _X_EXPORT void xf86ForceHWCursor(ScreenPtr pScreen, Bool on);
extern _X_EXPORT CursorPtr xf86CurrentCursor(ScreenPtr pScreen);

#define HARDWARE_CURSOR_INVERT_MASK 			0x00000001
#define HARDWARE_CURSOR_AND_SOURCE_WITH_MASK		0x00000002
#define HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK		0x00000004
#define HARDWARE_CURSOR_SOURCE_MASK_NOT_INTERLEAVED	0x00000008
#define HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1	0x00000010
#define HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_8	0x00000020
#define HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_16	0x00000040
#define HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_32	0x00000080
#define HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64	0x00000100
#define HARDWARE_CURSOR_TRUECOLOR_AT_8BPP		0x00000200
#define HARDWARE_CURSOR_BIT_ORDER_MSBFIRST		0x00000400
#define HARDWARE_CURSOR_NIBBLE_SWAPPED			0x00000800
#define HARDWARE_CURSOR_SHOW_TRANSPARENT		0x00001000
#define HARDWARE_CURSOR_UPDATE_UNHIDDEN			0x00002000
#define HARDWARE_CURSOR_ARGB				0x00004000

#endif                          /* _XF86CURSOR_H */
