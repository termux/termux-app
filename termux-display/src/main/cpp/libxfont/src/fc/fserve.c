/*

Copyright 1990, 1998  The Open Group

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

*/

/*
 * Copyright 1990 Network Computing Devices
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices, or Digital
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 *
 * NETWORK COMPUTING DEVICES, AND DIGITAL AND DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES,
 * OR DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 * Author:  	Dave Lemke, Network Computing Devices, Inc
 */
/*
 * font server specific font access
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include "src/util/replace.h"

#ifdef WIN32
#define _WILLWINSOCK_
#endif
#define FONT_t
#define TRANS_CLIENT
#include	"X11/Xtrans/Xtrans.h"
#include	"X11/Xpoll.h"
#include	<X11/fonts/FS.h>
#include	<X11/fonts/FSproto.h>
#include	<X11/X.h>
#include	<X11/Xos.h>
#include	<X11/fonts/fontmisc.h>
#include	<X11/fonts/fontstruct.h>
#include	"fservestr.h"
#include	<X11/fonts/fontutil.h>
#include	<errno.h>
#include	<limits.h>

#include	<time.h>
#define Time_t time_t

#ifdef NCD
#include	<ncd/nvram.h>
#endif

#include <stddef.h>

#ifndef MIN
#define MIN(a,b)    ((a)<(b)?(a):(b))
#endif
#define TimeCmp(a,c,b)	((int) ((a) - (b)) c 0)

#define NONZEROMETRICS(pci) ((pci)->leftSideBearing || \
			     (pci)->rightSideBearing || \
			     (pci)->ascent || \
			     (pci)->descent || \
			     (pci)->characterWidth)

/*
 * SIZEOF(r) is in bytes, length fields in the protocol are in 32-bit words,
 * so this converts for doing size comparisons.
 */
#define LENGTHOF(r)	(SIZEOF(r) >> 2)

/* Somewhat arbitrary limit on maximum reply size we'll try to read. */
#define MAX_REPLY_LENGTH	((64 * 1024 * 1024) >> 2)

static int fs_read_glyphs ( FontPathElementPtr fpe, FSBlockDataPtr blockrec );
static int fs_read_list ( FontPathElementPtr fpe, FSBlockDataPtr blockrec );
static int fs_read_list_info ( FontPathElementPtr fpe,
			       FSBlockDataPtr blockrec );

static void fs_block_handler ( void *wt );

static int fs_wakeup ( FontPathElementPtr fpe );

/*
 * List of all FPEs
 */
static FSFpePtr fs_fpes;
/*
 * Union of all FPE blockStates
 */
static CARD32	fs_blockState;

static int _fs_restart_connection ( FSFpePtr conn );
static void fs_send_query_bitmaps ( FontPathElementPtr fpe,
				   FSBlockDataPtr blockrec );
static int fs_send_close_font (FSFpePtr conn, Font id);
static void fs_client_died ( pointer client, FontPathElementPtr fpe );
static void _fs_client_access ( FSFpePtr conn, pointer client, Bool sync );
static void _fs_client_resolution ( FSFpePtr conn );
static fsGenericReply *fs_get_reply (FSFpePtr conn, int *error);
static int fs_await_reply (FSFpePtr conn);
static void _fs_do_blocked (FSFpePtr conn);
static void fs_cleanup_bfont (FSFpePtr conn, FSBlockedFontPtr bfont);

char _fs_glyph_undefined;
char _fs_glyph_requested;
static char _fs_glyph_zero_length;

static int  generationCount;

static int FontServerRequestTimeout = 30 * 1000;

static void
_fs_close_server (FSFpePtr conn);

static FSFpePtr
_fs_init_conn (const char *servername, FontPathElementPtr fpe);

static int
_fs_wait_connect (FSFpePtr conn);

static int
_fs_send_init_packets (FSFpePtr conn);

static void
_fs_check_reconnect (FSFpePtr conn);

static void
_fs_start_reconnect (FSFpePtr conn);

static void
_fs_free_conn (FSFpePtr conn);

static int
fs_free_fpe(FontPathElementPtr fpe);

static void
fs_fd_handler(int fd, void *data);

/*
 * Font server access
 *
 * the basic idea for the non-blocking access is to have the function
 * called multiple times until the actual data is returned, instead
 * of ClientBlocked.
 *
 * the first call to the function will cause the request to be sent to
 * the font server, and a block record to be stored in the fpe's list
 * of outstanding requests.  the FS block handler also sticks the
 * proper set of fd's into the select mask.  when data is ready to be
 * read in, the FS wakeup handler will be hit.  this will read the
 * data off the wire into the proper block record, and then signal the
 * client that caused the block so that it can restart.  it will then
 * call the access function again, which will realize that the data has
 * arrived and return it.
 */


#ifdef DEBUG
static void
_fs_add_req_log(FSFpePtr conn, int opcode)
{
    conn->current_seq++;
    fprintf (stderr, "\t\tRequest: %5d Opcode: %2d\n",
	     conn->current_seq, opcode);
    conn->reqbuffer[conn->reqindex].opcode = opcode;
    conn->reqbuffer[conn->reqindex].sequence = conn->current_seq;
    conn->reqindex++;
    if (conn->reqindex == REQUEST_LOG_SIZE)
	conn->reqindex = 0;
}

static void
_fs_add_rep_log (FSFpePtr conn, fsGenericReply *rep)
{
    int	    i;

    for (i = 0; i < REQUEST_LOG_SIZE; i++)
	if (conn->reqbuffer[i].sequence == rep->sequenceNumber)
	    break;
    if (i == REQUEST_LOG_SIZE)
	fprintf (stderr, "\t\t\t\t\tReply:  %5d Opcode: unknown\n",
		 rep->sequenceNumber);
    else
	fprintf (stderr, "\t\t\t\t\tReply:  %5d Opcode: %d\n",
		 rep->sequenceNumber,
		 conn->reqbuffer[i].opcode);
}

