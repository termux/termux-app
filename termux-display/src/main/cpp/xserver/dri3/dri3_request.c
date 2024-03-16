/*
 * Copyright © 2013 Keith Packard
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

#include "dri3_priv.h"
#include <syncsrv.h>
#include <unistd.h>
#include <xace.h>
#include "../Xext/syncsdk.h"
#include <protocol-versions.h>
#include <drm_fourcc.h>

static Bool
dri3_screen_can_one_point_two(ScreenPtr screen)
{
    dri3_screen_priv_ptr dri3 = dri3_screen_priv(screen);

    if (dri3 && dri3->info && dri3->info->version >= 2 &&
        dri3->info->pixmap_from_fds && dri3->info->fds_from_pixmap &&
        dri3->info->get_formats && dri3->info->get_modifiers &&
        dri3->info->get_drawable_modifiers)
        return TRUE;

    return FALSE;
}

static int
proc_dri3_query_version(ClientPtr client)
{
    REQUEST(xDRI3QueryVersionReq);
    xDRI3QueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_DRI3_MAJOR_VERSION,
        .minorVersion = SERVER_DRI3_MINOR_VERSION
    };

    REQUEST_SIZE_MATCH(xDRI3QueryVersionReq);

    for (int i = 0; i < screenInfo.numScreens; i++) {
        if (!dri3_screen_can_one_point_two(screenInfo.screens[i])) {
            rep.minorVersion = 0;
            break;
        }
    }

    for (int i = 0; i < screenInfo.numGPUScreens; i++) {
        if (!dri3_screen_can_one_point_two(screenInfo.gpuscreens[i])) {
            rep.minorVersion = 0;
            break;
        }
    }

    /* From DRI3 proto:
     *
     * The client sends the highest supported version to the server
     * and the server sends the highest version it supports, but no
     * higher than the requested version.
     */

    if (rep.majorVersion > stuff->majorVersion ||
        (rep.majorVersion == stuff->majorVersion &&
         rep.minorVersion > stuff->minorVersion)) {
        rep.majorVersion = stuff->majorVersion;
        rep.minorVersion = stuff->minorVersion;
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.majorVersion);
        swapl(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(rep), &rep);
    return Success;
}

int
dri3_send_open_reply(ClientPtr client, int fd)
{
    xDRI3OpenReply rep = {
        .type = X_Reply,
        .nfd = 1,
        .sequenceNumber = client->sequence,
        .length = 0,
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }

    if (WriteFdToClient(client, fd, TRUE) < 0) {
        close(fd);
        return BadAlloc;
    }

    WriteToClient(client, sizeof (rep), &rep);

    return Success;
}

static int
proc_dri3_open(ClientPtr client)
{
    REQUEST(xDRI3OpenReq);
    RRProviderPtr provider;
    DrawablePtr drawable;
    ScreenPtr screen;
    int fd;
    int status;

    REQUEST_SIZE_MATCH(xDRI3OpenReq);

    status = dixLookupDrawable(&drawable, stuff->drawable, client, 0, DixGetAttrAccess);
    if (status != Success)
        return status;

    if (stuff->provider == None)
        provider = NULL;
    else if (!RRProviderType) {
        return BadMatch;
    } else {
        VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);
        if (drawable->pScreen != provider->pScreen)
            return BadMatch;
    }
    screen = drawable->pScreen;

    status = dri3_open(client, screen, provider, &fd);
    if (status != Success)
        return status;

    if (client->ignoreCount == 0)
        return dri3_send_open_reply(client, fd);

    return Success;
}

