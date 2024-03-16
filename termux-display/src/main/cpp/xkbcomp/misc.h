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

#ifndef MISC_H
#define MISC_H 1

typedef struct _CommonInfo
{
    unsigned short defined;
    unsigned char fileID;
    unsigned char merge;
    struct _CommonInfo *next;
} CommonInfo;

extern Bool UseNewField(unsigned /* field */ ,
                        const CommonInfo * /* oldDefs */ ,
                        const CommonInfo * /* newDefs */ ,
                        unsigned *      /* pCollide */
    );

extern XPointer ClearCommonInfo(CommonInfo *    /* cmn */
    );

extern XPointer AddCommonInfo(CommonInfo * /* old */ ,
                              CommonInfo *      /* new */
    );

extern int ReportNotArray(const char * /* type */ ,
                          const char * /* field */ ,
                          const char *  /* name */
    );

extern int ReportShouldBeArray(const char * /* type */ ,
                               const char * /* field */ ,
                               char *   /* name */
    );

extern int ReportBadType(const char * /* type */ ,
                         const char * /* field */ ,
                         const char * /* name */ ,
                         const char *   /* wanted */
    );

extern int ReportBadIndexType(char * /* type */ ,
                              char * /* field */ ,
                              char * /* name */ ,
                              char *    /* wanted */
    );

extern int ReportBadField(const char * /* type */ ,
                          const char * /* field */ ,
                          const char *  /* name */
    );

extern int ReportMultipleDefs(char * /* type */ ,
                              char * /* field */ ,
                              char *    /* which */
    );

extern Bool ProcessIncludeFile(IncludeStmt * /* stmt */ ,
                               unsigned /* file_type */ ,
                               XkbFile ** /* file_rtrn */ ,
                               unsigned *       /* merge_rtrn */
    );

extern Status ComputeKbdDefaults(XkbDescPtr     /* xkb */
    );

extern Bool FindNamedKey(XkbDescPtr /* xkb */ ,
                         unsigned long /* name */ ,
                         unsigned int * /* kc_rtrn */ ,
                         Bool /* use_aliases */ ,
                         Bool /* create */ ,
                         int    /* start_from */
    );

extern Bool FindKeyNameForAlias(XkbDescPtr /* xkb */ ,
                                unsigned long /* lname */ ,
                                unsigned long * /* real_name */
    );

#endif /* MISC_H */
