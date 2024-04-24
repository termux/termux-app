#ifndef UTILS_H
#define	UTILS_H 1

  /*\
   *
   *                          COPYRIGHT 1990
   *                    DIGITAL EQUIPMENT CORPORATION
   *                       MAYNARD, MASSACHUSETTS
   *                        ALL RIGHTS RESERVED.
   *
   * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
   * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
   * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE
   * FOR ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED
   * WARRANTY.
   *
   * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT
   * RIGHTS, APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN
   * ADDITION TO THAT SET FORTH ABOVE.
   *
   * Permission to use, copy, modify, and distribute this software and its
   * documentation for any purpose and without fee is hereby granted, provided
   * that the above copyright notice appear in all copies and that both that
   * copyright notice and this permission notice appear in supporting
   * documentation, and that the name of Digital Equipment Corporation not be
   * used in advertising or publicity pertaining to distribution of the
   * software without specific, written prior permission.
   \*/

/***====================================================================***/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include 	<stdio.h>
#include 	<stdlib.h>
#include	<X11/Xos.h>
#include	<X11/Xfuncproto.h>
#include	<X11/Xfuncs.h>

#include <stddef.h>

#ifndef NUL
#define	NUL	'\0'
#endif

/***====================================================================***/

#ifndef BOOLEAN_DEFINED
typedef char Boolean;
#endif

#ifndef True
#define	True	((Boolean)1)
#define	False	((Boolean)0)
#endif /* ndef True */
#define	booleanText(b)	((b)?"True":"False")

#ifndef COMPARISON_DEFINED
typedef int Comparison;

#define	Greater		((Comparison)1)
#define	Equal		((Comparison)0)
#define	Less		((Comparison)-1)
#define	CannotCompare	((Comparison)-37)
#define	comparisonText(c)	((c)?((c)<0?"Less":"Greater"):"Equal")
#endif

/***====================================================================***/

#ifndef HAVE_REALLOCARRAY
#define reallocarray(p, n, s) realloc(p, (n) * (s))
#endif

#ifndef HAVE_RECALLOCARRAY
#define recallocarray uRecalloc

extern void *uRecalloc(void * /* old */ ,
                       size_t /* nOld */ ,
                       size_t /* nNew */ ,
                       size_t /* newSize */
    );
#endif

/***====================================================================***/

extern Boolean uSetErrorFile(const char * /* name */
    );

#define INFO 			uInformation

extern void
uInformation(const char * /* s */ , ...
    ) _X_ATTRIBUTE_PRINTF(1, 2);

#define ACTION			uAction

     extern void uAction(const char * /* s  */ , ...
    ) _X_ATTRIBUTE_PRINTF(1, 2);

#define WARN			uWarning

     extern void uWarning(const char * /* s  */ , ...
    ) _X_ATTRIBUTE_PRINTF(1, 2);

#define ERROR			uError

     extern void uError(const char * /* s  */ , ...
    ) _X_ATTRIBUTE_PRINTF(1, 2);

#define FATAL			uFatalError

     extern void uFatalError(const char * /* s  */ , ...
    ) _X_ATTRIBUTE_PRINTF(1, 2) _X_NORETURN;

/* WSGO stands for "Weird Stuff Going On" */
#define WSGO			uInternalError

     extern void uInternalError(const char * /* s  */ , ...
    ) _X_ATTRIBUTE_PRINTF(1, 2);

     extern void uSetPreErrorMessage(const char * /* msg */
    );

     extern void uSetPostErrorMessage(const char * /* msg */
    );

     extern void uSetErrorPrefix(const char * /* void */
    );

     extern void uFinishUp(void);


/***====================================================================***/

#define	NullString	((char *)NULL)

#define	uStringText(s)		((s)==NullString?"<NullString>":(s))
#define	uStringEqual(s1,s2)	(uStringCompare(s1,s2)==Equal)
#define	uStringPrefix(p,s)	(strncmp(p,s,strlen(p))==0)
#define	uStringCompare(s1,s2)	(((s1)==NullString||(s2)==NullString)?\
                                 (s1)!=(s2):strcmp(s1,s2))
#define	uStrCaseEqual(s1,s2)	(uStrCaseCmp(s1,s2)==0)
#ifdef HAVE_STRCASECMP
#include <strings.h>
#define	uStrCaseCmp(s1,s2)	(strcasecmp(s1,s2))
#define	uStrCasePrefix(p,s)	(strncasecmp(p,s,strlen(p))==0)
#else
     extern int uStrCaseCmp(const char * /* s1 */ ,
                            const char *        /* s2 */
    );
     extern int uStrCasePrefix(const char * /* p */ ,
                               char *   /* str */
    );
#endif
#ifdef HAVE_STRDUP
#include <string.h>
#define	uStringDup(s1)		((s1) ? strdup(s1) : NULL)
#else
     extern char *uStringDup(const char *       /* s1 */
    );
#endif

/***====================================================================***/

#ifdef DEBUG
#ifndef DEBUG_VAR
#define	DEBUG_VAR	debugFlags
#endif

extern unsigned int DEBUG_VAR;

extern void uDebug(const char *, ...)  _X_ATTRIBUTE_PRINTF(1, 2);

extern Boolean uSetDebugFile(const char *name);

extern int uDebugIndentLevel;

#define	uDebugIndent(l)		(uDebugIndentLevel+=(l))
#define	uDebugOutdent(l)	(uDebugIndentLevel-=(l))

#define	uDEBUG(f,s)		{ if (DEBUG_VAR&(f)) uDebug(s);}
#define	uDEBUG1(f,s,a)		{ if (DEBUG_VAR&(f)) uDebug(s,a);}
#define	uDEBUG2(f,s,a,b)	{ if (DEBUG_VAR&(f)) uDebug(s,a,b);}
#define	uDEBUG3(f,s,a,b,c)	{ if (DEBUG_VAR&(f)) uDebug(s,a,b,c);}
#define	uDEBUG4(f,s,a,b,c,d)	{ if (DEBUG_VAR&(f)) uDebug(s,a,b,c,d);}
#define	uDEBUG5(f,s,a,b,c,d,e)	{ if (DEBUG_VAR&(f)) uDebug(s,a,b,c,d,e);}
#else
#define	uDEBUG(f,s)
#define	uDEBUG1(f,s,a)
#define	uDEBUG2(f,s,a,b)
#define	uDEBUG3(f,s,a,b,c)
#define	uDEBUG4(f,s,a,b,c,d)
#define	uDEBUG5(f,s,a,b,c,d,e)
#endif

#endif /* UTILS_H */
