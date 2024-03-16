/*
 * Copyright © 2014 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "glamor_priv.h"
#include "glamor_prepare.h"
#include "glamor_transfer.h"

/*
 * Make a pixmap ready to draw with fb by
 * creating a PBO large enough for the whole object
 * and downloading all of the FBOs into it.
 */

static Bool
glamor_prep_pixmap_box(PixmapPtr pixmap, glamor_access_t access, BoxPtr box)
{
    ScreenPtr                   screen = pixmap->drawable.pScreen;
    glamor_screen_private       *glamor_priv = glamor_get_screen_private(screen);
    glamor_pixmap_private       *priv = glamor_get_pixmap_private(pixmap);
    int                         gl_access, gl_usage;
    RegionRec                   region;

    if (priv->type == GLAMOR_DRM_ONLY)
        return FALSE;

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(priv))
        return TRUE;

    glamor_make_current(glamor_priv);

    RegionInit(&region, box, 1);

    /* See if it's already mapped */
    if (pixmap->devPrivate.ptr) {
        /*
         * Someone else has mapped this pixmap;
         * we'll assume that it's directly mapped
         * by a lower level driver
         */
        if (!priv->prepared)
            return TRUE;

        /* In X, multiple Drawables can be stored in the same Pixmap (such as
         * each individual window in a non-composited screen pixmap, or the
         * reparented window contents inside the window-manager-decorated window
         * pixmap on a composited screen).
         *
         * As a result, when doing a series of mappings for a fallback, we may
         * need to add more boxes to the set of data we've downloaded, as we go.
         */
        RegionSubtract(&region, &region, &priv->prepare_region);
        if (!RegionNotEmpty(&region))
            return TRUE;

        if (access == GLAMOR_ACCESS_RW)
            FatalError("attempt to remap buffer as writable");

        if (priv->pbo) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, priv->pbo);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            pixmap->devPrivate.ptr = NULL;
        }
    } else {
        RegionInit(&priv->prepare_region, box, 1);

        if (glamor_priv->has_rw_pbo) {
            if (priv->pbo == 0)
                glGenBuffers(1, &priv->pbo);

            gl_usage = GL_STREAM_READ;

            glamor_priv->suppress_gl_out_of_memory_logging = true;

            glBindBuffer(GL_PIXEL_PACK_BUFFER, priv->pbo);
            glBufferData(GL_PIXEL_PACK_BUFFER,
                         pixmap->devKind * pixmap->drawable.height, NULL,
                         gl_usage);

            glamor_priv->suppress_gl_out_of_memory_logging = false;

            if (glGetError() == GL_OUT_OF_MEMORY) {
                if (!glamor_priv->logged_any_pbo_allocation_failure) {
                    LogMessageVerb(X_WARNING, 0, "glamor: Failed to allocate %d "
                                   "bytes PBO due to GL_OUT_OF_MEMORY.\n",
                                   pixmap->devKind * pixmap->drawable.height);
                    glamor_priv->logged_any_pbo_allocation_failure = true;
                }
                glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
                glDeleteBuffers(1, &priv->pbo);
                priv->pbo = 0;
            }
        }

        if (!priv->pbo) {
            pixmap->devPrivate.ptr = xallocarray(pixmap->devKind,
                                                 pixmap->drawable.height);
            if (!pixmap->devPrivate.ptr)
                return FALSE;
        }
        priv->map_access = access;
    }

    glamor_download_boxes(pixmap, RegionRects(&region), RegionNumRects(&region),
                          0, 0, 0, 0, pixmap->devPrivate.ptr, pixmap->devKind);

    RegionUninit(&region);

    if (priv->pbo) {
        if (priv->map_access == GLAMOR_ACCESS_RW)
            gl_access = GL_READ_WRITE;
        else
            gl_access = GL_READ_ONLY;

        pixmap->devPrivate.ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, gl_access);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    priv->prepared = TRUE;
    return TRUE;
}

/*
 * When we're done with the drawable, unmap the PBO, reupload
 * if we were writing to it and then unbind it to release the memory
 */

