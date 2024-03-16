/*
 *
 * Copyright (c) 1997  Metro Link Incorporated
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
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 *
 */
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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
 * This file contains the external interfaces for the XFree86 configuration
 * file parser.
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _xf86Parser_h_
#define _xf86Parser_h_

#include <X11/Xdefs.h>
#include "xf86Optrec.h"
#include "list.h"

#define HAVE_PARSER_DECLS

typedef struct {
    char *file_logfile;
    char *file_modulepath;
    char *file_fontpath;
    char *file_comment;
    char *file_xkbdir;
} XF86ConfFilesRec, *XF86ConfFilesPtr;

/* Values for load_type */
#define XF86_LOAD_MODULE	0
#define XF86_LOAD_DRIVER	1
#define XF86_DISABLE_MODULE	2

typedef struct {
    GenericListRec list;
    int load_type;
    const char *load_name;
    XF86OptionPtr load_opt;
    char *load_comment;
    int ignore;
} XF86LoadRec, *XF86LoadPtr;

typedef struct {
    XF86LoadPtr mod_load_lst;
    XF86LoadPtr mod_disable_lst;
    char *mod_comment;
} XF86ConfModuleRec, *XF86ConfModulePtr;

#define CONF_IMPLICIT_KEYBOARD	"Implicit Core Keyboard"

#define CONF_IMPLICIT_POINTER	"Implicit Core Pointer"

#define XF86CONF_PHSYNC    0x0001
#define XF86CONF_NHSYNC    0x0002
#define XF86CONF_PVSYNC    0x0004
#define XF86CONF_NVSYNC    0x0008
#define XF86CONF_INTERLACE 0x0010
#define XF86CONF_DBLSCAN   0x0020
#define XF86CONF_CSYNC     0x0040
#define XF86CONF_PCSYNC    0x0080
#define XF86CONF_NCSYNC    0x0100
#define XF86CONF_HSKEW     0x0200       /* hskew provided */
#define XF86CONF_BCAST     0x0400
#define XF86CONF_VSCAN     0x1000

typedef struct {
    GenericListRec list;
    const char *ml_identifier;
    int ml_clock;
    int ml_hdisplay;
    int ml_hsyncstart;
    int ml_hsyncend;
    int ml_htotal;
    int ml_vdisplay;
    int ml_vsyncstart;
    int ml_vsyncend;
    int ml_vtotal;
    int ml_vscan;
    int ml_flags;
    int ml_hskew;
    char *ml_comment;
} XF86ConfModeLineRec, *XF86ConfModeLinePtr;

typedef struct {
    GenericListRec list;
    const char *vp_identifier;
    XF86OptionPtr vp_option_lst;
    char *vp_comment;
} XF86ConfVideoPortRec, *XF86ConfVideoPortPtr;

typedef struct {
    GenericListRec list;
    const char *va_identifier;
    const char *va_vendor;
    const char *va_board;
    const char *va_busid;
    const char *va_driver;
    XF86OptionPtr va_option_lst;
    XF86ConfVideoPortPtr va_port_lst;
    const char *va_fwdref;
    char *va_comment;
} XF86ConfVideoAdaptorRec, *XF86ConfVideoAdaptorPtr;

#define CONF_MAX_HSYNC 8
#define CONF_MAX_VREFRESH 8

typedef struct {
    float hi, lo;
} parser_range;

typedef struct {
    int red, green, blue;
} parser_rgb;

typedef struct {
    GenericListRec list;
    const char *modes_identifier;
    XF86ConfModeLinePtr mon_modeline_lst;
    char *modes_comment;
} XF86ConfModesRec, *XF86ConfModesPtr;

typedef struct {
    GenericListRec list;
    const char *ml_modes_str;
    XF86ConfModesPtr ml_modes;
} XF86ConfModesLinkRec, *XF86ConfModesLinkPtr;

