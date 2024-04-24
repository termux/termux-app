%{
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

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#define _STDLIB_H 1 /* bison checks this to know if stdlib has been included */
#include <string.h>
#include "winprefs.h"

/* The following give better error messages in bison at the cost of a few KB */
#define YYERROR_VERBOSE 1

/* YYLTYPE_IS_TRIVIAL and YYENABLE_NLS defined to suppress warnings */
#define YYLTYPE_IS_TRIVIAL 1
#define YYENABLE_NLS 0

/* The global pref settings */
WINPREFS pref;

/* The working menu */  
static MENUPARSED menu;

/* Functions for parsing the tokens into out structure */
/* Defined at the end section of this file */

static void SetIconDirectory (char *path);
static void SetDefaultIcon (char *fname);
static void SetRootMenu (char *menu);
static void SetDefaultSysMenu (char *menu, int pos);
static void SetTrayIcon (char *fname);

static void OpenMenu(char *menuname);
static void AddMenuLine(const char *name, MENUCOMMANDTYPE cmd, const char *param);
static void CloseMenu(void);

static void OpenIcons(void);
static void AddIconLine(char *matchstr, char *iconfile);
static void CloseIcons(void);

static void OpenStyles(void);
static void AddStyleLine(char *matchstr, unsigned long style);
static void CloseStyles(void);

static void OpenSysMenu(void);
static void AddSysMenuLine(char *matchstr, char *menuname, int pos);
static void CloseSysMenu(void);

static int yyerror (const char *s);

extern char *yytext;
extern int yylineno;
extern int yylex(void);

%}

%union {
  char *sVal;
  unsigned long uVal;
  int iVal;
}

%token NEWLINE
%token MENU
%token LB
%token RB
%token ICONDIRECTORY
%token DEFAULTICON
%token ICONS
%token STYLES
%token TOPMOST
%token MAXIMIZE
%token MINIMIZE
%token BOTTOM
%token NOTITLE
%token OUTLINE
%token NOFRAME
%token DEFAULTSYSMENU
%token SYSMENU
%token ROOTMENU
%token SEPARATOR
%token ATSTART
%token ATEND
%token EXEC
%token ALWAYSONTOP
%token DEBUGOUTPUT "DEBUG"
%token RELOAD
%token TRAYICON
%token FORCEEXIT
%token SILENTEXIT

%token <sVal> STRING
%type <uVal>  group1
%type <uVal>  group2
%type <uVal>  stylecombo
%type <iVal>  atspot

%%

input:	/* empty */
	| input line
	;

line:	NEWLINE
	| command
	;


newline_or_nada:	
	| NEWLINE newline_or_nada
	;

command:	defaulticon
	| icondirectory
	| menu
	| icons
	| styles
	| sysmenu
	| rootmenu
	| defaultsysmenu
	| debug
	| trayicon
	| forceexit
	| silentexit
	;

trayicon:	TRAYICON STRING NEWLINE { SetTrayIcon($2); free($2); }
	;

rootmenu:	ROOTMENU STRING NEWLINE { SetRootMenu($2); free($2); }
	;

defaultsysmenu:	DEFAULTSYSMENU STRING atspot NEWLINE { SetDefaultSysMenu($2, $3); free($2); }
	;

defaulticon:	DEFAULTICON STRING NEWLINE { SetDefaultIcon($2); free($2); }
	;

icondirectory:	ICONDIRECTORY STRING NEWLINE { SetIconDirectory($2); free($2); }
	;

menuline:	SEPARATOR NEWLINE newline_or_nada  { AddMenuLine("-", CMD_SEPARATOR, ""); }
	| STRING ALWAYSONTOP NEWLINE newline_or_nada  { AddMenuLine($1, CMD_ALWAYSONTOP, ""); free($1); }
	| STRING EXEC STRING NEWLINE newline_or_nada  { AddMenuLine($1, CMD_EXEC, $3); free($1); free($3); }
	| STRING MENU STRING NEWLINE newline_or_nada  { AddMenuLine($1, CMD_MENU, $3); free($1); free($3); }
	| STRING RELOAD NEWLINE newline_or_nada  { AddMenuLine($1, CMD_RELOAD, ""); free($1); }
	;

