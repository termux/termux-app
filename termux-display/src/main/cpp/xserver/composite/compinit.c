/*
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright © 2003 Keith Packard
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

#include "compint.h"
#include "compositeext.h"

DevPrivateKeyRec CompScreenPrivateKeyRec;
DevPrivateKeyRec CompWindowPrivateKeyRec;
DevPrivateKeyRec CompSubwindowsPrivateKeyRec;

static Bool
compCloseScreen(ScreenPtr pScreen)
{
    CompScreenPtr cs = GetCompScreen(pScreen);
    Bool ret;

    free(cs->alternateVisuals);

    pScreen->CloseScreen = cs->CloseScreen;
    pScreen->InstallColormap = cs->InstallColormap;
    pScreen->ChangeWindowAttributes = cs->ChangeWindowAttributes;
    pScreen->ReparentWindow = cs->ReparentWindow;
    pScreen->ConfigNotify = cs->ConfigNotify;
    pScreen->MoveWindow = cs->MoveWindow;
    pScreen->ResizeWindow = cs->ResizeWindow;
    pScreen->ChangeBorderWidth = cs->ChangeBorderWidth;

    pScreen->ClipNotify = cs->ClipNotify;
    pScreen->UnrealizeWindow = cs->UnrealizeWindow;
    pScreen->RealizeWindow = cs->RealizeWindow;
    pScreen->DestroyWindow = cs->DestroyWindow;
    pScreen->CreateWindow = cs->CreateWindow;
    pScreen->CopyWindow = cs->CopyWindow;
    pScreen->PositionWindow = cs->PositionWindow;
    pScreen->SourceValidate = cs->SourceValidate;

    free(cs);
    dixSetPrivate(&pScreen->devPrivates, CompScreenPrivateKey, NULL);
    ret = (*pScreen->CloseScreen) (pScreen);

    return ret;
}

static void
compInstallColormap(ColormapPtr pColormap)
{
    VisualPtr pVisual = pColormap->pVisual;
    ScreenPtr pScreen = pColormap->pScreen;
    CompScreenPtr cs = GetCompScreen(pScreen);
    int a;

    for (a = 0; a < cs->numAlternateVisuals; a++)
        if (pVisual->vid == cs->alternateVisuals[a])
            return;
    pScreen->InstallColormap = cs->InstallColormap;
    (*pScreen->InstallColormap) (pColormap);
    cs->InstallColormap = pScreen->InstallColormap;
    pScreen->InstallColormap = compInstallColormap;
}

static void
compCheckBackingStore(WindowPtr pWin)
{
    if (pWin->backingStore != NotUseful) {
        compRedirectWindow(serverClient, pWin, CompositeRedirectAutomatic);
    }
    else {
        compUnredirectWindow(serverClient, pWin,
                             CompositeRedirectAutomatic);
    }
}

/* Fake backing store via automatic redirection */
static Bool
compChangeWindowAttributes(WindowPtr pWin, unsigned long mask)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    CompScreenPtr cs = GetCompScreen(pScreen);
    Bool ret;

    pScreen->ChangeWindowAttributes = cs->ChangeWindowAttributes;
    ret = pScreen->ChangeWindowAttributes(pWin, mask);

    if (ret && (mask & CWBackingStore) &&
        pScreen->backingStoreSupport != NotUseful)
        compCheckBackingStore(pWin);

    pScreen->ChangeWindowAttributes = compChangeWindowAttributes;

    return ret;
}

static void
compSourceValidate(DrawablePtr pDrawable,
                   int x, int y,
                   int width, int height, unsigned int subWindowMode)
{
    ScreenPtr pScreen = pDrawable->pScreen;
    CompScreenPtr cs = GetCompScreen(pScreen);

    pScreen->SourceValidate = cs->SourceValidate;
    if (pDrawable->type == DRAWABLE_WINDOW && subWindowMode == IncludeInferiors)
        compPaintChildrenToWindow((WindowPtr) pDrawable);
    (*pScreen->SourceValidate) (pDrawable, x, y, width, height,
                                subWindowMode);
    cs->SourceValidate = pScreen->SourceValidate;
    pScreen->SourceValidate = compSourceValidate;
}

/*
 * Add alternate visuals -- always expose an ARGB32 and RGB24 visual
 */

