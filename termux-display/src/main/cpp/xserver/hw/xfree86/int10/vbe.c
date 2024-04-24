
/*
 *                   XFree86 vbe module
 *               Copyright 2000 Egbert Eich
 *
 * The mode query/save/set/restore functions from the vesa driver
 * have been moved here.
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
 * Authors: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <string.h>

#include "xf86.h"
#include "xf86Modes.h"
#include "vbe.h"
#include <X11/extensions/dpmsconst.h>

#define VERSION(x) VBE_VERSION_MAJOR(x),VBE_VERSION_MINOR(x)

#if X_BYTE_ORDER == X_LITTLE_ENDIAN
#define B_O16(x)  (x)
#define B_O32(x)  (x)
#else
#define B_O16(x)  ((((x) & 0xff) << 8) | (((x) & 0xff) >> 8))
#define B_O32(x)  ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) \
                  | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
#endif
#define L_ADD(x)  (B_O32(x) & 0xffff) + ((B_O32(x) >> 12) & 0xffff00)

#define FARP(p)		(((unsigned)(p & 0xffff0000) >> 12) | (p & 0xffff))
#define R16(v)		((v) & 0xffff)

static unsigned char *vbeReadEDID(vbeInfoPtr pVbe);
static Bool vbeProbeDDC(vbeInfoPtr pVbe);

static const char vbeVersionString[] = "VBE2";

vbeInfoPtr
VBEInit(xf86Int10InfoPtr pInt, int entityIndex)
{
    return VBEExtendedInit(pInt, entityIndex, 0);
}

vbeInfoPtr
VBEExtendedInit(xf86Int10InfoPtr pInt, int entityIndex, int Flags)
{
    int RealOff;
    void *page = NULL;
    ScrnInfoPtr pScrn = xf86FindScreenForEntity(entityIndex);
    vbeControllerInfoPtr vbe = NULL;
    Bool init_int10 = FALSE;
    vbeInfoPtr vip = NULL;
    int screen;

    if (!pScrn)
        return NULL;
    screen = pScrn->scrnIndex;

    if (!pInt) {
        if (!xf86LoadSubModule(pScrn, "int10"))
            goto error;

        xf86DrvMsg(screen, X_INFO, "initializing int10\n");
        pInt = xf86ExtendedInitInt10(entityIndex, Flags);
        if (!pInt)
            goto error;
        init_int10 = TRUE;
    }

    page = xf86Int10AllocPages(pInt, 1, &RealOff);
    if (!page)
        goto error;
    vbe = (vbeControllerInfoPtr) page;
    memcpy(vbe->VbeSignature, vbeVersionString, 4);

    pInt->ax = 0x4F00;
    pInt->es = SEG_ADDR(RealOff);
    pInt->di = SEG_OFF(RealOff);
    pInt->num = 0x10;

    xf86ExecX86int10(pInt);

    if ((pInt->ax & 0xff) != 0x4f) {
        xf86DrvMsgVerb(screen, X_INFO, 3, "VESA BIOS not detected\n");
        goto error;
    }

    switch (pInt->ax & 0xff00) {
    case 0:
        xf86DrvMsg(screen, X_INFO, "VESA BIOS detected\n");
        break;
    case 0x100:
        xf86DrvMsg(screen, X_INFO, "VESA BIOS function failed\n");
        goto error;
    case 0x200:
        xf86DrvMsg(screen, X_INFO, "VESA BIOS not supported\n");
        goto error;
    case 0x300:
        xf86DrvMsg(screen, X_INFO, "VESA BIOS not supported in current mode\n");
        goto error;
    default:
        xf86DrvMsg(screen, X_INFO, "Invalid\n");
        goto error;
    }

    xf86DrvMsgVerb(screen, X_INFO, 4,
                   "VbeVersion is %d, OemStringPtr is 0x%08lx,\n"
                   "\tOemVendorNamePtr is 0x%08lx, OemProductNamePtr is 0x%08lx,\n"
                   "\tOemProductRevPtr is 0x%08lx\n",
                   vbe->VbeVersion, (unsigned long) vbe->OemStringPtr,
                   (unsigned long) vbe->OemVendorNamePtr,
                   (unsigned long) vbe->OemProductNamePtr,
                   (unsigned long) vbe->OemProductRevPtr);

    xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE Version %i.%i\n",
                   VERSION(vbe->VbeVersion));
    xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE Total Mem: %i kB\n",
                   vbe->TotalMem * 64);
    xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE OEM: %s\n",
                   (CARD8 *) xf86int10Addr(pInt, L_ADD(vbe->OemStringPtr)));

    if (B_O16(vbe->VbeVersion) >= 0x200) {
        xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE OEM Software Rev: %i.%i\n",
                       VERSION(vbe->OemSoftwareRev));
        if (vbe->OemVendorNamePtr)
            xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE OEM Vendor: %s\n",
                           (CARD8 *) xf86int10Addr(pInt,
                                                   L_ADD(vbe->
                                                         OemVendorNamePtr)));
        if (vbe->OemProductNamePtr)
            xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE OEM Product: %s\n",
                           (CARD8 *) xf86int10Addr(pInt,
                                                   L_ADD(vbe->
                                                         OemProductNamePtr)));
        if (vbe->OemProductRevPtr)
            xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE OEM Product Rev: %s\n",
                           (CARD8 *) xf86int10Addr(pInt,
                                                   L_ADD(vbe->
                                                         OemProductRevPtr)));
    }
    vip = (vbeInfoPtr) xnfalloc(sizeof(vbeInfoRec));
    vip->version = B_O16(vbe->VbeVersion);
    vip->pInt10 = pInt;
    vip->ddc = DDC_UNCHECKED;
    vip->memory = page;
    vip->real_mode_base = RealOff;
    vip->num_pages = 1;
    vip->init_int10 = init_int10;

    return vip;

 error:
    if (page)
        xf86Int10FreePages(pInt, page, 1);
    if (init_int10)
        xf86FreeInt10(pInt);
    return NULL;
}

void
vbeFree(vbeInfoPtr pVbe)
{
    if (!pVbe)
        return;

    xf86Int10FreePages(pVbe->pInt10, pVbe->memory, pVbe->num_pages);
    /* If we have initialized int10 we ought to free it, too */
    if (pVbe->init_int10)
        xf86FreeInt10(pVbe->pInt10);
    free(pVbe);
    return;
}

