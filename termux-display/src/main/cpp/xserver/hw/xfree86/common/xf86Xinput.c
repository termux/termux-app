/*
 * Copyright 1995-1999 by Frederic Lepied, France. <Lepied@XFree86.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Frederic   Lepied not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Frederic  Lepied   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * FREDERIC  LEPIED DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL FREDERIC  LEPIED BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*
 * Copyright (c) 2000-2002 by The XFree86 Project, Inc.
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
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include <X11/Xatom.h>
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Config.h"
#include "xf86Xinput.h"
#include "xf86Optrec.h"
#include "mipointer.h"
#include "extinit.h"
#include "loaderProcs.h"
#include "systemd-logind.h"

#include "exevents.h"           /* AddInputDevice */
#include "exglobals.h"
#include "eventstr.h"
#include "inpututils.h"
#include "optionstr.h"

#include <string.h>             /* InputClassMatches */
#ifdef HAVE_FNMATCH_H
#include <fnmatch.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#include <stdarg.h>
#include <stdint.h>             /* for int64_t */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif
#ifdef HAVE_SYS_MKDEV_H
#include <sys/mkdev.h>          /* for major() & minor() on Solaris */
#endif

#include "mi.h"

#include <ptrveloc.h>           /* dix pointer acceleration */
#include <xserver-properties.h>

#ifdef XFreeXDGA
#include "dgaproc.h"
#endif

#include "xkbsrv.h"

/* Valuator verification macro */
#define XI_VERIFY_VALUATORS(num_valuators)					\
	if (num_valuators > MAX_VALUATORS) {					\
		xf86Msg(X_ERROR, "%s: num_valuator %d is greater than"		\
			" MAX_VALUATORS\n", __FUNCTION__, num_valuators);	\
		return;								\
	}

static int
 xf86InputDevicePostInit(DeviceIntPtr dev);

typedef struct {
    struct xorg_list node;
    InputInfoPtr pInfo;
} PausedInputDeviceRec;
typedef PausedInputDeviceRec *PausedInputDevicePtr;

static struct xorg_list new_input_devices_list = {
    .next = &new_input_devices_list,
    .prev = &new_input_devices_list,
};

/**
 * Eval config and modify DeviceVelocityRec accordingly
 */
static void
ProcessVelocityConfiguration(DeviceIntPtr pDev, const char *devname, void *list,
                             DeviceVelocityPtr s)
{
    int tempi;
    float tempf;
    Atom float_prop = XIGetKnownProperty(XATOM_FLOAT);
    Atom prop;

    if (!s)
        return;

    /* common settings (available via device properties) */
    tempf = xf86SetRealOption(list, "ConstantDeceleration", 1.0);
    if (tempf != 1.0) {
        xf86Msg(X_CONFIG, "%s: (accel) constant deceleration by %.1f\n",
                devname, tempf);
        prop = XIGetKnownProperty(ACCEL_PROP_CONSTANT_DECELERATION);
        XIChangeDeviceProperty(pDev, prop, float_prop, 32,
                               PropModeReplace, 1, &tempf, FALSE);
    }

    tempf = xf86SetRealOption(list, "AdaptiveDeceleration", 1.0);
    if (tempf > 1.0) {
        xf86Msg(X_CONFIG, "%s: (accel) adaptive deceleration by %.1f\n",
                devname, tempf);
        prop = XIGetKnownProperty(ACCEL_PROP_ADAPTIVE_DECELERATION);
        XIChangeDeviceProperty(pDev, prop, float_prop, 32,
                               PropModeReplace, 1, &tempf, FALSE);
    }

    /* select profile by number */
    tempi = xf86SetIntOption(list, "AccelerationProfile",
                             s->statistics.profile_number);

    prop = XIGetKnownProperty(ACCEL_PROP_PROFILE_NUMBER);
    if (XIChangeDeviceProperty(pDev, prop, XA_INTEGER, 32,
                               PropModeReplace, 1, &tempi, FALSE) == Success) {
        xf86Msg(X_CONFIG, "%s: (accel) acceleration profile %i\n", devname,
                tempi);
    }
    else {
        xf86Msg(X_CONFIG, "%s: (accel) acceleration profile %i is unknown\n",
                devname, tempi);
    }

    /* set scaling */
    tempf = xf86SetRealOption(list, "ExpectedRate", 0);
    prop = XIGetKnownProperty(ACCEL_PROP_VELOCITY_SCALING);
    if (tempf > 0) {
        tempf = 1000.0 / tempf;
        XIChangeDeviceProperty(pDev, prop, float_prop, 32,
                               PropModeReplace, 1, &tempf, FALSE);
    }
    else {
        tempf = xf86SetRealOption(list, "VelocityScale", s->corr_mul);
        XIChangeDeviceProperty(pDev, prop, float_prop, 32,
                               PropModeReplace, 1, &tempf, FALSE);
    }

    tempi = xf86SetIntOption(list, "VelocityTrackerCount", -1);
    if (tempi > 1)
        InitTrackers(s, tempi);

    s->initial_range = xf86SetIntOption(list, "VelocityInitialRange",
                                        s->initial_range);

    s->max_diff = xf86SetRealOption(list, "VelocityAbsDiff", s->max_diff);

    tempf = xf86SetRealOption(list, "VelocityRelDiff", -1);
    if (tempf >= 0) {
        xf86Msg(X_CONFIG, "%s: (accel) max rel. velocity difference: %.1f%%\n",
                devname, tempf * 100.0);
        s->max_rel_diff = tempf;
    }

    /*  Configure softening. If const deceleration is used, this is expected
     *  to provide better subpixel information so we enable
     *  softening by default only if ConstantDeceleration is not used
     */
    s->use_softening = xf86SetBoolOption(list, "Softening",
                                         s->const_acceleration == 1.0);

    s->average_accel = xf86SetBoolOption(list, "AccelerationProfileAveraging",
                                         s->average_accel);

    s->reset_time = xf86SetIntOption(list, "VelocityReset", s->reset_time);
}

