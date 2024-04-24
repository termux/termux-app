/************************************************************************
Copyright 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

/*

Copyright 1994, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include <X11/fonts/fntfilst.h>
#include <X11/fonts/fontstruct.h>
/* use bitmap structure */
#include <X11/fonts/bitmap.h>
#include <X11/fonts/bdfint.h>

int bdfFileLineNum;

/***====================================================================***/

void
bdfError(const char* message, ...)
{
    va_list args;

    va_start (args, message);
    fprintf(stderr, "BDF Error on line %d: ", bdfFileLineNum);
    vfprintf(stderr, message, args);
    va_end (args);
}

/***====================================================================***/

void
bdfWarning(const char *message, ...)
{
    va_list args;

    va_start (args, message);
    fprintf(stderr, "BDF Warning on line %d: ", bdfFileLineNum);
    vfprintf(stderr, message, args);
    va_end (args);
}

/*
 * read the next (non-comment) line and keep a count for error messages.
 * Returns buf, or NULL if EOF.
 */

unsigned char *
bdfGetLine(FontFilePtr file, unsigned char *buf, int len)
{
    int         c;
    unsigned char *b;

    for (;;) {
	b = buf;
	while ((c = FontFileGetc(file)) != FontFileEOF) {
	    if (c == '\r')
		continue;
	    if (c == '\n') {
		bdfFileLineNum++;
		break;
	    }
	    if (b - buf >= (len - 1))
		break;
	    *b++ = c;
	}
	*b = '\0';
	if (c == FontFileEOF)
	    return NULL;
	if (b != buf && !bdfIsPrefix(buf, "COMMENT"))
	    break;
    }
    return buf;
}

/***====================================================================***/

Atom
bdfForceMakeAtom(const char *str, int *size)
{
    register int len = strlen(str);
    Atom the_atom;

    if (size != NULL)
	*size += len + 1;
    the_atom = MakeAtom(str, len, TRUE);
    if (the_atom == None)
      bdfError("Atom allocation failed\n");
    return the_atom;
}

/***====================================================================***/

/*
 * Handle quoted strings.
 */

Atom
bdfGetPropertyValue(char *s)
{
    register char *p,
               *pp;
    char *orig_s = s;
    Atom        atom;

    /* strip leading white space */
    while (*s && (*s == ' ' || *s == '\t'))
	s++;
    if (*s == 0) {
	return bdfForceMakeAtom(s, NULL);
    }
    if (*s != '"') {
	pp = s;
	/* no white space in value */
	for (pp = s; *pp; pp++)
	    if (*pp == ' ' || *pp == '\t' || *pp == '\015' || *pp == '\n') {
		*pp = 0;
		break;
	    }
	return bdfForceMakeAtom(s, NULL);
    }
    /* quoted string: strip outer quotes and undouble inner quotes */
    s++;
    pp = p = malloc((unsigned) strlen(s) + 1);
    if (pp == NULL) {
	bdfError("Couldn't allocate property value string (%d)\n",
		 (int) strlen(s) + 1);
	return None;
    }
    while (*s) {
	if (*s == '"') {
	    if (*(s + 1) != '"') {
		*p++ = 0;
		atom = bdfForceMakeAtom(pp, NULL);
		free(pp);
		return atom;
	    } else {
		s++;
	    }
	}
	*p++ = *s++;
    }
    free (pp);
    bdfError("unterminated quoted string property: %s\n", orig_s);
    return None;
}

/***====================================================================***/

/*
 * return TRUE if string is a valid integer
 */
int
bdfIsInteger(char *str)
{
    char        c;

    c = *str++;
    if (!(isdigit((unsigned char)c) || c == '-' || c == '+'))
	return (FALSE);

    while ((c = *str++))
	if (!isdigit((unsigned char)c))
	    return (FALSE);

    return (TRUE);
}

/***====================================================================***/

/*
 * make a byte from the first two hex characters in glyph picture
 */

unsigned char
bdfHexByte(unsigned char *s)
{
    unsigned char b = 0;
    register char c;
    int         i;

    for (i = 2; i; i--) {
	c = *s++;
	if ((c >= '0') && (c <= '9'))
	    b = (b << 4) + (c - '0');
	else if ((c >= 'A') && (c <= 'F'))
	    b = (b << 4) + 10 + (c - 'A');
	else if ((c >= 'a') && (c <= 'f'))
	    b = (b << 4) + 10 + (c - 'a');
	else
	    bdfError("bad hex char '%c'", c);
    }
    return b;
}

/***====================================================================***/

/*
 * check for known special property values
 */

static const char *SpecialAtoms[] = {
    "FONT_ASCENT",
#define BDF_FONT_ASCENT	0
    "FONT_DESCENT",
#define BDF_FONT_DESCENT 1
    "DEFAULT_CHAR",
#define BDF_DEFAULT_CHAR 2
    "POINT_SIZE",
#define BDF_POINT_SIZE 3
    "RESOLUTION",
#define BDF_RESOLUTION 4
    "X_HEIGHT",
#define BDF_X_HEIGHT 5
    "WEIGHT",
#define BDF_WEIGHT 6
    "QUAD_WIDTH",
#define BDF_QUAD_WIDTH 7
    "FONT",
#define BDF_FONT 8
    "RESOLUTION_X",
#define BDF_RESOLUTION_X 9
    "RESOLUTION_Y",
#define BDF_RESOLUTION_Y 10
    0,
};

Bool
bdfSpecialProperty(FontPtr pFont, FontPropPtr prop,
		   char isString, bdfFileState *bdfState)
{
    const char      **special;
    const char       *name;

    name = NameForAtom(prop->name);
    for (special = SpecialAtoms; *special; special++)
	if (!strcmp(name, *special))
	    break;

    switch (special - SpecialAtoms) {
    case BDF_FONT_ASCENT:
	if (!isString) {
	    pFont->info.fontAscent = prop->value;
	    bdfState->haveFontAscent = TRUE;
	}
	return TRUE;
    case BDF_FONT_DESCENT:
	if (!isString) {
	    pFont->info.fontDescent = prop->value;
	    bdfState->haveFontDescent = TRUE;
	}
	return TRUE;
    case BDF_DEFAULT_CHAR:
	if (!isString) {
	    pFont->info.defaultCh = prop->value;
	    bdfState->haveDefaultCh = TRUE;
	}
	return TRUE;
    case BDF_POINT_SIZE:
	bdfState->pointSizeProp = prop;
	return FALSE;
    case BDF_RESOLUTION:
	bdfState->resolutionProp = prop;
	return FALSE;
    case BDF_X_HEIGHT:
	bdfState->xHeightProp = prop;
	return FALSE;
    case BDF_WEIGHT:
	bdfState->weightProp = prop;
	return FALSE;
    case BDF_QUAD_WIDTH:
	bdfState->quadWidthProp = prop;
	return FALSE;
    case BDF_FONT:
	bdfState->fontProp = prop;
	return FALSE;
    case BDF_RESOLUTION_X:
	bdfState->resolutionXProp = prop;
	return FALSE;
    case BDF_RESOLUTION_Y:
	bdfState->resolutionYProp = prop;
	return FALSE;
    default:
	return FALSE;
    }
}
