/*

Copyright 1990, 1998  The Open Group

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

/*
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include	<X11/fonts/fontmisc.h>
#include	<X11/fonts/fontstruct.h>
#include	<X11/fonts/fontxlfd.h>
#include	<X11/fonts/fontutil.h>
#include	<X11/fonts/fntfilst.h> /* for MAXFONTNAMELEN */
#include	<X11/Xos.h>
#include	<math.h>
#include	<stdlib.h>
#if defined(sony) && !defined(SYSTYPE_SYSV) && !defined(_SYSTYPE_SYSV)
#define NO_LOCALE
#endif
#ifndef NO_LOCALE
#include	<locale.h>
#endif
#include	<ctype.h>
#include	<stdio.h>	/* for sprintf() */
#include	"src/util/replace.h"

static char *
GetInt(char *ptr, int *val)
{
    if (*ptr == '*') {
	*val = -1;
	ptr++;
    } else
	for (*val = 0; *ptr >= '0' && *ptr <= '9';)
	    *val = *val * 10 + *ptr++ - '0';
    if (*ptr == '-')
	return ptr;
    return (char *) 0;
}

#define minchar(p) ((p).min_char_low + ((p).min_char_high << 8))
#define maxchar(p) ((p).max_char_low + ((p).max_char_high << 8))


#ifndef NO_LOCALE
static struct lconv *locale = 0;
#endif
static const char *radix = ".", *plus = "+", *minus = "-";

static char *
readreal(char *ptr, double *result)
{
    char buffer[80], *p1, *p2;

#ifndef NO_LOCALE
    /* Figure out what symbols apply in this locale */

    if (!locale)
    {
	locale = localeconv();
	if (locale->decimal_point && *locale->decimal_point)
	    radix = locale->decimal_point;
	if (locale->positive_sign && *locale->positive_sign)
	    plus = locale->positive_sign;
	if (locale->negative_sign && *locale->negative_sign)
	    minus = locale->negative_sign;
    }
#endif
    /* Copy the first 80 chars of ptr into our local buffer, changing
       symbols as needed. */
    for (p1 = ptr, p2 = buffer;
	 *p1 && (p2 - buffer) < sizeof(buffer) - 1;
	 p1++, p2++)
    {
	switch(*p1)
	{
	    case '~': *p2 = *minus; break;
	    case '+': *p2 = *plus; break;
	    case '.': *p2 = *radix; break;
	    default: *p2 = *p1;
	}
    }
    *p2 = 0;

    /* Now we have something that strtod() can interpret... do it. */
    *result = strtod(buffer, &p1);
    /* Return NULL if failure, pointer past number if success */
    return (p1 == buffer) ? (char *)0 : (ptr + (p1 - buffer));
}

#define XLFD_DOUBLE_TO_TEXT_BUF_SIZE 80

static char *
xlfd_double_to_text(double value, char *buffer, int space_required)
{
    register char *p1;
    int ndigits, exponent;
    const size_t buflen = XLFD_DOUBLE_TO_TEXT_BUF_SIZE;

#ifndef NO_LOCALE
    if (!locale)
    {
	locale = localeconv();
	if (locale->decimal_point && *locale->decimal_point)
	    radix = locale->decimal_point;
	if (locale->positive_sign && *locale->positive_sign)
	    plus = locale->positive_sign;
	if (locale->negative_sign && *locale->negative_sign)
	    minus = locale->negative_sign;
    }
#endif

    if (space_required)
	*buffer++ = ' ';

    /* Render the number using printf's idea of formatting */
    snprintf(buffer, buflen, "%.*le", XLFD_NDIGITS, value);

    /* Find and read the exponent value */
    for (p1 = buffer + strlen(buffer);
	*p1-- != 'e' && p1[1] != 'E';);
    exponent = atoi(p1 + 2);
    if (value == 0.0) exponent = 0;

    /* Figure out how many digits are significant */
    while (p1 >= buffer && (!isdigit((unsigned char)*p1) || *p1 == '0')) p1--;
    ndigits = 0;
    while (p1 >= buffer) if (isdigit((unsigned char)*p1--)) ndigits++;

    /* Figure out notation to use */
    if (exponent >= XLFD_NDIGITS || ndigits - exponent > XLFD_NDIGITS + 1)
    {
	/* Scientific */
	snprintf(buffer, buflen, "%.*le", ndigits - 1, value);
    }
    else
    {
	/* Fixed */
	ndigits -= exponent + 1;
	if (ndigits < 0) ndigits = 0;
	snprintf(buffer, buflen, "%.*lf", ndigits, value);
	if (exponent < 0)
	{
	    p1 = buffer;
	    while (*p1 && *p1 != '0') p1++;
	    while (*p1++) p1[-1] = *p1;
	}
    }

    /* Last step, convert the locale-specific sign and radix characters
       to our own. */
    for (p1 = buffer; *p1; p1++)
    {
	if (*p1 == *minus) *p1 = '~';
	else if (*p1 == *plus) *p1 = '+';
	else if (*p1 == *radix) *p1 = '.';
    }

    return buffer - space_required;
}

