/* $Xorg: Printstr.h,v 1.3 2000/08/18 04:05:44 coskrey Exp $ */
/******************************************************************************
 ******************************************************************************
 **
 ** File:         Printstr.h
 **
 ** Description: Definitions needed by the server and library, but
 **              not clients.
 **
 **              Print.h for server, library and clients.
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
/* $XFree86: xc/include/extensions/Printstr.h,v 1.5 2001/08/01 00:44:35 tsi Exp $ */


#ifndef _XpPrintstr_H_
#define _XpPrintstr_H_

/*
 * NEED_EVENTS and NEED_REPLIES are hacks to limit the linker symbol-table
 * size.   When function prototypes are needed from Print.h, this sets up
 * a cascading dependency on Printstr.h and eventually Xproto.h to provide
 * the event and reply struct definitions.
 */
#ifndef NEED_EVENTS
#define NEED_EVENTS
#endif /* NEED_EVENTS */

#define NEED_REPLIES

#include <X11/Xproto.h>
#ifndef _XP_PRINT_SERVER_
#include <X11/Xlib.h>
#endif /* _XP_PRINT_SERVER_ */

/*
 * Pull in other definitions.  Print.h will hide some things if we're
 * doing server side work.
 */
#include <X11/extensions/Print.h>

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

/******************************************************************************
 *
 * Protocol requests constants and alignment values
 *
 * Note: Xlib macro's expect X_ABC where ABC is the name of the
 * protocol request.
 */
#define X_PrintQueryVersion		0
#define X_PrintGetPrinterList		1
#define X_PrintCreateContext		2
#define X_PrintSetContext		3
#define X_PrintGetContext		4
#define X_PrintDestroyContext		5
#define X_PrintGetContextScreen		6
#define X_PrintStartJob			7
#define X_PrintEndJob			8
#define X_PrintStartDoc			9
#define X_PrintEndDoc			10
#define X_PrintPutDocumentData		11
#define X_PrintGetDocumentData		12
#define X_PrintStartPage		13
#define X_PrintEndPage			14
#define X_PrintSelectInput		15
#define X_PrintInputSelected		16
#define X_PrintGetAttributes		17
#define X_PrintSetAttributes		18
#define X_PrintGetOneAttribute		19
#define X_PrintRehashPrinterList	20
#define X_PrintGetPageDimensions	21
#define X_PrintQueryScreens		22
#define X_PrintSetImageResolution	23
#define X_PrintGetImageResolution	24

/********************************************************************
 *
 * Protocol data types
 */
#define PCONTEXT CARD32
#define WINDOW   CARD32
#define DRAWABLE CARD32
#define BITMASK  CARD32

/******************************************************************************
 *
 * Event wire struct definitions
 *
 * Note: Xlib macro's expect xABC struct names and sz_xABC size
 * constants where ABC is the name of the protocol request.
 */


/*********************************************************************
 *
 * Events.
 *
 * See Print.h for the protocol "type" values.
 */
typedef struct _xPrintPrintEvent {
	BYTE type;		/* XPPrintNotify + extEntry->eventBase */
	BYTE detail;		/* XPStartJobNotify, XPEndJobNotify,
				   XPStartDocNotify, XPEndDocNotify,
				   XPStartPageNotify, XPEndPageNotify */
	CARD16 sequenceNumber;
	PCONTEXT printContext;	/* print context */
	BOOL   cancel;		/* canceled flag */
	CARD8  pad1;		/* rest is unused */
	CARD16 pad2;
	CARD32 pad3;
	CARD32 pad4;
	CARD32 pad5;
	CARD32 pad6;
	CARD32 pad7;
} xPrintPrintEvent;
#define sz_xPrintPrintEvent 32;

typedef struct _xPrintAttributeEvent {
	BYTE   type;		/* XPAttributeNotify + extEntry->eventBase */
	BYTE   detail;		/* XPJobAttr, XPDocAttr, XPPageAttr,
				   XPPrinterAttr, XPSpoolerAttr,
				   XPMediumAttr, XPServerAttr */
	CARD16 sequenceNumber;
	PCONTEXT printContext;	/* print context */
	CARD32 pad1;
	CARD32 pad2;
	CARD32 pad3;
	CARD32 pad4;
	CARD32 pad5;
	CARD32 pad6;
} xPrintAttributeEvent;
#define sz_xPrintAttributeEvent 32;


/*********************************************************************
 *
 * Requests
 */