static Bool
vbeProbeDDC(vbeInfoPtr pVbe)
{
    const char *ddc_level;
    int screen = pVbe->pInt10->pScrn->scrnIndex;

    if (pVbe->ddc == DDC_NONE)
        return FALSE;
    if (pVbe->ddc != DDC_UNCHECKED)
        return TRUE;

    pVbe->pInt10->ax = 0x4F15;
    pVbe->pInt10->bx = 0;
    pVbe->pInt10->cx = 0;
    pVbe->pInt10->es = 0;
    pVbe->pInt10->di = 0;
    pVbe->pInt10->num = 0x10;

    xf86ExecX86int10(pVbe->pInt10);

    if ((pVbe->pInt10->ax & 0xff) != 0x4f) {
        xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE DDC not supported\n");
        pVbe->ddc = DDC_NONE;
        return FALSE;
    }

    switch ((pVbe->pInt10->ax >> 8) & 0xff) {
    case 0:
        xf86DrvMsg(screen, X_INFO, "VESA VBE DDC supported\n");
        switch (pVbe->pInt10->bx & 0x3) {
        case 0:
            ddc_level = " none";
            pVbe->ddc = DDC_NONE;
            break;
        case 1:
            ddc_level = " 1";
            pVbe->ddc = DDC_1;
            break;
        case 2:
            ddc_level = " 2";
            pVbe->ddc = DDC_2;
            break;
        case 3:
            ddc_level = " 1 + 2";
            pVbe->ddc = DDC_1_2;
            break;
        default:
            ddc_level = "";
            pVbe->ddc = DDC_NONE;
            break;
        }
        xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE DDC Level%s\n", ddc_level);
        if (pVbe->pInt10->bx & 0x4) {
            xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE DDC Screen blanked"
                           "for data transfer\n");
            pVbe->ddc_blank = TRUE;
        }
        else
            pVbe->ddc_blank = FALSE;

        xf86DrvMsgVerb(screen, X_INFO, 3,
                       "VESA VBE DDC transfer in appr. %x sec.\n",
                       (pVbe->pInt10->bx >> 8) & 0xff);
    }

    return TRUE;
}

typedef enum {
    VBEOPT_NOVBE,
    VBEOPT_NODDC
} VBEOpts;

static const OptionInfoRec VBEOptions[] = {
    {VBEOPT_NOVBE, "NoVBE", OPTV_BOOLEAN, {0}, FALSE},
    {VBEOPT_NODDC, "NoDDC", OPTV_BOOLEAN, {0}, FALSE},
    {-1, NULL, OPTV_NONE, {0}, FALSE},
};

