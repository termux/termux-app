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

%token
	END_OF_FILE	0
	ERROR_TOK	255
	XKB_KEYMAP	1
	XKB_KEYCODES	2
	XKB_TYPES	3
	XKB_SYMBOLS	4
	XKB_COMPATMAP	5
	XKB_GEOMETRY	6
	XKB_SEMANTICS	7
	XKB_LAYOUT	8
	INCLUDE		10
	OVERRIDE	11
	AUGMENT		12
	REPLACE		13
	ALTERNATE	14
	VIRTUAL_MODS	20
	TYPE		21
	INTERPRET	22
	ACTION_TOK	23
	KEY		24
	ALIAS		25
	GROUP		26
	MODIFIER_MAP	27
	INDICATOR	28
	SHAPE		29
	KEYS		30
	ROW		31
	SECTION		32
	OVERLAY		33
	TEXT		34
	OUTLINE		35
	SOLID		36
	LOGO		37
	VIRTUAL		38
	EQUALS		40
	PLUS		41
	MINUS		42
	DIVIDE		43
	TIMES		44
	OBRACE		45
	CBRACE		46
	OPAREN		47
	CPAREN		48
	OBRACKET	49
	CBRACKET	50
	DOT		51
	COMMA		52
	SEMI		53
	EXCLAM		54
	INVERT		55
	STRING		60
	INTEGER		61
	FLOAT		62
	IDENT		63
	KEYNAME		64
	PARTIAL		70
	DEFAULT		71
	HIDDEN		72
	ALPHANUMERIC_KEYS	73
	MODIFIER_KEYS		74
	KEYPAD_KEYS		75
	FUNCTION_KEYS		76
	ALTERNATE_GROUP		77
%{
#ifdef DEBUG
#define	YYDEBUG 1
#define	DEBUG_VAR parseDebug
unsigned int parseDebug;
#endif
#include "parseutils.h"
#include <X11/keysym.h>
#include <X11/extensions/XKBgeom.h>
#include <stdlib.h>

%}
%right	EQUALS
%left	PLUS MINUS
%left	TIMES DIVIDE
%left	EXCLAM INVERT
%left	OPAREN
%start	XkbFile
%union	{
	int		 ival;
	unsigned	 uval;
	char		*str;
	Atom	 	sval;
	ParseCommon	*any;
	ExprDef		*expr;
	VarDef		*var;
	VModDef		*vmod;
	InterpDef	*interp;
	KeyTypeDef	*keyType;
	SymbolsDef	*syms;
	ModMapDef	*modMask;
	GroupCompatDef	*groupCompat;
	IndicatorMapDef	*ledMap;
	IndicatorNameDef *ledName;
	KeycodeDef	*keyName;
	KeyAliasDef	*keyAlias;
	ShapeDef	*shape;
	SectionDef	*section;
	RowDef		*row;
	KeyDef		*key;
	OverlayDef	*overlay;
	OverlayKeyDef	*olKey;
	OutlineDef	*outline;
	DoodadDef	*doodad;
	XkbFile		*file;
}
%type <ival>	Number Integer Float SignedNumber
%type <uval>	XkbCompositeType FileType MergeMode OptMergeMode
%type <uval>	DoodadType Flag Flags OptFlags
%type <str>	KeyName MapName OptMapName KeySym
%type <sval>	FieldSpec Ident Element String
%type <any>	DeclList Decl
%type <expr>	OptExprList ExprList Expr Term Lhs Terminal ArrayInit KeySyms
%type <expr>	OptKeySymList KeySymList Action ActionList Coord CoordList
%type <var>	VarDecl VarDeclList SymbolsBody SymbolsVarDecl
%type <vmod>	VModDecl VModDefList VModDef
%type <interp>	InterpretDecl InterpretMatch
%type <keyType>	KeyTypeDecl
%type <syms>	SymbolsDecl
%type <modMask>	ModMapDecl
%type <groupCompat> GroupCompatDecl
%type <ledMap>	IndicatorMapDecl
%type <ledName>	IndicatorNameDecl
%type <keyName>	KeyNameDecl
%type <keyAlias> KeyAliasDecl
%type <shape>	ShapeDecl
%type <section>	SectionDecl
%type <row>	SectionBody SectionBodyItem
%type <key>	RowBody RowBodyItem Keys Key
%type <overlay>	OverlayDecl
%type <olKey>	OverlayKeyList OverlayKey
%type <outline>	OutlineList OutlineInList
%type <doodad>	DoodadDecl
%type <file>	XkbFile XkbMapConfigList XkbMapConfig XkbConfig
%type <file>	XkbCompositeMap XkbCompMapList
%%
XkbFile		:	XkbCompMapList
			{ $$= rtrnValue= $1; }
		|	XkbMapConfigList
			{ $$= rtrnValue= $1;  }
		|	XkbConfig
			{ $$= rtrnValue= $1; }
		;

