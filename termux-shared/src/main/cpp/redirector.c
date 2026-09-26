#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <android/log.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/syscall.h>

#define LOG_TAG "InvappRedirector"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

/* Official Termux packages hardcode these path prefixes. */
static const char* OLD_DATA_PREFIX = "data/data/com.termux";
static const size_t OLD_DATA_PREFIX_LEN = 20; /* strlen("data/data/com.termux") */

static const char* OLD_USER_PREFIX = "data/user/0/com.termux";
static const size_t OLD_USER_PREFIX_LEN = 21; /* strlen("data/user/0/com.termux") */

/* Must match TermuxConstants.TERMUX_PACKAGE_NAME / applicationId. */
static const char* NEW_PREFIX = "/data/data/com.involvex.termux_app";
static const char* OLD_PKG = "com.termux";
static const char* NEW_PKG = "com.involvex.termux_app";
/* Lengths derived at runtime — do not hardcode (easy to get wrong). */

/*
 * Stock termux-api scripts use short components (package/.Class). Our API
 * applicationId is com.involvex.termux_app.api but Java/namespace is
 * com.invapp.api — short form resolves to the wrong class.
 */
static const char* OLD_API_LISTEN = "com.termux.api://listen";
static const char* NEW_API_LISTEN = "com.involvex.termux_app.api://listen";
static const char* OLD_API_COMPONENT = "com.termux.api/.TermuxApiReceiver";
static const char* NEW_API_COMPONENT =
    "com.involvex.termux_app.api/com.invapp.api.TermuxApiReceiver";
static const char* OLD_KEEPALIVE_COMPONENT = "com.termux.api/.KeepAliveService";
static const char* BROKEN_KEEPALIVE_COMPONENT =
    "com.involvex.termux_app.api/.KeepAliveService";
static const char* NEW_KEEPALIVE_COMPONENT =
    "com.involvex.termux_app.api/com.invapp.api.KeepAliveService";

static char* replace_all_alloc(const char* src, const char* from, const char* to);

/* Derived paths for SSH/login sessions (sshd clears LD_* in the child env). */
static const char* NEW_HOME = "/data/data/com.involvex.termux_app/files/home";
static const char* NEW_USR = "/data/data/com.involvex.termux_app/files/usr";
static const char* NEW_LIB = "/data/data/com.involvex.termux_app/files/usr/lib";
static const char* REDIRECTOR_SO =
    "/data/data/com.involvex.termux_app/files/usr/lib/libinvapp-redirector.so";

__attribute__((constructor))
static void redirector_init(void) {
    LOGI("loaded; redirecting com.termux → %s", NEW_PREFIX);
}

static const char* redirect_path(const char* path, char* buffer, size_t bufsize) {
    if (!path) return path;

    const char* p = path;
    /* Handle ./ prefix */
    if (p[0] == '.' && p[1] == '/') p += 2;
    /* Handle leading / */
    if (p[0] == '/') p++;

    if (strncmp(p, OLD_DATA_PREFIX, OLD_DATA_PREFIX_LEN) == 0) {
        snprintf(buffer, bufsize, "%s%s", NEW_PREFIX, p + OLD_DATA_PREFIX_LEN);
        return buffer;
    }
    if (strncmp(p, OLD_USER_PREFIX, OLD_USER_PREFIX_LEN) == 0) {
        snprintf(buffer, bufsize, "%s%s", NEW_PREFIX, p + OLD_USER_PREFIX_LEN);
        return buffer;
    }
    return path;
}

/*
 * Kernel shebang resolution ignores LD_PRELOAD. Stock termux-exec rewrites
 * #!/usr/bin/env, but when this redirector is first in LD_PRELOAD, RTLD_NEXT
 * often skips termux-exec → libc, so npm/npx die with:
 *   /usr/bin/env: bad interpreter: Permission denied / No such file
 *
 * Transform: #!/usr/bin/env CMD → execve($PREFIX/bin/env, [env, CMD, script, …])
 * Allocations intentionally leak on success (execve replaces the image).
 */
