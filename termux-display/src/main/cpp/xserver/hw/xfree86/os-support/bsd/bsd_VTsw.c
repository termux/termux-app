/*
 * Derived from VTsw_usl.c which is
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 * by S_ren Schmidt (sos@login.dkuug.dk)
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

/*
 * Handle the VT-switching interface for OSs that use USL-style ioctl()s
 * (the bsd, sysv, sco, and linux subdirs).
 */

/*
 * This function is the signal handler for the VT-switching signal.  It
 * is only referenced inside the OS-support layer.
 */
void
xf86VTRequest(int sig)
{
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
    if (xf86Info.consType == SYSCONS || xf86Info.consType == PCVT) {
        xf86Info.vtRequestsPending = TRUE;
    }
#endif
    return;
}

Bool
xf86VTSwitchPending()
{
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
    if (xf86Info.consType == SYSCONS || xf86Info.consType == PCVT) {
        return xf86Info.vtRequestsPending ? TRUE : FALSE;
    }
#endif
    return FALSE;
}

Bool
xf86VTSwitchAway()
{
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
    if (xf86Info.consType == SYSCONS || xf86Info.consType == PCVT) {
        xf86Info.vtRequestsPending = FALSE;
        if (ioctl(xf86Info.consoleFd, VT_RELDISP, 1) < 0)
            return FALSE;
        else
            return TRUE;
    }
#endif
    return FALSE;
}

Bool
xf86VTSwitchTo()
{
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
    if (xf86Info.consType == SYSCONS || xf86Info.consType == PCVT) {
        xf86Info.vtRequestsPending = FALSE;
        if (ioctl(xf86Info.consoleFd, VT_RELDISP, VT_ACKACQ) < 0)
            return FALSE;
        else
            return TRUE;
    }
#endif
    return TRUE;
}

Bool
xf86VTActivate(int vtno)
{
    if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, vtno) < 0) {
        return FALSE;
    }
    return TRUE;
}
