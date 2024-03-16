/* ===EmacsMode: -*- Mode: C; tab-width:4; c-basic-offset: 4; -*- === */
/* ===FileName: ===
   Copyright (c) 1998 Takuya SHIOZAKI, All Rights reserved.
   Copyright (c) 1998 X-TrueType Server Project, All rights reserved.
   Copyright (c) 2003 After X-TT Project, All rights reserved.

===Notice
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.

   Major Release ID: X-TrueType Server Version 1.4 [Charles's Wain Release 0]

Notice===
 */

/*
#include "xttversion.h"

static char const * const releaseID =
    _XTT_RELEASE_NAME;
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include <X11/fonts/fontmisc.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef True
#define True (-1)
#endif /* True */
#ifndef False
#define False (0)
#endif /* False */

#include "xttcap.h"


/**************************************************************************
  Private Data Types
 */

/* Property Record List */
/* List Node */
typedef struct TagSPropRecValListNodeP
{
    SPropRecValContainerEntityP containerE;
    struct TagSPropRecValListNodeP *nextNode;
} SPropRecValListNodeP;


/**************************************************************************
  Tables
 */

/* valid record field */
static SPropertyRecord const validRecords[] =
{
    { "FontFile",               eRecTypeString  },
    { "FaceNumber",             eRecTypeString },
    { "AutoItalic",             eRecTypeDouble  },
    { "DoubleStrike",           eRecTypeString  },
    { "FontProperties",         eRecTypeBool    },
    { "ForceSpacing",           eRecTypeString  },
    { "ScaleBBoxWidth",         eRecTypeString  },
    { "ScaleWidth",             eRecTypeDouble  },
    { "EncodingOptions",        eRecTypeString  },
    { "Hinting",                eRecTypeBool    },
    { "VeryLazyMetrics",        eRecTypeBool    },
    { "CodeRange",              eRecTypeString  },
    { "EmbeddedBitmap",         eRecTypeString  },
    { "VeryLazyBitmapWidthScale", eRecTypeDouble  },
    { "ForceConstantSpacingCodeRange", eRecTypeString },
    { "ForceConstantSpacingMetrics", eRecTypeString },
    { "Dummy",                  eRecTypeVoid    }
};
static int const
numOfValidRecords = sizeof(validRecords)/sizeof(validRecords[0]);

/* correspondence between record name and cap variable name */
static struct {
    char const * capVariable;
    char const * recordName;
} const correspondRelations[] = {
    { "fn", "FaceNumber" },
    { "ai", "AutoItalic" },
    { "ds", "DoubleStrike" },
    { "fp", "FontProperties" },
    { "fs", "ForceSpacing" },
    { "bw", "ScaleBBoxWidth" },
    { "sw", "ScaleWidth" },
    { "eo", "EncodingOptions" },
    { "vl", "VeryLazyMetrics" },
    { "bs", "VeryLazyBitmapWidthScale" },
    { "cr", "CodeRange" },
    { "eb", "EmbeddedBitmap" },
    { "hi", "Hinting" },
    { "fc", "ForceConstantSpacingCodeRange" },
    { "fm", "ForceConstantSpacingMetrics" }
};
static int const
numOfCorrespondRelations
= sizeof(correspondRelations)/sizeof(correspondRelations[0]);

/**************************************************************************
  Functions
 */

/* get property record type by record name */
static Bool /* True == Found, False == Not Found */
get_record_type_by_name(SPropertyRecord const ** const refRefRecord, /*result*/
                        char const *strName)
{
    Bool result = False;
    int i;

    *refRefRecord = NULL;
    for (i=0; i<numOfValidRecords; i++) {
        if (!strcasecmp(validRecords[i].strRecordName, strName)) {
            result = True;
            *refRefRecord = &validRecords[i];
            break;
        }
    }

    return result;
}

