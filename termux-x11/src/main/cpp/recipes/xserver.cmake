check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)
configure_file("patches/dix-config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/dix-config.h")

file(GENERATE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version-config.h
        CONTENT "
#pragma once
#define VENDOR_NAME \"The X.Org Foundation\"
#define VENDOR_RELEASE 12101099
")

file(GENERATE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xkb-config.h
        CONTENT "
#pragma once
#define XKB_BASE_DIRECTORY \"/usr/share/X11/xkb/\"
#define XKB_BIN_DIRECTORY \"\"
#define XKB_DFLT_LAYOUT \"us\"
#define XKB_DFLT_MODEL \"pc105\"
#define XKB_DFLT_OPTIONS \"\"
#define XKB_DFLT_RULES \"evdev\"
#define XKB_DFLT_VARIANT \"\"
#define XKM_OUTPUT_DIR (getenv(\"TMPDIR\") ?: \"/tmp\")
")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/xserver")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/xserver/GL")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xserver/GL/glext.h" CONTENT "#include <GL/gl.h>")
add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xserver/GL/gl.h"
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy/src/gen_dispatch.py"
        "--outputdir=${CMAKE_CURRENT_BINARY_DIR}/xserver/GL" "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy/registry/gl.xml"
        COMMENT "Generating source code (GL/gl.h)"
        VERBATIM)

set(inc "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/xserver"
        "libxfont/include"
        "pixman/pixman"
        "xorgproto/include"
        "libxkbfile/include"
        "xserver/Xext"
        "xserver/Xi"
        "xserver/composite"
        "xserver/damageext"
        "xserver/fb"
        "xserver/mi"
        "xserver/miext/damage"
        "xserver/miext/shadow"
        "xserver/miext/sync"
        "xserver/dbe"
        "xserver/dri3"
        "xserver/include"
        "xserver/present"
        "xserver/randr"
        "xserver/render"
        "xserver/xfixes"
        "xserver/glx")

set(compile_options
        ${common_compile_options}
        "-std=gnu99"
        "-DHAVE_DIX_CONFIG_H"
        "-fno-strict-aliasing"
        "-fvisibility=hidden"
        "-fPIC"
        "-D_DEFAULT_SOURCE"
        "-D_BSD_SOURCE"
        "-DHAS_FCHOWN"
        "-DHAS_STICKY_DIR_BIT"
        "-D_PATH_TMP=getenv(\"TMPDIR\")?:\"/tmp\""
        "-include" "${CMAKE_CURRENT_SOURCE_DIR}/lorie/shm/shm.h")
if (SIZEOF_UNSIGNED_LONG EQUAL 8)
    set(compile_options ${compile_options} "-D_XSERVER64=1")
endif ()

set(DIX_SOURCES
        atom.c colormap.c cursor.c devices.c dispatch.c dixfonts.c main.c dixutils.c enterleave.c
        events.c eventconvert.c extension.c gc.c gestures.c getevents.c globals.c glyphcurs.c
        grabs.c initatoms.c inpututils.c pixmap.c privates.c property.c ptrveloc.c region.c
        registry.c resource.c selection.c swaprep.c swapreq.c tables.c touch.c window.c)
list(TRANSFORM DIX_SOURCES PREPEND "xserver/dix/")
add_library(xserver_dix STATIC ${DIX_SOURCES})
target_include_directories(xserver_dix PRIVATE ${inc})
target_compile_options(xserver_dix PRIVATE ${compile_options})

add_library(xserver_main STATIC
        "xserver/dix/stubmain.c")
target_include_directories(xserver_main PRIVATE ${inc})
target_compile_options(xserver_main PRIVATE ${compile_options})

file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/drm_fourcc.h"
        CONTENT "
    #pragma once
    #define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
    #define DRM_FORMAT_RGB565	fourcc_code('R', 'G', '1', '6')
    #define DRM_FORMAT_XRGB8888	fourcc_code('X', 'R', '2', '4')
    #define DRM_FORMAT_XRGB2101010	fourcc_code('X', 'R', '3', '0')
    #define DRM_FORMAT_ARGB8888	fourcc_code('A', 'R', '2', '4')
    #define DRM_FORMAT_MOD_INVALID -1
