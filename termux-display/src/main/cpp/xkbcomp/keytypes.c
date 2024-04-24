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
#include "action.h"
#include "misc.h"

typedef struct _PreserveInfo
{
    CommonInfo defs;
    short matchingMapIndex;
    unsigned char indexMods;
    unsigned char preMods;
    unsigned short indexVMods;
    unsigned short preVMods;
} PreserveInfo;

#define	_KT_Name	(1<<0)
#define	_KT_Mask	(1<<1)
#define	_KT_Map		(1<<2)
#define	_KT_Preserve	(1<<3)
#define	_KT_LevelNames	(1<<4)

typedef struct _KeyTypeInfo
{
    CommonInfo defs;
    Display *dpy;
    Atom name;
    int fileID;
    unsigned mask;
    unsigned vmask;
    Bool groupInfo;
    int numLevels;
    int nEntries;
    int szEntries;
    XkbKTMapEntryPtr entries;
    PreserveInfo *preserve;
    int szNames;
    Atom *lvlNames;
} KeyTypeInfo;

typedef struct _KeyTypesInfo
{
    Display *dpy;
    char *name;
    int errorCount;
    int fileID;
    unsigned stdPresent;
    int nTypes;
    KeyTypeInfo *types;
    KeyTypeInfo dflt;
    VModInfo vmods;
} KeyTypesInfo;

Atom tok_ONE_LEVEL;
Atom tok_TWO_LEVEL;
Atom tok_ALPHABETIC;
Atom tok_KEYPAD;

/***====================================================================***/

#define	ReportTypeShouldBeArray(t,f) \
	ReportShouldBeArray("key type",(f),TypeTxt(t))
#define	ReportTypeBadType(t,f,w) \
	ReportBadType("key type",(f),TypeTxt(t),(w))

/***====================================================================***/

#define	MapEntryTxt(t,x,e)	\
    XkbVModMaskText((t)->dpy,(x),(e)->mods.real_mods,(e)->mods.vmods,XkbMessage)
#define	PreserveIndexTxt(t,x,p)	\
	XkbVModMaskText((t)->dpy,(x),(p)->indexMods,(p)->indexVMods,XkbMessage)
#define	PreserveTxt(t,x,p)	\
	XkbVModMaskText((t)->dpy,(x),(p)->preMods,(p)->preVMods,XkbMessage)
#define	TypeTxt(t)	XkbAtomText((t)->dpy,(t)->name,XkbMessage)
#define	TypeMaskTxt(t,x)	\
	XkbVModMaskText((t)->dpy,(x),(t)->mask,(t)->vmask,XkbMessage)

/***====================================================================***/

static void
InitKeyTypesInfo(KeyTypesInfo *info, XkbDescPtr xkb, const KeyTypesInfo *from)
{
    tok_ONE_LEVEL = XkbInternAtom(NULL, "ONE_LEVEL", False);
    tok_TWO_LEVEL = XkbInternAtom(NULL, "TWO_LEVEL", False);
    tok_ALPHABETIC = XkbInternAtom(NULL, "ALPHABETIC", False);
    tok_KEYPAD = XkbInternAtom(NULL, "KEYPAD", False);
    info->dpy = NULL;
    info->name = uStringDup("default");
    info->errorCount = 0;
    info->stdPresent = 0;
    info->nTypes = 0;
    info->types = NULL;
    info->dflt.defs.defined = 0;
    info->dflt.defs.fileID = 0;
    info->dflt.defs.merge = MergeOverride;
    info->dflt.defs.next = NULL;
    info->dflt.name = None;
    info->dflt.mask = 0;
    info->dflt.vmask = 0;
    info->dflt.groupInfo = False;
    info->dflt.numLevels = 1;
    info->dflt.nEntries = info->dflt.szEntries = 0;
    info->dflt.entries = NULL;
    info->dflt.szNames = 0;
    info->dflt.lvlNames = NULL;
    info->dflt.preserve = NULL;
    InitVModInfo(&info->vmods, xkb);
    if (from != NULL)
    {
        info->dpy = from->dpy;
        info->dflt = from->dflt;
        if (from->dflt.entries)
        {
            info->dflt.entries = calloc(from->dflt.szEntries,
                                        sizeof(XkbKTMapEntryRec));
            if (info->dflt.entries)
            {
                unsigned sz = from->dflt.nEntries * sizeof(XkbKTMapEntryRec);
                memcpy(info->dflt.entries, from->dflt.entries, sz);
            }
        }
        if (from->dflt.lvlNames)
        {
            info->dflt.lvlNames = calloc(from->dflt.szNames, sizeof(Atom));
            if (info->dflt.lvlNames)
            {
                unsigned sz = from->dflt.szNames * sizeof(Atom);
                memcpy(info->dflt.lvlNames, from->dflt.lvlNames, sz);
            }
        }
        if (from->dflt.preserve)
        {
            PreserveInfo *last = NULL;

            for (PreserveInfo *old = from->dflt.preserve;
                 old; old = (PreserveInfo *) old->defs.next)
            {
                PreserveInfo *new = malloc(sizeof(PreserveInfo));
                if (!new)
                    return;
                *new = *old;
                new->defs.next = NULL;
                if (last)
                    last->defs.next = (CommonInfo *) new;
                else
                    info->dflt.preserve = new;
                last = new;
            }
        }
    }
    return;
}

