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

/* Parser for encoding files */

/* This code assumes that we are using ASCII.  We don't use the ctype
   functions, as they depend on the current locale.  On the other
   hand, we do use strcasecmp, but only on strings that we've checked
   to be pure ASCII.  Bloody ``Code Set Independence''. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <strings.h>
#include <stdio.h>

#include <stdlib.h>

#include "zlib.h"
typedef gzFile FontFilePtr;

#define FontFileGetc(f) gzgetc(f)
#define FontFileOpen(filename) gzopen(filename, "rb")
#define FontFileClose(f) gzclose(f)

#define MAXFONTFILENAMELEN 1024
#define MAXFONTNAMELEN 1024

#include <X11/fonts/fontenc.h>
#include "fontencI.h"
#include "reallocarray.h"

#define MAXALIASES 20

#define EOF_TOKEN -1
#define ERROR_TOKEN -2
#define EOL_TOKEN 0
#define NUMBER_TOKEN 1
#define KEYWORD_TOKEN 2

#define EOF_LINE -1
#define ERROR_LINE -2
#define STARTENCODING_LINE 1
#define STARTMAPPING_LINE 2
#define ENDMAPPING_LINE 3
#define CODE_LINE 4
#define CODE_RANGE_LINE 5
#define CODE_UNDEFINE_LINE 6
#define NAME_LINE 7
#define SIZE_LINE 8
#define ALIAS_LINE 9
#define FIRSTINDEX_LINE 10

/* Return from lexer */
#define MAXKEYWORDLEN 100

static long number_value;
static char keyword_value[MAXKEYWORDLEN + 1];

static long value1, value2, value3;

/* Lexer code */

/* Skip to the beginning of new line */
static void
skipEndOfLine(FontFilePtr f, int c)
{
    if (c == 0)
        c = FontFileGetc(f);

    for (;;)
        if (c <= 0 || c == '\n')
            return;
        else
            c = FontFileGetc(f);
}

/* Get a number; we're at the first digit. */
static unsigned
getnum(FontFilePtr f, int c, int *cp)
{
    unsigned n = 0;
    int base = 10;

    /* look for `0' or `0x' prefix */
    if (c == '0') {
        c = FontFileGetc(f);
        base = 8;
        if (c == 'x' || c == 'X') {
            base = 16;
            c = FontFileGetc(f);
        }
    }

    /* accumulate digits */
    for (;;) {
        if ('0' <= c && c <= '9') {
            n *= base;
            n += c - '0';
        }
        else if ('a' <= c && c <= 'f') {
            n *= base;
            n += c - 'a' + 10;
        }
        else if ('A' <= c && c <= 'F') {
            n *= base;
            n += c - 'A' + 10;
        }
        else
            break;
        c = FontFileGetc(f);
    }

    *cp = c;
    return n;
}

/* Skip to beginning of new line; return 1 if only whitespace was found. */
static int
endOfLine(FontFilePtr f, int c)
{
    if (c == 0)
        c = FontFileGetc(f);

    for (;;) {
        if (c <= 0 || c == '\n')
            return 1;
        else if (c == '#') {
            skipEndOfLine(f, c);
            return 1;
        }
        else if (c == ' ' || c == '\t') {
            skipEndOfLine(f, c);
            return 0;
        }
        c = FontFileGetc(f);
    }
}

/* Get a token; we're at first char */
static int
gettoken(FontFilePtr f, int c, int *cp)
{
    char *p;

    if (c <= 0)
        c = FontFileGetc(f);

    if (c <= 0) {
        return EOF_TOKEN;
    }

    while (c == ' ' || c == '\t')
        c = FontFileGetc(f);

    if (c == '\n') {
        return EOL_TOKEN;
    }
    else if (c == '#') {
        skipEndOfLine(f, c);
        return EOL_TOKEN;
    }
    else if (c >= '0' && c <= '9') {
        number_value = getnum(f, c, cp);
        return NUMBER_TOKEN;
    }
    else if ((c >= 'A' && c <= 'Z') ||
             (c >= 'a' && c <= 'z') ||
             c == '/' || c == '_' || c == '-' || c == '.') {
        p = keyword_value;
        *p++ = c;
        while (p - keyword_value < MAXKEYWORDLEN) {
            c = FontFileGetc(f);
            if (c <= ' ' || c > '~' || c == '#')
                break;
            *p++ = c;
        }
        *cp = c;
        *p = '\0';
        return KEYWORD_TOKEN;
    }
    else {
        *cp = c;
        return ERROR_TOKEN;
    }
}

