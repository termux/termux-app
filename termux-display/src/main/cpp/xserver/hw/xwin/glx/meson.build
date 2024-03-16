python3 = import('python3')

# XWin requires OpenGL spec files in order to generate wrapper code for native GL functions
py3 = python3.find_python()
if run_command(py3, '-c', 'import lxml;').returncode() != 0
    error('python3 lxml module not found')
endif

khronos_spec_dir = dependency('khronos-opengl-registry').get_pkgconfig_variable('specdir')

gen_gl_wrappers_opts= ['-nodebug']
gen_gl_wrappers_cmd = ['env', 'PYTHONPATH=' + khronos_spec_dir, py3, files('./gen_gl_wrappers.py'), gen_gl_wrappers_opts]

wgl_wrappers = custom_target(
    'gen_wgl_wrappers',
    command: [gen_gl_wrappers_cmd, '-registry', '@INPUT@', '-prefix', 'wgl', '-wrapper', '-preresolve', '-outfile', '@OUTPUT@'],
    input: join_paths(khronos_spec_dir, 'wgl.xml'),
    output: 'generated_wgl_wrappers.ic',
    depend_files: join_paths(khronos_spec_dir, 'reg.py'),
)

gl_shim = custom_target(
    'gen_gl_shim',
    command: [gen_gl_wrappers_cmd, '-registry', '@INPUT@', '-shim', '-outfile', '@OUTPUT@'],
    input: join_paths(khronos_spec_dir, 'gl.xml'),
    output: 'generated_gl_shim.ic',
    depend_files: join_paths(khronos_spec_dir, 'reg.py'),
)

gl_thunks = custom_target(
    'gen_gl_thunks',
    command: [gen_gl_wrappers_cmd, '-registry', '@INPUT@', '-thunk', '-outfile', '@OUTPUT@'],
    input: join_paths(khronos_spec_dir, 'gl.xml'),
    output: 'generated_gl_thunks.ic',
    depend_files: join_paths(khronos_spec_dir, 'reg.py'),
)

gl_thunks_def = custom_target(
    'gen_gl_thunks_def',
    command: [gen_gl_wrappers_cmd, '-registry', '@INPUT@', '-thunkdefs', '-outfile', '@OUTPUT@'],
    input: join_paths(khronos_spec_dir, 'gl.xml'),
    output: 'generated_gl_thunks.def',
    depend_files: join_paths(khronos_spec_dir, 'reg.py'),
)

srcs_windows_glx = [
    'winpriv.c',
    'winpriv.h',
    'glwindows.h',
    'glshim.c',
    gl_shim,
    'indirect.c',
    'indirect.h',
    'wgl_ext_api.c',
    wgl_wrappers,
    'wgl_ext_api.h',
]

if build_windowsdri
    srcs_windows_glx += [
        'dri_helpers.c',
        'dri_helpers.h',
    ]
endif

xwin_glx_c_args = []
xwin_glx_c_args += '-DHAVE_XWIN_CONFIG_H'
xwin_glx_c_args += '-DXWIN_GLX_WINDOWS'

xwin_glx = static_library(
    'XwinGLX',
    srcs_windows_glx,
    include_directories: [
        inc,
        top_dir_inc,
        include_directories('../'),
    ],
    dependencies: common_dep,
    c_args: xwin_glx_c_args,
)

srcs_wgl_thunk = [
    'glthunk.c',
    gl_thunks,
]

WGLthunk = shared_library(
    'nativeGLthunk',
    srcs_wgl_thunk,
    include_directories: [
        inc,
        top_dir_inc,
    ],
    c_args: xwin_glx_c_args + [
      '-Wno-unused-function',
      '-Wno-missing-prototypes',
      '-Wno-missing-declarations',
    ],
    link_args: ['-lopengl32'],
    vs_module_defs: gl_thunks_def,
    install: true,
)
