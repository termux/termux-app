/*
 * Copyright 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "os.h"
#include "misc.h"
#include <errno.h>
#include <string.h>

#ifdef HAVE_LIBUNWIND

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

void
xorg_backtrace(void)
{
    unw_cursor_t cursor;
    unw_context_t context;
    unw_word_t ip;
    unw_word_t off;
    unw_proc_info_t pip;
    int ret, i = 0;
    char procname[256];
    const char *filename;
    Dl_info dlinfo;

    pip.unwind_info = NULL;
    ret = unw_getcontext(&context);
    if (ret) {
        ErrorFSigSafe("unw_getcontext failed: %s [%d]\n", unw_strerror(ret),
                ret);
        return;
    }

    ret = unw_init_local(&cursor, &context);
    if (ret) {
        ErrorFSigSafe("unw_init_local failed: %s [%d]\n", unw_strerror(ret),
                ret);
        return;
    }

    ErrorFSigSafe("\n");
    ErrorFSigSafe("Backtrace:\n");
    ret = unw_step(&cursor);
    while (ret > 0) {
        ret = unw_get_proc_info(&cursor, &pip);
        if (ret) {
            ErrorFSigSafe("unw_get_proc_info failed: %s [%d]\n",
                    unw_strerror(ret), ret);
            break;
        }

        off = 0;
        ret = unw_get_proc_name(&cursor, procname, 256, &off);
        if (ret && ret != -UNW_ENOMEM) {
            if (ret != -UNW_EUNSPEC)
                ErrorFSigSafe("unw_get_proc_name failed: %s [%d]\n",
                        unw_strerror(ret), ret);
            procname[0] = '?';
            procname[1] = 0;
        }

        if (unw_get_reg (&cursor, UNW_REG_IP, &ip) < 0)
          ip = pip.start_ip + off;
        if (dladdr((void *)(uintptr_t)(ip), &dlinfo) && dlinfo.dli_fname &&
                *dlinfo.dli_fname)
            filename = dlinfo.dli_fname;
        else
            filename = "?";

        ErrorFSigSafe("%u: %s (%s%s+0x%x) [%p]\n", i++, filename, procname,
            ret == -UNW_ENOMEM ? "..." : "", (int)off,
            (void *)(uintptr_t)(ip));

        ret = unw_step(&cursor);
        if (ret < 0)
            ErrorFSigSafe("unw_step failed: %s [%d]\n", unw_strerror(ret), ret);
    }
    ErrorFSigSafe("\n");
}
#else /* HAVE_LIBUNWIND */
#ifdef HAVE_BACKTRACE
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <execinfo.h>

void
xorg_backtrace(void)
{
    const int BT_SIZE = 64;
    void *array[BT_SIZE];
    const char *mod;
    int size, i;
    Dl_info info;

    ErrorFSigSafe("\n");
    ErrorFSigSafe("Backtrace:\n");
    size = backtrace(array, BT_SIZE);
    for (i = 0; i < size; i++) {
        int rc = dladdr(array[i], &info);

        if (rc == 0) {
            ErrorFSigSafe("%u: ?? [%p]\n", i, array[i]);
            continue;
        }
        mod = (info.dli_fname && *info.dli_fname) ? info.dli_fname : "(vdso)";
        if (info.dli_saddr)
            ErrorFSigSafe(
                "%u: %s (%s+0x%x) [%p]\n",
                i,
                mod,
                info.dli_sname,
                (unsigned int)((char *) array[i] -
                               (char *) info.dli_saddr),
                array[i]);
        else
            ErrorFSigSafe(
                "%u: %s (%p+0x%x) [%p]\n",
                i,
                mod,
                info.dli_fbase,
                (unsigned int)((char *) array[i] -
                               (char *) info.dli_fbase),
                array[i]);
    }
    ErrorFSigSafe("\n");
}

#else                           /* not glibc or glibc < 2.1 */

#if defined(__sun) && defined(__SVR4)
#define HAVE_PSTACK
#endif

#if defined(HAVE_WALKCONTEXT)   /* Solaris 9 & later */

#include <ucontext.h>
#include <signal.h>
#include <dlfcn.h>
#include <sys/elf.h>

