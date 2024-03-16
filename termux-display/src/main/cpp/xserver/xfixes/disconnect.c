/*
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2010 Red Hat, Inc.
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
#include "opaque.h"

static DevPrivateKeyRec ClientDisconnectPrivateKeyRec;

#define ClientDisconnectPrivateKey (&ClientDisconnectPrivateKeyRec)

typedef struct _ClientDisconnect {
    int disconnect_mode;
} ClientDisconnectRec, *ClientDisconnectPtr;

#define GetClientDisconnect(s) \
    ((ClientDisconnectPtr) dixLookupPrivate(&(s)->devPrivates, \
                                            ClientDisconnectPrivateKey))

int
ProcXFixesSetClientDisconnectMode(ClientPtr client)
{
    ClientDisconnectPtr pDisconnect = GetClientDisconnect(client);

    REQUEST(xXFixesSetClientDisconnectModeReq);

    pDisconnect->disconnect_mode = stuff->disconnect_mode;

    return Success;
}

int _X_COLD
SProcXFixesSetClientDisconnectMode(ClientPtr client)
{
    REQUEST(xXFixesSetClientDisconnectModeReq);

    swaps(&stuff->length);

    REQUEST_AT_LEAST_SIZE(xXFixesSetClientDisconnectModeReq);

    swapl(&stuff->disconnect_mode);

    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

int
ProcXFixesGetClientDisconnectMode(ClientPtr client)
{
    ClientDisconnectPtr pDisconnect = GetClientDisconnect(client);
    xXFixesGetClientDisconnectModeReply reply;

    REQUEST_SIZE_MATCH(xXFixesGetClientDisconnectModeReq);

    reply = (xXFixesGetClientDisconnectModeReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .disconnect_mode = pDisconnect->disconnect_mode,
    };
    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.disconnect_mode);
    }
    WriteToClient(client, sizeof(xXFixesGetClientDisconnectModeReply), &reply);

    return Success;
}

int _X_COLD
SProcXFixesGetClientDisconnectMode(ClientPtr client)
{
    REQUEST(xXFixesGetClientDisconnectModeReq);

    swaps(&stuff->length);

    REQUEST_SIZE_MATCH(xXFixesGetClientDisconnectModeReq);

    return (*ProcXFixesVector[stuff->xfixesReqType]) (client);
}

Bool
XFixesShouldDisconnectClient(ClientPtr client)
{
    ClientDisconnectPtr pDisconnect = GetClientDisconnect(client);

    if (!pDisconnect)
        return FALSE;

    if (dispatchExceptionAtReset & DE_TERMINATE)
        return (pDisconnect->disconnect_mode & XFixesClientDisconnectFlagTerminate);

    return FALSE;
}

Bool
XFixesClientDisconnectInit(void)
{
    if (!dixRegisterPrivateKey(&ClientDisconnectPrivateKeyRec,
                               PRIVATE_CLIENT, sizeof(ClientDisconnectRec)))
        return FALSE;

    return TRUE;
}
