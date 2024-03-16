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

   Major Release ID: X-TrueType Server Version 1.3 [Aoi MATSUBARA Release 3]

Notice===
 */

#ifndef _XTTCAP_H_
#define _XTTCAP_H_ (1)

#include <X11/Xdefs.h>

/*******************************************************************
  Data Types
 */

/* Record Type */
typedef enum
{
    eRecTypeInteger,
    eRecTypeDouble,
    eRecTypeBool,
    eRecTypeString,
    eRecTypeVoid=-1
} ERecType;

/* Record Name vs Record Type */
typedef struct
{
    char const *strRecordName;
    ERecType const recordType;
} SPropertyRecord;

/* Record Value Container */
typedef struct
{
    SPropertyRecord const *refRecordType;
    union {
        int integerValue;
        double doubleValue;
        Bool boolValue;
        char *dynStringValue;
    } uValue;
} SPropRecValContainerEntityP, *SPropRecValContainer;

/* Record Value List */
typedef struct TagSPropRecValListNodeP SPropRecValListNode;
typedef struct
{
    SPropRecValListNode *headNode;
} SDynPropRecValList;
typedef SDynPropRecValList const SRefPropRecValList;


/*******************************************************************
  Functions
 */

/* Constructor for Rec Val List */
extern Bool /* True == Error, False == Success */
SPropRecValList_new(SDynPropRecValList *pThisList);
/* Destructor for Rec Val List */
extern Bool /* True == Error, False == Success */
SPropRecValList_delete(SDynPropRecValList *pThisList);
/* Read Property File */
extern Bool /* True == Error, False == Success */
SPropRecValList_read_prop_file(SDynPropRecValList *pThisList,
                               char const * const strFileName);
/* Search Property Record */
extern Bool /* True == Hit, False == Miss */
SPropRecValList_search_record(SRefPropRecValList *pThisList,
                              SPropRecValContainer *refContRecVal,
                              char const * const recordName);
/* Add by Font Cap */
extern Bool /* True == Error, False == Success */
SPropRecValList_add_by_font_cap(SDynPropRecValList *pThisList,
                                char const *strCapHead);

#ifdef DUMP
void
SPropRecValList_dump(SRefPropRecValList *refList);
#endif

#define SPropContainer_value_int(contRecVal)\
  ((contRecVal)->uValue.integerValue)
#define SPropContainer_value_dbl(contRecVal)\
  ((contRecVal)->uValue.doubleValue)
#define SPropContainer_value_bool(contRecVal)\
  ((contRecVal)->uValue.boolValue)
#define SPropContainer_value_str(contRecVal)\
  ((contRecVal)->uValue.dynStringValue)

#endif /* !def _XTTCAP_H_ */

/* end of file */
