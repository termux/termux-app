/**************************************************************
 *
 * Startup code for the Quartz Darwin X Server
 * Copyright (c) 2008-2012 Apple Inc. All rights reserved.
 * Copyright (c) 2001-2004 Torrey T. Lyons. All Rights Reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#include "sanitizedCarbon.h"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <CoreFoundation/CoreFoundation.h>
#include "X11Controller.h"
#include "darwin.h"
#include "darwinEvents.h"
#include "quartz.h"
#include "opaque.h"
#include "micmap.h"

#include <assert.h>

#include <pthread.h>

int
dix_main(int argc, char **argv, char **envp);

struct arg {
    int argc;
    char **argv;
    char **envp;
};

_X_NORETURN
static void
server_thread(void *arg)
{
    struct arg args = *((struct arg *)arg);
    free(arg);
    exit(dix_main(args.argc, args.argv, args.envp));
}

static pthread_t
create_thread(void *func, void *arg)
{
    pthread_attr_t attr;
    pthread_t tid;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, func, arg);
    pthread_attr_destroy(&attr);

    return tid;
}

void
QuartzInitServer(int argc, char **argv, char **envp)
{
    struct arg *args = (struct arg *)malloc(sizeof(struct arg));
    if (!args)
        FatalError("Could not allocate memory.\n");

    args->argc = argc;
    args->argv = argv;
    args->envp = envp;

    if (!create_thread(server_thread, args)) {
        FatalError("can't create secondary thread\n");
    }

    /* Block signals on the AppKit thread that the X11 expects to handle on its thread */
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
#ifdef BUSFAULT
    sigaddset(&set, SIGBUS);
#endif
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}

int
server_main(int argc, char **argv, char **envp)
{
    int i;
    int fd[2];

    /* Unset CFProcessPath, so our children don't inherit this kludge we need
     * to load our nib.  If an xterm gets this set, then it fails to
     * 'open hi.txt' properly.
     */
    unsetenv("CFProcessPath");

    // Make a pipe to pass events
    assert(pipe(fd) == 0);
    darwinEventReadFD = fd[0];
    darwinEventWriteFD = fd[1];
    fcntl(darwinEventReadFD, F_SETFL, O_NONBLOCK);

    for (i = 1; i < argc; i++) {
        // Display version info without starting Mac OS X UI if requested
        if (!strcmp(argv[i],
                    "-showconfig") || !strcmp(argv[i], "-version")) {
            DarwinPrintBanner();
            exit(0);
        }
    }

    X11ControllerMain(argc, argv, envp);
    exit(0);
}