static int transform_usr_bin_env_shebang(const char* path, char* const argv[],
        char* interp_out, size_t interp_sz, char*** argv_out) {
    if (!path || !argv || !interp_out || interp_sz < 8 || !argv_out) return 0;
    *argv_out = NULL;

    static int (*orig_open)(const char*, int, ...);
    static ssize_t (*orig_read)(int, void*, size_t);
    static int (*orig_close)(int);
    if (!orig_open) orig_open = dlsym(RTLD_NEXT, "open");
    if (!orig_read) orig_read = dlsym(RTLD_NEXT, "read");
    if (!orig_close) orig_close = dlsym(RTLD_NEXT, "close");
    if (!orig_open || !orig_read || !orig_close) return 0;

    int fd = orig_open(path, O_RDONLY);
    if (fd < 0) return 0;

    char line[512];
    ssize_t n = orig_read(fd, line, sizeof(line) - 1);
    orig_close(fd);
    if (n < 4) return 0;
    line[n] = '\0';

    char* p = line;
    if (p[0] != '#' || p[1] != '!') return 0;
    p += 2;
    while (*p == ' ' || *p == '\t') p++;

    const char* env_paths[] = { "/usr/bin/env", "/bin/env", NULL };
    const char* matched = NULL;
    size_t matched_len = 0;
    for (int i = 0; env_paths[i]; i++) {
        size_t len = strlen(env_paths[i]);
        if (strncmp(p, env_paths[i], len) == 0
            && (p[len] == ' ' || p[len] == '\t' || p[len] == '\0'
                || p[len] == '\n' || p[len] == '\r')) {
            matched = env_paths[i];
            matched_len = len;
            break;
        }
    }
    if (!matched) return 0;

    p += matched_len;
    while (*p == ' ' || *p == '\t') p++;

    /* Strip CR/LF from shebang args. */
    char* end = p;
    while (*end && *end != '\n' && *end != '\r') end++;
    *end = '\0';
    if (*p == '\0') return 0; /* #!/usr/bin/env with no command */

    /* Build argv: env, <shebang-args…>, script, original argv[1…] */
    int shebang_argc = 0;
    char* tok_save = NULL;
    char* tmp = strdup(p);
    if (!tmp) return 0;
    for (char* t = strtok_r(tmp, " \t", &tok_save); t; t = strtok_r(NULL, " \t", &tok_save))
        shebang_argc++;
    free(tmp);
    if (shebang_argc <= 0) return 0;

    int orig_argc = 0;
    while (argv[orig_argc]) orig_argc++;
    /* env + shebang args + script + argv[1..] + NULL */
    int new_argc = 1 + shebang_argc + 1 + (orig_argc > 0 ? orig_argc - 1 : 0);
    char** new_argv = (char**)malloc((size_t)(new_argc + 1) * sizeof(char*));
    if (!new_argv) return 0;

    snprintf(interp_out, interp_sz, "%s/bin/env", NEW_USR);
    int ai = 0;
    new_argv[ai++] = strdup(interp_out);

    tok_save = NULL;
    tmp = strdup(p);
    if (!tmp) {
        free(new_argv);
        return 0;
    }
    for (char* t = strtok_r(tmp, " \t", &tok_save); t; t = strtok_r(NULL, " \t", &tok_save))
        new_argv[ai++] = strdup(t);
    free(tmp);

    new_argv[ai++] = strdup(path);
    for (int i = 1; i < orig_argc; i++)
        new_argv[ai++] = argv[i]; /* borrow original pointers */
    new_argv[ai] = NULL;

    *argv_out = new_argv;
    LOGI("rewrote shebang %s → %s for %s", matched, interp_out, path);
    return 1;
}

static int path_is_dpkg_maintainer_script(const char* path) {
    if (!path || strstr(path, "/var/lib/dpkg/") == NULL) {
        return 0;
    }
    if (strstr(path, "/var/lib/dpkg/tmp.ci/") != NULL) {
        return 1;
    }
    const char* slash = strrchr(path, '/');
    const char* name = slash ? slash + 1 : path;
    return strstr(name, ".preinst") != NULL || strstr(name, ".postinst") != NULL
        || strstr(name, ".prerm") != NULL || strstr(name, ".postrm") != NULL
        || strstr(name, ".config") != NULL || strcmp(name, "preinst") == 0
        || strcmp(name, "postinst") == 0 || strcmp(name, "prerm") == 0
        || strcmp(name, "postrm") == 0 || strcmp(name, "config") == 0;
}

/*
 * dpkg maintainer scripts may lack a shebang (e.g. openjdk-17 preinst) or ship
 * #!/data/data/com.termux/... interpreters. Kernel shebang resolution ignores
 * LD_PRELOAD, so patch scripts in-place immediately before execve.
 */
