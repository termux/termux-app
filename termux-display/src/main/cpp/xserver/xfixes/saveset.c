/*
 * Copyright © 2002 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "xfixesint.h"

int
ProcXFixesChangeSaveSet(ClientPtr client)
{
    Bool toRoot, map;
    int result;
    WindowPtr pWin;

    REQUEST(xXFixesChangeSaveSetReq);

    REQUEST_SIZE_MATCH(xXFixesChangeSaveSetReq);
    result = dixLookupWindow(&pWin, stuff->window, client, DixManageAccess);
    if (result != Success)
        return result;
    if (client->clientAsMask == (CLIENT_BITS(pWin->drawable.id)))
        return BadMatch;
    if ((stuff->mode != SetModeInsert) && (stuff->mode != SetModeDelete)) {
        client->errorValue = stuff->mode;
        return BadValue;
    }
    if ((stuff->target != SaveSetNearest) && (stuff->target != SaveSetRoot)) {
        client->errorValue = stuff->target;
        return BadValue;
    }
    if ((stuff->map != SaveSetMap) && (stuff->map != SaveSetUnmap)) {
        client->errorValue = stuff->map;
        return BadValue;
    }
    toRoot = (stuff->target == SaveSetRoot);
    map = (stuff->map == SaveSetMap);
    return AlterSaveSetForClient(client, pWin, stuff->mode, toRoot, map);
}

int _X_COLD
SProcXFixesChangeSaveSet(ClientPtr client)
{
    REQUEST(xXFixesChangeSaveSetReq);
    REQUEST_SIZE_MATCH(xXFixesChangeSaveSetReq);

    swaps(&stuff->length);
    swapl(&stuff->window);
    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}
