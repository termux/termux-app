/* $Xorg: Print.h,v 1.3 2000/08/18 04:05:44 coskrey Exp $ */
/******************************************************************************
 ******************************************************************************
 **
 ** File:         Print.h
 **
 ** Description:  Definitions needed by the server, library, and
 **               clients.  Subportion restricted to library and
 **               clients.
 **
 **               Server, Library, Client portion has:
 **                  o All sz_* defines
 **                  o Revision and Name defines
 **                  o Common defines and constants (e.g. Keywords, Masks)
 **                  o Extension version structure
 **
 **               Library and client subportion has:
 **                  o Convenience Macros
 **                  o Client side data structures
 **                  o Client side event structures (non wire)
 **                  o Library function prototypes
 **                  o some private stuff denoted with _whatever
 **
 **               Printstr.h for server and library, but NOT clients.
 **
 ******************************************************************************
 **
 ** (c) Copyright 1996 Hewlett-Packard Company
 ** (c) Copyright 1996 International Business Machines Corp.
 ** (c) Copyright 1996, Oracle and/or its affiliates.
 ** (c) Copyright 1996 Novell, Inc.
 ** (c) Copyright 1996 Digital Equipment Corp.
 ** (c) Copyright 1996 Fujitsu Limited
 ** (c) Copyright 1996 Hitachi, Ltd.
 **
 ** Permission is hereby granted, free of charge, to any person obtaining a copy
 ** of this software and associated documentation files (the "Software"), to deal
 ** in the Software without restriction, including without limitation the rights
 ** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 ** copies of the Software, and to permit persons to whom the Software is
 ** furnished to do so, subject to the following conditions:
 **
 ** The above copyright notice and this permission notice shall be included in
 ** all copies or substantial portions of the Software.
 **
 ** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 ** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 ** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 ** COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 ** IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 ** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **
 ** Except as contained in this notice, the names of the copyright holders shall
 ** not be used in advertising or otherwise to promote the sale, use or other
 ** dealings in this Software without prior written authorization from said
 ** copyright holders.
 **
 ******************************************************************************
 *****************************************************************************/
/* $XFree86: xc/include/extensions/Print.h,v 1.4 2000/01/25 18:37:31 dawes Exp $ */

#ifndef _XpPrint_H_
#define _XpPrint_H_

#ifndef _XP_PRINT_SERVER_
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xauth.h>
#endif /* _XP_PRINT_SERVER_ */

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

/******************************************************************************
 *
 * Definitions used by the server, library and client.
 */

/********************************************************************
 *
 * Naming and versioning information.
 */
#define XP_PRINTNAME  "XpExtension"

/*
 * Add a define below for each major extension release.
 */
#define XP_DONT_CHECK		0
#define XP_INITIAL_RELEASE	1

/*
 * For each single entry above, create one major/minor pair.
 */
#define XP_PROTO_MAJOR		1
#define XP_PROTO_MINOR		0

/*
 * Identify current version.
 */
#define XP_MAJOR_VERSION	XP_PROTO_MAJOR
#define XP_MINOR_VERSION	XP_PROTO_MINOR

/*
 * Misc version defines.
 */
#define XP_ABSENT		0	/* Prior to XP Print support */
#define XP_PRESENT		1	/* With XP Print support */

/********************************************************************
 *
 * Xp Print Error codes.
 */
#define XP_ERRORS		3	/* number of error types */

#define XPBadContext		0	/* Print Context invalid or missing */
#define XPBadSequence		1	/* Illegal sequence of XP operations */
#define XPBadResourceID		2	/* X-resource not valid */

/********************************************************************
 *
 * Xp Print Event masks and codes.
 *
 */
#define XP_EVENTS		2	/* number of event types */

#define XPNoEventMask		0	/* not an event - just a null mask */
#define XPPrintMask		(1L<<0)
#define XPAttributeMask		(1L<<1)

#define XPPrintNotify		0	/* contains "detail" - see below */
#define XPAttributeNotify	1	/* contains "detail" - see below */

#define XPStartJobNotify	0	/* value for "detail" in XPPrintNotify*/
#define XPEndJobNotify		1
#define XPStartDocNotify	2
#define XPEndDocNotify		3
#define XPStartPageNotify	4
#define XPEndPageNotify		5

/********************************************************************
 *
 * Xp Print Attribute Object codes (subset of ISO DPA 10175).  The
 * Xp Server can get and set any of the values, while the Xp Library
 * may only be able to set a subset of the attribute objects.
 *
 * note: the codes are also used as "detail" for XPAttributeNotify
 *
 * note: XPPageAttr is not defined in ISO DPA 10175.  It is unique
 * to Xp, and its attributes are a proper subset of XPDocAttr.
 */
typedef unsigned char XPAttributes;	/* type of Xp*Attr codes */

#define XP_ATTRIBUTES		5	/* those attrs currently supported */

