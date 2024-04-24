/*
 * Copyright 1993-2003 by The XFree86 Project, Inc.
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
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include <xserver_poll.h>
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

static int
GetBaud(int baudrate)
{
#ifdef B300
    if (baudrate == 300)
        return B300;
#endif
#ifdef B1200
    if (baudrate == 1200)
        return B1200;
#endif
#ifdef B2400
    if (baudrate == 2400)
        return B2400;
#endif
#ifdef B4800
    if (baudrate == 4800)
        return B4800;
#endif
#ifdef B9600
    if (baudrate == 9600)
        return B9600;
#endif
#ifdef B19200
    if (baudrate == 19200)
        return B19200;
#endif
#ifdef B38400
    if (baudrate == 38400)
        return B38400;
#endif
#ifdef B57600
    if (baudrate == 57600)
        return B57600;
#endif
#ifdef B115200
    if (baudrate == 115200)
        return B115200;
#endif
#ifdef B230400
    if (baudrate == 230400)
        return B230400;
#endif
#ifdef B460800
    if (baudrate == 460800)
        return B460800;
#endif
    return 0;
}

int
xf86OpenSerial(XF86OptionPtr options)
{
    struct termios t;
    int fd, i;
    char *dev;

    dev = xf86SetStrOption(options, "Device", NULL);
    if (!dev) {
        xf86Msg(X_ERROR, "xf86OpenSerial: No Device specified.\n");
        return -1;
    }

    fd = xf86CheckIntOption(options, "fd", -1);

    if (fd == -1)
        SYSCALL(fd = open(dev, O_RDWR | O_NONBLOCK));

    if (fd == -1) {
        xf86Msg(X_ERROR,
                "xf86OpenSerial: Cannot open device %s\n\t%s.\n",
                dev, strerror(errno));
        free(dev);
        return -1;
    }

    if (!isatty(fd)) {
        /* Allow non-tty devices to be opened. */
        free(dev);
        return fd;
    }

    /* set up default port parameters */
    SYSCALL(tcgetattr(fd, &t));
    t.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR
                   | IGNCR | ICRNL | IXON);
    t.c_oflag &= ~OPOST;
    t.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t.c_cflag &= ~(CSIZE | PARENB);
    t.c_cflag |= CS8 | CLOCAL;

    cfsetispeed(&t, B9600);
    cfsetospeed(&t, B9600);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;

    SYSCALL(tcsetattr(fd, TCSANOW, &t));

    if (xf86SetSerial(fd, options) == -1) {
        SYSCALL(close(fd));
        free(dev);
        return -1;
    }

    SYSCALL(i = fcntl(fd, F_GETFL, 0));
    if (i == -1) {
        SYSCALL(close(fd));
        free(dev);
        return -1;
    }
    i &= ~O_NONBLOCK;
    SYSCALL(i = fcntl(fd, F_SETFL, i));
    if (i == -1) {
        SYSCALL(close(fd));
        free(dev);
        return -1;
    }
    free(dev);
    return fd;
}

