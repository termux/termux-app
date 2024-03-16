/*
 * Copyright © 2009 Julien Cristau
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
 * Author: Julien Cristau <jcristau@debian.org>
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <libudev.h>
#include <ctype.h>
#include <unistd.h>

#include "input.h"
#include "inputstr.h"
#include "hotplug.h"
#include "config-backends.h"
#include "os.h"
#include "globals.h"
#include "systemd-logind.h"

#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif

#define UDEV_XKB_PROP_KEY "xkb"

#define LOG_PROPERTY(path, prop, val)                                   \
    LogMessageVerb(X_INFO, 10,                                          \
                   "config/udev: getting property %s on %s "            \
                   "returned \"%s\"\n",                                 \
                   (prop), (path), (val) ? (val) : "(null)")
#define LOG_SYSATTR(path, attr, val)                                    \
    LogMessageVerb(X_INFO, 10,                                          \
                   "config/udev: getting attribute %s on %s "           \
                   "returned \"%s\"\n",                                 \
                   (attr), (path), (val) ? (val) : "(null)")

static struct udev_monitor *udev_monitor;

#ifdef CONFIG_UDEV_KMS
static void
config_udev_odev_setup_attribs(struct udev_device *udev_device, const char *path, const char *syspath,
                               int major, int minor,
                               config_odev_probe_proc_ptr probe_callback);
#endif

static char itoa_buf[16];

static const char *itoa(int i)
{
    snprintf(itoa_buf, sizeof(itoa_buf), "%d", i);
    return itoa_buf;
}

static Bool
check_seat(struct udev_device *udev_device)
{
    const char *dev_seat;

    dev_seat = udev_device_get_property_value(udev_device, "ID_SEAT");
    if (!dev_seat)
        dev_seat = "seat0";

    if (SeatId && strcmp(dev_seat, SeatId))
        return FALSE;

    if (!SeatId && strcmp(dev_seat, "seat0"))
        return FALSE;

    return TRUE;
}

static void
device_added(struct udev_device *udev_device)
{
    const char *path, *name = NULL;
    char *config_info = NULL;
    const char *syspath;
    const char *tags_prop;
    const char *key, *value, *tmp;
#ifdef CONFIG_UDEV_KMS
    const char *subsys = NULL;
#endif
    InputOption *input_options;
    InputAttributes attrs = { };
    DeviceIntPtr dev = NULL;
    struct udev_list_entry *set, *entry;
    struct udev_device *parent;
    int rc;
    dev_t devnum;

    path = udev_device_get_devnode(udev_device);

    syspath = udev_device_get_syspath(udev_device);

    if (!path || !syspath)
        return;

    if (!check_seat(udev_device))
        return;

    devnum = udev_device_get_devnum(udev_device);

#ifdef CONFIG_UDEV_KMS
    subsys = udev_device_get_subsystem(udev_device);

    if (subsys && !strcmp(subsys, "drm")) {
        const char *sysname = udev_device_get_sysname(udev_device);

        if (strncmp(sysname, "card", 4) != 0)
            return;

        /* Check for devices already added through xf86platformProbe() */
        if (xf86_find_platform_device_by_devnum(major(devnum), minor(devnum)))
            return;

        LogMessage(X_INFO, "config/udev: Adding drm device (%s)\n", path);

        config_udev_odev_setup_attribs(udev_device, path, syspath, major(devnum),
                                       minor(devnum), NewGPUDeviceRequest);
        return;
    }
