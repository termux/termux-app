
/*

Copyright 1995, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

Author: David P. Wiggins, The Open Group

This work benefited from earlier work done by Martha Zimet of NCD
and Jim Haggerty of Metheus.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "dixstruct.h"
#include "extnsionst.h"
#include "extinit.h"
#include <X11/extensions/recordproto.h>
#include "set.h"
#include "swaprep.h"
#include "inputstr.h"
#include "eventconvert.h"
#include "scrnintstr.h"

#include <stdio.h>
#include <assert.h>

#ifdef PANORAMIX
#include "globals.h"
#include "panoramiX.h"
#include "panoramiXsrv.h"
#include "cursor.h"
#endif

#include "protocol-versions.h"

static RESTYPE RTContext;       /* internal resource type for Record contexts */

/* How many bytes of protocol data to buffer in a context. Don't set to less
 * than 32.
 */
#define REPLY_BUF_SIZE 1024

/* Record Context structure */

typedef struct {
    XID id;                     /* resource id of context */
    ClientPtr pRecordingClient; /* client that has context enabled */
    struct _RecordClientsAndProtocolRec *pListOfRCAP;   /* all registered info */
    ClientPtr pBufClient;       /* client whose protocol is in replyBuffer */
    unsigned int continuedReply:1;      /* recording a reply that is split up? */
    char elemHeaders;           /* element header flags (time/seq no.) */
    char bufCategory;           /* category of protocol in replyBuffer */
    int numBufBytes;            /* number of bytes in replyBuffer */
    char replyBuffer[REPLY_BUF_SIZE];   /* buffered recorded protocol */
    int inFlush;                /*  are we inside RecordFlushReplyBuffer */
} RecordContextRec, *RecordContextPtr;

/*  RecordMinorOpRec - to hold minor opcode selections for extension requests
 *  and replies
 */

typedef union {
    int count;                  /* first element of array: how many "major" structs to follow */
    struct {                    /* rest of array elements are this */
        short first;            /* first major opcode */
        short last;             /* last major opcode */
        RecordSetPtr pMinOpSet; /*  minor opcode set for above major range */
    } major;
} RecordMinorOpRec, *RecordMinorOpPtr;

/*  RecordClientsAndProtocolRec, nicknamed RCAP - holds all the client and
 *  protocol selections passed in a single CreateContext or RegisterClients.
 *  Generally, a context will have one of these from the create and an
 *  additional one for each RegisterClients.  RCAPs are freed when all their
 *  clients are unregistered.
 */

typedef struct _RecordClientsAndProtocolRec {
    RecordContextPtr pContext;  /* context that owns this RCAP */
    struct _RecordClientsAndProtocolRec *pNextRCAP;     /* next RCAP on context */
    RecordSetPtr pRequestMajorOpSet;    /* requests to record */
    RecordMinorOpPtr pRequestMinOpInfo; /* extension requests to record */
    RecordSetPtr pReplyMajorOpSet;      /* replies to record */
    RecordMinorOpPtr pReplyMinOpInfo;   /* extension replies to record */
    RecordSetPtr pDeviceEventSet;       /* device events to record */
    RecordSetPtr pDeliveredEventSet;    /* delivered events to record */
    RecordSetPtr pErrorSet;     /* errors to record */
    XID *pClientIDs;            /* array of clients to record */
    short numClients;           /* number of clients in pClientIDs */
    short sizeClients;          /* size of pClientIDs array */
    unsigned int clientStarted:1;       /* record new client connections? */
    unsigned int clientDied:1;  /* record client disconnections? */
    unsigned int clientIDsSeparatelyAllocated:1;        /* pClientIDs malloced? */
} RecordClientsAndProtocolRec, *RecordClientsAndProtocolPtr;

/* how much bigger to make pRCAP->pClientIDs when reallocing */
#define CLIENT_ARRAY_GROWTH_INCREMENT 4

/* counts the total number of RCAPs belonging to enabled contexts. */
static int numEnabledRCAPs;

/*  void VERIFY_CONTEXT(RecordContextPtr, XID, ClientPtr)
 *  In the spirit of the VERIFY_* macros in dix.h, this macro fills in
 *  the context pointer if the given ID is a valid Record Context, else it
 *  returns an error.
 */
#define VERIFY_CONTEXT(_pContext, _contextid, _client) { \
    int rc = dixLookupResourceByType((void **)&(_pContext), _contextid, \
                                     RTContext, _client, DixUseAccess); \
    if (rc != Success) \
	return rc; \
}

static int RecordDeleteContext(void     *value,
                               XID      id);

/***************************************************************************/

/* client private stuff */

/*  To make declarations less obfuscated, have a typedef for a pointer to a
 *  Proc function.
 */
typedef int (*ProcFunctionPtr) (ClientPtr       /*pClient */
    );

/* Record client private.  Generally a client only has one of these if
 * any of its requests are being recorded.
 */
typedef struct {
/* ptr to client's proc vector before Record stuck its nose in */
    ProcFunctionPtr *originalVector;

/* proc vector with pointers for recorded requests redirected to the
 * function RecordARequest
 */
    ProcFunctionPtr recordVector[256];
} RecordClientPrivateRec, *RecordClientPrivatePtr;

static DevPrivateKeyRec RecordClientPrivateKeyRec;

#define RecordClientPrivateKey (&RecordClientPrivateKeyRec)

/*  RecordClientPrivatePtr RecordClientPrivate(ClientPtr)
 *  gets the client private of the given client.  Syntactic sugar.
 */
#define RecordClientPrivate(_pClient) (RecordClientPrivatePtr) \
    dixLookupPrivate(&(_pClient)->devPrivates, RecordClientPrivateKey)

/***************************************************************************/

/* global list of all contexts */

static RecordContextPtr *ppAllContexts;

static int numContexts;         /* number of contexts in ppAllContexts */

/* number of currently enabled contexts.  All enabled contexts are bunched
 * up at the front of the ppAllContexts array, from ppAllContexts[0] to
 * ppAllContexts[numEnabledContexts-1], to eliminate time spent skipping
 * past disabled contexts.
 */
static int numEnabledContexts;

/* RecordFindContextOnAllContexts
 *
 * Arguments:
 *	pContext is the context to search for.
 *
 * Returns:
 *	The index into the array ppAllContexts at which pContext is stored.
 *	If pContext is not found in ppAllContexts, returns -1.
 *
 * Side Effects: none.
 */
static int
RecordFindContextOnAllContexts(RecordContextPtr pContext)
{
    int i;

    assert(numContexts >= numEnabledContexts);
    for (i = 0; i < numContexts; i++) {
        if (ppAllContexts[i] == pContext)
            return i;
    }
    return -1;
}                               /* RecordFindContextOnAllContexts */

/***************************************************************************/

/* RecordFlushReplyBuffer
 *
 * Arguments:
 *	pContext is the context to flush.
 *	data1 is a pointer to additional data, and len1 is its length in bytes.
 *	data2 is a pointer to additional data, and len2 is its length in bytes.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	If the context is enabled, any buffered (recorded) protocol is written
 *	to the recording client, and the number of buffered bytes is set to
 *	zero.  If len1 is not zero, data1/len1 are then written to the
 *	recording client, and similarly for data2/len2 (written after
 *	data1/len1).
 */
static void
RecordFlushReplyBuffer(RecordContextPtr pContext,
                       void *data1, int len1, void *data2, int len2)
{
    if (!pContext->pRecordingClient || pContext->pRecordingClient->clientGone ||
        pContext->inFlush)
        return;
    ++pContext->inFlush;
    if (pContext->numBufBytes)
        WriteToClient(pContext->pRecordingClient, pContext->numBufBytes,
                      pContext->replyBuffer);
    pContext->numBufBytes = 0;
    if (len1)
        WriteToClient(pContext->pRecordingClient, len1, data1);
    if (len2)
        WriteToClient(pContext->pRecordingClient, len2, data2);
    --pContext->inFlush;
}                               /* RecordFlushReplyBuffer */

/* RecordAProtocolElement
 *
 * Arguments:
 *	pContext is the context that is recording a protocol element.
 *	pClient is the client whose protocol is being recorded.  For
 *	  device events and EndOfData, pClient is NULL.
 *	category is the category of the protocol element, as defined
 *	  by the RECORD spec.
 *	data is a pointer to the protocol data, and datalen - padlen
 *	  is its length in bytes.
 *	padlen is the number of pad bytes from a zeroed array.
 *	futurelen is the number of bytes that will be sent in subsequent
 *	  calls to this function to complete this protocol element.
 *	  In those subsequent calls, futurelen will be -1 to indicate
 *	  that the current data is a continuation of the same protocol
 *	  element.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	The context may be flushed.  The new protocol element will be
 *	added to the context's protocol buffer with appropriate element
 *	headers prepended (sequence number and timestamp).  If the data
 *	is continuation data (futurelen == -1), element headers won't
 *	be added.  If the protocol element and headers won't fit in
 *	the context's buffer, it is sent directly to the recording
 *	client (after any buffered data).
 */
static void
RecordAProtocolElement(RecordContextPtr pContext, ClientPtr pClient,
                       int category, void *data, int datalen, int padlen,
                       int futurelen)
{
    CARD32 elemHeaderData[2];
    int numElemHeaders = 0;
    Bool recordingClientSwapped = pContext->pRecordingClient->swapped;
    CARD32 serverTime = 0;
    Bool gotServerTime = FALSE;
    int replylen;

    if (futurelen >= 0) {       /* start of new protocol element */
        xRecordEnableContextReply *pRep = (xRecordEnableContextReply *)
            pContext->replyBuffer;

        if (pContext->pBufClient != pClient ||
            pContext->bufCategory != category) {
            RecordFlushReplyBuffer(pContext, NULL, 0, NULL, 0);
            pContext->pBufClient = pClient;
            pContext->bufCategory = category;
        }

        if (!pContext->numBufBytes) {
            serverTime = GetTimeInMillis();
            gotServerTime = TRUE;
            pRep->type = X_Reply;
            pRep->category = category;
            pRep->sequenceNumber = pContext->pRecordingClient->sequence;
            pRep->length = 0;
            pRep->elementHeader = pContext->elemHeaders;
            pRep->serverTime = serverTime;
            if (pClient) {
                pRep->clientSwapped =
                    (pClient->swapped != recordingClientSwapped);
                pRep->idBase = pClient->clientAsMask;
                pRep->recordedSequenceNumber = pClient->sequence;
            }
            else {              /* it's a device event, StartOfData, or EndOfData */

                pRep->clientSwapped = (category != XRecordFromServer) &&
                    recordingClientSwapped;
                pRep->idBase = 0;
                pRep->recordedSequenceNumber = 0;
            }

            if (recordingClientSwapped) {
                swaps(&pRep->sequenceNumber);
                swapl(&pRep->length);
                swapl(&pRep->idBase);
                swapl(&pRep->serverTime);
                swapl(&pRep->recordedSequenceNumber);
            }
            pContext->numBufBytes = SIZEOF(xRecordEnableContextReply);
        }

        /* generate element headers if needed */

        if (((pContext->elemHeaders & XRecordFromClientTime)
             && category == XRecordFromClient)
            || ((pContext->elemHeaders & XRecordFromServerTime)
                && category == XRecordFromServer)) {
            if (gotServerTime)
                elemHeaderData[numElemHeaders] = serverTime;
            else
                elemHeaderData[numElemHeaders] = GetTimeInMillis();
            if (recordingClientSwapped)
                swapl(&elemHeaderData[numElemHeaders]);
            numElemHeaders++;
        }

        if ((pContext->elemHeaders & XRecordFromClientSequence)
            && (category == XRecordFromClient || category == XRecordClientDied)) {
            elemHeaderData[numElemHeaders] = pClient->sequence;
            if (recordingClientSwapped)
                swapl(&elemHeaderData[numElemHeaders]);
            numElemHeaders++;
        }

        /* adjust reply length */

        replylen = pRep->length;
        if (recordingClientSwapped)
            swapl(&replylen);
        replylen += numElemHeaders + bytes_to_int32(datalen) +
            bytes_to_int32(futurelen);
        if (recordingClientSwapped)
            swapl(&replylen);
        pRep->length = replylen;
    }                           /* end if not continued reply */

    numElemHeaders *= 4;

    /* if space available >= space needed, buffer the data */

    if (REPLY_BUF_SIZE - pContext->numBufBytes >= datalen + numElemHeaders) {
        if (numElemHeaders) {
            memcpy(pContext->replyBuffer + pContext->numBufBytes,
                   elemHeaderData, numElemHeaders);
            pContext->numBufBytes += numElemHeaders;
        }
        if (datalen) {
            static char padBuffer[3];   /* as in FlushClient */

            memcpy(pContext->replyBuffer + pContext->numBufBytes,
                   data, datalen - padlen);
            pContext->numBufBytes += datalen - padlen;
            memcpy(pContext->replyBuffer + pContext->numBufBytes,
                   padBuffer, padlen);
            pContext->numBufBytes += padlen;
        }
    }
    else {
        RecordFlushReplyBuffer(pContext, (void *) elemHeaderData,
                               numElemHeaders, (void *) data,
                               datalen - padlen);
    }
}                               /* RecordAProtocolElement */

