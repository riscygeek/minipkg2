# This shell script will be sourced by minipkg2.

[[ $MINIPKG2 ]] || { echo 'minipkg2: $MINIPKG is not set. Please check your package manager version.' >&2; exit 1; }

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

# Parallel make.
pmake() {
   make -j "${JOBS}"
}


