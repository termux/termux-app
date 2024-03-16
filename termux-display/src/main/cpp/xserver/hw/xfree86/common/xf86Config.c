/*
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 */

/*
 * Copyright 1992-2003 by The XFree86 Project, Inc.
 * Copyright 1997 by Metro Link, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 *
 * Authors:
 *	Dirk Hohndel <hohndel@XFree86.Org>
 *	David Dawes <dawes@XFree86.Org>
 *      Marc La France <tsi@XFree86.Org>
 *      Egbert Eich <eich@XFree86.Org>
 *      ... and others
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <sys/types.h>
#include <grp.h>

#include "xf86.h"
#include "xf86Modes.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "xf86Config.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "configProcs.h"
#include "globals.h"
#include "extension.h"
#include "xf86pciBus.h"
#include "xf86Xinput.h"
#include "loaderProcs.h"

#include "xkbsrv.h"
#include "picture.h"
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif

/*
 * These paths define the way the config file search is done.  The escape
 * sequences are documented in parser/scan.c.
 */
#ifndef ALL_CONFIGPATH
#define ALL_CONFIGPATH	"%A," "%R," \
			"/etc/X11/%R," "%P/etc/X11/%R," \
			"%E," "%F," \
			"/etc/X11/%F," "%P/etc/X11/%F," \
			"/etc/X11/%X," "/etc/%X," \
			"%P/etc/X11/%X.%H," \
			"%P/etc/X11/%X," \
			"%P/lib/X11/%X.%H," \
			"%P/lib/X11/%X"
#endif
#ifndef RESTRICTED_CONFIGPATH
#define RESTRICTED_CONFIGPATH	"/etc/X11/%S," "%P/etc/X11/%S," \
			"/etc/X11/%G," "%P/etc/X11/%G," \
			"/etc/X11/%X," "/etc/%X," \
			"%P/etc/X11/%X.%H," \
			"%P/etc/X11/%X," \
			"%P/lib/X11/%X.%H," \
			"%P/lib/X11/%X"
#endif
#ifndef ALL_CONFIGDIRPATH
#define ALL_CONFIGDIRPATH	"%A," "%R," \
				"/etc/X11/%R," "%C/X11/%R," \
				"/etc/X11/%X," "%C/X11/%X"
#endif
#ifndef RESTRICTED_CONFIGDIRPATH
#define RESTRICTED_CONFIGDIRPATH	"/etc/X11/%R," "%C/X11/%R," \
					"/etc/X11/%X," "%C/X11/%X"
#endif
#ifndef SYS_CONFIGDIRPATH
#define SYS_CONFIGDIRPATH	"%D/X11/%X"
#endif
#ifndef PROJECTROOT
#define PROJECTROOT	"/usr/X11R6"
#endif

static ModuleDefault ModuleDefaults[] = {
#ifdef GLXEXT
    {.name = "glx",.toLoad = TRUE,.load_opt = NULL},
#endif
#ifdef __CYGWIN__
    /* load DIX modules used by drivers first */
    {.name = "fb",.toLoad = TRUE,.load_opt = NULL},
    {.name = "shadow",.toLoad = TRUE,.load_opt = NULL},
#endif
    {.name = NULL,.toLoad = FALSE,.load_opt = NULL}
};

/* Forward declarations */
static Bool configScreen(confScreenPtr screenp, XF86ConfScreenPtr conf_screen,
                         int scrnum, MessageType from, Bool auto_gpu_device);
static Bool configMonitor(MonPtr monitorp, XF86ConfMonitorPtr conf_monitor);
static Bool configDevice(GDevPtr devicep, XF86ConfDevicePtr conf_device,
                         Bool active, Bool gpu);
static Bool configInput(InputInfoPtr pInfo, XF86ConfInputPtr conf_input,
                        MessageType from);
static Bool configDisplay(DispPtr displayp, XF86ConfDisplayPtr conf_display);
static Bool addDefaultModes(MonPtr monitorp);

static void configDRI(XF86ConfDRIPtr drip);
static void configExtensions(XF86ConfExtensionsPtr conf_ext);

/*
 * xf86GetPathElem --
 *	Extract a single element from the font path string starting at
 *	pnt.  The font path element will be returned, and pnt will be
 *	updated to point to the start of the next element, or set to
 *	NULL if there are no more.
 */
static char *
xf86GetPathElem(char **pnt)
{
    char *p1;

    p1 = *pnt;
    *pnt = index(*pnt, ',');
    if (*pnt != NULL) {
        **pnt = '\0';
        *pnt += 1;
    }
    return p1;
}

/*
 * xf86ValidateFontPath --
 *	Validates the user-specified font path.  Each element that
 *	begins with a '/' is checked to make sure the directory exists.
 *	If the directory exists, the existence of a file named 'fonts.dir'
 *	is checked.  If either check fails, an error is printed and the
 *	element is removed from the font path.
 */

#define DIR_FILE "/fonts.dir"
static char *
xf86ValidateFontPath(char *path)
{
    char *next, *tmp_path, *out_pnt, *path_elem, *p1, *dir_elem;
    struct stat stat_buf;
    int flag;
    int dirlen;

    tmp_path = calloc(1, strlen(path) + 1);
    out_pnt = tmp_path;
    path_elem = NULL;
    next = path;
    while (next != NULL) {
        path_elem = xf86GetPathElem(&next);
        if (*path_elem == '/') {
            dir_elem = xnfcalloc(1, strlen(path_elem) + 1);
            if ((p1 = strchr(path_elem, ':')) != 0)
                dirlen = p1 - path_elem;
            else
                dirlen = strlen(path_elem);
            strlcpy(dir_elem, path_elem, dirlen + 1);
            flag = stat(dir_elem, &stat_buf);
            if (flag == 0)
                if (!S_ISDIR(stat_buf.st_mode))
                    flag = -1;
            if (flag != 0) {
                xf86Msg(X_WARNING, "The directory \"%s\" does not exist.\n",
                        dir_elem);
                xf86ErrorF("\tEntry deleted from font path.\n");
                free(dir_elem);
                continue;
            }
            else {
                XNFasprintf(&p1, "%s%s", dir_elem, DIR_FILE);
                flag = stat(p1, &stat_buf);
                if (flag == 0)
                    if (!S_ISREG(stat_buf.st_mode))
                        flag = -1;
                free(p1);
                if (flag != 0) {
                    xf86Msg(X_WARNING,
                            "`fonts.dir' not found (or not valid) in \"%s\".\n",
                            dir_elem);
                    xf86ErrorF("\tEntry deleted from font path.\n");
                    xf86ErrorF("\t(Run 'mkfontdir' on \"%s\").\n", dir_elem);
                    free(dir_elem);
                    continue;
                }
            }
            free(dir_elem);
        }

        /*
         * Either an OK directory, or a font server name.  So add it to
         * the path.
         */
        if (out_pnt != tmp_path)
            *out_pnt++ = ',';
        strcat(out_pnt, path_elem);
        out_pnt += strlen(path_elem);
    }
    return tmp_path;
}

#define FIND_SUITABLE(pointertype, listhead, ptr)                                            \
    do {                                                                                     \
        pointertype _l, _p;                                                                  \
                                                                                             \
        for (_l = (listhead), _p = NULL; !_p && _l; _l = (pointertype)_l->list.next) {       \
            if (!_l->match_seat || (SeatId && xf86nameCompare(_l->match_seat, SeatId) == 0)) \
                _p = _l;                                                                     \
        }                                                                                    \
                                                                                             \
        (ptr) = _p;                                                                          \
    } while(0)

/*
 * use the datastructure that the parser provides and pick out the parts
 * that we need at this point
 */
const char **
xf86ModulelistFromConfig(void ***optlist)
{
    int count = 0, i = 0;
    const char **modulearray;

    const char *ignore[] = { "GLcore", "speedo", "bitmap", "drm",
        "freetype", "type1",
        NULL
    };
    void **optarray;
    XF86LoadPtr modp;
    Bool found;

    /*
     * make sure the config file has been parsed and that we have a
     * ModulePath set; if no ModulePath was given, use the default
     * ModulePath
     */
    if (xf86configptr == NULL) {
        xf86Msg(X_ERROR, "Cannot access global config data structure\n");
        return NULL;
    }

    if (xf86configptr->conf_modules) {
        /* Walk the disable list and let people know what we've parsed to
         * not be loaded
         */
        modp = xf86configptr->conf_modules->mod_disable_lst;
        while (modp) {
            xf86Msg(X_WARNING,
                    "\"%s\" will not be loaded unless you've specified it to be loaded elsewhere.\n",
                    modp->load_name);
            modp = (XF86LoadPtr) modp->list.next;
        }
        /*
         * Walk the default settings table. For each module listed to be
         * loaded, make sure it's in the mod_load_lst. If it's not, make
         * sure it's not in the mod_no_load_lst. If it's not disabled,
         * append it to mod_load_lst
         */
        for (i = 0; ModuleDefaults[i].name != NULL; i++) {
            if (ModuleDefaults[i].toLoad == FALSE) {
                xf86Msg(X_WARNING,
                        "\"%s\" is not to be loaded by default. Skipping.\n",
                        ModuleDefaults[i].name);
                continue;
            }
            found = FALSE;
            modp = xf86configptr->conf_modules->mod_load_lst;
            while (modp) {
                if (strcmp(modp->load_name, ModuleDefaults[i].name) == 0) {
                    xf86Msg(X_INFO,
                            "\"%s\" will be loaded. This was enabled by default and also specified in the config file.\n",
                            ModuleDefaults[i].name);
                    found = TRUE;
                    break;
                }
                modp = (XF86LoadPtr) modp->list.next;
            }
            if (found == FALSE) {
                modp = xf86configptr->conf_modules->mod_disable_lst;
                while (modp) {
                    if (strcmp(modp->load_name, ModuleDefaults[i].name) == 0) {
                        xf86Msg(X_INFO,
                                "\"%s\" will be loaded even though the default is to disable it.\n",
                                ModuleDefaults[i].name);
                        found = TRUE;
                        break;
                    }
                    modp = (XF86LoadPtr) modp->list.next;
                }
            }
            if (found == FALSE) {
                XF86LoadPtr ptr = (XF86LoadPtr) xf86configptr->conf_modules;

                xf86addNewLoadDirective(ptr, ModuleDefaults[i].name,
                                        XF86_LOAD_MODULE,
                                        ModuleDefaults[i].load_opt);
                xf86Msg(X_INFO, "\"%s\" will be loaded by default.\n",
                        ModuleDefaults[i].name);
            }
        }
    }
    else {
        xf86configptr->conf_modules = xnfcalloc(1, sizeof(XF86ConfModuleRec));
        for (i = 0; ModuleDefaults[i].name != NULL; i++) {
            if (ModuleDefaults[i].toLoad == TRUE) {
                XF86LoadPtr ptr = (XF86LoadPtr) xf86configptr->conf_modules;

                xf86addNewLoadDirective(ptr, ModuleDefaults[i].name,
                                        XF86_LOAD_MODULE,
                                        ModuleDefaults[i].load_opt);
            }
        }
    }

    /*
     * Walk the list of modules in the "Module" section to determine how
     * many we have.
     */
    modp = xf86configptr->conf_modules->mod_load_lst;
    while (modp) {
        for (i = 0; ignore[i]; i++) {
            if (strcmp(modp->load_name, ignore[i]) == 0)
                modp->ignore = 1;
        }
        if (!modp->ignore)
            count++;
        modp = (XF86LoadPtr) modp->list.next;
    }

    /*
     * allocate the memory and walk the list again to fill in the pointers
     */
    modulearray = xnfallocarray(count + 1, sizeof(char *));
    optarray = xnfallocarray(count + 1, sizeof(void *));
    count = 0;
    if (xf86configptr->conf_modules) {
        modp = xf86configptr->conf_modules->mod_load_lst;
        while (modp) {
            if (!modp->ignore) {
                modulearray[count] = modp->load_name;
                optarray[count] = modp->load_opt;
                count++;
            }
            modp = (XF86LoadPtr) modp->list.next;
        }
    }
    modulearray[count] = NULL;
    optarray[count] = NULL;
    if (optlist)
        *optlist = optarray;
    else
        free(optarray);
    return modulearray;
}