static void
glamor_fini_pixmap(PixmapPtr pixmap)
{
    glamor_pixmap_private       *priv = glamor_get_pixmap_private(pixmap);

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(priv))
        return;

    if (!priv->prepared)
        return;

    if (priv->pbo) {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, priv->pbo);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        pixmap->devPrivate.ptr = NULL;
    }

    if (priv->map_access == GLAMOR_ACCESS_RW) {
        glamor_upload_boxes(pixmap,
                            RegionRects(&priv->prepare_region),
                            RegionNumRects(&priv->prepare_region),
                            0, 0, 0, 0, pixmap->devPrivate.ptr, pixmap->devKind);
    }

    RegionUninit(&priv->prepare_region);

    if (priv->pbo) {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glDeleteBuffers(1, &priv->pbo);
        priv->pbo = 0;
    } else {
        free(pixmap->devPrivate.ptr);
        pixmap->devPrivate.ptr = NULL;
    }

    priv->prepared = FALSE;
}

Bool
glamor_prepare_access(DrawablePtr drawable, glamor_access_t access)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    BoxRec box;
    int off_x, off_y;

    glamor_get_drawable_deltas(drawable, pixmap, &off_x, &off_y);

    box.x1 = drawable->x + off_x;
    box.x2 = box.x1 + drawable->width;
    box.y1 = drawable->y + off_y;
    box.y2 = box.y1 + drawable->height;
    return glamor_prep_pixmap_box(pixmap, access, &box);
}

Bool
glamor_prepare_access_box(DrawablePtr drawable, glamor_access_t access,
                         int x, int y, int w, int h)
{
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    BoxRec box;
    int off_x, off_y;

    glamor_get_drawable_deltas(drawable, pixmap, &off_x, &off_y);
    box.x1 = drawable->x + x + off_x;
    box.x2 = box.x1 + w;
    box.y1 = drawable->y + y + off_y;
    box.y2 = box.y1 + h;
    return glamor_prep_pixmap_box(pixmap, access, &box);
}

void
glamor_finish_access(DrawablePtr drawable)
{
    glamor_fini_pixmap(glamor_get_drawable_pixmap(drawable));
}

/*
 * Make a picture ready to use with fb.
 */

Bool
glamor_prepare_access_picture(PicturePtr picture, glamor_access_t access)
{
    if (!picture || !picture->pDrawable)
        return TRUE;

    return glamor_prepare_access(picture->pDrawable, access);
}

Bool
glamor_prepare_access_picture_box(PicturePtr picture, glamor_access_t access,
                        int x, int y, int w, int h)
{
    if (!picture || !picture->pDrawable)
        return TRUE;

    /* If a transform is set, we don't know what the bounds is on the
     * source, so just prepare the whole pixmap.  XXX: We could
     * potentially work out where in the source would be sampled based
     * on the transform, and we don't need do do this for destination
     * pixmaps at all.
     */
    if (picture->transform) {
        return glamor_prepare_access_box(picture->pDrawable, access,
                                         0, 0,
                                         picture->pDrawable->width,
                                         picture->pDrawable->height);
    } else {
        return glamor_prepare_access_box(picture->pDrawable, access,
                                         x, y, w, h);
    }
}

void
glamor_finish_access_picture(PicturePtr picture)
{
    if (!picture || !picture->pDrawable)
        return;

    glamor_finish_access(picture->pDrawable);
}

/*
 * Make a GC ready to use with fb. This just
 * means making sure the appropriate fill pixmap is
 * in CPU memory again
 */

Bool
glamor_prepare_access_gc(GCPtr gc)
{
    switch (gc->fillStyle) {
    case FillTiled:
        return glamor_prepare_access(&gc->tile.pixmap->drawable,
                                     GLAMOR_ACCESS_RO);
    case FillStippled:
    case FillOpaqueStippled:
        return glamor_prepare_access(&gc->stipple->drawable, GLAMOR_ACCESS_RO);
    }
    return TRUE;
}

/*
 * Free any temporary CPU pixmaps for the GC
 */
void
glamor_finish_access_gc(GCPtr gc)
{
    switch (gc->fillStyle) {
    case FillTiled:
        glamor_finish_access(&gc->tile.pixmap->drawable);
        break;
    case FillStippled:
    case FillOpaqueStippled:
        glamor_finish_access(&gc->stipple->drawable);
        break;
    }
}