double
xlfd_round_double(double x)
{
   /* Utility for XLFD users to round numbers to XLFD_NDIGITS
      significant digits.  How do you round to n significant digits on
      a binary machine?  */

#if defined(i386) || defined(__i386__) || \
    defined(ia64) || defined(__ia64__) || \
    defined(__alpha__) || defined(__alpha) || \
    defined(__hppa__) || \
    defined(__amd64__) || defined(__amd64) || \
    defined(sgi)
#include <float.h>

/* if we have IEEE 754 fp, we can round to binary digits... */

#if (FLT_RADIX == 2) && (DBL_DIG == 15) && (DBL_MANT_DIG == 53)

#ifndef M_LN2
#define M_LN2       0.69314718055994530942
#endif
#ifndef M_LN10
#define M_LN10      2.30258509299404568402
#endif

/* convert # of decimal digits to # of binary digits */
#define XLFD_NDIGITS_2 ((int)(XLFD_NDIGITS * M_LN10 / M_LN2 + 0.5))

   union conv_d {
      double d;
      unsigned char b[8];
   } d;
   int i,j,k,d_exp;

   if (x == 0)
      return x;

   /* do minor sanity check for IEEE 754 fp and correct byte order */
   d.d = 1.0;
   if (sizeof(double) == 8 && d.b[7] == 0x3f && d.b[6] == 0xf0) {

      /*
       * this code will round IEEE 754 double to XLFD_NDIGITS_2 binary digits
       */

      d.d = x;
      d_exp = (d.b[7] << 4) | (d.b[6] >> 4);

      i = (DBL_MANT_DIG-XLFD_NDIGITS_2) >> 3;
      j = 1 << ((DBL_MANT_DIG-XLFD_NDIGITS_2) & 0x07);
      for (; i<7; i++) {
	 k = d.b[i] + j;
	 d.b[i] = k;
	 if (k & 0x100) j = 1;
	 else break;
      }
      if ((i==7) && ((d.b[6] & 0xf0) != ((d_exp<<4) & 0xf0))) {
	 /* mantissa overflow: increment exponent */
	 d_exp = (d_exp & 0x800 ) | ((d_exp & 0x7ff) + 1);
	 d.b[7] = d_exp >> 4;
	 d.b[6] = (d.b[6] & 0x0f) | (d_exp << 4);
      }

      i = (DBL_MANT_DIG-XLFD_NDIGITS_2) >> 3;
      j = 1 << ((DBL_MANT_DIG-XLFD_NDIGITS_2) & 0x07);
      d.b[i] &= ~(j-1);
      for (;--i>=0;) d.b[i] = 0;

      return d.d;
   }
   else
#endif
#endif /* i386 || __i386__ */
    {
	/*
	 * If not IEEE 754:  Let printf() do it for you.
	 */

	char buffer[40];

	snprintf(buffer, sizeof(buffer), "%.*lg", XLFD_NDIGITS, x);
	return atof(buffer);
    }
}

