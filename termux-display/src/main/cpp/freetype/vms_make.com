$! make FreeType 2 under OpenVMS
$!
$! Copyright (C) 2003-2023 by
$! David Turner, Robert Wilhelm, and Werner Lemberg.
$!
$! This file is part of the FreeType project, and may only be used, modified,
$! and distributed under the terms of the FreeType project license,
$! LICENSE.TXT.  By continuing to use, modify, or distribute this file you
$! indicate that you have read the license and understand and accept it
$! fully.
$!
$!
$! External libraries (like FreeType, XPM, etc.) are supported via the
$! config file VMSLIB.DAT. Please check the sample file, which is part of this
$! distribution, for the information you need to provide
$!
$! This procedure currently does support the following commandline options
$! in arbitrary order
$!
$! * DEBUG - Compile modules with /noopt/debug and link shareable image
$!           with /debug
$! * LOPTS - Options to be passed to the link command
$! * CCOPT - Options to be passed to the C compiler
$!
$! In case of problems with the install you might contact me at
$! zinser@zinser.no-ip.info (preferred) or
$! zinser@sysdev.deutsche-boerse.com (work)
$!
$! Make procedure history for FreeType 2
$!
$!------------------------------------------------------------------------------
$! Version history
$! 0.01 20040401 First version to receive a number
$! 0.02 20041030 Add error handling, FreeType 2.1.9
$!
$ on error then goto err_exit
$ true  = 1
$ false = 0
$ tmpnam = "temp_" + f$getjpi("","pid")
$ tt = tmpnam + ".txt"
$ tc = tmpnam + ".c"
$ th = tmpnam + ".h"
$ its_decc = false
$ its_vaxc = false
$ its_gnuc = false
$!
$! Setup variables holding "config" information
$!
$ Make    = ""
$ ccopt   = "/name=(as_is,short)/float=ieee"
$ lopts   = ""
$ dnsrl   = ""
$ aconf_in_file = "config.hin"
$ name    = "Freetype2"
$ mapfile = name + ".map"
$ optfile = name + ".opt"
$ s_case  = false
$ liblist = ""
$!
$ whoami = f$parse(f$environment("Procedure"),,,,"NO_CONCEAL")
$ mydef  = F$parse(whoami,,,"DEVICE")
$ mydir  = f$parse(whoami,,,"DIRECTORY") - "]["
$ myproc = f$parse(whoami,,,"Name") + f$parse(whoami,,,"type")
$!
$! Check for MMK/MMS
$!
$ If F$Search ("Sys$System:MMS.EXE") .nes. "" Then Make = "MMS"
$ If F$Type (MMK) .eqs. "STRING" Then Make = "MMK"
$!
$! Which command parameters were given
$!
$ gosub check_opts
$!
$! Create option file
$!
$ open/write optf 'optfile'
$!
$! Pull in external libraries
$!
$ create libs.opt
$ open/write libsf libs.opt
$ gosub check_create_vmslib
$!
$! Create objects
$!
$ if libdefs .nes. ""
$ then
$   ccopt = ccopt + "/define=(" + f$extract(0,f$length(libdefs)-1,libdefs) + ")"
$ endif
$!
$ if f$locate("AS_IS",f$edit(ccopt,"UPCASE")) .lt. f$length(ccopt) -
    then s_case = true
$ gosub crea_mms
$!
$ 'Make' /macro=(comp_flags="''ccopt'")
$ purge/nolog [...]descrip.mms
$!
$! Add them to options
$!
$FLOOP:
$  file = f$edit(f$search("[...]*.obj"),"UPCASE")
$  if (file .nes. "")
$  then
$    if f$locate("DEMOS",file) .eqs. f$length(file) then write optf file
$    goto floop
$  endif
$!
$ close optf
$!
$!
$! Alpha gets a shareable image
$!
$ If f$getsyi("HW_MODEL") .gt. 1024
$ Then
$   write sys$output "Creating freetype2shr.exe"
$   If f$getsyi("HW_MODEL") .le. 2048
$   Then
$     call anal_obj_axp 'optfile' _link.opt
$   Else
$     copy _link.opt_ia64 _link.opt
$     close libsf
$     copy libs.opt_ia64 libs.opt
$   endif
$   open/append  optf 'optfile'
$   if s_case then WRITE optf "case_sensitive=YES"
$   close optf
$   LINK_/NODEB/SHARE=[.lib]freetype2shr.exe -
                            'optfile'/opt,libs.opt/opt,_link.opt/opt
