/***********************************************************

Copyright 1987, 1989, 1998  The Open Group

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

Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

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
/*****************************************************************
 * i/o functions
 *
 *   WriteToClient, ReadRequestFromClient
 *   InsertFakeRequest, ResetCurrentRequest
 *
 *****************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#undef DEBUG_COMMUNICATION

#ifdef WIN32
#include <X11/Xwinsock.h>
#endif
#include <stdio.h>
#define XSERV_t
#define TRANS_SERVER
#define TRANS_REOPEN
#include <X11/Xtrans/Xtrans.h>
#include <X11/Xmd.h>
#include <errno.h>
#if !defined(WIN32)
#include <sys/uio.h>
#endif
#include <X11/X.h>
#include <X11/Xproto.h>
#include "os.h"
#include "osdep.h"
#include "opaque.h"
#include "dixstruct.h"
#include "misc.h"

CallbackListPtr ReplyCallback;
CallbackListPtr FlushCallback;

typedef struct _connectionInput {
    struct _connectionInput *next;
    char *buffer;               /* contains current client input */
    char *bufptr;               /* pointer to current start of data */
    int bufcnt;                 /* count of bytes in buffer */
    int lenLastReq;
    int size;
    unsigned int ignoreBytes;   /* bytes to ignore before the next request */
} ConnectionInput;

typedef struct _connectionOutput {
    struct _connectionOutput *next;
    unsigned char *buf;
    int size;
    int count;
} ConnectionOutput;

static ConnectionInputPtr AllocateInputBuffer(void);
static ConnectionOutputPtr AllocateOutputBuffer(void);

static Bool CriticalOutputPending;
static int timesThisConnection = 0;
static ConnectionInputPtr FreeInputs = (ConnectionInputPtr) NULL;
static ConnectionOutputPtr FreeOutputs = (ConnectionOutputPtr) NULL;
static OsCommPtr AvailableInput = (OsCommPtr) NULL;

#define get_req_len(req,cli) ((cli)->swapped ? \
			      bswap_16((req)->length) : (req)->length)

#include <X11/extensions/bigreqsproto.h>

#define get_big_req_len(req,cli) ((cli)->swapped ? \
				  bswap_32(((xBigReq *)(req))->length) : \
				  ((xBigReq *)(req))->length)

#define BUFSIZE 16384
#define BUFWATERMARK 32768

/*
 *   A lot of the code in this file manipulates a ConnectionInputPtr:
 *
 *    -----------------------------------------------
 *   |------- bufcnt ------->|           |           |
 *   |           |- gotnow ->|           |           |
 *   |           |-------- needed ------>|           |
 *   |-----------+--------- size --------+---------->|
 *    -----------------------------------------------
 *   ^           ^
 *   |           |
 *   buffer   bufptr
 *
 *  buffer is a pointer to the start of the buffer.
 *  bufptr points to the start of the current request.
 *  bufcnt counts how many bytes are in the buffer.
 *  size is the size of the buffer in bytes.
 *
 *  In several of the functions, gotnow and needed are local variables
 *  that do the following:
 *
 *  gotnow is the number of bytes of the request that we're
 *  trying to read that are currently in the buffer.
 *  Typically, gotnow = (buffer + bufcnt) - bufptr
 *
 *  needed = the length of the request that we're trying to
 *  read.  Watch out: needed sometimes counts bytes and sometimes
 *  counts CARD32's.
 */

/*****************************************************************
 * ReadRequestFromClient
 *    Returns one request in client->requestBuffer.  The request
 *    length will be in client->req_len.  Return status is:
 *
 *    > 0  if  successful, specifies length in bytes of the request
 *    = 0  if  entire request is not yet available
 *    < 0  if  client should be terminated
 *
 *    The request returned must be contiguous so that it can be
 *    cast in the dispatcher to the correct request type.  Because requests
 *    are variable length, ReadRequestFromClient() must look at the first 4
 *    or 8 bytes of a request to determine the length (the request length is
 *    in the 3rd and 4th bytes of the request unless it is a Big Request
 *    (see the Big Request Extension), in which case the 3rd and 4th bytes
 *    are zero and the following 4 bytes are the request length.
 *
 *    Note: in order to make the server scheduler (WaitForSomething())
 *    "fair", the ClientsWithInput mask is used.  This mask tells which
 *    clients have FULL requests left in their buffers.  Clients with
 *    partial requests require a read.  Basically, client buffers
 *    are drained before select() is called again.  But, we can't keep
 *    reading from a client that is sending buckets of data (or has
 *    a partial request) because others clients need to be scheduled.
 *****************************************************************/

