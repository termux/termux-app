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
#ifndef TOKENS_H
#define	TOKENS_H 1

#define	END_OF_FILE	0
#define	ERROR_TOK	255

#define	XKB_KEYMAP	1
#define	XKB_KEYCODES	2
#define	XKB_TYPES	3
#define	XKB_SYMBOLS	4
#define	XKB_COMPATMAP	5
#define	XKB_GEOMETRY	6
#define	XKB_SEMANTICS	7
#define	XKB_LAYOUT	8

#define	INCLUDE		10
#define	OVERRIDE	11
#define	AUGMENT		12
#define	REPLACE		13
#define	ALTERNATE	14

#define	VIRTUAL_MODS	20
#define	TYPE		21
#define	INTERPRET	22
#define	ACTION_TOK	23
#define	KEY		24
#define	ALIAS		25
#define	GROUP		26
#define	MODIFIER_MAP	27
#define	INDICATOR	28
#define	SHAPE		29
#define	KEYS		30
#define	ROW		31
#define	SECTION		32
#define	OVERLAY		33
#define	TEXT		34
#define	OUTLINE		35
#define	SOLID		36
#define	LOGO		37
#define	VIRTUAL		38

#define	EQUALS		40
#define	PLUS		41
#define	MINUS		42
#define	DIVIDE		43
#define	TIMES		44
#define	OBRACE		45
#define	CBRACE		46
#define	OPAREN		47
#define	CPAREN		48
#define	OBRACKET	49
#define	CBRACKET	50
#define	DOT		51
#define	COMMA		52
#define	SEMI		53
#define	EXCLAM		54
#define	INVERT		55

#define	STRING		60
#define	INTEGER		61
#define	FLOAT		62
#define	IDENT		63
#define	KEYNAME		64

#define	PARTIAL		70
#define	DEFAULT		71
#define	HIDDEN		72
#define	ALPHANUMERIC_KEYS	73
#define	MODIFIER_KEYS		74
#define	KEYPAD_KEYS		75
#define	FUNCTION_KEYS		76
#define	ALTERNATE_GROUP		77

extern Atom tok_ONE_LEVEL;
extern Atom tok_TWO_LEVEL;
extern Atom tok_ALPHABETIC;
extern Atom tok_KEYPAD;

#endif
