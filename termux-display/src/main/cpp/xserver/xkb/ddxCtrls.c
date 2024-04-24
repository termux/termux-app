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

void
XkbDDXKeybdCtrlProc(DeviceIntPtr dev, KeybdCtrl * ctrl)
{
    int realRepeat;

    realRepeat = ctrl->autoRepeat;
    if ((dev->kbdfeed) && (XkbDDXUsesSoftRepeat(dev)))
        ctrl->autoRepeat = 0;
    if (dev->key && dev->key->xkbInfo && dev->key->xkbInfo->kbdProc)
        (*dev->key->xkbInfo->kbdProc) (dev, ctrl);
    ctrl->autoRepeat = realRepeat;
    return;
}

int
XkbDDXUsesSoftRepeat(DeviceIntPtr pXDev)
{
    return 1;
}

void
XkbDDXChangeControls(DeviceIntPtr dev, XkbControlsPtr old, XkbControlsPtr new)
{
    unsigned changed, i;
    unsigned char *rep_old, *rep_new, *rep_fb;

    changed = new->enabled_ctrls ^ old->enabled_ctrls;
    for (rep_old = old->per_key_repeat,
         rep_new = new->per_key_repeat,
         rep_fb = dev->kbdfeed->ctrl.autoRepeats,
         i = 0; i < XkbPerKeyBitArraySize; i++) {
        if (rep_old[i] != rep_new[i]) {
            rep_fb[i] = rep_new[i];
            changed &= XkbPerKeyRepeatMask;
        }
    }

    if (changed & XkbPerKeyRepeatMask) {
        if (dev->kbdfeed->CtrlProc)
            (*dev->kbdfeed->CtrlProc) (dev, &dev->kbdfeed->ctrl);
    }
    return;
}
