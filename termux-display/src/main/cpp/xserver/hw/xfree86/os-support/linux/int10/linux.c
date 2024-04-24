/*
 * linux specific part of the int10 module
 * Copyright 1999, 2000, 2001, 2002, 2003, 2004, 2008 Egbert Eich
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "compiler.h"
#define _INT10_PRIVATE
#include "xf86int10.h"
#ifdef __sparc__
#define DEV_MEM "/dev/fb"
#else
#define DEV_MEM "/dev/mem"
#endif
#define ALLOC_ENTRIES(x) ((V_RAM / x) - 1)
#define SHMERRORPTR (void *)(-1)

#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>

static int counter = 0;
static unsigned long int10Generation = 0;

static CARD8 read_b(xf86Int10InfoPtr pInt, int addr);
static CARD16 read_w(xf86Int10InfoPtr pInt, int addr);
static CARD32 read_l(xf86Int10InfoPtr pInt, int addr);
static void write_b(xf86Int10InfoPtr pInt, int addr, CARD8 val);
static void write_w(xf86Int10InfoPtr pInt, int addr, CARD16 val);
static void write_l(xf86Int10InfoPtr pInt, int addr, CARD32 val);

int10MemRec linuxMem = {
    read_b,
    read_w,
    read_l,
    write_b,
    write_w,
    write_l
};

typedef struct {
    int lowMem;
    int highMem;
    char *base;
    char *base_high;
    char *alloc;
} linuxInt10Priv;

#if defined DoSubModules

typedef enum {
    INT10_NOT_LOADED,
    INT10_LOADED_VM86,
    INT10_LOADED_X86EMU,
    INT10_LOAD_FAILED
} Int10LinuxSubModuleState;

static Int10LinuxSubModuleState loadedSubModule = INT10_NOT_LOADED;

static Int10LinuxSubModuleState int10LinuxLoadSubModule(ScrnInfoPtr pScrn);

#endif                          /* DoSubModules */

static Bool
readLegacy(struct pci_device *dev, unsigned char *buf, int base, int len)
{
    void *map;

    if (pci_device_map_legacy(dev, base, len, 0, &map))
        return FALSE;

    memcpy(buf, map, len);
    pci_device_unmap_legacy(dev, man, len);

    return TRUE;
}

