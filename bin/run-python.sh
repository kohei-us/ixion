#!/usr/bin/env bash

PROGDIR=`dirname $0`
_PYTHONPATH="$PROGDIR/../src/python/.libs:$PROGDIR/../src/python"

export PYTHONPATH=$_PYTHONPATH
export LD_LIBRARY_PATH="$PROGDIR/../src/libixion/.libs"
export DYLD_LIBRARY_PATH=$LD_LIBRARY_PATH

exec $PWD/"$1"