static void rewrite_termux_paths_in_script(const char* path) {
    if (!path) return;

    static int (*orig_open)(const char*, int, ...);
    static ssize_t (*orig_read)(int, void*, size_t);
    static ssize_t (*orig_write)(int, const void*, size_t);
    static off_t (*orig_lseek)(int, off_t, int);
    static int (*orig_close)(int);
    if (!orig_open) orig_open = dlsym(RTLD_NEXT, "open");
    if (!orig_read) orig_read = dlsym(RTLD_NEXT, "read");
    if (!orig_write) orig_write = dlsym(RTLD_NEXT, "write");
    if (!orig_lseek) orig_lseek = dlsym(RTLD_NEXT, "lseek");
    if (!orig_close) orig_close = dlsym(RTLD_NEXT, "close");

    int fd = orig_open(path, O_RDONLY);
    if (fd < 0) return;

    off_t size = orig_lseek(fd, 0, SEEK_END);
    if (size < 0 || size > 1024 * 1024) {
        orig_close(fd);
        return;
    }
    orig_lseek(fd, 0, SEEK_SET);

    char* data = (char*)malloc((size_t)size + 1);
    if (!data) {
        orig_close(fd);
        return;
    }
    if (orig_read(fd, data, (size_t)size) != size) {
        free(data);
        orig_close(fd);
        return;
    }
    orig_close(fd);
    data[size] = '\0';

    int has_shebang = size >= 2 && data[0] == '#' && data[1] == '!';
    int is_maint = path_is_dpkg_maintainer_script(path);
    if (!has_shebang && !is_maint) {
        free(data);
        return;
    }

    char* prefixed = NULL;
    const char* body = data;
    size_t body_len = (size_t)size;
    if (!has_shebang && is_maint) {
        /* NEW_USR is a pointer, not a string literal — build shebang explicitly. */
        const char* prefix = "#!";
        const char* suffix = "/bin/sh\n";
        size_t shebang_len = strlen(prefix) + strlen(NEW_USR) + strlen(suffix);
        prefixed = (char*)malloc(shebang_len + (size_t)size + 1);
        if (!prefixed) {
            free(data);
            return;
        }
        snprintf(prefixed, shebang_len + 1, "%s%s%s", prefix, NEW_USR, suffix);
        memcpy(prefixed + shebang_len, data, (size_t)size);
        prefixed[shebang_len + (size_t)size] = '\0';
        body = prefixed;
        body_len = shebang_len + (size_t)size;
        has_shebang = 1;
    }

    int has_old_pkg = strstr(body, OLD_PKG) != NULL;
    int has_broken_keepalive = strstr(body, BROKEN_KEEPALIVE_COMPONENT) != NULL
        || strstr(body, OLD_KEEPALIVE_COMPONENT) != NULL;
    if (!has_old_pkg && !has_broken_keepalive && body == data) {
        free(data);
        free(prefixed);
        return;
    }

    const size_t old_pkg_len = strlen(OLD_PKG);
    const size_t new_pkg_len = strlen(NEW_PKG);

    /* Worst-case expansion bound (package rewrite + KeepAlive FQCN). */
    size_t out_cap = body_len * (new_pkg_len + 1)
        + strlen(NEW_KEEPALIVE_COMPONENT) * 4 + 64;
    char* out = (char*)malloc(out_cap);
    if (!out) {
        free(data);
        free(prefixed);
        return;
    }

    size_t oi = 0;
    for (size_t i = 0; i < body_len; ) {
        if (i + old_pkg_len <= body_len
            && memcmp(body + i, OLD_PKG, old_pkg_len) == 0) {
            /*
             * Do not rewrite Java packages that live inside stock TermuxAm
             * (am.apk still ships com.termux.termuxam.Am). Intent actions like
             * com.termux.app.* still get rewritten.
             */
            const char* after = body + i + old_pkg_len;
            if (strncmp(after, ".termuxam", 9) == 0) {
                memcpy(out + oi, body + i, old_pkg_len);
                oi += old_pkg_len;
                i += old_pkg_len;
                continue;
            }
            memcpy(out + oi, NEW_PKG, new_pkg_len);
            oi += new_pkg_len;
            i += old_pkg_len;
        } else {
            out[oi++] = body[i++];
        }
    }
    out[oi] = '\0';

    /* After package rewrite, short KeepAlive components are wrong (namespace ≠ id). */
    char* fixed = replace_all_alloc(out, BROKEN_KEEPALIVE_COMPONENT, NEW_KEEPALIVE_COMPONENT);
    if (fixed) {
        free(out);
        out = fixed;
        oi = strlen(out);
    }
    fixed = replace_all_alloc(out, OLD_KEEPALIVE_COMPONENT, NEW_KEEPALIVE_COMPONENT);
    if (fixed) {
        free(out);
        out = fixed;
        oi = strlen(out);
    }

    fd = orig_open(path, O_WRONLY | O_TRUNC);
    if (fd >= 0) {
        if (orig_write(fd, out, oi) == (ssize_t)oi) {
            LOGI("rewrote package paths in script: %s", path);
        }
        orig_close(fd);
    }

    free(data);
    free(prefixed);
    free(out);
}

/*
 * Replace every occurrence of {@code from} with {@code to} in {@code src}.
 * Caller owns the returned buffer (or gets NULL on OOM / no-op when unchanged
 * is not requested — always returns a new string when src != NULL).
 */
static char* replace_all_alloc(const char* src, const char* from, const char* to) {
    if (!src) return NULL;
    size_t from_len = strlen(from);
    size_t to_len = strlen(to);
    if (from_len == 0) {
        char* copy = (char*)malloc(strlen(src) + 1);
        if (copy) memcpy(copy, src, strlen(src) + 1);
        return copy;
    }

    size_t count = 0;
    for (const char* p = src; (p = strstr(p, from)) != NULL; p += from_len) {
        count++;
    }
    size_t src_len = strlen(src);
    size_t out_len = src_len + count * (to_len - from_len);
    char* out = (char*)malloc(out_len + 1);
    if (!out) return NULL;

    char* dst = out;
    const char* cursor = src;
    for (;;) {
        const char* hit = strstr(cursor, from);
        if (!hit) {
            memcpy(dst, cursor, strlen(cursor) + 1);
            break;
        }
        size_t keep = (size_t)(hit - cursor);
        memcpy(dst, cursor, keep);
        dst += keep;
        memcpy(dst, to, to_len);
        dst += to_len;
        cursor = hit + from_len;
    }
    return out;
}

static int env_key_is(const char* entry, const char* key) {
    size_t key_len = strlen(key);
    return strncmp(entry, key, key_len) == 0 && entry[key_len] == '=';
}

static int envp_has_key(char* const* envp, const char* key) {
    if (!envp) return 0;
    for (size_t i = 0; envp[i]; i++) {
        if (env_key_is(envp[i], key)) return 1;
    }
    return 0;
}

/*
 * sshd builds a clean session environment (often without LD_LIBRARY_PATH /
 * LD_PRELOAD and with HOME=/data/data/com.termux/...). Inject / rewrite those
 * here so the child linker can resolve Termux libs under our real prefix.
 *
 * Do NOT inject LD_* when exec'ing system binaries (e.g. /system/bin/app_process
 * used by TermuxAm) — those intentionally unset LD_LIBRARY_PATH/LD_PRELOAD and
 * break if Termux libs are forced in.
 * Allocations intentionally leak on success — execve replaces the process image.
 */