menulist:	menuline
	| menuline menulist
	;

menu:	MENU STRING LB { OpenMenu($2); free($2); } newline_or_nada menulist RB {CloseMenu();}
	;

iconline:	STRING STRING NEWLINE newline_or_nada { AddIconLine($1, $2); free($1); free($2); }
	;

iconlist:	iconline
	| iconline iconlist
	;

icons:	ICONS LB {OpenIcons();} newline_or_nada iconlist RB {CloseIcons();}
	;

group1:	TOPMOST { $$=STYLE_TOPMOST; }
	| MAXIMIZE { $$=STYLE_MAXIMIZE; }
	| MINIMIZE { $$=STYLE_MINIMIZE; }
	| BOTTOM { $$=STYLE_BOTTOM; }
	;

group2:	NOTITLE { $$=STYLE_NOTITLE; }
	| OUTLINE { $$=STYLE_OUTLINE; }
	| NOFRAME { $$=STYLE_NOFRAME; }
	;

stylecombo:	group1 { $$=$1; }
	| group2 { $$=$1; }
	| group1 group2 { $$=$1|$2; }
	| group2 group1 { $$=$1|$2; }
	;

styleline:	STRING stylecombo NEWLINE newline_or_nada { AddStyleLine($1, $2); free($1); }
	;

stylelist:	styleline
	| styleline stylelist
	;

styles:	STYLES LB {OpenStyles();} newline_or_nada stylelist RB {CloseStyles();}
	;

atspot:	{ $$=AT_END; }
	| ATSTART { $$=AT_START; }
	| ATEND { $$=AT_END; }
	;

sysmenuline:	STRING STRING atspot NEWLINE newline_or_nada { AddSysMenuLine($1, $2, $3); free($1); free($2); }
	;

sysmenulist:	sysmenuline
	| sysmenuline sysmenulist
	;

sysmenu:	SYSMENU LB NEWLINE {OpenSysMenu();} newline_or_nada sysmenulist RB {CloseSysMenu();}
	;

forceexit:	FORCEEXIT NEWLINE { pref.fForceExit = TRUE; }
	;

silentexit:	SILENTEXIT NEWLINE { pref.fSilentExit = TRUE; }
	;

debug: 	DEBUGOUTPUT STRING NEWLINE { ErrorF("LoadPreferences: %s\n", $2); free($2); }
	;


%%
/*
 * Errors in parsing abort and print log messages
 */
static int
yyerror (const char *s)
{
  ErrorF("LoadPreferences: %s line %d\n", s, yylineno);
  return 1;
}

/* Miscellaneous functions to store TOKENs into the structure */
static void
SetIconDirectory (char *path)
{
  strncpy (pref.iconDirectory, path, PATH_MAX);
  pref.iconDirectory[PATH_MAX] = 0;
}

static void
SetDefaultIcon (char *fname)
{
  strncpy (pref.defaultIconName, fname, NAME_MAX);
  pref.defaultIconName[NAME_MAX] = 0;
}

static void
SetTrayIcon (char *fname)
{
  strncpy (pref.trayIconName, fname, NAME_MAX);
  pref.trayIconName[NAME_MAX] = 0;
}

static void
SetRootMenu (char *menuname)
{
  strncpy (pref.rootMenuName, menuname, MENU_MAX);
  pref.rootMenuName[MENU_MAX] = 0;
}

static void
SetDefaultSysMenu (char *menuname, int pos)
{
  strncpy (pref.defaultSysMenuName, menuname, MENU_MAX);
  pref.defaultSysMenuName[MENU_MAX] = 0;
  pref.defaultSysMenuPos = pos;
}

static void
OpenMenu (char *menuname)
{
  if (menu.menuItem) free(menu.menuItem);
  menu.menuItem = NULL;
  strncpy(menu.menuName, menuname, MENU_MAX);
  menu.menuName[MENU_MAX] = 0;
  menu.menuItems = 0;
}