static DepthPtr
compFindVisuallessDepth(ScreenPtr pScreen, int d)
{
    int i;

    for (i = 0; i < pScreen->numDepths; i++) {
        DepthPtr depth = &pScreen->allowedDepths[i];

        if (depth->depth == d) {
            /*
             * Make sure it doesn't have visuals already
             */
            if (depth->numVids)
                return 0;
            /*
             * looks fine
             */
            return depth;
        }
    }
    /*
     * If there isn't one, then it's gonna be hard to have
     * an associated visual
     */
    return 0;
}

/*
 * Add a list of visual IDs to the list of visuals to implicitly redirect.
 */
static Bool
compRegisterAlternateVisuals(CompScreenPtr cs, VisualID * vids, int nVisuals)
{
    VisualID *p;

    p = reallocarray(cs->alternateVisuals,
                     cs->numAlternateVisuals + nVisuals, sizeof(VisualID));
    if (p == NULL)
        return FALSE;

    memcpy(&p[cs->numAlternateVisuals], vids, sizeof(VisualID) * nVisuals);

    cs->alternateVisuals = p;
    cs->numAlternateVisuals += nVisuals;

    return TRUE;
}

Bool
CompositeRegisterAlternateVisuals(ScreenPtr pScreen, VisualID * vids,
                                  int nVisuals)
{
    CompScreenPtr cs = GetCompScreen(pScreen);

    return compRegisterAlternateVisuals(cs, vids, nVisuals);
}

Bool
CompositeRegisterImplicitRedirectionException(ScreenPtr pScreen,
                                              VisualID parentVisual,
                                              VisualID winVisual)
{
    CompScreenPtr cs = GetCompScreen(pScreen);
    CompImplicitRedirectException *p;

    p = reallocarray(cs->implicitRedirectExceptions,
                     cs->numImplicitRedirectExceptions + 1, sizeof(p[0]));
    if (p == NULL)
        return FALSE;

    p[cs->numImplicitRedirectExceptions].parentVisual = parentVisual;
    p[cs->numImplicitRedirectExceptions].winVisual = winVisual;

    cs->implicitRedirectExceptions = p;
    cs->numImplicitRedirectExceptions++;

    return TRUE;
}

typedef struct _alternateVisual {
    int depth;
    CARD32 format;
} CompAlternateVisual;

static CompAlternateVisual altVisuals[] = {
#if COMP_INCLUDE_RGB24_VISUAL
    {24, PICT_r8g8b8},
#endif
    {32, PICT_a8r8g8b8},
};

static Bool
compAddAlternateVisual(ScreenPtr pScreen, CompScreenPtr cs,
                       CompAlternateVisual * alt)
{
    VisualPtr visual;
    DepthPtr depth;
    PictFormatPtr pPictFormat;
    unsigned long alphaMask;

    /*
     * The ARGB32 visual is always available.  Other alternate depth visuals
     * are only provided if their depth is less than the root window depth.
     * There's no deep reason for this.
     */
    if (alt->depth >= pScreen->rootDepth && alt->depth != 32)
        return FALSE;

    depth = compFindVisuallessDepth(pScreen, alt->depth);
    if (!depth)
        /* alt->depth doesn't exist or already has alternate visuals. */
        return TRUE;

    pPictFormat = PictureMatchFormat(pScreen, alt->depth, alt->format);
    if (!pPictFormat)
        return FALSE;

    if (ResizeVisualArray(pScreen, 1, depth) == FALSE) {
        return FALSE;
    }

    visual = pScreen->visuals + (pScreen->numVisuals - 1);      /* the new one */

    /* Initialize the visual */
    visual->bitsPerRGBValue = 8;
    if (PICT_FORMAT_TYPE(alt->format) == PICT_TYPE_COLOR) {
        visual->class = PseudoColor;
        visual->nplanes = PICT_FORMAT_BPP(alt->format);
        visual->ColormapEntries = 1 << visual->nplanes;
    }
    else {
        DirectFormatRec *direct = &pPictFormat->direct;

        visual->class = TrueColor;
        visual->redMask = ((unsigned long) direct->redMask) << direct->red;
        visual->greenMask =
            ((unsigned long) direct->greenMask) << direct->green;
        visual->blueMask = ((unsigned long) direct->blueMask) << direct->blue;
        alphaMask = ((unsigned long) direct->alphaMask) << direct->alpha;
        visual->offsetRed = direct->red;
        visual->offsetGreen = direct->green;
        visual->offsetBlue = direct->blue;
        /*
         * Include A bits in this (unlike GLX which includes only RGB)
         * This lets DIX compute suitable masks for colormap allocations
         */
        visual->nplanes = Ones(visual->redMask |
                               visual->greenMask |
                               visual->blueMask | alphaMask);
        /* find widest component */
        visual->ColormapEntries = (1 << max(Ones(visual->redMask),
                                            max(Ones(visual->greenMask),
                                                Ones(visual->blueMask))));
    }

    /* remember the visual ID to detect auto-update windows */
    compRegisterAlternateVisuals(cs, &visual->vid, 1);

    return TRUE;
}

