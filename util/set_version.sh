#!/bin/sh

{ [ -f Makefile ] && [ -f meson.build ]; } || { echo "Must be run in the top-level directory." >&2 && exit 1; }

sed -i "s/^VERSION\s*=.*$/VERSION = $1/g" Makefile
sed -i "s/^\\(\s*version:\s\\)'*.*',\s*$/\1'$1',/g" meson.build
