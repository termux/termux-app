/*
 * Copyright © 2013 Intel Corporation
 * Copyright © 2014 Broadcom
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * @file dri2.c
 *
 * Implements generic support for DRI2 on KMS, using glamor pixmaps
 * for color buffer management (no support for other aux buffers), and
 * the DRM vblank ioctls.
 *
 * This doesn't implement pageflipping yet.
 */

#ifdef HAVE_DIX_CONFIG_H
#include "dix-config.h"
#endif

#include <time.h>
#include "list.h"
#include "xf86.h"
#include "driver.h"
#include "dri2.h"

#ifdef GLAMOR_HAS_GBM

enum ms_dri2_frame_event_type {
    MS_DRI2_QUEUE_SWAP,
    MS_DRI2_QUEUE_FLIP,
    MS_DRI2_WAIT_MSC,
};

typedef struct ms_dri2_frame_event {
    ScreenPtr screen;

    DrawablePtr drawable;
    ClientPtr client;
    enum ms_dri2_frame_event_type type;
    int frame;
    xf86CrtcPtr crtc;

    struct xorg_list drawable_resource, client_resource;

    /* for swaps & flips only */
    DRI2SwapEventPtr event_complete;
    void *event_data;
    DRI2BufferPtr front;
    DRI2BufferPtr back;
} ms_dri2_frame_event_rec, *ms_dri2_frame_event_ptr;

typedef struct {
    int refcnt;
    PixmapPtr pixmap;
} ms_dri2_buffer_private_rec, *ms_dri2_buffer_private_ptr;

static DevPrivateKeyRec ms_dri2_client_key;
static RESTYPE frame_event_client_type, frame_event_drawable_type;
static int ms_dri2_server_generation;

struct ms_dri2_resource {
    XID id;
    RESTYPE type;
    struct xorg_list list;
};

static struct ms_dri2_resource *
ms_get_resource(XID id, RESTYPE type)
{
    struct ms_dri2_resource *resource;
    void *ptr;

    ptr = NULL;
    dixLookupResourceByType(&ptr, id, type, NULL, DixWriteAccess);
    if (ptr)
        return ptr;

    resource = malloc(sizeof(*resource));
    if (resource == NULL)
        return NULL;

    if (!AddResource(id, type, resource))
        return NULL;

    resource->id = id;
    resource->type = type;
    xorg_list_init(&resource->list);
    return resource;
}

static inline PixmapPtr
get_drawable_pixmap(DrawablePtr drawable)
{
    ScreenPtr screen = drawable->pScreen;

    if (drawable->type == DRAWABLE_PIXMAP)
        return (PixmapPtr) drawable;
    else
        return screen->GetWindowPixmap((WindowPtr) drawable);
}

static DRI2Buffer2Ptr
ms_dri2_create_buffer2(ScreenPtr screen, DrawablePtr drawable,
                       unsigned int attachment, unsigned int format)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    DRI2Buffer2Ptr buffer;
    PixmapPtr pixmap;
    CARD32 size;
    CARD16 pitch;
    ms_dri2_buffer_private_ptr private;

    buffer = calloc(1, sizeof *buffer);
    if (buffer == NULL)
        return NULL;

    private = calloc(1, sizeof(*private));
    if (private == NULL) {
        free(buffer);
        return NULL;
    }

    pixmap = NULL;
    if (attachment == DRI2BufferFrontLeft) {
        pixmap = get_drawable_pixmap(drawable);
        if (pixmap && pixmap->drawable.pScreen != screen)
            pixmap = NULL;
        if (pixmap)
            pixmap->refcnt++;
    }

    if (pixmap == NULL) {
        int pixmap_width = drawable->width;
        int pixmap_height = drawable->height;
        int pixmap_cpp = (format != 0) ? format : drawable->depth;

        /* Assume that non-color-buffers require special
         * device-specific handling.  Mesa currently makes no requests
         * for non-color aux buffers.
         */
        switch (attachment) {
        case DRI2BufferAccum:
        case DRI2BufferBackLeft:
        case DRI2BufferBackRight:
        case DRI2BufferFakeFrontLeft:
        case DRI2BufferFakeFrontRight:
        case DRI2BufferFrontLeft:
        case DRI2BufferFrontRight:
            break;

        case DRI2BufferStencil:
        case DRI2BufferDepth:
        case DRI2BufferDepthStencil:
        case DRI2BufferHiz:
        default:
            xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                       "Request for DRI2 buffer attachment %d unsupported\n",
                       attachment);
            free(private);
            free(buffer);
            return NULL;
        }

        pixmap = screen->CreatePixmap(screen,
                                      pixmap_width,
                                      pixmap_height,
                                      pixmap_cpp,
                                      0);
        if (pixmap == NULL) {
            free(private);
            free(buffer);
            return NULL;
        }
    }

    buffer->attachment = attachment;
    buffer->cpp = pixmap->drawable.bitsPerPixel / 8;
    buffer->format = format;
    /* The buffer's flags field is unused by the client drivers in
     * Mesa currently.
     */
    buffer->flags = 0;

    buffer->name = ms->glamor.name_from_pixmap(pixmap, &pitch, &size);
    buffer->pitch = pitch;
    if (buffer->name == -1) {
        xf86DrvMsg(scrn->scrnIndex, X_ERROR,
                   "Failed to get DRI2 name for pixmap\n");
        screen->DestroyPixmap(pixmap);
        free(private);
        free(buffer);
        return NULL;
    }

    buffer->driverPrivate = private;
    private->refcnt = 1;
    private->pixmap = pixmap;

    return buffer;
}

