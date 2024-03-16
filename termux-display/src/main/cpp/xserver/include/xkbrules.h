#ifndef _XKBRULES_H_
#define	_XKBRULES_H_ 1

/************************************************************
 Copyright (c) 1996 by Silicon Graphics Computer Systems, Inc.

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

/***====================================================================***/

typedef struct _XkbRMLVOSet {
    char *rules;
    char *model;
    char *layout;
    char *variant;
    char *options;
} XkbRMLVOSet;

typedef struct _XkbRF_VarDefs {
    const char *model;
    const char *layout;
    const char *variant;
    const char *options;
} XkbRF_VarDefsRec, *XkbRF_VarDefsPtr;

typedef struct _XkbRF_Rule {
    int number;
    int layout_num;
    int variant_num;
    const char *model;
    const char *layout;
    const char *variant;
    const char *option;
    /* yields */
    const char *keycodes;
    const char *symbols;
    const char *types;
    const char *compat;
    const char *geometry;
    unsigned flags;
} XkbRF_RuleRec, *XkbRF_RulePtr;

typedef struct _XkbRF_Group {
    int number;
    const char *name;
    char *words;
} XkbRF_GroupRec, *XkbRF_GroupPtr;

#define	XkbRF_PendingMatch	(1L<<1)
#define	XkbRF_Option		(1L<<2)
#define	XkbRF_Append		(1L<<3)
#define	XkbRF_Normal		(1L<<4)
#define	XkbRF_Invalid		(1L<<5)

typedef struct _XkbRF_Rules {
    unsigned short sz_rules;
    unsigned short num_rules;
    XkbRF_RulePtr rules;
    unsigned short sz_groups;
    unsigned short num_groups;
    XkbRF_GroupPtr groups;
} XkbRF_RulesRec, *XkbRF_RulesPtr;

/***====================================================================***/

_XFUNCPROTOBEGIN

/* Seems preferable to dragging xkbstr.h in. */
    struct _XkbComponentNames;

extern _X_EXPORT Bool XkbRF_GetComponents(XkbRF_RulesPtr /* rules */ ,
                                          XkbRF_VarDefsPtr /* var_defs */ ,
                                          struct _XkbComponentNames *   /* names */
    );

extern _X_EXPORT Bool XkbRF_LoadRules(FILE * /* file */ ,
                                      XkbRF_RulesPtr    /* rules */
    );

extern _X_EXPORT Bool XkbRF_LoadRulesByName(char * /* base */ ,
                                            char * /* locale */ ,
                                            XkbRF_RulesPtr      /* rules */
    );

/***====================================================================***/

extern _X_EXPORT XkbRF_RulesPtr XkbRF_Create(void);

extern _X_EXPORT void XkbRF_Free(XkbRF_RulesPtr /* rules */ ,
                                 Bool   /* freeRules */
    );

/***====================================================================***/

#define	_XKB_RF_NAMES_PROP_ATOM		"_XKB_RULES_NAMES"
#define	_XKB_RF_NAMES_PROP_MAXLEN	1024

_XFUNCPROTOEND
#endif                          /* _XKBRULES_H_ */
