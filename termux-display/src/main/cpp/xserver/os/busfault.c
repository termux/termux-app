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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/Xos.h>
#include <X11/Xdefs.h>
#include "misc.h"
#include <busfault.h>
#include <list.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <signal.h>

struct busfault {
    struct xorg_list    list;

    void                *addr;
    size_t              size;

    Bool                valid;

    busfault_notify_ptr notify;
    void                *context;
};

static Bool             busfaulted;
static struct xorg_list busfaults;

struct busfault *
busfault_register_mmap(void *addr, size_t size, busfault_notify_ptr notify, void *context)
{
    struct busfault     *busfault;

    busfault = calloc(1, sizeof (struct busfault));
    if (!busfault)
        return NULL;

    busfault->addr = addr;
    busfault->size = size;
    busfault->notify = notify;
    busfault->context = context;
    busfault->valid = TRUE;

    xorg_list_add(&busfault->list, &busfaults);
    return busfault;
}

void
busfault_unregister(struct busfault *busfault)
{
    xorg_list_del(&busfault->list);
    free(busfault);
}

void
busfault_check(void)
{
    struct busfault     *busfault, *tmp;

    if (!busfaulted)
        return;

    busfaulted = FALSE;

    xorg_list_for_each_entry_safe(busfault, tmp, &busfaults, list) {
        if (!busfault->valid)
            (*busfault->notify)(busfault->context);
    }
}

static void (*previous_busfault_sigaction)(int sig, siginfo_t *info, void *param);

static void
busfault_sigaction(int sig, siginfo_t *info, void *param)
{
    void                *fault = info->si_addr;
    struct busfault     *iter, *busfault = NULL;
    void                *new_addr;

    /* Locate the faulting address in our list of shared segments
     */
    xorg_list_for_each_entry(iter, &busfaults, list) {
	if ((char *) iter->addr <= (char *) fault && (char *) fault < (char *) iter->addr + iter->size) {
	    busfault = iter;
	    break;
	}
    }
    if (!busfault)
        goto panic;

    if (!busfault->valid)
        goto panic;

    busfault->valid = FALSE;
    busfaulted = TRUE;

    /* The client truncated the file; unmap the shared file, map
     * /dev/zero over that area and keep going
     */

    new_addr = mmap(busfault->addr, busfault->size, PROT_READ|PROT_WRITE,
                    MAP_ANON|MAP_PRIVATE|MAP_FIXED, -1, 0);

    if (new_addr == MAP_FAILED)
        goto panic;

    return;
panic:
    if (previous_busfault_sigaction)
        (*previous_busfault_sigaction)(sig, info, param);
    else
        FatalError("bus error\n");
}

Bool
busfault_init(void)
{
    struct sigaction    act, old_act;

    act.sa_sigaction = busfault_sigaction;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGBUS, &act, &old_act) < 0)
        return FALSE;
    previous_busfault_sigaction = old_act.sa_sigaction;
    xorg_list_init(&busfaults);
    return TRUE;
}
