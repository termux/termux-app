/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). All
 * rights reserved.
 * Copyright (c) 1993, 2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 *
 * This file contains functionality for identifying clients by various
 * means. The primary purpose of identification is to simply aid in
 * finding out which clients are using X server and how they are using
 * it. For example, it's often necessary to monitor what requests
 * clients are executing (to spot bad behaviour) and how they are
 * allocating resources in X server (to spot excessive resource
 * usage).
 *
 * This framework automatically allocates information, that can be
 * used for client identification, when a client connects to the
 * server. The information is freed when the client disconnects. The
 * allocated information is just a collection of various IDs, such as
 * PID and process name for local clients, that are likely to be
 * useful in analyzing X server usage.
 *
 * Users of the framework can query ID information about clients at
 * any time. To avoid repeated polling of IDs the users can also
 * subscribe for notifications about the availability of ID
 * information. IDs have been allocated before ClientStateCallback is
 * called with ClientStateInitial state. Similarly the IDs will be
 * released after ClientStateCallback is called with ClientStateGone
 * state.
 *
 * Author: Rami Ylimäki <rami.ylimaki@vincit.fi>
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "client.h"
#include "os.h"
#include "dixstruct.h"

#ifdef __sun
#include <errno.h>
#include <procfs.h>
#endif

#ifdef __OpenBSD__
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include <kvm.h>
#include <limits.h>
#endif

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#include <errno.h>
#include <sys/sysctl.h>
#endif

/**
 * Try to determine a PID for a client from its connection
 * information. This should be called only once when new client has
 * connected, use GetClientPid to determine the PID at other times.
 *
 * @param[in] client Connection linked to some process.
 *
 * @return PID of the client. Error (-1) if PID can't be determined
 *         for the client.
 *
 * @see GetClientPid
 */
pid_t
DetermineClientPid(struct _Client * client)
{
    LocalClientCredRec *lcc = NULL;
    pid_t pid = -1;

    if (client == NullClient)
        return pid;

    if (client == serverClient)
        return getpid();

    if (GetLocalClientCreds(client, &lcc) != -1) {
        if (lcc->fieldsSet & LCC_PID_SET)
            pid = lcc->pid;
        FreeLocalClientCreds(lcc);
    }

    return pid;
}

/**
 * Try to determine a command line string for a client based on its
 * PID. Note that mapping PID to a command hasn't been implemented for
 * some operating systems. This should be called only once when a new
 * client has connected, use GetClientCmdName/Args to determine the
 * string at other times.
 *
 * @param[in]  pid     Process ID of a client.

 * @param[out] cmdname Client process name without arguments. You must
 *                     release this by calling free. On error NULL is
 *                     returned. Pass NULL if you aren't interested in
 *                     this value.
 * @param[out] cmdargs Arguments to client process. Useful for
 *                     identifying a client that is executed from a
 *                     launcher program. You must release this by
 *                     calling free. On error NULL is returned. Pass
 *                     NULL if you aren't interested in this value.
 *
 * @see GetClientCmdName/Args
 */
