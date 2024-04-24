/*

Copyright 1995  Kaleb S. KEITHLEY

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL Kaleb S. KEITHLEY BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Kaleb S. KEITHLEY
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
from Kaleb S. KEITHLEY

*/
/* THIS IS NOT AN X CONSORTIUM STANDARD OR AN X PROJECT TEAM SPECIFICATION */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef XF86VIDMODE

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/xf86vmproto.h>
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "swaprep.h"
#include "vidmodestr.h"
#include "globals.h"
#include "protocol-versions.h"

static int VidModeErrorBase;
static int VidModeAllowNonLocal;

static DevPrivateKeyRec VidModeClientPrivateKeyRec;
#define VidModeClientPrivateKey (&VidModeClientPrivateKeyRec)

static DevPrivateKeyRec VidModePrivateKeyRec;
#define VidModePrivateKey (&VidModePrivateKeyRec)

/* This holds the client's version information */
typedef struct {
    int major;
    int minor;
} VidModePrivRec, *VidModePrivPtr;

#define VM_GETPRIV(c) ((VidModePrivPtr) \
    dixLookupPrivate(&(c)->devPrivates, VidModeClientPrivateKey))
#define VM_SETPRIV(c,p) \
    dixSetPrivate(&(c)->devPrivates, VidModeClientPrivateKey, p)

#ifdef DEBUG
#define DEBUG_P(x) DebugF(x"\n")
#else
#define DEBUG_P(x) /**/
#endif

static DisplayModePtr
VidModeCreateMode(void)
{
    DisplayModePtr mode;

    mode = malloc(sizeof(DisplayModeRec));
    if (mode != NULL) {
        mode->name = "";
        mode->VScan = 1;        /* divides refresh rate. default = 1 */
        mode->Private = NULL;
        mode->next = mode;
        mode->prev = mode;
    }
    return mode;
}

static void
VidModeCopyMode(DisplayModePtr modefrom, DisplayModePtr modeto)
{
    memcpy(modeto, modefrom, sizeof(DisplayModeRec));
}

static int
VidModeGetModeValue(DisplayModePtr mode, int valtyp)
{
    int ret = 0;

    switch (valtyp) {
    case VIDMODE_H_DISPLAY:
        ret = mode->HDisplay;
        break;
    case VIDMODE_H_SYNCSTART:
        ret = mode->HSyncStart;
        break;
    case VIDMODE_H_SYNCEND:
        ret = mode->HSyncEnd;
        break;
    case VIDMODE_H_TOTAL:
        ret = mode->HTotal;
        break;
    case VIDMODE_H_SKEW:
        ret = mode->HSkew;
        break;
    case VIDMODE_V_DISPLAY:
        ret = mode->VDisplay;
        break;
    case VIDMODE_V_SYNCSTART:
        ret = mode->VSyncStart;
        break;
    case VIDMODE_V_SYNCEND:
        ret = mode->VSyncEnd;
        break;
    case VIDMODE_V_TOTAL:
        ret = mode->VTotal;
        break;
    case VIDMODE_FLAGS:
        ret = mode->Flags;
        break;
    case VIDMODE_CLOCK:
        ret = mode->Clock;
        break;
    }
    return ret;
}

static void
VidModeSetModeValue(DisplayModePtr mode, int valtyp, int val)
{
    switch (valtyp) {
    case VIDMODE_H_DISPLAY:
        mode->HDisplay = val;
        break;
    case VIDMODE_H_SYNCSTART:
        mode->HSyncStart = val;
        break;
    case VIDMODE_H_SYNCEND:
        mode->HSyncEnd = val;
        break;
    case VIDMODE_H_TOTAL:
        mode->HTotal = val;
        break;
    case VIDMODE_H_SKEW:
        mode->HSkew = val;
        break;
    case VIDMODE_V_DISPLAY:
        mode->VDisplay = val;
        break;
    case VIDMODE_V_SYNCSTART:
        mode->VSyncStart = val;
        break;
    case VIDMODE_V_SYNCEND:
        mode->VSyncEnd = val;
        break;
    case VIDMODE_V_TOTAL:
        mode->VTotal = val;
        break;
    case VIDMODE_FLAGS:
        mode->Flags = val;
        break;
    case VIDMODE_CLOCK:
        mode->Clock = val;
        break;
    }
    return;
}

static int
ClientMajorVersion(ClientPtr client)
{
    VidModePrivPtr pPriv;

    pPriv = VM_GETPRIV(client);
    if (!pPriv)
        return 0;
    else
        return pPriv->major;
}

static int
ProcVidModeQueryVersion(ClientPtr client)
{
    xXF86VidModeQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_XF86VIDMODE_MAJOR_VERSION,
        .minorVersion = SERVER_XF86VIDMODE_MINOR_VERSION
    };

    DEBUG_P("XF86VidModeQueryVersion");

    REQUEST_SIZE_MATCH(xXF86VidModeQueryVersionReq);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xXF86VidModeQueryVersionReply), &rep);
    return Success;
}

static int
ProcVidModeGetModeLine(ClientPtr client)
{
    REQUEST(xXF86VidModeGetModeLineReq);
    xXF86VidModeGetModeLineReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence
    };
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    DisplayModePtr mode;
    int dotClock;
    int ver;

    DEBUG_P("XF86VidModeGetModeline");

    ver = ClientMajorVersion(client);
    REQUEST_SIZE_MATCH(xXF86VidModeGetModeLineReq);

    if (ver < 2) {
        rep.length = bytes_to_int32(SIZEOF(xXF86OldVidModeGetModeLineReply) -
                                    SIZEOF(xGenericReply));
    }
    else {
        rep.length = bytes_to_int32(SIZEOF(xXF86VidModeGetModeLineReply) -
                                    SIZEOF(xGenericReply));
    }

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];
    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (!pVidMode->GetCurrentModeline(pScreen, &mode, &dotClock))
        return BadValue;

    rep.dotclock = dotClock;
    rep.hdisplay = VidModeGetModeValue(mode, VIDMODE_H_DISPLAY);
    rep.hsyncstart = VidModeGetModeValue(mode, VIDMODE_H_SYNCSTART);
    rep.hsyncend = VidModeGetModeValue(mode, VIDMODE_H_SYNCEND);
    rep.htotal = VidModeGetModeValue(mode, VIDMODE_H_TOTAL);
    rep.hskew = VidModeGetModeValue(mode, VIDMODE_H_SKEW);
    rep.vdisplay = VidModeGetModeValue(mode, VIDMODE_V_DISPLAY);
    rep.vsyncstart = VidModeGetModeValue(mode, VIDMODE_V_SYNCSTART);
    rep.vsyncend = VidModeGetModeValue(mode, VIDMODE_V_SYNCEND);
    rep.vtotal = VidModeGetModeValue(mode, VIDMODE_V_TOTAL);
    rep.flags = VidModeGetModeValue(mode, VIDMODE_FLAGS);

    DebugF("GetModeLine - scrn: %d clock: %ld\n",
           stuff->screen, (unsigned long) rep.dotclock);
    DebugF("GetModeLine - hdsp: %d hbeg: %d hend: %d httl: %d\n",
           rep.hdisplay, rep.hsyncstart, rep.hsyncend, rep.htotal);
    DebugF("              vdsp: %d vbeg: %d vend: %d vttl: %d flags: %ld\n",
           rep.vdisplay, rep.vsyncstart, rep.vsyncend,
           rep.vtotal, (unsigned long) rep.flags);

    /*
     * Older servers sometimes had server privates that the VidMode
     * extension made available. So to be compatible pretend that
     * there are no server privates to pass to the client.
     */
    rep.privsize = 0;

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.dotclock);
        swaps(&rep.hdisplay);
        swaps(&rep.hsyncstart);
        swaps(&rep.hsyncend);
        swaps(&rep.htotal);
        swaps(&rep.hskew);
        swaps(&rep.vdisplay);
        swaps(&rep.vsyncstart);
        swaps(&rep.vsyncend);
        swaps(&rep.vtotal);
        swapl(&rep.flags);
        swapl(&rep.privsize);
    }
    if (ver < 2) {
        xXF86OldVidModeGetModeLineReply oldrep = {
            .type = rep.type,
            .sequenceNumber = rep.sequenceNumber,
            .length = rep.length,
            .dotclock = rep.dotclock,
            .hdisplay = rep.hdisplay,
            .hsyncstart = rep.hsyncstart,
            .hsyncend = rep.hsyncend,
            .htotal = rep.htotal,
            .vdisplay = rep.vdisplay,
            .vsyncstart = rep.vsyncstart,
            .vsyncend = rep.vsyncend,
            .vtotal = rep.vtotal,
            .flags = rep.flags,
            .privsize = rep.privsize
        };
        WriteToClient(client, sizeof(xXF86OldVidModeGetModeLineReply), &oldrep);
    }
    else {
        WriteToClient(client, sizeof(xXF86VidModeGetModeLineReply), &rep);
    }
    return Success;
}

