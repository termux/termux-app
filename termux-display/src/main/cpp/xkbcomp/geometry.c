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

#include "xkbcomp.h"
#include "tokens.h"
#include "expr.h"
#include "vmod.h"
#include "misc.h"
#include "indicators.h"
#include "action.h"
#include "keycodes.h"
#include "alias.h"

#include "X11/extensions/XKBgeom.h"

#define	DFLT_FONT	"helvetica"
#define	DFLT_SLANT	"r"
#define	DFLT_WEIGHT	"medium"
#define	DFLT_SET_WIDTH	"normal"
#define	DFLT_VARIANT	""
#define	DFLT_ENCODING	"iso8859-1"
#define	DFLT_SIZE	120

typedef struct _PropertyInfo
{
    CommonInfo defs;
    char *name;
    char *value;
} PropertyInfo;

#define	_GSh_Outlines	(1<<1)
#define	_GSh_Approx	(1<<2)
#define	_GSh_Primary	(1<<3)
typedef struct _ShapeInfo
{
    CommonInfo defs;
    Atom name;
    short index;
    unsigned short nOutlines;
    unsigned short szOutlines;
    XkbOutlinePtr outlines;
    XkbOutlinePtr approx;
    XkbOutlinePtr primary;
    int dfltCornerRadius;
} ShapeInfo;

#define	shText(d,s)	\
		((s)?XkbAtomText((d),(s)->name,XkbMessage):"default shape")

#define	_GD_Priority	(1<<0)
#define	_GD_Top		(1<<1)
#define	_GD_Left	(1<<2)
#define	_GD_Angle	(1<<3)
#define	_GD_Shape	(1<<4)
#define	_GD_FontVariant	(1<<4)  /* CHEATING */
#define	_GD_Corner	(1<<5)
#define	_GD_Width	(1<<5)  /* CHEATING */
#define	_GD_Color	(1<<6)
#define	_GD_OffColor	(1<<7)
#define	_GD_Height	(1<<7)  /* CHEATING */
#define	_GD_Text	(1<<8)
#define	_GD_Font	(1<<9)
#define	_GD_FontSlant	(1<<10)
#define	_GD_FontWeight	(1<<11)
#define	_GD_FontSetWidth (1<<12)
#define	_GD_FontSize	(1<<13)
#define	_GD_FontEncoding (1<<14)
#define	_GD_FontSpec	(1<<15)


#define	_GD_FontParts	(_GD_Font|_GD_FontSlant|_GD_FontWeight|_GD_FontSetWidth|_GD_FontSize|_GD_FontEncoding|_GD_FontVariant)

typedef struct _DoodadInfo
{
    CommonInfo defs;
    Atom name;
    unsigned char type;
    unsigned char priority;
    short top;
    short left;
    short angle;
    unsigned short corner;
    unsigned short width;
    unsigned short height;
    Atom shape;
    Atom color;
    Atom offColor;
    Atom text;
    Atom font;
    Atom fontSlant;
    Atom fontWeight;
    Atom fontSetWidth;
    Atom fontVariant;
    unsigned short fontSize;
    Atom fontEncoding;
    Atom fontSpec;
    char *logoName;
    struct _SectionInfo *section;
} DoodadInfo;

#define	Yes		1
#define	No		0
#define	Undefined	-1

#define	_GK_Default	(1<<0)
#define	_GK_Name	(1<<1)
#define	_GK_Gap		(1<<2)
#define	_GK_Shape	(1<<3)
#define	_GK_Color	(1<<4)
typedef struct _KeyInfo
{
    CommonInfo defs;
    char name[8];
    short gap;
    short index;
    Atom shape;
    Atom color;
    struct _RowInfo *row;
} KeyInfo;
#define	keyText(k)	((k)&&(k)->name[0]?(k)->name:"default")

#define	_GR_Default	(1<<0)
#define	_GR_Vertical	(1<<1)
#define	_GR_Top		(1<<2)
#define	_GR_Left	(1<<3)
typedef struct _RowInfo
{
    CommonInfo defs;
    unsigned short top;
    unsigned short left;
    short index;
    Bool vertical;
    unsigned short nKeys;
    KeyInfo *keys;
    KeyInfo dfltKey;
    struct _SectionInfo *section;
} RowInfo;
#define	rowText(d,r)	\
	((r)?XkbAtomText((d),(r)->section->name,XkbMessage):"default")

#define	_GOK_UnknownRow	-1
typedef struct _OverlayKeyInfo
{
    CommonInfo defs;
    short sectionRow;
    short overlayRow;
    char over[XkbKeyNameLength + 1];
    char under[XkbKeyNameLength + 1];
} OverlayKeyInfo;

typedef struct _OverlayInfo
{
    CommonInfo defs;
    Atom name;
    unsigned short nRows;
    unsigned short nKeys;
    OverlayKeyInfo *keys;
} OverlayInfo;


#define	_GS_Default	(1<<0)
#define	_GS_Name	(1<<1)
#define	_GS_Top		(1<<2)
#define	_GS_Left	(1<<3)
#define	_GS_Width	(1<<4)
#define	_GS_Height	(1<<5)
#define	_GS_Angle	(1<<6)
#define	_GS_Priority	(1<<7)
typedef struct _SectionInfo
{
    CommonInfo defs;
    Atom name;
    unsigned short top;
    unsigned short left;
    unsigned short width;
    unsigned short height;
    unsigned short angle;
    unsigned short nRows;
    unsigned short nDoodads;
    unsigned short nOverlays;
    unsigned char priority;
    unsigned char nextDoodadPriority;
    RowInfo *rows;
    DoodadInfo *doodads;
    RowInfo dfltRow;
    DoodadInfo *dfltDoodads;
    OverlayInfo *overlays;
    struct _GeometryInfo *geometry;
} SectionInfo;
#define	scText(d,s)	((s)?XkbAtomText((d),(s)->name,XkbMessage):"default")

typedef struct _GeometryInfo
{
    char *name;
    Display *dpy;
    unsigned fileID;
    unsigned merge;
    int errorCount;
    unsigned nextPriority;
    int nProps;
    int nShapes;
    int nSections;
    int nDoodads;
    PropertyInfo *props;
    ShapeInfo *shapes;
    SectionInfo *sections;
    DoodadInfo *doodads;
    int widthMM;
    int heightMM;
    Atom font;
    Atom fontSlant;
    Atom fontWeight;
    Atom fontSetWidth;
    Atom fontVariant;
    unsigned fontSize;
    Atom fontEncoding;
    Atom fontSpec;
    Atom baseColor;
    Atom labelColor;
    int dfltCornerRadius;
    SectionInfo dfltSection;
    DoodadInfo *dfltDoodads;
    AliasInfo *aliases;
} GeometryInfo;

static char *
ddText(Display *dpy, const DoodadInfo *di)
{
    static char buf[64];

    if (di == NULL)
    {
        strcpy(buf, "default");
        return buf;
    }
    if (di->section)
    {
        snprintf(buf, sizeof(buf), "%s in section %s",
                 XkbAtomText(dpy, di->name, XkbMessage),
                 scText(dpy, di->section));
        return buf;
    }
    return XkbAtomText(dpy, di->name, XkbMessage);
}

/***====================================================================***/

static void
InitPropertyInfo(PropertyInfo *pi, const GeometryInfo *info)
{
    pi->defs.defined = 0;
    pi->defs.fileID = info->fileID;
    pi->defs.merge = info->merge;
    pi->name = pi->value = NULL;
    return;
}

static void
FreeProperties(PropertyInfo * pi, GeometryInfo * info)
{
    PropertyInfo *tmp;
    PropertyInfo *next;

    if (info->props == pi)
    {
        info->props = NULL;
        info->nProps = 0;
    }
    for (tmp = pi; tmp != NULL; tmp = next)
    {
        free(tmp->name);
        free(tmp->value);
        tmp->name = tmp->value = NULL;
        next = (PropertyInfo *) tmp->defs.next;
        free(tmp);
    }
    return;
}

static void
InitKeyInfo(KeyInfo *key, RowInfo *row, const GeometryInfo *info)
{

    if (key != &row->dfltKey)
    {
        *key = row->dfltKey;
        strcpy(key->name, "unknown");
        key->defs.defined &= ~_GK_Default;
    }
    else
    {
        bzero(key, sizeof(KeyInfo));
        *key = (KeyInfo) {
            .defs.defined = _GK_Default,
            .defs.fileID = info->fileID,
            .defs.merge = info->merge,
            .defs.next = NULL,
            .row = row
        };
        strcpy(key->name, "default");
    }
    return;
}

static void
ClearKeyInfo(KeyInfo * key)
{
    key->defs.defined &= ~_GK_Default;
    strcpy(key->name, "default");
    key->gap = 0;
    key->shape = None;
    key->color = None;
    return;
}

static void
FreeKeys(KeyInfo *key, RowInfo *row, const GeometryInfo *info)
{
    KeyInfo *tmp;
    KeyInfo *next;

    if (row->keys == key)
    {
        row->nKeys = 0;
        row->keys = NULL;
    }
    for (tmp = key; tmp != NULL; tmp = next)
    {
        ClearKeyInfo(tmp);
        next = (KeyInfo *) tmp->defs.next;
        free(tmp);
    }
    return;
}

static void
InitRowInfo(RowInfo *row, SectionInfo *section, const GeometryInfo *info)
{
    if (row != &section->dfltRow)
    {
        *row = section->dfltRow;
        row->defs.defined &= ~_GR_Default;
    }
    else
    {
        bzero(row, sizeof(*row));
        row->defs.defined = _GR_Default;
        row->defs.fileID = info->fileID;
        row->defs.merge = info->merge;
        row->defs.next = NULL;
        row->section = section;
        row->nKeys = 0;
        row->keys = NULL;
        InitKeyInfo(&row->dfltKey, row, info);
    }
    return;
}

static void
ClearRowInfo(RowInfo *row, const GeometryInfo *info)
{
    row->defs.defined &= ~_GR_Default;
    row->top = row->left = 0;
    row->vertical = False;
    row->nKeys = 0;
    if (row->keys)
        FreeKeys(row->keys, row, info);
    ClearKeyInfo(&row->dfltKey);
    row->dfltKey.defs.defined |= _GK_Default;
    return;
}

static void
FreeRows(RowInfo *row, SectionInfo *section, const GeometryInfo *info)
{
    RowInfo *next;

    if (row == section->rows)
    {
        section->nRows = 0;
        section->rows = NULL;
    }
    for (RowInfo *tmp = row; tmp != NULL; tmp = next)
    {
        ClearRowInfo(tmp, info);
        next = (RowInfo *) tmp->defs.next;
        free(tmp);
    }
    return;
}

static DoodadInfo *
FindDoodadByType(DoodadInfo * di, unsigned type)
{
    while (di)
    {
        if (di->type == type)
            return di;
        di = (DoodadInfo *) di->defs.next;
    }
    return NULL;
}

static DoodadInfo *
FindDoodadByName(DoodadInfo * di, Atom name)
{
    while (di)
    {
        if (di->name == name)
            return di;
        di = (DoodadInfo *) di->defs.next;
    }
    return NULL;
}

static void
InitDoodadInfo(DoodadInfo * di, unsigned type, SectionInfo * si,
               GeometryInfo * info)
{
    DoodadInfo *dflt;

    dflt = NULL;
    if (si && si->dfltDoodads)
        dflt = FindDoodadByType(si->dfltDoodads, type);
    if ((dflt == NULL) && (info->dfltDoodads))
        dflt = FindDoodadByType(info->dfltDoodads, type);
    if (dflt != NULL)
    {
        *di = *dflt;
        di->defs.next = NULL;
    }
    else
    {
        bzero(di, sizeof(DoodadInfo));
        di->defs.fileID = info->fileID;
        di->type = type;
    }
    di->section = si;
    if (si != NULL)
    {
        di->priority = si->nextDoodadPriority++;
#if XkbGeomMaxPriority < 255
        if (si->nextDoodadPriority > XkbGeomMaxPriority)
            si->nextDoodadPriority = XkbGeomMaxPriority;
#endif
    }
    else
    {
        di->priority = info->nextPriority++;
        if (info->nextPriority > XkbGeomMaxPriority)
            info->nextPriority = XkbGeomMaxPriority;
    }
    return;
}

static void
ClearDoodadInfo(DoodadInfo * di)
{
    CommonInfo defs;

    defs = di->defs;
    bzero(di, sizeof(DoodadInfo));
    di->defs = defs;
    di->defs.defined = 0;
    return;
}

static void
ClearOverlayInfo(OverlayInfo * ol)
{
    if (ol && ol->keys)
    {
        ol->keys = (OverlayKeyInfo *) ClearCommonInfo(&ol->keys->defs);
        ol->nKeys = 0;
    }
    return;
}

static void
FreeDoodads(DoodadInfo * di, SectionInfo * si, GeometryInfo * info)
{
    DoodadInfo *tmp;
    DoodadInfo *next;

    if (si)
    {
        if (si->doodads == di)
        {
            si->doodads = NULL;
            si->nDoodads = 0;
        }
        if (si->dfltDoodads == di)
            si->dfltDoodads = NULL;
    }
    if (info->doodads == di)
    {
        info->doodads = NULL;
        info->nDoodads = 0;
    }
    if (info->dfltDoodads == di)
        info->dfltDoodads = NULL;
    for (tmp = di; tmp != NULL; tmp = next)
    {
        next = (DoodadInfo *) tmp->defs.next;
        ClearDoodadInfo(tmp);
        free(tmp);
    }
    return;
}

