/*
 * Copyright (c) 2001 Daniel Eischen <deischen@FreeBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/libc/include/un-namespace.h,v 1.13 2003/05/01 19:03:13 nectar Exp $
 */

#ifndef _UN_NAMESPACE_H_
#define _UN_NAMESPACE_H_

#undef		accept
#undef		__acl_aclcheck_fd
#undef		__acl_delete_fd
#undef		__acl_get_fd
#undef		__acl_set_fd
#undef		bind
#undef		__cap_get_fd
#undef		__cap_set_fd
#undef		close
#undef		connect
#undef		dup
#undef		dup2
#undef		execve
#undef		fcntl
#undef		flock
#undef		flockfile
#undef		fpathconf
#undef		fstat
#undef		fstatfs
#undef		fsync
#undef		funlockfile
#undef		getdirentries
#undef		getlogin
#undef		getpeername
#undef		getprogname
#undef		getsockname
#undef		getsockopt
#undef		ioctl
#undef		kevent
#undef		listen
#undef		nanosleep
#undef		open
#undef		poll
#undef		pthread_cond_broadcast
#undef		pthread_cond_destroy
#undef		pthread_cond_init
#undef		pthread_cond_signal
#undef		pthread_cond_timedwait
#undef		pthread_cond_wait
#undef		pthread_exit
#undef		pthread_getspecific
#undef		pthread_key_create
#undef		pthread_key_delete
#undef		pthread_main_np
#undef		pthread_mutex_destroy
#undef		pthread_mutex_init
#undef		pthread_mutex_lock
#undef		pthread_mutex_trylock
#undef		pthread_mutex_unlock
#undef		pthread_mutexattr_init
#undef		pthread_mutexattr_destroy
#undef		pthread_mutexattr_settype
#undef		pthread_once
#undef		pthread_rwlock_destroy
#undef		pthread_rwlock_init
#undef		pthread_rwlock_rdlock
#undef		pthread_rwlock_wrlock
#undef		pthread_rwlock_tryrdlock
#undef		pthread_rwlock_trywrlock
#undef		pthread_rwlock_unlock
#undef		pthread_self
#undef		pthread_setspecific
#undef		pthread_sigmask
#undef		read
#undef		readv
#undef		recvfrom
#undef		recvmsg
#undef		select
#undef		sendmsg
#undef		sendto
#undef		setsockopt
#undef		sigaction
#undef		sigprocmask
#undef		sigsuspend
#undef		socket
#undef		socketpair
#undef		wait4
#undef		waitpid
#undef		write
#undef		writev

#if 0
#undef		creat
#undef		fchflags
#undef		fchmod
#undef		ftrylockfile
#undef		msync
#undef		nfssvc
#undef		pause
#undef		pthread_rwlockattr_init
#undef		pthread_rwlockattr_destroy
#undef		sched_yield
#undef		sendfile
#undef		shutdown
#undef		sigaltstack
#undef		sigpending
#undef		sigreturn
#undef		sigsetmask
#undef		sleep
#undef		system
#undef		tcdrain
#undef		wait
#endif	/* 0 */

#ifdef _SIGNAL_H_
int     	_sigaction(int, const struct sigaction *, struct sigaction *);
#endif

#ifdef _SYS_EVENT_H_
int		_kevent(int, const struct kevent *, int, struct kevent *,
		    int, const struct timespec *);
#endif

#ifdef _SYS_FCNTL_H_
int		_flock(int, int);
#endif

#undef		err
#undef		warn
#undef		nsdispatch

#endif	/* _UN_NAMESPACE_H_ */