static DRI2Buffer2Ptr
ms_dri2_create_buffer(DrawablePtr drawable, unsigned int attachment,
                      unsigned int format)
{
    return ms_dri2_create_buffer2(drawable->pScreen, drawable, attachment,
                                  format);
}

static void
ms_dri2_reference_buffer(DRI2Buffer2Ptr buffer)
{
    if (buffer) {
        ms_dri2_buffer_private_ptr private = buffer->driverPrivate;
        private->refcnt++;
    }
}

static void ms_dri2_destroy_buffer2(ScreenPtr unused, DrawablePtr unused2,
                                    DRI2Buffer2Ptr buffer)
{
    if (!buffer)
        return;

    if (buffer->driverPrivate) {
        ms_dri2_buffer_private_ptr private = buffer->driverPrivate;
        if (--private->refcnt == 0) {
            ScreenPtr screen = private->pixmap->drawable.pScreen;
            screen->DestroyPixmap(private->pixmap);
            free(private);
            free(buffer);
        }
    } else {
        free(buffer);
    }
}

static void ms_dri2_destroy_buffer(DrawablePtr drawable, DRI2Buffer2Ptr buffer)
{
    ms_dri2_destroy_buffer2(NULL, drawable, buffer);
}

static void
ms_dri2_copy_region2(ScreenPtr screen, DrawablePtr drawable, RegionPtr pRegion,
                     DRI2BufferPtr destBuffer, DRI2BufferPtr sourceBuffer)
{
    ms_dri2_buffer_private_ptr src_priv = sourceBuffer->driverPrivate;
    ms_dri2_buffer_private_ptr dst_priv = destBuffer->driverPrivate;
    PixmapPtr src_pixmap = src_priv->pixmap;
    PixmapPtr dst_pixmap = dst_priv->pixmap;
    DrawablePtr src = (sourceBuffer->attachment == DRI2BufferFrontLeft)
        ? drawable : &src_pixmap->drawable;
    DrawablePtr dst = (destBuffer->attachment == DRI2BufferFrontLeft)
        ? drawable : &dst_pixmap->drawable;
    int off_x = 0, off_y = 0;
    Bool translate = FALSE;
    RegionPtr pCopyClip;
    GCPtr gc;

    if (destBuffer->attachment == DRI2BufferFrontLeft &&
             drawable->pScreen != screen) {
        dst = DRI2UpdatePrime(drawable, destBuffer);
        if (!dst)
            return;
        if (dst != drawable)
            translate = TRUE;
    }

    if (translate && drawable->type == DRAWABLE_WINDOW) {
#ifdef COMPOSITE
        PixmapPtr pixmap = get_drawable_pixmap(drawable);
        off_x = -pixmap->screen_x;
        off_y = -pixmap->screen_y;
#endif
        off_x += drawable->x;
        off_y += drawable->y;
    }

    gc = GetScratchGC(dst->depth, screen);
    if (!gc)
        return;

    pCopyClip = REGION_CREATE(screen, NULL, 0);
    REGION_COPY(screen, pCopyClip, pRegion);
    if (translate)
        REGION_TRANSLATE(screen, pCopyClip, off_x, off_y);
    (*gc->funcs->ChangeClip) (gc, CT_REGION, pCopyClip, 0);
    ValidateGC(dst, gc);

    /* It's important that this copy gets submitted before the direct
     * rendering client submits rendering for the next frame, but we
     * don't actually need to submit right now.  The client will wait
     * for the DRI2CopyRegion reply or the swap buffer event before
     * rendering, and we'll hit the flush callback chain before those
     * messages are sent.  We submit our batch buffers from the flush
     * callback chain so we know that will happen before the client
     * tries to render again.
     */
    gc->ops->CopyArea(src, dst, gc,
                      0, 0,
                      drawable->width, drawable->height,
                      off_x, off_y);

    FreeScratchGC(gc);
}

