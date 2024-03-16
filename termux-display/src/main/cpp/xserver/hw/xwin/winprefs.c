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

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#ifdef __CYGWIN__
#include <sys/resource.h>
#include <sys/cygwin.h>
#endif
#include "win.h"

#include <X11/Xwindows.h>
#include <shellapi.h>

#include "winprefs.h"
#include "windisplay.h"
#include "winmultiwindowclass.h"
#include "winmultiwindowicons.h"

/* Where will the custom menu commands start counting from? */
#define STARTMENUID WM_USER

extern const char *winGetBaseDir(void);

/* From winprefslex.l, the real parser */
extern int parse_file(FILE * fp);

/* Currently in use command ID, incremented each new menu item created */
static int g_cmdid = STARTMENUID;

/*
 * Creates or appends a menu from a MENUPARSED structure
 */
static HMENU
MakeMenu(char *name, HMENU editMenu, int editItem)
{
    int i;
    int item;
    MENUPARSED *m;
    HMENU hmenu, hsub;

    for (i = 0; i < pref.menuItems; i++) {
        if (!strcmp(name, pref.menu[i].menuName))
            break;
    }

    /* Didn't find a match, bummer */
    if (i == pref.menuItems) {
        ErrorF("MakeMenu: Can't find menu %s\n", name);
        return NULL;
    }

    m = &(pref.menu[i]);

    if (editMenu) {
        hmenu = editMenu;
        item = editItem;
    }
    else {
        hmenu = CreatePopupMenu();
        if (!hmenu) {
            ErrorF("MakeMenu: Unable to CreatePopupMenu() %s\n", name);
            return NULL;
        }
        item = 0;
    }

    /* Add the menu items */
    for (i = 0; i < m->menuItems; i++) {
        /* Only assign IDs one time... */
        if (m->menuItem[i].commandID == 0)
            m->menuItem[i].commandID = g_cmdid++;

        switch (m->menuItem[i].cmd) {
        case CMD_EXEC:
        case CMD_ALWAYSONTOP:
        case CMD_RELOAD:
            InsertMenu(hmenu,
                       item,
                       MF_BYPOSITION | MF_ENABLED | MF_STRING,
                       m->menuItem[i].commandID, m->menuItem[i].text);
            break;

        case CMD_SEPARATOR:
            InsertMenu(hmenu, item, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
            break;

        case CMD_MENU:
            /* Recursive! */
            hsub = MakeMenu(m->menuItem[i].param, 0, 0);
            if (hsub)
                InsertMenu(hmenu,
                           item,
                           MF_BYPOSITION | MF_POPUP | MF_ENABLED | MF_STRING,
                           (UINT_PTR) hsub, m->menuItem[i].text);
            break;
        }

        /* If item==-1 (means to add at end of menu) don't increment) */
        if (item >= 0)
            item++;
    }

    return hmenu;
}

/*
 * Callback routine that is executed once per window class.
 * Removes or creates custom window settings depending on LPARAM
 */
static wBOOL CALLBACK
ReloadEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    HICON hicon;

    if (!hwnd) {
        ErrorF("ReloadEnumWindowsProc: hwnd==NULL!\n");
        return FALSE;
    }

    /* It's our baby, either clean or dirty it */
    if (lParam == FALSE) {
        /* Reset the window's icon to undefined. */
        hicon = (HICON) SendMessage(hwnd, WM_SETICON, ICON_BIG, 0);

        /* If the old icon is generated on-the-fly, get rid of it, will regen */
        winDestroyIcon(hicon);

        /* Same for the small icon */
        hicon = (HICON) SendMessage(hwnd, WM_SETICON, ICON_SMALL, 0);
        winDestroyIcon(hicon);

        /* Remove any menu additions; bRevert=TRUE destroys any modified menus */
        GetSystemMenu(hwnd, TRUE);

        /* This window is now clean of our taint (but with undefined icons) */
    }
    else {
        /* Send a message to WM thread telling it re-evaluate the icon for this window */
        {
            winWMMessageRec wmMsg;

            WindowPtr pWin = GetProp(hwnd, WIN_WINDOW_PROP);

            if (pWin) {
                winPrivWinPtr pWinPriv = winGetWindowPriv(pWin);
                winPrivScreenPtr s_pScreenPriv = pWinPriv->pScreenPriv;

                wmMsg.msg = WM_WM_ICON_EVENT;
                wmMsg.hwndWindow = hwnd;
                wmMsg.iWindow = (Window) (INT_PTR) GetProp(hwnd, WIN_WID_PROP);

                winSendMessageToWM(s_pScreenPriv->pWMInfo, &wmMsg);
            }
        }

        /* Update the system menu for this window */
        SetupSysMenu(hwnd);

        /* That was easy... */
    }

    return TRUE;
}

