/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include <xkbsrv.h>
#include <X11/extensions/XI.h>

static void
XkbDDXUpdateIndicators(DeviceIntPtr dev, CARD32 new)
{
    dev->kbdfeed->ctrl.leds = new;
    (*dev->kbdfeed->CtrlProc) (dev, &dev->kbdfeed->ctrl);
    return;
}

void
XkbDDXUpdateDeviceIndicators(DeviceIntPtr dev, XkbSrvLedInfoPtr sli, CARD32 new)
{
    if (sli->fb.kf == dev->kbdfeed)
        XkbDDXUpdateIndicators(dev, new);
    else if (sli->class == KbdFeedbackClass) {
        KbdFeedbackPtr kf;

        kf = sli->fb.kf;
        if (kf && kf->CtrlProc) {
            (*kf->CtrlProc) (dev, &kf->ctrl);
        }
    }
    else if (sli->class == LedFeedbackClass) {
        LedFeedbackPtr lf;

        lf = sli->fb.lf;
        if (lf && lf->CtrlProc) {
            (*lf->CtrlProc) (dev, &lf->ctrl);
        }
    }
    return;
}
