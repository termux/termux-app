/*

Copyright 1991, 1998  The Open Group

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
 * Author:  Keith Packard, MIT X Consortium
 */

#ifndef _FNTFILIO_H_
#define _FNTFILIO_H_

#include <X11/fonts/bufio.h>

typedef BufFilePtr  FontFilePtr;

#define FontFileGetc(f)	    BufFileGet(f)
#define FontFilePutc(c,f)   BufFilePut(c,f)
#define FontFileRead(f,b,n) BufFileRead(f,b,n)
#define FontFileWrite(f,b,n)	BufFileWrite(f,b,n)
#define FontFileSkip(f,n)   (BufFileSkip (f, n) != BUFFILEEOF)
#define FontFileSeek(f,n)   (BufFileSeek (f,n,0) != BUFFILEEOF)

#define FontFileEOF	BUFFILEEOF

extern FontFilePtr FontFileOpen ( const char *name );
extern int FontFileClose ( FontFilePtr f );
extern FontFilePtr FontFileOpenWrite ( const char *name );
extern FontFilePtr FontFileOpenWriteFd ( int fd );
extern FontFilePtr FontFileOpenFd ( int fd );

#endif /* _FNTFILIO_H_ */
