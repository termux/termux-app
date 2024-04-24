/*
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Rich Murphey and David Dawes
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  Rich Murphey and
 * David Dawes make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * RICH MURPHEY AND DAVID DAWES DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RICH MURPHEY OR DAVID DAWES BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#if defined (SYSCONS_SUPPORT)
#include <sys/kbio.h>
#endif

#include <termios.h>

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

void
xf86OSRingBell(int loudness, int pitch, int duration)
{
#ifdef WSCONS_SUPPORT
    struct wskbd_bell_data wsb;
#endif

    if (loudness && pitch) {
#ifdef PCCONS_SUPPORT
        int data[2];
#endif

        switch (xf86Info.consType) {

#ifdef PCCONS_SUPPORT
        case PCCONS:
            data[0] = pitch;
            data[1] = (duration * loudness) / 50;
            ioctl(xf86Info.consoleFd, CONSOLE_X_BELL, data);
            break;
#endif
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
        case SYSCONS:
        case PCVT:
            ioctl(xf86Info.consoleFd, KDMKTONE,
                  ((1193190 / pitch) & 0xffff) |
                  (((unsigned long) duration * loudness / 50) << 16));
            break;
#endif
#if defined (WSCONS_SUPPORT)
        case WSCONS:
            wsb.which = WSKBD_BELL_DOALL;
            wsb.pitch = pitch;
            wsb.period = duration;
            wsb.volume = loudness;
            ioctl(xf86Info.consoleFd, WSKBDIO_COMPLEXBELL, &wsb);
            break;
#endif
        }
    }
}
