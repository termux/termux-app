/*
 * Copyright 1998 by Concurrent Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Concurrent Computer
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Concurrent Computer Corporation makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * CONCURRENT COMPUTER CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CONCURRENT COMPUTER CORPORATION BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Copyright 1998 by Metro Link Incorporated
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Metro Link
 * Incorporated not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Metro Link Incorporated makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * METRO LINK INCORPORATED DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL METRO LINK INCORPORATED BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * This file is derived in part from the original xf86_PCI.h that included
 * following copyright message:
 *
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holder(s)
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  The above listed
 * copyright holder(s) make(s) no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM(S) ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*
 * Copyright (c) 1999-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * This file contains just the public interface to the PCI code.
 * Drivers should use this file rather than Pci.h.
 */

#ifndef _XF86PCI_H
#define _XF86PCI_H 1
#include <X11/Xarch.h>
#include <X11/Xfuncproto.h>
#include "misc.h"
#include <pciaccess.h>

/*
 * PCI cfg space definitions (e.g. stuff right out of the PCI spec)
 */

/* Device identification register */
#define PCI_ID_REG			0x00

/* Command and status register */
#define PCI_CMD_STAT_REG		0x04
#define PCI_CMD_BASE_REG		0x10
#define PCI_CMD_BIOS_REG		0x30
#define PCI_CMD_MASK			0xffff
#define PCI_CMD_IO_ENABLE		0x01
#define PCI_CMD_MEM_ENABLE		0x02
#define PCI_CMD_MASTER_ENABLE		0x04
#define PCI_CMD_SPECIAL_ENABLE		0x08
#define PCI_CMD_INVALIDATE_ENABLE	0x10
#define PCI_CMD_PALETTE_ENABLE		0x20
#define PCI_CMD_PARITY_ENABLE		0x40
#define PCI_CMD_STEPPING_ENABLE		0x80
#define PCI_CMD_SERR_ENABLE		0x100
#define PCI_CMD_BACKTOBACK_ENABLE	0x200
#define PCI_CMD_BIOS_ENABLE		0x01

/* base class */
#define PCI_CLASS_REG		0x08
#define PCI_CLASS_MASK		0xff000000
#define PCI_CLASS_SHIFT		24
#define PCI_CLASS_EXTRACT(x)	\
	(((x) & PCI_CLASS_MASK) >> PCI_CLASS_SHIFT)

/* base class values */
#define PCI_CLASS_PREHISTORIC		0x00
#define PCI_CLASS_MASS_STORAGE		0x01
#define PCI_CLASS_NETWORK		0x02
#define PCI_CLASS_DISPLAY		0x03
#define PCI_CLASS_MULTIMEDIA		0x04
#define PCI_CLASS_MEMORY		0x05
#define PCI_CLASS_BRIDGE		0x06
#define PCI_CLASS_COMMUNICATIONS	0x07
#define PCI_CLASS_SYSPERIPH		0x08
#define PCI_CLASS_INPUT			0x09
#define PCI_CLASS_DOCKING		0x0a
#define PCI_CLASS_PROCESSOR		0x0b
#define PCI_CLASS_SERIALBUS		0x0c
#define PCI_CLASS_WIRELESS		0x0d
#define PCI_CLASS_I2O			0x0e
#define PCI_CLASS_SATELLITE		0x0f
#define PCI_CLASS_CRYPT			0x10
#define PCI_CLASS_DATA_ACQUISTION	0x11
#define PCI_CLASS_UNDEFINED		0xff

/* sub class */
#define PCI_SUBCLASS_MASK	0x00ff0000
#define PCI_SUBCLASS_SHIFT	16
#define PCI_SUBCLASS_EXTRACT(x)	\
	(((x) & PCI_SUBCLASS_MASK) >> PCI_SUBCLASS_SHIFT)

/* Sub class values */
/* 0x00 prehistoric subclasses */
#define PCI_SUBCLASS_PREHISTORIC_MISC	0x00
#define PCI_SUBCLASS_PREHISTORIC_VGA	0x01

/* 0x03 display subclasses */
#define PCI_SUBCLASS_DISPLAY_VGA	0x00
#define PCI_SUBCLASS_DISPLAY_XGA	0x01
#define PCI_SUBCLASS_DISPLAY_MISC	0x80

/* 0x04 multimedia subclasses */
#define PCI_SUBCLASS_MULTIMEDIA_VIDEO	0x00
#define PCI_SUBCLASS_MULTIMEDIA_AUDIO	0x01
#define PCI_SUBCLASS_MULTIMEDIA_MISC	0x80

/* 0x06 bridge subclasses */
#define PCI_SUBCLASS_BRIDGE_HOST	0x00
#define PCI_SUBCLASS_BRIDGE_ISA		0x01
#define PCI_SUBCLASS_BRIDGE_EISA	0x02
#define PCI_SUBCLASS_BRIDGE_MC		0x03
#define PCI_SUBCLASS_BRIDGE_PCI		0x04
#define PCI_SUBCLASS_BRIDGE_PCMCIA	0x05
#define PCI_SUBCLASS_BRIDGE_NUBUS	0x06
#define PCI_SUBCLASS_BRIDGE_CARDBUS	0x07
#define PCI_SUBCLASS_BRIDGE_RACEWAY	0x08
#define PCI_SUBCLASS_BRIDGE_MISC	0x80
#define PCI_IF_BRIDGE_PCI_SUBTRACTIVE	0x01

/* 0x0b processor subclasses */
#define PCI_SUBCLASS_PROCESSOR_386	0x00
#define PCI_SUBCLASS_PROCESSOR_486	0x01
#define PCI_SUBCLASS_PROCESSOR_PENTIUM	0x02
#define PCI_SUBCLASS_PROCESSOR_ALPHA	0x10
#define PCI_SUBCLASS_PROCESSOR_POWERPC	0x20
#define PCI_SUBCLASS_PROCESSOR_MIPS	0x30
#define PCI_SUBCLASS_PROCESSOR_COPROC	0x40

/* PCI-PCI bridge mapping registers */
#define PCI_PCI_BRIDGE_BUS_REG		0x18
#define PCI_SUBORDINATE_BUS_MASK	0x00ff0000
#define PCI_SECONDARY_BUS_MASK		0x0000ff00
#define PCI_PRIMARY_BUS_MASK		0x000000ff

#define PCI_PCI_BRIDGE_IO_REG		0x1c
#define PCI_PCI_BRIDGE_MEM_REG		0x20
#define PCI_PCI_BRIDGE_PMEM_REG		0x24

#define PCI_PCI_BRIDGE_CONTROL_REG	0x3E
#define PCI_PCI_BRIDGE_PARITY_EN	0x01
#define PCI_PCI_BRIDGE_SERR_EN		0x02
#define PCI_PCI_BRIDGE_ISA_EN		0x04
#define PCI_PCI_BRIDGE_VGA_EN		0x08
#define PCI_PCI_BRIDGE_MASTER_ABORT_EN	0x20
#define PCI_PCI_BRIDGE_SECONDARY_RESET	0x40
#define PCI_PCI_BRIDGE_FAST_B2B_EN	0x80

/* Subsystem identification register */
#define PCI_SUBSYSTEM_ID_REG		0x2c

/* User defined cfg space regs */
#define PCI_REG_USERCONFIG		0x40
#define PCI_OPTION_REG			0x40

/*
 * Typedefs, etc...
 */

/* Public PCI access functions */
extern _X_EXPORT Bool xf86scanpci(void);
extern _X_EXPORT char *DRICreatePCIBusID(const struct pci_device *dev);

#endif                          /* _XF86PCI_H */