typedef struct _PrintQueryVersion {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintQueryVersion */
	CARD16	length;
} xPrintQueryVersionReq;
#define sz_xPrintQueryVersionReq	4

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;			/* not used */
	CARD16	sequenceNumber;
	CARD32	length;
	CARD16	majorVersion;		/* major version of Xp protocol */
	CARD16	minorVersion;		/* minor version of Xp protocol */
	CARD32	pad1;
	CARD32	pad2;
	CARD32	pad3;
	CARD32	pad4;
	CARD32	pad5;
} xPrintQueryVersionReply;
#define sz_xPrintQueryVersionReply	32


typedef struct _PrintGetPrinterList {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintGetPrinterList */
	CARD16	length;
	CARD32	printerNameLen;		/* length of printer name */
	CARD32	localeLen;		/* length of locale string */

	/* variable portion *****************************************
	STRING8	printerName;		 * printer name *
	BYTE	pad(printerNameLen)	 * unused *
	STRING8	locale;			 * locale *
	BYTE	pad(localeLen)		 * unused *
	************************************************************/
} xPrintGetPrinterListReq;
#define sz_xPrintGetPrinterListReq	12

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;			/* not used */
	CARD16	sequenceNumber;
	CARD32	length;
	CARD32	listCount;		/* of PRINTER recs below */
	CARD32	pad1;
	CARD32	pad2;
	CARD32	pad3;
	CARD32	pad4;
	CARD32	pad5;

	/* variable portion *****************************************
	CARD32	nameLen;		* length of name in bytes *
	STRING8	name;			* name *
	BYTE	pad(nameLen)		* unused *

	CARD32	descLen;		* length of desc in bytes *
	STRING8	desc;			* localized description *
	BYTE	pad(descLen)		* unused *
	************************************************************/
} xPrintGetPrinterListReply;
#define sz_xPrintGetPrinterListReply	32


typedef struct _PrintRehashPrinterList {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintRehashPrinterList */
	CARD16	length;
} xPrintRehashPrinterListReq;
#define sz_xPrintRehashPrinterListReq	4


typedef struct _PrintCreateContext {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintInitSetContext */
	CARD16	length;
	CARD32	contextID;		/* ID for context */
	CARD32	printerNameLen;		/* length of printerName in bytes */
	CARD32	localeLen;		/* length of locale in bytes */

	/* variable portion *****************************************
	STRING8	printerName		 * printer name *
	BYTE	pad(printerNameLen)	 * unused *
	STRING8	locale			 * locale *
	BYTE	pad(locale)		 * unused *
	************************************************************/
} xPrintCreateContextReq;
#define sz_xPrintCreateContextReq	16


typedef struct _PrintSetContext {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintSetContext */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
} xPrintSetContextReq;
#define sz_xPrintSetContextReq		8


typedef struct _PrintGetContext {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintGetContext */
	CARD16	length;
} xPrintGetContextReq;
#define sz_xPrintGetContextReq		4

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;			/* not used */
	CARD16	sequenceNumber;
	CARD32	length;
	PCONTEXT printContext;		/* print context */
	CARD32	pad1;
	CARD32	pad2;
	CARD32	pad3;
	CARD32	pad4;
	CARD32	pad5;
} xPrintGetContextReply;
#define sz_xPrintGetContextReply	32


typedef struct _PrintDestroyContext {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintDestroyContext */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
} xPrintDestroyContextReq;
#define sz_xPrintDestroyContextReq	8


typedef struct _PrintGetContextScreen {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintGetContextScreen */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
} xPrintGetContextScreenReq;
#define sz_xPrintGetContextScreenReq	8

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;			/* not used */
	CARD16	sequenceNumber;
	CARD32	length;
	WINDOW  rootWindow;		/* screenPtr represented as rootWin */
	CARD32	pad1;
	CARD32	pad2;
	CARD32	pad3;
	CARD32	pad4;
	CARD32	pad5;
} xPrintGetContextScreenReply;
#define sz_xPrintGetContextScreenReply	32


typedef struct _PrintStartJob {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintStartJob */
	CARD16	length;
	CARD8	saveData;		/* save data boolean */
	CARD8	pad1;
	CARD16	pad2;
} xPrintStartJobReq;
#define sz_xPrintStartJobReq		8

typedef struct _PrintEndJob {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintEndJob */
	CARD16	length;
	BOOL	cancel;			/* cancel boolean */
	CARD8	pad1;
	CARD16	pad2;
} xPrintEndJobReq;
#define sz_xPrintEndJobReq		8