static void
ApplyAccelerationSettings(DeviceIntPtr dev)
{
    int scheme, i;
    DeviceVelocityPtr pVel;
    InputInfoPtr pInfo = (InputInfoPtr) dev->public.devicePrivate;
    char *schemeStr;

    if (dev->valuator && dev->ptrfeed) {
        schemeStr = xf86SetStrOption(pInfo->options, "AccelerationScheme", "");

        scheme = dev->valuator->accelScheme.number;

        if (!xf86NameCmp(schemeStr, "predictable"))
            scheme = PtrAccelPredictable;

        if (!xf86NameCmp(schemeStr, "lightweight"))
            scheme = PtrAccelLightweight;

        if (!xf86NameCmp(schemeStr, "none"))
            scheme = PtrAccelNoOp;

        /* reinit scheme if needed */
        if (dev->valuator->accelScheme.number != scheme) {
            if (dev->valuator->accelScheme.AccelCleanupProc) {
                dev->valuator->accelScheme.AccelCleanupProc(dev);
            }

            if (InitPointerAccelerationScheme(dev, scheme)) {
                xf86Msg(X_CONFIG, "%s: (accel) selected scheme %s/%i\n",
                        pInfo->name, schemeStr, scheme);
            }
            else {
                xf86Msg(X_CONFIG, "%s: (accel) could not init scheme %s\n",
                        pInfo->name, schemeStr);
                scheme = dev->valuator->accelScheme.number;
            }
        }
        else {
            xf86Msg(X_CONFIG, "%s: (accel) keeping acceleration scheme %i\n",
                    pInfo->name, scheme);
        }

        free(schemeStr);

        /* process special configuration */
        switch (scheme) {
        case PtrAccelPredictable:
            pVel = GetDevicePredictableAccelData(dev);
            ProcessVelocityConfiguration(dev, pInfo->name, pInfo->options,
                                         pVel);
            break;
        }

        i = xf86SetIntOption(pInfo->options, "AccelerationNumerator",
                             dev->ptrfeed->ctrl.num);
        if (i >= 0)
            dev->ptrfeed->ctrl.num = i;

        i = xf86SetIntOption(pInfo->options, "AccelerationDenominator",
                             dev->ptrfeed->ctrl.den);
        if (i > 0)
            dev->ptrfeed->ctrl.den = i;

        i = xf86SetIntOption(pInfo->options, "AccelerationThreshold",
                             dev->ptrfeed->ctrl.threshold);
        if (i >= 0)
            dev->ptrfeed->ctrl.threshold = i;

        xf86Msg(X_CONFIG, "%s: (accel) acceleration factor: %.3f\n",
                pInfo->name, ((float) dev->ptrfeed->ctrl.num) /
                ((float) dev->ptrfeed->ctrl.den));
        xf86Msg(X_CONFIG, "%s: (accel) acceleration threshold: %i\n",
                pInfo->name, dev->ptrfeed->ctrl.threshold);
    }
}

static void
ApplyTransformationMatrix(DeviceIntPtr dev)
{
    InputInfoPtr pInfo = (InputInfoPtr) dev->public.devicePrivate;
    char *str;
    int rc;
    float matrix[9] = { 0 };

    if (!dev->valuator)
        return;

    str = xf86SetStrOption(pInfo->options, "TransformationMatrix", NULL);
    if (!str)
        return;

    rc = sscanf(str, "%f %f %f %f %f %f %f %f %f", &matrix[0], &matrix[1],
                &matrix[2], &matrix[3], &matrix[4], &matrix[5], &matrix[6],
                &matrix[7], &matrix[8]);
    if (rc != 9) {
        xf86Msg(X_ERROR,
                "%s: invalid format for transformation matrix. Ignoring configuration.\n",
                pInfo->name);
        return;
    }

    XIChangeDeviceProperty(dev, XIGetKnownProperty(XI_PROP_TRANSFORM),
                           XIGetKnownProperty(XATOM_FLOAT), 32,
                           PropModeReplace, 9, matrix, FALSE);
}

static void
ApplyAutoRepeat(DeviceIntPtr dev)
{
    InputInfoPtr pInfo = (InputInfoPtr) dev->public.devicePrivate;
    XkbSrvInfoPtr xkbi;
    char *repeatStr;
    long delay, rate;

    if (!dev->key)
        return;

    xkbi = dev->key->xkbInfo;

    repeatStr = xf86SetStrOption(pInfo->options, "AutoRepeat", NULL);
    if (!repeatStr)
        return;

    if (sscanf(repeatStr, "%ld %ld", &delay, &rate) != 2) {
        xf86Msg(X_ERROR, "\"%s\" is not a valid AutoRepeat value\n", repeatStr);
        return;
    }

    xf86Msg(X_CONFIG, "AutoRepeat: %ld %ld\n", delay, rate);
    xkbi->desc->ctrls->repeat_delay = delay;
    xkbi->desc->ctrls->repeat_interval = rate;
}

/***********************************************************************
 *
 * xf86ProcessCommonOptions --
 *
 *	Process global options.
 *
 ***********************************************************************
 */
void
xf86ProcessCommonOptions(InputInfoPtr pInfo, XF86OptionPtr list)
{
    if (xf86SetBoolOption(list, "Floating", 0) ||
        !xf86SetBoolOption(list, "AlwaysCore", 1) ||
        !xf86SetBoolOption(list, "SendCoreEvents", 1) ||
        !xf86SetBoolOption(list, "CorePointer", 1) ||
        !xf86SetBoolOption(list, "CoreKeyboard", 1)) {
        xf86Msg(X_CONFIG, "%s: doesn't report core events\n", pInfo->name);
    }
    else {
        pInfo->flags |= XI86_ALWAYS_CORE;
        xf86Msg(X_CONFIG, "%s: always reports core events\n", pInfo->name);
    }
}

/***********************************************************************
 *
 * xf86ActivateDevice --
 *
 *	Initialize an input device.
 *
 * Returns TRUE on success, or FALSE otherwise.
 ***********************************************************************
 */
static DeviceIntPtr
xf86ActivateDevice(InputInfoPtr pInfo)
{
    DeviceIntPtr dev;
    Atom atom;

    dev = AddInputDevice(serverClient, pInfo->device_control, TRUE);

    if (dev == NULL) {
        xf86Msg(X_ERROR, "Too many input devices. Ignoring %s\n", pInfo->name);
        pInfo->dev = NULL;
        return NULL;
    }

    atom = MakeAtom(pInfo->type_name, strlen(pInfo->type_name), TRUE);
    AssignTypeAndName(dev, atom, pInfo->name);
    dev->public.devicePrivate = pInfo;
    pInfo->dev = dev;

    dev->coreEvents = pInfo->flags & XI86_ALWAYS_CORE;
    dev->type = SLAVE;
    dev->spriteInfo->spriteOwner = FALSE;

    dev->config_info = xf86SetStrOption(pInfo->options, "config_info", NULL);

    if (serverGeneration == 1)
        xf86Msg(X_INFO,
                "XINPUT: Adding extended input device \"%s\" (type: %s, id %d)\n",
                pInfo->name, pInfo->type_name, dev->id);

    return dev;
}