/* RecordFindClientOnContext
 *
 * Arguments:
 *	pContext is the context to search.
 *	clientspec is the resource ID mask identifying the client to search
 *	  for, or XRecordFutureClients.
 *	pposition is a pointer to an int, or NULL.  See Returns.
 *
 * Returns:
 *	The RCAP on which clientspec was found, or NULL if not found on
 *	any RCAP on the given context.
 *	If pposition was not NULL and the returned RCAP is not NULL,
 *	*pposition will be set to the index into the returned the RCAP's
 *	pClientIDs array that holds clientspec.
 *
 * Side Effects: none.
 */
static RecordClientsAndProtocolPtr
RecordFindClientOnContext(RecordContextPtr pContext,
                          XID clientspec, int *pposition)
{
    RecordClientsAndProtocolPtr pRCAP;

    for (pRCAP = pContext->pListOfRCAP; pRCAP; pRCAP = pRCAP->pNextRCAP) {
        int i;

        for (i = 0; i < pRCAP->numClients; i++) {
            if (pRCAP->pClientIDs[i] == clientspec) {
                if (pposition)
                    *pposition = i;
                return pRCAP;
            }
        }
    }
    return NULL;
}                               /* RecordFindClientOnContext */

/* RecordABigRequest
 *
 * Arguments:
 *	pContext is the recording context.
 *	client is the client being recorded.
 *	stuff is a pointer to the big request of client (see the Big Requests
 *	extension for details.)
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	The big request is recorded with the correct length field re-inserted.
 *	
 * Note: this function exists mainly to make RecordARequest smaller.
 */
static void
RecordABigRequest(RecordContextPtr pContext, ClientPtr client, xReq * stuff)
{
    CARD32 bigLength;
    int bytesLeft;

    /* note: client->req_len has been frobbed by ReadRequestFromClient
     * (os/io.c) to discount the extra 4 bytes taken by the extended length
     * field in a big request.  The actual request length to record is
     * client->req_len + 1 (measured in CARD32s).
     */

    /* record the request header */
    bytesLeft = client->req_len << 2;
    RecordAProtocolElement(pContext, client, XRecordFromClient,
                           (void *) stuff, SIZEOF(xReq), 0, bytesLeft);

    /* reinsert the extended length field that was squished out */
    bigLength = client->req_len + bytes_to_int32(sizeof(bigLength));
    if (client->swapped)
        swapl(&bigLength);
    RecordAProtocolElement(pContext, client, XRecordFromClient,
                           (void *) &bigLength, sizeof(bigLength), 0,
                           /* continuation */ -1);
    bytesLeft -= sizeof(bigLength);

    /* record the rest of the request after the length */
    RecordAProtocolElement(pContext, client, XRecordFromClient,
                           (void *) (stuff + 1), bytesLeft, 0,
                           /* continuation */ -1);
}                               /* RecordABigRequest */

/* RecordARequest
 *
 * Arguments:
 *	client is a client that the server has dispatched a request to by
 *	calling client->requestVector[request opcode] .
 *	The request is in client->requestBuffer.
 *
 * Returns:
 *	Whatever is returned by the "real" Proc function for this request.
 *	The "real" Proc function is the function that was in
 *	client->requestVector[request opcode]  before it was replaced by
 *	RecordARequest.  (See the function RecordInstallHooks.)
 *
 * Side Effects:
 *	The request is recorded by all contexts that have registered this
 *	request for this client.  The real Proc function is called.
 */
static int
RecordARequest(ClientPtr client)
{
    RecordContextPtr pContext;
    RecordClientsAndProtocolPtr pRCAP;
    int i;
    RecordClientPrivatePtr pClientPriv;

    REQUEST(xReq);
    int majorop;

    majorop = stuff->reqType;
    for (i = 0; i < numEnabledContexts; i++) {
        pContext = ppAllContexts[i];
        pRCAP = RecordFindClientOnContext(pContext, client->clientAsMask, NULL);
        if (pRCAP && pRCAP->pRequestMajorOpSet &&
            RecordIsMemberOfSet(pRCAP->pRequestMajorOpSet, majorop)) {
            if (majorop <= 127) {       /* core request */

                if (stuff->length == 0)
                    RecordABigRequest(pContext, client, stuff);
                else
                    RecordAProtocolElement(pContext, client, XRecordFromClient,
                                           (void *) stuff,
                                           client->req_len << 2, 0, 0);
            }
            else {              /* extension, check minor opcode */

                int minorop = client->minorOp;
                int numMinOpInfo;
                RecordMinorOpPtr pMinorOpInfo = pRCAP->pRequestMinOpInfo;

                assert(pMinorOpInfo);
                numMinOpInfo = pMinorOpInfo->count;
                pMinorOpInfo++;
                assert(numMinOpInfo);
                for (; numMinOpInfo; numMinOpInfo--, pMinorOpInfo++) {
                    if (majorop >= pMinorOpInfo->major.first &&
                        majorop <= pMinorOpInfo->major.last &&
                        RecordIsMemberOfSet(pMinorOpInfo->major.pMinOpSet,
                                            minorop)) {
                        if (stuff->length == 0)
                            RecordABigRequest(pContext, client, stuff);
                        else
                            RecordAProtocolElement(pContext, client,
                                                   XRecordFromClient,
                                                   (void *) stuff,
                                                   client->req_len << 2, 0, 0);
                        break;
                    }
                }               /* end for each minor op info */
            }                   /* end extension request */
        }                       /* end this RCAP wants this major opcode */
    }                           /* end for each context */
    pClientPriv = RecordClientPrivate(client);
    assert(pClientPriv);
    return (*pClientPriv->originalVector[majorop]) (client);
}                               /* RecordARequest */

/* RecordAReply
 *
 * Arguments:
 *	pcbl is &ReplyCallback.
 *	nulldata is NULL.
 *	calldata is a pointer to a ReplyInfoRec (include/os.h)
 *	  which provides information about replies that are being sent
 *	  to clients.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	The reply is recorded by all contexts that have registered this
 *	reply type for this client.  If more data belonging to the same
 *	reply is expected, and if the reply is being recorded by any
 *	context, pContext->continuedReply is set to 1.
 *	If pContext->continuedReply was already 1 and this is the last
 *	chunk of data belonging to this reply, it is set to 0.
 */
static void
RecordAReply(CallbackListPtr *pcbl, void *nulldata, void *calldata)
{
    RecordContextPtr pContext;
    RecordClientsAndProtocolPtr pRCAP;
    int eci;
    ReplyInfoRec *pri = (ReplyInfoRec *) calldata;
    ClientPtr client = pri->client;

    for (eci = 0; eci < numEnabledContexts; eci++) {
        pContext = ppAllContexts[eci];
        pRCAP = RecordFindClientOnContext(pContext, client->clientAsMask, NULL);
        if (pRCAP) {
            int majorop = client->majorOp;

            if (pContext->continuedReply) {
                RecordAProtocolElement(pContext, client, XRecordFromServer,
                                       (void *) pri->replyData,
                                       pri->dataLenBytes, pri->padBytes,
                                       /* continuation */ -1);
                if (!pri->bytesRemaining)
                    pContext->continuedReply = 0;
            }
            else if (pri->startOfReply && pRCAP->pReplyMajorOpSet &&
                     RecordIsMemberOfSet(pRCAP->pReplyMajorOpSet, majorop)) {
                if (majorop <= 127) {   /* core reply */
                    RecordAProtocolElement(pContext, client, XRecordFromServer,
                                           (void *) pri->replyData,
                                           pri->dataLenBytes, 0,
                                           pri->bytesRemaining);
                    if (pri->bytesRemaining)
                        pContext->continuedReply = 1;
                }
                else {          /* extension, check minor opcode */

                    int minorop = client->minorOp;
                    int numMinOpInfo;
                    RecordMinorOpPtr pMinorOpInfo = pRCAP->pReplyMinOpInfo;

                    assert(pMinorOpInfo);
                    numMinOpInfo = pMinorOpInfo->count;
                    pMinorOpInfo++;
                    assert(numMinOpInfo);
                    for (; numMinOpInfo; numMinOpInfo--, pMinorOpInfo++) {
                        if (majorop >= pMinorOpInfo->major.first &&
                            majorop <= pMinorOpInfo->major.last &&
                            RecordIsMemberOfSet(pMinorOpInfo->major.pMinOpSet,
                                                minorop)) {
                            RecordAProtocolElement(pContext, client,
                                                   XRecordFromServer,
                                                   (void *) pri->replyData,
                                                   pri->dataLenBytes, 0,
                                                   pri->bytesRemaining);
                            if (pri->bytesRemaining)
                                pContext->continuedReply = 1;
                            break;
                        }
                    }           /* end for each minor op info */
                }               /* end extension reply */
            }                   /* end continued reply vs. start of reply */
        }                       /* end client is registered on this context */
    }                           /* end for each context */
}                               /* RecordAReply */

/* RecordADeliveredEventOrError
 *
 * Arguments:
 *	pcbl is &EventCallback.
 *	nulldata is NULL.
 *	calldata is a pointer to a EventInfoRec (include/dix.h)
 *	  which provides information about events that are being sent
 *	  to clients.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	The event or error is recorded by all contexts that have registered
 *	it for this client.
 */
static void
RecordADeliveredEventOrError(CallbackListPtr *pcbl, void *nulldata,
                             void *calldata)
{
    EventInfoRec *pei = (EventInfoRec *) calldata;
    RecordContextPtr pContext;
    RecordClientsAndProtocolPtr pRCAP;
    int eci;                    /* enabled context index */
    ClientPtr pClient = pei->client;

    for (eci = 0; eci < numEnabledContexts; eci++) {
        pContext = ppAllContexts[eci];
        pRCAP = RecordFindClientOnContext(pContext, pClient->clientAsMask,
                                          NULL);
        if (pRCAP && (pRCAP->pDeliveredEventSet || pRCAP->pErrorSet)) {
            int ev;             /* event index */
            xEvent *pev = pei->events;

            for (ev = 0; ev < pei->count; ev++, pev++) {
                int recordit = 0;

                if (pRCAP->pErrorSet) {
                    recordit = RecordIsMemberOfSet(pRCAP->pErrorSet,
                                                   ((xError *) (pev))->
                                                   errorCode);
                }
                else if (pRCAP->pDeliveredEventSet) {
                    recordit = RecordIsMemberOfSet(pRCAP->pDeliveredEventSet,
                                                   pev->u.u.type & 0177);
                }
                if (recordit) {
                    xEvent swappedEvent;
                    xEvent *pEvToRecord = pev;

                    if (pClient->swapped) {
                        (*EventSwapVector[pev->u.u.type & 0177])
                            (pev, &swappedEvent);
                        pEvToRecord = &swappedEvent;

                    }
                    RecordAProtocolElement(pContext, pClient,
                                           XRecordFromServer, pEvToRecord,
                                           SIZEOF(xEvent), 0, 0);
                }
            }                   /* end for each event */
        }                       /* end this client is on this context */
    }                           /* end for each enabled context */
}                               /* RecordADeliveredEventOrError */