#endif

    value = udev_device_get_property_value(udev_device, "ID_INPUT");
    if (!value || !strcmp(value, "0")) {
        LogMessageVerb(X_INFO, 10,
                       "config/udev: ignoring device %s without "
                       "property ID_INPUT set\n", path);
        return;
    }

    input_options = input_option_new(NULL, "_source", "server/udev");
    if (!input_options)
        return;

    parent = udev_device_get_parent(udev_device);
    if (parent) {
        const char *ppath = udev_device_get_devnode(parent);
        const char *product = udev_device_get_property_value(parent, "PRODUCT");
        const char *pnp_id = udev_device_get_sysattr_value(parent, "id");
        unsigned int usb_vendor, usb_model;

        name = udev_device_get_sysattr_value(parent, "name");
        LOG_SYSATTR(ppath, "name", name);
        if (!name) {
            name = udev_device_get_property_value(parent, "NAME");
            LOG_PROPERTY(ppath, "NAME", name);
        }

        /* construct USB ID in lowercase hex - "0000:ffff" */
        if (product &&
            sscanf(product, "%*x/%4x/%4x/%*x", &usb_vendor, &usb_model) == 2) {
            char *usb_id;
            if (asprintf(&usb_id, "%04x:%04x", usb_vendor, usb_model)
                == -1)
                usb_id = NULL;
            else
                LOG_PROPERTY(ppath, "PRODUCT", product);
            attrs.usb_id = usb_id;
        }

        while (!pnp_id && (parent = udev_device_get_parent(parent))) {
            pnp_id = udev_device_get_sysattr_value(parent, "id");
            if (!pnp_id)
                continue;

            attrs.pnp_id = strdup(pnp_id);
            ppath = udev_device_get_devnode(parent);
            LOG_SYSATTR(ppath, "id", pnp_id);
        }

    }
    if (!name)
        name = "(unnamed)";
    else
        attrs.product = strdup(name);
    input_options = input_option_new(input_options, "name", name);
    input_options = input_option_new(input_options, "path", path);
    input_options = input_option_new(input_options, "device", path);
    input_options = input_option_new(input_options, "major", itoa(major(devnum)));
    input_options = input_option_new(input_options, "minor", itoa(minor(devnum)));
    if (path)
        attrs.device = strdup(path);

    tags_prop = udev_device_get_property_value(udev_device, "ID_INPUT.tags");
    LOG_PROPERTY(path, "ID_INPUT.tags", tags_prop);
    attrs.tags = xstrtokenize(tags_prop, ",");

    if (asprintf(&config_info, "udev:%s", syspath) == -1) {
        config_info = NULL;
        goto unwind;
    }

    if (device_is_duplicate(config_info)) {
        LogMessage(X_WARNING, "config/udev: device %s already added. "
                   "Ignoring.\n", name);
        goto unwind;
    }

    set = udev_device_get_properties_list_entry(udev_device);
    udev_list_entry_foreach(entry, set) {
        key = udev_list_entry_get_name(entry);
        if (!key)
            continue;
        value = udev_list_entry_get_value(entry);
        if (!strncasecmp(key, UDEV_XKB_PROP_KEY, sizeof(UDEV_XKB_PROP_KEY) - 1)) {
            LOG_PROPERTY(path, key, value);
            tmp = key + sizeof(UDEV_XKB_PROP_KEY) - 1;
            if (!strcasecmp(tmp, "rules"))
                input_options =
                    input_option_new(input_options, "xkb_rules", value);
            else if (!strcasecmp(tmp, "layout"))
                input_options =
                    input_option_new(input_options, "xkb_layout", value);
            else if (!strcasecmp(tmp, "variant"))
                input_options =
                    input_option_new(input_options, "xkb_variant", value);
            else if (!strcasecmp(tmp, "model"))
                input_options =
                    input_option_new(input_options, "xkb_model", value);
            else if (!strcasecmp(tmp, "options"))
                input_options =
                    input_option_new(input_options, "xkb_options", value);
        }
        else if (!strcmp(key, "ID_VENDOR")) {
            LOG_PROPERTY(path, key, value);
            attrs.vendor = strdup(value);
        } else if (!strncmp(key, "ID_INPUT_", 9)) {
            const struct pfmap {
                const char *property;
                unsigned int flag;
            } map[] = {
                { "ID_INPUT_KEY", ATTR_KEY },
                { "ID_INPUT_KEYBOARD", ATTR_KEYBOARD },
                { "ID_INPUT_MOUSE", ATTR_POINTER },
                { "ID_INPUT_JOYSTICK", ATTR_JOYSTICK },
                { "ID_INPUT_TABLET", ATTR_TABLET },
                { "ID_INPUT_TABLET_PAD", ATTR_TABLET_PAD },
                { "ID_INPUT_TOUCHPAD", ATTR_TOUCHPAD },
                { "ID_INPUT_TOUCHSCREEN", ATTR_TOUCHSCREEN },
                { NULL, 0 },
            };

            /* Anything but the literal string "0" is considered a
             * boolean true. The empty string isn't a thing with udev
             * properties anyway */
            if (value && strcmp(value, "0")) {
                const struct pfmap *m = map;

                while (m->property != NULL) {
                    if (!strcmp(m->property, key)) {
                        LOG_PROPERTY(path, key, value);
                        attrs.flags |= m->flag;
                    }
                    m++;
                }
            }
        }
    }

    input_options = input_option_new(input_options, "config_info", config_info);

    /* Default setting needed for non-seat0 seats */
    if (ServerIsNotSeat0())
        input_options = input_option_new(input_options, "GrabDevice", "on");

    LogMessage(X_INFO, "config/udev: Adding input device %s (%s)\n",
               name, path);
    rc = NewInputDeviceRequest(input_options, &attrs, &dev);
    if (rc != Success)
        goto unwind;

 unwind:
    free(config_info);
    input_option_free_list(&input_options);

    free(attrs.usb_id);
    free(attrs.pnp_id);
    free(attrs.product);
    free(attrs.device);
    free(attrs.vendor);
    if (attrs.tags) {
        char **tag = attrs.tags;

        while (*tag) {
            free(*tag);
            tag++;
        }
        free(attrs.tags);
    }

    return;
}

