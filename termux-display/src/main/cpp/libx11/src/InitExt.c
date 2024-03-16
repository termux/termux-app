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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlibint.h>
#include <X11/Xos.h>
#include <stdio.h>

/*
 * This routine is used to link a extension in so it will be called
 * at appropriate times.
 */

XExtCodes *XInitExtension (
	Display *dpy,
	_Xconst char *name)
{
	XExtCodes codes;	/* temp. place for extension information. */
	register _XExtension *ext;/* need a place to build it all */
	if (!XQueryExtension(dpy, name,
		&codes.major_opcode, &codes.first_event,
		&codes.first_error)) return (NULL);

	LockDisplay (dpy);
	if (! (ext = Xcalloc (1, sizeof (_XExtension))) ||
	    ! (ext->name = strdup(name))) {
	    Xfree(ext);
	    UnlockDisplay(dpy);
	    return (XExtCodes *) NULL;
	}
	codes.extension = dpy->ext_number++;
	ext->codes = codes;

	/* chain it onto the display list */
	ext->next = dpy->ext_procs;
	dpy->ext_procs = ext;
	UnlockDisplay (dpy);

	return (&ext->codes);		/* tell him which extension */
}

XExtCodes *XAddExtension (Display *dpy)
{
    register _XExtension *ext;

    LockDisplay (dpy);
    if (! (ext = Xcalloc (1, sizeof (_XExtension)))) {
	UnlockDisplay(dpy);
	return (XExtCodes *) NULL;
    }
    ext->codes.extension = dpy->ext_number++;

    /* chain it onto the display list */
    ext->next = dpy->ext_procs;
    dpy->ext_procs = ext;
    UnlockDisplay (dpy);

    return (&ext->codes);		/* tell him which extension */
}

static _XExtension *XLookupExtension (
	register Display *dpy,	/* display */
	register int extension)	/* extension number */
{
	register _XExtension *ext;
	for (ext = dpy->ext_procs; ext; ext = ext->next)
	    if (ext->codes.extension == extension) return (ext);
	return (NULL);
}

XExtData **XEHeadOfExtensionList(XEDataObject object)
{
    return *(XExtData ***)&object;
}

int
XAddToExtensionList(
    XExtData **structure,
    XExtData *ext_data)
{
    ext_data->next = *structure;
    *structure = ext_data;
    return 1;
}

XExtData *XFindOnExtensionList(
    XExtData **structure,
    int number)
{
    XExtData *ext;

    ext = *structure;
    while (ext && (ext->number != number))
	ext = ext->next;
    return ext;
}

/*
 * Routines to hang procs on the extension structure.
 */
CreateGCType XESetCreateGC(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	CreateGCType proc)	/* routine to call when GC created */
{
	register _XExtension *e;	/* for lookup of extension */
	register CreateGCType oldproc;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->create_GC;
	e->create_GC = proc;
	UnlockDisplay(dpy);
	return (CreateGCType)oldproc;
}

CopyGCType XESetCopyGC(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	CopyGCType proc)	/* routine to call when GC copied */
{
	register _XExtension *e;	/* for lookup of extension */
	register CopyGCType oldproc;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->copy_GC;
	e->copy_GC = proc;
	UnlockDisplay(dpy);
	return (CopyGCType)oldproc;
}

FlushGCType XESetFlushGC(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	FlushGCType proc)	/* routine to call when GC copied */
{
	register _XExtension *e;	/* for lookup of extension */
	register FlushGCType oldproc;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->flush_GC;
	e->flush_GC = proc;
	UnlockDisplay(dpy);
	return (FlushGCType)oldproc;
}

FreeGCType XESetFreeGC(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	FreeGCType proc)	/* routine to call when GC freed */
{
	register _XExtension *e;	/* for lookup of extension */
	register FreeGCType oldproc;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->free_GC;
	e->free_GC = proc;
	UnlockDisplay(dpy);
	return (FreeGCType)oldproc;
}

CreateFontType XESetCreateFont(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	CreateFontType proc)	/* routine to call when font created */
{
	register _XExtension *e;	/* for lookup of extension */
	register CreateFontType oldproc;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->create_Font;
	e->create_Font = proc;
	UnlockDisplay(dpy);
	return (CreateFontType)oldproc;
}

FreeFontType XESetFreeFont(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	FreeFontType proc)	/* routine to call when font freed */
{
	register _XExtension *e;	/* for lookup of extension */
	register FreeFontType oldproc;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->free_Font;
	e->free_Font = proc;
	UnlockDisplay(dpy);
	return (FreeFontType)oldproc;
}

CloseDisplayType XESetCloseDisplay(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	CloseDisplayType proc)	/* routine to call when display closed */
{
	register _XExtension *e;	/* for lookup of extension */
	register CloseDisplayType oldproc;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->close_display;
	e->close_display = proc;
	UnlockDisplay(dpy);
	return (CloseDisplayType)oldproc;
}

typedef Bool (*WireToEventType) (
    Display*	/* display */,
    XEvent*	/* re */,
    xEvent*	/* event */
);