const char **
xf86DriverlistFromConfig(void)
{
    int count = 0;
    int j, k;
    const char **modulearray;
    screenLayoutPtr slp;

    /*
     * make sure the config file has been parsed and that we have a
     * ModulePath set; if no ModulePath was given, use the default
     * ModulePath
     */
    if (xf86configptr == NULL) {
        xf86Msg(X_ERROR, "Cannot access global config data structure\n");
        return NULL;
    }

    /*
     * Walk the list of driver lines in active "Device" sections to
     * determine now many implicitly loaded modules there are.
     *
     */
    if (xf86ConfigLayout.screens) {
        slp = xf86ConfigLayout.screens;
        while (slp->screen) {
            count++;
            count += slp->screen->num_gpu_devices;
            slp++;
        }
    }

    /*
     * Handle the set of inactive "Device" sections.
     */
    j = 0;
    while (xf86ConfigLayout.inactives[j++].identifier)
        count++;

    if (count == 0)
        return NULL;

    /*
     * allocate the memory and walk the list again to fill in the pointers
     */
    modulearray = xnfallocarray(count + 1, sizeof(char *));
    count = 0;
    slp = xf86ConfigLayout.screens;
    while (slp->screen) {
        modulearray[count] = slp->screen->device->driver;
        count++;
        for (k = 0; k < slp->screen->num_gpu_devices; k++) {
            modulearray[count] = slp->screen->gpu_devices[k]->driver;
            count++;
        }
        slp++;
    }

    j = 0;

    while (xf86ConfigLayout.inactives[j].identifier)
        modulearray[count++] = xf86ConfigLayout.inactives[j++].driver;

    modulearray[count] = NULL;

    /* Remove duplicates */
    for (count = 0; modulearray[count] != NULL; count++) {
        int i;

        for (i = 0; i < count; i++)
            if (xf86NameCmp(modulearray[i], modulearray[count]) == 0) {
                modulearray[count] = "";
                break;
            }
    }
    return modulearray;
}

const char **
xf86InputDriverlistFromConfig(void)
{
    int count = 0;
    const char **modulearray;
    InputInfoPtr *idp;

    /*
     * make sure the config file has been parsed and that we have a
     * ModulePath set; if no ModulePath was given, use the default
     * ModulePath
     */
    if (xf86configptr == NULL) {
        xf86Msg(X_ERROR, "Cannot access global config data structure\n");
        return NULL;
    }

    /*
     * Walk the list of driver lines in active "InputDevice" sections to
     * determine now many implicitly loaded modules there are.
     */
    if (xf86ConfigLayout.inputs) {
        idp = xf86ConfigLayout.inputs;
        while (*idp) {
            count++;
            idp++;
        }
    }

    if (count == 0)
        return NULL;

    /*
     * allocate the memory and walk the list again to fill in the pointers
     */
    modulearray = xnfallocarray(count + 1, sizeof(char *));
    count = 0;
    idp = xf86ConfigLayout.inputs;
    while (idp && *idp) {
        modulearray[count] = (*idp)->driver;
        count++;
        idp++;
    }
    modulearray[count] = NULL;

    /* Remove duplicates */
    for (count = 0; modulearray[count] != NULL; count++) {
        int i;

        for (i = 0; i < count; i++)
            if (xf86NameCmp(modulearray[i], modulearray[count]) == 0) {
                modulearray[count] = "";
                break;
            }
    }
    return modulearray;
}

static void
configFiles(XF86ConfFilesPtr fileconf)
{
    MessageType pathFrom;
    Bool must_copy;
    int size, countDirs;
    char *temp_path, *log_buf, *start, *end;

    /* FontPath */
    must_copy = TRUE;

    temp_path = defaultFontPath ? (char *) defaultFontPath : (char *) "";
    if (xf86fpFlag)
        pathFrom = X_CMDLINE;
    else if (fileconf && fileconf->file_fontpath) {
        pathFrom = X_CONFIG;
        if (xf86Info.useDefaultFontPath) {
            char *new_font_path;
            if (asprintf(&new_font_path, "%s%s%s", fileconf->file_fontpath,
                         *temp_path ? "," : "", temp_path) == -1)
                new_font_path = NULL;
            else
                must_copy = FALSE;
            defaultFontPath = new_font_path;
        }
        else
            defaultFontPath = fileconf->file_fontpath;
    }
    else
        pathFrom = X_DEFAULT;
    temp_path = defaultFontPath ? (char *) defaultFontPath : (char *) "";

    /* xf86ValidateFontPath modifies its argument, but returns a copy of it. */
    temp_path = must_copy ? xnfstrdup(defaultFontPath) : (char *) defaultFontPath;
    defaultFontPath = xf86ValidateFontPath(temp_path);
    free(temp_path);

    /* make fontpath more readable in the logfiles */
    countDirs = 1;
    temp_path = (char *) defaultFontPath;
    while ((temp_path = index(temp_path, ',')) != NULL) {
        countDirs++;
        temp_path++;
    }

    log_buf = xnfalloc(strlen(defaultFontPath) + (2 * countDirs) + 1);
    temp_path = log_buf;
    start = (char *) defaultFontPath;
    while ((end = index(start, ',')) != NULL) {
        size = (end - start) + 1;
        *(temp_path++) = '\t';
        strncpy(temp_path, start, size);
        temp_path += size;
        *(temp_path++) = '\n';
        start += size;
    }
    /* copy last entry */
    *(temp_path++) = '\t';
    strcpy(temp_path, start);
    xf86Msg(pathFrom, "FontPath set to:\n%s\n", log_buf);
    free(log_buf);

    /* ModulePath */

    if (fileconf) {
        if (xf86ModPathFrom != X_CMDLINE && fileconf->file_modulepath) {
            xf86ModulePath = fileconf->file_modulepath;
            xf86ModPathFrom = X_CONFIG;
        }
    }

    xf86Msg(xf86ModPathFrom, "ModulePath set to \"%s\"\n", xf86ModulePath);

    if (!xf86xkbdirFlag && fileconf && fileconf->file_xkbdir) {
        XkbBaseDirectory = fileconf->file_xkbdir;
        xf86Msg(X_CONFIG, "XKB base directory set to \"%s\"\n",
                XkbBaseDirectory);
    }
#if 0
    /* LogFile */
    /*
     * XXX The problem with this is that the log file is already open.
     * One option might be to copy the exiting contents to the new location.
     * and re-open it.  The down side is that the default location would
     * already have been overwritten.  Another option would be to start with
     * unique temporary location, then copy it once the correct name is known.
     * A problem with this is what happens if the server exits before that
     * happens.
     */
    if (xf86LogFileFrom == X_DEFAULT && fileconf->file_logfile) {
        xf86LogFile = fileconf->file_logfile;
        xf86LogFileFrom = X_CONFIG;
    }
#endif

    return;
}

typedef enum {
    FLAG_DONTVTSWITCH,
    FLAG_DONTZAP,
    FLAG_DONTZOOM,
    FLAG_DISABLEVIDMODE,
    FLAG_ALLOWNONLOCAL,
    FLAG_ALLOWMOUSEOPENFAIL,
    FLAG_SAVER_BLANKTIME,
    FLAG_DPMS_STANDBYTIME,
    FLAG_DPMS_SUSPENDTIME,
    FLAG_DPMS_OFFTIME,
    FLAG_NOPM,
    FLAG_XINERAMA,
    FLAG_LOG,
    FLAG_RENDER_COLORMAP_MODE,
    FLAG_IGNORE_ABI,
    FLAG_ALLOW_EMPTY_INPUT,
    FLAG_USE_DEFAULT_FONT_PATH,
    FLAG_AUTO_ADD_DEVICES,
    FLAG_AUTO_ENABLE_DEVICES,
    FLAG_GLX_VISUALS,
    FLAG_DRI2,
    FLAG_USE_SIGIO,
    FLAG_AUTO_ADD_GPU,
    FLAG_AUTO_BIND_GPU,
    FLAG_MAX_CLIENTS,
    FLAG_IGLX,
    FLAG_DEBUG,
} FlagValues;

/**
 * NOTE: the last value for each entry is NOT the default. It is set to TRUE
 * if the parser found the option in the config file.
 */
