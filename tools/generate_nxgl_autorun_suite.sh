#!/usr/bin/env sh
set -eu

nxgl_dir="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
validation_dir="$nxgl_dir/validation"
suite_dir="$validation_dir/autorun_suite"
gen_dir="$suite_dir/generated"

mkdir -p "$gen_dir"

table="$gen_dir/probe_table.inc"
sources="$gen_dir/generated_sources.mk"

: > "$table"
: > "$sources"
printf 'AUTORUN_PROBE_SRCS := \\\n' >> "$sources"

find "$validation_dir" -maxdepth 1 -type d | sed "s#^$validation_dir/##" | \
    grep -E '^(2[7-9]|[3-9][0-9]|10[0-8])_gl' | sort -V | \
    while IFS= read -r probe; do
        safe="$(printf '%s' "$probe" | tr -c 'A-Za-z0-9' '_')"
        symbol="probe_${safe}_main"
        wrapper="$gen_dir/${safe}.c"
        cat > "$wrapper" <<EOF
#include "../autorun_hooks.h"

#define main ${symbol}
#define debugPrint nxgl_autorun_debug_print
#define Sleep nxgl_autorun_sleep
#include "../../${probe}/main.c"
EOF
        printf 'AUTORUN_PROBE("%s", %s)\n' "$probe" "$symbol" >> "$table"
        printf '\t$(CURDIR)/generated/%s.c \\\n' "$safe" >> "$sources"
    done

printf '\n' >> "$sources"