static void
FreeKeyTypeInfo(KeyTypeInfo * type)
{
    free(type->entries);
    type->entries = NULL;

    free(type->lvlNames);
    type->lvlNames = NULL;

    if (type->preserve != NULL)
    {
        ClearCommonInfo(&type->preserve->defs);
        type->preserve = NULL;
    }
    return;
}

static void
FreeKeyTypesInfo(KeyTypesInfo * info)
{
    info->dpy = NULL;
    free(info->name);
    info->name = NULL;
    if (info->types)
    {
        for (KeyTypeInfo *type = info->types; type;
             type = (KeyTypeInfo *) type->defs.next)
        {
            FreeKeyTypeInfo(type);
        }
        info->types = (KeyTypeInfo *) ClearCommonInfo(&info->types->defs);
    }
    FreeKeyTypeInfo(&info->dflt);
    return;
}

static KeyTypeInfo *
NextKeyType(KeyTypesInfo * info)
{
    KeyTypeInfo *type;

    type = calloc(1, sizeof(KeyTypeInfo));
    if (type != NULL)
    {
        type->defs.fileID = info->fileID;
        type->dpy = info->dpy;
        info->types = (KeyTypeInfo *) AddCommonInfo(&info->types->defs,
                                                    (CommonInfo *) type);
        info->nTypes++;
    }
    return type;
}

static KeyTypeInfo *
FindMatchingKeyType(KeyTypesInfo *info, const KeyTypeInfo *new)
{
    for (KeyTypeInfo *old = info->types; old;
         old = (KeyTypeInfo *) old->defs.next)
    {
        if (old->name == new->name)
            return old;
    }
    return NULL;
}

static Bool
ReportTypeBadWidth(const char *type, int has, int needs)
{
    ERROR("Key type \"%s\" has %d levels, must have %d\n", type, has, needs);
    ACTION("Illegal type definition ignored\n");
    return False;
}

