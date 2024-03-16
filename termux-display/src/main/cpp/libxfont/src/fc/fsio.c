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
/*
 * font server i/o routines
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"

#ifdef WIN32
#define _WILLWINSOCK_
#include	"X11/Xwindows.h"
#endif

#define FONT_t
#define TRANS_CLIENT
#include 	"X11/Xtrans/Xtrans.h"
#include	"X11/Xpoll.h"
#include	<X11/fonts/FS.h>
#include	<X11/fonts/FSproto.h>
#include	<X11/fonts/fontmisc.h>
#include	<X11/fonts/fontstruct.h>
#include	"fservestr.h"

#include	<stdio.h>
#include	<signal.h>
#include	<sys/types.h>
#if !defined(WIN32)
#ifndef Lynx
#include	<sys/socket.h>
#else
#include	<socket.h>
#endif
#endif
#include	<errno.h>
#ifdef WIN32
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EINTR
#define EINTR WSAEINTR
#endif



static int  padlength[4] = {0, 3, 2, 1};

static int
_fs_resize (FSBufPtr buf, long size);

static void
_fs_downsize (FSBufPtr buf, long size);

int
_fs_poll_connect (XtransConnInfo trans_conn, int timeout)
{
    fd_set	    w_mask;
    struct timeval  tv;
    int		    fs_fd = _FontTransGetConnectionNumber (trans_conn);
    int		    ret;

    do
    {
	tv.tv_usec = 0;
	tv.tv_sec = timeout;
	FD_ZERO (&w_mask);
	FD_SET (fs_fd, &w_mask);
	ret = Select (fs_fd + 1, NULL, &w_mask, NULL, &tv);
    } while (ret < 0 && ECHECK(EINTR));
    if (ret == 0)
	return FSIO_BLOCK;
    if (ret < 0)
	return FSIO_ERROR;
    return FSIO_READY;
}

XtransConnInfo
_fs_connect(char *servername, int *err)
{
    XtransConnInfo  trans_conn;		/* transport connection object */
    int		    ret;
    int		    i = 0;
    int		    retries = 5;

    /*
     * Open the network connection.
     */
    if( (trans_conn=_FontTransOpenCOTSClient(servername)) == NULL )
    {
	*err = FSIO_ERROR;
	return 0;
    }

    /*
     * Set the connection non-blocking since we use select() to block.
     */

    _FontTransSetOption(trans_conn, TRANS_NONBLOCKING, 1);

    do {
	i = _FontTransConnect(trans_conn,servername);
    } while ((i == TRANS_TRY_CONNECT_AGAIN) && (retries-- > 0));

    if (i < 0)
    {
	if (i == TRANS_IN_PROGRESS)
	    ret = FSIO_BLOCK;
	else
	    ret = FSIO_ERROR;
    }
    else
	ret = FSIO_READY;

    if (ret == FSIO_ERROR)
    {
	_FontTransClose(trans_conn);
	trans_conn = 0;
    }

    *err = ret;
    return trans_conn;
}

static int
_fs_fill (FSFpePtr conn)
{
    long    avail;
    long    bytes_read;
    Bool    waited = FALSE;

    if (_fs_flush (conn) < 0)
	return FSIO_ERROR;
    /*
     * Don't go overboard here; stop reading when we've
     * got enough to satisfy the pending request
     */
    while ((conn->inNeed - (conn->inBuf.insert - conn->inBuf.remove)) > 0)
    {
	avail = conn->inBuf.size - conn->inBuf.insert;
	/*
	 * For SVR4 with a unix-domain connection, ETEST() after selecting
	 * readable means the server has died.  To do this here, we look for
	 * two consecutive reads returning ETEST().
	 */
	ESET (0);
	bytes_read =_FontTransRead(conn->trans_conn,
				   conn->inBuf.buf + conn->inBuf.insert,
				   avail);
	if (bytes_read > 0) {
	    conn->inBuf.insert += bytes_read;
	    waited = FALSE;
	}
	else
	{
	    if (bytes_read == 0 || ETEST ())
	    {
		if (!waited)
		{
		    waited = TRUE;
		    if (_fs_wait_for_readable (conn, 0) == FSIO_BLOCK)
			return FSIO_BLOCK;
		    continue;
		}
	    }
	    if (!ECHECK(EINTR))
	    {
	        _fs_connection_died (conn);
	        return FSIO_ERROR;
	    }
	}
    }
    return FSIO_READY;
}

