/*

Copyright 1991, 1998  The Open Group

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

/*
 * Copyright 1990, 1991 by OMRON Corporation, NTT Software Corporation,
 *                      and Nippon Telegraph and Telephone Corporation
 * Copyright 1991 by the Open Software Foundation
 * Copyright 1993 by the TOSHIBA Corp.
 * Copyright 1993, 1994 by Sony Corporation
 * Copyright 1993, 1994 by the FUJITSU LIMITED
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of OMRON, NTT Software, NTT, Open
 * Software Foundation, and Sony Corporation not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. OMRON, NTT Software, NTT, Open Software
 * Foundation, and Sony Corporation  make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * OMRON, NTT SOFTWARE, NTT, OPEN SOFTWARE FOUNDATION, AND SONY
 * CORPORATION DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT
 * SHALL OMRON, NTT SOFTWARE, NTT, OPEN SOFTWARE FOUNDATION, OR SONY
 * CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *	Authors: Li Yuhong		OMRON Corporation
 *		 Tatsuya Kato		NTT Software Corporation
 *		 Hiroshi Kuribayashi	OMRON Coproration
 *		 Muneiyoshi Suzuki	Nippon Telegraph and Telephone Co.
 *
 *		 M. Collins		OSF
 *		 Katsuhisa Yano		TOSHIBA Corp.
 *               Makoto Wakamatsu       Sony Corporation
 *               Takashi Fujiwara	FUJITSU LIMITED
 */


#ifndef	_XLCINT_H_
#define	_XLCINT_H_

#ifndef _XP_PRINT_SERVER_

#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <stdarg.h>

typedef Bool (*XFilterEventProc)(
    Display*		/* display */,
    Window		/* window */,
    XEvent*		/* event */,
    XPointer		/* client_data */
);

typedef struct _XIMFilter {
    struct _XIMFilter *next;
    Window window;
    unsigned long event_mask;
    int start_type, end_type;
    XFilterEventProc filter;
    XPointer client_data;
} XFilterEventRec, *XFilterEventList;

typedef struct {
    char    *name;
    XPointer value;
} XIMArg;

#ifdef offsetof
#define XOffsetOf(s_type,field) offsetof(s_type,field)
#else
#define XOffsetOf(s_type,field) ((unsigned int)&(((s_type*)NULL)->field))
#endif

#define XIMNumber(arr) ((unsigned int) (sizeof(arr) / sizeof(arr[0])))

/*
 * define secondary data structs which are part of Input Methods
 * and Input Context
 */
typedef struct {
    const char		*resource_name;		/* Resource string */
    XrmQuark		xrm_name;		/* Resource name quark */
    int			resource_size;		/* Size in bytes of data */
    long		resource_offset;	/* Offset from base */
    unsigned short 	mode;			/* Read Write Permission */
    unsigned short 	id;			/* Input Method Protocol */
} XIMResource, *XIMResourceList;

/*
 * data block describing the visual attributes associated with
 * an input context
 */
typedef struct {
    XRectangle		area;
    XRectangle		area_needed;
    XPoint		spot_location;
    Colormap		colormap;
    Atom		std_colormap;
    unsigned long	foreground;
    unsigned long	background;
    Pixmap		background_pixmap;
    XFontSet            fontset;
    int	       		line_spacing;
    Cursor		cursor;
    XICCallback		start_callback;
    XICCallback		done_callback;
    XICCallback		draw_callback;
    XICCallback		caret_callback;
    XIMPreeditState	preedit_state;
    XICCallback		state_notify_callback;
} ICPreeditAttributes, *ICPreeditAttributesPtr;

typedef struct {
    XRectangle		area;
    XRectangle		area_needed;
    Colormap		colormap;
    Atom		std_colormap;
    unsigned long	foreground;
    unsigned long	background;
    Pixmap		background_pixmap;
    XFontSet            fontset;
    int	       		line_spacing;
    Cursor		cursor;
    XICCallback		start_callback;
    XICCallback		done_callback;
    XICCallback		draw_callback;
} ICStatusAttributes, *ICStatusAttributesPtr;

#endif /* !_XP_PRINT_SERVER_ */

/*
 * Methods for Xrm parsing
 */

/* The state is a pointer to an object created by the locale's
   init_parse_info function (default: _XrmDefaultInitParseInfo). */