xf86Int10InfoPtr
xf86ExtendedInitInt10(int entityIndex, int Flags)
{
    xf86Int10InfoPtr pInt = NULL;
    int screen;
    int fd;
    static void *vidMem = NULL;
    static void *sysMem = NULL;
    void *vMem = NULL;
    void *options = NULL;
    int low_mem;
    int high_mem = -1;
    char *base = SHMERRORPTR;
    char *base_high = SHMERRORPTR;
    int pagesize;
    memType cs;
    legacyVGARec vga;
    Bool videoBiosMapped = FALSE;
    ScrnInfoPtr pScrn;
    if (int10Generation != serverGeneration) {
        counter = 0;
        int10Generation = serverGeneration;
    }

    pScrn = xf86FindScreenForEntity(entityIndex);
    screen = pScrn->scrnIndex;

    options = xf86HandleInt10Options(pScrn, entityIndex);

    if (int10skip(options)) {
        free(options);
        return NULL;
    }

#if defined DoSubModules
    if (loadedSubModule == INT10_NOT_LOADED)
        loadedSubModule = int10LinuxLoadSubModule(pScrn);

    if (loadedSubModule == INT10_LOAD_FAILED)
        return NULL;
#endif

    if ((!vidMem) || (!sysMem)) {
        if ((fd = open(DEV_MEM, O_RDWR, 0)) >= 0) {
            if (!sysMem) {
                DebugF("Mapping sys bios area\n");
                if ((sysMem = mmap((void *) (SYS_BIOS), BIOS_SIZE,
                                   PROT_READ | PROT_EXEC,
                                   MAP_SHARED | MAP_FIXED, fd, SYS_BIOS))
                    == MAP_FAILED) {
                    xf86DrvMsg(screen, X_ERROR, "Cannot map SYS BIOS\n");
                    close(fd);
                    goto error0;
                }
            }
            if (!vidMem) {
                DebugF("Mapping VRAM area\n");
                if ((vidMem = mmap((void *) (V_RAM), VRAM_SIZE,
                                   PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_SHARED | MAP_FIXED, fd, V_RAM))
                    == MAP_FAILED) {
                    xf86DrvMsg(screen, X_ERROR, "Cannot map V_RAM\n");
                    close(fd);
                    goto error0;
                }
            }
            close(fd);
        }
        else {
            xf86DrvMsg(screen, X_ERROR, "Cannot open %s\n", DEV_MEM);
            goto error0;
        }
    }

    pInt = (xf86Int10InfoPtr) xnfcalloc(1, sizeof(xf86Int10InfoRec));
    pInt->pScrn = pScrn;
    pInt->entityIndex = entityIndex;
    pInt->dev = xf86GetPciInfoForEntity(entityIndex);

    if (!xf86Int10ExecSetup(pInt))
        goto error0;
    pInt->mem = &linuxMem;
    pagesize = getpagesize();
    pInt->private = (void *) xnfcalloc(1, sizeof(linuxInt10Priv));
    ((linuxInt10Priv *) pInt->private)->alloc =
        (void *) xnfcalloc(1, ALLOC_ENTRIES(pagesize));

    if (!xf86IsEntityPrimary(entityIndex)) {
        DebugF("Mapping high memory area\n");
        if ((high_mem = shmget(counter++, HIGH_MEM_SIZE,
                               IPC_CREAT | SHM_R | SHM_W)) == -1) {
            if (errno == ENOSYS)
                xf86DrvMsg(screen, X_ERROR, "shmget error\n Please reconfigure"
                           " your kernel to include System V IPC support\n");
            else
                xf86DrvMsg(screen, X_ERROR,
                           "shmget(highmem) error: %s\n", strerror(errno));
            goto error1;
        }
    }
    else {
        DebugF("Mapping Video BIOS\n");
        videoBiosMapped = TRUE;
        if ((fd = open(DEV_MEM, O_RDWR, 0)) >= 0) {
            if ((vMem = mmap((void *) (V_BIOS), SYS_BIOS - V_BIOS,
                             PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_SHARED | MAP_FIXED, fd, V_BIOS))
                == MAP_FAILED) {
                xf86DrvMsg(screen, X_ERROR, "Cannot map V_BIOS\n");
                close(fd);
                goto error1;
            }
            close(fd);
        }
        else
            goto error1;
    }
    ((linuxInt10Priv *) pInt->private)->highMem = high_mem;

    DebugF("Mapping 640kB area\n");
    if ((low_mem = shmget(counter++, V_RAM, IPC_CREAT | SHM_R | SHM_W)) == -1) {
        xf86DrvMsg(screen, X_ERROR,
                   "shmget(lowmem) error: %s\n", strerror(errno));
        goto error2;
    }

    ((linuxInt10Priv *) pInt->private)->lowMem = low_mem;
    base = shmat(low_mem, 0, 0);
    if (base == SHMERRORPTR) {
        xf86DrvMsg(screen, X_ERROR,
                   "shmat(low_mem) error: %s\n", strerror(errno));
        goto error3;
    }
    ((linuxInt10Priv *) pInt->private)->base = base;
    if (high_mem > -1) {
        base_high = shmat(high_mem, 0, 0);
        if (base_high == SHMERRORPTR) {
            xf86DrvMsg(screen, X_ERROR,
                       "shmat(high_mem) error: %s\n", strerror(errno));
            goto error3;
        }
        ((linuxInt10Priv *) pInt->private)->base_high = base_high;
    }
    else
        ((linuxInt10Priv *) pInt->private)->base_high = NULL;

    if (!MapCurrentInt10(pInt))
        goto error3;

    Int10Current = pInt;

    DebugF("Mapping int area\n");
    /* note: yes, we really are writing the 0 page here */
    if (!readLegacy(pInt->dev, (unsigned char *) 0, 0, LOW_PAGE_SIZE)) {
        xf86DrvMsg(screen, X_ERROR, "Cannot read int vect\n");
        goto error3;
    }
    DebugF("done\n");
    /*
     * Read in everything between V_BIOS and SYS_BIOS as some system BIOSes
     * have executable code there.  Note that xf86ReadBIOS() can only bring in
     * 64K bytes at a time.
     */
    if (!videoBiosMapped) {
        memset((void *) V_BIOS, 0, SYS_BIOS - V_BIOS);
        DebugF("Reading BIOS\n");
        for (cs = V_BIOS; cs < SYS_BIOS; cs += V_BIOS_SIZE)
            if (!readLegacy(pInt->dev, (void *)cs, cs, V_BIOS_SIZE))
                xf86DrvMsg(screen, X_WARNING,
                           "Unable to retrieve all of segment 0x%06lX.\n",
                           (long) cs);
        DebugF("done\n");
    }

    if (xf86IsEntityPrimary(entityIndex) && !(initPrimary(options))) {
        if (!xf86int10GetBiosSegment(pInt, NULL))
            goto error3;

        set_return_trap(pInt);
#ifdef _PC
        pInt->Flags = Flags & (SET_BIOS_SCRATCH | RESTORE_BIOS_SCRATCH);
        if (!(pInt->Flags & SET_BIOS_SCRATCH))
            pInt->Flags &= ~RESTORE_BIOS_SCRATCH;
        xf86Int10SaveRestoreBIOSVars(pInt, TRUE);
#endif
    }
    else {
        const BusType location_type = xf86int10GetBiosLocationType(pInt);

        switch (location_type) {
        case BUS_PCI:{
            int err;
            struct pci_device *rom_device =
                xf86GetPciInfoForEntity(pInt->entityIndex);

            pci_device_enable(rom_device);
            err = pci_device_read_rom(rom_device, (unsigned char *) (V_BIOS));
            if (err) {
                xf86DrvMsg(screen, X_ERROR, "Cannot read V_BIOS (%s)\n",
                           strerror(err));
                goto error3;
            }

            pInt->BIOSseg = V_BIOS >> 4;
            break;
        }
        default:
            goto error3;
        }

        pInt->num = 0xe6;
        reset_int_vect(pInt);
        set_return_trap(pInt);
        LockLegacyVGA(pInt, &vga);
        xf86ExecX86int10(pInt);
        UnlockLegacyVGA(pInt, &vga);
    }
#ifdef DEBUG
    dprint(0xc0000, 0x20);
#endif

    free(options);
    return pInt;

 error3:
    if (base_high)
        shmdt(base_high);
    shmdt(base);
    shmdt(0);
    if (base_high)
        shmdt((char *) HIGH_MEM);
    shmctl(low_mem, IPC_RMID, NULL);
    Int10Current = NULL;
 error2:
    if (high_mem > -1)
        shmctl(high_mem, IPC_RMID, NULL);
 error1:
    if (vMem)
        munmap(vMem, SYS_BIOS - V_BIOS);
    free(((linuxInt10Priv *) pInt->private)->alloc);
    free(pInt->private);
 error0:
    free(options);
    free(pInt);
    return NULL;
}