static void
RecordSendProtocolEvents(RecordClientsAndProtocolPtr pRCAP,
                         RecordContextPtr pContext, xEvent *pev, int count)
{
    int ev;                     /* event index */

    for (ev = 0; ev < count; ev++, pev++) {
        if (RecordIsMemberOfSet(pRCAP->pDeviceEventSet, pev->u.u.type & 0177)) {
            xEvent swappedEvent;
            xEvent *pEvToRecord = pev;

#ifdef PANORAMIX
            xEvent shiftedEvent;

            if (!noPanoramiXExtension &&
                (pev->u.u.type == MotionNotify ||
                 pev->u.u.type == ButtonPress ||
                 pev->u.u.type == ButtonRelease ||
                 pev->u.u.type == KeyPress || pev->u.u.type == KeyRelease)) {
                int scr = XineramaGetCursorScreen(inputInfo.pointer);

                memcpy(&shiftedEvent, pev, sizeof(xEvent));
                shiftedEvent.u.keyButtonPointer.rootX +=
                    screenInfo.screens[scr]->x - screenInfo.screens[0]->x;
                shiftedEvent.u.keyButtonPointer.rootY +=
                    screenInfo.screens[scr]->y - screenInfo.screens[0]->y;
                pEvToRecord = &shiftedEvent;
            }
#endif                          /* PANORAMIX */

            if (pContext->pRecordingClient->swapped) {
                (*EventSwapVector[pEvToRecord->u.u.type & 0177])
                    (pEvToRecord, &swappedEvent);
                pEvToRecord = &swappedEvent;
            }

            RecordAProtocolElement(pContext, NULL,
                                   XRecordFromServer, pEvToRecord,
                                   SIZEOF(xEvent), 0, 0);
            /* make sure device events get flushed in the absence
             * of other client activity
             */
            SetCriticalOutputPending();
        }
    }                           /* end for each event */

}                               /* RecordADeviceEvent */

/* RecordADeviceEvent
 *
 * Arguments:
 *	pcbl is &DeviceEventCallback.
 *	nulldata is NULL.
 *	calldata is a pointer to a DeviceEventInfoRec (include/dix.h)
 *	  which provides information about device events that occur.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	The device event is recorded by all contexts that have registered
 *	it for this client.
 */
static void
RecordADeviceEvent(CallbackListPtr *pcbl, void *nulldata, void *calldata)
{
    DeviceEventInfoRec *pei = (DeviceEventInfoRec *) calldata;
    RecordContextPtr pContext;
    RecordClientsAndProtocolPtr pRCAP;
    int eci;                    /* enabled context index */

    for (eci = 0; eci < numEnabledContexts; eci++) {
        pContext = ppAllContexts[eci];
        for (pRCAP = pContext->pListOfRCAP; pRCAP; pRCAP = pRCAP->pNextRCAP) {
            if (pRCAP->pDeviceEventSet) {
                int count;
                xEvent *xi_events = NULL;

                /* TODO check return values */
                if (IsMaster(pei->device)) {
                    xEvent *core_events;

                    EventToCore(pei->event, &core_events, &count);
                    RecordSendProtocolEvents(pRCAP, pContext, core_events,
                                             count);
                    free(core_events);
                }

                EventToXI(pei->event, &xi_events, &count);
                RecordSendProtocolEvents(pRCAP, pContext, xi_events, count);
                free(xi_events);
            }                   /* end this RCAP selects device events */
        }                       /* end for each RCAP on this context */
    }                           /* end for each enabled context */
}

/* RecordFlushAllContexts
 *
 * Arguments:
 *	pcbl is &FlushCallback.
 *	nulldata and calldata are NULL.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	All buffered reply data of all enabled contexts is written to
 *	the recording clients.
 */
static void
RecordFlushAllContexts(CallbackListPtr *pcbl,
                       void *nulldata, void *calldata)
{
    int eci;                    /* enabled context index */
    RecordContextPtr pContext;

    for (eci = 0; eci < numEnabledContexts; eci++) {
        pContext = ppAllContexts[eci];

        /* In most cases we leave it to RecordFlushReplyBuffer to make
         * this check, but this function could be called very often, so we
         * check before calling hoping to save the function call cost
         * most of the time.
         */
        if (pContext->numBufBytes)
            RecordFlushReplyBuffer(ppAllContexts[eci], NULL, 0, NULL, 0);
    }
}                               /* RecordFlushAllContexts */

/* RecordInstallHooks
 *
 * Arguments:
 *	pRCAP is an RCAP on an enabled or being-enabled context.
 *	oneclient can be zero or the resource ID mask identifying a client.
 *
 * Returns: BadAlloc if a memory allocation error occurred, else Success.
 *
 * Side Effects:
 *	Recording hooks needed by RCAP are installed.
 *	If oneclient is zero, recording hooks needed for all clients and
 *	protocol on the RCAP are installed.  If oneclient is non-zero,
 *	only those hooks needed for the specified client are installed.
 *	
 *	Client requestVectors may be altered.  numEnabledRCAPs will be
 *	incremented if oneclient == 0.  Callbacks may be added to
 *	various callback lists.
 */
static int
RecordInstallHooks(RecordClientsAndProtocolPtr pRCAP, XID oneclient)
{
    int i = 0;
    XID client;

    if (oneclient)
        client = oneclient;
    else
        client = pRCAP->numClients ? pRCAP->pClientIDs[i++] : 0;

    while (client) {
        if (client != XRecordFutureClients) {
            if (pRCAP->pRequestMajorOpSet) {
                RecordSetIteratePtr pIter = NULL;
                RecordSetInterval interval;
                ClientPtr pClient = clients[CLIENT_ID(client)];

                if (pClient && !RecordClientPrivate(pClient)) {
                    RecordClientPrivatePtr pClientPriv;

                    /* no Record proc vector; allocate one */
                    pClientPriv = (RecordClientPrivatePtr)
                        malloc(sizeof(RecordClientPrivateRec));
                    if (!pClientPriv)
                        return BadAlloc;
                    /* copy old proc vector to new */
                    memcpy(pClientPriv->recordVector, pClient->requestVector,
                           sizeof(pClientPriv->recordVector));
                    pClientPriv->originalVector = pClient->requestVector;
                    dixSetPrivate(&pClient->devPrivates,
                                  RecordClientPrivateKey, pClientPriv);
                    pClient->requestVector = pClientPriv->recordVector;
                }
                while ((pIter = RecordIterateSet(pRCAP->pRequestMajorOpSet,
                                                 pIter, &interval))) {
                    unsigned int j;

                    for (j = interval.first; j <= interval.last; j++)
                        pClient->requestVector[j] = RecordARequest;
                }
            }
        }
        if (oneclient)
            client = 0;
        else
            client = (i < pRCAP->numClients) ? pRCAP->pClientIDs[i++] : 0;
    }

    assert(numEnabledRCAPs >= 0);
    if (!oneclient && ++numEnabledRCAPs == 1) { /* we're enabling the first context */
        if (!AddCallback(&EventCallback, RecordADeliveredEventOrError, NULL))
            return BadAlloc;
        if (!AddCallback(&DeviceEventCallback, RecordADeviceEvent, NULL))
            return BadAlloc;
        if (!AddCallback(&ReplyCallback, RecordAReply, NULL))
            return BadAlloc;
        if (!AddCallback(&FlushCallback, RecordFlushAllContexts, NULL))
            return BadAlloc;
        /* Alternate context flushing scheme: delete the line above
         * and call RegisterBlockAndWakeupHandlers here passing
         * RecordFlushAllContexts.  Is this any better?
         */
    }
    return Success;
}                               /* RecordInstallHooks */

/* RecordUninstallHooks
 *
 * Arguments:
 *	pRCAP is an RCAP on an enabled or being-disabled context.
 *	oneclient can be zero or the resource ID mask identifying a client.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Recording hooks needed by RCAP may be uninstalled.
 *	If oneclient is zero, recording hooks needed for all clients and
 *	protocol on the RCAP may be uninstalled.  If oneclient is non-zero,
 *	only those hooks needed for the specified client may be uninstalled.
 *	
 *	Client requestVectors may be altered.  numEnabledRCAPs will be
 *	decremented if oneclient == 0.  Callbacks may be deleted from
 *	various callback lists.
 */
static void
RecordUninstallHooks(RecordClientsAndProtocolPtr pRCAP, XID oneclient)
{
    int i = 0;
    XID client;

    if (oneclient)
        client = oneclient;
    else
        client = pRCAP->numClients ? pRCAP->pClientIDs[i++] : 0;

    while (client) {
        if (client != XRecordFutureClients) {
            if (pRCAP->pRequestMajorOpSet) {
                ClientPtr pClient = clients[CLIENT_ID(client)];
                int c;
                Bool otherRCAPwantsProcVector = FALSE;
                RecordClientPrivatePtr pClientPriv = NULL;

                assert(pClient);
                pClientPriv = RecordClientPrivate(pClient);
                assert(pClientPriv);
                memcpy(pClientPriv->recordVector, pClientPriv->originalVector,
                       sizeof(pClientPriv->recordVector));

                for (c = 0; c < numEnabledContexts; c++) {
                    RecordClientsAndProtocolPtr pOtherRCAP;
                    RecordContextPtr pContext = ppAllContexts[c];

                    if (pContext == pRCAP->pContext)
                        continue;
                    pOtherRCAP = RecordFindClientOnContext(pContext, client,
                                                           NULL);
                    if (pOtherRCAP && pOtherRCAP->pRequestMajorOpSet) {
                        RecordSetIteratePtr pIter = NULL;
                        RecordSetInterval interval;

                        otherRCAPwantsProcVector = TRUE;
                        while ((pIter =
                                RecordIterateSet(pOtherRCAP->pRequestMajorOpSet,
                                                 pIter, &interval))) {
                            unsigned int j;

                            for (j = interval.first; j <= interval.last; j++)
                                pClient->requestVector[j] = RecordARequest;
                        }
                    }
                }
                if (!otherRCAPwantsProcVector) {        /* nobody needs it, so free it */
                    pClient->requestVector = pClientPriv->originalVector;
                    dixSetPrivate(&pClient->devPrivates,
                                  RecordClientPrivateKey, NULL);
                    free(pClientPriv);
                }
            }                   /* end if this RCAP specifies any requests */
        }                       /* end if not future clients */
        if (oneclient)
            client = 0;
        else
            client = (i < pRCAP->numClients) ? pRCAP->pClientIDs[i++] : 0;
    }

    assert(numEnabledRCAPs >= 1);
    if (!oneclient && --numEnabledRCAPs == 0) { /* we're disabling the last context */
        DeleteCallback(&EventCallback, RecordADeliveredEventOrError, NULL);
        DeleteCallback(&DeviceEventCallback, RecordADeviceEvent, NULL);
        DeleteCallback(&ReplyCallback, RecordAReply, NULL);
        DeleteCallback(&FlushCallback, RecordFlushAllContexts, NULL);
        /* Alternate context flushing scheme: delete the line above
         * and call RemoveBlockAndWakeupHandlers here passing
         * RecordFlushAllContexts.  Is this any better?
         */
        /* Having deleted the callback, call it one last time. -gildea */
        RecordFlushAllContexts(&FlushCallback, NULL, NULL);
    }
}                               /* RecordUninstallHooks */

/* RecordDeleteClientFromRCAP
 *
 * Arguments:
 *	pRCAP is an RCAP to delete the client from.
 *	position is the index into the array pRCAP->pClientIDs of the
 *	client to delete.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Recording hooks needed by client will be uninstalled if the context
 *	is enabled.  The designated client will be removed from the
 *	pRCAP->pClientIDs array.  If it was the only client on the RCAP,
 *	the RCAP is removed from the context and freed.  (Invariant: RCAPs
 *	have at least one client.)
 */
static void
RecordDeleteClientFromRCAP(RecordClientsAndProtocolPtr pRCAP, int position)
{
    if (pRCAP->pContext->pRecordingClient)
        RecordUninstallHooks(pRCAP, pRCAP->pClientIDs[position]);
    if (position != pRCAP->numClients - 1)
        pRCAP->pClientIDs[position] = pRCAP->pClientIDs[pRCAP->numClients - 1];
    if (--pRCAP->numClients == 0) {     /* no more clients; remove RCAP from context's list */
        RecordContextPtr pContext = pRCAP->pContext;

        if (pContext->pRecordingClient)
            RecordUninstallHooks(pRCAP, 0);
        if (pContext->pListOfRCAP == pRCAP)
            pContext->pListOfRCAP = pRCAP->pNextRCAP;
        else {
            RecordClientsAndProtocolPtr prevRCAP;

            for (prevRCAP = pContext->pListOfRCAP;
                 prevRCAP->pNextRCAP != pRCAP; prevRCAP = prevRCAP->pNextRCAP);
            prevRCAP->pNextRCAP = pRCAP->pNextRCAP;
        }
        /* free the RCAP */
        if (pRCAP->clientIDsSeparatelyAllocated)
            free(pRCAP->pClientIDs);
        free(pRCAP);
    }
}                               /* RecordDeleteClientFromRCAP */

