/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett Packard Company
 * Copyright © 2006 Intel Corporation
 * Copyright © 2008, 2021 Red Hat, Inc.
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
 *
 */

#ifndef _LIBXCVT_MODE_H_
#define _LIBXCVT_MODE_H_

#include <stdint.h>

/* Conveniently chosen to match the RandR definitions */
enum  libxcvt_mode_flags {
    LIBXCVT_MODE_FLAG_HSYNC_POSITIVE    = (1 << 0),
    LIBXCVT_MODE_FLAG_HSYNC_NEGATIVE    = (1 << 1),
    LIBXCVT_MODE_FLAG_VSYNC_POSITIVE    = (1 << 2),
    LIBXCVT_MODE_FLAG_VSYNC_NEGATIVE    = (1 << 3),
    LIBXCVT_MODE_FLAG_INTERLACE         = (1 << 4),
};

struct libxcvt_mode_info {
    uint32_t                hdisplay;
    uint32_t                vdisplay;
    float                   vrefresh;
    float                   hsync;
    uint64_t                dot_clock;
    uint16_t                hsync_start;
    uint16_t                hsync_end;
    uint16_t                htotal;
    uint16_t                vsync_start;
    uint16_t                vsync_end;
    uint16_t                vtotal;
    enum libxcvt_mode_flags mode_flags;
};

#endif /* _LIBXCVT_MODE_H_ */
