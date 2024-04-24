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
#include "parseutils.h"

#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <stdlib.h>

#include "expr.h"
#include "vmod.h"
#include "action.h"
#include "keycodes.h"
#include "misc.h"
#include "alias.h"

/***====================================================================***/

#define	RepeatYes	1
#define	RepeatNo	0
#define	RepeatUndefined	~((unsigned)0)

#define	_Key_Syms	(1<<0)
#define	_Key_Acts	(1<<1)
#define	_Key_Repeat	(1<<2)
#define	_Key_Behavior	(1<<3)
#define	_Key_Type_Dflt	(1<<4)
#define	_Key_Types	(1<<5)
#define	_Key_GroupInfo	(1<<6)
#define	_Key_VModMap	(1<<7)

typedef struct _KeyInfo
{
    CommonInfo defs;
    unsigned long name; /* the 4 chars of the key name, as long */
    unsigned char groupInfo;
    unsigned char typesDefined;
    unsigned char symsDefined;
    unsigned char actsDefined;
    short numLevels[XkbNumKbdGroups];
    KeySym *syms[XkbNumKbdGroups];
    XkbAction *acts[XkbNumKbdGroups];
    Atom types[XkbNumKbdGroups];
    unsigned repeat;
    XkbBehavior behavior;
    unsigned short vmodmap;
    unsigned long nameForOverlayKey;
    unsigned long allowNone;
    Atom dfltType;
} KeyInfo;

/**
 * Init the given key info to sane values.
 */
static void
InitKeyInfo(KeyInfo * info)
{
    static char dflt[4] = "*";

    info->defs.defined = 0;
    info->defs.fileID = 0;
    info->defs.merge = MergeOverride;
    info->defs.next = NULL;
    info->name = KeyNameToLong(dflt);
    info->groupInfo = 0;
    info->typesDefined = info->symsDefined = info->actsDefined = 0;
    for (int i = 0; i < XkbNumKbdGroups; i++)
    {
        info->numLevels[i] = 0;
        info->types[i] = None;
        info->syms[i] = NULL;
        info->acts[i] = NULL;
    }
    info->dfltType = None;
    info->behavior.type = XkbKB_Default;
    info->behavior.data = 0;
    info->vmodmap = 0;
    info->nameForOverlayKey = 0;
    info->repeat = RepeatUndefined;
    info->allowNone = 0;
    return;
}

/**
 * Free memory associated with this key info and reset to sane values.
 */
static void
FreeKeyInfo(KeyInfo * info)
{
    info->defs.defined = 0;
    info->defs.fileID = 0;
    info->defs.merge = MergeOverride;
    info->defs.next = NULL;
    info->groupInfo = 0;
    info->typesDefined = info->symsDefined = info->actsDefined = 0;
    for (int i = 0; i < XkbNumKbdGroups; i++)
    {
        info->numLevels[i] = 0;
        info->types[i] = None;
        free(info->syms[i]);
        info->syms[i] = NULL;
        free(info->acts[i]);
        info->acts[i] = NULL;
    }
    info->dfltType = None;
    info->behavior.type = XkbKB_Default;
    info->behavior.data = 0;
    info->vmodmap = 0;
    info->nameForOverlayKey = 0;
    info->repeat = RepeatUndefined;
    info->allowNone = 0;
    return;
}

/**
 * Copy old into new, optionally reset old to 0.
 * If old is reset, new simply re-uses old's memory. Otherwise, the memory is
 * newly allocated and new points to the new memory areas.
 */
static Bool
CopyKeyInfo(KeyInfo * old, KeyInfo * new, Bool clearOld)
{
    *new = *old;
    new->defs.next = NULL;
    if (clearOld)
    {
        for (int i = 0; i < XkbNumKbdGroups; i++)
        {
            old->numLevels[i] = 0;
            old->syms[i] = NULL;
            old->acts[i] = NULL;
        }
    }
    else
    {
        for (int i = 0; i < XkbNumKbdGroups; i++)
        {
            int width = new->numLevels[i];
            if (old->syms[i] != NULL)
            {
                new->syms[i] = calloc(width, sizeof(KeySym));
                if (!new->syms[i])
                {
                    new->syms[i] = NULL;
                    new->numLevels[i] = 0;
                    return False;
                }
                memcpy(new->syms[i], old->syms[i], width * sizeof(KeySym));
            }
            if (old->acts[i] != NULL)
            {
                new->acts[i] = calloc(width, sizeof(XkbAction));
                if (!new->acts[i])
                {
                    new->acts[i] = NULL;
                    return False;
                }
                memcpy(new->acts[i], old->acts[i], width * sizeof(XkbAction));
            }
        }
    }
    return True;
}

/***====================================================================***/

typedef struct _ModMapEntry
{
    CommonInfo defs;
    Bool haveSymbol;
    int modifier;
    union
    {
        unsigned long keyName;
        KeySym keySym;
    } u;
} ModMapEntry;

#define	SYMBOLS_INIT_SIZE	110
#define	SYMBOLS_CHUNK		20
typedef struct _SymbolsInfo
{
    char *name;         /* e.g. pc+us+inet(evdev) */
    int errorCount;
    unsigned fileID;
    unsigned merge;
    unsigned explicit_group;
    unsigned groupInfo;
    unsigned szKeys;
    unsigned nKeys;
    KeyInfo *keys;
    KeyInfo dflt;
    VModInfo vmods;
    ActionInfo *action;
    Atom groupNames[XkbNumKbdGroups];

    ModMapEntry *modMap;
    AliasInfo *aliases;
} SymbolsInfo;

static void
InitSymbolsInfo(SymbolsInfo * info, XkbDescPtr xkb)
{
    tok_ONE_LEVEL = XkbInternAtom(NULL, "ONE_LEVEL", False);
    tok_TWO_LEVEL = XkbInternAtom(NULL, "TWO_LEVEL", False);
    tok_KEYPAD = XkbInternAtom(NULL, "KEYPAD", False);
    info->name = NULL;
    info->explicit_group = 0;
    info->errorCount = 0;
    info->fileID = 0;
    info->merge = MergeOverride;
    info->groupInfo = 0;
    info->szKeys = SYMBOLS_INIT_SIZE;
    info->nKeys = 0;
    info->keys = calloc(SYMBOLS_INIT_SIZE, sizeof(KeyInfo));
    info->modMap = NULL;
    for (int i = 0; i < XkbNumKbdGroups; i++)
        info->groupNames[i] = None;
    InitKeyInfo(&info->dflt);
    InitVModInfo(&info->vmods, xkb);
    info->action = NULL;
    info->aliases = NULL;
    return;
}

static void
FreeSymbolsInfo(SymbolsInfo * info)
{
    free(info->name);
    info->name = NULL;
    if (info->keys)
    {
        for (int i = 0; i < info->nKeys; i++)
        {
            FreeKeyInfo(&info->keys[i]);
        }
        free(info->keys);
        info->keys = NULL;
    }
    if (info->modMap)
    {
        ClearCommonInfo(&info->modMap->defs);
        info->modMap = NULL;
    }
    if (info->aliases)
    {
        ClearAliases(&info->aliases);
        info->aliases = NULL;
    }
    bzero(info, sizeof(SymbolsInfo));
    return;
}

static Bool
ResizeKeyGroup(KeyInfo * key,
               unsigned group, unsigned atLeastSize, Bool forceActions)
{
    Bool tooSmall;
    unsigned newWidth;

    tooSmall = (key->numLevels[group] < atLeastSize);
    if (tooSmall)
        newWidth = atLeastSize;
    else
        newWidth = key->numLevels[group];

    if ((key->syms[group] == NULL) || tooSmall)
    {
        key->syms[group] = recallocarray(key->syms[group],
                                         key->numLevels[group], newWidth,
                                         sizeof(KeySym));
        if (!key->syms[group])
            return False;
    }
    if (((forceActions) && (tooSmall || (key->acts[group] == NULL))) ||
        (tooSmall && (key->acts[group] != NULL)))
    {
        key->acts[group] = recallocarray(key->acts[group],
                                         key->numLevels[group], newWidth,
                                         sizeof(XkbAction));
        if (!key->acts[group])
            return False;
    }
    key->numLevels[group] = newWidth;
    return True;
}

