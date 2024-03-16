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

#ifndef CLOSESTR_H
#define CLOSESTR_H

#include <X11/Xproto.h>
#include "closure.h"
#include "dix.h"
#include "misc.h"
#include "gcstruct.h"

/* closure structures */

/* OpenFont */

typedef struct _OFclosure {
    ClientPtr client;
    short current_fpe;
    short num_fpes;
    FontPathElementPtr *fpe_list;
    Mask flags;

/* XXX -- get these from request buffer instead? */
    const char *origFontName;
    int origFontNameLen;
    XID fontid;
    char *fontname;
    int fnamelen;
    FontPtr non_cachable_font;
} OFclosureRec;

/* ListFontsWithInfo */

#define XLFDMAXFONTNAMELEN	256
typedef struct _LFWIstate {
    char pattern[XLFDMAXFONTNAMELEN];
    int patlen;
    int current_fpe;
    int max_names;
    Bool list_started;
    void *private;
} LFWIstateRec, *LFWIstatePtr;

typedef struct _LFWIclosure {
    ClientPtr client;
    int num_fpes;
    FontPathElementPtr *fpe_list;
    xListFontsWithInfoReply *reply;
    int length;
    LFWIstateRec current;
    LFWIstateRec saved;
    int savedNumFonts;
    Bool haveSaved;
    char *savedName;
} LFWIclosureRec;

/* ListFonts */

typedef struct _LFclosure {
    ClientPtr client;
    int num_fpes;
    FontPathElementPtr *fpe_list;
    FontNamesPtr names;
    LFWIstateRec current;
    LFWIstateRec saved;
    Bool haveSaved;
    char *savedName;
    int savedNameLen;
} LFclosureRec;

/* PolyText */

typedef struct _PTclosure {
    ClientPtr client;
    DrawablePtr pDraw;
    GC *pGC;
    unsigned char *pElt;
    unsigned char *endReq;
    unsigned char *data;
    int xorg;
    int yorg;
    CARD8 reqType;
    XID did;
    int err;
} PTclosureRec;

/* ImageText */

typedef struct _ITclosure {
    ClientPtr client;
    DrawablePtr pDraw;
    GC *pGC;
    BYTE nChars;
    unsigned char *data;
    int xorg;
    int yorg;
    CARD8 reqType;
    XID did;
} ITclosureRec;
#endif                          /* CLOSESTR_H */
