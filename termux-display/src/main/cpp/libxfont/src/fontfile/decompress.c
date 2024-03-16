/*
 * Copyright 1985, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods, derived from original work by Spencer Thomas
 * and Joseph Orost.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*

Copyright 1993, 1998  The Open Group

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
 * decompress - cat a compressed file
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include <X11/fonts/fontmisc.h>
#include <X11/fonts/bufio.h>

#define BITS	16

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
#if BITS > 15
typedef long int	code_int;
#else
typedef int		code_int;
#endif

typedef long int	  count_int;

#ifdef NO_UCHAR
 typedef char	char_type;
#else
 typedef	unsigned char	char_type;
#endif /* UCHAR */

static char_type magic_header[] = { "\037\235" };	/* 1F 9D */

/* Defines for third byte of header */
#define BIT_MASK	0x1f
#define BLOCK_MASK	0x80
/* Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
   a fourth header byte (for expansion).
*/

#define INIT_BITS 9			/* initial number of bits/code */

#ifdef COMPATIBLE		/* But wrong! */
# define MAXCODE(n_bits)	(1 << (n_bits) - 1)
#else
# define MAXCODE(n_bits)	((1 << (n_bits)) - 1)
#endif /* COMPATIBLE */

/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */
#define FIRST	257	/* first free entry */
#define	CLEAR	256	/* table clear output code */

#define STACK_SIZE  65300

typedef struct _compressedFILE {
    BufFilePtr	    file;

    char_type	    *stackp;
    code_int	    oldcode;
    char_type	    finchar;

    int		block_compress;
    int		maxbits;
    code_int	maxcode, maxmaxcode;

    code_int	free_ent;
    int		clear_flg;
    int		n_bits;

    /* bit buffer */
    int		offset, size;
    char_type	buf[BITS];

    char_type	    de_stack[STACK_SIZE];
    char_type	    *tab_suffix;
    unsigned short  *tab_prefix;
} CompressedFile;


static int BufCompressedClose ( BufFilePtr f, int doClose );
static int BufCompressedFill ( BufFilePtr f );
static code_int getcode ( CompressedFile *file );
static int BufCompressedSkip ( BufFilePtr f, int bytes );

BufFilePtr
BufFilePushCompressed (BufFilePtr f)
{
    int		    code;
    int		    maxbits;
    CompressedFile  *file;
    int		    extra;

    if ((BufFileGet(f) != (magic_header[0] & 0xFF)) ||
	(BufFileGet(f) != (magic_header[1] & 0xFF)))
    {
	return 0;
    }
    code = BufFileGet (f);
    if (code == BUFFILEEOF) return 0;

    maxbits = code & BIT_MASK;
    if (maxbits > BITS || maxbits <= INIT_BITS)
	return 0;
    extra = (1 << maxbits) * sizeof (char_type) +
	    (1 << maxbits) * sizeof (unsigned short);
    file = malloc (sizeof (CompressedFile) + extra);
    if (!file)
	return 0;
    file->file = f;
    file->maxbits = maxbits;
    file->block_compress = code & BLOCK_MASK;
    file->maxmaxcode = 1 << file->maxbits;
    file->tab_suffix = (char_type *) &file[1];
    file->tab_prefix = (unsigned short *) (file->tab_suffix + file->maxmaxcode);
    /*
     * As above, initialize the first 256 entries in the table.
     */
    file->maxcode = MAXCODE(file->n_bits = INIT_BITS);
    for ( code = 255; code >= 0; code-- ) {
	file->tab_prefix[code] = 0;
	file->tab_suffix[code] = (char_type) code;
    }
    file->free_ent = ((file->block_compress) ? FIRST : 256 );
    file->oldcode = -1;
    file->clear_flg = 0;
    file->offset = 0;
    file->size = 0;
    file->stackp = file->de_stack;
    bzero(file->buf, BITS);
    return BufFileCreate ((char *) file,
			  BufCompressedFill,
			  0,
			  BufCompressedSkip,
			  BufCompressedClose);
}

static int
BufCompressedClose (BufFilePtr f, int doClose)
{
    CompressedFile  *file;
    BufFilePtr	    raw;

    file = (CompressedFile *) f->private;
    raw = file->file;
    free (file);
    BufFileClose (raw, doClose);
    return 1;
}

