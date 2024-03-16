%{ # -*- C -*-
/*
 * Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 * Copyright (C) Colin Harrison 2005-2008
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * from the XFree86 Project.
 *
 * Authors:     Earle F. Philhower, III
 *              Colin Harrison
 */
/* $XFree86: $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "winprefsyacc.h"

extern void ErrorF (const char* /*f*/, ...);

/* Copy the parsed string, must be free()d in yacc parser */
static char *makestr(char *str)
{
  char *ptr;
  ptr = malloc(strlen(str)+1);
  if (!ptr)
    {
      ErrorF ("winMultiWindowLex:makestr() out of memory\n");
      exit (-1);
    }
  strcpy(ptr, str);
  return ptr;
}

%}

%option yylineno
%option nounput
%option noinput
%option never-interactive

%%
\#.*[\r\n]              { /* comment */ return NEWLINE; }
\/\/.*[\r\n]            { /* comment */ return NEWLINE; }
[\r\n]                  { return NEWLINE; }
[ \t]+                  { /* ignore whitespace */ }
MENU                    { return MENU; }
ICONDIRECTORY           { return ICONDIRECTORY; }
DEFAULTICON             { return DEFAULTICON; }
ICONS                   { return ICONS; }
STYLES                  { return STYLES; }
TOPMOST                 { return TOPMOST; }
MAXIMIZE                { return MAXIMIZE; }
MINIMIZE                { return MINIMIZE; }
BOTTOM                  { return BOTTOM; }
NOTITLE                 { return NOTITLE; }
OUTLINE                 { return OUTLINE; }
NOFRAME                 { return NOFRAME; }
ROOTMENU                { return ROOTMENU; }
DEFAULTSYSMENU          { return DEFAULTSYSMENU; }
SYSMENU                 { return SYSMENU; }
SEPARATOR               { return SEPARATOR; }
ATSTART                 { return ATSTART; }
ATEND                   { return ATEND; }
EXEC                    { return EXEC; }
ALWAYSONTOP             { return ALWAYSONTOP; }
DEBUG                   { return DEBUGOUTPUT; }
RELOAD                  { return RELOAD; }
TRAYICON                { return TRAYICON; }
FORCEEXIT		{ return FORCEEXIT; }
SILENTEXIT		{ return SILENTEXIT; }
"{"                     { return LB; }
"}"                     { return RB; }
"\""[^\"\r\n]+"\""      { yylval.sVal = makestr(yytext+1); \
                          yylval.sVal[strlen(yylval.sVal)-1] = 0; \
                          return STRING; }
[^ \t\r\n]+             { yylval.sVal = makestr(yytext); \
                          return STRING; }
%%

/*
 * Run-of-the mill requirement for yacc
 */
int
yywrap (void)
{
  return 1;
}

/*
 * Run a file through the yacc parser
 */
int
parse_file (FILE *file)
{
  int ret;

  if (!file)
    return 1;
  
  yylineno = 1;
  yyin = file;
  ret = yyparse ();
  yylex_destroy ();
  return ret;
}