static void
ms_dri2_copy_region(DrawablePtr drawable, RegionPtr pRegion,
                    DRI2BufferPtr destBuffer, DRI2BufferPtr sourceBuffer)
{
    ms_dri2_copy_region2(drawable->pScreen, drawable, pRegion, destBuffer,
                         sourceBuffer);
}

static uint64_t
gettime_us(void)
{
    struct timespec tv;

    if (clock_gettime(CLOCK_MONOTONIC, &tv))
        return 0;

    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_nsec / 1000;
}

/**
 * Get current frame count and frame count timestamp, based on drawable's
 * crtc.
 */
static int
ms_dri2_get_msc(DrawablePtr draw, CARD64 *ust, CARD64 *msc)
{
    int ret;
    xf86CrtcPtr crtc = ms_dri2_crtc_covering_drawable(draw);

    /* Drawable not displayed, make up a *monotonic* value */
    if (crtc == NULL) {
        *ust = gettime_us();
        *msc = 0;
        return TRUE;
    }

    ret = ms_get_crtc_ust_msc(crtc, ust, msc);

    if (ret)
        return FALSE;

    return TRUE;
}

static XID
get_client_id(ClientPtr client)
{
    XID *ptr = dixGetPrivateAddr(&client->devPrivates, &ms_dri2_client_key);
    if (*ptr == 0)
        *ptr = FakeClientID(client->index);
    return *ptr;
}

/*
 * Hook this frame event into the server resource
 * database so we can clean it up if the drawable or
 * client exits while the swap is pending
 */
static Bool
ms_dri2_add_frame_event(ms_dri2_frame_event_ptr info)
{
    struct ms_dri2_resource *resource;

    resource = ms_get_resource(get_client_id(info->client),
                               frame_event_client_type);
    if (resource == NULL)
        return FALSE;

    xorg_list_add(&info->client_resource, &resource->list);

    resource = ms_get_resource(info->drawable->id, frame_event_drawable_type);
    if (resource == NULL) {
        xorg_list_del(&info->client_resource);
        return FALSE;
    }

    xorg_list_add(&info->drawable_resource, &resource->list);

    return TRUE;
}

static void
ms_dri2_del_frame_event(ms_dri2_frame_event_rec *info)
{
    xorg_list_del(&info->client_resource);
    xorg_list_del(&info->drawable_resource);

    if (info->front)
        ms_dri2_destroy_buffer(NULL, info->front);
    if (info->back)
        ms_dri2_destroy_buffer(NULL, info->back);

    free(info);
}

static void
ms_dri2_blit_swap(DrawablePtr drawable,
                  DRI2BufferPtr dst,
                  DRI2BufferPtr src)
{
    BoxRec box;
    RegionRec region;

    box.x1 = 0;
    box.y1 = 0;
    box.x2 = drawable->width;
    box.y2 = drawable->height;
    REGION_INIT(pScreen, &region, &box, 0);

    ms_dri2_copy_region(drawable, &region, dst, src);
}

struct ms_dri2_vblank_event {
    XID drawable_id;
    ClientPtr client;
    DRI2SwapEventPtr event_complete;
    void *event_data;
};

static void
ms_dri2_flip_abort(modesettingPtr ms, void *data)
{
    struct ms_present_vblank_event *event = data;

    ms->drmmode.dri2_flipping = FALSE;
    free(event);
}