static void
YieldControl(void)
{
    isItTimeToYield = TRUE;
    timesThisConnection = 0;
}

static void
YieldControlNoInput(ClientPtr client)
{
    OsCommPtr oc = client->osPrivate;
    YieldControl();
    if (oc->trans_conn)
        ospoll_reset_events(server_poll, oc->fd);
}

static void
YieldControlDeath(void)
{
    timesThisConnection = 0;
}

/* If an input buffer was empty, either free it if it is too big or link it
 * into our list of free input buffers.  This means that different clients can
 * share the same input buffer (at different times).  This was done to save
 * memory.
 */
static void
NextAvailableInput(OsCommPtr oc)
{
    if (AvailableInput) {
        if (AvailableInput != oc) {
            ConnectionInputPtr aci = AvailableInput->input;

            if (aci->size > BUFWATERMARK) {
                free(aci->buffer);
                free(aci);
            }
            else {
                aci->next = FreeInputs;
                FreeInputs = aci;
            }
            AvailableInput->input = NULL;
        }
        AvailableInput = NULL;
    }
}

int
ReadRequestFromClient(ClientPtr client)
{
    OsCommPtr oc = (OsCommPtr) client->osPrivate;
    ConnectionInputPtr oci = oc->input;
    unsigned int gotnow, needed;
    int result;
    register xReq *request;
    Bool need_header;
    Bool move_header;

    NextAvailableInput(oc);

    /* make sure we have an input buffer */

    if (!oci) {
        if ((oci = FreeInputs)) {
            FreeInputs = oci->next;
        }
        else if (!(oci = AllocateInputBuffer())) {
            YieldControlDeath();
            return -1;
        }
        oc->input = oci;
    }

#if XTRANS_SEND_FDS
    /* Discard any unused file descriptors */
    while (client->req_fds > 0) {
        int req_fd = ReadFdFromClient(client);
        if (req_fd >= 0)
            close(req_fd);
    }
#endif
    /* advance to start of next request */

    oci->bufptr += oci->lenLastReq;

    need_header = FALSE;
    move_header = FALSE;
    gotnow = oci->bufcnt + oci->buffer - oci->bufptr;

    if (oci->ignoreBytes > 0) {
        if (oci->ignoreBytes > oci->size)
            needed = oci->size;
        else
            needed = oci->ignoreBytes;
    }
    else if (gotnow < sizeof(xReq)) {
        /* We don't have an entire xReq yet.  Can't tell how big
         * the request will be until we get the whole xReq.
         */
        needed = sizeof(xReq);
        need_header = TRUE;
    }
    else {
        /* We have a whole xReq.  We can tell how big the whole
         * request will be unless it is a Big Request.
         */
        request = (xReq *) oci->bufptr;
        needed = get_req_len(request, client);
        if (!needed && client->big_requests) {
            /* It's a Big Request. */
            move_header = TRUE;
            if (gotnow < sizeof(xBigReq)) {
                /* Still need more data to tell just how big. */
                needed = bytes_to_int32(sizeof(xBigReq));       /* needed is in CARD32s now */
                need_header = TRUE;
            }
            else
                needed = get_big_req_len(request, client);
        }
        client->req_len = needed;
        needed <<= 2;           /* needed is in bytes now */
    }
    if (gotnow < needed) {
        /* Need to read more data, either so that we can get a
         * complete xReq (if need_header is TRUE), a complete
         * xBigReq (if move_header is TRUE), or the rest of the
         * request (if need_header and move_header are both FALSE).
         */

        oci->lenLastReq = 0;
        if (needed > maxBigRequestSize << 2) {
            /* request is too big for us to handle */
            /*
             * Mark the rest of it as needing to be ignored, and then return
             * the full size.  Dispatch() will turn it into a BadLength error.
             */
            oci->ignoreBytes = needed - gotnow;
            oci->lenLastReq = gotnow;
            return needed;
        }
        if ((gotnow == 0) || ((oci->bufptr - oci->buffer + needed) > oci->size)) {
            /* no data, or the request is too big to fit in the buffer */

            if ((gotnow > 0) && (oci->bufptr != oci->buffer))
                /* save the data we've already read */
                memmove(oci->buffer, oci->bufptr, gotnow);
            if (needed > oci->size) {
                /* make buffer bigger to accommodate request */
                char *ibuf;

                ibuf = (char *) realloc(oci->buffer, needed);
                if (!ibuf) {
                    YieldControlDeath();
                    return -1;
                }
                oci->size = needed;
                oci->buffer = ibuf;
            }
            oci->bufptr = oci->buffer;
            oci->bufcnt = gotnow;
        }
        /*  XXX this is a workaround.  This function is sometimes called
         *  after the trans_conn has been freed.  In this case trans_conn
         *  will be null.  Really ought to restructure things so that we
         *  never get here in those circumstances.
         */
        if (!oc->trans_conn) {
            /*  treat as if an error occurred on the read, which is what
             *  used to happen
             */
            YieldControlDeath();
            return -1;
        }
        result = _XSERVTransRead(oc->trans_conn, oci->buffer + oci->bufcnt,
                                 oci->size - oci->bufcnt);
        if (result <= 0) {
            if ((result < 0) && ETEST(errno)) {
                mark_client_not_ready(client);
#if defined(SVR4) && defined(__i386__) && !defined(__sun)
                if (0)
#endif
                {
                    YieldControlNoInput(client);
                    return 0;
                }
            }
            YieldControlDeath();
            return -1;
        }
        oci->bufcnt += result;
        gotnow += result;
        /* free up some space after huge requests */
        if ((oci->size > BUFWATERMARK) &&
            (oci->bufcnt < BUFSIZE) && (needed < BUFSIZE)) {
            char *ibuf;

            ibuf = (char *) realloc(oci->buffer, BUFSIZE);
            if (ibuf) {
                oci->size = BUFSIZE;
                oci->buffer = ibuf;
                oci->bufptr = ibuf + oci->bufcnt - gotnow;
            }
        }
        if (need_header && gotnow >= needed) {
            /* We wanted an xReq, now we've gotten it. */
            request = (xReq *) oci->bufptr;
            needed = get_req_len(request, client);
            if (!needed && client->big_requests) {
                move_header = TRUE;
                if (gotnow < sizeof(xBigReq))
                    needed = bytes_to_int32(sizeof(xBigReq));
                else
                    needed = get_big_req_len(request, client);
            }
            client->req_len = needed;
            needed <<= 2;
        }
        if (gotnow < needed) {
            /* Still don't have enough; punt. */
            YieldControlNoInput(client);
            return 0;
        }
    }
    if (needed == 0) {
        if (client->big_requests)
            needed = sizeof(xBigReq);
        else
            needed = sizeof(xReq);
    }

    /* If there are bytes to ignore, ignore them now. */

    if (oci->ignoreBytes > 0) {
        assert(needed == oci->ignoreBytes || needed == oci->size);
        /*
         * The _XSERVTransRead call above may return more or fewer bytes than we
         * want to ignore.  Ignore the smaller of the two sizes.
         */
        if (gotnow < needed) {
            oci->ignoreBytes -= gotnow;
            oci->bufptr += gotnow;
            gotnow = 0;
        }
        else {
            oci->ignoreBytes -= needed;
            oci->bufptr += needed;
            gotnow -= needed;
        }
        needed = 0;
    }

    oci->lenLastReq = needed;

    /*
     *  Check to see if client has at least one whole request in the
     *  buffer beyond the request we're returning to the caller.
     *  If there is only a partial request, treat like buffer
     *  is empty so that select() will be called again and other clients
     *  can get into the queue.
     */

    gotnow -= needed;
    if (!gotnow)
        AvailableInput = oc;
    if (move_header) {
        if (client->req_len < bytes_to_int32(sizeof(xBigReq) - sizeof(xReq))) {
            YieldControlDeath();
            return -1;
        }

        request = (xReq *) oci->bufptr;
        oci->bufptr += (sizeof(xBigReq) - sizeof(xReq));
        *(xReq *) oci->bufptr = *request;
        oci->lenLastReq -= (sizeof(xBigReq) - sizeof(xReq));
        client->req_len -= bytes_to_int32(sizeof(xBigReq) - sizeof(xReq));
    }
    client->requestBuffer = (void *) oci->bufptr;
#ifdef DEBUG_COMMUNICATION
    {
        xReq *req = client->requestBuffer;

        ErrorF("REQUEST: ClientIDX: %i, type: 0x%x data: 0x%x len: %i\n",
               client->index, req->reqType, req->data, req->length);
    }
#endif
    return needed;
}