int
xf86SetSerial(int fd, XF86OptionPtr options)
{
    struct termios t;
    int val;
    char *s;
    int baud, r;

    if (fd < 0)
        return -1;

    /* Don't try to set parameters for non-tty devices. */
    if (!isatty(fd))
        return 0;

    SYSCALL(tcgetattr(fd, &t));

    if ((val = xf86SetIntOption(options, "BaudRate", 0))) {
        if ((baud = GetBaud(val))) {
            cfsetispeed(&t, baud);
            cfsetospeed(&t, baud);
        }
        else {
            xf86Msg(X_ERROR, "Invalid Option BaudRate value: %d\n", val);
            return -1;
        }
    }

    if ((val = xf86SetIntOption(options, "StopBits", 0))) {
        switch (val) {
        case 1:
            t.c_cflag &= ~(CSTOPB);
            break;
        case 2:
            t.c_cflag |= CSTOPB;
            break;
        default:
            xf86Msg(X_ERROR, "Invalid Option StopBits value: %d\n", val);
            return -1;
            break;
        }
    }

    if ((val = xf86SetIntOption(options, "DataBits", 0))) {
        switch (val) {
        case 5:
            t.c_cflag &= ~(CSIZE);
            t.c_cflag |= CS5;
            break;
        case 6:
            t.c_cflag &= ~(CSIZE);
            t.c_cflag |= CS6;
            break;
        case 7:
            t.c_cflag &= ~(CSIZE);
            t.c_cflag |= CS7;
            break;
        case 8:
            t.c_cflag &= ~(CSIZE);
            t.c_cflag |= CS8;
            break;
        default:
            xf86Msg(X_ERROR, "Invalid Option DataBits value: %d\n", val);
            return -1;
            break;
        }
    }

    if ((s = xf86SetStrOption(options, "Parity", NULL))) {
        if (xf86NameCmp(s, "Odd") == 0) {
            t.c_cflag |= PARENB | PARODD;
        }
        else if (xf86NameCmp(s, "Even") == 0) {
            t.c_cflag |= PARENB;
            t.c_cflag &= ~(PARODD);
        }
        else if (xf86NameCmp(s, "None") == 0) {
            t.c_cflag &= ~(PARENB);
        }
        else {
            xf86Msg(X_ERROR, "Invalid Option Parity value: %s\n", s);
            free(s);
            return -1;
        }
        free(s);
    }

    if ((val = xf86SetIntOption(options, "Vmin", -1)) != -1) {
        t.c_cc[VMIN] = val;
    }
    if ((val = xf86SetIntOption(options, "Vtime", -1)) != -1) {
        t.c_cc[VTIME] = val;
    }

    if ((s = xf86SetStrOption(options, "FlowControl", NULL))) {
        xf86MarkOptionUsedByName(options, "FlowControl");
        if (xf86NameCmp(s, "Xoff") == 0) {
            t.c_iflag |= IXOFF;
        }
        else if (xf86NameCmp(s, "Xon") == 0) {
            t.c_iflag |= IXON;
        }
        else if (xf86NameCmp(s, "XonXoff") == 0) {
            t.c_iflag |= IXON | IXOFF;
        }
        else if (xf86NameCmp(s, "None") == 0) {
            t.c_iflag &= ~(IXON | IXOFF);
        }
        else {
            xf86Msg(X_ERROR, "Invalid Option FlowControl value: %s\n", s);
            free(s);
            return -1;
        }
        free(s);
    }

    if ((xf86SetBoolOption(options, "ClearDTR", FALSE))) {
#ifdef CLEARDTR_SUPPORT
#if defined(TIOCMBIC)
        val = TIOCM_DTR;
        SYSCALL(ioctl(fd, TIOCMBIC, &val));
#else
        SYSCALL(ioctl(fd, TIOCCDTR, NULL));
#endif
#else
        xf86Msg(X_WARNING, "Option ClearDTR not supported on this OS\n");
        return -1;
#endif
        xf86MarkOptionUsedByName(options, "ClearDTR");
    }

    if ((xf86SetBoolOption(options, "ClearRTS", FALSE))) {
        xf86Msg(X_WARNING, "Option ClearRTS not supported on this OS\n");
        return -1;
        xf86MarkOptionUsedByName(options, "ClearRTS");
    }

    SYSCALL(r = tcsetattr(fd, TCSANOW, &t));
    return r;
}

int
xf86SetSerialSpeed(int fd, int speed)
{
    struct termios t;
    int baud, r;

    if (fd < 0)
        return -1;

    /* Don't try to set parameters for non-tty devices. */
    if (!isatty(fd))
        return 0;

    SYSCALL(tcgetattr(fd, &t));

    if ((baud = GetBaud(speed))) {
        cfsetispeed(&t, baud);
        cfsetospeed(&t, baud);
    }
    else {
        xf86Msg(X_ERROR, "Invalid Option BaudRate value: %d\n", speed);
        return -1;
    }

    SYSCALL(r = tcsetattr(fd, TCSANOW, &t));
    return r;
}

int
xf86ReadSerial(int fd, void *buf, int count)
{
    int r;
    int i;

    SYSCALL(r = read(fd, buf, count));
    DebugF("ReadingSerial: 0x%x", (unsigned char) *(((unsigned char *) buf)));
    for (i = 1; i < r; i++)
        DebugF(", 0x%x", (unsigned char) *(((unsigned char *) buf) + i));
    DebugF("\n");
    return r;
}

int
xf86WriteSerial(int fd, const void *buf, int count)
{
    int r;
    int i;

    DebugF("WritingSerial: 0x%x", (unsigned char) *(((unsigned char *) buf)));
    for (i = 1; i < count; i++)
        DebugF(", 0x%x", (unsigned char) *(((unsigned char *) buf) + i));
    DebugF("\n");
    SYSCALL(r = write(fd, buf, count));
    return r;
}

int
xf86CloseSerial(int fd)
{
    int r;

    SYSCALL(r = close(fd));
    return r;
}

int
xf86WaitForInput(int fd, int timeout)
{
    int r;
    struct pollfd poll_fd;

    poll_fd.fd = fd;
    poll_fd.events = POLLIN;

    /* convert microseconds to milliseconds */
    timeout = (timeout + 999) / 1000;

    if (fd >= 0) {
        SYSCALL(r = xserver_poll(&poll_fd, 1, timeout));
    }
    else {
        SYSCALL(r = xserver_poll(&poll_fd, 0, timeout));
    }
    xf86ErrorFVerb(9, "poll returned %d\n", r);
    return r;
}