static void
ms_dri2_flip_handler(modesettingPtr ms, uint64_t msc,
                     uint64_t ust, void *data)
{
    struct ms_dri2_vblank_event *event = data;
    uint32_t frame = msc;
    uint32_t tv_sec = ust / 1000000;
    uint32_t tv_usec = ust % 1000000;
    DrawablePtr drawable;
    int status;

    status = dixLookupDrawable(&drawable, event->drawable_id, serverClient,
                               M_ANY, DixWriteAccess);
    if (status == Success)
        DRI2SwapComplete(event->client, drawable, frame, tv_sec, tv_usec,
                         DRI2_FLIP_COMPLETE, event->event_complete,
                         event->event_data);

    ms->drmmode.dri2_flipping = FALSE;
    free(event);
}

static Bool
ms_dri2_schedule_flip(ms_dri2_frame_event_ptr info)
{
    DrawablePtr draw = info->drawable;
    ScreenPtr screen = draw->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    ms_dri2_buffer_private_ptr back_priv = info->back->driverPrivate;
    struct ms_dri2_vblank_event *event;
    drmmode_crtc_private_ptr drmmode_crtc = info->crtc->driver_private;

    event = calloc(1, sizeof(struct ms_dri2_vblank_event));
    if (!event)
        return FALSE;

    event->drawable_id = draw->id;
    event->client = info->client;
    event->event_complete = info->event_complete;
    event->event_data = info->event_data;

    if (ms_do_pageflip(screen, back_priv->pixmap, event,
                       drmmode_crtc->vblank_pipe, FALSE,
                       ms_dri2_flip_handler,
                       ms_dri2_flip_abort,
                       "DRI2-flip")) {
        ms->drmmode.dri2_flipping = TRUE;
        return TRUE;
    }
    return FALSE;
}

static Bool
update_front(DrawablePtr draw, DRI2BufferPtr front)
{
    ScreenPtr screen = draw->pScreen;
    PixmapPtr pixmap = get_drawable_pixmap(draw);
    ms_dri2_buffer_private_ptr priv = front->driverPrivate;
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(screen));
    CARD32 size;
    CARD16 pitch;
    int name;

    name = ms->glamor.name_from_pixmap(pixmap, &pitch, &size);
    if (name < 0)
        return FALSE;

    front->name = name;

    (*screen->DestroyPixmap) (priv->pixmap);
    front->pitch = pixmap->devKind;
    front->cpp = pixmap->drawable.bitsPerPixel / 8;
    priv->pixmap = pixmap;
    pixmap->refcnt++;

    return TRUE;
}

static Bool
can_exchange(ScrnInfoPtr scrn, DrawablePtr draw,
	     DRI2BufferPtr front, DRI2BufferPtr back)
{
    ms_dri2_buffer_private_ptr front_priv = front->driverPrivate;
    ms_dri2_buffer_private_ptr back_priv = back->driverPrivate;
    PixmapPtr front_pixmap;
    PixmapPtr back_pixmap = back_priv->pixmap;
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(scrn);
    int num_crtcs_on = 0;
    int i;

    for (i = 0; i < config->num_crtc; i++) {
        drmmode_crtc_private_ptr drmmode_crtc = config->crtc[i]->driver_private;

        /* Don't do pageflipping if CRTCs are rotated. */
#ifdef GLAMOR_HAS_GBM
        if (drmmode_crtc->rotate_bo.gbm)
            return FALSE;
#endif

        if (xf86_crtc_on(config->crtc[i]))
            num_crtcs_on++;
    }

    /* We can't do pageflipping if all the CRTCs are off. */
    if (num_crtcs_on == 0)
        return FALSE;

    if (!update_front(draw, front))
        return FALSE;

    front_pixmap = front_priv->pixmap;

    if (front_pixmap->drawable.width != back_pixmap->drawable.width)
        return FALSE;

    if (front_pixmap->drawable.height != back_pixmap->drawable.height)
        return FALSE;

    if (front_pixmap->drawable.bitsPerPixel !=
        back_pixmap->drawable.bitsPerPixel)
        return FALSE;

    if (front_pixmap->devKind != back_pixmap->devKind)
        return FALSE;

    return TRUE;
}