static unsigned char *
vbeReadEDID(vbeInfoPtr pVbe)
{
    int RealOff = pVbe->real_mode_base;
    void *page = pVbe->memory;
    unsigned char *tmp = NULL;
    Bool novbe = FALSE;
    Bool noddc = FALSE;
    ScrnInfoPtr pScrn = pVbe->pInt10->pScrn;
    int screen = pScrn->scrnIndex;
    OptionInfoPtr options;

    if (!page)
        return NULL;

    options = xnfalloc(sizeof(VBEOptions));
    (void) memcpy(options, VBEOptions, sizeof(VBEOptions));
    xf86ProcessOptions(screen, pScrn->options, options);
    xf86GetOptValBool(options, VBEOPT_NOVBE, &novbe);
    xf86GetOptValBool(options, VBEOPT_NODDC, &noddc);
    free(options);
    if (novbe || noddc)
        return NULL;

    if (!vbeProbeDDC(pVbe))
        goto error;

    memset(page, 0, sizeof(vbeInfoPtr));
    strcpy(page, vbeVersionString);

    pVbe->pInt10->ax = 0x4F15;
    pVbe->pInt10->bx = 0x01;
    pVbe->pInt10->cx = 0;
    pVbe->pInt10->dx = 0;
    pVbe->pInt10->es = SEG_ADDR(RealOff);
    pVbe->pInt10->di = SEG_OFF(RealOff);
    pVbe->pInt10->num = 0x10;

    xf86ExecX86int10(pVbe->pInt10);

    if ((pVbe->pInt10->ax & 0xff) != 0x4f) {
        xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE DDC invalid\n");
        goto error;
    }
    switch (pVbe->pInt10->ax & 0xff00) {
    case 0x0:
        xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE DDC read successfully\n");
        tmp = (unsigned char *) xnfalloc(128);
        memcpy(tmp, page, 128);
        break;
    case 0x100:
        xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE DDC read failed\n");
        break;
    default:
        xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE DDC unknown failure %i\n",
                       pVbe->pInt10->ax & 0xff00);
        break;
    }

 error:
    return tmp;
}

xf86MonPtr
vbeDoEDID(vbeInfoPtr pVbe, void *unused)
{
    unsigned char *DDC_data = NULL;

    if (!pVbe)
        return NULL;
    if (pVbe->version < 0x102)
        return NULL;

    DDC_data = vbeReadEDID(pVbe);

    if (!DDC_data)
        return NULL;

    return xf86InterpretEDID(pVbe->pInt10->pScrn->scrnIndex, DDC_data);
}

#define GET_UNALIGNED2(x) \
            ((*(CARD16*)(x)) | (*(((CARD16*)(x) + 1))) << 16)

VbeInfoBlock *
VBEGetVBEInfo(vbeInfoPtr pVbe)
{
    VbeInfoBlock *block = NULL;
    int i, pStr, pModes;
    char *str;
    CARD16 major, *modes;

    memset(pVbe->memory, 0, sizeof(VbeInfoBlock));

    /*
       Input:
       AH    := 4Fh     Super VGA support
       AL    := 00h     Return Super VGA information
       ES:DI := Pointer to buffer

       Output:
       AX    := status
       (All other registers are preserved)
     */

    ((char *) pVbe->memory)[0] = 'V';
    ((char *) pVbe->memory)[1] = 'B';
    ((char *) pVbe->memory)[2] = 'E';
    ((char *) pVbe->memory)[3] = '2';

    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f00;
    pVbe->pInt10->es = SEG_ADDR(pVbe->real_mode_base);
    pVbe->pInt10->di = SEG_OFF(pVbe->real_mode_base);
    xf86ExecX86int10(pVbe->pInt10);

    if (R16(pVbe->pInt10->ax) != 0x4f)
        return NULL;

    block = calloc(sizeof(VbeInfoBlock), 1);
    block->VESASignature[0] = ((char *) pVbe->memory)[0];
    block->VESASignature[1] = ((char *) pVbe->memory)[1];
    block->VESASignature[2] = ((char *) pVbe->memory)[2];
    block->VESASignature[3] = ((char *) pVbe->memory)[3];

    block->VESAVersion = *(CARD16 *) (((char *) pVbe->memory) + 4);
    major = (unsigned) block->VESAVersion >> 8;

    pStr = GET_UNALIGNED2((((char *) pVbe->memory) + 6));
    str = xf86int10Addr(pVbe->pInt10, FARP(pStr));
    block->OEMStringPtr = strdup(str);

    block->Capabilities[0] = ((char *) pVbe->memory)[10];
    block->Capabilities[1] = ((char *) pVbe->memory)[11];
    block->Capabilities[2] = ((char *) pVbe->memory)[12];
    block->Capabilities[3] = ((char *) pVbe->memory)[13];

    pModes = GET_UNALIGNED2((((char *) pVbe->memory) + 14));
    modes = xf86int10Addr(pVbe->pInt10, FARP(pModes));
    i = 0;
    while (modes[i] != 0xffff)
        i++;
    block->VideoModePtr = xallocarray(i + 1, sizeof(CARD16));
    memcpy(block->VideoModePtr, modes, sizeof(CARD16) * i);
    block->VideoModePtr[i] = 0xffff;

    block->TotalMemory = *(CARD16 *) (((char *) pVbe->memory) + 18);

    if (major < 2)
        memcpy(&block->OemSoftwareRev, ((char *) pVbe->memory) + 20, 236);
    else {
        block->OemSoftwareRev = *(CARD16 *) (((char *) pVbe->memory) + 20);
        pStr = GET_UNALIGNED2((((char *) pVbe->memory) + 22));
        str = xf86int10Addr(pVbe->pInt10, FARP(pStr));
        block->OemVendorNamePtr = strdup(str);
        pStr = GET_UNALIGNED2((((char *) pVbe->memory) + 26));
        str = xf86int10Addr(pVbe->pInt10, FARP(pStr));
        block->OemProductNamePtr = strdup(str);
        pStr = GET_UNALIGNED2((((char *) pVbe->memory) + 30));
        str = xf86int10Addr(pVbe->pInt10, FARP(pStr));
        block->OemProductRevPtr = strdup(str);
        memcpy(&block->Reserved, ((char *) pVbe->memory) + 34, 222);
        memcpy(&block->OemData, ((char *) pVbe->memory) + 256, 256);
    }

    return block;
}

