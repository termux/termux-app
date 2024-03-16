/* This file has to be built with -mcpu=ev56 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "compiler.h"

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
    return *(volatile CARD8 *) ((unsigned long) Base + (Offset));
}

int
readDense16(void *Base, register unsigned long Offset)
{
    mem_barrier();
    return *(volatile CARD16 *) ((unsigned long) Base + (Offset));
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
    *(volatile CARD8 *) ((unsigned long) Base + (Offset)) = Value;
}

void
writeDense16(int Value, void *Base, register unsigned long Offset)
{
    write_mem_barrier();
    *(volatile CARD16 *) ((unsigned long) Base + (Offset)) = Value;
}

void
writeDense32(int Value, void *Base, register unsigned long Offset)
{
    write_mem_barrier();
    *(volatile CARD32 *) ((unsigned long) Base + (Offset)) = Value;
}
