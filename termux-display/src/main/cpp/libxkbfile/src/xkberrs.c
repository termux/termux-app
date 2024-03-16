/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#elif defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/Xproto.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xfuncs.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBfile.h>

const char *_XkbErrMessages[] = {
    "success",                                  /* _XkbSuccess               */
    "key names not defined",                    /* _XkbErrMissingNames       */
    "key types not defined",                    /* _XkbErrMissingTypes       */
    "required key types not present",           /* _XkbErrMissingReqTypes    */
    "symbols not defined",                      /* _XkbErrMissingSymbols     */
    "virtual modifier bindings not defined",    /* _XkbErrMissingVMods       */
    "indicators not defined",                   /* _XkbErrMissingIndicators  */
    "compatibility map not defined",            /* _XkbErrMissingCompatMap   */
    "symbol interpretations not defined",       /* _XkbErrMissingSymInterps  */
    "geometry not defined",                     /* _XkbErrMissingGeometry    */
    "illegal doodad type",                      /* _XkbErrIllegalDoodad      */
    "illegal TOC type",                         /* _XkbErrIllegalTOCType     */
    "illegal contents",                         /* _XkbErrIllegalContents    */
    "empty file",                               /* _XkbErrEmptyFile          */
    "file not found",                           /* _XkbErrFileNotFound       */
    "cannot open",                              /* _XkbErrFileCannotOpen     */
    "bad value",                                /* _XkbErrBadValue           */
    "bad match",                                /* _XkbErrBadMatch           */
    "illegal name for type",                    /* _XkbErrBadTypeName        */
    "illegal width for type",                   /* _XkbErrBadTypeWidth       */
    "bad file type",                            /* _XkbErrBadFileType        */
    "bad file version",                         /* _XkbErrBadFileVersion     */
    "error in Xkm file",                        /* _XkbErrBadFileFormat      */
    "allocation failed",                        /* _XkbErrBadAlloc           */
    "bad length",                               /* _XkbErrBadLength          */
    "X request failed",                         /* _XkbErrXReqFailure        */
    "not implemented"                           /* _XkbErrBadImplementation  */
};

unsigned        _XkbErrCode;
const char *    _XkbErrLocation = NULL;
unsigned        _XkbErrData;
