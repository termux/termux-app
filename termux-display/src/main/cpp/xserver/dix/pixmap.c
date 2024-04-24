/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "scrnintstr.h"
#include "mi.h"
#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "resource.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "servermd.h"
#include "X11/extensions/render.h"
#include "picturestr.h"
#include "randrstr.h"
/*
 *  Scratch pixmap management and device independent pixmap allocation
 *  function.
 */

/* callable by ddx */
PixmapPtr
GetScratchPixmapHeader(ScreenPtr pScreen, int width, int height, int depth,
                       int bitsPerPixel, int devKind, void *pPixData)
{
    PixmapPtr pPixmap = pScreen->pScratchPixmap;

    if (pPixmap)
        pScreen->pScratchPixmap = NULL;
    else
        /* width and height of 0 means don't allocate any pixmap data */
        pPixmap = (*pScreen->CreatePixmap) (pScreen, 0, 0, depth, 0);

    if (pPixmap) {
        if ((*pScreen->ModifyPixmapHeader) (pPixmap, width, height, depth,
                                            bitsPerPixel, devKind, pPixData))
            return pPixmap;
        (*pScreen->DestroyPixmap) (pPixmap);
    }
    return NullPixmap;
}

/* callable by ddx */
void
FreeScratchPixmapHeader(PixmapPtr pPixmap)
{
    if (pPixmap) {
        ScreenPtr pScreen = pPixmap->drawable.pScreen;

        pPixmap->devPrivate.ptr = NULL; /* lest ddx chases bad ptr */
        if (pScreen->pScratchPixmap)
            (*pScreen->DestroyPixmap) (pPixmap);
        else
            pScreen->pScratchPixmap = pPixmap;
    }
}

Bool
CreateScratchPixmapsForScreen(ScreenPtr pScreen)
{
    unsigned int pixmap_size;

    pixmap_size = sizeof(PixmapRec) + dixScreenSpecificPrivatesSize(pScreen, PRIVATE_PIXMAP);
    pScreen->totalPixmapSize =
        BitmapBytePad(pixmap_size * 8);

    /* let it be created on first use */
    pScreen->pScratchPixmap = NULL;
    return TRUE;
}

void
FreeScratchPixmapsForScreen(ScreenPtr pScreen)
{
    FreeScratchPixmapHeader(pScreen->pScratchPixmap);
}

/* callable by ddx */
PixmapPtr
AllocatePixmap(ScreenPtr pScreen, int pixDataSize)
{
    PixmapPtr pPixmap;

    assert(pScreen->totalPixmapSize > 0);

    if (pScreen->totalPixmapSize > ((size_t) - 1) - pixDataSize)
        return NullPixmap;

    pPixmap = calloc(1, pScreen->totalPixmapSize + pixDataSize);
    if (!pPixmap)
        return NullPixmap;

    dixInitScreenPrivates(pScreen, pPixmap, pPixmap + 1, PRIVATE_PIXMAP);
    return pPixmap;
}

/* callable by ddx */
void
FreePixmap(PixmapPtr pPixmap)
{
    dixFiniPrivates(pPixmap, PRIVATE_PIXMAP);
    free(pPixmap);
}

void PixmapUnshareSecondaryPixmap(PixmapPtr secondary_pixmap)
{
     int ihandle = -1;
     ScreenPtr pScreen = secondary_pixmap->drawable.pScreen;
     pScreen->SetSharedPixmapBacking(secondary_pixmap, ((void *)(long)ihandle));
}

PixmapPtr PixmapShareToSecondary(PixmapPtr pixmap, ScreenPtr secondary)
{
    PixmapPtr spix;
    int ret;
    void *handle;
    ScreenPtr primary = pixmap->drawable.pScreen;
    int depth = pixmap->drawable.depth;

    ret = primary->SharePixmapBacking(pixmap, secondary, &handle);
    if (ret == FALSE)
        return NULL;

    spix = secondary->CreatePixmap(secondary, 0, 0, depth,
                               CREATE_PIXMAP_USAGE_SHARED);
    secondary->ModifyPixmapHeader(spix, pixmap->drawable.width,
                                  pixmap->drawable.height, depth, 0,
                                  pixmap->devKind, NULL);

    /* have the secondary pixmap take a reference on the primary pixmap
       later we destroy them both at the same time */
    pixmap->refcnt++;

    spix->primary_pixmap = pixmap;

    ret = secondary->SetSharedPixmapBacking(spix, handle);
    if (ret == FALSE) {
        secondary->DestroyPixmap(spix);
        return NULL;
    }

    return spix;
}

