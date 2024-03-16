/*
 * Copyright © 2013 Red Hat Inc.
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
 * Author: Hans de Goede <hdegoede@redhat.com>
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <dbus/dbus.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "os.h"
#include "dbus-core.h"
#include "linux.h"
#include "xf86.h"
#include "xf86platformBus.h"
#include "xf86Xinput.h"
#include "xf86Priv.h"
#include "globals.h"

#include "systemd-logind.h"

struct systemd_logind_info {
    DBusConnection *conn;
    char *session;
    Bool active;
    Bool vt_active;
};

static struct systemd_logind_info logind_info;

static InputInfoPtr
systemd_logind_find_info_ptr_by_devnum(InputInfoPtr start,
                                       int major, int minor)
{
    InputInfoPtr pInfo;

    for (pInfo = start; pInfo; pInfo = pInfo->next)
        if (pInfo->major == major && pInfo->minor == minor &&
                (pInfo->flags & XI86_SERVER_FD))
            return pInfo;

    return NULL;
}

static void
systemd_logind_set_input_fd_for_all_devs(int major, int minor, int fd,
                                         Bool enable)
{
    InputInfoPtr pInfo;

    pInfo = systemd_logind_find_info_ptr_by_devnum(xf86InputDevs, major, minor);
    while (pInfo) {
        pInfo->fd = fd;
        pInfo->options = xf86ReplaceIntOption(pInfo->options, "fd", fd);
        if (enable)
            xf86EnableInputDeviceForVTSwitch(pInfo);

        pInfo = systemd_logind_find_info_ptr_by_devnum(pInfo->next, major, minor);
    }
}

int
systemd_logind_take_fd(int _major, int _minor, const char *path,
                       Bool *paused_ret)
{
    struct systemd_logind_info *info = &logind_info;
    InputInfoPtr pInfo;
    DBusError error;
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    dbus_int32_t major = _major;
    dbus_int32_t minor = _minor;
    dbus_bool_t paused;
    int fd = -1;

    if (!info->session || major == 0)
        return -1;

    /* logind does not support mouse devs (with evdev we don't need them) */
    if (strstr(path, "mouse"))
        return -1;

    /* Check if we already have an InputInfo entry with this major, minor
     * (shared device-nodes happen ie with Wacom tablets). */
    pInfo = systemd_logind_find_info_ptr_by_devnum(xf86InputDevs, major, minor);
    if (pInfo) {
        LogMessage(X_INFO, "systemd-logind: returning pre-existing fd for %s %u:%u\n",
               path, major, minor);
        *paused_ret = FALSE;
        return pInfo->fd;
    }

    dbus_error_init(&error);

    msg = dbus_message_new_method_call("org.freedesktop.login1", info->session,
            "org.freedesktop.login1.Session", "TakeDevice");
    if (!msg) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    if (!dbus_message_append_args(msg, DBUS_TYPE_UINT32, &major,
                                       DBUS_TYPE_UINT32, &minor,
                                       DBUS_TYPE_INVALID)) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    reply = dbus_connection_send_with_reply_and_block(info->conn, msg,
                                                      DBUS_TIMEOUT_USE_DEFAULT, &error);
    if (!reply) {
        LogMessage(X_ERROR, "systemd-logind: failed to take device %s: %s\n",
                   path, error.message);
        goto cleanup;
    }

    if (!dbus_message_get_args(reply, &error,
                               DBUS_TYPE_UNIX_FD, &fd,
                               DBUS_TYPE_BOOLEAN, &paused,
                               DBUS_TYPE_INVALID)) {
        LogMessage(X_ERROR, "systemd-logind: TakeDevice %s: %s\n",
                   path, error.message);
        goto cleanup;
    }

    *paused_ret = paused;

    LogMessage(X_INFO, "systemd-logind: got fd for %s %u:%u fd %d paused %d\n",
               path, major, minor, fd, paused);

cleanup:
    if (msg)
        dbus_message_unref(msg);
    if (reply)
        dbus_message_unref(reply);
    dbus_error_free(&error);

    return fd;
}

