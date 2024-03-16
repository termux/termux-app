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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "xshmfenceint.h"

/**
 * xshmfence_trigger:
 * @f: An X fence
 *
 * Set @f to triggered, waking all waiters.
 *
 * Return value: 0 on success and -1 on error (in which case, errno
 * will be set as appropriate).
 **/
int
xshmfence_trigger(struct xshmfence *f) {
    pthread_mutex_lock(&f->lock);
    if (f->value == 0) {
        f->value = 1;
        if (f->waiting) {
            f->waiting = 0;
            pthread_cond_broadcast(&f->wakeup);
        }
    }
    pthread_mutex_unlock(&f->lock);
    return 0;
}

/**
 * xshmfence_await:
 * @f: An X fence
 *
 * Wait for @f to be triggered. If @f is already triggered, this
 * function returns immediately.
 *
 * Return value: 0 on success and -1 on error (in which case, errno
 * will be set as appropriate).
 **/
int
xshmfence_await(struct xshmfence *f) {
    pthread_mutex_lock(&f->lock);
    while (f->value == 0) {
        f->waiting = 1;
        pthread_cond_wait(&f->wakeup, &f->lock);
    }
    pthread_mutex_unlock(&f->lock);
    return 0;
}

/**
 * xshmfence_query:
 * @f: An X fence
 *
 * Return value: 1 if @f is triggered, else returns 0.
 **/
int
xshmfence_query(struct xshmfence *f) {
    int value;

    pthread_mutex_lock(&f->lock);
    value = f->value;
    pthread_mutex_unlock(&f->lock);
    return value;
}

/**
 * xshmfence_reset:
 * @f: An X fence
 *
 * Reset @f to untriggered. If @f is already untriggered,
 * this function has no effect.
 **/
void
xshmfence_reset(struct xshmfence *f) {

    pthread_mutex_lock(&f->lock);
    f->value = 0;
    pthread_mutex_unlock(&f->lock);
}

/**
 * xshmfence_init:
 * @fd: An fd for an X fence
 *
 * Initialize the fence when first allocated
 **/

void
xshmfence_init(int fd)
{
    struct xshmfence *f = xshmfence_map_shm(fd);
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;

    if (!f)
        return;

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&f->lock, &mutex_attr);

    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&f->wakeup, &cond_attr);
    f->value = 0;
    f->waiting = 0;
    xshmfence_unmap_shm(f);
}
