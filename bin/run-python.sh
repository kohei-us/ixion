#!/usr/bin/env bash

PROGDIR=`dirname $0`
IXION_PYTHONPATH="$PROGDIR/../src/python/.libs"

export PYTHONPATH=$IXION_PYTHONPATH
export LD_LIBRARY_PATH="$PROGDIR/../src/libixion/.libs"
export DYLD_LIBRARY_PATH=$LD_LIBRARY_PATH
exec $PWD/"$1"