static int
proc_dri3_pixmap_from_buffer(ClientPtr client)
{
    REQUEST(xDRI3PixmapFromBufferReq);
    int fd;
    DrawablePtr drawable;
    PixmapPtr pixmap;
    CARD32 stride, offset;
    int rc;

    SetReqFds(client, 1);
    REQUEST_SIZE_MATCH(xDRI3PixmapFromBufferReq);
    LEGAL_NEW_RESOURCE(stuff->pixmap, client);
    rc = dixLookupDrawable(&drawable, stuff->drawable, client, M_ANY, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->drawable;
        return rc;
    }

    if (!stuff->width || !stuff->height) {
        client->errorValue = 0;
        return BadValue;
    }

    if (stuff->width > 32767 || stuff->height > 32767)
        return BadAlloc;

    if (stuff->depth != 1) {
        DepthPtr depth = drawable->pScreen->allowedDepths;
        int i;
        for (i = 0; i < drawable->pScreen->numDepths; i++, depth++)
            if (depth->depth == stuff->depth)
                break;
        if (i == drawable->pScreen->numDepths) {
            client->errorValue = stuff->depth;
            return BadValue;
        }
    }

    fd = ReadFdFromClient(client);
    if (fd < 0)
        return BadValue;

    offset = 0;
    stride = stuff->stride;
    rc = dri3_pixmap_from_fds(&pixmap,
                              drawable->pScreen, 1, &fd,
                              stuff->width, stuff->height,
                              &stride, &offset,
                              stuff->depth, stuff->bpp,
                              DRM_FORMAT_MOD_INVALID);
    close (fd);
    if (rc != Success)
        return rc;

    pixmap->drawable.id = stuff->pixmap;

    /* security creation/labeling check */
    rc = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pixmap, RT_PIXMAP,
                  pixmap, RT_NONE, NULL, DixCreateAccess);

    if (rc != Success) {
        (*drawable->pScreen->DestroyPixmap) (pixmap);
        return rc;
    }
    if (!AddResource(stuff->pixmap, RT_PIXMAP, (void *) pixmap))
        return BadAlloc;

    return Success;
}

static int
proc_dri3_buffer_from_pixmap(ClientPtr client)
{
    REQUEST(xDRI3BufferFromPixmapReq);
    xDRI3BufferFromPixmapReply rep = {
        .type = X_Reply,
        .nfd = 1,
        .sequenceNumber = client->sequence,
        .length = 0,
    };
    int rc;
    int fd;
    PixmapPtr pixmap;

    REQUEST_SIZE_MATCH(xDRI3BufferFromPixmapReq);
    rc = dixLookupResourceByType((void **) &pixmap, stuff->pixmap, RT_PIXMAP,
                                 client, DixWriteAccess);
    if (rc != Success) {
        client->errorValue = stuff->pixmap;
        return rc;
    }

    rep.width = pixmap->drawable.width;
    rep.height = pixmap->drawable.height;
    rep.depth = pixmap->drawable.depth;
    rep.bpp = pixmap->drawable.bitsPerPixel;

    fd = dri3_fd_from_pixmap(pixmap, &rep.stride, &rep.size);
    if (fd < 0)
        return BadPixmap;

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.size);
        swaps(&rep.width);
        swaps(&rep.height);
        swaps(&rep.stride);
    }
    if (WriteFdToClient(client, fd, TRUE) < 0) {
        close(fd);
        return BadAlloc;
    }

    WriteToClient(client, sizeof(rep), &rep);

    return Success;
}

static int
proc_dri3_fence_from_fd(ClientPtr client)
{
    REQUEST(xDRI3FenceFromFDReq);
    DrawablePtr drawable;
    int fd;
    int status;

    SetReqFds(client, 1);
    REQUEST_SIZE_MATCH(xDRI3FenceFromFDReq);
    LEGAL_NEW_RESOURCE(stuff->fence, client);

    status = dixLookupDrawable(&drawable, stuff->drawable, client, M_ANY, DixGetAttrAccess);
    if (status != Success)
        return status;

    fd = ReadFdFromClient(client);
    if (fd < 0)
        return BadValue;

    status = SyncCreateFenceFromFD(client, drawable, stuff->fence,
                                   fd, stuff->initially_triggered);

    return status;
}

static int
proc_dri3_fd_from_fence(ClientPtr client)
{
    REQUEST(xDRI3FDFromFenceReq);
    xDRI3FDFromFenceReply rep = {
        .type = X_Reply,
        .nfd = 1,
        .sequenceNumber = client->sequence,
        .length = 0,
    };
    DrawablePtr drawable;
    int fd;
    int status;
    SyncFence *fence;

    REQUEST_SIZE_MATCH(xDRI3FDFromFenceReq);

    status = dixLookupDrawable(&drawable, stuff->drawable, client, M_ANY, DixGetAttrAccess);
    if (status != Success)
        return status;
    status = SyncVerifyFence(&fence, stuff->fence, client, DixWriteAccess);
    if (status != Success)
        return status;

    fd = SyncFDFromFence(client, drawable, fence);
    if (fd < 0)
        return BadMatch;

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }
    if (WriteFdToClient(client, fd, FALSE) < 0)
        return BadAlloc;

    WriteToClient(client, sizeof(rep), &rep);

    return Success;
}