/****************************************************************************
 *
 * Caller:	ProcXSetDeviceMode
 *
 * Change the mode of an extension device.
 * This function is used to change the mode of a device from reporting
 * relative motion to reporting absolute positional information, and
 * vice versa.
 * The default implementation below is that no such devices are supported.
 *
 ***********************************************************************
 */

int
SetDeviceMode(ClientPtr client, DeviceIntPtr dev, int mode)
{
    InputInfoPtr pInfo = (InputInfoPtr) dev->public.devicePrivate;

    if (pInfo->switch_mode) {
        return (*pInfo->switch_mode) (client, dev, mode);
    }
    else
        return BadMatch;
}

/***********************************************************************
 *
 * Caller:	ProcXSetDeviceValuators
 *
 * Set the value of valuators on an extension input device.
 * This function is used to set the initial value of valuators on
 * those input devices that are capable of reporting either relative
 * motion or an absolute position, and allow an initial position to be set.
 * The default implementation below is that no such devices are supported.
 *
 ***********************************************************************
 */

int
SetDeviceValuators(ClientPtr client, DeviceIntPtr dev, int *valuators,
                   int first_valuator, int num_valuators)
{
    InputInfoPtr pInfo = (InputInfoPtr) dev->public.devicePrivate;

    if (pInfo->set_device_valuators)
        return (*pInfo->set_device_valuators) (pInfo, valuators, first_valuator,
                                               num_valuators);

    return BadMatch;
}

/***********************************************************************
 *
 * Caller:	ProcXChangeDeviceControl
 *
 * Change the specified device controls on an extension input device.
 *
 ***********************************************************************
 */

int
ChangeDeviceControl(ClientPtr client, DeviceIntPtr dev, xDeviceCtl * control)
{
    InputInfoPtr pInfo = (InputInfoPtr) dev->public.devicePrivate;

    if (!pInfo->control_proc) {
        switch (control->control) {
        case DEVICE_CORE:
        case DEVICE_ABS_CALIB:
        case DEVICE_ABS_AREA:
            return BadMatch;
        case DEVICE_RESOLUTION:
        case DEVICE_ENABLE:
            return Success;
        default:
            return BadMatch;
        }
    }
    else {
        return (*pInfo->control_proc) (pInfo, control);
    }
}

/*
 * Get the operating system name from uname and store it statically to avoid
 * repeating the system call each time MatchOS is checked.
 */
static const char *
HostOS(void)
{
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname name;
    static char host_os[sizeof(name.sysname)] = "";

    if (*host_os == '\0') {
        if (uname(&name) >= 0)
            strlcpy(host_os, name.sysname, sizeof(host_os));
        else {
            strlcpy(host_os, "unknown", sizeof(host_os));
        }
    }
    return host_os;
#else
    return "";
#endif
}

static int
match_substring(const char *attr, const char *pattern)
{
    return (strstr(attr, pattern)) ? 0 : -1;
}

#ifdef HAVE_FNMATCH_H
static int
match_pattern(const char *attr, const char *pattern)
{
    return fnmatch(pattern, attr, 0);
}
#else
#define match_pattern match_substring
#endif

#ifdef HAVE_FNMATCH_H
static int
match_path_pattern(const char *attr, const char *pattern)
{
    return fnmatch(pattern, attr, FNM_PATHNAME);
}
#else
#define match_path_pattern match_substring
#endif

/*
 * If no Layout section is found, xf86ServerLayout.id becomes "(implicit)"
 * It is convenient that "" in patterns means "no explicit layout"
 */
static int
match_string_implicit(const char *attr, const char *pattern)
{
    if (strlen(pattern)) {
        return strcmp(attr, pattern);
    }
    else {
        return strcmp(attr, "(implicit)");
    }
}

/*
 * Match an attribute against a list of NULL terminated arrays of patterns.
 * If a pattern in each list entry is matched, return TRUE.
 */
static Bool
MatchAttrToken(const char *attr, struct xorg_list *patterns,
               int (*compare) (const char *attr, const char *pattern))
{
    const xf86MatchGroup *group;

    /* If there are no patterns, accept the match */
    if (xorg_list_is_empty(patterns))
        return TRUE;

    /*
     * Iterate the list of patterns ensuring each entry has a
     * match. Each list entry is a separate Match line of the same type.
     */
    xorg_list_for_each_entry(group, patterns, entry) {
        char *const *cur;
        Bool is_negated = group->is_negated;
        Bool match = is_negated;

        /* If there's a pattern but no attribute, we reject the match for a
         * MatchFoo directive, and accept it for a NoMatchFoo directive
         */
        if (!attr)
            return is_negated;

        for (cur = group->values; *cur; cur++)
            if ((*compare) (attr, *cur) == 0) {
                match = !is_negated;
                break;
            }
        if (!match)
            return FALSE;
    }

    /* All the entries in the list matched the attribute */
    return TRUE;
}

/*
 * Classes without any Match statements match all devices. Otherwise, all
 * statements must match.
 */
