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

#ifndef XKBCOMP_H
#define	XKBCOMP_H 1

#ifndef DEBUG_VAR
#define	DEBUG_VAR debugFlags
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include "utils.h"

#include <X11/extensions/XKM.h>
#include <X11/extensions/XKBfile.h>

extern char *scanFile;

#define	TypeUnknown	0
#define	TypeBoolean	1
#define	TypeInt		2
#define	TypeFloat	3
#define	TypeString	4
#define	TypeAction	5
#define	TypeKeyName	6
#define	TypeSymbols	7

#define	StmtUnknown		0
#define	StmtInclude		1
#define	StmtKeycodeDef		2
#define	StmtKeyAliasDef		3
#define	StmtExpr		4
#define	StmtVarDef		5
#define	StmtKeyTypeDef		6
#define	StmtInterpDef		7
#define	StmtVModDef		8
#define	StmtSymbolsDef		9
#define	StmtModMapDef		10
#define	StmtGroupCompatDef 	11
#define	StmtIndicatorMapDef	12
#define	StmtIndicatorNameDef	13
#define	StmtOutlineDef		14
#define	StmtShapeDef		15
#define	StmtKeyDef		16
#define	StmtRowDef		17
#define	StmtSectionDef		18
#define	StmtOverlayKeyDef	19
#define	StmtOverlayDef		20
#define	StmtDoodadDef		21

#define	FileSymInterp	100

typedef struct _ParseCommon
{
    unsigned stmtType;
    struct _ParseCommon *next;
} ParseCommon;

#define	ExprValue	0
#define	ExprIdent	1
#define	ExprActionDecl	2
#define	ExprFieldRef	3
#define	ExprArrayRef	4
#define	ExprKeysymList	5
#define	ExprActionList	6
#define	ExprCoord	7

#define	OpAdd		20
#define	OpSubtract	21
#define	OpMultiply	22
#define	OpDivide	23
#define	OpAssign	24
#define	OpNot		25
#define	OpNegate	26
#define	OpInvert	27
#define	OpUnaryPlus	28

#define	MergeDefault	0
#define	MergeAugment	1
#define	MergeOverride	2
#define	MergeReplace	3
#define	MergeAltForm	4

#define	AutoKeyNames	(1L <<  0)
#define	CreateKeyNames(x)	((x)->flags&AutoKeyNames)

extern unsigned warningLevel;

typedef struct _IncludeStmt
{
    ParseCommon common;
    unsigned merge;
    char *stmt;
    char *file;
    char *map;
    char *modifier;
    char *path;
    struct _IncludeStmt *next;
} IncludeStmt;

typedef struct _Expr
{
    ParseCommon common;
    unsigned op;
    unsigned type;
    union
    {
        struct
        {
            struct _Expr *left;
            struct _Expr *right;
        } binary;
        struct
        {
            Atom element;
            Atom field;
        } field;
        struct
        {
            Atom element;
            Atom field;
            struct _Expr *entry;
        } array;
        struct
        {
            Atom name;
            struct _Expr *args;
        } action;
        struct
        {
            int nSyms;
            int szSyms;
            char **syms;
        } list;
        struct
        {
            int x;
            int y;
        } coord;
        struct _Expr *child;
        Atom str;
        unsigned uval;
        int ival;
        char keyName[5];
        void *ptr;
    } value;
} ExprDef;

typedef struct _VarDef
{
    ParseCommon common;
    unsigned merge;
    ExprDef *name;
    ExprDef *value;
} VarDef;

typedef struct _VModDef
{
    ParseCommon common;
    unsigned merge;
    Atom name;
    ExprDef *value;
} VModDef;

typedef struct _KeycodeDef
{
    ParseCommon common;
    unsigned merge;
    char name[5];
    ExprDef *value;
} KeycodeDef;

typedef struct _KeyAliasDef
{
    ParseCommon common;
    unsigned merge;
    char alias[5];
    char real[5];
} KeyAliasDef;

typedef struct _KeyTypeDef
{
    ParseCommon common;
    unsigned merge;
    Atom name;
    VarDef *body;
} KeyTypeDef;