void
VBEFreeVBEInfo(VbeInfoBlock * block)
{
    free(block->OEMStringPtr);
    free(block->VideoModePtr);
    if (((unsigned) block->VESAVersion >> 8) >= 2) {
        free(block->OemVendorNamePtr);
        free(block->OemProductNamePtr);
        free(block->OemProductRevPtr);
    }
    free(block);
}

Bool
VBESetVBEMode(vbeInfoPtr pVbe, int mode, VbeCRTCInfoBlock * block)
{
    /*
       Input:
       AH    := 4Fh     Super VGA support
       AL    := 02h     Set Super VGA video mode
       BX    := Video mode
       D0-D8  := Mode number
       D9-D10 := Reserved (must be 0)
       D11    := 0 Use current default refresh rate
       := 1 Use user specified CRTC values for refresh rate
       D12-13   Reserved for VBE/AF (must be 0)
       D14    := 0 Use windowed frame buffer model
       := 1 Use linear/flat frame buffer model
       D15    := 0 Clear video memory
       := 1 Don't clear video memory
       ES:DI := Pointer to VbeCRTCInfoBlock structure

       Output: AX = Status
       (All other registers are preserved)
     */
    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f02;
    pVbe->pInt10->bx = mode;
    if (block) {
        pVbe->pInt10->bx |= 1 << 11;
        memcpy(pVbe->memory, block, sizeof(VbeCRTCInfoBlock));
        pVbe->pInt10->es = SEG_ADDR(pVbe->real_mode_base);
        pVbe->pInt10->di = SEG_OFF(pVbe->real_mode_base);
    }
    else
        pVbe->pInt10->bx &= ~(1 << 11);

    xf86ExecX86int10(pVbe->pInt10);

    return (R16(pVbe->pInt10->ax) == 0x4f);
}

Bool
VBEGetVBEMode(vbeInfoPtr pVbe, int *mode)
{
    /*
       Input:
       AH := 4Fh        Super VGA support
       AL := 03h        Return current video mode

       Output:
       AX := Status
       BX := Current video mode
       (All other registers are preserved)
     */
    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f03;

    xf86ExecX86int10(pVbe->pInt10);

    if (R16(pVbe->pInt10->ax) == 0x4f) {
        *mode = R16(pVbe->pInt10->bx);

        return TRUE;
    }

    return FALSE;
}

