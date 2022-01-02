# TODO List for minipkg2


## General
[ ] Support for binary packages (binpkgs)
[ ] Copy Man page from [original minipkg](https://github.com/riscygeek/micro-linux/blob/e5e44de4fb51311958726bf58a0148af3f2b28dc/minipkg/minipkg.8)
[ ] /etc/minipkg2.conf
[ ] /usr/lib/minipkg2/env.bash
[ ] Support for option aliases (e.g. --yes for -y)
[ ] Generate package.info files instead of copying package.build
[ ] Add post-install script support


## Operations

### Finish
[ ] download-sources
[ ] clean-cache

### New
[ ] build (needs bdepends)
[ ] update or sync-repo
[ ] upgrade (needs update)


## New Options

### install
[ ] --clean-build   (Delete old build files before building)

### download-source
[ ] --no-deps       (Don't download the dependencies)


## package.build
[ ] Support for provides & conflicts
[ ] Split depends into {b,r,}depends
[ ] arch