static Bool
MergeKeyGroups(SymbolsInfo * info,
               KeyInfo * into, KeyInfo * from, unsigned group)
{
    KeySym *resultSyms;
    XkbAction *resultActs;
    int resultWidth;
    Bool report, clobber;

    clobber = (from->defs.merge != MergeAugment);
    report = (warningLevel > 9) ||
        ((into->defs.fileID == from->defs.fileID) && (warningLevel > 0));
    if ((from->numLevels[group] > into->numLevels[group])
        || (clobber && (from->types[group] != None)))
    {
        resultSyms = from->syms[group];
        resultActs = from->acts[group];
        resultWidth = from->numLevels[group];
    }
    else
    {
        resultSyms = into->syms[group];
        resultActs = into->acts[group];
        resultWidth = into->numLevels[group];
    }
    if (resultSyms == NULL)
    {
        resultSyms = calloc(resultWidth, sizeof(KeySym));
        if (!resultSyms)
        {
            WSGO("Could not allocate symbols for group merge\n");
            ACTION("Group %d of key %s not merged\n", group,
                    longText(into->name, XkbMessage));
            return False;
        }
    }
    if ((resultActs == NULL) && (into->acts[group] || from->acts[group]))
    {
        resultActs = calloc(resultWidth, sizeof(XkbAction));
        if (!resultActs)
        {
            WSGO("Could not allocate actions for group merge\n");
            ACTION("Group %d of key %s not merged\n", group,
                    longText(into->name, XkbMessage));
            return False;
        }
    }
    for (int i = 0; i < resultWidth; i++)
    {
        KeySym fromSym, toSym;
        if (from->syms[group] && (i < from->numLevels[group]))
            fromSym = from->syms[group][i];
        else
            fromSym = NoSymbol;
        if (into->syms[group] && (i < into->numLevels[group]))
            toSym = into->syms[group][i];
        else
            toSym = NoSymbol;
        if ((fromSym == NoSymbol) || (fromSym == toSym))
            resultSyms[i] = toSym;
        else if (toSym == NoSymbol)
            resultSyms[i] = fromSym;
        else
        {
            KeySym use, ignore;
            if (clobber)
            {
                use = fromSym;
                ignore = toSym;
            }
            else
            {
                use = toSym;
                ignore = fromSym;
            }
            if (report)
            {
                WARN
                    ("Multiple symbols for level %d/group %d on key %s\n",
                     i + 1, group + 1, longText(into->name, XkbMessage));
                ACTION("Using %s, ignoring %s\n",
                        XkbKeysymText(use, XkbMessage),
                        XkbKeysymText(ignore, XkbMessage));
            }
            resultSyms[i] = use;
        }
        if (resultActs != NULL)
        {
            XkbAction *fromAct, *toAct;
            fromAct = (from->acts[group] ? &from->acts[group][i] : NULL);
            toAct = (into->acts[group] ? &into->acts[group][i] : NULL);
            if (((fromAct == NULL) || (fromAct->type == XkbSA_NoAction))
                && (toAct != NULL))
            {
                resultActs[i] = *toAct;
            }
            else if (((toAct == NULL) || (toAct->type == XkbSA_NoAction))
                     && (fromAct != NULL))
            {
                resultActs[i] = *fromAct;
            }
            else
            {
                XkbAction *use, *ignore;
                if (clobber)
                {
                    use = fromAct;
                    ignore = toAct;
                }
                else
                {
                    use = toAct;
                    ignore = fromAct;
                }
                if (report)
                {
                    WARN
                        ("Multiple actions for level %d/group %d on key %s\n",
                         i + 1, group + 1, longText(into->name, XkbMessage));
                    ACTION("Using %s, ignoring %s\n",
                            XkbActionTypeText(use->type, XkbMessage),
                            XkbActionTypeText(ignore->type, XkbMessage));
                }
                resultActs[i] = *use;
            }
        }
    }
    if ((into->syms[group] != NULL) && (resultSyms != into->syms[group]))
        free(into->syms[group]);
    if ((from->syms[group] != NULL) && (resultSyms != from->syms[group]))
        free(from->syms[group]);
    if ((into->acts[group] != NULL) && (resultActs != into->acts[group]))
        free(into->acts[group]);
    if ((from->acts[group] != NULL) && (resultActs != from->acts[group]))
        free(from->acts[group]);
    into->numLevels[group] = resultWidth;
    into->syms[group] = resultSyms;
    from->syms[group] = NULL;
    into->acts[group] = resultActs;
    from->acts[group] = NULL;
    into->symsDefined |= (1U << group);
    from->symsDefined &= ~(1U << group);
    into->actsDefined |= (1U << group);
    from->actsDefined &= ~(1U << group);
    return True;
}

static Bool
MergeKeys(SymbolsInfo * info, KeyInfo * into, KeyInfo * from)
{
    unsigned collide = 0;
    Bool report;

    if (from->defs.merge == MergeReplace)
    {
        for (int i = 0; i < XkbNumKbdGroups; i++)
        {
            if (into->numLevels[i] != 0)
            {
                free(into->syms[i]);
                free(into->acts[i]);
            }
        }
        *into = *from;
        bzero(from, sizeof(KeyInfo));
        return True;
    }
    report = ((warningLevel > 9) ||
              ((into->defs.fileID == from->defs.fileID)
               && (warningLevel > 0)));
    for (int i = 0; i < XkbNumKbdGroups; i++)
    {
        if (from->numLevels[i] > 0)
        {
            if (into->numLevels[i] == 0)
            {
                into->numLevels[i] = from->numLevels[i];
                into->syms[i] = from->syms[i];
                into->acts[i] = from->acts[i];
                into->symsDefined |= (1U << i);
                from->syms[i] = NULL;
                from->acts[i] = NULL;
                from->numLevels[i] = 0;
                from->symsDefined &= ~(1U << i);
                if (into->syms[i])
                    into->defs.defined |= _Key_Syms;
                if (into->acts[i])
                    into->defs.defined |= _Key_Acts;
            }
            else
            {
                if (report)
                {
                    if (into->syms[i])
                        collide |= _Key_Syms;
                    if (into->acts[i])
                        collide |= _Key_Acts;
                }
                MergeKeyGroups(info, into, from, (unsigned) i);
            }
        }
        if (from->types[i] != None)
        {
            if ((into->types[i] != None) && (report) &&
                (into->types[i] != from->types[i]))
            {
                Atom use, ignore;
                collide |= _Key_Types;
                if (from->defs.merge != MergeAugment)
                {
                    use = from->types[i];
                    ignore = into->types[i];
                }
                else
                {
                    use = into->types[i];
                    ignore = from->types[i];
                }
                WARN
                    ("Multiple definitions for group %d type of key %s\n",
                     i, longText(into->name, XkbMessage));
                ACTION("Using %s, ignoring %s\n",
                        XkbAtomText(NULL, use, XkbMessage),
                        XkbAtomText(NULL, ignore, XkbMessage));
            }
            if ((from->defs.merge != MergeAugment)
                || (into->types[i] == None))
            {
                into->types[i] = from->types[i];
            }
        }
    }
    if (UseNewField(_Key_Behavior, &into->defs, &from->defs, &collide))
    {
        into->behavior = from->behavior;
        into->nameForOverlayKey = from->nameForOverlayKey;
        into->defs.defined |= _Key_Behavior;
    }
    if (UseNewField(_Key_VModMap, &into->defs, &from->defs, &collide))
    {
        into->vmodmap = from->vmodmap;
        into->defs.defined |= _Key_VModMap;
    }
    if (UseNewField(_Key_Repeat, &into->defs, &from->defs, &collide))
    {
        into->repeat = from->repeat;
        into->defs.defined |= _Key_Repeat;
    }
    if (UseNewField(_Key_Type_Dflt, &into->defs, &from->defs, &collide))
    {
        into->dfltType = from->dfltType;
        into->defs.defined |= _Key_Type_Dflt;
    }
    if (UseNewField(_Key_GroupInfo, &into->defs, &from->defs, &collide))
    {
        into->groupInfo = from->groupInfo;
        into->defs.defined |= _Key_GroupInfo;
    }
    if (collide && (warningLevel > 0))
    {
        WARN("Symbol map for key %s redefined\n",
              longText(into->name, XkbMessage));
        ACTION("Using %s definition for conflicting fields\n",
                (from->defs.merge == MergeAugment ? "first" : "last"));
    }
    return True;
}

static Bool
AddKeySymbols(SymbolsInfo * info, KeyInfo * key, XkbDescPtr xkb)
{
    unsigned long real_name;

    for (int i = 0; i < info->nKeys; i++)
    {
        if (info->keys[i].name == key->name)
            return MergeKeys(info, &info->keys[i], key);
    }
    if (FindKeyNameForAlias(xkb, key->name, &real_name))
    {
        for (int i = 0; i < info->nKeys; i++)
        {
            if (info->keys[i].name == real_name)
                return MergeKeys(info, &info->keys[i], key);
        }
    }
    if (info->nKeys >= info->szKeys)
    {
        info->szKeys += SYMBOLS_CHUNK;
        info->keys = recallocarray(info->keys, info->nKeys, info->szKeys,
                                   sizeof(KeyInfo));
        if (!info->keys)
        {
            WSGO("Could not allocate key symbols descriptions\n");
            ACTION("Some key symbols definitions may be lost\n");
            return False;
        }
    }
    return CopyKeyInfo(key, &info->keys[info->nKeys++], True);
}

