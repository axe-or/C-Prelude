#!/usr/bin/env sh
set -eu

Output='a.h'

{
	sed 's/#include "prelude.h"//g' prelude.h
	echo '#ifdef PRELUDE_IMPLEMENTATION'
	cat prelude.c
	echo '#endif'
} > "$Output"

echo "Created single header prelude: $Output"