int
ReadFdFromClient(ClientPtr client)
{
    int fd = -1;

#if XTRANS_SEND_FDS
    if (client->req_fds > 0) {
        OsCommPtr oc = (OsCommPtr) client->osPrivate;

        --client->req_fds;
        fd = _XSERVTransRecvFd(oc->trans_conn);
    } else
        LogMessage(X_ERROR, "Request asks for FD without setting req_fds\n");
#endif

    return fd;
}

int
WriteFdToClient(ClientPtr client, int fd, Bool do_close)
{
#if XTRANS_SEND_FDS
    OsCommPtr oc = (OsCommPtr) client->osPrivate;

    return _XSERVTransSendFd(oc->trans_conn, fd, do_close);
#else
    return -1;
#endif
}

/*****************************************************************
 * InsertFakeRequest
 *    Splice a consed up (possibly partial) request in as the next request.
 *
 **********************/

Bool
InsertFakeRequest(ClientPtr client, char *data, int count)
{
    OsCommPtr oc = (OsCommPtr) client->osPrivate;
    ConnectionInputPtr oci = oc->input;
    int gotnow, moveup;

    NextAvailableInput(oc);

    if (!oci) {
        if ((oci = FreeInputs))
            FreeInputs = oci->next;
        else if (!(oci = AllocateInputBuffer()))
            return FALSE;
        oc->input = oci;
    }
    oci->bufptr += oci->lenLastReq;
    oci->lenLastReq = 0;
    gotnow = oci->bufcnt + oci->buffer - oci->bufptr;
    if ((gotnow + count) > oci->size) {
        char *ibuf;

        ibuf = (char *) realloc(oci->buffer, gotnow + count);
        if (!ibuf)
            return FALSE;
        oci->size = gotnow + count;
        oci->buffer = ibuf;
        oci->bufptr = ibuf + oci->bufcnt - gotnow;
    }
    moveup = count - (oci->bufptr - oci->buffer);
    if (moveup > 0) {
        if (gotnow > 0)
            memmove(oci->bufptr + moveup, oci->bufptr, gotnow);
        oci->bufptr += moveup;
        oci->bufcnt += moveup;
    }
    memmove(oci->bufptr - count, data, count);
    oci->bufptr -= count;
    gotnow += count;
    if ((gotnow >= sizeof(xReq)) &&
        (gotnow >= (int) (get_req_len((xReq *) oci->bufptr, client) << 2)))
        mark_client_ready(client);
    else
        YieldControlNoInput(client);
    return TRUE;
}