static void
InitSectionInfo(SectionInfo * si, GeometryInfo * info)
{
    if (si != &info->dfltSection)
    {
        *si = info->dfltSection;
        si->defs.defined &= ~_GS_Default;
        si->name = XkbInternAtom(info->dpy, "unknown", False);
        si->priority = info->nextPriority++;
        if (info->nextPriority > XkbGeomMaxPriority)
            info->nextPriority = XkbGeomMaxPriority;
    }
    else
    {
        bzero(si, sizeof(SectionInfo));
        si->defs.fileID = info->fileID;
        si->defs.merge = info->merge;
        si->defs.next = NULL;
        si->geometry = info;
        si->name = XkbInternAtom(info->dpy, "default", False);
        InitRowInfo(&si->dfltRow, si, info);
    }
    return;
}

static void
DupSectionInfo(SectionInfo *into, const SectionInfo *from,
               const GeometryInfo *info)
{
    CommonInfo defs;

    defs = into->defs;
    *into = *from;
    into->defs.fileID = defs.fileID;
    into->defs.merge = defs.merge;
    into->defs.next = NULL;
    into->dfltRow.defs.fileID = defs.fileID;
    into->dfltRow.defs.merge = defs.merge;
    into->dfltRow.defs.next = NULL;
    into->dfltRow.section = into;
    into->dfltRow.dfltKey.defs.fileID = defs.fileID;
    into->dfltRow.dfltKey.defs.merge = defs.merge;
    into->dfltRow.dfltKey.defs.next = NULL;
    into->dfltRow.dfltKey.row = &into->dfltRow;
    return;
}

static void
ClearSectionInfo(SectionInfo * si, GeometryInfo * info)
{

    si->defs.defined &= ~_GS_Default;
    si->name = XkbInternAtom(info->dpy, "default", False);
    si->top = si->left = 0;
    si->width = si->height = 0;
    si->angle = 0;
    if (si->rows)
    {
        FreeRows(si->rows, si, info);
        si->rows = NULL;
    }
    ClearRowInfo(&si->dfltRow, info);
    if (si->doodads)
    {
        FreeDoodads(si->doodads, si, info);
        si->doodads = NULL;
    }
    si->dfltRow.defs.defined = _GR_Default;
    return;
}

static void
FreeSections(SectionInfo * si, GeometryInfo * info)
{
    SectionInfo *tmp;
    SectionInfo *next;

    if (si == info->sections)
    {
        info->nSections = 0;
        info->sections = NULL;
    }
    for (tmp = si; tmp != NULL; tmp = next)
    {
        ClearSectionInfo(tmp, info);
        next = (SectionInfo *) tmp->defs.next;
        free(tmp);
    }
    return;
}

static void
FreeShapes(ShapeInfo * si, GeometryInfo * info)
{
    ShapeInfo *tmp;
    ShapeInfo *next;

    if (si == info->shapes)
    {
        info->nShapes = 0;
        info->shapes = NULL;
    }
    for (tmp = si; tmp != NULL; tmp = next)
    {
        if (tmp->outlines)
        {
            int i;
            for (i = 0; i < tmp->nOutlines; i++)
            {
                if (tmp->outlines[i].points != NULL)
                {
                    free(tmp->outlines[i].points);
                    tmp->outlines[i].num_points = 0;
                    tmp->outlines[i].points = NULL;
                }
            }
            free(tmp->outlines);
            tmp->szOutlines = 0;
            tmp->nOutlines = 0;
            tmp->outlines = NULL;
            tmp->primary = tmp->approx = NULL;
        }
        next = (ShapeInfo *) tmp->defs.next;
        free(tmp);
    }
    return;
}

/***====================================================================***/

static void
InitGeometryInfo(GeometryInfo * info, unsigned fileID, unsigned merge)
{
    bzero(info, sizeof(GeometryInfo));
    info->fileID = fileID;
    info->merge = merge;
    InitSectionInfo(&info->dfltSection, info);
    info->dfltSection.defs.defined = _GS_Default;
    return;
}

static void
ClearGeometryInfo(GeometryInfo * info)
{
    free(info->name);
    info->name = NULL;
    if (info->props)
        FreeProperties(info->props, info);
    if (info->shapes)
        FreeShapes(info->shapes, info);
    if (info->sections)
        FreeSections(info->sections, info);
    info->widthMM = 0;
    info->heightMM = 0;
    info->dfltCornerRadius = 0;
    ClearSectionInfo(&info->dfltSection, info);
    info->dfltSection.defs.defined = _GS_Default;
    if (info->aliases)
        ClearAliases(&info->aliases);
    return;
}

/***====================================================================***/

static PropertyInfo *
NextProperty(GeometryInfo * info)
{
    PropertyInfo *pi;

    pi = calloc(1, sizeof(PropertyInfo));
    if (pi)
    {
        info->props = (PropertyInfo *) AddCommonInfo(&info->props->defs,
                                                     (CommonInfo *) pi);
        info->nProps++;
    }
    return pi;
}

static PropertyInfo *
FindProperty(const GeometryInfo *info, const char *name)
{
    PropertyInfo *old;

    if (!name)
        return NULL;
    for (old = info->props; old != NULL;
         old = (PropertyInfo *) old->defs.next)
    {
        if ((old->name) && (uStringEqual(name, old->name)))
            return old;
    }
    return NULL;
}

static Bool
AddProperty(GeometryInfo * info, PropertyInfo * new)
{
    PropertyInfo *old;

    if ((!new) || (!new->value) || (!new->name))
        return False;
    old = FindProperty(info, new->name);
    if (old != NULL)
    {
        if ((new->defs.merge == MergeReplace)
            || (new->defs.merge == MergeOverride))
        {
            if (((old->defs.fileID == new->defs.fileID)
                 && (warningLevel > 0)) || (warningLevel > 9))
            {
                WARN("Multiple definitions for the \"%s\" property\n",
                      new->name);
                ACTION("Ignoring \"%s\", using \"%s\"\n", old->value,
                        new->value);
            }
            free(old->value);
            old->value = uStringDup(new->value);
            return True;
        }
        if (((old->defs.fileID == new->defs.fileID) && (warningLevel > 0))
            || (warningLevel > 9))
        {
            WARN("Multiple definitions for \"%s\" property\n", new->name);
            ACTION("Using \"%s\", ignoring \"%s\" \n", old->value,
                    new->value);
        }
        return True;
    }
    old = new;
    if ((new = NextProperty(info)) == NULL)
        return False;
    new->defs.next = NULL;
    new->name = uStringDup(old->name);
    new->value = uStringDup(old->value);
    return True;
}

/***====================================================================***/

static ShapeInfo *
NextShape(GeometryInfo * info)
{
    ShapeInfo *si;

    si = calloc(1, sizeof(ShapeInfo));
    if (si)
    {
        info->shapes = (ShapeInfo *) AddCommonInfo(&info->shapes->defs,
                                                   (CommonInfo *) si);
        info->nShapes++;
        si->dfltCornerRadius = info->dfltCornerRadius;
    }
    return si;
}

static ShapeInfo *
FindShape(GeometryInfo * info, Atom name, const char *type, const char *which)
{
    ShapeInfo *old;

    for (old = info->shapes; old != NULL; old = (ShapeInfo *) old->defs.next)
    {
        if (name == old->name)
            return old;
    }
    if (type != NULL)
    {
        old = info->shapes;
        WARN("Unknown shape \"%s\" for %s %s\n",
              XkbAtomText(info->dpy, name, XkbMessage), type, which);
        if (old)
        {
            ACTION("Using default shape %s instead\n",
                    shText(info->dpy, old));
            return old;
        }
        ACTION("No default shape; definition ignored\n");
        return NULL;
    }
    return NULL;
}

static Bool
AddShape(GeometryInfo * info, ShapeInfo * new)
{
    ShapeInfo *old;

    old = FindShape(info, new->name, NULL, NULL);
    if (old != NULL)
    {
        if ((new->defs.merge == MergeReplace)
            || (new->defs.merge == MergeOverride))
        {
            ShapeInfo *next = (ShapeInfo *) old->defs.next;
            if (((old->defs.fileID == new->defs.fileID)
                 && (warningLevel > 0)) || (warningLevel > 9))
            {
                WARN("Duplicate shape name \"%s\"\n",
                      shText(info->dpy, old));
                ACTION("Using last definition\n");
            }
            *old = *new;
            old->defs.next = &next->defs;
            return True;
        }
        if (((old->defs.fileID == new->defs.fileID) && (warningLevel > 0))
            || (warningLevel > 9))
        {
            WARN("Multiple shapes named \"%s\"\n", shText(info->dpy, old));
            ACTION("Using first definition\n");
        }
        return True;
    }
    old = new;
    if ((new = NextShape(info)) == NULL)
        return False;
    *new = *old;
    new->defs.next = NULL;
    old->szOutlines = old->nOutlines = 0;
    old->outlines = NULL;
    old->approx = NULL;
    old->primary = NULL;
    return True;
}

/***====================================================================***/

static void
ReplaceDoodad(DoodadInfo * into, DoodadInfo * from)
{
    CommonInfo *next;

    next = into->defs.next;
    ClearDoodadInfo(into);
    *into = *from;
    into->defs.next = next;
    next = from->defs.next;
    ClearDoodadInfo(from);
    from->defs.next = next;
    return;
}

static DoodadInfo *
NextDfltDoodad(SectionInfo * si, GeometryInfo * info)
{
    DoodadInfo *di;

    di = calloc(1, sizeof(DoodadInfo));
    if (!di)
        return NULL;
    if (si)
    {
        si->dfltDoodads =
            (DoodadInfo *) AddCommonInfo(&si->dfltDoodads->defs,
                                         (CommonInfo *) di);
    }
    else
    {
        info->dfltDoodads =
            (DoodadInfo *) AddCommonInfo(&info->dfltDoodads->defs,
                                         (CommonInfo *) di);
    }
    return di;
}

static DoodadInfo *
NextDoodad(SectionInfo * si, GeometryInfo * info)
{
    DoodadInfo *di;

    di = calloc(1, sizeof(DoodadInfo));
    if (di)
    {
        if (si)
        {
            si->doodads = (DoodadInfo *) AddCommonInfo(&si->doodads->defs,
                                                       (CommonInfo *) di);
            si->nDoodads++;
        }
        else
        {
            info->doodads =
                (DoodadInfo *) AddCommonInfo(&info->doodads->defs,
                                             (CommonInfo *) di);
            info->nDoodads++;
        }
    }
    return di;
}

static Bool
AddDoodad(SectionInfo * si, GeometryInfo * info, DoodadInfo * new)
{
    DoodadInfo *old;

    old = FindDoodadByName((si ? si->doodads : info->doodads), new->name);
    if (old != NULL)
    {
        if ((new->defs.merge == MergeReplace)
            || (new->defs.merge == MergeOverride))
        {
            if (((old->defs.fileID == new->defs.fileID)
                 && (warningLevel > 0)) || (warningLevel > 9))
            {
                WARN("Multiple doodads named \"%s\"\n",
                      XkbAtomText(info->dpy, old->name, XkbMessage));
                ACTION("Using last definition\n");
            }
            ReplaceDoodad(old, new);
            old->section = si;
            return True;
        }
        if (((old->defs.fileID == new->defs.fileID) && (warningLevel > 0))
            || (warningLevel > 9))
        {
            WARN("Multiple doodads named \"%s\"\n",
                  XkbAtomText(info->dpy, old->name, XkbMessage));
            ACTION("Using first definition\n");
        }
        return True;
    }
    old = new;
    if ((new = NextDoodad(si, info)) == NULL)
        return False;
    ReplaceDoodad(new, old);
    new->section = si;
    new->defs.next = NULL;
    return True;
}

static DoodadInfo *
FindDfltDoodadByTypeName(const char *name, SectionInfo *si, GeometryInfo *info)
{
    DoodadInfo *dflt;
    unsigned type;

    if (uStrCaseCmp(name, "outline") == 0)
        type = XkbOutlineDoodad;
    else if (uStrCaseCmp(name, "solid") == 0)
        type = XkbSolidDoodad;
    else if (uStrCaseCmp(name, "text") == 0)
        type = XkbTextDoodad;
    else if (uStrCaseCmp(name, "indicator") == 0)
        type = XkbIndicatorDoodad;
    else if (uStrCaseCmp(name, "logo") == 0)
        type = XkbLogoDoodad;
    else
        return NULL;
    if ((si) && (si->dfltDoodads))
        dflt = FindDoodadByType(si->dfltDoodads, type);
    else
        dflt = NULL;
    if ((!dflt) && (info->dfltDoodads))
        dflt = FindDoodadByType(info->dfltDoodads, type);
    if (dflt == NULL)
    {
        dflt = NextDfltDoodad(si, info);
        if (dflt != NULL)
        {
            dflt->name = None;
            dflt->type = type;
        }
    }
    return dflt;
}

/***====================================================================***/

