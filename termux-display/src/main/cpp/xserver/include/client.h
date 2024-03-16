/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). All
 * rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* Author: Rami Ylimäki <rami.ylimaki@vincit.fi> */

#ifndef CLIENT_H
#define CLIENT_H

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif                          /* HAVE_DIX_CONFIG_H */
#include <X11/Xfuncproto.h>
#include <sys/types.h>

/* Client IDs. Use GetClientPid, GetClientCmdName and GetClientCmdArgs
 * instead of accessing the fields directly. */
typedef struct {
    pid_t pid;                  /* process ID, -1 if not available */
    const char *cmdname;        /* process name, NULL if not available */
    const char *cmdargs;        /* process arguments, NULL if not available */
} ClientIdRec, *ClientIdPtr;

struct _Client;

/* Initialize and clean up. */
void ReserveClientIds(struct _Client *client);
void ReleaseClientIds(struct _Client *client);

/* Determine client IDs for caching. Exported on purpose for
 * extensions such as SELinux. */
extern _X_EXPORT pid_t DetermineClientPid(struct _Client *client);
extern _X_EXPORT void DetermineClientCmd(pid_t, const char **cmdname,
                                         const char **cmdargs);

/* Query cached client IDs. Exported on purpose for drivers. */
extern _X_EXPORT pid_t GetClientPid(struct _Client *client);
extern _X_EXPORT const char *GetClientCmdName(struct _Client *client);
extern _X_EXPORT const char *GetClientCmdArgs(struct _Client *client);

#endif                          /* CLIENT_H */