XkbCompMapList	:	XkbCompMapList XkbCompositeMap
			{ $$= (XkbFile *)AppendStmt(&$1->common,&$2->common); }
		|	XkbCompositeMap
			{ $$= $1; }
		;

XkbCompositeMap	:	OptFlags XkbCompositeType OptMapName OBRACE
			    XkbMapConfigList
			CBRACE SEMI
			{ $$= CreateXKBFile($2,$3,&$5->common,$1); }
		;

XkbCompositeType:	XKB_KEYMAP	{ $$= XkmKeymapFile; }
		|	XKB_SEMANTICS	{ $$= XkmSemanticsFile; }
		|	XKB_LAYOUT	{ $$= XkmLayoutFile; }
		;

XkbMapConfigList :	XkbMapConfigList XkbMapConfig
			{ $$= (XkbFile *)AppendStmt(&$1->common,&$2->common); }
		|	XkbMapConfig
			{ $$= $1; }
		;

XkbMapConfig	:	OptFlags FileType OptMapName OBRACE
			    DeclList
			CBRACE SEMI
			{ $$= CreateXKBFile($2,$3,$5,$1); }
		;

XkbConfig	:	OptFlags FileType OptMapName DeclList
			{ $$= CreateXKBFile($2,$3,$4,$1); }
		;


FileType	:	XKB_KEYCODES		{ $$= XkmKeyNamesIndex; }
		|	XKB_TYPES		{ $$= XkmTypesIndex; }
		|	XKB_COMPATMAP		{ $$= XkmCompatMapIndex; }
		|	XKB_SYMBOLS		{ $$= XkmSymbolsIndex; }
		|	XKB_GEOMETRY		{ $$= XkmGeometryIndex; }
		;

OptFlags	:	Flags			{ $$= $1; }
		|				{ $$= 0; }
		;

Flags		:	Flags Flag		{ $$= (($1)|($2)); }
		|	Flag			{ $$= $1; }
		;

Flag		:	PARTIAL			{ $$= XkbLC_Partial; }
		|	DEFAULT			{ $$= XkbLC_Default; }
		|	HIDDEN			{ $$= XkbLC_Hidden; }
		|	ALPHANUMERIC_KEYS	{ $$= XkbLC_AlphanumericKeys; }
		|	MODIFIER_KEYS		{ $$= XkbLC_ModifierKeys; }
		|	KEYPAD_KEYS		{ $$= XkbLC_KeypadKeys; }
		|	FUNCTION_KEYS		{ $$= XkbLC_FunctionKeys; }
		|	ALTERNATE_GROUP		{ $$= XkbLC_AlternateGroup; }
		;

DeclList	:	DeclList Decl
			{ $$= AppendStmt($1,$2); }
		|	{ $$= NULL; }
		;

Decl		:	OptMergeMode VarDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode VModDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode InterpretDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode KeyNameDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode KeyAliasDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode KeyTypeDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode SymbolsDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode ModMapDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode GroupCompatDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode IndicatorMapDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode IndicatorNameDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode ShapeDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode SectionDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	OptMergeMode DoodadDecl
			{
			    $2->merge= StmtSetMerge(&$2->common,$1);
			    $$= &$2->common;
			}
		|	MergeMode STRING
			{
			    if ($1==MergeAltForm) {
				yyerror("cannot use 'alternate' to include other maps");
				$$= &IncludeCreate(scanBuf,MergeDefault)->common;
			    }
			    else {
				$$= &IncludeCreate(scanBuf,$1)->common;
			    }
                        }
		;

