
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _XF86_PCI_BUS_H
#define _XF86_PCI_BUS_H

#include "xf86MatchDrivers.h"

void xf86PciProbe(void);
Bool xf86PciAddMatchingDev(DriverPtr drvp);
Bool xf86PciProbeDev(DriverPtr drvp);
void xf86PciIsolateDevice(const char *argument);
void xf86PciMatchDriver(XF86MatchedDrivers *md);
Bool xf86PciConfigure(void *busData, struct pci_device *pDev);
void xf86PciConfigureNewDev(void *busData, struct pci_device *pVideo,
                            GDevRec * GDev, int *chipset);

#define MATCH_PCI_DEVICES(x, y) (((x)->domain == (y)->domain) &&        \
                                 ((x)->bus == (y)->bus) &&              \
                                 ((x)->func == (y)->func) &&            \
                                 ((x)->dev == (y)->dev))

void
xf86MatchDriverFromFiles(uint16_t match_vendor, uint16_t match_chip,
                         XF86MatchedDrivers *md);
void
xf86VideoPtrToDriverList(struct pci_device *dev, XF86MatchedDrivers *md);
#endif                          /* _XF86_PCI_BUS_H */