/* Parse a line.
 * Always skips to the beginning of a new line, even if an error occurs */
static int
getnextline(FontFilePtr f)
{
    int c, token;

    c = FontFileGetc(f);
    if (c <= 0)
        return EOF_LINE;

 again:
    token = gettoken(f, c, &c);

    switch (token) {
    case EOF_TOKEN:
        return EOF_LINE;
    case EOL_TOKEN:
        /* empty line */
        c = FontFileGetc(f);
        goto again;
    case NUMBER_TOKEN:
        value1 = number_value;
        token = gettoken(f, c, &c);
        switch (token) {
        case NUMBER_TOKEN:
            value2 = number_value;
            token = gettoken(f, c, &c);
            switch (token) {
            case NUMBER_TOKEN:
                value3 = number_value;
                return CODE_RANGE_LINE;
            case EOL_TOKEN:
                return CODE_LINE;
            default:
                skipEndOfLine(f, c);
                return ERROR_LINE;
            }
        case KEYWORD_TOKEN:
            if (!endOfLine(f, c))
                return ERROR_LINE;
            else
                return NAME_LINE;
        default:
            skipEndOfLine(f, c);
            return ERROR_LINE;
        }
    case KEYWORD_TOKEN:
        if (!strcasecmp(keyword_value, "STARTENCODING")) {
            token = gettoken(f, c, &c);
            if (token == KEYWORD_TOKEN) {
                if (endOfLine(f, c))
                    return STARTENCODING_LINE;
                else
                    return ERROR_LINE;
            }
            else {
                skipEndOfLine(f, c);
                return ERROR_LINE;
            }
        }
        else if (!strcasecmp(keyword_value, "ALIAS")) {
            token = gettoken(f, c, &c);
            if (token == KEYWORD_TOKEN) {
                if (endOfLine(f, c))
                    return ALIAS_LINE;
                else
                    return ERROR_LINE;
            }
            else {
                skipEndOfLine(f, c);
                return ERROR_LINE;
            }
        }
        else if (!strcasecmp(keyword_value, "SIZE")) {
            token = gettoken(f, c, &c);
            if (token == NUMBER_TOKEN) {
                value1 = number_value;
                token = gettoken(f, c, &c);
                switch (token) {
                case NUMBER_TOKEN:
                    value2 = number_value;
                    return SIZE_LINE;
                case EOL_TOKEN:
                    value2 = 0;
                    return SIZE_LINE;
                default:
                    skipEndOfLine(f, c);
                    return ERROR_LINE;
                }
            }
            else {
                skipEndOfLine(f, c);
                return ERROR_LINE;
            }
        }
        else if (!strcasecmp(keyword_value, "FIRSTINDEX")) {
            token = gettoken(f, c, &c);
            if (token == NUMBER_TOKEN) {
                value1 = number_value;
                token = gettoken(f, c, &c);
                switch (token) {
                case NUMBER_TOKEN:
                    value2 = number_value;
                    return FIRSTINDEX_LINE;
                case EOL_TOKEN:
                    value2 = 0;
                    return FIRSTINDEX_LINE;
                default:
                    skipEndOfLine(f, c);
                    return ERROR_LINE;
                }
            }
            else {
                skipEndOfLine(f, c);
                return ERROR_LINE;
            }
        }
        else if (!strcasecmp(keyword_value, "STARTMAPPING")) {
            keyword_value[0] = 0;
            value1 = 0;
            value2 = 0;
            /* first a keyword */
            token = gettoken(f, c, &c);
            if (token != KEYWORD_TOKEN) {
                skipEndOfLine(f, c);
                return ERROR_LINE;
            }

            /* optional first integer */
            token = gettoken(f, c, &c);
            if (token == NUMBER_TOKEN) {
                value1 = number_value;
            }
            else if (token == EOL_TOKEN) {
                return STARTMAPPING_LINE;
            }
            else {
                skipEndOfLine(f, c);
                return ERROR_LINE;
            }

            /* optional second integer */
            token = gettoken(f, c, &c);
            if (token == NUMBER_TOKEN) {
                value2 = number_value;
            }
            else if (token == EOL_TOKEN) {
                return STARTMAPPING_LINE;
            }
            else {
                skipEndOfLine(f, c);
                return ERROR_LINE;
            }

            if (!endOfLine(f, c))
                return ERROR_LINE;
            else {
                return STARTMAPPING_LINE;
            }
        }
        else if (!strcasecmp(keyword_value, "UNDEFINE")) {
            /* first integer */
            token = gettoken(f, c, &c);
            if (token != NUMBER_TOKEN) {
                skipEndOfLine(f, c);
                return ERROR_LINE;
            }
            value1 = number_value;
            /* optional second integer */
            token = gettoken(f, c, &c);
            if (token == EOL_TOKEN) {
                value2 = value1;
                return CODE_UNDEFINE_LINE;
            }
            else if (token == NUMBER_TOKEN) {
                value2 = number_value;
                if (endOfLine(f, c)) {
                    return CODE_UNDEFINE_LINE;
                }
                else
                    return ERROR_LINE;
            }
            else {
                skipEndOfLine(f, c);
                return ERROR_LINE;
            }
        }
        else if (!strcasecmp(keyword_value, "ENDENCODING")) {
            if (endOfLine(f, c))
                return EOF_LINE;
            else
                return ERROR_LINE;
        }
        else if (!strcasecmp(keyword_value, "ENDMAPPING")) {
            if (endOfLine(f, c))
                return ENDMAPPING_LINE;
            else
                return ERROR_LINE;
        }
        else {
            skipEndOfLine(f, c);
            return ERROR_LINE;
        }
    default:
        return ERROR_LINE;
    }
}

