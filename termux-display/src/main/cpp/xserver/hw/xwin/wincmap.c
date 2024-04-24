/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

/*
 * Local prototypes
 */

static int
 winListInstalledColormaps(ScreenPtr pScreen, Colormap * pmaps);

static void
 winStoreColors(ColormapPtr pmap, int ndef, xColorItem * pdefs);

static void
 winInstallColormap(ColormapPtr pmap);

static void
 winUninstallColormap(ColormapPtr pmap);

static void

winResolveColor(unsigned short *pred,
                unsigned short *pgreen,
                unsigned short *pblue, VisualPtr pVisual);

static Bool
 winCreateColormap(ColormapPtr pmap);

static void
 winDestroyColormap(ColormapPtr pmap);

static Bool
 winGetPaletteDIB(ScreenPtr pScreen, ColormapPtr pcmap);

static Bool
 winGetPaletteDD(ScreenPtr pScreen, ColormapPtr pcmap);

/*
 * Set screen functions for colormaps
 */

void
winSetColormapFunctions(ScreenPtr pScreen)
{
    pScreen->CreateColormap = winCreateColormap;
    pScreen->DestroyColormap = winDestroyColormap;
    pScreen->InstallColormap = winInstallColormap;
    pScreen->UninstallColormap = winUninstallColormap;
    pScreen->ListInstalledColormaps = winListInstalledColormaps;
    pScreen->StoreColors = winStoreColors;
    pScreen->ResolveColor = winResolveColor;
}

/* See Porting Layer Definition - p. 30 */
/*
 * Walk the list of installed colormaps, filling the pmaps list
 * with the resource ids of the installed maps, and return
 * a count of the total number of installed maps.
 */
static int
winListInstalledColormaps(ScreenPtr pScreen, Colormap * pmaps)
{
    winScreenPriv(pScreen);

    /*
     * There will only be one installed colormap, so we only need
     * to return one id, and the count of installed maps will always
     * be one.
     */
    *pmaps = pScreenPriv->pcmapInstalled->mid;
    return 1;
}

/* See Porting Layer Definition - p. 30 */
/* See Programming Windows - p. 663 */
static void
winInstallColormap(ColormapPtr pColormap)
{
    ScreenPtr pScreen = pColormap->pScreen;

    winScreenPriv(pScreen);
    ColormapPtr oldpmap = pScreenPriv->pcmapInstalled;

#if CYGDEBUG
    winDebug("winInstallColormap\n");
#endif

    /* Did the colormap actually change? */
    if (pColormap != oldpmap) {
#if CYGDEBUG
        winDebug("winInstallColormap - Colormap has changed, attempt "
                 "to install.\n");
#endif

        /* Was there a previous colormap? */
        if (oldpmap != (ColormapPtr) None) {
            /* There was a previous colormap; tell clients it is gone */
            WalkTree(pColormap->pScreen, TellLostMap, (char *) &oldpmap->mid);
        }

        /* Install new colormap */
        pScreenPriv->pcmapInstalled = pColormap;
        WalkTree(pColormap->pScreen, TellGainedMap, (char *) &pColormap->mid);

        /* Call the engine specific colormap install procedure */
        if (!((*pScreenPriv->pwinInstallColormap) (pColormap))) {
            winErrorFVerb(2,
                          "winInstallColormap - Screen specific colormap install "
                          "procedure failed.  Continuing, but colors may be "
                          "messed up from now on.\n");
        }
    }

    /* Save a pointer to the newly installed colormap */
    pScreenPriv->pcmapInstalled = pColormap;
}