void
systemd_logind_release_fd(int _major, int _minor, int fd)
{
    struct systemd_logind_info *info = &logind_info;
    InputInfoPtr pInfo;
    DBusError error;
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    dbus_int32_t major = _major;
    dbus_int32_t minor = _minor;
    int matches = 0;

    if (!info->session || major == 0)
        goto close;

    /* Only release the fd if there is only 1 InputInfo left for this major
     * and minor, otherwise other InputInfo's are still referencing the fd. */
    pInfo = systemd_logind_find_info_ptr_by_devnum(xf86InputDevs, major, minor);
    while (pInfo) {
        matches++;
        pInfo = systemd_logind_find_info_ptr_by_devnum(pInfo->next, major, minor);
    }
    if (matches > 1) {
        LogMessage(X_INFO, "systemd-logind: not releasing fd for %u:%u, still in use\n", major, minor);
        return;
    }

    LogMessage(X_INFO, "systemd-logind: releasing fd for %u:%u\n", major, minor);

    dbus_error_init(&error);

    msg = dbus_message_new_method_call("org.freedesktop.login1", info->session,
            "org.freedesktop.login1.Session", "ReleaseDevice");
    if (!msg) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    if (!dbus_message_append_args(msg, DBUS_TYPE_UINT32, &major,
                                       DBUS_TYPE_UINT32, &minor,
                                       DBUS_TYPE_INVALID)) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    reply = dbus_connection_send_with_reply_and_block(info->conn, msg,
                                                      DBUS_TIMEOUT_USE_DEFAULT, &error);
    if (!reply)
        LogMessage(X_ERROR, "systemd-logind: failed to release device: %s\n",
                   error.message);

cleanup:
    if (msg)
        dbus_message_unref(msg);
    if (reply)
        dbus_message_unref(reply);
    dbus_error_free(&error);
close:
    if (fd != -1)
        close(fd);
}

int
systemd_logind_controls_session(void)
{
    return logind_info.session ? 1 : 0;
}

void
systemd_logind_vtenter(void)
{
    struct systemd_logind_info *info = &logind_info;
    InputInfoPtr pInfo;
    int i;

    if (!info->session)
        return; /* Not using systemd-logind */

    if (!info->active)
        return; /* Session not active */

    if (info->vt_active)
        return; /* Already did vtenter */

    for (i = 0; i < xf86_num_platform_devices; i++) {
        if (xf86_platform_devices[i].flags & XF86_PDEV_PAUSED)
            break;
    }
    if (i != xf86_num_platform_devices)
        return; /* Some drm nodes are still paused wait for resume */

    xf86VTEnter();
    info->vt_active = TRUE;

    /* Activate any input devices which were resumed before the drm nodes */
    for (pInfo = xf86InputDevs; pInfo; pInfo = pInfo->next)
        if ((pInfo->flags & XI86_SERVER_FD) && pInfo->fd != -1)
            xf86EnableInputDeviceForVTSwitch(pInfo);

    /* Do delayed input probing, this must be done after the above enabling */
    xf86InputEnableVTProbe();
}

static void
systemd_logind_ack_pause(struct systemd_logind_info *info,
                         dbus_int32_t minor, dbus_int32_t major)
{
    DBusError error;
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;

    dbus_error_init(&error);

    msg = dbus_message_new_method_call("org.freedesktop.login1", info->session,
            "org.freedesktop.login1.Session", "PauseDeviceComplete");
    if (!msg) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    if (!dbus_message_append_args(msg, DBUS_TYPE_UINT32, &major,
                                       DBUS_TYPE_UINT32, &minor,
                                       DBUS_TYPE_INVALID)) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    reply = dbus_connection_send_with_reply_and_block(info->conn, msg,
                                                      DBUS_TIMEOUT_USE_DEFAULT, &error);
    if (!reply)
        LogMessage(X_ERROR, "systemd-logind: failed to ack pause: %s\n",
                   error.message);

cleanup:
    if (msg)
        dbus_message_unref(msg);
    if (reply)
        dbus_message_unref(reply);
    dbus_error_free(&error);
}

/*
 * Send a message to logind, to pause the drm device
 * and ensure the drm_drop_master is done before
 * VT_RELDISP when switching VT
 */
void systemd_logind_drop_master(void)
{
    int i;
    for (i = 0; i < xf86_num_platform_devices; i++) {
        if (xf86_platform_devices[i].flags & XF86_PDEV_SERVER_FD) {
            dbus_int32_t major, minor;
            struct systemd_logind_info *info = &logind_info;

            xf86_platform_devices[i].flags |= XF86_PDEV_PAUSED;
            major = xf86_platform_odev_attributes(i)->major;
            minor = xf86_platform_odev_attributes(i)->minor;
            systemd_logind_ack_pause(info, minor, major);
        }
    }
}

static Bool are_platform_devices_resumed(void) {
    int i;
    for (i = 0; i < xf86_num_platform_devices; i++) {
        if (xf86_platform_devices[i].flags & XF86_PDEV_PAUSED) {
            return FALSE;
        }
    }
    return TRUE;
}