static Bool
can_flip(ScrnInfoPtr scrn, DrawablePtr draw,
	 DRI2BufferPtr front, DRI2BufferPtr back)
{
    modesettingPtr ms = modesettingPTR(scrn);

    return draw->type == DRAWABLE_WINDOW &&
        ms->drmmode.pageflip &&
        !ms->drmmode.sprites_visible &&
        !ms->drmmode.present_flipping &&
        scrn->vtSema &&
        DRI2CanFlip(draw) && can_exchange(scrn, draw, front, back);
}

static void
ms_dri2_exchange_buffers(DrawablePtr draw, DRI2BufferPtr front,
                         DRI2BufferPtr back)
{
    ms_dri2_buffer_private_ptr front_priv = front->driverPrivate;
    ms_dri2_buffer_private_ptr back_priv = back->driverPrivate;
    ScreenPtr screen = draw->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    msPixmapPrivPtr front_pix = msGetPixmapPriv(&ms->drmmode, front_priv->pixmap);
    msPixmapPrivPtr back_pix = msGetPixmapPriv(&ms->drmmode, back_priv->pixmap);
    msPixmapPrivRec tmp_pix;
    RegionRec region;
    int tmp;

    /* Swap BO names so DRI works */
    tmp = front->name;
    front->name = back->name;
    back->name = tmp;

    /* Swap pixmap privates */
    tmp_pix = *front_pix;
    *front_pix = *back_pix;
    *back_pix = tmp_pix;

    ms->glamor.egl_exchange_buffers(front_priv->pixmap, back_priv->pixmap);

    /* Post damage on the front buffer so that listeners, such
     * as DisplayLink know take a copy and shove it over the USB.
     */
    region.extents.x1 = region.extents.y1 = 0;
    region.extents.x2 = front_priv->pixmap->drawable.width;
    region.extents.y2 = front_priv->pixmap->drawable.height;
    region.data = NULL;
    DamageRegionAppend(&front_priv->pixmap->drawable, &region);
    DamageRegionProcessPending(&front_priv->pixmap->drawable);
}

static void
ms_dri2_frame_event_handler(uint64_t msc,
                            uint64_t usec,
                            void *data)
{
    ms_dri2_frame_event_ptr frame_info = data;
    DrawablePtr drawable = frame_info->drawable;
    ScreenPtr screen = frame_info->screen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    uint32_t tv_sec = usec / 1000000;
    uint32_t tv_usec = usec % 1000000;

    if (!drawable) {
        ms_dri2_del_frame_event(frame_info);
        return;
    }

    switch (frame_info->type) {
    case MS_DRI2_QUEUE_FLIP:
        if (can_flip(scrn, drawable, frame_info->front, frame_info->back) &&
            ms_dri2_schedule_flip(frame_info)) {
            ms_dri2_exchange_buffers(drawable, frame_info->front, frame_info->back);
            break;
        }
        /* else fall through to blit */
    case MS_DRI2_QUEUE_SWAP:
        ms_dri2_blit_swap(drawable, frame_info->front, frame_info->back);
        DRI2SwapComplete(frame_info->client, drawable, msc, tv_sec, tv_usec,
                         DRI2_BLIT_COMPLETE,
                         frame_info->client ? frame_info->event_complete : NULL,
                         frame_info->event_data);
        break;

    case MS_DRI2_WAIT_MSC:
        if (frame_info->client)
            DRI2WaitMSCComplete(frame_info->client, drawable,
                                msc, tv_sec, tv_usec);
        break;

    default:
        xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                   "%s: unknown vblank event (type %d) received\n", __func__,
                   frame_info->type);
        break;
    }

    ms_dri2_del_frame_event(frame_info);
}

static void
ms_dri2_frame_event_abort(void *data)
{
    ms_dri2_frame_event_ptr frame_info = data;

    ms_dri2_del_frame_event(frame_info);
}

/**
 * Request a DRM event when the requested conditions will be satisfied.
 *
 * We need to handle the event and ask the server to wake up the client when
 * we receive it.
 */