typedef struct _PrintStartDoc {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintStartDoc */
	CARD16	length;
	CARD8	type;			/* type for document */
	CARD8	pad1;
	CARD16	pad2;
} xPrintStartDocReq;
#define sz_xPrintStartDocReq		8

typedef struct _PrintEndDoc {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintEndDoc */
	CARD16	length;
	BOOL	cancel;			/* cancel boolean */
	CARD8	pad1;
	CARD16	pad2;
} xPrintEndDocReq;
#define sz_xPrintEndDocReq		8


typedef struct _PrintPutDocumentData {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintPutDocumentData */
	CARD16	length;
	DRAWABLE drawable;		/* target drawable */
	CARD32	len_data;		/* big len in bytes */
	CARD16	len_fmt;		/* len in bytes */
	CARD16	len_options;		/* len in bytes */

	/* variable portion *****************************************
	LISTofBYTE	data;		 * data *
	BYTE		pad(len_data)	 * unused *
	STRING8		doc_fmt;	 * ISO compliant desc of data type *
	BYTE		pad(len_fmt)	 * unused *
	STRING8		options;	 * additional device-dependent desc *
	BYTE		pad(len_options) * unused *
	************************************************************/
} xPrintPutDocumentDataReq;
#define sz_xPrintPutDocumentDataReq	16


typedef struct _PrintGetDocumentData {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintGetDocumentData */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
	CARD32	maxBufferSize;		/* maximum buffer size requested */
} xPrintGetDocumentDataReq;
#define sz_xPrintGetDocumentDataReq	12

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;			/* not used */
	CARD16	sequenceNumber;
	CARD32	length;
	CARD32	statusCode;		/* status code for reply */
	CARD32	finishedFlag;		/* is this the last reply */
	CARD32	dataLen;		/* data length */
	CARD32	pad1;
	CARD32	pad2;
	CARD32	pad3;

	/* variable portion *****************************************
	LISTofBYTE	data;		 * data *
	BYTE		pad(count)	 * unused *
	************************************************************/
} xPrintGetDocumentDataReply;
#define sz_xPrintGetDocumentDataReply	32


typedef struct _PrintStartPage {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintStartPage */
	CARD16	length;
	WINDOW	window;			/* window */
} xPrintStartPageReq;
#define sz_xPrintStartPageReq		8

typedef struct _PrintEndPage {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintEndPage */
	CARD16	length;
	BOOL	cancel;			/* cancel boolean */
	CARD8	pad1;
	CARD16	pad2;
} xPrintEndPageReq;
#define sz_xPrintEndPageReq		8


typedef struct _PrintSelectInput {
        CARD8   reqType;        	/* always PrintReqCode */
	CARD8   printReqType;		/* always X_PrintSelectInput */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
	BITMASK	eventMask;
} xPrintSelectInputReq;
#define sz_xPrintSelectInputReq		12


typedef struct _PrintInputSelected {
        CARD8   reqType;        	/* always PrintReqCode */
	CARD8   printReqType;		/* always X_PrintInputSelected */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
} xPrintInputSelectedReq;
#define sz_xPrintInputSelectedReq	8

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;			/* not used */
	CARD16	sequenceNumber;
	CARD32	length;
	BITMASK	eventMask;		/* your event mask */
	BITMASK	allEventsMask;		/* all event mask */
	CARD32	pad1;
	CARD32	pad2;
	CARD32	pad3;
	CARD32	pad4;
} xPrintInputSelectedReply;
#define sz_xPrintInputSelectedReply	32

typedef struct _PrintGetAttributes {
        CARD8   reqType;        	/* always PrintReqCode */
	CARD8   printReqType;		/* always X_PrintGetAttributes */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
        CARD8   type;			/* type */
        CARD8   pad1;			/* unused */
        CARD16  pad2;			/* unused */
} xPrintGetAttributesReq;
#define sz_xPrintGetAttributesReq	12

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;			/* not used */
	CARD16	sequenceNumber;
	CARD32	length;
	CARD32	stringLen;		/* length of xrm db string */
	CARD32	pad1;
	CARD32	pad2;
	CARD32	pad3;
	CARD32	pad4;
	CARD32	pad5;

        /* variable portion *****************************************
	STRING8	string;                  * xrm db as a string *
	BYTE	pad(stringLen)           * unused *
        ************************************************************/
} xPrintGetAttributesReply;
#define sz_xPrintGetAttributesReply	32


