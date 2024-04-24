/*
 * Copyright 2000-2002 by Alan Hourihane, Flint Mountain, North Wales.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86Config.h"
#include "xf86_OSlib.h"
#include "xf86Priv.h"
#define IN_XSERVER
#include "Configint.h"
#include "xf86DDC.h"
#include "xf86pciBus.h"
#if (defined(__sparc__) || defined(__sparc)) && !defined(__OpenBSD__)
#include "xf86Bus.h"
#include "xf86Sbus.h"
#endif
#include "misc.h"
#include "loaderProcs.h"

typedef struct _DevToConfig {
    GDevRec GDev;
    struct pci_device *pVideo;
#if (defined(__sparc__) || defined(__sparc)) && !defined(__OpenBSD__)
    sbusDevicePtr sVideo;
#endif
    int iDriver;
} DevToConfigRec, *DevToConfigPtr;

static DevToConfigPtr DevToConfig = NULL;
static int nDevToConfig = 0, CurrentDriver;

xf86MonPtr ConfiguredMonitor;
Bool xf86DoConfigurePass1 = TRUE;
static Bool foundMouse = FALSE;

#if   defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
static const char *DFLT_MOUSE_DEV = "/dev/sysmouse";
static const char *DFLT_MOUSE_PROTO = "auto";
#elif defined(__linux__)
static const char *DFLT_MOUSE_DEV = "/dev/input/mice";
static const char *DFLT_MOUSE_PROTO = "auto";
#elif defined(WSCONS_SUPPORT)
static const char *DFLT_MOUSE_DEV = "/dev/wsmouse";
static const char *DFLT_MOUSE_PROTO = "wsmouse";
#else
static const char *DFLT_MOUSE_DEV = "/dev/mouse";
static const char *DFLT_MOUSE_PROTO = "auto";
#endif

/*
 * This is called by the driver, either through xf86Match???Instances() or
 * directly.  We allocate a GDevRec and fill it in as much as we can, letting
 * the caller fill in the rest and/or change it as it sees fit.
 */
GDevPtr
xf86AddBusDeviceToConfigure(const char *driver, BusType bus, void *busData,
                            int chipset)
{
    int ret, i, j;
    char *lower_driver;

    if (!xf86DoConfigure || !xf86DoConfigurePass1)
        return NULL;

    /* Check for duplicates */
    for (i = 0; i < nDevToConfig; i++) {
        switch (bus) {
#ifdef XSERVER_LIBPCIACCESS
        case BUS_PCI:
            ret = xf86PciConfigure(busData, DevToConfig[i].pVideo);
            break;
#endif
#if (defined(__sparc__) || defined(__sparc)) && !defined(__OpenBSD__)
        case BUS_SBUS:
            ret = xf86SbusConfigure(busData, DevToConfig[i].sVideo);
            break;
#endif
        default:
            return NULL;
        }
        if (ret == 0)
            goto out;
    }

    /* Allocate new structure occurrence */
    i = nDevToConfig++;
    DevToConfig =
        xnfreallocarray(DevToConfig, nDevToConfig, sizeof(DevToConfigRec));
    memset(DevToConfig + i, 0, sizeof(DevToConfigRec));

    DevToConfig[i].GDev.chipID =
        DevToConfig[i].GDev.chipRev = DevToConfig[i].GDev.irq = -1;

    DevToConfig[i].iDriver = CurrentDriver;

    /* Fill in what we know, converting the driver name to lower case */
    lower_driver = xnfalloc(strlen(driver) + 1);
    for (j = 0; (lower_driver[j] = tolower(driver[j])); j++);
    DevToConfig[i].GDev.driver = lower_driver;

    switch (bus) {
#ifdef XSERVER_LIBPCIACCESS
    case BUS_PCI:
	DevToConfig[i].pVideo = busData;
        xf86PciConfigureNewDev(busData, DevToConfig[i].pVideo,
                               &DevToConfig[i].GDev, &chipset);
        break;
#endif
#if (defined(__sparc__) || defined(__sparc)) && !defined(__OpenBSD__)
    case BUS_SBUS:
	DevToConfig[i].sVideo = busData;
        xf86SbusConfigureNewDev(busData, DevToConfig[i].sVideo,
                                &DevToConfig[i].GDev);
        break;
#endif
    default:
        break;
    }

    /* Get driver's available options */
    if (xf86DriverList[CurrentDriver]->AvailableOptions)
        DevToConfig[i].GDev.options = (OptionInfoPtr)
            (*xf86DriverList[CurrentDriver]->AvailableOptions) (chipset, bus);

    return &DevToConfig[i].GDev;

 out:
    return NULL;
}

