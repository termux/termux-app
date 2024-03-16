# Configuration file
configure_file(output: 'config.h', configuration: conf)

# List of generated sources:
#   - name of the generated file
#   - registry source file
#   - additional sources
generated_sources = [
  [ 'gl_generated_dispatch.c', gl_registry, [ 'dispatch_common.c', 'dispatch_common.h' ] ]
]

if build_egl
  generated_sources += [ [ 'egl_generated_dispatch.c', egl_registry, 'dispatch_egl.c' ] ]
endif

if build_glx
  generated_sources += [ [ 'glx_generated_dispatch.c', glx_registry, 'dispatch_glx.c' ] ]
endif

if build_wgl
  generated_sources += [ [ 'wgl_generated_dispatch.c', wgl_registry, 'dispatch_wgl.c' ] ]
endif

gen_sources = [ ]
sources = [ ]

foreach g: generated_sources
  gen_source = g[0]
  registry = g[1]
  source = g[2]

  generated = custom_target(gen_source,
                            input: registry,
                            output: [ gen_source ],
                            command: [
                              gen_dispatch_py,
                              '--source',
                              '--no-header',
                              '--outputdir=@OUTDIR@',
                              '@INPUT@',
                            ])

  gen_sources += [ generated ]
  sources += [ source ]
endforeach

epoxy_sources = sources + gen_sources

common_ldflags = []

if host_system == 'linux' and cc.get_id() == 'gcc'
  common_ldflags += cc.get_supported_link_arguments([ '-Wl,-Bsymbolic-functions', '-Wl,-z,relro' ])
endif

# Maintain compatibility with autotools; see: https://github.com/anholt/libepoxy/issues/108
darwin_versions = [1, '1.0']

epoxy_deps = [ dl_dep, ]
if host_system == 'windows'
  epoxy_deps += [ opengl32_dep, gdi32_dep ]
endif
if enable_x11
  epoxy_deps += [ x11_headers_dep, ]
endif
if build_egl
  epoxy_deps += [ elg_headers_dep, ]
endif

libepoxy = library(
  'epoxy',
  sources: epoxy_sources + epoxy_headers,
  version: '0.0.0',
  darwin_versions: darwin_versions,
  install: true,
  dependencies: epoxy_deps,
  include_directories: libepoxy_inc,
  c_args: common_cflags + visibility_cflags,
  link_args: common_ldflags,
)

epoxy_has_glx = build_glx ? '1' : '0'
epoxy_has_egl = build_egl ? '1' : '0'
epoxy_has_wgl = build_wgl ? '1' : '0'

libepoxy_dep = declare_dependency(
  link_with: libepoxy,
  include_directories: libepoxy_inc,
  dependencies: epoxy_deps,
  sources: epoxy_headers,
  variables: {
    'epoxy_has_glx': epoxy_has_glx,
    'epoxy_has_egl': epoxy_has_egl,
    'epoxy_has_wgl': epoxy_has_wgl,
  },
)

# We don't want to add these dependencies to the library, as they are
# not needed when building Epoxy; we do want to add them to the generated
# pkg-config file, for consumers of Epoxy
gl_reqs = []
if gl_dep.found() and gl_dep.type_name() == 'pkgconfig'
  gl_reqs += 'gl'
endif
if build_egl and egl_dep.found() and egl_dep.type_name() == 'pkgconfig'
  gl_reqs += 'egl'
endif

pkg = import('pkgconfig')
pkg.generate(
  libraries: libepoxy,
  name: 'epoxy',
  description: 'GL dispatch library',
  version: meson.project_version(),
  variables: [
    'epoxy_has_glx=@0@'.format(epoxy_has_glx),
    'epoxy_has_egl=@0@'.format(epoxy_has_egl),
    'epoxy_has_wgl=@0@'.format(epoxy_has_wgl),
  ],
  filebase: 'epoxy',
  requires_private: ' '.join(gl_reqs),
)
