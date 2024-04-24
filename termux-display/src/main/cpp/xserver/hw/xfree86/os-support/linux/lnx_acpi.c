#ifdef HAVE_XORG_CONFIG_H
#include "xorg-config.h"
#endif

#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define ACPI_SOCKET  "/var/run/acpid.socket"

#define ACPI_VIDEO_NOTIFY_SWITCH	0x80
#define ACPI_VIDEO_NOTIFY_PROBE		0x81
#define ACPI_VIDEO_NOTIFY_CYCLE		0x82
#define ACPI_VIDEO_NOTIFY_NEXT_OUTPUT	0x83
#define ACPI_VIDEO_NOTIFY_PREV_OUTPUT	0x84

#define ACPI_VIDEO_NOTIFY_CYCLE_BRIGHTNESS	0x85
#define	ACPI_VIDEO_NOTIFY_INC_BRIGHTNESS	0x86
#define ACPI_VIDEO_NOTIFY_DEC_BRIGHTNESS	0x87
#define ACPI_VIDEO_NOTIFY_ZERO_BRIGHTNESS	0x88
#define ACPI_VIDEO_NOTIFY_DISPLAY_OFF		0x89

#define ACPI_VIDEO_HEAD_INVALID		(~0u - 1)
#define ACPI_VIDEO_HEAD_END		(~0u)

static void lnxCloseACPI(void);
static void *ACPIihPtr = NULL;
PMClose lnxACPIOpen(void);

/* in milliseconds */
#define ACPI_REOPEN_DELAY 1000

static CARD32
lnxACPIReopen(OsTimerPtr timer, CARD32 time, void *arg)
{
    if (lnxACPIOpen()) {
        TimerFree(timer);
        return 0;
    }

    return ACPI_REOPEN_DELAY;
}

#define LINE_LENGTH 80

static int
lnxACPIGetEventFromOs(int fd, pmEvent * events, int num)
{
    char ev[LINE_LENGTH];
    int n;

    memset(ev, 0, LINE_LENGTH);

    do {
        n = read(fd, ev, LINE_LENGTH);
    } while ((n == -1) && (errno == EAGAIN || errno == EINTR));

    if (n <= 0) {
        lnxCloseACPI();
        TimerSet(NULL, 0, ACPI_REOPEN_DELAY, lnxACPIReopen, NULL);
        return 0;
    }
    /* FIXME: this only processes the first read ACPI event & might break
     * with interrupted reads. */

    /* Check that we have a video event */
    if (!strncmp(ev, "video", 5)) {
        char *GFX = NULL;
        char *notify = NULL;
        char *data = NULL;      /* doesn't appear to be used in the kernel */
        unsigned long int notify_l;

        strtok(ev, " ");

        if (!(GFX = strtok(NULL, " ")))
            return 0;
#if 0
        ErrorF("GFX: %s\n", GFX);
#endif

        if (!(notify = strtok(NULL, " ")))
            return 0;
        notify_l = strtoul(notify, NULL, 16);
#if 0
        ErrorF("notify: 0x%lx\n", notify_l);
#endif

        if (!(data = strtok(NULL, " ")))
            return 0;
#if 0
        data_l = strtoul(data, NULL, 16);
        ErrorF("data: 0x%lx\n", data_l);
#endif

        /* Differentiate between events */
        switch (notify_l) {
        case ACPI_VIDEO_NOTIFY_SWITCH:
        case ACPI_VIDEO_NOTIFY_CYCLE:
        case ACPI_VIDEO_NOTIFY_NEXT_OUTPUT:
        case ACPI_VIDEO_NOTIFY_PREV_OUTPUT:
            events[0] = XF86_APM_CAPABILITY_CHANGED;
            return 1;
        case ACPI_VIDEO_NOTIFY_PROBE:
            return 0;
        default:
            return 0;
        }
    }

    return 0;
}

static pmWait
lnxACPIConfirmEventToOs(int fd, pmEvent event)
{
    /* No ability to send back to the kernel in ACPI */
    switch (event) {
    default:
        return PM_NONE;
    }
}

PMClose
lnxACPIOpen(void)
{
    int fd;
    struct sockaddr_un addr;
    int r = -1;
    static int warned = 0;

    DebugF("ACPI: OSPMOpen called\n");
    if (ACPIihPtr || !xf86Info.pmFlag)
        return NULL;

    DebugF("ACPI: Opening device\n");
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) > -1) {
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, ACPI_SOCKET);
        if ((r = connect(fd, (struct sockaddr *) &addr, sizeof(addr))) == -1) {
            if (!warned)
                xf86MsgVerb(X_WARNING, 3, "Open ACPI failed (%s) (%s)\n",
                            ACPI_SOCKET, strerror(errno));
            warned = 1;
            shutdown(fd, 2);
            close(fd);
            return NULL;
        }
    }

    xf86PMGetEventFromOs = lnxACPIGetEventFromOs;
    xf86PMConfirmEventToOs = lnxACPIConfirmEventToOs;
    ACPIihPtr = xf86AddGeneralHandler(fd, xf86HandlePMEvents, NULL);
    xf86MsgVerb(X_INFO, 3, "Open ACPI successful (%s)\n", ACPI_SOCKET);
    warned = 0;

    return lnxCloseACPI;
}

static void
lnxCloseACPI(void)
{
    int fd;

    DebugF("ACPI: Closing device\n");
    if (ACPIihPtr) {
        fd = xf86RemoveGeneralHandler(ACPIihPtr);
        shutdown(fd, 2);
        close(fd);
        ACPIihPtr = NULL;
    }
}