/*
 * Removes any custom icons in classes, custom menus, etc.
 * Frees all members in pref structure.
 * Reloads the preferences file.
 * Set custom icons and menus again.
 */
static void
ReloadPrefs(winPrivScreenPtr pScreenPriv)
{
    int i;

    winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;

    /* First, iterate over all windows, deleting their icons and custom menus.
     * This is really only needed because winDestroyIcon() will try to
     * destroy the old global icons, which will have changed.
     * It is probably better to set a windows USER_DATA to flag locally defined
     * icons, and use that to accurately know when to destroy old icons.
     */
    if (pScreenInfo->fMultiWindow)
        EnumThreadWindows(g_dwCurrentThreadID, ReloadEnumWindowsProc, FALSE);

    /* Now, free/clear all info from our prefs structure */
    for (i = 0; i < pref.menuItems; i++)
        free(pref.menu[i].menuItem);
    free(pref.menu);
    pref.menu = NULL;
    pref.menuItems = 0;

    pref.rootMenuName[0] = 0;

    free(pref.sysMenu);
    pref.sysMenuItems = 0;

    pref.defaultSysMenuName[0] = 0;
    pref.defaultSysMenuPos = 0;

    pref.iconDirectory[0] = 0;
    pref.defaultIconName[0] = 0;
    pref.trayIconName[0] = 0;

    for (i = 0; i < pref.iconItems; i++)
        if (pref.icon[i].hicon)
            DestroyIcon((HICON) pref.icon[i].hicon);
    free(pref.icon);
    pref.icon = NULL;
    pref.iconItems = 0;

    /* Free global default X icon */
    if (g_hIconX)
        DestroyIcon(g_hIconX);
    if (g_hSmallIconX)
        DestroyIcon(g_hSmallIconX);

    /* Reset the custom command IDs */
    g_cmdid = STARTMENUID;

    /* Load the updated resource file */
    LoadPreferences();

    g_hIconX = NULL;
    g_hSmallIconX = NULL;

    if (pScreenInfo->fMultiWindow) {
        winInitGlobalIcons();

        /* Rebuild the icons and menus */
        EnumThreadWindows(g_dwCurrentThreadID, ReloadEnumWindowsProc, TRUE);
    }

    /* Whew, done */
}

/*
 * Check/uncheck the ALWAYSONTOP items in this menu
 */
