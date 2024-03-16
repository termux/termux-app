/*

Copyright 1986, 1990, 1998  The Open Group

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xintatom.h"

#define HASH(sig) ((sig) & (TABLESIZE-1))
#define REHASHVAL(sig) ((((sig) % (TABLESIZE-3)) + 2) | 1)
#define REHASH(idx,rehash) ((idx + rehash) & (TABLESIZE-1))

void
_XFreeAtomTable(Display *dpy)
{
    register Entry *table;
    register int i;
    register Entry e;

    if (dpy->atoms) {
	table = dpy->atoms->table;
	for (i = TABLESIZE; --i >= 0; ) {
	    if ((e = *table++) && (e != RESERVED))
		Xfree(e);
	}
	Xfree(dpy->atoms);
	dpy->atoms = NULL;
    }
}

static
Atom _XInternAtom(
    Display *dpy,
    _Xconst char *name,
    Bool onlyIfExists,
    unsigned long *psig,
    int *pidx,
    int *pn)
{
    register AtomTable *atoms;
    register char *s1, c, *s2;
    register unsigned long sig;
    register int idx = 0, i;
    Entry e;
    int n, firstidx, rehash = 0;
    xInternAtomReq *req;

    /* look in the cache first */
    if (!(atoms = dpy->atoms)) {
	dpy->atoms = atoms = Xcalloc(1, sizeof(AtomTable));
	dpy->free_funcs->atoms = _XFreeAtomTable;
    }
    sig = 0;
    for (s1 = (char *)name; (c = *s1++); )
	sig += c;
    n = s1 - (char *)name - 1;
    if (atoms) {
	firstidx = idx = HASH(sig);
	while ((e = atoms->table[idx])) {
	    if (e != RESERVED && e->sig == sig) {
	    	for (i = n, s1 = (char *)name, s2 = EntryName(e); --i >= 0; ) {
		    if (*s1++ != *s2++)
		    	goto nomatch;
	    	}
	    	if (!*s2)
		    return e->atom;
	    }
nomatch:    if (idx == firstidx)
		rehash = REHASHVAL(sig);
	    idx = REHASH(idx, rehash);
	    if (idx == firstidx)
		break;
	}
    }
    *psig = sig;
    *pidx = idx;
    if (atoms && !atoms->table[idx])
	atoms->table[idx] = RESERVED; /* reserve slot */
    *pn = n;
    /* not found, go to the server */
    GetReq(InternAtom, req);
    req->nbytes = n;
    req->onlyIfExists = onlyIfExists;
    req->length += (n+3)>>2;
    Data(dpy, name, n);
    return None;
}

void
_XUpdateAtomCache(
    Display *dpy,
    const char *name,
    Atom atom,
    unsigned long sig,
    int idx,
    int n)
{
    Entry e, oe;
    register char *s1;
    register char c;
    int firstidx, rehash;

    if (!dpy->atoms) {
	if (idx < 0) {
	    dpy->atoms = Xcalloc(1, sizeof(AtomTable));
	    dpy->free_funcs->atoms = _XFreeAtomTable;
	}
	if (!dpy->atoms)
	    return;
    }
    if (!sig) {
	for (s1 = (char *)name; (c = *s1++); )
	    sig += c;
	n = s1 - (char *)name - 1;
	if (idx < 0) {
	    firstidx = idx = HASH(sig);
	    if (dpy->atoms->table[idx]) {
		rehash = REHASHVAL(sig);
		do
		    idx = REHASH(idx, rehash);
		while (idx != firstidx && dpy->atoms->table[idx]);
	    }
	}
    }
    e = Xmalloc(sizeof(EntryRec) + n + 1);
    if (e) {
	e->sig = sig;
	e->atom = atom;
	strcpy(EntryName(e), name);
	if ((oe = dpy->atoms->table[idx]) && (oe != RESERVED))
	    Xfree(oe);
	dpy->atoms->table[idx] = e;
    }
}