static int
ms_dri2_schedule_wait_msc(ClientPtr client, DrawablePtr draw, CARD64 target_msc,
                          CARD64 divisor, CARD64 remainder)
{
    ScreenPtr screen = draw->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    ms_dri2_frame_event_ptr wait_info;
    int ret;
    xf86CrtcPtr crtc = ms_dri2_crtc_covering_drawable(draw);
    CARD64 current_msc, current_ust, request_msc;
    uint32_t seq;
    uint64_t queued_msc;

    /* Drawable not visible, return immediately */
    if (!crtc)
        goto out_complete;

    wait_info = calloc(1, sizeof(*wait_info));
    if (!wait_info)
        goto out_complete;

    wait_info->screen = screen;
    wait_info->drawable = draw;
    wait_info->client = client;
    wait_info->type = MS_DRI2_WAIT_MSC;

    if (!ms_dri2_add_frame_event(wait_info)) {
        free(wait_info);
        wait_info = NULL;
        goto out_complete;
    }

    /* Get current count */
    ret = ms_get_crtc_ust_msc(crtc, &current_ust, &current_msc);

    /*
     * If divisor is zero, or current_msc is smaller than target_msc,
     * we just need to make sure target_msc passes  before waking up the
     * client.
     */
    if (divisor == 0 || current_msc < target_msc) {
        /* If target_msc already reached or passed, set it to
         * current_msc to ensure we return a reasonable value back
         * to the caller. This keeps the client from continually
         * sending us MSC targets from the past by forcibly updating
         * their count on this call.
         */
        seq = ms_drm_queue_alloc(crtc, wait_info,
                                 ms_dri2_frame_event_handler,
                                 ms_dri2_frame_event_abort);
        if (!seq)
            goto out_free;

        if (current_msc >= target_msc)
            target_msc = current_msc;

        ret = ms_queue_vblank(crtc, MS_QUEUE_ABSOLUTE, target_msc, &queued_msc, seq);
        if (!ret) {
            static int limit = 5;
            if (limit) {
                xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                           "%s:%d get vblank counter failed: %s\n",
                           __FUNCTION__, __LINE__,
                           strerror(errno));
                limit--;
            }
            goto out_free;
        }

        wait_info->frame = queued_msc;
        DRI2BlockClient(client, draw);
        return TRUE;
    }

    /*
     * If we get here, target_msc has already passed or we don't have one,
     * so we queue an event that will satisfy the divisor/remainder equation.
     */
    request_msc = current_msc - (current_msc % divisor) +
        remainder;
    /*
     * If calculated remainder is larger than requested remainder,
     * it means we've passed the last point where
     * seq % divisor == remainder, so we need to wait for the next time
     * that will happen.
     */
    if ((current_msc % divisor) >= remainder)
        request_msc += divisor;

    seq = ms_drm_queue_alloc(crtc, wait_info,
                             ms_dri2_frame_event_handler,
                             ms_dri2_frame_event_abort);
    if (!seq)
        goto out_free;

    if (!ms_queue_vblank(crtc, MS_QUEUE_ABSOLUTE, request_msc, &queued_msc, seq)) {
        static int limit = 5;
        if (limit) {
            xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                       "%s:%d get vblank counter failed: %s\n",
                       __FUNCTION__, __LINE__,
                       strerror(errno));
            limit--;
        }
        goto out_free;
    }

    wait_info->frame = queued_msc;

    DRI2BlockClient(client, draw);

    return TRUE;

 out_free:
    ms_dri2_del_frame_event(wait_info);
 out_complete:
    DRI2WaitMSCComplete(client, draw, target_msc, 0, 0);
    return TRUE;
}

/**
 * ScheduleSwap is responsible for requesting a DRM vblank event for
 * the appropriate frame, or executing the swap immediately if it
 * doesn't need to wait.
 *
 * When the swap is complete, the driver should call into the server so it
 * can send any swap complete events that have been requested.
 */
