/*
 * Copyright 2007-2008 Peter Hutterer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Peter Hutterer, University of South Australia, NICTA
 */

/***********************************************************************
 *
 * Request to change a given device pointer's cursor.
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>              /* for inputstr.h    */
#include <X11/Xproto.h>         /* Request macro     */
#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "scrnintstr.h"         /* screen structure  */
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2proto.h>
#include "extnsionst.h"
#include "exevents.h"
#include "exglobals.h"
#include "input.h"

#include "xichangecursor.h"

/***********************************************************************
 *
 * This procedure allows a client to set one pointer's cursor.
 *
 */

int _X_COLD
SProcXIChangeCursor(ClientPtr client)
{
    REQUEST(xXIChangeCursorReq);
    REQUEST_SIZE_MATCH(xXIChangeCursorReq);
    swaps(&stuff->length);
    swapl(&stuff->win);
    swapl(&stuff->cursor);
    swaps(&stuff->deviceid);
    return (ProcXIChangeCursor(client));
}

int
ProcXIChangeCursor(ClientPtr client)
{
    int rc;
    WindowPtr pWin = NULL;
    DeviceIntPtr pDev = NULL;
    CursorPtr pCursor = NULL;

    REQUEST(xXIChangeCursorReq);
    REQUEST_SIZE_MATCH(xXIChangeCursorReq);

    rc = dixLookupDevice(&pDev, stuff->deviceid, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    if (!IsMaster(pDev) || !IsPointerDevice(pDev))
        return BadDevice;

    if (stuff->win != None) {
        rc = dixLookupWindow(&pWin, stuff->win, client, DixSetAttrAccess);
        if (rc != Success)
            return rc;
    }

    if (stuff->cursor == None) {
        if (pWin == pWin->drawable.pScreen->root)
            pCursor = rootCursor;
        else
            pCursor = (CursorPtr) None;
    }
    else {
        rc = dixLookupResourceByType((void **) &pCursor, stuff->cursor,
                                     RT_CURSOR, client, DixUseAccess);
        if (rc != Success)
            return rc;
    }

    ChangeWindowDeviceCursor(pWin, pDev, pCursor);

    return Success;
}
