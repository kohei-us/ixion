#!/usr/bin/env bash

PROGDIR=`dirname $0`
EXEC=$PROGDIR/../src/.libs/ixion-test
export LD_LIBRARY_PATH=$PROGDIR/../src/libixion/.libs
$EXEC "$@"

