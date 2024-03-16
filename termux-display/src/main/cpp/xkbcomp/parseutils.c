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

#define	DEBUG_VAR parseDebug
#include "parseutils.h"
#include "xkbpath.h"
#include <X11/keysym.h>
#include <X11/extensions/XKBgeom.h>
#include <limits.h>
#include <stdlib.h>

XkbFile *rtrnValue;

ParseCommon *
AppendStmt(ParseCommon * to, ParseCommon * append)
{
    ParseCommon *start = to;

    if (append == NULL)
        return to;
    while ((to != NULL) && (to->next != NULL))
    {
        to = to->next;
    }
    if (to)
    {
        to->next = append;
        return start;
    }
    return append;
}

ExprDef *
ExprCreate(unsigned op, unsigned type)
{
    ExprDef *expr;
    expr = malloc(sizeof(ExprDef));
    if (expr)
    {
        *expr = (ExprDef) {
            .common.stmtType = StmtExpr,
            .common.next = NULL,
            .op = op,
            .type = type
        };
    }
    else
    {
        FATAL("Couldn't allocate expression in parser\n");
        /* NOTREACHED */
    }
    return expr;
}

ExprDef *
ExprCreateUnary(unsigned op, unsigned type, ExprDef * child)
{
    ExprDef *expr;
    expr = malloc(sizeof(ExprDef));
    if (expr)
    {
        *expr = (ExprDef) {
            .common.stmtType = StmtExpr,
            .common.next = NULL,
            .op = op,
            .type = type,
            .value.child = child
        };
    }
    else
    {
        FATAL("Couldn't allocate expression in parser\n");
        /* NOTREACHED */
    }
    return expr;
}

ExprDef *
ExprCreateBinary(unsigned op, ExprDef * left, ExprDef * right)
{
    ExprDef *expr;
    expr = malloc(sizeof(ExprDef));
    if (expr)
    {
        *expr = (ExprDef) {
            .common.stmtType = StmtExpr,
            .common.next = NULL,
            .op = op,
            .type = TypeUnknown,
            .value.binary.left = left,
            .value.binary.right = right
        };

        if ((op == OpAssign) || (left->type == TypeUnknown))
            expr->type = right->type;
        else if ((left->type == right->type) || (right->type == TypeUnknown))
            expr->type = left->type;
    }
    else
    {
        FATAL("Couldn't allocate expression in parser\n");
        /* NOTREACHED */
    }
    return expr;
}

