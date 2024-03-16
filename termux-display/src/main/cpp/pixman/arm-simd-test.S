.text
.arch armv6
.object_arch armv4
.arm
.altmacro
#ifndef __ARM_EABI__
#error EABI is required (to be sure that calling conventions are compatible)
#endif
pld [r0]
uqadd8 r0, r0, r0