/*****************************************************************
 * ResetRequestFromClient
 *    Reset to reexecute the current request, and yield.
 *
 **********************/

void
ResetCurrentRequest(ClientPtr client)
{
    OsCommPtr oc = (OsCommPtr) client->osPrivate;

    /* ignore dying clients */
    if (!oc)
        return;

    register ConnectionInputPtr oci = oc->input;
    register xReq *request;
    int gotnow, needed;

    if (AvailableInput == oc)
        AvailableInput = (OsCommPtr) NULL;
    oci->lenLastReq = 0;
    gotnow = oci->bufcnt + oci->buffer - oci->bufptr;
    if (gotnow < sizeof(xReq)) {
        YieldControlNoInput(client);
    }
    else {
        request = (xReq *) oci->bufptr;
        needed = get_req_len(request, client);
        if (!needed && client->big_requests) {
            oci->bufptr -= sizeof(xBigReq) - sizeof(xReq);
            *(xReq *) oci->bufptr = *request;
            ((xBigReq *) oci->bufptr)->length = client->req_len;
            if (client->swapped) {
                swapl(&((xBigReq *) oci->bufptr)->length);
            }
        }
        if (gotnow >= (needed << 2)) {
            if (listen_to_client(client))
                mark_client_ready(client);
            YieldControl();
        }
        else
            YieldControlNoInput(client);
    }
}

 /********************
 * FlushAllOutput()
 *    Flush all clients with output.  However, if some client still
 *    has input in the queue (more requests), then don't flush.  This
 *    will prevent the output queue from being flushed every time around
 *    the round robin queue.  Now, some say that it SHOULD be flushed
 *    every time around, but...
 *
 **********************/