/*
 * Make space and return whether data have already arrived
 */

int
_fs_start_read (FSFpePtr conn, long size, char **buf)
{
    int	    ret;

    conn->inNeed = size;
    if (fs_inqueued(conn) < size)
    {
	if (_fs_resize (&conn->inBuf, size) != FSIO_READY)
	{
	    _fs_connection_died (conn);
	    return FSIO_ERROR;
	}
	ret = _fs_fill (conn);
	if (ret == FSIO_ERROR)
	    return ret;
	if (ret == FSIO_BLOCK || fs_inqueued(conn) < size)
	    return FSIO_BLOCK;
    }
    if (buf)
	*buf = conn->inBuf.buf + conn->inBuf.remove;
    return FSIO_READY;
}

void
_fs_done_read (FSFpePtr conn, long size)
{
    if (conn->inBuf.insert - conn->inBuf.remove < size)
    {
#ifdef DEBUG
	fprintf (stderr, "_fs_done_read skipping to many bytes\n");
#endif
	return;
    }
    conn->inBuf.remove += size;
    conn->inNeed -= size;
    _fs_downsize (&conn->inBuf, FS_BUF_MAX);
}

long
_fs_pad_length (long len)
{
    return len + padlength[len&3];
}

int
_fs_flush (FSFpePtr conn)
{
    long    bytes_written;
    long    remain;

    /* XXX - hack.  The right fix is to remember that the font server
       has gone away when we first discovered it. */
    if (conn->fs_fd < 0)
	return FSIO_ERROR;

    while ((remain = conn->outBuf.insert - conn->outBuf.remove) > 0)
    {
	bytes_written = _FontTransWrite(conn->trans_conn,
					conn->outBuf.buf + conn->outBuf.remove,
					(int) remain);
	if (bytes_written > 0)
	{
	    conn->outBuf.remove += bytes_written;
	}
	else
	{
	    if (bytes_written == 0 || ETEST ())
	    {
		conn->brokenWriteTime = GetTimeInMillis () + FS_FLUSH_POLL;
		_fs_mark_block (conn, FS_BROKEN_WRITE);
		break;
	    }
	    if (!ECHECK (EINTR))
	    {
		_fs_connection_died (conn);
		return FSIO_ERROR;
	    }
	}
    }
    if (conn->outBuf.remove == conn->outBuf.insert)
    {
	_fs_unmark_block (conn, FS_BROKEN_WRITE|FS_PENDING_WRITE);
	if (conn->outBuf.size > FS_BUF_INC)
	    conn->outBuf.buf = realloc (conn->outBuf.buf, FS_BUF_INC);
	conn->outBuf.remove = conn->outBuf.insert = 0;
    }
    return FSIO_READY;
}

static int
_fs_resize (FSBufPtr buf, long size)
{
    char    *new;
    long    new_size;

    if (buf->remove)
    {
	if (buf->remove != buf->insert)
	{
	    memmove (buf->buf,
		     buf->buf + buf->remove,
		     buf->insert - buf->remove);
	}
	buf->insert -= buf->remove;
	buf->remove = 0;
    }
    if (buf->size - buf->remove < size)
    {
	new_size = ((buf->remove + size + FS_BUF_INC) / FS_BUF_INC) * FS_BUF_INC;
	new = realloc (buf->buf, new_size);
	if (!new)
	    return FSIO_ERROR;
	buf->buf = new;
	buf->size = new_size;
    }
    return FSIO_READY;
}

