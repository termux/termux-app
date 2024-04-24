/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors: Alexander Gottwald	
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"
#include "winconfig.h"
#include "winmsg.h"
#include "globals.h"

#include "xkbsrv.h"

#ifdef XWIN_XF86CONFIG
#ifndef CONFIGPATH
#define CONFIGPATH  "%A," "%R," \
                    "/etc/X11/%R," "%P/etc/X11/%R," \
                    "%E," "%F," \
                    "/etc/X11/%F," "%P/etc/X11/%F," \
                    "/etc/X11/%X-%M," "/etc/X11/%X," "/etc/%X," \
                    "%P/etc/X11/%X.%H," "%P/etc/X11/%X-%M," \
                    "%P/etc/X11/%X," \
                    "%P/lib/X11/%X.%H," "%P/lib/X11/%X-%M," \
                    "%P/lib/X11/%X"
#endif
#ifndef CONFIGDIRPATH
#define CONFIGDIRPATH  "/etc/X11/%X-%M," "/etc/X11/%X," "/etc/%X," \
                       "%P/etc/X11/%X.%H," "%P/etc/X11/%X-%M," \
                       "%P/etc/X11/%X," \
                       "%P/lib/X11/%X.%H," "%P/lib/X11/%X-%M," \
                       "%P/lib/X11/%X"
#endif

XF86ConfigPtr g_xf86configptr = NULL;
#endif

WinCmdlineRec g_cmdline = {
#ifdef XWIN_XF86CONFIG
    NULL,                       /* configFile */
    NULL,                       /* configDir */
#endif
    NULL,                       /* fontPath */
#ifdef XWIN_XF86CONFIG
    NULL,                       /* keyboard */
#endif
    NULL,                       /* xkbRules */
    NULL,                       /* xkbModel */
    NULL,                       /* xkbLayout */
    NULL,                       /* xkbVariant */
    NULL,                       /* xkbOptions */
    NULL,                       /* screenname */
    NULL,                       /* mousename */
    FALSE,                      /* emulate3Buttons */
    0                           /* emulate3Timeout */
};

winInfoRec g_winInfo = {
    {                           /* keyboard */
     0,                         /* leds */
     500,                       /* delay */
     30                         /* rate */
     }
    ,
    {                           /* xkb */
     NULL,                      /* rules */
     NULL,                      /* model */
     NULL,                      /* layout */
     NULL,                      /* variant */
     NULL,                      /* options */
     }
    ,
    {
     FALSE,
     50}
};

#define NULL_IF_EMPTY(x) (winNameCompare(x,"")?x:NULL)

#ifdef XWIN_XF86CONFIG
serverLayoutRec g_winConfigLayout;

static Bool ParseOptionValue(int scrnIndex, void *options, OptionInfoPtr p);
static Bool configLayout(serverLayoutPtr, XF86ConfLayoutPtr, char *);
static Bool configImpliedLayout(serverLayoutPtr, XF86ConfScreenPtr);
static Bool GetBoolValue(OptionInfoPtr p, const char *s);