static OptionInfoRec FlagOptions[] = {
    {FLAG_DONTVTSWITCH, "DontVTSwitch", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_DONTZAP, "DontZap", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_DONTZOOM, "DontZoom", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_DISABLEVIDMODE, "DisableVidModeExtension", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_ALLOWNONLOCAL, "AllowNonLocalXvidtune", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_ALLOWMOUSEOPENFAIL, "AllowMouseOpenFail", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_SAVER_BLANKTIME, "BlankTime", OPTV_INTEGER,
     {0}, FALSE},
    {FLAG_DPMS_STANDBYTIME, "StandbyTime", OPTV_INTEGER,
     {0}, FALSE},
    {FLAG_DPMS_SUSPENDTIME, "SuspendTime", OPTV_INTEGER,
     {0}, FALSE},
    {FLAG_DPMS_OFFTIME, "OffTime", OPTV_INTEGER,
     {0}, FALSE},
    {FLAG_NOPM, "NoPM", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_XINERAMA, "Xinerama", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_LOG, "Log", OPTV_STRING,
     {0}, FALSE},
    {FLAG_RENDER_COLORMAP_MODE, "RenderColormapMode", OPTV_STRING,
     {0}, FALSE},
    {FLAG_IGNORE_ABI, "IgnoreABI", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_USE_DEFAULT_FONT_PATH, "UseDefaultFontPath", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_AUTO_ADD_DEVICES, "AutoAddDevices", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_AUTO_ENABLE_DEVICES, "AutoEnableDevices", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_GLX_VISUALS, "GlxVisuals", OPTV_STRING,
     {0}, FALSE},
    {FLAG_DRI2, "DRI2", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_USE_SIGIO, "UseSIGIO", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_AUTO_ADD_GPU, "AutoAddGPU", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_AUTO_BIND_GPU, "AutoBindGPU", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_MAX_CLIENTS, "MaxClients", OPTV_INTEGER,
     {0}, FALSE },
    {FLAG_IGLX, "IndirectGLX", OPTV_BOOLEAN,
     {0}, FALSE},
    {FLAG_DEBUG, "Debug", OPTV_STRING,
     {0}, FALSE},
    {-1, NULL, OPTV_NONE,
     {0}, FALSE},
};

static void
configServerFlags(XF86ConfFlagsPtr flagsconf, XF86OptionPtr layoutopts)
{
    XF86OptionPtr optp, tmp;
    int i;
    Bool value;
    MessageType from;
    const char *s;
    XkbRMLVOSet set;
    const char *rules;

    /*
     * Merge the ServerLayout and ServerFlags options.  The former have
     * precedence over the latter.
     */
    optp = NULL;
    if (flagsconf && flagsconf->flg_option_lst)
        optp = xf86optionListDup(flagsconf->flg_option_lst);
    if (layoutopts) {
        tmp = xf86optionListDup(layoutopts);
        if (optp)
            optp = xf86optionListMerge(optp, tmp);
        else
            optp = tmp;
    }

    xf86ProcessOptions(-1, optp, FlagOptions);

    xf86GetOptValBool(FlagOptions, FLAG_DONTVTSWITCH, &xf86Info.dontVTSwitch);
    xf86GetOptValBool(FlagOptions, FLAG_DONTZAP, &xf86Info.dontZap);
    xf86GetOptValBool(FlagOptions, FLAG_DONTZOOM, &xf86Info.dontZoom);

    xf86GetOptValBool(FlagOptions, FLAG_IGNORE_ABI, &xf86Info.ignoreABI);
    if (xf86Info.ignoreABI) {
        xf86Msg(X_CONFIG, "Ignoring ABI Version\n");
    }

    if (xf86IsOptionSet(FlagOptions, FLAG_AUTO_ADD_DEVICES)) {
        xf86GetOptValBool(FlagOptions, FLAG_AUTO_ADD_DEVICES,
                          &xf86Info.autoAddDevices);
        from = X_CONFIG;
    }
    else {
        from = X_DEFAULT;
    }
    xf86Msg(from, "%sutomatically adding devices\n",
            xf86Info.autoAddDevices ? "A" : "Not a");

    if (xf86IsOptionSet(FlagOptions, FLAG_AUTO_ENABLE_DEVICES)) {
        xf86GetOptValBool(FlagOptions, FLAG_AUTO_ENABLE_DEVICES,
                          &xf86Info.autoEnableDevices);
        from = X_CONFIG;
    }
    else {
        from = X_DEFAULT;
    }
    xf86Msg(from, "%sutomatically enabling devices\n",
            xf86Info.autoEnableDevices ? "A" : "Not a");

    if (xf86IsOptionSet(FlagOptions, FLAG_AUTO_ADD_GPU)) {
        xf86GetOptValBool(FlagOptions, FLAG_AUTO_ADD_GPU,
                          &xf86Info.autoAddGPU);
        from = X_CONFIG;
    }
    else {
        from = X_DEFAULT;
    }
    xf86Msg(from, "%sutomatically adding GPU devices\n",
            xf86Info.autoAddGPU ? "A" : "Not a");

    if (xf86AutoBindGPUDisabled) {
        xf86Info.autoBindGPU = FALSE;
        from = X_CMDLINE;
    }
    else if (xf86IsOptionSet(FlagOptions, FLAG_AUTO_BIND_GPU)) {
        xf86GetOptValBool(FlagOptions, FLAG_AUTO_BIND_GPU,
                          &xf86Info.autoBindGPU);
        from = X_CONFIG;
    }
    else {
        from = X_DEFAULT;
    }
    xf86Msg(from, "%sutomatically binding GPU devices\n",
            xf86Info.autoBindGPU ? "A" : "Not a");

    /*
     * Set things up based on the config file information.  Some of these
     * settings may be overridden later when the command line options are
     * checked.
     */
#ifdef XF86VIDMODE
    if (xf86GetOptValBool(FlagOptions, FLAG_DISABLEVIDMODE, &value))
        xf86Info.vidModeEnabled = !value;
    if (xf86GetOptValBool(FlagOptions, FLAG_ALLOWNONLOCAL, &value))
        xf86Info.vidModeAllowNonLocal = value;
#endif

    if (xf86GetOptValBool(FlagOptions, FLAG_ALLOWMOUSEOPENFAIL, &value))
        xf86Info.allowMouseOpenFail = value;

    xf86Info.pmFlag = TRUE;
    if (xf86GetOptValBool(FlagOptions, FLAG_NOPM, &value))
        xf86Info.pmFlag = !value;
    {
        if ((s = xf86GetOptValString(FlagOptions, FLAG_LOG))) {
            if (!xf86NameCmp(s, "flush")) {
                xf86Msg(X_CONFIG, "Flushing logfile enabled\n");
                LogSetParameter(XLOG_FLUSH, TRUE);
            }
            else if (!xf86NameCmp(s, "sync")) {
                xf86Msg(X_CONFIG, "Syncing logfile enabled\n");
                LogSetParameter(XLOG_FLUSH, TRUE);
                LogSetParameter(XLOG_SYNC, TRUE);
            }
            else {
                xf86Msg(X_WARNING, "Unknown Log option\n");
            }
        }
    }

    {
        if ((s = xf86GetOptValString(FlagOptions, FLAG_RENDER_COLORMAP_MODE))) {
            int policy = PictureParseCmapPolicy(s);

            if (policy == PictureCmapPolicyInvalid)
                xf86Msg(X_WARNING, "Unknown colormap policy \"%s\"\n", s);
            else {
                xf86Msg(X_CONFIG, "Render colormap policy set to %s\n", s);
                PictureCmapPolicy = policy;
            }
        }
    }

#ifdef GLXEXT
    xf86Info.glxVisuals = XF86_GlxVisualsTypical;
    xf86Info.glxVisualsFrom = X_DEFAULT;
    if ((s = xf86GetOptValString(FlagOptions, FLAG_GLX_VISUALS))) {
        if (!xf86NameCmp(s, "minimal")) {
            xf86Info.glxVisuals = XF86_GlxVisualsMinimal;
        }
        else if (!xf86NameCmp(s, "typical")) {
            xf86Info.glxVisuals = XF86_GlxVisualsTypical;
        }
        else if (!xf86NameCmp(s, "all")) {
            xf86Info.glxVisuals = XF86_GlxVisualsAll;
        }
        else {
            xf86Msg(X_WARNING, "Unknown GlxVisuals option\n");
        }
    }

    if (xf86Info.iglxFrom != X_CMDLINE) {
        if (xf86GetOptValBool(FlagOptions, FLAG_IGLX, &value)) {
            enableIndirectGLX = value;
            xf86Info.iglxFrom = X_CONFIG;
        }
    }
#endif

    xf86Info.debug = xf86GetOptValString(FlagOptions, FLAG_DEBUG);

    /* if we're not hotplugging, force some input devices to exist */
    xf86Info.forceInputDevices = !(xf86Info.autoAddDevices &&
                                   xf86Info.autoEnableDevices);

    /* when forcing input devices, we use kbd. otherwise evdev, so use the
     * evdev rules set. */
#if defined(__linux__)
    if (!xf86Info.forceInputDevices)
        rules = "evdev";
    else
#endif
        rules = "base";

    /* Xkb default options. */
    XkbInitRules(&set, rules, "pc105", "us", NULL, NULL);
    XkbSetRulesDflts(&set);
    XkbFreeRMLVOSet(&set, FALSE);

    xf86Info.useDefaultFontPath = TRUE;
    if (xf86GetOptValBool(FlagOptions, FLAG_USE_DEFAULT_FONT_PATH, &value)) {
        xf86Info.useDefaultFontPath = value;
    }

/* Make sure that timers don't overflow CARD32's after multiplying */
#define MAX_TIME_IN_MIN (0x7fffffff / MILLI_PER_MIN)

    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_SAVER_BLANKTIME, &i);
    if ((i >= 0) && (i < MAX_TIME_IN_MIN))
        ScreenSaverTime = defaultScreenSaverTime = i * MILLI_PER_MIN;
    else if (i != -1)
        ErrorF("BlankTime value %d outside legal range of 0 - %d minutes\n",
               i, MAX_TIME_IN_MIN);

#ifdef DPMSExtension
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_DPMS_STANDBYTIME, &i);
    if ((i >= 0) && (i < MAX_TIME_IN_MIN))
        DPMSStandbyTime = i * MILLI_PER_MIN;
    else if (i != -1)
        ErrorF("StandbyTime value %d outside legal range of 0 - %d minutes\n",
               i, MAX_TIME_IN_MIN);
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_DPMS_SUSPENDTIME, &i);
    if ((i >= 0) && (i < MAX_TIME_IN_MIN))
        DPMSSuspendTime = i * MILLI_PER_MIN;
    else if (i != -1)
        ErrorF("SuspendTime value %d outside legal range of 0 - %d minutes\n",
               i, MAX_TIME_IN_MIN);
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_DPMS_OFFTIME, &i);
    if ((i >= 0) && (i < MAX_TIME_IN_MIN))
        DPMSOffTime = i * MILLI_PER_MIN;
    else if (i != -1)
        ErrorF("OffTime value %d outside legal range of 0 - %d minutes\n",
               i, MAX_TIME_IN_MIN);
#endif

#ifdef PANORAMIX
    from = X_DEFAULT;
    if (!noPanoramiXExtension)
        from = X_CMDLINE;
    else if (xf86GetOptValBool(FlagOptions, FLAG_XINERAMA, &value)) {
        noPanoramiXExtension = !value;
        from = X_CONFIG;
    }
    if (!noPanoramiXExtension)
        xf86Msg(from, "Xinerama: enabled\n");
#endif

#ifdef DRI2
    xf86Info.dri2 = FALSE;
    xf86Info.dri2From = X_DEFAULT;
    if (xf86GetOptValBool(FlagOptions, FLAG_DRI2, &value)) {
        xf86Info.dri2 = value;
        xf86Info.dri2From = X_CONFIG;
    }
