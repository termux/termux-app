/*
 *
 * Copyright © 2001 Keith Packard, member of The XFree86 Project, Inc.
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

#ifndef _MIINDEX_H_
#define _MIINDEX_H_

#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "mi.h"
#include "picturestr.h"
#include "mipict.h"
#include "colormapst.h"

#define NUM_CUBE_LEVELS	4
#define NUM_GRAY_LEVELS	13

static Bool
miBuildRenderColormap(ColormapPtr pColormap, Pixel * pixels, int *nump)
{
    int r, g, b;
    unsigned short red, green, blue;
    Pixel pixel;
    Bool used[MI_MAX_INDEXED];
    int needed;
    int policy;
    int cube, gray;
    int i, n;

    if (pColormap->mid != pColormap->pScreen->defColormap) {
        policy = PictureCmapPolicyAll;
    }
    else {
        int avail = pColormap->pVisual->ColormapEntries;

        policy = PictureCmapPolicy;
        if (policy == PictureCmapPolicyDefault) {
            if (avail >= 256 &&
                (pColormap->pVisual->class | DynamicClass) == PseudoColor)
                policy = PictureCmapPolicyColor;
            else if (avail >= 64)
                policy = PictureCmapPolicyGray;
            else
                policy = PictureCmapPolicyMono;
        }
    }
    /*
     * Make sure enough cells are free for the chosen policy
     */
    for (;;) {
        switch (policy) {
        case PictureCmapPolicyAll:
            needed = 0;
            break;
        case PictureCmapPolicyColor:
            needed = 71;
            break;
        case PictureCmapPolicyGray:
            needed = 11;
            break;
        case PictureCmapPolicyMono:
        default:
            needed = 0;
            break;
        }
        if (needed <= pColormap->freeRed)
            break;
        policy--;
    }

    /*
     * Compute size of cube and gray ramps
     */
    cube = gray = 0;
    switch (policy) {
    case PictureCmapPolicyAll:
        /*
         * Allocate as big a cube as possible
         */
        if ((pColormap->pVisual->class | DynamicClass) == PseudoColor) {
            for (cube = 1;
                 cube * cube * cube < pColormap->pVisual->ColormapEntries;
                 cube++);
            cube--;
            if (cube == 1)
                cube = 0;
        }
        else
            cube = 0;
        /*
         * Figure out how many gray levels to use so that they
         * line up neatly with the cube
         */
        if (cube) {
            needed = pColormap->pVisual->ColormapEntries - (cube * cube * cube);
            /* levels to fill in with */
            gray = needed / (cube - 1);
            /* total levels */
            gray = (gray + 1) * (cube - 1) + 1;
        }
        else
            gray = pColormap->pVisual->ColormapEntries;
        break;

    case PictureCmapPolicyColor:
        cube = NUM_CUBE_LEVELS;
        /* fall through ... */
    case PictureCmapPolicyGray:
        gray = NUM_GRAY_LEVELS;
        break;
    case PictureCmapPolicyMono:
    default:
        gray = 2;
        break;
    }

    memset(used, '\0', pColormap->pVisual->ColormapEntries * sizeof(Bool));
    for (r = 0; r < cube; r++)
        for (g = 0; g < cube; g++)
            for (b = 0; b < cube; b++) {
                pixel = 0;
                red = (r * 65535 + (cube - 1) / 2) / (cube - 1);
                green = (g * 65535 + (cube - 1) / 2) / (cube - 1);
                blue = (b * 65535 + (cube - 1) / 2) / (cube - 1);
                if (AllocColor(pColormap, &red, &green,
                               &blue, &pixel, 0) != Success)
                    return FALSE;
                used[pixel] = TRUE;
            }
    for (g = 0; g < gray; g++) {
        pixel = 0;
        red = green = blue = (g * 65535 + (gray - 1) / 2) / (gray - 1);
        if (AllocColor(pColormap, &red, &green, &blue, &pixel, 0) != Success)
            return FALSE;
        used[pixel] = TRUE;
    }
    n = 0;
    for (i = 0; i < pColormap->pVisual->ColormapEntries; i++)
        if (used[i])
            pixels[n++] = i;

    *nump = n;

    return TRUE;
}

/* 0 <= red, green, blue < 32 */
static Pixel
FindBestColor(miIndexedPtr pIndexed, Pixel * pixels, int num,
              int red, int green, int blue)
{
    Pixel best = pixels[0];
    int bestDist = 1 << 30;
    int dist;
    int dr, dg, db;

    while (num--) {
        Pixel pixel = *pixels++;
        CARD32 v = pIndexed->rgba[pixel];

        dr = ((v >> 19) & 0x1f);
        dg = ((v >> 11) & 0x1f);
        db = ((v >> 3) & 0x1f);
        dr = dr - red;
        dg = dg - green;
        db = db - blue;
        dist = dr * dr + dg * dg + db * db;
        if (dist < bestDist) {
            bestDist = dist;
            best = pixel;
        }
    }
    return best;
}

