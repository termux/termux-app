/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "window.h"
#include "windowstr.h"
#include "colormapst.h"
#include "resource.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "Color.h"
#include "Visual.h"
#include "XNWindow.h"
#include "Args.h"

DevPrivateKeyRec xnestColormapPrivateKeyRec;

static DevPrivateKeyRec cmapScrPrivateKeyRec;

#define cmapScrPrivateKey (&cmapScrPrivateKeyRec)

#define GetInstalledColormap(s) ((ColormapPtr) dixLookupPrivate(&(s)->devPrivates, cmapScrPrivateKey))
#define SetInstalledColormap(s,c) (dixSetPrivate(&(s)->devPrivates, cmapScrPrivateKey, c))

Bool
xnestCreateColormap(ColormapPtr pCmap)
{
    VisualPtr pVisual;
    XColor *colors;
    int i, ncolors;
    Pixel red, green, blue;
    Pixel redInc, greenInc, blueInc;

    pVisual = pCmap->pVisual;
    ncolors = pVisual->ColormapEntries;

    xnestColormapPriv(pCmap)->colormap =
        XCreateColormap(xnestDisplay,
                        xnestDefaultWindows[pCmap->pScreen->myNum],
                        xnestVisual(pVisual),
                        (pVisual->class & DynamicClass) ? AllocAll : AllocNone);

    switch (pVisual->class) {
    case StaticGray:           /* read only */
        colors = xallocarray(ncolors, sizeof(XColor));
        for (i = 0; i < ncolors; i++)
            colors[i].pixel = i;
        XQueryColors(xnestDisplay, xnestColormap(pCmap), colors, ncolors);
        for (i = 0; i < ncolors; i++) {
            pCmap->red[i].co.local.red = colors[i].red;
            pCmap->red[i].co.local.green = colors[i].red;
            pCmap->red[i].co.local.blue = colors[i].red;
        }
        free(colors);
        break;

    case StaticColor:          /* read only */
        colors = xallocarray(ncolors, sizeof(XColor));
        for (i = 0; i < ncolors; i++)
            colors[i].pixel = i;
        XQueryColors(xnestDisplay, xnestColormap(pCmap), colors, ncolors);
        for (i = 0; i < ncolors; i++) {
            pCmap->red[i].co.local.red = colors[i].red;
            pCmap->red[i].co.local.green = colors[i].green;
            pCmap->red[i].co.local.blue = colors[i].blue;
        }
        free(colors);
        break;

    case TrueColor:            /* read only */
        colors = xallocarray(ncolors, sizeof(XColor));
        red = green = blue = 0L;
        redInc = lowbit(pVisual->redMask);
        greenInc = lowbit(pVisual->greenMask);
        blueInc = lowbit(pVisual->blueMask);
        for (i = 0; i < ncolors; i++) {
            colors[i].pixel = red | green | blue;
            red += redInc;
            if (red > pVisual->redMask)
                red = 0L;
            green += greenInc;
            if (green > pVisual->greenMask)
                green = 0L;
            blue += blueInc;
            if (blue > pVisual->blueMask)
                blue = 0L;
        }
        XQueryColors(xnestDisplay, xnestColormap(pCmap), colors, ncolors);
        for (i = 0; i < ncolors; i++) {
            pCmap->red[i].co.local.red = colors[i].red;
            pCmap->green[i].co.local.green = colors[i].green;
            pCmap->blue[i].co.local.blue = colors[i].blue;
        }
        free(colors);
        break;

    case GrayScale:            /* read and write */
        break;

    case PseudoColor:          /* read and write */
        break;

    case DirectColor:          /* read and write */
        break;
    }

    return True;
}

void
xnestDestroyColormap(ColormapPtr pCmap)
{
    XFreeColormap(xnestDisplay, xnestColormap(pCmap));
}

#define SEARCH_PREDICATE \
  (xnestWindow(pWin) != None && wColormap(pWin) == icws->cmapIDs[i])

