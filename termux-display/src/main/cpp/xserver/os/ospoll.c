/*
 * Copyright © 2016 Keith Packard
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

#include <X11/X.h>
#include <X11/Xproto.h>
#include <stdlib.h>
#include <unistd.h>
#include "misc.h"               /* for typedef of pointer */
#include "ospoll.h"
#include "list.h"

#if !HAVE_OSPOLL && defined(HAVE_POLLSET_CREATE)
#include <sys/pollset.h>
#define POLLSET         1
#define HAVE_OSPOLL     1
#endif

#if !HAVE_OSPOLL && defined(HAVE_PORT_CREATE)
#include <port.h>
#include <poll.h>
#define PORT            1
#define HAVE_OSPOLL     1
#endif

#if !HAVE_OSPOLL && defined(HAVE_EPOLL_CREATE1)
#include <sys/epoll.h>
#define EPOLL           1
#define HAVE_OSPOLL     1
#endif

#if !HAVE_OSPOLL
#include "xserver_poll.h"
#define POLL            1
#define HAVE_OSPOLL     1
#endif

#if POLLSET

// pollset-based implementation (as seen on AIX)
struct ospollfd {
    int                 fd;
    int                 xevents;
    short               revents;
    enum ospoll_trigger trigger;
    void                (*callback)(int fd, int xevents, void *data);
    void                *data;
};

struct ospoll {
    pollset_t           ps;
    struct ospollfd     *fds;
    int                 num;
    int                 size;
};

#endif

#if EPOLL || PORT

/* epoll-based implementation */
struct ospollfd {
    int                 fd;
    int                 xevents;
    enum ospoll_trigger trigger;
    void                (*callback)(int fd, int xevents, void *data);
    void                *data;
    struct xorg_list    deleted;
};

struct ospoll {
    int                 epoll_fd;
    struct ospollfd     **fds;
    int                 num;
    int                 size;
    struct xorg_list    deleted;
};

#endif

#if POLL

/* poll-based implementation */
struct ospollfd {
    short               revents;
    enum ospoll_trigger trigger;
    void                (*callback)(int fd, int revents, void *data);
    void                *data;
};

struct ospoll {
    struct pollfd       *fds;
    struct ospollfd     *osfds;
    int                 num;
    int                 size;
    Bool                changed;
};

#endif

/* Binary search for the specified file descriptor
 *
 * Returns position if found
 * Returns -position - 1 if not found
 */

static int
ospoll_find(struct ospoll *ospoll, int fd)
{
    int lo = 0;
    int hi = ospoll->num - 1;

    while (lo <= hi) {
        int m = (lo + hi) >> 1;
#if EPOLL || PORT
        int t = ospoll->fds[m]->fd;
#endif
#if POLL || POLLSET
        int t = ospoll->fds[m].fd;
#endif

        if (t < fd)
            lo = m + 1;
        else if (t > fd)
            hi = m - 1;
        else
            return m;
    }
    return -(lo + 1);
}

#if EPOLL || PORT
static void
ospoll_clean_deleted(struct ospoll *ospoll)
{
    struct ospollfd     *osfd, *tmp;

    xorg_list_for_each_entry_safe(osfd, tmp, &ospoll->deleted, deleted) {
        xorg_list_del(&osfd->deleted);
        free(osfd);
    }
}
#endif

/* Insert an element into an array
 *
 * base: base address of array
 * num:  number of elements in the array before the insert
 * size: size of each element
 * pos:  position to insert at
 */
static inline void
array_insert(void *base, size_t num, size_t size, size_t pos)
{
    char *b = base;

    memmove(b + (pos+1) * size,
            b + pos * size,
            (num - pos) * size);
}

/* Delete an element from an array
 *
 * base: base address of array
 * num:  number of elements in the array before the delete
 * size: size of each element
 * pos:  position to delete from
 */
static inline void
array_delete(void *base, size_t num, size_t size, size_t pos)
{
    char *b = base;

    memmove(b + pos * size, b + (pos + 1) * size,
            (num - pos - 1) * size);
}


