file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pixman-config.h"
        CONTENT "")
file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pixman-version.h"
        CONTENT "
#pragma once
#ifndef PIXMAN_H__
#  error pixman-version.h should only be included by pixman.h
#endif

#define PIXMAN_VERSION_MAJOR 0
#define PIXMAN_VERSION_MINOR 43
#define PIXMAN_VERSION_MICRO 4

#define PIXMAN_VERSION_STRING \"0.43.4\"
#define PIXMAN_VERSION 4304

#ifndef PIXMAN_API
# define PIXMAN_API
#endif
")
check_type_size("long" SIZEOF_LONG)

set(PIXMAN_SRC
        pixman/pixman/pixman.c
        pixman/pixman/pixman-access.c
        pixman/pixman/pixman-access-accessors.c
        pixman/pixman/pixman-bits-image.c
        pixman/pixman/pixman-combine32.c
        pixman/pixman/pixman-combine-float.c
        pixman/pixman/pixman-conical-gradient.c
        pixman/pixman/pixman-filter.c
        pixman/pixman/pixman-x86.c
        pixman/pixman/pixman-mips.c
        pixman/pixman/pixman-arm.c
        pixman/pixman/pixman-ppc.c
        pixman/pixman/pixman-edge.c
        pixman/pixman/pixman-edge-accessors.c
        pixman/pixman/pixman-fast-path.c
        pixman/pixman/pixman-glyph.c
        pixman/pixman/pixman-general.c
        pixman/pixman/pixman-gradient-walker.c
        pixman/pixman/pixman-image.c
        pixman/pixman/pixman-implementation.c
        pixman/pixman/pixman-linear-gradient.c
        pixman/pixman/pixman-matrix.c
        pixman/pixman/pixman-noop.c
        pixman/pixman/pixman-radial-gradient.c
        pixman/pixman/pixman-region16.c
        pixman/pixman/pixman-region32.c
        pixman/pixman/pixman-solid-fill.c
        pixman/pixman/pixman-timer.c
        pixman/pixman/pixman-trap.c
        pixman/pixman/pixman-utils.c)

set(PIXMAN_CFLAGS
        "-DHAVE_BUILTIN_CLZ=1"
        "-DHAVE_PTHREADS=1"
        "-DPACKAGE=\"pixman\""
        "-DTLS=__thread"
        "-DSIZEOF_LONG=${SIZEOF_LONG}"
        "-DUSE_OPENMP=1"
        "-Wno-unknown-attributes")

if("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "arm64-v8a")
    set(PIXMAN_SRC ${PIXMAN_SRC}
            "pixman/pixman/pixman-arm-neon.c"
            "pixman/pixman/pixman-arma64-neon-asm.S"
            "pixman/pixman/pixman-arma64-neon-asm-bilinear.S")
    set(PIXMAN_CFLAGS ${PIXMAN_CFLAGS} "-DUSE_ARM_A64_NEON=1")
endif()

if("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "armeabi-v7a")
    set(PIXMAN_SRC ${PIXMAN_SRC}
            "pixman/pixman/pixman-arm-neon.c"
            "pixman/pixman/pixman-arm-neon-asm.S"
            "pixman/pixman/pixman-arm-neon-asm-bilinear.S"
            "pixman/pixman/pixman-arm-simd-asm.S"
            "pixman/pixman/pixman-arm-simd-asm-scaled.S")
    set(PIXMAN_CFLAGS ${PIXMAN_CFLAGS} "-DUSE_ARM_SIMD=1" "-DUSE_ARM_NEON=1" "-v")
endif()

if ("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "x86" OR "${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "x86_64")
    set(PIXMAN_CFLAGS ${PIXMAN_CFLAGS} "-msse2" "-Winline" "-mssse3" "-Winline")
endif()

add_library(pixman STATIC ${PIXMAN_SRC})
target_compile_options(pixman PRIVATE ${PIXMAN_CFLAGS})
target_include_directories(pixman PRIVATE pixman "${CMAKE_CURRENT_BINARY_DIR}")
target_apply_patch(pixman "${CMAKE_CURRENT_SOURCE_DIR}/pixman" "${CMAKE_CURRENT_SOURCE_DIR}/patches/pixman.patch")