static char** rewrite_envp_for_exec(const char* filename, char* const envp[]) {
    size_t count = 0;
    if (envp) {
        while (envp[count]) count++;
    }

    int is_termux_bin = 0;
    if (filename) {
        if (strncmp(filename, NEW_PREFIX, strlen(NEW_PREFIX)) == 0) {
            is_termux_bin = 1;
        } else {
            char pathbuf[4096];
            const char* redirected = redirect_path(filename, pathbuf, sizeof(pathbuf));
            if (redirected && strncmp(redirected, NEW_PREFIX, strlen(NEW_PREFIX)) == 0) {
                is_termux_bin = 1;
            }
        }
    }

    int need_home = !envp_has_key(envp, "HOME");
    int need_prefix = !envp_has_key(envp, "PREFIX");
    /* Only force LD_* for Termux-prefix executables (bash/dpkg/sshd session). */
    int need_ld_lib = is_termux_bin && !envp_has_key(envp, "LD_LIBRARY_PATH");
    int need_ld_pre = is_termux_bin && !envp_has_key(envp, "LD_PRELOAD");
    size_t extra = (size_t)(need_home + need_prefix + need_ld_lib + need_ld_pre);

    char** out = (char**)malloc((count + extra + 1) * sizeof(char*));
    if (!out) return (char**)envp;

    size_t oi = 0;
    for (size_t i = 0; i < count; i++) {
        const char* entry = envp[i];
        if (is_termux_bin && env_key_is(entry, "LD_LIBRARY_PATH")) {
            const char* val = entry + strlen("LD_LIBRARY_PATH=");
            char* rewritten = replace_all_alloc(val, OLD_PKG, NEW_PKG);
            if (!rewritten) {
                out[oi++] = (char*)entry;
                continue;
            }
            int has_new = strstr(rewritten, NEW_LIB) != NULL;
            size_t nlen = strlen(NEW_LIB) + 1 + strlen(rewritten) + 1
                + strlen("LD_LIBRARY_PATH=");
            char* merged = (char*)malloc(nlen + 8);
            if (!merged) {
                free(rewritten);
                out[oi++] = (char*)entry;
                continue;
            }
            if (has_new) {
                snprintf(merged, nlen + 8, "LD_LIBRARY_PATH=%s", rewritten);
            } else if (rewritten[0]) {
                snprintf(merged, nlen + 8, "LD_LIBRARY_PATH=%s:%s", NEW_LIB, rewritten);
            } else {
                snprintf(merged, nlen + 8, "LD_LIBRARY_PATH=%s", NEW_LIB);
            }
            free(rewritten);
            out[oi++] = merged;
            continue;
        }
        if (is_termux_bin && env_key_is(entry, "LD_PRELOAD")) {
            const char* val = entry + strlen("LD_PRELOAD=");
            char* rewritten = replace_all_alloc(val, OLD_PKG, NEW_PKG);
            if (!rewritten) {
                out[oi++] = (char*)entry;
                continue;
            }
            int has_red = strstr(rewritten, "libinvapp-redirector.so") != NULL;
            int has_bun_seccomp = strstr(rewritten, "libinvapp-bun-seccomp.so") != NULL;
            size_t nlen = strlen(REDIRECTOR_SO) + 1 + strlen(rewritten)
                + strlen("LD_PRELOAD=") + 8;
            char* merged = (char*)malloc(nlen);
            if (!merged) {
                free(rewritten);
                out[oi++] = (char*)entry;
                continue;
            }
            if (has_red || has_bun_seccomp) {
                /* Keep as-is: either already has redirector, or Bun intentionally
                 * preloads only the seccomp SIGSYS shim (must not reinject us). */
                snprintf(merged, nlen, "LD_PRELOAD=%s", rewritten);
            } else if (rewritten[0]) {
                snprintf(merged, nlen, "LD_PRELOAD=%s:%s", REDIRECTOR_SO, rewritten);
            } else {
                /* Explicit LD_PRELOAD= (empty): honor clear — Bun/native tooling
                 * must not inherit the path redirector (SIGSYS with linux-* .node). */
                free(rewritten);
                free(merged);
                out[oi++] = (char*)entry;
                continue;
            }
            free(rewritten);
            out[oi++] = merged;
            continue;
        }
        /* System binaries: leave LD_* alone (may be intentionally unset). */
        if (!is_termux_bin
            && (env_key_is(entry, "LD_LIBRARY_PATH") || env_key_is(entry, "LD_PRELOAD"))) {
            out[oi++] = (char*)entry;
            continue;
        }
        if (strstr(entry, OLD_PKG)) {
            char* rewritten = replace_all_alloc(entry, OLD_PKG, NEW_PKG);
            out[oi++] = rewritten ? rewritten : (char*)entry;
            continue;
        }
        out[oi++] = (char*)entry;
    }

    if (need_home) {
        size_t n = strlen("HOME=") + strlen(NEW_HOME) + 1;
        char* e = (char*)malloc(n);
        if (e) {
            snprintf(e, n, "HOME=%s", NEW_HOME);
            out[oi++] = e;
        }
    }
    if (need_prefix) {
        size_t n = strlen("PREFIX=") + strlen(NEW_USR) + 1;
        char* e = (char*)malloc(n);
        if (e) {
            snprintf(e, n, "PREFIX=%s", NEW_USR);
            out[oi++] = e;
        }
    }
    if (need_ld_lib) {
        size_t n = strlen("LD_LIBRARY_PATH=") + strlen(NEW_LIB) + 1;
        char* e = (char*)malloc(n);
        if (e) {
            snprintf(e, n, "LD_LIBRARY_PATH=%s", NEW_LIB);
            out[oi++] = e;
        }
    }
    if (need_ld_pre) {
        size_t n = strlen("LD_PRELOAD=") + strlen(REDIRECTOR_SO) + 1;
        char* e = (char*)malloc(n);
        if (e) {
            snprintf(e, n, "LD_PRELOAD=%s", REDIRECTOR_SO);
            out[oi++] = e;
        }
    }
    out[oi] = NULL;
    return out;
}

