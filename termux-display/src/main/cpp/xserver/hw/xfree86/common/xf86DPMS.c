/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * This file contains the DPMS functions required by the extension.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "os.h"
#include "globals.h"
#include "windowstr.h"
#include "xf86.h"
#include "xf86Priv.h"
#ifdef DPMSExtension
#include <X11/extensions/dpmsconst.h>
#include "dpmsproc.h"
#endif
#ifdef XSERVER_LIBPCIACCESS
#include "xf86VGAarbiter.h"
#endif

#ifdef DPMSExtension
static void
xf86DPMS(ScreenPtr pScreen, int level)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    if (pScrn->DPMSSet && pScrn->vtSema) {
        xf86VGAarbiterLock(pScrn);
        pScrn->DPMSSet(pScrn, level, 0);
        xf86VGAarbiterUnlock(pScrn);
    }
}
#endif

Bool
xf86DPMSInit(ScreenPtr pScreen, DPMSSetProcPtr set, int flags)
{
#ifdef DPMSExtension
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    void *DPMSOpt;
    MessageType enabled_from = X_DEFAULT;
    Bool enabled = TRUE;

    DPMSOpt = xf86FindOption(pScrn->options, "dpms");
    if (DPMSDisabledSwitch) {
        enabled_from = X_CMDLINE;
        enabled = FALSE;
    }
    else if (DPMSOpt) {
        enabled_from = X_CONFIG;
        enabled = xf86CheckBoolOption(pScrn->options, "dpms", FALSE);
        xf86MarkOptionUsed(DPMSOpt);
    }
    if (enabled) {
        xf86DrvMsg(pScreen->myNum, enabled_from, "DPMS enabled\n");
        pScrn->DPMSSet = set;
        pScreen->DPMS = xf86DPMS;
    }
    return TRUE;
#else
    return FALSE;
#endif
}
