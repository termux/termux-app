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

#ifndef _XKBFILE_H_
#define	_XKBFILE_H_ 1

#include "xkbstr.h"

/***====================================================================***/

#define	XkbXKMFile	0
#define	XkbCFile	1
#define	XkbXKBFile	2
#define	XkbMessage	3

#define	XkbMapDefined		(1<<0)
#define	XkbStateDefined		(1<<1)

typedef void (*XkbFileAddOnFunc) (FILE * /* file */ ,
                                  XkbDescPtr /* result */ ,
                                  Bool /* topLevel */ ,
                                  Bool /* showImplicit */ ,
                                  int /* fileSection */ ,
                                  void *        /* priv */
    );

/***====================================================================***/

#define	_XkbSuccess			0
#define	_XkbErrMissingNames		1
#define	_XkbErrMissingTypes		2
#define	_XkbErrMissingReqTypes		3
#define	_XkbErrMissingSymbols		4
#define	_XkbErrMissingVMods		5
#define	_XkbErrMissingIndicators	6
#define	_XkbErrMissingCompatMap		7
#define	_XkbErrMissingSymInterps	8
#define	_XkbErrMissingGeometry		9
#define	_XkbErrIllegalDoodad		10
#define	_XkbErrIllegalTOCType		11
#define	_XkbErrIllegalContents		12
#define	_XkbErrEmptyFile		13
#define	_XkbErrFileNotFound		14
#define	_XkbErrFileCannotOpen		15
#define	_XkbErrBadValue			16
#define	_XkbErrBadMatch			17
#define	_XkbErrBadTypeName		18
#define	_XkbErrBadTypeWidth		19
#define	_XkbErrBadFileType		20
#define	_XkbErrBadFileVersion		21
#define	_XkbErrBadFileFormat		22
#define	_XkbErrBadAlloc			23
#define	_XkbErrBadLength		24
#define	_XkbErrXReqFailure		25
#define	_XkbErrBadImplementation	26

/***====================================================================***/

#define XkbInternAtom XXkbInternAtom
#define XkbAtomText XXkbAtomText
#define XkbStringText XXkbStringText
#define XkbVModIndexText XXkbVModIndexText
#define XkbVModMaskText XXkbVModMaskText
#define XkbModMaskText XXkbModMaskText
#define XkbModIndexText XXkbModIndexText
#define XkbConfigText XXkbConfigText
#define XkbKeysymText XXkbKeysymText
#define XkbKeyNameText XXkbKeyNameText
#define XkbSIMatchText XXkbSIMatchText
#define XkbIMWhichStateMaskText XXkbIMWhichStateMaskText
#define XkbControlsMaskText XXkbControlsMaskText
#define XkbGeomFPText XXkbGeomFPText
#define XkbDoodadTypeText XXkbDoodadTypeText
#define XkbActionTypeText XXkbActionTypeText
#define XkbActionText XXkbActionText
#define XkbBehaviorText XXkbBehaviorText
#define XkbIndentText XXkbIndentText
#define XkbWriteXKBKeycodes XXkbWriteXKBKeycodes
#define XkbWriteXKBKeyTypes XXkbWriteXKBKeyTypes
#define XkbWriteXKBCompatMap XXkbWriteXKBCompatMap
#define XkbWriteXKBSymbols XXkbWriteXKBSymbols
#define XkbWriteXKBGeometry XXkbWriteXKBGeometry
#define XkbWriteXKBKeymapForNames XXkbWriteXKBKeymapForNames
#define XkbFindKeycodeByName XXkbFindKeycodeByName
#define XkbConvertGetByNameComponents XXkbConvertGetByNameComponents
#define XkbNameMatchesPattern XXkbNameMatchesPattern
#define _XkbKSCheckCase _XXkbKSCheckCase

_XFUNCPROTOBEGIN

extern _X_EXPORT char *XkbIndentText(unsigned   /* size */
    );

extern _X_EXPORT char *XkbAtomText(Atom /* atm */ ,
                                   unsigned     /* format */
    );

extern _X_EXPORT char *XkbKeysymText(KeySym /* sym */ ,
                                     unsigned   /* format */
    );

extern _X_EXPORT char *XkbStringText(char * /* str */ ,
                                     unsigned   /* format */
    );

extern _X_EXPORT char *XkbKeyNameText(char * /* name */ ,
                                      unsigned  /* format */
    );

extern _X_EXPORT char *XkbModIndexText(unsigned /* ndx */ ,
                                       unsigned /* format */
    );

extern _X_EXPORT char *XkbModMaskText(unsigned /* mask */ ,
                                      unsigned  /* format */
    );

extern _X_EXPORT char *XkbVModIndexText(XkbDescPtr /* xkb */ ,
                                        unsigned /* ndx */ ,
                                        unsigned        /* format */
    );

extern _X_EXPORT char *XkbVModMaskText(XkbDescPtr /* xkb */ ,
                                       unsigned /* modMask */ ,
                                       unsigned /* mask */ ,
                                       unsigned /* format */
    );

extern _X_EXPORT char *XkbConfigText(unsigned /* config */ ,
                                     unsigned   /* format */
    );

extern _X_EXPORT const char *XkbSIMatchText(unsigned /* type */ ,
                                            unsigned    /* format */
    );

