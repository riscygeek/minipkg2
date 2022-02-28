#!/usr/bin/env bash

# Include the env file.
[[ -f $ENV_FILE ]] && source "$ENV_FILE"

# Let bash parse the package.
source "$1"

# Print the package.
echo "$pkgname"
echo "$pkgver"
echo "$url"
echo "$description"
for src in "${sources[@]}"; do
   echo "$src"
done
echo --
for pkg in "${depends[@]}" "${bdepends[@]}"; do
   echo "$pkg"
done
echo --
for pkg in "${depends[@]}" "${rdepends[@]}"; do
   echo "$pkg"
done
echo --
for pkg in "${provides[@]}"; do
   echo "$pkg"
done
echo --
for pkg in "${provides[@]}" "${conflicts[@]}"; do
   echo "$pkg"
done
echo --
for feature in "${features[@]}"; do
   echo "$feature"
done
echo --
echo "$build_date"
echo "$install_date"
exit 0
