project(
  'minipkg2',
  'c',
  version: '0.2.8.1',
  default_options: [
    'c_std=gnu11',
    'warning_level=3',
    ]
  )

sources = [
  'src/cmdline.c',
  'src/download.c',
  'src/git.c',
  'src/main.c',
  'src/minipkg2.c',
  'src/miniconf.c',
  'src/op_build.c',
  'src/op_check.c',
  'src/op_clean.c',
  'src/op_config.c',
  'src/op_download.c',
  'src/op_help.c',
  'src/op_info.c',
  'src/op_install.c',
  'src/op_list.c',
  'src/op_remove_purge.c',
  'src/op_repo.c',
  'src/package.c',
  'src/print.c',
  'src/utils.c',
]

c_args = []

c_args += '-DVERSION="' + meson.project_version() + '"'
c_args += '-DCONFIG_PREFIX="' + get_option('prefix') + '"'
c_args += '-DCONFIG_SYSCONFDIR="' + get_option('sysconfdir') + '"'
c_args += '-DCONFIG_LIBDIR="' + get_option('libdir') + '"'
c_args += '-DBUILD_SYS="meson"'

# Check for optional libcurl dependency.
libcurl = dependency('libcurl', required: false)
if libcurl.found()
  c_args += '-DHAS_LIBCURL=1'
endif

executable('minipkg2',
  sources: sources,
  dependencies: [libcurl],
  include_directories: 'include',
  c_args: c_args,
  install: true
)

install_data('util/env.bash', install_dir: get_option('libdir') / 'minipkg2')
install_data('util/minipkg2.conf', install_dir: get_option('sysconfdir'))