/* Add Property Record Value */
static Bool /* True == Error, False == Success */
SPropRecValList_add_record(SDynPropRecValList *pThisList,
                           char const * const recordName,
                           char const * const strValue)
{
    Bool result = False;
    SPropRecValContainerEntityP tmpContainerE;

    if (get_record_type_by_name(&tmpContainerE.refRecordType, recordName)) {
        switch (tmpContainerE.refRecordType->recordType) {
        case eRecTypeInteger:
            {
                int val;
                char *endPtr;

                val = strtol(strValue, &endPtr, 0);
                if ('\0' != *endPtr) {
                    fprintf(stderr,
                            "truetype font property : "
                            "%s record needs integer value.\n",
                            recordName);
                    result = True;
                    goto quit;
                }
                SPropContainer_value_int(&tmpContainerE) = val;
            }
            break;
        case eRecTypeDouble:
            {
                double val;
                char *endPtr;

                val = strtod(strValue, &endPtr);
                if ('\0' != *endPtr) {
                    fprintf(stderr,
                            "truetype font property : "
                            "%s record needs floating point value.\n",
                            recordName);
                    result = True;
                    goto quit;
                }
                SPropContainer_value_dbl(&tmpContainerE) = val;
            }
            break;
        case eRecTypeBool:
            {
                Bool val;

                if (!strcasecmp(strValue, "yes"))
                    val = True;
                else if (!strcasecmp(strValue, "y"))
                    val = True;
                else if (!strcasecmp(strValue, "on"))
                    val = True;
                else if (!strcasecmp(strValue, "true"))
                    val = True;
                else if (!strcasecmp(strValue, "t"))
                    val = True;
                else if (!strcasecmp(strValue, "ok"))
                    val = True;
                else if (!strcasecmp(strValue, "no"))
                    val = False;
                else if (!strcasecmp(strValue, "n"))
                    val = False;
                else if (!strcasecmp(strValue, "off"))
                    val = False;
                else if (!strcasecmp(strValue, "false"))
                    val = False;
                else if (!strcasecmp(strValue, "f"))
                    val = False;
                else if (!strcasecmp(strValue, "bad"))
                    val = False;
                else {
                    fprintf(stderr,
                            "truetype font property : "
                            "%s record needs boolean value.\n",
                            recordName);
                    result = True;
                    goto quit;
                }
                SPropContainer_value_bool(&tmpContainerE) = val;
            }
            break;
        case eRecTypeString:
            {
                char *p;

                if (NULL == (p = strdup(strValue))) {
                    fprintf(stderr,
                            "truetype font property : "
                            "cannot allocate memory.\n");
                    result = True;
                    goto quit;
                }
                SPropContainer_value_str(&tmpContainerE) = p;
            }
            break;
        case eRecTypeVoid:
            if ('\0' != *strValue) {
                fprintf(stderr,
                        "truetype font property : "
                        "%s record needs void.\n", recordName);
                result = True;
            }
            break;
        }
        {
            /* add to list */
            SPropRecValListNodeP *newNode;

            if (NULL == (newNode = malloc(sizeof(*newNode)))) {
                fprintf(stderr,
                        "truetype font property : "
                        "cannot allocate memory.\n");
                result = True;
                goto quit;
            }
            newNode->nextNode = pThisList->headNode;
            newNode->containerE = tmpContainerE;
            tmpContainerE.refRecordType = NULL; /* invalidate --
                                                   disown value handle. */
            pThisList->headNode = newNode;
        }
    } else {
        /* invalid record name */
        fprintf(stderr,
                "truetype font : "
                "invalid record name \"%s.\"\n", recordName);
        result = True;
    }

 quit:
    return result;
}

#ifdef USE_TTP_FILE

#ifndef LEN_LINEBUF
#define LEN_LINEBUF 2048
#endif /* !def LEN_LINEBUF */