$ endif
$!
$ exit
$!
$
$ERR_LIB:
$ write sys$output "Error reading config file vmslib.dat"
$ goto err_exit
$FT2_ERR:
$ write sys$output "Could not locate FreeType 2 include files"
$ goto err_exit
$ERR_EXIT:
$ set message/facil/ident/sever/text
$ close/nolog optf
$ close/nolog out
$ close/nolog libdata
$ close/nolog in
$ close/nolog atmp
$ close/nolog xtmp
$ write sys$output "Exiting..."
$ exit 2
$!
$!------------------------------------------------------------------------------
$!
$! If MMS/MMK are available dump out the descrip.mms if required
$!
$CREA_MMS:
$ write sys$output "Creating descrip.mms files ..."
$ write sys$output "... Main directory"
$ create descrip.mms
$ open/append out descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 build system -- top-level Makefile for OpenVMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.
$ EOD
$ write out "CFLAGS = ", ccopt
$ copy sys$input: out
$ deck


all :
        define config [--.include.freetype.config]
        define internal [--.include.freetype.internal]
        define autofit [-.autofit]
        define base [-.base]
        define cache [-.cache]
        define cff [-.cff]
        define cid [-.cid]
        define freetype [--.include.freetype]
        define pcf [-.pcf]
        define psaux [-.psaux]
        define psnames [-.psnames]
        define raster [-.raster]
        define sfnt [-.sfnt]
        define smooth [-.smooth]
        define truetype [-.truetype]
        define type1 [-.type1]
        define winfonts [-.winfonts]
        if f$search("lib.dir") .eqs. "" then create/directory [.lib]
        set default [.builds.vms]
        $(MMS)$(MMSQUALIFIERS)
        set default [--.src.autofit]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.base]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.bdf]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.cache]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.cff]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.cid]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.gxvalid]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.gzip]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.bzip2]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.lzw]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.otvalid]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.pcf]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.pfr]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.psaux]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.pshinter]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.psnames]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.raster]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.sfnt]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.smooth]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.truetype]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.type1]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.type42]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.winfonts]
        $(MMS)$(MMSQUALIFIERS)
        set default [--]

# EOF
$ eod
$ close out
$ write sys$output "... [.builds.vms] directory"
$ create [.builds.vms]descrip.mms
$ open/append out [.builds.vms]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 system rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/list/show=all/include=([],[--.include],[--.src.base])

OBJS=ftsystem.obj

all : $(OBJS)
        library/create [--.lib]freetype.olb $(OBJS)

ftsystem.obj : ftsystem.c ftconfig.h

# EOF
$ eod
$ close out
$ write sys$output "... [.src.autofit] directory"
$ create [.src.autofit]descrip.mms
$ open/append out [.src.autofit]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 auto-fit module compilation rules for VMS
#


# Copyright 2002-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.autofit])

OBJS=autofit.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.base] directory"
$ create [.src.base]descrip.mms
$ open/append out [.src.base]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 base layer compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.builds.vms],[--.include],[--.src.base])

OBJS=ftbase.obj,\
     ftbbox.obj,\
     ftbdf.obj,\
     ftbitmap.obj,\
     ftcid.obj,\
     ftdebug.obj,\
     ftfstype.obj,\
     ftgasp.obj,\
     ftglyph.obj,\
     ftinit.obj,\
     ftmm.obj,\
     ftpfr.obj,\
     ftstroke.obj,\
     ftsynth.obj,\
     fttype1.obj,\
     ftwinfnt.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.bdf] directory"
$ create [.src.bdf]descrip.mms
$ open/append out [.src.bdf]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 BDF driver compilation rules for VMS
#


# Copyright 2002-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.bdf])

OBJS=bdf.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.cache] directory"
$ create [.src.cache]descrip.mms
$ open/append out [.src.cache]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 Cache compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.cache])/nowarn