static char *
GetMatrix(char *ptr, FontScalablePtr vals, int which)
{
    double *matrix;

    if (which == PIXELSIZE_MASK)
	matrix = vals->pixel_matrix;
    else if (which == POINTSIZE_MASK)
	matrix = vals->point_matrix;
    else return (char *)0;

    while (isspace((unsigned char)*ptr)) ptr++;
    if (*ptr == '[')
    {
	/* This is a matrix containing real numbers.  It would be nice
	   to use strtod() or sscanf() to read the numbers, but those
	   don't handle '~' for minus and we cannot force them to use a
	   "."  for the radix.  We'll have to do the hard work ourselves
	   (in readreal()).  */

	if ((ptr = readreal(++ptr, matrix + 0)) &&
	    (ptr = readreal(ptr, matrix + 1)) &&
	    (ptr = readreal(ptr, matrix + 2)) &&
	    (ptr = readreal(ptr, matrix + 3)))
	{
	    while (isspace((unsigned char)*ptr)) ptr++;
	    if (*ptr != ']')
		ptr = (char *)0;
	    else
	    {
		ptr++;
		while (isspace((unsigned char)*ptr)) ptr++;
		if (*ptr == '-')
		{
		    if (which == POINTSIZE_MASK)
			vals->values_supplied |= POINTSIZE_ARRAY;
		    else
			vals->values_supplied |= PIXELSIZE_ARRAY;
		}
		else ptr = (char *)0;
	    }
	}
    }
    else
    {
	int value;
	if ((ptr = GetInt(ptr, &value)))
	{
	    vals->values_supplied &= ~which;
	    if (value > 0)
	    {
		matrix[3] = (double)value;
		if (which == POINTSIZE_MASK)
		{
		    matrix[3] /= 10.0;
		    vals->values_supplied |= POINTSIZE_SCALAR;
		}
		else
		    vals->values_supplied |= PIXELSIZE_SCALAR;
		/* If we're concocting the pixelsize array from a scalar,
		   we will need to normalize element 0 for the pixel shape.
		   This is done in FontFileCompleteXLFD(). */
		matrix[0] = matrix[3];
		matrix[1] = matrix[2] = 0.0;
	    }
	    else if (value < 0)
	    {
		if (which == POINTSIZE_MASK)
		    vals->values_supplied |= POINTSIZE_WILDCARD;
		else
		    vals->values_supplied |= PIXELSIZE_WILDCARD;
	    }
	}
    }
    return ptr;
}


static void
append_ranges(char *fname, size_t fnamelen, int nranges, fsRange *ranges)
{
    if (nranges)
    {
        int i;

        strlcat(fname, "[", fnamelen);
        for (i = 0; i < nranges && strlen(fname) < 1010; i++)
        {
	    size_t curlen;
	    if (i) strlcat(fname, " ", fnamelen);
	    curlen = strlen(fname);
	    snprintf(fname + curlen, fnamelen - curlen, "%d",
		     minchar(ranges[i]));
	    if (ranges[i].min_char_low ==
		ranges[i].max_char_low &&
		ranges[i].min_char_high ==
		ranges[i].max_char_high) continue;
	    snprintf(fname + curlen, fnamelen - curlen, "_%d",
		     maxchar(ranges[i]));
        }
        strlcat(fname, "]", fnamelen);
    }
}