int
xf86SerialSendBreak(int fd, int duration)
{
    int r;

    SYSCALL(r = tcsendbreak(fd, duration));
    return r;

}

int
xf86FlushInput(int fd)
{
    struct pollfd poll_fd;
    /* this needs to be big enough to flush an evdev event. */
    char c[256];

    DebugF("FlushingSerial\n");
    if (tcflush(fd, TCIFLUSH) == 0)
        return 0;

    poll_fd.fd = fd;
    poll_fd.events = POLLIN;
    while (xserver_poll(&poll_fd, 1, 0) > 0) {
        if (read(fd, &c, sizeof(c)) < 1)
            return 0;
    }
    return 0;
}

static struct states {
    int xf;
    int os;
} modemStates[] = {
#ifdef TIOCM_LE
    {
    XF86_M_LE, TIOCM_LE},
#endif
#ifdef TIOCM_DTR
    {
    XF86_M_DTR, TIOCM_DTR},
#endif
#ifdef TIOCM_RTS
    {
    XF86_M_RTS, TIOCM_RTS},
#endif
#ifdef TIOCM_ST
    {
    XF86_M_ST, TIOCM_ST},
#endif
#ifdef TIOCM_SR
    {
    XF86_M_SR, TIOCM_SR},
#endif
#ifdef TIOCM_CTS
    {
    XF86_M_CTS, TIOCM_CTS},
#endif
#ifdef TIOCM_CAR
    {
    XF86_M_CAR, TIOCM_CAR},
#elif defined(TIOCM_CD)
    {
    XF86_M_CAR, TIOCM_CD},
#endif
#ifdef TIOCM_RNG
    {
    XF86_M_RNG, TIOCM_RNG},
#elif defined(TIOCM_RI)
    {
    XF86_M_CAR, TIOCM_RI},
#endif
#ifdef TIOCM_DSR
    {
    XF86_M_DSR, TIOCM_DSR},
#endif
};

static int numStates = ARRAY_SIZE(modemStates);

static int
xf2osState(int state)
{
    int i;
    int ret = 0;

    for (i = 0; i < numStates; i++)
        if (state & modemStates[i].xf)
            ret |= modemStates[i].os;
    return ret;
}

static int
os2xfState(int state)
{
    int i;
    int ret = 0;

    for (i = 0; i < numStates; i++)
        if (state & modemStates[i].os)
            ret |= modemStates[i].xf;
    return ret;
}

static int
getOsStateMask(void)
{
    int i;
    int ret = 0;

    for (i = 0; i < numStates; i++)
        ret |= modemStates[i].os;
    return ret;
}

static int osStateMask = 0;

int
xf86SetSerialModemState(int fd, int state)
{
    int ret;
    int s;

    if (fd < 0)
        return -1;

    /* Don't try to set parameters for non-tty devices. */
    if (!isatty(fd))
        return 0;

#ifndef TIOCMGET
    return -1;
#else
    if (!osStateMask)
        osStateMask = getOsStateMask();

    state = xf2osState(state);
    SYSCALL((ret = ioctl(fd, TIOCMGET, &s)));
    if (ret < 0)
        return -1;
    s &= ~osStateMask;
    s |= state;
    SYSCALL((ret = ioctl(fd, TIOCMSET, &s)));
    if (ret < 0)
        return -1;
    else
        return 0;
#endif
}

int
xf86GetSerialModemState(int fd)
{
    int ret;
    int s;

    if (fd < 0)
        return -1;

    /* Don't try to set parameters for non-tty devices. */
    if (!isatty(fd))
        return 0;

#ifndef TIOCMGET
    return -1;
#else
    SYSCALL((ret = ioctl(fd, TIOCMGET, &s)));
    if (ret < 0)
        return -1;
    return os2xfState(s);
#endif
}

int
xf86SerialModemSetBits(int fd, int bits)
{
    int ret;
    int s;

    if (fd < 0)
        return -1;

    /* Don't try to set parameters for non-tty devices. */
    if (!isatty(fd))
        return 0;

#ifndef TIOCMGET
    return -1;
#else
    s = xf2osState(bits);
    SYSCALL((ret = ioctl(fd, TIOCMBIS, &s)));
    return ret;
#endif
}

int
xf86SerialModemClearBits(int fd, int bits)
{
    int ret;
    int s;

    if (fd < 0)
        return -1;

    /* Don't try to set parameters for non-tty devices. */
    if (!isatty(fd))
        return 0;

#ifndef TIOCMGET
    return -1;
#else
    s = xf2osState(bits);
    SYSCALL((ret = ioctl(fd, TIOCMBIC, &s)));
    return ret;
#endif
}