OBJS=ftcache.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.cff] directory"
$ create [.src.cff]descrip.mms
$ open/append out [.src.cff]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 OpenType/CFF driver compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.cff])

OBJS=cff.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.cid] directory"
$ create [.src.cid]descrip.mms
$ open/append out [.src.cid]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 CID driver compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.cid])

OBJS=type1cid.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.gxvalid] directory"
$ create [.src.gxvalid]descrip.mms
$ open/append out [.src.gxvalid]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 TrueTypeGX/AAT validation driver configuration rules for VMS
#


# Copyright 2004-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.gxvalid])

OBJS=gxvalid.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.gzip] directory"
$ create [.src.gzip]descrip.mms
$ open/append out [.src.gzip]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 GZip support compilation rules for VMS
#


# Copyright 2002-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.
$EOD
$ if libincs .nes. "" then write out "LIBINCS = ", libincs - ",", ","
$ write out "COMP_FLAGS = ", ccopt
$ copy sys$input: out
$ deck

CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=($(LIBINCS)[--.include],[--.src.gzip])

OBJS=ftgzip.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.bzip2] directory"
$ create [.src.bzip2]descrip.mms
$ open/append out [.src.bzip2]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 BZIP2 support compilation rules for VMS
#


# Copyright 2010-2019 by
# Joel Klinghed.
#
# based on `src/lzw/rules.mk'
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.
$EOD
$ if libincs .nes. "" then write out "LIBINCS = ", libincs - ",", ","
$ write out "COMP_FLAGS = ", ccopt
$ copy sys$input: out
$ deck

CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.bzip2])

OBJS=ftbzip2.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.lzw] directory"
$ create [.src.lzw]descrip.mms
$ open/append out [.src.lzw]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 LZW support compilation rules for VMS
#


# Copyright 2004-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.
$EOD
$ if libincs .nes. "" then write out "LIBINCS = ", libincs - ",", ","
$ write out "COMP_FLAGS = ", ccopt
$ copy sys$input: out
$ deck

CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=($(LIBINCS)[--.include],[--.src.lzw])

OBJS=ftlzw.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.otvalid] directory"
$ create [.src.otvalid]descrip.mms
$ open/append out [.src.otvalid]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 OpenType validation module compilation rules for VMS
#


# Copyright 2004-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.otvalid])

OBJS=otvalid.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.pcf] directory"
$ create [.src.pcf]descrip.mms
$ open/append out [.src.pcf]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 pcf driver compilation rules for VMS
#


# Copyright (C) 2001, 2002 by
# Francesco Zappa Nardelli
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.pcf])

OBJS=pcf.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.pfr] directory"
$ create [.src.pfr]descrip.mms
$ open/append out [.src.pfr]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 PFR driver compilation rules for VMS
#


# Copyright 2002-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.pfr])

OBJS=pfr.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.psaux] directory"
$ create [.src.psaux]descrip.mms
$ open/append out [.src.psaux]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 PSaux driver compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.psaux])

OBJS=psaux.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.pshinter] directory"
$ create [.src.pshinter]descrip.mms
$ open/append out [.src.pshinter]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 PSHinter driver compilation rules for OpenVMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.psnames])

OBJS=pshinter.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.psnames] directory"
$ create [.src.psnames]descrip.mms
$ open/append out [.src.psnames]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 psnames driver compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.psnames])

OBJS=psnames.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.raster] directory"
$ create [.src.raster]descrip.mms
$ open/append out [.src.raster]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 renderer module compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.raster])

OBJS=raster.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.sfnt] directory"
$ create [.src.sfnt]descrip.mms
$ open/append out [.src.sfnt]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 SFNT driver compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.sfnt])

OBJS=sfnt.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.smooth] directory"
$ create [.src.smooth]descrip.mms
$ open/append out [.src.smooth]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 smooth renderer module compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.smooth])

OBJS=smooth.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.truetype] directory"
$ create [.src.truetype]descrip.mms
$ open/append out [.src.truetype]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 TrueType driver compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.truetype])

OBJS=truetype.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.type1] directory"
$ create [.src.type1]descrip.mms
$ open/append out [.src.type1]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 Type1 driver compilation rules for VMS
#


# Copyright 1996-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.type1])

