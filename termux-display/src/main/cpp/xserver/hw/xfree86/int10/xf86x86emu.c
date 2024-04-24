/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#define _INT10_PRIVATE
#include "xf86int10.h"
#include "int10Defines.h"
#include <x86emu.h>

#define M _X86EMU_env

static void
x86emu_do_int(int num)
{
    Int10Current->num = num;

    if (!int_handler(Int10Current)) {
        X86EMU_halt_sys();
    }
}

void
xf86ExecX86int10(xf86Int10InfoPtr pInt)
{
    int sig = setup_int(pInt);

    if (sig < 0)
        return;

    if (int_handler(pInt)) {
        X86EMU_exec();
    }

    finish_int(pInt, sig);
}

Bool
xf86Int10ExecSetup(xf86Int10InfoPtr pInt)
{
    int i;
    X86EMU_intrFuncs intFuncs[256];

    X86EMU_pioFuncs pioFuncs = {
        .inb = x_inb,
        .inw = x_inw,
        .inl = x_inl,
        .outb = x_outb,
        .outw = x_outw,
        .outl = x_outl
    };

    X86EMU_memFuncs memFuncs = {
        (&Mem_rb),
        (&Mem_rw),
        (&Mem_rl),
        (&Mem_wb),
        (&Mem_ww),
        (&Mem_wl)
    };

    X86EMU_setupMemFuncs(&memFuncs);

    pInt->cpuRegs = &M;
    M.mem_base = 0;
    M.mem_size = 1024 * 1024 + 1024;
    X86EMU_setupPioFuncs(&pioFuncs);

    for (i = 0; i < 256; i++)
        intFuncs[i] = x86emu_do_int;
    X86EMU_setupIntrFuncs(intFuncs);
    return TRUE;
}

void
printk(const char *fmt, ...)
{
    va_list argptr;

    va_start(argptr, fmt);
    VErrorF(fmt, argptr);
    va_end(argptr);
}