/* See Porting Layer Definition - p. 30 */
static void
winUninstallColormap(ColormapPtr pmap)
{
    winScreenPriv(pmap->pScreen);
    ColormapPtr curpmap = pScreenPriv->pcmapInstalled;

#if CYGDEBUG
    winDebug("winUninstallColormap\n");
#endif

    /* Is the colormap currently installed? */
    if (pmap != curpmap) {
        /* Colormap not installed, nothing to do */
        return;
    }

    /* Clear the installed colormap flag */
    pScreenPriv->pcmapInstalled = NULL;

    /*
     * NOTE: The default colormap does not get "uninstalled" before
     * it is destroyed.
     */

    /* Install the default cmap in place of the cmap to be uninstalled */
    if (pmap->mid != pmap->pScreen->defColormap) {
        dixLookupResourceByType((void *) &curpmap, pmap->pScreen->defColormap,
                                RT_COLORMAP, NullClient, DixUnknownAccess);
        (*pmap->pScreen->InstallColormap) (curpmap);
    }
}

/* See Porting Layer Definition - p. 30 */
static void
winStoreColors(ColormapPtr pmap, int ndef, xColorItem * pdefs)
{
    ScreenPtr pScreen = pmap->pScreen;

    winScreenPriv(pScreen);
    winCmapPriv(pmap);
    int i;
    unsigned short nRed, nGreen, nBlue;

#if CYGDEBUG
    if (ndef != 1)
        winDebug("winStoreColors - ndef: %d\n", ndef);
#endif

    /* Save the new colors in the colormap privates */
    for (i = 0; i < ndef; ++i) {
        /* Adjust the colors from the X color spec to the Windows color spec */
        nRed = pdefs[i].red >> 8;
        nGreen = pdefs[i].green >> 8;
        nBlue = pdefs[i].blue >> 8;

        /* Copy the colors to a palette entry table */
        pCmapPriv->peColors[pdefs[0].pixel + i].peRed = nRed;
        pCmapPriv->peColors[pdefs[0].pixel + i].peGreen = nGreen;
        pCmapPriv->peColors[pdefs[0].pixel + i].peBlue = nBlue;

        /* Copy the colors to a RGBQUAD table */
        pCmapPriv->rgbColors[pdefs[0].pixel + i].rgbRed = nRed;
        pCmapPriv->rgbColors[pdefs[0].pixel + i].rgbGreen = nGreen;
        pCmapPriv->rgbColors[pdefs[0].pixel + i].rgbBlue = nBlue;

#if CYGDEBUG
        winDebug("winStoreColors - nRed %d nGreen %d nBlue %d\n",
                 nRed, nGreen, nBlue);
#endif
    }

    /* Call the engine specific store colors procedure */
    if (!((pScreenPriv->pwinStoreColors) (pmap, ndef, pdefs))) {
        winErrorFVerb(2,
                      "winStoreColors - Engine cpecific color storage procedure "
                      "failed.  Continuing, but colors may be messed up from now "
                      "on.\n");
    }
}

/* See Porting Layer Definition - p. 30 */
static void
winResolveColor(unsigned short *pred,
                unsigned short *pgreen,
                unsigned short *pblue, VisualPtr pVisual)
{
#if CYGDEBUG
    winDebug("winResolveColor ()\n");
#endif

    miResolveColor(pred, pgreen, pblue, pVisual);
}

