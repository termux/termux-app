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

#ifndef _PRESENT_PRIV_H_
#define _PRESENT_PRIV_H_

#include "dix-config.h"
#include <X11/X.h>
#include "scrnintstr.h"
#include "misc.h"
#include "list.h"
#include "windowstr.h"
#include "dixstruct.h"
#include "present.h"
#include <syncsdk.h>
#include <syncsrv.h>
#include <xfixes.h>
#include <randrstr.h>
#include <inttypes.h>

#if 0
#define DebugPresent(x) ErrorF x
#else
#define DebugPresent(x)
#endif

extern int present_request;

extern DevPrivateKeyRec present_screen_private_key;

typedef struct present_fence *present_fence_ptr;

typedef struct present_notify present_notify_rec, *present_notify_ptr;

struct present_notify {
    struct xorg_list    window_list;
    WindowPtr           window;
    CARD32              serial;
};

struct present_vblank {
    struct xorg_list    window_list;
    struct xorg_list    event_queue;
    ScreenPtr           screen;
    WindowPtr           window;
    PixmapPtr           pixmap;
    RegionPtr           valid;
    RegionPtr           update;
    RRCrtcPtr           crtc;
    uint32_t            serial;
    int16_t             x_off;
    int16_t             y_off;
    CARD16              kind;
    uint64_t            event_id;
    uint64_t            target_msc;     /* target MSC when present should complete */
    uint64_t            exec_msc;       /* MSC at which present can be executed */
    uint64_t            msc_offset;
    present_fence_ptr   idle_fence;
    present_fence_ptr   wait_fence;
    present_notify_ptr  notifies;
    int                 num_notifies;
    Bool                queued;         /* on present_exec_queue */
    Bool                flip;           /* planning on using flip */
    Bool                flip_ready;     /* wants to flip, but waiting for previous flip or unflip */
    Bool                sync_flip;      /* do flip synchronous to vblank */
    Bool                abort_flip;     /* aborting this flip */
    PresentFlipReason   reason;         /* reason for which flip is not possible */
    Bool                has_suboptimal; /* whether client can support SuboptimalCopy mode */
};

typedef struct present_screen_priv present_screen_priv_rec, *present_screen_priv_ptr;
typedef struct present_window_priv present_window_priv_rec, *present_window_priv_ptr;

/*
 * Mode hooks
 */
typedef uint32_t (*present_priv_query_capabilities_ptr)(present_screen_priv_ptr screen_priv);
typedef RRCrtcPtr (*present_priv_get_crtc_ptr)(present_screen_priv_ptr screen_priv,
                                               WindowPtr window);

typedef Bool (*present_priv_check_flip_ptr)(RRCrtcPtr crtc,
                                            WindowPtr window,
                                            PixmapPtr pixmap,
                                            Bool sync_flip,
                                            RegionPtr valid,
                                            int16_t x_off,
                                            int16_t y_off,
                                            PresentFlipReason *reason);
typedef void (*present_priv_check_flip_window_ptr)(WindowPtr window);
typedef Bool (*present_priv_can_window_flip_ptr)(WindowPtr window);
typedef void (*present_priv_clear_window_flip_ptr)(WindowPtr window);

typedef int (*present_priv_pixmap_ptr)(WindowPtr window,
                                       PixmapPtr pixmap,
                                       CARD32 serial,
                                       RegionPtr valid,
                                       RegionPtr update,
                                       int16_t x_off,
                                       int16_t y_off,
                                       RRCrtcPtr target_crtc,
                                       SyncFence *wait_fence,
                                       SyncFence *idle_fence,
                                       uint32_t options,
                                       uint64_t window_msc,
                                       uint64_t divisor,
                                       uint64_t remainder,
                                       present_notify_ptr notifies,
                                       int num_notifies);

typedef int (*present_priv_queue_vblank_ptr)(ScreenPtr screen,
                                             WindowPtr window,
                                             RRCrtcPtr crtc,
                                             uint64_t event_id,
                                             uint64_t msc);
typedef void (*present_priv_flush_ptr)(WindowPtr window);
typedef void (*present_priv_re_execute_ptr)(present_vblank_ptr vblank);

typedef void (*present_priv_abort_vblank_ptr)(ScreenPtr screen,
                                              WindowPtr window,
                                              RRCrtcPtr crtc,
                                              uint64_t event_id,
                                              uint64_t msc);
typedef void (*present_priv_flip_destroy_ptr)(ScreenPtr screen);

struct present_screen_priv {
    CloseScreenProcPtr          CloseScreen;
    ConfigNotifyProcPtr         ConfigNotify;
    DestroyWindowProcPtr        DestroyWindow;
    ClipNotifyProcPtr           ClipNotify;