static int
ProcVidModeGetAllModeLines(ClientPtr client)
{
    REQUEST(xXF86VidModeGetAllModeLinesReq);
    xXF86VidModeGetAllModeLinesReply rep;
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    DisplayModePtr mode;
    int modecount, dotClock;
    int ver;

    DEBUG_P("XF86VidModeGetAllModelines");

    REQUEST_SIZE_MATCH(xXF86VidModeGetAllModeLinesReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];
    ver = ClientMajorVersion(client);
    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    modecount = pVidMode->GetNumOfModes(pScreen);
    if (modecount < 1)
        return VidModeErrorBase + XF86VidModeExtensionDisabled;

    if (!pVidMode->GetFirstModeline(pScreen, &mode, &dotClock))
        return BadValue;

    rep = (xXF86VidModeGetAllModeLinesReply) {
        .type = X_Reply,
        .length = SIZEOF(xXF86VidModeGetAllModeLinesReply) -
            SIZEOF(xGenericReply),
        .sequenceNumber = client->sequence,
        .modecount = modecount
    };
    if (ver < 2)
        rep.length += modecount * sizeof(xXF86OldVidModeModeInfo);
    else
        rep.length += modecount * sizeof(xXF86VidModeModeInfo);
    rep.length >>= 2;
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.modecount);
    }
    WriteToClient(client, sizeof(xXF86VidModeGetAllModeLinesReply), &rep);

    do {
        xXF86VidModeModeInfo mdinf = {
            .dotclock = dotClock,
            .hdisplay = VidModeGetModeValue(mode, VIDMODE_H_DISPLAY),
            .hsyncstart = VidModeGetModeValue(mode, VIDMODE_H_SYNCSTART),
            .hsyncend = VidModeGetModeValue(mode, VIDMODE_H_SYNCEND),
            .htotal = VidModeGetModeValue(mode, VIDMODE_H_TOTAL),
            .hskew = VidModeGetModeValue(mode, VIDMODE_H_SKEW),
            .vdisplay = VidModeGetModeValue(mode, VIDMODE_V_DISPLAY),
            .vsyncstart = VidModeGetModeValue(mode, VIDMODE_V_SYNCSTART),
            .vsyncend = VidModeGetModeValue(mode, VIDMODE_V_SYNCEND),
            .vtotal = VidModeGetModeValue(mode, VIDMODE_V_TOTAL),
            .flags = VidModeGetModeValue(mode, VIDMODE_FLAGS),
            .privsize = 0
        };
        if (client->swapped) {
            swapl(&mdinf.dotclock);
            swaps(&mdinf.hdisplay);
            swaps(&mdinf.hsyncstart);
            swaps(&mdinf.hsyncend);
            swaps(&mdinf.htotal);
            swapl(&mdinf.hskew);
            swaps(&mdinf.vdisplay);
            swaps(&mdinf.vsyncstart);
            swaps(&mdinf.vsyncend);
            swaps(&mdinf.vtotal);
            swapl(&mdinf.flags);
            swapl(&mdinf.privsize);
        }
        if (ver < 2) {
            xXF86OldVidModeModeInfo oldmdinf = {
                .dotclock = mdinf.dotclock,
                .hdisplay = mdinf.hdisplay,
                .hsyncstart = mdinf.hsyncstart,
                .hsyncend = mdinf.hsyncend,
                .htotal = mdinf.htotal,
                .vdisplay = mdinf.vdisplay,
                .vsyncstart = mdinf.vsyncstart,
                .vsyncend = mdinf.vsyncend,
                .vtotal = mdinf.vtotal,
                .flags = mdinf.flags,
                .privsize = mdinf.privsize
            };
            WriteToClient(client, sizeof(xXF86OldVidModeModeInfo), &oldmdinf);
        }
        else {
            WriteToClient(client, sizeof(xXF86VidModeModeInfo), &mdinf);
        }

    } while (pVidMode->GetNextModeline(pScreen, &mode, &dotClock));

    return Success;
}

#define MODEMATCH(mode,stuff)	  \
     (VidModeGetModeValue(mode, VIDMODE_H_DISPLAY)  == stuff->hdisplay \
     && VidModeGetModeValue(mode, VIDMODE_H_SYNCSTART)  == stuff->hsyncstart \
     && VidModeGetModeValue(mode, VIDMODE_H_SYNCEND)  == stuff->hsyncend \
     && VidModeGetModeValue(mode, VIDMODE_H_TOTAL)  == stuff->htotal \
     && VidModeGetModeValue(mode, VIDMODE_V_DISPLAY)  == stuff->vdisplay \
     && VidModeGetModeValue(mode, VIDMODE_V_SYNCSTART)  == stuff->vsyncstart \
     && VidModeGetModeValue(mode, VIDMODE_V_SYNCEND)  == stuff->vsyncend \
     && VidModeGetModeValue(mode, VIDMODE_V_TOTAL)  == stuff->vtotal \
     && VidModeGetModeValue(mode, VIDMODE_FLAGS)  == stuff->flags )

static int
ProcVidModeAddModeLine(ClientPtr client)
{
    REQUEST(xXF86VidModeAddModeLineReq);
    xXF86OldVidModeAddModeLineReq *oldstuff =
        (xXF86OldVidModeAddModeLineReq *) client->requestBuffer;
    xXF86VidModeAddModeLineReq newstuff;
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    DisplayModePtr mode;
    int len;
    int dotClock;
    int ver;

    DEBUG_P("XF86VidModeAddModeline");

    ver = ClientMajorVersion(client);

    if (ver < 2) {
        REQUEST_AT_LEAST_SIZE(xXF86OldVidModeAddModeLineReq);
        len =
            client->req_len -
            bytes_to_int32(sizeof(xXF86OldVidModeAddModeLineReq));
    }
    else {
        REQUEST_AT_LEAST_SIZE(xXF86VidModeAddModeLineReq);
        len =
            client->req_len -
            bytes_to_int32(sizeof(xXF86VidModeAddModeLineReq));
    }

    if (ver < 2) {
        /* convert from old format */
        stuff = &newstuff;
        stuff->length = oldstuff->length;
        stuff->screen = oldstuff->screen;
        stuff->dotclock = oldstuff->dotclock;
        stuff->hdisplay = oldstuff->hdisplay;
        stuff->hsyncstart = oldstuff->hsyncstart;
        stuff->hsyncend = oldstuff->hsyncend;
        stuff->htotal = oldstuff->htotal;
        stuff->hskew = 0;
        stuff->vdisplay = oldstuff->vdisplay;
        stuff->vsyncstart = oldstuff->vsyncstart;
        stuff->vsyncend = oldstuff->vsyncend;
        stuff->vtotal = oldstuff->vtotal;
        stuff->flags = oldstuff->flags;
        stuff->privsize = oldstuff->privsize;
        stuff->after_dotclock = oldstuff->after_dotclock;
        stuff->after_hdisplay = oldstuff->after_hdisplay;
        stuff->after_hsyncstart = oldstuff->after_hsyncstart;
        stuff->after_hsyncend = oldstuff->after_hsyncend;
        stuff->after_htotal = oldstuff->after_htotal;
        stuff->after_hskew = 0;
        stuff->after_vdisplay = oldstuff->after_vdisplay;
        stuff->after_vsyncstart = oldstuff->after_vsyncstart;
        stuff->after_vsyncend = oldstuff->after_vsyncend;
        stuff->after_vtotal = oldstuff->after_vtotal;
        stuff->after_flags = oldstuff->after_flags;
    }
    DebugF("AddModeLine - scrn: %d clock: %ld\n",
           (int) stuff->screen, (unsigned long) stuff->dotclock);
    DebugF("AddModeLine - hdsp: %d hbeg: %d hend: %d httl: %d\n",
           stuff->hdisplay, stuff->hsyncstart,
           stuff->hsyncend, stuff->htotal);
    DebugF("              vdsp: %d vbeg: %d vend: %d vttl: %d flags: %ld\n",
           stuff->vdisplay, stuff->vsyncstart, stuff->vsyncend,
           stuff->vtotal, (unsigned long) stuff->flags);
    DebugF("      after - scrn: %d clock: %ld\n",
           (int) stuff->screen, (unsigned long) stuff->after_dotclock);
    DebugF("              hdsp: %d hbeg: %d hend: %d httl: %d\n",
           stuff->after_hdisplay, stuff->after_hsyncstart,
           stuff->after_hsyncend, stuff->after_htotal);
    DebugF("              vdsp: %d vbeg: %d vend: %d vttl: %d flags: %ld\n",
           stuff->after_vdisplay, stuff->after_vsyncstart,
           stuff->after_vsyncend, stuff->after_vtotal,
           (unsigned long) stuff->after_flags);

    if (len != stuff->privsize)
        return BadLength;

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    if (stuff->hsyncstart < stuff->hdisplay ||
        stuff->hsyncend < stuff->hsyncstart ||
        stuff->htotal < stuff->hsyncend ||
        stuff->vsyncstart < stuff->vdisplay ||
        stuff->vsyncend < stuff->vsyncstart || stuff->vtotal < stuff->vsyncend)
        return BadValue;

    if (stuff->after_hsyncstart < stuff->after_hdisplay ||
        stuff->after_hsyncend < stuff->after_hsyncstart ||
        stuff->after_htotal < stuff->after_hsyncend ||
        stuff->after_vsyncstart < stuff->after_vdisplay ||
        stuff->after_vsyncend < stuff->after_vsyncstart ||
        stuff->after_vtotal < stuff->after_vsyncend)
        return BadValue;

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (stuff->after_htotal != 0 || stuff->after_vtotal != 0) {
        Bool found = FALSE;

        if (pVidMode->GetFirstModeline(pScreen, &mode, &dotClock)) {
            do {
                if ((pVidMode->GetDotClock(pScreen, stuff->dotclock)
                     == dotClock) && MODEMATCH(mode, stuff)) {
                    found = TRUE;
                    break;
                }
            } while (pVidMode->GetNextModeline(pScreen, &mode, &dotClock));
        }
        if (!found)
            return BadValue;
    }

    mode = VidModeCreateMode();
    if (mode == NULL)
        return BadValue;

    VidModeSetModeValue(mode, VIDMODE_CLOCK, stuff->dotclock);
    VidModeSetModeValue(mode, VIDMODE_H_DISPLAY, stuff->hdisplay);
    VidModeSetModeValue(mode, VIDMODE_H_SYNCSTART, stuff->hsyncstart);
    VidModeSetModeValue(mode, VIDMODE_H_SYNCEND, stuff->hsyncend);
    VidModeSetModeValue(mode, VIDMODE_H_TOTAL, stuff->htotal);
    VidModeSetModeValue(mode, VIDMODE_H_SKEW, stuff->hskew);
    VidModeSetModeValue(mode, VIDMODE_V_DISPLAY, stuff->vdisplay);
    VidModeSetModeValue(mode, VIDMODE_V_SYNCSTART, stuff->vsyncstart);
    VidModeSetModeValue(mode, VIDMODE_V_SYNCEND, stuff->vsyncend);
    VidModeSetModeValue(mode, VIDMODE_V_TOTAL, stuff->vtotal);
    VidModeSetModeValue(mode, VIDMODE_FLAGS, stuff->flags);

    if (stuff->privsize)
        DebugF("AddModeLine - Privates in request have been ignored\n");

    /* Check that the mode is consistent with the monitor specs */
    switch (pVidMode->CheckModeForMonitor(pScreen, mode)) {
    case MODE_OK:
        break;
    case MODE_HSYNC:
    case MODE_H_ILLEGAL:
        free(mode);
        return VidModeErrorBase + XF86VidModeBadHTimings;
    case MODE_VSYNC:
    case MODE_V_ILLEGAL:
        free(mode);
        return VidModeErrorBase + XF86VidModeBadVTimings;
    default:
        free(mode);
        return VidModeErrorBase + XF86VidModeModeUnsuitable;
    }

    /* Check that the driver is happy with the mode */
    if (pVidMode->CheckModeForDriver(pScreen, mode) != MODE_OK) {
        free(mode);
        return VidModeErrorBase + XF86VidModeModeUnsuitable;
    }

    pVidMode->SetCrtcForMode(pScreen, mode);

    pVidMode->AddModeline(pScreen, mode);

    DebugF("AddModeLine - Succeeded\n");

    return Success;
}