typedef struct _PrintSetAttributes {
        CARD8   reqType;        	/* always PrintReqCode */
	CARD8   printReqType;		/* always X_PrintSetAttributes */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
	CARD32	stringLen;		/* length of xrm db string */
        CARD8   type;                   /* type */
	CARD8   rule;			/* replacement rule */
	CARD16  pad1;			/* unused */

        /* variable portion *****************************************
	STRING8	string;                  * xrm db as a string *
	BYTE	pad(stringLen)           * unused *
        ************************************************************/
} xPrintSetAttributesReq;
#define sz_xPrintSetAttributesReq	16


typedef struct _PrintGetOneAttribute {
        CARD8   reqType;        	/* always PrintReqCode */
	CARD8   printReqType;		/* always X_PrintGetOneAttribute */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
	CARD32	nameLen;		/* length of name string */
        CARD8   type;			/* type */
        CARD8   pad1;			/* unused */
        CARD16  pad2;			/* unused */

        /* variable portion *****************************************
	STRING8	name;			 * name as a string *
	BYTE	pad(name)		 * unused *
        ************************************************************/
} xPrintGetOneAttributeReq;
#define sz_xPrintGetOneAttributeReq	16

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;			/* not used */
	CARD16	sequenceNumber;
	CARD32	length;
	CARD32	valueLen;		/* length of value string */
	CARD32	pad1;
	CARD32	pad2;
	CARD32	pad3;
	CARD32	pad4;
	CARD32	pad5;

        /* variable portion *****************************************
	STRING8	value;			 * value as a string *
	BYTE	pad(value)		 * unused *
        ************************************************************/
} xPrintGetOneAttributeReply;
#define sz_xPrintGetOneAttributeReply	32


typedef struct _PrintGetPageDimensions {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintGetPageDimensions */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
} xPrintGetPageDimensionsReq;
#define sz_xPrintGetPageDimensionsReq	8

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;			/* not used */
	CARD16	sequenceNumber;
	CARD32	length;
	CARD16	width;			/* total pixel width */
	CARD16	height;			/* total pixel height */
	CARD16	rx;			/* reproducible x pixel offset */
	CARD16	ry;			/* reproducible y pixel offset */
	CARD16	rwidth;			/* reproducible x pixel width */
	CARD16	rheight;		/* reproducible y pixel width */
	CARD32	pad1;
	CARD32	pad2;
	CARD32	pad3;
} xPrintGetPageDimensionsReply;
#define sz_xPrintGetPageDimensionsReply	32


typedef struct _PrintQueryScreens {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintQueryScreens */
	CARD16	length;
} xPrintQueryScreensReq;
#define sz_xPrintQueryScreensReq	4

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;			/* not used */
	CARD16	sequenceNumber;
	CARD32	length;
	CARD32	listCount;		/* number of screens following */
	CARD32	pad1;
	CARD32	pad2;
	CARD32	pad3;
	CARD32	pad4;
	CARD32	pad5;

        /* variable portion *****************************************
	WINDOW	rootWindow;		 * root window of screen *
        ************************************************************/
} xPrintQueryScreensReply;
#define sz_xPrintQueryScreensReply	32

typedef struct _PrintSetImageResolution {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintSetImageResolution */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
	CARD16 imageRes;		/* image resolution */
	CARD16 pad1;
} xPrintSetImageResolutionReq;
#define sz_xPrintSetImageResolutionReq	12

typedef struct {
	BYTE	type;			/* X_Reply */
	BOOL	status;			/* accepted or not */
	CARD16	sequenceNumber;
	CARD32	length;
	CARD16	prevRes;		/* previous resolution */
	CARD16	pad1;
	CARD32	pad2;
	CARD32	pad3;
	CARD32	pad4;
	CARD32	pad5;
	CARD32	pad6;
} xPrintSetImageResolutionReply;
#define sz_xPrintSetImageResolutionReply 32

typedef struct _PrintGetImageResolution {
	CARD8	reqType;		/* always PrintReqCode */
	CARD8	printReqType;		/* always X_PrintGetImageResolution */
	CARD16	length;
	PCONTEXT printContext;		/* print context */
} xPrintGetImageResolutionReq;
#define sz_xPrintGetImageResolutionReq	8

typedef struct {
	BYTE	type;			/* X_Reply */
	CARD8	unused;
	CARD16	sequenceNumber;
	CARD32	length;
	CARD16	imageRes;		/* image resolution */
	CARD16	pad1;
	CARD32	pad2;
	CARD32	pad3;
	CARD32	pad4;
	CARD32	pad5;
	CARD32	pad6;
} xPrintGetImageResolutionReply;
#define sz_xPrintGetImageResolutionReply 32