#define XPJobAttr		1	/* get/set */
#define XPDocAttr		2	/* get/set */
#define XPPageAttr		3	/* get/set - subset of XPDocAttr */
#define XPPrinterAttr		4	/* get only (library) */
#define XPServerAttr		5	/* get only (library), no
					   context needed */

/*
 * note: ISO DPA 10175 defines a number of "attribute objects", of
 *       which POSIX 1387.4 and the SI Xp will only support a
 *       subset.
 */
#define XPMediumAttr		6	/* DPA-Object Medium */
#define XPFontAttr		7	/* DPA-Object Font */
#define XPResAttr		8	/* DPA-Object Resource */
#define XPTransAttr		9	/* DPA-Object Transfer method */
#define XPDelAttr		10	/* DPA-Object Delivery method */
#define XPAuxSPkg		11	/* DPA-Object Auxiliary sheet package */
#define XPAuxS			12	/* DPA-Object Auxiliary sheet */
#define XPFinishAttr		13	/* DPA-Object Finishing */
#define XPOutputAttr		14	/* DPA-Object Output method */
#define XPImpAttr		15	/* DPA-Object Imposition */
#define XPSchedAttr		16	/* DPA-Object Scheduler */
#define XPIntJobAttr		17	/* DPA-Object Initial value job */
#define XPIntDocAttr		18	/* DPA-Object Initial value document */
#define XPResConAttr		19	/* DPA-Object Resource context */


/*
 * Replacement rules for XpSetAttributes
 */
typedef unsigned char XPAttrReplacement;
#define	XPAttrReplace		1
#define XPAttrMerge		2


/*
 * Return codes for XpGetDocumentData
 */
typedef unsigned char XPGetDocStatus;
#define XPGetDocFinished	0	/* normal termination */
#define XPGetDocSecondConsumer	1	/* setup error */
#define XPGetDocError		2	/* runtime error, see generated error */


/*
 * Save data types for XpStartJob.
 */
typedef unsigned char XPSaveData;
#define XPSpool			1	/* Job data sent to spooler */
#define XPGetData		2	/* Job data via XpGetDocumentData */


/*
 * Document types for XpStartDoc.
 */
typedef unsigned char XPDocumentType;
#define	XPDocNormal		1	/* Doc data handled by Xserver */
#define	XPDocRaw		2	/* Doc data passed through Xserver */


/********************************************************************
 *
 * Xp Print Property Names
 */


#ifndef _XP_PRINT_SERVER_

/******************************************************************************
 *
 * Definitions used by the library and clients only.
 */

/*******************************************************************
 *
 * General API defines and such.
 */

/*
 * Print Context for XpInitContext and related calls.
 */
typedef XID XPContext;

/*
 * Struct for XpGetPrinterList.
 */
typedef struct {
    char	*name;		/* name */
    char	*desc;		/* localized description */
} XPPrinterRec, *XPPrinterList;

/*
 * Typedefs for XpGetDocumentData
 */
typedef void (*XPSaveProc)( Display *display,
                            XPContext context,
                            unsigned char *data,
                            unsigned int data_len,
                            XPointer client_data);

typedef void (*XPFinishProc)( Display *display,
                              XPContext context,
                              XPGetDocStatus status,
                              XPointer client_data);

/*
 * Typedefs for XpSetLocaleHinter and XpGetLocaleHinter
 */
typedef char * (*XPHinterProc)(void);

#if 0
/*******************************************************************
 *
 * Extension version structures.
 *
 **** this structure is now defined locally in the one file that uses it
 **** in order to avoid clashes with its definition in XI.h
 */
typedef struct {
        int     present;
        short   major_version;
        short   minor_version;
} XExtensionVersion;
#endif

/********************************************************************
 *
 * Event structs for clients.
 *
 * note: these events are relative to a print context, and
 * not to a window as in core X.
 */
typedef struct {
    int            type;       /* base + XPPrintNotify */
    unsigned long  serial;     /* # of last request processed by server */
    Bool           send_event; /* true if from a SendEvent request */
    Display        *display;   /* Display the event was read from */
    XPContext      context;    /* print context where operation was requested */
    Bool           cancel;     /* was detailed event canceled */
    int            detail;     /* XPStartJobNotify, XPEndJobNotify,
                                  XPStartDocNotify, XPEndDocNotify,
                                  XPStartPageNotify, XPEndPageNotify */
} XPPrintEvent;

typedef struct {
    int            type;       /* base + XPAttributeNotify */
    unsigned long  serial;     /* # of last request processed by server */
    Bool           send_event; /* true if from a SendEvent request */
    Display        *display;   /* Display the event was read from */
    XPContext      context;    /* print context where operation was requested */
    int            detail;     /* XPJobAttr, XPDocAttr, XPPageAttr,
                                  XPPrinterAttr, XPSpoolerAttr,
                                  XPMediumAttr, XPServerAttr */
} XPAttributeEvent;