typedef struct {
    GenericListRec list;
    const char *mon_identifier;
    const char *mon_vendor;
    char *mon_modelname;
    int mon_width;              /* in mm */
    int mon_height;             /* in mm */
    XF86ConfModeLinePtr mon_modeline_lst;
    int mon_n_hsync;
    parser_range mon_hsync[CONF_MAX_HSYNC];
    int mon_n_vrefresh;
    parser_range mon_vrefresh[CONF_MAX_VREFRESH];
    float mon_gamma_red;
    float mon_gamma_green;
    float mon_gamma_blue;
    XF86OptionPtr mon_option_lst;
    XF86ConfModesLinkPtr mon_modes_sect_lst;
    char *mon_comment;
} XF86ConfMonitorRec, *XF86ConfMonitorPtr;

#define CONF_MAXDACSPEEDS 4
#define CONF_MAXCLOCKS    128

typedef struct {
    GenericListRec list;
    const char *dev_identifier;
    const char *dev_vendor;
    const char *dev_board;
    const char *dev_chipset;
    const char *dev_busid;
    const char *dev_card;
    const char *dev_driver;
    const char *dev_ramdac;
    int dev_dacSpeeds[CONF_MAXDACSPEEDS];
    int dev_videoram;
    unsigned long dev_mem_base;
    unsigned long dev_io_base;
    const char *dev_clockchip;
    int dev_clocks;
    int dev_clock[CONF_MAXCLOCKS];
    int dev_chipid;
    int dev_chiprev;
    int dev_irq;
    int dev_screen;
    XF86OptionPtr dev_option_lst;
    char *dev_comment;
    char *match_seat;
} XF86ConfDeviceRec, *XF86ConfDevicePtr;

typedef struct {
    GenericListRec list;
    const char *mode_name;
} XF86ModeRec, *XF86ModePtr;

typedef struct {
    GenericListRec list;
    int disp_frameX0;
    int disp_frameY0;
    int disp_virtualX;
    int disp_virtualY;
    int disp_depth;
    int disp_bpp;
    const char *disp_visual;
    parser_rgb disp_weight;
    parser_rgb disp_black;
    parser_rgb disp_white;
    XF86ModePtr disp_mode_lst;
    XF86OptionPtr disp_option_lst;
    char *disp_comment;
} XF86ConfDisplayRec, *XF86ConfDisplayPtr;

typedef struct {
    XF86OptionPtr flg_option_lst;
    char *flg_comment;
} XF86ConfFlagsRec, *XF86ConfFlagsPtr;

typedef struct {
    GenericListRec list;
    const char *al_adaptor_str;
    XF86ConfVideoAdaptorPtr al_adaptor;
} XF86ConfAdaptorLinkRec, *XF86ConfAdaptorLinkPtr;

#define CONF_MAXGPUDEVICES 4
typedef struct {
    GenericListRec list;
    const char *scrn_identifier;
    const char *scrn_obso_driver;
    int scrn_defaultdepth;
    int scrn_defaultbpp;
    int scrn_defaultfbbpp;
    const char *scrn_monitor_str;
    XF86ConfMonitorPtr scrn_monitor;
    const char *scrn_device_str;
    XF86ConfDevicePtr scrn_device;
    XF86ConfAdaptorLinkPtr scrn_adaptor_lst;
    XF86ConfDisplayPtr scrn_display_lst;
    XF86OptionPtr scrn_option_lst;
    char *scrn_comment;
    int scrn_virtualX, scrn_virtualY;
    char *match_seat;

    int num_gpu_devices;
    const char *scrn_gpu_device_str[CONF_MAXGPUDEVICES];
    XF86ConfDevicePtr scrn_gpu_devices[CONF_MAXGPUDEVICES];
} XF86ConfScreenRec, *XF86ConfScreenPtr;

typedef struct {
    GenericListRec list;
    char *inp_identifier;
    char *inp_driver;
    XF86OptionPtr inp_option_lst;
    char *inp_comment;
} XF86ConfInputRec, *XF86ConfInputPtr;

