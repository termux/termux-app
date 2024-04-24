/*

Copyright 1986, 1998  The Open Group

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"

int
XDrawText(
    register Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    XTextItem *items,
    int nitems)
{
    register int i;
    register XTextItem *item;
    int length = 0;
    register xPolyText8Req *req;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq (PolyText8, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->x = x;
    req->y = y;

    item = items;
    for (i=0; i < nitems; i++) {
	if (item->font)
	    length += 5;  /* a 255 byte, plus size of Font id */
        if (item->delta)
        {
	    if (item->delta > 0)
	    {
	      length += SIZEOF(xTextElt) * ((item->delta + 126) / 127);
	    }
            else
            {
   	      length += SIZEOF(xTextElt) * ((-item->delta + 127) / 128);
 	    }
        }
	if (item->nchars > 0)
	{
	    length += SIZEOF(xTextElt) * ((item->nchars + 253) / 254 - 1);
	    if (!item->delta) length += SIZEOF(xTextElt);
	    length += item->nchars;
     	}
	item++;
    }

    req->length += (length + 3)>>2;  /* convert to number of 32-bit words */


    /*
     * If the entire request does not fit into the remaining space in the
     * buffer, flush the buffer first.   If the request does fit into the
     * empty buffer, then we won't have to flush it at the end to keep
     * the buffer 32-bit aligned.
     */

    if (dpy->bufptr + length > dpy->bufmax)
    	_XFlush (dpy);

    item = items;
    for (i=0; i< nitems; i++) {

	if (item->font) {
            /* to mark a font shift, write a 255 byte followed by
	       the 4 bytes of font ID, big-end first */
	    register unsigned char *f;
	    BufAlloc (unsigned char *, f, 5);

	    f[0] = 255;
	    f[1] = (item->font & 0xff000000) >> 24;
	    f[2] = (item->font & 0x00ff0000) >> 16;
	    f[3] = (item->font & 0x0000ff00) >> 8;
	    f[4] =  item->font & 0x000000ff;

            /* update GC shadow */
	    gc->values.font = item->font;
	    }

	{
	    int nbytes = SIZEOF(xTextElt);
	    int PartialNChars = item->nchars;
	    int PartialDelta = item->delta;
            /* register xTextElt *elt; */
	    int FirstTimeThrough = True;
 	    char *CharacterOffset = item->chars;
            char *tbuf = NULL;

	    while((PartialDelta < -128) || (PartialDelta > 127))
            {
	    	int nb = SIZEOF(xTextElt);

	    	BufAlloc (char *, tbuf, nb);
	    	*tbuf = 0;    /*   elt->len  */
	    	if (PartialDelta > 0 )
		{
		    *(tbuf+1) = 127;  /* elt->delta  */
		    PartialDelta = PartialDelta - 127;
		}
		else
		{
		    /* -128 = 0x8, need to be careful of signed chars... */
		    *((unsigned char *)(tbuf+1)) = 0x80;  /* elt->delta */
		    PartialDelta = PartialDelta + 128;
		}
	    }
	    if (PartialDelta)
            {
                BufAlloc (char *, tbuf , nbytes);
	        *tbuf = 0;      /* elt->len */
		*(tbuf+1) = PartialDelta;    /* elt->delta  */
	    }
	    while(PartialNChars > 254)
            {
		nbytes = 254;
	    	if (FirstTimeThrough)
		{
		    FirstTimeThrough = False;
		    if (!item->delta)
 		    {
			nbytes += SIZEOF(xTextElt);
	   		BufAlloc (char *, tbuf, nbytes);
		        *(tbuf+1) = 0;     /* elt->delta */
		    }
		    else
		    {
			char *DummyChar;
		        BufAlloc(char *, DummyChar, nbytes);
		    }
		}
		else
		{
 		    nbytes += SIZEOF(xTextElt);
	   	    BufAlloc (char *, tbuf, nbytes);
		    *(tbuf+1) = 0;   /* elt->delta */
		}
		/* watch out for signs on chars */
		*(unsigned char *)tbuf = 254;  /* elt->len */
                memcpy (tbuf+2 , CharacterOffset, 254);
		PartialNChars = PartialNChars - 254;
		CharacterOffset += 254;

	    }
	    if (PartialNChars)
            {
		nbytes = PartialNChars;
	    	if (FirstTimeThrough)
		{
		    FirstTimeThrough = False;
		    if (!item->delta)
 		    {
			nbytes += SIZEOF(xTextElt);
	   		BufAlloc (char *, tbuf, nbytes);
			*(tbuf+1) = 0;   /*  elt->delta  */
		    }
		    else
		    {
			char *DummyChar;
		        BufAlloc(char *, DummyChar, nbytes);
		    }
		}
		else
		{
 		    nbytes += SIZEOF(xTextElt);
	   	    BufAlloc (char *, tbuf, nbytes);
		    *(tbuf+1) = 0;   /* elt->delta  */
		}
	    	*tbuf = PartialNChars;   /*  elt->len  */
                memcpy (tbuf+2 , CharacterOffset, (size_t) PartialNChars);
	    }
	}
    item++;
    }

    /* Pad request out to a 32-bit boundary */

    if (length &= 3) {
	char *pad;
	/*
	 * BufAlloc is a macro that uses its last argument more than
	 * once, otherwise I'd write "BufAlloc (char *, pad, 4-length)"
	 */
	length = 4 - length;
	BufAlloc (char *, pad, length);
	/*
	 * if there are 3 bytes of padding, the first byte MUST be 0
	 * so the pad bytes aren't mistaken for a final xTextElt
	 */
	*pad = 0;
        }

    /*
     * If the buffer pointer is not now pointing to a 32-bit boundary,
     * we must flush the buffer so that it does point to a 32-bit boundary
     * at the end of this routine.
     */

    if ((dpy->bufptr - dpy->buffer) & 3)
       _XFlush (dpy);
    UnlockDisplay(dpy);
    SyncHandle();
    return 0;
    }