/* get one line */
static Bool /* True == Error, False == Success */
get_one_line(FILE *is, char *buf)
{
    Bool result = False;
    int count = 0;
    Bool flHead = True;
    Bool flSpace = False;
    Bool flInSingleQuote = False;
    Bool flInDoubleQuote = False;
    Bool flBackSlash = False;
    Bool flFirstElement = True;

    *buf = '\0';
    for (;;) {
        int c = fgetc(is);

        if (ferror(is)) {
            fprintf(stderr, "truetype font property file : read error.\n");
            result = True;
            break;
        }

        if (EOF == c) {
            if (flInSingleQuote || flInDoubleQuote) {
                fprintf(stderr,
                        "truetype font property file : unmatched quote.\n");
                result = True;
            }
            break;
        }
        if (flInSingleQuote) {
            if ('\'' == c) {
                /* end of single quoted string */
                flInSingleQuote = False;
                c = -1; /* NOT extract to buffer. */
            } else
                /* others, extract all character to buffer unconditionally. */
                ;
            goto trans;
        }
        if (flBackSlash) {
            /* escape --- when just before character is backslash,
               next character is escaped. */
            flBackSlash = False;
            if ('n' == c)
                /* newline */
                c = '\n';
            if ('\n' == c)
                /* ignore newline */
                c = -1;
            else
                /* others, extract all character to buffer unconditionally. */
                ;
            goto trans;
        }
        if ('\\' == c) {
            /* set flag to escape next character. */
            flBackSlash = True;
            c = -1; /* NOT extract to buffer. */
            goto trans;
        }
        if (flInDoubleQuote) {
            if ('"' == c) {
                /* end of double quoted string */
                flInDoubleQuote = False;
                c = -1; /* NOT extract to buffer. */
            } else
                /* others, extract all character to buffer unconditionally. */
                ;
            goto trans;
        }
        if ('#' == c) {
            /* skip comment till end of line. */
            while ('\n' != c) {
                c = fgetc(is);
                if (ferror(is)) {
                    fprintf(stderr,
                            "truetype font property file : read error.\n");
                    result = True;
                    break;
                }
                if (EOF == c) {
                    break;
                }
            }
            break;
        }
        if ('\'' == c) {
            /* into single quoted string */
            flInSingleQuote = True;
            c = -1; /* NOT extract to buffer. */
            goto trans;
        }
        if ('"' == c) {
            /* into double quoted string */
            flInDoubleQuote = True;
            c = -1; /* NOT extract to buffer. */
            goto trans;
        }
        if ('\n' == c)
            /* End of Line */
            break;
        if (isspace(c)) {
            /* combine multiple spaces */
            if (!flHead)
                /* except space at the head of line */
                flSpace = True;
            continue;
        }
      trans:
        /* set flHead to False, since current character is not white space
           when reaches here. */
        flHead = False;
        do {
          if (count>=LEN_LINEBUF-1) {
              /* overflow */
              fprintf(stderr,
                      "truetype font property file : too long line.\n");
              result = True;
              goto quit;
          }
          if (flSpace) {
              /* just before characters is white space, but
                 current character is not WS. */
              if (flFirstElement) {
                  /* this spaces is the first cell(?) of white spaces. */
                  flFirstElement = False;
                  /* separate record name and record value */
                  *buf = (char)0xff;
              } else
                  *buf = ' ';
              flSpace = False;
          } else
              if (-1 != c) {
                  *buf = c;
                  c = -1; /* invalidate */
              } else
                  /* skip */
                  buf--;
          buf++;
        } while (-1 != c); /* when 'c' is not -1, it means
                              that 'c' contains an untreated character. */
    }
    *buf = '\0';

  quit:
    return result;
}

/* parse one line */
static Bool /* True == Error, False == Success */
parse_one_line(SDynPropRecValList *pThisList, FILE *is)
{
    Bool result = False;
    char *buf = NULL;
    char *recordHead, *valueHead = NULL;

    if (NULL == (buf = malloc(LEN_LINEBUF))) {
        fprintf(stderr,
                "truetype font property file : cannot allocate memory.\n");
        result = True;
        goto abort;
    }
    {
        recordHead = buf;
/*        refRecordValue->refRecordType = NULL;*/
        do {
            if (get_one_line(is, buf)) {
                result = True;
                goto quit;
            }
            if (feof(is)) {
                if ('\0' == *buf)
                    goto quit;
                break;
            }
        } while ('\0' == *buf);

        if (NULL != (valueHead = strchr(buf, 0xff))) {
            *valueHead = '\0';
            valueHead++;
        } else
            valueHead = buf+strlen(buf);
#if 0
        fprintf(stderr,
                "truetype font property file : \n"
                "recName:\"%s\"\nvalue:\"%s\"\n",
                recordHead, valueHead);
#endif
        result = SPropRecValList_add_record(pThisList, recordHead, valueHead);
    }
  quit:
    free(buf);
  abort:
    return result;
}

/* Read Property File */
Bool /* True == Error, False == Success */
SPropRecValList_read_prop_file(SDynPropRecValList *pThisList,
                               char const * const strFileName)
{
    Bool result = False;
    FILE *is;

#if 1
    if (!strcmp(strFileName, "-"))
        is = stdin;
    else
#endif
        is = fopen(strFileName, "r");
    if (NULL == is) {
        fprintf(stderr, "truetype font property : cannot open file %s.\n",
                strFileName);
        result = True;
        goto abort;
    }
    {
        for (;;) {
            if (False != (result = parse_one_line(pThisList, is)))
                goto quit;
            if (feof(is))
                break;
        }
    }
  quit:
#if 1
    if (strcmp(strFileName, "-"))
#endif
        fclose(is);
  abort:
    return result;
}
#endif /* USE_TTP_FILE */

