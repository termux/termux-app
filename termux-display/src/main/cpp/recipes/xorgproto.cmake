file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/X11")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/X11/XlibConf.h" CONTENT "\n#pragma once\n#define XTHREADS 1\n#define XUSE_MTSAFE_API 1")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/libxtrans" "${CMAKE_CURRENT_BINARY_DIR}/X11/Xtrans")

add_library(xorgproto INTERFACE)
target_include_directories(xorgproto INTERFACE "xorgproto/include")
set(USE_FDS_BITS 1)
configure_file("xorgproto/include/X11/Xpoll.h.in" "${CMAKE_CURRENT_BINARY_DIR}/X11/Xpoll.h" @ONLY)
target_apply_patch(Xtrans "${CMAKE_CURRENT_SOURCE_DIR}/libxtrans" "${CMAKE_CURRENT_SOURCE_DIR}/patches/Xtrans.patch")