")
add_library(xserver_dri3 STATIC
        "xserver/dri3/dri3.c"
        "xserver/dri3/dri3_request.c"
        "xserver/dri3/dri3_screen.c")
target_include_directories(xserver_dri3 PRIVATE ${inc})
target_compile_options(xserver_dri3 PRIVATE ${compile_options})

set(FB_SOURCES
        fballpriv.c fbarc.c fbbits.c fbblt.c fbbltone.c fbcmap_mi.c fbcopy.c fbfill.c fbfillrect.c
        fbfillsp.c fbgc.c fbgetsp.c fbglyph.c fbimage.c fbline.c fboverlay.c fbpict.c fbpixmap.c
        fbpoint.c fbpush.c fbscreen.c fbseg.c fbsetsp.c fbsolid.c fbtrap.c fbutil.c fbwindow.c)
list(TRANSFORM FB_SOURCES PREPEND "xserver/fb/")
add_library(xserver_fb STATIC ${FB_SOURCES})
target_include_directories(xserver_fb PRIVATE ${inc})
target_compile_options(xserver_fb PRIVATE ${compile_options})

set(MI_SOURCES
        miarc.c mibitblt.c micmap.c micopy.c midash.c midispcur.c mieq.c miexpose.c mifillarc.c
        mifillrct.c migc.c miglblt.c mioverlay.c mipointer.c mipoly.c mipolypnt.c mipolyrect.c
        mipolyseg.c mipolytext.c mipushpxl.c miscrinit.c misprite.c mivaltree.c miwideline.c
        miwindow.c mizerarc.c mizerclip.c mizerline.c)
list(TRANSFORM MI_SOURCES PREPEND "xserver/mi/")
add_library(xserver_mi STATIC ${MI_SOURCES})
target_include_directories(xserver_mi PRIVATE ${inc})
target_compile_options(xserver_mi PRIVATE ${compile_options})

set(OS_SOURCES
        WaitFor.c access.c auth.c backtrace.c client.c connection.c inputthread.c io.c mitauth.c
        oscolor.c osinit.c ospoll.c utils.c xdmauth.c xsha1.c xstrans.c xprintf.c log.c strlcpy.c
        busfault.c timingsafe_memcmp.c rpcauth.c xdmcp.c)
list(TRANSFORM OS_SOURCES PREPEND "xserver/os/")
add_library(xserver_os STATIC ${OS_SOURCES})
target_include_directories(xserver_os PRIVATE ${inc})
target_link_libraries(xserver_os PRIVATE md Xdmcp Xau tirpc)
target_compile_options(xserver_os PRIVATE ${compile_options} "-DCLIENTIDS")

set(COMPOSITE_SOURCES compalloc.c compext.c compinit.c compoverlay.c compwindow.c)
list(TRANSFORM COMPOSITE_SOURCES PREPEND "xserver/composite/")
add_library(xserver_composite STATIC ${COMPOSITE_SOURCES})
target_include_directories(xserver_composite PRIVATE ${inc})
target_compile_options(xserver_composite PRIVATE ${compile_options})

add_library(xserver_damageext STATIC "xserver/damageext/damageext.c")
target_include_directories(xserver_damageext PRIVATE ${inc})
target_compile_options(xserver_damageext PRIVATE ${compile_options})

add_library(xserver_dbe STATIC "xserver/dbe/dbe.c" "xserver/dbe/midbe.c")
target_include_directories(xserver_dbe PRIVATE ${inc})
target_compile_options(xserver_dbe PRIVATE ${compile_options})

add_library(xserver_miext_damage STATIC "xserver/miext/damage/damage.c")
target_include_directories(xserver_miext_damage PRIVATE ${inc})
target_compile_options(xserver_miext_damage PRIVATE ${compile_options})

set(MIEXT_SYNC_SOURCES misync.c misyncfd.c misyncshm.c)
list(TRANSFORM MIEXT_SYNC_SOURCES PREPEND "xserver/miext/sync/")
add_library(xserver_miext_sync STATIC ${MIEXT_SYNC_SOURCES})
target_include_directories(xserver_miext_sync PRIVATE ${inc})
target_compile_options(xserver_miext_sync PRIVATE ${compile_options})

set(PRESENT_SOURCES
        present.c present_event.c present_execute.c present_fake.c present_fence.c present_notify.c
        present_request.c present_scmd.c present_screen.c present_vblank.c)