static int
proc_dri3_get_supported_modifiers(ClientPtr client)
{
    REQUEST(xDRI3GetSupportedModifiersReq);
    xDRI3GetSupportedModifiersReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
    };
    WindowPtr window;
    ScreenPtr pScreen;
    CARD64 *window_modifiers = NULL;
    CARD64 *screen_modifiers = NULL;
    CARD32 nwindowmodifiers = 0;
    CARD32 nscreenmodifiers = 0;
    int status;
    int i;

    REQUEST_SIZE_MATCH(xDRI3GetSupportedModifiersReq);

    status = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (status != Success)
        return status;
    pScreen = window->drawable.pScreen;

    dri3_get_supported_modifiers(pScreen, &window->drawable,
				 stuff->depth, stuff->bpp,
                                 &nwindowmodifiers, &window_modifiers,
                                 &nscreenmodifiers, &screen_modifiers);

    rep.numWindowModifiers = nwindowmodifiers;
    rep.numScreenModifiers = nscreenmodifiers;
    rep.length = bytes_to_int32(rep.numWindowModifiers * sizeof(CARD64)) +
                 bytes_to_int32(rep.numScreenModifiers * sizeof(CARD64));

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.numWindowModifiers);
        swapl(&rep.numScreenModifiers);
        for (i = 0; i < nwindowmodifiers; i++)
            swapll(&window_modifiers[i]);
        for (i = 0; i < nscreenmodifiers; i++)
            swapll(&screen_modifiers[i]);
    }

    WriteToClient(client, sizeof(rep), &rep);
    WriteToClient(client, nwindowmodifiers * sizeof(CARD64), window_modifiers);
    WriteToClient(client, nscreenmodifiers * sizeof(CARD64), screen_modifiers);

    free(window_modifiers);
    free(screen_modifiers);

    return Success;
}

static int
proc_dri3_pixmap_from_buffers(ClientPtr client)
{
    REQUEST(xDRI3PixmapFromBuffersReq);
    int fds[4];
    CARD32 strides[4], offsets[4];
    ScreenPtr screen;
    WindowPtr window;
    PixmapPtr pixmap;
    int rc;
    int i;

    SetReqFds(client, stuff->num_buffers);
    REQUEST_SIZE_MATCH(xDRI3PixmapFromBuffersReq);
    LEGAL_NEW_RESOURCE(stuff->pixmap, client);
    rc = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->window;
        return rc;
    }
    screen = window->drawable.pScreen;

    if (!stuff->width || !stuff->height || !stuff->bpp || !stuff->depth) {
        client->errorValue = 0;
        return BadValue;
    }

    if (stuff->width > 32767 || stuff->height > 32767)
        return BadAlloc;

    if (stuff->depth != 1) {
        DepthPtr depth = screen->allowedDepths;
        int j;
        for (j = 0; j < screen->numDepths; j++, depth++)
            if (depth->depth == stuff->depth)
                break;
        if (j == screen->numDepths) {
            client->errorValue = stuff->depth;
            return BadValue;
        }
    }

    if (!stuff->num_buffers || stuff->num_buffers > 4) {
        client->errorValue = stuff->num_buffers;
        return BadValue;
    }

    for (i = 0; i < stuff->num_buffers; i++) {
        fds[i] = ReadFdFromClient(client);
        if (fds[i] < 0) {
            while (--i >= 0)
                close(fds[i]);
            return BadValue;
        }
    }

    strides[0] = stuff->stride0;
    strides[1] = stuff->stride1;
    strides[2] = stuff->stride2;
    strides[3] = stuff->stride3;
    offsets[0] = stuff->offset0;
    offsets[1] = stuff->offset1;
    offsets[2] = stuff->offset2;
    offsets[3] = stuff->offset3;

    rc = dri3_pixmap_from_fds(&pixmap, screen,
                              stuff->num_buffers, fds,
                              stuff->width, stuff->height,
                              strides, offsets,
                              stuff->depth, stuff->bpp,
                              stuff->modifier);

    for (i = 0; i < stuff->num_buffers; i++)
        close (fds[i]);

    if (rc != Success)
        return rc;

    pixmap->drawable.id = stuff->pixmap;

    /* security creation/labeling check */
    rc = XaceHook(XACE_RESOURCE_ACCESS, client, stuff->pixmap, RT_PIXMAP,
                  pixmap, RT_NONE, NULL, DixCreateAccess);

    if (rc != Success) {
        (*screen->DestroyPixmap) (pixmap);
        return rc;
    }
    if (!AddResource(stuff->pixmap, RT_PIXMAP, (void *) pixmap))
        return BadAlloc;

    return Success;
}

