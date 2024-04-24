/*
Copyright (c) 1998-2001 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/* Header for backend-independent encoding code */

/* An encoding is identified with a name.  An encoding contains some
   global encoding data, such as its size, and a set of mappings.
   Mappings are identified by their type and two integers, known as
   pid and eid, the interpretation of which is type dependent. */

#ifndef _FONTENC_H
#define _FONTENC_H

/* Encoding types.  For future extensions, clients should be prepared
   to ignore unknown encoding types. */

/* 0 is treated specially. */

#define FONT_ENCODING_UNICODE 1
#define FONT_ENCODING_TRUETYPE 2
#define FONT_ENCODING_POSTSCRIPT 3

/* This structure represents a mapping, either from numeric codes from
   numeric codes, or from numeric codes to strings. */

/* It is expected that only one of `recode' and `name' will actually
   be present.  However, having both fields simplifies the interface
   somewhat. */

typedef struct _FontMap {
    int type;                   /* the type of the mapping */
    int pid, eid;               /* the identification of the mapping */
    unsigned (*recode) (unsigned, void *);      /* mapping function */
    char *(*name) (unsigned, void *);   /* function returning glyph names */
    void *client_data;          /* second parameter of the two above */
    struct _FontMap *next;      /* link to next element in list */
    /* The following was added for version 0.3 of the font interface. */
    /* It should be kept at the end to preserve binary compatibility. */
    struct _FontEnc *encoding;
} FontMapRec, *FontMapPtr;

/* This is the structure that holds all the info for one encoding.  It
   consists of a charset name, its size, and a linked list of mappings
   like above. */

typedef struct _FontEnc {
    char *name;                 /* the name of the encoding */
    char **aliases;             /* its aliases, null terminated */
    int size;                   /* its size, either in bytes or rows */
    int row_size;               /* the size of a row, or 0 if bytes */
    FontMapPtr mappings;        /* linked list of mappings */
    struct _FontEnc *next;      /* link to next element */
    /* the following two were added in version 0.2 of the font interface */
    /* they should be kept at the end to preserve binary compatibility */
    int first;                  /* first byte or row */
    int first_col;              /* first column in each row */
} FontEncRec, *FontEncPtr;

typedef struct _FontMapReverse {
    unsigned int (*reverse) (unsigned, void *);
    void *data;
} FontMapReverseRec, *FontMapReversePtr;


/* Function prototypes */

/* extract an encoding name from an XLFD name.  Returns a pointer to a
   *static* buffer, or NULL */
char *FontEncFromXLFD(const char *, int);

/* find the encoding data for a given encoding name; second parameter
   is the filename of the font for which the encoding is needed.
   Returns NULL on failure. */
FontEncPtr FontEncFind(const char *, const char *);

/* Find a given mapping for an encoding.  This is only a convenience
   function, as clients are allowed to scavenge the data structures
   themselves (as the TrueType backend does). */

FontMapPtr FontMapFind(FontEncPtr, int, int, int);

/* Do both in a single step */
FontMapPtr FontEncMapFind(const char *, int, int, int, const char *);

/* Recode a code.  Always succeeds. */
unsigned FontEncRecode(unsigned, FontMapPtr);

/* Return a name for a code.  Returns a string or NULL. */
char *FontEncName(unsigned, FontMapPtr);

/* Return a pointer to the name of the system encodings directory. */
/* This string is static and should not be modified. */
const char *FontEncDirectory(void);

/* Identify an encoding file.  If fileName doesn't exist, or is not an
   encoding file, return NULL, otherwise returns a NULL-terminated
   array of strings. */
char **FontEncIdentify(const char *fileName);

FontMapReversePtr FontMapReverse(FontMapPtr);

void FontMapReverseFree(FontMapReversePtr);
#endif
