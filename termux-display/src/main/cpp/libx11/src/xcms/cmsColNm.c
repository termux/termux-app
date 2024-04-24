
/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 *
 * This file is a component of an X Window System-specific implementation
 * of Xcms based on the TekColor Color Management System.  Permission is
 * hereby granted to use, copy, modify, sell, and otherwise distribute this
 * software and its documentation for any purpose and without fee, provided
 * that this copyright, permission, and disclaimer notice is reproduced in
 * all copies of this software and in supporting documentation.  TekColor
 * is a trademark of Tektronix, Inc.
 *
 * Tektronix makes no representation about the suitability of this software
 * for any purpose.  It is provided "as is" and with all faults.
 *
 * TEKTRONIX DISCLAIMS ALL WARRANTIES APPLICABLE TO THIS SOFTWARE,
 * INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL TEKTRONIX BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA, OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR THE PERFORMANCE OF THIS SOFTWARE.
 *
 *	NAME
 *		XcmsColNm.c
 *
 *	DESCRIPTION
 *		Source for _XcmsLookupColorName().
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"
#include <X11/Xos.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#define XK_LATIN1
#include <X11/keysymdef.h>
#include "Cv.h"

/* forwards/locals */
static Status LoadColornameDB(void);


/*
 *      LOCAL DEFINES
 *		#define declarations local to this package.
 */
#ifndef XCMSDB
#define XCMSDB  XCMSDIR "/Xcms.txt"
#endif

#ifndef isgraph
#  define isgraph(c)	(isprint((c)) && !isspace((c)))
#endif

#ifndef XCMSDB_MAXLINELEN
#  define XCMSDB_MAXLINELEN	256
#endif

#define FORMAT_VERSION	"0.1"
#define START_TOKEN	"XCMS_COLORDB_START"
#define END_TOKEN	"XCMS_COLORDB_END"
#define DELIM_CHAR	'\t'

#define	NOT_VISITED	0x0
#define	VISITED		0x1
#define CYCLE		0xFFFF
#define XcmsDbInitNone		-1
#define XcmsDbInitFailure	0
#define XcmsDbInitSuccess	1

/*
 *      LOCAL TYPEDEFS
 */
typedef struct _XcmsPair {
    const char *first;
    const char *second;
    int flag;
} XcmsPair;

/*
 *      LOCAL VARIABLES
 */
static int XcmsColorDbState = XcmsDbInitNone;
static int nEntries;
static char *strings;
static XcmsPair *pairs;
static const char whitePtStr[] = "WhitePoint";


/************************************************************************
 *									*
 *			PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		_XcmsColorSpaceOfString
 *
 *	SYNOPSIS
 */
static XcmsColorSpace *
_XcmsColorSpaceOfString(
    XcmsCCC ccc,
    const char *color_string)
/*
 *	DESCRIPTION
 *		Returns a pointer to the color space structure
 *		(XcmsColorSpace) associated with the specified color string.
 *
 *	RETURNS
 *		Pointer to matching XcmsColorSpace structure if found;
 *		otherwise NULL.
 *
 *	CAVEATS
 *
 */
{
    XcmsColorSpace	**papColorSpaces;
    size_t n;
    char *pchar;

    if ((pchar = strchr(color_string, ':')) == NULL) {
	return(XcmsFailure);
    }
    n = (size_t)(pchar - color_string);

    if (ccc == NULL) {
	return(NULL);
    }

    /*
     * First try Device-Independent color spaces
     */
    papColorSpaces = _XcmsDIColorSpaces;
    if (papColorSpaces != NULL) {
	while (*papColorSpaces != NULL) {
	    if (strncmp((*papColorSpaces)->prefix, color_string, n) == 0 &&
		!((*papColorSpaces)->prefix)[n]) {
		return(*papColorSpaces);
	    }
	    papColorSpaces++;
	}
    }

    /*
     * Next try Device-Dependent color spaces
     */
    papColorSpaces = ((XcmsFunctionSet *)ccc->pPerScrnInfo->functionSet)->DDColorSpaces;
    if (papColorSpaces != NULL) {
	while (*papColorSpaces != NULL) {
	    if (strncmp((*papColorSpaces)->prefix, color_string, n) == 0 &&
		!((*papColorSpaces)->prefix)[n]) {
		return(*papColorSpaces);
	    }
	    papColorSpaces++;
	}
    }

    return(NULL);
}