int chdir(const char* path) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_chdir)(const char*);
    if (!orig_chdir) orig_chdir = dlsym(RTLD_NEXT, "chdir");
    return orig_chdir(redirected);
}

/*
 * termux-am-socket embeds /data/data/com.termux/.../am.sock. Rewrite AF_UNIX
 * connect paths so the client reaches our real TermuxAm socket server.
 */
static int is_keepalive_short_component(const char* s) {
    return s && (strcmp(s, OLD_KEEPALIVE_COMPONENT) == 0
        || strcmp(s, BROKEN_KEEPALIVE_COMPONENT) == 0);
}

static int is_api_component_needing_rewrite(const char* s) {
    return s && (strcmp(s, OLD_API_COMPONENT) == 0
        || is_keepalive_short_component(s));
}

static const char* rewritten_api_component(const char* s) {
    if (!s) return NULL;
    if (strcmp(s, OLD_API_COMPONENT) == 0) return NEW_API_COMPONENT;
    if (is_keepalive_short_component(s)) return NEW_KEEPALIVE_COMPONENT;
    return NULL;
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    static int (*orig_connect)(int, const struct sockaddr*, socklen_t);
    if (!orig_connect) orig_connect = dlsym(RTLD_NEXT, "connect");
    if (!addr || addr->sa_family != AF_UNIX) {
        return orig_connect(sockfd, addr, addrlen);
    }

    const struct sockaddr_un* un = (const struct sockaddr_un*)addr;

    /* Abstract namespace: rewrite Termux:API listen socket address. */
    if (un->sun_path[0] == '\0') {
        const char* abs_name = un->sun_path + 1;
        size_t abs_len = strnlen(abs_name, sizeof(un->sun_path) - 1);
        if (abs_len == strlen(OLD_API_LISTEN)
            && memcmp(abs_name, OLD_API_LISTEN, abs_len) == 0) {
            struct sockaddr_un new_un;
            memset(&new_un, 0, sizeof(new_un));
            new_un.sun_family = AF_UNIX;
            new_un.sun_path[0] = '\0';
            size_t nlen = strlen(NEW_API_LISTEN);
            if (nlen + 1 >= sizeof(new_un.sun_path)) {
                errno = ENAMETOOLONG;
                return -1;
            }
            memcpy(new_un.sun_path + 1, NEW_API_LISTEN, nlen);
            socklen_t new_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path)
                + 1 + nlen);
            LOGI("rewrote API listen socket → %s", NEW_API_LISTEN);
            return orig_connect(sockfd, (struct sockaddr*)&new_un, new_len);
        }
        return orig_connect(sockfd, addr, addrlen);
    }

    if (!strstr(un->sun_path, OLD_PKG)) {
        return orig_connect(sockfd, addr, addrlen);
    }

    char* rewritten = replace_all_alloc(un->sun_path, OLD_PKG, NEW_PKG);
    if (!rewritten) {
        return orig_connect(sockfd, addr, addrlen);
    }
    if (strlen(rewritten) >= sizeof(un->sun_path)) {
        free(rewritten);
        errno = ENAMETOOLONG;
        return -1;
    }

    struct sockaddr_un new_un;
    memset(&new_un, 0, sizeof(new_un));
    new_un.sun_family = AF_UNIX;
    memcpy(new_un.sun_path, rewritten, strlen(rewritten) + 1);
    free(rewritten);

    socklen_t new_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path)
        + strlen(new_un.sun_path) + 1);
    return orig_connect(sockfd, (struct sockaddr*)&new_un, new_len);
}

/* Rewrite argv for am / termux-api so broadcasts hit our companion API app. */
static char** rewrite_argv_for_exec(char* const argv[]) {
    if (!argv) return NULL;

    size_t count = 0;
    while (argv[count]) count++;

    int needs = 0;
    for (size_t i = 0; i < count; i++) {
        if (is_api_component_needing_rewrite(argv[i])) {
            needs = 1;
            break;
        }
    }
    if (!needs) return (char**)argv;

    char** out = (char**)malloc((count + 1) * sizeof(char*));
    if (!out) return (char**)argv;

    for (size_t i = 0; i < count; i++) {
        const char* replacement = rewritten_api_component(argv[i]);
        if (replacement) {
            size_t n = strlen(replacement) + 1;
            char* copy = (char*)malloc(n);
            if (!copy) {
                out[i] = (char*)argv[i];
            } else {
                memcpy(copy, replacement, n);
                out[i] = copy;
                LOGI("rewrote API am component → %s", replacement);
            }
        } else {
            out[i] = (char*)argv[i];
        }
    }
    out[count] = NULL;
    return out;
}

int open(const char* path, int flags, ...) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_open)(const char*, int, ...);
    if (!orig_open) orig_open = dlsym(RTLD_NEXT, "open");

    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode_t mode = va_arg(args, mode_t);
        va_end(args);
        return orig_open(redirected, flags, mode);
    }
    return orig_open(redirected, flags);
}

