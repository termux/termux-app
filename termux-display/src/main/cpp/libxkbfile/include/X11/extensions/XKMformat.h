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

#ifndef _XKMFORMAT_H_
#define	_XKMFORMAT_H_ 1

#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBproto.h>
#include <X11/extensions/XKM.h>

typedef	struct _xkmFileInfo {
	CARD8		type;
	CARD8		min_kc;
	CARD8		max_kc;
	CARD8		num_toc;
	CARD16		present;
	CARD16		pad;
} xkmFileInfo;
#define	sz_xkmFileInfo	8

typedef	struct _xkmSectionInfo {
	CARD16		type;
	CARD16		format;
	CARD16		size;
	CARD16		offset;
} xkmSectionInfo;
#define	sz_xkmSectionInfo	8

typedef struct _xkmKeyTypeDesc {
	CARD8		realMods;
	CARD8		numLevels;
	CARD16		virtualMods;
	CARD8		nMapEntries;
	CARD8		nLevelNames;
	CARD8		preserve;
	CARD8		pad;
} xkmKeyTypeDesc;
#define	sz_xkmKeyTypeDesc	8

typedef struct _xkmKTMapEntryDesc {
	CARD8		level;
	CARD8		realMods;
	CARD16		virtualMods;
} xkmKTMapEntryDesc;
#define	sz_xkmKTMapEntryDesc	4

typedef struct _xkmModsDesc {
	CARD8		realMods;
	CARD8		pad;
	CARD16		virtualMods;
} xkmModsDesc;
#define	sz_xkmModsDesc	4

typedef struct _xkmVModMapDesc {
	CARD8		key;
	CARD8		pad;
	CARD16		vmods;
} xkmVModMapDesc;
#define	sz_xkmVModMapDesc	4

typedef struct _xkmSymInterpretDesc {
	CARD32		sym;
	CARD8		mods;
	CARD8		match;
	CARD8		virtualMod;
	CARD8		flags;
	CARD8		actionType;
	CARD8		actionData[7];
} xkmSymInterpretDesc;
#define	sz_xkmSymInterpretDesc	16

typedef struct _xkmBehaviorDesc {
	CARD8		type;
	CARD8		data;
	CARD16		pad;
} xkmBehaviorDesc;
#define	sz_xkmBehaviorDesc	4

typedef struct _xkmActionDesc {
	CARD8		type;
	CARD8		data[7];
} xkmActionDesc;
#define	sz_xkmActionDesc	8

#define	XkmKeyHasTypes		(0x0f)
#define	XkmKeyHasGroup1Type	(1<<0)
#define	XkmKeyHasGroup2Type	(1<<1)
#define	XkmKeyHasGroup3Type	(1<<2)
#define	XkmKeyHasGroup4Type	(1<<3)
#define	XkmKeyHasActions	(1<<4)
#define	XkmKeyHasBehavior	(1<<5)
#define	XkmRepeatingKey		(1<<6)
#define	XkmNonRepeatingKey	(1<<7)

typedef struct _xkmKeySymMapDesc {
	CARD8		width;
	CARD8		num_groups;
	CARD8		modifier_map;
	CARD8		flags;
} xkmKeySymMapDesc;
#define sz_xkmKeySymMapDesc	4

typedef struct _xkmIndicatorMapDesc {
	CARD8		indicator;
	CARD8		flags;
	CARD8		which_mods;
	CARD8		real_mods;
	CARD16		vmods;
	CARD8		which_groups;
	CARD8		groups;
	CARD32		ctrls;
} xkmIndicatorMapDesc;
#define sz_xkmIndicatorMapDesc	12

typedef struct _xkmGeometryDesc {
	CARD16		width_mm;
	CARD16		height_mm;
	CARD8		base_color_ndx;
	CARD8		label_color_ndx;
	CARD16		num_properties;
	CARD16		num_colors;
	CARD16		num_shapes;
	CARD16		num_sections;
	CARD16		num_doodads;
	CARD16		num_key_aliases;
	CARD16		pad1;
} xkmGeometryDesc;
#define	sz_xkmGeometryDesc	20

typedef struct _xkmPointDesc {
	INT16		x;
	INT16		y;
} xkmPointDesc;
#define	sz_xkmPointDesc		4