void
DetermineClientCmd(pid_t pid, const char **cmdname, const char **cmdargs)
{
#if !defined(__APPLE__)
    char path[PATH_MAX + 1];
    int totsize = 0;
    int fd = 0;
#endif

    if (cmdname)
        *cmdname = NULL;
    if (cmdargs)
        *cmdargs = NULL;

    if (pid == -1)
        return;

#if defined (__APPLE__)
    {
        static dispatch_once_t once;
        static int argmax;
        dispatch_once(&once, ^{
            int mib[2];
            size_t len;

            mib[0] = CTL_KERN;
            mib[1] = KERN_ARGMAX;

            len = sizeof(argmax);
            if (sysctl(mib, 2, &argmax, &len, NULL, 0) == -1) {
                ErrorF("Unable to dynamically determine kern.argmax, using ARG_MAX (%d)\n", ARG_MAX);
                argmax = ARG_MAX;
            }
        });

        int mib[3];
        size_t len = argmax;
        int32_t argc = -1;

        char * const procargs = malloc(len);
        if (!procargs) {
            ErrorF("Failed to allocate memory (%lu bytes) for KERN_PROCARGS2 result for pid %d: %s\n", len, pid, strerror(errno));
            return;
        }

        mib[0] = CTL_KERN;
        mib[1] = KERN_PROCARGS2;
        mib[2] = pid;

        if (sysctl(mib, 3, procargs, &len, NULL, 0) == -1) {
            ErrorF("Failed to determine KERN_PROCARGS2 for pid %d: %s\n", pid, strerror(errno));
            free(procargs);
            return;
        }

        if (len < sizeof(argc) || len > argmax) {
            ErrorF("Erroneous length returned when querying KERN_PROCARGS2 for pid %d: %zu\n", pid, len);
            free(procargs);
            return;
        }

        /* Ensure we have a failsafe NUL termination just in case the last entry
         * was not actually NUL terminated.
         */
        procargs[len-1] = '\0';

        /* Setup our iterator */
        char *is = procargs;

        /* The first element in the buffer is argc as a 32bit int. When using
         * the older KERN_PROCARGS, this is omitted, and one needs to guess
         * (usually by checking for an `=` character) when we start seeing
         * envvars instead of arguments.
         */
        argc = *(int32_t *)is;
        is += sizeof(argc);

        /* The very next string is the executable path.  Skip over it since
         * this function wants to return argv[0] and argv[1...n].
         */
        is += strlen(is) + 1;

        /* Skip over extra NUL characters to get to the start of argv[0] */
        for (; (is < &procargs[len]) && !(*is); is++);

        if (! (is < &procargs[len])) {
            ErrorF("Arguments were not returned when querying KERN_PROCARGS2 for pid %d: %zu\n", pid, len);
            free(procargs);
            return;
        }

        if (cmdname) {
            *cmdname = strdup(is);
        }

        /* Jump over argv[0] and point to argv[1] */
        is += strlen(is) + 1;

        if (cmdargs && is < &procargs[len]) {
            char *args = is;

            /* Remove the NUL terminators except the last one */
            for (int i = 1; i < argc - 1; i++) {
                /* Advance to the NUL terminator */
                is += strlen(is);

                /* Change the NUL to a space, ensuring we don't accidentally remove the terminal NUL */
                if (is < &procargs[len-1]) {
                    *is = ' ';
                }
            }

            *cmdargs = strdup(args);
        }

        free(procargs);
    }
#elif defined(__OpenBSD__)
    /* on OpenBSD use kvm_getargv() */
    {
        kvm_t *kd;
        char errbuf[_POSIX2_LINE_MAX];
        char **argv;
        struct kinfo_proc *kp;
        size_t len = 0;
        int i, n;

        kd = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, errbuf);
        if (kd == NULL)
            return;
        kp = kvm_getprocs(kd, KERN_PROC_PID, pid, sizeof(struct kinfo_proc),
                          &n);
        if (n != 1)
            return;
        argv = kvm_getargv(kd, kp, 0);
        *cmdname = strdup(argv[0]);
        i = 1;
        while (argv[i] != NULL) {
            len += strlen(argv[i]) + 1;
            i++;
        }
        *cmdargs = calloc(1, len);
        i = 1;
        while (argv[i] != NULL) {
            strlcat(*cmdargs, argv[i], len);
            strlcat(*cmdargs, " ", len);
            i++;
        }
        kvm_close(kd);
    }
#else                           /* Linux using /proc/pid/cmdline */

    /* Check if /proc/pid/cmdline exists. It's not supported on all
     * operating systems. */
    if (snprintf(path, sizeof(path), "/proc/%d/cmdline", pid) < 0)
        return;
    fd = open(path, O_RDONLY);
    if (fd < 0)
#ifdef __sun
        goto fallback;
#else
        return;
#endif

    /* Read the contents of /proc/pid/cmdline. It should contain the
     * process name and arguments. */
    totsize = read(fd, path, sizeof(path));
    close(fd);
    if (totsize <= 0)
        return;
    path[totsize - 1] = '\0';

    /* Construct the process name without arguments. */
    if (cmdname) {
        *cmdname = strdup(path);
    }

    /* Construct the arguments for client process. */
    if (cmdargs) {
        int cmdsize = strlen(path) + 1;
        int argsize = totsize - cmdsize;
        char *args = NULL;

        if (argsize > 0)
            args = malloc(argsize);
        if (args) {
            int i = 0;

            for (i = 0; i < (argsize - 1); ++i) {
                const char c = path[cmdsize + i];

                args[i] = (c == '\0') ? ' ' : c;
            }
            args[argsize - 1] = '\0';
            *cmdargs = args;
        }
    }
    return;
#endif