void
FlushAllOutput(void)
{
    OsCommPtr oc;
    register ClientPtr client, tmp;
    Bool newoutput = NewOutputPending;

    if (!newoutput)
        return;

    /*
     * It may be that some client still has critical output pending,
     * but he is not yet ready to receive it anyway, so we will
     * simply wait for the select to tell us when he's ready to receive.
     */
    CriticalOutputPending = FALSE;
    NewOutputPending = FALSE;

    xorg_list_for_each_entry_safe(client, tmp, &output_pending_clients, output_pending) {
        if (client->clientGone)
            continue;
        if (!client_is_ready(client)) {
            oc = (OsCommPtr) client->osPrivate;
            (void) FlushClient(client, oc, (char *) NULL, 0);
        } else
            NewOutputPending = TRUE;
    }
}

void
FlushIfCriticalOutputPending(void)
{
    if (CriticalOutputPending)
        FlushAllOutput();
}

void
SetCriticalOutputPending(void)
{
    CriticalOutputPending = TRUE;
}

/*****************
 * AbortClient:
 *    When a write error occurs to a client, close
 *    the connection and clean things up. Mark
 *    the client as 'ready' so that the server will
 *    try to read from it again, notice that the fd is
 *    closed and clean up from there.
 *****************/

static void
AbortClient(ClientPtr client)
{
    OsCommPtr oc = client->osPrivate;

    if (oc->trans_conn) {
        CloseDownFileDescriptor(oc);
        mark_client_ready(client);
    }
}

/*****************
 * WriteToClient
 *    Copies buf into ClientPtr.buf if it fits (with padding), else
 *    flushes ClientPtr.buf and buf to client.  As of this writing,
 *    every use of WriteToClient is cast to void, and the result
 *    is ignored.  Potentially, this could be used by requests
 *    that are sending several chunks of data and want to break
 *    out of a loop on error.  Thus, we will leave the type of
 *    this routine as int.
 *****************/

int
WriteToClient(ClientPtr who, int count, const void *__buf)
{
    OsCommPtr oc;
    ConnectionOutputPtr oco;
    int padBytes;
    const char *buf = __buf;

    BUG_RETURN_VAL_MSG(in_input_thread(), 0,
                       "******** %s called from input thread *********\n", __func__);

#ifdef DEBUG_COMMUNICATION
    Bool multicount = FALSE;
#endif
    if (!count || !who || who == serverClient || who->clientGone)
        return 0;
    oc = who->osPrivate;
    oco = oc->output;
#ifdef DEBUG_COMMUNICATION
    {
        char info[128];
        xError *err;
        xGenericReply *rep;
        xEvent *ev;

        if (!who->replyBytesRemaining) {
            switch (buf[0]) {
            case X_Reply:
                rep = (xGenericReply *) buf;
                if (rep->sequenceNumber == who->sequence) {
                    snprintf(info, 127, "Xreply: type: 0x%x data: 0x%x "
                             "len: %i seq#: 0x%x", rep->type, rep->data1,
                             rep->length, rep->sequenceNumber);
                    multicount = TRUE;
                }
                break;
            case X_Error:
                err = (xError *) buf;
                snprintf(info, 127, "Xerror: Code: 0x%x resID: 0x%x maj: 0x%x "
                         "min: %x", err->errorCode, err->resourceID,
                         err->minorCode, err->majorCode);
                break;
            default:
                if ((buf[0] & 0x7f) == KeymapNotify)
                    snprintf(info, 127, "KeymapNotifyEvent: %i", buf[0]);
                else {
                    ev = (xEvent *) buf;
                    snprintf(info, 127, "XEvent: type: 0x%x detail: 0x%x "
                             "seq#: 0x%x", ev->u.u.type, ev->u.u.detail,
                             ev->u.u.sequenceNumber);
                }
            }
            ErrorF("REPLY: ClientIDX: %i %s\n", who->index, info);
        }
        else
            multicount = TRUE;
    }
#endif

    if (!oco) {
        if ((oco = FreeOutputs)) {
            FreeOutputs = oco->next;
        }
        else if (!(oco = AllocateOutputBuffer())) {
            AbortClient(who);
            MarkClientException(who);
            return -1;
        }
        oc->output = oco;
    }

    padBytes = padding_for_int32(count);

    if (ReplyCallback) {
        ReplyInfoRec replyinfo;

        replyinfo.client = who;
        replyinfo.replyData = buf;
        replyinfo.dataLenBytes = count + padBytes;
        replyinfo.padBytes = padBytes;
        if (who->replyBytesRemaining) { /* still sending data of an earlier reply */
            who->replyBytesRemaining -= count + padBytes;
            replyinfo.startOfReply = FALSE;
            replyinfo.bytesRemaining = who->replyBytesRemaining;
            CallCallbacks((&ReplyCallback), (void *) &replyinfo);
        }
        else if (who->clientState == ClientStateRunning && buf[0] == X_Reply) { /* start of new reply */
            CARD32 replylen;
            unsigned long bytesleft;

            replylen = ((const xGenericReply *) buf)->length;
            if (who->swapped)
                swapl(&replylen);
            bytesleft = (replylen * 4) + SIZEOF(xReply) - count - padBytes;
            replyinfo.startOfReply = TRUE;
            replyinfo.bytesRemaining = who->replyBytesRemaining = bytesleft;
            CallCallbacks((&ReplyCallback), (void *) &replyinfo);
        }
    }
#ifdef DEBUG_COMMUNICATION
    else if (multicount) {
        if (who->replyBytesRemaining) {
            who->replyBytesRemaining -= (count + padBytes);
        }
        else {
            CARD32 replylen;

            replylen = ((xGenericReply *) buf)->length;
            who->replyBytesRemaining =
                (replylen * 4) + SIZEOF(xReply) - count - padBytes;
        }
    }
#endif
    if (oco->count == 0 || oco->count + count + padBytes > oco->size) {
        output_pending_clear(who);
        if (!any_output_pending()) {
            CriticalOutputPending = FALSE;
            NewOutputPending = FALSE;
        }

        return FlushClient(who, oc, buf, count);
    }

    NewOutputPending = TRUE;
    output_pending_mark(who);
    memmove((char *) oco->buf + oco->count, buf, count);
    oco->count += count;
    if (padBytes) {
        memset(oco->buf + oco->count, '\0', padBytes);
        oco->count += padBytes;
    }
    return count;
}

 /********************
 * FlushClient()
 *    If the client isn't keeping up with us, then we try to continue
 *    buffering the data and set the appropriate bit in ClientsWritable
 *    (which is used by WaitFor in the select).  If the connection yields
 *    a permanent error, or we can't allocate any more space, we then
 *    close the connection.
 *
 **********************/

