/*
 * debug.h -- debugging routines for libtirpc
 *
 * Copyright (c) 2020 SUSE LINUX GmbH, Nuernberg, Germany.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _CLNT_FD_LOCKS_H
#define _CLNT_FD_LOCKS_H

#include <sys/queue.h>
#include <errno.h>
#include <reentrant.h>
#include <rpc/xdr.h>


/*
 * This utility manages a list of per-fd locks for the clients.
 *
 * If MAX_FDLOCKS_PREALLOC is defined, a number of pre-fd locks will be
 * pre-allocated. This number is the minimum of MAX_FDLOCKS_PREALLOC or
 * the process soft limit of allowed fds.
 */
#ifdef MAX_FDLOCKS_PREALLOC
static unsigned int fd_locks_prealloc = 0;
#endif

/* per-fd lock */
struct fd_lock_t {
	bool_t active;
	cond_t cv;
};
typedef struct fd_lock_t fd_lock_t;


/* internal type to store per-fd locks in a list */
struct fd_lock_item_t {
	/* fd_lock_t first so we can cast to fd_lock_item_t */
	fd_lock_t fd_lock;
	int fd;
	unsigned int refs;
	TAILQ_ENTRY(fd_lock_item_t) link;
};
typedef struct fd_lock_item_t fd_lock_item_t;
#define to_fd_lock_item(fdlock_t_ptr) ((fd_lock_item_t*) fdlock_t_ptr)


/* internal list of per-fd locks */
typedef TAILQ_HEAD(,fd_lock_item_t) fd_lock_list_t;


#ifdef MAX_FDLOCKS_PREALLOC

/* With pre-allocation, keep track of both an array and a list */
struct fd_locks_t {
	fd_lock_list_t fd_lock_list;
	fd_lock_t *fd_lock_array;
};
typedef struct fd_locks_t fd_locks_t;
#define to_fd_lock_list(fd_locks_t_ptr) (&fd_locks_t_ptr->fd_lock_list)

#else

/* With no pre-allocation, just keep track of a list */
typedef fd_lock_list_t fd_locks_t;
#define to_fd_lock_list(fd_locks_t_ptr) ((fd_lock_list_t *) fd_locks_t_ptr)

#endif


/* allocate fd locks */
static inline
fd_locks_t* fd_locks_init() {
	fd_locks_t *fd_locks;

	fd_locks = (fd_locks_t *) mem_alloc(sizeof(fd_locks_t));
	if (fd_locks == (fd_locks_t *) NULL) {
		errno = ENOMEM;
		return (NULL);
	}
	TAILQ_INIT(to_fd_lock_list(fd_locks));

#ifdef MAX_FDLOCKS_PREALLOC
	size_t fd_lock_arraysz;

	if (fd_locks_prealloc == 0) {
		unsigned int dtbsize = __rpc_dtbsize();
		if (0 < dtbsize && dtbsize < MAX_FDLOCKS_PREALLOC)
			fd_locks_prealloc = dtbsize;
		else
			fd_locks_prealloc = MAX_FDLOCKS_PREALLOC;
	}

	if ( (size_t) fd_locks_prealloc > SIZE_MAX/sizeof(fd_lock_t)) {
		mem_free(fd_locks, sizeof (*fd_locks));
		errno = EOVERFLOW;
		return (NULL);
	}

	fd_lock_arraysz = fd_locks_prealloc * sizeof (fd_lock_t);
	fd_locks->fd_lock_array = (fd_lock_t *) mem_alloc(fd_lock_arraysz);
	if (fd_locks->fd_lock_array == (fd_lock_t *) NULL) {
		mem_free(fd_locks, sizeof (*fd_locks));
		errno = ENOMEM;
		return (NULL);
	}
	else {
		int i;

		for (i = 0; i < fd_locks_prealloc; i++) {
			fd_locks->fd_lock_array[i].active = FALSE;
			cond_init(&fd_locks->fd_lock_array[i].cv, 0, (void *) 0);
		}
	}
#endif

	return fd_locks;
}

/* de-allocate fd locks */
static inline
void fd_locks_destroy(fd_locks_t *fd_locks) {
#ifdef MAX_FDLOCKS_PREALLOC
	fd_lock_t *array = fd_locks->fd_lock_array;
	mem_free(array, fd_locks_prealloc * sizeof (fd_lock_t));
#endif
	fd_lock_item_t *item;
	fd_lock_list_t *list = to_fd_lock_list(fd_locks);

	TAILQ_FOREACH(item, list, link) {
		cond_destroy(&item->fd_lock.cv);
		mem_free(item, sizeof (*item));
	}
	mem_free(fd_locks, sizeof (*fd_locks));
}

/* allocate per-fd lock */
static inline
fd_lock_t* fd_lock_create(int fd, fd_locks_t *fd_locks) {
#ifdef MAX_FDLOCKS_PREALLOC
	if (fd < fd_locks_prealloc) {
		return &fd_locks->fd_lock_array[fd];
	}
#endif
	fd_lock_item_t *item;
	fd_lock_list_t *list = to_fd_lock_list(fd_locks);

	for (item = TAILQ_FIRST(list);
	     item != (fd_lock_item_t *) NULL && item->fd != fd;
	     item = TAILQ_NEXT(item, link));

	if (item == (fd_lock_item_t *) NULL) {
		item = (fd_lock_item_t *) mem_alloc(sizeof(fd_lock_item_t));
		if (item == (fd_lock_item_t *) NULL) {
			errno = ENOMEM;
			return (NULL);
		}
		item->fd = fd;
		item->refs = 1;
		item->fd_lock.active = FALSE;
		cond_init(&item->fd_lock.cv, 0, (void *) 0);
		TAILQ_INSERT_HEAD(list, item, link);
	} else {
		item->refs++;
	}
	return &item->fd_lock;
}

/* de-allocate per-fd lock */
static inline
void fd_lock_destroy(int fd, fd_lock_t *fd_lock, fd_locks_t *fd_locks) {
#ifdef MAX_FDLOCKS_PREALLOC
	if (fd < fd_locks_prealloc)
		return;
#endif
	fd_lock_item_t* item = to_fd_lock_item(fd_lock);
	item->refs--;
	if (item->refs <= 0) {
		TAILQ_REMOVE(to_fd_lock_list(fd_locks), item, link);
		cond_destroy(&item->fd_lock.cv);
		mem_free(item, sizeof (*item));
	}
}

#endif /* _CLNT_FD_LOCKS_H */