static XF86ConfInputPtr
configureInputSection(void)
{
    XF86ConfInputPtr mouse = NULL;

    parsePrologue(XF86ConfInputPtr, XF86ConfInputRec);

    ptr->inp_identifier = xnfstrdup("Keyboard0");
    ptr->inp_driver = xnfstrdup("kbd");
    ptr->list.next = NULL;

    /* Crude mechanism to auto-detect mouse (os dependent) */
    {
        int fd;

        fd = open(DFLT_MOUSE_DEV, 0);
        if (fd != -1) {
            foundMouse = TRUE;
            close(fd);
        }
    }

    mouse = calloc(1, sizeof(XF86ConfInputRec));
    mouse->inp_identifier = xnfstrdup("Mouse0");
    mouse->inp_driver = xnfstrdup("mouse");
    mouse->inp_option_lst =
        xf86addNewOption(mouse->inp_option_lst, xnfstrdup("Protocol"),
                         xnfstrdup(DFLT_MOUSE_PROTO));
    mouse->inp_option_lst =
        xf86addNewOption(mouse->inp_option_lst, xnfstrdup("Device"),
                         xnfstrdup(DFLT_MOUSE_DEV));
    mouse->inp_option_lst =
        xf86addNewOption(mouse->inp_option_lst, xnfstrdup("ZAxisMapping"),
                         xnfstrdup("4 5 6 7"));
    ptr = (XF86ConfInputPtr) xf86addListItem((glp) ptr, (glp) mouse);
    return ptr;
}

static XF86ConfScreenPtr
configureScreenSection(int screennum)
{
    int i;
    int depths[] = { 1, 4, 8, 15, 16, 24 /*, 32 */  };
    char *tmp;
    parsePrologue(XF86ConfScreenPtr, XF86ConfScreenRec);

    XNFasprintf(&tmp, "Screen%d", screennum);
    ptr->scrn_identifier = tmp;
    XNFasprintf(&tmp, "Monitor%d", screennum);
    ptr->scrn_monitor_str = tmp;
    XNFasprintf(&tmp, "Card%d", screennum);
    ptr->scrn_device_str = tmp;

    for (i = 0; i < ARRAY_SIZE(depths); i++) {
        XF86ConfDisplayPtr conf_display;

        conf_display = calloc(1, sizeof(XF86ConfDisplayRec));
        conf_display->disp_depth = depths[i];
        conf_display->disp_black.red = conf_display->disp_white.red = -1;
        conf_display->disp_black.green = conf_display->disp_white.green = -1;
        conf_display->disp_black.blue = conf_display->disp_white.blue = -1;
        ptr->scrn_display_lst = (XF86ConfDisplayPtr) xf86addListItem((glp) ptr->
                                                                     scrn_display_lst,
                                                                     (glp)
                                                                     conf_display);
    }

    return ptr;
}

static const char *
optionTypeToString(OptionValueType type)
{
    switch (type) {
    case OPTV_NONE:
        return "";
    case OPTV_INTEGER:
        return "<i>";
    case OPTV_STRING:
        return "<str>";
    case OPTV_ANYSTR:
        return "[<str>]";
    case OPTV_REAL:
        return "<f>";
    case OPTV_BOOLEAN:
        return "[<bool>]";
    case OPTV_FREQ:
        return "<freq>";
    case OPTV_PERCENT:
        return "<percent>";
    default:
        return "";
    }
}

