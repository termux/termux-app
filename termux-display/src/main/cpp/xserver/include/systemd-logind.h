/*
 * Copyright © 2013 Red Hat, Inc.
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
 *
 * Author: Hans de Goede <hdegoede@redhat.com>
 */

#ifndef SYSTEMD_LOGIND_H
#define SYSTEMD_LOGIND_H

#ifdef SYSTEMD_LOGIND
int systemd_logind_init(void);
void systemd_logind_fini(void);
int systemd_logind_take_fd(int major, int minor, const char *path, Bool *paus);
void systemd_logind_release_fd(int major, int minor, int fd);
int systemd_logind_controls_session(void);
void systemd_logind_vtenter(void);
void systemd_logind_drop_master(void);
#else
#define systemd_logind_init()
#define systemd_logind_fini()
#define systemd_logind_take_fd(major, minor, path, paus) -1
#define systemd_logind_release_fd(major, minor, fd) close(fd)
#define systemd_logind_controls_session() 0
#define systemd_logind_vtenter()
#define systemd_logind_drop_master()
#endif

#endif
