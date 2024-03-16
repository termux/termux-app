/*
 *
 * Copyright (c) 1997  Metro Link Incorporated
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 *
 */
/*
 * Copyright (c) 1997-2002 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * These definitions are used through out the configuration file parser, but
 * they should not be visible outside of the parser.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _Configint_h_
#define _Configint_h_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include "xf86Parser.h"

typedef enum { PARSE_DECIMAL, PARSE_OCTAL, PARSE_HEX } ParserNumType;

typedef struct {
    int num;                    /* returned number */
    char *str;                  /* private copy of the return-string */
    double realnum;             /* returned number as a real */
    ParserNumType numType;      /* used to enforce correct number formatting */
} LexRec, *LexPtr;

extern LexRec xf86_lex_val;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include "configProcs.h"
#include <stdlib.h>

#define TestFree(a) if (a) { free ((void *) a); a = NULL; }

#define parsePrologue(typeptr,typerec) typeptr ptr; \
if( (ptr=calloc(1,sizeof(typerec))) == NULL ) { return NULL; }

#define HANDLE_RETURN(f,func)\
if ((ptr->f=func) == NULL)\
{\
	CLEANUP (ptr);\
	return NULL;\
}

#define HANDLE_LIST(field,func,type)\
{\
type p = func ();\
if (p == NULL)\
{\
	CLEANUP (ptr);\
	return NULL;\
}\
else\
{\
	ptr->field = (type) xf86addListItem ((glp) ptr->field, (glp) p);\
}\
}

#define Error(...) do { \
		xf86parseError (__VA_ARGS__); CLEANUP (ptr); return NULL; \
		   } while (0)

/*
 * These are defines for error messages to promote consistency.
 * Error messages are preceded by the line number, section and file name,
 * so these messages should be about the specific keyword and syntax in error.
 * To help limit namespace pollution, end each with _MSG.
 * Limit messages to 70 characters if possible.
 */

#define BAD_OPTION_MSG \
"The Option keyword requires 1 or 2 quoted strings to follow it."
#define INVALID_KEYWORD_MSG \
"\"%s\" is not a valid keyword in this section."
#define INVALID_SECTION_MSG \
"\"%s\" is not a valid section name."
#define UNEXPECTED_EOF_MSG \
"Unexpected EOF. Missing EndSection keyword?"
#define QUOTE_MSG \
"The %s keyword requires a quoted string to follow it."
#define NUMBER_MSG \
"The %s keyword requires a number to follow it."
#define POSITIVE_INT_MSG \
"The %s keyword requires a positive integer to follow it."
#define BOOL_MSG \
"The %s keyword requires a boolean to follow it."
#define ZAXISMAPPING_MSG \
"The ZAxisMapping keyword requires 2 positive numbers or X or Y to follow it."
#define DACSPEED_MSG \
"The DacSpeed keyword must be followed by a list of up to %d numbers."
#define DISPLAYSIZE_MSG \
"The DisplaySize keyword must be followed by the width and height in mm."
#define HORIZSYNC_MSG \
"The HorizSync keyword must be followed by a list of numbers or ranges."
#define VERTREFRESH_MSG \
"The VertRefresh keyword must be followed by a list of numbers or ranges."
#define VIEWPORT_MSG \
"The Viewport keyword must be followed by an X and Y value."
#define VIRTUAL_MSG \
"The Virtual keyword must be followed by a width and height value."
#define WEIGHT_MSG \
"The Weight keyword must be followed by red, green and blue values."
#define BLACK_MSG \
"The Black keyword must be followed by red, green and blue values."
#define WHITE_MSG \
"The White keyword must be followed by red, green and blue values."
#define SCREEN_MSG \
"The Screen keyword must be followed by an optional number, a screen name\n" \
"\tin quotes, and optional position/layout information."
#define INVALID_SCR_MSG \
"Invalid Screen line."
#define INPUTDEV_MSG \
"The InputDevice keyword must be followed by an input device name in quotes."
#define INACTIVE_MSG \
"The Inactive keyword must be followed by a Device name in quotes."
#define UNDEFINED_SCREEN_MSG \
"Undefined Screen \"%s\" referenced by ServerLayout \"%s\"."
#define UNDEFINED_MODES_MSG \
"Undefined Modes Section \"%s\" referenced by Monitor \"%s\"."
#define UNDEFINED_DEVICE_MSG \
"Undefined Device \"%s\" referenced by Screen \"%s\"."
#define UNDEFINED_ADAPTOR_MSG \
"Undefined VideoAdaptor \"%s\" referenced by Screen \"%s\"."
#define ADAPTOR_REF_TWICE_MSG \
"VideoAdaptor \"%s\" already referenced by Screen \"%s\"."
#define UNDEFINED_DEVICE_LAY_MSG \
"Undefined Device \"%s\" referenced by ServerLayout \"%s\"."
#define UNDEFINED_INPUT_MSG \
"Undefined InputDevice \"%s\" referenced by ServerLayout \"%s\"."
#define NO_IDENT_MSG \
"This section must have an Identifier line."
#define ONLY_ONE_MSG \
"This section must have only one of either %s line."
#define UNDEFINED_INPUTDRIVER_MSG \
"InputDevice section \"%s\" must have a Driver line."
#define INVALID_GAMMA_MSG \
"gamma correction value(s) expected\n either one value or three r/g/b values."
#define GROUP_MSG \
"The Group keyword must be followed by either a group name in quotes or\n" \
"\ta numerical group id."
#define MULTIPLE_MSG \
"Multiple \"%s\" lines."
#define MUST_BE_OCTAL_MSG \
"The number \"%d\" given in this section must be in octal (0xxx) format."
#define GPU_DEVICE_TOO_MANY \
"More than %d GPU devices defined."

/* Warning messages */
#define OBSOLETE_MSG \
"Ignoring obsolete keyword \"%s\"."

#endif                          /* _Configint_h_ */