static int
ProcVidModeDeleteModeLine(ClientPtr client)
{
    REQUEST(xXF86VidModeDeleteModeLineReq);
    xXF86OldVidModeDeleteModeLineReq *oldstuff =
        (xXF86OldVidModeDeleteModeLineReq *) client->requestBuffer;
    xXF86VidModeDeleteModeLineReq newstuff;
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    DisplayModePtr mode;
    int len, dotClock;
    int ver;

    DEBUG_P("XF86VidModeDeleteModeline");

    ver = ClientMajorVersion(client);

    if (ver < 2) {
        REQUEST_AT_LEAST_SIZE(xXF86OldVidModeDeleteModeLineReq);
        len =
            client->req_len -
            bytes_to_int32(sizeof(xXF86OldVidModeDeleteModeLineReq));
    }
    else {
        REQUEST_AT_LEAST_SIZE(xXF86VidModeDeleteModeLineReq);
        len =
            client->req_len -
            bytes_to_int32(sizeof(xXF86VidModeDeleteModeLineReq));
    }

    if (ver < 2) {
        /* convert from old format */
        stuff = &newstuff;
        stuff->length = oldstuff->length;
        stuff->screen = oldstuff->screen;
        stuff->dotclock = oldstuff->dotclock;
        stuff->hdisplay = oldstuff->hdisplay;
        stuff->hsyncstart = oldstuff->hsyncstart;
        stuff->hsyncend = oldstuff->hsyncend;
        stuff->htotal = oldstuff->htotal;
        stuff->hskew = 0;
        stuff->vdisplay = oldstuff->vdisplay;
        stuff->vsyncstart = oldstuff->vsyncstart;
        stuff->vsyncend = oldstuff->vsyncend;
        stuff->vtotal = oldstuff->vtotal;
        stuff->flags = oldstuff->flags;
        stuff->privsize = oldstuff->privsize;
    }
    DebugF("DeleteModeLine - scrn: %d clock: %ld\n",
           (int) stuff->screen, (unsigned long) stuff->dotclock);
    DebugF("                 hdsp: %d hbeg: %d hend: %d httl: %d\n",
           stuff->hdisplay, stuff->hsyncstart,
           stuff->hsyncend, stuff->htotal);
    DebugF("                 vdsp: %d vbeg: %d vend: %d vttl: %d flags: %ld\n",
           stuff->vdisplay, stuff->vsyncstart, stuff->vsyncend, stuff->vtotal,
           (unsigned long) stuff->flags);

    if (len != stuff->privsize) {
        DebugF("req_len = %ld, sizeof(Req) = %d, privsize = %ld, "
               "len = %d, length = %d\n",
               (unsigned long) client->req_len,
               (int) sizeof(xXF86VidModeDeleteModeLineReq) >> 2,
               (unsigned long) stuff->privsize, len, stuff->length);
        return BadLength;
    }

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (!pVidMode->GetCurrentModeline(pScreen, &mode, &dotClock))
        return BadValue;

    DebugF("Checking against clock: %d (%d)\n",
           VidModeGetModeValue(mode, VIDMODE_CLOCK), dotClock);
    DebugF("                 hdsp: %d hbeg: %d hend: %d httl: %d\n",
           VidModeGetModeValue(mode, VIDMODE_H_DISPLAY),
           VidModeGetModeValue(mode, VIDMODE_H_SYNCSTART),
           VidModeGetModeValue(mode, VIDMODE_H_SYNCEND),
           VidModeGetModeValue(mode, VIDMODE_H_TOTAL));
    DebugF("                 vdsp: %d vbeg: %d vend: %d vttl: %d flags: %d\n",
           VidModeGetModeValue(mode, VIDMODE_V_DISPLAY),
           VidModeGetModeValue(mode, VIDMODE_V_SYNCSTART),
           VidModeGetModeValue(mode, VIDMODE_V_SYNCEND),
           VidModeGetModeValue(mode, VIDMODE_V_TOTAL),
           VidModeGetModeValue(mode, VIDMODE_FLAGS));

    if ((pVidMode->GetDotClock(pScreen, stuff->dotclock) == dotClock) &&
        MODEMATCH(mode, stuff))
        return BadValue;

    if (!pVidMode->GetFirstModeline(pScreen, &mode, &dotClock))
        return BadValue;

    do {
        DebugF("Checking against clock: %d (%d)\n",
               VidModeGetModeValue(mode, VIDMODE_CLOCK), dotClock);
        DebugF("                 hdsp: %d hbeg: %d hend: %d httl: %d\n",
               VidModeGetModeValue(mode, VIDMODE_H_DISPLAY),
               VidModeGetModeValue(mode, VIDMODE_H_SYNCSTART),
               VidModeGetModeValue(mode, VIDMODE_H_SYNCEND),
               VidModeGetModeValue(mode, VIDMODE_H_TOTAL));
        DebugF("                 vdsp: %d vbeg: %d vend: %d vttl: %d flags: %d\n",
               VidModeGetModeValue(mode, VIDMODE_V_DISPLAY),
               VidModeGetModeValue(mode, VIDMODE_V_SYNCSTART),
               VidModeGetModeValue(mode, VIDMODE_V_SYNCEND),
               VidModeGetModeValue(mode, VIDMODE_V_TOTAL),
               VidModeGetModeValue(mode, VIDMODE_FLAGS));

        if ((pVidMode->GetDotClock(pScreen, stuff->dotclock) == dotClock) &&
            MODEMATCH(mode, stuff)) {
            pVidMode->DeleteModeline(pScreen, mode);
            DebugF("DeleteModeLine - Succeeded\n");
            return Success;
        }
    } while (pVidMode->GetNextModeline(pScreen, &mode, &dotClock));

    return BadValue;
}