static int
proc_dri3_buffers_from_pixmap(ClientPtr client)
{
    REQUEST(xDRI3BuffersFromPixmapReq);
    xDRI3BuffersFromPixmapReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
    };
    int rc;
    int fds[4];
    int num_fds;
    uint32_t strides[4], offsets[4];
    uint64_t modifier;
    int i;
    PixmapPtr pixmap;

    REQUEST_SIZE_MATCH(xDRI3BuffersFromPixmapReq);
    rc = dixLookupResourceByType((void **) &pixmap, stuff->pixmap, RT_PIXMAP,
                                 client, DixWriteAccess);
    if (rc != Success) {
        client->errorValue = stuff->pixmap;
        return rc;
    }

    num_fds = dri3_fds_from_pixmap(pixmap, fds, strides, offsets, &modifier);
    if (num_fds == 0)
        return BadPixmap;

    rep.nfd = num_fds;
    rep.length = bytes_to_int32(num_fds * 2 * sizeof(CARD32));
    rep.width = pixmap->drawable.width;
    rep.height = pixmap->drawable.height;
    rep.depth = pixmap->drawable.depth;
    rep.bpp = pixmap->drawable.bitsPerPixel;
    rep.modifier = modifier;

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.width);
        swaps(&rep.height);
        swapll(&rep.modifier);
        for (i = 0; i < num_fds; i++) {
            swapl(&strides[i]);
            swapl(&offsets[i]);
        }
    }

    for (i = 0; i < num_fds; i++) {
        if (WriteFdToClient(client, fds[i], TRUE) < 0) {
            while (i--)
                close(fds[i]);
            return BadAlloc;
        }
    }

    WriteToClient(client, sizeof(rep), &rep);
    WriteToClient(client, num_fds * sizeof(CARD32), strides);
    WriteToClient(client, num_fds * sizeof(CARD32), offsets);

    return Success;
}

int (*proc_dri3_vector[DRI3NumberRequests]) (ClientPtr) = {
    proc_dri3_query_version,            /* 0 */
    proc_dri3_open,                     /* 1 */
    proc_dri3_pixmap_from_buffer,       /* 2 */
    proc_dri3_buffer_from_pixmap,       /* 3 */
    proc_dri3_fence_from_fd,            /* 4 */
    proc_dri3_fd_from_fence,            /* 5 */
    proc_dri3_get_supported_modifiers,  /* 6 */
    proc_dri3_pixmap_from_buffers,      /* 7 */
    proc_dri3_buffers_from_pixmap,      /* 8 */
};

int
proc_dri3_dispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (!client->local)
        return BadMatch;
    if (stuff->data >= DRI3NumberRequests || !proc_dri3_vector[stuff->data])
        return BadRequest;
    return (*proc_dri3_vector[stuff->data]) (client);
}

static int _X_COLD
sproc_dri3_query_version(ClientPtr client)
{
    REQUEST(xDRI3QueryVersionReq);
    REQUEST_SIZE_MATCH(xDRI3QueryVersionReq);

    swaps(&stuff->length);
    swapl(&stuff->majorVersion);
    swapl(&stuff->minorVersion);
    return (*proc_dri3_vector[stuff->dri3ReqType]) (client);
}

static int _X_COLD
sproc_dri3_open(ClientPtr client)
{
    REQUEST(xDRI3OpenReq);
    REQUEST_SIZE_MATCH(xDRI3OpenReq);

    swaps(&stuff->length);
    swapl(&stuff->drawable);
    swapl(&stuff->provider);
    return (*proc_dri3_vector[stuff->dri3ReqType]) (client);
}