#endif

    from = X_DEFAULT;
    if (LimitClients != LIMITCLIENTS)
	from = X_CMDLINE;
    i = -1;
    if (xf86GetOptValInteger(FlagOptions, FLAG_MAX_CLIENTS, &i)) {
        if (Ones(i) != 1 || i < 64 || i > 2048) {
	    ErrorF("MaxClients must be one of 64, 128, 256, 512, 1024, or 2048\n");
        } else {
            from = X_CONFIG;
            LimitClients = i;
        }
    }
    xf86Msg(from, "Max clients allowed: %i, resource mask: 0x%x\n",
	    LimitClients, RESOURCE_ID_MASK);
}

Bool
xf86DRI2Enabled(void)
{
    return xf86Info.dri2;
}

/**
 * Search for the pInfo in the null-terminated list given and remove (and
 * free) it if present. All other devices are moved forward.
 */
static void
freeDevice(InputInfoPtr * list, InputInfoPtr pInfo)
{
    InputInfoPtr *devs;

    for (devs = list; devs && *devs; devs++) {
        if (*devs == pInfo) {
            free(*devs);
            for (; devs && *devs; devs++)
                devs[0] = devs[1];
            break;
        }
    }
}

/**
 * Append pInfo to the null-terminated list, allocating space as necessary.
 * pInfo is used as the last element.
 */
static InputInfoPtr *
addDevice(InputInfoPtr * list, InputInfoPtr pInfo)
{
    InputInfoPtr *devs;
    int count = 1;

    for (devs = list; devs && *devs; devs++)
        count++;

    list = xnfreallocarray(list, count + 1, sizeof(InputInfoPtr));
    list[count] = NULL;

    list[count - 1] = pInfo;
    return list;
}

/*
 * Locate the core input devices.  These can be specified/located in
 * the following ways, in order of priority:
 *
 *  1. The InputDevices named by the -pointer and -keyboard command line
 *     options.
 *  2. The "CorePointer" and "CoreKeyboard" InputDevices referred to by
 *     the active ServerLayout.
 *  3. The first InputDevices marked as "CorePointer" and "CoreKeyboard".
 *  4. The first InputDevices that use 'keyboard' or 'kbd' and a valid mouse
 *     driver (mouse, synaptics, evdev, vmmouse, void)
 *  5. Default devices with an empty (default) configuration.  These defaults
 *     will reference the 'mouse' and 'keyboard' drivers.
 */

static Bool
checkCoreInputDevices(serverLayoutPtr servlayoutp, Bool implicitLayout)
{
    InputInfoPtr corePointer = NULL, coreKeyboard = NULL;
    Bool foundPointer = FALSE, foundKeyboard = FALSE;
    const char *pointerMsg = NULL, *keyboardMsg = NULL;
    InputInfoPtr *devs,         /* iterator */
     indp;
    InputInfoPtr Pointer, Keyboard;
    XF86ConfInputPtr confInput;
    XF86ConfInputRec defPtr, defKbd;
    MessageType from = X_DEFAULT;

    const char *mousedrivers[] = { "mouse", "synaptics", "evdev", "vmmouse",
        "void", NULL
    };

    /*
     * First check if a core pointer or core keyboard have been specified
     * in the active ServerLayout.  If more than one is specified for either,
     * remove the core attribute from the later ones.
     */
    for (devs = servlayoutp->inputs; devs && *devs; devs++) {
        indp = *devs;
        if (indp->options &&
            xf86CheckBoolOption(indp->options, "CorePointer", FALSE)) {
            if (!corePointer) {
                corePointer = indp;
            }
        }
        if (indp->options &&
            xf86CheckBoolOption(indp->options, "CoreKeyboard", FALSE)) {
            if (!coreKeyboard) {
                coreKeyboard = indp;
            }
        }
    }

    confInput = NULL;

    /* 1. Check for the -pointer command line option. */
    if (xf86PointerName) {
        confInput = xf86findInput(xf86PointerName,
                                  xf86configptr->conf_input_lst);
        if (!confInput) {
            xf86Msg(X_ERROR, "No InputDevice section called \"%s\"\n",
                    xf86PointerName);
            return FALSE;
        }
        from = X_CMDLINE;
        /*
         * If one was already specified in the ServerLayout, it needs to be
         * removed.
         */
        if (corePointer) {
            freeDevice(servlayoutp->inputs, corePointer);
            corePointer = NULL;
        }
        foundPointer = TRUE;
    }

    /* 2. ServerLayout-specified core pointer. */
    if (corePointer) {
        foundPointer = TRUE;
        from = X_CONFIG;
    }

    /* 3. First core pointer device. */
    if (!foundPointer && (xf86Info.forceInputDevices || implicitLayout)) {
        XF86ConfInputPtr p;

        for (p = xf86configptr->conf_input_lst; p; p = p->list.next) {
            if (p->inp_option_lst &&
                xf86CheckBoolOption(p->inp_option_lst, "CorePointer", FALSE)) {
                confInput = p;
                foundPointer = TRUE;
                from = X_DEFAULT;
                pointerMsg = "first core pointer device";
                break;
            }
        }
    }

    /* 4. First pointer with an allowed mouse driver. */
    if (!foundPointer && xf86Info.forceInputDevices) {
        const char **driver = mousedrivers;

        confInput = xf86findInput(CONF_IMPLICIT_POINTER,
                                  xf86configptr->conf_input_lst);
        while (*driver && !confInput) {
            confInput = xf86findInputByDriver(*driver,
                                              xf86configptr->conf_input_lst);
            driver++;
        }
        if (confInput) {
            foundPointer = TRUE;
            from = X_DEFAULT;
            pointerMsg = "first mouse device";
        }
    }

    /* 5. Built-in default. */
    if (!foundPointer && xf86Info.forceInputDevices) {
        memset(&defPtr, 0, sizeof(defPtr));
        defPtr.inp_identifier = strdup("<default pointer>");
        defPtr.inp_driver = strdup("mouse");
        confInput = &defPtr;
        foundPointer = TRUE;
        from = X_DEFAULT;
        pointerMsg = "default mouse configuration";
    }

    /* Add the core pointer device to the layout, and set it to Core. */
    if (foundPointer && confInput) {
        Pointer = xf86AllocateInput();
        if (Pointer)
            foundPointer = configInput(Pointer, confInput, from);
        if (foundPointer) {
            Pointer->options = xf86AddNewOption(Pointer->options,
                                                "CorePointer", "on");
            Pointer->options = xf86AddNewOption(Pointer->options,
                                                "driver",
                                                confInput->inp_driver);
            Pointer->options =
                xf86AddNewOption(Pointer->options, "identifier",
                                 confInput->inp_identifier);
            servlayoutp->inputs = addDevice(servlayoutp->inputs, Pointer);
        }
    }

    if (!foundPointer && xf86Info.forceInputDevices) {
        /* This shouldn't happen. */
        xf86Msg(X_ERROR, "Cannot locate a core pointer device.\n");
        xf86DeleteInput(Pointer, 0);
        return FALSE;
    }

    confInput = NULL;

    /* 1. Check for the -keyboard command line option. */
    if (xf86KeyboardName) {
        confInput = xf86findInput(xf86KeyboardName,
                                  xf86configptr->conf_input_lst);
        if (!confInput) {
            xf86Msg(X_ERROR, "No InputDevice section called \"%s\"\n",
                    xf86KeyboardName);
            return FALSE;
        }
        from = X_CMDLINE;
        /*
         * If one was already specified in the ServerLayout, it needs to be
         * removed.
         */
        if (coreKeyboard) {
            freeDevice(servlayoutp->inputs, coreKeyboard);
            coreKeyboard = NULL;
        }
        foundKeyboard = TRUE;
    }

    /* 2. ServerLayout-specified core keyboard. */
    if (coreKeyboard) {
        foundKeyboard = TRUE;
        from = X_CONFIG;
    }

    /* 3. First core keyboard device. */
    if (!foundKeyboard && (xf86Info.forceInputDevices || implicitLayout)) {
        XF86ConfInputPtr p;

        for (p = xf86configptr->conf_input_lst; p; p = p->list.next) {
            if (p->inp_option_lst &&
                xf86CheckBoolOption(p->inp_option_lst, "CoreKeyboard", FALSE)) {
                confInput = p;
                foundKeyboard = TRUE;
                from = X_DEFAULT;
                keyboardMsg = "first core keyboard device";
                break;
            }
        }
    }

    /* 4. First keyboard with 'keyboard' or 'kbd' as the driver. */
    if (!foundKeyboard && xf86Info.forceInputDevices) {
        confInput = xf86findInput(CONF_IMPLICIT_KEYBOARD,
                                  xf86configptr->conf_input_lst);
        if (!confInput) {
            confInput = xf86findInputByDriver("kbd",
                                              xf86configptr->conf_input_lst);
        }
        if (confInput) {
            foundKeyboard = TRUE;
            from = X_DEFAULT;
            keyboardMsg = "first keyboard device";
        }
    }

    /* 5. Built-in default. */
    if (!foundKeyboard && xf86Info.forceInputDevices) {
        memset(&defKbd, 0, sizeof(defKbd));
        defKbd.inp_identifier = strdup("<default keyboard>");
        defKbd.inp_driver = strdup("kbd");
        confInput = &defKbd;
        foundKeyboard = TRUE;
        keyboardMsg = "default keyboard configuration";
        from = X_DEFAULT;
    }

    /* Add the core keyboard device to the layout, and set it to Core. */
    if (foundKeyboard && confInput) {
        Keyboard = xf86AllocateInput();
        if (Keyboard)
            foundKeyboard = configInput(Keyboard, confInput, from);
        if (foundKeyboard) {
            Keyboard->options = xf86AddNewOption(Keyboard->options,
                                                 "CoreKeyboard", "on");
            Keyboard->options = xf86AddNewOption(Keyboard->options,
                                                 "driver",
                                                 confInput->inp_driver);
            Keyboard->options =
                xf86AddNewOption(Keyboard->options, "identifier",
                                 confInput->inp_identifier);
            servlayoutp->inputs = addDevice(servlayoutp->inputs, Keyboard);
        }
    }

    if (!foundKeyboard && xf86Info.forceInputDevices) {
        /* This shouldn't happen. */
        xf86Msg(X_ERROR, "Cannot locate a core keyboard device.\n");
        xf86DeleteInput(Keyboard, 0);
        return FALSE;
    }

    if (pointerMsg) {
        if (implicitLayout)
            xf86Msg(X_DEFAULT, "No Layout section. Using the %s.\n",
                    pointerMsg);
        else
            xf86Msg(X_DEFAULT, "The core pointer device wasn't specified "
                    "explicitly in the layout.\n"
                    "\tUsing the %s.\n", pointerMsg);
    }

    if (keyboardMsg) {
        if (implicitLayout)
            xf86Msg(X_DEFAULT, "No Layout section. Using the %s.\n",
                    keyboardMsg);
        else
            xf86Msg(X_DEFAULT, "The core keyboard device wasn't specified "
                    "explicitly in the layout.\n"
                    "\tUsing the %s.\n", keyboardMsg);
    }

    if (!xf86Info.forceInputDevices && !(foundPointer && foundKeyboard)) {
#if defined(CONFIG_HAL) || defined(CONFIG_UDEV) || defined(CONFIG_WSCONS)
        const char *config_backend;

#if defined(CONFIG_HAL)
        config_backend = "HAL";
#elif defined(CONFIG_UDEV)
        config_backend = "udev";
#else
        config_backend = "wscons";
#endif
        xf86Msg(X_INFO, "The server relies on %s to provide the list of "
                "input devices.\n\tIf no devices become available, "
                "reconfigure %s or disable AutoAddDevices.\n",
                config_backend, config_backend);
#else
        xf86Msg(X_WARNING, "Hotplugging requested but the server was "
                "compiled without a config backend. "
                "No input devices were configured, the server "
                "will start without any input devices.\n");
#endif
    }

    return TRUE;
}

