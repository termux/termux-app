
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"

#ifdef HAVE_ACPI
extern PMClose lnxACPIOpen(void);
#endif

#ifdef HAVE_APM

#include <linux/apm_bios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define APM_PROC   "/proc/apm"
#define APM_DEVICE "/dev/apm_bios"

#ifndef APM_STANDBY_FAILED
#define APM_STANDBY_FAILED 0xf000
#endif
#ifndef APM_SUSPEND_FAILED
#define APM_SUSPEND_FAILED 0xf001
#endif

static PMClose lnxAPMOpen(void);
static void lnxCloseAPM(void);
static void *APMihPtr = NULL;

static struct {
    apm_event_t apmLinux;
    pmEvent xf86;
} LinuxToXF86[] = {
    {APM_SYS_STANDBY, XF86_APM_SYS_STANDBY},
    {APM_SYS_SUSPEND, XF86_APM_SYS_SUSPEND},
    {APM_NORMAL_RESUME, XF86_APM_NORMAL_RESUME},
    {APM_CRITICAL_RESUME, XF86_APM_CRITICAL_RESUME},
    {APM_LOW_BATTERY, XF86_APM_LOW_BATTERY},
    {APM_POWER_STATUS_CHANGE, XF86_APM_POWER_STATUS_CHANGE},
    {APM_UPDATE_TIME, XF86_APM_UPDATE_TIME},
    {APM_CRITICAL_SUSPEND, XF86_APM_CRITICAL_SUSPEND},
    {APM_USER_STANDBY, XF86_APM_USER_STANDBY},
    {APM_USER_SUSPEND, XF86_APM_USER_SUSPEND},
    {APM_STANDBY_RESUME, XF86_APM_STANDBY_RESUME},
#if defined(APM_CAPABILITY_CHANGED)
    {APM_CAPABILITY_CHANGED, XF86_CAPABILITY_CHANGED},
#endif
#if 0
    {APM_STANDBY_FAILED, XF86_APM_STANDBY_FAILED},
    {APM_SUSPEND_FAILED, XF86_APM_SUSPEND_FAILED}
#endif
};

/*
 * APM is still under construction.
 * I'm not sure if the places where I initialize/deinitialize
 * apm is correct. Also I don't know what to do in SETUP state.
 * This depends if wakeup gets called in this situation, too.
 * Also we need to check if the action that is taken on an
 * event is reasonable.
 */
static int
lnxPMGetEventFromOs(int fd, pmEvent * events, int num)
{
    int i, j, n;
    apm_event_t linuxEvents[8];

    if ((n = read(fd, linuxEvents, num * sizeof(apm_event_t))) == -1)
        return 0;
    n /= sizeof(apm_event_t);
    if (n > num)
        n = num;
    for (i = 0; i < n; i++) {
        for (j = 0; j < ARRAY_SIZE(LinuxToXF86); j++)
            if (LinuxToXF86[j].apmLinux == linuxEvents[i]) {
                events[i] = LinuxToXF86[j].xf86;
                break;
            }
        if (j == ARRAY_SIZE(LinuxToXF86))
            events[i] = XF86_APM_UNKNOWN;
    }
    return n;
}

static pmWait
lnxPMConfirmEventToOs(int fd, pmEvent event)
{
    switch (event) {
    case XF86_APM_SYS_STANDBY:
    case XF86_APM_USER_STANDBY:
        if (ioctl(fd, APM_IOC_STANDBY, NULL))
            return PM_FAILED;
        return PM_CONTINUE;
    case XF86_APM_SYS_SUSPEND:
    case XF86_APM_CRITICAL_SUSPEND:
    case XF86_APM_USER_SUSPEND:
        if (ioctl(fd, APM_IOC_SUSPEND, NULL)) {
            /* I believe this is wrong (EE)
               EBUSY is sent when a device refuses to be suspended.
               In this case we still need to undo everything we have
               done to suspend ourselves or we will stay in suspended
               state forever. */
            if (errno == EBUSY)
                return PM_CONTINUE;
            else
                return PM_FAILED;
        }
        return PM_CONTINUE;
    case XF86_APM_STANDBY_RESUME:
    case XF86_APM_NORMAL_RESUME:
    case XF86_APM_CRITICAL_RESUME:
    case XF86_APM_STANDBY_FAILED:
    case XF86_APM_SUSPEND_FAILED:
        return PM_CONTINUE;
    default:
        return PM_NONE;
    }
}

#endif                          // HAVE_APM

PMClose
xf86OSPMOpen(void)
{
    PMClose ret = NULL;

#ifdef HAVE_ACPI
    /* Favour ACPI over APM, but only when enabled */

    if (!xf86acpiDisableFlag) {
        ret = lnxACPIOpen();
        if (ret)
            return ret;
    }
#endif
#ifdef HAVE_APM
    ret = lnxAPMOpen();
#endif

    return ret;
}

#ifdef HAVE_APM

static PMClose
lnxAPMOpen(void)
{
    int fd, pfd;

    DebugF("APM: OSPMOpen called\n");
    if (APMihPtr || !xf86Info.pmFlag)
        return NULL;

    DebugF("APM: Opening device\n");
    if ((fd = open(APM_DEVICE, O_RDWR)) > -1) {
        if (access(APM_PROC, R_OK) || ((pfd = open(APM_PROC, O_RDONLY)) == -1)) {
            xf86MsgVerb(X_WARNING, 3, "Cannot open APM (%s) (%s)\n",
                        APM_PROC, strerror(errno));
            close(fd);
            return NULL;
        }
        else
            close(pfd);
        xf86PMGetEventFromOs = lnxPMGetEventFromOs;
        xf86PMConfirmEventToOs = lnxPMConfirmEventToOs;
        APMihPtr = xf86AddGeneralHandler(fd, xf86HandlePMEvents, NULL);
        xf86MsgVerb(X_INFO, 3, "Open APM successful\n");
        return lnxCloseAPM;
    }
    return NULL;
}

static void
lnxCloseAPM(void)
{
    int fd;

    DebugF("APM: Closing device\n");
    if (APMihPtr) {
        fd = xf86RemoveGeneralHandler(APMihPtr);
        close(fd);
        APMihPtr = NULL;
    }
}

#endif                          // HAVE_APM
