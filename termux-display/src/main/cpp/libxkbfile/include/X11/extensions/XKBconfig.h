#ifndef _XKBCONFIG_H_
#define	_XKBCONFIG_H_ 1

/************************************************************
 Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

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


typedef struct _XkbConfigRtrn	*XkbConfigRtrnPtr;
typedef struct _XkbConfigField	*XkbConfigFieldPtr;
typedef struct _XkbConfigFields	*XkbConfigFieldsPtr;

typedef Bool (*XkbConfigParseFunc)(
	FILE *				/* file */,
	XkbConfigFieldsPtr		/* fields */,
	XkbConfigFieldPtr		/* field */,
	XkbDescPtr			/* xkb */,
	XkbConfigRtrnPtr		/* rtrn */
);

#define	XkbCF_Check	0
#define	XkbCF_Apply	1
#define	XkbCF_CleanUp	2
#define	XkbCF_Destroy	3

typedef	Bool (*XkbConfigFinishFunc)(
	XkbConfigFieldsPtr		/* fields */,
	XkbDescPtr			/* xkb */,
	XkbConfigRtrnPtr		/* rtrn */,
	int				/* what */
);

typedef struct _XkbConfigRtrnPriv {
	int				cfg_id;
	XPointer			priv;
	struct _XkbConfigRtrnPriv *	next;
} XkbConfigRtrnPrivRec,*XkbConfigRtrnPrivPtr;

typedef struct _XkbConfigModInfo {
	Bool			replace;
	unsigned char		mods;
	unsigned char		mods_clear;
	unsigned short		vmods;
	unsigned short		vmods_clear;
} XkbConfigModInfoRec,*XkbConfigModInfoPtr;

typedef struct _XkbConfigUnboundMod {
	unsigned char		what;
	unsigned char		mods;
	unsigned short		vmods;
	short			merge;
	char *			name;
} XkbConfigUnboundModRec,*XkbConfigUnboundModPtr;

#define	XkbCF_MergeSet			0
#define	XkbCF_MergeAdd			1
#define	XkbCF_MergeRemove		2

#define	XkbCF_InitialMods		(1L<<0)
#define	XkbCF_InternalMods		(1L<<1)
#define	XkbCF_IgnoreLockMods		(1L<<2)
#define	XkbCF_InitialCtrls		(1L<<3)
#define	XkbCF_AccessXTimeout		(1L<<4)
#define	XkbCF_AccessXTimeoutCtrlsOn	(1L<<5)
#define	XkbCF_AccessXTimeoutCtrlsOff	(1L<<6)
#define	XkbCF_AccessXTimeoutOptsOn	(1L<<7)
#define	XkbCF_AccessXTimeoutOptsOff	(1L<<8)
#define	XkbCF_GroupsWrap		(1L<<9)
#define	XkbCF_InitialOpts		(1L<<10)

typedef struct _XkbConfigRtrn {
	unsigned		defined;
	int			error;
	int			line;

	int			click_volume;
	int			bell_volume;
	int			bell_pitch;
	int			bell_duration;
	int			repeat_delay;
	int			repeat_interval;

	char *			rules_file;
	char *			model;
	char *			layout;
	char *			variant;
	char *			options;

	char *			keymap;
	char *			keycodes;
	char *			geometry;
	char *			phys_symbols;
	char *			symbols;
	char *			types;
	char *			compat;

	Bool			replace_initial_ctrls;
	unsigned long		initial_ctrls;
	unsigned long		initial_ctrls_clear;

	Bool			replace_initial_opts;
	unsigned short		initial_opts;
	unsigned short		initial_opts_clear;

	XkbConfigModInfoRec	initial_mods;
	XkbConfigModInfoRec	internal_mods;
	XkbConfigModInfoRec	ignore_lock_mods;

	short			num_unbound_mods;
	short			sz_unbound_mods;
	XkbConfigUnboundModPtr	unbound_mods;

	int			groups_wrap;
	int			slow_keys_delay;
	int			debounce_delay;
	int			mk_delay;
	int			mk_interval;
	int			mk_time_to_max;
	int			mk_max_speed;
	int			mk_curve;
	int			ax_timeout;

	Bool			replace_axt_ctrls_on;
	Bool			replace_axt_ctrls_off;
	unsigned long		axt_ctrls_on;
	unsigned long		axt_ctrls_off;
	unsigned long		axt_ctrls_ignore;

	Bool			replace_axt_opts_off;
	Bool			replace_axt_opts_on;
	unsigned short		axt_opts_off;
	unsigned short		axt_opts_on;
	unsigned short		axt_opts_ignore;
	XkbConfigRtrnPrivPtr	priv;
} XkbConfigRtrnRec;

