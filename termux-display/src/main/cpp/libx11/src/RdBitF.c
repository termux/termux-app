/*

Copyright 1987, 1998  The Open Group

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
 *	Code to read bitmaps from disk files. Interprets
 *	data from X10 and X11 bitmap files and creates
 *	Pixmap representations of files. Returns Pixmap
 *	ID and specifics about image.
 *
 *	Modified for speedup by Jim Becker, changed image
 *	data parsing logic (removed some fscanf()s).
 *	Aug 5, 1988
 *
 * Note that this file and ../Xmu/RdBitF.c look very similar....  Keep them
 * that way (but don't use common source code so that people can have one
 * without the other).
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include <X11/Xos.h>
#include "Xutil.h"
#include <stdio.h>
#include <ctype.h>
#include "reallocarray.h"

#define MAX_SIZE 255

/* shared data for the image read/parse logic */
static const short hexTable[256] = {
    ['0'] = 0,  ['1'] = 1,
    ['2'] = 2,  ['3'] = 3,
    ['4'] = 4,  ['5'] = 5,
    ['6'] = 6,  ['7'] = 7,
    ['8'] = 8,  ['9'] = 9,
    ['A'] = 10, ['B'] = 11,
    ['C'] = 12, ['D'] = 13,
    ['E'] = 14, ['F'] = 15,
    ['a'] = 10, ['b'] = 11,
    ['c'] = 12, ['d'] = 13,
    ['e'] = 14, ['f'] = 15,

    [' '] = -1, [','] = -1,
    ['}'] = -1, ['\n'] = -1,
    ['\t'] = -1
};

/*
 *	read next hex value in the input stream, return -1 if EOF
 */
static int
NextInt (
    FILE *fstream)
{
    int	ch;
    int	value = 0;
    int gotone = 0;
    int done = 0;

    /* loop, accumulate hex value until find delimiter  */
    /* skip any initial delimiters found in read stream */

    while (!done) {
	ch = getc(fstream);
	if (ch == EOF) {
	    value	= -1;
	    done++;
	} else {
	    /* trim high bits, check type and accumulate */
	    ch &= 0xff;
	    if (isascii(ch) && isxdigit(ch)) {
		value = (value << 4) + hexTable[ch];
		gotone++;
	    } else if ((hexTable[ch]) < 0 && gotone)
	      done++;
	}
    }
    return value;
}

int
XReadBitmapFileData (
    _Xconst char *filename,
    unsigned int *width,                /* RETURNED */
    unsigned int *height,               /* RETURNED */
    unsigned char **data,               /* RETURNED */
    int *x_hot,                         /* RETURNED */
    int *y_hot)                         /* RETURNED */
{
    FILE *fstream;			/* handle on file  */
    unsigned char *bits = NULL;		/* working variable */
    char line[MAX_SIZE];		/* input line from file */
    int size;				/* number of bytes of data */
    char name_and_type[MAX_SIZE];	/* an input line */
    char *type;				/* for parsing */
    int value;				/* from an input line */
    int version10p;			/* boolean, old format */
    int padding;			/* to handle alignment */
    int bytes_per_line;			/* per scanline of data */
    unsigned int ww = 0;		/* width */
    unsigned int hh = 0;		/* height */
    int hx = -1;			/* x hotspot */
    int hy = -1;			/* y hotspot */

#ifdef __UNIXOS2__
    filename = __XOS2RedirRoot(filename);
#endif
    if (!(fstream = fopen(filename, "r")))
	return BitmapOpenFailed;

    /* error cleanup and return macro	*/
#define	RETURN(code) \
{ Xfree (bits); fclose (fstream); return code; }

    while (fgets(line, MAX_SIZE, fstream)) {
	if (strlen(line) == MAX_SIZE-1)
	    RETURN (BitmapFileInvalid);
	if (sscanf(line,"#define %s %d",name_and_type,&value) == 2) {
	    if (!(type = strrchr(name_and_type, '_')))
	      type = name_and_type;
	    else
	      type++;

	    if (!strcmp("width", type))
	      ww = (unsigned int) value;
	    if (!strcmp("height", type))
	      hh = (unsigned int) value;
	    if (!strcmp("hot", type)) {
		if (type-- == name_and_type || type-- == name_and_type)
		  continue;
		if (!strcmp("x_hot", type))
		  hx = value;
		if (!strcmp("y_hot", type))
		  hy = value;
	    }
	    continue;
	}

	if (sscanf(line, "static short %s = {", name_and_type) == 1)
	  version10p = 1;
	else if (sscanf(line,"static unsigned char %s = {",name_and_type) == 1)
	  version10p = 0;
	else if (sscanf(line, "static char %s = {", name_and_type) == 1)
	  version10p = 0;
	else
	  continue;

	if (!(type = strrchr(name_and_type, '_')))
	  type = name_and_type;
	else
	  type++;

	if (strcmp("bits[]", type))
	  continue;

	if (!ww || !hh)
	  RETURN (BitmapFileInvalid);

	if ((ww % 16) && ((ww % 16) < 9) && version10p)
	  padding = 1;
	else
	  padding = 0;

	bytes_per_line = (ww+7)/8 + padding;

	bits = Xmallocarray (hh, bytes_per_line);
	if (!bits)
	  RETURN (BitmapNoMemory);
	size = bytes_per_line * hh;

	if (version10p) {
	    unsigned char *ptr;
	    int bytes;

	    for (bytes=0, ptr=bits; bytes<size; (bytes += 2)) {
		if ((value = NextInt(fstream)) < 0)
		  RETURN (BitmapFileInvalid);
		*(ptr++) = value;
		if (!padding || ((bytes+2) % bytes_per_line))
		  *(ptr++) = value >> 8;
	    }
	} else {
	    unsigned char *ptr;
	    int bytes;

	    for (bytes=0, ptr=bits; bytes<size; bytes++, ptr++) {
		if ((value = NextInt(fstream)) < 0)
		  RETURN (BitmapFileInvalid);
		*ptr=value;
	    }
	}

	/* If we got to this point, we read a full bitmap file. Break so we don't
	 * start reading another one from the same file and leak the memory
	 * allocated for the previous one. */
	break;
    }					/* end while */

    fclose(fstream);
    if (!bits)
	return (BitmapFileInvalid);

    *data = bits;
    *width = ww;
    *height = hh;
    if (x_hot) *x_hot = hx;
    if (y_hot) *y_hot = hy;

    return (BitmapSuccess);
}

int
XReadBitmapFile (
    Display *display,
    Drawable d,
    _Xconst char *filename,
    unsigned int *width,                /* RETURNED */
    unsigned int *height,               /* RETURNED */
    Pixmap *pixmap,                     /* RETURNED */
    int *x_hot,                         /* RETURNED */
    int *y_hot)                         /* RETURNED */
{
    unsigned char *data;
    int res;

    res = XReadBitmapFileData(filename, width, height, &data, x_hot, y_hot);
    if (res != BitmapSuccess)
	return res;
    *pixmap = XCreateBitmapFromData(display, d, (char *)data, *width, *height);
    Xfree(data);
    if (*pixmap == None)
	return (BitmapNoMemory);
    return (BitmapSuccess);
}
