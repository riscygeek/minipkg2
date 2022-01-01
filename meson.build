project('minipkg2', 'c')

sources = [
  'src/cmdline.c',
  'src/download.c',
  'src/main.c',
  'src/minipkg2.c',
  'src/op_info.c',
  'src/op_install.c',
  'src/op_list.c',
  'src/package.c',
  'src/print.c',
  'src/utils.c',
]

depends = [dependency('libcurl')]
c_args = ['-Wno-unused-value']
cpp_args = ['-DHAS_LIBCURL=1']

executable('minipkg2',
  sources: sources,
  dependencies: depends,
  include_directories: 'include',
  c_args: c_args,
  c_std: 'gnu11',
  cpp_args: cpp_args,
  install: true
  )
