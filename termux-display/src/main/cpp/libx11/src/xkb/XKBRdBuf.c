/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "Xlibint.h"
#include "XKBlibint.h"
#include <X11/extensions/XKBproto.h>

/***====================================================================***/

int
_XkbInitReadBuffer(Display *dpy, XkbReadBufferPtr buf, int size)
{
    if ((dpy != NULL) && (buf != NULL) && (size > 0)) {
        buf->error = 0;
        buf->size = size;
        buf->start = buf->data = _XkbAlloc(size);
        if (buf->start) {
            _XRead(dpy, buf->start, size);
            return 1;
        }
    }
    return 0;
}

#define	_XkbReadBufferDataLeft(b)	(((b)->size)-((b)->data-(b)->start))

int
_XkbSkipReadBufferData(XkbReadBufferPtr from, int size)
{
    if (size == 0)
        return 1;
    if ((from == NULL) || (from->error) || (size < 1) ||
        (_XkbReadBufferDataLeft(from) < size))
        return 0;
    from->data += size;
    return 1;
}

int
_XkbCopyFromReadBuffer(XkbReadBufferPtr from, char *to, int size)
{
    if (size == 0)
        return 1;
    if ((from == NULL) || (from->error) || (to == NULL) || (size < 1) ||
        (_XkbReadBufferDataLeft(from) < size))
        return 0;
    memcpy(to, from->data, (size_t)size);
    from->data += size;
    return 1;
}

#ifdef XKB_FORCE_INT_KEYSYM
int
_XkbReadCopyKeySyms(int *wire, KeySym * to, int num_words)
{
    while (num_words-- > 0) {
        *to++ = *wire++;
    }
    return 1;
}

int
_XkbReadBufferCopyKeySyms(XkbReadBufferPtr from, KeySym * to, int num_words)
{
    if ((unsigned) (num_words * 4) > _XkbReadBufferDataLeft(from))
        return 0;
    _XkbReadCopyKeySyms((int *) from->data, to, num_words);
    from->data += (4 * num_words);
    return True;
}

int
_XkbWriteCopyKeySyms(register KeySym * from, CARD32 *to, int len)
{
    while (len-- > 0) {
        *to++ = (CARD32) *from++;
    }
    return True;
}
#endif

#ifdef LONG64
int
_XkbReadCopyData32(int *wire, long *to, int num_words)
{
    while (num_words-- > 0) {
        *to++ = *wire++;
    }
    return 1;
}

int
_XkbReadBufferCopy32(XkbReadBufferPtr from, long *to, int num_words)
{
    if ((unsigned) (num_words * 4) > _XkbReadBufferDataLeft(from))
        return 0;
    _XkbReadCopyData32((int *) from->data, to, num_words);
    from->data += (4 * num_words);
    return True;
}

int
_XkbWriteCopyData32(register unsigned long *from, CARD32 *to, int len)
{
    while (len-- > 0) {
        *to++ = (CARD32) *from++;
    }
    return True;
}
#endif                          /* LONG64 */


char *
_XkbPeekAtReadBuffer(XkbReadBufferPtr from, int size)
{
    if ((from == NULL) || (from->error) || (size < 1) ||
        (_XkbReadBufferDataLeft(from) < size))
        return NULL;
    return from->data;
}

char *
_XkbGetReadBufferPtr(XkbReadBufferPtr from, int size)
{
    char *ptr;

    if ((from == NULL) || (from->error) || (size < 1) ||
        (_XkbReadBufferDataLeft(from) < size))
        return NULL;
    ptr = from->data;
    from->data += size;
    return ptr;
}


int
_XkbFreeReadBuffer(XkbReadBufferPtr buf)
{
    if ((buf != NULL) && (buf->start != NULL)) {
        int left;

        left = (int) _XkbReadBufferDataLeft(buf);

        Xfree(buf->start);
        buf->size = 0;
        buf->start = buf->data = NULL;
        return left;
    }
    return 0;
}

Bool
_XkbGetReadBufferCountedString(XkbReadBufferPtr buf, char **rtrn)
{
    CARD16 len, *pLen;
    int left;
    char *str = NULL;

    if ((buf == NULL) || (buf->error) ||
        ((left = (int) _XkbReadBufferDataLeft(buf)) < 4))
        return False;
    pLen = (CARD16 *) buf->data;
    len = *pLen;
    if (len > 0) {
        if (XkbPaddedSize(len + 2) > left)
            return False;
        str = _XkbAlloc(len + 1);
        if (str) {
            memcpy(str, &buf->data[2], len);
            str[len] = '\0';
        }
    }
    buf->data += XkbPaddedSize(len + 2);
    *rtrn = str;
    return True;
}
