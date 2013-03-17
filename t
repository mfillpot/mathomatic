#!/bin/sh
# Shell script to test Mathomatic.

if make --version >/dev/null 2>&1
then
	set -x
	make test
else
	set -x
	gmake test
fi
