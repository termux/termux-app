
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <string.h>

#include "misc.h"
#include "xf86.h"
#include "xf86_OSproc.h"

#include <X11/X.h>
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "xf86str.h"
#include "cursorstr.h"
#include "mi.h"
#include "mipointer.h"
#include "randrstr.h"
#include "xf86CursorPriv.h"

#include "servermd.h"

static void
xf86RecolorCursor_locked(xf86CursorScreenPtr ScreenPriv, CursorPtr pCurs);

static CARD32
xf86ReverseBitOrder(CARD32 v)
{
    return (((0x01010101 & v) << 7) | ((0x02020202 & v) << 5) |
            ((0x04040404 & v) << 3) | ((0x08080808 & v) << 1) |
            ((0x10101010 & v) >> 1) | ((0x20202020 & v) >> 3) |
            ((0x40404040 & v) >> 5) | ((0x80808080 & v) >> 7));
}

#if BITMAP_SCANLINE_PAD == 64

#if 1
/* Cursors might be only 32 wide. Give'em a chance */
#define SCANLINE CARD32
#define CUR_BITMAP_SCANLINE_PAD 32
#define CUR_LOG2_BITMAP_PAD 5
#define REVERSE_BIT_ORDER(w) xf86ReverseBitOrder(w)
#else
#define SCANLINE CARD64
#define CUR_BITMAP_SCANLINE_PAD BITMAP_SCANLINE_PAD
#define CUR_LOG2_BITMAP_PAD LOG2_BITMAP_PAD
#define REVERSE_BIT_ORDER(w) xf86CARD64ReverseBits(w)
static CARD64 xf86CARD64ReverseBits(CARD64 w);

static CARD64
xf86CARD64ReverseBits(CARD64 w)
{
    unsigned char *p = (unsigned char *) &w;

    p[0] = byte_reversed[p[0]];
    p[1] = byte_reversed[p[1]];
    p[2] = byte_reversed[p[2]];
    p[3] = byte_reversed[p[3]];
    p[4] = byte_reversed[p[4]];
    p[5] = byte_reversed[p[5]];
    p[6] = byte_reversed[p[6]];
    p[7] = byte_reversed[p[7]];

    return w;
}
#endif

#else

#define SCANLINE CARD32
#define CUR_BITMAP_SCANLINE_PAD BITMAP_SCANLINE_PAD
#define CUR_LOG2_BITMAP_PAD LOG2_BITMAP_PAD
#define REVERSE_BIT_ORDER(w) xf86ReverseBitOrder(w)

#endif                          /* BITMAP_SCANLINE_PAD == 64 */

static unsigned char *RealizeCursorInterleave0(xf86CursorInfoPtr, CursorPtr);
static unsigned char *RealizeCursorInterleave1(xf86CursorInfoPtr, CursorPtr);
static unsigned char *RealizeCursorInterleave8(xf86CursorInfoPtr, CursorPtr);
static unsigned char *RealizeCursorInterleave16(xf86CursorInfoPtr, CursorPtr);
static unsigned char *RealizeCursorInterleave32(xf86CursorInfoPtr, CursorPtr);
static unsigned char *RealizeCursorInterleave64(xf86CursorInfoPtr, CursorPtr);

Bool
xf86InitHardwareCursor(ScreenPtr pScreen, xf86CursorInfoPtr infoPtr)
{
    if ((infoPtr->MaxWidth <= 0) || (infoPtr->MaxHeight <= 0))
        return FALSE;

    /* These are required for now */
    if (!infoPtr->SetCursorPosition ||
        !xf86DriverHasLoadCursorImage(infoPtr) ||
        !infoPtr->HideCursor ||
        !xf86DriverHasShowCursor(infoPtr) ||
        !infoPtr->SetCursorColors)
        return FALSE;

    if (infoPtr->RealizeCursor) {
        /* Don't overwrite a driver provided Realize Cursor function */
    }
    else if (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1 & infoPtr->Flags) {
        infoPtr->RealizeCursor = RealizeCursorInterleave1;
    }
    else if (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_8 & infoPtr->Flags) {
        infoPtr->RealizeCursor = RealizeCursorInterleave8;
    }
    else if (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_16 & infoPtr->Flags) {
        infoPtr->RealizeCursor = RealizeCursorInterleave16;
    }
    else if (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_32 & infoPtr->Flags) {
        infoPtr->RealizeCursor = RealizeCursorInterleave32;
    }
    else if (HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 & infoPtr->Flags) {
        infoPtr->RealizeCursor = RealizeCursorInterleave64;
    }
    else {                      /* not interleaved */
        infoPtr->RealizeCursor = RealizeCursorInterleave0;
    }

    infoPtr->pScrn = xf86ScreenToScrn(pScreen);

    return TRUE;
}