Atom
XInternAtom (
    Display *dpy,
    const char *name,
    Bool onlyIfExists)
{
    Atom atom;
    unsigned long sig;
    int idx, n;
    xInternAtomReply rep;

    if (!name)
	name = "";
    LockDisplay(dpy);
    if ((atom = _XInternAtom(dpy, name, onlyIfExists, &sig, &idx, &n))) {
	UnlockDisplay(dpy);
	return atom;
    }
    if (dpy->atoms && dpy->atoms->table[idx] == RESERVED)
	dpy->atoms->table[idx] = NULL; /* unreserve slot */
    if (_XReply (dpy, (xReply *)&rep, 0, xTrue)) {
	if ((atom = rep.atom))
	    _XUpdateAtomCache(dpy, name, atom, sig, idx, n);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return (rep.atom);
}

typedef struct {
    uint64_t start_seq;
    uint64_t stop_seq;
    char **names;
    Atom *atoms;
    int count;
    Status status;
} _XIntAtomState;

static
Bool _XIntAtomHandler(
    register Display *dpy,
    register xReply *rep,
    char *buf,
    int len,
    XPointer data)
{
    register _XIntAtomState *state;
    register int i, idx = 0;
    xInternAtomReply replbuf;
    register xInternAtomReply *repl;
    uint64_t last_request_read = X_DPY_GET_LAST_REQUEST_READ(dpy);

    state = (_XIntAtomState *)data;

    if (last_request_read < state->start_seq ||
	last_request_read > state->stop_seq)
	return False;
    for (i = 0; i < state->count; i++) {
	if (state->atoms[i] & 0x80000000) {
	    idx = ~state->atoms[i];
	    state->atoms[i] = None;
	    break;
	}
    }
    if (i >= state->count)
	return False;
    if (rep->generic.type == X_Error) {
	state->status = 0;
	return False;
    }
    repl = (xInternAtomReply *)
	_XGetAsyncReply(dpy, (char *)&replbuf, rep, buf, len,
			(SIZEOF(xInternAtomReply) - SIZEOF(xReply)) >> 2,
			True);
    if ((state->atoms[i] = repl->atom))
	_XUpdateAtomCache(dpy, state->names[i], (Atom) repl->atom,
			  (unsigned long)0, idx, 0);
    return True;
}

Status
XInternAtoms (
    Display *dpy,
    char **names,
    int count,
    Bool onlyIfExists,
    Atom *atoms_return)
{
    int i, idx, n, tidx;
    unsigned long sig;
    _XAsyncHandler async;
    _XIntAtomState async_state;
    int missed = -1;
    xInternAtomReply rep;

    LockDisplay(dpy);
    async_state.start_seq = X_DPY_GET_REQUEST(dpy) + 1;
    async_state.atoms = atoms_return;
    async_state.names = names;
    async_state.count = count - 1;
    async_state.status = 1;
    async.next = dpy->async_handlers;
    async.handler = _XIntAtomHandler;
    async.data = (XPointer)&async_state;
    dpy->async_handlers = &async;
    for (i = 0; i < count; i++) {
	if (!(atoms_return[i] = _XInternAtom(dpy, names[i], onlyIfExists,
					     &sig, &idx, &n))) {
	    missed = i;
	    atoms_return[i] = ~((Atom)idx);
	    async_state.stop_seq = X_DPY_GET_REQUEST(dpy);
	}
    }
    if (missed >= 0) {
        if (dpy->atoms) {
	    /* unreserve anything we just reserved */
	    for (i = 0; i < count; i++) {
		if (atoms_return[i] & 0x80000000) {
		    tidx = ~atoms_return[i];
		    if (dpy->atoms->table[tidx] == RESERVED)
			dpy->atoms->table[tidx] = NULL;
		}
	    }
        }
	if (_XReply (dpy, (xReply *)&rep, 0, xTrue)) {
	    if ((atoms_return[missed] = rep.atom))
		_XUpdateAtomCache(dpy, names[missed], (Atom) rep.atom,
				  sig, idx, n);
	} else {
	    atoms_return[missed] = None;
	    async_state.status = 0;
	}
    }
    DeqAsyncHandler(dpy, &async);
    UnlockDisplay(dpy);
    if (missed >= 0)
	SyncHandle();
    return async_state.status;
}
