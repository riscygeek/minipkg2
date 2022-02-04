# This shell script will be sourced by minipkg2.

[[ $MINIPKG2 ]] || { echo 'minipkg2: $MINIPKG is not set. Please check your package manager version.' >&2; exit 1; }

__MINIPKG2_ENV=1

# Load the minipkg2 config.
declare -A config
eval "$("$MINIPKG2" --root="$ROOT" config --dump 2>/dev/null)"


# The $JOBS variable determines the amount of parallel workers per package.
if [[ -z $JOBS ]]; then
   JOBS="${config[build.jobs]}"
   [[ $JOBS = max ]] && JOBS="$(nproc)"
   JOBS="${JOBS:-1}"
fi


# Define some useful functions.

shopt -s expand_aliases

alias pmake="make -j '$JOBS'"


# This must be called from minipkg2:pkg_build() after package()
pkg_clean() {
   local suffixes s
   [[ $D ]] || { echo "\$D is not defined!" >&2; exit 1; }
   cd "$D" || { echo "Failed to ch into '$D'"; exit 1; }
   suffixes="${config[install.remove-suffixes]}"
   for s in ${suffixes}; do
      echo "Cleaning '.$s' files..."
      find . -name "*.$s" -print -delete
   done
}
