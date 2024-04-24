/* Copyright (c) 2021 Apple Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#include <assert.h>
#include <mach-o/dyld.h>
#include <libgen.h>
#include <spawn.h>
#include <sys/syslimits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* We wnt XQuartz.app to inherit a login shell environment.  This is handled by the X11.sh
 * script which re-execs the main binary from a login shell environment.  However, recent
 * versions of macOS require that the main executable of an app be Mach-O for full system
 * fidelity.
 *
 * Failure to do so results in two problems:
 *    1) bash is seen as the responsible executable for Security & Privacy, and the user doesn't
 *       get prompted to allow filesystem access (https://github.com/XQuartz/XQuartz/issues/6).
 *    2) The process is launched under Rosetta for compatability, which results in
 *       the subsequent spawn of the real executable under Rosetta rather than natively.
 *
 * This trampoline provides the mach-o needed by LaunchServices and TCC to satisfy those
 * needs and simply execs the startup script which then execs the main binary.
 */

static char *executable_path() {
    uint32_t bufsize = PATH_MAX;
    char *buf = calloc(1, bufsize);

    if (_NSGetExecutablePath(buf, &bufsize) == -1) {
        free(buf);
        buf = calloc(1, bufsize);
        assert(_NSGetExecutablePath(buf, &bufsize) == 0);
    }

    return buf;
}

int main(int argc, char **argv, char **envp) {
    char const * const executable_directory = dirname(executable_path());
    char *executable = NULL;

    asprintf(&executable, "%s/X11.sh", executable_directory);
    if (access(executable, X_OK) == -1) {
        free(executable);
        asprintf(&executable, "%s/X11", executable_directory);
    }
    assert(access(executable, X_OK) == 0);

    argv[0] = executable;

    posix_spawnattr_t attr;
    assert(posix_spawnattr_init(&attr) == 0);
    assert(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETEXEC) == 0);

    pid_t child_pid;
    assert(posix_spawn(&child_pid, executable, NULL, &attr, argv, envp) == 0);

    return EXIT_FAILURE;
}