/* RecordAddClientToRCAP
 *
 * Arguments:
 *	pRCAP is an RCAP to add the client to.
 *	clientspec is the resource ID mask identifying a client, or
 *	  XRecordFutureClients.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Recording hooks needed by client will be installed if the context
 *	is enabled.  The designated client will be added to the
 *	pRCAP->pClientIDs array, which may be realloced.
 *	pRCAP->clientIDsSeparatelyAllocated may be set to 1 if there
 *	is no more room to hold clients internal to the RCAP.
 */
static void
RecordAddClientToRCAP(RecordClientsAndProtocolPtr pRCAP, XID clientspec)
{
    if (pRCAP->numClients == pRCAP->sizeClients) {
        if (pRCAP->clientIDsSeparatelyAllocated) {
            XID *pNewIDs =
                reallocarray(pRCAP->pClientIDs,
                             pRCAP->sizeClients + CLIENT_ARRAY_GROWTH_INCREMENT,
                             sizeof(XID));
            if (!pNewIDs)
                return;
            pRCAP->pClientIDs = pNewIDs;
            pRCAP->sizeClients += CLIENT_ARRAY_GROWTH_INCREMENT;
        }
        else {
            XID *pNewIDs =
                xallocarray(pRCAP->sizeClients + CLIENT_ARRAY_GROWTH_INCREMENT,
                            sizeof(XID));
            if (!pNewIDs)
                return;
            memcpy(pNewIDs, pRCAP->pClientIDs, pRCAP->numClients * sizeof(XID));
            pRCAP->pClientIDs = pNewIDs;
            pRCAP->sizeClients += CLIENT_ARRAY_GROWTH_INCREMENT;
            pRCAP->clientIDsSeparatelyAllocated = 1;
        }
    }
    pRCAP->pClientIDs[pRCAP->numClients++] = clientspec;
    if (pRCAP->pContext->pRecordingClient)
        RecordInstallHooks(pRCAP, clientspec);
}                               /* RecordDeleteClientFromRCAP */

/* RecordDeleteClientFromContext
 *
 * Arguments:
 *	pContext is the context to delete from.
 *	clientspec is the resource ID mask identifying a client, or
 *	  XRecordFutureClients.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	If clientspec is on any RCAP of the context, it is deleted from that
 *	RCAP.  (A given clientspec can only be on one RCAP of a context.)
 */
static void
RecordDeleteClientFromContext(RecordContextPtr pContext, XID clientspec)
{
    RecordClientsAndProtocolPtr pRCAP;
    int position;

    if ((pRCAP = RecordFindClientOnContext(pContext, clientspec, &position)))
        RecordDeleteClientFromRCAP(pRCAP, position);
}                               /* RecordDeleteClientFromContext */

/* RecordSanityCheckClientSpecifiers
 *
 * Arguments:
 *	clientspecs is an array of alleged CLIENTSPECs passed by the client.
 *	nspecs is the number of elements in clientspecs.
 *	errorspec, if non-zero, is the resource id base of a client that
 *	  must not appear in clienspecs.
 *
 * Returns: BadMatch if any of the clientspecs are invalid, else Success.
 *
 * Side Effects: none.
 */
static int
RecordSanityCheckClientSpecifiers(ClientPtr client, XID *clientspecs,
                                  int nspecs, XID errorspec)
{
    int i;
    int clientIndex;
    int rc;
    void *value;

    for (i = 0; i < nspecs; i++) {
        if (clientspecs[i] == XRecordCurrentClients ||
            clientspecs[i] == XRecordFutureClients ||
            clientspecs[i] == XRecordAllClients)
            continue;
        if (errorspec && (CLIENT_BITS(clientspecs[i]) == errorspec))
            return BadMatch;
        clientIndex = CLIENT_ID(clientspecs[i]);
        if (clientIndex && clients[clientIndex] &&
            clients[clientIndex]->clientState == ClientStateRunning) {
            if (clientspecs[i] == clients[clientIndex]->clientAsMask)
                continue;
            rc = dixLookupResourceByClass(&value, clientspecs[i], RC_ANY,
                                          client, DixGetAttrAccess);
            if (rc != Success)
                return rc;
        }
        else
            return BadMatch;
    }
    return Success;
}                               /* RecordSanityCheckClientSpecifiers */

/* RecordCanonicalizeClientSpecifiers
 *
 * Arguments:
 *	pClientspecs is an array of CLIENTSPECs that have been sanity
 *	  checked.
 *	pNumClientspecs is a pointer to the number of elements in pClientspecs.
 *	excludespec, if non-zero, is the resource id base of a client that
 *	  should not be included in the expansion of XRecordAllClients or
 *	  XRecordCurrentClients.
 *
 * Returns:
 *	A pointer to an array of CLIENTSPECs that is the same as the
 *	passed array with the following modifications:
 *	  - all but the client id bits of resource IDs are stripped off.
 *	  - duplicates removed.
 *	  - XRecordAllClients expanded to a list of all currently connected
 *	    clients + XRecordFutureClients - excludespec (if non-zero)
 *	  - XRecordCurrentClients expanded to a list of all currently
 *	    connected clients - excludespec (if non-zero)
 *	The returned array may be the passed array modified in place, or
 *	it may be an malloc'ed array.  The caller should keep a pointer to the
 *	original array and free the returned array if it is different.
 *
 *	*pNumClientspecs is set to the number of elements in the returned
 *	array.
 *
 * Side Effects:
 *	pClientspecs may be modified in place.
 */
static XID *
RecordCanonicalizeClientSpecifiers(XID *pClientspecs, int *pNumClientspecs,
                                   XID excludespec)
{
    int i;
    int numClients = *pNumClientspecs;

    /*  first pass strips off the resource index bits, leaving just the
     *  client id bits.  This makes searching for a particular client simpler
     *  (and faster.)
     */
    for (i = 0; i < numClients; i++) {
        XID cs = pClientspecs[i];

        if (cs > XRecordAllClients)
            pClientspecs[i] = CLIENT_BITS(cs);
    }

    for (i = 0; i < numClients; i++) {
        if (pClientspecs[i] == XRecordAllClients || pClientspecs[i] == XRecordCurrentClients) { /* expand All/Current */
            int j, nc;
            XID *pCanon = xallocarray(currentMaxClients + 1, sizeof(XID));

            if (!pCanon)
                return NULL;
            for (nc = 0, j = 1; j < currentMaxClients; j++) {
                ClientPtr client = clients[j];

                if (client != NullClient &&
                    client->clientState == ClientStateRunning &&
                    client->clientAsMask != excludespec) {
                    pCanon[nc++] = client->clientAsMask;
                }
            }
            if (pClientspecs[i] == XRecordAllClients)
                pCanon[nc++] = XRecordFutureClients;
            *pNumClientspecs = nc;
            return pCanon;
        }
        else {                  /* not All or Current */

            int j;

            for (j = i + 1; j < numClients;) {
                if (pClientspecs[i] == pClientspecs[j]) {
                    pClientspecs[j] = pClientspecs[--numClients];
                }
                else
                    j++;
            }
        }
    }                           /* end for each clientspec */
    *pNumClientspecs = numClients;
    return pClientspecs;
}                               /* RecordCanonicalizeClientSpecifiers */

/****************************************************************************/

/* stuff for RegisterClients */

/* RecordPadAlign
 *
 * Arguments:
 *	size is the number of bytes taken by an object.
 *	align is a byte boundary (e.g. 4, 8)
 *
 * Returns:
 *	the number of pad bytes to add at the end of an object of the
 *	given size so that an object placed immediately behind it will
 *	begin on an <align>-byte boundary.
 *
 * Side Effects: none.
 */
static int
RecordPadAlign(int size, int align)
{
    return (align - (size & (align - 1))) & (align - 1);
}                               /* RecordPadAlign */

/* RecordSanityCheckRegisterClients
 *
 * Arguments:
 *	pContext is the context being registered on.
 *	client is the client that issued a RecordCreateContext or
 *	  RecordRegisterClients request.
 *	stuff is a pointer to the request.
 *
 * Returns:
 *	Any one of several possible error values if any of the request
 *	arguments are invalid.  Success if everything is OK.
 *
 * Side Effects: none.
 */
static int
RecordSanityCheckRegisterClients(RecordContextPtr pContext, ClientPtr client,
                                 xRecordRegisterClientsReq * stuff)
{
    int err;
    xRecordRange *pRange;
    int i;
    XID recordingClient;

    if (((client->req_len << 2) - SIZEOF(xRecordRegisterClientsReq)) !=
        4 * stuff->nClients + SIZEOF(xRecordRange) * stuff->nRanges)
        return BadLength;

    if (stuff->elementHeader &
        ~(XRecordFromClientSequence | XRecordFromClientTime |
          XRecordFromServerTime)) {
        client->errorValue = stuff->elementHeader;
        return BadValue;
    }

    recordingClient = pContext->pRecordingClient ?
        pContext->pRecordingClient->clientAsMask : 0;
    err = RecordSanityCheckClientSpecifiers(client, (XID *) &stuff[1],
                                            stuff->nClients, recordingClient);
    if (err != Success)
        return err;

    pRange = (xRecordRange *) (((XID *) &stuff[1]) + stuff->nClients);
    for (i = 0; i < stuff->nRanges; i++, pRange++) {
        if (pRange->coreRequestsFirst > pRange->coreRequestsLast) {
            client->errorValue = pRange->coreRequestsFirst;
            return BadValue;
        }
        if (pRange->coreRepliesFirst > pRange->coreRepliesLast) {
            client->errorValue = pRange->coreRepliesFirst;
            return BadValue;
        }
        if ((pRange->extRequestsMajorFirst || pRange->extRequestsMajorLast) &&
            (pRange->extRequestsMajorFirst < 128 ||
             pRange->extRequestsMajorLast < 128 ||
             pRange->extRequestsMajorFirst > pRange->extRequestsMajorLast)) {
            client->errorValue = pRange->extRequestsMajorFirst;
            return BadValue;
        }
        if (pRange->extRequestsMinorFirst > pRange->extRequestsMinorLast) {
            client->errorValue = pRange->extRequestsMinorFirst;
            return BadValue;
        }
        if ((pRange->extRepliesMajorFirst || pRange->extRepliesMajorLast) &&
            (pRange->extRepliesMajorFirst < 128 ||
             pRange->extRepliesMajorLast < 128 ||
             pRange->extRepliesMajorFirst > pRange->extRepliesMajorLast)) {
            client->errorValue = pRange->extRepliesMajorFirst;
            return BadValue;
        }
        if (pRange->extRepliesMinorFirst > pRange->extRepliesMinorLast) {
            client->errorValue = pRange->extRepliesMinorFirst;
            return BadValue;
        }
        if ((pRange->deliveredEventsFirst || pRange->deliveredEventsLast) &&
            (pRange->deliveredEventsFirst < 2 ||
             pRange->deliveredEventsLast < 2 ||
             pRange->deliveredEventsFirst > pRange->deliveredEventsLast)) {
            client->errorValue = pRange->deliveredEventsFirst;
            return BadValue;
        }
        if ((pRange->deviceEventsFirst || pRange->deviceEventsLast) &&
            (pRange->deviceEventsFirst < 2 ||
             pRange->deviceEventsLast < 2 ||
             pRange->deviceEventsFirst > pRange->deviceEventsLast)) {
            client->errorValue = pRange->deviceEventsFirst;
            return BadValue;
        }
        if (pRange->errorsFirst > pRange->errorsLast) {
            client->errorValue = pRange->errorsFirst;
            return BadValue;
        }
        if (pRange->clientStarted != xFalse && pRange->clientStarted != xTrue) {
            client->errorValue = pRange->clientStarted;
            return BadValue;
        }
        if (pRange->clientDied != xFalse && pRange->clientDied != xTrue) {
            client->errorValue = pRange->clientDied;
            return BadValue;
        }
    }                           /* end for each range */
    return Success;
}                               /* end RecordSanityCheckRegisterClients */