static int
ProcVidModeModModeLine(ClientPtr client)
{
    REQUEST(xXF86VidModeModModeLineReq);
    xXF86OldVidModeModModeLineReq *oldstuff =
        (xXF86OldVidModeModModeLineReq *) client->requestBuffer;
    xXF86VidModeModModeLineReq newstuff;
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    DisplayModePtr mode, modetmp;
    int len, dotClock;
    int ver;

    DEBUG_P("XF86VidModeModModeline");

    ver = ClientMajorVersion(client);

    if (ver < 2) {
        REQUEST_AT_LEAST_SIZE(xXF86OldVidModeModModeLineReq);
        len =
            client->req_len -
            bytes_to_int32(sizeof(xXF86OldVidModeModModeLineReq));
    }
    else {
        REQUEST_AT_LEAST_SIZE(xXF86VidModeModModeLineReq);
        len =
            client->req_len -
            bytes_to_int32(sizeof(xXF86VidModeModModeLineReq));
    }

    if (ver < 2) {
        /* convert from old format */
        stuff = &newstuff;
        stuff->length = oldstuff->length;
        stuff->screen = oldstuff->screen;
        stuff->hdisplay = oldstuff->hdisplay;
        stuff->hsyncstart = oldstuff->hsyncstart;
        stuff->hsyncend = oldstuff->hsyncend;
        stuff->htotal = oldstuff->htotal;
        stuff->hskew = 0;
        stuff->vdisplay = oldstuff->vdisplay;
        stuff->vsyncstart = oldstuff->vsyncstart;
        stuff->vsyncend = oldstuff->vsyncend;
        stuff->vtotal = oldstuff->vtotal;
        stuff->flags = oldstuff->flags;
        stuff->privsize = oldstuff->privsize;
    }
    DebugF("ModModeLine - scrn: %d hdsp: %d hbeg: %d hend: %d httl: %d\n",
           (int) stuff->screen, stuff->hdisplay, stuff->hsyncstart,
           stuff->hsyncend, stuff->htotal);
    DebugF("              vdsp: %d vbeg: %d vend: %d vttl: %d flags: %ld\n",
           stuff->vdisplay, stuff->vsyncstart, stuff->vsyncend,
           stuff->vtotal, (unsigned long) stuff->flags);

    if (len != stuff->privsize)
        return BadLength;

    if (stuff->hsyncstart < stuff->hdisplay ||
        stuff->hsyncend < stuff->hsyncstart ||
        stuff->htotal < stuff->hsyncend ||
        stuff->vsyncstart < stuff->vdisplay ||
        stuff->vsyncend < stuff->vsyncstart || stuff->vtotal < stuff->vsyncend)
        return BadValue;

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (!pVidMode->GetCurrentModeline(pScreen, &mode, &dotClock))
        return BadValue;

    modetmp = VidModeCreateMode();
    VidModeCopyMode(mode, modetmp);

    VidModeSetModeValue(modetmp, VIDMODE_H_DISPLAY, stuff->hdisplay);
    VidModeSetModeValue(modetmp, VIDMODE_H_SYNCSTART, stuff->hsyncstart);
    VidModeSetModeValue(modetmp, VIDMODE_H_SYNCEND, stuff->hsyncend);
    VidModeSetModeValue(modetmp, VIDMODE_H_TOTAL, stuff->htotal);
    VidModeSetModeValue(modetmp, VIDMODE_H_SKEW, stuff->hskew);
    VidModeSetModeValue(modetmp, VIDMODE_V_DISPLAY, stuff->vdisplay);
    VidModeSetModeValue(modetmp, VIDMODE_V_SYNCSTART, stuff->vsyncstart);
    VidModeSetModeValue(modetmp, VIDMODE_V_SYNCEND, stuff->vsyncend);
    VidModeSetModeValue(modetmp, VIDMODE_V_TOTAL, stuff->vtotal);
    VidModeSetModeValue(modetmp, VIDMODE_FLAGS, stuff->flags);

    if (stuff->privsize)
        DebugF("ModModeLine - Privates in request have been ignored\n");

    /* Check that the mode is consistent with the monitor specs */
    switch (pVidMode->CheckModeForMonitor(pScreen, modetmp)) {
    case MODE_OK:
        break;
    case MODE_HSYNC:
    case MODE_H_ILLEGAL:
        free(modetmp);
        return VidModeErrorBase + XF86VidModeBadHTimings;
    case MODE_VSYNC:
    case MODE_V_ILLEGAL:
        free(modetmp);
        return VidModeErrorBase + XF86VidModeBadVTimings;
    default:
        free(modetmp);
        return VidModeErrorBase + XF86VidModeModeUnsuitable;
    }

    /* Check that the driver is happy with the mode */
    if (pVidMode->CheckModeForDriver(pScreen, modetmp) != MODE_OK) {
        free(modetmp);
        return VidModeErrorBase + XF86VidModeModeUnsuitable;
    }
    free(modetmp);

    VidModeSetModeValue(mode, VIDMODE_H_DISPLAY, stuff->hdisplay);
    VidModeSetModeValue(mode, VIDMODE_H_SYNCSTART, stuff->hsyncstart);
    VidModeSetModeValue(mode, VIDMODE_H_SYNCEND, stuff->hsyncend);
    VidModeSetModeValue(mode, VIDMODE_H_TOTAL, stuff->htotal);
    VidModeSetModeValue(mode, VIDMODE_H_SKEW, stuff->hskew);
    VidModeSetModeValue(mode, VIDMODE_V_DISPLAY, stuff->vdisplay);
    VidModeSetModeValue(mode, VIDMODE_V_SYNCSTART, stuff->vsyncstart);
    VidModeSetModeValue(mode, VIDMODE_V_SYNCEND, stuff->vsyncend);
    VidModeSetModeValue(mode, VIDMODE_V_TOTAL, stuff->vtotal);
    VidModeSetModeValue(mode, VIDMODE_FLAGS, stuff->flags);

    pVidMode->SetCrtcForMode(pScreen, mode);
    pVidMode->SwitchMode(pScreen, mode);

    DebugF("ModModeLine - Succeeded\n");
    return Success;
}

static int
ProcVidModeValidateModeLine(ClientPtr client)
{
    REQUEST(xXF86VidModeValidateModeLineReq);
    xXF86OldVidModeValidateModeLineReq *oldstuff =
        (xXF86OldVidModeValidateModeLineReq *) client->requestBuffer;
    xXF86VidModeValidateModeLineReq newstuff;
    xXF86VidModeValidateModeLineReply rep;
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    DisplayModePtr mode, modetmp = NULL;
    int len, status, dotClock;
    int ver;

    DEBUG_P("XF86VidModeValidateModeline");

    ver = ClientMajorVersion(client);

    if (ver < 2) {
        REQUEST_AT_LEAST_SIZE(xXF86OldVidModeValidateModeLineReq);
        len = client->req_len -
            bytes_to_int32(sizeof(xXF86OldVidModeValidateModeLineReq));
    }
    else {
        REQUEST_AT_LEAST_SIZE(xXF86VidModeValidateModeLineReq);
        len =
            client->req_len -
            bytes_to_int32(sizeof(xXF86VidModeValidateModeLineReq));
    }

    if (ver < 2) {
        /* convert from old format */
        stuff = &newstuff;
        stuff->length = oldstuff->length;
        stuff->screen = oldstuff->screen;
        stuff->dotclock = oldstuff->dotclock;
        stuff->hdisplay = oldstuff->hdisplay;
        stuff->hsyncstart = oldstuff->hsyncstart;
        stuff->hsyncend = oldstuff->hsyncend;
        stuff->htotal = oldstuff->htotal;
        stuff->hskew = 0;
        stuff->vdisplay = oldstuff->vdisplay;
        stuff->vsyncstart = oldstuff->vsyncstart;
        stuff->vsyncend = oldstuff->vsyncend;
        stuff->vtotal = oldstuff->vtotal;
        stuff->flags = oldstuff->flags;
        stuff->privsize = oldstuff->privsize;
    }

    DebugF("ValidateModeLine - scrn: %d clock: %ld\n",
           (int) stuff->screen, (unsigned long) stuff->dotclock);
    DebugF("                   hdsp: %d hbeg: %d hend: %d httl: %d\n",
           stuff->hdisplay, stuff->hsyncstart,
           stuff->hsyncend, stuff->htotal);
    DebugF("                   vdsp: %d vbeg: %d vend: %d vttl: %d flags: %ld\n",
           stuff->vdisplay, stuff->vsyncstart, stuff->vsyncend, stuff->vtotal,
           (unsigned long) stuff->flags);

    if (len != stuff->privsize)
        return BadLength;

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    status = MODE_OK;

    if (stuff->hsyncstart < stuff->hdisplay ||
        stuff->hsyncend < stuff->hsyncstart ||
        stuff->htotal < stuff->hsyncend ||
        stuff->vsyncstart < stuff->vdisplay ||
        stuff->vsyncend < stuff->vsyncstart ||
        stuff->vtotal < stuff->vsyncend) {
        status = MODE_BAD;
        goto status_reply;
    }

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (!pVidMode->GetCurrentModeline(pScreen, &mode, &dotClock))
        return BadValue;

    modetmp = VidModeCreateMode();
    VidModeCopyMode(mode, modetmp);

    VidModeSetModeValue(modetmp, VIDMODE_H_DISPLAY, stuff->hdisplay);
    VidModeSetModeValue(modetmp, VIDMODE_H_SYNCSTART, stuff->hsyncstart);
    VidModeSetModeValue(modetmp, VIDMODE_H_SYNCEND, stuff->hsyncend);
    VidModeSetModeValue(modetmp, VIDMODE_H_TOTAL, stuff->htotal);
    VidModeSetModeValue(modetmp, VIDMODE_H_SKEW, stuff->hskew);
    VidModeSetModeValue(modetmp, VIDMODE_V_DISPLAY, stuff->vdisplay);
    VidModeSetModeValue(modetmp, VIDMODE_V_SYNCSTART, stuff->vsyncstart);
    VidModeSetModeValue(modetmp, VIDMODE_V_SYNCEND, stuff->vsyncend);
    VidModeSetModeValue(modetmp, VIDMODE_V_TOTAL, stuff->vtotal);
    VidModeSetModeValue(modetmp, VIDMODE_FLAGS, stuff->flags);
    if (stuff->privsize)
        DebugF("ValidateModeLine - Privates in request have been ignored\n");

    /* Check that the mode is consistent with the monitor specs */
    if ((status =
         pVidMode->CheckModeForMonitor(pScreen, modetmp)) != MODE_OK)
        goto status_reply;

    /* Check that the driver is happy with the mode */
    status = pVidMode->CheckModeForDriver(pScreen, modetmp);

 status_reply:
    free(modetmp);

    rep = (xXF86VidModeValidateModeLineReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(SIZEOF(xXF86VidModeValidateModeLineReply)
                                 - SIZEOF(xGenericReply)),
        .status = status
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.status);
    }
    WriteToClient(client, sizeof(xXF86VidModeValidateModeLineReply), &rep);
    DebugF("ValidateModeLine - Succeeded (status = %d)\n", status);

    return Success;
}