static Bool
InputClassMatches(const XF86ConfInputClassPtr iclass, const InputInfoPtr idev,
                  const InputAttributes * attrs)
{
    /* MatchProduct substring */
    if (!MatchAttrToken
        (attrs->product, &iclass->match_product, match_substring))
        return FALSE;

    /* MatchVendor substring */
    if (!MatchAttrToken(attrs->vendor, &iclass->match_vendor, match_substring))
        return FALSE;

    /* MatchDevicePath pattern */
    if (!MatchAttrToken
        (attrs->device, &iclass->match_device, match_path_pattern))
        return FALSE;

    /* MatchOS case-insensitive string */
    if (!MatchAttrToken(HostOS(), &iclass->match_os, strcasecmp))
        return FALSE;

    /* MatchPnPID pattern */
    if (!MatchAttrToken(attrs->pnp_id, &iclass->match_pnpid, match_pattern))
        return FALSE;

    /* MatchUSBID pattern */
    if (!MatchAttrToken(attrs->usb_id, &iclass->match_usbid, match_pattern))
        return FALSE;

    /* MatchDriver string */
    if (!MatchAttrToken(idev->driver, &iclass->match_driver, strcmp))
        return FALSE;

    /*
     * MatchTag string
     * See if any of the device's tags match any of the MatchTag tokens.
     */
    if (!xorg_list_is_empty(&iclass->match_tag)) {
        char *const *tag;
        Bool match;

        if (!attrs->tags)
            return FALSE;
        for (tag = attrs->tags, match = FALSE; *tag; tag++) {
            if (MatchAttrToken(*tag, &iclass->match_tag, strcmp)) {
                match = TRUE;
                break;
            }
        }
        if (!match)
            return FALSE;
    }

    /* MatchLayout string */
    if (!xorg_list_is_empty(&iclass->match_layout)) {
        if (!MatchAttrToken(xf86ConfigLayout.id,
                            &iclass->match_layout, match_string_implicit))
            return FALSE;
    }

    /* MatchIs* booleans */
    if (iclass->is_keyboard.set &&
        iclass->is_keyboard.val != ! !(attrs->flags & (ATTR_KEY|ATTR_KEYBOARD)))
        return FALSE;
    if (iclass->is_pointer.set &&
        iclass->is_pointer.val != ! !(attrs->flags & ATTR_POINTER))
        return FALSE;
    if (iclass->is_joystick.set &&
        iclass->is_joystick.val != ! !(attrs->flags & ATTR_JOYSTICK))
        return FALSE;
    if (iclass->is_tablet.set &&
        iclass->is_tablet.val != ! !(attrs->flags & ATTR_TABLET))
        return FALSE;
    if (iclass->is_tablet_pad.set &&
        iclass->is_tablet_pad.val != ! !(attrs->flags & ATTR_TABLET_PAD))
        return FALSE;
    if (iclass->is_touchpad.set &&
        iclass->is_touchpad.val != ! !(attrs->flags & ATTR_TOUCHPAD))
        return FALSE;
    if (iclass->is_touchscreen.set &&
        iclass->is_touchscreen.val != ! !(attrs->flags & ATTR_TOUCHSCREEN))
        return FALSE;

    return TRUE;
}

/*
 * Merge in any InputClass configurations. Options in each InputClass
 * section have more priority than the original device configuration as
 * well as any previous InputClass sections.
 */
static int
MergeInputClasses(const InputInfoPtr idev, const InputAttributes * attrs)
{
    XF86ConfInputClassPtr cl;
    XF86OptionPtr classopts;

    for (cl = xf86configptr->conf_inputclass_lst; cl; cl = cl->list.next) {
        if (!InputClassMatches(cl, idev, attrs))
            continue;

        /* Collect class options and driver settings */
        classopts = xf86optionListDup(cl->option_lst);
        if (cl->driver) {
            free((void *) idev->driver);
            idev->driver = xstrdup(cl->driver);
            if (!idev->driver) {
                xf86Msg(X_ERROR, "Failed to allocate memory while merging "
                        "InputClass configuration");
                return BadAlloc;
            }
            classopts = xf86ReplaceStrOption(classopts, "driver", idev->driver);
        }

        /* Apply options to device with InputClass settings preferred. */
        xf86Msg(X_CONFIG, "%s: Applying InputClass \"%s\"\n",
                idev->name, cl->identifier);
        idev->options = xf86optionListMerge(idev->options, classopts);
    }

    return Success;
}

/*
 * Iterate the list of classes and look for Option "Ignore". Return the
 * value of the last matching class and holler when returning TRUE.
 */
static Bool
IgnoreInputClass(const InputInfoPtr idev, const InputAttributes * attrs)
{
    XF86ConfInputClassPtr cl;
    Bool ignore = FALSE;
    const char *ignore_class;

    for (cl = xf86configptr->conf_inputclass_lst; cl; cl = cl->list.next) {
        if (!InputClassMatches(cl, idev, attrs))
            continue;
        if (xf86findOption(cl->option_lst, "Ignore")) {
            ignore = xf86CheckBoolOption(cl->option_lst, "Ignore", FALSE);
            ignore_class = cl->identifier;
        }
    }

    if (ignore)
        xf86Msg(X_CONFIG, "%s: Ignoring device from InputClass \"%s\"\n",
                idev->name, ignore_class);
    return ignore;
}

InputInfoPtr
xf86AllocateInput(void)
{
    InputInfoPtr pInfo;

    pInfo = calloc(sizeof(*pInfo), 1);
    if (!pInfo)
        return NULL;

    pInfo->fd = -1;
    pInfo->type_name = "UNKNOWN";

    return pInfo;
}

/* Append InputInfoRec to the tail of xf86InputDevs. */
static void
xf86AddInput(InputDriverPtr drv, InputInfoPtr pInfo)
{
    InputInfoPtr *prev = NULL;

    pInfo->drv = drv;
    pInfo->module = DuplicateModule(drv->module, NULL);

    for (prev = &xf86InputDevs; *prev; prev = &(*prev)->next);

    *prev = pInfo;
    pInfo->next = NULL;

    xf86CollectInputOptions(pInfo, (const char **) drv->default_options);
    xf86OptionListReport(pInfo->options);
    xf86ProcessCommonOptions(pInfo, pInfo->options);
}

/*
 * Remove an entry from xf86InputDevs and free all the device's information.
 */
void
xf86DeleteInput(InputInfoPtr pInp, int flags)
{
    /* First check if the inputdev is valid. */
    if (pInp == NULL)
        return;

    if (pInp->module)
        UnloadModule(pInp->module);

    /* This should *really* be handled in drv->UnInit(dev) call instead, but
     * if the driver forgets about it make sure we free it or at least crash
     * with flying colors */
    free(pInp->private);

    FreeInputAttributes(pInp->attrs);

    if (pInp->flags & XI86_SERVER_FD)
        systemd_logind_release_fd(pInp->major, pInp->minor, pInp->fd);

    /* Remove the entry from the list. */
    if (pInp == xf86InputDevs)
        xf86InputDevs = pInp->next;
    else {
        InputInfoPtr p = xf86InputDevs;

        while (p && p->next != pInp)
            p = p->next;
        if (p)
            p->next = pInp->next;
        /* Else the entry wasn't in the xf86InputDevs list (ignore this). */
    }

    free((void *) pInp->driver);
    free((void *) pInp->name);
    xf86optionListFree(pInp->options);
    free(pInp);
}