static XF86ConfDevicePtr
configureDeviceSection(int screennum)
{
    OptionInfoPtr p;
    int i = 0;
    char *identifier;

    parsePrologue(XF86ConfDevicePtr, XF86ConfDeviceRec);

    /* Move device info to parser structure */
   if (asprintf(&identifier, "Card%d", screennum) == -1)
        identifier = NULL;
    ptr->dev_identifier = identifier;
    ptr->dev_chipset = DevToConfig[screennum].GDev.chipset;
    ptr->dev_busid = DevToConfig[screennum].GDev.busID;
    ptr->dev_driver = DevToConfig[screennum].GDev.driver;
    ptr->dev_ramdac = DevToConfig[screennum].GDev.ramdac;
    for (i = 0; i < MAXDACSPEEDS; i++)
        ptr->dev_dacSpeeds[i] = DevToConfig[screennum].GDev.dacSpeeds[i];
    ptr->dev_videoram = DevToConfig[screennum].GDev.videoRam;
    ptr->dev_mem_base = DevToConfig[screennum].GDev.MemBase;
    ptr->dev_io_base = DevToConfig[screennum].GDev.IOBase;
    ptr->dev_clockchip = DevToConfig[screennum].GDev.clockchip;
    for (i = 0; (i < MAXCLOCKS) && (i < DevToConfig[screennum].GDev.numclocks);
         i++)
        ptr->dev_clock[i] = DevToConfig[screennum].GDev.clock[i];
    ptr->dev_clocks = i;
    ptr->dev_chipid = DevToConfig[screennum].GDev.chipID;
    ptr->dev_chiprev = DevToConfig[screennum].GDev.chipRev;
    ptr->dev_irq = DevToConfig[screennum].GDev.irq;

    /* Make sure older drivers don't segv */
    if (DevToConfig[screennum].GDev.options) {
        /* Fill in the available driver options for people to use */
        const char *descrip =
            "        ### Available Driver options are:-\n"
            "        ### Values: <i>: integer, <f>: float, "
            "<bool>: \"True\"/\"False\",\n"
            "        ### <string>: \"String\", <freq>: \"<f> Hz/kHz/MHz\",\n"
            "        ### <percent>: \"<f>%\"\n"
            "        ### [arg]: arg optional\n";
        ptr->dev_comment = xnfstrdup(descrip);
        if (ptr->dev_comment) {
            for (p = DevToConfig[screennum].GDev.options; p->name != NULL; p++) {
                char *p_e;
                const char *prefix = "        #Option     ";
                const char *middle = " \t# ";
                const char *suffix = "\n";
                const char *opttype = optionTypeToString(p->type);
                char *optname;
                int len = strlen(ptr->dev_comment) + strlen(prefix) +
                    strlen(middle) + strlen(suffix) + 1;

                if (asprintf(&optname, "\"%s\"", p->name) == -1)
                    break;

                len += max(20, strlen(optname));
                len += strlen(opttype);

                ptr->dev_comment = realloc(ptr->dev_comment, len);
                if (!ptr->dev_comment)
                    break;
                p_e = ptr->dev_comment + strlen(ptr->dev_comment);
                sprintf(p_e, "%s%-20s%s%s%s", prefix, optname, middle,
                        opttype, suffix);
                free(optname);
            }
        }
    }

    return ptr;
}

static XF86ConfLayoutPtr
configureLayoutSection(void)
{
    int scrnum = 0;

    parsePrologue(XF86ConfLayoutPtr, XF86ConfLayoutRec);

    ptr->lay_identifier = "X.org Configured";

    {
        XF86ConfInputrefPtr iptr;

        iptr = malloc(sizeof(XF86ConfInputrefRec));
        iptr->list.next = NULL;
        iptr->iref_option_lst = NULL;
        iptr->iref_inputdev_str = xnfstrdup("Mouse0");
        iptr->iref_option_lst =
            xf86addNewOption(iptr->iref_option_lst, xnfstrdup("CorePointer"),
                             NULL);
        ptr->lay_input_lst = (XF86ConfInputrefPtr)
            xf86addListItem((glp) ptr->lay_input_lst, (glp) iptr);
    }

    {
        XF86ConfInputrefPtr iptr;

        iptr = malloc(sizeof(XF86ConfInputrefRec));
        iptr->list.next = NULL;
        iptr->iref_option_lst = NULL;
        iptr->iref_inputdev_str = xnfstrdup("Keyboard0");
        iptr->iref_option_lst =
            xf86addNewOption(iptr->iref_option_lst, xnfstrdup("CoreKeyboard"),
                             NULL);
        ptr->lay_input_lst = (XF86ConfInputrefPtr)
            xf86addListItem((glp) ptr->lay_input_lst, (glp) iptr);
    }

    for (scrnum = 0; scrnum < nDevToConfig; scrnum++) {
        XF86ConfAdjacencyPtr aptr;
        char *tmp;

        aptr = malloc(sizeof(XF86ConfAdjacencyRec));
        aptr->list.next = NULL;
        aptr->adj_x = 0;
        aptr->adj_y = 0;
        aptr->adj_scrnum = scrnum;
        XNFasprintf(&tmp, "Screen%d", scrnum);
        aptr->adj_screen_str = tmp;
        if (scrnum == 0) {
            aptr->adj_where = CONF_ADJ_ABSOLUTE;
            aptr->adj_refscreen = NULL;
        }
        else {
            aptr->adj_where = CONF_ADJ_RIGHTOF;
            XNFasprintf(&tmp, "Screen%d", scrnum - 1);
            aptr->adj_refscreen = tmp;
        }
        ptr->lay_adjacency_lst =
            (XF86ConfAdjacencyPtr) xf86addListItem((glp) ptr->lay_adjacency_lst,
                                                   (glp) aptr);
    }

    return ptr;
}