Bool
winReadConfigfile()
{
    Bool retval = TRUE;
    char *filename, *dirname;
    MessageType filefrom = X_DEFAULT;
    MessageType dirfrom = X_DEFAULT;
    char *xf86ConfigFile = NULL;
    char *xf86ConfigDir = NULL;

    if (g_cmdline.configFile) {
        filefrom = X_CMDLINE;
        xf86ConfigFile = g_cmdline.configFile;
    }
    if (g_cmdline.configDir) {
        dirfrom = X_CMDLINE;
        xf86ConfigDir = g_cmdline.configDir;
    }

    /* Parse config file into data structure */
    xf86initConfigFiles();
    dirname = xf86openConfigDirFiles(CONFIGDIRPATH, xf86ConfigDir, PROJECTROOT);
    filename = xf86openConfigFile(CONFIGPATH, xf86ConfigFile, PROJECTROOT);

    /* Hack for backward compatibility */
    if (!filename && from == X_DEFAULT)
        filename = xf86openConfigFile(CONFIGPATH, "XF86Config", PROJECTROOT);

    if (filename) {
        winMsg(from, "Using config file: \"%s\"\n", filename);
    }
    else {
        winMsg(X_ERROR, "Unable to locate/open config file");
        if (xf86ConfigFile)
            ErrorF(": \"%s\"", xf86ConfigFile);
        ErrorF("\n");
    }
    if (dirname) {
        winMsg(from, "Using config directory: \"%s\"\n", dirname);
    }
    else {
        winMsg(X_ERROR, "Unable to locate/open config directory");
        if (xf86ConfigDir)
            ErrorF(": \"%s\"", xf86ConfigDir);
        ErrorF("\n");
    }
    if (!filename && !dirname) {
        return FALSE;
    }
    free(filename);
    free(dirname);
    if ((g_xf86configptr = xf86readConfigFile()) == NULL) {
        winMsg(X_ERROR, "Problem parsing the config file\n");
        return FALSE;
    }
    xf86closeConfigFile();

    LogPrintMarkers();

    /* set options from data structure */

    if (g_xf86configptr->conf_layout_lst == NULL ||
        g_cmdline.screenname != NULL) {
        if (g_cmdline.screenname == NULL) {
            winMsg(X_WARNING,
                   "No Layout section. Using the first Screen section.\n");
        }
        if (!configImpliedLayout(&g_winConfigLayout,
                                 g_xf86configptr->conf_screen_lst)) {
            winMsg(X_ERROR, "Unable to determine the screen layout\n");
            return FALSE;
        }
    }
    else {
        /* Check if layout is given in the config file */
        if (g_xf86configptr->conf_flags != NULL) {
            char *dfltlayout = NULL;
            void *optlist = g_xf86configptr->conf_flags->flg_option_lst;

            if (optlist && winFindOption(optlist, "defaultserverlayout"))
                dfltlayout =
                    winSetStrOption(optlist, "defaultserverlayout", NULL);

            if (!configLayout(&g_winConfigLayout,
                              g_xf86configptr->conf_layout_lst, dfltlayout)) {
                winMsg(X_ERROR, "Unable to determine the screen layout\n");
                return FALSE;
            }
        }
        else {
            if (!configLayout(&g_winConfigLayout,
                              g_xf86configptr->conf_layout_lst, NULL)) {
                winMsg(X_ERROR, "Unable to determine the screen layout\n");
                return FALSE;
            }
        }
    }

    /* setup special config files */
    winConfigFiles();
    return retval;
}
#endif

/* load layout definitions */
#include "winlayouts.h"