typedef enum {
    LAYOUT_ISOLATEDEVICE,
    LAYOUT_SINGLECARD
} LayoutValues;

static OptionInfoRec LayoutOptions[] = {
    {LAYOUT_ISOLATEDEVICE, "IsolateDevice", OPTV_STRING,
     {0}, FALSE},
    {LAYOUT_SINGLECARD, "SingleCard", OPTV_BOOLEAN,
     {0}, FALSE},
    {-1, NULL, OPTV_NONE,
     {0}, FALSE},
};

static Bool
configInputDevices(XF86ConfLayoutPtr layout, serverLayoutPtr servlayoutp)
{
    XF86ConfInputrefPtr irp;
    InputInfoPtr *indp;
    int count = 0;

    /*
     * Count the number of input devices.
     */
    irp = layout->lay_input_lst;
    while (irp) {
        count++;
        irp = (XF86ConfInputrefPtr) irp->list.next;
    }
    DebugF("Found %d input devices in the layout section %s\n",
           count, layout->lay_identifier);
    indp = xnfcalloc((count + 1), sizeof(InputInfoPtr));
    indp[count] = NULL;
    irp = layout->lay_input_lst;
    count = 0;
    while (irp) {
        indp[count] = xf86AllocateInput();
        if (!configInput(indp[count], irp->iref_inputdev, X_CONFIG)) {
            do {
                free(indp[count]);
            } while (count--);
            free(indp);
            return FALSE;
        }
        indp[count]->options = xf86OptionListMerge(indp[count]->options,
                                                   irp->iref_option_lst);
        count++;
        irp = (XF86ConfInputrefPtr) irp->list.next;
    }
    servlayoutp->inputs = indp;

    return TRUE;
}

/*
 * figure out which layout is active, which screens are used in that layout,
 * which drivers and monitors are used in these screens
 */
static Bool
configLayout(serverLayoutPtr servlayoutp, XF86ConfLayoutPtr conf_layout,
             char *default_layout)
{
    XF86ConfAdjacencyPtr adjp;
    XF86ConfInactivePtr idp;
    int saved_count, count = 0;
    int scrnum;
    XF86ConfLayoutPtr l;
    MessageType from;
    screenLayoutPtr slp;
    GDevPtr gdp;
    int i = 0, j;

    if (!servlayoutp)
        return FALSE;

    /*
     * which layout section is the active one?
     *
     * If there is a -layout command line option, use that one, otherwise
     * pick the first one.
     */
    from = X_DEFAULT;
    if (xf86LayoutName != NULL)
        from = X_CMDLINE;
    else if (default_layout) {
        xf86LayoutName = default_layout;
        from = X_CONFIG;
    }
    if (xf86LayoutName != NULL) {
        if ((l = xf86findLayout(xf86LayoutName, conf_layout)) == NULL) {
            xf86Msg(X_ERROR, "No ServerLayout section called \"%s\"\n",
                    xf86LayoutName);
            return FALSE;
        }
        conf_layout = l;
    }
    xf86Msg(from, "ServerLayout \"%s\"\n", conf_layout->lay_identifier);
    adjp = conf_layout->lay_adjacency_lst;

    /*
     * we know that each screen is referenced exactly once on the left side
     * of a layout statement in the Layout section. So to allocate the right
     * size for the array we do a quick walk of the list to figure out how
     * many sections we have
     */
    while (adjp) {
        count++;
        adjp = (XF86ConfAdjacencyPtr) adjp->list.next;
    }

    DebugF("Found %d screens in the layout section %s",
           count, conf_layout->lay_identifier);
    if (!count)                 /* alloc enough storage even if no screen is specified */
        count = 1;

    slp = xnfcalloc((count + 1), sizeof(screenLayoutRec));
    slp[count].screen = NULL;
    /*
     * now that we have storage, loop over the list again and fill in our
     * data structure; at this point we do not fill in the adjacency
     * information as it is not clear if we need it at all
     */
    adjp = conf_layout->lay_adjacency_lst;
    count = 0;
    while (adjp) {
        slp[count].screen = xnfcalloc(1, sizeof(confScreenRec));
        if (adjp->adj_scrnum < 0)
            scrnum = count;
        else
            scrnum = adjp->adj_scrnum;
        if (!configScreen(slp[count].screen, adjp->adj_screen, scrnum,
                          X_CONFIG, (scrnum == 0 && !adjp->list.next))) {
            do {
                free(slp[count].screen);
            } while (count--);
            free(slp);
            return FALSE;
        }
        slp[count].x = adjp->adj_x;
        slp[count].y = adjp->adj_y;
        slp[count].refname = adjp->adj_refscreen;
        switch (adjp->adj_where) {
        case CONF_ADJ_OBSOLETE:
            slp[count].where = PosObsolete;
            slp[count].topname = adjp->adj_top_str;
            slp[count].bottomname = adjp->adj_bottom_str;
            slp[count].leftname = adjp->adj_left_str;
            slp[count].rightname = adjp->adj_right_str;
            break;
        case CONF_ADJ_ABSOLUTE:
            slp[count].where = PosAbsolute;
            break;
        case CONF_ADJ_RIGHTOF:
            slp[count].where = PosRightOf;
            break;
        case CONF_ADJ_LEFTOF:
            slp[count].where = PosLeftOf;
            break;
        case CONF_ADJ_ABOVE:
            slp[count].where = PosAbove;
            break;
        case CONF_ADJ_BELOW:
            slp[count].where = PosBelow;
            break;
        case CONF_ADJ_RELATIVE:
            slp[count].where = PosRelative;
            break;
        }
        count++;
        adjp = (XF86ConfAdjacencyPtr) adjp->list.next;
    }

    /* No screen was specified in the layout. take the first one from the
     * config file, or - if it is NULL - configScreen autogenerates one for
     * us */
    if (!count) {
        XF86ConfScreenPtr screen;

        FIND_SUITABLE (XF86ConfScreenPtr, xf86configptr->conf_screen_lst, screen);
        slp[0].screen = xnfcalloc(1, sizeof(confScreenRec));
        if (!configScreen(slp[0].screen, screen,
                          0, X_CONFIG, TRUE)) {
            free(slp[0].screen);
            free(slp);
            return FALSE;
        }
    }

    /* XXX Need to tie down the upper left screen. */

    /* Fill in the refscreen and top/bottom/left/right values */
    for (i = 0; i < count; i++) {
        for (j = 0; j < count; j++) {
            if (slp[i].refname &&
                strcmp(slp[i].refname, slp[j].screen->id) == 0) {
                slp[i].refscreen = slp[j].screen;
            }
            if (slp[i].topname &&
                strcmp(slp[i].topname, slp[j].screen->id) == 0) {
                slp[i].top = slp[j].screen;
            }
            if (slp[i].bottomname &&
                strcmp(slp[i].bottomname, slp[j].screen->id) == 0) {
                slp[i].bottom = slp[j].screen;
            }
            if (slp[i].leftname &&
                strcmp(slp[i].leftname, slp[j].screen->id) == 0) {
                slp[i].left = slp[j].screen;
            }
            if (slp[i].rightname &&
                strcmp(slp[i].rightname, slp[j].screen->id) == 0) {
                slp[i].right = slp[j].screen;
            }
        }
        if (slp[i].where != PosObsolete
            && slp[i].where != PosAbsolute && !slp[i].refscreen) {
            xf86Msg(X_ERROR, "Screen %s doesn't exist: deleting placement\n",
                    slp[i].refname);
            slp[i].where = PosAbsolute;
            slp[i].x = 0;
            slp[i].y = 0;
        }
    }

    if (!count)
        saved_count = 1;
    else
        saved_count = count;
    /*
     * Count the number of inactive devices.
     */
    count = 0;
    idp = conf_layout->lay_inactive_lst;
    while (idp) {
        count++;
        idp = (XF86ConfInactivePtr) idp->list.next;
    }
    DebugF("Found %d inactive devices in the layout section %s\n",
           count, conf_layout->lay_identifier);
    gdp = xnfallocarray(count + 1, sizeof(GDevRec));
    gdp[count].identifier = NULL;
    idp = conf_layout->lay_inactive_lst;
    count = 0;
    while (idp) {
        if (!configDevice(&gdp[count], idp->inactive_device, FALSE, FALSE))
            goto bail;
        count++;
        idp = (XF86ConfInactivePtr) idp->list.next;
    }

    if (!configInputDevices(conf_layout, servlayoutp))
        goto bail;

    servlayoutp->id = conf_layout->lay_identifier;
    servlayoutp->screens = slp;
    servlayoutp->inactives = gdp;
    servlayoutp->options = conf_layout->lay_option_lst;
    from = X_DEFAULT;

    return TRUE;

 bail:
    do {
        free(slp[saved_count].screen);
    } while (saved_count--);
    free(slp);
    free(gdp);
    return FALSE;
}

/*
 * No layout section, so find the first Screen section and set that up as
 * the only active screen.
 */