static int
xnestCountInstalledColormapWindows(WindowPtr pWin, void *ptr)
{
    xnestInstalledColormapWindows *icws = (xnestInstalledColormapWindows *) ptr;
    int i;

    for (i = 0; i < icws->numCmapIDs; i++)
        if (SEARCH_PREDICATE) {
            icws->numWindows++;
            return WT_DONTWALKCHILDREN;
        }

    return WT_WALKCHILDREN;
}

static int
xnestGetInstalledColormapWindows(WindowPtr pWin, void *ptr)
{
    xnestInstalledColormapWindows *icws = (xnestInstalledColormapWindows *) ptr;
    int i;

    for (i = 0; i < icws->numCmapIDs; i++)
        if (SEARCH_PREDICATE) {
            icws->windows[icws->index++] = xnestWindow(pWin);
            return WT_DONTWALKCHILDREN;
        }

    return WT_WALKCHILDREN;
}

static Window *xnestOldInstalledColormapWindows = NULL;
static int xnestNumOldInstalledColormapWindows = 0;

static Bool
xnestSameInstalledColormapWindows(Window *windows, int numWindows)
{
    if (xnestNumOldInstalledColormapWindows != numWindows)
        return False;

    if (xnestOldInstalledColormapWindows == windows)
        return True;

    if (xnestOldInstalledColormapWindows == NULL || windows == NULL)
        return False;

    if (memcmp(xnestOldInstalledColormapWindows, windows,
               numWindows * sizeof(Window)))
        return False;

    return True;
}

void
xnestSetInstalledColormapWindows(ScreenPtr pScreen)
{
    xnestInstalledColormapWindows icws;
    int numWindows;

    icws.cmapIDs = xallocarray(pScreen->maxInstalledCmaps, sizeof(Colormap));
    icws.numCmapIDs = xnestListInstalledColormaps(pScreen, icws.cmapIDs);
    icws.numWindows = 0;
    WalkTree(pScreen, xnestCountInstalledColormapWindows, (void *) &icws);
    if (icws.numWindows) {
        icws.windows = xallocarray(icws.numWindows + 1, sizeof(Window));
        icws.index = 0;
        WalkTree(pScreen, xnestGetInstalledColormapWindows, (void *) &icws);
        icws.windows[icws.numWindows] = xnestDefaultWindows[pScreen->myNum];
        numWindows = icws.numWindows + 1;
    }
    else {
        icws.windows = NULL;
        numWindows = 0;
    }

    free(icws.cmapIDs);

    if (!xnestSameInstalledColormapWindows(icws.windows, icws.numWindows)) {
        free(xnestOldInstalledColormapWindows);

#ifdef _XSERVER64
        {
            int i;
            Window64 *windows = xallocarray(numWindows, sizeof(Window64));

            for (i = 0; i < numWindows; ++i)
                windows[i] = icws.windows[i];
            XSetWMColormapWindows(xnestDisplay,
                                  xnestDefaultWindows[pScreen->myNum], windows,
                                  numWindows);
            free(windows);
        }
#else
        XSetWMColormapWindows(xnestDisplay, xnestDefaultWindows[pScreen->myNum],
                              icws.windows, numWindows);
#endif

        xnestOldInstalledColormapWindows = icws.windows;
        xnestNumOldInstalledColormapWindows = icws.numWindows;

#ifdef DUMB_WINDOW_MANAGERS
        /*
           This code is for dumb window managers.
           This will only work with default local visual colormaps.
         */
        if (icws.numWindows) {
            WindowPtr pWin;
            Visual *visual;
            ColormapPtr pCmap;

            pWin = xnestWindowPtr(icws.windows[0]);
            visual = xnestVisualFromID(pScreen, wVisual(pWin));

            if (visual == xnestDefaultVisual(pScreen))
                dixLookupResourceByType((void **) &pCmap, wColormap(pWin),
                                        RT_COLORMAP, serverClient,
                                        DixUseAccess);
            else
                dixLookupResourceByType((void **) &pCmap,
                                        pScreen->defColormap, RT_COLORMAP,
                                        serverClient, DixUseAccess);

            XSetWindowColormap(xnestDisplay,
                               xnestDefaultWindows[pScreen->myNum],
                               xnestColormap(pCmap));
        }
#endif                          /* DUMB_WINDOW_MANAGERS */
    }
    else
        free(icws.windows);
}

