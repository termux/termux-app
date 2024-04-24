/*
 * Copyright © 2016 Broadcom
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void
kill_server(int server_pid)
{
    int ret = kill(server_pid, SIGTERM);
    int wstatus;

    if (ret) {
        fprintf(stderr, "Failed to send kill to the server: %s\n",
                strerror(errno));
        exit(1);
    }

    ret = waitpid(server_pid, &wstatus, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to wait for X to die: %s\n", strerror(errno));
        exit(1);
    }
}

static void
usage(int argc, char **argv)
{
    fprintf(stderr, "%s <client command> -- <server command>\n", argv[0]);
    exit(1);
}

static int server_displayfd;
static const char *server_dead = "server_dead";

static void
handle_sigchld(int sig)
{
    write(server_displayfd, server_dead, strlen(server_dead));
}

/* Starts the X server, returning its pid. */
static int
start_server(char *const *server_args)
{
    int server_pid = fork();

    if (server_pid == -1) {
        fprintf(stderr, "Fork failed: %s\n", strerror(errno));
        exit(1);
    } else if (server_pid != 0) {
        /* Continue along the main process that will exec the client. */

        struct sigaction sa;
        sa.sa_handler = handle_sigchld;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
        if (sigaction(SIGCHLD, &sa, 0) == -1) {
            fprintf(stderr, "Failed to set up signal handler: %s\n",
                    strerror(errno));
            exit(1);
        }

        return server_pid;
    }

    /* Execute the server.  This only returns if an error occurred. */
    execvp(server_args[0], server_args);
    fprintf(stderr, "Error starting the server: %s\n", strerror(errno));
    exit(1);
}

/* Reads the display number out of the started server's display socket. */
static int
get_display(int displayfd)
{
    char display_string[20];
    ssize_t ret;

    ret = read(displayfd, display_string, sizeof(display_string) - 1);
    if (ret <= 0) {
        fprintf(stderr, "Failed reading displayfd: %s\n", strerror(errno));
        exit(1);
    }

    /* We've read in the display number as a string terminated by
     * '\n', but not '\0'.  Cap it and parse the number.
     */
    display_string[ret] = '\0';

    if (strncmp(display_string, server_dead, strlen(server_dead)) == 0) {
        fprintf(stderr, "Server failed to start before setting up displayfd\n");
        exit(1);
    }

    return atoi(display_string);
}

static int
start_client(char *const *client_args, int display)
{
    char *display_string;
    int ret;
    int client_pid;

    ret = asprintf(&display_string, ":%d", display);
    if (ret < 0) {
        fprintf(stderr, "asprintf fail\n");
        exit(1);
    }

    ret = setenv("DISPLAY", display_string, true);
    if (ret) {
        fprintf(stderr, "Failed to set DISPLAY\n");
        exit(1);
    }

    client_pid = fork();
    if (client_pid == -1) {
        fprintf(stderr, "Fork failed: %s\n", strerror(errno));
        exit(1);
    } else if (client_pid) {
        int wstatus;

        ret = waitpid(client_pid, &wstatus, 0);
        if (ret < 0) {
            fprintf(stderr, "Error waiting for client to start: %s\n",
                    strerror(errno));
            return 1;
        }

        if (!WIFEXITED(wstatus))
            return 1;

        return WEXITSTATUS(wstatus);
    } else {
        execvp(client_args[0], client_args);
        /* exec only returns if an error occurred. */
        fprintf(stderr, "Error starting the client: %s\n", strerror(errno));
        exit(1);
    }
}

/* Splits the incoming argc/argv into a pair of NULL-terminated arrays
 * of args.
 */
static void
parse_args(int argc, char **argv,
           char * const **out_client_args,
           char * const **out_server_args,
           int displayfd)
{
    /* We're stripping the -- and the program name, inserting two
     * NULLs, and also the -displayfd and fd number.
     */
    char **args_storage = calloc(argc + 2, sizeof(char *));
    char *const *client_args;
    char *const *server_args = NULL;
    char **next_arg = args_storage;
    bool parsing_client = true;
    int i, ret;
    char *displayfd_string;

    if (!args_storage)
        exit(1);

    client_args = args_storage;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) {
            if (!parsing_client)
                usage(argc, argv);

            /* Cap the client list */
            *next_arg = NULL;
            next_arg++;

            /* Move to adding into server_args. */
            server_args = next_arg;
            parsing_client = false;
            continue;
        }

        /* A sort of escaped "--" argument so we can nest server
         * invocations for testing.
         */
        if (strcmp(argv[i], "----") == 0)
            *next_arg = (char *)"--";
        else
            *next_arg = argv[i];
        next_arg++;
    }

    if (client_args[0] == NULL || !server_args || server_args[0] == NULL)
        usage(argc, argv);

    /* Give the server -displayfd X */
    *next_arg = (char *)"-displayfd";
    next_arg++;

    ret = asprintf(&displayfd_string, "%d", displayfd);
    if (ret < 0) {
        fprintf(stderr, "asprintf fail\n");
        exit(1);
    }
    *next_arg = displayfd_string;
    next_arg++;

    *out_client_args = client_args;
    *out_server_args = server_args;
}

int
main(int argc, char **argv)
{
    char * const *client_args;
    char * const *server_args;
    int displayfd_pipe[2];
    int display, server_pid;
    int ret;

    ret = pipe(displayfd_pipe);
    if (ret) {
        fprintf(stderr, "Pipe creation failure: %s", strerror(errno));
        exit(1);
    }

    server_displayfd = displayfd_pipe[1];
    parse_args(argc, argv, &client_args, &server_args, server_displayfd);
    server_pid = start_server(server_args);
    display = get_display(displayfd_pipe[0]);
    ret = start_client(client_args, display);
    kill_server(server_pid);

    exit(ret);
}