static void
device_removed(struct udev_device *device)
{
    char *value;
    const char *syspath = udev_device_get_syspath(device);

#ifdef CONFIG_UDEV_KMS
    const char *subsys = udev_device_get_subsystem(device);

    if (subsys && !strcmp(subsys, "drm")) {
        const char *sysname = udev_device_get_sysname(device);
        const char *path = udev_device_get_devnode(device);
        dev_t devnum = udev_device_get_devnum(device);

        if ((strncmp(sysname,"card", 4) != 0) || (path == NULL))
            return;

        LogMessage(X_INFO, "config/udev: removing GPU device %s %s\n",
                   syspath, path);
        config_udev_odev_setup_attribs(device, path, syspath, major(devnum),
                                       minor(devnum), DeleteGPUDeviceRequest);
        /* Retry vtenter after a drm node removal */
        systemd_logind_vtenter();
        return;
    }
#endif

    if (asprintf(&value, "udev:%s", syspath) == -1)
        return;

    remove_devices("udev", value);

    free(value);
}

static void
socket_handler(int fd, int ready, void *data)
{
    struct udev_device *udev_device;
    const char *action;

    input_lock();
    udev_device = udev_monitor_receive_device(udev_monitor);
    if (!udev_device) {
        input_unlock();
        return;
    }
    action = udev_device_get_action(udev_device);
    if (action) {
        if (!strcmp(action, "add")) {
            device_removed(udev_device);
            device_added(udev_device);
        } else if (!strcmp(action, "change")) {
            /* ignore change for the drm devices */
            const char *subsys = udev_device_get_subsystem(udev_device);

            if (subsys && strcmp(subsys, "drm")) {
                device_removed(udev_device);
                device_added(udev_device);
            }
        }
        else if (!strcmp(action, "remove"))
            device_removed(udev_device);
    }
    udev_device_unref(udev_device);
    input_unlock();
}

int
config_udev_pre_init(void)
{
    struct udev *udev;

    udev = udev_new();
    if (!udev)
        return 0;

    udev_monitor = udev_monitor_new_from_netlink(udev, "udev");
    if (!udev_monitor)
        return 0;

    udev_monitor_filter_add_match_subsystem_devtype(udev_monitor, "input",
                                                    NULL);
    /* For Wacom serial devices */
    udev_monitor_filter_add_match_subsystem_devtype(udev_monitor, "tty", NULL);
#ifdef CONFIG_UDEV_KMS
    /* For output GPU devices */
    udev_monitor_filter_add_match_subsystem_devtype(udev_monitor, "drm", NULL);
#endif

#ifdef HAVE_UDEV_MONITOR_FILTER_ADD_MATCH_TAG
    if (ServerIsNotSeat0())
        udev_monitor_filter_add_match_tag(udev_monitor, SeatId);
#endif
    if (udev_monitor_enable_receiving(udev_monitor)) {
        ErrorF("config/udev: failed to bind the udev monitor\n");
        return 0;
    }
    return 1;
}

int
config_udev_init(void)
{
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *device;

    udev = udev_monitor_get_udev(udev_monitor);
    enumerate = udev_enumerate_new(udev);
    if (!enumerate)
        return 0;

    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_add_match_subsystem(enumerate, "tty");
#ifdef CONFIG_UDEV_KMS
    udev_enumerate_add_match_subsystem(enumerate, "drm");
#endif

#ifdef HAVE_UDEV_ENUMERATE_ADD_MATCH_TAG
    if (ServerIsNotSeat0())
        udev_enumerate_add_match_tag(enumerate, SeatId);
#endif

    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(device, devices) {
        const char *syspath = udev_list_entry_get_name(device);
        struct udev_device *udev_device =
            udev_device_new_from_syspath(udev, syspath);

        /* Device might be gone by the time we try to open it */
        if (!udev_device)
            continue;

        device_added(udev_device);
        udev_device_unref(udev_device);
    }
    udev_enumerate_unref(enumerate);

    SetNotifyFd(udev_monitor_get_fd(udev_monitor), socket_handler, X_NOTIFY_READ, NULL);

    return 1;
}

