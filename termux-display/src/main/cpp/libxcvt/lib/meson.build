libxcvt_sources = ['libxcvt.c']
libxcvt = shared_library('xcvt',
                         libxcvt_sources,
                         include_directories : inc,
                         version: meson.project_version(),
                         darwin_versions : ['1.0.0', '1.0.0' ],
                         install : true)