/* Set the keyboard configuration */
Bool
winConfigKeyboard(DeviceIntPtr pDevice)
{
    char layoutName[KL_NAMELENGTH];
    unsigned char layoutFriendlyName[256];
    unsigned int layoutNum = 0;
    unsigned int deviceIdentifier = 0;
    int keyboardType;

#ifdef XWIN_XF86CONFIG
    XF86ConfInputPtr kbd = NULL;
    XF86ConfInputPtr input_list = NULL;
    MessageType kbdfrom = X_CONFIG;
#endif
    MessageType from = X_DEFAULT;
    char *s = NULL;

    /* Setup defaults */
    XkbGetRulesDflts(&g_winInfo.xkb);

    /*
     * Query the windows autorepeat settings and change the xserver defaults.
     */
    {
        int kbd_delay;
        DWORD kbd_speed;

        if (SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &kbd_delay, 0) &&
            SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &kbd_speed, 0)) {
            switch (kbd_delay) {
            case 0:
                g_winInfo.keyboard.delay = 250;
                break;
            case 1:
                g_winInfo.keyboard.delay = 500;
                break;
            case 2:
                g_winInfo.keyboard.delay = 750;
                break;
            default:
            case 3:
                g_winInfo.keyboard.delay = 1000;
                break;
            }
            g_winInfo.keyboard.rate = (kbd_speed > 0) ? kbd_speed : 1;
            winMsg(X_PROBED, "Setting autorepeat to delay=%ld, rate=%ld\n",
                   g_winInfo.keyboard.delay, g_winInfo.keyboard.rate);

        }
    }

    keyboardType = GetKeyboardType(0);
    if (keyboardType > 0 && GetKeyboardLayoutName(layoutName)) {
        WinKBLayoutPtr pLayout;
        Bool bfound = FALSE;
        int pass;

        layoutNum = strtoul(layoutName, (char **) NULL, 16);
        if ((layoutNum & 0xffff) == 0x411) {
            if (keyboardType == 7) {
                /* Japanese layouts have problems with key event messages
                   such as the lack of WM_KEYUP for Caps Lock key.
                   Loading US layout fixes this problem. */
                if (LoadKeyboardLayout("00000409", KLF_ACTIVATE) != NULL)
                    winMsg(X_INFO, "Loading US keyboard layout.\n");
                else
                    winMsg(X_ERROR, "LoadKeyboardLayout failed.\n");
            }
        }

        /* Discover the friendly name of the current layout */
        {
            HKEY regkey = NULL;
            const char regtempl[] =
                "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\";
            char *regpath;
            DWORD namesize = sizeof(layoutFriendlyName);

            regpath = malloc(sizeof(regtempl) + KL_NAMELENGTH + 1);
            strcpy(regpath, regtempl);
            strcat(regpath, layoutName);

            if (!RegOpenKey(HKEY_LOCAL_MACHINE, regpath, &regkey))
                RegQueryValueEx(regkey, "Layout Text", 0, NULL,
                                layoutFriendlyName, &namesize);

            /* Close registry key */
            if (regkey)
                RegCloseKey(regkey);
            free(regpath);
        }

        winMsg(X_PROBED,
               "Windows keyboard layout: \"%s\" (%08x) \"%s\", type %d\n",
               layoutName, layoutNum, layoutFriendlyName, keyboardType);

        deviceIdentifier = layoutNum >> 16;
        for (pass = 0; pass < 2; pass++) {
            /* If we didn't find an exact match for the input locale identifier,
               try to find an match on the language identifier part only  */
            if (pass == 1)
                layoutNum = (layoutNum & 0xffff);

            for (pLayout = winKBLayouts; pLayout->winlayout != -1; pLayout++) {
                if (pLayout->winlayout != layoutNum)
                    continue;
                if (pLayout->winkbtype > 0 && pLayout->winkbtype != keyboardType)
                    continue;

                bfound = TRUE;
                winMsg(X_PROBED,
                       "Found matching XKB configuration \"%s\"\n",
                       pLayout->layoutname);

                winMsg(X_PROBED,
                       "Model = \"%s\" Layout = \"%s\""
                       " Variant = \"%s\" Options = \"%s\"\n",
                       pLayout->xkbmodel ? pLayout->xkbmodel : "none",
                       pLayout->xkblayout ? pLayout->xkblayout : "none",
                       pLayout->xkbvariant ? pLayout->xkbvariant : "none",
                       pLayout->xkboptions ? pLayout->xkboptions : "none");

                g_winInfo.xkb.model = pLayout->xkbmodel;
                g_winInfo.xkb.layout = pLayout->xkblayout;
                g_winInfo.xkb.variant = pLayout->xkbvariant;
                g_winInfo.xkb.options = pLayout->xkboptions;

                if (deviceIdentifier == 0xa000) {
                    winMsg(X_PROBED, "Windows keyboard layout device identifier indicates Macintosh, setting Model = \"macintosh\"");
                    g_winInfo.xkb.model = "macintosh";
                }

                break;
            }

            if (bfound)
                break;
        }

        if (!bfound) {
            winMsg(X_ERROR,
                   "Keyboardlayout \"%s\" (%s) is unknown, using X server default layout\n",
                   layoutFriendlyName, layoutName);
        }
    }

    /* parse the configuration */