void
xnestSetScreenSaverColormapWindow(ScreenPtr pScreen)
{
    free(xnestOldInstalledColormapWindows);

#ifdef _XSERVER64
    {
        Window64 window;

        window = xnestScreenSaverWindows[pScreen->myNum];
        XSetWMColormapWindows(xnestDisplay, xnestDefaultWindows[pScreen->myNum],
                              &window, 1);
        xnestScreenSaverWindows[pScreen->myNum] = window;
    }
#else
    XSetWMColormapWindows(xnestDisplay, xnestDefaultWindows[pScreen->myNum],
                          &xnestScreenSaverWindows[pScreen->myNum], 1);
#endif                          /* _XSERVER64 */

    xnestOldInstalledColormapWindows = NULL;
    xnestNumOldInstalledColormapWindows = 0;

    xnestDirectUninstallColormaps(pScreen);
}

void
xnestDirectInstallColormaps(ScreenPtr pScreen)
{
    int i, n;
    Colormap pCmapIDs[MAXCMAPS];

    if (!xnestDoDirectColormaps)
        return;

    n = (*pScreen->ListInstalledColormaps) (pScreen, pCmapIDs);

    for (i = 0; i < n; i++) {
        ColormapPtr pCmap;

        dixLookupResourceByType((void **) &pCmap, pCmapIDs[i], RT_COLORMAP,
                                serverClient, DixInstallAccess);
        if (pCmap)
            XInstallColormap(xnestDisplay, xnestColormap(pCmap));
    }
}

void
xnestDirectUninstallColormaps(ScreenPtr pScreen)
{
    int i, n;
    Colormap pCmapIDs[MAXCMAPS];

    if (!xnestDoDirectColormaps)
        return;

    n = (*pScreen->ListInstalledColormaps) (pScreen, pCmapIDs);

    for (i = 0; i < n; i++) {
        ColormapPtr pCmap;

        dixLookupResourceByType((void **) &pCmap, pCmapIDs[i], RT_COLORMAP,
                                serverClient, DixUninstallAccess);
        if (pCmap)
            XUninstallColormap(xnestDisplay, xnestColormap(pCmap));
    }
}

void
xnestInstallColormap(ColormapPtr pCmap)
{
    ColormapPtr pOldCmap = GetInstalledColormap(pCmap->pScreen);

    if (pCmap != pOldCmap) {
        xnestDirectUninstallColormaps(pCmap->pScreen);

        /* Uninstall pInstalledMap. Notify all interested parties. */
        if (pOldCmap != (ColormapPtr) None)
            WalkTree(pCmap->pScreen, TellLostMap, (void *) &pOldCmap->mid);

        SetInstalledColormap(pCmap->pScreen, pCmap);
        WalkTree(pCmap->pScreen, TellGainedMap, (void *) &pCmap->mid);

        xnestSetInstalledColormapWindows(pCmap->pScreen);
        xnestDirectInstallColormaps(pCmap->pScreen);
    }
}

void
xnestUninstallColormap(ColormapPtr pCmap)
{
    ColormapPtr pCurCmap = GetInstalledColormap(pCmap->pScreen);

    if (pCmap == pCurCmap) {
        if (pCmap->mid != pCmap->pScreen->defColormap) {
            dixLookupResourceByType((void **) &pCurCmap,
                                    pCmap->pScreen->defColormap,
                                    RT_COLORMAP,
                                    serverClient, DixInstallAccess);
            (*pCmap->pScreen->InstallColormap) (pCurCmap);
        }
    }
}

static Bool xnestInstalledDefaultColormap = False;

int
xnestListInstalledColormaps(ScreenPtr pScreen, Colormap * pCmapIDs)
{
    if (xnestInstalledDefaultColormap) {
        *pCmapIDs = GetInstalledColormap(pScreen)->mid;
        return 1;
    }
    else
        return 0;
}