static int
ProcVidModeSwitchMode(ClientPtr client)
{
    REQUEST(xXF86VidModeSwitchModeReq);
    ScreenPtr pScreen;
    VidModePtr pVidMode;

    DEBUG_P("XF86VidModeSwitchMode");

    REQUEST_SIZE_MATCH(xXF86VidModeSwitchModeReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    pVidMode->ZoomViewport(pScreen, (short) stuff->zoom);

    return Success;
}

static int
ProcVidModeSwitchToMode(ClientPtr client)
{
    REQUEST(xXF86VidModeSwitchToModeReq);
    xXF86OldVidModeSwitchToModeReq *oldstuff =
        (xXF86OldVidModeSwitchToModeReq *) client->requestBuffer;
    xXF86VidModeSwitchToModeReq newstuff;
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    DisplayModePtr mode;
    int len, dotClock;
    int ver;

    DEBUG_P("XF86VidModeSwitchToMode");

    ver = ClientMajorVersion(client);

    if (ver < 2) {
        REQUEST_AT_LEAST_SIZE(xXF86OldVidModeSwitchToModeReq);
        len =
            client->req_len -
            bytes_to_int32(sizeof(xXF86OldVidModeSwitchToModeReq));
    }
    else {
        REQUEST_AT_LEAST_SIZE(xXF86VidModeSwitchToModeReq);
        len =
            client->req_len -
            bytes_to_int32(sizeof(xXF86VidModeSwitchToModeReq));
    }

    if (ver < 2) {
        /* convert from old format */
        stuff = &newstuff;
        stuff->length = oldstuff->length;
        stuff->screen = oldstuff->screen;
        stuff->dotclock = oldstuff->dotclock;
        stuff->hdisplay = oldstuff->hdisplay;
        stuff->hsyncstart = oldstuff->hsyncstart;
        stuff->hsyncend = oldstuff->hsyncend;
        stuff->htotal = oldstuff->htotal;
        stuff->hskew = 0;
        stuff->vdisplay = oldstuff->vdisplay;
        stuff->vsyncstart = oldstuff->vsyncstart;
        stuff->vsyncend = oldstuff->vsyncend;
        stuff->vtotal = oldstuff->vtotal;
        stuff->flags = oldstuff->flags;
        stuff->privsize = oldstuff->privsize;
    }

    DebugF("SwitchToMode - scrn: %d clock: %ld\n",
           (int) stuff->screen, (unsigned long) stuff->dotclock);
    DebugF("               hdsp: %d hbeg: %d hend: %d httl: %d\n",
           stuff->hdisplay, stuff->hsyncstart,
           stuff->hsyncend, stuff->htotal);
    DebugF("               vdsp: %d vbeg: %d vend: %d vttl: %d flags: %ld\n",
           stuff->vdisplay, stuff->vsyncstart, stuff->vsyncend, stuff->vtotal,
           (unsigned long) stuff->flags);

    if (len != stuff->privsize)
        return BadLength;

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (!pVidMode->GetCurrentModeline(pScreen, &mode, &dotClock))
        return BadValue;

    if ((pVidMode->GetDotClock(pScreen, stuff->dotclock) == dotClock)
        && MODEMATCH(mode, stuff))
        return Success;

    if (!pVidMode->GetFirstModeline(pScreen, &mode, &dotClock))
        return BadValue;

    do {
        DebugF("Checking against clock: %d (%d)\n",
               VidModeGetModeValue(mode, VIDMODE_CLOCK), dotClock);
        DebugF("                 hdsp: %d hbeg: %d hend: %d httl: %d\n",
               VidModeGetModeValue(mode, VIDMODE_H_DISPLAY),
               VidModeGetModeValue(mode, VIDMODE_H_SYNCSTART),
               VidModeGetModeValue(mode, VIDMODE_H_SYNCEND),
               VidModeGetModeValue(mode, VIDMODE_H_TOTAL));
        DebugF("                 vdsp: %d vbeg: %d vend: %d vttl: %d flags: %d\n",
               VidModeGetModeValue(mode, VIDMODE_V_DISPLAY),
               VidModeGetModeValue(mode, VIDMODE_V_SYNCSTART),
               VidModeGetModeValue(mode, VIDMODE_V_SYNCEND),
               VidModeGetModeValue(mode, VIDMODE_V_TOTAL),
               VidModeGetModeValue(mode, VIDMODE_FLAGS));

        if ((pVidMode->GetDotClock(pScreen, stuff->dotclock) == dotClock) &&
            MODEMATCH(mode, stuff)) {

            if (!pVidMode->SwitchMode(pScreen, mode))
                return BadValue;

            DebugF("SwitchToMode - Succeeded\n");
            return Success;
        }
    } while (pVidMode->GetNextModeline(pScreen, &mode, &dotClock));

    return BadValue;
}

static int
ProcVidModeLockModeSwitch(ClientPtr client)
{
    REQUEST(xXF86VidModeLockModeSwitchReq);
    ScreenPtr pScreen;
    VidModePtr pVidMode;

    REQUEST_SIZE_MATCH(xXF86VidModeLockModeSwitchReq);

    DEBUG_P("XF86VidModeLockModeSwitch");

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (!pVidMode->LockZoom(pScreen, (short) stuff->lock))
        return VidModeErrorBase + XF86VidModeZoomLocked;

    return Success;
}

static int
ProcVidModeGetMonitor(ClientPtr client)
{
    REQUEST(xXF86VidModeGetMonitorReq);
    xXF86VidModeGetMonitorReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence
    };
    CARD32 *hsyncdata, *vsyncdata;
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    int i, nHsync, nVrefresh;

    DEBUG_P("XF86VidModeGetMonitor");

    REQUEST_SIZE_MATCH(xXF86VidModeGetMonitorReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    nHsync = pVidMode->GetMonitorValue(pScreen, VIDMODE_MON_NHSYNC, 0).i;
    nVrefresh = pVidMode->GetMonitorValue(pScreen, VIDMODE_MON_NVREFRESH, 0).i;

    if ((char *) (pVidMode->GetMonitorValue(pScreen, VIDMODE_MON_VENDOR, 0)).ptr)
        rep.vendorLength = strlen((char *) (pVidMode->GetMonitorValue(pScreen,
                                                                      VIDMODE_MON_VENDOR,
                                                                      0)).ptr);
    else
        rep.vendorLength = 0;
    if ((char *) (pVidMode->GetMonitorValue(pScreen, VIDMODE_MON_MODEL, 0)).ptr)
        rep.modelLength = strlen((char *) (pVidMode->GetMonitorValue(pScreen,
                                                                     VIDMODE_MON_MODEL,
                                                                     0)).ptr);
    else
        rep.modelLength = 0;
    rep.length =
        bytes_to_int32(SIZEOF(xXF86VidModeGetMonitorReply) -
                       SIZEOF(xGenericReply) + (nHsync +
                                                nVrefresh) * sizeof(CARD32) +
                       pad_to_int32(rep.vendorLength) +
                       pad_to_int32(rep.modelLength));
    rep.nhsync = nHsync;
    rep.nvsync = nVrefresh;
    hsyncdata = xallocarray(nHsync, sizeof(CARD32));
    if (!hsyncdata) {
        return BadAlloc;
    }
    vsyncdata = xallocarray(nVrefresh, sizeof(CARD32));

    if (!vsyncdata) {
        free(hsyncdata);
        return BadAlloc;
    }

    for (i = 0; i < nHsync; i++) {
        hsyncdata[i] = (unsigned short) (pVidMode->GetMonitorValue(pScreen,
                                                                   VIDMODE_MON_HSYNC_LO,
                                                                   i)).f |
            (unsigned
             short) (pVidMode->GetMonitorValue(pScreen, VIDMODE_MON_HSYNC_HI,
                                               i)).f << 16;
    }
    for (i = 0; i < nVrefresh; i++) {
        vsyncdata[i] = (unsigned short) (pVidMode->GetMonitorValue(pScreen,
                                                                   VIDMODE_MON_VREFRESH_LO,
                                                                   i)).f |
            (unsigned
             short) (pVidMode->GetMonitorValue(pScreen, VIDMODE_MON_VREFRESH_HI,
                                               i)).f << 16;
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }
    WriteToClient(client, SIZEOF(xXF86VidModeGetMonitorReply), &rep);
    client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
    WriteSwappedDataToClient(client, nHsync * sizeof(CARD32), hsyncdata);
    WriteSwappedDataToClient(client, nVrefresh * sizeof(CARD32), vsyncdata);
    if (rep.vendorLength)
        WriteToClient(client, rep.vendorLength,
                 (pVidMode->GetMonitorValue(pScreen, VIDMODE_MON_VENDOR, 0)).ptr);
    if (rep.modelLength)
        WriteToClient(client, rep.modelLength,
                 (pVidMode->GetMonitorValue(pScreen, VIDMODE_MON_MODEL, 0)).ptr);

    free(hsyncdata);
    free(vsyncdata);

    return Success;
}

static int
ProcVidModeGetViewPort(ClientPtr client)
{
    REQUEST(xXF86VidModeGetViewPortReq);
    xXF86VidModeGetViewPortReply rep;
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    int x, y;

    DEBUG_P("XF86VidModeGetViewPort");

    REQUEST_SIZE_MATCH(xXF86VidModeGetViewPortReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    pVidMode->GetViewPort(pScreen, &x, &y);

    rep = (xXF86VidModeGetViewPortReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .x = x,
        .y = y
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.x);
        swapl(&rep.y);
    }
    WriteToClient(client, SIZEOF(xXF86VidModeGetViewPortReply), &rep);
    return Success;
}

static int
ProcVidModeSetViewPort(ClientPtr client)
{
    REQUEST(xXF86VidModeSetViewPortReq);
    ScreenPtr pScreen;
    VidModePtr pVidMode;

    DEBUG_P("XF86VidModeSetViewPort");

    REQUEST_SIZE_MATCH(xXF86VidModeSetViewPortReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (!pVidMode->SetViewPort(pScreen, stuff->x, stuff->y))
        return BadValue;

    return Success;
}

static int
ProcVidModeGetDotClocks(ClientPtr client)
{
    REQUEST(xXF86VidModeGetDotClocksReq);
    xXF86VidModeGetDotClocksReply rep;
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    int n;
    int numClocks;
    CARD32 dotclock;
    int *Clocks = NULL;
    Bool ClockProg;

    DEBUG_P("XF86VidModeGetDotClocks");

    REQUEST_SIZE_MATCH(xXF86VidModeGetDotClocksReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    numClocks = pVidMode->GetNumOfClocks(pScreen, &ClockProg);

    rep = (xXF86VidModeGetDotClocksReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(SIZEOF(xXF86VidModeGetDotClocksReply)
                                 - SIZEOF(xGenericReply) + numClocks),
        .clocks = numClocks,
        .maxclocks = MAXCLOCKS,
        .flags = 0
    };

    if (!ClockProg) {
        Clocks = calloc(numClocks, sizeof(int));
        if (!Clocks)
            return BadValue;
        if (!pVidMode->GetClocks(pScreen, Clocks)) {
            free(Clocks);
            return BadValue;
        }
    }
    if (ClockProg) {
        rep.flags |= CLKFLAG_PROGRAMABLE;
    }
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.clocks);
        swapl(&rep.maxclocks);
        swapl(&rep.flags);
    }
    WriteToClient(client, sizeof(xXF86VidModeGetDotClocksReply), &rep);
    if (!ClockProg) {
        for (n = 0; n < numClocks; n++) {
            dotclock = Clocks[n];
            if (client->swapped) {
                WriteSwappedDataToClient(client, 4, (char *) &dotclock);
            }
            else {
                WriteToClient(client, 4, &dotclock);
            }
        }
    }

    free(Clocks);
    return Success;
}

static int
ProcVidModeSetGamma(ClientPtr client)
{
    REQUEST(xXF86VidModeSetGammaReq);
    ScreenPtr pScreen;
    VidModePtr pVidMode;

    DEBUG_P("XF86VidModeSetGamma");

    REQUEST_SIZE_MATCH(xXF86VidModeSetGammaReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (!pVidMode->SetGamma(pScreen, ((float) stuff->red) / 10000.,
                         ((float) stuff->green) / 10000.,
                         ((float) stuff->blue) / 10000.))
        return BadValue;

    return Success;
}

static int
ProcVidModeGetGamma(ClientPtr client)
{
    REQUEST(xXF86VidModeGetGammaReq);
    xXF86VidModeGetGammaReply rep;
    ScreenPtr pScreen;
    VidModePtr pVidMode;
    float red, green, blue;

    DEBUG_P("XF86VidModeGetGamma");

    REQUEST_SIZE_MATCH(xXF86VidModeGetGammaReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (!pVidMode->GetGamma(pScreen, &red, &green, &blue))
        return BadValue;
    rep = (xXF86VidModeGetGammaReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .red = (CARD32) (red * 10000.),
        .green = (CARD32) (green * 10000.),
        .blue = (CARD32) (blue * 10000.)
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.red);
        swapl(&rep.green);
        swapl(&rep.blue);
    }
    WriteToClient(client, sizeof(xXF86VidModeGetGammaReply), &rep);

    return Success;
}

static int
ProcVidModeSetGammaRamp(ClientPtr client)
{
    CARD16 *r, *g, *b;
    int length;
    ScreenPtr pScreen;
    VidModePtr pVidMode;

    REQUEST(xXF86VidModeSetGammaRampReq);
    REQUEST_AT_LEAST_SIZE(xXF86VidModeSetGammaRampReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (stuff->size != pVidMode->GetGammaRampSize(pScreen))
        return BadValue;

    length = (stuff->size + 1) & ~1;

    REQUEST_FIXED_SIZE(xXF86VidModeSetGammaRampReq, length * 6);

    r = (CARD16 *) &stuff[1];
    g = r + length;
    b = g + length;

    if (!pVidMode->SetGammaRamp(pScreen, stuff->size, r, g, b))
        return BadValue;

    return Success;
}

static int
ProcVidModeGetGammaRamp(ClientPtr client)
{
    CARD16 *ramp = NULL;
    int length;
    size_t ramplen = 0;
    xXF86VidModeGetGammaRampReply rep;
    ScreenPtr pScreen;
    VidModePtr pVidMode;

    REQUEST(xXF86VidModeGetGammaRampReq);

    REQUEST_SIZE_MATCH(xXF86VidModeGetGammaRampReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    if (stuff->size != pVidMode->GetGammaRampSize(pScreen))
        return BadValue;

    length = (stuff->size + 1) & ~1;

    if (stuff->size) {
        if (!(ramp = xallocarray(length, 3 * sizeof(CARD16))))
            return BadAlloc;
        ramplen = length * 3 * sizeof(CARD16);

        if (!pVidMode->GetGammaRamp(pScreen, stuff->size,
                                 ramp, ramp + length, ramp + (length * 2))) {
            free(ramp);
            return BadValue;
        }
    }
    rep = (xXF86VidModeGetGammaRampReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = (length >> 1) * 3,
        .size = stuff->size
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.size);
        SwapShorts((short *) ramp, length * 3);
    }
    WriteToClient(client, sizeof(xXF86VidModeGetGammaRampReply), &rep);

    if (stuff->size) {
        WriteToClient(client, ramplen, ramp);
        free(ramp);
    }

    return Success;
}


static int
ProcVidModeGetGammaRampSize(ClientPtr client)
{
    xXF86VidModeGetGammaRampSizeReply rep;
    ScreenPtr pScreen;
    VidModePtr pVidMode;

    REQUEST(xXF86VidModeGetGammaRampSizeReq);

    REQUEST_SIZE_MATCH(xXF86VidModeGetGammaRampSizeReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;
    pScreen = screenInfo.screens[stuff->screen];

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return BadImplementation;

    rep = (xXF86VidModeGetGammaRampSizeReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .size = pVidMode->GetGammaRampSize(pScreen)
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.size);
    }
    WriteToClient(client, sizeof(xXF86VidModeGetGammaRampSizeReply), &rep);

    return Success;
}

static int
ProcVidModeGetPermissions(ClientPtr client)
{
    xXF86VidModeGetPermissionsReply rep =  {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .permissions = XF86VM_READ_PERMISSION
    };

    REQUEST(xXF86VidModeGetPermissionsReq);

    REQUEST_SIZE_MATCH(xXF86VidModeGetPermissionsReq);

    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    if (VidModeAllowNonLocal || client->local) {
        rep.permissions |= XF86VM_WRITE_PERMISSION;
    }
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.permissions);
    }
    WriteToClient(client, sizeof(xXF86VidModeGetPermissionsReply), &rep);

    return Success;
}

static int
ProcVidModeSetClientVersion(ClientPtr client)
{
    REQUEST(xXF86VidModeSetClientVersionReq);

    VidModePrivPtr pPriv;

    DEBUG_P("XF86VidModeSetClientVersion");

    REQUEST_SIZE_MATCH(xXF86VidModeSetClientVersionReq);

    if ((pPriv = VM_GETPRIV(client)) == NULL) {
        pPriv = malloc(sizeof(VidModePrivRec));
        if (!pPriv)
            return BadAlloc;
        VM_SETPRIV(client, pPriv);
    }
    pPriv->major = stuff->major;

    pPriv->minor = stuff->minor;

    return Success;
}

static int
ProcVidModeDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_XF86VidModeQueryVersion:
        return ProcVidModeQueryVersion(client);
    case X_XF86VidModeGetModeLine:
        return ProcVidModeGetModeLine(client);
    case X_XF86VidModeGetMonitor:
        return ProcVidModeGetMonitor(client);
    case X_XF86VidModeGetAllModeLines:
        return ProcVidModeGetAllModeLines(client);
    case X_XF86VidModeValidateModeLine:
        return ProcVidModeValidateModeLine(client);
    case X_XF86VidModeGetViewPort:
        return ProcVidModeGetViewPort(client);
    case X_XF86VidModeGetDotClocks:
        return ProcVidModeGetDotClocks(client);
    case X_XF86VidModeSetClientVersion:
        return ProcVidModeSetClientVersion(client);
    case X_XF86VidModeGetGamma:
        return ProcVidModeGetGamma(client);
    case X_XF86VidModeGetGammaRamp:
        return ProcVidModeGetGammaRamp(client);
    case X_XF86VidModeGetGammaRampSize:
        return ProcVidModeGetGammaRampSize(client);
    case X_XF86VidModeGetPermissions:
        return ProcVidModeGetPermissions(client);
    default:
        if (VidModeAllowNonLocal || client->local) {
            switch (stuff->data) {
            case X_XF86VidModeAddModeLine:
                return ProcVidModeAddModeLine(client);
            case X_XF86VidModeDeleteModeLine:
                return ProcVidModeDeleteModeLine(client);
            case X_XF86VidModeModModeLine:
                return ProcVidModeModModeLine(client);
            case X_XF86VidModeSwitchMode:
                return ProcVidModeSwitchMode(client);
            case X_XF86VidModeSwitchToMode:
                return ProcVidModeSwitchToMode(client);
            case X_XF86VidModeLockModeSwitch:
                return ProcVidModeLockModeSwitch(client);
            case X_XF86VidModeSetViewPort:
                return ProcVidModeSetViewPort(client);
            case X_XF86VidModeSetGamma:
                return ProcVidModeSetGamma(client);
            case X_XF86VidModeSetGammaRamp:
                return ProcVidModeSetGammaRamp(client);
            default:
                return BadRequest;
            }
        }
        else
            return VidModeErrorBase + XF86VidModeClientNotLocal;
    }
}