static void
install_mapping(FontEncPtr encoding, FontMapPtr mapping)
{
    FontMapPtr m;

    if (encoding->mappings == NULL)
        encoding->mappings = mapping;
    else {
        m = encoding->mappings;
        while (m->next != NULL)
            m = m->next;
        m->next = mapping;
    }
    mapping->next = NULL;
    mapping->encoding = encoding;
}

static int
setCode(unsigned from, unsigned to, unsigned row_size,
        unsigned *first, unsigned *last,
        unsigned *encsize, unsigned short **enc)
{
    unsigned index, i;

    unsigned short *newenc;

    if (from > 0xFFFF)
        return 0;               /* success */

    if (row_size == 0)
        index = from;
    else {
        if ((value1 & 0xFF) >= row_size)
            return 0;           /* ignore out of range mappings */
        index = (from >> 8) * row_size + (from & 0xFF);
    }

    /* Optimize away useless identity mappings.  This is only expected
       to be useful with linear encodings. */
    if (index == to && (index < *first || index > *last))
        return 0;
    if (*encsize == 0) {
        *encsize = (index < 256) ? 256 : 0x10000;
        *enc = Xmallocarray(*encsize, sizeof(unsigned short));
        if (*enc == NULL) {
            *encsize = 0;
            return 1;
        }
    }
    else if (*encsize <= index) {
        *encsize = 0x10000;
        newenc = Xreallocarray(*enc, *encsize, sizeof(unsigned short));
        if (newenc == NULL)
            return 1;
        *enc = newenc;
    }
    if (*first > *last) {
        *first = *last = index;
    }
    if (index < *first) {
        for (i = index; i < *first; i++)
            (*enc)[i] = i;
        *first = index;
    }
    if (index > *last) {
        for (i = *last + 1; i <= index; i++)
            (*enc)[i] = i;
        *last = index;
    }
    (*enc)[index] = to;
    return 0;
}

/* Parser.  If headerOnly is true, we're only interested in the
   data contained in the encoding file's header. */

/* As font encodings are currently never freed, the allocations done
   by this function are mostly its private business.  Note, however,
   that FontEncIdentify needs to free the header fields -- so if you
   change this function, you may need to change FontEncIdentify. */

/* I want a garbage collector. */

