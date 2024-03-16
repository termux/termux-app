
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "input.h"
#include "scrnintstr.h"
#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86OSpriv.h"

#include <machine/bwx.h>

/*
 * The following functions are used only on EV56 and greater CPUs,
 * and the assembler requires going to EV56 mode in order to emit
 * these instructions.
 */
__asm(".arch ev56");

int readDense8(void *Base, register unsigned long Offset);
int readDense16(void *Base, register unsigned long Offset);
int readDense32(void *Base, register unsigned long Offset);
void
 writeDense8(int Value, void *Base, register unsigned long Offset);
void
 writeDense16(int Value, void *Base, register unsigned long Offset);
void
 writeDense32(int Value, void *Base, register unsigned long Offset);

int
readDense8(void *Base, register unsigned long Offset)
{
    mem_barrier();
    return (alpha_ldbu((void *) ((unsigned long) Base + (Offset))));
}

int
readDense16(void *Base, register unsigned long Offset)
{
    mem_barrier();
    return (alpha_ldwu((void *) ((unsigned long) Base + (Offset))));
}

int
readDense32(void *Base, register unsigned long Offset)
{
    mem_barrier();
    return *(volatile CARD32 *) ((unsigned long) Base + (Offset));
}

void
writeDense8(int Value, void *Base, register unsigned long Offset)
{
    write_mem_barrier();
    alpha_stb((void *) ((unsigned long) Base + (Offset)), Value);
}

void
writeDense16(int Value, void *Base, register unsigned long Offset)
{
    write_mem_barrier();
    alpha_stw((void *) ((unsigned long) Base + (Offset)), Value);
}

void
writeDense32(int Value, void *Base, register unsigned long Offset)
{
    write_mem_barrier();
    *(volatile CARD32 *) ((unsigned long) Base + (Offset)) = Value;
}