#ifdef XWIN_XF86CONFIG
    if (g_cmdline.keyboard)
        kbdfrom = X_CMDLINE;

    /*
     * Until the layout code is finished, I search for the keyboard
     * device and configure the server with it.
     */

    if (g_xf86configptr != NULL)
        input_list = g_xf86configptr->conf_input_lst;

    while (input_list != NULL) {
        if (winNameCompare(input_list->inp_driver, "keyboard") == 0) {
            /* Check if device name matches requested name */
            if (g_cmdline.keyboard && winNameCompare(input_list->inp_identifier,
                                                     g_cmdline.keyboard))
                continue;
            kbd = input_list;
        }
        input_list = input_list->list.next;
    }

    if (kbd != NULL) {

        if (kbd->inp_identifier)
            winMsg(kbdfrom, "Using keyboard \"%s\" as primary keyboard\n",
                   kbd->inp_identifier);

        if ((s = winSetStrOption(kbd->inp_option_lst, "AutoRepeat", NULL))) {
            if ((sscanf(s, "%ld %ld", &g_winInfo.keyboard.delay,
                        &g_winInfo.keyboard.rate) != 2) ||
                (g_winInfo.keyboard.delay < 1) ||
                (g_winInfo.keyboard.rate == 0) ||
                (1000 / g_winInfo.keyboard.rate) < 1) {
                winErrorFVerb(2, "\"%s\" is not a valid AutoRepeat value", s);
                free(s);
                return FALSE;
            }
            free(s);
            winMsg(X_CONFIG, "AutoRepeat: %ld %ld\n",
                   g_winInfo.keyboard.delay, g_winInfo.keyboard.rate);
        }
#endif

        s = NULL;
        if (g_cmdline.xkbRules) {
            s = g_cmdline.xkbRules;
            from = X_CMDLINE;
        }
#ifdef XWIN_XF86CONFIG
        else {
            s = winSetStrOption(kbd->inp_option_lst, "XkbRules", NULL);
            from = X_CONFIG;
        }
#endif
        if (s) {
            g_winInfo.xkb.rules = NULL_IF_EMPTY(s);
            winMsg(from, "XKB: rules: \"%s\"\n", s);
        }

        s = NULL;
        if (g_cmdline.xkbModel) {
            s = g_cmdline.xkbModel;
            from = X_CMDLINE;
        }
#ifdef XWIN_XF86CONFIG
        else {
            s = winSetStrOption(kbd->inp_option_lst, "XkbModel", NULL);
            from = X_CONFIG;
        }
#endif
        if (s) {
            g_winInfo.xkb.model = NULL_IF_EMPTY(s);
            winMsg(from, "XKB: model: \"%s\"\n", s);
        }

        s = NULL;
        if (g_cmdline.xkbLayout) {
            s = g_cmdline.xkbLayout;
            from = X_CMDLINE;
        }
#ifdef XWIN_XF86CONFIG
        else {
            s = winSetStrOption(kbd->inp_option_lst, "XkbLayout", NULL);
            from = X_CONFIG;
        }
#endif
        if (s) {
            g_winInfo.xkb.layout = NULL_IF_EMPTY(s);
            winMsg(from, "XKB: layout: \"%s\"\n", s);
        }

        s = NULL;
        if (g_cmdline.xkbVariant) {
            s = g_cmdline.xkbVariant;
            from = X_CMDLINE;
        }
#ifdef XWIN_XF86CONFIG
        else {
            s = winSetStrOption(kbd->inp_option_lst, "XkbVariant", NULL);
            from = X_CONFIG;
        }
#endif
        if (s) {
            g_winInfo.xkb.variant = NULL_IF_EMPTY(s);
            winMsg(from, "XKB: variant: \"%s\"\n", s);
        }

        s = NULL;
        if (g_cmdline.xkbOptions) {
            s = g_cmdline.xkbOptions;
            from = X_CMDLINE;
        }
#ifdef XWIN_XF86CONFIG
        else {
            s = winSetStrOption(kbd->inp_option_lst, "XkbOptions", NULL);
            from = X_CONFIG;
        }
#endif
        if (s) {
            g_winInfo.xkb.options = NULL_IF_EMPTY(s);
            winMsg(from, "XKB: options: \"%s\"\n", s);
        }

#ifdef XWIN_XF86CONFIG
    }
#endif

    return TRUE;
}

#ifdef XWIN_XF86CONFIG
Bool
winConfigMouse(DeviceIntPtr pDevice)
{
    MessageType mousefrom = X_CONFIG;

    XF86ConfInputPtr mouse = NULL;
    XF86ConfInputPtr input_list = NULL;

    if (g_cmdline.mouse)
        mousefrom = X_CMDLINE;

    if (g_xf86configptr != NULL)
        input_list = g_xf86configptr->conf_input_lst;

    while (input_list != NULL) {
        if (winNameCompare(input_list->inp_driver, "mouse") == 0) {
            /* Check if device name matches requested name */
            if (g_cmdline.mouse && winNameCompare(input_list->inp_identifier,
                                                  g_cmdline.mouse))
                continue;
            mouse = input_list;
        }
        input_list = input_list->list.next;
    }

    if (mouse != NULL) {
        if (mouse->inp_identifier)
            winMsg(mousefrom, "Using pointer \"%s\" as primary pointer\n",
                   mouse->inp_identifier);

        g_winInfo.pointer.emulate3Buttons =
            winSetBoolOption(mouse->inp_option_lst, "Emulate3Buttons", FALSE);
        if (g_cmdline.emulate3buttons)
            g_winInfo.pointer.emulate3Buttons = g_cmdline.emulate3buttons;

        g_winInfo.pointer.emulate3Timeout =
            winSetIntOption(mouse->inp_option_lst, "Emulate3Timeout", 50);
        if (g_cmdline.emulate3timeout)
            g_winInfo.pointer.emulate3Timeout = g_cmdline.emulate3timeout;
    }
    else {
        winMsg(X_ERROR, "No primary pointer configured\n");
        winMsg(X_DEFAULT, "Using compiletime defaults for pointer\n");
    }

    return TRUE;
}