static void
AddMenuLine (const char *text, MENUCOMMANDTYPE cmd, const char *param)
{
  if (menu.menuItem==NULL)
    menu.menuItem = malloc(sizeof(MENUITEM));
  else
    menu.menuItem = realloc(menu.menuItem, sizeof(MENUITEM)*(menu.menuItems+1));

  strncpy (menu.menuItem[menu.menuItems].text, text, MENU_MAX);
  menu.menuItem[menu.menuItems].text[MENU_MAX] = 0;

  menu.menuItem[menu.menuItems].cmd = cmd;

  strncpy(menu.menuItem[menu.menuItems].param, param, PARAM_MAX);
  menu.menuItem[menu.menuItems].param[PARAM_MAX] = 0;

  menu.menuItem[menu.menuItems].commandID = 0;

  menu.menuItems++;
}

static void
CloseMenu (void)
{
  if (menu.menuItem==NULL || menu.menuItems==0)
    {
      ErrorF("LoadPreferences: Empty menu detected\n");
      return;
    }
  
  if (pref.menuItems)
    pref.menu = realloc (pref.menu, (pref.menuItems+1)*sizeof(MENUPARSED));
  else
    pref.menu = malloc (sizeof(MENUPARSED));
  
  memcpy (pref.menu+pref.menuItems, &menu, sizeof(MENUPARSED));
  pref.menuItems++;

  memset (&menu, 0, sizeof(MENUPARSED));
}

static void 
OpenIcons (void)
{
  if (pref.icon != NULL) {
    ErrorF("LoadPreferences: Redefining icon mappings\n");
    free(pref.icon);
    pref.icon = NULL;
  }
  pref.iconItems = 0;
}

static void
AddIconLine (char *matchstr, char *iconfile)
{
  if (pref.icon==NULL)
    pref.icon = malloc(sizeof(ICONITEM));
  else
    pref.icon = realloc(pref.icon, sizeof(ICONITEM)*(pref.iconItems+1));

  strncpy(pref.icon[pref.iconItems].match, matchstr, MENU_MAX);
  pref.icon[pref.iconItems].match[MENU_MAX] = 0;

  strncpy(pref.icon[pref.iconItems].iconFile, iconfile, PATH_MAX+NAME_MAX+1);
  pref.icon[pref.iconItems].iconFile[PATH_MAX+NAME_MAX+1] = 0;

  pref.icon[pref.iconItems].hicon = 0;

  pref.iconItems++;
}

static void 
CloseIcons (void)
{
}

static void
OpenStyles (void)
{
  if (pref.style != NULL) {
    ErrorF("LoadPreferences: Redefining window style\n");
    free(pref.style);
    pref.style = NULL;
  }
  pref.styleItems = 0;
}

static void
AddStyleLine (char *matchstr, unsigned long style)
{
  if (pref.style==NULL)
    pref.style = malloc(sizeof(STYLEITEM));
  else
    pref.style = realloc(pref.style, sizeof(STYLEITEM)*(pref.styleItems+1));

  strncpy(pref.style[pref.styleItems].match, matchstr, MENU_MAX);
  pref.style[pref.styleItems].match[MENU_MAX] = 0;

  pref.style[pref.styleItems].type = style;

  pref.styleItems++;
}

static void
CloseStyles (void)
{
}

static void
OpenSysMenu (void)
{
  if (pref.sysMenu != NULL) {
    ErrorF("LoadPreferences: Redefining system menu\n");
    free(pref.sysMenu);
    pref.sysMenu = NULL;
  }
  pref.sysMenuItems = 0;
}

static void
AddSysMenuLine (char *matchstr, char *menuname, int pos)
{
  if (pref.sysMenu==NULL)
    pref.sysMenu = malloc(sizeof(SYSMENUITEM));
  else
    pref.sysMenu = realloc(pref.sysMenu, sizeof(SYSMENUITEM)*(pref.sysMenuItems+1));

  strncpy (pref.sysMenu[pref.sysMenuItems].match, matchstr, MENU_MAX);
  pref.sysMenu[pref.sysMenuItems].match[MENU_MAX] = 0;

  strncpy (pref.sysMenu[pref.sysMenuItems].menuName, menuname, MENU_MAX);
  pref.sysMenu[pref.sysMenuItems].menuName[MENU_MAX] = 0;

  pref.sysMenu[pref.sysMenuItems].menuPos = pos;

  pref.sysMenuItems++;
}

static void
CloseSysMenu (void)
{
}