static Bool
AddKeyType(XkbDescPtr xkb, KeyTypesInfo * info, KeyTypeInfo * new)
{
    KeyTypeInfo *old;

    if (new->name == tok_ONE_LEVEL)
    {
        if (new->numLevels > 1)
            return ReportTypeBadWidth("ONE_LEVEL", new->numLevels, 1);
        info->stdPresent |= XkbOneLevelMask;
    }
    else if (new->name == tok_TWO_LEVEL)
    {
        if (new->numLevels > 2)
            return ReportTypeBadWidth("TWO_LEVEL", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbTwoLevelMask;
    }
    else if (new->name == tok_ALPHABETIC)
    {
        if (new->numLevels > 2)
            return ReportTypeBadWidth("ALPHABETIC", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbAlphabeticMask;
    }
    else if (new->name == tok_KEYPAD)
    {
        if (new->numLevels > 2)
            return ReportTypeBadWidth("KEYPAD", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbKeypadMask;
    }

    old = FindMatchingKeyType(info, new);
    if (old != NULL)
    {
        Bool report;
        if ((new->defs.merge == MergeReplace)
            || (new->defs.merge == MergeOverride))
        {
            KeyTypeInfo *next = (KeyTypeInfo *) old->defs.next;
            if (((old->defs.fileID == new->defs.fileID)
                 && (warningLevel > 0)) || (warningLevel > 9))
            {
                WARN("Multiple definitions of the %s key type\n",
                      XkbAtomGetString(NULL, new->name));
                ACTION("Earlier definition ignored\n");
            }
            FreeKeyTypeInfo(old);
            *old = *new;
            new->szEntries = new->nEntries = 0;
            new->entries = NULL;
            new->preserve = NULL;
            new->lvlNames = NULL;
            old->defs.next = &next->defs;
            return True;
        }
        report = (old->defs.fileID == new->defs.fileID) && (warningLevel > 0);
        if (report)
        {
            WARN("Multiple definitions of the %s key type\n",
                  XkbAtomGetString(NULL, new->name));
            ACTION("Later definition ignored\n");
        }
        FreeKeyTypeInfo(new);
        return True;
    }
    old = NextKeyType(info);
    if (old == NULL)
        return False;
    *old = *new;
    old->defs.next = NULL;
    new->nEntries = new->szEntries = 0;
    new->entries = NULL;
    new->szNames = 0;
    new->lvlNames = NULL;
    new->preserve = NULL;
    return True;
}

/***====================================================================***/

static void
MergeIncludedKeyTypes(KeyTypesInfo * into,
                      KeyTypesInfo * from, unsigned merge, XkbDescPtr xkb)
{
    if (from->errorCount > 0)
    {
        into->errorCount += from->errorCount;
        return;
    }
    if (into->name == NULL)
    {
        into->name = from->name;
        from->name = NULL;
    }
    for (KeyTypeInfo *type = from->types; type;
         type = (KeyTypeInfo *) type->defs.next)
    {
        if (merge != MergeDefault)
            type->defs.merge = merge;
        if (!AddKeyType(xkb, into, type))
            into->errorCount++;
    }
    into->stdPresent |= from->stdPresent;
    return;
}

typedef void (*FileHandler) (XkbFile * /* file */ ,
                             XkbDescPtr /* xkb */ ,
                             unsigned /* merge */ ,
                             KeyTypesInfo *     /* included */
    );

static Bool
HandleIncludeKeyTypes(IncludeStmt * stmt,
                      XkbDescPtr xkb, KeyTypesInfo * info, FileHandler hndlr)
{
    unsigned newMerge;
    XkbFile *rtrn;
    KeyTypesInfo included;
    Bool haveSelf;

    haveSelf = False;
    if ((stmt->file == NULL) && (stmt->map == NULL))
    {
        haveSelf = True;
        included = *info;
        bzero(info, sizeof(KeyTypesInfo));
    }
    else if (ProcessIncludeFile(stmt, XkmTypesIndex, &rtrn, &newMerge))
    {
        InitKeyTypesInfo(&included, xkb, info);
        included.fileID = included.dflt.defs.fileID = rtrn->id;
        included.dflt.defs.merge = newMerge;

        (*hndlr) (rtrn, xkb, newMerge, &included);
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
        unsigned op;
        KeyTypesInfo next_incl;

        for (IncludeStmt *next = stmt->next; next != NULL; next = next->next)
        {
            if ((next->file == NULL) && (next->map == NULL))
            {
                haveSelf = True;
                MergeIncludedKeyTypes(&included, info, next->merge, xkb);
                FreeKeyTypesInfo(info);
            }
            else if (ProcessIncludeFile(next, XkmTypesIndex, &rtrn, &op))
            {
                InitKeyTypesInfo(&next_incl, xkb, &included);
                next_incl.fileID = next_incl.dflt.defs.fileID = rtrn->id;
                next_incl.dflt.defs.merge = op;
                (*hndlr) (rtrn, xkb, op, &next_incl);
                MergeIncludedKeyTypes(&included, &next_incl, op, xkb);
                FreeKeyTypesInfo(&next_incl);
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
        MergeIncludedKeyTypes(info, &included, newMerge, xkb);
        FreeKeyTypesInfo(&included);
    }
    return (info->errorCount == 0);
}

/***====================================================================***/

static XkbKTMapEntryPtr
FindMatchingMapEntry(const KeyTypeInfo *type, unsigned mask, unsigned vmask)
{
    int i;
    XkbKTMapEntryPtr entry;

    for (i = 0, entry = type->entries; i < type->nEntries; i++, entry++)
    {
        if ((entry->mods.real_mods == mask) && (entry->mods.vmods == vmask))
            return entry;
    }
    return NULL;
}

static void
DeleteLevel1MapEntries(KeyTypeInfo * type)
{
    for (int i = 0; i < type->nEntries; i++)
    {
        if (type->entries[i].level == 0)
        {
            for (int n = i; n < type->nEntries - 1; n++)
            {
                type->entries[n] = type->entries[n + 1];
            }
            type->nEntries--;
        }
    }
    return;
}

/**
 * Return a pointer to the next free XkbKTMapEntry, reallocating space if
 * necessary.
 */
static XkbKTMapEntryPtr
NextMapEntry(KeyTypeInfo * type)
{
    if (type->entries == NULL)
    {
        type->entries = calloc(2, sizeof(XkbKTMapEntryRec));
        if (type->entries == NULL)
        {
            ERROR("Couldn't allocate map entries for %s\n", TypeTxt(type));
            ACTION("Map entries lost\n");
            return NULL;
        }
        type->szEntries = 2;
        type->nEntries = 0;
    }
    else if (type->nEntries >= type->szEntries)
    {
        type->szEntries *= 2;
        type->entries = recallocarray(type->entries,
                                      type->nEntries, type->szEntries,
                                      sizeof(XkbKTMapEntryRec));
        if (type->entries == NULL)
        {
            ERROR("Couldn't reallocate map entries for %s\n", TypeTxt(type));
            ACTION("Map entries lost\n");
            return NULL;
        }
    }
    return &type->entries[type->nEntries++];
}

static Bool
AddPreserve(XkbDescPtr xkb,
            KeyTypeInfo * type, PreserveInfo * new, Bool clobber, Bool report)
{
    PreserveInfo *old;

    old = type->preserve;
    while (old != NULL)
    {
        if ((old->indexMods != new->indexMods) ||
            (old->indexVMods != new->indexVMods))
        {
            old = (PreserveInfo *) old->defs.next;
            continue;
        }
        if ((old->preMods == new->preMods)
            && (old->preVMods == new->preVMods))
        {
            if (warningLevel > 9)
            {
                WARN("Identical definitions for preserve[%s] in %s\n",
                      PreserveIndexTxt(type, xkb, old), TypeTxt(type));
                ACTION("Ignored\n");
            }
            return True;
        }
        if (report && (warningLevel > 0))
        {
            char *str;
            WARN("Multiple definitions for preserve[%s] in %s\n",
                  PreserveIndexTxt(type, xkb, old), TypeTxt(type));

            if (clobber)
                str = PreserveTxt(type, xkb, new);
            else
                str = PreserveTxt(type, xkb, old);
            ACTION("Using %s, ", str);
            if (clobber)
                str = PreserveTxt(type, xkb, old);
            else
                str = PreserveTxt(type, xkb, new);
            INFO("ignoring %s\n", str);
        }
        if (clobber)
        {
            old->preMods = new->preMods;
            old->preVMods = new->preVMods;
        }
        return True;
    }
    old = malloc(sizeof(PreserveInfo));
    if (!old)
    {
        WSGO("Couldn't allocate preserve in %s\n", TypeTxt(type));
        ACTION("Preserve[%s] lost\n", PreserveIndexTxt(type, xkb, new));
        return False;
    }
    *old = *new;
    old->matchingMapIndex = -1;
    type->preserve =
        (PreserveInfo *) AddCommonInfo(&type->preserve->defs, &old->defs);
    return True;
}

/**
 * Add a new KTMapEntry to the given key type. If an entry with the same mods
 * already exists, the level is updated (if clobber is TRUE). Otherwise, a new
 * entry is created.
 *
 * @param clobber Overwrite existing entry.
 * @param report True if a warning is to be printed on.
 */
static Bool
AddMapEntry(XkbDescPtr xkb,
            KeyTypeInfo * type,
            XkbKTMapEntryPtr new, Bool clobber, Bool report)
{
    XkbKTMapEntryPtr old;

    if ((old =
         FindMatchingMapEntry(type, new->mods.real_mods, new->mods.vmods)))
    {
        if (report && (old->level != new->level))
        {
            unsigned use, ignore;
            if (clobber)
            {
                use = new->level + 1;
                ignore = old->level + 1;
            }
            else
            {
                use = old->level + 1;
                ignore = new->level + 1;
            }
            WARN("Multiple map entries for %s in %s\n",
                  MapEntryTxt(type, xkb, new), TypeTxt(type));
            ACTION("Using %d, ignoring %d\n", use, ignore);
        }
        else if (warningLevel > 9)
        {
            WARN("Multiple occurrences of map[%s]= %d in %s\n",
                  MapEntryTxt(type, xkb, new), new->level + 1, TypeTxt(type));
            ACTION("Ignored\n");
            return True;
        }
        if (clobber)
            old->level = new->level;
        return True;
    }
    if ((old = NextMapEntry(type)) == NULL)
        return False;           /* allocation failure, already reported */
    if (new->level >= type->numLevels)
        type->numLevels = new->level + 1;
    if (new->mods.vmods == 0)
        old->active = True;
    else
        old->active = False;
    old->mods.mask = new->mods.real_mods;
    old->mods.real_mods = new->mods.real_mods;
    old->mods.vmods = new->mods.vmods;
    old->level = new->level;
    return True;
}

static LookupEntry lnames[] = {
    {"level1", 1},
    {"level2", 2},
    {"level3", 3},
    {"level4", 4},
    {"level5", 5},
    {"level6", 6},
    {"level7", 7},
    {"level8", 8},
    {NULL, 0}
};

static Bool
SetMapEntry(KeyTypeInfo *type, XkbDescPtr xkb,
            const ExprDef *arrayNdx, const ExprDef *value)
{
    ExprResult rtrn;
    XkbKTMapEntryRec entry;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(type, "map entry");
    if (!ExprResolveModMask(arrayNdx, &rtrn, LookupVModMask, (XPointer) xkb))
        return ReportTypeBadType(type, "map entry", "modifier mask");
    entry.mods.real_mods = rtrn.uval & 0xff;      /* modifiers < 512 */
    entry.mods.vmods = (rtrn.uval >> 8) & 0xffff; /* modifiers > 512 */
    if ((entry.mods.real_mods & (~type->mask)) ||
        ((entry.mods.vmods & (~type->vmask)) != 0))
    {
        if (warningLevel > 0)
        {
            WARN("Map entry for unused modifiers in %s\n", TypeTxt(type));
            ACTION("Using %s instead of ",
                    XkbVModMaskText(type->dpy, xkb,
                                    entry.mods.real_mods & type->mask,
                                    entry.mods.vmods & type->vmask,
                                    XkbMessage));
            INFO("%s\n", MapEntryTxt(type, xkb, &entry));
        }
        entry.mods.real_mods &= type->mask;
        entry.mods.vmods &= type->vmask;
    }
    if (!ExprResolveInteger(value, &rtrn, SimpleLookup, (XPointer) lnames))
    {
        ERROR("Level specifications in a key type must be integer\n");
        ACTION("Ignoring malformed level specification\n");
        return False;
    }
    if ((rtrn.ival < 1) || (rtrn.ival > XkbMaxShiftLevel + 1))
    {
        ERROR("Shift level %d out of range (1..%d) in key type %s\n",
               XkbMaxShiftLevel + 1, rtrn.ival, TypeTxt(type));
        ACTION("Ignoring illegal definition of map[%s]\n",
                MapEntryTxt(type, xkb, &entry));
        return False;
    }
    entry.level = rtrn.ival - 1;
    return AddMapEntry(xkb, type, &entry, True, True);
}

static Bool
SetPreserve(KeyTypeInfo *type, XkbDescPtr xkb,
            const ExprDef *arrayNdx, const ExprDef *value)
{
    ExprResult rtrn;
    PreserveInfo new;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(type, "preserve entry");
    if (!ExprResolveModMask(arrayNdx, &rtrn, LookupVModMask, (XPointer) xkb))
        return ReportTypeBadType(type, "preserve entry", "modifier mask");
    new.defs = type->defs;
    new.defs.next = NULL;
    new.indexMods = rtrn.uval & 0xff;
    new.indexVMods = (rtrn.uval >> 8) & 0xffff;
    if ((new.indexMods & (~type->mask)) || (new.indexVMods & (~type->vmask)))
    {
        if (warningLevel > 0)
        {
            WARN("Preserve for modifiers not used by the %s type\n",
                  TypeTxt(type));
            ACTION("Index %s converted to ",
                    PreserveIndexTxt(type, xkb, &new));
        }
        new.indexMods &= type->mask;
        new.indexVMods &= type->vmask;
        if (warningLevel > 0)
            INFO("%s\n", PreserveIndexTxt(type, xkb, &new));
    }
    if (!ExprResolveModMask(value, &rtrn, LookupVModMask, (XPointer) xkb))
    {
        ERROR("Preserve value in a key type is not a modifier mask\n");
        ACTION("Ignoring preserve[%s] in type %s\n",
                PreserveIndexTxt(type, xkb, &new), TypeTxt(type));
        return False;
    }
    new.preMods = rtrn.uval & 0xff;
    new.preVMods = (rtrn.uval >> 16) & 0xffff;
    if ((new.preMods & (~new.indexMods))
        || (new.preVMods & (~new.indexVMods)))
    {
        if (warningLevel > 0)
        {
            WARN("Illegal value for preserve[%s] in type %s\n",
                  PreserveTxt(type, xkb, &new), TypeTxt(type));
            ACTION("Converted %s to ", PreserveIndexTxt(type, xkb, &new));
        }
        new.preMods &= new.indexMods;
        new.preVMods &= new.indexVMods;
        if (warningLevel > 0)
        {
            INFO("%s\n", PreserveIndexTxt(type, xkb, &new));
        }
    }
    return AddPreserve(xkb, type, &new, True, True);
}

/***====================================================================***/

static Bool
AddLevelName(KeyTypeInfo * type,
             unsigned level, Atom name, Bool clobber, Bool report)
{
    if ((type->lvlNames == NULL) || (type->szNames <= level))
    {
        type->lvlNames = recallocarray(type->lvlNames, type->szNames,
                                       level + 1, sizeof(Atom));
        if (type->lvlNames == NULL)
        {
            ERROR("Couldn't allocate level names for type %s\n",
                   TypeTxt(type));
            ACTION("Level names lost\n");
            type->szNames = 0;
            return False;
        }
        type->szNames = level + 1;
    }
    else if (type->lvlNames[level] == name)
    {
        if (warningLevel > 9)
        {
            WARN("Duplicate names for level %d of key type %s\n",
                  level + 1, TypeTxt(type));
            ACTION("Ignored\n");
        }
        return True;
    }
    else if (type->lvlNames[level] != None)
    {
        if (warningLevel > 0)
        {
            char *old, *new;
            old = XkbAtomText(type->dpy, type->lvlNames[level], XkbMessage);
            new = XkbAtomText(type->dpy, name, XkbMessage);
            WARN("Multiple names for level %d of key type %s\n",
                  level + 1, TypeTxt(type));
            if (clobber)
                ACTION("Using %s, ignoring %s\n", new, old);
            else
                ACTION("Using %s, ignoring %s\n", old, new);
        }
        if (!clobber)
            return True;
    }
    if (level >= type->numLevels)
        type->numLevels = level + 1;
    type->lvlNames[level] = name;
    return True;
}

static Bool
SetLevelName(KeyTypeInfo *type, const ExprDef *arrayNdx, const ExprDef *value)
{
    ExprResult rtrn;
    unsigned level;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(type, "level name");
    if (!ExprResolveInteger(arrayNdx, &rtrn, SimpleLookup, (XPointer) lnames))
        return ReportTypeBadType(type, "level name", "integer");
    if ((rtrn.ival < 1) || (rtrn.ival > XkbMaxShiftLevel + 1))
    {
        ERROR("Level name %d out of range (1..%d) in key type %s\n",
               rtrn.ival,
               XkbMaxShiftLevel + 1,
               XkbAtomText(type->dpy, type->name, XkbMessage));
        ACTION("Ignoring illegal level name definition\n");
        return False;
    }
    level = rtrn.ival - 1;
    if (!ExprResolveString(value, &rtrn, NULL, NULL))
    {
        ERROR("Non-string name for level %d in key type %s\n", level + 1,
               XkbAtomText(type->dpy, type->name, XkbMessage));
        ACTION("Ignoring illegal level name definition\n");
        return False;
    }
    return
        AddLevelName(type, level, XkbInternAtom(NULL, rtrn.str, False), True,
                     True);
}

/***====================================================================***/

/**
 * Parses the fields in a type "..." { } description.
 *
 * @param field The field to parse (e.g. modifiers, map, level_name)
 */
static Bool
SetKeyTypeField(KeyTypeInfo * type, XkbDescPtr xkb, const char *field,
                const ExprDef *arrayNdx, const ExprDef *value,
                KeyTypesInfo *info)
{
    if (uStrCaseCmp(field, "modifiers") == 0)
    {
        ExprResult tmp;
        unsigned mods, vmods;

        if (arrayNdx != NULL)
        {
            WARN("The modifiers field of a key type is not an array\n");
            ACTION("Illegal array subscript ignored\n");
        }
        /* get modifier mask for current type */
        if (!ExprResolveModMask(value, &tmp, LookupVModMask, (XPointer) xkb))
        {
            ERROR("Key type mask field must be a modifier mask\n");
            ACTION("Key type definition ignored\n");
            return False;
        }
        mods = tmp.uval & 0xff; /* core mods */
        vmods = (tmp.uval >> 8) & 0xffff; /* xkb virtual mods */
        if (type->defs.defined & _KT_Mask)
        {
            WARN("Multiple modifier mask definitions for key type %s\n",
                  XkbAtomText(type->dpy, type->name, XkbMessage));
            ACTION("Using %s, ", TypeMaskTxt(type, xkb));
            INFO("ignoring %s\n", XkbVModMaskText(type->dpy, xkb, mods,
                                                   vmods, XkbMessage));
            return False;
        }
        type->mask = mods;
        type->vmask = vmods;
        type->defs.defined |= _KT_Mask;
        return True;
    }
    else if (uStrCaseCmp(field, "map") == 0)
    {
        type->defs.defined |= _KT_Map;
        return SetMapEntry(type, xkb, arrayNdx, value);
    }
    else if (uStrCaseCmp(field, "preserve") == 0)
    {
        type->defs.defined |= _KT_Preserve;
        return SetPreserve(type, xkb, arrayNdx, value);
    }
    else if ((uStrCaseCmp(field, "levelname") == 0) ||
             (uStrCaseCmp(field, "level_name") == 0))
    {
        type->defs.defined |= _KT_LevelNames;
        return SetLevelName(type, arrayNdx, value);
    }
    ERROR("Unknown field %s in key type %s\n", field, TypeTxt(type));
    ACTION("Definition ignored\n");
    return False;
}

static Bool
HandleKeyTypeVar(const VarDef *stmt, XkbDescPtr xkb, KeyTypesInfo *info)
{
    ExprResult elem, field;
    ExprDef *arrayNdx;

    if (!ExprResolveLhs(stmt->name, &elem, &field, &arrayNdx))
        return False;           /* internal error, already reported */
    if (elem.str && (uStrCaseCmp(elem.str, "type") == 0))
        return SetKeyTypeField(&info->dflt, xkb, field.str, arrayNdx,
                               stmt->value, info);
    if (elem.str != NULL)
    {
        ERROR("Default for unknown element %s\n", uStringText(elem.str));
        ACTION("Value for field %s ignored\n", uStringText(field.str));
    }
    else if (field.str != NULL)
    {
        ERROR("Default defined for unknown field %s\n",
               uStringText(field.str));
        ACTION("Ignored\n");
    }
    return False;
}

static int
HandleKeyTypeBody(const VarDef *def, XkbDescPtr xkb,
                  KeyTypeInfo *type, KeyTypesInfo *info)
{
    int ok = 1;
    ExprResult tmp, field;
    ExprDef *arrayNdx;

    for (; def != NULL; def = (VarDef *) def->common.next)
    {
        if ((def->name) && (def->name->type == ExprFieldRef))
        {
            ok = HandleKeyTypeVar(def, xkb, info);
            continue;
        }
        ok = ExprResolveLhs(def->name, &tmp, &field, &arrayNdx);
        if (ok)
            ok = SetKeyTypeField(type, xkb, field.str, arrayNdx, def->value,
                                 info);
    }
    return ok;
}

/**
 * Process a type "XYZ" { } specification in the xkb_types section.
 *
 */
static int
HandleKeyTypeDef(const KeyTypeDef *def,
                 XkbDescPtr xkb, unsigned merge, KeyTypesInfo *info)
{
    KeyTypeInfo type = {
        .defs.defined = 0,
        .defs.fileID = info->fileID,
        .defs.merge = (def->merge != MergeDefault) ? def->merge : merge,
        .defs.next = NULL,
        .dpy = info->dpy,
        .name = def->name,
        .mask = info->dflt.mask,
        .vmask = info->dflt.vmask,
        .groupInfo = info->dflt.groupInfo,
        .numLevels = 1,
        .nEntries = 0,
        .szEntries = 0,
        .entries = NULL,
        .preserve = NULL,
        .szNames = 0,
        .lvlNames = NULL
    };

    /* Parse the actual content. */
    if (!HandleKeyTypeBody(def->body, xkb, &type, info))
    {
        info->errorCount++;
        return False;
    }

    /* now copy any appropriate map, preserve or level names from the */
    /* default type */
    for (int i = 0; i < info->dflt.nEntries; i++)
    {
        XkbKTMapEntryPtr dflt;
        dflt = &info->dflt.entries[i];
        if (((dflt->mods.real_mods & type.mask) == dflt->mods.real_mods) &&
            ((dflt->mods.vmods & type.vmask) == dflt->mods.vmods))
        {
            AddMapEntry(xkb, &type, dflt, False, False);
        }
    }
    if (info->dflt.preserve)
    {
        PreserveInfo *dflt = info->dflt.preserve;
        while (dflt)
        {
            if (((dflt->indexMods & type.mask) == dflt->indexMods) &&
                ((dflt->indexVMods & type.vmask) == dflt->indexVMods))
            {
                AddPreserve(xkb, &type, dflt, False, False);
            }
            dflt = (PreserveInfo *) dflt->defs.next;
        }
    }
    for (int i = 0; i < info->dflt.szNames; i++)
    {
        if ((i < type.numLevels) && (info->dflt.lvlNames[i] != None))
        {
            AddLevelName(&type, i, info->dflt.lvlNames[i], False, False);
        }
    }
    /* Now add the new keytype to the info struct */
    if (!AddKeyType(xkb, info, &type))
    {
        info->errorCount++;
        return False;
    }
    return True;
}

/**
 * Process an xkb_types section.
 *
 * @param file The parsed xkb_types section.
 * @param merge Merge Strategy (e.g. MergeOverride)
 * @param info Pointer to memory where the outcome will be stored.
 */
static void
HandleKeyTypesFile(XkbFile * file,
                   XkbDescPtr xkb, unsigned merge, KeyTypesInfo * info)
{
    ParseCommon *stmt;

    info->name = uStringDup(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->stmtType)
        {
        case StmtInclude:
            if (!HandleIncludeKeyTypes((IncludeStmt *) stmt, xkb, info,
                                       HandleKeyTypesFile))
                info->errorCount++;
            break;
        case StmtKeyTypeDef: /* e.g. type "ONE_LEVEL" */
            if (!HandleKeyTypeDef((KeyTypeDef *) stmt, xkb, merge, info))
                info->errorCount++;
            break;
        case StmtVarDef:
            if (!HandleKeyTypeVar((VarDef *) stmt, xkb, info))
                info->errorCount++;
            break;
        case StmtVModDef: /* virtual_modifiers NumLock, ... */
            if (!HandleVModDef((VModDef *) stmt, merge, &info->vmods))
                info->errorCount++;
            break;
        case StmtKeyAliasDef:
            ERROR("Key type files may not include other declarations\n");
            ACTION("Ignoring definition of key alias\n");
            info->errorCount++;
            break;
        case StmtKeycodeDef:
            ERROR("Key type files may not include other declarations\n");
            ACTION("Ignoring definition of key name\n");
            info->errorCount++;
            break;
        case StmtInterpDef:
            ERROR("Key type files may not include other declarations\n");
            ACTION("Ignoring definition of symbol interpretation\n");
            info->errorCount++;
            break;
        default:
            WSGO("Unexpected statement type %d in HandleKeyTypesFile\n",
                  stmt->stmtType);
            break;
        }
        stmt = stmt->next;
        if (info->errorCount > 10)
        {
#ifdef NOISY
            ERROR("Too many errors\n");
#endif
            ACTION("Abandoning keytypes file \"%s\"\n", file->topName);
            break;
        }
    }
    return;
}

static Bool
CopyDefToKeyType(XkbDescPtr xkb, XkbKeyTypePtr type, KeyTypeInfo * def)
{
    for (PreserveInfo *pre = def->preserve; pre != NULL;
         pre = (PreserveInfo *) pre->defs.next)
    {
        XkbKTMapEntryPtr match;
        XkbKTMapEntryRec tmp = {
            .mods.real_mods = pre->indexMods,
            .mods.vmods = pre->indexVMods,
            .level = 0
        };

        AddMapEntry(xkb, def, &tmp, False, False);
        match = FindMatchingMapEntry(def, pre->indexMods, pre->indexVMods);
        if (!match)
        {
            WSGO("Couldn't find matching entry for preserve\n");
            ACTION("Aborting\n");
            return False;
        }
        pre->matchingMapIndex = match - def->entries;
    }
    type->mods.real_mods = def->mask;
    type->mods.vmods = def->vmask;
    type->num_levels = def->numLevels;
    type->map_count = def->nEntries;
    type->map = def->entries;
    if (def->preserve)
    {
        type->preserve = calloc(type->map_count, sizeof(XkbModsRec));
        if (!type->preserve)
        {
            WARN("Couldn't allocate preserve array in CopyDefToKeyType\n");
            ACTION("Preserve setting for type %s lost\n",
                    XkbAtomText(def->dpy, def->name, XkbMessage));
        }
        else
        {
            for (PreserveInfo *pre = def->preserve; pre != NULL;
                 pre = (PreserveInfo *) pre->defs.next)
            {
                int ndx = pre->matchingMapIndex;
                type->preserve[ndx].mask = pre->preMods;
                type->preserve[ndx].real_mods = pre->preMods;
                type->preserve[ndx].vmods = pre->preVMods;
            }
        }
    }
    else
        type->preserve = NULL;
    type->name = (Atom) def->name;
    if (def->szNames > 0)
    {
        type->level_names = calloc(def->numLevels, sizeof(Atom));

        /* assert def->szNames<=def->numLevels */
        for (int i = 0; i < def->szNames; i++)
        {
            type->level_names[i] = (Atom) def->lvlNames[i];
        }
    }
    else
    {
        type->level_names = NULL;
    }

    def->nEntries = def->szEntries = 0;
    def->entries = NULL;
    return XkbComputeEffectiveMap(xkb, type, NULL);
}

Bool
CompileKeyTypes(XkbFile * file, XkbFileInfo * result, unsigned merge)
{
    KeyTypesInfo info;
    XkbDescPtr xkb;

    xkb = result->xkb;
    InitKeyTypesInfo(&info, xkb, NULL);
    info.fileID = file->id;
    HandleKeyTypesFile(file, xkb, merge, &info);

    if (info.errorCount == 0)
    {
        int i;
        KeyTypeInfo *def;
        XkbKeyTypePtr type, next;

        if (info.name != NULL)
        {
            if (XkbAllocNames(xkb, XkbTypesNameMask, 0, 0) == Success)
                xkb->names->types = XkbInternAtom(xkb->dpy, info.name, False);
            else
            {
                WSGO("Couldn't allocate space for types name\n");
                ACTION("Name \"%s\" (from %s) NOT assigned\n",
                        scanFile, info.name);
            }
        }
        i = info.nTypes;
        if ((info.stdPresent & XkbOneLevelMask) == 0)
            i++;
        if ((info.stdPresent & XkbTwoLevelMask) == 0)
            i++;
        if ((info.stdPresent & XkbKeypadMask) == 0)
            i++;
        if ((info.stdPresent & XkbAlphabeticMask) == 0)
            i++;
        if (XkbAllocClientMap(xkb, XkbKeyTypesMask, i) != Success)
        {
            WSGO("Couldn't allocate client map\n");
            ACTION("Exiting\n");
            return False;
        }
        xkb->map->num_types = i;
        if (XkbAllRequiredTypes & (~info.stdPresent))
        {
            unsigned missing, keypadVMod;

            missing = XkbAllRequiredTypes & (~info.stdPresent);
            keypadVMod = FindKeypadVMod(xkb);
            if (XkbInitCanonicalKeyTypes(xkb, missing, keypadVMod) != Success)
            {
                WSGO("Couldn't initialize canonical key types\n");
                ACTION("Exiting\n");
                return False;
            }
            if (missing & XkbOneLevelMask)
                xkb->map->types[XkbOneLevelIndex].name = tok_ONE_LEVEL;
            if (missing & XkbTwoLevelMask)
                xkb->map->types[XkbTwoLevelIndex].name = tok_TWO_LEVEL;
            if (missing & XkbAlphabeticMask)
                xkb->map->types[XkbAlphabeticIndex].name = tok_ALPHABETIC;
            if (missing & XkbKeypadMask)
                xkb->map->types[XkbKeypadIndex].name = tok_KEYPAD;
        }
        next = &xkb->map->types[XkbLastRequiredType + 1];
        for (i = 0, def = info.types; i < info.nTypes; i++)
        {
            if (def->name == tok_ONE_LEVEL)
                type = &xkb->map->types[XkbOneLevelIndex];
            else if (def->name == tok_TWO_LEVEL)
                type = &xkb->map->types[XkbTwoLevelIndex];
            else if (def->name == tok_ALPHABETIC)
                type = &xkb->map->types[XkbAlphabeticIndex];
            else if (def->name == tok_KEYPAD)
                type = &xkb->map->types[XkbKeypadIndex];
            else
                type = next++;
            DeleteLevel1MapEntries(def);
            if (!CopyDefToKeyType(xkb, type, def))
                return False;
            def = (KeyTypeInfo *) def->defs.next;
        }
        return True;
    }
    return False;
}
