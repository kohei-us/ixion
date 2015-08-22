#!/usr/bin/env bash

PROGDIR=`dirname $0`
EXEC=$PROGDIR/../src/.libs/ixion-parser
export LD_LIBRARY_PATH=$PROGDIR/../src/libixion/.libs
export DYLD_LIBRARY_PATH=$LD_LIBRARY_PATH
$EXEC "$@"

