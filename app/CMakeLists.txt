cmake_minimum_required(VERSION 3.4.1)

add_library(termux SHARED src/main/jni/termux.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra -Os -fno-stack-protector -Wl,--gc-sections")
