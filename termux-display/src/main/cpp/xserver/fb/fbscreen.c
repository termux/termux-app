/*
 * Copyright © 1998 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "fb.h"

Bool
fbCloseScreen(ScreenPtr pScreen)
{
    int d;
    DepthPtr depths = pScreen->allowedDepths;

    fbDestroyGlyphCache();
    for (d = 0; d < pScreen->numDepths; d++)
        free(depths[d].vids);
    free(depths);
    free(pScreen->visuals);
    if (pScreen->devPrivate)
        FreePixmap((PixmapPtr)pScreen->devPrivate);
    return TRUE;
}

Bool
fbRealizeFont(ScreenPtr pScreen, FontPtr pFont)
{
    return TRUE;
}

Bool
fbUnrealizeFont(ScreenPtr pScreen, FontPtr pFont)
{
    return TRUE;
}

void
fbQueryBestSize(int class,
                unsigned short *width, unsigned short *height,
                ScreenPtr pScreen)
{
    unsigned short w;

    switch (class) {
    case CursorShape:
        if (*width > pScreen->width)
            *width = pScreen->width;
        if (*height > pScreen->height)
            *height = pScreen->height;
        break;
    case TileShape:
    case StippleShape:
        w = *width;
        if ((w & (w - 1)) && w < FB_UNIT) {
            for (w = 1; w < *width; w <<= 1);
            *width = w;
        }
    }
}

PixmapPtr
_fbGetWindowPixmap(WindowPtr pWindow)
{
    return fbGetWindowPixmap(pWindow);
}

void
_fbSetWindowPixmap(WindowPtr pWindow, PixmapPtr pPixmap)
{
    dixSetPrivate(&pWindow->devPrivates, fbGetWinPrivateKey(pWindow), pPixmap);
}

Bool
fbSetupScreen(ScreenPtr pScreen, void *pbits, /* pointer to screen bitmap */
              int xsize,        /* in pixels */
              int ysize, int dpix,      /* dots per inch */
              int dpiy, int width,      /* pixel width of frame buffer */
              int bpp)
{                               /* bits per pixel for screen */
    if (!fbAllocatePrivates(pScreen))
        return FALSE;
    pScreen->defColormap = FakeClientID(0);
    /* let CreateDefColormap do whatever it wants for pixels */
    pScreen->blackPixel = pScreen->whitePixel = (Pixel) 0;
    pScreen->QueryBestSize = fbQueryBestSize;
    /* SaveScreen */
    pScreen->GetImage = fbGetImage;
    pScreen->GetSpans = fbGetSpans;
    pScreen->CreateWindow = fbCreateWindow;
    pScreen->DestroyWindow = fbDestroyWindow;
    pScreen->PositionWindow = fbPositionWindow;
    pScreen->ChangeWindowAttributes = fbChangeWindowAttributes;
    pScreen->RealizeWindow = fbRealizeWindow;
    pScreen->UnrealizeWindow = fbUnrealizeWindow;
    pScreen->CopyWindow = fbCopyWindow;
    pScreen->CreatePixmap = fbCreatePixmap;
    pScreen->DestroyPixmap = fbDestroyPixmap;
    pScreen->RealizeFont = fbRealizeFont;
    pScreen->UnrealizeFont = fbUnrealizeFont;
    pScreen->CreateGC = fbCreateGC;
    pScreen->CreateColormap = fbInitializeColormap;
    pScreen->DestroyColormap = (void (*)(ColormapPtr)) NoopDDA;
    pScreen->InstallColormap = fbInstallColormap;
    pScreen->UninstallColormap = fbUninstallColormap;
    pScreen->ListInstalledColormaps = fbListInstalledColormaps;
    pScreen->StoreColors = (void (*)(ColormapPtr, int, xColorItem *)) NoopDDA;
    pScreen->ResolveColor = fbResolveColor;
    pScreen->BitmapToRegion = fbPixmapToRegion;

    pScreen->GetWindowPixmap = _fbGetWindowPixmap;
    pScreen->SetWindowPixmap = _fbSetWindowPixmap;

    return TRUE;
}

#ifdef FB_ACCESS_WRAPPER
Bool
wfbFinishScreenInit(ScreenPtr pScreen, void *pbits, int xsize, int ysize,
                    int dpix, int dpiy, int width, int bpp,
                    SetupWrapProcPtr setupWrap, FinishWrapProcPtr finishWrap)
#else
Bool
fbFinishScreenInit(ScreenPtr pScreen, void *pbits, int xsize, int ysize,
                   int dpix, int dpiy, int width, int bpp)
#endif
{
    VisualPtr visuals;
    DepthPtr depths;
    int nvisuals;
    int ndepths;
    int rootdepth;
    VisualID defaultVisual;

#ifdef FB_DEBUG
    int stride;

    ysize -= 2;
    stride = (width * bpp) / 8;
    fbSetBits((FbStip *) pbits, stride / sizeof(FbStip), FB_HEAD_BITS);
    pbits = (void *) ((char *) pbits + stride);
    fbSetBits((FbStip *) ((char *) pbits + stride * ysize),
              stride / sizeof(FbStip), FB_TAIL_BITS);
#endif
    /* fb requires power-of-two bpp */
    if (Ones(bpp) != 1)
        return FALSE;
#ifdef FB_ACCESS_WRAPPER
    fbGetScreenPrivate(pScreen)->setupWrap = setupWrap;
    fbGetScreenPrivate(pScreen)->finishWrap = finishWrap;
#endif
    rootdepth = 0;
    if (!fbInitVisuals(&visuals, &depths, &nvisuals, &ndepths, &rootdepth,
                       &defaultVisual, ((unsigned long) 1 << (bpp - 1)),
                       8))
        return FALSE;
    if (!miScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width,
                      rootdepth, ndepths, depths,
                      defaultVisual, nvisuals, visuals))
        return FALSE;
    /* overwrite miCloseScreen with our own */
    pScreen->CloseScreen = fbCloseScreen;
    return TRUE;
}

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
#ifdef FB_ACCESS_WRAPPER
Bool
wfbScreenInit(ScreenPtr pScreen, void *pbits, int xsize, int ysize,
              int dpix, int dpiy, int width, int bpp,
              SetupWrapProcPtr setupWrap, FinishWrapProcPtr finishWrap)
{
    if (!fbSetupScreen(pScreen, pbits, xsize, ysize, dpix, dpiy, width, bpp))
        return FALSE;
    if (!wfbFinishScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy,
                             width, bpp, setupWrap, finishWrap))
        return FALSE;
    return TRUE;
}
#else
Bool
fbScreenInit(ScreenPtr pScreen, void *pbits, int xsize, int ysize,
             int dpix, int dpiy, int width, int bpp)
{
    if (!fbSetupScreen(pScreen, pbits, xsize, ysize, dpix, dpiy, width, bpp))
        return FALSE;
    if (!fbFinishScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy,
                            width, bpp))
        return FALSE;
    return TRUE;
}
#endif