static Bool
xf86ScreenCheckHWCursor(ScreenPtr pScreen, CursorPtr cursor, xf86CursorInfoPtr infoPtr)
{
    return
        (cursor->bits->argb && infoPtr->UseHWCursorARGB &&
         infoPtr->UseHWCursorARGB(pScreen, cursor)) ||
        (cursor->bits->argb == 0 &&
         cursor->bits->height <= infoPtr->MaxHeight &&
         cursor->bits->width <= infoPtr->MaxWidth &&
         (!infoPtr->UseHWCursor || infoPtr->UseHWCursor(pScreen, cursor)));
}

Bool
xf86CheckHWCursor(ScreenPtr pScreen, CursorPtr cursor, xf86CursorInfoPtr infoPtr)
{
    ScreenPtr pSlave;
    Bool use_hw_cursor = TRUE;

    input_lock();

    if (!xf86ScreenCheckHWCursor(pScreen, cursor, infoPtr)) {
        use_hw_cursor = FALSE;
	goto unlock;
    }

    /* ask each driver consuming a pixmap if it can support HW cursor */
    xorg_list_for_each_entry(pSlave, &pScreen->secondary_list, secondary_head) {
        xf86CursorScreenPtr sPriv;

        if (!RRHasScanoutPixmap(pSlave))
            continue;

        sPriv = dixLookupPrivate(&pSlave->devPrivates, xf86CursorScreenKey);
        if (!sPriv) { /* NULL if Option "SWCursor", possibly other conditions */
            use_hw_cursor = FALSE;
	    break;
	}

        /* FALSE if HWCursor not supported by secondary */
        if (!xf86ScreenCheckHWCursor(pSlave, cursor, sPriv->CursorInfoPtr)) {
            use_hw_cursor = FALSE;
	    break;
	}
    }

unlock:
    input_unlock();

    return use_hw_cursor;
}

static Bool
xf86ScreenSetCursor(ScreenPtr pScreen, CursorPtr pCurs, int x, int y)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    xf86CursorInfoPtr infoPtr;
    unsigned char *bits;

    if (!ScreenPriv) { /* NULL if Option "SWCursor" */
        return (pCurs == NullCursor);
    }

    infoPtr = ScreenPriv->CursorInfoPtr;

    if (pCurs == NullCursor) {
        (*infoPtr->HideCursor) (infoPtr->pScrn);
        return TRUE;
    }

    /*
     * Hot plugged GPU's do not have a CursorScreenKey, force sw cursor.
     * This check can be removed once dix/privates.c gets relocation code for
     * PRIVATE_CURSOR. Also see the related comment in AddGPUScreen().
     */
    if (!_dixGetScreenPrivateKey(CursorScreenKey, pScreen))
        return FALSE;

    bits =
        dixLookupScreenPrivate(&pCurs->devPrivates, CursorScreenKey, pScreen);

    x -= infoPtr->pScrn->frameX0;
    y -= infoPtr->pScrn->frameY0;

    if (!pCurs->bits->argb || !xf86DriverHasLoadCursorARGB(infoPtr))
        if (!bits) {
            bits = (*infoPtr->RealizeCursor) (infoPtr, pCurs);
            dixSetScreenPrivate(&pCurs->devPrivates, CursorScreenKey, pScreen,
                                bits);
        }

    if (!(infoPtr->Flags & HARDWARE_CURSOR_UPDATE_UNHIDDEN))
        (*infoPtr->HideCursor) (infoPtr->pScrn);

    if (pCurs->bits->argb && xf86DriverHasLoadCursorARGB(infoPtr)) {
        if (!xf86DriverLoadCursorARGB (infoPtr, pCurs))
            return FALSE;
    } else
    if (bits)
        if (!xf86DriverLoadCursorImage (infoPtr, bits))
            return FALSE;

    xf86RecolorCursor_locked (ScreenPriv, pCurs);

    (*infoPtr->SetCursorPosition) (infoPtr->pScrn, x, y);

    return xf86DriverShowCursor(infoPtr);
}

