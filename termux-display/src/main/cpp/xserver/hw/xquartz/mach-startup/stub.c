/* Copyright (c) 2008-2012 Apple Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#include <CoreServices/CoreServices.h>

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <asl.h>

#include <sys/socket.h>
#include <sys/un.h>

#define kX11AppBundleId   BUNDLE_ID_PREFIX ".X11"
#define kX11AppBundlePath "/Contents/MacOS/X11"

#include <mach/mach.h>
#include <mach/mach_error.h>
#include <servers/bootstrap.h>
#include "mach_startup.h"

#include <signal.h>

#include "launchd_fd.h"

static CFURLRef x11appURL;
static FSRef x11_appRef;
static pid_t x11app_pid = 0;
aslclient aslc;

static void
set_x11_path(void)
{
    OSStatus osstatus = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR(kX11AppBundleId),
                                                 nil, &x11_appRef, &x11appURL);

    switch (osstatus) {
    case noErr:
        if (x11appURL == NULL) {
            asl_log(aslc, NULL, ASL_LEVEL_ERR,
                    "Xquartz: Invalid response from LSFindApplicationForInfo(%s)",
                    kX11AppBundleId);
            exit(1);
        }
        break;

    case kLSApplicationNotFoundErr:
        asl_log(aslc, NULL, ASL_LEVEL_ERR,
                "Xquartz: Unable to find application for %s",
                kX11AppBundleId);
        exit(10);

    default:
        asl_log(aslc, NULL, ASL_LEVEL_ERR,
                "Xquartz: Unable to find application for %s, error code = %d",
                kX11AppBundleId, (int)osstatus);
        exit(11);
    }
}

static int
connect_to_socket(const char *filename)
{
    struct sockaddr_un servaddr_un;
    struct sockaddr *servaddr;
    socklen_t servaddr_len;
    int ret_fd;

    /* Setup servaddr_un */
    memset(&servaddr_un, 0, sizeof(struct sockaddr_un));
    servaddr_un.sun_family = AF_UNIX;
    strlcpy(servaddr_un.sun_path, filename, sizeof(servaddr_un.sun_path));

    servaddr = (struct sockaddr *)&servaddr_un;
    servaddr_len = sizeof(struct sockaddr_un) -
                   sizeof(servaddr_un.sun_path) + strlen(filename);

    ret_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (ret_fd == -1) {
        asl_log(aslc, NULL, ASL_LEVEL_ERR,
                "Xquartz: Failed to create socket: %s - %d - %s",
                filename, errno, strerror(errno));
        return -1;
    }

    if (connect(ret_fd, servaddr, servaddr_len) < 0) {
        asl_log(aslc, NULL, ASL_LEVEL_ERR,
                "Xquartz: Failed to connect to socket: %s - %d - %s",
                filename, errno, strerror(errno));
        close(ret_fd);
        return -1;
    }

    return ret_fd;
}

static void
send_fd_handoff(int connected_fd, int launchd_fd)
{
    char databuf[] = "display";
    struct iovec iov[1];

    union {
        struct cmsghdr hdr;
        char bytes[CMSG_SPACE(sizeof(int))];
    } buf;

    struct msghdr msg;
    struct cmsghdr *cmsg;

    iov[0].iov_base = databuf;
    iov[0].iov_len = sizeof(databuf);

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf.bytes;
    msg.msg_controllen = sizeof(buf);
    msg.msg_name = 0;
    msg.msg_namelen = 0;
    msg.msg_flags = 0;

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    msg.msg_controllen = cmsg->cmsg_len;

    *((int *)CMSG_DATA(cmsg)) = launchd_fd;

    if (sendmsg(connected_fd, &msg, 0) < 0) {
        asl_log(aslc, NULL, ASL_LEVEL_ERR,
                "Xquartz: Error sending $DISPLAY file descriptor over fd %d: %d -- %s",
                connected_fd, errno, strerror(errno));
        return;
    }

    asl_log(aslc, NULL, ASL_LEVEL_DEBUG,
            "Xquartz: Message sent.  Closing handoff fd.");
    close(connected_fd);
}

__attribute__((__noreturn__))
static void
signal_handler(int sig)
{
    if (x11app_pid)
        kill(x11app_pid, sig);
    _exit(0);
}

