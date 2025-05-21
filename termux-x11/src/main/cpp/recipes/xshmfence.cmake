file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/values.h" CONTENT "#include <limits.h>")
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/libxshmfence/src/xshmfence.h" "${CMAKE_CURRENT_BINARY_DIR}/X11/xshmfence.h")
add_library(xshmfence_pthread STATIC "libxshmfence/src/xshmfence_alloc.c" "libxshmfence/src/xshmfence_pthread.c")
target_include_directories(xshmfence_pthread PRIVATE "xorgproto/include" "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(xshmfence_pthread PRIVATE "-DSHMDIR=\"/\"" "-DHAVE_PTHREAD" "-DMAXINT=INT_MAX" "-Dxshmfence_trigger=xshmfence_pthread_trigger" "-Dxshmfence_await=xshmfence_pthread_await" "-Dxshmfence_query=xshmfence_pthread_query" "-Dxshmfence_reset=xshmfence_pthread_reset" "-Dxshmfence_alloc_shm=xshmfence_pthread_alloc_shm" "-Dxshmfence_map_shm=xshmfence_pthread_map_shm" "-Dxshmfence_unmap_shm=xshmfence_pthread_unmap_shm")

add_library(xshmfence_futex STATIC "libxshmfence/src/xshmfence_alloc.c" "libxshmfence/src/xshmfence_futex.c")
target_include_directories(xshmfence_futex PRIVATE "xorgproto/include" "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(xshmfence_futex PRIVATE "-DSHMDIR=\"/\"" "-DHAVE_FUTEX" "-DMAXINT=INT_MAX" "-Dxshmfence_trigger=xshmfence_futex_trigger" "-Dxshmfence_await=xshmfence_futex_await" "-Dxshmfence_query=xshmfence_futex_query" "-Dxshmfence_reset=xshmfence_futex_reset" "-Dxshmfence_alloc_shm=xshmfence_futex_alloc_shm" "-Dxshmfence_map_shm=xshmfence_futex_map_shm" "-Dxshmfence_unmap_shm=xshmfence_futex_unmap_shm")

add_library(xshmfence STATIC "lorie/xshmfence.c")
target_link_libraries(xshmfence PRIVATE xshmfence_pthread xshmfence_futex)