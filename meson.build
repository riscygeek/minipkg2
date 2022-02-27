project(
  'minipkg2',
  'cpp',
  version: '0.4',
  default_options: [
    'cpp_std=gnu++17',
    'warning_level=3',
    'default_library=static'
  ],
)

sources = [
  'src/cmdline.cpp',
  'src/download.cpp',
  'src/git.cpp',
  'src/main.cpp',
  'src/miniconf.cpp',
  'src/minipkg2.cpp',
  'src/op_config.cpp',
  'src/op_download.cpp',
  'src/op_help.cpp',
  'src/op_info.cpp',
  'src/op_install.cpp',
  'src/op_list.cpp',
  'src/op_repo.cpp',
  'src/package.cpp',
  'src/utils.cpp',
]

cpp_args  = []
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

libfmt = dependency('fmt', fallback: ['fmt', 'fmt_dep'])

executable('minipkg2',
  sources: sources,
  dependencies: [libcurl, libfmt],
  include_directories: 'include',
  cpp_args: cpp_args,
  install: true
)

install_data('util/env.bash',       install_dir: get_option('libdir') / 'minipkg2')
install_data('util/parse.bash',     install_dir: get_option('libdir') / 'minipkg2')
install_data('util/build.bash',     install_dir: get_option('libdir') / 'minipkg2')
install_data('util/minipkg2.conf',  install_dir: get_option('sysconfdir'))