Bool
FontParseXLFDName(char *fname, FontScalablePtr vals, int subst)
{
    register char *ptr;
    register char *ptr1,
               *ptr2,
               *ptr3,
               *ptr4;
    register char *ptr5;
    FontScalableRec tmpvals;
    char        replaceChar = '0';
    char        tmpBuf[1024];
    size_t      tlen;
    size_t      fnamelen = MAXFONTNAMELEN; /* assumed for now */
    int         spacingLen;
    int		l;
    char	*p;

    bzero(&tmpvals, sizeof(tmpvals));
    if (subst != FONT_XLFD_REPLACE_VALUE)
	*vals = tmpvals;

    if (!(*(ptr = fname) == '-' || (*ptr++ == '*' && *ptr == '-')) ||  /* fndry */
	    !(ptr = strchr(ptr + 1, '-')) ||	/* family_name */
	    !(ptr1 = ptr = strchr(ptr + 1, '-')) ||	/* weight_name */
	    !(ptr = strchr(ptr + 1, '-')) ||	/* slant */
	    !(ptr = strchr(ptr + 1, '-')) ||	/* setwidth_name */
	    !(ptr = strchr(ptr + 1, '-')) ||	/* add_style_name */
	    !(ptr = strchr(ptr + 1, '-')) ||	/* pixel_size */
	    !(ptr = GetMatrix(ptr + 1, &tmpvals, PIXELSIZE_MASK)) ||
	    !(ptr2 = ptr = GetMatrix(ptr + 1, &tmpvals, POINTSIZE_MASK)) ||
	    !(ptr = GetInt(ptr + 1, &tmpvals.x)) ||	/* resolution_x */
	    !(ptr3 = ptr = GetInt(ptr + 1, &tmpvals.y)) ||  /* resolution_y */
	    !(ptr4 = ptr = strchr(ptr + 1, '-')) ||	/* spacing */
	    !(ptr5 = ptr = GetInt(ptr + 1, &tmpvals.width)) || /* average_width */
	    !(ptr = strchr(ptr + 1, '-')) ||	/* charset_registry */
	    strchr(ptr + 1, '-'))/* charset_encoding */
	return FALSE;

    /* Lop off HP charset subsetting enhancement.  Interpreting this
       field requires allocating some space in which to return the
       results.  So, to prevent memory leaks, this procedure will simply
       lop off and ignore charset subsetting, and initialize the
       relevant vals fields to zero.  It's up to the caller to make its
       own call to FontParseRanges() if it's interested in the charset
       subsetting.  */

    if (subst != FONT_XLFD_REPLACE_NONE &&
	(p = strchr(strrchr(fname, '-'), '[')))
    {
	tmpvals.values_supplied |= CHARSUBSET_SPECIFIED;
	*p = '\0';
    }

    /* Fill in deprecated fields for the benefit of rasterizers that care
       about them. */
    tmpvals.pixel = (tmpvals.pixel_matrix[3] >= 0) ?
		    (int)(tmpvals.pixel_matrix[3] + .5) :
		    (int)(tmpvals.pixel_matrix[3] - .5);
    tmpvals.point = (tmpvals.point_matrix[3] >= 0) ?
                    (int)(tmpvals.point_matrix[3] * 10 + .5) :
                    (int)(tmpvals.point_matrix[3] * 10 - .5);

    spacingLen = ptr4 - ptr3 + 1;

    switch (subst) {
    case FONT_XLFD_REPLACE_NONE:
	*vals = tmpvals;
	break;
    case FONT_XLFD_REPLACE_STAR:
	replaceChar = '*';
	/* FALLTHROUGH */
    case FONT_XLFD_REPLACE_ZERO:
	strlcpy(tmpBuf, ptr2, sizeof(tmpBuf));
	ptr5 = tmpBuf + (ptr5 - ptr2);
	ptr3 = tmpBuf + (ptr3 - ptr2);
	ptr2 = tmpBuf;
	ptr = ptr1 + 1;

	ptr = strchr(ptr, '-') + 1;		/* skip weight */
	ptr = strchr(ptr, '-') + 1;		/* skip slant */
	ptr = strchr(ptr, '-') + 1;		/* skip setwidth_name */
	ptr = strchr(ptr, '-') + 1;		/* skip add_style_name */

	if ((ptr - fname) + spacingLen + strlen(ptr5) + 10 >= (unsigned)1024)
	    return FALSE;
	*ptr++ = replaceChar;
	*ptr++ = '-';
	*ptr++ = replaceChar;
	*ptr++ = '-';
	*ptr++ = '*';
	*ptr++ = '-';
	*ptr++ = '*';
	if (spacingLen > 2)
	{
	    memmove(ptr, ptr3, spacingLen);
	    ptr += spacingLen;
	}
	else
	{
	    *ptr++ = '-';
	    *ptr++ = '*';
	    *ptr++ = '-';
	}
	*ptr++ = replaceChar;
	strlcpy(ptr, ptr5, fnamelen - (ptr - fname));
	*vals = tmpvals;
	break;
    case FONT_XLFD_REPLACE_VALUE:
	if (vals->values_supplied & PIXELSIZE_MASK)
	{
	    tmpvals.values_supplied =
		(tmpvals.values_supplied & ~PIXELSIZE_MASK) |
		(vals->values_supplied & PIXELSIZE_MASK);
	    tmpvals.pixel_matrix[0] = vals->pixel_matrix[0];
	    tmpvals.pixel_matrix[1] = vals->pixel_matrix[1];
	    tmpvals.pixel_matrix[2] = vals->pixel_matrix[2];
	    tmpvals.pixel_matrix[3] = vals->pixel_matrix[3];
	}
	if (vals->values_supplied & POINTSIZE_MASK)
	{
	    tmpvals.values_supplied =
		(tmpvals.values_supplied & ~POINTSIZE_MASK) |
		(vals->values_supplied & POINTSIZE_MASK);
	    tmpvals.point_matrix[0] = vals->point_matrix[0];
	    tmpvals.point_matrix[1] = vals->point_matrix[1];
	    tmpvals.point_matrix[2] = vals->point_matrix[2];
	    tmpvals.point_matrix[3] = vals->point_matrix[3];
	}
	if (vals->x >= 0)
	    tmpvals.x = vals->x;
	if (vals->y >= 0)
	    tmpvals.y = vals->y;
	if (vals->width >= 0)
	    tmpvals.width = vals->width;
	else if (vals->width < -1)	/* overload: -1 means wildcard */
	    tmpvals.width = -vals->width;


	p = ptr1 + 1;				/* weight field */
	l = strchr(p, '-') - p;
	snprintf(tmpBuf, sizeof(tmpBuf), "%*.*s", l, l, p);

	p += l + 1;				/* slant field */
	l = strchr(p, '-') - p;
	tlen = strlen(tmpBuf);
	snprintf(tmpBuf + tlen, sizeof(tmpBuf) - tlen, "-%*.*s", l, l, p);

	p += l + 1;				/* setwidth_name */
	l = strchr(p, '-') - p;
	tlen = strlen(tmpBuf);
	snprintf(tmpBuf + tlen, sizeof(tmpBuf) - tlen, "-%*.*s", l, l, p);

	p += l + 1;				/* add_style_name field */
	l = strchr(p, '-') - p;
	tlen = strlen(tmpBuf);
	snprintf(tmpBuf + tlen, sizeof(tmpBuf) - tlen, "-%*.*s", l, l, p);

	strlcat(tmpBuf, "-", sizeof(tmpBuf));
	if ((tmpvals.values_supplied & PIXELSIZE_MASK) == PIXELSIZE_ARRAY)
	{
	    char buffer[XLFD_DOUBLE_TO_TEXT_BUF_SIZE];
	    strlcat(tmpBuf, "[", sizeof(tmpBuf));
	    strlcat(tmpBuf,
		    xlfd_double_to_text(tmpvals.pixel_matrix[0], buffer, 0),
		    sizeof(tmpBuf));
	    strlcat(tmpBuf,
		    xlfd_double_to_text(tmpvals.pixel_matrix[1], buffer, 1),
		    sizeof(tmpBuf));
	    strlcat(tmpBuf,
		    xlfd_double_to_text(tmpvals.pixel_matrix[2], buffer, 1),
		    sizeof(tmpBuf));
	    strlcat(tmpBuf,
		    xlfd_double_to_text(tmpvals.pixel_matrix[3], buffer, 1),
		    sizeof(tmpBuf));
	    strlcat(tmpBuf, "]", sizeof(tmpBuf));
	}
	else
	{
	    tlen = strlen(tmpBuf);
	    snprintf(tmpBuf + tlen, sizeof(tmpBuf) - tlen, "%d",
		     (int)(tmpvals.pixel_matrix[3] + .5));
	}
	strlcat(tmpBuf, "-", sizeof(tmpBuf));
	if ((tmpvals.values_supplied & POINTSIZE_MASK) == POINTSIZE_ARRAY)
	{
	    char buffer[XLFD_DOUBLE_TO_TEXT_BUF_SIZE];
	    strlcat(tmpBuf, "[", sizeof(tmpBuf));
	    strlcat(tmpBuf,
		    xlfd_double_to_text(tmpvals.point_matrix[0], buffer, 0),
		    sizeof(tmpBuf));
	    strlcat(tmpBuf,
		    xlfd_double_to_text(tmpvals.point_matrix[1], buffer, 1),
		    sizeof(tmpBuf));
	    strlcat(tmpBuf,
		    xlfd_double_to_text(tmpvals.point_matrix[2], buffer, 1),
		    sizeof(tmpBuf));
	    strlcat(tmpBuf,
		    xlfd_double_to_text(tmpvals.point_matrix[3], buffer, 1),
		    sizeof(tmpBuf));
	    strlcat(tmpBuf, "]", sizeof(tmpBuf));
	}
	else
	{
	    tlen = strlen(tmpBuf);
	    snprintf(tmpBuf + tlen, sizeof(tmpBuf) - tlen, "%d",
		     (int)(tmpvals.point_matrix[3] * 10.0 + .5));
	}
	tlen = strlen(tmpBuf);
	snprintf(tmpBuf + tlen, sizeof(tmpBuf) - tlen, "-%d-%d%*.*s%d%s",
		 tmpvals.x, tmpvals.y,
		 spacingLen, spacingLen, ptr3, tmpvals.width, ptr5);
	strlcpy(ptr1 + 1, tmpBuf, fnamelen - (ptr1 - fname));
	if ((vals->values_supplied & CHARSUBSET_SPECIFIED) && !vals->nranges)
	    strlcat(fname, "[]", fnamelen);
	else
	    append_ranges(fname, fnamelen, vals->nranges, vals->ranges);
	break;
    }
    return TRUE;
}

