/*
 * Copyright © 2007, 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@redhat.com)
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <errno.h>
#ifdef WITH_LIBDRM
#include <xf86drm.h>
#endif
#include "list.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "dixstruct.h"
#include "dri2.h"
#include "dri2int.h"
#include "damage.h"
#include "xf86.h"

CARD8 dri2_major;               /* version of DRI2 supported by DDX */
CARD8 dri2_minor;

uint32_t prime_id_allocate_bitmask;

static DevPrivateKeyRec dri2ScreenPrivateKeyRec;

#define dri2ScreenPrivateKey (&dri2ScreenPrivateKeyRec)

static DevPrivateKeyRec dri2WindowPrivateKeyRec;

#define dri2WindowPrivateKey (&dri2WindowPrivateKeyRec)

static DevPrivateKeyRec dri2PixmapPrivateKeyRec;

#define dri2PixmapPrivateKey (&dri2PixmapPrivateKeyRec)

static DevPrivateKeyRec dri2ClientPrivateKeyRec;

#define dri2ClientPrivateKey (&dri2ClientPrivateKeyRec)

#define dri2ClientPrivate(_pClient) (dixLookupPrivate(&(_pClient)->devPrivates, \
                                                      dri2ClientPrivateKey))

typedef struct _DRI2Client {
    int prime_id;
} DRI2ClientRec, *DRI2ClientPtr;

static RESTYPE dri2DrawableRes;

typedef struct _DRI2Screen *DRI2ScreenPtr;

typedef struct _DRI2Drawable {
    DRI2ScreenPtr dri2_screen;
    DrawablePtr drawable;
    struct xorg_list reference_list;
    int width;
    int height;
    DRI2BufferPtr *buffers;
    int bufferCount;
    unsigned int swapsPending;
    int swap_interval;
    CARD64 swap_count;
    int64_t target_sbc;         /* -1 means no SBC wait outstanding */
    CARD64 last_swap_target;    /* most recently queued swap target */
    CARD64 last_swap_msc;       /* msc at completion of most recent swap */
    CARD64 last_swap_ust;       /* ust at completion of most recent swap */
    int swap_limit;             /* for N-buffering */
    unsigned blocked[3];
    Bool needInvalidate;
    int prime_id;
    PixmapPtr prime_secondary_pixmap;
    PixmapPtr redirectpixmap;
} DRI2DrawableRec, *DRI2DrawablePtr;

typedef struct _DRI2Screen {
    ScreenPtr screen;
    int refcnt;
    unsigned int numDrivers;
    const char **driverNames;
    const char *deviceName;
    int fd;
    unsigned int lastSequence;
    int prime_id;

    DRI2CreateBufferProcPtr CreateBuffer;
    DRI2DestroyBufferProcPtr DestroyBuffer;
    DRI2CopyRegionProcPtr CopyRegion;
    DRI2ScheduleSwapProcPtr ScheduleSwap;
    DRI2GetMSCProcPtr GetMSC;
    DRI2ScheduleWaitMSCProcPtr ScheduleWaitMSC;
    DRI2AuthMagic2ProcPtr AuthMagic;
    DRI2AuthMagicProcPtr LegacyAuthMagic;
    DRI2ReuseBufferNotifyProcPtr ReuseBufferNotify;
    DRI2SwapLimitValidateProcPtr SwapLimitValidate;
    DRI2GetParamProcPtr GetParam;

    HandleExposuresProcPtr HandleExposures;

    ConfigNotifyProcPtr ConfigNotify;
    SetWindowPixmapProcPtr SetWindowPixmap;
    DRI2CreateBuffer2ProcPtr CreateBuffer2;
    DRI2DestroyBuffer2ProcPtr DestroyBuffer2;
    DRI2CopyRegion2ProcPtr CopyRegion2;
} DRI2ScreenRec;

static void
destroy_buffer(DrawablePtr pDraw, DRI2BufferPtr buffer, int prime_id);

enum DRI2WakeType {
    WAKE_SBC,
    WAKE_MSC,
    WAKE_SWAP,
};

#define Wake(c, t) (void *)((uintptr_t)(c) | (t))

static Bool
dri2WakeClient(ClientPtr client, void *closure)
{
    ClientWakeup(client);
    return TRUE;
}

static Bool
dri2WakeAll(ClientPtr client, DRI2DrawablePtr pPriv, enum DRI2WakeType t)
{
    int count;

    if (!pPriv->blocked[t])
        return FALSE;

    count = ClientSignalAll(client, dri2WakeClient, Wake(pPriv, t));
    pPriv->blocked[t] -= count;
    return count;
}

static Bool
dri2Sleep(ClientPtr client, DRI2DrawablePtr pPriv, enum DRI2WakeType t)
{
    if (ClientSleep(client, dri2WakeClient, Wake(pPriv, t))) {
        pPriv->blocked[t]++;
        return TRUE;
    }
    return FALSE;
}

static DRI2ScreenPtr
DRI2GetScreen(ScreenPtr pScreen)
{
    return dixLookupPrivate(&pScreen->devPrivates, dri2ScreenPrivateKey);
}

static ScreenPtr
GetScreenPrime(ScreenPtr primary, int prime_id)
{
    ScreenPtr secondary;
    if (prime_id == 0) {
        return primary;
    }
    xorg_list_for_each_entry(secondary, &primary->secondary_list, secondary_head) {
        DRI2ScreenPtr ds;

        if (!secondary->is_offload_secondary)
            continue;

        ds = DRI2GetScreen(secondary);
        if (ds == NULL)
            continue;

        if (ds->prime_id == prime_id)
            return secondary;
    }
    return primary;
}

static DRI2ScreenPtr
DRI2GetScreenPrime(ScreenPtr primary, int prime_id)
{
    ScreenPtr secondary = GetScreenPrime(primary, prime_id);
    return DRI2GetScreen(secondary);
}

static DRI2DrawablePtr
DRI2GetDrawable(DrawablePtr pDraw)
{
    WindowPtr pWin;
    PixmapPtr pPixmap;

    switch (pDraw->type) {
    case DRAWABLE_WINDOW:
        pWin = (WindowPtr) pDraw;
        return dixLookupPrivate(&pWin->devPrivates, dri2WindowPrivateKey);
    case DRAWABLE_PIXMAP:
        pPixmap = (PixmapPtr) pDraw;
        return dixLookupPrivate(&pPixmap->devPrivates, dri2PixmapPrivateKey);
    default:
        return NULL;
    }
}

