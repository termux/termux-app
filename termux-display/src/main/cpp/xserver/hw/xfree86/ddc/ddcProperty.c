/*
 * Copyright 2006 Luc Verhaegen.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86DDC.h"
#include "xf86Priv.h"
#include <X11/Xatom.h>
#include "property.h"
#include "propertyst.h"
#include <string.h>

#define EDID1_ATOM_NAME         "XFree86_DDC_EDID1_RAWDATA"

static int
edidSize(const xf86MonPtr DDC)
{
    int ret = 128;

    if (DDC->flags & EDID_COMPLETE_RAWDATA)
       ret += DDC->no_sections * 128;

    return ret;
}

static void
setRootWindowEDID(ScreenPtr pScreen, xf86MonPtr DDC)
{
    Atom atom = MakeAtom(EDID1_ATOM_NAME, strlen(EDID1_ATOM_NAME), TRUE);

    dixChangeWindowProperty(serverClient, pScreen->root, atom, XA_INTEGER,
                            8, PropModeReplace, edidSize(DDC), DDC->rawData,
                            FALSE);
}

static void
addEDIDProp(CallbackListPtr *pcbl, void *scrn, void *screen)
{
    ScreenPtr pScreen = screen;
    ScrnInfoPtr pScrn = scrn;

    if (xf86ScreenToScrn(pScreen) == pScrn)
        setRootWindowEDID(pScreen, pScrn->monitor->DDC);
}

Bool
xf86SetDDCproperties(ScrnInfoPtr pScrn, xf86MonPtr DDC)
{
    if (!pScrn || !pScrn->monitor || !DDC)
        return FALSE;

    xf86EdidMonitorSet(pScrn->scrnIndex, pScrn->monitor, DDC);

    if (xf86Initialising)
        AddCallback(&RootWindowFinalizeCallback, addEDIDProp, pScrn);
    else
        setRootWindowEDID(pScrn->pScreen, DDC);

    return TRUE;
}