/*
 * Apply backend-specific initialization. Invoked after ActivateDevice(),
 * i.e. after the driver successfully completed DEVICE_INIT and the device
 * is advertised.
 * @param dev the device
 * @return Success or an error code
 */
static int
xf86InputDevicePostInit(DeviceIntPtr dev)
{
    ApplyAccelerationSettings(dev);
    ApplyTransformationMatrix(dev);
    ApplyAutoRepeat(dev);
    return Success;
}

static void
xf86stat(const char *path, int *maj, int *min)
{
    struct stat st;

    if (stat(path, &st) == -1)
        return;

    *maj = major(st.st_rdev);
    *min = minor(st.st_rdev);
}

static inline InputDriverPtr
xf86LoadInputDriver(const char *driver_name)
{
    InputDriverPtr drv = NULL;

    /* Memory leak for every attached device if we don't
     * test if the module is already loaded first */
    drv = xf86LookupInputDriver(driver_name);
    if (!drv) {
        if (xf86LoadOneModule(driver_name, NULL))
            drv = xf86LookupInputDriver(driver_name);
    }

    return drv;
}

/**
 * Create a new input device, activate and enable it.
 *
 * Possible return codes:
 *    BadName .. a bad driver name was supplied.
 *    BadImplementation ... The driver does not have a PreInit function. This
 *                          is a driver bug.
 *    BadMatch .. device initialization failed.
 *    BadAlloc .. too many input devices
 *
 * @param idev The device, already set up with identifier, driver, and the
 * options.
 * @param pdev Pointer to the new device, if Success was reported.
 * @param enable Enable the device after activating it.
 *
 * @return Success or an error code
 */
_X_INTERNAL int
xf86NewInputDevice(InputInfoPtr pInfo, DeviceIntPtr *pdev, BOOL enable)
{
    InputDriverPtr drv = NULL;
    DeviceIntPtr dev = NULL;
    Bool paused = FALSE;
    int rval;
    char *path = NULL;

    drv = xf86LoadInputDriver(pInfo->driver);
    if (!drv) {
        xf86Msg(X_ERROR, "No input driver matching `%s'\n", pInfo->driver);

        if (strlen(FALLBACK_INPUT_DRIVER) > 0) {
            xf86Msg(X_INFO, "Falling back to input driver `%s'\n",
                    FALLBACK_INPUT_DRIVER);
            drv = xf86LoadInputDriver(FALLBACK_INPUT_DRIVER);
            if (drv) {
                free(pInfo->driver);
                pInfo->driver = strdup(FALLBACK_INPUT_DRIVER);
            }
        }
        if (!drv) {
            rval = BadName;
            goto unwind;
        }
    }

    xf86Msg(X_INFO, "Using input driver '%s' for '%s'\n", drv->driverName,
            pInfo->name);

    if (!drv->PreInit) {
        xf86Msg(X_ERROR,
                "Input driver `%s' has no PreInit function (ignoring)\n",
                drv->driverName);
        rval = BadImplementation;
        goto unwind;
    }

    path = xf86CheckStrOption(pInfo->options, "Device", NULL);
    if (path && pInfo->major == 0 && pInfo->minor == 0)
        xf86stat(path, &pInfo->major, &pInfo->minor);

    if (path && (drv->capabilities & XI86_DRV_CAP_SERVER_FD)){
        int fd = systemd_logind_take_fd(pInfo->major, pInfo->minor,
                                        path, &paused);
        if (fd != -1) {
            if (paused) {
                /* Put on new_input_devices list for delayed probe */
                PausedInputDevicePtr new_device = xnfalloc(sizeof *new_device);
                new_device->pInfo = pInfo;

                xorg_list_append(&new_device->node, &new_input_devices_list);
                systemd_logind_release_fd(pInfo->major, pInfo->minor, fd);
                free(path);
                return BadMatch;
            }
            pInfo->fd = fd;
            pInfo->flags |= XI86_SERVER_FD;
            pInfo->options = xf86ReplaceIntOption(pInfo->options, "fd", fd);
        }
    }

    free(path);

    xf86AddInput(drv, pInfo);

    input_lock();
    rval = drv->PreInit(drv, pInfo, 0);
    input_unlock();

    if (rval != Success) {
        xf86Msg(X_ERROR, "PreInit returned %d for \"%s\"\n", rval, pInfo->name);
        goto unwind;
    }

    if (!(dev = xf86ActivateDevice(pInfo))) {
        rval = BadAlloc;
        goto unwind;
    }

    rval = ActivateDevice(dev, TRUE);
    if (rval != Success) {
        xf86Msg(X_ERROR, "Couldn't init device \"%s\"\n", pInfo->name);
        RemoveDevice(dev, TRUE);
        goto unwind;
    }

    rval = xf86InputDevicePostInit(dev);
    if (rval != Success) {
        xf86Msg(X_ERROR, "Couldn't post-init device \"%s\"\n", pInfo->name);
        RemoveDevice(dev, TRUE);
        goto unwind;
    }

    /* Enable it if it's properly initialised and we're currently in the VT */
    if (enable && dev->inited && dev->startup && xf86VTOwner()) {
        input_lock();
        EnableDevice(dev, TRUE);
        if (!dev->enabled) {
            xf86Msg(X_ERROR, "Couldn't init device \"%s\"\n", pInfo->name);
            RemoveDevice(dev, TRUE);
            rval = BadMatch;
            input_unlock();
            goto unwind;
        }
        /* send enter/leave event, update sprite window */
        CheckMotion(NULL, dev);
        input_unlock();
    }

    *pdev = dev;
    return Success;

 unwind:
    if (pInfo) {
        if (drv && drv->UnInit)
            drv->UnInit(drv, pInfo, 0);
        else
            xf86DeleteInput(pInfo, 0);
    }
    return rval;
}

