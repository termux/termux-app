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

#ifndef _DRI3_H_
#define _DRI3_H_

#ifdef DRI3

#include <X11/extensions/dri3proto.h>
#include <randrstr.h>

#define DRI3_SCREEN_INFO_VERSION        2

typedef int (*dri3_open_proc)(ScreenPtr screen,
                              RRProviderPtr provider,
                              int *fd);

typedef int (*dri3_open_client_proc)(ClientPtr client,
                                     ScreenPtr screen,
                                     RRProviderPtr provider,
                                     int *fd);

typedef PixmapPtr (*dri3_pixmap_from_fd_proc) (ScreenPtr screen,
                                               int fd,
                                               CARD16 width,
                                               CARD16 height,
                                               CARD16 stride,
                                               CARD8 depth,
                                               CARD8 bpp);

typedef PixmapPtr (*dri3_pixmap_from_fds_proc) (ScreenPtr screen,
                                                CARD8 num_fds,
                                                const int *fds,
                                                CARD16 width,
                                                CARD16 height,
                                                const CARD32 *strides,
                                                const CARD32 *offsets,
                                                CARD8 depth,
                                                CARD8 bpp,
                                                CARD64 modifier);

typedef int (*dri3_fd_from_pixmap_proc) (ScreenPtr screen,
                                         PixmapPtr pixmap,
                                         CARD16 *stride,
                                         CARD32 *size);

typedef int (*dri3_fds_from_pixmap_proc) (ScreenPtr screen,
                                          PixmapPtr pixmap,
                                          int *fds,
                                          uint32_t *strides,
                                          uint32_t *offsets,
                                          uint64_t *modifier);

typedef int (*dri3_get_formats_proc) (ScreenPtr screen,
                                      CARD32 *num_formats,
                                      CARD32 **formats);

typedef int (*dri3_get_modifiers_proc) (ScreenPtr screen,
                                        uint32_t format,
                                        uint32_t *num_modifiers,
                                        uint64_t **modifiers);

typedef int (*dri3_get_drawable_modifiers_proc) (DrawablePtr draw,
                                                 uint32_t format,
                                                 uint32_t *num_modifiers,
                                                 uint64_t **modifiers);

typedef struct dri3_screen_info {
    uint32_t                    version;

    dri3_open_proc              open;
    dri3_pixmap_from_fd_proc    pixmap_from_fd;
    dri3_fd_from_pixmap_proc    fd_from_pixmap;

    /* Version 1 */
    dri3_open_client_proc       open_client;

    /* Version 2 */
    dri3_pixmap_from_fds_proc   pixmap_from_fds;
    dri3_fds_from_pixmap_proc   fds_from_pixmap;
    dri3_get_formats_proc       get_formats;
    dri3_get_modifiers_proc     get_modifiers;
    dri3_get_drawable_modifiers_proc get_drawable_modifiers;

} dri3_screen_info_rec, *dri3_screen_info_ptr;

extern _X_EXPORT Bool
dri3_screen_init(ScreenPtr screen, const dri3_screen_info_rec *info);

extern _X_EXPORT int
dri3_send_open_reply(ClientPtr client, int fd);

extern _X_EXPORT uint32_t
drm_format_for_depth(uint32_t depth, uint32_t bpp);

#endif

#endif /* _DRI3_H_ */