static void
_fs_downsize (FSBufPtr buf, long size)
{
    if (buf->insert == buf->remove)
    {
	buf->insert = buf->remove = 0;
	if (buf->size > size)
	{
	    buf->buf = realloc (buf->buf, size);
	    buf->size = size;
	}
    }
}

void
_fs_io_reinit (FSFpePtr conn)
{
    conn->outBuf.insert = conn->outBuf.remove = 0;
    _fs_downsize (&conn->outBuf, FS_BUF_INC);
    conn->inBuf.insert = conn->inBuf.remove = 0;
    _fs_downsize (&conn->inBuf, FS_BUF_MAX);
}

Bool
_fs_io_init (FSFpePtr conn)
{
    conn->outBuf.insert = conn->outBuf.remove = 0;
    conn->outBuf.buf = malloc (FS_BUF_INC);
    if (!conn->outBuf.buf)
	return FALSE;
    conn->outBuf.size = FS_BUF_INC;

    conn->inBuf.insert = conn->inBuf.remove = 0;
    conn->inBuf.buf = malloc (FS_BUF_INC);
    if (!conn->inBuf.buf)
    {
	free (conn->outBuf.buf);
	conn->outBuf.buf = 0;
	return FALSE;
    }
    conn->inBuf.size = FS_BUF_INC;

    return TRUE;
}

void
_fs_io_fini (FSFpePtr conn)
{
    if (conn->outBuf.buf)
	free (conn->outBuf.buf);
    if (conn->inBuf.buf)
	free (conn->inBuf.buf);
}

static int
_fs_do_write(FSFpePtr conn, const char *data, long len, long size)
{
    if (size == 0) {
#ifdef DEBUG
	fprintf(stderr, "tried to write 0 bytes \n");
#endif
	return FSIO_READY;
    }

    if (conn->fs_fd == -1)
	return FSIO_ERROR;

    while (conn->outBuf.insert + size > conn->outBuf.size)
    {
	if (_fs_flush (conn) < 0)
	    return FSIO_ERROR;
	if (_fs_resize (&conn->outBuf, size) < 0)
	{
	    _fs_connection_died (conn);
	    return FSIO_ERROR;
	}
    }
    memcpy (conn->outBuf.buf + conn->outBuf.insert, data, len);
    /* Clear pad data */
    memset (conn->outBuf.buf + conn->outBuf.insert + len, 0, size - len);
    conn->outBuf.insert += size;
    _fs_mark_block (conn, FS_PENDING_WRITE);
    return FSIO_READY;
}

/*
 * Write the indicated bytes
 */
int
_fs_write (FSFpePtr conn, const char *data, long len)
{
    return _fs_do_write (conn, data, len, len);
}

/*
 * Write the indicated bytes adding any appropriate pad
 */
int
_fs_write_pad(FSFpePtr conn, const char *data, long len)
{
    return _fs_do_write (conn, data, len, len + padlength[len & 3]);
}

int
_fs_wait_for_readable(FSFpePtr conn, int ms)
{
    fd_set	r_mask;
    fd_set	e_mask;
    int         result;
    struct timeval  tv;

    for (;;) {
	if (conn->fs_fd < 0)
	    return FSIO_ERROR;
	FD_ZERO(&r_mask);
	FD_ZERO(&e_mask);
	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
	FD_SET(conn->fs_fd, &r_mask);
	FD_SET(conn->fs_fd, &e_mask);
	result = Select(conn->fs_fd + 1, &r_mask, NULL, &e_mask, &tv);
	if (result < 0)
	{
	    if (ECHECK(EINTR) || ECHECK(EAGAIN))
		continue;
	    else
		return FSIO_ERROR;
	}
	if (result == 0)
	    return FSIO_BLOCK;
	if (FD_ISSET(conn->fs_fd, &r_mask))
	    return FSIO_READY;
	return FSIO_ERROR;
    }
}
