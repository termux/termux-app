/*
 * SBUS and OpenPROM access functions.
 *
 * Copyright (C) 2000 Jakub Jelinek (jakub@redhat.com)
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * JAKUB JELINEK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#ifdef __sun
#include <sys/utsname.h>
#endif
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#include "xf86sbusBus.h"
#include "xf86Sbus.h"

int promRootNode;

static int promFd = -1;
static int promCurrentNode;
static int promOpenCount = 0;
static int promP1275 = -1;

#define MAX_PROP	128
#define MAX_VAL		(4096-128-4)
static struct openpromio *promOpio;

sbusDevicePtr *xf86SbusInfo = NULL;

struct sbus_devtable sbusDeviceTable[] = {
    {SBUS_DEVICE_BW2, FBTYPE_SUN2BW, "bwtwo", "sunbw2",
     "Sun Monochrome (bwtwo)"},
    {SBUS_DEVICE_CG2, FBTYPE_SUN2COLOR, "cgtwo", NULL, "Sun Color2 (cgtwo)"},
    {SBUS_DEVICE_CG3, FBTYPE_SUN3COLOR, "cgthree", "suncg3",
     "Sun Color3 (cgthree)"},
    {SBUS_DEVICE_CG4, FBTYPE_SUN4COLOR, "cgfour", NULL, "Sun Color4 (cgfour)"},
    {SBUS_DEVICE_CG6, FBTYPE_SUNFAST_COLOR, "cgsix", "suncg6", "Sun GX"},
    {SBUS_DEVICE_CG8, FBTYPE_MEMCOLOR, "cgeight", NULL, "Sun CG8/RasterOps"},
    {SBUS_DEVICE_CG12, FBTYPE_SUNGP3, "cgtwelve", NULL, "Sun GS (cgtwelve)"},
    {SBUS_DEVICE_CG14, FBTYPE_MDICOLOR, "cgfourteen", "suncg14", "Sun SX"},
    {SBUS_DEVICE_GT, FBTYPE_SUNGT, "gt", NULL, "Sun Graphics Tower"},
    {SBUS_DEVICE_MGX, -1, "mgx", NULL, "Quantum 3D MGXplus"},
    {SBUS_DEVICE_LEO, FBTYPE_SUNLEO, "leo", "sunleo", "Sun ZX or Turbo ZX"},
    {SBUS_DEVICE_TCX, FBTYPE_TCXCOLOR, "tcx", "suntcx", "Sun TCX"},
    {SBUS_DEVICE_FFB, FBTYPE_CREATOR, "ffb", "sunffb", "Sun FFB"},
    {SBUS_DEVICE_FFB, FBTYPE_CREATOR, "afb", "sunffb", "Sun Elite3D"},
    {0, 0, NULL}
};

int
promGetSibling(int node)
{
    promOpio->oprom_size = sizeof(int);

    if (node == -1)
        return 0;
    *(int *) promOpio->oprom_array = node;
    if (ioctl(promFd, OPROMNEXT, promOpio) < 0)
        return 0;
    promCurrentNode = *(int *) promOpio->oprom_array;
    return *(int *) promOpio->oprom_array;
}

int
promGetChild(int node)
{
    promOpio->oprom_size = sizeof(int);

    if (!node || node == -1)
        return 0;
    *(int *) promOpio->oprom_array = node;
    if (ioctl(promFd, OPROMCHILD, promOpio) < 0)
        return 0;
    promCurrentNode = *(int *) promOpio->oprom_array;
    return *(int *) promOpio->oprom_array;
}

char *
promGetProperty(const char *prop, int *lenp)
{
    promOpio->oprom_size = MAX_VAL;

    strcpy(promOpio->oprom_array, prop);
    if (ioctl(promFd, OPROMGETPROP, promOpio) < 0)
        return 0;
    if (lenp)
        *lenp = promOpio->oprom_size;
    return promOpio->oprom_array;
}

int
promGetBool(const char *prop)
{
    promOpio->oprom_size = 0;

    *(int *) promOpio->oprom_array = 0;
    for (;;) {
        promOpio->oprom_size = MAX_PROP;
        if (ioctl(promFd, OPROMNXTPROP, promOpio) < 0)
            return 0;
        if (!promOpio->oprom_size)
            return 0;
        if (!strcmp(promOpio->oprom_array, prop))
            return 1;
    }
}

#define PROM_NODE_SIBLING 0x01
#define PROM_NODE_PREF    0x02
#define PROM_NODE_SBUS    0x04
#define PROM_NODE_EBUS    0x08
#define PROM_NODE_PCI     0x10

static int
promSetNode(sbusPromNodePtr pnode)
{
    int node;

    if (!pnode->node || pnode->node == -1)
        return -1;
    if (pnode->cookie[0] & PROM_NODE_SIBLING)
        node = promGetSibling(pnode->cookie[1]);
    else
        node = promGetChild(pnode->cookie[1]);
    if (pnode->node != node)
        return -1;
    return 0;
}

static void
promIsP1275(void)
{
#ifdef __linux__
    FILE *f;
    char buffer[1024];

    if (promP1275 != -1)
        return;
    promP1275 = 0;
    f = fopen("/proc/cpuinfo", "r");
    if (!f)
        return;
    while (fgets(buffer, 1024, f) != NULL)
        if (!strncmp(buffer, "type", 4) && strstr(buffer, "sun4u")) {
            promP1275 = 1;
            break;
        }
    fclose(f);
#elif defined(__sun)
    struct utsname buffer;

    if ((uname(&buffer) >= 0) && !strcmp(buffer.machine, "sun4u"))
        promP1275 = TRUE;
    else
        promP1275 = FALSE;
#elif defined(__FreeBSD__)
    promP1275 = TRUE;
#else
#error Missing promIsP1275() function for this OS
#endif
}

void
sparcPromClose(void)
{
    if (promOpenCount > 1) {
        promOpenCount--;
        return;
    }
    if (promFd != -1) {
        close(promFd);
        promFd = -1;
    }
    free(promOpio);
    promOpio = NULL;
    promOpenCount = 0;
}

int
sparcPromInit(void)
{
    if (promOpenCount) {
        promOpenCount++;
        return 0;
    }
    promFd = open("/dev/openprom", O_RDONLY, 0);
    if (promFd == -1)
        return -1;
    promOpio = (struct openpromio *) malloc(4096);
    if (!promOpio) {
        sparcPromClose();
        return -1;
    }
    promRootNode = promGetSibling(0);
    if (!promRootNode) {
        sparcPromClose();
        return -1;
    }
    promIsP1275();
    promOpenCount++;

    return 0;
}

char *
sparcPromGetProperty(sbusPromNodePtr pnode, const char *prop, int *lenp)
{
    if (promSetNode(pnode))
        return NULL;
    return promGetProperty(prop, lenp);
}

int
sparcPromGetBool(sbusPromNodePtr pnode, const char *prop)
{
    if (promSetNode(pnode))
        return 0;
    return promGetBool(prop);
}

static char *
promWalkGetDriverName(int node, int oldnode)
{
    int nextnode;
    int len;
    char *prop;
    int devId, i;

    prop = promGetProperty("device_type", &len);
    if (prop && (len > 0))
        do {
            if (!strcmp(prop, "display")) {
                prop = promGetProperty("name", &len);
                if (!prop || len <= 0)
                    break;
                while ((*prop >= 'A' && *prop <= 'Z') || *prop == ',')
                    prop++;
                for (i = 0; sbusDeviceTable[i].devId; i++)
                    if (!strcmp(prop, sbusDeviceTable[i].promName))
                        break;
                devId = sbusDeviceTable[i].devId;
                if (!devId)
                    break;
                if (sbusDeviceTable[i].driverName)
                    return sbusDeviceTable[i].driverName;
            }
        } while (0);

    nextnode = promGetChild(node);
    if (nextnode) {
        char *name;

        name = promWalkGetDriverName(nextnode, node);
        if (name)
            return name;
    }

    nextnode = promGetSibling(node);
    if (nextnode)
        return promWalkGetDriverName(nextnode, node);
    return NULL;
}

char *
sparcDriverName(void)
{
    char *name;

    if (sparcPromInit() < 0)
        return NULL;
    promGetSibling(0);
    name = promWalkGetDriverName(promRootNode, 0);
    sparcPromClose();
    return name;
}

static void
promWalkAssignNodes(int node, int oldnode, int flags,
                    sbusDevicePtr * devicePtrs)
{
    int nextnode;
    int len, sbus = flags & PROM_NODE_SBUS;
    char *prop;
    int devId, i, j;
    sbusPromNode pNode, pNode2;

    prop = promGetProperty("device_type", &len);
    if (prop && (len > 0))
        do {
            if (!strcmp(prop, "display")) {
                prop = promGetProperty("name", &len);
                if (!prop || len <= 0)
                    break;
                while ((*prop >= 'A' && *prop <= 'Z') || *prop == ',')
                    prop++;
                for (i = 0; sbusDeviceTable[i].devId; i++)
                    if (!strcmp(prop, sbusDeviceTable[i].promName))
                        break;
                devId = sbusDeviceTable[i].devId;
                if (!devId)
                    break;
                if (!sbus) {
                    if (devId == SBUS_DEVICE_FFB) {
                        /*
                         * All /SUNW,ffb outside of SBUS tree come before all
                         * /SUNW,afb outside of SBUS tree in Linux.
                         */
                        if (!strcmp(prop, "afb"))
                            flags |= PROM_NODE_PREF;
                    }
                    else if (devId != SBUS_DEVICE_CG14)
                        break;
                }
                for (i = 0; i < 32; i++) {
                    if (!devicePtrs[i] || devicePtrs[i]->devId != devId)
                        continue;
                    if (devicePtrs[i]->node.node) {
                        if ((devicePtrs[i]->node.
                             cookie[0] & ~PROM_NODE_SIBLING) <=
                            (flags & ~PROM_NODE_SIBLING))
                            continue;
                        for (j = i + 1, pNode = devicePtrs[i]->node; j < 32;
                             j++) {
                            if (!devicePtrs[j] || devicePtrs[j]->devId != devId)
                                continue;
                            pNode2 = devicePtrs[j]->node;
                            devicePtrs[j]->node = pNode;
                            pNode = pNode2;
                        }
                    }
                    devicePtrs[i]->node.node = node;
                    devicePtrs[i]->node.cookie[0] = flags;
                    devicePtrs[i]->node.cookie[1] = oldnode;
                    break;
                }
                break;
            }
        } while (0);

    prop = promGetProperty("name", &len);
    if (prop && len > 0) {
        if (!strcmp(prop, "sbus") || !strcmp(prop, "sbi"))
            sbus = PROM_NODE_SBUS;
    }

    nextnode = promGetChild(node);
    if (nextnode)
        promWalkAssignNodes(nextnode, node, sbus, devicePtrs);

    nextnode = promGetSibling(node);
    if (nextnode)
        promWalkAssignNodes(nextnode, node, PROM_NODE_SIBLING | sbus,
                            devicePtrs);
}