/* 0 <= gray < 32768 */
static Pixel
FindBestGray(miIndexedPtr pIndexed, Pixel * pixels, int num, int gray)
{
    Pixel best = pixels[0];
    int bestDist = 1 << 30;
    int dist;
    int dr;
    int r;

    while (num--) {
        Pixel pixel = *pixels++;
        CARD32 v = pIndexed->rgba[pixel];

        r = v & 0xff;
        r = r | (r << 8);
        dr = gray - (r >> 1);
        dist = dr * dr;
        if (dist < bestDist) {
            bestDist = dist;
            best = pixel;
        }
    }
    return best;
}

Bool
miInitIndexed(ScreenPtr pScreen, PictFormatPtr pFormat)
{
    ColormapPtr pColormap = pFormat->index.pColormap;
    VisualPtr pVisual = pColormap->pVisual;
    miIndexedPtr pIndexed;
    Pixel pixels[MI_MAX_INDEXED];
    xrgb rgb[MI_MAX_INDEXED];
    int num;
    int i;
    Pixel p, r, g, b;

    if (pVisual->ColormapEntries > MI_MAX_INDEXED)
        return FALSE;

    if (pVisual->class & DynamicClass) {
        if (!miBuildRenderColormap(pColormap, pixels, &num))
            return FALSE;
    }
    else {
        num = pVisual->ColormapEntries;
        for (p = 0; p < num; p++)
            pixels[p] = p;
    }

    pIndexed = malloc(sizeof(miIndexedRec));
    if (!pIndexed)
        return FALSE;

    pFormat->index.nvalues = num;
    pFormat->index.pValues = xallocarray(num, sizeof(xIndexValue));
    if (!pFormat->index.pValues) {
        free(pIndexed);
        return FALSE;
    }

    /*
     * Build mapping from pixel value to ARGB
     */
    QueryColors(pColormap, num, pixels, rgb, serverClient);
    for (i = 0; i < num; i++) {
        p = pixels[i];
        pFormat->index.pValues[i].pixel = p;
        pFormat->index.pValues[i].red = rgb[i].red;
        pFormat->index.pValues[i].green = rgb[i].green;
        pFormat->index.pValues[i].blue = rgb[i].blue;
        pFormat->index.pValues[i].alpha = 0xffff;
        pIndexed->rgba[p] = (0xff000000 |
                             ((rgb[i].red & 0xff00) << 8) |
                             ((rgb[i].green & 0xff00)) |
                             ((rgb[i].blue & 0xff00) >> 8));
    }

    /*
     * Build mapping from RGB to pixel value.  This could probably be
     * done a bit quicker...
     */
    switch (pVisual->class | DynamicClass) {
    case GrayScale:
        pIndexed->color = FALSE;
        for (r = 0; r < 32768; r++)
            pIndexed->ent[r] = FindBestGray(pIndexed, pixels, num, r);
        break;
    case PseudoColor:
        pIndexed->color = TRUE;
        p = 0;
        for (r = 0; r < 32; r++)
            for (g = 0; g < 32; g++)
                for (b = 0; b < 32; b++) {
                    pIndexed->ent[p] = FindBestColor(pIndexed, pixels, num,
                                                     r, g, b);
                    p++;
                }
        break;
    }
    pFormat->index.devPrivate = pIndexed;
    return TRUE;
}

void
miCloseIndexed(ScreenPtr pScreen, PictFormatPtr pFormat)
{
    free(pFormat->index.devPrivate);
    pFormat->index.devPrivate = NULL;
    free(pFormat->index.pValues);
    pFormat->index.pValues = NULL;
}

void
miUpdateIndexed(ScreenPtr pScreen,
                PictFormatPtr pFormat, int ndef, xColorItem * pdef)
{
    miIndexedPtr pIndexed = pFormat->index.devPrivate;

    if (pIndexed) {
        while (ndef--) {
            pIndexed->rgba[pdef->pixel] = (0xff000000 |
                                           ((pdef->red & 0xff00) << 8) |
                                           ((pdef->green & 0xff00)) |
                                           ((pdef->blue & 0xff00) >> 8));
            pdef++;
        }
    }
}

#endif                          /* _MIINDEX_H_ */