OBJS=type1.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.type42] directory"
$ create [.src.type42]descrip.mms
$ open/append out [.src.type42]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 Type 42 driver compilation rules for VMS
#


# Copyright 2002-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.type42])

OBJS=type42.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ write sys$output "... [.src.winfonts] directory"
$ create [.src.winfonts]descrip.mms
$ open/append out [.src.winfonts]descrip.mms
$ copy sys$input: out
$ deck
#
# FreeType 2 Windows FNT/FON driver compilation rules for VMS
#


# Copyright 2001-2019 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


CFLAGS=$(COMP_FLAGS)$(DEBUG)/include=([--.include],[--.src.winfonts])

OBJS=winfnt.obj

all : $(OBJS)
        library [--.lib]freetype.olb $(OBJS)

# EOF
$ eod
$ close out
$ return
$!------------------------------------------------------------------------------
$!
$! Check command line options and set symbols accordingly
$!
$ CHECK_OPTS:
$ i = 1
$ OPT_LOOP:
$ if i .lt. 9
$ then
$   cparm = f$edit(p'i',"upcase")
$   if cparm .eqs. "DEBUG"
$   then
$     ccopt = ccopt + "/noopt/deb"
$     lopts = lopts + "/deb"
$   endif
$   if f$locate("CCOPT=",cparm) .lt. f$length(cparm)
$   then
$     start = f$locate("=",cparm) + 1
$     len   = f$length(cparm) - start
$     ccopt = ccopt + f$extract(start,len,cparm)
$   endif
$   if cparm .eqs. "LINK" then linkonly = true
$   if f$locate("LOPTS=",cparm) .lt. f$length(cparm)
$   then
$     start = f$locate("=",cparm) + 1
$     len   = f$length(cparm) - start
$     lopts = lopts + f$extract(start,len,cparm)
$   endif
$   if f$locate("CC=",cparm) .lt. f$length(cparm)
$   then
$     start  = f$locate("=",cparm) + 1
$     len    = f$length(cparm) - start
$     cc_com = f$extract(start,len,cparm)
      if (cc_com .nes. "DECC") .and. -
         (cc_com .nes. "VAXC") .and. -
	 (cc_com .nes. "GNUC")