Bool
xf86SetCursor(ScreenPtr pScreen, CursorPtr pCurs, int x, int y)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);
    ScreenPtr pSlave;
    Bool ret = FALSE;

    input_lock();

    x -= ScreenPriv->HotX;
    y -= ScreenPriv->HotY;

    if (!xf86ScreenSetCursor(pScreen, pCurs, x, y))
        goto out;

    /* ask each secondary driver to set the cursor. */
    xorg_list_for_each_entry(pSlave, &pScreen->secondary_list, secondary_head) {
        if (!RRHasScanoutPixmap(pSlave))
            continue;

        if (!xf86ScreenSetCursor(pSlave, pCurs, x, y)) {
            /*
             * hide the primary (and successfully set secondary) cursors,
             * otherwise both the hw and sw cursor will show.
             */
            xf86SetCursor(pScreen, NullCursor, x, y);
            goto out;
        }
    }
    ret = TRUE;

 out:
    input_unlock();
    return ret;
}

void
xf86SetTransparentCursor(ScreenPtr pScreen)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);
    xf86CursorInfoPtr infoPtr = ScreenPriv->CursorInfoPtr;

    input_lock();

    if (!ScreenPriv->transparentData)
        ScreenPriv->transparentData =
            (*infoPtr->RealizeCursor) (infoPtr, NullCursor);

    if (!(infoPtr->Flags & HARDWARE_CURSOR_UPDATE_UNHIDDEN))
        (*infoPtr->HideCursor) (infoPtr->pScrn);

    if (ScreenPriv->transparentData)
        xf86DriverLoadCursorImage (infoPtr,
                                   ScreenPriv->transparentData);

    xf86DriverShowCursor(infoPtr);

    input_unlock();
}

static void
xf86ScreenMoveCursor(ScreenPtr pScreen, int x, int y)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);
    xf86CursorInfoPtr infoPtr = ScreenPriv->CursorInfoPtr;

    x -= infoPtr->pScrn->frameX0;
    y -= infoPtr->pScrn->frameY0;

    (*infoPtr->SetCursorPosition) (infoPtr->pScrn, x, y);
}

void
xf86MoveCursor(ScreenPtr pScreen, int x, int y)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);
    ScreenPtr pSlave;

    input_lock();

    x -= ScreenPriv->HotX;
    y -= ScreenPriv->HotY;

    xf86ScreenMoveCursor(pScreen, x, y);

    /* ask each secondary driver to move the cursor */
    xorg_list_for_each_entry(pSlave, &pScreen->secondary_list, secondary_head) {
        if (!RRHasScanoutPixmap(pSlave))
            continue;

        xf86ScreenMoveCursor(pSlave, x, y);
    }

    input_unlock();
}

static void
xf86RecolorCursor_locked(xf86CursorScreenPtr ScreenPriv, CursorPtr pCurs)
{
    xf86CursorInfoPtr infoPtr = ScreenPriv->CursorInfoPtr;

    /* recoloring isn't applicable to ARGB cursors and drivers
       shouldn't have to ignore SetCursorColors requests */
    if (pCurs->bits->argb)
        return;

    if (ScreenPriv->PalettedCursor) {
        xColorItem sourceColor, maskColor;
        ColormapPtr pmap = ScreenPriv->pInstalledMap;

        if (!pmap)
            return;

        sourceColor.red = pCurs->foreRed;
        sourceColor.green = pCurs->foreGreen;
        sourceColor.blue = pCurs->foreBlue;
        FakeAllocColor(pmap, &sourceColor);
        maskColor.red = pCurs->backRed;
        maskColor.green = pCurs->backGreen;
        maskColor.blue = pCurs->backBlue;
        FakeAllocColor(pmap, &maskColor);
        FakeFreeColor(pmap, sourceColor.pixel);
        FakeFreeColor(pmap, maskColor.pixel);
        (*infoPtr->SetCursorColors) (infoPtr->pScrn,
                                     maskColor.pixel, sourceColor.pixel);
    }
    else {                      /* Pass colors in 8-8-8 RGB format */
        (*infoPtr->SetCursorColors) (infoPtr->pScrn,
                                     (pCurs->backBlue >> 8) |
                                     ((pCurs->backGreen >> 8) << 8) |
                                     ((pCurs->backRed >> 8) << 16),
                                     (pCurs->foreBlue >> 8) |
                                     ((pCurs->foreGreen >> 8) << 8) |
                                     ((pCurs->foreRed >> 8) << 16)
            );
    }
}

void
xf86RecolorCursor(ScreenPtr pScreen, CursorPtr pCurs, Bool displayed)
{
    xf86CursorScreenPtr ScreenPriv =
        (xf86CursorScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                               xf86CursorScreenKey);

    input_lock();
    xf86RecolorCursor_locked (ScreenPriv, pCurs);
    input_unlock();
}

