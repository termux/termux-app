/*

Copyright 2007 Peter Hutterer <peter@cs.unisa.edu.au>

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
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the author shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the author.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _GEEXT_H_
#define _GEEXT_H_
#include <X11/extensions/geproto.h>

/** Struct to keep information about registered extensions */
typedef struct _GEExtension {
    /** Event swapping routine */
    void (*evswap) (xGenericEvent *from, xGenericEvent *to);
} GEExtension, *GEExtensionPtr;

/* All registered extensions and their handling functions. */
extern _X_EXPORT GEExtension GEExtensions[MAXEXTENSIONS];

/* Typecast to generic event */
#define GEV(ev) ((xGenericEvent*)(ev))
/* Returns the extension offset from the event */
#define GEEXT(ev) (GEV(ev)->extension)

/* Return zero-based extension offset (offset - 128). Only for use in arrays */
#define GEEXTIDX(ev) (GEEXT(ev) & 0x7F)
/* True if mask is set for extension on window */
#define GEMaskIsSet(pWin, extension, mask) \
    ((pWin)->optional && \
     (pWin)->optional->geMasks && \
     ((pWin)->optional->geMasks->eventMasks[(extension) & 0x7F] & (mask)))

/* Returns first client */
#define GECLIENT(pWin) \
    (((pWin)->optional) ? (pWin)->optional->geMasks->geClients : NULL)

/* Returns the event_fill for the given event */
#define GEEventFill(ev) \
    GEExtensions[GEEXTIDX(ev)].evfill

#define GEIsType(ev, ext, ev_type) \
        ((GEV(ev)->type == GenericEvent) &&  \
         GEEXT(ev) == (ext) && \
         GEV(ev)->evtype == (ev_type))

/* Interface for other extensions */
extern _X_EXPORT void GERegisterExtension(int extension,
                                          void (*ev_dispatch) (xGenericEvent
                                                               *from,
                                                               xGenericEvent
                                                               *to));

extern _X_EXPORT void GEInitEvent(xGenericEvent *ev, int extension);

#endif                          /* _GEEXT_H_ */