typedef	struct _xkmOutlineDesc {
	CARD8		num_points;
	CARD8		corner_radius;
	CARD16		pad;
} xkmOutlineDesc;
#define	sz_xkmOutlineDesc	4

typedef struct _xkmShapeDesc {
	CARD8		num_outlines;
	CARD8		primary_ndx;
	CARD8		approx_ndx;
	CARD8		pad;
} xkmShapeDesc;
#define	sz_xkmShapeDesc	4

typedef struct _xkmSectionDesc {
	INT16		top;
	INT16		left;
	CARD16		width;
	CARD16		height;
	INT16		angle;
	CARD8		priority;
	CARD8		num_rows;
	CARD8		num_doodads;
	CARD8		num_overlays;
	CARD16		pad2;
} xkmSectionDesc;
#define	sz_xkmSectionDesc	16

typedef struct _xkmRowDesc {
	INT16		top;
	INT16		left;
	CARD8		num_keys;
	BOOL		vertical;
	CARD16		pad;
} xkmRowDesc;
#define	sz_xkmRowDesc		8

typedef struct _xkmKeyDesc {
	CARD8		name[XkbKeyNameLength];
	INT16		gap;
	CARD8		shape_ndx;
	CARD8		color_ndx;
} xkmKeyDesc;
#define	sz_xkmKeyDesc		8

typedef struct _xkmOverlayDesc {
	CARD8		num_rows;
	CARD8		pad1;
	CARD16		pad2;
} xkmOverlayDesc;
#define	sz_xkmOverlayDesc	4

typedef struct _xkmOverlayRowDesc {
	CARD8		row_under;
	CARD8		num_keys;
	CARD16		pad;
} xkmOverlayRowDesc;
#define	sz_xkmOverlayRowDesc	4

typedef struct _xkmOverlayKeyDesc {
	char		over[XkbKeyNameLength];
	char		under[XkbKeyNameLength];
} xkmOverlayKeyDesc;
#define sz_xkmOverlayKeyDesc	8

typedef struct _xkmShapeDoodadDesc {
	CARD8		type;
	CARD8		priority;
	INT16		top;
	INT16		left;
	INT16		angle;
	CARD8		color_ndx;
	CARD8		shape_ndx;
	CARD16		pad;
	CARD32		pad1;
} xkmShapeDoodadDesc;
#define	sz_xkmShapeDoodadDesc	16

typedef struct _xkmTextDoodadDesc {
	CARD8	 	type;
	CARD8	 	priority;
	INT16	 	top;
	INT16	 	left;
	INT16	 	angle;
	CARD16		width;
	CARD16		height;
	CARD8	 	color_ndx;
	CARD8		pad1;
	CARD16		pad2;
} xkmTextDoodadDesc;
#define	sz_xkmTextDoodadDesc	16

typedef struct _xkmIndicatorDoodadDesc {
	CARD8		type;
	CARD8		priority;
	INT16		top;
	INT16		left;
	CARD8		shape_ndx;
	CARD8		on_color_ndx;
	CARD8		off_color_ndx;
	CARD8		pad1;
	CARD16		pad2;
	CARD32		pad3;
} xkmIndicatorDoodadDesc;
#define	sz_xkmIndicatorDoodadDesc	16

typedef struct _xkmLogoDoodadDesc {
	CARD8		type;
	CARD8		priority;
	INT16		top;
	INT16		left;
	INT16		angle;
	CARD8		color_ndx;
	CARD8		shape_ndx;
	CARD16		pad;
	CARD32		pad1;
} xkmLogoDoodadDesc;
#define	sz_xkmLogoDoodadDesc	16

typedef struct _xkmAnyDoodadDesc {
	CARD8		type;
	CARD8		priority;
	INT16		top;
	INT16		left;
	CARD16		pad1;
	CARD32		pad2;
	CARD32		pad3;
} xkmAnyDoodadDesc;
#define	sz_xkmAnyDoodadDesc		16

typedef union _xkmDoodadDesc {
	xkmAnyDoodadDesc	any;
	xkmShapeDoodadDesc	shape;
	xkmTextDoodadDesc	text;
	xkmIndicatorDoodadDesc	indicator;
	xkmLogoDoodadDesc	logo;
} xkmDoodadDesc;
#define	sz_xkmDoodadDesc		16

#endif /* _XKMFORMAT_H_ */