/* Sets the state to the initial state.
   Initiates a sequence of calls to the XmbCharProc. */
typedef void (*XmbInitProc)(
    XPointer		state
);

/* Transforms one multibyte character, starting at str, and return a 'char'
   in the same parsing class (not a wide character!). Returns the number of
   consumed bytes in *lenp. */
typedef char (*XmbCharProc)(
    XPointer		state,
    const char *	str,
    int*		lenp
);

/* Terminates a sequence of calls to the XmbCharProc. */
typedef void (*XmbFinishProc)(
    XPointer		state
);

/* Returns the name of the state's locale, as a static string. */
typedef const char* (*XlcNameProc)(
    XPointer		state
);

/* Frees the state, which was allocated by the locale's init_parse_info
   function. */
typedef void (*XrmDestroyProc)(
    XPointer		state
);

/* Set of methods for Xrm parsing. */
typedef struct {
    XmbInitProc		mbinit;
    XmbCharProc		mbchar;
    XmbFinishProc	mbfinish;
    XlcNameProc		lcname;
    XrmDestroyProc	destroy;
} XrmMethodsRec;
typedef const XrmMethodsRec *XrmMethods;

#ifndef _XP_PRINT_SERVER_

typedef struct _XLCd *XLCd; /* need forward reference */

/*
 * define an LC, it's methods, and data.
 */

typedef void (*XCloseLCProc)(
    XLCd		/* lcd */
);

typedef char* (*XlcMapModifiersProc)(
    XLCd		/* lcd */,
    _Xconst char*	/* user_mods */,
    _Xconst char*	/* prog_mods */
);

typedef XOM (*XOpenOMProc)(
    XLCd		/* lcd */,
    Display*		/* display */,
    XrmDatabase		/* rdb */,
    _Xconst char*	/* res_name */,
    _Xconst char*	/* res_class */
);

typedef XIM (*XOpenIMProc)(
    XLCd		/* lcd */,
    Display*		/* display */,
    XrmDatabase		/* rdb */,
    char*		/* res_name */,
    char*		/* res_class */
);

typedef Bool (*XRegisterIMInstantiateCBProc)(
    XLCd		/* lcd */,
    Display*		/* display */,
    XrmDatabase		/* rdb */,
    char*		/* res_name */,
    char*		/* res_class */,
    XIDProc		/* callback */,
    XPointer		/* client_data */
);

typedef Bool (*XUnregisterIMInstantiateCBProc)(
    XLCd		/* lcd */,
    Display*		/* display */,
    XrmDatabase		/* rdb */,
    char*		/* res_name */,
    char*		/* res_class */,
    XIDProc		/* callback */,
    XPointer		/* client_data */
);

typedef XrmMethods (*XrmInitParseInfoProc)(
    XLCd		/* lcd */,
    XPointer*		/* state */
);

typedef int (*XmbTextPropertyToTextListProc)(
    XLCd		lcd,
    Display*		display,
    const XTextProperty* text_prop,
    char***		list_return,
    int*		count_return
);

typedef int (*XwcTextPropertyToTextListProc)(
    XLCd		lcd,
    Display*		display,
    const XTextProperty* text_prop,
    wchar_t***		list_return,
    int*		count_return
);

typedef int (*XmbTextListToTextPropertyProc)(
    XLCd		lcd,
    Display*		display,
    char**		list,
    int			count,
    XICCEncodingStyle	style,
    XTextProperty*	text_prop_return
);

typedef int (*XwcTextListToTextPropertyProc)(
    XLCd		lcd,
    Display*		display,
    wchar_t**		list,
    int			count,
    XICCEncodingStyle	style,
    XTextProperty*	text_prop_return
);

typedef void (*XwcFreeStringListProc)(
    XLCd		lcd,
    wchar_t**		list
);

typedef const char* (*XDefaultStringProc)(
    XLCd		lcd
);

typedef struct {
    XCloseLCProc			close;
    XlcMapModifiersProc			map_modifiers;
    XOpenOMProc				open_om;
    XOpenIMProc				open_im;
    XrmInitParseInfoProc		init_parse_info;
    XmbTextPropertyToTextListProc	mb_text_prop_to_list;
    XwcTextPropertyToTextListProc	wc_text_prop_to_list;
    XmbTextPropertyToTextListProc	utf8_text_prop_to_list;
    XmbTextListToTextPropertyProc	mb_text_list_to_prop;
    XwcTextListToTextPropertyProc	wc_text_list_to_prop;
    XmbTextListToTextPropertyProc	utf8_text_list_to_prop;
    XwcFreeStringListProc		wc_free_string_list;
    XDefaultStringProc			default_string;
    XRegisterIMInstantiateCBProc	register_callback;
    XUnregisterIMInstantiateCBProc	unregister_callback;
} XLCdMethodsRec, *XLCdMethods;