VbeModeInfoBlock *
VBEGetModeInfo(vbeInfoPtr pVbe, int mode)
{
    VbeModeInfoBlock *block = NULL;

    memset(pVbe->memory, 0, sizeof(VbeModeInfoBlock));

    /*
       Input:
       AH    := 4Fh     Super VGA support
       AL    := 01h     Return Super VGA mode information
       CX    :=         Super VGA video mode
       (mode number must be one of those returned by Function 0)
       ES:DI := Pointer to buffer

       Output:
       AX    := status
       (All other registers are preserved)
     */
    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f01;
    pVbe->pInt10->cx = mode;
    pVbe->pInt10->es = SEG_ADDR(pVbe->real_mode_base);
    pVbe->pInt10->di = SEG_OFF(pVbe->real_mode_base);
    xf86ExecX86int10(pVbe->pInt10);
    if (R16(pVbe->pInt10->ax) != 0x4f)
        return NULL;

    block = malloc(sizeof(VbeModeInfoBlock));
    if (block)
        memcpy(block, pVbe->memory, sizeof(*block));

    return block;
}

void
VBEFreeModeInfo(VbeModeInfoBlock * block)
{
    free(block);
}

Bool
VBESaveRestore(vbeInfoPtr pVbe, vbeSaveRestoreFunction function,
               void **memory, int *size, int *real_mode_pages)
{
    /*
       Input:
       AH    := 4Fh     Super VGA support
       AL    := 04h     Save/restore Super VGA video state
       DL    := 00h     Return save/restore state buffer size
       CX    := Requested states
       D0 = Save/restore video hardware state
       D1 = Save/restore video BIOS data state
       D2 = Save/restore video DAC state
       D3 = Save/restore Super VGA state

       Output:
       AX = Status
       BX = Number of 64-byte blocks to hold the state buffer
       (All other registers are preserved)

       Input:
       AH    := 4Fh     Super VGA support
       AL    := 04h     Save/restore Super VGA video state
       DL    := 01h     Save Super VGA video state
       CX    := Requested states (see above)
       ES:BX := Pointer to buffer

       Output:
       AX    := Status
       (All other registers are preserved)

       Input:
       AH    := 4Fh     Super VGA support
       AL    := 04h     Save/restore Super VGA video state
       DL    := 02h     Restore Super VGA video state
       CX    := Requested states (see above)
       ES:BX := Pointer to buffer

       Output:
       AX     := Status
       (All other registers are preserved)
     */

    if ((pVbe->version & 0xff00) > 0x100) {
        int screen = pVbe->pInt10->pScrn->scrnIndex;

        if (function == MODE_QUERY || (function == MODE_SAVE && !*memory)) {
            /* Query amount of memory to save state */

            pVbe->pInt10->num = 0x10;
            pVbe->pInt10->ax = 0x4f04;
            pVbe->pInt10->dx = 0;
            pVbe->pInt10->cx = 0x000f;
            xf86ExecX86int10(pVbe->pInt10);
            if (R16(pVbe->pInt10->ax) != 0x4f)
                return FALSE;

            if (function == MODE_SAVE) {
                int npages = (R16(pVbe->pInt10->bx) * 64) / 4096 + 1;

                if ((*memory = xf86Int10AllocPages(pVbe->pInt10, npages,
                                                   real_mode_pages)) == NULL) {
                    xf86DrvMsg(screen, X_ERROR,
                               "Cannot allocate memory to save SVGA state.\n");
                    return FALSE;
                }
            }
            *size = pVbe->pInt10->bx * 64;
        }

        /* Save/Restore Super VGA state */
        if (function != MODE_QUERY) {

            if (!*memory)
                return FALSE;
            pVbe->pInt10->num = 0x10;
            pVbe->pInt10->ax = 0x4f04;
            switch (function) {
            case MODE_SAVE:
                pVbe->pInt10->dx = 1;
                break;
            case MODE_RESTORE:
                pVbe->pInt10->dx = 2;
                break;
            case MODE_QUERY:
                return FALSE;
            }
            pVbe->pInt10->cx = 0x000f;

            pVbe->pInt10->es = SEG_ADDR(*real_mode_pages);
            pVbe->pInt10->bx = SEG_OFF(*real_mode_pages);
            xf86ExecX86int10(pVbe->pInt10);
            return (R16(pVbe->pInt10->ax) == 0x4f);

        }
    }
    return TRUE;
}

Bool
VBEBankSwitch(vbeInfoPtr pVbe, unsigned int iBank, int window)
{
    /*
       Input:
       AH    := 4Fh     Super VGA support
       AL    := 05h

       Output:
     */
    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f05;
    pVbe->pInt10->bx = window;
    pVbe->pInt10->dx = iBank;
    xf86ExecX86int10(pVbe->pInt10);

    if (R16(pVbe->pInt10->ax) != 0x4f)
        return FALSE;

    return TRUE;
}