static Bool
AddOverlay(SectionInfo * si, GeometryInfo * info, OverlayInfo * new)
{
    OverlayInfo *old;

    for (old = si->overlays; old != NULL;
         old = (OverlayInfo *) old->defs.next)
    {
        if (old->name == new->name)
            break;
    }
    if (old != NULL)
    {
        if ((new->defs.merge == MergeReplace)
            || (new->defs.merge == MergeOverride))
        {
            if (((old->defs.fileID == new->defs.fileID)
                 && (warningLevel > 0)) || (warningLevel > 9))
            {
                WARN
                    ("Multiple overlays named \"%s\" for section \"%s\"\n",
                     XkbAtomText(info->dpy, old->name, XkbMessage),
                     XkbAtomText(info->dpy, si->name, XkbMessage));
                ACTION("Using last definition\n");
            }
            ClearOverlayInfo(old);
            old->nKeys = new->nKeys;
            old->keys = new->keys;
            new->nKeys = 0;
            new->keys = NULL;
            return True;
        }
        if (((old->defs.fileID == new->defs.fileID) && (warningLevel > 0))
            || (warningLevel > 9))
        {
            WARN("Multiple doodads named \"%s\" in section \"%s\"\n",
                  XkbAtomText(info->dpy, old->name, XkbMessage),
                  XkbAtomText(info->dpy, si->name, XkbMessage));
            ACTION("Using first definition\n");
        }
        return True;
    }
    old = new;
    new = calloc(1, sizeof(OverlayInfo));
    if (!new)
    {
        if (warningLevel > 0)
        {
            WSGO("Couldn't allocate a new OverlayInfo\n");
            ACTION
                ("Overlay \"%s\" in section \"%s\" will be incomplete\n",
                 XkbAtomText(info->dpy, old->name, XkbMessage),
                 XkbAtomText(info->dpy, si->name, XkbMessage));
        }
        return False;
    }
    *new = *old;
    old->nKeys = 0;
    old->keys = NULL;
    si->overlays = (OverlayInfo *) AddCommonInfo(&si->overlays->defs,
                                                 (CommonInfo *) new);
    si->nOverlays++;
    return True;
}

/***====================================================================***/

static SectionInfo *
NextSection(GeometryInfo * info)
{
    SectionInfo *si;

    si = malloc(sizeof(SectionInfo));
    if (si)
    {
        *si = info->dfltSection;
        si->defs.defined &= ~_GS_Default;
        si->defs.next = NULL;
        si->nRows = 0;
        si->rows = NULL;
        info->sections =
            (SectionInfo *) AddCommonInfo(&info->sections->defs,
                                          (CommonInfo *) si);
        info->nSections++;
    }
    return si;
}

static SectionInfo *
FindMatchingSection(GeometryInfo *info, const SectionInfo *new)
{
    for (SectionInfo *old = info->sections; old != NULL;
         old = (SectionInfo *) old->defs.next)
    {
        if (new->name == old->name)
            return old;
    }
    return NULL;
}

static Bool
AddSection(GeometryInfo * info, SectionInfo * new)
{
    SectionInfo *old;

    old = FindMatchingSection(info, new);
    if (old != NULL)
    {
#ifdef NOTDEF
        if ((new->defs.merge == MergeReplace)
            || (new->defs.merge == MergeOverride))
        {
            SectionInfo *next = (SectionInfo *) old->defs.next;
            if (((old->defs.fileID == new->defs.fileID)
                 && (warningLevel > 0)) || (warningLevel > 9))
            {
                WARN("Duplicate shape name \"%s\"\n",
                      shText(info->dpy, old));
                ACTION("Using last definition\n");
            }
            *old = *new;
            old->defs.next = &next->defs;
            return True;
        }
        if (((old->defs.fileID == new->defs.fileID) && (warningLevel > 0))
            || (warningLevel > 9))
        {
            WARN("Multiple shapes named \"%s\"\n", shText(info->dpy, old));
            ACTION("Using first definition\n");
        }
        return True;
#else
        WARN("Don't know how to merge sections yet\n");
#endif
    }
    old = new;
    if ((new = NextSection(info)) == NULL)
        return False;
    *new = *old;
    new->defs.next = NULL;
    old->nRows = old->nDoodads = old->nOverlays = 0;
    old->rows = NULL;
    old->doodads = NULL;
    old->overlays = NULL;
    if (new->doodads)
    {
        DoodadInfo *di;
        for (di = new->doodads; di; di = (DoodadInfo *) di->defs.next)
        {
            di->section = new;
        }
    }
    return True;
}

/***====================================================================***/

static RowInfo *
NextRow(SectionInfo * si)
{
    RowInfo *row;

    row = malloc(sizeof(RowInfo));
    if (row)
    {
        *row = si->dfltRow;
        row->defs.defined &= ~_GR_Default;
        row->defs.next = NULL;
        row->nKeys = 0;
        row->keys = NULL;
        si->rows =
            (RowInfo *) AddCommonInfo(&si->rows->defs, (CommonInfo *) row);
        row->index = si->nRows++;
    }
    return row;
}

static Bool
AddRow(SectionInfo * si, RowInfo * new)
{
    RowInfo *old;

    old = new;
    if ((new = NextRow(si)) == NULL)
        return False;
    *new = *old;
    new->defs.next = NULL;
    old->nKeys = 0;
    old->keys = NULL;
    return True;
}

/***====================================================================***/

static KeyInfo *
NextKey(RowInfo * row)
{
    KeyInfo *key;

    key = malloc(sizeof(KeyInfo));
    if (key)
    {
        *key = row->dfltKey;
        key->defs.defined &= ~_GK_Default;
        key->defs.next = NULL;
        key->index = row->nKeys++;
    }
    return key;
}

static Bool
AddKey(RowInfo * row, KeyInfo * new)
{
    KeyInfo *old;

    old = new;
    if ((new = NextKey(row)) == NULL)
        return False;
    *new = *old;
    new->defs.next = NULL;
    row->keys =
        (KeyInfo *) AddCommonInfo(&row->keys->defs, (CommonInfo *) new);
    return True;
}

/***====================================================================***/

static void
MergeIncludedGeometry(GeometryInfo * into, GeometryInfo * from,
                      unsigned merge)
{
    Bool clobber;

    if (from->errorCount > 0)
    {
        into->errorCount += from->errorCount;
        return;
    }
    clobber = (merge == MergeOverride) || (merge == MergeReplace);
    if (into->name == NULL)
    {
        into->name = from->name;
        from->name = NULL;
    }
    if ((into->widthMM == 0) || ((from->widthMM != 0) && clobber))
        into->widthMM = from->widthMM;
    if ((into->heightMM == 0) || ((from->heightMM != 0) && clobber))
        into->heightMM = from->heightMM;
    if ((into->font == None) || ((from->font != None) && clobber))
        into->font = from->font;
    if ((into->fontSlant == None) || ((from->fontSlant != None) && clobber))
        into->fontSlant = from->fontSlant;
    if ((into->fontWeight == None) || ((from->fontWeight != None) && clobber))
        into->fontWeight = from->fontWeight;
    if ((into->fontSetWidth == None)
        || ((from->fontSetWidth != None) && clobber))
        into->fontSetWidth = from->fontSetWidth;
    if ((into->fontVariant == None)
        || ((from->fontVariant != None) && clobber))
        into->fontVariant = from->fontVariant;
    if ((into->fontSize == 0) || ((from->fontSize != 0) && clobber))
        into->fontSize = from->fontSize;
    if ((into->fontEncoding == None)
        || ((from->fontEncoding != None) && clobber))
        into->fontEncoding = from->fontEncoding;
    if ((into->fontSpec == None) || ((from->fontSpec != None) && clobber))
        into->fontSpec = from->fontSpec;
    if ((into->baseColor == None) || ((from->baseColor != None) && clobber))
        into->baseColor = from->baseColor;
    if ((into->labelColor == None) || ((from->labelColor != None) && clobber))
        into->labelColor = from->labelColor;
    into->nextPriority = from->nextPriority;
    if (from->props != NULL)
    {
        PropertyInfo *pi;
        for (pi = from->props; pi; pi = (PropertyInfo *) pi->defs.next)
        {
            if (!AddProperty(into, pi))
                into->errorCount++;
        }
    }
    if (from->shapes != NULL)
    {
        ShapeInfo *si;

        for (si = from->shapes; si; si = (ShapeInfo *) si->defs.next)
        {
            if (!AddShape(into, si))
                into->errorCount++;
        }
    }
    if (from->sections != NULL)
    {
        SectionInfo *si;

        for (si = from->sections; si; si = (SectionInfo *) si->defs.next)
        {
            if (!AddSection(into, si))
                into->errorCount++;
        }
    }
    if (from->doodads != NULL)
    {
        DoodadInfo *di;

        for (di = from->doodads; di; di = (DoodadInfo *) di->defs.next)
        {
            if (!AddDoodad(NULL, into, di))
                into->errorCount++;
        }
    }
    if (!MergeAliases(&into->aliases, &from->aliases, merge))
        into->errorCount++;
    return;
}

typedef void (*FileHandler) (XkbFile * /* file */ ,
                             XkbDescPtr /* xkb */ ,
                             unsigned /* merge */ ,
                             GeometryInfo *     /* info */
    );

static Bool
HandleIncludeGeometry(IncludeStmt * stmt, XkbDescPtr xkb, GeometryInfo * info,
                      FileHandler hndlr)
{
    unsigned newMerge;
    XkbFile *rtrn;
    GeometryInfo included;
    Bool haveSelf;

    haveSelf = False;
    if ((stmt->file == NULL) && (stmt->map == NULL))
    {
        haveSelf = True;
        included = *info;
        bzero(info, sizeof(GeometryInfo));
    }
    else if (ProcessIncludeFile(stmt, XkmGeometryIndex, &rtrn, &newMerge))
    {
        InitGeometryInfo(&included, rtrn->id, newMerge);
        included.nextPriority = info->nextPriority;
        included.dfltCornerRadius = info->dfltCornerRadius;
        DupSectionInfo(&included.dfltSection, &info->dfltSection, info);
        (*hndlr) (rtrn, xkb, MergeOverride, &included);
        if (stmt->stmt != NULL)
        {
            free(included.name);
            included.name = stmt->stmt;
            stmt->stmt = NULL;
        }
    }
    else
    {
        info->errorCount += 10;
        return False;
    }
    if ((stmt->next != NULL) && (included.errorCount < 1))
    {
        IncludeStmt *next;
        unsigned op;
        GeometryInfo next_incl;

        for (next = stmt->next; next != NULL; next = next->next)
        {
            if ((next->file == NULL) && (next->map == NULL))
            {
                haveSelf = True;
                MergeIncludedGeometry(&included, info, next->merge);
                ClearGeometryInfo(info);
            }
            else if (ProcessIncludeFile(next, XkmGeometryIndex, &rtrn, &op))
            {
                InitGeometryInfo(&next_incl, rtrn->id, op);
                next_incl.nextPriority = included.nextPriority;
                next_incl.dfltCornerRadius = included.dfltCornerRadius;
                DupSectionInfo(&next_incl.dfltSection,
                               &included.dfltSection, &included);
                (*hndlr) (rtrn, xkb, MergeOverride, &next_incl);
                MergeIncludedGeometry(&included, &next_incl, op);
                ClearGeometryInfo(&next_incl);
            }
            else
            {
                info->errorCount += 10;
                return False;
            }
        }
    }
    if (haveSelf)
        *info = included;
    else
    {
        MergeIncludedGeometry(info, &included, newMerge);
        ClearGeometryInfo(&included);
    }
    return (info->errorCount == 0);
}

static int
SetShapeField(ShapeInfo *si, const char *field,
              const ExprDef *arrayNdx, const ExprDef *value, GeometryInfo *info)
{
    ExprResult tmp;

    if ((uStrCaseCmp(field, "radius") == 0)
        || (uStrCaseCmp(field, "corner") == 0)
        || (uStrCaseCmp(field, "cornerradius") == 0))
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("key shape", field, shText(info->dpy, si));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("key shape", field,
                                 shText(info->dpy, si), "number");
        }
        if (si)
            si->dfltCornerRadius = tmp.ival;
        else
            info->dfltCornerRadius = tmp.ival;
        return True;
    }
    info->errorCount++;
    return ReportBadField("key shape", field, shText(info->dpy, si));
}

static int
SetShapeDoodadField(DoodadInfo *di, const char *field, const ExprDef *arrayNdx,
                    const ExprDef *value, SectionInfo *si, GeometryInfo *info)
{
    ExprResult tmp;
    const char *typeName;

    typeName =
        (di->type == XkbSolidDoodad ? "solid doodad" : "outline doodad");
    if ((!uStrCaseCmp(field, "corner"))
        || (!uStrCaseCmp(field, "cornerradius")))
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray(typeName, field, ddText(info->dpy, di));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType(typeName, field, ddText(info->dpy, di),
                                 "number");
        }
        di->defs.defined |= _GD_Corner;
        di->corner = tmp.ival;
        return True;
    }
    else if (uStrCaseCmp(field, "angle") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray(typeName, field, ddText(info->dpy, di));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType(typeName, field, ddText(info->dpy, di),
                                 "number");
        }
        di->defs.defined |= _GD_Angle;
        di->angle = tmp.ival;
        return True;
    }
    else if (uStrCaseCmp(field, "shape") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray(typeName, field, ddText(info->dpy, di));
        }
        if (!ExprResolveString(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType(typeName, field, ddText(info->dpy, di),
                                 "string");
        }
        di->shape = XkbInternAtom(info->dpy, tmp.str, False);
        di->defs.defined |= _GD_Shape;
        return True;
    }
    return ReportBadField(typeName, field, ddText(info->dpy, di));
}

#define	FIELD_STRING	0
#define	FIELD_SHORT	1
#define	FIELD_USHORT	2