VarDecl		:	Lhs EQUALS Expr SEMI
			{ $$= VarCreate($1,$3); }
		|	Ident SEMI
			{ $$= BoolVarCreate($1,1); }
		|	EXCLAM Ident SEMI
			{ $$= BoolVarCreate($2,0); }
		;

KeyNameDecl	:	KeyName EQUALS Expr SEMI
                        {
			    KeycodeDef *def;

			    def= KeycodeCreate($1,$3);
			    if ($1)
				free($1);
			    $$= def;
			}
		;

KeyAliasDecl	:	ALIAS KeyName EQUALS KeyName SEMI
			{
			    KeyAliasDef	*def;
			    def= KeyAliasCreate($2,$4);
			    if ($2)	free($2);	
			    if ($4)	free($4);	
			    $$= def;
			}
		;

VModDecl	:	VIRTUAL_MODS VModDefList SEMI
			{ $$= $2; }
		;

VModDefList	:	VModDefList COMMA VModDef
			{ $$= (VModDef *)AppendStmt(&$1->common,&$3->common); }
		|	VModDef
			{ $$= $1; }
		;

VModDef		:	Ident
			{ $$= VModCreate($1,NULL); }
		|	Ident EQUALS Expr
			{ $$= VModCreate($1,$3); }
		;

InterpretDecl	:	INTERPRET InterpretMatch OBRACE
			    VarDeclList
			CBRACE SEMI
			{
			    $2->def= $4;
			    $$= $2;
			}
		;

InterpretMatch	:	KeySym PLUS Expr	
			{ $$= InterpCreate($1, $3); }
		|	KeySym			
			{ $$= InterpCreate($1, NULL); }
		;

VarDeclList	:	VarDeclList VarDecl
			{ $$= (VarDef *)AppendStmt(&$1->common,&$2->common); }
		|	VarDecl
			{ $$= $1; }
		;

KeyTypeDecl	:	TYPE String OBRACE
			    VarDeclList
			CBRACE SEMI
			{ $$= KeyTypeCreate($2,$4); }
		;

SymbolsDecl	:	KEY KeyName OBRACE
			    SymbolsBody
			CBRACE SEMI
			{ $$= SymbolsCreate($2,(ExprDef *)$4); }
		;

SymbolsBody	:	SymbolsBody COMMA SymbolsVarDecl
			{ $$= (VarDef *)AppendStmt(&$1->common,&$3->common); }
		|	SymbolsVarDecl
			{ $$= $1; }
		|	{ $$= NULL; }
		;

SymbolsVarDecl	:	Lhs EQUALS Expr
			{ $$= VarCreate($1,$3); }
		|	Lhs EQUALS ArrayInit
			{ $$= VarCreate($1,$3); }
		|	Ident
			{ $$= BoolVarCreate($1,1); }
		|	EXCLAM Ident
			{ $$= BoolVarCreate($2,0); }
		|	ArrayInit
			{ $$= VarCreate(NULL,$1); }
		;

ArrayInit	:	OBRACKET OptKeySymList CBRACKET
			{ $$= $2; }
		|	OBRACKET ActionList CBRACKET
			{ $$= ExprCreateUnary(ExprActionList,TypeAction,$2); }
		;

GroupCompatDecl	:	GROUP Integer EQUALS Expr SEMI
			{ $$= GroupCompatCreate($2,$4); }
		;

ModMapDecl	:	MODIFIER_MAP Ident OBRACE ExprList CBRACE SEMI
			{ $$= ModMapCreate($2,$4); }
		;

IndicatorMapDecl:	INDICATOR String OBRACE VarDeclList CBRACE SEMI
			{ $$= IndicatorMapCreate($2,$4); }
		;

