/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

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

#ifndef _XKBLIBINT_H_
#define _XKBLIBINT_H_

#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include "reallocarray.h"

#define XkbMapPending           (1<<0)
#define XkbXlibNewKeyboard      (1<<1)

typedef int     (*XkbKSToMBFunc)(
        XPointer        /* priv */,
        KeySym          /* sym */,
        char *          /* buf */,
        int             /* len */,
        int *           /* extra_rtrn */
);

typedef KeySym  (*XkbMBToKSFunc)(
        XPointer        /* priv */,
        char *          /* buf */,
        int             /* len */,
        Status *        /* status */
);

typedef KeySym  (*XkbToUpperFunc)(
        KeySym          /* sym */
);

typedef struct _XkbConverters {
        XkbKSToMBFunc    KSToMB;
        XPointer         KSToMBPriv;
        XkbMBToKSFunc    MBToKS;
        XPointer         MBToKSPriv;
        XkbToUpperFunc   KSToUpper;
} XkbConverters;

extern  XkbInternAtomFunc       _XkbInternAtomFunc;
extern  XkbGetAtomNameFunc      _XkbGetAtomNameFunc;

typedef struct _XkbInfoRec {
        unsigned         flags;
        unsigned         xlib_ctrls;
        XExtCodes        *codes;
        int              srv_major;
        int              srv_minor;
        unsigned         selected_events;
        unsigned short   selected_nkn_details;
        unsigned short   selected_map_details;
        XkbDescRec      *desc;
        XkbMapChangesRec changes;
        Atom             composeLED;
        XkbConverters    cvt;
        XkbConverters    latin1cvt;
} XkbInfoRec, *XkbInfoPtr;


#define _XkbUnavailable(d) \
        (((d)->flags&XlibDisplayNoXkb) || \
         ((!(d)->xkb_info || (!(d)->xkb_info->desc)) && !_XkbLoadDpy(d)))

#define _XkbCheckPendingRefresh(d,xi) { \
    if ((xi)->flags&XkbXlibNewKeyboard) \
        _XkbReloadDpy((d)); \
    else if ((xi)->flags&XkbMapPending) { \
        if (XkbGetMapChanges((d),(xi)->desc, &(xi)->changes)==Success) { \
            LockDisplay((d)); \
            (xi)->changes.changed= 0; \
            UnlockDisplay((d)); \
        } \
    } \
}

#define _XkbNeedModmap(i)    ((!(i)->desc->map)||(!(i)->desc->map->modmap))

        /*
         * mask of the events that the "invisible" XKB support in Xlib needs
         */
#define XKB_XLIB_MAP_MASK (XkbAllClientInfoMask)

        /*
         * Handy helper macros
         */

typedef struct _XkbReadBuffer {
        int      error;
        int      size;
        char    *start;
        char    *data;
} XkbReadBufferRec, *XkbReadBufferPtr;

#define _XkbAlloc(s)            Xmalloc((s))
#define _XkbCalloc(n,s)         Xcalloc((n),(s))
#define _XkbRealloc(o,s)        Xrealloc((o),(s))
#define _XkbTypedAlloc(t)       ((t *)Xmalloc(sizeof(t)))
#define _XkbTypedCalloc(n,t)    ((t *)Xcalloc((n),sizeof(t)))
#define _XkbFree(p)             Xfree(p)

/* Resizes array to hold new_num elements, zeroing newly added entries.
   Destroys old array on failure. */
#define _XkbResizeArray(array, old_num, new_num, type)                       \
    do {                                                                     \
        if (array == NULL) {                                                 \
            array = _XkbTypedCalloc(new_num, type);                          \
        } else {                                                             \
            type *prev_array = array;                                        \
            array = Xreallocarray(array, new_num, sizeof(type));             \
            if (_X_UNLIKELY((array) == NULL)) {                              \
                _XkbFree(prev_array);                                        \
            } else if ((new_num) > (old_num)) {                              \
                bzero(&array[old_num],                                       \
                      ((new_num) - (old_num)) * sizeof(type));               \
            }                                                                \
        }                                                                    \
    } while(0)

_XFUNCPROTOBEGIN

extern  void _XkbReloadDpy(
    Display *           /* dpy */
);

extern KeySym _XKeycodeToKeysym(
    Display *           /* display */,
#if NeedWidePrototypes
    unsigned int        /* keycode */,
#else
    KeyCode             /* keycode */,
#endif
    int                 /* index */
);

extern KeyCode _XKeysymToKeycode(
    Display *           /* display */,
    KeySym              /* keysym */
);

extern KeySym _XLookupKeysym(
    XKeyEvent *         /* key_event */,
    int                 /* index */
);

extern int _XRefreshKeyboardMapping(
    XMappingEvent *     /* event_map */
);

extern unsigned _XKeysymToModifiers(
    Display *           /* dpy */,
    KeySym              /* ks */
);

extern int _XTranslateKey(
    register Display *  /* dpy */,
    KeyCode             /* keycode */,
    register unsigned int /* modifiers */,
    unsigned int *      /* modifiers_return */,
    KeySym *            /* keysym_return */
);