int
NewInputDeviceRequest(InputOption *options, InputAttributes * attrs,
                      DeviceIntPtr *pdev)
{
    InputInfoPtr pInfo = NULL;
    InputOption *option = NULL;
    int rval = Success;
    int is_auto = 0;

    pInfo = xf86AllocateInput();
    if (!pInfo)
        return BadAlloc;

    nt_list_for_each_entry(option, options, list.next) {
        const char *key = input_option_get_key(option);
        const char *value = input_option_get_value(option);

        if (strcasecmp(key, "driver") == 0) {
            if (pInfo->driver) {
                rval = BadRequest;
                goto unwind;
            }
            pInfo->driver = xstrdup(value);
            if (!pInfo->driver) {
                rval = BadAlloc;
                goto unwind;
            }
        }

        if (strcasecmp(key, "name") == 0 || strcasecmp(key, "identifier") == 0) {
            if (pInfo->name) {
                rval = BadRequest;
                goto unwind;
            }
            pInfo->name = xstrdup(value);
            if (!pInfo->name) {
                rval = BadAlloc;
                goto unwind;
            }
        }

        if (strcmp(key, "_source") == 0 &&
            (strcmp(value, "server/hal") == 0 ||
             strcmp(value, "server/udev") == 0 ||
             strcmp(value, "server/wscons") == 0)) {
            is_auto = 1;
            if (!xf86Info.autoAddDevices) {
                rval = BadMatch;
                goto unwind;
            }
        }

        if (strcmp(key, "major") == 0)
            pInfo->major = atoi(value);

        if (strcmp(key, "minor") == 0)
            pInfo->minor = atoi(value);
    }

    nt_list_for_each_entry(option, options, list.next) {
        /* Copy option key/value strings from the provided list */
        pInfo->options = xf86AddNewOption(pInfo->options,
                                          input_option_get_key(option),
                                          input_option_get_value(option));
    }

    /* Apply InputClass settings */
    if (attrs) {
        if (IgnoreInputClass(pInfo, attrs)) {
            rval = BadIDChoice;
            goto unwind;
        }

        rval = MergeInputClasses(pInfo, attrs);
        if (rval != Success)
            goto unwind;

        pInfo->attrs = DuplicateInputAttributes(attrs);
    }

    if (!pInfo->name) {
        xf86Msg(X_INFO, "No identifier specified, ignoring this device.\n");
        rval = BadRequest;
        goto unwind;
    }

    if (!pInfo->driver) {
        xf86Msg(X_INFO, "No input driver specified, ignoring this device.\n");
        xf86Msg(X_INFO,
                "This device may have been added with another device file.\n");
        rval = BadRequest;
        goto unwind;
    }

    rval = xf86NewInputDevice(pInfo, pdev,
                              (!is_auto ||
                               (is_auto && xf86Info.autoEnableDevices)));

    return rval;

 unwind:
    if (is_auto && !xf86Info.autoAddDevices)
        xf86Msg(X_INFO, "AutoAddDevices is off - not adding device.\n");
    xf86DeleteInput(pInfo, 0);
    return rval;
}

void
DeleteInputDeviceRequest(DeviceIntPtr pDev)
{
    InputInfoPtr pInfo = (InputInfoPtr) pDev->public.devicePrivate;
    InputDriverPtr drv = NULL;
    Bool isMaster = IsMaster(pDev);

    if (pInfo)                  /* need to get these before RemoveDevice */
        drv = pInfo->drv;

    input_lock();
    RemoveDevice(pDev, TRUE);

    if (!isMaster && pInfo != NULL) {
        if (drv->UnInit)
            drv->UnInit(drv, pInfo, 0);
        else
            xf86DeleteInput(pInfo, 0);
    }
    input_unlock();
}

void
RemoveInputDeviceTraces(const char *config_info)
{
    PausedInputDevicePtr d, tmp;

    xorg_list_for_each_entry_safe(d, tmp, &new_input_devices_list, node) {
        const char *ci = xf86findOptionValue(d->pInfo->options, "config_info");
        if (!ci || strcmp(ci, config_info) != 0)
            continue;

        xorg_list_del(&d->node);
        free(d);
    }
}

/*
 * convenient functions to post events
 */

void
xf86PostMotionEvent(DeviceIntPtr device,
                    int is_absolute, int first_valuator, int num_valuators, ...)
{
    va_list var;
    int i = 0;
    ValuatorMask mask;

    XI_VERIFY_VALUATORS(num_valuators);

    valuator_mask_zero(&mask);
    va_start(var, num_valuators);
    for (i = 0; i < num_valuators; i++)
        valuator_mask_set(&mask, first_valuator + i, va_arg(var, int));

    va_end(var);

    xf86PostMotionEventM(device, is_absolute, &mask);
}

void
xf86PostMotionEventP(DeviceIntPtr device,
                     int is_absolute,
                     int first_valuator,
                     int num_valuators, const int *valuators)
{
    ValuatorMask mask;

    XI_VERIFY_VALUATORS(num_valuators);

    valuator_mask_set_range(&mask, first_valuator, num_valuators, valuators);
    xf86PostMotionEventM(device, is_absolute, &mask);
}

static int
xf86CheckMotionEvent4DGA(DeviceIntPtr device, int is_absolute,
                         const ValuatorMask *mask)
{
    int stolen = 0;

#ifdef XFreeXDGA
    ScreenPtr scr = NULL;
    int idx = 0, i;

    /* The evdev driver may not always send all axes across. */
    if (valuator_mask_isset(mask, 0) || valuator_mask_isset(mask, 1)) {
        scr = miPointerGetScreen(device);
        if (scr) {
            int dx = 0, dy = 0;

            idx = scr->myNum;

            if (valuator_mask_isset(mask, 0)) {
                dx = valuator_mask_get(mask, 0);
                if (is_absolute)
                    dx -= device->last.valuators[0];
                else if (valuator_mask_has_unaccelerated(mask))
                    dx = valuator_mask_get_unaccelerated(mask, 0);
            }

            if (valuator_mask_isset(mask, 1)) {
                dy = valuator_mask_get(mask, 1);
                if (is_absolute)
                    dy -= device->last.valuators[1];
                else if (valuator_mask_has_unaccelerated(mask))
                    dy = valuator_mask_get_unaccelerated(mask, 1);
            }

            if (DGAStealMotionEvent(device, idx, dx, dy))
                stolen = 1;
        }
    }

    for (i = 2; i < valuator_mask_size(mask); i++) {
        AxisInfoPtr ax;
        double incr;
        int val, button;

        if (i >= device->valuator->numAxes)
            break;

        if (!valuator_mask_isset(mask, i))
            continue;

        ax = &device->valuator->axes[i];

        if (ax->scroll.type == SCROLL_TYPE_NONE)
            continue;

        if (!scr) {
            scr = miPointerGetScreen(device);
            if (!scr)
                break;
            idx = scr->myNum;
        }

        incr = ax->scroll.increment;
        val = valuator_mask_get(mask, i);

        if (ax->scroll.type == SCROLL_TYPE_VERTICAL) {
            if (incr * val < 0)
                button = 4; /* up */
            else
                button = 5; /* down */
        } else { /* SCROLL_TYPE_HORIZONTAL */
            if (incr * val < 0)
                button = 6; /* left */
            else
                button = 7; /* right */
        }

        if (DGAStealButtonEvent(device, idx, button, 1) &&
                DGAStealButtonEvent(device, idx, button, 0))
            stolen = 1;
    }

#endif

    return stolen;
}

