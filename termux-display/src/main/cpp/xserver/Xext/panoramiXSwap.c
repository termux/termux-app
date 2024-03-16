/*****************************************************************
Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.
******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "cursor.h"
#include "cursorstr.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "gc.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "window.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "panoramiX.h"
#include <X11/extensions/panoramiXproto.h>
#include "panoramiXsrv.h"
#include "globals.h"
#include "panoramiXh.h"

static int _X_COLD
SProcPanoramiXQueryVersion(ClientPtr client)
{
    REQUEST(xPanoramiXQueryVersionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xPanoramiXQueryVersionReq);
    return ProcPanoramiXQueryVersion(client);
}

static int _X_COLD
SProcPanoramiXGetState(ClientPtr client)
{
    REQUEST(xPanoramiXGetStateReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
    swapl(&stuff->window);
    return ProcPanoramiXGetState(client);
}

static int _X_COLD
SProcPanoramiXGetScreenCount(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenCountReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
    swapl(&stuff->window);
    return ProcPanoramiXGetScreenCount(client);
}

static int _X_COLD
SProcPanoramiXGetScreenSize(ClientPtr client)
{
    REQUEST(xPanoramiXGetScreenSizeReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);
    swapl(&stuff->window);
    swapl(&stuff->screen);
    return ProcPanoramiXGetScreenSize(client);
}

static int _X_COLD
SProcXineramaIsActive(ClientPtr client)
{
    REQUEST(xXineramaIsActiveReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXineramaIsActiveReq);
    return ProcXineramaIsActive(client);
}

static int _X_COLD
SProcXineramaQueryScreens(ClientPtr client)
{
    REQUEST(xXineramaQueryScreensReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);
    return ProcXineramaQueryScreens(client);
}

int _X_COLD
SProcPanoramiXDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_PanoramiXQueryVersion:
        return SProcPanoramiXQueryVersion(client);
    case X_PanoramiXGetState:
        return SProcPanoramiXGetState(client);
    case X_PanoramiXGetScreenCount:
        return SProcPanoramiXGetScreenCount(client);
    case X_PanoramiXGetScreenSize:
        return SProcPanoramiXGetScreenSize(client);
    case X_XineramaIsActive:
        return SProcXineramaIsActive(client);
    case X_XineramaQueryScreens:
        return SProcXineramaQueryScreens(client);
    }
    return BadRequest;
}
