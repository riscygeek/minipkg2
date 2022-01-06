# TODO List for minipkg2


## General
- [ ] Support for binary packages (binpkgs)
- [ ] Copy Man page from [original minipkg](https://github.com/riscygeek/micro-linux/blob/e5e44de4fb51311958726bf58a0148af3f2b28dc/minipkg/minipkg.8)
- [ ] /etc/minipkg2.conf (util/miniconf.conf)
- [ ] /usr/lib/minipkg2/env.bash
- [x] Support for option aliases (e.g. --yes for -y)
- [ ] Generate package.info files instead of copying package.build
- [ ] Add post-install script support
- [x] Merge code for parsing global and per-operation options
- [ ] bash-completion
- [ ] zsh-completion
- [ ] Cross-compilation with $HOST variable (requires build operation & /etc/minipkg2.conf).
- [ ] Add support for multiple repos (eg. localrepo)
- [x] verbose versions of: remove()


## Operations

### New
- [x] download
- [x] clean
- [ ] build (needs bdepends)
- [x] repo
- [ ] upgrade
- [ ] search

### install
- [x] --clean-build
- [ ] --builddir
- [x] --no-deps
- [x] Replacing of packages/Proper removal of reinstalled packages.
- [ ] Installing of binary packages (.bmpkg.tar.gz)

### download
- [ ] --deps (Also download sources of dependencies)
- [ ] --dest (Put downloaded files into ...)

### list
- [x] --upgradable
- [ ] Improve output of --upgradable

### search
- [ ] -R,--regex
- [ ] -E,--extended-regex
- [ ] -f,--full (Search by name, version, description, url)


## package.build
- [ ] Support for provides & conflicts
- [x] Split depends into {b,r,}depends
- [ ] arch
- [ ] features=(cross-compile)