static int
BufCompressedFill (BufFilePtr f)
{
    CompressedFile  *file;
    register char_type *stackp, *de_stack;
    register char_type finchar;
    register code_int code, oldcode, incode;
    BufChar	    *buf, *bufend;

    file = (CompressedFile *) f->private;

    buf = f->buffer;
    bufend = buf + BUFFILESIZE;
    stackp = file->stackp;
    de_stack = file->de_stack;
    finchar = file->finchar;
    oldcode = file->oldcode;
    while (buf < bufend) {
	while (stackp > de_stack && buf < bufend)
	    *buf++ = *--stackp;

	if (buf == bufend)
	    break;

	code = getcode (file);
	if (code == -1)
	    break;

    	if ( (code == CLEAR) && file->block_compress ) {
	    for ( code = 255; code >= 0; code-- )
	    	file->tab_prefix[code] = 0;
	    file->clear_flg = 1;
	    file->free_ent = FIRST;
	    oldcode = -1;
	    continue;
    	}
    	incode = code;
    	/*
     	 * Special case for KwKwK string.
     	 */
    	if ( code >= file->free_ent ) {
	    if ( code > file->free_ent || oldcode == -1 ) {
		/* Bad stream. */
		return BUFFILEEOF;
	    }
	    *stackp++ = finchar;
	    code = oldcode;
    	}
	/*
	 * The above condition ensures that code < free_ent.
	 * The construction of tab_prefixof in turn guarantees that
	 * each iteration decreases code and therefore stack usage is
	 * bound by 1 << BITS - 256.
	 */

    	/*
     	 * Generate output characters in reverse order
     	 */
    	while ( code >= 256 )
    	{
	    *stackp++ = file->tab_suffix[code];
	    code = file->tab_prefix[code];
    	}
	finchar = file->tab_suffix[code];
	*stackp++ = finchar;

    	/*
     	 * Generate the new entry.
     	 */
    	if ( (code=file->free_ent) < file->maxmaxcode && oldcode != -1) {
	    file->tab_prefix[code] = (unsigned short)oldcode;
	    file->tab_suffix[code] = finchar;
	    file->free_ent = code+1;
    	}
	/*
	 * Remember previous code.
	 */
	oldcode = incode;
    }
    file->oldcode = oldcode;
    file->stackp = stackp;
    file->finchar = finchar;
    if (buf == f->buffer) {
	f->left = 0;
	return BUFFILEEOF;
    }
    f->bufp = f->buffer + 1;
    f->left = (buf - f->buffer) - 1;
    return f->buffer[0];
}

/*****************************************************************
 * TAG( getcode )
 *
 * Read one code from the standard input.  If BUFFILEEOF, return -1.
 * Inputs:
 * 	stdin
 * Outputs:
 * 	code or -1 is returned.
 */

static char_type rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

static code_int
getcode(CompressedFile *file)
{
    register code_int code;
    register int r_off, bits;
    register char_type *bp = file->buf;
    register BufFilePtr	raw;

    if ( file->clear_flg > 0 || file->offset >= file->size ||
	file->free_ent > file->maxcode )
    {
	/*
	 * If the next entry will be too big for the current code
	 * size, then we must increase the size.  This implies reading
	 * a new buffer full, too.
	 */
	if ( file->free_ent > file->maxcode ) {
	    file->n_bits++;
	    if ( file->n_bits == file->maxbits )
		file->maxcode = file->maxmaxcode;	/* won't get any bigger now */
	    else
		file->maxcode = MAXCODE(file->n_bits);
	}
	if ( file->clear_flg > 0) {
    	    file->maxcode = MAXCODE (file->n_bits = INIT_BITS);
	    file->clear_flg = 0;
	}
	bits = file->n_bits;
	raw = file->file;
	while (bits > 0 && (code = BufFileGet (raw)) != BUFFILEEOF)
	{
	    *bp++ = code;
	    --bits;
	}
	bp = file->buf;
	if (bits == file->n_bits)
	    return -1;			/* end of file */
	file->size = file->n_bits - bits;
	file->offset = 0;
	/* Round size down to integral number of codes */
	file->size = (file->size << 3) - (file->n_bits - 1);
    }
    r_off = file->offset;
    bits = file->n_bits;
    /*
     * Get to the first byte.
     */
    bp += (r_off >> 3);
    r_off &= 7;
    /* Get first part (low order bits) */
#ifdef NO_UCHAR
    code = ((*bp++ >> r_off) & rmask[8 - r_off]) & 0xff;
#else
    code = (*bp++ >> r_off);
#endif /* NO_UCHAR */
    bits -= (8 - r_off);
    r_off = 8 - r_off;		/* now, offset into code word */
    /* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
    if ( bits >= 8 ) {
#ifdef NO_UCHAR
	code |= (*bp++ & 0xff) << r_off;
#else
	code |= *bp++ << r_off;
#endif /* NO_UCHAR */
	r_off += 8;
	bits -= 8;
    }
    /* high order bits. */
    code |= (*bp & rmask[bits]) << r_off;
    file->offset += file->n_bits;

    return code;
}

static int
BufCompressedSkip (BufFilePtr f, int bytes)
{
    int		    c;
    while (bytes--)
    {
	c = BufFileGet(f);
	if (c == BUFFILEEOF)
	    return BUFFILEEOF;
    }
    return 0;
}

#ifdef TEST
int
main (int argc, char *argv[])
{
    BufFilePtr	    inputraw, input, output;
    int		    c;

    inputraw = BufFileOpenRead (0);
    input = BufFilePushCompressed (inputraw);
    output = BufFileOpenWrite (1);
    while ((c = BufFileGet (input)) != BUFFILEEOF)
	BufFilePut (c, output);
    BufFileClose (input, FALSE);
    BufFileClose (output, FALSE);
    return 0;
}
#endif
