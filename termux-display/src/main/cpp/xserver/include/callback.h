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

#ifndef CALLBACK_H
#define CALLBACK_H

#include <X11/X.h>              /* for GContext, Mask */
#include <X11/Xdefs.h>          /* for Bool */
#include <X11/Xproto.h>
#include <X11/Xfuncproto.h>

/*
 *  callback manager stuff
 */

#ifndef _XTYPEDEF_CALLBACKLISTPTR
typedef struct _CallbackList *CallbackListPtr;  /* also in misc.h */

#define _XTYPEDEF_CALLBACKLISTPTR
#endif

typedef void (*CallbackProcPtr) (CallbackListPtr *, void *, void *);

extern _X_EXPORT Bool AddCallback(CallbackListPtr *pcbl,
                                  CallbackProcPtr callback,
                                  void *data);

extern _X_EXPORT Bool DeleteCallback(CallbackListPtr *pcbl,
                                     CallbackProcPtr callback,
                                     void *data);

extern _X_EXPORT void _CallCallbacks(CallbackListPtr *pcbl,
                                     void *call_data);

static inline void
CallCallbacks(CallbackListPtr *pcbl, void *call_data)
{
    if (!pcbl || !*pcbl)
        return;
    _CallCallbacks(pcbl, call_data);
}

extern _X_EXPORT void DeleteCallbackList(CallbackListPtr *pcbl);

extern _X_EXPORT void InitCallbackManager(void);
extern _X_EXPORT void DeleteCallbackManager(void);

#endif                          /* CALLBACK_H */