Bool
winConfigFiles()
{
    MessageType from;
    XF86ConfFilesPtr filesptr = NULL;

    /* set some shortcuts */
    if (g_xf86configptr != NULL) {
        filesptr = g_xf86configptr->conf_files;
    }

    /* Fontpath */
    from = X_DEFAULT;

    if (g_cmdline.fontPath) {
        from = X_CMDLINE;
        defaultFontPath = g_cmdline.fontPath;
    }
    else if (filesptr != NULL && filesptr->file_fontpath) {
        from = X_CONFIG;
        defaultFontPath = strdup(filesptr->file_fontpath);
    }
    winMsg(from, "FontPath set to \"%s\"\n", defaultFontPath);

    return TRUE;
}
#else
Bool
winConfigFiles(void)
{
    /* Fontpath */
    if (g_cmdline.fontPath) {
        defaultFontPath = g_cmdline.fontPath;
        winMsg(X_CMDLINE, "FontPath set to \"%s\"\n", defaultFontPath);
    }

    return TRUE;
}
#endif

Bool
winConfigOptions(void)
{
    return TRUE;
}

Bool
winConfigScreens(void)
{
    return TRUE;
}

#ifdef XWIN_XF86CONFIG
char *
winSetStrOption(void *optlist, const char *name, char *deflt)
{
    OptionInfoRec o;

    o.name = name;
    o.type = OPTV_STRING;
    if (ParseOptionValue(-1, optlist, &o))
        deflt = o.value.str;
    if (deflt)
        return strdup(deflt);
    else
        return NULL;
}

int
winSetBoolOption(void *optlist, const char *name, int deflt)
{
    OptionInfoRec o;

    o.name = name;
    o.type = OPTV_BOOLEAN;
    if (ParseOptionValue(-1, optlist, &o))
        deflt = o.value.boolean;
    return deflt;
}

int
winSetIntOption(void *optlist, const char *name, int deflt)
{
    OptionInfoRec o;

    o.name = name;
    o.type = OPTV_INTEGER;
    if (ParseOptionValue(-1, optlist, &o))
        deflt = o.value.num;
    return deflt;
}

double
winSetRealOption(void *optlist, const char *name, double deflt)
{
    OptionInfoRec o;

    o.name = name;
    o.type = OPTV_REAL;
    if (ParseOptionValue(-1, optlist, &o))
        deflt = o.value.realnum;
    return deflt;
}

double
winSetPercentOption(void *optlist, const char *name, double deflt)
{
    OptionInfoRec o;

    o.name = name;
    o.type = OPTV_PERCENT;
    if (ParseOptionValue(-1, optlist, &o))
        deflt = o.value.realnum;
    return deflt;
}
#endif

/*
 * Compare two strings for equality. This is caseinsensitive  and
 * The characters '_', ' ' (space) and '\t' (tab) are treated as
 * not existing.
 */

int
winNameCompare(const char *s1, const char *s2)
{
    char c1, c2;

    if (!s1 || *s1 == 0) {
        if (!s2 || *s2 == 0)
            return 0;
        else
            return 1;
    }

    while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
        s1++;
    while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
        s2++;

    c1 = (isupper((int) *s1) ? tolower((int) *s1) : *s1);
    c2 = (isupper((int) *s2) ? tolower((int) *s2) : *s2);

    while (c1 == c2) {
        if (c1 == 0)
            return 0;
        s1++;
        s2++;

        while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
            s1++;
        while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
            s2++;

        c1 = (isupper((int) *s1) ? tolower((int) *s1) : *s1);
        c2 = (isupper((int) *s2) ? tolower((int) *s2) : *s2);
    }
    return c1 - c2;
}