static DRI2DrawablePtr
DRI2AllocateDrawable(DrawablePtr pDraw)
{
    DRI2ScreenPtr ds = DRI2GetScreen(pDraw->pScreen);
    DRI2DrawablePtr pPriv;
    CARD64 ust;
    WindowPtr pWin;
    PixmapPtr pPixmap;

    pPriv = malloc(sizeof *pPriv);
    if (pPriv == NULL)
        return NULL;

    pPriv->dri2_screen = ds;
    pPriv->drawable = pDraw;
    pPriv->width = pDraw->width;
    pPriv->height = pDraw->height;
    pPriv->buffers = NULL;
    pPriv->bufferCount = 0;
    pPriv->swapsPending = 0;
    pPriv->swap_count = 0;
    pPriv->target_sbc = -1;
    pPriv->swap_interval = 1;
    /* Initialize last swap target from DDX if possible */
    if (!ds->GetMSC || !(*ds->GetMSC) (pDraw, &ust, &pPriv->last_swap_target))
        pPriv->last_swap_target = 0;

    memset(pPriv->blocked, 0, sizeof(pPriv->blocked));
    pPriv->swap_limit = 1;      /* default to double buffering */
    pPriv->last_swap_msc = 0;
    pPriv->last_swap_ust = 0;
    xorg_list_init(&pPriv->reference_list);
    pPriv->needInvalidate = FALSE;
    pPriv->redirectpixmap = NULL;
    pPriv->prime_secondary_pixmap = NULL;
    if (pDraw->type == DRAWABLE_WINDOW) {
        pWin = (WindowPtr) pDraw;
        dixSetPrivate(&pWin->devPrivates, dri2WindowPrivateKey, pPriv);
    }
    else {
        pPixmap = (PixmapPtr) pDraw;
        dixSetPrivate(&pPixmap->devPrivates, dri2PixmapPrivateKey, pPriv);
    }

    return pPriv;
}

Bool
DRI2SwapLimit(DrawablePtr pDraw, int swap_limit)
{
    DRI2DrawablePtr pPriv = DRI2GetDrawable(pDraw);
    DRI2ScreenPtr ds;

    if (!pPriv)
        return FALSE;

    ds = pPriv->dri2_screen;

    if (!ds->SwapLimitValidate || !ds->SwapLimitValidate(pDraw, swap_limit))
        return FALSE;

    pPriv->swap_limit = swap_limit;

    /* Check throttling */
    if (pPriv->swapsPending >= pPriv->swap_limit)
        return TRUE;

    dri2WakeAll(CLIENT_SIGNAL_ANY, pPriv, WAKE_SWAP);
    return TRUE;
}

typedef struct DRI2DrawableRefRec {
    XID id;
    XID dri2_id;
    DRI2InvalidateProcPtr invalidate;
    void *priv;
    struct xorg_list link;
} DRI2DrawableRefRec, *DRI2DrawableRefPtr;

static DRI2DrawableRefPtr
DRI2LookupDrawableRef(DRI2DrawablePtr pPriv, XID id)
{
    DRI2DrawableRefPtr ref;

    xorg_list_for_each_entry(ref, &pPriv->reference_list, link) {
        if (ref->id == id)
            return ref;
    }

    return NULL;
}

static int
DRI2AddDrawableRef(DRI2DrawablePtr pPriv, XID id, XID dri2_id,
                   DRI2InvalidateProcPtr invalidate, void *priv)
{
    DRI2DrawableRefPtr ref;

    ref = malloc(sizeof *ref);
    if (ref == NULL)
        return BadAlloc;

    if (!AddResource(dri2_id, dri2DrawableRes, pPriv)) {
        free(ref);
        return BadAlloc;
    }
    if (!DRI2LookupDrawableRef(pPriv, id))
        if (!AddResource(id, dri2DrawableRes, pPriv)) {
            FreeResourceByType(dri2_id, dri2DrawableRes, TRUE);
            free(ref);
            return BadAlloc;
        }

    ref->id = id;
    ref->dri2_id = dri2_id;
    ref->invalidate = invalidate;
    ref->priv = priv;
    xorg_list_add(&ref->link, &pPriv->reference_list);

    return Success;
}

int
DRI2CreateDrawable2(ClientPtr client, DrawablePtr pDraw, XID id,
                    DRI2InvalidateProcPtr invalidate, void *priv,
                    XID *dri2_id_out)
{
    DRI2DrawablePtr pPriv;
    DRI2ClientPtr dri2_client = dri2ClientPrivate(client);
    XID dri2_id;
    int rc;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL)
        pPriv = DRI2AllocateDrawable(pDraw);
    if (pPriv == NULL)
        return BadAlloc;

    pPriv->prime_id = dri2_client->prime_id;

    dri2_id = FakeClientID(client->index);
    rc = DRI2AddDrawableRef(pPriv, id, dri2_id, invalidate, priv);
    if (rc != Success)
        return rc;

    if (dri2_id_out)
        *dri2_id_out = dri2_id;

    return Success;
}

int
DRI2CreateDrawable(ClientPtr client, DrawablePtr pDraw, XID id,
                   DRI2InvalidateProcPtr invalidate, void *priv)
{
    return DRI2CreateDrawable2(client, pDraw, id, invalidate, priv, NULL);
}

static int
DRI2DrawableGone(void *p, XID id)
{
    DRI2DrawablePtr pPriv = p;
    DRI2DrawableRefPtr ref, next;
    WindowPtr pWin;
    PixmapPtr pPixmap;
    DrawablePtr pDraw;
    int i;

    xorg_list_for_each_entry_safe(ref, next, &pPriv->reference_list, link) {
        if (ref->dri2_id == id) {
            xorg_list_del(&ref->link);
            /* If this was the last ref under this X drawable XID,
             * unregister the X drawable resource. */
            if (!DRI2LookupDrawableRef(pPriv, ref->id))
                FreeResourceByType(ref->id, dri2DrawableRes, TRUE);
            free(ref);
            break;
        }

        if (ref->id == id) {
            xorg_list_del(&ref->link);
            FreeResourceByType(ref->dri2_id, dri2DrawableRes, TRUE);
            free(ref);
        }
    }

    if (!xorg_list_is_empty(&pPriv->reference_list))
        return Success;

    pDraw = pPriv->drawable;
    if (pDraw->type == DRAWABLE_WINDOW) {
        pWin = (WindowPtr) pDraw;
        dixSetPrivate(&pWin->devPrivates, dri2WindowPrivateKey, NULL);
    }
    else {
        pPixmap = (PixmapPtr) pDraw;
        dixSetPrivate(&pPixmap->devPrivates, dri2PixmapPrivateKey, NULL);
    }

    if (pPriv->prime_secondary_pixmap) {
        (*pPriv->prime_secondary_pixmap->primary_pixmap->drawable.pScreen->DestroyPixmap)(pPriv->prime_secondary_pixmap->primary_pixmap);
        (*pPriv->prime_secondary_pixmap->drawable.pScreen->DestroyPixmap)(pPriv->prime_secondary_pixmap);
    }

    if (pPriv->buffers != NULL) {
        for (i = 0; i < pPriv->bufferCount; i++)
            destroy_buffer(pDraw, pPriv->buffers[i], pPriv->prime_id);

        free(pPriv->buffers);
    }

    if (pPriv->redirectpixmap) {
        (*pDraw->pScreen->ReplaceScanoutPixmap)(pDraw, pPriv->redirectpixmap, FALSE);
        (*pDraw->pScreen->DestroyPixmap)(pPriv->redirectpixmap);
    }

    dri2WakeAll(CLIENT_SIGNAL_ANY, pPriv, WAKE_SWAP);
    dri2WakeAll(CLIENT_SIGNAL_ANY, pPriv, WAKE_MSC);
    dri2WakeAll(CLIENT_SIGNAL_ANY, pPriv, WAKE_SBC);

    free(pPriv);

    return Success;
}