/* See Porting Layer Definition - p. 29 */
static Bool
winCreateColormap(ColormapPtr pmap)
{
    winPrivCmapPtr pCmapPriv = NULL;
    ScreenPtr pScreen = pmap->pScreen;

    winScreenPriv(pScreen);

#if CYGDEBUG
    winDebug("winCreateColormap\n");
#endif

    /* Allocate colormap privates */
    if (!winAllocateCmapPrivates(pmap)) {
        ErrorF("winCreateColorma - Couldn't allocate cmap privates\n");
        return FALSE;
    }

    /* Get a pointer to the newly allocated privates */
    pCmapPriv = winGetCmapPriv(pmap);

    /*
     * FIXME: This is some evil hackery to help in handling some X clients
     * that expect the top pixel to be white.  This "help" only lasts until
     * some client overwrites the top colormap entry.
     *
     * We don't want to actually allocate the top entry, as that causes
     * problems with X clients that need 7 planes (128 colors) in the default
     * colormap, such as Magic 7.1.
     */
    pCmapPriv->rgbColors[WIN_NUM_PALETTE_ENTRIES - 1].rgbRed = 255;
    pCmapPriv->rgbColors[WIN_NUM_PALETTE_ENTRIES - 1].rgbGreen = 255;
    pCmapPriv->rgbColors[WIN_NUM_PALETTE_ENTRIES - 1].rgbBlue = 255;
    pCmapPriv->peColors[WIN_NUM_PALETTE_ENTRIES - 1].peRed = 255;
    pCmapPriv->peColors[WIN_NUM_PALETTE_ENTRIES - 1].peGreen = 255;
    pCmapPriv->peColors[WIN_NUM_PALETTE_ENTRIES - 1].peBlue = 255;

    /* Call the engine specific colormap initialization procedure */
    if (!((*pScreenPriv->pwinCreateColormap) (pmap))) {
        ErrorF("winCreateColormap - Engine specific colormap creation "
               "procedure failed.  Aborting.\n");
        return FALSE;
    }

    return TRUE;
}

/* See Porting Layer Definition - p. 29, 30 */
static void
winDestroyColormap(ColormapPtr pColormap)
{
    winScreenPriv(pColormap->pScreen);
    winCmapPriv(pColormap);

    /* Call the engine specific colormap destruction procedure */
    if (!((*pScreenPriv->pwinDestroyColormap) (pColormap))) {
        winErrorFVerb(2,
                      "winDestroyColormap - Engine specific colormap destruction "
                      "procedure failed.  Continuing, but it is possible that memory "
                      "was leaked, or that colors will be messed up from now on.\n");
    }

    /* Free the colormap privates */
    free(pCmapPriv);
    winSetCmapPriv(pColormap, NULL);

#if CYGDEBUG
    winDebug("winDestroyColormap - Returning\n");
#endif
}

/*
 * Internal function to load the palette used by the Shadow DIB
 */

static Bool
winGetPaletteDIB(ScreenPtr pScreen, ColormapPtr pcmap)
{
    winScreenPriv(pScreen);
    int i;
    Pixel pixel;                /* Pixel == CARD32 */
    CARD16 nRed, nGreen, nBlue; /* CARD16 == unsigned short */
    UINT uiColorsRetrieved = 0;
    RGBQUAD rgbColors[WIN_NUM_PALETTE_ENTRIES];

    /* Get the color table for the screen */
    uiColorsRetrieved = GetDIBColorTable(pScreenPriv->hdcScreen,
                                         0, WIN_NUM_PALETTE_ENTRIES, rgbColors);
    if (uiColorsRetrieved == 0) {
        ErrorF("winGetPaletteDIB - Could not retrieve screen color table\n");
        return FALSE;
    }

#if CYGDEBUG
    winDebug("winGetPaletteDIB - Retrieved %d colors from DIB\n",
             uiColorsRetrieved);
#endif

    /* Set the DIB color table to the default screen palette */
    if (SetDIBColorTable(pScreenPriv->hdcShadow,
                         0, uiColorsRetrieved, rgbColors) == 0) {
        ErrorF("winGetPaletteDIB - SetDIBColorTable () failed\n");
        return FALSE;
    }

    /* Alloc each color in the DIB color table */
    for (i = 0; i < uiColorsRetrieved; ++i) {
        pixel = i;

        /* Extract the color values for current palette entry */
        nRed = rgbColors[i].rgbRed << 8;
        nGreen = rgbColors[i].rgbGreen << 8;
        nBlue = rgbColors[i].rgbBlue << 8;

#if CYGDEBUG
        winDebug("winGetPaletteDIB - Allocating a color: %u; "
                 "%d %d %d\n", (unsigned int)pixel, nRed, nGreen, nBlue);
#endif

        /* Allocate a entry in the X colormap */
        if (AllocColor(pcmap, &nRed, &nGreen, &nBlue, &pixel, 0) != Success) {
            ErrorF("winGetPaletteDIB - AllocColor () failed, pixel %d\n", i);
            return FALSE;
        }

        if (i != pixel
            || nRed != rgbColors[i].rgbRed
            || nGreen != rgbColors[i].rgbGreen
            || nBlue != rgbColors[i].rgbBlue) {
            winDebug("winGetPaletteDIB - Got: %d; "
                     "%d %d %d\n", (int) pixel, nRed, nGreen, nBlue);
        }

        /* FIXME: Not sure that this bit is needed at all */
        pcmap->red[i].co.local.red = nRed;
        pcmap->red[i].co.local.green = nGreen;
        pcmap->red[i].co.local.blue = nBlue;
    }

    /* System is using a colormap */
    /* Set the black and white pixel indices */
    pScreen->whitePixel = uiColorsRetrieved - 1;
    pScreen->blackPixel = 0;

    return TRUE;
}