#ifdef _LP64
#define ElfSym Elf64_Sym
#else
#define ElfSym Elf32_Sym
#endif

/* Called for each frame on the stack to print its contents */
static int
xorg_backtrace_frame(uintptr_t pc, int signo, void *arg)
{
    Dl_info dlinfo;
    ElfSym *dlsym;
    char header[32];
    int depth = *((int *) arg);

    if (signo) {
        char signame[SIG2STR_MAX];

        if (sig2str(signo, signame) != 0) {
            strcpy(signame, "unknown");
        }

        ErrorFSigSafe("** Signal %u (%s)\n", signo, signame);
    }

    snprintf(header, sizeof(header), "%d: 0x%lx", depth, pc);
    *((int *) arg) = depth + 1;

    /* Ask system dynamic loader for info on the address */
    if (dladdr1((void *) pc, &dlinfo, (void **) &dlsym, RTLD_DL_SYMENT)) {
        unsigned long offset = pc - (uintptr_t) dlinfo.dli_saddr;
        const char *symname;

        if (offset < dlsym->st_size) {  /* inside a function */
            symname = dlinfo.dli_sname;
        }
        else {                  /* found which file it was in, but not which function */
            symname = "<section start>";
            offset = pc - (uintptr_t) dlinfo.dli_fbase;
        }
        ErrorFSigSafe("%s: %s:%s+0x%x\n", header, dlinfo.dli_fname, symname,
                     offset);

    }
    else {
        /* Couldn't find symbol info from system dynamic loader, should
         * probably poke elfloader here, but haven't written that code yet,
         * so we just print the pc.
         */
        ErrorFSigSafe("%s\n", header);
    }

    return 0;
}
#endif                          /* HAVE_WALKCONTEXT */

#ifdef HAVE_PSTACK
#include <unistd.h>

static int
xorg_backtrace_pstack(void)
{
    pid_t kidpid;
    int pipefd[2];

    if (pipe(pipefd) != 0) {
        return -1;
    }

    kidpid = fork1();

    if (kidpid == -1) {
        /* ERROR */
        return -1;
    }
    else if (kidpid == 0) {
        /* CHILD */
        char parent[16];

        seteuid(0);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        dup2(pipefd[1], STDOUT_FILENO);
        closefrom(STDERR_FILENO);

        snprintf(parent, sizeof(parent), "%d", getppid());
        execle("/usr/bin/pstack", "pstack", parent, NULL);
        exit(1);
    }
    else {
        /* PARENT */
        char btline[256];
        int kidstat;
        int bytesread;
        int done = 0;

        close(pipefd[1]);

        while (!done) {
            bytesread = read(pipefd[0], btline, sizeof(btline) - 1);

            if (bytesread > 0) {
                btline[bytesread] = 0;
                ErrorFSigSafe("%s", btline);
            }
            else if ((bytesread < 0) || ((errno != EINTR) && (errno != EAGAIN)))
                done = 1;
        }
        close(pipefd[0]);
        waitpid(kidpid, &kidstat, 0);
        if (kidstat != 0)
            return -1;
    }
    return 0;
}
#endif                          /* HAVE_PSTACK */

#if defined(HAVE_PSTACK) || defined(HAVE_WALKCONTEXT)

void
xorg_backtrace(void)
{

    ErrorFSigSafe("\n");
    ErrorFSigSafe("Backtrace:\n");

#ifdef HAVE_PSTACK
/* First try fork/exec of pstack - otherwise fall back to walkcontext
   pstack is preferred since it can print names of non-exported functions */

    if (xorg_backtrace_pstack() < 0)
#endif
    {
#ifdef HAVE_WALKCONTEXT
        ucontext_t u;
        int depth = 1;

        if (getcontext(&u) == 0)
            walkcontext(&u, xorg_backtrace_frame, &depth);
        else
#endif
            ErrorFSigSafe("Failed to get backtrace info: %s\n", strerror(errno));
    }
    ErrorFSigSafe("\n");
}

#else

/* Default fallback if we can't find any way to get a backtrace */
void
xorg_backtrace(void)
{
    return;
}

#endif
#endif
#endif