KeycodeDef *
KeycodeCreate(const char *name, ExprDef *value)
{
    KeycodeDef *def;

    def = malloc(sizeof(KeycodeDef));
    if (def)
    {
        *def = (KeycodeDef) {
            .common.stmtType = StmtKeycodeDef,
            .common.next = NULL,
            .value = value
        };
        strncpy(def->name, name, XkbKeyNameLength);
        def->name[XkbKeyNameLength] = '\0';
    }
    else
    {
        FATAL("Couldn't allocate key name definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

KeyAliasDef *
KeyAliasCreate(const char *alias, const char *real)
{
    KeyAliasDef *def;

    def = malloc(sizeof(KeyAliasDef));
    if (def)
    {
        *def = (KeyAliasDef) {
            .common.stmtType = StmtKeyAliasDef,
            .common.next = NULL,
        };
        strncpy(def->alias, alias, XkbKeyNameLength);
        def->alias[XkbKeyNameLength] = '\0';
        strncpy(def->real, real, XkbKeyNameLength);
        def->real[XkbKeyNameLength] = '\0';
    }
    else
    {
        FATAL("Couldn't allocate key alias definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

VModDef *
VModCreate(Atom name, ExprDef * value)
{
    VModDef *def;
    def = malloc(sizeof(VModDef));
    if (def)
    {
        *def = (VModDef) {
            .common.stmtType = StmtVModDef,
            .common.next = NULL,
            .name = name,
            .value = value
        };
    }
    else
    {
        FATAL("Couldn't allocate variable definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

VarDef *
VarCreate(ExprDef * name, ExprDef * value)
{
    VarDef *def;
    def = malloc(sizeof(VarDef));
    if (def)
    {
        *def = (VarDef) {
            .common.stmtType = StmtVarDef,
            .common.next = NULL,
            .name = name,
            .value = value
        };
    }
    else
    {
        FATAL("Couldn't allocate variable definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

VarDef *
BoolVarCreate(Atom nameToken, unsigned set)
{
    ExprDef *name, *value;

    name = ExprCreate(ExprIdent, TypeUnknown);
    name->value.str = nameToken;
    value = ExprCreate(ExprValue, TypeBoolean);
    value->value.uval = set;
    return VarCreate(name, value);
}

InterpDef *
InterpCreate(const char *sym_str, ExprDef * match)
{
    InterpDef *def;

    def = malloc(sizeof(InterpDef));
    if (def)
    {
        *def = (InterpDef) {
            .common.stmtType = StmtInterpDef,
            .common.next = NULL,
            .match = match
        };
        if (LookupKeysym(sym_str, &def->sym) == 0)
            def->ignore = True;
        else
            def->ignore = False;
    }
    else
    {
        FATAL("Couldn't allocate interp definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

KeyTypeDef *
KeyTypeCreate(Atom name, VarDef * body)
{
    KeyTypeDef *def;

    def = malloc(sizeof(KeyTypeDef));
    if (def)
    {
        *def = (KeyTypeDef) {
            .common.stmtType = StmtKeyTypeDef,
            .common.next = NULL,
            .merge = MergeDefault,
            .name = name,
            .body = body
        };
    }
    else
    {
        FATAL("Couldn't allocate key type definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

SymbolsDef *
SymbolsCreate(char *keyName, ExprDef * symbols)
{
    SymbolsDef *def;

    def = malloc(sizeof(SymbolsDef));
    if (def)
    {
        *def = (SymbolsDef) {
            .common.stmtType = StmtSymbolsDef,
            .common.next = NULL,
            .merge = MergeDefault,
            .symbols = symbols
        };
        strncpy(def->keyName, keyName, 4);
        def->keyName[4] = 0;
    }
    else
    {
        FATAL("Couldn't allocate symbols definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

GroupCompatDef *
GroupCompatCreate(int group, ExprDef * val)
{
    GroupCompatDef *def;

    def = malloc(sizeof(GroupCompatDef));
    if (def)
    {
        *def = (GroupCompatDef) {
            .common.stmtType = StmtGroupCompatDef,
            .common.next = NULL,
            .merge = MergeDefault,
            .group = group,
            .def = val
        };
    }
    else
    {
        FATAL("Couldn't allocate group compat definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

ModMapDef *
ModMapCreate(Atom modifier, ExprDef * keys)
{
    ModMapDef *def;

    def = malloc(sizeof(ModMapDef));
    if (def)
    {
        *def = (ModMapDef) {
            .common.stmtType = StmtModMapDef,
            .common.next = NULL,
            .merge = MergeDefault,
            .modifier = modifier,
            .keys = keys
        };
    }
    else
    {
        FATAL("Couldn't allocate mod mask definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

IndicatorMapDef *
IndicatorMapCreate(Atom name, VarDef * body)
{
    IndicatorMapDef *def;

    def = malloc(sizeof(IndicatorMapDef));
    if (def)
    {
        *def = (IndicatorMapDef) {
            .common.stmtType = StmtIndicatorMapDef,
            .common.next = NULL,
            .merge = MergeDefault,
            .name = name,
            .body = body
        };
    }
    else
    {
        FATAL("Couldn't allocate indicator map definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

IndicatorNameDef *
IndicatorNameCreate(int ndx, ExprDef * name, Bool virtual)
{
    IndicatorNameDef *def;

    def = malloc(sizeof(IndicatorNameDef));
    if (def)
    {
        *def = (IndicatorNameDef) {
            .common.stmtType = StmtIndicatorNameDef,
            .common.next = NULL,
            .merge = MergeDefault,
            .ndx = ndx,
            .name = name,
            .virtual = virtual
        };
    }
    else
    {
        FATAL("Couldn't allocate indicator index definition in parser\n");
        /* NOTREACHED */
    }
    return def;
}

ExprDef *
ActionCreate(Atom name, ExprDef * args)
{
    ExprDef *act;

    act = malloc(sizeof(ExprDef));
    if (act)
    {
        *act = (ExprDef) {
            .common.stmtType = StmtExpr,
            .common.next = NULL,
            .op = ExprActionDecl,
            .value.action.name = name,
            .value.action.args = args
        };
        return act;
    }
    FATAL("Couldn't allocate ActionDef in parser\n");
    return NULL;
}

ExprDef *
CreateKeysymList(char *sym)
{
    ExprDef *def;

    def = ExprCreate(ExprKeysymList, TypeSymbols);
    if (def)
    {
        def->value.list.nSyms = 1;
        def->value.list.szSyms = 4;
        def->value.list.syms = calloc(4, sizeof(char *));
        if (def->value.list.syms != NULL)
        {
            def->value.list.syms[0] = sym;
            return def;
        }
    }
    FATAL("Couldn't allocate expression for keysym list in parser\n");
    return NULL;
}

ShapeDef *
ShapeDeclCreate(Atom name, OutlineDef * outlines)
{
    ShapeDef *shape;

    shape = calloc(1, sizeof(ShapeDef));
    if (shape != NULL)
    {
        *shape = (ShapeDef) {
            .common.stmtType = StmtShapeDef,
            .common.next = NULL,
            .merge = MergeDefault,
            .name = name,
            .nOutlines = 0,
            .outlines = outlines
        };
        for (OutlineDef *ol = outlines; ol != NULL;
             ol = (OutlineDef *) ol->common.next)
        {
            if (ol->nPoints > 0)
                shape->nOutlines++;
        }
    }
    return shape;
}

OutlineDef *
OutlineCreate(Atom field, ExprDef * points)
{
    OutlineDef *outline;

    outline = calloc(1, sizeof(OutlineDef));
    if (outline != NULL)
    {
        *outline = (OutlineDef) {
            .common.stmtType = StmtOutlineDef,
            .common.next = NULL,
            .field = field,
            .nPoints = 0,
            .points = points
        };
        if (points->op == ExprCoord)
        {
            for (ExprDef *pt = points; pt != NULL;
                 pt = (ExprDef *) pt->common.next)
            {
                outline->nPoints++;
            }
        }
    }
    return outline;
}

KeyDef *
KeyDeclCreate(char *name, ExprDef * expr)
{
    KeyDef *key;

    key = calloc(1, sizeof(KeyDef));
    if (key != NULL)
    {
        *key = (KeyDef) {
            .common.stmtType = StmtKeyDef,
            .common.next = NULL,
        };
        if (name)
            key->name = name;
        else
            key->expr = expr;
    }
    return key;
}

#if 0
KeyDef *
KeyDeclMerge(KeyDef * into, KeyDef * from)
{
    into->expr =
        (ExprDef *) AppendStmt(&into->expr->common, &from->expr->common);
    from->expr = NULL;
    free(from);
    return into;
}
#endif

RowDef *
RowDeclCreate(KeyDef * keys)
{
    RowDef *row;

    row = calloc(1, sizeof(RowDef));
    if (row != NULL)
    {
        *row = (RowDef) {
            .common.stmtType = StmtRowDef,
            .common.next = NULL,
            .nKeys = 0,
            .keys = keys
        };
        for (KeyDef *key = keys; key != NULL; key = (KeyDef *) key->common.next)
        {
            if (key->common.stmtType == StmtKeyDef)
                row->nKeys++;
        }
    }
    return row;
}

SectionDef *
SectionDeclCreate(Atom name, RowDef * rows)
{
    SectionDef *section;

    section = calloc(1, sizeof(SectionDef));
    if (section != NULL)
    {
        *section = (SectionDef) {
            .common.stmtType = StmtSectionDef,
            .common.next = NULL,
            .name = name,
            .nRows = 0,
            .rows = rows
        };
        for (RowDef *row = rows; row != NULL; row = (RowDef *) row->common.next)
        {
            if (row->common.stmtType == StmtRowDef)
                section->nRows++;
        }
    }
    return section;
}

OverlayKeyDef *
OverlayKeyCreate(char *under, char *over)
{
    OverlayKeyDef *key;

    key = calloc(1, sizeof(OverlayKeyDef));
    if (key != NULL)
    {
        *key = (OverlayKeyDef) {
            .common.stmtType = StmtOverlayKeyDef
        };
        strncpy(key->over, over, XkbKeyNameLength);
        strncpy(key->under, under, XkbKeyNameLength);
        free(over);
        free(under);
    }
    return key;
}

OverlayDef *
OverlayDeclCreate(Atom name, OverlayKeyDef * keys)
{
    OverlayDef *ol;

    ol = calloc(1, sizeof(OverlayDef));
    if (ol != NULL)
    {
        *ol = (OverlayDef) {
            .common.stmtType = StmtOverlayDef,
            .name = name,
            .keys = keys
        };
        for (OverlayKeyDef *key = keys; key != NULL;
             key = (OverlayKeyDef *) key->common.next)
        {
            ol->nKeys++;
        }
    }
    return ol;
}

DoodadDef *
DoodadCreate(unsigned type, Atom name, VarDef * body)
{
    DoodadDef *doodad;

    doodad = calloc(1, sizeof(DoodadDef));
    if (doodad != NULL)
    {
        *doodad = (DoodadDef) {
            .common.stmtType = StmtDoodadDef,
            .common.next = NULL,
            .type = type,
            .name = name,
            .body = body
        };
    }
    return doodad;
}

ExprDef *
AppendKeysymList(ExprDef * list, char *sym)
{
    if (list->value.list.nSyms >= list->value.list.szSyms)
    {
        list->value.list.szSyms *= 2;
        list->value.list.syms = recallocarray(list->value.list.syms,
                                              list->value.list.nSyms,
                                              list->value.list.szSyms,
                                              sizeof(char *));
        if (list->value.list.syms == NULL)
        {
            FATAL("Couldn't resize list of symbols for append\n");
            return NULL;
        }
    }
    list->value.list.syms[list->value.list.nSyms++] = sym;
    return list;
}

int
LookupKeysym(const char *str, KeySym * sym_rtrn)
{
    KeySym sym;

    if ((!str) || (uStrCaseCmp(str, "any") == 0)
        || (uStrCaseCmp(str, "nosymbol") == 0))
    {
        *sym_rtrn = NoSymbol;
        return 1;
    }
    else if ((uStrCaseCmp(str, "none") == 0)
             || (uStrCaseCmp(str, "voidsymbol") == 0))
    {
        *sym_rtrn = XK_VoidSymbol;
        return 1;
    }
    sym = XStringToKeysym(str);
    if (sym != NoSymbol)
    {
        *sym_rtrn = sym;
        return 1;
    }
    if (strlen(str) > 2 && str[0] == '0' && str[1] == 'x') {
        char *tmp;

        sym = strtoul(str, &tmp, 16);
        if (sym != ULONG_MAX && (!tmp || *tmp == '\0')) {
            *sym_rtrn = sym;
            return 1;
        }
    }
    return 0;
}

IncludeStmt *
IncludeCreate(char *str, unsigned merge)
{
    IncludeStmt *incl, *first;
    char *file, *map, *stmt, *tmp, *extra_data;
    char nextop;
    Bool haveSelf;

    haveSelf = False;
    incl = first = NULL;
    file = map = NULL;
    tmp = str;
    stmt = uStringDup(str);
    while ((tmp) && (*tmp))
    {
        if (XkbParseIncludeMap(&tmp, &file, &map, &nextop, &extra_data))
        {
            if ((file == NULL) && (map == NULL))
            {
                if (haveSelf)
                    goto BAIL;
                haveSelf = True;
            }
            if (first == NULL)
                first = incl = malloc(sizeof(IncludeStmt));
            else
            {
                incl->next = malloc(sizeof(IncludeStmt));
                incl = incl->next;
            }
            if (incl)
            {
                *incl = (IncludeStmt) {
                    .common.stmtType = StmtInclude,
                    .common.next = NULL,
                    .merge = merge,
                    .stmt = NULL,
                    .file = file,
                    .map = map,
                    .modifier = extra_data,
                    .path = NULL,
                    .next = NULL
                };
            }
            else
            {
                WSGO("Allocation failure in IncludeCreate\n");
                ACTION("Using only part of the include\n");
                break;
            }
            if (nextop == '|')
                merge = MergeAugment;
            else
                merge = MergeOverride;
        }
        else
        {
            goto BAIL;
        }
    }
    if (first)
        first->stmt = stmt;
    else if (stmt)
        free(stmt);
    return first;
  BAIL:
    ERROR("Illegal include statement \"%s\"\n", stmt);
    ACTION("Ignored\n");
    while (first)
    {
        incl = first->next;
        free(first->file);
        free(first->map);
        free(first->modifier);
        free(first->path);
        first->file = first->map = first->modifier = first->path = NULL;
        free(first);
        first = incl;
    }
    if (stmt)
        free(stmt);
    return NULL;
}

#ifdef DEBUG
void
PrintStmtAddrs(ParseCommon * stmt)
{
    fprintf(stderr, "%p", stmt);
    if (stmt)
    {
        do
        {
            fprintf(stderr, "->%p", stmt->next);
            stmt = stmt->next;
        }
        while (stmt);
    }
    fprintf(stderr, "\n");
}
#endif

static void
CheckDefaultMap(XkbFile * maps)
{
    XkbFile *dflt = NULL;

    for (XkbFile *tmp = maps; tmp != NULL;
         tmp = (XkbFile *) tmp->common.next)
    {
        if (tmp->flags & XkbLC_Default)
        {
            if (dflt == NULL)
                dflt = tmp;
            else
            {
                if (warningLevel > 2)
                {
                    WARN("Multiple default components in %s\n",
                          (scanFile ? scanFile : "(unknown)"));
                    ACTION("Using %s, ignoring %s\n",
                            (dflt->name ? dflt->name : "(first)"),
                            (tmp->name ? tmp->name : "(subsequent)"));
                }
                tmp->flags &= (~XkbLC_Default);
            }
        }
    }
    return;
}

int
XKBParseFile(FILE * file, XkbFile ** pRtrn)
{
    if (file)
    {
        scan_set_file(file);
        rtrnValue = NULL;
        if (yyparse() == 0)
        {
            *pRtrn = rtrnValue;
            CheckDefaultMap(rtrnValue);
            rtrnValue = NULL;
            return 1;
        }
        *pRtrn = NULL;
        return 0;
    }
    *pRtrn = NULL;
    return 1;
}

XkbFile *
CreateXKBFile(int type, char *name, ParseCommon * defs, unsigned flags)
{
    XkbFile *file;
    static int fileID;

    file = calloc(1, sizeof(XkbFile));
    if (file)
    {
        XkbEnsureSafeMapName(name);
        *file = (XkbFile) {
            .type = type,
            .topName = uStringDup(name),
            .name = name,
            .defs = defs,
            .id = fileID++,
            .compiled = False,
            .flags = flags
        };
    }
    return file;
}

unsigned
StmtSetMerge(ParseCommon * stmt, unsigned merge)
{
    if ((merge == MergeAltForm) && (stmt->stmtType != StmtKeycodeDef))
    {
        yyerror("illegal use of 'alternate' merge mode");
        merge = MergeDefault;
    }
    return merge;
}