/* This is a tactical structure used to gather information about all the sets
 * (RecordSetPtr) that need to be created for an RCAP in the process of
 * digesting a list of RECORDRANGEs (converting it to the internal
 * representation).
 */
typedef struct {
    int nintervals;             /* number of intervals in following array */
    RecordSetInterval *intervals;       /* array of intervals for this set */
    int size;                   /* size of intervals array; >= nintervals */
    int align;                  /* alignment restriction for set */
    int offset;                 /* where to store set pointer rel. to start of RCAP */
    short first, last;          /* if for extension, major opcode interval */
} SetInfoRec, *SetInfoPtr;

#if defined(ERR) && defined(__sun)
#undef ERR /* Avoid conflict with Solaris <sys/regset.h> */
#endif

/* These constant are used to index into an array of SetInfoRec. */
enum { REQ,                     /* set info for requests */
    REP,                        /* set info for replies */
    ERR,                        /* set info for errors */
    DEV,                        /* set info for device events */
    DLEV,                       /* set info for delivered events */
    PREDEFSETS
};                              /* number of predefined array entries */

/* RecordAllocIntervals
 *
 * Arguments:
 *	psi is a pointer to a SetInfoRec whose intervals pointer is NULL.
 *	nIntervals is the desired size of the intervals array.
 *
 * Returns: BadAlloc if a memory allocation error occurred, else Success.
 *
 * Side Effects:
 *	If Success is returned, psi->intervals is a pointer to size
 *	RecordSetIntervals, all zeroed, and psi->size is set to size.
 */
static int
RecordAllocIntervals(SetInfoPtr psi, int nIntervals)
{
    assert(!psi->intervals);
    psi->intervals = xallocarray(nIntervals, sizeof(RecordSetInterval));
    if (!psi->intervals)
        return BadAlloc;
    memset(psi->intervals, 0, nIntervals * sizeof(RecordSetInterval));
    psi->size = nIntervals;
    return Success;
}                               /* end RecordAllocIntervals */

/* RecordConvertRangesToIntervals
 *
 * Arguments:
 *	psi is a pointer to the SetInfoRec we are building.
 *	pRanges is an array of xRecordRanges.
 *	nRanges is the number of elements in pRanges.
 *	byteoffset is the offset from the start of an xRecordRange of the
 *	  two bytes (1 for first, 1 for last) we are interested in.
 *	pExtSetInfo, if non-NULL, indicates that the two bytes mentioned
 *	  above are followed by four bytes (2 for first, 2 for last)
 *	  representing a minor opcode range, and this information should be
 *	  stored in one of the SetInfoRecs starting at pExtSetInfo.
 *	pnExtSetInfo is the number of elements in the pExtSetInfo array.
 *
 * Returns:  BadAlloc if a memory allocation error occurred, else Success.
 *
 * Side Effects:
 *	The slice of pRanges indicated by byteoffset is stored in psi.
 *	If pExtSetInfo is non-NULL, minor opcode intervals are stored
 *	in an existing SetInfoRec if the major opcode interval matches, else
 *	they are stored in a new SetInfoRec, and *pnExtSetInfo is
 *	increased accordingly.
 */
static int
RecordConvertRangesToIntervals(SetInfoPtr psi,
                               xRecordRange * pRanges,
                               int nRanges,
                               int byteoffset,
                               SetInfoPtr pExtSetInfo, int *pnExtSetInfo)
{
    int i;
    CARD8 *pCARD8;
    int first, last;
    int err;

    for (i = 0; i < nRanges; i++, pRanges++) {
        pCARD8 = ((CARD8 *) pRanges) + byteoffset;
        first = pCARD8[0];
        last = pCARD8[1];
        if (first || last) {
            if (!psi->intervals) {
                err = RecordAllocIntervals(psi, 2 * (nRanges - i));
                if (err != Success)
                    return err;
            }
            psi->intervals[psi->nintervals].first = first;
            psi->intervals[psi->nintervals].last = last;
            psi->nintervals++;
            assert(psi->nintervals <= psi->size);
            if (pExtSetInfo) {
                SetInfoPtr pesi = pExtSetInfo;
                CARD16 *pCARD16 = (CARD16 *) (pCARD8 + 2);
                int j;

                for (j = 0; j < *pnExtSetInfo; j++, pesi++) {
                    if ((first == pesi->first) && (last == pesi->last))
                        break;
                }
                if (j == *pnExtSetInfo) {
                    err = RecordAllocIntervals(pesi, 2 * (nRanges - i));
                    if (err != Success)
                        return err;
                    pesi->first = first;
                    pesi->last = last;
                    (*pnExtSetInfo)++;
                }
                pesi->intervals[pesi->nintervals].first = pCARD16[0];
                pesi->intervals[pesi->nintervals].last = pCARD16[1];
                pesi->nintervals++;
                assert(pesi->nintervals <= pesi->size);
            }
        }
    }
    return Success;
}                               /* end RecordConvertRangesToIntervals */

#define offset_of(_structure, _field) \
    ((char *)(& (_structure . _field)) - (char *)(&_structure))

/* RecordRegisterClients
 *
 * Arguments:
 *	pContext is the context on which to register the clients.
 *	client is the client that issued the RecordCreateContext or
 *	  RecordRegisterClients request.
 *	stuff is a pointer to the request.
 *
 * Returns:
 *	Any one of several possible error values defined by the protocol.
 *	Success if everything is OK.
 *
 * Side Effects:
 *	If different element headers are specified, the context is flushed.
 *	If any of the specified clients are already registered on the
 *	context, they are first unregistered.  A new RCAP is created to
 *	hold the specified protocol and clients, and it is linked onto the
 *	context.  If the context is enabled, appropriate hooks are installed
 *	to record the new clients and protocol.
 */
static int
RecordRegisterClients(RecordContextPtr pContext, ClientPtr client,
                      xRecordRegisterClientsReq * stuff)
{
    int err;
    int i;
    SetInfoPtr si;
    int maxSets;
    int nExtReqSets = 0;
    int nExtRepSets = 0;
    int extReqSetsOffset = 0;
    int extRepSetsOffset = 0;
    SetInfoPtr pExtReqSets, pExtRepSets;
    int clientListOffset;
    XID *pCanonClients;
    int clientStarted = 0, clientDied = 0;
    xRecordRange *pRanges, rr;
    int nClients;
    int sizeClients;
    int totRCAPsize;
    RecordClientsAndProtocolPtr pRCAP;
    int pad;
    XID recordingClient;

    /* do all sanity checking up front */

    err = RecordSanityCheckRegisterClients(pContext, client, stuff);
    if (err != Success)
        return err;

    /* if element headers changed, flush buffer */

    if (pContext->elemHeaders != stuff->elementHeader) {
        RecordFlushReplyBuffer(pContext, NULL, 0, NULL, 0);
        pContext->elemHeaders = stuff->elementHeader;
    }

    nClients = stuff->nClients;
    if (!nClients)
        /* if empty clients list, we're done. */
        return Success;

    recordingClient = pContext->pRecordingClient ?
        pContext->pRecordingClient->clientAsMask : 0;
    pCanonClients = RecordCanonicalizeClientSpecifiers((XID *) &stuff[1],
                                                       &nClients,
                                                       recordingClient);
    if (!pCanonClients)
        return BadAlloc;

    /* We may have to create as many as one set for each "predefined"
     * protocol types, plus one per range for extension requests, plus one per
     * range for extension replies.
     */
    maxSets = PREDEFSETS + 2 * stuff->nRanges;
    si = xallocarray(maxSets, sizeof(SetInfoRec));
    if (!si) {
        err = BadAlloc;
        goto bailout;
    }
    memset(si, 0, sizeof(SetInfoRec) * maxSets);

    /* theoretically you must do this because NULL may not be all-bits-zero */
    for (i = 0; i < maxSets; i++)
        si[i].intervals = NULL;

    pExtReqSets = si + PREDEFSETS;
    pExtRepSets = pExtReqSets + stuff->nRanges;

    pRanges = (xRecordRange *) (((XID *) &stuff[1]) + stuff->nClients);

    err = RecordConvertRangesToIntervals(&si[REQ], pRanges, stuff->nRanges,
                                         offset_of(rr, coreRequestsFirst), NULL,
                                         NULL);
    if (err != Success)
        goto bailout;

    err = RecordConvertRangesToIntervals(&si[REQ], pRanges, stuff->nRanges,
                                         offset_of(rr, extRequestsMajorFirst),
                                         pExtReqSets, &nExtReqSets);
    if (err != Success)
        goto bailout;

    err = RecordConvertRangesToIntervals(&si[REP], pRanges, stuff->nRanges,
                                         offset_of(rr, coreRepliesFirst), NULL,
                                         NULL);
    if (err != Success)
        goto bailout;

    err = RecordConvertRangesToIntervals(&si[REP], pRanges, stuff->nRanges,
                                         offset_of(rr, extRepliesMajorFirst),
                                         pExtRepSets, &nExtRepSets);
    if (err != Success)
        goto bailout;

    err = RecordConvertRangesToIntervals(&si[ERR], pRanges, stuff->nRanges,
                                         offset_of(rr, errorsFirst), NULL,
                                         NULL);
    if (err != Success)
        goto bailout;

    err = RecordConvertRangesToIntervals(&si[DLEV], pRanges, stuff->nRanges,
                                         offset_of(rr, deliveredEventsFirst),
                                         NULL, NULL);
    if (err != Success)
        goto bailout;

    err = RecordConvertRangesToIntervals(&si[DEV], pRanges, stuff->nRanges,
                                         offset_of(rr, deviceEventsFirst), NULL,
                                         NULL);
    if (err != Success)
        goto bailout;

    /* collect client-started and client-died */

    for (i = 0; i < stuff->nRanges; i++) {
        if (pRanges[i].clientStarted)
            clientStarted = TRUE;
        if (pRanges[i].clientDied)
            clientDied = TRUE;
    }

    /*  We now have all the information collected to create all the sets,
     * and we can compute the total memory required for the RCAP.
     */

    totRCAPsize = sizeof(RecordClientsAndProtocolRec);

    /* leave a little room to grow before forcing a separate allocation */
    sizeClients = nClients + CLIENT_ARRAY_GROWTH_INCREMENT;
    pad = RecordPadAlign(totRCAPsize, sizeof(XID));
    clientListOffset = totRCAPsize + pad;
    totRCAPsize += pad + sizeClients * sizeof(XID);

    if (nExtReqSets) {
        pad = RecordPadAlign(totRCAPsize, sizeof(RecordSetPtr));
        extReqSetsOffset = totRCAPsize + pad;
        totRCAPsize += pad + (nExtReqSets + 1) * sizeof(RecordMinorOpRec);
    }
    if (nExtRepSets) {
        pad = RecordPadAlign(totRCAPsize, sizeof(RecordSetPtr));
        extRepSetsOffset = totRCAPsize + pad;
        totRCAPsize += pad + (nExtRepSets + 1) * sizeof(RecordMinorOpRec);
    }

    for (i = 0; i < maxSets; i++) {
        if (si[i].nintervals) {
            si[i].size =
                RecordSetMemoryRequirements(si[i].intervals, si[i].nintervals,
                                            &si[i].align);
            pad = RecordPadAlign(totRCAPsize, si[i].align);
            si[i].offset = pad + totRCAPsize;
            totRCAPsize += pad + si[i].size;
        }
    }

    /* allocate memory for the whole RCAP */

    pRCAP = (RecordClientsAndProtocolPtr) malloc(totRCAPsize);
    if (!pRCAP) {
        err = BadAlloc;
        goto bailout;
    }

    /* fill in the RCAP */

    pRCAP->pContext = pContext;
    pRCAP->pClientIDs = (XID *) ((char *) pRCAP + clientListOffset);
    pRCAP->numClients = nClients;
    pRCAP->sizeClients = sizeClients;
    pRCAP->clientIDsSeparatelyAllocated = 0;
    for (i = 0; i < nClients; i++) {
        RecordDeleteClientFromContext(pContext, pCanonClients[i]);
        pRCAP->pClientIDs[i] = pCanonClients[i];
    }

    /* create all the sets */

    if (si[REQ].intervals) {
        pRCAP->pRequestMajorOpSet =
            RecordCreateSet(si[REQ].intervals, si[REQ].nintervals,
                            (RecordSetPtr) ((char *) pRCAP + si[REQ].offset),
                            si[REQ].size);
    }
    else
        pRCAP->pRequestMajorOpSet = NULL;

    if (si[REP].intervals) {
        pRCAP->pReplyMajorOpSet =
            RecordCreateSet(si[REP].intervals, si[REP].nintervals,
                            (RecordSetPtr) ((char *) pRCAP + si[REP].offset),
                            si[REP].size);
    }
    else
        pRCAP->pReplyMajorOpSet = NULL;

    if (si[ERR].intervals) {
        pRCAP->pErrorSet =
            RecordCreateSet(si[ERR].intervals, si[ERR].nintervals,
                            (RecordSetPtr) ((char *) pRCAP + si[ERR].offset),
                            si[ERR].size);
    }
    else
        pRCAP->pErrorSet = NULL;

    if (si[DEV].intervals) {
        pRCAP->pDeviceEventSet =
            RecordCreateSet(si[DEV].intervals, si[DEV].nintervals,
                            (RecordSetPtr) ((char *) pRCAP + si[DEV].offset),
                            si[DEV].size);
    }
    else
        pRCAP->pDeviceEventSet = NULL;

    if (si[DLEV].intervals) {
        pRCAP->pDeliveredEventSet =
            RecordCreateSet(si[DLEV].intervals, si[DLEV].nintervals,
                            (RecordSetPtr) ((char *) pRCAP + si[DLEV].offset),
                            si[DLEV].size);
    }
    else
        pRCAP->pDeliveredEventSet = NULL;

    if (nExtReqSets) {
        pRCAP->pRequestMinOpInfo = (RecordMinorOpPtr)
            ((char *) pRCAP + extReqSetsOffset);
        pRCAP->pRequestMinOpInfo[0].count = nExtReqSets;
        for (i = 0; i < nExtReqSets; i++, pExtReqSets++) {
            pRCAP->pRequestMinOpInfo[i + 1].major.first = pExtReqSets->first;
            pRCAP->pRequestMinOpInfo[i + 1].major.last = pExtReqSets->last;
            pRCAP->pRequestMinOpInfo[i + 1].major.pMinOpSet =
                RecordCreateSet(pExtReqSets->intervals,
                                pExtReqSets->nintervals,
                                (RecordSetPtr) ((char *) pRCAP +
                                                pExtReqSets->offset),
                                pExtReqSets->size);
        }
    }
    else
        pRCAP->pRequestMinOpInfo = NULL;

    if (nExtRepSets) {
        pRCAP->pReplyMinOpInfo = (RecordMinorOpPtr)
            ((char *) pRCAP + extRepSetsOffset);
        pRCAP->pReplyMinOpInfo[0].count = nExtRepSets;
        for (i = 0; i < nExtRepSets; i++, pExtRepSets++) {
            pRCAP->pReplyMinOpInfo[i + 1].major.first = pExtRepSets->first;
            pRCAP->pReplyMinOpInfo[i + 1].major.last = pExtRepSets->last;
            pRCAP->pReplyMinOpInfo[i + 1].major.pMinOpSet =
                RecordCreateSet(pExtRepSets->intervals,
                                pExtRepSets->nintervals,
                                (RecordSetPtr) ((char *) pRCAP +
                                                pExtRepSets->offset),
                                pExtRepSets->size);
        }
    }
    else
        pRCAP->pReplyMinOpInfo = NULL;

    pRCAP->clientStarted = clientStarted;
    pRCAP->clientDied = clientDied;

    /* link the RCAP onto the context */

    pRCAP->pNextRCAP = pContext->pListOfRCAP;
    pContext->pListOfRCAP = pRCAP;

    if (pContext->pRecordingClient)     /* context enabled */
        RecordInstallHooks(pRCAP, 0);

 bailout:
    if (si) {
        for (i = 0; i < maxSets; i++)
            free(si[i].intervals);
        free(si);
    }
    if (pCanonClients && pCanonClients != (XID *) &stuff[1])
        free(pCanonClients);
    return err;
}                               /* RecordRegisterClients */