static XF86ConfFlagsPtr
configureFlagsSection(void)
{
    parsePrologue(XF86ConfFlagsPtr, XF86ConfFlagsRec);

    return ptr;
}

static XF86ConfModulePtr
configureModuleSection(void)
{
    const char **elist, **el;

    parsePrologue(XF86ConfModulePtr, XF86ConfModuleRec);

    elist = LoaderListDir("extensions", NULL);
    if (elist) {
        for (el = elist; *el; el++) {
            XF86LoadPtr module;

            module = calloc(1, sizeof(XF86LoadRec));
            module->load_name = *el;
            ptr->mod_load_lst = (XF86LoadPtr) xf86addListItem((glp) ptr->
                                                              mod_load_lst,
                                                              (glp) module);
        }
        free(elist);
    }

    return ptr;
}

static XF86ConfFilesPtr
configureFilesSection(void)
{
    parsePrologue(XF86ConfFilesPtr, XF86ConfFilesRec);

    if (xf86ModulePath)
        ptr->file_modulepath = xnfstrdup(xf86ModulePath);
    if (defaultFontPath)
        ptr->file_fontpath = xnfstrdup(defaultFontPath);

    return ptr;
}

static XF86ConfMonitorPtr
configureMonitorSection(int screennum)
{
    char *tmp;
    parsePrologue(XF86ConfMonitorPtr, XF86ConfMonitorRec);

    XNFasprintf(&tmp, "Monitor%d", screennum);
    ptr->mon_identifier = tmp;
    ptr->mon_vendor = xnfstrdup("Monitor Vendor");
    ptr->mon_modelname = xnfstrdup("Monitor Model");

    return ptr;
}

/* Initialize Configure Monitor from Detailed Timing Block */
static void
handle_detailed_input(struct detailed_monitor_section *det_mon, void *data)
{
    XF86ConfMonitorPtr ptr = (XF86ConfMonitorPtr) data;

    switch (det_mon->type) {
    case DS_NAME:
        ptr->mon_modelname = realloc(ptr->mon_modelname,
                                     strlen((char *) (det_mon->section.name)) +
                                     1);
        strcpy(ptr->mon_modelname, (char *) (det_mon->section.name));
        break;
    case DS_RANGES:
        ptr->mon_hsync[ptr->mon_n_hsync].lo = det_mon->section.ranges.min_h;
        ptr->mon_hsync[ptr->mon_n_hsync].hi = det_mon->section.ranges.max_h;
        ptr->mon_n_vrefresh = 1;
        ptr->mon_vrefresh[ptr->mon_n_hsync].lo = det_mon->section.ranges.min_v;
        ptr->mon_vrefresh[ptr->mon_n_hsync].hi = det_mon->section.ranges.max_v;
        ptr->mon_n_hsync++;
    default:
        break;
    }
}

