#!/usr/bin/env bash

PROGDIR=`dirname $0`
source $PROGDIR/ixion-common-func.sh
ixion_exec ixion-test "$PWD" "$@"