static void
PixmapDirtyDamageDestroy(DamagePtr damage, void *closure)
{
    PixmapDirtyUpdatePtr dirty = closure;

    dirty->damage = NULL;
}

Bool
PixmapStartDirtyTracking(DrawablePtr src,
                         PixmapPtr secondary_dst,
                         int x, int y, int dst_x, int dst_y,
                         Rotation rotation)
{
    ScreenPtr screen = src->pScreen;
    PixmapDirtyUpdatePtr dirty_update;
    RegionPtr damageregion;
    RegionRec dstregion;
    BoxRec box;

    dirty_update = calloc(1, sizeof(PixmapDirtyUpdateRec));
    if (!dirty_update)
        return FALSE;

    dirty_update->src = src;
    dirty_update->secondary_dst = secondary_dst;
    dirty_update->x = x;
    dirty_update->y = y;
    dirty_update->dst_x = dst_x;
    dirty_update->dst_y = dst_y;
    dirty_update->rotation = rotation;
    dirty_update->damage = DamageCreate(NULL, PixmapDirtyDamageDestroy,
                                        DamageReportNone, TRUE, screen,
                                        dirty_update);

    if (rotation != RR_Rotate_0) {
        RRTransformCompute(x, y,
                           secondary_dst->drawable.width,
                           secondary_dst->drawable.height,
                           rotation,
                           NULL,
                           &dirty_update->transform,
                           &dirty_update->f_transform,
                           &dirty_update->f_inverse);
    }
    if (!dirty_update->damage) {
        free(dirty_update);
        return FALSE;
    }

    /* Damage destination rectangle so that the destination pixmap contents
     * will get fully initialized
     */
    box.x1 = dirty_update->x;
    box.y1 = dirty_update->y;
    if (dirty_update->rotation == RR_Rotate_90 ||
        dirty_update->rotation == RR_Rotate_270) {
        box.x2 = dirty_update->x + secondary_dst->drawable.height;
        box.y2 = dirty_update->y + secondary_dst->drawable.width;
    } else {
        box.x2 = dirty_update->x + secondary_dst->drawable.width;
        box.y2 = dirty_update->y + secondary_dst->drawable.height;
    }
    RegionInit(&dstregion, &box, 1);
    damageregion = DamageRegion(dirty_update->damage);
    RegionUnion(damageregion, damageregion, &dstregion);
    RegionUninit(&dstregion);

    DamageRegister(src, dirty_update->damage);
    xorg_list_add(&dirty_update->ent, &screen->pixmap_dirty_list);
    return TRUE;
}

Bool
PixmapStopDirtyTracking(DrawablePtr src, PixmapPtr secondary_dst)
{
    ScreenPtr screen = src->pScreen;
    PixmapDirtyUpdatePtr ent, safe;

    xorg_list_for_each_entry_safe(ent, safe, &screen->pixmap_dirty_list, ent) {
        if (ent->src == src && ent->secondary_dst == secondary_dst) {
            if (ent->damage)
                DamageDestroy(ent->damage);
            xorg_list_del(&ent->ent);
            free(ent);
        }
    }
    return TRUE;
}

static void
PixmapDirtyCopyArea(PixmapPtr dst,
                    PixmapDirtyUpdatePtr dirty,
                    RegionPtr dirty_region)
{
    DrawablePtr src = dirty->src;
    ScreenPtr pScreen = src->pScreen;
    int n;
    BoxPtr b;
    GCPtr pGC;

    n = RegionNumRects(dirty_region);
    b = RegionRects(dirty_region);

    pGC = GetScratchGC(src->depth, pScreen);
    if (pScreen->root) {
        ChangeGCVal subWindowMode;

        subWindowMode.val = IncludeInferiors;
        ChangeGC(NullClient, pGC, GCSubwindowMode, &subWindowMode);
    }
    ValidateGC(&dst->drawable, pGC);

    while (n--) {
        BoxRec dst_box;
        int w, h;

        dst_box = *b;
        w = dst_box.x2 - dst_box.x1;
        h = dst_box.y2 - dst_box.y1;

        pGC->ops->CopyArea(src, &dst->drawable, pGC,
                           dirty->x + dst_box.x1, dirty->y + dst_box.y1, w, h,
                           dirty->dst_x + dst_box.x1,
                           dirty->dst_y + dst_box.y1);
        b++;
    }
    FreeScratchGC(pGC);
}

