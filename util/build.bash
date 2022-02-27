#!/usr/bin/env bash

# Include the env file.
[[ -f $ENV_FILE ]] && source "$ENV_FILE"

# Let bash parse the package.
source "$1"

# Better error handling.
set -e v

cd "$builddir"
S="$srcdir"
B="$builddir"
F="$filesdir"

echo "prepare()"
prepare

echo "build()"
build

D="$pkgdir"
package

if [[ -f $ENV_FILE ]]; then
    echo "pkg_clean()"
    pkg_clean
fi