typedef struct _SymbolsDef
{
    ParseCommon common;
    unsigned merge;
    char keyName[5];
    ExprDef *symbols;
} SymbolsDef;

typedef struct _ModMapDef
{
    ParseCommon common;
    unsigned merge;
    Atom modifier;
    ExprDef *keys;
} ModMapDef;

typedef struct _GroupCompatDef
{
    ParseCommon common;
    unsigned merge;
    int group;
    ExprDef *def;
} GroupCompatDef;

typedef struct _InterpDef
{
    ParseCommon common;
    unsigned merge;
    KeySym sym;
    ExprDef *match;
    VarDef *def;
    Bool ignore;
} InterpDef;

typedef struct _IndicatorNameDef
{
    ParseCommon common;
    unsigned merge;
    int ndx;
    ExprDef *name;
    Bool virtual;
} IndicatorNameDef;

typedef struct _OutlineDef
{
    ParseCommon common;
    Atom field;
    int nPoints;
    ExprDef *points;
} OutlineDef;

typedef struct _ShapeDef
{
    ParseCommon common;
    unsigned merge;
    Atom name;
    int nOutlines;
    OutlineDef *outlines;
} ShapeDef;

typedef struct _KeyDef
{
    ParseCommon common;
    unsigned defined;
    char *name;
    ExprDef *expr;
} KeyDef;

typedef struct _RowDef
{
    ParseCommon common;
    int nKeys;
    KeyDef *keys;
} RowDef;

typedef struct _SectionDef
{
    ParseCommon common;
    unsigned merge;
    Atom name;
    int nRows;
    RowDef *rows;
} SectionDef;

typedef struct _OverlayKeyDef
{
    ParseCommon common;
    char over[5];
    char under[5];
} OverlayKeyDef;

typedef struct _OverlayDef
{
    ParseCommon common;
    unsigned merge;
    Atom name;
    int nKeys;
    OverlayKeyDef *keys;
} OverlayDef;

typedef struct _DoodadDef
{
    ParseCommon common;
    unsigned merge;
    unsigned type;
    Atom name;
    VarDef *body;
} DoodadDef;

/* IndicatorMapDef doesn't use the type field, but the rest of the fields
   need to be at the same offsets as in DoodadDef.  Use #define to avoid
   any strict aliasing problems.  */
#define IndicatorMapDef DoodadDef

typedef struct _XkbFile
{
    ParseCommon common;
    int type;
    char *topName;
    char *name;
    ParseCommon *defs;
    int id;
    unsigned flags;
    Bool compiled;
} XkbFile;

extern Bool CompileKeymap(XkbFile * /* file */ ,
                          XkbFileInfo * /* result */ ,
                          unsigned      /* merge */
    );

extern Bool CompileKeycodes(XkbFile * /* file */ ,
                            XkbFileInfo * /* result */ ,
                            unsigned    /* merge */
    );

extern Bool CompileGeometry(XkbFile * /* file */ ,
                            XkbFileInfo * /* result */ ,
                            unsigned    /* merge */
    );

extern Bool CompileKeyTypes(XkbFile * /* file */ ,
                            XkbFileInfo * /* result */ ,
                            unsigned    /* merge */
    );

typedef struct _LEDInfo *LEDInfoPtr;

extern Bool CompileCompatMap(const XkbFile * /* file */ ,
                             XkbFileInfo * /* result */ ,
                             unsigned /* merge */ ,
                             LEDInfoPtr *       /* unboundLEDs */
    );

extern Bool CompileSymbols(XkbFile * /* file */ ,
                           XkbFileInfo * /* result */ ,
                           unsigned     /* merge */
    );

#define	WantLongListing	(1<<0)
#define	WantPartialMaps	(1<<1)
#define	WantHiddenMaps	(1<<2)
#define	WantFullNames	(1<<3)
#define	ListRecursive	(1<<4)

extern unsigned verboseLevel;
extern unsigned dirsToStrip;

extern Bool AddMatchingFiles(char *     /* head_in */
    );

extern int AddMapOnly(char *    /* map */
    );

extern int GenerateListing(const char * /* filename */
    );

#endif /* XKBCOMP_H */