void
sparcPromAssignNodes(void)
{
    sbusDevicePtr psdp, *psdpp;
    int n, holes = 0, i, j;
    FILE *f;
    sbusDevicePtr devicePtrs[32];

    memset(devicePtrs, 0, sizeof(devicePtrs));
    for (psdpp = xf86SbusInfo, n = 0; (psdp = *psdpp); psdpp++, n++) {
        if (psdp->fbNum != n)
            holes = 1;
        devicePtrs[psdp->fbNum] = psdp;
    }
    if (holes && (f = fopen("/proc/fb", "r")) != NULL) {
        /* We could not open one of fb devices, check /proc/fb to see what
         * were the types of the cards missed. */
        char buffer[64];
        int fbNum, devId;
        static struct {
            int devId;
            char *prefix;
        } procFbPrefixes[] = {
            {SBUS_DEVICE_BW2, "BWtwo"},
            {SBUS_DEVICE_CG14, "CGfourteen"},
            {SBUS_DEVICE_CG6, "CGsix"},
            {SBUS_DEVICE_CG3, "CGthree"},
            {SBUS_DEVICE_FFB, "Creator"},
            {SBUS_DEVICE_FFB, "Elite 3D"},
            {SBUS_DEVICE_LEO, "Leo"},
            {SBUS_DEVICE_TCX, "TCX"},
            {0, NULL},
        };

        while (fscanf(f, "%d %63s\n", &fbNum, buffer) == 2) {
            for (i = 0; procFbPrefixes[i].devId; i++)
                if (!strncmp(procFbPrefixes[i].prefix, buffer,
                             strlen(procFbPrefixes[i].prefix)))
                    break;
            devId = procFbPrefixes[i].devId;
            if (!devId)
                continue;
            if (devicePtrs[fbNum]) {
                if (devicePtrs[fbNum]->devId != devId)
                    xf86ErrorF("Inconsistent /proc/fb with FBIOGATTR\n");
            }
            else if (!devicePtrs[fbNum]) {
                devicePtrs[fbNum] = psdp = xnfcalloc(sizeof(sbusDevice), 1);
                psdp->devId = devId;
                psdp->fbNum = fbNum;
                psdp->fd = -2;
            }
        }
        fclose(f);
    }
    promGetSibling(0);
    promWalkAssignNodes(promRootNode, 0, PROM_NODE_PREF, devicePtrs);
    for (i = 0, j = 0; i < 32; i++)
        if (devicePtrs[i] && devicePtrs[i]->fbNum == -1)
            j++;
    xf86SbusInfo = xnfreallocarray(xf86SbusInfo, n + j + 1, sizeof(psdp));
    for (i = 0, psdpp = xf86SbusInfo; i < 32; i++)
        if (devicePtrs[i]) {
            if (devicePtrs[i]->fbNum == -1) {
                memmove(psdpp + 1, psdpp, sizeof(psdpp) * (n + 1));
                *psdpp = devicePtrs[i];
            }
            else
                n--;
        }
}