int
FlushClient(ClientPtr who, OsCommPtr oc, const void *__extraBuf, int extraCount)
{
    ConnectionOutputPtr oco = oc->output;
    XtransConnInfo trans_conn = oc->trans_conn;
    struct iovec iov[3];
    static char padBuffer[3];
    const char *extraBuf = __extraBuf;
    long written;
    long padsize;
    long notWritten;
    long todo;

    if (!oco)
	return 0;
    written = 0;
    padsize = padding_for_int32(extraCount);
    notWritten = oco->count + extraCount + padsize;
    if (!notWritten)
        return 0;

    if (FlushCallback)
        CallCallbacks(&FlushCallback, who);

    todo = notWritten;
    while (notWritten) {
        long before = written;  /* amount of whole thing written */
        long remain = todo;     /* amount to try this time, <= notWritten */
        int i = 0;
        long len;

        /* You could be very general here and have "in" and "out" iovecs
         * and write a loop without using a macro, but what the heck.  This
         * translates to:
         *
         *     how much of this piece is new?
         *     if more new then we are trying this time, clamp
         *     if nothing new
         *         then bump down amount already written, for next piece
         *         else put new stuff in iovec, will need all of next piece
         *
         * Note that todo had better be at least 1 or else we'll end up
         * writing 0 iovecs.
         */
#define InsertIOV(pointer, length) \
	len = (length) - before; \
	if (len > remain) \
	    len = remain; \
	if (len <= 0) { \
	    before = (-len); \
	} else { \
	    iov[i].iov_len = len; \
	    iov[i].iov_base = (pointer) + before;	\
	    i++; \
	    remain -= len; \
	    before = 0; \
	}

        InsertIOV((char *) oco->buf, oco->count)
            InsertIOV((char *) extraBuf, extraCount)
            InsertIOV(padBuffer, padsize)

            errno = 0;
        if (trans_conn && (len = _XSERVTransWritev(trans_conn, iov, i)) >= 0) {
            written += len;
            notWritten -= len;
            todo = notWritten;
        }
        else if (ETEST(errno)
#ifdef SUNSYSV                  /* check for another brain-damaged OS bug */
                 || (errno == 0)
#endif
#ifdef EMSGSIZE                 /* check for another brain-damaged OS bug */
                 || ((errno == EMSGSIZE) && (todo == 1))
#endif
            ) {
            /* If we've arrived here, then the client is stuffed to the gills
               and not ready to accept more.  Make a note of it and buffer
               the rest. */
            output_pending_mark(who);

            if (written < oco->count) {
                if (written > 0) {
                    oco->count -= written;
                    memmove((char *) oco->buf,
                            (char *) oco->buf + written, oco->count);
                    written = 0;
                }
            }
            else {
                written -= oco->count;
                oco->count = 0;
            }

            if (notWritten > oco->size) {
                unsigned char *obuf = NULL;

                if (notWritten + BUFSIZE <= INT_MAX) {
                    obuf = realloc(oco->buf, notWritten + BUFSIZE);
                }
                if (!obuf) {
                    AbortClient(who);
                    MarkClientException(who);
                    oco->count = 0;
                    return -1;
                }
                oco->size = notWritten + BUFSIZE;
                oco->buf = obuf;
            }

            /* If the amount written extended into the padBuffer, then the
               difference "extraCount - written" may be less than 0 */
            if ((len = extraCount - written) > 0)
                memmove((char *) oco->buf + oco->count,
                        extraBuf + written, len);

            oco->count = notWritten;    /* this will include the pad */
            ospoll_listen(server_poll, oc->fd, X_NOTIFY_WRITE);

            /* return only the amount explicitly requested */
            return extraCount;
        }
#ifdef EMSGSIZE                 /* check for another brain-damaged OS bug */
        else if (errno == EMSGSIZE) {
            todo >>= 1;
        }
#endif
        else {
            AbortClient(who);
            MarkClientException(who);
            oco->count = 0;
            return -1;
        }
    }

    /* everything was flushed out */
    oco->count = 0;
    output_pending_clear(who);

    if (oco->size > BUFWATERMARK) {
        free(oco->buf);
        free(oco);
    }
    else {
        oco->next = FreeOutputs;
        FreeOutputs = oco;
    }
    oc->output = (ConnectionOutputPtr) NULL;
    return extraCount;          /* return only the amount explicitly requested */
}

