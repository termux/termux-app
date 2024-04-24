/*
 * Copyright 1999 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include <string.h>
#include "builtin.h"

typedef struct _BuiltinIO {
    int		    offset;
    BuiltinFilePtr  file;
} BuiltinIORec, *BuiltinIOPtr;

static int
BuiltinFill (BufFilePtr	f)
{
    int	    left, len;
    BuiltinIOPtr    io = ((BuiltinIOPtr) f->private);

    left = io->file->len - io->offset;
    if (left <= 0)
    {
	f->left = 0;
	return BUFFILEEOF;
    }
    len = BUFFILESIZE;
    if (len > left)
	len = left;
    memcpy (f->buffer, io->file->bits + io->offset, len);
    io->offset += len;
    f->left = len - 1;
    f->bufp = f->buffer + 1;
    return f->buffer[0];
}

static int
BuiltinSkip (BufFilePtr	f, int count)
{
    BuiltinIOPtr    io = ((BuiltinIOPtr) f->private);
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
	io->offset += todo;
	if (io->offset > io->file->len)
	    io->offset = io->file->len;
	if (io->offset < 0)
	    io->offset = 0;
	f->left = 0;
    }
    return count;
}

static int
BuiltinClose (BufFilePtr f, int unused)
{
    BuiltinIOPtr    io = ((BuiltinIOPtr) f->private);

    free (io);
    return 1;
}


FontFilePtr
BuiltinFileOpen (const char *name)
{
    int		    i;
    BuiltinIOPtr    io;
    BufFilePtr	    raw, cooked;

    if (*name == '/') name++;
    for (i = 0; i < builtin_files_count; i++)
	if (!strcmp (name, builtin_files[i].name))
	    break;
    if (i == builtin_files_count)
	return NULL;
    io = malloc (sizeof (BuiltinIORec));
    if (!io)
	return NULL;
    io->offset = 0;
    io->file = (void *) &builtin_files[i];
    raw = BufFileCreate ((char *) io, BuiltinFill, 0, BuiltinSkip, BuiltinClose);
    if (!raw)
    {
	free (io);
	return NULL;
    }
    if ((cooked = BufFilePushZIP (raw)))
	raw = cooked;
    else
    {
	raw->left += raw->bufp - raw->buffer;
	raw->bufp = raw->buffer;
    }
    return (FontFilePtr) raw;
}

int
BuiltinFileClose (FontFilePtr f, int unused)
{
    return BufFileClose ((BufFilePtr) f, TRUE);
}