/* These functions assume that MaxWidth is a multiple of 32 */
static unsigned char *
RealizeCursorInterleave0(xf86CursorInfoPtr infoPtr, CursorPtr pCurs)
{

    SCANLINE *SrcS, *SrcM, *DstS, *DstM;
    SCANLINE *pSrc, *pMsk;
    unsigned char *mem;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;
    int SrcPitch, DstPitch, Pitch, y, x;

    /* how many words are in the source or mask */
    int words = size / (CUR_BITMAP_SCANLINE_PAD / 4);

    if (!(mem = calloc(1, size)))
        return NULL;

    if (pCurs == NullCursor) {
        if (infoPtr->Flags & HARDWARE_CURSOR_INVERT_MASK) {
            DstM = (SCANLINE *) mem;
            if (!(infoPtr->Flags & HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK))
                DstM += words;
            memset(DstM, -1, words * sizeof(SCANLINE));
        }
        return mem;
    }

    /* SrcPitch == the number of scanlines wide the cursor image is */
    SrcPitch = (pCurs->bits->width + (BITMAP_SCANLINE_PAD - 1)) >>
        CUR_LOG2_BITMAP_PAD;

    /* DstPitch is the width of the hw cursor in scanlines */
    DstPitch = infoPtr->MaxWidth >> CUR_LOG2_BITMAP_PAD;
    Pitch = SrcPitch < DstPitch ? SrcPitch : DstPitch;

    SrcS = (SCANLINE *) pCurs->bits->source;
    SrcM = (SCANLINE *) pCurs->bits->mask;
    DstS = (SCANLINE *) mem;
    DstM = DstS + words;

    if (infoPtr->Flags & HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK) {
        SCANLINE *tmp;

        tmp = DstS;
        DstS = DstM;
        DstM = tmp;
    }

    if (infoPtr->Flags & HARDWARE_CURSOR_AND_SOURCE_WITH_MASK) {
        for (y = pCurs->bits->height, pSrc = DstS, pMsk = DstM;
             y--;
             pSrc += DstPitch, pMsk += DstPitch, SrcS += SrcPitch, SrcM +=
             SrcPitch) {
            for (x = 0; x < Pitch; x++) {
                pSrc[x] = SrcS[x] & SrcM[x];
                pMsk[x] = SrcM[x];
            }
        }
    }
    else {
        for (y = pCurs->bits->height, pSrc = DstS, pMsk = DstM;
             y--;
             pSrc += DstPitch, pMsk += DstPitch, SrcS += SrcPitch, SrcM +=
             SrcPitch) {
            for (x = 0; x < Pitch; x++) {
                pSrc[x] = SrcS[x];
                pMsk[x] = SrcM[x];
            }
        }
    }

    if (infoPtr->Flags & HARDWARE_CURSOR_NIBBLE_SWAPPED) {
        int count = size;
        unsigned char *pntr1 = (unsigned char *) DstS;
        unsigned char *pntr2 = (unsigned char *) DstM;
        unsigned char a, b;

        while (count) {

            a = *pntr1;
            b = *pntr2;
            *pntr1 = ((a & 0xF0) >> 4) | ((a & 0x0F) << 4);
            *pntr2 = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
            pntr1++;
            pntr2++;
            count -= 2;
        }
    }

    /*
     * Must be _after_ HARDWARE_CURSOR_AND_SOURCE_WITH_MASK to avoid wiping
     * out entire source mask.
     */
    if (infoPtr->Flags & HARDWARE_CURSOR_INVERT_MASK) {
        int count = words;
        SCANLINE *pntr = DstM;

        while (count--) {
            *pntr = ~(*pntr);
            pntr++;
        }
    }

    if (infoPtr->Flags & HARDWARE_CURSOR_BIT_ORDER_MSBFIRST) {
        for (y = pCurs->bits->height, pSrc = DstS, pMsk = DstM;
             y--; pSrc += DstPitch, pMsk += DstPitch) {
            for (x = 0; x < Pitch; x++) {
                pSrc[x] = REVERSE_BIT_ORDER(pSrc[x]);
                pMsk[x] = REVERSE_BIT_ORDER(pMsk[x]);
            }
        }
    }

    return mem;
}