struct ospoll *
ospoll_create(void)
{
#if POLLSET
    struct ospoll *ospoll = calloc(1, sizeof (struct ospoll));

    ospoll->ps = pollset_create(-1);
    if (ospoll->ps < 0) {
        free (ospoll);
        return NULL;
    }
    return ospoll;
#endif
#if PORT
    struct ospoll *ospoll = calloc(1, sizeof (struct ospoll));

    ospoll->epoll_fd = port_create();
    if (ospoll->epoll_fd < 0) {
        free (ospoll);
        return NULL;
    }
    xorg_list_init(&ospoll->deleted);
    return ospoll;
#endif
#if EPOLL
    struct ospoll       *ospoll = calloc(1, sizeof (struct ospoll));

    ospoll->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (ospoll->epoll_fd < 0) {
        free (ospoll);
        return NULL;
    }
    xorg_list_init(&ospoll->deleted);
    return ospoll;
#endif
#if POLL
    return calloc(1, sizeof (struct ospoll));
#endif
}

void
ospoll_destroy(struct ospoll *ospoll)
{
#if POLLSET
    if (ospoll) {
        assert (ospoll->num == 0);
        pollset_destroy(ospoll->ps);
        free(ospoll->fds);
        free(ospoll);
    }
#endif
#if EPOLL || PORT
    if (ospoll) {
        assert (ospoll->num == 0);
        close(ospoll->epoll_fd);
        ospoll_clean_deleted(ospoll);
        free(ospoll->fds);
        free(ospoll);
    }
#endif
#if POLL
    if (ospoll) {
        assert (ospoll->num == 0);
        free (ospoll->fds);
        free (ospoll->osfds);
        free (ospoll);
    }
#endif
}