Bool
VBESetGetLogicalScanlineLength(vbeInfoPtr pVbe, vbeScanwidthCommand command,
                               int width, int *pixels, int *bytes, int *max)
{
    if (command < SCANWID_SET || command > SCANWID_GET_MAX)
        return FALSE;

    /*
       Input:
       AX := 4F06h VBE Set/Get Logical Scan Line Length
       BL := 00h Set Scan Line Length in Pixels
       := 01h Get Scan Line Length
       := 02h Set Scan Line Length in Bytes
       := 03h Get Maximum Scan Line Length
       CX := If BL=00h Desired Width in Pixels
       If BL=02h Desired Width in Bytes
       (Ignored for Get Functions)

       Output:
       AX := VBE Return Status
       BX := Bytes Per Scan Line
       CX := Actual Pixels Per Scan Line
       (truncated to nearest complete pixel)
       DX := Maximum Number of Scan Lines
     */

    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f06;
    pVbe->pInt10->bx = command;
    if (command == SCANWID_SET || command == SCANWID_SET_BYTES)
        pVbe->pInt10->cx = width;
    xf86ExecX86int10(pVbe->pInt10);

    if (R16(pVbe->pInt10->ax) != 0x4f)
        return FALSE;

    if (command == SCANWID_GET || command == SCANWID_GET_MAX) {
        if (pixels)
            *pixels = R16(pVbe->pInt10->cx);
        if (bytes)
            *bytes = R16(pVbe->pInt10->bx);
        if (max)
            *max = R16(pVbe->pInt10->dx);
    }

    return TRUE;
}

Bool
VBESetDisplayStart(vbeInfoPtr pVbe, int x, int y, Bool wait_retrace)
{
    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f07;
    pVbe->pInt10->bx = wait_retrace ? 0x80 : 0x00;
    pVbe->pInt10->cx = x;
    pVbe->pInt10->dx = y;
    xf86ExecX86int10(pVbe->pInt10);

    if (R16(pVbe->pInt10->ax) != 0x4f)
        return FALSE;

    return TRUE;
}

Bool
VBEGetDisplayStart(vbeInfoPtr pVbe, int *x, int *y)
{
    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f07;
    pVbe->pInt10->bx = 0x01;
    xf86ExecX86int10(pVbe->pInt10);

    if (R16(pVbe->pInt10->ax) != 0x4f)
        return FALSE;

    *x = pVbe->pInt10->cx;
    *y = pVbe->pInt10->dx;

    return TRUE;
}

int
VBESetGetDACPaletteFormat(vbeInfoPtr pVbe, int bits)
{
    /*
       Input:
       AX := 4F08h VBE Set/Get Palette Format
       BL := 00h Set DAC Palette Format
       := 01h Get DAC Palette Format
       BH := Desired bits of color per primary
       (Set DAC Palette Format only)

       Output:
       AX := VBE Return Status
       BH := Current number of bits of color per primary
     */

    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f08;
    if (!bits)
        pVbe->pInt10->bx = 0x01;
    else
        pVbe->pInt10->bx = (bits & 0x00ff) << 8;
    xf86ExecX86int10(pVbe->pInt10);

    if (R16(pVbe->pInt10->ax) != 0x4f)
        return 0;

    return (bits != 0 ? bits : (pVbe->pInt10->bx >> 8) & 0x00ff);
}

CARD32 *
VBESetGetPaletteData(vbeInfoPtr pVbe, Bool set, int first, int num,
                     CARD32 *data, Bool secondary, Bool wait_retrace)
{
    /*
       Input:
       (16-bit)
       AX    := 4F09h VBE Load/Unload Palette Data
       BL    := 00h Set Palette Data
       := 01h Get Palette Data
       := 02h Set Secondary Palette Data
       := 03h Get Secondary Palette Data
       := 80h Set Palette Data during Vertical Retrace
       CX    := Number of palette registers to update (to a maximum of 256)
       DX    := First of the palette registers to update (start)
       ES:DI := Table of palette values (see below for format)

       Output:
       AX    := VBE Return Status

       Input:
       (32-bit)
       BL     := 00h Set Palette Data
       := 80h Set Palette Data during Vertical Retrace
       CX     := Number of palette registers to update (to a maximum of 256)
       DX     := First of the palette registers to update (start)
       ES:EDI := Table of palette values (see below for format)
       DS     := Selector for memory mapped registers
     */

    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f09;
    if (!secondary)
        pVbe->pInt10->bx = set && wait_retrace ? 0x80 : set ? 0 : 1;
    else
        pVbe->pInt10->bx = set ? 2 : 3;
    pVbe->pInt10->cx = num;
    pVbe->pInt10->dx = first;
    pVbe->pInt10->es = SEG_ADDR(pVbe->real_mode_base);
    pVbe->pInt10->di = SEG_OFF(pVbe->real_mode_base);
    if (set)
        memcpy(pVbe->memory, data, num * sizeof(CARD32));
    xf86ExecX86int10(pVbe->pInt10);

    if (R16(pVbe->pInt10->ax) != 0x4f)
        return NULL;

    if (set)
        return data;

    data = xallocarray(num, sizeof(CARD32));
    memcpy(data, pVbe->memory, num * sizeof(CARD32));

    return data;
}