static XF86ConfMonitorPtr
configureDDCMonitorSection(int screennum)
{
    int len, mon_width, mon_height;

#define displaySizeMaxLen 80
    char displaySize_string[displaySizeMaxLen];
    int displaySizeLen;
    char *tmp;

    parsePrologue(XF86ConfMonitorPtr, XF86ConfMonitorRec);

    XNFasprintf(&tmp, "Monitor%d", screennum);
    ptr->mon_identifier = tmp;
    ptr->mon_vendor = xnfstrdup(ConfiguredMonitor->vendor.name);
    XNFasprintf(&ptr->mon_modelname, "%x", ConfiguredMonitor->vendor.prod_id);

    /* features in centimetres, we want millimetres */
    mon_width = 10 * ConfiguredMonitor->features.hsize;
    mon_height = 10 * ConfiguredMonitor->features.vsize;

#ifdef CONFIGURE_DISPLAYSIZE
    ptr->mon_width = mon_width;
    ptr->mon_height = mon_height;
#else
    if (mon_width && mon_height) {
        /* when values available add DisplaySize option AS A COMMENT */

        displaySizeLen = snprintf(displaySize_string, displaySizeMaxLen,
                                  "\t#DisplaySize\t%5d %5d\t# mm\n",
                                  mon_width, mon_height);

        if (displaySizeLen > 0 && displaySizeLen < displaySizeMaxLen) {
            if (ptr->mon_comment) {
                len = strlen(ptr->mon_comment);
            }
            else {
                len = 0;
            }
            if ((ptr->mon_comment =
                 realloc(ptr->mon_comment,
                         len + strlen(displaySize_string) + 1))) {
                strcpy(ptr->mon_comment + len, displaySize_string);
            }
        }
    }
#endif                          /* def CONFIGURE_DISPLAYSIZE */

    xf86ForEachDetailedBlock(ConfiguredMonitor, handle_detailed_input, ptr);

    if (ConfiguredMonitor->features.dpms) {
        ptr->mon_option_lst =
            xf86addNewOption(ptr->mon_option_lst, xnfstrdup("DPMS"), NULL);
    }

    return ptr;
}

static int
is_fallback(const char *s)
{
    /* later entries are less preferred */
    const char *fallback[5] = { "modesetting", "fbdev", "vesa",  "wsfb", NULL };
    int i;

    for (i = 0; fallback[i]; i++)
	if (strstr(s, fallback[i]))
	    return i;

    return -1;
}

static int
driver_sort(const void *_l, const void *_r)
{
    const char *l = *(const char **)_l;
    const char *r = *(const char **)_r;
    int left = is_fallback(l);
    int right = is_fallback(r);

    /* neither is a fallback, asciibetize */
    if (left == -1 && right == -1)
	return strcmp(l, r);

    /* left is a fallback, right is not */
    if (left >= 0 && right == -1)
	return 1;

    /* right is a fallback, left is not */
    if (right >= 0 && left == -1)
	return -1;

    /* both are fallbacks, decide which is worse */
    return left - right;
}

static void
fixup_video_driver_list(const char **drivers)
{
    const char **end;

    /* walk to the end of the list */
    for (end = drivers; *end && **end; end++);

    qsort(drivers, end - drivers, sizeof(const char *), driver_sort);
}

static const char **
GenerateDriverList(void)
{
    const char **ret;
    static const char *patlist[] = { "(.*)_drv\\.so", NULL };
    ret = LoaderListDir("drivers", patlist);

    /* fix up the probe order for video drivers */
    if (ret != NULL)
        fixup_video_driver_list(ret);

    return ret;
}