Bool
MapCurrentInt10(xf86Int10InfoPtr pInt)
{
    void *addr;
    int fd = -1;

    if (Int10Current) {
        shmdt(0);
        if (((linuxInt10Priv *) Int10Current->private)->highMem >= 0)
            shmdt((char *) HIGH_MEM);
        else
            munmap((void *) V_BIOS, (SYS_BIOS - V_BIOS));
    }
    addr =
        shmat(((linuxInt10Priv *) pInt->private)->lowMem, (char *) 1, SHM_RND);
    if (addr == SHMERRORPTR) {
        xf86DrvMsg(pInt->pScrn->scrnIndex, X_ERROR, "Cannot shmat() low memory\n");
        xf86DrvMsg(pInt->pScrn->scrnIndex, X_ERROR,
                   "shmat(low_mem) error: %s\n", strerror(errno));
        return FALSE;
    }
    if (mprotect((void *) 0, V_RAM, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
        xf86DrvMsg(pInt->pScrn->scrnIndex, X_ERROR,
                   "Cannot set EXEC bit on low memory: %s\n", strerror(errno));

    if (((linuxInt10Priv *) pInt->private)->highMem >= 0) {
        addr = shmat(((linuxInt10Priv *) pInt->private)->highMem,
                     (char *) HIGH_MEM, 0);
        if (addr == SHMERRORPTR) {
            xf86DrvMsg(pInt->pScrn->scrnIndex, X_ERROR,
                       "Cannot shmat() high memory\n");
            xf86DrvMsg(pInt->pScrn->scrnIndex, X_ERROR,
                       "shmget error: %s\n", strerror(errno));
            return FALSE;
        }
        if (mprotect((void *) HIGH_MEM, HIGH_MEM_SIZE,
                     PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
            xf86DrvMsg(pInt->pScrn->scrnIndex, X_ERROR,
                       "Cannot set EXEC bit on high memory: %s\n",
                       strerror(errno));
    }
    else {
        if ((fd = open(DEV_MEM, O_RDWR, 0)) >= 0) {
            if (mmap((void *) (V_BIOS), SYS_BIOS - V_BIOS,
                     PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_SHARED | MAP_FIXED, fd, V_BIOS)
                == MAP_FAILED) {
                xf86DrvMsg(pInt->pScrn->scrnIndex, X_ERROR, "Cannot map V_BIOS\n");
                close(fd);
                return FALSE;
            }
        }
        else {
            xf86DrvMsg(pInt->pScrn->scrnIndex, X_ERROR, "Cannot open %s\n", DEV_MEM);
            return FALSE;
        }
        close(fd);
    }

    return TRUE;
}

void
xf86FreeInt10(xf86Int10InfoPtr pInt)
{
    if (!pInt)
        return;

#ifdef _PC
    xf86Int10SaveRestoreBIOSVars(pInt, FALSE);
#endif
    if (Int10Current == pInt) {
        shmdt(0);
        if (((linuxInt10Priv *) pInt->private)->highMem >= 0)
            shmdt((char *) HIGH_MEM);
        else
            munmap((void *) V_BIOS, (SYS_BIOS - V_BIOS));
        Int10Current = NULL;
    }

    if (((linuxInt10Priv *) pInt->private)->base_high)
        shmdt(((linuxInt10Priv *) pInt->private)->base_high);
    shmdt(((linuxInt10Priv *) pInt->private)->base);
    shmctl(((linuxInt10Priv *) pInt->private)->lowMem, IPC_RMID, NULL);
    if (((linuxInt10Priv *) pInt->private)->highMem >= 0)
        shmctl(((linuxInt10Priv *) pInt->private)->highMem, IPC_RMID, NULL);
    free(((linuxInt10Priv *) pInt->private)->alloc);
    free(pInt->private);
    free(pInt);
}

void *
xf86Int10AllocPages(xf86Int10InfoPtr pInt, int num, int *off)
{
    int pagesize = getpagesize();
    int num_pages = ALLOC_ENTRIES(pagesize);
    int i, j;

    for (i = 0; i < (num_pages - num); i++) {
        if (((linuxInt10Priv *) pInt->private)->alloc[i] == 0) {
            for (j = i; j < (num + i); j++)
                if ((((linuxInt10Priv *) pInt->private)->alloc[j] != 0))
                    break;
            if (j == (num + i))
                break;
            else
                i = i + num;
        }
    }
    if (i == (num_pages - num))
        return NULL;

    for (j = i; j < (i + num); j++)
        ((linuxInt10Priv *) pInt->private)->alloc[j] = 1;

    *off = (i + 1) * pagesize;

    return ((linuxInt10Priv *) pInt->private)->base + ((i + 1) * pagesize);
}

void
xf86Int10FreePages(xf86Int10InfoPtr pInt, void *pbase, int num)
{
    int pagesize = getpagesize();
    int first = (((unsigned long) pbase
                  - (unsigned long) ((linuxInt10Priv *) pInt->private)->base)
                 / pagesize) - 1;
    int i;

    for (i = first; i < (first + num); i++)
        ((linuxInt10Priv *) pInt->private)->alloc[i] = 0;
}

static CARD8
read_b(xf86Int10InfoPtr pInt, int addr)
{
    return *((CARD8 *) (memType) addr);
}

static CARD16
read_w(xf86Int10InfoPtr pInt, int addr)
{
    return *((CARD16 *) (memType) addr);
}

static CARD32
read_l(xf86Int10InfoPtr pInt, int addr)
{
    return *((CARD32 *) (memType) addr);
}

static void
write_b(xf86Int10InfoPtr pInt, int addr, CARD8 val)
{
    *((CARD8 *) (memType) addr) = val;
}

static void
write_w(xf86Int10InfoPtr pInt, int addr, CARD16 val)
{
    *((CARD16 *) (memType) addr) = val;
}

static
    void
write_l(xf86Int10InfoPtr pInt, int addr, CARD32 val)
{
    *((CARD32 *) (memType) addr) = val;
}

void *
xf86int10Addr(xf86Int10InfoPtr pInt, CARD32 addr)
{
    if (addr < V_RAM)
        return ((linuxInt10Priv *) pInt->private)->base + addr;
    else if (addr < V_BIOS)
        return (void *) (memType) addr;
    else if (addr < SYS_BIOS) {
        if (((linuxInt10Priv *) pInt->private)->base_high)
            return (void *) (((linuxInt10Priv *) pInt->private)->base_high
                              - V_BIOS + addr);
        else
            return (void *) (memType) addr;
    }
    else
        return (void *) (memType) addr;
}

#if defined DoSubModules

static Bool
vm86_tst(void)
{
    int __res;

#ifdef __PIC__
    /* When compiling with -fPIC, we can't use asm constraint "b" because
       %ebx is already taken by gcc. */
    __asm__ __volatile__("pushl %%ebx\n\t"
                         "movl %2,%%ebx\n\t"
                         "movl %1,%%eax\n\t"
                         "int $0x80\n\t" "popl %%ebx":"=a"(__res)
                         :"n"((int) 113), "r"(NULL));
#else
    __asm__ __volatile__("int $0x80\n\t":"=a"(__res):"a"((int) 113),
                         "b"((struct vm86_struct *) NULL));
#endif

    if (__res < 0 && __res == -ENOSYS)
        return FALSE;

    return TRUE;
}

static Int10LinuxSubModuleState
int10LinuxLoadSubModule(ScrnInfoPtr pScrn)
{
    if (vm86_tst()) {
        if (xf86LoadSubModule(pScrn, "vm86"))
            return INT10_LOADED_VM86;
    }
    if (xf86LoadSubModule(pScrn, "x86emu"))
        return INT10_LOADED_X86EMU;

    return INT10_LOAD_FAILED;
}

#endif                          /* DoSubModules */