/* Constructor for Container Node */
Bool /* True == Error, False == Success */
SPropRecValList_new(SDynPropRecValList *pThisList)
{
    Bool result = False;

    pThisList->headNode = NULL;

    return result;
}

#ifdef DUMP
void
SPropRecValList_dump(SRefPropRecValList *pThisList)
{
    SPropRecValListNodeP *p;
    for (p=pThisList->headNode; NULL!=p; p=p->nextNode) {
        switch (p->containerE.refRecordType->recordType) {
        case eRecTypeInteger:
            fprintf(stderr, "%s = %d\n",
                    p->containerE.refRecordType->strRecordName,
                    p->containerE.uValue.integerValue);
            break;
        case eRecTypeDouble:
            fprintf(stderr, "%s = %f\n",
                    p->containerE.refRecordType->strRecordName,
                    p->containerE.uValue.doubleValue);
            break;
        case eRecTypeBool:
            fprintf(stderr, "%s = %s\n",
                    p->containerE.refRecordType->strRecordName,
                    p->containerE.uValue.boolValue
                    ? "True":"False");
            break;
        case eRecTypeString:
            fprintf(stderr, "%s = \"%s\"\n",
                    p->containerE.refRecordType->strRecordName,
                    p->containerE.uValue.dynStringValue);
            break;
        case eRecTypeVoid:
            fprintf(stderr, "%s = void\n",
                    p->containerE.refRecordType->strRecordName);
            break;
        }
    }
}
#endif


/* Search Property Record */
Bool /* True == Hit, False == Miss */
SPropRecValList_search_record(SRefPropRecValList *pThisList,
                              SPropRecValContainer *refRecValue,
                              char const * const recordName)
{
    Bool result = False;
    SPropRecValListNodeP *p;

    *refRecValue = NULL;
    for (p=pThisList->headNode; NULL!=p; p=p->nextNode) {
        if (!strcasecmp(p->containerE.refRecordType->strRecordName,
                          recordName)) {
            *refRecValue = &p->containerE;
            result = True;
            break;
        }
    }

    return result;
}


/* Parse TTCap */
Bool /* True == Error, False == Success */
SPropRecValList_add_by_font_cap(SDynPropRecValList *pThisList,
                                char const *strCapHead)
{
    Bool result = False;
    /*    SPropertyRecord const *refRecordType; */
    char const *term;

    if (NULL == (term = strrchr(strCapHead, ':')))
        goto abort;

    {
        /* for xfsft compatible */
        char const *p;
        for (p=term-1; p>=strCapHead; p--) {
            if ( ':'==*p ) {
                /*
                 * :num:filename
                 * ^p  ^term
                 */
                if ( p!=term ) {
                    int len = term-p-1;
                    char *value;

                    value=malloc(len+1);
                    memcpy(value, p+1, len);
                    value[len]='\0';
                    SPropRecValList_add_record(pThisList,
                                               "FaceNumber",
                                               value);
                    free(value);
                    term=p;
                }
                break;
            }
            if ( !isdigit((unsigned char)*p) )
                break;
        }
    }

    while (strCapHead<term) {
        int i;
        char const *nextColon = strchr(strCapHead, ':');
        if (0<nextColon-strCapHead) {
            char *duplicated = malloc((nextColon-strCapHead)+1);
            {
                char *value;

                memcpy(duplicated, strCapHead, nextColon-strCapHead);
                duplicated[nextColon-strCapHead] = '\0';
                if (NULL != (value=strchr(duplicated, '='))) {
                    *value = '\0';
                    value++;
                } else
                    value = &duplicated[nextColon-strCapHead];

                for (i=0; i<numOfCorrespondRelations; i++) {
                    if (!strcasecmp(correspondRelations[i].capVariable,
                                      duplicated)) {
                        if (SPropRecValList_add_record(pThisList,
                                                        correspondRelations[i]
                                                       .recordName,
                                                       value))
                            break;
                        goto next;
                    }
                }
                fprintf(stderr, "truetype font : Illegal Font Cap.\n");
                result = True;
                break;
              next:
                ;
            }
            free(duplicated);
        }
        strCapHead = nextColon+1;
    }

    /*  quit: */
  abort:
    return result;
}

/* end of file */
