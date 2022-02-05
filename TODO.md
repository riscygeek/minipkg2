# TODO List for minipkg2


## General
- [ ] Support for binary packages (binpkgs)
- [ ] Copy Man page from [original minipkg](https://github.com/riscygeek/micro-linux/blob/e5e44de4fb51311958726bf58a0148af3f2b28dc/minipkg/minipkg.8)
- [x] /etc/minipkg2.conf (util/miniconf.conf)
- [x] /usr/lib/minipkg2/env.bash
- [x] Support for option aliases (e.g. --yes for -y)
- [ ] Generate package.info files instead of copying package.build
  - [ ] Add a way to set the install reason (eg. essential (Forbid removal), selected (Manually installed), automatic (eg. dependency))
- [x] Add post-install script support
- [x] Merge code for parsing global and per-operation options
- [ ] bash-completion
- [ ] zsh-completion
- [x] Cross-compilation with $HOST variable
- [ ] Add support for multiple repos (eg. localrepo)
- [x] verbose versions of: remove()
- [ ] Check provides and conflicts more in depth
- [ ] commandline: Support for fused options (like: -ys or -sy)
- [x] Parallel building with \$JOBS variable
- [ ] Package-specific options (Like USE flags on Gentoo).
- [x] Add support for -git packages

## Known Bugs
- [x] removing packages without files fails with "Arithmetic exception"

## Operations

### New
- [x] download
- [x] clean
- [x] build
- [x] repo
- [ ] upgrade
- [ ] search
- [x] check

### install
- [x] --clean-build
- [ ] --builddir
- [x] --skip-installed, -s
- [x] --no-deps
- [x] --jobs=N,-jN options for parallel building
- [x] Replacing of packages/Proper removal of reinstalled packages.
- [x] Installing of binary packages (.bmpkg.tar.gz)

### build
- [ ] --deps (Also build dependencies)

### download
- [ ] Make it more fancy
- [ ] --all (Download **all** available package sources from the repo)
- [x] --deps (Also download sources of dependencies)
- [ ] --bdeps, --rdeps (for more fine-grained control)
- [ ] --dest (Put downloaded files into ...)

### list
- [x] --upgradable
- [ ] Improve output of --upgradable

### search
- [ ] -R,--regex
- [ ] -E,--extended-regex
- [ ] -f,--full (Search by name, version, description, url)

### check
- [ ] If no options are given, check the validity of specified package arguments (--files + check package.info).
- [ ] --syntax (Check the syntax of .mpkg build files)
- [x] --files (Check if all files in a local package are actually installed or if they have been removed)
- [x] --system (Same as files, but checks all installed packages)
- [ ] --reinstall-broken (Maybe)
- [ ] --full (Perform a full system and repo check)
- [x] --werror (Error on warning)
- [ ] --toolchain

## package.build
- [x] Support for provides & conflicts
- [x] Split depends into {b,r,}depends
- [ ] arch=(any x86_64 ...)
- [ ] license=()
- [x] features=(cross-compile sysroot)