$     then
$       write sys$output "Unsupported compiler choice ''cc_com' ignored"
$       write sys$output "Use DECC, VAXC, or GNUC instead"
$     else
$     	if cc_com .eqs. "DECC" then its_decc = true
$     	if cc_com .eqs. "VAXC" then its_vaxc = true
$     	if cc_com .eqs. "GNUC" then its_gnuc = true
$     endif
$   endif
$   if f$locate("MAKE=",cparm) .lt. f$length(cparm)
$   then
$     start  = f$locate("=",cparm) + 1
$     len    = f$length(cparm) - start
$     mmks = f$extract(start,len,cparm)
$     if (mmks .eqs. "MMK") .or. (mmks .eqs. "MMS")
$     then
$       make = mmks
$     else
$       write sys$output "Unsupported make choice ''mmks' ignored"
$       write sys$output "Use MMK or MMS instead"
$     endif
$   endif
$   i = i + 1
$   goto opt_loop
$ endif
$ return
$!------------------------------------------------------------------------------
$!
$! Take care of driver file with information about external libraries
$!
$! Version history
$! 0.01 20040220 First version to receive a number
$! 0.02 20040229 Echo current procedure name; use general error exit handler
$!               Remove xpm hack -> Replaced by more general dnsrl handling
$CHECK_CREATE_VMSLIB:
$!
$ if f$search("VMSLIB.DAT") .eqs. ""
$ then
$   type/out=vmslib.dat sys$input
!
! This is a simple driver file with information used by vms_make.com to
! check if external libraries (like t1lib and FreeType) are available on
! the system.
!
! Layout of the file:
!
!    - Lines starting with ! are treated as comments
!    - Elements in a data line are separated by # signs
!    - The elements need to be listed in the following order
!      1.) Name of the Library (only used for informative messages
!                               from vms_make.com)
!      2.) Location where the object library can be found
!      3.) Location where the include files for the library can be found
!      4.) Include file used to verify library location
!      5.) CPP define to pass to the build to indicate availability of
!          the library
!
! Example: The following lines show how definitions
!          might look like. They are site specific and the locations of the
!          library and include files need almost certainly to be changed.
!
! Location: All of the libraries can be found at the following addresses
!
!   ZLIB:     http://zinser.no-ip.info/vms/sw/zlib.htmlx
!
ZLIB # sys$library:libz.olb # sys$library: # zlib.h # FT_CONFIG_OPTION_SYSTEM_ZLIB
$   write sys$output "New driver file vmslib.dat created."
$   write sys$output "Please customize library locations for your site"
$   write sys$output "and afterwards re-execute ''myproc'"
$   goto err_exit
$ endif
$!
$! Init symbols used to hold CPP definitions and include path
$!
$ libdefs = "FT2_BUILD_LIBRARY,FT_CONFIG_OPTION_OLD_INTERNALS,"
$ libincs = ""
$!
$! Open data file with location of libraries
$!
$ open/read/end=end_lib/err=err_lib libdata VMSLIB.DAT
$LIB_LOOP:
$ read/end=end_lib libdata libline
$ libline = f$edit(libline, "UNCOMMENT,COLLAPSE")
$ if libline .eqs. "" then goto LIB_LOOP ! Comment line
$ libname = f$edit(f$element(0,"#",libline),"UPCASE")
$ write sys$output "Processing ''libname' setup ..."
$ libloc  = f$element(1,"#",libline)
$ libsrc  = f$element(2,"#",libline)
$ testinc = f$element(3,"#",libline)
$ cppdef  = f$element(4,"#",libline)
$ old_cpp = f$locate("=1",cppdef)
$ if old_cpp.lt.f$length(cppdef) then cppdef = f$extract(0,old_cpp,cppdef)
$ if f$search("''libloc'").eqs. ""
$ then
$   write sys$output "Can not find library ''libloc' - Skipping ''libname'"
$   goto LIB_LOOP
$ endif
$ libsrc_elem = 0
$ libsrc_found = false
$LIBSRC_LOOP:
$ libsrcdir = f$element(libsrc_elem,",",libsrc)
$ if (libsrcdir .eqs. ",") then goto END_LIBSRC
$ if f$search("''libsrcdir'''testinc'") .nes. "" then libsrc_found = true
$ libsrc_elem = libsrc_elem + 1
$ goto LIBSRC_LOOP
$END_LIBSRC:
$ if .not. libsrc_found
$ then
$   write sys$output "Can not find includes at ''libsrc' - Skipping ''libname'"
$   goto LIB_LOOP
$ endif
$ if (cppdef .nes. "") then libdefs = libdefs +  cppdef + ","
$ libincs = libincs + "," + libsrc
$ lqual = "/lib"
$ libtype = f$edit(f$parse(libloc,,,"TYPE"),"UPCASE")
$ if f$locate("EXE",libtype) .lt. f$length(libtype) then lqual = "/share"
$ write optf libloc , lqual
$ if (f$trnlnm("topt") .nes. "") then write topt libloc , lqual
$!
$! Nasty hack to get the FreeType includes to work
$!
$ ft2def = false
$ if ((libname .eqs. "FREETYPE") .and. -
      (f$locate("FREETYPE2",cppdef) .lt. f$length(cppdef)))
$ then
$   if ((f$search("freetype:freetype.h") .nes. "") .and. -
        (f$search("freetype:[internal]ftobjs.h") .nes. ""))
$   then
$     write sys$output "Will use local definition of freetype logical"
$   else
$     ft2elem = 0
$FT2_LOOP:
$     ft2srcdir = f$element(ft2elem,",",libsrc)
$     if f$search("''ft2srcdir'''testinc'") .nes. ""
$     then
$        if f$search("''ft2srcdir'internal.dir") .nes. ""
$        then
$          ft2dev  = f$parse("''ft2srcdir'",,,"device","no_conceal")
$          ft2dir  = f$parse("''ft2srcdir'",,,"directory","no_conceal")
$          ft2conc = f$locate("][",ft2dir)
$          ft2len  = f$length(ft2dir)
$          if ft2conc .lt. ft2len
$          then
$             ft2dir = f$extract(0,ft2conc,ft2dir) + -
                       f$extract(ft2conc+2,ft2len-2,ft2dir)