fsRange *FontParseRanges(char *name, int *nranges)
{
    int n;
    unsigned long l;
    char *p1, *p2;
    fsRange *result = (fsRange *)0;

    name = strchr(name, '-');
    for (n = 1; name && n < 14; n++)
	name = strchr(name + 1, '-');

    *nranges = 0;
    if (!name || !(p1 = strchr(name, '['))) return (fsRange *)0;
    p1++;

    while (*p1 && *p1 != ']')
    {
	fsRange thisrange;

	l = strtol(p1, &p2, 0);
	if (p2 == p1 || l > 0xffff) break;
	thisrange.max_char_low = thisrange.min_char_low = l & 0xff;
	thisrange.max_char_high = thisrange.min_char_high = l >> 8;

	p1 = p2;
	if (*p1 == ']' || *p1 == ' ')
	{
	    while (*p1 == ' ') p1++;
	    if (add_range(&thisrange, nranges, &result, TRUE) != Successful)
		break;
	}
	else if (*p1 == '_')
	{
	    l = strtol(++p1, &p2, 0);
	    if (p2 == p1 || l > 0xffff) break;
	    thisrange.max_char_low = l & 0xff;
	    thisrange.max_char_high = l >> 8;
	    p1 = p2;
	    if (*p1 == ']' || *p1 == ' ')
	    {
		while (*p1 == ' ') p1++;
		if (add_range(&thisrange, nranges, &result, TRUE) != Successful)
		    break;
	    }
	}
	else break;
    }

    return result;
}