static int
ms_dri2_schedule_swap(ClientPtr client, DrawablePtr draw,
                      DRI2BufferPtr front, DRI2BufferPtr back,
                      CARD64 *target_msc, CARD64 divisor,
                      CARD64 remainder, DRI2SwapEventPtr func, void *data)
{
    ScreenPtr screen = draw->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    int ret, flip = 0;
    xf86CrtcPtr crtc = ms_dri2_crtc_covering_drawable(draw);
    ms_dri2_frame_event_ptr frame_info = NULL;
    uint64_t current_msc, current_ust;
    uint64_t request_msc;
    uint32_t seq;
    ms_queue_flag ms_flag = MS_QUEUE_ABSOLUTE;
    uint64_t queued_msc;

    /* Drawable not displayed... just complete the swap */
    if (!crtc)
        goto blit_fallback;

    frame_info = calloc(1, sizeof(*frame_info));
    if (!frame_info)
        goto blit_fallback;

    frame_info->screen = screen;
    frame_info->drawable = draw;
    frame_info->client = client;
    frame_info->event_complete = func;
    frame_info->event_data = data;
    frame_info->front = front;
    frame_info->back = back;
    frame_info->crtc = crtc;
    frame_info->type = MS_DRI2_QUEUE_SWAP;

    if (!ms_dri2_add_frame_event(frame_info)) {
        free(frame_info);
        frame_info = NULL;
        goto blit_fallback;
    }

    ms_dri2_reference_buffer(front);
    ms_dri2_reference_buffer(back);

    ret = ms_get_crtc_ust_msc(crtc, &current_ust, &current_msc);
    if (ret != Success)
        goto blit_fallback;

    /* Flips need to be submitted one frame before */
    if (can_flip(scrn, draw, front, back)) {
        frame_info->type = MS_DRI2_QUEUE_FLIP;
        flip = 1;
    }

    /* Correct target_msc by 'flip' if frame_info->type == MS_DRI2_QUEUE_FLIP.
     * Do it early, so handling of different timing constraints
     * for divisor, remainder and msc vs. target_msc works.
     */
    if (*target_msc > 0)
        *target_msc -= flip;

    /* If non-pageflipping, but blitting/exchanging, we need to use
     * DRM_VBLANK_NEXTONMISS to avoid unreliable timestamping later
     * on.
     */
    if (flip == 0)
        ms_flag |= MS_QUEUE_NEXT_ON_MISS;

    /*
     * If divisor is zero, or current_msc is smaller than target_msc
     * we just need to make sure target_msc passes before initiating
     * the swap.
     */
    if (divisor == 0 || current_msc < *target_msc) {

        /* If target_msc already reached or passed, set it to
         * current_msc to ensure we return a reasonable value back
         * to the caller. This makes swap_interval logic more robust.
         */
        if (current_msc >= *target_msc)
            *target_msc = current_msc;

        seq = ms_drm_queue_alloc(crtc, frame_info,
                                 ms_dri2_frame_event_handler,
                                 ms_dri2_frame_event_abort);
        if (!seq)
            goto blit_fallback;

        if (!ms_queue_vblank(crtc, ms_flag, *target_msc, &queued_msc, seq)) {
            xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                       "divisor 0 get vblank counter failed: %s\n",
                       strerror(errno));
            goto blit_fallback;
        }

        *target_msc = queued_msc + flip;
        frame_info->frame = *target_msc;

        return TRUE;
    }

    /*
     * If we get here, target_msc has already passed or we don't have one,
     * and we need to queue an event that will satisfy the divisor/remainder
     * equation.
     */

    request_msc = current_msc - (current_msc % divisor) +
        remainder;

    /*
     * If the calculated deadline vbl.request.sequence is smaller than
     * or equal to current_msc, it means we've passed the last point
     * when effective onset frame seq could satisfy
     * seq % divisor == remainder, so we need to wait for the next time
     * this will happen.

     * This comparison takes the DRM_VBLANK_NEXTONMISS delay into account.
     */
    if (request_msc <= current_msc)
        request_msc += divisor;

    seq = ms_drm_queue_alloc(crtc, frame_info,
                             ms_dri2_frame_event_handler,
                             ms_dri2_frame_event_abort);
    if (!seq)
        goto blit_fallback;

    /* Account for 1 frame extra pageflip delay if flip > 0 */
    if (!ms_queue_vblank(crtc, ms_flag, request_msc - flip, &queued_msc, seq)) {
        xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                   "final get vblank counter failed: %s\n",
                   strerror(errno));
        goto blit_fallback;
    }

    /* Adjust returned value for 1 fame pageflip offset of flip > 0 */
    *target_msc = queued_msc + flip;
    frame_info->frame = *target_msc;

    return TRUE;

 blit_fallback:
    ms_dri2_blit_swap(draw, front, back);
    DRI2SwapComplete(client, draw, 0, 0, 0, DRI2_BLIT_COMPLETE, func, data);
    if (frame_info)
        ms_dri2_del_frame_event(frame_info);
    *target_msc = 0; /* offscreen, so zero out target vblank count */
    return TRUE;
}