static DRI2BufferPtr
create_buffer(DRI2ScreenPtr ds, DrawablePtr pDraw,
              unsigned int attachment, unsigned int format)
{
    DRI2BufferPtr buffer;
    if (ds->CreateBuffer2)
        buffer = (*ds->CreateBuffer2)(GetScreenPrime(pDraw->pScreen,
                                                     DRI2GetDrawable(pDraw)->prime_id),
                                      pDraw, attachment, format);
    else
        buffer = (*ds->CreateBuffer)(pDraw, attachment, format);
    return buffer;
}

static void
destroy_buffer(DrawablePtr pDraw, DRI2BufferPtr buffer, int prime_id)
{
    ScreenPtr primeScreen;
    DRI2ScreenPtr ds;
    primeScreen = GetScreenPrime(pDraw->pScreen, prime_id);
    ds = DRI2GetScreen(primeScreen);
    if (ds->DestroyBuffer2)
        (*ds->DestroyBuffer2)(primeScreen, pDraw, buffer);
    else
        (*ds->DestroyBuffer)(pDraw, buffer);
}

static int
find_attachment(DRI2DrawablePtr pPriv, unsigned attachment)
{
    int i;

    if (pPriv->buffers == NULL) {
        return -1;
    }

    for (i = 0; i < pPriv->bufferCount; i++) {
        if ((pPriv->buffers[i] != NULL)
            && (pPriv->buffers[i]->attachment == attachment)) {
            return i;
        }
    }

    return -1;
}

static Bool
allocate_or_reuse_buffer(DrawablePtr pDraw, DRI2ScreenPtr ds,
                         DRI2DrawablePtr pPriv,
                         unsigned int attachment, unsigned int format,
                         int dimensions_match, DRI2BufferPtr * buffer)
{
    int old_buf = find_attachment(pPriv, attachment);

    if ((old_buf < 0)
        || attachment == DRI2BufferFrontLeft
        || !dimensions_match || (pPriv->buffers[old_buf]->format != format)) {
        *buffer = create_buffer(ds, pDraw, attachment, format);
        return TRUE;

    }
    else {
        *buffer = pPriv->buffers[old_buf];

        if (ds->ReuseBufferNotify)
            (*ds->ReuseBufferNotify) (pDraw, *buffer);

        pPriv->buffers[old_buf] = NULL;
        return FALSE;
    }
}

static void
update_dri2_drawable_buffers(DRI2DrawablePtr pPriv, DrawablePtr pDraw,
                             DRI2BufferPtr * buffers, int out_count, int *width,
                             int *height)
{
    int i;

    if (pPriv->buffers != NULL) {
        for (i = 0; i < pPriv->bufferCount; i++) {
            if (pPriv->buffers[i] != NULL) {
                destroy_buffer(pDraw, pPriv->buffers[i], pPriv->prime_id);
            }
        }

        free(pPriv->buffers);
    }

    pPriv->buffers = buffers;
    pPriv->bufferCount = out_count;
    pPriv->width = pDraw->width;
    pPriv->height = pDraw->height;
    *width = pPriv->width;
    *height = pPriv->height;
}

static DRI2BufferPtr *
do_get_buffers(DrawablePtr pDraw, int *width, int *height,
               unsigned int *attachments, int count, int *out_count,
               int has_format)
{
    DRI2DrawablePtr pPriv = DRI2GetDrawable(pDraw);
    DRI2ScreenPtr ds;
    DRI2BufferPtr *buffers;
    int need_real_front = 0;
    int need_fake_front = 0;
    int have_fake_front = 0;
    int front_format = 0;
    int dimensions_match;
    int buffers_changed = 0;
    int i;

    if (!pPriv) {
        *width = pDraw->width;
        *height = pDraw->height;
        *out_count = 0;
        return NULL;
    }

    ds = DRI2GetScreenPrime(pDraw->pScreen, pPriv->prime_id);

    dimensions_match = (pDraw->width == pPriv->width)
        && (pDraw->height == pPriv->height);

    buffers = calloc((count + 1), sizeof(buffers[0]));
    if (!buffers)
        goto err_out;

    for (i = 0; i < count; i++) {
        const unsigned attachment = *(attachments++);
        const unsigned format = (has_format) ? *(attachments++) : 0;

        if (allocate_or_reuse_buffer(pDraw, ds, pPriv, attachment,
                                     format, dimensions_match, &buffers[i]))
            buffers_changed = 1;

        if (buffers[i] == NULL)
            goto err_out;

        /* If the drawable is a window and the front-buffer is requested,
         * silently add the fake front-buffer to the list of requested
         * attachments.  The counting logic in the loop accounts for the case
         * where the client requests both the fake and real front-buffer.
         */
        if (attachment == DRI2BufferBackLeft) {
            need_real_front++;
            front_format = format;
        }

        if (attachment == DRI2BufferFrontLeft) {
            need_real_front--;
            front_format = format;

            if (pDraw->type == DRAWABLE_WINDOW) {
                need_fake_front++;
            }
        }

        if (pDraw->type == DRAWABLE_WINDOW) {
            if (attachment == DRI2BufferFakeFrontLeft) {
                need_fake_front--;
                have_fake_front = 1;
            }
        }
    }

    if (need_real_front > 0) {
        if (allocate_or_reuse_buffer(pDraw, ds, pPriv, DRI2BufferFrontLeft,
                                     front_format, dimensions_match,
                                     &buffers[i]))
            buffers_changed = 1;

        if (buffers[i] == NULL)
            goto err_out;
        i++;
    }

    if (need_fake_front > 0) {
        if (allocate_or_reuse_buffer(pDraw, ds, pPriv, DRI2BufferFakeFrontLeft,
                                     front_format, dimensions_match,
                                     &buffers[i]))
            buffers_changed = 1;

        if (buffers[i] == NULL)
            goto err_out;

        i++;
        have_fake_front = 1;
    }

    *out_count = i;

    update_dri2_drawable_buffers(pPriv, pDraw, buffers, *out_count, width,
                                 height);

    /* If the client is getting a fake front-buffer, pre-fill it with the
     * contents of the real front-buffer.  This ensures correct operation of
     * applications that call glXWaitX before calling glDrawBuffer.
     */
    if (have_fake_front && buffers_changed) {
        BoxRec box;
        RegionRec region;

        box.x1 = 0;
        box.y1 = 0;
        box.x2 = pPriv->width;
        box.y2 = pPriv->height;
        RegionInit(&region, &box, 0);

        DRI2CopyRegion(pDraw, &region, DRI2BufferFakeFrontLeft,
                       DRI2BufferFrontLeft);
    }

    pPriv->needInvalidate = TRUE;

    return pPriv->buffers;

 err_out:

    *out_count = 0;

    if (buffers) {
        for (i = 0; i < count; i++) {
            if (buffers[i] != NULL)
                destroy_buffer(pDraw, buffers[i], 0);
        }

        free(buffers);
        buffers = NULL;
    }

    update_dri2_drawable_buffers(pPriv, pDraw, buffers, *out_count, width,
                                 height);

    return buffers;
}