static Bool
configImpliedLayout(serverLayoutPtr servlayoutp, XF86ConfScreenPtr conf_screen,
                    XF86ConfigPtr conf_ptr)
{
    MessageType from;
    XF86ConfScreenPtr s;
    screenLayoutPtr slp;
    InputInfoPtr *indp;
    XF86ConfLayoutRec layout;

    if (!servlayoutp)
        return FALSE;

    /*
     * which screen section is the active one?
     *
     * If there is a -screen option, use that one, otherwise use the first
     * one.
     */

    from = X_CONFIG;
    if (xf86ScreenName != NULL) {
        if ((s = xf86findScreen(xf86ScreenName, conf_screen)) == NULL) {
            xf86Msg(X_ERROR, "No Screen section called \"%s\"\n",
                    xf86ScreenName);
            return FALSE;
        }
        conf_screen = s;
        from = X_CMDLINE;
    }

    /* We have exactly one screen */

    slp = xnfcalloc(1, 2 * sizeof(screenLayoutRec));
    slp[0].screen = xnfcalloc(1, sizeof(confScreenRec));
    slp[1].screen = NULL;
    if (!configScreen(slp[0].screen, conf_screen, 0, from, TRUE)) {
        free(slp);
        return FALSE;
    }
    servlayoutp->id = "(implicit)";
    servlayoutp->screens = slp;
    servlayoutp->inactives = xnfcalloc(1, sizeof(GDevRec));
    servlayoutp->options = NULL;

    memset(&layout, 0, sizeof(layout));
    layout.lay_identifier = servlayoutp->id;
    if (xf86layoutAddInputDevices(conf_ptr, &layout) > 0) {
        if (!configInputDevices(&layout, servlayoutp))
            return FALSE;
        from = X_DEFAULT;
    }
    else {
        /* Set up an empty input device list, then look for some core devices. */
        indp = xnfalloc(sizeof(InputInfoPtr));
        *indp = NULL;
        servlayoutp->inputs = indp;
    }

    return TRUE;
}

static Bool
configXvAdaptor(confXvAdaptorPtr adaptor, XF86ConfVideoAdaptorPtr conf_adaptor)
{
    int count = 0;
    XF86ConfVideoPortPtr conf_port;

    xf86Msg(X_CONFIG, "|   |-->VideoAdaptor \"%s\"\n",
            conf_adaptor->va_identifier);
    adaptor->identifier = conf_adaptor->va_identifier;
    adaptor->options = conf_adaptor->va_option_lst;
    if (conf_adaptor->va_busid || conf_adaptor->va_driver) {
        xf86Msg(X_CONFIG, "|   | Unsupported device type, skipping entry\n");
        return FALSE;
    }

    /*
     * figure out how many videoport subsections there are and fill them in
     */
    conf_port = conf_adaptor->va_port_lst;
    while (conf_port) {
        count++;
        conf_port = (XF86ConfVideoPortPtr) conf_port->list.next;
    }
    adaptor->ports = xnfallocarray(count, sizeof(confXvPortRec));
    adaptor->numports = count;
    count = 0;
    conf_port = conf_adaptor->va_port_lst;
    while (conf_port) {
        adaptor->ports[count].identifier = conf_port->vp_identifier;
        adaptor->ports[count].options = conf_port->vp_option_lst;
        count++;
        conf_port = (XF86ConfVideoPortPtr) conf_port->list.next;
    }

    return TRUE;
}

static Bool
configScreen(confScreenPtr screenp, XF86ConfScreenPtr conf_screen, int scrnum,
             MessageType from, Bool auto_gpu_device)
{
    int count = 0;
    XF86ConfDisplayPtr dispptr;
    XF86ConfAdaptorLinkPtr conf_adaptor;
    Bool defaultMonitor = FALSE;
    XF86ConfScreenRec local_conf_screen;
    int i;

    if (!conf_screen) {
        memset(&local_conf_screen, 0, sizeof(local_conf_screen));
        conf_screen = &local_conf_screen;
        conf_screen->scrn_identifier = "Default Screen Section";
        xf86Msg(X_DEFAULT, "No screen section available. Using defaults.\n");
    }

    xf86Msg(from, "|-->Screen \"%s\" (%d)\n", conf_screen->scrn_identifier,
            scrnum);
    /*
     * now we fill in the elements of the screen
     */
    screenp->id = conf_screen->scrn_identifier;
    screenp->screennum = scrnum;
    screenp->defaultdepth = conf_screen->scrn_defaultdepth;
    screenp->defaultbpp = conf_screen->scrn_defaultbpp;
    screenp->defaultfbbpp = conf_screen->scrn_defaultfbbpp;
    screenp->monitor = xnfcalloc(1, sizeof(MonRec));
    /* If no monitor is specified, create a default one. */
    if (!conf_screen->scrn_monitor) {
        XF86ConfMonitorRec defMon;

        memset(&defMon, 0, sizeof(defMon));
        defMon.mon_identifier = "<default monitor>";
        if (!configMonitor(screenp->monitor, &defMon))
            return FALSE;
        defaultMonitor = TRUE;
    }
    else {
        if (!configMonitor(screenp->monitor, conf_screen->scrn_monitor))
            return FALSE;
    }
    /* Configure the device. If there isn't one configured, attach to the
     * first inactive one that we can configure. If there's none that work,
     * set it to NULL so that the section can be autoconfigured later */
    screenp->device = xnfcalloc(1, sizeof(GDevRec));
    if ((!conf_screen->scrn_device) && (xf86configptr->conf_device_lst)) {
        FIND_SUITABLE (XF86ConfDevicePtr, xf86configptr->conf_device_lst, conf_screen->scrn_device);
        xf86Msg(X_DEFAULT, "No device specified for screen \"%s\".\n"
                "\tUsing the first device section listed.\n", screenp->id);
    }
    if (configDevice(screenp->device, conf_screen->scrn_device, TRUE, FALSE)) {
        screenp->device->myScreenSection = screenp;
    }
    else {
        screenp->device = NULL;
    }

    if (auto_gpu_device && conf_screen->num_gpu_devices == 0 &&
        xf86configptr->conf_device_lst) {
        /* Loop through the entire device list and skip the primary device
         * assigned to the screen. This is important because there are two
         * cases where the assigned primary device is not the first device in
         * the device list. Firstly, if the first device in the list is assigned
         * to a different seat than this X server, it will not have been picked
         * by the previous FIND_SUITABLE. Secondly, if the device was explicitly
         * assigned in the config but there is still only one screen, this code
         * path is executed but the explicitly assigned device may not be the
         * first device in the list. */
        XF86ConfDevicePtr ptmp, sdevice = xf86configptr->conf_device_lst;

        for (i = 0; i < MAX_GPUDEVICES; i++) {
            if (!sdevice)
                break;

            FIND_SUITABLE (XF86ConfDevicePtr, sdevice, ptmp);
            if (!ptmp)
                break;

            /* skip the primary device on the screen */
            if (ptmp != conf_screen->scrn_device) {
                conf_screen->scrn_gpu_devices[i] = ptmp;
            } else {
                sdevice = ptmp->list.next;
                i--; /* run the next iteration with the same index */
                continue;
            }

            screenp->gpu_devices[i] = xnfcalloc(1, sizeof(GDevRec));
            if (configDevice(screenp->gpu_devices[i], conf_screen->scrn_gpu_devices[i], TRUE, TRUE)) {
                screenp->gpu_devices[i]->myScreenSection = screenp;
            }
            sdevice = conf_screen->scrn_gpu_devices[i]->list.next;
        }
        screenp->num_gpu_devices = i;

    } else {
        for (i = 0; i < conf_screen->num_gpu_devices; i++) {
            screenp->gpu_devices[i] = xnfcalloc(1, sizeof(GDevRec));
            if (configDevice(screenp->gpu_devices[i], conf_screen->scrn_gpu_devices[i], TRUE, TRUE)) {
                screenp->gpu_devices[i]->myScreenSection = screenp;
            }
        }
        screenp->num_gpu_devices = conf_screen->num_gpu_devices;
    }

    screenp->options = conf_screen->scrn_option_lst;

    /*
     * figure out how many display subsections there are and fill them in
     */
    dispptr = conf_screen->scrn_display_lst;
    while (dispptr) {
        count++;
        dispptr = (XF86ConfDisplayPtr) dispptr->list.next;
    }
    screenp->displays = xnfallocarray(count, sizeof(DispPtr));
    screenp->numdisplays = count;

    for (count = 0, dispptr = conf_screen->scrn_display_lst;
         dispptr;
         dispptr = (XF86ConfDisplayPtr) dispptr->list.next, count++) {

        /* Allocate individual Display records */
        screenp->displays[count] = xnfcalloc(1, sizeof(DispRec));

        /* Fill in the default Virtual size, if any */
        if (conf_screen->scrn_virtualX && conf_screen->scrn_virtualY) {
            screenp->displays[count]->virtualX = conf_screen->scrn_virtualX;
            screenp->displays[count]->virtualY = conf_screen->scrn_virtualY;
        }

        /* Now do the per-Display Virtual sizes */
        configDisplay(screenp->displays[count], dispptr);
    }

    /*
     * figure out how many videoadaptor references there are and fill them in
     */
    count = 0;
    conf_adaptor = conf_screen->scrn_adaptor_lst;
    while (conf_adaptor) {
        count++;
        conf_adaptor = (XF86ConfAdaptorLinkPtr) conf_adaptor->list.next;
    }
    screenp->xvadaptors = xnfallocarray(count, sizeof(confXvAdaptorRec));
    screenp->numxvadaptors = 0;
    conf_adaptor = conf_screen->scrn_adaptor_lst;
    while (conf_adaptor) {
        if (configXvAdaptor(&(screenp->xvadaptors[screenp->numxvadaptors]),
                            conf_adaptor->al_adaptor))
            screenp->numxvadaptors++;
        conf_adaptor = (XF86ConfAdaptorLinkPtr) conf_adaptor->list.next;
    }

    if (defaultMonitor) {
        xf86Msg(X_DEFAULT, "No monitor specified for screen \"%s\".\n"
                "\tUsing a default monitor configuration.\n", screenp->id);
    }
    return TRUE;
}

typedef enum {
    MON_REDUCEDBLANKING,
    MON_MAX_PIX_CLOCK,
} MonitorValues;

static OptionInfoRec MonitorOptions[] = {
    {MON_REDUCEDBLANKING, "ReducedBlanking", OPTV_BOOLEAN,
     {0}, FALSE},
    {MON_MAX_PIX_CLOCK, "MaxPixClock", OPTV_FREQ,
     {0}, FALSE},
    {-1, NULL, OPTV_NONE,
     {0}, FALSE},
};