IndicatorNameDecl:	INDICATOR Integer EQUALS Expr SEMI
			{ $$= IndicatorNameCreate($2,$4,False); }
		|	VIRTUAL INDICATOR Integer EQUALS Expr SEMI
			{ $$= IndicatorNameCreate($3,$5,True); }
		;

ShapeDecl	:	SHAPE String OBRACE OutlineList CBRACE SEMI
			{ $$= ShapeDeclCreate($2,(OutlineDef *)&$4->common); }
		|	SHAPE String OBRACE CoordList CBRACE SEMI
			{
			    OutlineDef *outlines;
			    outlines= OutlineCreate(None,$4);
			    $$= ShapeDeclCreate($2,outlines);
			}
		;

SectionDecl	:	SECTION String OBRACE SectionBody CBRACE SEMI
			{ $$= SectionDeclCreate($2,$4); }
		;

SectionBody	:	SectionBody SectionBodyItem
			{ $$=(RowDef *)AppendStmt(&$1->common,&$2->common);}
		|	SectionBodyItem
			{ $$= $1; }
		;

SectionBodyItem	:	ROW OBRACE RowBody CBRACE SEMI
			{ $$= RowDeclCreate($3); }
		|	VarDecl
			{ $$= (RowDef *)$1; }
		|	DoodadDecl
			{ $$= (RowDef *)$1; }
		|	IndicatorMapDecl
			{ $$= (RowDef *)$1; }
		|	OverlayDecl
			{ $$= (RowDef *)$1; }
		;

RowBody		:	RowBody RowBodyItem
			{ $$=(KeyDef *)AppendStmt(&$1->common,&$2->common);}
		|	RowBodyItem
			{ $$= $1; }
		;

RowBodyItem	:	KEYS OBRACE Keys CBRACE SEMI
			{ $$= $3; }
		|	VarDecl
			{ $$= (KeyDef *)$1; }
		;

Keys		:	Keys COMMA Key
			{ $$=(KeyDef *)AppendStmt(&$1->common,&$3->common);}
		|	Key
			{ $$= $1; }
		;

Key		:	KeyName
			{ $$= KeyDeclCreate($1,NULL); }
		|	OBRACE ExprList CBRACE
			{ $$= KeyDeclCreate(NULL,$2); }
		;

OverlayDecl	:	OVERLAY String OBRACE OverlayKeyList CBRACE SEMI
			{ $$= OverlayDeclCreate($2,$4); }
		;

OverlayKeyList	:	OverlayKeyList COMMA OverlayKey
			{
			    $$= (OverlayKeyDef *)
				AppendStmt(&$1->common,&$3->common);
			}
		|	OverlayKey
			{ $$= $1; }
		;

OverlayKey	:	KeyName EQUALS KeyName
			{ $$= OverlayKeyCreate($1,$3); }
		;

OutlineList	:	OutlineList COMMA OutlineInList
			{ $$=(OutlineDef *)AppendStmt(&$1->common,&$3->common);}
		|	OutlineInList
			{ $$= $1; }
		;

OutlineInList	:	OBRACE CoordList CBRACE
			{ $$= OutlineCreate(None,$2); }
		|	Ident EQUALS OBRACE CoordList CBRACE
			{ $$= OutlineCreate($1,$4); }
		|	Ident EQUALS Expr
			{ $$= OutlineCreate($1,$3); }
		;

CoordList	:	CoordList COMMA Coord
			{ $$= (ExprDef *)AppendStmt(&$1->common,&$3->common); }
		|	Coord
			{ $$= $1; }
		;

Coord		:	OBRACKET SignedNumber COMMA SignedNumber CBRACKET
			{
			    ExprDef *expr;
			    expr= ExprCreate(ExprCoord,TypeUnknown);
			    expr->value.coord.x= $2;
			    expr->value.coord.y= $4;
			    $$= expr;
			}
		;

DoodadDecl	:	DoodadType String OBRACE VarDeclList CBRACE SEMI
			{ $$= DoodadCreate($1,$2,$4); }
		;