void
DoConfigure(void)
{
    int i, j, screennum = -1;
    const char *home = NULL;
    char filename[PATH_MAX];
    const char *addslash = "";
    XF86ConfigPtr xf86config = NULL;
    const char **vlist, **vl;
    int *dev2screen;

    vlist = GenerateDriverList();

    if (!vlist) {
        ErrorF("Missing output drivers.  Configuration failed.\n");
        goto bail;
    }

    ErrorF("List of video drivers:\n");
    for (vl = vlist; *vl; vl++)
        ErrorF("\t%s\n", *vl);

    /* Load all the drivers that were found. */
    xf86LoadModules(vlist, NULL);

    free(vlist);

    xorgHWAccess = xf86EnableIO();

    /* Create XF86Config file structure */
    xf86config = calloc(1, sizeof(XF86ConfigRec));

    /* Call all of the probe functions, reporting the results. */
    for (CurrentDriver = 0; CurrentDriver < xf86NumDrivers; CurrentDriver++) {
        Bool found_screen;
        DriverRec *const drv = xf86DriverList[CurrentDriver];

        found_screen = xf86CallDriverProbe(drv, TRUE);
        if (found_screen && drv->Identify) {
            (*drv->Identify) (0);
        }
    }

    if (nDevToConfig <= 0) {
        ErrorF("No devices to configure.  Configuration failed.\n");
        goto bail;
    }

    /* Add device, monitor and screen sections for detected devices */
    for (screennum = 0; screennum < nDevToConfig; screennum++) {
        XF86ConfDevicePtr device_ptr;
        XF86ConfMonitorPtr monitor_ptr;
        XF86ConfScreenPtr screen_ptr;

        device_ptr = configureDeviceSection(screennum);
        xf86config->conf_device_lst = (XF86ConfDevicePtr) xf86addListItem((glp)
                                                                          xf86config->
                                                                          conf_device_lst,
                                                                          (glp)
                                                                          device_ptr);
        monitor_ptr = configureMonitorSection(screennum);
        xf86config->conf_monitor_lst = (XF86ConfMonitorPtr) xf86addListItem((glp) xf86config->conf_monitor_lst, (glp) monitor_ptr);
        screen_ptr = configureScreenSection(screennum);
        xf86config->conf_screen_lst = (XF86ConfScreenPtr) xf86addListItem((glp)
                                                                          xf86config->
                                                                          conf_screen_lst,
                                                                          (glp)
                                                                          screen_ptr);
    }

    xf86config->conf_files = configureFilesSection();
    xf86config->conf_modules = configureModuleSection();
    xf86config->conf_flags = configureFlagsSection();
    xf86config->conf_videoadaptor_lst = NULL;
    xf86config->conf_modes_lst = NULL;
    xf86config->conf_vendor_lst = NULL;
    xf86config->conf_dri = NULL;
    xf86config->conf_input_lst = configureInputSection();
    xf86config->conf_layout_lst = configureLayoutSection();

    home = getenv("HOME");
    if ((home == NULL) || (home[0] == '\0')) {
        home = "/";
    }
    else {
        /* Determine if trailing slash is present or needed */
        int l = strlen(home);

        if (home[l - 1] != '/') {
            addslash = "/";
        }
    }

    snprintf(filename, sizeof(filename), "%s%s" XF86CONFIGFILE ".new",
             home, addslash);

    if (xf86writeConfigFile(filename, xf86config) == 0) {
        xf86Msg(X_ERROR, "Unable to write config file: \"%s\": %s\n",
                filename, strerror(errno));
        goto bail;
    }

    xf86DoConfigurePass1 = FALSE;
    /* Try to get DDC information filled in */
    xf86ConfigFile = filename;
    if (xf86HandleConfigFile(FALSE) != CONFIG_OK) {
        goto bail;
    }

    xf86DoConfigurePass1 = FALSE;

    dev2screen = xnfcalloc(nDevToConfig, sizeof(int));

    {
        Bool *driverProbed = xnfcalloc(xf86NumDrivers, sizeof(Bool));

        for (screennum = 0; screennum < nDevToConfig; screennum++) {
            int k, l, n, oldNumScreens;

            i = DevToConfig[screennum].iDriver;

            if (driverProbed[i])
                continue;
            driverProbed[i] = TRUE;

            oldNumScreens = xf86NumScreens;

            xf86CallDriverProbe(xf86DriverList[i], FALSE);

            /* reorder */
            k = screennum > 0 ? screennum : 1;
            for (l = oldNumScreens; l < xf86NumScreens; l++) {
                /* is screen primary? */
                Bool primary = FALSE;

                for (n = 0; n < xf86Screens[l]->numEntities; n++) {
                    if (xf86IsEntityPrimary(xf86Screens[l]->entityList[n])) {
                        dev2screen[0] = l;
                        primary = TRUE;
                        break;
                    }
                }
                if (primary)
                    continue;
                /* not primary: assign it to next device of same driver */
                /*
                 * NOTE: we assume that devices in DevToConfig
                 * and xf86Screens[] have the same order except
                 * for the primary device which always comes first.
                 */
                for (; k < nDevToConfig; k++) {
                    if (DevToConfig[k].iDriver == i) {
                        dev2screen[k++] = l;
                        break;
                    }
                }
            }
        }
        free(driverProbed);
    }

    if (nDevToConfig != xf86NumScreens) {
        ErrorF("Number of created screens does not match number of detected"
               " devices.\n  Configuration failed.\n");
        goto bail;
    }

    xf86PostProbe();

    for (j = 0; j < xf86NumScreens; j++) {
        xf86Screens[j]->scrnIndex = j;
    }

    xf86freeMonitorList(xf86config->conf_monitor_lst);
    xf86config->conf_monitor_lst = NULL;
    xf86freeScreenList(xf86config->conf_screen_lst);
    xf86config->conf_screen_lst = NULL;
    for (j = 0; j < xf86NumScreens; j++) {
        XF86ConfMonitorPtr monitor_ptr;
        XF86ConfScreenPtr screen_ptr;

        ConfiguredMonitor = NULL;

        if ((*xf86Screens[dev2screen[j]]->PreInit) &&
            (*xf86Screens[dev2screen[j]]->PreInit) (xf86Screens[dev2screen[j]],
                                                    PROBE_DETECT) &&
            ConfiguredMonitor) {
            monitor_ptr = configureDDCMonitorSection(j);
        }
        else {
            monitor_ptr = configureMonitorSection(j);
        }
        screen_ptr = configureScreenSection(j);

        xf86config->conf_monitor_lst = (XF86ConfMonitorPtr) xf86addListItem((glp) xf86config->conf_monitor_lst, (glp) monitor_ptr);
        xf86config->conf_screen_lst = (XF86ConfScreenPtr) xf86addListItem((glp)
                                                                          xf86config->
                                                                          conf_screen_lst,
                                                                          (glp)
                                                                          screen_ptr);
    }

    if (xf86writeConfigFile(filename, xf86config) == 0) {
        xf86Msg(X_ERROR, "Unable to write config file: \"%s\": %s\n",
                filename, strerror(errno));
        goto bail;
    }

    ErrorF("\n");

    if (!foundMouse) {
        ErrorF("\n" __XSERVERNAME__ " is not able to detect your mouse.\n"
               "Edit the file and correct the Device.\n");
    }
    else {
        ErrorF("\n" __XSERVERNAME__ " detected your mouse at device %s.\n"
               "Please check your config if the mouse is still not\n"
               "operational, as by default " __XSERVERNAME__
               " tries to autodetect\n" "the protocol.\n", DFLT_MOUSE_DEV);
    }

    if (xf86NumScreens > 1) {
        ErrorF("\n" __XSERVERNAME__
               " has configured a multihead system, please check your config.\n");
    }

    ErrorF("\nYour %s file is %s\n\n", XF86CONFIGFILE, filename);
    ErrorF("To test the server, run 'X -config %s'\n\n", filename);

 bail:
    OsCleanup(TRUE);
    ddxGiveUp(EXIT_ERR_CONFIGURE);
    fflush(stderr);
    exit(0);
}

