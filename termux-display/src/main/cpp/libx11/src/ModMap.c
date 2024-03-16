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
#include "reallocarray.h"
#include <limits.h>

XModifierKeymap *
XGetModifierMapping(register Display *dpy)
{
    xGetModifierMappingReply rep;
    _X_UNUSED register xReq *req;
    unsigned long nbytes;
    XModifierKeymap *res;

    LockDisplay(dpy);
    GetEmptyReq(GetModifierMapping, req);
    (void) _XReply (dpy, (xReply *)&rep, 0, xFalse);

    if (rep.length < (INT_MAX >> 2) &&
	(rep.length >> 1) == rep.numKeyPerModifier) {
	nbytes = (unsigned long)rep.length << 2;
	res = Xmalloc(sizeof (XModifierKeymap));
	if (res)
	    res->modifiermap = Xmalloc (nbytes);
    } else
	res = NULL;
    if ((! res) || (! res->modifiermap)) {
	Xfree(res);
	res = (XModifierKeymap *) NULL;
	_XEatDataWords(dpy, rep.length);
    } else {
	_XReadPad(dpy, (char *) res->modifiermap, (long) nbytes);
	res->max_keypermod = rep.numKeyPerModifier;
    }

    UnlockDisplay(dpy);
    SyncHandle();
    return (res);
}

/*
 *	Returns:
 *	MappingSuccess (0)	Success
 *	MappingBusy (1) 	Busy - one or more old or new modifiers are down
 *	MappingFailed (2)	Failed - one or more new modifiers unacceptable
 */
int
XSetModifierMapping(
    register Display *dpy,
    register XModifierKeymap *modifier_map)
{
    register xSetModifierMappingReq *req;
    xSetModifierMappingReply rep;
    int         mapSize = modifier_map->max_keypermod << 3;	/* 8 modifiers */

    LockDisplay(dpy);
    GetReq(SetModifierMapping, req);
    req->length += mapSize >> 2;
    req->numKeyPerModifier = modifier_map->max_keypermod;

    Data(dpy, (const char *)modifier_map->modifiermap, mapSize);

    (void) _XReply(dpy, (xReply *) & rep,
	(SIZEOF(xSetModifierMappingReply) - SIZEOF(xReply)) >> 2, xTrue);
    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.success);
}

XModifierKeymap *
XNewModifiermap(int keyspermodifier)
{
    XModifierKeymap *res = Xmalloc((sizeof (XModifierKeymap)));
    if (res) {
	res->max_keypermod = keyspermodifier;
	res->modifiermap = (keyspermodifier > 0 ?
			    Xmallocarray(keyspermodifier, 8)
			    : (KeyCode *) NULL);
	if (keyspermodifier && (res->modifiermap == NULL)) {
	    Xfree(res);
	    return (XModifierKeymap *) NULL;
	}
    }
    return (res);
}


int
XFreeModifiermap(XModifierKeymap *map)
{
    if (map) {
        Xfree(map->modifiermap);
	Xfree(map);
    }
    return 1;
}

XModifierKeymap *
XInsertModifiermapEntry(XModifierKeymap *map,
#if NeedWidePrototypes
			unsigned int keycode,
#else
			KeyCode keycode,
#endif
			int modifier)
{
    XModifierKeymap *newmap;
    int i,
	row = modifier * map->max_keypermod,
	newrow,
	lastrow;

    for (i=0; i<map->max_keypermod; i++) {
        if (map->modifiermap[ row+i ] == keycode)
	    return(map); /* already in the map */
        if (map->modifiermap[ row+i ] == 0) {
            map->modifiermap[ row+i ] = keycode;
	    return(map); /* we added it without stretching the map */
	}
    }

    /* stretch the map */
    if ((newmap = XNewModifiermap(map->max_keypermod+1)) == NULL)
	return (XModifierKeymap *) NULL;
    newrow = row = 0;
    lastrow = newmap->max_keypermod * 8;
    while (newrow < lastrow) {
	for (i=0; i<map->max_keypermod; i++)
	    newmap->modifiermap[ newrow+i ] = map->modifiermap[ row+i ];
	newmap->modifiermap[ newrow+i ] = 0;
	row += map->max_keypermod;
	newrow += newmap->max_keypermod;
    }
    (void) XFreeModifiermap(map);
    newrow = newmap->max_keypermod * modifier + newmap->max_keypermod - 1;
    newmap->modifiermap[ newrow ] = keycode;
    return(newmap);
}

XModifierKeymap *
XDeleteModifiermapEntry(XModifierKeymap *map,
#if NeedWidePrototypes
			unsigned int keycode,
#else
			KeyCode keycode,
#endif
			int modifier)
{
    int i,
	row = modifier * map->max_keypermod;

    for (i=0; i<map->max_keypermod; i++) {
        if (map->modifiermap[ row+i ] == keycode)
            map->modifiermap[ row+i ] = 0;
    }
    /* should we shrink the map?? */
    return (map);
}