typedef struct {
    char*		name;			/* name of this LC */
    char*		modifiers;		/* modifiers of locale */
} XLCdCoreRec, *XLCdCore;


typedef struct _XLCd {
    XLCdMethods		methods;		/* methods of this LC */
    XLCdCore		core;			/* data of this LC */
    XPointer		opaque;			/* LDX specific data */
} XLCdRec;

typedef int XlcPosition;

#define XlcHead		0
#define XlcTail		-1

typedef struct {
    char *name;
    XPointer value;
} XlcArg, *XlcArgList;

typedef struct _XlcResource {
    const char *name;
    XrmQuark xrm_name;
    int size;
    int offset;
    unsigned long mask;
} XlcResource, *XlcResourceList;

#define XlcCreateMask	(1L<<0)
#define XlcDefaultMask	(1L<<1)
#define XlcGetMask	(1L<<2)
#define XlcSetMask	(1L<<3)
#define XlcIgnoreMask	(1L<<4)

#define XlcNumber(arr)	(sizeof(arr) / sizeof(arr[0]))

typedef Status (*XCloseOMProc)(
    XOM			/* om */
);

typedef char* (*XSetOMValuesProc)(
    XOM			/* om */,
    XlcArgList		/* args */,
    int			/* num_args */
);

typedef char* (*XGetOMValuesProc)(
    XOM			/* om */,
    XlcArgList		/* args */,
    int			/* num_args */
);

typedef XOC (*XCreateOCProc)(
    XOM			/* om */,
    XlcArgList		/* args */,
    int			/* num_args */
);

typedef struct _XOMMethodsRec {
    XCloseOMProc	close;
    XSetOMValuesProc	set_values;
    XGetOMValuesProc	get_values;
    XCreateOCProc	create_oc;
} XOMMethodsRec, *XOMMethods;

typedef struct _XOMCoreRec {
    XLCd lcd;				/* lcd */
    Display *display;			/* display */
    XrmDatabase rdb;			/* database */
    char *res_name;			/* resource name */
    char *res_class;			/* resource class */
    XOC oc_list;			/* xoc list */
    XlcResourceList resources;		/* xom resources */
    int num_resources;			/* number of xom resources */
    XOMCharSetList required_charset;	/* required charset list */
    XOMOrientation orientation_list;	/* orientation list */
    Bool directional_dependent;		/* directional-dependent */
    Bool contextual_drawing;		/* contextual drawing */
    Bool context_dependent;		/* context-dependent drawing */
} XOMCoreRec, *XOMCore;

typedef struct _XOM {
    XOMMethods methods;
    XOMCoreRec core;
} XOMRec;

typedef void (*XDestroyOCProc)(
    XOC			/* oc */
);

typedef char* (*XSetOCValuesProc)(
    XOC			/* oc */,
    XlcArgList		/* args */,
    int			/* num_args */
);

typedef char* (*XGetOCValuesProc)(
    XOC			/* oc */,
    XlcArgList		/* args */,
    int			/* num_args */
);

/*
 * X Font Sets are an instantiable object, so we define it, the
 * object itself, a method list and data
 */

/*
 * XFontSet object method list
 */

typedef int (*XmbTextEscapementProc)(
    XFontSet		/* font_set */,
    _Xconst char*	/* text */,
    int			/* text_len */
);

typedef int (*XmbTextExtentsProc)(
    XFontSet		/* font_set */,
    _Xconst char*	/* text */,
    int			/* text_len */,
    XRectangle*		/* overall_ink_extents */,
    XRectangle*		/* overall_logical_extents */
);

typedef Status (*XmbTextPerCharExtentsProc)(
    XFontSet		/* font_set */,
    _Xconst char*	/* text */,
    int			/* text_len */,
    XRectangle*		/* ink_extents_buffer */,
    XRectangle*		/* logical_extents_buffer */,
    int			/* buffer_size */,
    int*		/* num_chars */,
    XRectangle*		/* max_ink_extents */,
    XRectangle*		/* max_logical_extents */
);