static char *
promGetReg(int type)
{
    char *prop;
    int len;
    static char regstr[40];

    regstr[0] = 0;
    prop = promGetProperty("reg", &len);
    if (prop && len >= 4) {
        unsigned int *reg = (unsigned int *) prop;

        if (!promP1275 || (type == PROM_NODE_SBUS) || (type == PROM_NODE_EBUS))
            snprintf(regstr, sizeof(regstr), "@%x,%x", reg[0], reg[1]);
        else if (type == PROM_NODE_PCI) {
            if ((reg[0] >> 8) & 7)
                snprintf(regstr, sizeof(regstr), "@%x,%x",
                         (reg[0] >> 11) & 0x1f, (reg[0] >> 8) & 7);
            else
                snprintf(regstr, sizeof(regstr), "@%x", (reg[0] >> 11) & 0x1f);
        }
        else if (len == 4)
            snprintf(regstr, sizeof(regstr), "@%x", reg[0]);
        else {
            unsigned int regs[2];

            /* Things get more complicated on UPA. If upa-portid exists,
               then address is @upa-portid,second-int-in-reg, otherwise
               it is @first-int-in-reg/16,second-int-in-reg (well, probably
               upa-portid always exists, but just to be safe). */
            memcpy(regs, reg, sizeof(regs));
            prop = promGetProperty("upa-portid", &len);
            if (prop && len == 4) {
                reg = (unsigned int *) prop;
                snprintf(regstr, sizeof(regstr), "@%x,%x", reg[0], regs[1]);
            }
            else
                snprintf(regstr, sizeof(regstr), "@%x,%x", regs[0] >> 4,
                         regs[1]);
        }
    }
    return regstr;
}

