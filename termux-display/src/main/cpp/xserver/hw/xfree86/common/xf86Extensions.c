/*
 * Copyright © 2011 Daniel Stone
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "extension.h"
#include "extinit.h"
#include "globals.h"

#include "xf86.h"
#include "xf86Config.h"
#include "xf86Module.h"
#include "xf86Extensions.h"
#include "xf86Opt.h"
#include "optionstr.h"

#ifdef XSELINUX
#include "xselinux.h"
#endif

#ifdef XFreeXDGA
#include <X11/extensions/xf86dgaproto.h>
#endif

#ifdef XF86VIDMODE
#include <X11/extensions/xf86vmproto.h>
#include "vidmodestr.h"
#endif

/*
 * DDX-specific extensions.
 */
static const ExtensionModule extensionModules[] = {
#ifdef XF86VIDMODE
    {
	XFree86VidModeExtensionInit,
	XF86VIDMODENAME,
	&noXFree86VidModeExtension
    },
#endif
#ifdef XFreeXDGA
    {
	XFree86DGAExtensionInit,
	XF86DGANAME,
	&noXFree86DGAExtension
    },
#endif
#ifdef XF86DRI
    {
        XFree86DRIExtensionInit,
        "XFree86-DRI",
        &noXFree86DRIExtension
    },
#endif
#ifdef DRI2
    {
        DRI2ExtensionInit,
        DRI2_NAME,
        &noDRI2Extension
    }
#endif
};

static void
load_extension_config(void)
{
    XF86ConfModulePtr mod_con = xf86configptr->conf_modules;
    XF86LoadPtr modp;

    /* Only the best. */
    if (!mod_con)
        return;

    nt_list_for_each_entry(modp, mod_con->mod_load_lst, list.next) {
        InputOption *opt;

        if (strcasecmp(modp->load_name, "extmod") != 0)
            continue;

        /* extmod options are of the form "omit <extension-name>" */
        nt_list_for_each_entry(opt, modp->load_opt, list.next) {
            const char *key = input_option_get_key(opt);
            if (strncasecmp(key, "omit", 4) != 0 || strlen(key) < 5)
                continue;
            if (EnableDisableExtension(key + 4, FALSE))
                xf86MarkOptionUsed(opt);
        }

#ifdef XSELINUX
        if ((opt = xf86FindOption(modp->load_opt,
                                  "SELinux mode disabled"))) {
            xf86MarkOptionUsed(opt);
            selinuxEnforcingState = SELINUX_MODE_DISABLED;
        }
        if ((opt = xf86FindOption(modp->load_opt,
                                  "SELinux mode permissive"))) {
            xf86MarkOptionUsed(opt);
            selinuxEnforcingState = SELINUX_MODE_PERMISSIVE;
        }
        if ((opt = xf86FindOption(modp->load_opt,
                                  "SELinux mode enforcing"))) {
            xf86MarkOptionUsed(opt);
            selinuxEnforcingState = SELINUX_MODE_ENFORCING;
        }
#endif
    }
}

void
xf86ExtensionInit(void)
{
    load_extension_config();

    LoadExtensionList(extensionModules, ARRAY_SIZE(extensionModules), TRUE);
}