static int _X_COLD
SProcVidModeQueryVersion(ClientPtr client)
{
    REQUEST(xXF86VidModeQueryVersionReq);
    swaps(&stuff->length);
    return ProcVidModeQueryVersion(client);
}

static int _X_COLD
SProcVidModeGetModeLine(ClientPtr client)
{
    REQUEST(xXF86VidModeGetModeLineReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeGetModeLineReq);
    swaps(&stuff->screen);
    return ProcVidModeGetModeLine(client);
}

static int _X_COLD
SProcVidModeGetAllModeLines(ClientPtr client)
{
    REQUEST(xXF86VidModeGetAllModeLinesReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeGetAllModeLinesReq);
    swaps(&stuff->screen);
    return ProcVidModeGetAllModeLines(client);
}

static int _X_COLD
SProcVidModeAddModeLine(ClientPtr client)
{
    xXF86OldVidModeAddModeLineReq *oldstuff =
        (xXF86OldVidModeAddModeLineReq *) client->requestBuffer;
    int ver;

    REQUEST(xXF86VidModeAddModeLineReq);
    ver = ClientMajorVersion(client);
    if (ver < 2) {
        swaps(&oldstuff->length);
        REQUEST_AT_LEAST_SIZE(xXF86OldVidModeAddModeLineReq);
        swapl(&oldstuff->screen);
        swaps(&oldstuff->hdisplay);
        swaps(&oldstuff->hsyncstart);
        swaps(&oldstuff->hsyncend);
        swaps(&oldstuff->htotal);
        swaps(&oldstuff->vdisplay);
        swaps(&oldstuff->vsyncstart);
        swaps(&oldstuff->vsyncend);
        swaps(&oldstuff->vtotal);
        swapl(&oldstuff->flags);
        swapl(&oldstuff->privsize);
        SwapRestL(oldstuff);
    }
    else {
        swaps(&stuff->length);
        REQUEST_AT_LEAST_SIZE(xXF86VidModeAddModeLineReq);
        swapl(&stuff->screen);
        swaps(&stuff->hdisplay);
        swaps(&stuff->hsyncstart);
        swaps(&stuff->hsyncend);
        swaps(&stuff->htotal);
        swaps(&stuff->hskew);
        swaps(&stuff->vdisplay);
        swaps(&stuff->vsyncstart);
        swaps(&stuff->vsyncend);
        swaps(&stuff->vtotal);
        swapl(&stuff->flags);
        swapl(&stuff->privsize);
        SwapRestL(stuff);
    }
    return ProcVidModeAddModeLine(client);
}