/* Proc functions all take a client argument, execute the request in
 * client->requestBuffer, and return a protocol error status.
 */

static int
ProcRecordQueryVersion(ClientPtr client)
{
    /* REQUEST(xRecordQueryVersionReq); */
    xRecordQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_RECORD_MAJOR_VERSION,
        .minorVersion = SERVER_RECORD_MINOR_VERSION
    };

    REQUEST_SIZE_MATCH(xRecordQueryVersionReq);
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xRecordQueryVersionReply), &rep);
    return Success;
}                               /* ProcRecordQueryVersion */

static int
ProcRecordCreateContext(ClientPtr client)
{
    REQUEST(xRecordCreateContextReq);
    RecordContextPtr pContext;
    RecordContextPtr *ppNewAllContexts = NULL;
    int err = BadAlloc;

    REQUEST_AT_LEAST_SIZE(xRecordCreateContextReq);
    LEGAL_NEW_RESOURCE(stuff->context, client);

    pContext = (RecordContextPtr) malloc(sizeof(RecordContextRec));
    if (!pContext)
        goto bailout;

    /* make sure there is room in ppAllContexts to store the new context */

    ppNewAllContexts =
        reallocarray(ppAllContexts, numContexts + 1, sizeof(RecordContextPtr));
    if (!ppNewAllContexts)
        goto bailout;
    ppAllContexts = ppNewAllContexts;

    pContext->id = stuff->context;
    pContext->pRecordingClient = NULL;
    pContext->pListOfRCAP = NULL;
    pContext->elemHeaders = 0;
    pContext->bufCategory = 0;
    pContext->numBufBytes = 0;
    pContext->pBufClient = NULL;
    pContext->continuedReply = 0;
    pContext->inFlush = 0;

    err = RecordRegisterClients(pContext, client,
                                (xRecordRegisterClientsReq *) stuff);
    if (err != Success)
        goto bailout;

    if (AddResource(pContext->id, RTContext, pContext)) {
        ppAllContexts[numContexts++] = pContext;
        return Success;
    }
    else {
        return BadAlloc;
    }
 bailout:
    free(pContext);
    return err;
}                               /* ProcRecordCreateContext */

static int
ProcRecordRegisterClients(ClientPtr client)
{
    RecordContextPtr pContext;

    REQUEST(xRecordRegisterClientsReq);

    REQUEST_AT_LEAST_SIZE(xRecordRegisterClientsReq);
    VERIFY_CONTEXT(pContext, stuff->context, client);

    return RecordRegisterClients(pContext, client, stuff);
}                               /* ProcRecordRegisterClients */

static int
ProcRecordUnregisterClients(ClientPtr client)
{
    RecordContextPtr pContext;
    int err;

    REQUEST(xRecordUnregisterClientsReq);
    XID *pCanonClients;
    int nClients;
    int i;

    REQUEST_AT_LEAST_SIZE(xRecordUnregisterClientsReq);
    if (INT_MAX / 4 < stuff->nClients ||
        (client->req_len << 2) - SIZEOF(xRecordUnregisterClientsReq) !=
        4 * stuff->nClients)
        return BadLength;
    VERIFY_CONTEXT(pContext, stuff->context, client);
    err = RecordSanityCheckClientSpecifiers(client, (XID *) &stuff[1],
                                            stuff->nClients, 0);
    if (err != Success)
        return err;

    nClients = stuff->nClients;
    pCanonClients = RecordCanonicalizeClientSpecifiers((XID *) &stuff[1],
                                                       &nClients, 0);
    if (!pCanonClients)
        return BadAlloc;

    for (i = 0; i < nClients; i++) {
        RecordDeleteClientFromContext(pContext, pCanonClients[i]);
    }
    if (pCanonClients != (XID *) &stuff[1])
        free(pCanonClients);
    return Success;
}                               /* ProcRecordUnregisterClients */

/****************************************************************************/

/* stuff for GetContext */

/* This is a tactical structure used to hold the xRecordRanges as they are
 * being reconstituted from the sets in the RCAPs.
 */

typedef struct {
    xRecordRange *pRanges;      /* array of xRecordRanges for one RCAP */
    int size;                   /* number of elements in pRanges, >= nRanges */
    int nRanges;                /* number of occupied element of pRanges */
} GetContextRangeInfoRec, *GetContextRangeInfoPtr;

/* RecordAllocRanges
 *
 * Arguments:
 *	pri is a pointer to a GetContextRangeInfoRec to allocate for.
 *	nRanges is the number of xRecordRanges desired for pri.
 *
 * Returns: BadAlloc if a memory allocation error occurred, else Success.
 *
 * Side Effects:
 *	If Success is returned, pri->pRanges points to at least nRanges
 *	ranges.  pri->nRanges is set to nRanges.  pri->size is the actual
 *	number of ranges.  Newly allocated ranges are zeroed.
 */
static int
RecordAllocRanges(GetContextRangeInfoPtr pri, int nRanges)
{
    int newsize;
    xRecordRange *pNewRange;

#define SZINCR 8

    newsize = max(pri->size + SZINCR, nRanges);
    pNewRange = reallocarray(pri->pRanges, newsize, sizeof(xRecordRange));
    if (!pNewRange)
        return BadAlloc;

    pri->pRanges = pNewRange;
    pri->size = newsize;
    memset(&pri->pRanges[pri->size - SZINCR], 0, SZINCR * sizeof(xRecordRange));
    if (pri->nRanges < nRanges)
        pri->nRanges = nRanges;
    return Success;
}                               /* RecordAllocRanges */

/* RecordConvertSetToRanges
 *
 * Arguments:
 *	pSet is the set to be converted.
 *	pri is where the result should be stored.
 *	byteoffset is the offset from the start of an xRecordRange of the
 *	  two vales (first, last) we are interested in.
 *	card8 is TRUE if the vales are one byte each and FALSE if two bytes
 *	  each.
 *	imax is the largest set value to store in pri->pRanges.
 *	pStartIndex, if non-NULL, is the index of the first range in
 *	  pri->pRanges that should be stored to.  If NULL,
 *	  start at index 0.
 *
 * Returns: BadAlloc if a memory allocation error occurred, else Success.
 *
 * Side Effects:
 *	If Success is returned, the slice of pri->pRanges indicated by
 *	byteoffset and card8 is filled in with the intervals from pSet.
 *	if pStartIndex was non-NULL, *pStartIndex is filled in with one
 *	more than the index of the last xRecordRange that was touched.
 */
static int
RecordConvertSetToRanges(RecordSetPtr pSet,
                         GetContextRangeInfoPtr pri,
                         int byteoffset,
                         Bool card8, unsigned int imax, int *pStartIndex)
{
    int nRanges;
    RecordSetIteratePtr pIter = NULL;
    RecordSetInterval interval;
    CARD8 *pCARD8;
    CARD16 *pCARD16;
    int err;

    if (!pSet)
        return Success;

    nRanges = pStartIndex ? *pStartIndex : 0;
    while ((pIter = RecordIterateSet(pSet, pIter, &interval))) {
        if (interval.first > imax)
            break;
        if (interval.last > imax)
            interval.last = imax;
        nRanges++;
        if (nRanges > pri->size) {
            err = RecordAllocRanges(pri, nRanges);
            if (err != Success)
                return err;
        }
        else
            pri->nRanges = max(pri->nRanges, nRanges);
        if (card8) {
            pCARD8 = ((CARD8 *) &pri->pRanges[nRanges - 1]) + byteoffset;
            *pCARD8++ = interval.first;
            *pCARD8 = interval.last;
        }
        else {
            pCARD16 = (CARD16 *)
                (((char *) &pri->pRanges[nRanges - 1]) + byteoffset);
            *pCARD16++ = interval.first;
            *pCARD16 = interval.last;
        }
    }
    if (pStartIndex)
        *pStartIndex = nRanges;
    return Success;
}                               /* RecordConvertSetToRanges */