static void
PixmapDirtyCompositeRotate(PixmapPtr dst_pixmap,
                           PixmapDirtyUpdatePtr dirty,
                           RegionPtr dirty_region)
{
    ScreenPtr pScreen = dirty->src->pScreen;
    PictFormatPtr format = PictureWindowFormat(pScreen->root);
    PicturePtr src, dst;
    XID include_inferiors = IncludeInferiors;
    int n = RegionNumRects(dirty_region);
    BoxPtr b = RegionRects(dirty_region);
    int error;

    src = CreatePicture(None,
                        dirty->src,
                        format,
                        CPSubwindowMode,
                        &include_inferiors, serverClient, &error);
    if (!src)
        return;

    dst = CreatePicture(None,
                        &dst_pixmap->drawable,
                        format, 0L, NULL, serverClient, &error);
    if (!dst)
        return;

    error = SetPictureTransform(src, &dirty->transform);
    if (error)
        return;
    while (n--) {
        BoxRec dst_box;

        dst_box = *b;
        dst_box.x1 += dirty->x;
        dst_box.x2 += dirty->x;
        dst_box.y1 += dirty->y;
        dst_box.y2 += dirty->y;
        pixman_f_transform_bounds(&dirty->f_inverse, &dst_box);

        CompositePicture(PictOpSrc,
                         src, NULL, dst,
                         dst_box.x1,
                         dst_box.y1,
                         0, 0,
                         dst_box.x1,
                         dst_box.y1,
                         dst_box.x2 - dst_box.x1,
                         dst_box.y2 - dst_box.y1);
        b++;
    }

    FreePicture(src, None);
    FreePicture(dst, None);
}

/*
 * this function can possibly be improved and optimised, by clipping
 * instead of iterating
 * Drivers are free to implement their own version of this.
 */
Bool PixmapSyncDirtyHelper(PixmapDirtyUpdatePtr dirty)
{
    ScreenPtr pScreen = dirty->src->pScreen;
    RegionPtr region = DamageRegion(dirty->damage);
    PixmapPtr dst;
    SourceValidateProcPtr SourceValidate;
    RegionRec pixregion;
    BoxRec box;

    dst = dirty->secondary_dst->primary_pixmap;
    if (!dst)
        dst = dirty->secondary_dst;

    box.x1 = 0;
    box.y1 = 0;
    if (dirty->rotation == RR_Rotate_90 ||
        dirty->rotation == RR_Rotate_270) {
        box.x2 = dst->drawable.height;
        box.y2 = dst->drawable.width;
    } else {
        box.x2 = dst->drawable.width;
        box.y2 = dst->drawable.height;
    }
    RegionInit(&pixregion, &box, 1);

    /*
     * SourceValidate is used by the software cursor code
     * to pull the cursor off of the screen when reading
     * bits from the frame buffer. Bypassing this function
     * leaves the software cursor in place
     */
    SourceValidate = pScreen->SourceValidate;
    pScreen->SourceValidate = miSourceValidate;

    RegionTranslate(&pixregion, dirty->x, dirty->y);
    RegionIntersect(&pixregion, &pixregion, region);

    if (RegionNil(&pixregion)) {
        RegionUninit(&pixregion);
        return FALSE;
    }

    RegionTranslate(&pixregion, -dirty->x, -dirty->y);

    if (!pScreen->root || dirty->rotation == RR_Rotate_0)
        PixmapDirtyCopyArea(dst, dirty, &pixregion);
    else
        PixmapDirtyCompositeRotate(dst, dirty, &pixregion);
    pScreen->SourceValidate = SourceValidate;
    return TRUE;
}