list(TRANSFORM PRESENT_SOURCES PREPEND "xserver/present/")
add_library(xserver_present STATIC ${PRESENT_SOURCES})
target_include_directories(xserver_present PRIVATE ${inc})
target_compile_options(xserver_present PRIVATE ${compile_options})

set(RANDR_SOURCES
        randr.c rrcrtc.c rrdispatch.c rrinfo.c rrlease.c rrmode.c rrmonitor.c rroutput.c rrpointer.c
        rrproperty.c rrprovider.c rrproviderproperty.c rrscreen.c rrsdispatch.c rrtransform.c
        rrxinerama.c)
list(TRANSFORM RANDR_SOURCES PREPEND "xserver/randr/")
add_library(xserver_randr STATIC ${RANDR_SOURCES})
target_include_directories(xserver_randr PRIVATE ${inc})
target_compile_options(xserver_randr PRIVATE ${compile_options})

add_library(xserver_record STATIC xserver/record/record.c xserver/record/set.c)
target_include_directories(xserver_record PRIVATE ${inc})
target_compile_options(xserver_record PRIVATE ${compile_options})

set(RENDER_SOURCES
        animcur.c filter.c glyph.c matrix.c miindex.c mipict.c mirect.c mitrap.c mitri.c picture.c
        render.c)
list(TRANSFORM RENDER_SOURCES PREPEND "xserver/render/")
add_library(xserver_render STATIC ${RENDER_SOURCES})
target_include_directories(xserver_render PRIVATE ${inc})
target_compile_options(xserver_render PRIVATE ${compile_options})

set(XFIXES_SOURCES cursor.c disconnect.c region.c saveset.c select.c xfixes.c)
list(TRANSFORM XFIXES_SOURCES PREPEND "xserver/xfixes/")
add_library(xserver_xfixes STATIC ${XFIXES_SOURCES})
target_include_directories(xserver_xfixes PRIVATE ${inc})
target_compile_options(xserver_xfixes PRIVATE ${compile_options})

set(XKB_SOURCES
        ddxBeep.c ddxCtrls.c ddxLEDs.c ddxLoad.c maprules.c xkmread.c xkbtext.c xkbfmisc.c xkbout.c
        xkb.c xkbUtils.c xkbEvents.c xkbAccessX.c xkbSwap.c xkbLEDs.c xkbInit.c xkbActions.c
        xkbPrKeyEv.c XKBMisc.c XKBAlloc.c XKBGAlloc.c XKBMAlloc.c)
list(TRANSFORM XKB_SOURCES PREPEND "xserver/xkb/")
add_library(xserver_xkb STATIC ${XKB_SOURCES})
target_include_directories(xserver_xkb PRIVATE ${inc})
target_compile_options(xserver_xkb PRIVATE ${compile_options} "-DXkbFreeGeomOverlayKeys=XkbFreeGeomOverlayKeysInternal")

set(XKB_STUBS_SOURCES ddxKillSrv.c ddxPrivate.c ddxVT.c)
list(TRANSFORM XKB_STUBS_SOURCES PREPEND "xserver/xkb/")
add_library(xserver_xkb_stubs STATIC ${XKB_STUBS_SOURCES})
target_include_directories(xserver_xkb_stubs PRIVATE ${inc})
target_compile_options(xserver_xkb_stubs PRIVATE ${compile_options})

set(XEXT_SOURCES
        bigreq.c geext.c shape.c sleepuntil.c sync.c xcmisc.c xtest.c dpms.c shm.c hashtable.c
        xres.c saver.c xace.c xf86bigfont.c panoramiX.c panoramiXprocs.c panoramiXSwap.c xvmain.c
        xvdisp.c xvmc.c vidmode.c)
list(TRANSFORM XEXT_SOURCES PREPEND "xserver/Xext/")
add_library(xserver_xext STATIC ${XEXT_SOURCES})
target_include_directories(xserver_xext PRIVATE ${inc})
target_compile_options(xserver_xext PRIVATE ${compile_options})