$          endif
$          ft2dir = ft2dir - "]" + ".]"
$          define freetype 'ft2dev''ft2dir','ft2srcdir'
$          ft2def = true
$        else
$          goto ft2_err
$        endif
$     else
$       ft2elem = ft2elem + 1
$       goto ft2_loop
$     endif
$   endif
$ endif
$ goto LIB_LOOP
$END_LIB:
$ close libdata
$ return
$!------------------------------------------------------------------------------
$!
$! Analyze Object files for OpenVMS AXP to extract Procedure and Data
$! information to build a symbol vector for a shareable image
$! All the "brains" of this logic was suggested by Hartmut Becker
$! (Hartmut.Becker@compaq.com). All the bugs were introduced by me
$! (zinser@zinser.no-ip.info), so if you do have problem reports please do not
$! bother Hartmut/HP, but get in touch with me
$!
$! Version history
$! 0.01 20040006 Skip over shareable images in option file
$!
$ ANAL_OBJ_AXP: Subroutine
$ V = 'F$Verify(0)
$ SAY := "WRITE_ SYS$OUTPUT"
$
$ IF F$SEARCH("''P1'") .EQS. ""
$ THEN
$    SAY "ANAL_OBJ_AXP-E-NOSUCHFILE:  Error, inputfile ''p1' not available"
$    goto exit_aa
$ ENDIF
$ IF "''P2'" .EQS. ""
$ THEN
$    SAY "ANAL_OBJ_AXP:  Error, no output file provided"
$    goto exit_aa
$ ENDIF
$
$ open/read in 'p1
$ create a.tmp
$ open/append atmp a.tmp
$ loop:
$ read/end=end_loop in line
$ if f$locate("/SHARE",f$edit(line,"upcase")) .lt. f$length(line)
$ then
$   write sys$output "ANAL_SKP_SHR-i-skipshare, ''line'"
$   goto loop
$ endif
$ if f$locate("/LIB",f$edit(line,"upcase")) .lt. f$length(line)
$ then
$   write libsf line
$   write sys$output "ANAL_SKP_LIB-i-skiplib, ''line'"
$   goto loop
$ endif
$ f= f$search(line)
$ if f .eqs. ""
$ then
$	write sys$output "ANAL_OBJ_AXP-w-nosuchfile, ''line'"
$	goto loop
$ endif
$ def/user sys$output nl:
$ def/user sys$error nl:
$ anal/obj/gsd 'f /out=x.tmp
$ open/read xtmp x.tmp
$ XLOOP:
$ read/end=end_xloop xtmp xline
$ xline = f$edit(xline,"compress")
$ write atmp xline
$ goto xloop
$ END_XLOOP:
$ close xtmp
$ goto loop
$ end_loop:
$ close in
$ close atmp
$ if f$search("a.tmp") .eqs. "" -
	then $ exit
$ ! all global definitions
$ search a.tmp "symbol:","EGSY$V_DEF 1","EGSY$V_NORM 1"/out=b.tmp
$ ! all procedures
$ search b.tmp "EGSY$V_NORM 1"/wind=(0,1) /out=c.tmp
$ search c.tmp "symbol:"/out=d.tmp
$ def/user sys$output nl:
$ edito/edt/command=sys$input d.tmp
sub/symbol: "/symbol_vector=(/whole
sub/"/=PROCEDURE)/whole
exit
$ ! all data
$ search b.tmp "EGSY$V_DEF 1"/wind=(0,1) /out=e.tmp
$ search e.tmp "symbol:"/out=f.tmp
$ def/user sys$output nl:
$ edito/edt/command=sys$input f.tmp
sub/symbol: "/symbol_vector=(/whole
sub/"/=DATA)/whole
exit
$ sort/nodupl d.tmp,f.tmp 'p2'
$ delete a.tmp;*,b.tmp;*,c.tmp;*,d.tmp;*,e.tmp;*,f.tmp;*
$ if f$search("x.tmp") .nes. "" -
	then $ delete x.tmp;*
$!
$ close libsf
$ EXIT_AA:
$ if V then set verify
$ endsubroutine
