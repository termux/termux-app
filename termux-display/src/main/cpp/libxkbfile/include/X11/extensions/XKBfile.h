
#ifndef _XKBFILE_H_
#define	_XKBFILE_H_ 1

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

/***====================================================================***/

#define	XkbXKMFile	0
#define	XkbCFile	1
#define	XkbXKBFile	2
#define	XkbMessage	3

#define	XkbMapDefined		(1<<0)
#define	XkbStateDefined		(1<<1)

typedef struct _XkbFileInfo {
    unsigned		type;
    unsigned		defined;
    XkbDescPtr	 	xkb;
} XkbFileInfo,*XkbFileInfoPtr;

typedef void	(*XkbFileAddOnFunc)(
    FILE *		/* file */,
    XkbFileInfo *	/* result */,
    Bool		/* topLevel */,
    Bool		/* showImplicit */,
    int			/* fileSection */,
    void *		/* priv */
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

extern const char *	_XkbErrMessages[];
extern unsigned		_XkbErrCode;
extern const char *	_XkbErrLocation;
extern unsigned		_XkbErrData;

/***====================================================================***/

_XFUNCPROTOBEGIN

extern	char *	XkbIndentText(
    unsigned	/* size */
);

extern	char *	XkbAtomText(
    Display *	/* dpy */,
    Atom 	/* atm */,
    unsigned	/* format */
);

extern char *	XkbKeysymText(
    KeySym	/* sym */,
    unsigned	/* format */
);

extern char *	XkbStringText(
    char *	/* str */,
    unsigned	/* format */
);

extern char *	XkbKeyNameText(
    char *	/* name */,
    unsigned	/* format */
);

extern char *
XkbModIndexText(
    unsigned	/* ndx */,
    unsigned	/* format */
);

extern char *
XkbModMaskText(
    unsigned	/* mask */,
    unsigned	/* format */
);

extern char *	XkbVModIndexText(
    Display *	/* dpy */,
    XkbDescPtr	/* xkb */,
    unsigned	/* ndx */,
    unsigned	/* format */
);

extern	char *	XkbVModMaskText(
    Display *	/* dpy */,
    XkbDescPtr	/* xkb */,
    unsigned	/* modMask */,
    unsigned	/* mask */,
    unsigned	/* format */
);

extern char *	XkbConfigText(
    unsigned 	/* config */,
    unsigned 	/* format */
);

extern char *	XkbSIMatchText(
    unsigned	/* type */,
    unsigned	/* format */
);

extern char *	XkbIMWhichStateMaskText(
    unsigned	/* use_which */,
    unsigned	/* format */
);

extern char *	XkbAccessXDetailText(
    unsigned	/* state */,
    unsigned	/* format */
);

extern char *	XkbNKNDetailMaskText(
    unsigned	/* detail */,
    unsigned	/* format */
);

extern char *	XkbControlsMaskText(
    unsigned	/* ctrls */,
    unsigned	/* format */
);

extern char *	XkbGeomFPText(
    int		/* val */,
    unsigned 	/* format */
);

extern char *	XkbDoodadTypeText(
    unsigned	/* type */,
    unsigned	/* format */
);

extern char *	XkbActionTypeText(
    unsigned	/* type */,
    unsigned	/* format */
);

extern char *	XkbActionText(
    Display *	/* dpy */,
    XkbDescPtr	/* xkb */,
    XkbAction *	/* action */,
    unsigned	/* format */
);

extern char *	XkbBehaviorText(
    XkbDescPtr 		/* xkb */,
    XkbBehavior *	/* behavior */,
    unsigned		/* format */
);

/***====================================================================***/

#define	_XkbKSLower	(1<<0)
#define	_XkbKSUpper	(1<<1)

#define	XkbKSIsLower(k)		(_XkbKSCheckCase(k)&_XkbKSLower)
#define	XkbKSIsUpper(k)		(_XkbKSCheckCase(k)&_XkbKSUpper)
#define XkbKSIsKeypad(k)	(((k)>=XK_KP_Space)&&((k)<=XK_KP_Equal))
#define	XkbKSIsDeadKey(k)	\
		(((k)>=XK_dead_grave)&&((k)<=XK_dead_semivoiced_sound))

extern	unsigned _XkbKSCheckCase(
   KeySym	/* sym */
);

extern	int	 XkbFindKeycodeByName(
    XkbDescPtr	/* xkb */,
    char *	/* name */,
    Bool	/* use_aliases */
);

extern	Bool	XkbLookupGroupAndLevel(
    XkbDescPtr	/* xkb */,
    int		/* key */,
    int	*	/* mods_inout */,
    int *	/* grp_inout */,
    int	*	/* lvl_rtrn */
);

/***====================================================================***/


extern Bool	XkbLookupCanonicalRGBColor(
    char *	/* def */,
    XColor *	/* color */
);


/***====================================================================***/

extern	char *	XkbAtomGetString(
    Display *	/* dpy */,
    Atom 	/* atm */
);

extern	Atom	XkbInternAtom(
    Display *	/* dpy */,
    const char */* name */,
    Bool	/* onlyIfExists */
);

extern	Status	XkbChangeKbdDisplay(
    Display *		/* newDpy */,
    XkbFileInfo *	/* result */
);

extern	Atom	XkbChangeAtomDisplay(
    Display *	/* oldDpy */,
    Display *	/* newDpy */,
    Atom	/* atm */
);

extern	void	XkbInitAtoms(
    Display *	/* dpy */
);

/***====================================================================***/

#ifdef _XKBGEOM_H_

#define	XkbDW_Unknown	0
#define	XkbDW_Doodad	1
#define	XkbDW_Section	2
typedef struct _XkbDrawable {
	int		type;
	int		priority;
	union {
	    XkbDoodadPtr	doodad;
	    XkbSectionPtr	section;
	} u;
	struct _XkbDrawable *	next;
} XkbDrawableRec,*XkbDrawablePtr;

extern	XkbDrawablePtr
XkbGetOrderedDrawables(
    XkbGeometryPtr	/* geom */,
    XkbSectionPtr	/* section */
);

extern	void
XkbFreeOrderedDrawables(
    XkbDrawablePtr	/* draw */
);

#endif

/***====================================================================***/

extern	unsigned	XkbConvertGetByNameComponents(
    Bool		/* toXkm */,
    unsigned 		/* orig */
);

extern	unsigned	XkbConvertXkbComponents(
    Bool		/* toXkm */,
    unsigned 		/* orig */
);

extern	Bool	XkbDetermineFileType(
    XkbFileInfo *	/* xkb */,
    int			/* format */,
    int *		/* opts_missing */
);

extern	Bool	XkbNameMatchesPattern(
    char *		/* name */,
    char *		/* pattern */
);

/***====================================================================***/

extern	Bool	XkbWriteXKBKeycodes(
    FILE *		/* file */,
    XkbFileInfo *	/* result */,
    Bool		/* topLevel */,
    Bool		/* showImplicit */,
    XkbFileAddOnFunc	/* addOn */,
    void *		/* priv */
);

extern	Bool	XkbWriteXKBKeyTypes(
    FILE *		/* file */,
    XkbFileInfo *	/* result */,
    Bool		/* topLevel */,
    Bool		/* showImplicit */,
    XkbFileAddOnFunc	/* addOn */,
    void *		/* priv */
);

extern	Bool	XkbWriteXKBCompatMap(
    FILE *		/* file */,
    XkbFileInfo *	/* result */,
    Bool		/* topLevel */,
    Bool		/* showImplicit */,
    XkbFileAddOnFunc	/* addOn */,
    void *		/* priv */
);

extern	Bool	XkbWriteXKBSymbols(
    FILE *		/* file */,
    XkbFileInfo *	/* result */,
    Bool		/* topLevel */,
    Bool		/* showImplicit */,
    XkbFileAddOnFunc	/* addOn */,
    void *		/* priv */
);

extern	Bool	XkbWriteXKBGeometry(
    FILE *		/* file */,
    XkbFileInfo *	/* result */,
    Bool		/* topLevel */,
    Bool		/* showImplicit */,
    XkbFileAddOnFunc	/* addOn */,
    void *		/* priv */
);

extern	Bool	XkbWriteXKBSemantics(
    FILE *		/* file */,
    XkbFileInfo *	/* result */,
    Bool		/* topLevel */,
    Bool		/* showImplicit */,
    XkbFileAddOnFunc	/* addOn */,
    void *		/* priv */
);

extern	Bool	XkbWriteXKBLayout(
    FILE *		/* file */,
    XkbFileInfo *	/* result */,
    Bool		/* topLevel */,
    Bool		/* showImplicit */,
    XkbFileAddOnFunc	/* addOn */,
    void *		/* priv */
);

extern	Bool	XkbWriteXKBKeymap(
    FILE *		/* file */,
    XkbFileInfo *	/* result */,
    Bool		/* topLevel */,
    Bool		/* showImplicit */,
    XkbFileAddOnFunc	/* addOn */,
    void *		/* priv */
);

extern	Bool	XkbWriteXKBFile(
    FILE *		/* file */,
    XkbFileInfo *	/* result */,
    Bool		/* showImplicit */,
    XkbFileAddOnFunc	/* addOn */,
    void *		/* priv */
);

extern	Bool	XkbWriteCFile(
    FILE *		/* file */,
    char *		/* name */,
    XkbFileInfo *	/* info */
);

extern	Bool	XkbWriteXKMFile(
    FILE *		/* file */,
    XkbFileInfo *	/* result */
);

extern	Bool	XkbWriteToServer(
    XkbFileInfo *	/* result */
);

extern	void	XkbEnsureSafeMapName(
    char *		/* name */
);

extern	Bool	XkbWriteXKBKeymapForNames(
    FILE *			/* file */,
    XkbComponentNamesPtr	/* names */,
    Display *			/* dpy */,
    XkbDescPtr			/* xkb */,
    unsigned			/* want */,
    unsigned			/* need */
);

extern	Status	XkbMergeFile(
    XkbDescPtr			/* xkb */,
    XkbFileInfo			/* finfo */
);

/***====================================================================***/

extern Bool	XkmProbe(
    FILE *		/* file */
);

extern unsigned XkbReadFromServer(
    Display *		/* dpy */,
    unsigned		/* need */,
    unsigned		/* want */,
    XkbFileInfo *	/* result */
);

extern unsigned	XkmReadFile(
    FILE *		/* file */,
    unsigned		/* need */,
    unsigned		/* want */,
    XkbFileInfo *	/* result */
);

#ifdef _XKMFORMAT_H_

extern Bool	XkmReadTOC(
    FILE *              /* file */,
    xkmFileInfo *       /* file_info */,
    int                 /* max_toc */,
    xkmSectionInfo *    /* toc */
);

extern xkmSectionInfo *XkmFindTOCEntry(
    xkmFileInfo *       /* finfo */,
    xkmSectionInfo *    /* toc */,
    unsigned            /* type */
);

extern Bool	XkmReadFileSection(
    FILE *              /* file */,
    xkmSectionInfo *    /* toc */,
    XkbFileInfo *       /* result */,
    unsigned *          /* loaded_rtrn */
);

extern char *	XkmReadFileSectionName(
    FILE *		/* file */,
    xkmSectionInfo *	/* toc */
);

#endif /* _XKMFORMAT_H  */

_XFUNCPROTOEND

#endif /* _XKBFILE_H_ */
