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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include "tokens.h"
#define	DEBUG_VAR	scanDebug
#include "utils.h"
#include "parseutils.h"

#ifdef DEBUG
unsigned int scanDebug;
#endif

static FILE *yyin;

static char scanFileBuf[1024] = {0};
char *scanFile = scanFileBuf;
int lineNum = 0;

int scanInt;

char scanBuf[1024];
static int scanStrLine = 0;

#define	BUFSIZE	4096
static char readBuf[BUFSIZE];
static int readBufPos = 0;
static int readBufLen = 0;

#ifdef DEBUG
extern int debugFlags;

static char *
tokText(int tok)
{
    static char buf[32];

    switch (tok)
    {
    case END_OF_FILE:
        snprintf(buf, sizeof(buf), "END_OF_FILE");
        break;
    case ERROR_TOK:
        snprintf(buf, sizeof(buf), "ERROR");
        break;

    case XKB_KEYMAP:
        snprintf(buf, sizeof(buf), "XKB_KEYMAP");
        break;
    case XKB_KEYCODES:
        snprintf(buf, sizeof(buf), "XKB_KEYCODES");
        break;
    case XKB_TYPES:
        snprintf(buf, sizeof(buf), "XKB_TYPES");
        break;
    case XKB_SYMBOLS:
        snprintf(buf, sizeof(buf), "XKB_SYMBOLS");
        break;
    case XKB_COMPATMAP:
        snprintf(buf, sizeof(buf), "XKB_COMPATMAP");
        break;
    case XKB_GEOMETRY:
        snprintf(buf, sizeof(buf), "XKB_GEOMETRY");
        break;
    case XKB_SEMANTICS:
        snprintf(buf, sizeof(buf), "XKB_SEMANTICS");
        break;
    case XKB_LAYOUT:
        snprintf(buf, sizeof(buf), "XKB_LAYOUT");
        break;

    case INCLUDE:
        snprintf(buf, sizeof(buf), "INCLUDE");
        break;
    case OVERRIDE:
        snprintf(buf, sizeof(buf), "OVERRIDE");
        break;
    case AUGMENT:
        snprintf(buf, sizeof(buf), "AUGMENT");
        break;
    case REPLACE:
        snprintf(buf, sizeof(buf), "REPLACE");
        break;
    case ALTERNATE:
        snprintf(buf, sizeof(buf), "ALTERNATE");
        break;

    case VIRTUAL_MODS:
        snprintf(buf, sizeof(buf), "VIRTUAL_MODS");
        break;
    case TYPE:
        snprintf(buf, sizeof(buf), "TYPE");
        break;
    case INTERPRET:
        snprintf(buf, sizeof(buf), "INTERPRET");
        break;
    case ACTION_TOK:
        snprintf(buf, sizeof(buf), "ACTION");
        break;
    case KEY:
        snprintf(buf, sizeof(buf), "KEY");
        break;
    case ALIAS:
        snprintf(buf, sizeof(buf), "ALIAS");
        break;
    case GROUP:
        snprintf(buf, sizeof(buf), "GROUP");
        break;
    case MODIFIER_MAP:
        snprintf(buf, sizeof(buf), "MODIFIER_MAP");
        break;
    case INDICATOR:
        snprintf(buf, sizeof(buf), "INDICATOR");
        break;
    case SHAPE:
        snprintf(buf, sizeof(buf), "SHAPE");
        break;
    case KEYS:
        snprintf(buf, sizeof(buf), "KEYS");
        break;
    case ROW:
        snprintf(buf, sizeof(buf), "ROW");
        break;
    case SECTION:
        snprintf(buf, sizeof(buf), "SECTION");
        break;
    case OVERLAY:
        snprintf(buf, sizeof(buf), "OVERLAY");
        break;
    case TEXT:
        snprintf(buf, sizeof(buf), "TEXT");
        break;
    case OUTLINE:
        snprintf(buf, sizeof(buf), "OUTLINE");
        break;
    case SOLID:
        snprintf(buf, sizeof(buf), "SOLID");
        break;
    case LOGO:
        snprintf(buf, sizeof(buf), "LOGO");
        break;
    case VIRTUAL:
        snprintf(buf, sizeof(buf), "VIRTUAL");
        break;

    case EQUALS:
        snprintf(buf, sizeof(buf), "EQUALS");
        break;
    case PLUS:
        snprintf(buf, sizeof(buf), "PLUS");
        break;
    case MINUS:
        snprintf(buf, sizeof(buf), "MINUS");
        break;
    case DIVIDE:
        snprintf(buf, sizeof(buf), "DIVIDE");
        break;
    case TIMES:
        snprintf(buf, sizeof(buf), "TIMES");
        break;
    case OBRACE:
        snprintf(buf, sizeof(buf), "OBRACE");
        break;
    case CBRACE:
        snprintf(buf, sizeof(buf), "CBRACE");
        break;
    case OPAREN:
        snprintf(buf, sizeof(buf), "OPAREN");
        break;
    case CPAREN:
        snprintf(buf, sizeof(buf), "CPAREN");
        break;
    case OBRACKET:
        snprintf(buf, sizeof(buf), "OBRACKET");
        break;
    case CBRACKET:
        snprintf(buf, sizeof(buf), "CBRACKET");
        break;
    case DOT:
        snprintf(buf, sizeof(buf), "DOT");
        break;
    case COMMA:
        snprintf(buf, sizeof(buf), "COMMA");
        break;
    case SEMI:
        snprintf(buf, sizeof(buf), "SEMI");
        break;
    case EXCLAM:
        snprintf(buf, sizeof(buf), "EXCLAM");
        break;
    case INVERT:
        snprintf(buf, sizeof(buf), "INVERT");
        break;

    case STRING:
        snprintf(buf, sizeof(buf), "STRING (%s)", scanBuf);
        break;
    case INTEGER:
        snprintf(buf, sizeof(buf), "INTEGER (0x%x)", scanInt);
        break;
    case FLOAT:
        snprintf(buf, sizeof(buf), "FLOAT (%d.%d)",
                scanInt / XkbGeomPtsPerMM, scanInt % XkbGeomPtsPerMM);
        break;
    case IDENT:
        snprintf(buf, sizeof(buf), "IDENT (%s)", scanBuf);
        break;
    case KEYNAME:
        snprintf(buf, sizeof(buf), "KEYNAME (%s)", scanBuf);
        break;

    case PARTIAL:
        snprintf(buf, sizeof(buf), "PARTIAL");
        break;
    case DEFAULT:
        snprintf(buf, sizeof(buf), "DEFAULT");
        break;
    case HIDDEN:
        snprintf(buf, sizeof(buf), "HIDDEN");
        break;

    case ALPHANUMERIC_KEYS:
        snprintf(buf, sizeof(buf), "ALPHANUMERIC_KEYS");
        break;
    case MODIFIER_KEYS:
        snprintf(buf, sizeof(buf), "MODIFIER_KEYS");
        break;
    case KEYPAD_KEYS:
        snprintf(buf, sizeof(buf), "KEYPAD_KEYS");
        break;
    case FUNCTION_KEYS:
        snprintf(buf, sizeof(buf), "FUNCTION_KEYS");
        break;
    case ALTERNATE_GROUP:
        snprintf(buf, sizeof(buf), "ALTERNATE_GROUP");
        break;

    default:
        snprintf(buf, sizeof(buf), "UNKNOWN");
        break;
    }
    return buf;
}
#endif

