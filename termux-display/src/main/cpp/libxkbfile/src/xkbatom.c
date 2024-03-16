/***********************************************************

Copyright 1987, 1998  The Open Group

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


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/************************************************************
 Copyright 1994 by Silicon Graphics Computer Systems, Inc.

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
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include "XKMformat.h"
#include "XKBfileInt.h"

/***====================================================================***/

#define InitialTableSize 100

typedef struct _Node {
    struct _Node *left, *right;
    Atom a;
    unsigned int fingerPrint;
    char *string;
} NodeRec, *NodePtr;

#define BAD_RESOURCE 0xe0000000

static Atom lastAtom = None;
static NodePtr atomRoot = (NodePtr) NULL;
static unsigned long tableLength;
static NodePtr *nodeTable = NULL;

static Atom
_XkbMakeAtom(const char *string, size_t len, Bool makeit)
{
    register NodePtr *np;
    unsigned i;
    int comp;
    register unsigned int fp = 0;

    np = &atomRoot;
    for (i = 0; i < (len + 1) / 2; i++) {
        fp = fp * 27 + string[i];
        fp = fp * 27 + string[len - 1 - i];
    }
    while (*np != (NodePtr) NULL) {
        if (fp < (*np)->fingerPrint)
            np = &((*np)->left);
        else if (fp > (*np)->fingerPrint)
            np = &((*np)->right);
        else {                  /* now start testing the strings */
            comp = strncmp(string, (*np)->string, len);
            if ((comp < 0) || ((comp == 0) && (len < strlen((*np)->string))))
                np = &((*np)->left);
            else if (comp > 0)
                np = &((*np)->right);
            else
                return (*np)->a;
        }
    }
    if (makeit) {
        register NodePtr nd;

        nd = (NodePtr) _XkbAlloc(sizeof(NodeRec));
        if (!nd)
            return BAD_RESOURCE;
#ifdef HAVE_STRNDUP
        nd->string = strndup(string, len);
#else
        nd->string = (char *) _XkbAlloc(len + 1);
#endif
        if (!nd->string) {
            _XkbFree(nd);
            return BAD_RESOURCE;
        }
#ifndef HAVE_STRNDUP
        strncpy(nd->string, string, len);
        nd->string[len] = 0;
#endif
        if ((lastAtom + 1) >= tableLength) {
            NodePtr *table;

            table = (NodePtr *) _XkbRealloc(nodeTable,
                                            tableLength * (2 *
                                                           sizeof(NodePtr)));
            if (!table) {
                _XkbFree(nd->string);
                _XkbFree(nd);
                return BAD_RESOURCE;
            }
            tableLength <<= 1;
            nodeTable = table;
        }
        *np = nd;
        nd->left = nd->right = (NodePtr) NULL;
        nd->fingerPrint = fp;
        nd->a = (++lastAtom);
        *(nodeTable + lastAtom) = nd;
        return nd->a;
    }
    else
        return None;
}

static char *
_XkbNameForAtom(Atom atom)
{
    NodePtr node;

    if (atom > lastAtom)
        return NULL;
    if ((node = nodeTable[atom]) == (NodePtr) NULL)
        return NULL;
    return strdup(node->string);
}

static void
_XkbInitAtoms(void)
{
    tableLength = InitialTableSize;
    nodeTable = (NodePtr *) _XkbAlloc(InitialTableSize * sizeof(NodePtr));
    if (nodeTable != NULL)
        nodeTable[None] = (NodePtr) NULL;
}

/***====================================================================***/

char *
XkbAtomGetString(Display *dpy, Atom atm)
{
    if (atm == None)
        return NULL;
    return _XkbNameForAtom(atm);
}

/***====================================================================***/

Atom
XkbInternAtom(Display *dpy, const char *name, Bool onlyIfExists)
{
    if (name == NULL)
        return None;
    return _XkbMakeAtom(name, strlen(name), (!onlyIfExists));
}

/***====================================================================***/

Atom
XkbChangeAtomDisplay(Display *oldDpy, Display *newDpy, Atom atm)
{
    if (atm != None) {
        char *tmp = XkbAtomGetString(oldDpy, atm);
        if (tmp != NULL) {
            Atom a = XkbInternAtom(newDpy, tmp, False);
            _XkbFree(tmp);
            return a;
        }
    }
    return None;
}

/***====================================================================***/

void
XkbInitAtoms(Display *dpy)
{
    if ((dpy == NULL) && (nodeTable == NULL)) {
        _XkbInitAtoms();
    }
    return;
}
