/*

Copyright 1992, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include <X11/extensions/bigreqsproto.h>
#include "opaque.h"
#include "extinit.h"

static int
ProcBigReqDispatch(ClientPtr client)
{
    REQUEST(xBigReqEnableReq);
    xBigReqEnableReply rep;

    if (client->swapped) {
        swaps(&stuff->length);
    }
    if (stuff->brReqType != X_BigReqEnable)
        return BadRequest;
    REQUEST_SIZE_MATCH(xBigReqEnableReq);
    client->big_requests = TRUE;
    rep = (xBigReqEnableReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .max_request_size = maxBigRequestSize
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.max_request_size);
    }
    WriteToClient(client, sizeof(xBigReqEnableReply), &rep);
    return Success;
}

void
BigReqExtensionInit(void)
{
    AddExtension(XBigReqExtensionName, 0, 0,
                 ProcBigReqDispatch, ProcBigReqDispatch,
                 NULL, StandardMinorOpcode);
}
