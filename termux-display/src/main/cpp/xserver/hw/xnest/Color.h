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

#ifndef XNESTCOLOR_H
#define XNESTCOLOR_H

#define DUMB_WINDOW_MANAGERS

#define MAXCMAPS 1
#define MINCMAPS 1

typedef struct {
    Colormap colormap;
} xnestPrivColormap;

typedef struct {
    int numCmapIDs;
    Colormap *cmapIDs;
    int numWindows;
    Window *windows;
    int index;
} xnestInstalledColormapWindows;

extern DevPrivateKeyRec xnestColormapPrivateKeyRec;

#define xnestColormapPriv(pCmap) \
  ((xnestPrivColormap *) dixLookupPrivate(&(pCmap)->devPrivates, &xnestColormapPrivateKeyRec))

#define xnestColormap(pCmap) (xnestColormapPriv(pCmap)->colormap)

#define xnestPixel(pixel) (pixel)

Bool xnestCreateColormap(ColormapPtr pCmap);
void xnestDestroyColormap(ColormapPtr pCmap);
void xnestSetInstalledColormapWindows(ScreenPtr pScreen);
void xnestSetScreenSaverColormapWindow(ScreenPtr pScreen);
void xnestDirectInstallColormaps(ScreenPtr pScreen);
void xnestDirectUninstallColormaps(ScreenPtr pScreen);
void xnestInstallColormap(ColormapPtr pCmap);
void xnestUninstallColormap(ColormapPtr pCmap);
int xnestListInstalledColormaps(ScreenPtr pScreen, Colormap * pCmapIDs);
void xnestStoreColors(ColormapPtr pCmap, int nColors, xColorItem * pColors);
void xnestResolveColor(unsigned short *pRed, unsigned short *pGreen,
                       unsigned short *pBlue, VisualPtr pVisual);
Bool xnestCreateDefaultColormap(ScreenPtr pScreen);

#endif                          /* XNESTCOLOR_H */