typedef struct {
    int            type;       /* base + XPDataReadyNotify */
    unsigned long  serial;     /* # of last request processed by server */
    Bool           send_event; /* true if from a SendEvent request */
    Display        *display;   /* Display the event was read from */
    XPContext      context;    /* print context where operation was requested */
    unsigned long  available;  /* bytes available for retrieval */
} XPDataReadyEvent;


/**********************************************************
 *
 * Function prototypes for library side.
 */

extern XPContext XpCreateContext (
    Display		*display,
    char		*printer_name
);

extern void XpSetContext (
    Display		*display,
    XPContext     	print_context
);

extern XPContext XpGetContext (
    Display		*display
);

extern void XpDestroyContext (
    Display		*display,
    XPContext     	print_context
);

extern Screen *XpGetScreenOfContext (
    Display		*display,
    XPContext     	print_context
);

extern Status XpGetPageDimensions (
    Display		*display,
    XPContext     	print_context,
    unsigned short	*width,			/* return value */
    unsigned short	*height,		/* return value */
    XRectangle		*reproducible_area	/* return value */
);

extern void XpStartJob (
    Display		*display,
    XPSaveData		save_data
);

extern void XpEndJob (
    Display		*display
);

extern void XpCancelJob (
    Display		*display,
    Bool		discard
);

extern void XpStartDoc (
    Display		*display,
    XPDocumentType	type
);

extern void XpEndDoc (
    Display		*display
);

extern void XpCancelDoc (
    Display		*display,
    Bool		discard
);

extern void XpPutDocumentData (
    Display		*display,
    Drawable		drawable,
    unsigned char	*data,
    int			data_len,
    char		*doc_fmt,
    char		*options
);

extern Status XpGetDocumentData (
    Display		*display,
    XPContext		context,
    XPSaveProc		save_proc,
    XPFinishProc	finish_proc,
    XPointer		client_data
);

extern void XpStartPage (
    Display		*display,
    Window		window
);

extern void XpEndPage (
    Display		*display
);

extern void XpCancelPage (
    Display		*display,
    Bool		discard
);

extern void XpSelectInput (
    Display		*display,
    XPContext     	print_context,
    unsigned long	event_mask
);

extern unsigned long XpInputSelected (
    Display		*display,
    XPContext     	print_context,
    unsigned long	*all_events_mask
);

extern Bool XpSetImageResolution (
    Display		*display,
    XPContext     	print_context,
    int			image_res,
    int			*prev_res
);

extern int XpGetImageResolution (
    Display		*display,
    XPContext     	print_context
);

extern char *XpGetAttributes (
    Display		*display,
    XPContext     	print_context,
    XPAttributes	type
);

extern void XpSetAttributes (
    Display		*display,
    XPContext     	print_context,
    XPAttributes	type,
    char		*pool,
    XPAttrReplacement	replacement_rule
);

extern char *XpGetOneAttribute (
    Display		*display,
    XPContext     	print_context,
    XPAttributes	type,
    char		*attribute_name
);

extern XPPrinterList XpGetPrinterList (
    Display		*display,
    char		*printer_name,
    int			*list_count		/* return value */
);

extern void XpFreePrinterList (
    XPPrinterList	printer_list
);

extern void XpRehashPrinterList (
    Display		*display
);

extern Status XpQueryVersion (
    Display		*display,
    short		*major_version,		/* return value */
    short		*minor_version		/* return value */
);

extern Bool XpQueryExtension (
    Display		*display,
    int			*event_base_return,	/* return value */
    int			*error_base_return	/* return value */
);

extern Screen **XpQueryScreens (
    Display		*display,
    int			*list_count		/* return value */
);

extern Status XpGetPdmStartParams (
    Display		*print_display,
    Window		print_window,
    XPContext		print_context,
    Display		*video_display,
    Window		video_window,
    Display		**selection_display,	/* return value */
    Atom		*selection,		/* return value */
    Atom		*type,			/* return value */
    int			*format,		/* return value */
    unsigned char	**data,			/* return value */
    int			*nelements		/* return value */
);

extern Status XpGetAuthParams (
    Display		*print_display,
    Display		*video_display,
    Display		**selection_display,	/* return value */
    Atom		*selection,		/* return value */
    Atom		*target			/* return value */
);

extern Status XpSendAuth (
    Display		*display,
    Window		window
);

extern Status XpSendOneTicket (
    Display		*display,
    Window		window,
    Xauth		*ticket,
    Bool		more
);

extern void XpSetLocaleHinter (
    XPHinterProc hinter_proc,
    char         *hinter_desc
);

extern char *XpGetLocaleHinter (
    XPHinterProc *hinter_proc
);

extern char *XpGetLocaleNetString(void);

extern char *XpNotifyPdm (
    Display		*print_display,
    Window		print_window,
    XPContext     	print_context,
    Display		*video_display,
    Window		video_window,
    Bool		auth_flag
);

#endif /* _XP_PRINT_SERVER_ */

_XFUNCPROTOEND

#endif /* _XpPrint_H_ */
