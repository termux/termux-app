/*
 * Copyright 1990 Network Computing Devices
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Network Computing Devices not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Network Computing
 * Devices makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * NETWORK COMPUTING DEVICES DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL NETWORK COMPUTING DEVICES BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  	Dave Lemke, Network Computing Devices, Inc
 */

#ifndef	_FSIO_H_
#define	_FSIO_H_

#ifdef DEBUG
#define	REQUEST_LOG_SIZE	100
#endif

typedef struct _fs_fpe_alternate {
    char       *name;
    Bool        subset;
}           FSFpeAltRec, *FSFpeAltPtr;


/* Per client access contexts */
typedef struct _fs_client_data {
    pointer		    client;
    struct _fs_client_data  *next;
    XID			    acid;
    int			    auth_generation;
} FSClientRec, *FSClientPtr;

#define FS_RECONNECT_POLL	1000
#define FS_RECONNECT_WAIT	5000
#define FS_GIVEUP_WAIT		20000
#define FS_REQUEST_TIMEOUT	20000
#define FS_OPEN_TIMEOUT		30000
#define FS_REOPEN_TIMEOUT	10000
#define FS_FLUSH_POLL		1000

typedef struct _fs_buf {
    char    *buf;		/* data */
    long    size;		/* sizeof data */
    long    insert;		/* where to insert new data */
    long    remove;		/* where to remove old data */
} FSBufRec, *FSBufPtr;

#define FS_BUF_INC  1024
#define FS_BUF_MAX  32768

#define FS_PENDING_WRITE	0x01	    /* some write data is queued */
#define FS_BROKEN_WRITE		0x02	    /* writes are broken */
#define FS_BROKEN_CONNECTION	0x04	    /* connection is broken */
#define FS_PENDING_REPLY	0x08	    /* waiting for a reply */
#define FS_GIVE_UP		0x10	    /* font server declared useless */
#define FS_COMPLETE_REPLY	0x20	    /* complete reply ready */
#define FS_RECONNECTING		0x40

#define FS_CONN_UNCONNECTED	0
#define FS_CONN_CONNECTING	1
#define FS_CONN_CONNECTED	2
#define FS_CONN_SENT_PREFIX	3
#define FS_CONN_RECV_INIT	4
#define FS_CONN_SENT_CAT    	5
#define FS_CONN_RUNNING		6

/* FS specific font FontPathElement data */
typedef struct _fs_fpe_data {
    FSFpePtr	next;		/* list of all active fs fpes */
    FontPathElementPtr fpe;	/* Back pointer to FPE */
    int         fs_fd;		/* < 0 when not running */
    Bool	fs_listening;	/* Listening for input */
    int		fs_conn_state;	/* connection state */
    int         current_seq;
    char       *servername;
    Bool	has_catalogues;

    int         generation;
    int         numAlts;
    int		alternate;	/* which alternate is in use +1 */
    int		fsMajorVersion; /* font server major version number */
    FSFpeAltPtr alts;

    FSClientPtr	clients;
    XID		curacid;
#ifdef DEBUG
    int         reqindex;
    struct {
	int	opcode;
	int	sequence;
    } reqbuffer[REQUEST_LOG_SIZE];
#endif
    FSBufRec	outBuf;		/* request queue */
    FSBufRec	inBuf;		/* reply queue */
    long	inNeed;		/* amount needed for reply */

    CARD32	blockState;
    CARD32	blockedReplyTime;	/* time to abort blocked read */
    CARD32	brokenWriteTime;	/* time to retry broken write */
    CARD32	blockedConnectTime;	/* time to abort blocked connect */
    CARD32	brokenConnectionTime;	/* time to retry broken connection */

    FSBlockDataPtr  blockedRequests;

    struct _XtransConnInfo *trans_conn; /* transport connection object */
}           FSFpeRec;

#define fs_outspace(conn)   ((conn)->outBuf.size - (conn)->outBuf.insert)
#define fs_outqueued(conn)  ((conn)->outBuf.insert - (conn)->outBuf.remove)
#define fs_inqueued(conn)   ((conn)->inBuf.insert - (conn)->inBuf.remove)
#define fs_needsflush(conn) (fs_outqueued(conn) != 0)
#define fs_needsfill(conn)  (fs_inqueued(conn) < (conn)->inNeed)
#define fs_needsconnect(conn)	((conn)->fs_fd < 0)
#define fs_data_read(conn)   ((conn)->inBuf.insert - (conn)->inBuf.remove)

#define FSIO_READY  1
#define FSIO_BLOCK  0
#define FSIO_ERROR  -1

extern Bool _fs_reopen_server ( FSFpePtr conn );
extern int _fs_write ( FSFpePtr conn, const char *data, long size );
extern int _fs_write_pad ( FSFpePtr conn, const char *data, long len );
extern int _fs_wait_for_readable ( FSFpePtr conn, int ms );
extern long _fs_pad_length (long len);

extern void _fs_connection_died ( FSFpePtr conn );

extern int  _fs_flush (FSFpePtr conn);
extern void _fs_mark_block (FSFpePtr conn, CARD32 mask);
extern void _fs_unmark_block (FSFpePtr conn, CARD32 mask);
extern void _fs_done_read (FSFpePtr conn, long size);
extern void _fs_io_reinit (FSFpePtr conn);
extern int  _fs_start_read (FSFpePtr conn, long size, char **buf);
extern Bool _fs_io_init (FSFpePtr conn);
extern void _fs_io_fini (FSFpePtr conn);
extern int  _fs_poll_connect (XtransConnInfo trans_conn, int timeout);
extern XtransConnInfo	_fs_connect(char *servername, int *ret);

/* check for both EAGAIN and EWOULDBLOCK, because some supposedly POSIX
 * systems are broken and return EWOULDBLOCK when they should return EAGAIN
 */
#ifdef WIN32
#define ETEST() (WSAGetLastError() == WSAEWOULDBLOCK)
#else
#if defined(EAGAIN) && defined(EWOULDBLOCK)
#define ETEST() (errno == EAGAIN || errno == EWOULDBLOCK)
#else
#ifdef EAGAIN
#define ETEST() (errno == EAGAIN)
#else
#define ETEST() (errno == EWOULDBLOCK)
#endif
#endif
#endif
#ifdef WIN32
#define ECHECK(err) (WSAGetLastError() == err)
#define ESET(val) WSASetLastError(val)
#else
#ifdef ISC
#define ECHECK(err) ((errno == err) || ETEST())
#else
#define ECHECK(err) (errno == err)
#endif
#define ESET(val) errno = val
#endif

#endif				/* _FSIO_H_ */