static int _X_COLD
SProcVidModeDeleteModeLine(ClientPtr client)
{
    xXF86OldVidModeDeleteModeLineReq *oldstuff =
        (xXF86OldVidModeDeleteModeLineReq *) client->requestBuffer;
    int ver;

    REQUEST(xXF86VidModeDeleteModeLineReq);
    ver = ClientMajorVersion(client);
    if (ver < 2) {
        swaps(&oldstuff->length);
        REQUEST_AT_LEAST_SIZE(xXF86OldVidModeDeleteModeLineReq);
        swapl(&oldstuff->screen);
        swaps(&oldstuff->hdisplay);
        swaps(&oldstuff->hsyncstart);
        swaps(&oldstuff->hsyncend);
        swaps(&oldstuff->htotal);
        swaps(&oldstuff->vdisplay);
        swaps(&oldstuff->vsyncstart);
        swaps(&oldstuff->vsyncend);
        swaps(&oldstuff->vtotal);
        swapl(&oldstuff->flags);
        swapl(&oldstuff->privsize);
        SwapRestL(oldstuff);
    }
    else {
        swaps(&stuff->length);
        REQUEST_AT_LEAST_SIZE(xXF86VidModeDeleteModeLineReq);
        swapl(&stuff->screen);
        swaps(&stuff->hdisplay);
        swaps(&stuff->hsyncstart);
        swaps(&stuff->hsyncend);
        swaps(&stuff->htotal);
        swaps(&stuff->hskew);
        swaps(&stuff->vdisplay);
        swaps(&stuff->vsyncstart);
        swaps(&stuff->vsyncend);
        swaps(&stuff->vtotal);
        swapl(&stuff->flags);
        swapl(&stuff->privsize);
        SwapRestL(stuff);
    }
    return ProcVidModeDeleteModeLine(client);
}

static int _X_COLD
SProcVidModeModModeLine(ClientPtr client)
{
    xXF86OldVidModeModModeLineReq *oldstuff =
        (xXF86OldVidModeModModeLineReq *) client->requestBuffer;
    int ver;

    REQUEST(xXF86VidModeModModeLineReq);
    ver = ClientMajorVersion(client);
    if (ver < 2) {
        swaps(&oldstuff->length);
        REQUEST_AT_LEAST_SIZE(xXF86OldVidModeModModeLineReq);
        swapl(&oldstuff->screen);
        swaps(&oldstuff->hdisplay);
        swaps(&oldstuff->hsyncstart);
        swaps(&oldstuff->hsyncend);
        swaps(&oldstuff->htotal);
        swaps(&oldstuff->vdisplay);
        swaps(&oldstuff->vsyncstart);
        swaps(&oldstuff->vsyncend);
        swaps(&oldstuff->vtotal);
        swapl(&oldstuff->flags);
        swapl(&oldstuff->privsize);
        SwapRestL(oldstuff);
    }
    else {
        swaps(&stuff->length);
        REQUEST_AT_LEAST_SIZE(xXF86VidModeModModeLineReq);
        swapl(&stuff->screen);
        swaps(&stuff->hdisplay);
        swaps(&stuff->hsyncstart);
        swaps(&stuff->hsyncend);
        swaps(&stuff->htotal);
        swaps(&stuff->hskew);
        swaps(&stuff->vdisplay);
        swaps(&stuff->vsyncstart);
        swaps(&stuff->vsyncend);
        swaps(&stuff->vtotal);
        swapl(&stuff->flags);
        swapl(&stuff->privsize);
        SwapRestL(stuff);
    }
    return ProcVidModeModModeLine(client);
}

