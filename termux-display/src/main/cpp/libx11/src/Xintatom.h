
#ifndef _XINTATOM_H_
#define _XINTATOM_H_ 1

#include <X11/Xlib.h>
#include <X11/Xfuncproto.h>

/* IntAtom.c */

#define TABLESIZE 64

typedef struct _Entry {
    unsigned long sig;
    Atom atom;
} EntryRec, *Entry;

#define RESERVED ((Entry) 1)

#define EntryName(e) ((char *)(e+1))

typedef struct _XDisplayAtoms {
    Entry table[TABLESIZE];
} AtomTable;

_XFUNCPROTOBEGIN

extern void _XUpdateAtomCache(Display *dpy, const char *name, Atom atom,
				unsigned long sig, int idx, int n);
extern void _XFreeAtomTable(Display *dpy);

_XFUNCPROTOEND

#endif /* _XINTATOM_H_ */