typedef int (*XmbDrawStringProc)(
    Display*		/* display */,
    Drawable		/* drawable */,
    XFontSet		/* font_set */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst char*	/* text */,
    int			/* text_len */
);

typedef void (*XmbDrawImageStringProc)(
    Display*		/* display */,
    Drawable		/* drawable */,
    XFontSet		/* font_set */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst char*	/* text */,
    int			/* text_len */
);

typedef int (*XwcTextEscapementProc)(
    XFontSet		/* font_set */,
    _Xconst wchar_t*	/* text */,
    int			/* text_len */
);

typedef int (*XwcTextExtentsProc)(
    XFontSet		/* font_set */,
    _Xconst wchar_t*	/* text */,
    int			/* text_len */,
    XRectangle*		/* overall_ink_extents */,
    XRectangle*		/* overall_logical_extents */
);

typedef Status (*XwcTextPerCharExtentsProc)(
    XFontSet		/* font_set */,
    _Xconst wchar_t*	/* text */,
    int			/* text_len */,
    XRectangle*		/* ink_extents_buffer */,
    XRectangle*		/* logical_extents_buffer */,
    int			/* buffer_size */,
    int*		/* num_chars */,
    XRectangle*		/* max_ink_extents */,
    XRectangle*		/* max_logical_extents */
);

typedef int (*XwcDrawStringProc)(
    Display*		/* display */,
    Drawable		/* drawable */,
    XFontSet		/* font_set */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst wchar_t*	/* text */,
    int			/* text_len */
);

typedef void (*XwcDrawImageStringProc)(
    Display*		/* display */,
    Drawable		/* drawable */,
    XFontSet		/* font_set */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    _Xconst wchar_t*	/* text */,
    int			/* text_len */
);

typedef struct {
    XDestroyOCProc 		destroy;
    XSetOCValuesProc 		set_values;
    XGetOCValuesProc 		get_values;

    /* multi-byte text drawing methods */

    XmbTextEscapementProc	mb_escapement;
    XmbTextExtentsProc		mb_extents;
    XmbTextPerCharExtentsProc	mb_extents_per_char;
    XmbDrawStringProc		mb_draw_string;
    XmbDrawImageStringProc	mb_draw_image_string;

    /* wide character text drawing methods */

    XwcTextEscapementProc	wc_escapement;
    XwcTextExtentsProc		wc_extents;
    XwcTextPerCharExtentsProc	wc_extents_per_char;
    XwcDrawStringProc		wc_draw_string;
    XwcDrawImageStringProc	wc_draw_image_string;

    /* UTF-8 text drawing methods */

    XmbTextEscapementProc	utf8_escapement;
    XmbTextExtentsProc		utf8_extents;
    XmbTextPerCharExtentsProc	utf8_extents_per_char;
    XmbDrawStringProc		utf8_draw_string;
    XmbDrawImageStringProc	utf8_draw_image_string;
} XOCMethodsRec, *XOCMethods;


/*
 * XOC independent data
 */

typedef struct {
    XOM om;				/* XOM */
    XOC next;				/* next XOC */
    XlcResourceList resources;		/* xoc resources */
    int num_resources;			/* number of xoc resources */
    char *base_name_list;     		/* base font name list */
    Bool om_automatic;			/* OM Automatic */
    XOMFontInfo font_info;		/* font info */
    XFontSetExtents font_set_extents;  	/* font set extents */
    char *default_string;     		/* default string */
    XOMCharSetList missing_list;	/* missing charset list */
    XOrientation orientation;		/* orientation */
    char *res_name;			/* resource name */
    char *res_class;			/* resource class */
} XOCCoreRec, *XOCCore;

typedef struct _XOC {
    XOCMethods methods;
    XOCCoreRec core;
} XOCRec;


/*
 * X Input Managers are an instantiable object, so we define it, the
 * object itself, a method list and data.
 */

/*
 * an Input Manager object method list
 */
typedef struct {
    Status (*close)(
	XIM
	);
    char* (*set_values)(
	XIM, XIMArg*
	);
    char* (*get_values)(
	XIM, XIMArg*
	);
    XIC (*create_ic)(
	XIM, XIMArg*
	);
    int (*ctstombs)(
	XIM, char*, int, char*, int, Status *
	);
    int (*ctstowcs)(
	XIM, char*, int, wchar_t*, int, Status *
	);
    int (*ctstoutf8)(
	XIM, char*, int, char*, int, Status *
	);
} XIMMethodsRec, *XIMMethods;