/*
 *	NAME
 *		_XcmsParseColorString
 *
 *	SYNOPSIS
 */
static int
_XcmsParseColorString(
    XcmsCCC ccc,
    const char *color_string,
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		Assuming color_string contains a numerical string color
 *		specification, attempts to parse a string into an
 *		XcmsColor structure.
 *
 *	RETURNS
 *		0 if failed; otherwise non-zero.
 *
 *	CAVEATS
 *		A color string containing a numerical color specification
 *		must be in ISO Latin-1 encoding!
 */
{
    XcmsColorSpace	*pColorSpace;
    char		string_buf[64];
    char		*string_lowered;
    size_t		len;
    int			res;

    if (ccc == NULL) {
	return(0);
    }

    /*
     * While copying color_string to string_lowered, convert to lowercase
     */
    if ((len = strlen(color_string)) >= sizeof(string_buf)) {
	string_lowered = Xmalloc(len+1);
	if (string_lowered == NULL)
	    return(XcmsFailure);
    } else {
	string_lowered = string_buf;
    }

    _XcmsCopyISOLatin1Lowered(string_lowered, color_string);

    if (*string_lowered == '#') {
	if ((pColorSpace = _XcmsColorSpaceOfString(ccc, "rgb:")) != NULL) {
	    res = (*pColorSpace->parseString)(string_lowered, pColor);
	    if (len >= sizeof(string_buf)) Xfree(string_lowered);
	    return res;
	}
    }

    if ((pColorSpace = _XcmsColorSpaceOfString(ccc, string_lowered)) != NULL) {
	res = (*pColorSpace->parseString)(string_lowered, pColor);
	if (len >= sizeof(string_buf)) Xfree(string_lowered);
	return res;
    }

    if (len >= sizeof(string_buf)) Xfree(string_lowered);
    return(0);
}


/*
 *	NAME
 *		FirstCmp - Compare color names of pair recs
 *
 *	SYNOPSIS
 */
static int
FirstCmp(const void *p1, const void *p2)
/*
 *	DESCRIPTION
 *		Compares the color names of XcmsColorTuples.
 *		This routine is public to allow access from qsort???.
 *
 *	RETURNS
 *		0 if equal;
 *		< 0 if first precedes second,
 *		> 0 if first succeeds second.
 *
 */
{
    return(strcmp(((const XcmsPair *)p1)->first, ((const XcmsPair *)p2)->first));
}



/*
 *	NAME
 *		stringSectionSize - determine memory needed for strings
 *
 *	SYNOPSIS
 */
static void
SetNoVisit(void)
/*
 *	DESCRIPTION
 *
 *	RETURNS
 *		void
 *
 */
{
    int i;
    XcmsPair *pair = pairs;

    for (i = 0; i < nEntries; i++, pair++) {
	if (pair->flag != CYCLE) {
	    pair->flag = NOT_VISITED;
	}
    }
}




/*
 *	NAME
 *		field2 - extract two fields
 *
 *	SYNOPSIS
 */
static int
field2(
    char *pBuf,
    char delim,	/* in:  field delimiter */
    char **p1,	/* in/out: pointer to pointer to field 1 */
    char **p2)	/* in/out: pointer to pointer to field 2 */
/*
 *	DESCRIPTION
 *		Extracts two fields from a "record".
 *
 *	RETURNS
 *		XcmsSuccess if succeeded, otherwise XcmsFailure.
 *
 */
{
    *p1 = *p2 = NULL;

    /* Find Field 1 */
    while (!isgraph(*pBuf)) {
	if ((*pBuf == '\n') || (*pBuf == '\0')) {
	    return(XcmsFailure);
	}
	if (isspace(*pBuf) || (*pBuf == delim)) {
	    pBuf++;
	}
    }
    *p1 = pBuf;

    /* Find end of Field 2 */
    while (isprint(*pBuf) && (*pBuf != delim)) {
	pBuf++;
    }
    if ((*pBuf == '\n') || (*pBuf == '\0')) {
	return(XcmsFailure);
    }
    if ((*pBuf == ' ') || (*pBuf == delim)) {
	*pBuf++ = '\0';	/* stuff end of string character */
    } else {
	return(XcmsFailure);
    }

    /* Find Field 2 */
    while (!isgraph(*pBuf)) {
	if ((*pBuf == '\n') || (*pBuf == '\0')) {
	    return(XcmsFailure);
	}
	if (isspace(*pBuf) || (*pBuf == delim)) {
	    pBuf++;
	}
    }
    *p2 = pBuf;

    /* Find end of Field 2 */
    while (isprint(*pBuf) && (*pBuf != delim)) {
	pBuf++;
    }
    if (*pBuf != '\0') {
	*pBuf = '\0';	/* stuff end of string character */
    }

    return(XcmsSuccess);
}


/*
 *	NAME
 *		_XcmsLookupColorName - Lookup DB entry for a color name
 *
 *	SYNOPSIS
 */
static Status
_XcmsLookupColorName(
    XcmsCCC ccc,
    const char **name,
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		Searches for an entry in the Device-Independent Color Name
 *		Database for the specified string.
 *
 *	RETURNS
 *		XcmsFailure if failed to find a matching entry in
 *			the database.
 *		XcmsSuccess if succeeded in converting color name to
 *			XcmsColor.
 *		_XCMS_NEWNAME if succeeded in converting color string (which
 *			is a color name to yet another color name.  Note
 *			that the new name is passed back via 'name'.
 */
 {
    Status		retval = 0;
    char		name_lowered_64[64];
    char		*name_lowered;
    register int	i, j, left, right;
    int			len;
    const char		*tmpName;
    XcmsPair		*pair = NULL;

    /*
     * Check state of Database:
     *		XcmsDbInitNone
     *		XcmsDbInitSuccess
     *		XcmsDbInitFailure
     */
    if (XcmsColorDbState == XcmsDbInitFailure) {
	return(XcmsFailure);
    }
    if (XcmsColorDbState == XcmsDbInitNone) {
	if (!LoadColornameDB()) {
	    return(XcmsFailure);
	}
    }

    SetNoVisit();

    /*
     * While copying name to name_lowered, convert to lowercase
     */

    tmpName = *name;

Retry:
    if ((len = (int)strlen(tmpName)) > 63) {
	name_lowered = Xmalloc(len+1);
	if (name_lowered == NULL)
	    return(XcmsFailure);
    } else {
	name_lowered = name_lowered_64;
    }

    _XcmsCopyISOLatin1Lowered(name_lowered, tmpName);

    /*
     * Now, remove spaces.
     */
    for (i = 0, j = 0; j < len; j++) {
	if (!isspace(name_lowered[j])) {
	    name_lowered[i++] = name_lowered[j];
	}
    }
    name_lowered[i] = '\0';

    left = 0;
    right = nEntries - 1;
    while (left <= right) {
	i = (left + right) >> 1;
	pair = &pairs[i];
	j = strcmp(name_lowered, pair->first);
	if (j < 0)
	    right = i - 1;
	else if (j > 0)
	    left = i + 1;
	else {
	    break;
	}
    }
    if (len > 63) Xfree(name_lowered);

    if (left > right) {
	if (retval == 2) {
	    if (*name != tmpName) {
		*name = tmpName;
	    }
	    return(_XCMS_NEWNAME);
	}
	return(XcmsFailure);
    }

    if (pair->flag == CYCLE) {
	return(XcmsFailure);
    }
    if (pair->flag == VISITED) {
	pair->flag = CYCLE;
	return(XcmsFailure);
    }

    if (_XcmsParseColorString(ccc, pair->second, pColor) == XcmsSuccess) {
	/* f2 contains a numerical string specification */
	return(XcmsSuccess);
    } else {
	/* f2 does not contain a numerical string specification */
	tmpName = pair->second;
	pair->flag = VISITED;
	retval = 2;
	goto Retry;
    }
}


/*
 *	NAME
 *		RemoveSpaces
 *
 *	SYNOPSIS
 */
static int
RemoveSpaces(
    char *pString)
/*
 *	DESCRIPTION
 *		Removes spaces from string.
 *
 *	RETURNS
 *		Void
 *
 */
{
    int i, count = 0;
    char *cptr;

    /* REMOVE SPACES */
    cptr = pString;
    for (i = (int)strlen(pString); i; i--, cptr++) {
	if (!isspace(*cptr)) {
	    *pString++ = *cptr;
	    count++;
	}
    }
    *pString = '\0';
    return(count);
}


/*
 *	NAME
 *		stringSectionSize - determine memory needed for strings
 *
 *	SYNOPSIS
 */
static int
stringSectionSize(
    FILE *stream,
    int	*pNumEntries,
    int	*pSectionSize)
/*
 *	DESCRIPTION
 *		Determines the amount of memory required to store the
 *		color name strings and also the number of strings.
 *
 *	RETURNS
 *		XcmsSuccess if succeeded, otherwise XcmsFailure.
 *
 */
{
    char buf[XCMSDB_MAXLINELEN];
    char token[XCMSDB_MAXLINELEN];
    char token2[XCMSDB_MAXLINELEN];
    char *pBuf;
    char *f1;
    char *f2;
    size_t i;

    unsigned int numEntries = 0;
    unsigned int sectionSize = 0;

    *pNumEntries = 0;
    *pSectionSize = 0;

    /*
     * Advance to START_TOKEN
     *	 Anything before is just considered as comments.
     */

    while((pBuf = fgets(buf, XCMSDB_MAXLINELEN, stream)) != NULL) {
	if ((sscanf(buf, "%s %s", token, token2))
		&& (strcmp(token, START_TOKEN) == 0)) {
	    if (strcmp(token2, FORMAT_VERSION) != 0) {
		/* text file not in the right format */
		return(XcmsFailure);
	    }
	    break;
	} /* else it was just a blank line or comment */
    }

    if (pBuf == NULL) {
	return(XcmsFailure);
    }

    while((fgets(buf, XCMSDB_MAXLINELEN, stream)) != NULL) {
	if ((sscanf(buf, "%s", token)) && (strcmp(token, END_TOKEN) == 0)) {
	    break;
	}

	if (field2(buf, DELIM_CHAR, &f1, &f2) != XcmsSuccess) {
	    return(XcmsFailure);
	}

	numEntries++;
	if (numEntries >= INT_MAX)
	    return(XcmsFailure);

	i = strlen(f1);
	if (i >= INT_MAX - sectionSize)
	    return(XcmsFailure);
	sectionSize += i + 1;
	for (; i; i--, f1++) {
	    /* REMOVE SPACES FROM COUNT */
	    if (isspace(*f1)) {
		sectionSize--;
	    }
	}

	i = strlen(f2);
	if (i >= INT_MAX - sectionSize)
	    return(XcmsFailure);
	sectionSize += i + 1;
	for (; i; i--, f2++) {
	    /* REMOVE SPACES FROM COUNT */
	    if (isspace(*f2)) {
		sectionSize--;
	    }
	}

    }

    *pNumEntries = (int) numEntries;
    *pSectionSize = (int) sectionSize;

    return(XcmsSuccess);
}


/*
 *	NAME
 *		ReadColornameDB - Read the Color Name Database
 *
 *	SYNOPSIS
 */
static Status
ReadColornameDB(
    FILE *stream,
    XcmsPair *pRec,
    char *pString)
/*
 *	DESCRIPTION
 *		Loads the Color Name Database from a text file.
 *
 *	RETURNS
 *		XcmsSuccess if succeeded, otherwise XcmsFailure.
 *
 */
{
    char buf[XCMSDB_MAXLINELEN];
    char token[XCMSDB_MAXLINELEN];
    char token2[XCMSDB_MAXLINELEN];
    char *f1;
    char *f2;
    char *pBuf;

    /*
     * Advance to START_TOKEN
     *	 Anything before is just considered as comments.
     */

    while((pBuf = fgets(buf, XCMSDB_MAXLINELEN, stream)) != NULL) {
	if ((sscanf(buf, "%s %s", token, token2))
		&& (strcmp(token, START_TOKEN) == 0)) {
	    if (strcmp(token2, FORMAT_VERSION) != 0) {
		/* text file not in the right format */
		return(XcmsFailure);
	    }
	    break;
	} /* else it was just a blank line or comment */
    }

    if (pBuf == NULL) {
	return(XcmsFailure);
    }

    /*
     * Process lines between START_TOKEN to END_TOKEN
     */

    while ((fgets(buf, XCMSDB_MAXLINELEN, stream)) != NULL) {
	if ((sscanf(buf, "%s", token)) && (strcmp(token, END_TOKEN) == 0)) {
	    /*
	     * Found END_TOKEN so break out of for loop
	     */
	    break;
	}

	/*
	 * Get pairs
	 */
	if (field2(buf, DELIM_CHAR, &f1, &f2) != XcmsSuccess) {
	    /* Invalid line */
	    continue;
	}

	/*
	 * Add strings
	 */

	/* Left String */
	pRec->first = pString;
	_XcmsCopyISOLatin1Lowered(pString, f1);
	pString += (1 + RemoveSpaces(pString));
	pRec->second = pString;
	/* Right String */
	_XcmsCopyISOLatin1Lowered(pString, f2);
	pString += RemoveSpaces(pString) + 1;
	pRec++;

    }

    return(XcmsSuccess);
}


/*
 *	NAME
 *		LoadColornameDB - Load the Color Name Database
 *
 *	SYNOPSIS
 */
static Status
LoadColornameDB(void)
/*
 *	DESCRIPTION
 *		Loads the Color Name Database from a text file.
 *
 *	RETURNS
 *		XcmsSuccess if succeeded, otherwise XcmsFailure.
 *
 */
{
    int size;
    FILE *stream;
    const char *pathname;
    struct stat txt;
    int length;

    /* use and name of this env var is not part of the standard */
    /* implementation-dependent feature */
    if ((pathname = getenv("XCMSDB")) == NULL) {
	pathname = XCMSDB;
    }
#ifdef __UNIXOS2__
    pathname = __XOS2RedirRoot(pathname);
#endif

    length = (int)strlen(pathname);
    if ((length == 0) || (length >= (BUFSIZ - 5))){
	XcmsColorDbState = XcmsDbInitFailure;
	return(XcmsFailure);
    }

    if (stat(pathname, &txt)) {
	/* can't stat file */
	XcmsColorDbState = XcmsDbInitFailure;
	return(XcmsFailure);
    }

    if ((stream = _XFopenFile (pathname, "r")) == NULL) {
	/* can't open file */
	XcmsColorDbState = XcmsDbInitFailure;
	return(XcmsFailure);
    }

    if (stringSectionSize(stream, &nEntries, &size) != XcmsSuccess ||
	nEntries == 0) {
	(void) fclose(stream);
	XcmsColorDbState = XcmsDbInitFailure;
	return(XcmsFailure);
    }
    rewind(stream);

    strings = Xmalloc(size);
    pairs = Xcalloc(nEntries, sizeof(XcmsPair));
    if (strings == NULL || pairs == NULL) {
	free(strings);
	free(pairs);
	(void) fclose(stream);
	XcmsColorDbState = XcmsDbInitFailure;
	return(XcmsFailure);
    }

    ReadColornameDB(stream, pairs, strings);
    (void) fclose(stream);

    /*
     * sort the pair recs
     */
    qsort((char *)pairs, nEntries, sizeof(XcmsPair), FirstCmp);

    XcmsColorDbState = XcmsDbInitSuccess;
    return(XcmsSuccess);
}


/************************************************************************
 *									*
 *			API PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		_XcmsCopyISOLatin1Lowered
 *
 *	SYNOPSIS
 */
void
_XcmsCopyISOLatin1Lowered(
    char *dst,
    const char *src)
/*
 *	DESCRIPTION
 *		ISO Latin-1 case conversion routine
 *		Identical to XmuCopyISOLatin1Lowered() but provided here
 *		to eliminate need to link with libXmu.a.
 *
 *		IMPLEMENTORS NOTE:
 *		    This routine is also used in XcmsFormatOfPrefix.
 *
 *	RETURNS
 *		Void
 *
 */
{
    register unsigned char *dest;
    register const unsigned char *source;

    for (dest = (unsigned char *)dst, source = (const unsigned char *)src;
	 *source;
	 source++, dest++)
    {
	if ((*source >= XK_A) && (*source <= XK_Z))
	    *dest = *source + (XK_a - XK_A);
	else if ((*source >= XK_Agrave) && (*source <= XK_Odiaeresis))
	    *dest = *source + (XK_agrave - XK_Agrave);
	else if ((*source >= XK_Ooblique) && (*source <= XK_Thorn))
	    *dest = *source + (XK_oslash - XK_Ooblique);
	else
	    *dest = *source;
    }
    *dest = '\0';
}


/*
 *	NAME
 *		_XcmsResolveColorString -
 *
 *	SYNOPSIS
 */
Status
_XcmsResolveColorString (
    XcmsCCC ccc,
    const char **color_string,
    XcmsColor *pColor_exact_return,
    XcmsColorFormat result_format)
/*
 *	DESCRIPTION
 *		The XcmsLookupColor function finds the color specification
 *		associated with a color name in the Device-Independent Color
 *		Name Database.
 *	RETURNS
 *		XcmsFailure if failed to convert valid color string.
 *		XcmsSuccess if succeeded in converting color string to
 *			XcmsColor.
 *		_XCMS_NEWNAME if failed to parse the string or find it in
 *			the database, or if succeeded in looking it up and
 *			found another name which is not in the database.
 *			Note that the new name is returned in color_string.
 *
 *		This function returns both the color specification found in the
 *		database (db specification) and the color specification for the
 *		color displayable by the specified screen (screen
 *		specification).  The calling routine sets the format for these
 *		returned specifications in the XcmsColor format component.
 *		If XcmsUndefinedFormat, the specification is returned in the
 *		format used to store the color in the database.
 */
{
    XcmsColor dbWhitePt;	/* whitePt associated with pColor_exact_return*/
				/*    the screen's white point */
    XcmsColor *pClientWhitePt;
    int retval;
    const char *strptr = whitePtStr;

/*
 * 0. Check for invalid arguments.
 */
    if (ccc == NULL || (*color_string)[0] == '\0' || pColor_exact_return == NULL) {
	return(XcmsFailure);
    }

/*
 * 1. First attempt to parse the string
 *    If successful, then convert the specification to the target format
 *    and return.
 */
    if (_XcmsParseColorString(ccc, *color_string, pColor_exact_return)
	    == 1) {
	if (result_format != XcmsUndefinedFormat
		&& pColor_exact_return->format != result_format) {
	    /* need to be converted to the target format */
	    return(XcmsConvertColors(ccc, pColor_exact_return, 1,
		    result_format, (Bool *)NULL));
	} else {
	    return(XcmsSuccess);
	}
    }

/*
 * 2. Attempt to find it in the DI Color Name Database
 */

    /*
     * a. Convert String into a XcmsColor structure
     *       Attempt to extract the specification for color_string from the
     *       DI Database (pColor_exact_return).  If the DI Database does not
     *	     have this entry, then return failure.
     */
    retval = _XcmsLookupColorName(ccc, color_string, pColor_exact_return);

    if (retval != XcmsSuccess) {
	/* color_string replaced with a color name, or not found */
	return(_XCMS_NEWNAME);
    }

    if (pColor_exact_return->format == XcmsUndefinedFormat) {
	return(XcmsFailure);
    }

    /*
     * b. If result_format not defined, then assume target format
     *	  is the exact format.
     */
    if (result_format == XcmsUndefinedFormat) {
	result_format = pColor_exact_return->format;
    }

    if ((ClientWhitePointOfCCC(ccc))->format == XcmsUndefinedFormat) {
	pClientWhitePt = ScreenWhitePointOfCCC(ccc);
    } else {
	pClientWhitePt = ClientWhitePointOfCCC(ccc);
    }

    /*
     * c. Convert to the target format, making adjustments for white
     *	  point differences as necessary.
     */
    if (XCMS_DD_ID(pColor_exact_return->format)) {
	/*
	 * The spec format is Device-Dependent, therefore assume the
	 *    its white point is the Screen White Point.
	 */
	if (XCMS_DD_ID(result_format)) {
	    /*
	     * Target format is Device-Dependent
	     *	Therefore, DD --> DD conversion
	     */
	    return(_XcmsDDConvertColors(ccc, pColor_exact_return,
		    1, result_format, (Bool *) NULL));
	} else {
	    /*
	     * Target format is Device-Independent
	     *	Therefore, DD --> DI conversion
	     */
	    if (ccc->whitePtAdjProc && !_XcmsEqualWhitePts(ccc,
		    pClientWhitePt, ScreenWhitePointOfCCC(ccc))) {
		return((*ccc->whitePtAdjProc)(ccc, ScreenWhitePointOfCCC(ccc),
			pClientWhitePt, result_format,
			pColor_exact_return, 1, (Bool *) NULL));
	    } else {
		if (_XcmsDDConvertColors(ccc, pColor_exact_return, 1,
			XcmsCIEXYZFormat, (Bool *) NULL) == XcmsFailure) {
		    return(XcmsFailure);
		}
		return(_XcmsDIConvertColors(ccc, pColor_exact_return,
			pClientWhitePt, 1, result_format));
	    }
	}
    } else {
	/*
	 * The spec format is Device-Independent, therefore attempt
	 * to find a database white point.
	 *
	 * If the Database does not have a white point, then assume the
	 * database white point is the same as the Screen White Point.
	 */

	if (_XcmsLookupColorName(ccc, &strptr, &dbWhitePt) != 1) {
	    memcpy((char *)&dbWhitePt,
		   (char *)&ccc->pPerScrnInfo->screenWhitePt,
		   sizeof(XcmsColor));
	}
	if (XCMS_DD_ID(result_format)) {
	    /*
	     * Target format is Device-Dependent
	     *	Therefore, DI --> DD conversion
	     */
	    if (ccc->whitePtAdjProc && !_XcmsEqualWhitePts(ccc,
		    &dbWhitePt, ScreenWhitePointOfCCC(ccc))) {
		return((*ccc->whitePtAdjProc)(ccc, &dbWhitePt,
			ScreenWhitePointOfCCC(ccc), result_format,
			pColor_exact_return, 1, (Bool *)NULL));
	    } else {
		if (pColor_exact_return->format != XcmsCIEXYZFormat) {
		    if (_XcmsDIConvertColors(ccc, pColor_exact_return,
			    &dbWhitePt, 1, XcmsCIEXYZFormat) == XcmsFailure) {
			return(XcmsFailure);
		    }
		}
		return (_XcmsDDConvertColors(ccc, pColor_exact_return, 1,
			result_format, (Bool *)NULL));
	    }
	} else {
	    /*
	     * Target format is Device-Independent
	     *	Therefore, DI --> DI conversion
	     */
	    if (ccc->whitePtAdjProc && !_XcmsEqualWhitePts(ccc,
		    &dbWhitePt, pClientWhitePt)) {
		/*
		 * The calling routine wants to resolve this color
		 * in terms if it's white point (i.e. Client White Point).
		 * Therefore, apply white adjustment for the displacement
		 * between dbWhitePt to clientWhitePt.
		 */
		return((*ccc->whitePtAdjProc)(ccc, &dbWhitePt,
			pClientWhitePt, result_format,
			pColor_exact_return, 1, (Bool *)NULL));
	    } else if (_XcmsEqualWhitePts(ccc,
		    &dbWhitePt, pClientWhitePt)) {
		/*
		 * Can use either dbWhitePt or pClientWhitePt to
		 * convert to the result_format.
		 */
		if (pColor_exact_return->format == result_format) {
		    return(XcmsSuccess);
		} else {
		    return (_XcmsDIConvertColors(ccc, pColor_exact_return,
			    &dbWhitePt, 1, result_format));
		}
	    } else {
		/*
		 * Need to convert to a white point independent color
		 * space (let's choose CIEXYZ) then convert to the
		 * target color space.  Why? Lets assume that
		 * pColor_exact_return->format and result format
		 * are white point dependent format (e.g., CIELUV, CIELAB,
		 * TekHVC ... same or any combination). If so, we'll
		 * need to convert the color with dbWhitePt to an absolute
		 * spec (i.e.  non-white point dependent) then convert that
		 * absolute value with clientWhitePt to the result_format.
		 */
		if (pColor_exact_return->format != XcmsCIEXYZFormat) {
		    if (_XcmsDIConvertColors(ccc, pColor_exact_return,
			    &dbWhitePt, 1, XcmsCIEXYZFormat) == XcmsFailure) {
			return(XcmsFailure);
		    }
		}
		if (result_format == XcmsCIEXYZFormat) {
		    return(XcmsSuccess);
		} else {
		    return(_XcmsDIConvertColors(ccc, pColor_exact_return,
			    pClientWhitePt, 1, result_format));
		}
	    }
	}
    }
}