static int
ms_dri2_frame_event_client_gone(void *data, XID id)
{
    struct ms_dri2_resource *resource = data;

    while (!xorg_list_is_empty(&resource->list)) {
        ms_dri2_frame_event_ptr info =
            xorg_list_first_entry(&resource->list,
                                  ms_dri2_frame_event_rec,
                                  client_resource);

        xorg_list_del(&info->client_resource);
        info->client = NULL;
    }
    free(resource);

    return Success;
}

static int
ms_dri2_frame_event_drawable_gone(void *data, XID id)
{
    struct ms_dri2_resource *resource = data;

    while (!xorg_list_is_empty(&resource->list)) {
        ms_dri2_frame_event_ptr info =
            xorg_list_first_entry(&resource->list,
                                  ms_dri2_frame_event_rec,
                                  drawable_resource);

        xorg_list_del(&info->drawable_resource);
        info->drawable = NULL;
    }
    free(resource);

    return Success;
}

static Bool
ms_dri2_register_frame_event_resource_types(void)
{
    frame_event_client_type =
        CreateNewResourceType(ms_dri2_frame_event_client_gone,
                              "Frame Event Client");
    if (!frame_event_client_type)
        return FALSE;

    frame_event_drawable_type =
        CreateNewResourceType(ms_dri2_frame_event_drawable_gone,
                              "Frame Event Drawable");
    if (!frame_event_drawable_type)
        return FALSE;

    return TRUE;
}

Bool
ms_dri2_screen_init(ScreenPtr screen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    DRI2InfoRec info;
    const char *driver_names[2] = { NULL, NULL };

    if (!ms->glamor.supports_pixmap_import_export(screen)) {
        xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                   "DRI2: glamor lacks support for pixmap import/export\n");
    }

    if (!xf86LoaderCheckSymbol("DRI2Version"))
        return FALSE;

    if (!dixRegisterPrivateKey(&ms_dri2_client_key,
                               PRIVATE_CLIENT, sizeof(XID)))
        return FALSE;

    if (serverGeneration != ms_dri2_server_generation) {
        ms_dri2_server_generation = serverGeneration;
        if (!ms_dri2_register_frame_event_resource_types()) {
            xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                       "Cannot register DRI2 frame event resources\n");
            return FALSE;
        }
    }

    memset(&info, '\0', sizeof(info));
    info.fd = ms->fd;
    info.driverName = NULL; /* Compat field, unused. */
    info.deviceName = drmGetDeviceNameFromFd(ms->fd);

    info.version = 9;
    info.CreateBuffer = ms_dri2_create_buffer;
    info.DestroyBuffer = ms_dri2_destroy_buffer;
    info.CopyRegion = ms_dri2_copy_region;
    info.ScheduleSwap = ms_dri2_schedule_swap;
    info.GetMSC = ms_dri2_get_msc;
    info.ScheduleWaitMSC = ms_dri2_schedule_wait_msc;
    info.CreateBuffer2 = ms_dri2_create_buffer2;
    info.DestroyBuffer2 = ms_dri2_destroy_buffer2;
    info.CopyRegion2 = ms_dri2_copy_region2;

    /* Ask Glamor to obtain the DRI driver name via EGL_MESA_query_driver, */
    if (ms->glamor.egl_get_driver_name)
        driver_names[0] = ms->glamor.egl_get_driver_name(screen);

    if (driver_names[0]) {
        /* There is no VDPAU driver for Intel, fallback to the generic
         * OpenGL/VAAPI va_gl backend to emulate VDPAU.  Otherwise,
         * guess that the DRI and VDPAU drivers have the same name.
         */
        if (strcmp(driver_names[0], "i965") == 0 ||
            strcmp(driver_names[0], "iris") == 0 ||
            strcmp(driver_names[0], "crocus") == 0) {
            driver_names[1] = "va_gl";
        } else {
            driver_names[1] = driver_names[0];
        }

        info.numDrivers = 2;
        info.driverNames = driver_names;
    } else {
        /* EGL_MESA_query_driver was unavailable; let dri2.c select the
         * driver and fill in these fields for us.
         */
        info.numDrivers = 0;
        info.driverNames = NULL;
    }

    return DRI2ScreenInit(screen, &info);
}

void
ms_dri2_close_screen(ScreenPtr screen)
{
    DRI2CloseScreen(screen);
}

#endif /* GLAMOR_HAS_GBM */