void
config_udev_fini(void)
{
    struct udev *udev;

    if (!udev_monitor)
        return;

    udev = udev_monitor_get_udev(udev_monitor);

    RemoveNotifyFd(udev_monitor_get_fd(udev_monitor));
    udev_monitor_unref(udev_monitor);
    udev_monitor = NULL;
    udev_unref(udev);
}

#ifdef CONFIG_UDEV_KMS

/* Find the last occurrence of the needle in haystack */
static char *strrstr(const char *haystack, const char *needle)
{
    char *prev, *last, *tmp;

    prev = strstr(haystack, needle);
    if (!prev)
        return NULL;

    last = prev;
    tmp = prev + 1;

    while (tmp) {
        last = strstr(tmp, needle);
        if (!last)
            return prev;
        else {
            prev = last;
            tmp = prev + 1;
        }
    }

    return last;
}

/* For certain devices udev does not create ID_PATH entry (which is presumably a bug
 * in udev). We work around that by implementing a minimal ID_PATH calculator
 * ourselves along the same logic that udev uses. This works only for the case of
 * a PCI device being directly connected to a PCI bus, but it will cover most end
 * users with e.g. a new laptop which only has beta hardware driver support.
 * See https://gitlab.freedesktop.org/xorg/xserver/-/issues/993 */
static char*
config_udev_get_fallback_bus_id(struct udev_device *udev_device)
{
    const char *sysname;
    char *busid;

    udev_device = udev_device_get_parent(udev_device);
    if (udev_device == NULL)
        return NULL;

    if (strcmp(udev_device_get_subsystem(udev_device), "pci") != 0)
        return NULL;

    sysname = udev_device_get_sysname(udev_device);
    busid = XNFalloc(strlen(sysname) + 5);
    busid[0] = '\0';
    strcat(busid, "pci:");
    strcat(busid, sysname);

    return busid;
}

static void
config_udev_odev_setup_attribs(struct udev_device *udev_device, const char *path, const char *syspath,
                               int major, int minor,
                               config_odev_probe_proc_ptr probe_callback)
{
    struct OdevAttributes *attribs = config_odev_allocate_attributes();
    const char *value, *str;

    attribs->path = XNFstrdup(path);
    attribs->syspath = XNFstrdup(syspath);
    attribs->major = major;
    attribs->minor = minor;

    value = udev_device_get_property_value(udev_device, "ID_PATH");
    if (value && (str = strrstr(value, "pci-"))) {
        value = str;

        if ((str = strstr(value, "usb-")))
            value = str;

        attribs->busid = XNFstrdup(value);
        attribs->busid[3] = ':';
    }

    if (!value)
        attribs->busid = config_udev_get_fallback_bus_id(udev_device);

    /* ownership of attribs is passed to probe layer */
    probe_callback(attribs);
}

void
config_udev_odev_probe(config_odev_probe_proc_ptr probe_callback)
{
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *device;

    udev = udev_monitor_get_udev(udev_monitor);
    enumerate = udev_enumerate_new(udev);
    if (!enumerate)
        return;

    udev_enumerate_add_match_subsystem(enumerate, "drm");
    udev_enumerate_add_match_sysname(enumerate, "card[0-9]*");
#ifdef HAVE_UDEV_ENUMERATE_ADD_MATCH_TAG
    if (ServerIsNotSeat0())
        udev_enumerate_add_match_tag(enumerate, SeatId);
#endif
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(device, devices) {
        const char *syspath = udev_list_entry_get_name(device);
        struct udev_device *udev_device = udev_device_new_from_syspath(udev, syspath);
        const char *path = udev_device_get_devnode(udev_device);
        const char *sysname = udev_device_get_sysname(udev_device);
        dev_t devnum = udev_device_get_devnum(udev_device);
        const char *subsys = udev_device_get_subsystem(udev_device);

        if (!path || !syspath || !subsys)
            goto no_probe;
        else if (strcmp(subsys, "drm") != 0)
            goto no_probe;
        else if (strncmp(sysname, "card", 4) != 0)
            goto no_probe;
        else if (!check_seat(udev_device))
            goto no_probe;

        config_udev_odev_setup_attribs(udev_device, path, syspath, major(devnum),
                                       minor(devnum), probe_callback);
    no_probe:
        udev_device_unref(udev_device);
    }
    udev_enumerate_unref(enumerate);
    return;
}
#endif