typedef struct {
    GenericListRec list;
    XF86ConfInputPtr iref_inputdev;
    char *iref_inputdev_str;
    XF86OptionPtr iref_option_lst;
} XF86ConfInputrefRec, *XF86ConfInputrefPtr;

typedef struct {
    Bool set;
    Bool val;
} xf86TriState;

typedef struct {
    struct xorg_list entry;
    char **values;
    Bool is_negated;
} xf86MatchGroup;

typedef struct {
    GenericListRec list;
    char *identifier;
    char *driver;
    struct xorg_list match_product;
    struct xorg_list match_vendor;
    struct xorg_list match_device;
    struct xorg_list match_os;
    struct xorg_list match_pnpid;
    struct xorg_list match_usbid;
    struct xorg_list match_driver;
    struct xorg_list match_tag;
    struct xorg_list match_layout;
    xf86TriState is_keyboard;
    xf86TriState is_pointer;
    xf86TriState is_joystick;
    xf86TriState is_tablet;
    xf86TriState is_tablet_pad;
    xf86TriState is_touchpad;
    xf86TriState is_touchscreen;
    XF86OptionPtr option_lst;
    char *comment;
} XF86ConfInputClassRec, *XF86ConfInputClassPtr;

typedef struct {
    GenericListRec list;
    char *identifier;
    char *driver;
    char *modulepath;
    struct xorg_list match_driver;
    XF86OptionPtr option_lst;
    char *comment;
} XF86ConfOutputClassRec, *XF86ConfOutputClassPtr;

/* Values for adj_where */
#define CONF_ADJ_OBSOLETE	-1
#define CONF_ADJ_ABSOLUTE	0
#define CONF_ADJ_RIGHTOF	1
#define CONF_ADJ_LEFTOF		2
#define CONF_ADJ_ABOVE		3
#define CONF_ADJ_BELOW		4
#define CONF_ADJ_RELATIVE	5

typedef struct {
    GenericListRec list;
    int adj_scrnum;
    XF86ConfScreenPtr adj_screen;
    const char *adj_screen_str;
    XF86ConfScreenPtr adj_top;
    const char *adj_top_str;
    XF86ConfScreenPtr adj_bottom;
    const char *adj_bottom_str;
    XF86ConfScreenPtr adj_left;
    const char *adj_left_str;
    XF86ConfScreenPtr adj_right;
    const char *adj_right_str;
    int adj_where;
    int adj_x;
    int adj_y;
    const char *adj_refscreen;
} XF86ConfAdjacencyRec, *XF86ConfAdjacencyPtr;

typedef struct {
    GenericListRec list;
    const char *inactive_device_str;
    XF86ConfDevicePtr inactive_device;
} XF86ConfInactiveRec, *XF86ConfInactivePtr;

typedef struct {
    GenericListRec list;
    const char *lay_identifier;
    XF86ConfAdjacencyPtr lay_adjacency_lst;
    XF86ConfInactivePtr lay_inactive_lst;
    XF86ConfInputrefPtr lay_input_lst;
    XF86OptionPtr lay_option_lst;
    char *match_seat;
    char *lay_comment;
} XF86ConfLayoutRec, *XF86ConfLayoutPtr;

typedef struct {
    GenericListRec list;
    const char *vs_name;
    const char *vs_identifier;
    XF86OptionPtr vs_option_lst;
    char *vs_comment;
} XF86ConfVendSubRec, *XF86ConfVendSubPtr;

typedef struct {
    GenericListRec list;
    const char *vnd_identifier;
    XF86OptionPtr vnd_option_lst;
    XF86ConfVendSubPtr vnd_sub_lst;
    char *vnd_comment;
} XF86ConfVendorRec, *XF86ConfVendorPtr;

typedef struct {
    const char *dri_group_name;
    int dri_group;
    int dri_mode;
    char *dri_comment;
} XF86ConfDRIRec, *XF86ConfDRIPtr;

typedef struct {
    XF86OptionPtr ext_option_lst;
    char *extensions_comment;
} XF86ConfExtensionsRec, *XF86ConfExtensionsPtr;

