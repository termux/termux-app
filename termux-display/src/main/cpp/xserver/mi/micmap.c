/*
 * Copyright (c) 1987, Oracle and/or its affiliates. All rights reserved.
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
 */

/*
 * This is based on cfbcmap.c.  The functions here are useful independently
 * of cfb, which is the reason for including them here.  How "mi" these
 * are may be debatable.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "colormapst.h"
#include "resource.h"
#include "globals.h"
#include "micmap.h"

DevPrivateKeyRec micmapScrPrivateKeyRec;

int
miListInstalledColormaps(ScreenPtr pScreen, Colormap * pmaps)
{
    if (GetInstalledmiColormap(pScreen)) {
        *pmaps = GetInstalledmiColormap(pScreen)->mid;
        return 1;
    }
    return 0;
}

void
miInstallColormap(ColormapPtr pmap)
{
    ColormapPtr oldpmap = GetInstalledmiColormap(pmap->pScreen);

    if (pmap != oldpmap) {
        /* Uninstall pInstalledMap. No hardware changes required, just
         * notify all interested parties. */
        if (oldpmap != (ColormapPtr) None)
            WalkTree(pmap->pScreen, TellLostMap, (char *) &oldpmap->mid);
        /* Install pmap */
        SetInstalledmiColormap(pmap->pScreen, pmap);
        WalkTree(pmap->pScreen, TellGainedMap, (char *) &pmap->mid);

    }
}

void
miUninstallColormap(ColormapPtr pmap)
{
    ColormapPtr curpmap = GetInstalledmiColormap(pmap->pScreen);

    if (pmap == curpmap) {
        if (pmap->mid != pmap->pScreen->defColormap) {
            dixLookupResourceByType((void **) &curpmap,
                                    pmap->pScreen->defColormap,
                                    RT_COLORMAP, serverClient, DixUseAccess);
            (*pmap->pScreen->InstallColormap) (curpmap);
        }
    }
}

void
miResolveColor(unsigned short *pred, unsigned short *pgreen,
               unsigned short *pblue, VisualPtr pVisual)
{
    int shift = 16 - pVisual->bitsPerRGBValue;
    unsigned lim = (1 << pVisual->bitsPerRGBValue) - 1;

    if ((pVisual->class | DynamicClass) == GrayScale) {
        /* rescale to gray then rgb bits */
        *pred = (30L * *pred + 59L * *pgreen + 11L * *pblue) / 100;
        *pblue = *pgreen = *pred = ((*pred >> shift) * 65535) / lim;
    }
    else {
        /* rescale to rgb bits */
        *pred = ((*pred >> shift) * 65535) / lim;
        *pgreen = ((*pgreen >> shift) * 65535) / lim;
        *pblue = ((*pblue >> shift) * 65535) / lim;
    }
}

Bool
miInitializeColormap(ColormapPtr pmap)
{
    unsigned i;
    VisualPtr pVisual;
    unsigned lim, maxent, shift;

    pVisual = pmap->pVisual;
    lim = (1 << pVisual->bitsPerRGBValue) - 1;
    shift = 16 - pVisual->bitsPerRGBValue;
    maxent = pVisual->ColormapEntries - 1;
    if (pVisual->class == TrueColor) {
        unsigned limr, limg, limb;

        limr = pVisual->redMask >> pVisual->offsetRed;
        limg = pVisual->greenMask >> pVisual->offsetGreen;
        limb = pVisual->blueMask >> pVisual->offsetBlue;
        for (i = 0; i <= maxent; i++) {
            /* rescale to [0..65535] then rgb bits */
            pmap->red[i].co.local.red =
                ((((i * 65535) / limr) >> shift) * 65535) / lim;
            pmap->green[i].co.local.green =
                ((((i * 65535) / limg) >> shift) * 65535) / lim;
            pmap->blue[i].co.local.blue =
                ((((i * 65535) / limb) >> shift) * 65535) / lim;
        }
    }
    else if (pVisual->class == StaticColor) {
        unsigned limr, limg, limb;

        limr = pVisual->redMask >> pVisual->offsetRed;
        limg = pVisual->greenMask >> pVisual->offsetGreen;
        limb = pVisual->blueMask >> pVisual->offsetBlue;
        for (i = 0; i <= maxent; i++) {
            /* rescale to [0..65535] then rgb bits */
            pmap->red[i].co.local.red =
                ((((((i & pVisual->redMask) >> pVisual->offsetRed)
                    * 65535) / limr) >> shift) * 65535) / lim;
            pmap->red[i].co.local.green =
                ((((((i & pVisual->greenMask) >> pVisual->offsetGreen)
                    * 65535) / limg) >> shift) * 65535) / lim;
            pmap->red[i].co.local.blue =
                ((((((i & pVisual->blueMask) >> pVisual->offsetBlue)
                    * 65535) / limb) >> shift) * 65535) / lim;
        }
    }
    else if (pVisual->class == StaticGray) {
        for (i = 0; i <= maxent; i++) {
            /* rescale to [0..65535] then rgb bits */
            pmap->red[i].co.local.red = ((((i * 65535) / maxent) >> shift)
                                         * 65535) / lim;
            pmap->red[i].co.local.green = pmap->red[i].co.local.red;
            pmap->red[i].co.local.blue = pmap->red[i].co.local.red;
        }
    }
    return TRUE;
}