static Bool
configMonitor(MonPtr monitorp, XF86ConfMonitorPtr conf_monitor)
{
    int count;
    DisplayModePtr mode, last = NULL;
    XF86ConfModeLinePtr cmodep;
    XF86ConfModesPtr modes;
    XF86ConfModesLinkPtr modeslnk = conf_monitor->mon_modes_sect_lst;
    Gamma zeros = { 0.0, 0.0, 0.0 };
    float badgamma = 0.0;
    double maxPixClock;

    xf86Msg(X_CONFIG, "|   |-->Monitor \"%s\"\n", conf_monitor->mon_identifier);
    monitorp->id = conf_monitor->mon_identifier;
    monitorp->vendor = conf_monitor->mon_vendor;
    monitorp->model = conf_monitor->mon_modelname;
    monitorp->Modes = NULL;
    monitorp->Last = NULL;
    monitorp->gamma = zeros;
    monitorp->widthmm = conf_monitor->mon_width;
    monitorp->heightmm = conf_monitor->mon_height;
    monitorp->reducedblanking = FALSE;
    monitorp->maxPixClock = 0;
    monitorp->options = conf_monitor->mon_option_lst;

    /*
     * fill in the monitor structure
     */
    for (count = 0;
         count < conf_monitor->mon_n_hsync && count < MAX_HSYNC; count++) {
        monitorp->hsync[count].hi = conf_monitor->mon_hsync[count].hi;
        monitorp->hsync[count].lo = conf_monitor->mon_hsync[count].lo;
    }
    monitorp->nHsync = count;
    for (count = 0;
         count < conf_monitor->mon_n_vrefresh && count < MAX_VREFRESH;
         count++) {
        monitorp->vrefresh[count].hi = conf_monitor->mon_vrefresh[count].hi;
        monitorp->vrefresh[count].lo = conf_monitor->mon_vrefresh[count].lo;
    }
    monitorp->nVrefresh = count;

    /*
     * first we collect the mode lines from the UseModes directive
     */
    while (modeslnk) {
        modes = xf86findModes(modeslnk->ml_modes_str,
                              xf86configptr->conf_modes_lst);
        modeslnk->ml_modes = modes;

        /* now add the modes found in the modes
           section to the list of modes for this
           monitor unless it has been added before
           because we are reusing the same section
           for another screen */
        if (xf86itemNotSublist((GenericListPtr) conf_monitor->mon_modeline_lst,
                               (GenericListPtr) modes->mon_modeline_lst)) {
            conf_monitor->mon_modeline_lst = (XF86ConfModeLinePtr)
                xf86addListItem((GenericListPtr) conf_monitor->mon_modeline_lst,
                                (GenericListPtr) modes->mon_modeline_lst);
        }
        modeslnk = modeslnk->list.next;
    }

    /*
     * we need to hook in the mode lines now
     * here both data structures use lists, only our internal one
     * is double linked
     */
    cmodep = conf_monitor->mon_modeline_lst;
    while (cmodep) {
        mode = xnfcalloc(1, sizeof(DisplayModeRec));
        mode->type = 0;
        mode->Clock = cmodep->ml_clock;
        mode->HDisplay = cmodep->ml_hdisplay;
        mode->HSyncStart = cmodep->ml_hsyncstart;
        mode->HSyncEnd = cmodep->ml_hsyncend;
        mode->HTotal = cmodep->ml_htotal;
        mode->VDisplay = cmodep->ml_vdisplay;
        mode->VSyncStart = cmodep->ml_vsyncstart;
        mode->VSyncEnd = cmodep->ml_vsyncend;
        mode->VTotal = cmodep->ml_vtotal;
        mode->Flags = cmodep->ml_flags;
        mode->HSkew = cmodep->ml_hskew;
        mode->VScan = cmodep->ml_vscan;
        mode->name = xnfstrdup(cmodep->ml_identifier);
        if (last) {
            mode->prev = last;
            last->next = mode;
        }
        else {
            /*
             * this is the first mode
             */
            monitorp->Modes = mode;
            mode->prev = NULL;
        }
        last = mode;
        cmodep = (XF86ConfModeLinePtr) cmodep->list.next;
    }
    if (last) {
        last->next = NULL;
    }
    monitorp->Last = last;

    /* add the (VESA) default modes */
    if (!addDefaultModes(monitorp))
        return FALSE;

    if (conf_monitor->mon_gamma_red > GAMMA_ZERO)
        monitorp->gamma.red = conf_monitor->mon_gamma_red;
    if (conf_monitor->mon_gamma_green > GAMMA_ZERO)
        monitorp->gamma.green = conf_monitor->mon_gamma_green;
    if (conf_monitor->mon_gamma_blue > GAMMA_ZERO)
        monitorp->gamma.blue = conf_monitor->mon_gamma_blue;

    /* Check that the gamma values are within range */
    if (monitorp->gamma.red > GAMMA_ZERO &&
        (monitorp->gamma.red < GAMMA_MIN || monitorp->gamma.red > GAMMA_MAX)) {
        badgamma = monitorp->gamma.red;
    }
    else if (monitorp->gamma.green > GAMMA_ZERO &&
             (monitorp->gamma.green < GAMMA_MIN ||
              monitorp->gamma.green > GAMMA_MAX)) {
        badgamma = monitorp->gamma.green;
    }
    else if (monitorp->gamma.blue > GAMMA_ZERO &&
             (monitorp->gamma.blue < GAMMA_MIN ||
              monitorp->gamma.blue > GAMMA_MAX)) {
        badgamma = monitorp->gamma.blue;
    }
    if (badgamma > GAMMA_ZERO) {
        ErrorF("Gamma value %.f is out of range (%.2f - %.1f)\n", badgamma,
               GAMMA_MIN, GAMMA_MAX);
        return FALSE;
    }

    xf86ProcessOptions(-1, monitorp->options, MonitorOptions);
    xf86GetOptValBool(MonitorOptions, MON_REDUCEDBLANKING,
                      &monitorp->reducedblanking);
    if (xf86GetOptValFreq(MonitorOptions, MON_MAX_PIX_CLOCK, OPTUNITS_KHZ,
                          &maxPixClock) == TRUE) {
        monitorp->maxPixClock = (int) maxPixClock;
    }

    return TRUE;
}

static int
lookupVisual(const char *visname)
{
    int i;

    if (!visname || !*visname)
        return -1;

    for (i = 0; i <= DirectColor; i++) {
        if (!xf86nameCompare(visname, xf86VisualNames[i]))
            break;
    }

    if (i <= DirectColor)
        return i;

    return -1;
}

static Bool
configDisplay(DispPtr displayp, XF86ConfDisplayPtr conf_display)
{
    int count = 0;
    XF86ModePtr modep;

    displayp->frameX0 = conf_display->disp_frameX0;
    displayp->frameY0 = conf_display->disp_frameY0;
    displayp->virtualX = conf_display->disp_virtualX;
    displayp->virtualY = conf_display->disp_virtualY;
    displayp->depth = conf_display->disp_depth;
    displayp->fbbpp = conf_display->disp_bpp;
    displayp->weight.red = conf_display->disp_weight.red;
    displayp->weight.green = conf_display->disp_weight.green;
    displayp->weight.blue = conf_display->disp_weight.blue;
    displayp->blackColour.red = conf_display->disp_black.red;
    displayp->blackColour.green = conf_display->disp_black.green;
    displayp->blackColour.blue = conf_display->disp_black.blue;
    displayp->whiteColour.red = conf_display->disp_white.red;
    displayp->whiteColour.green = conf_display->disp_white.green;
    displayp->whiteColour.blue = conf_display->disp_white.blue;
    displayp->options = conf_display->disp_option_lst;
    if (conf_display->disp_visual) {
        displayp->defaultVisual = lookupVisual(conf_display->disp_visual);
        if (displayp->defaultVisual == -1) {
            ErrorF("Invalid visual name: \"%s\"\n", conf_display->disp_visual);
            return FALSE;
        }
    }
    else {
        displayp->defaultVisual = -1;
    }

    /*
     * now hook in the modes
     */
    modep = conf_display->disp_mode_lst;
    while (modep) {
        count++;
        modep = (XF86ModePtr) modep->list.next;
    }
    displayp->modes = xnfallocarray(count + 1, sizeof(char *));
    modep = conf_display->disp_mode_lst;
    count = 0;
    while (modep) {
        displayp->modes[count] = modep->mode_name;
        count++;
        modep = (XF86ModePtr) modep->list.next;
    }
    displayp->modes[count] = NULL;

    return TRUE;
}

static Bool
configDevice(GDevPtr devicep, XF86ConfDevicePtr conf_device, Bool active, Bool gpu)
{
    int i;

    if (!conf_device) {
        return FALSE;
    }

    if (active) {
        if (gpu)
            xf86Msg(X_CONFIG, "|   |-->GPUDevice \"%s\"\n",
                    conf_device->dev_identifier);
        else
            xf86Msg(X_CONFIG, "|   |-->Device \"%s\"\n",
                    conf_device->dev_identifier);
    } else
        xf86Msg(X_CONFIG, "|-->Inactive Device \"%s\"\n",
                conf_device->dev_identifier);

    devicep->identifier = conf_device->dev_identifier;
    devicep->vendor = conf_device->dev_vendor;
    devicep->board = conf_device->dev_board;
    devicep->chipset = conf_device->dev_chipset;
    devicep->ramdac = conf_device->dev_ramdac;
    devicep->driver = conf_device->dev_driver;
    devicep->active = active;
    devicep->videoRam = conf_device->dev_videoram;
    devicep->MemBase = conf_device->dev_mem_base;
    devicep->IOBase = conf_device->dev_io_base;
    devicep->clockchip = conf_device->dev_clockchip;
    devicep->busID = conf_device->dev_busid;
    devicep->chipID = conf_device->dev_chipid;
    devicep->chipRev = conf_device->dev_chiprev;
    devicep->options = conf_device->dev_option_lst;
    devicep->irq = conf_device->dev_irq;
    devicep->screen = conf_device->dev_screen;

    for (i = 0; i < MAXDACSPEEDS; i++) {
        if (i < CONF_MAXDACSPEEDS)
            devicep->dacSpeeds[i] = conf_device->dev_dacSpeeds[i];
        else
            devicep->dacSpeeds[i] = 0;
    }
    devicep->numclocks = conf_device->dev_clocks;
    if (devicep->numclocks > MAXCLOCKS)
        devicep->numclocks = MAXCLOCKS;
    for (i = 0; i < devicep->numclocks; i++) {
        devicep->clock[i] = conf_device->dev_clock[i];
    }
    devicep->claimed = FALSE;

    return TRUE;
}

static void
configDRI(XF86ConfDRIPtr drip)
{
    struct group *grp;

    xf86ConfigDRI.group = -1;
    xf86ConfigDRI.mode = 0;

    if (drip) {
        if (drip->dri_group_name) {
            if ((grp = getgrnam(drip->dri_group_name)))
                xf86ConfigDRI.group = grp->gr_gid;
        }
        else {
            if (drip->dri_group >= 0)
                xf86ConfigDRI.group = drip->dri_group;
        }
        xf86ConfigDRI.mode = drip->dri_mode;
    }
}