DRI2BufferPtr *
DRI2GetBuffers(DrawablePtr pDraw, int *width, int *height,
               unsigned int *attachments, int count, int *out_count)
{
    return do_get_buffers(pDraw, width, height, attachments, count,
                          out_count, FALSE);
}

DRI2BufferPtr *
DRI2GetBuffersWithFormat(DrawablePtr pDraw, int *width, int *height,
                         unsigned int *attachments, int count, int *out_count)
{
    return do_get_buffers(pDraw, width, height, attachments, count,
                          out_count, TRUE);
}

static void
DRI2InvalidateDrawable(DrawablePtr pDraw)
{
    DRI2DrawablePtr pPriv = DRI2GetDrawable(pDraw);
    DRI2DrawableRefPtr ref;

    if (!pPriv || !pPriv->needInvalidate)
        return;

    pPriv->needInvalidate = FALSE;

    xorg_list_for_each_entry(ref, &pPriv->reference_list, link)
        ref->invalidate(pDraw, ref->priv, ref->id);
}

/*
 * In the direct rendered case, we throttle the clients that have more
 * than their share of outstanding swaps (and thus busy buffers) when a
 * new GetBuffers request is received.  In the AIGLX case, we allow the
 * client to get the new buffers, but throttle when the next GLX request
 * comes in (see __glXDRIcontextWait()).
 */
Bool
DRI2ThrottleClient(ClientPtr client, DrawablePtr pDraw)
{
    DRI2DrawablePtr pPriv;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL)
        return FALSE;

    /* Throttle to swap limit */
    if (pPriv->swapsPending >= pPriv->swap_limit) {
        if (dri2Sleep(client, pPriv, WAKE_SWAP)) {
            ResetCurrentRequest(client);
            client->sequence--;
            return TRUE;
        }
    }

    return FALSE;
}

void
DRI2BlockClient(ClientPtr client, DrawablePtr pDraw)
{
    DRI2DrawablePtr pPriv;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL)
        return;

    dri2Sleep(client, pPriv, WAKE_MSC);
}

static inline PixmapPtr GetDrawablePixmap(DrawablePtr drawable)
{
    if (drawable->type == DRAWABLE_PIXMAP)
        return (PixmapPtr)drawable;
    else {
        struct _Window *pWin = (struct _Window *)drawable;
        return drawable->pScreen->GetWindowPixmap(pWin);
    }
}

/*
 * A TraverseTree callback to invalidate all windows using the same
 * pixmap
 */
static int
DRI2InvalidateWalk(WindowPtr pWin, void *data)
{
    if (pWin->drawable.pScreen->GetWindowPixmap(pWin) != data)
        return WT_DONTWALKCHILDREN;
    DRI2InvalidateDrawable(&pWin->drawable);
    return WT_WALKCHILDREN;
}

static void
DRI2InvalidateDrawableAll(DrawablePtr pDraw)
{
    if (pDraw->type == DRAWABLE_WINDOW) {
        WindowPtr pWin = (WindowPtr) pDraw;
        PixmapPtr pPixmap = pDraw->pScreen->GetWindowPixmap(pWin);

        /*
         * Find the top-most window using this pixmap
         */
        while (pWin->parent &&
               pDraw->pScreen->GetWindowPixmap(pWin->parent) == pPixmap)
            pWin = pWin->parent;

        /*
         * Walk the sub-tree to invalidate all of the
         * windows using the same pixmap
         */
        TraverseTree(pWin, DRI2InvalidateWalk, pPixmap);
        DRI2InvalidateDrawable(&pPixmap->drawable);
    }
    else
        DRI2InvalidateDrawable(pDraw);
}

DrawablePtr DRI2UpdatePrime(DrawablePtr pDraw, DRI2BufferPtr pDest)
{
    DRI2DrawablePtr pPriv = DRI2GetDrawable(pDraw);
    PixmapPtr spix;
    PixmapPtr mpix = GetDrawablePixmap(pDraw);
    ScreenPtr primary, secondary;
    Bool ret;

    primary = mpix->drawable.pScreen;

    if (pDraw->type == DRAWABLE_WINDOW) {
        WindowPtr pWin = (WindowPtr)pDraw;
        PixmapPtr pPixmap = pDraw->pScreen->GetWindowPixmap(pWin);

        if (pDraw->pScreen->GetScreenPixmap(pDraw->pScreen) == pPixmap) {
            if (pPriv->redirectpixmap &&
                pPriv->redirectpixmap->drawable.width == pDraw->width &&
                pPriv->redirectpixmap->drawable.height == pDraw->height &&
                pPriv->redirectpixmap->drawable.depth == pDraw->depth) {
                mpix = pPriv->redirectpixmap;
            } else {
                if (primary->ReplaceScanoutPixmap) {
                    mpix = (*primary->CreatePixmap)(primary, pDraw->width, pDraw->height,
                                                   pDraw->depth, CREATE_PIXMAP_USAGE_SHARED);
                    if (!mpix)
                        return NULL;

                    ret = (*primary->ReplaceScanoutPixmap)(pDraw, mpix, TRUE);
                    if (ret == FALSE) {
                        (*primary->DestroyPixmap)(mpix);
                        return NULL;
                    }
                    pPriv->redirectpixmap = mpix;
                } else
                    return NULL;
            }
        } else if (pPriv->redirectpixmap) {
            (*primary->ReplaceScanoutPixmap)(pDraw, pPriv->redirectpixmap, FALSE);
            (*primary->DestroyPixmap)(pPriv->redirectpixmap);
            pPriv->redirectpixmap = NULL;
        }
    }

    secondary = GetScreenPrime(pDraw->pScreen, pPriv->prime_id);

    /* check if the pixmap is still fine */
    if (pPriv->prime_secondary_pixmap) {
        if (pPriv->prime_secondary_pixmap->primary_pixmap == mpix)
            return &pPriv->prime_secondary_pixmap->drawable;
        else {
            PixmapUnshareSecondaryPixmap(pPriv->prime_secondary_pixmap);
            (*pPriv->prime_secondary_pixmap->primary_pixmap->drawable.pScreen->DestroyPixmap)(pPriv->prime_secondary_pixmap->primary_pixmap);
            (*secondary->DestroyPixmap)(pPriv->prime_secondary_pixmap);
            pPriv->prime_secondary_pixmap = NULL;
        }
    }

    spix = PixmapShareToSecondary(mpix, secondary);
    if (!spix)
        return NULL;

    pPriv->prime_secondary_pixmap = spix;
#ifdef COMPOSITE
    spix->screen_x = mpix->screen_x;
    spix->screen_y = mpix->screen_y;
#endif

    DRI2InvalidateDrawableAll(pDraw);
    return &spix->drawable;
}

