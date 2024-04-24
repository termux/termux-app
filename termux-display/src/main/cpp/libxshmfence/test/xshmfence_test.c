/*
 * Copyright © 2013 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <xshmfence.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NCHILD  5               /* number of child processes to fork */
#define NCHECK  10              /* number of times to signal the fence */

/* Catch an alarm and bail
 */
static void
sigalrm(int sig)
{
    write(2, "caught alarm\n", 13);
    exit(1);
}

int
main(int argc, char **argv)
{
    int		        fd;
    struct xshmfence    *x;
    int		        i;
    int		        c;
    int		        pid;
    int                 status;
    int                 failed = 0;

    /* Allocate a fence
     */
    fd = xshmfence_alloc_shm();
    if (fd < 0) {
        perror("xshmfence_alloc_shm");
        exit(1);
    }

    /* fork NCHILD processes to wait for the fence
     */
    for (c = 0; c < NCHILD; c++) {
        switch (fork()) {
        case -1:
            perror("fork");
            exit(1);
        case 0:

            /* Set an alarm to limit how long
             * to wait
             */
            signal(SIGALRM, sigalrm);
            alarm(10);
    
            /* Map the fence
             */
            x = xshmfence_map_shm(fd);
            if (!x) {
                fprintf(stderr, "%6d: ", c);
                perror("xshmfence_map_shm");
                exit(1);
            }

            for (i = 0; i < NCHECK; i++) {

                /* Verify that the fence is currently reset
                 */
                if (xshmfence_query(x) != 0) {
                    fprintf(stderr, "%6d: query reset failed\n", c);
                    exit(1);
                }

                /* Wait for the fence
                 */
                fprintf(stderr, "%6d: waiting\n", c);
                if (xshmfence_await(x) < 0) {
                    fprintf(stderr, "%6d: ", c);
                    perror("xshmfence_await");
                    exit(1);
                }

                fprintf(stderr, "%6d: awoken\n", c);

                /* Verify that the fence is currently triggered
                 */
                if (xshmfence_query(x) == 0) {
                    fprintf(stderr, "%6d: query triggered failed\n", c);
                    exit(1);
                }

                usleep(10 * 1000);

                /* Reset the fence
                 */
                if (c == 0)
                    xshmfence_reset(x);

                usleep(10 * 1000);
            }
            fprintf(stderr, "%6d: done\n", c);
            exit(0);
        }
    }

    /* Map the fence into the parent process
     */
    x = xshmfence_map_shm(fd);
    if (!x) {
        perror("xshmfence_map_shm");
        exit(1);
    }

    for (i = 0; i < NCHECK; i++) {
        usleep(100 * 1000);
        fprintf(stderr, "trigger\n");

        /* Verify that the fence is reset
         */
        if (xshmfence_query(x) != 0) {
            fprintf(stderr, "query reset failed\n");
            exit(1);
        }

        /* Trigger the fence
         */
        if (xshmfence_trigger(x) < 0) {
            perror("xshmfence_trigger");
            exit(1);
        }

        /* Verify that the fence is triggered
         */
        if (xshmfence_query(x) == 0) {
            fprintf (stderr, "query triggered failed\n");
            exit(1);
        }

        fprintf(stderr, "trigger done\n");
    }

    /* Reap all of the child processes
     */
    for (c = 0; c < NCHILD; c++) {
        pid = wait(&status);
        if (pid < 0) {
            perror("wait");
            exit(1);
        }
        fprintf(stderr, "child %d done %d\n", pid, status);
        if (status)
            failed++;
    }
    exit(failed);
}
