#
# Copyright (c) 2010, Oracle and/or its affiliates. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

libmandir = $(LIB_MAN_DIR)

libman_PRE =				\
	Xau.man				\
	XauDisposeAuth.man		\
	XauFileName.man			\
	XauGetAuthByAddr.man		\
	XauGetBestAuthByAddr.man	\
	XauLockAuth.man			\
	XauReadAuth.man			\
	XauUnlockAuth.man		\
	XauWriteAuth.man

libman_DATA = $(libman_PRE:man=$(LIB_MAN_SUFFIX))

EXTRA_DIST = $(libman_PRE)

CLEANFILES = $(libman_DATA)

# String replacements in MAN_SUBSTS now come from xorg-macros.m4 via configure
SUFFIXES = .$(LIB_MAN_SUFFIX) .man

.man.$(LIB_MAN_SUFFIX):
	$(AM_V_GEN)$(SED) $(MAN_SUBSTS) < $< > $@
