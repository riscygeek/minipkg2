# Package Manager for [Micro-Linux](https://github.com/riscygeek/micro-linux)

## Description
This is a package manager for my own [Micro-Linux](https://github.com/riscygeek/micro-linux) source-based distribution.
The command-line interface was inspired by Debian's [apt](https://en.wikipedia.org/wiki/APT_(software)) package manager
and the [package.build](#packagebuild-layout) file format was inspired by the [PKGBUILD](https://wiki.archlinux.org/title/PKGBUILD) format used by Arch Linux.
This is actually the second "version".
The [first version](https://github.com/riscygeek/micro-linux/tree/e5e44de4fb51311958726bf58a0148af3f2b28dc/minipkg) was written in bash
and after many hours of frustration I decided to rewrite it in C.

## Installation
This project supports 2 installation methods: traditional make, meson.

### Meson (recommended)
```
meson setup build
meson compile -C build
meson install -C build [--destdir=...]
```

### Make
```
make [-DLIBCURL=1] all
make [DESTDIR=...] install
```

## Filesystem Layout

### /var/db/minipkg2/packages
Each installed package has its own directory in `/var/db/minipkg2/packages/$pkgname`.
In this directory there are the following files:
- `files`: A list of installed files.
- `package.info`: See the [package.info Layout](#packageinfo-layout) section.

### /var/db/minipkg2/repo
Each installable package has its own directory in `/var/db/minipkg2/repo/$pkgname`.
In this directory there is currently only one file: [`package.build`](#packagebuild-layout).

### /var/tmp/minipkg2
When building a package these directories are created:
- `/var/tmp/minipkg2/${pkgname}-${pkgver}/src`: Downloaded files.
- `/var/tmp/minipkg2/${pkgname}-${pkgver}/pkg`: Package directory.
- `/var/tmp/minipkg2/${pkgname}-${pkgver}/build`: Build directory.

## package.build Layout
The `package.build` file format is inspired by Arch Linux's PKGBUILD format. \
Here's an example [hello-world](https://github.com/riscygeek/hello-world) package:
```
# Declare some self-explaining package details.
pkgname=hello-world
pkgver=1.0
url="https://github.com/riscygeek/hello-world"
description="Hello World example package."

# Specify the package sources.
# Note: As of now minipkg2 doesn't extract downloaded tarballs.
sources=(https://github.com/riscygeek/hello-world/archive/refs/tags/v${pkgver}.tar.gz)

# Define package dependencies (optional).
# Note: As of now minipkg2 doesn't distinguish
#       between runtime- and build-dependencies.
depends=()

# Define packages this package provides (optional).
# Note: This feature is currently unimplemented.
# Note 2: Packages added here are automatically added to conflicts.
provides=()

# Define packages this package conflicts with (optional).
# Note: This feature is currently unimplemented.
conflicts=()

# Prepare the package for build.
# You should apply patches at this step.
prepare() {
   # Extract the tarball.
   tar -xf "${S}/v${pkgver}.tar.gz"
   cd "hello-world-${pkgver}"
}

# Build the package.
# If applicable, you should run ./configure here.
build() {
   # Build the hello-world package.
   make
}

# Copy the result to the package directory.
package() {
   # Install the hello-world package to the package directory.
   # Note: Never install packages directly to /.
   make prefix=/usr DESTDIR="$D" install
}
```

## package.info Layout
The `package.info` format is currently the same as the `package.build` format.
With the exception that only the `pkgname`, `pkgver`, `url`, `description` and `depends` fields are required.
Optionally `provides` and `conflicts` can be specified.
As of now minipkg2 just uses the `package.build` of a package.