#ifdef XWIN_XF86CONFIG
/*
 * Find the named option in the list.
 * @return the pointer to the option record, or NULL if not found.
 */

XF86OptionPtr
winFindOption(XF86OptionPtr list, const char *name)
{
    while (list) {
        if (winNameCompare(list->opt_name, name) == 0)
            return list;
        list = list->list.next;
    }
    return NULL;
}

/*
 * Find the Value of an named option.
 * @return The option value or NULL if not found.
 */

char *
winFindOptionValue(XF86OptionPtr list, const char *name)
{
    list = winFindOption(list, name);
    if (list) {
        if (list->opt_val)
            return list->opt_val;
        else
            return "";
    }
    return NULL;
}

/*
 * Parse the option.
 */

static Bool
ParseOptionValue(int scrnIndex, void *options, OptionInfoPtr p)
{
    char *s, *end;

    if ((s = winFindOptionValue(options, p->name)) != NULL) {
        switch (p->type) {
        case OPTV_INTEGER:
            if (*s == '\0') {
                winDrvMsg(scrnIndex, X_WARNING,
                          "Option \"%s\" requires an integer value\n", p->name);
                p->found = FALSE;
            }
            else {
                p->value.num = strtoul(s, &end, 0);
                if (*end == '\0') {
                    p->found = TRUE;
                }
                else {
                    winDrvMsg(scrnIndex, X_WARNING,
                              "Option \"%s\" requires an integer value\n",
                              p->name);
                    p->found = FALSE;
                }
            }
            break;
        case OPTV_STRING:
            if (*s == '\0') {
                winDrvMsg(scrnIndex, X_WARNING,
                          "Option \"%s\" requires a string value\n", p->name);
                p->found = FALSE;
            }
            else {
                p->value.str = s;
                p->found = TRUE;
            }
            break;
        case OPTV_ANYSTR:
            p->value.str = s;
            p->found = TRUE;
            break;
        case OPTV_REAL:
            if (*s == '\0') {
                winDrvMsg(scrnIndex, X_WARNING,
                          "Option \"%s\" requires a floating point value\n",
                          p->name);
                p->found = FALSE;
            }
            else {
                p->value.realnum = strtod(s, &end);
                if (*end == '\0') {
                    p->found = TRUE;
                }
                else {
                    winDrvMsg(scrnIndex, X_WARNING,
                              "Option \"%s\" requires a floating point value\n",
                              p->name);
                    p->found = FALSE;
                }
            }
            break;
        case OPTV_BOOLEAN:
            if (GetBoolValue(p, s)) {
                p->found = TRUE;
            }
            else {
                winDrvMsg(scrnIndex, X_WARNING,
                          "Option \"%s\" requires a boolean value\n", p->name);
                p->found = FALSE;
            }
            break;
        case OPTV_PERCENT:
            if (*s == '\0') {
                winDrvMsg(scrnIndex, X_WARNING,
                          "Option \"%s\" requires a percent value\n", p->name);
                p->found = FALSE;
            }
            else {
                double percent = strtod(s, &end);

                if (end != s && winNameCompare(end, "%")) {
                    p->found = TRUE;
                    p->value.realnum = percent;
                }
                else {
                    winDrvMsg(scrnIndex, X_WARNING,
                              "Option \"%s\" requires a frequency value\n",
                              p->name);
                    p->found = FALSE;
                }
            }
        case OPTV_FREQ:
            if (*s == '\0') {
                winDrvMsg(scrnIndex, X_WARNING,
                          "Option \"%s\" requires a frequency value\n",
                          p->name);
                p->found = FALSE;
            }
            else {
                double freq = strtod(s, &end);
                int units = 0;

                if (end != s) {
                    p->found = TRUE;
                    if (!winNameCompare(end, "Hz"))
                        units = 1;
                    else if (!winNameCompare(end, "kHz") ||
                             !winNameCompare(end, "k"))
                        units = 1000;
                    else if (!winNameCompare(end, "MHz") ||
                             !winNameCompare(end, "M"))
                        units = 1000000;
                    else {
                        winDrvMsg(scrnIndex, X_WARNING,
                                  "Option \"%s\" requires a frequency value\n",
                                  p->name);
                        p->found = FALSE;
                    }
                    if (p->found)
                        freq *= (double) units;
                }
                else {
                    winDrvMsg(scrnIndex, X_WARNING,
                              "Option \"%s\" requires a frequency value\n",
                              p->name);
                    p->found = FALSE;
                }
                if (p->found) {
                    p->value.freq.freq = freq;
                    p->value.freq.units = units;
                }
            }
            break;
        case OPTV_NONE:
            /* Should never get here */
            p->found = FALSE;
            break;
        }
        if (p->found) {
            winDrvMsgVerb(scrnIndex, X_CONFIG, 2, "Option \"%s\"", p->name);
            if (!(p->type == OPTV_BOOLEAN && *s == 0)) {
                winErrorFVerb(2, " \"%s\"", s);
            }
            winErrorFVerb(2, "\n");
        }
    }
    else if (p->type == OPTV_BOOLEAN) {
        /* Look for matches with options with or without a "No" prefix. */
        char *n, *newn;
        OptionInfoRec opt;

        n = winNormalizeName(p->name);
        if (!n) {
            p->found = FALSE;
            return FALSE;
        }
        if (strncmp(n, "no", 2) == 0) {
            newn = n + 2;
        }
        else {
            free(n);
            n = malloc(strlen(p->name) + 2 + 1);
            if (!n) {
                p->found = FALSE;
                return FALSE;
            }
            strcpy(n, "No");
            strcat(n, p->name);
            newn = n;
        }
        if ((s = winFindOptionValue(options, newn)) != NULL) {
            if (GetBoolValue(&opt, s)) {
                p->value.boolean = !opt.value.boolean;
                p->found = TRUE;
            }
            else {
                winDrvMsg(scrnIndex, X_WARNING,
                          "Option \"%s\" requires a boolean value\n", newn);
                p->found = FALSE;
            }
        }
        else {
            p->found = FALSE;
        }
        if (p->found) {
            winDrvMsgVerb(scrnIndex, X_CONFIG, 2, "Option \"%s\"", newn);
            if (*s != 0) {
                winErrorFVerb(2, " \"%s\"", s);
            }
            winErrorFVerb(2, "\n");
        }
        free(n);
    }
    else {
        p->found = FALSE;
    }
    return p->found;
}