static int _X_COLD
SProcVidModeValidateModeLine(ClientPtr client)
{
    xXF86OldVidModeValidateModeLineReq *oldstuff =
        (xXF86OldVidModeValidateModeLineReq *) client->requestBuffer;
    int ver;

    REQUEST(xXF86VidModeValidateModeLineReq);
    ver = ClientMajorVersion(client);
    if (ver < 2) {
        swaps(&oldstuff->length);
        REQUEST_AT_LEAST_SIZE(xXF86OldVidModeValidateModeLineReq);
        swapl(&oldstuff->screen);
        swaps(&oldstuff->hdisplay);
        swaps(&oldstuff->hsyncstart);
        swaps(&oldstuff->hsyncend);
        swaps(&oldstuff->htotal);
        swaps(&oldstuff->vdisplay);
        swaps(&oldstuff->vsyncstart);
        swaps(&oldstuff->vsyncend);
        swaps(&oldstuff->vtotal);
        swapl(&oldstuff->flags);
        swapl(&oldstuff->privsize);
        SwapRestL(oldstuff);
    }
    else {
        swaps(&stuff->length);
        REQUEST_AT_LEAST_SIZE(xXF86VidModeValidateModeLineReq);
        swapl(&stuff->screen);
        swaps(&stuff->hdisplay);
        swaps(&stuff->hsyncstart);
        swaps(&stuff->hsyncend);
        swaps(&stuff->htotal);
        swaps(&stuff->hskew);
        swaps(&stuff->vdisplay);
        swaps(&stuff->vsyncstart);
        swaps(&stuff->vsyncend);
        swaps(&stuff->vtotal);
        swapl(&stuff->flags);
        swapl(&stuff->privsize);
        SwapRestL(stuff);
    }
    return ProcVidModeValidateModeLine(client);
}

static int _X_COLD
SProcVidModeSwitchMode(ClientPtr client)
{
    REQUEST(xXF86VidModeSwitchModeReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeSwitchModeReq);
    swaps(&stuff->screen);
    swaps(&stuff->zoom);
    return ProcVidModeSwitchMode(client);
}

static int _X_COLD
SProcVidModeSwitchToMode(ClientPtr client)
{
    REQUEST(xXF86VidModeSwitchToModeReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeSwitchToModeReq);
    swapl(&stuff->screen);
    return ProcVidModeSwitchToMode(client);
}

static int _X_COLD
SProcVidModeLockModeSwitch(ClientPtr client)
{
    REQUEST(xXF86VidModeLockModeSwitchReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeLockModeSwitchReq);
    swaps(&stuff->screen);
    swaps(&stuff->lock);
    return ProcVidModeLockModeSwitch(client);
}

static int _X_COLD
SProcVidModeGetMonitor(ClientPtr client)
{
    REQUEST(xXF86VidModeGetMonitorReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeGetMonitorReq);
    swaps(&stuff->screen);
    return ProcVidModeGetMonitor(client);
}

static int _X_COLD
SProcVidModeGetViewPort(ClientPtr client)
{
    REQUEST(xXF86VidModeGetViewPortReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeGetViewPortReq);
    swaps(&stuff->screen);
    return ProcVidModeGetViewPort(client);
}

static int _X_COLD
SProcVidModeSetViewPort(ClientPtr client)
{
    REQUEST(xXF86VidModeSetViewPortReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeSetViewPortReq);
    swaps(&stuff->screen);
    swapl(&stuff->x);
    swapl(&stuff->y);
    return ProcVidModeSetViewPort(client);
}

static int _X_COLD
SProcVidModeGetDotClocks(ClientPtr client)
{
    REQUEST(xXF86VidModeGetDotClocksReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeGetDotClocksReq);
    swaps(&stuff->screen);
    return ProcVidModeGetDotClocks(client);
}

static int _X_COLD
SProcVidModeSetClientVersion(ClientPtr client)
{
    REQUEST(xXF86VidModeSetClientVersionReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeSetClientVersionReq);
    swaps(&stuff->major);
    swaps(&stuff->minor);
    return ProcVidModeSetClientVersion(client);
}

static int _X_COLD
SProcVidModeSetGamma(ClientPtr client)
{
    REQUEST(xXF86VidModeSetGammaReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeSetGammaReq);
    swaps(&stuff->screen);
    swapl(&stuff->red);
    swapl(&stuff->green);
    swapl(&stuff->blue);
    return ProcVidModeSetGamma(client);
}

static int _X_COLD
SProcVidModeGetGamma(ClientPtr client)
{
    REQUEST(xXF86VidModeGetGammaReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeGetGammaReq);
    swaps(&stuff->screen);
    return ProcVidModeGetGamma(client);
}

static int _X_COLD
SProcVidModeSetGammaRamp(ClientPtr client)
{
    int length;

    REQUEST(xXF86VidModeSetGammaRampReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xXF86VidModeSetGammaRampReq);
    swaps(&stuff->size);
    swaps(&stuff->screen);
    length = ((stuff->size + 1) & ~1) * 6;
    REQUEST_FIXED_SIZE(xXF86VidModeSetGammaRampReq, length);
    SwapRestS(stuff);
    return ProcVidModeSetGammaRamp(client);
}

static int _X_COLD
SProcVidModeGetGammaRamp(ClientPtr client)
{
    REQUEST(xXF86VidModeGetGammaRampReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeGetGammaRampReq);
    swaps(&stuff->size);
    swaps(&stuff->screen);
    return ProcVidModeGetGammaRamp(client);
}

static int _X_COLD
SProcVidModeGetGammaRampSize(ClientPtr client)
{
    REQUEST(xXF86VidModeGetGammaRampSizeReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeGetGammaRampSizeReq);
    swaps(&stuff->screen);
    return ProcVidModeGetGammaRampSize(client);
}

static int _X_COLD
SProcVidModeGetPermissions(ClientPtr client)
{
    REQUEST(xXF86VidModeGetPermissionsReq);
    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXF86VidModeGetPermissionsReq);
    swaps(&stuff->screen);
    return ProcVidModeGetPermissions(client);
}

static int _X_COLD
SProcVidModeDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_XF86VidModeQueryVersion:
        return SProcVidModeQueryVersion(client);
    case X_XF86VidModeGetModeLine:
        return SProcVidModeGetModeLine(client);
    case X_XF86VidModeGetMonitor:
        return SProcVidModeGetMonitor(client);
    case X_XF86VidModeGetAllModeLines:
        return SProcVidModeGetAllModeLines(client);
    case X_XF86VidModeGetViewPort:
        return SProcVidModeGetViewPort(client);
    case X_XF86VidModeValidateModeLine:
        return SProcVidModeValidateModeLine(client);
    case X_XF86VidModeGetDotClocks:
        return SProcVidModeGetDotClocks(client);
    case X_XF86VidModeSetClientVersion:
        return SProcVidModeSetClientVersion(client);
    case X_XF86VidModeGetGamma:
        return SProcVidModeGetGamma(client);
    case X_XF86VidModeGetGammaRamp:
        return SProcVidModeGetGammaRamp(client);
    case X_XF86VidModeGetGammaRampSize:
        return SProcVidModeGetGammaRampSize(client);
    case X_XF86VidModeGetPermissions:
        return SProcVidModeGetPermissions(client);
    default:
        if (VidModeAllowNonLocal || client->local) {
            switch (stuff->data) {
            case X_XF86VidModeAddModeLine:
                return SProcVidModeAddModeLine(client);
            case X_XF86VidModeDeleteModeLine:
                return SProcVidModeDeleteModeLine(client);
            case X_XF86VidModeModModeLine:
                return SProcVidModeModModeLine(client);
            case X_XF86VidModeSwitchMode:
                return SProcVidModeSwitchMode(client);
            case X_XF86VidModeSwitchToMode:
                return SProcVidModeSwitchToMode(client);
            case X_XF86VidModeLockModeSwitch:
                return SProcVidModeLockModeSwitch(client);
            case X_XF86VidModeSetViewPort:
                return SProcVidModeSetViewPort(client);
            case X_XF86VidModeSetGamma:
                return SProcVidModeSetGamma(client);
            case X_XF86VidModeSetGammaRamp:
                return SProcVidModeSetGammaRamp(client);
            default:
                return BadRequest;
            }
        }
        else
            return VidModeErrorBase + XF86VidModeClientNotLocal;
    }
}

void
VidModeAddExtension(Bool allow_non_local)
{
    ExtensionEntry *extEntry;

    DEBUG_P("VidModeAddExtension");

    if (!dixRegisterPrivateKey(VidModeClientPrivateKey, PRIVATE_CLIENT, 0))
        return;

    if ((extEntry = AddExtension(XF86VIDMODENAME,
                                 XF86VidModeNumberEvents,
                                 XF86VidModeNumberErrors,
                                 ProcVidModeDispatch,
                                 SProcVidModeDispatch,
                                 NULL, StandardMinorOpcode))) {
        VidModeErrorBase = extEntry->errorBase;
        VidModeAllowNonLocal = allow_non_local;
    }
}

VidModePtr VidModeGetPtr(ScreenPtr pScreen)
{
    return (VidModePtr) (dixLookupPrivate(&pScreen->devPrivates, VidModePrivateKey));
}

VidModePtr VidModeInit(ScreenPtr pScreen)
{
    if (!dixRegisterPrivateKey(VidModePrivateKey, PRIVATE_SCREEN, sizeof(VidModeRec)))
        return NULL;

    return VidModeGetPtr(pScreen);
}

#endif /* XF86VIDMODE */
