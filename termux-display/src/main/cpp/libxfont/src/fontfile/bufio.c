/*

Copyright 1991, 1998  The Open Group

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

/*
 * Author:  Keith Packard, MIT X Consortium
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include <X11/Xos.h>
#include <X11/fonts/fontmisc.h>
#include <X11/fonts/bufio.h>
#include <errno.h>

BufFilePtr
BufFileCreate (char *private,
	       int (*input)(BufFilePtr),
	       int (*output)(int, BufFilePtr),
	       int (*skip)(BufFilePtr, int),
	       int (*close)(BufFilePtr, int))
{
    BufFilePtr	f;

    f = malloc (sizeof *f);
    if (!f)
	return 0;
    f->private = private;
    f->bufp = f->buffer;
    f->left = 0;
    f->input = input;
    f->output = output;
    f->skip = skip;
    f->eof  = 0;
    f->close = close;
    return f;
}

#define FileDes(f)  ((int)(long) (f)->private)

static int
BufFileRawFill (BufFilePtr f)
{
    int	left;

    left = read (FileDes(f), (char *)f->buffer, BUFFILESIZE);
    if (left <= 0) {
	f->left = 0;
	return BUFFILEEOF;
    }
    f->left = left - 1;
    f->bufp = f->buffer + 1;
    return f->buffer[0];
}

static int
BufFileRawSkip (BufFilePtr f, int count)
{
    int	    curoff;
    int	    fileoff;
    int	    todo;

    curoff = f->bufp - f->buffer;
    fileoff = curoff + f->left;
    if (curoff + count <= fileoff) {
	f->bufp += count;
	f->left -= count;
    } else {
	todo = count - (fileoff - curoff);
	if (lseek (FileDes(f), todo, 1) == -1) {
	    if (errno != ESPIPE)
		return BUFFILEEOF;
	    while (todo) {
		curoff = BUFFILESIZE;
		if (curoff > todo)
		    curoff = todo;
		fileoff = read (FileDes(f), (char *)f->buffer, curoff);
		if (fileoff <= 0)
		    return BUFFILEEOF;
		todo -= fileoff;
	    }
	}
	f->left = 0;
    }
    return count;
}

static int
BufFileRawClose (BufFilePtr f, int doClose)
{
    if (doClose)
	close (FileDes (f));
    return 1;
}

BufFilePtr
BufFileOpenRead (int fd)
{
#if defined (WIN32)
    /* hv: I'd bet WIN32 has the same effect here */
    setmode(fd,O_BINARY);
#endif
    return BufFileCreate ((char *)(long) fd, BufFileRawFill, 0, BufFileRawSkip, BufFileRawClose);
}

static int
BufFileRawFlush (int c, BufFilePtr f)
{
    int	cnt;

    if (c != BUFFILEEOF)
	*f->bufp++ = c;
    cnt = f->bufp - f->buffer;
    f->bufp = f->buffer;
    f->left = BUFFILESIZE;
    if (write (FileDes(f), (char *)f->buffer, cnt) != cnt)
	return BUFFILEEOF;
    return c;
}

static int
BufFileFlush (BufFilePtr f, int doClose)
{
    if (f->bufp != f->buffer)
	return (*f->output) (BUFFILEEOF, f);
    return 0;
}

BufFilePtr
BufFileOpenWrite (int fd)
{
    BufFilePtr	f;

#if defined(WIN32)
    /* hv: I'd bet WIN32 has the same effect here */
    setmode(fd,O_BINARY);
#endif
    f = BufFileCreate ((char *)(long) fd, 0, BufFileRawFlush, 0, BufFileFlush);
    if (f != NULL) {
	f->bufp = f->buffer;
	f->left = BUFFILESIZE;
    }
    return f;
}

int
BufFileRead (BufFilePtr f, char *b, int n)
{
    int	    c, cnt;
    cnt = n;
    while (cnt--) {
	c = BufFileGet (f);
	if (c == BUFFILEEOF)
	    break;
	*b++ = c;
    }
    return n - cnt - 1;
}

int
BufFileWrite (BufFilePtr f, const char *b, int n)
{
    int	    cnt;
    cnt = n;
    while (cnt--) {
	if (BufFilePut (*b++, f) == BUFFILEEOF)
	    return BUFFILEEOF;
    }
    return n;
}

int
BufFileClose (BufFilePtr f, int doClose)
{
    int ret;
    ret = (*f->close) (f, doClose);
    free (f);
    return ret;
}