static int _X_COLD
sproc_dri3_pixmap_from_buffer(ClientPtr client)
{
    REQUEST(xDRI3PixmapFromBufferReq);
    REQUEST_SIZE_MATCH(xDRI3PixmapFromBufferReq);

    swaps(&stuff->length);
    swapl(&stuff->pixmap);
    swapl(&stuff->drawable);
    swapl(&stuff->size);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swaps(&stuff->stride);
    return (*proc_dri3_vector[stuff->dri3ReqType]) (client);
}

static int _X_COLD
sproc_dri3_buffer_from_pixmap(ClientPtr client)
{
    REQUEST(xDRI3BufferFromPixmapReq);
    REQUEST_SIZE_MATCH(xDRI3BufferFromPixmapReq);

    swaps(&stuff->length);
    swapl(&stuff->pixmap);
    return (*proc_dri3_vector[stuff->dri3ReqType]) (client);
}

static int _X_COLD
sproc_dri3_fence_from_fd(ClientPtr client)
{
    REQUEST(xDRI3FenceFromFDReq);
    REQUEST_SIZE_MATCH(xDRI3FenceFromFDReq);

    swaps(&stuff->length);
    swapl(&stuff->drawable);
    swapl(&stuff->fence);
    return (*proc_dri3_vector[stuff->dri3ReqType]) (client);
}

static int _X_COLD
sproc_dri3_fd_from_fence(ClientPtr client)
{
    REQUEST(xDRI3FDFromFenceReq);
    REQUEST_SIZE_MATCH(xDRI3FDFromFenceReq);

    swaps(&stuff->length);
    swapl(&stuff->drawable);
    swapl(&stuff->fence);
    return (*proc_dri3_vector[stuff->dri3ReqType]) (client);
}

static int _X_COLD
sproc_dri3_get_supported_modifiers(ClientPtr client)
{
    REQUEST(xDRI3GetSupportedModifiersReq);
    REQUEST_SIZE_MATCH(xDRI3GetSupportedModifiersReq);

    swaps(&stuff->length);
    swapl(&stuff->window);
    return (*proc_dri3_vector[stuff->dri3ReqType]) (client);
}

static int _X_COLD
sproc_dri3_pixmap_from_buffers(ClientPtr client)
{
    REQUEST(xDRI3PixmapFromBuffersReq);
    REQUEST_SIZE_MATCH(xDRI3PixmapFromBuffersReq);

    swaps(&stuff->length);
    swapl(&stuff->pixmap);
    swapl(&stuff->window);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swapl(&stuff->stride0);
    swapl(&stuff->offset0);
    swapl(&stuff->stride1);
    swapl(&stuff->offset1);
    swapl(&stuff->stride2);
    swapl(&stuff->offset2);
    swapl(&stuff->stride3);
    swapl(&stuff->offset3);
    swapll(&stuff->modifier);
    return (*proc_dri3_vector[stuff->dri3ReqType]) (client);
}

static int _X_COLD
sproc_dri3_buffers_from_pixmap(ClientPtr client)
{
    REQUEST(xDRI3BuffersFromPixmapReq);
    REQUEST_SIZE_MATCH(xDRI3BuffersFromPixmapReq);

    swaps(&stuff->length);
    swapl(&stuff->pixmap);
    return (*proc_dri3_vector[stuff->dri3ReqType]) (client);
}

int (*sproc_dri3_vector[DRI3NumberRequests]) (ClientPtr) = {
    sproc_dri3_query_version,           /* 0 */
    sproc_dri3_open,                    /* 1 */
    sproc_dri3_pixmap_from_buffer,      /* 2 */
    sproc_dri3_buffer_from_pixmap,      /* 3 */
    sproc_dri3_fence_from_fd,           /* 4 */
    sproc_dri3_fd_from_fence,           /* 5 */
    sproc_dri3_get_supported_modifiers, /* 6 */
    sproc_dri3_pixmap_from_buffers,     /* 7 */
    sproc_dri3_buffers_from_pixmap,     /* 8 */
};

int _X_COLD
sproc_dri3_dispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (!client->local)
        return BadMatch;
    if (stuff->data >= DRI3NumberRequests || !sproc_dri3_vector[stuff->data])
        return BadRequest;
    return (*sproc_dri3_vector[stuff->data]) (client);
}