int openat(int dirfd, const char* path, int flags, ...) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_openat)(int, const char*, int, ...);
    if (!orig_openat) orig_openat = dlsym(RTLD_NEXT, "openat");

    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode_t mode = va_arg(args, mode_t);
        va_end(args);
        return orig_openat(dirfd, redirected, flags, mode);
    }
    return orig_openat(dirfd, redirected, flags);
}

/* Bionic fortify / newer libc entry point used instead of open(). */
int __open_2(const char* path, int flags) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig___open_2)(const char*, int);
    if (!orig___open_2) orig___open_2 = dlsym(RTLD_NEXT, "__open_2");
    if (!orig___open_2) {
        static int (*orig_open)(const char*, int, ...);
        if (!orig_open) orig_open = dlsym(RTLD_NEXT, "open");
        return orig_open(redirected, flags);
    }
    return orig___open_2(redirected, flags);
}

int __openat_2(int dirfd, const char* path, int flags) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig___openat_2)(int, const char*, int);
    if (!orig___openat_2) orig___openat_2 = dlsym(RTLD_NEXT, "__openat_2");
    if (!orig___openat_2) {
        static int (*orig_openat)(int, const char*, int, ...);
        if (!orig_openat) orig_openat = dlsym(RTLD_NEXT, "openat");
        return orig_openat(dirfd, redirected, flags);
    }
    return orig___openat_2(dirfd, redirected, flags);
}

int stat(const char* path, struct stat* sbuf) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_stat)(const char*, struct stat*);
    if (!orig_stat) orig_stat = dlsym(RTLD_NEXT, "stat");
    return orig_stat(redirected, sbuf);
}

int lstat(const char* path, struct stat* sbuf) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_lstat)(const char*, struct stat*);
    if (!orig_lstat) orig_lstat = dlsym(RTLD_NEXT, "lstat");
    return orig_lstat(redirected, sbuf);
}

int fstatat(int dirfd, const char* path, struct stat* sbuf, int flags) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_fstatat)(int, const char*, struct stat*, int);
    if (!orig_fstatat) orig_fstatat = dlsym(RTLD_NEXT, "fstatat");
    return orig_fstatat(dirfd, redirected, sbuf, flags);
}

#if defined(__NR_execveat)
int execveat(int dirfd, const char* pathname, char* const argv[], char* const envp[],
             int flags) {
    char buf[4096];
    const char* redirected = pathname;
    if (pathname && pathname[0] == '/') {
        redirected = redirect_path(pathname, buf, sizeof(buf));
        rewrite_termux_paths_in_script(redirected);
    }
    char** new_argv = rewrite_argv_for_exec(argv);
    char* const* effective_argv = new_argv ? new_argv : argv;
    char** new_envp = rewrite_envp_for_exec(redirected, envp);
    static int (*orig_execveat)(int, const char*, char* const[], char* const[], int);
    if (!orig_execveat) orig_execveat = dlsym(RTLD_NEXT, "execveat");
    if (!orig_execveat) {
        errno = ENOSYS;
        return -1;
    }
    return orig_execveat(dirfd, redirected,
        (char**)effective_argv,
        new_envp ? new_envp : (char**)envp,
        flags);
}
#endif

int execve(const char* filename, char* const argv[], char* const envp[]) {
    char buf[4096];
    const char* redirected = redirect_path(filename, buf, sizeof(buf));
    rewrite_termux_paths_in_script(redirected);
    char** new_argv = rewrite_argv_for_exec(argv);
    char* const* effective_argv = new_argv ? new_argv : argv;

    char interp[512];
    char** env_argv = NULL;
    if (transform_usr_bin_env_shebang(redirected, (char* const*)effective_argv,
            interp, sizeof(interp), &env_argv)) {
        redirected = interp;
        effective_argv = env_argv;
    }

    char** new_envp = rewrite_envp_for_exec(redirected, envp);
    static int (*orig_execve)(const char*, char* const[], char* const[]);
    if (!orig_execve) orig_execve = dlsym(RTLD_NEXT, "execve");
    return orig_execve(redirected,
        (char**)effective_argv,
        new_envp ? new_envp : (char**)envp);
}

int execv(const char* path, char* const argv[]) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    rewrite_termux_paths_in_script(redirected);
    /* execv uses environ; rewrite via execve so SSH/login get LD_* / HOME. */
    extern char** environ;
    char** new_argv = rewrite_argv_for_exec(argv);
    char* const* effective_argv = new_argv ? new_argv : argv;

    char interp[512];
    char** env_argv = NULL;
    if (transform_usr_bin_env_shebang(redirected, (char* const*)effective_argv,
            interp, sizeof(interp), &env_argv)) {
        redirected = interp;
        effective_argv = env_argv;
    }

    char** new_envp = rewrite_envp_for_exec(redirected, environ);
    static int (*orig_execve)(const char*, char* const[], char* const[]);
    if (!orig_execve) orig_execve = dlsym(RTLD_NEXT, "execve");
    return orig_execve(redirected,
        (char**)effective_argv,
        new_envp ? new_envp : environ);
}

