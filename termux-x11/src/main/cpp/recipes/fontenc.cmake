add_library(fontenc STATIC "libfontenc/src/fontenc.c" "libfontenc/src/encparse.c" "libfontenc/src/reallocarray.c")
target_include_directories(fontenc PUBLIC "libfontenc/include")
target_link_libraries(fontenc PUBLIC xorgproto)
target_compile_options(fontenc PRIVATE "-DFONT_ENCODINGS_DIRECTORY=\"/\"")