static Bool
configLayout(serverLayoutPtr servlayoutp, XF86ConfLayoutPtr conf_layout,
             char *default_layout)
{
#if 0
#pragma warn UNIMPLEMENTED
#endif
    return TRUE;
}

static Bool
configImpliedLayout(serverLayoutPtr servlayoutp, XF86ConfScreenPtr conf_screen)
{
#if 0
#pragma warn UNIMPLEMENTED
#endif
    return TRUE;
}

static Bool
GetBoolValue(OptionInfoPtr p, const char *s)
{
    if (*s == 0) {
        p->value.boolean = TRUE;
    }
    else {
        if (winNameCompare(s, "1") == 0)
            p->value.boolean = TRUE;
        else if (winNameCompare(s, "on") == 0)
            p->value.boolean = TRUE;
        else if (winNameCompare(s, "true") == 0)
            p->value.boolean = TRUE;
        else if (winNameCompare(s, "yes") == 0)
            p->value.boolean = TRUE;
        else if (winNameCompare(s, "0") == 0)
            p->value.boolean = FALSE;
        else if (winNameCompare(s, "off") == 0)
            p->value.boolean = FALSE;
        else if (winNameCompare(s, "false") == 0)
            p->value.boolean = FALSE;
        else if (winNameCompare(s, "no") == 0)
            p->value.boolean = FALSE;
    }
    return TRUE;
}
#endif

char *
winNormalizeName(const char *s)
{
    char *ret, *q;
    const char *p;

    if (s == NULL)
        return NULL;

    ret = malloc(strlen(s) + 1);
    for (p = s, q = ret; *p != 0; p++) {
        switch (*p) {
        case '_':
        case ' ':
        case '\t':
            continue;
        default:
            if (isupper((int) *p))
                *q++ = tolower((int) *p);
            else
                *q++ = *p;
        }
    }
    *q = '\0';
    return ret;
}