void
scan_set_file(FILE *file)
{
    readBufLen = 0;
    readBufPos = 0;
    yyin = file;
}

static int
scanchar(void)
{
    if (readBufPos >= readBufLen) {
        readBufLen = fread(readBuf, 1, BUFSIZE, yyin);
        readBufPos = 0;
        if (!readBufLen)
            return EOF;
        if (feof(yyin))
            readBuf[readBufLen] = EOF;
    }

    return readBuf[readBufPos++];
}

static void
unscanchar(int c)
{
    if (readBuf[--readBufPos] != c) {
        fprintf(stderr, "UNGETCHAR FAILED! Put back %c, was expecting %c at "
                        "position %d, buf is '%s'\n", c, readBuf[readBufPos],
                        readBufPos, readBuf);
        _exit(94);
    }
}

int
setScanState(char *file, int line)
{
    if (file != NULL)
        strncpy(scanFile, file, 1024);
    if (line >= 0)
        lineNum = line;
    return 1;
}

static int
yyGetString(void)
{
    int ch, i;

    i = 0;
    while (((ch = scanchar()) != EOF) && (ch != '"'))
    {
        if (ch == '\\')
        {
            if ((ch = scanchar()) != EOF)
            {
                if (ch == 'n')
                    ch = '\n';
                else if (ch == 't')
                    ch = '\t';
                else if (ch == 'v')
                    ch = '\v';
                else if (ch == 'b')
                    ch = '\b';
                else if (ch == 'r')
                    ch = '\r';
                else if (ch == 'f')
                    ch = '\f';
                else if (ch == 'e')
                    ch = '\033';
                else if (ch == '0')
                {
                    int tmp, stop;
                    ch = stop = 0;
                    if (((tmp = scanchar()) != EOF) && (isdigit(tmp))
                        && (tmp != '8') && (tmp != '9'))
                    {
                        ch = (ch * 8) + (tmp - '0');
                    }
                    else
                    {
                        stop = 1;
                        unscanchar(tmp);
                    }
                    if (!stop)
                    {
                        if (((tmp = scanchar()) != EOF)
                            && (isdigit(tmp)) && (tmp != '8') && (tmp != '9'))
                        {
                            ch = (ch * 8) + (tmp - '0');
                        }
                        else
                        {
                            stop = 1;
                            unscanchar(tmp);
                        }
                    }
                    if (!stop)
                    {
                        if (((tmp = scanchar()) != EOF)
                            && (isdigit(tmp)) && (tmp != '8') && (tmp != '9'))
                        {
                            ch = (ch * 8) + (tmp - '0');
                        }
                        else
                        {
                            stop = 1;
                            unscanchar(tmp);
                        }
                    }
                }
            }
            else
                return ERROR_TOK;
        }
        if (i < sizeof(scanBuf) - 1)
            scanBuf[i++] = ch;
    }
    scanBuf[i] = '\0';
    if (ch == '"')
    {
        scanStrLine = lineNum;
        return STRING;
    }
    return ERROR_TOK;
}