/*
 * Internal function to load the standard system palette being used by DD
 */

static Bool
winGetPaletteDD(ScreenPtr pScreen, ColormapPtr pcmap)
{
    int i;
    Pixel pixel;                /* Pixel == CARD32 */
    CARD16 nRed, nGreen, nBlue; /* CARD16 == unsigned short */
    UINT uiSystemPaletteEntries;
    LPPALETTEENTRY ppeColors = NULL;
    HDC hdc = NULL;

    /* Get a DC to obtain the default palette */
    hdc = GetDC(NULL);
    if (hdc == NULL) {
        ErrorF("winGetPaletteDD - Couldn't get a DC\n");
        return FALSE;
    }

    /* Get the number of entries in the system palette */
    uiSystemPaletteEntries = GetSystemPaletteEntries(hdc, 0, 0, NULL);
    if (uiSystemPaletteEntries == 0) {
        ErrorF("winGetPaletteDD - Unable to determine number of "
               "system palette entries\n");
        return FALSE;
    }

#if CYGDEBUG
    winDebug("winGetPaletteDD - uiSystemPaletteEntries %d\n",
             uiSystemPaletteEntries);
#endif

    /* Allocate palette entries structure */
    ppeColors = malloc(uiSystemPaletteEntries * sizeof(PALETTEENTRY));
    if (ppeColors == NULL) {
        ErrorF("winGetPaletteDD - malloc () for colormap failed\n");
        return FALSE;
    }

    /* Get system palette entries */
    GetSystemPaletteEntries(hdc, 0, uiSystemPaletteEntries, ppeColors);

    /* Allocate an X colormap entry for every system palette entry */
    for (i = 0; i < uiSystemPaletteEntries; ++i) {
        pixel = i;

        /* Extract the color values for current palette entry */
        nRed = ppeColors[i].peRed << 8;
        nGreen = ppeColors[i].peGreen << 8;
        nBlue = ppeColors[i].peBlue << 8;
#if CYGDEBUG
        winDebug("winGetPaletteDD - Allocating a color: %u; "
                 "%d %d %d\n", (unsigned int)pixel, nRed, nGreen, nBlue);
#endif
        if (AllocColor(pcmap, &nRed, &nGreen, &nBlue, &pixel, 0) != Success) {
            ErrorF("winGetPaletteDD - AllocColor () failed, pixel %d\n", i);
            free(ppeColors);
            ppeColors = NULL;
            return FALSE;
        }

        pcmap->red[i].co.local.red = nRed;
        pcmap->red[i].co.local.green = nGreen;
        pcmap->red[i].co.local.blue = nBlue;
    }

    /* System is using a colormap */
    /* Set the black and white pixel indices */
    pScreen->whitePixel = uiSystemPaletteEntries - 1;
    pScreen->blackPixel = 0;

    /* Free colormap */
    free(ppeColors);
    ppeColors = NULL;

    /* Free the DC */
    if (hdc != NULL) {
        ReleaseDC(NULL, hdc);
        hdc = NULL;
    }

    return TRUE;
}