#define _fs_reply_failed(rep, name, op) do {                            \
    if (rep) {                                                          \
        if (rep->type == FS_Error)                                      \
            fprintf (stderr, "Error: %d Request: %s\n",                 \
                     ((fsError *)rep)->request, #name);                 \
        else                                                            \
            fprintf (stderr, "Bad Length for %s Reply: %u %s %d\n",     \
                     #name, (unsigned) rep->length, op, LENGTHOF(name));\
    }                                                                   \
} while (0)

#else
#define _fs_add_req_log(conn,op)    ((conn)->current_seq++)
#define _fs_add_rep_log(conn,rep)
#define _fs_reply_failed(rep,name,op)
#endif

static Bool
fs_name_check(const char *name)
{
    /* Just make sure there is a protocol/ prefix */
    return (name && *name != '/' && strchr(name, '/'));
}

static void
_fs_client_resolution(FSFpePtr conn)
{
    fsSetResolutionReq srreq;
    int         num_res;
    FontResolutionPtr res;

    res = GetClientResolutions(&num_res);

    if (num_res) {
	srreq.reqType = FS_SetResolution;
	srreq.num_resolutions = num_res;
	srreq.length = (SIZEOF(fsSetResolutionReq) +
			(num_res * SIZEOF(fsResolution)) + 3) >> 2;

	_fs_add_req_log(conn, FS_SetResolution);
	if (_fs_write(conn, (char *) &srreq, SIZEOF(fsSetResolutionReq)) != -1)
	    (void)_fs_write_pad(conn, (char *) res,
				(num_res * SIZEOF(fsResolution)));
    }
}

/*
 * close font server and remove any state associated with
 * this connection - this includes any client records.
 */

static void
fs_close_conn(FSFpePtr conn)
{
    FSClientPtr	client, nclient;

    _fs_close_server (conn);

    for (client = conn->clients; client; client = nclient)
    {
	nclient = client->next;
	free (client);
    }
    conn->clients = NULL;
}

/*
 * the wakeup handlers have to be set when the FPE is open, and not
 * removed until it is freed, in order to handle unexpected data, like
 * events
 */
/* ARGSUSED */
static int
fs_init_fpe(FontPathElementPtr fpe)
{
    FSFpePtr    conn;
    const char  *name;
    int         err;
    int		ret;

    /* open font server */
    /* create FS specific fpe info */
    name = fpe->name;

    /* hack for old style names */
    if (*name == ':')
	name++;			/* skip ':' */

    conn = _fs_init_conn (name, fpe);
    if (!conn)
	err = AllocError;
    else
    {
	err = init_fs_handlers2 (fpe, fs_block_handler);
	if (err != Successful)
	{
	    _fs_free_conn (conn);
	    err = AllocError;
	}
	else
	{
	    fpe->private = conn;
	    conn->next = fs_fpes;
	    fs_fpes = conn;
	    ret = _fs_wait_connect (conn);
	    if (ret != FSIO_READY)
	    {
		fs_free_fpe (fpe);
		err = BadFontPath;
	    }
	    else
		err = Successful;
	}
    }

    if (err == Successful)
    {
#ifdef NCD
	if (configData.ExtendedFontDiags)
	    printf("Connected to font server \"%s\"\n", name);
#endif
#ifdef DEBUG
	fprintf (stderr, "connected to FS \"%s\"\n", name);
#endif
    }
    else
    {
#ifdef DEBUG
	fprintf(stderr, "failed to connect to FS \"%s\" %d\n", name, err);
#endif
#ifdef NCD
	if (configData.ExtendedFontDiags)
	    printf("Failed to connect to font server \"%s\"\n", name);
#endif
	;
    }
    return err;
}

static int
fs_reset_fpe(FontPathElementPtr fpe)
{
    (void) _fs_send_init_packets((FSFpePtr) fpe->private);
    return Successful;
}

/*
 * this shouldn't be called till all refs to the FPE are gone
 */

static int
fs_free_fpe(FontPathElementPtr fpe)
{
    FSFpePtr    conn = (FSFpePtr) fpe->private, *prev;

    /* unhook from chain of all font servers */
    for (prev = &fs_fpes; *prev; prev = &(*prev)->next)
    {
	if (*prev == conn)
	{
	    *prev = conn->next;
	    break;
	}
    }
    _fs_unmark_block (conn, conn->blockState);
    fs_close_conn(conn);
    remove_fs_handlers2(fpe, fs_block_handler, fs_fpes == 0);
    _fs_free_conn (conn);
    fpe->private = (pointer) 0;

#ifdef NCD
    if (configData.ExtendedFontDiags)
	printf("Disconnected from font server \"%s\"\n", fpe->name);
#endif
#ifdef DEBUG
    fprintf (stderr, "disconnect from FS \"%s\"\n", fpe->name);
#endif

    return Successful;
}

static      FSBlockDataPtr
fs_new_block_rec(FontPathElementPtr fpe, pointer client, int type)
{
    FSBlockDataPtr blockrec,
                *prev;
    FSFpePtr    conn = (FSFpePtr) fpe->private;
    int         size;

    switch (type) {
    case FS_OPEN_FONT:
	size = sizeof(FSBlockedFontRec);
	break;
    case FS_LOAD_GLYPHS:
	size = sizeof(FSBlockedGlyphRec);
	break;
    case FS_LIST_FONTS:
	size = sizeof(FSBlockedListRec);
	break;
    case FS_LIST_WITH_INFO:
	size = sizeof(FSBlockedListInfoRec);
	break;
    default:
	size = 0;
	break;
    }
    blockrec = malloc(sizeof(FSBlockDataRec) + size);
    if (!blockrec)
	return (FSBlockDataPtr) 0;
    blockrec->data = (pointer) (blockrec + 1);
    blockrec->client = client;
    blockrec->sequenceNumber = -1;
    blockrec->errcode = StillWorking;
    blockrec->type = type;
    blockrec->depending = 0;
    blockrec->next = (FSBlockDataPtr) 0;

    /* stick it on the end of the list (since its expected last) */
    for (prev = &conn->blockedRequests; *prev; prev = &(*prev)->next)
	;
    *prev = blockrec;

    return blockrec;
}

static void
_fs_set_pending_reply (FSFpePtr conn)
{
    FSBlockDataPtr  blockrec;

    for (blockrec = conn->blockedRequests; blockrec; blockrec = blockrec->next)
	if (blockrec->errcode == StillWorking)
	    break;
    if (blockrec)
    {
	conn->blockedReplyTime = GetTimeInMillis () + FontServerRequestTimeout;
	_fs_mark_block (conn, FS_PENDING_REPLY);
    }
    else
	_fs_unmark_block (conn, FS_PENDING_REPLY);
}

static void
_fs_remove_block_rec(FSFpePtr conn, FSBlockDataPtr blockrec)
{
    FSBlockDataPtr *prev;

    for (prev = &conn->blockedRequests; *prev; prev = &(*prev)->next)
	if (*prev == blockrec)
	{
	    *prev = blockrec->next;
	    break;
	}
    if (blockrec->type == FS_LOAD_GLYPHS)
    {
	FSBlockedGlyphPtr bglyph = (FSBlockedGlyphPtr)blockrec->data;
	if (bglyph->num_expected_ranges)
	    free(bglyph->expected_ranges);
    }
    free(blockrec);
    _fs_set_pending_reply (conn);
}

static void
_fs_signal_clients_depending(FSClientsDependingPtr *clients_depending)
{
    FSClientsDependingPtr p;

    while ((p = *clients_depending))
    {
	*clients_depending = p->next;
	ClientSignal(p->client);
	free(p);
    }
}

static int
_fs_add_clients_depending(FSClientsDependingPtr *clients_depending, pointer client)
{
    FSClientsDependingPtr   new, cd;

    for (; (cd = *clients_depending);
	 clients_depending = &(*clients_depending)->next)
    {
	if (cd->client == client)
	    return Suspended;
    }

    new = malloc (sizeof (FSClientsDependingRec));
    if (!new)
	return BadAlloc;

    new->client = client;
    new->next = 0;
    *clients_depending = new;
    return Suspended;
}

static void
conn_start_listening(FSFpePtr conn)
{
    if (!conn->fs_listening) {
	add_fs_fd(conn->fs_fd, fs_fd_handler, conn->fpe);
	conn->fs_listening = TRUE;
    }
}

static void
conn_stop_listening(FSFpePtr conn)
{
    if (conn->fs_listening) {
	remove_fs_fd(conn->fs_fd);
	conn->fs_listening = FALSE;
    }
}

/*
 * When a request is aborted due to a font server failure,
 * signal any depending clients to restart their dependent
 * requests
 */
static void
_fs_clean_aborted_blockrec(FSFpePtr conn, FSBlockDataPtr blockrec)
{
    switch(blockrec->type) {
    case FS_OPEN_FONT: {
	FSBlockedFontPtr bfont = (FSBlockedFontPtr)blockrec->data;

	fs_cleanup_bfont (conn, bfont);
	_fs_signal_clients_depending(&bfont->clients_depending);
	break;
    }
    case FS_LOAD_GLYPHS: {
	FSBlockedGlyphPtr bglyph = (FSBlockedGlyphPtr)blockrec->data;

	_fs_clean_aborted_loadglyphs(bglyph->pfont,
				     bglyph->num_expected_ranges,
				     bglyph->expected_ranges);
	_fs_signal_clients_depending(&bglyph->clients_depending);
	break;
    }
    case FS_LIST_FONTS:
	break;
    case FS_LIST_WITH_INFO: {
	FSBlockedListInfoPtr binfo;
	binfo = (FSBlockedListInfoPtr) blockrec->data;
	if (binfo->status == FS_LFWI_REPLY)
	    conn_start_listening(conn);
	_fs_free_props (&binfo->info);
    }
    default:
	break;
    }
}

static void
fs_abort_blockrec(FSFpePtr conn, FSBlockDataPtr blockrec)
{
    _fs_clean_aborted_blockrec (conn, blockrec);
    _fs_remove_block_rec (conn, blockrec);
}

/*
 * Tell the font server we've failed to complete an open and
 * then unload the partially created font
 */
static void
fs_cleanup_bfont (FSFpePtr conn, FSBlockedFontPtr bfont)
{
    if (bfont->pfont)
    {
	/* make sure the FS knows we choked on it */
	fs_send_close_font(conn, bfont->fontid);

	/*
	 * Either unload the font if it's being opened for
	 * the first time, or smash the generation field to
	 * mark this font as an orphan
	 */
	if (!(bfont->flags & FontReopen))
	{
	    if (bfont->freeFont)
		(*bfont->pfont->unload_font) (bfont->pfont);
#ifdef DEBUG
	    else
		fprintf (stderr, "Not freeing other font in cleanup_bfont\n");
#endif
	    bfont->pfont = 0;
	}
	else
	{
	    FSFontDataRec *fsd = (FSFontDataRec *)bfont->pfont->fpePrivate;
	    fsd->generation = -1;
	}
    }
}

/*
 * Check to see if a complete reply is waiting
 */
static fsGenericReply *
fs_get_reply (FSFpePtr conn, int *error)
{
    char	    *buf;
    fsGenericReply  *rep;
    int		    ret;

    /* block if the connection is down or paused in lfwi */
    if (conn->fs_fd == -1 || !conn->fs_listening)
    {
	*error = FSIO_BLOCK;
	return 0;
    }

    ret = _fs_start_read (conn, sizeof (fsGenericReply), &buf);
    if (ret != FSIO_READY)
    {
	*error = FSIO_BLOCK;
	return 0;
    }

    rep = (fsGenericReply *) buf;

    /*
     * Refuse to accept replies longer than a maximum reasonable length,
     * before we pass to _fs_start_read, since it will try to resize the
     * incoming connection buffer to this size.  Also avoids integer overflow
     * on 32-bit systems.
     */
    if (rep->length > MAX_REPLY_LENGTH)
    {
	ErrorF("fserve: reply length %ld > MAX_REPLY_LENGTH, disconnecting"
	       " from font server\n", (long)rep->length);
	_fs_connection_died (conn);
	*error = FSIO_ERROR;
	return 0;
    }

    ret = _fs_start_read (conn, rep->length << 2, &buf);
    if (ret != FSIO_READY)
    {
	*error = FSIO_BLOCK;
	return 0;
    }

    *error = FSIO_READY;

    return (fsGenericReply *) buf;
}

static Bool
fs_reply_ready (FSFpePtr conn)
{
    fsGenericReply  *rep;

    if (conn->fs_fd == -1 || !conn->fs_listening)
	return FALSE;
    if (fs_data_read (conn) < sizeof (fsGenericReply))
	return FALSE;
    rep = (fsGenericReply *) (conn->inBuf.buf + conn->inBuf.remove);
    if (fs_data_read (conn) < rep->length << 2)
	return FALSE;
    return TRUE;
}

static void
_fs_pending_reply (FSFpePtr conn)
{
    if (!(conn->blockState & FS_PENDING_REPLY))
    {
	_fs_mark_block (conn, FS_PENDING_REPLY);
	conn->blockedReplyTime = GetTimeInMillis () + FontServerRequestTimeout;
    }
}

static void
_fs_prepare_for_reply (FSFpePtr conn)
{
    _fs_pending_reply (conn);
    _fs_flush (conn);
}

/*
 * Block (for a while) awaiting a complete reply
 */
static int
fs_await_reply (FSFpePtr conn)
{
    int		    ret;

    if (conn->blockState & FS_COMPLETE_REPLY)
	return FSIO_READY;

    while (!fs_get_reply (conn, &ret))
    {
	if (ret != FSIO_BLOCK)
	    return ret;
	if (_fs_wait_for_readable (conn, FontServerRequestTimeout) != FSIO_READY)
	{
	    _fs_connection_died (conn);
	    return FSIO_ERROR;
	}
    }
    return FSIO_READY;
}

/*
 * Process the reply to an OpenBitmapFont request
 */
static int
fs_read_open_font(FontPathElementPtr fpe, FSBlockDataPtr blockrec)
{
    FSFpePtr		    conn = (FSFpePtr) fpe->private;
    FSBlockedFontPtr	    bfont = (FSBlockedFontPtr) blockrec->data;
    fsOpenBitmapFontReply   *rep;
    FSBlockDataPtr	    blockOrig;
    FSBlockedFontPtr	    origBfont;
    int			    ret;

    rep = (fsOpenBitmapFontReply *) fs_get_reply (conn, &ret);
    if (!rep || rep->type == FS_Error ||
	(rep->length != LENGTHOF(fsOpenBitmapFontReply)))
    {
	if (ret == FSIO_BLOCK)
	    return StillWorking;
	if (rep)
	    _fs_done_read (conn, rep->length << 2);
	fs_cleanup_bfont (conn, bfont);
	_fs_reply_failed (rep, fsOpenBitmapFontReply, "!=");
	return BadFontName;
    }

    /* If we're not reopening a font and FS detected a duplicate font
       open request, replace our reference to the new font with a
       reference to an existing font (possibly one not finished
       opening).  If this is a reopen, keep the new font reference...
       it's got the metrics and extents we read when the font was opened
       before.  This also gives us the freedom to easily close the font
       if we we decide (in fs_read_query_info()) that we don't like what
       we got. */

    if (rep->otherid && !(bfont->flags & FontReopen))
    {
	fs_cleanup_bfont (conn, bfont);

	/* Find old font if we're completely done getting it from server. */
	bfont->pfont = find_old_font(rep->otherid);
	bfont->freeFont = FALSE;
	bfont->fontid = rep->otherid;
	bfont->state = FS_DONE_REPLY;
	/*
	 * look for a blocked request to open the same font
	 */
	for (blockOrig = conn->blockedRequests;
		blockOrig;
		blockOrig = blockOrig->next)
	{
	    if (blockOrig != blockrec && blockOrig->type == FS_OPEN_FONT)
	    {
		origBfont = (FSBlockedFontPtr) blockOrig->data;
		if (origBfont->fontid == rep->otherid)
		{
		    blockrec->depending = blockOrig->depending;
		    blockOrig->depending = blockrec;
		    bfont->state = FS_DEPENDING;
		    bfont->pfont = origBfont->pfont;
		    break;
		}
	    }
	}
	if (bfont->pfont == NULL)
	{
	    /* XXX - something nasty happened */
	    ret = BadFontName;
	}
	else
	    ret = AccessDone;
    }
    else
    {
	bfont->pfont->info.cachable = rep->cachable != 0;
	bfont->state = FS_INFO_REPLY;
	/*
	 * Reset the blockrec for the next reply
	 */
	blockrec->sequenceNumber = bfont->queryInfoSequence;
	conn->blockedReplyTime = GetTimeInMillis () + FontServerRequestTimeout;
	ret = StillWorking;
    }
    _fs_done_read (conn, rep->length << 2);
    return ret;
}

static Bool
fs_fonts_match (FontInfoPtr pInfo1, FontInfoPtr pInfo2)
{
    int	    i;

    if (pInfo1->firstCol != pInfo2->firstCol ||
	pInfo1->lastCol != pInfo2->lastCol ||
	pInfo1->firstRow != pInfo2->firstRow ||
	pInfo1->lastRow != pInfo2->lastRow ||
	pInfo1->defaultCh != pInfo2->defaultCh ||
	pInfo1->noOverlap != pInfo2->noOverlap ||
	pInfo1->terminalFont != pInfo2->terminalFont ||
	pInfo1->constantMetrics != pInfo2->constantMetrics ||
	pInfo1->constantWidth != pInfo2->constantWidth ||
	pInfo1->inkInside != pInfo2->inkInside ||
	pInfo1->inkMetrics != pInfo2->inkMetrics ||
	pInfo1->allExist != pInfo2->allExist ||
	pInfo1->drawDirection != pInfo2->drawDirection ||
	pInfo1->cachable != pInfo2->cachable ||
	pInfo1->anamorphic != pInfo2->anamorphic ||
	pInfo1->maxOverlap != pInfo2->maxOverlap ||
	pInfo1->fontAscent != pInfo2->fontAscent ||
	pInfo1->fontDescent != pInfo2->fontDescent ||
	pInfo1->nprops != pInfo2->nprops)
	return FALSE;

#define MATCH(xci1, xci2) \
    (((xci1).leftSideBearing == (xci2).leftSideBearing) && \
     ((xci1).rightSideBearing == (xci2).rightSideBearing) && \
     ((xci1).characterWidth == (xci2).characterWidth) && \
     ((xci1).ascent == (xci2).ascent) && \
     ((xci1).descent == (xci2).descent) && \
     ((xci1).attributes == (xci2).attributes))

    if (!MATCH(pInfo1->maxbounds, pInfo2->maxbounds) ||
	!MATCH(pInfo1->minbounds, pInfo2->minbounds) ||
	!MATCH(pInfo1->ink_maxbounds, pInfo2->ink_maxbounds) ||
	!MATCH(pInfo1->ink_minbounds, pInfo2->ink_minbounds))
	return FALSE;

#undef MATCH

    for (i = 0; i < pInfo1->nprops; i++)
	if (pInfo1->isStringProp[i] !=
		pInfo2->isStringProp[i] ||
	    pInfo1->props[i].name !=
		pInfo2->props[i].name ||
	    pInfo1->props[i].value !=
		pInfo2->props[i].value)
	{
	    return FALSE;
	}
    return TRUE;
}

static int
fs_read_query_info(FontPathElementPtr fpe, FSBlockDataPtr blockrec)
{
    FSBlockedFontPtr	bfont = (FSBlockedFontPtr) blockrec->data;
    FSFpePtr		conn = (FSFpePtr) fpe->private;
    fsQueryXInfoReply	*rep;
    char		*buf;
    long		bufleft; /* length of reply left to use */
    fsPropInfo		*pi;
    fsPropOffset	*po;
    pointer		pd;
    FontInfoPtr		pInfo;
    FontInfoRec		tempInfo;
    int			err;
    int			ret;

    rep = (fsQueryXInfoReply *) fs_get_reply (conn, &ret);
    if (!rep || rep->type == FS_Error ||
	(rep->length < LENGTHOF(fsQueryXInfoReply)))
    {
	if (ret == FSIO_BLOCK)
	    return StillWorking;
	if (rep)
	    _fs_done_read (conn, rep->length << 2);
	fs_cleanup_bfont (conn, bfont);
	_fs_reply_failed (rep, fsQueryXInfoReply, "<");
	return BadFontName;
    }

    /* If this is a reopen, accumulate the query info into a dummy
       font and compare to our original data. */
    if (bfont->flags & FontReopen)
	pInfo = &tempInfo;
    else
	pInfo = &bfont->pfont->info;

    buf = (char *) rep;
    buf += SIZEOF(fsQueryXInfoReply);

    bufleft = rep->length << 2;
    bufleft -= SIZEOF(fsQueryXInfoReply);

    /* move the data over */
    fsUnpack_XFontInfoHeader(rep, pInfo);

    /* compute accelerators */
    _fs_init_fontinfo(conn, pInfo);

    /* Compute offsets into the reply */
    if (bufleft < SIZEOF(fsPropInfo))
    {
	ret = -1;
#ifdef DEBUG
	fprintf(stderr, "fsQueryXInfo: bufleft (%ld) < SIZEOF(fsPropInfo)\n",
		bufleft);
#endif
	goto bail;
    }
    pi = (fsPropInfo *) buf;
    buf += SIZEOF (fsPropInfo);
    bufleft -= SIZEOF(fsPropInfo);

    if ((bufleft / SIZEOF(fsPropOffset)) < pi->num_offsets)
    {
	ret = -1;
#ifdef DEBUG
	fprintf(stderr,
		"fsQueryXInfo: bufleft (%ld) / SIZEOF(fsPropOffset) < %u\n",
		bufleft, (unsigned) pi->num_offsets);
#endif
	goto bail;
    }
    po = (fsPropOffset *) buf;
    buf += pi->num_offsets * SIZEOF(fsPropOffset);
    bufleft -= pi->num_offsets * SIZEOF(fsPropOffset);

    if (bufleft < pi->data_len)
    {
	ret = -1;
#ifdef DEBUG
	fprintf(stderr,
		"fsQueryXInfo: bufleft (%ld) < data_len (%u)\n",
		bufleft, (unsigned) pi->data_len);
#endif
	goto bail;
    }
    pd = (pointer) buf;
    buf += pi->data_len;
    bufleft -= pi->data_len;

    /* convert the properties and step over the reply */
    ret = _fs_convert_props(pi, po, pd, pInfo);
  bail:
    _fs_done_read (conn, rep->length << 2);

    if (ret == -1)
    {
	fs_cleanup_bfont (conn, bfont);
	return AllocError;
    }

    if (bfont->flags & FontReopen)
    {
	/* We're reopening a font that we lost because of a downed
	   connection.  In the interest of avoiding corruption from
	   opening a different font than the old one (we already have
	   its metrics, extents, and probably some of its glyphs),
	   verify that the metrics and properties all match.  */

	if (fs_fonts_match (pInfo, &bfont->pfont->info))
	{
	    err = Successful;
	    bfont->state = FS_DONE_REPLY;
	}
	else
	{
	    fs_cleanup_bfont (conn, bfont);
	    err = BadFontName;
	}
	_fs_free_props (pInfo);

	return err;
    }

    /*
     * Ask for terminal format fonts if possible
     */
    if (bfont->pfont->info.terminalFont)
	bfont->format = ((bfont->format & ~ (BitmapFormatImageRectMask)) |
			 BitmapFormatImageRectMax);

    /*
     * Figure out if the whole font should get loaded right now.
     */
    if (glyphCachingMode == CACHING_OFF ||
	(glyphCachingMode == CACHE_16_BIT_GLYPHS
	 && !bfont->pfont->info.lastRow))
    {
	bfont->flags |= FontLoadAll;
    }

    /*
     * Ready to send the query bitmaps; the terminal font bit has
     * been computed and glyphCaching has been considered
     */
    if (bfont->flags & FontLoadBitmaps)
    {
	fs_send_query_bitmaps (fpe, blockrec);
	_fs_flush (conn);
    }

    bfont->state = FS_EXTENT_REPLY;

    /*
     * Reset the blockrec for the next reply
     */
    blockrec->sequenceNumber = bfont->queryExtentsSequence;
    conn->blockedReplyTime = GetTimeInMillis () + FontServerRequestTimeout;

    return StillWorking;
}

static int
fs_read_extent_info(FontPathElementPtr fpe, FSBlockDataPtr blockrec)
{
    FSFpePtr		    conn = (FSFpePtr) fpe->private;
    FSBlockedFontPtr	    bfont = (FSBlockedFontPtr) blockrec->data;
    FSFontDataPtr	    fsd = (FSFontDataPtr) bfont->pfont->fpePrivate;
    FSFontPtr		    fsfont = (FSFontPtr) bfont->pfont->fontPrivate;
    fsQueryXExtents16Reply  *rep;
    char		    *buf;
    int			    i;
    int			    numExtents;
    int			    numInfos;
    int			    ret;
    Bool		    haveInk = FALSE; /* need separate ink metrics? */
    CharInfoPtr		    ci, pCI;
    char		    *fsci;
    fsXCharInfo		    fscilocal;
    FontInfoRec		    *fi = &bfont->pfont->info;

    rep = (fsQueryXExtents16Reply *) fs_get_reply (conn, &ret);
    if (!rep || rep->type == FS_Error ||
	(rep->length < LENGTHOF(fsQueryXExtents16Reply)))
    {
	if (ret == FSIO_BLOCK)
	    return StillWorking;
	if (rep)
	    _fs_done_read (conn, rep->length << 2);
	fs_cleanup_bfont (conn, bfont);
	_fs_reply_failed (rep, fsQueryXExtents16Reply, "<");
	return BadFontName;
    }

    /* move the data over */
    /* need separate inkMetrics for fixed font server protocol version */
    numExtents = rep->num_extents;
    numInfos = numExtents;
    if (bfont->pfont->info.terminalFont && conn->fsMajorVersion > 1)
    {
	numInfos *= 2;
	haveInk = TRUE;
    }
    if (numInfos >= (INT_MAX / sizeof(CharInfoRec))) {
#ifdef DEBUG
	fprintf(stderr,
		"fsQueryXExtents16: numInfos (%d) >= %ld\n",
		numInfos, (INT_MAX / sizeof(CharInfoRec)));
#endif
	pCI = NULL;
    }
    else if (numExtents > ((rep->length - LENGTHOF(fsQueryXExtents16Reply))
			    / LENGTHOF(fsXCharInfo))) {
#ifdef DEBUG
	fprintf(stderr,
		"fsQueryXExtents16: numExtents (%d) > (%u - %d) / %d\n",
		numExtents, (unsigned) rep->length,
		LENGTHOF(fsQueryXExtents16Reply), LENGTHOF(fsXCharInfo));
#endif
	pCI = NULL;
    }
    else
	pCI = mallocarray(numInfos, sizeof(CharInfoRec));

    if (!pCI)
    {
	_fs_done_read (conn, rep->length << 2);
	fs_cleanup_bfont(conn, bfont);
	return AllocError;
    }
    fsfont->encoding = pCI;
    if (haveInk)
	fsfont->inkMetrics = pCI + numExtents;
    else
        fsfont->inkMetrics = pCI;

    buf = (char *) rep;
    buf += SIZEOF (fsQueryXExtents16Reply);
    fsci = buf;

    fsd->glyphs_to_get = 0;
    ci = fsfont->inkMetrics;
    for (i = 0; i < numExtents; i++)
    {
	memcpy(&fscilocal, fsci, SIZEOF(fsXCharInfo)); /* align it */
	_fs_convert_char_info(&fscilocal, &ci->metrics);
	/* Bounds check. */
	if (ci->metrics.ascent > fi->maxbounds.ascent)
	{
	    ErrorF("fserve: warning: %s %s ascent (%d) > maxascent (%d)\n",
		   fpe->name, fsd->name,
		   ci->metrics.ascent, fi->maxbounds.ascent);
	    ci->metrics.ascent = fi->maxbounds.ascent;
	}
	if (ci->metrics.descent > fi->maxbounds.descent)
	{
	    ErrorF("fserve: warning: %s %s descent (%d) > maxdescent (%d)\n",
		   fpe->name, fsd->name,
		   ci->metrics.descent, fi->maxbounds.descent);
	    ci->metrics.descent = fi->maxbounds.descent;
	}
	fsci = fsci + SIZEOF(fsXCharInfo);
	/* Initialize the bits field for later glyph-caching use */
	if (NONZEROMETRICS(&ci->metrics))
	{
	    if (!haveInk &&
		(ci->metrics.leftSideBearing == ci->metrics.rightSideBearing ||
		 ci->metrics.ascent == -ci->metrics.descent))
		pCI[i].bits = &_fs_glyph_zero_length;
	    else
	    {
		pCI[i].bits = &_fs_glyph_undefined;
		fsd->glyphs_to_get++;
	    }
	}
	else
	    pCI[i].bits = (char *)0;
	ci++;
    }

    /* Done with reply */
    _fs_done_read (conn, rep->length << 2);

    /* build bitmap metrics, ImageRectMax style */
    if (haveInk)
    {
	CharInfoPtr ii;

	ci = fsfont->encoding;
	ii = fsfont->inkMetrics;
	for (i = 0; i < numExtents; i++, ci++, ii++)
	{
	    if (NONZEROMETRICS(&ii->metrics))
	    {
		ci->metrics.leftSideBearing = FONT_MIN_LEFT(fi);
		ci->metrics.rightSideBearing = FONT_MAX_RIGHT(fi);
		ci->metrics.ascent = FONT_MAX_ASCENT(fi);
		ci->metrics.descent = FONT_MAX_DESCENT(fi);
		ci->metrics.characterWidth = FONT_MAX_WIDTH(fi);
		ci->metrics.attributes = ii->metrics.attributes;
	    }
	    else
	    {
		ci->metrics = ii->metrics;
	    }
	    /* Bounds check. */
	    if (ci->metrics.ascent > fi->maxbounds.ascent)
	    {
		ErrorF("fserve: warning: %s %s ascent (%d) "
		       "> maxascent (%d)\n",
		       fpe->name, fsd->name,
		       ci->metrics.ascent, fi->maxbounds.ascent);
		ci->metrics.ascent = fi->maxbounds.ascent;
	    }
	    if (ci->metrics.descent > fi->maxbounds.descent)
	    {
		ErrorF("fserve: warning: %s %s descent (%d) "
		       "> maxdescent (%d)\n",
		       fpe->name, fsd->name,
		       ci->metrics.descent, fi->maxbounds.descent);
		ci->metrics.descent = fi->maxbounds.descent;
	    }
	}
    }
    {
	unsigned int r, c, numCols, firstCol;

	firstCol = bfont->pfont->info.firstCol;
	numCols = bfont->pfont->info.lastCol - firstCol + 1;
	c = bfont->pfont->info.defaultCh;
	fsfont->pDefault = 0;
	if (bfont->pfont->info.lastRow)
	{
	    r = c >> 8;
	    r -= bfont->pfont->info.firstRow;
	    c &= 0xff;
	    c -= firstCol;
	    if (r < bfont->pfont->info.lastRow-bfont->pfont->info.firstRow+1 &&
		c < numCols)
		fsfont->pDefault = &pCI[r * numCols + c];
	}
	else
	{
	    c -= firstCol;
	    if (c < numCols)
		fsfont->pDefault = &pCI[c];
	}
    }
    bfont->state = FS_GLYPHS_REPLY;

    if (bfont->flags & FontLoadBitmaps)
    {
	/*
	 * Reset the blockrec for the next reply
	 */
	blockrec->sequenceNumber = bfont->queryBitmapsSequence;
	conn->blockedReplyTime = GetTimeInMillis () + FontServerRequestTimeout;
	return StillWorking;
    }
    return Successful;
}

#ifdef DEBUG
static const char *fs_open_states[] = {
    "OPEN_REPLY  ",
    "INFO_REPLY  ",
    "EXTENT_REPLY",
    "GLYPHS_REPLY",
    "DONE_REPLY  ",
    "DEPENDING   ",
};
#endif

static int
fs_do_open_font(FontPathElementPtr fpe, FSBlockDataPtr blockrec)
{
    FSBlockedFontPtr	bfont = (FSBlockedFontPtr) blockrec->data;
    int			err;

#ifdef DEBUG
    fprintf (stderr, "fs_do_open_font state %s %s\n",
	     fs_open_states[bfont->state],
	     ((FSFontDataPtr) (bfont->pfont->fpePrivate))->name);
#endif
    err = BadFontName;
    switch (bfont->state) {
    case FS_OPEN_REPLY:
	err = fs_read_open_font(fpe, blockrec);
	if (err != StillWorking) {	/* already loaded, or error */
	    /* if font's already loaded, massage error code */
	    switch (bfont->state) {
	    case FS_DONE_REPLY:
		err = Successful;
		break;
	    case FS_DEPENDING:
		err = StillWorking;
		break;
	    }
	}
	break;
    case FS_INFO_REPLY:
	err = fs_read_query_info(fpe, blockrec);
	break;
    case FS_EXTENT_REPLY:
	err = fs_read_extent_info(fpe, blockrec);
	break;
    case FS_GLYPHS_REPLY:
	if (bfont->flags & FontLoadBitmaps)
	    err = fs_read_glyphs(fpe, blockrec);
	break;
    case FS_DEPENDING:		/* can't happen */
    default:
	break;
    }
#ifdef DEBUG
    fprintf (stderr, "fs_do_open_font err %d\n", err);
#endif
    if (err != StillWorking)
    {
	bfont->state = FS_DONE_REPLY;	/* for _fs_load_glyphs() */
	while ((blockrec = blockrec->depending))
	{
	    bfont = (FSBlockedFontPtr) blockrec->data;
	    bfont->state = FS_DONE_REPLY;	/* for _fs_load_glyphs() */
	}
    }
    return err;
}

void
_fs_mark_block (FSFpePtr conn, CARD32 mask)
{
    conn->blockState |= mask;
    fs_blockState |= mask;
}

void
_fs_unmark_block (FSFpePtr conn, CARD32 mask)
{
    FSFpePtr	c;

    if (conn->blockState & mask)
    {
	conn->blockState &= ~mask;
	fs_blockState = 0;
	for (c = fs_fpes; c; c = c->next)
	    fs_blockState |= c->blockState;
    }
}

/* ARGSUSED */
static void
fs_block_handler(void *wt)
{
    CARD32	now, earliest, wakeup;
    int		soonest;
    FSFpePtr    conn;

    /*
     * Flush all pending output
     */
    if (fs_blockState & FS_PENDING_WRITE)
	for (conn = fs_fpes; conn; conn = conn->next)
	    if (conn->blockState & FS_PENDING_WRITE)
		_fs_flush (conn);
    /*
     * Check for any fpe with a complete reply, set sleep time to zero
     */
    if (fs_blockState & FS_COMPLETE_REPLY)
	adjust_fs_wait_for_delay(wt, 0);
    /*
     * Walk through fpe list computing sleep time
     */
    else if (fs_blockState & (FS_BROKEN_WRITE|
			      FS_BROKEN_CONNECTION|
			      FS_PENDING_REPLY|
			      FS_RECONNECTING))
    {
	now = GetTimeInMillis ();
	earliest = now + 10000000;
	for (conn = fs_fpes; conn; conn = conn->next)
	{
	    if (conn->blockState & FS_RECONNECTING)
	    {
		wakeup = conn->blockedConnectTime;
		if (TimeCmp (wakeup, <, earliest))
		    earliest = wakeup;
	    }
	    if (conn->blockState & FS_BROKEN_CONNECTION)
	    {
		wakeup = conn->brokenConnectionTime;
		if (TimeCmp (wakeup, <, earliest))
		    earliest = wakeup;
	    }
	    if (conn->blockState & FS_BROKEN_WRITE)
	    {
		wakeup = conn->brokenWriteTime;
		if (TimeCmp (wakeup, <, earliest))
		    earliest = wakeup;
	    }
	    if (conn->blockState & FS_PENDING_REPLY)
	    {
		wakeup = conn->blockedReplyTime;
		if (TimeCmp (wakeup, <, earliest))
		    earliest = wakeup;
	    }
	}
	soonest = earliest - now;
	if (soonest < 0)
	    soonest = 0;
	adjust_fs_wait_for_delay(wt, soonest);
    }
}

static void
fs_handle_unexpected(FSFpePtr conn, fsGenericReply *rep)
{
    if (rep->type == FS_Event && rep->data1 == KeepAlive)
    {
	fsNoopReq   req;

	/* ping it back */
	req.reqType = FS_Noop;
	req.length = SIZEOF(fsNoopReq) >> 2;
	_fs_add_req_log(conn, FS_Noop);
	_fs_write(conn, (char *) &req, SIZEOF(fsNoopReq));
    }
    /* this should suck up unexpected replies and events */
    _fs_done_read (conn, rep->length << 2);
}

static void
fs_read_reply (FontPathElementPtr fpe, pointer client)
{
    FSFpePtr	    conn = (FSFpePtr) fpe->private;
    FSBlockDataPtr  blockrec;
    int		    ret;
    int		    err;
    fsGenericReply  *rep;

    if ((rep = fs_get_reply (conn, &ret)))
    {
	_fs_add_rep_log (conn, rep);
	for (blockrec = conn->blockedRequests;
	     blockrec;
	     blockrec = blockrec->next)
	{
	    if (blockrec->sequenceNumber == rep->sequenceNumber)
		break;
	}
	err = Successful;
	if (!blockrec)
	{
	    fs_handle_unexpected(conn, rep);
	}
	else
	{
	    /*
	     * go read it, and if we're done,
	     * wake up the appropriate client
	     */
	    switch (blockrec->type) {
	    case FS_OPEN_FONT:
		blockrec->errcode = fs_do_open_font(fpe, blockrec);
		break;
	    case FS_LOAD_GLYPHS:
		blockrec->errcode = fs_read_glyphs(fpe, blockrec);
		break;
	    case FS_LIST_FONTS:
		blockrec->errcode = fs_read_list(fpe, blockrec);
		break;
	    case FS_LIST_WITH_INFO:
		blockrec->errcode = fs_read_list_info(fpe, blockrec);
		break;
	    default:
		break;
	    }
	    err = blockrec->errcode;
	    if (err != StillWorking)
	    {
		while (blockrec)
		{
		    blockrec->errcode = err;
		    if (client != blockrec->client)
			ClientSignal(blockrec->client);
		    blockrec = blockrec->depending;
		}
		_fs_unmark_block (conn, FS_PENDING_REPLY);
	    }
	}
	if (fs_reply_ready (conn))
	    _fs_mark_block (conn, FS_COMPLETE_REPLY);
	else
	    _fs_unmark_block (conn, FS_COMPLETE_REPLY);
    }
}

static void
fs_fd_handler(int fd, void *data)
{
    FontPathElementPtr	fpe = data;
    FSFpePtr	    	conn = (FSFpePtr) fpe->private;

    /*
     * Don't continue if the fd is -1 (which will be true when the
     * font server terminates
     */
    if ((conn->blockState & FS_RECONNECTING))
	_fs_check_reconnect (conn);
    else if ((conn->fs_fd != -1))
	fs_read_reply (fpe, 0);
}

static int
fs_wakeup(FontPathElementPtr fpe)
{
    FSFpePtr	    conn = (FSFpePtr) fpe->private;

    if (conn->blockState & (FS_PENDING_REPLY|FS_BROKEN_CONNECTION|FS_BROKEN_WRITE))
	_fs_do_blocked (conn);
    if (conn->blockState & FS_COMPLETE_REPLY)
	fs_read_reply (fpe, 0);
#ifdef DEBUG
    {
	FSBlockDataPtr	    blockrec;
	FSBlockedFontPtr    bfont;
	static CARD32	    lastState;
	static FSBlockDataPtr	lastBlock;

	if (conn->blockState || conn->blockedRequests || lastState || lastBlock)
	{
	    fprintf (stderr, "  Block State 0x%x\n", (int) conn->blockState);
	    lastState = conn->blockState;
	    lastBlock = conn->blockedRequests;
	}
	for (blockrec = conn->blockedRequests; blockrec; blockrec = blockrec->next)
	{
	    switch (blockrec->type) {
	    case FS_OPEN_FONT:
		bfont = (FSBlockedFontPtr) blockrec->data;
		fprintf (stderr, "  Blocked font errcode %d sequence %d state %s %s\n",
			 blockrec->errcode,
			 blockrec->sequenceNumber,
			 fs_open_states[bfont->state],
			 bfont->pfont ?
			 ((FSFontDataPtr) (bfont->pfont->fpePrivate))->name :
			 "<freed>");
		break;
	    case FS_LIST_FONTS:
		fprintf (stderr, "  Blocked list errcode %d sequence %d\n",
			 blockrec->errcode, blockrec->sequenceNumber);
		break;
	    default:
		fprintf (stderr, "  Blocked type %d errcode %d sequence %d\n",
			 blockrec->type,
			 blockrec->errcode,
			 blockrec->sequenceNumber);
		break;
	    }
	}
    }
#endif
    return FALSE;
}

/*
 * Notice a dead connection and prepare for reconnect
 */

void
_fs_connection_died(FSFpePtr conn)
{
    if (conn->blockState & FS_BROKEN_CONNECTION)
	return;
    fs_close_conn(conn);
    conn->brokenConnectionTime = GetTimeInMillis ();
    _fs_mark_block (conn, FS_BROKEN_CONNECTION);
    _fs_unmark_block (conn, FS_BROKEN_WRITE|FS_PENDING_WRITE|FS_RECONNECTING);
}

/*
 * Signal clients that the connection has come back up
 */
static int
_fs_restart_connection(FSFpePtr conn)
{
    FSBlockDataPtr block;

    _fs_unmark_block (conn, FS_GIVE_UP);
    while ((block = (FSBlockDataPtr) conn->blockedRequests))
    {
	if (block->errcode == StillWorking)
	{
	    ClientSignal(block->client);
	    fs_abort_blockrec(conn, block);
	}
    }
    return TRUE;
}

/*
 * Declare this font server connection useless
 */
static void
_fs_giveup (FSFpePtr conn)
{
    FSBlockDataPtr  block;

    if (conn->blockState & FS_GIVE_UP)
	return;
#ifdef DEBUG
    fprintf (stderr, "give up on FS \"%s\"\n", conn->servername);
#endif
    _fs_mark_block (conn, FS_GIVE_UP);
    while ((block = (FSBlockDataPtr) conn->blockedRequests))
    {
	if (block->errcode == StillWorking)
	{
	    ClientSignal (block->client);
	    fs_abort_blockrec (conn, block);
	}
    }
    if (conn->fs_fd >= 0)
	_fs_connection_died (conn);
}

static void
_fs_do_blocked (FSFpePtr conn)
{
    CARD32      now;

    now = GetTimeInMillis ();
    if ((conn->blockState & FS_PENDING_REPLY) &&
	TimeCmp (conn->blockedReplyTime, <=, now))
    {
	_fs_giveup (conn);
    }
    else
    {
	if (conn->blockState & FS_BROKEN_CONNECTION)
	{
	    /* Try to reconnect broken connections */
	    if (TimeCmp (conn->brokenConnectionTime, <=, now))
		_fs_start_reconnect (conn);
	}
	else if (conn->blockState & FS_BROKEN_WRITE)
	{
	    /* Try to flush blocked connections */
	    if (TimeCmp (conn->brokenWriteTime, <=, now))
		_fs_flush (conn);
	}
    }
}

/*
 * sends the actual request out
 */
/* ARGSUSED */
static int
fs_send_open_font(pointer client, FontPathElementPtr fpe, Mask flags,
		  const char *name, int namelen,
		  fsBitmapFormat format, fsBitmapFormatMask fmask,
		  XID id, FontPtr *ppfont)
{
    FSFpePtr		    conn = (FSFpePtr) fpe->private;
    FontPtr		    font;
    FSBlockDataPtr	    blockrec = NULL;
    FSBlockedFontPtr	    bfont;
    FSFontDataPtr	    fsd;
    fsOpenBitmapFontReq	    openreq;
    fsQueryXInfoReq	    inforeq;
    fsQueryXExtents16Req    extreq;
    int			    err;
    unsigned char	    buf[1024];

    if (conn->blockState & FS_GIVE_UP)
	return BadFontName;

    if (namelen < 0 || namelen > sizeof (buf) - 1)
	return BadFontName;

    /*
     * Get the font structure put together, either by reusing
     * the existing one or creating a new one
     */
    if (flags & FontReopen)
    {
	Atom	nameatom, fn = None;
	int	i;

	font = *ppfont;
	fsd = (FSFontDataPtr)font->fpePrivate;
	/* This is an attempt to reopen a font.  Did the font have a
	   NAME property? */
	if ((nameatom = MakeAtom("FONT", 4, 0)) != None)
	{
	    for (i = 0; i < font->info.nprops; i++)
		if (font->info.props[i].name == nameatom &&
		    font->info.isStringProp[i])
		{
		    fn = font->info.props[i].value;
		    break;
		}
	}
	if (fn == None || !(name = NameForAtom(fn)))
	{
	    name = fsd->name;
	    namelen = fsd->namelen;
	}
	else
	    namelen = strlen(name);
    }
    else
    {
	font = fs_create_font (fpe, name, namelen, format, fmask);
	if (!font)
	    return AllocError;

	fsd = (FSFontDataPtr)font->fpePrivate;
    }

    /* make a new block record, and add it to the end of the list */
    blockrec = fs_new_block_rec(font->fpe, client, FS_OPEN_FONT);
    if (!blockrec)
    {
	if (!(flags & FontReopen))
	    (*font->unload_font) (font);
	return AllocError;
    }

    fsd->generation = conn->generation;

    bfont = (FSBlockedFontPtr) blockrec->data;
    bfont->fontid = fsd->fontid;
    bfont->pfont = font;
    bfont->state = FS_OPEN_REPLY;
    bfont->flags = flags;
    bfont->format = fsd->format;
    bfont->clients_depending = (FSClientsDependingPtr)0;
    bfont->freeFont = (flags & FontReopen) == 0;

    /*
     * Must check this before generating any protocol, otherwise we'll
     * mess up a reconnect in progress
     */
    if (conn->blockState & (FS_BROKEN_CONNECTION | FS_RECONNECTING))
    {
	_fs_pending_reply (conn);
	return Suspended;
    }

    _fs_client_access (conn, client, (flags & FontOpenSync) != 0);
    _fs_client_resolution(conn);

    /* do an FS_OpenFont, FS_QueryXInfo and FS_QueryXExtents */
    buf[0] = (unsigned char) namelen;
    memcpy(&buf[1], name, namelen);
    openreq.reqType = FS_OpenBitmapFont;
    openreq.pad = 0;
    openreq.fid = fsd->fontid;
    openreq.format_hint = fsd->format;
    openreq.format_mask = fsd->fmask;
    openreq.length = (SIZEOF(fsOpenBitmapFontReq) + namelen + 4) >> 2;

    _fs_add_req_log(conn, FS_OpenBitmapFont);
    _fs_write(conn, (char *) &openreq, SIZEOF(fsOpenBitmapFontReq));
    _fs_write_pad(conn, (char *) buf, namelen + 1);

    blockrec->sequenceNumber = conn->current_seq;

    inforeq.reqType = FS_QueryXInfo;
    inforeq.pad = 0;
    inforeq.id = fsd->fontid;
    inforeq.length = SIZEOF(fsQueryXInfoReq) >> 2;

    bfont->queryInfoSequence = conn->current_seq + 1;

    _fs_add_req_log(conn, FS_QueryXInfo);
    _fs_write(conn, (char *) &inforeq, SIZEOF(fsQueryXInfoReq));

    if (!(bfont->flags & FontReopen))
    {
	extreq.reqType = FS_QueryXExtents16;
	extreq.range = fsTrue;
	extreq.fid = fsd->fontid;
	extreq.num_ranges = 0;
	extreq.length = SIZEOF(fsQueryXExtents16Req) >> 2;

	bfont->queryExtentsSequence = conn->current_seq + 1;

	_fs_add_req_log(conn, FS_QueryXExtents16);
	_fs_write(conn, (char *) &extreq, SIZEOF(fsQueryXExtents16Req));
    }

#ifdef NCD
    if (configData.ExtendedFontDiags)
    {
	memcpy(buf, name, MIN(256, namelen));
	buf[MIN(256, namelen)] = '\0';
	printf("Requesting font \"%s\" from font server \"%s\"\n",
	       buf, font->fpe->name);
    }
#endif
    _fs_prepare_for_reply (conn);

    err = blockrec->errcode;
    if (bfont->flags & FontOpenSync)
    {
	while (blockrec->errcode == StillWorking)
	{
	    if (fs_await_reply (conn) != FSIO_READY)
	    {
		blockrec->errcode = BadFontName;
		break;
	    }
	    fs_read_reply (font->fpe, client);
	}
	err = blockrec->errcode;
	if (err == Successful)
	    *ppfont = bfont->pfont;
	else
	    fs_cleanup_bfont (conn, bfont);
	bfont->freeFont = FALSE;
	_fs_remove_block_rec (conn, blockrec);
    }
    return err == StillWorking ? Suspended : err;
}

static void
fs_send_query_bitmaps(FontPathElementPtr fpe, FSBlockDataPtr blockrec)
{
    FSFpePtr		    conn = (FSFpePtr) fpe->private;
    FSBlockedFontPtr	    bfont = (FSBlockedFontPtr) blockrec->data;
    fsQueryXBitmaps16Req    bitreq;

    /* send the request */
    bitreq.reqType = FS_QueryXBitmaps16;
    bitreq.fid = bfont->fontid;
    bitreq.format = bfont->format;
    bitreq.range = TRUE;
    bitreq.length = SIZEOF(fsQueryXBitmaps16Req) >> 2;
    bitreq.num_ranges = 0;

    bfont->queryBitmapsSequence = conn->current_seq + 1;

    _fs_add_req_log(conn, FS_QueryXBitmaps16);
    _fs_write(conn, (char *) &bitreq, SIZEOF(fsQueryXBitmaps16Req));
}

/* ARGSUSED */
static int
fs_open_font(pointer client, FontPathElementPtr fpe, Mask flags,
	     const char *name, int namelen,
	     fsBitmapFormat format, fsBitmapFormatMask fmask,
	     XID id, FontPtr *ppfont,
	     char **alias, FontPtr non_cachable_font)
{
    FSFpePtr		conn = (FSFpePtr) fpe->private;
    FSBlockDataPtr	blockrec;
    FSBlockedFontPtr	bfont;
    int			err;

    /* libfont interface expects ImageRectMin glyphs */
    format = (format & ~BitmapFormatImageRectMask) | BitmapFormatImageRectMin;

    *alias = (char *) 0;
    for (blockrec = conn->blockedRequests; blockrec; blockrec = blockrec->next)
    {
	if (blockrec->type == FS_OPEN_FONT && blockrec->client == client)
	{
	    err = blockrec->errcode;
	    if (err == StillWorking)
		return Suspended;

	    bfont = (FSBlockedFontPtr) blockrec->data;
	    if (err == Successful)
		*ppfont = bfont->pfont;
	    else
		fs_cleanup_bfont (conn, bfont);
	    _fs_remove_block_rec (conn, blockrec);
	    return err;
	}
    }
    return fs_send_open_font(client, fpe, flags, name, namelen, format, fmask,
			     id, ppfont);
}

/* ARGSUSED */
static int
fs_send_close_font(FSFpePtr conn, Font id)
{
    fsCloseReq  req;

    if (conn->blockState & FS_GIVE_UP)
	return Successful;
    /* tell the font server to close the font */
    req.reqType = FS_CloseFont;
    req.pad = 0;
    req.length = SIZEOF(fsCloseReq) >> 2;
    req.id = id;
    _fs_add_req_log(conn, FS_CloseFont);
    _fs_write(conn, (char *) &req, SIZEOF(fsCloseReq));

    return Successful;
}

/* ARGSUSED */
static void
fs_close_font(FontPathElementPtr fpe, FontPtr pfont)
{
    FSFontDataPtr   fsd = (FSFontDataPtr) pfont->fpePrivate;
    FSFpePtr	    conn = (FSFpePtr) fpe->private;

    if (conn->generation == fsd->generation)
	fs_send_close_font(conn, fsd->fontid);

#ifdef DEBUG
    {
	FSBlockDataPtr	    blockrec;
	FSBlockedFontPtr    bfont;

	for (blockrec = conn->blockedRequests; blockrec; blockrec = blockrec->next)
	{
	    if (blockrec->type == FS_OPEN_FONT)
	    {
		bfont = (FSBlockedFontPtr) blockrec->data;
		if (bfont->pfont == pfont)
		    fprintf (stderr, "closing font which hasn't been opened\n");
	    }
	}
    }
#endif
    (*pfont->unload_font) (pfont);
}

static int
fs_read_glyphs(FontPathElementPtr fpe, FSBlockDataPtr blockrec)
{
    FSBlockedGlyphPtr	    bglyph = (FSBlockedGlyphPtr) blockrec->data;
    FSBlockedFontPtr	    bfont = (FSBlockedFontPtr) blockrec->data;
    FSFpePtr		    conn = (FSFpePtr) fpe->private;
    FontPtr		    pfont = bglyph->pfont;
					/* works for either blocked font
					   or glyph rec...  pfont is at
					   the very beginning of both
					   blockrec->data structures */
    FSFontDataPtr	    fsd = (FSFontDataPtr) (pfont->fpePrivate);
    FSFontPtr		    fsdata = (FSFontPtr) pfont->fontPrivate;
    FontInfoPtr		    pfi = &pfont->info;
    fsQueryXBitmaps16Reply  *rep;
    char		    *buf;
    long		    bufleft; /* length of reply left to use */
    fsOffset32		    *ppbits;
    fsOffset32		    local_off;
    char		    *off_adr;
    pointer		    pbitmaps;
    char		    *bits, *allbits;
#ifdef DEBUG
    char		    *origallbits;
#endif
    int			    i,
			    err;
    int			    nranges = 0;
    int			    ret;
    fsRange		    *nextrange = 0;
    unsigned long	    minchar, maxchar;

    rep = (fsQueryXBitmaps16Reply *) fs_get_reply (conn, &ret);
    if (!rep || rep->type == FS_Error ||
	(rep->length < LENGTHOF(fsQueryXBitmaps16Reply)))
    {
	if (ret == FSIO_BLOCK)
	    return StillWorking;
	if (rep)
	    _fs_done_read (conn, rep->length << 2);
	err = AllocError;
	_fs_reply_failed (rep, fsQueryXBitmaps16Reply, "<");
	goto bail;
    }

    buf = (char *) rep;
    buf += SIZEOF (fsQueryXBitmaps16Reply);

    bufleft = rep->length << 2;
    bufleft -= SIZEOF (fsQueryXBitmaps16Reply);

    if ((bufleft / SIZEOF (fsOffset32)) < rep->num_chars)
    {
#ifdef DEBUG
	fprintf(stderr,
		"fsQueryXBitmaps16: num_chars (%u) > bufleft (%ld) / %d\n",
		(unsigned) rep->num_chars, bufleft, SIZEOF (fsOffset32));
#endif
	err = AllocError;
	goto bail;
    }
    ppbits = (fsOffset32 *) buf;
    buf += SIZEOF (fsOffset32) * (rep->num_chars);
    bufleft -= SIZEOF (fsOffset32) * (rep->num_chars);

    if (bufleft < rep->nbytes)
    {
#ifdef DEBUG
	fprintf(stderr,
		"fsQueryXBitmaps16: nbytes (%u) > bufleft (%ld)\n",
		(unsigned) rep->nbytes, bufleft);
#endif
	err = AllocError;
	goto bail;
    }
    pbitmaps = (pointer ) buf;

    if (blockrec->type == FS_LOAD_GLYPHS)
    {
	nranges = bglyph->num_expected_ranges;
	nextrange = bglyph->expected_ranges;
    }

    /* place the incoming glyphs */
    if (nranges)
    {
	/* We're operating under the assumption that the ranges
	   requested in the LoadGlyphs call were all legal for this
	   font, and that individual ranges do not cover multiple
	   rows...  fs_build_range() is designed to ensure this. */
	minchar = (nextrange->min_char_high - pfi->firstRow) *
		  (pfi->lastCol - pfi->firstCol + 1) +
		  nextrange->min_char_low - pfi->firstCol;
	maxchar = (nextrange->max_char_high - pfi->firstRow) *
		  (pfi->lastCol - pfi->firstCol + 1) +
		  nextrange->max_char_low - pfi->firstCol;
	nextrange++;
    }
    else
    {
	minchar = 0;
	maxchar = rep->num_chars;
    }

    off_adr = (char *)ppbits;

    allbits = fs_alloc_glyphs (pfont, rep->nbytes);

    if (!allbits)
    {
	err = AllocError;
	goto bail;
    }

#ifdef DEBUG
    origallbits = allbits;
    fprintf (stderr, "Reading %d glyphs in %d bytes for %s\n",
	     (int) rep->num_chars, (int) rep->nbytes, fsd->name);
#endif

    for (i = 0; i < rep->num_chars; i++)
    {
	memcpy(&local_off, off_adr, SIZEOF(fsOffset32));	/* align it */
	if (blockrec->type == FS_OPEN_FONT ||
	    fsdata->encoding[minchar].bits == &_fs_glyph_requested)
	{
	    /*
	     * Broken X font server returns bits for missing characters
	     * when font is padded
	     */
	    if (NONZEROMETRICS(&fsdata->encoding[minchar].metrics))
	    {
		if (local_off.length &&
		    (local_off.position < rep->nbytes) &&
		    (local_off.length <= (rep->nbytes - local_off.position)))
		{
		    bits = allbits;
		    allbits += local_off.length;
		    memcpy(bits, (char *)pbitmaps + local_off.position,
			   local_off.length);
		}
		else
		    bits = &_fs_glyph_zero_length;
	    }
	    else
		bits = 0;
	    if (fsdata->encoding[minchar].bits == &_fs_glyph_requested)
		fsd->glyphs_to_get--;
	    fsdata->encoding[minchar].bits = bits;
	}
	if (minchar++ == maxchar)
	{
	    if (!--nranges) break;
	    minchar = (nextrange->min_char_high - pfi->firstRow) *
		      (pfi->lastCol - pfi->firstCol + 1) +
		      nextrange->min_char_low - pfi->firstCol;
	    maxchar = (nextrange->max_char_high - pfi->firstRow) *
		      (pfi->lastCol - pfi->firstCol + 1) +
		      nextrange->max_char_low - pfi->firstCol;
	    nextrange++;
	}
	off_adr += SIZEOF(fsOffset32);
    }
#ifdef DEBUG
    fprintf (stderr, "Used %d bytes instead of %d\n",
	     (int) (allbits - origallbits), (int) rep->nbytes);
#endif

    if (blockrec->type == FS_OPEN_FONT)
    {
	fsd->glyphs_to_get = 0;
	bfont->state = FS_DONE_REPLY;
    }
    err = Successful;

bail:
    if (rep)
	_fs_done_read (conn, rep->length << 2);
    return err;
}

static int
fs_send_load_glyphs(pointer client, FontPtr pfont,
		    int nranges, fsRange *ranges)
{
    FontPathElementPtr	    fpe = pfont->fpe;
    FSFpePtr		    conn = (FSFpePtr) fpe->private;
    FSBlockedGlyphPtr	    blockedglyph;
    fsQueryXBitmaps16Req    req;
    FSBlockDataPtr	    blockrec;

    if (conn->blockState & FS_GIVE_UP)
	return BadCharRange;

    /* make a new block record, and add it to the end of the list */
    blockrec = fs_new_block_rec(fpe, client, FS_LOAD_GLYPHS);
    if (!blockrec)
	return AllocError;
    blockedglyph = (FSBlockedGlyphPtr) blockrec->data;
    blockedglyph->pfont = pfont;
    blockedglyph->num_expected_ranges = nranges;
    /* Assumption: it's our job to free ranges */
    blockedglyph->expected_ranges = ranges;
    blockedglyph->clients_depending = (FSClientsDependingPtr)0;

    if (conn->blockState & (FS_BROKEN_CONNECTION|FS_RECONNECTING))
    {
	_fs_pending_reply (conn);
	return Suspended;
    }

    /* send the request */
    req.reqType = FS_QueryXBitmaps16;
    req.fid = ((FSFontDataPtr) pfont->fpePrivate)->fontid;
    req.format = pfont->format;
    if (pfont->info.terminalFont)
	req.format = (req.format & ~(BitmapFormatImageRectMask)) |
		     BitmapFormatImageRectMax;
    req.range = TRUE;
    /* each range takes up 4 bytes */
    req.length = (SIZEOF(fsQueryXBitmaps16Req) >> 2) + nranges;
    req.num_ranges = nranges * 2;	/* protocol wants count of fsChar2bs */
    _fs_add_req_log(conn, FS_QueryXBitmaps16);
    _fs_write(conn, (char *) &req, SIZEOF(fsQueryXBitmaps16Req));

    blockrec->sequenceNumber = conn->current_seq;

    /* Send ranges to the server... pack into a char array by hand
       to avoid structure-packing portability problems and to
       handle swapping for version1 protocol */
    if (nranges)
    {
#define RANGE_BUFFER_SIZE 64
#define RANGE_BUFFER_SIZE_MASK 63
	int i;
	char range_buffer[RANGE_BUFFER_SIZE * 4];
	char *range_buffer_p;

	range_buffer_p = range_buffer;
	for (i = 0; i < nranges;)
	{
	    if (conn->fsMajorVersion > 1)
	    {
		*range_buffer_p++ = ranges[i].min_char_high;
		*range_buffer_p++ = ranges[i].min_char_low;
		*range_buffer_p++ = ranges[i].max_char_high;
		*range_buffer_p++ = ranges[i].max_char_low;
	    }
	    else
	    {
		*range_buffer_p++ = ranges[i].min_char_low;
		*range_buffer_p++ = ranges[i].min_char_high;
		*range_buffer_p++ = ranges[i].max_char_low;
		*range_buffer_p++ = ranges[i].max_char_high;
	    }

	    if (!(++i & RANGE_BUFFER_SIZE_MASK))
	    {
		_fs_write(conn, range_buffer, RANGE_BUFFER_SIZE * 4);
		range_buffer_p = range_buffer;
	    }
	}
	if (i &= RANGE_BUFFER_SIZE_MASK)
	    _fs_write(conn, range_buffer, i * 4);
    }

    _fs_prepare_for_reply (conn);
    return Suspended;
}

static int
_fs_load_glyphs(pointer client, FontPtr pfont, Bool range_flag,
		unsigned int nchars, int item_size, unsigned char *data)
{
    FSFpePtr		    conn = (FSFpePtr) pfont->fpe->private;
    int			    nranges = 0;
    fsRange		    *ranges = NULL;
    int			    res;
    FSBlockDataPtr	    blockrec;
    FSBlockedGlyphPtr	    blockedglyph;
    FSClientsDependingPtr   *clients_depending = NULL;
    int			    err;

    /* see if the result is already there */
    for (blockrec = conn->blockedRequests; blockrec; blockrec = blockrec->next)
    {
	if (blockrec->type == FS_LOAD_GLYPHS)
	{
	    blockedglyph = (FSBlockedGlyphPtr) blockrec->data;
	    if (blockedglyph->pfont == pfont)
	    {
		/* Look for this request */
		if (blockrec->client == client)
		{
		    err = blockrec->errcode;
		    if (err == StillWorking)
			return Suspended;
		    _fs_signal_clients_depending(&blockedglyph->clients_depending);
		    _fs_remove_block_rec(conn, blockrec);
		    return err;
		}
		/* We've found an existing LoadGlyphs blockrec for this
		   font but for another client.  Rather than build a
		   blockrec for it now (which entails some complex
		   maintenance), we'll add it to a queue of clients to
		   be signalled when the existing LoadGlyphs is
		   completed.  */
		clients_depending = &blockedglyph->clients_depending;
		break;
	    }
	}
	else if (blockrec->type == FS_OPEN_FONT)
	{
	    FSBlockedFontPtr bfont;
	    bfont = (FSBlockedFontPtr) blockrec->data;
	    if (bfont->pfont == pfont)
	    {
		/*
		 * An OpenFont is pending for this font, this must
		 * be from a reopen attempt, so finish the open
		 * attempt and retry the LoadGlyphs
		 */
		if (blockrec->client == client)
		{
		    err = blockrec->errcode;
		    if (err == StillWorking)
			return Suspended;

		    _fs_signal_clients_depending(&bfont->clients_depending);
		    _fs_remove_block_rec(conn, blockrec);
		    if (err != Successful)
			return err;
		    break;
		}
		/* We've found an existing OpenFont blockrec for this
		   font but for another client.  Rather than build a
		   blockrec for it now (which entails some complex
		   maintenance), we'll add it to a queue of clients to
		   be signalled when the existing OpenFont is
		   completed.  */
		if (blockrec->errcode == StillWorking)
		{
		    clients_depending = &bfont->clients_depending;
		    break;
		}
	    }
	}
    }

    /*
     * see if the desired glyphs already exist, and return Successful if they
     * do, otherwise build up character range/character string
     */
    res = fs_build_range(pfont, range_flag, nchars, item_size, data,
			 &nranges, &ranges);

    switch (res)
    {
	case AccessDone:
	    return Successful;

	case Successful:
	    break;

	default:
	    return res;
    }

    /*
     * If clients_depending is not null, this request must wait for
     * some prior request(s) to complete.
     */
    if (clients_depending)
    {
	/* Since we're not ready to send the load_glyphs request yet,
	   clean up the damage (if any) caused by the fs_build_range()
	   call. */
	if (nranges)
	{
	    _fs_clean_aborted_loadglyphs(pfont, nranges, ranges);
	    free(ranges);
	}
	return _fs_add_clients_depending(clients_depending, client);
    }

    /*
     * If fsd->generation != conn->generation, the font has been closed
     * due to a lost connection.  We will reopen it, which will result
     * in one of three things happening:
     *	 1) The open will succeed and obtain the same font.  Life
     *	    is wonderful.
     *	 2) The open will fail.  There is code above to recognize this
     *	    and flunk the LoadGlyphs request.  The client might not be
     *	    thrilled.
     *	 3) Worst case: the open will succeed but the font we open will
     *	    be different.  The fs_read_query_info() procedure attempts
     *	    to detect this by comparing the existing metrics and
     *	    properties against those of the reopened font... if they
     *	    don't match, we flunk the reopen, which eventually results
     *	    in flunking the LoadGlyphs request.  We could go a step
     *	    further and compare the extents, but this should be
     *	    sufficient.
     */
    if (((FSFontDataPtr)pfont->fpePrivate)->generation != conn->generation)
    {
	/* Since we're not ready to send the load_glyphs request yet,
	   clean up the damage caused by the fs_build_range() call. */
	_fs_clean_aborted_loadglyphs(pfont, nranges, ranges);
	free(ranges);

	/* Now try to reopen the font. */
	return fs_send_open_font(client, pfont->fpe,
				 (Mask)FontReopen, (char *)0, 0,
				 (fsBitmapFormat)0, (fsBitmapFormatMask)0,
				 (XID)0, &pfont);
    }

    return fs_send_load_glyphs(client, pfont, nranges, ranges);
}

int
fs_load_all_glyphs(FontPtr pfont)
{
    int		err;
    FSFpePtr	conn = (FSFpePtr) pfont->fpe->private;

    /*
     * The purpose of this procedure is to load all glyphs in the event
     * that we're dealing with someone who doesn't understand the finer
     * points of glyph caching...  it is called from _fs_get_glyphs() if
     * the latter is called to get glyphs that have not yet been loaded.
     * We assume that the caller will not know how to handle a return
     * value of Suspended (usually the case for a GetGlyphs() caller),
     * so this procedure hangs around, freezing the server, for the
     * request to complete.  This is an unpleasant kluge called to
     * perform an unpleasant job that, we hope, will never be required.
     */

    while ((err = _fs_load_glyphs(__GetServerClient(), pfont, TRUE, 0, 0, NULL)) ==
	   Suspended)
    {
	if (fs_await_reply (conn) != FSIO_READY)
	{
	    /* Get rid of blockrec */
	    fs_client_died(__GetServerClient(), pfont->fpe);
	    err = BadCharRange;
	    break;
	}
	fs_read_reply (pfont->fpe, __GetServerClient());
    }
    return err;
}

static int
fs_read_list(FontPathElementPtr fpe, FSBlockDataPtr blockrec)
{
    FSFpePtr		conn = (FSFpePtr) fpe->private;
    FSBlockedListPtr	blist = (FSBlockedListPtr) blockrec->data;
    fsListFontsReply	*rep;
    char		*data;
    long		dataleft; /* length of reply left to use */
    int			length,
			i,
			ret;
    int			err;

    rep = (fsListFontsReply *) fs_get_reply (conn, &ret);
    if (!rep || rep->type == FS_Error ||
	(rep->length < LENGTHOF(fsListFontsReply)))
    {
	if (ret == FSIO_BLOCK)
	    return StillWorking;
	if (rep)
	    _fs_done_read (conn, rep->length << 2);
	_fs_reply_failed (rep, fsListFontsReply, "<");
	return AllocError;
    }
    data = (char *) rep + SIZEOF (fsListFontsReply);
    dataleft = (rep->length << 2) - SIZEOF (fsListFontsReply);

    err = Successful;
    /* copy data into FontPathRecord */
    for (i = 0; i < rep->nFonts; i++)
    {
	if (dataleft < 1)
	    break;
	length = *(unsigned char *)data++;
	dataleft--; /* used length byte */
	if (length > dataleft) {
#ifdef DEBUG
	    fprintf(stderr,
		    "fsListFonts: name length (%d) > dataleft (%ld)\n",
		    length, dataleft);
#endif
	    err = BadFontName;
	    break;
	}
	err = xfont2_add_font_names_name(blist->names, data, length);
	if (err != Successful)
	    break;
	data += length;
	dataleft -= length;
    }
    _fs_done_read (conn, rep->length << 2);
    return err;
}

static int
fs_send_list_fonts(pointer client, FontPathElementPtr fpe, const char *pattern,
		   int patlen, int maxnames, FontNamesPtr newnames)
{
    FSFpePtr		conn = (FSFpePtr) fpe->private;
    FSBlockDataPtr	blockrec;
    FSBlockedListPtr	blockedlist;
    fsListFontsReq	req;

    if (conn->blockState & FS_GIVE_UP)
	return BadFontName;

    /* make a new block record, and add it to the end of the list */
    blockrec = fs_new_block_rec(fpe, client, FS_LIST_FONTS);
    if (!blockrec)
	return AllocError;
    blockedlist = (FSBlockedListPtr) blockrec->data;
    blockedlist->names = newnames;

    if (conn->blockState & (FS_BROKEN_CONNECTION | FS_RECONNECTING))
    {
	_fs_pending_reply (conn);
	return Suspended;
    }

    _fs_client_access (conn, client, FALSE);
    _fs_client_resolution(conn);

    /* send the request */
    req.reqType = FS_ListFonts;
    req.pad = 0;
    req.maxNames = maxnames;
    req.nbytes = patlen;
    req.length = (SIZEOF(fsListFontsReq) + patlen + 3) >> 2;
    _fs_add_req_log(conn, FS_ListFonts);
    _fs_write(conn, (char *) &req, SIZEOF(fsListFontsReq));
    _fs_write_pad(conn, (char *) pattern, patlen);

    blockrec->sequenceNumber = conn->current_seq;

#ifdef NCD
    if (configData.ExtendedFontDiags) {
	char        buf[256];

	memcpy(buf, pattern, MIN(256, patlen));
	buf[MIN(256, patlen)] = '\0';
	printf("Listing fonts on pattern \"%s\" from font server \"%s\"\n",
	       buf, fpe->name);
    }
#endif

    _fs_prepare_for_reply (conn);
    return Suspended;
}

static int
fs_list_fonts(pointer client, FontPathElementPtr fpe,
	      const char *pattern, int patlen, int maxnames, FontNamesPtr newnames)
{
    FSFpePtr		conn = (FSFpePtr) fpe->private;
    FSBlockDataPtr	blockrec;
    int			err;

    /* see if the result is already there */
    for (blockrec = conn->blockedRequests; blockrec; blockrec = blockrec->next)
    {
	if (blockrec->type == FS_LIST_FONTS && blockrec->client == client)
	{
	    err = blockrec->errcode;
	    if (err == StillWorking)
		return Suspended;
	    _fs_remove_block_rec(conn, blockrec);
	    return err;
	}
    }

    /* didn't find waiting record, so send a new one */
    return fs_send_list_fonts(client, fpe, pattern, patlen, maxnames, newnames);
}

/*
 * Read a single list info reply and restart for the next reply
 */
static int
fs_read_list_info(FontPathElementPtr fpe, FSBlockDataPtr blockrec)
{
    FSBlockedListInfoPtr	binfo = (FSBlockedListInfoPtr) blockrec->data;
    fsListFontsWithXInfoReply	*rep;
    char			*buf;
    long			bufleft;
    FSFpePtr			conn = (FSFpePtr) fpe->private;
    fsPropInfo			*pi;
    fsPropOffset		*po;
    pointer			pd;
    int				ret;
    int				err;

    /* clean up anything from the last trip */
    _fs_free_props (&binfo->info);

    rep = (fsListFontsWithXInfoReply *) fs_get_reply (conn, &ret);
    if (!rep || rep->type == FS_Error ||
	((rep->nameLength != 0) &&
	 (rep->length < LENGTHOF(fsListFontsWithXInfoReply))))
    {
	if (ret == FSIO_BLOCK)
	    return StillWorking;
	binfo->status = FS_LFWI_FINISHED;
	err = AllocError;
	_fs_reply_failed (rep, fsListFontsWithXInfoReply, "<");
	goto done;
    }
    /*
     * Normal termination -- the list ends with a name of length 0
     */
    if (rep->nameLength == 0)
    {
#ifdef DEBUG
	fprintf (stderr, "fs_read_list_info done\n");
#endif
	binfo->status = FS_LFWI_FINISHED;
	err = BadFontName;
	goto done;
    }

    buf = (char *) rep + SIZEOF (fsListFontsWithXInfoReply);
    bufleft = (rep->length << 2) - SIZEOF (fsListFontsWithXInfoReply);

    /*
     * The original FS implementation didn't match
     * the spec, version 1 was respecified to match the FS.
     * Version 2 matches the original intent
     */
    if (conn->fsMajorVersion <= 1)
    {
	if (rep->nameLength > bufleft) {
#ifdef DEBUG
	    fprintf(stderr,
		    "fsListFontsWithXInfo: name length (%d) > bufleft (%ld)\n",
		    (int) rep->nameLength, bufleft);
#endif
	    err = AllocError;
	    goto done;
	}
	/* binfo->name is a 256 char array, rep->nameLength is a CARD8 */
	memcpy (binfo->name, buf, rep->nameLength);
	buf += _fs_pad_length (rep->nameLength);
	bufleft -= _fs_pad_length (rep->nameLength);
    }
    pi = (fsPropInfo *) buf;
    if (SIZEOF (fsPropInfo) > bufleft) {
#ifdef DEBUG
	fprintf(stderr,
		"fsListFontsWithXInfo: PropInfo length (%d) > bufleft (%ld)\n",
		(int) SIZEOF (fsPropInfo), bufleft);
#endif
	err = AllocError;
	goto done;
    }
    bufleft -= SIZEOF (fsPropInfo);
    buf += SIZEOF (fsPropInfo);
    po = (fsPropOffset *) buf;
    if (pi->num_offsets > (bufleft / SIZEOF (fsPropOffset))) {
#ifdef DEBUG
	fprintf(stderr,
		"fsListFontsWithXInfo: offset length (%u * %d) > bufleft (%ld)\n",
		(unsigned) pi->num_offsets, (int) SIZEOF (fsPropOffset), bufleft);
#endif
	err = AllocError;
	goto done;
    }
    bufleft -= pi->num_offsets * SIZEOF (fsPropOffset);
    buf += pi->num_offsets * SIZEOF (fsPropOffset);
    pd = (pointer) buf;
    if (pi->data_len > bufleft) {
#ifdef DEBUG
	fprintf(stderr,
		"fsListFontsWithXInfo: data length (%u) > bufleft (%ld)\n",
		(unsigned) pi->data_len, bufleft);
#endif
	err = AllocError;
	goto done;
    }
    bufleft -= pi->data_len;
    buf += pi->data_len;
    if (conn->fsMajorVersion > 1)
    {
	if (rep->nameLength > bufleft) {
#ifdef DEBUG
	    fprintf(stderr,
		    "fsListFontsWithXInfo: name length (%d) > bufleft (%ld)\n",
		    (int) rep->nameLength, bufleft);
#endif
	    err = AllocError;
	    goto done;
	}
	/* binfo->name is a 256 char array, rep->nameLength is a CARD8 */
	memcpy (binfo->name, buf, rep->nameLength);
	buf += _fs_pad_length (rep->nameLength);
	bufleft -= _fs_pad_length (rep->nameLength);
    }

#ifdef DEBUG
    binfo->name[rep->nameLength] = '\0';
    fprintf (stderr, "fs_read_list_info %s\n", binfo->name);
#endif
    err = _fs_convert_lfwi_reply(conn, &binfo->info, rep, pi, po, pd);
    if (err != Successful)
    {
	binfo->status = FS_LFWI_FINISHED;
	goto done;
    }
    binfo->namelen = rep->nameLength;
    binfo->remaining = rep->nReplies;

    binfo->status = FS_LFWI_REPLY;

    /* disable this font server until we've processed this response */
    _fs_unmark_block (conn, FS_COMPLETE_REPLY);
    conn_stop_listening(conn);
done:
    _fs_done_read (conn, rep->length << 2);
    return err;
}

/* ARGSUSED */
static int
fs_start_list_with_info(pointer client, FontPathElementPtr fpe,
			const char *pattern, int len, int maxnames, pointer *pdata)
{
    FSFpePtr		    conn = (FSFpePtr) fpe->private;
    FSBlockDataPtr	    blockrec;
    FSBlockedListInfoPtr    binfo;
    fsListFontsWithXInfoReq req;

    if (conn->blockState & FS_GIVE_UP)
	return BadFontName;

    /* make a new block record, and add it to the end of the list */
    blockrec = fs_new_block_rec(fpe, client, FS_LIST_WITH_INFO);
    if (!blockrec)
	return AllocError;

    binfo = (FSBlockedListInfoPtr) blockrec->data;
    bzero((char *) binfo, sizeof(FSBlockedListInfoRec));
    binfo->status = FS_LFWI_WAITING;

    if (conn->blockState & (FS_BROKEN_CONNECTION | FS_RECONNECTING))
    {
	_fs_pending_reply (conn);
	return Suspended;
    }

    _fs_client_access (conn, client, FALSE);
    _fs_client_resolution(conn);

    /* send the request */
    req.reqType = FS_ListFontsWithXInfo;
    req.pad = 0;
    req.maxNames = maxnames;
    req.nbytes = len;
    req.length = (SIZEOF(fsListFontsWithXInfoReq) + len + 3) >> 2;
    _fs_add_req_log(conn, FS_ListFontsWithXInfo);
    (void) _fs_write(conn, (char *) &req, SIZEOF(fsListFontsWithXInfoReq));
    (void) _fs_write_pad(conn, pattern, len);

    blockrec->sequenceNumber = conn->current_seq;

#ifdef NCD
    if (configData.ExtendedFontDiags) {
	char        buf[256];

	memcpy(buf, pattern, MIN(256, len));
	buf[MIN(256, len)] = '\0';
	printf("Listing fonts with info on pattern \"%s\" from font server \"%s\"\n",
	       buf, fpe->name);
    }
#endif

    _fs_prepare_for_reply (conn);
    return Successful;
}

/* ARGSUSED */
static int
fs_next_list_with_info(pointer client, FontPathElementPtr fpe,
		       char **namep, int *namelenp,
		       FontInfoPtr *pFontInfo, int *numFonts,
		       pointer private)
{
    FSFpePtr		    conn = (FSFpePtr) fpe->private;
    FSBlockDataPtr	    blockrec;
    FSBlockedListInfoPtr    binfo;
    int			    err;

    /* see if the result is already there */
    for (blockrec = conn->blockedRequests; blockrec; blockrec = blockrec->next)
	if (blockrec->type == FS_LIST_WITH_INFO && blockrec->client == client)
	    break;

    if (!blockrec)
    {
	/* The only good reason for not finding a blockrec would be if
	   disconnect/reconnect to the font server wiped it out and the
	   code that called us didn't do the right thing to create
	   another one.  Under those circumstances, we need to return an
	   error to prevent that code from attempting to interpret the
	   information we don't return.  */
	return BadFontName;
    }

    binfo = (FSBlockedListInfoPtr) blockrec->data;

    if (binfo->status == FS_LFWI_WAITING)
	return Suspended;

    *namep = binfo->name;
    *namelenp = binfo->namelen;
    *pFontInfo = &binfo->info;
    *numFonts = binfo->remaining;

    /* Restart reply processing from this font server */
    conn_start_listening(conn);
    if (fs_reply_ready (conn))
	_fs_mark_block (conn, FS_COMPLETE_REPLY);

    err = blockrec->errcode;
    switch (binfo->status) {
    case FS_LFWI_FINISHED:
	_fs_remove_block_rec(conn, blockrec);
	break;
    case FS_LFWI_REPLY:
	binfo->status = FS_LFWI_WAITING;
	blockrec->errcode = StillWorking;
	conn->blockedReplyTime = GetTimeInMillis () + FontServerRequestTimeout;
	_fs_mark_block (conn, FS_PENDING_REPLY);
	break;
    }

    return err;
}

/*
 * Called when client exits
 */

static void
fs_client_died(pointer client, FontPathElementPtr fpe)
{
    FSFpePtr	    conn = (FSFpePtr) fpe->private;
    FSBlockDataPtr  blockrec,
		    depending;
    FSClientPtr	    *prev, cur;
    fsFreeACReq	    freeac;

    for (prev = &conn->clients; (cur = *prev); prev = &cur->next)
    {
	if (cur->client == client) {
	    freeac.reqType = FS_FreeAC;
	    freeac.pad = 0;
	    freeac.id = cur->acid;
	    freeac.length = sizeof (fsFreeACReq) >> 2;
	    _fs_add_req_log(conn, FS_FreeAC);
	    _fs_write (conn, (char *) &freeac, sizeof (fsFreeACReq));
	    *prev = cur->next;
	    free (cur);
	    break;
	}
    }
    /* find a pending requests */
    for (blockrec = conn->blockedRequests; blockrec; blockrec = blockrec->next)
	if (blockrec->client == client)
	    break;

    if (!blockrec)
	return;

    /* replace the client pointers in this block rec with the chained one */
    if ((depending = blockrec->depending))
    {
	blockrec->client = depending->client;
	blockrec->depending = depending->depending;
	blockrec = depending;
    }
    fs_abort_blockrec(conn, blockrec);
}

static void
_fs_client_access (FSFpePtr conn, pointer client, Bool sync)
{
    FSClientPtr	*prev,	    cur;
    fsCreateACReq	    crac;
    fsSetAuthorizationReq   setac;
    char		    *authorizations;
    int			    authlen;
    Bool		    new_cur = FALSE;
    char		    padding[4] = { 0, 0, 0, 0 };

#ifdef DEBUG
    if (conn->blockState & (FS_RECONNECTING|FS_BROKEN_CONNECTION))
    {
	fprintf (stderr, "Sending requests without a connection\n");
    }
#endif
    for (prev = &conn->clients; (cur = *prev); prev = &cur->next)
    {
	if (cur->client == client)
	{
	    if (prev != &conn->clients)
	    {
		*prev = cur->next;
		cur->next = conn->clients;
		conn->clients = cur;
	    }
	    break;
	}
    }
    if (!cur)
    {
	cur = malloc (sizeof (FSClientRec));
	if (!cur)
	    return;
	cur->client = client;
	cur->next = conn->clients;
	conn->clients = cur;
	cur->acid = GetNewFontClientID ();
	new_cur = TRUE;
    }
    if (new_cur || cur->auth_generation != client_auth_generation(client))
    {
	if (!new_cur)
	{
	    fsFreeACReq	freeac;
	    freeac.reqType = FS_FreeAC;
	    freeac.pad = 0;
	    freeac.id = cur->acid;
	    freeac.length = sizeof (fsFreeACReq) >> 2;
	    _fs_add_req_log(conn, FS_FreeAC);
	    _fs_write (conn, (char *) &freeac, sizeof (fsFreeACReq));
	}
	crac.reqType = FS_CreateAC;
	crac.num_auths = set_font_authorizations(&authorizations, &authlen,
						 client);
	/* Work around bug in xfs versions up through modular release 1.0.8
	   which rejects CreateAC packets with num_auths = 0 & authlen < 4 */
	if (crac.num_auths == 0) {
	    authorizations = padding;
	    authlen = 4;
	}
	crac.length = (sizeof (fsCreateACReq) + authlen + 3) >> 2;
	crac.acid = cur->acid;
	_fs_add_req_log(conn, FS_CreateAC);
	_fs_write(conn, (char *) &crac, sizeof (fsCreateACReq));
	_fs_write_pad(conn, authorizations, authlen);
	/* ignore reply; we don't even care about it */
	conn->curacid = 0;
	cur->auth_generation = client_auth_generation(client);
    }
    if (conn->curacid != cur->acid)
    {
    	setac.reqType = FS_SetAuthorization;
	setac.pad = 0;
    	setac.length = sizeof (fsSetAuthorizationReq) >> 2;
    	setac.id = cur->acid;
    	_fs_add_req_log(conn, FS_SetAuthorization);
    	_fs_write(conn, (char *) &setac, sizeof (fsSetAuthorizationReq));
	conn->curacid = cur->acid;
    }
}

/*
 * Poll a pending connect
 */

static int
_fs_check_connect (FSFpePtr conn)
{
    int	    ret;

    ret = _fs_poll_connect (conn->trans_conn, 0);
    switch (ret) {
    case FSIO_READY:
	conn->fs_fd = _FontTransGetConnectionNumber (conn->trans_conn);
	conn_start_listening(conn);
	break;
    case FSIO_BLOCK:
	break;
    }
    return ret;
}

/*
 * Return an FSIO status while waiting for the completed connection
 * reply to arrive
 */

static fsConnSetup *
_fs_get_conn_setup (FSFpePtr conn, int *error, int *setup_len)
{
    int			ret;
    char		*data;
    int			headlen;
    int			len;
    fsConnSetup		*setup;
    fsConnSetupAccept	*accept;

    ret = _fs_start_read (conn, SIZEOF (fsConnSetup), &data);
    if (ret != FSIO_READY)
    {
	*error = ret;
	return 0;
    }

    setup = (fsConnSetup *) data;
    if (setup->major_version > FS_PROTOCOL)
    {
	*error = FSIO_ERROR;
	return 0;
    }

    headlen = (SIZEOF (fsConnSetup) +
	       (setup->alternate_len << 2) +
	       (setup->auth_len << 2));
    /* On anything but Success, no extra data is sent */
    if (setup->status != AuthSuccess)
    {
	len = headlen;
    }
    else
    {
	ret = _fs_start_read (conn, headlen + SIZEOF (fsConnSetupAccept), &data);
	if (ret != FSIO_READY)
	{
	    *error = ret;
	    return 0;
	}
	setup = (fsConnSetup *) data;
	accept = (fsConnSetupAccept *) (data + headlen);
	len = headlen + (accept->length << 2);
    }
    ret = _fs_start_read (conn, len, &data);
    if (ret != FSIO_READY)
    {
	*error = ret;
	return 0;
    }
    *setup_len = len;
    return (fsConnSetup *) data;
}

static int
_fs_send_conn_client_prefix (FSFpePtr conn)
{
    fsConnClientPrefix	req;
    int			endian;
    int			ret;

    /* send setup prefix */
    endian = 1;
    if (*(char *) &endian)
	req.byteOrder = 'l';
    else
	req.byteOrder = 'B';

    req.major_version = FS_PROTOCOL;
    req.minor_version = FS_PROTOCOL_MINOR;

/* XXX add some auth info here */
    req.num_auths = 0;
    req.auth_len = 0;
    ret = _fs_write (conn, (char *) &req, SIZEOF (fsConnClientPrefix));
    if (ret != FSIO_READY)
	return FSIO_ERROR;
    conn->blockedConnectTime = GetTimeInMillis () + FontServerRequestTimeout;
    return ret;
}

static int
_fs_recv_conn_setup (FSFpePtr conn)
{
    int			ret = FSIO_ERROR;
    fsConnSetup		*setup;
    FSFpeAltPtr		alts;
    unsigned int	i, alt_len;
    int			setup_len;
    char		*alt_save, *alt_names;

    setup = _fs_get_conn_setup (conn, &ret, &setup_len);
    if (!setup)
	return ret;
    conn->current_seq = 0;
    conn->fsMajorVersion = setup->major_version;
    /*
     * Create an alternate list from the initial server, but
     * don't chain looking for alternates.
     */
    if (conn->alternate == 0)
    {
	/*
	 * free any existing alternates list, allowing the list to
	 * be updated
	 */
	if (conn->alts)
	{
	    free (conn->alts);
	    conn->alts = 0;
	    conn->numAlts = 0;
	}
	if (setup->num_alternates)
	{
	    size_t alt_name_len = setup->alternate_len << 2;
	    alts = malloc (setup->num_alternates * sizeof (FSFpeAltRec) +
			   alt_name_len);
	    if (alts)
	    {
		alt_names = (char *) (setup + 1);
		alt_save = (char *) (alts + setup->num_alternates);
		for (i = 0; i < setup->num_alternates; i++)
		{
		    alts[i].subset = alt_names[0];
		    alt_len = alt_names[1];
		    if (alt_len >= alt_name_len) {
			/*
			 * Length is longer than setup->alternate_len
			 * told us to allocate room for, assume entire
			 * alternate list is corrupted.
			 */
#ifdef DEBUG
			fprintf (stderr,
				 "invalid alt list (length %lx >= %lx)\n",
				 (long) alt_len, (long) alt_name_len);
#endif
			free(alts);
			return FSIO_ERROR;
		    }
		    alts[i].name = alt_save;
		    memcpy (alt_save, alt_names + 2, alt_len);
		    alt_save[alt_len] = '\0';
		    alt_save += alt_len + 1;
		    alt_name_len -= alt_len + 1;
		    alt_names += _fs_pad_length (alt_len + 2);
		}
		conn->numAlts = setup->num_alternates;
		conn->alts = alts;
	    }
	}
    }
    _fs_done_read (conn, setup_len);
    if (setup->status != AuthSuccess)
	return FSIO_ERROR;
    return FSIO_READY;
}

static int
_fs_open_server (FSFpePtr conn)
{
    int	    ret;
    char    *servername;

    if (conn->alternate == 0)
	servername = conn->servername;
    else
	servername = conn->alts[conn->alternate-1].name;
    conn->trans_conn = _fs_connect (servername, &ret);
    conn->blockedConnectTime = GetTimeInMillis () + FS_RECONNECT_WAIT;
    return ret;
}

static char *
_fs_catalog_name (char *servername)
{
    char    *sp;

    sp = strchr (servername, '/');
    if (!sp)
	return 0;
    return strrchr (sp + 1, '/');
}

static int
_fs_send_init_packets (FSFpePtr conn)
{
    fsSetResolutionReq	    srreq;
    fsSetCataloguesReq	    screq;
    int			    num_cats,
			    clen;
    char		    *catalogues;
    char		    *cat;
    char		    len;
    char		    *end;
    int			    num_res;
    FontResolutionPtr	    res;

#define	CATALOGUE_SEP	'+'

    res = GetClientResolutions(&num_res);
    if (num_res)
    {
	srreq.reqType = FS_SetResolution;
	srreq.num_resolutions = num_res;
	srreq.length = (SIZEOF(fsSetResolutionReq) +
			(num_res * SIZEOF(fsResolution)) + 3) >> 2;

	_fs_add_req_log(conn, FS_SetResolution);
	if (_fs_write(conn, (char *) &srreq, SIZEOF(fsSetResolutionReq)) != FSIO_READY)
	    return FSIO_ERROR;
	if (_fs_write_pad(conn, (char *) res, (num_res * SIZEOF(fsResolution))) != FSIO_READY)
	    return FSIO_ERROR;
    }

    catalogues = 0;
    if (conn->alternate != 0)
	catalogues = _fs_catalog_name (conn->alts[conn->alternate-1].name);
    if (!catalogues)
	catalogues = _fs_catalog_name (conn->servername);

    if (!catalogues)
    {
	conn->has_catalogues = FALSE;
	return FSIO_READY;
    }
    conn->has_catalogues = TRUE;

    /* turn cats into counted list */
    catalogues++;

    cat = catalogues;
    num_cats = 0;
    clen = 0;
    while (*cat)
    {
	num_cats++;
	end = strchr(cat, CATALOGUE_SEP);
	if (!end)
	    end = cat + strlen (cat);
	clen += (end - cat) + 1;	/* length byte + string */
	cat = end;
    }

    screq.reqType = FS_SetCatalogues;
    screq.num_catalogues = num_cats;
    screq.length = (SIZEOF(fsSetCataloguesReq) + clen + 3) >> 2;

    _fs_add_req_log(conn, FS_SetCatalogues);
    if (_fs_write(conn, (char *) &screq, SIZEOF(fsSetCataloguesReq)) != FSIO_READY)
	return FSIO_ERROR;

    while (*cat)
    {
	num_cats++;
	end = strchr(cat, CATALOGUE_SEP);
	if (!end)
	    end = cat + strlen (cat);
	len = end - cat;
	if (_fs_write (conn, &len, 1) != FSIO_READY)
	    return FSIO_ERROR;
	if (_fs_write (conn, cat, (int) len) != FSIO_READY)
	    return FSIO_ERROR;
	cat = end;
    }

    if (_fs_write (conn, "....", _fs_pad_length (clen) - clen) != FSIO_READY)
	return FSIO_ERROR;

    return FSIO_READY;
}

static int
_fs_send_cat_sync (FSFpePtr conn)
{
    fsListCataloguesReq	    lcreq;

    /*
     * now sync up with the font server, to see if an error was generated
     * by a bogus catalogue
     */
    lcreq.reqType = FS_ListCatalogues;
    lcreq.data = 0;
    lcreq.length = (SIZEOF(fsListCataloguesReq)) >> 2;
    lcreq.maxNames = 0;
    lcreq.nbytes = 0;
    lcreq.pad2 = 0;
    _fs_add_req_log(conn, FS_SetCatalogues);
    if (_fs_write(conn, (char *) &lcreq, SIZEOF(fsListCataloguesReq)) != FSIO_READY)
	return FSIO_ERROR;
    conn->blockedConnectTime = GetTimeInMillis () + FontServerRequestTimeout;
    return FSIO_READY;
}

static int
_fs_recv_cat_sync (FSFpePtr conn)
{
    fsGenericReply  *reply;
    fsError	    *error;
    int		    err;
    int		    ret;

    reply = fs_get_reply (conn, &err);
    if (!reply)
	return err;

    ret = FSIO_READY;
    if (reply->type == FS_Error)
    {
	error = (fsError *) reply;
	if (error->major_opcode == FS_SetCatalogues)
	    ret = FSIO_ERROR;
    }
    _fs_done_read (conn, reply->length << 2);
    return ret;
}

static void
_fs_close_server (FSFpePtr conn)
{
    _fs_unmark_block (conn, FS_PENDING_WRITE|FS_BROKEN_WRITE|FS_COMPLETE_REPLY|FS_BROKEN_CONNECTION);
    if (conn->trans_conn)
    {
	_FontTransClose (conn->trans_conn);
	conn->trans_conn = 0;
	_fs_io_reinit (conn);
    }
    if (conn->fs_fd >= 0)
    {
	conn_stop_listening(conn);
	conn->fs_fd = -1;
    }
    conn->fs_conn_state = FS_CONN_UNCONNECTED;
}

static int
_fs_do_setup_connection (FSFpePtr conn)
{
    int	    ret;

    do
    {
#ifdef DEBUG
	fprintf (stderr, "fs_do_setup_connection state %d\n", conn->fs_conn_state);
#endif
	switch (conn->fs_conn_state) {
	case FS_CONN_UNCONNECTED:
	    ret = _fs_open_server (conn);
	    if (ret == FSIO_BLOCK)
		conn->fs_conn_state = FS_CONN_CONNECTING;
	    break;
	case FS_CONN_CONNECTING:
	    ret = _fs_check_connect (conn);
	    break;
	case FS_CONN_CONNECTED:
	    ret = _fs_send_conn_client_prefix (conn);
	    break;
	case FS_CONN_SENT_PREFIX:
	    ret = _fs_recv_conn_setup (conn);
	    break;
	case FS_CONN_RECV_INIT:
	    ret = _fs_send_init_packets (conn);
	    if (conn->has_catalogues)
		ret = _fs_send_cat_sync (conn);
	    break;
	case FS_CONN_SENT_CAT:
	    if (conn->has_catalogues)
		ret = _fs_recv_cat_sync (conn);
	    else
		ret = FSIO_READY;
	    break;
	default:
	    ret = FSIO_READY;
	    break;
	}
	switch (ret) {
	case FSIO_READY:
	    if (conn->fs_conn_state < FS_CONN_RUNNING)
		conn->fs_conn_state++;
	    break;
	case FSIO_BLOCK:
	    if (TimeCmp (GetTimeInMillis (), <, conn->blockedConnectTime))
		break;
	    ret = FSIO_ERROR;
	    /* fall through... */
	case FSIO_ERROR:
	    _fs_close_server (conn);
	    /*
	     * Try the next alternate
	     */
	    if (conn->alternate < conn->numAlts)
	    {
		conn->alternate++;
		ret = FSIO_READY;
	    }
	    else
		conn->alternate = 0;
	    break;
	}
    } while (conn->fs_conn_state != FS_CONN_RUNNING && ret == FSIO_READY);
    if (ret == FSIO_READY)
	conn->generation = ++generationCount;
    return ret;
}

static int
_fs_wait_connect (FSFpePtr conn)
{
    int	    ret;

    for (;;)
    {
	ret = _fs_do_setup_connection (conn);
	if (ret != FSIO_BLOCK)
	    break;
	if (conn->fs_conn_state <= FS_CONN_CONNECTING)
	    ret = _fs_poll_connect (conn->trans_conn, 1000);
	else
	    ret = _fs_wait_for_readable (conn, 1000);
	if (ret == FSIO_ERROR)
	    break;
    }
    return ret;
}

/*
 * Poll a connection in the process of reconnecting
 */
static void
_fs_check_reconnect (FSFpePtr conn)
{
    int	    ret;

    ret = _fs_do_setup_connection (conn);
    switch (ret) {
    case FSIO_READY:
	_fs_unmark_block (conn, FS_RECONNECTING|FS_GIVE_UP);
	_fs_restart_connection (conn);
	break;
    case FSIO_BLOCK:
	break;
    case FSIO_ERROR:
	conn->brokenConnectionTime = GetTimeInMillis () + FS_RECONNECT_POLL;
	break;
    }
}

/*
 * Start the reconnection process
 */
static void
_fs_start_reconnect (FSFpePtr conn)
{
    if (conn->blockState & FS_RECONNECTING)
	return;
    conn->alternate = 0;
    _fs_mark_block (conn, FS_RECONNECTING);
    _fs_unmark_block (conn, FS_BROKEN_CONNECTION);
    _fs_check_reconnect (conn);
}


static FSFpePtr
_fs_init_conn (const char *servername, FontPathElementPtr fpe)
{
    FSFpePtr	conn;
    size_t	snlen = strlen (servername) + 1;

    conn = calloc (1, sizeof (FSFpeRec) + snlen);
    if (!conn)
	return 0;
    if (!_fs_io_init (conn))
    {
	free (conn);
	return 0;
    }
    conn->servername = (char *) (conn + 1);
    conn->fs_conn_state = FS_CONN_UNCONNECTED;
    conn->fs_fd = -1;
    conn->fpe = fpe;
    strlcpy (conn->servername, servername, snlen);
    return conn;
}

static void
_fs_free_conn (FSFpePtr conn)
{
    _fs_close_server (conn);
    _fs_io_fini (conn);
    if (conn->alts)
	free (conn->alts);
    free (conn);
}

/*
 * called at server init time
 */

static const xfont2_fpe_funcs_rec fs_fpe_funcs = {
	.version = XFONT2_FPE_FUNCS_VERSION,
	.name_check = fs_name_check,
	.init_fpe = fs_init_fpe,
	.free_fpe = fs_free_fpe,
	.reset_fpe = fs_reset_fpe,
	.open_font = fs_open_font,
	.close_font = fs_close_font,
	.list_fonts = fs_list_fonts,
	.start_list_fonts_with_info = fs_start_list_with_info,
	.list_next_font_with_info = fs_next_list_with_info,
	.wakeup_fpe = fs_wakeup,
	.client_died = fs_client_died,
	.load_glyphs = _fs_load_glyphs,
	.start_list_fonts_and_aliases = (StartLaFunc) 0,
	.list_next_font_or_alias = (NextLaFunc) 0,
	.set_path_hook = (SetPathFunc) 0
};

void
fs_register_fpe_functions(void)
{
    register_fpe_funcs(&fs_fpe_funcs);
}