/* When simulating DirectColor on PseudoColor hardware, multiple
   entries of the colormap must be updated
 */

#define AddElement(mask) { \
    pixel = red | green | blue; \
    for (i = 0; i < nresult; i++) \
  	if (outdefs[i].pixel == pixel) \
    	    break; \
    if (i == nresult) \
    { \
   	nresult++; \
	outdefs[i].pixel = pixel; \
	outdefs[i].flags = 0; \
    } \
    outdefs[i].flags |= (mask); \
    outdefs[i].red = pmap->red[red >> pVisual->offsetRed].co.local.red; \
    outdefs[i].green = pmap->green[green >> pVisual->offsetGreen].co.local.green; \
    outdefs[i].blue = pmap->blue[blue >> pVisual->offsetBlue].co.local.blue; \
}

int
miExpandDirectColors(ColormapPtr pmap, int ndef, xColorItem * indefs,
                     xColorItem * outdefs)
{
    int red, green, blue;
    int maxred, maxgreen, maxblue;
    int stepred, stepgreen, stepblue;
    VisualPtr pVisual;
    int pixel;
    int nresult;
    int i;

    pVisual = pmap->pVisual;

    stepred = 1 << pVisual->offsetRed;
    stepgreen = 1 << pVisual->offsetGreen;
    stepblue = 1 << pVisual->offsetBlue;
    maxred = pVisual->redMask;
    maxgreen = pVisual->greenMask;
    maxblue = pVisual->blueMask;
    nresult = 0;
    for (; ndef--; indefs++) {
        if (indefs->flags & DoRed) {
            red = indefs->pixel & pVisual->redMask;
            for (green = 0; green <= maxgreen; green += stepgreen) {
                for (blue = 0; blue <= maxblue; blue += stepblue) {
                    AddElement(DoRed)
                }
            }
        }
        if (indefs->flags & DoGreen) {
            green = indefs->pixel & pVisual->greenMask;
            for (red = 0; red <= maxred; red += stepred) {
                for (blue = 0; blue <= maxblue; blue += stepblue) {
                    AddElement(DoGreen)
                }
            }
        }
        if (indefs->flags & DoBlue) {
            blue = indefs->pixel & pVisual->blueMask;
            for (red = 0; red <= maxred; red += stepred) {
                for (green = 0; green <= maxgreen; green += stepgreen) {
                    AddElement(DoBlue)
                }
            }
        }
    }
    return nresult;
}

