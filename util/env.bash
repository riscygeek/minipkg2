# This shell script will be sourced by minipkg2.

# Load the minipkg2 config.
declare -A config
eval "$(minipkg2 --root="$ROOT" config --dump 2>/dev/null)"


# The $JOBS variable determines the amount of parallel workers per package.
if [[ -z $JOBS ]]; then
   JOBS="${config[build.jobs]}"
   JOBS="${JOBS:-1}"
fi


# Define some useful functions.

# Parallel make.
pmake() {
   make -j "${JOBS}"
}