void
xf86PostMotionEventM(DeviceIntPtr device,
                     int is_absolute, const ValuatorMask *mask)
{
    int flags = 0;

    if (xf86CheckMotionEvent4DGA(device, is_absolute, mask))
        return;

    if (valuator_mask_num_valuators(mask) > 0) {
        if (is_absolute)
            flags = POINTER_ABSOLUTE;
        else
            flags = POINTER_RELATIVE | POINTER_ACCELERATE;
    }

    QueuePointerEvents(device, MotionNotify, 0, flags, mask);
}

void
xf86PostProximityEvent(DeviceIntPtr device,
                       int is_in, int first_valuator, int num_valuators, ...)
{
    va_list var;
    int i;
    ValuatorMask mask;

    XI_VERIFY_VALUATORS(num_valuators);

    valuator_mask_zero(&mask);
    va_start(var, num_valuators);
    for (i = 0; i < num_valuators; i++)
        valuator_mask_set(&mask, first_valuator + i, va_arg(var, int));

    va_end(var);

    xf86PostProximityEventM(device, is_in, &mask);
}

void
xf86PostProximityEventP(DeviceIntPtr device,
                        int is_in,
                        int first_valuator,
                        int num_valuators, const int *valuators)
{
    ValuatorMask mask;

    XI_VERIFY_VALUATORS(num_valuators);

    valuator_mask_set_range(&mask, first_valuator, num_valuators, valuators);
    xf86PostProximityEventM(device, is_in, &mask);
}

void
xf86PostProximityEventM(DeviceIntPtr device,
                        int is_in, const ValuatorMask *mask)
{
    QueueProximityEvents(device, is_in ? ProximityIn : ProximityOut, mask);
}

void
xf86PostButtonEvent(DeviceIntPtr device,
                    int is_absolute,
                    int button,
                    int is_down, int first_valuator, int num_valuators, ...)
{
    va_list var;
    ValuatorMask mask;
    int i = 0;

    XI_VERIFY_VALUATORS(num_valuators);

    valuator_mask_zero(&mask);

    va_start(var, num_valuators);
    for (i = 0; i < num_valuators; i++)
        valuator_mask_set(&mask, first_valuator + i, va_arg(var, int));

    va_end(var);

    xf86PostButtonEventM(device, is_absolute, button, is_down, &mask);
}

void
xf86PostButtonEventP(DeviceIntPtr device,
                     int is_absolute,
                     int button,
                     int is_down,
                     int first_valuator,
                     int num_valuators, const int *valuators)
{
    ValuatorMask mask;

    XI_VERIFY_VALUATORS(num_valuators);

    valuator_mask_set_range(&mask, first_valuator, num_valuators, valuators);
    xf86PostButtonEventM(device, is_absolute, button, is_down, &mask);
}

void
xf86PostButtonEventM(DeviceIntPtr device,
                     int is_absolute,
                     int button, int is_down, const ValuatorMask *mask)
{
    int flags = 0;

    if (valuator_mask_num_valuators(mask) > 0) {
        if (is_absolute)
            flags = POINTER_ABSOLUTE;
        else
            flags = POINTER_RELATIVE | POINTER_ACCELERATE;
    }

#ifdef XFreeXDGA
    if (miPointerGetScreen(device)) {
        int index = miPointerGetScreen(device)->myNum;

        if (DGAStealButtonEvent(device, index, button, is_down))
            return;
    }
#endif

    QueuePointerEvents(device,
                       is_down ? ButtonPress : ButtonRelease, button,
                       flags, mask);
}

void
xf86PostKeyEvent(DeviceIntPtr device, unsigned int key_code, int is_down)
{
    xf86PostKeyEventM(device, key_code, is_down);
}

void
xf86PostKeyEventP(DeviceIntPtr device,
                  unsigned int key_code,
                  int is_down)
{
    xf86PostKeyEventM(device, key_code, is_down);
}

void
xf86PostKeyEventM(DeviceIntPtr device, unsigned int key_code, int is_down)
{
#ifdef XFreeXDGA
    DeviceIntPtr pointer;

    /* Some pointers send key events, paired device is wrong then. */
    pointer = GetMaster(device, POINTER_OR_FLOAT);

    if (miPointerGetScreen(pointer)) {
        int index = miPointerGetScreen(pointer)->myNum;

        if (DGAStealKeyEvent(device, index, key_code, is_down))
            return;
    }
#endif

    QueueKeyboardEvents(device, is_down ? KeyPress : KeyRelease, key_code);
}

void
xf86PostKeyboardEvent(DeviceIntPtr device, unsigned int key_code, int is_down)
{
    ValuatorMask mask;

    valuator_mask_zero(&mask);
    xf86PostKeyEventM(device, key_code, is_down);
}

InputInfoPtr
xf86FirstLocalDevice(void)
{
    return xf86InputDevs;
}

/*
 * Cx     - raw data from touch screen
 * to_max - scaled highest dimension
 *          (remember, this is of rows - 1 because of 0 origin)
 * to_min  - scaled lowest dimension
 * from_max - highest raw value from touch screen calibration
 * from_min  - lowest raw value from touch screen calibration
 *
 * This function is the same for X or Y coordinates.
 * You may have to reverse the high and low values to compensate for
 * different origins on the touch screen vs X.
 *
 * e.g. to scale from device coordinates into screen coordinates, call
 * xf86ScaleAxis(x, 0, screen_width, dev_min, dev_max);
 */