    present_vblank_ptr          flip_pending;
    uint64_t                    unflip_event_id;

    uint32_t                    fake_interval;

    /* Currently active flipped pixmap and fence */
    RRCrtcPtr                   flip_crtc;
    WindowPtr                   flip_window;
    uint32_t                    flip_serial;
    PixmapPtr                   flip_pixmap;
    present_fence_ptr           flip_idle_fence;
    Bool                        flip_sync;

    present_screen_info_ptr     info;

    /* Mode hooks */
    present_priv_query_capabilities_ptr query_capabilities;
    present_priv_get_crtc_ptr           get_crtc;

    present_priv_check_flip_ptr         check_flip;
    present_priv_check_flip_window_ptr  check_flip_window;
    present_priv_can_window_flip_ptr    can_window_flip;
    present_priv_clear_window_flip_ptr  clear_window_flip;

    present_priv_pixmap_ptr             present_pixmap;

    present_priv_queue_vblank_ptr       queue_vblank;
    present_priv_flush_ptr              flush;
    present_priv_re_execute_ptr         re_execute;

    present_priv_abort_vblank_ptr       abort_vblank;
    present_priv_flip_destroy_ptr       flip_destroy;
};

#define wrap(priv,real,mem,func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define unwrap(priv,real,mem) {\
    real->mem = priv->mem; \
}

static inline present_screen_priv_ptr
present_screen_priv(ScreenPtr screen)
{
    return (present_screen_priv_ptr)dixLookupPrivate(&(screen)->devPrivates, &present_screen_private_key);
}

/*
 * Each window has a list of clients and event masks
 */
typedef struct present_event *present_event_ptr;

typedef struct present_event {
    present_event_ptr next;
    ClientPtr client;
    WindowPtr window;
    XID id;
    int mask;
} present_event_rec;

struct present_window_priv {
    WindowPtr              window;
    present_event_ptr      events;
    RRCrtcPtr              crtc;        /* Last reported CRTC from get_ust_msc */
    uint64_t               msc_offset;
    uint64_t               msc;         /* Last reported MSC from the current crtc */
    struct xorg_list       vblank;
    struct xorg_list       notifies;
};

#define PresentCrtcNeverSet     ((RRCrtcPtr) 1)

extern DevPrivateKeyRec present_window_private_key;

static inline present_window_priv_ptr
present_window_priv(WindowPtr window)
{
    return (present_window_priv_ptr)dixGetPrivate(&(window)->devPrivates, &present_window_private_key);
}

present_window_priv_ptr
present_get_window_priv(WindowPtr window, Bool create);

/*
 * Returns:
 * TRUE if the first MSC value is after the second one
 * FALSE if the first MSC value is equal to or before the second one
 */
static inline Bool
msc_is_after(uint64_t test, uint64_t reference)
{
    return (int64_t)(test - reference) > 0;
}

/*
 * present.c
 */
uint32_t
present_query_capabilities(RRCrtcPtr crtc);

RRCrtcPtr
present_get_crtc(WindowPtr window);

void
present_copy_region(DrawablePtr drawable,
                    PixmapPtr pixmap,
                    RegionPtr update,
                    int16_t x_off,
                    int16_t y_off);

void
present_pixmap_idle(PixmapPtr pixmap, WindowPtr window, CARD32 serial, struct present_fence *present_fence);

void
present_set_tree_pixmap(WindowPtr window,
                        PixmapPtr expected,
                        PixmapPtr pixmap);

uint64_t
present_get_target_msc(uint64_t target_msc_arg,
                       uint64_t crtc_msc,
                       uint64_t divisor,
                       uint64_t remainder,
                       uint32_t options);

int
present_pixmap(WindowPtr window,
               PixmapPtr pixmap,
               CARD32 serial,
               RegionPtr valid,
               RegionPtr update,
               int16_t x_off,
               int16_t y_off,
               RRCrtcPtr target_crtc,
               SyncFence *wait_fence,
               SyncFence *idle_fence,
               uint32_t options,
               uint64_t target_msc,
               uint64_t divisor,
               uint64_t remainder,
               present_notify_ptr notifies,
               int num_notifies);

int
present_notify_msc(WindowPtr window,
                   CARD32 serial,
                   uint64_t target_msc,
                   uint64_t divisor,
                   uint64_t remainder);

/*
 * present_event.c
 */

void
present_free_events(WindowPtr window);

void
present_send_config_notify(WindowPtr window, int x, int y, int w, int h, int bw, WindowPtr sibling);