Bool
ospoll_add(struct ospoll *ospoll, int fd,
           enum ospoll_trigger trigger,
           void (*callback)(int fd, int xevents, void *data),
           void *data)
{
    int pos = ospoll_find(ospoll, fd);
#if POLLSET
    if (pos < 0) {
        if (ospoll->num == ospoll->size) {
            struct ospollfd *new_fds;
            int new_size = ospoll->size ? ospoll->size * 2 : MAXCLIENTS * 2;

            new_fds = reallocarray(ospoll->fds, new_size, sizeof (ospoll->fds[0]));
            if (!new_fds)
                return FALSE;
            ospoll->fds = new_fds;
            ospoll->size = new_size;
        }
        pos = -pos - 1;
        array_insert(ospoll->fds, ospoll->num, sizeof (ospoll->fds[0]), pos);
        ospoll->num++;

        ospoll->fds[pos].fd = fd;
        ospoll->fds[pos].xevents = 0;
        ospoll->fds[pos].revents = 0;
    }
    ospoll->fds[pos].trigger = trigger;
    ospoll->fds[pos].callback = callback;
    ospoll->fds[pos].data = data;
#endif
#if PORT
    struct ospollfd *osfd;

    if (pos < 0) {
        osfd = calloc(1, sizeof (struct ospollfd));
        if (!osfd)
            return FALSE;

        if (ospoll->num >= ospoll->size) {
            struct ospollfd **new_fds;
            int new_size = ospoll->size ? ospoll->size * 2 : MAXCLIENTS * 2;

            new_fds = reallocarray(ospoll->fds, new_size, sizeof (ospoll->fds[0]));
            if (!new_fds) {
                free (osfd);
                return FALSE;
            }
            ospoll->fds = new_fds;
            ospoll->size = new_size;
        }

        osfd->fd = fd;
        osfd->xevents = 0;

        pos = -pos - 1;
        array_insert(ospoll->fds, ospoll->num, sizeof (ospoll->fds[0]), pos);
        ospoll->fds[pos] = osfd;
        ospoll->num++;
    } else {
        osfd = ospoll->fds[pos];
    }
    osfd->data = data;
    osfd->callback = callback;
    osfd->trigger = trigger;
#endif
#if EPOLL
    struct ospollfd *osfd;

    if (pos < 0) {

        struct epoll_event ev;

        osfd = calloc(1, sizeof (struct ospollfd));
        if (!osfd)
            return FALSE;

        if (ospoll->num >= ospoll->size) {
            struct ospollfd **new_fds;
            int new_size = ospoll->size ? ospoll->size * 2 : MAXCLIENTS * 2;

            new_fds = reallocarray(ospoll->fds, new_size, sizeof (ospoll->fds[0]));
            if (!new_fds) {
                free (osfd);
                return FALSE;
            }
            ospoll->fds = new_fds;
            ospoll->size = new_size;
        }

        ev.events = 0;
        ev.data.ptr = osfd;
        if (trigger == ospoll_trigger_edge)
            ev.events |= EPOLLET;
        if (epoll_ctl(ospoll->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            free(osfd);
            return FALSE;
        }
        osfd->fd = fd;
        osfd->xevents = 0;

        pos = -pos - 1;
        array_insert(ospoll->fds, ospoll->num, sizeof (ospoll->fds[0]), pos);
        ospoll->fds[pos] = osfd;
        ospoll->num++;
    } else {
        osfd = ospoll->fds[pos];
    }
    osfd->data = data;
    osfd->callback = callback;
    osfd->trigger = trigger;
#endif
#if POLL
    if (pos < 0) {
        if (ospoll->num == ospoll->size) {
            struct pollfd   *new_fds;
            struct ospollfd *new_osfds;
            int             new_size = ospoll->size ? ospoll->size * 2 : MAXCLIENTS * 2;

            new_fds = reallocarray(ospoll->fds, new_size, sizeof (ospoll->fds[0]));
            if (!new_fds)
                return FALSE;
            ospoll->fds = new_fds;
            new_osfds = reallocarray(ospoll->osfds, new_size, sizeof (ospoll->osfds[0]));
            if (!new_osfds)
                return FALSE;
            ospoll->osfds = new_osfds;
            ospoll->size = new_size;
        }
        pos = -pos - 1;
        array_insert(ospoll->fds, ospoll->num, sizeof (ospoll->fds[0]), pos);
        array_insert(ospoll->osfds, ospoll->num, sizeof (ospoll->osfds[0]), pos);
        ospoll->num++;
        ospoll->changed = TRUE;

        ospoll->fds[pos].fd = fd;
        ospoll->fds[pos].events = 0;
        ospoll->fds[pos].revents = 0;
        ospoll->osfds[pos].revents = 0;
    }
    ospoll->osfds[pos].trigger = trigger;
    ospoll->osfds[pos].callback = callback;
    ospoll->osfds[pos].data = data;
#endif
    return TRUE;
}

void
ospoll_remove(struct ospoll *ospoll, int fd)
{
    int pos = ospoll_find(ospoll, fd);

    pos = ospoll_find(ospoll, fd);
    if (pos >= 0) {
#if POLLSET
        struct ospollfd *osfd = &ospoll->fds[pos];
        struct poll_ctl ctl = { .cmd = PS_DELETE, .fd = fd };
        pollset_ctl(ospoll->ps, &ctl, 1);

        array_delete(ospoll->fds, ospoll->num, sizeof (ospoll->fds[0]), pos);
        ospoll->num--;
#endif
#if PORT
        struct ospollfd *osfd = ospoll->fds[pos];
        port_dissociate(ospoll->epoll_fd, PORT_SOURCE_FD, fd);

        array_delete(ospoll->fds, ospoll->num, sizeof (ospoll->fds[0]), pos);
        ospoll->num--;
        osfd->callback = NULL;
        osfd->data = NULL;
        xorg_list_add(&osfd->deleted, &ospoll->deleted);
#endif
#if EPOLL
        struct ospollfd *osfd = ospoll->fds[pos];
        struct epoll_event ev;
        ev.events = 0;
        ev.data.ptr = osfd;
        (void) epoll_ctl(ospoll->epoll_fd, EPOLL_CTL_DEL, fd, &ev);

        array_delete(ospoll->fds, ospoll->num, sizeof (ospoll->fds[0]), pos);
        ospoll->num--;
        osfd->callback = NULL;
        osfd->data = NULL;
        xorg_list_add(&osfd->deleted, &ospoll->deleted);
#endif
#if POLL
        array_delete(ospoll->fds, ospoll->num, sizeof (ospoll->fds[0]), pos);
        array_delete(ospoll->osfds, ospoll->num, sizeof (ospoll->osfds[0]), pos);
        ospoll->num--;
        ospoll->changed = TRUE;
#endif
    }
}

#if PORT
static void
epoll_mod(struct ospoll *ospoll, struct ospollfd *osfd)
{
    int events = 0;
    if (osfd->xevents & X_NOTIFY_READ)
        events |= POLLIN;
    if (osfd->xevents & X_NOTIFY_WRITE)
        events |= POLLOUT;
    port_associate(ospoll->epoll_fd, PORT_SOURCE_FD, osfd->fd, events, osfd);
}
#endif

#if EPOLL
static void
epoll_mod(struct ospoll *ospoll, struct ospollfd *osfd)
{
    struct epoll_event ev;
    ev.events = 0;
    if (osfd->xevents & X_NOTIFY_READ)
        ev.events |= EPOLLIN;
    if (osfd->xevents & X_NOTIFY_WRITE)
        ev.events |= EPOLLOUT;
    if (osfd->trigger == ospoll_trigger_edge)
        ev.events |= EPOLLET;
    ev.data.ptr = osfd;
    (void) epoll_ctl(ospoll->epoll_fd, EPOLL_CTL_MOD, osfd->fd, &ev);
}
#endif

void
ospoll_listen(struct ospoll *ospoll, int fd, int xevents)
{
    int pos = ospoll_find(ospoll, fd);

    if (pos >= 0) {
#if POLLSET
        struct poll_ctl ctl = { .cmd = PS_MOD, .fd = fd };
        if (xevents & X_NOTIFY_READ) {
            ctl.events |= POLLIN;
            ospoll->fds[pos].revents &= ~POLLIN;
        }
        if (xevents & X_NOTIFY_WRITE) {
            ctl.events |= POLLOUT;
            ospoll->fds[pos].revents &= ~POLLOUT;
        }
        pollset_ctl(ospoll->ps, &ctl, 1);
        ospoll->fds[pos].xevents |= xevents;
#endif
#if EPOLL || PORT
        struct ospollfd *osfd = ospoll->fds[pos];
        osfd->xevents |= xevents;
        epoll_mod(ospoll, osfd);
#endif
#if POLL
        if (xevents & X_NOTIFY_READ) {
            ospoll->fds[pos].events |= POLLIN;
            ospoll->osfds[pos].revents &= ~POLLIN;
        }
        if (xevents & X_NOTIFY_WRITE) {
            ospoll->fds[pos].events |= POLLOUT;
            ospoll->osfds[pos].revents &= ~POLLOUT;
        }
#endif
    }
}

void
ospoll_mute(struct ospoll *ospoll, int fd, int xevents)
{
    int pos = ospoll_find(ospoll, fd);

    if (pos >= 0) {
#if POLLSET
        struct ospollfd *osfd = &ospoll->fds[pos];
        osfd->xevents &= ~xevents;
        struct poll_ctl ctl = { .cmd = PS_DELETE, .fd = fd };
        pollset_ctl(ospoll->ps, &ctl, 1);
        if (osfd->xevents) {
            ctl.cmd = PS_ADD;
            if (osfd->xevents & X_NOTIFY_READ) {
                ctl.events |= POLLIN;
            }
            if (osfd->xevents & X_NOTIFY_WRITE) {
                ctl.events |= POLLOUT;
            }
            pollset_ctl(ospoll->ps, &ctl, 1);
        }
#endif
#if EPOLL || PORT
        struct ospollfd *osfd = ospoll->fds[pos];
        osfd->xevents &= ~xevents;
        epoll_mod(ospoll, osfd);
#endif
#if POLL
        if (xevents & X_NOTIFY_READ)
            ospoll->fds[pos].events &= ~POLLIN;
        if (xevents & X_NOTIFY_WRITE)
            ospoll->fds[pos].events &= ~POLLOUT;
#endif
    }
}


int
ospoll_wait(struct ospoll *ospoll, int timeout)
{
    int nready;
#if POLLSET
#define MAX_EVENTS      256
    struct pollfd events[MAX_EVENTS];

    nready = pollset_poll(ospoll->ps, events, MAX_EVENTS, timeout);
    for (int i = 0; i < nready; i++) {
        struct pollfd *ev = &events[i];
        int pos = ospoll_find(ospoll, ev->fd);
        struct ospollfd *osfd = &ospoll->fds[pos];
        short revents = ev->revents;
        short oldevents = osfd->revents;

        osfd->revents = (revents & (POLLIN|POLLOUT));
        if (osfd->trigger == ospoll_trigger_edge)
            revents &= ~oldevents;
        if (revents) {
            int xevents = 0;
            if (revents & POLLIN)
                xevents |= X_NOTIFY_READ;
            if (revents & POLLOUT)
                xevents |= X_NOTIFY_WRITE;
            if (revents & (~(POLLIN|POLLOUT)))
                xevents |= X_NOTIFY_ERROR;
            osfd->callback(osfd->fd, xevents, osfd->data);
        }
    }
#endif
#if PORT
#define MAX_EVENTS      256
    port_event_t events[MAX_EVENTS];
    uint_t nget = 1;
    timespec_t port_timeout = {
        .tv_sec = timeout / 1000,
        .tv_nsec = (timeout % 1000) * 1000000
    };

    nready = 0;
    if (port_getn(ospoll->epoll_fd, events, MAX_EVENTS, &nget, &port_timeout)
        == 0) {
        nready = nget;
    }
    for (int i = 0; i < nready; i++) {
        port_event_t *ev = &events[i];
        struct ospollfd *osfd = ev->portev_user;
        uint32_t revents = ev->portev_events;
        int xevents = 0;

        if (revents & POLLIN)
            xevents |= X_NOTIFY_READ;
        if (revents & POLLOUT)
            xevents |= X_NOTIFY_WRITE;
        if (revents & (~(POLLIN|POLLOUT)))
            xevents |= X_NOTIFY_ERROR;

        if (osfd->callback)
            osfd->callback(osfd->fd, xevents, osfd->data);

        if (osfd->trigger == ospoll_trigger_level &&
            !xorg_list_is_empty(&osfd->deleted)) {
            epoll_mod(ospoll, osfd);
        }
    }
    ospoll_clean_deleted(ospoll);
#endif
#if EPOLL
#define MAX_EVENTS      256
    struct epoll_event events[MAX_EVENTS];
    int i;

    nready = epoll_wait(ospoll->epoll_fd, events, MAX_EVENTS, timeout);
    for (i = 0; i < nready; i++) {
        struct epoll_event *ev = &events[i];
        struct ospollfd *osfd = ev->data.ptr;
        uint32_t revents = ev->events;
        int xevents = 0;

        if (revents & EPOLLIN)
            xevents |= X_NOTIFY_READ;
        if (revents & EPOLLOUT)
            xevents |= X_NOTIFY_WRITE;
        if (revents & (~(EPOLLIN|EPOLLOUT)))
            xevents |= X_NOTIFY_ERROR;

        if (osfd->callback)
            osfd->callback(osfd->fd, xevents, osfd->data);
    }
    ospoll_clean_deleted(ospoll);
#endif
#if POLL
    nready = xserver_poll(ospoll->fds, ospoll->num, timeout);
    ospoll->changed = FALSE;
    if (nready > 0) {
        int f;
        for (f = 0; f < ospoll->num; f++) {
            short revents = ospoll->fds[f].revents;
            short oldevents = ospoll->osfds[f].revents;

            ospoll->osfds[f].revents = (revents & (POLLIN|POLLOUT));
            if (ospoll->osfds[f].trigger == ospoll_trigger_edge)
                revents &= ~oldevents;
            if (revents) {
                int    xevents = 0;
                if (revents & POLLIN)
                    xevents |= X_NOTIFY_READ;
                if (revents & POLLOUT)
                    xevents |= X_NOTIFY_WRITE;
                if (revents & (~(POLLIN|POLLOUT)))
                    xevents |= X_NOTIFY_ERROR;
                ospoll->osfds[f].callback(ospoll->fds[f].fd, xevents,
                                          ospoll->osfds[f].data);

                /* Check to see if the arrays have changed, and just go back
                 * around again
                 */
                if (ospoll->changed)
                    break;
            }
        }
    }
#endif
    return nready;
}

void
ospoll_reset_events(struct ospoll *ospoll, int fd)
{
#if POLLSET
    int pos = ospoll_find(ospoll, fd);

    if (pos < 0)
        return;

    ospoll->fds[pos].revents = 0;
#endif
#if PORT
    int pos = ospoll_find(ospoll, fd);

    if (pos < 0)
        return;

    epoll_mod(ospoll, ospoll->fds[pos]);
#endif
#if POLL
    int pos = ospoll_find(ospoll, fd);

    if (pos < 0)
        return;

    ospoll->osfds[pos].revents = 0;
#endif
}

void *
ospoll_data(struct ospoll *ospoll, int fd)
{
    int pos = ospoll_find(ospoll, fd);

    if (pos < 0)
        return NULL;
#if POLLSET
    return ospoll->fds[pos].data;
#endif
#if EPOLL || PORT
    return ospoll->fds[pos]->data;
#endif
#if POLL
    return ospoll->osfds[pos].data;
#endif
}
