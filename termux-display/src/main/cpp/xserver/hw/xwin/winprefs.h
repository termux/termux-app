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

#if !defined(WINPREFS_H)
#define WINPREFS_H

/* Need Bool */
#include <X11/Xdefs.h>
/* Need TRUE */
#include "misc.h"

/* Need to know how long paths can be... */
#include <limits.h>
/* Xwindows redefines PATH_MAX to at least 1024 */
#include <X11/Xwindows.h>

#include "winwindow.h"

#ifndef NAME_MAX
#define NAME_MAX PATH_MAX
#endif
#define MENU_MAX 128            /* Maximum string length of a menu name or item */
#define PARAM_MAX (4*PATH_MAX)  /* Maximum length of a parameter to a MENU */

/* Supported commands in a MENU {} statement */
typedef enum MENUCOMMANDTYPE {
    CMD_EXEC,                   /* /bin/sh -c the parameter            */
    CMD_MENU,                   /* Display a popup menu named param    */
    CMD_SEPARATOR,              /* Menu separator                      */
    CMD_ALWAYSONTOP,            /* Toggle always-on-top mode           */
    CMD_RELOAD                  /* Reparse the .XWINRC file            */
} MENUCOMMANDTYPE;

#define STYLE_NONE     (0L)     /* Dummy the first entry                      */
#define STYLE_NOTITLE  (1L)     /* Force window style no titlebar             */
#define STYLE_OUTLINE  (1L<<1)  /* Force window style just thin-line border   */
#define STYLE_NOFRAME  (1L<<2)  /* Force window style no frame                */
#define STYLE_TOPMOST  (1L<<3)  /* Open a window always-on-top                */
#define STYLE_MAXIMIZE (1L<<4)  /* Open a window maximized                    */
#define STYLE_MINIMIZE (1L<<5)  /* Open a window minimized                    */
#define STYLE_BOTTOM   (1L<<6)  /* Open a window at the bottom of the Z order */

/* Where to place a system menu */
typedef enum MENUPOSITION {
    AT_START,                   /* Place menu at the top of the system menu   */
    AT_END                      /* Put it at the bottom of the menu (default) */
} MENUPOSITION;

/* Menu item definitions */
typedef struct MENUITEM {
    char text[MENU_MAX + 1];    /* To be displayed in menu */
    MENUCOMMANDTYPE cmd;        /* What should it do? */
    char param[PARAM_MAX + 1];  /* Any parameters? */
    unsigned long commandID;    /* Windows WM_COMMAND ID assigned at runtime */
} MENUITEM;

/* A completely read in menu... */
typedef struct MENUPARSED {
    char menuName[MENU_MAX + 1];        /* What's it called in the text? */
    MENUITEM *menuItem;         /* Array of items */
    int menuItems;              /* How big's the array? */
} MENUPARSED;

/* To map between a window and a system menu to add for it */
typedef struct SYSMENUITEM {
    char match[MENU_MAX + 1];   /* String to look for to apply this sysmenu */
    char menuName[MENU_MAX + 1];        /* Which menu to show? Used to set *menu */
    MENUPOSITION menuPos;       /* Where to place it (ignored in root) */
} SYSMENUITEM;

/* To redefine icons for certain window types */
typedef struct ICONITEM {
    char match[MENU_MAX + 1];   /* What string to search for? */
    char iconFile[PATH_MAX + NAME_MAX + 2];     /* Icon location, WIN32 path */
    HICON hicon;                /* LoadImage() result */
} ICONITEM;

/* To redefine styles for certain window types */
typedef struct STYLEITEM {
    char match[MENU_MAX + 1];   /* What string to search for? */
    unsigned long type;         /* What should it do? */
} STYLEITEM;

typedef struct WINPREFS {
    /* Menu information */
    MENUPARSED *menu;           /* Array of created menus */
    int menuItems;              /* How big? */

    /* Taskbar menu settings */
    char rootMenuName[MENU_MAX + 1];    /* Menu for taskbar icon */

    /* System menu addition menus */
    SYSMENUITEM *sysMenu;
    int sysMenuItems;

    /* Which menu to add to unmatched windows? */
    char defaultSysMenuName[MENU_MAX + 1];
    MENUPOSITION defaultSysMenuPos;     /* Where to place it */

    /* Icon information */
    char iconDirectory[PATH_MAX + 1];   /* Where do the .icos lie? (Win32 path) */
    char defaultIconName[NAME_MAX + 1]; /* Replacement for x.ico */
    char trayIconName[NAME_MAX + 1];    /* Replacement for tray icon */

    ICONITEM *icon;
    int iconItems;

    STYLEITEM *style;
    int styleItems;

    /* Force exit flag */
    Bool fForceExit;

    /* Silent exit flag */
    Bool fSilentExit;

} WINPREFS;

/* The global pref settings structure loaded by the winprefyacc.y parser */
extern WINPREFS pref;

/* Functions */
void
 LoadPreferences(void);

void
 SetupRootMenu(HMENU root);

void
 SetupSysMenu(HWND hwnd);

void
 HandleCustomWM_INITMENU(HWND hwnd, HMENU hmenu);

Bool
 HandleCustomWM_COMMAND(HWND hwnd, WORD command, winPrivScreenPtr pScreenPriv);

int
 winIconIsOverride(HICON hicon);

HICON winOverrideIcon(char *res_name, char *res_class, char *wmName);

unsigned long
 winOverrideStyle(char *res_name, char *res_class, char *wmName);

HICON winTaskbarIcon(void);

HICON winOverrideDefaultIcon(int size);

HICON LoadImageComma(char *fname, char *iconDirectory, int sx, int sy, int flags);

#endif