/* Xorg -showopts:
 *   For each driver module installed, print out the list
 *   of options and their argument types, then exit
 *
 * Author:  Marcus Schaefer, ms@suse.de
 */

void
DoShowOptions(void)
{
    int i = 0;
    const char **vlist = NULL;
    char *pSymbol = 0;
    XF86ModuleData *initData = 0;

    if (!(vlist = GenerateDriverList())) {
        ErrorF("Missing output drivers\n");
        goto bail;
    }
    xf86LoadModules(vlist, 0);
    free(vlist);
    for (i = 0; i < xf86NumDrivers; i++) {
        if (xf86DriverList[i]->AvailableOptions) {
            const OptionInfoRec *pOption =
                (*xf86DriverList[i]->AvailableOptions) (0, 0);
            if (!pOption) {
                ErrorF("(EE) Couldn't read option table for %s driver\n",
                       xf86DriverList[i]->driverName);
                continue;
            }
            XNFasprintf(&pSymbol, "%sModuleData",
                        xf86DriverList[i]->driverName);
            initData = LoaderSymbol(pSymbol);
            if (initData) {
                XF86ModuleVersionInfo *vers = initData->vers;
                const OptionInfoRec *p;

                ErrorF("Driver[%d]:%s[%s] {\n",
                       i, xf86DriverList[i]->driverName, vers->vendor);
                for (p = pOption; p->name != NULL; p++) {
                    ErrorF("\t%s:%s\n", p->name, optionTypeToString(p->type));
                }
                ErrorF("}\n");
            }
        }
    }
 bail:
    OsCleanup(TRUE);
    ddxGiveUp(EXIT_ERR_DRIVERS);
    fflush(stderr);
    exit(0);
}