static int
yyGetKeyName(void)
{
    int ch, i;

    i = 0;
    while (((ch = scanchar()) != EOF) && (ch != '>'))
    {
        if (ch == '\\')
        {
            if ((ch = scanchar()) != EOF)
            {
                if (ch == 'n')
                    ch = '\n';
                else if (ch == 't')
                    ch = '\t';
                else if (ch == 'v')
                    ch = '\v';
                else if (ch == 'b')
                    ch = '\b';
                else if (ch == 'r')
                    ch = '\r';
                else if (ch == 'f')
                    ch = '\f';
                else if (ch == 'e')
                    ch = '\033';
                else if (ch == '0')
                {
                    int tmp, stop;
                    ch = stop = 0;
                    if (((tmp = scanchar()) != EOF) && (isdigit(tmp))
                        && (tmp != '8') && (tmp != '9'))
                    {
                        ch = (ch * 8) + (tmp - '0');
                    }
                    else
                    {
                        stop = 1;
                        unscanchar(tmp);
                    }
                    if ((!stop) && ((tmp = scanchar()) != EOF)
                        && (isdigit(tmp)) && (tmp != '8') && (tmp != '9'))
                    {
                        ch = (ch * 8) + (tmp - '0');
                    }
                    else
                    {
                        stop = 1;
                        unscanchar(tmp);
                    }
                    if ((!stop) && ((tmp = scanchar()) != EOF)
                        && (isdigit(tmp)) && (tmp != '8') && (tmp != '9'))
                    {
                        ch = (ch * 8) + (tmp - '0');
                    }
                    else
                    {
                        stop = 1;
                        unscanchar(tmp);
                    }
                }
            }
            else
                return ERROR_TOK;
        }
        if (i < sizeof(scanBuf) - 1)
            scanBuf[i++] = ch;
    }
    scanBuf[i] = '\0';
    if ((ch == '>') && (i < 5))
    {
        scanStrLine = lineNum;
        return KEYNAME;
    }
    return ERROR_TOK;
}