WireToEventType XESetWireToEvent(
	Display *dpy,		/* display */
	int event_number,	/* event routine to replace */
	WireToEventType proc)	/* routine to call when converting event */
{
	register WireToEventType oldproc;
	if (proc == NULL) proc = (WireToEventType)_XUnknownWireEvent;
	LockDisplay (dpy);
	oldproc = dpy->event_vec[event_number];
	dpy->event_vec[event_number] = proc;
	UnlockDisplay (dpy);
	return (WireToEventType)oldproc;
}

typedef Bool (*WireToEventCookieType) (
    Display*	/* display */,
    XGenericEventCookie*	/* re */,
    xEvent*	/* event */
);

WireToEventCookieType XESetWireToEventCookie(
    Display *dpy,       /* display */
    int extension,      /* extension major opcode */
    WireToEventCookieType proc /* routine to call for generic events */
    )
{
	WireToEventCookieType oldproc;
	if (proc == NULL) proc = (WireToEventCookieType)_XUnknownWireEventCookie;
	LockDisplay (dpy);
	oldproc = dpy->generic_event_vec[extension & 0x7F];
	dpy->generic_event_vec[extension & 0x7F] = proc;
	UnlockDisplay (dpy);
	return (WireToEventCookieType)oldproc;
}

typedef Bool (*CopyEventCookieType) (
    Display*	/* display */,
    XGenericEventCookie*	/* in */,
    XGenericEventCookie*	/* out */
);

CopyEventCookieType XESetCopyEventCookie(
    Display *dpy,       /* display */
    int extension,      /* extension major opcode */
    CopyEventCookieType proc /* routine to copy generic events */
    )
{
	CopyEventCookieType oldproc;
	if (proc == NULL) proc = (CopyEventCookieType)_XUnknownCopyEventCookie;
	LockDisplay (dpy);
	oldproc = dpy->generic_event_copy_vec[extension & 0x7F];
	dpy->generic_event_copy_vec[extension & 0x7F] = proc;
	UnlockDisplay (dpy);
	return (CopyEventCookieType)oldproc;
}


typedef Status (*EventToWireType) (
    Display*	/* display */,
    XEvent*	/* re */,
    xEvent*	/* event */
);

EventToWireType XESetEventToWire(
	Display *dpy,		/* display */
	int event_number,	/* event routine to replace */
	EventToWireType proc)	/* routine to call when converting event */
{
	register EventToWireType oldproc;
	if (proc == NULL) proc = (EventToWireType) _XUnknownNativeEvent;
	LockDisplay (dpy);
	oldproc = dpy->wire_vec[event_number];
	dpy->wire_vec[event_number] = proc;
	UnlockDisplay(dpy);
	return (EventToWireType)oldproc;
}

typedef Bool (*WireToErrorType) (
    Display*	/* display */,
    XErrorEvent* /* he */,
    xError*	/* we */
);

WireToErrorType XESetWireToError(
	Display *dpy,		/* display */
	int error_number,	/* error routine to replace */
	WireToErrorType proc)	/* routine to call when converting error */
{
	register WireToErrorType oldproc = NULL;
	if (proc == NULL) proc = (WireToErrorType)_XDefaultWireError;
	LockDisplay (dpy);
	if (!dpy->error_vec) {
	    int i;
	    dpy->error_vec = Xmalloc(256 * sizeof(oldproc));
	    for (i = 1; i < 256; i++)
		dpy->error_vec[i] = _XDefaultWireError;
	}
	if (dpy->error_vec) {
	    oldproc = dpy->error_vec[error_number];
	    dpy->error_vec[error_number] = proc;
	}
	UnlockDisplay (dpy);
	return (WireToErrorType)oldproc;
}

ErrorType XESetError(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	ErrorType proc)		/* routine to call when X error happens */
{
	register _XExtension *e;	/* for lookup of extension */
	register ErrorType oldproc;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->error;
	e->error = proc;
	UnlockDisplay(dpy);
	return (ErrorType)oldproc;
}

ErrorStringType XESetErrorString(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	ErrorStringType proc)	/* routine to call when I/O error happens */
{
	register _XExtension *e;	/* for lookup of extension */
	register ErrorStringType oldproc;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->error_string;
	e->error_string = proc;
	UnlockDisplay(dpy);
	return (ErrorStringType)oldproc;
}

PrintErrorType XESetPrintErrorValues(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	PrintErrorType proc)	/* routine to call to print */
{
	register _XExtension *e;	/* for lookup of extension */
	register PrintErrorType oldproc;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->error_values;
	e->error_values = proc;
	UnlockDisplay(dpy);
	return (PrintErrorType)oldproc;
}

BeforeFlushType XESetBeforeFlush(
	Display *dpy,		/* display */
	int extension,		/* extension number */
	BeforeFlushType proc)	/* routine to call on flush */
{
	register _XExtension *e;	/* for lookup of extension */
	register BeforeFlushType oldproc;
	register _XExtension *ext;
	if ((e = XLookupExtension (dpy, extension)) == NULL) return (NULL);
	LockDisplay(dpy);
	oldproc = e->before_flush;
	e->before_flush = proc;
	for (ext = dpy->flushes; ext && ext != e; ext = ext->next)
	    ;
	if (!ext) {
	    e->next_flush = dpy->flushes;
	    dpy->flushes = e;
	}
	UnlockDisplay(dpy);
	return (BeforeFlushType)oldproc;
}