static int
SetTextDoodadField(DoodadInfo *di, const char *field, const ExprDef *arrayNdx,
                   const ExprDef *value, SectionInfo *si, GeometryInfo *info)
{
    ExprResult tmp;
    unsigned def;
    unsigned type;
    const char *typeName = "text doodad";
    union
    {
        Atom *str;
        short *ival;
        unsigned short *uval;
    } pField;

    if (uStrCaseCmp(field, "angle") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray(typeName, field, ddText(info->dpy, di));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType(typeName, field, ddText(info->dpy, di),
                                 "number");
        }
        di->defs.defined |= _GD_Angle;
        di->angle = tmp.ival;
        return True;
    }
    if (uStrCaseCmp(field, "width") == 0)
    {
        type = FIELD_USHORT;
        pField.uval = &di->width;
        def = _GD_Width;
    }
    else if (uStrCaseCmp(field, "height") == 0)
    {
        type = FIELD_USHORT;
        pField.uval = &di->height;
        def = _GD_Height;
    }
    else if (uStrCaseCmp(field, "text") == 0)
    {
        type = FIELD_STRING;
        pField.str = &di->text;
        def = _GD_Text;
    }
    else if (uStrCaseCmp(field, "font") == 0)
    {
        type = FIELD_STRING;
        pField.str = &di->font;
        def = _GD_Font;
    }
    else if ((uStrCaseCmp(field, "fontslant") == 0) ||
             (uStrCaseCmp(field, "slant") == 0))
    {
        type = FIELD_STRING;
        pField.str = &di->fontSlant;
        def = _GD_FontSlant;
    }
    else if ((uStrCaseCmp(field, "fontweight") == 0) ||
             (uStrCaseCmp(field, "weight") == 0))
    {
        type = FIELD_STRING;
        pField.str = &di->fontWeight;
        def = _GD_FontWeight;
    }
    else if ((uStrCaseCmp(field, "fontwidth") == 0) ||
             (uStrCaseCmp(field, "setwidth") == 0))
    {
        type = FIELD_STRING;
        pField.str = &di->fontSetWidth;
        def = _GD_FontSetWidth;
    }
    else if ((uStrCaseCmp(field, "fontvariant") == 0) ||
             (uStrCaseCmp(field, "variant") == 0))
    {
        type = FIELD_STRING;
        pField.str = &di->fontVariant;
        def = _GD_FontVariant;
    }
    else if ((uStrCaseCmp(field, "fontencoding") == 0) ||
             (uStrCaseCmp(field, "encoding") == 0))
    {
        type = FIELD_STRING;
        pField.str = &di->fontEncoding;
        def = _GD_FontEncoding;
    }
    else if ((uStrCaseCmp(field, "xfont") == 0) ||
             (uStrCaseCmp(field, "xfontname") == 0))
    {
        type = FIELD_STRING;
        pField.str = &di->fontSpec;
        def = _GD_FontSpec;
    }
    else if (uStrCaseCmp(field, "fontsize") == 0)
    {
        type = FIELD_USHORT;
        pField.uval = &di->fontSize;
        def = _GD_FontSize;
    }
    else
    {
        return ReportBadField(typeName, field, ddText(info->dpy, di));
    }
    if (arrayNdx != NULL)
    {
        info->errorCount++;
        return ReportNotArray(typeName, field, ddText(info->dpy, di));
    }
    if (type == FIELD_STRING)
    {
        if (!ExprResolveString(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType(typeName, field, ddText(info->dpy, di),
                                 "string");
        }
        di->defs.defined |= def;
        *pField.str = XkbInternAtom(NULL, tmp.str, False);
    }
    else
    {
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType(typeName, field, ddText(info->dpy, di),
                                 "number");
        }
        if ((type == FIELD_USHORT) && (tmp.ival < 0))
        {
            info->errorCount++;
            return
                ReportBadType(typeName, field, ddText(info->dpy, di),
                              "unsigned");
        }
        di->defs.defined |= def;
        if (type == FIELD_USHORT)
            *pField.uval = tmp.uval;
        else
            *pField.ival = tmp.ival;
    }
    return True;
}

static int
SetIndicatorDoodadField(DoodadInfo * di, const char *field,
                        const ExprDef *arrayNdx, const ExprDef *value,
                        SectionInfo *si, GeometryInfo *info)
{
    ExprResult tmp;

    if ((uStrCaseCmp(field, "oncolor") == 0)
        || (uStrCaseCmp(field, "offcolor") == 0)
        || (uStrCaseCmp(field, "shape") == 0))
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("indicator doodad", field,
                                  ddText(info->dpy, di));
        }
        if (!ExprResolveString(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("indicator doodad", field,
                                 ddText(info->dpy, di), "string");
        }
        if (uStrCaseCmp(field, "oncolor") == 0)
        {
            di->defs.defined |= _GD_Color;
            di->color = XkbInternAtom(NULL, tmp.str, False);
        }
        else if (uStrCaseCmp(field, "offcolor") == 0)
        {
            di->defs.defined |= _GD_OffColor;
            di->offColor = XkbInternAtom(NULL, tmp.str, False);
        }
        else if (uStrCaseCmp(field, "shape") == 0)
        {
            di->defs.defined |= _GD_Shape;
            di->shape = XkbInternAtom(info->dpy, tmp.str, False);
        }
        return True;
    }
    return ReportBadField("indicator doodad", field, ddText(info->dpy, di));
}

static int
SetLogoDoodadField(DoodadInfo *di, const char *field, const ExprDef *arrayNdx,
                   const ExprDef *value, SectionInfo *si, GeometryInfo *info)
{
    ExprResult tmp;
    const char *typeName = "logo doodad";

    if ((!uStrCaseCmp(field, "corner"))
        || (!uStrCaseCmp(field, "cornerradius")))
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray(typeName, field, ddText(info->dpy, di));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType(typeName, field, ddText(info->dpy, di),
                                 "number");
        }
        di->defs.defined |= _GD_Corner;
        di->corner = tmp.ival;
        return True;
    }
    else if (uStrCaseCmp(field, "angle") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray(typeName, field, ddText(info->dpy, di));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType(typeName, field, ddText(info->dpy, di),
                                 "number");
        }
        di->defs.defined |= _GD_Angle;
        di->angle = tmp.ival;
        return True;
    }
    else if (uStrCaseCmp(field, "shape") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray(typeName, field, ddText(info->dpy, di));
        }
        if (!ExprResolveString(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType(typeName, field, ddText(info->dpy, di),
                                 "string");
        }
        di->shape = XkbInternAtom(info->dpy, tmp.str, False);
        di->defs.defined |= _GD_Shape;
        return True;
    }
    else if ((!uStrCaseCmp(field, "logoname"))
             || (!uStrCaseCmp(field, "name")))
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray(typeName, field, ddText(info->dpy, di));
        }
        if (!ExprResolveString(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType(typeName, field, ddText(info->dpy, di),
                                 "string");
        }
        di->logoName = uStringDup(tmp.str);
        return True;
    }
    return ReportBadField(typeName, field, ddText(info->dpy, di));
}

static int
SetDoodadField(DoodadInfo *di, const char *field,
               const ExprDef *arrayNdx, const ExprDef *value,
               SectionInfo *si, GeometryInfo *info)
{
    ExprResult tmp;

    if (uStrCaseCmp(field, "priority") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("doodad", field, ddText(info->dpy, di));
        }
        if (!ExprResolveInteger(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("doodad", field, ddText(info->dpy, di),
                                 "integer");
        }
        if ((tmp.ival < 0) || (tmp.ival > XkbGeomMaxPriority))
        {
            info->errorCount++;
            ERROR("Doodad priority %d out of range (must be 0..%d)\n",
                   tmp.ival, XkbGeomMaxPriority);
            ACTION("Priority for doodad %s not changed",
                    ddText(info->dpy, di));
            return False;
        }
        di->defs.defined |= _GD_Priority;
        di->priority = tmp.ival;
        return True;
    }
    else if (uStrCaseCmp(field, "left") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("doodad", field, ddText(info->dpy, di));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("doodad", field, ddText(info->dpy, di),
                                 "number");
        }
        di->defs.defined |= _GD_Left;
        di->left = tmp.ival;
        return True;
    }
    else if (uStrCaseCmp(field, "top") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("doodad", field, ddText(info->dpy, di));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("doodad", field, ddText(info->dpy, di),
                                 "number");
        }
        di->defs.defined |= _GD_Top;
        di->top = tmp.ival;
        return True;
    }
    else if (uStrCaseCmp(field, "color") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("doodad", field, ddText(info->dpy, di));
        }
        if (!ExprResolveString(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("doodad", field, ddText(info->dpy, di),
                                 "string");
        }
        di->defs.defined |= _GD_Color;
        di->color = XkbInternAtom(NULL, tmp.str, False);
        return True;
    }
    switch (di->type)
    {
    case XkbOutlineDoodad:
    case XkbSolidDoodad:
        return SetShapeDoodadField(di, field, arrayNdx, value, si, info);
    case XkbTextDoodad:
        return SetTextDoodadField(di, field, arrayNdx, value, si, info);
    case XkbIndicatorDoodad:
        return SetIndicatorDoodadField(di, field, arrayNdx, value, si, info);
    case XkbLogoDoodad:
        return SetLogoDoodadField(di, field, arrayNdx, value, si, info);
    }
    WSGO("Unknown doodad type %d in SetDoodadField\n",
          (unsigned int) di->type);
    ACTION("Definition of %s in %s ignored\n", field, ddText(info->dpy, di));
    return False;
}

static int
SetSectionField(SectionInfo *si, const char *field,
                const ExprDef *arrayNdx, const ExprDef *value,
                GeometryInfo *info)
{
    unsigned short *pField;
    unsigned def;
    ExprResult tmp;

    pField = NULL;
    def = 0;
    if (uStrCaseCmp(field, "priority") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("keyboard section", field,
                                  scText(info->dpy, si));
        }
        if (!ExprResolveInteger(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            ReportBadType("keyboard section", field,
                          scText(info->dpy, si), "integer");
            return False;
        }
        if ((tmp.ival < 0) || (tmp.ival > XkbGeomMaxPriority))
        {
            info->errorCount++;
            ERROR("Section priority %d out of range (must be 0..%d)\n",
                   tmp.ival, XkbGeomMaxPriority);
            ACTION("Priority for section %s not changed",
                    scText(info->dpy, si));
            return False;
        }
        si->priority = tmp.ival;
        si->defs.defined |= _GS_Priority;
        return True;
    }
    else if (uStrCaseCmp(field, "top") == 0)
    {
        pField = &si->top;
        def = _GS_Top;
    }
    else if (uStrCaseCmp(field, "left") == 0)
    {
        pField = &si->left;
        def = _GS_Left;
    }
    else if (uStrCaseCmp(field, "width") == 0)
    {
        pField = &si->width;
        def = _GS_Width;
    }
    else if (uStrCaseCmp(field, "height") == 0)
    {
        pField = &si->height;
        def = _GS_Height;
    }
    else if (uStrCaseCmp(field, "angle") == 0)
    {
        pField = &si->angle;
        def = _GS_Angle;
    }
    else
    {
        info->errorCount++;
        return ReportBadField("keyboard section", field,
                              scText(info->dpy, si));
    }
    if (arrayNdx != NULL)
    {
        info->errorCount++;
        return ReportNotArray("keyboard section", field,
                              scText(info->dpy, si));
    }
    if (!ExprResolveFloat(value, &tmp, NULL, NULL))
    {
        info->errorCount++;
        ReportBadType("keyboard section", field, scText(info->dpy, si),
                      "number");
        return False;
    }
    si->defs.defined |= def;
    *pField = tmp.uval;
    return True;
}

static int
SetRowField(RowInfo *row, const char *field,
            const ExprDef *arrayNdx, const ExprDef *value, GeometryInfo *info)
{
    ExprResult tmp;

    if (uStrCaseCmp(field, "top") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("keyboard row", field,
                                  rowText(info->dpy, row));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("keyboard row", field,
                                 rowText(info->dpy, row), "number");
        }
        row->defs.defined |= _GR_Top;
        row->top = tmp.uval;
    }
    else if (uStrCaseCmp(field, "left") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("keyboard row", field,
                                  rowText(info->dpy, row));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("keyboard row", field,
                                 rowText(info->dpy, row), "number");
        }
        row->defs.defined |= _GR_Left;
        row->left = tmp.uval;
    }
    else if (uStrCaseCmp(field, "vertical") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("keyboard row", field,
                                  rowText(info->dpy, row));
        }
        if (!ExprResolveBoolean(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("keyboard row", field,
                                 rowText(info->dpy, row), "boolean");
        }
        row->defs.defined |= _GR_Vertical;
        row->vertical = tmp.uval;
    }
    else
    {
        info->errorCount++;
        return ReportBadField("keyboard row", field, rowText(info->dpy, row));
    }
    return True;
}

static int
SetKeyField(KeyInfo *key, const char *field,
            const ExprDef *arrayNdx, const ExprDef *value, GeometryInfo *info)
{
    ExprResult tmp;

    if (uStrCaseCmp(field, "gap") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("key", field, keyText(key));
        }
        if (!ExprResolveFloat(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("key", field, keyText(key), "number");
        }
        key->defs.defined |= _GK_Gap;
        key->gap = tmp.ival;
    }
    else if (uStrCaseCmp(field, "shape") == 0)
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("key", field, keyText(key));
        }
        if (!ExprResolveString(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("key", field, keyText(key), "string");
        }
        key->defs.defined |= _GK_Shape;
        key->shape = XkbInternAtom(info->dpy, tmp.str, False);
    }
    else if ((uStrCaseCmp(field, "color") == 0) ||
             (uStrCaseCmp(field, "keycolor") == 0))
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("key", field, keyText(key));
        }
        if (!ExprResolveString(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("key", field, keyText(key), "string");
        }
        key->defs.defined |= _GK_Color;
        key->color = XkbInternAtom(NULL, tmp.str, False);
    }
    else if ((uStrCaseCmp(field, "name") == 0)
             || (uStrCaseCmp(field, "keyname") == 0))
    {
        if (arrayNdx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("key", field, keyText(key));
        }
        if (!ExprResolveKeyName(value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("key", field, keyText(key), "key name");
        }
        key->defs.defined |= _GK_Name;
        bzero(key->name, XkbKeyNameLength + 1);
        strncpy(key->name, tmp.keyName.name, XkbKeyNameLength);
    }
    else
    {
        info->errorCount++;
        return ReportBadField("key", field, keyText(key));
    }
    return True;
}