static FontEncPtr
parseEncodingFile(FontFilePtr f, int headerOnly)
{
    int line;

    unsigned short *enc = NULL;
    char **nam = NULL, **newnam;
    unsigned i, first = 0xFFFF, last = 0, encsize = 0, namsize = 0;
    FontEncPtr encoding = NULL;
    FontMapPtr mapping = NULL;
    FontEncSimpleMapPtr sm;
    FontEncSimpleNamePtr sn;
    char *aliases[MAXALIASES] = { NULL };
    int numaliases = 0;

#if 0
    /* GCC complains about unused labels.  Please fix GCC rather than
       obfuscating my code. */
 no_encoding:
#endif
    line = getnextline(f);
    switch (line) {
    case EOF_LINE:
        goto error;
    case STARTENCODING_LINE:
        encoding = malloc(sizeof(FontEncRec));
        if (encoding == NULL)
            goto error;
        encoding->name = strdup(keyword_value);
        if (encoding->name == NULL)
            goto error;
        encoding->size = 256;
        encoding->row_size = 0;
        encoding->mappings = NULL;
        encoding->next = NULL;
        encoding->first = encoding->first_col = 0;
        goto no_mapping;
    default:
        goto error;
    }

 no_mapping:
    line = getnextline(f);
    switch (line) {
    case EOF_LINE:
        goto done;
    case ALIAS_LINE:
        if (numaliases < MAXALIASES) {
            aliases[numaliases] = strdup(keyword_value);
            if (aliases[numaliases] == NULL)
                goto error;
            numaliases++;
        }
        goto no_mapping;
    case SIZE_LINE:
        encoding->size = value1;
        encoding->row_size = value2;
        goto no_mapping;
    case FIRSTINDEX_LINE:
        encoding->first = value1;
        encoding->first_col = value2;
        goto no_mapping;
    case STARTMAPPING_LINE:
        if (headerOnly)
            goto done;
        if (!strcasecmp(keyword_value, "unicode")) {
            mapping = malloc(sizeof(FontMapRec));
            if (mapping == NULL)
                goto error;
            mapping->type = FONT_ENCODING_UNICODE;
            mapping->pid = 0;
            mapping->eid = 0;
            mapping->recode = NULL;
            mapping->name = NULL;
            mapping->client_data = NULL;
            mapping->next = NULL;
            goto mapping;
        }
        else if (!strcasecmp(keyword_value, "cmap")) {
            mapping = malloc(sizeof(FontMapRec));
            if (mapping == NULL)
                goto error;
            mapping->type = FONT_ENCODING_TRUETYPE;
            mapping->pid = value1;
            mapping->eid = value2;
            mapping->recode = NULL;
            mapping->name = NULL;
            mapping->client_data = NULL;
            mapping->next = NULL;
            goto mapping;
        }
        else if (!strcasecmp(keyword_value, "postscript")) {
            mapping = malloc(sizeof(FontMapRec));
            if (mapping == NULL)
                goto error;
            mapping->type = FONT_ENCODING_POSTSCRIPT;
            mapping->pid = 0;
            mapping->eid = 0;
            mapping->recode = NULL;
            mapping->name = NULL;
            mapping->client_data = NULL;
            mapping->next = NULL;
            goto string_mapping;
        }
        else {                  /* unknown mapping type -- ignore */
            goto skipmapping;
        }
        /* NOTREACHED */
        goto error;
    default:
        goto no_mapping;        /* ignore unknown lines */
    }

 skipmapping:
    line = getnextline(f);
    switch (line) {
    case ENDMAPPING_LINE:
        goto no_mapping;
    case EOF_LINE:
        goto error;
    default:
        goto skipmapping;
    }

 mapping:
    line = getnextline(f);
    switch (line) {
    case EOF_LINE:
        goto error;
    case ENDMAPPING_LINE:
        mapping->recode = FontEncSimpleRecode;
        mapping->name = FontEncUndefinedName;
        mapping->client_data = sm = malloc(sizeof(FontEncSimpleMapRec));
        if (sm == NULL)
            goto error;
        sm->row_size = encoding->row_size;
        if (first <= last) {
            unsigned short *newmap;

            sm->first = first;
            sm->len = last - first + 1;
            newmap = Xmallocarray(sm->len, sizeof(unsigned short));
            if (newmap == NULL) {
                free(sm);
                mapping->client_data = sm = NULL;
                goto error;
            }
            for (i = 0; i < sm->len; i++)
                newmap[i] = enc[first + i];
            sm->map = newmap;
        }
        else {
            sm->first = 0;
            sm->len = 0;
            sm->map = NULL;
        }
        install_mapping(encoding, mapping);
        mapping = NULL;
        first = 0xFFFF;
        last = 0;
        goto no_mapping;

    case CODE_LINE:
        if (setCode(value1, value2, encoding->row_size,
                    &first, &last, &encsize, &enc))
            goto error;
        goto mapping;

    case CODE_RANGE_LINE:
        if (value1 > 0x10000)
            value1 = 0x10000;
        if (value2 > 0x10000)
            value2 = 0x10000;
        if (value2 < value1)
            goto mapping;
        /* Do the last value first to avoid having to realloc() */
        if (setCode(value2, value3 + (value2 - value1), encoding->row_size,
                    &first, &last, &encsize, &enc))
            goto error;
        for (i = value1; i < value2; i++) {
            if (setCode(i, value3 + (i - value1), encoding->row_size,
                        &first, &last, &encsize, &enc))
                goto error;
        }
        goto mapping;

    case CODE_UNDEFINE_LINE:
        if (value1 > 0x10000)
            value1 = 0x10000;
        if (value2 > 0x10000)
            value2 = 0x10000;
        if (value2 < value1)
            goto mapping;
        /* Do the last value first to avoid having to realloc() */
        if (setCode(value2, 0, encoding->row_size,
                    &first, &last, &encsize, &enc))
            goto error;
        for (i = value1; i < value2; i++) {
            if (setCode(i, 0, encoding->row_size,
                        &first, &last, &encsize, &enc))
                goto error;
        }
        goto mapping;

    default:
        goto mapping;           /* ignore unknown lines */
    }

 string_mapping:
    line = getnextline(f);
    switch (line) {
    case EOF_LINE:
        goto error;
    case ENDMAPPING_LINE:
        mapping->recode = FontEncUndefinedRecode;
        mapping->name = FontEncSimpleName;
        mapping->client_data = sn = malloc(sizeof(FontEncSimpleNameRec));
        if (sn == NULL)
            goto error;
        if (first > last) {
            free(sn);
            mapping->client_data = sn = NULL;
            goto error;
        }
        sn->first = first;
        sn->len = last - first + 1;
        sn->map = Xmallocarray(sn->len, sizeof(char *));
        if (sn->map == NULL) {
            free(sn);
            mapping->client_data = sn = NULL;
            goto error;
        }
        for (i = 0; i < sn->len; i++)
            sn->map[i] = nam[first + i];
        install_mapping(encoding, mapping);
        mapping = NULL;
        first = 0xFFFF;
        last = 0;
        goto no_mapping;
    case NAME_LINE:
        if (value1 >= 0x10000)
            goto string_mapping;
        if (namsize == 0) {
            namsize = (value1) < 256 ? 256 : 0x10000;
            nam = Xmallocarray(namsize, sizeof(char *));
            if (nam == NULL) {
                namsize = 0;
                goto error;
            }
        }
        else if (namsize <= value1) {
            namsize = 0x10000;
            if ((newnam = (char **) realloc(nam, namsize)) == NULL)
                goto error;
            nam = newnam;
        }
        if (first > last) {
            first = last = value1;
        }
        if (value1 < first) {
            for (i = value1; i < first; i++)
                nam[i] = NULL;
            first = value1;
        }
        if (value1 > last) {
            for (i = last + 1; i <= value1; i++)
                nam[i] = NULL;
            last = value1;
        }
        nam[value1] = strdup(keyword_value);
        if (nam[value1] == NULL) {
            goto error;
        }
        goto string_mapping;

    default:
        goto string_mapping;    /* ignore unknown lines */
    }

 done:
    if (encsize) {
        free(enc);
        encsize = 0;
        enc = NULL;
    }
    if (namsize) {
        free(nam);             /* don't free entries! */
        namsize = 0;
        nam = NULL;
    }

    encoding->aliases = NULL;
    if (numaliases) {
        encoding->aliases = Xmallocarray(numaliases + 1, sizeof(char *));
        if (encoding->aliases == NULL)
            goto error;
        for (i = 0; i < numaliases; i++)
            encoding->aliases[i] = aliases[i];
        encoding->aliases[numaliases] = NULL;
    }

    return encoding;

 error:
    if (encsize) {
        free(enc);
        encsize = 0;
    }
    if (namsize) {
        for (i = first; i <= last; i++)
            free(nam[i]);
        free(nam);
    }
    if (mapping) {
        free(mapping->client_data);
        free(mapping);
    }
    if (encoding) {
        FontMapPtr nextmap;

        free(encoding->name);
        for (mapping = encoding->mappings; mapping; mapping = nextmap) {
            free(mapping->client_data);
            nextmap = mapping->next;
            free(mapping);
        }
        free(encoding);
    }
    for (i = 0; i < numaliases; i++)
        free(aliases[i]);
    /* We don't need to free sn and sm as they handled locally in the body. */
    return NULL;
}