typedef struct {
    XF86ConfFilesPtr conf_files;
    XF86ConfModulePtr conf_modules;
    XF86ConfFlagsPtr conf_flags;
    XF86ConfVideoAdaptorPtr conf_videoadaptor_lst;
    XF86ConfModesPtr conf_modes_lst;
    XF86ConfMonitorPtr conf_monitor_lst;
    XF86ConfDevicePtr conf_device_lst;
    XF86ConfScreenPtr conf_screen_lst;
    XF86ConfInputPtr conf_input_lst;
    XF86ConfInputClassPtr conf_inputclass_lst;
    XF86ConfOutputClassPtr conf_outputclass_lst;
    XF86ConfLayoutPtr conf_layout_lst;
    XF86ConfVendorPtr conf_vendor_lst;
    XF86ConfDRIPtr conf_dri;
    XF86ConfExtensionsPtr conf_extensions;
    char *conf_comment;
} XF86ConfigRec, *XF86ConfigPtr;

typedef struct {
    int token;                  /* id of the token */
    const char *name;           /* pointer to the LOWERCASED name */
} xf86ConfigSymTabRec, *xf86ConfigSymTabPtr;

/*
 * prototypes for public functions
 */
extern void xf86initConfigFiles(void);
extern char *xf86openConfigFile(const char *path, const char *cmdline,
                                const char *projroot);
extern char *xf86openConfigDirFiles(const char *path, const char *cmdline,
                                    const char *projroot);
extern void xf86setBuiltinConfig(const char *config[]);
extern XF86ConfigPtr xf86readConfigFile(void);
extern void xf86closeConfigFile(void);
extern XF86ConfigPtr xf86allocateConfig(void);
extern void xf86freeConfig(XF86ConfigPtr p);
extern int xf86writeConfigFile(const char *, XF86ConfigPtr);
extern _X_EXPORT XF86ConfDevicePtr xf86findDevice(const char *ident,
                                                  XF86ConfDevicePtr p);
extern _X_EXPORT XF86ConfLayoutPtr xf86findLayout(const char *name,
                                                  XF86ConfLayoutPtr list);
extern _X_EXPORT XF86ConfMonitorPtr xf86findMonitor(const char *ident,
                                                    XF86ConfMonitorPtr p);
extern _X_EXPORT XF86ConfModesPtr xf86findModes(const char *ident,
                                                XF86ConfModesPtr p);
extern _X_EXPORT XF86ConfModeLinePtr xf86findModeLine(const char *ident,
                                                      XF86ConfModeLinePtr p);
extern _X_EXPORT XF86ConfScreenPtr xf86findScreen(const char *ident,
                                                  XF86ConfScreenPtr p);
extern _X_EXPORT XF86ConfInputPtr xf86findInput(const char *ident,
                                                XF86ConfInputPtr p);
extern _X_EXPORT XF86ConfInputPtr xf86findInputByDriver(const char *driver,
                                                        XF86ConfInputPtr p);
extern _X_EXPORT XF86ConfVideoAdaptorPtr xf86findVideoAdaptor(const char *ident,
                                                              XF86ConfVideoAdaptorPtr
                                                              p);
extern int xf86layoutAddInputDevices(XF86ConfigPtr config,
                                     XF86ConfLayoutPtr layout);

extern _X_EXPORT GenericListPtr xf86addListItem(GenericListPtr head,
                                                GenericListPtr c_new);
extern _X_EXPORT int xf86itemNotSublist(GenericListPtr list_1,
                                        GenericListPtr list_2);

extern _X_EXPORT int xf86pathIsAbsolute(const char *path);
extern _X_EXPORT int xf86pathIsSafe(const char *path);
extern _X_EXPORT char *xf86addComment(char *cur, const char *add);
extern _X_EXPORT Bool xf86getBoolValue(Bool *val, const char *str);

#endif                          /* _xf86Parser_h_ */