static int
SetGeometryProperty(GeometryInfo *info, char *property, const ExprDef *value)
{
    PropertyInfo pi;
    ExprResult result;

    InitPropertyInfo(&pi, info);
    pi.name = property;
    if (!ExprResolveString(value, &result, NULL, NULL))
    {
        info->errorCount++;
        ERROR("Property values must be type string\n");
        ACTION("Ignoring illegal definition of \"%s\" property\n", property);
        return False;
    }
    pi.value = result.str;
    return AddProperty(info, &pi);
}

static int
HandleGeometryVar(VarDef * stmt, XkbDescPtr xkb, GeometryInfo * info)
{
    ExprResult elem, field, tmp;
    ExprDef *ndx;
    DoodadInfo *di;
    Atom *pField;

    if (ExprResolveLhs(stmt->name, &elem, &field, &ndx) == 0)
        return 0;               /* internal error, already reported */
    if (elem.str && (uStrCaseCmp(elem.str, "shape") == 0))
        return SetShapeField(NULL, field.str, ndx, stmt->value, info);
    if (elem.str && (uStrCaseCmp(elem.str, "key") == 0))
        return SetKeyField(&info->dfltSection.dfltRow.dfltKey,
                           field.str, ndx, stmt->value, info);
    if (elem.str && (uStrCaseCmp(elem.str, "row") == 0))
        return SetRowField(&info->dfltSection.dfltRow, field.str, ndx,
                           stmt->value, info);
    if (elem.str && (uStrCaseCmp(elem.str, "section") == 0))
    {
        return SetSectionField(&info->dfltSection, field.str, ndx,
                               stmt->value, info);
    }
    if (elem.str && (uStrCaseCmp(elem.str, "property") == 0))
    {
        if (ndx != NULL)
        {
            info->errorCount++;
            ERROR("The %s geometry property is not an array\n", field.str);
            ACTION("Ignoring illegal property definition\n");
            return False;
        }
        return SetGeometryProperty(info, field.str, stmt->value);
    }
    if (elem.str
        && ((di = FindDfltDoodadByTypeName(elem.str, NULL, info)) != NULL))
    {
        return SetDoodadField(di, field.str, ndx, stmt->value, NULL, info);
    }
    if (elem.str && (uStrCaseCmp(elem.str, "solid") == 0))
    {
        DoodadInfo *dflt;
        dflt = FindDoodadByType(info->dfltDoodads, XkbSolidDoodad);
        if (dflt == NULL)
            dflt = NextDfltDoodad(NULL, info);
        return SetDoodadField(dflt, field.str, ndx, stmt->value, NULL, info);
    }
    if (elem.str && (uStrCaseCmp(elem.str, "outline") == 0))
    {
        DoodadInfo *dflt;
        dflt = FindDoodadByType(info->dfltDoodads, XkbOutlineDoodad);
        if (dflt == NULL)
            dflt = NextDfltDoodad(NULL, info);
        return SetDoodadField(dflt, field.str, ndx, stmt->value, NULL, info);
    }
    if (elem.str && (uStrCaseCmp(elem.str, "text") == 0))
    {
        DoodadInfo *dflt;
        dflt = FindDoodadByType(info->dfltDoodads, XkbTextDoodad);
        if (dflt == NULL)
            dflt = NextDfltDoodad(NULL, info);
        return SetDoodadField(dflt, field.str, ndx, stmt->value, NULL, info);
    }
    if (elem.str && (uStrCaseCmp(elem.str, "indicator") == 0))
    {
        DoodadInfo *dflt;
        dflt = FindDoodadByType(info->dfltDoodads, XkbIndicatorDoodad);
        if (dflt == NULL)
            dflt = NextDfltDoodad(NULL, info);
        return SetDoodadField(dflt, field.str, ndx, stmt->value, NULL, info);
    }
    if (elem.str && (uStrCaseCmp(elem.str, "logo") == 0))
    {
        DoodadInfo *dflt;
        dflt = FindDoodadByType(info->dfltDoodads, XkbLogoDoodad);
        if (dflt == NULL)
            dflt = NextDfltDoodad(NULL, info);
        return SetDoodadField(dflt, field.str, ndx, stmt->value, NULL, info);
    }
    if (elem.str)
    {
        WARN("Assignment to field of unknown element\n");
        ACTION("No value assigned to %s.%s\n", elem.str, field.str);
        return False;
    }

    if ((uStrCaseCmp(field.str, "width") == 0) ||
        (uStrCaseCmp(field.str, "widthmm") == 0))
    {
        if (ndx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("keyboard", field.str, "geometry");
        }
        if (!ExprResolveFloat(stmt->value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("keyboard", field.str, "geometry", "number");
        }
        if (tmp.ival < 1)
        {
            WARN("Keyboard width must be positive\n");
            ACTION("Ignoring illegal keyboard width %s\n",
                    XkbGeomFPText(tmp.ival, XkbMessage));
            return True;
        }
        if (info->widthMM != 0)
        {
            WARN("Keyboard width multiply defined\n");
            ACTION("Using last definition (%s),",
                    XkbGeomFPText(tmp.ival, XkbMessage));
            INFO(" ignoring first (%s)\n",
                  XkbGeomFPText(info->widthMM, XkbMessage));
        }
        info->widthMM = tmp.ival;
        return True;
    }
    else if ((uStrCaseCmp(field.str, "height") == 0) ||
             (uStrCaseCmp(field.str, "heightmm") == 0))
    {
        if (ndx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("keyboard", field.str, "geometry");
        }
        if (!ExprResolveFloat(stmt->value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("keyboard", field.str, "geometry", "number");
        }
        if (tmp.ival < 1)
        {
            WARN("Keyboard height must be positive\n");
            ACTION("Ignoring illegal keyboard height %s\n",
                    XkbGeomFPText(tmp.ival, XkbMessage));
            return True;
        }
        if (info->heightMM != 0)
        {
            WARN("Keyboard height multiply defined\n");
            ACTION("Using last definition (%s),",
                    XkbGeomFPText(tmp.ival, XkbMessage));
            INFO(" ignoring first (%s)\n",
                  XkbGeomFPText(info->heightMM, XkbMessage));
        }
        info->heightMM = tmp.ival;
        return True;
    }
    else if (uStrCaseCmp(field.str, "font") == 0)
    {
        pField = &info->font;
    }
    else if ((uStrCaseCmp(field.str, "fontslant") == 0) ||
             (uStrCaseCmp(field.str, "slant") == 0))
    {
        pField = &info->fontSlant;
    }
    else if ((uStrCaseCmp(field.str, "fontweight") == 0) ||
             (uStrCaseCmp(field.str, "weight") == 0))
    {
        pField = &info->fontWeight;
    }
    else if ((uStrCaseCmp(field.str, "fontwidth") == 0) ||
             (uStrCaseCmp(field.str, "setwidth") == 0))
    {
        pField = &info->fontWeight;
    }
    else if ((uStrCaseCmp(field.str, "fontencoding") == 0) ||
             (uStrCaseCmp(field.str, "encoding") == 0))
    {
        pField = &info->fontEncoding;
    }
    else if ((uStrCaseCmp(field.str, "xfont") == 0) ||
             (uStrCaseCmp(field.str, "xfontname") == 0))
    {
        pField = &info->fontSpec;
    }
    else if (uStrCaseCmp(field.str, "fontsize") == 0)
    {
        if (ndx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("keyboard", field.str, "geometry");
        }
        if (!ExprResolveFloat(stmt->value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("keyboard", field.str, "geometry", "number");
        }
        if ((tmp.ival < 40) || (tmp.ival > 2550))
        {
            info->errorCount++;
            ERROR("Illegal font size %d (must be 4..255)\n", tmp.ival);
            ACTION("Ignoring font size in keyboard geometry\n");
            return False;
        }
        info->fontSize = tmp.ival;
        return True;
    }
    else if ((uStrCaseCmp(field.str, "color") == 0) ||
             (uStrCaseCmp(field.str, "basecolor") == 0))
    {
        if (ndx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("keyboard", field.str, "geometry");
        }
        if (!ExprResolveString(stmt->value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("keyboard", field.str, "geometry", "string");
        }
        info->baseColor = XkbInternAtom(NULL, tmp.str, False);
        return True;
    }
    else if (uStrCaseCmp(field.str, "labelcolor") == 0)
    {
        if (ndx != NULL)
        {
            info->errorCount++;
            return ReportNotArray("keyboard", field.str, "geometry");
        }
        if (!ExprResolveString(stmt->value, &tmp, NULL, NULL))
        {
            info->errorCount++;
            return ReportBadType("keyboard", field.str, "geometry", "string");
        }
        info->labelColor = XkbInternAtom(NULL, tmp.str, False);
        return True;
    }
    else
    {
        return SetGeometryProperty(info, field.str, stmt->value);
    }

    if (ndx != NULL)
    {
        info->errorCount++;
        return ReportNotArray("keyboard", field.str, "geometry");
    }
    if (!ExprResolveString(stmt->value, &tmp, NULL, NULL))
    {
        info->errorCount++;
        return ReportBadType("keyboard", field.str, "geometry", "string");
    }
    *pField = XkbInternAtom(NULL, tmp.str, False);
    return True;
}

/***====================================================================***/

static Bool
HandleShapeBody(ShapeDef * def, ShapeInfo * si, unsigned merge,
                GeometryInfo * info)
{
    OutlineDef *ol;
    int nOut, nPt;
    XkbOutlinePtr outline;
    ExprDef *pt;

    if (def->nOutlines < 1)
    {
        WARN("Shape \"%s\" has no outlines\n", shText(info->dpy, si));
        ACTION("Definition ignored\n");
        return True;
    }
    si->nOutlines = def->nOutlines;
    si->outlines = calloc(def->nOutlines, sizeof(XkbOutlineRec));
    if (!si->outlines)
    {
        ERROR("Couldn't allocate outlines for \"%s\"\n",
               shText(info->dpy, si));
        ACTION("Definition ignored\n");
        info->errorCount++;
        return False;
    }
    for (nOut = 0, ol = def->outlines; ol != NULL;
         ol = (OutlineDef *) ol->common.next)
    {
        if (ol->nPoints < 1)
        {
            SetShapeField(si, XkbAtomGetString(NULL, ol->field), NULL,
                          ol->points, info);
            continue;
        }
        outline = &si->outlines[nOut++];
        outline->num_points = ol->nPoints;
        outline->corner_radius = si->dfltCornerRadius;
        outline->points = calloc(ol->nPoints, sizeof(XkbPointRec));
        if (!outline->points)
        {
            ERROR("Can't allocate points for \"%s\"\n",
                   shText(info->dpy, si));
            ACTION("Definition ignored\n");
            info->errorCount++;
            return False;
        }
        for (nPt = 0, pt = ol->points; pt != NULL;
             pt = (ExprDef *) pt->common.next)
        {
            outline->points[nPt].x = pt->value.coord.x;
            outline->points[nPt].y = pt->value.coord.y;
            nPt++;
        }
        if (ol->field != None)
        {
            char *str = XkbAtomText(NULL, ol->field, XkbMessage);
            if ((uStrCaseCmp(str, "approximation") == 0) ||
                (uStrCaseCmp(str, "approx") == 0))
            {
                if (si->approx == NULL)
                    si->approx = outline;
                else
                {
                    WARN("Multiple approximations for \"%s\"\n",
                          shText(info->dpy, si));
                    ACTION("Treating all but the first as normal outlines\n");
                }
            }
            else if (uStrCaseCmp(str, "primary") == 0)
            {
                if (si->primary == NULL)
                    si->primary = outline;
                else
                {
                    WARN("Multiple primary outlines for \"%s\"\n",
                          shText(info->dpy, si));
                    ACTION("Treating all but the first as normal outlines\n");
                }
            }
            else
            {
                WARN("Unknown outline type %s for \"%s\"\n", str,
                      shText(info->dpy, si));
                ACTION("Treated as a normal outline\n");
            }
        }
    }
    if (nOut != si->nOutlines)
    {
        WSGO("Expected %d outlines, got %d\n",
              (unsigned int) si->nOutlines, nOut);
        si->nOutlines = nOut;
    }
    return True;
}

static int
HandleShapeDef(ShapeDef * def, XkbDescPtr xkb, unsigned merge,
               GeometryInfo * info)
{
    ShapeInfo si;

    if (def->merge != MergeDefault)
        merge = def->merge;

    bzero(&si, sizeof(ShapeInfo));
    si.defs.merge = merge;
    si.name =
        XkbInternAtom(info->dpy, XkbAtomGetString(NULL, def->name), False);
    si.dfltCornerRadius = info->dfltCornerRadius;
    if (!HandleShapeBody(def, &si, merge, info))
        return False;
    if (!AddShape(info, &si))
        return False;
    return True;
}

/***====================================================================***/

static int
HandleDoodadDef(DoodadDef * def,
                unsigned merge, SectionInfo * si, GeometryInfo * info)
{
    ExprResult elem, field;
    ExprDef *ndx;
    DoodadInfo new;
    VarDef *var;

    if (def->common.stmtType == StmtIndicatorMapDef)
    {
        def->common.stmtType = StmtDoodadDef;
        def->type = XkbIndicatorDoodad;
    }
    InitDoodadInfo(&new, def->type, si, info);
    new.name =
        XkbInternAtom(info->dpy, XkbAtomGetString(NULL, def->name), False);
    for (var = def->body; var != NULL; var = (VarDef *) var->common.next)
    {
        if (ExprResolveLhs(var->name, &elem, &field, &ndx) == 0)
            return 0;           /* internal error, already reported */
        if (elem.str != NULL)
        {
            WARN("Assignment to field of unknown element in doodad %s\n",
                  ddText(info->dpy, &new));
            ACTION("No value assigned to %s.%s\n", elem.str, field.str);
        }
        else if (!SetDoodadField(&new, field.str, ndx, var->value, si, info))
            return False;
    }
    if (!AddDoodad(si, info, &new))
        return False;
    ClearDoodadInfo(&new);
    return True;
}

/***====================================================================***/

static int
HandleOverlayDef(OverlayDef * def,
                 unsigned merge, SectionInfo * si, GeometryInfo * info)
{
    OverlayInfo ol;

    if ((def->nKeys < 1) && (warningLevel > 3))
    {
        WARN("Overlay \"%s\" in section \"%s\" has no keys\n",
              XkbAtomText(NULL, def->name, XkbMessage), scText(info->dpy,
                                                               si));
        ACTION("Overlay ignored\n");
        return True;
    }
    bzero(&ol, sizeof(OverlayInfo));
    ol.name =
        XkbInternAtom(info->dpy, XkbAtomGetString(NULL, def->name), False);
    for (OverlayKeyDef *keyDef = def->keys; keyDef;
         keyDef = (OverlayKeyDef *) keyDef->common.next)
    {
        OverlayKeyInfo *key = calloc(1, sizeof(OverlayKeyInfo));
        if (!key)
        {
            if (warningLevel > 0)
            {
                WSGO("Couldn't allocate OverlayKeyInfo\n");
                ACTION("Overlay %s for section %s will be incomplete\n",
                        XkbAtomText(info->dpy, ol.name, XkbMessage),
                        scText(info->dpy, si));
            }
            return False;
        }
        strncpy(key->over, keyDef->over, sizeof(key->over));
        strncpy(key->under, keyDef->under, sizeof(key->under));
        key->sectionRow = _GOK_UnknownRow;
        key->overlayRow = _GOK_UnknownRow;
        ol.keys = (OverlayKeyInfo *) AddCommonInfo(&ol.keys->defs,
                                                   (CommonInfo *) key);
        ol.nKeys++;
    }
    if (!AddOverlay(si, info, &ol))
        return False;
    ClearOverlayInfo(&ol);
    return True;
}

/***====================================================================***/

static Bool
HandleComplexKey(KeyDef * def, KeyInfo * key, GeometryInfo * info)
{
    RowInfo *row = key->row;

    for (ExprDef *expr = def->expr; expr != NULL;
         expr = (ExprDef *) expr->common.next)
    {
        if (expr->op == OpAssign)
        {
            ExprResult elem, f;
            ExprDef *ndx;
            if (ExprResolveLhs(expr->value.binary.left, &elem, &f, &ndx) == 0)
                return False;   /* internal error, already reported */
            if ((elem.str == NULL) || (uStrCaseCmp(elem.str, "key") == 0))
            {
                if (!SetKeyField
                    (key, f.str, ndx, expr->value.binary.right, info))
                    return False;
            }
            else
            {
                ERROR("Illegal element used in a key definition\n");
                ACTION("Assignment to %s.%s ignored\n", elem.str, f.str);
                return False;
            }
        }
        else
        {
            switch (expr->type)
            {
            case TypeInt:
            case TypeFloat:
                if (!SetKeyField(key, "gap", NULL, expr, info))
                    return False;
                break;
            case TypeString:
                if (!SetKeyField(key, "shape", NULL, expr, info))
                    return False;
                break;
            case TypeKeyName:
                if (!SetKeyField(key, "name", NULL, expr, info))
                    return False;
                break;
            default:
                ERROR("Cannot determine field for unnamed expression\n");
                ACTION("Ignoring key %d in row %d of section %s\n",
                        row->nKeys + 1, row->section->nRows + 1,
                        rowText(info->dpy, row));
                return False;
            }
        }
    }
    return True;
}

static Bool
HandleRowBody(RowDef * def, RowInfo * row, unsigned merge,
              GeometryInfo * info)
{
    if ((def->nKeys < 1) && (warningLevel > 3))
    {
        ERROR("Row in section %s has no keys\n", rowText(info->dpy, row));
        ACTION("Section ignored\n");
        return True;
    }
    for (KeyDef *keyDef = def->keys; keyDef != NULL;
         keyDef = (KeyDef *) keyDef->common.next)
    {
        if (keyDef->common.stmtType == StmtVarDef)
        {
            VarDef *var = (VarDef *) keyDef;
            ExprResult elem, field;
            ExprDef *ndx;
            if (ExprResolveLhs(var->name, &elem, &field, &ndx) == 0)
                return 0;       /* internal error, already reported */
            if ((elem.str == NULL) || (uStrCaseCmp(elem.str, "row") == 0))
            {
                if (!SetRowField(row, field.str, ndx, var->value, info))
                    return False;
            }
            else if (uStrCaseCmp(elem.str, "key") == 0)
            {
                if (!SetKeyField
                    (&row->dfltKey, field.str, ndx, var->value, info))
                    return False;
            }
            else
            {
                WARN("Assignment to field of unknown element in row\n");
                ACTION("No value assigned to %s.%s\n", elem.str, field.str);
            }
        }
        else if (keyDef->common.stmtType == StmtKeyDef)
        {
            KeyInfo key;
            InitKeyInfo(&key, row, info);
            if (keyDef->name != NULL)
            {
                int len = strlen(keyDef->name);
                if ((len < 1) || (len > XkbKeyNameLength))
                {
                    ERROR("Illegal name %s for key in section %s\n",
                           keyDef->name, rowText(info->dpy, row));
                    ACTION("Section not compiled\n");
                    return False;
                }
                bzero(key.name, XkbKeyNameLength + 1);
                strncpy(key.name, keyDef->name, XkbKeyNameLength);
                key.defs.defined |= _GK_Name;
            }
            else if (!HandleComplexKey(keyDef, &key, info))
                return False;
            if (!AddKey(row, &key))
                return False;
        }
        else
        {
            WSGO("Unexpected statement (type %d) in row body\n",
                  keyDef->common.stmtType);
            return False;
        }
    }
    return True;
}

static Bool
HandleSectionBody(SectionDef * def,
                  SectionInfo * si, unsigned merge, GeometryInfo * info)
{
    for (RowDef *rowDef = def->rows; rowDef != NULL;
         rowDef = (RowDef *) rowDef->common.next)
    {
        if (rowDef->common.stmtType == StmtVarDef)
        {
            VarDef *var = (VarDef *) rowDef;
            ExprResult elem, field;
            ExprDef *ndx;
            DoodadInfo *di;

            if (ExprResolveLhs(var->name, &elem, &field, &ndx) == 0)
                return 0;       /* internal error, already reported */
            if ((elem.str == NULL) || (uStrCaseCmp(elem.str, "section") == 0))
            {
                if (!SetSectionField(si, field.str, ndx, var->value, info))
                    return False;
            }
            else if (uStrCaseCmp(elem.str, "row") == 0)
            {
                if (!SetRowField
                    (&si->dfltRow, field.str, ndx, var->value, info))
                    return False;
            }
            else if (uStrCaseCmp(elem.str, "key") == 0)
            {
                if (!SetKeyField(&si->dfltRow.dfltKey, field.str, ndx,
                                 var->value, info))
                    return False;
            }
            else if ((di =
                      FindDfltDoodadByTypeName(elem.str, si, info)) != NULL)
            {
                if (!SetDoodadField(di, field.str, ndx, var->value, si, info))
                    return False;
            }
            else
            {
                WARN("Assignment to field of unknown element in section\n");
                ACTION("No value assigned to %s.%s\n", elem.str, field.str);
            }
        }
        else if (rowDef->common.stmtType == StmtRowDef)
        {
            RowInfo row;
            InitRowInfo(&row, si, info);
            if (!HandleRowBody(rowDef, &row, merge, info))
                return False;
            if (!AddRow(si, &row))
                return False;
/*	    ClearRowInfo(&row,info);*/
        }
        else if ((rowDef->common.stmtType == StmtDoodadDef) ||
                 (rowDef->common.stmtType == StmtIndicatorMapDef))
        {
            if (!HandleDoodadDef((DoodadDef *) rowDef, merge, si, info))
                return False;
        }
        else if (rowDef->common.stmtType == StmtOverlayDef)
        {
            if (!HandleOverlayDef((OverlayDef *) rowDef, merge, si, info))
                return False;
        }
        else
        {
            WSGO("Unexpected statement (type %d) in section body\n",
                  rowDef->common.stmtType);
            return False;
        }
    }
    if (si->nRows != def->nRows)
    {
        WSGO("Expected %d rows, found %d\n", (unsigned int) def->nRows,
              (unsigned int) si->nRows);
        ACTION("Definition of section %s might be incorrect\n",
                scText(info->dpy, si));
    }
    return True;
}

static int
HandleSectionDef(SectionDef * def,
                 XkbDescPtr xkb, unsigned merge, GeometryInfo * info)
{
    SectionInfo si;
    char *str;

    if (def->merge != MergeDefault)
        merge = def->merge;
    InitSectionInfo(&si, info);
    si.defs.merge = merge;
    str = XkbAtomGetString(NULL, def->name);
    if ((str == NULL) || (strlen(str) < 1))
    {
        ERROR("Section defined without a name\n");
        ACTION("Definition ignored\n");
        return False;
    }
    si.name =
        XkbInternAtom(info->dpy, XkbAtomGetString(NULL, def->name), False);
    if (!HandleSectionBody(def, &si, merge, info))
        return False;
    if (!AddSection(info, &si))
        return False;
    return True;
}

/***====================================================================***/

static void
HandleGeometryFile(XkbFile * file,
                   XkbDescPtr xkb, unsigned merge, GeometryInfo * info)
{
    ParseCommon *stmt;

    if (merge == MergeDefault)
        merge = MergeAugment;
    info->name = uStringDup(file->name);
    stmt = file->defs;
    while (stmt)
    {
        const char *failWhat = NULL;

        switch (stmt->stmtType)
        {
        case StmtInclude:
            if (!HandleIncludeGeometry((IncludeStmt *) stmt, xkb, info,
                                       HandleGeometryFile))
                info->errorCount++;
            break;
        case StmtKeyAliasDef:
            if (!HandleAliasDef((KeyAliasDef *) stmt,
                                merge, info->fileID, &info->aliases))
            {
                info->errorCount++;
            }
            break;
        case StmtVarDef:
            if (!HandleGeometryVar((VarDef *) stmt, xkb, info))
                info->errorCount++;
            break;
        case StmtShapeDef:
            if (!HandleShapeDef((ShapeDef *) stmt, xkb, merge, info))
                info->errorCount++;
            break;
        case StmtSectionDef:
            if (!HandleSectionDef((SectionDef *) stmt, xkb, merge, info))
                info->errorCount++;
            break;
        case StmtIndicatorMapDef:
        case StmtDoodadDef:
            if (!HandleDoodadDef((DoodadDef *) stmt, merge, NULL, info))
                info->errorCount++;
            break;
        case StmtVModDef:
            failWhat = "virtual modifier";
            goto fail;
        case StmtInterpDef:
            failWhat = "symbol interpretation";
            goto fail;
        case StmtGroupCompatDef:
            failWhat = "group compatibility map";
            goto fail;
        case StmtKeycodeDef:
            failWhat = "key name";
            goto fail;
        fail:
            ERROR("Interpretation files may not include other types\n");
            ACTION("Ignoring %s definition.\n", failWhat);
            info->errorCount++;
            break;
        default:
            WSGO("Unexpected statement type %d in HandleGeometryFile\n",
                  stmt->stmtType);
            break;
        }
        stmt = stmt->next;
        if (info->errorCount > 10)
        {
#ifdef NOISY
            ERROR("Too many errors\n");
#endif
            ACTION("Abandoning geometry file \"%s\"\n", file->topName);
            break;
        }
    }
    return;
}

/***====================================================================***/

static Bool
CopyShapeDef(Display * dpy, XkbGeometryPtr geom, ShapeInfo * si)
{
    int n;
    XkbShapePtr shape;
    XkbOutlinePtr old_outline;
    Atom name;

    si->index = geom->num_shapes;
    name = XkbInternAtom(dpy, XkbAtomGetString(NULL, si->name), False);
    shape = XkbAddGeomShape(geom, name, si->nOutlines);
    if (!shape)
    {
        WSGO("Couldn't allocate shape in geometry\n");
        ACTION("Shape %s not compiled\n", shText(dpy, si));
        return False;
    }
    old_outline = si->outlines;
    for (int i = 0; i < si->nOutlines; i++, old_outline++)
    {
        XkbOutlinePtr outline =
            XkbAddGeomOutline(shape, old_outline->num_points);

        if (!outline)
        {
            WSGO("Couldn't allocate outline in shape\n");
            ACTION("Shape %s is incomplete\n", shText(dpy, si));
            return False;
        }
        n = old_outline->num_points;
        memcpy(outline->points, old_outline->points, n * sizeof(XkbPointRec));
        outline->num_points = old_outline->num_points;
        outline->corner_radius = old_outline->corner_radius;
    }
    if (si->approx)
    {
        n = (si->approx - si->outlines);
        shape->approx = &shape->outlines[n];
    }
    if (si->primary)
    {
        n = (si->primary - si->outlines);
        shape->primary = &shape->outlines[n];
    }
    XkbComputeShapeBounds(shape);
    return True;
}

static Bool
VerifyDoodadInfo(DoodadInfo * di, GeometryInfo * info)
{
    if ((di->defs.defined & (_GD_Top | _GD_Left)) != (_GD_Top | _GD_Left))
    {
        if (warningLevel < 9)
        {
            ERROR("No position defined for doodad %s\n",
                   ddText(info->dpy, di));
            ACTION("Illegal doodad ignored\n");
            return False;
        }
    }
    if ((di->defs.defined & _GD_Priority) == 0)
    {
        /* calculate priority -- should be just above previous doodad/row */
    }
    switch (di->type)
    {
    case XkbOutlineDoodad:
    case XkbSolidDoodad:
        if ((di->defs.defined & _GD_Shape) == 0)
        {
            ERROR("No shape defined for %s doodad %s\n",
                   (di->type == XkbOutlineDoodad ? "outline" : "filled"),
                   ddText(info->dpy, di));
            ACTION("Incomplete definition ignored\n");
            return False;
        }
        else
        {
            ShapeInfo *si;
            si = FindShape(info, di->shape,
                           (di->type ==
                            XkbOutlineDoodad ? "outline doodad" :
                            "solid doodad"), ddText(info->dpy, di));
            if (si)
                di->shape = si->name;
            else
            {
                ERROR("No legal shape for %s\n", ddText(info->dpy, di));
                ACTION("Incomplete definition ignored\n");
                return False;
            }
        }
        if ((di->defs.defined & _GD_Color) == 0)
        {
            if (warningLevel > 5)
            {
                WARN("No color for doodad %s\n", ddText(info->dpy, di));
                ACTION("Using black\n");
            }
            di->color = XkbInternAtom(NULL, "black", False);
        }
        break;
    case XkbTextDoodad:
        if ((di->defs.defined & _GD_Text) == 0)
        {
            ERROR("No text specified for text doodad %s\n",
                   ddText(info->dpy, di));
            ACTION("Illegal doodad definition ignored\n");
            return False;
        }
        if ((di->defs.defined & _GD_Angle) == 0)
            di->angle = 0;
        if ((di->defs.defined & _GD_Color) == 0)
        {
            if (warningLevel > 5)
            {
                WARN("No color specified for doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using black\n");
            }
            di->color = XkbInternAtom(NULL, "black", False);
        }
        if ((di->defs.defined & _GD_FontSpec) != 0)
        {
            if ((di->defs.defined & _GD_FontParts) == 0)
                return True;
            if (warningLevel < 9)
            {
                WARN
                    ("Text doodad %s has full and partial font definition\n",
                     ddText(info->dpy, di));
                ACTION("Full specification ignored\n");
            }
            di->defs.defined &= ~_GD_FontSpec;
            di->fontSpec = None;
        }
        if ((di->defs.defined & _GD_Font) == 0)
        {
            if (warningLevel > 5)
            {
                WARN("No font specified for doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using \"%s\"\n", DFLT_FONT);
            }
            di->font = XkbInternAtom(NULL, DFLT_FONT, False);
        }
        if ((di->defs.defined & _GD_FontSlant) == 0)
        {
            if (warningLevel > 7)
            {
                WARN("No font slant for text doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using \"%s\"\n", DFLT_SLANT);
            }
            di->fontSlant = XkbInternAtom(NULL, DFLT_SLANT, False);
        }
        if ((di->defs.defined & _GD_FontWeight) == 0)
        {
            if (warningLevel > 7)
            {
                WARN("No font weight for text doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using \"%s\"\n", DFLT_WEIGHT);
            }
            di->fontWeight = XkbInternAtom(NULL, DFLT_WEIGHT, False);
        }
        if ((di->defs.defined & _GD_FontSetWidth) == 0)
        {
            if (warningLevel > 9)
            {
                WARN("No font set width for text doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using \"%s\"\n", DFLT_SET_WIDTH);
            }
            di->fontSetWidth = XkbInternAtom(NULL, DFLT_SET_WIDTH, False);
        }
        if ((di->defs.defined & _GD_FontVariant) == 0)
        {
            if (warningLevel > 9)
            {
                WARN("No font variant for text doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using \"%s\"\n", DFLT_VARIANT);
            }
            di->fontVariant = XkbInternAtom(NULL, DFLT_VARIANT, False);
        }
        if ((di->defs.defined & _GD_FontEncoding) == 0)
        {
            if (warningLevel > 7)
            {
                WARN("No font encoding for doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using \"%s\"\n", DFLT_ENCODING);
            }
            di->fontEncoding = XkbInternAtom(NULL, DFLT_ENCODING, False);
        }
        if ((di->defs.defined & _GD_FontSize) == 0)
        {
            if (warningLevel > 7)
            {
                WARN("No font size for text doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using %s point text\n",
                        XkbGeomFPText(DFLT_SIZE, XkbMessage));
            }
            di->fontSize = DFLT_SIZE;
        }
        if ((di->defs.defined & _GD_Height) == 0)
        {
            unsigned size, nLines;
            char *tmp;
            size = (di->fontSize * 120) / 100;
            size = (size * 254) / 720;  /* convert to mm/10 */
            for (nLines = 1, tmp = XkbAtomGetString(NULL, di->text); *tmp;
                 tmp++)
            {
                if (*tmp == '\n')
                    nLines++;
            }
            size *= nLines;
            if (warningLevel > 5)
            {
                WARN("No height for text doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using calculated height %s millimeters\n",
                        XkbGeomFPText(size, XkbMessage));
            }
            di->height = size;
        }
        if ((di->defs.defined & _GD_Width) == 0)
        {
            unsigned width, tmp;
            char *str;
            width = tmp = 0;
            for (str = XkbAtomGetString(NULL, di->text); *str; str++)
            {
                if (*str != '\n')
                    tmp++;
                else
                {
                    if (tmp > width)
                        width = tmp;
                    tmp = 1;
                }
            }
            if (width == 0)
                width = tmp;
            width *= (di->height * 2) / 3;
            if (warningLevel > 5)
            {
                WARN("No width for text doodad %s\n", ddText(info->dpy, di));
                ACTION("Using calculated width %s millimeters\n",
                        XkbGeomFPText(width, XkbMessage));
            }
            di->width = width;
        }
        break;
    case XkbIndicatorDoodad:
        if ((di->defs.defined & _GD_Shape) == 0)
        {
            ERROR("No shape defined for indicator doodad %s\n",
                   ddText(info->dpy, di));
            ACTION("Incomplete definition ignored\n");
            return False;
        }
        else
        {
            ShapeInfo *si;
            si = FindShape(info, di->shape, "indicator doodad",
                           ddText(info->dpy, di));
            if (si)
                di->shape = si->name;
            else
            {
                ERROR("No legal shape for doodad %s\n",
                       ddText(info->dpy, di));
                ACTION("Incomplete definition ignored\n");
                return False;
            }
        }
        if ((di->defs.defined & _GD_Color) == 0)
        {
            if (warningLevel > 5)
            {
                WARN("No \"on\" color for indicator doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using green\n");
            }
            di->color = XkbInternAtom(NULL, "green", False);
        }
        if ((di->defs.defined & _GD_OffColor) == 0)
        {
            if (warningLevel > 5)
            {
                WARN("No \"off\" color for indicator doodad %s\n",
                      ddText(info->dpy, di));
                ACTION("Using black\n");
            }
            di->offColor = XkbInternAtom(NULL, "black", False);
        }
        break;
    case XkbLogoDoodad:
        if (di->logoName == NULL)
        {
            ERROR("No logo name defined for logo doodad %s\n",
                   ddText(info->dpy, di));
            ACTION("Incomplete definition ignored\n");
            return False;
        }
        if ((di->defs.defined & _GD_Shape) == 0)
        {
            ERROR("No shape defined for logo doodad %s\n",
                   ddText(info->dpy, di));
            ACTION("Incomplete definition ignored\n");
            return False;
        }
        else
        {
            ShapeInfo *si;
            si = FindShape(info, di->shape, "logo doodad",
                           ddText(info->dpy, di));
            if (si)
                di->shape = si->name;
            else
            {
                ERROR("No legal shape for %s\n", ddText(info->dpy, di));
                ACTION("Incomplete definition ignored\n");
                return False;
            }
        }
        if ((di->defs.defined & _GD_Color) == 0)
        {
            if (warningLevel > 5)
            {
                WARN("No color for doodad %s\n", ddText(info->dpy, di));
                ACTION("Using black\n");
            }
            di->color = XkbInternAtom(NULL, "black", False);
        }
        break;
    default:
        WSGO("Unknown doodad type %d in VerifyDoodad\n",
              (unsigned int) di->type);
        return False;
    }
    return True;
}

#define	FONT_TEMPLATE	"-*-%s-%s-%s-%s-%s-*-%d-*-*-*-*-%s"

static char *
FontFromParts(Atom fontTok,
              Atom weightTok,
              Atom slantTok,
              Atom setWidthTok, Atom varTok, int size, Atom encodingTok)
{
    const char *font, *weight, *slant, *setWidth, *variant, *encoding;
    char *rtrn;

    font = (fontTok != None ? XkbAtomGetString(NULL, fontTok) : DFLT_FONT);
    weight =
        (weightTok != None ? XkbAtomGetString(NULL, weightTok) : DFLT_WEIGHT);
    slant =
        (slantTok != None ? XkbAtomGetString(NULL, slantTok) : DFLT_SLANT);
    setWidth =
        (setWidthTok !=
         None ? XkbAtomGetString(NULL, setWidthTok) : DFLT_SET_WIDTH);
    variant =
        (varTok != None ? XkbAtomGetString(NULL, varTok) : DFLT_VARIANT);
    encoding =
        (encodingTok !=
         None ? XkbAtomGetString(NULL, encodingTok) : DFLT_ENCODING);
    if (size == 0)
        size = DFLT_SIZE;

#ifdef HAVE_ASPRINTF
    if (asprintf(&rtrn, FONT_TEMPLATE, font, weight, slant,
                 setWidth, variant, size, encoding) < 0)
        rtrn = NULL;
#else
    size_t totalSize =
        strlen(FONT_TEMPLATE) + strlen(font) + strlen(weight) + strlen(slant);
    totalSize += strlen(setWidth) + strlen(variant) + strlen(encoding);
    rtrn = calloc(totalSize, 1);
    if (rtrn)
    {
        snprintf(rtrn, totalSize, FONT_TEMPLATE, font, weight, slant,
                 setWidth, variant, size, encoding);
    }
#endif
    return rtrn;
}

static Bool
CopyDoodadDef(XkbGeometryPtr geom,
              XkbSectionPtr section, DoodadInfo * di, GeometryInfo * info)
{
    Atom name;
    XkbDoodadPtr doodad;
    XkbColorPtr color;
    XkbShapePtr shape;
    ShapeInfo *si;

    if (!VerifyDoodadInfo(di, info))
        return False;
    name = XkbInternAtom(NULL, XkbAtomGetString(NULL, di->name), False);
    doodad = XkbAddGeomDoodad(geom, section, name);
    if (!doodad)
    {
        WSGO("Couldn't allocate doodad in %s\n",
              (section ? "section" : "geometry"));
        ACTION("Cannot copy doodad %s\n", ddText(info->dpy, di));
        return False;
    }
    doodad->any.type = di->type;
    doodad->any.priority = di->priority;
    doodad->any.top = di->top;
    doodad->any.left = di->left;
    switch (di->type)
    {
    case XkbOutlineDoodad:
    case XkbSolidDoodad:
        si = FindShape(info, di->shape, NULL, NULL);
        if (!si)
            return False;
        doodad->shape.angle = di->angle;
        color =
            XkbAddGeomColor(geom, XkbAtomGetString(NULL, di->color),
                            geom->num_colors);
        shape = &geom->shapes[si->index];
        XkbSetShapeDoodadColor(geom, &doodad->shape, color);
        XkbSetShapeDoodadShape(geom, &doodad->shape, shape);
        break;
    case XkbTextDoodad:
        doodad->text.angle = di->angle;
        doodad->text.width = di->width;
        doodad->text.height = di->height;
        if (di->fontSpec == None)
            doodad->text.font = FontFromParts(di->font, di->fontWeight,
                                              di->fontSlant,
                                              di->fontSetWidth,
                                              di->fontVariant, di->fontSize,
                                              di->fontEncoding);
        else
            doodad->text.font = XkbAtomGetString(NULL, di->fontSpec);
        doodad->text.text = XkbAtomGetString(NULL, di->text);
        color =
            XkbAddGeomColor(geom, XkbAtomGetString(NULL, di->color),
                            geom->num_colors);
        XkbSetTextDoodadColor(geom, &doodad->text, color);
        break;
    case XkbIndicatorDoodad:
        si = FindShape(info, di->shape, NULL, NULL);
        if (!si)
            return False;
        shape = &geom->shapes[si->index];
        color =
            XkbAddGeomColor(geom, XkbAtomGetString(NULL, di->color),
                            geom->num_colors);
        XkbSetIndicatorDoodadShape(geom, &doodad->indicator, shape);
        XkbSetIndicatorDoodadOnColor(geom, &doodad->indicator, color);
        color =
            XkbAddGeomColor(geom, XkbAtomGetString(NULL, di->offColor),
                            geom->num_colors);
        XkbSetIndicatorDoodadOffColor(geom, &doodad->indicator, color);
        break;
    case XkbLogoDoodad:
        si = FindShape(info, di->shape, NULL, NULL);
        if (!si)
            return False;
        doodad->logo.angle = di->angle;
        color =
            XkbAddGeomColor(geom, XkbAtomGetString(NULL, di->color),
                            geom->num_colors);
        shape = &geom->shapes[si->index];
        XkbSetLogoDoodadColor(geom, &doodad->logo, color);
        XkbSetLogoDoodadShape(geom, &doodad->logo, shape);
        doodad->logo.logo_name = di->logoName;
        di->logoName = NULL;
        break;
    }
    return True;
}

/***====================================================================***/

static Bool
VerifyOverlayInfo(XkbGeometryPtr geom,
                  XkbSectionPtr section,
                  OverlayInfo * oi,
                  GeometryInfo * info, short rowMap[256], short rowSize[256])
{
    OverlayKeyInfo *ki, *next;
    XkbRowPtr row;
    XkbKeyPtr key;
    int r, k;

    /* find out which row each key is in */
    for (ki = oi->keys; ki != NULL; ki = (OverlayKeyInfo *) ki->defs.next)
    {
        unsigned long oKey = KeyNameToLong(ki->over);
        unsigned long uKey = KeyNameToLong(ki->under);

        for (r = 0, row = section->rows; (r < section->num_rows) && oKey;
             r++, row++)
        {
            for (k = 0, key = row->keys; (k < row->num_keys) && oKey;
                 k++, key++)
            {
                unsigned long sKey = KeyNameToLong(key->name.name);
                if (sKey == oKey)
                {
                    if (warningLevel > 0)
                    {
                        WARN
                            ("Key %s in section \"%s\" and overlay \"%s\"\n",
                             XkbKeyNameText(key->name.name,
                                            XkbMessage),
                             XkbAtomText(info->dpy, section->name,
                                         XkbMessage),
                             XkbAtomText(info->dpy, oi->name, XkbMessage));
                        ACTION("Overlay definition ignored\n");
                    }
                    oKey = 0;
                }
                else if (sKey == uKey)
                {
                    ki->sectionRow = r;
                    oKey = 0;
                }
            }
        }
        if ((ki->sectionRow == _GOK_UnknownRow) && (warningLevel > 0))
        {
            WARN
                ("Key %s not in \"%s\", but has an overlay key in \"%s\"\n",
                 XkbKeyNameText(ki->under, XkbMessage),
                 XkbAtomText(info->dpy, section->name, XkbMessage),
                 XkbAtomText(info->dpy, oi->name, XkbMessage));
            ACTION("Definition ignored\n");
        }
    }
    /* now prune out keys that aren't in the section */
    while ((oi->keys != NULL) && (oi->keys->sectionRow == _GOK_UnknownRow))
    {
        next = (OverlayKeyInfo *) oi->keys->defs.next;
        free(oi->keys);
        oi->keys = next;
        oi->nKeys--;
    }
    for (ki = oi->keys; (ki != NULL) && (ki->defs.next != NULL); ki = next)
    {
        next = (OverlayKeyInfo *) ki->defs.next;
        if (next->sectionRow == _GOK_UnknownRow)
        {
            ki->defs.next = next->defs.next;
            oi->nKeys--;
            free(next);
            next = (OverlayKeyInfo *) ki->defs.next;
        }
    }
    if (oi->nKeys < 1)
    {
        ERROR("Overlay \"%s\" for section \"%s\" has no legal keys\n",
               XkbAtomText(info->dpy, oi->name, XkbMessage),
               XkbAtomText(info->dpy, section->name, XkbMessage));
        ACTION("Overlay definition ignored\n");
        return False;
    }
    /* now figure out how many rows are defined for the overlay */
    bzero(rowSize, sizeof(short) * 256);
    for (k = 0; k < 256; k++)
    {
        rowMap[k] = -1;
    }
    oi->nRows = 0;
    for (ki = oi->keys; ki != NULL; ki = (OverlayKeyInfo *) ki->defs.next)
    {
        if (rowMap[ki->sectionRow] == -1)
            rowMap[ki->sectionRow] = oi->nRows++;
        ki->overlayRow = rowMap[ki->sectionRow];
        rowSize[ki->overlayRow]++;
    }
    return True;
}

static Bool
CopyOverlayDef(XkbGeometryPtr geom,
               XkbSectionPtr section, OverlayInfo * oi, GeometryInfo * info)
{
    Atom name;
    XkbOverlayPtr ol;
    short rowMap[256], rowSize[256];

    if (!VerifyOverlayInfo(geom, section, oi, info, rowMap, rowSize))
        return False;
    name = XkbInternAtom(NULL, XkbAtomGetString(NULL, oi->name), False);
    ol = XkbAddGeomOverlay(section, name, oi->nRows);
    if (!ol)
    {
        WSGO("Couldn't add overlay \"%s\" to section \"%s\"\n",
              XkbAtomText(info->dpy, name, XkbMessage),
              XkbAtomText(info->dpy, section->name, XkbMessage));
        return False;
    }
    for (int i = 0; i < oi->nRows; i++)
    {
        int tmp, row_under;
        for (tmp = 0, row_under = -1;
             (tmp < section->num_rows) && (row_under < 0); tmp++)
        {
            if (rowMap[tmp] == i)
                row_under = tmp;
        }
        if (!XkbAddGeomOverlayRow(ol, row_under, rowSize[i]))
        {
            WSGO
                ("Can't add row %d to overlay \"%s\" of section \"%s\"\n",
                 i, XkbAtomText(info->dpy, name, XkbMessage),
                 XkbAtomText(info->dpy, section->name, XkbMessage));
            return False;
        }
    }
    for (OverlayKeyInfo *ki = oi->keys; ki != NULL;
         ki = (OverlayKeyInfo *) ki->defs.next)
    {
        XkbOverlayRowPtr row = &ol->rows[ki->overlayRow];
        XkbOverlayKeyPtr key = &row->keys[row->num_keys++];
        bzero(key, sizeof(XkbOverlayKeyRec));
        strncpy(key->over.name, ki->over, XkbKeyNameLength);
        strncpy(key->under.name, ki->under, XkbKeyNameLength);
    }
    return True;
}

/***====================================================================***/

static Bool
CopySectionDef(XkbGeometryPtr geom, SectionInfo * si, GeometryInfo * info)
{
    XkbSectionPtr section;
    Atom name;

    name = XkbInternAtom(NULL, XkbAtomGetString(NULL, si->name), False);
    section =
        XkbAddGeomSection(geom, name, si->nRows, si->nDoodads, si->nOverlays);
    if (section == NULL)
    {
        WSGO("Couldn't allocate section in geometry\n");
        ACTION("Section %s not compiled\n", scText(info->dpy, si));
        return False;
    }
    section->top = si->top;
    section->left = si->left;
    section->width = si->width;
    section->height = si->height;
    section->angle = si->angle;
    section->priority = si->priority;
    for (RowInfo *ri = si->rows; ri != NULL; ri = (RowInfo *) ri->defs.next)
    {
        XkbRowPtr row = XkbAddGeomRow(section, ri->nKeys);
        if (row == NULL)
        {
            WSGO("Couldn't allocate row in section\n");
            ACTION("Section %s is incomplete\n", scText(info->dpy, si));
            return False;
        }
        row->top = ri->top;
        row->left = ri->left;
        row->vertical = ri->vertical;
        for (KeyInfo *ki = ri->keys; ki != NULL; ki = (KeyInfo *) ki->defs.next)
        {
            XkbKeyPtr key;
            XkbColorPtr color;
            if ((ki->defs.defined & _GK_Name) == 0)
            {
                ERROR("Key %d of row %d in section %s has no name\n",
                       (int) ki->index, (int) ri->index,
                       scText(info->dpy, si));
                ACTION("Section %s ignored\n", scText(info->dpy, si));
                return False;
            }
            key = XkbAddGeomKey(row);
            if (key == NULL)
            {
                WSGO("Couldn't allocate key in row\n");
                ACTION("Section %s is incomplete\n", scText(info->dpy, si));
                return False;
            }
            memcpy(key->name.name, ki->name, XkbKeyNameLength);
            key->gap = ki->gap;
            if (ki->shape == None)
                key->shape_ndx = 0;
            else
            {
                ShapeInfo *sinfo;
                sinfo = FindShape(info, ki->shape, "key", keyText(ki));
                if (!sinfo)
                    return False;
                key->shape_ndx = sinfo->index;
            }
            if (ki->color != None)
                color =
                    XkbAddGeomColor(geom,
                                    XkbAtomGetString(NULL, ki->color),
                                    geom->num_colors);
            else
                color = XkbAddGeomColor(geom, "white", geom->num_colors);
            XkbSetKeyColor(geom, key, color);
        }
    }
    if (si->doodads != NULL)
    {
        for (DoodadInfo *di = si->doodads; di != NULL;
             di = (DoodadInfo *) di->defs.next)
        {
            CopyDoodadDef(geom, section, di, info);
        }
    }
    if (si->overlays != NULL)
    {
        for (OverlayInfo *oi = si->overlays; oi != NULL;
             oi = (OverlayInfo *) oi->defs.next)
        {
            CopyOverlayDef(geom, section, oi, info);
        }
    }
    if (XkbComputeSectionBounds(geom, section))
    {
        /* 7/6/94 (ef) --  check for negative origin and translate */
        if ((si->defs.defined & _GS_Width) == 0)
            section->width = section->bounds.x2;
        if ((si->defs.defined & _GS_Height) == 0)
            section->height = section->bounds.y2;
    }
    return True;
}

/***====================================================================***/

Bool
CompileGeometry(XkbFile * file, XkbFileInfo * result, unsigned merge)
{
    GeometryInfo info;
    XkbDescPtr xkb;

    xkb = result->xkb;
    InitGeometryInfo(&info, file->id, merge);
    info.dpy = xkb->dpy;
    HandleGeometryFile(file, xkb, merge, &info);

    if (info.errorCount == 0)
    {
        XkbGeometryPtr geom;
        XkbGeometrySizesRec sizes = {
            .which = XkbGeomAllMask,
            .num_properties = info.nProps,
            .num_colors = 8,
            .num_shapes = info.nShapes,
            .num_sections = info.nSections,
            .num_doodads = info.nDoodads
        };

        if (XkbAllocGeometry(xkb, &sizes) != Success)
        {
            WSGO("Couldn't allocate GeometryRec\n");
            ACTION("Geometry not compiled\n");
            return False;
        }
        geom = xkb->geom;

        geom->width_mm = info.widthMM;
        geom->height_mm = info.heightMM;
        if (info.name != NULL)
        {
            geom->name = XkbInternAtom(xkb->dpy, info.name, False);
            if (XkbAllocNames(xkb, XkbGeometryNameMask, 0, 0) == Success)
                xkb->names->geometry = geom->name;
        }
        if (info.fontSpec != None)
            geom->label_font =
                uStringDup(XkbAtomGetString(NULL, info.fontSpec));
        else
            geom->label_font = FontFromParts(info.font, info.fontWeight,
                                             info.fontSlant,
                                             info.fontSetWidth,
                                             info.fontVariant,
                                             info.fontSize,
                                             info.fontEncoding);
        XkbAddGeomColor(geom, "black", geom->num_colors);
        XkbAddGeomColor(geom, "white", geom->num_colors);

        if (info.baseColor == None)
            info.baseColor = XkbInternAtom(NULL, "white", False);
        if (info.labelColor == None)
            info.labelColor = XkbInternAtom(NULL, "black", False);
        geom->base_color =
            XkbAddGeomColor(geom, XkbAtomGetString(NULL, info.baseColor),
                            geom->num_colors);
        geom->label_color =
            XkbAddGeomColor(geom, XkbAtomGetString(NULL, info.labelColor),
                            geom->num_colors);

        if (info.props)
        {
            for (PropertyInfo *pi = info.props; pi != NULL;
                 pi = (PropertyInfo *) pi->defs.next)
            {
                if (!XkbAddGeomProperty(geom, pi->name, pi->value))
                    return False;
            }
        }
        if (info.shapes)
        {
            for (ShapeInfo *si = info.shapes; si != NULL;
                 si = (ShapeInfo *) si->defs.next)
            {
                if (!CopyShapeDef(xkb->dpy, geom, si))
                    return False;
            }
        }
        if (info.sections)
        {
            for (SectionInfo *si = info.sections; si != NULL;
                 si = (SectionInfo *) si->defs.next)
            {
                if (!CopySectionDef(geom, si, &info))
                    return False;
            }
        }
        if (info.doodads)
        {
            for (DoodadInfo *di = info.doodads; di != NULL;
                 di = (DoodadInfo *) di->defs.next)
            {
                if (!CopyDoodadDef(geom, NULL, di, &info))
                    return False;
            }
        }
        if (info.aliases)
            ApplyAliases(xkb, True, &info.aliases);
        ClearGeometryInfo(&info);
        return True;
    }
    return False;
}
