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

#ifndef EXPR_H
#define EXPR_H 1

typedef union _ExprResult
{
    char *str;
    int ival;
    unsigned uval;
    XkbKeyNameRec keyName;
} ExprResult;

typedef Bool(*IdentLookupFunc) (const XPointer /* priv */ ,
                                Atom /* elem */ ,
                                Atom /* field */ ,
                                unsigned /* type */ ,
                                ExprResult *    /* val_rtrn */
    );

extern int ExprResolveLhs(const ExprDef * /* expr */ ,
                          ExprResult * /* elem_rtrn */ ,
                          ExprResult * /* field_rtrn */ ,
                          ExprDef **    /* index_rtrn */
    );

typedef struct _LookupPriv
{
    XPointer priv;
    IdentLookupFunc chain;
    XPointer chainPriv;
} LookupPriv;

typedef struct _LookupEntry
{
    const char *name;
    unsigned result;
} LookupEntry;

typedef struct _LookupTable
{
    char *element;
    LookupEntry *entries;
    struct _LookupTable *nextElement;
} LookupTable;


extern char *exprOpText(unsigned        /* type */
    );

extern Bool RadioLookup(const XPointer /* priv */ ,
                       Atom /* elem */ ,
                       Atom /* field */ ,
                       unsigned /* type */ ,
                       ExprResult *     /* val_rtrn */
    );

extern Bool SimpleLookup(const XPointer /* priv */ ,
                        Atom /* elem */ ,
                        Atom /* field */ ,
                        unsigned /* type */ ,
                        ExprResult *    /* val_rtrn */
    );

extern Bool LookupModIndex(const XPointer /* priv */ ,
                          Atom /* elem */ ,
                          Atom /* field */ ,
                          unsigned /* type */ ,
                          ExprResult *  /* val_rtrn */
    );

extern int ExprResolveModIndex(const ExprDef * /* expr */ ,
                               ExprResult * /* val_rtrn */ ,
                               IdentLookupFunc /* lookup */ ,
                               XPointer /* lookupPriv */
    );

extern int ExprResolveModMask(const ExprDef * /* expr */ ,
                              ExprResult * /* val_rtrn */ ,
                              IdentLookupFunc /* lookup */ ,
                              XPointer  /* priv */
    );

extern int ExprResolveBoolean(const ExprDef * /* expr */ ,
                              ExprResult * /* val_rtrn */ ,
                              IdentLookupFunc /* lookup */ ,
                              XPointer  /* lookupPriv */
    );

extern int ExprResolveInteger(const ExprDef * /* expr */ ,
                              ExprResult * /* val_rtrn */ ,
                              IdentLookupFunc /* lookup */ ,
                              XPointer  /* lookupPriv */
    );

extern int ExprResolveFloat(const ExprDef * /* expr */ ,
                            ExprResult * /* val_rtrn */ ,
                            IdentLookupFunc /* lookup */ ,
                            XPointer    /* lookupPriv */
    );

extern int ExprResolveString(const ExprDef * /* expr */ ,
                             ExprResult * /* val_rtrn */ ,
                             IdentLookupFunc /* lookup */ ,
                             XPointer   /* lookupPriv */
    );

extern int ExprResolveKeyName(const ExprDef * /* expr */ ,
                              ExprResult * /* val_rtrn */ ,
                              IdentLookupFunc /* lookup */ ,
                              XPointer  /* lookupPriv */
    );

extern int ExprResolveEnum(const ExprDef * /* expr */ ,
                           ExprResult * /* val_rtrn */ ,
                           const LookupEntry * /* values */
    );

extern int ExprResolveMask(const ExprDef * /* expr */ ,
                           ExprResult * /* val_rtrn */ ,
                           IdentLookupFunc /* lookup */ ,
                           XPointer     /* lookupPriv */
    );

extern int ExprResolveKeySym(const ExprDef * /* expr */ ,
                             ExprResult * /* val_rtrn */ ,
                             IdentLookupFunc /* lookup */ ,
                             XPointer   /* lookupPriv */
    );

#endif /* EXPR_H */