typedef struct _XkbConfigField {
	char *		field;
	unsigned char	field_id;
} XkbConfigFieldRec;

typedef struct _XkbConfigFields {
	unsigned short		cfg_id;
	unsigned short		num_fields;
	XkbConfigFieldPtr	fields;
	XkbConfigParseFunc	parser;
	XkbConfigFinishFunc	finish;
	XPointer		priv;
	struct _XkbConfigFields *next;
} XkbConfigFieldsRec;

#define	XkbCF_EOF			-1
#define	XkbCF_Unknown			 0
#define	XkbCF_EOL			 1
#define	XkbCF_Semi			 2
#define	XkbCF_Equals			 3
#define	XkbCF_PlusEquals		 4
#define	XkbCF_MinusEquals		 5
#define	XkbCF_Plus			 6
#define	XkbCF_Minus			 7
#define	XkbCF_String			10
#define	XkbCF_Ident			11
#define	XkbCF_Integer			12

#define	XkbCF_UnterminatedString	100
#define	XkbCF_BadAlloc			101
#define	XkbCF_MissingIdent		102
#define	XkbCF_MissingEquals		103
#define	XkbCF_ExpectedEOS		104
#define	XkbCF_ExpectedBoolean		105
#define	XkbCF_ExpectedInteger		106
#define	XkbCF_ExpectedString		107
#define	XkbCF_ExpectedModifier		108
#define	XkbCF_ExpectedControl		109
#define	XkbCF_ExpectedAXOption		110
#define	XkbCF_ExpectedOperator		111
#define	XkbCF_ExpectedOORGroupBehavior	112

typedef union {
	int		ival;
	char *		str;
} XkbCFScanResultRec,*XkbCFScanResultPtr;

extern	XkbConfigFieldsPtr	XkbCFDflts;

_XFUNCPROTOBEGIN

extern int XkbCFScan(
	FILE *			/* file */,
	XkbCFScanResultPtr	/* val_rtrn */,
	XkbConfigRtrnPtr	/* rtrn */
);

extern XkbConfigFieldsPtr XkbCFDup(
	XkbConfigFieldsPtr	/* fields */
);

extern XkbConfigFieldsPtr XkbCFFree(
	XkbConfigFieldsPtr	/* fields */,
	Bool			/* all */
);

extern	XkbConfigUnboundModPtr XkbCFAddModByName(
	XkbConfigRtrnPtr	/* rtrn */,
	int			/* what */,
	char *			/* name */,
	Bool			/* merge */,
	XkbConfigUnboundModPtr	/* last */
);

extern	Bool XkbCFBindMods(
	XkbConfigRtrnPtr	/* rtrn */,
	XkbDescPtr		/* xkb */
);

extern	Bool XkbCFApplyMods(
	XkbConfigRtrnPtr	/* rtrn */,
	int			/* what */,
	XkbConfigModInfoPtr	/* info */
);

extern	Bool XkbCFApplyRtrnValues(
	XkbConfigRtrnPtr	/* rtrn */,
	XkbConfigFieldsPtr	/* fields */,
	XkbDescPtr		/* xkb */
);

extern	XkbConfigRtrnPrivPtr XkbCFAddPrivate(
	XkbConfigRtrnPtr	/* rtrn */,
	XkbConfigFieldsPtr	/* fields */,
	XPointer		/* ptr */
);

extern void XkbCFFreeRtrn(
	XkbConfigRtrnPtr	/* rtrn */,
	XkbConfigFieldsPtr	/* fields */,
	XkbDescPtr		/* xkb */
);

extern Bool XkbCFParse(
	FILE *			/* file */,
	XkbConfigFieldsPtr	/* fields */,
	XkbDescPtr		/* xkb */,
	XkbConfigRtrnPtr	/* rtrn */
);

extern	void XkbCFReportError(
	FILE *			/* file */,
	char *			/* name */,
	int			/* error */,
	int			/* line */
);

_XFUNCPROTOEND

#endif /* _XKBCONFIG_H_ */