int execvp(const char* file, char* const argv[]) {
    char** new_argv = rewrite_argv_for_exec(argv);
    /* Absolute paths may still be maintainer scripts; relative names are PATH lookups. */
    if (file && file[0] == '/') {
        char buf[4096];
        const char* redirected = redirect_path(file, buf, sizeof(buf));
        rewrite_termux_paths_in_script(redirected);

        char* const* effective_argv = new_argv ? new_argv : argv;
        char interp[512];
        char** env_argv = NULL;
        if (transform_usr_bin_env_shebang(redirected, (char* const*)effective_argv,
                interp, sizeof(interp), &env_argv)) {
            /* Prefer execve so envp rewrite + shebang transform stay consistent. */
            extern char** environ;
            char** new_envp = rewrite_envp_for_exec(interp, environ);
            static int (*orig_execve)(const char*, char* const[], char* const[]);
            if (!orig_execve) orig_execve = dlsym(RTLD_NEXT, "execve");
            return orig_execve(interp,
                env_argv,
                new_envp ? new_envp : environ);
        }

        static int (*orig_execvp)(const char*, char* const[]);
        if (!orig_execvp) orig_execvp = dlsym(RTLD_NEXT, "execvp");
        return orig_execvp(redirected, new_argv ? new_argv : (char**)argv);
    }
    static int (*orig_execvp)(const char*, char* const[]);
    if (!orig_execvp) orig_execvp = dlsym(RTLD_NEXT, "execvp");
    return orig_execvp(file, new_argv ? new_argv : (char**)argv);
}

int access(const char* path, int mode) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_access)(const char*, int);
    if (!orig_access) orig_access = dlsym(RTLD_NEXT, "access");
    return orig_access(redirected, mode);
}

int faccessat(int dirfd, const char* path, int mode, int flags) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_faccessat)(int, const char*, int, int);
    if (!orig_faccessat) orig_faccessat = dlsym(RTLD_NEXT, "faccessat");
    return orig_faccessat(dirfd, redirected, mode, flags);
}

int mkdir(const char* path, mode_t mode) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_mkdir)(const char*, mode_t);
    if (!orig_mkdir) orig_mkdir = dlsym(RTLD_NEXT, "mkdir");
    return orig_mkdir(redirected, mode);
}

int mkdirat(int dirfd, const char* path, mode_t mode) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_mkdirat)(int, const char*, mode_t);
    if (!orig_mkdirat) orig_mkdirat = dlsym(RTLD_NEXT, "mkdirat");
    return orig_mkdirat(dirfd, redirected, mode);
}

int unlink(const char* path) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_unlink)(const char*);
    if (!orig_unlink) orig_unlink = dlsym(RTLD_NEXT, "unlink");
    return orig_unlink(redirected);
}

int unlinkat(int dirfd, const char* path, int flags) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_unlinkat)(int, const char*, int);
    if (!orig_unlinkat) orig_unlinkat = dlsym(RTLD_NEXT, "unlinkat");
    return orig_unlinkat(dirfd, redirected, flags);
}

int rmdir(const char* path) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_rmdir)(const char*);
    if (!orig_rmdir) orig_rmdir = dlsym(RTLD_NEXT, "rmdir");
    return orig_rmdir(redirected);
}

int rename(const char* oldpath, const char* newpath) {
    char oldbuf[4096];
    char newbuf[4096];
    const char* redirected_old = redirect_path(oldpath, oldbuf, sizeof(oldbuf));
    const char* redirected_new = redirect_path(newpath, newbuf, sizeof(newbuf));
    static int (*orig_rename)(const char*, const char*);
    if (!orig_rename) orig_rename = dlsym(RTLD_NEXT, "rename");
    return orig_rename(redirected_old, redirected_new);
}

int renameat(int olddirfd, const char* oldpath, int newdirfd, const char* newpath) {
    char oldbuf[4096];
    char newbuf[4096];
    const char* redirected_old = redirect_path(oldpath, oldbuf, sizeof(oldbuf));
    const char* redirected_new = redirect_path(newpath, newbuf, sizeof(newbuf));
    static int (*orig_renameat)(int, const char*, int, const char*);
    if (!orig_renameat) orig_renameat = dlsym(RTLD_NEXT, "renameat");
    return orig_renameat(olddirfd, redirected_old, newdirfd, redirected_new);
}

int renameat2(int olddirfd, const char* oldpath, int newdirfd, const char* newpath,
              unsigned int flags) {
    char oldbuf[4096];
    char newbuf[4096];
    const char* redirected_old = redirect_path(oldpath, oldbuf, sizeof(oldbuf));
    const char* redirected_new = redirect_path(newpath, newbuf, sizeof(newbuf));
    static int (*orig_renameat2)(int, const char*, int, const char*, unsigned int);
    if (!orig_renameat2) orig_renameat2 = dlsym(RTLD_NEXT, "renameat2");
    if (!orig_renameat2) {
        /* Fallback if renameat2 is unavailable. */
        static int (*orig_renameat)(int, const char*, int, const char*);
        if (!orig_renameat) orig_renameat = dlsym(RTLD_NEXT, "renameat");
        return orig_renameat(olddirfd, redirected_old, newdirfd, redirected_new);
    }
    return orig_renameat2(olddirfd, redirected_old, newdirfd, redirected_new, flags);
}

int link(const char* oldpath, const char* newpath) {
    char oldbuf[4096];
    char newbuf[4096];
    const char* redirected_old = redirect_path(oldpath, oldbuf, sizeof(oldbuf));
    const char* redirected_new = redirect_path(newpath, newbuf, sizeof(newbuf));
    static int (*orig_link)(const char*, const char*);
    if (!orig_link) orig_link = dlsym(RTLD_NEXT, "link");
    return orig_link(redirected_old, redirected_new);
}

