file(READ "freetype/builds/unix/ftconfig.h.in" FTCONFIG_H)
string(REGEX REPLACE "#undef +(HAVE_UNISTD_H)" "#define \\1 1" FTCONFIG_H "${FTCONFIG_H}")
string(REGEX REPLACE "#undef +(HAVE_FCNTL_H)" "#define \\1 1" FTCONFIG_H "${FTCONFIG_H}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/freetype/include/freetype/ftconfig")
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/freetype/include/freetype/config/ftconfig.h" CONTENT "${FTCONFIG_H}")

file(READ "freetype/include/freetype/config/ftoption.h" FTOPTION_H)
string(REGEX REPLACE "/\\* +(#define +FT_CONFIG_OPTION_SYSTEM_ZLIB) +\\*/" "\\1" FTOPTION_H "${FTOPTION_H}")
string(REGEX REPLACE "/\\* +(#define +FT_CONFIG_OPTION_USE_BZIP2) +\\*/" "\\1" FTOPTION_H "${FTOPTION_H}")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/freetype/include/freetype/config/ftoption.h" CONTENT "${FTOPTION_H}")

add_library(freetype
        "freetype/src/autofit/autofit.c"
        "freetype/src/base/ftbase.c"
        "freetype/src/base/ftbbox.c"
        "freetype/src/base/ftbdf.c"
        "freetype/src/base/ftbitmap.c"
        "freetype/src/base/ftcid.c"
        "freetype/src/base/ftfstype.c"
        "freetype/src/base/ftgasp.c"
        "freetype/src/base/ftglyph.c"
        "freetype/src/base/ftgxval.c"
        "freetype/src/base/ftinit.c"
        "freetype/src/base/ftmm.c"
        "freetype/src/base/ftotval.c"
        "freetype/src/base/ftpatent.c"
        "freetype/src/base/ftpfr.c"
        "freetype/src/base/ftstroke.c"
        "freetype/src/base/ftsynth.c"
        "freetype/src/base/fttype1.c"
        "freetype/src/base/ftwinfnt.c"
        "freetype/src/bdf/bdf.c"
        "freetype/src/bzip2/ftbzip2.c"
        "freetype/src/cache/ftcache.c"
        "freetype/src/cff/cff.c"
        "freetype/src/cid/type1cid.c"
        "freetype/src/gzip/ftgzip.c"
        "freetype/src/lzw/ftlzw.c"
        "freetype/src/pcf/pcf.c"
        "freetype/src/pfr/pfr.c"
        "freetype/src/psaux/psaux.c"
        "freetype/src/pshinter/pshinter.c"
        "freetype/src/psnames/psnames.c"
        "freetype/src/raster/raster.c"
        "freetype/src/sdf/sdf.c"
        "freetype/src/sfnt/sfnt.c"
        "freetype/src/smooth/smooth.c"
        "freetype/src/svg/svg.c"
        "freetype/src/truetype/truetype.c"
        "freetype/src/type1/type1.c"
        "freetype/src/type42/type42.c"
        "freetype/src/winfonts/winfnt.c"

        "freetype/builds/unix/ftsystem.c"
        "freetype/src/base/ftdebug.c"

        "bzip2/blocksort.c"
        "bzip2/huffman.c"
        "bzip2/crctable.c"
        "bzip2/randtable.c"
        "bzip2/compress.c"
        "bzip2/decompress.c"
        "bzip2/bzlib.c"
        )

target_include_directories(freetype
        PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/freetype/include" "freetype/include"
        PRIVATE "bzip2")
target_compile_options(freetype PRIVATE "-DFT2_BUILD_LIBRARY")