static void dri2_copy_region(DrawablePtr pDraw, RegionPtr pRegion,
                             DRI2BufferPtr pDest, DRI2BufferPtr pSrc)
{
    DRI2DrawablePtr pPriv = DRI2GetDrawable(pDraw);
    DRI2ScreenPtr ds;
    ScreenPtr primeScreen;

    primeScreen = GetScreenPrime(pDraw->pScreen, pPriv->prime_id);
    ds = DRI2GetScreen(primeScreen);

    if (ds->CopyRegion2)
        (*ds->CopyRegion2)(primeScreen, pDraw, pRegion, pDest, pSrc);
    else
        (*ds->CopyRegion) (pDraw, pRegion, pDest, pSrc);

    /* cause damage to the box */
    if (pPriv->prime_id) {
       BoxRec box;
       RegionRec region;
       box.x1 = 0;
       box.x2 = box.x1 + pDraw->width;
       box.y1 = 0;
       box.y2 = box.y1 + pDraw->height;
       RegionInit(&region, &box, 1);
       RegionTranslate(&region, pDraw->x, pDraw->y);
       DamageRegionAppend(pDraw, &region);
       DamageRegionProcessPending(pDraw);
       RegionUninit(&region);
    }
}

int
DRI2CopyRegion(DrawablePtr pDraw, RegionPtr pRegion,
               unsigned int dest, unsigned int src)
{
    DRI2DrawablePtr pPriv;
    DRI2BufferPtr pDestBuffer, pSrcBuffer;
    int i;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL)
        return BadDrawable;

    pDestBuffer = NULL;
    pSrcBuffer = NULL;
    for (i = 0; i < pPriv->bufferCount; i++) {
        if (pPriv->buffers[i]->attachment == dest)
            pDestBuffer = (DRI2BufferPtr) pPriv->buffers[i];
        if (pPriv->buffers[i]->attachment == src)
            pSrcBuffer = (DRI2BufferPtr) pPriv->buffers[i];
    }
    if (pSrcBuffer == NULL || pDestBuffer == NULL)
        return BadValue;

    dri2_copy_region(pDraw, pRegion, pDestBuffer, pSrcBuffer);

    return Success;
}

/* Can this drawable be page flipped? */
Bool
DRI2CanFlip(DrawablePtr pDraw)
{
    ScreenPtr pScreen = pDraw->pScreen;
    WindowPtr pWin, pRoot;
    PixmapPtr pWinPixmap, pRootPixmap;

    if (pDraw->type == DRAWABLE_PIXMAP)
        return TRUE;

    pRoot = pScreen->root;
    pRootPixmap = pScreen->GetWindowPixmap(pRoot);

    pWin = (WindowPtr) pDraw;
    pWinPixmap = pScreen->GetWindowPixmap(pWin);
    if (pRootPixmap != pWinPixmap)
        return FALSE;
    if (!RegionEqual(&pWin->clipList, &pRoot->winSize))
        return FALSE;

    /* Does the window match the pixmap exactly? */
    if (pDraw->x != 0 || pDraw->y != 0 ||
#ifdef COMPOSITE
        pDraw->x != pWinPixmap->screen_x || pDraw->y != pWinPixmap->screen_y ||
#endif
        pDraw->width != pWinPixmap->drawable.width ||
        pDraw->height != pWinPixmap->drawable.height)
        return FALSE;

    return TRUE;
}

/* Can we do a pixmap exchange instead of a blit? */
Bool
DRI2CanExchange(DrawablePtr pDraw)
{
    return FALSE;
}

void
DRI2WaitMSCComplete(ClientPtr client, DrawablePtr pDraw, int frame,
                    unsigned int tv_sec, unsigned int tv_usec)
{
    DRI2DrawablePtr pPriv;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL)
        return;

    ProcDRI2WaitMSCReply(client, ((CARD64) tv_sec * 1000000) + tv_usec,
                         frame, pPriv->swap_count);

    dri2WakeAll(client, pPriv, WAKE_MSC);
}

static void
DRI2WakeClient(ClientPtr client, DrawablePtr pDraw, int frame,
               unsigned int tv_sec, unsigned int tv_usec)
{
    ScreenPtr pScreen = pDraw->pScreen;
    DRI2DrawablePtr pPriv;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                   "[DRI2] %s: bad drawable\n", __func__);
        return;
    }

    /*
     * Swap completed.
     * Wake the client iff:
     *   - it was waiting on SBC
     *   - was blocked due to GLX make current
     *   - was blocked due to swap throttling
     *   - is not blocked due to an MSC wait
     */
    if (pPriv->target_sbc != -1 && pPriv->target_sbc <= pPriv->swap_count) {
        if (dri2WakeAll(client, pPriv, WAKE_SBC)) {
            ProcDRI2WaitMSCReply(client, ((CARD64) tv_sec * 1000000) + tv_usec,
                                 frame, pPriv->swap_count);
            pPriv->target_sbc = -1;
        }
    }

    dri2WakeAll(CLIENT_SIGNAL_ANY, pPriv, WAKE_SWAP);
}

void
DRI2SwapComplete(ClientPtr client, DrawablePtr pDraw, int frame,
                 unsigned int tv_sec, unsigned int tv_usec, int type,
                 DRI2SwapEventPtr swap_complete, void *swap_data)
{
    ScreenPtr pScreen = pDraw->pScreen;
    DRI2DrawablePtr pPriv;
    CARD64 ust = 0;
    BoxRec box;
    RegionRec region;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                   "[DRI2] %s: bad drawable\n", __func__);
        return;
    }

    pPriv->swapsPending--;
    pPriv->swap_count++;

    box.x1 = 0;
    box.y1 = 0;
    box.x2 = pDraw->width;
    box.y2 = pDraw->height;
    RegionInit(&region, &box, 0);
    DRI2CopyRegion(pDraw, &region, DRI2BufferFakeFrontLeft,
                   DRI2BufferFrontLeft);

    ust = ((CARD64) tv_sec * 1000000) + tv_usec;
    if (swap_complete)
        swap_complete(client, swap_data, type, ust, frame, pPriv->swap_count);

    pPriv->last_swap_msc = frame;
    pPriv->last_swap_ust = ust;

    DRI2WakeClient(client, pDraw, frame, tv_sec, tv_usec);
}