static Bool
AddModMapEntry(SymbolsInfo * info, ModMapEntry * new)
{
    ModMapEntry *mm;
    Bool clobber;

    clobber = (new->defs.merge != MergeAugment);
    for (mm = info->modMap; mm != NULL; mm = (ModMapEntry *) mm->defs.next)
    {
        if (new->haveSymbol && mm->haveSymbol
            && (new->u.keySym == mm->u.keySym))
        {
            if (mm->modifier != new->modifier)
            {
                unsigned use, ignore;

                if (clobber)
                {
                    use = new->modifier;
                    ignore = mm->modifier;
                }
                else
                {
                    use = mm->modifier;
                    ignore = new->modifier;
                }
                ERROR
                    ("%s added to symbol map for multiple modifiers\n",
                     XkbKeysymText(new->u.keySym, XkbMessage));
                ACTION("Using %s, ignoring %s.\n",
                        XkbModIndexText(use, XkbMessage),
                        XkbModIndexText(ignore, XkbMessage));
                mm->modifier = use;
            }
            return True;
        }
        if ((!new->haveSymbol) && (!mm->haveSymbol) &&
            (new->u.keyName == mm->u.keyName))
        {
            if (mm->modifier != new->modifier)
            {
                unsigned use, ignore;

                if (clobber)
                {
                    use = new->modifier;
                    ignore = mm->modifier;
                }
                else
                {
                    use = mm->modifier;
                    ignore = new->modifier;
                }
                ERROR("Key %s added to map for multiple modifiers\n",
                       longText(new->u.keyName, XkbMessage));
                ACTION("Using %s, ignoring %s.\n",
                        XkbModIndexText(use, XkbMessage),
                        XkbModIndexText(ignore, XkbMessage));
                mm->modifier = use;
            }
            return True;
        }
    }
    mm = malloc(sizeof(ModMapEntry));
    if (mm == NULL)
    {
        WSGO("Could not allocate modifier map entry\n");
        ACTION("Modifier map for %s will be incomplete\n",
                XkbModIndexText(new->modifier, XkbMessage));
        return False;
    }
    *mm = *new;
    mm->defs.next = &info->modMap->defs;
    info->modMap = mm;
    return True;
}

/***====================================================================***/

static void
MergeIncludedSymbols(SymbolsInfo * into, SymbolsInfo * from,
                     unsigned merge, XkbDescPtr xkb)
{
    int i;
    KeyInfo *key;

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
    for (i = 0; i < XkbNumKbdGroups; i++)
    {
        if (from->groupNames[i] != None)
        {
            if ((merge != MergeAugment) || (into->groupNames[i] == None))
                into->groupNames[i] = from->groupNames[i];
        }
    }
    for (i = 0, key = from->keys; i < from->nKeys; i++, key++)
    {
        if (merge != MergeDefault)
            key->defs.merge = merge;
        if (!AddKeySymbols(into, key, xkb))
            into->errorCount++;
    }
    if (from->modMap != NULL)
    {
        ModMapEntry *mm, *next;
        for (mm = from->modMap; mm != NULL; mm = next)
        {
            if (merge != MergeDefault)
                mm->defs.merge = merge;
            if (!AddModMapEntry(into, mm))
                into->errorCount++;
            next = (ModMapEntry *) mm->defs.next;
            free(mm);
        }
        from->modMap = NULL;
    }
    if (!MergeAliases(&into->aliases, &from->aliases, merge))
        into->errorCount++;
    return;
}

typedef void (*FileHandler) (XkbFile * /* rtrn */ ,
                             XkbDescPtr /* xkb */ ,
                             unsigned /* merge */ ,
                             SymbolsInfo *      /* included */
    );