/* RecordConvertMinorOpInfoToRanges
 *
 * Arguments:
 *	pMinOpInfo is the minor opcode info to convert to xRecordRanges.
 *	pri is where the result should be stored.
 *	byteoffset is the offset from the start of an xRecordRange of the
 *	  four vales (CARD8 major_first, CARD8 major_last,
 *	  CARD16 minor_first, CARD16 minor_last) we are going to store.
 *
 * Returns: BadAlloc if a memory allocation error occurred, else Success.
 *
 * Side Effects:
 *	If Success is returned, the slice of pri->pRanges indicated by
 *	byteoffset is filled in with the information from pMinOpInfo.
 */
static int
RecordConvertMinorOpInfoToRanges(RecordMinorOpPtr pMinOpInfo,
                                 GetContextRangeInfoPtr pri, int byteoffset)
{
    int nsets;
    int start;
    int i;
    int err;

    if (!pMinOpInfo)
        return Success;

    nsets = pMinOpInfo->count;
    pMinOpInfo++;
    start = 0;
    for (i = 0; i < nsets; i++) {
        int j, s;

        s = start;
        err = RecordConvertSetToRanges(pMinOpInfo[i].major.pMinOpSet, pri,
                                       byteoffset + 2, FALSE, 65535, &start);
        if (err != Success)
            return err;
        for (j = s; j < start; j++) {
            CARD8 *pCARD8 = ((CARD8 *) &pri->pRanges[j]) + byteoffset;

            *pCARD8++ = pMinOpInfo[i].major.first;
            *pCARD8 = pMinOpInfo[i].major.last;
        }
    }
    return Success;
}                               /* RecordConvertMinorOpInfoToRanges */

/* RecordSwapRanges
 *
 * Arguments:
 *	pRanges is an array of xRecordRanges.
 *	nRanges is the number of elements in pRanges.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	The 16 bit fields of each xRecordRange are byte swapped.
 */
static void
RecordSwapRanges(xRecordRange * pRanges, int nRanges)
{
    int i;

    for (i = 0; i < nRanges; i++, pRanges++) {
        swaps(&pRanges->extRequestsMinorFirst);
        swaps(&pRanges->extRequestsMinorLast);
        swaps(&pRanges->extRepliesMinorFirst);
        swaps(&pRanges->extRepliesMinorLast);
    }
}                               /* RecordSwapRanges */

static int
ProcRecordGetContext(ClientPtr client)
{
    RecordContextPtr pContext;

    REQUEST(xRecordGetContextReq);
    xRecordGetContextReply rep;
    RecordClientsAndProtocolPtr pRCAP;
    int nRCAPs = 0;
    GetContextRangeInfoPtr pRangeInfo;
    GetContextRangeInfoPtr pri;
    int i;
    int err;
    CARD32 nClients, length;

    REQUEST_SIZE_MATCH(xRecordGetContextReq);
    VERIFY_CONTEXT(pContext, stuff->context, client);

    /* how many RCAPs are there on this context? */

    for (pRCAP = pContext->pListOfRCAP; pRCAP; pRCAP = pRCAP->pNextRCAP)
        nRCAPs++;

    /* allocate and initialize space for record range info */

    pRangeInfo = xallocarray(nRCAPs, sizeof(GetContextRangeInfoRec));
    if (!pRangeInfo && nRCAPs > 0)
        return BadAlloc;
    for (i = 0; i < nRCAPs; i++) {
        pRangeInfo[i].pRanges = NULL;
        pRangeInfo[i].size = 0;
        pRangeInfo[i].nRanges = 0;
    }

    /* convert the RCAP (internal) representation of the recorded protocol
     * to the wire protocol (external) representation, storing the information
     * for the ith RCAP in pri[i]
     */

    for (pRCAP = pContext->pListOfRCAP, pri = pRangeInfo;
         pRCAP; pRCAP = pRCAP->pNextRCAP, pri++) {
        xRecordRange rr;

        err = RecordConvertSetToRanges(pRCAP->pRequestMajorOpSet, pri,
                                       offset_of(rr, coreRequestsFirst), TRUE,
                                       127, NULL);
        if (err != Success)
            goto bailout;

        err = RecordConvertSetToRanges(pRCAP->pReplyMajorOpSet, pri,
                                       offset_of(rr, coreRepliesFirst), TRUE,
                                       127, NULL);
        if (err != Success)
            goto bailout;

        err = RecordConvertSetToRanges(pRCAP->pDeliveredEventSet, pri,
                                       offset_of(rr, deliveredEventsFirst),
                                       TRUE, 255, NULL);
        if (err != Success)
            goto bailout;

        err = RecordConvertSetToRanges(pRCAP->pDeviceEventSet, pri,
                                       offset_of(rr, deviceEventsFirst), TRUE,
                                       255, NULL);
        if (err != Success)
            goto bailout;

        err = RecordConvertSetToRanges(pRCAP->pErrorSet, pri,
                                       offset_of(rr, errorsFirst), TRUE, 255,
                                       NULL);
        if (err != Success)
            goto bailout;

        err = RecordConvertMinorOpInfoToRanges(pRCAP->pRequestMinOpInfo,
                                               pri, offset_of(rr,
                                                              extRequestsMajorFirst));
        if (err != Success)
            goto bailout;

        err = RecordConvertMinorOpInfoToRanges(pRCAP->pReplyMinOpInfo,
                                               pri, offset_of(rr,
                                                              extRepliesMajorFirst));
        if (err != Success)
            goto bailout;

        if (pRCAP->clientStarted || pRCAP->clientDied) {
            if (pri->nRanges == 0)
                RecordAllocRanges(pri, 1);
            pri->pRanges[0].clientStarted = pRCAP->clientStarted;
            pri->pRanges[0].clientDied = pRCAP->clientDied;
        }
    }

    /* calculate number of clients and reply length */

    nClients = 0;
    length = 0;
    for (pRCAP = pContext->pListOfRCAP, pri = pRangeInfo;
         pRCAP; pRCAP = pRCAP->pNextRCAP, pri++) {
        nClients += pRCAP->numClients;
        length += pRCAP->numClients *
            (bytes_to_int32(sizeof(xRecordClientInfo)) +
             pri->nRanges * bytes_to_int32(sizeof(xRecordRange)));
    }

    /* write the reply header */

    rep = (xRecordGetContextReply) {
        .type = X_Reply,
        .enabled = pContext->pRecordingClient != NULL,
        .sequenceNumber = client->sequence,
        .length = length,
        .elementHeader = pContext->elemHeaders,
        .nClients = nClients
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.nClients);
    }
    WriteToClient(client, sizeof(xRecordGetContextReply), &rep);

    /* write all the CLIENT_INFOs */

    for (pRCAP = pContext->pListOfRCAP, pri = pRangeInfo;
         pRCAP; pRCAP = pRCAP->pNextRCAP, pri++) {
        xRecordClientInfo rci;

        rci.nRanges = pri->nRanges;
        if (client->swapped) {
            swapl(&rci.nRanges);
            RecordSwapRanges(pri->pRanges, pri->nRanges);
        }
        for (i = 0; i < pRCAP->numClients; i++) {
            rci.clientResource = pRCAP->pClientIDs[i];
            if (client->swapped)
                swapl(&rci.clientResource);
            WriteToClient(client, sizeof(xRecordClientInfo), &rci);
            WriteToClient(client, sizeof(xRecordRange) * pri->nRanges,
                          pri->pRanges);
        }
    }
    err = Success;

 bailout:
    for (i = 0; i < nRCAPs; i++) {
        free(pRangeInfo[i].pRanges);
    }
    free(pRangeInfo);
    return err;
}                               /* ProcRecordGetContext */

static int
ProcRecordEnableContext(ClientPtr client)
{
    RecordContextPtr pContext;

    REQUEST(xRecordEnableContextReq);
    int i;
    RecordClientsAndProtocolPtr pRCAP;

    REQUEST_SIZE_MATCH(xRecordGetContextReq);
    VERIFY_CONTEXT(pContext, stuff->context, client);
    if (pContext->pRecordingClient)
        return BadMatch;        /* already enabled */

    /* install record hooks for each RCAP */

    for (pRCAP = pContext->pListOfRCAP; pRCAP; pRCAP = pRCAP->pNextRCAP) {
        int err = RecordInstallHooks(pRCAP, 0);

        if (err != Success) {   /* undo the previous installs */
            RecordClientsAndProtocolPtr pUninstallRCAP;

            for (pUninstallRCAP = pContext->pListOfRCAP;
                 pUninstallRCAP != pRCAP;
                 pUninstallRCAP = pUninstallRCAP->pNextRCAP) {
                RecordUninstallHooks(pUninstallRCAP, 0);
            }
            return err;
        }
    }

    /* Disallow further request processing on this connection until
     * the context is disabled.
     */
    IgnoreClient(client);
    pContext->pRecordingClient = client;

    /* Don't allow the data connection to record itself; unregister it. */
    RecordDeleteClientFromContext(pContext,
                                  pContext->pRecordingClient->clientAsMask);

    /* move the newly enabled context to the front part of ppAllContexts,
     * where all the enabled contexts are
     */
    i = RecordFindContextOnAllContexts(pContext);
    assert(i >= numEnabledContexts);
    if (i != numEnabledContexts) {
        ppAllContexts[i] = ppAllContexts[numEnabledContexts];
        ppAllContexts[numEnabledContexts] = pContext;
    }

    ++numEnabledContexts;
    assert(numEnabledContexts > 0);

    /* send StartOfData */
    RecordAProtocolElement(pContext, NULL, XRecordStartOfData, NULL, 0, 0, 0);
    RecordFlushReplyBuffer(pContext, NULL, 0, NULL, 0);
    return Success;
}                               /* ProcRecordEnableContext */

/* RecordDisableContext
 *
 * Arguments:
 *	pContext is the context to disable.
 *	nRanges is the number of elements in pRanges.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	If the context was enabled, it is disabled.  An EndOfData
 *	message is sent to the recording client.  Recording hooks for
 *	this context are uninstalled.  The context is moved to the
 *	rear part of the ppAllContexts array.  numEnabledContexts is
 *	decremented.  Request processing for the formerly recording client
 *	is resumed.
 */
static void
RecordDisableContext(RecordContextPtr pContext)
{
    RecordClientsAndProtocolPtr pRCAP;
    int i;

    if (!pContext->pRecordingClient)
        return;
    if (!pContext->pRecordingClient->clientGone) {
        RecordAProtocolElement(pContext, NULL, XRecordEndOfData, NULL, 0, 0, 0);
        RecordFlushReplyBuffer(pContext, NULL, 0, NULL, 0);
    }
    /* Re-enable request processing on this connection. */
    AttendClient(pContext->pRecordingClient);

    for (pRCAP = pContext->pListOfRCAP; pRCAP; pRCAP = pRCAP->pNextRCAP) {
        RecordUninstallHooks(pRCAP, 0);
    }

    pContext->pRecordingClient = NULL;

    /* move the newly disabled context to the rear part of ppAllContexts,
     * where all the disabled contexts are
     */
    i = RecordFindContextOnAllContexts(pContext);
    assert(i != -1);
    assert(i < numEnabledContexts);
    if (i != (numEnabledContexts - 1)) {
        ppAllContexts[i] = ppAllContexts[numEnabledContexts - 1];
        ppAllContexts[numEnabledContexts - 1] = pContext;
    }
    --numEnabledContexts;
    assert(numEnabledContexts >= 0);
}                               /* RecordDisableContext */

static int
ProcRecordDisableContext(ClientPtr client)
{
    RecordContextPtr pContext;

    REQUEST(xRecordDisableContextReq);

    REQUEST_SIZE_MATCH(xRecordDisableContextReq);
    VERIFY_CONTEXT(pContext, stuff->context, client);
    RecordDisableContext(pContext);
    return Success;
}                               /* ProcRecordDisableContext */

/* RecordDeleteContext
 *
 * Arguments:
 *	value is the context to delete.
 *	id is its resource ID.
 *
 * Returns: Success.
 *
 * Side Effects:
 *	Disables the context, frees all associated memory, and removes
 *	it from the ppAllContexts array.
 */