int linkat(int olddirfd, const char* oldpath, int newdirfd, const char* newpath, int flags) {
    char oldbuf[4096];
    char newbuf[4096];
    const char* redirected_old = redirect_path(oldpath, oldbuf, sizeof(oldbuf));
    const char* redirected_new = redirect_path(newpath, newbuf, sizeof(newbuf));
    static int (*orig_linkat)(int, const char*, int, const char*, int);
    if (!orig_linkat) orig_linkat = dlsym(RTLD_NEXT, "linkat");
    return orig_linkat(olddirfd, redirected_old, newdirfd, redirected_new, flags);
}

int symlink(const char* target, const char* linkpath) {
    char targetbuf[4096];
    char linkbuf[4096];
    const char* redirected_target = redirect_path(target, targetbuf, sizeof(targetbuf));
    const char* redirected_link = redirect_path(linkpath, linkbuf, sizeof(linkbuf));
    static int (*orig_symlink)(const char*, const char*);
    if (!orig_symlink) orig_symlink = dlsym(RTLD_NEXT, "symlink");
    return orig_symlink(redirected_target, redirected_link);
}

int symlinkat(const char* target, int newdirfd, const char* linkpath) {
    char targetbuf[4096];
    char linkbuf[4096];
    const char* redirected_target = redirect_path(target, targetbuf, sizeof(targetbuf));
    const char* redirected_link = redirect_path(linkpath, linkbuf, sizeof(linkbuf));
    static int (*orig_symlinkat)(const char*, int, const char*);
    if (!orig_symlinkat) orig_symlinkat = dlsym(RTLD_NEXT, "symlinkat");
    return orig_symlinkat(redirected_target, newdirfd, redirected_link);
}

int chmod(const char* path, mode_t mode) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_chmod)(const char*, mode_t);
    if (!orig_chmod) orig_chmod = dlsym(RTLD_NEXT, "chmod");
    return orig_chmod(redirected, mode);
}

int fchmodat(int dirfd, const char* path, mode_t mode, int flags) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_fchmodat)(int, const char*, mode_t, int);
    if (!orig_fchmodat) orig_fchmodat = dlsym(RTLD_NEXT, "fchmodat");
    return orig_fchmodat(dirfd, redirected, mode, flags);
}

int chown(const char* path, uid_t owner, gid_t group) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_chown)(const char*, uid_t, gid_t);
    if (!orig_chown) orig_chown = dlsym(RTLD_NEXT, "chown");
    return orig_chown(redirected, owner, group);
}

int lchown(const char* path, uid_t owner, gid_t group) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_lchown)(const char*, uid_t, gid_t);
    if (!orig_lchown) orig_lchown = dlsym(RTLD_NEXT, "lchown");
    return orig_lchown(redirected, owner, group);
}

int fchownat(int dirfd, const char* path, uid_t owner, gid_t group, int flags) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_fchownat)(int, const char*, uid_t, gid_t, int);
    if (!orig_fchownat) orig_fchownat = dlsym(RTLD_NEXT, "fchownat");
    return orig_fchownat(dirfd, redirected, owner, group, flags);
}

int utimensat(int dirfd, const char* path, const struct timespec times[2], int flags) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_utimensat)(int, const char*, const struct timespec[2], int);
    if (!orig_utimensat) orig_utimensat = dlsym(RTLD_NEXT, "utimensat");
    return orig_utimensat(dirfd, redirected, times, flags);
}

int truncate(const char* path, off_t length) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static int (*orig_truncate)(const char*, off_t);
    if (!orig_truncate) orig_truncate = dlsym(RTLD_NEXT, "truncate");
    return orig_truncate(redirected, length);
}

FILE* fopen(const char* path, const char* mode) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static FILE* (*orig_fopen)(const char*, const char*);
    if (!orig_fopen) orig_fopen = dlsym(RTLD_NEXT, "fopen");
    return orig_fopen(redirected, mode);
}

FILE* freopen(const char* path, const char* mode, FILE* stream) {
    char buf[4096];
    const char* redirected = redirect_path(path, buf, sizeof(buf));
    static FILE* (*orig_freopen)(const char*, const char*, FILE*);
    if (!orig_freopen) orig_freopen = dlsym(RTLD_NEXT, "freopen");
    return orig_freopen(redirected, mode, stream);
}

DIR* opendir(const char* name) {
    char buf[4096];
    const char* redirected = redirect_path(name, buf, sizeof(buf));
    static DIR* (*orig_opendir)(const char*);
    if (!orig_opendir) orig_opendir = dlsym(RTLD_NEXT, "opendir");
    return orig_opendir(redirected);
}

ssize_t readlink(const char* path, char* buf, size_t bufsiz) {
    char pbuf[4096];
    const char* redirected = redirect_path(path, pbuf, sizeof(pbuf));
    static ssize_t (*orig_readlink)(const char*, char*, size_t);
    if (!orig_readlink) orig_readlink = dlsym(RTLD_NEXT, "readlink");
    return orig_readlink(redirected, buf, bufsiz);
}

ssize_t readlinkat(int dirfd, const char* path, char* buf, size_t bufsiz) {
    char pbuf[4096];
    const char* redirected = redirect_path(path, pbuf, sizeof(pbuf));
    static ssize_t (*orig_readlinkat)(int, const char*, char*, size_t);
    if (!orig_readlinkat) orig_readlinkat = dlsym(RTLD_NEXT, "readlinkat");
    return orig_readlinkat(dirfd, redirected, buf, bufsiz);
}