static DBusHandlerResult
message_filter(DBusConnection * connection, DBusMessage * message, void *data)
{
    struct systemd_logind_info *info = data;
    struct xf86_platform_device *pdev = NULL;
    InputInfoPtr pInfo = NULL;
    int ack = 0, pause = 0, fd = -1;
    DBusError error;
    dbus_int32_t major, minor;
    char *pause_str;

    if (strcmp(dbus_message_get_path(message), info->session) != 0)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    dbus_error_init(&error);

    if (dbus_message_is_signal(message, "org.freedesktop.login1.Session",
                               "PauseDevice")) {
        if (!dbus_message_get_args(message, &error,
                               DBUS_TYPE_UINT32, &major,
                               DBUS_TYPE_UINT32, &minor,
                               DBUS_TYPE_STRING, &pause_str,
                               DBUS_TYPE_INVALID)) {
            LogMessage(X_ERROR, "systemd-logind: PauseDevice: %s\n",
                       error.message);
            dbus_error_free(&error);
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        if (strcmp(pause_str, "pause") == 0) {
            pause = 1;
            ack = 1;
        }
        else if (strcmp(pause_str, "force") == 0) {
            pause = 1;
        }
        else if (strcmp(pause_str, "gone") == 0) {
            /* Device removal is handled through udev */
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        else {
            LogMessage(X_WARNING, "systemd-logind: unknown pause type: %s\n",
                       pause_str);
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
    }
    else if (dbus_message_is_signal(message, "org.freedesktop.login1.Session",
                                    "ResumeDevice")) {
        if (!dbus_message_get_args(message, &error,
                                   DBUS_TYPE_UINT32, &major,
                                   DBUS_TYPE_UINT32, &minor,
                                   DBUS_TYPE_UNIX_FD, &fd,
                                   DBUS_TYPE_INVALID)) {
            LogMessage(X_ERROR, "systemd-logind: ResumeDevice: %s\n",
                       error.message);
            dbus_error_free(&error);
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
    } else
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    LogMessage(X_INFO, "systemd-logind: got %s for %u:%u\n",
               pause ? "pause" : "resume", major, minor);

    pdev = xf86_find_platform_device_by_devnum(major, minor);
    if (!pdev)
        pInfo = systemd_logind_find_info_ptr_by_devnum(xf86InputDevs,
                                                       major, minor);
    if (!pdev && !pInfo) {
        LogMessage(X_WARNING, "systemd-logind: could not find dev %u:%u\n",
                   major, minor);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (pause) {
        /* Our VT_PROCESS usage guarantees we've already given up the vt */
        info->active = info->vt_active = FALSE;
        /* Note the actual vtleave has already been handled by xf86Events.c */
        if (pdev)
            pdev->flags |= XF86_PDEV_PAUSED;
        else {
            close(pInfo->fd);
            systemd_logind_set_input_fd_for_all_devs(major, minor, -1, FALSE);
        }
        if (ack)
            systemd_logind_ack_pause(info, major, minor);
    }
    else {
        /* info->vt_active gets set by systemd_logind_vtenter() */
        info->active = TRUE;

        if (pdev) {
            pdev->flags &= ~XF86_PDEV_PAUSED;
        } else
            systemd_logind_set_input_fd_for_all_devs(major, minor, fd,
                                                     info->vt_active);
        /* Call vtenter if all platform devices are resumed, or if there are no platform device */
        if (are_platform_devices_resumed())
            systemd_logind_vtenter();
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}

static void
connect_hook(DBusConnection *connection, void *data)
{
    struct systemd_logind_info *info = data;
    DBusError error;
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    dbus_int32_t arg;
    char *session = NULL;

    dbus_error_init(&error);

    msg = dbus_message_new_method_call("org.freedesktop.login1",
            "/org/freedesktop/login1", "org.freedesktop.login1.Manager",
            "GetSessionByPID");
    if (!msg) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    arg = getpid();
    if (!dbus_message_append_args(msg, DBUS_TYPE_UINT32, &arg,
                                  DBUS_TYPE_INVALID)) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    reply = dbus_connection_send_with_reply_and_block(connection, msg,
                                                      DBUS_TIMEOUT_USE_DEFAULT, &error);
    if (!reply) {
        LogMessage(X_ERROR, "systemd-logind: failed to get session: %s\n",
                   error.message);
        goto cleanup;
    }
    dbus_message_unref(msg);

    if (!dbus_message_get_args(reply, &error, DBUS_TYPE_OBJECT_PATH, &session,
                               DBUS_TYPE_INVALID)) {
        LogMessage(X_ERROR, "systemd-logind: GetSessionByPID: %s\n",
                   error.message);
        goto cleanup;
    }
    session = XNFstrdup(session);

    dbus_message_unref(reply);
    reply = NULL;


    msg = dbus_message_new_method_call("org.freedesktop.login1",
            session, "org.freedesktop.login1.Session", "TakeControl");
    if (!msg) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    arg = FALSE; /* Don't forcibly take over over the session */
    if (!dbus_message_append_args(msg, DBUS_TYPE_BOOLEAN, &arg,
                                  DBUS_TYPE_INVALID)) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    reply = dbus_connection_send_with_reply_and_block(connection, msg,
                                                      DBUS_TIMEOUT_USE_DEFAULT, &error);
    if (!reply) {
        LogMessage(X_ERROR, "systemd-logind: TakeControl failed: %s\n",
                   error.message);
        goto cleanup;
    }

    dbus_bus_add_match(connection,
        "type='signal',sender='org.freedesktop.login1',interface='org.freedesktop.login1.Session',member='PauseDevice'",
        &error);
    if (dbus_error_is_set(&error)) {
        LogMessage(X_ERROR, "systemd-logind: could not add match: %s\n",
                   error.message);
        goto cleanup;
    }

    dbus_bus_add_match(connection,
        "type='signal',sender='org.freedesktop.login1',interface='org.freedesktop.login1.Session',member='ResumeDevice'",
        &error);
    if (dbus_error_is_set(&error)) {
        LogMessage(X_ERROR, "systemd-logind: could not add match: %s\n",
                   error.message);
        goto cleanup;
    }

    /*
     * HdG: This is not useful with systemd <= 208 since the signal only
     * contains invalidated property names there, rather than property, val
     * pairs as it should.  Instead we just use the first resume / pause now.
     */
#if 0
    snprintf(match, sizeof(match),
        "type='signal',sender='org.freedesktop.login1',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',path='%s'",
        session);
    dbus_bus_add_match(connection, match, &error);
    if (dbus_error_is_set(&error)) {
        LogMessage(X_ERROR, "systemd-logind: could not add match: %s\n",
                   error.message);
        goto cleanup;
    }
#endif

    if (!dbus_connection_add_filter(connection, message_filter, info, NULL)) {
        LogMessage(X_ERROR, "systemd-logind: could not add filter: %s\n",
                   error.message);
        goto cleanup;
    }

    LogMessage(X_INFO, "systemd-logind: took control of session %s\n",
               session);
    info->conn = connection;
    info->session = session;
    info->vt_active = info->active = TRUE; /* The server owns the vt during init */
    session = NULL;

cleanup:
    free(session);
    if (msg)
        dbus_message_unref(msg);
    if (reply)
        dbus_message_unref(reply);
    dbus_error_free(&error);
}

static void
systemd_logind_release_control(struct systemd_logind_info *info)
{
    DBusError error;
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;

    dbus_error_init(&error);

    msg = dbus_message_new_method_call("org.freedesktop.login1",
            info->session, "org.freedesktop.login1.Session", "ReleaseControl");
    if (!msg) {
        LogMessage(X_ERROR, "systemd-logind: out of memory\n");
        goto cleanup;
    }

    reply = dbus_connection_send_with_reply_and_block(info->conn, msg,
                                                      DBUS_TIMEOUT_USE_DEFAULT, &error);
    if (!reply) {
        LogMessage(X_ERROR, "systemd-logind: ReleaseControl failed: %s\n",
                   error.message);
        goto cleanup;
    }

cleanup:
    if (msg)
        dbus_message_unref(msg);
    if (reply)
        dbus_message_unref(reply);
    dbus_error_free(&error);
}

static void
disconnect_hook(void *data)
{
    struct systemd_logind_info *info = data;

    free(info->session);
    info->session = NULL;
    info->conn = NULL;
}

static struct dbus_core_hook core_hook = {
    .connect = connect_hook,
    .disconnect = disconnect_hook,
    .data = &logind_info,
};

int
systemd_logind_init(void)
{
    if (!ServerIsNotSeat0() && xf86HasTTYs() && linux_parse_vt_settings(TRUE) && !linux_get_keeptty()) {
        LogMessage(X_INFO,
            "systemd-logind: logind integration requires -keeptty and "
            "-keeptty was not provided, disabling logind integration\n");
        return 1;
    }

    return dbus_core_add_hook(&core_hook);
}

void
systemd_logind_fini(void)
{
    if (logind_info.session)
        systemd_logind_release_control(&logind_info);

    dbus_core_remove_hook(&core_hook);
}