const char *
FontEncDirectory(void)
{
    static const char *dir = NULL;

    if (dir == NULL) {
        const char *c = getenv("FONT_ENCODINGS_DIRECTORY");

        if (c) {
            dir = strdup(c);
            if (!dir)
                return NULL;
        }
        else {
            dir = FONT_ENCODINGS_DIRECTORY;
        }
    }
    return dir;
}

static void
parseFontFileName(const char *fontFileName, char *buf, char *dir)
{
    const char *p;
    char *q, *lastslash;

    for (p = fontFileName, q = dir, lastslash = NULL; *p; p++, q++) {
        *q = *p;
        if (*p == '/')
            lastslash = q + 1;
    }

    if (!lastslash)
        lastslash = dir;

    *lastslash = '\0';

    if (buf && strlen(dir) + 14 < MAXFONTFILENAMELEN) {
        snprintf(buf, MAXFONTFILENAMELEN, "%s%s", dir, "encodings.dir");
    }
}

static FontEncPtr
FontEncReallyReallyLoad(const char *charset,
                        const char *dirname, const char *dir)
{
    FontFilePtr f;
    FILE *file;
    FontEncPtr encoding;
    char file_name[MAXFONTFILENAMELEN], encoding_name[MAXFONTNAMELEN],
        buf[MAXFONTFILENAMELEN];
    int count, n;
    static char format[24] = "";

    /* As we don't really expect to open encodings that often, we don't
       take the trouble of caching encodings directories. */

    if ((file = fopen(dirname, "r")) == NULL) {
        return NULL;
    }

    count = fscanf(file, "%d\n", &n);
    if (count == EOF || count != 1) {
        fclose(file);
        return NULL;
    }

    encoding = NULL;
    if (!format[0]) {
        snprintf(format, sizeof(format), "%%%ds %%%d[^\n]\n",
                 (int) sizeof(encoding_name) - 1, (int) sizeof(file_name) - 1);
    }
    for (;;) {
        count = fscanf(file, format, encoding_name, file_name);
        if (count == EOF)
            break;
        if (count != 2)
            break;

        if (!strcasecmp(encoding_name, charset)) {
            /* Found it */
            if (file_name[0] != '/') {
                if (strlen(dir) + strlen(file_name) >= MAXFONTFILENAMELEN) {
                    fclose(file);
                    return NULL;
                }
                snprintf(buf, MAXFONTFILENAMELEN, "%s%s", dir, file_name);
            }
            else {
                snprintf(buf, MAXFONTFILENAMELEN, "%s", file_name);
            }

            f = FontFileOpen(buf);
            if (f == NULL) {
                fclose(file);
                return NULL;
            }
            encoding = parseEncodingFile(f, 0);
            FontFileClose(f);
            break;
        }
    }

    fclose(file);

    return encoding;
}