DoodadType	:	TEXT			{ $$= XkbTextDoodad; }
		|	OUTLINE			{ $$= XkbOutlineDoodad; }
		|	SOLID			{ $$= XkbSolidDoodad; }
		|	LOGO			{ $$= XkbLogoDoodad; }
		;

FieldSpec	:	Ident			{ $$= $1; }
		|	Element			{ $$= $1; }
		;

Element		:	ACTION_TOK		
			{ $$= XkbInternAtom(NULL,"action",False); }
		|	INTERPRET
			{ $$= XkbInternAtom(NULL,"interpret",False); }
		|	TYPE
			{ $$= XkbInternAtom(NULL,"type",False); }
		|	KEY
			{ $$= XkbInternAtom(NULL,"key",False); }
		|	GROUP
			{ $$= XkbInternAtom(NULL,"group",False); }
		|	MODIFIER_MAP
			{$$=XkbInternAtom(NULL,"modifier_map",False);}
		|	INDICATOR
			{ $$= XkbInternAtom(NULL,"indicator",False); }
		|	SHAPE	
			{ $$= XkbInternAtom(NULL,"shape",False); }
		|	ROW	
			{ $$= XkbInternAtom(NULL,"row",False); }
		|	SECTION	
			{ $$= XkbInternAtom(NULL,"section",False); }
		|	TEXT
			{ $$= XkbInternAtom(NULL,"text",False); }
		;

OptMergeMode	:	MergeMode		{ $$= $1; }
		|				{ $$= MergeDefault; }
		;

MergeMode	:	INCLUDE			{ $$= MergeDefault; }
		|	AUGMENT			{ $$= MergeAugment; }
		|	OVERRIDE		{ $$= MergeOverride; }
		|	REPLACE			{ $$= MergeReplace; }
		|	ALTERNATE		{ $$= MergeAltForm; }
		;

OptExprList	:	ExprList			{ $$= $1; }
		|				{ $$= NULL; }
		;

ExprList	:	ExprList COMMA Expr
			{ $$= (ExprDef *)AppendStmt(&$1->common,&$3->common); }
		|	Expr
			{ $$= $1; }
		;

Expr		:	Expr DIVIDE Expr
			{ $$= ExprCreateBinary(OpDivide,$1,$3); }
		|	Expr PLUS Expr
			{ $$= ExprCreateBinary(OpAdd,$1,$3); }
		|	Expr MINUS Expr
			{ $$= ExprCreateBinary(OpSubtract,$1,$3); }
		|	Expr TIMES Expr
			{ $$= ExprCreateBinary(OpMultiply,$1,$3); }
		|	Lhs EQUALS Expr
			{ $$= ExprCreateBinary(OpAssign,$1,$3); }
		|	Term
			{ $$= $1; }
		;

Term		:	MINUS Term
			{ $$= ExprCreateUnary(OpNegate,$2->type,$2); }
		|	PLUS Term
			{ $$= ExprCreateUnary(OpUnaryPlus,$2->type,$2); }
		|	EXCLAM Term
			{ $$= ExprCreateUnary(OpNot,TypeBoolean,$2); }
		|	INVERT Term
			{ $$= ExprCreateUnary(OpInvert,$2->type,$2); }
		|	Lhs
			{ $$= $1;  }
		|	FieldSpec OPAREN OptExprList CPAREN %prec OPAREN
			{ $$= ActionCreate($1,$3); }
		|	Terminal
			{ $$= $1;  }
		|	OPAREN Expr CPAREN
			{ $$= $2;  }
		;

ActionList	:	ActionList COMMA Action
			{ $$= (ExprDef *)AppendStmt(&$1->common,&$3->common); }
		|	Action
			{ $$= $1; }
		;

Action		:	FieldSpec OPAREN OptExprList CPAREN
			{ $$= ActionCreate($1,$3); }
		;