static int
promWalkNode2Pathname(char *path, int parent, int node, int searchNode,
                      int type)
{
    int nextnode;
    int len, ntype = type;
    char *prop, *p;

    prop = promGetProperty("name", &len);
    *path = '/';
    if (!prop || len <= 0)
        return 0;
    if ((!strcmp(prop, "sbus") || !strcmp(prop, "sbi")) && !type)
        ntype = PROM_NODE_SBUS;
    else if (!strcmp(prop, "ebus") && type == PROM_NODE_PCI)
        ntype = PROM_NODE_EBUS;
    else if (!strcmp(prop, "pci") && !type)
        ntype = PROM_NODE_PCI;
    strcpy(path + 1, prop);
    p = promGetReg(type);
    if (*p)
        strcat(path, p);
    if (node == searchNode)
        return 1;
    nextnode = promGetChild(node);
    if (nextnode &&
        promWalkNode2Pathname(strchr(path, 0), node, nextnode, searchNode,
                              ntype))
        return 1;
    nextnode = promGetSibling(node);
    if (nextnode &&
        promWalkNode2Pathname(path, parent, nextnode, searchNode, type))
        return 1;
    return 0;
}

char *
sparcPromNode2Pathname(sbusPromNodePtr pnode)
{
    char *ret;

    if (!pnode->node)
        return NULL;
    ret = malloc(4096);
    if (!ret)
        return NULL;
    if (promWalkNode2Pathname
        (ret, promRootNode, promGetChild(promRootNode), pnode->node, 0))
        return ret;
    free(ret);
    return NULL;
}