#ifdef __sun                    /* Solaris */
  fallback:
    /* Solaris prior to 11.3.5 does not support /proc/pid/cmdline, but
     * makes information similar to what ps shows available in a binary
     * structure in the /proc/pid/psinfo file. */
    if (snprintf(path, sizeof(path), "/proc/%d/psinfo", pid) < 0)
        return;
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        ErrorF("Failed to open %s: %s\n", path, strerror(errno));
        return;
    }
    else {
        psinfo_t psinfo = { 0 };
        char *sp;

        totsize = read(fd, &psinfo, sizeof(psinfo_t));
        close(fd);
        if (totsize <= 0)
            return;

        /* pr_psargs is the first PRARGSZ (80) characters of the command
         * line string - assume up to the first space is the command name,
         * since it's not delimited.   While there is also pr_fname, that's
         * more limited, giving only the first 16 chars of the basename of
         * the file that was exec'ed, thus cutting off many long gnome
         * command names, or returning "isapython2.6" for all python scripts.
         */
        psinfo.pr_psargs[PRARGSZ - 1] = '\0';
        sp = strchr(psinfo.pr_psargs, ' ');
        if (sp)
            *sp++ = '\0';

        if (cmdname)
            *cmdname = strdup(psinfo.pr_psargs);

        if (cmdargs && sp)
            *cmdargs = strdup(sp);
    }
#endif
}

/**
 * Called when a new client connects. Allocates client ID information.
 *
 * @param[in] client Recently connected client.
 */
void
ReserveClientIds(struct _Client *client)
{
#ifdef CLIENTIDS
    if (client == NullClient)
        return;

    assert(!client->clientIds);
    client->clientIds = calloc(1, sizeof(ClientIdRec));
    if (!client->clientIds)
        return;

    client->clientIds->pid = DetermineClientPid(client);
    if (client->clientIds->pid != -1)
        DetermineClientCmd(client->clientIds->pid, &client->clientIds->cmdname,
                           &client->clientIds->cmdargs);

    DebugF("client(%lx): Reserved pid(%d).\n",
           (unsigned long) client->clientAsMask, client->clientIds->pid);
    DebugF("client(%lx): Reserved cmdname(%s) and cmdargs(%s).\n",
           (unsigned long) client->clientAsMask,
           client->clientIds->cmdname ? client->clientIds->cmdname : "NULL",
           client->clientIds->cmdargs ? client->clientIds->cmdargs : "NULL");
#endif                          /* CLIENTIDS */
}

/**
 * Called when an existing client disconnects. Frees client ID
 * information.
 *
 * @param[in] client Recently disconnected client.
 */
void
ReleaseClientIds(struct _Client *client)
{
#ifdef CLIENTIDS
    if (client == NullClient)
        return;

    if (!client->clientIds)
        return;

    DebugF("client(%lx): Released pid(%d).\n",
           (unsigned long) client->clientAsMask, client->clientIds->pid);
    DebugF("client(%lx): Released cmdline(%s) and cmdargs(%s).\n",
           (unsigned long) client->clientAsMask,
           client->clientIds->cmdname ? client->clientIds->cmdname : "NULL",
           client->clientIds->cmdargs ? client->clientIds->cmdargs : "NULL");

    free((void *) client->clientIds->cmdname);  /* const char * */
    free((void *) client->clientIds->cmdargs);  /* const char * */
    free(client->clientIds);
    client->clientIds = NULL;
#endif                          /* CLIENTIDS */
}

/**
 * Get cached PID of a client.
 *
 * param[in] client Client whose PID has been already cached.
 *
 * @return Cached client PID. Error (-1) if called:
 *         - before ClientStateInitial client state notification
 *         - after ClientStateGone client state notification
 *         - for remote clients
 *
 * @see DetermineClientPid
 */
pid_t
GetClientPid(struct _Client *client)
{
    if (client == NullClient)
        return -1;

    if (!client->clientIds)
        return -1;

    return client->clientIds->pid;
}

/**
 * Get cached command name string of a client.
 *
 * param[in] client Client whose command line string has been already
 *                  cached.
 *
 * @return Cached client command name. Error (NULL) if called:
 *         - before ClientStateInitial client state notification
 *         - after ClientStateGone client state notification
 *         - for remote clients
 *         - on OS that doesn't support mapping of PID to command line
 *
 * @see DetermineClientCmd
 */
const char *
GetClientCmdName(struct _Client *client)
{
    if (client == NullClient)
        return NULL;

    if (!client->clientIds)
        return NULL;

    return client->clientIds->cmdname;
}

/**
 * Get cached command arguments string of a client.
 *
 * param[in] client Client whose command line string has been already
 *                  cached.
 *
 * @return Cached client command arguments. Error (NULL) if called:
 *         - before ClientStateInitial client state notification
 *         - after ClientStateGone client state notification
 *         - for remote clients
 *         - on OS that doesn't support mapping of PID to command line
 *
 * @see DetermineClientCmd
 */
const char *
GetClientCmdArgs(struct _Client *client)
{
    if (client == NullClient)
        return NULL;

    if (!client->clientIds)
        return NULL;

    return client->clientIds->cmdargs;
}