Bool
miCreateDefColormap(ScreenPtr pScreen)
{
    unsigned short zero = 0, ones = 0xFFFF;
    Pixel wp, bp;
    VisualPtr pVisual;
    ColormapPtr cmap;
    int alloctype;

    if (!dixRegisterPrivateKey(&micmapScrPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    for (pVisual = pScreen->visuals;
         pVisual->vid != pScreen->rootVisual; pVisual++);

    if (pScreen->rootDepth == 1 || (pVisual->class & DynamicClass))
        alloctype = AllocNone;
    else
        alloctype = AllocAll;

    if (CreateColormap(pScreen->defColormap, pScreen, pVisual, &cmap,
                       alloctype, 0) != Success)
        return FALSE;

    if (pScreen->rootDepth > 1) {
        wp = pScreen->whitePixel;
        bp = pScreen->blackPixel;
        if ((AllocColor(cmap, &ones, &ones, &ones, &wp, 0) !=
             Success) ||
            (AllocColor(cmap, &zero, &zero, &zero, &bp, 0) != Success))
            return FALSE;
        pScreen->whitePixel = wp;
        pScreen->blackPixel = bp;
    }

    (*pScreen->InstallColormap) (cmap);
    return TRUE;
}

/*
 * Default true color bitmasks, should be overridden by
 * driver
 */

#define _RZ(d) ((d + 2) / 3)
#define _RS(d) 0
#define _RM(d) ((1U << _RZ(d)) - 1)
#define _GZ(d) ((d - _RZ(d) + 1) / 2)
#define _GS(d) _RZ(d)
#define _GM(d) (((1U << _GZ(d)) - 1) << _GS(d))
#define _BZ(d) (d - _RZ(d) - _GZ(d))
#define _BS(d) (_RZ(d) + _GZ(d))
#define _BM(d) (((1U << _BZ(d)) - 1) << _BS(d))
#define _CE(d) (1U << _RZ(d))

typedef struct _miVisuals {
    struct _miVisuals *next;
    int depth;
    int bitsPerRGB;
    int visuals;
    int count;
    int preferredCVC;
    Pixel redMask, greenMask, blueMask;
} miVisualsRec, *miVisualsPtr;

static int miVisualPriority[] = {
    PseudoColor, GrayScale, StaticColor, TrueColor, DirectColor, StaticGray
};

#define NUM_PRIORITY	6

static miVisualsPtr miVisuals;

void
miClearVisualTypes(void)
{
    miVisualsPtr v;

    while ((v = miVisuals)) {
        miVisuals = v->next;
        free(v);
    }
}

Bool
miSetVisualTypesAndMasks(int depth, int visuals, int bitsPerRGB,
                         int preferredCVC,
                         Pixel redMask, Pixel greenMask, Pixel blueMask)
{
    miVisualsPtr new, *prev, v;
    int count;

    new = malloc(sizeof *new);
    if (!new)
        return FALSE;
    if (!redMask || !greenMask || !blueMask) {
        redMask = _RM(depth);
        greenMask = _GM(depth);
        blueMask = _BM(depth);
    }
    new->next = 0;
    new->depth = depth;
    new->visuals = visuals;
    new->bitsPerRGB = bitsPerRGB;
    new->preferredCVC = preferredCVC;
    new->redMask = redMask;
    new->greenMask = greenMask;
    new->blueMask = blueMask;
    count = (visuals >> 1) & 033333333333;
    count = visuals - count - ((count >> 1) & 033333333333);
    count = (((count + (count >> 3)) & 030707070707) % 077);    /* HAKMEM 169 */
    new->count = count;
    for (prev = &miVisuals; (v = *prev); prev = &v->next);
    *prev = new;
    return TRUE;
}

Bool
miSetVisualTypes(int depth, int visuals, int bitsPerRGB, int preferredCVC)
{
    return miSetVisualTypesAndMasks(depth, visuals, bitsPerRGB,
                                    preferredCVC, 0, 0, 0);
}

int
miGetDefaultVisualMask(int depth)
{
    if (depth > MAX_PSEUDO_DEPTH)
        return LARGE_VISUALS;
    else if (depth >= MIN_TRUE_DEPTH)
        return ALL_VISUALS;
    else if (depth == 1)
        return StaticGrayMask;
    else
        return SMALL_VISUALS;
}

static Bool
miVisualTypesSet(int depth)
{
    miVisualsPtr visuals;

    for (visuals = miVisuals; visuals; visuals = visuals->next)
        if (visuals->depth == depth)
            return TRUE;
    return FALSE;
}

Bool
miSetPixmapDepths(void)
{
    int d, f;

    /* Add any unlisted depths from the pixmap formats */
    for (f = 0; f < screenInfo.numPixmapFormats; f++) {
        d = screenInfo.formats[f].depth;
        if (!miVisualTypesSet(d)) {
            if (!miSetVisualTypes(d, 0, 0, -1))
                return FALSE;
        }
    }
    return TRUE;
}

/*
 * Distance to least significant one bit
 */
static int
maskShift(Pixel p)
{
    int s;

    if (!p)
        return 0;
    s = 0;
    while (!(p & 1)) {
        s++;
        p >>= 1;
    }
    return s;
}

/*
 * Given a list of formats for a screen, create a list
 * of visuals and depths for the screen which correspond to
 * the set which can be used with this version of cfb.
 */

Bool
miInitVisuals(VisualPtr * visualp, DepthPtr * depthp, int *nvisualp,
              int *ndepthp, int *rootDepthp, VisualID * defaultVisp,
              unsigned long sizes, int bitsPerRGB, int preferredVis)
{
    int i, j = 0, k;
    VisualPtr visual;
    DepthPtr depth;
    VisualID *vid;
    int d, b;
    int f;
    int ndepth, nvisual;
    int nvtype;
    int vtype;
    miVisualsPtr visuals, nextVisuals;
    int *preferredCVCs, *prefp;
    int first_depth;

    /* none specified, we'll guess from pixmap formats */
    if (!miVisuals) {
        for (f = 0; f < screenInfo.numPixmapFormats; f++) {
            d = screenInfo.formats[f].depth;
            b = screenInfo.formats[f].bitsPerPixel;
            if (sizes & (1 << (b - 1)))
                vtype = miGetDefaultVisualMask(d);
            else
                vtype = 0;
            if (!miSetVisualTypes(d, vtype, bitsPerRGB, -1))
                return FALSE;
        }
    }
    nvisual = 0;
    ndepth = 0;
    for (visuals = miVisuals; visuals; visuals = nextVisuals) {
        nextVisuals = visuals->next;
        ndepth++;
        nvisual += visuals->count;
    }
    depth = xallocarray(ndepth, sizeof(DepthRec));
    visual = xallocarray(nvisual, sizeof(VisualRec));
    preferredCVCs = xallocarray(ndepth, sizeof(int));
    if (!depth || !visual || !preferredCVCs) {
        free(depth);
        free(visual);
        free(preferredCVCs);
        return FALSE;
    }
    *depthp = depth;
    *visualp = visual;
    *ndepthp = ndepth;
    *nvisualp = nvisual;
    prefp = preferredCVCs;
    for (visuals = miVisuals; visuals; visuals = nextVisuals) {
        nextVisuals = visuals->next;
        d = visuals->depth;
        vtype = visuals->visuals;
        nvtype = visuals->count;
        *prefp = visuals->preferredCVC;
        prefp++;
        vid = NULL;
        if (nvtype) {
            vid = xallocarray(nvtype, sizeof(VisualID));
            if (!vid) {
                free(depth);
                free(visual);
                free(preferredCVCs);
                return FALSE;
            }
        }
        depth->depth = d;
        depth->numVids = nvtype;
        depth->vids = vid;
        depth++;
        for (i = 0; i < NUM_PRIORITY; i++) {
            if (!(vtype & (1 << miVisualPriority[i])))
                continue;
            visual->class = miVisualPriority[i];
            visual->bitsPerRGBValue = visuals->bitsPerRGB;
            visual->ColormapEntries = 1 << d;
            visual->nplanes = d;
            visual->vid = *vid = FakeClientID(0);
            switch (visual->class) {
            case PseudoColor:
            case GrayScale:
            case StaticGray:
                visual->redMask = 0;
                visual->greenMask = 0;
                visual->blueMask = 0;
                visual->offsetRed = 0;
                visual->offsetGreen = 0;
                visual->offsetBlue = 0;
                break;
            case DirectColor:
            case TrueColor:
                visual->ColormapEntries = _CE(d);
                /* fall through */
            case StaticColor:
                visual->redMask = visuals->redMask;
                visual->greenMask = visuals->greenMask;
                visual->blueMask = visuals->blueMask;
                visual->offsetRed = maskShift(visuals->redMask);
                visual->offsetGreen = maskShift(visuals->greenMask);
                visual->offsetBlue = maskShift(visuals->blueMask);
            }
            vid++;
            visual++;
        }
        free(visuals);
    }
    miVisuals = NULL;
    visual = *visualp;
    depth = *depthp;

    /*
     * if we did not supplyied by a preferred visual class
     * check if there is a preferred class in one of the depth
     * structures - if there is, we want to start looking for the
     * default visual/depth from that depth.
     */
    first_depth = 0;
    if (preferredVis < 0 && defaultColorVisualClass < 0) {
        for (i = 0; i < ndepth; i++) {
            if (preferredCVCs[i] >= 0) {
                first_depth = i;
                break;
            }
        }
    }

    for (i = first_depth; i < ndepth; i++) {
        int prefColorVisualClass = -1;

        if (defaultColorVisualClass >= 0)
            prefColorVisualClass = defaultColorVisualClass;
        else if (preferredVis >= 0)
            prefColorVisualClass = preferredVis;
        else if (preferredCVCs[i] >= 0)
            prefColorVisualClass = preferredCVCs[i];

        if (*rootDepthp && *rootDepthp != depth[i].depth)
            continue;

        for (j = 0; j < depth[i].numVids; j++) {
            for (k = 0; k < nvisual; k++)
                if (visual[k].vid == depth[i].vids[j])
                    break;
            if (k == nvisual)
                continue;
            if (prefColorVisualClass < 0 ||
                visual[k].class == prefColorVisualClass)
                break;
        }
        if (j != depth[i].numVids)
            break;
    }
    if (i == ndepth) {
        i = 0;
        j = 0;
    }
    *rootDepthp = depth[i].depth;
    *defaultVisp = depth[i].vids[j];
    free(preferredCVCs);

    return TRUE;
}