VBEpmi *
VBEGetVBEpmi(vbeInfoPtr pVbe)
{
    VBEpmi *pmi;

    /*
       Input:
       AH    := 4Fh     Super VGA support
       AL    := 0Ah     Protected Mode Interface
       BL    := 00h     Return Protected Mode Table

       Output:
       AX    := Status
       ES    := Real Mode Segment of Table
       DI    := Offset of Table
       CX    := Length of Table including protected mode code in bytes (for copying purposes)
       (All other registers are preserved)
     */

    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f0a;
    pVbe->pInt10->bx = 0;
    pVbe->pInt10->di = 0;
    xf86ExecX86int10(pVbe->pInt10);

    if (R16(pVbe->pInt10->ax) != 0x4f)
        return NULL;

    pmi = malloc(sizeof(VBEpmi));
    pmi->seg_tbl = R16(pVbe->pInt10->es);
    pmi->tbl_off = R16(pVbe->pInt10->di);
    pmi->tbl_len = R16(pVbe->pInt10->cx);

    return pmi;
}

#if 0
vbeModeInfoPtr
VBEBuildVbeModeList(vbeInfoPtr pVbe, VbeInfoBlock * vbe)
{
    vbeModeInfoPtr ModeList = NULL;

    int i = 0;

    while (vbe->VideoModePtr[i] != 0xffff) {
        vbeModeInfoPtr m;
        VbeModeInfoBlock *mode;
        int id = vbe->VideoModePtr[i++];
        int bpp;

        if ((mode = VBEGetModeInfo(pVbe, id)) == NULL)
            continue;

        bpp = mode->BitsPerPixel;

        m = xnfcalloc(sizeof(vbeModeInfoRec), 1);
        m->width = mode->XResolution;
        m->height = mode->YResolution;
        m->bpp = bpp;
        m->n = id;
        m->next = ModeList;

        xf86DrvMsgVerb(pVbe->pInt10->pScrn->scrnIndex, X_PROBED, 3,
                       "BIOS reported VESA mode 0x%x: x:%i y:%i bpp:%i\n",
                       m->n, m->width, m->height, m->bpp);

        ModeList = m;

        VBEFreeModeInfo(mode);
    }
    return ModeList;
}

unsigned short
VBECalcVbeModeIndex(vbeModeInfoPtr m, DisplayModePtr mode, int bpp)
{
    while (m) {
        if (bpp == m->bpp
            && mode->HDisplay == m->width && mode->VDisplay == m->height)
            return m->n;
        m = m->next;
    }
    return 0;
}
#endif

void
VBEVesaSaveRestore(vbeInfoPtr pVbe, vbeSaveRestorePtr vbe_sr,
                   vbeSaveRestoreFunction function)
{
    Bool SaveSucc = FALSE;

    if (VBE_VERSION_MAJOR(pVbe->version) > 1
        && (function == MODE_SAVE || vbe_sr->pstate)) {
        if (function == MODE_RESTORE)
            memcpy(vbe_sr->state, vbe_sr->pstate, vbe_sr->stateSize);
        ErrorF("VBESaveRestore\n");
        if ((VBESaveRestore(pVbe, function,
                            (void *) &vbe_sr->state,
                            &vbe_sr->stateSize, &vbe_sr->statePage))) {
            if (function == MODE_SAVE) {
                SaveSucc = TRUE;
                vbe_sr->stateMode = -1; /* invalidate */
                /* don't rely on the memory not being touched */
                if (vbe_sr->pstate == NULL)
                    vbe_sr->pstate = malloc(vbe_sr->stateSize);
                memcpy(vbe_sr->pstate, vbe_sr->state, vbe_sr->stateSize);
            }
            ErrorF("VBESaveRestore done with success\n");
            return;
        }
        ErrorF("VBESaveRestore done\n");
    }

    if (function == MODE_SAVE && !SaveSucc)
        (void) VBEGetVBEMode(pVbe, &vbe_sr->stateMode);

    if (function == MODE_RESTORE && vbe_sr->stateMode != -1)
        VBESetVBEMode(pVbe, vbe_sr->stateMode, NULL);

}