static int
RecordDeleteContext(void *value, XID id)
{
    int i;
    RecordContextPtr pContext = (RecordContextPtr) value;
    RecordClientsAndProtocolPtr pRCAP;

    RecordDisableContext(pContext);

    /*  Remove all the clients from all the RCAPs.
     *  As a result, the RCAPs will be freed.
     */

    while ((pRCAP = pContext->pListOfRCAP)) {
        int numClients = pRCAP->numClients;

        /* when the last client is deleted, the RCAP will go away. */
        while (numClients--) {
            RecordDeleteClientFromRCAP(pRCAP, numClients);
        }
    }

    /* remove context from AllContexts list */

    if (-1 != (i = RecordFindContextOnAllContexts(pContext))) {
        ppAllContexts[i] = ppAllContexts[numContexts - 1];
        if (--numContexts == 0) {
            free(ppAllContexts);
            ppAllContexts = NULL;
        }
    }
    free(pContext);

    return Success;
}                               /* RecordDeleteContext */

static int
ProcRecordFreeContext(ClientPtr client)
{
    RecordContextPtr pContext;

    REQUEST(xRecordFreeContextReq);

    REQUEST_SIZE_MATCH(xRecordFreeContextReq);
    VERIFY_CONTEXT(pContext, stuff->context, client);
    FreeResource(stuff->context, RT_NONE);
    return Success;
}                               /* ProcRecordFreeContext */

static int
ProcRecordDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_RecordQueryVersion:
        return ProcRecordQueryVersion(client);
    case X_RecordCreateContext:
        return ProcRecordCreateContext(client);
    case X_RecordRegisterClients:
        return ProcRecordRegisterClients(client);
    case X_RecordUnregisterClients:
        return ProcRecordUnregisterClients(client);
    case X_RecordGetContext:
        return ProcRecordGetContext(client);
    case X_RecordEnableContext:
        return ProcRecordEnableContext(client);
    case X_RecordDisableContext:
        return ProcRecordDisableContext(client);
    case X_RecordFreeContext:
        return ProcRecordFreeContext(client);
    default:
        return BadRequest;
    }
}                               /* ProcRecordDispatch */

static int _X_COLD
SProcRecordQueryVersion(ClientPtr client)
{
    REQUEST(xRecordQueryVersionReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xRecordQueryVersionReq);
    swaps(&stuff->majorVersion);
    swaps(&stuff->minorVersion);
    return ProcRecordQueryVersion(client);
}                               /* SProcRecordQueryVersion */

static int _X_COLD
SwapCreateRegister(ClientPtr client, xRecordRegisterClientsReq * stuff)
{
    int i;
    XID *pClientID;

    swapl(&stuff->context);
    swapl(&stuff->nClients);
    swapl(&stuff->nRanges);
    pClientID = (XID *) &stuff[1];
    if (stuff->nClients >
        client->req_len - bytes_to_int32(sz_xRecordRegisterClientsReq))
        return BadLength;
    for (i = 0; i < stuff->nClients; i++, pClientID++) {
        swapl(pClientID);
    }
    if (stuff->nRanges >
        (client->req_len - bytes_to_int32(sz_xRecordRegisterClientsReq)
        - stuff->nClients) / bytes_to_int32(sz_xRecordRange))
        return BadLength;
    RecordSwapRanges((xRecordRange *) pClientID, stuff->nRanges);
    return Success;
}                               /* SwapCreateRegister */

static int _X_COLD
SProcRecordCreateContext(ClientPtr client)
{
    REQUEST(xRecordCreateContextReq);
    int status;

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xRecordCreateContextReq);
    if ((status = SwapCreateRegister(client, (void *) stuff)) != Success)
        return status;
    return ProcRecordCreateContext(client);
}                               /* SProcRecordCreateContext */

static int _X_COLD
SProcRecordRegisterClients(ClientPtr client)
{
    REQUEST(xRecordRegisterClientsReq);
    int status;

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xRecordRegisterClientsReq);
    if ((status = SwapCreateRegister(client, (void *) stuff)) != Success)
        return status;
    return ProcRecordRegisterClients(client);
}                               /* SProcRecordRegisterClients */

static int _X_COLD
SProcRecordUnregisterClients(ClientPtr client)
{
    REQUEST(xRecordUnregisterClientsReq);

    swaps(&stuff->length);
    REQUEST_AT_LEAST_SIZE(xRecordUnregisterClientsReq);
    swapl(&stuff->context);
    swapl(&stuff->nClients);
    SwapRestL(stuff);
    return ProcRecordUnregisterClients(client);
}                               /* SProcRecordUnregisterClients */

static int _X_COLD
SProcRecordGetContext(ClientPtr client)
{
    REQUEST(xRecordGetContextReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xRecordGetContextReq);
    swapl(&stuff->context);
    return ProcRecordGetContext(client);
}                               /* SProcRecordGetContext */

static int _X_COLD
SProcRecordEnableContext(ClientPtr client)
{
    REQUEST(xRecordEnableContextReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xRecordEnableContextReq);
    swapl(&stuff->context);
    return ProcRecordEnableContext(client);
}                               /* SProcRecordEnableContext */

static int _X_COLD
SProcRecordDisableContext(ClientPtr client)
{
    REQUEST(xRecordDisableContextReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xRecordDisableContextReq);
    swapl(&stuff->context);
    return ProcRecordDisableContext(client);
}                               /* SProcRecordDisableContext */

static int _X_COLD
SProcRecordFreeContext(ClientPtr client)
{
    REQUEST(xRecordFreeContextReq);

    swaps(&stuff->length);
    REQUEST_SIZE_MATCH(xRecordFreeContextReq);
    swapl(&stuff->context);
    return ProcRecordFreeContext(client);
}                               /* SProcRecordFreeContext */

static int _X_COLD
SProcRecordDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_RecordQueryVersion:
        return SProcRecordQueryVersion(client);
    case X_RecordCreateContext:
        return SProcRecordCreateContext(client);
    case X_RecordRegisterClients:
        return SProcRecordRegisterClients(client);
    case X_RecordUnregisterClients:
        return SProcRecordUnregisterClients(client);
    case X_RecordGetContext:
        return SProcRecordGetContext(client);
    case X_RecordEnableContext:
        return SProcRecordEnableContext(client);
    case X_RecordDisableContext:
        return SProcRecordDisableContext(client);
    case X_RecordFreeContext:
        return SProcRecordFreeContext(client);
    default:
        return BadRequest;
    }
}                               /* SProcRecordDispatch */

/* RecordConnectionSetupInfo
 *
 * Arguments:
 *	pContext is an enabled context that specifies recording of
 *	  connection setup info.
 *	pci holds the connection setup info.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	The connection setup info is sent to the recording client.
 */
static void
RecordConnectionSetupInfo(RecordContextPtr pContext, NewClientInfoRec * pci)
{
    int prefixsize = SIZEOF(xConnSetupPrefix);
    int restsize = pci->prefix->length * 4;

    if (pci->client->swapped) {
        char *pConnSetup = (char *) malloc(prefixsize + restsize);

        if (!pConnSetup)
            return;
        SwapConnSetupPrefix(pci->prefix, (xConnSetupPrefix *) pConnSetup);
        SwapConnSetupInfo((char *) pci->setup,
                          (char *) (pConnSetup + prefixsize));
        RecordAProtocolElement(pContext, pci->client, XRecordClientStarted,
                               (void *) pConnSetup, prefixsize + restsize, 0,
                               0);
        free(pConnSetup);
    }
    else {
        /* don't alloc and copy as in the swapped case; just send the
         * data in two pieces
         */
        RecordAProtocolElement(pContext, pci->client, XRecordClientStarted,
                               (void *) pci->prefix, prefixsize, 0, restsize);
        RecordAProtocolElement(pContext, pci->client, XRecordClientStarted,
                               (void *) pci->setup, restsize, 0,
                               /* continuation */ -1);
    }
}                               /* RecordConnectionSetupInfo */

/* RecordDeleteContext
 *
 * Arguments:
 *	pcbl is &ClientStateCallback.
 *	nullata is NULL.
 *	calldata is a pointer to a NewClientInfoRec (include/dixstruct.h)
 *	which contains information about client state changes.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	If a new client has connected and any contexts have specified
 *	XRecordFutureClients, the new client is registered on those contexts.
 *	If any of those contexts specify recording of the connection setup
 *	info, it is recorded.
 *
 *	If an existing client has disconnected, it is deleted from any
 *	contexts that it was registered on.  If any of those contexts
 *	specified XRecordClientDied, they record a ClientDied protocol element.
 *	If the disconnectiong client happened to be the data connection of an
 *	enabled context, the context is disabled.
 */

static void
RecordAClientStateChange(CallbackListPtr *pcbl, void *nulldata,
                         void *calldata)
{
    NewClientInfoRec *pci = (NewClientInfoRec *) calldata;
    int i;
    ClientPtr pClient = pci->client;
    RecordContextPtr *ppAllContextsCopy = NULL;
    int numContextsCopy = 0;

    switch (pClient->clientState) {
    case ClientStateRunning:   /* new client */
        for (i = 0; i < numContexts; i++) {
            RecordClientsAndProtocolPtr pRCAP;
            RecordContextPtr pContext = ppAllContexts[i];

            if ((pRCAP = RecordFindClientOnContext(pContext,
                                                   XRecordFutureClients, NULL)))
            {
                RecordAddClientToRCAP(pRCAP, pClient->clientAsMask);
                if (pContext->pRecordingClient && pRCAP->clientStarted)
                    RecordConnectionSetupInfo(pContext, pci);
            }
        }
        break;

    case ClientStateGone:
    case ClientStateRetained:  /* client disconnected */

        /* RecordDisableContext modifies contents of ppAllContexts. */
        if (!(numContextsCopy = numContexts))
            break;
        ppAllContextsCopy = xallocarray(numContextsCopy,
                                        sizeof(RecordContextPtr));
        assert(ppAllContextsCopy);
        memcpy(ppAllContextsCopy, ppAllContexts,
               numContextsCopy * sizeof(RecordContextPtr));

        for (i = 0; i < numContextsCopy; i++) {
            RecordClientsAndProtocolPtr pRCAP;
            RecordContextPtr pContext = ppAllContextsCopy[i];
            int pos;

            if (pContext->pRecordingClient == pClient)
                RecordDisableContext(pContext);
            if ((pRCAP = RecordFindClientOnContext(pContext,
                                                   pClient->clientAsMask,
                                                   &pos))) {
                if (pContext->pRecordingClient && pRCAP->clientDied)
                    RecordAProtocolElement(pContext, pClient,
                                           XRecordClientDied, NULL, 0, 0, 0);
                RecordDeleteClientFromRCAP(pRCAP, pos);
            }
        }

        free(ppAllContextsCopy);
        break;

    default:
        break;
    }                           /* end switch on client state */
}                               /* RecordAClientStateChange */

/* RecordCloseDown
 *
 * Arguments:
 *	extEntry is the extension information for RECORD.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Performs any cleanup needed by RECORD at server shutdown time.
 *	
 */
static void
RecordCloseDown(ExtensionEntry * extEntry)
{
    DeleteCallback(&ClientStateCallback, RecordAClientStateChange, NULL);
}                               /* RecordCloseDown */

/* RecordExtensionInit
 *
 * Arguments: none.
 *
 * Returns: nothing.
 *
 * Side Effects:
 *	Enables the RECORD extension if possible.
 */
void
RecordExtensionInit(void)
{
    ExtensionEntry *extentry;

    RTContext = CreateNewResourceType(RecordDeleteContext, "RecordContext");
    if (!RTContext)
        return;

    if (!dixRegisterPrivateKey(RecordClientPrivateKey, PRIVATE_CLIENT, 0))
        return;

    ppAllContexts = NULL;
    numContexts = numEnabledContexts = numEnabledRCAPs = 0;

    if (!AddCallback(&ClientStateCallback, RecordAClientStateChange, NULL))
        return;

    extentry = AddExtension(RECORD_NAME, RecordNumEvents, RecordNumErrors,
                            ProcRecordDispatch, SProcRecordDispatch,
                            RecordCloseDown, StandardMinorOpcode);
    if (!extentry) {
        DeleteCallback(&ClientStateCallback, RecordAClientStateChange, NULL);
        return;
    }
    SetResourceTypeErrorValue(RTContext,
                              extentry->errorBase + XRecordBadContext);

}                               /* RecordExtensionInit */