void
present_send_complete_notify(WindowPtr window, CARD8 kind, CARD8 mode, CARD32 serial, uint64_t ust, uint64_t msc);

void
present_send_idle_notify(WindowPtr window, CARD32 serial, PixmapPtr pixmap, present_fence_ptr idle_fence);

int
present_select_input(ClientPtr client,
                     CARD32 eid,
                     WindowPtr window,
                     CARD32 event_mask);

Bool
present_event_init(void);

/*
 * present_execute.c
 */
Bool
present_execute_wait(present_vblank_ptr vblank, uint64_t crtc_msc);

void
present_execute_copy(present_vblank_ptr vblank, uint64_t crtc_msc);

void
present_execute_post(present_vblank_ptr vblank, uint64_t ust, uint64_t crtc_msc);

/*
 * present_fake.c
 */
int
present_fake_get_ust_msc(ScreenPtr screen, uint64_t *ust, uint64_t *msc);

int
present_fake_queue_vblank(ScreenPtr screen, uint64_t event_id, uint64_t msc);

void
present_fake_abort_vblank(ScreenPtr screen, uint64_t event_id, uint64_t msc);

void
present_fake_screen_init(ScreenPtr screen);

void
present_fake_queue_init(void);

/*
 * present_fence.c
 */
struct present_fence *
present_fence_create(SyncFence *sync_fence);

void
present_fence_destroy(struct present_fence *present_fence);

void
present_fence_set_triggered(struct present_fence *present_fence);

Bool
present_fence_check_triggered(struct present_fence *present_fence);

void
present_fence_set_callback(struct present_fence *present_fence,
                           void (*callback)(void *param),
                           void *param);

XID
present_fence_id(struct present_fence *present_fence);

/*
 * present_notify.c
 */
void
present_clear_window_notifies(WindowPtr window);

void
present_free_window_notify(present_notify_ptr notify);

int
present_add_window_notify(present_notify_ptr notify);

int
present_create_notifies(ClientPtr client, int num_notifies, xPresentNotify *x_notifies, present_notify_ptr *p_notifies);

void
present_destroy_notifies(present_notify_ptr notifies, int num_notifies);

/*
 * present_redirect.c
 */

WindowPtr
present_redirect(ClientPtr client, WindowPtr target);

/*
 * present_request.c
 */
int
proc_present_dispatch(ClientPtr client);

int
sproc_present_dispatch(ClientPtr client);

/*
 * present_scmd.c
 */
void
present_abort_vblank(ScreenPtr screen, RRCrtcPtr crtc, uint64_t event_id, uint64_t msc);

void
present_flip_destroy(ScreenPtr screen);

void
present_restore_screen_pixmap(ScreenPtr screen);

void
present_set_abort_flip(ScreenPtr screen);

Bool
present_init(void);

void
present_scmd_init_mode_hooks(present_screen_priv_ptr screen_priv);

/*
 * present_screen.c
 */
Bool
present_screen_register_priv_keys(void);

present_screen_priv_ptr
present_screen_priv_init(ScreenPtr screen);

/*
 * present_vblank.c
 */
void
present_vblank_notify(present_vblank_ptr vblank, CARD8 kind, CARD8 mode, uint64_t ust, uint64_t crtc_msc);

Bool
present_vblank_init(present_vblank_ptr vblank,
                    WindowPtr window,
                    PixmapPtr pixmap,
                    CARD32 serial,
                    RegionPtr valid,
                    RegionPtr update,
                    int16_t x_off,
                    int16_t y_off,
                    RRCrtcPtr target_crtc,
                    SyncFence *wait_fence,
                    SyncFence *idle_fence,
                    uint32_t options,
                    const uint32_t capabilities,
                    present_notify_ptr notifies,
                    int num_notifies,
                    uint64_t target_msc,
                    uint64_t crtc_msc);

present_vblank_ptr
present_vblank_create(WindowPtr window,
                      PixmapPtr pixmap,
                      CARD32 serial,
                      RegionPtr valid,
                      RegionPtr update,
                      int16_t x_off,
                      int16_t y_off,
                      RRCrtcPtr target_crtc,
                      SyncFence *wait_fence,
                      SyncFence *idle_fence,
                      uint32_t options,
                      const uint32_t capabilities,
                      present_notify_ptr notifies,
                      int num_notifies,
                      uint64_t target_msc,
                      uint64_t crtc_msc);

void
present_vblank_scrap(present_vblank_ptr vblank);

void
present_vblank_destroy(present_vblank_ptr vblank);

#endif /*  _PRESENT_PRIV_H_ */