static Bool
HandleIncludeSymbols(IncludeStmt * stmt,
                     XkbDescPtr xkb, SymbolsInfo * info, FileHandler hndlr)
{
    unsigned newMerge;
    XkbFile *rtrn;
    SymbolsInfo included;
    Bool haveSelf;

    haveSelf = False;
    if ((stmt->file == NULL) && (stmt->map == NULL))
    {
        haveSelf = True;
        included = *info;
        bzero(info, sizeof(SymbolsInfo));
    }
    else if (ProcessIncludeFile(stmt, XkmSymbolsIndex, &rtrn, &newMerge))
    {
        InitSymbolsInfo(&included, xkb);
        included.fileID = included.dflt.defs.fileID = rtrn->id;
        included.merge = included.dflt.defs.merge = MergeOverride;
        if (stmt->modifier)
        {
            included.explicit_group = atoi(stmt->modifier) - 1;
        }
        else
        {
            included.explicit_group = info->explicit_group;
        }
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
        SymbolsInfo next_incl;

        for (next = stmt->next; next != NULL; next = next->next)
        {
            if ((next->file == NULL) && (next->map == NULL))
            {
                haveSelf = True;
                MergeIncludedSymbols(&included, info, next->merge, xkb);
                FreeSymbolsInfo(info);
            }
            else if (ProcessIncludeFile(next, XkmSymbolsIndex, &rtrn, &op))
            {
                InitSymbolsInfo(&next_incl, xkb);
                next_incl.fileID = next_incl.dflt.defs.fileID = rtrn->id;
                next_incl.merge = next_incl.dflt.defs.merge = MergeOverride;
                if (next->modifier)
                {
                    next_incl.explicit_group = atoi(next->modifier) - 1;
                }
                else
                {
                    next_incl.explicit_group = info->explicit_group;
                }
                (*hndlr) (rtrn, xkb, MergeOverride, &next_incl);
                MergeIncludedSymbols(&included, &next_incl, op, xkb);
                FreeSymbolsInfo(&next_incl);
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
        MergeIncludedSymbols(info, &included, newMerge, xkb);
        FreeSymbolsInfo(&included);
    }
    return (info->errorCount == 0);
}

static LookupEntry groupNames[] = {
    {"group1", 1},
    {"group2", 2},
    {"group3", 3},
    {"group4", 4},
    {"group5", 5},
    {"group6", 6},
    {"group7", 7},
    {"group8", 8},
    {NULL, 0}
};


#define	SYMBOLS 1
#define	ACTIONS	2

static Bool
GetGroupIndex(KeyInfo *key, const ExprDef *arrayNdx,
              unsigned what, unsigned *ndx_rtrn)
{
    const char *name;
    ExprResult tmp;

    if (what == SYMBOLS)
        name = "symbols";
    else
        name = "actions";

    if (arrayNdx == NULL)
    {
        unsigned defined;
        if (what == SYMBOLS)
            defined = key->symsDefined;
        else
            defined = key->actsDefined;

        for (int i = 0; i < XkbNumKbdGroups; i++)
        {
            if ((defined & (1U << i)) == 0)
            {
                *ndx_rtrn = i;
                return True;
            }
        }
        ERROR("Too many groups of %s for key %s (max %d)\n", name,
               longText(key->name, XkbMessage), XkbNumKbdGroups + 1);
        ACTION("Ignoring %s defined for extra groups\n", name);
        return False;
    }
    if (!ExprResolveInteger
        (arrayNdx, &tmp, SimpleLookup, (XPointer) groupNames))
    {
        ERROR("Illegal group index for %s of key %s\n", name,
               longText(key->name, XkbMessage));
        ACTION("Definition with non-integer array index ignored\n");
        return False;
    }
    if ((tmp.uval < 1) || (tmp.uval > XkbNumKbdGroups))
    {
        ERROR("Group index for %s of key %s is out of range (1..%d)\n",
               name, longText(key->name, XkbMessage), XkbNumKbdGroups + 1);
        ACTION("Ignoring %s for group %d\n", name, tmp.uval);
        return False;
    }
    *ndx_rtrn = tmp.uval - 1;
    return True;
}

static Bool
AddSymbolsToKey(KeyInfo *key, XkbDescPtr xkb, const char *field,
                const ExprDef *arrayNdx, const ExprDef *value,
                SymbolsInfo *info)
{
    unsigned ndx, nSyms;

    if (!GetGroupIndex(key, arrayNdx, SYMBOLS, &ndx))
        return False;
    if (value == NULL)
    {
        key->symsDefined |= (1U << ndx);
        return True;
    }
    if (value->op != ExprKeysymList)
    {
        ERROR("Expected a list of symbols, found %s\n",
               exprOpText(value->op));
        ACTION("Ignoring symbols for group %d of %s\n", ndx,
                longText(key->name, XkbMessage));
        return False;
    }
    if (key->syms[ndx] != NULL)
    {
        WSGO("Symbols for key %s, group %d already defined\n",
              longText(key->name, XkbMessage), ndx);
        return False;
    }
    nSyms = value->value.list.nSyms;
    if (((key->numLevels[ndx] < nSyms) || (key->syms[ndx] == NULL)) &&
        (!ResizeKeyGroup(key, ndx, nSyms, False)))
    {
        WSGO("Could not resize group %d of key %s\n", ndx,
              longText(key->name, XkbMessage));
        ACTION("Symbols lost\n");
        return False;
    }
    key->symsDefined |= (1U << ndx);
    for (int i = 0; i < nSyms; i++) {
        if (!LookupKeysym(value->value.list.syms[i], &key->syms[ndx][i])) {
            if (warningLevel > 0)
            {
                WARN("Could not resolve keysym %s\n",
                      value->value.list.syms[i]);
            }
            key->syms[ndx][i] = NoSymbol;
        }
    }
    for (int i = key->numLevels[ndx] - 1;
         (i >= 0) && (key->syms[ndx][i] == NoSymbol); i--)
    {
        key->numLevels[ndx]--;
    }
    return True;
}

static Bool
AddActionsToKey(KeyInfo *key, XkbDescPtr xkb, const char *field,
                const ExprDef *arrayNdx, const ExprDef *value,
                SymbolsInfo *info)
{
    unsigned ndx, nActs;
    ExprDef *act;
    XkbAnyAction *toAct;

    if (!GetGroupIndex(key, arrayNdx, ACTIONS, &ndx))
        return False;

    if (value == NULL)
    {
        key->actsDefined |= (1U << ndx);
        return True;
    }
    if (value->op != ExprActionList)
    {
        WSGO("Bad expression type (%d) for action list value\n", value->op);
        ACTION("Ignoring actions for group %d of %s\n", ndx,
                longText(key->name, XkbMessage));
        return False;
    }
    if (key->acts[ndx] != NULL)
    {
        WSGO("Actions for key %s, group %d already defined\n",
              longText(key->name, XkbMessage), ndx);
        return False;
    }
    for (nActs = 0, act = value->value.child; act != NULL; nActs++)
    {
        act = (ExprDef *) act->common.next;
    }
    if (nActs < 1)
    {
        WSGO("Action list but not actions in AddActionsToKey\n");
        return False;
    }
    if (((key->numLevels[ndx] < nActs) || (key->acts[ndx] == NULL)) &&
        (!ResizeKeyGroup(key, ndx, nActs, True)))
    {
        WSGO("Could not resize group %d of key %s\n", ndx,
              longText(key->name, XkbMessage));
        ACTION("Actions lost\n");
        return False;
    }
    key->actsDefined |= (1U << ndx);

    toAct = (XkbAnyAction *) key->acts[ndx];
    act = value->value.child;
    for (int i = 0; i < nActs; i++, toAct++)
    {
        if (!HandleActionDef(act, xkb, toAct, MergeOverride, info->action))
        {
            ERROR("Illegal action definition for %s\n",
                   longText(key->name, XkbMessage));
            ACTION("Action for group %d/level %d ignored\n", ndx + 1, i + 1);
        }
        act = (ExprDef *) act->common.next;
    }
    return True;
}

static int
SetAllowNone(KeyInfo *key, const ExprDef *arrayNdx, const ExprDef *value)
{
    ExprResult tmp;
    unsigned radio_groups = 0;

    if (arrayNdx == NULL)
    {
        radio_groups = XkbAllRadioGroupsMask;
    }
    else
    {
        if (!ExprResolveInteger(arrayNdx, &tmp, RadioLookup, NULL))
        {
            ERROR("Illegal index in group name definition\n");
            ACTION("Definition with non-integer array index ignored\n");
            return False;
        }
        if ((tmp.uval < 1) || (tmp.uval > XkbMaxRadioGroups))
        {
            ERROR("Illegal radio group specified (must be 1..%d)\n",
                   XkbMaxRadioGroups + 1);
            ACTION("Value of \"allow none\" for group %d ignored\n",
                    tmp.uval);
            return False;
        }
        radio_groups |= (1U << (tmp.uval - 1));
    }
    if (!ExprResolveBoolean(value, &tmp, NULL, NULL))
    {
        ERROR("Illegal \"allow none\" value for %s\n",
               longText(key->name, XkbMessage));
        ACTION("Non-boolean value ignored\n");
        return False;
    }
    if (tmp.uval)
        key->allowNone |= radio_groups;
    else
        key->allowNone &= ~radio_groups;
    return True;
}


static LookupEntry lockingEntries[] = {
    {"true", XkbKB_Lock},
    {"yes", XkbKB_Lock},
    {"on", XkbKB_Lock},
    {"false", XkbKB_Default},
    {"no", XkbKB_Default},
    {"off", XkbKB_Default},
    {"permanent", XkbKB_Lock | XkbKB_Permanent},
    {NULL, 0}
};

static LookupEntry repeatEntries[] = {
    {"true", RepeatYes},
    {"yes", RepeatYes},
    {"on", RepeatYes},
    {"false", RepeatNo},
    {"no", RepeatNo},
    {"off", RepeatNo},
    {"default", RepeatUndefined},
    {NULL, 0}
};

static LookupEntry rgEntries[] = {
    {"none", 0},
    {NULL, 0}
};

static Bool
SetSymbolsField(KeyInfo *key, XkbDescPtr xkb, const char *field,
                const ExprDef *arrayNdx, const ExprDef *value,
                SymbolsInfo *info)
{
    Bool ok = True;
    ExprResult tmp;

    if (uStrCaseCmp(field, "type") == 0)
    {
        ExprResult ndx;
        if ((!ExprResolveString(value, &tmp, NULL, NULL))
            && (warningLevel > 0))
        {
            WARN("The type field of a key symbol map must be a string\n");
            ACTION("Ignoring illegal type definition\n");
        }
        if (arrayNdx == NULL)
        {
            key->dfltType = XkbInternAtom(NULL, tmp.str, False);
            key->defs.defined |= _Key_Type_Dflt;
        }
        else if (!ExprResolveInteger(arrayNdx, &ndx, SimpleLookup,
                                     (XPointer) groupNames))
        {
            ERROR("Illegal group index for type of key %s\n",
                   longText(key->name, XkbMessage));
            ACTION("Definition with non-integer array index ignored\n");
            return False;
        }
        else if ((ndx.uval < 1) || (ndx.uval > XkbNumKbdGroups))
        {
            ERROR
                ("Group index for type of key %s is out of range (1..%d)\n",
                 longText(key->name, XkbMessage), XkbNumKbdGroups + 1);
            ACTION("Ignoring type for group %d\n", ndx.uval);
            return False;
        }
        else
        {
            key->types[ndx.uval - 1] = XkbInternAtom(NULL, tmp.str, False);
            key->typesDefined |= (1U << (ndx.uval - 1));
        }
    }
    else if (uStrCaseCmp(field, "symbols") == 0)
        return AddSymbolsToKey(key, xkb, field, arrayNdx, value, info);
    else if (uStrCaseCmp(field, "actions") == 0)
        return AddActionsToKey(key, xkb, field, arrayNdx, value, info);
    else if ((uStrCaseCmp(field, "vmods") == 0) ||
             (uStrCaseCmp(field, "virtualmods") == 0) ||
             (uStrCaseCmp(field, "virtualmodifiers") == 0))
    {
        ok = ExprResolveModMask(value, &tmp, LookupVModMask, (XPointer) xkb);
        if (ok)
        {
            key->vmodmap = (tmp.uval >> 8);
            key->defs.defined |= _Key_VModMap;
        }
        else
        {
            ERROR("Expected a virtual modifier mask, found %s\n",
                   exprOpText(value->op));
            ACTION("Ignoring virtual modifiers definition for key %s\n",
                    longText(key->name, XkbMessage));
        }
    }
    else if ((uStrCaseCmp(field, "locking") == 0)
             || (uStrCaseCmp(field, "lock") == 0)
             || (uStrCaseCmp(field, "locks") == 0))
    {
        ok = ExprResolveEnum(value, &tmp, lockingEntries);
        if (ok)
            key->behavior.type = tmp.uval;
        key->defs.defined |= _Key_Behavior;
    }
    else if ((uStrCaseCmp(field, "radiogroup") == 0) ||
             (uStrCaseCmp(field, "permanentradiogroup") == 0))
    {
        Bool permanent = False;
        if (uStrCaseCmp(field, "permanentradiogroup") == 0)
            permanent = True;
        ok = ExprResolveInteger(value, &tmp, SimpleLookup,
                                (XPointer) rgEntries);
        if (!ok)
        {
            ERROR("Illegal radio group specification for %s\n",
                   longText(key->name, XkbMessage));
            ACTION("Non-integer radio group ignored\n");
            return False;
        }
        if (tmp.uval == 0)
        {
            key->behavior.type = XkbKB_Default;
            key->behavior.data = 0;
            return ok;
        }
        if ((tmp.uval < 1) || (tmp.uval > XkbMaxRadioGroups))
        {
            ERROR
                ("Radio group specification for %s out of range (1..32)\n",
                 longText(key->name, XkbMessage));
            ACTION("Illegal radio group %d ignored\n", tmp.uval);
            return False;
        }
        key->behavior.type =
            XkbKB_RadioGroup | (permanent ? XkbKB_Permanent : 0);
        key->behavior.data = tmp.uval - 1;
        if (key->allowNone & (1U << (tmp.uval - 1)))
            key->behavior.data |= XkbKB_RGAllowNone;
        key->defs.defined |= _Key_Behavior;
    }
    else if (uStrCaseEqual(field, "allownone"))
    {
        ok = SetAllowNone(key, arrayNdx, value);
    }
    else if (uStrCasePrefix("overlay", field) ||
             uStrCasePrefix("permanentoverlay", field))
    {
        Bool permanent = False;
        const char *which;
        int overlayNdx;
        if (uStrCasePrefix("permanent", field))
        {
            permanent = True;
            which = &field[sizeof("permanentoverlay") - 1];
        }
        else
        {
            which = &field[sizeof("overlay") - 1];
        }
        if (sscanf(which, "%d", &overlayNdx) == 1)
        {
            if (((overlayNdx < 1) || (overlayNdx > 2)) && (warningLevel > 0))
            {
                ERROR("Illegal overlay %d specified for %s\n",
                       overlayNdx, longText(key->name, XkbMessage));
                ACTION("Ignored\n");
                return False;
            }
        }
        else if (*which == '\0')
            overlayNdx = 1;
        else if (warningLevel > 0)
        {
            ERROR("Illegal overlay \"%s\" specified for %s\n",
                   which, longText(key->name, XkbMessage));
            ACTION("Ignored\n");
            return False;
        }
        ok = ExprResolveKeyName(value, &tmp, NULL, NULL);
        if (!ok)
        {
            ERROR("Illegal overlay key specification for %s\n",
                   longText(key->name, XkbMessage));
            ACTION("Overlay key must be specified by name\n");
            return False;
        }
        if (overlayNdx == 1)
            key->behavior.type = XkbKB_Overlay1;
        else
            key->behavior.type = XkbKB_Overlay2;
        if (permanent)
            key->behavior.type |= XkbKB_Permanent;

        key->behavior.data = 0;
        key->nameForOverlayKey = KeyNameToLong(tmp.keyName.name);
        key->defs.defined |= _Key_Behavior;
    }
    else if ((uStrCaseCmp(field, "repeating") == 0) ||
             (uStrCaseCmp(field, "repeats") == 0) ||
             (uStrCaseCmp(field, "repeat") == 0))
    {
        ok = ExprResolveEnum(value, &tmp, repeatEntries);
        if (!ok)
        {
            ERROR("Illegal repeat setting for %s\n",
                   longText(key->name, XkbMessage));
            ACTION("Non-boolean repeat setting ignored\n");
            return False;
        }
        key->repeat = tmp.uval;
        key->defs.defined |= _Key_Repeat;
    }
    else if ((uStrCaseCmp(field, "groupswrap") == 0) ||
             (uStrCaseCmp(field, "wrapgroups") == 0))
    {
        ok = ExprResolveBoolean(value, &tmp, NULL, NULL);
        if (!ok)
        {
            ERROR("Illegal groupsWrap setting for %s\n",
                   longText(key->name, XkbMessage));
            ACTION("Non-boolean value ignored\n");
            return False;
        }
        if (tmp.uval)
            key->groupInfo = XkbWrapIntoRange;
        else
            key->groupInfo = XkbClampIntoRange;
        key->defs.defined |= _Key_GroupInfo;
    }
    else if ((uStrCaseCmp(field, "groupsclamp") == 0) ||
             (uStrCaseCmp(field, "clampgroups") == 0))
    {
        ok = ExprResolveBoolean(value, &tmp, NULL, NULL);
        if (!ok)
        {
            ERROR("Illegal groupsClamp setting for %s\n",
                   longText(key->name, XkbMessage));
            ACTION("Non-boolean value ignored\n");
            return False;
        }
        if (tmp.uval)
            key->groupInfo = XkbClampIntoRange;
        else
            key->groupInfo = XkbWrapIntoRange;
        key->defs.defined |= _Key_GroupInfo;
    }
    else if ((uStrCaseCmp(field, "groupsredirect") == 0) ||
             (uStrCaseCmp(field, "redirectgroups") == 0))
    {
        if (!ExprResolveInteger
            (value, &tmp, SimpleLookup, (XPointer) groupNames))
        {
            ERROR("Illegal group index for redirect of key %s\n",
                   longText(key->name, XkbMessage));
            ACTION("Definition with non-integer group ignored\n");
            return False;
        }
        if ((tmp.uval < 1) || (tmp.uval > XkbNumKbdGroups))
        {
            ERROR("Out-of-range (1..%d) group for redirect of key %s\n",
                   XkbNumKbdGroups, longText(key->name, XkbMessage));
            ERROR("Ignoring illegal group %d\n", tmp.uval);
            return False;
        }
        key->groupInfo =
            XkbSetGroupInfo(0, XkbRedirectIntoRange, tmp.uval - 1);
        key->defs.defined |= _Key_GroupInfo;
    }
    else
    {
        ERROR("Unknown field %s in a symbol interpretation\n", field);
        ACTION("Definition ignored\n");
        ok = False;
    }
    return ok;
}

static int
SetGroupName(SymbolsInfo *info, const ExprDef *arrayNdx,const  ExprDef *value)
{
    ExprResult tmp, name;

    if ((arrayNdx == NULL) && (warningLevel > 0))
    {
        WARN("You must specify an index when specifying a group name\n");
        ACTION("Group name definition without array subscript ignored\n");
        return False;
    }
    if (!ExprResolveInteger
        (arrayNdx, &tmp, SimpleLookup, (XPointer) groupNames))
    {
        ERROR("Illegal index in group name definition\n");
        ACTION("Definition with non-integer array index ignored\n");
        return False;
    }
    if ((tmp.uval < 1) || (tmp.uval > XkbNumKbdGroups))
    {
        ERROR
            ("Attempt to specify name for illegal group (must be 1..%d)\n",
             XkbNumKbdGroups + 1);
        ACTION("Name for group %d ignored\n", tmp.uval);
        return False;
    }
    if (!ExprResolveString(value, &name, NULL, NULL))
    {
        ERROR("Group name must be a string\n");
        ACTION("Illegal name for group %d ignored\n", tmp.uval);
        return False;
    }
    info->groupNames[tmp.uval - 1 + info->explicit_group] =
        XkbInternAtom(NULL, name.str, False);

    return True;
}

static int
HandleSymbolsVar(VarDef * stmt, XkbDescPtr xkb, SymbolsInfo * info)
{
    ExprResult elem, field, tmp;
    ExprDef *arrayNdx;

    if (ExprResolveLhs(stmt->name, &elem, &field, &arrayNdx) == 0)
        return 0;               /* internal error, already reported */
    if (elem.str && (uStrCaseCmp(elem.str, "key") == 0))
    {
        return SetSymbolsField(&info->dflt, xkb, field.str, arrayNdx,
                               stmt->value, info);
    }
    else if ((elem.str == NULL) && ((uStrCaseCmp(field.str, "name") == 0) ||
                                    (uStrCaseCmp(field.str, "groupname") ==
                                     0)))
    {
        return SetGroupName(info, arrayNdx, stmt->value);
    }
    else if ((elem.str == NULL)
             && ((uStrCaseCmp(field.str, "groupswrap") == 0)
                 || (uStrCaseCmp(field.str, "wrapgroups") == 0)))
    {
        if (!ExprResolveBoolean(stmt->value, &tmp, NULL, NULL))
        {
            ERROR("Illegal setting for global groupsWrap\n");
            ACTION("Non-boolean value ignored\n");
            return False;
        }
        if (tmp.uval)
            info->groupInfo = XkbWrapIntoRange;
        else
            info->groupInfo = XkbClampIntoRange;
        return True;
    }
    else if ((elem.str == NULL)
             && ((uStrCaseCmp(field.str, "groupsclamp") == 0)
                 || (uStrCaseCmp(field.str, "clampgroups") == 0)))
    {
        if (!ExprResolveBoolean(stmt->value, &tmp, NULL, NULL))
        {
            ERROR("Illegal setting for global groupsClamp\n");
            ACTION("Non-boolean value ignored\n");
            return False;
        }
        if (tmp.uval)
            info->groupInfo = XkbClampIntoRange;
        else
            info->groupInfo = XkbWrapIntoRange;
        return True;
    }
    else if ((elem.str == NULL)
             && ((uStrCaseCmp(field.str, "groupsredirect") == 0)
                 || (uStrCaseCmp(field.str, "redirectgroups") == 0)))
    {
        if (!ExprResolveInteger(stmt->value, &tmp,
                                SimpleLookup, (XPointer) groupNames))
        {
            ERROR("Illegal group index for global groupsRedirect\n");
            ACTION("Definition with non-integer group ignored\n");
            return False;
        }
        if ((tmp.uval < 1) || (tmp.uval > XkbNumKbdGroups))
        {
            ERROR
                ("Out-of-range (1..%d) group for global groupsRedirect\n",
                 XkbNumKbdGroups);
            ACTION("Ignoring illegal group %d\n", tmp.uval);
            return False;
        }
        info->groupInfo = XkbSetGroupInfo(0, XkbRedirectIntoRange, tmp.uval);
        return True;
    }
    else if ((elem.str == NULL) && (uStrCaseCmp(field.str, "allownone") == 0))
    {
        return SetAllowNone(&info->dflt, arrayNdx, stmt->value);
    }
    return SetActionField(xkb, elem.str, field.str, arrayNdx, stmt->value,
                          &info->action);
}

static Bool
HandleSymbolsBody(VarDef * def,
                  XkbDescPtr xkb, KeyInfo * key, SymbolsInfo * info)
{
    Bool ok = True;

    for (; def != NULL; def = (VarDef *) def->common.next)
    {
        if ((def->name) && (def->name->type == ExprFieldRef))
        {
            ok = HandleSymbolsVar(def, xkb, info);
            continue;
        }
        else
        {
            ExprResult tmp, field;
            ExprDef *arrayNdx;

            if (def->name == NULL)
            {
                if ((def->value == NULL)
                    || (def->value->op == ExprKeysymList))
                    field.str = "symbols";
                else
                    field.str = "actions";
                arrayNdx = NULL;
            }
            else
            {
                ok = ExprResolveLhs(def->name, &tmp, &field, &arrayNdx);
            }
            if (ok)
                ok = SetSymbolsField(key, xkb, field.str, arrayNdx,
                                     def->value, info);
        }
    }
    return ok;
}

static Bool
SetExplicitGroup(SymbolsInfo * info, KeyInfo * key)
{
    unsigned group = info->explicit_group;

    if (group == 0)
        return True;

    if ((key->typesDefined | key->symsDefined | key->actsDefined) & ~1)
    {
        if (warningLevel > 0)
        {
            WARN("For map %s an explicit group is specified\n", info->name);
            WARN("but key %s has more than one group defined\n",
                  longText(key->name, XkbMessage));
            ACTION("All groups except first one will be ignored\n");
        }
        for (int i = 1; i < XkbNumKbdGroups; i++)
        {
            key->numLevels[i] = 0;
            free(key->syms[i]);
            key->syms[i] = (KeySym *) NULL;
            free(key->acts[i]);
            key->acts[i] = (XkbAction *) NULL;
            key->types[i] = (Atom) 0;
        }
    }
    key->typesDefined = key->symsDefined = key->actsDefined = 1U << group;

    key->numLevels[group] = key->numLevels[0];
    key->numLevels[0] = 0;
    key->syms[group] = key->syms[0];
    key->syms[0] = (KeySym *) NULL;
    key->acts[group] = key->acts[0];
    key->acts[0] = (XkbAction *) NULL;
    key->types[group] = key->types[0];
    key->types[0] = (Atom) 0;
    return True;
}

static int
HandleSymbolsDef(SymbolsDef * stmt,
                 XkbDescPtr xkb, unsigned merge, SymbolsInfo * info)
{
    KeyInfo key;

    InitKeyInfo(&key);
    CopyKeyInfo(&info->dflt, &key, False);
    key.defs.merge = stmt->merge;
    key.name = KeyNameToLong(stmt->keyName);
    if (!HandleSymbolsBody((VarDef *) stmt->symbols, xkb, &key, info))
    {
        info->errorCount++;
        return False;
    }

    if (!SetExplicitGroup(info, &key))
    {
        info->errorCount++;
        return False;
    }

    if (!AddKeySymbols(info, &key, xkb))
    {
        info->errorCount++;
        return False;
    }
    return True;
}

static Bool
HandleModMapDef(ModMapDef * def,
                XkbDescPtr xkb, unsigned merge, SymbolsInfo * info)
{
    ModMapEntry tmp;
    ExprResult rtrn;
    Bool ok;

    if (!LookupModIndex(NULL, None, def->modifier, TypeInt, &rtrn))
    {
        ERROR("Illegal modifier map definition\n");
        ACTION("Ignoring map for non-modifier \"%s\"\n",
                XkbAtomText(NULL, def->modifier, XkbMessage));
        return False;
    }
    ok = True;
    tmp.modifier = rtrn.uval;
    for (ExprDef *key = def->keys; key != NULL;
         key = (ExprDef *) key->common.next)
    {
        if ((key->op == ExprValue) && (key->type == TypeKeyName))
        {
            tmp.haveSymbol = False;
            tmp.u.keyName = KeyNameToLong(key->value.keyName);
        }
        else if (ExprResolveKeySym(key, &rtrn, NULL, NULL))
        {
            tmp.haveSymbol = True;
            tmp.u.keySym = rtrn.uval;
        }
        else
        {
            ERROR("Modmap entries may contain only key names or keysyms\n");
            ACTION("Illegal definition for %s modifier ignored\n",
                    XkbModIndexText(tmp.modifier, XkbMessage));
            continue;
        }

        ok = AddModMapEntry(info, &tmp) && ok;
    }
    return ok;
}

static void
HandleSymbolsFile(XkbFile * file,
                  XkbDescPtr xkb, unsigned merge, SymbolsInfo * info)
{
    ParseCommon *stmt;

    info->name = uStringDup(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->stmtType)
        {
        case StmtInclude:
            if (!HandleIncludeSymbols((IncludeStmt *) stmt, xkb, info,
                                      HandleSymbolsFile))
                info->errorCount++;
            break;
        case StmtSymbolsDef:
            if (!HandleSymbolsDef((SymbolsDef *) stmt, xkb, merge, info))
                info->errorCount++;
            break;
        case StmtVarDef:
            if (!HandleSymbolsVar((VarDef *) stmt, xkb, info))
                info->errorCount++;
            break;
        case StmtVModDef:
            if (!HandleVModDef((VModDef *) stmt, merge, &info->vmods))
                info->errorCount++;
            break;
        case StmtInterpDef:
            ERROR("Interpretation files may not include other types\n");
            ACTION("Ignoring definition of symbol interpretation\n");
            info->errorCount++;
            break;
        case StmtKeycodeDef:
            ERROR("Interpretation files may not include other types\n");
            ACTION("Ignoring definition of key name\n");
            info->errorCount++;
            break;
        case StmtModMapDef:
            if (!HandleModMapDef((ModMapDef *) stmt, xkb, merge, info))
                info->errorCount++;
            break;
        default:
            WSGO("Unexpected statement type %d in HandleSymbolsFile\n",
                  stmt->stmtType);
            break;
        }
        stmt = stmt->next;
        if (info->errorCount > 10)
        {
#ifdef NOISY
            ERROR("Too many errors\n");
#endif
            ACTION("Abandoning symbols file \"%s\"\n", file->topName);
            break;
        }
    }
    return;
}

static Bool
FindKeyForSymbol(XkbDescPtr xkb, KeySym sym, unsigned int *kc_rtrn)
{
    int j;
    Bool gotOne;

    j = 0;
    do
    {
        gotOne = False;
        for (int i = xkb->min_key_code; i <= (int) xkb->max_key_code; i++)
        {
            if (j < (int) XkbKeyNumSyms(xkb, i))
            {
                gotOne = True;
                if (XkbKeySym(xkb, i, j) == sym)
                {
                    *kc_rtrn = i;
                    return True;
                }
            }
        }
        j++;
    }
    while (gotOne);
    return False;
}

/**
 * Find the given name in the xkb->map->types and return its index.
 *
 * @param name The atom to search for.
 * @param type_rtrn Set to the index of the name if found.
 *
 * @return True if found, False otherwise.
 */
static Bool
FindNamedType(const XkbDescPtr xkb, Atom name, unsigned *type_rtrn)
{
    if (xkb && xkb->map && xkb->map->types)
    {
        for (unsigned n = 0; n < xkb->map->num_types; n++)
        {
            if (xkb->map->types[n].name == (Atom) name)
            {
                *type_rtrn = n;
                return True;
            }
        }
    }
    return False;
}

static Bool
KSIsLower(KeySym ks)
{
    KeySym lower, upper;
    XConvertCase(ks, &lower, &upper);

    if (lower == upper)
        return False;
    return (ks == lower ? True : False);
}

static Bool
KSIsUpper(KeySym ks)
{
    KeySym lower, upper;
    XConvertCase(ks, &lower, &upper);

    if (lower == upper)
        return False;
    return (ks == upper ? True : False);
}

/**
 * Assign a type to the given sym and return the Atom for the type assigned.
 *
 * Simple recipe:
 * - ONE_LEVEL for width 0/1
 * - ALPHABETIC for 2 shift levels, with lower/uppercase
 * - KEYPAD for keypad keys.
 * - TWO_LEVEL for other 2 shift level keys.
 * and the same for four level keys.
 *
 * @param width Number of syms in syms.
 * @param syms The keysyms for the given key (must be size width).
 * @param typeNameRtrn Set to the Atom of the type name.
 *
 * @returns True if a type could be found, False otherwise.
 */
static Bool
FindAutomaticType(int width, const KeySym *syms,
                  Atom *typeNameRtrn, Bool *autoType)
{
    *autoType = False;
    if ((width == 1) || (width == 0))
    {
        *typeNameRtrn = XkbInternAtom(NULL, "ONE_LEVEL", False);
        *autoType = True;
    }
    else if (width == 2)
    {
        if (syms && KSIsLower(syms[0]) && KSIsUpper(syms[1]))
        {
            *typeNameRtrn = XkbInternAtom(NULL, "ALPHABETIC", False);
        }
        else if (syms && (XkbKSIsKeypad(syms[0]) || XkbKSIsKeypad(syms[1])))
        {
            *typeNameRtrn = XkbInternAtom(NULL, "KEYPAD", False);
            *autoType = True;
        }
        else
        {
            *typeNameRtrn = XkbInternAtom(NULL, "TWO_LEVEL", False);
            *autoType = True;
        }
    }
    else if (width <= 4)
    {
        if (syms && KSIsLower(syms[0]) && KSIsUpper(syms[1]))
            if (KSIsLower(syms[2]) && KSIsUpper(syms[3]))
                *typeNameRtrn =
                    XkbInternAtom(NULL, "FOUR_LEVEL_ALPHABETIC", False);
            else
                *typeNameRtrn = XkbInternAtom(NULL,
                                              "FOUR_LEVEL_SEMIALPHABETIC",
                                              False);

        else if (syms && (XkbKSIsKeypad(syms[0]) || XkbKSIsKeypad(syms[1])))
            *typeNameRtrn = XkbInternAtom(NULL, "FOUR_LEVEL_KEYPAD", False);
        else
            *typeNameRtrn = XkbInternAtom(NULL, "FOUR_LEVEL", False);
        /* XXX: why not set autoType here? */
    }
    return ((width >= 0) && (width <= 4));
}

/**
 * Ensure the given KeyInfo is in a coherent state, i.e. no gaps between the
 * groups, and reduce to one group if all groups are identical anyway.
 */
static void
PrepareKeyDef(KeyInfo * key)
{
    int i, defined, lastGroup;
    Bool identical;

    defined = key->symsDefined | key->actsDefined | key->typesDefined;
    /* get highest group number */
    for (i = XkbNumKbdGroups - 1; i >= 0; i--)
    {
        if (defined & (1U << i))
            break;
    }
    lastGroup = i;

    if (lastGroup == 0)
        return;

    /* If there are empty groups between non-empty ones fill them with data */
    /* from the first group. */
    /* We can make a wrong assumption here. But leaving gaps is worse. */
    for (i = lastGroup; i > 0; i--)
    {
        int width;

        if (defined & (1U << i))
            continue;
        width = key->numLevels[0];
        if (key->typesDefined & 1)
        {
            for (int j = 0; j < width; j++)
            {
                key->types[i] = key->types[0];
            }
            key->typesDefined |= 1U << i;
        }
        if ((key->actsDefined & 1) && key->acts[0])
        {
            key->acts[i] = calloc(width, sizeof(XkbAction));
            if (key->acts[i] == NULL)
                continue;
            memcpy(key->acts[i], key->acts[0], width * sizeof(XkbAction));
            key->actsDefined |= 1U << i;
        }
        if ((key->symsDefined & 1) && key->syms[0])
        {
            key->syms[i] = calloc(width, sizeof(KeySym));
            if (key->syms[i] == NULL)
                continue;
            memcpy(key->syms[i], key->syms[0], width * sizeof(KeySym));
            key->symsDefined |= 1U << i;
        }
        if (defined & 1)
        {
            key->numLevels[i] = key->numLevels[0];
        }
    }
    /* If all groups are completely identical remove them all */
    /* except the first one. */
    identical = True;
    for (i = lastGroup; i > 0; i--)
    {
        if ((key->numLevels[i] != key->numLevels[0]) ||
            (key->types[i] != key->types[0]))
        {
            identical = False;
            break;
        }
        if ((key->syms[i] != key->syms[0]) &&
            (key->syms[i] == NULL || key->syms[0] == NULL ||
             memcmp((void *) key->syms[i], (void *) key->syms[0],
                    sizeof(KeySym) * key->numLevels[0])))
        {
            identical = False;
            break;
        }
        if ((key->acts[i] != key->acts[0]) &&
            (key->acts[i] == NULL || key->acts[0] == NULL ||
             memcmp((void *) key->acts[i], (void *) key->acts[0],
                    sizeof(XkbAction) * key->numLevels[0])))
        {
            identical = False;
            break;
        }
    }
    if (identical)
    {
        for (i = lastGroup; i > 0; i--)
        {
            key->numLevels[i] = 0;
            free(key->syms[i]);
            key->syms[i] = (KeySym *) NULL;
            free(key->acts[i]);
            key->acts[i] = (XkbAction *) NULL;
            key->types[i] = (Atom) 0;
        }
        key->symsDefined &= 1;
        key->actsDefined &= 1;
        key->typesDefined &= 1;
    }
    return;
}

/**
 * Copy the KeyInfo into result.
 *
 * This function recurses.
 */
static Bool
CopySymbolsDef(XkbFileInfo * result, KeyInfo * key, int start_from)
{
    int i;
    unsigned kc, width, nGroups;
    XkbKeyTypePtr type;
    Bool haveActions, autoType, useAlias;
    KeySym *outSyms;
    XkbAction *outActs;
    XkbDescPtr xkb;
    unsigned types[XkbNumKbdGroups];

    xkb = result->xkb;
    useAlias = (start_from == 0);

    /* get the keycode for the key. */
    if (!FindNamedKey(xkb, key->name, &kc, useAlias, CreateKeyNames(xkb),
                      start_from))
    {
        if ((start_from == 0) && (warningLevel >= 5))
        {
            WARN("Key %s not found in %s keycodes\n",
                  longText(key->name, XkbMessage),
                  XkbAtomText(NULL, xkb->names->keycodes, XkbMessage));
            ACTION("Symbols ignored\n");
        }
        return False;
    }

    haveActions = False;
    for (i = width = nGroups = 0; i < XkbNumKbdGroups; i++)
    {
        if (((i + 1) > nGroups)
            && (((key->symsDefined | key->actsDefined) & (1U << i))
                || (key->typesDefined) & (1U << i)))
            nGroups = i + 1;
        if (key->acts[i])
            haveActions = True;
        autoType = False;
        /* Assign the type to the key, if it is missing. */
        if (key->types[i] == None)
        {
            if (key->dfltType != None)
                key->types[i] = key->dfltType;
            else if (FindAutomaticType(key->numLevels[i], key->syms[i],
                                       &key->types[i], &autoType))
            {
            }
            else
            {
                if (warningLevel >= 5)
                {
                    WARN("No automatic type for %d symbols\n",
                          (unsigned int) key->numLevels[i]);
                    ACTION("Using %s for the %s key (keycode %d)\n",
                            XkbAtomText(NULL, key->types[i],
                                        XkbMessage),
                            longText(key->name, XkbMessage), kc);
                }
            }
        }
        if (FindNamedType(xkb, key->types[i], &types[i]))
        {
            if (!autoType || key->numLevels[i] > 2)
                xkb->server->explicit[kc] |= (1U << i);
        }
        else
        {
            if (warningLevel >= 3)
            {
                WARN("Type \"%s\" is not defined\n",
                      XkbAtomText(NULL, key->types[i], XkbMessage));
                ACTION("Using TWO_LEVEL for the %s key (keycode %d)\n",
                        longText(key->name, XkbMessage), kc);
            }
            types[i] = XkbTwoLevelIndex;
        }
        /* if the type specifies less syms than the key has, shrink the key */
        type = &xkb->map->types[types[i]];
        if (type->num_levels < key->numLevels[i])
        {
            if (warningLevel > 5)
            {
                WARN
                    ("Type \"%s\" has %d levels, but %s has %d symbols\n",
                     XkbAtomText(NULL, type->name, XkbMessage),
                     (unsigned int) type->num_levels,
                     longText(key->name, XkbMessage),
                     (unsigned int) key->numLevels[i]);
                ACTION("Ignoring extra symbols\n");
            }
            key->numLevels[i] = type->num_levels;
        }
        if (key->numLevels[i] > width)
            width = key->numLevels[i];
        if (type->num_levels > width)
            width = type->num_levels;
    }

    /* width is now the largest width found */

    i = width * nGroups;
    outSyms = XkbResizeKeySyms(xkb, kc, i);
    if (outSyms == NULL)
    {
        WSGO("Could not enlarge symbols for %s (keycode %d)\n",
              longText(key->name, XkbMessage), kc);
        return False;
    }
    if (haveActions)
    {
        outActs = XkbResizeKeyActions(xkb, kc, i);
        if (outActs == NULL)
        {
            WSGO("Could not enlarge actions for %s (key %d)\n",
                  longText(key->name, XkbMessage), kc);
            return False;
        }
        xkb->server->explicit[kc] |= XkbExplicitInterpretMask;
    }
    else
        outActs = NULL;
    if (key->defs.defined & _Key_GroupInfo)
        i = key->groupInfo;
    else
        i = xkb->map->key_sym_map[kc].group_info;

    xkb->map->key_sym_map[kc].group_info = XkbSetNumGroups(i, nGroups);
    xkb->map->key_sym_map[kc].width = width;
    for (i = 0; i < nGroups; i++)
    {
        /* assign kt_index[i] to the index of the type in map->types.
         * kt_index[i] may have been set by a previous run (if we have two
         * layouts specified). Let's not overwrite it with the ONE_LEVEL
         * default group if we don't even have keys for this group anyway.
         *
         * FIXME: There should be a better fix for this.
         */
        if (key->numLevels[i])
            xkb->map->key_sym_map[kc].kt_index[i] = types[i];
        if (key->syms[i] != NULL)
        {
            /* fill key to "width" symbols*/
            for (unsigned tmp = 0; tmp < width; tmp++)
            {
                if (tmp < key->numLevels[i])
                    outSyms[tmp] = key->syms[i][tmp];
                else
                    outSyms[tmp] = NoSymbol;
                if ((outActs != NULL) && (key->acts[i] != NULL))
                {
                    if (tmp < key->numLevels[i])
                        outActs[tmp] = key->acts[i][tmp];
                    else
                        outActs[tmp].type = XkbSA_NoAction;
                }
            }
        }
        outSyms += width;
        if (outActs)
            outActs += width;
    }
    switch (key->behavior.type & XkbKB_OpMask)
    {
    case XkbKB_Default:
        break;
    case XkbKB_Overlay1:
    case XkbKB_Overlay2:
    {
        unsigned okc;
        /* find key by name! */
        if (!FindNamedKey(xkb, key->nameForOverlayKey, &okc, True,
                          CreateKeyNames(xkb), 0))
        {
            if (warningLevel >= 1)
            {
                WARN("Key %s not found in %s keycodes\n",
                      longText(key->nameForOverlayKey, XkbMessage),
                      XkbAtomText(NULL, xkb->names->keycodes, XkbMessage));
                ACTION("Not treating %s as an overlay key \n",
                        longText(key->name, XkbMessage));
            }
            break;
        }
        key->behavior.data = okc;
    }
    default:
        xkb->server->behaviors[kc] = key->behavior;
        xkb->server->explicit[kc] |= XkbExplicitBehaviorMask;
        break;
    }
    if (key->defs.defined & _Key_VModMap)
    {
        xkb->server->vmodmap[kc] = key->vmodmap;
        xkb->server->explicit[kc] |= XkbExplicitVModMapMask;
    }
    if (key->repeat != RepeatUndefined)
    {
        if (key->repeat == RepeatYes)
            xkb->ctrls->per_key_repeat[kc / 8] |= (1U << (kc % 8));
        else
            xkb->ctrls->per_key_repeat[kc / 8] &= ~(1U << (kc % 8));
        xkb->server->explicit[kc] |= XkbExplicitAutoRepeatMask;
    }

    /* do the same thing for the next key */
    CopySymbolsDef(result, key, kc + 1);
    return True;
}

static Bool
CopyModMapDef(XkbFileInfo * result, ModMapEntry * entry)
{
    unsigned kc;
    XkbDescPtr xkb;

    xkb = result->xkb;
    if ((!entry->haveSymbol)
        &&
        (!FindNamedKey
         (xkb, entry->u.keyName, &kc, True, CreateKeyNames(xkb), 0)))
    {
        if (warningLevel >= 5)
        {
            WARN("Key %s not found in %s keycodes\n",
                  longText(entry->u.keyName, XkbMessage),
                  XkbAtomText(NULL, xkb->names->keycodes, XkbMessage));
            ACTION("Modifier map entry for %s not updated\n",
                    XkbModIndexText(entry->modifier, XkbMessage));
        }
        return False;
    }
    else if (entry->haveSymbol
             && (!FindKeyForSymbol(xkb, entry->u.keySym, &kc)))
    {
        if (warningLevel > 5)
        {
            WARN("Key \"%s\" not found in %s symbol map\n",
                  XkbKeysymText(entry->u.keySym, XkbMessage),
                  XkbAtomText(NULL, xkb->names->symbols, XkbMessage));
            ACTION("Modifier map entry for %s not updated\n",
                    XkbModIndexText(entry->modifier, XkbMessage));
        }
        return False;
    }
    xkb->map->modmap[kc] |= (1U << entry->modifier);
    return True;
}

/**
 * Handle the xkb_symbols section of an xkb file.
 *
 * @param file The parsed xkb_symbols section of the xkb file.
 * @param result Handle to the data to store the result in.
 * @param merge Merge strategy (e.g. MergeOverride).
 */
Bool
CompileSymbols(XkbFile * file, XkbFileInfo * result, unsigned merge)
{
    SymbolsInfo info;
    XkbDescPtr xkb;

    xkb = result->xkb;
    InitSymbolsInfo(&info, xkb);
    info.dflt.defs.fileID = file->id;
    info.dflt.defs.merge = merge;
    HandleSymbolsFile(file, xkb, merge, &info);

    if (info.nKeys == 0)
        return True;
    if (info.errorCount == 0)
    {
        int i;
        KeyInfo *key;

        /* alloc memory in the xkb struct */
        if (XkbAllocNames(xkb, XkbSymbolsNameMask | XkbGroupNamesMask, 0, 0)
            != Success)
        {
            WSGO("Can not allocate names in CompileSymbols\n");
            ACTION("Symbols not added\n");
            return False;
        }
        if (XkbAllocClientMap(xkb, XkbKeySymsMask | XkbModifierMapMask, 0)
            != Success)
        {
            WSGO("Could not allocate client map in CompileSymbols\n");
            ACTION("Symbols not added\n");
            return False;
        }
        if (XkbAllocServerMap(xkb, XkbAllServerInfoMask, 32) != Success)
        {
            WSGO("Could not allocate server map in CompileSymbols\n");
            ACTION("Symbols not added\n");
            return False;
        }
        if (XkbAllocControls(xkb, XkbPerKeyRepeatMask) != Success)
        {
            WSGO("Could not allocate controls in CompileSymbols\n");
            ACTION("Symbols not added\n");
            return False;
        }

        /* now copy info into xkb. */
        xkb->names->symbols = XkbInternAtom(xkb->dpy, info.name, False);
        if (info.aliases)
            ApplyAliases(xkb, False, &info.aliases);
        for (i = 0; i < XkbNumKbdGroups; i++)
        {
            if (info.groupNames[i] != None)
                xkb->names->groups[i] = info.groupNames[i];
        }
        /* sanitize keys */
        for (key = info.keys, i = 0; i < info.nKeys; i++, key++)
        {
            PrepareKeyDef(key);
        }
        /* copy! */
        for (key = info.keys, i = 0; i < info.nKeys; i++, key++)
        {
            if (!CopySymbolsDef(result, key, 0))
                info.errorCount++;
        }
        if (warningLevel > 3)
        {
            for (i = xkb->min_key_code; i <= xkb->max_key_code; i++)
            {
                if (xkb->names->keys[i].name[0] == '\0')
                    continue;
                if (XkbKeyNumGroups(xkb, i) < 1)
                {
                    char buf[5];
                    memcpy(buf, xkb->names->keys[i].name, 4);
                    buf[4] = '\0';
                    INFO("No symbols defined for <%s> (keycode %d)\n",
                         buf, i);
                }
            }
        }
        if (info.modMap)
        {
            ModMapEntry *mm, *next;
            for (mm = info.modMap; mm != NULL; mm = next)
            {
                if (!CopyModMapDef(result, mm))
                    info.errorCount++;
                next = (ModMapEntry *) mm->defs.next;
            }
        }
        return True;
    }
    return False;
}