int
xf86ScaleAxis(int Cx, int to_max, int to_min, int from_max, int from_min)
{
    int X;
    int64_t to_width = to_max - to_min;
    int64_t from_width = from_max - from_min;

    if (from_width) {
        X = (int) (((to_width * (Cx - from_min)) / from_width) + to_min);
    }
    else {
        X = 0;
        ErrorF("Divide by Zero in xf86ScaleAxis\n");
    }

    if (X > to_max)
        X = to_max;
    if (X < to_min)
        X = to_min;

    return X;
}

Bool
xf86InitValuatorAxisStruct(DeviceIntPtr dev, int axnum, Atom label, int minval,
                           int maxval, int resolution, int min_res, int max_res,
                           int mode)
{
    if (!dev || !dev->valuator)
        return FALSE;

    return InitValuatorAxisStruct(dev, axnum, label, minval, maxval, resolution,
                                  min_res, max_res, mode);
}

/*
 * Set the valuator values to be in sync with dix/event.c
 * DefineInitialRootWindow().
 */
void
xf86InitValuatorDefaults(DeviceIntPtr dev, int axnum)
{
    if (axnum == 0) {
        dev->valuator->axisVal[0] = screenInfo.screens[0]->width / 2;
        dev->last.valuators[0] = dev->valuator->axisVal[0];
    }
    else if (axnum == 1) {
        dev->valuator->axisVal[1] = screenInfo.screens[0]->height / 2;
        dev->last.valuators[1] = dev->valuator->axisVal[1];
    }
}

/**
 * Deactivate a device. Call this function from the driver if you receive a
 * read error or something else that spoils your day.
 * Device will be moved to the off_devices list, but it will still be there
 * until you really clean up after it.
 * Notifies the client about an inactive device.
 *
 * @param panic True if device is unrecoverable and needs to be removed.
 */
void
xf86DisableDevice(DeviceIntPtr dev, Bool panic)
{
    if (!panic) {
        DisableDevice(dev, TRUE);
    }
    else {
        SendDevicePresenceEvent(dev->id, DeviceUnrecoverable);
        DeleteInputDeviceRequest(dev);
    }
}

/**
 * Reactivate a device. Call this function from the driver if you just found
 * out that the read error wasn't quite that bad after all.
 * Device will be re-activated, and an event sent to the client.
 */
void
xf86EnableDevice(DeviceIntPtr dev)
{
    EnableDevice(dev, TRUE);
}

/**
 * Post a touch event with optional valuators.  If this is the first touch in
 * the sequence, at least x & y valuators must be provided. The driver is
 * responsible for maintaining the correct event sequence (TouchBegin, TouchUpdate,
 * TouchEnd). Submitting an update or end event for a unregistered touchid will
 * result in errors.
 * Touch IDs may be reused by the driver but only after a TouchEnd has been
 * submitted for that touch ID.
 *
 * @param dev The device to post the event for
 * @param touchid The touchid of the current touch event. Must be an
 * existing ID for TouchUpdate or TouchEnd events
 * @param type One of XI_TouchBegin, XI_TouchUpdate, XI_TouchEnd
 * @param flags Flags for this event
 * @param The valuator mask with all valuators set for this event.
 */
void
xf86PostTouchEvent(DeviceIntPtr dev, uint32_t touchid, uint16_t type,
                   uint32_t flags, const ValuatorMask *mask)
{

    QueueTouchEvents(dev, type, touchid, flags, mask);
}

/**
 * Post a gesture pinch event.  The driver is responsible for maintaining the
 * correct event sequence (GesturePinchBegin, GesturePinchUpdate,
 * GesturePinchEnd).
 *
 * @param dev The device to post the event for
 * @param type One of XI_GesturePinchBegin, XI_GesturePinchUpdate,
 *        XI_GesturePinchEnd
 * @param num_touches The number of touches in the gesture
 * @param flags Flags for this event
 * @param delta_x,delta_y accelerated relative motion delta
 * @param delta_unaccel_x,delta_unaccel_y unaccelerated relative motion delta
 * @param scale absolute scale of a pinch gesture
 * @param delta_angle the ange delta in degrees between the last and the current pinch event.
 */
void
xf86PostGesturePinchEvent(DeviceIntPtr dev, uint16_t type,
                          uint16_t num_touches, uint32_t flags,
                          double delta_x, double delta_y,
                          double delta_unaccel_x,
                          double delta_unaccel_y,
                          double scale, double delta_angle)
{
    QueueGesturePinchEvents(dev, type, num_touches, flags, delta_x, delta_y,
                            delta_unaccel_x, delta_unaccel_y,
                            scale, delta_angle);
}

/**
 * Post a gesture swipe event.  The driver is responsible for maintaining the
 * correct event sequence (GestureSwipeBegin, GestureSwipeUpdate,
 * GestureSwipeEnd).
 *
 * @param dev The device to post the event for
 * @param type One of XI_GestureSwipeBegin, XI_GestureSwipeUpdate,
 *        XI_GestureSwipeEnd
 * @param num_touches The number of touches in the gesture
 * @param flags Flags for this event
 * @param delta_x,delta_y accelerated relative motion delta
 * @param delta_unaccel_x,delta_unaccel_y unaccelerated relative motion delta
 */
void
xf86PostGestureSwipeEvent(DeviceIntPtr dev, uint16_t type,
                          uint16_t num_touches, uint32_t flags,
                          double delta_x, double delta_y,
                          double delta_unaccel_x,
                          double delta_unaccel_y)
{
    QueueGestureSwipeEvents(dev, type, num_touches, flags, delta_x, delta_y,
                            delta_unaccel_x, delta_unaccel_y);
}

void
xf86InputEnableVTProbe(void)
{
    int is_auto = 0;
    DeviceIntPtr pdev;
    PausedInputDevicePtr d, tmp;

    xorg_list_for_each_entry_safe(d, tmp, &new_input_devices_list, node) {
        InputInfoPtr pInfo = d->pInfo;
        const char *value = xf86findOptionValue(pInfo->options, "_source");

        is_auto = 0;
        if (value &&
            (strcmp(value, "server/hal") == 0 ||
             strcmp(value, "server/udev") == 0 ||
             strcmp(value, "server/wscons") == 0))
            is_auto = 1;

        xf86NewInputDevice(pInfo, &pdev,
                                  (!is_auto ||
                                   (is_auto && xf86Info.autoEnableDevices)));
        xorg_list_del(&d->node);
        free(d);
    }
}

/* end of xf86Xinput.c */