static unsigned char *
RealizeCursorInterleave1(xf86CursorInfoPtr infoPtr, CursorPtr pCurs)
{
    unsigned char *DstS, *DstM;
    unsigned char *pntr;
    unsigned char *mem, *mem2;
    int count;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;

    /* Realize the cursor without interleaving */
    if (!(mem2 = RealizeCursorInterleave0(infoPtr, pCurs)))
        return NULL;

    if (!(mem = calloc(1, size))) {
        free(mem2);
        return NULL;
    }

    /* 1 bit interleave */
    DstS = mem2;
    DstM = DstS + (size >> 1);
    pntr = mem;
    count = size;
    while (count) {
        *pntr++ = ((*DstS & 0x01)) | ((*DstM & 0x01) << 1) |
            ((*DstS & 0x02) << 1) | ((*DstM & 0x02) << 2) |
            ((*DstS & 0x04) << 2) | ((*DstM & 0x04) << 3) |
            ((*DstS & 0x08) << 3) | ((*DstM & 0x08) << 4);
        *pntr++ = ((*DstS & 0x10) >> 4) | ((*DstM & 0x10) >> 3) |
            ((*DstS & 0x20) >> 3) | ((*DstM & 0x20) >> 2) |
            ((*DstS & 0x40) >> 2) | ((*DstM & 0x40) >> 1) |
            ((*DstS & 0x80) >> 1) | ((*DstM & 0x80));
        DstS++;
        DstM++;
        count -= 2;
    }

    /* Free the uninterleaved cursor */
    free(mem2);

    return mem;
}

static unsigned char *
RealizeCursorInterleave8(xf86CursorInfoPtr infoPtr, CursorPtr pCurs)
{
    unsigned char *DstS, *DstM;
    unsigned char *pntr;
    unsigned char *mem, *mem2;
    int count;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;

    /* Realize the cursor without interleaving */
    if (!(mem2 = RealizeCursorInterleave0(infoPtr, pCurs)))
        return NULL;

    if (!(mem = calloc(1, size))) {
        free(mem2);
        return NULL;
    }

    /* 8 bit interleave */
    DstS = mem2;
    DstM = DstS + (size >> 1);
    pntr = mem;
    count = size;
    while (count) {
        *pntr++ = *DstS++;
        *pntr++ = *DstM++;
        count -= 2;
    }

    /* Free the uninterleaved cursor */
    free(mem2);

    return mem;
}

static unsigned char *
RealizeCursorInterleave16(xf86CursorInfoPtr infoPtr, CursorPtr pCurs)
{
    unsigned short *DstS, *DstM;
    unsigned short *pntr;
    unsigned char *mem, *mem2;
    int count;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;

    /* Realize the cursor without interleaving */
    if (!(mem2 = RealizeCursorInterleave0(infoPtr, pCurs)))
        return NULL;

    if (!(mem = calloc(1, size))) {
        free(mem2);
        return NULL;
    }

    /* 16 bit interleave */
    DstS = (void *) mem2;
    DstM = DstS + (size >> 2);
    pntr = (void *) mem;
    count = (size >> 1);
    while (count) {
        *pntr++ = *DstS++;
        *pntr++ = *DstM++;
        count -= 2;
    }

    /* Free the uninterleaved cursor */
    free(mem2);

    return mem;
}

static unsigned char *
RealizeCursorInterleave32(xf86CursorInfoPtr infoPtr, CursorPtr pCurs)
{
    CARD32 *DstS, *DstM;
    CARD32 *pntr;
    unsigned char *mem, *mem2;
    int count;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;

    /* Realize the cursor without interleaving */
    if (!(mem2 = RealizeCursorInterleave0(infoPtr, pCurs)))
        return NULL;

    if (!(mem = calloc(1, size))) {
        free(mem2);
        return NULL;
    }

    /* 32 bit interleave */
    DstS = (void *) mem2;
    DstM = DstS + (size >> 3);
    pntr = (void *) mem;
    count = (size >> 2);
    while (count) {
        *pntr++ = *DstS++;
        *pntr++ = *DstM++;
        count -= 2;
    }

    /* Free the uninterleaved cursor */
    free(mem2);

    return mem;
}

static unsigned char *
RealizeCursorInterleave64(xf86CursorInfoPtr infoPtr, CursorPtr pCurs)
{
    CARD32 *DstS, *DstM;
    CARD32 *pntr;
    unsigned char *mem, *mem2;
    int count;
    int size = (infoPtr->MaxWidth * infoPtr->MaxHeight) >> 2;

    /* Realize the cursor without interleaving */
    if (!(mem2 = RealizeCursorInterleave0(infoPtr, pCurs)))
        return NULL;

    if (!(mem = calloc(1, size))) {
        free(mem2);
        return NULL;
    }

    /* 64 bit interleave */
    DstS = (void *) mem2;
    DstM = DstS + (size >> 3);
    pntr = (void *) mem;
    count = (size >> 2);
    while (count) {
        *pntr++ = *DstS++;
        *pntr++ = *DstS++;
        *pntr++ = *DstM++;
        *pntr++ = *DstM++;
        count -= 4;
    }

    /* Free the uninterleaved cursor */
    free(mem2);

    return mem;
}