static struct _Keyword
{
    const char *keyword;
    int token;
} keywords[] =
{
    {
    "xkb_keymap", XKB_KEYMAP},
    {
    "xkb_keycodes", XKB_KEYCODES},
    {
    "xkb_types", XKB_TYPES},
    {
    "xkb_symbols", XKB_SYMBOLS},
    {
    "xkb_compat", XKB_COMPATMAP},
    {
    "xkb_compat_map", XKB_COMPATMAP},
    {
    "xkb_compatibility", XKB_COMPATMAP},
    {
    "xkb_compatibility_map", XKB_COMPATMAP},
    {
    "xkb_geometry", XKB_GEOMETRY},
    {
    "xkb_semantics", XKB_SEMANTICS},
    {
    "xkb_layout", XKB_LAYOUT},
    {
    "include", INCLUDE},
    {
    "override", OVERRIDE},
    {
    "augment", AUGMENT},
    {
    "replace", REPLACE},
    {
    "alternate", ALTERNATE},
    {
    "partial", PARTIAL},
    {
    "default", DEFAULT},
    {
    "hidden", HIDDEN},
    {
    "virtual_modifiers", VIRTUAL_MODS},
    {
    "type", TYPE},
    {
    "interpret", INTERPRET},
    {
    "action", ACTION_TOK},
    {
    "key", KEY},
    {
    "alias", ALIAS},
    {
    "group", GROUP},
    {
    "modmap", MODIFIER_MAP},
    {
    "mod_map", MODIFIER_MAP},
    {
    "modifier_map", MODIFIER_MAP},
    {
    "indicator", INDICATOR},
    {
    "shape", SHAPE},
    {
    "row", ROW},
    {
    "keys", KEYS},
    {
    "section", SECTION},
    {
    "overlay", OVERLAY},
    {
    "text", TEXT},
    {
    "outline", OUTLINE},
    {
    "solid", SOLID},
    {
    "logo", LOGO},
    {
    "virtual", VIRTUAL},
    {
    "alphanumeric_keys", ALPHANUMERIC_KEYS},
    {
    "modifier_keys", MODIFIER_KEYS},
    {
    "keypad_keys", KEYPAD_KEYS},
    {
    "function_keys", FUNCTION_KEYS},
    {
    "alternate_group", ALTERNATE_GROUP}
};
static int numKeywords = sizeof(keywords) / sizeof(struct _Keyword);

static int
yyGetIdent(int first)
{
    int ch, j, found;
    int rtrn = IDENT;

    scanBuf[0] = first;
    j = 1;
    while (((ch = scanchar()) != EOF) && (isalnum(ch) || (ch == '_')))
    {
        if (j < sizeof(scanBuf) - 1)
            scanBuf[j++] = ch;
    }
    scanBuf[j++] = '\0';
    found = 0;

    for (int i = 0; (!found) && (i < numKeywords); i++)
    {
        if (uStrCaseCmp(scanBuf, keywords[i].keyword) == 0)
        {
            rtrn = keywords[i].token;
            found = 1;
        }
    }
    if (!found)
    {
        scanStrLine = lineNum;
        rtrn = IDENT;
    }

    if ((ch != EOF) && (!isspace(ch)))
        unscanchar(ch);
    else if (ch == '\n')
        lineNum++;

    return rtrn;
}