extern _X_EXPORT char *XkbIMWhichStateMaskText(unsigned /* use_which */ ,
                                               unsigned /* format */
    );

extern _X_EXPORT char *XkbControlsMaskText(unsigned /* ctrls */ ,
                                           unsigned     /* format */
    );

extern _X_EXPORT char *XkbGeomFPText(int /* val */ ,
                                     unsigned   /* format */
    );

extern _X_EXPORT char *XkbDoodadTypeText(unsigned /* type */ ,
                                         unsigned       /* format */
    );

extern _X_EXPORT const char *XkbActionTypeText(unsigned /* type */ ,
                                               unsigned /* format */
    );

extern _X_EXPORT char *XkbActionText(XkbDescPtr /* xkb */ ,
                                     XkbAction * /* action */ ,
                                     unsigned   /* format */
    );

extern _X_EXPORT char *XkbBehaviorText(XkbDescPtr /* xkb */ ,
                                       XkbBehavior * /* behavior */ ,
                                       unsigned /* format */
    );

/***====================================================================***/

#define	_XkbKSLower	(1<<0)
#define	_XkbKSUpper	(1<<1)

#define	XkbKSIsLower(k)		(_XkbKSCheckCase(k)&_XkbKSLower)
#define	XkbKSIsUpper(k)		(_XkbKSCheckCase(k)&_XkbKSUpper)
#define XkbKSIsKeypad(k)	(((k)>=XK_KP_Space)&&((k)<=XK_KP_Equal))
#define	XkbKSIsDeadKey(k)	\
		(((k)>=XK_dead_grave)&&((k)<=XK_dead_semivoiced_sound))

extern _X_EXPORT unsigned _XkbKSCheckCase(KeySym        /* sym */
    );

extern _X_EXPORT int XkbFindKeycodeByName(XkbDescPtr /* xkb */ ,
                                          char * /* name */ ,
                                          Bool  /* use_aliases */
    );

/***====================================================================***/

extern _X_EXPORT Atom XkbInternAtom(char * /* name */ ,
                                    Bool        /* onlyIfExists */
    );

/***====================================================================***/

#ifdef _XKBGEOM_H_

#define	XkbDW_Unknown	0
#define	XkbDW_Doodad	1
#define	XkbDW_Section	2
typedef struct _XkbDrawable {
    int type;
    int priority;
    union {
        XkbDoodadPtr doodad;
        XkbSectionPtr section;
    } u;
    struct _XkbDrawable *next;
} XkbDrawableRec, *XkbDrawablePtr;

#endif

/***====================================================================***/

extern _X_EXPORT unsigned XkbConvertGetByNameComponents(Bool /* toXkm */ ,
                                                        unsigned        /* orig */
    );

extern _X_EXPORT Bool XkbNameMatchesPattern(char * /* name */ ,
                                            char *      /* pattern */
    );

/***====================================================================***/

extern _X_EXPORT Bool XkbWriteXKBKeycodes(FILE * /* file */ ,
                                          XkbDescPtr /* result */ ,
                                          Bool /* topLevel */ ,
                                          Bool /* showImplicit */ ,
                                          XkbFileAddOnFunc /* addOn */ ,
                                          void *        /* priv */
    );

extern _X_EXPORT Bool XkbWriteXKBKeyTypes(FILE * /* file */ ,
                                          XkbDescPtr /* result */ ,
                                          Bool /* topLevel */ ,
                                          Bool /* showImplicit */ ,
                                          XkbFileAddOnFunc /* addOn */ ,
                                          void *        /* priv */
    );

extern _X_EXPORT Bool XkbWriteXKBCompatMap(FILE * /* file */ ,
                                           XkbDescPtr /* result */ ,
                                           Bool /* topLevel */ ,
                                           Bool /* showImplicit */ ,
                                           XkbFileAddOnFunc /* addOn */ ,
                                           void *       /* priv */
    );

extern _X_EXPORT Bool XkbWriteXKBSymbols(FILE * /* file */ ,
                                         XkbDescPtr /* result */ ,
                                         Bool /* topLevel */ ,
                                         Bool /* showImplicit */ ,
                                         XkbFileAddOnFunc /* addOn */ ,
                                         void * /* priv */
    );

extern _X_EXPORT Bool XkbWriteXKBGeometry(FILE * /* file */ ,
                                          XkbDescPtr /* result */ ,
                                          Bool /* topLevel */ ,
                                          Bool /* showImplicit */ ,
                                          XkbFileAddOnFunc /* addOn */ ,
                                          void *        /* priv */
    );

extern _X_EXPORT Bool XkbWriteXKBKeymapForNames(FILE * /* file */ ,
                                                XkbComponentNamesPtr /* names */
                                                ,
                                                XkbDescPtr /* xkb */ ,
                                                unsigned /* want */ ,
                                                unsigned        /* need */
    );

/***====================================================================***/

extern _X_EXPORT Bool XkmProbe(FILE *   /* file */
    );

extern _X_EXPORT unsigned XkmReadFile(FILE * /* file */ ,
                                      unsigned /* need */ ,
                                      unsigned /* want */ ,
                                      XkbDescPtr *      /* result */
    );

_XFUNCPROTOEND
#endif                          /* _XKBFILE_H_ */
