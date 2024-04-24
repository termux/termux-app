/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "dispatch.h"
#include "privates.h"
#include "registry.h"
#include "xace.h"

#define LAST_ERROR 255

static ExtensionEntry **extensions = (ExtensionEntry **) NULL;

int lastEvent = EXTENSION_EVENT_BASE;
static int lastError = FirstExtensionError;
static unsigned int NumExtensions = 0;

ExtensionEntry *
AddExtension(const char *name, int NumEvents, int NumErrors,
             int (*MainProc) (ClientPtr c1),
             int (*SwappedMainProc) (ClientPtr c2),
             void (*CloseDownProc) (ExtensionEntry * e),
             unsigned short (*MinorOpcodeProc) (ClientPtr c3))
{
    int i;
    ExtensionEntry *ext, **newexts;

    if (!MainProc || !SwappedMainProc || !MinorOpcodeProc)
        return ((ExtensionEntry *) NULL);
    if ((lastEvent + NumEvents > MAXEVENTS) ||
        (unsigned) (lastError + NumErrors > LAST_ERROR)) {
        LogMessage(X_ERROR, "Not enabling extension %s: maximum number of "
                   "events or errors exceeded.\n", name);
        return ((ExtensionEntry *) NULL);
    }

    ext = calloc(sizeof(ExtensionEntry), 1);
    if (!ext)
        return NULL;
    if (!dixAllocatePrivates(&ext->devPrivates, PRIVATE_EXTENSION)) {
        free(ext);
        return NULL;
    }
    ext->name = strdup(name);
    if (!ext->name) {
        dixFreePrivates(ext->devPrivates, PRIVATE_EXTENSION);
        free(ext);
        return ((ExtensionEntry *) NULL);
    }
    i = NumExtensions;
    newexts = reallocarray(extensions, i + 1, sizeof(ExtensionEntry *));
    if (!newexts) {
        free((void *) ext->name);
        dixFreePrivates(ext->devPrivates, PRIVATE_EXTENSION);
        free(ext);
        return ((ExtensionEntry *) NULL);
    }
    NumExtensions++;
    extensions = newexts;
    extensions[i] = ext;
    ext->index = i;
    ext->base = i + EXTENSION_BASE;
    ext->CloseDown = CloseDownProc;
    ext->MinorOpcode = MinorOpcodeProc;
    ProcVector[i + EXTENSION_BASE] = MainProc;
    SwappedProcVector[i + EXTENSION_BASE] = SwappedMainProc;
    if (NumEvents) {
        ext->eventBase = lastEvent;
        ext->eventLast = lastEvent + NumEvents;
        lastEvent += NumEvents;
    }
    else {
        ext->eventBase = 0;
        ext->eventLast = 0;
    }
    if (NumErrors) {
        ext->errorBase = lastError;
        ext->errorLast = lastError + NumErrors;
        lastError += NumErrors;
    }
    else {
        ext->errorBase = 0;
        ext->errorLast = 0;
    }

#ifdef X_REGISTRY_REQUEST
    RegisterExtensionNames(ext);
#endif
    return ext;
}

static int
FindExtension(const char *extname, int len)
{
    int i;

    for (i = 0; i < NumExtensions; i++) {
        if ((strlen(extensions[i]->name) == len) &&
            !strncmp(extname, extensions[i]->name, len))
            break;
    }
    return ((i == NumExtensions) ? -1 : i);
}

/*
 * CheckExtension returns the extensions[] entry for the requested
 * extension name.  Maybe this could just return a Bool instead?
 */
ExtensionEntry *
CheckExtension(const char *extname)
{
    int n;

    n = FindExtension(extname, strlen(extname));
    if (n != -1)
        return extensions[n];
    else
        return NULL;
}

/*
 * Added as part of Xace.
 */
ExtensionEntry *
GetExtensionEntry(int major)
{
    if (major < EXTENSION_BASE)
        return NULL;
    major -= EXTENSION_BASE;
    if (major >= NumExtensions)
        return NULL;
    return extensions[major];
}

unsigned short
StandardMinorOpcode(ClientPtr client)
{
    return ((xReq *) client->requestBuffer)->data;
}

void
CloseDownExtensions(void)
{
    int i;

    for (i = NumExtensions - 1; i >= 0; i--) {
        if (extensions[i]->CloseDown)
            extensions[i]->CloseDown(extensions[i]);
        NumExtensions = i;
        free((void *) extensions[i]->name);
        dixFreePrivates(extensions[i]->devPrivates, PRIVATE_EXTENSION);
        free(extensions[i]);
    }
    free(extensions);
    extensions = (ExtensionEntry **) NULL;
    lastEvent = EXTENSION_EVENT_BASE;
    lastError = FirstExtensionError;
}

static Bool
ExtensionAvailable(ClientPtr client, ExtensionEntry *ext)
{
    if (XaceHook(XACE_EXT_ACCESS, client, ext) != Success)
        return FALSE;
    if (!ext->base)
        return FALSE;
    return TRUE;
}

int
ProcQueryExtension(ClientPtr client)
{
    xQueryExtensionReply reply;
    int i;

    REQUEST(xQueryExtensionReq);

    REQUEST_FIXED_SIZE(xQueryExtensionReq, stuff->nbytes);

    reply = (xQueryExtensionReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .major_opcode = 0
    };

    if (!NumExtensions)
        reply.present = xFalse;
    else {
        i = FindExtension((char *) &stuff[1], stuff->nbytes);
        if (i < 0 || !ExtensionAvailable(client, extensions[i]))
            reply.present = xFalse;
        else {
            reply.present = xTrue;
            reply.major_opcode = extensions[i]->base;
            reply.first_event = extensions[i]->eventBase;
            reply.first_error = extensions[i]->errorBase;
        }
    }
    WriteReplyToClient(client, sizeof(xQueryExtensionReply), &reply);
    return Success;
}

int
ProcListExtensions(ClientPtr client)
{
    xListExtensionsReply reply;
    char *bufptr, *buffer;
    int total_length = 0;

    REQUEST_SIZE_MATCH(xReq);

    reply = (xListExtensionsReply) {
        .type = X_Reply,
        .nExtensions = 0,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    buffer = NULL;

    if (NumExtensions) {
        int i;

        for (i = 0; i < NumExtensions; i++) {
            /* call callbacks to find out whether to show extension */
            if (!ExtensionAvailable(client, extensions[i]))
                continue;

            total_length += strlen(extensions[i]->name) + 1;
            reply.nExtensions += 1;
        }
        reply.length = bytes_to_int32(total_length);
        buffer = bufptr = malloc(total_length);
        if (!buffer)
            return BadAlloc;
        for (i = 0; i < NumExtensions; i++) {
            int len;

            if (!ExtensionAvailable(client, extensions[i]))
                continue;

            *bufptr++ = len = strlen(extensions[i]->name);
            memmove(bufptr, extensions[i]->name, len);
            bufptr += len;
        }
    }
    WriteReplyToClient(client, sizeof(xListExtensionsReply), &reply);
    if (reply.length)
        WriteToClient(client, total_length, buffer);

    free(buffer);
    return Success;
}
