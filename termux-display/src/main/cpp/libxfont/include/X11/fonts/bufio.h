/*

Copyright 1993, 1998  The Open Group

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

#ifndef ___BUFIO_H___
#define ___BUFIO_H___ 1

#include <X11/Xfuncproto.h>

#define BUFFILESIZE	8192
#define BUFFILEEOF	-1

typedef unsigned char BufChar;
typedef struct _buffile *BufFilePtr;

typedef struct _buffile {
    BufChar *bufp;
    int	    left;
    int     eof;
    BufChar buffer[BUFFILESIZE];
    int	    (*input)( BufFilePtr /* f */);
    int     (*output)( int /* c */, BufFilePtr /* f */);
    int	    (*skip)( BufFilePtr /* f */, int /* count */);
    int	    (*close)( BufFilePtr /* f */, int /* doClose */);
    char    *private;
} BufFileRec;

extern BufFilePtr BufFileCreate (
    char*,
    int (*)(BufFilePtr),
    int (*)(int, BufFilePtr),
    int (*)(BufFilePtr, int),
    int (*)(BufFilePtr, int));
extern BufFilePtr BufFileOpenRead ( int );
extern BufFilePtr BufFileOpenWrite ( int );
extern BufFilePtr BufFilePushCompressed ( BufFilePtr );
#ifdef X_GZIP_FONT_COMPRESSION
extern BufFilePtr BufFilePushZIP ( BufFilePtr );
#endif
#ifdef X_BZIP2_FONT_COMPRESSION
extern BufFilePtr BufFilePushBZIP2 ( BufFilePtr );
#endif
extern int BufFileClose ( BufFilePtr, int );
extern int BufFileRead ( BufFilePtr, char*, int );
extern int BufFileWrite ( BufFilePtr, const char*, int );

#define BufFileGet(f)	((f)->left-- ? *(f)->bufp++ : ((f)->eof = (*(f)->input) (f)))
#define BufFilePut(c,f)	(--(f)->left ? *(f)->bufp++ = ((unsigned char)(c)) : (*(f)->output) ((unsigned char)(c),f))
#define BufFileSkip(f,c)    ((f)->eof = (*(f)->skip) (f, c))

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif /* ___BUFIO_H___ */