/*
 * Input Manager LC independent data
 */
typedef struct {
    XLCd		lcd;			/* LC of this input method */
    XIC			ic_chain;		/* list of ICs for this IM */

    Display *		display;               	/* display */
    XrmDatabase 	rdb;
    char *		res_name;
    char *		res_class;

    XIMValuesList	*im_values_list;
    XIMValuesList	*ic_values_list;
    XIMStyles		*styles;
    XIMCallback		 destroy_callback;
    char *		im_name;		/* XIMMODIFIER name */
    XIMResourceList	im_resources;		/* compiled IM resource list */
    unsigned int	im_num_resources;
    XIMResourceList	ic_resources;		/* compiled IC resource list */
    unsigned int	ic_num_resources;
    Bool		visible_position;
} XIMCoreRec, *XIMCore;



/*
 * An X Input Manager (IM).  Implementations may need to extend this data
 * structure to accommodate additional data, state information etc.
 */
typedef struct _XIM {
    XIMMethods		methods;		/* method list of this IM */
    XIMCoreRec		core;			/* data of this IM */
} XIMRec;



/*
 * X Input Contexts (IC) are an instantiable object, so we define it, the
 * object itself, a method list and data for this object
 */

/*
 * Input Context method list
 */
typedef struct {
    void (*destroy)(
	XIC
	);
    void (*set_focus)(
	XIC
	);
    void (*unset_focus)(
	XIC
	);
    char* (*set_values)(
	XIC, XIMArg*
	);
    char* (*get_values)(
	XIC, XIMArg*
	);
    char* (*mb_reset)(
	XIC
	);
    wchar_t* (*wc_reset)(
	XIC
	);
    char* (*utf8_reset)(
	XIC
	);
    int (*mb_lookup_string)(
	XIC, XKeyEvent*, char*, int, KeySym*, Status*
	);
    int (*wc_lookup_string)(
	XIC, XKeyEvent*, wchar_t*, int, KeySym*, Status*
	);
    int (*utf8_lookup_string)(
	XIC, XKeyEvent*, char*, int, KeySym*, Status*
	);
} XICMethodsRec, *XICMethods;


/*
 * Input Context LC independent data
 */
typedef struct {
    XIM			im;			/* XIM this IC belongs too */
    XIC			next;			/* linked list of ICs for IM */

    Window		client_window;		/* window IM can use for */
						/* display or subwindows */
    XIMStyle		input_style;		/* IM's input style */
    Window		focus_window;		/* where key events go */
    unsigned long	filter_events;		/* event mask from IM */
    XICCallback		geometry_callback;	/* client callback */
    char *		res_name;
    char *		res_class;

    XICCallback		destroy_callback;
    XICCallback		string_conversion_callback;
    XIMStringConversionText	 string_conversion;
    XIMResetState	reset_state;
    XIMHotKeyTriggers  *hotkey;
    XIMHotKeyState	hotkey_state;

    ICPreeditAttributes	preedit_attr;		/* visuals of preedit area */
    ICStatusAttributes	status_attr;		/* visuals of status area */
} XICCoreRec, *XICCore;


/*
 * an Input Context.  Implementations may need to extend this data
 * structure to accommodate additional data, state information etc.
 */
typedef struct _XIC {
    XICMethods		methods;		/* method list of this IC */
    XICCoreRec		core;			/* data of this IC */
} XICRec;


/* If the argument 'name' is appropriate for this loader, it instantiates an
   XLCd object with appropriate locale methods and returns it. May return
   NULL; in this case, the remaining loaders are tried. */
typedef XLCd (*XLCdLoadProc)(
    const char*		name
);

_XFUNCPROTOBEGIN

extern XLCd _XOpenLC(
    char*		name
);

extern void _XCloseLC(
    XLCd		lcd
);

extern XLCd _XlcCurrentLC (void);

extern Bool _XlcValidModSyntax(
    const char*		mods,
    const char* const *	valid
);

extern char *_XlcDefaultMapModifiers(
    XLCd		lcd,
    _Xconst char*		user_mods,
    _Xconst char*		prog_mods
);