extern int      _XTranslateKeySym(
    Display *           /* dpy */,
    register KeySym     /* symbol */,
    unsigned int        /* modifiers */,
    char *              /* buffer */,
    int                 /* nbytes */
);

extern  int _XLookupString(
    register XKeyEvent *        /* event */,
    char *                      /* buffer */,
    int                         /* nbytes */,
    KeySym *                    /* keysym */,
    XComposeStatus *            /* status */
);

extern void _XkbNoteCoreMapChanges(
    XkbMapChangesRec *          /* old */,
    XMappingEvent *             /* new */,
    unsigned int                /* wanted */
);

extern  int _XkbInitReadBuffer(
    Display *           /* dpy */,
    XkbReadBufferPtr    /* buf */,
    int                 /* size */
);

extern int _XkbSkipReadBufferData(
    XkbReadBufferPtr    /* from */,
    int                 /* size */
);

extern int _XkbCopyFromReadBuffer(
    XkbReadBufferPtr    /* from */,
    char *              /* to */,
    int                 /* size */
);


#ifdef LONG64
extern  int _XkbReadCopyData32(
    int *               /* from */,
    long *              /* to */,
    int                 /* num_words */
);

extern  int _XkbWriteCopyData32(
    unsigned long *     /* from */,
    CARD32 *            /* to */,
    int                 /* num_words */
);

extern int _XkbReadBufferCopy32(
    XkbReadBufferPtr    /* from */,
    long *              /* to */,
    int                 /* size */
);
#else
#define _XkbReadCopyData32(f,t,s)    memcpy((char *)(t), (char *)(f), (s)*4)
#define _XkbWriteCopyData32(f,t,s)   memcpy((char *)(t), (char *)(f), (s)*4)
#define _XkbReadBufferCopy32(f,t,s) _XkbCopyFromReadBuffer(f, (char *)t, (s)*4)
#endif

#ifndef NO_DEC_BINARY_COMPATIBILITY
#define XKB_FORCE_INT_KEYSYM 1
#endif

#ifdef XKB_FORCE_INT_KEYSYM
extern int _XkbReadCopyKeySyms(
    int *               /* from */,
    KeySym *            /* to */,
    int                 /* num_words */
);

extern  int _XkbWriteCopyKeySyms(
    KeySym *            /* from */,
    CARD32 *            /* to */,
    int                 /* num_words */
);

extern int _XkbReadBufferCopyKeySyms(
    XkbReadBufferPtr    /* from */,
#ifndef NO_DEC_BUG_FIX
    KeySym *            /* to */,
#else
    long *              /* to */,
#endif
    int                 /* size */
);
#else
#define _XkbReadCopyKeySyms(f,t,n)              _XkbReadCopyData32(f,t,n)
#define _XkbWriteCopyKeySyms(f,t,n)             _XkbWriteCopyData32(f,t,n)
#define _XkbReadBufferCopyKeySyms(f,t,s)        _XkbReadBufferCopy32(f,t,s)
#endif

extern char *_XkbPeekAtReadBuffer(
    XkbReadBufferPtr    /* from */,
    int                 /*  size */
);

extern char *_XkbGetReadBufferPtr(
    XkbReadBufferPtr    /* from */,
    int                 /* size */
);
#define _XkbGetTypedRdBufPtr(b,n,t) ((t *)_XkbGetReadBufferPtr(b,(n)*SIZEOF(t)))

extern int _XkbFreeReadBuffer(
    XkbReadBufferPtr    /* buf */
);

extern Bool
_XkbGetReadBufferCountedString(
    XkbReadBufferPtr    /* buf */,
    char **             /* rtrn */
);

extern char *_XkbGetCharset(
    void
);

extern int       _XkbGetConverters(
    const char *       /* encoding_name */,
    XkbConverters *    /* cvt_rtrn */
);

#ifdef  NEED_MAP_READERS

extern  Status  _XkbReadGetMapReply(
    Display *           /* dpy */,
    xkbGetMapReply *    /* rep */,
    XkbDescRec *        /* xkb */,
    int *               /* nread_rtrn */
);

extern  Status  _XkbReadGetCompatMapReply(
    Display *                   /* dpy */,
    xkbGetCompatMapReply *      /* rep */,
    XkbDescPtr                  /* xkb */,
    int *                       /* nread_rtrn */
);

extern  Status  _XkbReadGetIndicatorMapReply(
    Display *                   /* dpy */,
    xkbGetIndicatorMapReply *   /* rep */,
    XkbDescPtr                  /* xkb */,
    int *                       /* nread_rtrn */
);

extern  Status  _XkbReadGetNamesReply(
    Display *                   /* dpy */,
    xkbGetNamesReply *          /* rep */,
    XkbDescPtr                  /* xkb */,
    int *                       /* nread_rtrn */
);

extern  Status  _XkbReadGetGeometryReply(
    Display *                   /* dpy */,
    xkbGetGeometryReply *       /* rep */,
    XkbDescPtr                  /* xkb */,
    int *                       /* nread_rtrn */
);

#endif

_XFUNCPROTOEND

#endif /* _XKBLIBINT_H_ */
