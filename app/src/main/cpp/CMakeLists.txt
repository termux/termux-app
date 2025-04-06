# won't this help more than mk?
cmake_minimum_required(VERSION 3.10)
project(libtermux-bootstrap C)
set(SOURCES
        termux-bootstrap-zip.S
        termux-bootstrap.c
)
add_library(termux-bootstrap SHARED ${SOURCES})