Lhs		:	FieldSpec
			{
			    ExprDef *expr;
                            expr= ExprCreate(ExprIdent,TypeUnknown);
                            expr->value.str= $1;
                            $$= expr;
			}
		|	FieldSpec DOT FieldSpec
                        {
                            ExprDef *expr;
                            expr= ExprCreate(ExprFieldRef,TypeUnknown);
                            expr->value.field.element= $1;
                            expr->value.field.field= $3;
                            $$= expr;
			}
		|	FieldSpec OBRACKET Expr CBRACKET
			{
			    ExprDef *expr;
			    expr= ExprCreate(ExprArrayRef,TypeUnknown);
			    expr->value.array.element= None;
			    expr->value.array.field= $1;
			    expr->value.array.entry= $3;
			    $$= expr;
			}
		|	FieldSpec DOT FieldSpec OBRACKET Expr CBRACKET
			{
			    ExprDef *expr;
			    expr= ExprCreate(ExprArrayRef,TypeUnknown);
			    expr->value.array.element= $1;
			    expr->value.array.field= $3;
			    expr->value.array.entry= $5;
			    $$= expr;
			}
		;

Terminal	:	String
			{
			    ExprDef *expr;
                            expr= ExprCreate(ExprValue,TypeString);
                            expr->value.str= $1;
                            $$= expr;
			}
		|	Integer
			{
			    ExprDef *expr;
                            expr= ExprCreate(ExprValue,TypeInt);
                            expr->value.ival= $1;
                            $$= expr;
			}
		|	Float
			{
			    ExprDef *expr;
			    expr= ExprCreate(ExprValue,TypeFloat);
			    expr->value.ival= $1;
			    $$= expr;
			}
		|	KeyName
			{
			    ExprDef *expr;
			    expr= ExprCreate(ExprValue,TypeKeyName);
			    memset(expr->value.keyName,0,5);
			    strncpy(expr->value.keyName,$1,4);
			    free($1);
			    $$= expr;
			}
		;

OptKeySymList	:	KeySymList			{ $$= $1; }
		|					{ $$= NULL; }
		;

KeySymList	:	KeySymList COMMA KeySym
			{ $$= AppendKeysymList($1,$3); }
                |       KeySymList COMMA KeySyms
                        { $$= AppendKeysymList($1,strdup("NoSymbol")); }
		|	KeySym
			{ $$= CreateKeysymList($1); }
		|	KeySyms
			{ $$= CreateKeysymList(strdup("NoSymbol")); }
		;

KeySym		:	IDENT           { $$= strdup(scanBuf); }
		|	SECTION         { $$= strdup("section"); }
		|	Integer		
			{
			    if ($1<10)	{ $$= malloc(2); $$[0]= '0' + $1; $$[1]= '\0'; }
			    else	{ $$= malloc(19); snprintf($$, 19, "0x%x", $1); }
			}
		;

KeySyms         :       OBRACE KeySymList CBRACE
                        { $$= $2; }
                ;

SignedNumber	:	MINUS Number    { $$= -$2; }
		|	Number              { $$= $1; }
		;

Number		:	FLOAT		{ $$= scanInt; }
		|	INTEGER		{ $$= scanInt*XkbGeomPtsPerMM; }
		;

Float		:	FLOAT		{ $$= scanInt; }
		;

Integer		:	INTEGER		{ $$= scanInt; }
		;

KeyName		:	KEYNAME		{ $$= strdup(scanBuf); }
		;

Ident		:	IDENT	{ $$= XkbInternAtom(NULL,scanBuf,False); }
		|	DEFAULT { $$= XkbInternAtom(NULL,"default",False); }
		;

String		:	STRING	{ $$= XkbInternAtom(NULL,scanBuf,False); }
		;

OptMapName	:	MapName	{ $$= $1; }
		|		{ $$= NULL; }
		;

MapName		:	STRING 	{ $$= strdup(scanBuf); }
		;
%%
void
yyerror(const char *s)
{
    if (warningLevel>0) {
	(void)fprintf(stderr,"%s: line %d of %s\n",s,lineNum,
					(scanFile?scanFile:"(unknown)"));
	if ((warningLevel>3))
	    (void)fprintf(stderr,"last scanned symbol is: %s\n",scanBuf);
    }
    return;
}


int
yywrap(void)
{
   return 1;
}