set(XI_SOURCES
        allowev.c chgdctl.c chgfctl.c chgkbd.c chgkmap.c chgprop.c chgptr.c closedev.c devbell.c
        exevents.c extinit.c getbmap.c getdctl.c getfctl.c getfocus.c getkmap.c getmmap.c getprop.c
        getselev.c getvers.c grabdev.c grabdevb.c grabdevk.c gtmotion.c listdev.c opendev.c queryst.c
        selectev.c sendexev.c setbmap.c setdval.c setfocus.c setmmap.c setmode.c ungrdev.c ungrdevb.c
        ungrdevk.c xiallowev.c xibarriers.c xichangecursor.c xichangehierarchy.c xigetclientpointer.c
        xigrabdev.c xipassivegrab.c xiproperty.c xiquerydevice.c xiquerypointer.c xiqueryversion.c
        xiselectev.c xisetclientpointer.c xisetdevfocus.c xiwarppointer.c)
list(TRANSFORM XI_SOURCES PREPEND "xserver/Xi/")
add_library(xserver_xi STATIC ${XI_SOURCES})
target_include_directories(xserver_xi PRIVATE ${inc})
target_compile_options(xserver_xi PRIVATE ${compile_options})

add_library(xserver_xi_stubs STATIC "xserver/Xi/stubs.c")
target_include_directories(xserver_xi_stubs PRIVATE ${inc})
target_compile_options(xserver_xi_stubs PRIVATE ${compile_options})

set(GLX_SOURCES
        glxext.c indirect_dispatch.c indirect_dispatch_swap.c indirect_reqsize.c indirect_size_get.c
        indirect_table.c clientinfo.c createcontext.c extension_string.c indirect_util.c
        indirect_program.c indirect_texture_compression.c glxcmds.c glxcmdsswap.c glxext.c
        glxscreens.c render2.c render2swap.c renderpix.c renderpixswap.c rensize.c single2.c
        single2swap.c singlepix.c singlepixswap.c singlesize.c swap_interval.c xfont.c)
list(TRANSFORM GLX_SOURCES PREPEND "xserver/glx/")
add_library(xserver_glx STATIC ${GLX_SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/xserver/GL/gl.h")
target_include_directories(xserver_glx PRIVATE ${inc})
target_compile_options(xserver_glx PRIVATE ${compile_options})

set(GLXVND_SOURCES
        vndcmds.c vndext.c vndservermapping.c vndservervendor.c)
list(TRANSFORM GLXVND_SOURCES PREPEND "xserver/glx/")
add_library(xserver_glxvnd STATIC ${GLXVND_SOURCES})
target_include_directories(xserver_glxvnd PRIVATE ${inc})
target_compile_options(xserver_glxvnd PRIVATE ${compile_options})

set(XSERVER_LIBS tirpc Xdmcp Xau pixman Xfont2 fontenc GLESv2 xshmfence xkbcomp)
foreach (part glx glxvnd fb mi dix composite damageext dbe randr miext_damage render present xext
         dri3 miext_sync xfixes xi xkb record xi_stubs xkb_stubs os)
    set(XSERVER_LIBS ${XSERVER_LIBS} xserver_${part})
endforeach ()

add_library(Xlorie SHARED
        "xserver/mi/miinitext.c"
        "xserver/hw/xquartz/keysym2ucs.c"
        "libxcvt/lib/libxcvt.c"
        "lorie/shm/shmem.c"
        "lorie/android.c"
        "lorie/clipboard.c"
        "lorie/dri3.c"
        "lorie/InitOutput.c"
        "lorie/InitInput.c"
        "lorie/InputXKB.c"
        "lorie/renderer.c")
target_include_directories(Xlorie PRIVATE ${inc} "libxcvt/include")
target_link_options(Xlorie PRIVATE "-Wl,--as-needed" "-Wl,--no-undefined" "-fvisibility=hidden")
target_link_libraries(Xlorie "-Wl,--whole-archive" ${XSERVER_LIBS} "-Wl,--no-whole-archive" android log m z EGL GLESv2)
target_compile_options(Xlorie PRIVATE ${compile_options})
target_apply_patch(Xlorie "${CMAKE_CURRENT_SOURCE_DIR}/xserver" "${CMAKE_CURRENT_SOURCE_DIR}/patches/xserver.patch")
target_apply_patch(Xlorie "${CMAKE_CURRENT_SOURCE_DIR}/libepoxy" "${CMAKE_CURRENT_SOURCE_DIR}/patches/libepoxy.patch")
