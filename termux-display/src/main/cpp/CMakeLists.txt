cmake_minimum_required(VERSION 3.22)
project(lorie)

set(can_use_assembler TRUE)
enable_language(ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

include(TestBigEndian)
include(CheckIncludeFile)
include(CheckSourceCompiles)
include(CheckCCompilerFlag)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckTypeSize)

find_package(Python3 REQUIRED)
find_package(BISON REQUIRED)

function(target_apply_patch patch_target d p)
    execute_process(COMMAND "bash" "-c" "patch -p1 -d ${d} -i ${p} -R --dry-run > /dev/null 2>&1 || patch -p1 -N -d ${d} -i ${p} -r -")
endfunction()

set(test_compile_options
        "-Wall"
        "-Wpointer-arith"
        "-Wmissing-declarations"
        "-Wformat=2"
        "-Winvalid-pch"
        "-Wstrict-prototypes"
        "-Wmissing-prototypes"
        "-Wnested-externs"
        "-Wbad-function-cast"
        "-Wold-style-definition"
        "-Wunused"
        "-Wuninitialized"
        "-Wshadow"
        "-Wmissing-noreturn"
        "-Wmissing-format-attribute"
        "-Wredundant-decls"
        "-Wlogical-op"
        "-Werror=implicit"
        "-Werror=nonnull"
        "-Werror=init-self"
        "-Werror=main"
        "-Werror=missing-braces"
        "-Werror=sequence-point"
        "-Werror=return-type"
        "-Werror=trigraphs"
        "-Werror=array-bounds"
        "-Werror=write-strings"
        "-Werror=address"
        "-Werror=int-to-pointer-cast"
        "-Werror=pointer-to-int-cast"
        "-fno-strict-aliasing"
        "-Wno-unused-but-set-variable"
        "-Wno-unused-parameter"
        "-Wno-unused-variable"
        "-Wno-unused-function"
        "-Wno-missing-prototypes"
        "-Wno-macro-redefined"
        "-Wno-uninitialized"
        "-Wno-shadow")

set(CMAKE_REQUIRED_QUIET_OLD ${CMAKE_REQUIRED_QUIET})
set(CMAKE_REQUIRED_QUIET 1)
set(common_compile_options)
set(I 0)
foreach (option ${test_compile_options})
    check_c_compiler_flag(${option} COMPILER_TEST_${I})
    if (COMPILER_TEST_${I})
        set(common_compile_options ${common_compile_options} ${option})
    endif()
    MATH(EXPR I "${I}+1")
endforeach ()
set(CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET_OLD})

foreach(project xorgproto fontenc md pixman tirpc xshmfence Xdmcp Xau Xfont2 xkbcomp xserver)
    include(recipes/${project}.cmake)
endforeach()

if(${ANDROID_ABI} MATCHES "arm64-v8a")
    add_subdirectory(OpenXR-SDK)
    add_library(XRio SHARED
                "xrio/android.c"
                "xrio/engine.c"
                "xrio/framebuffer.c"
                "xrio/input.c"
                "xrio/math.c"
                "xrio/renderer.c")
    target_link_options(XRio PRIVATE "-Wl,--as-needed" "-Wl,--no-undefined" "-fvisibility=hidden")
    target_link_libraries(XRio "-Wl,--whole-archive" "-Wl,--no-whole-archive" android log EGL GLESv2 openxr_loader)
endif()
