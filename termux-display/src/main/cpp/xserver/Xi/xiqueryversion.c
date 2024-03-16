/*
 * Copyright © 2009 Red Hat, Inc.
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
 * Authors: Peter Hutterer
 *
 */

/**
 * @file xiqueryversion.c
 * Protocol handling for the XIQueryVersion request/reply.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "inputstr.h"

#include <X11/Xmd.h>
#include <X11/X.h>
#include <X11/extensions/XI2proto.h>

#include "exglobals.h"
#include "exevents.h"
#include "xiqueryversion.h"
#include "misc.h"

extern XExtensionVersion XIVersion;     /* defined in getvers.c */

/**
 * Return the supported XI version.
 *
 * Saves the version the client claims to support as well, for future
 * reference.
 */
int
ProcXIQueryVersion(ClientPtr client)
{
    xXIQueryVersionReply rep;
    XIClientPtr pXIClient;
    int major, minor;

    REQUEST(xXIQueryVersionReq);
    REQUEST_SIZE_MATCH(xXIQueryVersionReq);

    /* This request only exists after XI2 */
    if (stuff->major_version < 2) {
        client->errorValue = stuff->major_version;
        return BadValue;
    }

    pXIClient = dixLookupPrivate(&client->devPrivates, XIClientPrivateKey);

    if (version_compare(XIVersion.major_version, XIVersion.minor_version,
                stuff->major_version, stuff->minor_version) > 0) {
        major = stuff->major_version;
        minor = stuff->minor_version;
    } else {
        major = XIVersion.major_version;
        minor = XIVersion.minor_version;
    }

    if (pXIClient->major_version) {

        /* Check to see if the client has only ever asked
         * for version 2.2 or higher
         */
        if (version_compare(major, minor, 2, 2) >= 0 &&
            version_compare(pXIClient->major_version, pXIClient->minor_version, 2, 2) >= 0)
        {

            /* As of version 2.2, Peter promises to never again break
             * backward compatibility, so we'll return the requested
             * version to the client but leave the server internal
             * version set to the highest requested value
             */
            if (version_compare(major, minor,
                                pXIClient->major_version, pXIClient->minor_version) > 0)
            {
                pXIClient->major_version = major;
                pXIClient->minor_version = minor;
            }
        } else {
            if (version_compare(major, minor,
                                pXIClient->major_version, pXIClient->minor_version) < 0) {

                client->errorValue = stuff->major_version;
                return BadValue;
            }
            major = pXIClient->major_version;
            minor = pXIClient->minor_version;
        }
    } else {
        pXIClient->major_version = major;
        pXIClient->minor_version = minor;
    }

    rep = (xXIQueryVersionReply) {
        .repType = X_Reply,
        .RepType = X_XIQueryVersion,
        .sequenceNumber = client->sequence,
        .length = 0,
        .major_version = major,
        .minor_version = minor
    };

    WriteReplyToClient(client, sizeof(xXIQueryVersionReply), &rep);

    return Success;
}

/* Swapping routines */

int _X_COLD
SProcXIQueryVersion(ClientPtr client)
{
    REQUEST(xXIQueryVersionReq);
    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xXIQueryVersionReq);
    swaps(&stuff->major_version);
    swaps(&stuff->minor_version);
    return (ProcXIQueryVersion(client));
}

void _X_COLD
SRepXIQueryVersion(ClientPtr client, int size, xXIQueryVersionReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swaps(&rep->major_version);
    swaps(&rep->minor_version);
    WriteToClient(client, size, rep);
}
