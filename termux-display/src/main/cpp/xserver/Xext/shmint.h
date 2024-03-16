/*
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

#ifndef _SHMINT_H_
#define _SHMINT_H_

#include <X11/extensions/shmproto.h>

#include "screenint.h"
#include "pixmap.h"
#include "gc.h"

#define XSHM_PUT_IMAGE_ARGS \
    DrawablePtr		/* dst */, \
    GCPtr		/* pGC */, \
    int			/* depth */, \
    unsigned int	/* format */, \
    int			/* w */, \
    int			/* h */, \
    int			/* sx */, \
    int			/* sy */, \
    int			/* sw */, \
    int			/* sh */, \
    int			/* dx */, \
    int			/* dy */, \
    char *                      /* data */

#define XSHM_CREATE_PIXMAP_ARGS \
    ScreenPtr	/* pScreen */, \
    int		/* width */, \
    int		/* height */, \
    int		/* depth */, \
    char *                      /* addr */

typedef struct _ShmFuncs {
    PixmapPtr (*CreatePixmap) (XSHM_CREATE_PIXMAP_ARGS);
    void (*PutImage) (XSHM_PUT_IMAGE_ARGS);
} ShmFuncs, *ShmFuncsPtr;

#if XTRANS_SEND_FDS
#define SHM_FD_PASSING  1
#endif

typedef struct _ShmDesc {
    struct _ShmDesc *next;
    int shmid;
    int refcnt;
    char *addr;
    Bool writable;
    unsigned long size;
#ifdef SHM_FD_PASSING
    Bool is_fd;
    struct busfault *busfault;
    XID resource;
#endif
} ShmDescRec, *ShmDescPtr;

#ifdef SHM_FD_PASSING
#define SHMDESC_IS_FD(shmdesc)  ((shmdesc)->is_fd)
#else
#define SHMDESC_IS_FD(shmdesc)  (0)
#endif

extern const DevPrivateKey ShmGetDevPrivateKeyRec(void);

extern _X_EXPORT void
 ShmRegisterFuncs(ScreenPtr pScreen, ShmFuncsPtr funcs);

extern _X_EXPORT void
 ShmRegisterFbFuncs(ScreenPtr pScreen);

extern _X_EXPORT RESTYPE ShmSegType;
extern _X_EXPORT int ShmCompletionCode;
extern _X_EXPORT int BadShmSegCode;

#endif                          /* _SHMINT_H_ */
