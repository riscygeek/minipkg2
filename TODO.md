# TODO List for minipkg2


## General
- [ ] Support for binary packages (binpkgs)
- [ ] Copy Man page from [original minipkg](https://github.com/riscygeek/micro-linux/blob/e5e44de4fb51311958726bf58a0148af3f2b28dc/minipkg/minipkg.8)
- [ ] /etc/minipkg2.conf
- [ ] /usr/lib/minipkg2/env.bash
- [x] Support for option aliases (e.g. --yes for -y)
- [ ] Generate package.info files instead of copying package.build
- [ ] Add post-install script support
- [ ] Merge code for parsing global and per-operation options
- [ ] bash-completion
- [ ] zsh-completion


## Operations

### New
- [ ] download-sources
- [ ] clean-cache
- [ ] build (needs bdepends)
- [x] repo
- [ ] upgrade

### install
- [x] --clean-build
- [ ] --builddir=
- [ ] Replacing of packages/Proper removal of reinstalled packages.

### download-source
- [ ] --no-deps       (Don't download the dependencies)

### list
- [ ] --outdated or --upgradable

## package.build
- [ ] Support for provides & conflicts
- [ ] Split depends into {b,r,}depends
- [ ] arch