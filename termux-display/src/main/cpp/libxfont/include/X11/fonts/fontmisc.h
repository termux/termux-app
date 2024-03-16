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

#ifndef _FONTMISC_H_
#define _FONTMISC_H_

#include <X11/Xfuncs.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/Xdefs.h>


#ifndef LSBFirst
#define LSBFirst	0
#define MSBFirst	1
#endif

#ifndef None
#define None	0l
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern Atom MakeAtom ( const char *string, unsigned len, int makeit );
extern int ValidAtom ( Atom atom );
extern const char *NameForAtom (Atom atom);

#define lowbit(x) ((x) & (~(x) + 1))

#undef assert
#define assert(x)	((void)0)

extern void
BitOrderInvert(
    register unsigned char *,
    register int
);

extern void
TwoByteSwap(
    register unsigned char *,
    register int
);

extern void
FourByteSwap(
    register unsigned char *,
    register int
);

extern int
RepadBitmap (
    char*,
    char*,
    unsigned,
    unsigned,
    int,
    int
);

extern void CopyISOLatin1Lowered(
    char * /*dest*/,
    const char * /*source*/,
    int /*length*/
);

extern void register_fpe_functions(void);

#endif /* _FONTMISC_H_ */