static int
promWalkPathname2Node(char *name, char *regstr, int parent, int type)
{
    int len, node, ret;
    char *prop, *p;

    for (;;) {
        prop = promGetProperty("name", &len);
        if (!prop || len <= 0)
            return 0;
        if ((!strcmp(prop, "sbus") || !strcmp(prop, "sbi")) && !type)
            type = PROM_NODE_SBUS;
        else if (!strcmp(prop, "ebus") && type == PROM_NODE_PCI)
            type = PROM_NODE_EBUS;
        else if (!strcmp(prop, "pci") && !type)
            type = PROM_NODE_PCI;
        for (node = promGetChild(parent); node; node = promGetSibling(node)) {
            prop = promGetProperty("name", &len);
            if (!prop || len <= 0)
                continue;
            if (*name && strcmp(name, prop))
                continue;
            if (*regstr) {
                p = promGetReg(type);
                if (!*p || strcmp(p + 1, regstr))
                    continue;
            }
            break;
        }
        if (!node) {
            for (node = promGetChild(parent); node; node = promGetSibling(node)) {
                ret = promWalkPathname2Node(name, regstr, node, type);
                if (ret)
                    return ret;
            }
            return 0;
        }
        name = strchr(regstr, 0) + 1;
        if (!*name)
            return node;
        p = strchr(name, '/');
        if (p)
            *p = 0;
        else
            p = strchr(name, 0);
        regstr = strchr(name, '@');
        if (regstr)
            *regstr++ = 0;
        else
            regstr = p;
        if (name == regstr)
            return 0;
        parent = node;
    }
}

int
sparcPromPathname2Node(const char *pathName)
{
    int i;
    char *name, *regstr, *p;

    i = strlen(pathName);
    name = malloc(i + 2);
    if (!name)
        return 0;
    strcpy(name, pathName);
    name[i + 1] = 0;
    if (name[0] != '/') {
        free(name);
        return 0;
    }
    p = strchr(name + 1, '/');
    if (p)
        *p = 0;
    else
        p = strchr(name, 0);
    regstr = strchr(name, '@');
    if (regstr)
        *regstr++ = 0;
    else
        regstr = p;
    if (name + 1 == regstr) {
        free(name);
        return 0;
    }
    promGetSibling(0);
    i = promWalkPathname2Node(name + 1, regstr, promRootNode, 0);
    free(name);
    return i;
}

void *
xf86MapSbusMem(sbusDevicePtr psdp, unsigned long offset, unsigned long size)
{
    void *ret;
    unsigned long pagemask = getpagesize() - 1;
    unsigned long off = offset & ~pagemask;
    unsigned long len = ((offset + size + pagemask) & ~pagemask) - off;

    if (psdp->fd == -1) {
        psdp->fd = open(psdp->device, O_RDWR);
        if (psdp->fd == -1)
            return NULL;
    }
    else if (psdp->fd < 0)
        return NULL;

    ret = (void *) mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE,
                         psdp->fd, off);
    if (ret == (void *) -1) {
        ret = (void *) mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED,
                             psdp->fd, off);
    }
    if (ret == (void *) -1)
        return NULL;

    return (char *) ret + (offset - off);
}

void
xf86UnmapSbusMem(sbusDevicePtr psdp, void *addr, unsigned long size)
{
    unsigned long mask = getpagesize() - 1;
    unsigned long base = (unsigned long) addr & ~mask;
    unsigned long len = (((unsigned long) addr + size + mask) & ~mask) - base;

    munmap((void *) base, len);
}

/* Tell OS that we are driving the HW cursor ourselves. */
void
xf86SbusHideOsHwCursor(sbusDevicePtr psdp)
{
    struct fbcursor fbcursor;
    unsigned char zeros[8];

    memset(&fbcursor, 0, sizeof(fbcursor));
    memset(&zeros, 0, sizeof(zeros));
    fbcursor.cmap.count = 2;
    fbcursor.cmap.red = zeros;
    fbcursor.cmap.green = zeros;
    fbcursor.cmap.blue = zeros;
    fbcursor.image = (char *) zeros;
    fbcursor.mask = (char *) zeros;
    fbcursor.size.x = 32;
    fbcursor.size.y = 1;
    fbcursor.set = FB_CUR_SETALL;
    ioctl(psdp->fd, FBIOSCURSOR, &fbcursor);
}

/* Set HW cursor colormap. */
void
xf86SbusSetOsHwCursorCmap(sbusDevicePtr psdp, int bg, int fg)
{
    struct fbcursor fbcursor;
    unsigned char red[2], green[2], blue[2];

    memset(&fbcursor, 0, sizeof(fbcursor));
    red[0] = bg >> 16;
    green[0] = bg >> 8;
    blue[0] = bg;
    red[1] = fg >> 16;
    green[1] = fg >> 8;
    blue[1] = fg;
    fbcursor.cmap.count = 2;
    fbcursor.cmap.red = red;
    fbcursor.cmap.green = green;
    fbcursor.cmap.blue = blue;
    fbcursor.set = FB_CUR_SETCMAP;
    ioctl(psdp->fd, FBIOSCURSOR, &fbcursor);
}