Bool
DRI2WaitSwap(ClientPtr client, DrawablePtr pDrawable)
{
    DRI2DrawablePtr pPriv = DRI2GetDrawable(pDrawable);

    /* If we're currently waiting for a swap on this drawable, reset
     * the request and suspend the client. */
    if (pPriv && pPriv->swapsPending) {
        if (dri2Sleep(client, pPriv, WAKE_SWAP)) {
            ResetCurrentRequest(client);
            client->sequence--;
            return TRUE;
        }
    }

    return FALSE;
}



int
DRI2SwapBuffers(ClientPtr client, DrawablePtr pDraw, CARD64 target_msc,
                CARD64 divisor, CARD64 remainder, CARD64 * swap_target,
                DRI2SwapEventPtr func, void *data)
{
    ScreenPtr pScreen = pDraw->pScreen;
    DRI2ScreenPtr ds = DRI2GetScreen(pDraw->pScreen);
    DRI2DrawablePtr pPriv;
    DRI2BufferPtr pDestBuffer = NULL, pSrcBuffer = NULL;
    int ret, i;
    CARD64 ust, current_msc;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                   "[DRI2] %s: bad drawable\n", __func__);
        return BadDrawable;
    }

    /* According to spec, return expected swapbuffers count SBC after this swap
     * will complete. This is ignored unless we return Success, but it must be
     * initialized on every path where we return Success or the caller will send
     * an uninitialized value off the stack to the client. So let's initialize
     * it as early as possible, just to be sure.
     */
    *swap_target = pPriv->swap_count + pPriv->swapsPending + 1;

    for (i = 0; i < pPriv->bufferCount; i++) {
        if (pPriv->buffers[i]->attachment == DRI2BufferFrontLeft)
            pDestBuffer = (DRI2BufferPtr) pPriv->buffers[i];
        if (pPriv->buffers[i]->attachment == DRI2BufferBackLeft)
            pSrcBuffer = (DRI2BufferPtr) pPriv->buffers[i];
    }
    if (pSrcBuffer == NULL || pDestBuffer == NULL) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                   "[DRI2] %s: drawable has no back or front?\n", __func__);
        return BadDrawable;
    }

    /* Old DDX or no swap interval, just blit */
    if (!ds->ScheduleSwap || !pPriv->swap_interval || pPriv->prime_id) {
        BoxRec box;
        RegionRec region;

        box.x1 = 0;
        box.y1 = 0;
        box.x2 = pDraw->width;
        box.y2 = pDraw->height;
        RegionInit(&region, &box, 0);

        pPriv->swapsPending++;

        dri2_copy_region(pDraw, &region, pDestBuffer, pSrcBuffer);
        DRI2SwapComplete(client, pDraw, target_msc, 0, 0, DRI2_BLIT_COMPLETE,
                         func, data);
        return Success;
    }

    /*
     * In the simple glXSwapBuffers case, all params will be 0, and we just
     * need to schedule a swap for the last swap target + the swap interval.
     */
    if (target_msc == 0 && divisor == 0 && remainder == 0) {
        /* If the current vblank count of the drawable's crtc is lower
         * than the count stored in last_swap_target from a previous swap
         * then reinitialize last_swap_target to the current crtc's msc,
         * otherwise the swap will hang. This will happen if the drawable
         * is moved to a crtc with a lower refresh rate, or a crtc that just
         * got enabled.
         */
        if (ds->GetMSC) {
            if (!(*ds->GetMSC) (pDraw, &ust, &current_msc))
                pPriv->last_swap_target = 0;

            if (current_msc < pPriv->last_swap_target)
                pPriv->last_swap_target = current_msc;

        }

        /*
         * Swap target for this swap is last swap target + swap interval since
         * we have to account for the current swap count, interval, and the
         * number of pending swaps.
         */
        target_msc = pPriv->last_swap_target + pPriv->swap_interval;

    }

    pPriv->swapsPending++;
    ret = (*ds->ScheduleSwap) (client, pDraw, pDestBuffer, pSrcBuffer,
                               &target_msc, divisor, remainder, func, data);
    if (!ret) {
        pPriv->swapsPending--;  /* didn't schedule */
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                   "[DRI2] %s: driver failed to schedule swap\n", __func__);
        return BadDrawable;
    }

    pPriv->last_swap_target = target_msc;

    DRI2InvalidateDrawableAll(pDraw);

    return Success;
}

void
DRI2SwapInterval(DrawablePtr pDrawable, int interval)
{
    ScreenPtr pScreen = pDrawable->pScreen;
    DRI2DrawablePtr pPriv = DRI2GetDrawable(pDrawable);

    if (pPriv == NULL) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                   "[DRI2] %s: bad drawable\n", __func__);
        return;
    }

    /* fixme: check against arbitrary max? */
    pPriv->swap_interval = interval;
}

int
DRI2GetMSC(DrawablePtr pDraw, CARD64 * ust, CARD64 * msc, CARD64 * sbc)
{
    ScreenPtr pScreen = pDraw->pScreen;
    DRI2ScreenPtr ds = DRI2GetScreen(pDraw->pScreen);
    DRI2DrawablePtr pPriv;
    Bool ret;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                   "[DRI2] %s: bad drawable\n", __func__);
        return BadDrawable;
    }

    if (!ds->GetMSC) {
        *ust = 0;
        *msc = 0;
        *sbc = pPriv->swap_count;
        return Success;
    }

    /*
     * Spec needs to be updated to include unmapped or redirected
     * drawables
     */

    ret = (*ds->GetMSC) (pDraw, ust, msc);
    if (!ret)
        return BadDrawable;

    *sbc = pPriv->swap_count;

    return Success;
}

int
DRI2WaitMSC(ClientPtr client, DrawablePtr pDraw, CARD64 target_msc,
            CARD64 divisor, CARD64 remainder)
{
    DRI2ScreenPtr ds = DRI2GetScreen(pDraw->pScreen);
    DRI2DrawablePtr pPriv;
    Bool ret;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL)
        return BadDrawable;

    /* Old DDX just completes immediately */
    if (!ds->ScheduleWaitMSC) {
        DRI2WaitMSCComplete(client, pDraw, target_msc, 0, 0);

        return Success;
    }

    ret =
        (*ds->ScheduleWaitMSC) (client, pDraw, target_msc, divisor, remainder);
    if (!ret)
        return BadDrawable;

    return Success;
}