void
xnestStoreColors(ColormapPtr pCmap, int nColors, xColorItem * pColors)
{
    if (pCmap->pVisual->class & DynamicClass)
#ifdef _XSERVER64
    {
        int i;
        XColor *pColors64 = xallocarray(nColors, sizeof(XColor));

        for (i = 0; i < nColors; ++i) {
            pColors64[i].pixel = pColors[i].pixel;
            pColors64[i].red = pColors[i].red;
            pColors64[i].green = pColors[i].green;
            pColors64[i].blue = pColors[i].blue;
            pColors64[i].flags = pColors[i].flags;
        }
        XStoreColors(xnestDisplay, xnestColormap(pCmap), pColors64, nColors);
        free(pColors64);
    }
#else
        XStoreColors(xnestDisplay, xnestColormap(pCmap),
                     (XColor *) pColors, nColors);
#endif
}

void
xnestResolveColor(unsigned short *pRed, unsigned short *pGreen,
                  unsigned short *pBlue, VisualPtr pVisual)
{
    int shift;
    unsigned int lim;

    shift = 16 - pVisual->bitsPerRGBValue;
    lim = (1 << pVisual->bitsPerRGBValue) - 1;

    if ((pVisual->class == PseudoColor) || (pVisual->class == DirectColor)) {
        /* rescale to rgb bits */
        *pRed = ((*pRed >> shift) * 65535) / lim;
        *pGreen = ((*pGreen >> shift) * 65535) / lim;
        *pBlue = ((*pBlue >> shift) * 65535) / lim;
    }
    else if (pVisual->class == GrayScale) {
        /* rescale to gray then rgb bits */
        *pRed = (30L * *pRed + 59L * *pGreen + 11L * *pBlue) / 100;
        *pBlue = *pGreen = *pRed = ((*pRed >> shift) * 65535) / lim;
    }
    else if (pVisual->class == StaticGray) {
        unsigned int limg;

        limg = pVisual->ColormapEntries - 1;
        /* rescale to gray then [0..limg] then [0..65535] then rgb bits */
        *pRed = (30L * *pRed + 59L * *pGreen + 11L * *pBlue) / 100;
        *pRed = ((((*pRed * (limg + 1))) >> 16) * 65535) / limg;
        *pBlue = *pGreen = *pRed = ((*pRed >> shift) * 65535) / lim;
    }
    else {
        unsigned limr, limg, limb;

        limr = pVisual->redMask >> pVisual->offsetRed;
        limg = pVisual->greenMask >> pVisual->offsetGreen;
        limb = pVisual->blueMask >> pVisual->offsetBlue;
        /* rescale to [0..limN] then [0..65535] then rgb bits */
        *pRed = ((((((*pRed * (limr + 1)) >> 16) *
                    65535) / limr) >> shift) * 65535) / lim;
        *pGreen = ((((((*pGreen * (limg + 1)) >> 16) *
                      65535) / limg) >> shift) * 65535) / lim;
        *pBlue = ((((((*pBlue * (limb + 1)) >> 16) *
                     65535) / limb) >> shift) * 65535) / lim;
    }
}

Bool
xnestCreateDefaultColormap(ScreenPtr pScreen)
{
    VisualPtr pVisual;
    ColormapPtr pCmap;
    unsigned short zero = 0, ones = 0xFFFF;
    Pixel wp, bp;

    if (!dixRegisterPrivateKey(&cmapScrPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    for (pVisual = pScreen->visuals;
         pVisual->vid != pScreen->rootVisual; pVisual++);

    if (CreateColormap(pScreen->defColormap, pScreen, pVisual, &pCmap,
                       (pVisual->class & DynamicClass) ? AllocNone : AllocAll,
                       0)
        != Success)
        return False;

    wp = pScreen->whitePixel;
    bp = pScreen->blackPixel;
    if ((AllocColor(pCmap, &ones, &ones, &ones, &wp, 0) !=
         Success) ||
        (AllocColor(pCmap, &zero, &zero, &zero, &bp, 0) != Success))
        return FALSE;
    pScreen->whitePixel = wp;
    pScreen->blackPixel = bp;
    (*pScreen->InstallColormap) (pCmap);

    xnestInstalledDefaultColormap = True;

    return True;
}