/*
 * Install the standard fb colormap, or the GDI colormap,
 * depending on the current screen depth.
 */

Bool
winCreateDefColormap(ScreenPtr pScreen)
{
    winScreenPriv(pScreen);
    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
    unsigned short zero = 0, ones = 0xFFFF;
    VisualPtr pVisual = pScreenPriv->pRootVisual;
    ColormapPtr pcmap = NULL;
    Pixel wp, bp;

#if CYGDEBUG
    winDebug("winCreateDefColormap\n");
#endif

    /* Use standard fb colormaps for non palettized color modes */
    if (pScreenInfo->dwBPP > 8) {
        winDebug("winCreateDefColormap - Deferring to "
                 "fbCreateDefColormap ()\n");
        return fbCreateDefColormap(pScreen);
    }

    /*
     *  AllocAll for non-Dynamic visual classes,
     *  AllocNone for Dynamic visual classes.
     */

    /*
     * Dynamic visual classes allow the colors of the color map
     * to be changed by clients.
     */

#if CYGDEBUG
    winDebug("winCreateDefColormap - defColormap: %lu\n", pScreen->defColormap);
#endif

    /* Allocate an X colormap, owned by client 0 */
    if (CreateColormap(pScreen->defColormap,
                       pScreen,
                       pVisual,
                       &pcmap,
                       (pVisual->class & DynamicClass) ? AllocNone : AllocAll,
                       0) != Success) {
        ErrorF("winCreateDefColormap - CreateColormap failed\n");
        return FALSE;
    }
    if (pcmap == NULL) {
        ErrorF("winCreateDefColormap - Colormap could not be created\n");
        return FALSE;
    }

#if CYGDEBUG
    winDebug("winCreateDefColormap - Created a colormap\n");
#endif

    /* Branch on the visual class */
    if (!(pVisual->class & DynamicClass)) {
        /* Branch on engine type */
        if (pScreenInfo->dwEngine == WIN_SERVER_SHADOW_GDI) {
            /* Load the colors being used by the Shadow DIB */
            if (!winGetPaletteDIB(pScreen, pcmap)) {
                ErrorF("winCreateDefColormap - Couldn't get DIB colors\n");
                return FALSE;
            }
        }
        else {
            /* Load the colors from the default system palette */
            if (!winGetPaletteDD(pScreen, pcmap)) {
                ErrorF("winCreateDefColormap - Couldn't get colors "
                       "for DD\n");
                return FALSE;
            }
        }
    }
    else {
        wp = pScreen->whitePixel;
        bp = pScreen->blackPixel;

        /* Allocate a black and white pixel */
        if ((AllocColor(pcmap, &ones, &ones, &ones, &wp, 0) != Success)
            || (AllocColor(pcmap, &zero, &zero, &zero, &bp, 0) != Success)) {
            ErrorF("winCreateDefColormap - Couldn't allocate bp or wp\n");
            return FALSE;
        }

        pScreen->whitePixel = wp;
        pScreen->blackPixel = bp;

#if 0
        /* Have to reserve first 10 and last ten pixels in DirectDraw windowed */
        if (pScreenInfo->dwEngine != WIN_SERVER_SHADOW_GDI) {
            int k;
            Pixel p;

            for (k = 1; k < 10; ++k) {
                p = k;
                if (AllocColor(pcmap, &ones, &ones, &ones, &p, 0) != Success)
                    FatalError("Foo!\n");
            }

            for (k = 245; k < 255; ++k) {
                p = k;
                if (AllocColor(pcmap, &zero, &zero, &zero, &p, 0) != Success)
                    FatalError("Baz!\n");
            }
        }
#endif
    }

    /* Install the created colormap */
    (*pScreen->InstallColormap) (pcmap);

#if CYGDEBUG
    winDebug("winCreateDefColormap - Returning\n");
#endif

    return TRUE;
}