static Bool
compAddAlternateVisuals(ScreenPtr pScreen, CompScreenPtr cs)
{
    int alt, ret = 0;

    for (alt = 0; alt < ARRAY_SIZE(altVisuals); alt++)
        ret |= compAddAlternateVisual(pScreen, cs, altVisuals + alt);

    return ! !ret;
}

Bool
compScreenInit(ScreenPtr pScreen)
{
    CompScreenPtr cs;

    if (!dixRegisterPrivateKey(&CompScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;
    if (!dixRegisterPrivateKey(&CompWindowPrivateKeyRec, PRIVATE_WINDOW, 0))
        return FALSE;
    if (!dixRegisterPrivateKey(&CompSubwindowsPrivateKeyRec, PRIVATE_WINDOW, 0))
        return FALSE;

    if (GetCompScreen(pScreen))
        return TRUE;
    cs = (CompScreenPtr) malloc(sizeof(CompScreenRec));
    if (!cs)
        return FALSE;

    cs->overlayWid = FakeClientID(0);
    cs->pOverlayWin = NULL;
    cs->pOverlayClients = NULL;

    cs->pendingScreenUpdate = FALSE;

    cs->numAlternateVisuals = 0;
    cs->alternateVisuals = NULL;
    cs->numImplicitRedirectExceptions = 0;
    cs->implicitRedirectExceptions = NULL;

    if (!compAddAlternateVisuals(pScreen, cs)) {
        free(cs);
        return FALSE;
    }

    if (!disableBackingStore)
        pScreen->backingStoreSupport = WhenMapped;

    cs->PositionWindow = pScreen->PositionWindow;
    pScreen->PositionWindow = compPositionWindow;

    cs->CopyWindow = pScreen->CopyWindow;
    pScreen->CopyWindow = compCopyWindow;

    cs->CreateWindow = pScreen->CreateWindow;
    pScreen->CreateWindow = compCreateWindow;

    cs->DestroyWindow = pScreen->DestroyWindow;
    pScreen->DestroyWindow = compDestroyWindow;

    cs->RealizeWindow = pScreen->RealizeWindow;
    pScreen->RealizeWindow = compRealizeWindow;

    cs->UnrealizeWindow = pScreen->UnrealizeWindow;
    pScreen->UnrealizeWindow = compUnrealizeWindow;

    cs->ClipNotify = pScreen->ClipNotify;
    pScreen->ClipNotify = compClipNotify;

    cs->ConfigNotify = pScreen->ConfigNotify;
    pScreen->ConfigNotify = compConfigNotify;

    cs->MoveWindow = pScreen->MoveWindow;
    pScreen->MoveWindow = compMoveWindow;

    cs->ResizeWindow = pScreen->ResizeWindow;
    pScreen->ResizeWindow = compResizeWindow;

    cs->ChangeBorderWidth = pScreen->ChangeBorderWidth;
    pScreen->ChangeBorderWidth = compChangeBorderWidth;

    cs->ReparentWindow = pScreen->ReparentWindow;
    pScreen->ReparentWindow = compReparentWindow;

    cs->InstallColormap = pScreen->InstallColormap;
    pScreen->InstallColormap = compInstallColormap;

    cs->ChangeWindowAttributes = pScreen->ChangeWindowAttributes;
    pScreen->ChangeWindowAttributes = compChangeWindowAttributes;

    cs->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = compCloseScreen;

    cs->SourceValidate = pScreen->SourceValidate;
    pScreen->SourceValidate = compSourceValidate;

    dixSetPrivate(&pScreen->devPrivates, CompScreenPrivateKey, cs);

    RegisterRealChildHeadProc(CompositeRealChildHead);

    return TRUE;
}