#ifndef _XP_PRINT_SERVER_
/***********************************************************************
 *
 * Library-only definitions.
 */
extern XPHinterProc  _xp_hinter_proc;
extern char         *_xp_hinter_desc;
extern int           _xp_hinter_init;

#else /* _XP_PRINT_SERVER_ */

/***********************************************************************
 *
 * Server-only definitions shared between the extension and DDX layers.
 *
 */

/*
 * Internal return code used to indicate that the requesting
 * client has been suspended.
 */
#define Suspended 84

struct _XpContext;

extern void XpRegisterPrinterScreen(
    ScreenPtr pScreen,
    int (*CreateContext)(struct _XpContext *));

typedef struct _xpprintFuncs {
    int (*StartJob)(
	struct _XpContext *	/* pContext */,
	Bool			/* sendClientData */,
	ClientPtr		/* client */);
    int (*EndJob)(struct _XpContext *, int);
    int (*StartDoc)(
	struct _XpContext *	/* pContext */,
	XPDocumentType		/* type */);
    int (*EndDoc)(struct _XpContext *, int);
    int (*StartPage)(
	struct _XpContext *	/* pContext */,
	WindowPtr		/* pWin */);
    int (*EndPage)(
	struct _XpContext *	/* pContext */,
	WindowPtr		/* pWin */);
    int (*PutDocumentData)(
	struct _XpContext *	/* pContext */,
    	DrawablePtr		/* pDraw */,
	char *			/* pData */,
	int			/* len_data */,
	char *			/* pDoc_fmt */,
	int			/* len_fmt */,
	char *			/* pOptions */,
	int			/* len_options */,
	ClientPtr		/* client */);
    int (*GetDocumentData)(
	struct _XpContext *	/* pContext */,
	ClientPtr		/* client */,
	int			/* maxBufferSize */);
    int (*DestroyContext)(
	struct _XpContext *);	/* pContext */
    char *(*GetAttributes)(
	struct _XpContext *,
	XPAttributes 		/* pool */);
    char *(*GetOneAttribute)(
	struct _XpContext *	/* pContext */,
	XPAttributes 		/* pool */,
	char *			/* attrs */);
    int (*SetAttributes)(
	struct _XpContext *	/* pContext */,
	XPAttributes 		/* pool */,
	char *			/* attrs */);
    int (*AugmentAttributes)(
	struct _XpContext *	/* pContext */,
	XPAttributes 		/* pool */,
	char *			/* attrs */);
    int (*GetMediumDimensions)(
	struct _XpContext *	/* pPrintContext */,
	CARD16 *		/* pWidth */,
	CARD16 *		/* pHeight */);
    int (*GetReproducibleArea)(
	struct _XpContext *	/* pPrintContext */,
	xRectangle *		/* pRect */);
    int (*SetImageResolution)(
	struct _XpContext *	/* pPrintContext */,
	int			/* imageRes */,
	Bool *			/* pStatus */);
} XpDriverFuncs, *XpDriverFuncsPtr;

/*
 * Each print context is represented by one of the following structs
 * associated with a resource ID of type RTcontext .  A pointer to
 * the context is placed in the Xp extension's devPrivates
 * element in each client * which establishes a context via
 * either initContext or setContext.
 * The context pointer is also placed in the struct indicated by the
 * RTpage resource associated with each StartPage'd window.
 */
typedef struct _XpContext {
        XID contextID;
        char *printerName;
        int screenNum;          /* screen containing the printer */
        struct _XpClient *clientHead; /* list of clients */
        CARD32 state;
        VisualID pageWin;
        PrivateRec *devPrivates;
        XpDriverFuncs funcs;
	ClientPtr clientSlept;
	int imageRes;
} XpContextRec, *XpContextPtr;

#include <X11/fonts/fontstruct.h>	/* FontResolutionPtr */

extern FontResolutionPtr XpGetClientResolutions(ClientPtr, int *);
extern XpContextPtr XpContextOfClient(ClientPtr);
extern XpContextPtr XpGetPrintContext(ClientPtr);
extern int XpRehashPrinterList(void);
extern void XpSetFontResFunc(ClientPtr);
extern void XpUnsetFontResFunc(ClientPtr);
extern void XpRegisterInitFunc(ScreenPtr, char *, int (*)(struct _XpContext *));

#endif /* _XP_PRINT_SERVER_ */

_XFUNCPROTOEND

#endif /* _XpPrintstr_H_ */