extern void _XIMCompileResourceList(
    XIMResourceList	/* res */,
    unsigned int	/* num_res */
);

extern void _XCopyToArg(
    XPointer		/* src */,
    XPointer*		/* dst */,
    unsigned int	/* size */
);

extern char ** _XParseBaseFontNameList(
    char*		/* str */,
    int*		/* num */
);

extern XrmMethods _XrmInitParseInfo(
	XPointer*	statep
);

extern void _XRegisterFilterByMask(
    Display*		/* dpy */,
    Window		/* window */,
    unsigned long	/* event_mask */,
    Bool (*)(
	     Display*	/* display */,
	     Window	/* window */,
	     XEvent*	/* event */,
	     XPointer	/* client_data */
	     )		/* filter */,
    XPointer		/* client_data */
);

extern void _XRegisterFilterByType(
    Display*		/* dpy */,
    Window		/* window */,
    int			/* start_type */,
    int			/* end_type */,
    Bool (*)(
	     Display*	/* display */,
	     Window	/* window */,
	     XEvent*	/* event */,
	     XPointer	/* client_data */
	     )		/* filter */,
    XPointer		/* client_data */
);

extern void _XUnregisterFilter(
    Display*		/* dpy */,
    Window		/* window */,
    Bool (*)(
	     Display*	/* display */,
	     Window	/* window */,
	     XEvent*	/* event */,
	     XPointer	/* client_data */
	     )		/* filter */,
    XPointer		/* client_data */
);

extern void _XlcCountVaList(
    va_list		var,
    int*		count_return
);

extern void _XlcVaToArgList(
    va_list		var,
    int			count,
    XlcArgList*		args_return
);


extern void _XlcCopyFromArg(
    char *		src,
    char *		dst,
    int			size
);

extern void _XlcCopyToArg(
    char *		src,
    char **		dst,
    int			size
);

extern void _XlcCompileResourceList(
    XlcResourceList	resources,
    int			num_resources
);

extern char *_XlcGetValues(
    XPointer		base,
    XlcResourceList	resources,
    int			num_resources,
    XlcArgList		args,
    int			num_args,
    unsigned long	mask
);

extern char *_XlcSetValues(
    XPointer		base,
    XlcResourceList	resources,
    int			num_resources,
    XlcArgList		args,
    int			num_args,
    unsigned long	mask
);

/* documented in i18n/Framework.PS */
extern void _XlcInitLoader (void);

extern void _XlcDeInitLoader (void);

/* documented in i18n/Framework.PS */
/* Returns True on success, False on failure. */
extern Bool _XlcAddLoader(
    XLCdLoadProc	proc,
    XlcPosition		position
);

/* documented in i18n/Framework.PS */
extern void _XlcRemoveLoader(
    XLCdLoadProc	proc
);

/* Registers UTF-8 converters for a non-UTF-8 locale. */
extern void _XlcAddUtf8Converters(
    XLCd		lcd
);

/* Registers UTF-8 converters for a UTF-8 locale. */
extern void _XlcAddUtf8LocaleConverters(
    XLCd		lcd
);

/* Registers GB18030 converters for a GB18030 locale. */
extern void _XlcAddGB18030LocaleConverters(
    XLCd		lcd
);

/* The default locale loader. Assumes an ASCII encoding. */
extern XLCd _XlcDefaultLoader(
    const char*		name
);

/* The generic locale loader. Suitable for all encodings except UTF-8.
   Uses an XLC_LOCALE configuration file. */
extern XLCd _XlcGenericLoader(
    const char*		name
);

/* The UTF-8 locale loader. Suitable for UTF-8 encoding.
   Uses an XLC_LOCALE configuration file. */
extern XLCd _XlcUtf8Loader(
    const char*		name
);

extern XLCd _XlcDynamicLoad(
    const char*		name
);

/* The old dynamic loader. */
extern XLCd _XlcDynamicLoader(
    const char*		name
);

extern Bool _XInitDefaultIM(
    XLCd		lcd
);

extern Bool _XInitDefaultOM(
    XLCd		lcd
);

extern Bool _XInitDynamicIM(
    XLCd		lcd
);

extern Bool _XInitDynamicOM(
    XLCd		lcd
);

_XFUNCPROTOEND

#endif /* !_XP_PRINT_SERVER_ */

#endif	/* _XLCINT_H_ */
