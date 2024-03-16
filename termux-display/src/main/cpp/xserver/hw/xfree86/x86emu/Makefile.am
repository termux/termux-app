if INT10_X86EMU
noinst_LTLIBRARIES = libx86emu.la
endif

libx86emu_la_SOURCES = debug.c \
                      decode.c \
                      fpu.c \
                      ops2.c \
                      ops.c \
                      prim_ops.c \
                      sys.c \
                      x86emu.h

AM_CPPFLAGS = 

AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS)

EXTRA_DIST = validate.c \
             x86emu/debug.h \
             x86emu/decode.h \
             x86emu/fpu.h \
             x86emu/fpu_regs.h \
             x86emu/ops.h \
             x86emu/prim_asm.h \
             x86emu/prim_ops.h \
	     x86emu/prim_x86_gcc.h \
             x86emu/regs.h \
             x86emu/types.h \
             x86emu/x86emui.h