static int
yyGetNumber(int ch)
{
    const int nMaxBuffSize = 1024;
    int isFloat = 0;
    char buf[nMaxBuffSize];
    int nInBuf = 0;

    buf[0] = ch;
    nInBuf = 1;
    while (((ch = scanchar()) != EOF)
           && (isxdigit(ch) || ((nInBuf == 1) && (ch == 'x')))
           && nInBuf < (nMaxBuffSize - 1))
    {
        buf[nInBuf++] = ch;
    }
    if ((ch == '.') && (nInBuf < (nMaxBuffSize - 1)))
    {
        isFloat = 1;
        buf[nInBuf++] = ch;
        while (((ch = scanchar()) != EOF) && (isxdigit(ch))
               && nInBuf < (nMaxBuffSize - 1))
        {
            buf[nInBuf++] = ch;
        }
    }
    buf[nInBuf++] = '\0';
    if ((ch != EOF) && (!isspace(ch)))
        unscanchar(ch);

    if (isFloat)
    {
        float tmp;
        if (sscanf(buf, "%g", &tmp) == 1)
        {
            scanInt = tmp * XkbGeomPtsPerMM;
            return FLOAT;
        }
    }
    else if (sscanf(buf, "%i", &scanInt) == 1)
        return INTEGER;
    fprintf(stderr, "Malformed number %s\n", buf);
    return ERROR_TOK;
}

int
yylex(void)
{
    int ch;
    int rtrn;

    do
    {
        ch = scanchar();
        if (ch == '\n')
        {
            lineNum++;
        }
        else if (ch == '#')
        {                       /* handle shell style '#' comments */
            do
            {
                ch = scanchar();
            }
            while ((ch != '\n') && (ch != EOF));
            lineNum++;
        }
        else if (ch == '/')
        {                       /* handle C++ style double-/ comments */
            int newch = scanchar();
            if (newch == '/')
            {
                do
                {
                    ch = scanchar();
                }
                while ((ch != '\n') && (ch != EOF));
                lineNum++;
            }
            else if (newch != EOF)
            {
                unscanchar(newch);
            }
        }
    }
    while ((ch != EOF) && (isspace(ch)));
    if (ch == '=')
        rtrn = EQUALS;
    else if (ch == '+')
        rtrn = PLUS;
    else if (ch == '-')
        rtrn = MINUS;
    else if (ch == '/')
        rtrn = DIVIDE;
    else if (ch == '*')
        rtrn = TIMES;
    else if (ch == '{')
        rtrn = OBRACE;
    else if (ch == '}')
        rtrn = CBRACE;
    else if (ch == '(')
        rtrn = OPAREN;
    else if (ch == ')')
        rtrn = CPAREN;
    else if (ch == '[')
        rtrn = OBRACKET;
    else if (ch == ']')
        rtrn = CBRACKET;
    else if (ch == '.')
        rtrn = DOT;
    else if (ch == ',')
        rtrn = COMMA;
    else if (ch == ';')
        rtrn = SEMI;
    else if (ch == '!')
        rtrn = EXCLAM;
    else if (ch == '~')
        rtrn = INVERT;
    else if (ch == '"')
        rtrn = yyGetString();
    else if (ch == '<')
        rtrn = yyGetKeyName();
    else if (isalpha(ch) || (ch == '_'))
        rtrn = yyGetIdent(ch);
    else if (isdigit(ch))
        rtrn = yyGetNumber(ch);
    else if (ch == EOF)
        rtrn = END_OF_FILE;
    else
    {
#ifdef DEBUG
        if (debugFlags)
            fprintf(stderr,
                    "Unexpected character %c (%d) in input stream\n", ch, ch);
#endif
        rtrn = ERROR_TOK;
    }
#ifdef DEBUG
    if (debugFlags & 0x2)
        fprintf(stderr, "scan: %s\n", tokText(rtrn));
#endif
    return rtrn;
}
