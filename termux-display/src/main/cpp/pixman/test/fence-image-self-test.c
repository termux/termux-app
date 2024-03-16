/*
 * Copyright © 2015 Raspberry Pi Foundation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "utils.h"


#if FENCE_MALLOC_ACTIVE && defined (HAVE_SIGACTION)

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

pixman_bool_t verbose;

static void
segv_handler (int sig, siginfo_t *si, void *unused)
{
    _exit (EXIT_SUCCESS);
}

static void
die (const char *msg, int err)
{
    if (err)
        perror (msg);
    else
        fprintf (stderr, "%s\n", msg);

    abort ();
}

static void
prinfo (const char *fmt, ...)
{
    va_list ap;

    if (!verbose)
        return;

    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
}

static void
do_expect_signal (void (*fn)(void *), void *data)
{
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sigemptyset (&sa.sa_mask);
    sa.sa_sigaction = segv_handler;
    if (sigaction (SIGSEGV, &sa, NULL) == -1)
        die ("sigaction failed", errno);
    if (sigaction (SIGBUS, &sa, NULL) == -1)
        die ("sigaction failed", errno);

    (*fn)(data);

    _exit (EXIT_FAILURE);
}

/* Check that calling fn(data) causes a segmentation fault.
 *
 * You cannot portably return from a SIGSEGV handler in any way,
 * so we fork, and do the test in the child process. Child's
 * exit status will reflect the result. Its SEGV handler causes it
 * to exit with success, and return failure otherwise.
 */
static pixman_bool_t
expect_signal (void (*fn)(void *), void *data)
{
    pid_t pid, wp;
    int status;

    pid = fork ();
    if (pid == -1)
        die ("fork failed", errno);

    if (pid == 0)
        do_expect_signal (fn, data); /* never returns */

    wp = waitpid (pid, &status, 0);
    if (wp != pid)
        die ("waitpid did not work", wp == -1 ? errno : 0);

    if (WIFEXITED (status) && WEXITSTATUS (status) == EXIT_SUCCESS)
        return TRUE;

    return FALSE;
}

static void
read_u8 (void *data)
{
    volatile uint8_t *p = data;

    *p;
}

static pixman_bool_t
test_read_fault (uint8_t *p, int offset)
{
    prinfo ("*(uint8_t *)(%p + %d)", p, offset);

    if (expect_signal (read_u8, p + offset))
    {
        prinfo ("\tsignal OK\n");

        return TRUE;
    }

    prinfo ("\tFAILED\n");

    return FALSE;
}

static void
test_read_ok (uint8_t *p, int offset)
{
    prinfo ("*(uint8_t *)(%p + %d)", p, offset);

    /* If fails, SEGV. */
    read_u8 (p + offset);

    prinfo ("\tOK\n");
}

static pixman_bool_t
test_read_faults (pixman_image_t *image)
{
    pixman_bool_t ok = TRUE;
    pixman_format_code_t format = pixman_image_get_format (image);
    int width = pixman_image_get_width (image);
    int height = pixman_image_get_height (image);
    int stride = pixman_image_get_stride (image);
    uint8_t *p = (void *)pixman_image_get_data (image);
    int row_bytes = width * PIXMAN_FORMAT_BPP (format) / 8;

    prinfo ("%s %dx%d, row %d B, stride %d B:\n",
            format_name (format), width, height, row_bytes, stride);

    assert (height > 3);

    test_read_ok (p, 0);
    test_read_ok (p, row_bytes - 1);
    test_read_ok (p, stride);
    test_read_ok (p, stride + row_bytes - 1);
    test_read_ok (p, 2 * stride);
    test_read_ok (p, 2 * stride + row_bytes - 1);
    test_read_ok (p, 3 * stride);
    test_read_ok (p, (height - 1) * stride + row_bytes - 1);

    ok &= test_read_fault (p, -1);
    ok &= test_read_fault (p, row_bytes);
    ok &= test_read_fault (p, stride - 1);
    ok &= test_read_fault (p, stride + row_bytes);
    ok &= test_read_fault (p, 2 * stride - 1);
    ok &= test_read_fault (p, 2 * stride + row_bytes);
    ok &= test_read_fault (p, 3 * stride - 1);
    ok &= test_read_fault (p, height * stride);

    return ok;
}

static pixman_bool_t
test_image_faults (pixman_format_code_t format, int min_width, int height)
{
    pixman_bool_t ok;
    pixman_image_t *image;

    image = fence_image_create_bits (format, min_width, height, TRUE);
    ok = test_read_faults (image);
    pixman_image_unref (image);

    return ok;
}

int
main (int argc, char **argv)
{
    pixman_bool_t ok = TRUE;

    if (getenv ("VERBOSE") != NULL)
        verbose = TRUE;

    ok &= test_image_faults (PIXMAN_a8r8g8b8, 7, 5);
    ok &= test_image_faults (PIXMAN_r8g8b8, 7, 5);
    ok &= test_image_faults (PIXMAN_r5g6b5, 7, 5);
    ok &= test_image_faults (PIXMAN_a8, 7, 5);
    ok &= test_image_faults (PIXMAN_a4, 7, 5);
    ok &= test_image_faults (PIXMAN_a1, 7, 5);

    if (ok)
        return EXIT_SUCCESS;

    return EXIT_FAILURE;
}

#else /* FENCE_MALLOC_ACTIVE */

int
main (int argc, char **argv)
{
    /* Automake return code for test SKIP. */
    return 77;
}

#endif /* FENCE_MALLOC_ACTIVE */
