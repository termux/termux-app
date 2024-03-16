/*
 * debug.c -- debugging routines for libtirpc
 *
 * Copyright (C) 2014  Red Hat, Steve Dickson <steved@redhat.com>
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
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

#include "debug.h"

/* library global debug level */
int libtirpc_debug_level = 0;
int  log_stderr = 1; /* log to stderr instead of systlog */

/*
 * Set the debug level for the entire library.
 * Different area will used the value to determin
 * the verbosity of the debugging output.
 */
void
libtirpc_set_debug(char *name, int level, int use_stderr)
{
	if (level < 0)
		level = 0;

	log_stderr = use_stderr;
	if (!use_stderr)
		openlog(name, LOG_PID, LOG_DAEMON);

	libtirpc_debug_level = level;
	LIBTIRPC_DEBUG(1, ("libtirpc: debug level %d", libtirpc_debug_level));
}

void
libtirpc_log_dbg(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (log_stderr) {
		vfprintf(stderr, fmt, args);
		fprintf(stderr, "\n");
	} else
		vsyslog(LOG_NOTICE, fmt, args);
	va_end(args);
}