void
HandleCustomWM_INITMENU(HWND hwnd, HMENU hmenu)
{
    DWORD dwExStyle;
    int i, j;

    if (!hwnd || !hmenu)
        return;

    if (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
        dwExStyle = MF_BYCOMMAND | MF_CHECKED;
    else
        dwExStyle = MF_BYCOMMAND | MF_UNCHECKED;

    for (i = 0; i < pref.menuItems; i++)
        for (j = 0; j < pref.menu[i].menuItems; j++)
            if (pref.menu[i].menuItem[j].cmd == CMD_ALWAYSONTOP)
                CheckMenuItem(hmenu, pref.menu[i].menuItem[j].commandID,
                              dwExStyle);

}

/*
 * Searches for the custom WM_COMMAND command ID and performs action.
 * Return TRUE if command is processed, FALSE otherwise.
 */
Bool
HandleCustomWM_COMMAND(HWND hwnd, WORD command, winPrivScreenPtr pScreenPriv)
{
    int i, j;
    MENUPARSED *m;
    DWORD dwExStyle;

    if (!command)
        return FALSE;

    for (i = 0; i < pref.menuItems; i++) {
        m = &(pref.menu[i]);
        for (j = 0; j < m->menuItems; j++) {
            if (command == m->menuItem[j].commandID) {
                /* Match! */
                switch (m->menuItem[j].cmd) {
#ifdef __CYGWIN__
                case CMD_EXEC:
                    if (fork() == 0) {
                        struct rlimit rl;
                        int fd;

                        /* Close any open descriptors except for STD* */
                        getrlimit(RLIMIT_NOFILE, &rl);
                        for (fd = STDERR_FILENO + 1; fd < rl.rlim_cur; fd++)
                            close(fd);

                        /* Disassociate any TTYs */
                        setsid();

                        execl("/bin/sh",
                              "/bin/sh", "-c", m->menuItem[j].param, NULL);
                        exit(0);
                    }
                    else
                        return TRUE;
                    break;
#else
                case CMD_EXEC:
                {
                    /* Start process without console window */
                    STARTUPINFO start;
                    PROCESS_INFORMATION child;

                    memset(&start, 0, sizeof(start));
                    start.cb = sizeof(start);
                    start.dwFlags = STARTF_USESHOWWINDOW;
                    start.wShowWindow = SW_HIDE;

                    memset(&child, 0, sizeof(child));

                    if (CreateProcess
                        (NULL, m->menuItem[j].param, NULL, NULL, FALSE, 0, NULL,
                         NULL, &start, &child)) {
                        CloseHandle(child.hThread);
                        CloseHandle(child.hProcess);
                    }
                    else
                        MessageBox(NULL, m->menuItem[j].param,
                                   "Mingrc Exec Command Error!",
                                   MB_OK | MB_ICONEXCLAMATION);
                }
                    return TRUE;
#endif
                case CMD_ALWAYSONTOP:
                    if (!hwnd)
                        return FALSE;

                    /* Get extended window style */
                    dwExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

                    /* Handle topmost windows */
                    if (dwExStyle & WS_EX_TOPMOST)
                        SetWindowPos(hwnd,
                                     HWND_NOTOPMOST,
                                     0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
                    else
                        SetWindowPos(hwnd,
                                     HWND_TOPMOST,
                                     0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
                    {
                        winScreenInfo *pScreenInfo = pScreenPriv->pScreenInfo;
                        if (pScreenInfo->fMultiWindow)
                            /* Reflect the changed Z order */
                            winReorderWindowsMultiWindow();
                    }
                    return TRUE;

                case CMD_RELOAD:
                    ReloadPrefs(pScreenPriv);
                    return TRUE;

                default:
                    return FALSE;
                }
            }                   /* match */
        }                       /* for j */
    }                           /* for i */

    return FALSE;
}

/*
 * Add the default or a custom menu depending on the class match
 */
void
SetupSysMenu(HWND hwnd)
{
    HMENU sys;
    int i;
    WindowPtr pWin;
    char *res_name, *res_class;

    if (!hwnd)
        return;

    pWin = GetProp(hwnd, WIN_WINDOW_PROP);

    sys = GetSystemMenu(hwnd, FALSE);
    if (!sys)
        return;

    if (pWin) {
        /* First see if there's a class match... */
        if (winMultiWindowGetClassHint(pWin, &res_name, &res_class)) {
            for (i = 0; i < pref.sysMenuItems; i++) {
                if (!strcmp(pref.sysMenu[i].match, res_name) ||
                    !strcmp(pref.sysMenu[i].match, res_class)) {
                    free(res_name);
                    free(res_class);

                    MakeMenu(pref.sysMenu[i].menuName, sys,
                             pref.sysMenu[i].menuPos == AT_START ? 0 : -1);
                    return;
                }
            }

            /* No match, just free alloc'd strings */
            free(res_name);
            free(res_class);
        }                       /* Found wm_class */
    }                           /* if pwin */

    /* Fallback to system default */
    if (pref.defaultSysMenuName[0]) {
        if (pref.defaultSysMenuPos == AT_START)
            MakeMenu(pref.defaultSysMenuName, sys, 0);
        else
            MakeMenu(pref.defaultSysMenuName, sys, -1);
    }
}

/*
 * Possibly add a menu to the toolbar icon
 */
void
SetupRootMenu(HMENU root)
{
    if (!root)
        return;

    if (pref.rootMenuName[0]) {
        MakeMenu(pref.rootMenuName, root, 0);
    }
}

/*
 * Check for and return an overridden default ICON specified in the prefs
 */
HICON
winOverrideDefaultIcon(int size)
{
    HICON hicon;

    if (pref.defaultIconName[0]) {
        hicon = LoadImageComma(pref.defaultIconName, pref.iconDirectory, size, size, 0);
        if (hicon == NULL)
            ErrorF("winOverrideDefaultIcon: LoadImageComma(%s) failed\n",
                   pref.defaultIconName);

        return hicon;
    }

    return 0;
}

/*
 * Return the HICON to use in the taskbar notification area
 */
HICON
winTaskbarIcon(void)
{
    HICON hicon;

    hicon = 0;
    /* First try and load an overridden, if success then return it */
    if (pref.trayIconName[0]) {
        hicon = LoadImageComma(pref.trayIconName, pref.iconDirectory,
                               GetSystemMetrics(SM_CXSMICON),
                               GetSystemMetrics(SM_CYSMICON), 0);
        if (hicon == NULL)
            ErrorF("winTaskbarIcon: LoadImageComma(%s) failed\n",
                   pref.trayIconName);
    }

    /* Otherwise return the default */
    if (!hicon)
        hicon = (HICON) LoadImage(g_hInstance,
                                  MAKEINTRESOURCE(IDI_XWIN),
                                  IMAGE_ICON,
                                  GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON), 0);

    return hicon;
}

/*
 * Handle comma-ified icon names
 *
 * Parse a filename to extract an icon:
 *  If fname is exactly ",nnn" then extract icon from our resource
 *  else if it is "file,nnn" then extract icon nnn from that file
 *  else try to load it as an .ico file and if that fails return NULL
 */
HICON
LoadImageComma(char *fname, char *iconDirectory, int sx, int sy, int flags)
{
    HICON hicon;
    int i;

    /* Some input error checking */
    if (!fname || !fname[0])
        return NULL;

    i = 0;
    hicon = NULL;

    if (fname[0] == ',') {
        /* It's the XWIN.EXE resource they want */
        i = atoi(fname + 1);
        hicon = LoadImage(g_hInstance,
                          MAKEINTRESOURCE(i), IMAGE_ICON, sx, sy, flags);
    }
    else {
        char *file = malloc(PATH_MAX + NAME_MAX + 2);
        Bool convert = FALSE;

        if (!file)
            return NULL;

        file[0] = 0;

        /* If fname starts 'X:\', it's an absolute Windows path, do nothing */
        if (!(fname[0] && fname[1] == ':' && fname[2] == '\\')) {
#ifdef  __CYGWIN__
            /* If fname starts with '/', it's an absolute cygwin path, we'll
               need to convert it */
            if (fname[0] == '/') {
                convert = TRUE;
            }
            else
#endif
            if (iconDirectory) {
                /* Otherwise, prepend the default icon directory, which
                   currently must be in absolute Windows path form */
                strcpy(file, iconDirectory);
                if (iconDirectory[0])
                    if (iconDirectory[strlen(iconDirectory) - 1] != '\\')
                        strcat(file, "\\");
            }
        }
        strcat(file, fname);

        /* Trim off any ',index' */
        if (strrchr(file, ',')) {
            *(strrchr(file, ',')) = 0;  /* End string at comma */
            i = atoi(strrchr(fname, ',') + 1);
        }
        else {
            i = -1;
        }

#ifdef  __CYGWIN__
        /* Convert from Cygwin path to Windows path */
        if (convert) {
            char *converted_file = cygwin_create_path(CCP_POSIX_TO_WIN_A | CCP_ABSOLUTE, file);
            if (converted_file) {
                free(file);
                file = converted_file;
            }
        }
#endif

        if (i >= 0) {
            /* Specified as <fname>,<index> */
            hicon = ExtractIcon(g_hInstance, file, i);
        }
        else {
            /* Specified as just an .ico file */
            hicon = (HICON) LoadImage(NULL,
                                      file,
                                      IMAGE_ICON,
                                      sx, sy, LR_LOADFROMFILE | flags);
        }
        free(file);
    }
    return hicon;
}

/*
 * Check for a match of the window class to one specified in the
 * ICONS{} section in the prefs file, and load the icon from a file
 */
HICON
winOverrideIcon(char *res_name, char *res_class, char *wmName)
{
    int i;
    HICON hicon;

    for (i = 0; i < pref.iconItems; i++) {
        if ((res_name && !strcmp(pref.icon[i].match, res_name)) ||
            (res_class && !strcmp(pref.icon[i].match, res_class)) ||
            (wmName && strstr(wmName, pref.icon[i].match))) {
            if (pref.icon[i].hicon)
                return pref.icon[i].hicon;

            hicon = LoadImageComma(pref.icon[i].iconFile, pref.iconDirectory, 0, 0, LR_DEFAULTSIZE);
            if (hicon == NULL)
                ErrorF("winOverrideIcon: LoadImageComma(%s) failed\n",
                       pref.icon[i].iconFile);

            pref.icon[i].hicon = hicon;
            return hicon;
        }
    }

    /* Didn't find the icon, fail gracefully */
    return 0;
}

/*
 * Should we free this icon or leave it in memory (is it part of our
 * ICONS{} overrides)?
 */
int
winIconIsOverride(HICON hicon)
{
    int i;

    if (!hicon)
        return 0;

    for (i = 0; i < pref.iconItems; i++)
        if ((HICON) pref.icon[i].hicon == hicon)
            return 1;

    return 0;
}

/*
 * Open and parse the XWinrc config file @path.
 * If @path is NULL, use the built-in default.
 */
static int
winPrefsLoadPreferences(const char *path)
{
    FILE *prefFile = NULL;

    if (path)
        prefFile = fopen(path, "r");
#ifdef __CYGWIN__
    else {
        char defaultPrefs[] =
            "MENU rmenu {\n"
            "  \"How to customize this menu\" EXEC \"xterm +tb -e man XWinrc\"\n"
            "  \"Launch xterm\" EXEC xterm\n"
            "  \"Load .XWinrc\" RELOAD\n"
            "  SEPARATOR\n" "}\n" "\n" "ROOTMENU rmenu\n";

        path = "built-in default";
        prefFile = fmemopen(defaultPrefs, strlen(defaultPrefs), "r");
    }
#endif

    if (!prefFile) {
        ErrorF("LoadPreferences: %s not found\n", path);
        return FALSE;
    }

    ErrorF("LoadPreferences: Loading %s\n", path);

    if ((parse_file(prefFile)) != 0) {
        ErrorF("LoadPreferences: %s is badly formed!\n", path);
        fclose(prefFile);
        return FALSE;
    }

    fclose(prefFile);
    return TRUE;
}

/*
 * Try and open ~/.XWinrc and system.XWinrc
 * Load it into prefs structure for use by other functions
 */
void
LoadPreferences(void)
{
    char *home;
    char fname[PATH_MAX + NAME_MAX + 2];
    char szDisplay[512];
    char *szEnvDisplay;
    int i, j;
    char param[PARAM_MAX + 1];
    char *srcParam, *dstParam;
    int parsed = FALSE;

    /* First, clear all preference settings */
    memset(&pref, 0, sizeof(pref));

    /* Now try and find a ~/.xwinrc file */
    home = getenv("HOME");
    if (home) {
        strcpy(fname, home);
        if (fname[strlen(fname) - 1] != '/')
            strcat(fname, "/");
        strcat(fname, ".XWinrc");
        parsed = winPrefsLoadPreferences(fname);
    }

    /* No home file found, check system default */
    if (!parsed) {
        char buffer[MAX_PATH];

#ifdef RELOCATE_PROJECTROOT
        snprintf(buffer, sizeof(buffer), "%s\\system.XWinrc", winGetBaseDir());
#else
        strncpy(buffer, SYSCONFDIR "/X11/system.XWinrc", sizeof(buffer));
#endif
        buffer[sizeof(buffer) - 1] = 0;
        parsed = winPrefsLoadPreferences(buffer);
    }

    /* Neither user nor system configuration found, or were badly formed */
    if (!parsed) {
        ErrorF
            ("LoadPreferences: See \"man XWinrc\" to customize the XWin menu.\n");
        parsed = winPrefsLoadPreferences(NULL);
    }

    /* Setup a DISPLAY environment variable, need to allocate on heap */
    /* because putenv doesn't copy the argument... */
    winGetDisplayName(szDisplay, 0);
    szEnvDisplay = (char *) (malloc(strlen(szDisplay) + strlen("DISPLAY=") + 1));
    if (szEnvDisplay) {
        snprintf(szEnvDisplay, 512, "DISPLAY=%s", szDisplay);
        putenv(szEnvDisplay);
    }

    /* Replace any "%display%" in menu commands with display string */
    for (i = 0; i < pref.menuItems; i++) {
        for (j = 0; j < pref.menu[i].menuItems; j++) {
            if (pref.menu[i].menuItem[j].cmd == CMD_EXEC) {
                srcParam = pref.menu[i].menuItem[j].param;
                dstParam = param;
                while (*srcParam) {
                    if (!strncmp(srcParam, "%display%", 9)) {
                        memcpy(dstParam, szDisplay, strlen(szDisplay));
                        dstParam += strlen(szDisplay);
                        srcParam += 9;
                    }
                    else {
                        *dstParam = *srcParam;
                        dstParam++;
                        srcParam++;
                    }
                }
                *dstParam = 0;
                strcpy(pref.menu[i].menuItem[j].param, param);
            }                   /* cmd==cmd_exec */
        }                       /* for all menuitems */
    }                           /* for all menus */

}

/*
 * Check for a match of the window class to one specified in the
 * STYLES{} section in the prefs file, and return the style type
 */
unsigned long
winOverrideStyle(char *res_name, char *res_class, char *wmName)
{
    int i;

    for (i = 0; i < pref.styleItems; i++) {
        if ((res_name && !strcmp(pref.style[i].match, res_name)) ||
            (res_class && !strcmp(pref.style[i].match, res_class)) ||
            (wmName && strstr(wmName, pref.style[i].match))) {
            if (pref.style[i].type)
                return pref.style[i].type;
        }
    }

    /* Didn't find the style, fail gracefully */
    return STYLE_NONE;
}