int
VBEGetPixelClock(vbeInfoPtr pVbe, int mode, int clock)
{
    /*
       Input:
       AX := 4F0Bh VBE Get Pixel Clock
       BL := 00h Get Pixel Clock
       ECX := pixel clock in units of Hz
       DX := mode number

       Output:
       AX := VBE Return Status
       ECX := Closest pixel clock
     */

    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f0b;
    pVbe->pInt10->bx = 0x00;
    pVbe->pInt10->cx = clock;
    pVbe->pInt10->dx = mode;
    xf86ExecX86int10(pVbe->pInt10);

    if (R16(pVbe->pInt10->ax) != 0x4f)
        return 0;

    return pVbe->pInt10->cx;
}

Bool
VBEDPMSSet(vbeInfoPtr pVbe, int mode)
{
    /*
       Input:
       AX := 4F10h DPMS
       BL := 01h Set Display Power State
       BH := requested power state

       Output:
       AX := VBE Return Status
     */

    pVbe->pInt10->num = 0x10;
    pVbe->pInt10->ax = 0x4f10;
    pVbe->pInt10->bx = 0x01;
    switch (mode) {
    case DPMSModeOn:
        break;
    case DPMSModeStandby:
        pVbe->pInt10->bx |= 0x100;
        break;
    case DPMSModeSuspend:
        pVbe->pInt10->bx |= 0x200;
        break;
    case DPMSModeOff:
        pVbe->pInt10->bx |= 0x400;
        break;
    }
    xf86ExecX86int10(pVbe->pInt10);
    return (R16(pVbe->pInt10->ax) == 0x4f);
}

void
VBEInterpretPanelID(ScrnInfoPtr pScrn, struct vbePanelID *data)
{
    DisplayModePtr mode;
    const float PANEL_HZ = 60.0;

    if (!data)
        return;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "PanelID returned panel resolution %dx%d\n",
               data->hsize, data->vsize);

    if (pScrn->monitor->nHsync || pScrn->monitor->nVrefresh)
        return;

    if (data->hsize < 320 || data->vsize < 240) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "...which I refuse to believe\n");
        return;
    }

    mode = xf86CVTMode(data->hsize, data->vsize, PANEL_HZ, 1, 0);

    pScrn->monitor->nHsync = 1;
    pScrn->monitor->hsync[0].lo = 29.37;
    pScrn->monitor->hsync[0].hi = (float) mode->Clock / (float) mode->HTotal;
    pScrn->monitor->nVrefresh = 1;
    pScrn->monitor->vrefresh[0].lo = 56.0;
    pScrn->monitor->vrefresh[0].hi =
        (float) mode->Clock * 1000.0 / (float) mode->HTotal /
        (float) mode->VTotal;

    if (pScrn->monitor->vrefresh[0].hi < 59.47)
        pScrn->monitor->vrefresh[0].hi = 59.47;

    free(mode);
}

struct vbePanelID *
VBEReadPanelID(vbeInfoPtr pVbe)
{
    int RealOff = pVbe->real_mode_base;
    void *page = pVbe->memory;
    void *tmp = NULL;
    int screen = pVbe->pInt10->pScrn->scrnIndex;

    pVbe->pInt10->ax = 0x4F11;
    pVbe->pInt10->bx = 0x01;
    pVbe->pInt10->cx = 0;
    pVbe->pInt10->dx = 0;
    pVbe->pInt10->es = SEG_ADDR(RealOff);
    pVbe->pInt10->di = SEG_OFF(RealOff);
    pVbe->pInt10->num = 0x10;

    xf86ExecX86int10(pVbe->pInt10);

    if ((pVbe->pInt10->ax & 0xff) != 0x4f) {
        xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE PanelID invalid\n");
        goto error;
    }

    switch (pVbe->pInt10->ax & 0xff00) {
    case 0x0:
        xf86DrvMsgVerb(screen, X_INFO, 3,
                       "VESA VBE PanelID read successfully\n");
        tmp = xnfalloc(32);
        memcpy(tmp, page, 32);
        break;
    case 0x100:
        xf86DrvMsgVerb(screen, X_INFO, 3, "VESA VBE PanelID read failed\n");
        break;
    default:
        xf86DrvMsgVerb(screen, X_INFO, 3,
                       "VESA VBE PanelID unknown failure %i\n",
                       pVbe->pInt10->ax & 0xff00);
        break;
    }

 error:
    return tmp;
}
