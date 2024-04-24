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
#include "Xintatom.h"

static
char *_XGetAtomName(
    Display *dpy,
    Atom atom)
{
    xResourceReq *req;
    register Entry *table;
    register int idx;
    register Entry e;

    if (dpy->atoms) {
	table = dpy->atoms->table;
	for (idx = TABLESIZE; --idx >= 0; ) {
	    if ((e = *table++) && (e->atom == atom)) {
		return strdup(EntryName(e));
	    }
	}
    }
    GetResReq(GetAtomName, atom, req);
    return (char *)NULL;
}

char *XGetAtomName(
    register Display *dpy,
    Atom atom)
{
    xGetAtomNameReply rep;
    char *name;

    LockDisplay(dpy);
    if ((name = _XGetAtomName(dpy, atom))) {
	UnlockDisplay(dpy);
	return name;
    }
    if (_XReply(dpy, (xReply *)&rep, 0, xFalse) == 0) {
	UnlockDisplay(dpy);
	SyncHandle();
	return(NULL);
    }
    if ((name = Xmalloc(rep.nameLength + 1))) {
	_XReadPad(dpy, name, (long)rep.nameLength);
	name[rep.nameLength] = '\0';
	_XUpdateAtomCache(dpy, name, atom, 0, -1, 0);
    } else {
	_XEatDataWords(dpy, rep.length);
	name = (char *) NULL;
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return(name);
}

typedef struct {
    uint64_t start_seq;
    uint64_t stop_seq;
    Atom *atoms;
    char **names;
    int idx;
    int count;
    Status status;
} _XGetAtomNameState;

static
Bool _XGetAtomNameHandler(
    register Display *dpy,
    register xReply *rep,
    char *buf,
    int len,
    XPointer data)
{
    register _XGetAtomNameState *state;
    xGetAtomNameReply replbuf;
    register xGetAtomNameReply *repl;
    uint64_t last_request_read = X_DPY_GET_LAST_REQUEST_READ(dpy);

    state = (_XGetAtomNameState *)data;
    if (last_request_read < state->start_seq ||
	last_request_read > state->stop_seq)
	return False;
    while (state->idx < state->count && state->names[state->idx])
	state->idx++;
    if (state->idx >= state->count)
	return False;
    if (rep->generic.type == X_Error) {
	state->status = 0;
	return False;
    }
    repl = (xGetAtomNameReply *)
	_XGetAsyncReply(dpy, (char *)&replbuf, rep, buf, len,
			(SIZEOF(xGetAtomNameReply) - SIZEOF(xReply)) >> 2,
			False);
    state->names[state->idx] = Xmalloc(repl->nameLength + 1);
    _XGetAsyncData(dpy, state->names[state->idx], buf, len,
		   SIZEOF(xGetAtomNameReply), repl->nameLength,
		   repl->length << 2);
    if (state->names[state->idx]) {
	state->names[state->idx][repl->nameLength] = '\0';
	_XUpdateAtomCache(dpy, state->names[state->idx],
			  state->atoms[state->idx], 0, -1, 0);
    } else {
	state->status = 0;
    }
    return True;
}

Status
XGetAtomNames (
    Display *dpy,
    Atom *atoms,
    int count,
    char **names_return)
{
    _XAsyncHandler async;
    _XGetAtomNameState async_state;
    xGetAtomNameReply rep;
    int i;
    int missed = -1;

    LockDisplay(dpy);
    async_state.start_seq = X_DPY_GET_REQUEST(dpy) + 1;
    async_state.atoms = atoms;
    async_state.names = names_return;
    async_state.idx = 0;
    async_state.count = count - 1;
    async_state.status = 1;
    async.next = dpy->async_handlers;
    async.handler = _XGetAtomNameHandler;
    async.data = (XPointer)&async_state;
    dpy->async_handlers = &async;
    for (i = 0; i < count; i++) {
	if (!(names_return[i] = _XGetAtomName(dpy, atoms[i]))) {
	    missed = i;
	    async_state.stop_seq = X_DPY_GET_REQUEST(dpy);
	}
    }
    if (missed >= 0) {
	if (_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	    if ((names_return[missed] = Xmalloc(rep.nameLength + 1))) {
		_XReadPad(dpy, names_return[missed], (long)rep.nameLength);
		names_return[missed][rep.nameLength] = '\0';
		_XUpdateAtomCache(dpy, names_return[missed], atoms[missed],
				  0, -1, 0);
	    } else {
		_XEatDataWords(dpy, rep.length);
		async_state.status = 0;
	    }
	}
    }
    DeqAsyncHandler(dpy, &async);
    UnlockDisplay(dpy);
    if (missed >= 0)
	SyncHandle();
    return async_state.status;
}
