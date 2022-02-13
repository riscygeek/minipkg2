project(
  'minipkg2',
  'cpp',
  version: '0.4',
  default_options: [
    'cpp_std=gnu++17',
    'warning_level=3',
    ]
  )

sources = [
  'src/cmdline.cpp',
  'src/git.cpp',
  'src/main.cpp',
  'src/miniconf.cpp',
  'src/minipkg2.cpp',
  'src/op_help.cpp',
  'src/op_repo.cpp',
  'src/utils.cpp',
]

cpp_args = []
cpp_args += '-DVERSION="' + meson.project_version() + '"'
cpp_args += '-DCONFIG_PREFIX="' + get_option('prefix') + '"'
cpp_args += '-DCONFIG_SYSCONFDIR="' + get_option('sysconfdir') + '"'
cpp_args += '-DCONFIG_LIBDIR="' + get_option('libdir') + '"'
cpp_args += '-DBUILD_SYS="meson"'

# Check for optional libcurl dependency.
libcurl = dependency('libcurl', required: false)
if libcurl.found()
  cpp_args += '-DHAS_LIBCURL=1'
endif

executable('minipkg2',
  sources: sources,
  dependencies: [libcurl],
  include_directories: 'include',
  cpp_args: cpp_args,
  install: true
)

#install_data('util/env.bash', install_dir: get_option('libdir') / 'minipkg2')
#install_data('util/minipkg2.conf', install_dir: get_option('sysconfdir'))