int
DRI2WaitSBC(ClientPtr client, DrawablePtr pDraw, CARD64 target_sbc)
{
    DRI2DrawablePtr pPriv;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL)
        return BadDrawable;

    if (pPriv->target_sbc != -1) /* already in use */
        return BadDrawable;

    /* target_sbc == 0 means to block until all pending swaps are
     * finished. Recalculate target_sbc to get that behaviour.
     */
    if (target_sbc == 0)
        target_sbc = pPriv->swap_count + pPriv->swapsPending;

    /* If current swap count already >= target_sbc, reply and
     * return immediately with (ust, msc, sbc) triplet of
     * most recent completed swap.
     */
    if (pPriv->swap_count >= target_sbc) {
        ProcDRI2WaitMSCReply(client, pPriv->last_swap_ust,
                             pPriv->last_swap_msc, pPriv->swap_count);
        return Success;
    }

    if (!dri2Sleep(client, pPriv, WAKE_SBC))
        return BadAlloc;

    pPriv->target_sbc = target_sbc;
    return Success;
}

Bool
DRI2HasSwapControl(ScreenPtr pScreen)
{
    DRI2ScreenPtr ds = DRI2GetScreen(pScreen);

    return ds->ScheduleSwap && ds->GetMSC;
}

Bool
DRI2Connect(ClientPtr client, ScreenPtr pScreen,
            unsigned int driverType, int *fd,
            const char **driverName, const char **deviceName)
{
    DRI2ScreenPtr ds;
    uint32_t prime_id = DRI2DriverPrimeId(driverType);
    uint32_t driver_id = driverType & 0xffff;

    if (!dixPrivateKeyRegistered(dri2ScreenPrivateKey))
        return FALSE;

    ds = DRI2GetScreenPrime(pScreen, prime_id);
    if (ds == NULL)
        return FALSE;

    if (driver_id >= ds->numDrivers ||
        !ds->driverNames[driver_id])
        return FALSE;

    *driverName = ds->driverNames[driver_id];
    *deviceName = ds->deviceName;
    *fd = ds->fd;

    if (client) {
        DRI2ClientPtr dri2_client;
        dri2_client = dri2ClientPrivate(client);
        dri2_client->prime_id = prime_id;
    }

    return TRUE;
}

static int
DRI2AuthMagic (ScreenPtr pScreen, uint32_t magic)
{
    DRI2ScreenPtr ds = DRI2GetScreen(pScreen);
    if (ds == NULL)
        return -EINVAL;

    return (*ds->LegacyAuthMagic) (ds->fd, magic);
}

Bool
DRI2Authenticate(ClientPtr client, ScreenPtr pScreen, uint32_t magic)
{
    DRI2ScreenPtr ds;
    DRI2ClientPtr dri2_client = dri2ClientPrivate(client);
    ScreenPtr primescreen;

    ds = DRI2GetScreenPrime(pScreen, dri2_client->prime_id);
    if (ds == NULL)
        return FALSE;

    primescreen = GetScreenPrime(pScreen, dri2_client->prime_id);
    if ((*ds->AuthMagic)(primescreen, magic))
        return FALSE;
    return TRUE;
}

static int
DRI2ConfigNotify(WindowPtr pWin, int x, int y, int w, int h, int bw,
                 WindowPtr pSib)
{
    DrawablePtr pDraw = (DrawablePtr) pWin;
    ScreenPtr pScreen = pDraw->pScreen;
    DRI2ScreenPtr ds = DRI2GetScreen(pScreen);
    DRI2DrawablePtr dd = DRI2GetDrawable(pDraw);
    int ret;

    if (ds->ConfigNotify) {
        pScreen->ConfigNotify = ds->ConfigNotify;

        ret = (*pScreen->ConfigNotify) (pWin, x, y, w, h, bw, pSib);

        ds->ConfigNotify = pScreen->ConfigNotify;
        pScreen->ConfigNotify = DRI2ConfigNotify;
        if (ret)
            return ret;
    }

    if (!dd || (dd->width == w && dd->height == h))
        return Success;

    DRI2InvalidateDrawable(pDraw);
    return Success;
}

static void
DRI2SetWindowPixmap(WindowPtr pWin, PixmapPtr pPix)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRI2ScreenPtr ds = DRI2GetScreen(pScreen);

    pScreen->SetWindowPixmap = ds->SetWindowPixmap;
    (*pScreen->SetWindowPixmap) (pWin, pPix);
    ds->SetWindowPixmap = pScreen->SetWindowPixmap;
    pScreen->SetWindowPixmap = DRI2SetWindowPixmap;

    DRI2InvalidateDrawable(&pWin->drawable);
}

#define MAX_PRIME DRI2DriverPrimeMask
static int
get_prime_id(void)
{
    int i;
    /* start at 1, prime id 0 is just normal driver */
    for (i = 1; i < MAX_PRIME; i++) {
         if (prime_id_allocate_bitmask & (1 << i))
             continue;

         prime_id_allocate_bitmask |= (1 << i);
         return i;
    }
    return -1;
}

#include "pci_ids/pci_id_driver_map.h"

static char *
dri2_probe_driver_name(ScreenPtr pScreen, DRI2InfoPtr info)
{
#ifdef WITH_LIBDRM
    int i, j;
    char *driver = NULL;
    drmDevicePtr dev;

    /* For non-PCI devices and drmGetDevice fail, just assume that
     * the 3D driver is named the same as the kernel driver. This is
     * currently true for vc4 and msm (freedreno).
     */
    if (drmGetDevice(info->fd, &dev) || dev->bustype != DRM_BUS_PCI) {
        drmVersionPtr version = drmGetVersion(info->fd);

        if (!version) {
            xf86DrvMsg(pScreen->myNum, X_ERROR,
                       "[DRI2] Couldn't drmGetVersion() on non-PCI device, "
                       "no driver name found.\n");
            return NULL;
        }

        driver = strndup(version->name, version->name_len);
        drmFreeVersion(version);
        return driver;
    }

    for (i = 0; driver_map[i].driver; i++) {
        if (dev->deviceinfo.pci->vendor_id != driver_map[i].vendor_id)
            continue;

        if (driver_map[i].num_chips_ids == -1) {
             driver = strdup(driver_map[i].driver);
             goto out;
        }

        for (j = 0; j < driver_map[i].num_chips_ids; j++) {
            if (driver_map[i].chip_ids[j] == dev->deviceinfo.pci->device_id) {
                driver = strdup(driver_map[i].driver);
                goto out;
            }
        }
    }

    xf86DrvMsg(pScreen->myNum, X_ERROR,
               "[DRI2] No driver mapping found for PCI device "
               "0x%04x / 0x%04x\n",
               dev->deviceinfo.pci->vendor_id, dev->deviceinfo.pci->device_id);
out:
    drmFreeDevice(&dev);
    return driver;
#else
    return NULL;
#endif
}