int
main(int argc, char **argv, char **envp)
{
    int envpc;
    kern_return_t kr;
    mach_port_t mp;
    string_array_t newenvp;
    string_array_t newargv;
    size_t i;
    int launchd_fd;
    string_t handoff_socket_filename;
    sig_t handler;
    char *asl_sender;
    char *asl_facility;
    char *server_bootstrap_name = kX11AppBundleId;

    if (getenv("X11_PREFS_DOMAIN"))
        server_bootstrap_name = getenv("X11_PREFS_DOMAIN");

    asprintf(&asl_sender, "%s.stub", server_bootstrap_name);
    assert(asl_sender);

    asl_facility = strdup(server_bootstrap_name);
    assert(asl_facility);
    if (strcmp(asl_facility + strlen(asl_facility) - 4, ".X11") == 0)
        asl_facility[strlen(asl_facility) - 4] = '\0';

    assert(aslc = asl_open(asl_sender, asl_facility, ASL_OPT_NO_DELAY));
    free(asl_sender);
    free(asl_facility);

    /* We don't have a mechanism in place to handle this interrupt driven
     * server-start notification, so just send the signal now, so xinit doesn't
     * time out waiting for it and will just poll for the server.
     */
    handler = signal(SIGUSR1, SIG_IGN);
    if (handler == SIG_IGN)
        kill(getppid(), SIGUSR1);
    signal(SIGUSR1, handler);

    /* Pass on SIGs to X11.app */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Get the $DISPLAY FD */
    launchd_fd = launchd_display_fd();

    kr = bootstrap_look_up(bootstrap_port, server_bootstrap_name, &mp);
    if (kr != KERN_SUCCESS) {
        pid_t child;

        asl_log(aslc, NULL, ASL_LEVEL_WARNING,
                "Xquartz: Unable to locate waiting server: %s",
                server_bootstrap_name);
        set_x11_path();

        char *listenOnlyArg = "--listenonly";
        CFStringRef silentLaunchArg = CFStringCreateWithCString(NULL, listenOnlyArg, kCFStringEncodingUTF8);
        CFStringRef args[] = { silentLaunchArg };
        CFArrayRef passArgv = CFArrayCreate(NULL, (const void**) args, 1, NULL);
        LSApplicationParameters params = { 0, /* CFIndex version == 0 */
                                           kLSLaunchDefaults, /* LSLaunchFlags flags */
                                           &x11_appRef, /* FSRef application */
                                           NULL, /* void* asyncLaunchRefCon*/
                                           NULL, /* CFDictionaryRef environment */
                                           passArgv, /* CFArrayRef arguments */
                                           NULL /* AppleEvent* initialEvent */
        };

        OSStatus status = LSOpenApplication(&params, NULL);
        if (status != noErr) {
            asl_log(aslc, NULL, ASL_LEVEL_ERR, "Xquartz: Unable to launch: %d", (int)status);
            return EXIT_FAILURE;
        }

        /* Try connecting for 10 seconds */
        for (i = 0; i < 80; i++) {
            usleep(250000);
            kr = bootstrap_look_up(bootstrap_port, server_bootstrap_name, &mp);
            if (kr == KERN_SUCCESS)
                break;
        }

        if (kr != KERN_SUCCESS) {
            asl_log(aslc, NULL, ASL_LEVEL_ERR,
                    "Xquartz: bootstrap_look_up(): %s", bootstrap_strerror(kr));
            return EXIT_FAILURE;
        }
    }

    /* Get X11.app's pid */
    request_pid(mp, &x11app_pid);

    /* Handoff the $DISPLAY FD */
    if (launchd_fd != -1) {
        size_t try, try_max;
        int handoff_fd = -1;

        for (try = 0, try_max = 5; try < try_max; try++) {
            if (request_fd_handoff_socket(mp, handoff_socket_filename) != KERN_SUCCESS) {
                asl_log(aslc, NULL, ASL_LEVEL_INFO,
                        "Xquartz: Failed to request a socket from the server to send the $DISPLAY fd over (try %d of %d)",
                        (int)try + 1, (int)try_max);
                continue;
            }

            handoff_fd = connect_to_socket(handoff_socket_filename);
            if (handoff_fd == -1) {
                asl_log(aslc, NULL, ASL_LEVEL_ERR,
                        "Xquartz: Failed to connect to socket (try %d of %d)",
                        (int)try + 1, (int)try_max);
                continue;
            }

            asl_log(aslc, NULL, ASL_LEVEL_INFO,
                    "Xquartz: Handoff connection established (try %d of %d) on fd %d, \"%s\".  Sending message.",
                    (int)try + 1, (int)try_max, handoff_fd, handoff_socket_filename);
            send_fd_handoff(handoff_fd, launchd_fd);
            close(handoff_fd);
            break;
        }
    }

    /* Count envp */
    for (envpc = 0; envp[envpc]; envpc++) ;

    /* We have fixed-size string lengths due to limitations in IPC,
     * so we need to copy our argv and envp.
     */
    newargv = (string_array_t)calloc((1 + argc), sizeof(string_t));
    newenvp = (string_array_t)calloc((1 + envpc), sizeof(string_t));

    if (!newargv || !newenvp) {
        /* Silence the clang static analyzer */
        free(newargv);
        free(newenvp);

        asl_log(aslc, NULL, ASL_LEVEL_ERR, "Xquartz: Memory allocation failure");
        return EXIT_FAILURE;
    }

    for (i = 0; i < argc; i++) {
        strlcpy(newargv[i], argv[i], STRING_T_SIZE);
    }
    for (i = 0; i < envpc; i++) {
        strlcpy(newenvp[i], envp[i], STRING_T_SIZE);
    }

    kr = start_x11_server(mp, newargv, argc, newenvp, envpc);

    free(newargv);
    free(newenvp);

    if (kr != KERN_SUCCESS) {
        asl_log(aslc, NULL, ASL_LEVEL_ERR, "Xquartz: start_x11_server: %s",
                mach_error_string(kr));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