static void
configExtensions(XF86ConfExtensionsPtr conf_ext)
{
    XF86OptionPtr o;

    if (conf_ext && conf_ext->ext_option_lst) {
        for (o = conf_ext->ext_option_lst; o; o = xf86NextOption(o)) {
            char *name = xf86OptionName(o);
            char *val = xf86OptionValue(o);
            char *n;
            Bool enable = TRUE;

            /* Handle "No<ExtensionName>" */
            n = xf86NormalizeName(name);
            if (strncmp(n, "no", 2) == 0) {
                name += 2;
                enable = FALSE;
            }

            if (!val ||
                xf86NameCmp(val, "enable") == 0 ||
                xf86NameCmp(val, "enabled") == 0 ||
                xf86NameCmp(val, "on") == 0 ||
                xf86NameCmp(val, "1") == 0 ||
                xf86NameCmp(val, "yes") == 0 || xf86NameCmp(val, "true") == 0) {
                /* NOTHING NEEDED -- enabling is handled below */
            }
            else if (xf86NameCmp(val, "disable") == 0 ||
                     xf86NameCmp(val, "disabled") == 0 ||
                     xf86NameCmp(val, "off") == 0 ||
                     xf86NameCmp(val, "0") == 0 ||
                     xf86NameCmp(val, "no") == 0 ||
                     xf86NameCmp(val, "false") == 0) {
                enable = !enable;
            }
            else {
                xf86Msg(X_WARNING, "Ignoring unrecognized value \"%s\"\n", val);
                free(n);
                continue;
            }

            if (EnableDisableExtension(name, enable)) {
                xf86Msg(X_CONFIG, "Extension \"%s\" is %s\n",
                        name, enable ? "enabled" : "disabled");
            }
            else {
                xf86Msg(X_WARNING, "Ignoring unrecognized extension \"%s\"\n",
                        name);
            }
            free(n);
        }
    }
}

static Bool
configInput(InputInfoPtr inputp, XF86ConfInputPtr conf_input, MessageType from)
{
    xf86Msg(from, "|-->Input Device \"%s\"\n", conf_input->inp_identifier);
    inputp->name = conf_input->inp_identifier;
    inputp->driver = conf_input->inp_driver;
    inputp->options = conf_input->inp_option_lst;
    inputp->attrs = NULL;

    return TRUE;
}

static Bool
modeIsPresent(DisplayModePtr mode, MonPtr monitorp)
{
    DisplayModePtr knownmodes = monitorp->Modes;

    /* all I can think of is a linear search... */
    while (knownmodes != NULL) {
        if (!strcmp(mode->name, knownmodes->name) &&
            !(knownmodes->type & M_T_DEFAULT))
            return TRUE;
        knownmodes = knownmodes->next;
    }
    return FALSE;
}

static Bool
addDefaultModes(MonPtr monitorp)
{
    DisplayModePtr mode;
    DisplayModePtr last = monitorp->Last;
    int i = 0;

    for (i = 0; i < xf86NumDefaultModes; i++) {
        mode = xf86DuplicateMode(&xf86DefaultModes[i]);
        if (!modeIsPresent(mode, monitorp)) {
            monitorp->Modes = xf86ModesAdd(monitorp->Modes, mode);
            last = mode;
        }
        else {
            free(mode);
        }
    }
    monitorp->Last = last;

    return TRUE;
}

static void
checkInput(serverLayoutPtr layout, Bool implicit_layout)
{
    checkCoreInputDevices(layout, implicit_layout);

    /* Unless we're forcing input devices, disable mouse/kbd devices in the
     * config. Otherwise the same physical device is added multiple times,
     * leading to duplicate events.
     */
    if (!xf86Info.forceInputDevices && layout->inputs) {
        InputInfoPtr *dev = layout->inputs;
        BOOL warned = FALSE;

        while (*dev) {
            if (strcmp((*dev)->driver, "kbd") == 0 ||
                strcmp((*dev)->driver, "mouse") == 0 ||
                strcmp((*dev)->driver, "vmmouse") == 0) {
                InputInfoPtr *current;

                if (!warned) {
                    xf86Msg(X_WARNING, "Hotplugging is on, devices using "
                            "drivers 'kbd', 'mouse' or 'vmmouse' will be disabled.\n");
                    warned = TRUE;
                }

                xf86Msg(X_WARNING, "Disabling %s\n", (*dev)->name);

                current = dev;
                free(*dev);
                *dev = NULL;

                do {
                    *current = *(current + 1);
                    current++;
                } while (*current);
            }
            else
                dev++;
        }
    }
}

/*
 * load the config file and fill the global data structure
 */
ConfigStatus
xf86HandleConfigFile(Bool autoconfig)
{
#ifdef XSERVER_LIBPCIACCESS
    const char *scanptr;
    Bool singlecard = 0;
#endif
    Bool implicit_layout = FALSE;
    XF86ConfLayoutPtr layout;

    if (!autoconfig) {
        char *filename, *dirname, *sysdirname;
        const char *filesearch, *dirsearch;
        MessageType filefrom = X_DEFAULT;
        MessageType dirfrom = X_DEFAULT;

        if (!PrivsElevated()) {
            filesearch = ALL_CONFIGPATH;
            dirsearch = ALL_CONFIGDIRPATH;
        }
        else {
            filesearch = RESTRICTED_CONFIGPATH;
            dirsearch = RESTRICTED_CONFIGDIRPATH;
        }

        if (xf86ConfigFile)
            filefrom = X_CMDLINE;
        if (xf86ConfigDir)
            dirfrom = X_CMDLINE;

        xf86initConfigFiles();
        sysdirname = xf86openConfigDirFiles(SYS_CONFIGDIRPATH, NULL,
                                            PROJECTROOT);
        dirname = xf86openConfigDirFiles(dirsearch, xf86ConfigDir, PROJECTROOT);
        filename = xf86openConfigFile(filesearch, xf86ConfigFile, PROJECTROOT);
        if (filename) {
            xf86MsgVerb(filefrom, 0, "Using config file: \"%s\"\n", filename);
            xf86ConfigFile = xnfstrdup(filename);
        }
        else {
            if (xf86ConfigFile)
                xf86Msg(X_ERROR, "Unable to locate/open config file: \"%s\"\n",
                        xf86ConfigFile);
        }
        if (dirname) {
            xf86MsgVerb(dirfrom, 0, "Using config directory: \"%s\"\n",
                        dirname);
            xf86ConfigDir = xnfstrdup(dirname);
        }
        else {
            if (xf86ConfigDir)
                xf86Msg(X_ERROR,
                        "Unable to locate/open config directory: \"%s\"\n",
                        xf86ConfigDir);
        }
        if (sysdirname)
            xf86MsgVerb(X_DEFAULT, 0, "Using system config directory \"%s\"\n",
                        sysdirname);
        if (!filename && !dirname && !sysdirname)
            return CONFIG_NOFILE;

        free(filename);
        free(dirname);
        free(sysdirname);
    }

    if ((xf86configptr = xf86readConfigFile()) == NULL) {
        xf86Msg(X_ERROR, "Problem parsing the config file\n");
        return CONFIG_PARSE_ERROR;
    }
    xf86closeConfigFile();

    /* Initialise a few things. */

    /*
     * now we convert part of the information contained in the parser
     * structures into our own structures.
     * The important part here is to figure out which Screen Sections
     * in the XF86Config file are active so that we can piece together
     * the modes that we need later down the road.
     * And while we are at it, we'll decode the rest of the stuff as well
     */

    /* First check if a layout section is present, and if it is valid. */
    FIND_SUITABLE(XF86ConfLayoutPtr, xf86configptr->conf_layout_lst, layout);
    if (layout == NULL || xf86ScreenName != NULL) {
        XF86ConfScreenPtr screen;

        if (xf86ScreenName == NULL) {
            xf86Msg(X_DEFAULT,
                    "No Layout section.  Using the first Screen section.\n");
        }
        FIND_SUITABLE (XF86ConfScreenPtr, xf86configptr->conf_screen_lst, screen);
        if (!configImpliedLayout(&xf86ConfigLayout,
                                 screen,
                                 xf86configptr)) {
            xf86Msg(X_ERROR, "Unable to determine the screen layout\n");
            return CONFIG_PARSE_ERROR;
        }
        implicit_layout = TRUE;
    }
    else {
        if (xf86configptr->conf_flags != NULL) {
            char *dfltlayout = NULL;
            void *optlist = xf86configptr->conf_flags->flg_option_lst;

            if (optlist && xf86FindOption(optlist, "defaultserverlayout"))
                dfltlayout =
                    xf86SetStrOption(optlist, "defaultserverlayout", NULL);
            if (!configLayout(&xf86ConfigLayout, layout, dfltlayout)) {
                xf86Msg(X_ERROR, "Unable to determine the screen layout\n");
                return CONFIG_PARSE_ERROR;
            }
        }
        else {
            if (!configLayout(&xf86ConfigLayout, layout, NULL)) {
                xf86Msg(X_ERROR, "Unable to determine the screen layout\n");
                return CONFIG_PARSE_ERROR;
            }
        }
    }

    xf86ProcessOptions(-1, xf86ConfigLayout.options, LayoutOptions);
#ifdef XSERVER_LIBPCIACCESS
    if ((scanptr = xf86GetOptValString(LayoutOptions, LAYOUT_ISOLATEDEVICE))) {
        ;                       /* IsolateDevice specified; overrides SingleCard */
    }
    else {
        xf86GetOptValBool(LayoutOptions, LAYOUT_SINGLECARD, &singlecard);
        if (singlecard)
            scanptr = xf86ConfigLayout.screens->screen->device->busID;
    }
    if (scanptr) {
        if (strncmp(scanptr, "PCI:", 4) != 0) {
            xf86Msg(X_WARNING, "Bus types other than PCI not yet isolable.\n"
                    "\tIgnoring IsolateDevice option.\n");
        }
        else
            xf86PciIsolateDevice(scanptr);
    }
#endif
    /* Now process everything else */
    configServerFlags(xf86configptr->conf_flags, xf86ConfigLayout.options);
    configFiles(xf86configptr->conf_files);
    configExtensions(xf86configptr->conf_extensions);
    configDRI(xf86configptr->conf_dri);

    checkInput(&xf86ConfigLayout, implicit_layout);

    /*
     * Handle some command line options that can override some of the
     * ServerFlags settings.
     */
#ifdef XF86VIDMODE
    if (xf86VidModeDisabled)
        xf86Info.vidModeEnabled = FALSE;
    if (xf86VidModeAllowNonLocal)
        xf86Info.vidModeAllowNonLocal = TRUE;
#endif

    if (xf86AllowMouseOpenFail)
        xf86Info.allowMouseOpenFail = TRUE;

    return CONFIG_OK;
}

Bool
xf86PathIsSafe(const char *path)
{
    return (xf86pathIsSafe(path) != 0);
}