Bool
DRI2ScreenInit(ScreenPtr pScreen, DRI2InfoPtr info)
{
    DRI2ScreenPtr ds;

    const char *driverTypeNames[] = {
        "DRI",                  /* DRI2DriverDRI */
        "VDPAU",                /* DRI2DriverVDPAU */
    };
    unsigned int i;
    CARD8 cur_minor;

    if (info->version < 3)
        return FALSE;

    if (!dixRegisterPrivateKey(&dri2ScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    if (!dixRegisterPrivateKey(&dri2WindowPrivateKeyRec, PRIVATE_WINDOW, 0))
        return FALSE;

    if (!dixRegisterPrivateKey(&dri2PixmapPrivateKeyRec, PRIVATE_PIXMAP, 0))
        return FALSE;

    if (!dixRegisterPrivateKey(&dri2ClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(DRI2ClientRec)))
        return FALSE;

    ds = calloc(1, sizeof *ds);
    if (!ds)
        return FALSE;

    ds->screen = pScreen;
    ds->fd = info->fd;
    ds->deviceName = info->deviceName;
    dri2_major = 1;

    ds->CreateBuffer = info->CreateBuffer;
    ds->DestroyBuffer = info->DestroyBuffer;
    ds->CopyRegion = info->CopyRegion;
    cur_minor = 1;

    if (info->version >= 4) {
        ds->ScheduleSwap = info->ScheduleSwap;
        ds->ScheduleWaitMSC = info->ScheduleWaitMSC;
        ds->GetMSC = info->GetMSC;
        cur_minor = 3;
    }

    if (info->version >= 5) {
        ds->LegacyAuthMagic = info->AuthMagic;
    }

    if (info->version >= 6) {
        ds->ReuseBufferNotify = info->ReuseBufferNotify;
        ds->SwapLimitValidate = info->SwapLimitValidate;
    }

    if (info->version >= 7) {
        ds->GetParam = info->GetParam;
        cur_minor = 4;
    }

    if (info->version >= 8) {
        ds->AuthMagic = info->AuthMagic2;
    }

    if (info->version >= 9) {
        ds->CreateBuffer2 = info->CreateBuffer2;
        if (info->CreateBuffer2 && pScreen->isGPU) {
            ds->prime_id = get_prime_id();
            if (ds->prime_id == -1) {
                free(ds);
                return FALSE;
            }
        }
        ds->DestroyBuffer2 = info->DestroyBuffer2;
        ds->CopyRegion2 = info->CopyRegion2;
    }

    /*
     * if the driver doesn't provide an AuthMagic function or the info struct
     * version is too low, call through LegacyAuthMagic
     */
    if (!ds->AuthMagic) {
        ds->AuthMagic = DRI2AuthMagic;
        /*
         * If the driver doesn't provide an AuthMagic function
         * it relies on the old method (using libdrm) or fails
         */
        if (!ds->LegacyAuthMagic)
#ifdef WITH_LIBDRM
            ds->LegacyAuthMagic = drmAuthMagic;
#else
            goto err_out;
#endif
    }

    /* Initialize minor if needed and set to minimum provied by DDX */
    if (!dri2_minor || dri2_minor > cur_minor)
        dri2_minor = cur_minor;

    if (info->version == 3 || info->numDrivers == 0) {
        /* Driver too old: use the old-style driverName field */
        ds->numDrivers = info->driverName ? 1 : 2;
        ds->driverNames = xallocarray(ds->numDrivers, sizeof(*ds->driverNames));
        if (!ds->driverNames)
            goto err_out;

        if (info->driverName) {
            ds->driverNames[0] = info->driverName;
        } else {
            /* FIXME dri2_probe_driver_name() returns a strdup-ed string,
             * currently this gets leaked */
            ds->driverNames[0] = ds->driverNames[1] = dri2_probe_driver_name(pScreen, info);
            if (!ds->driverNames[0])
                return FALSE;

            /* There is no VDPAU driver for i965, fallback to the generic
             * OpenGL/VAAPI va_gl backend to emulate VDPAU on i965. */
            if (strcmp(ds->driverNames[0], "i965") == 0)
                ds->driverNames[1] = "va_gl";
        }
    }
    else {
        ds->numDrivers = info->numDrivers;
        ds->driverNames = xallocarray(info->numDrivers, sizeof(*ds->driverNames));
        if (!ds->driverNames)
            goto err_out;
        memcpy(ds->driverNames, info->driverNames,
               info->numDrivers * sizeof(*ds->driverNames));
    }

    dixSetPrivate(&pScreen->devPrivates, dri2ScreenPrivateKey, ds);

    ds->ConfigNotify = pScreen->ConfigNotify;
    pScreen->ConfigNotify = DRI2ConfigNotify;

    ds->SetWindowPixmap = pScreen->SetWindowPixmap;
    pScreen->SetWindowPixmap = DRI2SetWindowPixmap;

    xf86DrvMsg(pScreen->myNum, X_INFO, "[DRI2] Setup complete\n");
    for (i = 0; i < ARRAY_SIZE(driverTypeNames); i++) {
        if (i < ds->numDrivers && ds->driverNames[i]) {
            xf86DrvMsg(pScreen->myNum, X_INFO, "[DRI2]   %s driver: %s\n",
                       driverTypeNames[i], ds->driverNames[i]);
        }
    }

    return TRUE;

 err_out:
    xf86DrvMsg(pScreen->myNum, X_WARNING,
               "[DRI2] Initialization failed for info version %d.\n",
               info->version);
    free(ds);
    return FALSE;
}

void
DRI2CloseScreen(ScreenPtr pScreen)
{
    DRI2ScreenPtr ds = DRI2GetScreen(pScreen);

    pScreen->ConfigNotify = ds->ConfigNotify;
    pScreen->SetWindowPixmap = ds->SetWindowPixmap;

    if (ds->prime_id)
        prime_id_allocate_bitmask &= ~(1 << ds->prime_id);
    free(ds->driverNames);
    free(ds);
    dixSetPrivate(&pScreen->devPrivates, dri2ScreenPrivateKey, NULL);
}

/* Called by InitExtensions() */
Bool
DRI2ModuleSetup(void)
{
    dri2DrawableRes = CreateNewResourceType(DRI2DrawableGone, "DRI2Drawable");
    if (!dri2DrawableRes)
        return FALSE;

    return TRUE;
}

void
DRI2Version(int *major, int *minor)
{
    if (major != NULL)
        *major = 1;

    if (minor != NULL)
        *minor = 2;
}

int
DRI2GetParam(ClientPtr client,
             DrawablePtr drawable,
             CARD64 param,
             BOOL *is_param_recognized,
             CARD64 *value)
{
    DRI2ScreenPtr ds = DRI2GetScreen(drawable->pScreen);
    char high_byte = (param >> 24);

    switch (high_byte) {
    case 0:
        /* Parameter names whose high_byte is 0 are reserved for the X
         * server. The server currently recognizes no parameters.
         */
        goto not_recognized;
    case 1:
        /* Parameter names whose high byte is 1 are reserved for the DDX. */
        if (ds->GetParam)
            return ds->GetParam(client, drawable, param,
                                is_param_recognized, value);
        else
            goto not_recognized;
    default:
        /* Other parameter names are reserved for future use. They are never
         * recognized.
         */
        goto not_recognized;
    }

not_recognized:
    *is_param_recognized = FALSE;
    return Success;
}