/* Parser ntrypoint -- used by FontEncLoad */
FontEncPtr
FontEncReallyLoad(const char *charset, const char *fontFileName)
{
    FontEncPtr encoding;
    char dir[MAXFONTFILENAMELEN], dirname[MAXFONTFILENAMELEN];
    const char *d;

    if (fontFileName) {
        parseFontFileName(fontFileName, dirname, dir);
        encoding = FontEncReallyReallyLoad(charset, dirname, dir);
        if (encoding)
            return (encoding);
    }

    d = FontEncDirectory();
    if (d) {
        parseFontFileName(d, NULL, dir);
        encoding = FontEncReallyReallyLoad(charset, d, dir);
        return encoding;
    }

    return NULL;
}

/* Return a NULL-terminated array of encoding names.  Note that this
 * function has incestuous knowledge of the allocations done by
 * parseEncodingFile. */

char **
FontEncIdentify(const char *fileName)
{
    FontFilePtr f;
    FontEncPtr encoding;
    char **names, **name, **alias;
    int numaliases;

    if ((f = FontFileOpen(fileName)) == NULL) {
        return NULL;
    }
    encoding = parseEncodingFile(f, 1);
    FontFileClose(f);

    if (!encoding)
        return NULL;

    numaliases = 0;
    if (encoding->aliases)
        for (alias = encoding->aliases; *alias; alias++)
            numaliases++;

    names = Xmallocarray(numaliases + 2, sizeof(char *));
    if (names == NULL) {
        free(encoding->aliases);
        free(encoding);
        return NULL;
    }

    name = names;
    *(name++) = encoding->name;
    if (numaliases > 0)
        for (alias = encoding->aliases; *alias; alias++, name++)
            *name = *alias;

    *name = NULL;
    free(encoding->aliases);
    free(encoding);

    return names;
}
