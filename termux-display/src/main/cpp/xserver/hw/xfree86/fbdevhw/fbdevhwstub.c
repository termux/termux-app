#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86cmap.h"
#include "fbdevhw.h"

/* Stubs for the static server on platforms that don't support fbdev */

Bool
fbdevHWGetRec(ScrnInfoPtr pScrn)
{
    return FALSE;
}

void
fbdevHWFreeRec(ScrnInfoPtr pScrn)
{
}

Bool
fbdevHWProbe(struct pci_device *pPci, char *device, char **namep)
{
    return FALSE;
}

Bool
fbdevHWInit(ScrnInfoPtr pScrn, struct pci_device *pPci, char *device)
{
    xf86Msg(X_ERROR, "fbdevhw is not available on this platform\n");
    return FALSE;
}

char *
fbdevHWGetName(ScrnInfoPtr pScrn)
{
    return NULL;
}

int
fbdevHWGetDepth(ScrnInfoPtr pScrn, int *fbbpp)
{
    return -1;
}

int
fbdevHWGetLineLength(ScrnInfoPtr pScrn)
{
    return -1;                  /* Should cause something spectacular... */
}

int
fbdevHWGetType(ScrnInfoPtr pScrn)
{
    return -1;
}

int
fbdevHWGetVidmem(ScrnInfoPtr pScrn)
{
    return -1;
}

void
fbdevHWSetVideoModes(ScrnInfoPtr pScrn)
{
}

DisplayModePtr
fbdevHWGetBuildinMode(ScrnInfoPtr pScrn)
{
    return NULL;
}

void
fbdevHWUseBuildinMode(ScrnInfoPtr pScrn)
{
}

void *
fbdevHWMapVidmem(ScrnInfoPtr pScrn)
{
    return NULL;
}

int
fbdevHWLinearOffset(ScrnInfoPtr pScrn)
{
    return 0;
}

Bool
fbdevHWUnmapVidmem(ScrnInfoPtr pScrn)
{
    return FALSE;
}

void *
fbdevHWMapMMIO(ScrnInfoPtr pScrn)
{
    return NULL;
}

Bool
fbdevHWUnmapMMIO(ScrnInfoPtr pScrn)
{
    return FALSE;
}

Bool
fbdevHWModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    return FALSE;
}

void
fbdevHWSave(ScrnInfoPtr pScrn)
{
}

void
fbdevHWRestore(ScrnInfoPtr pScrn)
{
}

void
fbdevHWLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
                   LOCO * colors, VisualPtr pVisual)
{
}

ModeStatus
fbdevHWValidMode(ScrnInfoPtr pScrn, DisplayModePtr mode, Bool verbose, int flags)
{
    return MODE_ERROR;
}

Bool
fbdevHWSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    return FALSE;
}

void
fbdevHWAdjustFrame(ScrnInfoPtr pScrn, int x, int y)
{
}

Bool
fbdevHWEnterVT(ScrnInfoPtr pScrn)
{
    return FALSE;
}

void
fbdevHWLeaveVT(ScrnInfoPtr pScrn)
{
}

void
fbdevHWDPMSSet(ScrnInfoPtr pScrn, int mode, int flags)
{
}

Bool
fbdevHWSaveScreen(ScreenPtr pScreen, int mode)
{
    return FALSE;
}

xf86SwitchModeProc *
fbdevHWSwitchModeWeak(void)
{
    return fbdevHWSwitchMode;
}

xf86AdjustFrameProc *
fbdevHWAdjustFrameWeak(void)
{
    return fbdevHWAdjustFrame;
}

xf86EnterVTProc *
fbdevHWEnterVTWeak(void)
{
    return fbdevHWEnterVT;
}

xf86LeaveVTProc *
fbdevHWLeaveVTWeak(void)
{
    return fbdevHWLeaveVT;
}

xf86ValidModeProc *
fbdevHWValidModeWeak(void)
{
    return fbdevHWValidMode;
}

xf86DPMSSetProc *
fbdevHWDPMSSetWeak(void)
{
    return fbdevHWDPMSSet;
}

xf86LoadPaletteProc *
fbdevHWLoadPaletteWeak(void)
{
    return fbdevHWLoadPalette;
}

SaveScreenProcPtr
fbdevHWSaveScreenWeak(void)
{
    return fbdevHWSaveScreen;
}