static ConnectionInputPtr
AllocateInputBuffer(void)
{
    ConnectionInputPtr oci;

    oci = malloc(sizeof(ConnectionInput));
    if (!oci)
        return NULL;
    oci->buffer = malloc(BUFSIZE);
    if (!oci->buffer) {
        free(oci);
        return NULL;
    }
    oci->size = BUFSIZE;
    oci->bufptr = oci->buffer;
    oci->bufcnt = 0;
    oci->lenLastReq = 0;
    oci->ignoreBytes = 0;
    return oci;
}

static ConnectionOutputPtr
AllocateOutputBuffer(void)
{
    ConnectionOutputPtr oco;

    oco = malloc(sizeof(ConnectionOutput));
    if (!oco)
        return NULL;
    oco->buf = calloc(1, BUFSIZE);
    if (!oco->buf) {
        free(oco);
        return NULL;
    }
    oco->size = BUFSIZE;
    oco->count = 0;
    return oco;
}

void
FreeOsBuffers(OsCommPtr oc)
{
    ConnectionInputPtr oci;
    ConnectionOutputPtr oco;

    if (AvailableInput == oc)
        AvailableInput = (OsCommPtr) NULL;
    if ((oci = oc->input)) {
        if (FreeInputs) {
            free(oci->buffer);
            free(oci);
        }
        else {
            FreeInputs = oci;
            oci->next = (ConnectionInputPtr) NULL;
            oci->bufptr = oci->buffer;
            oci->bufcnt = 0;
            oci->lenLastReq = 0;
            oci->ignoreBytes = 0;
        }
    }
    if ((oco = oc->output)) {
        if (FreeOutputs) {
            free(oco->buf);
            free(oco);
        }
        else {
            FreeOutputs = oco;
            oco->next = (ConnectionOutputPtr) NULL;
            oco->count = 0;
        }
    }
}

void
ResetOsBuffers(void)
{
    ConnectionInputPtr oci;
    ConnectionOutputPtr oco;

    while ((oci = FreeInputs)) {
        FreeInputs = oci->next;
        free(oci->buffer);
        free(oci);
    }
    while ((oco = FreeOutputs)) {
        FreeOutputs = oco->next;
        free(oco->buf);
        free(oco);
    }
}
